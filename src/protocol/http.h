#ifndef XS_PROTOCOL_HTTP_H
#define XS_PROTOCOL_HTTP_H

typedef struct {
	XS_ServerConfig* pServer;
	xhttpdconn* pConn;
	xnetstream* pStream;
	int64 iLastActiveMS;
	volatile bool bClosing;
	ptr pTracker;
} XS_HttpConnContext;

typedef struct {
	xhttpdserver* pServer;
	xhttpdserver* pServerTLS;
	XS_ServerConfig* pOwner;
	xthread hIdleThread;
	volatile bool bStopThread;
	volatile bool bStopping;
	xmutex pConnLock;
	xarray arrConn;
} XS_HttpHandle;

static inline void XS_HttpOnCloseMetrics(void);

static inline bool XS_HttpHostEqualsToken(const char* sHostValue, const char* sToken)
{
	size_t iHostLen;
	size_t iTokenLen;
	size_t i;
	
	if ( sHostValue == NULL || sToken == NULL ) {
		return FALSE;
	}
	
	iHostLen = strlen(sHostValue);
	iTokenLen = strlen(sToken);
	
	if ( iHostLen == 0 || iTokenLen == 0 ) {
		return FALSE;
	}
	
	if ( iHostLen != iTokenLen ) {
		return FALSE;
	}
	
	for ( i = 0; i < iHostLen; i++ ) {
		if ( tolower((unsigned char)sHostValue[i]) != tolower((unsigned char)sToken[i]) ) {
			return FALSE;
		}
	}
	
	return TRUE;
}

static inline const XS_HostConfig* XS_HttpLocateHost(XS_ServerConfig* objServer, const xhttpdrequest* pReq)
{
	const char* sHostHeader;
	char sHostOnly[256];
	const char* pColon;
	uint32 i;
	
	if ( objServer == NULL ) {
		return NULL;
	}
	
	sHostHeader = xrtHttpdRequestHeader(pReq, "Host");
	if ( sHostHeader == NULL || sHostHeader[0] == '\0' ) {
		return objServer->EnableDefaultHost ? &objServer->DefaultHost : NULL;
	}
	
	pColon = strchr(sHostHeader, ':');
	if ( pColon == NULL ) {
		strncpy(sHostOnly, sHostHeader, sizeof(sHostOnly) - 1);
		sHostOnly[sizeof(sHostOnly) - 1] = '\0';
	} else {
		size_t iLen = (size_t)(pColon - sHostHeader);
		if ( iLen >= sizeof(sHostOnly) ) {
			iLen = sizeof(sHostOnly) - 1;
		}
		memcpy(sHostOnly, sHostHeader, iLen);
		sHostOnly[iLen] = '\0';
	}
	
	for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
		XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
		char* sHosts;
		char* sCursor;
		
		if ( !objHost->Enabled || objHost->Host == NULL || objHost->Host[0] == '\0' ) {
			continue;
		}
		
		sHosts = xrtCopyStr(objHost->Host, 0);
		if ( sHosts == NULL ) {
			continue;
		}
		
		sCursor = strtok(sHosts, ";");
		while ( sCursor ) {
			while ( *sCursor == ' ' || *sCursor == '\t' ) sCursor++;
			if ( XS_HttpHostEqualsToken(sHostOnly, sCursor) ) {
				xrtFree(sHosts);
				return objHost;
			}
			sCursor = strtok(NULL, ";");
		}
		
		xrtFree(sHosts);
	}
	
	return objServer->EnableDefaultHost ? &objServer->DefaultHost : NULL;
}

