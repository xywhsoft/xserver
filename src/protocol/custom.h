#ifndef XS_PROTOCOL_CUSTOM_H
#define XS_PROTOCOL_CUSTOM_H

typedef struct {
	XS_ServerConfig* pServer;
	xnetstream* pStream;
	int64 iLastActiveMS;
	volatile bool bClosing;
} XS_CustomConnContext;

typedef struct {
	xnetlistener* pListener;
	XS_ServerConfig* pServer;
	xthread hAcceptThread;
	xthread hIdleThread;
	volatile bool bStopAccept;
	xmutex pConnLock;
	xarray arrConn;
} XS_CustomHandle;

static inline void XS_CustomTouch(XS_CustomConnContext* objCtx)
{
	if ( objCtx == NULL ) {
		return;
	}

	objCtx->iLastActiveMS = XS_XtpNowMS();
}

static inline void XS_CustomTrackConn(XS_CustomHandle* objHandle, XS_CustomConnContext* objCtx)
{
	XS_CustomConnContext** ppSlot;
	uint32 iPos;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL || objCtx == NULL ) {
		return;
	}

	xrtMutexLock(objHandle->pConnLock);
	iPos = xrtArrayAppend(objHandle->arrConn, 1);
	ppSlot = (XS_CustomConnContext**)xrtArrayGet(objHandle->arrConn, iPos);
	if ( ppSlot ) {
		*ppSlot = objCtx;
	}
	xrtMutexUnlock(objHandle->pConnLock);
}

static inline void XS_CustomUntrackConn(XS_CustomHandle* objHandle, XS_CustomConnContext* objCtx)
{
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL || objCtx == NULL ) {
		return;
	}

	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_CustomConnContext** ppItem = (XS_CustomConnContext**)xrtArrayGet(objHandle->arrConn, i);

		if ( ppItem && *ppItem == objCtx ) {
			xrtArrayRemove(objHandle->arrConn, i, 1);
			break;
		}
	}
	xrtMutexUnlock(objHandle->pConnLock);
}

static inline int64 XS_CustomTrackedConnCount(XS_CustomHandle* objHandle)
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

static inline int64 XS_CustomCloseTrackedConns(XS_CustomHandle* objHandle)
{
	xarray arrClose;
	int64 iCloseCount;
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		return 0;
	}

	arrClose = xrtArrayCreate(sizeof(xnetstream*), XRT_OBJMODE_LOCAL);
	if ( arrClose == NULL ) {
		return 0;
	}

	iCloseCount = 0;
	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_CustomConnContext** ppItem = (XS_CustomConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_CustomConnContext* objCtx = ppItem ? *ppItem : NULL;
		xnetstream** ppClose;
		uint32 iPos;

		if ( objCtx == NULL || objCtx->pStream == NULL ) {
			continue;
		}

		objCtx->bClosing = TRUE;
		iPos = xrtArrayAppend(arrClose, 1);
		ppClose = (xnetstream**)xrtArrayGet(arrClose, iPos);
		if ( ppClose ) {
			*ppClose = objCtx->pStream;
			iCloseCount++;
		}
	}
	xrtMutexUnlock(objHandle->pConnLock);

	for ( i = 1; i <= arrClose->Count; i++ ) {
		xnetstream** ppClose = (xnetstream**)xrtArrayGet(arrClose, i);

		if ( ppClose && *ppClose ) {
			xrtNetStreamClose(*ppClose, 0u);
		}
	}
	xrtArrayDestroy(arrClose);
	return iCloseCount;
}

static inline int64 XS_CustomWaitTrackedConnDrain(XS_CustomHandle* objHandle, uint32 iTimeoutMS)
{
	uint32 iWaitedMS = 0;
	int64 iRemain = XS_CustomTrackedConnCount(objHandle);

	while ( iRemain > 0 && iWaitedMS < iTimeoutMS ) {
		xrtSleep(20);
		iWaitedMS += 20;
		iRemain = XS_CustomTrackedConnCount(objHandle);
	}

	return iRemain;
}

