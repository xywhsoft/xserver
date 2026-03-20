#ifndef XS_PROTOCOL_WS_H
#define XS_PROTOCOL_WS_H

typedef struct {
	XS_ServerConfig* pServer;
	xwsconn* pConn;
	int64 iLastActiveMS;
	volatile bool bClosing;
	ptr pTracker;
} XS_WsConnContext;

typedef struct {
	xwsserver* pServer;
	XS_ServerConfig* pOwner;
	xthread hIdleThread;
	volatile bool bStopThread;
	xmutex pConnLock;
	xarray arrConn;
} XS_WsHandle;

static inline XS_HostConfig* XS_WsResolveHost(XS_ServerConfig* objServer);

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

static inline int64 XS_WsNowMS(void)
{
	#if defined(_WIN32) || defined(_WIN64)
		return (int64)GetTickCount64();
	#else
		struct timespec tNow;

		clock_gettime(CLOCK_MONOTONIC, &tNow);
		return ((int64)tNow.tv_sec * 1000) + ((int64)tNow.tv_nsec / 1000000);
	#endif
}

static inline void XS_WsSleepMS(uint32 iMS)
{
	#if defined(_WIN32) || defined(_WIN64)
		Sleep(iMS);
	#else
		struct timespec tReq;

		tReq.tv_sec = (time_t)(iMS / 1000u);
		tReq.tv_nsec = (long)((iMS % 1000u) * 1000000u);
		nanosleep(&tReq, NULL);
	#endif
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

static inline XS_WsConnContext* XS_WsGetConnContext(xwsconn* pConn)
{
	if ( pConn == NULL || pConn->pStream == NULL ) {
		return NULL;
	}

	return (XS_WsConnContext*)xrtNetStreamGetUserData(pConn->pStream);
}

static inline void XS_WsTouch(XS_WsConnContext* objCtx)
{
	if ( objCtx == NULL ) {
		return;
	}

	objCtx->iLastActiveMS = XS_WsNowMS();
}

static inline void XS_WsTrackConn(XS_WsHandle* objHandle, XS_WsConnContext* objCtx)
{
	XS_WsConnContext** ppSlot;
	uint32 iPos;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL || objCtx == NULL ) {
		return;
	}

	xrtMutexLock(objHandle->pConnLock);
	iPos = xrtArrayAppend(objHandle->arrConn, 1);
	ppSlot = (XS_WsConnContext**)xrtArrayGet(objHandle->arrConn, iPos);
	if ( ppSlot ) {
		*ppSlot = objCtx;
	}
	xrtMutexUnlock(objHandle->pConnLock);
}

static inline void XS_WsUntrackConn(XS_WsHandle* objHandle, XS_WsConnContext* objCtx)
{
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL || objCtx == NULL ) {
		return;
	}

	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_WsConnContext** ppItem = (XS_WsConnContext**)xrtArrayGet(objHandle->arrConn, i);

		if ( ppItem && *ppItem == objCtx ) {
			xrtArrayRemove(objHandle->arrConn, i, 1);
			break;
		}
	}
	xrtMutexUnlock(objHandle->pConnLock);
}

static inline int64 XS_WsTrackedConnCount(XS_WsHandle* objHandle)
{
	int64 iCount;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		return 0;
	}

	xrtMutexLock(objHandle->pConnLock);
	iCount = (int64)objHandle->arrConn->Count;
	xrtMutexUnlock(objHandle->pConnLock);
	return iCount;
}

static inline void XS_WsRecordIdleClose(void)
{
	g_iXsWsIdleCloseCount++;
	g_tXsWsLastIdleCloseTime = xrtNow();
}

static inline void XS_WsRecordConnLimitClose(void)
{
	g_iXsWsConnLimitCloseCount++;
	g_tXsWsLastConnLimitCloseTime = xrtNow();
}

