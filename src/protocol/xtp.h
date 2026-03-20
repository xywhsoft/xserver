#ifndef XS_PROTOCOL_XTP_H
#define XS_PROTOCOL_XTP_H

typedef struct {
	unsigned char HeadInfo[4];
	uint32 PackSize;
	uint64 MsgID;
	uint16 Flags;
	uint16 MsgType;
	uint16 CmdSize;
	uint16 ParamCount;
	uint32 BodySize;
	int32 Status;
} XTP_PackHeader;

typedef struct {
	uint16 KeySize;
	uint16 ValSize;
} XTP_ParamInfo;

typedef enum {
	XTP_MSG_REQUEST = 1,
	XTP_MSG_RESPONSE = 2,
	XTP_MSG_PUSH = 3,
	XTP_MSG_EVENT = 4
} XTP_MessageType;

typedef struct XTP_Message {
	uint16 Flags;
	uint16 MsgType;
	uint64 MsgID;
	int32 Status;
	uint32 PackSize;
	char* pPackBuf;
	const char* pCmd;
	uint16 CmdSize;
	const XTP_ParamInfo* pParamInfo;
	const char* pParamData;
	uint16 ParamCount;
	const char* pBody;
	uint32 BodySize;
} XTP_Message, *XTP_MessageObject;

typedef struct {
	XS_ServerConfig* pServer;
	xnetstream* pStream;
	char* pRecvBuf;
	size_t iRecvLen;
	size_t iRecvCap;
	int64 iLastActiveMS;
	volatile bool bClosing;
	ptr pTracker;
} XS_XtpConnContext;

typedef struct {
	xsocket hSocket;
	XS_XtpConnContext* pRecvCtx;
	uint32 iTimeoutMs;
} XS_XtpSyncClient;

typedef struct {
	xnetlistener* pListener;
	xnetlistener* pListenerTLS;
	XS_ServerConfig* pServer;
	xthread hAcceptThread;
	xthread hAcceptThreadTLS;
	xthread hIdleThread;
	volatile bool bStopAccept;
	xmutex pConnLock;
	xarray arrConn;
} XS_XtpHandle;

static _Thread_local int g_iXsXtpClientLastErrorCode = 0;
static _Thread_local char g_sXsXtpClientLastError[128] = "";

static inline bool XS_XtpAppendChain(XS_XtpConnContext* objCtx, xnetchain* pChain);
static inline bool XS_XtpPacketHeaderInvalid(const XS_XtpConnContext* objCtx);
static inline void XS_XtpConsume(XS_XtpConnContext* objCtx, size_t iBytes);
static inline bool XS_XtpParseMessage(XS_XtpConnContext* objCtx, XTP_Message* pMsg, size_t* pPackBytes);
static inline bool XS_XtpBuildPacket(uint16 iMsgType, uint64 iMsgID, uint16 iFlags, int32 iStatus, const char* sCmd, size_t iCmdSize, uint32 iParamCount, const char** arrParam, const char** arrValue, const void* pBody, size_t iBodySize, char** ppSendBuf, size_t* piPackSize);
static inline bool XS_XtpIsOK(const void* pMsg);
static inline char* XS_XtpBodyDup(const void* pMsg, const char* sDefault);
static inline xvalue XS_XtpBodyValue(const void* pMsg);
static inline xvalue XS_XtpErrorValue(const void* pMsg);
static inline xvalue XS_XtpParamsValue(const void* pMsg);
static inline xvalue XS_XtpValue(const void* pMsg);
static inline char* XS_XtpMetaText(const void* pMsg);
static inline char* XS_XtpMetaJson(const void* pMsg);
static inline char* XS_XtpResultJson(const void* pMsg);
static inline char* XS_XtpErrorJson(const void* pMsg, const char* sDefault);
static inline char* XS_XtpSummaryJson(const void* pMsg);

static inline int64 XS_XtpNowMS(void)
{
	#if defined(_WIN32) || defined(_WIN64)
		return (int64)GetTickCount64();
	#else
		struct timespec tNow;

		clock_gettime(CLOCK_MONOTONIC, &tNow);
		return ((int64)tNow.tv_sec * 1000) + ((int64)tNow.tv_nsec / 1000000);
	#endif
}

static inline void XS_XtpSleepMS(uint32 iMS)
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

static inline void XS_XtpClientSetLastError(int iCode, const char* sText)
{
	g_iXsXtpClientLastErrorCode = iCode;
	if ( sText && sText[0] ) {
		strncpy(g_sXsXtpClientLastError, sText, sizeof(g_sXsXtpClientLastError) - 1);
		g_sXsXtpClientLastError[sizeof(g_sXsXtpClientLastError) - 1] = '\0';
	} else {
		g_sXsXtpClientLastError[0] = '\0';
	}
}

static inline int XS_XtpClientLastErrorCode(void)
{
	return g_iXsXtpClientLastErrorCode;
}

static inline const char* XS_XtpClientLastError(void)
{
	return g_sXsXtpClientLastError;
}

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

static inline void XS_XtpRecordIdleClose(void)
{
	g_iXsXtpIdleCloseCount++;
	g_tXsXtpLastIdleCloseTime = xrtNow();
}

static inline void XS_XtpRecordConnLimitClose(void)
{
	g_iXsXtpConnLimitCloseCount++;
	g_tXsXtpLastConnLimitCloseTime = xrtNow();
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

static inline void XS_XtpRecordInvalid(const char* sReason)
{
	XS_XtpMetricAdd(&g_iXsXtpInvalidCount, 1);
	g_tXsXtpLastInvalidTime = xrtNow();
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
	if ( sAddr && sAddr[0] ) {
		strncpy(g_sXsXtpLastRemote, sAddr, sizeof(g_sXsXtpLastRemote) - 1);
		g_sXsXtpLastRemote[sizeof(g_sXsXtpLastRemote) - 1] = '\0';
	} else {
		g_sXsXtpLastRemote[0] = '\0';
	}
}



static inline void XS_XtpFreeMessage(XTP_Message* pMsg)
{
	if ( pMsg == NULL ) {
		return;
	}
	
	if ( pMsg->pPackBuf ) {
		xrtFree(pMsg->pPackBuf);
	}
	
	memset(pMsg, 0, sizeof(XTP_Message));
}

static inline XS_XtpConnContext* XS_XtpGetContext(xnetstream* pStream)
{
	return pStream ? (XS_XtpConnContext*)xrtNetStreamGetUserData(pStream) : NULL;
}

static inline void XS_XtpDestroyContext(XS_XtpConnContext* objCtx)
{
	if ( objCtx == NULL ) {
		return;
	}
	
	if ( objCtx->pRecvBuf ) {
		xrtFree(objCtx->pRecvBuf);
	}

	xrtFree(objCtx);
}

static inline void XS_XtpSyncClientCloseSocket(XS_XtpSyncClient* objClient)
{
	if ( objClient == NULL ) {
		return;
	}

	if ( objClient->hSocket != XNET_SOCKET_INVALID ) {
		#if defined(_WIN32) || defined(_WIN64)
			closesocket(objClient->hSocket);
		#else
			close(objClient->hSocket);
		#endif
		objClient->hSocket = XNET_SOCKET_INVALID;
	}
}

static inline bool XS_XtpSocketSetNonBlock(xsocket hSocket, bool bEnable)
{
	#if defined(_WIN32) || defined(_WIN64)
		u_long iMode = bEnable ? 1ul : 0ul;

		return ioctlsocket(hSocket, FIONBIO, &iMode) == 0;
	#else
		int iFlags = fcntl(hSocket, F_GETFL, 0);

		if ( iFlags < 0 ) {
			return FALSE;
		}
		if ( bEnable ) {
			iFlags |= O_NONBLOCK;
		} else {
			iFlags &= ~O_NONBLOCK;
		}
		return fcntl(hSocket, F_SETFL, iFlags) == 0;
	#endif
}

static inline void XS_XtpSocketSetTimeout(xsocket hSocket, uint32 iTimeoutMs)
{
	if ( hSocket == XNET_SOCKET_INVALID || iTimeoutMs == 0u ) {
		return;
	}

	#if defined(_WIN32) || defined(_WIN64)
		DWORD iValue = (DWORD)iTimeoutMs;

		(void)setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&iValue, (int)sizeof(iValue));
		(void)setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&iValue, (int)sizeof(iValue));
	#else
		struct timeval tValue;

		tValue.tv_sec = (int)(iTimeoutMs / 1000u);
		tValue.tv_usec = (int)((iTimeoutMs % 1000u) * 1000u);
		(void)setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, &tValue, (socklen_t)sizeof(tValue));
		(void)setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, &tValue, (socklen_t)sizeof(tValue));
	#endif
}

static inline bool XS_XtpSocketWaitWritable(xsocket hSocket, uint32 iTimeoutMs)
{
	fd_set tWriteSet;
	struct timeval tWait;
	int iRet;

	FD_ZERO(&tWriteSet);
	FD_SET(hSocket, &tWriteSet);
	tWait.tv_sec = (long)(iTimeoutMs / 1000u);
	tWait.tv_usec = (long)((iTimeoutMs % 1000u) * 1000u);
	iRet = select((int)hSocket + 1, NULL, &tWriteSet, NULL, &tWait);
	return iRet > 0 && FD_ISSET(hSocket, &tWriteSet);
}

static inline bool XS_XtpSocketConnect(xsocket hSocket, const struct sockaddr* pAddr, socklen_t iAddrLen, uint32 iTimeoutMs, int* piSysErr)
{
	int iRet;

	if ( piSysErr ) {
		*piSysErr = 0;
	}
	if ( !XS_XtpSocketSetNonBlock(hSocket, TRUE) ) {
		return FALSE;
	}

	iRet = connect(hSocket, pAddr, iAddrLen);
	if ( iRet == 0 ) {
		(void)XS_XtpSocketSetNonBlock(hSocket, FALSE);
		return TRUE;
	}

	#if defined(_WIN32) || defined(_WIN64)
		if ( WSAGetLastError() != WSAEWOULDBLOCK ) {
			if ( piSysErr ) {
				*piSysErr = WSAGetLastError();
			}
			return FALSE;
		}
	#else
		if ( errno != EINPROGRESS ) {
			if ( piSysErr ) {
				*piSysErr = errno;
			}
			return FALSE;
		}
	#endif

	if ( !XS_XtpSocketWaitWritable(hSocket, iTimeoutMs) ) {
		if ( piSysErr ) {
			*piSysErr = -1;
		}
		return FALSE;
	}

	{
		int iErr = 0;
		socklen_t iErrLen = (socklen_t)sizeof(iErr);

		if ( getsockopt(hSocket, SOL_SOCKET, SO_ERROR, (char*)&iErr, &iErrLen) != 0 || iErr != 0 ) {
			if ( piSysErr ) {
				*piSysErr = iErr != 0 ? iErr : -2;
			}
			return FALSE;
		}
	}

	(void)XS_XtpSocketSetNonBlock(hSocket, FALSE);
	return TRUE;
}

static inline bool XS_XtpSocketSendAll(xsocket hSocket, const char* pData, size_t iSize)
{
	size_t iSent = 0;

	while ( iSent < iSize ) {
		int iRet = send(hSocket, pData + iSent, (int)(iSize - iSent), 0);

		if ( iRet <= 0 ) {
			return FALSE;
		}
		iSent += (size_t)iRet;
	}

	return TRUE;
}

static inline bool XS_XtpSocketRecvAll(xsocket hSocket, char* pBuf, size_t iNeed)
{
	size_t iRead = 0;

	while ( iRead < iNeed ) {
		int iRet = recv(hSocket, pBuf + iRead, (int)(iNeed - iRead), 0);

		if ( iRet <= 0 ) {
			return FALSE;
		}
		iRead += (size_t)iRet;
	}

	return TRUE;
}

