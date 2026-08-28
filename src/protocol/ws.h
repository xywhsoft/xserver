#ifndef XS_PROTOCOL_WS_H
#define XS_PROTOCOL_WS_H

/*
 * xs3 WebSocket 驱动（设计 §6.2）
 * ws 建监听（含 wss），HTTP 升级移交范式：
 *   ParseBuffer → xrtWsUpgradeRequestCheck → 101（ResponseWrite）
 *   → xrtWsStreamAttach(prefix=head.Bytes) → 脚本 Ws* 回调
 * 消息按整条投递（MessageBegin/Data/End 累积，上限 ws_message_limit）。
 * 连接生命周期由 xs 持有（Close 时 WsStreamDestroy）。
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"
#include "../core/config.h"
#include "../core/engine.h"
#include "../core/tls.h"
#include "../script/script.h"
#include "http.h"

#define XS_WS_MAX_FIELDS	16

typedef struct XS_WsRuntime {
	XS_ServerInfo*		pServer;
	XS_HostInfo*		pHost;
	bool			bTls;
	xnetlistener*		pListener;
	xtlslistener*		pTlsListener;
	XS_TlsTable		tTls;
	xhttp1limits		tLimits;
	str			sProtocol;	/* ws_protocol 旋钮（可空） */
	uint64			iMessageLimit;	/* 0 = 内核默认 */
	struct XS_WsConn*	pConns;		/* 活动连接链（停机批量收口） */
	uint32			iConnCount;
	volatile bool		bStopping;
} XS_WsRuntime;

typedef struct XS_WsConn {
	struct XS_WsConn*	pNext;
	XS_WsRuntime*		pRuntime;
	XS_HostInfo*		pHost;
	xnetstream*		pTcp;
	xtlsstream*		pTls;
	xwsstream*		pWs;
	/* 握手期请求头 */
	xhttp1head		tHead;
	xhttpfield		arrFields[XS_WS_MAX_FIELDS];
	/* 消息累积器 */
	unsigned char*		pMsg;
	size_t			iMsgSize;
	size_t			iMsgCap;
	uint8			iMsgOpcode;
	bool			bHandshakeDone;
} XS_WsConn;

static void XS_WsListAdd(XS_WsRuntime* pRuntime, XS_WsConn* pConn)
{
	pConn->pNext = pRuntime->pConns;
	pRuntime->pConns = pConn;
	pRuntime->iConnCount++;
}

static void XS_WsListRemove(XS_WsRuntime* pRuntime, XS_WsConn* pConn)
{
	XS_WsConn** ppLink;

	for ( ppLink = &pRuntime->pConns; *ppLink != NULL; ppLink = &(*ppLink)->pNext ) {
		if ( *ppLink == pConn ) {
			*ppLink = pConn->pNext;
			pRuntime->iConnCount--;
			return;
		}
	}
}

/* ============================================================
 * 握手：HTTP 请求 → 101 → Attach
 * ============================================================ */

static bool XS_WsSendRaw(XS_WsConn* pConn, const void* pData, size_t iSize)
{
	size_t iWritten = 0;

	if ( pConn->pTls != NULL ) {
		return xrtTlsStreamSend(pConn->pTls, pData, iSize, &iWritten) == XTLS_OK &&
		       iWritten == iSize;
	}
	return xrtNetStreamSend(pConn->pTcp, pData, iSize) == XNET_RESULT_OK;
}

static void XS_WsReject(XS_WsConn* pConn, uint16 iStatus)
{
	char arrHead[512];
	xhttpfield arrFields[1];
	size_t iSize = 0;

	arrFields[0].Name = XRT_STR_LITERAL("Content-Length");
	arrFields[0].Value = XRT_STR_LITERAL("0");
	if ( xrtHttp1ResponseWrite(XHTTP_VERSION_1_1, iStatus, xrtHttpStatusText(iStatus),
		arrFields, 1, arrHead, sizeof(arrHead), &iSize) ) {
		(void)XS_WsSendRaw(pConn, arrHead, iSize);
	}
	if ( pConn->pTls != NULL ) {
		(void)xrtTlsStreamClose(pConn->pTls);
	} else {
		(void)xrtNetStreamClose(pConn->pTcp);
	}
}

static const xwsstreamevents g_XS_WsStreamEvents;	/* 前向声明（定义在事件段） */

