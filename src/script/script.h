#ifndef XS_SCRIPT_SCRIPT_H
#define XS_SCRIPT_SCRIPT_H

/*
 * xs3 脚本运行时：devfile 读取（xrt fs，UTF-8）→ VFS 内存挂载 → TCC 编译
 * → 符号解析 → ServiceInit。卸载时 ServiceUnit → tcc_delete。
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"
#include "../runtime/generation.h"
#include "tcc_host.h"

typedef struct XS_ScriptRuntime {
	TCCState*			pTcc;
	uint64			tGeneration;	/* 代际（热重载语义见设计 §10） */
	xvalue*			pSwap;		/* 旧代 ServiceSwap 导出、新代 xsSwapTake 取走 */
	XS_HostInfo*		pHost;
	volatile int32		iReferences;	/* host 所有权 1 + 连接/定时器引用 */
	xatomic32		tRetired;	/* 退役后拒绝新增脚本资源 */
	xatomic32		tReady;		/* ServiceInit 完整返回后才允许异步回调 */
	xatomic32		tUnitCalled;	/* ServiceUnit 全生命周期至多一次 */
	xatomic32		tOwnerReleased;	/* host owner 引用至多撤销一次 */
	xmutex*			pInitLock;	/* 阻挡 ServiceInit 中创建后抢先触发的 timer */
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

/* 让脚本内创建的定时器绑定“正在执行的旧代”，而不是可变的 host->Runtime。 */
static XRT_THREAD_LOCAL XS_ScriptRuntime* g_XS_CurrentScript = NULL;

static XS_ScriptRuntime* XS_ScriptEnter(XS_ScriptRuntime* pRuntime)
{
	XS_ScriptRuntime* pPrevious = g_XS_CurrentScript;

	g_XS_CurrentScript = pRuntime;
	return pPrevious;
}

static void XS_ScriptLeave(XS_ScriptRuntime* pPrevious)
{
	g_XS_CurrentScript = pPrevious;
}

static bool XS_ScriptIsRetired(const XS_ScriptRuntime* pRuntime)
{
	return pRuntime == NULL ||
	       xrtAtomic32Load(&pRuntime->tRetired, XMEMORY_ACQUIRE) != 0;
}

static bool XS_ScriptRetain(XS_ScriptRuntime* pRuntime)
{
	return pRuntime != NULL && !XS_ScriptIsRetired(pRuntime) &&
	       xrtRefRetain(&pRuntime->iReferences) > 0;
}

static bool XS_ScriptWaitReady(XS_ScriptRuntime* pRuntime)
{
	bool bReady;

	if ( pRuntime == NULL || pRuntime->pInitLock == NULL ) return false;
	xrtMutexLock(pRuntime->pInitLock);
	bReady = xrtAtomic32Load(&pRuntime->tReady, XMEMORY_ACQUIRE) != 0;
	xrtMutexUnlock(pRuntime->pInitLock);
	return bReady;
}

static void XS_ScriptCallUnit(XS_ScriptRuntime* pRuntime)
{
	if ( pRuntime != NULL &&
	     xrtAtomic32Exchange(&pRuntime->tUnitCalled, 1, XMEMORY_ACQ_REL) == 0 &&
	     pRuntime->procUnit != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pRuntime);

		pRuntime->procUnit(pRuntime->pHost);
		XS_ScriptLeave(pPrevious);
	}
}

static XS_ScriptRuntime* XS_ScriptAcquireHost(XS_HostInfo* pHost)
{
	XS_ScriptRuntime* pRuntime = NULL;

	if ( pHost == NULL || pHost->RuntimeLock == NULL ) {
		return NULL;
	}
	xrtMutexLock((xmutex*)pHost->RuntimeLock);
	pRuntime = (XS_ScriptRuntime*)pHost->Runtime;
	if ( !XS_ScriptRetain(pRuntime) ) {
		pRuntime = NULL;
	}
	xrtMutexUnlock((xmutex*)pHost->RuntimeLock);
	return pRuntime;
}

