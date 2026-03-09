


// 获取图表数据
void Request_Chart_Get(XS_ServerObject objServer, XS_HostObject objHost, xnetconn* pConn, xhttpdreq* pReq)
{
	xvalue option = xvoCreateTable();
	xvalue xAxis = xvoCreateTable();
	xvoTableSetValue(option, "xAxis", 5, xAxis, TRUE);
	xvoTableSetText(xAxis, "type", 4, "category", 0, FALSE);
	xvalue data = xvoCreateArray();
	xvoTableSetValue(xAxis, "data", 4, data, TRUE);
	xvoArrayAppendText(data, "Mon", 0, FALSE);
	xvoArrayAppendText(data, "Tue", 0, FALSE);
	xvoArrayAppendText(data, "Wed", 0, FALSE);
	xvoArrayAppendText(data, "Thu", 0, FALSE);
	xvoArrayAppendText(data, "Fri", 0, FALSE);
	xvoArrayAppendText(data, "Sat", 0, FALSE);
	xvoArrayAppendText(data, "Sun", 0, FALSE);
	xvalue yAxis = xvoCreateTable();
	xvoTableSetValue(option, "yAxis", 5, yAxis, TRUE);
	xvoTableSetText(yAxis, "type", 4, "value", 0, FALSE);
	xvalue series = xvoCreateArray();
	xvoTableSetValue(option, "series", 6, series, TRUE);
	xvalue arr0 = xvoCreateTable();
	xvoArrayAppendValue(series, arr0, TRUE);
	xvalue arr0_data = xvoCreateArray();
	xvoTableSetValue(arr0, "data", 4, arr0_data, TRUE);
	xvoTableSetText(arr0, "type", 4, "line", 0, FALSE);
	xvoTableSetBool(arr0, "smooth", 6, TRUE);
	for ( int i = 0; i < 7; i++ ) {
		int iVal = xrtRandRange(100, 1500);
		xvoArrayAppendInt(arr0_data, iVal);
	}
	
	// 生成 JSON
	size_t iRetSize = 0;
	char* sRet = xrtStringifyJSON(option, FALSE, &iRetSize);
	xrtHttpReplyJSON(pConn, 200, sRet);
	// 释放内存
	xrtFree(sRet);
	xvoUnref(option);
}