static inline void XS_CustomRecordIdleClose(void)
{
	g_iXsCustomIdleCloseCount++;
	g_tXsCustomLastIdleCloseTime = xrtNow();
}

static inline void XS_CustomRecordConnLimitClose(void)
{
	g_iXsCustomConnLimitCloseCount++;
	g_tXsCustomLastConnLimitCloseTime = xrtNow();
	XS_CustomRecordRejectEvent("conn_limit");
}

static inline void XS_CustomRecordRecvLimitClose(void)
{
	g_iXsCustomRecvLimitCloseCount++;
	g_tXsCustomLastRecvLimitCloseTime = xrtNow();
}

static uint32 XS_CustomIdleThread(ptr pArg)
{
	XS_CustomHandle* objHandle = (XS_CustomHandle*)pArg;

	while ( objHandle && !objHandle->bStopAccept ) {
		XS_ServerConfig* objServer = objHandle->pServer;
		uint32 iIdleTimeout = objServer ? objServer->IdleTimeout : 0u;

		if ( iIdleTimeout > 0u && objHandle->pConnLock && objHandle->arrConn ) {
			int64 iNowMS = XS_XtpNowMS();
			xarray arrClose = xrtArrayCreate(sizeof(xnetstream*), XRT_OBJMODE_LOCAL);
			uint32 i;

			xrtMutexLock(objHandle->pConnLock);
			for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
				XS_CustomConnContext** ppItem = (XS_CustomConnContext**)xrtArrayGet(objHandle->arrConn, i);
				XS_CustomConnContext* objCtx = (ppItem ? *ppItem : NULL);

				if ( objCtx == NULL || objCtx->pStream == NULL || objCtx->bClosing ) {
					continue;
				}
				if ( (objCtx->iLastActiveMS > 0) && ((iNowMS - objCtx->iLastActiveMS) >= (int64)iIdleTimeout) ) {
					xnetstream** ppClose;
					uint32 iPos;

					objCtx->bClosing = TRUE;
					iPos = xrtArrayAppend(arrClose, 1);
					ppClose = (xnetstream**)xrtArrayGet(arrClose, iPos);
					if ( ppClose ) {
						*ppClose = objCtx->pStream;
					}
				}
			}
			xrtMutexUnlock(objHandle->pConnLock);

			for ( i = 1; i <= arrClose->Count; i++ ) {
				xnetstream** ppClose = (xnetstream**)xrtArrayGet(arrClose, i);

				if ( ppClose && *ppClose ) {
					XS_CustomRecordIdleClose();
					xrtNetStreamClose(*ppClose, 0u);
				}
			}
			xrtArrayDestroy(arrClose);
		}

		xrtSleep(500);
	}

	return 0;
}

static inline void XS_CustomRecordInvalid(const char* sReason)
{
	g_iXsCustomInvalidCount++;
	g_tXsCustomLastInvalidTime = xrtNow();
	if ( sReason && strcmp(sReason, "recv limit exceeded") == 0 ) {
		XS_CustomRecordRecvLimitClose();
	}
	XS_CustomRecordRejectEvent(sReason);
	if ( sReason ) {
		strncpy(g_sXsCustomLastInvalidReason, sReason, sizeof(g_sXsCustomLastInvalidReason) - 1);
		g_sXsCustomLastInvalidReason[sizeof(g_sXsCustomLastInvalidReason) - 1] = '\0';
	} else {
		g_sXsCustomLastInvalidReason[0] = '\0';
	}
}

static inline void XS_CustomRecordRemote(xnetstream* pStream)
{
	const xnetaddr* pAddr;
	const char* sAddr;

	if ( pStream == NULL ) {
		return;
	}

	pAddr = xrtNetStreamRemoteAddr(pStream);
	sAddr = pAddr ? xrtNetAddrToStr(pAddr) : NULL;
	if ( sAddr && sAddr[0] ) {
		strncpy(g_sXsCustomLastRemote, sAddr, sizeof(g_sXsCustomLastRemote) - 1);
		g_sXsCustomLastRemote[sizeof(g_sXsCustomLastRemote) - 1] = '\0';
	} else {
		g_sXsCustomLastRemote[0] = '\0';
	}
}

