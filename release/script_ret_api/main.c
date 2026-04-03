#include <xsbase.h>
#include <string.h>

bool RequestProc(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	const char* sPath = xsReqPath(objReq);

	if ( sPath == NULL ) {
		return false;
	}
	if ( strcmp(sPath, "/index.html") == 0 ) {
		return false;
	}
	if ( strcmp(sPath, "/ret404") == 0 ) {
		xvalue objData = xvoCreateTable();
		int iRet;

		xvoTableSetText(objData, "from", 4, "script", 0, FALSE);
		xvoTableSetText(objData, "note", 4, "ret404", 0, FALSE);
		iRet = Ret404(objServer, objHost, objReq, objResp, "script 404", objData);
		xvoUnref(objData);
		return iRet != 0;
	}
	if ( strcmp(sPath, "/ret500") == 0 ) {
		return Ret500(objServer, objHost, objReq, objResp, "script 500", NULL) != 0;
	}

	return false;
}
