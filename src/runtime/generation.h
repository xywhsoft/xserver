#ifndef XS_RUNTIME_GENERATION_H
#define XS_RUNTIME_GENERATION_H

#include <string.h>

/*
 * 服务代生命周期。
 *
 * 一个 generation 初始持有一个管理引用。监听器、连接、UDP socket、
 * idle/lifecycle timer 与公开 server lease 各自持有引用，并且只在各自唯一
 * 的终态释放。退役代最后一个脚本活动结束时把 ServiceUnit 工作投递给
 * reaper；最后一个总引用结束时再投递最终析构。网络线程只完成终态记账，
 * 这里没有宽限时间或超时强制释放路径。
 */

#include "../sdk/xsbase.h"

typedef struct XS_ServerGeneration XS_ServerGeneration;
typedef struct XS_GenerationTimer XS_GenerationTimer;
typedef void (*XS_GenerationQuiesceProc)(XS_ServerGeneration* pGeneration);
typedef void (*XS_GenerationFinalizeProc)(XS_ServerGeneration* pGeneration);

typedef enum XS_GenerationReaperWork {
	XS_GENERATION_REAPER_NONE = 0,
	XS_GENERATION_REAPER_QUIESCE,
	XS_GENERATION_REAPER_FINALIZE
} XS_GenerationReaperWork;

struct XS_GenerationTimer {
	XS_GenerationTimer*	pNext;
	XS_ServerGeneration*	pGeneration;
	void*			pOwner;		/* 通常为 XS_ScriptRuntime* */
	uint64			iId;
};

struct XS_ServerGeneration {
	volatile int32		iReferences;	/* 管理引用 1 + 所有异步资源 */
	uint64			iCookie;	/* 进程内单调唯一；TLS 选择与 Accept 绑定 */
	xatomic32		tRetired;	/* 0 active；1 正在发布终态；2 已完整发布 */
	xatomic32		tConnections;	/* 仅用于状态/测试；安全性由引用决定 */
	xatomic32		tActivities;	/* 会进入脚本的连接/UDP 回调/lifecycle timer */
	xatomic32		tQuiesced;	/* 0 未排队，1 reaper 中，2 已通知 */
	xmutex*			pTimerLock;
	xmutex*			pPublishLock;
	xcond*			pPublishCond;
	xatomic32		tPublishState;	/* 0 preparing, 1 published, 2 discarded */
	XS_GenerationTimer*	pTimers;	/* lifecycle timer；退役时全部取消 */
	XS_ServerInfo*		pServer;
	void*			pDriverRuntime;
	void*			pConfigOwner;
	bool			bFreeServer;
	XS_GenerationQuiesceProc	procQuiesce;
	XS_GenerationFinalizeProc	procFinalize;
	XS_GenerationReaperWork	iReaperWork;
	XS_ServerGeneration*	pReaperNext;	/* quiesce/finalize 互斥排队 */
};

typedef struct XS_GenerationReaper {
	xmutex*		pLock;
	xcond*		pCond;
	xthread*	pThread;
	XS_ServerGeneration*	pHead;
	XS_ServerGeneration*	pTail;
	uint32		iActive;
	bool		bStopping;
} XS_GenerationReaper;

static XS_GenerationReaper g_XS_GenerationReaper;
static xatomic64 g_XS_ServerGenerationCookie;

static void XS_GenerationRelease(XS_ServerGeneration* pGeneration);

static void XS_GenerationReaperRun(
	XS_ServerGeneration* pGeneration,
	XS_GenerationReaperWork iWork)
{
	if ( iWork == XS_GENERATION_REAPER_QUIESCE ) {
		if ( pGeneration->procQuiesce != NULL ) {
			pGeneration->procQuiesce(pGeneration);
		}
		xrtAtomic32Store(&pGeneration->tQuiesced, 2, XMEMORY_RELEASE);
		/* quiesce work item 自身的保活引用；若这是最后一个引用，下一项
		 * 会在当前项已经出队后重新排入 finalizer。 */
		XS_GenerationRelease(pGeneration);
	} else if ( iWork == XS_GENERATION_REAPER_FINALIZE ) {
		pGeneration->procFinalize(pGeneration);
	}
}