static uint32 XS_CustomAcceptThread(ptr pArg)
{
	XS_CustomHandle* objHandle = (XS_CustomHandle*)pArg;
	
	while ( objHandle && objHandle->pListener && !objHandle->bStopAccept ) {
		xnetstream* pStream = NULL;
		xnet_result iRet = xrtNetListenerAcceptTimeout(objHandle->pListener, 500, &pStream);
		
		if ( iRet == XRT_NET_TIMEOUT ) {
			continue;
		}
		if ( iRet == XRT_NET_OK ) {
			XS_LogInfo(
				"custom accept: server=%s stream=%p",
				objHandle->pServer && objHandle->pServer->Name ? objHandle->pServer->Name : "(null)",
				(void*)pStream
			);
			continue;
		}
		if ( objHandle->bStopAccept ) {
			break;
		}
		
		XS_LogWarn(
			"custom accept failed: server=%s code=%d",
			objHandle->pServer && objHandle->pServer->Name ? objHandle->pServer->Name : "(null)",
			(int)iRet
		);
		xrtSleep(50);
	}
	
	return 0;
}

static bool XS_CustomOnAccept(ptr pOwner, xnetlistener* pListener, xnetstream* pStream)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_CustomHandle* objHandle;
	XS_CustomConnContext* objCtx;
	(void)pListener;

	objHandle = objServer ? (XS_CustomHandle*)objServer->pHandle : NULL;
	objCtx = (XS_CustomConnContext*)xrtCalloc(1, sizeof(XS_CustomConnContext));
	if ( objCtx == NULL ) {
		return FALSE;
	}

	objCtx->pServer = objServer;
	objCtx->pStream = pStream;
	XS_CustomTouch(objCtx);
	xrtNetStreamSetUserData(pStream, objCtx);
	XS_CustomTrackConn(objHandle, objCtx);
	return TRUE;
}

static void XS_CustomOnOpen(ptr pOwner, xnetstream* pStream)
{
	XS_CustomConnContext* objCtx = (XS_CustomConnContext*)pOwner;
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;
	XS_CustomHandle* objHandle = objServer ? (XS_CustomHandle*)objServer->pHandle : NULL;
	
	g_iXsCustomOpenCount++;
	g_iXsCustomConnCurrent++;
	if ( g_iXsCustomConnCurrent > g_iXsCustomConnPeak ) {
		g_iXsCustomConnPeak = g_iXsCustomConnCurrent;
	}
	XS_CustomTouch(objCtx);
	XS_CustomRecordRemote(pStream);
	if ( objServer && objServer->ConnLimit > 0u && XS_CustomTrackedConnCount(objHandle) > (int64)objServer->ConnLimit ) {
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		XS_CustomRecordConnLimitClose();
		XS_LogWarn(
			"custom conn limit exceeded: server=%s current=%lld limit=%u",
			objServer->Name ? objServer->Name : "(null)",
			(long long)XS_CustomTrackedConnCount(objHandle),
			(unsigned)objServer->ConnLimit
		);
		xrtNetStreamClose(pStream, 0u);
		return;
	}
	XS_LogInfo(
		"custom open: server=%s stream=%p script_open=%s script_data=%s",
		objServer && objServer->Name ? objServer->Name : "(null)"
		,
		(void*)pStream,
		(objServer && objServer->procStreamOpen) ? "true" : "false",
		(objServer && objServer->procStreamData) ? "true" : "false"
	);
	if ( objServer && objServer->procStreamOpen ) {
		objServer->procStreamOpen(objServer, pStream);
	}
}

