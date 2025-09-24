#ifndef MYSQL_DRIVER_H
#define MYSQL_DRIVER_H



// 连接池结构
typedef struct {
    XDO_Connect* connections;  // 连接数组
    int pool_size;             // 连接池大小
    int* in_use;               // 标记哪些连接正在使用
} MySQLConnectionPool;


// MySQL 记录集对象
typedef struct {
	char* LastError;							// 最后一次出错的描述
	int __pri_FreeError;						// 报错文本是否需要 free
	XDO_Connect objConn;						// 数据库连接对象指针
	XDO_Driver objDriver;						// 数据库驱动对象指针
	
	/* ---------------- 以上字段继承自 XDO_RecordsetStruct 结构，下面为自定义字段 ---------------- */
	
	int Line;									// 游标位置（默认为第 1 行）
	MYSQL_RES* objRS;							// 记录集对象
	MYSQL_FIELD* objFields;						// 列信息数组
	MYSQL_ROW arrRow;							// 当前游标行数据内容（字符串数组）
	int RecordCount;							// 记录数量
	int FieldCount;								// 列数量
	PAMM_Object AutoFreeMEM;					// 存储临时申请的内存指针，用于在释放记录集时同步释放
} XDO_RecordsetStruct_MySQL, *XDO_Recordset_MySQL;


// 初始化连接池（静态内联实现）
static inline MySQLConnectionPool* MySQL_CreateConnectionPool(int pool_size,
                                                           const char* host,
                                                           int port,
                                                           const char* user,
                                                           const char* pwd,
                                                           const char* database,
                                                           const char* charset) {
    printf("Allocating pool structure...\n");
    MySQLConnectionPool* pool = (MySQLConnectionPool*)xrtMalloc(sizeof(MySQLConnectionPool));
    if (!pool) {
        printf("ERROR: Failed to allocate pool structure\n");
        return NULL;
    }
    memset(pool, 0, sizeof(MySQLConnectionPool));

    printf("Allocating connections array...\n");
    pool->connections = (XDO_Connect*)xrtMalloc(sizeof(XDO_Connect) * pool_size);
    if (!pool->connections) {
        printf("ERROR: Failed to allocate connections array\n");
        xrtFree(pool);
        return NULL;
    }
    memset(pool->connections, 0, sizeof(XDO_Connect) * pool_size);

    printf("Allocating in_use array...\n");
    pool->in_use = (int*)calloc(pool_size, sizeof(int));
    if (!pool->in_use) {
        printf("ERROR: Failed to allocate in_use array\n");
        xrtFree(pool->connections);
        xrtFree(pool);
        return NULL;
    }

    pool->pool_size = pool_size;
    printf("Pool memory allocated, creating %d connections...\n", pool_size);

    int i = 0;
    for (i = 0; i < pool_size; i++) {
        // 仅创建连接对象但不立即连接
        pool->connections[i] = xdoConnectMySQL(host, port, user, pwd, database, charset);
        if (!pool->connections[i]) {
            printf("ERROR: xdoConnectMySQL failed for connection %d\n", i);
            goto cleanup;
        }

        // 不在此处调用xdoConnect，改为在首次使用时连接
        pool->in_use[i] = 0;
    }

    printf("Connection pool created successfully (connections will be established on first use)\n");
    return pool;

cleanup:
    for (int j = 0; j < i; j++) {
        if (pool->connections[j]) {
            xdoDestroy(pool->connections[j]);
        }
    }
    xrtFree(pool->connections);
    xrtFree(pool->in_use);
    xrtFree(pool);
    return NULL;
}

// 从池中获取连接
static inline XDO_Connect MySQL_GetConnection(MySQLConnectionPool* pool) {
    if (!pool) return NULL;

    for (int i = 0; i < pool->pool_size; i++) {
        if (!pool->in_use[i]) {
            pool->in_use[i] = 1;

            // 在首次使用时建立连接
            if (!pool->connections[i]->objDB) {
                printf("Establishing connection %d...\n", i);
                if (!xdoConnect(pool->connections[i])) {
                    printf("ERROR: xdoConnect failed for connection %d. Error: %s\n",
                          i, pool->connections[i]->LastError);
                    pool->in_use[i] = 0; // 释放标记
                    return NULL;
                }
            }

            return pool->connections[i];
        }
    }
    return NULL; // 所有连接都在使用中
}

