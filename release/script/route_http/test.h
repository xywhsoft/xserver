


// 测试 HTTP 客户端
void Request_Test(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	http_reply(c, 200, "Access-Control-Allow-Origin: *\r\nContent-Type: text/html\r\n", "page load success !", 0);
}



// 代入模板
void Request_Template(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	xvalue data = xvoCreateTable();
	xvoTableSetText(data, "title", 5, "网站标题", 0, FALSE);
	xvoTableSetText(data, "desc", 4, "网站说明", 0, FALSE);
	xvalue list = xvoCreateArray();
	xvoArrayAppendText(list, "文章列表 1", 0, FALSE);
	xvoArrayAppendText(list, "文章列表 2", 0, FALSE);
	xvoArrayAppendText(list, "文章列表 3", 0, FALSE);
	xvoArrayAppendText(list, "文章列表 4", 0, FALSE);
	xvoArrayAppendText(list, "文章列表 5", 0, FALSE);
	xvoTableSetValue(data, "list", 4, list, TRUE);
	size_t iRetSize = 0;
	str sRet = MakePageWithTemplate("page.html", data, &iRetSize);
	http_reply(c, 200, "Access-Control-Allow-Origin: *\r\nContent-Type: text/html\r\n", sRet, iRetSize);
}


