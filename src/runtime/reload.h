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
#include "../core/driver.h"
#include "gc.h"

static volatile bool g_XS_ReloadBusy = false;
static XS_ScriptRuntime* g_XS_Retired = NULL;

/* server 是否无活动连接（http/tcp 走注册表计数；custom 无 xs 侧视图 → false） */
static bool XS_HostHasScript(XS_HostInfo* pHost);
static bool XS_UdpStart(XS_ServerInfo* pServer, char* sErr, size_t iErrCap);
static bool XS_WsStart(XS_ServerInfo* pServer, char* sErr, size_t iErrCap);
static bool XS_ScriptLoad(XS_HostInfo* pHost);

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
static bool XS_ReloadHostInner(XS_HostInfo* pHost)
{
	XS_ScriptRuntime* pOld;
	XS_ScriptRuntime* pNew;
	xvalue* pShared = NULL;

	if ( pHost == NULL || pHost->Runtime == NULL ) {
		return false;
	}
	pNew = XS_ScriptCompile(pHost);
	if ( pNew == NULL ) {
		printf("[xs] reload '%s' compile failed, old generation keeps serving\n",
			pHost->Name != NULL ? pHost->Name : "?");
		pHost->State = XS_RUN_RELOAD_FAILED;
		if ( pHost->Server != NULL ) {
			pHost->Server->State = XS_RUN_RELOAD_FAILED;
		}
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
	{
		uint64 tOldGen = pOld->tGeneration;
		XS_RetiredEnqueue(pOld);
	XS_ScriptAttach(pHost, pNew);
	pHost->State = XS_RUN_RUNNING;
	if ( pHost->Server != NULL ) {
		pHost->Server->State = XS_RUN_RUNNING;
	}
	printf("[xs] reload '%s' ok (gen %llu -> %llu)\n",
		pHost->Name != NULL ? pHost->Name : "?",
		(unsigned long long)tOldGen, (unsigned long long)pNew->tGeneration);
	}
	return true;
}

/* Host 级重载入口（并发窗口互斥；编译失败 = 回滚） */
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

/* ============================================================
 * Server 级配置重载（设计 §10.2 完整时序）
 * 结构比对：配置未变 → 逐 host 脚本重载 + 证书热替换；
 *          配置有变 → 驱动收口 → 新配置重建驱动 → 脚本重载。
 * v1 比对字段：class/ip/port/tls/port_tls/backlog/recv_limit/devfile 拓扑
 * ============================================================ */

static bool XS_StrEq(const char* a, const char* b)
{
	if ( a == NULL || b == NULL ) {
		return a == b;
	}
	return strcmp(a, b) == 0;
}

static bool XS_ServerStructEquals(XS_ServerInfo* pA, XS_ServerInfo* pB)
{
	if ( strcmp(pA->Class, pB->Class) != 0 ||
	     !XS_StrEq(pA->IP, pB->IP) || pA->Port != pB->Port ||
	     pA->TLS != pB->TLS || pA->PortTLS != pB->PortTLS ||
	     pA->Backlog != pB->Backlog || pA->RecvLimit != pB->RecvLimit ||
	     pA->HostCount != pB->HostCount ) {
		return false;
	}
	{
		uint32 i;

		for ( i = 0; i < pA->HostCount; i++ ) {
			XS_HostInfo* pHA = pA->Hosts[i];
			XS_HostInfo* pHB = pB->Hosts[i];

			if ( !XS_StrEq(pHA->Name, pHB->Name) || !XS_StrEq(pHA->Host, pHB->Host) ||
			     !XS_StrEq(pHA->Path, pHB->Path) || !XS_StrEq(pHA->DevFile, pHB->DevFile) ||
			     !XS_StrEq(pHA->TlsCert, pHB->TlsCert) || !XS_StrEq(pHA->TlsKey, pHB->TlsKey) ) {
				return false;
			}
		}
	}
	return true;
}

/* 延迟重载请求（回调内调用 xsReload* 时避免自毁当前调用栈） */
typedef struct XS_DeferredReload {
	XS_App*			pApp;
	XS_HostInfo*		pHost;		/* host 级（三选一） */
	const char*		sServerName;	/* server 级（strdup） */
	bool			bAll;
} XS_DeferredReload;

static void XS_DeferredFire(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData);
static bool XS_ReloadServerNow(XS_App* pApp, const char* sName);
static bool XS_ReloadAllNow(XS_App* pApp);

static bool XS_DeferReload(XS_App* pApp, XS_HostInfo* pHost, const char* sName, bool bAll)
{
	XS_DeferredReload* pReq = (XS_DeferredReload*)xrtCalloc(1, sizeof(XS_DeferredReload));

	if ( pReq == NULL || pApp->Engine == NULL ) {
		xrtFree(pReq);
		return false;
	}
	pReq->pApp = pApp;
	pReq->pHost = pHost;
	pReq->sServerName = sName != NULL ? xrtStrDup(sName) : NULL;
	pReq->bAll = bAll;
	return xrtNetEngineAfter(pApp->Engine, 0, 1000, XS_DeferredFire, pReq) != 0;
}

static void XS_DeferredFire(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_DeferredReload* pReq = (XS_DeferredReload*)pData;

	(void)pWorker; (void)iId;
	if ( iResult == XNET_RESULT_OK ) {
		if ( pReq->pHost != NULL ) {
			(void)XS_ReloadHostNow(pReq->pHost);
		} else if ( pReq->sServerName != NULL ) {
			(void)XS_ReloadServerNow(pReq->pApp, pReq->sServerName);
		} else if ( pReq->bAll ) {
			(void)XS_ReloadAllNow(pReq->pApp);
		}
	}
	xrtFree((void*)pReq->sServerName);
	xrtFree(pReq);
}

/* Server 级：重读配置 → 结构比对 → 分流 */
static bool XS_ReloadServerNow(XS_App* pApp, const char* sName)
{
	XS_ServerInfo* pOld;
	XS_App tFresh;
	char sCfgPath[4096];
	uint32 i;
	bool bOk = true;

	if ( g_XS_ReloadBusy ) {
		printf("[xs] reload busy\n");
		return false;
	}
	pOld = xsServerFind(sName);
	if ( pOld == NULL ) {
		return false;
	}
	snprintf(sCfgPath, sizeof(sCfgPath), "%.200s/xs.json", XS_AppPath());
	g_XS_ReloadBusy = true;
	if ( !XS_ConfigLoad(sCfgPath, &tFresh) ) {
		printf("[xs] server reload: config re-parse failed: %s\n", tFresh.ParseError);
		XS_ConfigFree(&tFresh);
		pOld->State = XS_RUN_RELOAD_FAILED;
		g_XS_ReloadBusy = false;
		return false;
	}
	for ( i = 0; i < tFresh.ServerCount; i++ ) {
		if ( strcmp(tFresh.Servers[i]->Name, sName) == 0 ) {
			break;
		}
	}
	if ( i >= tFresh.ServerCount ) {
		printf("[xs] server reload: '%s' removed from config (shutdown via xsReloadAll)\n", sName);
		XS_ConfigFree(&tFresh);
		g_XS_ReloadBusy = false;
		return false;
	}

		XS_ServerInfo* pNewCfg = tFresh.Servers[i];

		if ( XS_ServerStructEquals(pOld, pNewCfg) ) {
			char sErr[256];

			sErr[0] = '\0';
			/* 结构未变：证书热替换 + 逐 host 脚本重载（复用旧结构体） */
			if ( pOld->TLS &&
			     (strcmp(pOld->Class, "tcp") == 0 || strcmp(pOld->Class, "http") == 0 ||
			      strcmp(pOld->Class, "ws") == 0) ) {
				void* pDriver = pOld->Runtime;

				/* 取驱动内的 TlsTable（各驱动布局不同，按类取） */
				if ( strcmp(pOld->Class, "tcp") == 0 && pDriver != NULL ) {
					(void)XS_TlsTableRefresh(pOld, &((XS_TcpRuntime*)pDriver)->tTls, sErr, sizeof(sErr));
				} else if ( strcmp(pOld->Class, "http") == 0 && pDriver != NULL ) {
					(void)XS_TlsTableRefresh(pOld, &((XS_HttpRuntime*)pDriver)->tTls, sErr, sizeof(sErr));
				} else if ( strcmp(pOld->Class, "ws") == 0 && pDriver != NULL ) {
					(void)XS_TlsTableRefresh(pOld, &((XS_WsRuntime*)pDriver)->tTls, sErr, sizeof(sErr));
				}
				if ( sErr[0] != '\0' ) {
					printf("[xs] %s\n", sErr);
				}
			}
			if ( pOld->DefaultHost->Runtime != NULL ) {
				bOk = XS_ReloadHostInner(pOld->DefaultHost) && bOk;
			}
			for ( i = 0; i < pOld->HostCount; i++ ) {
				if ( pOld->Hosts[i]->Runtime != NULL ) {
					bOk = XS_ReloadHostInner(pOld->Hosts[i]) && bOk;
				}
			}
			XS_ConfigFree(&tFresh);
			g_XS_ReloadBusy = false;
			return bOk;
		}
	else {
	/* 结构有变：listener 在线重建（设计 §10.2 完整时序）
	 * 1. ServiceSwap 导出 → 2. 旧驱动收口 + 等排空 → 3. 新配置装配 →
	 * 失败回滚旧配置 → 4. 旧结构体退役链延迟释放 */

		XS_ServerInfo* pNewCfg = tFresh.Servers[i];
		XS_ScriptRuntime* pOldRt = (XS_ScriptRuntime*)pOld->DefaultHost->Runtime;
		xvalue* pShared = NULL;
		char sErr2[256];

		sErr2[0] = '\0';
		printf("[xs] server reload '%s': structural change, rebuilding listener\n", sName);
		if ( pOldRt != NULL && pOldRt->procSwap != NULL ) {
			xvalue* pOut = NULL;

			if ( pOldRt->procSwap(pOld->DefaultHost, &pOut) && pOut != NULL ) {
				pShared = pOut;
			}
		}
		XS_ServerDriverStop(pOld);
		if ( !XS_ServerDrain(pApp, 5000) ) {
			printf("[xs] server reload '%s': drain timeout, forcing rebuild\n", sName);
		}
		/* 旧驱动 runtime / TCC / server 结构体交给 GC 延迟释放
		 * （连接计数归零 = 所有 Close 回调完成 = 安全） */

		/* 新配置装配（新结构体接管拓扑） */
		pNewCfg->Engine = pApp->Engine;
		if ( XS_HostHasScript(pNewCfg->DefaultHost) ) {
			XS_ScriptRuntime* pNewRt = XS_ScriptCompile(pNewCfg->DefaultHost);

			if ( pNewRt != NULL ) {
				pNewRt->pSwap = pShared;
				XS_ScriptAttach(pNewCfg->DefaultHost, pNewRt);
			} else {
				printf("[xs] server rebuild '%s': new script compile failed\n", sName);
			}
		}
		if ( XS_ServerDriverStart(pNewCfg, sErr2, sizeof(sErr2)) ) {
			uint32 iSlot;

			for ( iSlot = 0; iSlot < pApp->ServerCount; iSlot++ ) {
				if ( pApp->Servers[iSlot] == pOld ) {
					tFresh.Servers[i] = NULL;		/* 移交所有权 */
					pApp->Servers[iSlot] = pNewCfg;
					break;
				}
			}
			XS_ScriptUnitHost(pOld->DefaultHost);
			/* 旧代三件套入 GC：驱动 runtime + TCC 状态 + server 结构体
			 * GC 在连接归零后按序释放（见 src/runtime/gc.h） */
			XS_GcEnqueue(pApp->Engine, pOld->Runtime,
				(XS_ScriptRuntime*)pOld->DefaultHost->Runtime, pOld);
			/* 不释放 tFresh.Taken/Root：新 server 的 Custom 借用其中 xvalue 视图。
			 * 仅释放未移交的 server 结构（字符串字段），xvalue 树随进程存活 */
			{
				uint32 k;

				for ( k = 0; k < tFresh.ServerCount; k++ ) {
					if ( tFresh.Servers[k] != NULL ) {
						/* 其他 server：释放其字符串（xvalue 树同样借用不释放） */
						xrtFree((void*)tFresh.Servers[k]->Class);
						xrtFree((void*)tFresh.Servers[k]->Name);
						xrtFree((void*)tFresh.Servers[k]->IP);
						xrtFree((void*)tFresh.Servers[k]->IPTLS);
						xrtFree(tFresh.Servers[k]->DefaultHost);
						xrtFree(tFresh.Servers[k]->Hosts);
						xrtFree(tFresh.Servers[k]);
					}
				}
				xrtFree(tFresh.Servers);
				tFresh.Servers = NULL;
				tFresh.ServerCount = 0;
				tFresh.Taken = NULL;		/* 放弃 Taken 所有权（防 ConfigFree 释放借用树） */
				tFresh.TakenCount = 0;
				tFresh.Root = NULL;
			}
			printf("[xs] server reload '%s': rebuild ok\n", sName);
		} else {
			/* 失败回滚：旧配置重建（不进拓扑——旧结构体仍在槽位） */
			printf("[xs] server rebuild '%s' failed: %s, rolling back\n", sName, sErr2);
			if ( XS_HostHasScript(pOld->DefaultHost) ) {
				XS_ScriptRuntime* pRollRt = XS_ScriptCompile(pOld->DefaultHost);

				if ( pRollRt != NULL ) {
					pRollRt->pSwap = pShared;
					XS_ScriptAttach(pOld->DefaultHost, pRollRt);
				}
			}
			if ( XS_ServerDriverStart(pOld, sErr2, sizeof(sErr2)) ) {
				printf("[xs] server reload '%s': rollback ok (old config serving)\n", sName);
			} else {
				printf("[xs] server reload '%s': ROLLBACK FAILED: %s\n", sName, sErr2);
				bOk = false;
			}
		}
		XS_ConfigFree(&tFresh);
		g_XS_ReloadBusy = false;
		return bOk;
	}
	}
/* All 级：重读配置 → 逐 server 分流（现存的按 server 级；新增的装配；消失的收口下线） */
static bool XS_ReloadAllNow(XS_App* pApp)
{
	XS_App tFresh;
	char sCfgPath[4096];
	uint32 i, j;
	bool bOk = true;

	if ( g_XS_ReloadBusy ) {
		return false;
	}
	snprintf(sCfgPath, sizeof(sCfgPath), "%.200s/xs.json", XS_AppPath());
	g_XS_ReloadBusy = true;
	if ( !XS_ConfigLoad(sCfgPath, &tFresh) ) {
		printf("[xs] reload all: config re-parse failed: %s\n", tFresh.ParseError);
		XS_ConfigFree(&tFresh);
		g_XS_ReloadBusy = false;
		return false;
	}
	/* 现存 server：在新配置中找同名 → 比对（v1：脚本级 + 证书热替换） */
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		XS_ServerInfo* pOld = pApp->Servers[i];

		for ( j = 0; j < tFresh.ServerCount; j++ ) {
			if ( strcmp(tFresh.Servers[j]->Name, pOld->Name) == 0 ) {
				break;
			}
		}
		if ( j < tFresh.ServerCount && XS_ServerStructEquals(pOld, tFresh.Servers[j]) ) {
			if ( pOld->DefaultHost->Runtime != NULL ) {
				bOk = XS_ReloadHostInner(pOld->DefaultHost) && bOk;
			}
		} else if ( j >= tFresh.ServerCount ) {
			printf("[xs] reload all: server '%s' removed from config "
				"(v1: keep running, restart process to remove)\n", pOld->Name);
		} else {
			printf("[xs] reload all: server '%s' structural change "
				"(v1: scripts only, restart for listener changes)\n", pOld->Name);
			if ( XS_HostHasScript(pOld->DefaultHost) ) {
				bOk = XS_ReloadHostInner(pOld->DefaultHost) && bOk;
			}
		}
	}
	/* 新增 server：装配（引擎与 VFS 环境共享） */
	for ( j = 0; j < tFresh.ServerCount; j++ ) {
		bool bExists = false;

		for ( i = 0; i < pApp->ServerCount; i++ ) {
			if ( strcmp(pApp->Servers[i]->Name, tFresh.Servers[j]->Name) == 0 ) {
				bExists = true;
				break;
			}
		}
		if ( !bExists ) {
			XS_ServerInfo* pAdd = tFresh.Servers[j];
			char sErr[256];

			sErr[0] = '\0';
			pAdd->Engine = pApp->Engine;
			tFresh.Servers[j] = NULL;		/* 移交所有权 */
			{
				XS_ServerInfo** pNewArr = (XS_ServerInfo**)xrtRealloc(pApp->Servers,
					sizeof(XS_ServerInfo*) * (size_t)(pApp->ServerCount + 1));
				if ( pNewArr != NULL ) {
					pNewArr[pApp->ServerCount++] = pAdd;
					pApp->Servers = pNewArr;
					/* 脚本 + 驱动装配（复用启动装配路径，单 server 版） */
					if ( XS_HostHasScript(pAdd->DefaultHost) ) {
						(void)XS_ScriptLoad(pAdd->DefaultHost);
					}
					if ( strcmp(pAdd->Class, "custom") == 0 ) {
						pAdd->State = XS_RUN_RUNNING;
						printf("[xs] reload all: server '%s' added (custom)\n", pAdd->Name);
					} else if ( strcmp(pAdd->Class, "http") == 0 ) {
						if ( XS_HttpStart(pAdd, sErr, sizeof(sErr)) ) {
							pAdd->State = XS_RUN_RUNNING;
						} else {
							printf("[xs] %s\n", sErr);
						}
					} else if ( strcmp(pAdd->Class, "tcp") == 0 ) {
						if ( XS_TcpStart(pAdd, sErr, sizeof(sErr)) ) {
							pAdd->State = XS_RUN_RUNNING;
						} else {
							printf("[xs] %s\n", sErr);
						}
					} else if ( strcmp(pAdd->Class, "udp") == 0 ) {
						if ( XS_UdpStart(pAdd, sErr, sizeof(sErr)) ) {
							pAdd->State = XS_RUN_RUNNING;
						} else {
							printf("[xs] %s\n", sErr);
						}
					} else if ( strcmp(pAdd->Class, "ws") == 0 ) {
						if ( XS_WsStart(pAdd, sErr, sizeof(sErr)) ) {
							pAdd->State = XS_RUN_RUNNING;
						} else {
							printf("[xs] %s\n", sErr);
						}
					}
				}
			}
		}
	}
	XS_ConfigFree(&tFresh);
	g_XS_ReloadBusy = false;
	return bOk;
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