/* 完成 101 并移交 WS 流；失败返回 false（连接已按错误关闭由调用方处理） */
static bool XS_WsUpgrade(XS_WsConn* pConn)
{
	XS_WsRuntime* pRuntime = pConn->pRuntime;
	xwsupgradeserverconfig tSrvCfg;
	xwsupgrade tUpgrade;
	xhttpfield arrFields[4];
	size_t iCount = 0;
	char arrHead[512];
	size_t iSize = 0;
	xwsstreamconfig tStreamCfg;
	xwsstream* pWs;

	memset(&tSrvCfg, 0, sizeof(tSrvCfg));
	if ( pRuntime->sProtocol != NULL ) {
		tSrvCfg.Protocols = xrtStrView(pRuntime->sProtocol);
	}
	if ( !xrtWsUpgradeRequestCheck(&pConn->tHead, &tSrvCfg, &tUpgrade) ) {
		XS_WsReject(pConn, 400);
		return false;
	}
	if ( !xrtWsUpgradeResponseFields(xrtStrViewN(tUpgrade.Accept, strlen(tUpgrade.Accept)),
		tUpgrade.Protocol, xrtStrViewN(tUpgrade.Extensions, tUpgrade.ExtensionSize),
		arrFields, 4, &iCount) ||
	     !xrtHttp1ResponseWrite(XHTTP_VERSION_1_1, 101, XRT_STR_LITERAL("Switching Protocols"),
		arrFields, iCount, arrHead, sizeof(arrHead), &iSize) ||
	     !XS_WsSendRaw(pConn, arrHead, iSize) ) {
		XS_WsReject(pConn, 500);
		return false;
	}

	xrtWsStreamConfigInit(&tStreamCfg);
	if ( !xrtWsUpgradeStreamConfig(&tStreamCfg, XWS_ROLE_SERVER, &tUpgrade) ) {
		XS_WsReject(pConn, 500);
		return false;
	}
	if ( pRuntime->iMessageLimit > 0 ) {
		tStreamCfg.MessageLimit = pRuntime->iMessageLimit;
	}
	if ( pConn->pTls != NULL ) {
		pWs = xrtWsStreamAttachTls(pConn->pTls, pConn->tHead.Bytes,
			&tStreamCfg, &g_XS_WsStreamEvents, pConn);
	} else {
		pWs = xrtWsStreamAttach(pConn->pTcp, pConn->tHead.Bytes,
			&tStreamCfg, &g_XS_WsStreamEvents, pConn);
	}
	if ( pWs == NULL ) {
		return false;	/* 库已关闭传输 */
	}
	pConn->pWs = pWs;
	pConn->bHandshakeDone = true;
	{
		XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pConn->pHost->Runtime;

		if ( pScript != NULL && pScript->procWsOpen != NULL ) {
			pScript->procWsOpen(pConn->pHost, pWs);
		}
	}
	return true;
}

/* ============================================================
 * 传输事件（握手期）
 * ============================================================ */

static void XS_WsOnRead(xnetstream* pStream, xnetbuf* pBuffer, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	xhttp1errorinfo tErr;

	(void)pStream; (void)pBuffer;
	if ( pConn->bHandshakeDone ) {
		return;		/* 已移交 WS 层，不该再收到 */
	}
	switch ( xrtHttp1RequestParseBuffer((xnetbuf*)xrtNetStreamBuffer(pConn->pTcp),
		&pConn->tHead, &pConn->pRuntime->tLimits, &tErr) ) {
	case XHTTP1_MORE:
		return;
	case XHTTP1_ERROR:
		XS_WsReject(pConn, 400);
		return;
	default:
		break;
	}
	(void)XS_WsUpgrade(pConn);
}

static void XS_WsOnClose(xnetstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;

	(void)iResult; (void)pError;
	if ( !pConn->bHandshakeDone ) {
		xrtNetStreamDestroy(pStream);
		XS_WsListRemove(pConn->pRuntime, pConn);
		xrtFree(pConn->pMsg);
		xrtFree(pConn);
	}
	/* 已握手：由 WS 层 Close 事件统一回收 */
}

static const xnetstreamevents g_XS_WsTransportEvents = {
	NULL, XS_WsOnRead, NULL, NULL, NULL, NULL, XS_WsOnClose
};

static void XS_WsTlsOnRead(xtlsstream* pStream, const xnetbuf* pBuffer, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	xhttp1errorinfo tErr;

	(void)pStream; (void)pBuffer;
	if ( pConn->bHandshakeDone ) {
		return;
	}
	switch ( xrtHttp1RequestParseTls(pConn->pTls, &pConn->tHead, &pConn->pRuntime->tLimits, &tErr) ) {
	case XHTTP1_MORE:
		return;
	case XHTTP1_ERROR:
		XS_WsReject(pConn, 400);
		return;
	default:
		break;
	}
	(void)XS_WsUpgrade(pConn);
}