static inline XS_XtpSyncClient* XS_XtpSyncClientCreate(uint32 iRecvLimit)
{
	XS_XtpSyncClient* objClient;
	XS_XtpConnContext* objCtx;

	objClient = (XS_XtpSyncClient*)xrtMalloc(sizeof(XS_XtpSyncClient));
	if ( objClient == NULL ) {
		XS_XtpClientSetLastError(1, "client alloc failed");
		return NULL;
	}
	memset(objClient, 0, sizeof(XS_XtpSyncClient));
	objClient->hSocket = XNET_SOCKET_INVALID;

	objCtx = (XS_XtpConnContext*)xrtMalloc(sizeof(XS_XtpConnContext));
	if ( objCtx == NULL ) {
		XS_XtpClientSetLastError(2, "recv context alloc failed");
		xrtFree(objClient);
		return NULL;
	}
	memset(objCtx, 0, sizeof(XS_XtpConnContext));
	objCtx->pServer = NULL;

	objClient->pRecvCtx = objCtx;
	if ( iRecvLimit > 0u ) {
		objCtx->iRecvCap = 0;
	}

	return objClient;
}

static inline void XS_XtpSyncClientDestroy(XS_XtpSyncClient* objClient)
{
	if ( objClient == NULL ) {
		return;
	}

	XS_XtpSyncClientCloseSocket(objClient);
	if ( objClient->pRecvCtx ) {
		XS_XtpDestroyContext(objClient->pRecvCtx);
	}

	xrtFree(objClient);
}

static inline int XS_XtpSyncClientConnect(XS_XtpSyncClient* objClient, const char* sHost, uint16 iPort, uint32 iTimeoutMs)
{
	struct addrinfo tHints;
	struct addrinfo* pRes = NULL;
	struct addrinfo* pCur;
	char sPort[16];
	char sMsg[192];
	int iSysErr = 0;

	if ( objClient == NULL || sHost == NULL || sHost[0] == '\0' ) {
		XS_XtpClientSetLastError(3, "invalid connect args");
		return FALSE;
	}

	XS_XtpSyncClientCloseSocket(objClient);
	memset(&tHints, 0, sizeof(tHints));
	tHints.ai_family = AF_UNSPEC;
	tHints.ai_socktype = SOCK_STREAM;
	tHints.ai_protocol = IPPROTO_TCP;
	snprintf(sPort, sizeof(sPort), "%u", (unsigned)iPort);
	if ( getaddrinfo(sHost, sPort, &tHints, &pRes) != 0 || pRes == NULL ) {
		XS_XtpClientSetLastError(4, "getaddrinfo failed");
		return FALSE;
	}

	for ( pCur = pRes; pCur; pCur = pCur->ai_next ) {
		xsocket hSocket = socket(pCur->ai_family, pCur->ai_socktype, pCur->ai_protocol);

		if ( hSocket == XNET_SOCKET_INVALID ) {
			continue;
		}
		if ( XS_XtpSocketConnect(hSocket, pCur->ai_addr, (socklen_t)pCur->ai_addrlen, iTimeoutMs ? iTimeoutMs : 5000u, &iSysErr) ) {
			objClient->hSocket = hSocket;
			objClient->iTimeoutMs = iTimeoutMs ? iTimeoutMs : 5000u;
			XS_XtpSocketSetTimeout(objClient->hSocket, objClient->iTimeoutMs);
			freeaddrinfo(pRes);
			return TRUE;
		}
		#if defined(_WIN32) || defined(_WIN64)
			closesocket(hSocket);
		#else
			close(hSocket);
		#endif
	}

	freeaddrinfo(pRes);
	snprintf(sMsg, sizeof(sMsg), "connect failed: %d", iSysErr);
	XS_XtpClientSetLastError(5, sMsg);
	return FALSE;
}

static inline int XS_XtpSyncClientRecv(XS_XtpSyncClient* objClient, XTP_Message* pMsg, uint32 iTimeoutMs)
{
	XTP_PackHeader tHeader;
	size_t iPackBytes;
	size_t iNeedBytes;

	if ( objClient == NULL || objClient->hSocket == XNET_SOCKET_INVALID || objClient->pRecvCtx == NULL || pMsg == NULL ) {
		XS_XtpClientSetLastError(6, "invalid recv args");
		return FALSE;
	}

	memset(pMsg, 0, sizeof(XTP_Message));
	(void)iTimeoutMs;
	if ( !XS_XtpSocketRecvAll(objClient->hSocket, (char*)&tHeader, sizeof(tHeader)) ) {
		XS_XtpClientSetLastError(7, "recv header failed");
		return FALSE;
	}
	if ( memcmp(tHeader.HeadInfo, "xtp\2", 4) != 0 || tHeader.PackSize < sizeof(XTP_PackHeader) ) {
		XS_XtpClientSetLastError(8, "invalid response packet");
		return FALSE;
	}

	iNeedBytes = (size_t)tHeader.PackSize;
	objClient->pRecvCtx->pRecvBuf = (char*)xrtRealloc(objClient->pRecvCtx->pRecvBuf, iNeedBytes);
	if ( objClient->pRecvCtx->pRecvBuf == NULL ) {
		objClient->pRecvCtx->iRecvCap = 0;
		objClient->pRecvCtx->iRecvLen = 0;
		XS_XtpClientSetLastError(9, "recv buffer alloc failed");
		return FALSE;
	}
	objClient->pRecvCtx->iRecvCap = iNeedBytes;
	objClient->pRecvCtx->iRecvLen = iNeedBytes;
	memcpy(objClient->pRecvCtx->pRecvBuf, &tHeader, sizeof(tHeader));
	if ( iNeedBytes > sizeof(XTP_PackHeader) ) {
		if ( !XS_XtpSocketRecvAll(objClient->hSocket, objClient->pRecvCtx->pRecvBuf + sizeof(XTP_PackHeader), iNeedBytes - sizeof(XTP_PackHeader)) ) {
			XS_XtpClientSetLastError(10, "recv body failed");
			return FALSE;
		}
	}

	if ( !XS_XtpParseMessage(objClient->pRecvCtx, pMsg, &iPackBytes) ) {
		XS_XtpClientSetLastError(11, "parse response failed");
		return FALSE;
	}

	XS_XtpConsume(objClient->pRecvCtx, iPackBytes);
	return TRUE;
}

static inline int XS_XtpSyncClientDo(
	XS_XtpSyncClient* objClient,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize,
	uint32 iTimeoutMs,
	XTP_Message* pResp
)
{
	char* pSendBuf = NULL;
	size_t iPackSize = 0;

	if ( objClient == NULL || pResp == NULL ) {
		XS_XtpClientSetLastError(12, "invalid do args");
		return FALSE;
	}

	if ( !XS_XtpBuildPacket(XTP_MSG_REQUEST, iMsgID, 0, 0, sCmd, 0, iParamCount, arrParam, arrValue, pBody, iBodySize, &pSendBuf, &iPackSize) ) {
		XS_XtpClientSetLastError(13, "build request failed");
		return FALSE;
	}

	if ( !XS_XtpSocketSendAll(objClient->hSocket, pSendBuf, iPackSize) ) {
		xrtFree(pSendBuf);
		XS_XtpClientSetLastError(14, "send request failed");
		return FALSE;
	}
	xrtFree(pSendBuf);
	XS_XtpMetricAdd(&g_iXsXtpSendCount, 1);
	XS_XtpMetricAdd(&g_iXsXtpSendBytes, (int64)iPackSize);

	return XS_XtpSyncClientRecv(objClient, pResp, iTimeoutMs);
}

static inline void XS_XtpMessageDestroy(XTP_Message* pMsg)
{
	if ( pMsg == NULL ) {
		return;
	}

	XS_XtpFreeMessage(pMsg);
	xrtFree(pMsg);
}

static inline void* XS_XtpClientOpen(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iTimeoutMs)
{
	XS_XtpSyncClient* objClient;

	objClient = XS_XtpSyncClientCreate(iRecvLimit);
	if ( objClient == NULL ) {
		return NULL;
	}

	if ( !XS_XtpSyncClientConnect(objClient, sHost, iPort, iTimeoutMs) ) {
		XS_XtpSyncClientDestroy(objClient);
		return NULL;
	}

	return objClient;
}

static inline void XS_XtpClientClose(void* pClient)
{
	XS_XtpSyncClient* objClient = (XS_XtpSyncClient*)pClient;

	XS_XtpSyncClientDestroy(objClient);
}

static inline void* XS_XtpClientDo(
	void* pClient,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize,
	uint32 iTimeoutMs
)
{
	XS_XtpSyncClient* objClient = (XS_XtpSyncClient*)pClient;
	XTP_Message* pResp;

	if ( objClient == NULL ) {
		XS_XtpClientSetLastError(17, "client is null");
		return NULL;
	}

	pResp = (XTP_Message*)xrtMalloc(sizeof(XTP_Message));
	if ( pResp == NULL ) {
		XS_XtpClientSetLastError(18, "response alloc failed");
		return NULL;
	}
	memset(pResp, 0, sizeof(XTP_Message));

	if ( !XS_XtpSyncClientDo(objClient, iMsgID, sCmd, iParamCount, arrParam, arrValue, pBody, iBodySize, iTimeoutMs, pResp) ) {
		XS_XtpMessageDestroy(pResp);
		return NULL;
	}

	return pResp;
}

static inline void* XS_XtpClientDoText(
	void* pClient,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sText,
	uint32 iTimeoutMs
)
{
	return XS_XtpClientDo(
		pClient,
		iMsgID,
		sCmd,
		iParamCount,
		arrParam,
		arrValue,
		sText,
		sText ? strlen(sText) : 0u,
		iTimeoutMs
	);
}

static inline void* XS_XtpClientDoSimple(
	void* pClient,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs
)
{
	return XS_XtpClientDo(
		pClient,
		iMsgID,
		sCmd,
		0u,
		NULL,
		NULL,
		NULL,
		0u,
		iTimeoutMs
	);
}

static inline void* XS_XtpClientDoJson(
	void* pClient,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sJson,
	uint32 iTimeoutMs
)
{
	return XS_XtpClientDo(
		pClient,
		iMsgID,
		sCmd,
		iParamCount,
		arrParam,
		arrValue,
		sJson,
		sJson ? strlen(sJson) : 0u,
		iTimeoutMs
	);
}

static inline void* XS_XtpClientCall(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize,
	uint32 iTimeoutMs
)
{
	void* pClient;
	void* pResp;

	pClient = XS_XtpClientOpen(sHost, iPort, iRecvLimit, iConnectTimeoutMs);
	if ( pClient == NULL ) {
		return NULL;
	}

	pResp = XS_XtpClientDo(pClient, iMsgID, sCmd, iParamCount, arrParam, arrValue, pBody, iBodySize, iTimeoutMs);
	XS_XtpClientClose(pClient);
	return pResp;
}

static inline void* XS_XtpClientCallText(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sText,
	uint32 iTimeoutMs
)
{
	return XS_XtpClientCall(
		sHost,
		iPort,
		iRecvLimit,
		iConnectTimeoutMs,
		iMsgID,
		sCmd,
		iParamCount,
		arrParam,
		arrValue,
		sText,
		sText ? strlen(sText) : 0u,
		iTimeoutMs
	);
}

static inline void* XS_XtpClientCallSimple(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs
)
{
	return XS_XtpClientCall(
		sHost,
		iPort,
		iRecvLimit,
		iConnectTimeoutMs,
		iMsgID,
		sCmd,
		0u,
		NULL,
		NULL,
		NULL,
		0u,
		iTimeoutMs
	);
}

static inline void* XS_XtpClientCallJson(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sJson,
	uint32 iTimeoutMs
)
{
	return XS_XtpClientCall(
		sHost,
		iPort,
		iRecvLimit,
		iConnectTimeoutMs,
		iMsgID,
		sCmd,
		iParamCount,
		arrParam,
		arrValue,
		sJson,
		sJson ? strlen(sJson) : 0u,
		iTimeoutMs
	);
}

static inline void XS_XtpClientSetResponseError(const void* pMsg, const char* sDefault)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	char* sBody;

	if ( objMsg == NULL ) {
		XS_XtpClientSetLastError(19, sDefault ? sDefault : "response is null");
		return;
	}

	sBody = XS_XtpBodyDup(objMsg, sDefault ? sDefault : "response status not ok");
	XS_XtpClientSetLastError(objMsg->Status != 0 ? (int)objMsg->Status : 20, sBody ? sBody : (sDefault ? sDefault : "response status not ok"));
	if ( sBody ) {
		xrtFree(sBody);
	}
}

