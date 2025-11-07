


#include <sql.h>
#include <sqlext.h>



#pragma comment (lib, "odbc32")



// ODBC 数据库对象
typedef struct {
	char* Host;									// 数据库服务器地址 或 数据库连接串
	int Port;									// 数据库连接端口
	char* User;									// 数据库认证账号
	char* Pwd;									// 数据库认证密码
	char* DataBase;								// 默认数据库
	char* Charset;								// 默认编码
	char* LastError;							// 最后一次出错的描述
	int __pri_FreeError;						// 报错文本是否需要 free
	XDO_Driver objDriver;						// 数据库驱动对象指针
	void* objDB;								// 数据库连接对象
	
	/* ---------------- 以上字段继承自 XDO_Connect_Struct 结构，下面为自定义字段 ---------------- */
	
	SQLHENV hEnv;					// 环境句柄
	SQLHDBC hDBC;					// 连接句柄
	SQLHSTMT hStmt;					// 语句句柄
} XDO_ConnectStruct_ODBC, *XDO_Connect_ODBC;



// 列信息结构
typedef struct {
	str Name;						// 列名
	SQLLEN TypeDB;					// ODBC 类型值
	int TypeDrv;					// 驱动返回的类型值
	int PriKey;						// 是否为主键字段
	int NotNull;					// 是否可以为空
	SQLLEN AutoInc;					// 是否为自增字段
	SQLLEN MaxSize;					// 列数据最大长度
} XDO_FieldInfo_ODBC, *XDO_FieldObject_ODBC;



// ODBC 记录集对象
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
} XDO_RecordsetStruct_ODBC, *XDO_Recordset_ODBC;





// 获取错误描述
str ODBC_GetLastError(SQLHANDLE hHdr, SQLSMALLINT iType)
{
	SQLSMALLINT iRec = 1;
	SQLINTEGER  iError;
	char sState[SQL_SQLSTATE_SIZE + 1];
	char sMsg[256];
	str sError = xCore->sNull;
	while ( SQLGetDiagRec(iType, hHdr, iRec++, sState, &iError, sMsg, (SQLSMALLINT)sizeof(sMsg), (SQLSMALLINT*)NULL) == SQL_SUCCESS ) {
		str sNewError = xrtFormat("%s\n[%5.5s] %s (%d)", sError, sState, sMsg, iError);
		xrtFree(sError);
		sError = sNewError;
	}
	return sError;
}



// 关闭数据库连接
int ODBC_Disconnect(XDO_Connect_ODBC objConn)
{
	if ( objConn->hStmt ) {
		SQLFreeHandle(SQL_HANDLE_STMT, objConn->hStmt);
		objConn->hStmt = NULL;
	}
	if ( objConn->hDBC ) {
		if ( objConn->objDB ) {
			SQLDisconnect(objConn->hDBC);
			objConn->objDB = NULL;
		}
		SQLFreeHandle(SQL_HANDLE_DBC, objConn->hDBC);
		objConn->hDBC = NULL;
	}
	if ( objConn->hEnv ) {
		SQLFreeHandle(SQL_HANDLE_ENV, objConn->hEnv);
		objConn->hEnv = NULL;
	}
	return TRUE;
}



