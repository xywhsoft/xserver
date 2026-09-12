/*
 * 迁移自 dev/v1 的表格增删改查范例
 *
 *   GET  /app/list   查询 test 表
 *   POST /app/add    新增记录
 *   POST /app/del    批量删除，body: [id, ...]
 *   POST /app/edit   单字段编辑，body: {id, field, value}
 */



static void Handle_App_List(
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
	if ( !DBPrepare("SELECT id, name, age, mail, \"desc\" FROM test",
		&pStatement) ) {
		xrtValueRelease(pRows);
		xrtValueRelease(pResult);
		ReplyDBError(pReq, "query failed");
		return;
	}
	while ( (iStep = sqlite3_step(pStatement)) == SQLITE_ROW ) {
		xvalue* pRow = xrtValueObject();
		cstr sID = (cstr)sqlite3_column_text(pStatement, 0);
		cstr sName = (cstr)sqlite3_column_text(pStatement, 1);
		cstr sAge = (cstr)sqlite3_column_text(pStatement, 2);
		cstr sMail = (cstr)sqlite3_column_text(pStatement, 3);
		cstr sDescription = (cstr)sqlite3_column_text(pStatement, 4);

		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("id"),
			xrtValueString(xrtStrView((sID ? sID : ""))));
		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("name"),
			xrtValueString(xrtStrView((sName ? sName : ""))));
		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("age"),
			xrtValueString(xrtStrView((sAge ? sAge : ""))));
		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("mail"),
			xrtValueString(xrtStrView((sMail ? sMail : ""))));
		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("desc"),
			xrtValueString(xrtStrView((sDescription ? sDescription : ""))));
		xrtValueArrayAppendNew(pRows, pRow);
		iCount++;
	}
	sqlite3_finalize(pStatement);
	if ( iStep != SQLITE_DONE ) {
		xrtValueRelease(pRows);
		xrtValueRelease(pResult);
		ReplyDBError(pReq, "query failed");
		return;
	}

	xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("count"),
		xrtValueInt(iCount));
	xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("code"), xrtValueInt(0));
	xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("msg"),
		xrtValueString(XRT_STR_LITERAL("platform list query success")));
	xrtValueObjectSetNew(pResult, XRT_STR_LITERAL("data"), pRows);
	(void)ReplyJSON(pReq, 200, pResult);
	xrtValueRelease(pResult);
}



static void Handle_App_Add(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	bool bMissing = false;
	xvalue* pBody = ReqBodyJSON(pReq, &bMissing);
	char* sName;
	char* sMail;
	char* sDescription;
	sqlite3_stmt* pStatement = NULL;
	int64 iAge = 0;
	int iResult;

	(void)arrParam;
	(void)iParamCount;
	if ( pBody == NULL ) {
		(void)ReplyResult(pReq, false,
			(bMissing ? "body required" : "invalid json"));
		return;
	}
	if ( xrtValueType(pBody) != XVALUE_OBJECT ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "body must be json object");
		return;
	}
	sName = ObjTextDup(pBody, "name");
	if ( sName == NULL ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "name required");
		return;
	}
	sMail = ObjTextDup(pBody, "mail");
	sDescription = ObjTextDup(pBody, "desc");
	(void)xrtValueGetInt(
		xrtValueObjectGet(pBody, XRT_STR_LITERAL("age")), &iAge);
	xrtValueRelease(pBody);

	if ( !DBPrepare(
		"INSERT INTO test (name, age, mail, \"desc\") VALUES (?, ?, ?, ?)",
		&pStatement) ) {
		xrtFree(sName);
		xrtFree(sMail);
		xrtFree(sDescription);
		ReplyDBError(pReq, "prepare insert failed");
		return;
	}
	DBBindTextOrEmpty(pStatement, 1, sName);
	sqlite3_bind_int64(pStatement, 2, iAge);
	DBBindTextOrEmpty(pStatement, 3, sMail);
	DBBindTextOrEmpty(pStatement, 4, sDescription);
	iResult = sqlite3_step(pStatement);
	sqlite3_finalize(pStatement);
	xrtFree(sName);
	xrtFree(sMail);
	xrtFree(sDescription);
	if ( iResult != SQLITE_DONE ) {
		ReplyDBError(pReq, "insert failed");
		return;
	}
	(void)ReplyResult(pReq, true, "add success");
}



