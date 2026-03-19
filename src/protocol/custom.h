#ifndef XS_PROTOCOL_CUSTOM_H
#define XS_PROTOCOL_CUSTOM_H

typedef struct {
	xnetlistener* pListener;
	XS_ServerConfig* pServer;
	xthread hAcceptThread;
	volatile bool bStopAccept;
} XS_CustomHandle;

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
	(void)pListener;
	
	xrtNetStreamSetUserData(pStream, objServer);
	return TRUE;
}

static void XS_CustomOnOpen(ptr pOwner, xnetstream* pStream)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	
	g_iXsCustomOpenCount++;
	g_iXsCustomConnCurrent++;
	if ( g_iXsCustomConnCurrent > g_iXsCustomConnPeak ) {
		g_iXsCustomConnPeak = g_iXsCustomConnCurrent;
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
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
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
		g_iXsCustomInvalidCount++;
		XS_LogWarn(
			"custom recv limit exceeded: server=%s stream=%p bytes=%u limit=%u",
			objServer->Name ? objServer->Name : "(null)",
			(void*)pStream,
			(unsigned)iLen,
			(unsigned)objServer->RecvLimit
		);
		xrtNetChainClear(pChain);
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
	g_tXsCustomLastTime = xrtNow();
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
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	
	g_iXsCustomCloseCount++;
	g_iXsCustomLastCloseReason = (int64)iReason;
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
}

static void XS_CustomOnError(ptr pOwner, xnetstream* pStream, int iSysErr)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	(void)pStream;
	
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
		if ( objHandle->pListener ) {
			xrtNetListenerDestroy(objHandle->pListener);
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
