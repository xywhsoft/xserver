#ifndef XS_SCRIPT_SCRIPT_API_H
#define XS_SCRIPT_SCRIPT_API_H

#include "../../lib/sqlite3.h"
#include "import_c/import_all.h"

typedef struct XTP_Message XTP_Message;
typedef XTP_Message* XTP_MessageObject;

static inline int XS_XtpSend(
	void* pStream,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
);
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
);
static inline int XS_XtpSendPush(
	void* pStream,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
);
static inline int XS_XtpSendEvent(
	void* pStream,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
);
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
);
static inline int XS_XtpSendPush(
	void* pStream,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
);
static inline int XS_XtpSendEvent(
	void* pStream,
	const char* sCmd,
	size_t iCmdSize,
	uint32 iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
);

static inline const char* XS_XtpGetParam(void* pMsg, const char* sKey);
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
);
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
);
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
);
static inline uint64 XS_XtpMsgId(const void* pMsg);
static inline uint16 XS_XtpMsgType(const void* pMsg);
static inline uint16 XS_XtpMsgFlags(const void* pMsg);
static inline bool XS_XtpIsOK(const void* pMsg);
static inline int32 XS_XtpStatus(const void* pMsg);
static inline const char* XS_XtpCmd(const void* pMsg);
static inline uint16 XS_XtpCmdLen(const void* pMsg);
static inline const void* XS_XtpBody(const void* pMsg);
static inline uint32 XS_XtpBodyLen(const void* pMsg);
static inline char* XS_XtpBodyDup(const void* pMsg, const char* sDefault);
static inline char* XS_XtpCmdDup(const void* pMsg, const char* sDefault);
static inline xvalue XS_XtpParamsValue(const void* pMsg);
static inline xvalue XS_XtpValue(const void* pMsg);
static inline char* XS_XtpMetaText(const void* pMsg);
static inline char* XS_XtpMetaJson(const void* pMsg);
static inline char* XS_XtpResultJson(const void* pMsg);
static inline char* XS_XtpErrorJson(const void* pMsg, const char* sDefault);
static inline char* XS_XtpSummaryText(const void* pMsg);
static inline char* XS_XtpSummaryJson(const void* pMsg);
static inline uint16 XS_XtpParamCount(const void* pMsg);
static inline bool XS_XtpParamAt(const void* pMsg, uint16 iIndex, const char** ppKey, uint16* piKeyLen, const char** ppVal, uint16* piValLen);
static inline bool XS_XtpFindParamView(const void* pMsg, const char* sKey, const char** ppVal, uint16* piValLen);
static inline bool XS_XtpNeedReply(const void* pMsg);
static inline bool XS_XtpIsRequest(const void* pMsg);
static inline bool XS_XtpIsResponse(const void* pMsg);
static inline bool XS_XtpIsPush(const void* pMsg);
static inline bool XS_XtpIsEvent(const void* pMsg);
static inline bool XS_XtpCmdIs(const void* pMsg, const char* sCmd);
static inline bool XS_XtpHasParam(const void* pMsg, const char* sKey);
static inline const char* XS_XtpParamText(const void* pMsg, const char* sKey, const char* sDefault);
static inline const char* XS_XtpResultText(const void* pMsg, const char* sDefault);
static inline bool XS_XtpResultIs(const void* pMsg, const char* sResult);
static inline bool XS_XtpStatusIs(const void* pMsg, int32 iStatus);
static inline char* XS_XtpErrorText(const void* pMsg, const char* sDefault);
static inline char* XS_XtpParamDup(const void* pMsg, const char* sKey, const char* sDefault);
static inline int64 XS_XtpParamInt(const void* pMsg, const char* sKey, int64 iDefault);
static inline bool XS_XtpParamBool(const void* pMsg, const char* sKey, bool bDefault);
static inline int XS_XtpReplyText(void* pStream, const void* pReqMsg, int32 iStatus, const char* sCmd, uint32 iParamCount, const char** arrParam, const char** arrValue, const char* sText);
static inline int XS_XtpReplyJson(void* pStream, const void* pReqMsg, int32 iStatus, const char* sCmd, uint32 iParamCount, const char** arrParam, const char** arrValue, const char* sJson);
static inline int XS_XtpReplyOKText(void* pStream, const void* pReqMsg, const char* sCmd, const char* sText);
static inline int XS_XtpReplyErrorText(void* pStream, const void* pReqMsg, int32 iStatus, const char* sCmd, const char* sText);
static inline int XS_XtpReplyOKJson(void* pStream, const void* pReqMsg, const char* sCmd, const char* sJson);
static inline int XS_XtpReplyErrorJson(void* pStream, const void* pReqMsg, int32 iStatus, const char* sCmd, const char* sJson);
static inline int XS_XtpReplyMissingParam(void* pStream, const void* pReqMsg, const char* sParam);
static inline int XS_XtpReplyUnsupportedCmd(void* pStream, const void* pReqMsg, const char* sCmd);
static inline int XS_XtpReplyMissingParam(void* pStream, const void* pReqMsg, const char* sParam);
static inline int XS_XtpReplyUnsupportedCmd(void* pStream, const void* pReqMsg, const char* sCmd);
static inline void* XS_XtpClientOpen(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iTimeoutMs);
static inline void XS_XtpClientClose(void* pClient);
static inline void* XS_XtpClientDo(void* pClient, uint64 iMsgID, const char* sCmd, uint32 iParamCount, const char** arrParam, const char** arrValue, const void* pBody, size_t iBodySize, uint32 iTimeoutMs);
static inline void* XS_XtpClientDoText(void* pClient, uint64 iMsgID, const char* sCmd, uint32 iParamCount, const char** arrParam, const char** arrValue, const char* sText, uint32 iTimeoutMs);
static inline void* XS_XtpClientDoSimple(void* pClient, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs);
static inline void* XS_XtpClientDoJson(void* pClient, uint64 iMsgID, const char* sCmd, uint32 iParamCount, const char** arrParam, const char** arrValue, const char* sJson, uint32 iTimeoutMs);
static inline void* XS_XtpClientCall(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iParamCount, const char** arrParam, const char** arrValue, const void* pBody, size_t iBodySize, uint32 iTimeoutMs);
static inline void* XS_XtpClientCallText(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iParamCount, const char** arrParam, const char** arrValue, const char* sText, uint32 iTimeoutMs);
static inline void* XS_XtpClientCallSimple(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs);
static inline void* XS_XtpClientCallJson(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iParamCount, const char** arrParam, const char** arrValue, const char* sJson, uint32 iTimeoutMs);
static inline char* XS_XtpClientCallSimpleBody(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs, const char* sDefault);
static inline char* XS_XtpClientCallTextBody(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iParamCount, const char** arrParam, const char** arrValue, const char* sText, uint32 iTimeoutMs, const char* sDefault);
static inline char* XS_XtpClientCallJsonBody(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iParamCount, const char** arrParam, const char** arrValue, const char* sJson, uint32 iTimeoutMs, const char* sDefault);
static inline char* XS_XtpClientCallSimpleSummary(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs);
static inline char* XS_XtpClientCallSimpleSummaryJson(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs);
static inline char* XS_XtpClientCallSimpleResult(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs, const char* sDefault);
static inline char* XS_XtpClientCallSimpleError(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs, const char* sDefault);
static inline char* XS_XtpClientCallSimpleMeta(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs);
static inline char* XS_XtpClientCallSimpleMetaJson(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs);
static inline char* XS_XtpClientCallSimpleResultJson(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs);
static inline char* XS_XtpClientCallSimpleErrorJson(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs, const char* sDefault);
static inline int32 XS_XtpClientCallSimpleStatus(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs, int32 iDefault);
static inline char* XS_XtpClientCallSimpleCmd(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs, const char* sDefault);
static inline xvalue XS_XtpClientCallSimpleValue(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs);
static inline xvalue XS_XtpClientCallSimpleParamsValue(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs);
static inline xvalue XS_XtpBodyValue(const void* pMsg);
static inline xvalue XS_XtpErrorValue(const void* pMsg);
static inline xvalue XS_XtpClientCallSimpleBodyValue(const char* sHost, uint16 iPort, uint32 iRecvLimit, uint32 iConnectTimeoutMs, uint64 iMsgID, const char* sCmd, uint32 iTimeoutMs);
static inline void XS_XtpMessageDestroy(XTP_Message* pMsg);
static inline int XS_XtpClientLastErrorCode(void);
static inline const char* XS_XtpClientLastError(void);

