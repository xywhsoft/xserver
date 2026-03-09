


// ==================== WebSocket 服务 ====================
// 基于 xrt WebSocket 服务器 (xwsserver) 实现



// WebSocket 连接打开回调
static void OnWsOpen(ptr pOwner, xnetconn* pConn)
{
	xwsserver* pWsServer = (xwsserver*)pOwner;
	XS_ServerObject objServer = (XS_ServerObject)xrtWsServerGetUserData(pWsServer);
	
	if ( objServer->DefaultHost.WsEventProc ) {
		void (*WsEventProc)(XS_ServerObject, XS_HostObject, xnetconn*, int, int, const char*, size_t) 
			= objServer->DefaultHost.WsEventProc;
		WsEventProc(objServer, &objServer->DefaultHost, pConn, XRT_EV_ACCEPT, 0, NULL, 0);
	}
}



// WebSocket 消息回调
static void OnWsMessage(ptr pOwner, xnetconn* pConn, int iOpcode, const char* pData, size_t iLen)
{
	xwsserver* pWsServer = (xwsserver*)pOwner;
	XS_ServerObject objServer = (XS_ServerObject)xrtWsServerGetUserData(pWsServer);
	
	if ( objServer->DefaultHost.WsEventProc ) {
		void (*WsEventProc)(XS_ServerObject, XS_HostObject, xnetconn*, int, int, const char*, size_t) 
			= objServer->DefaultHost.WsEventProc;
		WsEventProc(objServer, &objServer->DefaultHost, pConn, XRT_EV_RECV, iOpcode, pData, iLen);
	} else {
		// 默认回显
		if ( iOpcode == XRT_WS_OP_TEXT ) {
			xrtWsServerSendText(pWsServer, pConn->iId, pData, iLen);
		} else if ( iOpcode == XRT_WS_OP_BINARY ) {
			xrtWsServerSendBinary(pWsServer, pConn->iId, pData, iLen);
		}
	}
}



// WebSocket 连接关闭回调
static void OnWsClose(ptr pOwner, xnetconn* pConn, uint16 iCode, const char* sReason)
{
	xwsserver* pWsServer = (xwsserver*)pOwner;
	XS_ServerObject objServer = (XS_ServerObject)xrtWsServerGetUserData(pWsServer);
	
	if ( objServer->DefaultHost.WsEventProc ) {
		void (*WsEventProc)(XS_ServerObject, XS_HostObject, xnetconn*, int, int, const char*, size_t) 
			= objServer->DefaultHost.WsEventProc;
		WsEventProc(objServer, &objServer->DefaultHost, pConn, XRT_EV_CLOSE, iCode, sReason, sReason ? strlen(sReason) : 0);
	}
}





// 启动 WebSocket 服务
int RunServerWS(XS_ServerObject objServer)
{
	char sIP[64] = {0};
	uint16 iPort = 0;
	
	// 解析地址端口
	ParseAddr(objServer->Addr, sIP, sizeof(sIP), &iPort);
	
	// 配置 WebSocket 服务器
	xwsconfig tConfig = {0};
	tConfig.sPath = "/";
	tConfig.iMaxMessageSize = 10 * 1024 * 1024;  // 10MB
	tConfig.iHandshakeTimeoutSec = 30;
	
	// 事件回调
	xwsevents tEvents = {0};
	tEvents.OnOpen = OnWsOpen;
	tEvents.OnMessage = OnWsMessage;
	tEvents.OnClose = OnWsClose;
	
	// 创建 WebSocket 服务器 (共享事件循环)
	printf("    Run Server [WebSocket] : %s (%s:%d)\n", objServer->Name, sIP, iPort);
	xwsserver* pServer = xrtWsServerCreateEx(g_pLoop, sIP, iPort, &tConfig, &tEvents);
	if ( pServer == NULL ) {
		printf("    !!! ERROR !!! Cannot create WebSocket server on %s:%d\n", sIP, iPort);
		exit(EXIT_FAILURE);
	}
	
	// 设置用户数据为 Server 对象
	xrtWsServerSetUserData(pServer, objServer);
	objServer->pServer = pServer;
	
	// 启动服务器
	if ( xrtWsServerStart(pServer) != XRT_NET_OK ) {
		printf("    !!! ERROR !!! Cannot start WebSocket server\n");
		exit(EXIT_FAILURE);
	}
	
	// WSS (TLS)
	if ( objServer->EnableTLS ) {
		uint16 iTlsPort = 0;
		ParseAddr(objServer->AddrTLS, sIP, sizeof(sIP), &iTlsPort);
		
		printf("    Run Server [WebSocket TLS] : %s (%s:%d)\n", objServer->Name, sIP, iTlsPort);
		
		// 创建 WSS 服务器
		xwsserver* pServerTLS = xrtWsServerCreateEx(g_pLoop, sIP, iTlsPort, &tConfig, &tEvents);
		if ( pServerTLS == NULL ) {
			printf("    !!! ERROR !!! Cannot create WSS server on %s:%d\n", sIP, iTlsPort);
			exit(EXIT_FAILURE);
		}
		
		xrtWsServerSetUserData(pServerTLS, objServer);
		objServer->pServerTLS = pServerTLS;
		
		// 配置 TLS
		if ( objServer->EnableDefaultHost && objServer->DefaultHost.tTlsConfig.sCertFile ) {
			xtlsconfig tTlsConfig = {0};
			tTlsConfig.sCertFile = objServer->DefaultHost.tTlsConfig.sCertFile;
			tTlsConfig.sKeyFile = objServer->DefaultHost.tTlsConfig.sKeyFile;
			
			if ( xrtWsServerEnableTLS(pServerTLS, &tTlsConfig) != XRT_NET_OK ) {
				printf("    !!! ERROR !!! Cannot enable TLS for WSS server\n");
				exit(EXIT_FAILURE);
			}
		}
		
		// 启动 WSS 服务器
		if ( xrtWsServerStart(pServerTLS) != XRT_NET_OK ) {
			printf("    !!! ERROR !!! Cannot start WSS server\n");
			exit(EXIT_FAILURE);
		}
	}
	
	return TRUE;
}



// 停止 WebSocket 服务
int StopServerWS(XS_ServerObject objServer)
{
	printf("    Stop Server [WebSocket] : %s\n", objServer->Name);
	
	if ( objServer->pServer ) {
		xrtWsServerDestroy((xwsserver*)objServer->pServer);
		objServer->pServer = NULL;
	}
	if ( objServer->pServerTLS ) {
		xrtWsServerDestroy((xwsserver*)objServer->pServerTLS);
		objServer->pServerTLS = NULL;
	}
	
	return TRUE;
}


