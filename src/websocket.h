


// WebSocket 协议处理
static void ProcWS(struct mg_connection *c, int ev, void *ev_data) {
	XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
	// 先尝试调用自定义的协议处理逻辑
	if ( objServer->DefaultHost.DevLang == SLT_C ) {
		if ( objServer->DefaultHost.EventProc ) {
			int (*EventProc)(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection *c, int ev, void *ev_data) = objServer->DefaultHost.EventProc;
			if ( EventProc(objServer, &objServer->DefaultHost, c, ev, ev_data) ) {
				return;
			}
		}
	} else if ( objServer->DefaultHost.DevLang == SLT_LUA ) {
	} else if ( objServer->DefaultHost.DevLang == SLT_JS ) {
	}
	// 进入协议处理逻辑
	if ( ev == MG_EV_HTTP_MSG ) {
		struct mg_http_message *hm = (struct mg_http_message *) ev_data;
		if ( mg_match(hm->uri, mg_str("/websocket"), NULL) ) {
			// 升级到 WebSocket
			mg_ws_upgrade(c, hm, NULL);
		} else {
			// 静态文件服务
			struct mg_http_serve_opts opts = {.root_dir = objServer->DefaultHost.Path};
			mg_http_serve_dir(c, ev_data, &opts);
		}
	} else if ( ev == MG_EV_WS_MSG ) {
		// WebSocket 消息处理（默认回显）
		struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
		mg_ws_send(c, wm->data.buf, wm->data.len, WEBSOCKET_OP_TEXT);
	}
}



// WebSocket TLS 协议处理
static void ProcWSS(struct mg_connection *c, int ev, void *ev_data) {
	XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
	// 设置 TLS 证书
	if ( ev == MG_EV_ACCEPT ) {
		if ( objServer->EnableDefaultHost ) {
			InitTLS(c, &objServer->DefaultHost, objServer);
		} else {
			printf("!!! ERROR !!! NO Enabled Default Host [WSS] !");
			return;
		}
	}
	// 先尝试调用自定义的协议处理逻辑
	if ( objServer->DefaultHost.DevLang == SLT_C ) {
		if ( objServer->DefaultHost.EventProc ) {
			int (*EventProc)(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection *c, int ev, void *ev_data) = objServer->DefaultHost.EventProc;
			if ( EventProc(objServer, &objServer->DefaultHost, c, ev, ev_data) ) {
				return;
			}
		}
	} else if ( objServer->DefaultHost.DevLang == SLT_LUA ) {
	} else if ( objServer->DefaultHost.DevLang == SLT_JS ) {
	}
	// 进入协议处理逻辑
	if ( ev == MG_EV_HTTP_MSG ) {
		struct mg_http_message *hm = (struct mg_http_message *) ev_data;
		if ( mg_match(hm->uri, mg_str("/websocket"), NULL) ) {
			mg_ws_upgrade(c, hm, NULL);
		} else {
			struct mg_http_serve_opts opts = {.root_dir = objServer->DefaultHost.Path};
			mg_http_serve_dir(c, ev_data, &opts);
		}
	} else if ( ev == MG_EV_WS_MSG ) {
		struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
		mg_ws_send(c, wm->data.buf, wm->data.len, WEBSOCKET_OP_TEXT);
	}
}





// 启动 WebSocket 服务
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
	return TRUE;
}



// 停止 WebSocket 服务
int StopServerWS(XS_ServerObject objServer)
{
	printf("    Stop Server [WebSocket] : %s\n", objServer->Name);
	// 连接由 mg_mgr_free 统一释放
	objServer->Conn = NULL;
	objServer->ConnTLS = NULL;
	return TRUE;
}


