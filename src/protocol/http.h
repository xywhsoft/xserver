#ifndef XS_PROTOCOL_HTTP_H
#define XS_PROTOCOL_HTTP_H

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
	
	if ( _stricmp(sExt, ".html") == 0 || _stricmp(sExt, ".htm") == 0 ) return "text/html; charset=utf-8";
	if ( _stricmp(sExt, ".css") == 0 ) return "text/css; charset=utf-8";
	if ( _stricmp(sExt, ".js") == 0 ) return "application/javascript; charset=utf-8";
	if ( _stricmp(sExt, ".json") == 0 ) return "application/json; charset=utf-8";
	if ( _stricmp(sExt, ".txt") == 0 ) return "text/plain; charset=utf-8";
	if ( _stricmp(sExt, ".svg") == 0 ) return "image/svg+xml";
	if ( _stricmp(sExt, ".png") == 0 ) return "image/png";
	if ( _stricmp(sExt, ".jpg") == 0 || _stricmp(sExt, ".jpeg") == 0 ) return "image/jpeg";
	if ( _stricmp(sExt, ".gif") == 0 ) return "image/gif";
	if ( _stricmp(sExt, ".woff") == 0 ) return "font/woff";
	if ( _stricmp(sExt, ".woff2") == 0 ) return "font/woff2";
	if ( _stricmp(sExt, ".ttf") == 0 ) return "font/ttf";
	if ( _stricmp(sExt, ".eot") == 0 ) return "application/vnd.ms-fontobject";
	
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

static inline void XS_HttpApplyDefaultHeaders(const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	if ( pResp == NULL ) {
		return;
	}

	(void)xrtHttpdResponseSetHeader(pResp, "X-Content-Type-Options", "nosniff");
	if ( pReq && strncmp(pReq->sPath, "/__xs/", 6) == 0 ) {
		(void)xrtHttpdResponseSetHeader(pResp, "Cache-Control", "no-store");
	}
}

static inline bool XS_HttpValidateRequest(XS_ServerConfig* objServer, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	size_t iPathLen;
	size_t iQueryLen;

	if ( objServer == NULL || pReq == NULL || pResp == NULL ) {
		return FALSE;
	}

	if ( objServer->HeaderLimit > 0u && pReq->iHeaderCount > objServer->HeaderLimit ) {
		return XS_HttpRespondText(pResp, 431, "Request Header Fields Too Large", "header count limit exceeded");
	}

	if ( objServer->BodyLimit > 0u && pReq->iBodyLen > (size_t)objServer->BodyLimit ) {
		return XS_HttpRespondText(pResp, 413, "Payload Too Large", "request body limit exceeded");
	}

	iPathLen = strlen(pReq->sPath);
	iQueryLen = strlen(pReq->sQuery);
	if ( objServer->PathLimit > 0u && (iPathLen + iQueryLen) > (size_t)objServer->PathLimit ) {
		return XS_HttpRespondText(pResp, 414, "URI Too Long", "request path limit exceeded");
	}

	return FALSE;
}

static inline bool XS_HttpStaticPathSensitive(const char* sRelPath)
{
	const char* sExt;
	const char* pSeg;

	if ( sRelPath == NULL || sRelPath[0] == '\0' ) {
		return TRUE;
	}

	for ( pSeg = sRelPath; *pSeg; pSeg++ ) {
		if ( *pSeg == '\\' ) {
			return TRUE;
		}
		if ( *pSeg == '.' && (pSeg == sRelPath || pSeg[-1] == '/') ) {
			return TRUE;
		}
	}

	sExt = strrchr(sRelPath, '.');
	if ( sExt == NULL ) {
		return FALSE;
	}

	if ( _stricmp(sExt, ".c") == 0 ) return TRUE;
	if ( _stricmp(sExt, ".h") == 0 ) return TRUE;
	if ( _stricmp(sExt, ".json") == 0 ) return TRUE;
	if ( _stricmp(sExt, ".db") == 0 ) return TRUE;
	if ( _stricmp(sExt, ".sqlite") == 0 ) return TRUE;
	if ( _stricmp(sExt, ".sqlite3") == 0 ) return TRUE;
	if ( _stricmp(sExt, ".pem") == 0 ) return TRUE;
	if ( _stricmp(sExt, ".key") == 0 ) return TRUE;
	if ( _stricmp(sExt, ".log") == 0 ) return TRUE;
	if ( _stricmp(sExt, ".bak") == 0 ) return TRUE;

	return FALSE;
}

