#ifndef XS_PROTOCOL_WS_H
#define XS_PROTOCOL_WS_H

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
	
	if ( objHost && objHost->procWsBinary ) {
		bHandled = objHost->procWsBinary(objServer, objHost, pConn, pData, iLen);
	}
	
	if ( !bHandled ) {
		(void)xrtWsConnSendBinary(pConn, pData, iLen);
	}
}

static void XS_WsOnClose(ptr pOwner, xwsserver* pServer, xwsconn* pConn, xnet_result iReason)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HostConfig* objHost = XS_WsResolveHost(objServer);
	(void)pServer;
	
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
	tConfig.iRecvLimit = objServer->RecvLimit;
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
	tEvents.OnClose = XS_WsOnClose;
	tEvents.OnError = XS_WsOnError;
	
	pServer = xrtWsServerCreate(pEngine, &tConfig, &tEvents, objServer);
	if ( pServer == NULL ) {
		XS_ReportError("ws init failed: xrtWsServerCreate returned null");
		return FALSE;
	}
	
	objServer->pHandle = pServer;
	XS_LogInfo(
		"ws init: name=%s addr=%s host=%s protocol=%s tls=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->EnableTLS ? (objServer->AddrTLS ? objServer->AddrTLS : "(null)") : (objServer->Addr ? objServer->Addr : "(null)"),
		XS_WsResolveHost(objServer) && XS_WsResolveHost(objServer)->Name ? XS_WsResolveHost(objServer)->Name : "(default)",
		objServer->WsProtocol ? objServer->WsProtocol : "",
		objServer->EnableTLS ? "true" : "false"
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