static void XS_ScriptRelease(XS_ScriptRuntime* pRuntime)
{
	if ( pRuntime != NULL && xrtRefRelease(&pRuntime->iReferences) == 0 ) {
		XS_ScriptCallUnit(pRuntime);
		if ( pRuntime->pSwap != NULL ) {
			xrtValueRelease(pRuntime->pSwap);
		}
		tcc_delete(pRuntime->pTcc);
		if ( pRuntime->pInitLock != NULL ) xrtMutexDestroy(pRuntime->pInitLock);
		xrtFree(pRuntime);
	}
}

static void XS_ScriptBeginRetire(XS_ScriptRuntime* pRuntime)
{
	if ( pRuntime != NULL &&
	     xrtAtomic32Exchange(&pRuntime->tRetired, 1, XMEMORY_ACQ_REL) == 0 ) {
		XS_ServerGeneration* pGeneration =
			pRuntime->pHost != NULL && pRuntime->pHost->Server != NULL ?
			(XS_ServerGeneration*)pRuntime->pHost->Server->Generation : NULL;

		XS_GenerationTimerCancelOwner(pGeneration, pRuntime);
	}
}

static void XS_ScriptRetire(XS_ScriptRuntime* pRuntime)
{
	if ( pRuntime == NULL ) return;
	XS_ScriptBeginRetire(pRuntime);
	if ( xrtAtomic32Exchange(&pRuntime->tOwnerReleased, 1, XMEMORY_ACQ_REL) == 0 ) {
		XS_ScriptRelease(pRuntime);	/* 撤销 host 所有权 */
	}
}

/* 每个 host 固定一个 VFS 槽位路径：重载时同路径重挂（mount_memory 为替换语义），
 * 源挂载不再随代数累积——6 小时演练观察项的修复。
 * 注：host 结构体跨代复用（配置不变时），以 Server 名 + host 指针为键 */
typedef struct XS_ScriptSlot {
	const void*		pKey;
	char			sPath[96];
} XS_ScriptSlot;

static XS_ScriptSlot g_XS_ScriptSlots[96];
static uint32 g_XS_ScriptSlotCount = 0;

static const char* XS_ScriptSlotPath(XS_HostInfo* pHost)
{
	uint32 i;

	for ( i = 0; i < g_XS_ScriptSlotCount; i++ ) {
		if ( g_XS_ScriptSlots[i].pKey == (const void*)pHost ) {
			return g_XS_ScriptSlots[i].sPath;
		}
	}
	if ( g_XS_ScriptSlotCount >= 96 ) {
		return NULL;		/* 槽位耗尽：退回 clear_dynamic 由调用方处理 */
	}
	snprintf(g_XS_ScriptSlots[g_XS_ScriptSlotCount].sPath, 96,
		"/xs/script/h%u.c", g_XS_ScriptSlotCount);
	g_XS_ScriptSlots[g_XS_ScriptSlotCount].pKey = (const void*)pHost;
	return g_XS_ScriptSlots[g_XS_ScriptSlotCount++].sPath;
}

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
	{
		const char* sSlot = XS_ScriptSlotPath(pHost);

		if ( sSlot == NULL ) {
			tcc_vfs_clear_dynamic();
			sSlot = XS_ScriptSlotPath(pHost);	/* 清空后槽位表失效，重取 */
			if ( sSlot == NULL ) {
				printf("[xs] script slot exhausted\n");
				xrtFree(pData);
				xrtFree(sDevPath);
				return NULL;
			}
		}
		snprintf(sVirtual, sizeof(sVirtual), "%s", sSlot);
	}
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
	pRuntime->iReferences = 1;
	pRuntime->pInitLock = xrtMutexCreate();
	if ( pRuntime->pInitLock == NULL ) {
		tcc_delete(pTcc);
		xrtFree(pRuntime);
		xrtFree(sDevPath);
		return NULL;
	}
	xrtAtomic32Init(&pRuntime->tRetired, 0);
	xrtAtomic32Init(&pRuntime->tReady, 0);
	xrtAtomic32Init(&pRuntime->tUnitCalled, 0);
	xrtAtomic32Init(&pRuntime->tOwnerReleased, 0);

	printf("[xs] script loaded: %s (%.1f KB gen %llu)\n", sDevPath, (double)iSize / 1024.0,
		(unsigned long long)pRuntime->tGeneration);
	xrtFree(sDevPath);
	return pRuntime;
}

