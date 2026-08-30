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
	XS_ListenerSlot*	pListenerSlot;
	XS_TlsTable		tTls;
	XS_ConnRegistry	tRegistry;
	XS_ServerGeneration*	pGeneration;
	uint64			iIdleMs;	/* 0 = 关闭 idle 保护 */
	XS_GenerationTimer	tSweepTimer;
	xatomic32		tStopping;
} XS_TcpRuntime;

/* Custom 旋钮只读（不弹出，键仍留给应用读取）。
 * 文档化的资源上限一律为非负整数；类型错误不能静默退回默认值。 */
static bool XS_CustomReadUInt(
	xvalue* pCustom,
	const char* sKey,
	uint64* pOut,
	char* sErr,
	size_t iErrCap)
{
	xvalue* pVal = (pCustom != NULL) ? xrtValueObjectGet(pCustom, xrtStrViewN(sKey, strlen(sKey))) : NULL;
	int64 iValue = 0;

	*pOut = 0;
	if ( pVal == NULL ) return true;
	if ( !xrtValueGetInt(pVal, &iValue) ) {
		snprintf(sErr, iErrCap, "custom field '%s' expect int", sKey);
		return false;
	}
	if ( iValue < 0 ) {
		snprintf(sErr, iErrCap, "custom field '%s' expect non-negative int", sKey);
		return false;
	}
	*pOut = (uint64)iValue;
	return true;
}

/* recv_limit 是缓冲硬上限；当它小于 xrt 默认单次读取块时，同步缩小
 * ReadSize，避免一份合法的小上限配置在底层只报泛化 listen failed。 */
static void XS_StreamApplyReceiveLimit(xnetstreamconfig* pConfig, size_t iLimit)
{
	if ( pConfig == NULL || iLimit == 0 ) return;
	pConfig->ReadLimit = iLimit;
	if ( pConfig->ReadSize > iLimit ) pConfig->ReadSize = iLimit;
}

static void XS_TcpConnView(XS_ConnRecord* pRecord, XS_StreamConn* pConn)
{
	pConn->tcp = pRecord->pTcp;
	pConn->tls = pRecord->pTls;
}

/* 终态可能在发送 API 内同步重入。记录所有权由 Close 撤销，但脚本、
 * generation 与记录本身必须活到所有在途回调退出。 */
static bool XS_TcpRecordRetain(XS_ConnRecord* pRecord)
{
	return pRecord != NULL && xrtRefRetain(&pRecord->iReferences) > 0;
}

static void XS_TcpRecordRelease(XS_ConnRecord* pRecord)
{
	XS_ScriptRuntime* pScript;
	XS_ServerGeneration* pGeneration;

	if ( pRecord == NULL || xrtRefRelease(&pRecord->iReferences) != 0 ) return;
	pScript = pRecord->pScript;
	pGeneration = pRecord->pGeneration;
	xrtFree(pRecord);
	XS_ScriptRelease(pScript);
	XS_GenerationConnectionRelease(pGeneration);
}

/* —— 明文流事件 shim —— */

static void XS_TcpOnOpen(xnetstream* pStream, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript;

	if ( !XS_TcpRecordRetain(pRecord) ) return;
	if ( xrtNetStreamRef(pStream) == NULL ) {
		XS_TcpRecordRelease(pRecord);
		return;
	}
	pScript = pRecord->pScript;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventOpen != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventOpen(pRecord->pHost, &tConn);
		XS_ScriptLeave(pPrevious);
	}
	xrtNetStreamDestroy(pStream);
	XS_TcpRecordRelease(pRecord);
}