static void XS_WsTlsOnClose(xtlsstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;

	(void)iResult; (void)pError;
	if ( !pConn->bHandshakeDone ) {
		xrtTlsStreamDestroy(pStream);
		XS_WsListRemove(pConn->pRuntime, pConn);
		xrtFree(pConn->pMsg);
		xrtFree(pConn);
	}
}

static const xtlsstreamevents g_XS_WsTlsTransportEvents = {
	NULL, XS_WsTlsOnRead, NULL, NULL, NULL, XS_WsTlsOnClose, NULL
};

/* ============================================================
 * WS 流事件（消息整条投递）
 * ============================================================ */

static void XS_WsOnMessageBegin(xwsstream* pStream, const xwsmessageinfo* pInfo, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;

	(void)pStream;
	pConn->iMsgSize = 0;
	pConn->iMsgOpcode = pInfo->Opcode;
}

static void XS_WsOnMessageData(xwsstream* pStream, xbytesview Data, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	uint64 iLimit = pConn->pRuntime->iMessageLimit;

	(void)pStream;
	if ( iLimit > 0 && pConn->iMsgSize + Data.Size > iLimit ) {
		(void)xrtWsStreamClose(pStream, 1009, XRT_STR_LITERAL("message too big"));
		return;
	}
	if ( pConn->iMsgSize + Data.Size > pConn->iMsgCap ) {
		size_t iNewCap = pConn->iMsgCap * 2;
		unsigned char* pNew;

		if ( iNewCap < 4096 ) iNewCap = 4096;
		while ( iNewCap < pConn->iMsgSize + Data.Size ) {
			iNewCap *= 2;
		}
		pNew = (unsigned char*)xrtRealloc(pConn->pMsg, iNewCap);
		if ( pNew == NULL ) {
			(void)xrtWsStreamClose(pStream, 1011, XRT_STR_LITERAL("oom"));
			return;
		}
		pConn->pMsg = pNew;
		pConn->iMsgCap = iNewCap;
	}
	memcpy(pConn->pMsg + pConn->iMsgSize, Data.Data, Data.Size);
	pConn->iMsgSize += Data.Size;
}

static void XS_WsOnMessageEnd(xwsstream* pStream, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pConn->pHost->Runtime;

	if ( pScript != NULL ) {
		if ( pConn->iMsgOpcode == XWS_OPCODE_TEXT && pScript->procWsText != NULL ) {
			pScript->procWsText(pConn->pHost, pStream,
				xrtStrViewN((cstr)pConn->pMsg, pConn->iMsgSize));
		} else if ( pConn->iMsgOpcode == XWS_OPCODE_BINARY && pScript->procWsBinary != NULL ) {
			xbytesview tData;

			tData.Data = pConn->pMsg;
			tData.Size = pConn->iMsgSize;
			pScript->procWsBinary(pConn->pHost, pStream, tData);
		}
	}
	pConn->iMsgSize = 0;
}

static void XS_WsOnPing(xwsstream* pStream, xbytesview Payload, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pConn->pHost->Runtime;

	if ( pScript != NULL && pScript->procWsPing != NULL ) {
		pScript->procWsPing(pConn->pHost, pStream, Payload);
	}
}

static void XS_WsOnPong(xwsstream* pStream, xbytesview Payload, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pConn->pHost->Runtime;

	if ( pScript != NULL && pScript->procWsPong != NULL ) {
		pScript->procWsPong(pConn->pHost, pStream, Payload);
	}
}

static void XS_WsOnWsClose(xwsstream* pStream, const xwsstreamclose* pClose, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pConn->pHost->Runtime;

	if ( pScript != NULL && pScript->procWsClose != NULL ) {
		uint16 iCode = 0;
		xstrview tReason = {0};

		if ( pClose != NULL ) {
			iCode = pClose->RemoteCode != 0 ? pClose->RemoteCode : pClose->LocalCode;
			tReason = pClose->Reason;
		}
		pScript->procWsClose(pConn->pHost, pStream, iCode, tReason);
	}
	xrtWsStreamDestroy(pStream);	/* 附带关闭并回收底层传输 */
	XS_WsListRemove(pConn->pRuntime, pConn);
	xrtFree(pConn->pMsg);
	xrtFree(pConn);
}

