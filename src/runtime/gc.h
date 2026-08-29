#ifndef XS_RUNTIME_GC_H
#define XS_RUNTIME_GC_H

/*
 * 延迟释放队列（listener 在线重建配套）
 *
 * 安全释放信号 = 旧驱动的连接注册表计数归零：
 *   Close 回调最后一步才做 XS_RegistryRemove（计数 --），
 *   所以计数 == 0 意味着所有 Close 回调已经完成，
 *   不再有人引用旧 driver runtime / TCC 代码 / server 结构体。
 *
 * 释放顺序（依赖链，前一步完成才做下一步）：
 *   1. driver runtime（含注册表互斥锁）
 *   2. TCC 状态（脚本代码内存）
 *   3. server 结构体（字符串字段 + hosts 数组）
 *
 * custom 类无连接计数 → 固定宽限期后释放。
 * 监控由引擎定时器驱动（500ms 间隔），归零后加 1s 宽限再实际释放。
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"
#include "../core/config.h"
#include "../core/engine.h"
#include "../core/driver.h"
#include "../script/script.h"

typedef struct XS_GcItem {
	struct XS_GcItem*	pNext;
	void*			pDriverRuntime;	/* XS_HttpRuntime* / XS_TcpRuntime* / XS_WsRuntime* / XS_UdpRuntime* / NULL(custom) */
	XS_ScriptRuntime*	pScriptRuntime;	/* 旧脚本运行时（TCC 状态） */
	XS_ServerInfo*		pServer;	/* 旧 server 结构体 */
	char			sClass[8];	/* "http"/"tcp"/"ws"/"udp"/"custom" */
	int64			tEnqueued;	/* 入队时间（xrtNow 微秒） */
	uint32			iPollCount;	/* 扫描次数（用于超时） */
} XS_GcItem;

static XS_GcItem* g_XS_GcHead = NULL;
static xmutex* g_XS_GcLock = NULL;
static bool g_XS_GcTimerActive = false;

/* 旧驱动的连接计数（归零 = 所有 Close 回调完成） */
static uint32 XS_GcConnCount(const XS_GcItem* pItem)
{
	if ( pItem->pDriverRuntime == NULL ) {
		return 0;
	}
	if ( strcmp(pItem->sClass, "http") == 0 ) {
		return ((XS_HttpRuntime*)pItem->pDriverRuntime)->tRegistry.iCount;
	}
	if ( strcmp(pItem->sClass, "tcp") == 0 ) {
		return ((XS_TcpRuntime*)pItem->pDriverRuntime)->tRegistry.iCount;
	}
	if ( strcmp(pItem->sClass, "ws") == 0 ) {
		return ((XS_WsRuntime*)pItem->pDriverRuntime)->iConnCount;
	}
	return 0;		/* udp / custom：无连接计数 */
}

/* 按序释放一项（调用时机：连接计数已归零 + 宽限期已过） */
static void XS_GcFreeItem(XS_GcItem* pItem)
{
	/* 日志在释放前打（释放后 pServer->Name 为 UAF） */
	printf("[xs] gc: freeing retired server '%s' (%s, polls=%u)\n",
		pItem->pServer != NULL && pItem->pServer->Name != NULL ? pItem->pServer->Name : "?",
		pItem->sClass, pItem->iPollCount);

	/* 1. driver runtime（含注册表锁与 TLS 表） */
	if ( pItem->pDriverRuntime != NULL ) {
		if ( strcmp(pItem->sClass, "http") == 0 ) {
			XS_HttpUnit((XS_HttpRuntime*)pItem->pDriverRuntime);
		} else if ( strcmp(pItem->sClass, "tcp") == 0 ) {
			XS_TcpUnit((XS_TcpRuntime*)pItem->pDriverRuntime);
		} else if ( strcmp(pItem->sClass, "ws") == 0 ) {
			XS_WsUnit((XS_WsRuntime*)pItem->pDriverRuntime);
		} else if ( strcmp(pItem->sClass, "udp") == 0 ) {
			XS_UdpUnit((XS_UdpRuntime*)pItem->pDriverRuntime);
		}
	}
	/* 2. TCC 状态（脚本代码内存——所有引用者已消失） */
	if ( pItem->pScriptRuntime != NULL ) {
		tcc_delete(pItem->pScriptRuntime->pTcc);
		xrtFree(pItem->pScriptRuntime);
	}
	/* 3. server 结构体（字符串 + hosts；Custom xvalue 树不释放——新代借用） */
	if ( pItem->pServer != NULL ) {
		XS_ConfigServerFree(pItem->pServer);
	}
	xrtFree(pItem);
}

