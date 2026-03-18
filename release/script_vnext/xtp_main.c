#include <xs_vnext_full.h>
#include <string.h>

void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
	char sText[256];
	(void)objHost;
	
	snprintf(
		sText,
		sizeof(sText),
		"xtp init server=%s",
		xsServerName(objServer)
	);
	xsLog(sText);
}

void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
	char sText[256];
	(void)objHost;
	
	snprintf(
		sText,
		sizeof(sText),
		"xtp unit server=%s",
		xsServerName(objServer)
	);
	xsLog(sText);
}

bool EventXtpProc(XS_ServerObject objServer, void* pStream, void* pMsg)
{
	XTP_MessageObject objMsg;
	const char* sTag;
	const char* pCmd;
	const void* pBody;
	const char* arrParam[3];
	const char* arrValue[3];
	char sCmdText[128];
	char sBody[512];
	int iWrite;
	uint64_t iMsgID;
	unsigned iMsgType;
	unsigned iStatus;
	unsigned iCmdLen;
	unsigned iBodyLen;
	
	objMsg = (XTP_MessageObject)pMsg;
	if ( objMsg == NULL ) {
		return false;
	}
	
	iMsgID = xsXtpMsgId(objMsg);
	iMsgType = xsXtpMsgType(objMsg);
	iStatus = (unsigned)xsXtpStatus(objMsg);
	pCmd = xsXtpCmd(objMsg);
	iCmdLen = xsXtpCmdLen(objMsg);
	pBody = xsXtpBody(objMsg);
	iBodyLen = xsXtpBodyLen(objMsg);
	sTag = xsXtpGetParam(objMsg, "tag");
	if ( iCmdLen >= sizeof(sCmdText) ) {
		iCmdLen = sizeof(sCmdText) - 1;
	}
	if ( pCmd && iCmdLen > 0 ) {
		memcpy(sCmdText, pCmd, iCmdLen);
		sCmdText[iCmdLen] = '\0';
	} else {
		sCmdText[0] = '\0';
	}
	arrParam[0] = "result";
	arrValue[0] = "ok";
	arrParam[1] = "cmd";
	arrValue[1] = sCmdText;
	arrParam[2] = "tag";
	arrValue[2] = sTag ? sTag : "";
	
	iWrite = snprintf(
		sBody,
		sizeof(sBody),
		"xtp demo\nserver=%s\nmsg_type=%u\nmsg_id=%llu\nstatus=%u\ncmd=%.*s\nbody=%.*s\n",
		xsServerName(objServer),
		iMsgType,
		(unsigned long long)iMsgID,
		iStatus,
		(int)iCmdLen,
		pCmd ? pCmd : "",
		(int)iBodyLen,
		pBody ? (const char*)pBody : ""
	);
	if ( iWrite < 0 ) {
		return false;
	}
	
	if ( iMsgType != 1 ) {
		return true;
	}
	if ( iMsgID == 0 ) {
		return true;
	}
	
	return xsXtpReply(
		pStream,
		objMsg,
		"xtp.reply",
		0,
		3,
		arrParam,
		arrValue,
		sBody,
		strlen(sBody)
	) != 0;
}
