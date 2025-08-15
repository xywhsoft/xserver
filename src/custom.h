




// 启动 自定义 服务
int RunServerCustom(XS_ServerObject objServer)
{
	DynLoad_C_GlobalData(objServer);
	if ( objServer->DefaultHost.ServiceInit ) {
		void (*ServiceInit)(XS_ServerObject objServer) = objServer->DefaultHost.ServiceInit;
		ServiceInit(objServer);
	}
	if ( objServer->DefaultHost.ServiceStart ) {
		void (*ServiceStart)(XS_ServerObject objServer) = objServer->DefaultHost.ServiceStart;
		ServiceStart(objServer);
	}
}



// 停止 自定义 服务
int StopServerCustom(XS_ServerObject objServer)
{
	if ( objServer->DefaultHost.ServiceUnit ) {
		void (*ServiceUnit)(XS_ServerObject objServer) = objServer->DefaultHost.ServiceUnit;
		ServiceUnit(objServer);
	}
}


