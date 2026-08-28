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

/* —— 重载与定时器（src/runtime/reload.h）—— */

bool xsReloadHost(XS_HostInfo* pHost)
{
	return XS_ReloadHostNow(pHost);
}

bool xsReloadServer(const char* sName)
{
	XS_ServerInfo* pServer = xsServerFind(sName);
	uint32 i;
	bool bOk = true;

	if ( pServer == NULL ) {
		return false;
	}
	if ( pServer->DefaultHost->Runtime != NULL ) {
		bOk = XS_ReloadHostNow(pServer->DefaultHost) && bOk;
	}
	for ( i = 0; i < pServer->HostCount; i++ ) {
		if ( pServer->Hosts[i]->Runtime != NULL ) {
			bOk = XS_ReloadHostNow(pServer->Hosts[i]) && bOk;
		}
	}
	return bOk;
}

bool xsReloadAll(void)
{
	uint32 i;
	bool bOk = true;

	if ( g_XS_App == NULL ) {
		return false;
	}
	for ( i = 0; i < g_XS_App->ServerCount; i++ ) {
		XS_ServerInfo* pServer = g_XS_App->Servers[i];

		if ( pServer->DefaultHost->Runtime != NULL ) {
			bOk = XS_ReloadHostNow(pServer->DefaultHost) && bOk;
		}
	}
	return bOk;
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
	XS_ScriptRuntime* pRuntime = pHost != NULL ? (XS_ScriptRuntime*)pHost->Runtime : NULL;
	xvalue* pSwap;

	if ( pRuntime == NULL || pRuntime->pSwap == NULL ) {
		return NULL;
	}
	pSwap = pRuntime->pSwap;
	pRuntime->pSwap = NULL;
	return pSwap;
}

#endif
