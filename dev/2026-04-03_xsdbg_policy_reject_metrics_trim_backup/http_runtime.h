/* extracted http runtime surface */

static inline bool XS_HttpIsManagePath(const char* sPath);

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

static inline void XS_HttpRecordRemoteByConn(const xhttpdconn* pConn);
static inline const char* XS_HttpLastRemote(void);

static inline void XS_HttpLogRequest(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdconn* pConn)
{
	if ( objServer == NULL || objHost == NULL || pReq == NULL ) {
		return;
	}

	XS_HttpRecordRemoteByConn(pConn);
	XS_LogInfo(
		"http request: server=%s host=%s method=%s path=%s remote=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		objHost->Name ? objHost->Name : "(default)",
		pReq->sMethod,
		pReq->sPath,
		XS_HttpLastRemote()[0] ? XS_HttpLastRemote() : "(none)"
	);
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

static inline char* XS_RuntimeTimeText(xtime tLast)
{
	if ( tLast <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(tLast, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_RuntimeAgeMS(xtime tLast)
{
	if ( tLast <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - tLast) * 1000;
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

static inline void XS_HttpRecordRejectCommon(volatile int64* pCount, xtime* pTime, char* sReason, size_t iReasonCap, char* sRemote, size_t iRemoteCap, const char* sValue, const char* sRemoteValue)
{
	if ( pCount ) {
		XS_HttpMetricAdd(pCount, 1);
	}
	if ( pTime ) {
		*pTime = xrtNow();
	}
	XS_HttpMetricSetText(sReason, iReasonCap, sValue);
	XS_HttpMetricSetText(sRemote, iRemoteCap, sRemoteValue);
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

	XS_HttpRecordRejectCommon(&g_iXsHttpRejectCount, &g_tXsHttpLastRejectTime, g_sXsHttpLastRejectReason, sizeof(g_sXsHttpLastRejectReason), g_sXsHttpLastRejectRemote, sizeof(g_sXsHttpLastRejectRemote), sCanonical, g_sXsHttpLastRemote);
	g_iXsHttpLastRejectStatus = iStatusCode;
	XS_HttpRecordPolicyReject(sCanonical);
	XS_HttpRecordManageReject(sCanonical);
}

static inline void XS_HttpRecordStopCleanup(int64 iClosed, int64 iRemain)
{
	XS_HttpRecordStopCleanupCommon(&g_iXsHttpStopCleanupCount, &g_tXsHttpLastStopCleanupTime, &g_iXsHttpLastStopCleanupClosed, &g_iXsHttpLastStopCleanupRemain, iClosed, iRemain);
}

static inline void XS_RecordRejectReasonMetric(volatile int64* pCount, xtime* pTime)
{
	if ( pCount ) {
		XS_HttpMetricAdd(pCount, 1);
	}
	if ( pTime ) {
		*pTime = xrtNow();
	}
}

static inline const char* XS_WsCanonicalRejectReason(const char* sReason)
{
	if ( sReason == NULL || sReason[0] == '\0' ) {
		return "other";
	}
	if ( strcmp(sReason, "server_stopping") == 0 ) {
		return "server_stopping";
	}
	if ( strcmp(sReason, "conn_limit") == 0 ) {
		return "conn_limit";
	}
	if ( strcmp(sReason, "message limit exceeded") == 0 ) {
		return "message_limit";
	}
	if (
		(strcmp(sReason, "protocol violation") == 0) ||
		(strcmp(sReason, "invalid handshake") == 0)
	) {
		return "invalid";
	}

	return "other";
}

static inline void XS_WsRecordRejectReasonMetric(const char* sReason)
{
	if ( sReason == NULL || sReason[0] == '\0' ) {
		XS_RecordRejectReasonMetric(&g_iXsWsOtherRejectCount, &g_tXsWsLastOtherRejectTime);
		return;
	}
	if ( strcmp(sReason, "server_stopping") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsWsServerStoppingRejectCount, &g_tXsWsLastServerStoppingRejectTime);
		return;
	}
	if ( strcmp(sReason, "conn_limit") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsWsConnLimitRejectCount, &g_tXsWsLastConnLimitRejectTime);
		return;
	}
	if ( strcmp(sReason, "message_limit") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsWsMessageLimitRejectCount, &g_tXsWsLastMessageLimitRejectTime);
		return;
	}
	if ( strcmp(sReason, "invalid") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsWsInvalidRejectCount, &g_tXsWsLastInvalidRejectTime);
		return;
	}

	XS_RecordRejectReasonMetric(&g_iXsWsOtherRejectCount, &g_tXsWsLastOtherRejectTime);
}

static inline const char* XS_XtpCanonicalRejectReason(const char* sReason)
{
	if ( sReason == NULL || sReason[0] == '\0' ) {
		return "other";
	}
	if ( strcmp(sReason, "server_stopping") == 0 ) {
		return "server_stopping";
	}
	if ( strcmp(sReason, "conn_limit") == 0 ) {
		return "conn_limit";
	}
	if (
		(strcmp(sReason, "recv limit exceeded") == 0) ||
		(strcmp(sReason, "pack limit exceeded") == 0)
	) {
		return "recv_limit";
	}
	if (
		(strcmp(sReason, "invalid header") == 0) ||
		(strcmp(sReason, "invalid size") == 0) ||
		(strcmp(sReason, "size mismatch") == 0) ||
		(strcmp(sReason, "param overflow") == 0) ||
		(strcmp(sReason, "body overflow") == 0)
	) {
		return "invalid";
	}

	return "other";
}

static inline void XS_XtpRecordRejectReasonMetric(const char* sReason)
{
	if ( sReason == NULL || sReason[0] == '\0' ) {
		XS_RecordRejectReasonMetric(&g_iXsXtpOtherRejectCount, &g_tXsXtpLastOtherRejectTime);
		return;
	}
	if ( strcmp(sReason, "server_stopping") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsXtpServerStoppingRejectCount, &g_tXsXtpLastServerStoppingRejectTime);
		return;
	}
	if ( strcmp(sReason, "conn_limit") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsXtpConnLimitRejectCount, &g_tXsXtpLastConnLimitRejectTime);
		return;
	}
	if ( strcmp(sReason, "recv_limit") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsXtpRecvLimitRejectCount, &g_tXsXtpLastRecvLimitRejectTime);
		return;
	}
	if ( strcmp(sReason, "invalid") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsXtpInvalidRejectCount, &g_tXsXtpLastInvalidRejectTime);
		return;
	}

	XS_RecordRejectReasonMetric(&g_iXsXtpOtherRejectCount, &g_tXsXtpLastOtherRejectTime);
}