static inline const char* XS_HttpMimeTypeByPath(const char* sPath)
{
	const char* sExt;
	
	if ( sPath == NULL ) {
		return "application/octet-stream";
	}
	
	sExt = strrchr(sPath, '.');
	if ( sExt == NULL ) {
		return "application/octet-stream";
	}
	
	if ( strcasecmp(sExt, ".html") == 0 || strcasecmp(sExt, ".htm") == 0 ) return "text/html; charset=utf-8";
	if ( strcasecmp(sExt, ".css") == 0 ) return "text/css; charset=utf-8";
	if ( strcasecmp(sExt, ".js") == 0 ) return "application/javascript; charset=utf-8";
	if ( strcasecmp(sExt, ".json") == 0 ) return "application/json; charset=utf-8";
	if ( strcasecmp(sExt, ".txt") == 0 ) return "text/plain; charset=utf-8";
	if ( strcasecmp(sExt, ".svg") == 0 ) return "image/svg+xml";
	if ( strcasecmp(sExt, ".png") == 0 ) return "image/png";
	if ( strcasecmp(sExt, ".jpg") == 0 || strcasecmp(sExt, ".jpeg") == 0 ) return "image/jpeg";
	if ( strcasecmp(sExt, ".gif") == 0 ) return "image/gif";
	if ( strcasecmp(sExt, ".woff") == 0 ) return "font/woff";
	if ( strcasecmp(sExt, ".woff2") == 0 ) return "font/woff2";
	if ( strcasecmp(sExt, ".ttf") == 0 ) return "font/ttf";
	if ( strcasecmp(sExt, ".eot") == 0 ) return "application/vnd.ms-fontobject";
	
	return "application/octet-stream";
}

static inline bool XS_HttpRespondText(xhttpdresponse* pResp, uint32 iStatus, const char* sReason, const char* sText)
{
	if ( pResp == NULL || sReason == NULL || sText == NULL ) {
		return FALSE;
	}
	
	xrtHttpdResponseSetStatus(pResp, iStatus, sReason);
	return xrtHttpdResponseSetBodyCopy(pResp, sText, strlen(sText), "text/plain; charset=utf-8");
}

static inline bool XS_HttpLooksLikeJson(const char* sText)
{
	size_t iStart;
	size_t iEnd;

	if ( sText == NULL ) {
		return FALSE;
	}

	for ( iStart = 0; sText[iStart] != '\0'; iStart++ ) {
		if ( sText[iStart] != ' ' && sText[iStart] != '\t' && sText[iStart] != '\r' && sText[iStart] != '\n' ) {
			break;
		}
	}
	if ( sText[iStart] == '\0' ) {
		return FALSE;
	}

	iEnd = strlen(sText);
	while ( iEnd > iStart ) {
		char ch = sText[iEnd - 1];
		if ( ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n' ) {
			break;
		}
		iEnd--;
	}
	if ( iEnd <= iStart ) {
		return FALSE;
	}

	if ( sText[iStart] == '{' ) {
		return sText[iEnd - 1] == '}';
	}
	if ( sText[iStart] == '[' ) {
		return sText[iEnd - 1] == ']';
	}

	return FALSE;
}

static inline bool XS_HttpExtractLineValue(const char* sText, const char* sName, char* sBuf, size_t iBufCap)
{
	const char* pLine;
	const char* pValue;
	const char* pEnd;
	size_t iNameLen;
	size_t iCopy;

	if ( sBuf && iBufCap > 0 ) {
		sBuf[0] = '\0';
	}
	if ( sText == NULL || sName == NULL || sName[0] == '\0' || sBuf == NULL || iBufCap <= 1 ) {
		return FALSE;
	}

	iNameLen = strlen(sName);
	pLine = sText;
	while ( pLine && *pLine ) {
		while ( *pLine == '\r' || *pLine == '\n' ) {
			pLine++;
		}
		if ( strncmp(pLine, sName, iNameLen) == 0 && pLine[iNameLen] == '=' ) {
			pValue = pLine + iNameLen + 1;
			pEnd = pValue;
			while ( *pEnd != '\0' && *pEnd != '\r' && *pEnd != '\n' ) {
				pEnd++;
			}
			iCopy = (size_t)(pEnd - pValue);
			if ( iCopy >= iBufCap ) {
				iCopy = iBufCap - 1;
			}
			memcpy(sBuf, pValue, iCopy);
			sBuf[iCopy] = '\0';
			return TRUE;
		}
		while ( *pLine != '\0' && *pLine != '\n' ) {
			pLine++;
		}
		if ( *pLine == '\n' ) {
			pLine++;
		}
	}

	return FALSE;
}

