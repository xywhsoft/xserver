#ifndef XS_SCRIPT_VNEXT_ROUTE_HTTP_BASIC_H
#define XS_SCRIPT_VNEXT_ROUTE_HTTP_BASIC_H

static sqlite3* g_pDemoDB = NULL;
static sqlite3_stmt* g_stmtDemoList = NULL;
static sqlite3_stmt* g_stmtDemoAdd = NULL;
static sqlite3_stmt* g_stmtDemoDel = NULL;
static sqlite3_stmt* g_stmtDemoEditName = NULL;
static sqlite3_stmt* g_stmtDemoEditAge = NULL;
static sqlite3_stmt* g_stmtDemoEditMail = NULL;
static sqlite3_stmt* g_stmtDemoEditDesc = NULL;

static int XS_DemoDB_Prepare(sqlite3_stmt** ppStmt, const char* sSQL)
{
	if ( g_pDemoDB == NULL ) {
		return SQLITE_MISUSE;
	}
	
	return sqlite3_prepare_v2(g_pDemoDB, sSQL, -1, ppStmt, NULL);
}

static sqlite3_stmt* XS_DemoDB_GetEditStmt(const char* sField)
{
	if ( sField == NULL ) {
		return NULL;
	}
	
	if ( strcmp(sField, "name") == 0 ) {
		return g_stmtDemoEditName;
	}
	if ( strcmp(sField, "age") == 0 ) {
		return g_stmtDemoEditAge;
	}
	if ( strcmp(sField, "mail") == 0 ) {
		return g_stmtDemoEditMail;
	}
	if ( strcmp(sField, "desc") == 0 ) {
		return g_stmtDemoEditDesc;
	}
	
	return NULL;
}

static void XS_DemoDB_ResetStmt(sqlite3_stmt* pStmt)
{
	if ( pStmt == NULL ) {
		return;
	}
	
	sqlite3_reset(pStmt);
	sqlite3_clear_bindings(pStmt);
}

static void XS_DemoDB_Close(void)
{
	if ( g_stmtDemoList ) {
		sqlite3_finalize(g_stmtDemoList);
		g_stmtDemoList = NULL;
	}
	if ( g_stmtDemoAdd ) {
		sqlite3_finalize(g_stmtDemoAdd);
		g_stmtDemoAdd = NULL;
	}
	if ( g_stmtDemoDel ) {
		sqlite3_finalize(g_stmtDemoDel);
		g_stmtDemoDel = NULL;
	}
	if ( g_stmtDemoEditName ) {
		sqlite3_finalize(g_stmtDemoEditName);
		g_stmtDemoEditName = NULL;
	}
	if ( g_stmtDemoEditAge ) {
		sqlite3_finalize(g_stmtDemoEditAge);
		g_stmtDemoEditAge = NULL;
	}
	if ( g_stmtDemoEditMail ) {
		sqlite3_finalize(g_stmtDemoEditMail);
		g_stmtDemoEditMail = NULL;
	}
	if ( g_stmtDemoEditDesc ) {
		sqlite3_finalize(g_stmtDemoEditDesc);
		g_stmtDemoEditDesc = NULL;
	}
	if ( g_pDemoDB ) {
		sqlite3_close(g_pDemoDB);
		g_pDemoDB = NULL;
	}
}

