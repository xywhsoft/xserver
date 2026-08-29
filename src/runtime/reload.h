#ifndef XS_RUNTIME_RELOAD_H
#define XS_RUNTIME_RELOAD_H

/*
 * 热重载：候选代先成功，新旧代再切换；旧代只由终态引用归零回收。
 * 所有入口都投递到固定 worker，避免从 TCC 回调栈同步退役自身。
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"
#include "../core/config.h"
#include "../core/engine.h"
#include "../script/script.h"
#include "../protocol/http.h"
#include "../protocol/stream.h"
#include "../protocol/ws.h"
#include "../protocol/udp.h"
#include "../core/driver.h"
#include "gc.h"

static volatile bool g_XS_ReloadBusy = false;
static xmutex* g_XS_ReloadLock = NULL;
static xatomic32 g_XS_ReloadStopping;

static bool XS_ReloadRuntimeInit(void)
{
	if ( g_XS_ReloadLock != NULL ) return true;
	g_XS_ReloadLock = xrtMutexCreate();
	if ( g_XS_ReloadLock == NULL ) return false;
	xrtAtomic32Init(&g_XS_ReloadStopping, 0);
	return true;
}

/* 主线程停机栅栏：阻止新任务进入，并等待 worker0 当前换槽操作返回。 */
static void XS_ReloadQuiesce(void)
{
	if ( g_XS_ReloadLock == NULL ) return;
	xrtAtomic32Store(&g_XS_ReloadStopping, 1, XMEMORY_RELEASE);
	xrtMutexLock(g_XS_ReloadLock);
	xrtMutexUnlock(g_XS_ReloadLock);
}

static void XS_ReloadRuntimeUnit(void)
{
	if ( g_XS_ReloadLock != NULL ) {
		xrtMutexDestroy(g_XS_ReloadLock);
		g_XS_ReloadLock = NULL;
	}
}

static bool XS_StrEq(const char* a, const char* b)
{
	if ( a == NULL || b == NULL ) {
		return a == b;
	}
	return strcmp(a, b) == 0;
}

static bool XS_ServerStructEquals(XS_ServerInfo* pA, XS_ServerInfo* pB)
{
	uint32 i;
	XS_HostInfo* pDA = pA->DefaultHost;
	XS_HostInfo* pDB = pB->DefaultHost;

	if ( strcmp(pA->Class, pB->Class) != 0 || pA->Enabled != pB->Enabled ||
	     !XS_StrEq(pA->IP, pB->IP) || !XS_StrEq(pA->IPTLS, pB->IPTLS) ||
	     pA->Port != pB->Port || pA->TLS != pB->TLS || pA->PortTLS != pB->PortTLS ||
	     pA->Backlog != pB->Backlog || pA->RecvLimit != pB->RecvLimit ||
	     pA->HostCount != pB->HostCount ||
	     !XS_StrEq(pDA->Name, pDB->Name) || !XS_StrEq(pDA->Host, pDB->Host) ||
	     !XS_StrEq(pDA->Path, pDB->Path) || !XS_StrEq(pDA->DevLang, pDB->DevLang) ||
	     !XS_StrEq(pDA->DevFile, pDB->DevFile) || !XS_StrEq(pDA->DevInc, pDB->DevInc) ||
	     !XS_StrEq(pDA->DevLib, pDB->DevLib) || !XS_StrEq(pDA->TlsCA, pDB->TlsCA) ||
	     !XS_StrEq(pDA->TlsCert, pDB->TlsCert) || !XS_StrEq(pDA->TlsKey, pDB->TlsKey) ) {
		return false;
	}
	for ( i = 0; i < pA->HostCount; i++ ) {
		XS_HostInfo* pHA = pA->Hosts[i];
		XS_HostInfo* pHB = pB->Hosts[i];

		if ( !XS_StrEq(pHA->Name, pHB->Name) || !XS_StrEq(pHA->Host, pHB->Host) ||
		     !XS_StrEq(pHA->Path, pHB->Path) || !XS_StrEq(pHA->DevFile, pHB->DevFile) ||
		     !XS_StrEq(pHA->TlsCA, pHB->TlsCA) || !XS_StrEq(pHA->TlsCert, pHB->TlsCert) ||
		     !XS_StrEq(pHA->TlsKey, pHB->TlsKey) ) {
			return false;
		}
	}
	return true;
}

