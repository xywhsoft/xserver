#ifndef XS_CORE_ASSEMBLE_H
#define XS_CORE_ASSEMBLE_H

/*
 * xs3 装配：按 class 分流（设计 §6）
 *   custom —— xs 只交出引擎与数据模型，一切由应用手动装配
 *   http/https、tcp/tcps、udp —— xs 建监听/绑定，事件透传脚本回调
 *     http 脚本可选（无 RequestProc 走纯静态）
 *   ws —— 驱动随 WS 工作包接入
 * 停机四阶段：驱动收口 → ServiceUnit 全部 → 引擎排空 → TCC 销毁
 */

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "engine.h"
#include "../script/script.h"
#include "../protocol/http.h"
#include "../protocol/stream.h"
#include "../protocol/udp.h"

static bool XS_HostHasScript(XS_HostInfo* pHost)
{
	return pHost->DevFile != NULL && pHost->DevFile[0] != '\0' &&
	       (pHost->DevLang == NULL || strcmp(pHost->DevLang, "c") == 0);
}

static bool XS_ClassNeedsScript(const char* sClass)
{
	return strcmp(sClass, "custom") == 0 || strcmp(sClass, "tcp") == 0 ||
	       strcmp(sClass, "udp") == 0;
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

		if ( strcmp(pServer->Class, "custom") == 0 ) {
			pServer->DefaultHost->State = XS_RUN_RUNNING;
			pServer->State = XS_RUN_RUNNING;
			printf("[xs] server '%s' custom ready (manual assembly)\n", pServer->Name);
		} else if ( strcmp(pServer->Class, "http") == 0 ) {
			if ( !XS_HttpStart(pServer, sErr, sizeof(sErr)) ) {
				printf("[xs] %s\n", sErr);
				return false;
			}
			pServer->DefaultHost->State = XS_RUN_RUNNING;
			pServer->State = XS_RUN_RUNNING;
		} else if ( strcmp(pServer->Class, "tcp") == 0 ) {
			if ( !XS_TcpStart(pServer, sErr, sizeof(sErr)) ) {
				printf("[xs] %s\n", sErr);
				return false;
			}
			pServer->DefaultHost->State = XS_RUN_RUNNING;
			pServer->State = XS_RUN_RUNNING;
		} else if ( strcmp(pServer->Class, "udp") == 0 ) {
			if ( !XS_UdpStart(pServer, sErr, sizeof(sErr)) ) {
				printf("[xs] %s\n", sErr);
				return false;
			}
			pServer->DefaultHost->State = XS_RUN_RUNNING;
			pServer->State = XS_RUN_RUNNING;
		} else {
			/* 驱动随协议工作包接入；当前明确跳过，不静默 */
			printf("[xs] server '%s' class %s: driver pending, skipped in this build\n",
				pServer->Name, pServer->Class);
		}
	}
	return true;
}

/* 阶段一：驱动收口（关监听、关全部连接；custom 无 xs 侧资源） */
static void XS_ServersDrain(XS_App* pApp)
{
	uint32 i;
	XS_ServerInfo* pServer;

	for ( i = 0; i < pApp->ServerCount; i++ ) {
		pServer = pApp->Servers[i];
		if ( pServer->State != XS_RUN_RUNNING ) {
			continue;
		}
		pServer->State = XS_RUN_STOPPING;
		if ( strcmp(pServer->Class, "tcp") == 0 ) {
			XS_TcpStop((XS_TcpRuntime*)pServer->Runtime);
		} else if ( strcmp(pServer->Class, "http") == 0 ) {
			XS_HttpStop((XS_HttpRuntime*)pServer->Runtime);
		} else if ( strcmp(pServer->Class, "udp") == 0 ) {
			XS_UdpStop((XS_UdpRuntime*)pServer->Runtime);
		}
	}
}

static void XS_ShutdownServers(XS_App* pApp)
{
	uint32 i;
	XS_ServerInfo* pServer;

	/* 阶段二：ServiceUnit 全部先跑（脚本间仍可互访） */
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		XS_ScriptUnitHost(pApp->Servers[i]->DefaultHost);
	}
	/* 阶段三：等引擎 LiveObjects 排空（worker 不再执行脚本代码）后销毁 TCC */
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		XS_ScriptDeleteHost(pApp->Servers[i]->DefaultHost);
		pApp->Servers[i]->State = XS_RUN_STOPPED;
		if ( pApp->Servers[i]->DefaultHost != NULL ) {
			pApp->Servers[i]->DefaultHost->State = XS_RUN_STOPPED;
		}
	}
	/* 阶段四：驱动运行时释放（连接已排空） */
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		pServer = pApp->Servers[i];
		if ( strcmp(pServer->Class, "tcp") == 0 ) {
			XS_TcpUnit((XS_TcpRuntime*)pServer->Runtime);
		} else if ( strcmp(pServer->Class, "http") == 0 ) {
			XS_HttpUnit((XS_HttpRuntime*)pServer->Runtime);
		} else if ( strcmp(pServer->Class, "udp") == 0 ) {
			XS_UdpUnit((XS_UdpRuntime*)pServer->Runtime);
		}
		pServer->Runtime = NULL;
	}
	/* 清空全部动态 VFS 挂载（脚本源）；内置资源不受影响 */
	tcc_vfs_clear_dynamic();
}

#endif