static int32 XS_GenerationReaperProc(ptr pData)
{
	(void)pData;
	for ( ;; ) {
		XS_ServerGeneration* pGeneration;
		XS_GenerationReaperWork iWork;

		xrtMutexLock(g_XS_GenerationReaper.pLock);
		while ( !g_XS_GenerationReaper.bStopping &&
		        g_XS_GenerationReaper.pHead == NULL ) {
			if ( xrtCondWait(g_XS_GenerationReaper.pCond,
				g_XS_GenerationReaper.pLock) == XWAIT_ERROR ) {
				g_XS_GenerationReaper.bStopping = true;
				break;
			}
		}
		if ( g_XS_GenerationReaper.pHead == NULL &&
		     g_XS_GenerationReaper.bStopping ) {
			xrtMutexUnlock(g_XS_GenerationReaper.pLock);
			break;
		}
		pGeneration = g_XS_GenerationReaper.pHead;
		g_XS_GenerationReaper.pHead = pGeneration->pReaperNext;
		if ( g_XS_GenerationReaper.pHead == NULL ) {
			g_XS_GenerationReaper.pTail = NULL;
		}
		pGeneration->pReaperNext = NULL;
		iWork = pGeneration->iReaperWork;
		pGeneration->iReaperWork = XS_GENERATION_REAPER_NONE;
		g_XS_GenerationReaper.iActive++;
		xrtMutexUnlock(g_XS_GenerationReaper.pLock);

		XS_GenerationReaperRun(pGeneration, iWork);

		xrtMutexLock(g_XS_GenerationReaper.pLock);
		g_XS_GenerationReaper.iActive--;
		(void)xrtCondBroadcast(g_XS_GenerationReaper.pCond);
		xrtMutexUnlock(g_XS_GenerationReaper.pLock);
	}
	return 0;
}

static bool XS_GenerationReaperInit(void)
{
	memset(&g_XS_GenerationReaper, 0, sizeof(g_XS_GenerationReaper));
	xrtAtomic64Init(&g_XS_ServerGenerationCookie, 0);
	g_XS_GenerationReaper.pLock = xrtMutexCreate();
	g_XS_GenerationReaper.pCond = xrtCondCreate();
	if ( g_XS_GenerationReaper.pLock == NULL ||
	     g_XS_GenerationReaper.pCond == NULL ) goto failed;
	g_XS_GenerationReaper.pThread = xrtThreadCreate(
		XS_GenerationReaperProc, NULL, 0);
	if ( g_XS_GenerationReaper.pThread == NULL ) goto failed;
	return true;

failed:
	if ( g_XS_GenerationReaper.pCond != NULL )
		xrtCondDestroy(g_XS_GenerationReaper.pCond);
	if ( g_XS_GenerationReaper.pLock != NULL )
		xrtMutexDestroy(g_XS_GenerationReaper.pLock);
	memset(&g_XS_GenerationReaper, 0, sizeof(g_XS_GenerationReaper));
	return false;
}

static void XS_GenerationReaperEnqueue(
	XS_ServerGeneration* pGeneration,
	XS_GenerationReaperWork iWork)
{
	if ( pGeneration == NULL || iWork == XS_GENERATION_REAPER_NONE ) return;
	if ( g_XS_GenerationReaper.pLock == NULL ) {
		XS_GenerationReaperRun(pGeneration, iWork);
		return;
	}
	xrtMutexLock(g_XS_GenerationReaper.pLock);
	if ( g_XS_GenerationReaper.bStopping ) {
		xrtMutexUnlock(g_XS_GenerationReaper.pLock);
		XS_GenerationReaperRun(pGeneration, iWork);
		return;
	}
	pGeneration->iReaperWork = iWork;
	pGeneration->pReaperNext = NULL;
	if ( g_XS_GenerationReaper.pTail != NULL ) {
		g_XS_GenerationReaper.pTail->pReaperNext = pGeneration;
	} else {
		g_XS_GenerationReaper.pHead = pGeneration;
	}
	g_XS_GenerationReaper.pTail = pGeneration;
	(void)xrtCondSignal(g_XS_GenerationReaper.pCond);
	xrtMutexUnlock(g_XS_GenerationReaper.pLock);
}

