#ifndef XS_RUNTIME_RELOAD_H
#define XS_RUNTIME_RELOAD_H

/*
 * xs3 热重载（设计 §10）：generation 换代 + 回滚语义 + 退役代回收。
 *
 * 换代时序：编译新脚本（失败→旧代原样服务）→ 旧代 ServiceSwap 导出 →
 * 旧代退役挂链 → 新代挂载并 ServiceInit（xsSwapTake 取交接）→
 * 退役代等各驱动无活动连接后 ServiceUnit + 销毁（定时扫描）。
 *
 * v1 边界：xsReloadServer/xsReloadAll 为"逐 host 脚本重载"（监听器与配置
 * 不重建——配置热加载随连接注册表深化工作包）。
 */

#include <stdio.h>

#include "../sdk/xsbase.h"
#include "../core/config.h"
#include "../core/engine.h"
#include "../script/script.h"
#include "../protocol/http.h"
#include "../protocol/stream.h"
#include "../protocol/ws.h"

static volatile bool g_XS_ReloadBusy = false;
static XS_ScriptRuntime* g_XS_Retired = NULL;

/* server 是否无活动连接（http/tcp 走注册表计数；custom 无 xs 侧视图 → false） */
static bool XS_ReloadServerIdle(XS_ServerInfo* pServer)
{
	if ( strcmp(pServer->Class, "tcp") == 0 ) {
		XS_TcpRuntime* pTcp = (XS_TcpRuntime*)pServer->Runtime;
		return pTcp == NULL || pTcp->tRegistry.iCount == 0;
	}
	if ( strcmp(pServer->Class, "http") == 0 ) {
		XS_HttpRuntime* pHttp = (XS_HttpRuntime*)pServer->Runtime;
		return pHttp == NULL || pHttp->tRegistry.iCount == 0;
	}
	if ( strcmp(pServer->Class, "ws") == 0 ) {
		XS_WsRuntime* pWs = (XS_WsRuntime*)pServer->Runtime;
		return pWs == NULL || pWs->iConnCount == 0;
	}
	if ( strcmp(pServer->Class, "udp") == 0 ) {
		return true;
	}
	return false;		/* custom：脚本自持连接不可见，停机时统一回收 */
}

static void XS_RetiredSweepProc(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_ScriptRuntime** ppLink;
	XS_ScriptRuntime* pFree;
	xnetengine* pEngine = (xnetengine*)pData;

	(void)pWorker; (void)iId;
	if ( iResult != XNET_RESULT_OK ) {
		return;
	}
	ppLink = &g_XS_Retired;
	while ( *ppLink != NULL ) {
		XS_ScriptRuntime* pR = *ppLink;

		if ( pR->pHost != NULL && pR->pHost->Server != NULL &&
		     XS_ReloadServerIdle(pR->pHost->Server) ) {
			*ppLink = pR->pRetiredNext;
			if ( pR->procUnit != NULL ) {
				pR->procUnit(pR->pHost);
			}
			tcc_delete(pR->pTcc);
			xrtFree(pR);
		} else {
			ppLink = &pR->pRetiredNext;
		}
	}
	if ( g_XS_Retired != NULL && pEngine != NULL ) {
		(void)xrtNetEngineAfter(pEngine, 0, 1000 * 1000, XS_RetiredSweepProc, pData);
	}
	pFree = NULL;
	(void)pFree;
}

static void XS_RetiredEnqueue(XS_ScriptRuntime* pRuntime)
{
	pRuntime->bRetired = true;
	pRuntime->pRetiredNext = g_XS_Retired;
	g_XS_Retired = pRuntime;
	XS_RetiredSweepProc(NULL, 0, XNET_RESULT_OK,
		pRuntime->pHost != NULL && pRuntime->pHost->Server != NULL ? pRuntime->pHost->Server->Engine : NULL);
}

/* Host 级重载（并发窗口互斥；编译失败 = 回滚） */
static bool XS_ReloadHostNow(XS_HostInfo* pHost)
{
	XS_ScriptRuntime* pOld;
	XS_ScriptRuntime* pNew;
	xvalue* pShared = NULL;

	if ( g_XS_ReloadBusy ) {
		printf("[xs] reload busy\n");
		return false;
	}
	if ( pHost == NULL || pHost->Runtime == NULL ) {
		return false;
	}
	g_XS_ReloadBusy = true;

	pNew = XS_ScriptCompile(pHost);
	if ( pNew == NULL ) {
		printf("[xs] reload '%s' compile failed, old generation keeps serving\n",
			pHost->Name != NULL ? pHost->Name : "?");
		pHost->State = XS_RUN_RELOAD_FAILED;
		if ( pHost->Server != NULL ) {
			pHost->Server->State = XS_RUN_RELOAD_FAILED;
		}
		g_XS_ReloadBusy = false;
		return false;		/* 回滚语义 */
	}
	pOld = (XS_ScriptRuntime*)pHost->Runtime;
	if ( pOld->procSwap != NULL ) {
		xvalue* pOut = NULL;

		if ( pOld->procSwap(pHost, &pOut) && pOut != NULL ) {
			pShared = pOut;
		}
	}
	pNew->pSwap = pShared;
	XS_RetiredEnqueue(pOld);
	XS_ScriptAttach(pHost, pNew);
	pHost->State = XS_RUN_RUNNING;
	if ( pHost->Server != NULL ) {
		pHost->Server->State = XS_RUN_RUNNING;
	}
	printf("[xs] reload '%s' ok (gen %llu -> %llu)\n",
		pHost->Name != NULL ? pHost->Name : "?",
		(unsigned long long)pOld->tGeneration, (unsigned long long)pNew->tGeneration);
	g_XS_ReloadBusy = false;
	return true;
}

/* ============================================================
 * 生命周期定时器（绑定代际：退役代的定时器自然作废）
 * ============================================================ */

typedef struct XS_TimerWrap {
	XS_TimerProc		proc;
	void*			pUserData;
	XS_ScriptRuntime*	pGen;		/* 注册时的代 */
} XS_TimerWrap;

static void XS_TimerFire(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_TimerWrap* pWrap = (XS_TimerWrap*)pData;

	(void)pWorker; (void)iId;
	if ( iResult == XNET_RESULT_OK && !pWrap->pGen->bRetired ) {
		pWrap->proc(pWrap->pUserData);
	}
	xrtFree(pWrap);
}

static uint64 XS_TimerAfter(XS_HostInfo* pOwner, uint32 iMillisecond, XS_TimerProc proc, void* pUserData)
{
	XS_TimerWrap* pWrap;
	xnetengine* pEngine;

	if ( pOwner == NULL || pOwner->Server == NULL || pOwner->Server->Engine == NULL ||
	     pOwner->Runtime == NULL ) {
		return 0;
	}
	pWrap = (XS_TimerWrap*)xrtCalloc(1, sizeof(XS_TimerWrap));
	if ( pWrap == NULL ) {
		return 0;
	}
	pWrap->proc = proc;
	pWrap->pUserData = pUserData;
	pWrap->pGen = (XS_ScriptRuntime*)pOwner->Runtime;
	pEngine = pOwner->Server->Engine;
	return xrtNetEngineAfter(pEngine, 0, (uint64)iMillisecond * 1000, XS_TimerFire, pWrap);
}

#endif