static inline bool XS_HttpServeStatic(const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	const char* sReqPath;
	const char* sRelPath;
	char sLength[32];
	char* sFilePath;
	ptr pFileData;
	size_t iFileSize;
	const char* sMime;
	
	if ( objHost == NULL || pReq == NULL || pResp == NULL || objHost->Path == NULL ) {
		return FALSE;
	}

	if ( _stricmp(pReq->sMethod, "GET") != 0 && _stricmp(pReq->sMethod, "HEAD") != 0 ) {
		xrtHttpdResponseSetHeader(pResp, "Allow", "GET, HEAD");
		return XS_HttpRespondText(pResp, 405, "Method Not Allowed", "static host only supports GET or HEAD");
	}
	
	sReqPath = pReq->sPath[0] ? pReq->sPath : "/";
	if ( strstr(sReqPath, "..") != NULL ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "forbidden");
	}
	
	if ( strcmp(sReqPath, "/") == 0 ) {
		sRelPath = "index.html";
	} else if ( sReqPath[0] == '/' ) {
		sRelPath = sReqPath + 1;
	} else {
		sRelPath = sReqPath;
	}

	if ( XS_HttpStaticPathSensitive(sRelPath) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "static path denied");
	}
	
	sFilePath = xrtPathJoin(2, objHost->Path, sRelPath);
	if ( sFilePath == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "path join failed");
	}
	
	if ( !xrtFileExists(sFilePath) ) {
		xrtFree(sFilePath);
		return XS_HttpRespondText(pResp, 404, "Not Found", "file not found");
	}
	
	pFileData = xrtFileGetAll(sFilePath, &iFileSize);
	if ( pFileData == NULL ) {
		xrtFree(sFilePath);
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "file read failed");
	}
	
	sMime = XS_HttpMimeTypeByPath(sFilePath);
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( _stricmp(pReq->sMethod, "HEAD") == 0 ) {
		snprintf(sLength, sizeof(sLength), "%llu", (unsigned long long)iFileSize);
		(void)xrtHttpdResponseSetHeader(pResp, "Content-Type", sMime);
		(void)xrtHttpdResponseSetHeader(pResp, "Content-Length", sLength);
		xrtFree(pFileData);
		xrtFree(sFilePath);
		return TRUE;
	}
	if ( !xrtHttpdResponseSetBodyCopy(pResp, pFileData, iFileSize, sMime) ) {
		xrtFree(pFileData);
		xrtFree(sFilePath);
		return FALSE;
	}
	
	xrtFree(pFileData);
	xrtFree(sFilePath);
	return TRUE;
}

static inline bool XS_HttpHandleScriptHost(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	bool bHandled;
	XS_ScriptHttpRequestProc procRequest;
	
	procRequest = XS_GetHostHttpRequestProc((XS_HostConfig*)objHost);
	if ( procRequest == NULL ) {
		return FALSE;
	}
	
	bHandled = procRequest(objServer, (void*)objHost, pReq, pResp);
	if ( bHandled ) {
		return TRUE;
	}
	
	if ( objHost->Path && objHost->Path[0] ) {
		XS_LogInfo(
			"http fallback static: server=%s host=%s path=%s",
			objServer && objServer->Name ? objServer->Name : "(null)",
			objHost->Name ? objHost->Name : "(default)",
			pReq ? pReq->sPath : ""
		);
		return XS_HttpServeStatic(objHost, pReq, pResp);
	}
	
	return FALSE;
}

