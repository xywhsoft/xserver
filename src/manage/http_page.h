#ifndef XS_MANAGE_HTTP_PAGE_H
#define XS_MANAGE_HTTP_PAGE_H

static inline bool XS_HttpIsHeadRequest(const xhttpdrequest* pReq)
{
	if ( pReq == NULL || pReq->sMethod == NULL ) {
		return FALSE;
	}

	return _stricmp(pReq->sMethod, "HEAD") == 0;
}

static inline bool XS_HttpRespondBodyEx(
	const xhttpdrequest* pReq,
	xhttpdresponse* pResp,
	uint32 iStatus,
	const char* sReason,
	const void* pData,
	size_t iLen,
	const char* sContentType
)
{
	char sLength[32];

	if ( pResp == NULL || sReason == NULL || sContentType == NULL ) {
		return FALSE;
	}

	xrtHttpdResponseSetStatus(pResp, iStatus, sReason);
	if ( XS_HttpIsHeadRequest(pReq) ) {
		snprintf(sLength, sizeof(sLength), "%llu", (unsigned long long)iLen);
		if ( !xrtHttpdResponseSetHeader(pResp, "Content-Type", sContentType) ) {
			return FALSE;
		}
		if ( !xrtHttpdResponseSetHeader(pResp, "Content-Length", sLength) ) {
			return FALSE;
		}
		return TRUE;
	}

	return xrtHttpdResponseSetBodyCopy(pResp, pData, iLen, sContentType);
}

static inline bool XS_HttpRespondPlainTextEx(const xhttpdrequest* pReq, xhttpdresponse* pResp, uint32 iStatus, const char* sReason, const char* sText)
{
	const char* sBody = sText ? sText : "";
	return XS_HttpRespondBodyEx(pReq, pResp, iStatus, sReason, sBody, strlen(sBody), "text/plain; charset=utf-8");
}

static inline bool XS_HttpIsTemplatePage(const char* sFilePath)
{
	const char* sExt;

	if ( sFilePath == NULL || sFilePath[0] == '\0' ) {
		return FALSE;
	}

	sExt = strrchr(sFilePath, '.');
	if ( sExt == NULL ) {
		return FALSE;
	}

	return _stricmp(sExt, ".xtl") == 0;
}

static inline const char* XS_HttpSelectErrorPageFile(const XS_HttpPageConfig* objPages, uint32 iStatus)
{
	if ( objPages == NULL ) {
		return NULL;
	}

	switch ( iStatus ) {
		case 403:
			if ( objPages->Page403 && objPages->Page403[0] ) {
				return objPages->Page403;
			}
			break;
		case 404:
			if ( objPages->Page404 && objPages->Page404[0] ) {
				return objPages->Page404;
			}
			break;
		case 500:
			if ( objPages->Page500 && objPages->Page500[0] ) {
				return objPages->Page500;
			}
			break;
		default:
			break;
	}

	if ( objPages->ErrorPage && objPages->ErrorPage[0] ) {
		return objPages->ErrorPage;
	}

	return NULL;
}

static inline bool XS_HttpRenderDataSetText(xvalue objRender, const char* sKey, const char* sText)
{
	return xvoTableSetText(objRender, sKey, 0, (ptr)(sText ? sText : ""), 0, FALSE);
}