static inline bool XS_HttpExtractJsonMessage(const char* sText, char* sBuf, size_t iBufCap)
{
	const char* pKey;
	const char* pValue;
	const char* pEnd;
	size_t iCopy;

	if ( sBuf && iBufCap > 0 ) {
		sBuf[0] = '\0';
	}
	if ( sText == NULL || sBuf == NULL || iBufCap <= 1 ) {
		return FALSE;
	}

	pKey = strstr(sText, "\"message\"");
	while ( pKey ) {
		pValue = pKey + strlen("\"message\"");
		while ( *pValue == ' ' || *pValue == '\t' || *pValue == '\r' || *pValue == '\n' ) {
			pValue++;
		}
		if ( *pValue != ':' ) {
			pKey = strstr(pValue, "\"message\"");
			continue;
		}
		pValue++;
		while ( *pValue == ' ' || *pValue == '\t' || *pValue == '\r' || *pValue == '\n' ) {
			pValue++;
		}
		if ( *pValue != '"' ) {
			return FALSE;
		}
		pValue++;
		pEnd = pValue;
		while ( *pEnd != '\0' ) {
			if ( *pEnd == '"' && (pEnd == pValue || pEnd[-1] != '\\') ) {
				break;
			}
			pEnd++;
		}
		if ( *pEnd != '"' ) {
			return FALSE;
		}
		iCopy = (size_t)(pEnd - pValue);
		if ( iCopy >= iBufCap ) {
			iCopy = iBufCap - 1;
		}
		memcpy(sBuf, pValue, iCopy);
		sBuf[iCopy] = '\0';
		return TRUE;
	}

	return FALSE;
}

static inline bool XS_HttpExtractResponseMessage(const xhttpdresponse* pResp, char* sBuf, size_t iBufCap)
{
	if ( sBuf && iBufCap > 0 ) {
		sBuf[0] = '\0';
	}
	if ( pResp == NULL || pResp->pBody == NULL || pResp->iBodyLen == 0 ) {
		return FALSE;
	}

	if ( XS_HttpLooksLikeJson(pResp->pBody) ) {
		return XS_HttpExtractJsonMessage(pResp->pBody, sBuf, iBufCap);
	}

	return XS_HttpExtractLineValue(pResp->pBody, "message", sBuf, iBufCap);
}

#include "../manage/http_runtime.h"

#include "../manage/http_manage.h"

static void XS_HttpOnOpen(ptr pOwner, xhttpdserver* pServer, xhttpdconn* pConn)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HttpHandle* objHandle = objServer ? (XS_HttpHandle*)objServer->pHandle : NULL;
	XS_HttpConnContext* objCtx = NULL;
	(void)pServer;

	if ( pConn && pConn->pStream ) {
		objCtx = (XS_HttpConnContext*)xrtCalloc(1, sizeof(XS_HttpConnContext));
		if ( objCtx ) {
			objCtx->pServer = objServer;
			objCtx->pConn = pConn;
			objCtx->pStream = pConn->pStream;
			objCtx->pTracker = objHandle;
			XS_HttpTouch(objCtx);
			XS_HttpTrackConn(objHandle, objCtx);
		}
	}

	if ( objHandle && objHandle->bStopping ) {
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		XS_HttpRecordRemoteByConn(pConn);
		XS_HttpRecordRejectEvent(503, "server_stopping");
		XS_LogWarn(
			"http open rejected while stopping: server=%s remote=%s",
			objServer && objServer->Name ? objServer->Name : "(null)",
			XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : "(none)"
		);
		if ( pConn && pConn->pStream ) {
			xrtNetStreamClose(pConn->pStream, XNET_CLOSE_F_ABORT);
		}
		return;
	}

	if ( XS_RuntimeStatsEnabled() ) {
		XS_HttpOnOpenMetrics();
	}
	if ( XS_RuntimeGovernEnabled() && objServer && objServer->ConnLimit > 0u && XS_HttpTrackedConnCount(objHandle) > (int64)objServer->ConnLimit ) {
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		XS_HttpRecordRemoteByConn(pConn);
		XS_HttpRecordConnLimitClose();
		XS_LogWarn(
			"http conn limit exceeded: server=%s current=%lld limit=%u remote=%s",
			objServer->Name ? objServer->Name : "(null)",
			(long long)XS_HttpTrackedConnCount(objHandle),
			(unsigned)objServer->ConnLimit,
			XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : "(none)"
		);
		if ( pConn && pConn->pStream ) {
			xrtNetStreamClose(pConn->pStream, 0u);
		}
		return;
	}
	
	XS_HttpRecordRemoteByConn(pConn);
	XS_LogInfo(
		"http open: server=%s remote=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : "(none)"
	);
}

