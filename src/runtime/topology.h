#ifndef XS_RUNTIME_TOPOLOGY_H
#define XS_RUNTIME_TOPOLOGY_H

/*
 * 进程级拓扑同步与公开配置 lease。
 *
 * 拓扑数组/Root 的读写由 RWLock 线性化；公开返回的 server 指针额外持有其
 * generation 引用，调用方必须 xsServerRelease。这样结构换代可以立即撤销
 * 拓扑所有权，但配置、脚本和 driver 要等最后一个连接/公开 lease 一并结束。
 */

#include <stdio.h>
#include <string.h>

#include "../core/config.h"
#include "generation.h"

static xrwlock* g_XS_TopologyLock = NULL;
static XS_App* g_XS_TopologyApp = NULL;
static xatomic32 g_XS_TopologyStopping;
static xatomic32 g_XS_TopologyLeases;

static bool XS_TopologyRuntimeInit(XS_App* pApp)
{
	if ( pApp == NULL || g_XS_TopologyLock != NULL ) return false;
	g_XS_TopologyLock = xrtRWLockCreate();
	if ( g_XS_TopologyLock == NULL ) return false;
	g_XS_TopologyApp = pApp;
	xrtAtomic32Init(&g_XS_TopologyStopping, 0);
	xrtAtomic32Init(&g_XS_TopologyLeases, 0);
	return true;
}

static bool XS_TopologyReadLock(void)
{
	return g_XS_TopologyLock != NULL && xrtRWLockRead(g_XS_TopologyLock);
}

static void XS_TopologyReadUnlock(void)
{
	if ( g_XS_TopologyLock != NULL ) (void)xrtRWLockReadUnlock(g_XS_TopologyLock);
}

static bool XS_TopologyWriteLock(void)
{
	return g_XS_TopologyLock != NULL && xrtRWLockWrite(g_XS_TopologyLock);
}

static void XS_TopologyWriteUnlock(void)
{
	if ( g_XS_TopologyLock != NULL ) (void)xrtRWLockWriteUnlock(g_XS_TopologyLock);
}

/* 仅允许在已经持有 topology 读锁或写锁时调用，返回借用指针。 */
static XS_ServerInfo* XS_TopologyFindLocked(XS_App* pApp, const char* sName)
{
	uint32 i;

	if ( pApp == NULL || sName == NULL ) return NULL;
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		XS_ServerInfo* pServer = pApp->Servers[i];

		if ( pServer != NULL && strcmp(pServer->Name, sName) == 0 ) return pServer;
	}
	return NULL;
}

static bool XS_TopologyLeaseRetain(XS_ServerInfo* pServer)
{
	XS_ServerGeneration* pGeneration;

	if ( pServer == NULL ||
	     xrtAtomic32Load(&g_XS_TopologyStopping, XMEMORY_ACQUIRE) != 0 ) {
		return false;
	}
	pGeneration = (XS_ServerGeneration*)pServer->Generation;
	if ( !XS_GenerationRetain(pGeneration) ) return false;
	xrtAtomic32FetchAdd(&g_XS_TopologyLeases, 1, XMEMORY_ACQ_REL);
	return true;
}

static XS_ServerInfo* XS_TopologyServerAcquire(const char* sName)
{
	XS_ServerInfo* pServer = NULL;

	if ( sName == NULL || !XS_TopologyReadLock() ) return NULL;
	if ( xrtAtomic32Load(&g_XS_TopologyStopping, XMEMORY_ACQUIRE) == 0 ) {
		pServer = XS_TopologyFindLocked(g_XS_TopologyApp, sName);
		if ( !XS_TopologyLeaseRetain(pServer) ) pServer = NULL;
	}
	XS_TopologyReadUnlock();
	return pServer;
}

/* pServer 必须已由连接回调、枚举回调或现有 lease 保证在本调用期间有效。 */
static XS_ServerInfo* XS_TopologyServerRetain(XS_ServerInfo* pServer)
{
	XS_ServerInfo* pRetained = NULL;

	/* Retain 也必须进入 topology 读侧临界区。StopAccepting 的写锁屏障由此
	 * 保证：停机开始后，不会出现 WaitLeases 已观察到 0、随后才登记的新 lease。 */
	if ( pServer == NULL || !XS_TopologyReadLock() ) return NULL;
	if ( XS_TopologyLeaseRetain(pServer) ) pRetained = pServer;
	XS_TopologyReadUnlock();
	return pRetained;
}

