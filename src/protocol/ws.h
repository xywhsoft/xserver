#ifndef XS_PROTOCOL_WS_H
#define XS_PROTOCOL_WS_H

typedef struct {
	XS_ServerConfig* pServer;
	xwsconn* pConn;
	xnetstream* pStream;
	int64 iLastActiveMS;
	volatile bool bClosing;
	char sServerName[128];
	char sHostName[128];
	ptr pTracker;
} XS_WsConnContext;

typedef struct {
	xwsserver* pServer;
	XS_ServerConfig* pOwner;
	xthread hIdleThread;
	volatile bool bStopThread;
	volatile bool bStopping;
	xmutex pConnLock;
	xarray arrConn;
} XS_WsHandle;

static inline XS_HostConfig* XS_WsResolveHost(XS_ServerConfig* objServer);
static inline void XS_WsRecordClose(xnet_result iReason);

#include "../manage/ws_runtime.h"

static void XS_WsOnOpen(ptr pOwner, xwsserver* pServer, xwsconn* pConn)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HostConfig* objHost = XS_WsResolveHost(objServer);
	XS_WsHandle* objHandle = objServer ? (XS_WsHandle*)objServer->pHandle : NULL;
	XS_WsConnContext* objCtx = NULL;
	(void)pServer;

	if ( objHandle && objHandle->bStopping ) {
		XS_WsRecordRemoteOpenConn(pConn);
		XS_WsRecordRejectEvent("server_stopping");
		XS_LogWarn(
			"ws open rejected while stopping: server=%s host=%s remote=%s",
			objServer && objServer->Name ? objServer->Name : "(null)",
			objHost && objHost->Name ? objHost->Name : "(default)",
			g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)"
		);
		if ( pConn && pConn->pStream ) {
			xrtNetStreamClose(pConn->pStream, XNET_CLOSE_F_ABORT);
		}
		return;
	}

	if ( pConn && pConn->pStream ) {
		objCtx = (XS_WsConnContext*)xrtCalloc(1, sizeof(XS_WsConnContext));
		if ( objCtx ) {
			objCtx->pServer = objServer;
			objCtx->pConn = pConn;
			objCtx->pStream = pConn->pStream;
			XS_WsCopyContextName(objCtx->sServerName, sizeof(objCtx->sServerName), objServer ? objServer->Name : NULL);
			XS_WsCopyContextName(objCtx->sHostName, sizeof(objCtx->sHostName), objHost && objHost->Name ? objHost->Name : "(default)");
			objCtx->pTracker = objHandle;
			XS_WsTouch(objCtx);
			XS_WsTrackConn(objHandle, objCtx);
		}
	}
	XS_WsMetricAdd(&g_iXsWsOpenCount, 1);
	XS_WsMetricUpdateMax(&g_iXsWsConnPeak, XS_WsMetricAdd(&g_iXsWsConnCurrent, 1));
	XS_WsRecordRemoteOpenConn(pConn);
	if ( objServer && objServer->ConnLimit > 0u && XS_WsTrackedConnCount(objHandle) > (int64)objServer->ConnLimit ) {
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		XS_WsRecordConnLimitClose();
		XS_LogWarn(
			"ws conn limit exceeded: server=%s current=%lld limit=%u remote=%s",
			objServer->Name ? objServer->Name : "(null)",
			(long long)XS_WsTrackedConnCount(objHandle),
			(unsigned)objServer->ConnLimit,
			g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)"
		);
		if ( pConn && pConn->pStream ) {
			xrtNetStreamClose(pConn->pStream, 0u);
		}
		return;
	}
	
	XS_LogInfo(
		"ws open: server=%s host=%s remote=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost && objHost->Name ? objHost->Name : "(default)",
		g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)"
	);
	
	if ( objHost && objHost->procWsOpen ) {
		objHost->procWsOpen(objServer, objHost, pConn);
	}
}