static inline xvalue XS_HttpBuildErrorPageData(
	XS_ServerConfig* objServer,
	const XS_HostConfig* objHost,
	const xhttpdrequest* pReq,
	const char* sRemote,
	uint32 iStatus,
	const char* sReason,
	const char* sMessage,
	xvalue objData
)
{
	xvalue objRender = xvoCreateTable();
	const char* sRequestHost = "";

	if ( objRender == NULL ) {
		return NULL;
	}

	if ( pReq ) {
		const char* sHeaderHost = xrtHttpdRequestHeader(pReq, "Host");
		if ( sHeaderHost && sHeaderHost[0] ) {
			sRequestHost = sHeaderHost;
		}
	}

	if ( !xvoTableSetInt(objRender, "status", 0, (int64)iStatus) ) goto fail;
	if ( !xvoTableSetInt(objRender, "status_code", 0, (int64)iStatus) ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "reason", sReason) ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "status_text", sReason) ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "message", sMessage) ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "server_name", objServer && objServer->Name ? objServer->Name : "") ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "server_class", objServer ? XS_ServerClassName(objServer->Class) : "") ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "server_param", objServer && objServer->Param ? objServer->Param : "") ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "host_name", objHost && objHost->Name ? objHost->Name : "") ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "host", objHost && objHost->Host ? objHost->Host : "") ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "host_param", objHost && objHost->Param ? objHost->Param : "") ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "host_path", objHost && objHost->Path ? objHost->Path : "") ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "request_host", sRequestHost) ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "method", pReq && pReq->sMethod ? pReq->sMethod : "") ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "target", pReq && pReq->sTarget ? pReq->sTarget : "") ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "path", pReq && pReq->sPath ? pReq->sPath : "") ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "query", pReq && pReq->sQuery ? pReq->sQuery : "") ) goto fail;
	if ( !XS_HttpRenderDataSetText(objRender, "remote", sRemote) ) goto fail;

	if ( objData ) {
		if ( !xvoTableSetValue(objRender, "data", 0, objData, FALSE) ) goto fail;
		if ( objData->Type == XVO_DT_TABLE ) {
			if ( !xvoTableMerge(objRender, objData, TRUE) ) goto fail;
		}
	}

	return objRender;

fail:
	xvoUnref(objRender);
	return NULL;
}

static inline bool XS_HttpRespondErrorPageFile(
	XS_ServerConfig* objServer,
	const XS_HostConfig* objHost,
	const xhttpdrequest* pReq,
	xhttpdresponse* pResp,
	const char* sRemote,
	uint32 iStatus,
	const char* sReason,
	const char* sMessage,
	const char* sFilePath,
	xvalue objData
)
{
	ptr pFileData;
	size_t iFileSize;

	if ( sFilePath == NULL || sFilePath[0] == '\0' ) {
		return FALSE;
	}
	if ( !xrtFileExists((char*)sFilePath) ) {
		XS_LogWarn("http error page file not found: %s", sFilePath);
		return FALSE;
	}

	pFileData = xrtFileGetAll((char*)sFilePath, &iFileSize);
	if ( pFileData == NULL ) {
		XS_LogWarn("http error page read failed: %s", sFilePath);
		return FALSE;
	}

	if ( XS_HttpIsTemplatePage(sFilePath) ) {
		xtetemplate hTemplate = xteParse((const char*)pFileData, iFileSize, NULL);
		xvalue objRender;
		char* sBody;
		size_t iBodyLen = 0;

		xrtFree(pFileData);
		if ( hTemplate == NULL ) {
			XS_LogWarn("http error page template parse failed: %s", sFilePath);
			return FALSE;
		}

		objRender = XS_HttpBuildErrorPageData(objServer, objHost, pReq, sRemote, iStatus, sReason, sMessage, objData);
		if ( objRender == NULL ) {
			xteDestroyTemplate(hTemplate);
			return FALSE;
		}

		sBody = xteMake(hTemplate, objRender, NULL, NULL, &iBodyLen);
		xvoUnref(objRender);
		xteDestroyTemplate(hTemplate);
		if ( sBody == NULL ) {
			XS_LogWarn("http error page template render failed: %s", sFilePath);
			return FALSE;
		}

		if ( !XS_HttpRespondBodyEx(pReq, pResp, iStatus, sReason, sBody, iBodyLen, "text/html; charset=utf-8") ) {
			xrtFree(sBody);
			return FALSE;
		}

		xrtFree(sBody);
		return TRUE;
	}

	if ( !XS_HttpRespondBodyEx(pReq, pResp, iStatus, sReason, pFileData, iFileSize, "text/html; charset=utf-8") ) {
		xrtFree(pFileData);
		return FALSE;
	}

	xrtFree(pFileData);
	return TRUE;
}

