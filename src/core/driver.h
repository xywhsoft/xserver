#ifndef XS_CORE_DRIVER_H
#define XS_CORE_DRIVER_H

/*
 * 单 server 驱动启停抽象：启动装配、停机收口、四阶段停机与在线重建共用。
 * 依赖各协议驱动的 Start/Stop/Unit（protocol 头文件）。
 */

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "engine.h"
#include "../protocol/http.h"
#include "../protocol/stream.h"
#include "../protocol/ws.h"
#include "../protocol/udp.h"

static void XS_ServerDriverUnit(XS_ServerInfo* pServer);

/* 单 server 驱动启停（启动装配与在线重建共用） */
static bool XS_ServerDriverStartEx(
	XS_ServerInfo* pServer,
	bool bStartEndpoint,
	bool bAcceptEndpoint,
	char* sErr,
	size_t iErrCap)
{
	if ( pServer->Generation == NULL && XS_GenerationCreate(pServer) == NULL ) {
		snprintf(sErr, iErrCap, "server '%s' generation create failed", pServer->Name);
		return false;
	}
	if ( strcmp(pServer->Class, "custom") == 0 ) {
		pServer->State = XS_RUN_RUNNING;
		printf("[xs] server '%s' custom ready (manual assembly)\n", pServer->Name);
		return true;
	}
	if ( strcmp(pServer->Class, "http") == 0 ) {
		if ( XS_HttpStartEx(pServer, bStartEndpoint, bAcceptEndpoint, sErr, iErrCap) ) return true;
		XS_ServerDriverUnit(pServer);
		return false;
	}
	if ( strcmp(pServer->Class, "tcp") == 0 ) {
		if ( XS_TcpStartEx(pServer, bStartEndpoint, bAcceptEndpoint, sErr, iErrCap) ) return true;
		XS_ServerDriverUnit(pServer);
		return false;
	}
	if ( strcmp(pServer->Class, "ws") == 0 ) {
		if ( XS_WsStartEx(pServer, bStartEndpoint, bAcceptEndpoint, sErr, iErrCap) ) return true;
		XS_ServerDriverUnit(pServer);
		return false;
	}
	if ( strcmp(pServer->Class, "udp") == 0 ) {
		if ( XS_UdpStartEx(pServer, bStartEndpoint, bAcceptEndpoint, sErr, iErrCap) ) return true;
		XS_ServerDriverUnit(pServer);
		return false;
	}
	snprintf(sErr, iErrCap, "unknown class '%s'", pServer->Class);
	return false;
}

static bool XS_ServerDriverStart(XS_ServerInfo* pServer, char* sErr, size_t iErrCap)
{
	return XS_ServerDriverStartEx(pServer, true, true, sErr, iErrCap);
}

static XS_ListenerSlot* XS_ServerDriverListenerSlot(
	XS_ServerInfo* pServer,
	void** ppExpectedRuntime)
{
	if ( ppExpectedRuntime != NULL ) *ppExpectedRuntime = NULL;
	if ( pServer == NULL || pServer->Runtime == NULL || ppExpectedRuntime == NULL ) return NULL;
	if ( strcmp(pServer->Class, "http") == 0 ) {
		XS_HttpRuntime* pRuntime = (XS_HttpRuntime*)pServer->Runtime;
		*ppExpectedRuntime = pRuntime;
		return pRuntime->pListenerSlot;
	}
	if ( strcmp(pServer->Class, "tcp") == 0 ) {
		XS_TcpRuntime* pRuntime = (XS_TcpRuntime*)pServer->Runtime;
		*ppExpectedRuntime = pRuntime;
		return pRuntime->pListenerSlot;
	}
	if ( strcmp(pServer->Class, "ws") == 0 ) {
		XS_WsRuntime* pRuntime = (XS_WsRuntime*)pServer->Runtime;
		*ppExpectedRuntime = pRuntime;
		return pRuntime->pListenerSlot;
	}
	if ( strcmp(pServer->Class, "udp") == 0 ) {
		XS_UdpRuntime* pRuntime = (XS_UdpRuntime*)pServer->Runtime;
		*ppExpectedRuntime = pRuntime;
		return pRuntime->pListenerSlot;
	}
	return NULL;
}

