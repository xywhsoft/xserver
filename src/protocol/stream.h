#ifndef XS_PROTOCOL_STREAM_H
#define XS_PROTOCOL_STREAM_H

/*
 * xs3 TCP / TCP+TLS 驱动（设计 §6.3）
 * - xs 建监听（含 tcps：host 证书 → SNI 身份表），流事件透传脚本回调
 * - 连接生命周期由 xs 持有（Accept 接管 → 注册表 → Close 时 Destroy）
 * - Custom 旋钮：idle_timeout（毫秒，server Custom，缺省关闭）——超时连接
 *   graceful Close（复用注册表扫描；正式按代 drain 随重载工作包）
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"
#include "../core/config.h"
#include "../core/engine.h"
#include "../core/tls.h"
#include "../runtime/registry.h"
#include "../script/script.h"

typedef struct XS_TcpRuntime {
	XS_ServerInfo*		pServer;
	XS_HostInfo*		pHost;		/* DefaultHost：回调挂载点 */
	bool			bTls;
	xnetlistener*		pListener;	/* 明文模式 */
	xtlslistener*		pTlsListener;	/* tcps 模式 */
	XS_ListenerSlot*	pListenerSlot;
	XS_TlsTable		tTls;
	XS_ConnRegistry	tRegistry;
	XS_ServerGeneration*	pGeneration;
	uint64			iIdleMs;	/* 0 = 关闭 idle 保护 */
	uint64			iSweepTimer;
	xatomic32		tStopping;
} XS_TcpRuntime;

/* Custom 旋钮只读（不弹出，键仍留给应用读取） */
static bool XS_CustomGetInt(xvalue* pCustom, const char* sKey, int64* pOut)
{
	xvalue* pVal = (pCustom != NULL) ? xrtValueObjectGet(pCustom, xrtStrViewN(sKey, strlen(sKey))) : NULL;

	*pOut = 0;
	return pVal != NULL && xrtValueGetInt(pVal, pOut);
}

static void XS_TcpConnView(XS_ConnRecord* pRecord, XS_StreamConn* pConn)
{
	pConn->tcp = pRecord->pTcp;
	pConn->tls = pRecord->pTls;
}

/* —— 明文流事件 shim —— */

static void XS_TcpOnOpen(xnetstream* pStream, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = pRecord->pScript;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventOpen != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventOpen(pRecord->pHost, &tConn);
		XS_ScriptLeave(pPrevious);
	}
	(void)pStream;
}

static void XS_TcpOnRead(xnetstream* pStream, xnetbuf* pBuffer, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = pRecord->pScript;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventData != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventData(pRecord->pHost, &tConn, pBuffer);
		XS_ScriptLeave(pPrevious);
	}
	(void)pStream;
}

static void XS_TcpOnEnd(xnetstream* pStream, ptr pData)
{
	(void)pData;
	(void)xrtNetStreamClose(pStream);
}

static void XS_TcpOnClose(xnetstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = pRecord->pScript;
	XS_ServerGeneration* pGeneration = pRecord->pGeneration;

	if ( pScript != NULL && pScript->procEventClose != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventClose(pRecord->pHost, &tConn, iResult, pError);
		XS_ScriptLeave(pPrevious);
	}
	xrtNetStreamDestroy(pStream);
	XS_RegistryRemove(pRecord->pRegistry, pRecord);
	xrtFree(pRecord);
	XS_ScriptRelease(pScript);
	XS_GenerationConnectionRelease(pGeneration);
}

static const xnetstreamevents g_XS_TcpStreamEvents = {
	NULL,			/* Open 由 Accept 侧触发语义替代：见下，Accept 后手动补发 */
	XS_TcpOnRead,
	XS_TcpOnEnd,		/* peer FIN → 唯一 Close 终态 */
	NULL, NULL, NULL,	/* HighWater / LowWater / Drain */
	XS_TcpOnClose
};

