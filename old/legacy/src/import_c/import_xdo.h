


void ImportXDO(TCCState* s)
{
	
	
	
	// 添加函数 - XDO
	tcc_add_symbol(s, "xdoCreate", xdoCreate);
	tcc_add_symbol(s, "xdoConnect", xdoConnect);
	tcc_add_symbol(s, "xdoDisconnect", xdoDisconnect);
	tcc_add_symbol(s, "xdoDestroy", xdoDestroy);
	tcc_add_symbol(s, "xdoExecute", xdoExecute);
	tcc_add_symbol(s, "xdoSelect", xdoSelect);
	tcc_add_symbol(s, "xdoInsert", xdoInsert);
	tcc_add_symbol(s, "xdoUpdate", xdoUpdate);
	tcc_add_symbol(s, "xdoDelete", xdoDelete);
	tcc_add_symbol(s, "xdoBeginTransaction", xdoBeginTransaction);
	tcc_add_symbol(s, "xdoCommit", xdoCommit);
	tcc_add_symbol(s, "xdoRollback", xdoRollback);
	tcc_add_symbol(s, "xrsFree", xrsFree);
	tcc_add_symbol(s, "xrsGetFieldCount", xrsGetFieldCount);
	tcc_add_symbol(s, "xrsGetRecordCount", xrsGetRecordCount);
	tcc_add_symbol(s, "xrsGetFieldName", xrsGetFieldName);
	tcc_add_symbol(s, "xrsGetFieldType", xrsGetFieldType);
	tcc_add_symbol(s, "xrsFieldIsPrimaryKey", xrsFieldIsPrimaryKey);
	tcc_add_symbol(s, "xrsFieldIsNotNull", xrsFieldIsNotNull);
	tcc_add_symbol(s, "xrsNext", xrsNext);
	tcc_add_symbol(s, "xrsGetValue", xrsGetValue);
	
	
	
	// 添加函数 - SQLite
	tcc_add_symbol(s, "xdoConnectSQLite", xdoConnectSQLite);
	
	
	
}


