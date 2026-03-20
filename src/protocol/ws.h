#ifndef XS_PROTOCOL_WS_H
#define XS_PROTOCOL_WS_H

static inline int64 XS_WsMetricAdd(volatile int64* pValue, int64 iDelta)
{
	return XS_HttpMetricAdd(pValue, iDelta);
}

static inline int64 XS_WsMetricGet(const volatile int64* pValue)
{
	return XS_HttpMetricGet(pValue);
}

static inline void XS_WsMetricUpdateMax(volatile int64* pValue, int64 iValue)
{
	XS_HttpMetricUpdateMax(pValue, iValue);
}

static inline const char* XS_WsLastFrameTypeName(void)
{
	switch ( XS_WsMetricGet(&g_iXsWsLastFrameType) ) {
		case 1: return "text";
		case 2: return "binary";
		case 3: return "ping";
		case 4: return "pong";
		default: return "";
	}
}

static inline char* XS_WsLastTimeText(void)
{
	xtime tLast = g_tXsWsLastTime;
	
	if ( tLast <= 0 ) {
		return NULL;
	}
	
	return xrtTimeToStr(tLast, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_WsLastAgeMS(void)
{
	xtime tLast = g_tXsWsLastTime;
	
	if ( tLast <= 0 ) {
		return -1;
	}
	
	return (int64)(xrtNow() - tLast) * 1000;
}

static inline char* XS_WsLastErrorTimeText(void)
{
	xtime tLast = g_tXsWsLastErrorTime;

	if ( tLast <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(tLast, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_WsLastErrorAgeMS(void)
{
	xtime tLast = g_tXsWsLastErrorTime;

	if ( tLast <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - tLast) * 1000;
}

static inline char* XS_WsLastCloseTimeText(void)
{
	xtime tLast = g_tXsWsLastCloseTime;

	if ( tLast <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(tLast, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_WsLastCloseAgeMS(void)
{
	xtime tLast = g_tXsWsLastCloseTime;

	if ( tLast <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - tLast) * 1000;
}

static inline void XS_WsRecordRemote(xwsconn* pConn)
{
	const xnetaddr* pAddr;
	const char* sAddr;

	if ( pConn == NULL || pConn->pStream == NULL ) {
		g_sXsWsLastRemote[0] = '\0';
		return;
	}

	pAddr = xrtNetStreamRemoteAddr(pConn->pStream);
	sAddr = pAddr ? xrtNetAddrToStr(pAddr) : NULL;
	if ( sAddr == NULL || sAddr[0] == '\0' ) {
		g_sXsWsLastRemote[0] = '\0';
		return;
	}

	snprintf(g_sXsWsLastRemote, sizeof(g_sXsWsLastRemote), "%s", sAddr);
}

static inline void XS_WsRecordLastFrame(int64 iType, const void* pData, size_t iLen)
{
	size_t iCopy;
	
	g_iXsWsLastFrameType = iType;
	g_iXsWsLastBytes = (int64)iLen;
	g_tXsWsLastTime = xrtNow();
	if ( pData == NULL || iLen == 0 ) {
		g_sXsWsLastText[0] = '\0';
		return;
	}
	
	iCopy = iLen;
	if ( iCopy >= sizeof(g_sXsWsLastText) ) {
		iCopy = sizeof(g_sXsWsLastText) - 1;
	}
	memcpy(g_sXsWsLastText, pData, iCopy);
	g_sXsWsLastText[iCopy] = '\0';
}

static inline XS_HostConfig* XS_WsResolveHost(XS_ServerConfig* objServer)
{
	uint32 i;
	
	if ( objServer == NULL ) {
		return NULL;
	}
	
	if ( objServer->EnableDefaultHost ) {
		return &objServer->DefaultHost;
	}
	
	for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
		XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
		if ( objHost && objHost->Enabled ) {
			return objHost;
		}
	}
	
	return NULL;
}

static void XS_WsOnOpen(ptr pOwner, xwsserver* pServer, xwsconn* pConn)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HostConfig* objHost = XS_WsResolveHost(objServer);
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsOpenCount, 1);
	XS_WsMetricUpdateMax(&g_iXsWsConnPeak, XS_WsMetricAdd(&g_iXsWsConnCurrent, 1));
	XS_WsRecordRemote(pConn);
	
	XS_LogInfo(
		"ws open: server=%s host=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost && objHost->Name ? objHost->Name : "(default)"
	);
	
	if ( objHost && objHost->procWsOpen ) {
		objHost->procWsOpen(objServer, objHost, pConn);
	}
}

static void XS_WsOnText(ptr pOwner, xwsserver* pServer, xwsconn* pConn, const char* pData, size_t iLen)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HostConfig* objHost = XS_WsResolveHost(objServer);
	bool bHandled = FALSE;
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsTextCount, 1);
	XS_WsRecordRemote(pConn);
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
	bool bHandled = FALSE;
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsBinaryCount, 1);
	XS_WsRecordRemote(pConn);
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
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsPingCount, 1);
	XS_WsRecordRemote(pConn);
	XS_WsRecordLastFrame(3, pData, iLen);

	XS_LogInfo(
		"ws ping: server=%s host=%s bytes=%u",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost && objHost->Name ? objHost->Name : "(default)",
		(unsigned)iLen
	);

	if ( objHost && objHost->procWsPing ) {
		objHost->procWsPing(objServer, objHost, pConn, pData, iLen);
	}
}

static void XS_WsOnPong(ptr pOwner, xwsserver* pServer, xwsconn* pConn, const void* pData, size_t iLen)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HostConfig* objHost = XS_WsResolveHost(objServer);
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsPongCount, 1);
	XS_WsRecordRemote(pConn);
	XS_WsRecordLastFrame(4, pData, iLen);

	XS_LogInfo(
		"ws pong: server=%s host=%s bytes=%u",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost && objHost->Name ? objHost->Name : "(default)",
		(unsigned)iLen
	);

	if ( objHost && objHost->procWsPong ) {
		objHost->procWsPong(objServer, objHost, pConn, pData, iLen);
	}
}