/* 挂载并初始化（ServiceInit 内可 xsSwapTake 取回交接数据） */
static void XS_ScriptAttach(XS_HostInfo* pHost, XS_ScriptRuntime* pRuntime)
{
	XS_ScriptRuntime* pPrevious = NULL;

	/* 候选代先完整初始化；旧槽位在此期间继续为新 Accept 提供旧脚本。
	 * Runtime 发布和 ready 标记在锁序内完成：Accept 只有在 ready 已发布后才能
	 * 取得新脚本，Init 创建的 timer 则要等 init 锁释放后才能运行。 */
	if ( pRuntime != NULL ) {
		xrtMutexLock(pRuntime->pInitLock);
		if ( pRuntime->procInit != NULL ) {
			XS_ScriptRuntime* pContext = XS_ScriptEnter(pRuntime);

			pRuntime->procInit(pHost);
			XS_ScriptLeave(pContext);
		}
		xrtMutexLock((xmutex*)pHost->RuntimeLock);
		pPrevious = (XS_ScriptRuntime*)pHost->Runtime;
		pHost->Runtime = pRuntime;
		xrtAtomic32Store(&pRuntime->tReady, 1, XMEMORY_RELEASE);
		xrtMutexUnlock((xmutex*)pHost->RuntimeLock);
		xrtMutexUnlock(pRuntime->pInitLock);
	} else {
		xrtMutexLock((xmutex*)pHost->RuntimeLock);
		pPrevious = (XS_ScriptRuntime*)pHost->Runtime;
		pHost->Runtime = NULL;
		xrtMutexUnlock((xmutex*)pHost->RuntimeLock);
	}
	if ( pPrevious != NULL && pPrevious != pRuntime ) {
		XS_ScriptRetire(pPrevious);
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
	XS_ScriptRuntime* pRuntime;

	if ( pHost == NULL || pHost->RuntimeLock == NULL ) {
		return;
	}
	xrtMutexLock((xmutex*)pHost->RuntimeLock);
	pRuntime = (XS_ScriptRuntime*)pHost->Runtime;
	pHost->Runtime = NULL;
	xrtMutexUnlock((xmutex*)pHost->RuntimeLock);
	XS_ScriptRetire(pRuntime);
}

/* custom 停机阶段：先通知脚本关闭自持资源，但保留 TCC owner 到引擎停止后。 */
static void XS_ScriptRequestUnitHost(XS_HostInfo* pHost)
{
	XS_ScriptRuntime* pRuntime = XS_ScriptAcquireHost(pHost);

	if ( pRuntime != NULL ) {
		XS_ScriptCallUnit(pRuntime);
		XS_ScriptRelease(pRuntime);
	}
}

static void XS_ScriptQuiesceHost(XS_HostInfo* pHost)
{
	XS_ScriptRuntime* pRuntime = NULL;

	if ( pHost == NULL || pHost->RuntimeLock == NULL ) return;
	xrtMutexLock((xmutex*)pHost->RuntimeLock);
	pRuntime = (XS_ScriptRuntime*)pHost->Runtime;
	if ( pRuntime != NULL && xrtRefRetain(&pRuntime->iReferences) < 0 ) pRuntime = NULL;
	xrtMutexUnlock((xmutex*)pHost->RuntimeLock);
	if ( pRuntime != NULL ) {
		XS_ScriptBeginRetire(pRuntime);
		XS_ScriptRelease(pRuntime);
	}
}

#endif