static inline const char* XS_ScriptServerName(ptr objServer)
{
	XS_ServerConfig* objCfg = (XS_ServerConfig*)objServer;
	
	if ( objCfg == NULL || objCfg->Name == NULL || objCfg->Name[0] == '\0' ) {
		return "(server)";
	}
	
	return objCfg->Name;
}

static inline int XS_ScriptServerClass(ptr objServer)
{
	XS_ServerConfig* objCfg = (XS_ServerConfig*)objServer;
	
	if ( objCfg == NULL ) {
		return XS_SVC_NONE;
	}
	
	return objCfg->Class;
}

static inline int XS_ScriptServerDebug(ptr objServer)
{
	XS_ServerConfig* objCfg = (XS_ServerConfig*)objServer;
	return (objCfg && objCfg->Debug) ? 1 : 0;
}

static inline const char* XS_ScriptServerAddr(ptr objServer)
{
	XS_ServerConfig* objCfg = (XS_ServerConfig*)objServer;
	
	if ( objCfg == NULL || objCfg->Addr == NULL ) {
		return "";
	}
	
	return objCfg->Addr;
}

static inline const char* XS_ScriptServerParam(ptr objServer)
{
	XS_ServerConfig* objCfg = (XS_ServerConfig*)objServer;
	
	if ( objCfg == NULL || objCfg->Param == NULL ) {
		return "";
	}
	
	return objCfg->Param;
}

static inline const char* XS_ScriptAppPath(void)
{
	return xCore.AppPath ? (const char*)xCore.AppPath : "";
}

static inline const char* XS_ScriptHostName(ptr objHost)
{
	XS_HostConfig* objCfg = (XS_HostConfig*)objHost;
	
	if ( objCfg == NULL || objCfg->Name == NULL || objCfg->Name[0] == '\0' ) {
		return "(host)";
	}
	
	return objCfg->Name;
}

static inline const char* XS_ScriptHostParam(ptr objHost)
{
	XS_HostConfig* objCfg = (XS_HostConfig*)objHost;
	
	if ( objCfg == NULL || objCfg->Param == NULL ) {
		return "";
	}
	
	return objCfg->Param;
}

static inline const char* XS_ScriptHostPath(ptr objHost)
{
	XS_HostConfig* objCfg = (XS_HostConfig*)objHost;
	
	if ( objCfg == NULL || objCfg->Path == NULL || objCfg->Path[0] == '\0' ) {
		return "";
	}
	
	return objCfg->Path;
}

static inline const char* XS_ScriptHostDevFile(ptr objHost)
{
	XS_HostConfig* objCfg = (XS_HostConfig*)objHost;
	
	if ( objCfg == NULL || objCfg->DevFile == NULL ) {
		return "";
	}
	
	return objCfg->DevFile;
}

static inline int XS_ScriptHostDebug(ptr objHost)
{
	XS_HostConfig* objCfg = (XS_HostConfig*)objHost;
	return (objCfg && objCfg->Debug) ? 1 : 0;
}

static inline int XS_ScriptHostDevMode(ptr objHost)
{
	XS_HostConfig* objCfg = (XS_HostConfig*)objHost;
	
	if ( objCfg == NULL ) {
		return XS_DEV_STATIC;
	}
	
	return objCfg->DevMode;
}

static inline const char* XS_ScriptRequestMethod(const void* pReq)
{
	const xhttpdrequest* pHttpReq = (const xhttpdrequest*)pReq;
	
	if ( pHttpReq == NULL ) {
		return "";
	}
	
	return pHttpReq->sMethod;
}

static inline const char* XS_ScriptRequestTarget(const void* pReq)
{
	const xhttpdrequest* pHttpReq = (const xhttpdrequest*)pReq;
	
	if ( pHttpReq == NULL ) {
		return "";
	}
	
	return pHttpReq->sTarget;
}

static inline const char* XS_ScriptRequestPath(const void* pReq)
{
	const xhttpdrequest* pHttpReq = (const xhttpdrequest*)pReq;
	
	if ( pHttpReq == NULL ) {
		return "";
	}
	
	return pHttpReq->sPath;
}

static inline const char* XS_ScriptRequestQuery(const void* pReq)
{
	const xhttpdrequest* pHttpReq = (const xhttpdrequest*)pReq;
	
	if ( pHttpReq == NULL ) {
		return "";
	}
	
	return pHttpReq->sQuery;
}

static inline const void* XS_ScriptRequestBody(const void* pReq)
{
	const xhttpdrequest* pHttpReq = (const xhttpdrequest*)pReq;
	
	if ( pHttpReq == NULL ) {
		return NULL;
	}
	
	return pHttpReq->pBody;
}

static inline size_t XS_ScriptRequestBodyLen(const void* pReq)
{
	const xhttpdrequest* pHttpReq = (const xhttpdrequest*)pReq;
	
	if ( pHttpReq == NULL ) {
		return 0;
	}
	
	return pHttpReq->iBodyLen;
}

static inline const char* XS_ScriptRequestHeader(const void* pReq, const char* sName)
{
	return xrtHttpdRequestHeader((const xhttpdrequest*)pReq, sName);
}

static inline int XS_ScriptHttpStatus(void* pResp, uint32 iStatus, const char* sReason)
{
	xhttpdresponse* pHttpResp = (xhttpdresponse*)pResp;
	
	if ( pHttpResp == NULL ) {
		return 0;
	}
	
	xrtHttpdResponseSetStatus(pHttpResp, iStatus, sReason);
	return 1;
}

static inline int XS_ScriptHttpHeader(void* pResp, const char* sName, const char* sValue)
{
	xhttpdresponse* pHttpResp = (xhttpdresponse*)pResp;
	
	if ( pHttpResp == NULL ) {
		return 0;
	}
	
	return xrtHttpdResponseSetHeader(pHttpResp, sName, sValue) ? 1 : 0;
}

static inline int XS_ScriptHttpText(void* pResp, uint32 iStatus, const char* sReason, const char* sText)
{
	xhttpdresponse* pHttpResp = (xhttpdresponse*)pResp;
	
	if ( pHttpResp == NULL ) {
		return 0;
	}
	
	xrtHttpdResponseSetStatus(pHttpResp, iStatus, sReason);
	return xrtHttpdResponseSetBodyCopy(
		pHttpResp,
		sText ? sText : "",
		sText ? strlen(sText) : 0,
		"text/plain; charset=utf-8"
	) ? 1 : 0;
}

static inline int XS_ScriptHttpBody(void* pResp, const void* pData, size_t iLen, const char* sContentType)
{
	xhttpdresponse* pHttpResp = (xhttpdresponse*)pResp;
	
	if ( pHttpResp == NULL ) {
		return 0;
	}
	
	return xrtHttpdResponseSetBodyCopy(pHttpResp, pData, iLen, sContentType) ? 1 : 0;
}