static bool XS_HttpOnRequest(ptr pOwner, xhttpdserver* pServer, xhttpdconn* pConn, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	const XS_HostConfig* objHost;
	char sBody[512];
	bool bRet;
	double fStartTick;
	double fEndTick;
	double fElapsed;
	int64 iElapsedMS;
	XS_HttpConnContext* objCtx;
	XS_HttpHandle* objHandle;
	(void)pServer;
	
	if ( pReq == NULL || pResp == NULL ) {
		return FALSE;
	}

	bRet = FALSE;
	fStartTick = xrtTimer();
	objHandle = objServer ? (XS_HttpHandle*)objServer->pHandle : NULL;
	objCtx = XS_HttpGetConnContext(objHandle, pConn);
	XS_HttpTouch(objCtx);
	if ( XS_RuntimeStatsEnabled() ) {
		XS_HttpRecordMethodMetrics(pReq);
	}

	XS_HttpApplyDefaultHeaders(pReq, pResp);

	if ( objHandle && objHandle->bStopping ) {
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		XS_HttpRecordRemoteByConn(pConn);
		XS_HttpRecordRejectEvent(503, "server_stopping");
		XS_LogWarn(
			"http request rejected while stopping: server=%s method=%s path=%s remote=%s",
			objServer && objServer->Name ? objServer->Name : "(null)",
			pReq->sMethod ? pReq->sMethod : "(null)",
			pReq->sPath ? pReq->sPath : "(null)",
			XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : "(none)"
		);
		if ( XS_HttpPathExpectsJSONError(pReq->sPath) ) {
			bRet = XS_HttpRespondJsonResult(pResp, 503, "Service Unavailable", FALSE, "server stopping");
		} else {
			bRet = XS_HttpRetErrorEx(
				objServer,
				NULL,
				pReq,
				pResp,
				XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : NULL,
				503,
				"Service Unavailable",
				"server stopping",
				NULL
			);
		}
		goto end;
	}
	
	objHost = XS_HttpLocateHost(objServer, pReq);
	if ( objHost == NULL ) {
		if ( XS_HttpPathExpectsJSONError(pReq->sPath) ) {
			bRet = XS_HttpRespondJsonResult(pResp, 404, "Not Found", FALSE, "host not found");
		} else {
			bRet = XS_HttpRet404Ex(objServer, NULL, pReq, pResp, NULL, "host not found", NULL);
		}
		goto end;
	}

	if ( XS_HttpTryManageRequest(objServer, objHost, pReq, pResp, pConn) ) {
		bRet = TRUE;
		goto end;
	}

	XS_HttpLogRequest(objServer, objHost, pReq, pConn);
	
	if ( objHost->DevMode == XS_DEV_STATIC ) {
		bRet = XS_HttpServeStatic(objHost, pReq, pResp);
		goto end;
	}
	
	if ( objHost->DevMode == XS_DEV_SCRIPT_C ) {
		if ( XS_HttpHandleScriptHost(objServer, objHost, pReq, pResp, pConn) ) {
			bRet = TRUE;
			goto end;
		}
		
		snprintf(
			sBody,
			sizeof(sBody),
			"script host did not handle request\nserver=%s\nhost=%s\npath=%s\n",
			objServer && objServer->Name ? objServer->Name : "(null)",
			objHost->Name ? objHost->Name : "(default)",
			pReq->sPath
		);
		bRet = XS_HttpRet404Ex(objServer, objHost, pReq, pResp, XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : NULL, sBody, NULL);
		goto end;
	}
	
	snprintf(
		sBody,
		sizeof(sBody),
		"xserver vNext http skeleton\nserver=%s\nhost=%s\npath=%s\n",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost->Name ? objHost->Name : "(default)",
		pReq->sPath
	);
	bRet = XS_HttpRespondText(pResp, 200, "OK", sBody);

end:
	fEndTick = xrtTimer();
	fElapsed = fEndTick - fStartTick;
	if ( fElapsed < 0.0 ) {
		fElapsed = 0.0;
	}
	iElapsedMS = (int64)(fElapsed * 1000.0);
	if ( XS_RuntimeStatsEnabled() ) {
		XS_HttpRecordTimeMetrics(pReq, iElapsedMS);
		XS_HttpRecordResponseMetrics(pConn, pReq, pResp);
	}
	return bRet;
}

