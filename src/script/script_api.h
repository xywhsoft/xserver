#ifndef XS_SCRIPT_SCRIPT_API_H
#define XS_SCRIPT_SCRIPT_API_H

#include "../../lib/sqlite3.h"

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
static inline int32 XS_XtpStatus(const void* pMsg);
static inline const char* XS_XtpCmd(const void* pMsg);
static inline uint16 XS_XtpCmdLen(const void* pMsg);
static inline const void* XS_XtpBody(const void* pMsg);
static inline uint32 XS_XtpBodyLen(const void* pMsg);
static inline uint16 XS_XtpParamCount(const void* pMsg);
static inline bool XS_XtpParamAt(const void* pMsg, uint16 iIndex, const char** ppKey, uint16* piKeyLen, const char** ppVal, uint16* piValLen);
static inline bool XS_XtpFindParamView(const void* pMsg, const char* sKey, const char** ppVal, uint16* piValLen);
static inline bool XS_XtpNeedReply(const void* pMsg);

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
	return xrtNetStreamSend((xnetstream*)pStream, pData, iLen) == XRT_NET_OK ? 1 : 0;
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

static inline unsigned XS_ScriptXtpParamCount(const void* pMsg)
{
	return XS_XtpParamCount(pMsg);
}

static inline int XS_ScriptXtpNeedReply(const void* pMsg)
{
	return XS_XtpNeedReply(pMsg) ? 1 : 0;
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
	
	if ( pSock == NULL || sIP == NULL || sIP[0] == '\0' || iPort == 0 ) {
		return 0;
	}
	if ( xrtNetAddrParse(&tAddr, sIP, iPort) != XRT_NET_OK ) {
		return 0;
	}
	
	return xrtNetDgramSendTo((xdgramsock*)pSock, &tAddr, pData, iLen) == XRT_NET_OK ? 1 : 0;
}

