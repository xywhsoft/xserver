/*
 * 最基础的 HTTP 返回范例
 *
 * 每个处理函数都直接接收 XS_HttpReq，完成“读取请求 -> 处理 -> 返回响应”。
 * 静态路由没有路径参数，因此这里显式忽略 arrParam 和 iParamCount。
 */



/* GET /text —— 返回一段普通文本。 */
static void Handle_Text(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	(void)arrParam;
	(void)iParamCount;
	(void)ReplyText(pReq, 200, DEMO_EXTRA_TEXT);
}



/* GET /json —— 使用 xvalue 构造 JSON 响应。 */
static void Handle_JSON(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	const XS_HostInfo* pHost = pReq->host;
	xvalue* pObject = xrtValueObject();

	(void)arrParam;
	(void)iParamCount;
	xrtValueObjectSetNew(pObject, XRT_STR_LITERAL("server"),
		xrtValueString(XRT_STR_LITERAL("xs3 demo")));
	xrtValueObjectSetNew(pObject, XRT_STR_LITERAL("ok"), xrtValueBool(true));
	xrtValueObjectSetNew(pObject, XRT_STR_LITERAL("host"),
		xrtValueString(xrtStrView((pHost->Name ? pHost->Name : "?"))));
	(void)ReplyJSON(pReq, 200, pObject);
	xrtValueRelease(pObject);
}



/* GET /test —— 保留旧版 dev/v1 的最小 HTML 自检。 */
static void Handle_Test(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	(void)arrParam;
	(void)iParamCount;
	(void)ReplyHTML(pReq, 200, "page load success !");
}



/* GET /page —— 从 wwwroot 外的 page 目录加载受控页面。 */
static void Handle_Page(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	(void)arrParam;
	(void)iParamCount;
	(void)LoadPage(pReq, "demo.html");
}



/* GET /template —— 准备数据，然后通过 LoadTemplate 渲染模板。 */
static void Handle_Template(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
)
{
	static const char* arrItem[] = {
		"条目甲", "条目乙", "条目丙", "条目丁", "条目戊"
	};
	xvalue* pData = xrtValueObject();
	xvalue* pList = xrtValueArray();
	uint32 i;

	(void)arrParam;
	(void)iParamCount;
	xrtValueObjectSetNew(pData, XRT_STR_LITERAL("title"),
		xrtValueString(XRT_STR_LITERAL("模板页")));
	xrtValueObjectSetNew(pData, XRT_STR_LITERAL("desc"),
		xrtValueString(XRT_STR_LITERAL("由 xrtTemplateCompile 渲染")));
	for ( i = 0; i < sizeof(arrItem) / sizeof(arrItem[0]); i++ ) {
		xrtValueArrayAppendNew(pList, xrtValueString(xrtStrView(arrItem[i])));
	}
	xrtValueObjectSetNew(pData, XRT_STR_LITERAL("list"), pList);

	(void)LoadTemplate(pReq, "page.html", pData);
	xrtValueRelease(pData);
}