static void XS_GenerationReaperDrain(void)
{
	if ( g_XS_GenerationReaper.pLock == NULL ) return;
	xrtMutexLock(g_XS_GenerationReaper.pLock);
	while ( g_XS_GenerationReaper.pHead != NULL ||
	        g_XS_GenerationReaper.iActive != 0 ) {
		if ( xrtCondWait(g_XS_GenerationReaper.pCond,
			g_XS_GenerationReaper.pLock) == XWAIT_ERROR ) break;
	}
	xrtMutexUnlock(g_XS_GenerationReaper.pLock);
}

static void XS_GenerationReaperUnit(void)
{
	if ( g_XS_GenerationReaper.pLock == NULL ) return;
	XS_GenerationReaperDrain();
	xrtMutexLock(g_XS_GenerationReaper.pLock);
	g_XS_GenerationReaper.bStopping = true;
	(void)xrtCondBroadcast(g_XS_GenerationReaper.pCond);
	xrtMutexUnlock(g_XS_GenerationReaper.pLock);
	if ( g_XS_GenerationReaper.pThread != NULL ) {
		(void)xrtThreadWait(g_XS_GenerationReaper.pThread);
		xrtThreadDestroy(g_XS_GenerationReaper.pThread);
	}
	xrtCondDestroy(g_XS_GenerationReaper.pCond);
	xrtMutexDestroy(g_XS_GenerationReaper.pLock);
	memset(&g_XS_GenerationReaper, 0, sizeof(g_XS_GenerationReaper));
}

static XS_ServerGeneration* XS_GenerationCreate(XS_ServerInfo* pServer)
{
	XS_ServerGeneration* pGeneration =
		(XS_ServerGeneration*)xrtCalloc(1, sizeof(XS_ServerGeneration));

	if ( pGeneration == NULL ) {
		return NULL;
	}
	pGeneration->pTimerLock = xrtMutexCreate();
	pGeneration->pPublishLock = xrtMutexCreate();
	pGeneration->pPublishCond = xrtCondCreate();
	if ( pGeneration->pTimerLock == NULL || pGeneration->pPublishLock == NULL ||
	     pGeneration->pPublishCond == NULL ) {
		if ( pGeneration->pPublishCond != NULL ) xrtCondDestroy(pGeneration->pPublishCond);
		if ( pGeneration->pPublishLock != NULL ) xrtMutexDestroy(pGeneration->pPublishLock);
		if ( pGeneration->pTimerLock != NULL ) xrtMutexDestroy(pGeneration->pTimerLock);
		xrtFree(pGeneration);
		return NULL;
	}
	pGeneration->iReferences = 1;
	pGeneration->iCookie = xrtAtomic64FetchAdd(
		&g_XS_ServerGenerationCookie, 1, XMEMORY_RELAXED) + 1u;
	/* 0 专门表示“没有选择 cookie”；仅理论上的 2^64 回绕需要跳过。 */
	if ( pGeneration->iCookie == 0 ) {
		pGeneration->iCookie = xrtAtomic64FetchAdd(
			&g_XS_ServerGenerationCookie, 1, XMEMORY_RELAXED) + 1u;
	}
	xrtAtomic32Init(&pGeneration->tRetired, 0);
	xrtAtomic32Init(&pGeneration->tConnections, 0);
	xrtAtomic32Init(&pGeneration->tActivities, 0);
	xrtAtomic32Init(&pGeneration->tQuiesced, 0);
	xrtAtomic32Init(&pGeneration->tPublishState, 0);
	pGeneration->pServer = pServer;
	pServer->Generation = pGeneration;
	return pGeneration;
}

