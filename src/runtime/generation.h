#ifndef XS_RUNTIME_GENERATION_H
#define XS_RUNTIME_GENERATION_H

/*
 * 服务代生命周期。
 *
 * 一个 generation 初始持有一个管理引用。监听器、连接、UDP socket、
 * idle/lifecycle timer 等异步资源各自持有引用，并且只在各自唯一的终态
 * 回调中释放。退役只标记状态并撤销管理引用；最后一个资源结束时同步触发
 * finalizer。这里没有宽限时间，也没有超时强制释放路径。
 */

#include "../sdk/xsbase.h"

typedef struct XS_ServerGeneration XS_ServerGeneration;
typedef struct XS_GenerationTimer XS_GenerationTimer;
typedef void (*XS_GenerationFinalizeProc)(XS_ServerGeneration* pGeneration);

struct XS_GenerationTimer {
	XS_GenerationTimer*	pNext;
	XS_ServerGeneration*	pGeneration;
	void*			pOwner;		/* 通常为 XS_ScriptRuntime* */
	uint64			iId;
};

struct XS_ServerGeneration {
	volatile int32		iReferences;	/* 管理引用 1 + 所有异步资源 */
	xatomic32		tRetired;
	xatomic32		tConnections;	/* 仅用于状态/测试；安全性由引用决定 */
	xmutex*			pTimerLock;
	XS_GenerationTimer*	pTimers;	/* lifecycle timer；退役时全部取消 */
	XS_ServerInfo*		pServer;
	void*			pDriverRuntime;
	void*			pConfigOwner;
	bool			bFreeServer;
	XS_GenerationFinalizeProc	procFinalize;
};

static XS_ServerGeneration* XS_GenerationCreate(XS_ServerInfo* pServer)
{
	XS_ServerGeneration* pGeneration =
		(XS_ServerGeneration*)xrtCalloc(1, sizeof(XS_ServerGeneration));

	if ( pGeneration == NULL ) {
		return NULL;
	}
	pGeneration->pTimerLock = xrtMutexCreate();
	if ( pGeneration->pTimerLock == NULL ) {
		xrtFree(pGeneration);
		return NULL;
	}
	pGeneration->iReferences = 1;
	xrtAtomic32Init(&pGeneration->tRetired, 0);
	xrtAtomic32Init(&pGeneration->tConnections, 0);
	pGeneration->pServer = pServer;
	pServer->Generation = pGeneration;
	return pGeneration;
}

static bool XS_GenerationRetain(XS_ServerGeneration* pGeneration)
{
	return pGeneration != NULL && xrtRefRetain(&pGeneration->iReferences) > 0;
}

static bool XS_GenerationIsRetired(const XS_ServerGeneration* pGeneration)
{
	return pGeneration == NULL ||
	       xrtAtomic32Load(&pGeneration->tRetired, XMEMORY_ACQUIRE) != 0;
}

static void XS_GenerationRelease(XS_ServerGeneration* pGeneration)
{
	if ( pGeneration != NULL && xrtRefRelease(&pGeneration->iReferences) == 0 ) {
		/* procFinalize 在撤销管理引用之前发布，故归零时必定可见。 */
		pGeneration->procFinalize(pGeneration);
	}
}

static uint64 XS_GenerationTimerSchedule(
	XS_ServerGeneration* pGeneration,
	uint64 iTimeoutUs,
	xnettimerproc procTimer,
	ptr pData,
	void* pOwner,
	XS_GenerationTimer* pTimer)
{
	XS_GenerationTimer** ppLink;
	uint64 iId = 0;

	if ( pGeneration == NULL || pTimer == NULL || procTimer == NULL ) {
		return 0;
	}
	xrtMutexLock(pGeneration->pTimerLock);
	if ( !XS_GenerationIsRetired(pGeneration) && XS_GenerationRetain(pGeneration) ) {
		pTimer->pGeneration = pGeneration;
		pTimer->pOwner = pOwner;
		pTimer->pNext = pGeneration->pTimers;
		pGeneration->pTimers = pTimer;
		iId = xrtNetEngineAfter(pGeneration->pServer->Engine, 0,
			iTimeoutUs, procTimer, pData);
		pTimer->iId = iId;
		if ( iId == 0 ) {
			for ( ppLink = &pGeneration->pTimers; *ppLink != NULL;
			      ppLink = &(*ppLink)->pNext ) {
				if ( *ppLink == pTimer ) {
					*ppLink = pTimer->pNext;
					break;
				}
			}
			pTimer->pGeneration = NULL;
			XS_GenerationRelease(pGeneration);
		}
	}
	xrtMutexUnlock(pGeneration->pTimerLock);
	return iId;
}