static inline bool XS_HttpQueryBool(const char* sQuery, const char* sName, bool bDefault)
{
	const char* sFind;
	size_t iNameLen;
	
	if ( sQuery == NULL || sName == NULL || sName[0] == '\0' ) {
		return bDefault;
	}
	
	iNameLen = strlen(sName);
	sFind = sQuery;
	while ( sFind && sFind[0] ) {
		const char* pNext = strchr(sFind, '&');
		size_t iPairLen = pNext ? (size_t)(pNext - sFind) : strlen(sFind);
		if ( iPairLen > iNameLen + 1 && strncmp(sFind, sName, iNameLen) == 0 && sFind[iNameLen] == '=' ) {
			const char* sVal = sFind + iNameLen + 1;
			size_t iValLen = iPairLen - iNameLen - 1;
			if ( iValLen == 1 && (sVal[0] == '1' || sVal[0] == 'y' || sVal[0] == 'Y')) return TRUE;
			if ( iValLen == 4 && _strnicmp(sVal, "true", 4) == 0 ) return TRUE;
			if ( iValLen == 3 && _strnicmp(sVal, "yes", 3) == 0 ) return TRUE;
			if ( iValLen == 1 && (sVal[0] == '0' || sVal[0] == 'n' || sVal[0] == 'N')) return FALSE;
			if ( iValLen == 5 && _strnicmp(sVal, "false", 5) == 0 ) return FALSE;
			if ( iValLen == 2 && _strnicmp(sVal, "no", 2) == 0 ) return FALSE;
			return bDefault;
		}
		sFind = pNext ? (pNext + 1) : NULL;
	}
	
	return bDefault;
}

static inline int XS_HttpHexValue(char ch)
{
	if ( ch >= '0' && ch <= '9' ) return ch - '0';
	if ( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
	if ( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
	return -1;
}

static inline char* XS_HttpUrlDecodeCopy(const char* sVal, size_t iLen)
{
	char* sRet;
	size_t iSrc;
	size_t iDst;
	
	sRet = (char*)xrtMalloc(iLen + 1);
	if ( sRet == NULL ) {
		return NULL;
	}
	
	iDst = 0;
	for ( iSrc = 0; iSrc < iLen; iSrc++ ) {
		if ( sVal[iSrc] == '+' ) {
			sRet[iDst++] = ' ';
			continue;
		}
		
		if ( sVal[iSrc] == '%' && (iSrc + 2) < iLen ) {
			int iHi = XS_HttpHexValue(sVal[iSrc + 1]);
			int iLo = XS_HttpHexValue(sVal[iSrc + 2]);
			if ( iHi >= 0 && iLo >= 0 ) {
				sRet[iDst++] = (char)((iHi << 4) | iLo);
				iSrc += 2;
				continue;
			}
		}
		
		sRet[iDst++] = sVal[iSrc];
	}
	
	sRet[iDst] = '\0';
	return sRet;
}

static inline char* XS_HttpQueryText(const char* sQuery, const char* sName)
{
	const char* sFind;
	size_t iNameLen;
	
	if ( sQuery == NULL || sName == NULL || sName[0] == '\0' ) {
		return NULL;
	}
	
	iNameLen = strlen(sName);
	sFind = sQuery;
	while ( sFind && sFind[0] ) {
		const char* pNext = strchr(sFind, '&');
		size_t iPairLen = pNext ? (size_t)(pNext - sFind) : strlen(sFind);
		if ( iPairLen > iNameLen + 1 && strncmp(sFind, sName, iNameLen) == 0 && sFind[iNameLen] == '=' ) {
			const char* sVal = sFind + iNameLen + 1;
			size_t iValLen = iPairLen - iNameLen - 1;
			return XS_HttpUrlDecodeCopy(sVal, iValLen);
		}
		sFind = pNext ? (pNext + 1) : NULL;
	}
	
	return NULL;
}

static inline char* XS_HttpQueryDup(const char* sQuery, const char* sName)
{
	return XS_HttpQueryText(sQuery, sName);
}

static inline bool XS_HttpHandleReload(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sReloadHostName;
	XS_HostConfig* objReloadHost;
	bool bForce;
	int iRet;
	char sBody[512];
	
	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	
	if ( strcmp(pReq->sPath, "/__xs/reload") != 0 ) {
		return FALSE;
	}
	
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "reload api disabled");
	}
	
	if ( _stricmp(pReq->sMethod, "GET") != 0 && _stricmp(pReq->sMethod, "POST") != 0 ) {
		return XS_HttpRespondText(pResp, 405, "Method Not Allowed", "reload api only supports GET or POST");
	}
	
	sReloadHostName = XS_HttpQueryText(pReq->sQuery, "host");
	bForce = XS_HttpQueryBool(pReq->sQuery, "force", FALSE);
	
	if ( sReloadHostName && sReloadHostName[0] ) {
		objReloadHost = XS_FindServerHostByName(objServer, sReloadHostName);
	} else {
		objReloadHost = (XS_HostConfig*)objHost;
	}
	
	if ( objReloadHost == NULL ) {
		if ( sReloadHostName ) xrtFree(sReloadHostName);
		return XS_HttpRespondText(pResp, 404, "Not Found", "reload host not found");
	}
	
	iRet = XS_ReloadServerHostScript(objServer, objReloadHost, bForce);
	snprintf(
		sBody,
		sizeof(sBody),
		"reload=%s\nserver=%s\nhost=%s\nforce=%s\ncode=%d\nmessage=%s\n",
		(iRet == 0) ? "ok" : "failed",
		objServer->Name ? objServer->Name : "(null)",
		objReloadHost->Name ? objReloadHost->Name : "(default)",
		bForce ? "true" : "false",
		iRet,
		XS_ReloadResultText(iRet)
	);
	
	if ( sReloadHostName ) {
		xrtFree(sReloadHostName);
	}
	
	return XS_HttpRespondText(
		pResp,
		(iRet == 0) ? 200 : 500,
		(iRet == 0) ? "OK" : "Internal Server Error",
		sBody
	);
}