// 连接到数据库
int ODBC_Connect(XDO_Connect_ODBC objConn)
{
	// 创建 ODBC 数据库对象
	if ( SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &objConn->hEnv) == SQL_ERROR ) {
		// 对象创建失败
		xrtSetError("[XDO] SQLAllocHandle 创建 ODBC 数据库对象失败", FALSE);
		ODBC_Disconnect(objConn);
		return FALSE;
	}
	
	// 设置 ODBC 版本
	RETCODE rc = SQLSetEnvAttr(objConn->hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
	if ( rc == SQL_ERROR ) {
		xrtSetError("[XDO] SQLSetEnvAttr 设置版本失败", FALSE);
		ODBC_Disconnect(objConn);
		return FALSE;
	}
	
	// 创建 ODBC 连接对象
	rc = SQLAllocHandle(SQL_HANDLE_DBC, objConn->hEnv, &objConn->hDBC);
	if ( rc == SQL_ERROR ) {
		xrtSetError("[XDO] SQLAllocHandle 创建 ODBC 连接对象失败", FALSE);
		ODBC_Disconnect(objConn);
		return FALSE;
	}
	
	// 连接到数据库驱动
	rc = SQLDriverConnect(objConn->hDBC, NULL, objConn->Host, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
	if ( rc == SQL_ERROR ) {
		str sError = ODBC_GetLastError(objConn->hDBC, SQL_HANDLE_DBC);
		xrtSetError(xrtFormat("[XDO] SQLDriverConnect 连接到数据库失败：%s", sError), TRUE);
		xrtFree(sError);
		ODBC_Disconnect(objConn);
		return FALSE;
	}
	objConn->objDB = (void*)-1;
	
	// 创建 SQL 语句对象
	SQLAllocHandle(SQL_HANDLE_STMT, objConn->hDBC, &objConn->hStmt);
	if ( rc == SQL_ERROR ) {
		xrtSetError("[XDO] SQLAllocHandle 创建 SQL 语句对象失败", FALSE);
		ODBC_Disconnect(objConn);
		return FALSE;
	}
	
	return TRUE;
}



// 执行 SQL 语句（无返回值）
int ODBC_Execute(XDO_Connect_ODBC objConn, str sSQL)
{
	RETCODE RetCode = SQLExecDirect(objConn->hStmt, sSQL, SQL_NTS);
	if ( RetCode == SQL_SUCCESS ) {
		return TRUE;
	} else if ( RetCode == SQL_SUCCESS_WITH_INFO ) {
		str sError = ODBC_GetLastError(objConn->hStmt, SQL_HANDLE_STMT);
		xrtSetError(xrtFormat("[XDO] SQLExecDirect 警告 : %s", sError), TRUE);
		xrtFree(sError);
		return FALSE;
	} else if ( RetCode == SQL_ERROR ) {
		str sError = ODBC_GetLastError(objConn->hStmt, SQL_HANDLE_STMT);
		xrtSetError(xrtFormat("[XDO] SQLExecDirect 报错 : %s", sError), TRUE);
		xrtFree(sError);
		return FALSE;
	} else {
		xrtSetError(xrtFormat("[XDO] SQLExecDirect 返回未知代码 : %d", RetCode), TRUE);
		return FALSE;
	}
}



// 执行 SQL 语句（有返回值）
XDO_Recordset_ODBC ODBC_Select(XDO_Connect_ODBC objConn, str sSQL)
{
	RETCODE RetCode = SQLExecDirect(objConn->hStmt, sSQL, SQL_NTS);
	if ( RetCode == SQL_SUCCESS ) {
		
		// 获取字段数量
		int iColCount = 0;
		RETCODE rc = SQLNumResultCols(objConn->hStmt, (ptr)&iColCount);
		if ( rc == SQL_ERROR ) {
			str sError = ODBC_GetLastError(objConn->hStmt, SQL_HANDLE_STMT);
			xrtSetError(xrtFormat("[XDO] SQLNumResultCols 报错 : %s", sError), TRUE);
			xrtFree(sError);
			return NULL;
		}
		
		// 创建记录集对象
		XDO_Recordset_ODBC objRS = xrtMalloc(sizeof(XDO_RecordsetStruct_ODBC));
		if ( objRS == NULL ) {
			xrtSetError("[XDO] Memory allocate failed !", FALSE);
			return NULL;
		}
		objRS->LastError = xCore->sNull;
		objRS->__pri_FreeError = FALSE;
		objRS->objConn = (XDO_Connect)objConn;
		objRS->objDriver = objConn->objDriver;
		objRS->Line = 0;
		
		// 创建内存管理器
		objRS->RowData = SAMM_Create(sizeof(ptr) * iColCount);
		if ( objRS->RowData == NULL ) {
			xrtSetError("[XDO] SAMM_Create 创建行数据管理器失败", FALSE);
			return NULL;
		}
		objRS->ColInfo = SAMM_Create(sizeof(XDO_FieldInfo_ODBC));
		if ( objRS->ColInfo == NULL ) {
			xrtSetError("[XDO] SAMM_Create 创建列信息管理器失败", FALSE);
			return NULL;
		}
		SAMM_Malloc(objRS->ColInfo, iColCount);
		int iRet = SAMM_Append(objRS->ColInfo, iColCount);
		if ( iRet == 0 ) {
			xrtSetError("[XDO] SAMM_Append 申请内存失败", FALSE);
			return NULL;
		}
		
		// 获取列信息
		str* sd = calloc(sizeof(str), iColCount);
		int* sl = calloc(sizeof(int), iColCount);
		for ( int iCol = 0; iCol < iColCount; iCol++ ) {
			XDO_FieldObject_ODBC objField = SAMM_GetPtr_Inline(objRS->ColInfo, iCol + 1);
			// 获取列名
			objField->Name = xrtMalloc(1024);
			SQLColAttributeA(objConn->hStmt, iCol + 1, SQL_DESC_NAME, objField->Name, 1024, NULL, 0);
			// 获取列数据最大长度
			SQLColAttribute(objConn->hStmt, iCol + 1, SQL_DESC_DISPLAY_SIZE, NULL, 0, NULL, &objField->MaxSize);
			// 获取列的数据类型
			SQLColAttribute(objConn->hStmt, iCol + 1, SQL_DESC_TYPE, NULL, 0, NULL, &objField->TypeDB);
			// 获取是否可以为 NULL
			SQLLEN iRetSQL;
			SQLColAttribute(objConn->hStmt, iCol + 1, SQL_DESC_NULLABLE, NULL, 0, NULL, &iRetSQL);
			if ( iRetSQL == SQL_NO_NULLS ) {
				objField->NotNull = TRUE;
			} else {
				objField->NotNull = FALSE;
			}
			// 获取是否为自增字段
			SQLColAttribute(objConn->hStmt, iCol + 1, SQL_DESC_AUTO_UNIQUE_VALUE, NULL, 0, NULL, &objField->AutoInc);
			// 目前无法获取是否为主键字段，默认返回 FALSE
			objField->PriKey = FALSE;
			// 转换为通用类型ID
			switch ( objField->TypeDB ) {
				case SQL_INTEGER:
				case SQL_SMALLINT:
					objField->TypeDrv = XDO_DT_INT;
					break;
				case SQL_NUMERIC:
				case SQL_DECIMAL:
				case SQL_FLOAT:
				case SQL_REAL:
				case SQL_DOUBLE:
					objField->TypeDrv = XDO_DT_NUM;
					break;
				case SQL_TYPE_DATE:
					objField->TypeDrv = XDO_DT_DATE;
					break;
				case SQL_TYPE_TIME:
					objField->TypeDrv = XDO_DT_TIME;
					break;
				case SQL_DATETIME:
					objField->TypeDrv = XDO_DT_DATETIME;
					break;
				case SQL_TYPE_TIMESTAMP:
					// 时间戳，目前也返回 INT
					objField->TypeDrv = XDO_DT_INT;
					break;
				default :
					objField->TypeDrv = XDO_DT_TEXT;
			}
			// 绑定数据
			sd[iCol] = xrtMalloc(objField->MaxSize + 1);
			SQLBindCol(objConn->hStmt, iCol + 1, SQL_C_CHAR, sd[iCol], objField->MaxSize + 1, (ptr)&sl[iCol]);
			
			/* 调试输出
			printf("\tField Info : %d\n", iCol);
			printf("\t\tName : %s\n", objField->Name);
			printf("\t\tType : %d\n", objField->TypeDrv);
			printf("\t\tNotNull : %d\n", objField->NotNull);
			printf("\t\tAutoInc : %d\n", objField->AutoInc);
			printf("\t\tMaxSize : %d\n", objField->MaxSize);
			//*/
		}
		
		// 读取所有行
		int iRowCount = 0;
		while ( TRUE ) {
			if ( SQLFetch(objConn->hStmt) == SQL_NO_DATA_FOUND ) {
				break;
			} else {
				iRowCount++;
				iRet = SAMM_Append(objRS->RowData, 1);
				str* arrVal = SAMM_GetPtr(objRS->RowData, iRet);
				for ( int iCol = 0; iCol < iColCount; iCol++ ) {
					if ( sl[iCol] > 0 ) {
						/*
						if ( objConn->ConvChar ) {
							arrVal[iCol] = xCore_U2A(sd[iCol], sl[iCol]);
						} else {
							arrVal[iCol] = xCore_CopyStringA(sd[iCol], sl[iCol]);
						}
						*/
						arrVal[iCol] = xCore_CopyStringA(sd[iCol], sl[iCol]);
					} else {
						arrVal[iCol] = xCore->sNull;
					}
				}
			}
		}
		
		// 解绑所有列
		SQLFreeStmt(objConn->hStmt, SQL_UNBIND);
		SQLFreeStmt(objConn->hStmt, SQL_CLOSE);
		
		// 释放绑定数据指针
		/* 64位系统可能崩溃，原因未知
		for ( int iCol = 0; iCol < iColCount; iCol++ ) {
			xrtFree(sd[iCol]);
		}
		xrtFree(sd);
		xrtFree(sl);
		*/
		
		// 更新行数和列数
		objRS->RecordCount = iRowCount;
		objRS->FieldCount = iColCount;
		
		// 返回记录集对象
		return objRS;
	} else if ( RetCode == SQL_SUCCESS_WITH_INFO ) {
		str sError = ODBC_GetLastError(objConn->hStmt, SQL_HANDLE_STMT);
		xrtSetError(xrtFormat("[XDO] SQLExecDirect 警告 : %s", sError), TRUE);
		xrtFree(sError);
		return NULL;
	} else if ( RetCode == SQL_ERROR ) {
		str sError = ODBC_GetLastError(objConn->hStmt, SQL_HANDLE_STMT);
		xrtSetError(xrtFormat("[XDO] SQLExecDirect 报错 : %s", sError), TRUE);
		xrtFree(sError);
		return NULL;
	} else {
		xrtSetError(xrtFormat("[XDO] SQLExecDirect 返回未知代码 : %d", RetCode), TRUE);
		return NULL;
	}
}



// 释放记录集（DB_Select返回值）
int ODBC_RS_Free(XDO_Recordset_ODBC objRS)
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
int ODBC_RS_GetFieldCount(XDO_Recordset_ODBC objRS)
{
	return objRS->FieldCount;
}



// 获取结果集包含的记录数量
int ODBC_RS_GetRecordCount(XDO_Recordset_ODBC objRS)
{
	return objRS->RecordCount;
}



// 从结果集中获取字段名，字段序号从 0 开始
char* ODBC_RS_GetFieldName(XDO_Recordset_ODBC objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return xCore->sNull;}
	XDO_FieldObject_ODBC objField = SAMM_GetPtr(objRS->ColInfo, idx + 1);
	return objField->Name;
}



