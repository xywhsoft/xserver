#ifndef XS_MANAGE_HTTP_MANAGE_DEBUG_H
#define XS_MANAGE_HTTP_MANAGE_DEBUG_H

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

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/http_metrics") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "http metrics api disabled");
	}
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
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
			"http_reject_count=%lld\nhttp_last_reject_status=%lld\nhttp_last_reject_reason=%s\nhttp_last_reject_remote=%s\nhttp_last_reject_time=%s\nhttp_last_reject_age_ms=%lld\nhttp_stop_cleanup_count=%lld\nhttp_last_stop_cleanup_closed=%lld\nhttp_last_stop_cleanup_remain=%lld\nhttp_last_stop_cleanup_time=%s\nhttp_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsHttpLastRejectStatus),
			g_sXsHttpLastRejectReason[0] ? g_sXsHttpLastRejectReason : "(none)",
			g_sXsHttpLastRejectRemote[0] ? g_sXsHttpLastRejectRemote : "(none)",
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
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
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
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
	}

	XS_HttpClearMetrics();
	snprintf(
		sBody,
		sizeof(sBody),
		"http_req_count=%lld\nhttp_manage_req_count=%lld\nhttp_app_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_conn_current=%lld\nhttp_conn_peak=%lld\nhttp_idle_close_count=%lld\nhttp_conn_limit_close_count=%lld\nhttp_reject_count=%lld\nhttp_stop_cleanup_count=%lld\nhttp_get_count=%lld\nhttp_post_count=%lld\nhttp_head_count=%lld\nhttp_other_count=%lld\nhttp_time_total_ms=%lld\nhttp_time_max_ms=%lld\nhttp_time_avg_ms=%lld\nhttp_last_method=\nhttp_last_status=%lld\nhttp_last_reject_status=0\nhttp_last_reject_reason=(none)\nhttp_last_reject_remote=(none)\nhttp_last_stop_cleanup_closed=0\nhttp_last_stop_cleanup_remain=0\nhttp_last_stop_cleanup_time=(none)\nhttp_last_stop_cleanup_age_ms=-1\nhttp_last_path=(none)\nhttp_last_target=(none)\nhttp_last_remote=(none)\nhttp_last_host=(none)\nhttp_last_user_agent=(none)\nhttp_last_referer=(none)\nhttp_last_origin=(none)\nhttp_last_accept=(none)\nhttp_last_accept_encoding=(none)\nhttp_last_content_type=(none)\nhttp_last_header_count=0\nhttp_last_query_len=0\nhttp_last_body_len=0\nhttp_last_time=(none)\nhttp_last_age_ms=-1\nhttp_last_duration_ms=0\nhttp_last_idle_close_time=(none)\nhttp_last_idle_close_age_ms=-1\nhttp_last_conn_limit_close_time=(none)\nhttp_last_conn_limit_close_age_ms=-1\nhttp_last_reject_time=(none)\nhttp_last_reject_age_ms=-1\nhttp_last_app_method=\nhttp_last_app_status=0\nhttp_last_app_path=(none)\nhttp_last_app_target=(none)\nhttp_last_app_remote=(none)\nhttp_last_app_host=(none)\nhttp_last_app_user_agent=(none)\nhttp_last_app_referer=(none)\nhttp_last_app_origin=(none)\nhttp_last_app_accept=(none)\nhttp_last_app_accept_encoding=(none)\nhttp_last_app_content_type=(none)\nhttp_last_app_header_count=0\nhttp_last_app_query_len=0\nhttp_last_app_body_len=0\nhttp_last_app_time=(none)\nhttp_last_app_age_ms=-1\nhttp_last_app_duration_ms=0\n",
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
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleWsMetrics(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[4096];
	char* sLastTime;
	char* sLastCloseTime;
	char* sLastErrorTime;
	char* sLastInvalidTime;
	char* sLastIdleCloseTime;
	char* sLastConnLimitCloseTime;
	char* sLastMessageLimitCloseTime;
	char* sLastRejectTime;
	char* sLastStopCleanupTime;
	char* sLastServerStoppingRejectTime;
	char* sLastConnLimitRejectTime;
	char* sLastMessageLimitRejectTime;
	char* sLastInvalidRejectTime;
	char* sLastOtherRejectTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/ws_metrics") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "ws metrics api disabled");
	}
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
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
	sLastServerStoppingRejectTime = XS_RuntimeTimeText(g_tXsWsLastServerStoppingRejectTime);
	sLastConnLimitRejectTime = XS_RuntimeTimeText(g_tXsWsLastConnLimitRejectTime);
	sLastMessageLimitRejectTime = XS_RuntimeTimeText(g_tXsWsLastMessageLimitRejectTime);
	sLastInvalidRejectTime = XS_RuntimeTimeText(g_tXsWsLastInvalidRejectTime);
	sLastOtherRejectTime = XS_RuntimeTimeText(g_tXsWsLastOtherRejectTime);
	snprintf(
		sBody,
		sizeof(sBody),
		"ws_message_limit=%u\nws_conn_current=%lld\nws_conn_peak=%lld\nws_open_count=%lld\nws_close_count=%lld\nws_text_count=%lld\nws_binary_count=%lld\nws_ping_count=%lld\nws_pong_count=%lld\nws_error_count=%lld\nws_invalid_count=%lld\nws_idle_close_count=%lld\nws_conn_limit_close_count=%lld\nws_message_limit_close_count=%lld\nws_last_error_code=%lld\nws_last_invalid_reason=%s\nws_last_invalid_remote=%s\nws_last_invalid_time=%s\nws_last_invalid_age_ms=%lld\nws_last_close_reason=%lld\nws_last_close_time=%s\nws_last_close_age_ms=%lld\nws_last_idle_close_time=%s\nws_last_idle_close_age_ms=%lld\nws_last_conn_limit_close_time=%s\nws_last_conn_limit_close_age_ms=%lld\nws_last_message_limit_close_time=%s\nws_last_message_limit_close_age_ms=%lld\nws_last_frame_type=%s\nws_last_remote=%s\nws_last_bytes=%lld\nws_last_text=%s\nws_last_time=%s\nws_last_age_ms=%lld\nws_last_error_remote=%s\nws_last_error_time=%s\nws_last_error_age_ms=%lld\n",
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
		g_sXsWsLastInvalidRemote[0] ? g_sXsWsLastInvalidRemote : "(none)",
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
		g_sXsWsLastErrorRemote[0] ? g_sXsWsLastErrorRemote : "(none)",
		sLastErrorTime ? sLastErrorTime : "(none)",
		(long long)XS_WsLastErrorAgeMS()
	);
	if ( strlen(sBody) < sizeof(sBody) ) {
		snprintf(
			sBody + strlen(sBody),
			sizeof(sBody) - strlen(sBody),
			"ws_reject_count=%lld\nws_last_reject_reason=%s\nws_last_reject_remote=%s\nws_last_reject_time=%s\nws_last_reject_age_ms=%lld\nws_server_stopping_reject_count=%lld\nws_last_server_stopping_reject_time=%s\nws_last_server_stopping_reject_age_ms=%lld\nws_conn_limit_reject_count=%lld\nws_last_conn_limit_reject_time=%s\nws_last_conn_limit_reject_age_ms=%lld\nws_message_limit_reject_count=%lld\nws_last_message_limit_reject_time=%s\nws_last_message_limit_reject_age_ms=%lld\nws_invalid_reject_count=%lld\nws_last_invalid_reject_time=%s\nws_last_invalid_reject_age_ms=%lld\nws_other_reject_count=%lld\nws_last_other_reject_time=%s\nws_last_other_reject_age_ms=%lld\nws_stop_cleanup_count=%lld\nws_last_stop_cleanup_closed=%lld\nws_last_stop_cleanup_remain=%lld\nws_last_stop_cleanup_time=%s\nws_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsWsRejectCount),
			g_sXsWsLastRejectReason[0] ? g_sXsWsLastRejectReason : "(none)",
			g_sXsWsLastRejectRemote[0] ? g_sXsWsLastRejectRemote : "(none)",
			sLastRejectTime ? sLastRejectTime : "(none)",
			(long long)XS_WsLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsWsServerStoppingRejectCount),
			sLastServerStoppingRejectTime ? sLastServerStoppingRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsWsLastServerStoppingRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsWsConnLimitRejectCount),
			sLastConnLimitRejectTime ? sLastConnLimitRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsWsLastConnLimitRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsWsMessageLimitRejectCount),
			sLastMessageLimitRejectTime ? sLastMessageLimitRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsWsLastMessageLimitRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsWsInvalidRejectCount),
			sLastInvalidRejectTime ? sLastInvalidRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsWsLastInvalidRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsWsOtherRejectCount),
			sLastOtherRejectTime ? sLastOtherRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsWsLastOtherRejectTime),
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
	if ( sLastServerStoppingRejectTime ) {
		xrtFree(sLastServerStoppingRejectTime);
	}
	if ( sLastConnLimitRejectTime ) {
		xrtFree(sLastConnLimitRejectTime);
	}
	if ( sLastMessageLimitRejectTime ) {
		xrtFree(sLastMessageLimitRejectTime);
	}
	if ( sLastInvalidRejectTime ) {
		xrtFree(sLastInvalidRejectTime);
	}
	if ( sLastOtherRejectTime ) {
		xrtFree(sLastOtherRejectTime);
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
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
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
	xvoTableSetText(objRet, "ws_last_invalid_remote", 22, (ptr)g_sXsWsLastInvalidRemote, 0, FALSE);
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
	xvoTableSetText(objRet, "ws_last_error_remote", 20, (ptr)g_sXsWsLastErrorRemote, 0, FALSE);
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
	char sBody[2048];

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/ws_metrics_clear") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "ws metrics clear api disabled");
	}
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
	}

	XS_WsClearMetrics();
	snprintf(
		sBody,
		sizeof(sBody),
		"ws_message_limit=%u\nws_conn_current=%lld\nws_conn_peak=%lld\nws_open_count=%lld\nws_close_count=%lld\nws_text_count=%lld\nws_binary_count=%lld\nws_ping_count=%lld\nws_pong_count=%lld\nws_error_count=%lld\nws_invalid_count=0\nws_idle_close_count=0\nws_conn_limit_close_count=0\nws_message_limit_close_count=0\nws_reject_count=0\nws_stop_cleanup_count=0\nws_last_reject_reason=(none)\nws_last_reject_remote=(none)\nws_last_reject_time=(none)\nws_last_reject_age_ms=-1\nws_server_stopping_reject_count=0\nws_last_server_stopping_reject_time=(none)\nws_last_server_stopping_reject_age_ms=-1\nws_conn_limit_reject_count=0\nws_last_conn_limit_reject_time=(none)\nws_last_conn_limit_reject_age_ms=-1\nws_message_limit_reject_count=0\nws_last_message_limit_reject_time=(none)\nws_last_message_limit_reject_age_ms=-1\nws_invalid_reject_count=0\nws_last_invalid_reject_time=(none)\nws_last_invalid_reject_age_ms=-1\nws_other_reject_count=0\nws_last_other_reject_time=(none)\nws_last_other_reject_age_ms=-1\nws_last_stop_cleanup_closed=0\nws_last_stop_cleanup_remain=0\nws_last_stop_cleanup_time=(none)\nws_last_stop_cleanup_age_ms=-1\nws_last_error_code=0\nws_last_invalid_reason=(none)\nws_last_invalid_remote=(none)\nws_last_invalid_time=(none)\nws_last_invalid_age_ms=-1\nws_last_close_reason=0\nws_last_close_time=(none)\nws_last_close_age_ms=-1\nws_last_idle_close_time=(none)\nws_last_idle_close_age_ms=-1\nws_last_conn_limit_close_time=(none)\nws_last_conn_limit_close_age_ms=-1\nws_last_message_limit_close_time=(none)\nws_last_message_limit_close_age_ms=-1\nws_last_frame_type=(none)\nws_last_remote=(none)\nws_last_bytes=0\nws_last_text=(none)\nws_last_time=(none)\nws_last_age_ms=-1\nws_last_error_remote=(none)\nws_last_error_time=(none)\nws_last_error_age_ms=-1\n",
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
	char sBody[4096];
	char* sLastTime;
	char* sLastInvalidTime;
	char* sLastErrorTime;
	char* sLastIdleCloseTime;
	char* sLastConnLimitCloseTime;
	char* sLastRecvLimitCloseTime;
	char* sLastRejectTime;
	char* sLastStopCleanupTime;
	char* sLastServerStoppingRejectTime;
	char* sLastConnLimitRejectTime;
	char* sLastRecvLimitRejectTime;
	char* sLastInvalidRejectTime;
	char* sLastOtherRejectTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/xtp_metrics") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "xtp metrics api disabled");
	}
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
	}

	sLastTime = XS_XtpLastTimeText();
	sLastInvalidTime = XS_XtpLastInvalidTimeText();
	sLastErrorTime = XS_XtpLastErrorTimeText();
	sLastIdleCloseTime = XS_XtpLastIdleCloseTimeText();
	sLastConnLimitCloseTime = XS_XtpLastConnLimitCloseTimeText();
	sLastRecvLimitCloseTime = XS_XtpLastRecvLimitCloseTimeText();
	sLastRejectTime = XS_XtpLastRejectTimeText();
	sLastStopCleanupTime = XS_XtpLastStopCleanupTimeText();
	sLastServerStoppingRejectTime = XS_RuntimeTimeText(g_tXsXtpLastServerStoppingRejectTime);
	sLastConnLimitRejectTime = XS_RuntimeTimeText(g_tXsXtpLastConnLimitRejectTime);
	sLastRecvLimitRejectTime = XS_RuntimeTimeText(g_tXsXtpLastRecvLimitRejectTime);
	sLastInvalidRejectTime = XS_RuntimeTimeText(g_tXsXtpLastInvalidRejectTime);
	sLastOtherRejectTime = XS_RuntimeTimeText(g_tXsXtpLastOtherRejectTime);
	snprintf(
		sBody,
		sizeof(sBody),
		"xtp_conn_current=%lld\nxtp_conn_peak=%lld\nxtp_open_count=%lld\nxtp_close_count=%lld\nxtp_error_count=%lld\nxtp_invalid_count=%lld\nxtp_msg_count=%lld\nxtp_req_count=%lld\nxtp_resp_count=%lld\nxtp_push_count=%lld\nxtp_event_count=%lld\nxtp_send_count=%lld\nxtp_recv_bytes=%lld\nxtp_send_bytes=%lld\nxtp_last_msg_type=%s\nxtp_last_status=%lld\nxtp_last_msg_id=%lld\nxtp_last_flags=%lld\nxtp_last_param_count=%lld\nxtp_last_body_size=%lld\nxtp_last_remote=%s\nxtp_last_bytes=%lld\nxtp_last_cmd=%s\nxtp_last_time=%s\nxtp_last_age_ms=%lld\nxtp_last_invalid_reason=%s\nxtp_last_invalid_remote=%s\nxtp_last_invalid_time=%s\nxtp_last_invalid_age_ms=%lld\nxtp_last_error_code=%lld\nxtp_last_error_remote=%s\nxtp_last_error_time=%s\nxtp_last_error_age_ms=%lld\nxtp_idle_close_count=%lld\nxtp_conn_limit_close_count=%lld\nxtp_recv_limit_close_count=%lld\nxtp_last_idle_close_time=%s\nxtp_last_idle_close_age_ms=%lld\nxtp_last_conn_limit_close_time=%s\nxtp_last_conn_limit_close_age_ms=%lld\nxtp_last_recv_limit_close_time=%s\nxtp_last_recv_limit_close_age_ms=%lld\n",
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
		g_sXsXtpLastInvalidRemote[0] ? g_sXsXtpLastInvalidRemote : "(none)",
		sLastInvalidTime ? sLastInvalidTime : "(none)",
		(long long)XS_XtpLastInvalidAgeMS(),
		(long long)g_iXsXtpLastErrorCode,
		g_sXsXtpLastErrorRemote[0] ? g_sXsXtpLastErrorRemote : "(none)",
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
			"xtp_reject_count=%lld\nxtp_last_reject_reason=%s\nxtp_last_reject_remote=%s\nxtp_last_reject_time=%s\nxtp_last_reject_age_ms=%lld\nxtp_server_stopping_reject_count=%lld\nxtp_last_server_stopping_reject_time=%s\nxtp_last_server_stopping_reject_age_ms=%lld\nxtp_conn_limit_reject_count=%lld\nxtp_last_conn_limit_reject_time=%s\nxtp_last_conn_limit_reject_age_ms=%lld\nxtp_recv_limit_reject_count=%lld\nxtp_last_recv_limit_reject_time=%s\nxtp_last_recv_limit_reject_age_ms=%lld\nxtp_invalid_reject_count=%lld\nxtp_last_invalid_reject_time=%s\nxtp_last_invalid_reject_age_ms=%lld\nxtp_other_reject_count=%lld\nxtp_last_other_reject_time=%s\nxtp_last_other_reject_age_ms=%lld\nxtp_stop_cleanup_count=%lld\nxtp_last_stop_cleanup_closed=%lld\nxtp_last_stop_cleanup_remain=%lld\nxtp_last_stop_cleanup_time=%s\nxtp_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsXtpRejectCount),
			g_sXsXtpLastRejectReason[0] ? g_sXsXtpLastRejectReason : "(none)",
			g_sXsXtpLastRejectRemote[0] ? g_sXsXtpLastRejectRemote : "(none)",
			sLastRejectTime ? sLastRejectTime : "(none)",
			(long long)XS_XtpLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpServerStoppingRejectCount),
			sLastServerStoppingRejectTime ? sLastServerStoppingRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsXtpLastServerStoppingRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsXtpConnLimitRejectCount),
			sLastConnLimitRejectTime ? sLastConnLimitRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsXtpLastConnLimitRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsXtpRecvLimitRejectCount),
			sLastRecvLimitRejectTime ? sLastRecvLimitRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsXtpLastRecvLimitRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsXtpInvalidRejectCount),
			sLastInvalidRejectTime ? sLastInvalidRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsXtpLastInvalidRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsXtpOtherRejectCount),
			sLastOtherRejectTime ? sLastOtherRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsXtpLastOtherRejectTime),
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
	if ( sLastServerStoppingRejectTime ) {
		xrtFree(sLastServerStoppingRejectTime);
	}
	if ( sLastConnLimitRejectTime ) {
		xrtFree(sLastConnLimitRejectTime);
	}
	if ( sLastRecvLimitRejectTime ) {
		xrtFree(sLastRecvLimitRejectTime);
	}
	if ( sLastInvalidRejectTime ) {
		xrtFree(sLastInvalidRejectTime);
	}
	if ( sLastOtherRejectTime ) {
		xrtFree(sLastOtherRejectTime);
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
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
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
	xvoTableSetText(objRet, "xtp_last_invalid_remote", 23, (ptr)g_sXsXtpLastInvalidRemote, 0, FALSE);
	xvoTableSetText(objRet, "xtp_last_invalid_time", 21, (ptr)(sLastInvalidTime ? sLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "xtp_last_invalid_age_ms", 23, XS_XtpLastInvalidAgeMS());
	xvoTableSetInt(objRet, "xtp_last_error_code", 19, g_iXsXtpLastErrorCode);
	xvoTableSetText(objRet, "xtp_last_error_remote", 21, (ptr)g_sXsXtpLastErrorRemote, 0, FALSE);
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
	char sBody[3072];

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/xtp_metrics_clear") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "xtp metrics clear api disabled");
	}
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
	}

	XS_XtpClearMetrics();
	snprintf(
		sBody,
		sizeof(sBody),
		"xtp_conn_current=0\nxtp_conn_peak=0\nxtp_open_count=0\nxtp_close_count=0\nxtp_error_count=0\nxtp_invalid_count=0\nxtp_msg_count=0\nxtp_req_count=0\nxtp_resp_count=0\nxtp_push_count=0\nxtp_event_count=0\nxtp_send_count=0\nxtp_recv_bytes=0\nxtp_send_bytes=0\nxtp_last_msg_type=(none)\nxtp_last_status=0\nxtp_last_msg_id=0\nxtp_last_flags=0\nxtp_last_param_count=0\nxtp_last_body_size=0\nxtp_last_remote=(none)\nxtp_last_bytes=0\nxtp_last_cmd=(none)\nxtp_last_time=(none)\nxtp_last_age_ms=-1\nxtp_last_invalid_reason=(none)\nxtp_last_invalid_remote=(none)\nxtp_last_invalid_time=(none)\nxtp_last_invalid_age_ms=-1\nxtp_last_error_code=0\nxtp_last_error_remote=(none)\nxtp_last_error_time=(none)\nxtp_last_error_age_ms=-1\nxtp_idle_close_count=0\nxtp_conn_limit_close_count=0\nxtp_recv_limit_close_count=0\nxtp_reject_count=0\nxtp_stop_cleanup_count=0\nxtp_last_reject_reason=(none)\nxtp_last_reject_remote=(none)\nxtp_last_reject_time=(none)\nxtp_last_reject_age_ms=-1\nxtp_server_stopping_reject_count=0\nxtp_last_server_stopping_reject_time=(none)\nxtp_last_server_stopping_reject_age_ms=-1\nxtp_conn_limit_reject_count=0\nxtp_last_conn_limit_reject_time=(none)\nxtp_last_conn_limit_reject_age_ms=-1\nxtp_recv_limit_reject_count=0\nxtp_last_recv_limit_reject_time=(none)\nxtp_last_recv_limit_reject_age_ms=-1\nxtp_invalid_reject_count=0\nxtp_last_invalid_reject_time=(none)\nxtp_last_invalid_reject_age_ms=-1\nxtp_other_reject_count=0\nxtp_last_other_reject_time=(none)\nxtp_last_other_reject_age_ms=-1\nxtp_last_stop_cleanup_closed=0\nxtp_last_stop_cleanup_remain=0\nxtp_last_stop_cleanup_time=(none)\nxtp_last_stop_cleanup_age_ms=-1\nxtp_last_idle_close_time=(none)\nxtp_last_idle_close_age_ms=-1\nxtp_last_conn_limit_close_time=(none)\nxtp_last_conn_limit_close_age_ms=-1\nxtp_last_recv_limit_close_time=(none)\nxtp_last_recv_limit_close_age_ms=-1\n"
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleUdpMetrics(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[1536];
	char* sLastTime;
	char* sLastErrorTime;
	char* sLastRejectTime;
	char* sLastRecvLimitRejectTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/udp_metrics") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "udp metrics api disabled");
	}
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
	}

	sLastTime = XS_UdpLastTimeText();
	sLastErrorTime = XS_UdpLastErrorTimeText();
	sLastRejectTime = XS_UdpLastRejectTimeText();
	sLastRecvLimitRejectTime = XS_UdpLastRecvLimitRejectTimeText();
	snprintf(
		sBody,
		sizeof(sBody),
		"udp_recv_count=%lld\nudp_send_count=%lld\nudp_error_count=%lld\nudp_reject_count=%lld\nudp_recv_limit_reject_count=%lld\nudp_last_error_code=%lld\nudp_recv_bytes=%lld\nudp_send_bytes=%lld\nudp_last_from=%s\nudp_last_text=%s\nudp_last_bytes=%lld\nudp_last_time=%s\nudp_last_age_ms=%lld\nudp_last_error_time=%s\nudp_last_error_age_ms=%lld\nudp_last_reject_reason=%s\nudp_last_reject_from=%s\nudp_last_reject_bytes=%lld\nudp_last_reject_time=%s\nudp_last_reject_age_ms=%lld\nudp_last_recv_limit_reject_time=%s\nudp_last_recv_limit_reject_age_ms=%lld\n",
		(long long)XS_HttpMetricGet(&g_iXsUdpRecvCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpSendCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpErrorCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpRejectCount),
		(long long)XS_HttpMetricGet(&g_iXsUdpRecvLimitRejectCount),
		(long long)g_iXsUdpLastErrorCode,
		(long long)XS_HttpMetricGet(&g_iXsUdpRecvBytes),
		(long long)XS_HttpMetricGet(&g_iXsUdpSendBytes),
		g_sXsUdpLastFrom[0] ? g_sXsUdpLastFrom : "(none)",
		g_sXsUdpLastText[0] ? g_sXsUdpLastText : "(none)",
		(long long)g_iXsUdpLastBytes,
		sLastTime ? sLastTime : "(none)",
		(long long)XS_UdpLastAgeMS(),
		sLastErrorTime ? sLastErrorTime : "(none)",
		(long long)XS_UdpLastErrorAgeMS(),
		g_sXsUdpLastRejectReason[0] ? g_sXsUdpLastRejectReason : "(none)",
		g_sXsUdpLastRejectFrom[0] ? g_sXsUdpLastRejectFrom : "(none)",
		(long long)g_iXsUdpLastRejectBytes,
		sLastRejectTime ? sLastRejectTime : "(none)",
		(long long)XS_UdpLastRejectAgeMS(),
		sLastRecvLimitRejectTime ? sLastRecvLimitRejectTime : "(none)",
		(long long)XS_UdpLastRecvLimitRejectAgeMS()
	);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	if ( sLastRejectTime ) {
		xrtFree(sLastRejectTime);
	}
	if ( sLastRecvLimitRejectTime ) {
		xrtFree(sLastRecvLimitRejectTime);
	}
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleUdpMetricsJson(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	xvalue objRet;
	char* sLastTime;
	char* sLastErrorTime;
	char* sLastRejectTime;
	char* sLastRecvLimitRejectTime;
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
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
	}

	sLastTime = XS_UdpLastTimeText();
	sLastErrorTime = XS_UdpLastErrorTimeText();
	sLastRejectTime = XS_UdpLastRejectTimeText();
	sLastRecvLimitRejectTime = XS_UdpLastRecvLimitRejectTimeText();
	objRet = xvoCreateTable();
	xvoTableSetInt(objRet, "udp_recv_count", sizeof("udp_recv_count") - 1, XS_HttpMetricGet(&g_iXsUdpRecvCount));
	xvoTableSetInt(objRet, "udp_send_count", sizeof("udp_send_count") - 1, XS_HttpMetricGet(&g_iXsUdpSendCount));
	xvoTableSetInt(objRet, "udp_error_count", sizeof("udp_error_count") - 1, XS_HttpMetricGet(&g_iXsUdpErrorCount));
	xvoTableSetInt(objRet, "udp_reject_count", sizeof("udp_reject_count") - 1, XS_HttpMetricGet(&g_iXsUdpRejectCount));
	xvoTableSetInt(objRet, "udp_recv_limit_reject_count", sizeof("udp_recv_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsUdpRecvLimitRejectCount));
	xvoTableSetInt(objRet, "udp_last_error_code", sizeof("udp_last_error_code") - 1, g_iXsUdpLastErrorCode);
	xvoTableSetInt(objRet, "udp_recv_bytes", sizeof("udp_recv_bytes") - 1, XS_HttpMetricGet(&g_iXsUdpRecvBytes));
	xvoTableSetInt(objRet, "udp_send_bytes", sizeof("udp_send_bytes") - 1, XS_HttpMetricGet(&g_iXsUdpSendBytes));
	xvoTableSetText(objRet, "udp_last_from", sizeof("udp_last_from") - 1, (ptr)g_sXsUdpLastFrom, 0, FALSE);
	xvoTableSetText(objRet, "udp_last_text", sizeof("udp_last_text") - 1, (ptr)g_sXsUdpLastText, 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_bytes", sizeof("udp_last_bytes") - 1, g_iXsUdpLastBytes);
	xvoTableSetText(objRet, "udp_last_time", sizeof("udp_last_time") - 1, (ptr)(sLastTime ? sLastTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_age_ms", sizeof("udp_last_age_ms") - 1, XS_UdpLastAgeMS());
	xvoTableSetText(objRet, "udp_last_error_time", sizeof("udp_last_error_time") - 1, (ptr)(sLastErrorTime ? sLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_error_age_ms", sizeof("udp_last_error_age_ms") - 1, XS_UdpLastErrorAgeMS());
	xvoTableSetText(objRet, "udp_last_reject_reason", sizeof("udp_last_reject_reason") - 1, (ptr)g_sXsUdpLastRejectReason, 0, FALSE);
	xvoTableSetText(objRet, "udp_last_reject_from", sizeof("udp_last_reject_from") - 1, (ptr)g_sXsUdpLastRejectFrom, 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_reject_bytes", sizeof("udp_last_reject_bytes") - 1, g_iXsUdpLastRejectBytes);
	xvoTableSetText(objRet, "udp_last_reject_time", sizeof("udp_last_reject_time") - 1, (ptr)(sLastRejectTime ? sLastRejectTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_reject_age_ms", sizeof("udp_last_reject_age_ms") - 1, XS_UdpLastRejectAgeMS());
	xvoTableSetText(objRet, "udp_last_recv_limit_reject_time", sizeof("udp_last_recv_limit_reject_time") - 1, (ptr)(sLastRecvLimitRejectTime ? sLastRecvLimitRejectTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "udp_last_recv_limit_reject_age_ms", sizeof("udp_last_recv_limit_reject_age_ms") - 1, XS_UdpLastRecvLimitRejectAgeMS());

	sJson = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sLastTime ) {
		xrtFree(sLastTime);
	}
	if ( sLastErrorTime ) {
		xrtFree(sLastErrorTime);
	}
	if ( sLastRejectTime ) {
		xrtFree(sLastRejectTime);
	}
	if ( sLastRecvLimitRejectTime ) {
		xrtFree(sLastRecvLimitRejectTime);
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
	char sBody[768];

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/udp_metrics_clear") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "udp metrics clear api disabled");
	}
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
	}

	XS_UdpClearMetrics();
	snprintf(
		sBody,
		sizeof(sBody),
		"udp_recv_count=0\nudp_send_count=0\nudp_error_count=0\nudp_reject_count=0\nudp_recv_limit_reject_count=0\nudp_last_error_code=0\nudp_recv_bytes=0\nudp_send_bytes=0\nudp_last_from=(none)\nudp_last_text=(none)\nudp_last_bytes=0\nudp_last_time=(none)\nudp_last_age_ms=-1\nudp_last_error_time=(none)\nudp_last_error_age_ms=-1\nudp_last_reject_reason=(none)\nudp_last_reject_from=(none)\nudp_last_reject_bytes=0\nudp_last_reject_time=(none)\nudp_last_reject_age_ms=-1\nudp_last_recv_limit_reject_time=(none)\nudp_last_recv_limit_reject_age_ms=-1\n"
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

static inline bool XS_HttpHandleCustomMetrics(XS_ServerConfig* objServer, const XS_HostConfig* objHost, const xhttpdrequest* pReq, xhttpdresponse* pResp)
{
	char sBody[3072];
	char* sLastTime;
	char* sLastCloseTime;
	char* sLastErrorTime;
	char* sLastInvalidTime;
	char* sLastIdleCloseTime;
	char* sLastConnLimitCloseTime;
	char* sLastRecvLimitCloseTime;
	char* sLastRejectTime;
	char* sLastStopCleanupTime;
	char* sLastServerStoppingRejectTime;
	char* sLastConnLimitRejectTime;
	char* sLastRecvLimitRejectTime;
	char* sLastInvalidRejectTime;
	char* sLastOtherRejectTime;

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/custom_metrics") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "custom metrics api disabled");
	}
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
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
	sLastServerStoppingRejectTime = XS_RuntimeTimeText(g_tXsCustomLastServerStoppingRejectTime);
	sLastConnLimitRejectTime = XS_RuntimeTimeText(g_tXsCustomLastConnLimitRejectTime);
	sLastRecvLimitRejectTime = XS_RuntimeTimeText(g_tXsCustomLastRecvLimitRejectTime);
	sLastInvalidRejectTime = XS_RuntimeTimeText(g_tXsCustomLastInvalidRejectTime);
	sLastOtherRejectTime = XS_RuntimeTimeText(g_tXsCustomLastOtherRejectTime);
	snprintf(
		sBody,
		sizeof(sBody),
		"custom_conn_current=%lld\ncustom_conn_peak=%lld\ncustom_open_count=%lld\ncustom_close_count=%lld\ncustom_error_count=%lld\ncustom_invalid_count=%lld\ncustom_last_invalid_reason=%s\ncustom_last_invalid_remote=%s\ncustom_last_invalid_time=%s\ncustom_last_invalid_age_ms=%lld\ncustom_last_close_reason=%lld\ncustom_last_close_time=%s\ncustom_last_close_age_ms=%lld\ncustom_last_error_code=%lld\ncustom_last_error_remote=%s\ncustom_last_error_time=%s\ncustom_last_error_age_ms=%lld\ncustom_recv_count=%lld\ncustom_send_count=%lld\ncustom_recv_bytes=%lld\ncustom_send_bytes=%lld\ncustom_last_remote=%s\ncustom_last_bytes=%lld\ncustom_last_text=%s\ncustom_last_time=%s\ncustom_last_age_ms=%lld\ncustom_idle_close_count=%lld\ncustom_conn_limit_close_count=%lld\ncustom_recv_limit_close_count=%lld\ncustom_last_idle_close_time=%s\ncustom_last_idle_close_age_ms=%lld\ncustom_last_conn_limit_close_time=%s\ncustom_last_conn_limit_close_age_ms=%lld\ncustom_last_recv_limit_close_time=%s\ncustom_last_recv_limit_close_age_ms=%lld\n",
		(long long)XS_HttpMetricGet(&g_iXsCustomConnCurrent),
		(long long)XS_HttpMetricGet(&g_iXsCustomConnPeak),
		(long long)XS_HttpMetricGet(&g_iXsCustomOpenCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomCloseCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomErrorCount),
		(long long)XS_HttpMetricGet(&g_iXsCustomInvalidCount),
		g_sXsCustomLastInvalidReason[0] ? g_sXsCustomLastInvalidReason : "(none)",
		g_sXsCustomLastInvalidRemote[0] ? g_sXsCustomLastInvalidRemote : "(none)",
		sLastInvalidTime ? sLastInvalidTime : "(none)",
		(long long)XS_CustomLastInvalidAgeMS(),
		(long long)g_iXsCustomLastCloseReason,
		sLastCloseTime ? sLastCloseTime : "(none)",
		(long long)XS_CustomLastCloseAgeMS(),
		(long long)g_iXsCustomLastErrorCode,
		g_sXsCustomLastErrorRemote[0] ? g_sXsCustomLastErrorRemote : "(none)",
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
			"custom_reject_count=%lld\ncustom_last_reject_reason=%s\ncustom_last_reject_remote=%s\ncustom_last_reject_time=%s\ncustom_last_reject_age_ms=%lld\ncustom_server_stopping_reject_count=%lld\ncustom_last_server_stopping_reject_time=%s\ncustom_last_server_stopping_reject_age_ms=%lld\ncustom_conn_limit_reject_count=%lld\ncustom_last_conn_limit_reject_time=%s\ncustom_last_conn_limit_reject_age_ms=%lld\ncustom_recv_limit_reject_count=%lld\ncustom_last_recv_limit_reject_time=%s\ncustom_last_recv_limit_reject_age_ms=%lld\ncustom_invalid_reject_count=%lld\ncustom_last_invalid_reject_time=%s\ncustom_last_invalid_reject_age_ms=%lld\ncustom_other_reject_count=%lld\ncustom_last_other_reject_time=%s\ncustom_last_other_reject_age_ms=%lld\ncustom_stop_cleanup_count=%lld\ncustom_last_stop_cleanup_closed=%lld\ncustom_last_stop_cleanup_remain=%lld\ncustom_last_stop_cleanup_time=%s\ncustom_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsCustomRejectCount),
			g_sXsCustomLastRejectReason[0] ? g_sXsCustomLastRejectReason : "(none)",
			g_sXsCustomLastRejectRemote[0] ? g_sXsCustomLastRejectRemote : "(none)",
			sLastRejectTime ? sLastRejectTime : "(none)",
			(long long)XS_CustomLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomServerStoppingRejectCount),
			sLastServerStoppingRejectTime ? sLastServerStoppingRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsCustomLastServerStoppingRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsCustomConnLimitRejectCount),
			sLastConnLimitRejectTime ? sLastConnLimitRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsCustomLastConnLimitRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsCustomRecvLimitRejectCount),
			sLastRecvLimitRejectTime ? sLastRecvLimitRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsCustomLastRecvLimitRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsCustomInvalidRejectCount),
			sLastInvalidRejectTime ? sLastInvalidRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsCustomLastInvalidRejectTime),
			(long long)XS_HttpMetricGet(&g_iXsCustomOtherRejectCount),
			sLastOtherRejectTime ? sLastOtherRejectTime : "(none)",
			(long long)XS_RuntimeAgeMS(g_tXsCustomLastOtherRejectTime),
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
	if ( sLastServerStoppingRejectTime ) {
		xrtFree(sLastServerStoppingRejectTime);
	}
	if ( sLastConnLimitRejectTime ) {
		xrtFree(sLastConnLimitRejectTime);
	}
	if ( sLastRecvLimitRejectTime ) {
		xrtFree(sLastRecvLimitRejectTime);
	}
	if ( sLastInvalidRejectTime ) {
		xrtFree(sLastInvalidRejectTime);
	}
	if ( sLastOtherRejectTime ) {
		xrtFree(sLastOtherRejectTime);
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
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
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
	xvoTableSetText(objRet, "custom_last_invalid_remote", 26, (ptr)g_sXsCustomLastInvalidRemote, 0, FALSE);
	xvoTableSetText(objRet, "custom_last_invalid_time", 24, (ptr)(sLastInvalidTime ? sLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_invalid_age_ms", 26, XS_CustomLastInvalidAgeMS());
	xvoTableSetInt(objRet, "custom_last_close_reason", 24, g_iXsCustomLastCloseReason);
	xvoTableSetText(objRet, "custom_last_close_time", 22, (ptr)(sLastCloseTime ? sLastCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objRet, "custom_last_close_age_ms", 24, XS_CustomLastCloseAgeMS());
	xvoTableSetInt(objRet, "custom_last_error_code", 22, g_iXsCustomLastErrorCode);
	xvoTableSetText(objRet, "custom_last_error_remote", 24, (ptr)g_sXsCustomLastErrorRemote, 0, FALSE);
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
	char sBody[2048];

	if ( pReq == NULL || pResp == NULL || objServer == NULL || objHost == NULL ) {
		return FALSE;
	}
	if ( strcmp(pReq->sPath, "/__xs/custom_metrics_clear") != 0 ) {
		return FALSE;
	}
	if ( !XS_ManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondText(pResp, 403, "Forbidden", "custom metrics clear api disabled");
	}
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
	}

	XS_CustomClearMetrics();
	snprintf(
		sBody,
		sizeof(sBody),
		"custom_conn_current=0\ncustom_conn_peak=0\ncustom_open_count=0\ncustom_close_count=0\ncustom_error_count=0\ncustom_invalid_count=0\ncustom_last_invalid_reason=(none)\ncustom_last_invalid_remote=(none)\ncustom_last_invalid_time=(none)\ncustom_last_invalid_age_ms=-1\ncustom_last_close_reason=0\ncustom_last_close_time=(none)\ncustom_last_close_age_ms=-1\ncustom_last_error_code=0\ncustom_last_error_remote=(none)\ncustom_last_error_time=(none)\ncustom_last_error_age_ms=-1\ncustom_recv_count=0\ncustom_send_count=0\ncustom_recv_bytes=0\ncustom_send_bytes=0\ncustom_last_remote=(none)\ncustom_last_bytes=0\ncustom_last_text=(none)\ncustom_last_time=(none)\ncustom_last_age_ms=-1\ncustom_idle_close_count=0\ncustom_conn_limit_close_count=0\ncustom_recv_limit_close_count=0\ncustom_reject_count=0\ncustom_stop_cleanup_count=0\ncustom_last_reject_reason=(none)\ncustom_last_reject_remote=(none)\ncustom_last_reject_time=(none)\ncustom_last_reject_age_ms=-1\ncustom_server_stopping_reject_count=0\ncustom_last_server_stopping_reject_time=(none)\ncustom_last_server_stopping_reject_age_ms=-1\ncustom_conn_limit_reject_count=0\ncustom_last_conn_limit_reject_time=(none)\ncustom_last_conn_limit_reject_age_ms=-1\ncustom_recv_limit_reject_count=0\ncustom_last_recv_limit_reject_time=(none)\ncustom_last_recv_limit_reject_age_ms=-1\ncustom_invalid_reject_count=0\ncustom_last_invalid_reject_time=(none)\ncustom_last_invalid_reject_age_ms=-1\ncustom_other_reject_count=0\ncustom_last_other_reject_time=(none)\ncustom_last_other_reject_age_ms=-1\ncustom_last_stop_cleanup_closed=0\ncustom_last_stop_cleanup_remain=0\ncustom_last_stop_cleanup_time=(none)\ncustom_last_stop_cleanup_age_ms=-1\ncustom_last_idle_close_time=(none)\ncustom_last_idle_close_age_ms=-1\ncustom_last_conn_limit_close_time=(none)\ncustom_last_conn_limit_close_age_ms=-1\ncustom_last_recv_limit_close_time=(none)\ncustom_last_recv_limit_close_age_ms=-1\n"
	);
	return XS_HttpRespondText(pResp, 200, "OK", sBody);
}

#endif
