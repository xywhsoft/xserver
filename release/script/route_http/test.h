


// 测试 HTTP 客户端
void Request_Test(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	http_reply(c, 200, "Access-Control-Allow-Origin: *\r\nContent-Type: text/html\r\n", "okk", 0);
}


