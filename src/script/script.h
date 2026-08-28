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
	XS_ServiceInitProc		procInit;
	XS_ServiceUnitProc		procUnit;
	XS_ServiceSwapProc		procSwap;
	/* 协议回调（按服务类按需导出） */
	XS_RequestProc			procRequest;
	XS_EventOpenProc		procEventOpen;
	XS_EventDataProc		procEventData;
	XS_EventCloseProc		procEventClose;
	XS_EventDgramProc		procEventDgram;
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

/* 编译并启动一个 host 的脚本；成功后 host->Runtime 就绪且 ServiceInit 已调用 */
static bool XS_ScriptLoad(XS_HostInfo* pHost)
{
	str sDevPath;
	bytes pData;
	size_t iSize = 0;
	char sVirtual[128];
	TCCState* pTcc;
	XS_ScriptRuntime* pRuntime;

	sDevPath = XS_ScriptDevPath(pHost);
	if ( sDevPath == NULL ) {
		return false;		/* 无脚本：由调用方决定语义 */
	}
	pData = xrtFileReadAll(sDevPath, &iSize);
	if ( pData == NULL ) {
		printf("[xs] script read failed: %s\n", sDevPath);
		xrtFree(sDevPath);
		return false;
	}
	snprintf(sVirtual, sizeof(sVirtual), "/xs/script/s%u.c", g_XS_ScriptSeq++);
	if ( !tcc_vfs_mount_memory(sVirtual, pData, iSize) ) {
		printf("[xs] script vfs mount failed\n");
		xrtFree(pData);
		xrtFree(sDevPath);
		return false;
	}
	xrtFree(pData);		/* mount_memory 深拷贝，源缓冲即弃 */

	pTcc = XS_TccCreate();
	if ( pTcc == NULL ) {
		printf("[xs] tcc create failed\n");
		xrtFree(sDevPath);
		return false;
	}
	if ( tcc_add_file(pTcc, sVirtual) < 0 ) {
		printf("[xs] script compile failed: %s\n", sDevPath);
		tcc_delete(pTcc);
		xrtFree(sDevPath);
		return false;
	}
	if ( tcc_relocate(pTcc) < 0 ) {
		printf("[xs] script relocate failed: %s\n", sDevPath);
		tcc_delete(pTcc);
		xrtFree(sDevPath);
		return false;
	}

	pRuntime = (XS_ScriptRuntime*)xrtCalloc(1, sizeof(XS_ScriptRuntime));
	if ( pRuntime == NULL ) {
		tcc_delete(pTcc);
		xrtFree(sDevPath);
		return false;
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
	pHost->Runtime = pRuntime;

	printf("[xs] script loaded: %s (%.1f KB)\n", sDevPath, (double)iSize / 1024.0);
	xrtFree(sDevPath);

	if ( pRuntime->procInit != NULL ) {
		pRuntime->procInit(pHost);
	}
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
