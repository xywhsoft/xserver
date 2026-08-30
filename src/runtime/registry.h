#ifndef XS_RUNTIME_REGISTRY_H
#define XS_RUNTIME_REGISTRY_H

/*
 * xs3 连接注册表（设计 §10.3）
 * 职责：tcp/tcps 连接追踪 —— idle 超时扫描、停机时批量收口与观测。
 * 回调在各 worker 线程并发执行，全部操作持锁；代际安全与最终回收由
 * generation lease 的唯一终态释放保证，不以 iCount 或轮询作为释放判据。
 */

#include <stdio.h>

#include "../sdk/xsbase.h"
#include "generation.h"

typedef struct XS_ScriptRuntime XS_ScriptRuntime;

typedef struct XS_ConnRecord {
	struct XS_ConnRecord*	pNext;
	volatile int32		iReferences;	/* 连接所有权 1 + 正在执行的协议回调 */
	XS_HostInfo*		pHost;
	struct XS_ConnRegistry*	pRegistry;
	XS_ServerGeneration*	pGeneration;
	XS_ScriptRuntime*	pScript;
	xnetstream*		pTcp;		/* 与 pTls 二选一 */
	xtlsstream*		pTls;
	xatomic64		tLastActive;	/* xrtNow() 微秒，跨 worker 原子快照 */
} XS_ConnRecord;

typedef struct XS_ConnRegistry {
	xmutex*			pLock;
	XS_ConnRecord*		pHead;
	uint32			iCount;
	bool			bClosing;
} XS_ConnRegistry;

static bool XS_RegistryInit(XS_ConnRegistry* pReg)
{
	memset(pReg, 0, sizeof(*pReg));
	pReg->pLock = xrtMutexCreate();
	return pReg->pLock != NULL;
}

static bool XS_RegistryAdd(XS_ConnRegistry* pReg, XS_ConnRecord* pRecord)
{
	bool bAdded = false;

	if ( pReg == NULL || pReg->pLock == NULL || pRecord == NULL ) return false;
	xrtMutexLock(pReg->pLock);
	if ( !pReg->bClosing ) {
		xrtAtomic64Init(&pRecord->tLastActive, (uint64)xrtNow());
		pRecord->pRegistry = pReg;
		pRecord->pNext = pReg->pHead;
		pReg->pHead = pRecord;
		pReg->iCount++;
		bAdded = true;
	}
	xrtMutexUnlock(pReg->pLock);
	return bAdded;
}

static void XS_RegistryRemove(XS_ConnRegistry* pReg, XS_ConnRecord* pRecord)
{
	XS_ConnRecord** ppLink;

	if ( pReg == NULL || pReg->pLock == NULL || pRecord == NULL ) return;
	xrtMutexLock(pReg->pLock);
	for ( ppLink = &pReg->pHead; *ppLink != NULL; ppLink = &(*ppLink)->pNext ) {
		if ( *ppLink == pRecord ) {
			*ppLink = pRecord->pNext;
			pReg->iCount--;
			break;
		}
	}
	xrtMutexUnlock(pReg->pLock);
}

static void XS_RegistryTouch(XS_ConnRecord* pRecord)
{
	xrtAtomic64Store(&pRecord->tLastActive, (uint64)xrtNow(), XMEMORY_RELAXED);
}

/* 关闭全部连接（graceful）。Close 事件触发后由各自 Close shim 出表。 */
static void XS_RegistryStopAccepting(XS_ConnRegistry* pReg)
{
	if ( pReg == NULL || pReg->pLock == NULL ) return;
	xrtMutexLock(pReg->pLock);
	pReg->bClosing = true;
	xrtMutexUnlock(pReg->pLock);
}

static void XS_RegistryCloseAll(XS_ConnRegistry* pReg)
{
	XS_ConnRecord* pRecord;

	if ( pReg == NULL || pReg->pLock == NULL ) return;
	xrtMutexLock(pReg->pLock);
	pReg->bClosing = true;
	for ( pRecord = pReg->pHead; pRecord != NULL; pRecord = pRecord->pNext ) {
		if ( pRecord->pTcp != NULL ) {
			xrtNetStreamClose(pRecord->pTcp);
		} else if ( pRecord->pTls != NULL ) {
			xrtTlsStreamClose(pRecord->pTls);
		}
	}
	xrtMutexUnlock(pReg->pLock);
}

/* idle 扫描：关闭超过 iIdleMs 无活动的连接，返回关闭数 */
static uint32 XS_RegistrySweepIdle(XS_ConnRegistry* pReg, uint64 iIdleMs)
{
	XS_ConnRecord* pRecord;
	uint32 iStale = 0;
	int64 tNow = xrtNow();

	if ( pReg == NULL || pReg->pLock == NULL || iIdleMs == 0 ) return 0;
	xrtMutexLock(pReg->pLock);
	for ( pRecord = pReg->pHead; pRecord != NULL; pRecord = pRecord->pNext ) {
		int64 tLast = (int64)xrtAtomic64Load(&pRecord->tLastActive, XMEMORY_RELAXED);
		uint64 iElapsedMs = tNow > tLast ? (uint64)(tNow - tLast) / 1000u : 0;

		if ( iElapsedMs > iIdleMs ) {
			iStale++;
			/* Close 只投递终态；记录仍由 Close 回调从表中摘除。锁内发起可
			 * 避免先做裸指针快照再解锁造成 UAF。 */
			if ( pRecord->pTcp != NULL ) {
				xrtNetStreamClose(pRecord->pTcp);
			} else if ( pRecord->pTls != NULL ) {
				xrtTlsStreamClose(pRecord->pTls);
			}
		}
	}
	xrtMutexUnlock(pReg->pLock);
	return iStale;
}

static void XS_RegistryUnit(XS_ConnRegistry* pReg)
{
	if ( pReg->pLock != NULL ) {
		xrtMutexLock(pReg->pLock);
		xrtMutexUnlock(pReg->pLock);
		xrtMutexDestroy(pReg->pLock);
		pReg->pLock = NULL;
	}
	pReg->pHead = NULL;
	pReg->iCount = 0;
}

#endif
