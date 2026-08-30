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
#include "../runtime/vhost.h"
#include "../script/script.h"
#include "http.h"

#define XS_WS_MAX_FIELDS		16
#define XS_WS_MESSAGE_RETAIN_LIMIT	(256 * 1024)

typedef struct XS_WsRuntime {
	XS_ServerInfo*		pServer;
	bool			bTls;
	XS_ListenerSlot*	pListenerSlot;
	XS_TlsTable		tTls;
	XS_VHostTable		tVHosts;
	xhttp1limits		tLimits;
	size_t			iReceiveLimit;	/* 握手期线路硬边界，防止 ReadLimit 满后永久 MORE */
	str			sProtocol;	/* ws_protocol 旋钮（可空） */
	uint64			iMessageLimit;	/* 0 = 内核默认 */
	uint64			iIdleMs;	/* 0 = 关闭 idle 保护 */
	XS_GenerationTimer	tSweepTimer;
	struct XS_WsConn*	pConns;		/* 活动连接链（停机批量收口） */
	uint32			iConnCount;
	xmutex*			pConnLock;
	XS_ServerGeneration*	pGeneration;
	xatomic32		tStopping;
} XS_WsRuntime;

typedef struct XS_WsConn {
	struct XS_WsConn*	pNext;
	volatile int32		iReferences;	/* 连接所有权 1 + 正在执行的协议回调 */
	XS_WsRuntime*		pRuntime;
	XS_HostInfo*		pHost;
	XS_ServerGeneration*	pGeneration;
	XS_ScriptRuntime*	pScript;
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
	xatomic64		tLastActive;	/* xrtNow() 微秒 */
	bool			bUpgradeStarted;
	bool			bHandshakeDone;
	bool			bMessageFailed;
} XS_WsConn;

static void XS_WsTouch(XS_WsConn* pConn)
{
	if ( pConn != NULL ) {
		xrtAtomic64Store(&pConn->tLastActive, (uint64)xrtNow(), XMEMORY_RELAXED);
	}
}

static bool XS_WsListAdd(XS_WsRuntime* pRuntime, XS_WsConn* pConn)
{
	bool bAdded = false;

	xrtMutexLock(pRuntime->pConnLock);
	if ( xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) == 0 ) {
		pConn->pNext = pRuntime->pConns;
		pRuntime->pConns = pConn;
		pRuntime->iConnCount++;
		bAdded = true;
	}
	xrtMutexUnlock(pRuntime->pConnLock);
	return bAdded;
}

static void XS_WsListRemove(XS_WsRuntime* pRuntime, XS_WsConn* pConn)
{
	XS_WsConn** ppLink;

	xrtMutexLock(pRuntime->pConnLock);
	for ( ppLink = &pRuntime->pConns; *ppLink != NULL; ppLink = &(*ppLink)->pNext ) {
		if ( *ppLink == pConn ) {
			*ppLink = pConn->pNext;
			pRuntime->iConnCount--;
			break;
		}
	}
	xrtMutexUnlock(pRuntime->pConnLock);
}

static bool XS_WsConnRetain(XS_WsConn* pConn)
{
	return pConn != NULL && xrtRefRetain(&pConn->iReferences) > 0;
}

static void XS_WsConnRelease(XS_WsConn* pConn)
{
	XS_ScriptRuntime* pScript;
	XS_ServerGeneration* pGeneration;

	if ( pConn == NULL || xrtRefRelease(&pConn->iReferences) != 0 ) return;
	pScript = pConn->pScript;
	pGeneration = pConn->pGeneration;
	xrtFree(pConn->pMsg);
	xrtFree(pConn);
	XS_ScriptRelease(pScript);
	XS_GenerationConnectionRelease(pGeneration);
}

static bool XS_WsNetCallbackEnter(XS_WsConn* pConn, xnetstream* pStream)
{
	if ( !XS_WsConnRetain(pConn) ) return false;
	if ( xrtNetStreamRef(pStream) != NULL ) return true;
	XS_WsConnRelease(pConn);
	return false;
}

static void XS_WsNetCallbackLeave(XS_WsConn* pConn, xnetstream* pStream)
{
	xrtNetStreamDestroy(pStream);
	XS_WsConnRelease(pConn);
}

static bool XS_WsTlsCallbackEnter(XS_WsConn* pConn, xtlsstream* pStream)
{
	if ( !XS_WsConnRetain(pConn) ) return false;
	if ( xrtTlsStreamRef(pStream) != NULL ) return true;
	XS_WsConnRelease(pConn);
	return false;
}

static void XS_WsTlsCallbackLeave(XS_WsConn* pConn, xtlsstream* pStream)
{
	xrtTlsStreamDestroy(pStream);
	XS_WsConnRelease(pConn);
}

static bool XS_WsStreamCallbackEnter(XS_WsConn* pConn, xwsstream* pStream)
{
	if ( !XS_WsConnRetain(pConn) ) return false;
	if ( xrtWsStreamRef(pStream) != NULL ) return true;
	XS_WsConnRelease(pConn);
	return false;
}