// 释放连接回池中（静态内联实现）
static inline void MySQL_ReleaseConnection(MySQLConnectionPool* pool, XDO_Connect conn) {
    if (conn) {
        for (int i = 0; i < pool->pool_size; i++) {
            if (pool->connections[i] == conn) {
                pool->in_use[i] = 0;
                break;
            }
        }
    }
}

// 销毁连接池（静态内联实现）
static inline void MySQL_DestroyConnectionPool(MySQLConnectionPool* pool) {
    if (!pool) return;

    for (int i = 0; i < pool->pool_size; i++) {
        xdoDestroy(pool->connections[i]);
    }
    xrtFree(pool->connections);
    xrtFree(pool->in_use);
    xrtFree(pool);
}


/* ==================== 基础功能 ==================== */

// 连接到数据库
int MySQL_Connect(XDO_Connect objConn)
{
	MYSQL* objSQL = mysql_init(NULL);
	if ( objSQL == NULL ) {
		// 对象创建失败
		xdoSetError(objConn, "mysql_init failed !", FALSE);
		return FALSE;
	}

	if ( mysql_real_connect(objSQL, objConn->Host, objConn->User, objConn->Pwd, objConn->DataBase, objConn->Port, NULL, 0) == NULL ) {
		// 数据库连接失败
		xdoSetError(objConn, mysql_error(objSQL), FALSE);
		mysql_close(objSQL);
		return FALSE;
	}
	objConn->objDB = objSQL;
	mysql_set_character_set(objSQL, objConn->Charset);
	return TRUE;
}


// 关闭数据库连接
int MySQL_Disconnect(XDO_Connect objConn)
{
	mysql_close(objConn->objDB);
	objConn->objDB = NULL;
	return TRUE;
}


// 执行 SQL 语句（无返回值）
int MySQL_Execute(XDO_Connect objConn, char* sSQL)
{
	int iRet = mysql_query(objConn->objDB, sSQL);
	if ( iRet != 0 ) {
		xdoSetError(objConn, (char*)mysql_error(objConn->objDB), FALSE);
		return FALSE;
	}
	return TRUE;
}


// 执行 SQL 语句（有返回值）
XDO_Recordset_MySQL MySQL_Select(XDO_Connect objConn, char* sSQL)
{
	int iRet = mysql_query(objConn->objDB, sSQL);
	if ( iRet != 0 ) {
		xdoSetError(objConn, (char*)mysql_error(objConn->objDB), FALSE);
		return NULL;
	}
	XDO_Recordset_MySQL objRS = xrtMalloc(sizeof(XDO_RecordsetStruct_MySQL));
	if ( objRS == NULL ) {
		xdoSetError(objConn, "Memory allocate failed !", FALSE);
		return NULL;
	}
	objRS->LastError = xCore->sNull;
	objRS->__pri_FreeError = FALSE;
	objRS->objConn = objConn;
	objRS->objDriver = objConn->objDriver;
	objRS->Line = 0;
	objRS->objRS = mysql_store_result(objConn->objDB);
	objRS->objFields = mysql_fetch_fields(objRS->objRS);
	objRS->arrRow = NULL;
	objRS->RecordCount = mysql_num_rows(objRS->objRS);
	objRS->FieldCount = mysql_num_fields(objRS->objRS);
	objRS->AutoFreeMEM = PAMM_Create();
	return objRS;
}


// 记录集检索下一条记录
int MySQL_RS_Next(XDO_Recordset_MySQL objRS)
{
	objRS->arrRow = mysql_fetch_row(objRS->objRS);
	if (objRS->arrRow ) {
		objRS->Line++;
		return TRUE;
	} else {
		// 记录游标触底目前还没有添加处理代码，仅能返回失败状态
		objRS->Line = 0;
		return FALSE;
	}
}