static void XS_HttpOnClose(ptr pOwner, xhttpdserver* pServer, xhttpdconn* pConn, xnet_result iReason)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HttpHandle* objHandle = objServer ? (XS_HttpHandle*)objServer->pHandle : NULL;
	XS_HttpConnContext* objCtx = XS_HttpGetConnContext(objHandle, pConn);
	(void)pServer;

	if ( XS_RuntimeStatsEnabled() ) {
		XS_HttpOnCloseMetrics();
	}
	XS_HttpRecordRemoteByConn(pConn);

	if ( objCtx ) {
		XS_HttpUntrackConn((XS_HttpHandle*)objCtx->pTracker, objCtx);
		xrtFree(objCtx);
	}
	
	XS_LogInfo(
		"http close: server=%s reason=%d remote=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		(int)iReason,
		XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : "(none)"
	);
}

static void XS_HttpOnError(ptr pOwner, xhttpdserver* pServer, xhttpdconn* pConn, int iSysErr)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HttpHandle* objHandle = objServer ? (XS_HttpHandle*)objServer->pHandle : NULL;
	XS_HttpConnContext* objCtx = XS_HttpGetConnContext(objHandle, pConn);
	(void)pServer;

	if ( objCtx && objCtx->bClosing ) {
		return;
	}
	if ( iSysErr == -1 ) {
		return;
	}
	XS_HttpRecordRemoteByConn(pConn);
	
	XS_LogWarn(
		"http error: server=%s sys=%d remote=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		iSysErr,
		XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : "(none)"
	);
}

static inline bool XS_ParseBindAddr(const char* sAddr, xnetaddr* pAddr)
{
	char sHost[256];
	const char* pWork = sAddr;
	const char* pColon;
	size_t iHostLen;
	uint16 iPort;
	
	if ( sAddr == NULL || sAddr[0] == '\0' || pAddr == NULL ) {
		return FALSE;
	}
	
	if ( strncmp(pWork, "http://", 7) == 0 ) {
		pWork += 7;
	} else if ( strncmp(pWork, "https://", 8) == 0 ) {
		pWork += 8;
	} else if ( strncmp(pWork, "ws://", 5) == 0 ) {
		pWork += 5;
	} else if ( strncmp(pWork, "wss://", 6) == 0 ) {
		pWork += 6;
	} else if ( strncmp(pWork, "tcp://", 6) == 0 ) {
		pWork += 6;
	}
	
	pColon = strrchr(pWork, ':');
	if ( pColon == NULL ) {
		return FALSE;
	}
	
	iHostLen = (size_t)(pColon - pWork);
	if ( iHostLen == 0 ) {
		strcpy(sHost, "0.0.0.0");
	} else {
		if ( iHostLen >= sizeof(sHost) ) {
			iHostLen = sizeof(sHost) - 1;
		}
		memcpy(sHost, pWork, iHostLen);
		sHost[iHostLen] = '\0';
	}
	
	if ( !XS_ParsePortText(pColon + 1, &iPort) ) {
		return FALSE;
	}
	if ( xrtNetAddrParse(pAddr, sHost, iPort) != XRT_NET_OK ) {
		return FALSE;
	}
	
	return TRUE;
}

