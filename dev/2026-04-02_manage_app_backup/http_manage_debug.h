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
	char* sUdpLastRejectTime;
	char* sUdpLastRecvLimitRejectTime;
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
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
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
	sUdpLastRejectTime = XS_UdpLastRejectTimeText();
	sUdpLastRecvLimitRejectTime = XS_UdpLastRecvLimitRejectTimeText();
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
	xvoTableSetText(objStatus, "ws_last_invalid_remote", 22, (ptr)g_sXsWsLastInvalidRemote, 0, FALSE);
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
	xvoTableSetText(objStatus, "ws_last_error_remote", 20, (ptr)g_sXsWsLastErrorRemote, 0, FALSE);
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
	xvoTableSetText(objStatus, "xtp_last_invalid_remote", 23, (ptr)g_sXsXtpLastInvalidRemote, 0, FALSE);
	xvoTableSetText(objStatus, "xtp_last_invalid_time", 21, (ptr)(sXtpLastInvalidTime ? sXtpLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_invalid_age_ms", 23, XS_XtpLastInvalidAgeMS());
	xvoTableSetInt(objStatus, "xtp_last_error_code", 19, g_iXsXtpLastErrorCode);
	xvoTableSetText(objStatus, "xtp_last_error_remote", 21, (ptr)g_sXsXtpLastErrorRemote, 0, FALSE);
	xvoTableSetText(objStatus, "xtp_last_error_time", 19, (ptr)(sXtpLastErrorTime ? sXtpLastErrorTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_error_age_ms", 21, XS_XtpLastErrorAgeMS());
	xvoTableSetText(objStatus, "xtp_last_idle_close_time", 24, (ptr)(sXtpLastIdleCloseTime ? sXtpLastIdleCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_idle_close_age_ms", 26, XS_XtpLastIdleCloseAgeMS());
	xvoTableSetText(objStatus, "xtp_last_conn_limit_close_time", 30, (ptr)(sXtpLastConnLimitCloseTime ? sXtpLastConnLimitCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "xtp_last_conn_limit_close_age_ms", 32, XS_XtpLastConnLimitCloseAgeMS());
	xvoTableSetInt(objStatus, "udp_recv_count", 14, XS_HttpMetricGet(&g_iXsUdpRecvCount));
	xvoTableSetInt(objStatus, "udp_send_count", 14, XS_HttpMetricGet(&g_iXsUdpSendCount));
	xvoTableSetInt(objStatus, "udp_error_count", 15, XS_HttpMetricGet(&g_iXsUdpErrorCount));
	xvoTableSetInt(objStatus, "udp_reject_count", sizeof("udp_reject_count") - 1, XS_HttpMetricGet(&g_iXsUdpRejectCount));
	xvoTableSetInt(objStatus, "udp_recv_limit_reject_count", sizeof("udp_recv_limit_reject_count") - 1, XS_HttpMetricGet(&g_iXsUdpRecvLimitRejectCount));
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
	xvoTableSetText(objStatus, "udp_last_reject_reason", sizeof("udp_last_reject_reason") - 1, (ptr)g_sXsUdpLastRejectReason, 0, FALSE);
	xvoTableSetText(objStatus, "udp_last_reject_from", sizeof("udp_last_reject_from") - 1, (ptr)g_sXsUdpLastRejectFrom, 0, FALSE);
	xvoTableSetInt(objStatus, "udp_last_reject_bytes", sizeof("udp_last_reject_bytes") - 1, g_iXsUdpLastRejectBytes);
	xvoTableSetText(objStatus, "udp_last_reject_time", sizeof("udp_last_reject_time") - 1, (ptr)(sUdpLastRejectTime ? sUdpLastRejectTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "udp_last_reject_age_ms", sizeof("udp_last_reject_age_ms") - 1, XS_UdpLastRejectAgeMS());
	xvoTableSetText(objStatus, "udp_last_recv_limit_reject_time", sizeof("udp_last_recv_limit_reject_time") - 1, (ptr)(sUdpLastRecvLimitRejectTime ? sUdpLastRecvLimitRejectTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "udp_last_recv_limit_reject_age_ms", sizeof("udp_last_recv_limit_reject_age_ms") - 1, XS_UdpLastRecvLimitRejectAgeMS());
	xvoTableSetInt(objStatus, "custom_conn_current", 19, XS_HttpMetricGet(&g_iXsCustomConnCurrent));
	xvoTableSetInt(objStatus, "custom_conn_peak", 16, XS_HttpMetricGet(&g_iXsCustomConnPeak));
	xvoTableSetInt(objStatus, "custom_open_count", 17, XS_HttpMetricGet(&g_iXsCustomOpenCount));
	xvoTableSetInt(objStatus, "custom_close_count", 18, XS_HttpMetricGet(&g_iXsCustomCloseCount));
	xvoTableSetInt(objStatus, "custom_error_count", 18, XS_HttpMetricGet(&g_iXsCustomErrorCount));
	xvoTableSetInt(objStatus, "custom_invalid_count", 20, XS_HttpMetricGet(&g_iXsCustomInvalidCount));
	xvoTableSetText(objStatus, "custom_last_invalid_reason", 26, (ptr)g_sXsCustomLastInvalidReason, 0, FALSE);
	xvoTableSetText(objStatus, "custom_last_invalid_remote", 26, (ptr)g_sXsCustomLastInvalidRemote, 0, FALSE);
	xvoTableSetText(objStatus, "custom_last_invalid_time", 24, (ptr)(sCustomLastInvalidTime ? sCustomLastInvalidTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "custom_last_invalid_age_ms", 26, XS_CustomLastInvalidAgeMS());
	xvoTableSetInt(objStatus, "custom_last_close_reason", 24, g_iXsCustomLastCloseReason);
	xvoTableSetText(objStatus, "custom_last_close_time", 22, (ptr)(sCustomLastCloseTime ? sCustomLastCloseTime : ""), 0, FALSE);
	xvoTableSetInt(objStatus, "custom_last_close_age_ms", 24, XS_CustomLastCloseAgeMS());
	xvoTableSetInt(objStatus, "custom_last_error_code", 22, g_iXsCustomLastErrorCode);
	xvoTableSetText(objStatus, "custom_last_error_remote", 24, (ptr)g_sXsCustomLastErrorRemote, 0, FALSE);
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
		if ( sUdpLastRejectTime ) {
			xrtFree(sUdpLastRejectTime);
		}
		if ( sUdpLastRecvLimitRejectTime ) {
			xrtFree(sUdpLastRecvLimitRejectTime);
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
	if ( !XS_DebugManageAPIEnabled(objServer, objHost) ) {
		return XS_HttpRespondDebugOnlyDisabled(pReq->sPath, pResp);
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
		"ok=%s\nserver=%s\nclass=%s\naddr=%s\nbind_ip=%s\nbind_port=%u\ntls=%s\nbind_ip_tls=%s\nbind_port_tls=%u\naddr_tls=%s\nidle_timeout=%u\nconn_limit=%u\nws_protocol=%s\nws_message_limit=%u\nws_conn_current=%lld\nws_conn_peak=%lld\nws_open_count=%lld\nws_close_count=%lld\nws_text_count=%lld\nws_binary_count=%lld\nws_ping_count=%lld\nws_pong_count=%lld\nws_error_count=%lld\nws_invalid_count=%lld\nws_idle_close_count=%lld\nws_conn_limit_close_count=%lld\nws_last_error_code=%lld\nws_last_invalid_reason=%s\nws_last_invalid_remote=%s\nws_last_invalid_time=%s\nws_last_invalid_age_ms=%lld\nws_last_close_reason=%lld\nws_last_close_time=%s\nws_last_close_age_ms=%lld\nws_last_idle_close_time=%s\nws_last_idle_close_age_ms=%lld\nws_last_conn_limit_close_time=%s\nws_last_conn_limit_close_age_ms=%lld\nws_last_frame_type=%s\nws_last_remote=%s\nws_last_bytes=%lld\nws_last_text=%s\nws_last_time=%s\nws_last_age_ms=%lld\nws_last_error_remote=%s\nws_last_error_time=%s\nws_last_error_age_ms=%lld\nxtp_conn_current=%lld\nxtp_conn_peak=%lld\nxtp_open_count=%lld\nxtp_close_count=%lld\nxtp_error_count=%lld\nxtp_invalid_count=%lld\nxtp_msg_count=%lld\nxtp_req_count=%lld\nxtp_resp_count=%lld\nxtp_push_count=%lld\nxtp_event_count=%lld\nxtp_send_count=%lld\nxtp_recv_bytes=%lld\nxtp_send_bytes=%lld\nxtp_last_msg_type=%s\nxtp_last_status=%lld\nxtp_last_msg_id=%lld\nxtp_last_flags=%lld\nxtp_last_param_count=%lld\nxtp_last_body_size=%lld\nxtp_last_remote=%s\nxtp_last_bytes=%lld\nxtp_last_cmd=%s\nxtp_last_time=%s\nxtp_last_age_ms=%lld\nxtp_last_invalid_reason=%s\nxtp_last_invalid_remote=%s\nxtp_last_invalid_time=%s\nxtp_last_invalid_age_ms=%lld\nxtp_last_error_code=%lld\nxtp_last_error_remote=%s\nxtp_last_error_time=%s\nxtp_last_error_age_ms=%lld\nxtp_idle_close_count=%lld\nxtp_conn_limit_close_count=%lld\nxtp_last_idle_close_time=%s\nxtp_last_idle_close_age_ms=%lld\nxtp_last_conn_limit_close_time=%s\nxtp_last_conn_limit_close_age_ms=%lld\nudp_recv_count=%lld\nudp_send_count=%lld\nudp_error_count=%lld\nudp_last_error_code=%lld\nudp_recv_bytes=%lld\nudp_send_bytes=%lld\nudp_last_from=%s\nudp_last_text=%s\nudp_last_bytes=%lld\nudp_last_time=%s\nudp_last_age_ms=%lld\nudp_last_error_time=%s\nudp_last_error_age_ms=%lld\ncustom_conn_current=%lld\ncustom_conn_peak=%lld\ncustom_open_count=%lld\ncustom_close_count=%lld\ncustom_error_count=%lld\ncustom_invalid_count=%lld\ncustom_last_invalid_reason=%s\ncustom_last_invalid_remote=%s\ncustom_last_invalid_time=%s\ncustom_last_invalid_age_ms=%lld\ncustom_last_close_reason=%lld\ncustom_last_close_time=%s\ncustom_last_close_age_ms=%lld\ncustom_last_error_code=%lld\ncustom_last_error_remote=%s\ncustom_last_error_time=%s\ncustom_last_error_age_ms=%lld\ncustom_recv_count=%lld\ncustom_send_count=%lld\ncustom_recv_bytes=%lld\ncustom_send_bytes=%lld\ncustom_last_remote=%s\ncustom_last_bytes=%lld\ncustom_last_text=%s\ncustom_last_time=%s\ncustom_last_age_ms=%lld\ncustom_idle_close_count=%lld\ncustom_conn_limit_close_count=%lld\ncustom_last_idle_close_time=%s\ncustom_last_idle_close_age_ms=%lld\ncustom_last_conn_limit_close_time=%s\ncustom_last_conn_limit_close_age_ms=%lld\ntls_cert_file=%s\ntls_key_file=%s\ntls_ca_file=%s\ncurrent_dir=%s\napp_file=%s\napp_mtime=%s\napp_size=%lld\napp_path=%s\nbuild=%s\ncompiler=%s\nplatform=%s\narch=%s\nmem_debug=%s\npid=%llu\nstart_time=%s\nuptime_ms=%lld\nengine_workers=%u\nruntime_server_count=%u\nhttp_req_count=%lld\nhttp_manage_req_count=%lld\nhttp_app_req_count=%lld\nhttp_2xx_count=%lld\nhttp_3xx_count=%lld\nhttp_4xx_count=%lld\nhttp_5xx_count=%lld\nhttp_last_method=%s\nhttp_last_status=%lld\nhttp_last_path=%s\nhttp_last_target=%s\nhttp_last_remote=%s\nhttp_last_body_len=%lld\nhttp_last_header_count=%lld\nhttp_last_query_len=%lld\nhttp_last_time=%s\nhttp_last_age_ms=%lld\nhttp_last_duration_ms=%lld\nhttp_idle_close_count=%lld\nhttp_conn_limit_close_count=%lld\nhttp_last_idle_close_time=%s\nhttp_last_idle_close_age_ms=%lld\nhttp_last_conn_limit_close_time=%s\nhttp_last_conn_limit_close_age_ms=%lld\nhttp_last_app_method=%s\nhttp_last_app_status=%lld\nhttp_last_app_path=%s\nhttp_last_app_target=%s\nhttp_last_app_remote=%s\nhttp_last_app_body_len=%lld\nhttp_last_app_header_count=%lld\nhttp_last_app_query_len=%lld\nhttp_last_app_time=%s\nhttp_last_app_age_ms=%lld\nhttp_last_app_duration_ms=%lld\nmanage_api=%s\ndebug=%s\nconfig_file=%s\nconfig_name=%s\nconfig_mtime=%s\nconfig_size=%lld\nconfig_base=%s\ncheck_result=%s\ncheck_file=%s\ncheck_base=%s\ncheck_server_count=%lld\ncheck_message=%s\ncheck_last_time=%s\ncheck_last_age_ms=%lld\nhost_aware=%s\ndefault_host=%s\nhost_count=%u\nscript_loaded=%s\nreload_busy=%s\nreload_has_result=%s\nreload_success=%s\nreload_server=%s\nreload_host=%s\nreload_message=%s\nbus_queue_count=%lld\nbus_data_count=%lld\nbus_total_queued=%lld\nbus_total_delivered=%lld\nbus_total_dropped=%lld\nbus_last_queue_time=%lld\nbus_last_queue_time_text=%s\nbus_last_dispatch_time=%lld\nbus_last_dispatch_time_text=%s\nbus_sweep_interval_ms=%lld\nbus_sweep_batch_limit=%lld\nbus_sweep_count=%lld\nbus_sweep_removed_count=%lld\nbus_last_sweep_time=%lld\nbus_last_sweep_time_text=%s\nbus_last_sweep_age_ms=%lld\nbus_last_sweep_removed=%lld\nbus_last_sweep_remain=%lld\nbus_cleanup_count=%lld\nbus_last_cleanup_time=%lld\nbus_last_cleanup_time_text=%s\nbus_last_cleanup_age_ms=%lld\nbus_last_cleanup_removed=%lld\nbus_last_cleanup_remain=%lld\n",
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
		g_sXsWsLastInvalidRemote[0] ? g_sXsWsLastInvalidRemote : "(none)",
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
		g_sXsWsLastErrorRemote[0] ? g_sXsWsLastErrorRemote : "(none)",
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
		g_sXsXtpLastInvalidRemote[0] ? g_sXsXtpLastInvalidRemote : "(none)",
		sXtpLastInvalidTime ? sXtpLastInvalidTime : "(none)",
		(long long)XS_XtpLastInvalidAgeMS(),
		(long long)g_iXsXtpLastErrorCode,
		g_sXsXtpLastErrorRemote[0] ? g_sXsXtpLastErrorRemote : "(none)",
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
		g_sXsCustomLastInvalidRemote[0] ? g_sXsCustomLastInvalidRemote : "(none)",
		sCustomLastInvalidTime ? sCustomLastInvalidTime : "(none)",
		(long long)XS_CustomLastInvalidAgeMS(),
		(long long)g_iXsCustomLastCloseReason,
		sCustomLastCloseTime ? sCustomLastCloseTime : "(none)",
		(long long)XS_CustomLastCloseAgeMS(),
		(long long)g_iXsCustomLastErrorCode,
		g_sXsCustomLastErrorRemote[0] ? g_sXsCustomLastErrorRemote : "(none)",
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
			"http_conn_current=%lld\nhttp_conn_peak=%lld\nhttp_idle_close_count=%lld\nhttp_last_idle_close_time=%s\nhttp_last_idle_close_age_ms=%lld\nhttp_reject_count=%lld\nhttp_last_reject_status=%lld\nhttp_last_reject_reason=%s\nhttp_last_reject_remote=%s\nhttp_last_reject_time=%s\nhttp_last_reject_age_ms=%lld\nhttp_stop_cleanup_count=%lld\nhttp_last_stop_cleanup_closed=%lld\nhttp_last_stop_cleanup_remain=%lld\nhttp_last_stop_cleanup_time=%s\nhttp_last_stop_cleanup_age_ms=%lld\nws_reject_count=%lld\nws_last_reject_reason=%s\nws_last_reject_remote=%s\nws_last_reject_time=%s\nws_last_reject_age_ms=%lld\nws_stop_cleanup_count=%lld\nws_last_stop_cleanup_closed=%lld\nws_last_stop_cleanup_remain=%lld\nws_last_stop_cleanup_time=%s\nws_last_stop_cleanup_age_ms=%lld\nxtp_reject_count=%lld\nxtp_last_reject_reason=%s\nxtp_last_reject_remote=%s\nxtp_last_reject_time=%s\nxtp_last_reject_age_ms=%lld\nxtp_stop_cleanup_count=%lld\nxtp_last_stop_cleanup_closed=%lld\nxtp_last_stop_cleanup_remain=%lld\nxtp_last_stop_cleanup_time=%s\nxtp_last_stop_cleanup_age_ms=%lld\ncustom_reject_count=%lld\ncustom_last_reject_reason=%s\ncustom_last_reject_remote=%s\ncustom_last_reject_time=%s\ncustom_last_reject_age_ms=%lld\ncustom_stop_cleanup_count=%lld\ncustom_last_stop_cleanup_closed=%lld\ncustom_last_stop_cleanup_remain=%lld\ncustom_last_stop_cleanup_time=%s\ncustom_last_stop_cleanup_age_ms=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsHttpConnCurrent),
			(long long)XS_HttpMetricGet(&g_iXsHttpConnPeak),
			(long long)XS_HttpMetricGet(&g_iXsHttpIdleCloseCount),
			sHttpLastIdleCloseTime ? sHttpLastIdleCloseTime : "(none)",
			(long long)XS_HttpLastIdleCloseAgeMS(),
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
			(long long)XS_HttpLastStopCleanupAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsWsRejectCount),
			g_sXsWsLastRejectReason[0] ? g_sXsWsLastRejectReason : "(none)",
			g_sXsWsLastRejectRemote[0] ? g_sXsWsLastRejectRemote : "(none)",
			sWsLastRejectTime ? sWsLastRejectTime : "(none)",
			(long long)XS_WsLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsWsStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsWsLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsWsLastStopCleanupRemain),
			sWsLastStopCleanupTime ? sWsLastStopCleanupTime : "(none)",
			(long long)XS_WsLastStopCleanupAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpRejectCount),
			g_sXsXtpLastRejectReason[0] ? g_sXsXtpLastRejectReason : "(none)",
			g_sXsXtpLastRejectRemote[0] ? g_sXsXtpLastRejectRemote : "(none)",
			sXtpLastRejectTime ? sXtpLastRejectTime : "(none)",
			(long long)XS_XtpLastRejectAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsXtpStopCleanupCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastStopCleanupClosed),
			(long long)XS_HttpMetricGet(&g_iXsXtpLastStopCleanupRemain),
			sXtpLastStopCleanupTime ? sXtpLastStopCleanupTime : "(none)",
			(long long)XS_XtpLastStopCleanupAgeMS(),
			(long long)XS_HttpMetricGet(&g_iXsCustomRejectCount),
			g_sXsCustomLastRejectReason[0] ? g_sXsCustomLastRejectReason : "(none)",
			g_sXsCustomLastRejectRemote[0] ? g_sXsCustomLastRejectRemote : "(none)",
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
			"ws_server_stopping_reject_count=%lld\nws_conn_limit_reject_count=%lld\nws_message_limit_reject_count=%lld\nws_invalid_reject_count=%lld\nws_other_reject_count=%lld\nxtp_server_stopping_reject_count=%lld\nxtp_conn_limit_reject_count=%lld\nxtp_recv_limit_reject_count=%lld\nxtp_invalid_reject_count=%lld\nxtp_other_reject_count=%lld\ncustom_server_stopping_reject_count=%lld\ncustom_conn_limit_reject_count=%lld\ncustom_recv_limit_reject_count=%lld\ncustom_invalid_reject_count=%lld\ncustom_other_reject_count=%lld\n",
			(long long)XS_HttpMetricGet(&g_iXsWsServerStoppingRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsWsConnLimitRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsWsMessageLimitRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsWsInvalidRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsWsOtherRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpServerStoppingRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpConnLimitRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpRecvLimitRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpInvalidRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsXtpOtherRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomServerStoppingRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomConnLimitRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomRecvLimitRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomInvalidRejectCount),
			(long long)XS_HttpMetricGet(&g_iXsCustomOtherRejectCount)
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

#endif