static bool XS_TcpOnAccept(xnetlistener* pListener, xnetstream* pStream, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;
	XS_TcpRuntime* pRuntime = NULL;
	XS_ServerGeneration* pGeneration = NULL;
	XS_ConnRecord* pRecord = (XS_ConnRecord*)xrtCalloc(1, sizeof(XS_ConnRecord));
	XS_ScriptRuntime* pScript;

	(void)pListener;
	if ( pRecord == NULL ||
	     !XS_ListenerSlotAcquireConnection(pSlot, (void**)&pRuntime, &pGeneration) ) {
		xrtFree(pRecord);
		return false;		/* 拒绝接入（库立即关闭该流） */
	}
	if ( xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
		XS_GenerationConnectionRelease(pGeneration);
		xrtFree(pRecord);
		return false;
	}
	pScript = XS_ScriptAcquireHost(pRuntime->pHost);
	if ( pScript == NULL ) {
		XS_GenerationConnectionRelease(pGeneration);
		xrtFree(pRecord);
		return false;
	}
	pRecord->pHost = pRuntime->pHost;
	pRecord->pTcp = pStream;
	pRecord->pGeneration = pGeneration;
	pRecord->pScript = pScript;
	if ( !XS_RegistryAdd(&pRuntime->tRegistry, pRecord) ) {
		XS_ScriptRelease(pScript);
		XS_GenerationConnectionRelease(pGeneration);
		xrtFree(pRecord);
		return false;
	}
	(void)xrtNetStreamSetData(pStream, pRecord);	/* Accept 运行于目标流 Worker，合法 */
	XS_TcpOnOpen(pStream, (ptr)pRecord);		/* 补发 Open（事件表 Open 位留空防双发） */
	return true;
}

static void XS_TcpOnListenerClose(xnetlistener* pListener, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;

	xrtNetListenerDestroy(pListener);
	XS_ListenerSlotResourceClose(pSlot);
}

static const xnetlistenerevents g_XS_TcpListenerEvents = {
	XS_TcpOnAccept,
	NULL,			/* Error */
	XS_TcpOnListenerClose
};

/* —— tcps 流事件 shim（buffer 为 const 视图，消费走 xrtTlsStreamConsume）—— */

static void XS_TlsOnOpen(xtlsstream* pStream, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = pRecord->pScript;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventOpen != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventOpen(pRecord->pHost, &tConn);
		XS_ScriptLeave(pPrevious);
	}
	(void)pStream;
}

static void XS_TlsOnRead(xtlsstream* pStream, const xnetbuf* pBuffer, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = pRecord->pScript;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventData != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventData(pRecord->pHost, &tConn, (xnetbuf*)pBuffer);	/* 仅 Peek；tcps 消费走 xrtTlsStreamConsume */
		XS_ScriptLeave(pPrevious);
	}
	(void)pStream;
}

static void XS_TlsOnEnd(xtlsstream* pStream, ptr pData)
{
	(void)pData;
	(void)xrtTlsStreamClose(pStream);
}

static void XS_TlsOnClose(xtlsstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = pRecord->pScript;
	XS_ServerGeneration* pGeneration = pRecord->pGeneration;

	if ( pScript != NULL && pScript->procEventClose != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventClose(pRecord->pHost, &tConn, iResult, pError);
		XS_ScriptLeave(pPrevious);
	}
	xrtTlsStreamDestroy(pStream);
	XS_RegistryRemove(pRecord->pRegistry, pRecord);
	xrtFree(pRecord);
	XS_ScriptRelease(pScript);
	XS_GenerationConnectionRelease(pGeneration);
}

static const xtlsstreamevents g_XS_TlsStreamEvents = {
	XS_TlsOnOpen,
	XS_TlsOnRead,
	XS_TlsOnEnd, NULL, NULL,
	XS_TlsOnClose,
	NULL			/* Ticket */
};