static inline bool XS_BuildBindAddr(const XS_ServerConfig* objServer, bool bTLS, xnetaddr* pAddr)
{
	const char* sIP;
	uint16 iPort;
	const char* sLegacyAddr;
	
	if ( objServer == NULL || pAddr == NULL ) {
		return FALSE;
	}
	
	if ( bTLS ) {
		sIP = objServer->BindIPTLS;
		iPort = objServer->BindPortTLS;
		sLegacyAddr = objServer->AddrTLS;
	} else {
		sIP = objServer->BindIP;
		iPort = objServer->BindPort;
		sLegacyAddr = objServer->Addr;
	}
	
	if ( sIP && sIP[0] != '\0' && iPort > 0 ) {
		return xrtNetAddrParse(pAddr, sIP, iPort) == XRT_NET_OK;
	}
	
	return XS_ParseBindAddr(sLegacyAddr, pAddr);
}

static inline void XS_HttpInitEvents(xhttpdevents* pEvents)
{
	memset(pEvents, 0, sizeof(xhttpdevents));
	pEvents->OnOpen = XS_HttpOnOpen;
	pEvents->OnRequest = XS_HttpOnRequest;
	pEvents->OnClose = XS_HttpOnClose;
	pEvents->OnError = XS_HttpOnError;
}

static inline bool XS_HttpBuildRuntimeConfigEx(XS_ServerConfig* objServer, bool bTLS, xhttpdconfig* pCfg)
{
	if ( objServer == NULL || pCfg == NULL ) {
		return FALSE;
	}

	xrtHttpdConfigInit(pCfg);
	if ( bTLS ) {
		if ( !objServer->EnableTLS ) {
			return FALSE;
		}
		if ( objServer->BindPortTLS == 0 ) {
			XS_ReportError("http tls init failed: missing port_tls server=%s", objServer->Name ? objServer->Name : "(null)");
			return FALSE;
		}
		if ( objServer->TlsConfig.sCertFile == NULL || objServer->TlsConfig.sKeyFile == NULL ) {
			XS_ReportError("http tls init failed: missing tls cert/key server=%s", objServer->Name ? objServer->Name : "(null)");
			return FALSE;
		}
	}
	if ( !XS_BuildBindAddr(objServer, bTLS, &pCfg->tBindAddr) ) {
		return FALSE;
	}
	pCfg->iBacklog = objServer->Backlog;
	pCfg->iRecvLimit = objServer->RecvLimit;
	if ( bTLS ) {
		pCfg->pTlsConfig = &objServer->TlsConfig;
	}

	return TRUE;
}

static inline xhttpdserver* XS_HttpCreateListener(xnetengine* pEngine, XS_ServerConfig* objServer, bool bTLS)
{
	xhttpdconfig tConfig;
	xhttpdevents tEvents;

	if ( pEngine == NULL || objServer == NULL ) {
		return NULL;
	}
	if ( !XS_HttpBuildRuntimeConfigEx(objServer, bTLS, &tConfig) ) {
		XS_ReportError(
			bTLS ? "http tls init failed: invalid addr: %s" : "http init failed: invalid addr: %s",
			bTLS ? (objServer->AddrTLS ? objServer->AddrTLS : "(null)") : (objServer->Addr ? objServer->Addr : "(null)")
		);
		return NULL;
	}

	XS_HttpInitEvents(&tEvents);
	return xrtHttpdCreate(pEngine, &tConfig, &tEvents, objServer);
}

