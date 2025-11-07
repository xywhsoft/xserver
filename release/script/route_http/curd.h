


// 获取数据列表
void Request_List(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	// 查询数据库
	XDO_Recordset rs = xdoSelect(G_DB, "SELECT * FROM test;");
	if ( rs == NULL ) {
		mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"%s\"}", xCore->LastError);
		return;
	}
	// 构建返回值
	xvalue tblRet = xvoCreateTable();
	xvalue arrList = xvoCreateArray();
	while ( xrsNext(rs) ) {
		xvalue tblRow = xvoCreateTable();
		xvoTableSetText(tblRow, "id", 2, xrsGetValue(rs, 0), 0, FALSE);
		xvoTableSetText(tblRow, "name", 4, xrsGetValue(rs, 1), 0, FALSE);
		xvoTableSetText(tblRow, "age", 3, xrsGetValue(rs, 2), 0, FALSE);
		xvoTableSetText(tblRow, "mail", 4, xrsGetValue(rs, 3), 0, FALSE);
		xvoTableSetText(tblRow, "desc", 4, xrsGetValue(rs, 4), 0, FALSE);
		xvoArrayAppendValue(arrList, tblRow, TRUE);
	}
	xvoTableSetInt(tblRet, "count", 5, xrsGetRecordCount(rs));
	xvoTableSetInt(tblRet, "code", 4, 0);
	xvoTableSetText(tblRet, "msg", 3, "平台列表查询成功！", 0, FALSE);
	xvoTableSetValue(tblRet, "data", 4, arrList, TRUE);
	// 释放记录集
	xrsFree(rs);
	// 生成 JSON
	size_t iRetSize = 0;
	char* sRet = xrtStringifyJSON(tblRet, FALSE, &iRetSize);
	http_reply(c, 200, "Content-Type: application/json\r\n", sRet, iRetSize);
}



// 添加数据
void Request_Add(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	// 解析 body 域
	if ( hm->body.len == 0 ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"Body 域不能为空！\"}", 0);
		return;
	}
	xvalue objBody = xrtParseJSON(hm->body.buf, hm->body.len);
	if ( objBody->Type != XVO_DT_TABLE ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"Body 域必须传递为 JSON 对象！\"}", 0);
		return;
	}
	// 检查 name 属性是否正确
	str sName = xvoTableGetText(objBody, "name", 4);
	if ( (sName == NULL) || (sName[0] == 0) ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"参数 name 不能为空！\"}", 0);
		return;
	}
	// 写入数据库
	int iAge = xvoTableGetInt(objBody, "age", 3);
	str sMail = xvoTableGetText(objBody, "mail", 4);
	str sdesc = xvoTableGetText(objBody, "desc", 4);
	str sSQL = xrtFormat("INSERT INTO test (name, age, mail, desc) VALUES ('%s', %d, '%s', '%s')", sName, iAge, sMail, sdesc);
	xdoExecute(G_DB, sSQL);
	xrtFree(sSQL);
	// 返回消息
	http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": true, \"msg\": \"添加数据成功！\"}", 0);
}



// 删除数据
void Request_Del(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	// 解析 body 域
	if ( hm->body.len == 0 ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"Body 域不能为空！\"}", 0);
		return;
	}
	xvalue objBody = xrtParseJSON(hm->body.buf, hm->body.len);
	if ( objBody->Type != XVO_DT_ARRAY ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"Body 域必须传递为 JSON 数组！\"}", 0);
		return;
	}
	// 遍历数组删除数据
	int iCount = xvoArrayItemCount(objBody);
	for ( int i = 0; i < iCount; i++ ) {
		str id = xvoArrayGetText(objBody, i);
		str sSQL = xrtFormat("DELETE FROM test WHERE id = %s", id);
		xdoExecute(G_DB, sSQL);
		xrtFree(sSQL);
	}
	// 返回消息
	http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": true, \"msg\": \"数据删除成功！\"}", 0);
}



// 编辑数据
void Request_Edit(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	// 解析 body 域
	if ( hm->body.len == 0 ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"Body 域不能为空！\"}", 0);
		return;
	}
	xvalue objBody = xrtParseJSON(hm->body.buf, hm->body.len);
	if ( objBody->Type != XVO_DT_TABLE ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"Body 域必须传递为 JSON 对象！\"}", 0);
		return;
	}
	// 检查 id 属性是否正确
	str sID = xvoTableGetText(objBody, "id", 2);
	if ( (sID == NULL) || (sID[0] == 0) ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"参数 id 不能为空！\"}", 0);
		return;
	}
	// 检查 field 属性是否正确
	str sField = xvoTableGetText(objBody, "field", 5);
	if ( (sField == NULL) || (sField[0] == 0) ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"参数 field 不能为空！\"}", 0);
		return;
	}
	// 修改属性
	str sValue = xvoTableGetText(objBody, "value", 5);
	str sSQL = xrtFormat("UPDATE test SET %s = '%s' WHERE id = %s", sField, sValue, sID);
	xdoExecute(G_DB, sSQL);
	xrtFree(sSQL);
	// 返回消息
	http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": true, \"msg\": \"数据编辑成功！\"}", 0);
}