static void XS_WsOnText(ptr pOwner, xwsserver* pServer, xwsconn* pConn, const char* pData, size_t iLen)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HostConfig* objHost = XS_WsResolveHost(objServer);
	XS_WsConnContext* objCtx = XS_WsGetConnContext(objServer, pConn);
	bool bHandled = FALSE;
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsTextCount, 1);
	XS_WsTouch(objCtx);
	XS_WsRecordRemoteContext(objCtx);
	XS_WsRecordLastFrame(1, pData, iLen);
	
	if ( objHost && objHost->procWsText ) {
		bHandled = objHost->procWsText(objServer, objHost, pConn, pData, iLen);
	}
	
	if ( !bHandled ) {
		(void)xrtWsConnSendText(pConn, pData ? pData : "", iLen);
	}
}

static void XS_WsOnBinary(ptr pOwner, xwsserver* pServer, xwsconn* pConn, const void* pData, size_t iLen)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HostConfig* objHost = XS_WsResolveHost(objServer);
	XS_WsConnContext* objCtx = XS_WsGetConnContext(objServer, pConn);
	bool bHandled = FALSE;
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsBinaryCount, 1);
	XS_WsTouch(objCtx);
	XS_WsRecordRemoteContext(objCtx);
	XS_WsRecordLastFrame(2, pData, iLen);
	
	if ( objHost && objHost->procWsBinary ) {
		bHandled = objHost->procWsBinary(objServer, objHost, pConn, pData, iLen);
	}
	
	if ( !bHandled ) {
		(void)xrtWsConnSendBinary(pConn, pData, iLen);
	}
}

static void XS_WsOnPing(ptr pOwner, xwsserver* pServer, xwsconn* pConn, const void* pData, size_t iLen)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HostConfig* objHost = XS_WsResolveHost(objServer);
	XS_WsConnContext* objCtx = XS_WsGetConnContext(objServer, pConn);
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsPingCount, 1);
	XS_WsTouch(objCtx);
	XS_WsRecordRemoteContext(objCtx);
	XS_WsRecordLastFrame(3, pData, iLen);

	XS_LogInfo(
		"ws ping: server=%s host=%s bytes=%u remote=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost && objHost->Name ? objHost->Name : "(default)",
		(unsigned)iLen,
		g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)"
	);

	if ( objHost && objHost->procWsPing ) {
		objHost->procWsPing(objServer, objHost, pConn, pData, iLen);
	}
}

static void XS_WsOnPong(ptr pOwner, xwsserver* pServer, xwsconn* pConn, const void* pData, size_t iLen)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HostConfig* objHost = XS_WsResolveHost(objServer);
	XS_WsConnContext* objCtx = XS_WsGetConnContext(objServer, pConn);
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsPongCount, 1);
	XS_WsTouch(objCtx);
	XS_WsRecordRemoteContext(objCtx);
	XS_WsRecordLastFrame(4, pData, iLen);

	XS_LogInfo(
		"ws pong: server=%s host=%s bytes=%u remote=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost && objHost->Name ? objHost->Name : "(default)",
		(unsigned)iLen,
		g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)"
	);

	if ( objHost && objHost->procWsPong ) {
		objHost->procWsPong(objServer, objHost, pConn, pData, iLen);
	}
}

