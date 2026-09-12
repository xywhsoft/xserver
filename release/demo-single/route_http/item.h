/*
 * item 数据与 RESTful API 范例
 *
 * 旧接口继续保留；/api/v1/items 使用同一 URI 的多个方法槽展示 RESTful
 * 注册方式。路径参数通过 GetRouteParamHTTP 按名称读取。
 */



static bool CopyRouteParamHTTP(
	const RouteParamHTTP* arrParam,
	uint32 iParamCount,
	const char* sName,
	char* sOutput,
	size_t iCapacity
)
{
	xstrview tValue;

	if ( sOutput == NULL || iCapacity == 0 ||
	     !GetRouteParamHTTP(arrParam, iParamCount, sName, &tValue) ||
	     tValue.Size >= iCapacity ) {
		return false;
	}
	memcpy(sOutput, tValue.Data, tValue.Size);
	sOutput[tValue.Size] = '\0';
	return true;
}



/* GET /api/v1/file/{name}.txt —— 捕获同一路径段中文件名部分。 */
static void Handle_API_File(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	xvalue* pResult = xrtValueObject();
	char arrName[128];

	if ( CopyRouteParamHTTP(arrParam, iParamCount, "name",
		arrName, sizeof(arrName)) ) {
		xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("name"),
			xrtValueString(xrtStrView(arrName)));
		xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("ext"),
			xrtValueString(XRT_STR_LITERAL("txt")));
	} else {
		xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("name"),
			xrtValueString(XRT_STR_LITERAL("")));
	}
	(void)ReplyJSON(pReq, 200, pResult);
	xrtValueRelease(pResult);
}



/* GET /api/list 和 GET /api/v1/items —— 查询 item 列表。 */
static void Handle_API_List(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	xvalue* pResult = xrtValueObject();
	xvalue* pRows = xrtValueArray();
	sqlite3_stmt* pStatement = NULL;
	int iStep;
	int iCount = 0;

	(void)arrParam;
	(void)iParamCount;
	if ( G_DB == NULL ) {
		xrtValueRelease(pRows);
		xrtValueRelease(pResult);
		(void)ReplyResult(pReq, false, "database not ready");
		return;
	}
	if ( sqlite3_prepare_v2(G_DB,
		"SELECT id, name, created FROM item ORDER BY id",
		-1, &pStatement, NULL) != SQLITE_OK ) {
		xrtValueRelease(pRows);
		xrtValueRelease(pResult);
		(void)ReplyResult(pReq, false, sqlite3_errmsg(G_DB));
		return;
	}
	while ( (iStep = sqlite3_step(pStatement)) == SQLITE_ROW ) {
		xvalue* pRow = xrtValueObject();
		cstr sName = (cstr)sqlite3_column_text(pStatement, 1);

		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("id"),
			xrtValueInt(sqlite3_column_int64(pStatement, 0)));
		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("name"),
			xrtValueString(xrtStrView((sName ? sName : ""))));
		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("created"),
			xrtValueInt(sqlite3_column_int64(pStatement, 2)));
		xrtValueArrayAppendNew(pRows, pRow);
		iCount++;
	}
	sqlite3_finalize(pStatement);
	if ( iStep != SQLITE_DONE ) {
		xrtValueRelease(pRows);
		xrtValueRelease(pResult);
		(void)ReplyResult(pReq, false, sqlite3_errmsg(G_DB));
		return;
	}

	xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("count"),
		xrtValueInt(iCount));
	xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("data"), pRows);
	(void)ReplyJSON(pReq, 200, pResult);
	xrtValueRelease(pResult);
}



/* POST /api/add 和 POST /api/v1/items —— 新建 item。 */
static void Handle_API_Add(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	bool bMissing = false;
	xvalue* pBody;
	xvalue* pNameValue;
	xstrview tName = {0};
	sqlite3_stmt* pStatement = NULL;

	(void)arrParam;
	(void)iParamCount;
	pBody = ReqBodyJSON(pReq, &bMissing);
	if ( pBody == NULL ) {
		(void)ReplyResult(pReq, false,
			(bMissing ? "body required" : "invalid json"));
		return;
	}
	pNameValue = xrtValueObjectGet(pBody, XRT_STR_LITERAL("name"));
	if ( pNameValue == NULL || !xrtValueGetString(pNameValue, &tName) ||
	     tName.Size == 0 ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "name required");
		return;
	}
	if ( G_DB == NULL ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "database not ready");
		return;
	}
	if ( sqlite3_prepare_v2(G_DB,
		"INSERT INTO item (name, created) VALUES (?, ?)",
		-1, &pStatement, NULL) != SQLITE_OK ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, sqlite3_errmsg(G_DB));
		return;
	}
	{
		char* sName = xrtStrDupN(tName.Data, tName.Size);

		sqlite3_bind_text(pStatement, 1, (sName ? sName : ""),
			-1, SQLITE_TRANSIENT);
		xrtFree(sName);
	}
	sqlite3_bind_int64(pStatement, 2,
		(sqlite3_int64)(xrtNow() / 1000000));
	(void)sqlite3_step(pStatement);
	sqlite3_finalize(pStatement);
	xrtValueRelease(pBody);
	(void)ReplyResult(pReq, true, "ok");
}



