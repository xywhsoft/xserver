bool RequestProc(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	const char* sPath;

	if ( objReq == NULL || objResp == NULL ) {
		return FALSE;
	}
	if ( !DemoEnsureReady(objHost) ) {
		return FALSE;
	}

	sPath = xsReqPath(objReq);
	if ( sPath == NULL || sPath[0] == '\0' ) {
		return FALSE;
	}
	if ( strcmp(sPath, "/test") == 0 ) {
		return Request_Test(objServer, objHost, objReq, objResp);
	}
	if ( strcmp(sPath, "/template") == 0 ) {
		return Request_Template(objServer, objHost, objReq, objResp);
	}
	if ( strcmp(sPath, "/chart/get") == 0 ) {
		return Request_Chart_Get(objServer, objHost, objReq, objResp);
	}
	if ( strcmp(sPath, "/app/list") == 0 ) {
		return Request_List(objServer, objHost, objReq, objResp);
	}
	if ( strcmp(sPath, "/app/add") == 0 ) {
		return Request_Add(objServer, objHost, objReq, objResp);
	}
	if ( strcmp(sPath, "/app/del") == 0 ) {
		return Request_Del(objServer, objHost, objReq, objResp);
	}
	if ( strcmp(sPath, "/app/edit") == 0 ) {
		return Request_Edit(objServer, objHost, objReq, objResp);
	}

	return FALSE;
}