static bool XS_GenerationIsPublished(const XS_ServerGeneration* pGeneration)
{
	return pGeneration != NULL &&
	       xrtAtomic32Load(&pGeneration->tPublishState, XMEMORY_ACQUIRE) == 1;
}

static bool XS_GenerationWaitPublished(XS_ServerGeneration* pGeneration)
{
	uint32 iState;

	if ( pGeneration == NULL || pGeneration->pPublishLock == NULL ) return false;
	xrtMutexLock(pGeneration->pPublishLock);
	while ( (iState = xrtAtomic32Load(&pGeneration->tPublishState,
		XMEMORY_ACQUIRE)) == 0 ) {
		if ( xrtCondWait(pGeneration->pPublishCond,
			pGeneration->pPublishLock) == XWAIT_ERROR ) break;
	}
	iState = xrtAtomic32Load(&pGeneration->tPublishState, XMEMORY_ACQUIRE);
	xrtMutexUnlock(pGeneration->pPublishLock);
	return iState == 1;
}

static bool XS_GenerationPublish(XS_ServerGeneration* pGeneration)
{
	uint32 iExpected = 0;
	bool bPublished;

	if ( pGeneration == NULL || pGeneration->pPublishLock == NULL ) return false;
	xrtMutexLock(pGeneration->pPublishLock);
	bPublished = xrtAtomic32CompareExchange(&pGeneration->tPublishState,
		&iExpected, 1, XMEMORY_ACQ_REL, XMEMORY_ACQUIRE);
	if ( bPublished ) (void)xrtCondBroadcast(pGeneration->pPublishCond);
	xrtMutexUnlock(pGeneration->pPublishLock);
	return bPublished || iExpected == 1;
}

static void XS_GenerationDiscard(XS_ServerGeneration* pGeneration)
{
	uint32 iExpected = 0;

	if ( pGeneration == NULL || pGeneration->pPublishLock == NULL ) return;
	xrtMutexLock(pGeneration->pPublishLock);
	if ( xrtAtomic32CompareExchange(&pGeneration->tPublishState,
		&iExpected, 2, XMEMORY_ACQ_REL, XMEMORY_ACQUIRE) ) {
		(void)xrtCondBroadcast(pGeneration->pPublishCond);
	}
	xrtMutexUnlock(pGeneration->pPublishLock);
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
	uint32 iExpected = 0;

	if ( pGeneration != NULL &&
	     xrtAtomic32Load(&pGeneration->tRetired, XMEMORY_ACQUIRE) == 2 &&
	     xrtAtomic32Load(&pGeneration->tActivities, XMEMORY_ACQUIRE) == 0 &&
	     pGeneration->procQuiesce != NULL &&
	     xrtAtomic32CompareExchange(&pGeneration->tQuiesced, &iExpected, 1,
		XMEMORY_ACQ_REL, XMEMORY_ACQUIRE) ) {
		/* 调用方仍持有 generation 引用，因此这个保活 retain 必定成功。
		 * ServiceUnit 只在 reaper 线程执行，不阻塞网络终态回调。 */
		if ( XS_GenerationRetain(pGeneration) ) {
			XS_GenerationReaperEnqueue(pGeneration, XS_GENERATION_REAPER_QUIESCE);
		} else {
			xrtAtomic32Store(&pGeneration->tQuiesced, 2, XMEMORY_RELEASE);
		}
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
		XS_GenerationReaperEnqueue(pGeneration, XS_GENERATION_REAPER_FINALIZE);
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
	if ( !XS_GenerationIsPublished(pGeneration) || !XS_GenerationRetain(pGeneration) ) {
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
	XS_GenerationDiscard(pGeneration);
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