static inline int XS_ScriptHttpJson(void* pResp, uint32 iStatus, const char* sReason, const char* sJson)
{
	xhttpdresponse* pHttpResp = (xhttpdresponse*)pResp;
	
	if ( pHttpResp == NULL ) {
		return 0;
	}
	
	xrtHttpdResponseSetStatus(pHttpResp, iStatus, sReason);
	return xrtHttpdResponseSetBodyCopy(
		pHttpResp,
		sJson ? sJson : "{}",
		sJson ? strlen(sJson) : 2,
		"application/json; charset=utf-8"
	) ? 1 : 0;
}

static inline int XS_ScriptWsIsOpen(void* pConn)
{
	return xrtWsConnIsOpen((xwsconn*)pConn) ? 1 : 0;
}

static inline int XS_ScriptWsSendText(void* pConn, const char* sText, size_t iLen)
{
	return xrtWsConnSendText((xwsconn*)pConn, sText, iLen) == XRT_NET_OK ? 1 : 0;
}

static inline int XS_ScriptWsSendBinary(void* pConn, const void* pData, size_t iLen)
{
	return xrtWsConnSendBinary((xwsconn*)pConn, pData, iLen) == XRT_NET_OK ? 1 : 0;
}

static inline const char* XS_ScriptWsProtocol(void* pConn)
{
	xwsconn* pWsConn = (xwsconn*)pConn;

	if ( pWsConn == NULL ) {
		return "";
	}

	return pWsConn->sProtocol;
}

static inline int XS_ScriptWsPing(void* pConn, const void* pData, size_t iLen)
{
	return xrtWsConnPing((xwsconn*)pConn, pData, iLen) == XRT_NET_OK ? 1 : 0;
}

static inline int XS_ScriptWsClose(void* pConn, uint16 iCode, const char* sReason)
{
	return xrtWsConnClose((xwsconn*)pConn, iCode, sReason) == XRT_NET_OK ? 1 : 0;
}

static inline int XS_ScriptStreamSend(void* pStream, const void* pData, size_t iLen)
{
	int iRet;

	iRet = xrtNetStreamSend((xnetstream*)pStream, pData, iLen) == XRT_NET_OK ? 1 : 0;
	if ( iRet ) {
		g_iXsCustomSendCount++;
		g_iXsCustomSendBytes += (int64)iLen;
	}
	return iRet;
}

static inline int XS_ScriptStreamClose(void* pStream, unsigned iFlags)
{
	xrtNetStreamClose((xnetstream*)pStream, iFlags);
	return 1;
}

static inline int XS_ScriptXtpSend(
	void* pStream,
	const char* sCmd,
	size_t iCmdSize,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpSend(pStream, sCmd, iCmdSize, iParamCount, arrParam, arrValue, pBody, iBodySize);
}

static inline int XS_ScriptXtpSendRequest(
	void* pStream,
	uint64 iMsgID,
	const char* sCmd,
	size_t iCmdSize,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpSendRequest(pStream, iMsgID, sCmd, iCmdSize, iParamCount, arrParam, arrValue, pBody, iBodySize);
}

static inline int XS_ScriptXtpSendPush(
	void* pStream,
	const char* sCmd,
	size_t iCmdSize,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpSendPush(pStream, sCmd, iCmdSize, iParamCount, arrParam, arrValue, pBody, iBodySize);
}

static inline int XS_ScriptXtpSendEvent(
	void* pStream,
	const char* sCmd,
	size_t iCmdSize,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpSendEvent(pStream, sCmd, iCmdSize, iParamCount, arrParam, arrValue, pBody, iBodySize);
}

static inline int XS_ScriptXtpSendEx(
	void* pStream,
	unsigned iMsgType,
	uint64 iMsgID,
	unsigned iFlags,
	int iStatus,
	const char* sCmd,
	size_t iCmdSize,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpSendEx(
		pStream,
		(uint16)iMsgType,
		iMsgID,
		(uint16)iFlags,
		(int32)iStatus,
		sCmd,
		iCmdSize,
		iParamCount,
		arrParam,
		arrValue,
		pBody,
		iBodySize
	);
}

static inline const char* XS_ScriptXtpGetParam(const void* pMsg, const char* sKey)
{
	const char* pVal = NULL;
	uint16 iValLen = 0;
	static _Thread_local char sBuf[1024];
	
	if ( !XS_XtpFindParamView((void*)pMsg, sKey, &pVal, &iValLen) ) {
		return NULL;
	}
	if ( iValLen >= sizeof(sBuf) ) {
		iValLen = (uint16)(sizeof(sBuf) - 1);
	}
	memcpy(sBuf, pVal, iValLen);
	sBuf[iValLen] = '\0';
	return sBuf;
}

static inline int XS_ScriptXtpReply(
	void* pStream,
	const void* pReqMsg,
	const char* sCmd,
	size_t iCmdSize,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpReply(
		pStream,
		pReqMsg,
		sCmd,
		iCmdSize,
		iParamCount,
		arrParam,
		arrValue,
		pBody,
		iBodySize
	);
}

static inline int XS_ScriptXtpReplyEx(
	void* pStream,
	const void* pReqMsg,
	int iStatus,
	const char* sCmd,
	size_t iCmdSize,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize
)
{
	return XS_XtpReplyEx(
		pStream,
		pReqMsg,
		(int32)iStatus,
		sCmd,
		iCmdSize,
		iParamCount,
		arrParam,
		arrValue,
		pBody,
		iBodySize
	);
}

static inline uint64 XS_ScriptXtpMsgId(const void* pMsg)
{
	return XS_XtpMsgId(pMsg);
}

static inline unsigned XS_ScriptXtpMsgType(const void* pMsg)
{
	return XS_XtpMsgType(pMsg);
}

static inline unsigned XS_ScriptXtpMsgFlags(const void* pMsg)
{
	return XS_XtpMsgFlags(pMsg);
}

static inline int XS_ScriptXtpIsOK(const void* pMsg)
{
	return XS_XtpIsOK(pMsg) ? 1 : 0;
}

static inline int XS_ScriptXtpStatus(const void* pMsg)
{
	return XS_XtpStatus(pMsg);
}

static inline const char* XS_ScriptXtpCmd(const void* pMsg)
{
	return XS_XtpCmd(pMsg);
}

static inline unsigned XS_ScriptXtpCmdLen(const void* pMsg)
{
	return XS_XtpCmdLen(pMsg);
}

static inline const void* XS_ScriptXtpBody(const void* pMsg)
{
	return XS_XtpBody(pMsg);
}

static inline unsigned XS_ScriptXtpBodyLen(const void* pMsg)
{
	return XS_XtpBodyLen(pMsg);
}

static inline char* XS_ScriptXtpBodyDup(const void* pMsg, const char* sDefault)
{
	return XS_XtpBodyDup(pMsg, sDefault);
}

static inline char* XS_ScriptXtpCmdDup(const void* pMsg, const char* sDefault)
{
	return XS_XtpCmdDup(pMsg, sDefault);
}

static inline xvalue XS_ScriptXtpValue(const void* pMsg)
{
	return XS_XtpValue(pMsg);
}

static inline xvalue XS_ScriptXtpParamsValue(const void* pMsg)
{
	return XS_XtpParamsValue(pMsg);
}

static inline xvalue XS_ScriptXtpBodyValue(const void* pMsg)
{
	return XS_XtpBodyValue(pMsg);
}

static inline xvalue XS_ScriptXtpErrorValue(const void* pMsg)
{
	return XS_XtpErrorValue(pMsg);
}

static inline char* XS_ScriptXtpMetaText(const void* pMsg)
{
	return XS_XtpMetaText(pMsg);
}

static inline char* XS_ScriptXtpMetaJson(const void* pMsg)
{
	return XS_XtpMetaJson(pMsg);
}

static inline char* XS_ScriptXtpResultJson(const void* pMsg)
{
	return XS_XtpResultJson(pMsg);
}

static inline char* XS_ScriptXtpErrorJson(const void* pMsg, const char* sDefault)
{
	return XS_XtpErrorJson(pMsg, sDefault);
}

