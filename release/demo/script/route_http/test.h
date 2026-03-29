bool Request_Test(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	(void)objServer;
	(void)objHost;
	(void)objReq;

	return DemoHttpReplyHTML(objResp, 200, "OK", "page load success !");
}



bool Request_Template(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp)
{
	xvalue objData;
	xvalue objList;
	str sHTML;
	size_t iHTMLSize = 0;
	bool bRet;

	(void)objServer;
	(void)objHost;
	(void)objReq;

	objData = xvoCreateTable();
	objList = xvoCreateArray();
	xvoTableSetText(objData, "title", 5, "网站标题", 0, FALSE);
	xvoTableSetText(objData, "desc", 4, "网站说明", 0, FALSE);
	xvoArrayAppendText(objList, "文章列表 1", 0, FALSE);
	xvoArrayAppendText(objList, "文章列表 2", 0, FALSE);
	xvoArrayAppendText(objList, "文章列表 3", 0, FALSE);
	xvoArrayAppendText(objList, "文章列表 4", 0, FALSE);
	xvoArrayAppendText(objList, "文章列表 5", 0, FALSE);
	xvoTableSetValue(objData, "list", 4, objList, TRUE);

	sHTML = MakePageWithTemplate("page.html", objData, &iHTMLSize);
	xvoUnref(objData);
	bRet = DemoHttpReplyHTML(objResp, 200, "OK", sHTML ? sHTML : "");

	if ( sHTML ) {
		xrtFree(sHTML);
	}
	return bRet;
}
