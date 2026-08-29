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
#include "../protocol/ws.h"
#include "../protocol/stream.h"
#include "../protocol/udp.h"
#include "driver.h"

/* 阶段一：驱动收口（关监听、关全部连接；custom 无 xs 侧资源） */
static void XS_ServersDrain(XS_App* pApp)
{
	uint32 i;

	for ( i = 0; i < pApp->ServerCount; i++ ) {
		XS_ServerDriverStop(pApp->Servers[i]);
	}
}

static void XS_ShutdownServers(XS_App* pApp)
{
	uint32 i;

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
		XS_ServerDriverUnit(pApp->Servers[i]);
	}
	/* 清空全部动态 VFS 挂载（脚本源）；内置资源不受影响 */
	tcc_vfs_clear_dynamic();
}

#endif