static inline char* XS_ScriptXtpSummaryText(const void* pMsg)
{
	return XS_XtpSummaryText(pMsg);
}

static inline char* XS_ScriptXtpSummaryJson(const void* pMsg)
{
	return XS_XtpSummaryJson(pMsg);
}

static inline unsigned XS_ScriptXtpParamCount(const void* pMsg)
{
	return XS_XtpParamCount(pMsg);
}

static inline int XS_ScriptXtpNeedReply(const void* pMsg)
{
	return XS_XtpNeedReply(pMsg) ? 1 : 0;
}

static inline int XS_ScriptXtpIsRequest(const void* pMsg)
{
	return XS_XtpIsRequest(pMsg) ? 1 : 0;
}

static inline int XS_ScriptXtpIsResponse(const void* pMsg)
{
	return XS_XtpIsResponse(pMsg) ? 1 : 0;
}

static inline int XS_ScriptXtpIsPush(const void* pMsg)
{
	return XS_XtpIsPush(pMsg) ? 1 : 0;
}

static inline int XS_ScriptXtpIsEvent(const void* pMsg)
{
	return XS_XtpIsEvent(pMsg) ? 1 : 0;
}

static inline int XS_ScriptXtpCmdIs(const void* pMsg, const char* sCmd)
{
	return XS_XtpCmdIs(pMsg, sCmd) ? 1 : 0;
}

static inline int XS_ScriptXtpHasParam(const void* pMsg, const char* sKey)
{
	return XS_XtpHasParam(pMsg, sKey) ? 1 : 0;
}

static inline const char* XS_ScriptXtpParamText(const void* pMsg, const char* sKey, const char* sDefault)
{
	return XS_XtpParamText(pMsg, sKey, sDefault);
}

static inline const char* XS_ScriptXtpResultText(const void* pMsg, const char* sDefault)
{
	return XS_XtpResultText(pMsg, sDefault);
}

static inline int XS_ScriptXtpResultIs(const void* pMsg, const char* sResult)
{
	return XS_XtpResultIs(pMsg, sResult) ? 1 : 0;
}

static inline int XS_ScriptXtpStatusIs(const void* pMsg, int iStatus)
{
	return XS_XtpStatusIs(pMsg, (int32)iStatus) ? 1 : 0;
}

static inline char* XS_ScriptXtpErrorText(const void* pMsg, const char* sDefault)
{
	return XS_XtpErrorText(pMsg, sDefault);
}

static inline char* XS_ScriptXtpParamDup(const void* pMsg, const char* sKey, const char* sDefault)
{
	return XS_XtpParamDup(pMsg, sKey, sDefault);
}

static inline int64 XS_ScriptXtpParamInt(const void* pMsg, const char* sKey, int64 iDefault)
{
	return XS_XtpParamInt(pMsg, sKey, iDefault);
}

static inline int XS_ScriptXtpParamBool(const void* pMsg, const char* sKey, int bDefault)
{
	return XS_XtpParamBool(pMsg, sKey, bDefault ? TRUE : FALSE) ? 1 : 0;
}

static inline int XS_ScriptXtpReplyText(
	void* pStream,
	const void* pReqMsg,
	int iStatus,
	const char* sCmd,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sText
)
{
	return XS_XtpReplyText(
		pStream,
		pReqMsg,
		(int32)iStatus,
		sCmd,
		iParamCount,
		arrParam,
		arrValue,
		sText
	);
}

static inline int XS_ScriptXtpReplyJson(
	void* pStream,
	const void* pReqMsg,
	int iStatus,
	const char* sCmd,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sJson
)
{
	return XS_XtpReplyJson(
		pStream,
		pReqMsg,
		(int32)iStatus,
		sCmd,
		iParamCount,
		arrParam,
		arrValue,
		sJson
	);
}

static inline int XS_ScriptXtpReplyOKText(
	void* pStream,
	const void* pReqMsg,
	const char* sCmd,
	const char* sText
)
{
	return XS_XtpReplyOKText(pStream, pReqMsg, sCmd, sText);
}

static inline int XS_ScriptXtpReplyErrorText(
	void* pStream,
	const void* pReqMsg,
	int iStatus,
	const char* sCmd,
	const char* sText
)
{
	return XS_XtpReplyErrorText(pStream, pReqMsg, (int32)iStatus, sCmd, sText);
}

static inline int XS_ScriptXtpReplyOKJson(
	void* pStream,
	const void* pReqMsg,
	const char* sCmd,
	const char* sJson
)
{
	return XS_XtpReplyOKJson(pStream, pReqMsg, sCmd, sJson);
}

static inline int XS_ScriptXtpReplyErrorJson(
	void* pStream,
	const void* pReqMsg,
	int iStatus,
	const char* sCmd,
	const char* sJson
)
{
	return XS_XtpReplyErrorJson(pStream, pReqMsg, (int32)iStatus, sCmd, sJson);
}

static inline int XS_ScriptXtpReplyMissingParam(
	void* pStream,
	const void* pReqMsg,
	const char* sParam
)
{
	return XS_XtpReplyMissingParam(pStream, pReqMsg, sParam);
}

static inline int XS_ScriptXtpReplyUnsupportedCmd(
	void* pStream,
	const void* pReqMsg,
	const char* sCmd
)
{
	return XS_XtpReplyUnsupportedCmd(pStream, pReqMsg, sCmd);
}

static inline void* XS_ScriptXtpClientOpen(const char* sHost, unsigned iPort, unsigned iRecvLimit, unsigned iTimeoutMs)
{
	return XS_XtpClientOpen(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iTimeoutMs);
}

static inline void XS_ScriptXtpClientClose(void* pClient)
{
	XS_XtpClientClose(pClient);
}

static inline void* XS_ScriptXtpClientDo(
	void* pClient,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientDo(
		pClient,
		iMsgID,
		sCmd,
		(uint32)iParamCount,
		arrParam,
		arrValue,
		pBody,
		iBodySize,
		(uint32)iTimeoutMs
	);
}

static inline void* XS_ScriptXtpClientDoText(
	void* pClient,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sText,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientDoText(
		pClient,
		iMsgID,
		sCmd,
		(uint32)iParamCount,
		arrParam,
		arrValue,
		sText,
		(uint32)iTimeoutMs
	);
}

static inline void* XS_ScriptXtpClientDoSimple(
	void* pClient,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientDoSimple(pClient, iMsgID, sCmd, (uint32)iTimeoutMs);
}

static inline void* XS_ScriptXtpClientDoJson(
	void* pClient,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sJson,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientDoJson(pClient, iMsgID, sCmd, (uint32)iParamCount, arrParam, arrValue, sJson, (uint32)iTimeoutMs);
}

static inline void* XS_ScriptXtpClientCall(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const void* pBody,
	size_t iBodySize,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCall(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iParamCount, arrParam, arrValue, pBody, iBodySize, (uint32)iTimeoutMs);
}

static inline void* XS_ScriptXtpClientCallText(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sText,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCallText(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iParamCount, arrParam, arrValue, sText, (uint32)iTimeoutMs);
}

static inline void* XS_ScriptXtpClientCallSimple(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCallSimple(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs);
}

static inline void* XS_ScriptXtpClientCallJson(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sJson,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCallJson(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iParamCount, arrParam, arrValue, sJson, (uint32)iTimeoutMs);
}

static inline char* XS_ScriptXtpClientCallSimpleBody(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	return XS_XtpClientCallSimpleBody(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs, sDefault);
}

static inline char* XS_ScriptXtpClientCallTextBody(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sText,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	return XS_XtpClientCallTextBody(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iParamCount, arrParam, arrValue, sText, (uint32)iTimeoutMs, sDefault);
}