static bool XS_DemoDB_Open(void)
{
	char* sDbFile;
	char* sErrMsg = NULL;
	int iRet;
	
	if ( g_pDemoDB ) {
		return true;
	}
	
	sDbFile = xrtPathJoin(4, xsAppPath(), "data", "db", "main.db");
	if ( sDbFile == NULL ) {
		return false;
	}
	
	iRet = sqlite3_open(sDbFile, &g_pDemoDB);
	xrtFree(sDbFile);
	if ( iRet != SQLITE_OK ) {
		XS_DemoDB_Close();
		return false;
	}
	
	iRet = sqlite3_exec(
		g_pDemoDB,
		"CREATE TABLE IF NOT EXISTS xs_demo_test ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"name TEXT NOT NULL,"
			"age INTEGER NOT NULL DEFAULT 0,"
			"mail TEXT,"
			"\"desc\" TEXT"
		");",
		NULL,
		NULL,
		&sErrMsg
	);
	if ( iRet != SQLITE_OK ) {
		if ( sErrMsg ) {
			sqlite3_free(sErrMsg);
		}
		XS_DemoDB_Close();
		return false;
	}
	
	if ( XS_DemoDB_Prepare(&g_stmtDemoList, "SELECT id, name, age, mail, \"desc\" FROM xs_demo_test ORDER BY id DESC;") != SQLITE_OK ) {
		XS_DemoDB_Close();
		return false;
	}
	if ( XS_DemoDB_Prepare(&g_stmtDemoAdd, "INSERT INTO xs_demo_test (name, age, mail, \"desc\") VALUES (?, ?, ?, ?);") != SQLITE_OK ) {
		XS_DemoDB_Close();
		return false;
	}
	if ( XS_DemoDB_Prepare(&g_stmtDemoDel, "DELETE FROM xs_demo_test WHERE id = ?;") != SQLITE_OK ) {
		XS_DemoDB_Close();
		return false;
	}
	if ( XS_DemoDB_Prepare(&g_stmtDemoEditName, "UPDATE xs_demo_test SET name = ? WHERE id = ?;") != SQLITE_OK ) {
		XS_DemoDB_Close();
		return false;
	}
	if ( XS_DemoDB_Prepare(&g_stmtDemoEditAge, "UPDATE xs_demo_test SET age = ? WHERE id = ?;") != SQLITE_OK ) {
		XS_DemoDB_Close();
		return false;
	}
	if ( XS_DemoDB_Prepare(&g_stmtDemoEditMail, "UPDATE xs_demo_test SET mail = ? WHERE id = ?;") != SQLITE_OK ) {
		XS_DemoDB_Close();
		return false;
	}
	if ( XS_DemoDB_Prepare(&g_stmtDemoEditDesc, "UPDATE xs_demo_test SET \"desc\" = ? WHERE id = ?;") != SQLITE_OK ) {
		XS_DemoDB_Close();
		return false;
	}
	
	return true;
}

static bool XS_DemoDB_ReplyError(XS_ResponseObject objResp, int iStatus, const char* sMsg)
{
	xvalue objRet = xvoCreateTable();
	char* sRet;
	int bOK;
	
	xvoTableSetBool(objRet, "result", 6, FALSE);
	xvoTableSetText(objRet, "msg", 3, sMsg ? sMsg : "unknown error", 0, FALSE);
	xvoTableSetText(objRet, "message", 7, sMsg ? sMsg : "unknown error", 0, FALSE);
	sRet = xrtStringifyJSON(objRet, FALSE, NULL);
	xvoUnref(objRet);
	if ( sRet == NULL ) {
		return xsHttpJson(objResp, iStatus, "Error", "{\"result\":false}") != 0;
	}
	
	bOK = xsHttpJson(objResp, iStatus, "Error", sRet) != 0;
	xrtFree(sRet);
	return bOK;
}

static bool XS_DemoDB_ReplyJSON(XS_ResponseObject objResp, xvalue objRet)
{
	char* sRet = xrtStringifyJSON(objRet, FALSE, NULL);
	
	if ( sRet == NULL ) {
		xvoUnref(objRet);
		return xsHttpJson(objResp, 500, "Internal Server Error", "{\"result\":false}") != 0;
	}
	
	xvoUnref(objRet);
	if ( xsHttpJson(objResp, 200, "OK", sRet) == 0 ) {
		xrtFree(sRet);
		return false;
	}
	
	xrtFree(sRet);
	return true;
}

