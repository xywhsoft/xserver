bool Request_Chart_Get(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	xvalue objOption = xvoCreateTable();
	xvalue objXAxis = xvoCreateTable();
	xvalue arrXAxisData = xvoCreateArray();
	xvalue objYAxis = xvoCreateTable();
	xvalue arrSeries = xvoCreateArray();
	xvalue objSeriesItem = xvoCreateTable();
	xvalue arrSeriesData = xvoCreateArray();
	bool bRet;
	int i;

	(void)objServer;
	(void)objHost;
	(void)objReq;

	xvoTableSetValue(objOption, "xAxis", 5, objXAxis, TRUE);
	xvoTableSetText(objXAxis, "type", 4, "category", 0, FALSE);
	xvoTableSetValue(objXAxis, "data", 4, arrXAxisData, TRUE);
	xvoArrayAppendText(arrXAxisData, "Mon", 0, FALSE);
	xvoArrayAppendText(arrXAxisData, "Tue", 0, FALSE);
	xvoArrayAppendText(arrXAxisData, "Wed", 0, FALSE);
	xvoArrayAppendText(arrXAxisData, "Thu", 0, FALSE);
	xvoArrayAppendText(arrXAxisData, "Fri", 0, FALSE);
	xvoArrayAppendText(arrXAxisData, "Sat", 0, FALSE);
	xvoArrayAppendText(arrXAxisData, "Sun", 0, FALSE);

	xvoTableSetValue(objOption, "yAxis", 5, objYAxis, TRUE);
	xvoTableSetText(objYAxis, "type", 4, "value", 0, FALSE);

	xvoTableSetValue(objOption, "series", 6, arrSeries, TRUE);
	xvoArrayAppendValue(arrSeries, objSeriesItem, TRUE);
	xvoTableSetValue(objSeriesItem, "data", 4, arrSeriesData, TRUE);
	xvoTableSetText(objSeriesItem, "type", 4, "line", 0, FALSE);
	xvoTableSetBool(objSeriesItem, "smooth", 6, TRUE);

	for ( i = 0; i < 7; i++ ) {
		xvoArrayAppendInt(arrSeriesData, xrtRandRange(100, 1500));
	}

	bRet = DemoHttpReplyJSONValue(objResp, 200, "OK", objOption);
	xvoUnref(objOption);
	return bRet;
}