static inline bool XS_HttpHandleConfigReload(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[512];
	char* sServerName;
	char* sHostName;
	bool bQueued;
	bool bForce;
	
	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/reload_config") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "config reload api disabled");
	}
	if ( _stricmp(pReq->sMethod, "GET") != 0 && _stricmp(pReq->sMethod, "POST") != 0 ) {
		return XS_HttpRespondText(pResp, 405, "Method Not Allowed", "config reload api only supports GET or POST");
	}
	
	bForce = XS_HttpQueryBool(pReq->sQuery, "force", FALSE);
	sServerName = XS_HttpQueryDup(pReq->sQuery, "server");
	sHostName = XS_HttpQueryDup(pReq->sQuery, "host");
	if ( (sServerName == NULL || sServerName[0] == '\0') && sHostName && sHostName[0] != '\0' ) {
		sServerName = XS_CopyText(objServer->Name);
	}
	bQueued = XS_RequestConfigReloadEx(sServerName, sHostName, bForce);
	snprintf(
		sBody,
		sizeof(sBody),
		"config_reload=%s\nserver=%s\ntarget_server=%s\ntarget_host=%s\nforce=%s\n",
		bQueued ? "queued" : "busy",
		objServer->Name ? objServer->Name : "(null)",
		(sServerName && sServerName[0]) ? sServerName : "(all)",
		(sHostName && sHostName[0]) ? sHostName : "(all)",
		bForce ? "true" : "false"
	);
	if ( sServerName ) {
		xrtFree(sServerName);
	}
	if ( sHostName ) {
		xrtFree(sHostName);
	}
	return XS_HttpRespondText(pResp, bQueued ? 200 : 409, bQueued ? "OK" : "Conflict", sBody);
}