static xvalue XS_DemoDB_ParseBodyJSON(XS_RequestObject objReq)
{
	const void* pBody = xsReqBody(objReq);
	size_t iBodyLen = xsReqBodyLen(objReq);
	
	if ( pBody == NULL || iBodyLen == 0 ) {
		return NULL;
	}
	
	return xrtParseJSON((void*)pBody, iBodyLen);
}

static bool Request_List(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	xvalue objRet;
	xvalue arrList;
	int iCount = 0;
	
	(void)objServer;
	(void)objHost;
	(void)objReq;
	
	if ( !XS_DemoDB_Open() ) {
		return XS_DemoDB_ReplyError(objResp, 500, "open sqlite demo db failed");
	}
	
	objRet = xvoCreateTable();
	arrList = xvoCreateArray();
	while ( sqlite3_step(g_stmtDemoList) == SQLITE_ROW ) {
		xvalue objRow = xvoCreateTable();
		char sID[32];
		char sAge[32];
		const char* sName = (const char*)sqlite3_column_text(g_stmtDemoList, 1);
		const char* sMail = (const char*)sqlite3_column_text(g_stmtDemoList, 3);
		const char* sDesc = (const char*)sqlite3_column_text(g_stmtDemoList, 4);
		
		snprintf(sID, sizeof(sID), "%lld", (long long)sqlite3_column_int64(g_stmtDemoList, 0));
		snprintf(sAge, sizeof(sAge), "%d", sqlite3_column_int(g_stmtDemoList, 2));
		xvoTableSetText(objRow, "id", 2, sID, 0, FALSE);
		xvoTableSetText(objRow, "name", 4, sName ? sName : "", 0, FALSE);
		xvoTableSetText(objRow, "age", 3, sAge, 0, FALSE);
		xvoTableSetText(objRow, "mail", 4, sMail ? sMail : "", 0, FALSE);
		xvoTableSetText(objRow, "desc", 4, sDesc ? sDesc : "", 0, FALSE);
		xvoArrayAppendValue(arrList, objRow, TRUE);
		iCount++;
	}
	XS_DemoDB_ResetStmt(g_stmtDemoList);
	
	xvoTableSetInt(objRet, "count", 5, iCount);
	xvoTableSetInt(objRet, "code", 4, 0);
	xvoTableSetText(objRet, "msg", 3, "平台列表查询成功！", 0, FALSE);
	xvoTableSetText(objRet, "message", 7, "平台列表查询成功！", 0, FALSE);
	xvoTableSetBool(objRet, "result", 6, TRUE);
	xvoTableSetValue(objRet, "data", 4, arrList, TRUE);
	return XS_DemoDB_ReplyJSON(objResp, objRet);
}

