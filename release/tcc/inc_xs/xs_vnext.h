#ifndef XS_VNEXT_H
#define XS_VNEXT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef void* XS_ServerObject;
typedef void* XS_HostObject;
typedef const void* XS_RequestObject;
typedef void* XS_ResponseObject;

#define SLT_STATIC		0
#define SLT_C			1
#define SLT_PROTOCOL	2

void xsLog(const char* sText);
const char* xsServerName(XS_ServerObject objServer);
int xsServerClass(XS_ServerObject objServer);
int xsServerDebug(XS_ServerObject objServer);
const char* xsServerAddr(XS_ServerObject objServer);
const char* xsServerParam(XS_ServerObject objServer);
const char* xsAppPath(void);
const char* xsHostName(XS_HostObject objHost);
const char* xsHostParam(XS_HostObject objHost);
const char* xsHostPath(XS_HostObject objHost);
const char* xsHostDevFile(XS_HostObject objHost);
int xsHostDebug(XS_HostObject objHost);
int xsHostDevMode(XS_HostObject objHost);
const char* xsReqMethod(XS_RequestObject objReq);
const char* xsReqTarget(XS_RequestObject objReq);
const char* xsReqPath(XS_RequestObject objReq);
const char* xsReqQuery(XS_RequestObject objReq);
const void* xsReqBody(XS_RequestObject objReq);
size_t xsReqBodyLen(XS_RequestObject objReq);
const char* xsReqHeader(XS_RequestObject objReq, const char* sName);
int xsHttpStatus(XS_ResponseObject objResp, unsigned iStatus, const char* sReason);
int xsHttpHeader(XS_ResponseObject objResp, const char* sName, const char* sValue);
int xsHttpText(XS_ResponseObject objResp, unsigned iStatus, const char* sReason, const char* sText);
int xsHttpBody(XS_ResponseObject objResp, const void* pData, size_t iLen, const char* sContentType);
int xsHttpJson(XS_ResponseObject objResp, unsigned iStatus, const char* sReason, const char* sJson);
int xsWsIsOpen(void* pConn);
int xsWsSendText(void* pConn, const char* sText, size_t iLen);
int xsWsSendBinary(void* pConn, const void* pData, size_t iLen);
const char* xsWsProtocol(void* pConn);
int xsWsPing(void* pConn, const void* pData, size_t iLen);
int xsWsClose(void* pConn, unsigned short iCode, const char* sReason);
int xsStreamSend(void* pStream, const void* pData, size_t iLen);
int xsStreamClose(void* pStream, unsigned iFlags);
int xsXtpSend(void* pStream, const char* sCmd, size_t iCmdSize, unsigned iParamCount, const char** arrParam, const char** arrValue, const void* pBody, size_t iBodySize);
int xsXtpSendEx(void* pStream, unsigned iMsgType, uint64_t iMsgID, unsigned iFlags, int iStatus, const char* sCmd, size_t iCmdSize, unsigned iParamCount, const char** arrParam, const char** arrValue, const void* pBody, size_t iBodySize);
const char* xsXtpGetParam(const void* pMsg, const char* sKey);
int xsXtpReply(void* pStream, const void* pReqMsg, const char* sCmd, size_t iCmdSize, unsigned iParamCount, const char** arrParam, const char** arrValue, const void* pBody, size_t iBodySize);
int xsXtpReplyEx(void* pStream, const void* pReqMsg, int iStatus, const char* sCmd, size_t iCmdSize, unsigned iParamCount, const char** arrParam, const char** arrValue, const void* pBody, size_t iBodySize);
uint64_t xsXtpMsgId(const void* pMsg);
unsigned xsXtpMsgType(const void* pMsg);
unsigned xsXtpMsgFlags(const void* pMsg);
int xsXtpStatus(const void* pMsg);
const char* xsXtpCmd(const void* pMsg);
unsigned xsXtpCmdLen(const void* pMsg);
const void* xsXtpBody(const void* pMsg);
unsigned xsXtpBodyLen(const void* pMsg);
unsigned xsXtpParamCount(const void* pMsg);
int xsXtpNeedReply(const void* pMsg);
const char* xsXtpParamKeyAt(const void* pMsg, unsigned iIndex, unsigned* piLen);
const char* xsXtpParamValueAt(const void* pMsg, unsigned iIndex, unsigned* piLen);
int xsXtpFindParamView(const void* pMsg, const char* sKey, const char** ppVal, unsigned* piLen);
int xsDgramSendTo(void* pSock, const char* sIP, unsigned short iPort, const void* pData, size_t iLen);
int xsDgramReply(void* pSock, const void* pFromAddr, const void* pData, size_t iLen);
const char* xsAddrText(const void* pAddr);
int xsReloadCurrentHost(XS_ServerObject objServer, XS_HostObject objHost, int bForce);
int xsReloadHostByName(XS_ServerObject objServer, const char* sHostName, int bForce);

#endif