static inline bool XS_HttpHandleConfigReloadStatus(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[512];
	
	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/reload_status") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "config reload status api disabled");
	}
	
	snprintf(
		sBody,
		sizeof(sBody),
		"busy=%s\nhas_result=%s\nsuccess=%s\nserver=%s\nhost=%s\nmessage=%s\n",
		XS_ConfigReloadStatusBusy() ? "true" : "false",
		XS_ConfigReloadStatusHasResult() ? "true" : "false",
		XS_ConfigReloadStatusSuccess() ? "true" : "false",
		XS_ConfigReloadStatusServer(),
		XS_ConfigReloadStatusHost(),
		XS_ConfigReloadStatusMessage()
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleStatus(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[2048];
	size_t iLen;
	uint32 i;
	
	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	
	if ( strcmp(pReq->sPath, "/__xs/status") != 0 ) {
		return FALSE;
	}
	
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "status api disabled");
	}
	
	iLen = (size_t)snprintf(
		sBody,
		sizeof(sBody),
		"server=%s\nclass=%s\naddr=%s\ndebug=%s\nhost_aware=%s\ndefault_host=%s\npath_limit=%u\nheader_limit=%u\nbody_limit=%u\nrecv_limit=%u\nbacklog=%u\n",
		objServer->Name ? objServer->Name : "(null)",
		XS_ServerClassName(objServer->Class),
		objServer->Addr ? objServer->Addr : "(null)",
		objServer->Debug ? "true" : "false",
		objServer->HostAware ? "true" : "false",
		objServer->EnableDefaultHost ? "true" : "false",
		(unsigned int)objServer->PathLimit,
		(unsigned int)objServer->HeaderLimit,
		(unsigned int)objServer->BodyLimit,
		(unsigned int)objServer->RecvLimit,
		(unsigned int)objServer->Backlog
	);
	
	if ( objServer->EnableDefaultHost && iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"default.name=%s\ndefault.dev=%s\ndefault.debug=%s\ndefault.devfile=%s\ndefault.script_loaded=%s\n",
			objServer->DefaultHost.Name ? objServer->DefaultHost.Name : "(default)",
			XS_DevModeName(objServer->DefaultHost.DevMode),
			objServer->DefaultHost.Debug ? "true" : "false",
			objServer->DefaultHost.DevFile ? objServer->DefaultHost.DevFile : "(null)",
			objServer->DefaultHost.pScriptState ? "true" : "false"
		);
	}
	
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(sBody + iLen, sizeof(sBody) - iLen, "hosts=%u\n", objServer->Hosts ? objServer->Hosts->Count : 0);
	}
	
	for ( i = 1; objServer->Hosts && i <= objServer->Hosts->Count && iLen < sizeof(sBody); i++ ) {
		XS_HostConfig* objItem = xrtArrayGet_Inline(objServer->Hosts, i);
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"host[%u].name=%s\nhost[%u].dev=%s\nhost[%u].debug=%s\nhost[%u].devfile=%s\nhost[%u].script_loaded=%s\n",
			i - 1,
			objItem->Name ? objItem->Name : "(null)",
			i - 1,
			XS_DevModeName(objItem->DevMode),
			i - 1,
			objItem->Debug ? "true" : "false",
			i - 1,
			objItem->DevFile ? objItem->DevFile : "(null)",
			i - 1,
			objItem->pScriptState ? "true" : "false"
		);
	}
	
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static void XS_HttpOnOpen(ptr pOwner, xhttpdserver* pServer, xhttpdconn* pConn)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	(void)pServer;
	(void)pConn;
	
	XS_LogInfo("http open: server=%s", objServer && objServer->Name ? objServer->Name : "(null)");
}

