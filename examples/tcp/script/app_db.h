#ifndef EXAMPLES_TCP_APP_DB_H
#define EXAMPLES_TCP_APP_DB_H

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
		"CREATE TABLE IF NOT EXISTS tcp_peer ("
			"peer_id TEXT PRIMARY KEY,"
			"name TEXT NOT NULL,"
			"online INTEGER NOT NULL,"
			"last_text TEXT NOT NULL,"
			"updated_at TEXT NOT NULL"
		");"
		"CREATE TABLE IF NOT EXISTS tcp_log ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"peer_id TEXT NOT NULL,"
			"name TEXT NOT NULL,"
			"text TEXT NOT NULL,"
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

static void procMakePeerID(void* pStream, char* sBuf, size_t iBufSize)
{
	snprintf(sBuf, iBufSize, "peer-%p", pStream);
}

static bool procUpsertPeer(const char* sPeerID, const char* sName, int iOnline, const char* sLastText)
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
		"INSERT INTO tcp_peer (peer_id, name, online, last_text, updated_at) VALUES (?, ?, ?, ?, ?) "
		"ON CONFLICT(peer_id) DO UPDATE SET name=excluded.name, online=excluded.online, last_text=excluded.last_text, updated_at=excluded.updated_at;",
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

	sqlite3_bind_text(pStmt, 1, sPeerID ? sPeerID : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 2, sName ? sName : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(pStmt, 3, iOnline);
	sqlite3_bind_text(pStmt, 4, sLastText ? sLastText : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 5, sNow ? sNow : "", -1, SQLITE_TRANSIENT);
	iRet = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	sqlite3_close(pDB);
	if ( sNow ) {
		xrtFree(sNow);
	}

	return iRet == SQLITE_DONE;
}

static bool procInsertLog(const char* sPeerID, const char* sName, const char* sText)
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
		"INSERT INTO tcp_log (peer_id, name, text, created_at) VALUES (?, ?, ?, ?);",
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

	sqlite3_bind_text(pStmt, 1, sPeerID ? sPeerID : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 2, sName ? sName : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 3, sText ? sText : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 4, sNow ? sNow : "", -1, SQLITE_TRANSIENT);
	iRet = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	sqlite3_close(pDB);
	if ( sNow ) {
		xrtFree(sNow);
	}

	return iRet == SQLITE_DONE;
}

static xvalue procPeerListValue(void)
{
	sqlite3* pDB;
	sqlite3_stmt* pStmt = NULL;
	xvalue objRet = xvoCreateTable();
	xvalue arrItems = xvoCreateArray();
	int iCount = 0;

	pDB = procDBOpen();
	if ( pDB == NULL ) {
		xvoTableSetBool(objRet, "result", 6, FALSE);
		xvoTableSetValue(objRet, "items", 5, arrItems, TRUE);
		return objRet;
	}
	if ( sqlite3_prepare_v2(
		pDB,
		"SELECT peer_id, name, online, last_text, updated_at FROM tcp_peer ORDER BY updated_at DESC;",
		-1,
		&pStmt,
		NULL
	) != SQLITE_OK ) {
		sqlite3_close(pDB);
		xvoTableSetBool(objRet, "result", 6, FALSE);
		xvoTableSetValue(objRet, "items", 5, arrItems, TRUE);
		return objRet;
	}

	while ( sqlite3_step(pStmt) == SQLITE_ROW ) {
		xvalue objItem = xvoCreateTable();

		xvoTableSetText(objItem, "peer_id", 7, (const char*)sqlite3_column_text(pStmt, 0) ? (const char*)sqlite3_column_text(pStmt, 0) : "", 0, FALSE);
		xvoTableSetText(objItem, "name", 4, (const char*)sqlite3_column_text(pStmt, 1) ? (const char*)sqlite3_column_text(pStmt, 1) : "", 0, FALSE);
		xvoTableSetInt(objItem, "online", 6, sqlite3_column_int(pStmt, 2));
		xvoTableSetText(objItem, "last_text", 9, (const char*)sqlite3_column_text(pStmt, 3) ? (const char*)sqlite3_column_text(pStmt, 3) : "", 0, FALSE);
		xvoTableSetText(objItem, "updated_at", 10, (const char*)sqlite3_column_text(pStmt, 4) ? (const char*)sqlite3_column_text(pStmt, 4) : "", 0, FALSE);
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

static xvalue procLogListValue(void)
{
	sqlite3* pDB;
	sqlite3_stmt* pStmt = NULL;
	xvalue objRet = xvoCreateTable();
	xvalue arrItems = xvoCreateArray();
	int iCount = 0;

	pDB = procDBOpen();
	if ( pDB == NULL ) {
		xvoTableSetBool(objRet, "result", 6, FALSE);
		xvoTableSetValue(objRet, "items", 5, arrItems, TRUE);
		return objRet;
	}
	if ( sqlite3_prepare_v2(
		pDB,
		"SELECT id, peer_id, name, text, created_at FROM tcp_log ORDER BY id DESC LIMIT 120;",
		-1,
		&pStmt,
		NULL
	) != SQLITE_OK ) {
		sqlite3_close(pDB);
		xvoTableSetBool(objRet, "result", 6, FALSE);
		xvoTableSetValue(objRet, "items", 5, arrItems, TRUE);
		return objRet;
	}

	while ( sqlite3_step(pStmt) == SQLITE_ROW ) {
		xvalue objItem = xvoCreateTable();
		char sID[32];

		snprintf(sID, sizeof(sID), "%lld", (long long)sqlite3_column_int64(pStmt, 0));
		xvoTableSetText(objItem, "id", 2, sID, 0, FALSE);
		xvoTableSetText(objItem, "peer_id", 7, (const char*)sqlite3_column_text(pStmt, 1) ? (const char*)sqlite3_column_text(pStmt, 1) : "", 0, FALSE);
		xvoTableSetText(objItem, "name", 4, (const char*)sqlite3_column_text(pStmt, 2) ? (const char*)sqlite3_column_text(pStmt, 2) : "", 0, FALSE);
		xvoTableSetText(objItem, "text", 4, (const char*)sqlite3_column_text(pStmt, 3) ? (const char*)sqlite3_column_text(pStmt, 3) : "", 0, FALSE);
		xvoTableSetText(objItem, "created_at", 10, (const char*)sqlite3_column_text(pStmt, 4) ? (const char*)sqlite3_column_text(pStmt, 4) : "", 0, FALSE);
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
