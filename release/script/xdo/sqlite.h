


// 全局定义
#define SQL_PREPARE_DEFAULT		SQLITE_PREPARE_PERSISTENT | SQLITE_PREPARE_DONT_LOG



// 列信息结构
typedef struct {
	char* Name;						// 列名
	int Type;						// 数据类型
	int PriKey;						// 是否为主键字段
	int NotNull;					// 是否可以为空
	int AutoInc;					// 是否为自增字段
} XDO_FieldInfo_SQLite, *XDO_FieldObject_SQLite;



// SQLite 记录集对象
typedef struct {
	char* LastError;							// 最后一次出错的描述
	int __pri_FreeError;						// 报错文本是否需要 free
	XDO_Connect objConn;						// 数据库连接对象指针
	XDO_Driver objDriver;						// 数据库驱动对象指针
	
	/* ---------------- 以上字段继承自 XDO_RecordsetStruct 结构，下面为自定义字段 ---------------- */
	
	int Line;							// 游标位置（默认为第 1 行）
	int RecordCount;					// 记录数量
	int FieldCount;						// 列数量
	SAMM_Object RowData;				// 行数据管理器
	SAMM_Object ColInfo;				// 列信息管理器
} XDO_RecordsetStruct_SQLite, *XDO_Recordset_SQLite;



// 连接到数据库
int SQLite_Connect(XDO_Connect objConn)
{
	sqlite3* objDB;
	int iRet = sqlite3_open(objConn->Host, &objDB);
	if ( iRet != SQLITE_OK ) {
		// 对象创建失败
		xdoSetError(objConn, xrtFormat("sqlite3_open error code : %d\n", iRet), TRUE);
		return FALSE;
	}
	objConn->objDB = objDB;
	return TRUE;
}



// 关闭数据库连接
int SQLite_Disconnect(XDO_Connect objConn)
{
	sqlite3_close(objConn->objDB);
	objConn->objDB = NULL;
	return TRUE;
}



// 执行 SQL 语句（无返回值）
int SQLite_Execute(XDO_Connect objConn, char* sSQL)
{
	char* sError = NULL;
	int iRet = sqlite3_exec(objConn->objDB, sSQL, 0, 0, &sError);
	if ( iRet != SQLITE_OK ) {
		xdoSetError(objConn, xrtFormat("SQL错误 : %s", sError), TRUE);
		sqlite3_free(sError);
		return FALSE;
	}
	return TRUE;
}



// 执行 SQL 语句（有返回值）
XDO_Recordset_SQLite SQLite_Select(XDO_Connect objConn, char* sSQL)
{
	// 编译 SQL 语句
	sqlite3_stmt* stmt = NULL;
	int iRet = sqlite3_prepare_v3(objConn->objDB, sSQL, -1, SQL_PREPARE_DEFAULT, &stmt, NULL);
	if ( iRet != SQLITE_OK ) {
		const char* sError = sqlite3_errmsg(objConn->objDB);
		xdoSetError(objConn, xrtFormat("SQL错误 (%d) : %s", iRet, sError), TRUE);
		return FALSE;
	}
	
	// 创建记录集对象
	XDO_Recordset_SQLite objRS = xrtMalloc(sizeof(XDO_RecordsetStruct_SQLite));
	if ( objRS == NULL ) {
		xdoSetError(objConn, "Memory allocate failed !", FALSE);
		return NULL;
	}
	objRS->LastError = xCore->sNull;
	objRS->__pri_FreeError = FALSE;
	objRS->objConn = objConn;
	objRS->objDriver = objConn->objDriver;
	objRS->Line = 0;
	objRS->RecordCount = 0;
	
	// 执行 SQL 语句
	int bFieldInfo = TRUE;
	while ( sqlite3_step(stmt) == SQLITE_ROW ) {
		// 获取列信息
		if ( bFieldInfo ) {
			objRS->FieldCount = sqlite3_column_count(stmt);
			
			// 创建内存管理器
			objRS->RowData = SAMM_Create(sizeof(ptr) * objRS->FieldCount);
			if ( objRS->RowData == NULL ) {
				xdoSetError(objConn, "SAMM_Create 创建行数据管理器失败", FALSE);
				return NULL;
			}
			objRS->ColInfo = SAMM_Create(sizeof(XDO_FieldInfo_SQLite));
			if ( objRS->ColInfo == NULL ) {
				xdoSetError(objConn, "SAMM_Create 创建列信息管理器失败", FALSE);
				return NULL;
			}
			SAMM_Malloc(objRS->ColInfo, objRS->FieldCount);
			int iRet = SAMM_Append(objRS->ColInfo, objRS->FieldCount);
			if ( iRet == 0 ) {
				xdoSetError(objConn, "SAMM_Append 申请内存失败", FALSE);
				return NULL;
			}
			
			// 获取列信息
			for ( int iCol = 0; iCol < objRS->FieldCount; iCol++ ) {
				XDO_FieldObject_SQLite objField = SAMM_GetPtr_Inline(objRS->ColInfo, iCol + 1);
				objField->Name = xrtCopyStr((char*)sqlite3_column_name(stmt, iCol), 0);
				int iType = sqlite3_column_type(stmt, iCol);
				if ( iType == SQLITE_INTEGER ) {
					objField->Type = XDO_DT_INT;
				} else if ( iType == SQLITE_FLOAT ) {
					objField->Type = XDO_DT_NUM;
				} else if ( iType == SQLITE_TEXT ) {
					objField->Type = XDO_DT_TEXT;
				} else if ( iType == SQLITE_BLOB ) {
					objField->Type = XDO_DT_BINARY;
				} else if ( iType == SQLITE_NULL ) {
					objField->Type = XDO_DT_NULL;
				} else {
					objField->Type = XDO_DT_UNKNOWN;
				}
				// 暂时不获取这些信息
				objField->PriKey = FALSE;
				objField->NotNull = FALSE;
				objField->AutoInc = FALSE;
			}
			
			bFieldInfo = FALSE;
		}
		
		// 读取所有行
		objRS->RecordCount++;
		iRet = SAMM_Append(objRS->RowData, 1);
		str* arrVal = SAMM_GetPtr(objRS->RowData, iRet);
		for ( int iCol = 0; iCol < objRS->FieldCount; iCol++ ) {
			const char* sVal = sqlite3_column_text(stmt, iCol);
			if ( sVal ) {
				arrVal[iCol] = xrtCopyStr((char*)sVal, 0);
			} else {
				arrVal[iCol] = xCore->sNull;
			}
		}
		
	}
	
	// 返回记录集对象
	return objRS;
}