static inline char* XS_XtpClientCallSimpleBody(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	char* sBody;

	if ( objResp == NULL ) {
		return NULL;
	}
	if ( !XS_XtpIsOK(objResp) ) {
		XS_XtpClientSetResponseError(objResp, "response status not ok");
		XS_XtpMessageDestroy(objResp);
		return NULL;
	}

	sBody = XS_XtpBodyDup(objResp, sDefault);
	XS_XtpMessageDestroy(objResp);
	return sBody;
}

static inline char* XS_XtpClientCallTextBody(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sText,
	uint32 iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallText(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iParamCount, arrParam, arrValue, sText, iTimeoutMs);
	char* sBody;

	if ( objResp == NULL ) {
		return NULL;
	}
	if ( !XS_XtpIsOK(objResp) ) {
		XS_XtpClientSetResponseError(objResp, "response status not ok");
		XS_XtpMessageDestroy(objResp);
		return NULL;
	}

	sBody = XS_XtpBodyDup(objResp, sDefault);
	XS_XtpMessageDestroy(objResp);
	return sBody;
}

static inline char* XS_XtpClientCallJsonBody(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sJson,
	uint32 iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallJson(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iParamCount, arrParam, arrValue, sJson, iTimeoutMs);
	char* sBody;

	if ( objResp == NULL ) {
		return NULL;
	}
	if ( !XS_XtpIsOK(objResp) ) {
		XS_XtpClientSetResponseError(objResp, "response status not ok");
		XS_XtpMessageDestroy(objResp);
		return NULL;
	}

	sBody = XS_XtpBodyDup(objResp, sDefault);
	XS_XtpMessageDestroy(objResp);
	return sBody;
}

static inline char* XS_XtpClientCallSimpleSummary(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	char* sText;

	if ( objResp == NULL ) {
		return NULL;
	}
	sText = XS_XtpSummaryText(objResp);
	XS_XtpMessageDestroy(objResp);
	return sText;
}

static inline char* XS_XtpClientCallSimpleSummaryJson(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	char* sText;

	if ( objResp == NULL ) {
		return NULL;
	}
	sText = XS_XtpSummaryJson(objResp);
	XS_XtpMessageDestroy(objResp);
	return sText;
}

static inline xvalue XS_XtpClientCallSimpleValue(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	xvalue objRet;

	if ( objResp == NULL ) {
		return NULL;
	}

	objRet = XS_XtpValue(objResp);
	XS_XtpMessageDestroy(objResp);
	return objRet;
}

static inline xvalue XS_XtpClientCallSimpleParamsValue(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	xvalue objRet;

	if ( objResp == NULL ) {
		return NULL;
	}

	objRet = XS_XtpParamsValue(objResp);
	XS_XtpMessageDestroy(objResp);
	return objRet;
}

static inline xvalue XS_XtpClientCallSimpleBodyValue(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	xvalue objRet;

	if ( objResp == NULL ) {
		return NULL;
	}

	objRet = XS_XtpBodyValue(objResp);
	XS_XtpMessageDestroy(objResp);
	return objRet;
}

static inline char* XS_XtpClientCallSimpleResult(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	char* sText;

	if ( objResp == NULL ) {
		return NULL;
	}
	sText = XS_XtpParamDup(objResp, "result", sDefault);
	XS_XtpMessageDestroy(objResp);
	return sText;
}

static inline char* XS_XtpClientCallSimpleError(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	char* sText;

	if ( objResp == NULL ) {
		return NULL;
	}
	sText = XS_XtpErrorText(objResp, sDefault);
	XS_XtpMessageDestroy(objResp);
	return sText;
}

static inline char* XS_XtpClientCallSimpleMeta(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	char* sText;

	if ( objResp == NULL ) {
		return NULL;
	}
	sText = XS_XtpMetaText(objResp);
	XS_XtpMessageDestroy(objResp);
	return sText;
}

static inline char* XS_XtpClientCallSimpleMetaJson(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	char* sText;

	if ( objResp == NULL ) {
		return NULL;
	}
	sText = XS_XtpMetaJson(objResp);
	XS_XtpMessageDestroy(objResp);
	return sText;
}

static inline char* XS_XtpClientCallSimpleResultJson(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	char* sText;

	if ( objResp == NULL ) {
		return NULL;
	}
	sText = XS_XtpResultJson(objResp);
	XS_XtpMessageDestroy(objResp);
	return sText;
}

static inline char* XS_XtpClientCallSimpleErrorJson(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	char* sText;

	if ( objResp == NULL ) {
		return NULL;
	}
	sText = XS_XtpErrorJson(objResp, sDefault);
	XS_XtpMessageDestroy(objResp);
	return sText;
}

static inline int32 XS_XtpClientCallSimpleStatus(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs,
	int32 iDefault
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	int32 iStatus;

	if ( objResp == NULL ) {
		return iDefault;
	}
	iStatus = objResp->Status;
	XS_XtpMessageDestroy(objResp);
	return iStatus;
}

static inline char* XS_XtpClientCallSimpleCmd(
	const char* sHost,
	uint16 iPort,
	uint32 iRecvLimit,
	uint32 iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	uint32 iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp = (XTP_MessageObject)XS_XtpClientCallSimple(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, iTimeoutMs);
	char* sText;

	if ( objResp == NULL ) {
		return NULL;
	}
	sText = XS_XtpCmdDup(objResp, sDefault);
	XS_XtpMessageDestroy(objResp);
	return sText;
}

static inline bool XS_XtpEnsureBuffer(XS_XtpConnContext* objCtx, size_t iNeed)
{
	size_t iCap;
	char* pNewBuf;
	
	if ( objCtx == NULL ) {
		return FALSE;
	}
	if ( iNeed <= objCtx->iRecvCap ) {
		return TRUE;
	}
	
	iCap = objCtx->iRecvCap == 0 ? 4096 : objCtx->iRecvCap;
	while ( iCap < iNeed ) {
		if ( iCap > (SIZE_MAX / 2) ) {
			return FALSE;
		}
		iCap *= 2;
	}
	
	pNewBuf = (char*)xrtRealloc(objCtx->pRecvBuf, iCap);
	if ( pNewBuf == NULL ) {
		return FALSE;
	}
	
	objCtx->pRecvBuf = pNewBuf;
	objCtx->iRecvCap = iCap;
	return TRUE;
}

static inline bool XS_XtpAppendChain(XS_XtpConnContext* objCtx, xnetchain* pChain)
{
	size_t iBytes;
	size_t iNeed;
	
	if ( objCtx == NULL || pChain == NULL ) {
		return FALSE;
	}
	
	iBytes = xrtNetChainBytes(pChain);
	if ( iBytes == 0 ) {
		return TRUE;
	}
	iNeed = objCtx->iRecvLen + iBytes;
	if ( objCtx->pServer && objCtx->pServer->RecvLimit > 0u && iNeed > (size_t)objCtx->pServer->RecvLimit ) {
		XS_LogWarn(
			"xtp recv limit exceeded: server=%s recv_limit=%u need=%u",
			objCtx->pServer->Name ? objCtx->pServer->Name : "(null)",
			(unsigned)objCtx->pServer->RecvLimit,
			(unsigned)iNeed
		);
		return FALSE;
	}
	if ( !XS_XtpEnsureBuffer(objCtx, iNeed) ) {
		return FALSE;
	}
	
	(void)xrtNetChainPeek(pChain, objCtx->pRecvBuf + objCtx->iRecvLen, iBytes);
	xrtNetChainConsume(pChain, iBytes);
	objCtx->iRecvLen += iBytes;
	XS_XtpMetricAdd(&g_iXsXtpRecvBytes, (int64)iBytes);
	return TRUE;
}

static inline bool XS_XtpPacketHeaderInvalid(const XS_XtpConnContext* objCtx)
{
	XTP_PackHeader tHeader;
	size_t iNeedBytes;
	uint16 i;
	const XTP_ParamInfo* pInfo;
	uint32 iRecvLimit;
	
	if ( objCtx == NULL || objCtx->iRecvLen < sizeof(XTP_PackHeader) ) {
		return FALSE;
	}
	
	memcpy(&tHeader, objCtx->pRecvBuf, sizeof(XTP_PackHeader));
	if ( memcmp(tHeader.HeadInfo, "xtp\2", 4) != 0 ) {
		return TRUE;
	}
	if ( tHeader.PackSize < sizeof(XTP_PackHeader) ) {
		return TRUE;
	}
	iRecvLimit = (objCtx->pServer && objCtx->pServer->RecvLimit > 0u) ? objCtx->pServer->RecvLimit : 0u;
	if ( iRecvLimit > 0u && tHeader.PackSize > iRecvLimit ) {
		return TRUE;
	}
	if ( objCtx->iRecvLen < sizeof(XTP_PackHeader) + ((size_t)tHeader.ParamCount * sizeof(XTP_ParamInfo)) ) {
		return FALSE;
	}
	
	iNeedBytes = sizeof(XTP_PackHeader) + ((size_t)tHeader.ParamCount * sizeof(XTP_ParamInfo)) + (size_t)tHeader.CmdSize + (size_t)tHeader.BodySize;
	for ( i = 0; i < tHeader.ParamCount; i++ ) {
		pInfo = (const XTP_ParamInfo*)(objCtx->pRecvBuf + sizeof(XTP_PackHeader) + ((size_t)i * sizeof(XTP_ParamInfo)));
		iNeedBytes += (size_t)pInfo->KeySize + (size_t)pInfo->ValSize;
		if ( iRecvLimit > 0u && iNeedBytes > iRecvLimit ) {
			return TRUE;
		}
	}
	if ( objCtx->iRecvLen >= (size_t)tHeader.PackSize && iNeedBytes != (size_t)tHeader.PackSize ) {
		return TRUE;
	}
	
	return FALSE;
}

static inline void XS_XtpConsume(XS_XtpConnContext* objCtx, size_t iBytes)
{
	if ( objCtx == NULL || iBytes == 0 ) {
		return;
	}
	if ( iBytes >= objCtx->iRecvLen ) {
		objCtx->iRecvLen = 0;
		return;
	}
	
	memmove(objCtx->pRecvBuf, objCtx->pRecvBuf + iBytes, objCtx->iRecvLen - iBytes);
	objCtx->iRecvLen -= iBytes;
}

static inline bool XS_XtpParamAt(const void* pMsg, uint16 iIndex, const char** ppKey, uint16* piKeyLen, const char** ppVal, uint16* piValLen)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	const char* pCursor;
	uint16 i;
	
	if ( ppKey ) *ppKey = NULL;
	if ( piKeyLen ) *piKeyLen = 0;
	if ( ppVal ) *ppVal = NULL;
	if ( piValLen ) *piValLen = 0;
	
	if ( objMsg == NULL || iIndex >= objMsg->ParamCount || objMsg->pParamInfo == NULL || objMsg->pParamData == NULL ) {
		return FALSE;
	}
	
	pCursor = objMsg->pParamData;
	for ( i = 0; i < objMsg->ParamCount; i++ ) {
		const XTP_ParamInfo* pInfo = &objMsg->pParamInfo[i];
		
		if ( i == iIndex ) {
			if ( ppKey ) *ppKey = pCursor;
			if ( piKeyLen ) *piKeyLen = pInfo->KeySize;
			if ( ppVal ) *ppVal = pCursor + pInfo->KeySize;
			if ( piValLen ) *piValLen = pInfo->ValSize;
			return TRUE;
		}
		
		pCursor += (size_t)pInfo->KeySize + (size_t)pInfo->ValSize;
	}
	
	return FALSE;
}

static inline bool XS_XtpFindParamView(const void* pMsg, const char* sKey, const char** ppVal, uint16* piValLen)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	uint16 i;
	size_t iKeyLen;
	
	if ( ppVal ) *ppVal = NULL;
	if ( piValLen ) *piValLen = 0;
	
	if ( objMsg == NULL || sKey == NULL ) {
		return FALSE;
	}
	
	iKeyLen = strlen(sKey);
	for ( i = 0; i < objMsg->ParamCount; i++ ) {
		const char* pKey;
		const char* pVal;
		uint16 iKeySize;
		uint16 iValSize;
		
		if ( !XS_XtpParamAt(objMsg, i, &pKey, &iKeySize, &pVal, &iValSize) ) {
			continue;
		}
		if ( iKeySize == (uint16)iKeyLen && memcmp(pKey, sKey, iKeyLen) == 0 ) {
			if ( ppVal ) *ppVal = pVal;
			if ( piValLen ) *piValLen = iValSize;
			return TRUE;
		}
	}
	
	return FALSE;
}

