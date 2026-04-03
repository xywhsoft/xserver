#include <xsbase.h>
#include <string.h>

#include "app_db.h"



bool RequestProc(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	const char* sPath;

	(void)objServer;
	(void)objHost;

	sPath = xsReqPath(objReq);
	if ( sPath == NULL ) {
		return FALSE;
	}
	if ( strcmp(sPath, "/api/peer/list") == 0 ) {
		return procReplyJSONValue(objResp, procPeerListValue());
	}
	if ( strcmp(sPath, "/api/log/list") == 0 ) {
		return procReplyJSONValue(objResp, procLogListValue());
	}

	return FALSE;
}