static void XS_CustomOnRecv(ptr pOwner, xnetstream* pStream, xnetchain* pChain)
{
	XS_CustomConnContext* objCtx = (XS_CustomConnContext*)pOwner;
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;
	size_t iLen;
	char* pBuf;
	bool bHandled = FALSE;
	
	if ( pChain == NULL ) {
		return;
	}
	
	iLen = xrtNetChainBytes(pChain);
	if ( iLen == 0 ) {
		XS_LogInfo("custom recv ignored: empty chain");
		return;
	}

	if ( (objServer) && (objServer->RecvLimit > 0) && ((uint32)iLen > objServer->RecvLimit) ) {
		XS_CustomRecordRemote(pStream);
		XS_CustomRecordInvalid("recv limit exceeded");
		XS_LogWarn(
			"custom recv limit exceeded: server=%s stream=%p bytes=%u limit=%u",
			objServer->Name ? objServer->Name : "(null)",
			(void*)pStream,
			(unsigned)iLen,
			(unsigned)objServer->RecvLimit
		);
		xrtNetChainClear(pChain);
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		xrtNetStreamClose(pStream, XNET_CLOSE_F_ABORT);
		return;
	}
	
	XS_LogInfo(
		"custom recv: server=%s stream=%p bytes=%u script_data=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		(void*)pStream,
		(unsigned)iLen,
		(objServer && objServer->procStreamData) ? "true" : "false"
	);
	
	pBuf = (char*)xrtMalloc(iLen);
	if ( pBuf == NULL ) {
		XS_LogWarn("custom recv alloc failed: bytes=%u", (unsigned)iLen);
		xrtNetChainClear(pChain);
		return;
	}
	
	(void)xrtNetChainPeek(pChain, pBuf, iLen);
	xrtNetChainConsume(pChain, iLen);
	g_iXsCustomRecvCount++;
	g_iXsCustomRecvBytes += (int64)iLen;
	g_iXsCustomLastBytes = (int64)iLen;
	g_tXsCustomLastTime = xrtNow();
	XS_CustomTouch(objCtx);
	XS_CustomRecordRemote(pStream);
	if ( iLen > 0 ) {
		size_t iCopy = iLen;
		if ( iCopy >= sizeof(g_sXsCustomLastText) ) {
			iCopy = sizeof(g_sXsCustomLastText) - 1;
		}
		memcpy(g_sXsCustomLastText, pBuf, iCopy);
		g_sXsCustomLastText[iCopy] = '\0';
	} else {
		g_sXsCustomLastText[0] = '\0';
	}
	
	if ( objServer && objServer->procStreamData ) {
		bHandled = objServer->procStreamData(objServer, pStream, pBuf, iLen);
	}
	
	if ( !bHandled ) {
		XS_LogInfo("custom recv fallback echo: bytes=%u", (unsigned)iLen);
		(void)xrtNetStreamSend(pStream, pBuf, iLen);
		g_iXsCustomSendCount++;
		g_iXsCustomSendBytes += (int64)iLen;
	} else {
		XS_LogInfo("custom recv handled by script: bytes=%u", (unsigned)iLen);
	}
	
	xrtFree(pBuf);
}

static void XS_CustomOnClose(ptr pOwner, xnetstream* pStream, xnet_result iReason)
{
	XS_CustomConnContext* objCtx = (XS_CustomConnContext*)pOwner;
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;
	XS_CustomHandle* objHandle = objServer ? (XS_CustomHandle*)objServer->pHandle : NULL;
	
	g_iXsCustomCloseCount++;
	g_iXsCustomLastCloseReason = (int64)iReason;
	g_tXsCustomLastCloseTime = xrtNow();
	g_iXsCustomConnCurrent--;
	if ( g_iXsCustomConnCurrent < 0 ) {
		g_iXsCustomConnCurrent = 0;
	}
	XS_LogInfo(
		"custom close: server=%s reason=%d",
		objServer && objServer->Name ? objServer->Name : "(null)",
		(int)iReason
	);
	if ( objServer && objServer->procStreamClose ) {
		objServer->procStreamClose(objServer, pStream, (int)iReason);
	}
	XS_CustomUntrackConn(objHandle, objCtx);
	xrtNetStreamSetUserData(pStream, NULL);
	if ( objCtx ) {
		xrtFree(objCtx);
	}
}

