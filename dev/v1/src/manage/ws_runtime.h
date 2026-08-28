/* extracted runtime surface */

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

static inline XS_WsConnContext* XS_WsFindConnContext(XS_WsHandle* objHandle, xwsconn* pConn)
{
	XS_WsConnContext* objResult = NULL;
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL || pConn == NULL ) {
		return NULL;
	}

	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_WsConnContext** ppItem = (XS_WsConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_WsConnContext* objCtx = ppItem ? *ppItem : NULL;

		if ( objCtx && objCtx->pConn == pConn ) {
			objResult = objCtx;
			break;
		}
	}
	xrtMutexUnlock(objHandle->pConnLock);
	return objResult;
}

static inline XS_WsConnContext* XS_WsGetConnContext(XS_ServerConfig* objServer, xwsconn* pConn)
{
	XS_WsHandle* objHandle = objServer ? (XS_WsHandle*)objServer->pHandle : NULL;

	return XS_WsFindConnContext(objHandle, pConn);
}

static inline void XS_WsTouch(XS_WsConnContext* objCtx)
{
	if ( objCtx == NULL ) {
		return;
	}

	objCtx->iLastActiveMS = XS_WsNowMS();
}

static inline void XS_WsCopyContextName(char* sDst, size_t iDstSize, const char* sValue)
{
	if ( sDst == NULL || iDstSize == 0 ) {
		return;
	}

	if ( sValue && sValue[0] ) {
		strncpy(sDst, sValue, iDstSize - 1);
		sDst[iDstSize - 1] = '\0';
	} else {
		sDst[0] = '\0';
	}
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

static inline int64 XS_WsCloseTrackedConns(XS_WsHandle* objHandle)
{
	xarray arrClose;
	int64 iCloseCount;
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		return 0;
	}

	arrClose = xrtArrayCreate(sizeof(XS_WsConnContext*), XRT_OBJMODE_LOCAL);
	if ( arrClose == NULL ) {
		return 0;
	}

	iCloseCount = 0;
	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_WsConnContext** ppItem = (XS_WsConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_WsConnContext* objCtx = ppItem ? *ppItem : NULL;
		XS_WsConnContext** ppClose;
		uint32 iPos;

		if ( objCtx == NULL || objCtx->pStream == NULL ) {
			continue;
		}

		objCtx->bClosing = TRUE;
		iPos = xrtArrayAppend(arrClose, 1);
		ppClose = (XS_WsConnContext**)xrtArrayGet(arrClose, iPos);
		if ( ppClose ) {
			*ppClose = objCtx;
			iCloseCount++;
		}
	}
	xrtMutexUnlock(objHandle->pConnLock);

	for ( i = 1; i <= arrClose->Count; i++ ) {
		XS_WsConnContext** ppClose = (XS_WsConnContext**)xrtArrayGet(arrClose, i);
		XS_WsConnContext* objCtx = ppClose ? *ppClose : NULL;

		if ( objCtx == NULL || objCtx->pStream == NULL ) {
			continue;
		}
		if ( objCtx->pConn && xrtWsConnIsOpen(objCtx->pConn) ) {
			if ( xrtWsConnClose(objCtx->pConn, XWS_CLOSE_GOING_AWAY, "server stop") == XRT_NET_OK ) {
				continue;
			}
		}
		xrtNetStreamClose(objCtx->pStream, 0u);
	}
	xrtArrayDestroy(arrClose);
	return iCloseCount;
}

static inline void XS_WsAbortTrackedConns(XS_WsHandle* objHandle)
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
		XS_WsConnContext** ppItem = (XS_WsConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_WsConnContext* objCtx = ppItem ? *ppItem : NULL;
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

static inline void XS_WsDetachTrackedConns(XS_WsHandle* objHandle)
{
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		return;
	}

	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_WsConnContext** ppItem = (XS_WsConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_WsConnContext* objCtx = ppItem ? *ppItem : NULL;

		if ( objCtx == NULL ) {
			continue;
		}

		objCtx->pTracker = NULL;
		objCtx->pServer = NULL;
	}
	xrtMutexUnlock(objHandle->pConnLock);
}

static inline int64 XS_WsFinalizeTrackedConns(XS_WsHandle* objHandle)
{
	xarray arrFinalize;
	int64 iFinalizeCount;
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		return 0;
	}

	arrFinalize = xrtArrayCreate(sizeof(XS_WsConnContext*), XRT_OBJMODE_LOCAL);
	if ( arrFinalize == NULL ) {
		return 0;
	}

	iFinalizeCount = 0;
	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_WsConnContext** ppItem = (XS_WsConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_WsConnContext* objCtx = ppItem ? *ppItem : NULL;
		XS_WsConnContext** ppFinalize;
		uint32 iPos;

		if ( objCtx == NULL ) {
			continue;
		}

		iPos = xrtArrayAppend(arrFinalize, 1);
		ppFinalize = (XS_WsConnContext**)xrtArrayGet(arrFinalize, iPos);
		if ( ppFinalize ) {
			*ppFinalize = objCtx;
			iFinalizeCount++;
		}
	}
	xrtArrayClear(objHandle->arrConn);
	xrtMutexUnlock(objHandle->pConnLock);

	for ( i = 1; i <= arrFinalize->Count; i++ ) {
		XS_WsConnContext** ppFinalize = (XS_WsConnContext**)xrtArrayGet(arrFinalize, i);
		XS_WsConnContext* objCtx = ppFinalize ? *ppFinalize : NULL;

		if ( objCtx == NULL ) {
			continue;
		}

		if ( objCtx->pStream ) {
			objCtx->pStream = NULL;
		}
		objCtx->pConn = NULL;
		objCtx->pServer = NULL;
		objCtx->pTracker = NULL;
		XS_WsRecordClose((xnet_result)XWS_CLOSE_GOING_AWAY);
		xrtFree(objCtx);
	}
	xrtArrayDestroy(arrFinalize);
	return iFinalizeCount;
}