static inline bool XS_XtpParseMessage(XS_XtpConnContext* objCtx, XTP_Message* pMsg, size_t* pPackBytes)
{
	XTP_PackHeader tHeader;
	size_t iNeedBytes;
	size_t iPos;
	uint16 i;
	char* pPackBuf;
	const XTP_ParamInfo* pParamInfo;
	
	if ( objCtx == NULL || pMsg == NULL || pPackBytes == NULL ) {
		return FALSE;
	}
	if ( objCtx->iRecvLen < sizeof(XTP_PackHeader) ) {
		return FALSE;
	}
	
	memcpy(&tHeader, objCtx->pRecvBuf, sizeof(XTP_PackHeader));
	if ( memcmp(tHeader.HeadInfo, "xtp\2", 4) != 0 ) {
		XS_XtpRecordInvalid("invalid header");
		XS_LogWarn(
			"xtp invalid header: server=%s stream=%p",
			objCtx->pServer && objCtx->pServer->Name ? objCtx->pServer->Name : "(null)",
			(void*)objCtx->pStream
		);
		return FALSE;
	}
	if ( tHeader.PackSize < sizeof(XTP_PackHeader) ) {
		XS_XtpRecordInvalid("invalid size");
		XS_LogWarn("xtp invalid size: pack=%u", (unsigned)tHeader.PackSize);
		return FALSE;
	}
	if ( objCtx->pServer && objCtx->pServer->RecvLimit > 0u && tHeader.PackSize > objCtx->pServer->RecvLimit ) {
		XS_XtpRecordInvalid("pack limit exceeded");
		XS_LogWarn(
			"xtp pack limit exceeded: pack=%u recv_limit=%u",
			(unsigned)tHeader.PackSize,
			(unsigned)objCtx->pServer->RecvLimit
		);
		return FALSE;
	}
	
	iNeedBytes = sizeof(XTP_PackHeader) + ((size_t)tHeader.ParamCount * sizeof(XTP_ParamInfo)) + (size_t)tHeader.CmdSize + (size_t)tHeader.BodySize;
	for ( i = 0; i < tHeader.ParamCount; i++ ) {
		const XTP_ParamInfo* pInfo = (const XTP_ParamInfo*)(objCtx->pRecvBuf + sizeof(XTP_PackHeader) + ((size_t)i * sizeof(XTP_ParamInfo)));
		iNeedBytes += (size_t)pInfo->KeySize + (size_t)pInfo->ValSize;
	}
	if ( iNeedBytes != (size_t)tHeader.PackSize ) {
		XS_XtpRecordInvalid("size mismatch");
		XS_LogWarn("xtp size mismatch: pack=%u need=%u", (unsigned)tHeader.PackSize, (unsigned)iNeedBytes);
		return FALSE;
	}
	if ( objCtx->iRecvLen < (size_t)tHeader.PackSize ) {
		return FALSE;
	}
	
	pPackBuf = (char*)xrtMalloc((size_t)tHeader.PackSize);
	if ( pPackBuf == NULL ) {
		return FALSE;
	}
	memcpy(pPackBuf, objCtx->pRecvBuf, (size_t)tHeader.PackSize);
	pParamInfo = (const XTP_ParamInfo*)(pPackBuf + sizeof(XTP_PackHeader));
	
	memset(pMsg, 0, sizeof(XTP_Message));
	pMsg->Flags = tHeader.Flags;
	pMsg->MsgType = tHeader.MsgType;
	pMsg->MsgID = tHeader.MsgID;
	pMsg->Status = tHeader.Status;
	pMsg->PackSize = tHeader.PackSize;
	pMsg->pPackBuf = pPackBuf;
	pMsg->pParamInfo = pParamInfo;
	pMsg->ParamCount = tHeader.ParamCount;
	pMsg->pCmd = pPackBuf + sizeof(XTP_PackHeader) + ((size_t)tHeader.ParamCount * sizeof(XTP_ParamInfo));
	pMsg->CmdSize = tHeader.CmdSize;
	pMsg->pParamData = pMsg->pCmd + tHeader.CmdSize;
	
	iPos = sizeof(XTP_PackHeader) + ((size_t)tHeader.ParamCount * sizeof(XTP_ParamInfo)) + (size_t)tHeader.CmdSize;
	for ( i = 0; i < tHeader.ParamCount; i++ ) {
		iPos += (size_t)pParamInfo[i].KeySize + (size_t)pParamInfo[i].ValSize;
		if ( iPos > (size_t)tHeader.PackSize ) {
			XS_XtpFreeMessage(pMsg);
			XS_XtpRecordInvalid("param overflow");
			XS_LogWarn("xtp param overflow: index=%u", (unsigned)i);
			return FALSE;
		}
	}
	
	if ( iPos + (size_t)tHeader.BodySize != (size_t)tHeader.PackSize ) {
		XS_XtpFreeMessage(pMsg);
		XS_XtpRecordInvalid("body overflow");
		XS_LogWarn("xtp body overflow");
		return FALSE;
	}
	
	pMsg->pBody = pPackBuf + iPos;
	pMsg->BodySize = tHeader.BodySize;
	g_iXsXtpLastBytes = (int64)tHeader.PackSize;
	XS_XtpRecordRemote(objCtx->pStream);
	*pPackBytes = (size_t)tHeader.PackSize;
	return TRUE;
}

static inline int32 XS_XtpStatus(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	return objMsg ? objMsg->Status : 0;
}

static inline uint64 XS_XtpMsgId(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	return objMsg ? objMsg->MsgID : 0;
}

static inline uint16 XS_XtpMsgType(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	return objMsg ? objMsg->MsgType : 0;
}

static inline uint16 XS_XtpMsgFlags(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	return objMsg ? objMsg->Flags : 0;
}

static inline bool XS_XtpIsOK(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;

	return objMsg && objMsg->MsgType == XTP_MSG_RESPONSE && objMsg->Status == 0;
}

static inline char* XS_XtpBodyDup(const void* pMsg, const char* sDefault)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	const char* pBody;
	uint32 iBodyLen;
	char* sText;

	if ( objMsg && objMsg->pBody && objMsg->BodySize > 0u ) {
		pBody = objMsg->pBody;
		iBodyLen = objMsg->BodySize;
	} else {
		if ( sDefault == NULL ) {
			return NULL;
		}
		pBody = sDefault;
		iBodyLen = (uint32)strlen(sDefault);
	}

	sText = (char*)xrtMalloc((size_t)iBodyLen + 1u);
	if ( sText == NULL ) {
		return NULL;
	}
	if ( iBodyLen > 0u ) {
		memcpy(sText, pBody, iBodyLen);
	}
	sText[iBodyLen] = '\0';
	return sText;
}

static inline char* XS_XtpCmdDup(const void* pMsg, const char* sDefault)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	const char* pCmd;
	uint16 iCmdLen;
	char* sText;

	if ( objMsg && objMsg->pCmd && objMsg->CmdSize > 0u ) {
		pCmd = objMsg->pCmd;
		iCmdLen = objMsg->CmdSize;
	} else {
		if ( sDefault == NULL ) {
			return NULL;
		}
		pCmd = sDefault;
		iCmdLen = (uint16)strlen(sDefault);
	}

	sText = (char*)xrtMalloc((size_t)iCmdLen + 1u);
	if ( sText == NULL ) {
		return NULL;
	}
	if ( iCmdLen > 0u ) {
		memcpy(sText, pCmd, iCmdLen);
	}
	sText[iCmdLen] = '\0';
	return sText;
}

static inline xvalue XS_XtpValue(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	const char* sResult;
	char* sError;
	xvalue objParams;
	xvalue objRet;

	if ( objMsg == NULL ) {
		return NULL;
	}

	objRet = xvoCreateTable();
	if ( objRet == NULL ) {
		return NULL;
	}

	xvoTableSetBool(objRet, "ok", 0, XS_XtpIsOK(objMsg));
	xvoTableSetInt(objRet, "status", 0, (int64)objMsg->Status);
	xvoTableSetInt(objRet, "msg_type", 0, (int64)objMsg->MsgType);
	xvoTableSetInt(objRet, "msg_id", 0, (int64)objMsg->MsgID);
	xvoTableSetInt(objRet, "flags", 0, (int64)objMsg->Flags);
	xvoTableSetInt(objRet, "param_count", 0, (int64)objMsg->ParamCount);
	xvoTableSetInt(objRet, "body_len", 0, (int64)objMsg->BodySize);
	xvoTableSetText(objRet, "cmd", 0, (ptr)(objMsg->pCmd ? objMsg->pCmd : ""), objMsg->CmdSize, FALSE);
	objParams = XS_XtpParamsValue(objMsg);
	if ( objParams ) {
		xvoTableSetValue(objRet, "params", 0, objParams, TRUE);
	}

	sResult = XS_XtpResultText(objMsg, "");
	xvoTableSetText(objRet, "result", 0, (ptr)(sResult ? sResult : ""), 0, FALSE);
	xvoTableSetText(objRet, "body", 0, (ptr)(objMsg->pBody ? objMsg->pBody : ""), objMsg->BodySize, FALSE);

	if ( !XS_XtpIsOK(objMsg) ) {
		sError = XS_XtpErrorText(objMsg, "");
		xvoTableSetText(objRet, "error", 0, (ptr)(sError ? sError : ""), 0, FALSE);
		if ( sError ) {
			xrtFree(sError);
		}
	} else {
		xvoTableSetText(objRet, "error", 0, (ptr)"", 0, FALSE);
	}

	return objRet;
}

static inline xvalue XS_XtpBodyValue(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;

	if ( objMsg == NULL || objMsg->pBody == NULL || objMsg->BodySize == 0u ) {
		return NULL;
	}

	return xrtParseJSON((ptr)objMsg->pBody, objMsg->BodySize);
}

static inline xvalue XS_XtpErrorValue(const void* pMsg)
{
	if ( pMsg == NULL || XS_XtpIsOK(pMsg) ) {
		return NULL;
	}

	return XS_XtpBodyValue(pMsg);
}

static inline xvalue XS_XtpParamsValue(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	const char* pKey;
	const char* pVal;
	uint16 iKeyLen;
	uint16 iValLen;
	uint16 i;
	xvalue objRet;

	if ( objMsg == NULL ) {
		return NULL;
	}

	objRet = xvoCreateTable();
	if ( objRet == NULL ) {
		return NULL;
	}

	for ( i = 0; i < objMsg->ParamCount; i++ ) {
		if ( !XS_XtpParamAt(objMsg, i, &pKey, &iKeyLen, &pVal, &iValLen) ) {
			continue;
		}
		xvoTableSetText(objRet, (ptr)(pKey ? pKey : ""), iKeyLen, (ptr)(pVal ? pVal : ""), iValLen, FALSE);
	}

	return objRet;
}

static inline char* XS_XtpSummaryText(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	const char* sResult;
	char* sCmd;
	char* sText;
	int iNeed;

	if ( objMsg == NULL ) {
		sText = (char*)xrtMalloc(32);
		if ( sText ) {
			strcpy(sText, "response=(null)\n");
		}
		return sText;
	}

	sResult = XS_XtpResultText(objMsg, "");
	sCmd = XS_XtpCmdDup(objMsg, "");
	iNeed = snprintf(
		NULL,
		0,
		"ok=%s\nstatus=%d\nmsg_type=%u\nmsg_id=%llu\ncmd=%s\nresult=%s\nbody_len=%u\n",
		XS_XtpIsOK(objMsg) ? "true" : "false",
		(int)objMsg->Status,
		(unsigned)objMsg->MsgType,
		(unsigned long long)objMsg->MsgID,
		sCmd ? sCmd : "",
		sResult ? sResult : "",
		(unsigned)objMsg->BodySize
	);
	if ( iNeed < 0 ) {
		if ( sCmd ) {
			xrtFree(sCmd);
		}
		return NULL;
	}

	sText = (char*)xrtMalloc((size_t)iNeed + 1u);
	if ( sText ) {
		snprintf(
			sText,
			(size_t)iNeed + 1u,
			"ok=%s\nstatus=%d\nmsg_type=%u\nmsg_id=%llu\ncmd=%s\nresult=%s\nbody_len=%u\n",
			XS_XtpIsOK(objMsg) ? "true" : "false",
			(int)objMsg->Status,
			(unsigned)objMsg->MsgType,
			(unsigned long long)objMsg->MsgID,
			sCmd ? sCmd : "",
			sResult ? sResult : "",
			(unsigned)objMsg->BodySize
		);
	}
	if ( sCmd ) {
		xrtFree(sCmd);
	}
	return sText;
}

