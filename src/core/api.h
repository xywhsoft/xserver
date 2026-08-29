#ifndef XS_CORE_API_H
#define XS_CORE_API_H

/*
 * xs3 契约 API 实现（xsbase.h 声明的 13 个函数）
 * 本轮交付：配置访问（枚举/查找/根 Custom）+ xsAppPath + xsCreateTCC/xsDestroyTCC
 * reload / timer 属于"运行时与重载"工作包，当前为占位实现
 */

#include "../sdk/xsbase.h"
#include "config.h"
#include "engine.h"
#include "../script/tcc_host.h"
#include "../runtime/reload.h"

static XS_App* g_XS_App = NULL;

XS_ServerInfo* xsServerFind(const char* sName)
{
	uint32 i;

	if ( g_XS_App == NULL || sName == NULL ) {
		return NULL;
	}
	for ( i = 0; i < g_XS_App->ServerCount; i++ ) {
		if ( strcmp(g_XS_App->Servers[i]->Name, sName) == 0 ) {
			return g_XS_App->Servers[i];
		}
	}
	return NULL;
}

void xsEnumServers(XS_ServerEnumProc procEach, void* pUserData)
{
	uint32 i;

	if ( g_XS_App == NULL || procEach == NULL ) {
		return;
	}
	for ( i = 0; i < g_XS_App->ServerCount; i++ ) {
		if ( !procEach(g_XS_App->Servers[i], pUserData) ) {
			return;
		}
	}
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
	return g_XS_App != NULL ? g_XS_App->Root : NULL;
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
