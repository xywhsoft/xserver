#ifndef XS_RUNTIME_TLS_REFRESH_H
#define XS_RUNTIME_TLS_REFRESH_H

/*
 * TLS 证书热替换：不重启进程、不触发脚本重载、不断连。
 *
 * 原理：XS_ListenerSlot.pTlsContext 是 TLS 握手回调在锁内读取的指针。
 * 刷新 = 构建新 XS_TlsTable（重读磁盘上的证书文件）→ 锁内换指针。
 * 正在握手的连接已完成身份选择，不受影响；下一个握手用新证书。
 *
 * 旧表回收：保留上一代（宽限），下下次刷新时释放更早的。
 * 原 runtime 内嵌表随 runtime 销毁；刷新堆表由本模块跟踪。
 */

#include "../core/tls.h"
#include "../core/driver.h"
#include "topology.h"
#include "listener_slot.h"

/* ------------------------------------------------------------------ */
/* 刷新表节点与链回收（须 core/tls.h 可见——XS_TlsTableUnit 在此处调用） */
/* ------------------------------------------------------------------ */

typedef struct XS_TlsRefreshNode {
	XS_TlsTable			tTable;		/* 堆分配的表本体 */
	struct XS_TlsRefreshNode*	pPrev;		/* 上一代（宽限一代） */
} XS_TlsRefreshNode;

static void XS_TlsRefreshChainUnit(void* pNodeVoid)
{
	XS_TlsRefreshNode* pNode = (XS_TlsRefreshNode*)pNodeVoid;
	while ( pNode != NULL ) {
		XS_TlsRefreshNode* pNext = pNode->pPrev;
		XS_TlsTableUnit(&pNode->tTable);
		xrtFree(pNode);
		pNode = pNext;
	}
}

/* 保留当前代，释放更早的 */
static void XS_TlsRefreshChainGC(XS_TlsRefreshNode* pCurrent)
{
	if ( pCurrent == NULL || pCurrent->pPrev == NULL ) return;
	XS_TlsRefreshChainUnit(pCurrent->pPrev);
	pCurrent->pPrev = NULL;
}

/* ------------------------------------------------------------------ */
/* 核心：构建新表 + 原子换指针                                          */
/* ------------------------------------------------------------------ */

static bool XS_TlsSlotRefresh(
	XS_ListenerSlot*		pSlot,
	XS_ServerInfo*			pServer,
	char*				sErr,
	size_t				iErrCap)
{
	XS_TlsRefreshNode* pNode;
	XS_TlsRefreshNode* pOld;

	if ( pSlot == NULL || pServer == NULL ) {
		snprintf(sErr, iErrCap, "tls refresh: null slot or server");
		return false;
	}
	if ( !pServer->TLS ) {
		snprintf(sErr, iErrCap, "tls refresh: server '%s' has no TLS endpoint", pServer->Name);
		return false;
	}

	/* 1. 构建新表（堆分配，重读磁盘证书） */
	pNode = (XS_TlsRefreshNode*)xrtCalloc(1, sizeof(XS_TlsRefreshNode));
	if ( pNode == NULL ) {
		snprintf(sErr, iErrCap, "tls refresh: out of memory");
		return false;
	}
	if ( !XS_TlsTableBuild(pServer, &pNode->tTable, sErr, iErrCap) ) {
		xrtFree(pNode);
		return false;
	}
	if ( pNode->tTable.iCount == 0 ) {
		XS_TlsTableUnit(&pNode->tTable);
		xrtFree(pNode);
		snprintf(sErr, iErrCap, "tls refresh: no usable identity for server '%s'", pServer->Name);
		return false;
	}

	/* 2. 拓扑读锁（保证 slot 在本操作期间有效） */
	if ( !XS_TopologyReadLock() ) {
		XS_TlsTableUnit(&pNode->tTable);
		xrtFree(pNode);
		snprintf(sErr, iErrCap, "tls refresh: topology lock failed");
		return false;
	}

	/* 3. slot 锁内原子换指针 */
	xrtMutexLock(pSlot->pLock);
	if ( pSlot->bClosing || !pSlot->bAccepting ) {
		xrtMutexUnlock(pSlot->pLock);
		XS_TopologyReadUnlock();
		XS_TlsTableUnit(&pNode->tTable);
		xrtFree(pNode);
		snprintf(sErr, iErrCap, "tls refresh: slot not accepting");
		return false;
	}
	pOld = (XS_TlsRefreshNode*)pSlot->pTlsRefreshNode;
	pNode->pPrev = pOld;
	pSlot->pTlsContext = (void*)&pNode->tTable;
	pSlot->pTlsRefreshNode = (void*)pNode;
	xrtMutexUnlock(pSlot->pLock);

	XS_TopologyReadUnlock();
	return true;
}

/* ------------------------------------------------------------------ */
/* 公开 API：按服务器名刷新 TLS 证书                                    */
/* ------------------------------------------------------------------ */

XRT_API bool xsTlsRefresh(const char* sServerName, char* sErr, size_t iErrCap)
{
	XS_ServerInfo* pServer = NULL;
	XS_ListenerSlot* pSlot = NULL;
	void* pExpectedRuntime = NULL;
	XS_TlsRefreshNode* pCurrent = NULL;
	void* pOldChain = NULL;
	bool bOK;

	if ( sErr && iErrCap > 0 ) sErr[0] = '\0';
	if ( sServerName == NULL || sServerName[0] == '\0' ) {
		if ( sErr ) snprintf(sErr, iErrCap, "tls refresh: server name required");
		return false;
	}

	/* 1. 按名查找服务器（拓扑 lease 保证存活） */
	pServer = XS_TopologyServerAcquire(sServerName);
	if ( pServer == NULL ) {
		if ( sErr ) snprintf(sErr, iErrCap, "tls refresh: server '%s' not found", sServerName);
		return false;
	}

	/* 2. 获取 listener slot */
	pSlot = XS_ServerDriverListenerSlot(pServer, &pExpectedRuntime);
	if ( pSlot == NULL ) {
		XS_TopologyServerRelease(pServer);
		if ( sErr ) snprintf(sErr, iErrCap, "tls refresh: no listener slot for '%s'", sServerName);
		return false;
	}

	/* 3. 执行刷新 */
	bOK = XS_TlsSlotRefresh(pSlot, pServer, sErr, iErrCap);

	/* 4. 回收旧代（保留当前代和上一代） */
	if ( bOK ) {
		xrtMutexLock(pSlot->pLock);
		pCurrent = (XS_TlsRefreshNode*)pSlot->pTlsRefreshNode;
		xrtMutexUnlock(pSlot->pLock);
		if ( pCurrent ) XS_TlsRefreshChainGC(pCurrent);
	}

	XS_TopologyServerRelease(pServer);
	return bOK;
}

#endif /* XS_RUNTIME_TLS_REFRESH_H */
