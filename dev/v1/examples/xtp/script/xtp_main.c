#include <xsbase.h>
#include <string.h>

#include "app_db.h"



void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
	sqlite3* pDB;

	(void)objServer;
	(void)objHost;

	pDB = procDBOpen();
	if ( pDB ) {
		sqlite3_close(pDB);
	}
}



bool EventXtpProc(XS_ServerObject objServer, void* pStream, void* pMsg)
{
	XTP_MessageObject objMsg = (XTP_MessageObject)pMsg;
	const char* sLevel;
	const char* sSource;
	const char* sCmd;
	const void* pBody;
	size_t iBodyLen;
	char* sBody;
	bool bOK;

	(void)objServer;

	if ( objMsg == NULL ) {
		return FALSE;
	}
	if ( !xsXtpCmdIs(objMsg, "log.push") ) {
		if ( xsXtpNeedReply(objMsg) ) {
			return xsXtpReplyUnsupportedCmd(pStream, objMsg, xsXtpCmd(objMsg)) != 0;
		}
		return TRUE;
	}

	sLevel = xsXtpParamText(objMsg, "level", "INFO");
	sSource = xsXtpParamText(objMsg, "source", "xtp-client");
	sCmd = xsXtpCmd(objMsg);
	pBody = xsXtpBody(objMsg);
	iBodyLen = xsXtpBodyLen(objMsg);
	sBody = xsXtpBodyDup(objMsg, "");
	if ( sBody == NULL && pBody && iBodyLen > 0 ) {
		return FALSE;
	}

	bOK = procInsertLog(sLevel, sSource, sCmd, (int)xsXtpMsgType(objMsg), sBody ? sBody : "");
	if ( sBody ) {
		xrtFree(sBody);
	}

	if ( xsXtpNeedReply(objMsg) ) {
		if ( bOK ) {
			return xsXtpReplyOKJson(pStream, objMsg, "log.reply", "{\"result\":true,\"message\":\"log accepted\"}") != 0;
		}

		return xsXtpReplyErrorJson(pStream, objMsg, 500, "log.reply", "{\"result\":false,\"message\":\"sqlite write failed\"}") != 0;
	}

	return TRUE;
}