static inline char* XS_XtpMetaText(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	const char* sResult;
	char* sCmd;
	char* sText;
	int iNeed;

	if ( objMsg == NULL ) {
		sText = (char*)xrtMalloc(17);
		if ( sText ) {
			strcpy(sText, "response=(null)");
		}
		return sText;
	}

	sResult = XS_XtpResultText(objMsg, "");
	sCmd = XS_XtpCmdDup(objMsg, "");
	iNeed = snprintf(
		NULL,
		0,
		"ok=%s status=%d cmd=%s result=%s body_len=%u",
		XS_XtpIsOK(objMsg) ? "true" : "false",
		(int)objMsg->Status,
		sCmd ? sCmd : "",
		sResult ? sResult : "",
		(unsigned)objMsg->BodySize
	);
	if ( iNeed < 0 ) {
		if ( sCmd ) {
			xrtFree(sCmd);
		}
		return NULL;
	}

	sText = (char*)xrtMalloc((size_t)iNeed + 1u);
	if ( sText ) {
		snprintf(
			sText,
			(size_t)iNeed + 1u,
			"ok=%s status=%d cmd=%s result=%s body_len=%u",
			XS_XtpIsOK(objMsg) ? "true" : "false",
			(int)objMsg->Status,
			sCmd ? sCmd : "",
			sResult ? sResult : "",
			(unsigned)objMsg->BodySize
		);
	}
	if ( sCmd ) {
		xrtFree(sCmd);
	}
	return sText;
}

static inline char* XS_XtpMetaJson(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	const char* sResult;
	char* sCmd;
	char* sText;
	json_sax_print_hd hPrint;
	json_print_choice_t tChoice;
	json_string_t tKey;
	json_string_t tVal;

	if ( objMsg == NULL ) {
		sText = (char*)xrtMalloc(18);
		if ( sText ) {
			strcpy(sText, "{\"response\":null}");
		}
		return sText;
	}

	sResult = XS_XtpResultText(objMsg, "");
	sCmd = XS_XtpCmdDup(objMsg, "");
	memset(&tChoice, 0, sizeof(tChoice));
	hPrint = xrtJsonPrintStart(&tChoice);
	if ( hPrint == NULL ) {
		if ( sCmd ) {
			xrtFree(sCmd);
		}
		return NULL;
	}

	memset(&tKey, 0, sizeof(tKey));
	memset(&tVal, 0, sizeof(tVal));
	xrtJsonPrintObjectStart(hPrint, NULL);

	tKey.str = "ok";
	xrtJsonUpdateStringInfo(&tKey);
	xrtJsonPrintBool(hPrint, &tKey, XS_XtpIsOK(objMsg));

	tKey.str = "status";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	xrtJsonPrintInt(hPrint, &tKey, (int32)objMsg->Status);

	tKey.str = "cmd";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	tVal.str = sCmd ? sCmd : "";
	memset(&tVal.info, 0, sizeof(tVal.info));
	xrtJsonUpdateStringInfo(&tVal);
	xrtJsonPrintString(hPrint, &tKey, &tVal);

	tKey.str = "result";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	tVal.str = (char*)(sResult ? sResult : "");
	memset(&tVal.info, 0, sizeof(tVal.info));
	xrtJsonUpdateStringInfo(&tVal);
	xrtJsonPrintString(hPrint, &tKey, &tVal);

	tKey.str = "body_len";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	xrtJsonPrintInt(hPrint, &tKey, (int32)objMsg->BodySize);

	xrtJsonPrintObjectFinish(hPrint);
	sText = xrtJsonPrintFinish(hPrint, NULL, NULL);
	if ( sCmd ) {
		xrtFree(sCmd);
	}
	return sText;
}

static inline char* XS_XtpResultJson(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	const char* sResult;
	char* sText;
	json_sax_print_hd hPrint;
	json_print_choice_t tChoice;
	json_string_t tKey;
	json_string_t tVal;

	if ( objMsg == NULL ) {
		sText = (char*)xrtMalloc(18);
		if ( sText ) {
			strcpy(sText, "{\"response\":null}");
		}
		return sText;
	}

	sResult = XS_XtpResultText(objMsg, "");
	memset(&tChoice, 0, sizeof(tChoice));
	hPrint = xrtJsonPrintStart(&tChoice);
	if ( hPrint == NULL ) {
		return NULL;
	}

	memset(&tKey, 0, sizeof(tKey));
	memset(&tVal, 0, sizeof(tVal));
	xrtJsonPrintObjectStart(hPrint, NULL);

	tKey.str = "result";
	xrtJsonUpdateStringInfo(&tKey);
	tVal.str = (char*)(sResult ? sResult : "");
	xrtJsonUpdateStringInfo(&tVal);
	xrtJsonPrintString(hPrint, &tKey, &tVal);

	tKey.str = "ok";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	xrtJsonPrintBool(hPrint, &tKey, XS_XtpIsOK(objMsg));

	xrtJsonPrintObjectFinish(hPrint);
	return xrtJsonPrintFinish(hPrint, NULL, NULL);
}

static inline char* XS_XtpErrorJson(const void* pMsg, const char* sDefault)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	char* sErr;
	char* sText;
	json_sax_print_hd hPrint;
	json_print_choice_t tChoice;
	json_string_t tKey;
	json_string_t tVal;

	if ( objMsg == NULL ) {
		sText = (char*)xrtMalloc(18);
		if ( sText ) {
			strcpy(sText, "{\"response\":null}");
		}
		return sText;
	}

	sErr = XS_XtpErrorText(objMsg, sDefault);
	memset(&tChoice, 0, sizeof(tChoice));
	hPrint = xrtJsonPrintStart(&tChoice);
	if ( hPrint == NULL ) {
		if ( sErr ) {
			xrtFree(sErr);
		}
		return NULL;
	}

	memset(&tKey, 0, sizeof(tKey));
	memset(&tVal, 0, sizeof(tVal));
	xrtJsonPrintObjectStart(hPrint, NULL);

	tKey.str = "ok";
	xrtJsonUpdateStringInfo(&tKey);
	xrtJsonPrintBool(hPrint, &tKey, FALSE);

	tKey.str = "status";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	xrtJsonPrintInt(hPrint, &tKey, (int32)objMsg->Status);

	tKey.str = "error";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	tVal.str = sErr ? sErr : "";
	memset(&tVal.info, 0, sizeof(tVal.info));
	xrtJsonUpdateStringInfo(&tVal);
	xrtJsonPrintString(hPrint, &tKey, &tVal);

	xrtJsonPrintObjectFinish(hPrint);
	sText = xrtJsonPrintFinish(hPrint, NULL, NULL);
	if ( sErr ) {
		xrtFree(sErr);
	}
	return sText;
}

static inline char* XS_XtpSummaryJson(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	const char* sResult;
	char* sCmd;
	char* sBody;
	char* sText;
	json_sax_print_hd hPrint;
	json_print_choice_t tChoice;
	json_string_t tKey;
	json_string_t tVal;

	if ( objMsg == NULL ) {
		sText = (char*)xrtMalloc(18);
		if ( sText ) {
			strcpy(sText, "{\"response\":null}");
		}
		return sText;
	}

	sResult = XS_XtpResultText(objMsg, "");
	sCmd = XS_XtpCmdDup(objMsg, "");
	sBody = XS_XtpBodyDup(objMsg, "");
	memset(&tChoice, 0, sizeof(tChoice));
	hPrint = xrtJsonPrintStart(&tChoice);
	if ( hPrint == NULL ) {
		if ( sCmd ) {
			xrtFree(sCmd);
		}
		if ( sBody ) {
			xrtFree(sBody);
		}
		return NULL;
	}

	memset(&tKey, 0, sizeof(tKey));
	memset(&tVal, 0, sizeof(tVal));
	xrtJsonPrintObjectStart(hPrint, NULL);

	tKey.str = "ok";
	xrtJsonUpdateStringInfo(&tKey);
	xrtJsonPrintBool(hPrint, &tKey, XS_XtpIsOK(objMsg));

	tKey.str = "status";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	xrtJsonPrintInt(hPrint, &tKey, (int32)objMsg->Status);

	tKey.str = "msg_type";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	xrtJsonPrintInt(hPrint, &tKey, (int32)objMsg->MsgType);

	tKey.str = "msg_id";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	xrtJsonPrintInt64(hPrint, &tKey, (int64)objMsg->MsgID);

	tKey.str = "cmd";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	tVal.str = sCmd ? sCmd : "";
	memset(&tVal.info, 0, sizeof(tVal.info));
	xrtJsonUpdateStringInfo(&tVal);
	xrtJsonPrintString(hPrint, &tKey, &tVal);

	tKey.str = "result";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	tVal.str = (char*)(sResult ? sResult : "");
	memset(&tVal.info, 0, sizeof(tVal.info));
	xrtJsonUpdateStringInfo(&tVal);
	xrtJsonPrintString(hPrint, &tKey, &tVal);

	tKey.str = "body";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	tVal.str = sBody ? sBody : "";
	memset(&tVal.info, 0, sizeof(tVal.info));
	xrtJsonUpdateStringInfo(&tVal);
	xrtJsonPrintString(hPrint, &tKey, &tVal);

	tKey.str = "body_len";
	tKey.info.len = 0;
	xrtJsonUpdateStringInfo(&tKey);
	xrtJsonPrintInt(hPrint, &tKey, (int32)objMsg->BodySize);

	xrtJsonPrintObjectFinish(hPrint);
	sText = xrtJsonPrintFinish(hPrint, NULL, NULL);
	if ( sCmd ) {
		xrtFree(sCmd);
	}
	if ( sBody ) {
		xrtFree(sBody);
	}
	return sText;
}

static inline const char* XS_XtpCmd(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	return objMsg ? objMsg->pCmd : NULL;
}

static inline uint16 XS_XtpCmdLen(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	return objMsg ? objMsg->CmdSize : 0;
}

static inline const void* XS_XtpBody(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	return objMsg ? objMsg->pBody : NULL;
}

static inline uint32 XS_XtpBodyLen(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	return objMsg ? objMsg->BodySize : 0;
}

static inline uint16 XS_XtpParamCount(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	return objMsg ? objMsg->ParamCount : 0;
}

static inline bool XS_XtpNeedReply(const void* pMsg)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	return (objMsg && objMsg->MsgType == XTP_MSG_REQUEST && objMsg->MsgID != 0) ? TRUE : FALSE;
}

static inline bool XS_XtpIsRequest(const void* pMsg)
{
	return XS_XtpMsgType(pMsg) == XTP_MSG_REQUEST ? TRUE : FALSE;
}

static inline bool XS_XtpIsResponse(const void* pMsg)
{
	return XS_XtpMsgType(pMsg) == XTP_MSG_RESPONSE ? TRUE : FALSE;
}

static inline bool XS_XtpIsPush(const void* pMsg)
{
	return XS_XtpMsgType(pMsg) == XTP_MSG_PUSH ? TRUE : FALSE;
}

static inline bool XS_XtpIsEvent(const void* pMsg)
{
	return XS_XtpMsgType(pMsg) == XTP_MSG_EVENT ? TRUE : FALSE;
}

