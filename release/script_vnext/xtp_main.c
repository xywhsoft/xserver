#include <xsbase.h>
#include <string.h>

#ifndef XTP_MSG_RESPONSE
	#define XTP_MSG_RESPONSE	2u
#endif

static unsigned short procParsePort(const char* sAddr)
{
	const char* sPos;
	const char* sPort;
	unsigned long iPort;

	if ( sAddr == NULL ) {
		return 0;
	}

	sPos = strrchr(sAddr, ':');
	if ( sPos == NULL || sPos[1] == '\0' ) {
		return 0;
	}

	sPort = sPos + 1;
	for ( ; *sPort; ++sPort ) {
		if ( *sPort < '0' || *sPort > '9' ) {
			return 0;
		}
	}

	iPort = strtoul(sPos + 1, NULL, 10);
	if ( iPort == 0 || iPort > 65535ul ) {
		return 0;
	}

	return (unsigned short)iPort;
}

static unsigned short procParseRequestPort(XTP_MessageObject objMsg, int* piState)
{
	const char* sPort;
	const char* sPortText;
	unsigned long iPort;

	if ( piState ) {
		*piState = 0;
	}
	if ( objMsg == NULL ) {
		return 0;
	}

	sPortText = xsXtpParamText(objMsg, "port", NULL);
	if ( sPortText == NULL || sPortText[0] == '\0' ) {
		return 0;
	}

	sPort = sPortText;
	for ( ; *sPort; ++sPort ) {
		if ( *sPort < '0' || *sPort > '9' ) {
			if ( piState ) {
				*piState = -1;
			}
			return 0;
		}
	}

	iPort = strtoul(sPortText, NULL, 10);
	if ( iPort == 0 || iPort > 65535ul ) {
		if ( piState ) {
			*piState = -1;
		}
		return 0;
	}

	if ( piState ) {
		*piState = 1;
	}
	return (unsigned short)iPort;
}

static char* procDupText(const char* sText)
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

typedef struct
{
	void* pStream;
	xfuture* pCloseFuture;
	uint64_t iMsgID;
	unsigned short iPort;
	char* sTag;
} XTP_CallSelfTask;