/* 扫描：归零 + 宽限 → 释放；超时强制释放（30s 兜底） */
static void XS_GcSweep(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_GcItem** ppLink;
	int64 tNow = xrtNow();
	xnetengine* pEngine = (xnetengine*)pData;
	bool bFreedAny = false;

	(void)pWorker; (void)iId;
	if ( iResult != XNET_RESULT_OK || pEngine == NULL ) {
		return;
	}
	if ( g_XS_GcLock != NULL ) {
		xrtMutexLock(g_XS_GcLock);
	}
	ppLink = &g_XS_GcHead;
	while ( *ppLink != NULL ) {
		XS_GcItem* pItem = *ppLink;
		uint32 iConn = XS_GcConnCount(pItem);
		int64 tAge = tNow - pItem->tEnqueued;

		/* 条件：连接归零 + 至少 1s 宽限（跨核可见性余量）
		 * 或 custom/udp 类无计数 + 5s 固定宽限
		 * 或 30s 超时强制释放 */
		bool bConnIdle = (iConn == 0);
		bool bGraceOk = pItem->pDriverRuntime == NULL ? tAge > 5 * 1000 * 1000 : tAge > 1 * 1000 * 1000;
		bool bTimeout = pItem->iPollCount > 60;		/* 60 × 500ms = 30s */

		if ( (bConnIdle && bGraceOk) || bTimeout ) {
			*ppLink = pItem->pNext;
			if ( bTimeout && !bConnIdle ) {
				printf("[xs] gc: timeout force-free (conn=%u, age=%lldms)\n",
					iConn, (long long)(tAge / 1000));
			}
			XS_GcFreeItem(pItem);
			bFreedAny = true;
		} else {
			pItem->iPollCount++;
			ppLink = &pItem->pNext;
		}
	}
	{
		bool bEmpty = g_XS_GcHead == NULL;

		if ( g_XS_GcLock != NULL ) {
			xrtMutexUnlock(g_XS_GcLock);
		}
		if ( bEmpty ) {
			g_XS_GcTimerActive = false;
		} else {
			(void)xrtNetEngineAfter(pEngine, 0, 500 * 1000, XS_GcSweep, pData);
		}
	}
	(void)bFreedAny;
}

/* 入队：由重建路径调用（DeferredFire 的 worker 上） */
static void XS_GcEnqueue(
	xnetengine* pEngine,
	void* pDriverRuntime,
	XS_ScriptRuntime* pScriptRuntime,
	XS_ServerInfo* pServer)
{
	XS_GcItem* pItem;

	if ( g_XS_GcLock == NULL ) {
		g_XS_GcLock = xrtMutexCreate();
		if ( g_XS_GcLock == NULL ) {
			return;		/* 极端 OOM：退化为泄漏 */
		}
	}
	pItem = (XS_GcItem*)xrtCalloc(1, sizeof(XS_GcItem));
	if ( pItem == NULL ) {
		return;
	}
	pItem->pDriverRuntime = pDriverRuntime;
	pItem->pScriptRuntime = pScriptRuntime;
	pItem->pServer = pServer;
	if ( pServer != NULL && pServer->Class != NULL ) {
		snprintf(pItem->sClass, sizeof(pItem->sClass), "%s", pServer->Class);
	}
	pItem->tEnqueued = xrtNow();

	if ( g_XS_GcLock != NULL ) {
		xrtMutexLock(g_XS_GcLock);
	}
	pItem->pNext = g_XS_GcHead;
	g_XS_GcHead = pItem;
	{
		bool bNeedTimer = !g_XS_GcTimerActive;

		g_XS_GcTimerActive = true;
		if ( g_XS_GcLock != NULL ) {
			xrtMutexUnlock(g_XS_GcLock);
		}
		if ( bNeedTimer && pEngine != NULL ) {
			(void)xrtNetEngineAfter(pEngine, 0, 500 * 1000, XS_GcSweep, pEngine);
		}
	}
	printf("[xs] gc: enqueued retired server '%s' (%s, conn=%u)\n",
		pServer != NULL && pServer->Name != NULL ? pServer->Name : "?",
		pItem->sClass, XS_GcConnCount(pItem));
}

/* 进程停机时清空全部（不安全但进程即将退出） */
static void XS_GcShutdown(void)
{
	XS_GcItem* pItem = g_XS_GcHead;

	while ( pItem != NULL ) {
		XS_GcItem* pNext = pItem->pNext;

		XS_GcFreeItem(pItem);
		pItem = pNext;
	}
	g_XS_GcHead = NULL;
	g_XS_GcTimerActive = false;
}

#endif