static void XS_TopologyServerRelease(XS_ServerInfo* pServer)
{
	XS_ServerGeneration* pGeneration;

	if ( pServer == NULL ) return;
	pGeneration = (XS_ServerGeneration*)pServer->Generation;
	if ( pGeneration == NULL ) return;
	/* 若这是最后一个引用，先完整执行 finalizer，再让停机观察到 lease=0。 */
	XS_GenerationRelease(pGeneration);
	xrtAtomic32FetchSub(&g_XS_TopologyLeases, 1, XMEMORY_ACQ_REL);
}

static bool XS_TopologyServerSnapshot(XS_ServerInfo*** pppServers, uint32* piCount)
{
	XS_ServerInfo** pServers = NULL;
	uint32 i;
	uint32 iHeld = 0;
	uint32 iCount;

	if ( pppServers == NULL || piCount == NULL ) return false;
	*pppServers = NULL;
	*piCount = 0;
	if ( !XS_TopologyReadLock() ) return false;
	if ( xrtAtomic32Load(&g_XS_TopologyStopping, XMEMORY_ACQUIRE) != 0 ) {
		XS_TopologyReadUnlock();
		return false;
	}
	iCount = g_XS_TopologyApp != NULL ? g_XS_TopologyApp->ServerCount : 0;
	pServers = (XS_ServerInfo**)xrtCalloc(iCount > 0 ? iCount : 1, sizeof(XS_ServerInfo*));
	if ( pServers != NULL ) {
		for ( i = 0; i < iCount; i++ ) {
			XS_ServerInfo* pServer = g_XS_TopologyApp->Servers[i];

			if ( !XS_TopologyLeaseRetain(pServer) ) break;
			pServers[i] = pServer;
			iHeld++;
		}
	}
	XS_TopologyReadUnlock();
	if ( pServers == NULL || iHeld != iCount ) {
		for ( i = 0; i < iHeld; i++ ) XS_TopologyServerRelease(pServers[i]);
		xrtFree(pServers);
		return false;
	}
	*pppServers = pServers;
	*piCount = iCount;
	return true;
}

/* 返回 retained xvalue；调用方使用 xrtValueRelease。 */
static xvalue* XS_TopologyRootAcquire(void)
{
	xvalue* pRoot = NULL;

	if ( !XS_TopologyReadLock() ) return NULL;
	if ( xrtAtomic32Load(&g_XS_TopologyStopping, XMEMORY_ACQUIRE) == 0 &&
	     g_XS_TopologyApp != NULL && g_XS_TopologyApp->Root != NULL ) {
		pRoot = xrtValueRetain(g_XS_TopologyApp->Root);
	}
	XS_TopologyReadUnlock();
	return pRoot;
}

/* 写锁内替换 Root；返回旧引用，调用方必须在解锁后 Release。 */
static xvalue* XS_TopologyRootReplaceLocked(XS_App* pApp, const xvalue* pNewRoot)
{
	xvalue* pOld;

	if ( pApp == NULL || pNewRoot == NULL ) return NULL;
	pOld = pApp->Root;
	pApp->Root = xrtValueRetain(pNewRoot);
	return pOld;
}

static bool XS_TopologyRootReplace(XS_App* pApp, const xvalue* pNewRoot)
{
	xvalue* pOld;

	if ( pApp == NULL || pNewRoot == NULL || !XS_TopologyWriteLock() ) return false;
	pOld = XS_TopologyRootReplaceLocked(pApp, pNewRoot);
	XS_TopologyWriteUnlock();
	xrtValueRelease(pOld);
	return true;
}

/* 停机先封住新 lease；写锁屏障保证并发查找已经完成 retain 或已经失败。 */
static void XS_TopologyStopAccepting(void)
{
	if ( g_XS_TopologyLock == NULL ) return;
	xrtAtomic32Store(&g_XS_TopologyStopping, 1, XMEMORY_RELEASE);
	if ( XS_TopologyWriteLock() ) XS_TopologyWriteUnlock();
}

static void XS_TopologyWaitLeases(void)
{
	uint32 iSeconds = 0;

	while ( xrtAtomic32Load(&g_XS_TopologyLeases, XMEMORY_ACQUIRE) != 0 ) {
		xrtSleep(100);
		if ( ++iSeconds % 50 == 0 ) {
			printf("[xs] waiting topology leases: %u\n",
				xrtAtomic32Load(&g_XS_TopologyLeases, XMEMORY_ACQUIRE));
		}
	}
}

static void XS_TopologyRuntimeUnit(void)
{
	if ( g_XS_TopologyLock == NULL ) return;
	XS_TopologyStopAccepting();
	XS_TopologyWaitLeases();
	xrtRWLockDestroy(g_XS_TopologyLock);
	g_XS_TopologyLock = NULL;
	g_XS_TopologyApp = NULL;
}

#endif
