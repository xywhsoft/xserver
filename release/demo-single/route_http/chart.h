/* ECharts 图表数据范例。wwwroot/chart.html 会请求这个接口。 */



static void Handle_Chart_Get(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	static const char* arrDays[7] = {
		"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
	};
	xvalue* pOption = xrtValueObject();
	xvalue* pXAxis = xrtValueObject();
	xvalue* pXData = xrtValueArray();
	xvalue* pYAxis = xrtValueObject();
	xvalue* pSeries = xrtValueArray();
	xvalue* pItem = xrtValueObject();
	xvalue* pData = xrtValueArray();
	int i;

	(void)arrParam;
	(void)iParamCount;
	xrtValueObjectSetNew(pXAxis, XRT_STR_LITERAL("type"),
		xrtValueString(XRT_STR_LITERAL("category")));
	for ( i = 0; i < 7; i++ ) {
		xrtValueArrayAppendNew(pXData,
			xrtValueString(xrtStrView(arrDays[i])));
	}
	xrtValueObjectSetNew(pXAxis, XRT_STR_LITERAL("data"), pXData);
	xrtValueObjectSetNew(pOption, XRT_STR_LITERAL("xAxis"), pXAxis);

	xrtValueObjectSetNew(pYAxis, XRT_STR_LITERAL("type"),
		xrtValueString(XRT_STR_LITERAL("value")));
	xrtValueObjectSetNew(pOption, XRT_STR_LITERAL("yAxis"), pYAxis);

	xrtValueObjectSetNew(pItem, XRT_STR_LITERAL("type"),
		xrtValueString(XRT_STR_LITERAL("line")));
	xrtValueObjectSetNew(pItem, XRT_STR_LITERAL("smooth"), xrtValueBool(true));
	for ( i = 0; i < 7; i++ ) {
		xrtValueArrayAppendNew(pData, xrtValueInt(xrtRandRange(100, 1500)));
	}
	xrtValueObjectSetNew(pItem, XRT_STR_LITERAL("data"), pData);
	xrtValueArrayAppendNew(pSeries, pItem);
	xrtValueObjectSetNew(pOption, XRT_STR_LITERAL("series"), pSeries);

	(void)ReplyJSON(pReq, 200, pOption);
	xrtValueRelease(pOption);
}
