


// ==================== HTTP 服务 ====================
// 基于 xrt HTTP 服务器 (xhttpserver) 实现



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



// 根据 HTTP 请求定位 Host
static XS_HostObject LocateHost_HTTP(XS_ServerObject objServer, xhttpdreq* pReq)
{
	const char* sHost = xrtHttpReqGetHeader(pReq, "host");
	XS_HostObject objHost = NULL;
	
	if ( sHost && sHost[0] != '\0' ) {
		size_t iHostLen = strlen(sHost);
		
		// 校验 Host 头合法性
		if ( !IsHostHeaderValid(sHost, iHostLen) ) {
			// Host 头格式非法，使用默认 Host
			if ( objServer->EnableDefaultHost ) {
				return &objServer->DefaultHost;
			}
			return NULL;
		}
		
		// 去除端口号（如果有）
		for ( size_t i = 0; i < iHostLen; i++ ) {
			if ( sHost[i] == ':' ) {
				iHostLen = i;
				break;
			}
		}
		
		XS_HostObject* ppHost = xrtDictGet(objServer->HostMap, (ptr)sHost, iHostLen);
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



// HTTP 请求回调
static void OnHttpRequest(ptr pUserData, xnetconn* pConn, xhttpdreq* pReq)
{
	XS_ServerObject objServer = (XS_ServerObject)pUserData;
	XS_HostObject objHost = LocateHost_HTTP(objServer, pReq);
	
	if ( objHost == NULL ) {
		xrtHttpReply(pConn, 404, NULL, "Host not found");
		return;
	}
	
	// 调试输出
	if ( objHost->DebugMode ) {
		str sTime = xrtNowStr();
		printf("[%s] %s [%s] %s\n", sTime, objHost->Name, pReq->sMethod, pReq->sUri);
		xrtFree(sTime);
	}
	
	// 检查路径安全性
	if ( !IsPathSafe(pReq->sUri, strlen(pReq->sUri)) ) {
		xrtHttpReply(pConn, 403, NULL, "Forbidden: Invalid path");
		return;
	}
	
	// 调用脚本回调或静态文件服务
	if ( objHost->DevLang == SLT_C ) {
		if ( objHost->RequestProc ) {
			void (*RequestProc)(XS_ServerObject, XS_HostObject, xnetconn*, xhttpdreq*) = objHost->RequestProc;
			RequestProc(objServer, objHost, pConn, pReq);
		} else {
			xrtHttpReply(pConn, 500, NULL, "RequestProc not found");
		}
	} else {
		// 静态文件服务
		xrtHttpServeDir(pConn, pReq, objHost->Path);
	}
}



// HTTP 连接关闭回调
static void OnHttpClose(ptr pUserData, xnetconn* pConn)
{
	// 连接关闭处理（如需要可在此添加日志）
}



// WebSocket 升级请求回调
static bool OnHttpUpgrade(ptr pUserData, xnetconn* pConn, xhttpdreq* pReq)
{
	XS_ServerObject objServer = (XS_ServerObject)pUserData;
	XS_HostObject objHost = LocateHost_HTTP(objServer, pReq);
	
	if ( objHost == NULL ) {
		return false;
	}
	
	// 如果 Host 有 WebSocket 事件处理，则允许升级
	if ( objHost->WsEventProc ) {
		// 保存 Host 信息到连接用户数据
		pConn->pUserData = objHost;
		return true;
	}
	
	return false;
}



// TLS SNI 回调 - 根据域名选择证书
static void OnHttpSNI(xtlsctx* pCtx, const char* sHostName, ptr pUserData)
{
	XS_ServerObject objServer = (XS_ServerObject)pUserData;
	
	if ( sHostName == NULL || sHostName[0] == '\0' ) {
		// 使用默认证书
		if ( objServer->EnableDefaultHost && objServer->DefaultHost.tTlsConfig.sCertFile ) {
			xrtTlsSetCert(pCtx, objServer->DefaultHost.tTlsConfig.sCertFile,
			              objServer->DefaultHost.tTlsConfig.sKeyFile);
		}
		return;
	}
	
	size_t iHostLen = strlen(sHostName);
	
	// 校验 SNI 主机名合法性
	if ( !IsHostHeaderValid(sHostName, iHostLen) ) {
		if ( objServer->EnableDefaultHost && objServer->DefaultHost.tTlsConfig.sCertFile ) {
			xrtTlsSetCert(pCtx, objServer->DefaultHost.tTlsConfig.sCertFile,
			              objServer->DefaultHost.tTlsConfig.sKeyFile);
		}
		return;
	}
	
	// 查找匹配的 Host
	XS_HostObject* ppHost = xrtDictGet(objServer->HostMap, (ptr)sHostName, iHostLen);
	if ( ppHost && ppHost[0]->tTlsConfig.sCertFile ) {
		xrtTlsSetCert(pCtx, ppHost[0]->tTlsConfig.sCertFile,
		              ppHost[0]->tTlsConfig.sKeyFile);
	} else if ( objServer->EnableDefaultHost && objServer->DefaultHost.tTlsConfig.sCertFile ) {
		xrtTlsSetCert(pCtx, objServer->DefaultHost.tTlsConfig.sCertFile,
		              objServer->DefaultHost.tTlsConfig.sKeyFile);
	}
}



// 启动 HTTP 服务
int RunServerHTTP(XS_ServerObject objServer)
{
	char sIP[64] = {0};
	uint16 iPort = 0;
	
	// 解析地址端口
	ParseAddr(objServer->Addr, sIP, sizeof(sIP), &iPort);
	
	// 配置 HTTP 服务器
	xhttpsrvconfig tConfig = {0};
	tConfig.iMaxClients = 1024;
	tConfig.iMaxHeaderSize = 8192;
	tConfig.iMaxBodySize = 10 * 1024 * 1024;  // 10MB
	tConfig.iKeepAliveTimeout = 60;
	
	// 事件回调
	xhttpsrvevents tEvents = {0};
	tEvents.OnRequest = OnHttpRequest;
	tEvents.OnClose = OnHttpClose;
	tEvents.OnUpgrade = OnHttpUpgrade;
	
	// 创建 HTTP 服务器 (共享事件循环)
	printf("    Run Server [HTTP] : %s (%s:%d)\n", objServer->Name, sIP, iPort);
	xhttpserver* pServer = xrtHttpServerCreateEx(g_pLoop, sIP, iPort, &tConfig, &tEvents);
	if ( pServer == NULL ) {
		printf("    !!! ERROR !!! Cannot create HTTP server on %s:%d\n", sIP, iPort);
		exit(EXIT_FAILURE);
	}
	
	// 设置用户数据为 Server 对象
	xrtHttpServerSetUserData(pServer, objServer);
	objServer->pServer = pServer;
	
	// 启动服务器
	if ( xrtHttpServerStart(pServer) != XRT_NET_OK ) {
		printf("    !!! ERROR !!! Cannot start HTTP server\n");
		exit(EXIT_FAILURE);
	}
	
	// HTTPS (TLS)
	if ( objServer->EnableTLS ) {
		uint16 iTlsPort = 0;
		ParseAddr(objServer->AddrTLS, sIP, sizeof(sIP), &iTlsPort);
		
		printf("    Run Server [HTTPS] : %s (%s:%d)\n", objServer->Name, sIP, iTlsPort);
		
		// 创建 HTTPS 服务器
		xhttpserver* pServerTLS = xrtHttpServerCreateEx(g_pLoop, sIP, iTlsPort, &tConfig, &tEvents);
		if ( pServerTLS == NULL ) {
			printf("    !!! ERROR !!! Cannot create HTTPS server on %s:%d\n", sIP, iTlsPort);
			exit(EXIT_FAILURE);
		}
		
		xrtHttpServerSetUserData(pServerTLS, objServer);
		objServer->pServerTLS = pServerTLS;
		
		// 配置 TLS (使用默认 Host 的证书)
		if ( objServer->EnableDefaultHost && objServer->DefaultHost.tTlsConfig.sCertFile ) {
			xtlsconfig tTlsConfig = {0};
			tTlsConfig.sCertFile = objServer->DefaultHost.tTlsConfig.sCertFile;
			tTlsConfig.sKeyFile = objServer->DefaultHost.tTlsConfig.sKeyFile;
			tTlsConfig.OnSNI = OnHttpSNI;
			tTlsConfig.pSNIUserData = objServer;
			
			if ( xrtHttpServerEnableTLS(pServerTLS, &tTlsConfig) != XRT_NET_OK ) {
				printf("    !!! ERROR !!! Cannot enable TLS for HTTPS server\n");
				exit(EXIT_FAILURE);
			}
		}
		
		// 启动 HTTPS 服务器
		if ( xrtHttpServerStart(pServerTLS) != XRT_NET_OK ) {
			printf("    !!! ERROR !!! Cannot start HTTPS server\n");
			exit(EXIT_FAILURE);
		}
	}
	
	return TRUE;
}



// 停止 HTTP 服务
int StopServerHTTP(XS_ServerObject objServer)
{
	printf("    Stop Server [HTTP] : %s\n", objServer->Name);
	
	if ( objServer->pServer ) {
		xrtHttpServerDestroy((xhttpserver*)objServer->pServer);
		objServer->pServer = NULL;
	}
	if ( objServer->pServerTLS ) {
		xrtHttpServerDestroy((xhttpserver*)objServer->pServerTLS);
		objServer->pServerTLS = NULL;
	}
	
	return TRUE;
}


