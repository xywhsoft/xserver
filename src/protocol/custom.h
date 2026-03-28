#ifndef XS_PROTOCOL_CUSTOM_H
#define XS_PROTOCOL_CUSTOM_H

typedef struct {
	XS_ServerConfig* pServer;
	xnetstream* pStream;
	int64 iLastActiveMS;
	volatile bool bClosing;
	volatile bool bStopRejectRecorded;
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

#include "../manage/custom_runtime.h"

static inline bool XS_CustomIsTransportRecvLimitError(XS_CustomConnContext* objCtx, xnetstream* pStream, int iSysErr)
{
	if ( iSysErr != -1 ) {
		return FALSE;
	}
	if ( objCtx == NULL || pStream == NULL ) {
		return FALSE;
	}
	if ( pStream->iRecvLimit == 0u ) {
		return FALSE;
	}
	if ( pStream->bClosing ) {
		return FALSE;
	}
	if ( pStream->pProxyState != NULL ) {
		return FALSE;
	}
	if ( pStream->pTls != NULL && !xrtNetTlsSessionIsReady(pStream->pTls) ) {
		return FALSE;
	}
	if ( xrtNetChainBytes(&pStream->tRxChain) == 0u ) {
		return FALSE;
	}
	return TRUE;
}

static uint32 XS_CustomAcceptThread(ptr pArg)
{
	XS_CustomHandle* objHandle = (XS_CustomHandle*)pArg;
	
	while ( objHandle && objHandle->pListener && (!objHandle->bStopAccept || objHandle->pListener->bRunning) ) {
		xnetstream* pStream = NULL;
		uint32 iAcceptTimeout = objHandle->bStopAccept ? 50u : 500u;
		xnet_result iRet = xrtNetListenerAcceptTimeout(objHandle->pListener, iAcceptTimeout, &pStream);
		
		if ( iRet == XRT_NET_TIMEOUT ) {
			continue;
		}
		if ( iRet == XRT_NET_OK ) {
			XS_CustomConnContext* objCtx = pStream ? (XS_CustomConnContext*)xrtNetStreamGetUserData(pStream) : NULL;

			if ( objHandle->bStopAccept ) {
				if ( objCtx ) {
					objCtx->bClosing = TRUE;
					objCtx->bStopRejectRecorded = TRUE;
				}
				XS_CustomRecordRemote(pStream);
				XS_CustomRecordRejectEvent("server_stopping");
				XS_LogWarn(
					"custom accept rejected while stopping: server=%s stream=%p remote=%s",
					objHandle->pServer && objHandle->pServer->Name ? objHandle->pServer->Name : "(null)",
					(void*)pStream,
					g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
				);
				xrtNetStreamClose(pStream, XNET_CLOSE_F_ABORT);
				continue;
			}
			XS_CustomRecordRemote(pStream);
			XS_LogInfo(
				"custom accept: server=%s stream=%p remote=%s",
				objHandle->pServer && objHandle->pServer->Name ? objHandle->pServer->Name : "(null)",
				(void*)pStream,
				g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
			);
			continue;
		}
		if ( objHandle->bStopAccept ) {
			break;
		}
		
		XS_LogWarn(
			"custom accept failed: server=%s addr=%s code=%d",
			objHandle->pServer && objHandle->pServer->Name ? objHandle->pServer->Name : "(null)",
			(objHandle && objHandle->pServer && objHandle->pServer->Addr) ? objHandle->pServer->Addr : "(null)",
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
	if ( objHandle && objHandle->bStopAccept ) {
		XS_CustomRecordRemote(pStream);
		XS_CustomRecordRejectEvent("server_stopping");
		XS_LogWarn(
			"custom accept rejected while stopping: server=%s stream=%p remote=%s",
			objServer && objServer->Name ? objServer->Name : "(null)",
			(void*)pStream,
			g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
		);
		xrtNetStreamClose(pStream, XNET_CLOSE_F_ABORT);
		return FALSE;
	}

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

	if ( objHandle && objHandle->bStopAccept ) {
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		XS_CustomRecordRemote(pStream);
		if ( objCtx == NULL || !objCtx->bStopRejectRecorded ) {
			XS_CustomRecordRejectEvent("server_stopping");
			XS_LogWarn(
				"custom open rejected while stopping: server=%s stream=%p remote=%s",
				objServer && objServer->Name ? objServer->Name : "(null)",
				(void*)pStream,
				g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
			);
		}
		xrtNetStreamClose(pStream, XNET_CLOSE_F_ABORT);
		return;
	}
	
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
			"custom conn limit exceeded: server=%s current=%lld limit=%u remote=%s",
			objServer->Name ? objServer->Name : "(null)",
			(long long)XS_CustomTrackedConnCount(objHandle),
			(unsigned)objServer->ConnLimit,
			g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
		);
		xrtNetStreamClose(pStream, 0u);
		return;
	}
	XS_LogInfo(
		"custom open: server=%s stream=%p script_open=%s script_data=%s remote=%s",
		objServer && objServer->Name ? objServer->Name : "(null)"
		,
		(void*)pStream,
		(objServer && objServer->procStreamOpen) ? "true" : "false",
		(objServer && objServer->procStreamData) ? "true" : "false",
		g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
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
		XS_CustomRecordRemote(pStream);
		XS_LogInfo(
			"custom recv ignored: server=%s stream=%p bytes=0 remote=%s",
			objServer && objServer->Name ? objServer->Name : "(null)",
			(void*)pStream,
			g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
		);
		return;
	}

	if ( (objServer) && (objServer->RecvLimit > 0) && ((uint32)iLen > objServer->RecvLimit) ) {
		XS_CustomRecordRemote(pStream);
		XS_CustomRecordInvalid("recv limit exceeded");
		XS_LogWarn(
			"custom recv limit exceeded: server=%s stream=%p bytes=%u limit=%u remote=%s",
			objServer->Name ? objServer->Name : "(null)",
			(void*)pStream,
			(unsigned)iLen,
			(unsigned)objServer->RecvLimit,
			g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
		);
		xrtNetChainClear(pChain);
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		xrtNetStreamClose(pStream, XNET_CLOSE_F_ABORT);
		return;
	}
	
	XS_CustomRecordRemote(pStream);
	XS_LogInfo(
		"custom recv: server=%s stream=%p bytes=%u script_data=%s remote=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		(void*)pStream,
		(unsigned)iLen,
		(objServer && objServer->procStreamData) ? "true" : "false",
		g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
	);
	
	pBuf = (char*)xrtMalloc(iLen);
	if ( pBuf == NULL ) {
		XS_CustomRecordRemote(pStream);
		XS_LogWarn(
			"custom recv alloc failed: server=%s stream=%p bytes=%u remote=%s",
			objServer && objServer->Name ? objServer->Name : "(null)",
			(void*)pStream,
			(unsigned)iLen,
			g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
		);
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
		XS_LogInfo(
			"custom recv fallback echo: server=%s stream=%p bytes=%u remote=%s",
			objServer && objServer->Name ? objServer->Name : "(null)",
			(void*)pStream,
			(unsigned)iLen,
			g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
		);
		(void)xrtNetStreamSend(pStream, pBuf, iLen);
		g_iXsCustomSendCount++;
		g_iXsCustomSendBytes += (int64)iLen;
	} else {
		XS_LogInfo(
			"custom recv handled by script: server=%s stream=%p bytes=%u remote=%s",
			objServer && objServer->Name ? objServer->Name : "(null)",
			(void*)pStream,
			(unsigned)iLen,
			g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
		);
	}
	
	xrtFree(pBuf);
}

static void XS_CustomOnClose(ptr pOwner, xnetstream* pStream, xnet_result iReason)
{
	XS_CustomConnContext* objCtx = (XS_CustomConnContext*)pOwner;
	if ( objCtx == NULL && pStream != NULL ) {
		objCtx = (XS_CustomConnContext*)xrtNetStreamGetUserData(pStream);
	}
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;
	XS_CustomHandle* objHandle = objServer ? (XS_CustomHandle*)objServer->pHandle : NULL;
	
	g_iXsCustomCloseCount++;
	g_iXsCustomLastCloseReason = (int64)iReason;
	g_tXsCustomLastCloseTime = xrtNow();
	g_iXsCustomConnCurrent--;
	if ( g_iXsCustomConnCurrent < 0 ) {
		g_iXsCustomConnCurrent = 0;
	}
	XS_CustomRecordRemote(pStream);
	XS_LogInfo(
		"custom close: server=%s reason=%d remote=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		(int)iReason,
		g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
	);
	if ( objServer && objServer->procStreamClose ) {
		objServer->procStreamClose(objServer, pStream, (int)iReason);
	}
	if ( objCtx ) {
		XS_CustomUntrackConn(objHandle, objCtx);
		xrtNetStreamSetUserData(pStream, NULL);
		xrtFree(objCtx);
	}
}

static void XS_CustomOnError(ptr pOwner, xnetstream* pStream, int iSysErr)
{
	XS_CustomConnContext* objCtx = (XS_CustomConnContext*)pOwner;
	if ( objCtx == NULL && pStream != NULL ) {
		objCtx = (XS_CustomConnContext*)xrtNetStreamGetUserData(pStream);
	}
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;

	if ( XS_CustomIsTransportRecvLimitError(objCtx, pStream, iSysErr) ) {
		XS_CustomRecordRemote(pStream);
		XS_CustomRecordInvalid("recv limit exceeded");
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		XS_LogWarn(
			"custom transport recv limit exceeded: server=%s stream=%p limit=%u remote=%s",
			objServer && objServer->Name ? objServer->Name : "(null)",
			(void*)pStream,
			(unsigned)(pStream ? pStream->iRecvLimit : 0u),
			g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
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
	XS_CustomRecordRemote(pStream);
	XS_CustomSnapshotErrorRemote();
	XS_LogWarn(
		"custom error: server=%s sys=%d remote=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		iSysErr,
		g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
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
		xrtSleep(50);
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
		if ( iRemainConn > 0 ) {
			XS_CustomAbortTrackedConns(objHandle);
			iRemainConn = XS_CustomWaitTrackedConnDrain(objHandle, 1000u);
		}
		if ( iClosedConn > 0 || iRemainConn > 0 ) {
			XS_CustomRecordStopCleanup(iClosedConn, iRemainConn);
			XS_LogInfo(
				"custom stop cleanup: server=%s addr=%s closed=%lld remain=%lld",
				objServer->Name ? objServer->Name : "(null)",
				objServer->Addr ? objServer->Addr : "(null)",
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
		"custom stop: server=%s addr=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)"
	);
}

#endif
