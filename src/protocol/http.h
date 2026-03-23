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
	XS_ServerConfig* pOwner;
	xthread hIdleThread;
	volatile bool bStopThread;
	xmutex pConnLock;
	xarray arrConn;
} XS_HttpHandle;

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

static inline bool XS_HttpIsManagePath(const char* sPath)
{
	if ( sPath == NULL ) {
		return FALSE;
	}

	return strcmp(sPath, "/__xs") == 0 || strncmp(sPath, "/__xs/", 6) == 0;
}

static inline uint64 XS_ProcessID(void)
{
#ifdef _WIN32
	return (uint64)GetCurrentProcessId();
#else
	return (uint64)getpid();
#endif
}

static inline int64 XS_ProcessUptimeMS(void)
{
	double fNowTick;
	double fSpan;

	fNowTick = xrtTimer();
	fSpan = fNowTick - g_fXsStartTick;
	if ( fSpan < 0.0 ) {
		fSpan = 0.0;
	}

	return (int64)(fSpan * 1000.0);
}

static inline const char* XS_BuildVariant(void)
{
	const char* sFileName;
	const char* pSlash;
	const char* pBackSlash;

	sFileName = xCore.AppFile ? (char*)xCore.AppFile : NULL;
	if ( sFileName ) {
		pSlash = strrchr(sFileName, '/');
		pBackSlash = strrchr(sFileName, '\\');
		if ( pSlash || pBackSlash ) {
			if ( pSlash == NULL ) {
				sFileName = pBackSlash + 1;
			} else if ( pBackSlash == NULL ) {
				sFileName = pSlash + 1;
			} else {
				sFileName = ((pSlash > pBackSlash) ? pSlash : pBackSlash) + 1;
			}
		}
	}

	if ( sFileName && _stricmp(sFileName, "xsdbg.exe") == 0 ) {
		return "xsdbg";
	}
	if ( sFileName && _stricmp(sFileName, "xsdbg") == 0 ) {
		return "xsdbg";
	}
	if ( sFileName && _stricmp(sFileName, "xs.exe") == 0 ) {
		return "xs";
	}
	if ( sFileName && _stricmp(sFileName, "xs") == 0 ) {
		return "xs";
	}

	return "unknown";
}

static inline const char* XS_CompilerName(void)
{
#if defined(__TINYC__)
	return "tcc";
#elif defined(__clang__)
	return "clang";
#elif defined(__GNUC__)
	return "gcc";
#elif defined(_MSC_VER)
	return "msvc";
#else
	return "unknown";
#endif
}

static inline bool XS_MemDebugEnabled(void)
{
#ifdef XRT_MEM_DEBUG
	return TRUE;
#else
	return FALSE;
#endif
}

static inline char* XS_CurrentWorkDir(void)
{
#ifdef _WIN32
	char sBuf[4096];
	DWORD iLen;

	iLen = GetCurrentDirectoryA((DWORD)sizeof(sBuf), sBuf);
	if ( iLen == 0 || iLen >= sizeof(sBuf) ) {
		return NULL;
	}

	return xrtCopyStr(sBuf, 0);
#else
	char sBuf[4096];

	if ( getcwd(sBuf, sizeof(sBuf)) == NULL ) {
		return NULL;
	}

	return xrtCopyStr(sBuf, 0);
#endif
}

static inline char* XS_ConfigFileName(void)
{
	if ( g_sXsConfigFile == NULL || g_sXsConfigFile[0] == '\0' ) {
		return NULL;
	}

	return xrtPathGetName(g_sXsConfigFile, 0);
}

static inline char* XS_ConfigNormalizeRuntimePath(const char* sFilePath)
{
	return XS_NormalizePath(xCore.AppPath, sFilePath);
}

static inline char* XS_FileChangeTimeText(const char* sPath)
{
	int64 iTime;

	if ( sPath == NULL || sPath[0] == '\0' ) {
		return NULL;
	}

	iTime = xrtFileGetChangeTime((char*)sPath);
	if ( iTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr((xtime)iTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_FileSizeValue(const char* sPath)
{
	if ( sPath == NULL || sPath[0] == '\0' ) {
		return -1;
	}

	return (int64)xrtFileGetSize((char*)sPath);
}

static inline int64 XS_HttpMetricAdd(volatile int64* pValue, int64 iDelta)
{
	return __xrtAtomicAddFetch64(pValue, iDelta);
}

static inline int64 XS_HttpMetricGet(const volatile int64* pValue)
{
	return __xrtAtomicAddFetch64((volatile int64*)pValue, 0);
}

static inline int64 XS_HttpMetricCompareExchange(volatile int64* pValue, int64 iExchange, int64 iComparand)
{
#ifdef _WIN32
	return (int64)InterlockedCompareExchange64((volatile LONG64*)pValue, (LONG64)iExchange, (LONG64)iComparand);
#else
	return __sync_val_compare_and_swap(pValue, iComparand, iExchange);
#endif
}

static inline void XS_HttpMetricUpdateMax(volatile int64* pValue, int64 iValue)
{
	int64 iPrev;

	for ( ; ; ) {
		iPrev = XS_HttpMetricGet(pValue);
		if ( iValue <= iPrev ) {
			break;
		}
		if ( XS_HttpMetricCompareExchange(pValue, iValue, iPrev) == iPrev ) {
			break;
		}
	}
}

static inline void XS_HttpMetricSetText(char* sValue, size_t iCapacity, const char* sText)
{
	if ( sValue == NULL || iCapacity == 0 ) {
		return;
	}

	if ( sText && sText[0] ) {
		strncpy(sValue, sText, iCapacity - 1);
		sValue[iCapacity - 1] = '\0';
	} else {
		sValue[0] = '\0';
	}
}

static inline const char* XS_HttpCanonicalRejectReason(const char* sReason)
{
	if ( sReason == NULL || sReason[0] == '\0' ) {
		return "reject";
	}
	if ( strcmp(sReason, "header count limit exceeded") == 0 ) {
		return "header_limit";
	}
	if ( strcmp(sReason, "request body limit exceeded") == 0 ) {
		return "body_limit";
	}
	if ( strcmp(sReason, "request path limit exceeded") == 0 ) {
		return "path_limit";
	}
	if ( strstr(sReason, "api disabled") != NULL ) {
		return "api_disabled";
	}
	if ( strstr(sReason, "only supports ") != NULL ) {
		return "method_not_allowed";
	}
	if ( strcmp(sReason, "reload host not found") == 0 || strcmp(sReason, "host not found") == 0 ) {
		return "host_not_found";
	}
	if ( strcmp(sReason, "config reload busy") == 0 || strcmp(sReason, "config_reload=busy") == 0 || strcmp(sReason, "reload busy") == 0 ) {
		return "reload_busy";
	}
	if ( strcmp(sReason, "reload failed") == 0 || strcmp(sReason, "invalid target") == 0 || strcmp(sReason, "host is not script-c") == 0 ) {
		return "reload_failed";
	}
	if ( strcmp(sReason, "check config failed") == 0 || strcmp(sReason, "config file not set") == 0 ) {
		return "check_config_failed";
	}
	if (
		strcmp(sReason, "data limit exceeded") == 0 ||
		strcmp(sReason, "queue limit exceeded") == 0 ||
		strcmp(sReason, "namespace limit exceeded") == 0 ||
		strcmp(sReason, "namespace data limit exceeded") == 0 ||
		strcmp(sReason, "bus_limit") == 0
	) {
		return "bus_limit";
	}
	if ( strcmp(sReason, "bus_bad_request") == 0 || strcmp(sReason, "bus_not_found") == 0 || strcmp(sReason, "bus_failed") == 0 ) {
		return sReason;
	}

	return sReason;
}

static inline void XS_HttpRecordPolicyReject(const char* sReason)
{
	if ( sReason == NULL || sReason[0] == '\0' ) {
		return;
	}
	if ( strcmp(sReason, "api_disabled") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpApiDisabledRejectCount, 1);
		g_tXsHttpLastApiDisabledRejectTime = xrtNow();
		return;
	}
	if ( strcmp(sReason, "method_not_allowed") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpMethodRejectCount, 1);
		g_tXsHttpLastMethodRejectTime = xrtNow();
		return;
	}
	if ( strcmp(sReason, "host_not_found") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpHostNotFoundRejectCount, 1);
		g_tXsHttpLastHostNotFoundRejectTime = xrtNow();
		return;
	}
}

static inline void XS_HttpRecordManageReject(const char* sReason)
{
	if ( sReason == NULL || sReason[0] == '\0' ) {
		return;
	}
	if ( strcmp(sReason, "reload_busy") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpReloadBusyRejectCount, 1);
		g_tXsHttpLastReloadBusyRejectTime = xrtNow();
		return;
	}
	if ( strcmp(sReason, "reload_failed") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpReloadFailedRejectCount, 1);
		g_tXsHttpLastReloadFailedRejectTime = xrtNow();
		return;
	}
	if ( strcmp(sReason, "check_config_failed") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpCheckConfigFailedRejectCount, 1);
		g_tXsHttpLastCheckConfigFailedRejectTime = xrtNow();
		return;
	}
}

static inline void XS_HttpRecordBusReject(const char* sReason)
{
	if ( sReason == NULL || sReason[0] == '\0' ) {
		return;
	}
	if ( strcmp(sReason, "bus_bad_request") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpBusBadRequestRejectCount, 1);
		g_tXsHttpLastBusBadRequestRejectTime = xrtNow();
		return;
	}
	if ( strcmp(sReason, "bus_not_found") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpBusNotFoundRejectCount, 1);
		g_tXsHttpLastBusNotFoundRejectTime = xrtNow();
		return;
	}
	if ( strcmp(sReason, "bus_limit") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpBusLimitRejectCount, 1);
		g_tXsHttpLastBusLimitRejectTime = xrtNow();
		return;
	}
	if ( strcmp(sReason, "bus_failed") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpBusFailedRejectCount, 1);
		g_tXsHttpLastBusFailedRejectTime = xrtNow();
		return;
	}
}

static inline int32 XS_HttpBusErrorStatusCode(int32 iBusCode)
{
	if ( XS_BusIsBadRequestError(iBusCode) ) {
		return 400;
	}
	if ( XS_BusIsLimitError(iBusCode) ) {
		return 409;
	}

	return 500;
}

static inline const char* XS_HttpBusErrorStatusText(int32 iBusCode)
{
	if ( XS_BusIsBadRequestError(iBusCode) ) {
		return "Bad Request";
	}
	if ( XS_BusIsLimitError(iBusCode) ) {
		return "Conflict";
	}

	return "Internal Server Error";
}

static inline bool XS_HttpRespondBusNamespaceBadRequest(xhttpdresponse* pResp, const char* sNamespace)
{
	const char* sError;
	int32 iBusCode;
	char sBody[256];

	if ( pResp == NULL || sNamespace == NULL || sNamespace[0] == '\0' ) {
		return FALSE;
	}

	sError = XS_BusNamespaceInvalidReason(sNamespace);
	iBusCode = XS_BUS_ERR_NONE;
	if ( sError ) {
		iBusCode = XS_BUS_ERR_INVALID_NAMESPACE;
	} else if ( XS_BusIsReservedNamespace(sNamespace) ) {
		sError = "reserved namespace";
		iBusCode = XS_BUS_ERR_RESERVED_NAMESPACE;
	}
	if ( sError == NULL || iBusCode == XS_BUS_ERR_NONE ) {
		return FALSE;
	}

	snprintf(
		sBody,
		sizeof(sBody),
		"{\"result\":false,\"message\":\"invalid namespace\",\"bus_code\":%d,\"bus_error\":\"%s\"}",
		(int)iBusCode,
		sError
	);
	xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpParseInt64(const char* sText, int64* piValue)
{
	char* sEnd = NULL;
	int64 iValue = 0;

	if ( piValue ) {
		*piValue = 0;
	}
	if ( sText == NULL || sText[0] == '\0' ) {
		return FALSE;
	}

	iValue = (int64)strtoll(sText, &sEnd, 10);
	if ( sEnd == NULL || sEnd[0] != '\0' ) {
		return FALSE;
	}
	if ( piValue ) {
		*piValue = iValue;
	}
	return TRUE;
}

static inline bool XS_HttpParseBoolText(const char* sText, size_t iTextLen, bool* pbValue)
{
	if ( pbValue ) {
		*pbValue = FALSE;
	}
	if ( sText == NULL || iTextLen == 0 ) {
		return FALSE;
	}

	if ( iTextLen == 1 && (sText[0] == '1' || sText[0] == 'y' || sText[0] == 'Y') ) {
		if ( pbValue ) {
			*pbValue = TRUE;
		}
		return TRUE;
	}
	if ( iTextLen == 4 && _strnicmp(sText, "true", 4) == 0 ) {
		if ( pbValue ) {
			*pbValue = TRUE;
		}
		return TRUE;
	}
	if ( iTextLen == 3 && _strnicmp(sText, "yes", 3) == 0 ) {
		if ( pbValue ) {
			*pbValue = TRUE;
		}
		return TRUE;
	}
	if ( iTextLen == 1 && (sText[0] == '0' || sText[0] == 'n' || sText[0] == 'N') ) {
		if ( pbValue ) {
			*pbValue = FALSE;
		}
		return TRUE;
	}
	if ( iTextLen == 5 && _strnicmp(sText, "false", 5) == 0 ) {
		if ( pbValue ) {
			*pbValue = FALSE;
		}
		return TRUE;
	}
	if ( iTextLen == 2 && _strnicmp(sText, "no", 2) == 0 ) {
		if ( pbValue ) {
			*pbValue = FALSE;
		}
		return TRUE;
	}

	return FALSE;
}

static inline bool XS_HttpParsePositiveInt64(const char* sText, int64* piValue)
{
	int64 iValue = 0;

	if ( !XS_HttpParseInt64(sText, &iValue) || iValue <= 0 ) {
		return FALSE;
	}
	if ( piValue ) {
		*piValue = iValue;
	}
	return TRUE;
}

static inline bool XS_HttpParseNonNegativeInt64(const char* sText, int64* piValue)
{
	int64 iValue = 0;

	if ( !XS_HttpParseInt64(sText, &iValue) || iValue < 0 ) {
		return FALSE;
	}
	if ( piValue ) {
		*piValue = iValue;
	}
	return TRUE;
}

static inline bool XS_ParsePortText(const char* sText, uint16* piPort)
{
	uint32 iValue = 0;
	size_t iTextLen = 0;

	if ( piPort ) {
		*piPort = 0;
	}
	if ( sText == NULL || sText[0] == '\0' ) {
		return FALSE;
	}

	iTextLen = strlen(sText);
	if ( iTextLen == 0 || iTextLen > 5 ) {
		return FALSE;
	}

	for ( size_t i = 0; i < iTextLen; ++i ) {
		char ch = sText[i];

		if ( ch < '0' || ch > '9' ) {
			return FALSE;
		}
		iValue = (iValue * 10u) + (uint32)(ch - '0');
		if ( iValue > 65535u ) {
			return FALSE;
		}
	}

	if ( iValue == 0u ) {
		return FALSE;
	}
	if ( piPort ) {
		*piPort = (uint16)iValue;
	}
	return TRUE;
}

static inline bool XS_HttpRespondBusDataIDBadRequest(xhttpdresponse* pResp, const char* sID)
{
	char sBody[128];

	if ( pResp == NULL || sID == NULL || sID[0] == '\0' ) {
		return FALSE;
	}
	if ( XS_HttpParsePositiveInt64(sID, NULL) ) {
		return FALSE;
	}

	snprintf(
		sBody,
		sizeof(sBody),
		"{\"result\":false,\"message\":\"invalid data id\",\"data_id\":0}"
	);
	xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpRespondBusTTLBadRequest(xhttpdresponse* pResp, const char* sTTL)
{
	char sBody[120];

	if ( pResp == NULL || sTTL == NULL || sTTL[0] == '\0' ) {
		return FALSE;
	}
	if ( XS_HttpParseNonNegativeInt64(sTTL, NULL) ) {
		return FALSE;
	}

	snprintf(
		sBody,
		sizeof(sBody),
		"{\"result\":false,\"message\":\"invalid ttl\",\"ttl\":0}"
	);
	xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpRespondBusNamedIntBadRequest(xhttpdresponse* pResp, const char* sName)
{
	char sBody[160];

	if ( pResp == NULL || sName == NULL || sName[0] == '\0' ) {
		return FALSE;
	}

	snprintf(
		sBody,
		sizeof(sBody),
		"{\"result\":false,\"message\":\"invalid %s\",\"%s\":0}",
		sName,
		sName
	);
	xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpRespondNamedBoolBadRequestText(xhttpdresponse* pResp, const char* sName)
{
	char sBody[160];

	if ( pResp == NULL || sName == NULL || sName[0] == '\0' ) {
		return FALSE;
	}

	snprintf(sBody, sizeof(sBody), "invalid %s", sName);
	return XS_HttpRespondText(pResp, 400, "Bad Request", sBody);
}

static inline bool XS_HttpRespondNamedBoolBadRequestJson(xhttpdresponse* pResp, const char* sName)
{
	char sBody[160];

	if ( pResp == NULL || sName == NULL || sName[0] == '\0' ) {
		return FALSE;
	}

	snprintf(sBody, sizeof(sBody), "{\"result\":false,\"message\":\"invalid %s\"}", sName);
	xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpRespondJsonResult(xhttpdresponse* pResp, int32 iStatusCode, const char* sStatusText, bool bResult, const char* sMessage)
{
	char sBody[512];
	const char* pStatusText = sStatusText ? sStatusText : (bResult ? "OK" : "Bad Request");
	const char* pMessage = sMessage ? sMessage : "";

	if ( pResp == NULL ) {
		return FALSE;
	}

	snprintf(
		sBody,
		sizeof(sBody),
		"{\"result\":%s,\"message\":\"%s\"}",
		bResult ? "true" : "false",
		pMessage
	);
	xrtHttpdResponseSetStatus(pResp, iStatusCode, pStatusText);
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline void XS_HttpFreeBusLimitQueryValues(
	char* sDataLimit,
	char* sQueueLimit,
	char* sNamespaceLimit,
	char* sNamespaceDataLimit,
	char* sSweepIntervalMS,
	char* sReadonlyNamespaces,
	char* sDisabledNamespaces,
	char* sTTLRequiredNamespaces,
	char* sTagRequiredNamespaces
)
{
	if ( sDataLimit ) {
		xrtFree(sDataLimit);
	}
	if ( sQueueLimit ) {
		xrtFree(sQueueLimit);
	}
	if ( sNamespaceLimit ) {
		xrtFree(sNamespaceLimit);
	}
	if ( sNamespaceDataLimit ) {
		xrtFree(sNamespaceDataLimit);
	}
	if ( sSweepIntervalMS ) {
		xrtFree(sSweepIntervalMS);
	}
	if ( sReadonlyNamespaces ) {
		xrtFree(sReadonlyNamespaces);
	}
	if ( sDisabledNamespaces ) {
		xrtFree(sDisabledNamespaces);
	}
	if ( sTTLRequiredNamespaces ) {
		xrtFree(sTTLRequiredNamespaces);
	}
	if ( sTagRequiredNamespaces ) {
		xrtFree(sTagRequiredNamespaces);
	}
}

static inline void XS_HttpRecordRejectCommon(volatile int64* pCount, xtime* pTime, char* sReason, size_t iReasonCap, const char* sValue)
{
	if ( pCount ) {
		XS_HttpMetricAdd(pCount, 1);
	}
	if ( pTime ) {
		*pTime = xrtNow();
	}
	XS_HttpMetricSetText(sReason, iReasonCap, sValue);
}

static inline void XS_HttpRecordStopCleanupCommon(volatile int64* pCount, xtime* pTime, volatile int64* pClosed, volatile int64* pRemain, int64 iClosed, int64 iRemain)
{
	if ( pCount ) {
		XS_HttpMetricAdd(pCount, 1);
	}
	if ( pTime ) {
		*pTime = xrtNow();
	}
	if ( pClosed ) {
		*pClosed = iClosed;
	}
	if ( pRemain ) {
		*pRemain = iRemain;
	}
}

static inline void XS_HttpRecordRejectEvent(int64 iStatusCode, const char* sReason)
{
	const char* sCanonical = XS_HttpCanonicalRejectReason(sReason);

	XS_HttpRecordRejectCommon(&g_iXsHttpRejectCount, &g_tXsHttpLastRejectTime, g_sXsHttpLastRejectReason, sizeof(g_sXsHttpLastRejectReason), sCanonical);
	g_iXsHttpLastRejectStatus = iStatusCode;
	XS_HttpRecordPolicyReject(sCanonical);
	XS_HttpRecordManageReject(sCanonical);
	XS_HttpRecordBusReject(sCanonical);
}

static inline void XS_HttpRecordStopCleanup(int64 iClosed, int64 iRemain)
{
	XS_HttpRecordStopCleanupCommon(&g_iXsHttpStopCleanupCount, &g_tXsHttpLastStopCleanupTime, &g_iXsHttpLastStopCleanupClosed, &g_iXsHttpLastStopCleanupRemain, iClosed, iRemain);
}

static inline const char* XS_HttpRejectReasonText(const xhttpdresponse* pResp, char* sBuf, size_t iBufCap)
{
	const char* sText;
	size_t iStart;
	size_t iEnd;
	size_t iCopy;

	if ( sBuf && iBufCap > 0 ) {
		sBuf[0] = '\0';
	}
	if ( pResp == NULL ) {
		return "reject";
	}

	sText = pResp->pBody;
	if ( sText && pResp->iBodyLen > 0 ) {
		iStart = 0;
		while ( iStart < pResp->iBodyLen && (sText[iStart] == ' ' || sText[iStart] == '\t' || sText[iStart] == '\r' || sText[iStart] == '\n') ) {
			iStart++;
		}
		if ( iStart < pResp->iBodyLen && sText[iStart] != '{' && sText[iStart] != '[' ) {
			iEnd = iStart;
			while ( iEnd < pResp->iBodyLen && sText[iEnd] != '\0' && sText[iEnd] != '\r' && sText[iEnd] != '\n' ) {
				iEnd++;
			}
			if ( iEnd > iStart && sBuf && iBufCap > 1 ) {
				iCopy = iEnd - iStart;
				if ( iCopy >= iBufCap ) {
					iCopy = iBufCap - 1;
				}
				memcpy(sBuf, sText + iStart, iCopy);
				sBuf[iCopy] = '\0';
				return XS_HttpCanonicalRejectReason(sBuf);
			}
		}
	}

	if ( pResp->sReason[0] ) {
		return XS_HttpCanonicalRejectReason(pResp->sReason);
	}

	return "reject";
}

static inline const char* XS_HttpRejectReasonTextEx(const xhttpdrequest* pReq, const xhttpdresponse* pResp, char* sBuf, size_t iBufCap)
{
	const char* sPath;
	uint32 iStatusCode;

	if ( sBuf && iBufCap > 0 ) {
		sBuf[0] = '\0';
	}
	if ( pResp == NULL ) {
		return "reject";
	}

	sPath = (pReq && pReq->sPath) ? pReq->sPath : NULL;
	iStatusCode = pResp->iStatusCode;
	if ( sPath ) {
		if ( strcmp(sPath, "/__xs/bus") == 0 || strncmp(sPath, "/__xs/bus/", 10) == 0 ) {
			if ( iStatusCode == 400u ) {
				return "bus_bad_request";
			}
			if ( iStatusCode == 404u ) {
				return "bus_not_found";
			}
			if ( iStatusCode == 409u ) {
				return "bus_limit";
			}
			if ( iStatusCode >= 500u ) {
				return "bus_failed";
			}
		}
		if (
			(strcmp(sPath, "/__xs/reload_config") == 0 || strcmp(sPath, "/__xs/reload_config_json") == 0) &&
			iStatusCode == 409u
		) {
			return "reload_busy";
		}
		if (
			(strcmp(sPath, "/__xs/check_config") == 0 || strcmp(sPath, "/__xs/check_config_json") == 0) &&
			iStatusCode >= 500u
		) {
			return "check_config_failed";
		}
	}
	if ( XS_HttpExtractResponseMessage(pResp, sBuf, iBufCap) ) {
		return XS_HttpCanonicalRejectReason(sBuf);
	}
	if ( sPath ) {
		if ( (strcmp(sPath, "/__xs/reload") == 0 || strcmp(sPath, "/__xs/reload_json") == 0) && iStatusCode >= 500u ) {
			return "reload_failed";
		}
	}

	return XS_HttpRejectReasonText(pResp, sBuf, iBufCap);
}

static inline void XS_WsRecordRejectEvent(const char* sReason)
{
	XS_HttpRecordRejectCommon(&g_iXsWsRejectCount, &g_tXsWsLastRejectTime, g_sXsWsLastRejectReason, sizeof(g_sXsWsLastRejectReason), sReason);
}

static inline void XS_WsRecordStopCleanup(int64 iClosed, int64 iRemain)
{
	XS_HttpRecordStopCleanupCommon(&g_iXsWsStopCleanupCount, &g_tXsWsLastStopCleanupTime, &g_iXsWsLastStopCleanupClosed, &g_iXsWsLastStopCleanupRemain, iClosed, iRemain);
}

static inline void XS_XtpRecordRejectEvent(const char* sReason)
{
	XS_HttpRecordRejectCommon(&g_iXsXtpRejectCount, &g_tXsXtpLastRejectTime, g_sXsXtpLastRejectReason, sizeof(g_sXsXtpLastRejectReason), sReason);
}

static inline void XS_XtpRecordStopCleanup(int64 iClosed, int64 iRemain)
{
	XS_HttpRecordStopCleanupCommon(&g_iXsXtpStopCleanupCount, &g_tXsXtpLastStopCleanupTime, &g_iXsXtpLastStopCleanupClosed, &g_iXsXtpLastStopCleanupRemain, iClosed, iRemain);
}

static inline void XS_CustomRecordRejectEvent(const char* sReason)
{
	XS_HttpRecordRejectCommon(&g_iXsCustomRejectCount, &g_tXsCustomLastRejectTime, g_sXsCustomLastRejectReason, sizeof(g_sXsCustomLastRejectReason), sReason);
}

static inline void XS_CustomRecordStopCleanup(int64 iClosed, int64 iRemain)
{
	XS_HttpRecordStopCleanupCommon(&g_iXsCustomStopCleanupCount, &g_tXsCustomLastStopCleanupTime, &g_iXsCustomLastStopCleanupClosed, &g_iXsCustomLastStopCleanupRemain, iClosed, iRemain);
}

static inline int64 XS_HttpNowMS(void)
{
	#if defined(_WIN32) || defined(_WIN64)
		return (int64)GetTickCount64();
	#else
		struct timespec tNow;

		clock_gettime(CLOCK_MONOTONIC, &tNow);
		return ((int64)tNow.tv_sec * 1000) + ((int64)tNow.tv_nsec / 1000000);
	#endif
}

static inline void XS_HttpSleepMS(uint32 iMS)
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

static inline const char* XS_HttpConnRemoteText(const xhttpdconn* pConn)
{
	const xnetaddr* pAddr;

	if ( pConn == NULL || pConn->pStream == NULL ) {
		return NULL;
	}

	pAddr = xrtNetStreamRemoteAddr(pConn->pStream);
	return pAddr ? xrtNetAddrToStr(pAddr) : NULL;
}

static inline XS_HttpConnContext* XS_HttpGetConnContext(XS_HttpHandle* objHandle, xhttpdconn* pConn)
{
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL || pConn == NULL || pConn->pStream == NULL ) {
		return NULL;
	}

	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_HttpConnContext** ppItem = (XS_HttpConnContext**)xrtArrayGet(objHandle->arrConn, i);

		if ( ppItem && *ppItem && (*ppItem)->pStream == pConn->pStream ) {
			XS_HttpConnContext* objCtx = *ppItem;

			xrtMutexUnlock(objHandle->pConnLock);
			return objCtx;
		}
	}
	xrtMutexUnlock(objHandle->pConnLock);

	return NULL;
}

static inline void XS_HttpTouch(XS_HttpConnContext* objCtx)
{
	if ( objCtx == NULL ) {
		return;
	}

	objCtx->iLastActiveMS = XS_HttpNowMS();
}

static inline void XS_HttpTrackConn(XS_HttpHandle* objHandle, XS_HttpConnContext* objCtx)
{
	XS_HttpConnContext** ppSlot;
	uint32 iPos;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL || objCtx == NULL ) {
		return;
	}

	xrtMutexLock(objHandle->pConnLock);
	iPos = xrtArrayAppend(objHandle->arrConn, 1);
	ppSlot = (XS_HttpConnContext**)xrtArrayGet(objHandle->arrConn, iPos);
	if ( ppSlot ) {
		*ppSlot = objCtx;
	}
	xrtMutexUnlock(objHandle->pConnLock);
}

static inline void XS_HttpUntrackConn(XS_HttpHandle* objHandle, XS_HttpConnContext* objCtx)
{
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL || objCtx == NULL ) {
		return;
	}

	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_HttpConnContext** ppItem = (XS_HttpConnContext**)xrtArrayGet(objHandle->arrConn, i);

		if ( ppItem && *ppItem == objCtx ) {
			xrtArrayRemove(objHandle->arrConn, i, 1);
			break;
		}
	}
	xrtMutexUnlock(objHandle->pConnLock);
}

static inline int64 XS_HttpTrackedConnCount(XS_HttpHandle* objHandle)
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

static inline int64 XS_HttpCloseTrackedConns(XS_HttpHandle* objHandle)
{
	xarray arrClose;
	int64 iCloseCount;
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		return 0;
	}

	arrClose = xrtArrayCreate(sizeof(xnetstream*), XRT_OBJMODE_LOCAL);
	if ( arrClose == NULL ) {
		return 0;
	}

	iCloseCount = 0;
	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_HttpConnContext** ppItem = (XS_HttpConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_HttpConnContext* objCtx = ppItem ? *ppItem : NULL;
		xnetstream** ppClose;
		uint32 iPos;

		if ( objCtx == NULL || objCtx->pStream == NULL ) {
			continue;
		}

		objCtx->bClosing = TRUE;
		iPos = xrtArrayAppend(arrClose, 1);
		ppClose = (xnetstream**)xrtArrayGet(arrClose, iPos);
		if ( ppClose ) {
			*ppClose = objCtx->pStream;
			iCloseCount++;
		}
	}
	xrtMutexUnlock(objHandle->pConnLock);

	for ( i = 1; i <= arrClose->Count; i++ ) {
		xnetstream** ppClose = (xnetstream**)xrtArrayGet(arrClose, i);

		if ( ppClose && *ppClose ) {
			xrtNetStreamClose(*ppClose, 0u);
		}
	}
	xrtArrayDestroy(arrClose);
	return iCloseCount;
}

static inline int64 XS_HttpWaitTrackedConnDrain(XS_HttpHandle* objHandle, uint32 iTimeoutMS)
{
	uint32 iWaitedMS = 0;
	int64 iRemain = XS_HttpTrackedConnCount(objHandle);

	while ( iRemain > 0 && iWaitedMS < iTimeoutMS ) {
		XS_HttpSleepMS(20);
		iWaitedMS += 20;
		iRemain = XS_HttpTrackedConnCount(objHandle);
	}

	return iRemain;
}

static inline void XS_HttpRecordIdleClose(void)
{
	XS_HttpMetricAdd(&g_iXsHttpIdleCloseCount, 1);
	g_tXsHttpLastIdleCloseTime = xrtNow();
}

static inline void XS_HttpRecordConnLimitClose(void)
{
	XS_HttpMetricAdd(&g_iXsHttpConnLimitCloseCount, 1);
	g_tXsHttpLastConnLimitCloseTime = xrtNow();
	XS_HttpRecordRejectEvent(0, "conn_limit");
}

static inline void XS_HttpRecordHeaderLimitReject(void)
{
	XS_HttpMetricAdd(&g_iXsHttpHeaderLimitRejectCount, 1);
	g_tXsHttpLastHeaderLimitRejectTime = xrtNow();
}

static inline void XS_HttpRecordBodyLimitReject(void)
{
	XS_HttpMetricAdd(&g_iXsHttpBodyLimitRejectCount, 1);
	g_tXsHttpLastBodyLimitRejectTime = xrtNow();
}

static inline void XS_HttpRecordPathLimitReject(void)
{
	XS_HttpMetricAdd(&g_iXsHttpPathLimitRejectCount, 1);
	g_tXsHttpLastPathLimitRejectTime = xrtNow();
}

static inline int64 XS_HttpQueryLength(const xhttpdrequest* pReq)
{
	const char* pQuery;

	if ( pReq == NULL || pReq->sTarget == NULL ) {
		return 0;
	}

	pQuery = strchr(pReq->sTarget, '?');
	if ( pQuery == NULL || pQuery[1] == '\0' ) {
		return 0;
	}

	return (int64)strlen(pQuery + 1);
}

static inline void XS_HttpRecordResponseMetrics(const xhttpdconn* pConn, const xhttpdrequest* pReq, const xhttpdresponse* pResp)
{
	uint32 iStatusCode;
	bool bManagePath;
	const char* sRemote;
	const char* sContentType;
	const char* sHost;
	const char* sUserAgent;
	const char* sReferer;
	const char* sOrigin;
	const char* sAccept;
	const char* sAcceptEncoding;
	const char* sCookie;
	const char* sForwardedFor;
	const char* sRealIP;
	const char* sConnection;
	const char* sCacheControl;

	if ( pReq && (
		strcmp(pReq->sPath, "/__xs/http_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/http_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/http_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/ws_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/ws_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/ws_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/xtp_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/xtp_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/xtp_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/udp_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/udp_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/udp_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/custom_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/custom_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/custom_metrics_clear") == 0
	) ) {
		return;
	}

	XS_HttpMetricAdd(&g_iXsHttpReqCount, 1);
	bManagePath = (pReq && XS_HttpIsManagePath(pReq->sPath));
	if ( bManagePath ) {
		XS_HttpMetricAdd(&g_iXsHttpManageReqCount, 1);
	} else {
		XS_HttpMetricAdd(&g_iXsHttpAppReqCount, 1);
	}

	iStatusCode = pResp ? pResp->iStatusCode : 0;
	g_iXsHttpLastStatusCode = (int64)iStatusCode;
	g_tXsHttpLastRequestTime = xrtNow();
	g_iXsHttpLastBodyLen = (int64)(pReq ? pReq->iBodyLen : 0);
	g_iXsHttpLastHeaderCount = (int64)(pReq ? pReq->iHeaderCount : 0);
	g_iXsHttpLastQueryLen = XS_HttpQueryLength(pReq);
	sContentType = pReq ? xrtHttpdRequestHeader(pReq, "Content-Type") : NULL;
	sHost = pReq ? xrtHttpdRequestHeader(pReq, "Host") : NULL;
	sUserAgent = pReq ? xrtHttpdRequestHeader(pReq, "User-Agent") : NULL;
	sReferer = pReq ? xrtHttpdRequestHeader(pReq, "Referer") : NULL;
	sOrigin = pReq ? xrtHttpdRequestHeader(pReq, "Origin") : NULL;
	sAccept = pReq ? xrtHttpdRequestHeader(pReq, "Accept") : NULL;
	sAcceptEncoding = pReq ? xrtHttpdRequestHeader(pReq, "Accept-Encoding") : NULL;
	sCookie = pReq ? xrtHttpdRequestHeader(pReq, "Cookie") : NULL;
	sForwardedFor = pReq ? xrtHttpdRequestHeader(pReq, "X-Forwarded-For") : NULL;
	sRealIP = pReq ? xrtHttpdRequestHeader(pReq, "X-Real-IP") : NULL;
	sConnection = pReq ? xrtHttpdRequestHeader(pReq, "Connection") : NULL;
	sCacheControl = pReq ? xrtHttpdRequestHeader(pReq, "Cache-Control") : NULL;
	sRemote = XS_HttpConnRemoteText(pConn);
	if ( pReq && pReq->sPath ) {
		strncpy(g_sXsHttpLastPath, pReq->sPath, sizeof(g_sXsHttpLastPath) - 1);
		g_sXsHttpLastPath[sizeof(g_sXsHttpLastPath) - 1] = '\0';
	} else {
		g_sXsHttpLastPath[0] = '\0';
	}
	if ( pReq && pReq->sTarget ) {
		strncpy(g_sXsHttpLastTarget, pReq->sTarget, sizeof(g_sXsHttpLastTarget) - 1);
		g_sXsHttpLastTarget[sizeof(g_sXsHttpLastTarget) - 1] = '\0';
	} else {
		g_sXsHttpLastTarget[0] = '\0';
	}
	if ( pReq && pReq->sVersion[0] ) {
		strncpy(g_sXsHttpLastVersion, pReq->sVersion, sizeof(g_sXsHttpLastVersion) - 1);
		g_sXsHttpLastVersion[sizeof(g_sXsHttpLastVersion) - 1] = '\0';
	} else {
		g_sXsHttpLastVersion[0] = '\0';
	}
	if ( sRemote && sRemote[0] ) {
		strncpy(g_sXsHttpLastRemote, sRemote, sizeof(g_sXsHttpLastRemote) - 1);
		g_sXsHttpLastRemote[sizeof(g_sXsHttpLastRemote) - 1] = '\0';
	} else {
		g_sXsHttpLastRemote[0] = '\0';
	}
	if ( sContentType && sContentType[0] ) {
		strncpy(g_sXsHttpLastContentType, sContentType, sizeof(g_sXsHttpLastContentType) - 1);
		g_sXsHttpLastContentType[sizeof(g_sXsHttpLastContentType) - 1] = '\0';
	} else {
		g_sXsHttpLastContentType[0] = '\0';
	}
	if ( sHost && sHost[0] ) {
		strncpy(g_sXsHttpLastHost, sHost, sizeof(g_sXsHttpLastHost) - 1);
		g_sXsHttpLastHost[sizeof(g_sXsHttpLastHost) - 1] = '\0';
	} else {
		g_sXsHttpLastHost[0] = '\0';
	}
	if ( sUserAgent && sUserAgent[0] ) {
		strncpy(g_sXsHttpLastUserAgent, sUserAgent, sizeof(g_sXsHttpLastUserAgent) - 1);
		g_sXsHttpLastUserAgent[sizeof(g_sXsHttpLastUserAgent) - 1] = '\0';
	} else {
		g_sXsHttpLastUserAgent[0] = '\0';
	}
	if ( sReferer && sReferer[0] ) {
		strncpy(g_sXsHttpLastReferer, sReferer, sizeof(g_sXsHttpLastReferer) - 1);
		g_sXsHttpLastReferer[sizeof(g_sXsHttpLastReferer) - 1] = '\0';
	} else {
		g_sXsHttpLastReferer[0] = '\0';
	}
	if ( sOrigin && sOrigin[0] ) {
		strncpy(g_sXsHttpLastOrigin, sOrigin, sizeof(g_sXsHttpLastOrigin) - 1);
		g_sXsHttpLastOrigin[sizeof(g_sXsHttpLastOrigin) - 1] = '\0';
	} else {
		g_sXsHttpLastOrigin[0] = '\0';
	}
	if ( sAccept && sAccept[0] ) {
		strncpy(g_sXsHttpLastAccept, sAccept, sizeof(g_sXsHttpLastAccept) - 1);
		g_sXsHttpLastAccept[sizeof(g_sXsHttpLastAccept) - 1] = '\0';
	} else {
		g_sXsHttpLastAccept[0] = '\0';
	}
	if ( sAcceptEncoding && sAcceptEncoding[0] ) {
		strncpy(g_sXsHttpLastAcceptEncoding, sAcceptEncoding, sizeof(g_sXsHttpLastAcceptEncoding) - 1);
		g_sXsHttpLastAcceptEncoding[sizeof(g_sXsHttpLastAcceptEncoding) - 1] = '\0';
	} else {
		g_sXsHttpLastAcceptEncoding[0] = '\0';
	}
	if ( sCookie && sCookie[0] ) {
		strncpy(g_sXsHttpLastCookie, sCookie, sizeof(g_sXsHttpLastCookie) - 1);
		g_sXsHttpLastCookie[sizeof(g_sXsHttpLastCookie) - 1] = '\0';
	} else {
		g_sXsHttpLastCookie[0] = '\0';
	}
	if ( sForwardedFor && sForwardedFor[0] ) {
		strncpy(g_sXsHttpLastForwardedFor, sForwardedFor, sizeof(g_sXsHttpLastForwardedFor) - 1);
		g_sXsHttpLastForwardedFor[sizeof(g_sXsHttpLastForwardedFor) - 1] = '\0';
	} else {
		g_sXsHttpLastForwardedFor[0] = '\0';
	}
	if ( sRealIP && sRealIP[0] ) {
		strncpy(g_sXsHttpLastRealIP, sRealIP, sizeof(g_sXsHttpLastRealIP) - 1);
		g_sXsHttpLastRealIP[sizeof(g_sXsHttpLastRealIP) - 1] = '\0';
	} else {
		g_sXsHttpLastRealIP[0] = '\0';
	}
	if ( sConnection && sConnection[0] ) {
		strncpy(g_sXsHttpLastConnection, sConnection, sizeof(g_sXsHttpLastConnection) - 1);
		g_sXsHttpLastConnection[sizeof(g_sXsHttpLastConnection) - 1] = '\0';
	} else {
		g_sXsHttpLastConnection[0] = '\0';
	}
	if ( sCacheControl && sCacheControl[0] ) {
		strncpy(g_sXsHttpLastCacheControl, sCacheControl, sizeof(g_sXsHttpLastCacheControl) - 1);
		g_sXsHttpLastCacheControl[sizeof(g_sXsHttpLastCacheControl) - 1] = '\0';
	} else {
		g_sXsHttpLastCacheControl[0] = '\0';
	}
	bManagePath = (pReq && XS_HttpIsManagePath(pReq->sPath));
	if ( !bManagePath ) {
		g_iXsHttpLastAppStatusCode = (int64)iStatusCode;
		g_tXsHttpLastAppRequestTime = g_tXsHttpLastRequestTime;
		g_iXsHttpLastAppMethodType = g_iXsHttpLastMethodType;
		g_iXsHttpLastAppBodyLen = g_iXsHttpLastBodyLen;
		g_iXsHttpLastAppHeaderCount = g_iXsHttpLastHeaderCount;
		g_iXsHttpLastAppQueryLen = g_iXsHttpLastQueryLen;
		if ( pReq && pReq->sPath ) {
			strncpy(g_sXsHttpLastAppPath, pReq->sPath, sizeof(g_sXsHttpLastAppPath) - 1);
			g_sXsHttpLastAppPath[sizeof(g_sXsHttpLastAppPath) - 1] = '\0';
		} else {
			g_sXsHttpLastAppPath[0] = '\0';
		}
		if ( pReq && pReq->sTarget ) {
			strncpy(g_sXsHttpLastAppTarget, pReq->sTarget, sizeof(g_sXsHttpLastAppTarget) - 1);
			g_sXsHttpLastAppTarget[sizeof(g_sXsHttpLastAppTarget) - 1] = '\0';
		} else {
			g_sXsHttpLastAppTarget[0] = '\0';
		}
		if ( pReq && pReq->sVersion[0] ) {
			strncpy(g_sXsHttpLastAppVersion, pReq->sVersion, sizeof(g_sXsHttpLastAppVersion) - 1);
			g_sXsHttpLastAppVersion[sizeof(g_sXsHttpLastAppVersion) - 1] = '\0';
		} else {
			g_sXsHttpLastAppVersion[0] = '\0';
		}
		if ( sRemote && sRemote[0] ) {
			strncpy(g_sXsHttpLastAppRemote, sRemote, sizeof(g_sXsHttpLastAppRemote) - 1);
			g_sXsHttpLastAppRemote[sizeof(g_sXsHttpLastAppRemote) - 1] = '\0';
		} else {
			g_sXsHttpLastAppRemote[0] = '\0';
		}
		if ( sContentType && sContentType[0] ) {
			strncpy(g_sXsHttpLastAppContentType, sContentType, sizeof(g_sXsHttpLastAppContentType) - 1);
			g_sXsHttpLastAppContentType[sizeof(g_sXsHttpLastAppContentType) - 1] = '\0';
		} else {
			g_sXsHttpLastAppContentType[0] = '\0';
		}
		if ( sHost && sHost[0] ) {
			strncpy(g_sXsHttpLastAppHost, sHost, sizeof(g_sXsHttpLastAppHost) - 1);
			g_sXsHttpLastAppHost[sizeof(g_sXsHttpLastAppHost) - 1] = '\0';
		} else {
			g_sXsHttpLastAppHost[0] = '\0';
		}
		if ( sUserAgent && sUserAgent[0] ) {
			strncpy(g_sXsHttpLastAppUserAgent, sUserAgent, sizeof(g_sXsHttpLastAppUserAgent) - 1);
			g_sXsHttpLastAppUserAgent[sizeof(g_sXsHttpLastAppUserAgent) - 1] = '\0';
		} else {
			g_sXsHttpLastAppUserAgent[0] = '\0';
		}
		if ( sReferer && sReferer[0] ) {
			strncpy(g_sXsHttpLastAppReferer, sReferer, sizeof(g_sXsHttpLastAppReferer) - 1);
			g_sXsHttpLastAppReferer[sizeof(g_sXsHttpLastAppReferer) - 1] = '\0';
		} else {
			g_sXsHttpLastAppReferer[0] = '\0';
		}
		if ( sOrigin && sOrigin[0] ) {
			strncpy(g_sXsHttpLastAppOrigin, sOrigin, sizeof(g_sXsHttpLastAppOrigin) - 1);
			g_sXsHttpLastAppOrigin[sizeof(g_sXsHttpLastAppOrigin) - 1] = '\0';
		} else {
			g_sXsHttpLastAppOrigin[0] = '\0';
		}
		if ( sAccept && sAccept[0] ) {
			strncpy(g_sXsHttpLastAppAccept, sAccept, sizeof(g_sXsHttpLastAppAccept) - 1);
			g_sXsHttpLastAppAccept[sizeof(g_sXsHttpLastAppAccept) - 1] = '\0';
		} else {
			g_sXsHttpLastAppAccept[0] = '\0';
		}
		if ( sAcceptEncoding && sAcceptEncoding[0] ) {
			strncpy(g_sXsHttpLastAppAcceptEncoding, sAcceptEncoding, sizeof(g_sXsHttpLastAppAcceptEncoding) - 1);
			g_sXsHttpLastAppAcceptEncoding[sizeof(g_sXsHttpLastAppAcceptEncoding) - 1] = '\0';
		} else {
			g_sXsHttpLastAppAcceptEncoding[0] = '\0';
		}
		if ( sCookie && sCookie[0] ) {
			strncpy(g_sXsHttpLastAppCookie, sCookie, sizeof(g_sXsHttpLastAppCookie) - 1);
			g_sXsHttpLastAppCookie[sizeof(g_sXsHttpLastAppCookie) - 1] = '\0';
		} else {
			g_sXsHttpLastAppCookie[0] = '\0';
		}
		if ( sForwardedFor && sForwardedFor[0] ) {
			strncpy(g_sXsHttpLastAppForwardedFor, sForwardedFor, sizeof(g_sXsHttpLastAppForwardedFor) - 1);
			g_sXsHttpLastAppForwardedFor[sizeof(g_sXsHttpLastAppForwardedFor) - 1] = '\0';
		} else {
			g_sXsHttpLastAppForwardedFor[0] = '\0';
		}
		if ( sRealIP && sRealIP[0] ) {
			strncpy(g_sXsHttpLastAppRealIP, sRealIP, sizeof(g_sXsHttpLastAppRealIP) - 1);
			g_sXsHttpLastAppRealIP[sizeof(g_sXsHttpLastAppRealIP) - 1] = '\0';
		} else {
			g_sXsHttpLastAppRealIP[0] = '\0';
		}
		if ( sConnection && sConnection[0] ) {
			strncpy(g_sXsHttpLastAppConnection, sConnection, sizeof(g_sXsHttpLastAppConnection) - 1);
			g_sXsHttpLastAppConnection[sizeof(g_sXsHttpLastAppConnection) - 1] = '\0';
		} else {
			g_sXsHttpLastAppConnection[0] = '\0';
		}
		if ( sCacheControl && sCacheControl[0] ) {
			strncpy(g_sXsHttpLastAppCacheControl, sCacheControl, sizeof(g_sXsHttpLastAppCacheControl) - 1);
			g_sXsHttpLastAppCacheControl[sizeof(g_sXsHttpLastAppCacheControl) - 1] = '\0';
		} else {
			g_sXsHttpLastAppCacheControl[0] = '\0';
		}
	}
	if ( (iStatusCode >= 200u) && (iStatusCode < 300u) ) {
		XS_HttpMetricAdd(&g_iXsHttpResp2xxCount, 1);
	} else if ( (iStatusCode >= 300u) && (iStatusCode < 400u) ) {
		XS_HttpMetricAdd(&g_iXsHttpResp3xxCount, 1);
	} else if ( (iStatusCode >= 400u) && (iStatusCode < 500u) ) {
		XS_HttpMetricAdd(&g_iXsHttpResp4xxCount, 1);
	} else if ( iStatusCode >= 500u ) {
		XS_HttpMetricAdd(&g_iXsHttpResp5xxCount, 1);
	}
	if ( iStatusCode >= 400u ) {
		char sRejectReason[128];

		XS_HttpRecordRejectEvent(
			(int64)iStatusCode,
			XS_HttpRejectReasonTextEx(pReq, pResp, sRejectReason, sizeof(sRejectReason))
		);
	}
}

static inline void XS_HttpRecordMethodMetrics(const xhttpdrequest* pReq)
{
	if ( pReq == NULL || pReq->sMethod == NULL ) {
		XS_HttpMetricAdd(&g_iXsHttpMethodOtherCount, 1);
		g_iXsHttpLastMethodType = 4;
		return;
	}

	if ( _stricmp(pReq->sMethod, "GET") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpMethodGetCount, 1);
		g_iXsHttpLastMethodType = 1;
	} else if ( _stricmp(pReq->sMethod, "POST") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpMethodPostCount, 1);
		g_iXsHttpLastMethodType = 2;
	} else if ( _stricmp(pReq->sMethod, "HEAD") == 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpMethodHeadCount, 1);
		g_iXsHttpLastMethodType = 3;
	} else {
		XS_HttpMetricAdd(&g_iXsHttpMethodOtherCount, 1);
		g_iXsHttpLastMethodType = 4;
	}
}

static inline void XS_HttpRecordTimeMetrics(const xhttpdrequest* pReq, int64 iElapsedMS)
{
	bool bManagePath;

	if ( iElapsedMS < 0 ) {
		iElapsedMS = 0;
	}

	if ( pReq && (
		strcmp(pReq->sPath, "/__xs/http_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/http_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/http_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/ws_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/ws_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/ws_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/xtp_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/xtp_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/xtp_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/udp_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/udp_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/udp_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/custom_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/custom_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/custom_metrics_clear") == 0
	) ) {
		return;
	}

	bManagePath = (pReq && XS_HttpIsManagePath(pReq->sPath));
	XS_HttpMetricAdd(&g_iXsHttpTimeTotalMS, iElapsedMS);
	XS_HttpMetricUpdateMax(&g_iXsHttpTimeMaxMS, iElapsedMS);
	g_iXsHttpLastTimeMS = iElapsedMS;
	if ( !bManagePath ) {
		g_iXsHttpLastAppTimeMS = iElapsedMS;
	}
}

static inline void XS_HttpOnOpenMetrics(void)
{
	int64 iCurrent;
	int64 iPeak;

	iCurrent = XS_HttpMetricAdd(&g_iXsHttpConnCurrent, 1);
	for ( ; ; ) {
		iPeak = XS_HttpMetricGet(&g_iXsHttpConnPeak);
		if ( iCurrent <= iPeak ) {
			break;
		}
		if ( XS_HttpMetricCompareExchange(&g_iXsHttpConnPeak, iCurrent, iPeak) == iPeak ) {
			break;
		}
	}
}

static inline void XS_HttpOnCloseMetrics(void)
{
	int64 iCurrent;

	iCurrent = XS_HttpMetricAdd(&g_iXsHttpConnCurrent, -1);
	if ( iCurrent < 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpConnCurrent, -iCurrent);
	}
}

static uint32 XS_HttpIdleThread(ptr pArg)
{
	XS_HttpHandle* objHandle = (XS_HttpHandle*)pArg;

	while ( objHandle && !objHandle->bStopThread ) {
		XS_ServerConfig* objServer = objHandle->pOwner;
		uint32 iIdleTimeout = objServer ? objServer->IdleTimeout : 0u;

		if ( iIdleTimeout > 0 && objHandle->pConnLock && objHandle->arrConn ) {
			xarray arrClose = xrtArrayCreate(sizeof(XS_HttpConnContext*), XRT_OBJMODE_LOCAL);
			int64 iNowMS = XS_HttpNowMS();
			uint32 i;

			xrtMutexLock(objHandle->pConnLock);
			for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
				XS_HttpConnContext** ppItem = (XS_HttpConnContext**)xrtArrayGet(objHandle->arrConn, i);
				XS_HttpConnContext* objCtx = ppItem ? *ppItem : NULL;

				if ( objCtx == NULL || objCtx->pConn == NULL || objCtx->bClosing ) {
					continue;
				}
				if ( (objCtx->iLastActiveMS > 0) && ((iNowMS - objCtx->iLastActiveMS) >= (int64)iIdleTimeout) ) {
					XS_HttpConnContext** ppClose;
					uint32 iPos;

					objCtx->bClosing = TRUE;
					iPos = xrtArrayAppend(arrClose, 1);
					ppClose = (XS_HttpConnContext**)xrtArrayGet(arrClose, iPos);
					if ( ppClose ) {
						*ppClose = objCtx;
					}
				}
			}
			xrtMutexUnlock(objHandle->pConnLock);

			for ( i = 1; i <= arrClose->Count; i++ ) {
				XS_HttpConnContext** ppClose = (XS_HttpConnContext**)xrtArrayGet(arrClose, i);
				XS_HttpConnContext* objCtx = ppClose ? *ppClose : NULL;
				xhttpdconn* pConn;
				xnetstream* pStream;
				const char* sRemote;

				if ( objCtx == NULL ) {
					continue;
				}

				pConn = objCtx->pConn;
				pStream = (pConn && pConn->pStream) ? pConn->pStream : NULL;
				sRemote = XS_HttpConnRemoteText(pConn);
				XS_HttpRecordIdleClose();
				XS_LogInfo(
					"http idle close: server=%s timeout=%u remote=%s",
					objServer && objServer->Name ? objServer->Name : "(null)",
					(unsigned)iIdleTimeout,
					(sRemote && sRemote[0]) ? sRemote : "(none)"
				);
				if ( pStream ) {
					xrtNetStreamClose(pStream, 0u);
				}
			}
			xrtArrayDestroy(arrClose);
		}

		XS_HttpSleepMS(500);
	}

	return 0;
}

static inline void XS_HttpClearMetrics(void)
{
	int64 iValue;

	iValue = XS_HttpMetricGet(&g_iXsHttpReqCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpReqCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpManageReqCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpManageReqCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpAppReqCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpAppReqCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsHttpResp2xxCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpResp2xxCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsHttpResp3xxCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpResp3xxCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsHttpResp4xxCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpResp4xxCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsHttpResp5xxCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpResp5xxCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsHttpConnPeak);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpConnPeak, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpIdleCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpIdleCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpConnLimitCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpConnLimitCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpStopCleanupCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpStopCleanupCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpHeaderLimitRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpHeaderLimitRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpBodyLimitRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpBodyLimitRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpPathLimitRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpPathLimitRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpApiDisabledRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpApiDisabledRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpMethodRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpMethodRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpHostNotFoundRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpHostNotFoundRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpReloadBusyRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpReloadBusyRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpReloadFailedRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpReloadFailedRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpCheckConfigFailedRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpCheckConfigFailedRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpBusBadRequestRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpBusBadRequestRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpBusNotFoundRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpBusNotFoundRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpBusLimitRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpBusLimitRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsHttpBusFailedRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpBusFailedRejectCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsHttpMethodGetCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpMethodGetCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsHttpMethodPostCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpMethodPostCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsHttpMethodHeadCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpMethodHeadCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsHttpMethodOtherCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpMethodOtherCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsHttpTimeTotalMS);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpTimeTotalMS, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsHttpTimeMaxMS);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpTimeMaxMS, -iValue);
	}
	g_iXsHttpLastTimeMS = 0;
	g_iXsHttpLastAppTimeMS = 0;
	g_iXsHttpLastBodyLen = 0;
	g_iXsHttpLastAppBodyLen = 0;
	g_iXsHttpLastHeaderCount = 0;
	g_iXsHttpLastAppHeaderCount = 0;
	g_iXsHttpLastQueryLen = 0;
	g_iXsHttpLastAppQueryLen = 0;

	g_iXsHttpLastMethodType = 0;
	g_iXsHttpLastAppMethodType = 0;
	g_iXsHttpLastStatusCode = 0;
	g_iXsHttpLastAppStatusCode = 0;
	g_iXsHttpLastRejectStatus = 0;
	g_tXsHttpLastRequestTime = 0;
	g_tXsHttpLastAppRequestTime = 0;
	g_tXsHttpLastIdleCloseTime = 0;
	g_tXsHttpLastConnLimitCloseTime = 0;
	g_tXsHttpLastStopCleanupTime = 0;
	g_tXsHttpLastRejectTime = 0;
	g_tXsHttpLastHeaderLimitRejectTime = 0;
	g_tXsHttpLastBodyLimitRejectTime = 0;
	g_tXsHttpLastPathLimitRejectTime = 0;
	g_tXsHttpLastApiDisabledRejectTime = 0;
	g_tXsHttpLastMethodRejectTime = 0;
	g_tXsHttpLastHostNotFoundRejectTime = 0;
	g_tXsHttpLastReloadBusyRejectTime = 0;
	g_tXsHttpLastReloadFailedRejectTime = 0;
	g_tXsHttpLastCheckConfigFailedRejectTime = 0;
	g_tXsHttpLastBusBadRequestRejectTime = 0;
	g_tXsHttpLastBusNotFoundRejectTime = 0;
	g_tXsHttpLastBusLimitRejectTime = 0;
	g_tXsHttpLastBusFailedRejectTime = 0;
	g_iXsHttpLastStopCleanupClosed = 0;
	g_iXsHttpLastStopCleanupRemain = 0;
	g_sXsHttpLastPath[0] = '\0';
	g_sXsHttpLastTarget[0] = '\0';
	g_sXsHttpLastVersion[0] = '\0';
	g_sXsHttpLastRemote[0] = '\0';
	g_sXsHttpLastContentType[0] = '\0';
	g_sXsHttpLastHost[0] = '\0';
	g_sXsHttpLastUserAgent[0] = '\0';
	g_sXsHttpLastReferer[0] = '\0';
	g_sXsHttpLastOrigin[0] = '\0';
	g_sXsHttpLastAccept[0] = '\0';
	g_sXsHttpLastAcceptEncoding[0] = '\0';
	g_sXsHttpLastCookie[0] = '\0';
	g_sXsHttpLastForwardedFor[0] = '\0';
	g_sXsHttpLastRealIP[0] = '\0';
	g_sXsHttpLastConnection[0] = '\0';
	g_sXsHttpLastCacheControl[0] = '\0';
	g_sXsHttpLastAppPath[0] = '\0';
	g_sXsHttpLastAppTarget[0] = '\0';
	g_sXsHttpLastAppVersion[0] = '\0';
	g_sXsHttpLastAppRemote[0] = '\0';
	g_sXsHttpLastAppContentType[0] = '\0';
	g_sXsHttpLastAppHost[0] = '\0';
	g_sXsHttpLastAppUserAgent[0] = '\0';
	g_sXsHttpLastAppReferer[0] = '\0';
	g_sXsHttpLastAppOrigin[0] = '\0';
	g_sXsHttpLastAppAccept[0] = '\0';
	g_sXsHttpLastAppAcceptEncoding[0] = '\0';
	g_sXsHttpLastAppCookie[0] = '\0';
	g_sXsHttpLastAppForwardedFor[0] = '\0';
	g_sXsHttpLastAppRealIP[0] = '\0';
	g_sXsHttpLastAppConnection[0] = '\0';
	g_sXsHttpLastAppCacheControl[0] = '\0';
	g_sXsHttpLastRejectReason[0] = '\0';
}

static inline void XS_WsClearMetrics(void)
{
	int64 iValue;

	iValue = XS_HttpMetricGet(&g_iXsWsConnCurrent);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsConnCurrent, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsWsConnPeak);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsConnPeak, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsWsOpenCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsOpenCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsWsCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsCloseCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsWsTextCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsTextCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsWsBinaryCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsBinaryCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsWsPingCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsPingCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsWsPongCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsPongCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsWsErrorCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsErrorCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsWsInvalidCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsInvalidCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsWsIdleCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsIdleCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsWsConnLimitCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsConnLimitCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsWsMessageLimitCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsMessageLimitCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsWsStopCleanupCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsStopCleanupCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsWsRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsWsRejectCount, -iValue);
	}
	
	g_iXsWsLastFrameType = 0;
	g_iXsWsLastBytes = 0;
	g_iXsWsIdleCloseCount = 0;
	g_iXsWsConnLimitCloseCount = 0;
	g_iXsWsMessageLimitCloseCount = 0;
	g_tXsWsLastTime = 0;
	g_tXsWsLastCloseTime = 0;
	g_tXsWsLastErrorTime = 0;
	g_tXsWsLastInvalidTime = 0;
	g_tXsWsLastIdleCloseTime = 0;
	g_tXsWsLastConnLimitCloseTime = 0;
	g_tXsWsLastMessageLimitCloseTime = 0;
	g_tXsWsLastStopCleanupTime = 0;
	g_tXsWsLastRejectTime = 0;
	g_iXsWsLastErrorCode = 0;
	g_iXsWsLastCloseReason = 0;
	g_iXsWsLastStopCleanupClosed = 0;
	g_iXsWsLastStopCleanupRemain = 0;
	g_sXsWsLastText[0] = '\0';
	g_sXsWsLastRemote[0] = '\0';
	g_sXsWsLastInvalidReason[0] = '\0';
	g_sXsWsLastRejectReason[0] = '\0';
}

static inline char* XS_WsLastIdleCloseTimeText(void)
{
	xtime tLast = g_tXsWsLastIdleCloseTime;

	if ( tLast <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(tLast, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_WsLastConnLimitCloseTimeText(void)
{
	xtime tLast = g_tXsWsLastConnLimitCloseTime;

	if ( tLast <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(tLast, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_WsLastMessageLimitCloseTimeText(void)
{
	xtime tLast = g_tXsWsLastMessageLimitCloseTime;

	if ( tLast <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(tLast, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_WsLastStopCleanupTimeText(void)
{
	xtime tLast = g_tXsWsLastStopCleanupTime;

	if ( tLast <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(tLast, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_WsLastIdleCloseAgeMS(void)
{
	xtime tLast = g_tXsWsLastIdleCloseTime;

	if ( tLast <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - tLast) * 1000;
}

static inline int64 XS_WsLastConnLimitCloseAgeMS(void)
{
	xtime tLast = g_tXsWsLastConnLimitCloseTime;

	if ( tLast <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - tLast) * 1000;
}

static inline int64 XS_WsLastMessageLimitCloseAgeMS(void)
{
	xtime tLast = g_tXsWsLastMessageLimitCloseTime;

	if ( tLast <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - tLast) * 1000;
}

static inline int64 XS_WsLastStopCleanupAgeMS(void)
{
	xtime tLast = g_tXsWsLastStopCleanupTime;

	if ( tLast <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - tLast) * 1000;
}

static inline char* XS_WsLastRejectTimeText(void)
{
	xtime tLast = g_tXsWsLastRejectTime;

	if ( tLast <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(tLast, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_WsLastInvalidTimeText(void)
{
	xtime tLast = g_tXsWsLastInvalidTime;

	if ( tLast <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(tLast, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_WsLastRejectAgeMS(void)
{
	xtime tLast = g_tXsWsLastRejectTime;

	if ( tLast <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - tLast) * 1000;
}

static inline int64 XS_WsLastInvalidAgeMS(void)
{
	xtime tLast = g_tXsWsLastInvalidTime;

	if ( tLast <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - tLast) * 1000;
}

static inline void XS_XtpClearMetrics(void)
{
	int64 iValue;

	iValue = XS_HttpMetricGet(&g_iXsXtpConnCurrent);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpConnCurrent, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpConnPeak);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpConnPeak, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpOpenCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpOpenCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpErrorCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpErrorCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpMsgCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpMsgCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpReqCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpReqCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpRespCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpRespCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpPushCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpPushCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpEventCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpEventCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpSendCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpSendCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpRecvBytes);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpRecvBytes, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpSendBytes);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpSendBytes, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpIdleCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpIdleCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpConnLimitCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpConnLimitCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpRecvLimitCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpRecvLimitCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpStopCleanupCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpStopCleanupCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsXtpRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsXtpRejectCount, -iValue);
	}
	g_iXsXtpLastMsgType = 0;
	g_iXsXtpLastStatus = 0;
	g_iXsXtpLastMsgID = 0;
	g_iXsXtpLastFlags = 0;
	g_iXsXtpLastParamCount = 0;
	g_iXsXtpLastBodySize = 0;
	g_iXsXtpLastBytes = 0;
	g_tXsXtpLastTime = 0;
	g_sXsXtpLastCmd[0] = '\0';
	g_sXsXtpLastRemote[0] = '\0';
	g_tXsXtpLastInvalidTime = 0;
	g_sXsXtpLastInvalidReason[0] = '\0';
	g_tXsXtpLastErrorTime = 0;
	g_iXsXtpLastErrorCode = 0;
	g_tXsXtpLastIdleCloseTime = 0;
	g_tXsXtpLastConnLimitCloseTime = 0;
	g_tXsXtpLastRecvLimitCloseTime = 0;
	g_tXsXtpLastStopCleanupTime = 0;
	g_tXsXtpLastRejectTime = 0;
	g_iXsXtpRecvLimitCloseCount = 0;
	g_iXsXtpLastStopCleanupClosed = 0;
	g_iXsXtpLastStopCleanupRemain = 0;
	g_sXsXtpLastRejectReason[0] = '\0';
}

static inline void XS_UdpClearMetrics(void)
{
	int64 iValue;

	iValue = XS_HttpMetricGet(&g_iXsUdpRecvCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsUdpRecvCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsUdpSendCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsUdpSendCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsUdpErrorCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsUdpErrorCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsUdpRecvBytes);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsUdpRecvBytes, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsUdpSendBytes);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsUdpSendBytes, -iValue);
	}
	g_tXsUdpLastTime = 0;
	g_tXsUdpLastErrorTime = 0;
	g_iXsUdpLastErrorCode = 0;
	g_iXsUdpLastBytes = 0;
	g_sXsUdpLastFrom[0] = '\0';
	g_sXsUdpLastText[0] = '\0';
}

static inline void XS_CustomClearMetrics(void)
{
	int64 iValue;

	iValue = XS_HttpMetricGet(&g_iXsCustomConnCurrent);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomConnCurrent, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomConnPeak);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomConnPeak, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomOpenCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomOpenCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomErrorCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomErrorCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomInvalidCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomInvalidCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomRecvCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomRecvCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomSendCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomSendCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomRecvBytes);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomRecvBytes, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomSendBytes);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomSendBytes, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomIdleCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomIdleCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomConnLimitCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomConnLimitCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomRecvLimitCloseCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomRecvLimitCloseCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomStopCleanupCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomStopCleanupCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsCustomRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCustomRejectCount, -iValue);
	}
	g_tXsCustomLastTime = 0;
	g_tXsCustomLastErrorTime = 0;
	g_tXsCustomLastInvalidTime = 0;
	g_tXsCustomLastIdleCloseTime = 0;
	g_tXsCustomLastConnLimitCloseTime = 0;
	g_tXsCustomLastRecvLimitCloseTime = 0;
	g_tXsCustomLastStopCleanupTime = 0;
	g_tXsCustomLastRejectTime = 0;
	g_tXsCustomLastCloseTime = 0;
	g_iXsCustomLastCloseReason = 0;
	g_iXsCustomLastErrorCode = 0;
	g_iXsCustomLastBytes = 0;
	g_iXsCustomRecvLimitCloseCount = 0;
	g_iXsCustomLastStopCleanupClosed = 0;
	g_iXsCustomLastStopCleanupRemain = 0;
	g_sXsCustomLastText[0] = '\0';
	g_sXsCustomLastRemote[0] = '\0';
	g_sXsCustomLastInvalidReason[0] = '\0';
	g_sXsCustomLastRejectReason[0] = '\0';
}

static inline const char* XS_HttpLastMethodName(void)
{
	switch ( XS_HttpMetricGet(&g_iXsHttpLastMethodType) ) {
		case 1: return "GET";
		case 2: return "POST";
		case 3: return "HEAD";
		case 4: return "OTHER";
		default: return "";
	}
}

static inline const char* XS_HttpLastAppMethodName(void)
{
	switch ( XS_HttpMetricGet(&g_iXsHttpLastAppMethodType) ) {
		case 1: return "GET";
		case 2: return "POST";
		case 3: return "HEAD";
		case 4: return "OTHER";
		default: return "";
	}
}

static inline char* XS_HttpLastRequestTimeText(void)
{
	if ( g_tXsHttpLastRequestTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastRequestTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastIdleCloseTimeText(void)
{
	if ( g_tXsHttpLastIdleCloseTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastIdleCloseTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastConnLimitCloseTimeText(void)
{
	if ( g_tXsHttpLastConnLimitCloseTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastConnLimitCloseTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastStopCleanupTimeText(void)
{
	if ( g_tXsHttpLastStopCleanupTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastStopCleanupTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastRejectTimeText(void)
{
	if ( g_tXsHttpLastRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastHeaderLimitRejectTimeText(void)
{
	if ( g_tXsHttpLastHeaderLimitRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastHeaderLimitRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastBodyLimitRejectTimeText(void)
{
	if ( g_tXsHttpLastBodyLimitRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastBodyLimitRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastPathLimitRejectTimeText(void)
{
	if ( g_tXsHttpLastPathLimitRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastPathLimitRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastApiDisabledRejectTimeText(void)
{
	if ( g_tXsHttpLastApiDisabledRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastApiDisabledRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastMethodRejectTimeText(void)
{
	if ( g_tXsHttpLastMethodRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastMethodRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastHostNotFoundRejectTimeText(void)
{
	if ( g_tXsHttpLastHostNotFoundRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastHostNotFoundRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastReloadBusyRejectTimeText(void)
{
	if ( g_tXsHttpLastReloadBusyRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastReloadBusyRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastReloadFailedRejectTimeText(void)
{
	if ( g_tXsHttpLastReloadFailedRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastReloadFailedRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastCheckConfigFailedRejectTimeText(void)
{
	if ( g_tXsHttpLastCheckConfigFailedRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastCheckConfigFailedRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastBusBadRequestRejectTimeText(void)
{
	if ( g_tXsHttpLastBusBadRequestRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastBusBadRequestRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastBusNotFoundRejectTimeText(void)
{
	if ( g_tXsHttpLastBusNotFoundRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastBusNotFoundRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastBusLimitRejectTimeText(void)
{
	if ( g_tXsHttpLastBusLimitRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastBusLimitRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_HttpLastBusFailedRejectTimeText(void)
{
	if ( g_tXsHttpLastBusFailedRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastBusFailedRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline const char* XS_XtpLastMsgTypeName(void)
{
	switch ( XS_HttpMetricGet(&g_iXsXtpLastMsgType) ) {
		case 1: return "request";
		case 2: return "response";
		case 3: return "push";
		case 4: return "event";
		default: return "";
	}
}

static inline char* XS_XtpLastTimeText(void)
{
	if ( g_tXsXtpLastTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsXtpLastTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_XtpLastAgeMS(void)
{
	if ( g_tXsXtpLastTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsXtpLastTime) * 1000;
}

static inline char* XS_XtpLastInvalidTimeText(void)
{
	if ( g_tXsXtpLastInvalidTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsXtpLastInvalidTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_XtpLastInvalidAgeMS(void)
{
	if ( g_tXsXtpLastInvalidTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsXtpLastInvalidTime) * 1000;
}

static inline char* XS_XtpLastErrorTimeText(void)
{
	if ( g_tXsXtpLastErrorTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsXtpLastErrorTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_XtpLastErrorAgeMS(void)
{
	if ( g_tXsXtpLastErrorTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsXtpLastErrorTime) * 1000;
}

static inline char* XS_XtpLastIdleCloseTimeText(void)
{
	if ( g_tXsXtpLastIdleCloseTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsXtpLastIdleCloseTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_XtpLastConnLimitCloseTimeText(void)
{
	if ( g_tXsXtpLastConnLimitCloseTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsXtpLastConnLimitCloseTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_XtpLastRecvLimitCloseTimeText(void)
{
	if ( g_tXsXtpLastRecvLimitCloseTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsXtpLastRecvLimitCloseTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_XtpLastStopCleanupTimeText(void)
{
	if ( g_tXsXtpLastStopCleanupTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsXtpLastStopCleanupTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_XtpLastRejectTimeText(void)
{
	if ( g_tXsXtpLastRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsXtpLastRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_XtpLastIdleCloseAgeMS(void)
{
	if ( g_tXsXtpLastIdleCloseTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsXtpLastIdleCloseTime) * 1000;
}

static inline int64 XS_XtpLastConnLimitCloseAgeMS(void)
{
	if ( g_tXsXtpLastConnLimitCloseTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsXtpLastConnLimitCloseTime) * 1000;
}

static inline int64 XS_XtpLastRecvLimitCloseAgeMS(void)
{
	if ( g_tXsXtpLastRecvLimitCloseTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsXtpLastRecvLimitCloseTime) * 1000;
}

static inline int64 XS_XtpLastStopCleanupAgeMS(void)
{
	if ( g_tXsXtpLastStopCleanupTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsXtpLastStopCleanupTime) * 1000;
}

static inline int64 XS_XtpLastRejectAgeMS(void)
{
	if ( g_tXsXtpLastRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsXtpLastRejectTime) * 1000;
}

static inline char* XS_UdpLastTimeText(void)
{
	if ( g_tXsUdpLastTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsUdpLastTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_UdpLastAgeMS(void)
{
	if ( g_tXsUdpLastTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsUdpLastTime) * 1000;
}

static inline char* XS_UdpLastErrorTimeText(void)
{
	if ( g_tXsUdpLastErrorTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsUdpLastErrorTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_UdpLastErrorAgeMS(void)
{
	if ( g_tXsUdpLastErrorTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsUdpLastErrorTime) * 1000;
}

static inline char* XS_CustomLastTimeText(void)
{
	if ( g_tXsCustomLastTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsCustomLastTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_CustomLastAgeMS(void)
{
	if ( g_tXsCustomLastTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsCustomLastTime) * 1000;
}

static inline char* XS_CustomLastErrorTimeText(void)
{
	if ( g_tXsCustomLastErrorTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsCustomLastErrorTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_CustomLastCloseTimeText(void)
{
	if ( g_tXsCustomLastCloseTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsCustomLastCloseTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_CustomLastCloseAgeMS(void)
{
	if ( g_tXsCustomLastCloseTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsCustomLastCloseTime) * 1000;
}

static inline int64 XS_CustomLastErrorAgeMS(void)
{
	if ( g_tXsCustomLastErrorTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsCustomLastErrorTime) * 1000;
}

static inline char* XS_CustomLastInvalidTimeText(void)
{
	if ( g_tXsCustomLastInvalidTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsCustomLastInvalidTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_CustomLastInvalidAgeMS(void)
{
	if ( g_tXsCustomLastInvalidTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsCustomLastInvalidTime) * 1000;
}

static inline char* XS_CustomLastIdleCloseTimeText(void)
{
	if ( g_tXsCustomLastIdleCloseTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsCustomLastIdleCloseTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_CustomLastConnLimitCloseTimeText(void)
{
	if ( g_tXsCustomLastConnLimitCloseTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsCustomLastConnLimitCloseTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_CustomLastRecvLimitCloseTimeText(void)
{
	if ( g_tXsCustomLastRecvLimitCloseTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsCustomLastRecvLimitCloseTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_CustomLastStopCleanupTimeText(void)
{
	if ( g_tXsCustomLastStopCleanupTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsCustomLastStopCleanupTime, XRT_TIME_FORMAT_DATETIME);
}

static inline char* XS_CustomLastRejectTimeText(void)
{
	if ( g_tXsCustomLastRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsCustomLastRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_CustomLastIdleCloseAgeMS(void)
{
	if ( g_tXsCustomLastIdleCloseTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsCustomLastIdleCloseTime) * 1000;
}

static inline int64 XS_CustomLastConnLimitCloseAgeMS(void)
{
	if ( g_tXsCustomLastConnLimitCloseTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsCustomLastConnLimitCloseTime) * 1000;
}

static inline int64 XS_CustomLastRecvLimitCloseAgeMS(void)
{
	if ( g_tXsCustomLastRecvLimitCloseTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsCustomLastRecvLimitCloseTime) * 1000;
}

static inline int64 XS_CustomLastStopCleanupAgeMS(void)
{
	if ( g_tXsCustomLastStopCleanupTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsCustomLastStopCleanupTime) * 1000;
}

static inline int64 XS_CustomLastRejectAgeMS(void)
{
	if ( g_tXsCustomLastRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsCustomLastRejectTime) * 1000;
}

static inline int64 XS_HttpLastRequestAgeMS(void)
{
	if ( g_tXsHttpLastRequestTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastRequestTime) * 1000;
}

static inline int64 XS_HttpLastIdleCloseAgeMS(void)
{
	if ( g_tXsHttpLastIdleCloseTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastIdleCloseTime) * 1000;
}

static inline int64 XS_HttpLastConnLimitCloseAgeMS(void)
{
	if ( g_tXsHttpLastConnLimitCloseTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastConnLimitCloseTime) * 1000;
}

static inline int64 XS_HttpLastStopCleanupAgeMS(void)
{
	if ( g_tXsHttpLastStopCleanupTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastStopCleanupTime) * 1000;
}

static inline int64 XS_HttpLastRejectAgeMS(void)
{
	if ( g_tXsHttpLastRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastRejectTime) * 1000;
}

static inline int64 XS_HttpLastHeaderLimitRejectAgeMS(void)
{
	if ( g_tXsHttpLastHeaderLimitRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastHeaderLimitRejectTime) * 1000;
}

static inline int64 XS_HttpLastBodyLimitRejectAgeMS(void)
{
	if ( g_tXsHttpLastBodyLimitRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastBodyLimitRejectTime) * 1000;
}

static inline int64 XS_HttpLastPathLimitRejectAgeMS(void)
{
	if ( g_tXsHttpLastPathLimitRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastPathLimitRejectTime) * 1000;
}

static inline int64 XS_HttpLastApiDisabledRejectAgeMS(void)
{
	if ( g_tXsHttpLastApiDisabledRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastApiDisabledRejectTime) * 1000;
}

static inline int64 XS_HttpLastMethodRejectAgeMS(void)
{
	if ( g_tXsHttpLastMethodRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastMethodRejectTime) * 1000;
}

static inline int64 XS_HttpLastHostNotFoundRejectAgeMS(void)
{
	if ( g_tXsHttpLastHostNotFoundRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastHostNotFoundRejectTime) * 1000;
}

static inline int64 XS_HttpLastReloadBusyRejectAgeMS(void)
{
	if ( g_tXsHttpLastReloadBusyRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastReloadBusyRejectTime) * 1000;
}

static inline int64 XS_HttpLastReloadFailedRejectAgeMS(void)
{
	if ( g_tXsHttpLastReloadFailedRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastReloadFailedRejectTime) * 1000;
}

static inline int64 XS_HttpLastCheckConfigFailedRejectAgeMS(void)
{
	if ( g_tXsHttpLastCheckConfigFailedRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastCheckConfigFailedRejectTime) * 1000;
}

static inline int64 XS_HttpLastBusBadRequestRejectAgeMS(void)
{
	if ( g_tXsHttpLastBusBadRequestRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastBusBadRequestRejectTime) * 1000;
}

static inline int64 XS_HttpLastBusNotFoundRejectAgeMS(void)
{
	if ( g_tXsHttpLastBusNotFoundRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastBusNotFoundRejectTime) * 1000;
}

static inline int64 XS_HttpLastBusLimitRejectAgeMS(void)
{
	if ( g_tXsHttpLastBusLimitRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastBusLimitRejectTime) * 1000;
}

static inline int64 XS_HttpLastBusFailedRejectAgeMS(void)
{
	if ( g_tXsHttpLastBusFailedRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastBusFailedRejectTime) * 1000;
}

static inline int64 XS_HttpLastAppRequestAgeMS(void)
{
	if ( g_tXsHttpLastAppRequestTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastAppRequestTime) * 1000;
}

static inline const char* XS_HttpLastPath(void)
{
	return g_sXsHttpLastPath;
}

static inline const char* XS_HttpLastTarget(void)
{
	return g_sXsHttpLastTarget;
}

static inline const char* XS_HttpLastRemote(void)
{
	return g_sXsHttpLastRemote;
}

static inline char* XS_HttpLastAppRequestTimeText(void)
{
	if ( g_tXsHttpLastAppRequestTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsHttpLastAppRequestTime, XRT_TIME_FORMAT_DATETIME);
}

static inline const char* XS_HttpLastAppPath(void)
{
	return g_sXsHttpLastAppPath;
}

static inline const char* XS_HttpLastAppTarget(void)
{
	return g_sXsHttpLastAppTarget;
}

static inline const char* XS_HttpLastAppRemote(void)
{
	return g_sXsHttpLastAppRemote;
}

static inline void XS_HttpAppendRejectMetrics(xvalue objValue)
{
	char* sLastTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_HttpLastRejectTimeText();
	xvoTableSetInt(objValue, "http_reject_count", sizeof("http_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpRejectCount));
	xvoTableSetInt(objValue, "http_last_reject_status", sizeof("http_last_reject_status") - 1, XS_HttpMetricGet(&g_iXsHttpLastRejectStatus));
	xvoTableSetText(objValue, "http_last_reject_reason", sizeof("http_last_reject_reason") - 1, (ptr)g_sXsHttpLastRejectReason, 0, FALSE);
	xvoTableSetText(objValue, "http_last_reject_time", sizeof("http_last_reject_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_reject_age_ms", sizeof("http_last_reject_age_ms") - 1, XS_HttpLastRejectAgeMS());
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
}

static inline void XS_HttpAppendRequestLimitMetrics(xvalue objValue)
{
	char* sHeaderTime;
	char* sBodyTime;
	char* sPathTime;

	if ( objValue == NULL ) {
		return;
	}

	sHeaderTime = XS_HttpLastHeaderLimitRejectTimeText();
	sBodyTime = XS_HttpLastBodyLimitRejectTimeText();
	sPathTime = XS_HttpLastPathLimitRejectTimeText();
	xvoTableSetInt(objValue, "http_header_limit_reject_count", sizeof("http_header_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpHeaderLimitRejectCount));
	xvoTableSetText(objValue, "http_last_header_limit_reject_time", sizeof("http_last_header_limit_reject_time") - 1, (ptr)(sHeaderTime ? sHeaderTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_header_limit_reject_age_ms", sizeof("http_last_header_limit_reject_age_ms") - 1, XS_HttpLastHeaderLimitRejectAgeMS());
	xvoTableSetInt(objValue, "http_body_limit_reject_count", sizeof("http_body_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpBodyLimitRejectCount));
	xvoTableSetText(objValue, "http_last_body_limit_reject_time", sizeof("http_last_body_limit_reject_time") - 1, (ptr)(sBodyTime ? sBodyTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_body_limit_reject_age_ms", sizeof("http_last_body_limit_reject_age_ms") - 1, XS_HttpLastBodyLimitRejectAgeMS());
	xvoTableSetInt(objValue, "http_path_limit_reject_count", sizeof("http_path_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpPathLimitRejectCount));
	xvoTableSetText(objValue, "http_last_path_limit_reject_time", sizeof("http_last_path_limit_reject_time") - 1, (ptr)(sPathTime ? sPathTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_path_limit_reject_age_ms", sizeof("http_last_path_limit_reject_age_ms") - 1, XS_HttpLastPathLimitRejectAgeMS());
	if ( sHeaderTime ) {
		xrtFree(sHeaderTime);
	}
	if ( sBodyTime ) {
		xrtFree(sBodyTime);
	}
	if ( sPathTime ) {
		xrtFree(sPathTime);
	}
}

static inline void XS_HttpAppendPolicyRejectMetrics(xvalue objValue)
{
	char* sApiDisabledTime;
	char* sMethodTime;
	char* sHostNotFoundTime;

	if ( objValue == NULL ) {
		return;
	}

	sApiDisabledTime = XS_HttpLastApiDisabledRejectTimeText();
	sMethodTime = XS_HttpLastMethodRejectTimeText();
	sHostNotFoundTime = XS_HttpLastHostNotFoundRejectTimeText();
	xvoTableSetInt(objValue, "http_api_disabled_reject_count", sizeof("http_api_disabled_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpApiDisabledRejectCount));
	xvoTableSetText(objValue, "http_last_api_disabled_reject_time", sizeof("http_last_api_disabled_reject_time") - 1, (ptr)(sApiDisabledTime ? sApiDisabledTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_api_disabled_reject_age_ms", sizeof("http_last_api_disabled_reject_age_ms") - 1, XS_HttpLastApiDisabledRejectAgeMS());
	xvoTableSetInt(objValue, "http_method_reject_count", sizeof("http_method_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpMethodRejectCount));
	xvoTableSetText(objValue, "http_last_method_reject_time", sizeof("http_last_method_reject_time") - 1, (ptr)(sMethodTime ? sMethodTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_method_reject_age_ms", sizeof("http_last_method_reject_age_ms") - 1, XS_HttpLastMethodRejectAgeMS());
	xvoTableSetInt(objValue, "http_host_not_found_reject_count", sizeof("http_host_not_found_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpHostNotFoundRejectCount));
	xvoTableSetText(objValue, "http_last_host_not_found_reject_time", sizeof("http_last_host_not_found_reject_time") - 1, (ptr)(sHostNotFoundTime ? sHostNotFoundTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_host_not_found_reject_age_ms", sizeof("http_last_host_not_found_reject_age_ms") - 1, XS_HttpLastHostNotFoundRejectAgeMS());
	if ( sApiDisabledTime ) {
		xrtFree(sApiDisabledTime);
	}
	if ( sMethodTime ) {
		xrtFree(sMethodTime);
	}
	if ( sHostNotFoundTime ) {
		xrtFree(sHostNotFoundTime);
	}
}

static inline void XS_HttpAppendManageRejectMetrics(xvalue objValue)
{
	char* sReloadBusyTime;
	char* sReloadFailedTime;
	char* sCheckConfigFailedTime;

	if ( objValue == NULL ) {
		return;
	}

	sReloadBusyTime = XS_HttpLastReloadBusyRejectTimeText();
	sReloadFailedTime = XS_HttpLastReloadFailedRejectTimeText();
	sCheckConfigFailedTime = XS_HttpLastCheckConfigFailedRejectTimeText();
	xvoTableSetInt(objValue, "http_reload_busy_reject_count", sizeof("http_reload_busy_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpReloadBusyRejectCount));
	xvoTableSetText(objValue, "http_last_reload_busy_reject_time", sizeof("http_last_reload_busy_reject_time") - 1, (ptr)(sReloadBusyTime ? sReloadBusyTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_reload_busy_reject_age_ms", sizeof("http_last_reload_busy_reject_age_ms") - 1, XS_HttpLastReloadBusyRejectAgeMS());
	xvoTableSetInt(objValue, "http_reload_failed_reject_count", sizeof("http_reload_failed_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpReloadFailedRejectCount));
	xvoTableSetText(objValue, "http_last_reload_failed_reject_time", sizeof("http_last_reload_failed_reject_time") - 1, (ptr)(sReloadFailedTime ? sReloadFailedTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_reload_failed_reject_age_ms", sizeof("http_last_reload_failed_reject_age_ms") - 1, XS_HttpLastReloadFailedRejectAgeMS());
	xvoTableSetInt(objValue, "http_check_config_failed_reject_count", sizeof("http_check_config_failed_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpCheckConfigFailedRejectCount));
	xvoTableSetText(objValue, "http_last_check_config_failed_reject_time", sizeof("http_last_check_config_failed_reject_time") - 1, (ptr)(sCheckConfigFailedTime ? sCheckConfigFailedTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_check_config_failed_reject_age_ms", sizeof("http_last_check_config_failed_reject_age_ms") - 1, XS_HttpLastCheckConfigFailedRejectAgeMS());
	if ( sReloadBusyTime ) {
		xrtFree(sReloadBusyTime);
	}
	if ( sReloadFailedTime ) {
		xrtFree(sReloadFailedTime);
	}
	if ( sCheckConfigFailedTime ) {
		xrtFree(sCheckConfigFailedTime);
	}
}

static inline void XS_HttpAppendBusRejectMetrics(xvalue objValue)
{
	char* sBadRequestTime;
	char* sNotFoundTime;
	char* sLimitTime;
	char* sFailedTime;

	if ( objValue == NULL ) {
		return;
	}

	sBadRequestTime = XS_HttpLastBusBadRequestRejectTimeText();
	sNotFoundTime = XS_HttpLastBusNotFoundRejectTimeText();
	sLimitTime = XS_HttpLastBusLimitRejectTimeText();
	sFailedTime = XS_HttpLastBusFailedRejectTimeText();
	xvoTableSetInt(objValue, "http_bus_bad_request_reject_count", sizeof("http_bus_bad_request_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpBusBadRequestRejectCount));
	xvoTableSetText(objValue, "http_last_bus_bad_request_reject_time", sizeof("http_last_bus_bad_request_reject_time") - 1, (ptr)(sBadRequestTime ? sBadRequestTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_bus_bad_request_reject_age_ms", sizeof("http_last_bus_bad_request_reject_age_ms") - 1, XS_HttpLastBusBadRequestRejectAgeMS());
	xvoTableSetInt(objValue, "http_bus_not_found_reject_count", sizeof("http_bus_not_found_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpBusNotFoundRejectCount));
	xvoTableSetText(objValue, "http_last_bus_not_found_reject_time", sizeof("http_last_bus_not_found_reject_time") - 1, (ptr)(sNotFoundTime ? sNotFoundTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_bus_not_found_reject_age_ms", sizeof("http_last_bus_not_found_reject_age_ms") - 1, XS_HttpLastBusNotFoundRejectAgeMS());
	xvoTableSetInt(objValue, "http_bus_limit_reject_count", sizeof("http_bus_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpBusLimitRejectCount));
	xvoTableSetText(objValue, "http_last_bus_limit_reject_time", sizeof("http_last_bus_limit_reject_time") - 1, (ptr)(sLimitTime ? sLimitTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_bus_limit_reject_age_ms", sizeof("http_last_bus_limit_reject_age_ms") - 1, XS_HttpLastBusLimitRejectAgeMS());
	xvoTableSetInt(objValue, "http_bus_failed_reject_count", sizeof("http_bus_failed_reject_count") - 1, XS_HttpMetricGet(&g_iXsHttpBusFailedRejectCount));
	xvoTableSetText(objValue, "http_last_bus_failed_reject_time", sizeof("http_last_bus_failed_reject_time") - 1, (ptr)(sFailedTime ? sFailedTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_bus_failed_reject_age_ms", sizeof("http_last_bus_failed_reject_age_ms") - 1, XS_HttpLastBusFailedRejectAgeMS());
	if ( sBadRequestTime ) {
		xrtFree(sBadRequestTime);
	}
	if ( sNotFoundTime ) {
		xrtFree(sNotFoundTime);
	}
	if ( sLimitTime ) {
		xrtFree(sLimitTime);
	}
	if ( sFailedTime ) {
		xrtFree(sFailedTime);
	}
}

static inline void XS_ConfigCheckStatusSnapshot(XS_CheckConfigStatusSnapshot* pStatus)
{
	XS_GetCheckConfigStatusSnapshot(pStatus);
}

static inline char* XS_CheckConfigTimeTextByStatus(const XS_CheckConfigStatusSnapshot* pStatus)
{
	if ( pStatus == NULL || pStatus->LastTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(pStatus->LastTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_CheckConfigAgeMSByStatus(const XS_CheckConfigStatusSnapshot* pStatus)
{
	if ( pStatus == NULL || pStatus->LastTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - pStatus->LastTime) * 1000;
}

static inline void XS_HttpAppendCheckConfigSnapshotByStatus(xvalue objValue, const XS_CheckConfigStatusSnapshot* pStatus)
{
	if ( objValue == NULL || pStatus == NULL ) {
		return;
	}

	xvoTableSetBool(objValue, "check_has_result", sizeof("check_has_result") - 1, pStatus->HasResult);
	xvoTableSetBool(objValue, "check_last_result", sizeof("check_last_result") - 1, pStatus->LastResult);
	xvoTableSetText(objValue, "check_last_file", sizeof("check_last_file") - 1, (ptr)pStatus->sLastFile, 0, FALSE);
	xvoTableSetText(objValue, "check_last_base", sizeof("check_last_base") - 1, (ptr)pStatus->sLastBase, 0, FALSE);
	xvoTableSetInt(objValue, "check_last_server_count", sizeof("check_last_server_count") - 1, pStatus->iLastServerCount);
	xvoTableSetText(objValue, "check_last_message", sizeof("check_last_message") - 1, (ptr)pStatus->sLastMessage, 0, FALSE);
}

static inline void XS_HttpAppendCheckConfigSnapshot(xvalue objValue)
{
	XS_CheckConfigStatusSnapshot tStatus;

	XS_ConfigCheckStatusSnapshot(&tStatus);
	XS_HttpAppendCheckConfigSnapshotByStatus(objValue, &tStatus);
}

static inline void XS_HttpAppendStopCleanupMetrics(xvalue objValue)
{
	char* sLastTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_HttpLastStopCleanupTimeText();
	xvoTableSetInt(objValue, "http_stop_cleanup_count", sizeof("http_stop_cleanup_count") - 1, XS_HttpMetricGet(&g_iXsHttpStopCleanupCount));
	xvoTableSetInt(objValue, "http_last_stop_cleanup_closed", sizeof("http_last_stop_cleanup_closed") - 1, XS_HttpMetricGet(&g_iXsHttpLastStopCleanupClosed));
	xvoTableSetInt(objValue, "http_last_stop_cleanup_remain", sizeof("http_last_stop_cleanup_remain") - 1, XS_HttpMetricGet(&g_iXsHttpLastStopCleanupRemain));
	xvoTableSetText(objValue, "http_last_stop_cleanup_time", sizeof("http_last_stop_cleanup_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "http_last_stop_cleanup_age_ms", sizeof("http_last_stop_cleanup_age_ms") - 1, XS_HttpLastStopCleanupAgeMS());
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
}

static inline void XS_WsAppendRejectMetrics(xvalue objValue)
{
	char* sLastTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_WsLastRejectTimeText();
	xvoTableSetInt(objValue, "ws_reject_count", sizeof("ws_reject_count") - 1, XS_HttpMetricGet(&g_iXsWsRejectCount));
	xvoTableSetText(objValue, "ws_last_reject_reason", sizeof("ws_last_reject_reason") - 1, (ptr)g_sXsWsLastRejectReason, 0, FALSE);
	xvoTableSetText(objValue, "ws_last_reject_time", sizeof("ws_last_reject_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "ws_last_reject_age_ms", sizeof("ws_last_reject_age_ms") - 1, XS_WsLastRejectAgeMS());
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
}

static inline void XS_WsAppendStopCleanupMetrics(xvalue objValue)
{
	char* sLastTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_WsLastStopCleanupTimeText();
	xvoTableSetInt(objValue, "ws_stop_cleanup_count", sizeof("ws_stop_cleanup_count") - 1, XS_HttpMetricGet(&g_iXsWsStopCleanupCount));
	xvoTableSetInt(objValue, "ws_last_stop_cleanup_closed", sizeof("ws_last_stop_cleanup_closed") - 1, XS_HttpMetricGet(&g_iXsWsLastStopCleanupClosed));
	xvoTableSetInt(objValue, "ws_last_stop_cleanup_remain", sizeof("ws_last_stop_cleanup_remain") - 1, XS_HttpMetricGet(&g_iXsWsLastStopCleanupRemain));
	xvoTableSetText(objValue, "ws_last_stop_cleanup_time", sizeof("ws_last_stop_cleanup_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "ws_last_stop_cleanup_age_ms", sizeof("ws_last_stop_cleanup_age_ms") - 1, XS_WsLastStopCleanupAgeMS());
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
}

static inline void XS_WsAppendMessageLimitMetrics(xvalue objValue)
{
	char* sLastTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_WsLastMessageLimitCloseTimeText();
	xvoTableSetInt(objValue, "ws_message_limit_close_count", sizeof("ws_message_limit_close_count") - 1, XS_HttpMetricGet(&g_iXsWsMessageLimitCloseCount));
	xvoTableSetText(objValue, "ws_last_message_limit_close_time", sizeof("ws_last_message_limit_close_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "ws_last_message_limit_close_age_ms", sizeof("ws_last_message_limit_close_age_ms") - 1, XS_WsLastMessageLimitCloseAgeMS());
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
}

static inline void XS_XtpAppendRejectMetrics(xvalue objValue)
{
	char* sLastTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_XtpLastRejectTimeText();
	xvoTableSetInt(objValue, "xtp_reject_count", sizeof("xtp_reject_count") - 1, XS_HttpMetricGet(&g_iXsXtpRejectCount));
	xvoTableSetText(objValue, "xtp_last_reject_reason", sizeof("xtp_last_reject_reason") - 1, (ptr)g_sXsXtpLastRejectReason, 0, FALSE);
	xvoTableSetText(objValue, "xtp_last_reject_time", sizeof("xtp_last_reject_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "xtp_last_reject_age_ms", sizeof("xtp_last_reject_age_ms") - 1, XS_XtpLastRejectAgeMS());
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
}

static inline void XS_XtpAppendRecvLimitMetrics(xvalue objValue)
{
	char* sLastTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_XtpLastRecvLimitCloseTimeText();
	xvoTableSetInt(objValue, "xtp_recv_limit_close_count", sizeof("xtp_recv_limit_close_count") - 1, XS_HttpMetricGet(&g_iXsXtpRecvLimitCloseCount));
	xvoTableSetText(objValue, "xtp_last_recv_limit_close_time", sizeof("xtp_last_recv_limit_close_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "xtp_last_recv_limit_close_age_ms", sizeof("xtp_last_recv_limit_close_age_ms") - 1, XS_XtpLastRecvLimitCloseAgeMS());
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
}

static inline void XS_XtpAppendStopCleanupMetrics(xvalue objValue)
{
	char* sLastTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_XtpLastStopCleanupTimeText();
	xvoTableSetInt(objValue, "xtp_stop_cleanup_count", sizeof("xtp_stop_cleanup_count") - 1, XS_HttpMetricGet(&g_iXsXtpStopCleanupCount));
	xvoTableSetInt(objValue, "xtp_last_stop_cleanup_closed", sizeof("xtp_last_stop_cleanup_closed") - 1, XS_HttpMetricGet(&g_iXsXtpLastStopCleanupClosed));
	xvoTableSetInt(objValue, "xtp_last_stop_cleanup_remain", sizeof("xtp_last_stop_cleanup_remain") - 1, XS_HttpMetricGet(&g_iXsXtpLastStopCleanupRemain));
	xvoTableSetText(objValue, "xtp_last_stop_cleanup_time", sizeof("xtp_last_stop_cleanup_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "xtp_last_stop_cleanup_age_ms", sizeof("xtp_last_stop_cleanup_age_ms") - 1, XS_XtpLastStopCleanupAgeMS());
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
}

static inline void XS_CustomAppendRecvLimitMetrics(xvalue objValue)
{
	char* sLastTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_CustomLastRecvLimitCloseTimeText();
	xvoTableSetInt(objValue, "custom_recv_limit_close_count", sizeof("custom_recv_limit_close_count") - 1, XS_HttpMetricGet(&g_iXsCustomRecvLimitCloseCount));
	xvoTableSetText(objValue, "custom_last_recv_limit_close_time", sizeof("custom_last_recv_limit_close_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "custom_last_recv_limit_close_age_ms", sizeof("custom_last_recv_limit_close_age_ms") - 1, XS_CustomLastRecvLimitCloseAgeMS());
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
}

static inline void XS_CustomAppendRejectMetrics(xvalue objValue)
{
	char* sLastTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_CustomLastRejectTimeText();
	xvoTableSetInt(objValue, "custom_reject_count", sizeof("custom_reject_count") - 1, XS_HttpMetricGet(&g_iXsCustomRejectCount));
	xvoTableSetText(objValue, "custom_last_reject_reason", sizeof("custom_last_reject_reason") - 1, (ptr)g_sXsCustomLastRejectReason, 0, FALSE);
	xvoTableSetText(objValue, "custom_last_reject_time", sizeof("custom_last_reject_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "custom_last_reject_age_ms", sizeof("custom_last_reject_age_ms") - 1, XS_CustomLastRejectAgeMS());
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
}

static inline void XS_CustomAppendStopCleanupMetrics(xvalue objValue)
{
	char* sLastTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_CustomLastStopCleanupTimeText();
	xvoTableSetInt(objValue, "custom_stop_cleanup_count", sizeof("custom_stop_cleanup_count") - 1, XS_HttpMetricGet(&g_iXsCustomStopCleanupCount));
	xvoTableSetInt(objValue, "custom_last_stop_cleanup_closed", sizeof("custom_last_stop_cleanup_closed") - 1, XS_HttpMetricGet(&g_iXsCustomLastStopCleanupClosed));
	xvoTableSetInt(objValue, "custom_last_stop_cleanup_remain", sizeof("custom_last_stop_cleanup_remain") - 1, XS_HttpMetricGet(&g_iXsCustomLastStopCleanupRemain));
	xvoTableSetText(objValue, "custom_last_stop_cleanup_time", sizeof("custom_last_stop_cleanup_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "custom_last_stop_cleanup_age_ms", sizeof("custom_last_stop_cleanup_age_ms") - 1, XS_CustomLastStopCleanupAgeMS());
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
}

static inline void XS_CheckConfigRecordResultEx(bool bSuccess, const char* sFilePath, const char* sBaseDir, int64 iServerCount, const char* sMessage)
{
	XS_LockCheckConfigState();
	XS_HttpMetricAdd(&g_iXsCheckConfigTotalCount, 1);
	if ( bSuccess ) {
		XS_HttpMetricAdd(&g_iXsCheckConfigSuccessCount, 1);
	} else {
		XS_HttpMetricAdd(&g_iXsCheckConfigFailureCount, 1);
	}
	g_tXsCheckConfigLastTime = xrtNow();
	g_bXsCheckConfigHasResult = TRUE;
	g_bXsCheckConfigLastResult = bSuccess;
	g_iXsCheckConfigLastServerCount = iServerCount;
	XS_HttpMetricSetText(g_sXsCheckConfigLastFile, sizeof(g_sXsCheckConfigLastFile), sFilePath ? sFilePath : "");
	XS_HttpMetricSetText(g_sXsCheckConfigLastBase, sizeof(g_sXsCheckConfigLastBase), sBaseDir ? sBaseDir : "");
	XS_HttpMetricSetText(g_sXsCheckConfigLastMessage, sizeof(g_sXsCheckConfigLastMessage), sMessage ? sMessage : "");
	XS_UnlockCheckConfigState();
}

static inline void XS_CheckConfigRecordResult(bool bSuccess)
{
	XS_CheckConfigRecordResultEx(bSuccess, g_sXsConfigFile, NULL, 0, bSuccess ? "config check passed" : "config check failed");
}

static inline char* XS_CheckConfigLastTimeText(void)
{
	XS_CheckConfigStatusSnapshot tStatus;

	XS_ConfigCheckStatusSnapshot(&tStatus);
	return XS_CheckConfigTimeTextByStatus(&tStatus);
}

static inline int64 XS_CheckConfigLastAgeMS(void)
{
	XS_CheckConfigStatusSnapshot tStatus;

	XS_ConfigCheckStatusSnapshot(&tStatus);
	return XS_CheckConfigAgeMSByStatus(&tStatus);
}

static inline void XS_CheckConfigClearStats(void)
{
	int64 iValue;

	XS_LockCheckConfigState();
	iValue = XS_HttpMetricGet(&g_iXsCheckConfigTotalCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCheckConfigTotalCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsCheckConfigSuccessCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCheckConfigSuccessCount, -iValue);
	}

	iValue = XS_HttpMetricGet(&g_iXsCheckConfigFailureCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsCheckConfigFailureCount, -iValue);
	}

	g_tXsCheckConfigLastTime = 0;
	g_iXsCheckConfigLastServerCount = 0;
	g_bXsCheckConfigHasResult = FALSE;
	g_bXsCheckConfigLastResult = FALSE;
	g_sXsCheckConfigLastFile[0] = '\0';
	g_sXsCheckConfigLastBase[0] = '\0';
	g_sXsCheckConfigLastMessage[0] = '\0';
	XS_UnlockCheckConfigState();
}

static inline void XS_ConfigReloadStatusSnapshot(XS_ReloadStatusSnapshot* pStatus)
{
	XS_GetReloadStatusSnapshot(pStatus);
}

static inline char* XS_ReloadTimeTextByStatus(const XS_ReloadStatusSnapshot* pStatus)
{
	if ( pStatus == NULL || pStatus->LastTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(pStatus->LastTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_ReloadAgeMSByStatus(const XS_ReloadStatusSnapshot* pStatus)
{
	double fNowTick;
	double fReloadTick;
	double fSpan;

	if ( pStatus == NULL || pStatus->LastTime <= 0 ) {
		return -1;
	}

	fNowTick = xrtTimer();
	fReloadTick = g_fXsStartTick + ((double)(pStatus->LastTime - g_tXsStartTime));
	fSpan = fNowTick - fReloadTick;
	if ( fSpan < 0.0 ) {
		fSpan = 0.0;
	}

	return (int64)(fSpan * 1000.0);
}

static inline const char* XS_PlatformName(void)
{
#if defined(_WIN32) || defined(_WIN64)
	return "windows";
#elif defined(__linux__)
	return "linux";
#elif defined(__APPLE__)
	return "macos";
#else
	return "unknown";
#endif
}

static inline const char* XS_ArchName(void)
{
#if defined(_M_X64) || defined(__x86_64__)
	return "x64";
#elif defined(_M_IX86) || defined(__i386__)
	return "x86";
#elif defined(_M_ARM64) || defined(__aarch64__)
	return "arm64";
#elif defined(_M_ARM) || defined(__arm__)
	return "arm";
#else
	return "unknown";
#endif
}

static inline bool XS_ManageAPIEnabled(const XS_ServerConfig* objServer, const XS_HostConfig* objHost)
{
	if ( objServer == NULL || objHost == NULL ) {
		return FALSE;
	}

	return objServer->Debug || objHost->Debug;
}

static inline bool XS_HttpPathExpectsJSONError(const char* sPath)
{
	size_t iLen;

	if ( sPath == NULL || sPath[0] == '\0' ) {
		return FALSE;
	}
	if ( strcmp(sPath, "/__xs/bus") == 0 || strncmp(sPath, "/__xs/bus/", 10) == 0 ) {
		return TRUE;
	}
	if ( strncmp(sPath, "/__xs/", 6) == 0 ) {
		iLen = strlen(sPath);
		if ( iLen >= 5u && strcmp(sPath + iLen - 5u, "_json") == 0 ) {
			return TRUE;
		}
	}
	return
		strcmp(sPath, "/__xs/http_metrics_json") == 0 ||
		strcmp(sPath, "/__xs/ws_metrics_json") == 0 ||
		strcmp(sPath, "/__xs/xtp_metrics_json") == 0 ||
		strcmp(sPath, "/__xs/udp_metrics_json") == 0 ||
		strcmp(sPath, "/__xs/custom_metrics_json") == 0 ||
		strcmp(sPath, "/__xs/check_config_json") == 0 ||
		strcmp(sPath, "/__xs/status_json") == 0 ||
		strcmp(sPath, "/__xs/reload_json") == 0 ||
		strcmp(sPath, "/__xs/reload_status_json") == 0 ||
		strcmp(sPath, "/__xs/reload_config_json") == 0 ||
		strcmp(sPath, "/__xs/health_json") == 0 ||
		strcmp(sPath, "/__xs/dashboard_json") == 0;
}

static inline bool XS_HttpPathReservedManage(const char* sPath)
{
	return sPath && (strcmp(sPath, "/__xs") == 0 || strncmp(sPath, "/__xs/", 6) == 0);
}

static inline const char* XS_HttpReadOnlyAPIAllowHeader(const char* sPath)
{
	(void)sPath;
	return "GET, HEAD";
}

static inline const char* XS_HttpGetOnlyAPIMethodMessage(const char* sPath)
{
	if ( sPath == NULL || sPath[0] == '\0' ) {
		return "manage api only supports GET";
	}
	if (
		strcmp(sPath, "/__xs/bus/retain") == 0 ||
		strcmp(sPath, "/__xs/bus/release") == 0 ||
		strcmp(sPath, "/__xs/bus/touch") == 0 ||
		strcmp(sPath, "/__xs/bus/set") == 0 ||
		strcmp(sPath, "/__xs/bus/remove") == 0 ||
		strcmp(sPath, "/__xs/bus/reset") == 0 ||
		strcmp(sPath, "/__xs/bus/sweep") == 0 ||
		strcmp(sPath, "/__xs/bus/limits") == 0 ||
		strcmp(sPath, "/__xs/bus/register") == 0 ||
		strcmp(sPath, "/__xs/bus/send") == 0
	) {
		return "bus management api only supports GET";
	}
	if ( strcmp(sPath, "/__xs/check_config") == 0 || strcmp(sPath, "/__xs/check_config_json") == 0 ) {
		return (strcmp(sPath, "/__xs/check_config_json") == 0) ? "check config json api only supports GET" : "check config api only supports GET";
	}
	if ( strcmp(sPath, "/__xs/check_config_clear") == 0 ) {
		return "check config clear api only supports GET";
	}
	if ( strcmp(sPath, "/__xs/reload_clear") == 0 ) {
		return "config reload clear api only supports GET";
	}
	if ( strcmp(sPath, "/__xs/reload_reset") == 0 ) {
		return "config reload reset api only supports GET";
	}
	if ( strcmp(sPath, "/__xs/http_metrics_clear") == 0 ) {
		return "http metrics clear api only supports GET";
	}
	if ( strcmp(sPath, "/__xs/ws_metrics_clear") == 0 ) {
		return "ws metrics clear api only supports GET";
	}
	if ( strcmp(sPath, "/__xs/xtp_metrics_clear") == 0 ) {
		return "xtp metrics clear api only supports GET";
	}
	if ( strcmp(sPath, "/__xs/udp_metrics_clear") == 0 ) {
		return "udp metrics clear api only supports GET";
	}
	if ( strcmp(sPath, "/__xs/custom_metrics_clear") == 0 ) {
		return "custom metrics clear api only supports GET";
	}
	return "manage api only supports GET";
}

static inline bool XS_HttpBusReadOnlyPath(const char* sPath)
{
	if ( sPath == NULL || sPath[0] == '\0' ) {
		return FALSE;
	}
	return
		strcmp(sPath, "/__xs/bus/status") == 0 ||
		strcmp(sPath, "/__xs/bus/registry") == 0 ||
		strcmp(sPath, "/__xs/bus/namespaces") == 0 ||
		strcmp(sPath, "/__xs/bus/find") == 0 ||
		strcmp(sPath, "/__xs/bus/exists") == 0 ||
		strcmp(sPath, "/__xs/bus/get") == 0 ||
		strcmp(sPath, "/__xs/bus/values") == 0;
}

static inline bool XS_HttpBusMutatingPath(const char* sPath)
{
	if ( sPath == NULL || sPath[0] == '\0' ) {
		return FALSE;
	}
	return
		strcmp(sPath, "/__xs/bus/retain") == 0 ||
		strcmp(sPath, "/__xs/bus/release") == 0 ||
		strcmp(sPath, "/__xs/bus/touch") == 0 ||
		strcmp(sPath, "/__xs/bus/set") == 0 ||
		strcmp(sPath, "/__xs/bus/remove") == 0 ||
		strcmp(sPath, "/__xs/bus/reset") == 0 ||
		strcmp(sPath, "/__xs/bus/sweep") == 0 ||
		strcmp(sPath, "/__xs/bus/limits") == 0 ||
		strcmp(sPath, "/__xs/bus/register") == 0 ||
		strcmp(sPath, "/__xs/bus/send") == 0;
}

static inline const char* XS_HttpReadOnlyAPIMethodMessage(const char* sPath)
{
	if ( sPath == NULL || sPath[0] == '\0' ) {
		return "manage api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/health") == 0 || strcmp(sPath, "/__xs/health_json") == 0 ) {
		return (strcmp(sPath, "/__xs/health_json") == 0) ? "health json api only supports GET or HEAD" : "health api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/status") == 0 || strcmp(sPath, "/__xs/status_json") == 0 ) {
		return (strcmp(sPath, "/__xs/status_json") == 0) ? "status json api only supports GET or HEAD" : "status api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/dashboard") == 0 || strcmp(sPath, "/__xs/dashboard_json") == 0 ) {
		return (strcmp(sPath, "/__xs/dashboard_json") == 0) ? "dashboard json api only supports GET or HEAD" : "dashboard api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/reload_status") == 0 || strcmp(sPath, "/__xs/reload_status_json") == 0 ) {
		return (strcmp(sPath, "/__xs/reload_status_json") == 0) ? "config reload status json api only supports GET or HEAD" : "config reload status api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/http_metrics") == 0 || strcmp(sPath, "/__xs/http_metrics_json") == 0 ) {
		return (strcmp(sPath, "/__xs/http_metrics_json") == 0) ? "http metrics json api only supports GET or HEAD" : "http metrics api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/http_metrics_clear") == 0 ) {
		return "http metrics clear api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/ws_metrics") == 0 || strcmp(sPath, "/__xs/ws_metrics_json") == 0 ) {
		return (strcmp(sPath, "/__xs/ws_metrics_json") == 0) ? "ws metrics json api only supports GET or HEAD" : "ws metrics api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/ws_metrics_clear") == 0 ) {
		return "ws metrics clear api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/xtp_metrics") == 0 || strcmp(sPath, "/__xs/xtp_metrics_json") == 0 ) {
		return (strcmp(sPath, "/__xs/xtp_metrics_json") == 0) ? "xtp metrics json api only supports GET or HEAD" : "xtp metrics api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/xtp_metrics_clear") == 0 ) {
		return "xtp metrics clear api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/udp_metrics") == 0 || strcmp(sPath, "/__xs/udp_metrics_json") == 0 ) {
		return (strcmp(sPath, "/__xs/udp_metrics_json") == 0) ? "udp metrics json api only supports GET or HEAD" : "udp metrics api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/udp_metrics_clear") == 0 ) {
		return "udp metrics clear api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/custom_metrics") == 0 || strcmp(sPath, "/__xs/custom_metrics_json") == 0 ) {
		return (strcmp(sPath, "/__xs/custom_metrics_json") == 0) ? "custom metrics json api only supports GET or HEAD" : "custom metrics api only supports GET or HEAD";
	}
	if ( strcmp(sPath, "/__xs/custom_metrics_clear") == 0 ) {
		return "custom metrics clear api only supports GET or HEAD";
	}
	return "manage api only supports GET or HEAD";
}

static inline const char* XS_TlsConfigFileText(const char* sText)
{
	return (sText && sText[0] != '\0') ? sText : "";
}

static inline void XS_HttpApplyDefaultHeaders(const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	if ( pResp == NULL ) {
		return;
	}

	(void)xrtHttpdResponseSetHeader(pResp, "X-Content-Type-Options", "nosniff");
	if ( pReq && XS_HttpPathReservedManage(pReq->sPath) ) {
		(void)xrtHttpdResponseSetHeader(pResp, "Cache-Control", "no-store");
		(void)xrtHttpdResponseSetHeader(pResp, "X-Frame-Options", "DENY");
		(void)xrtHttpdResponseSetHeader(pResp, "Referrer-Policy", "no-referrer");
	}
}

static inline bool XS_HttpValidateRequest(XS_ServerConfig* objServer, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	size_t iPathLen;
	size_t iQueryLen;
	bool bJsonError;

	if ( objServer == NULL || pReq == NULL || pResp == NULL ) {
		return FALSE;
	}
	bJsonError = XS_HttpPathExpectsJSONError(pReq->sPath);

	if ( objServer->HeaderLimit > 0u && pReq->iHeaderCount > objServer->HeaderLimit ) {
		XS_HttpRecordHeaderLimitReject();
		if ( bJsonError ) {
			return XS_HttpRespondJsonResult(pResp, 431, "Request Header Fields Too Large", FALSE, "header count limit exceeded");
		}
		return XS_HttpRespondText(pResp, 431, "Request Header Fields Too Large", "header count limit exceeded");
	}

	if ( objServer->BodyLimit > 0u && pReq->iBodyLen > (size_t)objServer->BodyLimit ) {
		XS_HttpRecordBodyLimitReject();
		if ( bJsonError ) {
			return XS_HttpRespondJsonResult(pResp, 413, "Payload Too Large", FALSE, "request body limit exceeded");
		}
		return XS_HttpRespondText(pResp, 413, "Payload Too Large", "request body limit exceeded");
	}

	if ( XS_HttpBusReadOnlyPath(pReq->sPath) ) {
		if ( _stricmp(pReq->sMethod, "GET") != 0 ) {
			xrtHttpdResponseSetHeader(pResp, "Allow", "GET");
			if ( bJsonError ) {
				return XS_HttpRespondJsonResult(pResp, 405, "Method Not Allowed", FALSE, "bus management api only supports GET");
			}
			return XS_HttpRespondText(pResp, 405, "Method Not Allowed", "bus management api only supports GET");
		}
	}

	if ( XS_HttpBusMutatingPath(pReq->sPath) ) {
		if ( _stricmp(pReq->sMethod, "GET") != 0 ) {
			xrtHttpdResponseSetHeader(pResp, "Allow", "GET");
			if ( bJsonError ) {
				return XS_HttpRespondJsonResult(pResp, 405, "Method Not Allowed", FALSE, "bus management api only supports GET");
			}
			return XS_HttpRespondText(pResp, 405, "Method Not Allowed", "bus management api only supports GET");
		}
	}

	if ( strcmp(pReq->sPath, "/__xs/reload_json") == 0 || strcmp(pReq->sPath, "/__xs/reload_config_json") == 0 ) {
		if ( _stricmp(pReq->sMethod, "GET") != 0 && _stricmp(pReq->sMethod, "POST") != 0 ) {
			xrtHttpdResponseSetHeader(pResp, "Allow", "GET, POST");
			if ( strcmp(pReq->sPath, "/__xs/reload_json") == 0 ) {
				return XS_HttpRespondJsonResult(pResp, 405, "Method Not Allowed", FALSE, "reload json api only supports GET or POST");
			}
			return XS_HttpRespondJsonResult(pResp, 405, "Method Not Allowed", FALSE, "config reload json api only supports GET or POST");
		}
	}

	if (
		strcmp(pReq->sPath, "/__xs/check_config") == 0 ||
		strcmp(pReq->sPath, "/__xs/check_config_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/check_config_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/reload_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/reload_reset") == 0 ||
		strcmp(pReq->sPath, "/__xs/http_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/ws_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/xtp_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/udp_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/custom_metrics_clear") == 0
	) {
		if ( _stricmp(pReq->sMethod, "GET") != 0 ) {
			const char* sMethodMessage = XS_HttpGetOnlyAPIMethodMessage(pReq->sPath);
			xrtHttpdResponseSetHeader(pResp, "Allow", "GET");
			if ( bJsonError ) {
				return XS_HttpRespondJsonResult(pResp, 405, "Method Not Allowed", FALSE, sMethodMessage);
			}
			return XS_HttpRespondText(pResp, 405, "Method Not Allowed", sMethodMessage);
		}
	}

	if (
		strcmp(pReq->sPath, "/__xs/health") == 0 ||
		strcmp(pReq->sPath, "/__xs/http_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/http_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/ws_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/ws_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/xtp_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/xtp_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/udp_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/udp_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/custom_metrics") == 0 ||
		strcmp(pReq->sPath, "/__xs/custom_metrics_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/dashboard") == 0 ||
		strcmp(pReq->sPath, "/__xs/status") == 0 ||
		strcmp(pReq->sPath, "/__xs/status_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/reload_status") == 0 ||
		strcmp(pReq->sPath, "/__xs/reload_status_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/health_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/dashboard_json") == 0
	) {
		if ( _stricmp(pReq->sMethod, "GET") != 0 && _stricmp(pReq->sMethod, "HEAD") != 0 ) {
			const char* sMethodMessage = XS_HttpReadOnlyAPIMethodMessage(pReq->sPath);
			xrtHttpdResponseSetHeader(pResp, "Allow", XS_HttpReadOnlyAPIAllowHeader(pReq->sPath));
			if ( bJsonError ) {
				return XS_HttpRespondJsonResult(pResp, 405, "Method Not Allowed", FALSE, sMethodMessage);
			}
			return XS_HttpRespondText(pResp, 405, "Method Not Allowed", sMethodMessage);
		}
	}

	iPathLen = strlen(pReq->sPath);
	iQueryLen = strlen(pReq->sQuery);
	if ( objServer->PathLimit > 0u && (iPathLen + iQueryLen) > (size_t)objServer->PathLimit ) {
		XS_HttpRecordPathLimitReject();
		if ( bJsonError ) {
			return XS_HttpRespondJsonResult(pResp, 414, "URI Too Long", FALSE, "request path limit exceeded");
		}
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

static inline bool XS_HttpQueryBoolEx(const char* sQuery, const char* sName, bool bDefault, bool* pbValue, bool* pbPresent)
{
	const char* sFind;
	size_t iNameLen;
	bool bValue = bDefault;
	
	if ( pbValue ) {
		*pbValue = bDefault;
	}
	if ( pbPresent ) {
		*pbPresent = FALSE;
	}
	if ( sQuery == NULL || sName == NULL || sName[0] == '\0' ) {
		return TRUE;
	}
	
	iNameLen = strlen(sName);
	sFind = sQuery;
	while ( sFind && sFind[0] ) {
		const char* pNext = strchr(sFind, '&');
		size_t iPairLen = pNext ? (size_t)(pNext - sFind) : strlen(sFind);
		if ( iPairLen > iNameLen + 1 && strncmp(sFind, sName, iNameLen) == 0 && sFind[iNameLen] == '=' ) {
			const char* sVal = sFind + iNameLen + 1;
			size_t iValLen = iPairLen - iNameLen - 1;
			if ( pbPresent ) {
				*pbPresent = TRUE;
			}
			if ( !XS_HttpParseBoolText(sVal, iValLen, &bValue) ) {
				return FALSE;
			}
			if ( pbValue ) {
				*pbValue = bValue;
			}
			return TRUE;
		}
		sFind = pNext ? (pNext + 1) : NULL;
	}
	
	return TRUE;
}

static inline bool XS_HttpQueryBool(const char* sQuery, const char* sName, bool bDefault)
{
	bool bValue = bDefault;

	(void)XS_HttpQueryBoolEx(sQuery, sName, bDefault, &bValue, NULL);
	return bValue;
}

static inline bool XS_HttpQueryHas(const char* sQuery, const char* sName)
{
	const char* sFind;
	size_t iNameLen;

	if ( sQuery == NULL || sName == NULL || sName[0] == '\0' ) {
		return FALSE;
	}

	iNameLen = strlen(sName);
	sFind = sQuery;
	while ( sFind && sFind[0] ) {
		const char* pNext = strchr(sFind, '&');
		size_t iPairLen = pNext ? (size_t)(pNext - sFind) : strlen(sFind);
		if ( iPairLen >= iNameLen + 1 && strncmp(sFind, sName, iNameLen) == 0 && sFind[iNameLen] == '=' ) {
			return TRUE;
		}
		sFind = pNext ? (pNext + 1) : NULL;
	}

	return FALSE;
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

static inline void XS_HttpRecordReloadStatus(XS_ServerConfig* objServer, const XS_HostConfig* objHost, bool bForce, int iCode)
{
	XS_RecordReloadStatusLite(
		(objServer && objServer->Name) ? objServer->Name : NULL,
		(objHost && objHost->Name) ? objHost->Name : NULL,
		bForce,
		iCode
	);
}

static inline int32 XS_HttpReloadStatusCode(int iCode)
{
	if ( iCode == 0 ) {
		return 200;
	}
	if ( iCode == -4 ) {
		return 409;
	}

	return 500;
}

static inline const char* XS_HttpReloadStatusText(int iCode)
{
	if ( iCode == 0 ) {
		return "OK";
	}
	if ( iCode == -4 ) {
		return "Conflict";
	}

	return "Internal Server Error";
}

static inline bool XS_HttpHandleReload(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ReloadStatusSnapshot tReloadStatus;
	char* sReloadHostName;
	char* sReloadTime;
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
	if ( !XS_HttpQueryBoolEx(pReq->sQuery, "force", FALSE, &bForce, NULL) ) {
		if ( sReloadHostName ) {
			xrtFree(sReloadHostName);
		}
		return XS_HttpRespondNamedBoolBadRequestText(pResp, "force");
	}
	
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
	XS_HttpRecordReloadStatus(objServer, objReloadHost, bForce, iRet);
	XS_ConfigReloadStatusSnapshot(&tReloadStatus);
	sReloadTime = XS_ReloadTimeTextByStatus(&tReloadStatus);
	snprintf(
		sBody,
		sizeof(sBody),
		"result=%s\nserver=%s\nhost=%s\nforce=%s\ncode=%d\nmessage=%s\nreload_has_result=%s\nreload_time=%s\nreload_age_ms=%lld\nreload_total_count=%lld\nreload_success_count=%lld\nreload_failure_count=%lld\n",
		(iRet == 0) ? "true" : "false",
		objServer->Name ? objServer->Name : "(null)",
		objReloadHost->Name ? objReloadHost->Name : "(default)",
		bForce ? "true" : "false",
		iRet,
		XS_ReloadResultText(iRet),
		tReloadStatus.HasResult ? "true" : "false",
		sReloadTime ? sReloadTime : "(none)",
		(long long)XS_ReloadAgeMSByStatus(&tReloadStatus),
		(long long)tReloadStatus.iTotalCount,
		(long long)tReloadStatus.iSuccessCount,
		(long long)tReloadStatus.iFailureCount
	);
	
	if ( sReloadHostName ) {
		xrtFree(sReloadHostName);
	}
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	
	return XS_HttpRespondText(
		pResp,
		XS_HttpReloadStatusCode(iRet),
		XS_HttpReloadStatusText(iRet),
		sBody
	);
}

static inline bool XS_HttpHandleReloadJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ReloadStatusSnapshot tReloadStatus;
	xvalue objRet;
	char* sJson;
	char* sReloadHostName;
	char* sReloadTime;
	XS_HostConfig* objReloadHost;
	bool bForce;
	int iRet;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/reload_json") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "reload json api disabled");
	}
	if ( _stricmp(pReq->sMethod, "GET") != 0 && _stricmp(pReq->sMethod, "POST") != 0 ) {
		return XS_HttpRespondJsonResult(pResp, 405, "Method Not Allowed", FALSE, "reload json api only supports GET or POST");
	}

	sReloadHostName = XS_HttpQueryText(pReq->sQuery, "host");
	if ( !XS_HttpQueryBoolEx(pReq->sQuery, "force", FALSE, &bForce, NULL) ) {
		if ( sReloadHostName ) {
			xrtFree(sReloadHostName);
		}
		return XS_HttpRespondNamedBoolBadRequestJson(pResp, "force");
	}
	if ( sReloadHostName && sReloadHostName[0] ) {
		objReloadHost = XS_FindServerHostByName(objServer, sReloadHostName);
	} else {
		objReloadHost = (XS_HostConfig*)objHost;
	}
	if ( objReloadHost == NULL ) {
		if ( sReloadHostName ) {
			xrtFree(sReloadHostName);
		}
		return XS_HttpRespondJsonResult(pResp, 404, "Not Found", FALSE, "reload host not found");
	}

	iRet = XS_ReloadServerHostScript(objServer, objReloadHost, bForce);
	XS_HttpRecordReloadStatus(objServer, objReloadHost, bForce, iRet);
	XS_ConfigReloadStatusSnapshot(&tReloadStatus);
	sReloadTime = XS_ReloadTimeTextByStatus(&tReloadStatus);
	objRet = xvoCreateTable();
	xvoTableSetBool(objRet, "result", 6, (iRet == 0));
	xvoTableSetText(objRet, "message", 7, (ptr)XS_ReloadResultText(iRet), 0, FALSE);
	xvoTableSetText(objRet, "server", 6, (ptr)(objServer->Name ? objServer->Name : "(null)"), 0, FALSE);
	xvoTableSetText(objRet, "host", 4, (ptr)(objReloadHost->Name ? objReloadHost->Name : "(default)"), 0, FALSE);
	xvoTableSetBool(objRet, "force", 5, bForce);
	xvoTableSetInt(objRet, "code", 4, iRet);
	xvoTableSetBool(objRet, "reload_has_result", 17, tReloadStatus.HasResult);
	xvoTableSetText(objRet, "reload_time", 11, (ptr)(sReloadTime ? sReloadTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "reload_age_ms", 13, XS_ReloadAgeMSByStatus(&tReloadStatus));
	xvoTableSetInt(objRet, "reload_total_count", 18, tReloadStatus.iTotalCount);
	xvoTableSetInt(objRet, "reload_success_count", 20, tReloadStatus.iSuccessCount);
	xvoTableSetInt(objRet, "reload_failure_count", 20, tReloadStatus.iFailureCount);

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sReloadHostName ) {
		xrtFree(sReloadHostName);
	}
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "reload json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, XS_HttpReloadStatusCode(iRet), XS_HttpReloadStatusText(iRet));
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpConfigReloadTargetsCurrentServer(const XS_ServerConfig* objServer, const char* sServerName)
{
	if ( objServer == NULL ) {
		return FALSE;
	}
	if ( sServerName == NULL || sServerName[0] == '\0' ) {
		return TRUE;
	}
	if ( objServer->Name == NULL || objServer->Name[0] == '\0' ) {
		return FALSE;
	}
	return strcmp(objServer->Name, sServerName) == 0;
}

static inline XS_HostConfig* XS_HttpFindConfigReloadTargetHost(XS_ServerConfig* objServer, const char* sServerName, const char* sHostName)
{
	if ( objServer == NULL || sHostName == NULL || sHostName[0] == '\0' ) {
		return NULL;
	}
	if ( !XS_HttpConfigReloadTargetsCurrentServer(objServer, sServerName) ) {
		return NULL;
	}
	return XS_FindServerHostByName(objServer, sHostName);
}

static inline bool XS_HttpHandleConfigReload(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[512];
	char* sServerName;
	char* sHostName;
	XS_HostConfig* objTargetHost;
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
	
	if ( !XS_HttpQueryBoolEx(pReq->sQuery, "force", FALSE, &bForce, NULL) ) {
		return XS_HttpRespondNamedBoolBadRequestText(pResp, "force");
	}
	sServerName = XS_HttpQueryDup(pReq->sQuery, "server");
	sHostName = XS_HttpQueryDup(pReq->sQuery, "host");
	if ( (sServerName == NULL || sServerName[0] == '\0') && sHostName && sHostName[0] != '\0' ) {
		sServerName = XS_CopyText(objServer->Name);
	}
	objTargetHost = XS_HttpFindConfigReloadTargetHost(objServer, sServerName, sHostName);
	if ( sHostName && sHostName[0] != '\0' && XS_HttpConfigReloadTargetsCurrentServer(objServer, sServerName) && objTargetHost == NULL ) {
		if ( sServerName ) {
			xrtFree(sServerName);
		}
		if ( sHostName ) {
			xrtFree(sHostName);
		}
		return XS_HttpRespondText(pResp, 404, "Not Found", "reload host not found");
	}
	bQueued = XS_RequestConfigReloadEx(sServerName, sHostName, bForce);
	snprintf(
		sBody,
		sizeof(sBody),
		"result=%s\nmessage=%s\nserver=%s\ntarget_server=%s\ntarget_host=%s\nforce=%s\n",
		bQueued ? "true" : "false",
		bQueued ? "config reload queued" : "config reload busy",
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

static inline bool XS_HttpHandleConfigReloadJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sJson;
	char* sServerName;
	char* sHostName;
	XS_HostConfig* objTargetHost;
	bool bQueued;
	bool bForce;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/reload_config_json") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "config reload json api disabled");
	}
	if ( _stricmp(pReq->sMethod, "GET") != 0 && _stricmp(pReq->sMethod, "POST") != 0 ) {
		return XS_HttpRespondJsonResult(pResp, 405, "Method Not Allowed", FALSE, "config reload json api only supports GET or POST");
	}

	if ( !XS_HttpQueryBoolEx(pReq->sQuery, "force", FALSE, &bForce, NULL) ) {
		return XS_HttpRespondNamedBoolBadRequestJson(pResp, "force");
	}
	sServerName = XS_HttpQueryDup(pReq->sQuery, "server");
	sHostName = XS_HttpQueryDup(pReq->sQuery, "host");
	if ( (sServerName == NULL || sServerName[0] == '\0') && sHostName && sHostName[0] != '\0' ) {
		sServerName = XS_CopyText(objServer->Name);
	}
	objTargetHost = XS_HttpFindConfigReloadTargetHost(objServer, sServerName, sHostName);
	if ( sHostName && sHostName[0] != '\0' && XS_HttpConfigReloadTargetsCurrentServer(objServer, sServerName) && objTargetHost == NULL ) {
		if ( sServerName ) {
			xrtFree(sServerName);
		}
		if ( sHostName ) {
			xrtFree(sHostName);
		}
		return XS_HttpRespondJsonResult(pResp, 404, "Not Found", FALSE, "reload host not found");
	}
	bQueued = XS_RequestConfigReloadEx(sServerName, sHostName, bForce);

	objRet = xvoCreateTable();
	xvoTableSetBool(objRet, "result", 6, bQueued);
	xvoTableSetText(objRet, "message", 7, bQueued ? "config reload queued" : "config reload busy", 0, FALSE);
	xvoTableSetText(objRet, "server", 6, (ptr)(objServer->Name ? objServer->Name : "(null)"), 0, FALSE);
	xvoTableSetText(objRet, "target_server", 13, (ptr)((sServerName && sServerName[0]) ? sServerName : "(all)"), 0, FALSE);
	xvoTableSetText(objRet, "target_host", 11, (ptr)((sHostName && sHostName[0]) ? sHostName : "(all)"), 0, FALSE);
	xvoTableSetBool(objRet, "force", 5, bForce);

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sServerName ) {
		xrtFree(sServerName);
	}
	if ( sHostName ) {
		xrtFree(sHostName);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "config reload json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, bQueued ? 200 : 409, bQueued ? "OK" : "Conflict");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleConfigReloadStatus(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ReloadStatusSnapshot tStatus;
	char sBody[512];
	char* sReloadTime;
	
	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/reload_status") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "config reload status api disabled");
	}

	XS_ConfigReloadStatusSnapshot(&tStatus);
	sReloadTime = XS_ReloadTimeTextByStatus(&tStatus);
	
	snprintf(
		sBody,
		sizeof(sBody),
		"busy=%s\nhas_result=%s\nsuccess=%s\nserver=%s\nhost=%s\nmessage=%s\nreload_time=%s\nreload_age_ms=%lld\nreload_total_count=%lld\nreload_success_count=%lld\nreload_failure_count=%lld\n",
		tStatus.Busy ? "true" : "false",
		tStatus.HasResult ? "true" : "false",
		tStatus.Success ? "true" : "false",
		tStatus.sServerName[0] ? tStatus.sServerName : "(all)",
		tStatus.sHostName[0] ? tStatus.sHostName : "(all)",
		tStatus.sMessage[0] ? tStatus.sMessage : "(none)",
		sReloadTime ? sReloadTime : "(none)",
		(long long)XS_ReloadAgeMSByStatus(&tStatus),
		(long long)tStatus.iTotalCount,
		(long long)tStatus.iSuccessCount,
		(long long)tStatus.iFailureCount
	);
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleConfigReloadStatusJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ReloadStatusSnapshot tStatus;
	xvalue objRet;
	char* sReloadTime;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/reload_status_json") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "config reload status json api disabled");
	}

	XS_ConfigReloadStatusSnapshot(&tStatus);
	sReloadTime = XS_ReloadTimeTextByStatus(&tStatus);
	objRet = xvoCreateTable();
	xvoTableSetBool(objRet, "busy", 4, tStatus.Busy);
	xvoTableSetBool(objRet, "has_result", 10, tStatus.HasResult);
	xvoTableSetBool(objRet, "success", 7, tStatus.Success);
	xvoTableSetText(objRet, "server", 6, (ptr)(tStatus.sServerName[0] ? tStatus.sServerName : "(all)"), 0, FALSE);
	xvoTableSetText(objRet, "host", 4, (ptr)(tStatus.sHostName[0] ? tStatus.sHostName : "(all)"), 0, FALSE);
	xvoTableSetText(objRet, "message", 7, (ptr)(tStatus.sMessage[0] ? tStatus.sMessage : "(none)"), 0, FALSE);
	xvoTableSetText(objRet, "reload_time", 11, (ptr)(sReloadTime ? sReloadTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "reload_age_ms", 13, XS_ReloadAgeMSByStatus(&tStatus));
	xvoTableSetInt(objRet, "reload_total_count", 18, tStatus.iTotalCount);
	xvoTableSetInt(objRet, "reload_success_count", 20, tStatus.iSuccessCount);
	xvoTableSetInt(objRet, "reload_failure_count", 20, tStatus.iFailureCount);

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "reload status json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleConfigReloadClear(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ReloadStatusSnapshot tStatus;
	char sBody[512];
	char* sReloadTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/reload_clear") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "config reload clear api disabled");
	}

	XS_ClearConfigReloadStatus();
	XS_ConfigReloadStatusSnapshot(&tStatus);
	sReloadTime = XS_ReloadTimeTextByStatus(&tStatus);
	snprintf(
		sBody,
		sizeof(sBody),
		"busy=%s\nhas_result=%s\nsuccess=%s\nserver=%s\nhost=%s\nmessage=%s\nreload_time=%s\nreload_age_ms=%lld\nreload_total_count=%lld\nreload_success_count=%lld\nreload_failure_count=%lld\n",
		tStatus.Busy ? "true" : "false",
		tStatus.HasResult ? "true" : "false",
		tStatus.Success ? "true" : "false",
		tStatus.sServerName[0] ? tStatus.sServerName : "(all)",
		tStatus.sHostName[0] ? tStatus.sHostName : "(all)",
		tStatus.sMessage[0] ? tStatus.sMessage : "(none)",
		sReloadTime ? sReloadTime : "(none)",
		(long long)XS_ReloadAgeMSByStatus(&tStatus),
		(long long)tStatus.iTotalCount,
		(long long)tStatus.iSuccessCount,
		(long long)tStatus.iFailureCount
	);
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleConfigReloadReset(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ReloadStatusSnapshot tStatus;
	char sBody[512];
	char* sReloadTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/reload_reset") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "config reload reset api disabled");
	}

	XS_ResetConfigReloadStats();
	XS_ConfigReloadStatusSnapshot(&tStatus);
	sReloadTime = XS_ReloadTimeTextByStatus(&tStatus);
	snprintf(
		sBody,
		sizeof(sBody),
		"busy=%s\nhas_result=%s\nsuccess=%s\nserver=%s\nhost=%s\nmessage=%s\nreload_time=%s\nreload_age_ms=%lld\nreload_total_count=%lld\nreload_success_count=%lld\nreload_failure_count=%lld\n",
		tStatus.Busy ? "true" : "false",
		tStatus.HasResult ? "true" : "false",
		tStatus.Success ? "true" : "false",
		tStatus.sServerName[0] ? tStatus.sServerName : "(all)",
		tStatus.sHostName[0] ? tStatus.sHostName : "(all)",
		tStatus.sMessage[0] ? tStatus.sMessage : "(none)",
		sReloadTime ? sReloadTime : "(none)",
		(long long)XS_ReloadAgeMSByStatus(&tStatus),
		(long long)tStatus.iTotalCount,
		(long long)tStatus.iSuccessCount,
		(long long)tStatus.iFailureCount
	);
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleBusStatus(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/status") != 0 && strcmp(pReq->sPath, "/__xs/bus/registry") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus status api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	if ( (sNamespace && sNamespace[0] != '\0') || (sTag && sTag[0] != '\0') ) {
		sJson = XS_BusBuildStatusJsonEx(
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
			(sTag && sTag[0] != '\0') ? sTag : NULL
		);
	} else {
		sJson = XS_BusBuildStatusJson();
	}

	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "bus status build failed");
	}
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleBusNamespaces(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/namespaces") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus namespace api disabled");
	}

	sJson = XS_BusBuildNamespaceStatsJson();
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "bus namespace build failed");
	}
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleBusFind(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char sBody[256];
	int64 iDataID;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/find") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus find api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	iDataID = XS_BusDataFindFirst(
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
		(sTag && sTag[0] != '\0') ? sTag : NULL
	);
	snprintf(
		sBody,
		sizeof(sBody),
		"{\"result\":true,\"data_id\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
		(long long)iDataID,
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
		(sTag && sTag[0] != '\0') ? sTag : ""
	);
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpHandleBusExists(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char* sID;
	char sBody[320];
	int64 iDataID;
	bool bExists;
	xvalue objData;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/exists") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus exists api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	if ( XS_HttpRespondBusDataIDBadRequest(pResp, sID) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		return TRUE;
	}
	iDataID = 0;
	if ( sID && sID[0] != '\0' ) {
		(void)XS_HttpParsePositiveInt64(sID, &iDataID);
	}
	objData = NULL;
	if ( iDataID > 0 ) {
		objData = XS_BusDataGet(iDataID);
		bExists = (objData != NULL);
	} else if ( ((sNamespace && sNamespace[0] != '\0')) || ((sTag && sTag[0] != '\0')) ) {
		iDataID = XS_BusDataFindFirst(
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
			(sTag && sTag[0] != '\0') ? sTag : NULL
		);
		bExists = (iDataID > 0);
	} else {
		bExists = FALSE;
	}
	snprintf(
		sBody,
		sizeof(sBody),
		"{\"result\":true,\"exists\":%s,\"data_id\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
		bExists ? "true" : "false",
		(long long)iDataID,
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
		(sTag && sTag[0] != '\0') ? sTag : ""
	);
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sID ) {
		xrtFree(sID);
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpHandleBusGet(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char* sID;
	char* sJson;
	char sBody[320];
	int64 iDataID;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/get") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus get api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	if ( XS_HttpRespondBusDataIDBadRequest(pResp, sID) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		return TRUE;
	}
	iDataID = 0;
	if ( sID && sID[0] != '\0' ) {
		(void)XS_HttpParsePositiveInt64(sID, &iDataID);
	}

	if ( iDataID <= 0 && ((sNamespace && sNamespace[0] != '\0') || (sTag && sTag[0] != '\0')) ) {
		iDataID = XS_BusDataFindFirst(
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
			(sTag && sTag[0] != '\0') ? sTag : NULL
		);
	}

	if ( iDataID <= 0 ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"data not found\",\"data_id\":0,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		xrtHttpdResponseSetStatus(pResp, 404, "Not Found");
		return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
	}

	sJson = XS_BusBuildDataJson(iDataID);
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sID ) {
		xrtFree(sID);
	}
	if ( sJson == NULL ) {
		xvalue objExists = XS_BusDataGet(iDataID);

		if ( objExists == NULL ) {
			snprintf(
				sBody,
				sizeof(sBody),
				"{\"result\":false,\"message\":\"data not found\",\"data_id\":%lld}",
				(long long)iDataID
			);
			xrtHttpdResponseSetStatus(pResp, 404, "Not Found");
			return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
		}

		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"data build failed\",\"data_id\":%lld}",
			(long long)iDataID
		);
		xrtHttpdResponseSetStatus(pResp, 500, "Internal Server Error");
		return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
	}

	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleBusValues(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char* sJson;
	char sBody[320];

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/values") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus values api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sJson = XS_BusBuildDataListJsonEx(
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
		(sTag && sTag[0] != '\0') ? sTag : NULL
	);
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sJson == NULL ) {
		snprintf(sBody, sizeof(sBody), "{\"result\":false,\"message\":\"data list build failed\"}");
		xrtHttpdResponseSetStatus(pResp, 500, "Internal Server Error");
		return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
	}

	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleBusRetain(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char* sID;
	char sBody[384];
	int64 iDataID;
	bool bOk;
	int32 iBusCode = XS_BUS_ERR_NONE;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/retain") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus retain api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	XS_BusClearLastError();
	if ( XS_HttpRespondBusDataIDBadRequest(pResp, sID) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		return TRUE;
	}
	iDataID = 0;
	if ( sID && sID[0] != '\0' ) {
		(void)XS_HttpParsePositiveInt64(sID, &iDataID);
	}
	iDataID = XS_BusDataResolveID(
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
		(sTag && sTag[0] != '\0') ? sTag : NULL,
		iDataID
	);
	bOk = (iDataID > 0) ? XS_BusDataRetainManaged(iDataID) : FALSE;
	iBusCode = XS_BusGetLastErrorCode();
	if ( !bOk && iBusCode != XS_BUS_ERR_NONE ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"retain failed\",\"bus_code\":%d,\"bus_error\":\"%s\",\"data_id\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			(int)iBusCode,
			XS_BusGetLastError(),
			(long long)iDataID,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
	} else {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":%s,\"message\":\"%s\",\"data_id\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			bOk ? "true" : "false",
			bOk ? "retain ok" : "data not found",
			(long long)iDataID,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
	}
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sID ) {
		xrtFree(sID);
	}
	xrtHttpdResponseSetStatus(
		pResp,
		bOk ? 200 : (iBusCode != XS_BUS_ERR_NONE ? XS_HttpBusErrorStatusCode(iBusCode) : 404),
		bOk ? "OK" : (iBusCode != XS_BUS_ERR_NONE ? XS_HttpBusErrorStatusText(iBusCode) : "Not Found")
	);
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpHandleBusRelease(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char* sID;
	char sBody[384];
	int64 iDataID;
	bool bOk;
	int32 iBusCode = XS_BUS_ERR_NONE;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/release") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus release api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	XS_BusClearLastError();
	if ( XS_HttpRespondBusDataIDBadRequest(pResp, sID) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		return TRUE;
	}
	iDataID = 0;
	if ( sID && sID[0] != '\0' ) {
		(void)XS_HttpParsePositiveInt64(sID, &iDataID);
	}
	iDataID = XS_BusDataResolveID(
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
		(sTag && sTag[0] != '\0') ? sTag : NULL,
		iDataID
	);
	bOk = (iDataID > 0) ? XS_BusDataReleaseManaged(iDataID) : FALSE;
	iBusCode = XS_BusGetLastErrorCode();
	if ( !bOk && iBusCode != XS_BUS_ERR_NONE ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"release failed\",\"bus_code\":%d,\"bus_error\":\"%s\",\"data_id\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			(int)iBusCode,
			XS_BusGetLastError(),
			(long long)iDataID,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
	} else {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":%s,\"message\":\"%s\",\"data_id\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			bOk ? "true" : "false",
			bOk ? "release ok" : "data not found",
			(long long)iDataID,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
	}
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sID ) {
		xrtFree(sID);
	}
	xrtHttpdResponseSetStatus(
		pResp,
		bOk ? 200 : (iBusCode != XS_BUS_ERR_NONE ? XS_HttpBusErrorStatusCode(iBusCode) : 404),
		bOk ? "OK" : (iBusCode != XS_BUS_ERR_NONE ? XS_HttpBusErrorStatusText(iBusCode) : "Not Found")
	);
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpHandleBusTouch(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char* sID;
	char* sTTL;
	char sBody[512];
	int64 iDataID;
	int64 iTTL;
	int64 tExpire;
	bool bOk;
	int32 iBusCode = XS_BUS_ERR_NONE;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/touch") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus touch api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	if ( XS_HttpRespondBusDataIDBadRequest(pResp, sID) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		return TRUE;
	}
	sTTL = XS_HttpQueryDup(pReq->sQuery, "ttl");
	if ( XS_HttpRespondBusTTLBadRequest(pResp, sTTL) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		if ( sTTL ) {
			xrtFree(sTTL);
		}
		return TRUE;
	}
	XS_BusClearLastError();
	iDataID = 0;
	if ( sID && sID[0] != '\0' ) {
		(void)XS_HttpParsePositiveInt64(sID, &iDataID);
	}
	iDataID = XS_BusDataResolveID(
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
		(sTag && sTag[0] != '\0') ? sTag : NULL,
		iDataID
	);
	iTTL = 0;
	if ( sTTL && sTTL[0] != '\0' ) {
		(void)XS_HttpParseNonNegativeInt64(sTTL, &iTTL);
	}
	if ( iTTL <= 0 ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"ttl must be greater than 0\",\"data_id\":%lld,\"ttl\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			(long long)iDataID,
			(long long)iTTL,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		if ( sTTL ) {
			xrtFree(sTTL);
		}
		xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
		return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
	}

	bOk = (iDataID > 0) ? XS_BusDataTouchTTL(iDataID, iTTL) : FALSE;
	iBusCode = XS_BusGetLastErrorCode();
	tExpire = bOk ? XS_BusDataGetExpireTime(iDataID) : 0;
	if ( !bOk && iBusCode != XS_BUS_ERR_NONE ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"touch failed\",\"bus_code\":%d,\"bus_error\":\"%s\",\"data_id\":%lld,\"ttl\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			(int)iBusCode,
			XS_BusGetLastError(),
			(long long)iDataID,
			(long long)iTTL,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
	} else {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":%s,\"message\":\"%s\",\"data_id\":%lld,\"ttl\":%lld,\"expire_time\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			bOk ? "true" : "false",
			bOk ? "ttl updated" : "data not found",
			(long long)iDataID,
			(long long)iTTL,
			(long long)tExpire,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
	}
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sID ) {
		xrtFree(sID);
	}
	if ( sTTL ) {
		xrtFree(sTTL);
	}
	xrtHttpdResponseSetStatus(
		pResp,
		bOk ? 200 : (iBusCode != XS_BUS_ERR_NONE ? XS_HttpBusErrorStatusCode(iBusCode) : 404),
		bOk ? "OK" : (iBusCode != XS_BUS_ERR_NONE ? XS_HttpBusErrorStatusText(iBusCode) : "Not Found")
	);
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpHandleBusSet(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char* sID;
	char* sText;
	char* sJson;
	char* sTTL;
	char sBody[640];
	int64 iDataID;
	int64 iTTL = -1;
	int64 tExpire = 0;
	bool bOk = FALSE;
	int32 iBusCode = XS_BUS_ERR_NONE;
	xvalue objData = NULL;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/set") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus set api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	if ( XS_HttpRespondBusDataIDBadRequest(pResp, sID) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		return TRUE;
	}
	sText = XS_HttpQueryDup(pReq->sQuery, "text");
	sJson = XS_HttpQueryDup(pReq->sQuery, "json");
	sTTL = XS_HttpQueryDup(pReq->sQuery, "ttl");
	if ( XS_HttpRespondBusTTLBadRequest(pResp, sTTL) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		if ( sTTL ) {
			xrtFree(sTTL);
		}
		return TRUE;
	}
	XS_BusClearLastError();
	iDataID = 0;
	if ( sID && sID[0] != '\0' ) {
		(void)XS_HttpParsePositiveInt64(sID, &iDataID);
	}
	iDataID = XS_BusDataResolveID(
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
		(sTag && sTag[0] != '\0') ? sTag : NULL,
		iDataID
	);
	if ( sTTL && sTTL[0] != '\0' ) {
		(void)XS_HttpParseNonNegativeInt64(sTTL, &iTTL);
	}

	if ( iDataID <= 0 ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"data not found\",\"data_id\":0,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
		xrtHttpdResponseSetStatus(pResp, 404, "Not Found");
		goto ExitBusSet;
	}

	if ( sJson && sJson[0] != '\0' ) {
		if ( !XS_HttpLooksLikeJson(sJson) ) {
			snprintf(sBody, sizeof(sBody), "{\"result\":false,\"message\":\"invalid json payload\",\"data_id\":%lld}", (long long)iDataID);
			xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
			goto ExitBusSet;
		}
		objData = xrtParseJSON(sJson, 0);
		if ( objData == NULL ) {
			snprintf(sBody, sizeof(sBody), "{\"result\":false,\"message\":\"invalid json payload\",\"data_id\":%lld}", (long long)iDataID);
			xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
			goto ExitBusSet;
		}
	} else {
		objData = xvoCreateTable();
	}

	if ( objData == NULL ) {
		snprintf(sBody, sizeof(sBody), "{\"result\":false,\"message\":\"payload build failed\",\"data_id\":%lld}", (long long)iDataID);
		xrtHttpdResponseSetStatus(pResp, 500, "Internal Server Error");
		goto ExitBusSet;
	}

	if ( xvoType(objData) != XVO_DT_TABLE ) {
		xvalue objWrap = xvoCreateTable();

		xvoTableSetValue(objWrap, "data", 4, objData, TRUE);
		objData = objWrap;
	}

	xvoTableSetText(objData, "source", 6, "__xs.bus.set", 0, FALSE);
	xvoTableSetText(objData, "server", 6, objServer->Name ? objServer->Name : "", 0, FALSE);
	xvoTableSetText(objData, "host", 4, objHost->Name ? objHost->Name : "", 0, FALSE);
	xvoTableSetText(objData, "path", 4, (ptr)pReq->sPath, 0, FALSE);
	xvoTableSetText(objData, "query", 5, (ptr)pReq->sQuery, 0, FALSE);
	xvoTableSetTime(objData, "update_time", 11, xrtNow());
	if ( (sText && sText[0] != '\0') && xvoTableGetValue(objData, "text", 4) == NULL ) {
		xvoTableSetText(objData, "text", 4, sText, 0, FALSE);
	}

	bOk = XS_BusDataSetValueEx(iDataID, objData, iTTL, iTTL >= 0);
	iBusCode = XS_BusGetLastErrorCode();
	tExpire = bOk ? XS_BusDataGetExpireTime(iDataID) : 0;
	if ( !bOk && iBusCode != XS_BUS_ERR_NONE ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"update failed\",\"bus_code\":%d,\"bus_error\":\"%s\",\"data_id\":%lld,\"ttl\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			(int)iBusCode,
			XS_BusGetLastError(),
			(long long)iDataID,
			(long long)iTTL,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
	} else {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":%s,\"message\":\"%s\",\"data_id\":%lld,\"ttl\":%lld,\"expire_time\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			bOk ? "true" : "false",
			bOk ? "data updated" : "update failed",
			(long long)iDataID,
			(long long)iTTL,
			(long long)tExpire,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
	}
	xrtHttpdResponseSetStatus(
		pResp,
		bOk ? 200 : (iBusCode != XS_BUS_ERR_NONE ? XS_HttpBusErrorStatusCode(iBusCode) : 500),
		bOk ? "OK" : (iBusCode != XS_BUS_ERR_NONE ? XS_HttpBusErrorStatusText(iBusCode) : "Internal Server Error")
	);

ExitBusSet:
	if ( objData ) {
		xvoUnref(objData);
	}
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sID ) {
		xrtFree(sID);
	}
	if ( sText ) {
		xrtFree(sText);
	}
	if ( sJson ) {
		xrtFree(sJson);
	}
	if ( sTTL ) {
		xrtFree(sTTL);
	}
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpHandleBusRemove(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char* sID;
	char sBody[320];
	bool bAll;
	int64 iDataID;
	int64 iRemoved;
	int64 iBatchRemoved;
	int32 iBusCode = XS_BUS_ERR_NONE;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/remove") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus remove api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	XS_BusClearLastError();
	if ( !XS_HttpQueryBoolEx(pReq->sQuery, "all", FALSE, &bAll, NULL) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		return XS_HttpRespondNamedBoolBadRequestJson(pResp, "all");
	}
	iRemoved = 0;
	iDataID = 0;
	if ( XS_HttpRespondBusDataIDBadRequest(pResp, sID) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sID ) {
			xrtFree(sID);
		}
		return TRUE;
	}

	if ( sID && sID[0] != '\0' ) {
		(void)XS_HttpParsePositiveInt64(sID, &iDataID);
		if ( iDataID > 0 && XS_BusDataRemove(iDataID) ) {
			iRemoved = 1;
		}
	} else if ( bAll ) {
		do {
			iBatchRemoved = XS_BusDataRemoveByQuery(
				(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
				(sTag && sTag[0] != '\0') ? sTag : NULL,
				256
			);
			iRemoved += iBatchRemoved;
		} while ( iBatchRemoved > 0 );
	} else if ( (sNamespace && sNamespace[0] != '\0') || (sTag && sTag[0] != '\0') ) {
		iDataID = XS_BusDataFindFirst(
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
			(sTag && sTag[0] != '\0') ? sTag : NULL
		);
		if ( iDataID > 0 && XS_BusDataRemove(iDataID) ) {
			iRemoved = 1;
		}
	}
	iBusCode = XS_BusGetLastErrorCode();

	if ( iRemoved == 0 && iBusCode != XS_BUS_ERR_NONE ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"remove failed\",\"bus_code\":%d,\"bus_error\":\"%s\",\"removed\":0,\"data_id\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\",\"all\":%s}",
			(int)iBusCode,
			XS_BusGetLastError(),
			(long long)iDataID,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : "",
			bAll ? "true" : "false"
		);
	} else {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":true,\"removed\":%lld,\"data_id\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\",\"all\":%s}",
			(long long)iRemoved,
			(long long)iDataID,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : "",
			bAll ? "true" : "false"
		);
	}
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sID ) {
		xrtFree(sID);
	}
	xrtHttpdResponseSetStatus(
		pResp,
		(iRemoved > 0 || iBusCode == XS_BUS_ERR_NONE) ? 200 : XS_HttpBusErrorStatusCode(iBusCode),
		(iRemoved > 0 || iBusCode == XS_BUS_ERR_NONE) ? "OK" : XS_HttpBusErrorStatusText(iBusCode)
	);
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpHandleBusReset(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/reset") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus reset api disabled");
	}

	XS_BusClearStats();
	objRet = XS_BusBuildStatusValueNoSweep();
	if ( objRet == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "bus reset build failed");
	}
	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "bus reset stringify failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleBusSweep(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sMaxPass;
	xvalue objRet;
	char* sJson;
	bool bDrain;
	int64 iPassCount = 0;
	int64 iRemovedTotal = 0;
	int64 iRemain = 0;
	int32 iMaxPass = 0;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/sweep") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus sweep api disabled");
	}

	{
		bool bAll = FALSE;
		bool bDrainFlag = FALSE;

		if ( !XS_HttpQueryBoolEx(pReq->sQuery, "all", FALSE, &bAll, NULL) ) {
			return XS_HttpRespondNamedBoolBadRequestJson(pResp, "all");
		}
		if ( !XS_HttpQueryBoolEx(pReq->sQuery, "drain", FALSE, &bDrainFlag, NULL) ) {
			return XS_HttpRespondNamedBoolBadRequestJson(pResp, "drain");
		}
		bDrain = bAll || bDrainFlag;
	}
	sMaxPass = XS_HttpQueryDup(pReq->sQuery, "max_pass");
	if ( sMaxPass && sMaxPass[0] != '\0' ) {
		int64 iMaxPassValue = 0;

		if ( !XS_HttpParseInt64(sMaxPass, &iMaxPassValue) ) {
			xrtFree(sMaxPass);
			return XS_HttpRespondBusNamedIntBadRequest(pResp, "max_pass");
		}
		if ( iMaxPassValue <= 0 ) {
			xrtFree(sMaxPass);
			return XS_HttpRespondJsonResult(pResp, 400, "Bad Request", FALSE, "max_pass must be > 0");
		}
		if ( iMaxPassValue > 1024 ) {
			xrtFree(sMaxPass);
			return XS_HttpRespondJsonResult(pResp, 400, "Bad Request", FALSE, "max_pass must be <= 1024");
		}
		iMaxPass = (int32)iMaxPassValue;
	}
	if ( bDrain ) {
		XS_BusSweepExpiredDataDrain(iMaxPass, &iPassCount, &iRemovedTotal, &iRemain);
	} else {
		XS_BusSweepExpiredDataForce();
		iPassCount = 1;
	}
	objRet = XS_BusBuildLimitValue();
	if ( sMaxPass ) {
		xrtFree(sMaxPass);
	}
	if ( objRet == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "bus sweep build failed");
	}
	if ( !bDrain ) {
		iRemovedTotal = xvoTableGetInt(objRet, "last_sweep_removed", sizeof("last_sweep_removed") - 1);
		iRemain = xvoTableGetInt(objRet, "last_sweep_remain", sizeof("last_sweep_remain") - 1);
	}
	xvoTableSetBool(objRet, "result", 6, TRUE);
	xvoTableSetBool(objRet, "forced", 6, TRUE);
	xvoTableSetBool(objRet, "drain", 5, bDrain);
	xvoTableSetInt(objRet, "pass_count", 10, iPassCount);
	xvoTableSetInt(objRet, "pass_limit", 10, (iMaxPass > 0) ? iMaxPass : XS_BUS_SWEEP_DRAIN_MAX_PASS);
	xvoTableSetInt(objRet, "removed_total", 13, iRemovedTotal);
	xvoTableSetInt(objRet, "remain_after", 12, iRemain);
	xvoTableSetBool(objRet, "pass_limited", 12, bDrain && (iRemain > 0) && (iPassCount >= ((iMaxPass > 0) ? iMaxPass : XS_BUS_SWEEP_DRAIN_MAX_PASS)));
	xvoTableSetText(objRet, "message", 7, bDrain ? "bus sweep drain ok" : "bus sweep ok", 0, FALSE);
	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "bus sweep stringify failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleBusLimits(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sDataLimit;
	char* sQueueLimit;
	char* sNamespaceLimit;
	char* sNamespaceDataLimit;
	char* sSweepIntervalMS;
	char* sReadonlyNamespaces;
	char* sDisabledNamespaces;
	char* sTTLRequiredNamespaces;
	char* sTagRequiredNamespaces;
	char sReadonlyRules[XS_BUS_NAMESPACE_RULES_MAX_LEN];
	char sDisabledRules[XS_BUS_NAMESPACE_RULES_MAX_LEN];
	char sTTLRequiredRules[XS_BUS_NAMESPACE_RULES_MAX_LEN];
	char sTagRequiredRules[XS_BUS_NAMESPACE_RULES_MAX_LEN];
	const char* sRuleError = NULL;
	bool bHasReadonlyNamespaces = FALSE;
	bool bHasDisabledNamespaces = FALSE;
	bool bHasTTLRequiredNamespaces = FALSE;
	bool bHasTagRequiredNamespaces = FALSE;
	bool bSetDataLimit = FALSE;
	bool bSetQueueLimit = FALSE;
	bool bSetNamespaceLimit = FALSE;
	bool bSetNamespaceDataLimit = FALSE;
	bool bSetSweepIntervalMS = FALSE;
	bool bSetReadonlyNamespaces = FALSE;
	bool bSetDisabledNamespaces = FALSE;
	bool bSetTTLRequiredNamespaces = FALSE;
	bool bSetTagRequiredNamespaces = FALSE;
	int64 iDataLimit = 0;
	int64 iQueueLimit = 0;
	int64 iNamespaceLimit = 0;
	int64 iNamespaceDataLimit = 0;
	int64 iSweepIntervalMS = 0;
	xvalue objRet;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/limits") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus limit api disabled");
	}

	sDataLimit = XS_HttpQueryDup(pReq->sQuery, "data_limit");
	sQueueLimit = XS_HttpQueryDup(pReq->sQuery, "queue_limit");
	sNamespaceLimit = XS_HttpQueryDup(pReq->sQuery, "namespace_limit");
	sNamespaceDataLimit = XS_HttpQueryDup(pReq->sQuery, "namespace_data_limit");
	sSweepIntervalMS = XS_HttpQueryDup(pReq->sQuery, "sweep_interval_ms");
	sReadonlyNamespaces = XS_HttpQueryDup(pReq->sQuery, "readonly_namespaces");
	sDisabledNamespaces = XS_HttpQueryDup(pReq->sQuery, "disabled_namespaces");
	sTTLRequiredNamespaces = XS_HttpQueryDup(pReq->sQuery, "ttl_required_namespaces");
	sTagRequiredNamespaces = XS_HttpQueryDup(pReq->sQuery, "tag_required_namespaces");
	bHasReadonlyNamespaces = XS_HttpQueryHas(pReq->sQuery, "readonly_namespaces");
	bHasDisabledNamespaces = XS_HttpQueryHas(pReq->sQuery, "disabled_namespaces");
	bHasTTLRequiredNamespaces = XS_HttpQueryHas(pReq->sQuery, "ttl_required_namespaces");
	bHasTagRequiredNamespaces = XS_HttpQueryHas(pReq->sQuery, "tag_required_namespaces");
	sReadonlyRules[0] = '\0';
	sDisabledRules[0] = '\0';
	sTTLRequiredRules[0] = '\0';
	sTagRequiredRules[0] = '\0';
	if ( sDataLimit && sDataLimit[0] != '\0' ) {
		if ( !XS_HttpParseInt64(sDataLimit, &iDataLimit) ) {
			XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
			return XS_HttpRespondBusNamedIntBadRequest(pResp, "data_limit");
		}
		if ( iDataLimit < 0 ) {
			XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
			return XS_HttpRespondJsonResult(pResp, 400, "Bad Request", FALSE, "data_limit must be >= 0");
		}
		bSetDataLimit = TRUE;
	}
	if ( sQueueLimit && sQueueLimit[0] != '\0' ) {
		if ( !XS_HttpParseInt64(sQueueLimit, &iQueueLimit) ) {
			XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
			return XS_HttpRespondBusNamedIntBadRequest(pResp, "queue_limit");
		}
		if ( iQueueLimit < 0 ) {
			XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
			return XS_HttpRespondJsonResult(pResp, 400, "Bad Request", FALSE, "queue_limit must be >= 0");
		}
		bSetQueueLimit = TRUE;
	}
	if ( sNamespaceLimit && sNamespaceLimit[0] != '\0' ) {
		if ( !XS_HttpParseInt64(sNamespaceLimit, &iNamespaceLimit) ) {
			XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
			return XS_HttpRespondBusNamedIntBadRequest(pResp, "namespace_limit");
		}
		if ( iNamespaceLimit < 0 ) {
			XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
			return XS_HttpRespondJsonResult(pResp, 400, "Bad Request", FALSE, "namespace_limit must be >= 0");
		}
		bSetNamespaceLimit = TRUE;
	}
	if ( sNamespaceDataLimit && sNamespaceDataLimit[0] != '\0' ) {
		if ( !XS_HttpParseInt64(sNamespaceDataLimit, &iNamespaceDataLimit) ) {
			XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
			return XS_HttpRespondBusNamedIntBadRequest(pResp, "namespace_data_limit");
		}
		if ( iNamespaceDataLimit < 0 ) {
			XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
			return XS_HttpRespondJsonResult(pResp, 400, "Bad Request", FALSE, "namespace_data_limit must be >= 0");
		}
		bSetNamespaceDataLimit = TRUE;
	}
	if ( sSweepIntervalMS && sSweepIntervalMS[0] != '\0' ) {
		if ( !XS_HttpParseInt64(sSweepIntervalMS, &iSweepIntervalMS) ) {
			XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
			return XS_HttpRespondBusNamedIntBadRequest(pResp, "sweep_interval_ms");
		}
		if ( iSweepIntervalMS < 0 ) {
			XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
			return XS_HttpRespondJsonResult(pResp, 400, "Bad Request", FALSE, "sweep_interval_ms must be >= 0");
		}
		bSetSweepIntervalMS = TRUE;
	}
	if ( bHasReadonlyNamespaces ) {
		if ( sReadonlyNamespaces && sReadonlyNamespaces[0] != '\0' ) {
			if ( !XS_BusNormalizeNamespaceRules(sReadonlyNamespaces, sReadonlyRules, sizeof(sReadonlyRules), &sRuleError) ) {
				XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
				return XS_HttpRespondJsonResult(pResp, 400, "Bad Request", FALSE, sRuleError ? sRuleError : "invalid readonly namespaces");
			}
		}
		bSetReadonlyNamespaces = TRUE;
	}
	if ( bHasDisabledNamespaces ) {
		if ( sDisabledNamespaces && sDisabledNamespaces[0] != '\0' ) {
			if ( !XS_BusNormalizeNamespaceRules(sDisabledNamespaces, sDisabledRules, sizeof(sDisabledRules), &sRuleError) ) {
				XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
				return XS_HttpRespondJsonResult(pResp, 400, "Bad Request", FALSE, sRuleError ? sRuleError : "invalid disabled namespaces");
			}
		}
		bSetDisabledNamespaces = TRUE;
	}
	if ( bHasTTLRequiredNamespaces ) {
		if ( sTTLRequiredNamespaces && sTTLRequiredNamespaces[0] != '\0' ) {
			if ( !XS_BusNormalizeNamespaceRules(sTTLRequiredNamespaces, sTTLRequiredRules, sizeof(sTTLRequiredRules), &sRuleError) ) {
				XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
				return XS_HttpRespondJsonResult(pResp, 400, "Bad Request", FALSE, sRuleError ? sRuleError : "invalid ttl required namespaces");
			}
		}
		bSetTTLRequiredNamespaces = TRUE;
	}
	if ( bHasTagRequiredNamespaces ) {
		if ( sTagRequiredNamespaces && sTagRequiredNamespaces[0] != '\0' ) {
			if ( !XS_BusNormalizeNamespaceRules(sTagRequiredNamespaces, sTagRequiredRules, sizeof(sTagRequiredRules), &sRuleError) ) {
				XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
				return XS_HttpRespondJsonResult(pResp, 400, "Bad Request", FALSE, sRuleError ? sRuleError : "invalid tag required namespaces");
			}
		}
		bSetTagRequiredNamespaces = TRUE;
	}
	if ( bSetDataLimit || bSetQueueLimit || bSetNamespaceLimit || bSetNamespaceDataLimit || bSetReadonlyNamespaces || bSetDisabledNamespaces || bSetTTLRequiredNamespaces || bSetTagRequiredNamespaces ) {
		XS_BusConfigureLimits(
			bSetDataLimit,
			iDataLimit,
			bSetQueueLimit,
			iQueueLimit,
			bSetNamespaceLimit,
			iNamespaceLimit,
			bSetNamespaceDataLimit,
			iNamespaceDataLimit,
			bSetReadonlyNamespaces,
			sReadonlyRules,
			bSetDisabledNamespaces,
			sDisabledRules,
			bSetTTLRequiredNamespaces,
			sTTLRequiredRules,
			bSetTagRequiredNamespaces,
			sTagRequiredRules
		);
	}
	if ( bSetSweepIntervalMS ) {
		XS_BusConfigureSweep(TRUE, iSweepIntervalMS);
	}

	objRet = XS_BusBuildLimitValue();
	XS_HttpFreeBusLimitQueryValues(sDataLimit, sQueueLimit, sNamespaceLimit, sNamespaceDataLimit, sSweepIntervalMS, sReadonlyNamespaces, sDisabledNamespaces, sTTLRequiredNamespaces, sTagRequiredNamespaces);
	if ( objRet == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "bus limit build failed");
	}
	xvoTableSetBool(objRet, "updated", 7, bSetDataLimit || bSetQueueLimit || bSetNamespaceLimit || bSetNamespaceDataLimit || bSetSweepIntervalMS || bSetReadonlyNamespaces || bSetDisabledNamespaces || bSetTTLRequiredNamespaces || bSetTagRequiredNamespaces);
	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "bus limit stringify failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleBusRegister(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char* sTTL;
	char sBody[384];
	int64 iTTL;
	int64 iDataID;
	int32 iBusCode;
	xvalue objData;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/register") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus register api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sTTL = XS_HttpQueryDup(pReq->sQuery, "ttl");
	if ( XS_HttpRespondBusTTLBadRequest(pResp, sTTL) ) {
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sTTL ) {
			xrtFree(sTTL);
		}
		return TRUE;
	}
	iTTL = 0;
	if ( sTTL && sTTL[0] != '\0' ) {
		(void)XS_HttpParseNonNegativeInt64(sTTL, &iTTL);
	}

	objData = xvoCreateTable();
	xvoTableSetText(objData, "source", 6, "__xs.bus.register", 0, FALSE);
	xvoTableSetText(objData, "server", 6, objServer->Name ? objServer->Name : "", 0, FALSE);
	xvoTableSetText(objData, "host", 4, objHost->Name ? objHost->Name : "", 0, FALSE);
	xvoTableSetText(objData, "path", 4, (ptr)pReq->sPath, 0, FALSE);
	xvoTableSetText(objData, "query", 5, (ptr)pReq->sQuery, 0, FALSE);
	xvoTableSetTime(objData, "create_time", 11, xrtNow());

	iDataID = XS_BusDataRegisterEx(
		objData,
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
		(sTag && sTag[0] != '\0') ? sTag : NULL,
		iTTL
	);
	xvoUnref(objData);

	iBusCode = XS_BusGetLastErrorCode();
	if ( iDataID <= 0 ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"register failed\",\"bus_code\":%d,\"bus_error\":\"%s\"}",
			(int)iBusCode,
			XS_BusGetLastError()
		);
	} else {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":true,\"message\":\"data registered\",\"data_id\":%lld,\"ttl\":%lld,\"namespace\":\"%s\",\"tag\":\"%s\"}",
			(long long)iDataID,
			(long long)iTTL,
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : "",
			(sTag && sTag[0] != '\0') ? sTag : ""
		);
	}

	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sTTL ) {
		xrtFree(sTTL);
	}
	xrtHttpdResponseSetStatus(
		pResp,
		iDataID > 0 ? 200 : XS_HttpBusErrorStatusCode(iBusCode),
		iDataID > 0 ? "OK" : XS_HttpBusErrorStatusText(iBusCode)
	);
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpHandleBusSend(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sTarget;
	char* sTopic;
	char* sServerName;
	char* sHostName;
	char* sNamespace;
	char* sTag;
	char* sDataID;
	char* sText;
	char* sJson;
	char* sTTL;
	bool bPersist;
	char sBody[512];
	bool bSendOk = FALSE;
	int64 iDataID = 0;
	int64 iTTL = -1;
	bool bAutoRegister = FALSE;
	int32 iBusCode = 0;
	xvalue objData = NULL;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/send") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "bus send api disabled");
	}

	sTarget = XS_HttpQueryDup(pReq->sQuery, "target");
	sTopic = XS_HttpQueryDup(pReq->sQuery, "topic");
	sServerName = XS_HttpQueryDup(pReq->sQuery, "server");
	sHostName = XS_HttpQueryDup(pReq->sQuery, "host");
	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	if ( XS_HttpRespondBusNamespaceBadRequest(pResp, sNamespace) ) {
		if ( sTarget ) {
			xrtFree(sTarget);
		}
		if ( sTopic ) {
			xrtFree(sTopic);
		}
		if ( sServerName ) {
			xrtFree(sServerName);
		}
		if ( sHostName ) {
			xrtFree(sHostName);
		}
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		return TRUE;
	}
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sDataID = XS_HttpQueryDup(pReq->sQuery, "data_id");
	if ( XS_HttpRespondBusDataIDBadRequest(pResp, sDataID) ) {
		if ( sTarget ) {
			xrtFree(sTarget);
		}
		if ( sTopic ) {
			xrtFree(sTopic);
		}
		if ( sServerName ) {
			xrtFree(sServerName);
		}
		if ( sHostName ) {
			xrtFree(sHostName);
		}
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sDataID ) {
			xrtFree(sDataID);
		}
		return TRUE;
	}
	sText = XS_HttpQueryDup(pReq->sQuery, "text");
	sJson = XS_HttpQueryDup(pReq->sQuery, "json");
	sTTL = XS_HttpQueryDup(pReq->sQuery, "ttl");
	if ( XS_HttpRespondBusTTLBadRequest(pResp, sTTL) ) {
		if ( sTarget ) {
			xrtFree(sTarget);
		}
		if ( sTopic ) {
			xrtFree(sTopic);
		}
		if ( sServerName ) {
			xrtFree(sServerName);
		}
		if ( sHostName ) {
			xrtFree(sHostName);
		}
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sDataID ) {
			xrtFree(sDataID);
		}
		if ( sText ) {
			xrtFree(sText);
		}
		if ( sJson ) {
			xrtFree(sJson);
		}
		if ( sTTL ) {
			xrtFree(sTTL);
		}
		return TRUE;
	}
	if ( !XS_HttpQueryBoolEx(pReq->sQuery, "persist", FALSE, &bPersist, NULL) ) {
		if ( sTarget ) {
			xrtFree(sTarget);
		}
		if ( sTopic ) {
			xrtFree(sTopic);
		}
		if ( sServerName ) {
			xrtFree(sServerName);
		}
		if ( sHostName ) {
			xrtFree(sHostName);
		}
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sDataID ) {
			xrtFree(sDataID);
		}
		if ( sText ) {
			xrtFree(sText);
		}
		if ( sJson ) {
			xrtFree(sJson);
		}
		if ( sTTL ) {
			xrtFree(sTTL);
		}
		return XS_HttpRespondNamedBoolBadRequestJson(pResp, "persist");
	}

	if ( sTTL && sTTL[0] != '\0' ) {
		(void)XS_HttpParseNonNegativeInt64(sTTL, &iTTL);
	}

	if ( sDataID && sDataID[0] != '\0' ) {
		(void)XS_HttpParsePositiveInt64(sDataID, &iDataID);
	} else if ( (sNamespace && sNamespace[0] != '\0') || (sTag && sTag[0] != '\0') ) {
		iDataID = XS_BusDataFindFirst(
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
			(sTag && sTag[0] != '\0') ? sTag : NULL
		);
	}

	if ( sTopic == NULL || sTopic[0] == '\0' ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"topic required\"}"
		);
		xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
	} else {
		const char* pTarget = (sTarget && sTarget[0] != '\0') ? sTarget : "server";
		const char* pServer = (sServerName && sServerName[0] != '\0') ? sServerName : objServer->Name;
		const char* pHost = (sHostName && sHostName[0] != '\0') ? sHostName : objHost->Name;

		if (
			strcmp(pTarget, "server") != 0 &&
			strcmp(pTarget, "host") != 0 &&
			strcmp(pTarget, "broadcast") != 0
		) {
			snprintf(
				sBody,
				sizeof(sBody),
				"{\"result\":false,\"message\":\"invalid target\",\"target\":\"%s\"}",
				pTarget
			);
			xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
			goto ExitBusSend;
		}

		if ( iDataID <= 0 && ((sText && sText[0] != '\0') || (sJson && sJson[0] != '\0') || (sNamespace && sNamespace[0] != '\0') || (sTag && sTag[0] != '\0') || (iTTL >= 0)) ) {
			int64 iRegisterTTL = (iTTL >= 0) ? iTTL : 60000;

			if ( sJson && sJson[0] != '\0' ) {
				if ( !XS_HttpLooksLikeJson(sJson) ) {
					snprintf(
						sBody,
						sizeof(sBody),
						"{\"result\":false,\"message\":\"invalid json payload\"}"
					);
					xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
					goto ExitBusSend;
				}
				objData = xrtParseJSON(sJson, 0);
				if ( objData == NULL ) {
					snprintf(
						sBody,
						sizeof(sBody),
						"{\"result\":false,\"message\":\"invalid json payload\"}"
					);
					xrtHttpdResponseSetStatus(pResp, 400, "Bad Request");
					goto ExitBusSend;
				}
			} else {
				objData = xvoCreateTable();
			}

			if ( objData == NULL ) {
				snprintf(
					sBody,
					sizeof(sBody),
					"{\"result\":false,\"message\":\"payload build failed\"}"
				);
				xrtHttpdResponseSetStatus(pResp, 500, "Internal Server Error");
				goto ExitBusSend;
			}

			if ( xvoType(objData) != XVO_DT_TABLE ) {
				xvalue objWrap = xvoCreateTable();

				xvoTableSetValue(objWrap, "data", 4, objData, TRUE);
				objData = objWrap;
			}

			xvoTableSetText(objData, "source", 6, "__xs.bus.send", 0, FALSE);
			xvoTableSetText(objData, "server", 6, objServer->Name ? objServer->Name : "", 0, FALSE);
			xvoTableSetText(objData, "host", 4, objHost->Name ? objHost->Name : "", 0, FALSE);
			xvoTableSetText(objData, "path", 4, (ptr)pReq->sPath, 0, FALSE);
			xvoTableSetText(objData, "query", 5, (ptr)pReq->sQuery, 0, FALSE);
			xvoTableSetText(objData, "topic", 5, sTopic, 0, FALSE);
			xvoTableSetText(objData, "target", 6, (ptr)pTarget, 0, FALSE);
			if ( (sText && sText[0] != '\0') && xvoTableGetValue(objData, "text", 4) == NULL ) {
				xvoTableSetText(objData, "text", 4, sText, 0, FALSE);
			}
			xvoTableSetTime(objData, "create_time", 11, xrtNow());
			iDataID = XS_BusDataRegisterEx(
				objData,
				(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
				(sTag && sTag[0] != '\0') ? sTag : NULL,
				iRegisterTTL
			);
			xvoUnref(objData);
			objData = NULL;
			iBusCode = XS_BusGetLastErrorCode();
			if ( iDataID <= 0 ) {
				snprintf(
					sBody,
					sizeof(sBody),
					"{\"result\":false,\"message\":\"auto register failed\",\"bus_code\":%d,\"bus_error\":\"%s\"}",
					(int)iBusCode,
					XS_BusGetLastError()
				);
				xrtHttpdResponseSetStatus(pResp, XS_HttpBusErrorStatusCode(iBusCode), XS_HttpBusErrorStatusText(iBusCode));
				goto ExitBusSend;
			}
			bAutoRegister = TRUE;
			iTTL = iRegisterTTL;
		}

		if ( strcmp(pTarget, "host") == 0 ) {
			bSendOk = XS_BusSendToHost(pServer, pHost, sTopic, iDataID, NULL);
		} else if ( strcmp(pTarget, "broadcast") == 0 ) {
			bSendOk = XS_BusBroadcast(sTopic, iDataID, NULL);
		} else {
			bSendOk = XS_BusSendToServer(pServer, sTopic, iDataID, NULL);
		}
		iBusCode = XS_BusGetLastErrorCode();

		if ( !bSendOk && bAutoRegister && iDataID > 0 ) {
			(void)XS_BusDataRemove(iDataID);
			iDataID = 0;
		} else if ( bSendOk && bAutoRegister && !bPersist && iDataID > 0 ) {
			(void)XS_BusDataRelease(iDataID);
		}

		if ( bSendOk ) {
			snprintf(
				sBody,
				sizeof(sBody),
				"{\"result\":true,\"message\":\"message queued\",\"data_id\":%lld,\"target\":\"%s\",\"topic\":\"%s\",\"server\":\"%s\",\"host\":\"%s\",\"auto_register\":%s,\"persist\":%s,\"ttl\":%lld}",
				(long long)iDataID,
				pTarget,
				sTopic,
				(strcmp(pTarget, "broadcast") == 0) ? "" : (pServer ? pServer : ""),
				(strcmp(pTarget, "host") == 0) ? (pHost ? pHost : "") : "",
				bAutoRegister ? "true" : "false",
				bPersist ? "true" : "false",
				(long long)((iTTL >= 0) ? iTTL : 0)
			);
			xrtHttpdResponseSetStatus(pResp, 200, "OK");
		} else {
			snprintf(
				sBody,
				sizeof(sBody),
				"{\"result\":false,\"message\":\"send failed\",\"bus_code\":%d,\"bus_error\":\"%s\",\"data_id\":%lld,\"target\":\"%s\",\"topic\":\"%s\",\"server\":\"%s\",\"host\":\"%s\",\"auto_register\":%s,\"persist\":%s,\"ttl\":%lld}",
				(int)iBusCode,
				XS_BusGetLastError(),
				(long long)iDataID,
				pTarget,
				sTopic,
				(strcmp(pTarget, "broadcast") == 0) ? "" : (pServer ? pServer : ""),
				(strcmp(pTarget, "host") == 0) ? (pHost ? pHost : "") : "",
				bAutoRegister ? "true" : "false",
				bPersist ? "true" : "false",
				(long long)((iTTL >= 0) ? iTTL : 0)
			);
			xrtHttpdResponseSetStatus(pResp, XS_HttpBusErrorStatusCode(iBusCode), XS_HttpBusErrorStatusText(iBusCode));
		}
	}

ExitBusSend:
	if ( sTarget ) {
		xrtFree(sTarget);
	}
	if ( sTopic ) {
		xrtFree(sTopic);
	}
	if ( sServerName ) {
		xrtFree(sServerName);
	}
	if ( sHostName ) {
		xrtFree(sHostName);
	}
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sDataID ) {
		xrtFree(sDataID);
	}
	if ( sText ) {
		xrtFree(sText);
	}
	if ( sJson ) {
		xrtFree(sJson);
	}
	if ( sTTL ) {
		xrtFree(sTTL);
	}
	return xrtHttpdResponseSetBodyCopy(pResp, sBody, strlen(sBody), "application/json; charset=utf-8");
}

static inline bool XS_HttpHandleStatus(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[24576];
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sHttpLastIdleCloseTime;
	char* sHttpLastConnLimitCloseTime;
	char* sHttpLastRejectTime;
	char* sHttpLastStopCleanupTime;
	char* sHttpLastHeaderLimitRejectTime;
	char* sHttpLastBodyLimitRejectTime;
	char* sHttpLastPathLimitRejectTime;
	char* sHttpLastApiDisabledRejectTime;
	char* sHttpLastMethodRejectTime;
	char* sHttpLastHostNotFoundRejectTime;
	char* sHttpLastReloadBusyRejectTime;
	char* sHttpLastReloadFailedRejectTime;
	char* sHttpLastCheckConfigFailedRejectTime;
	char* sHttpLastBusBadRequestRejectTime;
	char* sHttpLastBusNotFoundRejectTime;
	char* sHttpLastBusLimitRejectTime;
	char* sHttpLastBusFailedRejectTime;
	char* sWsLastTime;
	char* sWsLastCloseTime;
	char* sWsLastErrorTime;
	char* sWsLastInvalidTime;
	char* sWsLastIdleCloseTime;
	char* sWsLastConnLimitCloseTime;
	char* sWsLastMessageLimitCloseTime;
	char* sXtpLastTime;
	char* sXtpLastInvalidTime;
	char* sXtpLastErrorTime;
	char* sXtpLastIdleCloseTime;
	char* sXtpLastConnLimitCloseTime;
	char* sXtpLastRecvLimitCloseTime;
	char* sUdpLastTime;
	char* sUdpLastErrorTime;
	char* sCustomLastTime;
	char* sCustomLastCloseTime;
	char* sCustomLastErrorTime;
	char* sCustomLastInvalidTime;
	char* sCustomLastIdleCloseTime;
	char* sCustomLastConnLimitCloseTime;
	char* sCustomLastRecvLimitCloseTime;
	char* sWsLastRejectTime;
	char* sWsLastStopCleanupTime;
	char* sXtpLastRejectTime;
	char* sXtpLastStopCleanupTime;
	char* sCustomLastRejectTime;
	char* sCustomLastStopCleanupTime;
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

	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sHttpLastIdleCloseTime = XS_HttpLastIdleCloseTimeText();
	sHttpLastConnLimitCloseTime = XS_HttpLastConnLimitCloseTimeText();
	sHttpLastRejectTime = XS_HttpLastRejectTimeText();
	sHttpLastStopCleanupTime = XS_HttpLastStopCleanupTimeText();
	sHttpLastHeaderLimitRejectTime = XS_HttpLastHeaderLimitRejectTimeText();
	sHttpLastBodyLimitRejectTime = XS_HttpLastBodyLimitRejectTimeText();
	sHttpLastPathLimitRejectTime = XS_HttpLastPathLimitRejectTimeText();
	sHttpLastApiDisabledRejectTime = XS_HttpLastApiDisabledRejectTimeText();
	sHttpLastMethodRejectTime = XS_HttpLastMethodRejectTimeText();
	sHttpLastHostNotFoundRejectTime = XS_HttpLastHostNotFoundRejectTimeText();
	sHttpLastReloadBusyRejectTime = XS_HttpLastReloadBusyRejectTimeText();
	sHttpLastReloadFailedRejectTime = XS_HttpLastReloadFailedRejectTimeText();
	sHttpLastCheckConfigFailedRejectTime = XS_HttpLastCheckConfigFailedRejectTimeText();
	sHttpLastBusBadRequestRejectTime = XS_HttpLastBusBadRequestRejectTimeText();
	sHttpLastBusNotFoundRejectTime = XS_HttpLastBusNotFoundRejectTimeText();
	sHttpLastBusLimitRejectTime = XS_HttpLastBusLimitRejectTimeText();
	sHttpLastBusFailedRejectTime = XS_HttpLastBusFailedRejectTimeText();
	sWsLastTime = XS_WsLastTimeText();
	sWsLastCloseTime = XS_WsLastCloseTimeText();
	sWsLastErrorTime = XS_WsLastErrorTimeText();
	sWsLastInvalidTime = XS_WsLastInvalidTimeText();
	sWsLastIdleCloseTime = XS_WsLastIdleCloseTimeText();
	sWsLastConnLimitCloseTime = XS_WsLastConnLimitCloseTimeText();
	sWsLastMessageLimitCloseTime = XS_WsLastMessageLimitCloseTimeText();
	sXtpLastTime = XS_XtpLastTimeText();
	sXtpLastInvalidTime = XS_XtpLastInvalidTimeText();
	sXtpLastErrorTime = XS_XtpLastErrorTimeText();
	sXtpLastIdleCloseTime = XS_XtpLastIdleCloseTimeText();
	sXtpLastConnLimitCloseTime = XS_XtpLastConnLimitCloseTimeText();
	sXtpLastRecvLimitCloseTime = XS_XtpLastRecvLimitCloseTimeText();
	sUdpLastTime = XS_UdpLastTimeText();
	sUdpLastErrorTime = XS_UdpLastErrorTimeText();
	sCustomLastTime = XS_CustomLastTimeText();
	sCustomLastCloseTime = XS_CustomLastCloseTimeText();
	sCustomLastErrorTime = XS_CustomLastErrorTimeText();
	sCustomLastInvalidTime = XS_CustomLastInvalidTimeText();
	sCustomLastIdleCloseTime = XS_CustomLastIdleCloseTimeText();
	sCustomLastConnLimitCloseTime = XS_CustomLastConnLimitCloseTimeText();
	sCustomLastRecvLimitCloseTime = XS_CustomLastRecvLimitCloseTimeText();
	sWsLastRejectTime = XS_WsLastRejectTimeText();
	sWsLastStopCleanupTime = XS_WsLastStopCleanupTimeText();
	sXtpLastRejectTime = XS_XtpLastRejectTimeText();
	sXtpLastStopCleanupTime = XS_XtpLastStopCleanupTimeText();
	sCustomLastRejectTime = XS_CustomLastRejectTimeText();
	sCustomLastStopCleanupTime = XS_CustomLastStopCleanupTimeText();
	iLen = (size_t)snprintf(
		sBody,
		sizeof(sBody),
		"server=%s\nclass=%s\naddr=%s\ndebug=%s\nhost_aware=%s\ndefault_host=%s\npath_limit=%u\nheader_limit=%u\nbody_limit=%u\nrecv_limit=%u\nidle_timeout=%u\nconn_limit=%u\nws_message_limit=%u\nbacklog=%u\nhttp_req_count=%lld\nhttp_manage_req_count=%lld\nhttp_app_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_last_method=%s\nhttp_last_status=%lld\nhttp_last_path=%s\nhttp_last_target=%s\nhttp_last_version=%s\nhttp_last_remote=%s\nhttp_last_host=%s\nhttp_last_user_agent=%s\nhttp_last_referer=%s\nhttp_last_origin=%s\nhttp_last_accept=%s\nhttp_last_accept_encoding=%s\nhttp_last_cookie=%s\nhttp_last_forwarded_for=%s\nhttp_last_real_ip=%s\nhttp_last_connection=%s\nhttp_last_cache_control=%s\nhttp_last_content_type=%s\nhttp_last_header_count=%lld\nhttp_last_query_len=%lld\nhttp_last_body_len=%lld\nhttp_last_time=%s\nhttp_last_age_ms=%lld\nhttp_last_duration_ms=%lld\nhttp_last_app_method=%s\nhttp_last_app_status=%lld\nhttp_last_app_path=%s\nhttp_last_app_target=%s\nhttp_last_app_version=%s\nhttp_last_app_remote=%s\nhttp_last_app_host=%s\nhttp_last_app_user_agent=%s\nhttp_last_app_referer=%s\nhttp_last_app_origin=%s\nhttp_last_app_accept=%s\nhttp_last_app_accept_encoding=%s\nhttp_last_app_cookie=%s\nhttp_last_app_forwarded_for=%s\nhttp_last_app_real_ip=%s\nhttp_last_app_connection=%s\nhttp_last_app_cache_control=%s\nhttp_last_app_content_type=%s\nhttp_last_app_header_count=%lld\nhttp_last_app_query_len=%lld\nhttp_last_app_body_len=%lld\nhttp_last_app_time=%s\nhttp_last_app_age_ms=%lld\nhttp_last_app_duration_ms=%lld\n",
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
		(unsigned int)objServer->IdleTimeout,
		(unsigned int)objServer->ConnLimit,
		(unsigned int)objServer->WsMessageLimit,
		(unsigned int)objServer->Backlog,
		(long long)XS_HttpMetricGet(&g_iXsHttpReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpManageReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpAppReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp2xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp3xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp4xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp5xxCount),
		XS_HttpLastMethodName()[0] ? XS_HttpLastMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastStatusCode),
		XS_HttpLastPath()[0] ? XS_HttpLastPath() : "(none)",
		XS_HttpLastTarget()[0] ? XS_HttpLastTarget() : "(none)",
		g_sXsHttpLastVersion[0] ? g_sXsHttpLastVersion : "(none)",
		XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : "(none)",
		g_sXsHttpLastHost[0] ? g_sXsHttpLastHost : "(none)",
		g_sXsHttpLastUserAgent[0] ? g_sXsHttpLastUserAgent : "(none)",
		g_sXsHttpLastReferer[0] ? g_sXsHttpLastReferer : "(none)",
		g_sXsHttpLastOrigin[0] ? g_sXsHttpLastOrigin : "(none)",
		g_sXsHttpLastAccept[0] ? g_sXsHttpLastAccept : "(none)",
		g_sXsHttpLastAcceptEncoding[0] ? g_sXsHttpLastAcceptEncoding : "(none)",
		g_sXsHttpLastCookie[0] ? g_sXsHttpLastCookie : "(none)",
		g_sXsHttpLastForwardedFor[0] ? g_sXsHttpLastForwardedFor : "(none)",
		g_sXsHttpLastRealIP[0] ? g_sXsHttpLastRealIP : "(none)",
		g_sXsHttpLastConnection[0] ? g_sXsHttpLastConnection : "(none)",
		g_sXsHttpLastCacheControl[0] ? g_sXsHttpLastCacheControl : "(none)",
		g_sXsHttpLastContentType[0] ? g_sXsHttpLastContentType : "(none)",
		(long long)g_iXsHttpLastHeaderCount,
		(long long)g_iXsHttpLastQueryLen,
		(long long)g_iXsHttpLastBodyLen,
		sHttpLastTime ? sHttpLastTime : "(none)",
		(long long)XS_HttpLastRequestAgeMS(),
		(long long)g_iXsHttpLastTimeMS,
		XS_HttpLastAppMethodName()[0] ? XS_HttpLastAppMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode),
		XS_HttpLastAppPath()[0] ? XS_HttpLastAppPath() : "(none)",
		XS_HttpLastAppTarget()[0] ? XS_HttpLastAppTarget() : "(none)",
		g_sXsHttpLastAppVersion[0] ? g_sXsHttpLastAppVersion : "(none)",
		XS_HttpLastAppRemote()[0] ? XS_HttpLastAppRemote() : "(none)",
		g_sXsHttpLastAppHost[0] ? g_sXsHttpLastAppHost : "(none)",
		g_sXsHttpLastAppUserAgent[0] ? g_sXsHttpLastAppUserAgent : "(none)",
		g_sXsHttpLastAppReferer[0] ? g_sXsHttpLastAppReferer : "(none)",
		g_sXsHttpLastAppOrigin[0] ? g_sXsHttpLastAppOrigin : "(none)",
		g_sXsHttpLastAppAccept[0] ? g_sXsHttpLastAppAccept : "(none)",
		g_sXsHttpLastAppAcceptEncoding[0] ? g_sXsHttpLastAppAcceptEncoding : "(none)",
		g_sXsHttpLastAppCookie[0] ? g_sXsHttpLastAppCookie : "(none)",
		g_sXsHttpLastAppForwardedFor[0] ? g_sXsHttpLastAppForwardedFor : "(none)",
		g_sXsHttpLastAppRealIP[0] ? g_sXsHttpLastAppRealIP : "(none)",
		g_sXsHttpLastAppConnection[0] ? g_sXsHttpLastAppConnection : "(none)",
		g_sXsHttpLastAppCacheControl[0] ? g_sXsHttpLastAppCacheControl : "(none)",
		g_sXsHttpLastAppContentType[0] ? g_sXsHttpLastAppContentType : "(none)",
		(long long)g_iXsHttpLastAppHeaderCount,
		(long long)g_iXsHttpLastAppQueryLen,
		(long long)g_iXsHttpLastAppBodyLen,
		sHttpLastAppTime ? sHttpLastAppTime : "(none)",
		(long long)XS_HttpLastAppRequestAgeMS(),
		(long long)g_iXsHttpLastAppTimeMS
	);

	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"bind_ip=%s\nbind_port=%u\ntls=%s\nbind_ip_tls=%s\nbind_port_tls=%u\naddr_tls=%s\nws_protocol=%s\ntls_cert_file=%s\ntls_key_file=%s\ntls_ca_file=%s\ncompiler=%s\nmanage_api=%s\nmem_debug=%s\nruntime_server_count=%u\nhost_count=%u\n",
			objServer->BindIP ? objServer->BindIP : "(null)",
			(unsigned int)objServer->BindPort,
			objServer->EnableTLS ? "true" : "false",
			objServer->BindIPTLS ? objServer->BindIPTLS : "(null)",
			(unsigned int)objServer->BindPortTLS,
			objServer->AddrTLS ? objServer->AddrTLS : "(null)",
			objServer->WsProtocol ? objServer->WsProtocol : "(none)",
			XS_TlsConfigFileText(objServer->TlsConfig.sCertFile),
			XS_TlsConfigFileText(objServer->TlsConfig.sKeyFile),
			XS_TlsConfigFileText(objServer->TlsConfig.sCaFile),
			XS_CompilerName(),
			XS_ManageAPIEnabled(objServer, objHost) ? "true" : "false",
			XS_MemDebugEnabled() ? "true" : "false",
			(unsigned int)g_iXsRuntimeServerCount,
			(unsigned int)(objServer->Hosts ? objServer->Hosts->Count : 0)
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"http_time_total_ms=%lld\nhttp_time_max_ms=%lld\nhttp_time_avg_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpTimeTotalMS),
			(long long)XS_HttpMetricGet(&g_iXsHttpTimeMaxMS),
			(long long)((XS_HttpMetricGet(&g_iXsHttpReqCount) > 0) ? (XS_HttpMetricGet(&g_iXsHttpTimeTotalMS) / XS_HttpMetricGet(&g_iXsHttpReqCount)) : 0)
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"http_conn_current=%lld\nhttp_conn_peak=%lld\nhttp_idle_close_count=%lld\nhttp_conn_limit_close_count=%lld\nhttp_last_idle_close_time=%s\nhttp_last_idle_close_age_ms=%lld\nhttp_last_conn_limit_close_time=%s\nhttp_last_conn_limit_close_age_ms=%lld\nhttp_get_count=%lld\nhttp_post_count=%lld\nhttp_head_count=%lld\nhttp_other_count=%lld\nws_conn_current=%lld\nws_conn_peak=%lld\nws_open_count=%lld\nws_close_count=%lld\nws_text_count=%lld\nws_binary_count=%lld\nws_ping_count=%lld\nws_pong_count=%lld\nws_error_count=%lld\nws_invalid_count=%lld\nws_idle_close_count=%lld\nws_conn_limit_close_count=%lld\nws_last_invalid_reason=%s\nws_last_invalid_time=%s\nws_last_invalid_age_ms=%lld\nws_last_idle_close_time=%s\nws_last_idle_close_age_ms=%lld\nws_last_conn_limit_close_time=%s\nws_last_conn_limit_close_age_ms=%lld\nws_last_frame_type=%s\nws_last_remote=%s\nws_last_bytes=%lld\nws_last_text=%s\nws_last_time=%s\nws_last_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpConnCurrent),
			(long long)XS_HttpMetricGet(&g_iXsHttpConnPeak),
			(long long)XS_HttpMetricGet(&g_iXsHttpIdleCloseCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpConnLimitCloseCount),
			sHttpLastIdleCloseTime ? sHttpLastIdleCloseTime : "(none)",
			(long long)XS_HttpLastIdleCloseAgeMS(),
			sHttpLastConnLimitCloseTime ? sHttpLastConnLimitCloseTime : "(none)",
			(long long)XS_HttpLastConnLimitCloseAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodGetCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodPostCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodHeadCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodOtherCount),
			(long long)XS_HttpMetricGet(&g_iXsWsConnCurrent),
			(long long)XS_HttpMetricGet(&g_iXsWsConnPeak),
			(long long)XS_HttpMetricGet(&g_iXsWsOpenCount),
			(long long)XS_HttpMetricGet(&g_iXsWsCloseCount),
			(long long)XS_HttpMetricGet(&g_iXsWsTextCount),
			(long long)XS_HttpMetricGet(&g_iXsWsBinaryCount),
			(long long)XS_HttpMetricGet(&g_iXsWsPingCount),
			(long long)XS_HttpMetricGet(&g_iXsWsPongCount),
			(long long)XS_HttpMetricGet(&g_iXsWsErrorCount),
			(long long)XS_HttpMetricGet(&g_iXsWsInvalidCount),
			(long long)XS_HttpMetricGet(&g_iXsWsIdleCloseCount),
			(long long)XS_HttpMetricGet(&g_iXsWsConnLimitCloseCount),
			g_sXsWsLastInvalidReason[0] ? g_sXsWsLastInvalidReason : "(none)",
			sWsLastInvalidTime ? sWsLastInvalidTime : "(none)",
			(long long)XS_WsLastInvalidAgeMS(),
			sWsLastIdleCloseTime ? sWsLastIdleCloseTime : "(none)",
			(long long)XS_WsLastIdleCloseAgeMS(),
			sWsLastConnLimitCloseTime ? sWsLastConnLimitCloseTime : "(none)",
			(long long)XS_WsLastConnLimitCloseAgeMS(),
			XS_WsLastFrameTypeName()[0] ? XS_WsLastFrameTypeName() : "(none)",
			g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)",
			(long long)XS_HttpMetricGet(&g_iXsWsLastBytes),
			g_sXsWsLastText[0] ? g_sXsWsLastText : "(none)",
			sWsLastTime ? sWsLastTime : "(none)",
			(long long)XS_WsLastAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"ws_last_close_reason=%lld\nws_last_close_time=%s\nws_last_close_age_ms=%lld\nws_last_error_code=%lld\nws_last_error_time=%s\nws_last_error_age_ms=%lld\n",
			(long long)g_iXsWsLastCloseReason,
			sWsLastCloseTime ? sWsLastCloseTime : "(none)",
			(long long)XS_WsLastCloseAgeMS(),
			(long long)g_iXsWsLastErrorCode,
			sWsLastErrorTime ? sWsLastErrorTime : "(none)",
			(long long)XS_WsLastErrorAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"http_reject_count=%lld\nhttp_last_reject_status=%lld\nhttp_last_reject_reason=%s\nhttp_last_reject_time=%s\nhttp_last_reject_age_ms=%lld\nws_reject_count=%lld\nws_last_reject_reason=%s\nws_last_reject_time=%s\nws_last_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastRejectStatus),
			g_sXsHttpLastRejectReason[0] ? g_sXsHttpLastRejectReason : "(none)",
			sHttpLastRejectTime ? sHttpLastRejectTime : "(none)",
			(long long)XS_HttpLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsWsRejectCount),
			g_sXsWsLastRejectReason[0] ? g_sXsWsLastRejectReason : "(none)",
			sWsLastRejectTime ? sWsLastRejectTime : "(none)",
			(long long)XS_WsLastRejectAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"http_header_limit_reject_count=%lld\nhttp_last_header_limit_reject_time=%s\nhttp_last_header_limit_reject_age_ms=%lld\nhttp_body_limit_reject_count=%lld\nhttp_last_body_limit_reject_time=%s\nhttp_last_body_limit_reject_age_ms=%lld\nhttp_path_limit_reject_count=%lld\nhttp_last_path_limit_reject_time=%s\nhttp_last_path_limit_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpHeaderLimitRejectCount),
			sHttpLastHeaderLimitRejectTime ? sHttpLastHeaderLimitRejectTime : "(none)",
			(long long)XS_HttpLastHeaderLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBodyLimitRejectCount),
			sHttpLastBodyLimitRejectTime ? sHttpLastBodyLimitRejectTime : "(none)",
			(long long)XS_HttpLastBodyLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpPathLimitRejectCount),
			sHttpLastPathLimitRejectTime ? sHttpLastPathLimitRejectTime : "(none)",
			(long long)XS_HttpLastPathLimitRejectAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"http_api_disabled_reject_count=%lld\nhttp_last_api_disabled_reject_time=%s\nhttp_last_api_disabled_reject_age_ms=%lld\nhttp_method_reject_count=%lld\nhttp_last_method_reject_time=%s\nhttp_last_method_reject_age_ms=%lld\nhttp_host_not_found_reject_count=%lld\nhttp_last_host_not_found_reject_time=%s\nhttp_last_host_not_found_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpApiDisabledRejectCount),
			sHttpLastApiDisabledRejectTime ? sHttpLastApiDisabledRejectTime : "(none)",
			(long long)XS_HttpLastApiDisabledRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodRejectCount),
			sHttpLastMethodRejectTime ? sHttpLastMethodRejectTime : "(none)",
			(long long)XS_HttpLastMethodRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpHostNotFoundRejectCount),
			sHttpLastHostNotFoundRejectTime ? sHttpLastHostNotFoundRejectTime : "(none)",
			(long long)XS_HttpLastHostNotFoundRejectAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"http_reload_busy_reject_count=%lld\nhttp_last_reload_busy_reject_time=%s\nhttp_last_reload_busy_reject_age_ms=%lld\nhttp_reload_failed_reject_count=%lld\nhttp_last_reload_failed_reject_time=%s\nhttp_last_reload_failed_reject_age_ms=%lld\nhttp_check_config_failed_reject_count=%lld\nhttp_last_check_config_failed_reject_time=%s\nhttp_last_check_config_failed_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpReloadBusyRejectCount),
			sHttpLastReloadBusyRejectTime ? sHttpLastReloadBusyRejectTime : "(none)",
			(long long)XS_HttpLastReloadBusyRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpReloadFailedRejectCount),
			sHttpLastReloadFailedRejectTime ? sHttpLastReloadFailedRejectTime : "(none)",
			(long long)XS_HttpLastReloadFailedRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpCheckConfigFailedRejectCount),
			sHttpLastCheckConfigFailedRejectTime ? sHttpLastCheckConfigFailedRejectTime : "(none)",
			(long long)XS_HttpLastCheckConfigFailedRejectAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"http_bus_bad_request_reject_count=%lld\nhttp_last_bus_bad_request_reject_time=%s\nhttp_last_bus_bad_request_reject_age_ms=%lld\nhttp_bus_not_found_reject_count=%lld\nhttp_last_bus_not_found_reject_time=%s\nhttp_last_bus_not_found_reject_age_ms=%lld\nhttp_bus_limit_reject_count=%lld\nhttp_last_bus_limit_reject_time=%s\nhttp_last_bus_limit_reject_age_ms=%lld\nhttp_bus_failed_reject_count=%lld\nhttp_last_bus_failed_reject_time=%s\nhttp_last_bus_failed_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpBusBadRequestRejectCount),
			sHttpLastBusBadRequestRejectTime ? sHttpLastBusBadRequestRejectTime : "(none)",
			(long long)XS_HttpLastBusBadRequestRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusNotFoundRejectCount),
			sHttpLastBusNotFoundRejectTime ? sHttpLastBusNotFoundRejectTime : "(none)",
			(long long)XS_HttpLastBusNotFoundRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusLimitRejectCount),
			sHttpLastBusLimitRejectTime ? sHttpLastBusLimitRejectTime : "(none)",
			(long long)XS_HttpLastBusLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusFailedRejectCount),
			sHttpLastBusFailedRejectTime ? sHttpLastBusFailedRejectTime : "(none)",
			(long long)XS_HttpLastBusFailedRejectAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"http_stop_cleanup_count=%lld\nhttp_last_stop_cleanup_closed=%lld\nhttp_last_stop_cleanup_remain=%lld\nhttp_last_stop_cleanup_time=%s\nhttp_last_stop_cleanup_age_ms=%lld\nws_stop_cleanup_count=%lld\nws_last_stop_cleanup_closed=%lld\nws_last_stop_cleanup_remain=%lld\nws_last_stop_cleanup_time=%s\nws_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastStopCleanupRemain),
			sHttpLastStopCleanupTime ? sHttpLastStopCleanupTime : "(none)",
			(long long)XS_HttpLastStopCleanupAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsWsStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsWsLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsWsLastStopCleanupRemain),
			sWsLastStopCleanupTime ? sWsLastStopCleanupTime : "(none)",
			(long long)XS_WsLastStopCleanupAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"xtp_reject_count=%lld\nxtp_last_reject_reason=%s\nxtp_last_reject_time=%s\nxtp_last_reject_age_ms=%lld\nxtp_stop_cleanup_count=%lld\nxtp_last_stop_cleanup_closed=%lld\nxtp_last_stop_cleanup_remain=%lld\nxtp_last_stop_cleanup_time=%s\nxtp_last_stop_cleanup_age_ms=%lld\ncustom_reject_count=%lld\ncustom_last_reject_reason=%s\ncustom_last_reject_time=%s\ncustom_last_reject_age_ms=%lld\ncustom_stop_cleanup_count=%lld\ncustom_last_stop_cleanup_closed=%lld\ncustom_last_stop_cleanup_remain=%lld\ncustom_last_stop_cleanup_time=%s\ncustom_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsXtpRejectCount),
			g_sXsXtpLastRejectReason[0] ? g_sXsXtpLastRejectReason : "(none)",
			sXtpLastRejectTime ? sXtpLastRejectTime : "(none)",
			(long long)XS_XtpLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastStopCleanupRemain),
			sXtpLastStopCleanupTime ? sXtpLastStopCleanupTime : "(none)",
			(long long)XS_XtpLastStopCleanupAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomRejectCount),
			g_sXsCustomLastRejectReason[0] ? g_sXsCustomLastRejectReason : "(none)",
			sCustomLastRejectTime ? sCustomLastRejectTime : "(none)",
			(long long)XS_CustomLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsCustomLastStopCleanupRemain),
			sCustomLastStopCleanupTime ? sCustomLastStopCleanupTime : "(none)",
			(long long)XS_CustomLastStopCleanupAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"ws_message_limit_close_count=%lld\nws_last_message_limit_close_time=%s\nws_last_message_limit_close_age_ms=%lld\nxtp_recv_limit_close_count=%lld\nxtp_last_recv_limit_close_time=%s\nxtp_last_recv_limit_close_age_ms=%lld\ncustom_recv_limit_close_count=%lld\ncustom_last_recv_limit_close_time=%s\ncustom_last_recv_limit_close_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsWsMessageLimitCloseCount),
			sWsLastMessageLimitCloseTime ? sWsLastMessageLimitCloseTime : "(none)",
			(long long)XS_WsLastMessageLimitCloseAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpRecvLimitCloseCount),
			sXtpLastRecvLimitCloseTime ? sXtpLastRecvLimitCloseTime : "(none)",
			(long long)XS_XtpLastRecvLimitCloseAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomRecvLimitCloseCount),
			sCustomLastRecvLimitCloseTime ? sCustomLastRecvLimitCloseTime : "(none)",
			(long long)XS_CustomLastRecvLimitCloseAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"xtp_conn_current=%lld\nxtp_conn_peak=%lld\nxtp_open_count=%lld\nxtp_close_count=%lld\nxtp_error_count=%lld\nxtp_msg_count=%lld\nxtp_req_count=%lld\nxtp_resp_count=%lld\nxtp_push_count=%lld\nxtp_event_count=%lld\nxtp_send_count=%lld\nxtp_recv_bytes=%lld\nxtp_send_bytes=%lld\nxtp_idle_close_count=%lld\nxtp_conn_limit_close_count=%lld\nxtp_last_msg_type=%s\nxtp_last_status=%lld\nxtp_last_msg_id=%lld\nxtp_last_flags=%lld\nxtp_last_param_count=%lld\nxtp_last_body_size=%lld\nxtp_last_remote=%s\nxtp_last_bytes=%lld\nxtp_last_cmd=%s\nxtp_last_time=%s\nxtp_last_age_ms=%lld\nxtp_last_invalid_reason=%s\nxtp_last_invalid_time=%s\nxtp_last_invalid_age_ms=%lld\nxtp_last_error_code=%lld\nxtp_last_error_time=%s\nxtp_last_error_age_ms=%lld\nxtp_last_idle_close_time=%s\nxtp_last_idle_close_age_ms=%lld\nxtp_last_conn_limit_close_time=%s\nxtp_last_conn_limit_close_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsXtpConnCurrent),
			(long long)XS_HttpMetricGet(&g_iXsXtpConnPeak),
			(long long)XS_HttpMetricGet(&g_iXsXtpOpenCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpCloseCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpErrorCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpMsgCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpReqCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpRespCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpPushCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpEventCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpSendCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpRecvBytes),
			(long long)XS_HttpMetricGet(&g_iXsXtpSendBytes),
			(long long)XS_HttpMetricGet(&g_iXsXtpIdleCloseCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpConnLimitCloseCount),
			XS_XtpLastMsgTypeName()[0] ? XS_XtpLastMsgTypeName() : "(none)",
			(long long)XS_HttpMetricGet(&g_iXsXtpLastStatus),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastMsgID),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastFlags),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastParamCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastBodySize),
			g_sXsXtpLastRemote[0] ? g_sXsXtpLastRemote : "(none)",
			(long long)g_iXsXtpLastBytes,
			g_sXsXtpLastCmd[0] ? g_sXsXtpLastCmd : "(none)",
			sXtpLastTime ? sXtpLastTime : "(none)",
			(long long)XS_XtpLastAgeMS(),
			g_sXsXtpLastInvalidReason[0] ? g_sXsXtpLastInvalidReason : "(none)",
			sXtpLastInvalidTime ? sXtpLastInvalidTime : "(none)",
			(long long)XS_XtpLastInvalidAgeMS(),
			(long long)g_iXsXtpLastErrorCode,
			sXtpLastErrorTime ? sXtpLastErrorTime : "(none)",
			(long long)XS_XtpLastErrorAgeMS(),
			sXtpLastIdleCloseTime ? sXtpLastIdleCloseTime : "(none)",
			(long long)XS_XtpLastIdleCloseAgeMS(),
			sXtpLastConnLimitCloseTime ? sXtpLastConnLimitCloseTime : "(none)",
			(long long)XS_XtpLastConnLimitCloseAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"udp_recv_count=%lld\nudp_send_count=%lld\nudp_error_count=%lld\nudp_recv_bytes=%lld\nudp_send_bytes=%lld\nudp_last_from=%s\nudp_last_text=%s\nudp_last_bytes=%lld\nudp_last_time=%s\nudp_last_age_ms=%lld\nudp_last_error_code=%lld\nudp_last_error_time=%s\nudp_last_error_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsUdpRecvCount),
			(long long)XS_HttpMetricGet(&g_iXsUdpSendCount),
			(long long)XS_HttpMetricGet(&g_iXsUdpErrorCount),
			(long long)XS_HttpMetricGet(&g_iXsUdpRecvBytes),
			(long long)XS_HttpMetricGet(&g_iXsUdpSendBytes),
			g_sXsUdpLastFrom[0] ? g_sXsUdpLastFrom : "(none)",
			g_sXsUdpLastText[0] ? g_sXsUdpLastText : "(none)",
			(long long)g_iXsUdpLastBytes,
			sUdpLastTime ? sUdpLastTime : "(none)",
			(long long)XS_UdpLastAgeMS(),
			(long long)g_iXsUdpLastErrorCode,
			sUdpLastErrorTime ? sUdpLastErrorTime : "(none)",
			(long long)XS_UdpLastErrorAgeMS()
		);
	}
	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"custom_conn_current=%lld\ncustom_conn_peak=%lld\ncustom_open_count=%lld\ncustom_close_count=%lld\ncustom_error_count=%lld\ncustom_invalid_count=%lld\ncustom_recv_count=%lld\ncustom_send_count=%lld\ncustom_recv_bytes=%lld\ncustom_send_bytes=%lld\ncustom_idle_close_count=%lld\ncustom_conn_limit_close_count=%lld\ncustom_last_invalid_reason=%s\ncustom_last_invalid_time=%s\ncustom_last_invalid_age_ms=%lld\ncustom_last_close_reason=%lld\ncustom_last_close_time=%s\ncustom_last_close_age_ms=%lld\ncustom_last_error_code=%lld\ncustom_last_error_time=%s\ncustom_last_error_age_ms=%lld\ncustom_last_remote=%s\ncustom_last_bytes=%lld\ncustom_last_text=%s\ncustom_last_time=%s\ncustom_last_age_ms=%lld\ncustom_last_idle_close_time=%s\ncustom_last_idle_close_age_ms=%lld\ncustom_last_conn_limit_close_time=%s\ncustom_last_conn_limit_close_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsCustomConnCurrent),
			(long long)XS_HttpMetricGet(&g_iXsCustomConnPeak),
			(long long)XS_HttpMetricGet(&g_iXsCustomOpenCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomCloseCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomErrorCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomInvalidCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomRecvCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomSendCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomRecvBytes),
			(long long)XS_HttpMetricGet(&g_iXsCustomSendBytes),
			(long long)XS_HttpMetricGet(&g_iXsCustomIdleCloseCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomConnLimitCloseCount),
			g_sXsCustomLastInvalidReason[0] ? g_sXsCustomLastInvalidReason : "(none)",
			sCustomLastInvalidTime ? sCustomLastInvalidTime : "(none)",
			(long long)XS_CustomLastInvalidAgeMS(),
			(long long)g_iXsCustomLastCloseReason,
			sCustomLastCloseTime ? sCustomLastCloseTime : "(none)",
			(long long)XS_CustomLastCloseAgeMS(),
			(long long)g_iXsCustomLastErrorCode,
			sCustomLastErrorTime ? sCustomLastErrorTime : "(none)",
			(long long)XS_CustomLastErrorAgeMS(),
			g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)",
			(long long)g_iXsCustomLastBytes,
			g_sXsCustomLastText[0] ? g_sXsCustomLastText : "(none)",
			sCustomLastTime ? sCustomLastTime : "(none)",
			(long long)XS_CustomLastAgeMS(),
			sCustomLastIdleCloseTime ? sCustomLastIdleCloseTime : "(none)",
			(long long)XS_CustomLastIdleCloseAgeMS(),
			sCustomLastConnLimitCloseTime ? sCustomLastConnLimitCloseTime : "(none)",
			(long long)XS_CustomLastConnLimitCloseAgeMS()
		);
	}
	
	if ( objServer->EnableDefaultHost && iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"default=true\n"
		);
	}
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

	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
	}
	if ( sHttpLastIdleCloseTime ) {
		xrtFree(sHttpLastIdleCloseTime);
	}
	if ( sHttpLastConnLimitCloseTime ) {
		xrtFree(sHttpLastConnLimitCloseTime);
	}
	if ( sHttpLastRejectTime ) {
		xrtFree(sHttpLastRejectTime);
	}
	if ( sHttpLastStopCleanupTime ) {
		xrtFree(sHttpLastStopCleanupTime);
	}
	if ( sHttpLastHeaderLimitRejectTime ) {
		xrtFree(sHttpLastHeaderLimitRejectTime);
	}
	if ( sHttpLastBodyLimitRejectTime ) {
		xrtFree(sHttpLastBodyLimitRejectTime);
	}
	if ( sHttpLastPathLimitRejectTime ) {
		xrtFree(sHttpLastPathLimitRejectTime);
	}
	if ( sHttpLastApiDisabledRejectTime ) {
		xrtFree(sHttpLastApiDisabledRejectTime);
	}
	if ( sHttpLastMethodRejectTime ) {
		xrtFree(sHttpLastMethodRejectTime);
	}
	if ( sHttpLastHostNotFoundRejectTime ) {
		xrtFree(sHttpLastHostNotFoundRejectTime);
	}
	if ( sHttpLastReloadBusyRejectTime ) {
		xrtFree(sHttpLastReloadBusyRejectTime);
	}
	if ( sHttpLastReloadFailedRejectTime ) {
		xrtFree(sHttpLastReloadFailedRejectTime);
	}
	if ( sHttpLastCheckConfigFailedRejectTime ) {
		xrtFree(sHttpLastCheckConfigFailedRejectTime);
	}
	if ( sHttpLastBusBadRequestRejectTime ) {
		xrtFree(sHttpLastBusBadRequestRejectTime);
	}
	if ( sHttpLastBusNotFoundRejectTime ) {
		xrtFree(sHttpLastBusNotFoundRejectTime);
	}
	if ( sHttpLastBusLimitRejectTime ) {
		xrtFree(sHttpLastBusLimitRejectTime);
	}
	if ( sHttpLastBusFailedRejectTime ) {
		xrtFree(sHttpLastBusFailedRejectTime);
	}
	if ( sWsLastTime ) {
		xrtFree(sWsLastTime);
	}
	if ( sWsLastCloseTime ) {
		xrtFree(sWsLastCloseTime);
	}
	if ( sWsLastErrorTime ) {
		xrtFree(sWsLastErrorTime);
	}
	if ( sWsLastInvalidTime ) {
		xrtFree(sWsLastInvalidTime);
	}
	if ( sWsLastIdleCloseTime ) {
		xrtFree(sWsLastIdleCloseTime);
	}
	if ( sWsLastConnLimitCloseTime ) {
		xrtFree(sWsLastConnLimitCloseTime);
	}
	if ( sWsLastMessageLimitCloseTime ) {
		xrtFree(sWsLastMessageLimitCloseTime);
	}
	if ( sXtpLastTime ) {
		xrtFree(sXtpLastTime);
	}
	if ( sXtpLastInvalidTime ) {
		xrtFree(sXtpLastInvalidTime);
	}
	if ( sXtpLastErrorTime ) {
		xrtFree(sXtpLastErrorTime);
	}
	if ( sXtpLastIdleCloseTime ) {
		xrtFree(sXtpLastIdleCloseTime);
	}
	if ( sXtpLastConnLimitCloseTime ) {
		xrtFree(sXtpLastConnLimitCloseTime);
	}
	if ( sXtpLastRecvLimitCloseTime ) {
		xrtFree(sXtpLastRecvLimitCloseTime);
	}
	if ( sUdpLastTime ) {
		xrtFree(sUdpLastTime);
	}
	if ( sUdpLastErrorTime ) {
		xrtFree(sUdpLastErrorTime);
	}
	if ( sCustomLastTime ) {
		xrtFree(sCustomLastTime);
	}
	if ( sCustomLastCloseTime ) {
		xrtFree(sCustomLastCloseTime);
	}
	if ( sCustomLastErrorTime ) {
		xrtFree(sCustomLastErrorTime);
	}
	if ( sCustomLastInvalidTime ) {
		xrtFree(sCustomLastInvalidTime);
	}
	if ( sCustomLastIdleCloseTime ) {
		xrtFree(sCustomLastIdleCloseTime);
	}
	if ( sCustomLastConnLimitCloseTime ) {
		xrtFree(sCustomLastConnLimitCloseTime);
	}
	if ( sCustomLastRecvLimitCloseTime ) {
		xrtFree(sCustomLastRecvLimitCloseTime);
	}
	if ( sWsLastRejectTime ) {
		xrtFree(sWsLastRejectTime);
	}
	if ( sWsLastStopCleanupTime ) {
		xrtFree(sWsLastStopCleanupTime);
	}
	if ( sXtpLastRejectTime ) {
		xrtFree(sXtpLastRejectTime);
	}
	if ( sXtpLastStopCleanupTime ) {
		xrtFree(sXtpLastStopCleanupTime);
	}
	if ( sCustomLastRejectTime ) {
		xrtFree(sCustomLastRejectTime);
	}
	if ( sCustomLastStopCleanupTime ) {
		xrtFree(sCustomLastStopCleanupTime);
	}

	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleStatusJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	xvalue objHosts;
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sHttpLastIdleCloseTime;
	char* sHttpLastConnLimitCloseTime;
	char* sWsLastTime;
	char* sWsLastCloseTime;
	char* sWsLastErrorTime;
	char* sWsLastInvalidTime;
	char* sWsLastIdleCloseTime;
	char* sWsLastConnLimitCloseTime;
	char* sXtpLastTime;
	char* sXtpLastInvalidTime;
	char* sXtpLastErrorTime;
	char* sXtpLastIdleCloseTime;
	char* sXtpLastConnLimitCloseTime;
	char* sUdpLastTime;
	char* sUdpLastErrorTime;
	char* sCustomLastTime;
	char* sCustomLastCloseTime;
	char* sCustomLastErrorTime;
	char* sCustomLastInvalidTime;
	char* sCustomLastIdleCloseTime;
	char* sCustomLastConnLimitCloseTime;
	char* sJson;
	uint32 i;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/status_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "status json api disabled");
	}

	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sHttpLastIdleCloseTime = XS_HttpLastIdleCloseTimeText();
	sHttpLastConnLimitCloseTime = XS_HttpLastConnLimitCloseTimeText();
	sWsLastTime = XS_WsLastTimeText();
	sWsLastCloseTime = XS_WsLastCloseTimeText();
	sWsLastErrorTime = XS_WsLastErrorTimeText();
	sWsLastInvalidTime = XS_WsLastInvalidTimeText();
	sWsLastIdleCloseTime = XS_WsLastIdleCloseTimeText();
	sWsLastConnLimitCloseTime = XS_WsLastConnLimitCloseTimeText();
	sXtpLastTime = XS_XtpLastTimeText();
	sXtpLastInvalidTime = XS_XtpLastInvalidTimeText();
	sXtpLastErrorTime = XS_XtpLastErrorTimeText();
	sXtpLastIdleCloseTime = XS_XtpLastIdleCloseTimeText();
	sXtpLastConnLimitCloseTime = XS_XtpLastConnLimitCloseTimeText();
	sUdpLastTime = XS_UdpLastTimeText();
	sUdpLastErrorTime = XS_UdpLastErrorTimeText();
	sCustomLastTime = XS_CustomLastTimeText();
	sCustomLastCloseTime = XS_CustomLastCloseTimeText();
	sCustomLastErrorTime = XS_CustomLastErrorTimeText();
	sCustomLastInvalidTime = XS_CustomLastInvalidTimeText();
	sCustomLastIdleCloseTime = XS_CustomLastIdleCloseTimeText();
	sCustomLastConnLimitCloseTime = XS_CustomLastConnLimitCloseTimeText();
	objRet = xvoCreateTable();
	xvoTableSetText(objRet, "server", 6, objServer->Name ? objServer->Name : "", 0, FALSE);
	xvoTableSetText(objRet, "class", 5, (ptr)XS_ServerClassName(objServer->Class), 0, FALSE);
	xvoTableSetText(objRet, "addr", 4, objServer->Addr ? objServer->Addr : "", 0, FALSE);
	xvoTableSetText(objRet, "bind_ip", 7, objServer->BindIP ? objServer->BindIP : "", 0, FALSE);
	xvoTableSetInt(objRet, "bind_port", 9, objServer->BindPort);
	xvoTableSetBool(objRet, "tls", 3, objServer->EnableTLS);
	xvoTableSetText(objRet, "bind_ip_tls", 11, objServer->BindIPTLS ? objServer->BindIPTLS : "", 0, FALSE);
	xvoTableSetInt(objRet, "bind_port_tls", 13, objServer->BindPortTLS);
	xvoTableSetText(objRet, "addr_tls", 8, objServer->AddrTLS ? objServer->AddrTLS : "", 0, FALSE);
	xvoTableSetText(objRet, "ws_protocol", 11, objServer->WsProtocol ? objServer->WsProtocol : "", 0, FALSE);
	xvoTableSetInt(objRet, "ws_message_limit", 16, objServer->WsMessageLimit);
	xvoTableSetInt(objRet, "idle_timeout", 12, objServer->IdleTimeout);
	xvoTableSetInt(objRet, "conn_limit", 10, objServer->ConnLimit);
	xvoTableSetText(objRet, "tls_cert_file", 13, (ptr)XS_TlsConfigFileText(objServer->TlsConfig.sCertFile), 0, FALSE);
	xvoTableSetText(objRet, "tls_key_file", 12, (ptr)XS_TlsConfigFileText(objServer->TlsConfig.sKeyFile), 0, FALSE);
	xvoTableSetText(objRet, "tls_ca_file", 11, (ptr)XS_TlsConfigFileText(objServer->TlsConfig.sCaFile), 0, FALSE);
	xvoTableSetText(objRet, "compiler", 8, (ptr)XS_CompilerName(), 0, FALSE);
	xvoTableSetBool(objRet, "mem_debug", 9, XS_MemDebugEnabled());
	xvoTableSetBool(objRet, "manage_api", 10, XS_ManageAPIEnabled(objServer, objHost));
	xvoTableSetBool(objRet, "debug", 5, objServer->Debug);
	xvoTableSetBool(objRet, "host_aware", 10, objServer->HostAware);
	xvoTableSetBool(objRet, "default_host", 12, objServer->EnableDefaultHost);
	xvoTableSetInt(objRet, "runtime_server_count", 20, (int64)g_iXsRuntimeServerCount);
	xvoTableSetInt(objRet, "http_req_count", 14, XS_HttpMetricGet(&g_iXsHttpReqCount));
	xvoTableSetInt(objRet, "http_manage_req_count", 21, XS_HttpMetricGet(&g_iXsHttpManageReqCount));
	xvoTableSetInt(objRet, "http_app_req_count", 18, XS_HttpMetricGet(&g_iXsHttpAppReqCount));
	xvoTableSetInt(objRet, "http_2xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp2xxCount));
	xvoTableSetInt(objRet, "http_3xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp3xxCount));
	xvoTableSetInt(objRet, "http_4xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp4xxCount));
	xvoTableSetInt(objRet, "http_5xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp5xxCount));
	xvoTableSetInt(objRet, "http_conn_current", 17, XS_HttpMetricGet(&g_iXsHttpConnCurrent));
	xvoTableSetInt(objRet, "http_conn_peak", 14, XS_HttpMetricGet(&g_iXsHttpConnPeak));
	xvoTableSetInt(objRet, "http_idle_close_count", 21, XS_HttpMetricGet(&g_iXsHttpIdleCloseCount));
	xvoTableSetInt(objRet, "http_conn_limit_close_count", 27, XS_HttpMetricGet(&g_iXsHttpConnLimitCloseCount));
	XS_HttpAppendRequestLimitMetrics(objRet);
	XS_HttpAppendPolicyRejectMetrics(objRet);
	XS_HttpAppendManageRejectMetrics(objRet);
	XS_HttpAppendBusRejectMetrics(objRet);
	XS_HttpAppendRejectMetrics(objRet);
	XS_HttpAppendStopCleanupMetrics(objRet);
	XS_WsAppendMessageLimitMetrics(objRet);
	XS_WsAppendRejectMetrics(objRet);
	XS_WsAppendStopCleanupMetrics(objRet);
	XS_XtpAppendRecvLimitMetrics(objRet);
	XS_XtpAppendRejectMetrics(objRet);
	XS_XtpAppendStopCleanupMetrics(objRet);
	XS_CustomAppendRecvLimitMetrics(objRet);
	XS_CustomAppendRejectMetrics(objRet);
	XS_CustomAppendStopCleanupMetrics(objRet);
	xvoTableSetInt(objRet, "http_get_count", 14, XS_HttpMetricGet(&g_iXsHttpMethodGetCount));
	xvoTableSetInt(objRet, "http_post_count", 15, XS_HttpMetricGet(&g_iXsHttpMethodPostCount));
	xvoTableSetInt(objRet, "http_head_count", 15, XS_HttpMetricGet(&g_iXsHttpMethodHeadCount));
	xvoTableSetInt(objRet, "http_other_count", 16, XS_HttpMetricGet(&g_iXsHttpMethodOtherCount));
	xvoTableSetInt(objRet, "http_time_total_ms", 18, XS_HttpMetricGet(&g_iXsHttpTimeTotalMS));
	xvoTableSetInt(objRet, "http_time_max_ms", 16, XS_HttpMetricGet(&g_iXsHttpTimeMaxMS));
	xvoTableSetInt(
		objRet,
		"http_time_avg_ms",
		16,
		(XS_HttpMetricGet(&g_iXsHttpReqCount) > 0)
			? (XS_HttpMetricGet(&g_iXsHttpTimeTotalMS) / XS_HttpMetricGet(&g_iXsHttpReqCount))
			: 0
	);
	xvoTableSetInt(objRet, "http_last_status", 16, XS_HttpMetricGet(&g_iXsHttpLastStatusCode));
	xvoTableSetText(objRet, "http_last_method", 16, (ptr)XS_HttpLastMethodName(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_path", 14, (ptr)XS_HttpLastPath(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_target", 16, (ptr)XS_HttpLastTarget(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_version", 17, (ptr)g_sXsHttpLastVersion, 0, FALSE);
	xvoTableSetText(objRet, "http_last_remote", 16, (ptr)XS_HttpLastRemote(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_host", 14, (ptr)g_sXsHttpLastHost, 0, FALSE);
	xvoTableSetText(objRet, "http_last_user_agent", 20, (ptr)g_sXsHttpLastUserAgent, 0, FALSE);
	xvoTableSetText(objRet, "http_last_referer", 17, (ptr)g_sXsHttpLastReferer, 0, FALSE);
	xvoTableSetText(objRet, "http_last_origin", 16, (ptr)g_sXsHttpLastOrigin, 0, FALSE);
	xvoTableSetText(objRet, "http_last_accept", 16, (ptr)g_sXsHttpLastAccept, 0, FALSE);
	xvoTableSetText(objRet, "http_last_accept_encoding", 25, (ptr)g_sXsHttpLastAcceptEncoding, 0, FALSE);
	xvoTableSetText(objRet, "http_last_cookie", 16, (ptr)g_sXsHttpLastCookie, 0, FALSE);
	xvoTableSetText(objRet, "http_last_forwarded_for", 23, (ptr)g_sXsHttpLastForwardedFor, 0, FALSE);
	xvoTableSetText(objRet, "http_last_real_ip", 17, (ptr)g_sXsHttpLastRealIP, 0, FALSE);
	xvoTableSetText(objRet, "http_last_connection", 20, (ptr)g_sXsHttpLastConnection, 0, FALSE);
	xvoTableSetText(objRet, "http_last_cache_control", 23, (ptr)g_sXsHttpLastCacheControl, 0, FALSE);
	xvoTableSetText(objRet, "http_last_content_type", 22, (ptr)g_sXsHttpLastContentType, 0, FALSE);
	xvoTableSetInt(objRet, "http_last_header_count", 22, g_iXsHttpLastHeaderCount);
	xvoTableSetInt(objRet, "http_last_query_len", 19, g_iXsHttpLastQueryLen);
	xvoTableSetInt(objRet, "http_last_body_len", 18, g_iXsHttpLastBodyLen);
	xvoTableSetText(objRet, "http_last_time", 14, (ptr)(sHttpLastTime ? sHttpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_age_ms", 16, XS_HttpLastRequestAgeMS());
	xvoTableSetInt(objRet, "http_last_duration_ms", 21, g_iXsHttpLastTimeMS);
	xvoTableSetText(objRet, "http_last_idle_close_time", 25, (ptr)(sHttpLastIdleCloseTime ? sHttpLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_idle_close_age_ms", 27, XS_HttpLastIdleCloseAgeMS());
	xvoTableSetText(objRet, "http_last_conn_limit_close_time", 31, (ptr)(sHttpLastConnLimitCloseTime ? sHttpLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_conn_limit_close_age_ms", 33, XS_HttpLastConnLimitCloseAgeMS());
	xvoTableSetInt(objRet, "http_last_app_status", 20, XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode));
	xvoTableSetText(objRet, "http_last_app_method", 20, (ptr)XS_HttpLastAppMethodName(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_path", 18, (ptr)XS_HttpLastAppPath(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_target", 20, (ptr)XS_HttpLastAppTarget(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_version", 21, (ptr)g_sXsHttpLastAppVersion, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_remote", 20, (ptr)XS_HttpLastAppRemote(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_host", 18, (ptr)g_sXsHttpLastAppHost, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_user_agent", 24, (ptr)g_sXsHttpLastAppUserAgent, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_referer", 21, (ptr)g_sXsHttpLastAppReferer, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_origin", 20, (ptr)g_sXsHttpLastAppOrigin, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_accept", 20, (ptr)g_sXsHttpLastAppAccept, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_accept_encoding", 29, (ptr)g_sXsHttpLastAppAcceptEncoding, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_cookie", 20, (ptr)g_sXsHttpLastAppCookie, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_forwarded_for", 27, (ptr)g_sXsHttpLastAppForwardedFor, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_real_ip", 21, (ptr)g_sXsHttpLastAppRealIP, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_connection", 24, (ptr)g_sXsHttpLastAppConnection, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_cache_control", 27, (ptr)g_sXsHttpLastAppCacheControl, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_content_type", 26, (ptr)g_sXsHttpLastAppContentType, 0, FALSE);
	xvoTableSetInt(objRet, "http_last_app_header_count", 26, g_iXsHttpLastAppHeaderCount);
	xvoTableSetInt(objRet, "http_last_app_query_len", 23, g_iXsHttpLastAppQueryLen);
	xvoTableSetInt(objRet, "http_last_app_body_len", 22, g_iXsHttpLastAppBodyLen);
	xvoTableSetText(objRet, "http_last_app_time", 18, (ptr)(sHttpLastAppTime ? sHttpLastAppTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_app_age_ms", 20, XS_HttpLastAppRequestAgeMS());
	xvoTableSetInt(objRet, "http_last_app_duration_ms", 25, g_iXsHttpLastAppTimeMS);
	xvoTableSetInt(objRet, "ws_conn_current", 15, XS_HttpMetricGet(&g_iXsWsConnCurrent));
	xvoTableSetInt(objRet, "ws_conn_peak", 12, XS_HttpMetricGet(&g_iXsWsConnPeak));
	xvoTableSetInt(objRet, "ws_open_count", 13, XS_HttpMetricGet(&g_iXsWsOpenCount));
	xvoTableSetInt(objRet, "ws_close_count", 14, XS_HttpMetricGet(&g_iXsWsCloseCount));
	xvoTableSetInt(objRet, "ws_text_count", 13, XS_HttpMetricGet(&g_iXsWsTextCount));
	xvoTableSetInt(objRet, "ws_binary_count", 15, XS_HttpMetricGet(&g_iXsWsBinaryCount));
	xvoTableSetInt(objRet, "ws_ping_count", 13, XS_HttpMetricGet(&g_iXsWsPingCount));
	xvoTableSetInt(objRet, "ws_pong_count", 13, XS_HttpMetricGet(&g_iXsWsPongCount));
	xvoTableSetInt(objRet, "ws_error_count", 14, XS_HttpMetricGet(&g_iXsWsErrorCount));
	xvoTableSetInt(objRet, "ws_invalid_count", 16, XS_HttpMetricGet(&g_iXsWsInvalidCount));
	xvoTableSetInt(objRet, "ws_idle_close_count", 19, XS_HttpMetricGet(&g_iXsWsIdleCloseCount));
	xvoTableSetInt(objRet, "ws_conn_limit_close_count", 25, XS_HttpMetricGet(&g_iXsWsConnLimitCloseCount));
	XS_WsAppendMessageLimitMetrics(objRet);
	XS_WsAppendRejectMetrics(objRet);
	XS_WsAppendStopCleanupMetrics(objRet);
	xvoTableSetInt(objRet, "ws_last_error_code", 18, g_iXsWsLastErrorCode);
	xvoTableSetText(objRet, "ws_last_invalid_reason", 22, (ptr)g_sXsWsLastInvalidReason, 0, FALSE);
	xvoTableSetText(objRet, "ws_last_invalid_time", 20, (ptr)(sWsLastInvalidTime ? sWsLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_invalid_age_ms", 22, XS_WsLastInvalidAgeMS());
	xvoTableSetInt(objRet, "ws_last_close_reason", 20, g_iXsWsLastCloseReason);
	xvoTableSetText(objRet, "ws_last_close_time", 18, (ptr)(sWsLastCloseTime ? sWsLastCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_close_age_ms", 20, XS_WsLastCloseAgeMS());
	xvoTableSetText(objRet, "ws_last_idle_close_time", 23, (ptr)(sWsLastIdleCloseTime ? sWsLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_idle_close_age_ms", 25, XS_WsLastIdleCloseAgeMS());
	xvoTableSetText(objRet, "ws_last_conn_limit_close_time", 29, (ptr)(sWsLastConnLimitCloseTime ? sWsLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_conn_limit_close_age_ms", 31, XS_WsLastConnLimitCloseAgeMS());
	xvoTableSetText(objRet, "ws_last_frame_type", 18, (ptr)XS_WsLastFrameTypeName(), 0, FALSE);
	xvoTableSetText(objRet, "ws_last_remote", 14, (ptr)g_sXsWsLastRemote, 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_bytes", 13, XS_HttpMetricGet(&g_iXsWsLastBytes));
	xvoTableSetText(objRet, "ws_last_text", 12, (ptr)g_sXsWsLastText, 0, FALSE);
	xvoTableSetText(objRet, "ws_last_time", 12, (ptr)(sWsLastTime ? sWsLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_age_ms", 14, XS_WsLastAgeMS());
	xvoTableSetText(objRet, "ws_last_error_time", 18, (ptr)(sWsLastErrorTime ? sWsLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_error_age_ms", 20, XS_WsLastErrorAgeMS());
	xvoTableSetInt(objRet, "xtp_conn_current", 16, XS_HttpMetricGet(&g_iXsXtpConnCurrent));
	xvoTableSetInt(objRet, "xtp_conn_peak", 13, XS_HttpMetricGet(&g_iXsXtpConnPeak));
	xvoTableSetInt(objRet, "xtp_open_count", 14, XS_HttpMetricGet(&g_iXsXtpOpenCount));
	xvoTableSetInt(objRet, "xtp_close_count", 15, XS_HttpMetricGet(&g_iXsXtpCloseCount));
	xvoTableSetInt(objRet, "xtp_error_count", 15, XS_HttpMetricGet(&g_iXsXtpErrorCount));
	xvoTableSetInt(objRet, "xtp_msg_count", 13, XS_HttpMetricGet(&g_iXsXtpMsgCount));
	xvoTableSetInt(objRet, "xtp_req_count", 13, XS_HttpMetricGet(&g_iXsXtpReqCount));
	xvoTableSetInt(objRet, "xtp_resp_count", 14, XS_HttpMetricGet(&g_iXsXtpRespCount));
	xvoTableSetInt(objRet, "xtp_push_count", 14, XS_HttpMetricGet(&g_iXsXtpPushCount));
	xvoTableSetInt(objRet, "xtp_event_count", 15, XS_HttpMetricGet(&g_iXsXtpEventCount));
	xvoTableSetInt(objRet, "xtp_send_count", 14, XS_HttpMetricGet(&g_iXsXtpSendCount));
	xvoTableSetInt(objRet, "xtp_recv_bytes", 14, XS_HttpMetricGet(&g_iXsXtpRecvBytes));
	xvoTableSetInt(objRet, "xtp_send_bytes", 14, XS_HttpMetricGet(&g_iXsXtpSendBytes));
	xvoTableSetInt(objRet, "xtp_idle_close_count", 20, XS_HttpMetricGet(&g_iXsXtpIdleCloseCount));
	xvoTableSetInt(objRet, "xtp_conn_limit_close_count", 26, XS_HttpMetricGet(&g_iXsXtpConnLimitCloseCount));
	XS_XtpAppendRecvLimitMetrics(objRet);
	XS_XtpAppendRejectMetrics(objRet);
	XS_XtpAppendStopCleanupMetrics(objRet);
	xvoTableSetText(objRet, "xtp_last_msg_type", 17, (ptr)XS_XtpLastMsgTypeName(), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_status", 15, XS_HttpMetricGet(&g_iXsXtpLastStatus));
	xvoTableSetInt(objRet, "xtp_last_msg_id", 15, XS_HttpMetricGet(&g_iXsXtpLastMsgID));
	xvoTableSetInt(objRet, "xtp_last_flags", 14, XS_HttpMetricGet(&g_iXsXtpLastFlags));
	xvoTableSetInt(objRet, "xtp_last_param_count", 20, XS_HttpMetricGet(&g_iXsXtpLastParamCount));
	xvoTableSetInt(objRet, "xtp_last_body_size", 18, XS_HttpMetricGet(&g_iXsXtpLastBodySize));
	xvoTableSetText(objRet, "xtp_last_remote", 15, (ptr)g_sXsXtpLastRemote, 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_bytes", 14, g_iXsXtpLastBytes);
	xvoTableSetText(objRet, "xtp_last_cmd", 12, (ptr)g_sXsXtpLastCmd, 0, FALSE);
	xvoTableSetText(objRet, "xtp_last_time", 13, (ptr)(sXtpLastTime ? sXtpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_age_ms", 15, XS_XtpLastAgeMS());
	xvoTableSetText(objRet, "xtp_last_invalid_reason", 23, (ptr)g_sXsXtpLastInvalidReason, 0, FALSE);
	xvoTableSetText(objRet, "xtp_last_invalid_time", 21, (ptr)(sXtpLastInvalidTime ? sXtpLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_invalid_age_ms", 23, XS_XtpLastInvalidAgeMS());
	xvoTableSetInt(objRet, "xtp_last_error_code", 19, g_iXsXtpLastErrorCode);
	xvoTableSetText(objRet, "xtp_last_error_time", 19, (ptr)(sXtpLastErrorTime ? sXtpLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_error_age_ms", 21, XS_XtpLastErrorAgeMS());
	xvoTableSetText(objRet, "xtp_last_idle_close_time", 24, (ptr)(sXtpLastIdleCloseTime ? sXtpLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_idle_close_age_ms", 26, XS_XtpLastIdleCloseAgeMS());
	xvoTableSetText(objRet, "xtp_last_conn_limit_close_time", 30, (ptr)(sXtpLastConnLimitCloseTime ? sXtpLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_conn_limit_close_age_ms", 32, XS_XtpLastConnLimitCloseAgeMS());
	xvoTableSetInt(objRet, "udp_recv_count", 14, XS_HttpMetricGet(&g_iXsUdpRecvCount));
	xvoTableSetInt(objRet, "udp_send_count", 14, XS_HttpMetricGet(&g_iXsUdpSendCount));
	xvoTableSetInt(objRet, "udp_error_count", 15, XS_HttpMetricGet(&g_iXsUdpErrorCount));
	xvoTableSetInt(objRet, "udp_recv_bytes", 14, XS_HttpMetricGet(&g_iXsUdpRecvBytes));
	xvoTableSetInt(objRet, "udp_send_bytes", 14, XS_HttpMetricGet(&g_iXsUdpSendBytes));
	xvoTableSetText(objRet, "udp_last_from", 13, (ptr)g_sXsUdpLastFrom, 0, FALSE);
	xvoTableSetText(objRet, "udp_last_text", 13, (ptr)g_sXsUdpLastText, 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_bytes", 14, g_iXsUdpLastBytes);
	xvoTableSetText(objRet, "udp_last_time", 13, (ptr)(sUdpLastTime ? sUdpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_age_ms", 15, XS_UdpLastAgeMS());
	xvoTableSetInt(objRet, "udp_last_error_code", 19, g_iXsUdpLastErrorCode);
	xvoTableSetText(objRet, "udp_last_error_time", 19, (ptr)(sUdpLastErrorTime ? sUdpLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_error_age_ms", 21, XS_UdpLastErrorAgeMS());
	xvoTableSetInt(objRet, "custom_conn_current", 19, XS_HttpMetricGet(&g_iXsCustomConnCurrent));
	xvoTableSetInt(objRet, "custom_conn_peak", 16, XS_HttpMetricGet(&g_iXsCustomConnPeak));
	xvoTableSetInt(objRet, "custom_open_count", 17, XS_HttpMetricGet(&g_iXsCustomOpenCount));
	xvoTableSetInt(objRet, "custom_close_count", 18, XS_HttpMetricGet(&g_iXsCustomCloseCount));
	xvoTableSetInt(objRet, "custom_error_count", 18, XS_HttpMetricGet(&g_iXsCustomErrorCount));
	xvoTableSetInt(objRet, "custom_invalid_count", 20, XS_HttpMetricGet(&g_iXsCustomInvalidCount));
	xvoTableSetText(objRet, "custom_last_invalid_reason", 26, (ptr)g_sXsCustomLastInvalidReason, 0, FALSE);
	xvoTableSetText(objRet, "custom_last_invalid_time", 24, (ptr)(sCustomLastInvalidTime ? sCustomLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_invalid_age_ms", 26, XS_CustomLastInvalidAgeMS());
	xvoTableSetInt(objRet, "custom_last_close_reason", 24, g_iXsCustomLastCloseReason);
	xvoTableSetText(objRet, "custom_last_close_time", 22, (ptr)(sCustomLastCloseTime ? sCustomLastCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_close_age_ms", 24, XS_CustomLastCloseAgeMS());
	xvoTableSetInt(objRet, "custom_last_error_code", 22, g_iXsCustomLastErrorCode);
	xvoTableSetText(objRet, "custom_last_error_time", 22, (ptr)(sCustomLastErrorTime ? sCustomLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_error_age_ms", 24, XS_CustomLastErrorAgeMS());
	xvoTableSetInt(objRet, "custom_recv_count", 17, XS_HttpMetricGet(&g_iXsCustomRecvCount));
	xvoTableSetInt(objRet, "custom_send_count", 17, XS_HttpMetricGet(&g_iXsCustomSendCount));
	xvoTableSetInt(objRet, "custom_recv_bytes", 17, XS_HttpMetricGet(&g_iXsCustomRecvBytes));
	xvoTableSetInt(objRet, "custom_send_bytes", 17, XS_HttpMetricGet(&g_iXsCustomSendBytes));
	xvoTableSetInt(objRet, "custom_idle_close_count", 23, XS_HttpMetricGet(&g_iXsCustomIdleCloseCount));
	xvoTableSetInt(objRet, "custom_conn_limit_close_count", 29, XS_HttpMetricGet(&g_iXsCustomConnLimitCloseCount));
	XS_CustomAppendRecvLimitMetrics(objRet);
	XS_CustomAppendRejectMetrics(objRet);
	XS_CustomAppendStopCleanupMetrics(objRet);
	xvoTableSetText(objRet, "custom_last_remote", 18, (ptr)g_sXsCustomLastRemote, 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_bytes", 17, g_iXsCustomLastBytes);
	xvoTableSetText(objRet, "custom_last_text", 16, (ptr)g_sXsCustomLastText, 0, FALSE);
	xvoTableSetText(objRet, "custom_last_time", 16, (ptr)(sCustomLastTime ? sCustomLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_age_ms", 18, XS_CustomLastAgeMS());
	xvoTableSetText(objRet, "custom_last_idle_close_time", 27, (ptr)(sCustomLastIdleCloseTime ? sCustomLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_idle_close_age_ms", 29, XS_CustomLastIdleCloseAgeMS());
	xvoTableSetText(objRet, "custom_last_conn_limit_close_time", 33, (ptr)(sCustomLastConnLimitCloseTime ? sCustomLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_conn_limit_close_age_ms", 35, XS_CustomLastConnLimitCloseAgeMS());
	xvoTableSetInt(objRet, "path_limit", 10, objServer->PathLimit);
	xvoTableSetInt(objRet, "header_limit", 12, objServer->HeaderLimit);
	xvoTableSetInt(objRet, "body_limit", 10, objServer->BodyLimit);
	xvoTableSetInt(objRet, "recv_limit", 10, objServer->RecvLimit);
	xvoTableSetInt(objRet, "backlog", 7, objServer->Backlog);

	if ( objServer->EnableDefaultHost ) {
		xvalue objDefault = xvoCreateTable();

		xvoTableSetText(objDefault, "name", 4, objServer->DefaultHost.Name ? objServer->DefaultHost.Name : "", 0, FALSE);
		xvoTableSetText(objDefault, "dev", 3, (ptr)XS_DevModeName(objServer->DefaultHost.DevMode), 0, FALSE);
		xvoTableSetBool(objDefault, "debug", 5, objServer->DefaultHost.Debug);
		xvoTableSetText(objDefault, "devfile", 7, objServer->DefaultHost.DevFile ? objServer->DefaultHost.DevFile : "", 0, FALSE);
		xvoTableSetBool(objDefault, "script_loaded", 13, objServer->DefaultHost.pScriptState != NULL);
		xvoTableSetValue(objRet, "default", 7, objDefault, TRUE);
	}

	objHosts = xvoCreateArray();
	xvoTableSetInt(objRet, "host_count", 10, objServer->Hosts ? objServer->Hosts->Count : 0);
	if ( objServer->Hosts ) {
		for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
			XS_HostConfig* objItem = xrtArrayGet_Inline(objServer->Hosts, i);
			xvalue objHostItem = xvoCreateTable();

			xvoTableSetText(objHostItem, "name", 4, objItem->Name ? objItem->Name : "", 0, FALSE);
			xvoTableSetText(objHostItem, "dev", 3, (ptr)XS_DevModeName(objItem->DevMode), 0, FALSE);
			xvoTableSetBool(objHostItem, "debug", 5, objItem->Debug);
			xvoTableSetText(objHostItem, "devfile", 7, objItem->DevFile ? objItem->DevFile : "", 0, FALSE);
			xvoTableSetBool(objHostItem, "script_loaded", 13, objItem->pScriptState != NULL);
			xvoArrayAppendValue(objHosts, objHostItem, TRUE);
		}
	}
	xvoTableSetValue(objRet, "hosts", 5, objHosts, TRUE);

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
	}
	if ( sHttpLastIdleCloseTime ) {
		xrtFree(sHttpLastIdleCloseTime);
	}
	if ( sWsLastTime ) {
		xrtFree(sWsLastTime);
	}
	if ( sWsLastCloseTime ) {
		xrtFree(sWsLastCloseTime);
	}
	if ( sWsLastErrorTime ) {
		xrtFree(sWsLastErrorTime);
	}
	if ( sWsLastInvalidTime ) {
		xrtFree(sWsLastInvalidTime);
	}
	if ( sWsLastIdleCloseTime ) {
		xrtFree(sWsLastIdleCloseTime);
	}
	if ( sXtpLastTime ) {
		xrtFree(sXtpLastTime);
	}
	if ( sXtpLastInvalidTime ) {
		xrtFree(sXtpLastInvalidTime);
	}
	if ( sXtpLastErrorTime ) {
		xrtFree(sXtpLastErrorTime);
	}
	if ( sXtpLastIdleCloseTime ) {
		xrtFree(sXtpLastIdleCloseTime);
	}
	if ( sXtpLastConnLimitCloseTime ) {
		xrtFree(sXtpLastConnLimitCloseTime);
	}
	if ( sUdpLastTime ) {
		xrtFree(sUdpLastTime);
	}
	if ( sUdpLastErrorTime ) {
		xrtFree(sUdpLastErrorTime);
	}
	if ( sCustomLastTime ) {
		xrtFree(sCustomLastTime);
	}
	if ( sCustomLastCloseTime ) {
		xrtFree(sCustomLastCloseTime);
	}
	if ( sCustomLastErrorTime ) {
		xrtFree(sCustomLastErrorTime);
	}
	if ( sCustomLastInvalidTime ) {
		xrtFree(sCustomLastInvalidTime);
	}
	if ( sCustomLastIdleCloseTime ) {
		xrtFree(sCustomLastIdleCloseTime);
	}
	if ( sCustomLastConnLimitCloseTime ) {
		xrtFree(sCustomLastConnLimitCloseTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "status json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpServerHealthy(const XS_ServerConfig* objServer, const XS_HostConfig* objHost)
{
	if ( objServer == NULL || objHost == NULL ) {
		return FALSE;
	}

	if ( objServer->EnableDefaultHost && objServer->DefaultHost.DevMode == XS_DEV_SCRIPT_C && objServer->DefaultHost.pScriptState == NULL ) {
		return FALSE;
	}
	if ( objHost->DevMode == XS_DEV_SCRIPT_C && objHost->pScriptState == NULL ) {
		return FALSE;
	}

	return TRUE;
}

static inline bool XS_HttpHandleHealthJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ReloadStatusSnapshot tReloadStatus;
	XS_CheckConfigStatusSnapshot tCheckStatus;
	xvalue objRet;
	xvalue objBus;
	char* sCheckTime;
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sHttpLastIdleCloseTime;
	char* sHttpLastConnLimitCloseTime;
	char* sReloadTime;
	char* sJson;
	bool bScriptLoaded;
	bool bOk;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/health_json") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "health json api disabled");
	}

	XS_ConfigReloadStatusSnapshot(&tReloadStatus);
	XS_ConfigCheckStatusSnapshot(&tCheckStatus);
	sReloadTime = XS_ReloadTimeTextByStatus(&tReloadStatus);
	sCheckTime = XS_CheckConfigTimeTextByStatus(&tCheckStatus);
	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sHttpLastIdleCloseTime = XS_HttpLastIdleCloseTimeText();
	sHttpLastConnLimitCloseTime = XS_HttpLastConnLimitCloseTimeText();
	bScriptLoaded = FALSE;
	if ( objServer->EnableDefaultHost && objServer->DefaultHost.pScriptState ) {
		bScriptLoaded = TRUE;
	}
	if ( !bScriptLoaded && objHost->pScriptState ) {
		bScriptLoaded = TRUE;
	}
	bOk = XS_HttpServerHealthy(objServer, objHost);

	objRet = xvoCreateTable();
	xvoTableSetBool(objRet, "ok", 2, bOk);
	xvoTableSetText(objRet, "server", 6, objServer->Name ? objServer->Name : "", 0, FALSE);
	xvoTableSetText(objRet, "class", 5, (ptr)XS_ServerClassName(objServer->Class), 0, FALSE);
	xvoTableSetText(objRet, "addr", 4, objServer->Addr ? objServer->Addr : "", 0, FALSE);
	xvoTableSetBool(objRet, "debug", 5, objServer->Debug);
	xvoTableSetBool(objRet, "host_aware", 10, objServer->HostAware);
	xvoTableSetBool(objRet, "script_loaded", 13, bScriptLoaded);
	xvoTableSetInt(objRet, "host_count", 10, objServer->Hosts ? objServer->Hosts->Count : 0);
	xvoTableSetBool(objRet, "reload_busy", 11, tReloadStatus.Busy);
	xvoTableSetBool(objRet, "reload_has_result", 17, tReloadStatus.HasResult);
	xvoTableSetBool(objRet, "reload_success", 14, tReloadStatus.Success);
	xvoTableSetText(objRet, "reload_server", 13, (ptr)(tReloadStatus.sServerName[0] ? tReloadStatus.sServerName : "(all)"), 0, FALSE);
	xvoTableSetText(objRet, "reload_host", 11, (ptr)(tReloadStatus.sHostName[0] ? tReloadStatus.sHostName : "(all)"), 0, FALSE);
	xvoTableSetText(objRet, "reload_message", 14, (ptr)(tReloadStatus.sMessage[0] ? tReloadStatus.sMessage : "(none)"), 0, FALSE);
	xvoTableSetText(objRet, "reload_time", 11, (ptr)(sReloadTime ? sReloadTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "reload_age_ms", 13, XS_ReloadAgeMSByStatus(&tReloadStatus));
	xvoTableSetInt(objRet, "reload_total_count", 18, tReloadStatus.iTotalCount);
	xvoTableSetInt(objRet, "reload_success_count", 20, tReloadStatus.iSuccessCount);
	xvoTableSetInt(objRet, "reload_failure_count", 20, tReloadStatus.iFailureCount);
	xvoTableSetInt(objRet, "check_total_count", 17, tCheckStatus.iTotalCount);
	xvoTableSetInt(objRet, "check_success_count", 19, tCheckStatus.iSuccessCount);
	xvoTableSetInt(objRet, "check_failure_count", 19, tCheckStatus.iFailureCount);
	xvoTableSetText(objRet, "check_last_time", 15, (ptr)(sCheckTime ? sCheckTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "check_last_age_ms", 17, XS_CheckConfigAgeMSByStatus(&tCheckStatus));
	XS_HttpAppendCheckConfigSnapshotByStatus(objRet, &tCheckStatus);
	xvoTableSetInt(objRet, "http_req_count", 14, XS_HttpMetricGet(&g_iXsHttpReqCount));
	xvoTableSetInt(objRet, "http_manage_req_count", 21, XS_HttpMetricGet(&g_iXsHttpManageReqCount));
	xvoTableSetInt(objRet, "http_app_req_count", 18, XS_HttpMetricGet(&g_iXsHttpAppReqCount));
	xvoTableSetInt(objRet, "http_2xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp2xxCount));
	xvoTableSetInt(objRet, "http_3xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp3xxCount));
	xvoTableSetInt(objRet, "http_4xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp4xxCount));
	xvoTableSetInt(objRet, "http_5xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp5xxCount));
	xvoTableSetInt(objRet, "http_conn_current", 17, XS_HttpMetricGet(&g_iXsHttpConnCurrent));
	xvoTableSetInt(objRet, "http_conn_peak", 14, XS_HttpMetricGet(&g_iXsHttpConnPeak));
	xvoTableSetInt(objRet, "http_idle_close_count", 21, XS_HttpMetricGet(&g_iXsHttpIdleCloseCount));
	xvoTableSetInt(objRet, "http_conn_limit_close_count", 27, XS_HttpMetricGet(&g_iXsHttpConnLimitCloseCount));
	XS_HttpAppendRequestLimitMetrics(objRet);
	XS_HttpAppendPolicyRejectMetrics(objRet);
	XS_HttpAppendManageRejectMetrics(objRet);
	XS_HttpAppendBusRejectMetrics(objRet);
	XS_HttpAppendRejectMetrics(objRet);
	XS_HttpAppendStopCleanupMetrics(objRet);
	XS_WsAppendMessageLimitMetrics(objRet);
	XS_WsAppendRejectMetrics(objRet);
	XS_WsAppendStopCleanupMetrics(objRet);
	XS_XtpAppendRecvLimitMetrics(objRet);
	XS_XtpAppendRejectMetrics(objRet);
	XS_XtpAppendStopCleanupMetrics(objRet);
	XS_CustomAppendRecvLimitMetrics(objRet);
	XS_CustomAppendRejectMetrics(objRet);
	XS_CustomAppendStopCleanupMetrics(objRet);
	xvoTableSetInt(objRet, "http_get_count", 14, XS_HttpMetricGet(&g_iXsHttpMethodGetCount));
	xvoTableSetInt(objRet, "http_post_count", 15, XS_HttpMetricGet(&g_iXsHttpMethodPostCount));
	xvoTableSetInt(objRet, "http_head_count", 15, XS_HttpMetricGet(&g_iXsHttpMethodHeadCount));
	xvoTableSetInt(objRet, "http_other_count", 16, XS_HttpMetricGet(&g_iXsHttpMethodOtherCount));
	xvoTableSetInt(objRet, "http_time_total_ms", 18, XS_HttpMetricGet(&g_iXsHttpTimeTotalMS));
	xvoTableSetInt(objRet, "http_time_max_ms", 16, XS_HttpMetricGet(&g_iXsHttpTimeMaxMS));
	xvoTableSetInt(
		objRet,
		"http_time_avg_ms",
		16,
		(XS_HttpMetricGet(&g_iXsHttpReqCount) > 0)
			? (XS_HttpMetricGet(&g_iXsHttpTimeTotalMS) / XS_HttpMetricGet(&g_iXsHttpReqCount))
			: 0
	);
	xvoTableSetInt(objRet, "http_last_status", 16, XS_HttpMetricGet(&g_iXsHttpLastStatusCode));
	xvoTableSetText(objRet, "http_last_method", 16, (ptr)XS_HttpLastMethodName(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_path", 14, (ptr)XS_HttpLastPath(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_target", 16, (ptr)XS_HttpLastTarget(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_version", 17, (ptr)g_sXsHttpLastVersion, 0, FALSE);
	xvoTableSetText(objRet, "http_last_remote", 16, (ptr)XS_HttpLastRemote(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_host", 14, (ptr)g_sXsHttpLastHost, 0, FALSE);
	xvoTableSetText(objRet, "http_last_user_agent", 20, (ptr)g_sXsHttpLastUserAgent, 0, FALSE);
	xvoTableSetText(objRet, "http_last_referer", 17, (ptr)g_sXsHttpLastReferer, 0, FALSE);
	xvoTableSetText(objRet, "http_last_origin", 16, (ptr)g_sXsHttpLastOrigin, 0, FALSE);
	xvoTableSetText(objRet, "http_last_accept", 16, (ptr)g_sXsHttpLastAccept, 0, FALSE);
	xvoTableSetText(objRet, "http_last_accept_encoding", 25, (ptr)g_sXsHttpLastAcceptEncoding, 0, FALSE);
	xvoTableSetText(objRet, "http_last_cookie", 16, (ptr)g_sXsHttpLastCookie, 0, FALSE);
	xvoTableSetText(objRet, "http_last_forwarded_for", 23, (ptr)g_sXsHttpLastForwardedFor, 0, FALSE);
	xvoTableSetText(objRet, "http_last_real_ip", 17, (ptr)g_sXsHttpLastRealIP, 0, FALSE);
	xvoTableSetText(objRet, "http_last_connection", 20, (ptr)g_sXsHttpLastConnection, 0, FALSE);
	xvoTableSetText(objRet, "http_last_cache_control", 23, (ptr)g_sXsHttpLastCacheControl, 0, FALSE);
	xvoTableSetText(objRet, "http_last_content_type", 22, (ptr)g_sXsHttpLastContentType, 0, FALSE);
	xvoTableSetInt(objRet, "http_last_header_count", 22, g_iXsHttpLastHeaderCount);
	xvoTableSetInt(objRet, "http_last_query_len", 19, g_iXsHttpLastQueryLen);
	xvoTableSetInt(objRet, "http_last_body_len", 18, g_iXsHttpLastBodyLen);
	xvoTableSetText(objRet, "http_last_time", 14, (ptr)(sHttpLastTime ? sHttpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_age_ms", 16, XS_HttpLastRequestAgeMS());
	xvoTableSetInt(objRet, "http_last_duration_ms", 21, g_iXsHttpLastTimeMS);
	xvoTableSetText(objRet, "http_last_idle_close_time", 25, (ptr)(sHttpLastIdleCloseTime ? sHttpLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_idle_close_age_ms", 27, XS_HttpLastIdleCloseAgeMS());
	xvoTableSetText(objRet, "http_last_conn_limit_close_time", 31, (ptr)(sHttpLastConnLimitCloseTime ? sHttpLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_conn_limit_close_age_ms", 33, XS_HttpLastConnLimitCloseAgeMS());
	xvoTableSetInt(objRet, "http_last_app_status", 20, XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode));
	xvoTableSetText(objRet, "http_last_app_method", 20, (ptr)XS_HttpLastAppMethodName(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_path", 18, (ptr)XS_HttpLastAppPath(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_target", 20, (ptr)XS_HttpLastAppTarget(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_version", 21, (ptr)g_sXsHttpLastAppVersion, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_remote", 20, (ptr)XS_HttpLastAppRemote(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_host", 18, (ptr)g_sXsHttpLastAppHost, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_user_agent", 24, (ptr)g_sXsHttpLastAppUserAgent, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_referer", 21, (ptr)g_sXsHttpLastAppReferer, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_origin", 20, (ptr)g_sXsHttpLastAppOrigin, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_accept", 20, (ptr)g_sXsHttpLastAppAccept, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_accept_encoding", 29, (ptr)g_sXsHttpLastAppAcceptEncoding, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_cookie", 20, (ptr)g_sXsHttpLastAppCookie, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_forwarded_for", 27, (ptr)g_sXsHttpLastAppForwardedFor, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_real_ip", 21, (ptr)g_sXsHttpLastAppRealIP, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_connection", 24, (ptr)g_sXsHttpLastAppConnection, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_cache_control", 27, (ptr)g_sXsHttpLastAppCacheControl, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_content_type", 26, (ptr)g_sXsHttpLastAppContentType, 0, FALSE);
	xvoTableSetInt(objRet, "http_last_app_header_count", 26, g_iXsHttpLastAppHeaderCount);
	xvoTableSetInt(objRet, "http_last_app_query_len", 23, g_iXsHttpLastAppQueryLen);
	xvoTableSetInt(objRet, "http_last_app_body_len", 22, g_iXsHttpLastAppBodyLen);
	xvoTableSetText(objRet, "http_last_app_time", 18, (ptr)(sHttpLastAppTime ? sHttpLastAppTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_app_age_ms", 20, XS_HttpLastAppRequestAgeMS());
	xvoTableSetInt(objRet, "http_last_app_duration_ms", 25, g_iXsHttpLastAppTimeMS);

	objBus = XS_BusBuildNamespaceStatsValue();
	if ( objBus ) {
		xvoTableSetInt(objRet, "bus_queue_count", 15, xvoTableGetInt(objBus, "queue_count", 11));
		xvoTableSetInt(objRet, "bus_data_count", 14, xvoTableGetInt(objBus, "data_count", 10));
		xvoTableSetInt(objRet, "bus_namespace_count", sizeof("bus_namespace_count") - 1, xvoTableGetInt(objBus, "namespace_count", sizeof("namespace_count") - 1));
		xvoTableSetInt(objRet, "bus_namespace_item_count", sizeof("bus_namespace_item_count") - 1, xvoTableGetInt(objBus, "namespace_item_count", sizeof("namespace_item_count") - 1));
		xvoTableSetInt(objRet, "bus_data_limit", sizeof("bus_data_limit") - 1, xvoTableGetInt(objBus, "data_limit", sizeof("data_limit") - 1));
		xvoTableSetInt(objRet, "bus_queue_limit", sizeof("bus_queue_limit") - 1, xvoTableGetInt(objBus, "queue_limit", sizeof("queue_limit") - 1));
		xvoTableSetInt(objRet, "bus_namespace_limit", sizeof("bus_namespace_limit") - 1, xvoTableGetInt(objBus, "namespace_limit", sizeof("namespace_limit") - 1));
		xvoTableSetInt(objRet, "bus_namespace_data_limit", sizeof("bus_namespace_data_limit") - 1, xvoTableGetInt(objBus, "namespace_data_limit", sizeof("namespace_data_limit") - 1));
		xvoTableSetInt(objRet, "bus_namespace_limit_remaining", sizeof("bus_namespace_limit_remaining") - 1, xvoTableGetInt(objBus, "namespace_limit_remaining", sizeof("namespace_limit_remaining") - 1));
		xvoTableSetBool(objRet, "bus_namespace_limit_reached", sizeof("bus_namespace_limit_reached") - 1, xvoGetBool(xvoTableGetValue(objBus, "namespace_limit_reached", sizeof("namespace_limit_reached") - 1)));
		xvoTableSetInt(objRet, "bus_readonly_namespace_count", sizeof("bus_readonly_namespace_count") - 1, xvoTableGetInt(objBus, "readonly_namespace_count", sizeof("readonly_namespace_count") - 1));
		xvoTableSetInt(objRet, "bus_disabled_namespace_count", sizeof("bus_disabled_namespace_count") - 1, xvoTableGetInt(objBus, "disabled_namespace_count", sizeof("disabled_namespace_count") - 1));
		xvoTableSetInt(objRet, "bus_ttl_required_namespace_count", sizeof("bus_ttl_required_namespace_count") - 1, xvoTableGetInt(objBus, "ttl_required_namespace_count", sizeof("ttl_required_namespace_count") - 1));
		xvoTableSetInt(objRet, "bus_tag_required_namespace_count", sizeof("bus_tag_required_namespace_count") - 1, xvoTableGetInt(objBus, "tag_required_namespace_count", sizeof("tag_required_namespace_count") - 1));
		xvoTableSetText(objRet, "bus_readonly_namespaces", sizeof("bus_readonly_namespaces") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "readonly_namespaces", sizeof("readonly_namespaces") - 1)), 0, FALSE);
		xvoTableSetText(objRet, "bus_disabled_namespaces", sizeof("bus_disabled_namespaces") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "disabled_namespaces", sizeof("disabled_namespaces") - 1)), 0, FALSE);
		xvoTableSetText(objRet, "bus_ttl_required_namespaces", sizeof("bus_ttl_required_namespaces") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "ttl_required_namespaces", sizeof("ttl_required_namespaces") - 1)), 0, FALSE);
		xvoTableSetText(objRet, "bus_tag_required_namespaces", sizeof("bus_tag_required_namespaces") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "tag_required_namespaces", sizeof("tag_required_namespaces") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_data_limit_reject_count", sizeof("bus_data_limit_reject_count") - 1, xvoTableGetInt(objBus, "data_limit_reject_count", sizeof("data_limit_reject_count") - 1));
		xvoTableSetText(objRet, "bus_last_data_limit_reject_time", sizeof("bus_last_data_limit_reject_time") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_data_limit_reject_time", sizeof("last_data_limit_reject_time") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_data_limit_reject_age_ms", sizeof("bus_last_data_limit_reject_age_ms") - 1, xvoTableGetInt(objBus, "last_data_limit_reject_age_ms", sizeof("last_data_limit_reject_age_ms") - 1));
		xvoTableSetInt(objRet, "bus_last_data_limit_count", sizeof("bus_last_data_limit_count") - 1, xvoTableGetInt(objBus, "last_data_limit_count", sizeof("last_data_limit_count") - 1));
		xvoTableSetInt(objRet, "bus_queue_limit_reject_count", sizeof("bus_queue_limit_reject_count") - 1, xvoTableGetInt(objBus, "queue_limit_reject_count", sizeof("queue_limit_reject_count") - 1));
		xvoTableSetText(objRet, "bus_last_queue_limit_reject_time", sizeof("bus_last_queue_limit_reject_time") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_queue_limit_reject_time", sizeof("last_queue_limit_reject_time") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_queue_limit_reject_age_ms", sizeof("bus_last_queue_limit_reject_age_ms") - 1, xvoTableGetInt(objBus, "last_queue_limit_reject_age_ms", sizeof("last_queue_limit_reject_age_ms") - 1));
		xvoTableSetInt(objRet, "bus_last_queue_limit_count", sizeof("bus_last_queue_limit_count") - 1, xvoTableGetInt(objBus, "last_queue_limit_count", sizeof("last_queue_limit_count") - 1));
		xvoTableSetInt(objRet, "bus_namespace_limit_reject_count", sizeof("bus_namespace_limit_reject_count") - 1, xvoTableGetInt(objBus, "namespace_limit_reject_count", sizeof("namespace_limit_reject_count") - 1));
		xvoTableSetText(objRet, "bus_last_namespace_limit_reject_time", sizeof("bus_last_namespace_limit_reject_time") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_namespace_limit_reject_time", sizeof("last_namespace_limit_reject_time") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_namespace_limit_reject_age_ms", sizeof("bus_last_namespace_limit_reject_age_ms") - 1, xvoTableGetInt(objBus, "last_namespace_limit_reject_age_ms", sizeof("last_namespace_limit_reject_age_ms") - 1));
		xvoTableSetInt(objRet, "bus_last_namespace_limit_count", sizeof("bus_last_namespace_limit_count") - 1, xvoTableGetInt(objBus, "last_namespace_limit_count", sizeof("last_namespace_limit_count") - 1));
		xvoTableSetText(objRet, "bus_last_namespace_limit_namespace", sizeof("bus_last_namespace_limit_namespace") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_namespace_limit_namespace", sizeof("last_namespace_limit_namespace") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_namespace_data_limit_reject_count", sizeof("bus_namespace_data_limit_reject_count") - 1, xvoTableGetInt(objBus, "namespace_data_limit_reject_count", sizeof("namespace_data_limit_reject_count") - 1));
		xvoTableSetText(objRet, "bus_last_namespace_data_limit_reject_time", sizeof("bus_last_namespace_data_limit_reject_time") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_namespace_data_limit_reject_time", sizeof("last_namespace_data_limit_reject_time") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_namespace_data_limit_reject_age_ms", sizeof("bus_last_namespace_data_limit_reject_age_ms") - 1, xvoTableGetInt(objBus, "last_namespace_data_limit_reject_age_ms", sizeof("last_namespace_data_limit_reject_age_ms") - 1));
		xvoTableSetInt(objRet, "bus_last_namespace_data_limit_count", sizeof("bus_last_namespace_data_limit_count") - 1, xvoTableGetInt(objBus, "last_namespace_data_limit_count", sizeof("last_namespace_data_limit_count") - 1));
		xvoTableSetText(objRet, "bus_last_namespace_data_limit_namespace", sizeof("bus_last_namespace_data_limit_namespace") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_namespace_data_limit_namespace", sizeof("last_namespace_data_limit_namespace") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_readonly_namespace_reject_count", sizeof("bus_readonly_namespace_reject_count") - 1, xvoTableGetInt(objBus, "readonly_namespace_reject_count", sizeof("readonly_namespace_reject_count") - 1));
		xvoTableSetText(objRet, "bus_last_readonly_namespace_reject_time", sizeof("bus_last_readonly_namespace_reject_time") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_readonly_namespace_reject_time", sizeof("last_readonly_namespace_reject_time") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_readonly_namespace_reject_age_ms", sizeof("bus_last_readonly_namespace_reject_age_ms") - 1, xvoTableGetInt(objBus, "last_readonly_namespace_reject_age_ms", sizeof("last_readonly_namespace_reject_age_ms") - 1));
		xvoTableSetText(objRet, "bus_last_readonly_namespace", sizeof("bus_last_readonly_namespace") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_readonly_namespace", sizeof("last_readonly_namespace") - 1)), 0, FALSE);
		xvoTableSetText(objRet, "bus_last_readonly_namespace_action", sizeof("bus_last_readonly_namespace_action") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_readonly_namespace_action", sizeof("last_readonly_namespace_action") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_disabled_namespace_reject_count", sizeof("bus_disabled_namespace_reject_count") - 1, xvoTableGetInt(objBus, "disabled_namespace_reject_count", sizeof("disabled_namespace_reject_count") - 1));
		xvoTableSetText(objRet, "bus_last_disabled_namespace_reject_time", sizeof("bus_last_disabled_namespace_reject_time") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_disabled_namespace_reject_time", sizeof("last_disabled_namespace_reject_time") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_disabled_namespace_reject_age_ms", sizeof("bus_last_disabled_namespace_reject_age_ms") - 1, xvoTableGetInt(objBus, "last_disabled_namespace_reject_age_ms", sizeof("last_disabled_namespace_reject_age_ms") - 1));
		xvoTableSetText(objRet, "bus_last_disabled_namespace", sizeof("bus_last_disabled_namespace") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_disabled_namespace", sizeof("last_disabled_namespace") - 1)), 0, FALSE);
		xvoTableSetText(objRet, "bus_last_disabled_namespace_action", sizeof("bus_last_disabled_namespace_action") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_disabled_namespace_action", sizeof("last_disabled_namespace_action") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_ttl_required_namespace_reject_count", sizeof("bus_ttl_required_namespace_reject_count") - 1, xvoTableGetInt(objBus, "ttl_required_namespace_reject_count", sizeof("ttl_required_namespace_reject_count") - 1));
		xvoTableSetText(objRet, "bus_last_ttl_required_namespace_reject_time", sizeof("bus_last_ttl_required_namespace_reject_time") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_ttl_required_namespace_reject_time", sizeof("last_ttl_required_namespace_reject_time") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_ttl_required_namespace_reject_age_ms", sizeof("bus_last_ttl_required_namespace_reject_age_ms") - 1, xvoTableGetInt(objBus, "last_ttl_required_namespace_reject_age_ms", sizeof("last_ttl_required_namespace_reject_age_ms") - 1));
		xvoTableSetText(objRet, "bus_last_ttl_required_namespace", sizeof("bus_last_ttl_required_namespace") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_ttl_required_namespace", sizeof("last_ttl_required_namespace") - 1)), 0, FALSE);
		xvoTableSetText(objRet, "bus_last_ttl_required_namespace_action", sizeof("bus_last_ttl_required_namespace_action") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_ttl_required_namespace_action", sizeof("last_ttl_required_namespace_action") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_tag_required_namespace_reject_count", sizeof("bus_tag_required_namespace_reject_count") - 1, xvoTableGetInt(objBus, "tag_required_namespace_reject_count", sizeof("tag_required_namespace_reject_count") - 1));
		xvoTableSetText(objRet, "bus_last_tag_required_namespace_reject_time", sizeof("bus_last_tag_required_namespace_reject_time") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_tag_required_namespace_reject_time", sizeof("last_tag_required_namespace_reject_time") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_tag_required_namespace_reject_age_ms", sizeof("bus_last_tag_required_namespace_reject_age_ms") - 1, xvoTableGetInt(objBus, "last_tag_required_namespace_reject_age_ms", sizeof("last_tag_required_namespace_reject_age_ms") - 1));
		xvoTableSetText(objRet, "bus_last_tag_required_namespace", sizeof("bus_last_tag_required_namespace") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_tag_required_namespace", sizeof("last_tag_required_namespace") - 1)), 0, FALSE);
		xvoTableSetText(objRet, "bus_last_tag_required_namespace_action", sizeof("bus_last_tag_required_namespace_action") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_tag_required_namespace_action", sizeof("last_tag_required_namespace_action") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_total_queued", 16, xvoTableGetInt(objBus, "total_queued", 12));
		xvoTableSetInt(objRet, "bus_total_delivered", 19, xvoTableGetInt(objBus, "total_delivered", 15));
		xvoTableSetInt(objRet, "bus_total_dropped", 17, xvoTableGetInt(objBus, "total_dropped", 13));
		xvoTableSetInt(objRet, "bus_last_queue_time", 19, xvoTableGetInt(objBus, "last_queue_time", 15));
		xvoTableSetText(objRet, "bus_last_queue_time_text", 24, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_queue_time_text", 20)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_dispatch_time", 22, xvoTableGetInt(objBus, "last_dispatch_time", 18));
		xvoTableSetText(objRet, "bus_last_dispatch_time_text", 27, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_dispatch_time_text", 23)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_sweep_interval_ms", sizeof("bus_sweep_interval_ms") - 1, xvoTableGetInt(objBus, "sweep_interval_ms", sizeof("sweep_interval_ms") - 1));
		xvoTableSetInt(objRet, "bus_sweep_batch_limit", sizeof("bus_sweep_batch_limit") - 1, xvoTableGetInt(objBus, "sweep_batch_limit", sizeof("sweep_batch_limit") - 1));
		xvoTableSetInt(objRet, "bus_sweep_count", sizeof("bus_sweep_count") - 1, xvoTableGetInt(objBus, "sweep_count", sizeof("sweep_count") - 1));
		xvoTableSetInt(objRet, "bus_sweep_removed_count", sizeof("bus_sweep_removed_count") - 1, xvoTableGetInt(objBus, "sweep_removed_count", sizeof("sweep_removed_count") - 1));
		xvoTableSetInt(objRet, "bus_last_sweep_time", sizeof("bus_last_sweep_time") - 1, xvoTableGetInt(objBus, "last_sweep_time", sizeof("last_sweep_time") - 1));
		xvoTableSetText(objRet, "bus_last_sweep_time_text", sizeof("bus_last_sweep_time_text") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_sweep_time_text", sizeof("last_sweep_time_text") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_sweep_age_ms", sizeof("bus_last_sweep_age_ms") - 1, xvoTableGetInt(objBus, "last_sweep_age_ms", sizeof("last_sweep_age_ms") - 1));
		xvoTableSetInt(objRet, "bus_last_sweep_removed", sizeof("bus_last_sweep_removed") - 1, xvoTableGetInt(objBus, "last_sweep_removed", sizeof("last_sweep_removed") - 1));
		xvoTableSetInt(objRet, "bus_last_sweep_remain", sizeof("bus_last_sweep_remain") - 1, xvoTableGetInt(objBus, "last_sweep_remain", sizeof("last_sweep_remain") - 1));
		xvoTableSetInt(objRet, "bus_cleanup_count", sizeof("bus_cleanup_count") - 1, xvoTableGetInt(objBus, "cleanup_count", sizeof("cleanup_count") - 1));
		xvoTableSetInt(objRet, "bus_last_cleanup_time", sizeof("bus_last_cleanup_time") - 1, xvoTableGetInt(objBus, "last_cleanup_time", sizeof("last_cleanup_time") - 1));
		xvoTableSetText(objRet, "bus_last_cleanup_time_text", sizeof("bus_last_cleanup_time_text") - 1, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_cleanup_time_text", sizeof("last_cleanup_time_text") - 1)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_cleanup_age_ms", sizeof("bus_last_cleanup_age_ms") - 1, xvoTableGetInt(objBus, "last_cleanup_age_ms", sizeof("last_cleanup_age_ms") - 1));
		xvoTableSetInt(objRet, "bus_last_cleanup_removed", sizeof("bus_last_cleanup_removed") - 1, xvoTableGetInt(objBus, "last_cleanup_removed", sizeof("last_cleanup_removed") - 1));
		xvoTableSetInt(objRet, "bus_last_cleanup_remain", sizeof("bus_last_cleanup_remain") - 1, xvoTableGetInt(objBus, "last_cleanup_remain", sizeof("last_cleanup_remain") - 1));
		xvoUnref(objBus);
	} else {
		xvoTableSetInt(objRet, "bus_queue_count", 15, 0);
		xvoTableSetInt(objRet, "bus_data_count", 14, 0);
		xvoTableSetInt(objRet, "bus_namespace_count", sizeof("bus_namespace_count") - 1, 0);
		xvoTableSetInt(objRet, "bus_namespace_item_count", sizeof("bus_namespace_item_count") - 1, 0);
		xvoTableSetInt(objRet, "bus_data_limit", sizeof("bus_data_limit") - 1, 0);
		xvoTableSetInt(objRet, "bus_queue_limit", sizeof("bus_queue_limit") - 1, 0);
		xvoTableSetInt(objRet, "bus_namespace_limit", sizeof("bus_namespace_limit") - 1, 0);
		xvoTableSetInt(objRet, "bus_namespace_data_limit", sizeof("bus_namespace_data_limit") - 1, 0);
		xvoTableSetInt(objRet, "bus_namespace_limit_remaining", sizeof("bus_namespace_limit_remaining") - 1, 0);
		xvoTableSetBool(objRet, "bus_namespace_limit_reached", sizeof("bus_namespace_limit_reached") - 1, FALSE);
		xvoTableSetInt(objRet, "bus_readonly_namespace_count", sizeof("bus_readonly_namespace_count") - 1, 0);
		xvoTableSetInt(objRet, "bus_disabled_namespace_count", sizeof("bus_disabled_namespace_count") - 1, 0);
		xvoTableSetInt(objRet, "bus_ttl_required_namespace_count", sizeof("bus_ttl_required_namespace_count") - 1, 0);
		xvoTableSetInt(objRet, "bus_tag_required_namespace_count", sizeof("bus_tag_required_namespace_count") - 1, 0);
		xvoTableSetText(objRet, "bus_readonly_namespaces", sizeof("bus_readonly_namespaces") - 1, "", 0, FALSE);
		xvoTableSetText(objRet, "bus_disabled_namespaces", sizeof("bus_disabled_namespaces") - 1, "", 0, FALSE);
		xvoTableSetText(objRet, "bus_ttl_required_namespaces", sizeof("bus_ttl_required_namespaces") - 1, "", 0, FALSE);
		xvoTableSetText(objRet, "bus_tag_required_namespaces", sizeof("bus_tag_required_namespaces") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_data_limit_reject_count", sizeof("bus_data_limit_reject_count") - 1, 0);
		xvoTableSetText(objRet, "bus_last_data_limit_reject_time", sizeof("bus_last_data_limit_reject_time") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_data_limit_reject_age_ms", sizeof("bus_last_data_limit_reject_age_ms") - 1, -1);
		xvoTableSetInt(objRet, "bus_last_data_limit_count", sizeof("bus_last_data_limit_count") - 1, 0);
		xvoTableSetInt(objRet, "bus_queue_limit_reject_count", sizeof("bus_queue_limit_reject_count") - 1, 0);
		xvoTableSetText(objRet, "bus_last_queue_limit_reject_time", sizeof("bus_last_queue_limit_reject_time") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_queue_limit_reject_age_ms", sizeof("bus_last_queue_limit_reject_age_ms") - 1, -1);
		xvoTableSetInt(objRet, "bus_last_queue_limit_count", sizeof("bus_last_queue_limit_count") - 1, 0);
		xvoTableSetInt(objRet, "bus_namespace_limit_reject_count", sizeof("bus_namespace_limit_reject_count") - 1, 0);
		xvoTableSetText(objRet, "bus_last_namespace_limit_reject_time", sizeof("bus_last_namespace_limit_reject_time") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_namespace_limit_reject_age_ms", sizeof("bus_last_namespace_limit_reject_age_ms") - 1, -1);
		xvoTableSetInt(objRet, "bus_last_namespace_limit_count", sizeof("bus_last_namespace_limit_count") - 1, 0);
		xvoTableSetText(objRet, "bus_last_namespace_limit_namespace", sizeof("bus_last_namespace_limit_namespace") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_namespace_data_limit_reject_count", sizeof("bus_namespace_data_limit_reject_count") - 1, 0);
		xvoTableSetText(objRet, "bus_last_namespace_data_limit_reject_time", sizeof("bus_last_namespace_data_limit_reject_time") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_namespace_data_limit_reject_age_ms", sizeof("bus_last_namespace_data_limit_reject_age_ms") - 1, -1);
		xvoTableSetInt(objRet, "bus_last_namespace_data_limit_count", sizeof("bus_last_namespace_data_limit_count") - 1, 0);
		xvoTableSetText(objRet, "bus_last_namespace_data_limit_namespace", sizeof("bus_last_namespace_data_limit_namespace") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_readonly_namespace_reject_count", sizeof("bus_readonly_namespace_reject_count") - 1, 0);
		xvoTableSetText(objRet, "bus_last_readonly_namespace_reject_time", sizeof("bus_last_readonly_namespace_reject_time") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_readonly_namespace_reject_age_ms", sizeof("bus_last_readonly_namespace_reject_age_ms") - 1, -1);
		xvoTableSetText(objRet, "bus_last_readonly_namespace", sizeof("bus_last_readonly_namespace") - 1, "", 0, FALSE);
		xvoTableSetText(objRet, "bus_last_readonly_namespace_action", sizeof("bus_last_readonly_namespace_action") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_disabled_namespace_reject_count", sizeof("bus_disabled_namespace_reject_count") - 1, 0);
		xvoTableSetText(objRet, "bus_last_disabled_namespace_reject_time", sizeof("bus_last_disabled_namespace_reject_time") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_disabled_namespace_reject_age_ms", sizeof("bus_last_disabled_namespace_reject_age_ms") - 1, -1);
		xvoTableSetText(objRet, "bus_last_disabled_namespace", sizeof("bus_last_disabled_namespace") - 1, "", 0, FALSE);
		xvoTableSetText(objRet, "bus_last_disabled_namespace_action", sizeof("bus_last_disabled_namespace_action") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_ttl_required_namespace_reject_count", sizeof("bus_ttl_required_namespace_reject_count") - 1, 0);
		xvoTableSetText(objRet, "bus_last_ttl_required_namespace_reject_time", sizeof("bus_last_ttl_required_namespace_reject_time") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_ttl_required_namespace_reject_age_ms", sizeof("bus_last_ttl_required_namespace_reject_age_ms") - 1, -1);
		xvoTableSetText(objRet, "bus_last_ttl_required_namespace", sizeof("bus_last_ttl_required_namespace") - 1, "", 0, FALSE);
		xvoTableSetText(objRet, "bus_last_ttl_required_namespace_action", sizeof("bus_last_ttl_required_namespace_action") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_tag_required_namespace_reject_count", sizeof("bus_tag_required_namespace_reject_count") - 1, 0);
		xvoTableSetText(objRet, "bus_last_tag_required_namespace_reject_time", sizeof("bus_last_tag_required_namespace_reject_time") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_tag_required_namespace_reject_age_ms", sizeof("bus_last_tag_required_namespace_reject_age_ms") - 1, -1);
		xvoTableSetText(objRet, "bus_last_tag_required_namespace", sizeof("bus_last_tag_required_namespace") - 1, "", 0, FALSE);
		xvoTableSetText(objRet, "bus_last_tag_required_namespace_action", sizeof("bus_last_tag_required_namespace_action") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_total_queued", 16, 0);
		xvoTableSetInt(objRet, "bus_total_delivered", 19, 0);
		xvoTableSetInt(objRet, "bus_total_dropped", 17, 0);
		xvoTableSetInt(objRet, "bus_last_queue_time", 19, 0);
		xvoTableSetText(objRet, "bus_last_queue_time_text", 24, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_dispatch_time", 22, 0);
		xvoTableSetText(objRet, "bus_last_dispatch_time_text", 27, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_sweep_interval_ms", sizeof("bus_sweep_interval_ms") - 1, 0);
		xvoTableSetInt(objRet, "bus_sweep_batch_limit", sizeof("bus_sweep_batch_limit") - 1, 0);
		xvoTableSetInt(objRet, "bus_sweep_count", sizeof("bus_sweep_count") - 1, 0);
		xvoTableSetInt(objRet, "bus_sweep_removed_count", sizeof("bus_sweep_removed_count") - 1, 0);
		xvoTableSetInt(objRet, "bus_last_sweep_time", sizeof("bus_last_sweep_time") - 1, 0);
		xvoTableSetText(objRet, "bus_last_sweep_time_text", sizeof("bus_last_sweep_time_text") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_sweep_age_ms", sizeof("bus_last_sweep_age_ms") - 1, -1);
		xvoTableSetInt(objRet, "bus_last_sweep_removed", sizeof("bus_last_sweep_removed") - 1, 0);
		xvoTableSetInt(objRet, "bus_last_sweep_remain", sizeof("bus_last_sweep_remain") - 1, 0);
		xvoTableSetInt(objRet, "bus_cleanup_count", sizeof("bus_cleanup_count") - 1, 0);
		xvoTableSetInt(objRet, "bus_last_cleanup_time", sizeof("bus_last_cleanup_time") - 1, 0);
		xvoTableSetText(objRet, "bus_last_cleanup_time_text", sizeof("bus_last_cleanup_time_text") - 1, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_cleanup_age_ms", sizeof("bus_last_cleanup_age_ms") - 1, -1);
		xvoTableSetInt(objRet, "bus_last_cleanup_removed", sizeof("bus_last_cleanup_removed") - 1, 0);
		xvoTableSetInt(objRet, "bus_last_cleanup_remain", sizeof("bus_last_cleanup_remain") - 1, 0);
	}

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	if ( sCheckTime ) {
		xrtFree(sCheckTime);
	}
	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
	}
	if ( sHttpLastIdleCloseTime ) {
		xrtFree(sHttpLastIdleCloseTime);
	}
	if ( sHttpLastConnLimitCloseTime ) {
		xrtFree(sHttpLastConnLimitCloseTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "health json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleMetrics(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[6144];
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sHttpLastIdleCloseTime;
	char* sHttpLastConnLimitCloseTime;
	char* sHttpLastRejectTime;
	char* sHttpLastStopCleanupTime;
	char* sHttpLastHeaderLimitRejectTime;
	char* sHttpLastBodyLimitRejectTime;
	char* sHttpLastPathLimitRejectTime;
	char* sHttpLastApiDisabledRejectTime;
	char* sHttpLastMethodRejectTime;
	char* sHttpLastHostNotFoundRejectTime;
	char* sHttpLastReloadBusyRejectTime;
	char* sHttpLastReloadFailedRejectTime;
	char* sHttpLastCheckConfigFailedRejectTime;
	char* sHttpLastBusBadRequestRejectTime;
	char* sHttpLastBusNotFoundRejectTime;
	char* sHttpLastBusLimitRejectTime;
	char* sHttpLastBusFailedRejectTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/http_metrics") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "http metrics api disabled");
	}

	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sHttpLastIdleCloseTime = XS_HttpLastIdleCloseTimeText();
	sHttpLastConnLimitCloseTime = XS_HttpLastConnLimitCloseTimeText();
	sHttpLastRejectTime = XS_HttpLastRejectTimeText();
	sHttpLastStopCleanupTime = XS_HttpLastStopCleanupTimeText();
	sHttpLastHeaderLimitRejectTime = XS_HttpLastHeaderLimitRejectTimeText();
	sHttpLastBodyLimitRejectTime = XS_HttpLastBodyLimitRejectTimeText();
	sHttpLastPathLimitRejectTime = XS_HttpLastPathLimitRejectTimeText();
	sHttpLastApiDisabledRejectTime = XS_HttpLastApiDisabledRejectTimeText();
	sHttpLastMethodRejectTime = XS_HttpLastMethodRejectTimeText();
	sHttpLastHostNotFoundRejectTime = XS_HttpLastHostNotFoundRejectTimeText();
	sHttpLastReloadBusyRejectTime = XS_HttpLastReloadBusyRejectTimeText();
	sHttpLastReloadFailedRejectTime = XS_HttpLastReloadFailedRejectTimeText();
	sHttpLastCheckConfigFailedRejectTime = XS_HttpLastCheckConfigFailedRejectTimeText();
	sHttpLastBusBadRequestRejectTime = XS_HttpLastBusBadRequestRejectTimeText();
	sHttpLastBusNotFoundRejectTime = XS_HttpLastBusNotFoundRejectTimeText();
	sHttpLastBusLimitRejectTime = XS_HttpLastBusLimitRejectTimeText();
	sHttpLastBusFailedRejectTime = XS_HttpLastBusFailedRejectTimeText();
	snprintf(
		sBody,
		sizeof(sBody),
		"http_req_count=%lld\nhttp_manage_req_count=%lld\nhttp_app_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_conn_current=%lld\nhttp_conn_peak=%lld\nhttp_idle_close_count=%lld\nhttp_conn_limit_close_count=%lld\nhttp_get_count=%lld\nhttp_post_count=%lld\nhttp_head_count=%lld\nhttp_other_count=%lld\nhttp_time_total_ms=%lld\nhttp_time_max_ms=%lld\nhttp_time_avg_ms=%lld\nhttp_last_method=%s\nhttp_last_status=%lld\nhttp_last_path=%s\nhttp_last_target=%s\nhttp_last_remote=%s\nhttp_last_host=%s\nhttp_last_user_agent=%s\nhttp_last_referer=%s\nhttp_last_origin=%s\nhttp_last_accept=%s\nhttp_last_accept_encoding=%s\nhttp_last_content_type=%s\nhttp_last_header_count=%lld\nhttp_last_query_len=%lld\nhttp_last_body_len=%lld\nhttp_last_time=%s\nhttp_last_age_ms=%lld\nhttp_last_duration_ms=%lld\nhttp_last_idle_close_time=%s\nhttp_last_idle_close_age_ms=%lld\nhttp_last_conn_limit_close_time=%s\nhttp_last_conn_limit_close_age_ms=%lld\nhttp_last_app_method=%s\nhttp_last_app_status=%lld\nhttp_last_app_path=%s\nhttp_last_app_target=%s\nhttp_last_app_remote=%s\nhttp_last_app_host=%s\nhttp_last_app_user_agent=%s\nhttp_last_app_referer=%s\nhttp_last_app_origin=%s\nhttp_last_app_accept=%s\nhttp_last_app_accept_encoding=%s\nhttp_last_app_content_type=%s\nhttp_last_app_header_count=%lld\nhttp_last_app_query_len=%lld\nhttp_last_app_body_len=%lld\nhttp_last_app_time=%s\nhttp_last_app_age_ms=%lld\nhttp_last_app_duration_ms=%lld\n",
		(long long)XS_HttpMetricGet(&g_iXsHttpReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpManageReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpAppReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp2xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp3xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp4xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp5xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsHttpIdleCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnLimitCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodGetCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodPostCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodHeadCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodOtherCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpTimeTotalMS),
		(long long)XS_HttpMetricGet(&g_iXsHttpTimeMaxMS),
		(long long)((XS_HttpMetricGet(&g_iXsHttpReqCount) > 0) ? (XS_HttpMetricGet(&g_iXsHttpTimeTotalMS) / XS_HttpMetricGet(&g_iXsHttpReqCount)) : 0),
		XS_HttpLastMethodName()[0] ? XS_HttpLastMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastStatusCode),
		XS_HttpLastPath()[0] ? XS_HttpLastPath() : "(none)",
		XS_HttpLastTarget()[0] ? XS_HttpLastTarget() : "(none)",
		XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : "(none)",
		g_sXsHttpLastHost[0] ? g_sXsHttpLastHost : "(none)",
		g_sXsHttpLastUserAgent[0] ? g_sXsHttpLastUserAgent : "(none)",
		g_sXsHttpLastReferer[0] ? g_sXsHttpLastReferer : "(none)",
		g_sXsHttpLastOrigin[0] ? g_sXsHttpLastOrigin : "(none)",
		g_sXsHttpLastAccept[0] ? g_sXsHttpLastAccept : "(none)",
		g_sXsHttpLastAcceptEncoding[0] ? g_sXsHttpLastAcceptEncoding : "(none)",
		g_sXsHttpLastContentType[0] ? g_sXsHttpLastContentType : "(none)",
		(long long)g_iXsHttpLastHeaderCount,
		(long long)g_iXsHttpLastQueryLen,
		(long long)g_iXsHttpLastBodyLen,
		sHttpLastTime ? sHttpLastTime : "(none)",
		(long long)XS_HttpLastRequestAgeMS(),
		(long long)g_iXsHttpLastTimeMS,
		sHttpLastIdleCloseTime ? sHttpLastIdleCloseTime : "(none)",
		(long long)XS_HttpLastIdleCloseAgeMS(),
		sHttpLastConnLimitCloseTime ? sHttpLastConnLimitCloseTime : "(none)",
		(long long)XS_HttpLastConnLimitCloseAgeMS(),
		XS_HttpLastAppMethodName()[0] ? XS_HttpLastAppMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode),
		XS_HttpLastAppPath()[0] ? XS_HttpLastAppPath() : "(none)",
		XS_HttpLastAppTarget()[0] ? XS_HttpLastAppTarget() : "(none)",
		XS_HttpLastAppRemote()[0] ? XS_HttpLastAppRemote() : "(none)",
		g_sXsHttpLastAppHost[0] ? g_sXsHttpLastAppHost : "(none)",
		g_sXsHttpLastAppUserAgent[0] ? g_sXsHttpLastAppUserAgent : "(none)",
		g_sXsHttpLastAppReferer[0] ? g_sXsHttpLastAppReferer : "(none)",
		g_sXsHttpLastAppOrigin[0] ? g_sXsHttpLastAppOrigin : "(none)",
		g_sXsHttpLastAppAccept[0] ? g_sXsHttpLastAppAccept : "(none)",
		g_sXsHttpLastAppAcceptEncoding[0] ? g_sXsHttpLastAppAcceptEncoding : "(none)",
		g_sXsHttpLastAppContentType[0] ? g_sXsHttpLastAppContentType : "(none)",
		(long long)g_iXsHttpLastAppHeaderCount,
		(long long)g_iXsHttpLastAppQueryLen,
		(long long)g_iXsHttpLastAppBodyLen,
		sHttpLastAppTime ? sHttpLastAppTime : "(none)",
		(long long)XS_HttpLastAppRequestAgeMS(),
		(long long)g_iXsHttpLastAppTimeMS
	);
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_reject_count=%lld\nhttp_last_reject_status=%lld\nhttp_last_reject_reason=%s\nhttp_last_reject_time=%s\nhttp_last_reject_age_ms=%lld\nhttp_stop_cleanup_count=%lld\nhttp_last_stop_cleanup_closed=%lld\nhttp_last_stop_cleanup_remain=%lld\nhttp_last_stop_cleanup_time=%s\nhttp_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastRejectStatus),
			g_sXsHttpLastRejectReason[0] ? g_sXsHttpLastRejectReason : "(none)",
			sHttpLastRejectTime ? sHttpLastRejectTime : "(none)",
			(long long)XS_HttpLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastStopCleanupRemain),
			sHttpLastStopCleanupTime ? sHttpLastStopCleanupTime : "(none)",
			(long long)XS_HttpLastStopCleanupAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_header_limit_reject_count=%lld\nhttp_last_header_limit_reject_time=%s\nhttp_last_header_limit_reject_age_ms=%lld\nhttp_body_limit_reject_count=%lld\nhttp_last_body_limit_reject_time=%s\nhttp_last_body_limit_reject_age_ms=%lld\nhttp_path_limit_reject_count=%lld\nhttp_last_path_limit_reject_time=%s\nhttp_last_path_limit_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpHeaderLimitRejectCount),
			sHttpLastHeaderLimitRejectTime ? sHttpLastHeaderLimitRejectTime : "(none)",
			(long long)XS_HttpLastHeaderLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBodyLimitRejectCount),
			sHttpLastBodyLimitRejectTime ? sHttpLastBodyLimitRejectTime : "(none)",
			(long long)XS_HttpLastBodyLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpPathLimitRejectCount),
			sHttpLastPathLimitRejectTime ? sHttpLastPathLimitRejectTime : "(none)",
			(long long)XS_HttpLastPathLimitRejectAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_api_disabled_reject_count=%lld\nhttp_last_api_disabled_reject_time=%s\nhttp_last_api_disabled_reject_age_ms=%lld\nhttp_method_reject_count=%lld\nhttp_last_method_reject_time=%s\nhttp_last_method_reject_age_ms=%lld\nhttp_host_not_found_reject_count=%lld\nhttp_last_host_not_found_reject_time=%s\nhttp_last_host_not_found_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpApiDisabledRejectCount),
			sHttpLastApiDisabledRejectTime ? sHttpLastApiDisabledRejectTime : "(none)",
			(long long)XS_HttpLastApiDisabledRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodRejectCount),
			sHttpLastMethodRejectTime ? sHttpLastMethodRejectTime : "(none)",
			(long long)XS_HttpLastMethodRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpHostNotFoundRejectCount),
			sHttpLastHostNotFoundRejectTime ? sHttpLastHostNotFoundRejectTime : "(none)",
			(long long)XS_HttpLastHostNotFoundRejectAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_reload_busy_reject_count=%lld\nhttp_last_reload_busy_reject_time=%s\nhttp_last_reload_busy_reject_age_ms=%lld\nhttp_reload_failed_reject_count=%lld\nhttp_last_reload_failed_reject_time=%s\nhttp_last_reload_failed_reject_age_ms=%lld\nhttp_check_config_failed_reject_count=%lld\nhttp_last_check_config_failed_reject_time=%s\nhttp_last_check_config_failed_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpReloadBusyRejectCount),
			sHttpLastReloadBusyRejectTime ? sHttpLastReloadBusyRejectTime : "(none)",
			(long long)XS_HttpLastReloadBusyRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpReloadFailedRejectCount),
			sHttpLastReloadFailedRejectTime ? sHttpLastReloadFailedRejectTime : "(none)",
			(long long)XS_HttpLastReloadFailedRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpCheckConfigFailedRejectCount),
			sHttpLastCheckConfigFailedRejectTime ? sHttpLastCheckConfigFailedRejectTime : "(none)",
			(long long)XS_HttpLastCheckConfigFailedRejectAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_bus_bad_request_reject_count=%lld\nhttp_last_bus_bad_request_reject_time=%s\nhttp_last_bus_bad_request_reject_age_ms=%lld\nhttp_bus_not_found_reject_count=%lld\nhttp_last_bus_not_found_reject_time=%s\nhttp_last_bus_not_found_reject_age_ms=%lld\nhttp_bus_limit_reject_count=%lld\nhttp_last_bus_limit_reject_time=%s\nhttp_last_bus_limit_reject_age_ms=%lld\nhttp_bus_failed_reject_count=%lld\nhttp_last_bus_failed_reject_time=%s\nhttp_last_bus_failed_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpBusBadRequestRejectCount),
			sHttpLastBusBadRequestRejectTime ? sHttpLastBusBadRequestRejectTime : "(none)",
			(long long)XS_HttpLastBusBadRequestRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusNotFoundRejectCount),
			sHttpLastBusNotFoundRejectTime ? sHttpLastBusNotFoundRejectTime : "(none)",
			(long long)XS_HttpLastBusNotFoundRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusLimitRejectCount),
			sHttpLastBusLimitRejectTime ? sHttpLastBusLimitRejectTime : "(none)",
			(long long)XS_HttpLastBusLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusFailedRejectCount),
			sHttpLastBusFailedRejectTime ? sHttpLastBusFailedRejectTime : "(none)",
			(long long)XS_HttpLastBusFailedRejectAgeMS()
		);
	}
	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
	}
	if ( sHttpLastIdleCloseTime ) {
		xrtFree(sHttpLastIdleCloseTime);
	}
	if ( sHttpLastConnLimitCloseTime ) {
		xrtFree(sHttpLastConnLimitCloseTime);
	}
	if ( sHttpLastRejectTime ) {
		xrtFree(sHttpLastRejectTime);
	}
	if ( sHttpLastStopCleanupTime ) {
		xrtFree(sHttpLastStopCleanupTime);
	}
	if ( sHttpLastHeaderLimitRejectTime ) {
		xrtFree(sHttpLastHeaderLimitRejectTime);
	}
	if ( sHttpLastBodyLimitRejectTime ) {
		xrtFree(sHttpLastBodyLimitRejectTime);
	}
	if ( sHttpLastPathLimitRejectTime ) {
		xrtFree(sHttpLastPathLimitRejectTime);
	}
	if ( sHttpLastApiDisabledRejectTime ) {
		xrtFree(sHttpLastApiDisabledRejectTime);
	}
	if ( sHttpLastMethodRejectTime ) {
		xrtFree(sHttpLastMethodRejectTime);
	}
	if ( sHttpLastHostNotFoundRejectTime ) {
		xrtFree(sHttpLastHostNotFoundRejectTime);
	}
	if ( sHttpLastReloadBusyRejectTime ) {
		xrtFree(sHttpLastReloadBusyRejectTime);
	}
	if ( sHttpLastReloadFailedRejectTime ) {
		xrtFree(sHttpLastReloadFailedRejectTime);
	}
	if ( sHttpLastCheckConfigFailedRejectTime ) {
		xrtFree(sHttpLastCheckConfigFailedRejectTime);
	}
	if ( sHttpLastBusBadRequestRejectTime ) {
		xrtFree(sHttpLastBusBadRequestRejectTime);
	}
	if ( sHttpLastBusNotFoundRejectTime ) {
		xrtFree(sHttpLastBusNotFoundRejectTime);
	}
	if ( sHttpLastBusLimitRejectTime ) {
		xrtFree(sHttpLastBusLimitRejectTime);
	}
	if ( sHttpLastBusFailedRejectTime ) {
		xrtFree(sHttpLastBusFailedRejectTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleMetricsJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sHttpLastIdleCloseTime;
	char* sHttpLastConnLimitCloseTime;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/http_metrics_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "http metrics json api disabled");
	}

	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sHttpLastIdleCloseTime = XS_HttpLastIdleCloseTimeText();
	sHttpLastConnLimitCloseTime = XS_HttpLastConnLimitCloseTimeText();
	objRet = xvoCreateTable();
	xvoTableSetInt(objRet, "http_req_count", 14, XS_HttpMetricGet(&g_iXsHttpReqCount));
	xvoTableSetInt(objRet, "http_manage_req_count", 21, XS_HttpMetricGet(&g_iXsHttpManageReqCount));
	xvoTableSetInt(objRet, "http_app_req_count", 18, XS_HttpMetricGet(&g_iXsHttpAppReqCount));
	xvoTableSetInt(objRet, "http_2xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp2xxCount));
	xvoTableSetInt(objRet, "http_3xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp3xxCount));
	xvoTableSetInt(objRet, "http_4xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp4xxCount));
	xvoTableSetInt(objRet, "http_5xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp5xxCount));
	xvoTableSetInt(objRet, "http_conn_current", 17, XS_HttpMetricGet(&g_iXsHttpConnCurrent));
	xvoTableSetInt(objRet, "http_conn_peak", 14, XS_HttpMetricGet(&g_iXsHttpConnPeak));
	xvoTableSetInt(objRet, "http_idle_close_count", 21, XS_HttpMetricGet(&g_iXsHttpIdleCloseCount));
	xvoTableSetInt(objRet, "http_conn_limit_close_count", 27, XS_HttpMetricGet(&g_iXsHttpConnLimitCloseCount));
	XS_HttpAppendRequestLimitMetrics(objRet);
	XS_HttpAppendPolicyRejectMetrics(objRet);
	XS_HttpAppendManageRejectMetrics(objRet);
	XS_HttpAppendBusRejectMetrics(objRet);
	XS_HttpAppendRejectMetrics(objRet);
	XS_HttpAppendStopCleanupMetrics(objRet);
	xvoTableSetInt(objRet, "http_get_count", 14, XS_HttpMetricGet(&g_iXsHttpMethodGetCount));
	xvoTableSetInt(objRet, "http_post_count", 15, XS_HttpMetricGet(&g_iXsHttpMethodPostCount));
	xvoTableSetInt(objRet, "http_head_count", 15, XS_HttpMetricGet(&g_iXsHttpMethodHeadCount));
	xvoTableSetInt(objRet, "http_other_count", 16, XS_HttpMetricGet(&g_iXsHttpMethodOtherCount));
	xvoTableSetInt(objRet, "http_time_total_ms", 18, XS_HttpMetricGet(&g_iXsHttpTimeTotalMS));
	xvoTableSetInt(objRet, "http_time_max_ms", 16, XS_HttpMetricGet(&g_iXsHttpTimeMaxMS));
	xvoTableSetInt(
		objRet,
		"http_time_avg_ms",
		16,
		(XS_HttpMetricGet(&g_iXsHttpReqCount) > 0)
			? (XS_HttpMetricGet(&g_iXsHttpTimeTotalMS) / XS_HttpMetricGet(&g_iXsHttpReqCount))
			: 0
	);
	xvoTableSetInt(objRet, "http_last_status", 16, XS_HttpMetricGet(&g_iXsHttpLastStatusCode));
	xvoTableSetText(objRet, "http_last_method", 16, (ptr)XS_HttpLastMethodName(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_path", 14, (ptr)XS_HttpLastPath(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_target", 16, (ptr)XS_HttpLastTarget(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_remote", 16, (ptr)XS_HttpLastRemote(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_host", 14, (ptr)g_sXsHttpLastHost, 0, FALSE);
	xvoTableSetText(objRet, "http_last_user_agent", 20, (ptr)g_sXsHttpLastUserAgent, 0, FALSE);
	xvoTableSetText(objRet, "http_last_referer", 17, (ptr)g_sXsHttpLastReferer, 0, FALSE);
	xvoTableSetText(objRet, "http_last_origin", 16, (ptr)g_sXsHttpLastOrigin, 0, FALSE);
	xvoTableSetText(objRet, "http_last_accept", 16, (ptr)g_sXsHttpLastAccept, 0, FALSE);
	xvoTableSetText(objRet, "http_last_accept_encoding", 25, (ptr)g_sXsHttpLastAcceptEncoding, 0, FALSE);
	xvoTableSetText(objRet, "http_last_content_type", 22, (ptr)g_sXsHttpLastContentType, 0, FALSE);
	xvoTableSetInt(objRet, "http_last_header_count", 22, g_iXsHttpLastHeaderCount);
	xvoTableSetInt(objRet, "http_last_query_len", 19, g_iXsHttpLastQueryLen);
	xvoTableSetInt(objRet, "http_last_body_len", 18, g_iXsHttpLastBodyLen);
	xvoTableSetText(objRet, "http_last_time", 14, (ptr)(sHttpLastTime ? sHttpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_age_ms", 16, XS_HttpLastRequestAgeMS());
	xvoTableSetInt(objRet, "http_last_duration_ms", 21, g_iXsHttpLastTimeMS);
	xvoTableSetText(objRet, "http_last_idle_close_time", 25, (ptr)(sHttpLastIdleCloseTime ? sHttpLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_idle_close_age_ms", 27, XS_HttpLastIdleCloseAgeMS());
	xvoTableSetText(objRet, "http_last_conn_limit_close_time", 31, (ptr)(sHttpLastConnLimitCloseTime ? sHttpLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_conn_limit_close_age_ms", 33, XS_HttpLastConnLimitCloseAgeMS());
	xvoTableSetInt(objRet, "http_last_app_status", 20, XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode));
	xvoTableSetText(objRet, "http_last_app_method", 20, (ptr)XS_HttpLastAppMethodName(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_path", 18, (ptr)XS_HttpLastAppPath(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_target", 20, (ptr)XS_HttpLastAppTarget(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_remote", 20, (ptr)XS_HttpLastAppRemote(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_host", 18, (ptr)g_sXsHttpLastAppHost, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_user_agent", 24, (ptr)g_sXsHttpLastAppUserAgent, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_referer", 21, (ptr)g_sXsHttpLastAppReferer, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_origin", 20, (ptr)g_sXsHttpLastAppOrigin, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_accept", 20, (ptr)g_sXsHttpLastAppAccept, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_accept_encoding", 29, (ptr)g_sXsHttpLastAppAcceptEncoding, 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_content_type", 26, (ptr)g_sXsHttpLastAppContentType, 0, FALSE);
	xvoTableSetInt(objRet, "http_last_app_header_count", 26, g_iXsHttpLastAppHeaderCount);
	xvoTableSetInt(objRet, "http_last_app_query_len", 23, g_iXsHttpLastAppQueryLen);
	xvoTableSetInt(objRet, "http_last_app_body_len", 22, g_iXsHttpLastAppBodyLen);
	xvoTableSetText(objRet, "http_last_app_time", 18, (ptr)(sHttpLastAppTime ? sHttpLastAppTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_app_age_ms", 20, XS_HttpLastAppRequestAgeMS());
	xvoTableSetInt(objRet, "http_last_app_duration_ms", 25, g_iXsHttpLastAppTimeMS);

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
	}
	if ( sHttpLastIdleCloseTime ) {
		xrtFree(sHttpLastIdleCloseTime);
	}
	if ( sHttpLastConnLimitCloseTime ) {
		xrtFree(sHttpLastConnLimitCloseTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "http metrics json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleMetricsClear(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[4096];

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/http_metrics_clear") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "http metrics clear api disabled");
	}

	XS_HttpClearMetrics();
	snprintf(
		sBody,
		sizeof(sBody),
		"http_req_count=%lld\nhttp_manage_req_count=%lld\nhttp_app_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_conn_current=%lld\nhttp_conn_peak=%lld\nhttp_idle_close_count=%lld\nhttp_conn_limit_close_count=%lld\nhttp_reject_count=%lld\nhttp_stop_cleanup_count=%lld\nhttp_get_count=%lld\nhttp_post_count=%lld\nhttp_head_count=%lld\nhttp_other_count=%lld\nhttp_time_total_ms=%lld\nhttp_time_max_ms=%lld\nhttp_time_avg_ms=%lld\nhttp_last_method=\nhttp_last_status=%lld\nhttp_last_reject_status=0\nhttp_last_reject_reason=(none)\nhttp_last_stop_cleanup_closed=0\nhttp_last_stop_cleanup_remain=0\nhttp_last_stop_cleanup_time=(none)\nhttp_last_stop_cleanup_age_ms=-1\nhttp_last_path=(none)\nhttp_last_target=(none)\nhttp_last_remote=(none)\nhttp_last_host=(none)\nhttp_last_user_agent=(none)\nhttp_last_referer=(none)\nhttp_last_origin=(none)\nhttp_last_accept=(none)\nhttp_last_accept_encoding=(none)\nhttp_last_content_type=(none)\nhttp_last_header_count=0\nhttp_last_query_len=0\nhttp_last_body_len=0\nhttp_last_time=(none)\nhttp_last_age_ms=-1\nhttp_last_duration_ms=0\nhttp_last_idle_close_time=(none)\nhttp_last_idle_close_age_ms=-1\nhttp_last_conn_limit_close_time=(none)\nhttp_last_conn_limit_close_age_ms=-1\nhttp_last_reject_time=(none)\nhttp_last_reject_age_ms=-1\nhttp_last_app_method=\nhttp_last_app_status=0\nhttp_last_app_path=(none)\nhttp_last_app_target=(none)\nhttp_last_app_remote=(none)\nhttp_last_app_host=(none)\nhttp_last_app_user_agent=(none)\nhttp_last_app_referer=(none)\nhttp_last_app_origin=(none)\nhttp_last_app_accept=(none)\nhttp_last_app_accept_encoding=(none)\nhttp_last_app_content_type=(none)\nhttp_last_app_header_count=0\nhttp_last_app_query_len=0\nhttp_last_app_body_len=0\nhttp_last_app_time=(none)\nhttp_last_app_age_ms=-1\nhttp_last_app_duration_ms=0\n",
		(long long)XS_HttpMetricGet(&g_iXsHttpReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpManageReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpAppReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp2xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp3xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp4xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp5xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsHttpIdleCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnLimitCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpRejectCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpStopCleanupCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodGetCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodPostCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodHeadCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodOtherCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpTimeTotalMS),
		(long long)XS_HttpMetricGet(&g_iXsHttpTimeMaxMS),
		(long long)((XS_HttpMetricGet(&g_iXsHttpReqCount) > 0) ? (XS_HttpMetricGet(&g_iXsHttpTimeTotalMS) / XS_HttpMetricGet(&g_iXsHttpReqCount)) : 0),
		(long long)XS_HttpMetricGet(&g_iXsHttpLastStatusCode)
	);
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_header_limit_reject_count=0\nhttp_last_header_limit_reject_time=(none)\nhttp_last_header_limit_reject_age_ms=-1\nhttp_body_limit_reject_count=0\nhttp_last_body_limit_reject_time=(none)\nhttp_last_body_limit_reject_age_ms=-1\nhttp_path_limit_reject_count=0\nhttp_last_path_limit_reject_time=(none)\nhttp_last_path_limit_reject_age_ms=-1\n"
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_api_disabled_reject_count=0\nhttp_last_api_disabled_reject_time=(none)\nhttp_last_api_disabled_reject_age_ms=-1\nhttp_method_reject_count=0\nhttp_last_method_reject_time=(none)\nhttp_last_method_reject_age_ms=-1\nhttp_host_not_found_reject_count=0\nhttp_last_host_not_found_reject_time=(none)\nhttp_last_host_not_found_reject_age_ms=-1\n"
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_reload_busy_reject_count=0\nhttp_last_reload_busy_reject_time=(none)\nhttp_last_reload_busy_reject_age_ms=-1\nhttp_reload_failed_reject_count=0\nhttp_last_reload_failed_reject_time=(none)\nhttp_last_reload_failed_reject_age_ms=-1\nhttp_check_config_failed_reject_count=0\nhttp_last_check_config_failed_reject_time=(none)\nhttp_last_check_config_failed_reject_age_ms=-1\n"
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_bus_bad_request_reject_count=0\nhttp_last_bus_bad_request_reject_time=(none)\nhttp_last_bus_bad_request_reject_age_ms=-1\nhttp_bus_not_found_reject_count=0\nhttp_last_bus_not_found_reject_time=(none)\nhttp_last_bus_not_found_reject_age_ms=-1\nhttp_bus_limit_reject_count=0\nhttp_last_bus_limit_reject_time=(none)\nhttp_last_bus_limit_reject_age_ms=-1\nhttp_bus_failed_reject_count=0\nhttp_last_bus_failed_reject_time=(none)\nhttp_last_bus_failed_reject_age_ms=-1\n"
		);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleWsMetrics(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[2048];
	char* sLastTime;
	char* sLastCloseTime;
	char* sLastErrorTime;
	char* sLastInvalidTime;
	char* sLastIdleCloseTime;
	char* sLastConnLimitCloseTime;
	char* sLastMessageLimitCloseTime;
	char* sLastRejectTime;
	char* sLastStopCleanupTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/ws_metrics") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "ws metrics api disabled");
	}

	sLastTime = XS_WsLastTimeText();
	sLastCloseTime = XS_WsLastCloseTimeText();
	sLastErrorTime = XS_WsLastErrorTimeText();
	sLastInvalidTime = XS_WsLastInvalidTimeText();
	sLastIdleCloseTime = XS_WsLastIdleCloseTimeText();
	sLastConnLimitCloseTime = XS_WsLastConnLimitCloseTimeText();
	sLastMessageLimitCloseTime = XS_WsLastMessageLimitCloseTimeText();
	sLastRejectTime = XS_WsLastRejectTimeText();
	sLastStopCleanupTime = XS_WsLastStopCleanupTimeText();
	snprintf(
		sBody,
		sizeof(sBody),
		"ws_message_limit=%u\nws_conn_current=%lld\nws_conn_peak=%lld\nws_open_count=%lld\nws_close_count=%lld\nws_text_count=%lld\nws_binary_count=%lld\nws_ping_count=%lld\nws_pong_count=%lld\nws_error_count=%lld\nws_invalid_count=%lld\nws_idle_close_count=%lld\nws_conn_limit_close_count=%lld\nws_message_limit_close_count=%lld\nws_last_error_code=%lld\nws_last_invalid_reason=%s\nws_last_invalid_time=%s\nws_last_invalid_age_ms=%lld\nws_last_close_reason=%lld\nws_last_close_time=%s\nws_last_close_age_ms=%lld\nws_last_idle_close_time=%s\nws_last_idle_close_age_ms=%lld\nws_last_conn_limit_close_time=%s\nws_last_conn_limit_close_age_ms=%lld\nws_last_message_limit_close_time=%s\nws_last_message_limit_close_age_ms=%lld\nws_last_frame_type=%s\nws_last_remote=%s\nws_last_bytes=%lld\nws_last_text=%s\nws_last_time=%s\nws_last_age_ms=%lld\nws_last_error_time=%s\nws_last_error_age_ms=%lld\n",
		objServer->WsMessageLimit,
		(long long)XS_HttpMetricGet(&g_iXsWsConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsWsConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsWsOpenCount),
		(long long)XS_HttpMetricGet(&g_iXsWsCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsWsTextCount),
		(long long)XS_HttpMetricGet(&g_iXsWsBinaryCount),
		(long long)XS_HttpMetricGet(&g_iXsWsPingCount),
		(long long)XS_HttpMetricGet(&g_iXsWsPongCount),
		(long long)XS_HttpMetricGet(&g_iXsWsErrorCount),
		(long long)XS_HttpMetricGet(&g_iXsWsInvalidCount),
		(long long)XS_HttpMetricGet(&g_iXsWsIdleCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsWsConnLimitCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsWsMessageLimitCloseCount),
		(long long)g_iXsWsLastErrorCode,
		g_sXsWsLastInvalidReason[0] ? g_sXsWsLastInvalidReason : "(none)",
		sLastInvalidTime ? sLastInvalidTime : "(none)",
		(long long)XS_WsLastInvalidAgeMS(),
		(long long)g_iXsWsLastCloseReason,
		sLastCloseTime ? sLastCloseTime : "(none)",
		(long long)XS_WsLastCloseAgeMS(),
		sLastIdleCloseTime ? sLastIdleCloseTime : "(none)",
		(long long)XS_WsLastIdleCloseAgeMS(),
		sLastConnLimitCloseTime ? sLastConnLimitCloseTime : "(none)",
		(long long)XS_WsLastConnLimitCloseAgeMS(),
		sLastMessageLimitCloseTime ? sLastMessageLimitCloseTime : "(none)",
		(long long)XS_WsLastMessageLimitCloseAgeMS(),
		XS_WsLastFrameTypeName()[0] ? XS_WsLastFrameTypeName() : "(none)",
		g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsWsLastBytes),
		g_sXsWsLastText[0] ? g_sXsWsLastText : "(none)",
		sLastTime ? sLastTime : "(none)",
		(long long)XS_WsLastAgeMS(),
		sLastErrorTime ? sLastErrorTime : "(none)",
		(long long)XS_WsLastErrorAgeMS()
	);
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"ws_reject_count=%lld\nws_last_reject_reason=%s\nws_last_reject_time=%s\nws_last_reject_age_ms=%lld\nws_stop_cleanup_count=%lld\nws_last_stop_cleanup_closed=%lld\nws_last_stop_cleanup_remain=%lld\nws_last_stop_cleanup_time=%s\nws_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsWsRejectCount),
			g_sXsWsLastRejectReason[0] ? g_sXsWsLastRejectReason : "(none)",
			sLastRejectTime ? sLastRejectTime : "(none)",
			(long long)XS_WsLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsWsStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsWsLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsWsLastStopCleanupRemain),
			sLastStopCleanupTime ? sLastStopCleanupTime : "(none)",
			(long long)XS_WsLastStopCleanupAgeMS()
		);
	}
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastCloseTime ) {
		xrtFree(sLastCloseTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	if ( sLastInvalidTime ) {
		xrtFree(sLastInvalidTime);
	}
	if ( sLastIdleCloseTime ) {
		xrtFree(sLastIdleCloseTime);
	}
	if ( sLastConnLimitCloseTime ) {
		xrtFree(sLastConnLimitCloseTime);
	}
	if ( sLastMessageLimitCloseTime ) {
		xrtFree(sLastMessageLimitCloseTime);
	}
	if ( sLastRejectTime ) {
		xrtFree(sLastRejectTime);
	}
	if ( sLastStopCleanupTime ) {
		xrtFree(sLastStopCleanupTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleWsMetricsJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sLastTime;
	char* sLastCloseTime;
	char* sLastErrorTime;
	char* sLastInvalidTime;
	char* sLastIdleCloseTime;
	char* sLastConnLimitCloseTime;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/ws_metrics_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "ws metrics json api disabled");
	}

	sLastTime = XS_WsLastTimeText();
	sLastCloseTime = XS_WsLastCloseTimeText();
	sLastErrorTime = XS_WsLastErrorTimeText();
	sLastInvalidTime = XS_WsLastInvalidTimeText();
	sLastIdleCloseTime = XS_WsLastIdleCloseTimeText();
	sLastConnLimitCloseTime = XS_WsLastConnLimitCloseTimeText();
	objRet = xvoCreateTable();
	xvoTableSetInt(objRet, "ws_message_limit", 16, objServer->WsMessageLimit);
	xvoTableSetInt(objRet, "ws_conn_current", 15, XS_HttpMetricGet(&g_iXsWsConnCurrent));
	xvoTableSetInt(objRet, "ws_conn_peak", 12, XS_HttpMetricGet(&g_iXsWsConnPeak));
	xvoTableSetInt(objRet, "ws_open_count", 13, XS_HttpMetricGet(&g_iXsWsOpenCount));
	xvoTableSetInt(objRet, "ws_close_count", 14, XS_HttpMetricGet(&g_iXsWsCloseCount));
	xvoTableSetInt(objRet, "ws_text_count", 13, XS_HttpMetricGet(&g_iXsWsTextCount));
	xvoTableSetInt(objRet, "ws_binary_count", 15, XS_HttpMetricGet(&g_iXsWsBinaryCount));
	xvoTableSetInt(objRet, "ws_ping_count", 13, XS_HttpMetricGet(&g_iXsWsPingCount));
	xvoTableSetInt(objRet, "ws_pong_count", 13, XS_HttpMetricGet(&g_iXsWsPongCount));
	xvoTableSetInt(objRet, "ws_error_count", 14, XS_HttpMetricGet(&g_iXsWsErrorCount));
	xvoTableSetInt(objRet, "ws_invalid_count", 16, XS_HttpMetricGet(&g_iXsWsInvalidCount));
	xvoTableSetInt(objRet, "ws_idle_close_count", 19, XS_HttpMetricGet(&g_iXsWsIdleCloseCount));
	xvoTableSetInt(objRet, "ws_conn_limit_close_count", 25, XS_HttpMetricGet(&g_iXsWsConnLimitCloseCount));
	XS_WsAppendMessageLimitMetrics(objRet);
	XS_WsAppendRejectMetrics(objRet);
	XS_WsAppendStopCleanupMetrics(objRet);
	xvoTableSetInt(objRet, "ws_last_error_code", 18, g_iXsWsLastErrorCode);
	xvoTableSetText(objRet, "ws_last_invalid_reason", 22, (ptr)g_sXsWsLastInvalidReason, 0, FALSE);
	xvoTableSetText(objRet, "ws_last_invalid_time", 20, (ptr)(sLastInvalidTime ? sLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_invalid_age_ms", 22, XS_WsLastInvalidAgeMS());
	xvoTableSetInt(objRet, "ws_last_close_reason", 20, g_iXsWsLastCloseReason);
	xvoTableSetText(objRet, "ws_last_close_time", 18, (ptr)(sLastCloseTime ? sLastCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_close_age_ms", 20, XS_WsLastCloseAgeMS());
	xvoTableSetText(objRet, "ws_last_idle_close_time", 23, (ptr)(sLastIdleCloseTime ? sLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_idle_close_age_ms", 25, XS_WsLastIdleCloseAgeMS());
	xvoTableSetText(objRet, "ws_last_conn_limit_close_time", 29, (ptr)(sLastConnLimitCloseTime ? sLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_conn_limit_close_age_ms", 31, XS_WsLastConnLimitCloseAgeMS());
	xvoTableSetText(objRet, "ws_last_frame_type", 18, (ptr)XS_WsLastFrameTypeName(), 0, FALSE);
	xvoTableSetText(objRet, "ws_last_remote", 14, (ptr)g_sXsWsLastRemote, 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_bytes", 13, XS_HttpMetricGet(&g_iXsWsLastBytes));
	xvoTableSetText(objRet, "ws_last_text", 12, (ptr)g_sXsWsLastText, 0, FALSE);
	xvoTableSetText(objRet, "ws_last_time", 12, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_age_ms", 14, XS_WsLastAgeMS());
	xvoTableSetText(objRet, "ws_last_error_time", 18, (ptr)(sLastErrorTime ? sLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ws_last_error_age_ms", 20, XS_WsLastErrorAgeMS());

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastCloseTime ) {
		xrtFree(sLastCloseTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	if ( sLastInvalidTime ) {
		xrtFree(sLastInvalidTime);
	}
	if ( sLastIdleCloseTime ) {
		xrtFree(sLastIdleCloseTime);
	}
	if ( sLastConnLimitCloseTime ) {
		xrtFree(sLastConnLimitCloseTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "ws metrics json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleWsMetricsClear(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[1280];

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/ws_metrics_clear") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "ws metrics clear api disabled");
	}

	XS_WsClearMetrics();
	snprintf(
		sBody,
		sizeof(sBody),
		"ws_message_limit=%u\nws_conn_current=%lld\nws_conn_peak=%lld\nws_open_count=%lld\nws_close_count=%lld\nws_text_count=%lld\nws_binary_count=%lld\nws_ping_count=%lld\nws_pong_count=%lld\nws_error_count=%lld\nws_invalid_count=0\nws_idle_close_count=0\nws_conn_limit_close_count=0\nws_message_limit_close_count=0\nws_reject_count=0\nws_stop_cleanup_count=0\nws_last_reject_reason=(none)\nws_last_reject_time=(none)\nws_last_reject_age_ms=-1\nws_last_stop_cleanup_closed=0\nws_last_stop_cleanup_remain=0\nws_last_stop_cleanup_time=(none)\nws_last_stop_cleanup_age_ms=-1\nws_last_error_code=0\nws_last_invalid_reason=(none)\nws_last_invalid_time=(none)\nws_last_invalid_age_ms=-1\nws_last_close_reason=0\nws_last_close_time=(none)\nws_last_close_age_ms=-1\nws_last_idle_close_time=(none)\nws_last_idle_close_age_ms=-1\nws_last_conn_limit_close_time=(none)\nws_last_conn_limit_close_age_ms=-1\nws_last_message_limit_close_time=(none)\nws_last_message_limit_close_age_ms=-1\nws_last_frame_type=(none)\nws_last_remote=(none)\nws_last_bytes=0\nws_last_text=(none)\nws_last_time=(none)\nws_last_age_ms=-1\nws_last_error_time=(none)\nws_last_error_age_ms=-1\n",
		objServer->WsMessageLimit,
		(long long)XS_HttpMetricGet(&g_iXsWsConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsWsConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsWsOpenCount),
		(long long)XS_HttpMetricGet(&g_iXsWsCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsWsTextCount),
		(long long)XS_HttpMetricGet(&g_iXsWsBinaryCount),
		(long long)XS_HttpMetricGet(&g_iXsWsPingCount),
		(long long)XS_HttpMetricGet(&g_iXsWsPongCount),
		(long long)XS_HttpMetricGet(&g_iXsWsErrorCount)
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleXtpMetrics(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[2048];
	char* sLastTime;
	char* sLastInvalidTime;
	char* sLastErrorTime;
	char* sLastIdleCloseTime;
	char* sLastConnLimitCloseTime;
	char* sLastRecvLimitCloseTime;
	char* sLastRejectTime;
	char* sLastStopCleanupTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/xtp_metrics") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "xtp metrics api disabled");
	}

	sLastTime = XS_XtpLastTimeText();
	sLastInvalidTime = XS_XtpLastInvalidTimeText();
	sLastErrorTime = XS_XtpLastErrorTimeText();
	sLastIdleCloseTime = XS_XtpLastIdleCloseTimeText();
	sLastConnLimitCloseTime = XS_XtpLastConnLimitCloseTimeText();
	sLastRecvLimitCloseTime = XS_XtpLastRecvLimitCloseTimeText();
	sLastRejectTime = XS_XtpLastRejectTimeText();
	sLastStopCleanupTime = XS_XtpLastStopCleanupTimeText();
	snprintf(
		sBody,
		sizeof(sBody),
		"xtp_conn_current=%lld\nxtp_conn_peak=%lld\nxtp_open_count=%lld\nxtp_close_count=%lld\nxtp_error_count=%lld\nxtp_invalid_count=%lld\nxtp_msg_count=%lld\nxtp_req_count=%lld\nxtp_resp_count=%lld\nxtp_push_count=%lld\nxtp_event_count=%lld\nxtp_send_count=%lld\nxtp_recv_bytes=%lld\nxtp_send_bytes=%lld\nxtp_last_msg_type=%s\nxtp_last_status=%lld\nxtp_last_msg_id=%lld\nxtp_last_flags=%lld\nxtp_last_param_count=%lld\nxtp_last_body_size=%lld\nxtp_last_remote=%s\nxtp_last_bytes=%lld\nxtp_last_cmd=%s\nxtp_last_time=%s\nxtp_last_age_ms=%lld\nxtp_last_invalid_reason=%s\nxtp_last_invalid_time=%s\nxtp_last_invalid_age_ms=%lld\nxtp_last_error_code=%lld\nxtp_last_error_time=%s\nxtp_last_error_age_ms=%lld\nxtp_idle_close_count=%lld\nxtp_conn_limit_close_count=%lld\nxtp_recv_limit_close_count=%lld\nxtp_last_idle_close_time=%s\nxtp_last_idle_close_age_ms=%lld\nxtp_last_conn_limit_close_time=%s\nxtp_last_conn_limit_close_age_ms=%lld\nxtp_last_recv_limit_close_time=%s\nxtp_last_recv_limit_close_age_ms=%lld\n",
		(long long)XS_HttpMetricGet(&g_iXsXtpConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsXtpConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsXtpOpenCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpErrorCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpInvalidCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpMsgCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpReqCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpRespCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpPushCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpEventCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpSendCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpRecvBytes),
		(long long)XS_HttpMetricGet(&g_iXsXtpSendBytes),
		XS_XtpLastMsgTypeName()[0] ? XS_XtpLastMsgTypeName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsXtpLastStatus),
		(long long)XS_HttpMetricGet(&g_iXsXtpLastMsgID),
		(long long)XS_HttpMetricGet(&g_iXsXtpLastFlags),
		(long long)XS_HttpMetricGet(&g_iXsXtpLastParamCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpLastBodySize),
		g_sXsXtpLastRemote[0] ? g_sXsXtpLastRemote : "(none)",
		(long long)g_iXsXtpLastBytes,
		g_sXsXtpLastCmd[0] ? g_sXsXtpLastCmd : "(none)",
		sLastTime ? sLastTime : "(none)",
		(long long)XS_XtpLastAgeMS(),
		g_sXsXtpLastInvalidReason[0] ? g_sXsXtpLastInvalidReason : "(none)",
		sLastInvalidTime ? sLastInvalidTime : "(none)",
		(long long)XS_XtpLastInvalidAgeMS(),
		(long long)g_iXsXtpLastErrorCode,
		sLastErrorTime ? sLastErrorTime : "(none)",
		(long long)XS_XtpLastErrorAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsXtpIdleCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpConnLimitCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpRecvLimitCloseCount),
		sLastIdleCloseTime ? sLastIdleCloseTime : "(none)",
		(long long)XS_XtpLastIdleCloseAgeMS(),
		sLastConnLimitCloseTime ? sLastConnLimitCloseTime : "(none)",
		(long long)XS_XtpLastConnLimitCloseAgeMS(),
		sLastRecvLimitCloseTime ? sLastRecvLimitCloseTime : "(none)",
		(long long)XS_XtpLastRecvLimitCloseAgeMS()
	);
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"xtp_reject_count=%lld\nxtp_last_reject_reason=%s\nxtp_last_reject_time=%s\nxtp_last_reject_age_ms=%lld\nxtp_stop_cleanup_count=%lld\nxtp_last_stop_cleanup_closed=%lld\nxtp_last_stop_cleanup_remain=%lld\nxtp_last_stop_cleanup_time=%s\nxtp_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsXtpRejectCount),
			g_sXsXtpLastRejectReason[0] ? g_sXsXtpLastRejectReason : "(none)",
			sLastRejectTime ? sLastRejectTime : "(none)",
			(long long)XS_XtpLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastStopCleanupRemain),
			sLastStopCleanupTime ? sLastStopCleanupTime : "(none)",
			(long long)XS_XtpLastStopCleanupAgeMS()
		);
	}
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastInvalidTime ) {
		xrtFree(sLastInvalidTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	if ( sLastIdleCloseTime ) {
		xrtFree(sLastIdleCloseTime);
	}
	if ( sLastConnLimitCloseTime ) {
		xrtFree(sLastConnLimitCloseTime);
	}
	if ( sLastRecvLimitCloseTime ) {
		xrtFree(sLastRecvLimitCloseTime);
	}
	if ( sLastRejectTime ) {
		xrtFree(sLastRejectTime);
	}
	if ( sLastStopCleanupTime ) {
		xrtFree(sLastStopCleanupTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleXtpMetricsJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sLastTime;
	char* sLastInvalidTime;
	char* sLastErrorTime;
	char* sLastIdleCloseTime;
	char* sLastConnLimitCloseTime;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/xtp_metrics_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "xtp metrics json api disabled");
	}

	sLastTime = XS_XtpLastTimeText();
	sLastInvalidTime = XS_XtpLastInvalidTimeText();
	sLastErrorTime = XS_XtpLastErrorTimeText();
	sLastIdleCloseTime = XS_XtpLastIdleCloseTimeText();
	sLastConnLimitCloseTime = XS_XtpLastConnLimitCloseTimeText();
	objRet = xvoCreateTable();
	xvoTableSetInt(objRet, "xtp_conn_current", 16, XS_HttpMetricGet(&g_iXsXtpConnCurrent));
	xvoTableSetInt(objRet, "xtp_conn_peak", 13, XS_HttpMetricGet(&g_iXsXtpConnPeak));
	xvoTableSetInt(objRet, "xtp_open_count", 14, XS_HttpMetricGet(&g_iXsXtpOpenCount));
	xvoTableSetInt(objRet, "xtp_close_count", 15, XS_HttpMetricGet(&g_iXsXtpCloseCount));
	xvoTableSetInt(objRet, "xtp_error_count", 15, XS_HttpMetricGet(&g_iXsXtpErrorCount));
	xvoTableSetInt(objRet, "xtp_invalid_count", 17, XS_HttpMetricGet(&g_iXsXtpInvalidCount));
	xvoTableSetInt(objRet, "xtp_msg_count", 13, XS_HttpMetricGet(&g_iXsXtpMsgCount));
	xvoTableSetInt(objRet, "xtp_req_count", 13, XS_HttpMetricGet(&g_iXsXtpReqCount));
	xvoTableSetInt(objRet, "xtp_resp_count", 14, XS_HttpMetricGet(&g_iXsXtpRespCount));
	xvoTableSetInt(objRet, "xtp_push_count", 14, XS_HttpMetricGet(&g_iXsXtpPushCount));
	xvoTableSetInt(objRet, "xtp_event_count", 15, XS_HttpMetricGet(&g_iXsXtpEventCount));
	xvoTableSetInt(objRet, "xtp_send_count", 14, XS_HttpMetricGet(&g_iXsXtpSendCount));
	xvoTableSetInt(objRet, "xtp_recv_bytes", 14, XS_HttpMetricGet(&g_iXsXtpRecvBytes));
	xvoTableSetInt(objRet, "xtp_send_bytes", 14, XS_HttpMetricGet(&g_iXsXtpSendBytes));
	xvoTableSetInt(objRet, "xtp_idle_close_count", 20, XS_HttpMetricGet(&g_iXsXtpIdleCloseCount));
	xvoTableSetInt(objRet, "xtp_conn_limit_close_count", 26, XS_HttpMetricGet(&g_iXsXtpConnLimitCloseCount));
	XS_XtpAppendRecvLimitMetrics(objRet);
	XS_XtpAppendRejectMetrics(objRet);
	XS_XtpAppendStopCleanupMetrics(objRet);
	xvoTableSetText(objRet, "xtp_last_msg_type", 17, (ptr)XS_XtpLastMsgTypeName(), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_status", 15, XS_HttpMetricGet(&g_iXsXtpLastStatus));
	xvoTableSetInt(objRet, "xtp_last_msg_id", 15, XS_HttpMetricGet(&g_iXsXtpLastMsgID));
	xvoTableSetInt(objRet, "xtp_last_flags", 14, XS_HttpMetricGet(&g_iXsXtpLastFlags));
	xvoTableSetInt(objRet, "xtp_last_param_count", 20, XS_HttpMetricGet(&g_iXsXtpLastParamCount));
	xvoTableSetInt(objRet, "xtp_last_body_size", 18, XS_HttpMetricGet(&g_iXsXtpLastBodySize));
	xvoTableSetText(objRet, "xtp_last_remote", 15, (ptr)g_sXsXtpLastRemote, 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_bytes", 14, g_iXsXtpLastBytes);
	xvoTableSetText(objRet, "xtp_last_cmd", 12, (ptr)g_sXsXtpLastCmd, 0, FALSE);
	xvoTableSetText(objRet, "xtp_last_time", 13, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_age_ms", 15, XS_XtpLastAgeMS());
	xvoTableSetText(objRet, "xtp_last_invalid_reason", 23, (ptr)g_sXsXtpLastInvalidReason, 0, FALSE);
	xvoTableSetText(objRet, "xtp_last_invalid_time", 21, (ptr)(sLastInvalidTime ? sLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_invalid_age_ms", 23, XS_XtpLastInvalidAgeMS());
	xvoTableSetInt(objRet, "xtp_last_error_code", 19, g_iXsXtpLastErrorCode);
	xvoTableSetText(objRet, "xtp_last_error_time", 19, (ptr)(sLastErrorTime ? sLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_error_age_ms", 21, XS_XtpLastErrorAgeMS());
	xvoTableSetText(objRet, "xtp_last_idle_close_time", 24, (ptr)(sLastIdleCloseTime ? sLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_idle_close_age_ms", 26, XS_XtpLastIdleCloseAgeMS());
	xvoTableSetText(objRet, "xtp_last_conn_limit_close_time", 30, (ptr)(sLastConnLimitCloseTime ? sLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_conn_limit_close_age_ms", 32, XS_XtpLastConnLimitCloseAgeMS());

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastInvalidTime ) {
		xrtFree(sLastInvalidTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	if ( sLastIdleCloseTime ) {
		xrtFree(sLastIdleCloseTime);
	}
	if ( sLastConnLimitCloseTime ) {
		xrtFree(sLastConnLimitCloseTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "xtp metrics json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleXtpMetricsClear(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[1536];

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/xtp_metrics_clear") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "xtp metrics clear api disabled");
	}

	XS_XtpClearMetrics();
	snprintf(
		sBody,
		sizeof(sBody),
		"xtp_conn_current=0\nxtp_conn_peak=0\nxtp_open_count=0\nxtp_close_count=0\nxtp_error_count=0\nxtp_invalid_count=0\nxtp_msg_count=0\nxtp_req_count=0\nxtp_resp_count=0\nxtp_push_count=0\nxtp_event_count=0\nxtp_send_count=0\nxtp_recv_bytes=0\nxtp_send_bytes=0\nxtp_last_msg_type=(none)\nxtp_last_status=0\nxtp_last_msg_id=0\nxtp_last_flags=0\nxtp_last_param_count=0\nxtp_last_body_size=0\nxtp_last_remote=(none)\nxtp_last_bytes=0\nxtp_last_cmd=(none)\nxtp_last_time=(none)\nxtp_last_age_ms=-1\nxtp_last_invalid_reason=(none)\nxtp_last_invalid_time=(none)\nxtp_last_invalid_age_ms=-1\nxtp_last_error_code=0\nxtp_last_error_time=(none)\nxtp_last_error_age_ms=-1\nxtp_idle_close_count=0\nxtp_conn_limit_close_count=0\nxtp_recv_limit_close_count=0\nxtp_reject_count=0\nxtp_stop_cleanup_count=0\nxtp_last_reject_reason=(none)\nxtp_last_reject_time=(none)\nxtp_last_reject_age_ms=-1\nxtp_last_stop_cleanup_closed=0\nxtp_last_stop_cleanup_remain=0\nxtp_last_stop_cleanup_time=(none)\nxtp_last_stop_cleanup_age_ms=-1\nxtp_last_idle_close_time=(none)\nxtp_last_idle_close_age_ms=-1\nxtp_last_conn_limit_close_time=(none)\nxtp_last_conn_limit_close_age_ms=-1\nxtp_last_recv_limit_close_time=(none)\nxtp_last_recv_limit_close_age_ms=-1\n"
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleUdpMetrics(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[768];
	char* sLastTime;
	char* sLastErrorTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/udp_metrics") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "udp metrics api disabled");
	}

	sLastTime = XS_UdpLastTimeText();
	sLastErrorTime = XS_UdpLastErrorTimeText();
	snprintf(
		sBody,
		sizeof(sBody),
		"udp_recv_count=%lld\nudp_send_count=%lld\nudp_error_count=%lld\nudp_last_error_code=%lld\nudp_recv_bytes=%lld\nudp_send_bytes=%lld\nudp_last_from=%s\nudp_last_text=%s\nudp_last_bytes=%lld\nudp_last_time=%s\nudp_last_age_ms=%lld\nudp_last_error_time=%s\nudp_last_error_age_ms=%lld\n",
		(long long)XS_HttpMetricGet(&g_iXsUdpRecvCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpSendCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpErrorCount),
		(long long)g_iXsUdpLastErrorCode,
		(long long)XS_HttpMetricGet(&g_iXsUdpRecvBytes),
		(long long)XS_HttpMetricGet(&g_iXsUdpSendBytes),
		g_sXsUdpLastFrom[0] ? g_sXsUdpLastFrom : "(none)",
		g_sXsUdpLastText[0] ? g_sXsUdpLastText : "(none)",
		(long long)g_iXsUdpLastBytes,
		sLastTime ? sLastTime : "(none)",
		(long long)XS_UdpLastAgeMS(),
		sLastErrorTime ? sLastErrorTime : "(none)",
		(long long)XS_UdpLastErrorAgeMS()
	);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleUdpMetricsJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sLastTime;
	char* sLastErrorTime;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/udp_metrics_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "udp metrics json api disabled");
	}

	sLastTime = XS_UdpLastTimeText();
	sLastErrorTime = XS_UdpLastErrorTimeText();
	objRet = xvoCreateTable();
	xvoTableSetInt(objRet, "udp_recv_count", 14, XS_HttpMetricGet(&g_iXsUdpRecvCount));
	xvoTableSetInt(objRet, "udp_send_count", 14, XS_HttpMetricGet(&g_iXsUdpSendCount));
	xvoTableSetInt(objRet, "udp_error_count", 15, XS_HttpMetricGet(&g_iXsUdpErrorCount));
	xvoTableSetInt(objRet, "udp_last_error_code", 19, g_iXsUdpLastErrorCode);
	xvoTableSetInt(objRet, "udp_recv_bytes", 14, XS_HttpMetricGet(&g_iXsUdpRecvBytes));
	xvoTableSetInt(objRet, "udp_send_bytes", 14, XS_HttpMetricGet(&g_iXsUdpSendBytes));
	xvoTableSetText(objRet, "udp_last_from", 13, (ptr)g_sXsUdpLastFrom, 0, FALSE);
	xvoTableSetText(objRet, "udp_last_text", 13, (ptr)g_sXsUdpLastText, 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_bytes", 14, g_iXsUdpLastBytes);
	xvoTableSetText(objRet, "udp_last_time", 13, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_age_ms", 15, XS_UdpLastAgeMS());
	xvoTableSetText(objRet, "udp_last_error_time", 19, (ptr)(sLastErrorTime ? sLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_error_age_ms", 21, XS_UdpLastErrorAgeMS());

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "udp metrics json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleUdpMetricsClear(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[512];

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/udp_metrics_clear") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "udp metrics clear api disabled");
	}

	XS_UdpClearMetrics();
	snprintf(
		sBody,
		sizeof(sBody),
		"udp_recv_count=0\nudp_send_count=0\nudp_error_count=0\nudp_last_error_code=0\nudp_recv_bytes=0\nudp_send_bytes=0\nudp_last_from=(none)\nudp_last_text=(none)\nudp_last_bytes=0\nudp_last_time=(none)\nudp_last_age_ms=-1\nudp_last_error_time=(none)\nudp_last_error_age_ms=-1\n"
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleCustomMetrics(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[1536];
	char* sLastTime;
	char* sLastCloseTime;
	char* sLastErrorTime;
	char* sLastInvalidTime;
	char* sLastIdleCloseTime;
	char* sLastConnLimitCloseTime;
	char* sLastRecvLimitCloseTime;
	char* sLastRejectTime;
	char* sLastStopCleanupTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/custom_metrics") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "custom metrics api disabled");
	}

	sLastTime = XS_CustomLastTimeText();
	sLastCloseTime = XS_CustomLastCloseTimeText();
	sLastErrorTime = XS_CustomLastErrorTimeText();
	sLastInvalidTime = XS_CustomLastInvalidTimeText();
	sLastIdleCloseTime = XS_CustomLastIdleCloseTimeText();
	sLastConnLimitCloseTime = XS_CustomLastConnLimitCloseTimeText();
	sLastRecvLimitCloseTime = XS_CustomLastRecvLimitCloseTimeText();
	sLastRejectTime = XS_CustomLastRejectTimeText();
	sLastStopCleanupTime = XS_CustomLastStopCleanupTimeText();
	snprintf(
		sBody,
		sizeof(sBody),
		"custom_conn_current=%lld\ncustom_conn_peak=%lld\ncustom_open_count=%lld\ncustom_close_count=%lld\ncustom_error_count=%lld\ncustom_invalid_count=%lld\ncustom_last_invalid_reason=%s\ncustom_last_invalid_time=%s\ncustom_last_invalid_age_ms=%lld\ncustom_last_close_reason=%lld\ncustom_last_close_time=%s\ncustom_last_close_age_ms=%lld\ncustom_last_error_code=%lld\ncustom_last_error_time=%s\ncustom_last_error_age_ms=%lld\ncustom_recv_count=%lld\ncustom_send_count=%lld\ncustom_recv_bytes=%lld\ncustom_send_bytes=%lld\ncustom_last_remote=%s\ncustom_last_bytes=%lld\ncustom_last_text=%s\ncustom_last_time=%s\ncustom_last_age_ms=%lld\ncustom_idle_close_count=%lld\ncustom_conn_limit_close_count=%lld\ncustom_recv_limit_close_count=%lld\ncustom_last_idle_close_time=%s\ncustom_last_idle_close_age_ms=%lld\ncustom_last_conn_limit_close_time=%s\ncustom_last_conn_limit_close_age_ms=%lld\ncustom_last_recv_limit_close_time=%s\ncustom_last_recv_limit_close_age_ms=%lld\n",
		(long long)XS_HttpMetricGet(&g_iXsCustomConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsCustomConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsCustomOpenCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomErrorCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomInvalidCount),
		g_sXsCustomLastInvalidReason[0] ? g_sXsCustomLastInvalidReason : "(none)",
		sLastInvalidTime ? sLastInvalidTime : "(none)",
		(long long)XS_CustomLastInvalidAgeMS(),
		(long long)g_iXsCustomLastCloseReason,
		sLastCloseTime ? sLastCloseTime : "(none)",
		(long long)XS_CustomLastCloseAgeMS(),
		(long long)g_iXsCustomLastErrorCode,
		sLastErrorTime ? sLastErrorTime : "(none)",
		(long long)XS_CustomLastErrorAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsCustomRecvCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomSendCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomRecvBytes),
		(long long)XS_HttpMetricGet(&g_iXsCustomSendBytes),
		g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)",
		(long long)g_iXsCustomLastBytes,
		g_sXsCustomLastText[0] ? g_sXsCustomLastText : "(none)",
		sLastTime ? sLastTime : "(none)",
		(long long)XS_CustomLastAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsCustomIdleCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomConnLimitCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomRecvLimitCloseCount),
		sLastIdleCloseTime ? sLastIdleCloseTime : "(none)",
		(long long)XS_CustomLastIdleCloseAgeMS(),
		sLastConnLimitCloseTime ? sLastConnLimitCloseTime : "(none)",
		(long long)XS_CustomLastConnLimitCloseAgeMS(),
		sLastRecvLimitCloseTime ? sLastRecvLimitCloseTime : "(none)",
		(long long)XS_CustomLastRecvLimitCloseAgeMS()
	);
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"custom_reject_count=%lld\ncustom_last_reject_reason=%s\ncustom_last_reject_time=%s\ncustom_last_reject_age_ms=%lld\ncustom_stop_cleanup_count=%lld\ncustom_last_stop_cleanup_closed=%lld\ncustom_last_stop_cleanup_remain=%lld\ncustom_last_stop_cleanup_time=%s\ncustom_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsCustomRejectCount),
			g_sXsCustomLastRejectReason[0] ? g_sXsCustomLastRejectReason : "(none)",
			sLastRejectTime ? sLastRejectTime : "(none)",
			(long long)XS_CustomLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsCustomLastStopCleanupRemain),
			sLastStopCleanupTime ? sLastStopCleanupTime : "(none)",
			(long long)XS_CustomLastStopCleanupAgeMS()
		);
	}
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastCloseTime ) {
		xrtFree(sLastCloseTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	if ( sLastInvalidTime ) {
		xrtFree(sLastInvalidTime);
	}
	if ( sLastIdleCloseTime ) {
		xrtFree(sLastIdleCloseTime);
	}
	if ( sLastConnLimitCloseTime ) {
		xrtFree(sLastConnLimitCloseTime);
	}
	if ( sLastRecvLimitCloseTime ) {
		xrtFree(sLastRecvLimitCloseTime);
	}
	if ( sLastRejectTime ) {
		xrtFree(sLastRejectTime);
	}
	if ( sLastStopCleanupTime ) {
		xrtFree(sLastStopCleanupTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleCustomMetricsJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sLastTime;
	char* sLastCloseTime;
	char* sLastErrorTime;
	char* sLastInvalidTime;
	char* sLastIdleCloseTime;
	char* sLastConnLimitCloseTime;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/custom_metrics_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "custom metrics json api disabled");
	}

	sLastTime = XS_CustomLastTimeText();
	sLastCloseTime = XS_CustomLastCloseTimeText();
	sLastErrorTime = XS_CustomLastErrorTimeText();
	sLastInvalidTime = XS_CustomLastInvalidTimeText();
	sLastIdleCloseTime = XS_CustomLastIdleCloseTimeText();
	sLastConnLimitCloseTime = XS_CustomLastConnLimitCloseTimeText();
	objRet = xvoCreateTable();
	xvoTableSetInt(objRet, "custom_conn_current", 19, XS_HttpMetricGet(&g_iXsCustomConnCurrent));
	xvoTableSetInt(objRet, "custom_conn_peak", 16, XS_HttpMetricGet(&g_iXsCustomConnPeak));
	xvoTableSetInt(objRet, "custom_open_count", 17, XS_HttpMetricGet(&g_iXsCustomOpenCount));
	xvoTableSetInt(objRet, "custom_close_count", 18, XS_HttpMetricGet(&g_iXsCustomCloseCount));
	xvoTableSetInt(objRet, "custom_error_count", 18, XS_HttpMetricGet(&g_iXsCustomErrorCount));
	xvoTableSetInt(objRet, "custom_invalid_count", 20, XS_HttpMetricGet(&g_iXsCustomInvalidCount));
	xvoTableSetText(objRet, "custom_last_invalid_reason", 26, (ptr)g_sXsCustomLastInvalidReason, 0, FALSE);
	xvoTableSetText(objRet, "custom_last_invalid_time", 24, (ptr)(sLastInvalidTime ? sLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_invalid_age_ms", 26, XS_CustomLastInvalidAgeMS());
	xvoTableSetInt(objRet, "custom_last_close_reason", 24, g_iXsCustomLastCloseReason);
	xvoTableSetText(objRet, "custom_last_close_time", 22, (ptr)(sLastCloseTime ? sLastCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_close_age_ms", 24, XS_CustomLastCloseAgeMS());
	xvoTableSetInt(objRet, "custom_last_error_code", 22, g_iXsCustomLastErrorCode);
	xvoTableSetText(objRet, "custom_last_error_time", 22, (ptr)(sLastErrorTime ? sLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_error_age_ms", 24, XS_CustomLastErrorAgeMS());
	xvoTableSetInt(objRet, "custom_recv_count", 17, XS_HttpMetricGet(&g_iXsCustomRecvCount));
	xvoTableSetInt(objRet, "custom_send_count", 17, XS_HttpMetricGet(&g_iXsCustomSendCount));
	xvoTableSetInt(objRet, "custom_recv_bytes", 17, XS_HttpMetricGet(&g_iXsCustomRecvBytes));
	xvoTableSetInt(objRet, "custom_send_bytes", 17, XS_HttpMetricGet(&g_iXsCustomSendBytes));
	xvoTableSetInt(objRet, "custom_idle_close_count", 23, XS_HttpMetricGet(&g_iXsCustomIdleCloseCount));
	xvoTableSetInt(objRet, "custom_conn_limit_close_count", 29, XS_HttpMetricGet(&g_iXsCustomConnLimitCloseCount));
	XS_CustomAppendRecvLimitMetrics(objRet);
	XS_CustomAppendRejectMetrics(objRet);
	XS_CustomAppendStopCleanupMetrics(objRet);
	xvoTableSetText(objRet, "custom_last_remote", 18, (ptr)g_sXsCustomLastRemote, 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_bytes", 17, g_iXsCustomLastBytes);
	xvoTableSetText(objRet, "custom_last_text", 16, (ptr)g_sXsCustomLastText, 0, FALSE);
	xvoTableSetText(objRet, "custom_last_time", 16, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_age_ms", 18, XS_CustomLastAgeMS());
	xvoTableSetText(objRet, "custom_last_idle_close_time", 27, (ptr)(sLastIdleCloseTime ? sLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_idle_close_age_ms", 29, XS_CustomLastIdleCloseAgeMS());
	xvoTableSetText(objRet, "custom_last_conn_limit_close_time", 33, (ptr)(sLastConnLimitCloseTime ? sLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_conn_limit_close_age_ms", 35, XS_CustomLastConnLimitCloseAgeMS());

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastCloseTime ) {
		xrtFree(sLastCloseTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	if ( sLastInvalidTime ) {
		xrtFree(sLastInvalidTime);
	}
	if ( sLastIdleCloseTime ) {
		xrtFree(sLastIdleCloseTime);
	}
	if ( sLastConnLimitCloseTime ) {
		xrtFree(sLastConnLimitCloseTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "custom metrics json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleCustomMetricsClear(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[1280];

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/custom_metrics_clear") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "custom metrics clear api disabled");
	}

	XS_CustomClearMetrics();
	snprintf(
		sBody,
		sizeof(sBody),
		"custom_conn_current=0\ncustom_conn_peak=0\ncustom_open_count=0\ncustom_close_count=0\ncustom_error_count=0\ncustom_invalid_count=0\ncustom_last_invalid_reason=(none)\ncustom_last_invalid_time=(none)\ncustom_last_invalid_age_ms=-1\ncustom_last_close_reason=0\ncustom_last_close_time=(none)\ncustom_last_close_age_ms=-1\ncustom_last_error_code=0\ncustom_last_error_time=(none)\ncustom_last_error_age_ms=-1\ncustom_recv_count=0\ncustom_send_count=0\ncustom_recv_bytes=0\ncustom_send_bytes=0\ncustom_last_remote=(none)\ncustom_last_bytes=0\ncustom_last_text=(none)\ncustom_last_time=(none)\ncustom_last_age_ms=-1\ncustom_idle_close_count=0\ncustom_conn_limit_close_count=0\ncustom_recv_limit_close_count=0\ncustom_reject_count=0\ncustom_stop_cleanup_count=0\ncustom_last_reject_reason=(none)\ncustom_last_reject_time=(none)\ncustom_last_reject_age_ms=-1\ncustom_last_stop_cleanup_closed=0\ncustom_last_stop_cleanup_remain=0\ncustom_last_stop_cleanup_time=(none)\ncustom_last_stop_cleanup_age_ms=-1\ncustom_last_idle_close_time=(none)\ncustom_last_idle_close_age_ms=-1\ncustom_last_conn_limit_close_time=(none)\ncustom_last_conn_limit_close_age_ms=-1\ncustom_last_recv_limit_close_time=(none)\ncustom_last_recv_limit_close_age_ms=-1\n"
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleDashboardJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ReloadStatusSnapshot tReloadStatus;
	XS_CheckConfigStatusSnapshot tCheckStatus;
	XS_Config objCfgCheck;
	xvalue objRet;
	xvalue objStatus;
	xvalue objHealth;
	xvalue objReload;
	xvalue objBus;
	xvalue objCheck;
	char* sStartTime;
	char* sReloadTime;
	char* sCheckTime;
	char* sCheckBase;
	char* sConfigBase;
	char* sCurrentDir;
	char* sConfigName;
	char* sAppMTime;
	char* sConfigMTime;
	char* sBusQueueTime;
	char* sBusDispatchTime;
	char* sBusLastSweepTime;
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sHttpLastIdleCloseTime;
	char* sHttpLastConnLimitCloseTime;
	char* sWsLastTime;
	char* sWsLastCloseTime;
	char* sWsLastErrorTime;
	char* sWsLastInvalidTime;
	char* sWsLastIdleCloseTime;
	char* sWsLastConnLimitCloseTime;
	char* sXtpLastTime;
	char* sXtpLastInvalidTime;
	char* sXtpLastErrorTime;
	char* sXtpLastIdleCloseTime;
	char* sXtpLastConnLimitCloseTime;
	char* sUdpLastTime;
	char* sUdpLastErrorTime;
	char* sCustomLastTime;
	char* sCustomLastCloseTime;
	char* sCustomLastErrorTime;
	char* sCustomLastInvalidTime;
	char* sCustomLastIdleCloseTime;
	char* sCustomLastConnLimitCloseTime;
	int64 iAppSize;
	int64 iConfigSize;
	char* sJson;
	bool bConfigOK;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/dashboard_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "dashboard json api disabled");
	}
	memset(&objCfgCheck, 0, sizeof(objCfgCheck));
	sConfigBase = (g_sXsConfigFile && g_sXsConfigFile[0]) ? xrtPathGetDir(g_sXsConfigFile, 0) : NULL;
	sCurrentDir = XS_CurrentWorkDir();
	sConfigName = XS_ConfigFileName();
	sAppMTime = XS_FileChangeTimeText(xCore.AppFile ? (char*)xCore.AppFile : NULL);
	sConfigMTime = XS_FileChangeTimeText(g_sXsConfigFile);
	sBusQueueTime = NULL;
	sBusDispatchTime = NULL;
	iAppSize = XS_FileSizeValue(xCore.AppFile ? (char*)xCore.AppFile : NULL);
	iConfigSize = XS_FileSizeValue(g_sXsConfigFile);
	XS_ConfigReloadStatusSnapshot(&tReloadStatus);
	XS_ConfigCheckStatusSnapshot(&tCheckStatus);
	sReloadTime = XS_ReloadTimeTextByStatus(&tReloadStatus);
	sCheckTime = XS_CheckConfigTimeTextByStatus(&tCheckStatus);
	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sHttpLastIdleCloseTime = XS_HttpLastIdleCloseTimeText();
	sHttpLastConnLimitCloseTime = XS_HttpLastConnLimitCloseTimeText();
	sWsLastTime = XS_WsLastTimeText();
	sWsLastCloseTime = XS_WsLastCloseTimeText();
	sWsLastErrorTime = XS_WsLastErrorTimeText();
	sWsLastInvalidTime = XS_WsLastInvalidTimeText();
	sWsLastIdleCloseTime = XS_WsLastIdleCloseTimeText();
	sWsLastConnLimitCloseTime = XS_WsLastConnLimitCloseTimeText();
	sXtpLastTime = XS_XtpLastTimeText();
	sXtpLastInvalidTime = XS_XtpLastInvalidTimeText();
	sXtpLastErrorTime = XS_XtpLastErrorTimeText();
	sXtpLastIdleCloseTime = XS_XtpLastIdleCloseTimeText();
	sXtpLastConnLimitCloseTime = XS_XtpLastConnLimitCloseTimeText();
	sUdpLastTime = XS_UdpLastTimeText();
	sUdpLastErrorTime = XS_UdpLastErrorTimeText();
	sCustomLastTime = XS_CustomLastTimeText();
	sCustomLastCloseTime = XS_CustomLastCloseTimeText();
	sCustomLastErrorTime = XS_CustomLastErrorTimeText();
	sCustomLastInvalidTime = XS_CustomLastInvalidTimeText();
	sCustomLastIdleCloseTime = XS_CustomLastIdleCloseTimeText();
	sCustomLastConnLimitCloseTime = XS_CustomLastConnLimitCloseTimeText();
	bConfigOK = (g_sXsConfigFile && g_sXsConfigFile[0]) ? XS_LoadConfig(&objCfgCheck, g_sXsConfigFile) : FALSE;
	sCheckBase = (bConfigOK && objCfgCheck.BaseDir) ? XS_CopyText(objCfgCheck.BaseDir) : NULL;
	sStartTime = g_tXsStartTime ? xrtTimeToStr(g_tXsStartTime, XRT_TIME_FORMAT_DATETIME) : NULL;

	objStatus = xvoCreateTable();
	xvoTableSetText(objStatus, "server", 6, objServer->Name ? objServer->Name : "", 0, FALSE);
	xvoTableSetText(objStatus, "class", 5, (ptr)XS_ServerClassName(objServer->Class), 0, FALSE);
	xvoTableSetText(objStatus, "addr", 4, objServer->Addr ? objServer->Addr : "", 0, FALSE);
	xvoTableSetText(objStatus, "bind_ip", 7, objServer->BindIP ? objServer->BindIP : "", 0, FALSE);
	xvoTableSetInt(objStatus, "bind_port", 9, objServer->BindPort);
	xvoTableSetBool(objStatus, "tls", 3, objServer->EnableTLS);
	xvoTableSetText(objStatus, "bind_ip_tls", 11, objServer->BindIPTLS ? objServer->BindIPTLS : "", 0, FALSE);
	xvoTableSetInt(objStatus, "bind_port_tls", 13, objServer->BindPortTLS);
	xvoTableSetText(objStatus, "addr_tls", 8, objServer->AddrTLS ? objServer->AddrTLS : "", 0, FALSE);
	xvoTableSetText(objStatus, "ws_protocol", 11, objServer->WsProtocol ? objServer->WsProtocol : "", 0, FALSE);
	xvoTableSetInt(objStatus, "ws_message_limit", 16, objServer->WsMessageLimit);
	xvoTableSetInt(objStatus, "idle_timeout", 12, objServer->IdleTimeout);
	xvoTableSetInt(objStatus, "conn_limit", 10, objServer->ConnLimit);
	xvoTableSetInt(objStatus, "ws_conn_current", 15, XS_HttpMetricGet(&g_iXsWsConnCurrent));
	xvoTableSetInt(objStatus, "ws_conn_peak", 12, XS_HttpMetricGet(&g_iXsWsConnPeak));
	xvoTableSetInt(objStatus, "ws_open_count", 13, XS_HttpMetricGet(&g_iXsWsOpenCount));
	xvoTableSetInt(objStatus, "ws_close_count", 14, XS_HttpMetricGet(&g_iXsWsCloseCount));
	xvoTableSetInt(objStatus, "ws_text_count", 13, XS_HttpMetricGet(&g_iXsWsTextCount));
	xvoTableSetInt(objStatus, "ws_binary_count", 15, XS_HttpMetricGet(&g_iXsWsBinaryCount));
	xvoTableSetInt(objStatus, "ws_ping_count", 13, XS_HttpMetricGet(&g_iXsWsPingCount));
	xvoTableSetInt(objStatus, "ws_pong_count", 13, XS_HttpMetricGet(&g_iXsWsPongCount));
	xvoTableSetInt(objStatus, "ws_error_count", 14, XS_HttpMetricGet(&g_iXsWsErrorCount));
	xvoTableSetInt(objStatus, "ws_invalid_count", 16, XS_HttpMetricGet(&g_iXsWsInvalidCount));
	xvoTableSetInt(objStatus, "ws_idle_close_count", 19, XS_HttpMetricGet(&g_iXsWsIdleCloseCount));
	xvoTableSetInt(objStatus, "ws_conn_limit_close_count", 25, XS_HttpMetricGet(&g_iXsWsConnLimitCloseCount));
	XS_WsAppendMessageLimitMetrics(objStatus);
	XS_WsAppendRejectMetrics(objStatus);
	XS_WsAppendStopCleanupMetrics(objStatus);
	xvoTableSetInt(objStatus, "ws_last_error_code", 18, g_iXsWsLastErrorCode);
	xvoTableSetText(objStatus, "ws_last_invalid_reason", 22, (ptr)g_sXsWsLastInvalidReason, 0, FALSE);
	xvoTableSetText(objStatus, "ws_last_invalid_time", 20, (ptr)(sWsLastInvalidTime ? sWsLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "ws_last_invalid_age_ms", 22, XS_WsLastInvalidAgeMS());
	xvoTableSetInt(objStatus, "ws_last_close_reason", 20, g_iXsWsLastCloseReason);
	xvoTableSetText(objStatus, "ws_last_close_time", 18, (ptr)(sWsLastCloseTime ? sWsLastCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "ws_last_close_age_ms", 20, XS_WsLastCloseAgeMS());
	xvoTableSetText(objStatus, "ws_last_idle_close_time", 23, (ptr)(sWsLastIdleCloseTime ? sWsLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "ws_last_idle_close_age_ms", 25, XS_WsLastIdleCloseAgeMS());
	xvoTableSetText(objStatus, "ws_last_conn_limit_close_time", 29, (ptr)(sWsLastConnLimitCloseTime ? sWsLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "ws_last_conn_limit_close_age_ms", 31, XS_WsLastConnLimitCloseAgeMS());
	xvoTableSetText(objStatus, "ws_last_frame_type", 18, (ptr)XS_WsLastFrameTypeName(), 0, FALSE);
	xvoTableSetText(objStatus, "ws_last_remote", 14, (ptr)g_sXsWsLastRemote, 0, FALSE);
	xvoTableSetInt(objStatus, "ws_last_bytes", 13, XS_HttpMetricGet(&g_iXsWsLastBytes));
	xvoTableSetText(objStatus, "ws_last_text", 12, (ptr)g_sXsWsLastText, 0, FALSE);
	xvoTableSetText(objStatus, "ws_last_time", 12, (ptr)(sWsLastTime ? sWsLastTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "ws_last_age_ms", 14, XS_WsLastAgeMS());
	xvoTableSetText(objStatus, "ws_last_error_time", 18, (ptr)(sWsLastErrorTime ? sWsLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "ws_last_error_age_ms", 20, XS_WsLastErrorAgeMS());
	xvoTableSetInt(objStatus, "xtp_conn_current", 16, XS_HttpMetricGet(&g_iXsXtpConnCurrent));
	xvoTableSetInt(objStatus, "xtp_conn_peak", 13, XS_HttpMetricGet(&g_iXsXtpConnPeak));
	xvoTableSetInt(objStatus, "xtp_open_count", 14, XS_HttpMetricGet(&g_iXsXtpOpenCount));
	xvoTableSetInt(objStatus, "xtp_close_count", 15, XS_HttpMetricGet(&g_iXsXtpCloseCount));
	xvoTableSetInt(objStatus, "xtp_error_count", 15, XS_HttpMetricGet(&g_iXsXtpErrorCount));
	xvoTableSetInt(objStatus, "xtp_invalid_count", 17, XS_HttpMetricGet(&g_iXsXtpInvalidCount));
	xvoTableSetInt(objStatus, "xtp_msg_count", 13, XS_HttpMetricGet(&g_iXsXtpMsgCount));
	xvoTableSetInt(objStatus, "xtp_req_count", 13, XS_HttpMetricGet(&g_iXsXtpReqCount));
	xvoTableSetInt(objStatus, "xtp_resp_count", 14, XS_HttpMetricGet(&g_iXsXtpRespCount));
	xvoTableSetInt(objStatus, "xtp_push_count", 14, XS_HttpMetricGet(&g_iXsXtpPushCount));
	xvoTableSetInt(objStatus, "xtp_event_count", 15, XS_HttpMetricGet(&g_iXsXtpEventCount));
	xvoTableSetInt(objStatus, "xtp_send_count", 14, XS_HttpMetricGet(&g_iXsXtpSendCount));
	xvoTableSetInt(objStatus, "xtp_recv_bytes", 14, XS_HttpMetricGet(&g_iXsXtpRecvBytes));
	xvoTableSetInt(objStatus, "xtp_send_bytes", 14, XS_HttpMetricGet(&g_iXsXtpSendBytes));
	xvoTableSetInt(objStatus, "xtp_idle_close_count", 20, XS_HttpMetricGet(&g_iXsXtpIdleCloseCount));
	xvoTableSetInt(objStatus, "xtp_conn_limit_close_count", 26, XS_HttpMetricGet(&g_iXsXtpConnLimitCloseCount));
	XS_XtpAppendRecvLimitMetrics(objStatus);
	XS_XtpAppendRejectMetrics(objStatus);
	XS_XtpAppendStopCleanupMetrics(objStatus);
	xvoTableSetText(objStatus, "xtp_last_msg_type", 17, (ptr)XS_XtpLastMsgTypeName(), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_status", 15, XS_HttpMetricGet(&g_iXsXtpLastStatus));
	xvoTableSetInt(objStatus, "xtp_last_msg_id", 15, XS_HttpMetricGet(&g_iXsXtpLastMsgID));
	xvoTableSetInt(objStatus, "xtp_last_flags", 14, XS_HttpMetricGet(&g_iXsXtpLastFlags));
	xvoTableSetInt(objStatus, "xtp_last_param_count", 20, XS_HttpMetricGet(&g_iXsXtpLastParamCount));
	xvoTableSetInt(objStatus, "xtp_last_body_size", 18, XS_HttpMetricGet(&g_iXsXtpLastBodySize));
	xvoTableSetText(objStatus, "xtp_last_remote", 15, (ptr)g_sXsXtpLastRemote, 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_bytes", 14, g_iXsXtpLastBytes);
	xvoTableSetText(objStatus, "xtp_last_cmd", 12, (ptr)g_sXsXtpLastCmd, 0, FALSE);
	xvoTableSetText(objStatus, "xtp_last_time", 13, (ptr)(sXtpLastTime ? sXtpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_age_ms", 15, XS_XtpLastAgeMS());
	xvoTableSetText(objStatus, "xtp_last_invalid_reason", 23, (ptr)g_sXsXtpLastInvalidReason, 0, FALSE);
	xvoTableSetText(objStatus, "xtp_last_invalid_time", 21, (ptr)(sXtpLastInvalidTime ? sXtpLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_invalid_age_ms", 23, XS_XtpLastInvalidAgeMS());
	xvoTableSetInt(objStatus, "xtp_last_error_code", 19, g_iXsXtpLastErrorCode);
	xvoTableSetText(objStatus, "xtp_last_error_time", 19, (ptr)(sXtpLastErrorTime ? sXtpLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_error_age_ms", 21, XS_XtpLastErrorAgeMS());
	xvoTableSetText(objStatus, "xtp_last_idle_close_time", 24, (ptr)(sXtpLastIdleCloseTime ? sXtpLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_idle_close_age_ms", 26, XS_XtpLastIdleCloseAgeMS());
	xvoTableSetText(objStatus, "xtp_last_conn_limit_close_time", 30, (ptr)(sXtpLastConnLimitCloseTime ? sXtpLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_conn_limit_close_age_ms", 32, XS_XtpLastConnLimitCloseAgeMS());
	xvoTableSetInt(objStatus, "udp_recv_count", 14, XS_HttpMetricGet(&g_iXsUdpRecvCount));
	xvoTableSetInt(objStatus, "udp_send_count", 14, XS_HttpMetricGet(&g_iXsUdpSendCount));
	xvoTableSetInt(objStatus, "udp_error_count", 15, XS_HttpMetricGet(&g_iXsUdpErrorCount));
	xvoTableSetInt(objStatus, "udp_recv_bytes", 14, XS_HttpMetricGet(&g_iXsUdpRecvBytes));
	xvoTableSetInt(objStatus, "udp_send_bytes", 14, XS_HttpMetricGet(&g_iXsUdpSendBytes));
	xvoTableSetText(objStatus, "udp_last_from", 13, (ptr)g_sXsUdpLastFrom, 0, FALSE);
	xvoTableSetText(objStatus, "udp_last_text", 13, (ptr)g_sXsUdpLastText, 0, FALSE);
	xvoTableSetInt(objStatus, "udp_last_bytes", 14, g_iXsUdpLastBytes);
	xvoTableSetText(objStatus, "udp_last_time", 13, (ptr)(sUdpLastTime ? sUdpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "udp_last_age_ms", 15, XS_UdpLastAgeMS());
	xvoTableSetInt(objStatus, "udp_last_error_code", 19, g_iXsUdpLastErrorCode);
	xvoTableSetText(objStatus, "udp_last_error_time", 19, (ptr)(sUdpLastErrorTime ? sUdpLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "udp_last_error_age_ms", 21, XS_UdpLastErrorAgeMS());
	xvoTableSetInt(objStatus, "custom_conn_current", 19, XS_HttpMetricGet(&g_iXsCustomConnCurrent));
	xvoTableSetInt(objStatus, "custom_conn_peak", 16, XS_HttpMetricGet(&g_iXsCustomConnPeak));
	xvoTableSetInt(objStatus, "custom_open_count", 17, XS_HttpMetricGet(&g_iXsCustomOpenCount));
	xvoTableSetInt(objStatus, "custom_close_count", 18, XS_HttpMetricGet(&g_iXsCustomCloseCount));
	xvoTableSetInt(objStatus, "custom_error_count", 18, XS_HttpMetricGet(&g_iXsCustomErrorCount));
	xvoTableSetInt(objStatus, "custom_invalid_count", 20, XS_HttpMetricGet(&g_iXsCustomInvalidCount));
	xvoTableSetText(objStatus, "custom_last_invalid_reason", 26, (ptr)g_sXsCustomLastInvalidReason, 0, FALSE);
	xvoTableSetText(objStatus, "custom_last_invalid_time", 24, (ptr)(sCustomLastInvalidTime ? sCustomLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "custom_last_invalid_age_ms", 26, XS_CustomLastInvalidAgeMS());
	xvoTableSetInt(objStatus, "custom_last_close_reason", 24, g_iXsCustomLastCloseReason);
	xvoTableSetText(objStatus, "custom_last_close_time", 22, (ptr)(sCustomLastCloseTime ? sCustomLastCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "custom_last_close_age_ms", 24, XS_CustomLastCloseAgeMS());
	xvoTableSetInt(objStatus, "custom_last_error_code", 22, g_iXsCustomLastErrorCode);
	xvoTableSetText(objStatus, "custom_last_error_time", 22, (ptr)(sCustomLastErrorTime ? sCustomLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "custom_last_error_age_ms", 24, XS_CustomLastErrorAgeMS());
	xvoTableSetInt(objStatus, "custom_recv_count", 17, XS_HttpMetricGet(&g_iXsCustomRecvCount));
	xvoTableSetInt(objStatus, "custom_send_count", 17, XS_HttpMetricGet(&g_iXsCustomSendCount));
	xvoTableSetInt(objStatus, "custom_recv_bytes", 17, XS_HttpMetricGet(&g_iXsCustomRecvBytes));
	xvoTableSetInt(objStatus, "custom_send_bytes", 17, XS_HttpMetricGet(&g_iXsCustomSendBytes));
	xvoTableSetInt(objStatus, "custom_idle_close_count", 23, XS_HttpMetricGet(&g_iXsCustomIdleCloseCount));
	xvoTableSetInt(objStatus, "custom_conn_limit_close_count", 29, XS_HttpMetricGet(&g_iXsCustomConnLimitCloseCount));
	XS_CustomAppendRecvLimitMetrics(objStatus);
	XS_CustomAppendRejectMetrics(objStatus);
	XS_CustomAppendStopCleanupMetrics(objStatus);
	xvoTableSetText(objStatus, "custom_last_remote", 18, (ptr)g_sXsCustomLastRemote, 0, FALSE);
	xvoTableSetInt(objStatus, "custom_last_bytes", 17, g_iXsCustomLastBytes);
	xvoTableSetText(objStatus, "custom_last_text", 16, (ptr)g_sXsCustomLastText, 0, FALSE);
	xvoTableSetText(objStatus, "custom_last_time", 16, (ptr)(sCustomLastTime ? sCustomLastTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "custom_last_age_ms", 18, XS_CustomLastAgeMS());
	xvoTableSetText(objStatus, "custom_last_idle_close_time", 27, (ptr)(sCustomLastIdleCloseTime ? sCustomLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "custom_last_idle_close_age_ms", 29, XS_CustomLastIdleCloseAgeMS());
	xvoTableSetText(objStatus, "custom_last_conn_limit_close_time", 33, (ptr)(sCustomLastConnLimitCloseTime ? sCustomLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "custom_last_conn_limit_close_age_ms", 35, XS_CustomLastConnLimitCloseAgeMS());
	xvoTableSetText(objStatus, "tls_cert_file", 13, (ptr)XS_TlsConfigFileText(objServer->TlsConfig.sCertFile), 0, FALSE);
	xvoTableSetText(objStatus, "tls_key_file", 12, (ptr)XS_TlsConfigFileText(objServer->TlsConfig.sKeyFile), 0, FALSE);
	xvoTableSetText(objStatus, "tls_ca_file", 11, (ptr)XS_TlsConfigFileText(objServer->TlsConfig.sCaFile), 0, FALSE);
	xvoTableSetText(objStatus, "current_dir", 11, (ptr)(sCurrentDir ? sCurrentDir : ""), 0, FALSE);
	xvoTableSetText(objStatus, "config_file", 11, (ptr)(g_sXsConfigFile ? g_sXsConfigFile : ""), 0, FALSE);
	xvoTableSetText(objStatus, "config_name", 11, (ptr)(sConfigName ? sConfigName : ""), 0, FALSE);
	xvoTableSetText(objStatus, "config_mtime", 12, (ptr)(sConfigMTime ? sConfigMTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "config_size", 11, iConfigSize);
	xvoTableSetText(objStatus, "config_base", 11, (ptr)(sConfigBase ? sConfigBase : ""), 0, FALSE);
	xvoTableSetText(objStatus, "app_file", 8, (ptr)(xCore.AppFile ? (char*)xCore.AppFile : ""), 0, FALSE);
	xvoTableSetText(objStatus, "app_path", 8, (ptr)(xCore.AppPath ? (char*)xCore.AppPath : ""), 0, FALSE);
	xvoTableSetText(objStatus, "app_mtime", 9, (ptr)(sAppMTime ? sAppMTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "app_size", 8, iAppSize);
	xvoTableSetText(objStatus, "build", 5, (ptr)XS_BuildVariant(), 0, FALSE);
	xvoTableSetText(objStatus, "compiler", 8, (ptr)XS_CompilerName(), 0, FALSE);
	xvoTableSetText(objStatus, "platform", 8, (ptr)XS_PlatformName(), 0, FALSE);
	xvoTableSetText(objStatus, "arch", 4, (ptr)XS_ArchName(), 0, FALSE);
	xvoTableSetBool(objStatus, "mem_debug", 9, XS_MemDebugEnabled());
	xvoTableSetInt(objStatus, "pid", 3, (int64)XS_ProcessID());
	xvoTableSetText(objStatus, "start_time", 10, (ptr)(sStartTime ? sStartTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "uptime_ms", 9, XS_ProcessUptimeMS());
	xvoTableSetInt(objStatus, "engine_workers", 14, (int64)g_iXsEngineWorkers);
	xvoTableSetInt(objStatus, "runtime_server_count", 20, (int64)g_iXsRuntimeServerCount);
	xvoTableSetInt(objStatus, "http_req_count", 14, XS_HttpMetricGet(&g_iXsHttpReqCount));
	xvoTableSetInt(objStatus, "http_manage_req_count", 21, XS_HttpMetricGet(&g_iXsHttpManageReqCount));
	xvoTableSetInt(objStatus, "http_app_req_count", 18, XS_HttpMetricGet(&g_iXsHttpAppReqCount));
	xvoTableSetInt(objStatus, "http_2xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp2xxCount));
	xvoTableSetInt(objStatus, "http_3xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp3xxCount));
	xvoTableSetInt(objStatus, "http_4xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp4xxCount));
	xvoTableSetInt(objStatus, "http_5xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp5xxCount));
	xvoTableSetInt(objStatus, "http_conn_current", 17, XS_HttpMetricGet(&g_iXsHttpConnCurrent));
	xvoTableSetInt(objStatus, "http_conn_peak", 14, XS_HttpMetricGet(&g_iXsHttpConnPeak));
	xvoTableSetInt(objStatus, "http_idle_close_count", 21, XS_HttpMetricGet(&g_iXsHttpIdleCloseCount));
	xvoTableSetInt(objStatus, "http_conn_limit_close_count", 27, XS_HttpMetricGet(&g_iXsHttpConnLimitCloseCount));
	XS_HttpAppendRequestLimitMetrics(objStatus);
	XS_HttpAppendPolicyRejectMetrics(objStatus);
	XS_HttpAppendManageRejectMetrics(objStatus);
	XS_HttpAppendBusRejectMetrics(objStatus);
	XS_HttpAppendRejectMetrics(objStatus);
	XS_HttpAppendStopCleanupMetrics(objStatus);
	xvoTableSetInt(objStatus, "http_get_count", 14, XS_HttpMetricGet(&g_iXsHttpMethodGetCount));
	xvoTableSetInt(objStatus, "http_post_count", 15, XS_HttpMetricGet(&g_iXsHttpMethodPostCount));
	xvoTableSetInt(objStatus, "http_head_count", 15, XS_HttpMetricGet(&g_iXsHttpMethodHeadCount));
	xvoTableSetInt(objStatus, "http_other_count", 16, XS_HttpMetricGet(&g_iXsHttpMethodOtherCount));
	xvoTableSetInt(objStatus, "http_time_total_ms", 18, XS_HttpMetricGet(&g_iXsHttpTimeTotalMS));
	xvoTableSetInt(objStatus, "http_time_max_ms", 16, XS_HttpMetricGet(&g_iXsHttpTimeMaxMS));
	xvoTableSetInt(
		objStatus,
		"http_time_avg_ms",
		16,
		(XS_HttpMetricGet(&g_iXsHttpReqCount) > 0)
			? (XS_HttpMetricGet(&g_iXsHttpTimeTotalMS) / XS_HttpMetricGet(&g_iXsHttpReqCount))
			: 0
	);
	xvoTableSetInt(objStatus, "http_last_status", 16, XS_HttpMetricGet(&g_iXsHttpLastStatusCode));
	xvoTableSetText(objStatus, "http_last_method", 16, (ptr)XS_HttpLastMethodName(), 0, FALSE);
	xvoTableSetText(objStatus, "http_last_path", 14, (ptr)XS_HttpLastPath(), 0, FALSE);
	xvoTableSetText(objStatus, "http_last_target", 16, (ptr)XS_HttpLastTarget(), 0, FALSE);
	xvoTableSetText(objStatus, "http_last_version", 17, (ptr)g_sXsHttpLastVersion, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_remote", 16, (ptr)XS_HttpLastRemote(), 0, FALSE);
	xvoTableSetText(objStatus, "http_last_host", 14, (ptr)g_sXsHttpLastHost, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_user_agent", 20, (ptr)g_sXsHttpLastUserAgent, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_referer", 17, (ptr)g_sXsHttpLastReferer, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_origin", 16, (ptr)g_sXsHttpLastOrigin, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_accept", 16, (ptr)g_sXsHttpLastAccept, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_accept_encoding", 25, (ptr)g_sXsHttpLastAcceptEncoding, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_cookie", 16, (ptr)g_sXsHttpLastCookie, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_forwarded_for", 23, (ptr)g_sXsHttpLastForwardedFor, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_real_ip", 17, (ptr)g_sXsHttpLastRealIP, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_connection", 20, (ptr)g_sXsHttpLastConnection, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_cache_control", 23, (ptr)g_sXsHttpLastCacheControl, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_content_type", 22, (ptr)g_sXsHttpLastContentType, 0, FALSE);
	xvoTableSetInt(objStatus, "http_last_header_count", 22, g_iXsHttpLastHeaderCount);
	xvoTableSetInt(objStatus, "http_last_query_len", 19, g_iXsHttpLastQueryLen);
	xvoTableSetInt(objStatus, "http_last_body_len", 18, g_iXsHttpLastBodyLen);
	xvoTableSetText(objStatus, "http_last_time", 14, (ptr)(sHttpLastTime ? sHttpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "http_last_age_ms", 16, XS_HttpLastRequestAgeMS());
	xvoTableSetInt(objStatus, "http_last_duration_ms", 21, g_iXsHttpLastTimeMS);
	xvoTableSetText(objStatus, "http_last_idle_close_time", 25, (ptr)(sHttpLastIdleCloseTime ? sHttpLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "http_last_idle_close_age_ms", 27, XS_HttpLastIdleCloseAgeMS());
	xvoTableSetText(objStatus, "http_last_conn_limit_close_time", 31, (ptr)(sHttpLastConnLimitCloseTime ? sHttpLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "http_last_conn_limit_close_age_ms", 33, XS_HttpLastConnLimitCloseAgeMS());
	xvoTableSetInt(objStatus, "http_last_app_status", 20, XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode));
	xvoTableSetText(objStatus, "http_last_app_method", 20, (ptr)XS_HttpLastAppMethodName(), 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_path", 18, (ptr)XS_HttpLastAppPath(), 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_target", 20, (ptr)XS_HttpLastAppTarget(), 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_version", 21, (ptr)g_sXsHttpLastAppVersion, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_remote", 20, (ptr)XS_HttpLastAppRemote(), 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_host", 18, (ptr)g_sXsHttpLastAppHost, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_user_agent", 24, (ptr)g_sXsHttpLastAppUserAgent, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_referer", 21, (ptr)g_sXsHttpLastAppReferer, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_origin", 20, (ptr)g_sXsHttpLastAppOrigin, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_accept", 20, (ptr)g_sXsHttpLastAppAccept, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_accept_encoding", 29, (ptr)g_sXsHttpLastAppAcceptEncoding, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_cookie", 20, (ptr)g_sXsHttpLastAppCookie, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_forwarded_for", 27, (ptr)g_sXsHttpLastAppForwardedFor, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_real_ip", 21, (ptr)g_sXsHttpLastAppRealIP, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_connection", 24, (ptr)g_sXsHttpLastAppConnection, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_cache_control", 27, (ptr)g_sXsHttpLastAppCacheControl, 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_content_type", 26, (ptr)g_sXsHttpLastAppContentType, 0, FALSE);
	xvoTableSetInt(objStatus, "http_last_app_header_count", 26, g_iXsHttpLastAppHeaderCount);
	xvoTableSetInt(objStatus, "http_last_app_query_len", 23, g_iXsHttpLastAppQueryLen);
	xvoTableSetInt(objStatus, "http_last_app_body_len", 22, g_iXsHttpLastAppBodyLen);
	xvoTableSetText(objStatus, "http_last_app_time", 18, (ptr)(sHttpLastAppTime ? sHttpLastAppTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "http_last_app_age_ms", 20, XS_HttpLastAppRequestAgeMS());
	xvoTableSetInt(objStatus, "http_last_app_duration_ms", 25, g_iXsHttpLastAppTimeMS);
	xvoTableSetBool(objStatus, "manage_api", 10, XS_ManageAPIEnabled(objServer, objHost));
	xvoTableSetBool(objStatus, "debug", 5, objServer->Debug);
	xvoTableSetBool(objStatus, "host_aware", 10, objServer->HostAware);
	xvoTableSetBool(objStatus, "default_host", 12, objServer->EnableDefaultHost);
	xvoTableSetInt(objStatus, "host_count", 10, objServer->Hosts ? objServer->Hosts->Count : 0);
	xvoTableSetBool(objStatus, "script_loaded", 13, objServer->EnableDefaultHost ? (objServer->DefaultHost.pScriptState != NULL) : FALSE);

	objHealth = xvoCreateTable();
	xvoTableSetBool(objHealth, "ok", 2, !tReloadStatus.Busy);
	xvoTableSetBool(objHealth, "reload_busy", 11, tReloadStatus.Busy);
	xvoTableSetBool(objHealth, "reload_has_result", 17, tReloadStatus.HasResult);
	xvoTableSetBool(objHealth, "reload_success", 14, tReloadStatus.Success);
	xvoTableSetText(objHealth, "reload_server", 13, (ptr)(tReloadStatus.sServerName[0] ? tReloadStatus.sServerName : "(all)"), 0, FALSE);
	xvoTableSetText(objHealth, "reload_host", 11, (ptr)(tReloadStatus.sHostName[0] ? tReloadStatus.sHostName : "(all)"), 0, FALSE);
	xvoTableSetText(objHealth, "reload_message", 14, (ptr)(tReloadStatus.sMessage[0] ? tReloadStatus.sMessage : "(none)"), 0, FALSE);
	xvoTableSetText(objHealth, "reload_time", 11, (ptr)(sReloadTime ? sReloadTime : ""), 0, FALSE);
	xvoTableSetInt(objHealth, "reload_age_ms", 13, XS_ReloadAgeMSByStatus(&tReloadStatus));
	xvoTableSetInt(objHealth, "reload_total_count", 18, tReloadStatus.iTotalCount);
	xvoTableSetInt(objHealth, "reload_success_count", 20, tReloadStatus.iSuccessCount);
	xvoTableSetInt(objHealth, "reload_failure_count", 20, tReloadStatus.iFailureCount);
	xvoTableSetInt(objHealth, "check_total_count", 17, tCheckStatus.iTotalCount);
	xvoTableSetInt(objHealth, "check_success_count", 19, tCheckStatus.iSuccessCount);
	xvoTableSetInt(objHealth, "check_failure_count", 19, tCheckStatus.iFailureCount);
	xvoTableSetText(objHealth, "check_last_time", 15, (ptr)(sCheckTime ? sCheckTime : ""), 0, FALSE);
	xvoTableSetInt(objHealth, "check_last_age_ms", 17, XS_CheckConfigAgeMSByStatus(&tCheckStatus));
	XS_HttpAppendCheckConfigSnapshotByStatus(objHealth, &tCheckStatus);
	xvoTableSetInt(objHealth, "http_req_count", 14, XS_HttpMetricGet(&g_iXsHttpReqCount));
	xvoTableSetInt(objHealth, "http_manage_req_count", 21, XS_HttpMetricGet(&g_iXsHttpManageReqCount));
	xvoTableSetInt(objHealth, "http_app_req_count", 18, XS_HttpMetricGet(&g_iXsHttpAppReqCount));
	xvoTableSetInt(objHealth, "http_2xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp2xxCount));
	xvoTableSetInt(objHealth, "http_3xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp3xxCount));
	xvoTableSetInt(objHealth, "http_4xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp4xxCount));
	xvoTableSetInt(objHealth, "http_5xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp5xxCount));
	xvoTableSetInt(objHealth, "http_conn_current", 17, XS_HttpMetricGet(&g_iXsHttpConnCurrent));
	xvoTableSetInt(objHealth, "http_conn_peak", 14, XS_HttpMetricGet(&g_iXsHttpConnPeak));
	xvoTableSetInt(objHealth, "http_idle_close_count", 21, XS_HttpMetricGet(&g_iXsHttpIdleCloseCount));
	xvoTableSetInt(objHealth, "http_conn_limit_close_count", 27, XS_HttpMetricGet(&g_iXsHttpConnLimitCloseCount));
	XS_HttpAppendRequestLimitMetrics(objHealth);
	XS_HttpAppendPolicyRejectMetrics(objHealth);
	XS_HttpAppendManageRejectMetrics(objHealth);
	XS_HttpAppendBusRejectMetrics(objHealth);
	XS_HttpAppendRejectMetrics(objHealth);
	XS_HttpAppendStopCleanupMetrics(objHealth);
	xvoTableSetInt(objHealth, "http_time_total_ms", 18, XS_HttpMetricGet(&g_iXsHttpTimeTotalMS));
	xvoTableSetInt(objHealth, "http_time_max_ms", 16, XS_HttpMetricGet(&g_iXsHttpTimeMaxMS));
	xvoTableSetInt(
		objHealth,
		"http_time_avg_ms",
		16,
		(XS_HttpMetricGet(&g_iXsHttpReqCount) > 0)
			? (XS_HttpMetricGet(&g_iXsHttpTimeTotalMS) / XS_HttpMetricGet(&g_iXsHttpReqCount))
			: 0
	);
	xvoTableSetInt(objHealth, "http_last_status", 16, XS_HttpMetricGet(&g_iXsHttpLastStatusCode));
	xvoTableSetText(objHealth, "http_last_method", 16, (ptr)XS_HttpLastMethodName(), 0, FALSE);
	xvoTableSetText(objHealth, "http_last_path", 14, (ptr)XS_HttpLastPath(), 0, FALSE);
	xvoTableSetText(objHealth, "http_last_target", 16, (ptr)XS_HttpLastTarget(), 0, FALSE);
	xvoTableSetText(objHealth, "http_last_version", 17, (ptr)g_sXsHttpLastVersion, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_remote", 16, (ptr)XS_HttpLastRemote(), 0, FALSE);
	xvoTableSetText(objHealth, "http_last_host", 14, (ptr)g_sXsHttpLastHost, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_user_agent", 20, (ptr)g_sXsHttpLastUserAgent, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_referer", 17, (ptr)g_sXsHttpLastReferer, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_origin", 16, (ptr)g_sXsHttpLastOrigin, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_accept", 16, (ptr)g_sXsHttpLastAccept, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_accept_encoding", 25, (ptr)g_sXsHttpLastAcceptEncoding, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_cookie", 16, (ptr)g_sXsHttpLastCookie, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_forwarded_for", 23, (ptr)g_sXsHttpLastForwardedFor, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_real_ip", 17, (ptr)g_sXsHttpLastRealIP, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_connection", 20, (ptr)g_sXsHttpLastConnection, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_cache_control", 23, (ptr)g_sXsHttpLastCacheControl, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_content_type", 22, (ptr)g_sXsHttpLastContentType, 0, FALSE);
	xvoTableSetInt(objHealth, "http_last_header_count", 22, g_iXsHttpLastHeaderCount);
	xvoTableSetInt(objHealth, "http_last_query_len", 19, g_iXsHttpLastQueryLen);
	xvoTableSetInt(objHealth, "http_last_body_len", 18, g_iXsHttpLastBodyLen);
	xvoTableSetText(objHealth, "http_last_time", 14, (ptr)(sHttpLastTime ? sHttpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objHealth, "http_last_age_ms", 16, XS_HttpLastRequestAgeMS());
	xvoTableSetInt(objHealth, "http_last_duration_ms", 21, g_iXsHttpLastTimeMS);
	xvoTableSetText(objHealth, "http_last_idle_close_time", 25, (ptr)(sHttpLastIdleCloseTime ? sHttpLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objHealth, "http_last_idle_close_age_ms", 27, XS_HttpLastIdleCloseAgeMS());
	xvoTableSetText(objHealth, "http_last_conn_limit_close_time", 31, (ptr)(sHttpLastConnLimitCloseTime ? sHttpLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objHealth, "http_last_conn_limit_close_age_ms", 33, XS_HttpLastConnLimitCloseAgeMS());
	xvoTableSetInt(objHealth, "http_last_app_status", 20, XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode));
	xvoTableSetText(objHealth, "http_last_app_method", 20, (ptr)XS_HttpLastAppMethodName(), 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_path", 18, (ptr)XS_HttpLastAppPath(), 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_target", 20, (ptr)XS_HttpLastAppTarget(), 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_version", 21, (ptr)g_sXsHttpLastAppVersion, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_remote", 20, (ptr)XS_HttpLastAppRemote(), 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_host", 18, (ptr)g_sXsHttpLastAppHost, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_user_agent", 24, (ptr)g_sXsHttpLastAppUserAgent, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_referer", 21, (ptr)g_sXsHttpLastAppReferer, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_origin", 20, (ptr)g_sXsHttpLastAppOrigin, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_accept", 20, (ptr)g_sXsHttpLastAppAccept, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_accept_encoding", 29, (ptr)g_sXsHttpLastAppAcceptEncoding, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_cookie", 20, (ptr)g_sXsHttpLastAppCookie, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_forwarded_for", 27, (ptr)g_sXsHttpLastAppForwardedFor, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_real_ip", 21, (ptr)g_sXsHttpLastAppRealIP, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_connection", 24, (ptr)g_sXsHttpLastAppConnection, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_cache_control", 27, (ptr)g_sXsHttpLastAppCacheControl, 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_content_type", 26, (ptr)g_sXsHttpLastAppContentType, 0, FALSE);
	xvoTableSetInt(objHealth, "http_last_app_header_count", 26, g_iXsHttpLastAppHeaderCount);
	xvoTableSetInt(objHealth, "http_last_app_query_len", 23, g_iXsHttpLastAppQueryLen);
	xvoTableSetInt(objHealth, "http_last_app_body_len", 22, g_iXsHttpLastAppBodyLen);
	xvoTableSetText(objHealth, "http_last_app_time", 18, (ptr)(sHttpLastAppTime ? sHttpLastAppTime : ""), 0, FALSE);
	xvoTableSetInt(objHealth, "http_last_app_age_ms", 20, XS_HttpLastAppRequestAgeMS());
	xvoTableSetInt(objHealth, "http_last_app_duration_ms", 25, g_iXsHttpLastAppTimeMS);

	objReload = xvoCreateTable();
	xvoTableSetBool(objReload, "busy", 4, tReloadStatus.Busy);
	xvoTableSetBool(objReload, "has_result", 10, tReloadStatus.HasResult);
	xvoTableSetBool(objReload, "success", 7, tReloadStatus.Success);
	xvoTableSetText(objReload, "server", 6, (ptr)(tReloadStatus.sServerName[0] ? tReloadStatus.sServerName : "(all)"), 0, FALSE);
	xvoTableSetText(objReload, "host", 4, (ptr)(tReloadStatus.sHostName[0] ? tReloadStatus.sHostName : "(all)"), 0, FALSE);
	xvoTableSetText(objReload, "message", 7, (ptr)(tReloadStatus.sMessage[0] ? tReloadStatus.sMessage : "(none)"), 0, FALSE);
	xvoTableSetText(objReload, "reload_time", 11, (ptr)(sReloadTime ? sReloadTime : ""), 0, FALSE);
	xvoTableSetInt(objReload, "reload_age_ms", 13, XS_ReloadAgeMSByStatus(&tReloadStatus));
	xvoTableSetInt(objReload, "reload_total_count", 18, tReloadStatus.iTotalCount);
	xvoTableSetInt(objReload, "reload_success_count", 20, tReloadStatus.iSuccessCount);
	xvoTableSetInt(objReload, "reload_failure_count", 20, tReloadStatus.iFailureCount);

	objCheck = xvoCreateTable();
	xvoTableSetBool(objCheck, "has_result", 10, tCheckStatus.HasResult);
	xvoTableSetBool(objCheck, "result", 6, tCheckStatus.LastResult);
	xvoTableSetText(objCheck, "file", 4, (ptr)tCheckStatus.sLastFile, 0, FALSE);
	xvoTableSetText(objCheck, "base", 4, (ptr)tCheckStatus.sLastBase, 0, FALSE);
	xvoTableSetInt(objCheck, "server_count", 12, tCheckStatus.iLastServerCount);
	xvoTableSetText(objCheck, "message", 7, (ptr)tCheckStatus.sLastMessage, 0, FALSE);
	xvoTableSetInt(objCheck, "check_total_count", 17, tCheckStatus.iTotalCount);
	xvoTableSetInt(objCheck, "check_success_count", 19, tCheckStatus.iSuccessCount);
	xvoTableSetInt(objCheck, "check_failure_count", 19, tCheckStatus.iFailureCount);
	xvoTableSetText(objCheck, "check_last_time", 15, (ptr)(sCheckTime ? sCheckTime : ""), 0, FALSE);
	xvoTableSetInt(objCheck, "check_last_age_ms", 17, XS_CheckConfigAgeMSByStatus(&tCheckStatus));

	objBus = XS_BusBuildNamespaceStatsValue();
	if ( objBus == NULL ) {
		xvoUnref(objStatus);
		xvoUnref(objHealth);
		xvoUnref(objReload);
		xvoUnref(objCheck);
		XS_FreeConfig(&objCfgCheck);
		if ( sCurrentDir ) {
			xrtFree(sCurrentDir);
		}
		if ( sConfigName ) {
			xrtFree(sConfigName);
		}
		if ( sReloadTime ) {
			xrtFree(sReloadTime);
		}
		if ( sCheckTime ) {
			xrtFree(sCheckTime);
		}
		if ( sHttpLastTime ) {
			xrtFree(sHttpLastTime);
		}
		if ( sHttpLastAppTime ) {
			xrtFree(sHttpLastAppTime);
		}
		if ( sHttpLastIdleCloseTime ) {
			xrtFree(sHttpLastIdleCloseTime);
		}
		if ( sHttpLastConnLimitCloseTime ) {
			xrtFree(sHttpLastConnLimitCloseTime);
		}
		if ( sWsLastTime ) {
			xrtFree(sWsLastTime);
		}
		if ( sWsLastCloseTime ) {
			xrtFree(sWsLastCloseTime);
		}
		if ( sWsLastErrorTime ) {
			xrtFree(sWsLastErrorTime);
		}
		if ( sWsLastInvalidTime ) {
			xrtFree(sWsLastInvalidTime);
		}
		if ( sWsLastIdleCloseTime ) {
			xrtFree(sWsLastIdleCloseTime);
		}
		if ( sWsLastConnLimitCloseTime ) {
			xrtFree(sWsLastConnLimitCloseTime);
		}
		if ( sXtpLastTime ) {
			xrtFree(sXtpLastTime);
		}
		if ( sXtpLastInvalidTime ) {
			xrtFree(sXtpLastInvalidTime);
		}
		if ( sXtpLastErrorTime ) {
			xrtFree(sXtpLastErrorTime);
		}
		if ( sXtpLastIdleCloseTime ) {
			xrtFree(sXtpLastIdleCloseTime);
		}
		if ( sXtpLastConnLimitCloseTime ) {
			xrtFree(sXtpLastConnLimitCloseTime);
		}
		if ( sUdpLastTime ) {
			xrtFree(sUdpLastTime);
		}
		if ( sUdpLastErrorTime ) {
			xrtFree(sUdpLastErrorTime);
		}
		if ( sCustomLastTime ) {
			xrtFree(sCustomLastTime);
		}
		if ( sCustomLastErrorTime ) {
			xrtFree(sCustomLastErrorTime);
		}
		if ( sCustomLastInvalidTime ) {
			xrtFree(sCustomLastInvalidTime);
		}
		if ( sCustomLastIdleCloseTime ) {
			xrtFree(sCustomLastIdleCloseTime);
		}
		if ( sCustomLastConnLimitCloseTime ) {
			xrtFree(sCustomLastConnLimitCloseTime);
		}
		if ( sAppMTime ) {
			xrtFree(sAppMTime);
		}
		if ( sConfigMTime ) {
			xrtFree(sConfigMTime);
		}
		if ( sCheckBase ) {
			xrtFree(sCheckBase);
		}
		if ( sConfigBase ) {
			xrtFree(sConfigBase);
		}
		if ( sStartTime ) {
			xrtFree(sStartTime);
		}
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "dashboard bus build failed");
	}

	objRet = xvoCreateTable();
	xvoTableSetValue(objRet, "status", 6, objStatus, TRUE);
	xvoTableSetValue(objRet, "health", 6, objHealth, TRUE);
	xvoTableSetValue(objRet, "reload", 6, objReload, TRUE);
	xvoTableSetValue(objRet, "check_config", 12, objCheck, TRUE);
	xvoTableSetValue(objRet, "bus", 3, objBus, TRUE);

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	XS_FreeConfig(&objCfgCheck);
	if ( sCurrentDir ) {
		xrtFree(sCurrentDir);
	}
	if ( sConfigName ) {
		xrtFree(sConfigName);
	}
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	if ( sCheckTime ) {
		xrtFree(sCheckTime);
	}
	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
	}
	if ( sHttpLastIdleCloseTime ) {
		xrtFree(sHttpLastIdleCloseTime);
	}
	if ( sHttpLastConnLimitCloseTime ) {
		xrtFree(sHttpLastConnLimitCloseTime);
	}
	if ( sWsLastTime ) {
		xrtFree(sWsLastTime);
	}
	if ( sWsLastCloseTime ) {
		xrtFree(sWsLastCloseTime);
	}
	if ( sWsLastErrorTime ) {
		xrtFree(sWsLastErrorTime);
	}
	if ( sWsLastInvalidTime ) {
		xrtFree(sWsLastInvalidTime);
	}
	if ( sWsLastIdleCloseTime ) {
		xrtFree(sWsLastIdleCloseTime);
	}
	if ( sWsLastConnLimitCloseTime ) {
		xrtFree(sWsLastConnLimitCloseTime);
	}
	if ( sXtpLastTime ) {
		xrtFree(sXtpLastTime);
	}
	if ( sXtpLastInvalidTime ) {
		xrtFree(sXtpLastInvalidTime);
	}
	if ( sXtpLastErrorTime ) {
		xrtFree(sXtpLastErrorTime);
	}
	if ( sXtpLastIdleCloseTime ) {
		xrtFree(sXtpLastIdleCloseTime);
	}
	if ( sXtpLastConnLimitCloseTime ) {
		xrtFree(sXtpLastConnLimitCloseTime);
	}
	if ( sUdpLastTime ) {
		xrtFree(sUdpLastTime);
	}
	if ( sUdpLastErrorTime ) {
		xrtFree(sUdpLastErrorTime);
	}
	if ( sCustomLastTime ) {
		xrtFree(sCustomLastTime);
	}
	if ( sCustomLastCloseTime ) {
		xrtFree(sCustomLastCloseTime);
	}
	if ( sCustomLastErrorTime ) {
		xrtFree(sCustomLastErrorTime);
	}
	if ( sCustomLastInvalidTime ) {
		xrtFree(sCustomLastInvalidTime);
	}
	if ( sCustomLastIdleCloseTime ) {
		xrtFree(sCustomLastIdleCloseTime);
	}
	if ( sCustomLastConnLimitCloseTime ) {
		xrtFree(sCustomLastConnLimitCloseTime);
	}
	if ( sAppMTime ) {
		xrtFree(sAppMTime);
	}
	if ( sConfigMTime ) {
		xrtFree(sConfigMTime);
	}
	if ( sConfigBase ) {
		xrtFree(sConfigBase);
	}
	if ( sStartTime ) {
		xrtFree(sStartTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "dashboard json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, 200, "OK");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleHealth(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ReloadStatusSnapshot tReloadStatus;
	XS_CheckConfigStatusSnapshot tCheckStatus;
	char sBody[16384];
	char* sCheckTime;
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sHttpLastIdleCloseTime;
	char* sHttpLastConnLimitCloseTime;
	char* sHttpLastRejectTime;
	char* sHttpLastStopCleanupTime;
	char* sHttpLastHeaderLimitRejectTime;
	char* sHttpLastBodyLimitRejectTime;
	char* sHttpLastPathLimitRejectTime;
	char* sHttpLastApiDisabledRejectTime;
	char* sHttpLastMethodRejectTime;
	char* sHttpLastHostNotFoundRejectTime;
	char* sHttpLastReloadBusyRejectTime;
	char* sHttpLastReloadFailedRejectTime;
	char* sHttpLastCheckConfigFailedRejectTime;
	char* sHttpLastBusBadRequestRejectTime;
	char* sHttpLastBusNotFoundRejectTime;
	char* sHttpLastBusLimitRejectTime;
	char* sHttpLastBusFailedRejectTime;
	char* sWsLastMessageLimitCloseTime;
	char* sWsLastRejectTime;
	char* sWsLastStopCleanupTime;
	char* sXtpLastRecvLimitCloseTime;
	char* sXtpLastRejectTime;
	char* sXtpLastStopCleanupTime;
	char* sCustomLastRecvLimitCloseTime;
	char* sCustomLastRejectTime;
	char* sCustomLastStopCleanupTime;
	char* sReloadTime;
	char* sBusQueueTime;
	char* sBusDispatchTime;
	char* sBusLastSweepTime;
	char* sBusLastCleanupTime;
	xvalue objBus;
	int64 iBusQueueCount;
	int64 iBusDataCount;
	int64 iBusTotalQueued;
	int64 iBusTotalDelivered;
	int64 iBusTotalDropped;
	int64 tBusLastQueue;
	int64 tBusLastDispatch;
	int64 iBusSweepIntervalMS;
	int64 iBusSweepBatchLimit;
	int64 iBusSweepCount;
	int64 iBusSweepRemovedCount;
	int64 tBusLastSweep;
	int64 iBusLastSweepRemoved;
	int64 iBusLastSweepRemain;
	int64 iBusCleanupCount;
	int64 tBusLastCleanup;
	int64 iBusLastCleanupRemoved;
	int64 iBusLastCleanupRemain;
	bool bOk;
	bool bScriptLoaded;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/health") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "health api disabled");
	}

	bOk = XS_HttpServerHealthy(objServer, objHost);
	bScriptLoaded = FALSE;
	if ( objServer->EnableDefaultHost && objServer->DefaultHost.pScriptState ) {
		bScriptLoaded = TRUE;
	}
	if ( !bScriptLoaded && objHost->pScriptState ) {
		bScriptLoaded = TRUE;
	}
	XS_ConfigReloadStatusSnapshot(&tReloadStatus);
	XS_ConfigCheckStatusSnapshot(&tCheckStatus);
	sCheckTime = XS_CheckConfigTimeTextByStatus(&tCheckStatus);
	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sHttpLastIdleCloseTime = XS_HttpLastIdleCloseTimeText();
	sHttpLastConnLimitCloseTime = XS_HttpLastConnLimitCloseTimeText();
	sHttpLastRejectTime = XS_HttpLastRejectTimeText();
	sHttpLastStopCleanupTime = XS_HttpLastStopCleanupTimeText();
	sHttpLastHeaderLimitRejectTime = XS_HttpLastHeaderLimitRejectTimeText();
	sHttpLastBodyLimitRejectTime = XS_HttpLastBodyLimitRejectTimeText();
	sHttpLastPathLimitRejectTime = XS_HttpLastPathLimitRejectTimeText();
	sHttpLastApiDisabledRejectTime = XS_HttpLastApiDisabledRejectTimeText();
	sHttpLastMethodRejectTime = XS_HttpLastMethodRejectTimeText();
	sHttpLastHostNotFoundRejectTime = XS_HttpLastHostNotFoundRejectTimeText();
	sHttpLastReloadBusyRejectTime = XS_HttpLastReloadBusyRejectTimeText();
	sHttpLastReloadFailedRejectTime = XS_HttpLastReloadFailedRejectTimeText();
	sHttpLastCheckConfigFailedRejectTime = XS_HttpLastCheckConfigFailedRejectTimeText();
	sHttpLastBusBadRequestRejectTime = XS_HttpLastBusBadRequestRejectTimeText();
	sHttpLastBusNotFoundRejectTime = XS_HttpLastBusNotFoundRejectTimeText();
	sHttpLastBusLimitRejectTime = XS_HttpLastBusLimitRejectTimeText();
	sHttpLastBusFailedRejectTime = XS_HttpLastBusFailedRejectTimeText();
	sWsLastMessageLimitCloseTime = XS_WsLastMessageLimitCloseTimeText();
	sWsLastRejectTime = XS_WsLastRejectTimeText();
	sWsLastStopCleanupTime = XS_WsLastStopCleanupTimeText();
	sXtpLastRecvLimitCloseTime = XS_XtpLastRecvLimitCloseTimeText();
	sXtpLastRejectTime = XS_XtpLastRejectTimeText();
	sXtpLastStopCleanupTime = XS_XtpLastStopCleanupTimeText();
	sCustomLastRecvLimitCloseTime = XS_CustomLastRecvLimitCloseTimeText();
	sCustomLastRejectTime = XS_CustomLastRejectTimeText();
	sCustomLastStopCleanupTime = XS_CustomLastStopCleanupTimeText();
	sReloadTime = XS_ReloadTimeTextByStatus(&tReloadStatus);
	sBusQueueTime = NULL;
	sBusDispatchTime = NULL;
	sBusLastSweepTime = NULL;
	sBusLastCleanupTime = NULL;
	objBus = XS_BusBuildNamespaceStatsValue();
	iBusQueueCount = 0;
	iBusDataCount = 0;
	iBusTotalQueued = 0;
	iBusTotalDelivered = 0;
	iBusTotalDropped = 0;
	tBusLastQueue = 0;
	tBusLastDispatch = 0;
	iBusSweepIntervalMS = 0;
	iBusSweepBatchLimit = 0;
	iBusSweepCount = 0;
	iBusSweepRemovedCount = 0;
	tBusLastSweep = 0;
	iBusLastSweepRemoved = 0;
	iBusLastSweepRemain = 0;
	iBusCleanupCount = 0;
	tBusLastCleanup = 0;
	iBusLastCleanupRemoved = 0;
	iBusLastCleanupRemain = 0;
	if ( objBus ) {
		iBusQueueCount = xvoTableGetInt(objBus, "queue_count", 11);
		iBusDataCount = xvoTableGetInt(objBus, "data_count", 10);
		iBusTotalQueued = xvoTableGetInt(objBus, "total_queued", 12);
		iBusTotalDelivered = xvoTableGetInt(objBus, "total_delivered", 15);
		iBusTotalDropped = xvoTableGetInt(objBus, "total_dropped", 13);
		tBusLastQueue = xvoTableGetInt(objBus, "last_queue_time", 15);
		tBusLastDispatch = xvoTableGetInt(objBus, "last_dispatch_time", 18);
		iBusSweepIntervalMS = xvoTableGetInt(objBus, "sweep_interval_ms", sizeof("sweep_interval_ms") - 1);
		iBusSweepBatchLimit = xvoTableGetInt(objBus, "sweep_batch_limit", sizeof("sweep_batch_limit") - 1);
		iBusSweepCount = xvoTableGetInt(objBus, "sweep_count", sizeof("sweep_count") - 1);
		iBusSweepRemovedCount = xvoTableGetInt(objBus, "sweep_removed_count", sizeof("sweep_removed_count") - 1);
		tBusLastSweep = xvoTableGetInt(objBus, "last_sweep_time", sizeof("last_sweep_time") - 1);
		iBusLastSweepRemoved = xvoTableGetInt(objBus, "last_sweep_removed", sizeof("last_sweep_removed") - 1);
		iBusLastSweepRemain = xvoTableGetInt(objBus, "last_sweep_remain", sizeof("last_sweep_remain") - 1);
		iBusCleanupCount = xvoTableGetInt(objBus, "cleanup_count", sizeof("cleanup_count") - 1);
		tBusLastCleanup = xvoTableGetInt(objBus, "last_cleanup_time", sizeof("last_cleanup_time") - 1);
		iBusLastCleanupRemoved = xvoTableGetInt(objBus, "last_cleanup_removed", sizeof("last_cleanup_removed") - 1);
		iBusLastCleanupRemain = xvoTableGetInt(objBus, "last_cleanup_remain", sizeof("last_cleanup_remain") - 1);
		sBusQueueTime = (tBusLastQueue > 0) ? xrtTimeToStr(tBusLastQueue, XRT_TIME_FORMAT_DATETIME) : NULL;
		sBusDispatchTime = (tBusLastDispatch > 0) ? xrtTimeToStr(tBusLastDispatch, XRT_TIME_FORMAT_DATETIME) : NULL;
		sBusLastSweepTime = (tBusLastSweep > 0) ? xrtTimeToStr(tBusLastSweep, XRT_TIME_FORMAT_DATETIME) : NULL;
		sBusLastCleanupTime = (tBusLastCleanup > 0) ? xrtTimeToStr(tBusLastCleanup, XRT_TIME_FORMAT_DATETIME) : NULL;
	}
	snprintf(
		sBody,
		sizeof(sBody),
		"ok=%s\nserver=%s\nclass=%s\naddr=%s\nreload_busy=%s\nreload_message=%s\nreload_time=%s\nreload_age_ms=%lld\nreload_total_count=%lld\nreload_success_count=%lld\nreload_failure_count=%lld\ncheck_total_count=%lld\ncheck_success_count=%lld\ncheck_failure_count=%lld\ncheck_last_time=%s\ncheck_last_age_ms=%lld\nbus_queue_count=%lld\nbus_data_count=%lld\nbus_total_queued=%lld\nbus_total_delivered=%lld\nbus_total_dropped=%lld\nbus_last_queue_time=%lld\nbus_last_queue_time_text=%s\nbus_last_dispatch_time=%lld\nbus_last_dispatch_time_text=%s\nbus_sweep_interval_ms=%lld\nbus_sweep_batch_limit=%lld\nbus_sweep_count=%lld\nbus_sweep_removed_count=%lld\nbus_last_sweep_time=%lld\nbus_last_sweep_time_text=%s\nbus_last_sweep_age_ms=%lld\nbus_last_sweep_removed=%lld\nbus_last_sweep_remain=%lld\nbus_cleanup_count=%lld\nbus_last_cleanup_time=%lld\nbus_last_cleanup_time_text=%s\nbus_last_cleanup_age_ms=%lld\nbus_last_cleanup_removed=%lld\nbus_last_cleanup_remain=%lld\nhttp_req_count=%lld\nhttp_manage_req_count=%lld\nhttp_app_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_conn_current=%lld\nhttp_conn_peak=%lld\nhttp_time_total_ms=%lld\nhttp_time_max_ms=%lld\nhttp_time_avg_ms=%lld\nhttp_last_method=%s\nhttp_last_status=%lld\nhttp_last_path=%s\nhttp_last_target=%s\nhttp_last_remote=%s\nhttp_last_body_len=%lld\nhttp_last_header_count=%lld\nhttp_last_query_len=%lld\nhttp_last_time=%s\nhttp_last_age_ms=%lld\nhttp_last_duration_ms=%lld\nhttp_last_app_method=%s\nhttp_last_app_status=%lld\nhttp_last_app_path=%s\nhttp_last_app_target=%s\nhttp_last_app_remote=%s\nhttp_last_app_body_len=%lld\nhttp_last_app_header_count=%lld\nhttp_last_app_query_len=%lld\nhttp_last_app_time=%s\nhttp_last_app_age_ms=%lld\nhttp_last_app_duration_ms=%lld\n",
		bOk ? "true" : "false",
		objServer->Name ? objServer->Name : "(null)",
		XS_ServerClassName(objServer->Class),
		objServer->Addr ? objServer->Addr : "(null)",
		tReloadStatus.Busy ? "true" : "false",
		tReloadStatus.sMessage[0] ? tReloadStatus.sMessage : "(none)",
		sReloadTime ? sReloadTime : "(none)",
		(long long)XS_ReloadAgeMSByStatus(&tReloadStatus),
		(long long)tReloadStatus.iTotalCount,
		(long long)tReloadStatus.iSuccessCount,
		(long long)tReloadStatus.iFailureCount,
		(long long)tCheckStatus.iTotalCount,
		(long long)tCheckStatus.iSuccessCount,
		(long long)tCheckStatus.iFailureCount,
		sCheckTime ? sCheckTime : "(none)",
		(long long)XS_CheckConfigAgeMSByStatus(&tCheckStatus),
		(long long)iBusQueueCount,
		(long long)iBusDataCount,
		(long long)iBusTotalQueued,
		(long long)iBusTotalDelivered,
		(long long)iBusTotalDropped,
		(long long)tBusLastQueue,
		sBusQueueTime ? sBusQueueTime : "(none)",
		(long long)tBusLastDispatch,
		sBusDispatchTime ? sBusDispatchTime : "(none)",
		(long long)iBusSweepIntervalMS,
		(long long)iBusSweepBatchLimit,
		(long long)iBusSweepCount,
		(long long)iBusSweepRemovedCount,
		(long long)tBusLastSweep,
		sBusLastSweepTime ? sBusLastSweepTime : "(none)",
		(long long)((tBusLastSweep > 0) ? ((xrtNow() - tBusLastSweep) * 1000) : -1),
		(long long)iBusLastSweepRemoved,
		(long long)iBusLastSweepRemain,
		(long long)iBusCleanupCount,
		(long long)tBusLastCleanup,
		sBusLastCleanupTime ? sBusLastCleanupTime : "(none)",
		(long long)((tBusLastCleanup > 0) ? ((xrtNow() - tBusLastCleanup) * 1000) : -1),
		(long long)iBusLastCleanupRemoved,
		(long long)iBusLastCleanupRemain,
		(long long)XS_HttpMetricGet(&g_iXsHttpReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpManageReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpAppReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp2xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp3xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp4xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp5xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsHttpTimeTotalMS),
		(long long)XS_HttpMetricGet(&g_iXsHttpTimeMaxMS),
		(long long)((XS_HttpMetricGet(&g_iXsHttpReqCount) > 0) ? (XS_HttpMetricGet(&g_iXsHttpTimeTotalMS) / XS_HttpMetricGet(&g_iXsHttpReqCount)) : 0),
		XS_HttpLastMethodName()[0] ? XS_HttpLastMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastStatusCode),
		XS_HttpLastPath()[0] ? XS_HttpLastPath() : "(none)",
		XS_HttpLastTarget()[0] ? XS_HttpLastTarget() : "(none)",
		XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : "(none)",
		(long long)g_iXsHttpLastBodyLen,
		(long long)g_iXsHttpLastHeaderCount,
		(long long)g_iXsHttpLastQueryLen,
		sHttpLastTime ? sHttpLastTime : "(none)",
		(long long)XS_HttpLastRequestAgeMS(),
		(long long)g_iXsHttpLastTimeMS,
		XS_HttpLastAppMethodName()[0] ? XS_HttpLastAppMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode),
		XS_HttpLastAppPath()[0] ? XS_HttpLastAppPath() : "(none)",
		XS_HttpLastAppTarget()[0] ? XS_HttpLastAppTarget() : "(none)",
		XS_HttpLastAppRemote()[0] ? XS_HttpLastAppRemote() : "(none)",
		(long long)g_iXsHttpLastAppBodyLen,
		(long long)g_iXsHttpLastAppHeaderCount,
		(long long)g_iXsHttpLastAppQueryLen,
		sHttpLastAppTime ? sHttpLastAppTime : "(none)",
		(long long)XS_HttpLastAppRequestAgeMS(),
		(long long)g_iXsHttpLastAppTimeMS
	);
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"check_has_result=%s\ncheck_last_result=%s\ncheck_last_file=%s\ncheck_last_base=%s\ncheck_last_server_count=%lld\ncheck_last_message=%s\n",
			tCheckStatus.HasResult ? "true" : "false",
			tCheckStatus.HasResult ? (tCheckStatus.LastResult ? "true" : "false") : "(none)",
			tCheckStatus.sLastFile[0] ? tCheckStatus.sLastFile : "(none)",
			tCheckStatus.sLastBase[0] ? tCheckStatus.sLastBase : "(none)",
			(long long)tCheckStatus.iLastServerCount,
			tCheckStatus.sLastMessage[0] ? tCheckStatus.sLastMessage : "(none)"
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"debug=%s\nhost_aware=%s\nhost_count=%u\nscript_loaded=%s\nreload_has_result=%s\nreload_success=%s\nreload_server=%s\nreload_host=%s\n",
			objServer->Debug ? "true" : "false",
			objServer->HostAware ? "true" : "false",
			(unsigned int)(objServer->Hosts ? objServer->Hosts->Count : 0),
			bScriptLoaded ? "true" : "false",
			tReloadStatus.HasResult ? "true" : "false",
			tReloadStatus.Success ? "true" : "false",
			tReloadStatus.sServerName[0] ? tReloadStatus.sServerName : "(all)",
			tReloadStatus.sHostName[0] ? tReloadStatus.sHostName : "(all)"
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_get_count=%lld\nhttp_post_count=%lld\nhttp_head_count=%lld\nhttp_other_count=%lld\nhttp_last_version=%s\nhttp_last_host=%s\nhttp_last_user_agent=%s\nhttp_last_referer=%s\nhttp_last_origin=%s\nhttp_last_accept=%s\nhttp_last_accept_encoding=%s\nhttp_last_cookie=%s\nhttp_last_forwarded_for=%s\nhttp_last_real_ip=%s\nhttp_last_connection=%s\nhttp_last_cache_control=%s\nhttp_last_content_type=%s\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodGetCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodPostCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodHeadCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodOtherCount),
			g_sXsHttpLastVersion[0] ? g_sXsHttpLastVersion : "(none)",
			g_sXsHttpLastHost[0] ? g_sXsHttpLastHost : "(none)",
			g_sXsHttpLastUserAgent[0] ? g_sXsHttpLastUserAgent : "(none)",
			g_sXsHttpLastReferer[0] ? g_sXsHttpLastReferer : "(none)",
			g_sXsHttpLastOrigin[0] ? g_sXsHttpLastOrigin : "(none)",
			g_sXsHttpLastAccept[0] ? g_sXsHttpLastAccept : "(none)",
			g_sXsHttpLastAcceptEncoding[0] ? g_sXsHttpLastAcceptEncoding : "(none)",
			g_sXsHttpLastCookie[0] ? g_sXsHttpLastCookie : "(none)",
			g_sXsHttpLastForwardedFor[0] ? g_sXsHttpLastForwardedFor : "(none)",
			g_sXsHttpLastRealIP[0] ? g_sXsHttpLastRealIP : "(none)",
			g_sXsHttpLastConnection[0] ? g_sXsHttpLastConnection : "(none)",
			g_sXsHttpLastCacheControl[0] ? g_sXsHttpLastCacheControl : "(none)",
			g_sXsHttpLastContentType[0] ? g_sXsHttpLastContentType : "(none)"
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_last_app_version=%s\nhttp_last_app_host=%s\nhttp_last_app_user_agent=%s\nhttp_last_app_referer=%s\nhttp_last_app_origin=%s\nhttp_last_app_accept=%s\nhttp_last_app_accept_encoding=%s\nhttp_last_app_cookie=%s\nhttp_last_app_forwarded_for=%s\nhttp_last_app_real_ip=%s\nhttp_last_app_connection=%s\nhttp_last_app_cache_control=%s\nhttp_last_app_content_type=%s\n",
			g_sXsHttpLastAppVersion[0] ? g_sXsHttpLastAppVersion : "(none)",
			g_sXsHttpLastAppHost[0] ? g_sXsHttpLastAppHost : "(none)",
			g_sXsHttpLastAppUserAgent[0] ? g_sXsHttpLastAppUserAgent : "(none)",
			g_sXsHttpLastAppReferer[0] ? g_sXsHttpLastAppReferer : "(none)",
			g_sXsHttpLastAppOrigin[0] ? g_sXsHttpLastAppOrigin : "(none)",
			g_sXsHttpLastAppAccept[0] ? g_sXsHttpLastAppAccept : "(none)",
			g_sXsHttpLastAppAcceptEncoding[0] ? g_sXsHttpLastAppAcceptEncoding : "(none)",
			g_sXsHttpLastAppCookie[0] ? g_sXsHttpLastAppCookie : "(none)",
			g_sXsHttpLastAppForwardedFor[0] ? g_sXsHttpLastAppForwardedFor : "(none)",
			g_sXsHttpLastAppRealIP[0] ? g_sXsHttpLastAppRealIP : "(none)",
			g_sXsHttpLastAppConnection[0] ? g_sXsHttpLastAppConnection : "(none)",
			g_sXsHttpLastAppCacheControl[0] ? g_sXsHttpLastAppCacheControl : "(none)",
			g_sXsHttpLastAppContentType[0] ? g_sXsHttpLastAppContentType : "(none)"
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_idle_close_count=%lld\nhttp_conn_limit_close_count=%lld\nhttp_reject_count=%lld\nhttp_stop_cleanup_count=%lld\nhttp_last_reject_status=%lld\nhttp_last_reject_reason=%s\nhttp_last_reject_time=%s\nhttp_last_reject_age_ms=%lld\nhttp_last_stop_cleanup_closed=%lld\nhttp_last_stop_cleanup_remain=%lld\nhttp_last_stop_cleanup_time=%s\nhttp_last_stop_cleanup_age_ms=%lld\nhttp_last_idle_close_time=%s\nhttp_last_idle_close_age_ms=%lld\nhttp_last_conn_limit_close_time=%s\nhttp_last_conn_limit_close_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpIdleCloseCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpConnLimitCloseCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastRejectStatus),
			g_sXsHttpLastRejectReason[0] ? g_sXsHttpLastRejectReason : "(none)",
			sHttpLastRejectTime ? sHttpLastRejectTime : "(none)",
			(long long)XS_HttpLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastStopCleanupRemain),
			sHttpLastStopCleanupTime ? sHttpLastStopCleanupTime : "(none)",
			(long long)XS_HttpLastStopCleanupAgeMS(),
			sHttpLastIdleCloseTime ? sHttpLastIdleCloseTime : "(none)",
			(long long)XS_HttpLastIdleCloseAgeMS(),
			sHttpLastConnLimitCloseTime ? sHttpLastConnLimitCloseTime : "(none)",
			(long long)XS_HttpLastConnLimitCloseAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_header_limit_reject_count=%lld\nhttp_last_header_limit_reject_time=%s\nhttp_last_header_limit_reject_age_ms=%lld\nhttp_body_limit_reject_count=%lld\nhttp_last_body_limit_reject_time=%s\nhttp_last_body_limit_reject_age_ms=%lld\nhttp_path_limit_reject_count=%lld\nhttp_last_path_limit_reject_time=%s\nhttp_last_path_limit_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpHeaderLimitRejectCount),
			sHttpLastHeaderLimitRejectTime ? sHttpLastHeaderLimitRejectTime : "(none)",
			(long long)XS_HttpLastHeaderLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBodyLimitRejectCount),
			sHttpLastBodyLimitRejectTime ? sHttpLastBodyLimitRejectTime : "(none)",
			(long long)XS_HttpLastBodyLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpPathLimitRejectCount),
			sHttpLastPathLimitRejectTime ? sHttpLastPathLimitRejectTime : "(none)",
			(long long)XS_HttpLastPathLimitRejectAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_api_disabled_reject_count=%lld\nhttp_last_api_disabled_reject_time=%s\nhttp_last_api_disabled_reject_age_ms=%lld\nhttp_method_reject_count=%lld\nhttp_last_method_reject_time=%s\nhttp_last_method_reject_age_ms=%lld\nhttp_host_not_found_reject_count=%lld\nhttp_last_host_not_found_reject_time=%s\nhttp_last_host_not_found_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpApiDisabledRejectCount),
			sHttpLastApiDisabledRejectTime ? sHttpLastApiDisabledRejectTime : "(none)",
			(long long)XS_HttpLastApiDisabledRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodRejectCount),
			sHttpLastMethodRejectTime ? sHttpLastMethodRejectTime : "(none)",
			(long long)XS_HttpLastMethodRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpHostNotFoundRejectCount),
			sHttpLastHostNotFoundRejectTime ? sHttpLastHostNotFoundRejectTime : "(none)",
			(long long)XS_HttpLastHostNotFoundRejectAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_reload_busy_reject_count=%lld\nhttp_last_reload_busy_reject_time=%s\nhttp_last_reload_busy_reject_age_ms=%lld\nhttp_reload_failed_reject_count=%lld\nhttp_last_reload_failed_reject_time=%s\nhttp_last_reload_failed_reject_age_ms=%lld\nhttp_check_config_failed_reject_count=%lld\nhttp_last_check_config_failed_reject_time=%s\nhttp_last_check_config_failed_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpReloadBusyRejectCount),
			sHttpLastReloadBusyRejectTime ? sHttpLastReloadBusyRejectTime : "(none)",
			(long long)XS_HttpLastReloadBusyRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpReloadFailedRejectCount),
			sHttpLastReloadFailedRejectTime ? sHttpLastReloadFailedRejectTime : "(none)",
			(long long)XS_HttpLastReloadFailedRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpCheckConfigFailedRejectCount),
			sHttpLastCheckConfigFailedRejectTime ? sHttpLastCheckConfigFailedRejectTime : "(none)",
			(long long)XS_HttpLastCheckConfigFailedRejectAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_bus_bad_request_reject_count=%lld\nhttp_last_bus_bad_request_reject_time=%s\nhttp_last_bus_bad_request_reject_age_ms=%lld\nhttp_bus_not_found_reject_count=%lld\nhttp_last_bus_not_found_reject_time=%s\nhttp_last_bus_not_found_reject_age_ms=%lld\nhttp_bus_limit_reject_count=%lld\nhttp_last_bus_limit_reject_time=%s\nhttp_last_bus_limit_reject_age_ms=%lld\nhttp_bus_failed_reject_count=%lld\nhttp_last_bus_failed_reject_time=%s\nhttp_last_bus_failed_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpBusBadRequestRejectCount),
			sHttpLastBusBadRequestRejectTime ? sHttpLastBusBadRequestRejectTime : "(none)",
			(long long)XS_HttpLastBusBadRequestRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusNotFoundRejectCount),
			sHttpLastBusNotFoundRejectTime ? sHttpLastBusNotFoundRejectTime : "(none)",
			(long long)XS_HttpLastBusNotFoundRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusLimitRejectCount),
			sHttpLastBusLimitRejectTime ? sHttpLastBusLimitRejectTime : "(none)",
			(long long)XS_HttpLastBusLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusFailedRejectCount),
			sHttpLastBusFailedRejectTime ? sHttpLastBusFailedRejectTime : "(none)",
			(long long)XS_HttpLastBusFailedRejectAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"bus_namespace_count=%lld\nbus_namespace_item_count=%lld\nbus_data_limit=%lld\nbus_queue_limit=%lld\nbus_namespace_limit=%lld\nbus_namespace_data_limit=%lld\nbus_namespace_limit_remaining=%lld\nbus_namespace_limit_reached=%s\nbus_readonly_namespace_count=%lld\nbus_disabled_namespace_count=%lld\nbus_ttl_required_namespace_count=%lld\nbus_tag_required_namespace_count=%lld\nbus_readonly_namespaces=%s\nbus_disabled_namespaces=%s\nbus_ttl_required_namespaces=%s\nbus_tag_required_namespaces=%s\nbus_data_limit_reject_count=%lld\nbus_last_data_limit_reject_time=%s\nbus_last_data_limit_reject_age_ms=%lld\nbus_last_data_limit_count=%lld\nbus_queue_limit_reject_count=%lld\nbus_last_queue_limit_reject_time=%s\nbus_last_queue_limit_reject_age_ms=%lld\nbus_last_queue_limit_count=%lld\n",
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_count", sizeof("namespace_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_item_count", sizeof("namespace_item_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "data_limit", sizeof("data_limit") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "queue_limit", sizeof("queue_limit") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_limit", sizeof("namespace_limit") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_data_limit", sizeof("namespace_data_limit") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_limit_remaining", sizeof("namespace_limit_remaining") - 1) : 0),
			(objBus && xvoGetBool(xvoTableGetValue(objBus, "namespace_limit_reached", sizeof("namespace_limit_reached") - 1))) ? "true" : "false",
			(long long)(objBus ? xvoTableGetInt(objBus, "readonly_namespace_count", sizeof("readonly_namespace_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "disabled_namespace_count", sizeof("disabled_namespace_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "ttl_required_namespace_count", sizeof("ttl_required_namespace_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "tag_required_namespace_count", sizeof("tag_required_namespace_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "readonly_namespaces", sizeof("readonly_namespaces") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "disabled_namespaces", sizeof("disabled_namespaces") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "ttl_required_namespaces", sizeof("ttl_required_namespaces") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "tag_required_namespaces", sizeof("tag_required_namespaces") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "data_limit_reject_count", sizeof("data_limit_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_data_limit_reject_time", sizeof("last_data_limit_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_data_limit_reject_age_ms", sizeof("last_data_limit_reject_age_ms") - 1) : -1),
			(long long)(objBus ? xvoTableGetInt(objBus, "last_data_limit_count", sizeof("last_data_limit_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "queue_limit_reject_count", sizeof("queue_limit_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_queue_limit_reject_time", sizeof("last_queue_limit_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_queue_limit_reject_age_ms", sizeof("last_queue_limit_reject_age_ms") - 1) : -1),
			(long long)(objBus ? xvoTableGetInt(objBus, "last_queue_limit_count", sizeof("last_queue_limit_count") - 1) : 0)
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"bus_namespace_limit_reject_count=%lld\nbus_last_namespace_limit_reject_time=%s\nbus_last_namespace_limit_reject_age_ms=%lld\nbus_last_namespace_limit_count=%lld\nbus_last_namespace_limit_namespace=%s\nbus_namespace_data_limit_reject_count=%lld\nbus_last_namespace_data_limit_reject_time=%s\nbus_last_namespace_data_limit_reject_age_ms=%lld\nbus_last_namespace_data_limit_count=%lld\nbus_last_namespace_data_limit_namespace=%s\nbus_readonly_namespace_reject_count=%lld\nbus_last_readonly_namespace_reject_time=%s\nbus_last_readonly_namespace_reject_age_ms=%lld\nbus_last_readonly_namespace=%s\nbus_last_readonly_namespace_action=%s\nbus_disabled_namespace_reject_count=%lld\nbus_last_disabled_namespace_reject_time=%s\nbus_last_disabled_namespace_reject_age_ms=%lld\nbus_last_disabled_namespace=%s\nbus_last_disabled_namespace_action=%s\nbus_ttl_required_namespace_reject_count=%lld\nbus_last_ttl_required_namespace_reject_time=%s\nbus_last_ttl_required_namespace_reject_age_ms=%lld\nbus_last_ttl_required_namespace=%s\nbus_last_ttl_required_namespace_action=%s\nbus_tag_required_namespace_reject_count=%lld\nbus_last_tag_required_namespace_reject_time=%s\nbus_last_tag_required_namespace_reject_age_ms=%lld\nbus_last_tag_required_namespace=%s\nbus_last_tag_required_namespace_action=%s\n",
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_limit_reject_count", sizeof("namespace_limit_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_namespace_limit_reject_time", sizeof("last_namespace_limit_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_namespace_limit_reject_age_ms", sizeof("last_namespace_limit_reject_age_ms") - 1) : -1),
			(long long)(objBus ? xvoTableGetInt(objBus, "last_namespace_limit_count", sizeof("last_namespace_limit_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_namespace_limit_namespace", sizeof("last_namespace_limit_namespace") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_data_limit_reject_count", sizeof("namespace_data_limit_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_namespace_data_limit_reject_time", sizeof("last_namespace_data_limit_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_namespace_data_limit_reject_age_ms", sizeof("last_namespace_data_limit_reject_age_ms") - 1) : -1),
			(long long)(objBus ? xvoTableGetInt(objBus, "last_namespace_data_limit_count", sizeof("last_namespace_data_limit_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_namespace_data_limit_namespace", sizeof("last_namespace_data_limit_namespace") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "readonly_namespace_reject_count", sizeof("readonly_namespace_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_readonly_namespace_reject_time", sizeof("last_readonly_namespace_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_readonly_namespace_reject_age_ms", sizeof("last_readonly_namespace_reject_age_ms") - 1) : -1),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_readonly_namespace", sizeof("last_readonly_namespace") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_readonly_namespace_action", sizeof("last_readonly_namespace_action") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "disabled_namespace_reject_count", sizeof("disabled_namespace_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_disabled_namespace_reject_time", sizeof("last_disabled_namespace_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_disabled_namespace_reject_age_ms", sizeof("last_disabled_namespace_reject_age_ms") - 1) : -1),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_disabled_namespace", sizeof("last_disabled_namespace") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_disabled_namespace_action", sizeof("last_disabled_namespace_action") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "ttl_required_namespace_reject_count", sizeof("ttl_required_namespace_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_ttl_required_namespace_reject_time", sizeof("last_ttl_required_namespace_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_ttl_required_namespace_reject_age_ms", sizeof("last_ttl_required_namespace_reject_age_ms") - 1) : -1),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_ttl_required_namespace", sizeof("last_ttl_required_namespace") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_ttl_required_namespace_action", sizeof("last_ttl_required_namespace_action") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "tag_required_namespace_reject_count", sizeof("tag_required_namespace_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_tag_required_namespace_reject_time", sizeof("last_tag_required_namespace_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_tag_required_namespace_reject_age_ms", sizeof("last_tag_required_namespace_reject_age_ms") - 1) : -1),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_tag_required_namespace", sizeof("last_tag_required_namespace") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_tag_required_namespace_action", sizeof("last_tag_required_namespace_action") - 1)) : (str)""
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"ws_reject_count=%lld\nws_last_reject_reason=%s\nws_last_reject_time=%s\nws_last_reject_age_ms=%lld\nws_stop_cleanup_count=%lld\nws_last_stop_cleanup_closed=%lld\nws_last_stop_cleanup_remain=%lld\nws_last_stop_cleanup_time=%s\nws_last_stop_cleanup_age_ms=%lld\nxtp_reject_count=%lld\nxtp_last_reject_reason=%s\nxtp_last_reject_time=%s\nxtp_last_reject_age_ms=%lld\nxtp_stop_cleanup_count=%lld\nxtp_last_stop_cleanup_closed=%lld\nxtp_last_stop_cleanup_remain=%lld\nxtp_last_stop_cleanup_time=%s\nxtp_last_stop_cleanup_age_ms=%lld\ncustom_reject_count=%lld\ncustom_last_reject_reason=%s\ncustom_last_reject_time=%s\ncustom_last_reject_age_ms=%lld\ncustom_stop_cleanup_count=%lld\ncustom_last_stop_cleanup_closed=%lld\ncustom_last_stop_cleanup_remain=%lld\ncustom_last_stop_cleanup_time=%s\ncustom_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsWsRejectCount),
			g_sXsWsLastRejectReason[0] ? g_sXsWsLastRejectReason : "(none)",
			sWsLastRejectTime ? sWsLastRejectTime : "(none)",
			(long long)XS_WsLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsWsStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsWsLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsWsLastStopCleanupRemain),
			sWsLastStopCleanupTime ? sWsLastStopCleanupTime : "(none)",
			(long long)XS_WsLastStopCleanupAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpRejectCount),
			g_sXsXtpLastRejectReason[0] ? g_sXsXtpLastRejectReason : "(none)",
			sXtpLastRejectTime ? sXtpLastRejectTime : "(none)",
			(long long)XS_XtpLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastStopCleanupRemain),
			sXtpLastStopCleanupTime ? sXtpLastStopCleanupTime : "(none)",
			(long long)XS_XtpLastStopCleanupAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomRejectCount),
			g_sXsCustomLastRejectReason[0] ? g_sXsCustomLastRejectReason : "(none)",
			sCustomLastRejectTime ? sCustomLastRejectTime : "(none)",
			(long long)XS_CustomLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsCustomLastStopCleanupRemain),
			sCustomLastStopCleanupTime ? sCustomLastStopCleanupTime : "(none)",
			(long long)XS_CustomLastStopCleanupAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"ws_message_limit_close_count=%lld\nws_last_message_limit_close_time=%s\nws_last_message_limit_close_age_ms=%lld\nxtp_recv_limit_close_count=%lld\nxtp_last_recv_limit_close_time=%s\nxtp_last_recv_limit_close_age_ms=%lld\ncustom_recv_limit_close_count=%lld\ncustom_last_recv_limit_close_time=%s\ncustom_last_recv_limit_close_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsWsMessageLimitCloseCount),
			sWsLastMessageLimitCloseTime ? sWsLastMessageLimitCloseTime : "(none)",
			(long long)XS_WsLastMessageLimitCloseAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpRecvLimitCloseCount),
			sXtpLastRecvLimitCloseTime ? sXtpLastRecvLimitCloseTime : "(none)",
			(long long)XS_XtpLastRecvLimitCloseAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomRecvLimitCloseCount),
			sCustomLastRecvLimitCloseTime ? sCustomLastRecvLimitCloseTime : "(none)",
			(long long)XS_CustomLastRecvLimitCloseAgeMS()
		);
	}
	if ( objBus ) {
		xvoUnref(objBus);
	}
	if ( sCheckTime ) {
		xrtFree(sCheckTime);
	}
	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
	}
	if ( sHttpLastIdleCloseTime ) {
		xrtFree(sHttpLastIdleCloseTime);
	}
	if ( sHttpLastConnLimitCloseTime ) {
		xrtFree(sHttpLastConnLimitCloseTime);
	}
	if ( sHttpLastRejectTime ) {
		xrtFree(sHttpLastRejectTime);
	}
	if ( sHttpLastStopCleanupTime ) {
		xrtFree(sHttpLastStopCleanupTime);
	}
	if ( sHttpLastHeaderLimitRejectTime ) {
		xrtFree(sHttpLastHeaderLimitRejectTime);
	}
	if ( sHttpLastBodyLimitRejectTime ) {
		xrtFree(sHttpLastBodyLimitRejectTime);
	}
	if ( sHttpLastPathLimitRejectTime ) {
		xrtFree(sHttpLastPathLimitRejectTime);
	}
	if ( sHttpLastApiDisabledRejectTime ) {
		xrtFree(sHttpLastApiDisabledRejectTime);
	}
	if ( sHttpLastMethodRejectTime ) {
		xrtFree(sHttpLastMethodRejectTime);
	}
	if ( sHttpLastHostNotFoundRejectTime ) {
		xrtFree(sHttpLastHostNotFoundRejectTime);
	}
	if ( sHttpLastReloadBusyRejectTime ) {
		xrtFree(sHttpLastReloadBusyRejectTime);
	}
	if ( sHttpLastReloadFailedRejectTime ) {
		xrtFree(sHttpLastReloadFailedRejectTime);
	}
	if ( sHttpLastCheckConfigFailedRejectTime ) {
		xrtFree(sHttpLastCheckConfigFailedRejectTime);
	}
	if ( sHttpLastBusBadRequestRejectTime ) {
		xrtFree(sHttpLastBusBadRequestRejectTime);
	}
	if ( sHttpLastBusNotFoundRejectTime ) {
		xrtFree(sHttpLastBusNotFoundRejectTime);
	}
	if ( sHttpLastBusLimitRejectTime ) {
		xrtFree(sHttpLastBusLimitRejectTime);
	}
	if ( sHttpLastBusFailedRejectTime ) {
		xrtFree(sHttpLastBusFailedRejectTime);
	}
	if ( sWsLastMessageLimitCloseTime ) {
		xrtFree(sWsLastMessageLimitCloseTime);
	}
	if ( sWsLastRejectTime ) {
		xrtFree(sWsLastRejectTime);
	}
	if ( sWsLastStopCleanupTime ) {
		xrtFree(sWsLastStopCleanupTime);
	}
	if ( sXtpLastRecvLimitCloseTime ) {
		xrtFree(sXtpLastRecvLimitCloseTime);
	}
	if ( sXtpLastRejectTime ) {
		xrtFree(sXtpLastRejectTime);
	}
	if ( sXtpLastStopCleanupTime ) {
		xrtFree(sXtpLastStopCleanupTime);
	}
	if ( sCustomLastRecvLimitCloseTime ) {
		xrtFree(sCustomLastRecvLimitCloseTime);
	}
	if ( sCustomLastRejectTime ) {
		xrtFree(sCustomLastRejectTime);
	}
	if ( sCustomLastStopCleanupTime ) {
		xrtFree(sCustomLastStopCleanupTime);
	}
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	if ( sBusQueueTime ) {
		xrtFree(sBusQueueTime);
	}
	if ( sBusDispatchTime ) {
		xrtFree(sBusDispatchTime);
	}
	if ( sBusLastSweepTime ) {
		xrtFree(sBusLastSweepTime);
	}
	if ( sBusLastCleanupTime ) {
		xrtFree(sBusLastCleanupTime);
	}
	return XS_HttpRespondText(pResp, bOk ? 200 : 503, bOk ? "OK" : "Service Unavailable", sBody);
}

static inline bool XS_HttpHandleDashboard(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_ReloadStatusSnapshot tReloadStatus;
	XS_CheckConfigStatusSnapshot tCheckStatus;
	char sBody[32768];
	char* sConfigBase;
	char* sCheckTime;
	char* sReloadTime;
	char* sStartTime;
	char* sCurrentDir;
	char* sConfigName;
	char* sAppMTime;
	char* sConfigMTime;
	char* sBusQueueTime;
	char* sBusDispatchTime;
	char* sBusLastSweepTime;
	char* sBusLastCleanupTime;
	char* sBusItems;
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sHttpLastIdleCloseTime;
	char* sHttpLastConnLimitCloseTime;
	char* sHttpLastRejectTime;
	char* sHttpLastStopCleanupTime;
	char* sHttpLastHeaderLimitRejectTime;
	char* sHttpLastBodyLimitRejectTime;
	char* sHttpLastPathLimitRejectTime;
	char* sHttpLastApiDisabledRejectTime;
	char* sHttpLastMethodRejectTime;
	char* sHttpLastHostNotFoundRejectTime;
	char* sHttpLastReloadBusyRejectTime;
	char* sHttpLastReloadFailedRejectTime;
	char* sHttpLastCheckConfigFailedRejectTime;
	char* sHttpLastBusBadRequestRejectTime;
	char* sHttpLastBusNotFoundRejectTime;
	char* sHttpLastBusLimitRejectTime;
	char* sHttpLastBusFailedRejectTime;
	char* sWsLastTime;
	char* sWsLastCloseTime;
	char* sWsLastErrorTime;
	char* sWsLastInvalidTime;
	char* sWsLastIdleCloseTime;
	char* sWsLastConnLimitCloseTime;
	char* sWsLastMessageLimitCloseTime;
	char* sWsLastRejectTime;
	char* sWsLastStopCleanupTime;
	char* sXtpLastTime;
	char* sXtpLastInvalidTime;
	char* sXtpLastErrorTime;
	char* sXtpLastIdleCloseTime;
	char* sXtpLastConnLimitCloseTime;
	char* sXtpLastRecvLimitCloseTime;
	char* sXtpLastRejectTime;
	char* sXtpLastStopCleanupTime;
	char* sUdpLastTime;
	char* sUdpLastErrorTime;
	char* sCustomLastTime;
	char* sCustomLastCloseTime;
	char* sCustomLastErrorTime;
	char* sCustomLastInvalidTime;
	char* sCustomLastIdleCloseTime;
	char* sCustomLastConnLimitCloseTime;
	char* sCustomLastRecvLimitCloseTime;
	char* sCustomLastRejectTime;
	char* sCustomLastStopCleanupTime;
	int64 iAppSize;
	int64 iConfigSize;
	xvalue objBus;
	int64 iBusQueueCount;
	int64 iBusDataCount;
	int64 iBusTotalQueued;
	int64 iBusTotalDelivered;
	int64 iBusTotalDropped;
	int64 tBusLastQueue;
	int64 tBusLastDispatch;
	int64 iBusSweepIntervalMS;
	int64 iBusSweepBatchLimit;
	int64 iBusSweepCount;
	int64 iBusSweepRemovedCount;
	int64 tBusLastSweep;
	int64 iBusLastSweepRemoved;
	int64 iBusLastSweepRemain;
	int64 iBusCleanupCount;
	int64 tBusLastCleanup;
	int64 iBusLastCleanupRemoved;
	int64 iBusLastCleanupRemain;
	bool bOk;
	bool bScriptLoaded;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/dashboard") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "dashboard api disabled");
	}
	sConfigBase = (g_sXsConfigFile && g_sXsConfigFile[0]) ? xrtPathGetDir(g_sXsConfigFile, 0) : NULL;
	sCurrentDir = XS_CurrentWorkDir();
	sConfigName = XS_ConfigFileName();
	sAppMTime = XS_FileChangeTimeText(xCore.AppFile ? (char*)xCore.AppFile : NULL);
	sConfigMTime = XS_FileChangeTimeText(g_sXsConfigFile);
	iAppSize = XS_FileSizeValue(xCore.AppFile ? (char*)xCore.AppFile : NULL);
	iConfigSize = XS_FileSizeValue(g_sXsConfigFile);
	XS_ConfigReloadStatusSnapshot(&tReloadStatus);
	XS_ConfigCheckStatusSnapshot(&tCheckStatus);
	sCheckTime = XS_CheckConfigTimeTextByStatus(&tCheckStatus);
	sReloadTime = XS_ReloadTimeTextByStatus(&tReloadStatus);
	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sHttpLastIdleCloseTime = XS_HttpLastIdleCloseTimeText();
	sHttpLastConnLimitCloseTime = XS_HttpLastConnLimitCloseTimeText();
	sHttpLastRejectTime = XS_HttpLastRejectTimeText();
	sHttpLastStopCleanupTime = XS_HttpLastStopCleanupTimeText();
	sHttpLastHeaderLimitRejectTime = XS_HttpLastHeaderLimitRejectTimeText();
	sHttpLastBodyLimitRejectTime = XS_HttpLastBodyLimitRejectTimeText();
	sHttpLastPathLimitRejectTime = XS_HttpLastPathLimitRejectTimeText();
	sHttpLastApiDisabledRejectTime = XS_HttpLastApiDisabledRejectTimeText();
	sHttpLastMethodRejectTime = XS_HttpLastMethodRejectTimeText();
	sHttpLastHostNotFoundRejectTime = XS_HttpLastHostNotFoundRejectTimeText();
	sHttpLastReloadBusyRejectTime = XS_HttpLastReloadBusyRejectTimeText();
	sHttpLastReloadFailedRejectTime = XS_HttpLastReloadFailedRejectTimeText();
	sHttpLastCheckConfigFailedRejectTime = XS_HttpLastCheckConfigFailedRejectTimeText();
	sHttpLastBusBadRequestRejectTime = XS_HttpLastBusBadRequestRejectTimeText();
	sHttpLastBusNotFoundRejectTime = XS_HttpLastBusNotFoundRejectTimeText();
	sHttpLastBusLimitRejectTime = XS_HttpLastBusLimitRejectTimeText();
	sHttpLastBusFailedRejectTime = XS_HttpLastBusFailedRejectTimeText();
	sWsLastTime = XS_WsLastTimeText();
	sWsLastCloseTime = XS_WsLastCloseTimeText();
	sWsLastErrorTime = XS_WsLastErrorTimeText();
	sWsLastInvalidTime = XS_WsLastInvalidTimeText();
	sWsLastIdleCloseTime = XS_WsLastIdleCloseTimeText();
	sWsLastConnLimitCloseTime = XS_WsLastConnLimitCloseTimeText();
	sWsLastMessageLimitCloseTime = XS_WsLastMessageLimitCloseTimeText();
	sWsLastRejectTime = XS_WsLastRejectTimeText();
	sWsLastStopCleanupTime = XS_WsLastStopCleanupTimeText();
	sXtpLastTime = XS_XtpLastTimeText();
	sXtpLastInvalidTime = XS_XtpLastInvalidTimeText();
	sXtpLastErrorTime = XS_XtpLastErrorTimeText();
	sXtpLastIdleCloseTime = XS_XtpLastIdleCloseTimeText();
	sXtpLastConnLimitCloseTime = XS_XtpLastConnLimitCloseTimeText();
	sXtpLastRecvLimitCloseTime = XS_XtpLastRecvLimitCloseTimeText();
	sXtpLastRejectTime = XS_XtpLastRejectTimeText();
	sXtpLastStopCleanupTime = XS_XtpLastStopCleanupTimeText();
	sUdpLastTime = XS_UdpLastTimeText();
	sUdpLastErrorTime = XS_UdpLastErrorTimeText();
	sCustomLastTime = XS_CustomLastTimeText();
	sCustomLastCloseTime = XS_CustomLastCloseTimeText();
	sCustomLastErrorTime = XS_CustomLastErrorTimeText();
	sCustomLastInvalidTime = XS_CustomLastInvalidTimeText();
	sCustomLastIdleCloseTime = XS_CustomLastIdleCloseTimeText();
	sCustomLastConnLimitCloseTime = XS_CustomLastConnLimitCloseTimeText();
	sCustomLastRecvLimitCloseTime = XS_CustomLastRecvLimitCloseTimeText();
	sCustomLastRejectTime = XS_CustomLastRejectTimeText();
	sCustomLastStopCleanupTime = XS_CustomLastStopCleanupTimeText();
	sBusQueueTime = NULL;
	sBusDispatchTime = NULL;
	sBusLastSweepTime = NULL;
	sBusLastCleanupTime = NULL;
	sBusItems = NULL;
	sStartTime = g_tXsStartTime ? xrtTimeToStr(g_tXsStartTime, XRT_TIME_FORMAT_DATETIME) : NULL;

	objBus = XS_BusBuildNamespaceStatsValue();
	iBusQueueCount = 0;
	iBusDataCount = 0;
	iBusTotalQueued = 0;
	iBusTotalDelivered = 0;
	iBusTotalDropped = 0;
	tBusLastQueue = 0;
	tBusLastDispatch = 0;
	iBusSweepIntervalMS = 0;
	iBusSweepBatchLimit = 0;
	iBusSweepCount = 0;
	iBusSweepRemovedCount = 0;
	tBusLastSweep = 0;
	iBusLastSweepRemoved = 0;
	iBusLastSweepRemain = 0;
	iBusCleanupCount = 0;
	tBusLastCleanup = 0;
	iBusLastCleanupRemoved = 0;
	iBusLastCleanupRemain = 0;
	if ( objBus ) {
		iBusQueueCount = xvoTableGetInt(objBus, "queue_count", 11);
		iBusDataCount = xvoTableGetInt(objBus, "data_count", 10);
		iBusTotalQueued = xvoTableGetInt(objBus, "total_queued", 12);
		iBusTotalDelivered = xvoTableGetInt(objBus, "total_delivered", 15);
		iBusTotalDropped = xvoTableGetInt(objBus, "total_dropped", 13);
		tBusLastQueue = xvoTableGetInt(objBus, "last_queue_time", 15);
		tBusLastDispatch = xvoTableGetInt(objBus, "last_dispatch_time", 18);
		iBusSweepIntervalMS = xvoTableGetInt(objBus, "sweep_interval_ms", sizeof("sweep_interval_ms") - 1);
		iBusSweepBatchLimit = xvoTableGetInt(objBus, "sweep_batch_limit", sizeof("sweep_batch_limit") - 1);
		iBusSweepCount = xvoTableGetInt(objBus, "sweep_count", sizeof("sweep_count") - 1);
		iBusSweepRemovedCount = xvoTableGetInt(objBus, "sweep_removed_count", sizeof("sweep_removed_count") - 1);
		tBusLastSweep = xvoTableGetInt(objBus, "last_sweep_time", sizeof("last_sweep_time") - 1);
		iBusLastSweepRemoved = xvoTableGetInt(objBus, "last_sweep_removed", sizeof("last_sweep_removed") - 1);
		iBusLastSweepRemain = xvoTableGetInt(objBus, "last_sweep_remain", sizeof("last_sweep_remain") - 1);
		iBusCleanupCount = xvoTableGetInt(objBus, "cleanup_count", sizeof("cleanup_count") - 1);
		tBusLastCleanup = xvoTableGetInt(objBus, "last_cleanup_time", sizeof("last_cleanup_time") - 1);
		iBusLastCleanupRemoved = xvoTableGetInt(objBus, "last_cleanup_removed", sizeof("last_cleanup_removed") - 1);
		iBusLastCleanupRemain = xvoTableGetInt(objBus, "last_cleanup_remain", sizeof("last_cleanup_remain") - 1);
		sBusQueueTime = (tBusLastQueue > 0) ? xrtTimeToStr(tBusLastQueue, XRT_TIME_FORMAT_DATETIME) : NULL;
		sBusDispatchTime = (tBusLastDispatch > 0) ? xrtTimeToStr(tBusLastDispatch, XRT_TIME_FORMAT_DATETIME) : NULL;
		sBusLastSweepTime = (tBusLastSweep > 0) ? xrtTimeToStr(tBusLastSweep, XRT_TIME_FORMAT_DATETIME) : NULL;
		sBusLastCleanupTime = (tBusLastCleanup > 0) ? xrtTimeToStr(tBusLastCleanup, XRT_TIME_FORMAT_DATETIME) : NULL;
		sBusItems = xrtStringifyJSON(xvoTableGetValue(objBus, "items", 5), FALSE, NULL);
	}

	bScriptLoaded = FALSE;
	if ( objServer->EnableDefaultHost && objServer->DefaultHost.pScriptState ) {
		bScriptLoaded = TRUE;
	}
	if ( !bScriptLoaded && objHost->pScriptState ) {
		bScriptLoaded = TRUE;
	}
	bOk = XS_HttpServerHealthy(objServer, objHost);

	snprintf(
		sBody,
		sizeof(sBody),
		"ok=%s\nserver=%s\nclass=%s\naddr=%s\nbind_ip=%s\nbind_port=%u\ntls=%s\nbind_ip_tls=%s\nbind_port_tls=%u\naddr_tls=%s\nidle_timeout=%u\nconn_limit=%u\nws_protocol=%s\nws_message_limit=%u\nws_conn_current=%lld\nws_conn_peak=%lld\nws_open_count=%lld\nws_close_count=%lld\nws_text_count=%lld\nws_binary_count=%lld\nws_ping_count=%lld\nws_pong_count=%lld\nws_error_count=%lld\nws_invalid_count=%lld\nws_idle_close_count=%lld\nws_conn_limit_close_count=%lld\nws_last_error_code=%lld\nws_last_invalid_reason=%s\nws_last_invalid_time=%s\nws_last_invalid_age_ms=%lld\nws_last_close_reason=%lld\nws_last_close_time=%s\nws_last_close_age_ms=%lld\nws_last_idle_close_time=%s\nws_last_idle_close_age_ms=%lld\nws_last_conn_limit_close_time=%s\nws_last_conn_limit_close_age_ms=%lld\nws_last_frame_type=%s\nws_last_remote=%s\nws_last_bytes=%lld\nws_last_text=%s\nws_last_time=%s\nws_last_age_ms=%lld\nws_last_error_time=%s\nws_last_error_age_ms=%lld\nxtp_conn_current=%lld\nxtp_conn_peak=%lld\nxtp_open_count=%lld\nxtp_close_count=%lld\nxtp_error_count=%lld\nxtp_invalid_count=%lld\nxtp_msg_count=%lld\nxtp_req_count=%lld\nxtp_resp_count=%lld\nxtp_push_count=%lld\nxtp_event_count=%lld\nxtp_send_count=%lld\nxtp_recv_bytes=%lld\nxtp_send_bytes=%lld\nxtp_last_msg_type=%s\nxtp_last_status=%lld\nxtp_last_msg_id=%lld\nxtp_last_flags=%lld\nxtp_last_param_count=%lld\nxtp_last_body_size=%lld\nxtp_last_remote=%s\nxtp_last_bytes=%lld\nxtp_last_cmd=%s\nxtp_last_time=%s\nxtp_last_age_ms=%lld\nxtp_last_invalid_reason=%s\nxtp_last_invalid_time=%s\nxtp_last_invalid_age_ms=%lld\nxtp_last_error_code=%lld\nxtp_last_error_time=%s\nxtp_last_error_age_ms=%lld\nxtp_idle_close_count=%lld\nxtp_conn_limit_close_count=%lld\nxtp_last_idle_close_time=%s\nxtp_last_idle_close_age_ms=%lld\nxtp_last_conn_limit_close_time=%s\nxtp_last_conn_limit_close_age_ms=%lld\nudp_recv_count=%lld\nudp_send_count=%lld\nudp_error_count=%lld\nudp_last_error_code=%lld\nudp_recv_bytes=%lld\nudp_send_bytes=%lld\nudp_last_from=%s\nudp_last_text=%s\nudp_last_bytes=%lld\nudp_last_time=%s\nudp_last_age_ms=%lld\nudp_last_error_time=%s\nudp_last_error_age_ms=%lld\ncustom_conn_current=%lld\ncustom_conn_peak=%lld\ncustom_open_count=%lld\ncustom_close_count=%lld\ncustom_error_count=%lld\ncustom_invalid_count=%lld\ncustom_last_invalid_reason=%s\ncustom_last_invalid_time=%s\ncustom_last_invalid_age_ms=%lld\ncustom_last_close_reason=%lld\ncustom_last_close_time=%s\ncustom_last_close_age_ms=%lld\ncustom_last_error_code=%lld\ncustom_last_error_time=%s\ncustom_last_error_age_ms=%lld\ncustom_recv_count=%lld\ncustom_send_count=%lld\ncustom_recv_bytes=%lld\ncustom_send_bytes=%lld\ncustom_last_remote=%s\ncustom_last_bytes=%lld\ncustom_last_text=%s\ncustom_last_time=%s\ncustom_last_age_ms=%lld\ncustom_idle_close_count=%lld\ncustom_conn_limit_close_count=%lld\ncustom_last_idle_close_time=%s\ncustom_last_idle_close_age_ms=%lld\ncustom_last_conn_limit_close_time=%s\ncustom_last_conn_limit_close_age_ms=%lld\ntls_cert_file=%s\ntls_key_file=%s\ntls_ca_file=%s\ncurrent_dir=%s\napp_file=%s\napp_mtime=%s\napp_size=%lld\napp_path=%s\nbuild=%s\ncompiler=%s\nplatform=%s\narch=%s\nmem_debug=%s\npid=%llu\nstart_time=%s\nuptime_ms=%lld\nengine_workers=%u\nruntime_server_count=%u\nhttp_req_count=%lld\nhttp_manage_req_count=%lld\nhttp_app_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_last_method=%s\nhttp_last_status=%lld\nhttp_last_path=%s\nhttp_last_target=%s\nhttp_last_remote=%s\nhttp_last_body_len=%lld\nhttp_last_header_count=%lld\nhttp_last_query_len=%lld\nhttp_last_time=%s\nhttp_last_age_ms=%lld\nhttp_last_duration_ms=%lld\nhttp_idle_close_count=%lld\nhttp_conn_limit_close_count=%lld\nhttp_last_idle_close_time=%s\nhttp_last_idle_close_age_ms=%lld\nhttp_last_conn_limit_close_time=%s\nhttp_last_conn_limit_close_age_ms=%lld\nhttp_last_app_method=%s\nhttp_last_app_status=%lld\nhttp_last_app_path=%s\nhttp_last_app_target=%s\nhttp_last_app_remote=%s\nhttp_last_app_body_len=%lld\nhttp_last_app_header_count=%lld\nhttp_last_app_query_len=%lld\nhttp_last_app_time=%s\nhttp_last_app_age_ms=%lld\nhttp_last_app_duration_ms=%lld\nmanage_api=%s\ndebug=%s\nconfig_file=%s\nconfig_name=%s\nconfig_mtime=%s\nconfig_size=%lld\nconfig_base=%s\ncheck_result=%s\ncheck_file=%s\ncheck_base=%s\ncheck_server_count=%lld\ncheck_message=%s\ncheck_last_time=%s\ncheck_last_age_ms=%lld\nhost_aware=%s\ndefault_host=%s\nhost_count=%u\nscript_loaded=%s\nreload_busy=%s\nreload_has_result=%s\nreload_success=%s\nreload_server=%s\nreload_host=%s\nreload_message=%s\nbus_queue_count=%lld\nbus_data_count=%lld\nbus_total_queued=%lld\nbus_total_delivered=%lld\nbus_total_dropped=%lld\nbus_last_queue_time=%lld\nbus_last_queue_time_text=%s\nbus_last_dispatch_time=%lld\nbus_last_dispatch_time_text=%s\nbus_sweep_interval_ms=%lld\nbus_sweep_batch_limit=%lld\nbus_sweep_count=%lld\nbus_sweep_removed_count=%lld\nbus_last_sweep_time=%lld\nbus_last_sweep_time_text=%s\nbus_last_sweep_age_ms=%lld\nbus_last_sweep_removed=%lld\nbus_last_sweep_remain=%lld\nbus_cleanup_count=%lld\nbus_last_cleanup_time=%lld\nbus_last_cleanup_time_text=%s\nbus_last_cleanup_age_ms=%lld\nbus_last_cleanup_removed=%lld\nbus_last_cleanup_remain=%lld\n",
		bOk ? "true" : "false",
		objServer->Name ? objServer->Name : "(null)",
		XS_ServerClassName(objServer->Class),
		objServer->Addr ? objServer->Addr : "(null)",
		objServer->BindIP ? objServer->BindIP : "(null)",
		(unsigned int)objServer->BindPort,
		objServer->EnableTLS ? "true" : "false",
		objServer->BindIPTLS ? objServer->BindIPTLS : "(null)",
		(unsigned int)objServer->BindPortTLS,
		objServer->AddrTLS ? objServer->AddrTLS : "(null)",
		(unsigned int)objServer->IdleTimeout,
		(unsigned int)objServer->ConnLimit,
		objServer->WsProtocol ? objServer->WsProtocol : "(null)",
		(unsigned int)objServer->WsMessageLimit,
		(long long)XS_HttpMetricGet(&g_iXsWsConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsWsConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsWsOpenCount),
		(long long)XS_HttpMetricGet(&g_iXsWsCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsWsTextCount),
		(long long)XS_HttpMetricGet(&g_iXsWsBinaryCount),
		(long long)XS_HttpMetricGet(&g_iXsWsPingCount),
		(long long)XS_HttpMetricGet(&g_iXsWsPongCount),
		(long long)XS_HttpMetricGet(&g_iXsWsErrorCount),
		(long long)XS_HttpMetricGet(&g_iXsWsInvalidCount),
		(long long)XS_HttpMetricGet(&g_iXsWsIdleCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsWsConnLimitCloseCount),
		(long long)g_iXsWsLastErrorCode,
		g_sXsWsLastInvalidReason[0] ? g_sXsWsLastInvalidReason : "(none)",
		sWsLastInvalidTime ? sWsLastInvalidTime : "(none)",
		(long long)XS_WsLastInvalidAgeMS(),
		(long long)g_iXsWsLastCloseReason,
		sWsLastCloseTime ? sWsLastCloseTime : "(none)",
		(long long)XS_WsLastCloseAgeMS(),
		sWsLastIdleCloseTime ? sWsLastIdleCloseTime : "(none)",
		(long long)XS_WsLastIdleCloseAgeMS(),
		sWsLastConnLimitCloseTime ? sWsLastConnLimitCloseTime : "(none)",
		(long long)XS_WsLastConnLimitCloseAgeMS(),
		XS_WsLastFrameTypeName()[0] ? XS_WsLastFrameTypeName() : "(none)",
		g_sXsWsLastRemote[0] ? g_sXsWsLastRemote : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsWsLastBytes),
		g_sXsWsLastText[0] ? g_sXsWsLastText : "(none)",
		sWsLastTime ? sWsLastTime : "(none)",
		(long long)XS_WsLastAgeMS(),
		sWsLastErrorTime ? sWsLastErrorTime : "(none)",
		(long long)XS_WsLastErrorAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsXtpConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsXtpConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsXtpOpenCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpErrorCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpInvalidCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpMsgCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpReqCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpRespCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpPushCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpEventCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpSendCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpRecvBytes),
		(long long)XS_HttpMetricGet(&g_iXsXtpSendBytes),
		XS_XtpLastMsgTypeName()[0] ? XS_XtpLastMsgTypeName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsXtpLastStatus),
		(long long)XS_HttpMetricGet(&g_iXsXtpLastMsgID),
		(long long)XS_HttpMetricGet(&g_iXsXtpLastFlags),
		(long long)XS_HttpMetricGet(&g_iXsXtpLastParamCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpLastBodySize),
		g_sXsXtpLastRemote[0] ? g_sXsXtpLastRemote : "(none)",
		(long long)g_iXsXtpLastBytes,
		g_sXsXtpLastCmd[0] ? g_sXsXtpLastCmd : "(none)",
		sXtpLastTime ? sXtpLastTime : "(none)",
		(long long)XS_XtpLastAgeMS(),
		g_sXsXtpLastInvalidReason[0] ? g_sXsXtpLastInvalidReason : "(none)",
		sXtpLastInvalidTime ? sXtpLastInvalidTime : "(none)",
		(long long)XS_XtpLastInvalidAgeMS(),
		(long long)g_iXsXtpLastErrorCode,
		sXtpLastErrorTime ? sXtpLastErrorTime : "(none)",
		(long long)XS_XtpLastErrorAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsXtpIdleCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsXtpConnLimitCloseCount),
		sXtpLastIdleCloseTime ? sXtpLastIdleCloseTime : "(none)",
		(long long)XS_XtpLastIdleCloseAgeMS(),
		sXtpLastConnLimitCloseTime ? sXtpLastConnLimitCloseTime : "(none)",
		(long long)XS_XtpLastConnLimitCloseAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsUdpRecvCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpSendCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpErrorCount),
		(long long)g_iXsUdpLastErrorCode,
		(long long)XS_HttpMetricGet(&g_iXsUdpRecvBytes),
		(long long)XS_HttpMetricGet(&g_iXsUdpSendBytes),
		g_sXsUdpLastFrom[0] ? g_sXsUdpLastFrom : "(none)",
		g_sXsUdpLastText[0] ? g_sXsUdpLastText : "(none)",
		(long long)g_iXsUdpLastBytes,
		sUdpLastTime ? sUdpLastTime : "(none)",
		(long long)XS_UdpLastAgeMS(),
		sUdpLastErrorTime ? sUdpLastErrorTime : "(none)",
		(long long)XS_UdpLastErrorAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsCustomConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsCustomConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsCustomOpenCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomErrorCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomInvalidCount),
		g_sXsCustomLastInvalidReason[0] ? g_sXsCustomLastInvalidReason : "(none)",
		sCustomLastInvalidTime ? sCustomLastInvalidTime : "(none)",
		(long long)XS_CustomLastInvalidAgeMS(),
		(long long)g_iXsCustomLastCloseReason,
		sCustomLastCloseTime ? sCustomLastCloseTime : "(none)",
		(long long)XS_CustomLastCloseAgeMS(),
		(long long)g_iXsCustomLastErrorCode,
		sCustomLastErrorTime ? sCustomLastErrorTime : "(none)",
		(long long)XS_CustomLastErrorAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsCustomRecvCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomSendCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomRecvBytes),
		(long long)XS_HttpMetricGet(&g_iXsCustomSendBytes),
		g_sXsCustomLastRemote[0] ? g_sXsCustomLastRemote : "(none)",
		(long long)g_iXsCustomLastBytes,
		g_sXsCustomLastText[0] ? g_sXsCustomLastText : "(none)",
		sCustomLastTime ? sCustomLastTime : "(none)",
		(long long)XS_CustomLastAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsCustomIdleCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomConnLimitCloseCount),
		sCustomLastIdleCloseTime ? sCustomLastIdleCloseTime : "(none)",
		(long long)XS_CustomLastIdleCloseAgeMS(),
		sCustomLastConnLimitCloseTime ? sCustomLastConnLimitCloseTime : "(none)",
		(long long)XS_CustomLastConnLimitCloseAgeMS(),
		XS_TlsConfigFileText(objServer->TlsConfig.sCertFile),
		XS_TlsConfigFileText(objServer->TlsConfig.sKeyFile),
		XS_TlsConfigFileText(objServer->TlsConfig.sCaFile),
		sCurrentDir ? sCurrentDir : "(null)",
		xCore.AppFile ? (char*)xCore.AppFile : "(null)",
		sAppMTime ? sAppMTime : "(null)",
		(long long)iAppSize,
		xCore.AppPath ? (char*)xCore.AppPath : "(null)",
		XS_BuildVariant(),
		XS_CompilerName(),
		XS_PlatformName(),
		XS_ArchName(),
		XS_MemDebugEnabled() ? "true" : "false",
		(unsigned long long)XS_ProcessID(),
		sStartTime ? sStartTime : "(null)",
		(long long)XS_ProcessUptimeMS(),
		(unsigned int)g_iXsEngineWorkers,
		(unsigned int)g_iXsRuntimeServerCount,
		(long long)XS_HttpMetricGet(&g_iXsHttpReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpManageReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpAppReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp2xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp3xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp4xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp5xxCount),
		XS_HttpLastMethodName()[0] ? XS_HttpLastMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastStatusCode),
		XS_HttpLastPath()[0] ? XS_HttpLastPath() : "(none)",
		XS_HttpLastTarget()[0] ? XS_HttpLastTarget() : "(none)",
		XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : "(none)",
		(long long)g_iXsHttpLastBodyLen,
		(long long)g_iXsHttpLastHeaderCount,
		(long long)g_iXsHttpLastQueryLen,
		sHttpLastTime ? sHttpLastTime : "(none)",
		(long long)XS_HttpLastRequestAgeMS(),
		(long long)g_iXsHttpLastTimeMS,
		(long long)XS_HttpMetricGet(&g_iXsHttpIdleCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnLimitCloseCount),
		sHttpLastIdleCloseTime ? sHttpLastIdleCloseTime : "(none)",
		(long long)XS_HttpLastIdleCloseAgeMS(),
		sHttpLastConnLimitCloseTime ? sHttpLastConnLimitCloseTime : "(none)",
		(long long)XS_HttpLastConnLimitCloseAgeMS(),
		XS_HttpLastAppMethodName()[0] ? XS_HttpLastAppMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode),
		XS_HttpLastAppPath()[0] ? XS_HttpLastAppPath() : "(none)",
		XS_HttpLastAppTarget()[0] ? XS_HttpLastAppTarget() : "(none)",
		XS_HttpLastAppRemote()[0] ? XS_HttpLastAppRemote() : "(none)",
		(long long)g_iXsHttpLastAppBodyLen,
		(long long)g_iXsHttpLastAppHeaderCount,
		(long long)g_iXsHttpLastAppQueryLen,
		sHttpLastAppTime ? sHttpLastAppTime : "(none)",
		(long long)XS_HttpLastAppRequestAgeMS(),
		(long long)g_iXsHttpLastAppTimeMS,
		XS_ManageAPIEnabled(objServer, objHost) ? "true" : "false",
		objServer->Debug ? "true" : "false",
		g_sXsConfigFile ? g_sXsConfigFile : "(null)",
		sConfigName ? sConfigName : "(null)",
		sConfigMTime ? sConfigMTime : "(null)",
		(long long)iConfigSize,
		sConfigBase ? sConfigBase : "(null)",
		tCheckStatus.LastResult ? "true" : "false",
		tCheckStatus.sLastFile[0] ? tCheckStatus.sLastFile : "(none)",
		tCheckStatus.sLastBase[0] ? tCheckStatus.sLastBase : "(none)",
		(long long)tCheckStatus.iLastServerCount,
		tCheckStatus.sLastMessage[0] ? tCheckStatus.sLastMessage : "(none)",
		sCheckTime ? sCheckTime : "(none)",
		(long long)XS_CheckConfigAgeMSByStatus(&tCheckStatus),
		objServer->HostAware ? "true" : "false",
		objServer->EnableDefaultHost ? "true" : "false",
		(unsigned int)(objServer->Hosts ? objServer->Hosts->Count : 0),
		bScriptLoaded ? "true" : "false",
		tReloadStatus.Busy ? "true" : "false",
		tReloadStatus.HasResult ? "true" : "false",
		tReloadStatus.Success ? "true" : "false",
		tReloadStatus.sServerName[0] ? tReloadStatus.sServerName : "(all)",
		tReloadStatus.sHostName[0] ? tReloadStatus.sHostName : "(all)",
		tReloadStatus.sMessage[0] ? tReloadStatus.sMessage : "(none)",
		(long long)iBusQueueCount,
		(long long)iBusDataCount,
		(long long)iBusTotalQueued,
		(long long)iBusTotalDelivered,
		(long long)iBusTotalDropped,
		(long long)tBusLastQueue,
		sBusQueueTime ? sBusQueueTime : "(none)",
		(long long)tBusLastDispatch,
		sBusDispatchTime ? sBusDispatchTime : "(none)",
		(long long)iBusSweepIntervalMS,
		(long long)iBusSweepBatchLimit,
		(long long)iBusSweepCount,
		(long long)iBusSweepRemovedCount,
		(long long)tBusLastSweep,
		sBusLastSweepTime ? sBusLastSweepTime : "(none)",
		(long long)((tBusLastSweep > 0) ? ((xrtNow() - tBusLastSweep) * 1000) : -1),
		(long long)iBusLastSweepRemoved,
		(long long)iBusLastSweepRemain,
		(long long)iBusCleanupCount,
		(long long)tBusLastCleanup,
		sBusLastCleanupTime ? sBusLastCleanupTime : "(none)",
		(long long)((tBusLastCleanup > 0) ? ((xrtNow() - tBusLastCleanup) * 1000) : -1),
		(long long)iBusLastCleanupRemoved,
		(long long)iBusLastCleanupRemain
	);
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"reload_time=%s\nreload_age_ms=%lld\nreload_total_count=%lld\nreload_success_count=%lld\nreload_failure_count=%lld\n",
			sReloadTime ? sReloadTime : "(none)",
			(long long)XS_ReloadAgeMSByStatus(&tReloadStatus),
			(long long)tReloadStatus.iTotalCount,
			(long long)tReloadStatus.iSuccessCount,
			(long long)tReloadStatus.iFailureCount
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_get_count=%lld\nhttp_post_count=%lld\nhttp_head_count=%lld\nhttp_other_count=%lld\nhttp_time_total_ms=%lld\nhttp_time_max_ms=%lld\nhttp_time_avg_ms=%lld\nhttp_last_version=%s\nhttp_last_host=%s\nhttp_last_user_agent=%s\nhttp_last_referer=%s\nhttp_last_origin=%s\nhttp_last_accept=%s\nhttp_last_accept_encoding=%s\nhttp_last_cookie=%s\nhttp_last_forwarded_for=%s\nhttp_last_real_ip=%s\nhttp_last_connection=%s\nhttp_last_cache_control=%s\nhttp_last_content_type=%s\nhttp_last_app_version=%s\nhttp_last_app_host=%s\nhttp_last_app_user_agent=%s\nhttp_last_app_referer=%s\nhttp_last_app_origin=%s\nhttp_last_app_accept=%s\nhttp_last_app_accept_encoding=%s\nhttp_last_app_cookie=%s\nhttp_last_app_forwarded_for=%s\nhttp_last_app_real_ip=%s\nhttp_last_app_connection=%s\nhttp_last_app_cache_control=%s\nhttp_last_app_content_type=%s\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodGetCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodPostCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodHeadCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodOtherCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpTimeTotalMS),
			(long long)XS_HttpMetricGet(&g_iXsHttpTimeMaxMS),
			(long long)((XS_HttpMetricGet(&g_iXsHttpReqCount) > 0) ? (XS_HttpMetricGet(&g_iXsHttpTimeTotalMS) / XS_HttpMetricGet(&g_iXsHttpReqCount)) : 0),
			g_sXsHttpLastVersion[0] ? g_sXsHttpLastVersion : "(none)",
			g_sXsHttpLastHost[0] ? g_sXsHttpLastHost : "(none)",
			g_sXsHttpLastUserAgent[0] ? g_sXsHttpLastUserAgent : "(none)",
			g_sXsHttpLastReferer[0] ? g_sXsHttpLastReferer : "(none)",
			g_sXsHttpLastOrigin[0] ? g_sXsHttpLastOrigin : "(none)",
			g_sXsHttpLastAccept[0] ? g_sXsHttpLastAccept : "(none)",
			g_sXsHttpLastAcceptEncoding[0] ? g_sXsHttpLastAcceptEncoding : "(none)",
			g_sXsHttpLastCookie[0] ? g_sXsHttpLastCookie : "(none)",
			g_sXsHttpLastForwardedFor[0] ? g_sXsHttpLastForwardedFor : "(none)",
			g_sXsHttpLastRealIP[0] ? g_sXsHttpLastRealIP : "(none)",
			g_sXsHttpLastConnection[0] ? g_sXsHttpLastConnection : "(none)",
			g_sXsHttpLastCacheControl[0] ? g_sXsHttpLastCacheControl : "(none)",
			g_sXsHttpLastContentType[0] ? g_sXsHttpLastContentType : "(none)",
			g_sXsHttpLastAppVersion[0] ? g_sXsHttpLastAppVersion : "(none)",
			g_sXsHttpLastAppHost[0] ? g_sXsHttpLastAppHost : "(none)",
			g_sXsHttpLastAppUserAgent[0] ? g_sXsHttpLastAppUserAgent : "(none)",
			g_sXsHttpLastAppReferer[0] ? g_sXsHttpLastAppReferer : "(none)",
			g_sXsHttpLastAppOrigin[0] ? g_sXsHttpLastAppOrigin : "(none)",
			g_sXsHttpLastAppAccept[0] ? g_sXsHttpLastAppAccept : "(none)",
			g_sXsHttpLastAppAcceptEncoding[0] ? g_sXsHttpLastAppAcceptEncoding : "(none)",
			g_sXsHttpLastAppCookie[0] ? g_sXsHttpLastAppCookie : "(none)",
			g_sXsHttpLastAppForwardedFor[0] ? g_sXsHttpLastAppForwardedFor : "(none)",
			g_sXsHttpLastAppRealIP[0] ? g_sXsHttpLastAppRealIP : "(none)",
			g_sXsHttpLastAppConnection[0] ? g_sXsHttpLastAppConnection : "(none)",
			g_sXsHttpLastAppCacheControl[0] ? g_sXsHttpLastAppCacheControl : "(none)",
			g_sXsHttpLastAppContentType[0] ? g_sXsHttpLastAppContentType : "(none)"
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"bus_items=%s\n",
			sBusItems ? sBusItems : "[]"
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"check_total_count=%lld\ncheck_success_count=%lld\ncheck_failure_count=%lld\n",
			(long long)tCheckStatus.iTotalCount,
			(long long)tCheckStatus.iSuccessCount,
			(long long)tCheckStatus.iFailureCount
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"check_has_result=%s\ncheck_last_result=%s\ncheck_last_file=%s\ncheck_last_base=%s\ncheck_last_server_count=%lld\ncheck_last_message=%s\n",
			tCheckStatus.HasResult ? "true" : "false",
			tCheckStatus.HasResult ? (tCheckStatus.LastResult ? "true" : "false") : "(none)",
			tCheckStatus.sLastFile[0] ? tCheckStatus.sLastFile : "(none)",
			tCheckStatus.sLastBase[0] ? tCheckStatus.sLastBase : "(none)",
			(long long)tCheckStatus.iLastServerCount,
			tCheckStatus.sLastMessage[0] ? tCheckStatus.sLastMessage : "(none)"
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_conn_current=%lld\nhttp_conn_peak=%lld\nhttp_idle_close_count=%lld\nhttp_last_idle_close_time=%s\nhttp_last_idle_close_age_ms=%lld\nhttp_reject_count=%lld\nhttp_last_reject_status=%lld\nhttp_last_reject_reason=%s\nhttp_last_reject_time=%s\nhttp_last_reject_age_ms=%lld\nhttp_stop_cleanup_count=%lld\nhttp_last_stop_cleanup_closed=%lld\nhttp_last_stop_cleanup_remain=%lld\nhttp_last_stop_cleanup_time=%s\nhttp_last_stop_cleanup_age_ms=%lld\nws_reject_count=%lld\nws_last_reject_reason=%s\nws_last_reject_time=%s\nws_last_reject_age_ms=%lld\nws_stop_cleanup_count=%lld\nws_last_stop_cleanup_closed=%lld\nws_last_stop_cleanup_remain=%lld\nws_last_stop_cleanup_time=%s\nws_last_stop_cleanup_age_ms=%lld\nxtp_reject_count=%lld\nxtp_last_reject_reason=%s\nxtp_last_reject_time=%s\nxtp_last_reject_age_ms=%lld\nxtp_stop_cleanup_count=%lld\nxtp_last_stop_cleanup_closed=%lld\nxtp_last_stop_cleanup_remain=%lld\nxtp_last_stop_cleanup_time=%s\nxtp_last_stop_cleanup_age_ms=%lld\ncustom_reject_count=%lld\ncustom_last_reject_reason=%s\ncustom_last_reject_time=%s\ncustom_last_reject_age_ms=%lld\ncustom_stop_cleanup_count=%lld\ncustom_last_stop_cleanup_closed=%lld\ncustom_last_stop_cleanup_remain=%lld\ncustom_last_stop_cleanup_time=%s\ncustom_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpConnCurrent),
			(long long)XS_HttpMetricGet(&g_iXsHttpConnPeak),
			(long long)XS_HttpMetricGet(&g_iXsHttpIdleCloseCount),
			sHttpLastIdleCloseTime ? sHttpLastIdleCloseTime : "(none)",
			(long long)XS_HttpLastIdleCloseAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastRejectStatus),
			g_sXsHttpLastRejectReason[0] ? g_sXsHttpLastRejectReason : "(none)",
			sHttpLastRejectTime ? sHttpLastRejectTime : "(none)",
			(long long)XS_HttpLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastStopCleanupRemain),
			sHttpLastStopCleanupTime ? sHttpLastStopCleanupTime : "(none)",
			(long long)XS_HttpLastStopCleanupAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsWsRejectCount),
			g_sXsWsLastRejectReason[0] ? g_sXsWsLastRejectReason : "(none)",
			sWsLastRejectTime ? sWsLastRejectTime : "(none)",
			(long long)XS_WsLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsWsStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsWsLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsWsLastStopCleanupRemain),
			sWsLastStopCleanupTime ? sWsLastStopCleanupTime : "(none)",
			(long long)XS_WsLastStopCleanupAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpRejectCount),
			g_sXsXtpLastRejectReason[0] ? g_sXsXtpLastRejectReason : "(none)",
			sXtpLastRejectTime ? sXtpLastRejectTime : "(none)",
			(long long)XS_XtpLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastStopCleanupRemain),
			sXtpLastStopCleanupTime ? sXtpLastStopCleanupTime : "(none)",
			(long long)XS_XtpLastStopCleanupAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomRejectCount),
			g_sXsCustomLastRejectReason[0] ? g_sXsCustomLastRejectReason : "(none)",
			sCustomLastRejectTime ? sCustomLastRejectTime : "(none)",
			(long long)XS_CustomLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsCustomLastStopCleanupRemain),
			sCustomLastStopCleanupTime ? sCustomLastStopCleanupTime : "(none)",
			(long long)XS_CustomLastStopCleanupAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_header_limit_reject_count=%lld\nhttp_last_header_limit_reject_time=%s\nhttp_last_header_limit_reject_age_ms=%lld\nhttp_body_limit_reject_count=%lld\nhttp_last_body_limit_reject_time=%s\nhttp_last_body_limit_reject_age_ms=%lld\nhttp_path_limit_reject_count=%lld\nhttp_last_path_limit_reject_time=%s\nhttp_last_path_limit_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpHeaderLimitRejectCount),
			sHttpLastHeaderLimitRejectTime ? sHttpLastHeaderLimitRejectTime : "(none)",
			(long long)XS_HttpLastHeaderLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBodyLimitRejectCount),
			sHttpLastBodyLimitRejectTime ? sHttpLastBodyLimitRejectTime : "(none)",
			(long long)XS_HttpLastBodyLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpPathLimitRejectCount),
			sHttpLastPathLimitRejectTime ? sHttpLastPathLimitRejectTime : "(none)",
			(long long)XS_HttpLastPathLimitRejectAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_api_disabled_reject_count=%lld\nhttp_last_api_disabled_reject_time=%s\nhttp_last_api_disabled_reject_age_ms=%lld\nhttp_method_reject_count=%lld\nhttp_last_method_reject_time=%s\nhttp_last_method_reject_age_ms=%lld\nhttp_host_not_found_reject_count=%lld\nhttp_last_host_not_found_reject_time=%s\nhttp_last_host_not_found_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpApiDisabledRejectCount),
			sHttpLastApiDisabledRejectTime ? sHttpLastApiDisabledRejectTime : "(none)",
			(long long)XS_HttpLastApiDisabledRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpMethodRejectCount),
			sHttpLastMethodRejectTime ? sHttpLastMethodRejectTime : "(none)",
			(long long)XS_HttpLastMethodRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpHostNotFoundRejectCount),
			sHttpLastHostNotFoundRejectTime ? sHttpLastHostNotFoundRejectTime : "(none)",
			(long long)XS_HttpLastHostNotFoundRejectAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_reload_busy_reject_count=%lld\nhttp_last_reload_busy_reject_time=%s\nhttp_last_reload_busy_reject_age_ms=%lld\nhttp_reload_failed_reject_count=%lld\nhttp_last_reload_failed_reject_time=%s\nhttp_last_reload_failed_reject_age_ms=%lld\nhttp_check_config_failed_reject_count=%lld\nhttp_last_check_config_failed_reject_time=%s\nhttp_last_check_config_failed_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpReloadBusyRejectCount),
			sHttpLastReloadBusyRejectTime ? sHttpLastReloadBusyRejectTime : "(none)",
			(long long)XS_HttpLastReloadBusyRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpReloadFailedRejectCount),
			sHttpLastReloadFailedRejectTime ? sHttpLastReloadFailedRejectTime : "(none)",
			(long long)XS_HttpLastReloadFailedRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpCheckConfigFailedRejectCount),
			sHttpLastCheckConfigFailedRejectTime ? sHttpLastCheckConfigFailedRejectTime : "(none)",
			(long long)XS_HttpLastCheckConfigFailedRejectAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"http_bus_bad_request_reject_count=%lld\nhttp_last_bus_bad_request_reject_time=%s\nhttp_last_bus_bad_request_reject_age_ms=%lld\nhttp_bus_not_found_reject_count=%lld\nhttp_last_bus_not_found_reject_time=%s\nhttp_last_bus_not_found_reject_age_ms=%lld\nhttp_bus_limit_reject_count=%lld\nhttp_last_bus_limit_reject_time=%s\nhttp_last_bus_limit_reject_age_ms=%lld\nhttp_bus_failed_reject_count=%lld\nhttp_last_bus_failed_reject_time=%s\nhttp_last_bus_failed_reject_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpBusBadRequestRejectCount),
			sHttpLastBusBadRequestRejectTime ? sHttpLastBusBadRequestRejectTime : "(none)",
			(long long)XS_HttpLastBusBadRequestRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusNotFoundRejectCount),
			sHttpLastBusNotFoundRejectTime ? sHttpLastBusNotFoundRejectTime : "(none)",
			(long long)XS_HttpLastBusNotFoundRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusLimitRejectCount),
			sHttpLastBusLimitRejectTime ? sHttpLastBusLimitRejectTime : "(none)",
			(long long)XS_HttpLastBusLimitRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsHttpBusFailedRejectCount),
			sHttpLastBusFailedRejectTime ? sHttpLastBusFailedRejectTime : "(none)",
			(long long)XS_HttpLastBusFailedRejectAgeMS()
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"bus_namespace_count=%lld\nbus_namespace_item_count=%lld\nbus_data_limit=%lld\nbus_queue_limit=%lld\nbus_namespace_limit=%lld\nbus_namespace_data_limit=%lld\nbus_namespace_limit_remaining=%lld\nbus_namespace_limit_reached=%s\nbus_readonly_namespace_count=%lld\nbus_disabled_namespace_count=%lld\nbus_ttl_required_namespace_count=%lld\nbus_tag_required_namespace_count=%lld\nbus_readonly_namespaces=%s\nbus_disabled_namespaces=%s\nbus_ttl_required_namespaces=%s\nbus_tag_required_namespaces=%s\nbus_data_limit_reject_count=%lld\nbus_last_data_limit_reject_time=%s\nbus_last_data_limit_reject_age_ms=%lld\nbus_last_data_limit_count=%lld\nbus_queue_limit_reject_count=%lld\nbus_last_queue_limit_reject_time=%s\nbus_last_queue_limit_reject_age_ms=%lld\nbus_last_queue_limit_count=%lld\n",
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_count", sizeof("namespace_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_item_count", sizeof("namespace_item_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "data_limit", sizeof("data_limit") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "queue_limit", sizeof("queue_limit") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_limit", sizeof("namespace_limit") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_data_limit", sizeof("namespace_data_limit") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_limit_remaining", sizeof("namespace_limit_remaining") - 1) : 0),
			(objBus && xvoGetBool(xvoTableGetValue(objBus, "namespace_limit_reached", sizeof("namespace_limit_reached") - 1))) ? "true" : "false",
			(long long)(objBus ? xvoTableGetInt(objBus, "readonly_namespace_count", sizeof("readonly_namespace_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "disabled_namespace_count", sizeof("disabled_namespace_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "ttl_required_namespace_count", sizeof("ttl_required_namespace_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "tag_required_namespace_count", sizeof("tag_required_namespace_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "readonly_namespaces", sizeof("readonly_namespaces") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "disabled_namespaces", sizeof("disabled_namespaces") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "ttl_required_namespaces", sizeof("ttl_required_namespaces") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "tag_required_namespaces", sizeof("tag_required_namespaces") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "data_limit_reject_count", sizeof("data_limit_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_data_limit_reject_time", sizeof("last_data_limit_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_data_limit_reject_age_ms", sizeof("last_data_limit_reject_age_ms") - 1) : -1),
			(long long)(objBus ? xvoTableGetInt(objBus, "last_data_limit_count", sizeof("last_data_limit_count") - 1) : 0),
			(long long)(objBus ? xvoTableGetInt(objBus, "queue_limit_reject_count", sizeof("queue_limit_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_queue_limit_reject_time", sizeof("last_queue_limit_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_queue_limit_reject_age_ms", sizeof("last_queue_limit_reject_age_ms") - 1) : -1),
			(long long)(objBus ? xvoTableGetInt(objBus, "last_queue_limit_count", sizeof("last_queue_limit_count") - 1) : 0)
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"bus_namespace_limit_reject_count=%lld\nbus_last_namespace_limit_reject_time=%s\nbus_last_namespace_limit_reject_age_ms=%lld\nbus_last_namespace_limit_count=%lld\nbus_last_namespace_limit_namespace=%s\nbus_namespace_data_limit_reject_count=%lld\nbus_last_namespace_data_limit_reject_time=%s\nbus_last_namespace_data_limit_reject_age_ms=%lld\nbus_last_namespace_data_limit_count=%lld\nbus_last_namespace_data_limit_namespace=%s\nbus_readonly_namespace_reject_count=%lld\nbus_last_readonly_namespace_reject_time=%s\nbus_last_readonly_namespace_reject_age_ms=%lld\nbus_last_readonly_namespace=%s\nbus_last_readonly_namespace_action=%s\nbus_disabled_namespace_reject_count=%lld\nbus_last_disabled_namespace_reject_time=%s\nbus_last_disabled_namespace_reject_age_ms=%lld\nbus_last_disabled_namespace=%s\nbus_last_disabled_namespace_action=%s\nbus_ttl_required_namespace_reject_count=%lld\nbus_last_ttl_required_namespace_reject_time=%s\nbus_last_ttl_required_namespace_reject_age_ms=%lld\nbus_last_ttl_required_namespace=%s\nbus_last_ttl_required_namespace_action=%s\nbus_tag_required_namespace_reject_count=%lld\nbus_last_tag_required_namespace_reject_time=%s\nbus_last_tag_required_namespace_reject_age_ms=%lld\nbus_last_tag_required_namespace=%s\nbus_last_tag_required_namespace_action=%s\n",
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_limit_reject_count", sizeof("namespace_limit_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_namespace_limit_reject_time", sizeof("last_namespace_limit_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_namespace_limit_reject_age_ms", sizeof("last_namespace_limit_reject_age_ms") - 1) : -1),
			(long long)(objBus ? xvoTableGetInt(objBus, "last_namespace_limit_count", sizeof("last_namespace_limit_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_namespace_limit_namespace", sizeof("last_namespace_limit_namespace") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "namespace_data_limit_reject_count", sizeof("namespace_data_limit_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_namespace_data_limit_reject_time", sizeof("last_namespace_data_limit_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_namespace_data_limit_reject_age_ms", sizeof("last_namespace_data_limit_reject_age_ms") - 1) : -1),
			(long long)(objBus ? xvoTableGetInt(objBus, "last_namespace_data_limit_count", sizeof("last_namespace_data_limit_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_namespace_data_limit_namespace", sizeof("last_namespace_data_limit_namespace") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "readonly_namespace_reject_count", sizeof("readonly_namespace_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_readonly_namespace_reject_time", sizeof("last_readonly_namespace_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_readonly_namespace_reject_age_ms", sizeof("last_readonly_namespace_reject_age_ms") - 1) : -1),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_readonly_namespace", sizeof("last_readonly_namespace") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_readonly_namespace_action", sizeof("last_readonly_namespace_action") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "disabled_namespace_reject_count", sizeof("disabled_namespace_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_disabled_namespace_reject_time", sizeof("last_disabled_namespace_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_disabled_namespace_reject_age_ms", sizeof("last_disabled_namespace_reject_age_ms") - 1) : -1),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_disabled_namespace", sizeof("last_disabled_namespace") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_disabled_namespace_action", sizeof("last_disabled_namespace_action") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "ttl_required_namespace_reject_count", sizeof("ttl_required_namespace_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_ttl_required_namespace_reject_time", sizeof("last_ttl_required_namespace_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_ttl_required_namespace_reject_age_ms", sizeof("last_ttl_required_namespace_reject_age_ms") - 1) : -1),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_ttl_required_namespace", sizeof("last_ttl_required_namespace") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_ttl_required_namespace_action", sizeof("last_ttl_required_namespace_action") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "tag_required_namespace_reject_count", sizeof("tag_required_namespace_reject_count") - 1) : 0),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_tag_required_namespace_reject_time", sizeof("last_tag_required_namespace_reject_time") - 1)) : (str)"",
			(long long)(objBus ? xvoTableGetInt(objBus, "last_tag_required_namespace_reject_age_ms", sizeof("last_tag_required_namespace_reject_age_ms") - 1) : -1),
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_tag_required_namespace", sizeof("last_tag_required_namespace") - 1)) : (str)"",
			objBus ? xvoGetText(xvoTableGetValue(objBus, "last_tag_required_namespace_action", sizeof("last_tag_required_namespace_action") - 1)) : (str)""
		);
	}
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"ws_message_limit_close_count=%lld\nws_last_message_limit_close_time=%s\nws_last_message_limit_close_age_ms=%lld\nxtp_recv_limit_close_count=%lld\nxtp_last_recv_limit_close_time=%s\nxtp_last_recv_limit_close_age_ms=%lld\ncustom_recv_limit_close_count=%lld\ncustom_last_recv_limit_close_time=%s\ncustom_last_recv_limit_close_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsWsMessageLimitCloseCount),
			sWsLastMessageLimitCloseTime ? sWsLastMessageLimitCloseTime : "(none)",
			(long long)XS_WsLastMessageLimitCloseAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpRecvLimitCloseCount),
			sXtpLastRecvLimitCloseTime ? sXtpLastRecvLimitCloseTime : "(none)",
			(long long)XS_XtpLastRecvLimitCloseAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomRecvLimitCloseCount),
			sCustomLastRecvLimitCloseTime ? sCustomLastRecvLimitCloseTime : "(none)",
			(long long)XS_CustomLastRecvLimitCloseAgeMS()
		);
	}
	if ( objBus ) {
		xvoUnref(objBus);
	}
	if ( sCurrentDir ) {
		xrtFree(sCurrentDir);
	}
	if ( sConfigName ) {
		xrtFree(sConfigName);
	}
	if ( sAppMTime ) {
		xrtFree(sAppMTime);
	}
	if ( sConfigMTime ) {
		xrtFree(sConfigMTime);
	}
	if ( sCheckTime ) {
		xrtFree(sCheckTime);
	}
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	if ( sBusItems ) {
		xrtFree(sBusItems);
	}
	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
	}
	if ( sHttpLastIdleCloseTime ) {
		xrtFree(sHttpLastIdleCloseTime);
	}
	if ( sHttpLastConnLimitCloseTime ) {
		xrtFree(sHttpLastConnLimitCloseTime);
	}
	if ( sHttpLastRejectTime ) {
		xrtFree(sHttpLastRejectTime);
	}
	if ( sHttpLastStopCleanupTime ) {
		xrtFree(sHttpLastStopCleanupTime);
	}
	if ( sHttpLastHeaderLimitRejectTime ) {
		xrtFree(sHttpLastHeaderLimitRejectTime);
	}
	if ( sHttpLastBodyLimitRejectTime ) {
		xrtFree(sHttpLastBodyLimitRejectTime);
	}
	if ( sHttpLastPathLimitRejectTime ) {
		xrtFree(sHttpLastPathLimitRejectTime);
	}
	if ( sHttpLastApiDisabledRejectTime ) {
		xrtFree(sHttpLastApiDisabledRejectTime);
	}
	if ( sHttpLastMethodRejectTime ) {
		xrtFree(sHttpLastMethodRejectTime);
	}
	if ( sHttpLastHostNotFoundRejectTime ) {
		xrtFree(sHttpLastHostNotFoundRejectTime);
	}
	if ( sHttpLastReloadBusyRejectTime ) {
		xrtFree(sHttpLastReloadBusyRejectTime);
	}
	if ( sHttpLastReloadFailedRejectTime ) {
		xrtFree(sHttpLastReloadFailedRejectTime);
	}
	if ( sHttpLastCheckConfigFailedRejectTime ) {
		xrtFree(sHttpLastCheckConfigFailedRejectTime);
	}
	if ( sHttpLastBusBadRequestRejectTime ) {
		xrtFree(sHttpLastBusBadRequestRejectTime);
	}
	if ( sHttpLastBusNotFoundRejectTime ) {
		xrtFree(sHttpLastBusNotFoundRejectTime);
	}
	if ( sHttpLastBusLimitRejectTime ) {
		xrtFree(sHttpLastBusLimitRejectTime);
	}
	if ( sHttpLastBusFailedRejectTime ) {
		xrtFree(sHttpLastBusFailedRejectTime);
	}
	if ( sWsLastTime ) {
		xrtFree(sWsLastTime);
	}
	if ( sWsLastCloseTime ) {
		xrtFree(sWsLastCloseTime);
	}
	if ( sWsLastErrorTime ) {
		xrtFree(sWsLastErrorTime);
	}
	if ( sWsLastInvalidTime ) {
		xrtFree(sWsLastInvalidTime);
	}
	if ( sWsLastIdleCloseTime ) {
		xrtFree(sWsLastIdleCloseTime);
	}
	if ( sWsLastConnLimitCloseTime ) {
		xrtFree(sWsLastConnLimitCloseTime);
	}
	if ( sWsLastMessageLimitCloseTime ) {
		xrtFree(sWsLastMessageLimitCloseTime);
	}
	if ( sWsLastRejectTime ) {
		xrtFree(sWsLastRejectTime);
	}
	if ( sWsLastStopCleanupTime ) {
		xrtFree(sWsLastStopCleanupTime);
	}
	if ( sXtpLastTime ) {
		xrtFree(sXtpLastTime);
	}
	if ( sXtpLastInvalidTime ) {
		xrtFree(sXtpLastInvalidTime);
	}
	if ( sXtpLastErrorTime ) {
		xrtFree(sXtpLastErrorTime);
	}
	if ( sUdpLastTime ) {
		xrtFree(sUdpLastTime);
	}
	if ( sUdpLastErrorTime ) {
		xrtFree(sUdpLastErrorTime);
	}
	if ( sCustomLastTime ) {
		xrtFree(sCustomLastTime);
	}
	if ( sCustomLastCloseTime ) {
		xrtFree(sCustomLastCloseTime);
	}
	if ( sCustomLastErrorTime ) {
		xrtFree(sCustomLastErrorTime);
	}
	if ( sCustomLastInvalidTime ) {
		xrtFree(sCustomLastInvalidTime);
	}
	if ( sXtpLastIdleCloseTime ) {
		xrtFree(sXtpLastIdleCloseTime);
	}
	if ( sXtpLastConnLimitCloseTime ) {
		xrtFree(sXtpLastConnLimitCloseTime);
	}
	if ( sXtpLastRecvLimitCloseTime ) {
		xrtFree(sXtpLastRecvLimitCloseTime);
	}
	if ( sXtpLastRejectTime ) {
		xrtFree(sXtpLastRejectTime);
	}
	if ( sXtpLastStopCleanupTime ) {
		xrtFree(sXtpLastStopCleanupTime);
	}
	if ( sCustomLastIdleCloseTime ) {
		xrtFree(sCustomLastIdleCloseTime);
	}
	if ( sCustomLastConnLimitCloseTime ) {
		xrtFree(sCustomLastConnLimitCloseTime);
	}
	if ( sCustomLastRecvLimitCloseTime ) {
		xrtFree(sCustomLastRecvLimitCloseTime);
	}
	if ( sCustomLastRejectTime ) {
		xrtFree(sCustomLastRejectTime);
	}
	if ( sCustomLastStopCleanupTime ) {
		xrtFree(sCustomLastStopCleanupTime);
	}
	if ( sBusQueueTime ) {
		xrtFree(sBusQueueTime);
	}
	if ( sBusDispatchTime ) {
		xrtFree(sBusDispatchTime);
	}
	if ( sBusLastSweepTime ) {
		xrtFree(sBusLastSweepTime);
	}
	if ( sBusLastCleanupTime ) {
		xrtFree(sBusLastCleanupTime);
	}
	if ( sConfigBase ) {
		xrtFree(sConfigBase);
	}
	if ( sStartTime ) {
		xrtFree(sStartTime);
	}
	return XS_HttpRespondText(pResp, bOk ? 200 : 503, bOk ? "OK" : "Service Unavailable", sBody);
}

static inline bool XS_HttpHandleCheckConfig(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_CheckConfigStatusSnapshot tCheckStatus;
	XS_Config objCfgCheck;
	const char* sFilePath;
	char* sFileArg;
	char* sFileResolved;
	char* sLastTime;
	char sBody[1536];
	bool bOK;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/check_config") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "check config api disabled");
	}

	sFileArg = XS_HttpQueryDup(pReq->sQuery, "file");
	sFileResolved = NULL;
	if ( sFileArg && sFileArg[0] ) {
		sFileResolved = XS_ConfigNormalizeRuntimePath(sFileArg);
	}
	sFilePath = (sFileResolved && sFileResolved[0]) ? sFileResolved : ((sFileArg && sFileArg[0]) ? sFileArg : g_sXsConfigFile);
	if ( sFilePath == NULL || sFilePath[0] == '\0' ) {
		XS_CheckConfigRecordResultEx(FALSE, "", "", 0, "config file not set");
		if ( sFileResolved ) {
			xrtFree(sFileResolved);
		}
		if ( sFileArg ) {
			xrtFree(sFileArg);
		}
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "config file not set");
	}

	memset(&objCfgCheck, 0, sizeof(objCfgCheck));
	bOK = XS_LoadConfig(&objCfgCheck, sFilePath);
	XS_CheckConfigRecordResultEx(
		bOK,
		sFilePath,
		(bOK && objCfgCheck.BaseDir) ? objCfgCheck.BaseDir : "",
		(int64)((bOK && objCfgCheck.Servers) ? objCfgCheck.Servers->Count : 0),
		bOK ? "config check passed" : "config check failed"
	);
	XS_ConfigCheckStatusSnapshot(&tCheckStatus);
	sLastTime = XS_CheckConfigTimeTextByStatus(&tCheckStatus);
	snprintf(
		sBody,
		sizeof(sBody),
		"result=%s\nfile=%s\nbase=%s\nserver_count=%u\nmessage=%s\ncheck_total_count=%lld\ncheck_success_count=%lld\ncheck_failure_count=%lld\ncheck_last_time=%s\ncheck_last_age_ms=%lld\n",
		tCheckStatus.LastResult ? "true" : "false",
		tCheckStatus.sLastFile[0] ? tCheckStatus.sLastFile : "(none)",
		tCheckStatus.sLastBase[0] ? tCheckStatus.sLastBase : "(none)",
		(unsigned int)tCheckStatus.iLastServerCount,
		tCheckStatus.sLastMessage[0] ? tCheckStatus.sLastMessage : "(none)",
		(long long)tCheckStatus.iTotalCount,
		(long long)tCheckStatus.iSuccessCount,
		(long long)tCheckStatus.iFailureCount,
		sLastTime ? sLastTime : "(none)",
		(long long)XS_CheckConfigAgeMSByStatus(&tCheckStatus)
	);
	XS_FreeConfig(&objCfgCheck);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sFileResolved ) {
		xrtFree(sFileResolved);
	}
	if ( sFileArg ) {
		xrtFree(sFileArg);
	}
	return XS_HttpRespondText(pResp, bOK ? 200 : 500, bOK ? "OK" : "Internal Server Error", sBody);
}

static inline bool XS_HttpHandleCheckConfigJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_CheckConfigStatusSnapshot tCheckStatus;
	XS_Config objCfgCheck;
	xvalue objRet;
	const char* sFilePath;
	char* sFileArg;
	char* sFileResolved;
	char* sLastTime;
	char* sJson;
	bool bOK;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/check_config_json") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondJsonResult(pResp, 403, "Forbidden", FALSE, "check config json api disabled");
	}

	sFileArg = XS_HttpQueryDup(pReq->sQuery, "file");
	sFileResolved = NULL;
	if ( sFileArg && sFileArg[0] ) {
		sFileResolved = XS_ConfigNormalizeRuntimePath(sFileArg);
	}
	sFilePath = (sFileResolved && sFileResolved[0]) ? sFileResolved : ((sFileArg && sFileArg[0]) ? sFileArg : g_sXsConfigFile);
	if ( sFilePath == NULL || sFilePath[0] == '\0' ) {
		XS_CheckConfigRecordResultEx(FALSE, "", "", 0, "config file not set");
		if ( sFileResolved ) {
			xrtFree(sFileResolved);
		}
		if ( sFileArg ) {
			xrtFree(sFileArg);
		}
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "config file not set");
	}

	memset(&objCfgCheck, 0, sizeof(objCfgCheck));
	bOK = XS_LoadConfig(&objCfgCheck, sFilePath);
	XS_CheckConfigRecordResultEx(
		bOK,
		sFilePath,
		(bOK && objCfgCheck.BaseDir) ? objCfgCheck.BaseDir : "",
		(int64)((bOK && objCfgCheck.Servers) ? objCfgCheck.Servers->Count : 0),
		bOK ? "config check passed" : "config check failed"
	);
	XS_ConfigCheckStatusSnapshot(&tCheckStatus);
	sLastTime = XS_CheckConfigTimeTextByStatus(&tCheckStatus);

	objRet = xvoCreateTable();
	xvoTableSetBool(objRet, "result", 6, tCheckStatus.LastResult);
	xvoTableSetText(objRet, "file", 4, (ptr)tCheckStatus.sLastFile, 0, FALSE);
	xvoTableSetText(objRet, "base", 4, (ptr)tCheckStatus.sLastBase, 0, FALSE);
	xvoTableSetInt(objRet, "server_count", 12, tCheckStatus.iLastServerCount);
	xvoTableSetText(objRet, "message", 7, (ptr)tCheckStatus.sLastMessage, 0, FALSE);
	xvoTableSetInt(objRet, "check_total_count", 17, tCheckStatus.iTotalCount);
	xvoTableSetInt(objRet, "check_success_count", 19, tCheckStatus.iSuccessCount);
	xvoTableSetInt(objRet, "check_failure_count", 19, tCheckStatus.iFailureCount);
	xvoTableSetText(objRet, "check_last_time", 15, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "check_last_age_ms", 17, XS_CheckConfigAgeMSByStatus(&tCheckStatus));

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	XS_FreeConfig(&objCfgCheck);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sFileResolved ) {
		xrtFree(sFileResolved);
	}
	if ( sFileArg ) {
		xrtFree(sFileArg);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondJsonResult(pResp, 500, "Internal Server Error", FALSE, "check config json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, bOK ? 200 : 500, bOK ? "OK" : "Internal Server Error");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
}

static inline bool XS_HttpHandleCheckConfigClear(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_CheckConfigStatusSnapshot tCheckStatus;
	char sBody[512];
	char* sLastTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/check_config_clear") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "check config clear api disabled");
	}

	XS_CheckConfigClearStats();
	XS_ConfigCheckStatusSnapshot(&tCheckStatus);
	sLastTime = XS_CheckConfigTimeTextByStatus(&tCheckStatus);
	snprintf(
		sBody,
		sizeof(sBody),
		"result=%s\nfile=%s\nbase=%s\nserver_count=%u\nmessage=%s\ncheck_total_count=%lld\ncheck_success_count=%lld\ncheck_failure_count=%lld\ncheck_last_time=%s\ncheck_last_age_ms=%lld\n",
		tCheckStatus.LastResult ? "true" : "false",
		tCheckStatus.sLastFile[0] ? tCheckStatus.sLastFile : "(none)",
		tCheckStatus.sLastBase[0] ? tCheckStatus.sLastBase : "(none)",
		(unsigned int)tCheckStatus.iLastServerCount,
		tCheckStatus.sLastMessage[0] ? tCheckStatus.sLastMessage : "(none)",
		(long long)tCheckStatus.iTotalCount,
		(long long)tCheckStatus.iSuccessCount,
		(long long)tCheckStatus.iFailureCount,
		sLastTime ? sLastTime : "(none)",
		(long long)XS_CheckConfigAgeMSByStatus(&tCheckStatus)
	);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

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

	XS_HttpOnOpenMetrics();
	if ( objServer && objServer->ConnLimit > 0u && XS_HttpTrackedConnCount(objHandle) > (int64)objServer->ConnLimit ) {
		if ( objCtx ) {
			objCtx->bClosing = TRUE;
		}
		XS_HttpRecordConnLimitClose();
		XS_LogWarn(
			"http conn limit exceeded: server=%s current=%lld limit=%u",
			objServer->Name ? objServer->Name : "(null)",
			(long long)XS_HttpTrackedConnCount(objHandle),
			(unsigned)objServer->ConnLimit
		);
		if ( pConn && pConn->pStream ) {
			xrtNetStreamClose(pConn->pStream, 0u);
		}
		return;
	}
	
	XS_LogInfo("http open: server=%s", objServer && objServer->Name ? objServer->Name : "(null)");
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
	XS_HttpRecordMethodMetrics(pReq);

	XS_HttpApplyDefaultHeaders(pReq, pResp);
	
	objHost = XS_HttpLocateHost(objServer, pReq);
	if ( objHost == NULL ) {
		if ( XS_HttpPathExpectsJSONError(pReq->sPath) ) {
			bRet = XS_HttpRespondJsonResult(pResp, 404, "Not Found", FALSE, "host not found");
		} else {
			bRet = XS_HttpRespondText(pResp, 404, "Not Found", "host not found");
		}
		goto end;
	}

	if ( XS_HttpValidateRequest(objServer, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	
	XS_LogInfo(
		"http request: server=%s host=%s method=%s path=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost->Name ? objHost->Name : "(default)",
		pReq->sMethod,
		pReq->sPath
	);
	
	if ( XS_HttpHandleReload(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleReloadJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleConfigReload(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleConfigReloadJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleConfigReloadStatus(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleConfigReloadStatusJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleConfigReloadClear(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleConfigReloadReset(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleHealth(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleMetrics(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleMetricsJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleMetricsClear(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleWsMetrics(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleWsMetricsJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleWsMetricsClear(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleXtpMetrics(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleXtpMetricsJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleXtpMetricsClear(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleUdpMetrics(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleUdpMetricsJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleUdpMetricsClear(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleCustomMetrics(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleCustomMetricsJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleCustomMetricsClear(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleDashboard(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleCheckConfig(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleHealthJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleDashboardJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleCheckConfigJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleCheckConfigClear(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusStatus(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusNamespaces(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusFind(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusExists(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusGet(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusValues(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusRetain(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusRelease(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusTouch(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusSet(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusRemove(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusReset(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusSweep(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusLimits(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusRegister(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleBusSend(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleStatus(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}
	if ( XS_HttpHandleStatusJson(objServer, objHost, pReq, pResp) ) {
		bRet = TRUE;
		goto end;
	}

	if ( XS_HttpPathReservedManage(pReq->sPath) ) {
		if ( XS_HttpPathExpectsJSONError(pReq->sPath) ) {
			bRet = XS_HttpRespondJsonResult(pResp, 404, "Not Found", FALSE, "manage api not found");
		} else {
			bRet = XS_HttpRespondText(pResp, 404, "Not Found", "manage api not found");
		}
		goto end;
	}
	
	if ( objHost->DevMode == XS_DEV_STATIC ) {
		bRet = XS_HttpServeStatic(objHost, pReq, pResp);
		goto end;
	}
	
	if ( objHost->DevMode == XS_DEV_SCRIPT_C ) {
		if ( objHost->procHttpRequest ) {
			if ( XS_HttpHandleScriptHost(objServer, objHost, pReq, pResp) ) {
				bRet = TRUE;
				goto end;
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
		bRet = XS_HttpRespondText(pResp, 404, "Not Found", sBody);
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
	XS_HttpRecordTimeMetrics(pReq, iElapsedMS);
	XS_HttpRecordResponseMetrics(pConn, pReq, pResp);
	return bRet;
}

static void XS_HttpOnClose(ptr pOwner, xhttpdserver* pServer, xhttpdconn* pConn, xnet_result iReason)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	XS_HttpHandle* objHandle = objServer ? (XS_HttpHandle*)objServer->pHandle : NULL;
	XS_HttpConnContext* objCtx = XS_HttpGetConnContext(objHandle, pConn);
	(void)pServer;

	XS_HttpOnCloseMetrics();

	if ( objCtx ) {
		XS_HttpUntrackConn((XS_HttpHandle*)objCtx->pTracker, objCtx);
		xrtFree(objCtx);
	}
	
	XS_LogInfo(
		"http close: server=%s reason=%d",
		objServer && objServer->Name ? objServer->Name : "(null)",
		(int)iReason
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

static inline bool XS_HttpInitServer(xnetengine* pEngine, XS_ServerConfig* objServer)
{
	xhttpdconfig tConfig;
	xhttpdevents tEvents;
	xhttpdserver* pServer;
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
	
	objHandle = (XS_HttpHandle*)xrtCalloc(1, sizeof(XS_HttpHandle));
	if ( objHandle == NULL ) {
		xrtHttpdDestroy(pServer);
		XS_ReportError("http init failed: handle alloc failed");
		return FALSE;
	}
	objHandle->pServer = pServer;
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
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	objHandle = (XS_HttpHandle*)objServer->pHandle;
	pServer = objHandle ? objHandle->pServer : NULL;
	if ( pServer == NULL ) {
		XS_ReportError("http start failed: server handle is null");
		return FALSE;
	}
	if ( xrtHttpdStart(pServer) != XRT_NET_OK ) {
		XS_ReportError("http start failed: xrtHttpdStart returned error");
		return FALSE;
	}
	if ( objHandle && objServer->IdleTimeout > 0 ) {
		objHandle->bStopThread = FALSE;
		objHandle->hIdleThread = xrtThreadCreate(XS_HttpIdleThread, objHandle, 0);
		if ( objHandle->hIdleThread == NULL ) {
			xrtHttpdStop(pServer);
			XS_ReportError("http start failed: idle thread create failed");
			return FALSE;
		}
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
	XS_HttpHandle* objHandle;
	xhttpdserver* pServer;
	int64 iClosedConn;
	int64 iRemainConn;
	
	if ( objServer == NULL ) {
		return;
	}
	
	objHandle = (XS_HttpHandle*)objServer->pHandle;
	pServer = objHandle ? objHandle->pServer : NULL;
	if ( objHandle ) {
		objHandle->bStopThread = TRUE;
		if ( objHandle->hIdleThread ) {
			xrtThreadWait(objHandle->hIdleThread);
			xrtThreadDestroy(objHandle->hIdleThread);
			objHandle->hIdleThread = NULL;
		}
	}
	if ( pServer ) {
		xrtHttpdStop(pServer);
	}
	iClosedConn = XS_HttpCloseTrackedConns(objHandle);
	iRemainConn = XS_HttpWaitTrackedConnDrain(objHandle, 500u);
	if ( iClosedConn > 0 || iRemainConn > 0 ) {
		XS_HttpRecordStopCleanup(iClosedConn, iRemainConn);
		XS_LogInfo(
			"http stop cleanup: server=%s closed=%lld remain=%lld",
			objServer->Name ? objServer->Name : "(null)",
			(long long)iClosedConn,
			(long long)iRemainConn
		);
	}
	if ( pServer ) {
		xrtHttpdDestroy(pServer);
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
		"http stop: server=%s",
		objServer->Name ? objServer->Name : "(null)"
	);
}

#endif
