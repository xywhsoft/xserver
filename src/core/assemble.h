#ifndef XS_CORE_ASSEMBLE_H
#define XS_CORE_ASSEMBLE_H

/*
 * xs3 装配：按 class 分流（设计 §6）
 *   custom —— xs 只交出引擎与数据模型，一切由应用手动装配
 *   http/https、tcp/tcps、udp —— xs 建监听/绑定，事件透传脚本回调
 *     http 脚本可选（无 RequestProc 走纯静态）
 *   ws —— 驱动随 WS 工作包接入
 * 停机：驱动收口 → builtin generation 退役 → 引擎排空 → custom 最终退役
 */

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "engine.h"
#include "../script/script.h"
#include "../protocol/http.h"
#include "../protocol/ws.h"
#include "../protocol/stream.h"
#include "../protocol/udp.h"
#include "driver.h"
#include "../runtime/gc.h"
#include "../runtime/topology.h"

/* 阶段一：驱动收口（关监听、关全部连接；custom 无 xs 侧资源） */
static void XS_ServersDrain(XS_App* pApp)
{
	uint32 i;

	XS_ReloadQuiesce();
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		XS_ServerInfo* pServer = pApp->Servers[i];

		if ( pServer == NULL ) continue;
		XS_ServerDriverStop(pServer);
		XS_ServerDriverCloseConnections(pServer);
		/* custom 资源对 xs 不透明：先让 ServiceUnit 主动关闭，TCC 暂不卸载。 */
		if ( strcmp(pServer->Class, "custom") == 0 ) {
			XS_ScriptRequestUnitHost(pServer->DefaultHost);
			XS_ScriptQuiesceHost(pServer->DefaultHost);
		}
	}
	XS_TopologyStopAccepting();
}

static void XS_ShutdownServers(XS_App* pApp)
{
	uint32 i;

	/* builtin：退役撤销管理引用；终态回调归零时自行 Unit/TCC/driver。 */
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		XS_ServerInfo* pServer = pApp->Servers[i];

		if ( pServer == NULL || strcmp(pServer->Class, "custom") == 0 ) continue;
		pServer->State = XS_RUN_STOPPED;
		if ( pServer->DefaultHost != NULL ) pServer->DefaultHost->State = XS_RUN_STOPPED;
		/* 动态配置快照可能在同步 finalizer 中释放 server，先清借用槽位。 */
		if ( pServer->ConfigOwner != NULL ) pApp->Servers[i] = NULL;
		(void)XS_GcRetireServer(pServer, false);
	}
	/* 退役后最后一个脚本活动先触发 ServiceUnit，使脚本可释放其公开 lease；
	 * lease 本身仍无超时等待，归零后才允许引擎进入最终停止。 */
	XS_TopologyWaitLeases();
}

/* Engine 已 Stop/Destroy：custom 的任意回调都已终结，此时才撤销脚本 owner。 */
static void XS_ShutdownAfterEngine(XS_App* pApp)
{
	uint32 i;

	for ( i = 0; i < pApp->ServerCount; i++ ) {
		XS_ServerInfo* pServer = pApp->Servers[i];

		if ( pServer == NULL || strcmp(pServer->Class, "custom") != 0 ) continue;
		pServer->State = XS_RUN_STOPPED;
		if ( pServer->ConfigOwner != NULL ) pApp->Servers[i] = NULL;
		(void)XS_GcRetireServer(pServer, false);
	}
	tcc_vfs_clear_dynamic();
}

#endif