/* 两代不能同时绑定同一传输端点时，不做“先拆旧再赌回滚”。 */
static bool XS_ServerBindConflicts(XS_ServerInfo* pOld, XS_ServerInfo* pNew)
{
	bool bOldUdp = strcmp(pOld->Class, "udp") == 0;
	bool bNewUdp = strcmp(pNew->Class, "udp") == 0;

	return bOldUdp == bNewUdp && pOld->Port == pNew->Port &&
	       XS_StrEq(pOld->IP, pNew->IP);
}

static bool XS_ReloadClassSupported(XS_ServerInfo* pServer, const char* sScope)
{
	if ( pServer == NULL || pServer->Class == NULL ) {
		return false;
	}
	if ( strcmp(pServer->Class, "custom") == 0 ) {
		printf("[xs] %s reload rejected for custom server '%s': "
			"raw application resources have no terminal lease\n",
			sScope, pServer->Name);
		return false;
	}
	return true;
}

static bool XS_ReloadHostInner(XS_HostInfo* pHost)
{
	XS_ScriptRuntime* pOld;
	XS_ScriptRuntime* pNew;
	xvalue* pShared = NULL;
	uint64 tOldGen;

	if ( pHost == NULL || pHost->Server == NULL ||
	     !XS_ReloadClassSupported(pHost->Server, "host") ) {
		return false;
	}
	/* UDP socket 捕获脚本代直到 socket Close；原地换脚本不会改变接收者。 */
	if ( strcmp(pHost->Server->Class, "udp") == 0 ) {
		printf("[xs] host reload rejected for udp server '%s': use structural server reload\n",
			pHost->Server->Name);
		return false;
	}
	pOld = XS_ScriptAcquireHost(pHost);
	if ( pOld == NULL ) {
		return false;
	}
	pNew = XS_ScriptCompile(pHost);
	if ( pNew == NULL ) {
		printf("[xs] reload '%s' compile failed, old generation keeps serving\n",
			pHost->Name != NULL ? pHost->Name : "?");
		pHost->State = XS_RUN_RELOAD_FAILED;
		pHost->Server->State = XS_RUN_RELOAD_FAILED;
		XS_ScriptRelease(pOld);
		return false;
	}
	if ( pOld->procSwap != NULL ) {
		xvalue* pOut = NULL;
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pOld);

		if ( pOld->procSwap(pHost, &pOut) && pOut != NULL ) {
			pShared = pOut;
		}
		XS_ScriptLeave(pPrevious);
	}
	pNew->pSwap = pShared;
	tOldGen = pOld->tGeneration;
	XS_ScriptAttach(pHost, pNew);	/* 原子切槽；内部撤销旧代 owner 引用 */
	pHost->State = XS_RUN_RUNNING;
	pHost->Server->State = XS_RUN_RUNNING;
	printf("[xs] reload '%s' ok (gen %llu -> %llu)\n",
		pHost->Name != NULL ? pHost->Name : "?",
		(unsigned long long)tOldGen, (unsigned long long)pNew->tGeneration);
	XS_ScriptRelease(pOld);		/* 撤销本函数临时引用 */
	return true;
}

static bool XS_ReloadHostNow(XS_HostInfo* pHost)
{
	bool bOk;

	if ( g_XS_ReloadBusy ) {
		printf("[xs] reload busy\n");
		return false;
	}
	g_XS_ReloadBusy = true;
	bOk = XS_ReloadHostInner(pHost);
	g_XS_ReloadBusy = false;
	return bOk;
}

/* 延迟请求保存名字而非裸 host 指针，结构换代后执行也不会追到已退役对象。 */
typedef struct XS_DeferredReload {
	XS_App*		pApp;
	char*		sServerName;
	char*		sHostName;
	bool		bHost;
	bool		bAll;
} XS_DeferredReload;

