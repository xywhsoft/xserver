/* extracted xtp runtime surface */

static inline void XS_XtpTouch(XS_XtpConnContext* objCtx)
{
	if ( objCtx == NULL ) {
		return;
	}

	objCtx->iLastActiveMS = XS_XtpNowMS();
}

static inline void XS_XtpTrackConn(XS_XtpHandle* objHandle, XS_XtpConnContext* objCtx)
{
	XS_XtpConnContext** ppSlot;
	uint32 iPos;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL || objCtx == NULL ) {
		return;
	}

	xrtMutexLock(objHandle->pConnLock);
	iPos = xrtArrayAppend(objHandle->arrConn, 1);
	ppSlot = (XS_XtpConnContext**)xrtArrayGet(objHandle->arrConn, iPos);
	if ( ppSlot ) {
		*ppSlot = objCtx;
	}
	xrtMutexUnlock(objHandle->pConnLock);
}

static inline void XS_XtpUntrackConn(XS_XtpHandle* objHandle, XS_XtpConnContext* objCtx)
{
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL || objCtx == NULL ) {
		return;
	}

	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_XtpConnContext** ppItem = (XS_XtpConnContext**)xrtArrayGet(objHandle->arrConn, i);

		if ( ppItem && *ppItem == objCtx ) {
			xrtArrayRemove(objHandle->arrConn, i, 1);
			break;
		}
	}
	xrtMutexUnlock(objHandle->pConnLock);
}

static inline int64 XS_XtpTrackedConnCount(XS_XtpHandle* objHandle)
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

static inline int64 XS_XtpCloseTrackedConns(XS_XtpHandle* objHandle)
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
		XS_XtpConnContext** ppItem = (XS_XtpConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_XtpConnContext* objCtx = ppItem ? *ppItem : NULL;
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

static inline void XS_XtpAbortTrackedConns(XS_XtpHandle* objHandle)
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
		XS_XtpConnContext** ppItem = (XS_XtpConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_XtpConnContext* objCtx = ppItem ? *ppItem : NULL;
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

static inline int64 XS_XtpWaitTrackedConnDrain(XS_XtpHandle* objHandle, uint32 iTimeoutMS)
{
	uint32 iWaitedMS = 0;
	int64 iRemain = XS_XtpTrackedConnCount(objHandle);

	while ( iRemain > 0 && iWaitedMS < iTimeoutMS ) {
		XS_XtpSleepMS(20);
		iWaitedMS += 20;
		iRemain = XS_XtpTrackedConnCount(objHandle);
	}

	return iRemain;
}

static inline void XS_XtpRecordIdleClose(void)
{
	g_iXsXtpIdleCloseCount++;
	g_tXsXtpLastIdleCloseTime = xrtNow();
}

static inline void XS_XtpRecordConnLimitClose(void)
{
	g_iXsXtpConnLimitCloseCount++;
	g_tXsXtpLastConnLimitCloseTime = xrtNow();
	XS_XtpRecordRejectEvent("conn_limit");
}

static inline void XS_XtpRecordRecvLimitClose(void)
{
	g_iXsXtpRecvLimitCloseCount++;
	g_tXsXtpLastRecvLimitCloseTime = xrtNow();
}

static inline int64 XS_XtpMetricGet(const volatile int64* pValue)
{
	return XS_HttpMetricGet(pValue);
}

static inline int64 XS_XtpMetricAdd(volatile int64* pValue, int64 iValue)
{
	return XS_HttpMetricAdd(pValue, iValue);
}

static inline void XS_XtpMetricUpdateMax(volatile int64* pValue, int64 iValue)
{
	XS_HttpMetricUpdateMax(pValue, iValue);
}

static inline void XS_XtpSnapshotInvalidRemote(void);
static inline void XS_XtpSnapshotErrorRemote(void);

static inline void XS_XtpRecordInvalid(const char* sReason)
{
	XS_XtpMetricAdd(&g_iXsXtpInvalidCount, 1);
	g_tXsXtpLastInvalidTime = xrtNow();
	if (
		(sReason && strcmp(sReason, "recv limit exceeded") == 0) ||
		(sReason && strcmp(sReason, "pack limit exceeded") == 0)
	) {
		XS_XtpRecordRecvLimitClose();
	}
	XS_XtpRecordRejectEvent(sReason);
	XS_XtpSnapshotInvalidRemote();
	if ( sReason && sReason[0] ) {
		strncpy(g_sXsXtpLastInvalidReason, sReason, sizeof(g_sXsXtpLastInvalidReason) - 1);
		g_sXsXtpLastInvalidReason[sizeof(g_sXsXtpLastInvalidReason) - 1] = '\0';
	} else {
		g_sXsXtpLastInvalidReason[0] = '\0';
	}
}

static inline void XS_XtpRecordRemote(xnetstream* pStream)
{
	const xnetaddr* pAddr;
	const char* sAddr;

	if ( pStream == NULL ) {
		return;
	}

	pAddr = xrtNetStreamRemoteAddr(pStream);
	sAddr = pAddr ? xrtNetAddrToStr(pAddr) : NULL;
	if ( (!sAddr || !sAddr[0]) && (pStream->hSocket != XSOCKET_INVALID) ) {
		if ( __xnetSocketUpdateRemoteAddr(pStream->hSocket, &pStream->tRemoteAddr) ) {
			pAddr = &pStream->tRemoteAddr;
			sAddr = xrtNetAddrToStr(pAddr);
		}
	}
	if ( sAddr && sAddr[0] ) {
		strncpy(g_sXsXtpLastRemote, sAddr, sizeof(g_sXsXtpLastRemote) - 1);
		g_sXsXtpLastRemote[sizeof(g_sXsXtpLastRemote) - 1] = '\0';
	} else {
		g_sXsXtpLastRemote[0] = '\0';
	}
}

static inline void XS_XtpSnapshotRemote(char* sValue, size_t iValueSize)
{
	if ( sValue == NULL || iValueSize == 0 ) {
		return;
	}
	if ( g_sXsXtpLastRemote[0] ) {
		strncpy(sValue, g_sXsXtpLastRemote, iValueSize - 1);
		sValue[iValueSize - 1] = '\0';
	} else {
		sValue[0] = '\0';
	}
}

static inline void XS_XtpSnapshotInvalidRemote(void)
{
	XS_XtpSnapshotRemote(g_sXsXtpLastInvalidRemote, sizeof(g_sXsXtpLastInvalidRemote));
}

static inline void XS_XtpSnapshotErrorRemote(void)
{
	XS_XtpSnapshotRemote(g_sXsXtpLastErrorRemote, sizeof(g_sXsXtpLastErrorRemote));
}