static void XS_TcpOnRead(xnetstream* pStream, xnetbuf* pBuffer, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript;

	if ( !XS_TcpRecordRetain(pRecord) ) return;
	if ( xrtNetStreamRef(pStream) == NULL ) {
		XS_TcpRecordRelease(pRecord);
		return;
	}
	pScript = pRecord->pScript;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventData != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventData(pRecord->pHost, &tConn, pBuffer);
		XS_ScriptLeave(pPrevious);
	}
	xrtNetStreamDestroy(pStream);
	XS_TcpRecordRelease(pRecord);
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

	if ( pScript != NULL && pScript->procEventClose != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventClose(pRecord->pHost, &tConn, iResult, pError);
		XS_ScriptLeave(pPrevious);
	}
	XS_RegistryRemove(pRecord->pRegistry, pRecord);
	xrtNetStreamDestroy(pStream);
	XS_TcpRecordRelease(pRecord);
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
	if ( pRecord == NULL ) return false;
	pRecord->iReferences = 1;
	if ( !XS_ListenerSlotAcquireConnection(pSlot, 0, (void**)&pRuntime, &pGeneration) ) {
		XS_TcpRecordRelease(pRecord);
		return false;		/* 拒绝接入（库立即关闭该流） */
	}
	pRecord->pGeneration = pGeneration;
	if ( xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
		XS_TcpRecordRelease(pRecord);
		return false;
	}
	pScript = XS_ScriptAcquireHost(pRuntime->pHost);
	if ( pScript == NULL ) {
		XS_TcpRecordRelease(pRecord);
		return false;
	}
	pRecord->pHost = pRuntime->pHost;
	pRecord->pTcp = pStream;
	pRecord->pScript = pScript;
	if ( !XS_RegistryAdd(&pRuntime->tRegistry, pRecord) ) {
		XS_TcpRecordRelease(pRecord);
		return false;
	}
	if ( !xrtNetStreamSetData(pStream, pRecord) ) {
		XS_RegistryRemove(&pRuntime->tRegistry, pRecord);
		XS_TcpRecordRelease(pRecord);
		return false;
	}
	XS_TcpOnOpen(pStream, (ptr)pRecord);		/* 补发 Open（事件表 Open 位留空防双发） */
	return true;
}

static void XS_TcpOnListenerClose(xnetlistener* pListener, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;

	XS_ListenerSlotResourceClose(pSlot, XS_LISTENER_RESOURCE_PLAIN);
	xrtNetListenerDestroy(pListener);
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
	XS_ScriptRuntime* pScript;

	if ( !XS_TcpRecordRetain(pRecord) ) return;
	if ( xrtTlsStreamRef(pStream) == NULL ) {
		XS_TcpRecordRelease(pRecord);
		return;
	}
	pScript = pRecord->pScript;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventOpen != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventOpen(pRecord->pHost, &tConn);
		XS_ScriptLeave(pPrevious);
	}
	xrtTlsStreamDestroy(pStream);
	XS_TcpRecordRelease(pRecord);
}

static void XS_TlsOnRead(xtlsstream* pStream, const xnetbuf* pBuffer, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript;

	if ( !XS_TcpRecordRetain(pRecord) ) return;
	if ( xrtTlsStreamRef(pStream) == NULL ) {
		XS_TcpRecordRelease(pRecord);
		return;
	}
	pScript = pRecord->pScript;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventData != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventData(pRecord->pHost, &tConn, (xnetbuf*)pBuffer);	/* 仅 Peek；tcps 消费走 xrtTlsStreamConsume */
		XS_ScriptLeave(pPrevious);
	}
	xrtTlsStreamDestroy(pStream);
	XS_TcpRecordRelease(pRecord);
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

	if ( pScript != NULL && pScript->procEventClose != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventClose(pRecord->pHost, &tConn, iResult, pError);
		XS_ScriptLeave(pPrevious);
	}
	XS_RegistryRemove(pRecord->pRegistry, pRecord);
	xrtTlsStreamDestroy(pStream);
	XS_TcpRecordRelease(pRecord);
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
	if ( pRecord == NULL ) return false;
	pRecord->iReferences = 1;
	if ( !XS_TlsAcquireConnection(pSlot, pStream, (void**)&pRuntime, &pGeneration) ) {
		XS_TcpRecordRelease(pRecord);
		return false;
	}
	pRecord->pGeneration = pGeneration;
	if ( xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
		XS_TcpRecordRelease(pRecord);
		return false;
	}
	pScript = XS_ScriptAcquireHost(pRuntime->pHost);
	if ( pScript == NULL ) {
		XS_TcpRecordRelease(pRecord);
		return false;
	}
	pRecord->pHost = pRuntime->pHost;
	pRecord->pTls = pStream;
	pRecord->pScript = pScript;
	if ( !XS_RegistryAdd(&pRuntime->tRegistry, pRecord) ) {
		XS_TcpRecordRelease(pRecord);
		return false;
	}
	if ( !xrtTlsStreamSetEvents(pStream, &g_XS_TlsStreamEvents, pRecord) ) {
		XS_RegistryRemove(&pRuntime->tRegistry, pRecord);
		XS_TcpRecordRelease(pRecord);
		return false;
	}
	return true;
}