static bool XS_ReloadServerNow(XS_App* pApp, const char* sName);
static bool XS_ReloadAllNow(XS_App* pApp);

static void XS_DeferredFire(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_DeferredReload* pReq = (XS_DeferredReload*)pData;

	(void)pWorker; (void)iId;
	if ( iResult == XNET_RESULT_OK && g_XS_ReloadLock != NULL &&
	     xrtAtomic32Load(&g_XS_ReloadStopping, XMEMORY_ACQUIRE) == 0 ) {
		xrtMutexLock(g_XS_ReloadLock);
		if ( xrtAtomic32Load(&g_XS_ReloadStopping, XMEMORY_ACQUIRE) != 0 ) {
			xrtMutexUnlock(g_XS_ReloadLock);
			goto FreeRequest;
		}
		if ( pReq->bHost ) {
			XS_ServerInfo* pServer = xsServerFind(pReq->sServerName);
			XS_HostInfo* pHost = pServer != NULL ? xsHostFind(pServer, pReq->sHostName) : NULL;

			(void)XS_ReloadHostNow(pHost);
		} else if ( pReq->sServerName != NULL ) {
			(void)XS_ReloadServerNow(pReq->pApp, pReq->sServerName);
		} else if ( pReq->bAll ) {
			(void)XS_ReloadAllNow(pReq->pApp);
		}
		xrtMutexUnlock(g_XS_ReloadLock);
	}
FreeRequest:
	xrtFree(pReq->sServerName);
	xrtFree(pReq->sHostName);
	xrtFree(pReq);
}

static bool XS_DeferReload(XS_App* pApp, XS_HostInfo* pHost, const char* sName, bool bAll)
{
	XS_DeferredReload* pReq;
	uint64 iTimer;

	if ( pApp == NULL || pApp->Engine == NULL || g_XS_ReloadLock == NULL ||
	     xrtAtomic32Load(&g_XS_ReloadStopping, XMEMORY_ACQUIRE) != 0 ) {
		return false;
	}
	pReq = (XS_DeferredReload*)xrtCalloc(1, sizeof(XS_DeferredReload));
	if ( pReq == NULL ) {
		return false;
	}
	pReq->pApp = pApp;
	pReq->bAll = bAll;
	if ( pHost != NULL && pHost->Server != NULL ) {
		pReq->bHost = true;
		pReq->sServerName = xrtStrDup(pHost->Server->Name);
		pReq->sHostName = xrtStrDup(pHost->Name);
	} else if ( sName != NULL ) {
		pReq->sServerName = xrtStrDup(sName);
	}
	if ( (pHost != NULL && (pReq->sServerName == NULL || pReq->sHostName == NULL)) ||
	     (sName != NULL && pReq->sServerName == NULL) ) {
		xrtFree(pReq->sServerName);
		xrtFree(pReq->sHostName);
		xrtFree(pReq);
		return false;
	}
	iTimer = xrtNetEngineAfter(pApp->Engine, 0, 1000, XS_DeferredFire, pReq);
	if ( iTimer == 0 ) {
		xrtFree(pReq->sServerName);
		xrtFree(pReq->sHostName);
		xrtFree(pReq);
		return false;
	}
	return true;
}

static XS_App* XS_ReloadTakeSnapshot(XS_App* pFresh)
{
	XS_App* pOwner = (XS_App*)xrtMalloc(sizeof(XS_App));

	if ( pOwner == NULL ) {
		return NULL;
	}
	*pOwner = *pFresh;
	memset(pFresh, 0, sizeof(*pFresh));
	return pOwner;
}

