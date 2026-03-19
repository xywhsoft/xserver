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

static inline bool XS_HttpIsManagePath(const char* sPath)
{
	if ( sPath == NULL ) {
		return FALSE;
	}

	return strncmp(sPath, "/__xs/", 6) == 0;
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

static inline void XS_HttpRecordResponseMetrics(const xhttpdrequest* pReq, const xhttpdresponse* pResp)
{
	uint32 iStatusCode;
	bool bManagePath;

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

	iStatusCode = pResp ? pResp->iStatusCode : 0;
	g_iXsHttpLastStatusCode = (int64)iStatusCode;
	g_tXsHttpLastRequestTime = xrtNow();
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
	bManagePath = (pReq && XS_HttpIsManagePath(pReq->sPath));
	if ( !bManagePath ) {
		g_iXsHttpLastAppStatusCode = (int64)iStatusCode;
		g_tXsHttpLastAppRequestTime = g_tXsHttpLastRequestTime;
		g_iXsHttpLastAppMethodType = g_iXsHttpLastMethodType;
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

	XS_HttpMetricAdd(&g_iXsHttpTimeTotalMS, iElapsedMS);
	XS_HttpMetricUpdateMax(&g_iXsHttpTimeMaxMS, iElapsedMS);
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

static inline void XS_HttpClearMetrics(void)
{
	int64 iValue;

	iValue = XS_HttpMetricGet(&g_iXsHttpReqCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsHttpReqCount, -iValue);
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

	g_iXsHttpLastMethodType = 0;
	g_iXsHttpLastAppMethodType = 0;
	g_iXsHttpLastStatusCode = 0;
	g_iXsHttpLastAppStatusCode = 0;
	g_tXsHttpLastRequestTime = 0;
	g_tXsHttpLastAppRequestTime = 0;
	g_sXsHttpLastPath[0] = '\0';
	g_sXsHttpLastTarget[0] = '\0';
	g_sXsHttpLastAppPath[0] = '\0';
	g_sXsHttpLastAppTarget[0] = '\0';
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
	
	g_iXsWsLastFrameType = 0;
	g_iXsWsLastBytes = 0;
	g_tXsWsLastTime = 0;
	g_tXsWsLastErrorTime = 0;
	g_iXsWsLastErrorCode = 0;
	g_sXsWsLastText[0] = '\0';
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
	g_iXsXtpLastMsgType = 0;
	g_iXsXtpLastStatus = 0;
	g_iXsXtpLastMsgID = 0;
	g_tXsXtpLastTime = 0;
	g_sXsXtpLastCmd[0] = '\0';
	g_tXsXtpLastInvalidTime = 0;
	g_sXsXtpLastInvalidReason[0] = '\0';
	g_tXsXtpLastErrorTime = 0;
	g_iXsXtpLastErrorCode = 0;
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
	g_tXsCustomLastTime = 0;
	g_tXsCustomLastErrorTime = 0;
	g_iXsCustomLastCloseReason = 0;
	g_iXsCustomLastErrorCode = 0;
	g_sXsCustomLastText[0] = '\0';
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

static inline int64 XS_CustomLastErrorAgeMS(void)
{
	if ( g_tXsCustomLastErrorTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsCustomLastErrorTime) * 1000;
}

static inline int64 XS_HttpLastRequestAgeMS(void)
{
	if ( g_tXsHttpLastRequestTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsHttpLastRequestTime) * 1000;
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

static inline void XS_CheckConfigRecordResult(bool bSuccess)
{
	XS_HttpMetricAdd(&g_iXsCheckConfigTotalCount, 1);
	if ( bSuccess ) {
		XS_HttpMetricAdd(&g_iXsCheckConfigSuccessCount, 1);
	} else {
		XS_HttpMetricAdd(&g_iXsCheckConfigFailureCount, 1);
	}
	g_tXsCheckConfigLastTime = xrtNow();
}

static inline char* XS_CheckConfigLastTimeText(void)
{
	if ( g_tXsCheckConfigLastTime <= 0 ) {
		return NULL;
	}
	
	return xrtTimeToStr(g_tXsCheckConfigLastTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_CheckConfigLastAgeMS(void)
{
	if ( g_tXsCheckConfigLastTime <= 0 ) {
		return -1;
	}
	
	return (int64)(xrtNow() - g_tXsCheckConfigLastTime) * 1000;
}

static inline void XS_CheckConfigClearStats(void)
{
	int64 iValue;

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
}

static inline char* XS_ReloadTimeText(void)
{
	xtime tReloadTime;

	tReloadTime = XS_ConfigReloadStatusTime();
	if ( tReloadTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(tReloadTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_ReloadAgeMS(void)
{
	xtime tReloadTime;
	double fNowTick;
	double fReloadTick;
	double fSpan;

	tReloadTime = XS_ConfigReloadStatusTime();
	if ( tReloadTime <= 0 ) {
		return -1;
	}

	fNowTick = xrtTimer();
	fReloadTick = g_fXsStartTick + ((double)(tReloadTime - g_tXsStartTime));
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
	if ( pReq && strncmp(pReq->sPath, "/__xs/", 6) == 0 ) {
		(void)xrtHttpdResponseSetHeader(pResp, "Cache-Control", "no-store");
		(void)xrtHttpdResponseSetHeader(pResp, "X-Frame-Options", "DENY");
		(void)xrtHttpdResponseSetHeader(pResp, "Referrer-Policy", "no-referrer");
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

	if ( strncmp(pReq->sPath, "/__xs/bus/", 10) == 0 ) {
		if ( _stricmp(pReq->sMethod, "GET") != 0 && _stricmp(pReq->sMethod, "HEAD") != 0 ) {
			xrtHttpdResponseSetHeader(pResp, "Allow", "GET, HEAD");
			return XS_HttpRespondText(pResp, 405, "Method Not Allowed", "bus management api only supports GET or HEAD");
		}
	}

	if (
		strcmp(pReq->sPath, "/__xs/health") == 0 ||
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
		strcmp(pReq->sPath, "/__xs/custom_metrics_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/dashboard") == 0 ||
		strcmp(pReq->sPath, "/__xs/check_config") == 0 ||
		strcmp(pReq->sPath, "/__xs/check_config_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/check_config_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/status") == 0 ||
		strcmp(pReq->sPath, "/__xs/status_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/reload_clear") == 0 ||
		strcmp(pReq->sPath, "/__xs/reload_reset") == 0 ||
		strcmp(pReq->sPath, "/__xs/reload_status") == 0 ||
		strcmp(pReq->sPath, "/__xs/reload_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/reload_status_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/reload_config_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/health_json") == 0 ||
		strcmp(pReq->sPath, "/__xs/dashboard_json") == 0
	) {
		if ( _stricmp(pReq->sMethod, "GET") != 0 && _stricmp(pReq->sMethod, "HEAD") != 0 ) {
			xrtHttpdResponseSetHeader(pResp, "Allow", "GET, HEAD");
			return XS_HttpRespondText(pResp, 405, "Method Not Allowed", "status api only supports GET or HEAD");
		}
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

static inline bool XS_HttpHandleReloadJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sJson;
	char* sReloadHostName;
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
		return XS_HttpRespondText(pResp, 403, "Forbidden", "reload json api disabled");
	}
	if ( _stricmp(pReq->sMethod, "GET") != 0 && _stricmp(pReq->sMethod, "POST") != 0 ) {
		return XS_HttpRespondText(pResp, 405, "Method Not Allowed", "reload json api only supports GET or POST");
	}

	sReloadHostName = XS_HttpQueryText(pReq->sQuery, "host");
	bForce = XS_HttpQueryBool(pReq->sQuery, "force", FALSE);
	if ( sReloadHostName && sReloadHostName[0] ) {
		objReloadHost = XS_FindServerHostByName(objServer, sReloadHostName);
	} else {
		objReloadHost = (XS_HostConfig*)objHost;
	}
	if ( objReloadHost == NULL ) {
		if ( sReloadHostName ) {
			xrtFree(sReloadHostName);
		}
		return XS_HttpRespondText(pResp, 404, "Not Found", "reload host not found");
	}

	iRet = XS_ReloadServerHostScript(objServer, objReloadHost, bForce);
	objRet = xvoCreateTable();
	xvoTableSetBool(objRet, "result", 6, (iRet == 0));
	xvoTableSetText(objRet, "message", 7, (ptr)XS_ReloadResultText(iRet), 0, FALSE);
	xvoTableSetText(objRet, "server", 6, (ptr)(objServer->Name ? objServer->Name : "(null)"), 0, FALSE);
	xvoTableSetText(objRet, "host", 4, (ptr)(objReloadHost->Name ? objReloadHost->Name : "(default)"), 0, FALSE);
	xvoTableSetBool(objRet, "force", 5, bForce);
	xvoTableSetInt(objRet, "code", 4, iRet);

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sReloadHostName ) {
		xrtFree(sReloadHostName);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "reload json build failed");
	}
	xrtHttpdResponseSetStatus(pResp, (iRet == 0) ? 200 : 500, (iRet == 0) ? "OK" : "Internal Server Error");
	if ( !xrtHttpdResponseSetBodyCopy(pResp, sJson, strlen(sJson), "application/json; charset=utf-8") ) {
		xrtFree(sJson);
		return FALSE;
	}
	xrtFree(sJson);
	return TRUE;
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

static inline bool XS_HttpHandleConfigReloadJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sJson;
	char* sServerName;
	char* sHostName;
	bool bQueued;
	bool bForce;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/reload_config_json") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "config reload json api disabled");
	}
	if ( _stricmp(pReq->sMethod, "GET") != 0 && _stricmp(pReq->sMethod, "POST") != 0 ) {
		return XS_HttpRespondText(pResp, 405, "Method Not Allowed", "config reload json api only supports GET or POST");
	}

	bForce = XS_HttpQueryBool(pReq->sQuery, "force", FALSE);
	sServerName = XS_HttpQueryDup(pReq->sQuery, "server");
	sHostName = XS_HttpQueryDup(pReq->sQuery, "host");
	if ( (sServerName == NULL || sServerName[0] == '\0') && sHostName && sHostName[0] != '\0' ) {
		sServerName = XS_CopyText(objServer->Name);
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
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "config reload json build failed");
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

	sReloadTime = XS_ReloadTimeText();
	
	snprintf(
		sBody,
		sizeof(sBody),
		"busy=%s\nhas_result=%s\nsuccess=%s\nserver=%s\nhost=%s\nmessage=%s\nreload_time=%s\nreload_age_ms=%lld\nreload_total_count=%lld\nreload_success_count=%lld\nreload_failure_count=%lld\n",
		XS_ConfigReloadStatusBusy() ? "true" : "false",
		XS_ConfigReloadStatusHasResult() ? "true" : "false",
		XS_ConfigReloadStatusSuccess() ? "true" : "false",
		XS_ConfigReloadStatusServer(),
		XS_ConfigReloadStatusHost(),
		XS_ConfigReloadStatusMessage(),
		sReloadTime ? sReloadTime : "(none)",
		(long long)XS_ReloadAgeMS(),
		(long long)XS_ConfigReloadStatusTotalCount(),
		(long long)XS_ConfigReloadStatusSuccessCount(),
		(long long)XS_ConfigReloadStatusFailureCount()
	);
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleConfigReloadStatusJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
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
		return XS_HttpRespondText(pResp, 403, "Forbidden", "config reload status json api disabled");
	}

	sReloadTime = XS_ReloadTimeText();
	objRet = xvoCreateTable();
	xvoTableSetBool(objRet, "busy", 4, XS_ConfigReloadStatusBusy());
	xvoTableSetBool(objRet, "has_result", 10, XS_ConfigReloadStatusHasResult());
	xvoTableSetBool(objRet, "success", 7, XS_ConfigReloadStatusSuccess());
	xvoTableSetText(objRet, "server", 6, (ptr)XS_ConfigReloadStatusServer(), 0, FALSE);
	xvoTableSetText(objRet, "host", 4, (ptr)XS_ConfigReloadStatusHost(), 0, FALSE);
	xvoTableSetText(objRet, "message", 7, (ptr)XS_ConfigReloadStatusMessage(), 0, FALSE);
	xvoTableSetText(objRet, "reload_time", 11, (ptr)(sReloadTime ? sReloadTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "reload_age_ms", 13, XS_ReloadAgeMS());
	xvoTableSetInt(objRet, "reload_total_count", 18, XS_ConfigReloadStatusTotalCount());
	xvoTableSetInt(objRet, "reload_success_count", 20, XS_ConfigReloadStatusSuccessCount());
	xvoTableSetInt(objRet, "reload_failure_count", 20, XS_ConfigReloadStatusFailureCount());

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sReloadTime ) {
		xrtFree(sReloadTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "reload status json build failed");
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
	return XS_HttpRespondText(pResp, 200, "OK", "reload status cleared");
}

static inline bool XS_HttpHandleConfigReloadReset(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[256];

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
	snprintf(
		sBody,
		sizeof(sBody),
		"reload_total_count=%lld\nreload_success_count=%lld\nreload_failure_count=%lld\nreload_time=(none)\n",
		(long long)XS_ConfigReloadStatusTotalCount(),
		(long long)XS_ConfigReloadStatusSuccessCount(),
		(long long)XS_ConfigReloadStatusFailureCount()
	);
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
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus status api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
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
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "bus status build failed");
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
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus namespace api disabled");
	}

	sJson = XS_BusBuildNamespaceStatsJson();
	if ( sJson == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "bus namespace build failed");
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
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus find api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
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

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/exists") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus exists api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	iDataID = (sID && sID[0] != '\0') ? _strtoi64(sID, NULL, 10) : 0;
	if ( iDataID <= 0 && ((sNamespace && sNamespace[0] != '\0') || (sTag && sTag[0] != '\0')) ) {
		iDataID = XS_BusDataFindFirst(
			(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
			(sTag && sTag[0] != '\0') ? sTag : NULL
		);
	}
	bExists = (iDataID > 0);
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
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus get api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	iDataID = (sID && sID[0] != '\0') ? _strtoi64(sID, NULL, 10) : 0;

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
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus values api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
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

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/retain") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus retain api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	iDataID = (sID && sID[0] != '\0') ? _strtoi64(sID, NULL, 10) : 0;
	iDataID = XS_BusDataResolveID(
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
		(sTag && sTag[0] != '\0') ? sTag : NULL,
		iDataID
	);
	bOk = (iDataID > 0) ? XS_BusDataRetain(iDataID) : FALSE;
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
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sID ) {
		xrtFree(sID);
	}
	xrtHttpdResponseSetStatus(pResp, bOk ? 200 : 404, bOk ? "OK" : "Not Found");
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

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/release") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus release api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	iDataID = (sID && sID[0] != '\0') ? _strtoi64(sID, NULL, 10) : 0;
	iDataID = XS_BusDataResolveID(
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
		(sTag && sTag[0] != '\0') ? sTag : NULL,
		iDataID
	);
	bOk = (iDataID > 0) ? XS_BusDataRelease(iDataID) : FALSE;
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
	if ( sNamespace ) {
		xrtFree(sNamespace);
	}
	if ( sTag ) {
		xrtFree(sTag);
	}
	if ( sID ) {
		xrtFree(sID);
	}
	xrtHttpdResponseSetStatus(pResp, bOk ? 200 : 404, bOk ? "OK" : "Not Found");
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

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/touch") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus touch api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	sTTL = XS_HttpQueryDup(pReq->sQuery, "ttl");
	iDataID = (sID && sID[0] != '\0') ? _strtoi64(sID, NULL, 10) : 0;
	iDataID = XS_BusDataResolveID(
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
		(sTag && sTag[0] != '\0') ? sTag : NULL,
		iDataID
	);
	iTTL = (sTTL && sTTL[0] != '\0') ? _strtoi64(sTTL, NULL, 10) : 0;
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
	tExpire = bOk ? XS_BusDataGetExpireTime(iDataID) : 0;
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
	xrtHttpdResponseSetStatus(pResp, bOk ? 200 : 404, bOk ? "OK" : "Not Found");
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
	xvalue objData = NULL;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/set") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus set api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	sText = XS_HttpQueryDup(pReq->sQuery, "text");
	sJson = XS_HttpQueryDup(pReq->sQuery, "json");
	sTTL = XS_HttpQueryDup(pReq->sQuery, "ttl");
	iDataID = (sID && sID[0] != '\0') ? _strtoi64(sID, NULL, 10) : 0;
	iDataID = XS_BusDataResolveID(
		(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
		(sTag && sTag[0] != '\0') ? sTag : NULL,
		iDataID
	);
	if ( sTTL && sTTL[0] != '\0' ) {
		iTTL = _strtoi64(sTTL, NULL, 10);
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

	bOk = XS_BusDataSetValue(iDataID, objData);
	if ( bOk && iTTL >= 0 ) {
		bOk = XS_BusDataSetTTL(iDataID, iTTL);
	}
	tExpire = bOk ? XS_BusDataGetExpireTime(iDataID) : 0;
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
	xrtHttpdResponseSetStatus(pResp, bOk ? 200 : 500, bOk ? "OK" : "Internal Server Error");

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

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/remove") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus remove api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sID = XS_HttpQueryDup(pReq->sQuery, "id");
	bAll = XS_HttpQueryBool(pReq->sQuery, "all", FALSE);
	iRemoved = 0;
	iDataID = 0;

	if ( sID && sID[0] != '\0' ) {
		iDataID = _strtoi64(sID, NULL, 10);
		if ( iDataID > 0 && XS_BusDataRemove(iDataID) ) {
			iRemoved = 1;
		}
	} else if ( (sNamespace && sNamespace[0] != '\0') || (sTag && sTag[0] != '\0') ) {
		if ( bAll ) {
			iRemoved = XS_BusDataRemoveByQuery(
				(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
				(sTag && sTag[0] != '\0') ? sTag : NULL,
				256
			);
		} else {
			iDataID = XS_BusDataFindFirst(
				(sNamespace && sNamespace[0] != '\0') ? sNamespace : NULL,
				(sTag && sTag[0] != '\0') ? sTag : NULL
			);
			if ( iDataID > 0 && XS_BusDataRemove(iDataID) ) {
				iRemoved = 1;
			}
		}
	}

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

static inline bool XS_HttpHandleBusReset(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[256];

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/reset") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus reset api disabled");
	}

	XS_BusClearStats();
	snprintf(
		sBody,
		sizeof(sBody),
		"total_queued=0\ntotal_delivered=0\ntotal_dropped=0\nlast_queue_time=0\nlast_dispatch_time=0\n"
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleBusRegister(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char* sNamespace;
	char* sTag;
	char* sTTL;
	char sBody[384];
	int64 iTTL;
	int64 iDataID;
	xvalue objData;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/register") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus register api disabled");
	}

	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sTTL = XS_HttpQueryDup(pReq->sQuery, "ttl");
	iTTL = (sTTL && sTTL[0] != '\0') ? _strtoi64(sTTL, NULL, 10) : 0;

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

	if ( iDataID <= 0 ) {
		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":false,\"message\":\"register failed\",\"bus_code\":%d,\"bus_error\":\"%s\"}",
			(int)XS_BusGetLastErrorCode(),
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
	xrtHttpdResponseSetStatus(pResp, iDataID > 0 ? 200 : 500, iDataID > 0 ? "OK" : "Internal Server Error");
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
	xvalue objData = NULL;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/bus/send") != 0 ) {
		return FALSE;
	}
	if ( !(objServer->Debug || objHost->Debug) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "bus send api disabled");
	}

	sTarget = XS_HttpQueryDup(pReq->sQuery, "target");
	sTopic = XS_HttpQueryDup(pReq->sQuery, "topic");
	sServerName = XS_HttpQueryDup(pReq->sQuery, "server");
	sHostName = XS_HttpQueryDup(pReq->sQuery, "host");
	sNamespace = XS_HttpQueryDup(pReq->sQuery, "namespace");
	sTag = XS_HttpQueryDup(pReq->sQuery, "tag");
	sDataID = XS_HttpQueryDup(pReq->sQuery, "data_id");
	sText = XS_HttpQueryDup(pReq->sQuery, "text");
	sJson = XS_HttpQueryDup(pReq->sQuery, "json");
	sTTL = XS_HttpQueryDup(pReq->sQuery, "ttl");
	bPersist = XS_HttpQueryBool(pReq->sQuery, "persist", FALSE);

	if ( sTTL && sTTL[0] != '\0' ) {
		iTTL = _strtoi64(sTTL, NULL, 10);
	}

	if ( sDataID && sDataID[0] != '\0' ) {
		iDataID = _strtoi64(sDataID, NULL, 10);
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
			if ( iDataID <= 0 ) {
				snprintf(
					sBody,
					sizeof(sBody),
					"{\"result\":false,\"message\":\"auto register failed\",\"bus_code\":%d,\"bus_error\":\"%s\"}",
					(int)XS_BusGetLastErrorCode(),
					XS_BusGetLastError()
				);
				xrtHttpdResponseSetStatus(pResp, 500, "Internal Server Error");
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

		if ( !bSendOk && bAutoRegister && iDataID > 0 ) {
			(void)XS_BusDataRemove(iDataID);
			iDataID = 0;
		} else if ( bSendOk && bAutoRegister && !bPersist && iDataID > 0 ) {
			(void)XS_BusDataRelease(iDataID);
		}

		snprintf(
			sBody,
			sizeof(sBody),
			"{\"result\":%s,\"message\":\"%s\",\"data_id\":%lld,\"target\":\"%s\",\"topic\":\"%s\",\"server\":\"%s\",\"host\":\"%s\",\"auto_register\":%s,\"persist\":%s,\"ttl\":%lld}",
			bSendOk ? "true" : "false",
			bSendOk ? "message queued" : "send failed",
			(long long)iDataID,
			pTarget,
			sTopic,
			(strcmp(pTarget, "broadcast") == 0) ? "" : (pServer ? pServer : ""),
			(strcmp(pTarget, "host") == 0) ? (pHost ? pHost : "") : "",
			bAutoRegister ? "true" : "false",
			bPersist ? "true" : "false",
			(long long)((iTTL >= 0) ? iTTL : 0)
		);
		xrtHttpdResponseSetStatus(pResp, bSendOk ? 200 : 500, bSendOk ? "OK" : "Internal Server Error");
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
	char sBody[3072];
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sWsLastTime;
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
	sWsLastTime = XS_WsLastTimeText();
	iLen = (size_t)snprintf(
		sBody,
		sizeof(sBody),
		"server=%s\nclass=%s\naddr=%s\ndebug=%s\nhost_aware=%s\ndefault_host=%s\npath_limit=%u\nheader_limit=%u\nbody_limit=%u\nrecv_limit=%u\nws_message_limit=%u\nbacklog=%u\nhttp_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_last_method=%s\nhttp_last_status=%lld\nhttp_last_path=%s\nhttp_last_target=%s\nhttp_last_time=%s\nhttp_last_age_ms=%lld\nhttp_last_app_method=%s\nhttp_last_app_status=%lld\nhttp_last_app_path=%s\nhttp_last_app_target=%s\nhttp_last_app_time=%s\nhttp_last_app_age_ms=%lld\n",
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
		(unsigned int)objServer->WsMessageLimit,
		(unsigned int)objServer->Backlog,
		(long long)XS_HttpMetricGet(&g_iXsHttpReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp2xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp3xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp4xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp5xxCount),
		XS_HttpLastMethodName()[0] ? XS_HttpLastMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastStatusCode),
		XS_HttpLastPath()[0] ? XS_HttpLastPath() : "(none)",
		XS_HttpLastTarget()[0] ? XS_HttpLastTarget() : "(none)",
		sHttpLastTime ? sHttpLastTime : "(none)",
		(long long)XS_HttpLastRequestAgeMS(),
		XS_HttpLastAppMethodName()[0] ? XS_HttpLastAppMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode),
		XS_HttpLastAppPath()[0] ? XS_HttpLastAppPath() : "(none)",
		XS_HttpLastAppTarget()[0] ? XS_HttpLastAppTarget() : "(none)",
		sHttpLastAppTime ? sHttpLastAppTime : "(none)",
		(long long)XS_HttpLastAppRequestAgeMS()
	);

	if ( iLen < sizeof(sBody) ) {
		iLen += (size_t)snprintf(
			sBody + iLen,
			sizeof(sBody) - iLen,
			"http_conn_current=%lld\nhttp_conn_peak=%lld\nhttp_get_count=%lld\nhttp_post_count=%lld\nhttp_head_count=%lld\nhttp_other_count=%lld\nws_conn_current=%lld\nws_conn_peak=%lld\nws_open_count=%lld\nws_close_count=%lld\nws_text_count=%lld\nws_binary_count=%lld\nws_ping_count=%lld\nws_pong_count=%lld\nws_error_count=%lld\nws_last_frame_type=%s\nws_last_bytes=%lld\nws_last_text=%s\nws_last_time=%s\nws_last_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpConnCurrent),
			(long long)XS_HttpMetricGet(&g_iXsHttpConnPeak),
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
			XS_WsLastFrameTypeName()[0] ? XS_WsLastFrameTypeName() : "(none)",
			(long long)XS_HttpMetricGet(&g_iXsWsLastBytes),
			g_sXsWsLastText[0] ? g_sXsWsLastText : "(none)",
			sWsLastTime ? sWsLastTime : "(none)",
			(long long)XS_WsLastAgeMS()
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
	if ( sWsLastTime ) {
		xrtFree(sWsLastTime);
	}

	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleStatusJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	xvalue objHosts;
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sWsLastTime;
	char* sWsLastErrorTime;
	char* sXtpLastTime;
	char* sXtpLastInvalidTime;
	char* sXtpLastErrorTime;
	char* sUdpLastTime;
	char* sUdpLastErrorTime;
	char* sCustomLastTime;
	char* sCustomLastErrorTime;
	char* sJson;
	uint32 i;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/status_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "status json api disabled");
	}

	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sWsLastTime = XS_WsLastTimeText();
	sWsLastErrorTime = XS_WsLastErrorTimeText();
	sXtpLastTime = XS_XtpLastTimeText();
	sXtpLastInvalidTime = XS_XtpLastInvalidTimeText();
	sXtpLastErrorTime = XS_XtpLastErrorTimeText();
	sUdpLastTime = XS_UdpLastTimeText();
	sUdpLastErrorTime = XS_UdpLastErrorTimeText();
	sCustomLastTime = XS_CustomLastTimeText();
	sCustomLastErrorTime = XS_CustomLastErrorTimeText();
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
	xvoTableSetInt(objRet, "http_2xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp2xxCount));
	xvoTableSetInt(objRet, "http_3xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp3xxCount));
	xvoTableSetInt(objRet, "http_4xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp4xxCount));
	xvoTableSetInt(objRet, "http_5xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp5xxCount));
	xvoTableSetInt(objRet, "http_conn_current", 17, XS_HttpMetricGet(&g_iXsHttpConnCurrent));
	xvoTableSetInt(objRet, "http_conn_peak", 14, XS_HttpMetricGet(&g_iXsHttpConnPeak));
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
	xvoTableSetText(objRet, "http_last_time", 14, (ptr)(sHttpLastTime ? sHttpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_age_ms", 16, XS_HttpLastRequestAgeMS());
	xvoTableSetInt(objRet, "http_last_app_status", 20, XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode));
	xvoTableSetText(objRet, "http_last_app_method", 20, (ptr)XS_HttpLastAppMethodName(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_path", 18, (ptr)XS_HttpLastAppPath(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_target", 20, (ptr)XS_HttpLastAppTarget(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_time", 18, (ptr)(sHttpLastAppTime ? sHttpLastAppTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_app_age_ms", 20, XS_HttpLastAppRequestAgeMS());
	xvoTableSetInt(objRet, "ws_conn_current", 15, XS_HttpMetricGet(&g_iXsWsConnCurrent));
	xvoTableSetInt(objRet, "ws_conn_peak", 12, XS_HttpMetricGet(&g_iXsWsConnPeak));
	xvoTableSetInt(objRet, "ws_open_count", 13, XS_HttpMetricGet(&g_iXsWsOpenCount));
	xvoTableSetInt(objRet, "ws_close_count", 14, XS_HttpMetricGet(&g_iXsWsCloseCount));
	xvoTableSetInt(objRet, "ws_text_count", 13, XS_HttpMetricGet(&g_iXsWsTextCount));
	xvoTableSetInt(objRet, "ws_binary_count", 15, XS_HttpMetricGet(&g_iXsWsBinaryCount));
	xvoTableSetInt(objRet, "ws_ping_count", 13, XS_HttpMetricGet(&g_iXsWsPingCount));
	xvoTableSetInt(objRet, "ws_pong_count", 13, XS_HttpMetricGet(&g_iXsWsPongCount));
	xvoTableSetInt(objRet, "ws_error_count", 14, XS_HttpMetricGet(&g_iXsWsErrorCount));
	xvoTableSetInt(objRet, "ws_last_error_code", 18, g_iXsWsLastErrorCode);
	xvoTableSetText(objRet, "ws_last_frame_type", 18, (ptr)XS_WsLastFrameTypeName(), 0, FALSE);
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
	xvoTableSetText(objRet, "xtp_last_msg_type", 17, (ptr)XS_XtpLastMsgTypeName(), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_status", 15, XS_HttpMetricGet(&g_iXsXtpLastStatus));
	xvoTableSetInt(objRet, "xtp_last_msg_id", 15, XS_HttpMetricGet(&g_iXsXtpLastMsgID));
	xvoTableSetText(objRet, "xtp_last_cmd", 12, (ptr)g_sXsXtpLastCmd, 0, FALSE);
	xvoTableSetText(objRet, "xtp_last_time", 13, (ptr)(sXtpLastTime ? sXtpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_age_ms", 15, XS_XtpLastAgeMS());
	xvoTableSetText(objRet, "xtp_last_invalid_reason", 23, (ptr)g_sXsXtpLastInvalidReason, 0, FALSE);
	xvoTableSetText(objRet, "xtp_last_invalid_time", 21, (ptr)(sXtpLastInvalidTime ? sXtpLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_invalid_age_ms", 23, XS_XtpLastInvalidAgeMS());
	xvoTableSetInt(objRet, "xtp_last_error_code", 19, g_iXsXtpLastErrorCode);
	xvoTableSetText(objRet, "xtp_last_error_time", 19, (ptr)(sXtpLastErrorTime ? sXtpLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_error_age_ms", 21, XS_XtpLastErrorAgeMS());
	xvoTableSetInt(objRet, "udp_recv_count", 14, XS_HttpMetricGet(&g_iXsUdpRecvCount));
	xvoTableSetInt(objRet, "udp_send_count", 14, XS_HttpMetricGet(&g_iXsUdpSendCount));
	xvoTableSetInt(objRet, "udp_error_count", 15, XS_HttpMetricGet(&g_iXsUdpErrorCount));
	xvoTableSetInt(objRet, "udp_recv_bytes", 14, XS_HttpMetricGet(&g_iXsUdpRecvBytes));
	xvoTableSetInt(objRet, "udp_send_bytes", 14, XS_HttpMetricGet(&g_iXsUdpSendBytes));
	xvoTableSetText(objRet, "udp_last_from", 13, (ptr)g_sXsUdpLastFrom, 0, FALSE);
	xvoTableSetText(objRet, "udp_last_text", 13, (ptr)g_sXsUdpLastText, 0, FALSE);
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
	xvoTableSetInt(objRet, "custom_recv_count", 17, XS_HttpMetricGet(&g_iXsCustomRecvCount));
	xvoTableSetInt(objRet, "custom_send_count", 17, XS_HttpMetricGet(&g_iXsCustomSendCount));
	xvoTableSetInt(objRet, "custom_recv_bytes", 17, XS_HttpMetricGet(&g_iXsCustomRecvBytes));
	xvoTableSetInt(objRet, "custom_send_bytes", 17, XS_HttpMetricGet(&g_iXsCustomSendBytes));
	xvoTableSetText(objRet, "custom_last_text", 16, (ptr)g_sXsCustomLastText, 0, FALSE);
	xvoTableSetText(objRet, "custom_last_time", 16, (ptr)(sCustomLastTime ? sCustomLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_age_ms", 18, XS_CustomLastAgeMS());
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
	if ( sWsLastTime ) {
		xrtFree(sWsLastTime);
	}
	if ( sWsLastErrorTime ) {
		xrtFree(sWsLastErrorTime);
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
	if ( sCustomLastErrorTime ) {
		xrtFree(sCustomLastErrorTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "status json build failed");
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
	xvalue objRet;
	xvalue objBus;
	char* sCheckTime;
	char* sHttpLastTime;
	char* sHttpLastAppTime;
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
		return XS_HttpRespondText(pResp, 403, "Forbidden", "health json api disabled");
	}

	sReloadTime = XS_ReloadTimeText();
	sCheckTime = XS_CheckConfigLastTimeText();
	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
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
	xvoTableSetBool(objRet, "reload_busy", 11, XS_ConfigReloadStatusBusy());
	xvoTableSetBool(objRet, "reload_has_result", 17, XS_ConfigReloadStatusHasResult());
	xvoTableSetBool(objRet, "reload_success", 14, XS_ConfigReloadStatusSuccess());
	xvoTableSetText(objRet, "reload_server", 13, (ptr)XS_ConfigReloadStatusServer(), 0, FALSE);
	xvoTableSetText(objRet, "reload_host", 11, (ptr)XS_ConfigReloadStatusHost(), 0, FALSE);
	xvoTableSetText(objRet, "reload_message", 14, (ptr)XS_ConfigReloadStatusMessage(), 0, FALSE);
	xvoTableSetText(objRet, "reload_time", 11, (ptr)(sReloadTime ? sReloadTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "reload_age_ms", 13, XS_ReloadAgeMS());
	xvoTableSetInt(objRet, "reload_total_count", 18, XS_ConfigReloadStatusTotalCount());
	xvoTableSetInt(objRet, "reload_success_count", 20, XS_ConfigReloadStatusSuccessCount());
	xvoTableSetInt(objRet, "reload_failure_count", 20, XS_ConfigReloadStatusFailureCount());
	xvoTableSetInt(objRet, "check_total_count", 17, XS_HttpMetricGet(&g_iXsCheckConfigTotalCount));
	xvoTableSetInt(objRet, "check_success_count", 19, XS_HttpMetricGet(&g_iXsCheckConfigSuccessCount));
	xvoTableSetInt(objRet, "check_failure_count", 19, XS_HttpMetricGet(&g_iXsCheckConfigFailureCount));
	xvoTableSetText(objRet, "check_last_time", 15, (ptr)(sCheckTime ? sCheckTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "check_last_age_ms", 17, XS_CheckConfigLastAgeMS());
	xvoTableSetInt(objRet, "http_req_count", 14, XS_HttpMetricGet(&g_iXsHttpReqCount));
	xvoTableSetInt(objRet, "http_2xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp2xxCount));
	xvoTableSetInt(objRet, "http_3xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp3xxCount));
	xvoTableSetInt(objRet, "http_4xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp4xxCount));
	xvoTableSetInt(objRet, "http_5xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp5xxCount));
	xvoTableSetInt(objRet, "http_conn_current", 17, XS_HttpMetricGet(&g_iXsHttpConnCurrent));
	xvoTableSetInt(objRet, "http_conn_peak", 14, XS_HttpMetricGet(&g_iXsHttpConnPeak));
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
	xvoTableSetText(objRet, "http_last_time", 14, (ptr)(sHttpLastTime ? sHttpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_age_ms", 16, XS_HttpLastRequestAgeMS());
	xvoTableSetInt(objRet, "http_last_app_status", 20, XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode));
	xvoTableSetText(objRet, "http_last_app_method", 20, (ptr)XS_HttpLastAppMethodName(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_path", 18, (ptr)XS_HttpLastAppPath(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_target", 20, (ptr)XS_HttpLastAppTarget(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_time", 18, (ptr)(sHttpLastAppTime ? sHttpLastAppTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_app_age_ms", 20, XS_HttpLastAppRequestAgeMS());

	objBus = XS_BusBuildStatusValue();
	if ( objBus ) {
		xvoTableSetInt(objRet, "bus_queue_count", 15, xvoTableGetInt(objBus, "queue_count", 11));
		xvoTableSetInt(objRet, "bus_data_count", 14, xvoTableGetInt(objBus, "data_count", 10));
		xvoTableSetInt(objRet, "bus_total_queued", 16, xvoTableGetInt(objBus, "total_queued", 12));
		xvoTableSetInt(objRet, "bus_total_delivered", 19, xvoTableGetInt(objBus, "total_delivered", 15));
		xvoTableSetInt(objRet, "bus_total_dropped", 17, xvoTableGetInt(objBus, "total_dropped", 13));
		xvoTableSetInt(objRet, "bus_last_queue_time", 19, xvoTableGetInt(objBus, "last_queue_time", 15));
		xvoTableSetText(objRet, "bus_last_queue_time_text", 24, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_queue_time_text", 20)), 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_dispatch_time", 22, xvoTableGetInt(objBus, "last_dispatch_time", 18));
		xvoTableSetText(objRet, "bus_last_dispatch_time_text", 27, (ptr)xvoGetText(xvoTableGetValue(objBus, "last_dispatch_time_text", 23)), 0, FALSE);
		xvoUnref(objBus);
	} else {
		xvoTableSetInt(objRet, "bus_queue_count", 15, 0);
		xvoTableSetInt(objRet, "bus_data_count", 14, 0);
		xvoTableSetInt(objRet, "bus_total_queued", 16, 0);
		xvoTableSetInt(objRet, "bus_total_delivered", 19, 0);
		xvoTableSetInt(objRet, "bus_total_dropped", 17, 0);
		xvoTableSetInt(objRet, "bus_last_queue_time", 19, 0);
		xvoTableSetText(objRet, "bus_last_queue_time_text", 24, "", 0, FALSE);
		xvoTableSetInt(objRet, "bus_last_dispatch_time", 22, 0);
		xvoTableSetText(objRet, "bus_last_dispatch_time_text", 27, "", 0, FALSE);
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
	if ( sJson == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "health json build failed");
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
	char sBody[1536];
	char* sHttpLastTime;
	char* sHttpLastAppTime;

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
	snprintf(
		sBody,
		sizeof(sBody),
		"http_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_conn_current=%lld\nhttp_conn_peak=%lld\nhttp_get_count=%lld\nhttp_post_count=%lld\nhttp_head_count=%lld\nhttp_other_count=%lld\nhttp_time_total_ms=%lld\nhttp_time_max_ms=%lld\nhttp_time_avg_ms=%lld\nhttp_last_method=%s\nhttp_last_status=%lld\nhttp_last_path=%s\nhttp_last_target=%s\nhttp_last_time=%s\nhttp_last_age_ms=%lld\nhttp_last_app_method=%s\nhttp_last_app_status=%lld\nhttp_last_app_path=%s\nhttp_last_app_target=%s\nhttp_last_app_time=%s\nhttp_last_app_age_ms=%lld\n",
		(long long)XS_HttpMetricGet(&g_iXsHttpReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp2xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp3xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp4xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp5xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnPeak),
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
		sHttpLastTime ? sHttpLastTime : "(none)",
		(long long)XS_HttpLastRequestAgeMS(),
		XS_HttpLastAppMethodName()[0] ? XS_HttpLastAppMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode),
		XS_HttpLastAppPath()[0] ? XS_HttpLastAppPath() : "(none)",
		XS_HttpLastAppTarget()[0] ? XS_HttpLastAppTarget() : "(none)",
		sHttpLastAppTime ? sHttpLastAppTime : "(none)",
		(long long)XS_HttpLastAppRequestAgeMS()
	);
	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleMetricsJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/http_metrics_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "http metrics json api disabled");
	}

	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	objRet = xvoCreateTable();
	xvoTableSetInt(objRet, "http_req_count", 14, XS_HttpMetricGet(&g_iXsHttpReqCount));
	xvoTableSetInt(objRet, "http_2xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp2xxCount));
	xvoTableSetInt(objRet, "http_3xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp3xxCount));
	xvoTableSetInt(objRet, "http_4xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp4xxCount));
	xvoTableSetInt(objRet, "http_5xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp5xxCount));
	xvoTableSetInt(objRet, "http_conn_current", 17, XS_HttpMetricGet(&g_iXsHttpConnCurrent));
	xvoTableSetInt(objRet, "http_conn_peak", 14, XS_HttpMetricGet(&g_iXsHttpConnPeak));
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
	xvoTableSetText(objRet, "http_last_time", 14, (ptr)(sHttpLastTime ? sHttpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_age_ms", 16, XS_HttpLastRequestAgeMS());
	xvoTableSetInt(objRet, "http_last_app_status", 20, XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode));
	xvoTableSetText(objRet, "http_last_app_method", 20, (ptr)XS_HttpLastAppMethodName(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_path", 18, (ptr)XS_HttpLastAppPath(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_target", 20, (ptr)XS_HttpLastAppTarget(), 0, FALSE);
	xvoTableSetText(objRet, "http_last_app_time", 18, (ptr)(sHttpLastAppTime ? sHttpLastAppTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "http_last_app_age_ms", 20, XS_HttpLastAppRequestAgeMS());

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "http metrics json build failed");
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
	char sBody[768];

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
		"http_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_conn_current=%lld\nhttp_conn_peak=%lld\nhttp_get_count=%lld\nhttp_post_count=%lld\nhttp_head_count=%lld\nhttp_other_count=%lld\nhttp_time_total_ms=%lld\nhttp_time_max_ms=%lld\nhttp_time_avg_ms=%lld\nhttp_last_method=\nhttp_last_status=%lld\nhttp_last_path=(none)\nhttp_last_target=(none)\nhttp_last_time=(none)\nhttp_last_age_ms=-1\nhttp_last_app_method=\nhttp_last_app_status=0\nhttp_last_app_path=(none)\nhttp_last_app_target=(none)\nhttp_last_app_time=(none)\nhttp_last_app_age_ms=-1\n",
		(long long)XS_HttpMetricGet(&g_iXsHttpReqCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp2xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp3xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp4xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp5xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsHttpConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodGetCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodPostCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodHeadCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpMethodOtherCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpTimeTotalMS),
		(long long)XS_HttpMetricGet(&g_iXsHttpTimeMaxMS),
		(long long)((XS_HttpMetricGet(&g_iXsHttpReqCount) > 0) ? (XS_HttpMetricGet(&g_iXsHttpTimeTotalMS) / XS_HttpMetricGet(&g_iXsHttpReqCount)) : 0),
		(long long)XS_HttpMetricGet(&g_iXsHttpLastStatusCode)
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleWsMetrics(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[768];
	char* sLastTime;
	char* sLastErrorTime;

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
	sLastErrorTime = XS_WsLastErrorTimeText();
	snprintf(
		sBody,
		sizeof(sBody),
		"ws_message_limit=%u\nws_conn_current=%lld\nws_conn_peak=%lld\nws_open_count=%lld\nws_close_count=%lld\nws_text_count=%lld\nws_binary_count=%lld\nws_ping_count=%lld\nws_pong_count=%lld\nws_error_count=%lld\nws_last_error_code=%lld\nws_last_frame_type=%s\nws_last_bytes=%lld\nws_last_text=%s\nws_last_time=%s\nws_last_age_ms=%lld\nws_last_error_time=%s\nws_last_error_age_ms=%lld\n",
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
		(long long)g_iXsWsLastErrorCode,
		XS_WsLastFrameTypeName()[0] ? XS_WsLastFrameTypeName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsWsLastBytes),
		g_sXsWsLastText[0] ? g_sXsWsLastText : "(none)",
		sLastTime ? sLastTime : "(none)",
		(long long)XS_WsLastAgeMS(),
		sLastErrorTime ? sLastErrorTime : "(none)",
		(long long)XS_WsLastErrorAgeMS()
	);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleWsMetricsJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sLastTime;
	char* sLastErrorTime;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/ws_metrics_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "ws metrics json api disabled");
	}

	sLastTime = XS_WsLastTimeText();
	sLastErrorTime = XS_WsLastErrorTimeText();
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
	xvoTableSetInt(objRet, "ws_last_error_code", 18, g_iXsWsLastErrorCode);
	xvoTableSetText(objRet, "ws_last_frame_type", 18, (ptr)XS_WsLastFrameTypeName(), 0, FALSE);
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
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "ws metrics json build failed");
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
	char sBody[512];

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
		"ws_message_limit=%u\nws_conn_current=%lld\nws_conn_peak=%lld\nws_open_count=%lld\nws_close_count=%lld\nws_text_count=%lld\nws_binary_count=%lld\nws_ping_count=%lld\nws_pong_count=%lld\nws_error_count=%lld\nws_last_error_code=0\nws_last_frame_type=(none)\nws_last_bytes=0\nws_last_text=(none)\nws_last_time=(none)\nws_last_age_ms=-1\nws_last_error_time=(none)\nws_last_error_age_ms=-1\n",
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
	char sBody[1024];
	char* sLastTime;
	char* sLastInvalidTime;
	char* sLastErrorTime;

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
	snprintf(
		sBody,
		sizeof(sBody),
		"xtp_conn_current=%lld\nxtp_conn_peak=%lld\nxtp_open_count=%lld\nxtp_close_count=%lld\nxtp_error_count=%lld\nxtp_invalid_count=%lld\nxtp_msg_count=%lld\nxtp_req_count=%lld\nxtp_resp_count=%lld\nxtp_push_count=%lld\nxtp_event_count=%lld\nxtp_send_count=%lld\nxtp_recv_bytes=%lld\nxtp_send_bytes=%lld\nxtp_last_msg_type=%s\nxtp_last_status=%lld\nxtp_last_msg_id=%lld\nxtp_last_cmd=%s\nxtp_last_time=%s\nxtp_last_age_ms=%lld\nxtp_last_invalid_reason=%s\nxtp_last_invalid_time=%s\nxtp_last_invalid_age_ms=%lld\nxtp_last_error_code=%lld\nxtp_last_error_time=%s\nxtp_last_error_age_ms=%lld\n",
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
		g_sXsXtpLastCmd[0] ? g_sXsXtpLastCmd : "(none)",
		sLastTime ? sLastTime : "(none)",
		(long long)XS_XtpLastAgeMS(),
		g_sXsXtpLastInvalidReason[0] ? g_sXsXtpLastInvalidReason : "(none)",
		sLastInvalidTime ? sLastInvalidTime : "(none)",
		(long long)XS_XtpLastInvalidAgeMS(),
		(long long)g_iXsXtpLastErrorCode,
		sLastErrorTime ? sLastErrorTime : "(none)",
		(long long)XS_XtpLastErrorAgeMS()
	);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastInvalidTime ) {
		xrtFree(sLastInvalidTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleXtpMetricsJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sLastTime;
	char* sLastInvalidTime;
	char* sLastErrorTime;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/xtp_metrics_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "xtp metrics json api disabled");
	}

	sLastTime = XS_XtpLastTimeText();
	sLastInvalidTime = XS_XtpLastInvalidTimeText();
	sLastErrorTime = XS_XtpLastErrorTimeText();
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
	xvoTableSetText(objRet, "xtp_last_msg_type", 17, (ptr)XS_XtpLastMsgTypeName(), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_status", 15, XS_HttpMetricGet(&g_iXsXtpLastStatus));
	xvoTableSetInt(objRet, "xtp_last_msg_id", 15, XS_HttpMetricGet(&g_iXsXtpLastMsgID));
	xvoTableSetText(objRet, "xtp_last_cmd", 12, (ptr)g_sXsXtpLastCmd, 0, FALSE);
	xvoTableSetText(objRet, "xtp_last_time", 13, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_age_ms", 15, XS_XtpLastAgeMS());
	xvoTableSetText(objRet, "xtp_last_invalid_reason", 23, (ptr)g_sXsXtpLastInvalidReason, 0, FALSE);
	xvoTableSetText(objRet, "xtp_last_invalid_time", 21, (ptr)(sLastInvalidTime ? sLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_invalid_age_ms", 23, XS_XtpLastInvalidAgeMS());
	xvoTableSetInt(objRet, "xtp_last_error_code", 19, g_iXsXtpLastErrorCode);
	xvoTableSetText(objRet, "xtp_last_error_time", 19, (ptr)(sLastErrorTime ? sLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_error_age_ms", 21, XS_XtpLastErrorAgeMS());

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
	if ( sJson == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "xtp metrics json build failed");
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
	char sBody[1024];

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
		"xtp_conn_current=0\nxtp_conn_peak=0\nxtp_open_count=0\nxtp_close_count=0\nxtp_error_count=0\nxtp_invalid_count=0\nxtp_msg_count=0\nxtp_req_count=0\nxtp_resp_count=0\nxtp_push_count=0\nxtp_event_count=0\nxtp_send_count=0\nxtp_recv_bytes=0\nxtp_send_bytes=0\nxtp_last_msg_type=(none)\nxtp_last_status=0\nxtp_last_msg_id=0\nxtp_last_cmd=(none)\nxtp_last_time=(none)\nxtp_last_age_ms=-1\nxtp_last_invalid_reason=(none)\nxtp_last_invalid_time=(none)\nxtp_last_invalid_age_ms=-1\nxtp_last_error_code=0\nxtp_last_error_time=(none)\nxtp_last_error_age_ms=-1\n"
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
		"udp_recv_count=%lld\nudp_send_count=%lld\nudp_error_count=%lld\nudp_last_error_code=%lld\nudp_recv_bytes=%lld\nudp_send_bytes=%lld\nudp_last_from=%s\nudp_last_text=%s\nudp_last_time=%s\nudp_last_age_ms=%lld\nudp_last_error_time=%s\nudp_last_error_age_ms=%lld\n",
		(long long)XS_HttpMetricGet(&g_iXsUdpRecvCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpSendCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpErrorCount),
		(long long)g_iXsUdpLastErrorCode,
		(long long)XS_HttpMetricGet(&g_iXsUdpRecvBytes),
		(long long)XS_HttpMetricGet(&g_iXsUdpSendBytes),
		g_sXsUdpLastFrom[0] ? g_sXsUdpLastFrom : "(none)",
		g_sXsUdpLastText[0] ? g_sXsUdpLastText : "(none)",
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
		return XS_HttpRespondText(pResp, 403, "Forbidden", "udp metrics json api disabled");
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
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "udp metrics json build failed");
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
		"udp_recv_count=0\nudp_send_count=0\nudp_error_count=0\nudp_last_error_code=0\nudp_recv_bytes=0\nudp_send_bytes=0\nudp_last_from=(none)\nudp_last_text=(none)\nudp_last_time=(none)\nudp_last_age_ms=-1\nudp_last_error_time=(none)\nudp_last_error_age_ms=-1\n"
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleCustomMetrics(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[768];
	char* sLastTime;
	char* sLastErrorTime;

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
	sLastErrorTime = XS_CustomLastErrorTimeText();
	snprintf(
		sBody,
		sizeof(sBody),
		"custom_conn_current=%lld\ncustom_conn_peak=%lld\ncustom_open_count=%lld\ncustom_close_count=%lld\ncustom_error_count=%lld\ncustom_invalid_count=%lld\ncustom_last_close_reason=%lld\ncustom_last_error_code=%lld\ncustom_last_error_time=%s\ncustom_last_error_age_ms=%lld\ncustom_recv_count=%lld\ncustom_send_count=%lld\ncustom_recv_bytes=%lld\ncustom_send_bytes=%lld\ncustom_last_text=%s\ncustom_last_time=%s\ncustom_last_age_ms=%lld\n",
		(long long)XS_HttpMetricGet(&g_iXsCustomConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsCustomConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsCustomOpenCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomErrorCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomInvalidCount),
		(long long)g_iXsCustomLastCloseReason,
		(long long)g_iXsCustomLastErrorCode,
		sLastErrorTime ? sLastErrorTime : "(none)",
		(long long)XS_CustomLastErrorAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsCustomRecvCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomSendCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomRecvBytes),
		(long long)XS_HttpMetricGet(&g_iXsCustomSendBytes),
		g_sXsCustomLastText[0] ? g_sXsCustomLastText : "(none)",
		sLastTime ? sLastTime : "(none)",
		(long long)XS_CustomLastAgeMS()
	);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleCustomMetricsJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sLastTime;
	char* sLastErrorTime;
	char* sJson;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/custom_metrics_json") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "custom metrics json api disabled");
	}

	sLastTime = XS_CustomLastTimeText();
	sLastErrorTime = XS_CustomLastErrorTimeText();
	objRet = xvoCreateTable();
	xvoTableSetInt(objRet, "custom_conn_current", 19, XS_HttpMetricGet(&g_iXsCustomConnCurrent));
	xvoTableSetInt(objRet, "custom_conn_peak", 16, XS_HttpMetricGet(&g_iXsCustomConnPeak));
	xvoTableSetInt(objRet, "custom_open_count", 17, XS_HttpMetricGet(&g_iXsCustomOpenCount));
	xvoTableSetInt(objRet, "custom_close_count", 18, XS_HttpMetricGet(&g_iXsCustomCloseCount));
	xvoTableSetInt(objRet, "custom_error_count", 18, XS_HttpMetricGet(&g_iXsCustomErrorCount));
	xvoTableSetInt(objRet, "custom_invalid_count", 20, XS_HttpMetricGet(&g_iXsCustomInvalidCount));
	xvoTableSetInt(objRet, "custom_last_close_reason", 24, g_iXsCustomLastCloseReason);
	xvoTableSetInt(objRet, "custom_last_error_code", 22, g_iXsCustomLastErrorCode);
	xvoTableSetText(objRet, "custom_last_error_time", 22, (ptr)(sLastErrorTime ? sLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_error_age_ms", 24, XS_CustomLastErrorAgeMS());
	xvoTableSetInt(objRet, "custom_recv_count", 17, XS_HttpMetricGet(&g_iXsCustomRecvCount));
	xvoTableSetInt(objRet, "custom_send_count", 17, XS_HttpMetricGet(&g_iXsCustomSendCount));
	xvoTableSetInt(objRet, "custom_recv_bytes", 17, XS_HttpMetricGet(&g_iXsCustomRecvBytes));
	xvoTableSetInt(objRet, "custom_send_bytes", 17, XS_HttpMetricGet(&g_iXsCustomSendBytes));
	xvoTableSetText(objRet, "custom_last_text", 16, (ptr)g_sXsCustomLastText, 0, FALSE);
	xvoTableSetText(objRet, "custom_last_time", 16, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_age_ms", 18, XS_CustomLastAgeMS());

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "custom metrics json build failed");
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
	char sBody[512];

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
		"custom_conn_current=0\ncustom_conn_peak=0\ncustom_open_count=0\ncustom_close_count=0\ncustom_error_count=0\ncustom_invalid_count=0\ncustom_last_close_reason=0\ncustom_last_error_code=0\ncustom_last_error_time=(none)\ncustom_last_error_age_ms=-1\ncustom_recv_count=0\ncustom_send_count=0\ncustom_recv_bytes=0\ncustom_send_bytes=0\ncustom_last_text=(none)\ncustom_last_time=(none)\ncustom_last_age_ms=-1\n"
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleDashboardJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
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
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sWsLastTime;
	char* sWsLastErrorTime;
	char* sXtpLastTime;
	char* sXtpLastInvalidTime;
	char* sUdpLastTime;
	char* sUdpLastErrorTime;
	char* sCustomLastTime;
	char* sCustomLastErrorTime;
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
		return XS_HttpRespondText(pResp, 403, "Forbidden", "dashboard json api disabled");
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
	sReloadTime = XS_ReloadTimeText();
	sCheckTime = XS_CheckConfigLastTimeText();
	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sWsLastTime = XS_WsLastTimeText();
	sWsLastErrorTime = XS_WsLastErrorTimeText();
	sXtpLastTime = XS_XtpLastTimeText();
	sXtpLastInvalidTime = XS_XtpLastInvalidTimeText();
	sUdpLastTime = XS_UdpLastTimeText();
	sUdpLastErrorTime = XS_UdpLastErrorTimeText();
	sCustomLastTime = XS_CustomLastTimeText();
	sCustomLastErrorTime = XS_CustomLastErrorTimeText();
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
	xvoTableSetInt(objStatus, "ws_conn_current", 15, XS_HttpMetricGet(&g_iXsWsConnCurrent));
	xvoTableSetInt(objStatus, "ws_conn_peak", 12, XS_HttpMetricGet(&g_iXsWsConnPeak));
	xvoTableSetInt(objStatus, "ws_open_count", 13, XS_HttpMetricGet(&g_iXsWsOpenCount));
	xvoTableSetInt(objStatus, "ws_close_count", 14, XS_HttpMetricGet(&g_iXsWsCloseCount));
	xvoTableSetInt(objStatus, "ws_text_count", 13, XS_HttpMetricGet(&g_iXsWsTextCount));
	xvoTableSetInt(objStatus, "ws_binary_count", 15, XS_HttpMetricGet(&g_iXsWsBinaryCount));
	xvoTableSetInt(objStatus, "ws_ping_count", 13, XS_HttpMetricGet(&g_iXsWsPingCount));
	xvoTableSetInt(objStatus, "ws_pong_count", 13, XS_HttpMetricGet(&g_iXsWsPongCount));
	xvoTableSetInt(objStatus, "ws_error_count", 14, XS_HttpMetricGet(&g_iXsWsErrorCount));
	xvoTableSetInt(objStatus, "ws_last_error_code", 18, g_iXsWsLastErrorCode);
	xvoTableSetText(objStatus, "ws_last_frame_type", 18, (ptr)XS_WsLastFrameTypeName(), 0, FALSE);
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
	xvoTableSetText(objStatus, "xtp_last_msg_type", 17, (ptr)XS_XtpLastMsgTypeName(), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_status", 15, XS_HttpMetricGet(&g_iXsXtpLastStatus));
	xvoTableSetInt(objStatus, "xtp_last_msg_id", 15, XS_HttpMetricGet(&g_iXsXtpLastMsgID));
	xvoTableSetText(objStatus, "xtp_last_cmd", 12, (ptr)g_sXsXtpLastCmd, 0, FALSE);
	xvoTableSetText(objStatus, "xtp_last_time", 13, (ptr)(sXtpLastTime ? sXtpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_age_ms", 15, XS_XtpLastAgeMS());
	xvoTableSetText(objStatus, "xtp_last_invalid_reason", 23, (ptr)g_sXsXtpLastInvalidReason, 0, FALSE);
	xvoTableSetText(objStatus, "xtp_last_invalid_time", 21, (ptr)(sXtpLastInvalidTime ? sXtpLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_invalid_age_ms", 23, XS_XtpLastInvalidAgeMS());
	xvoTableSetInt(objStatus, "udp_recv_count", 14, XS_HttpMetricGet(&g_iXsUdpRecvCount));
	xvoTableSetInt(objStatus, "udp_send_count", 14, XS_HttpMetricGet(&g_iXsUdpSendCount));
	xvoTableSetInt(objStatus, "udp_error_count", 15, XS_HttpMetricGet(&g_iXsUdpErrorCount));
	xvoTableSetInt(objStatus, "udp_recv_bytes", 14, XS_HttpMetricGet(&g_iXsUdpRecvBytes));
	xvoTableSetInt(objStatus, "udp_send_bytes", 14, XS_HttpMetricGet(&g_iXsUdpSendBytes));
	xvoTableSetText(objStatus, "udp_last_from", 13, (ptr)g_sXsUdpLastFrom, 0, FALSE);
	xvoTableSetText(objStatus, "udp_last_text", 13, (ptr)g_sXsUdpLastText, 0, FALSE);
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
	xvoTableSetInt(objStatus, "custom_last_close_reason", 24, g_iXsCustomLastCloseReason);
	xvoTableSetInt(objStatus, "custom_last_error_code", 22, g_iXsCustomLastErrorCode);
	xvoTableSetText(objStatus, "custom_last_error_time", 22, (ptr)(sCustomLastErrorTime ? sCustomLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "custom_last_error_age_ms", 24, XS_CustomLastErrorAgeMS());
	xvoTableSetInt(objStatus, "custom_recv_count", 17, XS_HttpMetricGet(&g_iXsCustomRecvCount));
	xvoTableSetInt(objStatus, "custom_send_count", 17, XS_HttpMetricGet(&g_iXsCustomSendCount));
	xvoTableSetInt(objStatus, "custom_recv_bytes", 17, XS_HttpMetricGet(&g_iXsCustomRecvBytes));
	xvoTableSetInt(objStatus, "custom_send_bytes", 17, XS_HttpMetricGet(&g_iXsCustomSendBytes));
	xvoTableSetText(objStatus, "custom_last_text", 16, (ptr)g_sXsCustomLastText, 0, FALSE);
	xvoTableSetText(objStatus, "custom_last_time", 16, (ptr)(sCustomLastTime ? sCustomLastTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "custom_last_age_ms", 18, XS_CustomLastAgeMS());
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
	xvoTableSetInt(objStatus, "http_2xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp2xxCount));
	xvoTableSetInt(objStatus, "http_3xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp3xxCount));
	xvoTableSetInt(objStatus, "http_4xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp4xxCount));
	xvoTableSetInt(objStatus, "http_5xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp5xxCount));
	xvoTableSetInt(objStatus, "http_conn_current", 17, XS_HttpMetricGet(&g_iXsHttpConnCurrent));
	xvoTableSetInt(objStatus, "http_conn_peak", 14, XS_HttpMetricGet(&g_iXsHttpConnPeak));
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
	xvoTableSetText(objStatus, "http_last_time", 14, (ptr)(sHttpLastTime ? sHttpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "http_last_age_ms", 16, XS_HttpLastRequestAgeMS());
	xvoTableSetInt(objStatus, "http_last_app_status", 20, XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode));
	xvoTableSetText(objStatus, "http_last_app_method", 20, (ptr)XS_HttpLastAppMethodName(), 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_path", 18, (ptr)XS_HttpLastAppPath(), 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_target", 20, (ptr)XS_HttpLastAppTarget(), 0, FALSE);
	xvoTableSetText(objStatus, "http_last_app_time", 18, (ptr)(sHttpLastAppTime ? sHttpLastAppTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "http_last_app_age_ms", 20, XS_HttpLastAppRequestAgeMS());
	xvoTableSetBool(objStatus, "manage_api", 10, XS_ManageAPIEnabled(objServer, objHost));
	xvoTableSetBool(objStatus, "debug", 5, objServer->Debug);
	xvoTableSetBool(objStatus, "host_aware", 10, objServer->HostAware);
	xvoTableSetBool(objStatus, "default_host", 12, objServer->EnableDefaultHost);
	xvoTableSetInt(objStatus, "host_count", 10, objServer->Hosts ? objServer->Hosts->Count : 0);
	xvoTableSetBool(objStatus, "script_loaded", 13, objServer->EnableDefaultHost ? (objServer->DefaultHost.pScriptState != NULL) : FALSE);

	objHealth = xvoCreateTable();
	xvoTableSetBool(objHealth, "ok", 2, !XS_ConfigReloadStatusBusy());
	xvoTableSetBool(objHealth, "reload_busy", 11, XS_ConfigReloadStatusBusy());
	xvoTableSetBool(objHealth, "reload_has_result", 17, XS_ConfigReloadStatusHasResult());
	xvoTableSetBool(objHealth, "reload_success", 14, XS_ConfigReloadStatusSuccess());
	xvoTableSetText(objHealth, "reload_message", 14, (ptr)XS_ConfigReloadStatusMessage(), 0, FALSE);
	xvoTableSetText(objHealth, "reload_time", 11, (ptr)(sReloadTime ? sReloadTime : ""), 0, FALSE);
	xvoTableSetInt(objHealth, "reload_age_ms", 13, XS_ReloadAgeMS());
	xvoTableSetInt(objHealth, "reload_total_count", 18, XS_ConfigReloadStatusTotalCount());
	xvoTableSetInt(objHealth, "reload_success_count", 20, XS_ConfigReloadStatusSuccessCount());
	xvoTableSetInt(objHealth, "reload_failure_count", 20, XS_ConfigReloadStatusFailureCount());
	xvoTableSetInt(objHealth, "check_total_count", 17, XS_HttpMetricGet(&g_iXsCheckConfigTotalCount));
	xvoTableSetInt(objHealth, "check_success_count", 19, XS_HttpMetricGet(&g_iXsCheckConfigSuccessCount));
	xvoTableSetInt(objHealth, "check_failure_count", 19, XS_HttpMetricGet(&g_iXsCheckConfigFailureCount));
	xvoTableSetText(objHealth, "check_last_time", 15, (ptr)(sCheckTime ? sCheckTime : ""), 0, FALSE);
	xvoTableSetInt(objHealth, "check_last_age_ms", 17, XS_CheckConfigLastAgeMS());
	xvoTableSetInt(objHealth, "http_req_count", 14, XS_HttpMetricGet(&g_iXsHttpReqCount));
	xvoTableSetInt(objHealth, "http_2xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp2xxCount));
	xvoTableSetInt(objHealth, "http_3xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp3xxCount));
	xvoTableSetInt(objHealth, "http_4xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp4xxCount));
	xvoTableSetInt(objHealth, "http_5xx_count", 14, XS_HttpMetricGet(&g_iXsHttpResp5xxCount));
	xvoTableSetInt(objHealth, "http_conn_current", 17, XS_HttpMetricGet(&g_iXsHttpConnCurrent));
	xvoTableSetInt(objHealth, "http_conn_peak", 14, XS_HttpMetricGet(&g_iXsHttpConnPeak));
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
	xvoTableSetText(objHealth, "http_last_time", 14, (ptr)(sHttpLastTime ? sHttpLastTime : ""), 0, FALSE);
	xvoTableSetInt(objHealth, "http_last_age_ms", 16, XS_HttpLastRequestAgeMS());
	xvoTableSetInt(objHealth, "http_last_app_status", 20, XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode));
	xvoTableSetText(objHealth, "http_last_app_method", 20, (ptr)XS_HttpLastAppMethodName(), 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_path", 18, (ptr)XS_HttpLastAppPath(), 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_target", 20, (ptr)XS_HttpLastAppTarget(), 0, FALSE);
	xvoTableSetText(objHealth, "http_last_app_time", 18, (ptr)(sHttpLastAppTime ? sHttpLastAppTime : ""), 0, FALSE);
	xvoTableSetInt(objHealth, "http_last_app_age_ms", 20, XS_HttpLastAppRequestAgeMS());

	objReload = xvoCreateTable();
	xvoTableSetBool(objReload, "busy", 4, XS_ConfigReloadStatusBusy());
	xvoTableSetBool(objReload, "has_result", 10, XS_ConfigReloadStatusHasResult());
	xvoTableSetBool(objReload, "success", 7, XS_ConfigReloadStatusSuccess());
	xvoTableSetText(objReload, "server", 6, (ptr)XS_ConfigReloadStatusServer(), 0, FALSE);
	xvoTableSetText(objReload, "host", 4, (ptr)XS_ConfigReloadStatusHost(), 0, FALSE);
	xvoTableSetText(objReload, "message", 7, (ptr)XS_ConfigReloadStatusMessage(), 0, FALSE);
	xvoTableSetText(objReload, "reload_time", 11, (ptr)(sReloadTime ? sReloadTime : ""), 0, FALSE);
	xvoTableSetInt(objReload, "reload_age_ms", 13, XS_ReloadAgeMS());
	xvoTableSetInt(objReload, "reload_total_count", 18, XS_ConfigReloadStatusTotalCount());
	xvoTableSetInt(objReload, "reload_success_count", 20, XS_ConfigReloadStatusSuccessCount());
	xvoTableSetInt(objReload, "reload_failure_count", 20, XS_ConfigReloadStatusFailureCount());

	objCheck = xvoCreateTable();
	xvoTableSetBool(objCheck, "result", 6, bConfigOK);
	xvoTableSetText(objCheck, "file", 4, (ptr)(g_sXsConfigFile ? g_sXsConfigFile : ""), 0, FALSE);
	xvoTableSetText(objCheck, "base", 4, (ptr)(sCheckBase ? sCheckBase : ""), 0, FALSE);
	xvoTableSetInt(objCheck, "server_count", 12, (bConfigOK && objCfgCheck.Servers) ? objCfgCheck.Servers->Count : 0);
	xvoTableSetText(objCheck, "message", 7, bConfigOK ? "config check passed" : "config check failed", 0, FALSE);
	xvoTableSetInt(objCheck, "check_total_count", 17, XS_HttpMetricGet(&g_iXsCheckConfigTotalCount));
	xvoTableSetInt(objCheck, "check_success_count", 19, XS_HttpMetricGet(&g_iXsCheckConfigSuccessCount));
	xvoTableSetInt(objCheck, "check_failure_count", 19, XS_HttpMetricGet(&g_iXsCheckConfigFailureCount));
	xvoTableSetText(objCheck, "check_last_time", 15, (ptr)(sCheckTime ? sCheckTime : ""), 0, FALSE);
	xvoTableSetInt(objCheck, "check_last_age_ms", 17, XS_CheckConfigLastAgeMS());

	objBus = XS_BusBuildStatusValue();
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
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "dashboard bus build failed");
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
	if ( sWsLastTime ) {
		xrtFree(sWsLastTime);
	}
	if ( sWsLastErrorTime ) {
		xrtFree(sWsLastErrorTime);
	}
	if ( sXtpLastTime ) {
		xrtFree(sXtpLastTime);
	}
	if ( sXtpLastInvalidTime ) {
		xrtFree(sXtpLastInvalidTime);
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
	if ( sJson == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "dashboard json build failed");
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
	char sBody[3072];
	char* sCheckTime;
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sReloadTime;
	char* sBusQueueTime;
	char* sBusDispatchTime;
	xvalue objBus;
	int64 iBusQueueCount;
	int64 iBusDataCount;
	int64 iBusTotalQueued;
	int64 iBusTotalDelivered;
	int64 iBusTotalDropped;
	int64 tBusLastQueue;
	int64 tBusLastDispatch;
	bool bOk;

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
	sCheckTime = XS_CheckConfigLastTimeText();
	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sReloadTime = XS_ReloadTimeText();
	sBusQueueTime = NULL;
	sBusDispatchTime = NULL;
	objBus = XS_BusBuildStatusValue();
	iBusQueueCount = 0;
	iBusDataCount = 0;
	iBusTotalQueued = 0;
	iBusTotalDelivered = 0;
	iBusTotalDropped = 0;
	tBusLastQueue = 0;
	tBusLastDispatch = 0;
	if ( objBus ) {
		iBusQueueCount = xvoTableGetInt(objBus, "queue_count", 11);
		iBusDataCount = xvoTableGetInt(objBus, "data_count", 10);
		iBusTotalQueued = xvoTableGetInt(objBus, "total_queued", 12);
		iBusTotalDelivered = xvoTableGetInt(objBus, "total_delivered", 15);
		iBusTotalDropped = xvoTableGetInt(objBus, "total_dropped", 13);
		tBusLastQueue = xvoTableGetInt(objBus, "last_queue_time", 15);
		tBusLastDispatch = xvoTableGetInt(objBus, "last_dispatch_time", 18);
		sBusQueueTime = (tBusLastQueue > 0) ? xrtTimeToStr(tBusLastQueue, XRT_TIME_FORMAT_DATETIME) : NULL;
		sBusDispatchTime = (tBusLastDispatch > 0) ? xrtTimeToStr(tBusLastDispatch, XRT_TIME_FORMAT_DATETIME) : NULL;
		xvoUnref(objBus);
	}
	snprintf(
		sBody,
		sizeof(sBody),
		"ok=%s\nserver=%s\nclass=%s\naddr=%s\nreload_busy=%s\nreload_message=%s\nreload_time=%s\nreload_age_ms=%lld\nreload_total_count=%lld\nreload_success_count=%lld\nreload_failure_count=%lld\ncheck_total_count=%lld\ncheck_success_count=%lld\ncheck_failure_count=%lld\ncheck_last_time=%s\ncheck_last_age_ms=%lld\nbus_queue_count=%lld\nbus_data_count=%lld\nbus_total_queued=%lld\nbus_total_delivered=%lld\nbus_total_dropped=%lld\nbus_last_queue_time=%lld\nbus_last_queue_time_text=%s\nbus_last_dispatch_time=%lld\nbus_last_dispatch_time_text=%s\nhttp_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_conn_current=%lld\nhttp_conn_peak=%lld\nhttp_time_total_ms=%lld\nhttp_time_max_ms=%lld\nhttp_time_avg_ms=%lld\nhttp_last_method=%s\nhttp_last_status=%lld\nhttp_last_path=%s\nhttp_last_target=%s\nhttp_last_time=%s\nhttp_last_age_ms=%lld\nhttp_last_app_method=%s\nhttp_last_app_status=%lld\nhttp_last_app_path=%s\nhttp_last_app_target=%s\nhttp_last_app_time=%s\nhttp_last_app_age_ms=%lld\n",
		bOk ? "true" : "false",
		objServer->Name ? objServer->Name : "(null)",
		XS_ServerClassName(objServer->Class),
		objServer->Addr ? objServer->Addr : "(null)",
		XS_ConfigReloadStatusBusy() ? "true" : "false",
		XS_ConfigReloadStatusMessage(),
		sReloadTime ? sReloadTime : "(none)",
		(long long)XS_ReloadAgeMS(),
		(long long)XS_ConfigReloadStatusTotalCount(),
		(long long)XS_ConfigReloadStatusSuccessCount(),
		(long long)XS_ConfigReloadStatusFailureCount(),
		(long long)XS_HttpMetricGet(&g_iXsCheckConfigTotalCount),
		(long long)XS_HttpMetricGet(&g_iXsCheckConfigSuccessCount),
		(long long)XS_HttpMetricGet(&g_iXsCheckConfigFailureCount),
		sCheckTime ? sCheckTime : "(none)",
		(long long)XS_CheckConfigLastAgeMS(),
		(long long)iBusQueueCount,
		(long long)iBusDataCount,
		(long long)iBusTotalQueued,
		(long long)iBusTotalDelivered,
		(long long)iBusTotalDropped,
		(long long)tBusLastQueue,
		sBusQueueTime ? sBusQueueTime : "(none)",
		(long long)tBusLastDispatch,
		sBusDispatchTime ? sBusDispatchTime : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpReqCount),
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
		sHttpLastTime ? sHttpLastTime : "(none)",
		(long long)XS_HttpLastRequestAgeMS(),
		XS_HttpLastAppMethodName()[0] ? XS_HttpLastAppMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode),
		XS_HttpLastAppPath()[0] ? XS_HttpLastAppPath() : "(none)",
		XS_HttpLastAppTarget()[0] ? XS_HttpLastAppTarget() : "(none)",
		sHttpLastAppTime ? sHttpLastAppTime : "(none)",
		(long long)XS_HttpLastAppRequestAgeMS()
	);
	if ( sCheckTime ) {
		xrtFree(sCheckTime);
	}
	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
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
	return XS_HttpRespondText(pResp, bOk ? 200 : 503, bOk ? "OK" : "Service Unavailable", sBody);
}

static inline bool XS_HttpHandleDashboard(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[8192];
	XS_Config objCfgCheck;
	char* sConfigBase;
	char* sCheckBase;
	char* sCheckTime;
	char* sStartTime;
	char* sCurrentDir;
	char* sConfigName;
	char* sAppMTime;
	char* sConfigMTime;
	char* sBusQueueTime;
	char* sBusDispatchTime;
	char* sHttpLastTime;
	char* sHttpLastAppTime;
	char* sWsLastTime;
	char* sWsLastErrorTime;
	char* sXtpLastTime;
	char* sXtpLastInvalidTime;
	char* sUdpLastTime;
	char* sUdpLastErrorTime;
	char* sCustomLastTime;
	char* sCustomLastErrorTime;
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
	bool bOk;
	bool bScriptLoaded;
	bool bConfigOK;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/dashboard") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "dashboard api disabled");
	}
	memset(&objCfgCheck, 0, sizeof(objCfgCheck));
	sConfigBase = (g_sXsConfigFile && g_sXsConfigFile[0]) ? xrtPathGetDir(g_sXsConfigFile, 0) : NULL;
	sCurrentDir = XS_CurrentWorkDir();
	sConfigName = XS_ConfigFileName();
	sAppMTime = XS_FileChangeTimeText(xCore.AppFile ? (char*)xCore.AppFile : NULL);
	sConfigMTime = XS_FileChangeTimeText(g_sXsConfigFile);
	iAppSize = XS_FileSizeValue(xCore.AppFile ? (char*)xCore.AppFile : NULL);
	iConfigSize = XS_FileSizeValue(g_sXsConfigFile);
	sCheckTime = XS_CheckConfigLastTimeText();
	sHttpLastTime = XS_HttpLastRequestTimeText();
	sHttpLastAppTime = XS_HttpLastAppRequestTimeText();
	sWsLastTime = XS_WsLastTimeText();
	sWsLastErrorTime = XS_WsLastErrorTimeText();
	sXtpLastTime = XS_XtpLastTimeText();
	sXtpLastInvalidTime = XS_XtpLastInvalidTimeText();
	sUdpLastTime = XS_UdpLastTimeText();
	sUdpLastErrorTime = XS_UdpLastErrorTimeText();
	sCustomLastTime = XS_CustomLastTimeText();
	sCustomLastErrorTime = XS_CustomLastErrorTimeText();
	sBusQueueTime = NULL;
	sBusDispatchTime = NULL;
	bConfigOK = (g_sXsConfigFile && g_sXsConfigFile[0]) ? XS_LoadConfig(&objCfgCheck, g_sXsConfigFile) : FALSE;
	sCheckBase = (bConfigOK && objCfgCheck.BaseDir) ? XS_CopyText(objCfgCheck.BaseDir) : NULL;
	sStartTime = g_tXsStartTime ? xrtTimeToStr(g_tXsStartTime, XRT_TIME_FORMAT_DATETIME) : NULL;

	objBus = XS_BusBuildStatusValue();
	iBusQueueCount = 0;
	iBusDataCount = 0;
	iBusTotalQueued = 0;
	iBusTotalDelivered = 0;
	iBusTotalDropped = 0;
	tBusLastQueue = 0;
	tBusLastDispatch = 0;
	if ( objBus ) {
		iBusQueueCount = xvoTableGetInt(objBus, "queue_count", 11);
		iBusDataCount = xvoTableGetInt(objBus, "data_count", 10);
		iBusTotalQueued = xvoTableGetInt(objBus, "total_queued", 12);
		iBusTotalDelivered = xvoTableGetInt(objBus, "total_delivered", 15);
		iBusTotalDropped = xvoTableGetInt(objBus, "total_dropped", 13);
		tBusLastQueue = xvoTableGetInt(objBus, "last_queue_time", 15);
		tBusLastDispatch = xvoTableGetInt(objBus, "last_dispatch_time", 18);
		sBusQueueTime = (tBusLastQueue > 0) ? xrtTimeToStr(tBusLastQueue, XRT_TIME_FORMAT_DATETIME) : NULL;
		sBusDispatchTime = (tBusLastDispatch > 0) ? xrtTimeToStr(tBusLastDispatch, XRT_TIME_FORMAT_DATETIME) : NULL;
		xvoUnref(objBus);
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
		"ok=%s\nserver=%s\nclass=%s\naddr=%s\nbind_ip=%s\nbind_port=%u\ntls=%s\nbind_ip_tls=%s\nbind_port_tls=%u\naddr_tls=%s\nws_protocol=%s\nws_message_limit=%u\nws_conn_current=%lld\nws_conn_peak=%lld\nws_open_count=%lld\nws_close_count=%lld\nws_text_count=%lld\nws_binary_count=%lld\nws_ping_count=%lld\nws_pong_count=%lld\nws_error_count=%lld\nws_last_error_code=%lld\nws_last_frame_type=%s\nws_last_bytes=%lld\nws_last_text=%s\nws_last_time=%s\nws_last_age_ms=%lld\nws_last_error_time=%s\nws_last_error_age_ms=%lld\nxtp_conn_current=%lld\nxtp_conn_peak=%lld\nxtp_open_count=%lld\nxtp_close_count=%lld\nxtp_error_count=%lld\nxtp_invalid_count=%lld\nxtp_msg_count=%lld\nxtp_req_count=%lld\nxtp_resp_count=%lld\nxtp_push_count=%lld\nxtp_event_count=%lld\nxtp_send_count=%lld\nxtp_recv_bytes=%lld\nxtp_send_bytes=%lld\nxtp_last_msg_type=%s\nxtp_last_status=%lld\nxtp_last_msg_id=%lld\nxtp_last_cmd=%s\nxtp_last_time=%s\nxtp_last_age_ms=%lld\nxtp_last_invalid_reason=%s\nxtp_last_invalid_time=%s\nxtp_last_invalid_age_ms=%lld\nudp_recv_count=%lld\nudp_send_count=%lld\nudp_error_count=%lld\nudp_last_error_code=%lld\nudp_recv_bytes=%lld\nudp_send_bytes=%lld\nudp_last_from=%s\nudp_last_text=%s\nudp_last_time=%s\nudp_last_age_ms=%lld\nudp_last_error_time=%s\nudp_last_error_age_ms=%lld\ncustom_conn_current=%lld\ncustom_conn_peak=%lld\ncustom_open_count=%lld\ncustom_close_count=%lld\ncustom_error_count=%lld\ncustom_invalid_count=%lld\ncustom_last_close_reason=%lld\ncustom_last_error_code=%lld\ncustom_last_error_time=%s\ncustom_last_error_age_ms=%lld\ncustom_recv_count=%lld\ncustom_send_count=%lld\ncustom_recv_bytes=%lld\ncustom_send_bytes=%lld\ncustom_last_text=%s\ncustom_last_time=%s\ncustom_last_age_ms=%lld\ntls_cert_file=%s\ntls_key_file=%s\ntls_ca_file=%s\ncurrent_dir=%s\napp_file=%s\napp_mtime=%s\napp_size=%lld\napp_path=%s\nbuild=%s\ncompiler=%s\nplatform=%s\narch=%s\nmem_debug=%s\npid=%llu\nstart_time=%s\nuptime_ms=%lld\nengine_workers=%u\nruntime_server_count=%u\nhttp_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_last_method=%s\nhttp_last_status=%lld\nhttp_last_path=%s\nhttp_last_target=%s\nhttp_last_time=%s\nhttp_last_age_ms=%lld\nhttp_last_app_method=%s\nhttp_last_app_status=%lld\nhttp_last_app_path=%s\nhttp_last_app_target=%s\nhttp_last_app_time=%s\nhttp_last_app_age_ms=%lld\nmanage_api=%s\nconfig_file=%s\nconfig_name=%s\nconfig_mtime=%s\nconfig_size=%lld\nconfig_base=%s\nconfig_check=%s\nconfig_check_base=%s\nconfig_check_server_count=%u\ncheck_last_time=%s\ncheck_last_age_ms=%lld\nhost_aware=%s\ndefault_host=%s\nhost_count=%u\nscript_loaded=%s\nreload_busy=%s\nreload_has_result=%s\nreload_success=%s\nreload_server=%s\nreload_host=%s\nreload_message=%s\nbus_queue_count=%lld\nbus_data_count=%lld\nbus_total_queued=%lld\nbus_total_delivered=%lld\nbus_total_dropped=%lld\nbus_last_queue_time=%lld\nbus_last_queue_time_text=%s\nbus_last_dispatch_time=%lld\nbus_last_dispatch_time_text=%s\n",
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
		(long long)g_iXsWsLastErrorCode,
		XS_WsLastFrameTypeName()[0] ? XS_WsLastFrameTypeName() : "(none)",
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
		g_sXsXtpLastCmd[0] ? g_sXsXtpLastCmd : "(none)",
		sXtpLastTime ? sXtpLastTime : "(none)",
		(long long)XS_XtpLastAgeMS(),
		g_sXsXtpLastInvalidReason[0] ? g_sXsXtpLastInvalidReason : "(none)",
		sXtpLastInvalidTime ? sXtpLastInvalidTime : "(none)",
		(long long)XS_XtpLastInvalidAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsUdpRecvCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpSendCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpErrorCount),
		(long long)g_iXsUdpLastErrorCode,
		(long long)XS_HttpMetricGet(&g_iXsUdpRecvBytes),
		(long long)XS_HttpMetricGet(&g_iXsUdpSendBytes),
		g_sXsUdpLastFrom[0] ? g_sXsUdpLastFrom : "(none)",
		g_sXsUdpLastText[0] ? g_sXsUdpLastText : "(none)",
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
		(long long)g_iXsCustomLastCloseReason,
		(long long)g_iXsCustomLastErrorCode,
		sCustomLastErrorTime ? sCustomLastErrorTime : "(none)",
		(long long)XS_CustomLastErrorAgeMS(),
		(long long)XS_HttpMetricGet(&g_iXsCustomRecvCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomSendCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomRecvBytes),
		(long long)XS_HttpMetricGet(&g_iXsCustomSendBytes),
		g_sXsCustomLastText[0] ? g_sXsCustomLastText : "(none)",
		sCustomLastTime ? sCustomLastTime : "(none)",
		(long long)XS_CustomLastAgeMS(),
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
		(long long)XS_HttpMetricGet(&g_iXsHttpResp2xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp3xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp4xxCount),
		(long long)XS_HttpMetricGet(&g_iXsHttpResp5xxCount),
		XS_HttpLastMethodName()[0] ? XS_HttpLastMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastStatusCode),
		XS_HttpLastPath()[0] ? XS_HttpLastPath() : "(none)",
		XS_HttpLastTarget()[0] ? XS_HttpLastTarget() : "(none)",
		sHttpLastTime ? sHttpLastTime : "(none)",
		(long long)XS_HttpLastRequestAgeMS(),
		XS_HttpLastAppMethodName()[0] ? XS_HttpLastAppMethodName() : "(none)",
		(long long)XS_HttpMetricGet(&g_iXsHttpLastAppStatusCode),
		XS_HttpLastAppPath()[0] ? XS_HttpLastAppPath() : "(none)",
		XS_HttpLastAppTarget()[0] ? XS_HttpLastAppTarget() : "(none)",
		sHttpLastAppTime ? sHttpLastAppTime : "(none)",
		(long long)XS_HttpLastAppRequestAgeMS(),
		XS_ManageAPIEnabled(objServer, objHost) ? "true" : "false",
		g_sXsConfigFile ? g_sXsConfigFile : "(null)",
		sConfigName ? sConfigName : "(null)",
		sConfigMTime ? sConfigMTime : "(null)",
		(long long)iConfigSize,
		sConfigBase ? sConfigBase : "(null)",
		bConfigOK ? "true" : "false",
		sCheckBase ? sCheckBase : "(null)",
		(unsigned int)((bConfigOK && objCfgCheck.Servers) ? objCfgCheck.Servers->Count : 0),
		sCheckTime ? sCheckTime : "(none)",
		(long long)XS_CheckConfigLastAgeMS(),
		objServer->HostAware ? "true" : "false",
		objServer->EnableDefaultHost ? "true" : "false",
		(unsigned int)(objServer->Hosts ? objServer->Hosts->Count : 0),
		bScriptLoaded ? "true" : "false",
		XS_ConfigReloadStatusBusy() ? "true" : "false",
		XS_ConfigReloadStatusHasResult() ? "true" : "false",
		XS_ConfigReloadStatusSuccess() ? "true" : "false",
		XS_ConfigReloadStatusServer(),
		XS_ConfigReloadStatusHost(),
		XS_ConfigReloadStatusMessage(),
		(long long)iBusQueueCount,
		(long long)iBusDataCount,
		(long long)iBusTotalQueued,
		(long long)iBusTotalDelivered,
		(long long)iBusTotalDropped,
		(long long)tBusLastQueue,
		sBusQueueTime ? sBusQueueTime : "(none)",
		(long long)tBusLastDispatch,
		sBusDispatchTime ? sBusDispatchTime : "(none)"
	);
	XS_FreeConfig(&objCfgCheck);
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
	if ( sHttpLastTime ) {
		xrtFree(sHttpLastTime);
	}
	if ( sHttpLastAppTime ) {
		xrtFree(sHttpLastAppTime);
	}
	if ( sWsLastTime ) {
		xrtFree(sWsLastTime);
	}
	if ( sWsLastErrorTime ) {
		xrtFree(sWsLastErrorTime);
	}
	if ( sXtpLastTime ) {
		xrtFree(sXtpLastTime);
	}
	if ( sXtpLastInvalidTime ) {
		xrtFree(sXtpLastInvalidTime);
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
	if ( sBusQueueTime ) {
		xrtFree(sBusQueueTime);
	}
	if ( sBusDispatchTime ) {
		xrtFree(sBusDispatchTime);
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
	return XS_HttpRespondText(pResp, bOk ? 200 : 503, bOk ? "OK" : "Service Unavailable", sBody);
}

static inline bool XS_HttpHandleCheckConfig(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_Config objCfgCheck;
	const char* sFilePath;
	char* sFileArg;
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
	sFilePath = (sFileArg && sFileArg[0]) ? sFileArg : g_sXsConfigFile;
	if ( sFilePath == NULL || sFilePath[0] == '\0' ) {
		if ( sFileArg ) {
			xrtFree(sFileArg);
		}
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "config file not set");
	}

	memset(&objCfgCheck, 0, sizeof(objCfgCheck));
	bOK = XS_LoadConfig(&objCfgCheck, sFilePath);
	XS_CheckConfigRecordResult(bOK);
	sLastTime = XS_CheckConfigLastTimeText();
	snprintf(
		sBody,
		sizeof(sBody),
		"result=%s\nfile=%s\nbase=%s\nserver_count=%u\ncheck_total_count=%lld\ncheck_success_count=%lld\ncheck_failure_count=%lld\ncheck_last_time=%s\ncheck_last_age_ms=%lld\n",
		bOK ? "true" : "false",
		sFilePath,
		(bOK && objCfgCheck.BaseDir) ? objCfgCheck.BaseDir : "(none)",
		(unsigned int)((bOK && objCfgCheck.Servers) ? objCfgCheck.Servers->Count : 0),
		(long long)XS_HttpMetricGet(&g_iXsCheckConfigTotalCount),
		(long long)XS_HttpMetricGet(&g_iXsCheckConfigSuccessCount),
		(long long)XS_HttpMetricGet(&g_iXsCheckConfigFailureCount),
		sLastTime ? sLastTime : "(none)",
		(long long)XS_CheckConfigLastAgeMS()
	);
	XS_FreeConfig(&objCfgCheck);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sFileArg ) {
		xrtFree(sFileArg);
	}
	return XS_HttpRespondText(pResp, bOK ? 200 : 500, bOK ? "OK" : "Internal Server Error", sBody);
}

static inline bool XS_HttpHandleCheckConfigJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	XS_Config objCfgCheck;
	xvalue objRet;
	const char* sFilePath;
	char* sFileArg;
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
		return XS_HttpRespondText(pResp, 403, "Forbidden", "check config json api disabled");
	}

	sFileArg = XS_HttpQueryDup(pReq->sQuery, "file");
	sFilePath = (sFileArg && sFileArg[0]) ? sFileArg : g_sXsConfigFile;
	if ( sFilePath == NULL || sFilePath[0] == '\0' ) {
		if ( sFileArg ) {
			xrtFree(sFileArg);
		}
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "config file not set");
	}

	memset(&objCfgCheck, 0, sizeof(objCfgCheck));
	bOK = XS_LoadConfig(&objCfgCheck, sFilePath);
	XS_CheckConfigRecordResult(bOK);
	sLastTime = XS_CheckConfigLastTimeText();

	objRet = xvoCreateTable();
	xvoTableSetBool(objRet, "result", 6, bOK);
	xvoTableSetText(objRet, "file", 4, (ptr)sFilePath, 0, FALSE);
	xvoTableSetText(objRet, "base", 4, (ptr)((bOK && objCfgCheck.BaseDir) ? objCfgCheck.BaseDir : ""), 0, FALSE);
	xvoTableSetInt(objRet, "server_count", 12, (bOK && objCfgCheck.Servers) ? objCfgCheck.Servers->Count : 0);
	xvoTableSetText(objRet, "message", 7, bOK ? "config check passed" : "config check failed", 0, FALSE);
	xvoTableSetInt(objRet, "check_total_count", 17, XS_HttpMetricGet(&g_iXsCheckConfigTotalCount));
	xvoTableSetInt(objRet, "check_success_count", 19, XS_HttpMetricGet(&g_iXsCheckConfigSuccessCount));
	xvoTableSetInt(objRet, "check_failure_count", 19, XS_HttpMetricGet(&g_iXsCheckConfigFailureCount));
	xvoTableSetText(objRet, "check_last_time", 15, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "check_last_age_ms", 17, XS_CheckConfigLastAgeMS());

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	XS_FreeConfig(&objCfgCheck);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sFileArg ) {
		xrtFree(sFileArg);
	}
	if ( sJson == NULL ) {
		return XS_HttpRespondText(pResp, 500, "Internal Server Error", "check config json build failed");
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
	char sBody[256];

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
	snprintf(
		sBody,
		sizeof(sBody),
		"check_total_count=%lld\ncheck_success_count=%lld\ncheck_failure_count=%lld\ncheck_last_time=(none)\ncheck_last_age_ms=-1\n",
		(long long)XS_HttpMetricGet(&g_iXsCheckConfigTotalCount),
		(long long)XS_HttpMetricGet(&g_iXsCheckConfigSuccessCount),
		(long long)XS_HttpMetricGet(&g_iXsCheckConfigFailureCount)
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static void XS_HttpOnOpen(ptr pOwner, xhttpdserver* pServer, xhttpdconn* pConn)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	(void)pServer;
	(void)pConn;

	XS_HttpOnOpenMetrics();
	
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
	(void)pServer;
	(void)pConn;
	
	if ( pReq == NULL || pResp == NULL ) {
		return FALSE;
	}

	bRet = FALSE;
	fStartTick = xrtTimer();
	XS_HttpRecordMethodMetrics(pReq);

	XS_HttpApplyDefaultHeaders(pReq, pResp);
	
	objHost = XS_HttpLocateHost(objServer, pReq);
	if ( objHost == NULL ) {
		bRet = XS_HttpRespondText(pResp, 404, "Not Found", "host not found");
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
	XS_HttpRecordResponseMetrics(pReq, pResp);
	return bRet;
}

static void XS_HttpOnClose(ptr pOwner, xhttpdserver* pServer, xhttpdconn* pConn, xnet_result iReason)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	(void)pServer;
	(void)pConn;

	XS_HttpOnCloseMetrics();
	
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
