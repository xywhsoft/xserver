#include <xsbase.h>
#include <string.h>





bool RequestProc(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	xvalue objRet;
	char* sJSON;
	const char* sPath;

	(void)objServer;
	(void)objHost;

	sPath = xsReqPath(objReq);
	if ( sPath == NULL ) {
		return FALSE;
	}
	if ( strcmp(sPath, "/api/config") != 0 ) {
		return FALSE;
	}

	objRet = xvoCreateTable();
	xvoTableSetText(objRet, "ws_url", 6, "ws://127.0.0.1:18181/", 0, FALSE);
	xvoTableSetText(objRet, "ws_protocol", 11, "xs-demo-p2p", 0, FALSE);
	xvoTableSetText(objRet, "title", 5, "WebSocket 点对点示例", 0, FALSE);
	sJSON = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sJSON == NULL ) {
		return xsHttpJson(objResp, 500, "Internal Server Error", "{\"result\":false}") != 0;
	}

	if ( xsHttpJson(objResp, 200, "OK", sJSON) == 0 ) {
		xrtFree(sJSON);
		return FALSE;
	}

	xrtFree(sJSON);
	return TRUE;
}