static bool XS_TlsOnAccept(xtlslistener* pListener, xtlsstream* pStream, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;
	XS_TcpRuntime* pRuntime = NULL;
	XS_ServerGeneration* pGeneration = NULL;
	XS_ConnRecord* pRecord = (XS_ConnRecord*)xrtCalloc(1, sizeof(XS_ConnRecord));
	XS_ScriptRuntime* pScript;

	(void)pListener;
	if ( pRecord == NULL ||
	     !XS_ListenerSlotAcquireConnection(pSlot, (void**)&pRuntime, &pGeneration) ) {
		xrtFree(pRecord);
		return false;
	}
	if ( xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
		XS_GenerationConnectionRelease(pGeneration);
		xrtFree(pRecord);
		return false;
	}
	pScript = XS_ScriptAcquireHost(pRuntime->pHost);
	if ( pScript == NULL ) {
		XS_GenerationConnectionRelease(pGeneration);
		xrtFree(pRecord);
		return false;
	}
	pRecord->pHost = pRuntime->pHost;
	pRecord->pTls = pStream;
	pRecord->pGeneration = pGeneration;
	pRecord->pScript = pScript;
	if ( !XS_RegistryAdd(&pRuntime->tRegistry, pRecord) ) {
		XS_ScriptRelease(pScript);
		XS_GenerationConnectionRelease(pGeneration);
		xrtFree(pRecord);
		return false;
	}
	(void)xrtTlsStreamSetEvents(pStream, &g_XS_TlsStreamEvents, pRecord);	/* Accept 在目标流 Worker，合法 */
	return true;
}

static void XS_TlsOnListenerClose(xtlslistener* pListener, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;

	xrtTlsListenerDestroy(pListener);
	XS_ListenerSlotResourceClose(pSlot);
}

static const xtlslistenerevents g_XS_TlsListenerEvents = {
	XS_TlsOnAccept,
	NULL,			/* HandshakeError：握手失败由库内部关闭 */
	NULL,			/* Error */
	XS_TlsOnListenerClose
};

/* —— idle 扫描（引擎定时器，周期 = min(idle/2, 1s)）—— */

static void XS_TcpSweepProc(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_TcpRuntime* pRuntime = (XS_TcpRuntime*)pData;

	(void)pWorker; (void)iId;
	if ( iResult == XNET_RESULT_OK &&
	     xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) == 0 ) {
		(void)XS_RegistrySweepIdle(&pRuntime->tRegistry, pRuntime->iIdleMs);
		{
			uint64 iInterval = pRuntime->iIdleMs / 2;

			if ( iInterval > 1000 ) {
				iInterval = 1000;
			}
			if ( iInterval < 10 ) {
				iInterval = 10;
			}
			if ( XS_GenerationRetain(pRuntime->pGeneration) ) {
				pRuntime->iSweepTimer = xrtNetEngineAfter(pRuntime->pServer->Engine, 0,
					iInterval * 1000, XS_TcpSweepProc, pData);
				if ( pRuntime->iSweepTimer == 0 ) {
					XS_GenerationRelease(pRuntime->pGeneration);
				}
			}
		}
	}
	XS_GenerationRelease(pRuntime->pGeneration);
}

static bool XS_TcpScheduleSweep(XS_TcpRuntime* pRuntime)
{
	uint64 iInterval = pRuntime->iIdleMs / 2;

	if ( iInterval > 1000 ) iInterval = 1000;
	if ( iInterval < 10 ) iInterval = 10;
	if ( !XS_GenerationRetain(pRuntime->pGeneration) ) return false;
	pRuntime->iSweepTimer = xrtNetEngineAfter(pRuntime->pServer->Engine, 0,
		iInterval * 1000, XS_TcpSweepProc, pRuntime);
	if ( pRuntime->iSweepTimer == 0 ) {
		XS_GenerationRelease(pRuntime->pGeneration);
		return false;
	}
	return true;
}

/* —— 启动 / 停止 —— */