static inline bool XS_HttpTryConfiguredErrorPage(
	XS_ServerConfig* objServer,
	const XS_HostConfig* objHost,
	const xhttpdrequest* pReq,
	xhttpdresponse* pResp,
	const char* sRemote,
	uint32 iStatus,
	const char* sReason,
	const char* sMessage,
	xvalue objData
)
{
	const XS_HttpPageConfig* objPages = objHost ? &objHost->Pages : (objServer ? &objServer->Pages : NULL);
	const char* sPrimary;
	const char* sFallback;

	if ( objPages == NULL ) {
		return FALSE;
	}

	sPrimary = XS_HttpSelectErrorPageFile(objPages, iStatus);
	if ( sPrimary && sPrimary[0] ) {
		if ( XS_HttpRespondErrorPageFile(objServer, objHost, pReq, pResp, sRemote, iStatus, sReason, sMessage, sPrimary, objData) ) {
			return TRUE;
		}
	}

	sFallback = objPages->ErrorPage;
	if (
		sFallback &&
		sFallback[0] &&
		((sPrimary == NULL) || (_stricmp(sPrimary, sFallback) != 0))
	) {
		if ( XS_HttpRespondErrorPageFile(objServer, objHost, pReq, pResp, sRemote, iStatus, sReason, sMessage, sFallback, objData) ) {
			return TRUE;
		}
	}

	return FALSE;
}

static inline bool XS_HttpRetErrorEx(
	XS_ServerConfig* objServer,
	const XS_HostConfig* objHost,
	const xhttpdrequest* pReq,
	xhttpdresponse* pResp,
	const char* sRemote,
	uint32 iStatus,
	const char* sReason,
	const char* sMessage,
	xvalue objData
)
{
	const char* sBody;

	if ( pResp == NULL || sReason == NULL ) {
		return FALSE;
	}

	if ( XS_HttpTryConfiguredErrorPage(objServer, objHost, pReq, pResp, sRemote, iStatus, sReason, sMessage, objData) ) {
		return TRUE;
	}

	sBody = (sMessage && sMessage[0]) ? sMessage : sReason;
	return XS_HttpRespondPlainTextEx(pReq, pResp, iStatus, sReason, sBody);
}

static inline bool XS_HttpRet403Ex(
	XS_ServerConfig* objServer,
	const XS_HostConfig* objHost,
	const xhttpdrequest* pReq,
	xhttpdresponse* pResp,
	const char* sRemote,
	const char* sMessage,
	xvalue objData
)
{
	return XS_HttpRetErrorEx(objServer, objHost, pReq, pResp, sRemote, 403, "Forbidden", sMessage ? sMessage : "forbidden", objData);
}

static inline bool XS_HttpRet404Ex(
	XS_ServerConfig* objServer,
	const XS_HostConfig* objHost,
	const xhttpdrequest* pReq,
	xhttpdresponse* pResp,
	const char* sRemote,
	const char* sMessage,
	xvalue objData
)
{
	return XS_HttpRetErrorEx(objServer, objHost, pReq, pResp, sRemote, 404, "Not Found", sMessage ? sMessage : "not found", objData);
}

static inline bool XS_HttpRet500Ex(
	XS_ServerConfig* objServer,
	const XS_HostConfig* objHost,
	const xhttpdrequest* pReq,
	xhttpdresponse* pResp,
	const char* sRemote,
	const char* sMessage,
	xvalue objData
)
{
	return XS_HttpRetErrorEx(objServer, objHost, pReq, pResp, sRemote, 500, "Internal Server Error", sMessage ? sMessage : "internal server error", objData);
}

static inline char* XS_HttpResolveDefaultPagePath(const XS_HostConfig* objHost)
{
	if ( objHost == NULL ) {
		return NULL;
	}

	if ( objHost->Pages.DefaultPage && objHost->Pages.DefaultPage[0] ) {
		return xrtCopyStr(objHost->Pages.DefaultPage, 0);
	}
	if ( objHost->Path && objHost->Path[0] ) {
		return xrtPathJoin(2, objHost->Path, "index.html");
	}

	return NULL;
}

#endif
