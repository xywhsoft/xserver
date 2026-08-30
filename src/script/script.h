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
	xatomic32		tInitialized;	/* 候选是否已经执行 Init；决定失败补偿是否调用 Unit */
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

/* reload controller 可为一次 reconcile 安装线程局部源码快照读取器。返回缓冲
 * 始终由调用方释放；启动路径未安装 hook 时仍直接读取文件。 */
typedef bytes (*XS_ScriptReadHookProc)(const char* sPath, size_t* piSize, void* pContext);
static XRT_THREAD_LOCAL XS_ScriptReadHookProc g_XS_ScriptReadHook = NULL;
static XRT_THREAD_LOCAL void* g_XS_ScriptReadContext = NULL;

static void XS_ScriptSetReadHook(XS_ScriptReadHookProc procRead, void* pContext)
{
	g_XS_ScriptReadHook = procRead;
	g_XS_ScriptReadContext = pContext;
}

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
	XS_ServerGeneration* pGeneration;

	if ( pRuntime == NULL || pRuntime->pInitLock == NULL ) return false;
	xrtMutexLock(pRuntime->pInitLock);
	bReady = xrtAtomic32Load(&pRuntime->tReady, XMEMORY_ACQUIRE) != 0;
	xrtMutexUnlock(pRuntime->pInitLock);
	pGeneration = pRuntime->pHost != NULL && pRuntime->pHost->Server != NULL ?
		(XS_ServerGeneration*)pRuntime->pHost->Server->Generation : NULL;
	return bReady && XS_GenerationWaitPublished(pGeneration);
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

static xatomic64 g_XS_Generation;

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
	pData = g_XS_ScriptReadHook != NULL ?
		g_XS_ScriptReadHook(sDevPath, &iSize, g_XS_ScriptReadContext) :
		xrtFileReadAll(sDevPath, &iSize);
	if ( pData == NULL ) {
		printf("[xs] script read failed: %s\n", sDevPath);
		xrtFree(sDevPath);
		return NULL;
	}
	pTcc = XS_TccCreateForHost(pHost);	/* 每个候选先拥有独立编译状态 */
	if ( pTcc == NULL ) {
		printf("[xs] tcc create failed\n");
		xrtFree(pData);
		xrtFree(sDevPath);
		return NULL;
	}
	/* 唯一路径避免并行候选互相覆盖源码；add_file 返回后立即卸载。 */
	snprintf(sVirtual, sizeof(sVirtual), "/xs/script/%p.c", (void*)pTcc);
	if ( !tcc_vfs_mount_memory(sVirtual, pData, iSize) ) {
		printf("[xs] script vfs mount failed\n");
		tcc_delete(pTcc);
		xrtFree(pData);
		xrtFree(sDevPath);
		return NULL;
	}
	xrtFree(pData);		/* mount_memory 深拷贝，源缓冲即弃 */
	if ( tcc_add_file(pTcc, sVirtual) < 0 ) {
		(void)tcc_vfs_unmount(sVirtual);
		printf("[xs] script compile failed: %s\n", sDevPath);
		tcc_delete(pTcc);
		xrtFree(sDevPath);
		return NULL;
	}
	(void)tcc_vfs_unmount(sVirtual);
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
	pRuntime->tGeneration = xrtAtomic64FetchAdd(
		&g_XS_Generation, 1, XMEMORY_RELAXED) + 1u;
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
	xrtAtomic32Init(&pRuntime->tInitialized, 0);
	xrtAtomic32Init(&pRuntime->tUnitCalled, 0);
	xrtAtomic32Init(&pRuntime->tOwnerReleased, 0);

	printf("[xs] script loaded: %s (%.1f KB gen %llu)\n", sDevPath, (double)iSize / 1024.0,
		(unsigned long long)pRuntime->tGeneration);
	xrtFree(sDevPath);
	return pRuntime;
}

/* 候选先进入 staging 并保持 init 锁。reload 会先完成所有脚本编译和 endpoint
 * 预绑定，越过不可逆 staging 线性点后才执行 Swap/Init。 */
static void XS_ScriptStagePrepared(XS_ScriptRuntime* pRuntime)
{
	if ( pRuntime == NULL ) return;
	xrtMutexLock(pRuntime->pInitLock);
	/* pInitLock 有意保持到唯一的 Publish/Discard。 */
}

/* init 锁已由本 controller 线程持有。Init 创建的 timer 会等待同一个锁，
 * 因而在整个 generation 发布前不会进入脚本。 */
static void XS_ScriptInitializePrepared(XS_HostInfo* pHost, XS_ScriptRuntime* pRuntime)
{
	if ( pHost == NULL || pRuntime == NULL ) return;
	if ( pRuntime->procInit != NULL ) {
		XS_ScriptRuntime* pContext = XS_ScriptEnter(pRuntime);

		pRuntime->procInit(pHost);
		XS_ScriptLeave(pContext);
	}
	xrtAtomic32Store(&pRuntime->tInitialized, 1, XMEMORY_RELEASE);
}