static bool XS_ReloadPrepareServer(
	XS_ServerInfo* pServer,
	xvalue** ppShared,
	char* sErr,
	size_t iErrCap)
{
	if ( XS_GenerationCreate(pServer) == NULL ) {
		snprintf(sErr, iErrCap, "generation create failed");
		return false;
	}
	if ( XS_HostHasScript(pServer->DefaultHost) ) {
		XS_ScriptRuntime* pRuntime = XS_ScriptCompile(pServer->DefaultHost);

		if ( pRuntime == NULL ) {
			snprintf(sErr, iErrCap, "script compile failed");
			return false;
		}
		pRuntime->pSwap = *ppShared;
		*ppShared = NULL;
		XS_ScriptAttach(pServer->DefaultHost, pRuntime);
	} else if ( XS_ClassNeedsScript(pServer->Class) ) {
		snprintf(sErr, iErrCap, "class '%s' requires devfile", pServer->Class);
		return false;
	} else if ( *ppShared != NULL ) {
		xrtValueRelease(*ppShared);
		*ppShared = NULL;
	}
	if ( !XS_ServerDriverStart(pServer, sErr, iErrCap) ) {
		return false;
	}
	pServer->State = XS_RUN_RUNNING;
	pServer->DefaultHost->State = XS_RUN_RUNNING;
	return true;
}

static void XS_ReloadDiscardCandidate(XS_ServerInfo* pServer)
{
	XS_App* pOwner;

	if ( pServer == NULL ) return;
	if ( pServer->Generation == NULL ) {
		pOwner = (XS_App*)pServer->ConfigOwner;
		if ( pOwner != NULL ) {
			XS_ConfigFree(pOwner);
			xrtFree(pOwner);
		}
		return;
	}
	if ( pServer->State == XS_RUN_RUNNING ) {
		XS_ServerDriverStop(pServer);
		XS_ServerDriverCloseConnections(pServer);
	}
	(void)XS_GcRetireServer(pServer, false);
}

static bool XS_ReloadSameServer(XS_ServerInfo* pOld)
{
	bool bOk = true;
	uint32 i;
	char sErr[256];

	if ( !XS_ReloadClassSupported(pOld, "server") ) return false;
	if ( strcmp(pOld->Class, "udp") == 0 ) {
		printf("[xs] same-endpoint udp reload rejected for '%s'; change endpoint or restart\n",
			pOld->Name);
		return false;
	}
	sErr[0] = '\0';
	if ( pOld->TLS && pOld->Runtime != NULL ) {
		if ( strcmp(pOld->Class, "tcp") == 0 ) {
			(void)XS_TlsTableRefresh(pOld, &((XS_TcpRuntime*)pOld->Runtime)->tTls, sErr, sizeof(sErr));
		} else if ( strcmp(pOld->Class, "http") == 0 ) {
			(void)XS_TlsTableRefresh(pOld, &((XS_HttpRuntime*)pOld->Runtime)->tTls, sErr, sizeof(sErr));
		} else if ( strcmp(pOld->Class, "ws") == 0 ) {
			(void)XS_TlsTableRefresh(pOld, &((XS_WsRuntime*)pOld->Runtime)->tTls, sErr, sizeof(sErr));
		}
		if ( sErr[0] != '\0' ) printf("[xs] %s\n", sErr);
	}
	if ( pOld->DefaultHost->Runtime != NULL ) {
		bOk = XS_ReloadHostInner(pOld->DefaultHost) && bOk;
	}
	for ( i = 0; i < pOld->HostCount; i++ ) {
		if ( pOld->Hosts[i]->Runtime != NULL ) {
			bOk = XS_ReloadHostInner(pOld->Hosts[i]) && bOk;
		}
	}
	return bOk;
}