static inline const char* XS_CustomCanonicalRejectReason(const char* sReason)
{
	if ( sReason == NULL || sReason[0] == '\0' ) {
		return "other";
	}
	if ( strcmp(sReason, "server_stopping") == 0 ) {
		return "server_stopping";
	}
	if ( strcmp(sReason, "conn_limit") == 0 ) {
		return "conn_limit";
	}
	if ( strcmp(sReason, "recv limit exceeded") == 0 ) {
		return "recv_limit";
	}
	if ( strstr(sReason, "invalid") != NULL ) {
		return "invalid";
	}

	return "other";
}

static inline void XS_CustomRecordRejectReasonMetric(const char* sReason)
{
	if ( sReason == NULL || sReason[0] == '\0' ) {
		XS_RecordRejectReasonMetric(&g_iXsCustomOtherRejectCount, &g_tXsCustomLastOtherRejectTime);
		return;
	}
	if ( strcmp(sReason, "server_stopping") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsCustomServerStoppingRejectCount, &g_tXsCustomLastServerStoppingRejectTime);
		return;
	}
	if ( strcmp(sReason, "conn_limit") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsCustomConnLimitRejectCount, &g_tXsCustomLastConnLimitRejectTime);
		return;
	}
	if ( strcmp(sReason, "recv_limit") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsCustomRecvLimitRejectCount, &g_tXsCustomLastRecvLimitRejectTime);
		return;
	}
	if ( strcmp(sReason, "invalid") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsCustomInvalidRejectCount, &g_tXsCustomLastInvalidRejectTime);
		return;
	}

	XS_RecordRejectReasonMetric(&g_iXsCustomOtherRejectCount, &g_tXsCustomLastOtherRejectTime);
}

static inline const char* XS_UdpCanonicalRejectReason(const char* sReason)
{
	if ( sReason == NULL || sReason[0] == '\0' ) {
		return "other";
	}
	if ( strcmp(sReason, "recv limit exceeded") == 0 ) {
		return "recv_limit";
	}

	return "other";
}

