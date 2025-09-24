


// 获取设备列表 - 新接口
void Request_List(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	// 查询数据库
	XDO_Recordset rs = xdoSelect(G_DB, "SELECT * FROM test;");
	if ( rs == NULL ) {
		if ( c ) {
			mg_http_reply(c, 200, HTTP_JSON, "{\"result\": false, \"code\": 500, \"msg\": \"%s\"}", conn->LastError);
			return;
		} else {
			printf("Load Platform Cache Error : SQL Error (%s) !\n", conn->LastError);
			exit(1);
		}
	}
	
	// 读取参数
	int iSize = mg_http_get_var(&hm->query, "idel", Field_Format, 16);
	if ( iSize <= 0 ) {
		Field_Format[0] = 0;
	}
	int bAll = TRUE;
	if ( strcasecmp(Field_Format, "true") == 0 ) {
		bAll = FALSE;
	}
	// 返回数据
	XTE_Value tblRet = xteValueCreateTable();
	xteTableSetInt(tblRet, "code", 4, 0);
	xteTableSetText(tblRet, "msg", 3, "设备列表获取成功", FALSE);
	XTE_Value arrDev = Device_GetList(bAll, FALSE);
	xteTableSetValue(tblRet, "data", 4, arrDev, TRUE);
	xteTableSetInt(tblRet, "count", 5, xteArrayItemCount(arrDev));
	//xteStringifyJSON_File("c:\\1.txt", tblRet, FALSE);
	size_t iRetSize = 0;
	char* sRet = xteStringifyJSON(tblRet, FALSE, &iRetSize);
	http_reply(c, 200, HTTP_JSON, sRet, iRetSize);
	//xrtFree(sRet);
}