static bool XS_ReloadServerNow(XS_App* pApp, const char* sName)
{
	XS_ServerInfo* pOld;
	XS_ServerInfo* pNew;
	XS_App tFresh;
	XS_App* pOwner;
	XS_ScriptRuntime* pOldScript;
	xvalue* pShared = NULL;
	char sCfgPath[4096];
	char sErr[256];
	uint32 i;
	bool bOk = false;

	if ( g_XS_ReloadBusy ) {
		printf("[xs] reload busy\n");
		return false;
	}
	pOld = xsServerFind(sName);
	if ( pOld == NULL || !XS_ReloadClassSupported(pOld, "server") ) return false;
	g_XS_ReloadBusy = true;
	snprintf(sCfgPath, sizeof(sCfgPath), "%.200s/xs.json", XS_AppPath());
	if ( !XS_ConfigLoad(sCfgPath, &tFresh) ) {
		printf("[xs] server reload: config re-parse failed: %s\n", tFresh.ParseError);
		XS_ConfigFree(&tFresh);
		pOld->State = XS_RUN_RELOAD_FAILED;
		goto Done;
	}
	for ( i = 0; i < tFresh.ServerCount; i++ ) {
		if ( strcmp(tFresh.Servers[i]->Name, sName) == 0 ) break;
	}
	if ( i >= tFresh.ServerCount ) {
		printf("[xs] server reload: '%s' no longer exists (use reload-all to remove)\n", sName);
		XS_ConfigFree(&tFresh);
		goto Done;
	}
	pNew = tFresh.Servers[i];
	if ( XS_ServerStructEquals(pOld, pNew) ) {
		bOk = XS_ReloadSameServer(pOld);
		XS_ConfigFree(&tFresh);
		goto Done;
	}
	if ( strcmp(pNew->Class, "custom") == 0 ) {
		printf("[xs] server reload '%s' rejected: target custom generation has no lease boundary\n", sName);
		XS_ConfigFree(&tFresh);
		goto Done;
	}
	if ( XS_ServerBindConflicts(pOld, pNew) ) {
		printf("[xs] server reload '%s' rejected: structural change keeps the same bind endpoint; "
			"safe candidate-first rebuild is impossible\n", sName);
		XS_ConfigFree(&tFresh);
		goto Done;
	}

	pOldScript = XS_ScriptAcquireHost(pOld->DefaultHost);
	if ( pOldScript != NULL && pOldScript->procSwap != NULL ) {
		xvalue* pOut = NULL;
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pOldScript);

		if ( pOldScript->procSwap(pOld->DefaultHost, &pOut) && pOut != NULL ) pShared = pOut;
		XS_ScriptLeave(pPrevious);
	}
	XS_ScriptRelease(pOldScript);

	pOwner = XS_ReloadTakeSnapshot(&tFresh);
	if ( pOwner == NULL ) {
		if ( pShared != NULL ) xrtValueRelease(pShared);
		XS_ConfigFree(&tFresh);
		goto Done;
	}
	pOwner->Engine = pApp->Engine;
	pNew = pOwner->Servers[i];
	pNew->Engine = pApp->Engine;
	pNew->ConfigOwner = pOwner;
	sErr[0] = '\0';
	if ( !XS_ReloadPrepareServer(pNew, &pShared, sErr, sizeof(sErr)) ) {
		printf("[xs] server reload '%s': candidate failed: %s; old generation unchanged\n",
			sName, sErr);
		if ( pShared != NULL ) xrtValueRelease(pShared);
		XS_ReloadDiscardCandidate(pNew);
		goto Done;
	}
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		if ( pApp->Servers[i] == pOld ) {
			pApp->Servers[i] = pNew;
			break;
		}
	}
	XS_ServerDriverStop(pOld);
	(void)XS_GcRetireServer(pOld, pOld->ConfigOwner == NULL);
	printf("[xs] server reload '%s': candidate active; old generation draining\n", sName);
	bOk = true;

Done:
	g_XS_ReloadBusy = false;
	return bOk;
}

static bool XS_ReloadRemoveServer(XS_App* pApp, const char* sName)
{
	uint32 i;
	XS_ServerInfo* pOld;

	for ( i = 0; i < pApp->ServerCount; i++ ) {
		if ( strcmp(pApp->Servers[i]->Name, sName) == 0 ) break;
	}
	if ( i >= pApp->ServerCount ) return true;
	pOld = pApp->Servers[i];
	if ( !XS_ReloadClassSupported(pOld, "remove") ) return false;
	for ( ; i + 1 < pApp->ServerCount; i++ ) pApp->Servers[i] = pApp->Servers[i + 1];
	pApp->Servers[--pApp->ServerCount] = NULL;
	XS_ServerDriverStop(pOld);
	(void)XS_GcRetireServer(pOld, pOld->ConfigOwner == NULL);
	printf("[xs] reload all: server '%s' removed; old generation draining\n", sName);
	return true;
}