/* 必须由 Timer 的唯一终态回调调用；返回其持有的 generation 引用。 */
static XS_ServerGeneration* XS_GenerationTimerFinish(XS_GenerationTimer* pTimer)
{
	XS_ServerGeneration* pGeneration;
	XS_GenerationTimer** ppLink;

	if ( pTimer == NULL || pTimer->pGeneration == NULL ) {
		return NULL;
	}
	pGeneration = pTimer->pGeneration;
	xrtMutexLock(pGeneration->pTimerLock);
	for ( ppLink = &pGeneration->pTimers; *ppLink != NULL;
	      ppLink = &(*ppLink)->pNext ) {
		if ( *ppLink == pTimer ) {
			*ppLink = pTimer->pNext;
			break;
		}
	}
	pTimer->pGeneration = NULL;
	pTimer->pNext = NULL;
	xrtMutexUnlock(pGeneration->pTimerLock);
	return pGeneration;
}

static void XS_GenerationTimerCancelOwner(XS_ServerGeneration* pGeneration, void* pOwner)
{
	XS_GenerationTimer* pTimer;

	if ( pGeneration == NULL || pOwner == NULL || pGeneration->pTimerLock == NULL ) return;
	xrtMutexLock(pGeneration->pTimerLock);
	for ( pTimer = pGeneration->pTimers; pTimer != NULL; pTimer = pTimer->pNext ) {
		if ( pTimer->pOwner == pOwner && pTimer->iId != 0 &&
		     pGeneration->pServer->Engine != NULL ) {
			(void)xrtNetEngineTimerCancel(pGeneration->pServer->Engine, pTimer->iId);
		}
	}
	xrtMutexUnlock(pGeneration->pTimerLock);
}

static bool XS_GenerationConnectionAcquire(XS_ServerGeneration* pGeneration)
{
	if ( !XS_GenerationRetain(pGeneration) ) {
		return false;
	}
	xrtAtomic32FetchAdd(&pGeneration->tConnections, 1, XMEMORY_RELAXED);
	return true;
}

static void XS_GenerationConnectionRelease(XS_ServerGeneration* pGeneration)
{
	if ( pGeneration == NULL ) {
		return;
	}
	xrtAtomic32FetchSub(&pGeneration->tConnections, 1, XMEMORY_RELAXED);
	XS_GenerationRelease(pGeneration);
}

static uint32 XS_GenerationConnectionCount(const XS_ServerGeneration* pGeneration)
{
	return pGeneration != NULL ?
		xrtAtomic32Load(&pGeneration->tConnections, XMEMORY_ACQUIRE) : 0;
}

static void XS_GenerationRetire(
	XS_ServerGeneration* pGeneration,
	void* pDriverRuntime,
	void* pConfigOwner,
	bool bFreeServer,
	XS_GenerationFinalizeProc procFinalize)
{
	if ( pGeneration == NULL || procFinalize == NULL ) {
		return;
	}
	if ( xrtAtomic32Exchange(&pGeneration->tRetired, 1, XMEMORY_ACQ_REL) != 0 ) {
		return;
	}
	pGeneration->pDriverRuntime = pDriverRuntime;
	pGeneration->pConfigOwner = pConfigOwner;
	pGeneration->bFreeServer = bFreeServer;
	pGeneration->procFinalize = procFinalize;
	/* lifecycle timer 不属于要继续排空的连接；退役即请求其终态取消。 */
	xrtMutexLock(pGeneration->pTimerLock);
	{
		XS_GenerationTimer* pTimer;

		for ( pTimer = pGeneration->pTimers; pTimer != NULL; pTimer = pTimer->pNext ) {
			if ( pTimer->iId != 0 && pGeneration->pServer->Engine != NULL ) {
				(void)xrtNetEngineTimerCancel(pGeneration->pServer->Engine, pTimer->iId);
			}
		}
	}
	xrtMutexUnlock(pGeneration->pTimerLock);
	XS_GenerationRelease(pGeneration);	/* 撤销唯一的管理引用 */
}

#endif