static void XS_TlsOnListenerClose(xtlslistener* pListener, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;

	XS_ListenerSlotResourceClose(pSlot, XS_LISTENER_RESOURCE_TLS);
	xrtTlsListenerDestroy(pListener);
}

static const xtlslistenerevents g_XS_TlsListenerEvents = {
	XS_TlsOnAccept,
	XS_TlsHandshakeError,
	NULL,			/* Error */
	XS_TlsOnListenerClose
};

/* —— idle 扫描（引擎定时器，周期 = min(idle/2, 1s)）—— */

static void XS_TcpSweepProc(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_TcpRuntime* pRuntime = (XS_TcpRuntime*)pData;
	XS_ServerGeneration* pGeneration = XS_GenerationTimerFinish(&pRuntime->tSweepTimer);

	(void)pWorker; (void)iId;
	if ( pGeneration == NULL ) return;
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
			if ( XS_GenerationTimerSchedule(pRuntime->pGeneration,
				iInterval * 1000, XS_TcpSweepProc, pData,
				pRuntime, &pRuntime->tSweepTimer) != 0 &&
			     xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
				XS_GenerationTimerCancelOwner(pRuntime->pGeneration, pRuntime);
			}
		}
	}
	XS_GenerationActivityRelease(pGeneration);
	XS_GenerationRelease(pGeneration);
}

