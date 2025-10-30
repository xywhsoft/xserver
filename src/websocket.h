


// 订阅列表数据结构
static const char *s_listen_on = "ws://localhost:8000";
static const char *s_web_root = ".";
static const char *s_ca_path = "ca.pem";
static const char *s_cert_path = "cert.pem";
static const char *s_key_path = "key.pem";





// MQTT 协议处理
static void ProcWS(struct mg_connection *c, int ev, void *ev_data) {
	XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
	if ( ev == MG_EV_OPEN ) {
		// c->is_hexdumping = 1;
	} else if( ev == MG_EV_ACCEPT && mg_url_is_ssl(s_listen_on) ) {
		struct mg_str ca = mg_file_read(&mg_fs_posix, s_ca_path);
		struct mg_str cert = mg_file_read(&mg_fs_posix, s_cert_path);
		struct mg_str key = mg_file_read(&mg_fs_posix, s_key_path);
		struct mg_tls_opts opts = {.ca = ca, .cert = cert, .key = key};
		mg_tls_init(c, &opts);
	} else if ( ev == MG_EV_HTTP_MSG ) {
		struct mg_http_message *hm = (struct mg_http_message *) ev_data;
		if (mg_match(hm->uri, mg_str("/websocket"), NULL)) {
			// Upgrade to websocket. From now on, a connection is a full-duplex
			// Websocket connection, which will receive MG_EV_WS_MSG events.
			mg_ws_upgrade(c, hm, NULL);
		} else if ( mg_match(hm->uri, mg_str("/rest"), NULL) ) {
			// Serve REST response
			mg_http_reply(c, 200, "", "{\"result\": %d}\n", 123);
		} else {
			// Serve static files
			struct mg_http_serve_opts opts = {.root_dir = s_web_root};
			mg_http_serve_dir(c, ev_data, &opts);
		}
	} else if ( ev == MG_EV_WS_MSG ) {
		// Got websocket frame. Received data is wm->data. Echo it back!
		struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
		mg_ws_send(c, wm->data.buf, wm->data.len, WEBSOCKET_OP_TEXT);
	}
}
static void ProcWSS(struct mg_connection *c, int ev, void *ev_data) {
	XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
}





// 启动 MQTT 服务
int RunServerWS(XS_ServerObject objServer)
{
	// 启动 WebSocket 服务
	printf("\n    Run Server [WebSocket] : %s (%s)\n", objServer->Name, objServer->Addr);
	struct mg_connection* objConn = mg_http_listen(&mgr, objServer->Addr, ProcWS, objServer);
	if ( objConn == NULL ) {
		printf("    !!! ERROR !!! Cannot listen on %s. Use ws://ADDR:PORT or :PORT\n", objServer->Addr);
		exit(EXIT_FAILURE);
	} else {
		objServer->Conn = objConn;
	}
	if ( objServer->EnableTLS ) {
		printf("    Run Server [WebSocketS] : %s (%s)\n", objServer->Name, objServer->Addr);
		objConn = mg_http_listen(&mgr, objServer->AddrTLS, ProcWSS, objServer);
		if ( objConn == NULL ) {
			printf("    !!! ERROR !!! Cannot listen on %s. Use wss://ADDR:PORT or :PORT\n", objServer->AddrTLS);
			exit(EXIT_FAILURE);
		} else {
			objServer->ConnTLS = objConn;
		}
	}
}



// 停止 HTTP 服务
int StopServerWS(XS_ServerObject objServer)
{
	return TRUE;
}


