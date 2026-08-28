#ifndef EXAMPLES_CUSTOM_APP_DB_H
#define EXAMPLES_CUSTOM_APP_DB_H

static char G_sDBDir[1024] = "";
static char G_sDBFile[1024] = "";



static void procDBSetAppPath(const char* sAppPath)
{
	char* sDBDir;
	char* sDBFile;

	if ( sAppPath == NULL || sAppPath[0] == '\0' ) {
		return;
	}

	sDBDir = xrtPathJoin(4, (char*)sAppPath, "data", "db", "");
	if ( sDBDir == NULL ) {
		return;
	}

	sDBFile = xrtPathJoin(4, (char*)sAppPath, "data", "db", "main.db");
	if ( sDBFile == NULL ) {
		xrtFree(sDBDir);
		return;
	}

	snprintf(G_sDBDir, sizeof(G_sDBDir), "%s", sDBDir);
	snprintf(G_sDBFile, sizeof(G_sDBFile), "%s", sDBFile);
	xrtFree(sDBDir);
	xrtFree(sDBFile);
}

static char* procDBFile(void)
{
	char* sDBDir;
	char* sDBFile;

	if ( G_sDBFile[0] != '\0' ) {
		if ( G_sDBDir[0] != '\0' ) {
			xrtDirCreateAll(G_sDBDir);
		}
		return xrtCopyStr((str)G_sDBFile, 0);
	}

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
		"CREATE TABLE IF NOT EXISTS task ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"name TEXT NOT NULL,"
			"run_at TEXT NOT NULL,"
			"command TEXT NOT NULL,"
			"workdir TEXT NOT NULL,"
			"enabled INTEGER NOT NULL,"
			"last_run_at TEXT NOT NULL,"
			"last_status TEXT NOT NULL,"
			"created_at TEXT NOT NULL"
		");"
		"CREATE TABLE IF NOT EXISTS task_run ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"task_id INTEGER NOT NULL,"
			"task_name TEXT NOT NULL,"
			"status TEXT NOT NULL,"
			"exit_code INTEGER NOT NULL,"
			"output TEXT NOT NULL,"
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

static xvalue procTaskListValue(void)
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
		"SELECT id, name, run_at, command, workdir, enabled, last_run_at, last_status, created_at "
		"FROM task ORDER BY id DESC;",
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
		xvoTableSetText(objItem, "name", 4, (const char*)sqlite3_column_text(pStmt, 1) ? (const char*)sqlite3_column_text(pStmt, 1) : "", 0, FALSE);
		xvoTableSetText(objItem, "run_at", 6, (const char*)sqlite3_column_text(pStmt, 2) ? (const char*)sqlite3_column_text(pStmt, 2) : "", 0, FALSE);
		xvoTableSetText(objItem, "command", 7, (const char*)sqlite3_column_text(pStmt, 3) ? (const char*)sqlite3_column_text(pStmt, 3) : "", 0, FALSE);
		xvoTableSetText(objItem, "workdir", 7, (const char*)sqlite3_column_text(pStmt, 4) ? (const char*)sqlite3_column_text(pStmt, 4) : "", 0, FALSE);
		xvoTableSetInt(objItem, "enabled", 7, sqlite3_column_int(pStmt, 5));
		xvoTableSetText(objItem, "last_run_at", 11, (const char*)sqlite3_column_text(pStmt, 6) ? (const char*)sqlite3_column_text(pStmt, 6) : "", 0, FALSE);
		xvoTableSetText(objItem, "last_status", 11, (const char*)sqlite3_column_text(pStmt, 7) ? (const char*)sqlite3_column_text(pStmt, 7) : "", 0, FALSE);
		xvoTableSetText(objItem, "created_at", 10, (const char*)sqlite3_column_text(pStmt, 8) ? (const char*)sqlite3_column_text(pStmt, 8) : "", 0, FALSE);
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

static xvalue procRunListValue(void)
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
		"SELECT id, task_id, task_name, status, exit_code, output, created_at "
		"FROM task_run ORDER BY id DESC LIMIT 120;",
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
		char sTaskID[32];
		char sExitCode[32];

		snprintf(sID, sizeof(sID), "%lld", (long long)sqlite3_column_int64(pStmt, 0));
		snprintf(sTaskID, sizeof(sTaskID), "%lld", (long long)sqlite3_column_int64(pStmt, 1));
		snprintf(sExitCode, sizeof(sExitCode), "%d", sqlite3_column_int(pStmt, 4));
		xvoTableSetText(objItem, "id", 2, sID, 0, FALSE);
		xvoTableSetText(objItem, "task_id", 7, sTaskID, 0, FALSE);
		xvoTableSetText(objItem, "task_name", 9, (const char*)sqlite3_column_text(pStmt, 2) ? (const char*)sqlite3_column_text(pStmt, 2) : "", 0, FALSE);
		xvoTableSetText(objItem, "status", 6, (const char*)sqlite3_column_text(pStmt, 3) ? (const char*)sqlite3_column_text(pStmt, 3) : "", 0, FALSE);
		xvoTableSetText(objItem, "exit_code", 9, sExitCode, 0, FALSE);
		xvoTableSetText(objItem, "output", 6, (const char*)sqlite3_column_text(pStmt, 5) ? (const char*)sqlite3_column_text(pStmt, 5) : "", 0, FALSE);
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

static bool procAddTask(const char* sName, const char* sRunAt, const char* sCommand, const char* sWorkDir)
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
		"INSERT INTO task (name, run_at, command, workdir, enabled, last_run_at, last_status, created_at) "
		"VALUES (?, ?, ?, ?, 1, '', '', ?);",
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

	sqlite3_bind_text(pStmt, 1, sName ? sName : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 2, sRunAt ? sRunAt : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 3, sCommand ? sCommand : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 4, sWorkDir ? sWorkDir : "", -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, 5, sNow ? sNow : "", -1, SQLITE_TRANSIENT);
	iRet = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	sqlite3_close(pDB);
	if ( sNow ) {
		xrtFree(sNow);
	}

	return iRet == SQLITE_DONE;
}

