#ifndef XS_SCRIPT_SCRIPT_H
#define XS_SCRIPT_SCRIPT_H

/*
 * xs3 脚本运行时：devfile 读取（xrt fs，UTF-8）→ VFS 内存挂载 → TCC 编译
 * → 符号解析 → ServiceInit。卸载时 ServiceUnit → tcc_delete。
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"
#include "tcc_host.h"

typedef struct XS_ScriptRuntime {
	TCCState*			pTcc;
	uint64			tGeneration;	/* 代际（热重载语义见设计 §10） */
	xvalue*			pSwap;		/* 旧代 ServiceSwap 导出、新代 xsSwapTake 取走 */
	XS_HostInfo*		pHost;
	bool			bRetired;	/* 已退役：无引用后 ServiceUnit + 销毁 */
	struct XS_ScriptRuntime*	pRetiredNext;
	XS_ServiceInitProc		procInit;
	XS_ServiceUnitProc		procUnit;
	XS_ServiceSwapProc		procSwap;
	/* 协议回调（按服务类按需导出） */
	XS_RequestProc			procRequest;
	XS_EventOpenProc		procEventOpen;
	XS_EventDataProc		procEventData;
	XS_EventCloseProc		procEventClose;
	XS_EventDgramProc		procEventDgram;
	XS_WsOpenProc			procWsOpen;
	XS_WsTextProc			procWsText;
	XS_WsBinaryProc			procWsBinary;
	XS_WsPingProc			procWsPing;
	XS_WsPongProc			procWsPong;
	XS_WsCloseProc			procWsClose;
} XS_ScriptRuntime;

static uint32 g_XS_ScriptSeq = 0;

/* devfile 相对 appPath 解析为绝对路径（UTF-8） */
static str XS_ScriptDevPath(XS_HostInfo* pHost)
{
	if ( pHost->DevFile == NULL ) {
		return NULL;
	}
	if ( xrtPathIsAbs(pHost->DevFile) ) {
		return xrtStrDup(pHost->DevFile);
	}
	return xrtPathJoin(XS_AppPath(), pHost->DevFile);
}

static uint64 g_XS_Generation = 0;

/* 编译（不挂载不初始化）；失败返回 NULL。热重载先编译、成功才换代 = 回滚语义 */
static XS_ScriptRuntime* XS_ScriptCompile(XS_HostInfo* pHost)
{
	str sDevPath;
	bytes pData;
	size_t iSize = 0;
	char sVirtual[128];
	TCCState* pTcc;
	XS_ScriptRuntime* pRuntime;

	sDevPath = XS_ScriptDevPath(pHost);
	if ( sDevPath == NULL ) {
		return NULL;		/* 无脚本：由调用方决定语义 */
	}
	pData = xrtFileReadAll(sDevPath, &iSize);
	if ( pData == NULL ) {
		printf("[xs] script read failed: %s\n", sDevPath);
		xrtFree(sDevPath);
		return NULL;
	}
	snprintf(sVirtual, sizeof(sVirtual), "/xs/script/s%u.c", g_XS_ScriptSeq++);
	if ( !tcc_vfs_mount_memory(sVirtual, pData, iSize) ) {
		printf("[xs] script vfs mount failed\n");
		xrtFree(pData);
		xrtFree(sDevPath);
		return NULL;
	}
	xrtFree(pData);		/* mount_memory 深拷贝，源缓冲即弃 */

	pTcc = XS_TccCreateForHost(pHost);	/* 基础环境 + host 的 dev_inc/dev_lib */
	if ( pTcc == NULL ) {
		printf("[xs] tcc create failed\n");
		xrtFree(sDevPath);
		return NULL;
	}
	if ( tcc_add_file(pTcc, sVirtual) < 0 ) {
		printf("[xs] script compile failed: %s\n", sDevPath);
		tcc_delete(pTcc);
		xrtFree(sDevPath);
		return NULL;
	}
	if ( tcc_relocate(pTcc) < 0 ) {
		printf("[xs] script relocate failed: %s\n", sDevPath);
		tcc_delete(pTcc);
		xrtFree(sDevPath);
		return NULL;
	}

	pRuntime = (XS_ScriptRuntime*)xrtCalloc(1, sizeof(XS_ScriptRuntime));
	if ( pRuntime == NULL ) {
		tcc_delete(pTcc);
		xrtFree(sDevPath);
		return NULL;
	}
	pRuntime->pTcc = pTcc;
	pRuntime->procInit = (XS_ServiceInitProc)tcc_get_symbol(pTcc, XS_SYM_SERVICE_INIT);
	pRuntime->procUnit = (XS_ServiceUnitProc)tcc_get_symbol(pTcc, XS_SYM_SERVICE_UNIT);
	pRuntime->procSwap = (XS_ServiceSwapProc)tcc_get_symbol(pTcc, XS_SYM_SERVICE_SWAP);
	pRuntime->procRequest = (XS_RequestProc)tcc_get_symbol(pTcc, XS_SYM_REQUEST_PROC);
	pRuntime->procEventOpen = (XS_EventOpenProc)tcc_get_symbol(pTcc, XS_SYM_EVENT_OPEN);
	pRuntime->procEventData = (XS_EventDataProc)tcc_get_symbol(pTcc, XS_SYM_EVENT_DATA);
	pRuntime->procEventClose = (XS_EventCloseProc)tcc_get_symbol(pTcc, XS_SYM_EVENT_CLOSE);
	pRuntime->procEventDgram = (XS_EventDgramProc)tcc_get_symbol(pTcc, XS_SYM_EVENT_DGRAM);
	pRuntime->procWsOpen = (XS_WsOpenProc)tcc_get_symbol(pTcc, XS_SYM_WS_OPEN);
	pRuntime->procWsText = (XS_WsTextProc)tcc_get_symbol(pTcc, XS_SYM_WS_TEXT);
	pRuntime->procWsBinary = (XS_WsBinaryProc)tcc_get_symbol(pTcc, XS_SYM_WS_BINARY);
	pRuntime->procWsPing = (XS_WsPingProc)tcc_get_symbol(pTcc, XS_SYM_WS_PING);
	pRuntime->procWsPong = (XS_WsPongProc)tcc_get_symbol(pTcc, XS_SYM_WS_PONG);
	pRuntime->procWsClose = (XS_WsCloseProc)tcc_get_symbol(pTcc, XS_SYM_WS_CLOSE);
	pRuntime->pHost = pHost;
	pRuntime->tGeneration = ++g_XS_Generation;

	printf("[xs] script loaded: %s (%.1f KB gen %llu)\n", sDevPath, (double)iSize / 1024.0,
		(unsigned long long)pRuntime->tGeneration);
	xrtFree(sDevPath);
	return pRuntime;
}

