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
} XS_XtpConnContext;

typedef struct {
	xnetlistener* pListener;
	xnetlistener* pListenerTLS;
	XS_ServerConfig* pServer;
	xthread hAcceptThread;
	xthread hAcceptThreadTLS;
	volatile bool bStopAccept;
} XS_XtpHandle;

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
	size_t iPackSize;
	size_t iOffset;
	char* pSendBuf;
	XTP_PackHeader* pHeader;
	XTP_ParamInfo* pParamInfo = NULL;
	uint32 i;
	
	if ( pStream == NULL || sCmd == NULL ) {
		return FALSE;
	}
	
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
	
	i = xrtNetStreamSend((xnetstream*)pStream, pSendBuf, iPackSize) == XRT_NET_OK ? TRUE : FALSE;
	if ( i ) {
		XS_XtpMetricAdd(&g_iXsXtpSendCount, 1);
		XS_XtpMetricAdd(&g_iXsXtpSendBytes, (int64)iPackSize);
	}
	xrtFree(pSendBuf);
	if ( pParamInfo ) {
		xrtFree(pParamInfo);
	}
	return (int)i;
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

static bool XS_XtpOnAccept(ptr pOwner, xnetlistener* pListener, xnetstream* pStream)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_XtpConnContext* objCtx;
	
	(void)pListener;
	
	objCtx = (XS_XtpConnContext*)xrtCalloc(1, sizeof(XS_XtpConnContext));
	if ( objCtx == NULL ) {
		return FALSE;
	}

	objCtx->pServer = objServer;
	objCtx->pStream = pStream;
	xrtNetStreamSetUserData(pStream, objCtx);
	XS_XtpMetricUpdateMax(&g_iXsXtpConnPeak, XS_XtpMetricAdd(&g_iXsXtpConnCurrent, 1));
	return TRUE;
}

static void XS_XtpOnOpen(ptr pOwner, xnetstream* pStream)
{
	XS_XtpConnContext* objCtx = (XS_XtpConnContext*)pOwner;
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;
	(void)pStream;
	
	XS_XtpMetricAdd(&g_iXsXtpOpenCount, 1);
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
	
	xrtNetStreamSetUserData(pStream, NULL);
	XS_XtpDestroyContext(objCtx);
	xrtNetStreamDestroy(pStream);
}

static void XS_XtpOnError(ptr pOwner, xnetstream* pStream, int iSysErr)
{
	XS_XtpConnContext* objCtx = (XS_XtpConnContext*)pOwner;
	XS_ServerConfig* objServer = objCtx ? objCtx->pServer : NULL;
	(void)pStream;
	
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
	
	if ( objServer->EnableTLS ) {
		if ( objServer->BindPortTLS == 0 ) {
			xrtNetListenerDestroy(objHandle->pListener);
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
			xrtFree(objHandle);
			XS_ReportError("xtps init failed: tls cert/key missing");
			return FALSE;
		}
		
		xrtNetListenConfigInit(&tCfg);
		if ( !XS_BuildBindAddr(objServer, TRUE, &tCfg.tBindAddr) ) {
			xrtNetListenerDestroy(objHandle->pListener);
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
	if ( objHandle->pListenerTLS ) {
		if ( xrtNetListenerStart(objHandle->pListenerTLS) != XRT_NET_OK ) {
			XS_ReportError("xtps start failed: listener start error");
			objHandle->bStopAccept = TRUE;
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