// 结果集获取当前记录某一列的值，列号从 0 开始
char* MySQL_RS_GetValue(XDO_Recordset_MySQL objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return xCore->sNull;}
	return objRS->arrRow[idx] ? objRS->arrRow[idx] : (char*)xCore->sNull;
}


// 从结果集中获取字段名，字段序号从 0 开始
char* MySQL_RS_GetFieldName(XDO_Recordset_MySQL objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return xCore->sNull; }
	return (char*)objRS->objFields[idx].name;
}


// 释放记录集（DB_Select返回值）
int MySQL_RS_Free(XDO_Recordset_MySQL objRS)
{
	mysql_free_result(objRS->objRS);
	for ( int i = 1; i <= objRS->AutoFreeMEM->Count; i++ ) {
		void* pMem = PAMM_GetVal_Inline(objRS->AutoFreeMEM, i);
		xrtFree(pMem);
	}
	PAMM_Destroy(objRS->AutoFreeMEM);
	xrtFree(objRS);
	return TRUE;
}


// 插入数据，返回插入ID
static long MySQL_Insert(XDO_Connect objConn, const char* table, const char* columns, const char* values) {
    if (!objConn || !objConn->objDB || !table || !values) {
        xdoSetError(objConn, "Invalid parameters", FALSE);
        return 0;
    }

    char* sql = NULL;
    XDO_Recordset_MySQL rs = NULL;
    long insert_id = 0;

    do {
        // 构建SQL
        sql = columns ?
              xrtFormat("INSERT INTO %s (%s) VALUES (%s)", table, columns, values) :
              xrtFormat("INSERT INTO %s VALUES (%s)", table, values);

        if (!sql) {
            xdoSetError(objConn, "SQL allocation failed", FALSE);
            break;
        }

        // 执行插入
        if (!MySQL_Execute(objConn, sql)) {
            const char* error = mysql_error(objConn->objDB);
            printf("SQL Error: %s\n", error);
            xdoSetError(objConn, mysql_error(objConn->objDB), FALSE);
            break;
        }

        // 获取影响行数和插入ID
        int affected_rows = mysql_affected_rows(objConn->objDB);
        insert_id = mysql_insert_id(objConn->objDB);

        if (insert_id == 0) {
            if (affected_rows > 0) {
                // 插入成功但无自增ID，返回-1表示成功
                insert_id = -1;
            } else {
                // 没有行被影响
                insert_id = 0;
            }
        }
    } while (0);

    // 资源清理
    if (rs) MySQL_RS_Free(rs);
    if (sql) xrtFree(sql);

    return insert_id;
}


// 更新数据，返回影响行数
static int MySQL_Update(XDO_Connect objConn, const char* table, const char* set_clause, const char* where) {
    if (!objConn || !objConn->objDB || !table || !set_clause) {
        xdoSetError(objConn, "Invalid parameters", FALSE);
        return 0;
    }

    char* sql = NULL;

    // 构建SQL
    sql = where ?
          xrtFormat("UPDATE %s SET %s WHERE %s", table, set_clause, where) :
          xrtFormat("UPDATE %s SET %s", table, set_clause);

    if (!sql) {
        xdoSetError(objConn, "Failed to allocate SQL string", FALSE);
        return 0;
    }

    // 执行更新
    if (!MySQL_Execute(objConn, sql)) {
        xdoSetError(objConn, mysql_error(objConn->objDB), FALSE);
        xrtFree(sql);
        return 0;
    }

    // 获取影响行数
    int affected = (int)mysql_affected_rows(objConn->objDB);

    xrtFree(sql);
    return affected;
}