static bool XS_ReloadAddServer(XS_App* pApp, const char* sName)
{
	XS_App tFresh;
	XS_App* pOwner;
	XS_ServerInfo* pAdd;
	XS_ServerInfo** pServers;
	xvalue* pShared = NULL;
	char sCfgPath[4096];
	char sErr[256];
	uint32 i;

	snprintf(sCfgPath, sizeof(sCfgPath), "%.200s/xs.json", XS_AppPath());
	if ( !XS_ConfigLoad(sCfgPath, &tFresh) ) return false;
	for ( i = 0; i < tFresh.ServerCount; i++ ) {
		if ( strcmp(tFresh.Servers[i]->Name, sName) == 0 ) break;
	}
	if ( i >= tFresh.ServerCount || strcmp(tFresh.Servers[i]->Class, "custom") == 0 ) {
		XS_ConfigFree(&tFresh);
		return false;
	}
	pOwner = XS_ReloadTakeSnapshot(&tFresh);
	if ( pOwner == NULL ) {
		XS_ConfigFree(&tFresh);
		return false;
	}
	pOwner->Engine = pApp->Engine;
	pAdd = pOwner->Servers[i];
	pAdd->Engine = pApp->Engine;
	pAdd->ConfigOwner = pOwner;
	sErr[0] = '\0';
	if ( !XS_ReloadPrepareServer(pAdd, &pShared, sErr, sizeof(sErr)) ) {
		printf("[xs] reload all: add '%s' failed: %s\n", sName, sErr);
		XS_ReloadDiscardCandidate(pAdd);
		return false;
	}
	pServers = (XS_ServerInfo**)xrtRealloc(pApp->Servers,
		sizeof(XS_ServerInfo*) * (size_t)(pApp->ServerCount + 1));
	if ( pServers == NULL ) {
		XS_ServerDriverStop(pAdd);
		XS_ReloadDiscardCandidate(pAdd);
		return false;
	}
	pApp->Servers = pServers;
	pApp->Servers[pApp->ServerCount++] = pAdd;
	printf("[xs] reload all: server '%s' added\n", sName);
	return true;
}

static bool XS_NameInList(char** arrNames, uint32 iCount, const char* sName)
{
	uint32 i;

	for ( i = 0; i < iCount; i++ ) if ( strcmp(arrNames[i], sName) == 0 ) return true;
	return false;
}

static void XS_NameListFree(char** arrNames, uint32 iCount)
{
	uint32 i;

	for ( i = 0; i < iCount; i++ ) xrtFree(arrNames[i]);
	xrtFree(arrNames);
}