static void XS_WsStreamCallbackLeave(XS_WsConn* pConn, xwsstream* pStream)
{
	xrtWsStreamDestroy(pStream);
	XS_WsConnRelease(pConn);
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
	xhttpversion eVersion = pConn->tHead.Version == XHTTP_VERSION_1_0 ?
		XHTTP_VERSION_1_0 : XHTTP_VERSION_1_1;

	arrFields[0].Name = XRT_STR_LITERAL("Content-Length");
	arrFields[0].Value = XRT_STR_LITERAL("0");
	if ( xrtHttp1ResponseWrite(eVersion, iStatus, xrtHttpStatusText(iStatus),
		arrFields, 1, arrHead, sizeof(arrHead), &iSize) ) {
		(void)XS_WsSendRaw(pConn, arrHead, iSize);
	}
	if ( pConn->pTls != NULL ) {
		(void)xrtTlsStreamClose(pConn->pTls);
	} else {
		(void)xrtNetStreamClose(pConn->pTcp);
	}
}

static size_t XS_WsHandshakeAvailable(XS_WsConn* pConn)
{
	const xnetbuf* pBuffer = pConn->pTls != NULL ?
		xrtTlsStreamBuffer(pConn->pTls) : xrtNetStreamBuffer(pConn->pTcp);

	return pBuffer != NULL ? xrtNetBufSize(pBuffer) : 0;
}

static bool XS_WsConsumeHandshake(XS_WsConn* pConn)
{
	if ( pConn->pTls != NULL ) {
		return xrtTlsStreamConsume(pConn->pTls, pConn->tHead.Bytes);
	}
	return xrtNetStreamConsume(pConn->pTcp, pConn->tHead.Bytes) ==
		pConn->tHead.Bytes;
}

static const xwsstreamevents g_XS_WsStreamEvents;	/* 前向声明（定义在事件段） */