// 删除数据，返回影响行数
static int MySQL_Delete(XDO_Connect objConn, const char* table, const char* where) {
    // 参数严格校验
    if (!objConn || !objConn->objDB || !table) {
        xdoSetError(objConn, "Invalid parameters", FALSE);
        return 0;
    }

    char* sql = NULL;
    XDO_Recordset_MySQL rs = NULL;
    int affected = 0;

    // 构建SQL
    sql = where ? xrtFormat("DELETE FROM %s WHERE %s", table, where) :
                  xrtFormat("DELETE FROM %s", table);

    if (!sql) {
        xdoSetError(objConn, "SQL allocation failed", FALSE);
        goto cleanup;
    }

    // 执行删除
    if (!MySQL_Execute(objConn, sql)) {
        xdoSetError(objConn, mysql_error(objConn->objDB), FALSE);
        goto cleanup;
    }

    // 获取影响行数
    affected = (int)mysql_affected_rows(objConn->objDB);

cleanup:
    // 安全释放资源
    if (rs) MySQL_RS_Free(rs);
    if (sql) xrtFree(sql);

    return affected;
}


// 将查询结果转为JSON格式
static char* MySQL_QueryToJson(XDO_Recordset_MySQL objRS) {
    if (!objRS) return NULL;

    PAMM_Object mem = PAMM_Create();
    char* json = strdup("[");
    PAMM_Append(mem, json);

    int first_row = 1;
    while (MySQL_RS_Next(objRS)) {
        if (!first_row) {
            char* temp = xrtFormat("%s,", json);
            PAMM_Append(mem, temp);
            xrtFree(json);
            json = temp;
        }
        first_row = 0;

        char* row = strdup("{");
        PAMM_Append(mem, row);

        for (int i = 0; i < objRS->FieldCount; i++) {
            char* field_name = MySQL_RS_GetFieldName(objRS, i);
            char* field_value = MySQL_RS_GetValue(objRS, i);

            char* field = NULL;
            if (i > 0) {
                field = xrtFormat("%s\"%s\":\"%s\"", row, field_name, field_value);
            } else {
                field = xrtFormat("\"%s\":\"%s\"", field_name, field_value);
            }

            xrtFree(row);
            row = field;
            PAMM_Append(mem, row);

            if (i < objRS->FieldCount - 1) {
                char* temp = xrtFormat("%s,", row);
                xrtFree(row);
                row = temp;
                PAMM_Append(mem, row);
            }
        }

        char* temp = xrtFormat("%s}", row);
        xrtFree(row);
        row = temp;
        PAMM_Append(mem, row);

        char* new_json = xrtFormat("%s%s", json, row);
        xrtFree(json);
        json = new_json;
        PAMM_Append(mem, json);
    }

    char* final_json = xrtFormat("%s]", json);
    PAMM_Append(mem, final_json);
    xrtFree(json);

    // 转移内存所有权
    for (int i = 1; i <= mem->Count; i++) {
        if (PAMM_GetVal_Inline(mem, i) != final_json) {
            xrtFree(PAMM_GetVal_Inline(mem, i));
        }
    }

    PAMM_Destroy(mem);
    return final_json;
}

// 事务处理
static int MySQL_BeginTransaction(XDO_Connect objConn) {
    return MySQL_Execute(objConn, "START TRANSACTION");
}

static int MySQL_Commit(XDO_Connect objConn) {
    return MySQL_Execute(objConn, "COMMIT");
}

static int MySQL_Rollback(XDO_Connect objConn) {
    return MySQL_Execute(objConn, "ROLLBACK");
}


// 获取结果集包含的字段数量
int MySQL_RS_GetFieldCount(XDO_Recordset_MySQL objRS)
{
	return objRS->FieldCount;
}


// 获取结果集包含的记录数量
int MySQL_RS_GetRecordCount(XDO_Recordset_MySQL objRS)
{
	return objRS->RecordCount;
}