static inline int64 XS_WsWaitTrackedConnDrain(XS_WsHandle* objHandle, uint32 iTimeoutMS)
{
	uint32 iWaitedMS = 0;
	int64 iRemain = XS_WsTrackedConnCount(objHandle);

	while ( iRemain > 0 && iWaitedMS < iTimeoutMS ) {
		XS_WsSleepMS(20);
		iWaitedMS += 20;
		iRemain = XS_WsTrackedConnCount(objHandle);
	}

	return iRemain;
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
	XS_WsRecordRejectEvent("conn_limit");
}

static inline void XS_WsRecordMessageLimitClose(void)
{
	g_iXsWsMessageLimitCloseCount++;
	g_tXsWsLastMessageLimitCloseTime = xrtNow();
}

static inline void XS_WsSnapshotRemote(char* sValue, size_t iValueSize)
{
	if ( sValue == NULL || iValueSize == 0 ) {
		return;
	}
	if ( g_sXsWsLastRemote[0] ) {
		strncpy(sValue, g_sXsWsLastRemote, iValueSize - 1);
		sValue[iValueSize - 1] = '\0';
	} else {
		sValue[0] = '\0';
	}
}

static inline void XS_WsSnapshotInvalidRemote(void)
{
	XS_WsSnapshotRemote(g_sXsWsLastInvalidRemote, sizeof(g_sXsWsLastInvalidRemote));
}

static inline void XS_WsSnapshotErrorRemote(void)
{
	XS_WsSnapshotRemote(g_sXsWsLastErrorRemote, sizeof(g_sXsWsLastErrorRemote));
}

static inline void XS_WsRecordInvalid(const char* sReason)
{
	XS_WsMetricAdd(&g_iXsWsInvalidCount, 1);
	g_tXsWsLastInvalidTime = xrtNow();
	if ( sReason && strcmp(sReason, "message limit exceeded") == 0 ) {
		XS_WsRecordMessageLimitClose();
	}
	XS_WsRecordRejectEvent(sReason);
	XS_WsSnapshotInvalidRemote();
	if ( sReason && sReason[0] ) {
		strncpy(g_sXsWsLastInvalidReason, sReason, sizeof(g_sXsWsLastInvalidReason) - 1);
		g_sXsWsLastInvalidReason[sizeof(g_sXsWsLastInvalidReason) - 1] = '\0';
	} else {
		g_sXsWsLastInvalidReason[0] = '\0';
	}
}

static inline const char* XS_WsInvalidCloseReasonText(xnet_result iReason)
{
	switch ( (uint32)iReason ) {
		case XWS_CLOSE_TOO_BIG:
			return "message limit exceeded";
		case XWS_CLOSE_PROTOCOL:
			return "protocol violation";
		default:
			return NULL;
	}
}

static inline const char* XS_WsInvalidErrorReasonText(int iSysErr)
{
	switch ( iSysErr ) {
		case -31:
			return "invalid handshake";
		default:
			return NULL;
	}
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

static inline void XS_WsRecordRemoteContext(XS_WsConnContext* objCtx);

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

				if ( objCtx == NULL || objCtx->pStream == NULL || objCtx->bClosing ) {
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
				pStream = objCtx->pStream;
				XS_WsRecordRemoteContext(objCtx);
				XS_WsRecordClose((xnet_result)XWS_CLOSE_NORMAL);
				XS_WsRecordIdleClose();
				XS_LogInfo(
					"ws idle close: server=%s host=%s timeout=%u remote=%s",
					objServer && objServer->Name ? objServer->Name : "(null)",
					objHost && objHost->Name ? objHost->Name : "(default)",
					(unsigned)(objServer ? objServer->IdleTimeout : 0u),
					g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)"
				);
				if ( objHost && objHost->procWsClose ) {
					objHost->procWsClose(objServer, objHost, pConn, (int)XWS_CLOSE_NORMAL);
				}
				XS_WsUntrackConn(objHandle, objCtx);
				if ( pStream ) {
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

static inline void XS_WsRecordRemoteByStream(xnetstream* pStream)
{
	const xnetaddr* pAddr;
	const char* sAddr;

	if ( pStream == NULL ) {
		g_sXsWsLastRemote[0] = '\0';
		return;
	}

	pAddr = xrtNetStreamRemoteAddr(pStream);
	sAddr = pAddr ? xrtNetAddrToStr(pAddr) : NULL;
	if ( sAddr == NULL || sAddr[0] == '\0' ) {
		g_sXsWsLastRemote[0] = '\0';
		return;
	}

	snprintf(g_sXsWsLastRemote, sizeof(g_sXsWsLastRemote), "%s", sAddr);
}

static inline void XS_WsRecordRemoteOpenConn(xwsconn* pConn)
{
	XS_WsRecordRemoteByStream((pConn && pConn->pStream) ? pConn->pStream : NULL);
}

static inline void XS_WsRecordRemoteContext(XS_WsConnContext* objCtx)
{
	XS_WsRecordRemoteByStream(objCtx ? objCtx->pStream : NULL);
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