static bool XS_TcpStartEx(
	XS_ServerInfo* pServer,
	bool bStartEndpoint,
	bool bAcceptEndpoint,
	char* sErr,
	size_t iErrCap)
{
	XS_TcpRuntime* pRuntime = (XS_TcpRuntime*)xrtCalloc(1, sizeof(XS_TcpRuntime));
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pServer->DefaultHost->Runtime;
	xnetaddr tAddr;
	int64 iIdle = 0;

	if ( pRuntime == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	pRuntime->pServer = pServer;
	pRuntime->pHost = pServer->DefaultHost;
	pRuntime->pGeneration = (XS_ServerGeneration*)pServer->Generation;
	xrtAtomic32Init(&pRuntime->tStopping, 0);
	pServer->Runtime = pRuntime;

	if ( pScript == NULL || pScript->procEventData == NULL ) {
		snprintf(sErr, iErrCap, "tcp server '%s' requires script exporting EventData", pServer->Name);
		return false;
	}
	if ( !xrtNetAddrParse(&tAddr, pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port) ) {
		snprintf(sErr, iErrCap, "tcp server '%s' addr parse failed", pServer->Name);
		return false;
	}
	if ( !XS_RegistryInit(&pRuntime->tRegistry) ) {
		snprintf(sErr, iErrCap, "registry init failed");
		return false;
	}
	(void)XS_CustomGetInt(pServer->Custom, "idle_timeout", &iIdle);
	pRuntime->iIdleMs = (iIdle > 0) ? (uint64)iIdle : 0;

	if ( pServer->TLS ) {
		if ( XS_TlsSharedContext() == NULL ) {
			snprintf(sErr, iErrCap, "tls context create failed");
			return false;
		}
		if ( !XS_TlsTableBuild(pServer, &pRuntime->tTls, sErr, iErrCap) ) return false;
		if ( pRuntime->tTls.iCount == 0 ) {
			snprintf(sErr, iErrCap, "tcps server '%s' has no tls_cert/tls_key host", pServer->Name);
			return false;
		}
		pRuntime->bTls = true;
	}
	if ( bStartEndpoint ) {
		pRuntime->pListenerSlot = XS_ListenerSlotCreate(pRuntime, pRuntime->pGeneration,
			pRuntime->bTls ? (void*)&pRuntime->tTls : NULL, bAcceptEndpoint);
		if ( pRuntime->pListenerSlot == NULL ) {
			snprintf(sErr, iErrCap, "tcp listener slot create failed");
			return false;
		}
	}

	if ( bStartEndpoint && !pServer->TLS ) {
		xnetlistenconfig tListen;

		xrtNetListenConfigInit(&tListen);
		tListen.Address = tAddr;
		tListen.ReuseAddress = true;	/* Windows 默认独占，快速重绑 */
		tListen.ExclusiveAddress = false;	/* 与 ReuseAddress 互斥（配置校验） */
		if ( pServer->Backlog > 0 ) {
			tListen.Backlog = (int)pServer->Backlog;
		}
		if ( pServer->RecvLimit > 0 ) {
			tListen.Stream.ReadLimit = pServer->RecvLimit;
		}
		if ( !XS_ListenerSlotResourceAdd(pRuntime->pListenerSlot) ) {
			return false;
		}
		pRuntime->pListener = xrtNetListen(pServer->Engine, &tListen,
			&g_XS_TcpListenerEvents, &g_XS_TcpStreamEvents, pRuntime->pListenerSlot);
		if ( pRuntime->pListener == NULL ) {
			XS_ListenerSlotResourceCancel(pRuntime->pListenerSlot);
			XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
			pRuntime->pListenerSlot = NULL;
			const xerror* pErr = xrtGetError();
			snprintf(sErr, iErrCap, "tcp server '%s' listen failed (port %u): %s", pServer->Name,
				pServer->Port, pErr != NULL ? xrtErrorMessage(pErr) : "unknown");
			return false;
		}
	} else if ( bStartEndpoint ) {
		xtlslistenerconfig tTlsListen;
		xtlscontext* pContext = XS_TlsSharedContext();

		xrtNetListenConfigInit(&tTlsListen.Listen);
		tTlsListen.Listen.Address = tAddr;
		tTlsListen.Listen.ReuseAddress = true;
		tTlsListen.Listen.ExclusiveAddress = false;
		if ( pServer->Backlog > 0 ) {
			tTlsListen.Listen.Backlog = (int)pServer->Backlog;
		}
		if ( pServer->RecvLimit > 0 ) {
			tTlsListen.Listen.Stream.ReadLimit = pServer->RecvLimit;
		}
		xrtTlsServerConfigInit(&tTlsListen.Tls);
		tTlsListen.Tls.Context = pContext;
		tTlsListen.Tls.Identity = pRuntime->tTls.pEntries[0].pIdentity;	/* 无 SNI 回落 */
		tTlsListen.Tls.Select = XS_TlsSlotSelect;
		tTlsListen.Tls.SelectContext = pRuntime->pListenerSlot;
		if ( !XS_ListenerSlotResourceAdd(pRuntime->pListenerSlot) ) {
			return false;
		}
		pRuntime->pTlsListener = xrtTlsListenerStart(pServer->Engine, &tTlsListen,
			&g_XS_TlsListenerEvents, &g_XS_TlsStreamEvents, pRuntime->pListenerSlot);
		if ( pRuntime->pTlsListener == NULL ) {
			XS_ListenerSlotResourceCancel(pRuntime->pListenerSlot);
			XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
			pRuntime->pListenerSlot = NULL;
			snprintf(sErr, iErrCap, "tcps server '%s' listen failed (port %u)", pServer->Name, pServer->Port);
			return false;
		}
	}

	if ( pRuntime->iIdleMs > 0 ) {
		(void)XS_TcpScheduleSweep(pRuntime);
	}
	printf("[xs] server '%s' %s %s on %s:%u%s\n", pServer->Name,
		pRuntime->bTls ? "tcps" : "tcp",
		bStartEndpoint ? (bAcceptEndpoint ? "ready" : "bound") : "prepared",
		pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port,
		pRuntime->iIdleMs > 0 ? " (idle protected)" : "");
	return true;
}

static bool XS_TcpHandoff(XS_TcpRuntime* pOld, XS_TcpRuntime* pNew)
{
	XS_ListenerSlot* pSlot;

	if ( pOld == NULL || pNew == NULL || pOld->bTls != pNew->bTls ||
	     pOld->pListenerSlot == NULL ) return false;
	pSlot = pOld->pListenerSlot;
	if ( !XS_ListenerSlotHandoff(pSlot, pOld, pNew, pNew->pGeneration,
		pNew->bTls ? (void*)&pNew->tTls : NULL) ) return false;
	pNew->pListenerSlot = pSlot;
	pNew->pListener = pOld->pListener;
	pNew->pTlsListener = pOld->pTlsListener;
	pOld->pListenerSlot = NULL;
	pOld->pListener = NULL;
	pOld->pTlsListener = NULL;
	return true;
}

static void XS_TcpStop(XS_TcpRuntime* pRuntime)
{
	XS_ListenerSlot* pSlot;

	if ( pRuntime == NULL ) {
		return;
	}
	xrtAtomic32Store(&pRuntime->tStopping, 1, XMEMORY_RELEASE);
	if ( pRuntime->iSweepTimer != 0 ) {
		(void)xrtNetEngineTimerCancel(pRuntime->pServer->Engine, pRuntime->iSweepTimer);
		pRuntime->iSweepTimer = 0;
	}
	XS_RegistryStopAccepting(&pRuntime->tRegistry);
	pSlot = pRuntime->pListenerSlot;
	if ( pSlot != NULL ) XS_ListenerSlotBeginClose(pSlot, pRuntime);
	if ( pRuntime->pListener != NULL ) {
		xrtNetListenerClose(pRuntime->pListener);	/* Close 回调里 Destroy */
		pRuntime->pListener = NULL;
	}
	if ( pRuntime->pTlsListener != NULL ) {
		xrtTlsListenerClose(pRuntime->pTlsListener);
		pRuntime->pTlsListener = NULL;
	}
	pRuntime->pListenerSlot = NULL;
}

static void XS_TcpCloseConnections(XS_TcpRuntime* pRuntime)
{
	if ( pRuntime != NULL ) {
		XS_RegistryCloseAll(&pRuntime->tRegistry);
	}
}

static void XS_TcpUnit(XS_TcpRuntime* pRuntime)
{
	if ( pRuntime == NULL ) {
		return;
	}
	XS_RegistryUnit(&pRuntime->tRegistry);
	XS_TlsTableUnit(&pRuntime->tTls);
	if ( pRuntime->pListenerSlot != NULL ) {
		XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
		pRuntime->pListenerSlot = NULL;
	}
	xrtFree(pRuntime);
}

#endif