static inline bool XS_XtpCmdIs(const void* pMsg, const char* sCmd)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;
	size_t iCmdLen;

	if ( objMsg == NULL || sCmd == NULL || objMsg->pCmd == NULL ) {
		return FALSE;
	}

	iCmdLen = strlen(sCmd);
	if ( objMsg->CmdSize != (uint16)iCmdLen ) {
		return FALSE;
	}

	return memcmp(objMsg->pCmd, sCmd, iCmdLen) == 0 ? TRUE : FALSE;
}

static inline bool XS_XtpHasParam(const void* pMsg, const char* sKey)
{
	return XS_XtpFindParamView(pMsg, sKey, NULL, NULL);
}

static inline const char* XS_XtpParamText(const void* pMsg, const char* sKey, const char* sDefault)
{
	const char* pVal;
	uint16 iValLen;
	static _Thread_local char sBuf[1024];

	if ( !XS_XtpFindParamView(pMsg, sKey, &pVal, &iValLen) || pVal == NULL ) {
		return sDefault ? sDefault : "";
	}

	if ( iValLen >= sizeof(sBuf) ) {
		iValLen = (uint16)(sizeof(sBuf) - 1);
	}

	memcpy(sBuf, pVal, iValLen);
	sBuf[iValLen] = '\0';
	return sBuf;
}

static inline const char* XS_XtpResultText(const void* pMsg, const char* sDefault)
{
	return XS_XtpParamText(pMsg, "result", sDefault);
}

static inline bool XS_XtpResultIs(const void* pMsg, const char* sResult)
{
	const char* sNow;

	if ( sResult == NULL ) {
		return FALSE;
	}
	sNow = XS_XtpResultText(pMsg, NULL);
	if ( sNow == NULL ) {
		return FALSE;
	}

	return strcmp(sNow, sResult) == 0 ? TRUE : FALSE;
}

static inline bool XS_XtpStatusIs(const void* pMsg, int32 iStatus)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;

	if ( objMsg == NULL ) {
		return FALSE;
	}

	return objMsg->Status == iStatus ? TRUE : FALSE;
}

static inline char* XS_XtpErrorText(const void* pMsg, const char* sDefault)
{
	const XTP_MessageObject objMsg = (const XTP_MessageObject)pMsg;

	if ( objMsg == NULL ) {
		if ( sDefault == NULL ) {
			return NULL;
		}
		return XS_XtpBodyDup(NULL, sDefault);
	}
	if ( objMsg->Status == 0 ) {
		if ( sDefault == NULL ) {
			return NULL;
		}
		return XS_XtpBodyDup(NULL, sDefault);
	}

	return XS_XtpBodyDup(objMsg, sDefault ? sDefault : "response status not ok");
}

static inline char* XS_XtpParamDup(const void* pMsg, const char* sKey, const char* sDefault)
{
	const char* pVal;
	uint16 iValLen;
	char* sText;

	if ( !XS_XtpFindParamView(pMsg, sKey, &pVal, &iValLen) || pVal == NULL ) {
		if ( sDefault == NULL ) {
			return NULL;
		}
		iValLen = (uint16)strlen(sDefault);
		pVal = sDefault;
	}

	sText = (char*)xrtMalloc((size_t)iValLen + 1u);
	if ( sText == NULL ) {
		return NULL;
	}
	if ( iValLen > 0 ) {
		memcpy(sText, pVal, iValLen);
	}
	sText[iValLen] = '\0';
	return sText;
}

static inline int64 XS_XtpParamInt(const void* pMsg, const char* sKey, int64 iDefault)
{
	const char* pVal;
	uint16 iValLen;
	char sBuf[64];
	char* pEnd;
	long long iRet;

	if ( !XS_XtpFindParamView(pMsg, sKey, &pVal, &iValLen) || pVal == NULL || iValLen == 0 ) {
		return iDefault;
	}

	if ( iValLen >= sizeof(sBuf) ) {
		iValLen = (uint16)(sizeof(sBuf) - 1);
	}

	memcpy(sBuf, pVal, iValLen);
	sBuf[iValLen] = '\0';
	iRet = strtoll(sBuf, &pEnd, 10);
	if ( pEnd == sBuf || (pEnd && *pEnd != '\0') ) {
		return iDefault;
	}
	return (int64)iRet;
}

static inline bool XS_XtpParamBool(const void* pMsg, const char* sKey, bool bDefault)
{
	const char* pVal;
	uint16 iValLen;
	char sBuf[16];

	if ( !XS_XtpFindParamView(pMsg, sKey, &pVal, &iValLen) || pVal == NULL || iValLen == 0 ) {
		return bDefault;
	}

	if ( iValLen >= sizeof(sBuf) ) {
		iValLen = (uint16)(sizeof(sBuf) - 1);
	}

	memcpy(sBuf, pVal, iValLen);
	sBuf[iValLen] = '\0';

	if ( _stricmp(sBuf, "1") == 0 || _stricmp(sBuf, "true") == 0 || _stricmp(sBuf, "yes") == 0 || _stricmp(sBuf, "on") == 0 ) {
		return TRUE;
	}
	if ( _stricmp(sBuf, "0") == 0 || _stricmp(sBuf, "false") == 0 || _stricmp(sBuf, "no") == 0 || _stricmp(sBuf, "off") == 0 ) {
		return FALSE;
	}

	return bDefault;
}

static inline bool XS_XtpBuildPacket(
	uint16 iMsgType,
	uint64 iMsgID,
	uint16 iFlags,
	int32 iStatus,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize,
	char** ppSendBuf,
	size_t* piPackSize
)
{
	size_t iPackSize;
	size_t iOffset;
	char* pSendBuf;
	XTP_PackHeader* pHeader;
	XTP_ParamInfo* pParamInfo = NULL;
	uint32 i;
	
	if ( ppSendBuf == NULL || piPackSize == NULL || sCmd == NULL ) {
		return FALSE;
	}
	*ppSendBuf = NULL;
	*piPackSize = 0u;
	
	if ( iCmdSize == 0 ) {
		iCmdSize = strlen(sCmd);
	}
	if ( pBody && iBodySize == 0 ) {
		iBodySize = strlen((const char*)pBody);
	}
	if ( iParamCount > 65535 ) {
		return FALSE;
	}
	
	iPackSize = sizeof(XTP_PackHeader) + ((size_t)iParamCount * sizeof(XTP_ParamInfo)) + iCmdSize + iBodySize;
	if ( iParamCount > 0 ) {
		pParamInfo = (XTP_ParamInfo*)xrtMalloc((size_t)iParamCount * sizeof(XTP_ParamInfo));
		if ( pParamInfo == NULL ) {
			return FALSE;
		}
		
		for ( i = 0; i < iParamCount; i++ ) {
			size_t iKeyLen = arrParam && arrParam[i] ? strlen(arrParam[i]) : 0;
			size_t iValLen = arrValue && arrValue[i] ? strlen(arrValue[i]) : 0;
			
			if ( iKeyLen > 65535 || iValLen > 65535 ) {
				xrtFree(pParamInfo);
				return FALSE;
			}
			
			pParamInfo[i].KeySize = (uint16)iKeyLen;
			pParamInfo[i].ValSize = (uint16)iValLen;
			iPackSize += iKeyLen + iValLen;
		}
	}
	
	if ( iPackSize > UINT32_MAX ) {
		if ( pParamInfo ) {
			xrtFree(pParamInfo);
		}
		return FALSE;
	}
	
	pSendBuf = (char*)xrtMalloc(iPackSize);
	if ( pSendBuf == NULL ) {
		if ( pParamInfo ) {
			xrtFree(pParamInfo);
		}
		return FALSE;
	}
	
	pHeader = (XTP_PackHeader*)pSendBuf;
	memcpy(pHeader->HeadInfo, "xtp\2", 4);
	pHeader->PackSize = (uint32)iPackSize;
	pHeader->Flags = iFlags;
	pHeader->MsgType = iMsgType;
	pHeader->MsgID = iMsgID;
	pHeader->CmdSize = (uint16)iCmdSize;
	pHeader->ParamCount = (uint16)iParamCount;
	pHeader->BodySize = (uint32)iBodySize;
	pHeader->Status = iStatus;
	
	iOffset = sizeof(XTP_PackHeader);
	if ( iParamCount > 0 && pParamInfo ) {
		memcpy(pSendBuf + iOffset, pParamInfo, (size_t)iParamCount * sizeof(XTP_ParamInfo));
		iOffset += (size_t)iParamCount * sizeof(XTP_ParamInfo);
	}
	
	memcpy(pSendBuf + iOffset, sCmd, iCmdSize);
	iOffset += iCmdSize;
	
	for ( i = 0; i < iParamCount; i++ ) {
		if ( pParamInfo[i].KeySize > 0 ) {
			memcpy(pSendBuf + iOffset, arrParam[i], pParamInfo[i].KeySize);
			iOffset += pParamInfo[i].KeySize;
		}
		if ( pParamInfo[i].ValSize > 0 ) {
			memcpy(pSendBuf + iOffset, arrValue[i], pParamInfo[i].ValSize);
			iOffset += pParamInfo[i].ValSize;
		}
	}
	
	if ( pBody && iBodySize > 0 ) {
		memcpy(pSendBuf + iOffset, pBody, iBodySize);
	}

	*ppSendBuf = pSendBuf;
	*piPackSize = iPackSize;
	if ( pParamInfo ) {
		xrtFree(pParamInfo);
	}
	return TRUE;
}

static inline int XS_XtpSendEx(
	void* pStream,
	uint16 iMsgType,
	uint64 iMsgID,
	uint16 iFlags,
	int32 iStatus,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	char* pSendBuf = NULL;
	size_t iPackSize = 0;
	int iOk;

	if ( pStream == NULL ) {
		return FALSE;
	}
	if ( !XS_XtpBuildPacket(iMsgType, iMsgID, iFlags, iStatus, sCmd, iCmdSize, iParamCount, arrParam, arrValue, pBody, iBodySize, &pSendBuf, &iPackSize) ) {
		return FALSE;
	}

	iOk = xrtNetStreamSend((xnetstream*)pStream, pSendBuf, iPackSize) == XRT_NET_OK ? TRUE : FALSE;
	if ( iOk ) {
		XS_XtpMetricAdd(&g_iXsXtpSendCount, 1);
		XS_XtpMetricAdd(&g_iXsXtpSendBytes, (int64)iPackSize);
	}
	xrtFree(pSendBuf);
	return iOk;
}

static inline int XS_XtpSend(
	void* pStream,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpSendEx(
		pStream,
		XTP_MSG_REQUEST,
		0,
		0,
		0,
		sCmd,
		iCmdSize,
		iParamCount,
		arrParam,
		arrValue,
		pBody,
		iBodySize
	);
}

static inline int XS_XtpSendRequest(
	void* pStream,
	uint64 iMsgID,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpSendEx(pStream, XTP_MSG_REQUEST, iMsgID, 0, 0, sCmd, iCmdSize, iParamCount, arrParam, arrValue, pBody, iBodySize);
}

static inline int XS_XtpSendPush(
	void* pStream,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpSendEx(pStream, XTP_MSG_PUSH, 0, 0, 0, sCmd, iCmdSize, iParamCount, arrParam, arrValue, pBody, iBodySize);
}

static inline int XS_XtpSendEvent(
	void* pStream,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpSendEx(pStream, XTP_MSG_EVENT, 0, 0, 0, sCmd, iCmdSize, iParamCount, arrParam, arrValue, pBody, iBodySize);
}

static inline int XS_XtpReplyEx(
	void* pStream,
	const void* pReqMsg,
	int32 iStatus,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	const XTP_MessageObject objReq = (const XTP_MessageObject)pReqMsg;
	uint64 iMsgID = objReq ? objReq->MsgID : 0;
	
	if ( objReq == NULL || iMsgID == 0 ) {
		return TRUE;
	}
	
	return XS_XtpSendEx(
		pStream,
		XTP_MSG_RESPONSE,
		iMsgID,
		0,
		iStatus,
		sCmd,
		iCmdSize,
		iParamCount,
		arrParam,
		arrValue,
		pBody,
		iBodySize
	);
}

static inline int XS_XtpReplyText(
	void* pStream,
	const void* pReqMsg,
	int32 iStatus,
	const char* sCmd,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sText
)
{
	return XS_XtpReplyEx(
		pStream,
		pReqMsg,
		iStatus,
		sCmd,
		0,
		iParamCount,
		arrParam,
		arrValue,
		sText ? sText : "",
		sText ? strlen(sText) : 0
	);
}

