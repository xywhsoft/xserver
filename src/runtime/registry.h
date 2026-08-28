#ifndef XS_RUNTIME_REGISTRY_H
#define XS_RUNTIME_REGISTRY_H

/*
 * xs3 连接注册表（设计 §10.3）
 * 职责：tcp/tcps 连接追踪 —— idle 超时扫描、停机时批量收口、
 * 以及后续热重载的按代 drain 判据（当前停机用 LiveObjects 轮询兜底）。
 * 回调在各 worker 线程并发执行，全部操作持锁。
 */

#include <stdio.h>

#include "../sdk/xsbase.h"

typedef struct XS_ConnRecord {
	struct XS_ConnRecord*	pNext;
	XS_HostInfo*		pHost;
	xnetstream*		pTcp;		/* 与 pTls 二选一 */
	xtlsstream*		pTls;
	int64			tLastActive;	/* xrtNow() 微秒 */
} XS_ConnRecord;

typedef struct XS_ConnRegistry {
	xmutex*			pLock;
	XS_ConnRecord*		pHead;
	uint32			iCount;
} XS_ConnRegistry;

static bool XS_RegistryInit(XS_ConnRegistry* pReg)
{
	memset(pReg, 0, sizeof(*pReg));
	pReg->pLock = xrtMutexCreate();
	return pReg->pLock != NULL;
}

static void XS_RegistryAdd(XS_ConnRegistry* pReg, XS_ConnRecord* pRecord)
{
	xrtMutexLock(pReg->pLock);
	pRecord->pNext = pReg->pHead;
	pReg->pHead = pRecord;
	pReg->iCount++;
	xrtMutexUnlock(pReg->pLock);
}

static void XS_RegistryRemove(XS_ConnRegistry* pReg, XS_ConnRecord* pRecord)
{
	XS_ConnRecord** ppLink;

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
	/* lastActive 仅作 idle 判定，竞态下读到旧值只影响一拍扫描，无需加锁 */
	pRecord->tLastActive = xrtNow();
}

/* 关闭全部连接（graceful）。Close 事件触发后由各自 Close shim 出表。 */
static void XS_RegistryCloseAll(XS_ConnRegistry* pReg)
{
	XS_ConnRecord* pRecord;

	xrtMutexLock(pReg->pLock);
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
	XS_ConnRecord* arrStale[256];
	uint32 iStale = 0;
	uint32 i;
	int64 tNow = xrtNow();
	int64 iIdleUs = (int64)iIdleMs * 1000;

	xrtMutexLock(pReg->pLock);
	for ( pRecord = pReg->pHead; pRecord != NULL && iStale < 256; pRecord = pRecord->pNext ) {
		if ( tNow - pRecord->tLastActive > iIdleUs ) {
			arrStale[iStale++] = pRecord;
		}
	}
	xrtMutexUnlock(pReg->pLock);
	for ( i = 0; i < iStale; i++ ) {
		if ( arrStale[i]->pTcp != NULL ) {
			xrtNetStreamClose(arrStale[i]->pTcp);
		} else if ( arrStale[i]->pTls != NULL ) {
			xrtTlsStreamClose(arrStale[i]->pTls);
		}
	}
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