// 释放记录集（DB_Select返回值）
int SQLite_RS_Free(XDO_Recordset_SQLite objRS)
{
	// 释放行数据内存
	if ( objRS->RowData ) {
		for ( int i = 1; i <= objRS->RowData->Count; i++ ) {
			str* arrVal = SAMM_GetPtr_Inline(objRS->RowData, i);
			for ( int iCol = 0; iCol < objRS->FieldCount; iCol++ ) {
				xrtFree(arrVal[iCol]);
			}
		}
		SAMM_Destroy(objRS->RowData);
	}
	// 释放字段信息内存
	if ( objRS->ColInfo ) {
		SAMM_Destroy(objRS->ColInfo);
	}
	// 释放对象本身
	xrtFree(objRS);
	return TRUE;
}



// 获取结果集包含的字段数量
int SQLite_RS_GetFieldCount(XDO_Recordset_SQLite objRS)
{
	return objRS->FieldCount;
}



// 获取结果集包含的记录数量
int SQLite_RS_GetRecordCount(XDO_Recordset_SQLite objRS)
{
	return objRS->RecordCount;
}



// 从结果集中获取字段名，字段序号从 0 开始
char* SQLite_RS_GetFieldName(XDO_Recordset_SQLite objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return xCore->sNull;}
	XDO_FieldObject_SQLite objField = SAMM_GetPtr(objRS->ColInfo, idx + 1);
	return objField->Name;
}



// 从结果集中获取字段数据类型，字段序号从 0 开始
int SQLite_RS_GetFieldType(XDO_Recordset_SQLite objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return XDO_DT_UNKNOWN;}
	XDO_FieldObject_SQLite objField = SAMM_GetPtr(objRS->ColInfo, idx + 1);
	return objField->Type;
}



// 判断结果集中指定字段是否为主键，字段序号从 0 开始
int SQLite_RS_FieldIsPrimaryKey(XDO_Recordset_SQLite objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return FALSE;}
	XDO_FieldObject_SQLite objField = SAMM_GetPtr(objRS->ColInfo, idx + 1);
	return objField->PriKey;
}



// 判断结果集中指定字段是否不能为空，字段序号从 0 开始
int SQLite_RS_FieldIsNotNull(XDO_Recordset_SQLite objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return FALSE;}
	XDO_FieldObject_SQLite objField = SAMM_GetPtr(objRS->ColInfo, idx + 1);
	return objField->NotNull;
}



// 记录集检索下一条记录
int SQLite_RS_Next(XDO_Recordset_SQLite objRS)
{
	if ( objRS->Line >= objRS->RecordCount ) {
		objRS->Line = 0;
		return FALSE;
	} else {
		objRS->Line++;
		return TRUE;
	}
}



// 结果集获取当前记录某一列的值，列号从 0 开始
char* SQLite_RS_GetValue(XDO_Recordset_SQLite objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return xCore->sNull;}
	str* arrVal = SAMM_GetPtr(objRS->RowData, objRS->Line);
	return arrVal[idx];
}







// 全局 SQLite 驱动
XDO_Driver_Struct XDO_Driver_SQLite = {
	(void*)SQLite_Connect,
	(void*)SQLite_Disconnect,
	(void*)SQLite_Execute,
	(void*)SQLite_Select,
	(void*)SQLite_RS_Free,
	(void*)SQLite_RS_GetFieldCount,
	(void*)SQLite_RS_GetRecordCount,
	(void*)SQLite_RS_GetFieldName,
	(void*)SQLite_RS_GetFieldType,
	(void*)SQLite_RS_FieldIsPrimaryKey,
	(void*)SQLite_RS_FieldIsNotNull,
	(void*)SQLite_RS_Next,
	(void*)SQLite_RS_GetValue,
};







// 连接到 SQLite3 数据库
XDO_Connect xdoConnectSQLite(char* sFile)
{
	XDO_Connect objConn = xdoCreate(&XDO_Driver_SQLite);
	objConn->Host = sFile;
	xdoConnect(objConn);
	return objConn;
}