static bool XS_ServerDriverHandoff(XS_ServerInfo* pOld, XS_ServerInfo* pNew)
{
	if ( pOld == NULL || pNew == NULL || strcmp(pOld->Class, pNew->Class) != 0 ) return false;
	if ( strcmp(pOld->Class, "http") == 0 ) {
		return XS_HttpHandoff((XS_HttpRuntime*)pOld->Runtime, (XS_HttpRuntime*)pNew->Runtime);
	}
	if ( strcmp(pOld->Class, "tcp") == 0 ) {
		return XS_TcpHandoff((XS_TcpRuntime*)pOld->Runtime, (XS_TcpRuntime*)pNew->Runtime);
	}
	if ( strcmp(pOld->Class, "ws") == 0 ) {
		return XS_WsHandoff((XS_WsRuntime*)pOld->Runtime, (XS_WsRuntime*)pNew->Runtime);
	}
	if ( strcmp(pOld->Class, "udp") == 0 ) {
		return XS_UdpHandoff((XS_UdpRuntime*)pOld->Runtime, (XS_UdpRuntime*)pNew->Runtime);
	}
	return false;
}

static bool XS_ServerDriverCanHandoff(XS_ServerInfo* pOld, XS_ServerInfo* pNew)
{
	XS_ListenerSlot* pSlot;
	void* pExpected = NULL;

	if ( pOld == NULL || pNew == NULL || pOld->Runtime == NULL || pNew->Runtime == NULL ||
	     strcmp(pOld->Class, pNew->Class) != 0 ) return false;
	pSlot = XS_ServerDriverListenerSlot(pOld, &pExpected);
	return XS_ListenerSlotCanHandoff(pSlot, pExpected);
}

/* 候选新端点已经 bind，但在 topology 事务提交前拒绝所有连接。 */
static bool XS_ServerDriverCanActivate(XS_ServerInfo* pServer)
{
	XS_ListenerSlot* pSlot;
	void* pExpected = NULL;

	if ( pServer == NULL ) return false;
	if ( !pServer->Enabled ) return true;
	pSlot = XS_ServerDriverListenerSlot(pServer, &pExpected);
	return XS_ListenerSlotCanActivate(pSlot, pExpected);
}

static bool XS_ServerDriverActivate(XS_ServerInfo* pServer)
{
	XS_ListenerSlot* pSlot;
	void* pExpected = NULL;

	if ( pServer == NULL ) return false;
	if ( !pServer->Enabled ) return true;
	pSlot = XS_ServerDriverListenerSlot(pServer, &pExpected);
	return XS_ListenerSlotActivate(pSlot, pExpected);
}

static void XS_ServerDriverDeactivate(XS_ServerInfo* pServer)
{
	XS_ListenerSlot* pSlot;
	void* pExpected = NULL;

	if ( pServer == NULL || !pServer->Enabled ) return;
	pSlot = XS_ServerDriverListenerSlot(pServer, &pExpected);
	XS_ListenerSlotDeactivate(pSlot, pExpected);
}

static void XS_ServerDriverStop(XS_ServerInfo* pServer)
{
	if ( pServer->State != XS_RUN_RUNNING && pServer->State != XS_RUN_RELOAD_FAILED ) {
		return;
	}
	pServer->State = XS_RUN_STOPPING;
	if ( strcmp(pServer->Class, "tcp") == 0 ) {
		XS_TcpStop((XS_TcpRuntime*)pServer->Runtime);
	} else if ( strcmp(pServer->Class, "http") == 0 ) {
		XS_HttpStop((XS_HttpRuntime*)pServer->Runtime);
	} else if ( strcmp(pServer->Class, "ws") == 0 ) {
		XS_WsStop((XS_WsRuntime*)pServer->Runtime);
	} else if ( strcmp(pServer->Class, "udp") == 0 ) {
		XS_UdpStop((XS_UdpRuntime*)pServer->Runtime);
	}
	pServer->State = XS_RUN_STOPPED;
}

/* 进程退出才主动关闭旧连接；在线换代只 Stop listener 后自然排空。 */
static void XS_ServerDriverCloseConnections(XS_ServerInfo* pServer)
{
	if ( pServer == NULL || pServer->Runtime == NULL ) return;
	if ( strcmp(pServer->Class, "tcp") == 0 ) {
		XS_TcpCloseConnections((XS_TcpRuntime*)pServer->Runtime);
	} else if ( strcmp(pServer->Class, "http") == 0 ) {
		XS_HttpCloseConnections((XS_HttpRuntime*)pServer->Runtime);
	} else if ( strcmp(pServer->Class, "ws") == 0 ) {
		XS_WsCloseConnections((XS_WsRuntime*)pServer->Runtime);
	}
}