static inline char* XS_ScriptXtpClientCallJsonBody(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iParamCount,
	const char** arrParam,
	const char** arrValue,
	const char* sJson,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	return XS_XtpClientCallJsonBody(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iParamCount, arrParam, arrValue, sJson, (uint32)iTimeoutMs, sDefault);
}

static inline char* XS_ScriptXtpClientCallSimpleSummary(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCallSimpleSummary(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs);
}

static inline char* XS_ScriptXtpClientCallSimpleSummaryJson(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCallSimpleSummaryJson(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs);
}

static inline char* XS_ScriptXtpClientCallSimpleResult(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	return XS_XtpClientCallSimpleResult(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs, sDefault);
}

static inline char* XS_ScriptXtpClientCallSimpleError(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	return XS_XtpClientCallSimpleError(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs, sDefault);
}

static inline char* XS_ScriptXtpClientCallSimpleMeta(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCallSimpleMeta(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs);
}

static inline char* XS_ScriptXtpClientCallSimpleMetaJson(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCallSimpleMetaJson(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs);
}

static inline char* XS_ScriptXtpClientCallSimpleResultJson(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCallSimpleResultJson(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs);
}

static inline char* XS_ScriptXtpClientCallSimpleErrorJson(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	return XS_XtpClientCallSimpleErrorJson(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs, sDefault);
}

static inline int XS_ScriptXtpClientCallSimpleStatus(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs,
	int iDefault
)
{
	return (int)XS_XtpClientCallSimpleStatus(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs, (int32)iDefault);
}

static inline char* XS_ScriptXtpClientCallSimpleCmd(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	return XS_XtpClientCallSimpleCmd(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs, sDefault);
}

static inline xvalue XS_ScriptXtpClientCallSimpleValue(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCallSimpleValue(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs);
}

static inline xvalue XS_ScriptXtpClientCallSimpleParamsValue(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCallSimpleParamsValue(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs);
}

static inline xvalue XS_ScriptXtpClientCallSimpleBodyValue(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	unsigned iTimeoutMs
)
{
	return XS_XtpClientCallSimpleBodyValue(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, (uint32)iTimeoutMs);
}

static inline void XS_ScriptXtpFreeParamArrays(char** arrParam, char** arrValue, unsigned iCount)
{
	unsigned i;

	if ( arrParam ) {
		for ( i = 0; i < iCount; i++ ) {
			if ( arrParam[i] ) {
				xrtFree(arrParam[i]);
			}
		}
		xrtFree(arrParam);
	}

	if ( arrValue ) {
		for ( i = 0; i < iCount; i++ ) {
			if ( arrValue[i] ) {
				xrtFree(arrValue[i]);
			}
		}
		xrtFree(arrValue);
	}
}

static inline int XS_ScriptXtpBuildParamArrays(xvalue objParams, const char*** ppParam, const char*** ppValue, unsigned* piCount)
{
	xdict objDict;
	char** arrParam;
	char** arrValue;
	unsigned iCount;
	unsigned iIndex;

	if ( ppParam ) {
		*ppParam = NULL;
	}
	if ( ppValue ) {
		*ppValue = NULL;
	}
	if ( piCount ) {
		*piCount = 0;
	}

	if ( objParams == NULL ) {
		return 1;
	}

	objDict = xvoGetTable(objParams);
	if ( objDict == NULL ) {
		return 0;
	}

	iCount = xvoTableItemCount(objParams);
	if ( iCount == 0u ) {
		return 1;
	}

	arrParam = (char**)xrtCalloc(iCount, sizeof(char*));
	arrValue = (char**)xrtCalloc(iCount, sizeof(char*));
	if ( arrParam == NULL || arrValue == NULL ) {
		XS_ScriptXtpFreeParamArrays(arrParam, arrValue, iCount);
		return 0;
	}

	iIndex = 0;
	DICT_FOREACH_TYPE(objDict, objKey, ppItem, xvalue*) {
		xvalue objItem = ppItem ? *ppItem : NULL;

		if ( iIndex >= iCount ) {
			break;
		}

		arrParam[iIndex] = (char*)xrtCopyStr((str)(objKey ? objKey->Key : ""), objKey ? objKey->KeyLen : 0u);
		if ( objItem == NULL ) {
			arrValue[iIndex] = (char*)xrtCopyStr((str)"", 0);
		} else if ( xvoType(objItem) == XVO_DT_TEXT ) {
			arrValue[iIndex] = (char*)xrtCopyStr((str)xvoGetText(objItem), 0);
		} else if ( xvoType(objItem) == XVO_DT_BOOL ) {
			arrValue[iIndex] = (char*)xrtCopyStr((str)(xvoGetBool(objItem) ? "true" : "false"), 0);
		} else if ( xvoType(objItem) == XVO_DT_INT ) {
			arrValue[iIndex] = xrtFormat("%lld", (long long)xvoGetInt(objItem));
		} else {
			arrValue[iIndex] = xrtStringifyJSON(objItem, FALSE, NULL);
		}

		if ( arrParam[iIndex] == NULL || arrValue[iIndex] == NULL ) {
			XS_ScriptXtpFreeParamArrays(arrParam, arrValue, iCount);
			return 0;
		}

		iIndex++;
	}

	if ( ppParam ) {
		*ppParam = (const char**)arrParam;
	}
	if ( ppValue ) {
		*ppValue = (const char**)arrValue;
	}
	if ( piCount ) {
		*piCount = iIndex;
	}
	return 1;
}

static inline void* XS_ScriptXtpClientCallTableText(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	xvalue objParams,
	const char* sText,
	unsigned iTimeoutMs
)
{
	const char** arrParam = NULL;
	const char** arrValue = NULL;
	unsigned iCount = 0;
	void* pRet;

	if ( !XS_ScriptXtpBuildParamArrays(objParams, &arrParam, &arrValue, &iCount) ) {
		return NULL;
	}

	pRet = XS_XtpClientCallText(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, iCount, arrParam, arrValue, sText, (uint32)iTimeoutMs);
	XS_ScriptXtpFreeParamArrays((char**)arrParam, (char**)arrValue, iCount);
	return pRet;
}

static inline void* XS_ScriptXtpClientCallTableJson(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	xvalue objParams,
	const char* sJson,
	unsigned iTimeoutMs
)
{
	const char** arrParam = NULL;
	const char** arrValue = NULL;
	unsigned iCount = 0;
	void* pRet;

	if ( !XS_ScriptXtpBuildParamArrays(objParams, &arrParam, &arrValue, &iCount) ) {
		return NULL;
	}

	pRet = XS_XtpClientCallJson(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, sCmd, iCount, arrParam, arrValue, sJson, (uint32)iTimeoutMs);
	XS_ScriptXtpFreeParamArrays((char**)arrParam, (char**)arrValue, iCount);
	return pRet;
}

static inline void* XS_ScriptXtpClientCallTableValue(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	const char* sCmd,
	xvalue objParams,
	xvalue objBody,
	unsigned iTimeoutMs
)
{
	char* sJson;
	void* pRet;

	sJson = objBody ? xrtStringifyJSON(objBody, FALSE, NULL) : NULL;
	pRet = XS_ScriptXtpClientCallTableJson(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, sCmd, objParams, sJson ? sJson : "", iTimeoutMs);
	if ( sJson ) {
		xrtFree(sJson);
	}
	return pRet;
}


typedef struct {
	char* sCmd;
	xvalue objParams;
	char* sText;
	char* sJson;
	xvalue objBody;
} XS_ScriptXtpRequest;

static inline char* XS_ScriptDupText(const char* sText)
{
	size_t iLen;
	char* sDup;

	if ( sText == NULL ) {
		return NULL;
	}

	iLen = strlen(sText);
	sDup = (char*)xrtMalloc(iLen + 1u);
	if ( sDup == NULL ) {
		return NULL;
	}

	memcpy(sDup, sText, iLen + 1u);
	return sDup;
}

