


// 校验请求路径安全性（防止路径遍历）
static int IsPathSafe(const char* path, size_t len)
{
	if ( path == NULL || len == 0 ) return FALSE;
	
	// 检查路径遍历攻击
	for ( size_t i = 0; i < len - 1; i++ ) {
		if ( (path[i] == '.') && (path[i + 1] == '.') ) {
			// 发现 ".." 路径遍历尝试
			return FALSE;
		}
	}
	
	// 检查绝对路径访问尝试 (Windows 和 Unix)
	if ( len > 0 && path[0] == '/' ) {
		if ( len > 1 && path[1] == '/' ) {
			return FALSE;  // UNC 路径
		}
	}
	if ( len > 1 && path[1] == ':' ) {
		return FALSE;  // Windows 绝对路径
	}
	
	return TRUE;
}



// 校验 Host 头格式合法性
static int IsHostHeaderValid(const char* host, size_t len)
{
	if ( host == NULL || len == 0 || len > 253 ) return FALSE;
	
	for ( size_t i = 0; i < len; i++ ) {
		char c = host[i];
		// 允许: 字母、数字、点、连字符、冒号(端口)
		if ( !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
		       (c >= '0' && c <= '9') || c == '.' || c == '-' || c == ':') ) {
			return FALSE;
		}
	}
	return TRUE;
}



// HTTP TLS 初始化
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
	if ( Host && (Host->len > 0) && (Host->buf != NULL) ) {
		// 校验 Host 头合法性
		if ( !IsHostHeaderValid(Host->buf, Host->len) ) {
			// Host 头格式非法，使用默认 Host
			if ( objServer->EnableDefaultHost ) {
				return &objServer->DefaultHost;
			}
			return NULL;
		}
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
		// 静态文件服务 - 先检查路径安全性
		if ( !IsPathSafe(hm->uri.buf, hm->uri.len) ) {
			mg_http_reply(c, 403, NULL, "Forbidden: Invalid path");
			return;
		}
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
				const char* sHostName = (const char*)ev_data;
				size_t iHostLen = strlen(sHostName);
				// 校验 SNI 主机名合法性
				if ( !IsHostHeaderValid(sHostName, iHostLen) ) {
					if ( objServer->EnableDefaultHost ) {
						InitTLS(c, &objServer->DefaultHost, objServer);
					} else {
						printf("!!! ERROR !!! Invalid SNI hostname [TLS] !");
					}
				} else {
					XS_HostObject* ppHost = xrtDictGet(objServer->HostMap, (char*)sHostName, iHostLen);
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
	printf("    Stop Server [HTTP] : %s\n", objServer->Name);
	// 连接由 mg_mgr_free 统一释放
	objServer->Conn = NULL;
	objServer->ConnTLS = NULL;
	return TRUE;
}