static inline void XS_WsRecordClose(xnet_result iReason)
{
	g_iXsWsLastCloseReason = (int64)iReason;
	g_tXsWsLastCloseTime = xrtNow();
	XS_WsMetricAdd(&g_iXsWsCloseCount, 1);
	if ( XS_WsMetricAdd(&g_iXsWsConnCurrent, -1) < 0 ) {
		XS_WsMetricAdd(&g_iXsWsConnCurrent, -XS_WsMetricGet(&g_iXsWsConnCurrent));
	}
}

static uint32 XS_WsIdleThread(ptr pArg)
{
	XS_WsHandle* objHandle = (XS_WsHandle*)pArg;

	while ( objHandle && !objHandle->bStopThread ) {
		XS_ServerConfig* objServer = objHandle->pOwner;
		uint32 iIdleTimeout = objServer ? objServer->IdleTimeout : 0u;

		if ( iIdleTimeout > 0 && objHandle->pConnLock && objHandle->arrConn ) {
			xarray arrClose = xrtArrayCreate(sizeof(XS_WsConnContext*), XRT_OBJMODE_LOCAL);
			int64 iNowMS = XS_WsNowMS();
			uint32 i;

			xrtMutexLock(objHandle->pConnLock);
			for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
				XS_WsConnContext** ppItem = (XS_WsConnContext**)xrtArrayGet(objHandle->arrConn, i);
				XS_WsConnContext* objCtx = ppItem ? *ppItem : NULL;

				if ( objCtx == NULL || objCtx->pConn == NULL || objCtx->bClosing ) {
					continue;
				}
				if ( (objCtx->iLastActiveMS > 0) && ((iNowMS - objCtx->iLastActiveMS) >= (int64)iIdleTimeout) ) {
					XS_WsConnContext** ppClose;
					uint32 iPos;

					objCtx->bClosing = TRUE;
					iPos = xrtArrayAppend(arrClose, 1);
					ppClose = (XS_WsConnContext**)xrtArrayGet(arrClose, iPos);
					if ( ppClose ) {
						*ppClose = objCtx;
					}
				}
			}
			xrtMutexUnlock(objHandle->pConnLock);

			for ( i = 1; i <= arrClose->Count; i++ ) {
				XS_WsConnContext** ppClose = (XS_WsConnContext**)xrtArrayGet(arrClose, i);
				XS_WsConnContext* objCtx = ppClose ? *ppClose : NULL;
				XS_ServerConfig* objServer;
				XS_HostConfig* objHost;
				xwsconn* pConn;
				xnetstream* pStream;

				if ( objCtx == NULL ) {
					continue;
				}
				objServer = objCtx->pServer;
				objHost = XS_WsResolveHost(objServer);
				pConn = objCtx->pConn;
				pStream = (pConn && pConn->pStream) ? pConn->pStream : NULL;
				XS_WsRecordClose((xnet_result)XWS_CLOSE_NORMAL);
				XS_WsRecordIdleClose();
				XS_LogInfo(
					"ws idle close: server=%s host=%s timeout=%u",
					objServer && objServer->Name ? objServer->Name : "(null)",
					objHost && objHost->Name ? objHost->Name : "(default)",
					(unsigned)(objServer ? objServer->IdleTimeout : 0u)
				);
				if ( objHost && objHost->procWsClose ) {
					objHost->procWsClose(objServer, objHost, pConn, (int)XWS_CLOSE_NORMAL);
				}
				XS_WsUntrackConn(objHandle, objCtx);
				if ( pStream ) {
					xrtNetStreamSetUserData(pStream, NULL);
					xrtNetStreamClose(pStream, 0u);
				}
				xrtFree(objCtx);
			}
			xrtArrayDestroy(arrClose);
		}

		XS_WsSleepMS(500);
	}

	return 0;
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
	XS_WsHandle* objHandle = objServer ? (XS_WsHandle*)objServer->pHandle : NULL;
	XS_WsConnContext* objCtx = NULL;
	(void)pServer;

	if ( pConn && pConn->pStream ) {
		objCtx = (XS_WsConnContext*)xrtCalloc(1, sizeof(XS_WsConnContext));
		if ( objCtx ) {
			objCtx->pServer = objServer;
			objCtx->pConn = pConn;
			objCtx->pTracker = objHandle;
			XS_WsTouch(objCtx);
			xrtNetStreamSetUserData(pConn->pStream, objCtx);
			XS_WsTrackConn(objHandle, objCtx);
		}
	}
	XS_WsMetricAdd(&g_iXsWsOpenCount, 1);
	XS_WsMetricUpdateMax(&g_iXsWsConnPeak, XS_WsMetricAdd(&g_iXsWsConnCurrent, 1));
	if ( objServer && objServer->ConnLimit > 0u && XS_WsTrackedConnCount(objHandle) > (int64)objServer->ConnLimit ) {
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		XS_WsRecordConnLimitClose();
		XS_LogWarn(
			"ws conn limit exceeded: server=%s current=%lld limit=%u",
			objServer->Name ? objServer->Name : "(null)",
			(long long)XS_WsTrackedConnCount(objHandle),
			(unsigned)objServer->ConnLimit
		);
		if ( pConn && pConn->pStream ) {
			xrtNetStreamClose(pConn->pStream, 0u);
		}
		return;
	}
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
	XS_WsConnContext* objCtx = XS_WsGetConnContext(pConn);
	bool bHandled = FALSE;
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsTextCount, 1);
	XS_WsTouch(objCtx);
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
	XS_WsConnContext* objCtx = XS_WsGetConnContext(pConn);
	bool bHandled = FALSE;
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsBinaryCount, 1);
	XS_WsTouch(objCtx);
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
	XS_WsConnContext* objCtx = XS_WsGetConnContext(pConn);
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsPingCount, 1);
	XS_WsTouch(objCtx);
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
	XS_WsConnContext* objCtx = XS_WsGetConnContext(pConn);
	(void)pServer;

	XS_WsMetricAdd(&g_iXsWsPongCount, 1);
	XS_WsTouch(objCtx);
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
	XS_WsConnContext* objCtx = XS_WsGetConnContext(pConn);
	(void)pServer;

	if ( objCtx == NULL ) {
		return;
	}

	XS_WsRecordClose(iReason);
	XS_WsRecordRemote(pConn);
	
	XS_LogInfo(
		"ws close: server=%s host=%s reason=%d",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost && objHost->Name ? objHost->Name : "(default)",
		(int)iReason
	);
	
	if ( objHost && objHost->procWsClose ) {
		objHost->procWsClose(objServer, objHost, pConn, (int)iReason);
	}

	if ( objCtx ) {
		XS_WsUntrackConn((XS_WsHandle*)objCtx->pTracker, objCtx);
		if ( pConn && pConn->pStream ) {
			xrtNetStreamSetUserData(pConn->pStream, NULL);
		}
		xrtFree(objCtx);
	}
}

static void XS_WsOnError(ptr pOwner, xwsserver* pServer, xwsconn* pConn, int iSysErr)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_WsConnContext* objCtx = XS_WsGetConnContext(pConn);
	(void)pServer;

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
	
	if ( objServer == NULL ) {
		return;
	}
	
	objHandle = (XS_WsHandle*)objServer->pHandle;
	pServer = objHandle ? objHandle->pServer : NULL;
	if ( objHandle ) {
		objHandle->bStopThread = TRUE;
		if ( objHandle->hIdleThread ) {
			xrtThreadWait(objHandle->hIdleThread);
			xrtThreadDestroy(objHandle->hIdleThread);
			objHandle->hIdleThread = NULL;
		}
	}
	if ( pServer ) {
		xrtWsServerStop(pServer);
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
		"ws stop: server=%s",
		objServer->Name ? objServer->Name : "(null)"
	);
}

#endif
