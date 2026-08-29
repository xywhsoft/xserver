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

/* 单 server 驱动启停（启动装配与在线重建共用） */
static bool XS_ServerDriverStart(XS_ServerInfo* pServer, char* sErr, size_t iErrCap)
{
	if ( strcmp(pServer->Class, "custom") == 0 ) {
		pServer->State = XS_RUN_RUNNING;
		printf("[xs] server '%s' custom ready (manual assembly)\n", pServer->Name);
		return true;
	}
	if ( strcmp(pServer->Class, "http") == 0 ) {
		return XS_HttpStart(pServer, sErr, iErrCap);
	}
	if ( strcmp(pServer->Class, "tcp") == 0 ) {
		return XS_TcpStart(pServer, sErr, iErrCap);
	}
	if ( strcmp(pServer->Class, "ws") == 0 ) {
		return XS_WsStart(pServer, sErr, iErrCap);
	}
	if ( strcmp(pServer->Class, "udp") == 0 ) {
		return XS_UdpStart(pServer, sErr, iErrCap);
	}
	snprintf(sErr, iErrCap, "unknown class '%s'", pServer->Class);
	return false;
}

static void XS_ServerDriverStop(XS_ServerInfo* pServer)
{
	if ( pServer->State != XS_RUN_RUNNING ) {
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

/* 等 server 活动连接排空：驱动 Stop 已关全部连接，worker 上在执行的回调
 * 在引擎 After 定时器调度下必然先于本函数完成（After 投递到同一 Worker） */
static bool XS_ServerDrain(XS_App* pApp, uint32 iMaxWaitMs)
{
	xrtSleep(iMaxWaitMs > 200 ? 200 : iMaxWaitMs);
	return true;
}

static bool XS_HostHasScript(XS_HostInfo* pHost)
{
	return pHost->DevFile != NULL && pHost->DevFile[0] != '\0' &&
	       (pHost->DevLang == NULL || strcmp(pHost->DevLang, "c") == 0);
}

static bool XS_ClassNeedsScript(const char* sClass)
{
	return strcmp(sClass, "custom") == 0 || strcmp(sClass, "tcp") == 0 ||
	       strcmp(sClass, "udp") == 0 || strcmp(sClass, "ws") == 0;
}

static bool XS_AssembleServers(XS_App* pApp)
{
	uint32 i;
	XS_ServerInfo* pServer;
	char sErr[256];

	for ( i = 0; i < pApp->ServerCount; i++ ) {
		pServer = pApp->Servers[i];
		sErr[0] = '\0';
		if ( !pServer->Enabled ) {
			printf("[xs] server '%s' disabled, skipped\n", pServer->Name);
			continue;
		}
		/* 脚本先行：custom/tcp/udp 必须有；http 可选（无脚本走纯静态） */
		if ( XS_ClassNeedsScript(pServer->Class) || strcmp(pServer->Class, "http") == 0 ) {
			if ( XS_HostHasScript(pServer->DefaultHost) ) {
				if ( !XS_ScriptLoad(pServer->DefaultHost) ) {
					return false;		/* 失败原因已打印；启动 fail-fast */
				}
			} else if ( XS_ClassNeedsScript(pServer->Class) ) {
				printf("[xs] server '%s' class %s requires devfile\n", pServer->Name, pServer->Class);
				return false;
			}
		}

		if ( XS_ServerDriverStart(pServer, sErr, sizeof(sErr)) ) {
			if ( pServer->DefaultHost != NULL ) {
				pServer->DefaultHost->State = XS_RUN_RUNNING;
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