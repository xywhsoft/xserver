


// ==================== TCP 服务 ====================
// 基于 xrt TCP 服务器 (xtcpserver) 实现



// TCP 连接接受回调
static void OnTcpAccept(ptr pOwner, xnetconn* pConn)
{
	xtcpserver* pTcpServer = (xtcpserver*)pOwner;
	XS_ServerObject objServer = (XS_ServerObject)xrtTcpServerGetUserData(pTcpServer);
	
	if ( objServer->DefaultHost.EventProc ) {
		void (*EventProc)(XS_ServerObject, XS_HostObject, xnetconn*, int, const char*, size_t) 
			= objServer->DefaultHost.EventProc;
		EventProc(objServer, &objServer->DefaultHost, pConn, XRT_EV_ACCEPT, NULL, 0);
	}
}



// TCP 数据接收回调
static void OnTcpRecv(ptr pOwner, xnetconn* pConn, const char* pData, size_t iLen)
{
	xtcpserver* pTcpServer = (xtcpserver*)pOwner;
	XS_ServerObject objServer = (XS_ServerObject)xrtTcpServerGetUserData(pTcpServer);
	
	if ( objServer->DefaultHost.EventProc ) {
		void (*EventProc)(XS_ServerObject, XS_HostObject, xnetconn*, int, const char*, size_t) 
			= objServer->DefaultHost.EventProc;
		EventProc(objServer, &objServer->DefaultHost, pConn, XRT_EV_RECV, pData, iLen);
	}
}



// TCP 连接关闭回调
static void OnTcpClose(ptr pOwner, xnetconn* pConn)
{
	xtcpserver* pTcpServer = (xtcpserver*)pOwner;
	XS_ServerObject objServer = (XS_ServerObject)xrtTcpServerGetUserData(pTcpServer);
	
	if ( objServer->DefaultHost.EventProc ) {
		void (*EventProc)(XS_ServerObject, XS_HostObject, xnetconn*, int, const char*, size_t) 
			= objServer->DefaultHost.EventProc;
		EventProc(objServer, &objServer->DefaultHost, pConn, XRT_EV_CLOSE, NULL, 0);
	}
}





// 启动 TCP 服务
int RunServerTCP(XS_ServerObject objServer)
{
	char sIP[64] = {0};
	uint16 iPort = 0;
	
	// 解析地址端口
	ParseAddr(objServer->Addr, sIP, sizeof(sIP), &iPort);
	
	// 配置 TCP 服务器
	xnetconfig tConfig = {0};
	tConfig.iRecvBufSize = 8192;
	tConfig.iMaxClients = 1024;
	
	// 事件回调
	xnetevents tEvents = {0};
	tEvents.OnAccept = OnTcpAccept;
	tEvents.OnRecv = OnTcpRecv;
	tEvents.OnClose = OnTcpClose;
	
	// 创建 TCP 服务器 (共享事件循环)
	printf("    Run Server [TCP] : %s (%s:%d)\n", objServer->Name, sIP, iPort);
	xtcpserver* pServer = xrtTcpServerCreateEx(g_pLoop, sIP, iPort, &tConfig, &tEvents);
	if ( pServer == NULL ) {
		printf("    !!! ERROR !!! Cannot create TCP server on %s:%d\n", sIP, iPort);
		exit(EXIT_FAILURE);
	}
	
	// 设置用户数据为 Server 对象
	xrtTcpServerSetUserData(pServer, objServer);
	objServer->pServer = pServer;
	
	// 启动服务器
	if ( xrtTcpServerStart(pServer) != XRT_NET_OK ) {
		printf("    !!! ERROR !!! Cannot start TCP server\n");
		exit(EXIT_FAILURE);
	}
	
	// TLS
	if ( objServer->EnableTLS ) {
		uint16 iTlsPort = 0;
		ParseAddr(objServer->AddrTLS, sIP, sizeof(sIP), &iTlsPort);
		
		printf("    Run Server [TCP TLS] : %s (%s:%d)\n", objServer->Name, sIP, iTlsPort);
		
		// 创建 TLS TCP 服务器
		xtcpserver* pServerTLS = xrtTcpServerCreateEx(g_pLoop, sIP, iTlsPort, &tConfig, &tEvents);
		if ( pServerTLS == NULL ) {
			printf("    !!! ERROR !!! Cannot create TCP TLS server on %s:%d\n", sIP, iTlsPort);
			exit(EXIT_FAILURE);
		}
		
		xrtTcpServerSetUserData(pServerTLS, objServer);
		objServer->pServerTLS = pServerTLS;
		
		// 配置 TLS
		if ( objServer->EnableDefaultHost && objServer->DefaultHost.tTlsConfig.sCertFile ) {
			if ( xrtTcpServerEnableTLS(pServerTLS, &objServer->DefaultHost.tTlsConfig) != XRT_NET_OK ) {
				printf("    !!! ERROR !!! Cannot enable TLS for TCP server\n");
				exit(EXIT_FAILURE);
			}
		}
		
		// 启动 TLS TCP 服务器
		if ( xrtTcpServerStart(pServerTLS) != XRT_NET_OK ) {
			printf("    !!! ERROR !!! Cannot start TCP TLS server\n");
			exit(EXIT_FAILURE);
		}
	}
	
	return TRUE;
}



// 停止 TCP 服务
int StopServerTCP(XS_ServerObject objServer)
{
	printf("    Stop Server [TCP] : %s\n", objServer->Name);
	
	if ( objServer->pServer ) {
		xrtTcpServerDestroy((xtcpserver*)objServer->pServer);
		objServer->pServer = NULL;
	}
	if ( objServer->pServerTLS ) {
		xrtTcpServerDestroy((xtcpserver*)objServer->pServerTLS);
		objServer->pServerTLS = NULL;
	}
	
	return TRUE;
}