static inline int XS_ScriptDgramReply(void* pSock, const void* pFromAddr, const void* pData, size_t iLen)
{
	if ( pSock == NULL || pFromAddr == NULL ) {
		return 0;
	}
	
	return xrtNetDgramSendTo((xdgramsock*)pSock, (const xnetaddr*)pFromAddr, pData, iLen) == XRT_NET_OK ? 1 : 0;
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

static inline char* XS_ScriptFileReadAll(const char* sFile, int iCodePage, size_t* pRetSize)
{
	return (char*)xrtFileReadAll((str)sFile, iCodePage, pRetSize);
}

static inline xvalue XS_ScriptValueArrayGetValue(xvalue pArr, uint32 iIndex)
{
	return xvoArrayGetValue(pArr, iIndex);
}

static inline uint32 XS_ScriptValueArrayItemCount(xvalue pArr)
{
	return xvoArrayItemCount(pArr);
}

static inline XTE_LiteObject XS_ScriptTemplateParse(char* sText, size_t iSize, char* sBracket)
{
	return xteParse(sText, iSize, sBracket);
}

static inline void XS_ScriptTemplateParseFree(XTE_LiteObject objTemplate)
{
	xteParseFree(objTemplate);
}

static inline char* XS_ScriptTemplateMake(XTE_LiteObject objTemplate, xvalue tblVal, xvalue tblEnv, xdict tblInclude, size_t* pRetSize)
{
	return xteMake(objTemplate, tblVal, tblEnv, tblInclude, pRetSize);
}

static inline void XS_ImportScriptAPI(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}
	
	tcc_add_symbol(s, "printf", printf);
	tcc_add_symbol(s, "snprintf", snprintf);
	tcc_add_symbol(s, "xrtMalloc", xrtMalloc);
	tcc_add_symbol(s, "xrtCalloc", xrtCalloc);
	tcc_add_symbol(s, "xrtRealloc", xrtRealloc);
	tcc_add_symbol(s, "xrtFree", xrtFree);
	tcc_add_symbol(s, "xrtRandRange", xrtRandRange);
	tcc_add_symbol(s, "xrtCopyStr", xrtCopyStr);
	tcc_add_symbol(s, "xrtFormat", xrtFormat);
	tcc_add_symbol(s, "xrtPathJoin", xrtPathJoin);
	tcc_add_symbol(s, "xrtPathGetDir", xrtPathGetDir);
	tcc_add_symbol(s, "xrtFileExists", xrtFileExists);
	tcc_add_symbol(s, "xrtFileReadAll", XS_ScriptFileReadAll);
	tcc_add_symbol(s, "xrtFileGetAll", xrtFileGetAll);
	tcc_add_symbol(s, "xrtFileWriteAll", xrtFileWriteAll);
	tcc_add_symbol(s, "xrtPathExists", xrtPathExists);
	tcc_add_symbol(s, "xrtDirExists", xrtDirExists);
	tcc_add_symbol(s, "xrtDirCreate", xrtDirCreate);
	tcc_add_symbol(s, "xrtDirCreateAll", xrtDirCreateAll);
	tcc_add_symbol(s, "xrtNowStr", xrtNowStr);
	tcc_add_symbol(s, "xrtStringifyJSON", xrtStringifyJSON);
	tcc_add_symbol(s, "xrtParseJSON", xrtParseJSON);
	tcc_add_symbol(s, "xteParse", XS_ScriptTemplateParse);
	tcc_add_symbol(s, "xteParseFree", XS_ScriptTemplateParseFree);
	tcc_add_symbol(s, "xteMake", XS_ScriptTemplateMake);
	tcc_add_symbol(s, "xvoCreateNull", xvoCreateNull);
	tcc_add_symbol(s, "xvoCreateBool", xvoCreateBool);
	tcc_add_symbol(s, "xvoCreateInt", xvoCreateInt);
	tcc_add_symbol(s, "xvoCreateFloat", xvoCreateFloat);
	tcc_add_symbol(s, "xvoCreateText", xvoCreateText);
	tcc_add_symbol(s, "xvoCreateArray", xvoCreateArray);
	tcc_add_symbol(s, "xvoCreateTable", xvoCreateTable);
	tcc_add_symbol(s, "xvoUnref", xvoUnref);
	tcc_add_symbol(s, "xvoArrayItemCount", XS_ScriptValueArrayItemCount);
	tcc_add_symbol(s, "xvoArrayGetValue", XS_ScriptValueArrayGetValue);
	tcc_add_symbol(s, "xvoArrayAppendValue", xvoArrayAppendValue);
	tcc_add_symbol(s, "xvoTableSetValue", xvoTableSetValue);
	tcc_add_symbol(s, "xvoTableGetValue", xvoTableGetValue);
	tcc_add_symbol(s, "xvoGetText", xvoGetText);
	tcc_add_symbol(s, "xvoGetBool", xvoGetBool);
	tcc_add_symbol(s, "xvoGetInt", xvoGetInt);
	tcc_add_symbol(s, "xrtDictCreate", xrtDictCreate);
	tcc_add_symbol(s, "xrtDictDestroy", xrtDictDestroy);
	tcc_add_symbol(s, "xrtDictSet", xrtDictSet);
	tcc_add_symbol(s, "xrtDictGet", xrtDictGet);
	tcc_add_symbol(s, "xrtDictRemove", xrtDictRemove);
	tcc_add_symbol(s, "sqlite3_open", sqlite3_open);
	tcc_add_symbol(s, "sqlite3_close", sqlite3_close);
	tcc_add_symbol(s, "sqlite3_exec", sqlite3_exec);
	tcc_add_symbol(s, "sqlite3_prepare_v2", sqlite3_prepare_v2);
	tcc_add_symbol(s, "sqlite3_step", sqlite3_step);
	tcc_add_symbol(s, "sqlite3_finalize", sqlite3_finalize);
	tcc_add_symbol(s, "sqlite3_reset", sqlite3_reset);
	tcc_add_symbol(s, "sqlite3_clear_bindings", sqlite3_clear_bindings);
	tcc_add_symbol(s, "sqlite3_free", sqlite3_free);
	tcc_add_symbol(s, "sqlite3_errmsg", sqlite3_errmsg);
	tcc_add_symbol(s, "sqlite3_bind_text", sqlite3_bind_text);
	tcc_add_symbol(s, "sqlite3_bind_int", sqlite3_bind_int);
	tcc_add_symbol(s, "sqlite3_bind_int64", sqlite3_bind_int64);
	tcc_add_symbol(s, "sqlite3_column_text", sqlite3_column_text);
	tcc_add_symbol(s, "sqlite3_column_int", sqlite3_column_int);
	tcc_add_symbol(s, "sqlite3_column_int64", sqlite3_column_int64);
	tcc_add_symbol(s, "sqlite3_column_count", sqlite3_column_count);
	tcc_add_symbol(s, "sqlite3_column_name", sqlite3_column_name);
	tcc_add_symbol(s, "xsLog", XS_ScriptLog);
	tcc_add_symbol(s, "xsServerName", XS_ScriptServerName);
	tcc_add_symbol(s, "xsServerClass", XS_ScriptServerClass);
	tcc_add_symbol(s, "xsServerDebug", XS_ScriptServerDebug);
	tcc_add_symbol(s, "xsServerAddr", XS_ScriptServerAddr);
	tcc_add_symbol(s, "xsServerParam", XS_ScriptServerParam);
	tcc_add_symbol(s, "xsAppPath", XS_ScriptAppPath);
	tcc_add_symbol(s, "xsHostName", XS_ScriptHostName);
	tcc_add_symbol(s, "xsHostParam", XS_ScriptHostParam);
	tcc_add_symbol(s, "xsHostPath", XS_ScriptHostPath);
	tcc_add_symbol(s, "xsHostDevFile", XS_ScriptHostDevFile);
	tcc_add_symbol(s, "xsHostDebug", XS_ScriptHostDebug);
	tcc_add_symbol(s, "xsHostDevMode", XS_ScriptHostDevMode);
	tcc_add_symbol(s, "xsReqMethod", XS_ScriptRequestMethod);
	tcc_add_symbol(s, "xsReqTarget", XS_ScriptRequestTarget);
	tcc_add_symbol(s, "xsReqPath", XS_ScriptRequestPath);
	tcc_add_symbol(s, "xsReqQuery", XS_ScriptRequestQuery);
	tcc_add_symbol(s, "xsReqBody", XS_ScriptRequestBody);
	tcc_add_symbol(s, "xsReqBodyLen", XS_ScriptRequestBodyLen);
	tcc_add_symbol(s, "xsReqHeader", XS_ScriptRequestHeader);
	tcc_add_symbol(s, "xsHttpStatus", XS_ScriptHttpStatus);
	tcc_add_symbol(s, "xsHttpHeader", XS_ScriptHttpHeader);
	tcc_add_symbol(s, "xsHttpText", XS_ScriptHttpText);
	tcc_add_symbol(s, "xsHttpBody", XS_ScriptHttpBody);
	tcc_add_symbol(s, "xsHttpJson", XS_ScriptHttpJson);
	tcc_add_symbol(s, "xsWsIsOpen", XS_ScriptWsIsOpen);
	tcc_add_symbol(s, "xsWsSendText", XS_ScriptWsSendText);
	tcc_add_symbol(s, "xsWsSendBinary", XS_ScriptWsSendBinary);
	tcc_add_symbol(s, "xsWsProtocol", XS_ScriptWsProtocol);
	tcc_add_symbol(s, "xsWsPing", XS_ScriptWsPing);
	tcc_add_symbol(s, "xsWsClose", XS_ScriptWsClose);
	tcc_add_symbol(s, "xsStreamSend", XS_ScriptStreamSend);
	tcc_add_symbol(s, "xsStreamClose", XS_ScriptStreamClose);
	tcc_add_symbol(s, "xsXtpSend", XS_ScriptXtpSend);
	tcc_add_symbol(s, "xsXtpSendEx", XS_ScriptXtpSendEx);
	tcc_add_symbol(s, "xsXtpGetParam", XS_ScriptXtpGetParam);
	tcc_add_symbol(s, "xsXtpReply", XS_ScriptXtpReply);
	tcc_add_symbol(s, "xsXtpReplyEx", XS_ScriptXtpReplyEx);
	tcc_add_symbol(s, "xsXtpMsgId", XS_ScriptXtpMsgId);
	tcc_add_symbol(s, "xsXtpMsgType", XS_ScriptXtpMsgType);
	tcc_add_symbol(s, "xsXtpMsgFlags", XS_ScriptXtpMsgFlags);
	tcc_add_symbol(s, "xsXtpStatus", XS_ScriptXtpStatus);
	tcc_add_symbol(s, "xsXtpCmd", XS_ScriptXtpCmd);
	tcc_add_symbol(s, "xsXtpCmdLen", XS_ScriptXtpCmdLen);
	tcc_add_symbol(s, "xsXtpBody", XS_ScriptXtpBody);
	tcc_add_symbol(s, "xsXtpBodyLen", XS_ScriptXtpBodyLen);
	tcc_add_symbol(s, "xsXtpParamCount", XS_ScriptXtpParamCount);
	tcc_add_symbol(s, "xsXtpNeedReply", XS_ScriptXtpNeedReply);
	tcc_add_symbol(s, "xsXtpParamKeyAt", XS_ScriptXtpParamKeyAt);
	tcc_add_symbol(s, "xsXtpParamValueAt", XS_ScriptXtpParamValueAt);
	tcc_add_symbol(s, "xsXtpFindParamView", XS_ScriptXtpFindParamView);
	tcc_add_symbol(s, "xsDgramSendTo", XS_ScriptDgramSendTo);
	tcc_add_symbol(s, "xsDgramReply", XS_ScriptDgramReply);
	tcc_add_symbol(s, "xsAddrText", XS_ScriptAddrText);
	tcc_add_symbol(s, "xsReloadCurrentHost", XS_ScriptReloadCurrentHost);
	tcc_add_symbol(s, "xsReloadHostByName", XS_ScriptReloadHostByName);
	tcc_add_symbol(s, "xsDataRegister", XS_ScriptDataRegister);
	tcc_add_symbol(s, "xsDataRegisterEx", XS_ScriptDataRegisterEx);
	tcc_add_symbol(s, "xsDataGet", XS_ScriptDataGet);
	tcc_add_symbol(s, "xsDataRetain", XS_ScriptDataRetain);
	tcc_add_symbol(s, "xsDataRelease", XS_ScriptDataRelease);
	tcc_add_symbol(s, "xsDataRemove", XS_ScriptDataRemove);
	tcc_add_symbol(s, "xsDataRemoveByQuery", XS_ScriptDataRemoveByQuery);
	tcc_add_symbol(s, "xsDataFindFirst", XS_ScriptDataFindFirst);
	tcc_add_symbol(s, "xsBusStatusJson", XS_ScriptBusStatusJson);
	tcc_add_symbol(s, "xsBusStatusJsonEx", XS_ScriptBusStatusJsonEx);
	tcc_add_symbol(s, "xsBusNamespaceJson", XS_ScriptBusNamespaceJson);
	tcc_add_symbol(s, "xsBusLastErrorCode", XS_ScriptBusLastErrorCode);
	tcc_add_symbol(s, "xsBusLastError", XS_ScriptBusLastError);
	tcc_add_symbol(s, "xsMsgSendToHost", XS_ScriptMsgSendToHost);
	tcc_add_symbol(s, "xsMsgSendToServer", XS_ScriptMsgSendToServer);
	tcc_add_symbol(s, "xsMsgBroadcast", XS_ScriptMsgBroadcast);
}

#endif
