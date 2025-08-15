


#include <xsbase.h>





void RequestProc(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	if ( mg_match(hm->uri, mg_str("/test"), NULL) ) {
		mg_http_reply(c, 200, NULL, "URI: %.*s", (int) hm->uri.len, hm->uri.buf);
	} else {
		// 设置服务器根目录
		struct mg_http_serve_opts opts = {0};
		opts.root_dir = objHost->Path;
		mg_http_serve_dir(c, hm, &opts);
	}
}