static inline void XS_ScriptXtpRequestClearBody(XS_ScriptXtpRequest* pReq)
{
	if ( pReq == NULL ) {
		return;
	}

	if ( pReq->sText ) {
		xrtFree(pReq->sText);
		pReq->sText = NULL;
	}
	if ( pReq->sJson ) {
		xrtFree(pReq->sJson);
		pReq->sJson = NULL;
	}
	if ( pReq->objBody ) {
		xvoUnref(pReq->objBody);
		pReq->objBody = NULL;
	}
}

static inline xvalue XS_ScriptXtpRequestEnsureParams(XS_ScriptXtpRequest* pReq)
{
	if ( pReq == NULL ) {
		return NULL;
	}

	if ( pReq->objParams == NULL ) {
		pReq->objParams = xvoCreateTable();
	}

	return pReq->objParams;
}

static inline void* XS_ScriptXtpRequestCreate(const char* sCmd)
{
	XS_ScriptXtpRequest* pReq;

	pReq = (XS_ScriptXtpRequest*)xrtMalloc(sizeof(XS_ScriptXtpRequest));
	if ( pReq == NULL ) {
		return NULL;
	}

	memset(pReq, 0, sizeof(XS_ScriptXtpRequest));
	if ( sCmd && sCmd[0] ) {
		pReq->sCmd = XS_ScriptDupText(sCmd);
		if ( pReq->sCmd == NULL ) {
			xrtFree(pReq);
			return NULL;
		}
	}

	return pReq;
}

static inline void XS_ScriptXtpRequestFree(void* pReqObj)
{
	XS_ScriptXtpRequest* pReq = (XS_ScriptXtpRequest*)pReqObj;

	if ( pReq == NULL ) {
		return;
	}

	if ( pReq->sCmd ) {
		xrtFree(pReq->sCmd);
	}
	if ( pReq->objParams ) {
		xvoUnref(pReq->objParams);
	}
	XS_ScriptXtpRequestClearBody(pReq);
	xrtFree(pReq);
}

static inline int XS_ScriptXtpRequestSetCmd(void* pReqObj, const char* sCmd)
{
	XS_ScriptXtpRequest* pReq = (XS_ScriptXtpRequest*)pReqObj;
	char* sDup;

	if ( pReq == NULL ) {
		return 0;
	}

	sDup = ( sCmd && sCmd[0] ) ? XS_ScriptDupText(sCmd) : NULL;
	if ( sCmd && sCmd[0] && (sDup == NULL) ) {
		return 0;
	}

	if ( pReq->sCmd ) {
		xrtFree(pReq->sCmd);
	}
	pReq->sCmd = sDup;
	return 1;
}

static inline int XS_ScriptXtpRequestSetParamsValue(void* pReqObj, xvalue objParams)
{
	XS_ScriptXtpRequest* pReq = (XS_ScriptXtpRequest*)pReqObj;

	if ( pReq == NULL ) {
		return 0;
	}

	if ( pReq->objParams ) {
		xvoUnref(pReq->objParams);
		pReq->objParams = NULL;
	}
	if ( objParams ) {
		xvoAddRef(objParams);
		pReq->objParams = objParams;
	}

	return 1;
}

static inline int XS_ScriptXtpRequestSetParamText(void* pReqObj, const char* sKey, const char* sValue)
{
	xvalue objParams;

	if ( sKey == NULL || sKey[0] == '\0' ) {
		return 0;
	}

	objParams = XS_ScriptXtpRequestEnsureParams((XS_ScriptXtpRequest*)pReqObj);
	if ( objParams == NULL ) {
		return 0;
	}

	xvoTableSetText(objParams, (str)sKey, 0, (ptr)(sValue ? sValue : ""), 0, FALSE);
	return 1;
}

static inline int XS_ScriptXtpRequestSetParamInt(void* pReqObj, const char* sKey, int64 iValue)
{
	xvalue objParams;

	if ( sKey == NULL || sKey[0] == '\0' ) {
		return 0;
	}

	objParams = XS_ScriptXtpRequestEnsureParams((XS_ScriptXtpRequest*)pReqObj);
	if ( objParams == NULL ) {
		return 0;
	}

	xvoTableSetInt(objParams, (str)sKey, 0, iValue);
	return 1;
}

static inline int XS_ScriptXtpRequestSetParamBool(void* pReqObj, const char* sKey, int bValue)
{
	xvalue objParams;

	if ( sKey == NULL || sKey[0] == '\0' ) {
		return 0;
	}

	objParams = XS_ScriptXtpRequestEnsureParams((XS_ScriptXtpRequest*)pReqObj);
	if ( objParams == NULL ) {
		return 0;
	}

	xvoTableSetBool(objParams, (str)sKey, 0, bValue ? TRUE : FALSE);
	return 1;
}

static inline int XS_ScriptXtpRequestSetBodyText(void* pReqObj, const char* sText)
{
	XS_ScriptXtpRequest* pReq = (XS_ScriptXtpRequest*)pReqObj;

	if ( pReq == NULL ) {
		return 0;
	}

	XS_ScriptXtpRequestClearBody(pReq);
	if ( sText && sText[0] ) {
		pReq->sText = XS_ScriptDupText(sText);
		if ( pReq->sText == NULL ) {
			return 0;
		}
	}

	return 1;
}

static inline int XS_ScriptXtpRequestSetBodyJson(void* pReqObj, const char* sJson)
{
	XS_ScriptXtpRequest* pReq = (XS_ScriptXtpRequest*)pReqObj;

	if ( pReq == NULL ) {
		return 0;
	}

	XS_ScriptXtpRequestClearBody(pReq);
	if ( sJson && sJson[0] ) {
		pReq->sJson = XS_ScriptDupText(sJson);
		if ( pReq->sJson == NULL ) {
			return 0;
		}
	}

	return 1;
}

static inline int XS_ScriptXtpRequestSetBodyValue(void* pReqObj, xvalue objBody)
{
	XS_ScriptXtpRequest* pReq = (XS_ScriptXtpRequest*)pReqObj;

	if ( pReq == NULL ) {
		return 0;
	}

	XS_ScriptXtpRequestClearBody(pReq);
	if ( objBody ) {
		xvoAddRef(objBody);
		pReq->objBody = objBody;
	}

	return 1;
}

static inline void* XS_ScriptXtpClientDoRequest(void* pClient, uint64 iMsgID, void* pReqObj, unsigned iTimeoutMs)
{
	XS_ScriptXtpRequest* pReq = (XS_ScriptXtpRequest*)pReqObj;
	const char** arrParam = NULL;
	const char** arrValue = NULL;
	unsigned iCount = 0;
	void* pRet = NULL;
	char* sJson = NULL;

	if ( pClient == NULL || pReq == NULL || pReq->sCmd == NULL || pReq->sCmd[0] == '\0' ) {
		return NULL;
	}

	if ( !XS_ScriptXtpBuildParamArrays(pReq->objParams, &arrParam, &arrValue, &iCount) ) {
		return NULL;
	}

	if ( pReq->objBody ) {
		sJson = xrtStringifyJSON(pReq->objBody, FALSE, NULL);
		if ( sJson == NULL ) {
			XS_ScriptXtpFreeParamArrays((char**)arrParam, (char**)arrValue, iCount);
			return NULL;
		}
		pRet = XS_XtpClientDoJson(pClient, iMsgID, pReq->sCmd, iCount, arrParam, arrValue, sJson, (uint32)iTimeoutMs);
		xrtFree(sJson);
	} else if ( pReq->sJson ) {
		pRet = XS_XtpClientDoJson(pClient, iMsgID, pReq->sCmd, iCount, arrParam, arrValue, pReq->sJson, (uint32)iTimeoutMs);
	} else if ( pReq->sText ) {
		pRet = XS_XtpClientDoText(pClient, iMsgID, pReq->sCmd, iCount, arrParam, arrValue, pReq->sText, (uint32)iTimeoutMs);
	} else if ( iCount > 0 ) {
		pRet = XS_XtpClientDo(pClient, iMsgID, pReq->sCmd, iCount, arrParam, arrValue, NULL, 0, (uint32)iTimeoutMs);
	} else {
		pRet = XS_XtpClientDoSimple(pClient, iMsgID, pReq->sCmd, (uint32)iTimeoutMs);
	}

	XS_ScriptXtpFreeParamArrays((char**)arrParam, (char**)arrValue, iCount);
	return pRet;
}