/* GET /api/v1/item/{id} 和 GET /api/v1/items/{id}。 */
static void Handle_API_Item(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	char arrID[32];
	sqlite3_stmt* pStatement = NULL;
	int iStep;

	if ( !CopyRouteParamHTTP(arrParam, iParamCount, "id",
		arrID, sizeof(arrID)) ) {
		(void)ReplyResult(pReq, false, "invalid id");
		return;
	}
	if ( !DBPrepare("SELECT id, name, created FROM item WHERE id = ?",
		&pStatement) ) {
		ReplyDBError(pReq, "query failed");
		return;
	}
	sqlite3_bind_text(pStatement, 1, arrID, -1, SQLITE_TRANSIENT);
	iStep = sqlite3_step(pStatement);
	if ( iStep == SQLITE_ROW ) {
		xvalue* pResult = xrtValueObject();
		cstr sName = (cstr)sqlite3_column_text(pStatement, 1);

		xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("id"),
			xrtValueInt(sqlite3_column_int64(pStatement, 0)));
		xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("name"),
			xrtValueString(xrtStrView((sName ? sName : ""))));
		xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("created"),
			xrtValueInt(sqlite3_column_int64(pStatement, 2)));
		(void)ReplyJSON(pReq, 200, pResult);
		xrtValueRelease(pResult);
	} else {
		(void)ReplyResult(pReq, false, "not found");
	}
	sqlite3_finalize(pStatement);
}



/* PUT/PATCH /api/v1/items/{id} —— 更新 item 名称。 */
static void Handle_API_Item_Update(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	char arrID[32];
	bool bMissing = false;
	xvalue* pBody;
	char* sName;
	sqlite3_stmt* pStatement = NULL;
	int iResult;

	if ( !CopyRouteParamHTTP(arrParam, iParamCount, "id",
		arrID, sizeof(arrID)) ) {
		(void)ReplyResult(pReq, false, "invalid id");
		return;
	}
	pBody = ReqBodyJSON(pReq, &bMissing);
	if ( pBody == NULL ) {
		(void)ReplyResult(pReq, false,
			(bMissing ? "body required" : "invalid json"));
		return;
	}
	sName = ObjTextDup(pBody, "name");
	xrtValueRelease(pBody);
	if ( sName == NULL ) {
		(void)ReplyResult(pReq, false, "name required");
		return;
	}
	if ( !DBPrepare("UPDATE item SET name = ? WHERE id = ?", &pStatement) ) {
		xrtFree(sName);
		ReplyDBError(pReq, "prepare update failed");
		return;
	}
	DBBindTextOrEmpty(pStatement, 1, sName);
	sqlite3_bind_text(pStatement, 2, arrID, -1, SQLITE_TRANSIENT);
	iResult = sqlite3_step(pStatement);
	xrtFree(sName);
	if ( iResult != SQLITE_DONE ) {
		sqlite3_finalize(pStatement);
		ReplyDBError(pReq, "update failed");
		return;
	}
	iResult = sqlite3_changes(G_DB);
	sqlite3_finalize(pStatement);
	(void)ReplyResult(pReq, iResult != 0,
		(iResult != 0 ? "update success" : "not found"));
}



/* DELETE /api/v1/items/{id} —— 删除一个 item。 */
static void Handle_API_Item_Delete(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	char arrID[32];
	sqlite3_stmt* pStatement = NULL;
	int iResult;

	if ( !CopyRouteParamHTTP(arrParam, iParamCount, "id",
		arrID, sizeof(arrID)) ) {
		(void)ReplyResult(pReq, false, "invalid id");
		return;
	}
	if ( !DBPrepare("DELETE FROM item WHERE id = ?", &pStatement) ) {
		ReplyDBError(pReq, "prepare delete failed");
		return;
	}
	sqlite3_bind_text(pStatement, 1, arrID, -1, SQLITE_TRANSIENT);
	iResult = sqlite3_step(pStatement);
	if ( iResult != SQLITE_DONE ) {
		sqlite3_finalize(pStatement);
		ReplyDBError(pReq, "delete failed");
		return;
	}
	iResult = sqlite3_changes(G_DB);
	sqlite3_finalize(pStatement);
	(void)ReplyResult(pReq, iResult != 0,
		(iResult != 0 ? "delete success" : "not found"));
}
