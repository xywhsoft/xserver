


// 获取数据列表
void Request_List(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	// 查询数据库
	XDO_Recordset rs = xdoSelect(G_DB, "SELECT * FROM test;");
	if ( rs == NULL ) {
		mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"%s\"}", G_DB->LastError);
		return;
	}
	// 构建返回值
	XTE_Value tblRet = xteValueCreateTable();
	XTE_Value arrList = xteValueCreateArray();
	while ( xrsNext(rs) ) {
		XTE_Value tblRow = xteValueCreateTable();
		xteTableSetText(tblRow, "id", 2, xrsGetValue(rs, 0), FALSE);
		xteTableSetText(tblRow, "name", 4, xrsGetValue(rs, 1), FALSE);
		xteTableSetText(tblRow, "age", 3, xrsGetValue(rs, 2), FALSE);
		xteTableSetText(tblRow, "mail", 4, xrsGetValue(rs, 3), FALSE);
		xteTableSetText(tblRow, "desc", 4, xrsGetValue(rs, 4), FALSE);
		xteArrayAppendValue(arrList, tblRow, TRUE);
	}
	xteTableSetInt(tblRet, "count", 5, xrsGetRecordCount(rs));
	xteTableSetInt(tblRet, "code", 4, 0);
	xteTableSetText(tblRet, "msg", 3, "平台列表查询成功！", FALSE);
	xteTableSetValue(tblRet, "data", 4, arrList, TRUE);
	// 释放记录集
	xrsFree(rs);
	// 生成 JSON
	size_t iRetSize = 0;
	char* sRet = xteStringifyJSON(tblRet, FALSE, &iRetSize);
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
	XTE_Value objBody = xteParseJSON(hm->body.buf, hm->body.len);
	if ( objBody->MainType != XTE_DT_TABLE ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"Body 域必须传递为 JSON 对象！\"}", 0);
		return;
	}
	// 检查 name 属性是否正确
	str sName = xteTableGetText(objBody, "name", 4);
	if ( (sName == NULL) || (sName[0] == 0) ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"参数 name 不能为空！\"}", 0);
		return;
	}
	// 写入数据库
	int iAge = xteTableGetInt(objBody, "age", 3);
	str sMail = xteTableGetText(objBody, "mail", 4);
	str sdesc = xteTableGetText(objBody, "desc", 4);
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
	XTE_Value objBody = xteParseJSON(hm->body.buf, hm->body.len);
	if ( objBody->MainType != XTE_DT_ARRAY ) {
		http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"Body 域必须传递为 JSON 数组！\"}", 0);
		return;
	}
	// 遍历数组删除数据
	int iCount = xteArrayItemCount(objBody);
	for ( int i = 0; i < iCount; i++ ) {
		int id = xteArrayGetInt(objBody, i);
		str sSQL = xrtFormat("DELETE FROM test WHERE id = %d", id);
		xdoExecute(G_DB, sSQL);
		xrtFree(sSQL);
	}
	// 返回消息
	http_reply(c, 200, "Content-Type: application/json\r\n", "{\"result\": true, \"msg\": \"数据删除成功！\"}", 0);
}



// 编辑数据
void Request_Edit(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	XTE_Value option = xteValueCreateTable();
	XTE_Value xAxis = xteValueCreateTable();
	xteTableSetValue(option, "xAxis", 5, xAxis, TRUE);
	xteTableSetText(xAxis, "type", 4, "category", FALSE);
	XTE_Value data = xteValueCreateArray();
	xteTableSetValue(xAxis, "data", 4, data, TRUE);
	xteArrayAppendText(data, "Mon", FALSE);
	xteArrayAppendText(data, "Tue", FALSE);
	xteArrayAppendText(data, "Wed", FALSE);
	xteArrayAppendText(data, "Thu", FALSE);
	xteArrayAppendText(data, "Fri", FALSE);
	xteArrayAppendText(data, "Sat", FALSE);
	xteArrayAppendText(data, "Sun", FALSE);
	XTE_Value yAxis = xteValueCreateTable();
	xteTableSetValue(option, "yAxis", 5, yAxis, TRUE);
	xteTableSetText(yAxis, "type", 4, "value", FALSE);
	XTE_Value series = xteValueCreateArray();
	xteTableSetValue(option, "series", 6, series, TRUE);
	XTE_Value arr0 = xteValueCreateTable();
	xteArrayAppendValue(series, arr0, TRUE);
	
	// 生成 JSON
	size_t iRetSize = 0;
	char* sRet = xteStringifyJSON(option, FALSE, &iRetSize);
	http_reply(c, 200, "Content-Type: application/json\r\n", sRet, iRetSize);
}