static inline void* XS_ScriptXtpClientCallRequest(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs
)
{
	XS_ScriptXtpRequest* pReq = (XS_ScriptXtpRequest*)pReqObj;

	if ( pReq == NULL || pReq->sCmd == NULL || pReq->sCmd[0] == '\0' ) {
		return NULL;
	}

	if ( pReq->objBody ) {
		return XS_ScriptXtpClientCallTableValue(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReq->sCmd, pReq->objParams, pReq->objBody, iTimeoutMs);
	}
	if ( pReq->sJson ) {
		return XS_ScriptXtpClientCallTableJson(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReq->sCmd, pReq->objParams, pReq->sJson, iTimeoutMs);
	}
	if ( pReq->sText ) {
		return XS_ScriptXtpClientCallTableText(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReq->sCmd, pReq->objParams, pReq->sText, iTimeoutMs);
	}
	if ( pReq->objParams ) {
		return XS_ScriptXtpClientCallTableText(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReq->sCmd, pReq->objParams, "", iTimeoutMs);
	}

	return XS_XtpClientCallSimple(sHost, (uint16)iPort, (uint32)iRecvLimit, (uint32)iConnectTimeoutMs, iMsgID, pReq->sCmd, (uint32)iTimeoutMs);
}

static inline xvalue XS_ScriptXtpClientCallRequestValue(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs
)
{
	XTP_MessageObject objResp;
	xvalue objRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	objRet = XS_XtpValue(objResp);
	XS_XtpMessageDestroy(objResp);
	return objRet;
}

static inline xvalue XS_ScriptXtpClientCallRequestParamsValue(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs
)
{
	XTP_MessageObject objResp;
	xvalue objRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	objRet = XS_XtpParamsValue(objResp);
	XS_XtpMessageDestroy(objResp);
	return objRet;
}

static inline xvalue XS_ScriptXtpClientCallRequestBodyValue(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs
)
{
	XTP_MessageObject objResp;
	xvalue objRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	objRet = XS_XtpBodyValue(objResp);
	XS_XtpMessageDestroy(objResp);
	return objRet;
}

static inline char* XS_ScriptXtpClientCallRequestBody(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp;
	char* sRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	sRet = XS_XtpBodyDup(objResp, sDefault);
	XS_XtpMessageDestroy(objResp);
	return sRet;
}

static inline char* XS_ScriptXtpClientCallRequestSummary(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs
)
{
	XTP_MessageObject objResp;
	char* sRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	sRet = XS_XtpSummaryText(objResp);
	XS_XtpMessageDestroy(objResp);
	return sRet;
}

static inline char* XS_ScriptXtpClientCallRequestSummaryJson(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs
)
{
	XTP_MessageObject objResp;
	char* sRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	sRet = XS_XtpSummaryJson(objResp);
	XS_XtpMessageDestroy(objResp);
	return sRet;
}

static inline char* XS_ScriptXtpClientCallRequestResult(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp;
	char* sRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	sRet = XS_XtpParamDup(objResp, "result", sDefault);
	XS_XtpMessageDestroy(objResp);
	return sRet;
}

static inline char* XS_ScriptXtpClientCallRequestError(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp;
	char* sRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	sRet = XS_XtpErrorText(objResp, sDefault);
	XS_XtpMessageDestroy(objResp);
	return sRet;
}

static inline char* XS_ScriptXtpClientCallRequestMeta(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs
)
{
	XTP_MessageObject objResp;
	char* sRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	sRet = XS_XtpMetaText(objResp);
	XS_XtpMessageDestroy(objResp);
	return sRet;
}

static inline char* XS_ScriptXtpClientCallRequestMetaJson(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs
)
{
	XTP_MessageObject objResp;
	char* sRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	sRet = XS_XtpMetaJson(objResp);
	XS_XtpMessageDestroy(objResp);
	return sRet;
}

static inline char* XS_ScriptXtpClientCallRequestResultJson(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs
)
{
	XTP_MessageObject objResp;
	char* sRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	sRet = XS_XtpResultJson(objResp);
	XS_XtpMessageDestroy(objResp);
	return sRet;
}

static inline char* XS_ScriptXtpClientCallRequestErrorJson(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp;
	char* sRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	sRet = XS_XtpErrorJson(objResp, sDefault);
	XS_XtpMessageDestroy(objResp);
	return sRet;
}

static inline int XS_ScriptXtpClientCallRequestOK(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs
)
{
	XTP_MessageObject objResp;
	int iRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return 0;
	}

	iRet = XS_XtpIsOK(objResp) ? 1 : 0;
	XS_XtpMessageDestroy(objResp);
	return iRet;
}

static inline int XS_ScriptXtpClientCallRequestStatus(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs,
	int iDefault
)
{
	XTP_MessageObject objResp;
	int iRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return iDefault;
	}

	iRet = (int)XS_XtpStatus(objResp);
	XS_XtpMessageDestroy(objResp);
	return iRet;
}

static inline char* XS_ScriptXtpClientCallRequestCmd(
	const char* sHost,
	unsigned iPort,
	unsigned iRecvLimit,
	unsigned iConnectTimeoutMs,
	uint64 iMsgID,
	void* pReqObj,
	unsigned iTimeoutMs,
	const char* sDefault
)
{
	XTP_MessageObject objResp;
	char* sRet;

	objResp = (XTP_MessageObject)XS_ScriptXtpClientCallRequest(sHost, iPort, iRecvLimit, iConnectTimeoutMs, iMsgID, pReqObj, iTimeoutMs);
	if ( objResp == NULL ) {
		return NULL;
	}

	sRet = XS_XtpCmdDup(objResp, sDefault);
	XS_XtpMessageDestroy(objResp);
	return sRet;
}


static inline void XS_ScriptXtpMessageFree(void* pMsg)
{
	XS_XtpMessageDestroy((XTP_Message*)pMsg);
}

static inline int XS_ScriptXtpClientLastErrorCode(void)
{
	return XS_XtpClientLastErrorCode();
}

static inline const char* XS_ScriptXtpClientLastError(void)
{
	return XS_XtpClientLastError();
}

static inline const char* XS_ScriptXtpParamKeyAt(const void* pMsg, unsigned iIndex, unsigned* piLen)
{
	const char* pKey = NULL;
	const char* pVal;
	uint16 iKeyLen = 0;
	uint16 iValLen;
	
	if ( piLen ) *piLen = 0;
	if ( !XS_XtpParamAt(pMsg, (uint16)iIndex, &pKey, &iKeyLen, &pVal, &iValLen) ) {
		return NULL;
	}
	if ( piLen ) *piLen = iKeyLen;
	(void)pVal;
	(void)iValLen;
	return pKey;
}

static inline const char* XS_ScriptXtpParamValueAt(const void* pMsg, unsigned iIndex, unsigned* piLen)
{
	const char* pKey;
	const char* pVal = NULL;
	uint16 iKeyLen;
	uint16 iValLen = 0;
	
	if ( piLen ) *piLen = 0;
	if ( !XS_XtpParamAt(pMsg, (uint16)iIndex, &pKey, &iKeyLen, &pVal, &iValLen) ) {
		return NULL;
	}
	if ( piLen ) *piLen = iValLen;
	(void)pKey;
	(void)iKeyLen;
	return pVal;
}