static const xwsstreamevents g_XS_WsStreamEvents = {
	XS_WsOnMessageBegin,
	XS_WsOnMessageData,
	XS_WsOnMessageEnd,
	XS_WsOnPing,
	XS_WsOnPong,
	NULL,			/* Backpressure */
	NULL,			/* Writable */
	NULL,			/* Drain */
	NULL,			/* Error */
	XS_WsOnWsClose
};

/* ============================================================
 * 接入 shim
 * ============================================================ */

static bool XS_WsOnAccept(xnetlistener* pListener, xnetstream* pStream, ptr pData)
{
	XS_WsRuntime* pRuntime = (XS_WsRuntime*)pData;
	XS_WsConn* pConn = (XS_WsConn*)xrtCalloc(1, sizeof(XS_WsConn));

	(void)pListener;
	if ( pConn == NULL ) {
		return false;
	}
	pConn->pRuntime = pRuntime;
	pConn->pHost = pRuntime->pHost;
	pConn->pTcp = pStream;
	xrtHttp1HeadInit(&pConn->tHead, pConn->arrFields, XS_WS_MAX_FIELDS);
	XS_WsListAdd(pRuntime, pConn);
	(void)xrtNetStreamSetData(pStream, pConn);
	return true;
}

static void XS_WsOnListenerClose(xnetlistener* pListener, ptr pData)
{
	(void)pData;
	xrtNetListenerDestroy(pListener);
}

static const xnetlistenerevents g_XS_WsListenerEvents = {
	XS_WsOnAccept, NULL, XS_WsOnListenerClose
};

static bool XS_WsTlsOnAccept(xtlslistener* pListener, xtlsstream* pStream, ptr pData)
{
	XS_WsRuntime* pRuntime = (XS_WsRuntime*)pData;
	XS_WsConn* pConn = (XS_WsConn*)xrtCalloc(1, sizeof(XS_WsConn));

	(void)pListener;
	if ( pConn == NULL ) {
		return false;
	}
	pConn->pRuntime = pRuntime;
	pConn->pHost = pRuntime->pHost;
	pConn->pTls = pStream;
	xrtHttp1HeadInit(&pConn->tHead, pConn->arrFields, XS_WS_MAX_FIELDS);
	XS_WsListAdd(pRuntime, pConn);
	(void)xrtTlsStreamSetEvents(pStream, &g_XS_WsTlsTransportEvents, pConn);
	return true;
}

static void XS_WsTlsOnListenerClose(xtlslistener* pListener, ptr pData)
{
	(void)pData;
	xrtTlsListenerDestroy(pListener);
}

static const xtlslistenerevents g_XS_WsTlsListenerEvents = {
	XS_WsTlsOnAccept, NULL, NULL, XS_WsTlsOnListenerClose
};

/* ============================================================
 * 启动 / 停止
 * ============================================================ */

