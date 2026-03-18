


// ==================== UDP 服务 ====================
// 基于 xrt UDP 服务器 (xudpserver) 实现



// UDP 数据接收回调
static void OnUdpRecvFrom(ptr pOwner, xnetconn* pConn, const xnetaddr* pFromAddr, const char* pData, size_t iLen)
{
	xudpserver* pUdpServer = (xudpserver*)pOwner;
	XS_ServerObject objServer = (XS_ServerObject)xrtUdpServerGetUserData(pUdpServer);
	
	if ( objServer->DefaultHost.EventProc ) {
		// 将地址信息存入连接对象的 pUserData 临时保存
		pConn->pUserData = (ptr)pFromAddr;
		
		void (*EventProc)(XS_ServerObject, XS_HostObject, xnetconn*, int, const char*, size_t) 
			= objServer->DefaultHost.EventProc;
		EventProc(objServer, &objServer->DefaultHost, pConn, XRT_EV_RECV, pData, iLen);
	}
}





// 启动 UDP 服务
int RunServerUDP(XS_ServerObject objServer)
{
	char sIP[64] = {0};
	uint16 iPort = 0;
	
	// 解析地址端口
	ParseAddr(objServer->Addr, sIP, sizeof(sIP), &iPort);
	
	// 配置 UDP 服务器
	xnetconfig tConfig = {0};
	tConfig.iRecvBufSize = 65536;  // UDP 最大包大小
	
	// 事件回调
	xnetevents tEvents = {0};
	tEvents.OnRecvFrom = OnUdpRecvFrom;
	
	// 创建 UDP 服务器 (共享事件循环)
	printf("    Run Server [UDP] : %s (%s:%d)\n", objServer->Name, sIP, iPort);
	xudpserver* pServer = xrtUdpServerCreateEx(g_pLoop, sIP, iPort, &tConfig, &tEvents);
	if ( pServer == NULL ) {
		printf("    !!! ERROR !!! Cannot create UDP server on %s:%d\n", sIP, iPort);
		exit(EXIT_FAILURE);
	}
	
	// 设置用户数据为 Server 对象
	xrtUdpServerSetUserData(pServer, objServer);
	objServer->pServer = pServer;
	
	// 启动服务器
	if ( xrtUdpServerStart(pServer) != XRT_NET_OK ) {
		printf("    !!! ERROR !!! Cannot start UDP server\n");
		exit(EXIT_FAILURE);
	}
	
	return TRUE;
}



// 停止 UDP 服务
int StopServerUDP(XS_ServerObject objServer)
{
	printf("    Stop Server [UDP] : %s\n", objServer->Name);
	
	if ( objServer->pServer ) {
		xrtUdpServerDestroy((xudpserver*)objServer->pServer);
		objServer->pServer = NULL;
	}
	
	return TRUE;
}