/* 挂载并初始化（ServiceInit 内可 xsSwapTake 取回交接数据） */
static void XS_ScriptAttach(XS_HostInfo* pHost, XS_ScriptRuntime* pRuntime)
{
	pHost->Runtime = pRuntime;
	if ( pRuntime != NULL && pRuntime->procInit != NULL ) {
		pRuntime->procInit(pHost);
	}
}

/* 编译并启动（首次装配路径） */
static bool XS_ScriptLoad(XS_HostInfo* pHost)
{
	XS_ScriptRuntime* pRuntime = XS_ScriptCompile(pHost);

	if ( pRuntime == NULL ) {
		return false;
	}
	XS_ScriptAttach(pHost, pRuntime);
	return true;
}

/* 卸载阶段一：ServiceUnit（脚本仍可使用全部宿主能力，含关闭自己的监听器） */
static void XS_ScriptUnitHost(XS_HostInfo* pHost)
{
	XS_ScriptRuntime* pRuntime = (XS_ScriptRuntime*)pHost->Runtime;

	if ( pRuntime != NULL && pRuntime->procUnit != NULL ) {
		pRuntime->procUnit(pHost);
	}
}

/* 卸载阶段二：TCC 状态销毁。
 * 必须等引擎 LiveObjects 排空（监听器/流的异步 Close 完成、worker 线程
 * 不再执行脚本代码）之后才能调用，否则释放中的代码页会在 worker 里炸。
 * 正式的按代 drain 判据随连接注册表落地（设计 §10.3），此处为停机同步排空。 */
static void XS_ScriptDeleteHost(XS_HostInfo* pHost)
{
	XS_ScriptRuntime* pRuntime = (XS_ScriptRuntime*)pHost->Runtime;

	if ( pRuntime == NULL ) {
		return;
	}
	if ( pHost->Server != NULL && pHost->Server->Engine != NULL ) {
		xnetenginestats tStats;
		int iWait;

		for ( iWait = 0; iWait < 50; iWait++ ) {
			memset(&tStats, 0, sizeof(tStats));
			if ( !xrtNetEngineStats(pHost->Server->Engine, &tStats) || tStats.LiveObjects == 0 ) {
				break;
			}
			xrtSleep(100);
		}
	}
	tcc_delete(pRuntime->pTcc);
	xrtFree(pRuntime);
	pHost->Runtime = NULL;
}

#endif