static inline int XS_ScriptXtpFindParamView(const void* pMsg, const char* sKey, const char** ppVal, unsigned* piLen)
{
	uint16 iValLen = 0;
	bool bRet;
	
	bRet = XS_XtpFindParamView(pMsg, sKey, ppVal, &iValLen);
	if ( piLen ) {
		*piLen = iValLen;
	}
	return bRet ? 1 : 0;
}

static inline int XS_ScriptDgramSendTo(void* pSock, const char* sIP, unsigned short iPort, const void* pData, size_t iLen)
{
	xnetaddr tAddr;
	int iRet;
	
	if ( pSock == NULL || sIP == NULL || sIP[0] == '\0' || iPort == 0 ) {
		return 0;
	}
	if ( xrtNetAddrParse(&tAddr, sIP, iPort) != XRT_NET_OK ) {
		return 0;
	}
	
	iRet = xrtNetDgramSendTo((xdgramsock*)pSock, &tAddr, pData, iLen) == XRT_NET_OK ? 1 : 0;
	if ( iRet ) {
		g_iXsUdpSendCount++;
		g_iXsUdpSendBytes += (int64)iLen;
	}
	return iRet;
}

static inline int XS_ScriptDgramReply(void* pSock, const void* pFromAddr, const void* pData, size_t iLen)
{
	int iRet;

	if ( pSock == NULL || pFromAddr == NULL ) {
		return 0;
	}
	
	iRet = xrtNetDgramSendTo((xdgramsock*)pSock, (const xnetaddr*)pFromAddr, pData, iLen) == XRT_NET_OK ? 1 : 0;
	if ( iRet ) {
		g_iXsUdpSendCount++;
		g_iXsUdpSendBytes += (int64)iLen;
	}
	return iRet;
}

static inline const char* XS_ScriptAddrText(const void* pAddr)
{
	if ( pAddr == NULL ) {
		return "";
	}
	
	return xrtNetAddrToStr((const xnetaddr*)pAddr);
}

static inline void XS_ScriptLog(const char* sText)
{
	XS_LogInfo("script: %s", sText ? sText : "");
}

static inline int XS_ScriptReloadCurrentHost(ptr objServer, ptr objHost, int bForce)
{
	return XS_ReloadServerHostScript((XS_ServerConfig*)objServer, (XS_HostConfig*)objHost, bForce ? TRUE : FALSE);
}

static inline int XS_ScriptReloadHostByName(ptr objServer, const char* sHostName, int bForce)
{
	return XS_ReloadServerHostScriptByName((XS_ServerConfig*)objServer, sHostName, bForce ? TRUE : FALSE);
}

static inline int64 XS_ScriptDataRegister(xvalue objValue)
{
	return XS_BusDataRegister(objValue);
}

static inline int64 XS_ScriptDataRegisterEx(xvalue objValue, const char* sNamespace, const char* sTag, int64 iTTL)
{
	return XS_BusDataRegisterEx(objValue, sNamespace, sTag, iTTL);
}

static inline xvalue XS_ScriptDataGet(int64 iID)
{
	return XS_BusDataGet(iID);
}

static inline int XS_ScriptDataRetain(int64 iID)
{
	return XS_BusDataRetain(iID) ? 1 : 0;
}

static inline int XS_ScriptDataRelease(int64 iID)
{
	return XS_BusDataRelease(iID) ? 1 : 0;
}

static inline int XS_ScriptDataRemove(int64 iID)
{
	return XS_BusDataRemove(iID) ? 1 : 0;
}

static inline int64 XS_ScriptDataRemoveByQuery(const char* sNamespace, const char* sTag, int iLimit)
{
	char* sNamespaceCopy = NULL;
	char* sTagCopy = NULL;
	int64 iRemoved;
	
	if ( sNamespace && sNamespace[0] != '\0' ) {
		sNamespaceCopy = (char*)xrtCopyStr((str)sNamespace, 0);
	}
	if ( sTag && sTag[0] != '\0' ) {
		sTagCopy = (char*)xrtCopyStr((str)sTag, 0);
	}
	
	iRemoved = XS_BusDataRemoveByQuery(sNamespaceCopy, sTagCopy, iLimit);
	
	if ( sNamespaceCopy ) {
		xrtFree(sNamespaceCopy);
	}
	if ( sTagCopy ) {
		xrtFree(sTagCopy);
	}
	
	return iRemoved;
}

static inline int64 XS_ScriptDataFindFirst(const char* sNamespace, const char* sTag)
{
	char* sNamespaceCopy = NULL;
	char* sTagCopy = NULL;
	int64 iDataID;
	
	if ( sNamespace && sNamespace[0] != '\0' ) {
		sNamespaceCopy = (char*)xrtCopyStr((str)sNamespace, 0);
	}
	if ( sTag && sTag[0] != '\0' ) {
		sTagCopy = (char*)xrtCopyStr((str)sTag, 0);
	}
	
	iDataID = XS_BusDataFindFirst(sNamespaceCopy, sTagCopy);
	
	if ( sNamespaceCopy ) {
		xrtFree(sNamespaceCopy);
	}
	if ( sTagCopy ) {
		xrtFree(sTagCopy);
	}
	
	return iDataID;
}

static inline char* XS_ScriptBusStatusJson(void)
{
	return XS_BusBuildStatusJson();
}

static inline char* XS_ScriptBusStatusJsonEx(const char* sNamespace, const char* sTag)
{
	char* sNamespaceCopy = NULL;
	char* sTagCopy = NULL;
	char* sRet;
	
	if ( sNamespace && sNamespace[0] != '\0' ) {
		sNamespaceCopy = (char*)xrtCopyStr((str)sNamespace, 0);
	}
	if ( sTag && sTag[0] != '\0' ) {
		sTagCopy = (char*)xrtCopyStr((str)sTag, 0);
	}
	
	sRet = XS_BusBuildStatusJsonEx(sNamespaceCopy, sTagCopy);
	
	if ( sNamespaceCopy ) {
		xrtFree(sNamespaceCopy);
	}
	if ( sTagCopy ) {
		xrtFree(sTagCopy);
	}
	
	return sRet;
}

static inline char* XS_ScriptBusNamespaceJson(void)
{
	return XS_BusBuildNamespaceStatsJson();
}

static inline int XS_ScriptBusLastErrorCode(void)
{
	return XS_BusGetLastErrorCode();
}

static inline const char* XS_ScriptBusLastError(void)
{
	return XS_BusGetLastError();
}

static inline int XS_ScriptMsgSendToHost(const char* sServer, const char* sHost, const char* sTopic, int64 iDataID, xvalue objArgs)
{
	return XS_BusSendToHost(sServer, sHost, sTopic, iDataID, objArgs) ? 1 : 0;
}

static inline int XS_ScriptMsgSendToServer(const char* sServer, const char* sTopic, int64 iDataID, xvalue objArgs)
{
	return XS_BusSendToServer(sServer, sTopic, iDataID, objArgs) ? 1 : 0;
}

static inline int XS_ScriptMsgBroadcast(const char* sTopic, int64 iDataID, xvalue objArgs)
{
	return XS_BusBroadcast(sTopic, iDataID, objArgs) ? 1 : 0;
}

#include "import_c/import_xs.h"

static inline void XS_ImportScriptHostAPI(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}
}

static inline void XS_ImportScriptAPI(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	XS_ImportThirdPartyAPI(s);
	XS_ImportXSSymbols(s);

	tcc_add_symbol(s, "XS_ImportScriptHostAPI", XS_ImportScriptHostAPI);
	tcc_add_symbol(s, "XS_ImportScriptAPI", XS_ImportScriptAPI);
}

#endif