static void XS_WsOnClose(ptr pOwner, xwsserver* pServer, xwsconn* pConn, xnet_result iReason)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_WsConnContext* objCtx = XS_WsGetConnContext(objServer, pConn);
	XS_WsHandle* objHandle = objCtx ? (XS_WsHandle*)objCtx->pTracker : (objServer ? (XS_WsHandle*)objServer->pHandle : NULL);
	XS_HostConfig* objHost = NULL;
	const char* sServerName = (objCtx && objCtx->sServerName[0]) ? objCtx->sServerName : (objServer && objServer->Name ? objServer->Name : "(null)");
	const char* sHostName = (objCtx && objCtx->sHostName[0]) ? objCtx->sHostName : "(default)";
	const char* sInvalidReason = XS_WsInvalidCloseReasonText(iReason);
	bool bSuppressCloseCallback = FALSE;
	(void)pServer;

	if ( objHandle && objHandle->bStopping ) {
		bSuppressCloseCallback = TRUE;
	}
	if ( objCtx && objCtx->pTracker == NULL ) {
		bSuppressCloseCallback = TRUE;
	}
	if ( !bSuppressCloseCallback ) {
		objHost = XS_WsResolveHost(objServer);
	}
	if ( sInvalidReason ) {
		if ( objCtx ) {
			XS_WsRecordRemoteContext(objCtx);
		} else {
			XS_WsRecordRemoteOpenConn(pConn);
		}
		XS_WsRecordInvalid(sInvalidReason);
		XS_LogWarn(
			"ws invalid close: server=%s host=%s reason=%s code=%d remote=%s",
			sServerName,
			objHost && objHost->Name ? objHost->Name : sHostName,
			sInvalidReason,
			(int)iReason,
			g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)"
		);
	}
	if ( objCtx == NULL ) {
		return;
	}

	XS_WsRecordClose(iReason);
	XS_WsRecordRemoteContext(objCtx);
	
	XS_LogInfo(
		"ws close: server=%s host=%s reason=%d remote=%s",
		sServerName,
		objHost && objHost->Name ? objHost->Name : sHostName,
		(int)iReason,
		g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)"
	);
	
	if ( !bSuppressCloseCallback && objHost && objHost->procWsClose ) {
		objHost->procWsClose(objServer, objHost, pConn, (int)iReason);
	}

	if ( objCtx ) {
		XS_WsUntrackConn((XS_WsHandle*)objCtx->pTracker, objCtx);
		if ( objCtx->pStream ) {
			objCtx->pStream = NULL;
		}
		xrtFree(objCtx);
	}
}

static void XS_WsOnError(ptr pOwner, xwsserver* pServer, xwsconn* pConn, int iSysErr)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_WsConnContext* objCtx = XS_WsGetConnContext(objServer, pConn);
	const char* sServerName = (objCtx && objCtx->sServerName[0]) ? objCtx->sServerName : (objServer && objServer->Name ? objServer->Name : "(null)");
	const char* sHostName = (objCtx && objCtx->sHostName[0]) ? objCtx->sHostName : "(default)";
	const char* sInvalidReason = XS_WsInvalidErrorReasonText(iSysErr);
	(void)pServer;

	if ( sInvalidReason ) {
		if ( objCtx ) {
			XS_WsRecordRemoteContext(objCtx);
		} else {
			XS_WsRecordRemoteOpenConn(pConn);
		}
		XS_WsRecordInvalid(sInvalidReason);
		XS_LogWarn(
			"ws invalid error: server=%s host=%s reason=%s sys=%d remote=%s",
			sServerName,
			sHostName,
			sInvalidReason,
			iSysErr,
			g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)"
		);
		return;
	}
	if ( objCtx == NULL ) {
		return;
	}
	if ( objCtx->bClosing ) {
		return;
	}
	if ( iSysErr == -1 ) {
		return;
	}
	XS_WsMetricAdd(&g_iXsWsErrorCount, 1);
	g_iXsWsLastErrorCode = (int64)iSysErr;
	g_tXsWsLastErrorTime = xrtNow();
	XS_WsRecordRemoteContext(objCtx);
	XS_WsSnapshotErrorRemote();
	
	XS_LogWarn(
		"ws error: server=%s host=%s sys=%d remote=%s",
		sServerName,
		sHostName,
		iSysErr,
		g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)"
	);
}

