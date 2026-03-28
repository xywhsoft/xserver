/* extracted runtime surface */

static inline void XS_CustomRecordRemote(xnetstream* pStream);

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

static inline void XS_CustomAbortTrackedConns(XS_CustomHandle* objHandle)
{
	xarray arrClose;
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		return;
	}

	arrClose = xrtArrayCreate(sizeof(xnetstream*), XRT_OBJMODE_LOCAL);
	if ( arrClose == NULL ) {
		return;
	}

	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_CustomConnContext** ppItem = (XS_CustomConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_CustomConnContext* objCtx = ppItem ? *ppItem : NULL;
		xnetstream* pStream = objCtx ? objCtx->pStream : NULL;
		xnetstream** ppClose;
		uint32 iPos;

		if ( objCtx == NULL || pStream == NULL ) {
			continue;
		}

		objCtx->bClosing = TRUE;
		iPos = xrtArrayAppend(arrClose, 1);
		ppClose = (xnetstream**)xrtArrayGet(arrClose, iPos);
		if ( ppClose ) {
			*ppClose = pStream;
		}
	}
	xrtMutexUnlock(objHandle->pConnLock);

	for ( i = 1; i <= arrClose->Count; i++ ) {
		xnetstream** ppClose = (xnetstream**)xrtArrayGet(arrClose, i);

		if ( ppClose && *ppClose ) {
			xrtNetStreamClose(*ppClose, XNET_CLOSE_F_ABORT);
		}
	}
	xrtArrayDestroy(arrClose);
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
			xarray arrClose = xrtArrayCreate(sizeof(XS_CustomConnContext*), XRT_OBJMODE_LOCAL);
			uint32 i;

			xrtMutexLock(objHandle->pConnLock);
			for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
				XS_CustomConnContext** ppItem = (XS_CustomConnContext**)xrtArrayGet(objHandle->arrConn, i);
				XS_CustomConnContext* objCtx = (ppItem ? *ppItem : NULL);

				if ( objCtx == NULL || objCtx->pStream == NULL || objCtx->bClosing ) {
					continue;
				}
				if ( (objCtx->iLastActiveMS > 0) && ((iNowMS - objCtx->iLastActiveMS) >= (int64)iIdleTimeout) ) {
					XS_CustomConnContext** ppClose;
					uint32 iPos;

					objCtx->bClosing = TRUE;
					iPos = xrtArrayAppend(arrClose, 1);
					ppClose = (XS_CustomConnContext**)xrtArrayGet(arrClose, iPos);
					if ( ppClose ) {
						*ppClose = objCtx;
					}
				}
			}
			xrtMutexUnlock(objHandle->pConnLock);

			for ( i = 1; i <= arrClose->Count; i++ ) {
				XS_CustomConnContext** ppClose = (XS_CustomConnContext**)xrtArrayGet(arrClose, i);
				XS_CustomConnContext* objCtx = ppClose ? *ppClose : NULL;

				if ( objCtx && objCtx->pStream ) {
					XS_CustomRecordRemote(objCtx->pStream);
					XS_CustomRecordIdleClose();
					XS_LogInfo(
						"custom idle close: server=%s timeout=%u remote=%s",
						objServer && objServer->Name ? objServer->Name : "(null)",
						(unsigned)iIdleTimeout,
						g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)"
					);
					xrtNetStreamClose(objCtx->pStream, 0u);
				}
			}
			xrtArrayDestroy(arrClose);
		}

		xrtSleep(500);
	}

	return 0;
}

static inline void XS_CustomSnapshotInvalidRemote(void);
static inline void XS_CustomSnapshotErrorRemote(void);

static inline void XS_CustomRecordInvalid(const char* sReason)
{
	g_iXsCustomInvalidCount++;
	g_tXsCustomLastInvalidTime = xrtNow();
	if ( sReason && strcmp(sReason, "recv limit exceeded") == 0 ) {
		XS_CustomRecordRecvLimitClose();
	}
	XS_CustomRecordRejectEvent(sReason);
	XS_CustomSnapshotInvalidRemote();
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

static inline void XS_CustomSnapshotRemote(char* sValue, size_t iValueSize)
{
	if ( sValue == NULL || iValueSize == 0 ) {
		return;
	}
	if ( g_sXsCustomLastRemote[0] ) {
		strncpy(sValue, g_sXsCustomLastRemote, iValueSize - 1);
		sValue[iValueSize - 1] = '\0';
	} else {
		sValue[0] = '\0';
	}
}

static inline void XS_CustomSnapshotInvalidRemote(void)
{
	XS_CustomSnapshotRemote(g_sXsCustomLastInvalidRemote, sizeof(g_sXsCustomLastInvalidRemote));
}

static inline void XS_CustomSnapshotErrorRemote(void)
{
	XS_CustomSnapshotRemote(g_sXsCustomLastErrorRemote, sizeof(g_sXsCustomLastErrorRemote));
}