static inline int XS_XtpReplyJson(
	void* pStream,
	const void* pReqMsg,
	int32 iStatus,
	const char* sCmd,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sJson
)
{
	return XS_XtpReplyEx(
		pStream,
		pReqMsg,
		iStatus,
		sCmd,
		0,
		iParamCount,
		arrParam,
		arrValue,
		sJson ? sJson : "{}",
		sJson ? strlen(sJson) : 2
	);
}

static inline int XS_XtpReplyOKText(
	void* pStream,
	const void* pReqMsg,
	const char* sCmd,
	const char* sText
)
{
	const char* arrParam[1];
	const char* arrValue[1];

	arrParam[0] = "result";
	arrValue[0] = "ok";
	return XS_XtpReplyText(pStream, pReqMsg, 0, sCmd, 1, arrParam, arrValue, sText);
}

static inline int XS_XtpReplyErrorText(
	void* pStream,
	const void* pReqMsg,
	int32 iStatus,
	const char* sCmd,
	const char* sText
)
{
	const char* arrParam[1];
	const char* arrValue[1];

	arrParam[0] = "result";
	arrValue[0] = "error";
	return XS_XtpReplyText(pStream, pReqMsg, iStatus, sCmd, 1, arrParam, arrValue, sText);
}

static inline int XS_XtpReplyOKJson(
	void* pStream,
	const void* pReqMsg,
	const char* sCmd,
	const char* sJson
)
{
	const char* arrParam[1];
	const char* arrValue[1];

	arrParam[0] = "result";
	arrValue[0] = "ok";
	return XS_XtpReplyJson(pStream, pReqMsg, 0, sCmd, 1, arrParam, arrValue, sJson);
}

static inline int XS_XtpReplyErrorJson(
	void* pStream,
	const void* pReqMsg,
	int32 iStatus,
	const char* sCmd,
	const char* sJson
)
{
	const char* arrParam[1];
	const char* arrValue[1];

	arrParam[0] = "result";
	arrValue[0] = "error";
	return XS_XtpReplyJson(pStream, pReqMsg, iStatus, sCmd, 1, arrParam, arrValue, sJson);
}

static inline int XS_XtpReplyMissingParam(
	void* pStream,
	const void* pReqMsg,
	const char* sParam
)
{
	char sJson[256];

	snprintf(
		sJson,
		sizeof(sJson),
		"{\"result\":\"error\",\"message\":\"missing param\",\"param\":\"%s\"}",
		sParam ? sParam : ""
	);
	return XS_XtpReplyErrorJson(pStream, pReqMsg, 422, "xtp.error", sJson);
}

static inline int XS_XtpReplyUnsupportedCmd(
	void* pStream,
	const void* pReqMsg,
	const char* sCmd
)
{
	char sJson[256];

	snprintf(
		sJson,
		sizeof(sJson),
		"{\"result\":\"error\",\"message\":\"unsupported cmd\",\"cmd\":\"%s\"}",
		sCmd ? sCmd : ""
	);
	return XS_XtpReplyErrorJson(pStream, pReqMsg, 400, "xtp.error", sJson);
}

static inline int XS_XtpReply(
	void* pStream,
	const void* pReqMsg,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpReplyEx(
		pStream,
		pReqMsg,
		0,
		sCmd,
		iCmdSize,
		iParamCount,
		arrParam,
		arrValue,
		pBody,
		iBodySize
	);
}



static uint32 XS_XtpAcceptThread(ptr pArg)
{
	XS_XtpHandle* objHandle = (XS_XtpHandle*)pArg;
	
	while ( objHandle && objHandle->pListener && !objHandle->bStopAccept ) {
		xnetstream* pStream = NULL;
		xnet_result iRet = xrtNetListenerAcceptTimeout(objHandle->pListener, 500, &pStream);
		
		if ( iRet == XRT_NET_TIMEOUT ) {
			continue;
		}
		if ( iRet == XRT_NET_OK ) {
			continue;
		}
		if ( objHandle->bStopAccept ) {
			break;
		}
		
		XS_LogWarn(
			"xtp accept failed: server=%s code=%d",
			objHandle->pServer && objHandle->pServer->Name ? objHandle->pServer->Name : "(null)",
			(int)iRet
		);
		xrtSleep(50);
	}
	
	return 0;
}

static uint32 XS_XtpAcceptThreadTLS(ptr pArg)
{
	XS_XtpHandle* objHandle = (XS_XtpHandle*)pArg;
	
	while ( objHandle && objHandle->pListenerTLS && !objHandle->bStopAccept ) {
		xnetstream* pStream = NULL;
		xnet_result iRet = xrtNetListenerAcceptTimeout(objHandle->pListenerTLS, 500, &pStream);
		
		if ( iRet == XRT_NET_TIMEOUT ) {
			continue;
		}
		if ( iRet == XRT_NET_OK ) {
			continue;
		}
		if ( objHandle->bStopAccept ) {
			break;
		}
		
		XS_LogWarn(
			"xtps accept failed: server=%s code=%d",
			objHandle->pServer && objHandle->pServer->Name ? objHandle->pServer->Name : "(null)",
			(int)iRet
		);
		xrtSleep(50);
	}
	
	return 0;
}

static uint32 XS_XtpIdleThread(ptr pArg)
{
	XS_XtpHandle* objHandle = (XS_XtpHandle*)pArg;

	while ( objHandle && !objHandle->bStopAccept ) {
		XS_ServerConfig* objServer = objHandle->pServer;
		uint32 iIdleTimeout = objServer ? objServer->IdleTimeout : 0u;

		if ( iIdleTimeout > 0u && objHandle->pConnLock && objHandle->arrConn ) {
			int64 iNowMS = XS_XtpNowMS();
			xarray arrClose = xrtArrayCreate(sizeof(xnetstream*), XRT_OBJMODE_LOCAL);
			uint32 i;

			xrtMutexLock(objHandle->pConnLock);
			for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
				XS_XtpConnContext** ppItem = (XS_XtpConnContext**)xrtArrayGet(objHandle->arrConn, i);
				XS_XtpConnContext* objCtx = ppItem ? *ppItem : NULL;

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
					XS_XtpRecordIdleClose();
					xrtNetStreamClose(*ppClose, 0u);
				}
			}
			xrtArrayDestroy(arrClose);
		}

		xrtSleep(500);
	}

	return 0;
}

static bool XS_XtpOnAccept(ptr pOwner, xnetlistener* pListener, xnetstream* pStream)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_XtpHandle* objHandle;
	XS_XtpConnContext* objCtx;
	
	(void)pListener;
	
	objCtx = (XS_XtpConnContext*)xrtCalloc(1, sizeof(XS_XtpConnContext));
	if ( objCtx == NULL ) {
		return FALSE;
	}

	objCtx->pServer = objServer;
	objCtx->pStream = pStream;
	objHandle = objServer ? (XS_XtpHandle*)objServer->pHandle : NULL;
	objCtx->pTracker = objHandle;
	XS_XtpTouch(objCtx);
	xrtNetStreamSetUserData(pStream, objCtx);
	XS_XtpTrackConn(objHandle, objCtx);
	XS_XtpMetricUpdateMax(&g_iXsXtpConnPeak, XS_XtpMetricAdd(&g_iXsXtpConnCurrent, 1));
	return TRUE;
}

static void XS_XtpOnOpen(ptr pOwner, xnetstream* pStream)
{
	XS_XtpConnContext* objCtx = (XS_XtpConnContext*)pOwner;
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;
	(void)pStream;
	
	XS_XtpMetricAdd(&g_iXsXtpOpenCount, 1);
	XS_XtpTouch(objCtx);
	if ( objServer && objServer->ConnLimit > 0u && XS_XtpTrackedConnCount((XS_XtpHandle*)objCtx->pTracker) > (int64)objServer->ConnLimit ) {
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		XS_XtpRecordConnLimitClose();
		XS_LogWarn(
			"xtp conn limit exceeded: server=%s current=%lld limit=%u",
			objServer->Name ? objServer->Name : "(null)",
			(long long)XS_XtpTrackedConnCount((XS_XtpHandle*)objCtx->pTracker),
			(unsigned)objServer->ConnLimit
		);
		xrtNetStreamClose(pStream, 0u);
		return;
	}
	if ( objServer && objServer->procStreamOpen ) {
		objServer->procStreamOpen(objServer, objCtx ? objCtx->pStream : pStream);
	}
}

static void XS_XtpOnRecv(ptr pOwner, xnetstream* pStream, xnetchain* pChain)
{
	XS_XtpConnContext* objCtx = (XS_XtpConnContext*)pOwner;
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;
	
	if ( objCtx == NULL || pChain == NULL ) {
		return;
	}
	XS_XtpTouch(objCtx);
	if ( !XS_XtpAppendChain(objCtx, pChain) ) {
		XS_LogWarn("xtp recv append failed: server=%s", objServer && objServer->Name ? objServer->Name : "(null)");
		xrtNetStreamClose(pStream, XNET_CLOSE_F_ABORT);
		return;
	}
	
	for ( ;; ) {
		XTP_Message tMsg;
		size_t iPackBytes = 0;
		bool bHandled = FALSE;
		
		memset(&tMsg, 0, sizeof(tMsg));
		if ( !XS_XtpParseMessage(objCtx, &tMsg, &iPackBytes) ) {
			if ( XS_XtpPacketHeaderInvalid(objCtx) ) {
				xrtNetStreamClose(pStream, XNET_CLOSE_F_ABORT);
			}
			break;
		}
		
		if ( objServer && objServer->procXtpMessage ) {
			bHandled = objServer->procXtpMessage(objServer, pStream, &tMsg);
		}
		XS_XtpMetricAdd(&g_iXsXtpMsgCount, 1);
		g_iXsXtpLastMsgType = (int64)tMsg.MsgType;
		g_iXsXtpLastStatus = (int64)tMsg.Status;
		g_iXsXtpLastMsgID = (int64)tMsg.MsgID;
		g_iXsXtpLastFlags = (int64)tMsg.Flags;
		g_iXsXtpLastParamCount = (int64)tMsg.ParamCount;
		g_iXsXtpLastBodySize = (int64)tMsg.BodySize;
		g_tXsXtpLastTime = xrtNow();
		if ( tMsg.MsgType == XTP_MSG_REQUEST ) {
			XS_XtpMetricAdd(&g_iXsXtpReqCount, 1);
		} else if ( tMsg.MsgType == XTP_MSG_RESPONSE ) {
			XS_XtpMetricAdd(&g_iXsXtpRespCount, 1);
		} else if ( tMsg.MsgType == XTP_MSG_PUSH ) {
			XS_XtpMetricAdd(&g_iXsXtpPushCount, 1);
		} else if ( tMsg.MsgType == XTP_MSG_EVENT ) {
			XS_XtpMetricAdd(&g_iXsXtpEventCount, 1);
		}
		if ( tMsg.pCmd && tMsg.CmdSize > 0 ) {
			size_t iCmdCopy = (size_t)tMsg.CmdSize;
			if ( iCmdCopy >= sizeof(g_sXsXtpLastCmd) ) {
				iCmdCopy = sizeof(g_sXsXtpLastCmd) - 1;
			}
			memcpy(g_sXsXtpLastCmd, tMsg.pCmd, iCmdCopy);
			g_sXsXtpLastCmd[iCmdCopy] = '\0';
		} else {
			g_sXsXtpLastCmd[0] = '\0';
		}
		
		XS_LogInfo(
			"xtp recv: server=%s stream=%p type=%u msg_id=%llu status=%d cmd=%.*s params=%u body=%u handled=%s",
			objServer && objServer->Name ? objServer->Name : "(null)",
			(void*)pStream,
			(unsigned)tMsg.MsgType,
			(unsigned long long)tMsg.MsgID,
			(int)tMsg.Status,
			(int)tMsg.CmdSize,
			tMsg.pCmd ? tMsg.pCmd : "",
			(unsigned)tMsg.ParamCount,
			(unsigned)tMsg.BodySize,
			bHandled ? "true" : "false"
		);
		
		if ( !bHandled && XS_XtpNeedReply(&tMsg) ) {
			const char* arrParam[1] = {"result"};
			const char* arrValue[1] = {"unhandled"};
			
			(void)XS_XtpReplyEx(
				pStream,
				&tMsg,
				-404,
				"error",
				0,
				1,
				arrParam,
				arrValue,
				tMsg.pCmd ? tMsg.pCmd : "",
				tMsg.CmdSize
			);
		}
		
		XS_XtpConsume(objCtx, iPackBytes);
		XS_XtpFreeMessage(&tMsg);
	}
}