/* 完成 101 并移交 WS 流；失败返回 false（连接已按错误关闭由调用方处理） */
static bool XS_WsUpgrade(XS_WsConn* pConn)
{
	XS_WsRuntime* pRuntime = pConn->pRuntime;
	XS_HostInfo* pHost = NULL;
	XS_ScriptRuntime* pScript = NULL;
	XS_VHostResult eRoute;
	xwsupgradeserverconfig tSrvCfg;
	xwsupgrade tUpgrade;
	xhttpfield arrFields[4];
	size_t iCount = 0;
	char arrHead[512];
	size_t iSize = 0;
	xwsstreamconfig tStreamCfg;
	xwsstream* pWs;
	xwsstream* pWsGuard;

	memset(&tSrvCfg, 0, sizeof(tSrvCfg));
	if ( pRuntime->sProtocol != NULL ) {
		tSrvCfg.Protocols = xrtStrView(pRuntime->sProtocol);
	}
	if ( !xrtWsUpgradeRequestCheck(&pConn->tHead, &tSrvCfg, &tUpgrade) ) {
		XS_WsReject(pConn, 400);
		return false;
	}
	eRoute = XS_VHostRouteHead(&pRuntime->tVHosts, &pConn->tHead, true, &pHost);
	if ( eRoute != XS_VHOST_FOUND ) {
		XS_WsReject(pConn, eRoute == XS_VHOST_BAD_REQUEST ? 400 : 404);
		return false;
	}
	pScript = XS_ScriptAcquireHost(pHost);
	if ( pScript == NULL ||
	     (pScript->procWsText == NULL && pScript->procWsBinary == NULL) ) {
		XS_ScriptRelease(pScript);
		XS_WsReject(pConn, 503);
		return false;
	}
	/* upgrade 是 WS 连接的唯一路由点；此后 host+脚本一直固定到 Close。 */
	pConn->pHost = pHost;
	pConn->pScript = pScript;
	if ( !xrtWsUpgradeResponseFields(xrtStrViewN(tUpgrade.Accept, strlen(tUpgrade.Accept)),
		tUpgrade.Protocol, xrtStrViewN(tUpgrade.Extensions, tUpgrade.ExtensionSize),
		arrFields, 4, &iCount) ||
	     !xrtHttp1ResponseWrite(XHTTP_VERSION_1_1, 101, XRT_STR_LITERAL("Switching Protocols"),
		arrFields, iCount, arrHead, sizeof(arrHead), &iSize) ) {
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
	/* 先消费 HTTP 前缀，再以 prefix=0 移交。这使 Attach 失败不会留下
	 * 已换事件却无 Close 回调的半接管传输。 */
	if ( !XS_WsConsumeHandshake(pConn) ) {
		XS_WsReject(pConn, 500);
		return false;
	}
	if ( pConn->pTls != NULL ) {
		pWs = xrtWsStreamAttachTls(pConn->pTls, 0,
			&tStreamCfg, &g_XS_WsStreamEvents, pConn);
	} else {
		pWs = xrtWsStreamAttach(pConn->pTcp, 0,
			&tStreamCfg, &g_XS_WsStreamEvents, pConn);
	}
	if ( pWs == NULL ) {
		XS_WsReject(pConn, 500);
		return false;
	}
	xrtMutexLock(pRuntime->pConnLock);
	pConn->pWs = pWs;
	pConn->bHandshakeDone = true;
	xrtMutexUnlock(pRuntime->pConnLock);
	pWsGuard = xrtWsStreamRef(pWs);
	if ( pWsGuard == NULL ) {
		(void)xrtWsStreamAbort(pWs);
		return false;
	}
	/* Attach 已成功后只能走 WS 终态；101 发送失败时不再发第二个 HTTP 响应。 */
	if ( !XS_WsSendRaw(pConn, arrHead, iSize) ) {
		(void)xrtWsStreamAbort(pWsGuard);
		xrtWsStreamDestroy(pWsGuard);
		return false;
	}
	{
		XS_ScriptRuntime* pScript = pConn->pScript;

		if ( pScript != NULL && pScript->procWsOpen != NULL ) {
			XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

			pScript->procWsOpen(pConn->pHost, pWs);
			XS_ScriptLeave(pPrevious);
		}
	}
	xrtWsStreamDestroy(pWsGuard);
	return true;
}

/* ============================================================
 * 传输事件（握手期）
 * ============================================================ */

static void XS_WsOnRead(xnetstream* pStream, xnetbuf* pBuffer, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	xhttp1errorinfo tErr;

	(void)pBuffer;
	if ( !XS_WsNetCallbackEnter(pConn, pStream) ) return;
	XS_WsTouch(pConn);
	if ( pConn->bHandshakeDone || pConn->bUpgradeStarted ) {
		goto Done;		/* 已移交 WS 层，不该再收到 */
	}
	switch ( xrtHttp1RequestParseBuffer((xnetbuf*)xrtNetStreamBuffer(pConn->pTcp),
		&pConn->tHead, &pConn->pRuntime->tLimits, &tErr) ) {
	case XHTTP1_MORE:
		if ( XS_WsHandshakeAvailable(pConn) >= pConn->pRuntime->iReceiveLimit ) {
			XS_WsReject(pConn, 431);
		}
		goto Done;
	case XHTTP1_ERROR:
		XS_WsReject(pConn,
			(tErr.Code == XHTTP1_ERROR_HEAD_TOO_LARGE ||
			 tErr.Code == XHTTP1_ERROR_START_LINE_TOO_LARGE ||
			 tErr.Code == XHTTP1_ERROR_FIELD_LINE_TOO_LARGE ||
			 tErr.Code == XHTTP1_ERROR_TOO_MANY_FIELDS) ? 431 : 400);
		goto Done;
	default:
		break;
	}
	pConn->bUpgradeStarted = true;
	(void)XS_WsUpgrade(pConn);
Done:
	XS_WsNetCallbackLeave(pConn, pStream);
}

static void XS_WsOnEnd(xnetstream* pStream, ptr pData)
{
	(void)pData;
	(void)xrtNetStreamClose(pStream);
}

static void XS_WsOnClose(xnetstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;

	(void)iResult; (void)pError;
	if ( !pConn->bHandshakeDone ) {
		XS_WsListRemove(pConn->pRuntime, pConn);
		xrtNetStreamDestroy(pStream);
		XS_WsConnRelease(pConn);
	}
	/* 已握手：由 WS 层 Close 事件统一回收 */
}

static const xnetstreamevents g_XS_WsTransportEvents = {
	NULL, XS_WsOnRead, XS_WsOnEnd, NULL, NULL, NULL, XS_WsOnClose
};

static void XS_WsTlsOnRead(xtlsstream* pStream, const xnetbuf* pBuffer, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	xhttp1errorinfo tErr;

	(void)pBuffer;
	if ( !XS_WsTlsCallbackEnter(pConn, pStream) ) return;
	XS_WsTouch(pConn);
	if ( pConn->bHandshakeDone || pConn->bUpgradeStarted ) {
		goto Done;
	}
	switch ( xrtHttp1RequestParseTls(pConn->pTls, &pConn->tHead, &pConn->pRuntime->tLimits, &tErr) ) {
	case XHTTP1_MORE:
		if ( XS_WsHandshakeAvailable(pConn) >= pConn->pRuntime->iReceiveLimit ) {
			XS_WsReject(pConn, 431);
		}
		goto Done;
	case XHTTP1_ERROR:
		XS_WsReject(pConn,
			(tErr.Code == XHTTP1_ERROR_HEAD_TOO_LARGE ||
			 tErr.Code == XHTTP1_ERROR_START_LINE_TOO_LARGE ||
			 tErr.Code == XHTTP1_ERROR_FIELD_LINE_TOO_LARGE ||
			 tErr.Code == XHTTP1_ERROR_TOO_MANY_FIELDS) ? 431 : 400);
		goto Done;
	default:
		break;
	}
	pConn->bUpgradeStarted = true;
	(void)XS_WsUpgrade(pConn);
Done:
	XS_WsTlsCallbackLeave(pConn, pStream);
}

static void XS_WsTlsOnEnd(xtlsstream* pStream, ptr pData)
{
	(void)pData;
	(void)xrtTlsStreamClose(pStream);
}

static void XS_WsTlsOnClose(xtlsstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;

	(void)iResult; (void)pError;
	if ( !pConn->bHandshakeDone ) {
		XS_WsListRemove(pConn->pRuntime, pConn);
		xrtTlsStreamDestroy(pStream);
		XS_WsConnRelease(pConn);
	}
}

static const xtlsstreamevents g_XS_WsTlsTransportEvents = {
	NULL, XS_WsTlsOnRead, XS_WsTlsOnEnd, NULL, NULL, XS_WsTlsOnClose, NULL
};

/* ============================================================
 * WS 流事件（消息整条投递）
 * ============================================================ */

static void XS_WsOnMessageBegin(xwsstream* pStream, const xwsmessageinfo* pInfo, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;

	if ( !XS_WsStreamCallbackEnter(pConn, pStream) ) return;
	XS_WsTouch(pConn);
	pConn->iMsgSize = 0;
	pConn->iMsgOpcode = pInfo->Opcode;
	pConn->bMessageFailed = false;
	XS_WsStreamCallbackLeave(pConn, pStream);
}

static void XS_WsOnMessageData(xwsstream* pStream, xbytesview Data, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	uint64 iLimit;
	size_t iNeeded;

	if ( !XS_WsStreamCallbackEnter(pConn, pStream) ) return;
	iLimit = pConn->pRuntime->iMessageLimit;
	XS_WsTouch(pConn);
	if ( pConn->bMessageFailed ) goto Done;
	if ( Data.Size > SIZE_MAX - pConn->iMsgSize ) {
		pConn->bMessageFailed = true;
		(void)xrtWsStreamClose(pStream, 1009, XRT_STR_LITERAL("message too big"));
		goto Done;
	}
	iNeeded = pConn->iMsgSize + Data.Size;
	if ( iLimit > 0 && ((uint64)pConn->iMsgSize > iLimit ||
	     (uint64)Data.Size > iLimit - (uint64)pConn->iMsgSize) ) {
		pConn->bMessageFailed = true;
		(void)xrtWsStreamClose(pStream, 1009, XRT_STR_LITERAL("message too big"));
		goto Done;
	}
	if ( iNeeded > pConn->iMsgCap ) {
		size_t iNewCap = pConn->iMsgCap > SIZE_MAX / 2 ?
			SIZE_MAX : pConn->iMsgCap * 2u;
		unsigned char* pNew;

		if ( iNewCap < 4096 ) iNewCap = 4096;
		while ( iNewCap < iNeeded ) {
			if ( iNewCap > SIZE_MAX / 2 ) {
				iNewCap = iNeeded;
				break;
			}
			iNewCap *= 2u;
		}
		pNew = (unsigned char*)xrtRealloc(pConn->pMsg, iNewCap);
		if ( pNew == NULL ) {
			pConn->bMessageFailed = true;
			(void)xrtWsStreamClose(pStream, 1011, XRT_STR_LITERAL("oom"));
			goto Done;
		}
		pConn->pMsg = pNew;
		pConn->iMsgCap = iNewCap;
	}
	if ( Data.Size != 0 ) memcpy(pConn->pMsg + pConn->iMsgSize, Data.Data, Data.Size);
	pConn->iMsgSize += Data.Size;
Done:
	XS_WsStreamCallbackLeave(pConn, pStream);
}

static void XS_WsOnMessageEnd(xwsstream* pStream, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	XS_ScriptRuntime* pScript;

	if ( !XS_WsStreamCallbackEnter(pConn, pStream) ) return;
	pScript = pConn->pScript;
	XS_WsTouch(pConn);
	if ( !pConn->bMessageFailed && pScript != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		if ( pConn->iMsgOpcode == XWS_OPCODE_TEXT && pScript->procWsText != NULL ) {
			pScript->procWsText(pConn->pHost, pStream,
				xrtStrViewN((cstr)pConn->pMsg, pConn->iMsgSize));
		} else if ( pConn->iMsgOpcode == XWS_OPCODE_BINARY && pScript->procWsBinary != NULL ) {
			xbytesview tData;

			tData.Data = pConn->pMsg;
			tData.Size = pConn->iMsgSize;
			pScript->procWsBinary(pConn->pHost, pStream, tData);
		}
		XS_ScriptLeave(pPrevious);
	}
	pConn->iMsgSize = 0;
	/* 小/中等消息复用累积器；单次大消息的峰值不保留到连接终态。 */
	if ( pConn->iMsgCap > XS_WS_MESSAGE_RETAIN_LIMIT ) {
		xrtFree(pConn->pMsg);
		pConn->pMsg = NULL;
		pConn->iMsgCap = 0;
	}
	XS_WsStreamCallbackLeave(pConn, pStream);
}

static void XS_WsOnPing(xwsstream* pStream, xbytesview Payload, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	XS_ScriptRuntime* pScript;

	if ( !XS_WsStreamCallbackEnter(pConn, pStream) ) return;
	pScript = pConn->pScript;
	XS_WsTouch(pConn);
	if ( pScript != NULL && pScript->procWsPing != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		pScript->procWsPing(pConn->pHost, pStream, Payload);
		XS_ScriptLeave(pPrevious);
	}
	XS_WsStreamCallbackLeave(pConn, pStream);
}

static void XS_WsOnPong(xwsstream* pStream, xbytesview Payload, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	XS_ScriptRuntime* pScript;

	if ( !XS_WsStreamCallbackEnter(pConn, pStream) ) return;
	pScript = pConn->pScript;
	XS_WsTouch(pConn);
	if ( pScript != NULL && pScript->procWsPong != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		pScript->procWsPong(pConn->pHost, pStream, Payload);
		XS_ScriptLeave(pPrevious);
	}
	XS_WsStreamCallbackLeave(pConn, pStream);
}

static void XS_WsOnWsClose(xwsstream* pStream, const xwsstreamclose* pClose, ptr pData)
{
	XS_WsConn* pConn = (XS_WsConn*)pData;
	XS_ScriptRuntime* pScript = pConn->pScript;

	if ( pScript != NULL && pScript->procWsClose != NULL ) {
		uint16 iCode = 0;
		xstrview tReason = {0};
		XS_ScriptRuntime* pPrevious;

		if ( pClose != NULL ) {
			iCode = pClose->RemoteCode != 0 ? pClose->RemoteCode : pClose->LocalCode;
			tReason = pClose->Reason;
		}
		pPrevious = XS_ScriptEnter(pScript);
		pScript->procWsClose(pConn->pHost, pStream, iCode, tReason);
		XS_ScriptLeave(pPrevious);
	}
	/* 先出活动表，避免 idle/停机线程对已经 Destroy 的 WS 句柄再次 Abort。 */
	XS_WsListRemove(pConn->pRuntime, pConn);
	xrtWsStreamDestroy(pStream);	/* 附带关闭并回收底层传输 */
	XS_WsConnRelease(pConn);
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
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;
	XS_WsRuntime* pRuntime = NULL;
	XS_ServerGeneration* pGeneration = NULL;
	XS_WsConn* pConn = (XS_WsConn*)xrtCalloc(1, sizeof(XS_WsConn));

	(void)pListener;
	if ( pConn == NULL ) return false;
	pConn->iReferences = 1;
	if ( !XS_ListenerSlotAcquireConnection(pSlot, 0, (void**)&pRuntime, &pGeneration) ) {
		XS_WsConnRelease(pConn);
		return false;
	}
	pConn->pGeneration = pGeneration;
	if ( xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
		XS_WsConnRelease(pConn);
		return false;
	}
	pConn->pRuntime = pRuntime;
	pConn->pTcp = pStream;
	xrtAtomic64Init(&pConn->tLastActive, (uint64)xrtNow());
	xrtHttp1HeadInit(&pConn->tHead, pConn->arrFields, XS_WS_MAX_FIELDS);
	if ( !XS_WsListAdd(pRuntime, pConn) ) {
		XS_WsConnRelease(pConn);
		return false;
	}
	if ( !xrtNetStreamSetData(pStream, pConn) ) {
		XS_WsListRemove(pRuntime, pConn);
		XS_WsConnRelease(pConn);
		return false;
	}
	return true;
}

static void XS_WsOnListenerClose(xnetlistener* pListener, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;

	XS_ListenerSlotResourceClose(pSlot, XS_LISTENER_RESOURCE_PLAIN);
	xrtNetListenerDestroy(pListener);
}

static const xnetlistenerevents g_XS_WsListenerEvents = {
	XS_WsOnAccept, NULL, XS_WsOnListenerClose
};

static bool XS_WsTlsOnAccept(xtlslistener* pListener, xtlsstream* pStream, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;
	XS_WsRuntime* pRuntime = NULL;
	XS_ServerGeneration* pGeneration = NULL;
	XS_WsConn* pConn = (XS_WsConn*)xrtCalloc(1, sizeof(XS_WsConn));

	(void)pListener;
	if ( pConn == NULL ) return false;
	pConn->iReferences = 1;
	if ( !XS_TlsAcquireConnection(pSlot, pStream, (void**)&pRuntime, &pGeneration) ) {
		XS_WsConnRelease(pConn);
		return false;
	}
	pConn->pGeneration = pGeneration;
	if ( xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
		XS_WsConnRelease(pConn);
		return false;
	}
	pConn->pRuntime = pRuntime;
	pConn->pTls = pStream;
	xrtAtomic64Init(&pConn->tLastActive, (uint64)xrtNow());
	xrtHttp1HeadInit(&pConn->tHead, pConn->arrFields, XS_WS_MAX_FIELDS);
	if ( !XS_WsListAdd(pRuntime, pConn) ) {
		XS_WsConnRelease(pConn);
		return false;
	}
	if ( !xrtTlsStreamSetEvents(pStream, &g_XS_WsTlsTransportEvents, pConn) ) {
		XS_WsListRemove(pRuntime, pConn);
		XS_WsConnRelease(pConn);
		return false;
	}
	return true;
}

static void XS_WsTlsOnListenerClose(xtlslistener* pListener, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;

	XS_ListenerSlotResourceClose(pSlot, XS_LISTENER_RESOURCE_TLS);
	xrtTlsListenerDestroy(pListener);
}

static const xtlslistenerevents g_XS_WsTlsListenerEvents = {
	XS_WsTlsOnAccept, XS_TlsHandshakeError, NULL, XS_WsTlsOnListenerClose
};

/* ============================================================
 * idle 扫描
 * ============================================================ */

static uint32 XS_WsSweepIdle(XS_WsRuntime* pRuntime)
{
	XS_WsConn* pConn;
	int64 tNow = xrtNow();
	uint32 iStale = 0;

	if ( pRuntime == NULL || pRuntime->iIdleMs == 0 ) return 0;
	xrtMutexLock(pRuntime->pConnLock);
	for ( pConn = pRuntime->pConns; pConn != NULL; pConn = pConn->pNext ) {
		int64 tLast = (int64)xrtAtomic64Load(&pConn->tLastActive, XMEMORY_RELAXED);
		uint64 iElapsedMs = tNow > tLast ? (uint64)(tNow - tLast) / 1000u : 0;

		if ( iElapsedMs <= pRuntime->iIdleMs ) continue;
		iStale++;
		if ( pConn->pWs != NULL ) {
			(void)xrtWsStreamAbort(pConn->pWs);
		} else if ( pConn->pTls != NULL ) {
			(void)xrtTlsStreamAbort(pConn->pTls);
		} else if ( pConn->pTcp != NULL ) {
			(void)xrtNetStreamAbort(pConn->pTcp);
		}
	}
	xrtMutexUnlock(pRuntime->pConnLock);
	return iStale;
}

static void XS_WsSweepProc(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_WsRuntime* pRuntime = (XS_WsRuntime*)pData;
	XS_ServerGeneration* pGeneration = XS_GenerationTimerFinish(&pRuntime->tSweepTimer);

	(void)pWorker; (void)iId;
	if ( pGeneration == NULL ) return;
	if ( iResult == XNET_RESULT_OK &&
	     xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) == 0 ) {
		uint64 iInterval = pRuntime->iIdleMs / 2u;

		(void)XS_WsSweepIdle(pRuntime);
		if ( iInterval > 1000 ) iInterval = 1000;
		if ( iInterval < 10 ) iInterval = 10;
		if ( XS_GenerationTimerSchedule(pRuntime->pGeneration,
			iInterval * 1000, XS_WsSweepProc, pRuntime,
			pRuntime, &pRuntime->tSweepTimer) != 0 &&
		     xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
			XS_GenerationTimerCancelOwner(pRuntime->pGeneration, pRuntime);
		}
	}
	XS_GenerationActivityRelease(pGeneration);
	XS_GenerationRelease(pGeneration);
}

