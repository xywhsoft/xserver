#ifndef EXAMPLES_XTP_APP_DB_H
#define EXAMPLES_XTP_APP_DB_H

static char* procDBFile(void)
{
	char* sDBDir;
	char* sDBFile;

	sDBDir = xrtPathJoin(4, xsAppPath(), "data", "db", "");
	if ( sDBDir == NULL ) {
		return NULL;
	}
	xrtDirCreateAll(sDBDir);
	xrtFree(sDBDir);

	sDBFile = xrtPathJoin(4, xsAppPath(), "data", "db", "main.db");
	return sDBFile;
}

static sqlite3* procDBOpen(void)
{
	sqlite3* pDB = NULL;
	char* sDBFile;
	char* sErr = NULL;

	sDBFile = procDBFile();
	if ( sDBFile == NULL ) {
		return NULL;
	}
	if ( sqlite3_open(sDBFile, &pDB) != SQLITE_OK ) {
		xrtFree(sDBFile);
		if ( pDB ) {
			sqlite3_close(pDB);
		}
		return NULL;
	}
	xrtFree(sDBFile);
	sqlite3_busy_timeout(pDB, 2000);
	if ( sqlite3_exec(
		pDB,
		"CREATE TABLE IF NOT EXISTS xtp_log ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"level TEXT NOT NULL,"
			"source TEXT NOT NULL,"
			"cmd TEXT NOT NULL,"
			"msg_type INTEGER NOT NULL,"
			"message TEXT NOT NULL,"
			"created_at TEXT NOT NULL"
		");",
		NULL,
		NULL,
		&sErr
	) != SQLITE_OK ) {
		if ( sErr ) {
			sqlite3_free(sErr);
		}
		sqlite3_close(pDB);
		return NULL;
	}

	return pDB;
}

static bool procInsertLog(const char* sLevel, const char* sSource, const char* sCmd, int iMsgType, const char* sMessage)
{
	sqlite3* pDB;
	sqlite3_stmt* pStmt = NULL;
	char* sNow;
	int iRet;

	pDB = procDBOpen();
	if ( pDB == NULL ) {
		return FALSE;
	}

	sNow = xrtNowStr();
	if ( sqlite3_prepare_v2(
		pDB,
		"INSERT INTO xtp_log (level, source, cmd, msg_type, message, created_at) VALUES (?, ?, ?, ?, ?, ?);",
		-1,
		&pStmt,
		NULL
	) != SQLITE_OK ) {
		sqlite3_close(pDB);
		if ( sNow ) {
			xrtFree(sNow);
		}
		return FALSE;
	}

	sqlite3_bind_text(pStmt, 1, sLevel ? sLevel : "INFO", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 2, sSource ? sSource : "xtp-client", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 3, sCmd ? sCmd : "log.push", -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(pStmt, 4, iMsgType);
	sqlite3_bind_text(pStmt, 5, sMessage ? sMessage : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 6, sNow ? sNow : "", -1, SQLITE_TRANSIENT);
	iRet = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	sqlite3_close(pDB);
	if ( sNow ) {
		xrtFree(sNow);
	}

	return iRet == SQLITE_DONE;
}

static xvalue procListLogsValue(void)
{
	sqlite3* pDB;
	sqlite3_stmt* pStmt = NULL;
	xvalue objRet = xvoCreateTable();
	xvalue arrItems = xvoCreateArray();
	int iCount = 0;

	pDB = procDBOpen();
	if ( pDB == NULL ) {
		xvoTableSetBool(objRet, "result", 6, FALSE);
		xvoTableSetText(objRet, "message", 7, "open sqlite failed", 0, FALSE);
		xvoTableSetValue(objRet, "items", 5, arrItems, TRUE);
		return objRet;
	}

	if ( sqlite3_prepare_v2(
		pDB,
		"SELECT id, level, source, cmd, msg_type, message, created_at "
		"FROM xtp_log ORDER BY id DESC LIMIT 120;",
		-1,
		&pStmt,
		NULL
	) != SQLITE_OK ) {
		sqlite3_close(pDB);
		xvoTableSetBool(objRet, "result", 6, FALSE);
		xvoTableSetText(objRet, "message", 7, "prepare query failed", 0, FALSE);
		xvoTableSetValue(objRet, "items", 5, arrItems, TRUE);
		return objRet;
	}

	while ( sqlite3_step(pStmt) == SQLITE_ROW ) {
		xvalue objItem = xvoCreateTable();
		char sID[32];
		char sMsgType[32];

		snprintf(sID, sizeof(sID), "%lld", (long long)sqlite3_column_int64(pStmt, 0));
		snprintf(sMsgType, sizeof(sMsgType), "%d", sqlite3_column_int(pStmt, 4));
		xvoTableSetText(objItem, "id", 2, sID, 0, FALSE);
		xvoTableSetText(objItem, "level", 5, (const char*)sqlite3_column_text(pStmt, 1) ? (const char*)sqlite3_column_text(pStmt, 1) : "", 0, FALSE);
		xvoTableSetText(objItem, "source", 6, (const char*)sqlite3_column_text(pStmt, 2) ? (const char*)sqlite3_column_text(pStmt, 2) : "", 0, FALSE);
		xvoTableSetText(objItem, "cmd", 3, (const char*)sqlite3_column_text(pStmt, 3) ? (const char*)sqlite3_column_text(pStmt, 3) : "", 0, FALSE);
		xvoTableSetText(objItem, "msg_type", 8, sMsgType, 0, FALSE);
		xvoTableSetText(objItem, "message", 7, (const char*)sqlite3_column_text(pStmt, 5) ? (const char*)sqlite3_column_text(pStmt, 5) : "", 0, FALSE);
		xvoTableSetText(objItem, "created_at", 10, (const char*)sqlite3_column_text(pStmt, 6) ? (const char*)sqlite3_column_text(pStmt, 6) : "", 0, FALSE);
		xvoArrayAppendValue(arrItems, objItem, TRUE);
		iCount++;
	}

	sqlite3_finalize(pStmt);
	sqlite3_close(pDB);
	xvoTableSetBool(objRet, "result", 6, TRUE);
	xvoTableSetInt(objRet, "count", 5, iCount);
	xvoTableSetValue(objRet, "items", 5, arrItems, TRUE);
	return objRet;
}

static bool procReplyJSONValue(XS_ResponseObject objResp, xvalue objVal)
{
	char* sJSON;
	bool bRet;

	if ( objVal == NULL ) {
		return FALSE;
	}

	sJSON = xrtStringifyJSON(objVal, FALSE, NULL);
	xvoUnref(objVal);
	if ( sJSON == NULL ) {
		return xsHttpJson(objResp, 500, "Internal Server Error", "{\"result\":false}") != 0;
	}

	bRet = xsHttpJson(objResp, 200, "OK", sJSON) != 0;
	xrtFree(sJSON);
	return bRet;
}

#endif