static bool Request_Add(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	xvalue objBody;
	const char* sName;
	const char* sMail;
	const char* sDesc;
	int iAge;
	
	(void)objServer;
	(void)objHost;
	
	if ( !XS_DemoDB_Open() ) {
		return XS_DemoDB_ReplyError(objResp, 500, "open sqlite demo db failed");
	}
	
	objBody = XS_DemoDB_ParseBodyJSON(objReq);
	if ( objBody == NULL || objBody->Type != XVO_DT_TABLE ) {
		if ( objBody ) xvoUnref(objBody);
		return XS_DemoDB_ReplyError(objResp, 400, "Body 域必须传递为 JSON 对象！");
	}
	
	sName = xvoTableGetText(objBody, "name", 4);
	if ( sName == NULL || sName[0] == '\0' ) {
		xvoUnref(objBody);
		return XS_DemoDB_ReplyError(objResp, 400, "参数 name 不能为空！");
	}
	
	sMail = xvoTableGetText(objBody, "mail", 4);
	sDesc = xvoTableGetText(objBody, "desc", 4);
	iAge = (int)xvoTableGetInt(objBody, "age", 3);
	
	sqlite3_bind_text(g_stmtDemoAdd, 1, sName, -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(g_stmtDemoAdd, 2, iAge);
	sqlite3_bind_text(g_stmtDemoAdd, 3, sMail ? sMail : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(g_stmtDemoAdd, 4, sDesc ? sDesc : "", -1, SQLITE_TRANSIENT);
	if ( sqlite3_step(g_stmtDemoAdd) != SQLITE_DONE ) {
		XS_DemoDB_ResetStmt(g_stmtDemoAdd);
		xvoUnref(objBody);
		return XS_DemoDB_ReplyError(objResp, 500, sqlite3_errmsg(g_pDemoDB));
	}
	
	XS_DemoDB_ResetStmt(g_stmtDemoAdd);
	xvoUnref(objBody);
	return xsHttpJson(objResp, 200, "OK", "{\"result\":true,\"msg\":\"添加数据成功！\",\"message\":\"添加数据成功！\"}") != 0;
}

static bool Request_Del(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	xvalue objBody;
	int iCount;
	int i;
	
	(void)objServer;
	(void)objHost;
	
	if ( !XS_DemoDB_Open() ) {
		return XS_DemoDB_ReplyError(objResp, 500, "open sqlite demo db failed");
	}
	
	objBody = XS_DemoDB_ParseBodyJSON(objReq);
	if ( objBody == NULL ) {
		return XS_DemoDB_ReplyError(objResp, 400, "Body 域必须传递为 JSON 数组或单个 ID！");
	}
	if ( objBody->Type != XVO_DT_ARRAY ) {
		int64 iID = xvoGetInt(objBody);
		if ( iID <= 0 ) {
			xvoUnref(objBody);
			return XS_DemoDB_ReplyError(objResp, 400, "Body 域必须传递为 JSON 数组或单个 ID！");
		}
		
		sqlite3_bind_int64(g_stmtDemoDel, 1, iID);
		if ( sqlite3_step(g_stmtDemoDel) != SQLITE_DONE ) {
			XS_DemoDB_ResetStmt(g_stmtDemoDel);
			xvoUnref(objBody);
			return XS_DemoDB_ReplyError(objResp, 500, sqlite3_errmsg(g_pDemoDB));
		}
		
		XS_DemoDB_ResetStmt(g_stmtDemoDel);
		xvoUnref(objBody);
		return xsHttpJson(objResp, 200, "OK", "{\"result\":true,\"msg\":\"数据删除成功！\",\"message\":\"数据删除成功！\"}") != 0;
	}
	iCount = xvoArrayItemCount(objBody);
	for ( i = 0; i < iCount; i++ ) {
		int64 iID = xvoArrayGetInt(objBody, i);
		sqlite3_bind_int64(g_stmtDemoDel, 1, iID);
		if ( sqlite3_step(g_stmtDemoDel) != SQLITE_DONE ) {
			XS_DemoDB_ResetStmt(g_stmtDemoDel);
			xvoUnref(objBody);
			return XS_DemoDB_ReplyError(objResp, 500, sqlite3_errmsg(g_pDemoDB));
		}
		XS_DemoDB_ResetStmt(g_stmtDemoDel);
	}
	
	xvoUnref(objBody);
	return xsHttpJson(objResp, 200, "OK", "{\"result\":true,\"msg\":\"数据删除成功！\",\"message\":\"数据删除成功！\"}") != 0;
}

static bool Request_Edit(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	xvalue objBody;
	const char* sField;
	sqlite3_stmt* pStmt;
	int64 iID;
	
	(void)objServer;
	(void)objHost;
	
	if ( !XS_DemoDB_Open() ) {
		return XS_DemoDB_ReplyError(objResp, 500, "open sqlite demo db failed");
	}
	
	objBody = XS_DemoDB_ParseBodyJSON(objReq);
	if ( objBody == NULL || objBody->Type != XVO_DT_TABLE ) {
		if ( objBody ) xvoUnref(objBody);
		return XS_DemoDB_ReplyError(objResp, 400, "Body 域必须传递为 JSON 对象！");
	}
	
	iID = xvoTableGetInt(objBody, "id", 2);
	if ( iID <= 0 ) {
		xvoUnref(objBody);
		return XS_DemoDB_ReplyError(objResp, 400, "参数 id 不能为空！");
	}
	
	sField = xvoTableGetText(objBody, "field", 5);
	pStmt = XS_DemoDB_GetEditStmt(sField);
	if ( pStmt == NULL ) {
		xvoUnref(objBody);
		return XS_DemoDB_ReplyError(objResp, 400, "无效的字段名！");
	}
	
	if ( strcmp(sField, "age") == 0 ) {
		sqlite3_bind_int64(pStmt, 1, xvoTableGetInt(objBody, "value", 5));
	} else {
		const char* sValue = xvoTableGetText(objBody, "value", 5);
		sqlite3_bind_text(pStmt, 1, sValue ? sValue : "", -1, SQLITE_TRANSIENT);
	}
	sqlite3_bind_int64(pStmt, 2, iID);
	if ( sqlite3_step(pStmt) != SQLITE_DONE ) {
		XS_DemoDB_ResetStmt(pStmt);
		xvoUnref(objBody);
		return XS_DemoDB_ReplyError(objResp, 500, sqlite3_errmsg(g_pDemoDB));
	}
	
	XS_DemoDB_ResetStmt(pStmt);
	xvoUnref(objBody);
	return xsHttpJson(objResp, 200, "OK", "{\"result\":true,\"msg\":\"数据编辑成功！\",\"message\":\"数据编辑成功！\"}") != 0;
}

static bool Request_Test(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	char sBody[256];
	
	snprintf(
		sBody,
		sizeof(sBody),
		"page load success !\nserver=%s\nhost=%s\npath=%s\n",
		xsServerName(objServer),
		xsHostName(objHost),
		xsReqPath(objReq)
	);
	return xsHttpText(objResp, 200, "OK", sBody) != 0;
}

static bool Request_Chart_Get(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	xvalue objOption = xvoCreateTable();
	xvalue objXAxis = xvoCreateTable();
	xvalue objXAxisData = xvoCreateArray();
	xvalue objYAxis = xvoCreateTable();
	xvalue objSeries = xvoCreateArray();
	xvalue objLine = xvoCreateTable();
	xvalue objLineData = xvoCreateArray();
	char* sRet;
	int i;
	
	(void)objServer;
	(void)objHost;
	(void)objReq;
	
	xvoTableSetValue(objOption, "xAxis", 5, objXAxis, TRUE);
	xvoTableSetText(objXAxis, "type", 4, "category", 0, FALSE);
	xvoTableSetValue(objXAxis, "data", 4, objXAxisData, TRUE);
	xvoArrayAppendText(objXAxisData, "Mon", 0, FALSE);
	xvoArrayAppendText(objXAxisData, "Tue", 0, FALSE);
	xvoArrayAppendText(objXAxisData, "Wed", 0, FALSE);
	xvoArrayAppendText(objXAxisData, "Thu", 0, FALSE);
	xvoArrayAppendText(objXAxisData, "Fri", 0, FALSE);
	xvoArrayAppendText(objXAxisData, "Sat", 0, FALSE);
	xvoArrayAppendText(objXAxisData, "Sun", 0, FALSE);
	
	xvoTableSetValue(objOption, "yAxis", 5, objYAxis, TRUE);
	xvoTableSetText(objYAxis, "type", 4, "value", 0, FALSE);
	
	xvoTableSetValue(objOption, "series", 6, objSeries, TRUE);
	xvoArrayAppendValue(objSeries, objLine, TRUE);
	xvoTableSetText(objLine, "type", 4, "line", 0, FALSE);
	xvoTableSetBool(objLine, "smooth", 6, TRUE);
	xvoTableSetValue(objLine, "data", 4, objLineData, TRUE);
	for ( i = 0; i < 7; i++ ) {
		xvoArrayAppendInt(objLineData, xrtRandRange(100, 1500));
	}
	
	sRet = xrtStringifyJSON(objOption, FALSE, NULL);
	xvoUnref(objOption);
	if ( sRet == NULL ) {
		return xsHttpJson(objResp, 500, "Internal Server Error", "{\"result\":false}") != 0;
	}
	
	xsHttpHeader(objResp, "X-XS-Mode", "chart");
	if ( xsHttpJson(objResp, 200, "OK", sRet) == 0 ) {
		xrtFree(sRet);
		return false;
	}
	
	xrtFree(sRet);
	return true;
}

static bool Request_Template(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	char* sTemplateFile;
	char* sTemplateText;
	XTE_LiteObject objTemplate;
	xvalue objData;
	xvalue objList;
	char* sPage;
	
	(void)objServer;
	(void)objReq;
	
	sTemplateFile = xrtPathJoin(4, xsAppPath(), "data", "template", "page.html");
	if ( sTemplateFile == NULL ) {
		return xsHttpText(objResp, 500, "Internal Server Error", "template path build failed") != 0;
	}
	
	sTemplateText = xrtFileReadAll(sTemplateFile, XRT_CP_BINARY, NULL);
	xrtFree(sTemplateFile);
	if ( sTemplateText == NULL ) {
		return xsHttpText(objResp, 500, "Internal Server Error", "template read failed") != 0;
	}
	
	objTemplate = xteParse(sTemplateText, strlen(sTemplateText), NULL);
	xrtFree(sTemplateText);
	if ( objTemplate == NULL || objTemplate->Success == FALSE ) {
		if ( objTemplate ) {
			xteParseFree(objTemplate);
		}
		return xsHttpText(objResp, 500, "Internal Server Error", "template parse failed") != 0;
	}
	
	objData = xvoCreateTable();
	objList = xvoCreateArray();
	xvoTableSetText(objData, "title", 5, "vNext 模板页面", 0, FALSE);
	xvoTableSetText(objData, "desc", 4, xsHostName(objHost), 0, FALSE);
	xvoArrayAppendText(objList, "文章列表 1", 0, FALSE);
	xvoArrayAppendText(objList, "文章列表 2", 0, FALSE);
	xvoArrayAppendText(objList, "文章列表 3", 0, FALSE);
	xvoArrayAppendText(objList, "文章列表 4", 0, FALSE);
	xvoArrayAppendText(objList, "文章列表 5", 0, FALSE);
	xvoTableSetValue(objData, "list", 4, objList, TRUE);
	
	sPage = xteMake(objTemplate, objData, NULL, NULL, NULL);
	xvoUnref(objData);
	xteParseFree(objTemplate);
	if ( sPage == NULL ) {
		return xsHttpText(objResp, 500, "Internal Server Error", "template render failed") != 0;
	}
	
	if ( xsHttpBody(objResp, sPage, strlen(sPage), "text/html; charset=utf-8") == 0 ) {
		xrtFree(sPage);
		return false;
	}
	
	xrtFree(sPage);
	return true;
}

static bool DispatchBasicRoute(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	const char* sPath = xsReqPath(objReq);
	
	if ( sPath == NULL ) {
		return false;
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
	if ( strcmp(sPath, "/test") == 0 ) {
		return Request_Test(objServer, objHost, objReq, objResp);
	}
	if ( strcmp(sPath, "/chart/get") == 0 ) {
		return Request_Chart_Get(objServer, objHost, objReq, objResp);
	}
	if ( strcmp(sPath, "/template") == 0 ) {
		return Request_Template(objServer, objHost, objReq, objResp);
	}
	
	return false;
}

#endif
