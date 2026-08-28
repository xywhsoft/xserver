#include <xsbase.h>
#include <string.h>

#include "route_http_basic.h"

static int64_t g_iLastBusDataID = 0;
static char* g_sLastBusTopic = NULL;
static char* g_sLastBusPayload = NULL;

static bool XS_QueryGetParam(const char* sQuery, const char* sKey, char* sValue, size_t iValueSize)
{
	const char* pFind;
	size_t iKeyLen;
	size_t iPos = 0;
	
	if ( sValue == NULL || iValueSize == 0 ) {
		return FALSE;
	}
	
	sValue[0] = '\0';
	if ( sQuery == NULL || sKey == NULL || sKey[0] == '\0' ) {
		return FALSE;
	}
	
	iKeyLen = strlen(sKey);
	pFind = sQuery;
	while ( pFind && pFind[0] ) {
		if ( strncmp(pFind, sKey, iKeyLen) == 0 && pFind[iKeyLen] == '=' ) {
			pFind += iKeyLen + 1;
			while ( pFind[0] && pFind[0] != '&' && iPos < (iValueSize - 1) ) {
				sValue[iPos++] = pFind[0];
				pFind++;
			}
			sValue[iPos] = '\0';
			return TRUE;
		}
		
		pFind = strchr(pFind, '&');
		if ( pFind ) {
			pFind++;
		}
	}
	
	return FALSE;
}

static void XS_BusFreeLastMessage(void)
{
	if ( g_sLastBusTopic ) {
		xrtFree(g_sLastBusTopic);
		g_sLastBusTopic = NULL;
	}
	if ( g_sLastBusPayload ) {
		xrtFree(g_sLastBusPayload);
		g_sLastBusPayload = NULL;
	}
	g_iLastBusDataID = 0;
}

void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
	char sText[256];
	
	snprintf(
		sText,
		sizeof(sText),
		"init server=%s host=%s path=%s",
		xsServerName(objServer),
		xsHostName(objHost),
		xsHostPath(objHost)
	);
	xsLog(sText);
}

void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
	char sText[256];
	
	XS_DemoDB_Close();
	XS_BusFreeLastMessage();
	
	snprintf(
		sText,
		sizeof(sText),
		"unit server=%s host=%s",
		xsServerName(objServer),
		xsHostName(objHost)
	);
	xsLog(sText);
}

void ServiceStart(XS_ServerObject objServer, XS_HostObject objHost)
{
	char sText[256];
	
	snprintf(
		sText,
		sizeof(sText),
		"start server=%s host=%s",
		xsServerName(objServer),
		xsHostName(objHost)
	);
	xsLog(sText);
}

void ServiceStop(XS_ServerObject objServer, XS_HostObject objHost)
{
	char sText[256];
	
	snprintf(
		sText,
		sizeof(sText),
		"stop server=%s host=%s",
		xsServerName(objServer),
		xsHostName(objHost)
	);
	xsLog(sText);
}

bool MessageProc(XS_ServerObject objServer, XS_HostObject objHost, const char* sTopic, int64_t iDataID, xvalue objArgs)
{
	xvalue objData = xsDataGet(iDataID);
	char* sPayload = NULL;
	char sText[256];
	
	(void)objHost;
	(void)objArgs;
	
	XS_BusFreeLastMessage();
	if ( sTopic ) {
		g_sLastBusTopic = xrtCopyStr((str)sTopic, 0);
	}
	g_iLastBusDataID = iDataID;
	if ( objData ) {
		sPayload = xrtStringifyJSON(objData, FALSE, NULL);
		if ( sPayload ) {
			g_sLastBusPayload = sPayload;
		}
	}
	
	snprintf(
		sText,
		sizeof(sText),
		"message topic=%s data_id=%lld server=%s",
		sTopic ? sTopic : "",
		(long long)iDataID,
		xsServerName(objServer)
	);
	xsLog(sText);
	return true;
}

