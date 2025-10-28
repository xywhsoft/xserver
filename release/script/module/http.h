


// HTTP 请求处理
void RequestProc(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	RouteItemHTTP* objItem = xrtDictGet(StaticRouteTableHTTP, hm->uri.buf, hm->uri.len);
	if ( objItem ) {
		// C 语言静态路由
		objItem->Proc(objServer, objHost, c, hm);
	} else {
		// 访问服务器静态资源
		struct mg_http_serve_opts opts = {0};
		opts.root_dir = objHost->Path;
		mg_http_serve_dir(c, hm, &opts);
	}
}