static bool XS_ReloadAllNow(XS_App* pApp)
{
	XS_App tFresh;
	char sCfgPath[4096];
	char** arrDesired;
	char** arrCurrent;
	uint32 i;
	uint32 iDesired;
	uint32 iCurrent = pApp->ServerCount;
	bool bOk = true;

	if ( g_XS_ReloadBusy ) return false;
	snprintf(sCfgPath, sizeof(sCfgPath), "%.200s/xs.json", XS_AppPath());
	if ( !XS_ConfigLoad(sCfgPath, &tFresh) ) {
		printf("[xs] reload all: config re-parse failed: %s\n", tFresh.ParseError);
		XS_ConfigFree(&tFresh);
		return false;
	}
	iDesired = tFresh.ServerCount;
	arrDesired = (char**)xrtCalloc(iDesired > 0 ? iDesired : 1, sizeof(char*));
	arrCurrent = (char**)xrtCalloc(iCurrent > 0 ? iCurrent : 1, sizeof(char*));
	if ( arrDesired == NULL || arrCurrent == NULL ) {
		xrtFree(arrDesired); xrtFree(arrCurrent); XS_ConfigFree(&tFresh); return false;
	}
	for ( i = 0; i < iDesired; i++ ) {
		arrDesired[i] = xrtStrDup(tFresh.Servers[i]->Name);
		if ( arrDesired[i] == NULL ) {
			XS_NameListFree(arrDesired, iDesired);
			xrtFree(arrCurrent);
			XS_ConfigFree(&tFresh);
			return false;
		}
	}
	for ( i = 0; i < iCurrent; i++ ) {
		arrCurrent[i] = xrtStrDup(pApp->Servers[i]->Name);
		if ( arrCurrent[i] == NULL ) {
			XS_NameListFree(arrDesired, iDesired);
			XS_NameListFree(arrCurrent, iCurrent);
			XS_ConfigFree(&tFresh);
			return false;
		}
	}
	XS_ConfigFree(&tFresh);

	for ( i = 0; i < iCurrent; i++ ) {
		if ( XS_NameInList(arrDesired, iDesired, arrCurrent[i]) ) {
			bOk = XS_ReloadServerNow(pApp, arrCurrent[i]) && bOk;
		} else {
			bOk = XS_ReloadRemoveServer(pApp, arrCurrent[i]) && bOk;
		}
	}
	for ( i = 0; i < iDesired; i++ ) {
		if ( !XS_NameInList(arrCurrent, iCurrent, arrDesired[i]) ) {
			bOk = XS_ReloadAddServer(pApp, arrDesired[i]) && bOk;
		}
	}
	XS_NameListFree(arrDesired, iDesired);
	XS_NameListFree(arrCurrent, iCurrent);
	return bOk;
}

/* lifecycle timer 同时持有脚本代和服务代；取消也必须经过唯一终态回调。 */
typedef struct XS_TimerWrap {
	XS_GenerationTimer	tTimer;
	XS_TimerProc		proc;
	void*			pUserData;
	XS_ScriptRuntime*	pScript;
} XS_TimerWrap;

static void XS_TimerFire(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_TimerWrap* pWrap = (XS_TimerWrap*)pData;
	XS_ServerGeneration* pGeneration = XS_GenerationTimerFinish(&pWrap->tTimer);
	XS_ScriptRuntime* pScript = pWrap->pScript;

	(void)pWorker; (void)iId;
	if ( iResult == XNET_RESULT_OK && !XS_GenerationIsRetired(pGeneration) &&
	     !XS_ScriptIsRetired(pScript) ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		pWrap->proc(pWrap->pUserData);
		XS_ScriptLeave(pPrevious);
	}
	XS_ScriptRelease(pScript);
	xrtFree(pWrap);
	XS_GenerationRelease(pGeneration);
}

static uint64 XS_TimerAfter(XS_HostInfo* pOwner, uint32 iMillisecond, XS_TimerProc proc, void* pUserData)
{
	XS_TimerWrap* pWrap;
	XS_ScriptRuntime* pScript;
	XS_ServerGeneration* pGeneration;
	uint64 iId;

	if ( pOwner == NULL || pOwner->Server == NULL || pOwner->Server->Engine == NULL ||
	     proc == NULL ) {
		return 0;
	}
	pScript = g_XS_CurrentScript;
	if ( pScript == NULL || pScript->pHost != pOwner || !XS_ScriptRetain(pScript) ) {
		pScript = XS_ScriptAcquireHost(pOwner);
	}
	if ( pScript == NULL ) return 0;
	pGeneration = (XS_ServerGeneration*)pOwner->Server->Generation;
	pWrap = (XS_TimerWrap*)xrtCalloc(1, sizeof(XS_TimerWrap));
	if ( pWrap == NULL ) {
		XS_ScriptRelease(pScript);
		return 0;
	}
	pWrap->proc = proc;
	pWrap->pUserData = pUserData;
	pWrap->pScript = pScript;
	iId = XS_GenerationTimerSchedule(pGeneration, (uint64)iMillisecond * 1000,
		XS_TimerFire, pWrap, pScript, &pWrap->tTimer);
	if ( iId == 0 ) {
		XS_ScriptRelease(pScript);
		xrtFree(pWrap);
	}
	return iId;
}

#endif
