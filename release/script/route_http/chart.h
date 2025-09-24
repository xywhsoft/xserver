


// 获取图表数据
void Request_Chart_Get(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
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
	XTE_Value arr0_data = xteValueCreateArray();
	xteTableSetValue(arr0, "data", 4, arr0_data, TRUE);
	xteTableSetText(arr0, "type", 4, "line", FALSE);
	xteTableSetBool(arr0, "smooth", 6, TRUE);
	for ( int i = 0; i < 7; i++ ) {
		int iVal = xrtRandRange(100, 1500);
		xteArrayAppendInt(arr0_data, iVal);
	}
	
	// 生成 JSON
	size_t iRetSize = 0;
	char* sRet = xteStringifyJSON(option, FALSE, &iRetSize);
	http_reply(c, 200, "Content-Type: application/json\r\n", sRet, iRetSize);
}