static void Handle_App_Del(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	bool bMissing = false;
	xvalue* pBody = ReqBodyJSON(pReq, &bMissing);
	sqlite3_stmt* pStatement = NULL;
	size_t iCount;
	size_t i;

	(void)arrParam;
	(void)iParamCount;
	if ( pBody == NULL ) {
		(void)ReplyResult(pReq, false,
			(bMissing ? "body required" : "invalid json"));
		return;
	}
	if ( xrtValueType(pBody) != XVALUE_ARRAY ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "body must be json array");
		return;
	}
	if ( !DBPrepare("DELETE FROM test WHERE id = ?", &pStatement) ) {
		xrtValueRelease(pBody);
		ReplyDBError(pReq, "prepare delete failed");
		return;
	}

	iCount = xrtValueCount(pBody);
	for ( i = 0; i < iCount; i++ ) {
		xvalue* pItem = xrtValueArrayGet(pBody, i);
		xstrview tID = {0};
		char* sID = NULL;
		int64 iID = 0;

		if ( pItem != NULL && xrtValueGetString(pItem, &tID) &&
		     tID.Size > 0 ) {
			sID = xrtStrDupN(tID.Data, tID.Size);
		} else if ( pItem != NULL && xrtValueGetInt(pItem, &iID) &&
		            iID > 0 ) {
			sID = xrtFormat("%lld", (long long)iID);
		}
		if ( sID == NULL ) {
			continue;
		}
		DBBindTextOrEmpty(pStatement, 1, sID);
		xrtFree(sID);
		if ( sqlite3_step(pStatement) != SQLITE_DONE ) {
			sqlite3_finalize(pStatement);
			xrtValueRelease(pBody);
			ReplyDBError(pReq, "delete failed");
			return;
		}
		sqlite3_reset(pStatement);
		sqlite3_clear_bindings(pStatement);
	}
	sqlite3_finalize(pStatement);
	xrtValueRelease(pBody);
	(void)ReplyResult(pReq, true, "delete success");
}



static void Handle_App_Edit(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	static const struct {
		const char* sField;
		const char* sSQL;
	} arrField[4] = {
		{ "name", "UPDATE test SET name = ? WHERE id = ?" },
		{ "age",  "UPDATE test SET age = ? WHERE id = ?" },
		{ "mail", "UPDATE test SET mail = ? WHERE id = ?" },
		{ "desc", "UPDATE test SET \"desc\" = ? WHERE id = ?" }
	};
	xvalue* pBody;
	char* sID;
	char* sField;
	char* sValue;
	const char* sSQL = NULL;
	sqlite3_stmt* pStatement = NULL;
	int iResult;
	size_t i;
	bool bMissing = false;

	(void)arrParam;
	(void)iParamCount;
	pBody = ReqBodyJSON(pReq, &bMissing);
	if ( pBody == NULL ) {
		(void)ReplyResult(pReq, false,
			(bMissing ? "body required" : "invalid json"));
		return;
	}
	if ( xrtValueType(pBody) != XVALUE_OBJECT ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "body must be json object");
		return;
	}
	sID = ObjTextDup(pBody, "id");
	sField = ObjTextDup(pBody, "field");
	sValue = ObjTextDup(pBody, "value");
	xrtValueRelease(pBody);
	if ( sID == NULL || sField == NULL ) {
		const char* sError = sID == NULL ? "id required" : "field required";

		xrtFree(sID);
		xrtFree(sField);
		xrtFree(sValue);
		(void)ReplyResult(pReq, false, sError);
		return;
	}
	for ( i = 0; i < sizeof(arrField) / sizeof(arrField[0]); i++ ) {
		if ( strcmp(sField, arrField[i].sField) == 0 ) {
			sSQL = arrField[i].sSQL;
			break;
		}
	}
	if ( sSQL == NULL ) {
		xrtFree(sID);
		xrtFree(sField);
		xrtFree(sValue);
		(void)ReplyResult(pReq, false, "invalid field");
		return;
	}
	if ( !DBPrepare(sSQL, &pStatement) ) {
		xrtFree(sID);
		xrtFree(sField);
		xrtFree(sValue);
		ReplyDBError(pReq, "prepare update failed");
		return;
	}
	DBBindTextOrEmpty(pStatement, 1, sValue);
	DBBindTextOrEmpty(pStatement, 2, sID);
	iResult = sqlite3_step(pStatement);
	sqlite3_finalize(pStatement);
	xrtFree(sID);
	xrtFree(sField);
	xrtFree(sValue);
	if ( iResult != SQLITE_DONE ) {
		ReplyDBError(pReq, "update failed");
		return;
	}
	(void)ReplyResult(pReq, true, "edit success");
}
