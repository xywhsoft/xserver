


// HTTP 协议处理
void InitTLS(struct mg_connection *c, XS_HostObject objHost, XS_ServerObject objServer)
{
	struct mg_tls_opts opts;
	memset(&opts, 0, sizeof(opts));
	opts.ca = objHost->TLS_CA;
	opts.cert = objHost->TLS_Cert;
	opts.key = objHost->TLS_Key;
	/*
	printf("InitTLS : %s\n", objHost->Name);
	printf("CA :\n%s\n", opts.ca.buf);
	printf("Cert :\n%s\n", opts.cert.buf);
	printf("Key :\n%s\n", opts.key.buf);
	//*/
	mg_tls_init(c, &opts);
}



// 请求处理
void ProcRequest_HTTP(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm)
{
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
		// 定位 Host
		struct mg_str* Host = mg_http_get_header(hm, "host");
		XS_HostObject* ppHost = AVLHT32_Get(objServer->HostMap, Host->buf, Host->len);
		XS_HostObject objHost;
		if ( ppHost ) {
			objHost = ppHost[0];
		} else {
			if ( objServer->EnableDefaultHost ) {
				objHost = &objServer->DefaultHost;
			} else {
				printf("!!! ERROR !!! NO Enabled Default Host [HTTP] !");
				return;
			}
		}
		// 处理请求
		ProcRequest_HTTP(objServer, objHost, c, hm);
	}
}



// HTTPS 协议处理
#if MG_TLS == MG_TLS_MBED
	static void ProcHTTPS(struct mg_connection* c, int ev, void *ev_data) {
		XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
		// 设置 TLS 证书
		if ( ev == MG_EV_ACCEPT ) {
			// 使用 mbedtls 时，仅支持默认 Host
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
			// 定位 Host
			struct mg_str* Host = mg_http_get_header(hm, "host");
			XS_HostObject* ppHost = AVLHT32_Get(objServer->HostMap, Host->buf, Host->len);
			XS_HostObject objHost;
			if ( ppHost ) {
				objHost = ppHost[0];
			} else {
				if ( objServer->EnableDefaultHost ) {
					objHost = &objServer->DefaultHost;
				} else {
					printf("!!! ERROR !!! NO Enabled Default Host [HTTP] !");
					return;
				}
			}
			// 处理请求
			ProcRequest_HTTP(objServer, objHost, c, hm);
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
				// 读取 Host 映射
				XS_HostObject* ppHost = AVLHT32_Get(objServer->HostMap, (char*)ev_data, strlen(ev_data));
				if ( ppHost ) {
					XS_HostObject objHost = ppHost[0];
					InitTLS(c, objHost, objServer);
				} else {
					// 找不到 Host 使用默认 Host
					if ( objServer->EnableDefaultHost ) {
						InitTLS(c, &objServer->DefaultHost, objServer);
					} else {
						printf("!!! ERROR !!! NO Enabled Default Host [TLS host missing] !");
					}
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
			// 定位 Host
			struct mg_str* Host = mg_http_get_header(hm, "host");
			XS_HostObject* ppHost = AVLHT32_Get(objServer->HostMap, Host->buf, Host->len);
			XS_HostObject objHost;
			if ( ppHost ) {
				objHost = ppHost[0];
			} else {
				if ( objServer->EnableDefaultHost ) {
					objHost = &objServer->DefaultHost;
				} else {
					printf("!!! ERROR !!! NO Enabled Default Host [HTTP] !");
					return;
				}
			}
			// 处理请求
			ProcRequest_HTTP(objServer, objHost, c, hm);
		}
	}
#endif





// 启动 HTTP 服务
int RunServerHTTP(XS_ServerObject objServer)
{
	// 初始化 HTTP 服务
	DynLoad_C_GlobalData(objServer);
	if ( objServer->DefaultHost.ServiceInit ) {
		void (*ServiceInit)(XS_ServerObject objServer) = objServer->DefaultHost.ServiceInit;
		ServiceInit(objServer);
	}
	// 启动 HTTP 服务
	printf("Run Server [HTTP] : %s (%s)\n", objServer->Name, objServer->Addr);
	struct mg_connection* objConn = mg_http_listen(&mgr, objServer->Addr, ProcHTTP, objServer);
	if ( objConn == NULL ) {
		printf("!!! ERROR !!! Cannot listen on %s. Use http://ADDR:PORT or :PORT\n", objServer->Addr);
		exit(EXIT_FAILURE);
	} else {
		objServer->Conn = objConn;
	}
	if ( objServer->EnableTLS ) {
		printf("Run Server [HTTPS] : %s (%s)\n", objServer->Name, objServer->AddrTLS);
		objConn = mg_http_listen(&mgr, objServer->AddrTLS, ProcHTTPS, objServer);
		if ( objConn == NULL ) {
			printf("!!! ERROR !!! Cannot listen on %s. Use https://ADDR:PORT or :PORT\n", objServer->AddrTLS);
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
	if ( objServer->DefaultHost.ServiceUnit ) {
		void (*ServiceUnit)(XS_ServerObject objServer) = objServer->DefaultHost.ServiceUnit;
		ServiceUnit(objServer);
	}
	return TRUE;
}