static void XS_XtpOnClose(ptr pOwner, xnetstream* pStream, xnet_result iReason)
{
	XS_XtpConnContext* objCtx = (XS_XtpConnContext*)pOwner;
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;
	
	XS_XtpMetricAdd(&g_iXsXtpCloseCount, 1);
	if ( XS_XtpMetricAdd(&g_iXsXtpConnCurrent, -1) < 0 ) {
		XS_XtpMetricAdd(&g_iXsXtpConnCurrent, -XS_XtpMetricGet(&g_iXsXtpConnCurrent));
	}
	if ( objServer && objServer->procStreamClose ) {
		objServer->procStreamClose(objServer, pStream, (int)iReason);
	}
	
	XS_XtpUntrackConn((XS_XtpHandle*)objCtx->pTracker, objCtx);
	xrtNetStreamSetUserData(pStream, NULL);
	XS_XtpDestroyContext(objCtx);
}

static void XS_XtpOnError(ptr pOwner, xnetstream* pStream, int iSysErr)
{
	XS_XtpConnContext* objCtx = (XS_XtpConnContext*)pOwner;
	if ( objCtx == NULL && pStream != NULL ) {
		objCtx = (XS_XtpConnContext*)xrtNetStreamGetUserData(pStream);
	}
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;

	if ( objCtx == NULL ) {
		return;
	}
	if ( objCtx && objCtx->bClosing ) {
		return;
	}
	if ( iSysErr == -1 ) {
		return;
	}
	
	XS_XtpMetricAdd(&g_iXsXtpErrorCount, 1);
	g_iXsXtpLastErrorCode = (int64)iSysErr;
	g_tXsXtpLastErrorTime = xrtNow();
	XS_LogWarn(
		"xtp error: server=%s sys=%d",
		objServer && objServer->Name ? objServer->Name : "(null)",
		iSysErr
	);
}

static const xnetlistenerevents* XS_XtpListenerEvents(void)
{
	static const xnetlistenerevents tEvents = {
		XS_XtpOnAccept,
		NULL
	};
	
	return &tEvents;
}

static const xnetstreamevents* XS_XtpStreamEvents(void)
{
	static const xnetstreamevents tEvents = {
		XS_XtpOnOpen,
		XS_XtpOnRecv,
		NULL,
		XS_XtpOnClose,
		XS_XtpOnError,
		NULL,
		NULL
	};
	
	return &tEvents;
}

static inline bool XS_XtpInitServer(xnetengine* pEngine, XS_ServerConfig* objServer)
{
	xnetlistenconfig tCfg;
	XS_XtpHandle* objHandle;
	
	if ( objServer == NULL || pEngine == NULL ) {
		return FALSE;
	}
	
	objHandle = (XS_XtpHandle*)xrtCalloc(1, sizeof(XS_XtpHandle));
	if ( objHandle == NULL ) {
		XS_ReportError("xtp init failed: alloc handle");
		return FALSE;
	}
	
	xrtNetListenConfigInit(&tCfg);
	if ( !XS_BuildBindAddr(objServer, FALSE, &tCfg.tBindAddr) ) {
		xrtFree(objHandle);
		XS_ReportError("xtp init failed: invalid addr: %s", objServer->Addr ? objServer->Addr : "(null)");
		return FALSE;
	}
	tCfg.iBacklog = objServer->Backlog;
	tCfg.iRecvLimit = objServer->RecvLimit;
	
	objHandle->pListener = xrtNetListenerCreate(pEngine, &tCfg, XS_XtpListenerEvents(), XS_XtpStreamEvents(), objServer);
	if ( objHandle->pListener == NULL ) {
		xrtFree(objHandle);
		XS_ReportError("xtp init failed: create listener");
		return FALSE;
	}

	objHandle->pConnLock = xrtMutexCreate();
	objHandle->arrConn = xrtArrayCreate(sizeof(XS_XtpConnContext*), XRT_OBJMODE_SHARED);
	if ( objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		if ( objHandle->arrConn ) {
			xrtArrayDestroy(objHandle->arrConn);
		}
		if ( objHandle->pConnLock ) {
			xrtMutexDestroy(objHandle->pConnLock);
		}
		xrtNetListenerDestroy(objHandle->pListener);
		xrtFree(objHandle);
		XS_ReportError("xtp init failed: alloc conn tracker");
		return FALSE;
	}
	
	if ( objServer->EnableTLS ) {
		if ( objServer->BindPortTLS == 0 ) {
			xrtNetListenerDestroy(objHandle->pListener);
			xrtArrayDestroy(objHandle->arrConn);
			xrtMutexDestroy(objHandle->pConnLock);
			xrtFree(objHandle);
			XS_ReportError(
				"xtps init failed: missing port_tls enable_tls=%s ip_tls=%s addr_tls=%s",
				objServer->EnableTLS ? "true" : "false",
				objServer->BindIPTLS ? objServer->BindIPTLS : "(null)",
				objServer->AddrTLS ? objServer->AddrTLS : "(null)"
			);
			return FALSE;
		}
		if ( objServer->TlsConfig.sCertFile == NULL || objServer->TlsConfig.sKeyFile == NULL ) {
			xrtNetListenerDestroy(objHandle->pListener);
			xrtArrayDestroy(objHandle->arrConn);
			xrtMutexDestroy(objHandle->pConnLock);
			xrtFree(objHandle);
			XS_ReportError("xtps init failed: tls cert/key missing");
			return FALSE;
		}
		
		xrtNetListenConfigInit(&tCfg);
		if ( !XS_BuildBindAddr(objServer, TRUE, &tCfg.tBindAddr) ) {
			xrtNetListenerDestroy(objHandle->pListener);
			xrtArrayDestroy(objHandle->arrConn);
			xrtMutexDestroy(objHandle->pConnLock);
			xrtFree(objHandle);
			XS_ReportError("xtps init failed: invalid addr: %s", objServer->AddrTLS ? objServer->AddrTLS : "(null)");
			return FALSE;
		}
		tCfg.iBacklog = objServer->Backlog;
		tCfg.iRecvLimit = objServer->RecvLimit;
		tCfg.pTlsConfig = &objServer->TlsConfig;
		objHandle->pListenerTLS = xrtNetListenerCreate(pEngine, &tCfg, XS_XtpListenerEvents(), XS_XtpStreamEvents(), objServer);
		if ( objHandle->pListenerTLS == NULL ) {
			xrtNetListenerDestroy(objHandle->pListener);
			xrtArrayDestroy(objHandle->arrConn);
			xrtMutexDestroy(objHandle->pConnLock);
			xrtFree(objHandle);
			XS_ReportError("xtps init failed: create tls listener");
			return FALSE;
		}
	}
	
	objHandle->pServer = objServer;
	objServer->pHandle = objHandle;
	XS_LogInfo(
		"xtp init: server=%s addr=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)"
	);
	if ( objHandle->pListenerTLS ) {
		XS_LogInfo(
			"xtps init: server=%s addr=%s",
			objServer->Name ? objServer->Name : "(null)",
			objServer->AddrTLS ? objServer->AddrTLS : "(null)"
		);
	}
	return TRUE;
}

static inline bool XS_XtpStartServer(XS_ServerConfig* objServer)
{
	XS_XtpHandle* objHandle;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	
	objHandle = (XS_XtpHandle*)objServer->pHandle;
	if ( objHandle == NULL || objHandle->pListener == NULL ) {
		XS_ReportError("xtp start failed: handle is null");
		return FALSE;
	}
	if ( xrtNetListenerStart(objHandle->pListener) != XRT_NET_OK ) {
		XS_ReportError("xtp start failed: listener start error");
		return FALSE;
	}
	
	objHandle->bStopAccept = FALSE;
	objHandle->hAcceptThread = xrtThreadCreate(XS_XtpAcceptThread, objHandle, 0);
	if ( objHandle->hAcceptThread == NULL ) {
		XS_ReportError("xtp start failed: create accept thread error");
		xrtNetListenerStop(objHandle->pListener);
		return FALSE;
	}
	objHandle->hIdleThread = xrtThreadCreate(XS_XtpIdleThread, objHandle, 0);
	if ( objHandle->hIdleThread == NULL ) {
		XS_ReportError("xtp start failed: create idle thread error");
		objHandle->bStopAccept = TRUE;
		xrtThreadWait(objHandle->hAcceptThread);
		xrtThreadDestroy(objHandle->hAcceptThread);
		objHandle->hAcceptThread = NULL;
		xrtNetListenerStop(objHandle->pListener);
		return FALSE;
	}
	if ( objHandle->pListenerTLS ) {
		if ( xrtNetListenerStart(objHandle->pListenerTLS) != XRT_NET_OK ) {
			XS_ReportError("xtps start failed: listener start error");
			objHandle->bStopAccept = TRUE;
			xrtThreadWait(objHandle->hIdleThread);
			xrtThreadDestroy(objHandle->hIdleThread);
			objHandle->hIdleThread = NULL;
			xrtThreadWait(objHandle->hAcceptThread);
			xrtThreadDestroy(objHandle->hAcceptThread);
			objHandle->hAcceptThread = NULL;
			xrtNetListenerStop(objHandle->pListener);
			return FALSE;
		}
		objHandle->hAcceptThreadTLS = xrtThreadCreate(XS_XtpAcceptThreadTLS, objHandle, 0);
		if ( objHandle->hAcceptThreadTLS == NULL ) {
			XS_ReportError("xtps start failed: create accept thread error");
			xrtNetListenerStop(objHandle->pListenerTLS);
			objHandle->bStopAccept = TRUE;
			xrtThreadWait(objHandle->hIdleThread);
			xrtThreadDestroy(objHandle->hIdleThread);
			objHandle->hIdleThread = NULL;
			xrtThreadWait(objHandle->hAcceptThread);
			xrtThreadDestroy(objHandle->hAcceptThread);
			objHandle->hAcceptThread = NULL;
			xrtNetListenerStop(objHandle->pListener);
			return FALSE;
		}
	}
	
	XS_LogInfo(
		"xtp start: server=%s addr=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)"
	);
	if ( objHandle->pListenerTLS ) {
		XS_LogInfo(
			"xtps start: server=%s addr=%s",
			objServer->Name ? objServer->Name : "(null)",
			objServer->AddrTLS ? objServer->AddrTLS : "(null)"
		);
	}
	return TRUE;
}

static inline void XS_XtpStopServer(XS_ServerConfig* objServer)
{
	XS_XtpHandle* objHandle;
	
	if ( objServer == NULL ) {
		return;
	}
	
	objHandle = (XS_XtpHandle*)objServer->pHandle;
	if ( objHandle ) {
		objHandle->bStopAccept = TRUE;
		if ( objHandle->pListener ) {
			xrtNetListenerStop(objHandle->pListener);
		}
		if ( objHandle->pListenerTLS ) {
			xrtNetListenerStop(objHandle->pListenerTLS);
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
		if ( objHandle->hAcceptThreadTLS ) {
			xrtThreadWait(objHandle->hAcceptThreadTLS);
			xrtThreadDestroy(objHandle->hAcceptThreadTLS);
			objHandle->hAcceptThreadTLS = NULL;
		}
		if ( objHandle->pListener ) {
			xrtNetListenerDestroy(objHandle->pListener);
		}
		if ( objHandle->pListenerTLS ) {
			xrtNetListenerDestroy(objHandle->pListenerTLS);
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
		"xtp stop: server=%s",
		objServer->Name ? objServer->Name : "(null)"
	);
	if ( objServer->EnableTLS ) {
		XS_LogInfo(
			"xtps stop: server=%s",
			objServer->Name ? objServer->Name : "(null)"
		);
	}
}

#endif