static uint32 procCallSelfThread(ptr pParam)
{
	XTP_CallSelfTask* pTask;
	XTP_MessageObject objResp;
	const char* arrInnerParam[3];
	const char* arrInnerValue[3];
	char* sRespBody;
	char sBody[512];
	const char* sCmd;
	int iStatus;

	pTask = (XTP_CallSelfTask*)pParam;
	if ( pTask == NULL ) {
		return 0;
	}

	arrInnerParam[0] = "tag";
	arrInnerValue[0] = (pTask->sTag && pTask->sTag[0]) ? pTask->sTag : "self";
	arrInnerParam[1] = "seq";
	arrInnerValue[1] = "1";
	arrInnerParam[2] = "dry";
	arrInnerValue[2] = "false";

	objResp = (XTP_MessageObject)xsXtpClientCallText(
		"127.0.0.1",
		pTask->iPort,
		1048576u,
		1500u,
		(pTask->iMsgID == 0u) ? 1u : (pTask->iMsgID + 1u),
		"demo.ping",
		3u,
		arrInnerParam,
		arrInnerValue,
		"hello self call",
		1500u
	);
	if ( objResp == NULL ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"client do failed\ncode=%d\nerror=%s\n",
			xsXtpClientLastErrorCode(),
			xsXtpClientLastError() ? xsXtpClientLastError() : ""
		);
		sCmd = "xtp.error";
		iStatus = 500;
	} else {
		sRespBody = xsXtpBodyDup(objResp, "");
		snprintf(
			sBody,
			sizeof(sBody),
			"self call ok\nstatus=%d\ncmd=%.*s\nbody=%s\n",
			xsXtpStatus(objResp),
			(int)xsXtpCmdLen(objResp),
			xsXtpCmd(objResp) ? xsXtpCmd(objResp) : "",
			sRespBody ? sRespBody : ""
		);
		if ( sRespBody ) {
			xrtFree(sRespBody);
		}
		xsXtpMessageFree(objResp);
		sCmd = "xtp.reply";
		iStatus = 0;
	}

	if ( pTask->iMsgID != 0u ) {
		(void)xsXtpSendEx(
			pTask->pStream,
			XTP_MSG_RESPONSE,
			pTask->iMsgID,
			0u,
			iStatus,
			sCmd,
			strlen(sCmd),
			0u,
			NULL,
			NULL,
			sBody,
			strlen(sBody)
		);
	}

	if ( pTask->sTag ) {
		xrtFree(pTask->sTag);
	}
	if ( pTask->pCloseFuture ) {
		xFutureRelease(pTask->pCloseFuture);
	}
	xrtFree(pTask);
	return 0;
}

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
	char* sTagDup;
	char* sHostDup;
	char* sInnerCmdDup;
	const char* sTag;
	const char* sHost;
	const char* sInnerCmd;
	const char* pCmd;
	const void* pBody;
	const char* arrParam[3];
	const char* arrValue[3];
	char sCmdText[128];
	char sBody[512];
	char sJson[256];
	char sInnerBody[512];
	const char* arrInnerParam[3];
	const char* arrInnerValue[3];
	XTP_MessageObject objResp;
	void* pClient;
	int64_t iSeq;
	int iDry;
	int iPortState;
	int iWrite;
	uint64_t iMsgID;
	unsigned iMsgType;
	unsigned iStatus;
	unsigned short iPort;
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
	sTagDup = xsXtpParamDup(objMsg, "tag", "");
	sTag = sTagDup ? sTagDup : "";
	sHostDup = xsXtpParamDup(objMsg, "host", "127.0.0.1");
	sHost = sHostDup ? sHostDup : "127.0.0.1";
	sInnerCmdDup = xsXtpParamDup(objMsg, "inner_cmd", "demo.ping");
	sInnerCmd = sInnerCmdDup ? sInnerCmdDup : "demo.ping";
	iSeq = xsXtpParamInt(objMsg, "seq", -1);
	iPortState = 0;
	iPort = procParseRequestPort(objMsg, &iPortState);
	iDry = xsXtpParamBool(objMsg, "dry", 0);
	if ( iCmdLen >= sizeof(sCmdText) ) {
		iCmdLen = sizeof(sCmdText) - 1;
	}
	if ( pCmd && iCmdLen > 0 ) {
		memcpy(sCmdText, pCmd, iCmdLen);
		sCmdText[iCmdLen] = '\0';
	} else {
		sCmdText[0] = '\0';
	}
