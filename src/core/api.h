#ifndef XS_CORE_API_H
#define XS_CORE_API_H

/*
 * xs3 契约 API 实现
 * 配置访问使用显式 generation lease；reload / timer 由 runtime 工作包实现。
 */

#include "../sdk/xsbase.h"
#include "config.h"
#include "engine.h"
#include "../script/tcc_host.h"
#include "../runtime/topology.h"
#include "../runtime/reload.h"

static XS_App* g_XS_App = NULL;

XS_ServerInfo* xsServerFind(const char* sName)
{
	return XS_TopologyServerAcquire(sName);
}

XS_ServerInfo* xsServerRetain(XS_ServerInfo* pServer)
{
	return XS_TopologyServerRetain(pServer);
}

void xsServerRelease(XS_ServerInfo* pServer)
{
	XS_TopologyServerRelease(pServer);
}

void xsEnumServers(XS_ServerEnumProc procEach, void* pUserData)
{
	XS_ServerInfo** pServers;
	uint32 iCount;
	uint32 i;

	if ( procEach == NULL || !XS_TopologyServerSnapshot(&pServers, &iCount) ) {
		return;
	}
	for ( i = 0; i < iCount; i++ ) {
		if ( !procEach(pServers[i], pUserData) ) break;
	}
	for ( i = 0; i < iCount; i++ ) XS_TopologyServerRelease(pServers[i]);
	xrtFree(pServers);
}

XS_HostInfo* xsHostFind(XS_ServerInfo* pServer, const char* sName)
{
	uint32 i;

	if ( pServer == NULL || sName == NULL ) {
		return NULL;
	}
	if ( pServer->DefaultHost != NULL && strcmp(pServer->DefaultHost->Name, sName) == 0 ) {
		return pServer->DefaultHost;
	}
	for ( i = 0; i < pServer->HostCount; i++ ) {
		if ( strcmp(pServer->Hosts[i]->Name, sName) == 0 ) {
			return pServer->Hosts[i];
		}
	}
	return NULL;
}

void xsEnumHosts(XS_ServerInfo* pServer, XS_HostEnumProc procEach, void* pUserData)
{
	uint32 i;

	if ( pServer == NULL || procEach == NULL ) {
		return;
	}
	if ( pServer->DefaultHost != NULL && !procEach(pServer->DefaultHost, pUserData) ) {
		return;
	}
	for ( i = 0; i < pServer->HostCount; i++ ) {
		if ( !procEach(pServer->Hosts[i], pUserData) ) {
			return;
		}
	}
}

xvalue* xsConfigRoot(void)
{
	return XS_TopologyRootAcquire();
}

TCCState* xsCreateTCC(void)
{
	return XS_TccCreate();
}

void xsDestroyTCC(TCCState* pTcc)
{
	if ( pTcc != NULL ) {
		tcc_delete(pTcc);
	}
}

/* —— 重载与定时器（src/runtime/reload.h）——
 * 重载一律延迟到固定 worker，调用点永不在自己的 TCC 回调栈上卸载。 */

bool xsReloadHost(XS_HostInfo* pHost)
{
	if ( pHost == NULL ) {
		return false;
	}
	return XS_DeferReload(g_XS_App, pHost, NULL, false);
}

bool xsReloadServer(const char* sName)
{
	if ( sName == NULL || g_XS_App == NULL ) {
		return false;
	}
	/* server 级重建总是延迟执行：结构变更路径要拆旧监听器，
	 * 同步执行会摧毁当前回调栈（回调正跑在旧结构上） */
	return XS_DeferReload(g_XS_App, NULL, sName, false);
}

bool xsReloadAll(void)
{
	if ( g_XS_App == NULL ) {
		return false;
	}
	return XS_DeferReload(g_XS_App, NULL, NULL, true);
}

uint64 xsTimerAfter(XS_HostInfo* pOwner, uint32 iMillisecond, XS_TimerProc proc, void* pUserData)
{
	return XS_TimerAfter(pOwner, iMillisecond, proc, pUserData);
}

bool xsTimerCancel(uint64 iTimerId)
{
	if ( g_XS_App == NULL || g_XS_App->Engine == NULL || iTimerId == 0 ) {
		return false;
	}
	return xrtNetEngineTimerCancel(g_XS_App->Engine, iTimerId);
}

xvalue* xsSwapTake(XS_HostInfo* pHost)
{
	XS_ScriptRuntime* pRuntime = g_XS_CurrentScript;
	xvalue* pSwap;

	if ( pRuntime == NULL || pRuntime->pHost != pHost || pRuntime->pSwap == NULL ) {
		return NULL;
	}
	pSwap = pRuntime->pSwap;
	pRuntime->pSwap = NULL;
	return pSwap;
}

#endif