// 从结果集中获取字段数据类型，字段序号从 0 开始
int MySQL_RS_GetFieldType(XDO_Recordset_MySQL objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return XDO_DT_UNKNOWN;}
	switch ( objRS->objFields[idx].type ) {
		case MYSQL_TYPE_TINY:
		case MYSQL_TYPE_SHORT:
		case MYSQL_TYPE_LONG:
		case MYSQL_TYPE_LONGLONG:
		case MYSQL_TYPE_INT24:
			return XDO_DT_INT;
		case MYSQL_TYPE_FLOAT:
		case MYSQL_TYPE_DOUBLE:
		case MYSQL_TYPE_DECIMAL:
		case MYSQL_TYPE_NEWDECIMAL:
			return XDO_DT_NUM;
		case MYSQL_TYPE_VARCHAR:
		case MYSQL_TYPE_JSON:
		case MYSQL_TYPE_TINY_BLOB:
		case MYSQL_TYPE_MEDIUM_BLOB:
		case MYSQL_TYPE_LONG_BLOB:
		case MYSQL_TYPE_BLOB:
		case MYSQL_TYPE_VAR_STRING:
		case MYSQL_TYPE_STRING:
			return XDO_DT_TEXT;
		case MYSQL_TYPE_DATE:
			return XDO_DT_DATE;
		case MYSQL_TYPE_TIME:
			return XDO_DT_TIME;
		case MYSQL_TYPE_DATETIME:
			return XDO_DT_DATETIME;
		case MYSQL_TYPE_TIMESTAMP:
		case MYSQL_TYPE_TIMESTAMP2:
		case MYSQL_TYPE_YEAR:
			// 时间戳和年，目前也返回 INT
			return XDO_DT_INT;
		default :
			return XDO_DT_UNKNOWN;
	}
	/* 未处理的类型：
		MYSQL_TYPE_NEWDATE
		MYSQL_TYPE_BIT
		MYSQL_TYPE_DATETIME2
		MYSQL_TYPE_TIME2
		MYSQL_TYPE_ENUM
		MYSQL_TYPE_SET
		MYSQL_TYPE_GEOMETRY
	*/
}


// 判断结果集中指定字段是否为主键，字段序号从 0 开始
int MySQL_RS_FieldIsPrimaryKey(XDO_Recordset_MySQL objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return FALSE;}
	if ( objRS->objFields[idx].flags & PRI_KEY_FLAG ) {
		return TRUE;
	} else {
		return FALSE;
	}
}


// 判断结果集中指定字段是否不能为空，字段序号从 0 开始
int MySQL_RS_FieldIsNotNull(XDO_Recordset_MySQL objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return FALSE;}
	if ( objRS->objFields[idx].flags & NOT_NULL_FLAG ) {
		return TRUE;
	} else {
		return FALSE;
	}
}


// 全局 MySQL 驱动
XDO_Driver_Struct XDO_Driver_MYSQL = {
	(void*)MySQL_Connect,
	(void*)MySQL_Disconnect,
	(void*)MySQL_Execute,
	(void*)MySQL_Select,
	(void*)MySQL_RS_Free,
	(void*)MySQL_RS_GetFieldCount,
	(void*)MySQL_RS_GetRecordCount,
	(void*)MySQL_RS_GetFieldName,
	(void*)MySQL_RS_GetFieldType,
	(void*)MySQL_RS_FieldIsPrimaryKey,
	(void*)MySQL_RS_FieldIsNotNull,
	(void*)MySQL_RS_Next,
	(void*)MySQL_RS_GetValue,
	(void*)MySQL_Insert,
    (void*)MySQL_Update,
    (void*)MySQL_Delete,
    (void*)MySQL_BeginTransaction,
    (void*)MySQL_Commit,
    (void*)MySQL_Rollback
};


// 连接到 MySQL 数据库
XDO_Connect xdoConnectMySQL(const char* sHost, const int iPort, const char* sUser, const char* sPwd, const char* sDataBase, const char* sCharset)
{
	XDO_Connect objConn = xdoCreate(&XDO_Driver_MYSQL);
	objConn->Host = sHost;
	objConn->Port = iPort;
	objConn->User = sUser;
	objConn->Pwd = sPwd;
	objConn->DataBase = sDataBase;
	objConn->Charset = sCharset;
	xdoConnect(objConn);
	return objConn;
}

#endif
