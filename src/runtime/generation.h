#ifndef XS_RUNTIME_GENERATION_H
#define XS_RUNTIME_GENERATION_H

/*
 * 服务代生命周期。
 *
 * 一个 generation 初始持有一个管理引用。监听器、连接、UDP socket、
 * idle/lifecycle timer 与公开 server lease 各自持有引用，并且只在各自唯一
 * 的终态释放。退役代最后一个脚本活动结束时先通知 ServiceUnit；最后一个
 * 总引用结束时同步触发 finalizer。这里没有宽限时间或超时强制释放路径。
 */

#include "../sdk/xsbase.h"

typedef struct XS_ServerGeneration XS_ServerGeneration;
typedef struct XS_GenerationTimer XS_GenerationTimer;
typedef void (*XS_GenerationQuiesceProc)(XS_ServerGeneration* pGeneration);
typedef void (*XS_GenerationFinalizeProc)(XS_ServerGeneration* pGeneration);

struct XS_GenerationTimer {
	XS_GenerationTimer*	pNext;
	XS_ServerGeneration*	pGeneration;
	void*			pOwner;		/* 通常为 XS_ScriptRuntime* */
	uint64			iId;
};

struct XS_ServerGeneration {
	volatile int32		iReferences;	/* 管理引用 1 + 所有异步资源 */
	xatomic32		tRetired;	/* 0 active；1 正在发布终态；2 已完整发布 */
	xatomic32		tConnections;	/* 仅用于状态/测试；安全性由引用决定 */
	xatomic32		tActivities;	/* 会进入脚本的连接/UDP 回调/lifecycle timer */
	xatomic32		tQuiesced;	/* ServiceUnit 提前通知至多一次 */
	xmutex*			pTimerLock;
	XS_GenerationTimer*	pTimers;	/* lifecycle timer；退役时全部取消 */
	XS_ServerInfo*		pServer;
	void*			pDriverRuntime;
	void*			pConfigOwner;
	bool			bFreeServer;
	XS_GenerationQuiesceProc	procQuiesce;
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
	xrtAtomic32Init(&pGeneration->tActivities, 0);
	xrtAtomic32Init(&pGeneration->tQuiesced, 0);
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

/* 调用点必须仍持有至少一个 generation 引用，防止 quiesce 回调释放外部 lease 后归零。 */
static void XS_GenerationMaybeQuiesce(XS_ServerGeneration* pGeneration)
{
	if ( pGeneration != NULL &&
	     xrtAtomic32Load(&pGeneration->tRetired, XMEMORY_ACQUIRE) == 2 &&
	     xrtAtomic32Load(&pGeneration->tActivities, XMEMORY_ACQUIRE) == 0 &&
	     pGeneration->procQuiesce != NULL &&
	     xrtAtomic32Exchange(&pGeneration->tQuiesced, 1, XMEMORY_ACQ_REL) == 0 ) {
		pGeneration->procQuiesce(pGeneration);
	}
}

static void XS_GenerationActivityRelease(XS_ServerGeneration* pGeneration)
{
	if ( pGeneration == NULL ) return;
	xrtAtomic32FetchSub(&pGeneration->tActivities, 1, XMEMORY_ACQ_REL);
	XS_GenerationMaybeQuiesce(pGeneration);
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
	bool bReleaseFailed = false;

	if ( pGeneration == NULL || pTimer == NULL || procTimer == NULL ) {
		return 0;
	}
	xrtMutexLock(pGeneration->pTimerLock);
	if ( !XS_GenerationIsRetired(pGeneration) && XS_GenerationRetain(pGeneration) ) {
		xrtAtomic32FetchAdd(&pGeneration->tActivities, 1, XMEMORY_ACQ_REL);
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
			bReleaseFailed = true;
		}
	}
	xrtMutexUnlock(pGeneration->pTimerLock);
	/* MaybeQuiesce 可调用 ServiceUnit，而 ServiceUnit 允许再次调用 xsAfter；
	 * 因此失败引用必须在 timer 锁外撤销，避免同锁重入。 */
	if ( bReleaseFailed ) {
		XS_GenerationActivityRelease(pGeneration);
		XS_GenerationRelease(pGeneration);
	}
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
	xrtAtomic32FetchAdd(&pGeneration->tActivities, 1, XMEMORY_ACQ_REL);
	return true;
}

static void XS_GenerationConnectionRelease(XS_ServerGeneration* pGeneration)
{
	if ( pGeneration == NULL ) {
		return;
	}
	xrtAtomic32FetchSub(&pGeneration->tConnections, 1, XMEMORY_RELAXED);
	XS_GenerationActivityRelease(pGeneration);
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
	XS_GenerationQuiesceProc procQuiesce,
	XS_GenerationFinalizeProc procFinalize)
{
	uint32 iExpected = 0;

	if ( pGeneration == NULL || procFinalize == NULL ) {
		return;
	}
	/* 先用 preparing 状态封住新 timer/activity，再发布全部终态字段。
	 * MaybeQuiesce 只接受状态 2，因此不会并发读取尚未写完的回调指针。 */
	if ( !xrtAtomic32CompareExchange(&pGeneration->tRetired, &iExpected, 1,
	     XMEMORY_ACQ_REL, XMEMORY_ACQUIRE) ) {
		return;
	}
	pGeneration->pDriverRuntime = pDriverRuntime;
	pGeneration->pConfigOwner = pConfigOwner;
	pGeneration->bFreeServer = bFreeServer;
	pGeneration->procQuiesce = procQuiesce;
	pGeneration->procFinalize = procFinalize;
	xrtAtomic32Store(&pGeneration->tRetired, 2, XMEMORY_RELEASE);
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
	XS_GenerationMaybeQuiesce(pGeneration);
	XS_GenerationRelease(pGeneration);	/* 撤销唯一的管理引用 */
}

#endif