static bool XS_WsStart(XS_ServerInfo* pServer, char* sErr, size_t iErrCap)
{
	XS_WsRuntime* pRuntime = (XS_WsRuntime*)xrtCalloc(1, sizeof(XS_WsRuntime));
	xhttp1limits tDef;
	xnetaddr tAddr;
	int64 iVal = 0;

	if ( pRuntime == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	pRuntime->pServer = pServer;
	pRuntime->pHost = pServer->DefaultHost;
	pServer->Runtime = pRuntime;

	{
		XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pServer->DefaultHost->Runtime;

		if ( pScript == NULL || (pScript->procWsText == NULL && pScript->procWsBinary == NULL) ) {
			snprintf(sErr, iErrCap, "ws server '%s' requires script exporting WsText/WsBinary",
				pServer->Name);
			return false;
		}
	}
	if ( !xrtNetAddrParse(&tAddr, pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port) ) {
		snprintf(sErr, iErrCap, "ws server '%s' addr parse failed", pServer->Name);
		return false;
	}
	xrtHttp1LimitsInit(&tDef);
	pRuntime->tLimits = tDef;
	(void)XS_CustomGetInt(pServer->Custom, "ws_message_limit", &iVal);
	pRuntime->iMessageLimit = (iVal > 0) ? (uint64)iVal : 0;
	{
		const char* sProto = NULL;

		if ( pServer->Custom != NULL ) {
			xvalue* pVal = xrtValueObjectGet(pServer->Custom,
				xrtStrViewN("ws_protocol", strlen("ws_protocol")));
			xstrview tView;

			if ( pVal != NULL && xrtValueGetString(pVal, &tView) ) {
				pRuntime->sProtocol = xrtStrDupN(tView.Data, tView.Size);
			}
		}
		(void)sProto;
	}

	if ( !pServer->TLS ) {
		xnetlistenconfig tListen;

		xrtNetListenConfigInit(&tListen);
		tListen.Address = tAddr;
		tListen.ReuseAddress = true;
		tListen.ExclusiveAddress = false;
		if ( pServer->Backlog > 0 ) {
			tListen.Backlog = (int)pServer->Backlog;
		}
		if ( pServer->RecvLimit > 0 ) {
			tListen.Stream.ReadLimit = pServer->RecvLimit;
		}
		pRuntime->pListener = xrtNetListen(pServer->Engine, &tListen,
			&g_XS_WsListenerEvents, &g_XS_WsTransportEvents, pRuntime);
		if ( pRuntime->pListener == NULL ) {
			const xerror* pErr = xrtGetError();
			snprintf(sErr, iErrCap, "ws server '%s' listen failed (port %u): %s",
				pServer->Name, pServer->Port, pErr ? xrtErrorMessage(pErr) : "unknown");
			return false;
		}
	} else {
		xtlslistenerconfig tTlsListen;
		xtlscontext* pContext = XS_TlsSharedContext();

		if ( pContext == NULL || !XS_TlsTableBuild(pServer, &pRuntime->tTls, sErr, iErrCap) ||
		     pRuntime->tTls.iCount == 0 ) {
			snprintf(sErr + strlen(sErr), iErrCap - strlen(sErr),
				"wss server '%s' has no usable tls identity", pServer->Name);
			return false;
		}
		xrtNetListenConfigInit(&tTlsListen.Listen);
		tTlsListen.Listen.Address = tAddr;
		tTlsListen.Listen.ReuseAddress = true;
		tTlsListen.Listen.ExclusiveAddress = false;
		if ( pServer->Backlog > 0 ) {
			tTlsListen.Listen.Backlog = (int)pServer->Backlog;
		}
		xrtTlsServerConfigInit(&tTlsListen.Tls);
		tTlsListen.Tls.Context = pContext;
		tTlsListen.Tls.Identity = pRuntime->tTls.pEntries[0].pIdentity;
		tTlsListen.Tls.Select = XS_TlsSelect;
		tTlsListen.Tls.SelectContext = &pRuntime->tTls;
		pRuntime->bTls = true;
		pRuntime->pTlsListener = xrtTlsListenerStart(pServer->Engine, &tTlsListen,
			&g_XS_WsTlsListenerEvents, &g_XS_WsTlsTransportEvents, pRuntime);
		if ( pRuntime->pTlsListener == NULL ) {
			XS_TlsTableUnit(&pRuntime->tTls);
			snprintf(sErr, iErrCap, "wss server '%s' listen failed (port %u)", pServer->Name, pServer->Port);
			return false;
		}
	}
	printf("[xs] server '%s' %s ready on %s:%u%s\n", pServer->Name,
		pRuntime->bTls ? "wss" : "ws",
		pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port,
		pRuntime->sProtocol != NULL ? " (subprotocol required)" : "");
	return true;
}

static void XS_WsStop(XS_WsRuntime* pRuntime)
{
	XS_WsConn* pConn;

	if ( pRuntime == NULL ) {
		return;
	}
	pRuntime->bStopping = true;
	pConn = pRuntime->pConns;
	while ( pConn != NULL ) {
		XS_WsConn* pNext = pConn->pNext;

		if ( pConn->pWs != NULL ) {
			(void)xrtWsStreamClose(pConn->pWs, 1001, XRT_STR_LITERAL("server shutdown"));
		} else if ( pConn->pTls != NULL ) {
			(void)xrtTlsStreamClose(pConn->pTls);
		} else if ( pConn->pTcp != NULL ) {
			(void)xrtNetStreamClose(pConn->pTcp);
		}
		pConn = pNext;
	}
	if ( pRuntime->pListener != NULL ) {
		xrtNetListenerClose(pRuntime->pListener);
		pRuntime->pListener = NULL;
	}
	if ( pRuntime->pTlsListener != NULL ) {
		xrtTlsListenerClose(pRuntime->pTlsListener);
		pRuntime->pTlsListener = NULL;
	}
}

static void XS_WsUnit(XS_WsRuntime* pRuntime)
{
	if ( pRuntime == NULL ) {
		return;
	}
	xrtFree(pRuntime->sProtocol);
	XS_TlsTableUnit(&pRuntime->tTls);
	xrtFree(pRuntime);
}

#endif