static void XS_CustomOnError(ptr pOwner, xnetstream* pStream, int iSysErr)
{
	XS_CustomConnContext* objCtx = (XS_CustomConnContext*)pOwner;
	uint32 iInternalError = pStream ? xrtNetStreamTakeInternalError(pStream) : XNET_STREAM_INTERNAL_ERROR_NONE;
	if ( objCtx == NULL && pStream != NULL ) {
		objCtx = (XS_CustomConnContext*)xrtNetStreamGetUserData(pStream);
	}
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;

	if ( iInternalError == XNET_STREAM_INTERNAL_ERROR_RECV_LIMIT ) {
		XS_CustomRecordRemote(pStream);
		XS_CustomRecordInvalid("recv limit exceeded");
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		XS_LogWarn(
			"custom transport recv limit exceeded: server=%s stream=%p limit=%u",
			objServer && objServer->Name ? objServer->Name : "(null)",
			(void*)pStream,
			(unsigned)(pStream ? pStream->iRecvLimit : 0u)
		);
		return;
	}
	if ( objCtx && objCtx->bClosing ) {
		return;
	}
	if ( iSysErr == -1 ) {
		return;
	}
	
	g_iXsCustomErrorCount++;
	g_iXsCustomLastErrorCode = (int64)iSysErr;
	g_tXsCustomLastErrorTime = xrtNow();
	XS_LogWarn(
		"custom error: server=%s sys=%d",
		objServer && objServer->Name ? objServer->Name : "(null)",
		iSysErr
	);
}

static const xnetlistenerevents* XS_CustomListenerEvents(void)
{
	static const xnetlistenerevents tEvents = {
		XS_CustomOnAccept,
		NULL
	};
	
	return &tEvents;
}

static const xnetstreamevents* XS_CustomStreamEvents(void)
{
	static const xnetstreamevents tEvents = {
		XS_CustomOnOpen,
		XS_CustomOnRecv,
		NULL,
		XS_CustomOnClose,
		XS_CustomOnError,
		NULL,
		NULL
	};
	
	return &tEvents;
}

static inline bool XS_CustomInitServer(xnetengine* pEngine, XS_ServerConfig* objServer)
{
	xnetlistenconfig tCfg;
	XS_CustomHandle* objHandle;
	
	if ( objServer == NULL || pEngine == NULL ) {
		return FALSE;
	}
	
	objHandle = (XS_CustomHandle*)xrtCalloc(1, sizeof(XS_CustomHandle));
	if ( objHandle == NULL ) {
		XS_ReportError("custom init failed: alloc handle");
		return FALSE;
	}
	
	xrtNetListenConfigInit(&tCfg);
	if ( !XS_BuildBindAddr(objServer, FALSE, &tCfg.tBindAddr) ) {
		xrtFree(objHandle);
		XS_ReportError("custom init failed: invalid addr: %s", objServer->Addr ? objServer->Addr : "(null)");
		return FALSE;
	}
	tCfg.iBacklog = objServer->Backlog;
	tCfg.iRecvLimit = objServer->RecvLimit;
	
	objHandle->pListener = xrtNetListenerCreate(pEngine, &tCfg, XS_CustomListenerEvents(), XS_CustomStreamEvents(), objServer);
	if ( objHandle->pListener == NULL ) {
		xrtFree(objHandle);
		XS_ReportError("custom init failed: create listener");
		return FALSE;
	}

	objHandle->pConnLock = xrtMutexCreate();
	objHandle->arrConn = xrtArrayCreate(sizeof(XS_CustomConnContext*), XRT_OBJMODE_SHARED);
	if ( objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		if ( objHandle->arrConn ) {
			xrtArrayDestroy(objHandle->arrConn);
		}
		if ( objHandle->pConnLock ) {
			xrtMutexDestroy(objHandle->pConnLock);
		}
		xrtNetListenerDestroy(objHandle->pListener);
		xrtFree(objHandle);
		XS_ReportError("custom init failed: alloc conn tracker");
		return FALSE;
	}
	
	objHandle->pServer = objServer;
	objServer->pHandle = objHandle;
	XS_LogInfo(
		"custom init: server=%s addr=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)"
	);
	return TRUE;
}