static void XS_WsOnClose(ptr pOwner, xwsserver* pServer, xwsconn* pConn, xnet_result iReason)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HostConfig* objHost = XS_WsResolveHost(objServer);
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsCloseCount, 1);
	XS_WsRecordRemote(pConn);
	g_iXsWsLastCloseReason = (int64)iReason;
	g_tXsWsLastCloseTime = xrtNow();
	if ( XS_WsMetricAdd(&g_iXsWsConnCurrent, -1) < 0 ) {
		XS_WsMetricAdd(&g_iXsWsConnCurrent, -XS_WsMetricGet(&g_iXsWsConnCurrent));
	}
	
	XS_LogInfo(
		"ws close: server=%s host=%s reason=%d",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost && objHost->Name ? objHost->Name : "(default)",
		(int)iReason
	);
	
	if ( objHost && objHost->procWsClose ) {
		objHost->procWsClose(objServer, objHost, pConn, (int)iReason);
	}
}

static void XS_WsOnError(ptr pOwner, xwsserver* pServer, xwsconn* pConn, int iSysErr)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	(void)pServer;
	(void)pConn;

	XS_WsMetricAdd(&g_iXsWsErrorCount, 1);
	g_iXsWsLastErrorCode = (int64)iSysErr;
	g_tXsWsLastErrorTime = xrtNow();
	
	XS_LogWarn(
		"ws error: server=%s sys=%d",
		objServer && objServer->Name ? objServer->Name : "(null)",
		iSysErr
	);
}

static inline bool XS_WsInitServer(xnetengine* pEngine, XS_ServerConfig* objServer)
{
	xwsserverconfig tConfig;
	xwsserverevents tEvents;
	xwsserver* pServer;
	
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
	
	objServer->pHandle = pServer;
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
	xwsserver* pServer;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	
	pServer = (xwsserver*)objServer->pHandle;
	if ( pServer == NULL ) {
		XS_ReportError("ws start failed: server handle is null");
		return FALSE;
	}
	if ( xrtWsServerStart(pServer) != XRT_NET_OK ) {
		XS_ReportError("ws start failed: xrtWsServerStart returned error");
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
	xwsserver* pServer;
	
	if ( objServer == NULL ) {
		return;
	}
	
	pServer = (xwsserver*)objServer->pHandle;
	if ( pServer ) {
		xrtWsServerStop(pServer);
		xrtWsServerDestroy(pServer);
		objServer->pHandle = NULL;
	}
	
	XS_LogInfo(
		"ws stop: server=%s",
		objServer->Name ? objServer->Name : "(null)"
	);
}

#endif