static bool XS_WsScheduleSweep(XS_WsRuntime* pRuntime)
{
	uint64 iInterval = pRuntime->iIdleMs / 2u;

	if ( iInterval > 1000 ) iInterval = 1000;
	if ( iInterval < 10 ) iInterval = 10;
	return XS_GenerationTimerSchedule(pRuntime->pGeneration,
		iInterval * 1000, XS_WsSweepProc, pRuntime,
		pRuntime, &pRuntime->tSweepTimer) != 0;
}

/* ============================================================
 * 启动 / 停止
 * ============================================================ */

static bool XS_WsStartEx(
	XS_ServerInfo* pServer,
	bool bStartEndpoint,
	bool bAcceptEndpoint,
	char* sErr,
	size_t iErrCap)
{
	XS_WsRuntime* pRuntime = (XS_WsRuntime*)xrtCalloc(1, sizeof(XS_WsRuntime));
	xhttp1limits tDef;
	xnetlistenconfig tNetDef;
	xnetaddr tAddr;
	xnetaddr tTlsAddr;
	uint64 iVal = 0;

	if ( pRuntime == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	pRuntime->pServer = pServer;
	pRuntime->pGeneration = (XS_ServerGeneration*)pServer->Generation;
	xrtAtomic32Init(&pRuntime->tStopping, 0);
	pRuntime->pConnLock = xrtMutexCreate();
	if ( pRuntime->pConnLock == NULL ) {
		xrtFree(pRuntime);
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	pServer->Runtime = pRuntime;

	{
		uint32 i;

		for ( i = 0; i <= pServer->HostCount; i++ ) {
			XS_HostInfo* pHost = i == 0 ? pServer->DefaultHost : pServer->Hosts[i - 1];
			XS_ScriptRuntime* pScript;

			if ( pHost == NULL || !pHost->Enabled ) continue;
			pScript = (XS_ScriptRuntime*)pHost->Runtime;
			if ( pScript == NULL ||
			     (pScript->procWsText == NULL && pScript->procWsBinary == NULL) ) {
				snprintf(sErr, iErrCap,
					"ws host '%s/%s' requires script exporting WsText/WsBinary",
					pServer->Name, pHost->Name != NULL ? pHost->Name : "?");
				return false;
			}
		}
	}
	if ( !XS_VHostTableBuild(pServer, &pRuntime->tVHosts, sErr, iErrCap) ) {
		return false;
	}
	if ( !xrtNetAddrParse(&tAddr, pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port) ) {
		snprintf(sErr, iErrCap, "ws server '%s' addr parse failed", pServer->Name);
		return false;
	}
	if ( pServer->TLS && !xrtNetAddrParse(&tTlsAddr,
		pServer->IPTLS ? pServer->IPTLS : (pServer->IP ? pServer->IP : "0.0.0.0"),
		pServer->PortTLS) ) {
		snprintf(sErr, iErrCap, "wss server '%s' tls addr parse failed", pServer->Name);
		return false;
	}
	xrtHttp1LimitsInit(&tDef);
	pRuntime->tLimits = tDef;
	xrtNetListenConfigInit(&tNetDef);
	pRuntime->iReceiveLimit = pServer->RecvLimit > 0 ?
		pServer->RecvLimit : tNetDef.Stream.ReadLimit;
	if ( pRuntime->tLimits.MaxFields > XS_WS_MAX_FIELDS ) {
		pRuntime->tLimits.MaxFields = XS_WS_MAX_FIELDS;
	}
	if ( !XS_CustomReadUInt(pServer->Custom, "ws_message_limit", &iVal,
		sErr, iErrCap) ) return false;
	pRuntime->iMessageLimit = iVal;
	if ( !XS_CustomReadUInt(pServer->Custom, "idle_timeout", &iVal,
		sErr, iErrCap) ) return false;
	pRuntime->iIdleMs = iVal;
	{
		if ( pServer->Custom != NULL ) {
			xvalue* pVal = xrtValueObjectGet(pServer->Custom,
				xrtStrViewN("ws_protocol", strlen("ws_protocol")));
			xstrview tView;

			if ( pVal != NULL ) {
				if ( !xrtValueGetString(pVal, &tView) ) {
					snprintf(sErr, iErrCap, "custom field 'ws_protocol' expect string");
					return false;
				}
				if ( tView.Size > 0 && !xrtWsProtocolsValid(tView) ) {
					snprintf(sErr, iErrCap, "custom field 'ws_protocol' is invalid");
					return false;
				}
				if ( tView.Size > 0 ) {
					pRuntime->sProtocol = xrtStrDupN(tView.Data, tView.Size);
					if ( pRuntime->sProtocol == NULL ) {
						snprintf(sErr, iErrCap, "out of memory reading ws_protocol");
						return false;
					}
				}
			}
		}
	}

	if ( pServer->TLS ) {
		if ( XS_TlsSharedContext() == NULL ||
		     !XS_TlsTableBuild(pServer, &pRuntime->tTls, sErr, iErrCap) ||
		     pRuntime->tTls.iCount == 0 ) {
			if ( sErr[0] == '\0' ) {
				snprintf(sErr, iErrCap,
					"wss server '%s' has no usable tls identity", pServer->Name);
			}
			return false;
		}
		pRuntime->bTls = true;
	}
	if ( bStartEndpoint ) {
		pRuntime->pListenerSlot = XS_ListenerSlotCreate(pRuntime, pRuntime->pGeneration,
			pRuntime->bTls ? (void*)&pRuntime->tTls : NULL, bAcceptEndpoint);
		if ( pRuntime->pListenerSlot == NULL ) {
			snprintf(sErr, iErrCap, "ws listener slot create failed");
			return false;
		}
	}

	if ( bStartEndpoint ) {
		xnetlistenconfig tListen;
		xnetlistener* pListener;

		xrtNetListenConfigInit(&tListen);
		tListen.Address = tAddr;
		tListen.ReuseAddress = true;
		tListen.ExclusiveAddress = false;
		if ( pServer->Backlog > 0 ) {
			tListen.Backlog = (int)pServer->Backlog;
		}
		XS_StreamApplyReceiveLimit(&tListen.Stream, pServer->RecvLimit);
		if ( !XS_ListenerSlotResourceAdd(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_PLAIN) ) return false;
		pListener = xrtNetListen(pServer->Engine, &tListen,
			&g_XS_WsListenerEvents, &g_XS_WsTransportEvents, pRuntime->pListenerSlot);
		if ( pListener == NULL ) {
			XS_ListenerSlotResourceCancel(pRuntime->pListenerSlot,
				XS_LISTENER_RESOURCE_PLAIN);
			XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
			pRuntime->pListenerSlot = NULL;
			const xerror* pErr = xrtGetError();
			snprintf(sErr, iErrCap, "ws server '%s' listen failed (port %u): %s",
				pServer->Name, pServer->Port, pErr ? xrtErrorMessage(pErr) : "unknown");
			return false;
		}
		if ( !XS_ListenerSlotResourceAttach(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_PLAIN, pListener) ) {
			XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
			pRuntime->pListenerSlot = NULL;
			snprintf(sErr, iErrCap, "ws server '%s' listener closed during start",
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
		tTlsListen.Tls.Identity = pRuntime->tTls.pEntries[0].pIdentity;
		tTlsListen.Tls.Select = XS_TlsSlotSelect;
		tTlsListen.Tls.SelectContext = pRuntime->pListenerSlot;
		if ( !XS_ListenerSlotResourceAdd(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_TLS) ) return false;
		pTlsListener = xrtTlsListenerStart(pServer->Engine, &tTlsListen,
			&g_XS_WsTlsListenerEvents, NULL, pRuntime->pListenerSlot);
		if ( pTlsListener == NULL ) {
			XS_ListenerSlotResourceCancel(pRuntime->pListenerSlot,
				XS_LISTENER_RESOURCE_TLS);
			snprintf(sErr, iErrCap, "wss server '%s' listen failed (port %u)", pServer->Name, pServer->PortTLS);
			return false;
		}
		if ( !XS_ListenerSlotResourceAttach(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_TLS, pTlsListener) ) {
			snprintf(sErr, iErrCap, "wss server '%s' listener closed during start",
				pServer->Name);
			return false;
		}
	}
	if ( pRuntime->iIdleMs > 0 && !XS_WsScheduleSweep(pRuntime) ) {
		snprintf(sErr, iErrCap, "ws server '%s' idle timer start failed", pServer->Name);
		return false;
	}
	printf("[xs] server '%s' ws%s %s on %s:%u", pServer->Name,
		pRuntime->bTls ? "+wss" : "",
		bStartEndpoint ? (bAcceptEndpoint ? "ready" : "bound") : "prepared",
		pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port);
	if ( pRuntime->bTls ) {
		printf(" and %s:%u", pServer->IPTLS ? pServer->IPTLS :
			(pServer->IP ? pServer->IP : "0.0.0.0"), pServer->PortTLS);
	}
	printf("%s%s\n",
		pRuntime->sProtocol != NULL ? " (subprotocol required)" : "",
		pRuntime->iIdleMs > 0 ? " (idle protected)" : "");
	return true;
}

static bool XS_WsHandoff(XS_WsRuntime* pOld, XS_WsRuntime* pNew)
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

static void XS_WsStop(XS_WsRuntime* pRuntime)
{
	XS_ListenerSlot* pSlot;
	XS_ListenerResources tResources;

	if ( pRuntime == NULL ) {
		return;
	}
	if ( xrtAtomic32Exchange(&pRuntime->tStopping, 1, XMEMORY_ACQ_REL) != 0 ) return;
	XS_GenerationTimerCancelOwner(pRuntime->pGeneration, pRuntime);
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

static void XS_WsCloseConnections(XS_WsRuntime* pRuntime)
{
	XS_WsConn* pConn;

	if ( pRuntime == NULL ) return;
	xrtMutexLock(pRuntime->pConnLock);
	for ( pConn = pRuntime->pConns; pConn != NULL; pConn = pConn->pNext ) {
		if ( pConn->pWs != NULL ) {
			/* Close 要求所属 worker；Abort 是跨线程安全的终态收口 API。 */
			(void)xrtWsStreamAbort(pConn->pWs);
		} else if ( pConn->pTls != NULL ) {
			(void)xrtTlsStreamAbort(pConn->pTls);
		} else if ( pConn->pTcp != NULL ) {
			(void)xrtNetStreamAbort(pConn->pTcp);
		}
	}
	xrtMutexUnlock(pRuntime->pConnLock);
}

static void XS_WsUnit(XS_WsRuntime* pRuntime)
{
	if ( pRuntime == NULL ) {
		return;
	}
	xrtFree(pRuntime->sProtocol);
	XS_VHostTableUnit(&pRuntime->tVHosts);
	XS_TlsTableUnit(&pRuntime->tTls);
	if ( pRuntime->pListenerSlot != NULL ) {
		XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
		pRuntime->pListenerSlot = NULL;
	}
	if ( pRuntime->pConnLock != NULL ) {
		xrtMutexDestroy(pRuntime->pConnLock);
	}
	xrtFree(pRuntime);
}

#endif
