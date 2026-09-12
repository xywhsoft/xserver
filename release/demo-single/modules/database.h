/*
 * SQLite 示例模块
 *
 * 数据库文件位于应用根目录下的 db/main.db，不进入 wwwroot。这里只保留
 * 范例路由需要的初始化、预处理和释放操作，不建立 ORM 或数据访问框架。
 */



static sqlite3* G_DB = NULL;



static bool DB_Init(const char* sAppPath)
{
	str sDBPath;
	str sDBFile;
	int iResult;

	if ( sAppPath == NULL ) {
		return false;
	}
	sDBPath = xrtPathJoin(sAppPath, "db");
	if ( sDBPath == NULL ) {
		return false;
	}
	(void)xrtDirCreate(sDBPath);
	sDBFile = xrtPathJoin(sDBPath, "main.db");
	xrtFree(sDBPath);
	if ( sDBFile == NULL ) {
		return false;
	}

	/* FULLMUTEX 明确要求 SQLite 序列化同一连接上的并发操作，适配
	 * xServer 的多工作线程请求模型，而不在范例里再增加一把应用锁。 */
	iResult = sqlite3_open_v2(sDBFile, &G_DB,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
		NULL);
	if ( iResult == SQLITE_OK ) {
		printf("[demo] database opened: %s\n", sDBFile);
	}
	xrtFree(sDBFile);
	if ( iResult != SQLITE_OK ) {
		printf("[demo][error] sqlite open failed: %s\n",
			(G_DB ? sqlite3_errmsg(G_DB) : "unknown error"));
		if ( G_DB != NULL ) sqlite3_close(G_DB);
		G_DB = NULL;
		return false;
	}

	/* item 表供基础 JSON 和 RESTful 路由示例使用。 */
	(void)sqlite3_exec(G_DB,
		"CREATE TABLE IF NOT EXISTS item ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT, "
		"name TEXT NOT NULL, "
		"created INTEGER DEFAULT 0)",
		NULL, NULL, NULL);

	/* test 表保留旧版 dev/v1 范例的表格增删改查。 */
	(void)sqlite3_exec(G_DB,
		"CREATE TABLE IF NOT EXISTS test ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT, "
		"name TEXT NOT NULL, "
		"age INTEGER DEFAULT 0, "
		"mail TEXT DEFAULT '', "
		"\"desc\" TEXT DEFAULT '')",
		NULL, NULL, NULL);
	return true;
}



static void DB_Unit(void)
{
	if ( G_DB != NULL ) {
		sqlite3_close(G_DB);
		G_DB = NULL;
	}
}



static bool DBPrepare(const char* sSQL, sqlite3_stmt** ppStatement)
{
	if ( ppStatement == NULL ) {
		return false;
	}
	*ppStatement = NULL;
	if ( G_DB == NULL || sSQL == NULL ) {
		return false;
	}
	return sqlite3_prepare_v2(G_DB, sSQL, -1, ppStatement, NULL) == SQLITE_OK;
}



static int DBBindTextOrEmpty(
	sqlite3_stmt* pStatement,
	int iIndex,
	const char* sText
)
{
	if ( sText == NULL || sText[0] == '\0' ) {
		return sqlite3_bind_text(pStatement, iIndex, "", 0, SQLITE_STATIC);
	}
	return sqlite3_bind_text(pStatement, iIndex, sText, -1, SQLITE_TRANSIENT);
}



static void ReplyDBError(XS_HttpReq* pReq, const char* sMessage)
{
	if ( G_DB != NULL ) {
		(void)ReplyResult(pReq, false, sqlite3_errmsg(G_DB));
	} else {
		(void)ReplyResult(pReq, false,
			(sMessage ? sMessage : "database not ready"));
	}
}