static bool XS_HttpOnRequest(ptr pOwner, xhttpdserver* pServer, xhttpdconn* pConn, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	const XS_HostConfig* objHost;
	char sBody[512];
	(void)pServer;
	(void)pConn;
	
	if ( pReq == NULL || pResp == NULL ) {
		return FALSE;
	}

	XS_HttpApplyDefaultHeaders(pReq, pResp);
	
	objHost = XS_HttpLocateHost(objServer, pReq);
	if ( objHost == NULL ) {
		return XS_HttpRespondText(pResp, 404, "Not Found", "host not found");
	}

	if ( XS_HttpValidateRequest(objServer, pReq, pResp) ) {
		return TRUE;
	}
	
	XS_LogInfo(
		"http request: server=%s host=%s method=%s path=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost->Name ? objHost->Name : "(default)",
		pReq->sMethod,
		pReq->sPath
	);
	
	if ( XS_HttpHandleReload(objServer, objHost, pReq, pResp) ) {
		return TRUE;
	}
	if ( XS_HttpHandleConfigReload(objServer, objHost, pReq, pResp) ) {
		return TRUE;
	}
	if ( XS_HttpHandleConfigReloadStatus(objServer, objHost, pReq, pResp) ) {
		return TRUE;
	}
	if ( XS_HttpHandleStatus(objServer, objHost, pReq, pResp) ) {
		return TRUE;
	}
	
	if ( objHost->DevMode == XS_DEV_STATIC ) {
		return XS_HttpServeStatic(objHost, pReq, pResp);
	}
	
	if ( objHost->DevMode == XS_DEV_SCRIPT_C ) {
		if ( objHost->procHttpRequest ) {
			if ( XS_HttpHandleScriptHost(objServer, objHost, pReq, pResp) ) {
				return TRUE;
			}
		}
		
		snprintf(
			sBody,
			sizeof(sBody),
			"script host did not handle request\nserver=%s\nhost=%s\npath=%s\n",
			objServer && objServer->Name ? objServer->Name : "(null)",
			objHost->Name ? objHost->Name : "(default)",
			pReq->sPath
		);
		return XS_HttpRespondText(pResp, 404, "Not Found", sBody);
	}
	
	snprintf(
		sBody,
		sizeof(sBody),
		"xserver vNext http skeleton\nserver=%s\nhost=%s\npath=%s\n",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost->Name ? objHost->Name : "(default)",
		pReq->sPath
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static void XS_HttpOnClose(ptr pOwner, xhttpdserver* pServer, xhttpdconn* pConn, xnet_result iReason)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	(void)pServer;
	(void)pConn;
	
	XS_LogInfo(
		"http close: server=%s reason=%d",
		objServer && objServer->Name ? objServer->Name : "(null)",
		(int)iReason
	);
}

static void XS_HttpOnError(ptr pOwner, xhttpdserver* pServer, xhttpdconn* pConn, int iSysErr)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	(void)pServer;
	(void)pConn;
	
	XS_LogWarn(
		"http error: server=%s sys=%d",
		objServer && objServer->Name ? objServer->Name : "(null)",
		iSysErr
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
	
	iPort = (uint16)atoi(pColon + 1);
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

static inline bool XS_HttpInitServer(xnetengine* pEngine, XS_ServerConfig* objServer)
{
	xhttpdconfig tConfig;
	xhttpdevents tEvents;
	xhttpdserver* pServer;
	
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
	
	xrtHttpdConfigInit(&tConfig);
	if ( !XS_BuildBindAddr(objServer, FALSE, &tConfig.tBindAddr) ) {
		XS_ReportError("http init failed: invalid addr: %s", objServer->Addr ? objServer->Addr : "(null)");
		return FALSE;
	}
	tConfig.iBacklog = objServer->Backlog;
	tConfig.iRecvLimit = objServer->RecvLimit;
	
	memset(&tEvents, 0, sizeof(tEvents));
	tEvents.OnOpen = XS_HttpOnOpen;
	tEvents.OnRequest = XS_HttpOnRequest;
	tEvents.OnClose = XS_HttpOnClose;
	tEvents.OnError = XS_HttpOnError;
	
	pServer = xrtHttpdCreate(pEngine, &tConfig, &tEvents, objServer);
	if ( pServer == NULL ) {
		XS_ReportError("http init failed: xrtHttpdCreate returned null");
		return FALSE;
	}
	
	objServer->pHandle = pServer;
	
	return TRUE;
}

static inline bool XS_HttpStartServer(XS_ServerConfig* objServer)
{
	xhttpdserver* pServer = (xhttpdserver*)objServer->pHandle;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	if ( pServer == NULL ) {
		XS_ReportError("http start failed: server handle is null");
		return FALSE;
	}
	if ( xrtHttpdStart(pServer) != XRT_NET_OK ) {
		XS_ReportError("http start failed: xrtHttpdStart returned error");
		return FALSE;
	}
	
	XS_LogInfo(
		"http start: server=%s addr=%s bound_port=%u",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)",
		(unsigned)xrtHttpdBoundPort(pServer)
	);
	
	return TRUE;
}

static inline void XS_HttpStopServer(XS_ServerConfig* objServer)
{
	xhttpdserver* pServer;
	
	if ( objServer == NULL ) {
		return;
	}
	
	pServer = (xhttpdserver*)objServer->pHandle;
	if ( pServer ) {
		xrtHttpdStop(pServer);
		xrtHttpdDestroy(pServer);
		objServer->pHandle = NULL;
	}
	
	XS_LogInfo(
		"http stop: server=%s",
		objServer->Name ? objServer->Name : "(null)"
	);
}

#endif
