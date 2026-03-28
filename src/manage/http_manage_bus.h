#ifndef XS_MANAGE_HTTP_MANAGE_BUS_H
#define XS_MANAGE_HTTP_MANAGE_BUS_H

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

#endif