// 从结果集中获取字段数据类型，字段序号从 0 开始
int ODBC_RS_GetFieldType(XDO_Recordset_ODBC objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return XDO_DT_UNKNOWN;}
	XDO_FieldObject_ODBC objField = SAMM_GetPtr(objRS->ColInfo, idx + 1);
	return objField->TypeDrv;
}



// 判断结果集中指定字段是否为主键，字段序号从 0 开始
int ODBC_RS_FieldIsPrimaryKey(XDO_Recordset_ODBC objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return FALSE;}
	XDO_FieldObject_ODBC objField = SAMM_GetPtr(objRS->ColInfo, idx + 1);
	return objField->PriKey;
}



// 判断结果集中指定字段是否不能为空，字段序号从 0 开始
int ODBC_RS_FieldIsNotNull(XDO_Recordset_ODBC objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return FALSE;}
	XDO_FieldObject_ODBC objField = SAMM_GetPtr(objRS->ColInfo, idx + 1);
	return objField->NotNull;
}



// 记录集检索下一条记录
int ODBC_RS_Next(XDO_Recordset_ODBC objRS)
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
char* ODBC_RS_GetValue(XDO_Recordset_ODBC objRS, int idx)
{
	if ( idx >= objRS->FieldCount ) { return xCore->sNull;}
	str* arrVal = SAMM_GetPtr(objRS->RowData, objRS->Line);
	return arrVal[idx];
}







// 全局 ODBC 驱动
XDO_Driver_Struct XDO_Driver_ODBC = {
	(void*)ODBC_Connect,
	(void*)ODBC_Disconnect,
	(void*)ODBC_Execute,
	(void*)ODBC_Select,
	(void*)ODBC_RS_Free,
	(void*)ODBC_RS_GetFieldCount,
	(void*)ODBC_RS_GetRecordCount,
	(void*)ODBC_RS_GetFieldName,
	(void*)ODBC_RS_GetFieldType,
	(void*)ODBC_RS_FieldIsPrimaryKey,
	(void*)ODBC_RS_FieldIsNotNull,
	(void*)ODBC_RS_Next,
	(void*)ODBC_RS_GetValue,
};







// 连接到 ODBC 数据库
XDO_Connect xdoConnectODBC(char* sConnect)
{
	XDO_Connect objConn = xdoCreate(&XDO_Driver_ODBC);
	objConn->Host = sConnect;
	xdoConnect(objConn);
	return objConn;
}


