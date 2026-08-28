#include <xsbase.h>





str ExePath;
str AppPath;
str WebPath;
str DBPath;
str TempPath;
str ToolPath;
str OptionPath;
str TemplatePath;



sqlite3* G_DB;
extern xvalue tblENV;
extern xdict StaticRouteTableHTTP;
void InitTemplate();
void FreeTemplate();
void InitRouteHTTP();


static bool DemoEnsureReady(XS_HostObject objHost)
{
	int iResult;
	str sFileDB;

	if ( objHost == NULL ) {
		return FALSE;
	}

	if ( WebPath == NULL ) {
		ExePath = (str)xsAppPath();
		WebPath = (str)xsHostPath(objHost);
		AppPath = xrtPathGetDir(WebPath, 0);
		DBPath = xrtPathJoin(3, AppPath, "data", "db");
		TempPath = xrtPathJoin(3, AppPath, "data", "temp");
		ToolPath = xrtPathJoin(2, ExePath, "tools");
		OptionPath = xrtPathJoin(3, AppPath, "data", "options");
		TemplatePath = xrtPathJoin(3, AppPath, "data", "template");
	}

	if ( TempPath ) {
		xrtDirCreate(TempPath);
	}

	if ( G_DB == NULL ) {
		sFileDB = xrtPathJoin(2, DBPath, "main.db");
		iResult = sqlite3_open(sFileDB, &G_DB);
		xrtFree(sFileDB);
		if ( iResult != SQLITE_OK ) {
			printf("demo sqlite open failed: %s\n", G_DB ? sqlite3_errmsg(G_DB) : "unknown");
			if ( G_DB ) {
				sqlite3_close(G_DB);
				G_DB = NULL;
			}
			return FALSE;
		}
	}

	if ( tblENV == NULL ) {
		InitTemplate();
	}
	if ( StaticRouteTableHTTP == NULL ) {
		InitRouteHTTP();
	}

	return TRUE;
}


static bool DemoSQLitePrepare(sqlite3_stmt** ppStmt, const char* sSQL)
{
	if ( ppStmt == NULL ) {
		return FALSE;
	}

	*ppStmt = NULL;
	if ( G_DB == NULL || sSQL == NULL ) {
		return FALSE;
	}

	return sqlite3_prepare_v3(G_DB, sSQL, -1, 0, ppStmt, NULL) == SQLITE_OK;
}

static int DemoSQLiteBindTextOrEmpty(sqlite3_stmt* pStmt, int iIndex, const char* sText)
{
	if ( sText == NULL || sText[0] == '\0' ) {
		return sqlite3_bind_text(pStmt, iIndex, "", 0, SQLITE_STATIC);
	}

	return sqlite3_bind_text(pStmt, iIndex, sText, -1, SQLITE_TRANSIENT);
}

static bool DemoHttpReplyHTML(XS_ResponseObject objResp, uint32 iStatus, const char* sReason, const char* sHTML)
{
	size_t iLen = 0;

	if ( sHTML ) {
		iLen = strlen(sHTML);
	}

	(void)xsHttpStatus(objResp, iStatus, sReason);
	(void)xsHttpHeader(objResp, "Access-Control-Allow-Origin", "*");
	return xsHttpBody(objResp, sHTML ? sHTML : "", iLen, "text/html; charset=utf-8") != 0;
}

static bool DemoHttpReplyJSON(XS_ResponseObject objResp, uint32 iStatus, const char* sReason, const char* sJSON)
{
	return xsHttpJson(objResp, iStatus, sReason, sJSON ? sJSON : "{}") != 0;
}

static bool DemoHttpReplyJSONValue(XS_ResponseObject objResp, uint32 iStatus, const char* sReason, xvalue objJSON)
{
	bool bRet = FALSE;
	char* sJSON;

	sJSON = xrtStringifyJSON(objJSON, FALSE, NULL);
	if ( sJSON == NULL ) {
		return DemoHttpReplyJSON(objResp, 500, "Internal Server Error", "{\"result\":false,\"msg\":\"json stringify failed\"}");
	}

	bRet = DemoHttpReplyJSON(objResp, iStatus, sReason, sJSON);
	xrtFree(sJSON);
	return bRet;
}

static bool DemoHttpReplyResult(XS_ResponseObject objResp, bool bResult, const char* sMsg)
{
	xvalue objRet = xvoCreateTable();
	bool bRet;

	xvoTableSetBool(objRet, "result", 6, bResult);
	xvoTableSetText(objRet, "msg", 3, sMsg ? sMsg : "", 0, FALSE);

	bRet = DemoHttpReplyJSONValue(objResp, 200, "OK", objRet);
	xvoUnref(objRet);
	return bRet;
}

static bool DemoHttpReplyDBError(XS_ResponseObject objResp, const char* sMsg)
{
	xvalue objRet = xvoCreateTable();
	bool bRet;
	const char* sErr = "sqlite error";

	if ( G_DB ) {
		sErr = sqlite3_errmsg(G_DB);
	}

	xvoTableSetBool(objRet, "result", 6, FALSE);
	xvoTableSetText(objRet, "msg", 3, sMsg ? sMsg : sErr, 0, FALSE);

	bRet = DemoHttpReplyJSONValue(objResp, 200, "OK", objRet);
	xvoUnref(objRet);
	return bRet;
}



#include "module/template.h"



#include "route_http/curd.h"
#include "route_http/chart.h"
#include "route_http/test.h"



#include "route.h"



#include "module/http.h"





void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
	(void)objServer;

	(void)DemoEnsureReady(objHost);
}



void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
	(void)objServer;
	(void)objHost;

	if ( StaticRouteTableHTTP ) {
		xrtDictDestroy(StaticRouteTableHTTP);
		StaticRouteTableHTTP = NULL;
	}

	if ( G_DB ) {
		sqlite3_close(G_DB);
		G_DB = NULL;
	}

	FreeTemplate();

	if ( AppPath ) {
		xrtFree(AppPath);
		AppPath = NULL;
	}
	if ( DBPath ) {
		xrtFree(DBPath);
		DBPath = NULL;
	}
	if ( TempPath ) {
		xrtFree(TempPath);
		TempPath = NULL;
	}
	if ( ToolPath ) {
		xrtFree(ToolPath);
		ToolPath = NULL;
	}
	if ( OptionPath ) {
		xrtFree(OptionPath);
		OptionPath = NULL;
	}
	if ( TemplatePath ) {
		xrtFree(TemplatePath);
		TemplatePath = NULL;
	}
}