static inline bool XS_HttpInitServer(xnetengine* pEngine, XS_ServerConfig* objServer)
{
	xhttpdserver* pServer;
	xhttpdserver* pServerTLS;
	XS_HttpHandle* objHandle;
	
	if ( objServer == NULL ) {
		XS_ReportError("http init failed: server is null");
		return FALSE;
	}
	if ( pEngine == NULL ) {
		XS_ReportError("http init failed: runtime engine is null");
		return FALSE;
	}
	
	XS_LogInfo(
		"http init: name=%s addr=%s default_host=%s hosts=%u",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)",
		objServer->EnableDefaultHost ? "true" : "false",
		objServer->Hosts ? objServer->Hosts->Count : 0
	);
	
	if ( objServer->EnableDefaultHost ) {
		XS_LogInfo(
			"http default host: name=%s path=%s dev=%s",
			objServer->DefaultHost.Name ? objServer->DefaultHost.Name : "(null)",
			objServer->DefaultHost.Path ? objServer->DefaultHost.Path : "(null)",
			XS_DevModeName(objServer->DefaultHost.DevMode)
		);
	}
	
	pServer = XS_HttpCreateListener(pEngine, objServer, FALSE);
	if ( pServer == NULL ) {
		XS_ReportError("http init failed: xrtHttpdCreate returned null");
		return FALSE;
	}

	pServerTLS = NULL;
	if ( objServer->EnableTLS ) {
		pServerTLS = XS_HttpCreateListener(pEngine, objServer, TRUE);
		if ( pServerTLS == NULL ) {
			xrtHttpdDestroy(pServer);
			XS_ReportError("http tls init failed: xrtHttpdCreate returned null");
			return FALSE;
		}
	}
	
	objHandle = (XS_HttpHandle*)xrtCalloc(1, sizeof(XS_HttpHandle));
	if ( objHandle == NULL ) {
		if ( pServerTLS ) {
			xrtHttpdDestroy(pServerTLS);
		}
		xrtHttpdDestroy(pServer);
		XS_ReportError("http init failed: handle alloc failed");
		return FALSE;
	}
	objHandle->pServer = pServer;
	objHandle->pServerTLS = pServerTLS;
	objHandle->pOwner = objServer;
	objHandle->pConnLock = xrtMutexCreate();
	objHandle->arrConn = xrtArrayCreate(sizeof(XS_HttpConnContext*), XRT_OBJMODE_SHARED);
	if ( objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		if ( objHandle->arrConn ) {
			xrtArrayDestroy(objHandle->arrConn);
		}
		if ( objHandle->pConnLock ) {
			xrtMutexDestroy(objHandle->pConnLock);
		}
		if ( pServerTLS ) {
			xrtHttpdDestroy(pServerTLS);
		}
		xrtHttpdDestroy(pServer);
		xrtFree(objHandle);
		XS_ReportError("http init failed: handle resource alloc failed");
		return FALSE;
	}

	objServer->pHandle = objHandle;
	
	return TRUE;
}

static inline bool XS_HttpStartServer(XS_ServerConfig* objServer)
{
	XS_HttpHandle* objHandle;
	xhttpdserver* pServer;
	xhttpdserver* pServerTLS;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	objHandle = (XS_HttpHandle*)objServer->pHandle;
	pServer = objHandle ? objHandle->pServer : NULL;
	pServerTLS = objHandle ? objHandle->pServerTLS : NULL;
	if ( pServer == NULL ) {
		XS_ReportError("http start failed: server handle is null");
		return FALSE;
	}
	if ( xrtHttpdStart(pServer) != XRT_NET_OK ) {
		XS_ReportError("http start failed: xrtHttpdStart returned error");
		return FALSE;
	}
	if ( pServerTLS ) {
		if ( xrtHttpdStart(pServerTLS) != XRT_NET_OK ) {
			xrtHttpdStop(pServer);
			XS_ReportError("http tls start failed: xrtHttpdStart returned error");
			return FALSE;
		}
	}
	if ( objHandle && objServer->IdleTimeout > 0 ) {
		objHandle->bStopping = FALSE;
		objHandle->bStopThread = FALSE;
		objHandle->hIdleThread = xrtThreadCreate(XS_HttpIdleThread, objHandle, 0);
		if ( objHandle->hIdleThread == NULL ) {
			if ( pServerTLS ) {
				xrtHttpdStop(pServerTLS);
			}
			xrtHttpdStop(pServer);
			XS_ReportError("http start failed: idle thread create failed");
			return FALSE;
		}
	} else if ( objHandle ) {
		objHandle->bStopping = FALSE;
	}
	
	XS_LogInfo(
		"http start: server=%s addr=%s bound_port=%u",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)",
		(unsigned)xrtHttpdBoundPort(pServer)
	);
	if ( pServerTLS ) {
		XS_LogInfo(
			"http tls start: server=%s addr=%s bound_port=%u",
			objServer->Name ? objServer->Name : "(null)",
			objServer->AddrTLS ? objServer->AddrTLS : "(null)",
			(unsigned)xrtHttpdBoundPort(pServerTLS)
		);
	}
	
	return TRUE;
}