bool RequestProc(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	char sBody[512];
	const char* sPath = xsReqPath(objReq);
	const char* sQuery = xsReqQuery(objReq);
	
	if ( strcmp(sPath, "/index.html") == 0 ) {
		return false;
	}
	if ( strncmp(sPath, "/res/", 5) == 0 ) {
		return false;
	}
	if ( strcmp(sPath, "/json") == 0 ) {
		xvalue objJson = xvoCreateTable();
		char* sRet;
		
		xvoTableSetText(objJson, "server", 6, xsServerName(objServer), 0, FALSE);
		xvoTableSetText(objJson, "host", 4, xsHostName(objHost), 0, FALSE);
		xvoTableSetText(objJson, "path", 4, sPath, 0, FALSE);
		xvoTableSetText(objJson, "query", 5, sQuery ? sQuery : "", 0, FALSE);
		xvoTableSetText(objJson, "app_path", 8, xsAppPath(), 0, FALSE);
		xvoTableSetBool(objJson, "debug", 5, xsHostDebug(objHost));
		xvoTableSetInt(objJson, "class", 5, xsServerClass(objServer));
		xvoTableSetInt(objJson, "dev_mode", 8, xsHostDevMode(objHost));
		
		sRet = xrtStringifyJSON(objJson, FALSE, NULL);
		xvoUnref(objJson);
		if ( sRet == NULL ) {
			return xsHttpJson(objResp, 500, "Internal Server Error", "{\"result\":false}") != 0;
		}
		
		xsHttpHeader(objResp, "X-XS-Mode", "json");
		if ( xsHttpJson(objResp, 200, "OK", sRet) == 0 ) {
			xrtFree(sRet);
			return false;
		}
		
		xrtFree(sRet);
		return true;
	}
	if ( strcmp(sPath, "/stream") == 0 ) {
		if ( xsHttpStart(objResp, 200, "OK", "Content-Type: text/plain; charset=utf-8\r\nX-XS-Mode: stream\r\n") == 0 ) {
			return false;
		}
		if ( xsHttpSend(objResp, "stream-", 7) == 0 ) {
			return false;
		}
		if ( xsHttpSend(objResp, "chunk-", 6) == 0 ) {
			return false;
		}
		if ( xsHttpSend(objResp, "done", 4) == 0 ) {
			return false;
		}
		return xsHttpEnd(objResp) != 0;
	}
	if ( strcmp(sPath, "/bus/status") == 0 ) {
		xvalue objJson = xvoCreateTable();
		char* sRet;
		
		xvoTableSetText(objJson, "topic", 5, g_sLastBusTopic ? g_sLastBusTopic : "", 0, FALSE);
		xvoTableSetInt(objJson, "data_id", 7, g_iLastBusDataID);
		xvoTableSetText(objJson, "payload", 7, g_sLastBusPayload ? g_sLastBusPayload : "", 0, FALSE);
		
		sRet = xrtStringifyJSON(objJson, FALSE, NULL);
		xvoUnref(objJson);
		if ( sRet == NULL ) {
			return xsHttpJson(objResp, 500, "Internal Server Error", "{\"result\":false}") != 0;
		}
		
		if ( xsHttpJson(objResp, 200, "OK", sRet) == 0 ) {
			xrtFree(sRet);
			return false;
		}
		
		xrtFree(sRet);
		return true;
	}
	if ( strncmp(sPath, "/bus/", 5) == 0 ) {
		return xsHttpJson(objResp, 404, "Not Found", "{\"result\":false,\"message\":\"bus demo route removed, expose your own app route via xsData/xsMsg APIs\"}") != 0;
	}
	if ( DispatchBasicRoute(objServer, objHost, objReq, objResp) ) {
		return true;
	}
	
	snprintf(
		sBody,
		sizeof(sBody),
		"xserver vNext script host reload live\nserver=%s\nhost=%s\nmethod=%s\npath=%s\nquery=%s\ndevfile=%s\n",
		xsServerName(objServer),
		xsHostName(objHost),
		xsReqMethod(objReq),
		xsReqPath(objReq),
		sQuery ? sQuery : "",
		xsHostDevFile(objHost)
	);
	
	xsHttpHeader(objResp, "X-XS-Host", xsHostName(objHost));
	return xsHttpText(objResp, 200, "OK", sBody) != 0;
}
