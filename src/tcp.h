


// TCP 协议处理
static void ProcTCP(struct mg_connection* c, int ev, void *ev_data) {
	XS_ServerObject objServer = (XS_ServerObject)c->fn_data;
	if ( objServer->DefaultHost.DevLang == SLT_C ) {
		if ( objServer->DefaultHost.EventProc ) {
			void (*EventProc)(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection *c, int ev, void *ev_data) = objServer->DefaultHost.EventProc;
			EventProc(objServer, &objServer->DefaultHost, c, ev, ev_data);
		} else {
			printf("!!! ERROR !!! not found EventProc [TCP] !");
			return;
		}
	} else if ( objServer->DefaultHost.DevLang == SLT_LUA ) {
	} else if ( objServer->DefaultHost.DevLang == SLT_JS ) {
	} else {
		// 没有设置开发语言，报错返回
		printf("!!! ERROR !!! must select devlang [TCP] !");
		return;
	}
}





// 启动 TCP 服务
int RunServerTCP(XS_ServerObject objServer)
{
	// 初始化 TCP 服务
	DynLoad_C_GlobalData(objServer);
	if ( objServer->DefaultHost.ServiceInit ) {
		void (*ServiceInit)(XS_ServerObject objServer) = objServer->DefaultHost.ServiceInit;
		ServiceInit(objServer);
	}
	// 启动 TCP 服务
	printf("Run Server [TCP] : %s (%s)\n", objServer->Name, objServer->Addr);
	struct mg_connection* objConn = mg_listen(&mgr, objServer->Addr, ProcTCP, objServer);
	if ( objConn == NULL ) {
		printf("!!! ERROR !!! Cannot listen on %s. Use tcp://ADDR:PORT or :PORT\n", objServer->Addr);
		exit(EXIT_FAILURE);
	} else {
		objServer->Conn = objConn;
	}
	return TRUE;
}



// 停止 TCP 服务
int StopServerTCP(XS_ServerObject objServer)
{
	if ( objServer->DefaultHost.ServiceUnit ) {
		void (*ServiceUnit)(XS_ServerObject objServer) = objServer->DefaultHost.ServiceUnit;
		ServiceUnit(objServer);
	}
	return TRUE;
}