static inline void XS_UdpRecordRejectEvent(const char* sReason, const char* sFrom, size_t iBytes)
{
	const char* sCanonical = XS_UdpCanonicalRejectReason(sReason);

	XS_HttpRecordRejectCommon(
		&g_iXsUdpRejectCount,
		&g_tXsUdpLastRejectTime,
		g_sXsUdpLastRejectReason,
		sizeof(g_sXsUdpLastRejectReason),
		g_sXsUdpLastRejectFrom,
		sizeof(g_sXsUdpLastRejectFrom),
		sCanonical,
		sFrom
	);
	g_iXsUdpLastRejectBytes = (int64)iBytes;
	if ( strcmp(sCanonical, "recv_limit") == 0 ) {
		XS_RecordRejectReasonMetric(&g_iXsUdpRecvLimitRejectCount, &g_tXsUdpLastRecvLimitRejectTime);
	}
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
	const char* sCanonical = XS_WsCanonicalRejectReason(sReason);

	XS_HttpRecordRejectCommon(&g_iXsWsRejectCount, &g_tXsWsLastRejectTime, g_sXsWsLastRejectReason, sizeof(g_sXsWsLastRejectReason), g_sXsWsLastRejectRemote, sizeof(g_sXsWsLastRejectRemote), sReason, g_sXsWsLastRemote);
	XS_WsRecordRejectReasonMetric(sCanonical);
}

static inline void XS_WsRecordStopCleanup(int64 iClosed, int64 iRemain)
{
	XS_HttpRecordStopCleanupCommon(&g_iXsWsStopCleanupCount, &g_tXsWsLastStopCleanupTime, &g_iXsWsLastStopCleanupClosed, &g_iXsWsLastStopCleanupRemain, iClosed, iRemain);
}

static inline void XS_XtpRecordRejectEvent(const char* sReason)
{
	const char* sCanonical = XS_XtpCanonicalRejectReason(sReason);

	XS_HttpRecordRejectCommon(&g_iXsXtpRejectCount, &g_tXsXtpLastRejectTime, g_sXsXtpLastRejectReason, sizeof(g_sXsXtpLastRejectReason), g_sXsXtpLastRejectRemote, sizeof(g_sXsXtpLastRejectRemote), sReason, g_sXsXtpLastRemote);
	XS_XtpRecordRejectReasonMetric(sCanonical);
}

static inline void XS_XtpRecordStopCleanup(int64 iClosed, int64 iRemain)
{
	XS_HttpRecordStopCleanupCommon(&g_iXsXtpStopCleanupCount, &g_tXsXtpLastStopCleanupTime, &g_iXsXtpLastStopCleanupClosed, &g_iXsXtpLastStopCleanupRemain, iClosed, iRemain);
}