static inline bool XS_WsInitServer(xnetengine* pEngine, XS_ServerConfig* objServer)
{
	xwsserverconfig tConfig;
	xwsserverevents tEvents;
	xwsserver* pServer;
	XS_WsHandle* objHandle;
	
	if ( objServer == NULL ) {
		XS_ReportError("ws init failed: server is null");
		return FALSE;
	}
	if ( pEngine == NULL ) {
		XS_ReportError("ws init failed: runtime engine is null");
		return FALSE;
	}
	
	xrtWsServerConfigInit(&tConfig);
	if ( !XS_BuildBindAddr(objServer, objServer->EnableTLS, &tConfig.tBindAddr) ) {
		XS_ReportError(
			"ws init failed: invalid addr: %s",
			objServer->EnableTLS ? (objServer->AddrTLS ? objServer->AddrTLS : "(null)") : (objServer->Addr ? objServer->Addr : "(null)")
		);
		return FALSE;
	}
	tConfig.iBacklog = objServer->Backlog;
	tConfig.iRecvLimit = objServer->WsMessageLimit ? objServer->WsMessageLimit : objServer->RecvLimit;
	if ( objServer->EnableTLS ) {
		tConfig.pTlsConfig = &objServer->TlsConfig;
	}
	if ( objServer->WsProtocol && objServer->WsProtocol[0] ) {
		snprintf(tConfig.sProtocol, sizeof(tConfig.sProtocol), "%s", objServer->WsProtocol);
	}
	
	memset(&tEvents, 0, sizeof(tEvents));
	tEvents.OnOpen = XS_WsOnOpen;
	tEvents.OnText = XS_WsOnText;
	tEvents.OnBinary = XS_WsOnBinary;
	tEvents.OnPing = XS_WsOnPing;
	tEvents.OnPong = XS_WsOnPong;
	tEvents.OnClose = XS_WsOnClose;
	tEvents.OnError = XS_WsOnError;
	
	pServer = xrtWsServerCreate(pEngine, &tConfig, &tEvents, objServer);
	if ( pServer == NULL ) {
		XS_ReportError("ws init failed: xrtWsServerCreate returned null");
		return FALSE;
	}

	objHandle = (XS_WsHandle*)xrtCalloc(1, sizeof(XS_WsHandle));
	if ( objHandle == NULL ) {
		xrtWsServerDestroy(pServer);
		XS_ReportError("ws init failed: handle alloc failed");
		return FALSE;
	}
	objHandle->pServer = pServer;
	objHandle->pOwner = objServer;
	objHandle->pConnLock = xrtMutexCreate();
	objHandle->arrConn = xrtArrayCreate(sizeof(XS_WsConnContext*), XRT_OBJMODE_SHARED);
	if ( objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		if ( objHandle->arrConn ) {
			xrtArrayDestroy(objHandle->arrConn);
		}
		if ( objHandle->pConnLock ) {
			xrtMutexDestroy(objHandle->pConnLock);
		}
		xrtWsServerDestroy(pServer);
		xrtFree(objHandle);
		XS_ReportError("ws init failed: handle resource alloc failed");
		return FALSE;
	}

	objServer->pHandle = objHandle;
	XS_LogInfo(
		"ws init: name=%s addr=%s host=%s protocol=%s tls=%s recv=%u message=%u",
		objServer->Name ? objServer->Name : "(null)",
		objServer->EnableTLS ? (objServer->AddrTLS ? objServer->AddrTLS : "(null)") : (objServer->Addr ? objServer->Addr : "(null)"),
		XS_WsResolveHost(objServer) && XS_WsResolveHost(objServer)->Name ? XS_WsResolveHost(objServer)->Name : "(default)",
		objServer->WsProtocol ? objServer->WsProtocol : "",
		objServer->EnableTLS ? "true" : "false",
		(unsigned)objServer->RecvLimit,
		(unsigned)tConfig.iRecvLimit
	);
	return TRUE;
}

