


// HTTP 协议处理
void InitTLS(struct mg_connection *c, XS_HostObject objHost, XS_ServerObject objServer)
{
	struct mg_tls_opts opts;
	memset(&opts, 0, sizeof(opts));
	opts.ca = objHost->TLS_CA;
	opts.cert = objHost->TLS_Cert;
	opts.key = objHost->TLS_Key;
	mg_tls_init(c, &opts);
}



// 根据 HTTP 请求定位 Host
XS_HostObject LocateHost_HTTP(XS_ServerObject objServer, struct mg_http_message* hm)
{
	struct mg_str* Host = mg_http_get_header(hm, "host");
	XS_HostObject objHost = NULL;
	if ( Host && (Host->len > 0) ) {
		// 去除端口号（如果有）
		size_t hostLen = Host->len;
		for ( size_t i = 0; i < Host->len; i++ ) {
			if ( Host->buf[i] == ':' ) {
				hostLen = i;
				break;
			}
		}
		XS_HostObject* ppHost = xrtDictGet(objServer->HostMap, Host->buf, hostLen);
		if ( ppHost ) {
			objHost = ppHost[0];
		} else if ( objServer->EnableDefaultHost ) {
			objHost = &objServer->DefaultHost;
		}
	} else if ( objServer->EnableDefaultHost ) {
		objHost = &objServer->DefaultHost;
	}
	return objHost;
}



// 请求处理
void ProcRequest_HTTP(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
	if ( objHost->DebugMode ) {
		str sTime = xrtNowStr();
		printf("[%s] %s [%.*s] %.*s\n", sTime, objHost->Name, hm->method.len, hm->method.buf, hm->uri.len, hm->uri.buf);
		xrtFree(sTime);
	}
	if ( objHost->DevLang == SLT_C ) {
		if ( objHost->RequestProc ) {
			void (*RequestProc)(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection *c, struct mg_http_message* hm) = objHost->RequestProc;
			RequestProc(objServer, objHost, c, hm);
		} else {
			mg_http_reply(c, 500, NULL, "not found RequestProc!");
		}
	} else if ( objHost->DevLang == SLT_LUA ) {
	} else if ( objHost->DevLang == SLT_JS ) {
	} else {
		// 设置服务器根目录
		struct mg_http_serve_opts opts = {0};
		opts.root_dir = objHost->Path;
		mg_http_serve_dir(c, hm, &opts);
	}
}



// HTTP 协议处理
static void ProcHTTP(struct mg_connection* c, int ev, void *ev_data) {
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
		struct mg_http_message* hm = ev_data;
		XS_HostObject objHost = LocateHost_HTTP(objServer, hm);
		if ( objHost ) {
			ProcRequest_HTTP(objServer, objHost, c, hm);
		} else {
			printf("!!! ERROR !!! NO Enabled Default Host [HTTP] !");
		}
	}
}



// HTTPS 协议处理
#if MG_TLS == MG_TLS_MBED
	static void ProcHTTPS(struct mg_connection* c, int ev, void *ev_data) {
		XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
		// 设置 TLS 证书
		if ( ev == MG_EV_ACCEPT ) {
			if ( objServer->EnableDefaultHost ) {
				InitTLS(c, &objServer->DefaultHost, objServer);
			} else {
				printf("!!! ERROR !!! NO Enabled Default Host [TLS host missing] !");
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
			struct mg_http_message* hm = ev_data;
			XS_HostObject objHost = LocateHost_HTTP(objServer, hm);
			if ( objHost ) {
				ProcRequest_HTTP(objServer, objHost, c, hm);
			} else {
				printf("!!! ERROR !!! NO Enabled Default Host [HTTPS] !");
			}
		}
	}
#else
	static void ProcHTTPS(struct mg_connection* c, int ev, void *ev_data) {
		XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
		// 设置 TLS 证书
		if ( ev == MG_EV_ACCEPT ) {
			mg_tls_init_accept(c);
		}
		if ( ev == MG_EV_TLS_HSCH ) {
			if ( ev_data == NULL ) {
				if ( objServer->EnableDefaultHost ) {
					InitTLS(c, &objServer->DefaultHost, objServer);
				} else {
					printf("!!! ERROR !!! NO Enabled Default Host [TLS] !");
				}
			} else {
				XS_HostObject* ppHost = xrtDictGet(objServer->HostMap, (char*)ev_data, strlen(ev_data));
				if ( ppHost ) {
					XS_HostObject objHost = ppHost[0];
					InitTLS(c, objHost, objServer);
				} else if ( objServer->EnableDefaultHost ) {
					InitTLS(c, &objServer->DefaultHost, objServer);
				} else {
					printf("!!! ERROR !!! NO Enabled Default Host [TLS host missing] !");
				}
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
			struct mg_http_message* hm = ev_data;
			XS_HostObject objHost = LocateHost_HTTP(objServer, hm);
			if ( objHost ) {
				ProcRequest_HTTP(objServer, objHost, c, hm);
			} else {
				printf("!!! ERROR !!! NO Enabled Default Host [HTTPS] !");
			}
		}
	}
#endif





// 启动 HTTP 服务
int RunServerHTTP(XS_ServerObject objServer)
{
	// 启动 HTTP 服务
	printf("\n    Run Server [HTTP] : %s (%s)\n", objServer->Name, objServer->Addr);
	struct mg_connection* objConn = mg_http_listen(&mgr, objServer->Addr, ProcHTTP, objServer);
	if ( objConn == NULL ) {
		printf("    !!! ERROR !!! Cannot listen on %s. Use http://ADDR:PORT or :PORT\n", objServer->Addr);
		exit(EXIT_FAILURE);
	} else {
		objServer->Conn = objConn;
	}
	if ( objServer->EnableTLS ) {
		printf("    Run Server [HTTPS] : %s (%s)\n", objServer->Name, objServer->AddrTLS);
		objConn = mg_http_listen(&mgr, objServer->AddrTLS, ProcHTTPS, objServer);
		if ( objConn == NULL ) {
			printf("    !!! ERROR !!! Cannot listen on %s. Use https://ADDR:PORT or :PORT\n", objServer->AddrTLS);
			exit(EXIT_FAILURE);
		} else {
			objServer->ConnTLS = objConn;
		}
	}
	return TRUE;
}



// 停止 HTTP 服务
int StopServerHTTP(XS_ServerObject objServer)
{
	return TRUE;
}