static void XS_ServerDriverUnit(XS_ServerInfo* pServer)
{
	if ( strcmp(pServer->Class, "tcp") == 0 ) {
		XS_TcpUnit((XS_TcpRuntime*)pServer->Runtime);
	} else if ( strcmp(pServer->Class, "http") == 0 ) {
		XS_HttpUnit((XS_HttpRuntime*)pServer->Runtime);
	} else if ( strcmp(pServer->Class, "ws") == 0 ) {
		XS_WsUnit((XS_WsRuntime*)pServer->Runtime);
	} else if ( strcmp(pServer->Class, "udp") == 0 ) {
		XS_UdpUnit((XS_UdpRuntime*)pServer->Runtime);
	}
	pServer->Runtime = NULL;
}

static bool XS_HostHasScript(XS_HostInfo* pHost)
{
	return pHost->DevFile != NULL && pHost->DevFile[0] != '\0' &&
	       (pHost->DevLang == NULL || strcmp(pHost->DevLang, "c") == 0);
}

static bool XS_ServerScriptsLoad(XS_ServerInfo* pServer)
{
	bool bVirtualHosts = strcmp(pServer->Class, "http") == 0 ||
		strcmp(pServer->Class, "ws") == 0;
	uint32 iCount = bVirtualHosts ? pServer->HostCount + 1 : 1;
	uint32 i;

	if ( bVirtualHosts ) {
		XS_VHostTable tCheck;
		char sErr[256];

		if ( !XS_VHostTableBuild(pServer, &tCheck, sErr, sizeof(sErr)) ) {
			printf("[xs] server '%s' virtual host config invalid: %s\n",
				pServer->Name, sErr);
			return false;
		}
		XS_VHostTableUnit(&tCheck);
	}

	for ( i = 0; i < iCount; i++ ) {
		XS_HostInfo* pHost = i == 0 ? pServer->DefaultHost : pServer->Hosts[i - 1];

		if ( pHost == NULL || !pHost->Enabled ) continue;
		if ( XS_HostHasScript(pHost) ) {
			if ( XS_ScriptLoad(pHost) ) continue;
		} else if ( strcmp(pServer->Class, "http") == 0 ) {
			continue;	/* HTTP host 可以是纯静态站点 */
		} else {
			printf("[xs] server '%s' host '%s' class %s requires devfile\n",
				pServer->Name, pHost->Name != NULL ? pHost->Name : "?",
				pServer->Class);
		}
		/* 失败时撤销本 server 已经挂载的脚本，不留半装配代。 */
		while ( i > 0 ) {
			XS_HostInfo* pLoaded;

			i--;
			pLoaded = i == 0 ? pServer->DefaultHost : pServer->Hosts[i - 1];
			if ( pLoaded != NULL ) XS_ScriptUnitHost(pLoaded);
		}
		return false;
	}
	return true;
}

static bool XS_AssembleServers(XS_App* pApp)
{
	uint32 i;
	XS_ServerInfo* pServer;
	char sErr[256];

	/* 所有拓扑对象先建立 generation，ServiceInit 中跨 server 查找才能取得 lease。 */
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		pServer = pApp->Servers[i];
		if ( pServer->Generation == NULL && XS_GenerationCreate(pServer) == NULL ) {
			printf("[xs] server '%s' generation create failed\n", pServer->Name);
			return false;
		}
	}
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		pServer = pApp->Servers[i];
		sErr[0] = '\0';
		if ( !pServer->Enabled ) {
			pServer->State = XS_RUN_STOPPED;
			pServer->DefaultHost->State = XS_RUN_STOPPED;
			printf("[xs] server '%s' disabled, skipped\n", pServer->Name);
			continue;
		}
		/* 脚本先行：HTTP/WS 编译每个启用 host；HTTP 可纯静态，WS 必须有脚本。 */
		if ( !XS_ServerScriptsLoad(pServer) ) return false;

		if ( XS_ServerDriverStart(pServer, sErr, sizeof(sErr)) ) {
			pServer->DefaultHost->State = pServer->DefaultHost->Enabled ?
				XS_RUN_RUNNING : XS_RUN_STOPPED;
			for ( uint32 j = 0; j < pServer->HostCount; j++ ) {
				pServer->Hosts[j]->State = pServer->Hosts[j]->Enabled ?
					XS_RUN_RUNNING : XS_RUN_STOPPED;
			}
			pServer->State = XS_RUN_RUNNING;
		} else {
			printf("[xs] %s\n", sErr);
			return false;
		}
	}
	return true;
}

#endif