static inline void XS_HttpStopServer(XS_ServerConfig* objServer)
{
	XS_HttpHandle* objHandle;
	xhttpdserver* pServer;
	xhttpdserver* pServerTLS;
	int64 iClosedConn;
	int64 iRemainConn;
	
	if ( objServer == NULL ) {
		return;
	}
	
	objHandle = (XS_HttpHandle*)objServer->pHandle;
	pServer = objHandle ? objHandle->pServer : NULL;
	pServerTLS = objHandle ? objHandle->pServerTLS : NULL;
	if ( objHandle ) {
		objHandle->bStopping = TRUE;
		objHandle->bStopThread = TRUE;
		if ( objHandle->hIdleThread ) {
			xrtThreadWait(objHandle->hIdleThread);
			xrtThreadDestroy(objHandle->hIdleThread);
			objHandle->hIdleThread = NULL;
		}
	}
	if ( pServer && pServer->pListener ) {
		xrtNetListenerStop(pServer->pListener);
		xrtNetListenerDestroy(pServer->pListener);
		pServer->pListener = NULL;
	}
	if ( pServerTLS && pServerTLS->pListener ) {
		xrtNetListenerStop(pServerTLS->pListener);
		xrtNetListenerDestroy(pServerTLS->pListener);
		pServerTLS->pListener = NULL;
	}
	iClosedConn = XS_HttpCloseTrackedConns(objHandle);
	iRemainConn = XS_HttpWaitTrackedConnDrain(objHandle, 500u);
	if ( iRemainConn > 0 ) {
		XS_HttpAbortTrackedConns(objHandle);
		iRemainConn = XS_HttpWaitTrackedConnDrain(objHandle, 1000u);
	}
	if ( iRemainConn > 0 ) {
		XS_HttpFinalizeTrackedConns(objHandle);
		iRemainConn = XS_HttpTrackedConnCount(objHandle);
	}
	if ( iClosedConn > 0 || iRemainConn > 0 ) {
		XS_HttpRecordStopCleanup(iClosedConn, iRemainConn);
		XS_LogInfo(
			"http stop cleanup: server=%s addr=%s closed=%lld remain=%lld",
			objServer->Name ? objServer->Name : "(null)",
			objServer->Addr ? objServer->Addr : "(null)",
			(long long)iClosedConn,
			(long long)iRemainConn
		);
	}
	if ( pServer ) {
		xrtHttpdDestroy(pServer);
	}
	if ( pServerTLS ) {
		xrtHttpdDestroy(pServerTLS);
	}
	if ( objHandle ) {
		if ( objHandle->arrConn ) {
			xrtArrayDestroy(objHandle->arrConn);
		}
		if ( objHandle->pConnLock ) {
			xrtMutexDestroy(objHandle->pConnLock);
		}
		xrtFree(objHandle);
	}
	objServer->pHandle = NULL;
	
	XS_LogInfo(
		"http stop: server=%s addr=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)"
	);
	if ( objServer->EnableTLS ) {
		XS_LogInfo(
			"http tls stop: server=%s addr=%s",
			objServer->Name ? objServer->Name : "(null)",
			objServer->AddrTLS ? objServer->AddrTLS : "(null)"
		);
	}
}

#endif