static void XS_ScriptPrepare(XS_HostInfo* pHost, XS_ScriptRuntime* pRuntime)
{
	XS_ScriptStagePrepared(pRuntime);
	XS_ScriptInitializePrepared(pHost, pRuntime);
}

/* 把已 Init 的脚本装入候选 host，但不设 ready。server 候选驱动可以
 * 在此后绑定脚本符号；Init 中创建的 timer 仍被 pInitLock 阻挡。 */
static XS_ScriptRuntime* XS_ScriptMountPrepared(
	XS_HostInfo* pHost,
	XS_ScriptRuntime* pRuntime)
{
	XS_ScriptRuntime* pPrevious;

	if ( pHost == NULL || pRuntime == NULL ) return NULL;
	xrtMutexLock((xmutex*)pHost->RuntimeLock);
	pPrevious = (XS_ScriptRuntime*)pHost->Runtime;
	pHost->Runtime = pRuntime;
	xrtMutexUnlock((xmutex*)pHost->RuntimeLock);
	return pPrevious;
}

/* 只做极短的 ready 发布；调用方已将 host/拓扑/listener 切到新代。 */
static void XS_ScriptActivatePrepared(XS_ScriptRuntime* pRuntime)
{
	if ( pRuntime == NULL ) return;
	xrtAtomic32Store(&pRuntime->tReady, 1, XMEMORY_RELEASE);
	xrtMutexUnlock(pRuntime->pInitLock);
}

/* host 级换代使用：指针与 ready 在同一个极短提交段内发布。 */
static XS_ScriptRuntime* XS_ScriptPublishPrepared(
	XS_HostInfo* pHost,
	XS_ScriptRuntime* pRuntime)
{
	XS_ScriptRuntime* pPrevious = XS_ScriptMountPrepared(pHost, pRuntime);

	XS_ScriptActivatePrepared(pRuntime);
	return pPrevious;
}

static void XS_ScriptDiscardPrepared(XS_ScriptRuntime* pRuntime)
{
	if ( pRuntime == NULL ) return;
	/* 纯编译/绑定阶段尚未执行 Init 时，不得为它调用 ServiceUnit。 */
	if ( xrtAtomic32Load(&pRuntime->tInitialized, XMEMORY_ACQUIRE) == 0 ) {
		xrtAtomic32Store(&pRuntime->tUnitCalled, 1, XMEMORY_RELEASE);
	}
	XS_ScriptBeginRetire(pRuntime);
	xrtMutexUnlock(pRuntime->pInitLock);
	XS_ScriptRetire(pRuntime);
}

/* 整个 server 预编译阶段后续脚本失败：此 runtime 从未 Init，
 * 因此析构时不得调用 ServiceUnit。 */
static void XS_ScriptDiscardCompiled(XS_ScriptRuntime* pRuntime)
{
	if ( pRuntime == NULL ) return;
	xrtAtomic32Store(&pRuntime->tUnitCalled, 1, XMEMORY_RELEASE);
	XS_ScriptRetire(pRuntime);
}

/* server/all 候选未发布：先从私有 host 槽位拆下，再解锁退役。 */
static void XS_ScriptDiscardMountedPrepared(
	XS_HostInfo* pHost,
	XS_ScriptRuntime* pRuntime)
{
	if ( pHost == NULL || pRuntime == NULL ) return;
	xrtMutexLock((xmutex*)pHost->RuntimeLock);
	if ( pHost->Runtime == pRuntime ) pHost->Runtime = NULL;
	xrtMutexUnlock((xmutex*)pHost->RuntimeLock);
	XS_ScriptDiscardPrepared(pRuntime);
}

/* 挂载并初始化（ServiceInit 内可 xsSwapTake 取回交接数据）。 */
static void XS_ScriptAttach(XS_HostInfo* pHost, XS_ScriptRuntime* pRuntime)
{
	XS_ScriptRuntime* pPrevious = NULL;

	if ( pRuntime != NULL ) {
		XS_ScriptPrepare(pHost, pRuntime);
		pPrevious = XS_ScriptPublishPrepared(pHost, pRuntime);
	} else if ( pHost != NULL ) {
		xrtMutexLock((xmutex*)pHost->RuntimeLock);
		pPrevious = (XS_ScriptRuntime*)pHost->Runtime;
		pHost->Runtime = NULL;
		xrtMutexUnlock((xmutex*)pHost->RuntimeLock);
	}
	if ( pPrevious != NULL && pPrevious != pRuntime ) XS_ScriptRetire(pPrevious);
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
