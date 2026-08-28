/*
 * sqlite 门控探针（XS_USE_SQLITE 构建变体）：内存库建表/插入/查询回路。
 * 非 sqlite 构建下此服务不配置（xs.json 默认不含）。
 */
#include <xsbase.h>
#include <sqlite3.h>
#include <stdio.h>

void ServiceInit(XS_HostInfo* pHost)
{
	sqlite3* pDb = NULL;
	char* sErr = NULL;
	int iCount = -1;

	(void)pHost;
	if ( sqlite3_open(":memory:", &pDb) == SQLITE_OK ) {
		if ( sqlite3_exec(pDb, "CREATE TABLE t(id INTEGER PRIMARY KEY, v TEXT)", NULL, NULL, &sErr) == SQLITE_OK &&
		     sqlite3_exec(pDb, "INSERT INTO t(v) VALUES ('a'),('b'),('c')", NULL, NULL, &sErr) == SQLITE_OK ) {
			sqlite3_stmt* pStmt = NULL;

			if ( sqlite3_prepare_v2(pDb, "SELECT COUNT(*) FROM t", -1, &pStmt, NULL) == SQLITE_OK ) {
				if ( sqlite3_step(pStmt) == SQLITE_ROW ) {
					iCount = sqlite3_column_int(pStmt, 0);
				}
				sqlite3_finalize(pStmt);
			}
		}
		if ( sErr != NULL ) {
			sqlite3_free(sErr);
		}
		sqlite3_close(pDb);
	}
	printf("[sqlite] probe %s (rows=%d)\n", iCount == 3 ? "ok" : "FAIL", iCount);
}

void ServiceUnit(XS_HostInfo* pHost)
{
	(void)pHost;
}