static bool XS_TcpScheduleSweep(XS_TcpRuntime* pRuntime)
{
	uint64 iInterval = pRuntime->iIdleMs / 2;

	if ( iInterval > 1000 ) iInterval = 1000;
	if ( iInterval < 10 ) iInterval = 10;
	return XS_GenerationTimerSchedule(pRuntime->pGeneration,
		iInterval * 1000, XS_TcpSweepProc, pRuntime,
		pRuntime, &pRuntime->tSweepTimer) != 0;
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
	xnetaddr tTlsAddr;
	uint64 iIdle = 0;

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
	if ( pServer->TLS && !xrtNetAddrParse(&tTlsAddr,
		pServer->IPTLS ? pServer->IPTLS : (pServer->IP ? pServer->IP : "0.0.0.0"),
		pServer->PortTLS) ) {
		snprintf(sErr, iErrCap, "tcps server '%s' tls addr parse failed", pServer->Name);
		return false;
	}
	if ( !XS_RegistryInit(&pRuntime->tRegistry) ) {
		snprintf(sErr, iErrCap, "registry init failed");
		return false;
	}
	if ( !XS_CustomReadUInt(pServer->Custom, "idle_timeout", &iIdle,
		sErr, iErrCap) ) return false;
	pRuntime->iIdleMs = iIdle;

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

	if ( bStartEndpoint ) {
		xnetlistenconfig tListen;
		xnetlistener* pListener;

		xrtNetListenConfigInit(&tListen);
		tListen.Address = tAddr;
		tListen.ReuseAddress = true;	/* Windows 默认独占，快速重绑 */
		tListen.ExclusiveAddress = false;	/* 与 ReuseAddress 互斥（配置校验） */
		if ( pServer->Backlog > 0 ) {
			tListen.Backlog = (int)pServer->Backlog;
		}
		XS_StreamApplyReceiveLimit(&tListen.Stream, pServer->RecvLimit);
		if ( !XS_ListenerSlotResourceAdd(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_PLAIN) ) {
			return false;
		}
		pListener = xrtNetListen(pServer->Engine, &tListen,
			&g_XS_TcpListenerEvents, &g_XS_TcpStreamEvents, pRuntime->pListenerSlot);
		if ( pListener == NULL ) {
			XS_ListenerSlotResourceCancel(pRuntime->pListenerSlot,
				XS_LISTENER_RESOURCE_PLAIN);
			XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
			pRuntime->pListenerSlot = NULL;
			const xerror* pErr = xrtGetError();
			snprintf(sErr, iErrCap, "tcp server '%s' listen failed (port %u): %s", pServer->Name,
				pServer->Port, pErr != NULL ? xrtErrorMessage(pErr) : "unknown");
			return false;
		}
		if ( !XS_ListenerSlotResourceAttach(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_PLAIN, pListener) ) {
			XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
			pRuntime->pListenerSlot = NULL;
			snprintf(sErr, iErrCap, "tcp server '%s' listener closed during start",
				pServer->Name);
			return false;
		}
	}
	if ( bStartEndpoint && pServer->TLS ) {
		xtlslistenerconfig tTlsListen;
		xtlscontext* pContext = XS_TlsSharedContext();
		xtlslistener* pTlsListener;

		xrtTlsListenerConfigInit(&tTlsListen);
		tTlsListen.Listen.Address = tTlsAddr;
		tTlsListen.Listen.ReuseAddress = true;
		tTlsListen.Listen.ExclusiveAddress = false;
		if ( pServer->Backlog > 0 ) {
			tTlsListen.Listen.Backlog = (int)pServer->Backlog;
		}
		XS_StreamApplyReceiveLimit(&tTlsListen.Listen.Stream, pServer->RecvLimit);
		tTlsListen.Tls.Context = pContext;
		tTlsListen.Tls.Identity = pRuntime->tTls.pEntries[0].pIdentity;	/* 无 SNI 回落 */
		tTlsListen.Tls.Select = XS_TlsSlotSelect;
		tTlsListen.Tls.SelectContext = pRuntime->pListenerSlot;
		if ( !XS_ListenerSlotResourceAdd(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_TLS) ) {
			return false;
		}
		pTlsListener = xrtTlsListenerStart(pServer->Engine, &tTlsListen,
			&g_XS_TlsListenerEvents, NULL, pRuntime->pListenerSlot);
		if ( pTlsListener == NULL ) {
			XS_ListenerSlotResourceCancel(pRuntime->pListenerSlot,
				XS_LISTENER_RESOURCE_TLS);
			snprintf(sErr, iErrCap, "tcps server '%s' listen failed (port %u)", pServer->Name, pServer->PortTLS);
			return false;
		}
		if ( !XS_ListenerSlotResourceAttach(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_TLS, pTlsListener) ) {
			snprintf(sErr, iErrCap, "tcps server '%s' listener closed during start",
				pServer->Name);
			return false;
		}
	}

	if ( pRuntime->iIdleMs > 0 ) {
		if ( !XS_TcpScheduleSweep(pRuntime) ) {
			snprintf(sErr, iErrCap, "tcp server '%s' idle timer start failed", pServer->Name);
			return false;
		}
	}
	printf("[xs] server '%s' tcp%s %s on %s:%u", pServer->Name,
		pRuntime->bTls ? "+tcps" : "",
		bStartEndpoint ? (bAcceptEndpoint ? "ready" : "bound") : "prepared",
		pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port);
	if ( pRuntime->bTls ) {
		printf(" and %s:%u", pServer->IPTLS ? pServer->IPTLS :
			(pServer->IP ? pServer->IP : "0.0.0.0"), pServer->PortTLS);
	}
	printf("%s\n", pRuntime->iIdleMs > 0 ? " (idle protected)" : "");
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
	pOld->pListenerSlot = NULL;
	return true;
}

static void XS_TcpStop(XS_TcpRuntime* pRuntime)
{
	XS_ListenerSlot* pSlot;
	XS_ListenerResources tResources;

	if ( pRuntime == NULL ) {
		return;
	}
	if ( xrtAtomic32Exchange(&pRuntime->tStopping, 1, XMEMORY_ACQ_REL) != 0 ) return;
	XS_GenerationTimerCancelOwner(pRuntime->pGeneration, pRuntime);
	XS_RegistryStopAccepting(&pRuntime->tRegistry);
	pSlot = pRuntime->pListenerSlot;
	pRuntime->pListenerSlot = NULL;
	if ( pSlot != NULL && XS_ListenerSlotBeginClose(pSlot, pRuntime, &tResources) ) {
		if ( tResources.pPlain != NULL ) {
			xrtNetListenerClose(tResources.pPlain);
			xrtNetListenerDestroy(tResources.pPlain);
		}
		if ( tResources.pTls != NULL ) {
			xrtTlsListenerClose(tResources.pTls);
			xrtTlsListenerDestroy(tResources.pTls);
		}
	}
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