static inline void XS_CustomRecordRejectEvent(const char* sReason)
{
	const char* sCanonical = XS_CustomCanonicalRejectReason(sReason);

	XS_HttpRecordRejectCommon(&g_iXsCustomRejectCount, &g_tXsCustomLastRejectTime, g_sXsCustomLastRejectReason, sizeof(g_sXsCustomLastRejectReason), g_sXsCustomLastRejectRemote, sizeof(g_sXsCustomLastRejectRemote), sReason, g_sXsCustomLastRemote);
	XS_CustomRecordRejectReasonMetric(sCanonical);
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

static inline void XS_HttpRecordRemoteByConn(const xhttpdconn* pConn)
{
	const char* sRemote;

	sRemote = XS_HttpConnRemoteText(pConn);
	if ( sRemote && sRemote[0] ) {
		strncpy(g_sXsHttpLastRemote, sRemote, sizeof(g_sXsHttpLastRemote) - 1);
		g_sXsHttpLastRemote[sizeof(g_sXsHttpLastRemote) - 1] = '\0';
	} else {
		g_sXsHttpLastRemote[0] = '\0';
	}
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

static inline void XS_HttpAbortTrackedConns(XS_HttpHandle* objHandle)
{
	xarray arrClose;
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		return;
	}

	arrClose = xrtArrayCreate(sizeof(xnetstream*), XRT_OBJMODE_LOCAL);
	if ( arrClose == NULL ) {
		return;
	}

	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_HttpConnContext** ppItem = (XS_HttpConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_HttpConnContext* objCtx = ppItem ? *ppItem : NULL;
		xnetstream* pStream = objCtx ? objCtx->pStream : NULL;
		xnetstream** ppClose;
		uint32 iPos;

		if ( objCtx == NULL || pStream == NULL ) {
			continue;
		}

		objCtx->bClosing = TRUE;
		iPos = xrtArrayAppend(arrClose, 1);
		ppClose = (xnetstream**)xrtArrayGet(arrClose, iPos);
		if ( ppClose ) {
			*ppClose = pStream;
		}
	}
	xrtMutexUnlock(objHandle->pConnLock);

	for ( i = 1; i <= arrClose->Count; i++ ) {
		xnetstream** ppClose = (xnetstream**)xrtArrayGet(arrClose, i);

		if ( ppClose && *ppClose ) {
			xrtNetStreamClose(*ppClose, XNET_CLOSE_F_ABORT);
		}
	}
	xrtArrayDestroy(arrClose);
}

static inline int64 XS_HttpFinalizeTrackedConns(XS_HttpHandle* objHandle)
{
	xarray arrFinalize;
	int64 iFinalizeCount;
	uint32 i;

	if ( objHandle == NULL || objHandle->pConnLock == NULL || objHandle->arrConn == NULL ) {
		return 0;
	}

	arrFinalize = xrtArrayCreate(sizeof(XS_HttpConnContext*), XRT_OBJMODE_LOCAL);
	if ( arrFinalize == NULL ) {
		return 0;
	}

	iFinalizeCount = 0;
	xrtMutexLock(objHandle->pConnLock);
	for ( i = 1; i <= objHandle->arrConn->Count; i++ ) {
		XS_HttpConnContext** ppItem = (XS_HttpConnContext**)xrtArrayGet(objHandle->arrConn, i);
		XS_HttpConnContext* objCtx = ppItem ? *ppItem : NULL;
		XS_HttpConnContext** ppFinalize;
		uint32 iPos;

		if ( objCtx == NULL ) {
			continue;
		}

		objCtx->bClosing = TRUE;
		iPos = xrtArrayAppend(arrFinalize, 1);
		ppFinalize = (XS_HttpConnContext**)xrtArrayGet(arrFinalize, iPos);
		if ( ppFinalize ) {
			*ppFinalize = objCtx;
			iFinalizeCount++;
		}
	}
	xrtArrayClear(objHandle->arrConn);
	xrtMutexUnlock(objHandle->pConnLock);

	for ( i = 1; i <= arrFinalize->Count; i++ ) {
		XS_HttpConnContext** ppFinalize = (XS_HttpConnContext**)xrtArrayGet(arrFinalize, i);
		XS_HttpConnContext* objCtx = ppFinalize ? *ppFinalize : NULL;

		if ( objCtx == NULL ) {
			continue;
		}

		if ( objCtx->pStream ) {
			objCtx->pStream = NULL;
		}
		objCtx->pConn = NULL;
		objCtx->pServer = NULL;
		objCtx->pTracker = NULL;
		XS_HttpOnCloseMetrics();
		xrtFree(objCtx);
	}
	xrtArrayDestroy(arrFinalize);
	return iFinalizeCount;
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
	XS_HttpRecordRemoteByConn(pConn);
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
				XS_HttpRecordRemoteByConn(pConn);
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
	g_sXsHttpLastRejectRemote[0] = '\0';
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
	g_iXsWsServerStoppingRejectCount = 0;
	g_iXsWsConnLimitRejectCount = 0;
	g_iXsWsMessageLimitRejectCount = 0;
	g_iXsWsInvalidRejectCount = 0;
	g_iXsWsOtherRejectCount = 0;
	
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
	g_tXsWsLastServerStoppingRejectTime = 0;
	g_tXsWsLastConnLimitRejectTime = 0;
	g_tXsWsLastMessageLimitRejectTime = 0;
	g_tXsWsLastInvalidRejectTime = 0;
	g_tXsWsLastOtherRejectTime = 0;
	g_iXsWsLastErrorCode = 0;
	g_iXsWsLastCloseReason = 0;
	g_iXsWsLastStopCleanupClosed = 0;
	g_iXsWsLastStopCleanupRemain = 0;
	g_sXsWsLastText[0] = '\0';
	g_sXsWsLastRemote[0] = '\0';
	g_sXsWsLastInvalidReason[0] = '\0';
	g_sXsWsLastInvalidRemote[0] = '\0';
	g_sXsWsLastRejectReason[0] = '\0';
	g_sXsWsLastRejectRemote[0] = '\0';
	g_sXsWsLastErrorRemote[0] = '\0';
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
	g_iXsXtpServerStoppingRejectCount = 0;
	g_iXsXtpConnLimitRejectCount = 0;
	g_iXsXtpRecvLimitRejectCount = 0;
	g_iXsXtpInvalidRejectCount = 0;
	g_iXsXtpOtherRejectCount = 0;
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
	g_sXsXtpLastInvalidRemote[0] = '\0';
	g_tXsXtpLastErrorTime = 0;
	g_iXsXtpLastErrorCode = 0;
	g_sXsXtpLastErrorRemote[0] = '\0';
	g_tXsXtpLastIdleCloseTime = 0;
	g_tXsXtpLastConnLimitCloseTime = 0;
	g_tXsXtpLastRecvLimitCloseTime = 0;
	g_tXsXtpLastStopCleanupTime = 0;
	g_tXsXtpLastRejectTime = 0;
	g_tXsXtpLastServerStoppingRejectTime = 0;
	g_tXsXtpLastConnLimitRejectTime = 0;
	g_tXsXtpLastRecvLimitRejectTime = 0;
	g_tXsXtpLastInvalidRejectTime = 0;
	g_tXsXtpLastOtherRejectTime = 0;
	g_iXsXtpRecvLimitCloseCount = 0;
	g_iXsXtpLastStopCleanupClosed = 0;
	g_iXsXtpLastStopCleanupRemain = 0;
	g_sXsXtpLastRejectReason[0] = '\0';
	g_sXsXtpLastRejectRemote[0] = '\0';
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
	iValue = XS_HttpMetricGet(&g_iXsUdpRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsUdpRejectCount, -iValue);
	}
	iValue = XS_HttpMetricGet(&g_iXsUdpRecvLimitRejectCount);
	if ( iValue != 0 ) {
		XS_HttpMetricAdd(&g_iXsUdpRecvLimitRejectCount, -iValue);
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
	g_tXsUdpLastRejectTime = 0;
	g_tXsUdpLastRecvLimitRejectTime = 0;
	g_iXsUdpLastErrorCode = 0;
	g_iXsUdpLastBytes = 0;
	g_iXsUdpLastRejectBytes = 0;
	g_sXsUdpLastFrom[0] = '\0';
	g_sXsUdpLastText[0] = '\0';
	g_sXsUdpLastRejectReason[0] = '\0';
	g_sXsUdpLastRejectFrom[0] = '\0';
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
	g_iXsCustomServerStoppingRejectCount = 0;
	g_iXsCustomConnLimitRejectCount = 0;
	g_iXsCustomRecvLimitRejectCount = 0;
	g_iXsCustomInvalidRejectCount = 0;
	g_iXsCustomOtherRejectCount = 0;
	g_tXsCustomLastTime = 0;
	g_tXsCustomLastErrorTime = 0;
	g_tXsCustomLastInvalidTime = 0;
	g_tXsCustomLastIdleCloseTime = 0;
	g_tXsCustomLastConnLimitCloseTime = 0;
	g_tXsCustomLastRecvLimitCloseTime = 0;
	g_tXsCustomLastStopCleanupTime = 0;
	g_tXsCustomLastRejectTime = 0;
	g_tXsCustomLastServerStoppingRejectTime = 0;
	g_tXsCustomLastConnLimitRejectTime = 0;
	g_tXsCustomLastRecvLimitRejectTime = 0;
	g_tXsCustomLastInvalidRejectTime = 0;
	g_tXsCustomLastOtherRejectTime = 0;
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
	g_sXsCustomLastInvalidRemote[0] = '\0';
	g_sXsCustomLastRejectReason[0] = '\0';
	g_sXsCustomLastRejectRemote[0] = '\0';
	g_sXsCustomLastErrorRemote[0] = '\0';
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

static inline char* XS_UdpLastRejectTimeText(void)
{
	if ( g_tXsUdpLastRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsUdpLastRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_UdpLastRejectAgeMS(void)
{
	if ( g_tXsUdpLastRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsUdpLastRejectTime) * 1000;
}

static inline char* XS_UdpLastRecvLimitRejectTimeText(void)
{
	if ( g_tXsUdpLastRecvLimitRejectTime <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr(g_tXsUdpLastRecvLimitRejectTime, XRT_TIME_FORMAT_DATETIME);
}

static inline int64 XS_UdpLastRecvLimitRejectAgeMS(void)
{
	if ( g_tXsUdpLastRecvLimitRejectTime <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - g_tXsUdpLastRecvLimitRejectTime) * 1000;
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
	xvoTableSetText(objValue, "http_last_reject_remote", sizeof("http_last_reject_remote") - 1, (ptr)g_sXsHttpLastRejectRemote, 0, FALSE);
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
	(void)objValue;
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
	char* sServerStoppingTime;
	char* sConnLimitTime;
	char* sMessageLimitTime;
	char* sInvalidTime;
	char* sOtherTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_WsLastRejectTimeText();
	sServerStoppingTime = XS_RuntimeTimeText(g_tXsWsLastServerStoppingRejectTime);
	sConnLimitTime = XS_RuntimeTimeText(g_tXsWsLastConnLimitRejectTime);
	sMessageLimitTime = XS_RuntimeTimeText(g_tXsWsLastMessageLimitRejectTime);
	sInvalidTime = XS_RuntimeTimeText(g_tXsWsLastInvalidRejectTime);
	sOtherTime = XS_RuntimeTimeText(g_tXsWsLastOtherRejectTime);
	xvoTableSetInt(objValue, "ws_reject_count", sizeof("ws_reject_count") - 1, XS_HttpMetricGet(&g_iXsWsRejectCount));
	xvoTableSetText(objValue, "ws_last_reject_reason", sizeof("ws_last_reject_reason") - 1, (ptr)g_sXsWsLastRejectReason, 0, FALSE);
	xvoTableSetText(objValue, "ws_last_reject_remote", sizeof("ws_last_reject_remote") - 1, (ptr)g_sXsWsLastRejectRemote, 0, FALSE);
	xvoTableSetText(objValue, "ws_last_reject_time", sizeof("ws_last_reject_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "ws_last_reject_age_ms", sizeof("ws_last_reject_age_ms") - 1, XS_WsLastRejectAgeMS());
	xvoTableSetInt(objValue, "ws_server_stopping_reject_count", sizeof("ws_server_stopping_reject_count") - 1, XS_HttpMetricGet(&g_iXsWsServerStoppingRejectCount));
	xvoTableSetText(objValue, "ws_last_server_stopping_reject_time", sizeof("ws_last_server_stopping_reject_time") - 1, (ptr)(sServerStoppingTime ? sServerStoppingTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "ws_last_server_stopping_reject_age_ms", sizeof("ws_last_server_stopping_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsWsLastServerStoppingRejectTime));
	xvoTableSetInt(objValue, "ws_conn_limit_reject_count", sizeof("ws_conn_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsWsConnLimitRejectCount));
	xvoTableSetText(objValue, "ws_last_conn_limit_reject_time", sizeof("ws_last_conn_limit_reject_time") - 1, (ptr)(sConnLimitTime ? sConnLimitTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "ws_last_conn_limit_reject_age_ms", sizeof("ws_last_conn_limit_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsWsLastConnLimitRejectTime));
	xvoTableSetInt(objValue, "ws_message_limit_reject_count", sizeof("ws_message_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsWsMessageLimitRejectCount));
	xvoTableSetText(objValue, "ws_last_message_limit_reject_time", sizeof("ws_last_message_limit_reject_time") - 1, (ptr)(sMessageLimitTime ? sMessageLimitTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "ws_last_message_limit_reject_age_ms", sizeof("ws_last_message_limit_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsWsLastMessageLimitRejectTime));
	xvoTableSetInt(objValue, "ws_invalid_reject_count", sizeof("ws_invalid_reject_count") - 1, XS_HttpMetricGet(&g_iXsWsInvalidRejectCount));
	xvoTableSetText(objValue, "ws_last_invalid_reject_time", sizeof("ws_last_invalid_reject_time") - 1, (ptr)(sInvalidTime ? sInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "ws_last_invalid_reject_age_ms", sizeof("ws_last_invalid_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsWsLastInvalidRejectTime));
	xvoTableSetInt(objValue, "ws_other_reject_count", sizeof("ws_other_reject_count") - 1, XS_HttpMetricGet(&g_iXsWsOtherRejectCount));
	xvoTableSetText(objValue, "ws_last_other_reject_time", sizeof("ws_last_other_reject_time") - 1, (ptr)(sOtherTime ? sOtherTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "ws_last_other_reject_age_ms", sizeof("ws_last_other_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsWsLastOtherRejectTime));
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sServerStoppingTime ) {
		xrtFree(sServerStoppingTime);
	}
	if ( sConnLimitTime ) {
		xrtFree(sConnLimitTime);
	}
	if ( sMessageLimitTime ) {
		xrtFree(sMessageLimitTime);
	}
	if ( sInvalidTime ) {
		xrtFree(sInvalidTime);
	}
	if ( sOtherTime ) {
		xrtFree(sOtherTime);
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
	char* sServerStoppingTime;
	char* sConnLimitTime;
	char* sRecvLimitTime;
	char* sInvalidTime;
	char* sOtherTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_XtpLastRejectTimeText();
	sServerStoppingTime = XS_RuntimeTimeText(g_tXsXtpLastServerStoppingRejectTime);
	sConnLimitTime = XS_RuntimeTimeText(g_tXsXtpLastConnLimitRejectTime);
	sRecvLimitTime = XS_RuntimeTimeText(g_tXsXtpLastRecvLimitRejectTime);
	sInvalidTime = XS_RuntimeTimeText(g_tXsXtpLastInvalidRejectTime);
	sOtherTime = XS_RuntimeTimeText(g_tXsXtpLastOtherRejectTime);
	xvoTableSetInt(objValue, "xtp_reject_count", sizeof("xtp_reject_count") - 1, XS_HttpMetricGet(&g_iXsXtpRejectCount));
	xvoTableSetText(objValue, "xtp_last_reject_reason", sizeof("xtp_last_reject_reason") - 1, (ptr)g_sXsXtpLastRejectReason, 0, FALSE);
	xvoTableSetText(objValue, "xtp_last_reject_remote", sizeof("xtp_last_reject_remote") - 1, (ptr)g_sXsXtpLastRejectRemote, 0, FALSE);
	xvoTableSetText(objValue, "xtp_last_reject_time", sizeof("xtp_last_reject_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "xtp_last_reject_age_ms", sizeof("xtp_last_reject_age_ms") - 1, XS_XtpLastRejectAgeMS());
	xvoTableSetInt(objValue, "xtp_server_stopping_reject_count", sizeof("xtp_server_stopping_reject_count") - 1, XS_HttpMetricGet(&g_iXsXtpServerStoppingRejectCount));
	xvoTableSetText(objValue, "xtp_last_server_stopping_reject_time", sizeof("xtp_last_server_stopping_reject_time") - 1, (ptr)(sServerStoppingTime ? sServerStoppingTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "xtp_last_server_stopping_reject_age_ms", sizeof("xtp_last_server_stopping_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsXtpLastServerStoppingRejectTime));
	xvoTableSetInt(objValue, "xtp_conn_limit_reject_count", sizeof("xtp_conn_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsXtpConnLimitRejectCount));
	xvoTableSetText(objValue, "xtp_last_conn_limit_reject_time", sizeof("xtp_last_conn_limit_reject_time") - 1, (ptr)(sConnLimitTime ? sConnLimitTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "xtp_last_conn_limit_reject_age_ms", sizeof("xtp_last_conn_limit_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsXtpLastConnLimitRejectTime));
	xvoTableSetInt(objValue, "xtp_recv_limit_reject_count", sizeof("xtp_recv_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsXtpRecvLimitRejectCount));
	xvoTableSetText(objValue, "xtp_last_recv_limit_reject_time", sizeof("xtp_last_recv_limit_reject_time") - 1, (ptr)(sRecvLimitTime ? sRecvLimitTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "xtp_last_recv_limit_reject_age_ms", sizeof("xtp_last_recv_limit_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsXtpLastRecvLimitRejectTime));
	xvoTableSetInt(objValue, "xtp_invalid_reject_count", sizeof("xtp_invalid_reject_count") - 1, XS_HttpMetricGet(&g_iXsXtpInvalidRejectCount));
	xvoTableSetText(objValue, "xtp_last_invalid_reject_time", sizeof("xtp_last_invalid_reject_time") - 1, (ptr)(sInvalidTime ? sInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "xtp_last_invalid_reject_age_ms", sizeof("xtp_last_invalid_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsXtpLastInvalidRejectTime));
	xvoTableSetInt(objValue, "xtp_other_reject_count", sizeof("xtp_other_reject_count") - 1, XS_HttpMetricGet(&g_iXsXtpOtherRejectCount));
	xvoTableSetText(objValue, "xtp_last_other_reject_time", sizeof("xtp_last_other_reject_time") - 1, (ptr)(sOtherTime ? sOtherTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "xtp_last_other_reject_age_ms", sizeof("xtp_last_other_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsXtpLastOtherRejectTime));
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sServerStoppingTime ) {
		xrtFree(sServerStoppingTime);
	}
	if ( sConnLimitTime ) {
		xrtFree(sConnLimitTime);
	}
	if ( sRecvLimitTime ) {
		xrtFree(sRecvLimitTime);
	}
	if ( sInvalidTime ) {
		xrtFree(sInvalidTime);
	}
	if ( sOtherTime ) {
		xrtFree(sOtherTime);
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
	char* sServerStoppingTime;
	char* sConnLimitTime;
	char* sRecvLimitTime;
	char* sInvalidTime;
	char* sOtherTime;

	if ( objValue == NULL ) {
		return;
	}

	sLastTime = XS_CustomLastRejectTimeText();
	sServerStoppingTime = XS_RuntimeTimeText(g_tXsCustomLastServerStoppingRejectTime);
	sConnLimitTime = XS_RuntimeTimeText(g_tXsCustomLastConnLimitRejectTime);
	sRecvLimitTime = XS_RuntimeTimeText(g_tXsCustomLastRecvLimitRejectTime);
	sInvalidTime = XS_RuntimeTimeText(g_tXsCustomLastInvalidRejectTime);
	sOtherTime = XS_RuntimeTimeText(g_tXsCustomLastOtherRejectTime);
	xvoTableSetInt(objValue, "custom_reject_count", sizeof("custom_reject_count") - 1, XS_HttpMetricGet(&g_iXsCustomRejectCount));
	xvoTableSetText(objValue, "custom_last_reject_reason", sizeof("custom_last_reject_reason") - 1, (ptr)g_sXsCustomLastRejectReason, 0, FALSE);
	xvoTableSetText(objValue, "custom_last_reject_remote", sizeof("custom_last_reject_remote") - 1, (ptr)g_sXsCustomLastRejectRemote, 0, FALSE);
	xvoTableSetText(objValue, "custom_last_reject_time", sizeof("custom_last_reject_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "custom_last_reject_age_ms", sizeof("custom_last_reject_age_ms") - 1, XS_CustomLastRejectAgeMS());
	xvoTableSetInt(objValue, "custom_server_stopping_reject_count", sizeof("custom_server_stopping_reject_count") - 1, XS_HttpMetricGet(&g_iXsCustomServerStoppingRejectCount));
	xvoTableSetText(objValue, "custom_last_server_stopping_reject_time", sizeof("custom_last_server_stopping_reject_time") - 1, (ptr)(sServerStoppingTime ? sServerStoppingTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "custom_last_server_stopping_reject_age_ms", sizeof("custom_last_server_stopping_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsCustomLastServerStoppingRejectTime));
	xvoTableSetInt(objValue, "custom_conn_limit_reject_count", sizeof("custom_conn_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsCustomConnLimitRejectCount));
	xvoTableSetText(objValue, "custom_last_conn_limit_reject_time", sizeof("custom_last_conn_limit_reject_time") - 1, (ptr)(sConnLimitTime ? sConnLimitTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "custom_last_conn_limit_reject_age_ms", sizeof("custom_last_conn_limit_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsCustomLastConnLimitRejectTime));
	xvoTableSetInt(objValue, "custom_recv_limit_reject_count", sizeof("custom_recv_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsCustomRecvLimitRejectCount));
	xvoTableSetText(objValue, "custom_last_recv_limit_reject_time", sizeof("custom_last_recv_limit_reject_time") - 1, (ptr)(sRecvLimitTime ? sRecvLimitTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "custom_last_recv_limit_reject_age_ms", sizeof("custom_last_recv_limit_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsCustomLastRecvLimitRejectTime));
	xvoTableSetInt(objValue, "custom_invalid_reject_count", sizeof("custom_invalid_reject_count") - 1, XS_HttpMetricGet(&g_iXsCustomInvalidRejectCount));
	xvoTableSetText(objValue, "custom_last_invalid_reject_time", sizeof("custom_last_invalid_reject_time") - 1, (ptr)(sInvalidTime ? sInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "custom_last_invalid_reject_age_ms", sizeof("custom_last_invalid_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsCustomLastInvalidRejectTime));
	xvoTableSetInt(objValue, "custom_other_reject_count", sizeof("custom_other_reject_count") - 1, XS_HttpMetricGet(&g_iXsCustomOtherRejectCount));
	xvoTableSetText(objValue, "custom_last_other_reject_time", sizeof("custom_last_other_reject_time") - 1, (ptr)(sOtherTime ? sOtherTime : ""), 0, FALSE);
	xvoTableSetInt(objValue, "custom_last_other_reject_age_ms", sizeof("custom_last_other_reject_age_ms") - 1, XS_RuntimeAgeMS(g_tXsCustomLastOtherRejectTime));
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sServerStoppingTime ) {
		xrtFree(sServerStoppingTime);
	}
	if ( sConnLimitTime ) {
		xrtFree(sConnLimitTime);
	}
	if ( sRecvLimitTime ) {
		xrtFree(sRecvLimitTime);
	}
	if ( sInvalidTime ) {
		xrtFree(sInvalidTime);
	}
	if ( sOtherTime ) {
		xrtFree(sOtherTime);
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