#define FREE_XTP_DUP() do { \
	if ( sTagDup ) xrtFree(sTagDup); \
	if ( sHostDup ) xrtFree(sHostDup); \
	if ( sInnerCmdDup ) xrtFree(sInnerCmdDup); \
} while ( 0 )
#define CHECK_XTP_PORT() do { \
	if ( iPort == 0 ) { \
		if ( iPortState < 0 ) { \
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 400, "xtp.error", "invalid port") != 0; \
		} else { \
			iWrite = xsXtpReplyMissingParam(pStream, objMsg, "port") != 0; \
		} \
		FREE_XTP_DUP(); \
		return iWrite; \
	} \
} while ( 0 )
	if ( xsXtpCmdIs(objMsg, "demo.call") ) {
		CHECK_XTP_PORT();

		pClient = xsXtpClientOpen(sHost, iPort, 1048576u, 3000u);
		if ( pClient == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client open failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		arrInnerParam[0] = "tag";
		arrInnerValue[0] = sTag && sTag[0] ? sTag : "remote";
		arrInnerParam[1] = "seq";
		arrInnerValue[1] = "1";
		arrInnerParam[2] = "dry";
		arrInnerValue[2] = "false";
		objResp = (XTP_MessageObject)xsXtpClientDoText(
			pClient,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3,
			arrInnerParam,
			arrInnerValue,
			"hello remote call",
			3000u
		);
		xsXtpClientClose(pClient);
		if ( objResp == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client do failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		snprintf(
			sInnerBody,
			sizeof(sInnerBody),
			"remote call ok\nstatus=%d\ncmd=%.*s\nbody=%.*s\n",
			xsXtpStatus(objResp),
			(int)xsXtpCmdLen(objResp),
			xsXtpCmd(objResp) ? xsXtpCmd(objResp) : "",
			(int)xsXtpBodyLen(objResp),
			xsXtpBody(objResp) ? (const char*)xsXtpBody(objResp) : ""
		);
		xsXtpMessageFree(objResp);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callsimple") ) {
		CHECK_XTP_PORT();

		pClient = xsXtpClientOpen(sHost, iPort, 1048576u, 3000u);
		if ( pClient == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client open failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		objResp = (XTP_MessageObject)xsXtpClientDoSimple(
			pClient,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3000u
		);
		xsXtpClientClose(pClient);
		if ( objResp == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client do failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		snprintf(
			sInnerBody,
			sizeof(sInnerBody),
			"remote simple ok=%s\nstatus=%d\ncmd=%.*s\nbody=%.*s\n",
			xsXtpIsOK(objResp) ? "true" : "false",
			xsXtpStatus(objResp),
			(int)xsXtpCmdLen(objResp),
			xsXtpCmd(objResp) ? xsXtpCmd(objResp) : "",
			(int)xsXtpBodyLen(objResp),
			xsXtpBody(objResp) ? (const char*)xsXtpBody(objResp) : ""
		);
		xsXtpMessageFree(objResp);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callonce") ) {
		CHECK_XTP_PORT();

		objResp = (XTP_MessageObject)xsXtpClientCallSimple(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3000u
		);
		if ( objResp == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		{
			char* sRespBody = xsXtpBodyDup(objResp, "");

			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"remote call once ok=%s\nstatus=%d\ncmd=%.*s\nbody=%s\n",
				xsXtpIsOK(objResp) ? "true" : "false",
				xsXtpStatus(objResp),
				(int)xsXtpCmdLen(objResp),
				xsXtpCmd(objResp) ? xsXtpCmd(objResp) : "",
				sRespBody ? sRespBody : ""
			);
			if ( sRespBody ) {
				xrtFree(sRespBody);
			}
		}
		xsXtpMessageFree(objResp);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.calljson") ) {
		CHECK_XTP_PORT();

		snprintf(
			sJson,
			sizeof(sJson),
			"{\"tag\":\"%s\",\"seq\":%lld,\"dry\":%s}",
			sTag ? sTag : "",
			(long long)iSeq,
			iDry ? "true" : "false"
		);
		objResp = (XTP_MessageObject)xsXtpClientCallJson(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			0,
			NULL,
			NULL,
			sJson,
			3000u
		);
		if ( objResp == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call json failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		{
			char* sRespBody = xsXtpBodyDup(objResp, "");

			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"remote call json ok=%s\nstatus=%d\ncmd=%.*s\nbody=%s\n",
				xsXtpIsOK(objResp) ? "true" : "false",
				xsXtpStatus(objResp),
				(int)xsXtpCmdLen(objResp),
				xsXtpCmd(objResp) ? xsXtpCmd(objResp) : "",
				sRespBody ? sRespBody : ""
			);
			if ( sRespBody ) {
				xrtFree(sRespBody);
			}
		}
		xsXtpMessageFree(objResp);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callbody") ) {
		char* sRespBody;

		CHECK_XTP_PORT();

		sRespBody = xsXtpClientCallSimpleBody(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3000u,
			""
		);
		if ( sRespBody == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call body failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		snprintf(
			sInnerBody,
			sizeof(sInnerBody),
			"remote body only ok\nbody=%s\n",
			sRespBody
		);
		xrtFree(sRespBody);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callsummary") ) {
		char* sSummary;

		CHECK_XTP_PORT();

		sSummary = xsXtpClientCallSimpleSummary(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3000u
		);
		if ( sSummary == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call summary failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		snprintf(
			sInnerBody,
			sizeof(sInnerBody),
			"remote summary ok\n%s",
			sSummary
		);
		xrtFree(sSummary);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callsummaryjson") ) {
		char* sSummary;

		CHECK_XTP_PORT();

		sSummary = xsXtpClientCallSimpleSummaryJson(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3000u
		);
		if ( sSummary == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call summary json failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		iWrite = xsXtpReplyOKJson(pStream, objMsg, "xtp.reply", sSummary) != 0;
		xrtFree(sSummary);
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callresult") ) {
		char* sResult;

		CHECK_XTP_PORT();

		sResult = xsXtpClientCallSimpleResult(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3000u,
			"(none)"
		);
		if ( sResult == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call result failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		snprintf(
			sInnerBody,
			sizeof(sInnerBody),
			"remote result ok\nresult=%s\n",
			sResult
		);
		xrtFree(sResult);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callstatus") ) {
		int iRemoteStatus;
		char* sRemoteCmd;

		CHECK_XTP_PORT();

		iRemoteStatus = xsXtpClientCallSimpleStatus(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3000u,
			-999
		);
		sRemoteCmd = xsXtpClientCallSimpleCmd(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 2u),
			sInnerCmd,
			3000u,
			"(none)"
		);
		if ( sRemoteCmd == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call status failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		snprintf(
			sInnerBody,
			sizeof(sInnerBody),
			"remote status ok\nstatus=%d\nis_ok=%s\ncmd=%s\n",
			iRemoteStatus,
			(iRemoteStatus == 0) ? "true" : "false",
			sRemoteCmd
		);
		xrtFree(sRemoteCmd);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callmeta") ) {
		char* sMeta;

		CHECK_XTP_PORT();

		sMeta = xsXtpClientCallSimpleMeta(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3000u
		);
		if ( sMeta == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call meta failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		snprintf(
			sInnerBody,
			sizeof(sInnerBody),
			"remote meta ok\n%s\n",
			sMeta
		);
		xrtFree(sMeta);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callmetajson") ) {
		char* sMeta;

		CHECK_XTP_PORT();

		sMeta = xsXtpClientCallSimpleMetaJson(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3000u
		);
		if ( sMeta == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call meta json failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		iWrite = xsXtpReplyOKJson(pStream, objMsg, "xtp.reply", sMeta) != 0;
		xrtFree(sMeta);
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callvalue") ) {
		xvalue objVal;
		char* sValueJson;

		CHECK_XTP_PORT();

		objVal = xsXtpClientCallSimpleValue(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3000u
		);
		if ( objVal == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call value failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		sValueJson = xrtStringifyJSON(objVal, FALSE, NULL);
		xvoUnref(objVal);
		if ( sValueJson == NULL ) {
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "stringify value failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		iWrite = xsXtpReplyOKJson(pStream, objMsg, "xtp.reply", sValueJson) != 0;
		xrtFree(sValueJson);
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callparamsvalue") ) {
		xvalue objVal;
		char* sValueJson;

		CHECK_XTP_PORT();

		objVal = xsXtpClientCallSimpleParamsValue(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd && sInnerCmd[0] ? sInnerCmd : "demo.python",
			3000u
		);
		if ( objVal == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call params value failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		sValueJson = xrtStringifyJSON(objVal, FALSE, NULL);
		xvoUnref(objVal);
		if ( sValueJson == NULL ) {
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "stringify params value failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		iWrite = xsXtpReplyOKJson(pStream, objMsg, "xtp.reply", sValueJson) != 0;
		xrtFree(sValueJson);
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callbodyvalue") ) {
		xvalue objVal;
		char* sValueJson;

		CHECK_XTP_PORT();

		objVal = xsXtpClientCallSimpleBodyValue(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			xsXtpHasParam(objMsg, "inner_cmd") ? sInnerCmd : "demo.json.reply",
			3000u
		);
		if ( objVal == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call body value failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		sValueJson = xrtStringifyJSON(objVal, FALSE, NULL);
		xvoUnref(objVal);
		if ( sValueJson == NULL ) {
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "stringify body value failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		iWrite = xsXtpReplyOKJson(pStream, objMsg, "xtp.reply", sValueJson) != 0;
		xrtFree(sValueJson);
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.calltablevalue") ) {
		xvalue objParam;
		xvalue objBody;
		XTP_MessageObject objResp;
		xvalue objRespVal;
		char* sValueJson;

		CHECK_XTP_PORT();

		objParam = xvoCreateTable();
		objBody = xvoCreateTable();
		if ( objParam == NULL || objBody == NULL ) {
			if ( objParam ) {
				xvoUnref(objParam);
			}
			if ( objBody ) {
				xvoUnref(objBody);
			}
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "create value failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		xvoTableSetText(objParam, "tag", 0, (ptr)(sTag && sTag[0] ? sTag : "tablev"), 0, FALSE);
		xvoTableSetInt(objParam, "seq", 0, iSeq >= 0 ? iSeq : 100);
		xvoTableSetBool(objParam, "dry", 0, iDry ? TRUE : FALSE);

		xvoTableSetText(objBody, "kind", 0, (ptr)"table-body", 0, FALSE);
		xvoTableSetText(objBody, "tag", 0, (ptr)(sTag && sTag[0] ? sTag : "tablev"), 0, FALSE);

		objResp = (XTP_MessageObject)xsXtpClientCallTableValue(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd && sInnerCmd[0] ? sInnerCmd : "demo.python",
			objParam,
			objBody,
			3000u
		);
		xvoUnref(objParam);
		xvoUnref(objBody);
		if ( objResp == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call table value failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		objRespVal = xsXtpValue(objResp);
		xsXtpMessageFree(objResp);
		if ( objRespVal == NULL ) {
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "xtp value build failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		sValueJson = xrtStringifyJSON(objRespVal, FALSE, NULL);
		xvoUnref(objRespVal);
		if ( sValueJson == NULL ) {
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "stringify response value failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		iWrite = xsXtpReplyOKJson(pStream, objMsg, "xtp.reply", sValueJson) != 0;
		xrtFree(sValueJson);
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callrequest") ) {
		void* pReq;
		xvalue objParam;
		xvalue objBody;
		xvalue objRespVal;
		char* sValueJson;

		CHECK_XTP_PORT();

		pReq = xsXtpRequestCreate(sInnerCmd && sInnerCmd[0] ? sInnerCmd : "demo.python");
		objParam = xvoCreateTable();
		objBody = xvoCreateTable();
		if ( pReq == NULL || objParam == NULL || objBody == NULL ) {
			if ( pReq ) {
				xsXtpRequestFree(pReq);
			}
			if ( objParam ) {
				xvoUnref(objParam);
			}
			if ( objBody ) {
				xvoUnref(objBody);
			}
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "create request failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		xvoTableSetText(objParam, "tag", 0, (ptr)(sTag && sTag[0] ? sTag : "request"), 0, FALSE);
		xvoTableSetInt(objParam, "seq", 0, iSeq >= 0 ? iSeq : 200);
		xvoTableSetBool(objParam, "dry", 0, iDry ? TRUE : FALSE);
		xsXtpRequestSetParamsValue(pReq, objParam);
		xvoUnref(objParam);
		xvoTableSetText(objBody, "kind", 0, (ptr)"request-body", 0, FALSE);
		xvoTableSetText(objBody, "tag", 0, (ptr)(sTag && sTag[0] ? sTag : "request"), 0, FALSE);
		xsXtpRequestSetBodyValue(pReq, objBody);
		xvoUnref(objBody);

		objRespVal = xsXtpClientCallRequestValue(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			pReq,
			3000u
		);
		xsXtpRequestFree(pReq);
		if ( objRespVal == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call request failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		sValueJson = xrtStringifyJSON(objRespVal, FALSE, NULL);
		xvoUnref(objRespVal);
		if ( sValueJson == NULL ) {
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "stringify request value failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		iWrite = xsXtpReplyOKJson(pStream, objMsg, "xtp.reply", sValueJson) != 0;
		xrtFree(sValueJson);
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callrequestsummaryjson") ) {
		void* pReq;
		xvalue objParam;
		xvalue objBody;
		char* sSummaryJson;

		CHECK_XTP_PORT();

		pReq = xsXtpRequestCreate(sInnerCmd && sInnerCmd[0] ? sInnerCmd : "demo.python");
		objParam = xvoCreateTable();
		objBody = xvoCreateTable();
		if ( pReq == NULL || objParam == NULL || objBody == NULL ) {
			if ( pReq ) {
				xsXtpRequestFree(pReq);
			}
			if ( objParam ) {
				xvoUnref(objParam);
			}
			if ( objBody ) {
				xvoUnref(objBody);
			}
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "create request summary failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		xvoTableSetText(objParam, "tag", 0, (ptr)(sTag && sTag[0] ? sTag : "request-summary"), 0, FALSE);
		xvoTableSetInt(objParam, "seq", 0, iSeq >= 0 ? iSeq : 300);
		xvoTableSetBool(objParam, "dry", 0, iDry ? TRUE : FALSE);
		xsXtpRequestSetParamsValue(pReq, objParam);
		xvoUnref(objParam);

		xvoTableSetText(objBody, "kind", 0, (ptr)"request-summary-body", 0, FALSE);
		xvoTableSetText(objBody, "tag", 0, (ptr)(sTag && sTag[0] ? sTag : "request-summary"), 0, FALSE);
		xsXtpRequestSetBodyValue(pReq, objBody);
		xvoUnref(objBody);

		sSummaryJson = xsXtpClientCallRequestSummaryJson(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			pReq,
			3000u
		);
		xsXtpRequestFree(pReq);
		if ( sSummaryJson == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call request summary json failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		iWrite = xsXtpReplyOKJson(pStream, objMsg, "xtp.reply", sSummaryJson) != 0;
		xrtFree(sSummaryJson);
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callrequestresultjson") ) {
		void* pReq;
		xvalue objParam;
		char* sResultJson;

		CHECK_XTP_PORT();

		pReq = xsXtpRequestCreate(sInnerCmd && sInnerCmd[0] ? sInnerCmd : "demo.python");
		objParam = xvoCreateTable();
		if ( pReq == NULL || objParam == NULL ) {
			if ( pReq ) {
				xsXtpRequestFree(pReq);
			}
			if ( objParam ) {
				xvoUnref(objParam);
			}
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "create request result failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		xvoTableSetText(objParam, "tag", 0, (ptr)(sTag && sTag[0] ? sTag : "request-result"), 0, FALSE);
		xsXtpRequestSetParamsValue(pReq, objParam);
		xvoUnref(objParam);

		sResultJson = xsXtpClientCallRequestResultJson(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			pReq,
			3000u
		);
		xsXtpRequestFree(pReq);
		if ( sResultJson == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call request result json failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		iWrite = xsXtpReplyOKJson(pStream, objMsg, "xtp.reply", sResultJson) != 0;
		xrtFree(sResultJson);
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callrequeststatus") ) {
		void* pReq;
		xvalue objParam;
		char* sCmdDup;
		int iCallOK;
		int iCallStatus;

		CHECK_XTP_PORT();

		pReq = xsXtpRequestCreate(sInnerCmd && sInnerCmd[0] ? sInnerCmd : "demo.python");
		objParam = xvoCreateTable();
		if ( pReq == NULL || objParam == NULL ) {
			if ( pReq ) {
				xsXtpRequestFree(pReq);
			}
			if ( objParam ) {
				xvoUnref(objParam);
			}
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "create request status failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		xvoTableSetText(objParam, "tag", 0, (ptr)(sTag && sTag[0] ? sTag : "request-status"), 0, FALSE);
		xsXtpRequestSetParamsValue(pReq, objParam);
		xvoUnref(objParam);

		iCallOK = xsXtpClientCallRequestOK(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			pReq,
			3000u
		);
		iCallStatus = xsXtpClientCallRequestStatus(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			pReq,
			3000u,
			-1
		);
		sCmdDup = xsXtpClientCallRequestCmd(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			pReq,
			3000u,
			""
		);
		xsXtpRequestFree(pReq);
		if ( sCmdDup == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call request status failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		snprintf(
			sInnerBody,
			sizeof(sInnerBody),
			"request status ok=%s\nstatus=%d\ncmd=%s\n",
			iCallOK ? "true" : "false",
			iCallStatus,
			sCmdDup
		);
		xrtFree(sCmdDup);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callresultjson") ) {
		char* sResultJson;

		CHECK_XTP_PORT();

		sResultJson = xsXtpClientCallSimpleResultJson(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd,
			3000u
		);
		if ( sResultJson == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call result json failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		iWrite = xsXtpReplyOKJson(pStream, objMsg, "xtp.reply", sResultJson) != 0;
		xrtFree(sResultJson);
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callerrorjson") ) {
		char* sErrorJson;

		CHECK_XTP_PORT();

		sErrorJson = xsXtpClientCallSimpleErrorJson(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd && sInnerCmd[0] ? sInnerCmd : "demo.fail",
			3000u,
			"(none)"
		);
		if ( sErrorJson == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call error json failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		iWrite = xsXtpReplyOKJson(pStream, objMsg, "xtp.reply", sErrorJson) != 0;
		xrtFree(sErrorJson);
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callerror") ) {
		char* sError;

		CHECK_XTP_PORT();

		objResp = (XTP_MessageObject)xsXtpClientCallSimple(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd && sInnerCmd[0] ? sInnerCmd : "demo.fail",
			3000u
		);
		if ( objResp == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call error failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		sError = xsXtpBodyDup(objResp, "");
		snprintf(
			sInnerBody,
			sizeof(sInnerBody),
			"remote error captured\nok=%s\nstatus=%d\ncmd=%.*s\nresult=%s\nerror=%s\n",
			xsXtpIsOK(objResp) ? "true" : "false",
			xsXtpStatus(objResp),
			(int)xsXtpCmdLen(objResp),
			xsXtpCmd(objResp) ? xsXtpCmd(objResp) : "",
			xsXtpResultText(objResp, ""),
			sError ? sError : ""
		);
		if ( sError ) {
			xrtFree(sError);
		}
		xsXtpMessageFree(objResp);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callerrorbody") ) {
		char* sError;

		CHECK_XTP_PORT();

		sError = xsXtpClientCallSimpleError(
			sHost,
			iPort,
			1048576u,
			3000u,
			(iMsgID == 0) ? 1u : (iMsgID + 1u),
			sInnerCmd && sInnerCmd[0] ? sInnerCmd : "demo.fail",
			3000u,
			"(none)"
		);
		if ( sError == NULL ) {
			snprintf(
				sInnerBody,
				sizeof(sInnerBody),
				"client call error body failed\ncode=%d\nerror=%s\n",
				xsXtpClientLastErrorCode(),
				xsXtpClientLastError() ? xsXtpClientLastError() : ""
			);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", sInnerBody) != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		snprintf(
			sInnerBody,
			sizeof(sInnerBody),
			"remote error only\nerror=%s\n",
			sError
		);
		xrtFree(sError);
		iWrite = xsXtpReplyOKText(pStream, objMsg, "xtp.reply", sInnerBody) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	if ( xsXtpCmdIs(objMsg, "demo.callself") ) {
		XTP_CallSelfTask* pTask;
		xthread hThread;

		iPort = procParsePort(xsServerAddr(objServer));
		if ( iPort == 0 ) {
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "invalid self addr") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		pTask = (XTP_CallSelfTask*)xrtCalloc(1, sizeof(XTP_CallSelfTask));
		if ( pTask == NULL ) {
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "callself task alloc failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		pTask->pStream = pStream;
		pTask->pCloseFuture = xrtNetStreamCloseFuture((xnetstream*)pStream);
		if ( pTask->pCloseFuture == NULL ) {
			xrtFree(pTask);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "callself close future create failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}
		pTask->iMsgID = iMsgID;
		pTask->iPort = iPort;
		pTask->sTag = procDupText((sTag && sTag[0]) ? sTag : "self");
		if ( pTask->sTag == NULL ) {
			xFutureRelease(pTask->pCloseFuture);
			xrtFree(pTask);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "callself tag alloc failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}

		hThread = xrtThreadCreate((ptr)procCallSelfThread, pTask, 0);
		if ( hThread == NULL ) {
			if ( pTask->sTag ) {
				xrtFree(pTask->sTag);
			}
			xFutureRelease(pTask->pCloseFuture);
			xrtFree(pTask);
			iWrite = xsXtpReplyErrorText(pStream, objMsg, 500, "xtp.error", "callself thread create failed") != 0;
			FREE_XTP_DUP();
			return iWrite;
		}
		hThread->bAutoDestroy = TRUE;
		FREE_XTP_DUP();
		return TRUE;
	}

	if ( !xsXtpCmdIs(objMsg, "demo.ping") ) {
		iWrite = xsXtpReplyUnsupportedCmd(pStream, objMsg, sCmdText) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}
	if ( !xsXtpHasParam(objMsg, "tag") ) {
		iWrite = xsXtpReplyMissingParam(pStream, objMsg, "tag") != 0;
		FREE_XTP_DUP();
		return iWrite;
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
		"xtp demo\nserver=%s\nmsg_type=%u\nmsg_id=%llu\nstatus=%u\nseq=%lld\ndry=%s\ncmd=%.*s\nbody=%.*s\n",
		xsServerName(objServer),
		iMsgType,
		(unsigned long long)iMsgID,
		iStatus,
		(long long)iSeq,
		iDry ? "true" : "false",
		(int)iCmdLen,
		pCmd ? pCmd : "",
		(int)iBodyLen,
		pBody ? (const char*)pBody : ""
	);
	if ( iWrite < 0 ) {
		return false;
	}
	
	if ( !xsXtpIsRequest(objMsg) ) {
		FREE_XTP_DUP();
		return true;
	}
	if ( !xsXtpNeedReply(objMsg) ) {
		FREE_XTP_DUP();
		return true;
	}
	
	if ( iDry ) {
		snprintf(
			sJson,
			sizeof(sJson),
			"{\"result\":\"ok\",\"dry\":true,\"tag\":\"%s\"}",
			sTag ? sTag : ""
		);
		iWrite = xsXtpReplyOKJson(
			pStream,
			objMsg,
			"xtp.reply",
			sJson
		) != 0;
		FREE_XTP_DUP();
		return iWrite;
	}

	iWrite = xsXtpReplyText(
		pStream,
		objMsg,
		0,
		"xtp.reply",
		3,
		arrParam,
		arrValue,
		sBody
	) != 0;
	FREE_XTP_DUP();
	return iWrite;
#undef FREE_XTP_DUP
}