static inline bool XS_WsStartServer(XS_ServerConfig* objServer)
{
	XS_WsHandle* objHandle;
	xwsserver* pServer;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	
	objHandle = (XS_WsHandle*)objServer->pHandle;
	pServer = objHandle ? objHandle->pServer : NULL;
	if ( pServer == NULL ) {
		XS_ReportError("ws start failed: server handle is null");
		return FALSE;
	}
	if ( xrtWsServerStart(pServer) != XRT_NET_OK ) {
		XS_ReportError("ws start failed: xrtWsServerStart returned error");
		return FALSE;
	}
	objHandle->bStopThread = FALSE;
	objHandle->hIdleThread = xrtThreadCreate(XS_WsIdleThread, objHandle, 0);
	if ( objHandle->hIdleThread == NULL ) {
		xrtWsServerStop(pServer);
		XS_ReportError("ws start failed: idle thread create failed");
		return FALSE;
	}
	
	XS_LogInfo(
		"ws start: server=%s addr=%s bound_port=%u tls=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->EnableTLS ? (objServer->AddrTLS ? objServer->AddrTLS : "(null)") : (objServer->Addr ? objServer->Addr : "(null)"),
		(unsigned)xrtWsServerBoundPort(pServer),
		objServer->EnableTLS ? "true" : "false"
	);
	return TRUE;
}

static inline void XS_WsStopServer(XS_ServerConfig* objServer)
{
	XS_WsHandle* objHandle;
	xwsserver* pServer;
	int64 iClosedConn;
	int64 iRemainConn;
	
	if ( objServer == NULL ) {
		return;
	}
	
	objHandle = (XS_WsHandle*)objServer->pHandle;
	pServer = objHandle ? objHandle->pServer : NULL;
	if ( objHandle ) {
		objHandle->bStopping = TRUE;
		objHandle->bStopThread = TRUE;
		if ( objHandle->hIdleThread ) {
			xrtThreadWait(objHandle->hIdleThread);
			xrtThreadDestroy(objHandle->hIdleThread);
			objHandle->hIdleThread = NULL;
		}
	}
	iClosedConn = XS_WsCloseTrackedConns(objHandle);
	iRemainConn = XS_WsWaitTrackedConnDrain(objHandle, 500u);
	if ( iRemainConn > 0 ) {
		XS_WsAbortTrackedConns(objHandle);
		iRemainConn = XS_WsWaitTrackedConnDrain(objHandle, 1000u);
	}
	if ( iRemainConn > 0 ) {
		XS_WsFinalizeTrackedConns(objHandle);
		iRemainConn = XS_WsTrackedConnCount(objHandle);
	}
	if ( pServer && pServer->pListener ) {
		xrtNetListenerStop(pServer->pListener);
	}
	if ( iClosedConn > 0 || iRemainConn > 0 ) {
		XS_WsRecordStopCleanup(iClosedConn, iRemainConn);
		XS_LogInfo(
			"ws stop cleanup: server=%s addr=%s closed=%lld remain=%lld",
			objServer->Name ? objServer->Name : "(null)",
			objServer->EnableTLS ? (objServer->AddrTLS ? objServer->AddrTLS : "(null)") : (objServer->Addr ? objServer->Addr : "(null)"),
			(long long)iClosedConn,
			(long long)iRemainConn
		);
	}
	if ( pServer ) {
		XS_WsDetachTrackedConns(objHandle);
		xrtWsServerDestroy(pServer);
	}
	if ( objHandle ) {
		if ( objHandle->arrConn ) {
			xrtArrayDestroy(objHandle->arrConn);
		}
		if ( objHandle->pConnLock ) {
			xrtMutexDestroy(objHandle->pConnLock);
		}
		xrtFree(objHandle);
	}
	objServer->pHandle = NULL;
	
	XS_LogInfo(
		"ws stop: server=%s addr=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->EnableTLS ? (objServer->AddrTLS ? objServer->AddrTLS : "(null)") : (objServer->Addr ? objServer->Addr : "(null)")
	);
}

#endif
