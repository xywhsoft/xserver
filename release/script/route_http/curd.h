


// 获取数据列表
void Request_List(XS_ServerObject objServer, XS_HostObject objHost, xnetconn* pConn, xhttpdreq* pReq)
{
	// 查询数据库
	XDO_Recordset rs = xdoSelect(G_DB, "SELECT * FROM test;");
	if ( rs == NULL ) {
		xrtHttpReplyFmt(pConn, 200, "Content-Type: application/json\r\n", "{\"result\": false, \"msg\": \"%s\"}", xCore->LastError);
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
	xrtHttpReplyJSON(pConn, 200, sRet);
	// 释放内存
	xrtFree(sRet);
	xvoUnref(tblRet);
}



// 添加数据
void Request_Add(XS_ServerObject objServer, XS_HostObject objHost, xnetconn* pConn, xhttpdreq* pReq)
{
	// 解析 body 域
	if ( pReq->iBodyLen == 0 ) {
		xrtHttpReplyJSON(pConn, 200, "{\"result\": false, \"msg\": \"Body 域不能为空！\"}");
		return;
	}
	xvalue objBody = xrtParseJSON(pReq->pBody, pReq->iBodyLen);
	if ( objBody->Type != XVO_DT_TABLE ) {
		xrtHttpReplyJSON(pConn, 200, "{\"result\": false, \"msg\": \"Body 域必须传递为 JSON 对象！\"}");
		xvoUnref(objBody);
		return;
	}
	// 检查 name 属性是否正确
	str sName = xvoTableGetText(objBody, "name", 4);
	if ( (sName == NULL) || (sName[0] == 0) ) {
		xrtHttpReplyJSON(pConn, 200, "{\"result\": false, \"msg\": \"参数 name 不能为空！\"}");
		xvoUnref(objBody);
		return;
	}
	// 写入数据库（使用转义防止 SQL 注入）
	int iAge = xvoTableGetInt(objBody, "age", 3);
	str sMail = xvoTableGetText(objBody, "mail", 4);
	str sdesc = xvoTableGetText(objBody, "desc", 4);
	str sNameEsc = sql_escape(sName, 0);
	str sMailEsc = sql_escape(sMail, 0);
	str sdescEsc = sql_escape(sdesc, 0);
	str sSQL = xrtFormat("INSERT INTO test (name, age, mail, desc) VALUES ('%s', %d, '%s', '%s')", sNameEsc, iAge, sMailEsc, sdescEsc);
	xdoExecute(G_DB, sSQL);
	xrtFree(sSQL);
	xrtFree(sNameEsc);
	xrtFree(sMailEsc);
	xrtFree(sdescEsc);
	xvoUnref(objBody);
	// 返回消息
	xrtHttpReplyJSON(pConn, 200, "{\"result\": true, \"msg\": \"添加数据成功！\"}");
}



// 删除数据
void Request_Del(XS_ServerObject objServer, XS_HostObject objHost, xnetconn* pConn, xhttpdreq* pReq)
{
	// 解析 body 域
	if ( pReq->iBodyLen == 0 ) {
		xrtHttpReplyJSON(pConn, 200, "{\"result\": false, \"msg\": \"Body 域不能为空！\"}");
		return;
	}
	xvalue objBody = xrtParseJSON(pReq->pBody, pReq->iBodyLen);
	if ( objBody->Type != XVO_DT_ARRAY ) {
		xrtHttpReplyJSON(pConn, 200, "{\"result\": false, \"msg\": \"Body 域必须传递为 JSON 数组！\"}");
		xvoUnref(objBody);
		return;
	}
	// 遍历数组删除数据
	int iCount = xvoArrayItemCount(objBody);
	for ( int i = 0; i < iCount; i++ ) {
		str id = xvoArrayGetText(objBody, i);
		str idEsc = sql_escape(id, 0);
		str sSQL = xrtFormat("DELETE FROM test WHERE id = '%s'", idEsc);
		xdoExecute(G_DB, sSQL);
		xrtFree(sSQL);
		xrtFree(idEsc);
	}
	xvoUnref(objBody);
	// 返回消息
	xrtHttpReplyJSON(pConn, 200, "{\"result\": true, \"msg\": \"数据删除成功！\"}");
}



// 编辑数据
void Request_Edit(XS_ServerObject objServer, XS_HostObject objHost, xnetconn* pConn, xhttpdreq* pReq)
{
	// 解析 body 域
	if ( pReq->iBodyLen == 0 ) {
		xrtHttpReplyJSON(pConn, 200, "{\"result\": false, \"msg\": \"Body 域不能为空！\"}");
		return;
	}
	xvalue objBody = xrtParseJSON(pReq->pBody, pReq->iBodyLen);
	if ( objBody->Type != XVO_DT_TABLE ) {
		xrtHttpReplyJSON(pConn, 200, "{\"result\": false, \"msg\": \"Body 域必须传递为 JSON 对象！\"}");
		xvoUnref(objBody);
		return;
	}
	// 检查 id 属性是否正确
	str sID = xvoTableGetText(objBody, "id", 2);
	if ( (sID == NULL) || (sID[0] == 0) ) {
		xrtHttpReplyJSON(pConn, 200, "{\"result\": false, \"msg\": \"参数 id 不能为空！\"}");
		xvoUnref(objBody);
		return;
	}
	// 检查 field 属性是否正确（只允许特定字段名防止注入）
	str sField = xvoTableGetText(objBody, "field", 5);
	if ( (sField == NULL) || (sField[0] == 0) ) {
		xrtHttpReplyJSON(pConn, 200, "{\"result\": false, \"msg\": \"参数 field 不能为空！\"}");
		xvoUnref(objBody);
		return;
	}
	// 白名单验证字段名（防止字段名注入）
	if ( strcmp(sField, "name") != 0 && strcmp(sField, "age") != 0 && 
	     strcmp(sField, "mail") != 0 && strcmp(sField, "desc") != 0 ) {
		xrtHttpReplyJSON(pConn, 200, "{\"result\": false, \"msg\": \"无效的字段名！\"}");
		xvoUnref(objBody);
		return;
	}
	// 修改属性（使用转义防止 SQL 注入）
	str sValue = xvoTableGetText(objBody, "value", 5);
	str sValueEsc = sql_escape(sValue, 0);
	str sIDEsc = sql_escape(sID, 0);
	str sSQL = xrtFormat("UPDATE test SET %s = '%s' WHERE id = '%s'", sField, sValueEsc, sIDEsc);
	xdoExecute(G_DB, sSQL);
	xrtFree(sSQL);
	xrtFree(sValueEsc);
	xrtFree(sIDEsc);
	xvoUnref(objBody);
	// 返回消息
	xrtHttpReplyJSON(pConn, 200, "{\"result\": true, \"msg\": \"数据编辑成功！\"}");
}