static inline bool XS_CustomStartServer(XS_ServerConfig* objServer)
{
	XS_CustomHandle* objHandle;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	
	objHandle = (XS_CustomHandle*)objServer->pHandle;
	if ( objHandle == NULL || objHandle->pListener == NULL ) {
		XS_ReportError("custom start failed: handle is null");
		return FALSE;
	}
	
	if ( xrtNetListenerStart(objHandle->pListener) != XRT_NET_OK ) {
		XS_ReportError("custom start failed: listener start error");
		return FALSE;
	}
	objHandle->bStopAccept = FALSE;
	objHandle->hAcceptThread = xrtThreadCreate(XS_CustomAcceptThread, objHandle, 0);
	if ( objHandle->hAcceptThread == NULL ) {
		XS_ReportError("custom start failed: create accept thread error");
		xrtNetListenerStop(objHandle->pListener);
		return FALSE;
	}
	objHandle->hIdleThread = xrtThreadCreate(XS_CustomIdleThread, objHandle, 0);
	if ( objHandle->hIdleThread == NULL ) {
		XS_ReportError("custom start failed: create idle thread error");
		objHandle->bStopAccept = TRUE;
		xrtThreadWait(objHandle->hAcceptThread);
		xrtThreadDestroy(objHandle->hAcceptThread);
		objHandle->hAcceptThread = NULL;
		xrtNetListenerStop(objHandle->pListener);
		return FALSE;
	}
	
	XS_LogInfo(
		"custom start: server=%s addr=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)"
	);
	return TRUE;
}

static inline void XS_CustomStopServer(XS_ServerConfig* objServer)
{
	XS_CustomHandle* objHandle;
	int64 iClosedConn;
	int64 iRemainConn;
	
	if ( objServer == NULL ) {
		return;
	}
	
	objHandle = (XS_CustomHandle*)objServer->pHandle;
	if ( objHandle ) {
		objHandle->bStopAccept = TRUE;
		if ( objHandle->pListener ) {
			xrtNetListenerStop(objHandle->pListener);
		}
		if ( objHandle->hAcceptThread ) {
			xrtThreadWait(objHandle->hAcceptThread);
			xrtThreadDestroy(objHandle->hAcceptThread);
			objHandle->hAcceptThread = NULL;
		}
		if ( objHandle->hIdleThread ) {
			xrtThreadWait(objHandle->hIdleThread);
			xrtThreadDestroy(objHandle->hIdleThread);
			objHandle->hIdleThread = NULL;
		}
		iClosedConn = XS_CustomCloseTrackedConns(objHandle);
		iRemainConn = XS_CustomWaitTrackedConnDrain(objHandle, 500u);
		if ( iClosedConn > 0 || iRemainConn > 0 ) {
			XS_CustomRecordStopCleanup(iClosedConn, iRemainConn);
			XS_LogInfo(
				"custom stop cleanup: server=%s closed=%lld remain=%lld",
				objServer->Name ? objServer->Name : "(null)",
				(long long)iClosedConn,
				(long long)iRemainConn
			);
		}
		if ( objHandle->pListener ) {
			xrtNetListenerDestroy(objHandle->pListener);
		}
		if ( objHandle->arrConn ) {
			xrtArrayDestroy(objHandle->arrConn);
		}
		if ( objHandle->pConnLock ) {
			xrtMutexDestroy(objHandle->pConnLock);
		}
		xrtFree(objHandle);
		objServer->pHandle = NULL;
	}
	
	XS_LogInfo(
		"custom stop: server=%s",
		objServer->Name ? objServer->Name : "(null)"
	);
}

#endif