static bool procDeleteTask(int64 iTaskID)
{
	sqlite3* pDB;
	sqlite3_stmt* pStmt = NULL;
	int iRet;

	pDB = procDBOpen();
	if ( pDB == NULL ) {
		return FALSE;
	}
	if ( sqlite3_prepare_v2(pDB, "DELETE FROM task WHERE id = ?;", -1, &pStmt, NULL) != SQLITE_OK ) {
		sqlite3_close(pDB);
		return FALSE;
	}

	sqlite3_bind_int64(pStmt, 1, iTaskID);
	iRet = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	sqlite3_close(pDB);
	return iRet == SQLITE_DONE;
}

static bool procToggleTask(int64 iTaskID, int iEnabled)
{
	sqlite3* pDB;
	sqlite3_stmt* pStmt = NULL;
	int iRet;

	pDB = procDBOpen();
	if ( pDB == NULL ) {
		return FALSE;
	}
	if ( sqlite3_prepare_v2(pDB, "UPDATE task SET enabled = ? WHERE id = ?;", -1, &pStmt, NULL) != SQLITE_OK ) {
		sqlite3_close(pDB);
		return FALSE;
	}

	sqlite3_bind_int(pStmt, 1, iEnabled);
	sqlite3_bind_int64(pStmt, 2, iTaskID);
	iRet = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	sqlite3_close(pDB);
	return iRet == SQLITE_DONE;
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

static bool procReplyResult(XS_ResponseObject objResp, bool bResult, const char* sMessage)
{
	xvalue objRet = xvoCreateTable();

	xvoTableSetBool(objRet, "result", 6, bResult);
	xvoTableSetText(objRet, "message", 7, sMessage ? sMessage : "", 0, FALSE);
	return procReplyJSONValue(objResp, objRet);
}

#endif
