




// 启动 自定义 服务
int RunServerThread(XS_ServerObject objServer)
{
	for ( int i = 1; i <= objServer->HostCount; i++ ) {
		XS_HostObject objHost = xrtArrayGet_Inline(objServer->Hosts, i);
		if ( objHost->ServiceStart ) {
			void (*ServiceStart)(XS_ServerObject objServer, XS_HostObject objHost) = objHost->ServiceStart;
			ServiceStart(objServer, objHost);
		}
	}
	return TRUE;
}



// 停止 自定义 服务
int StopServerThread(XS_ServerObject objServer)
{
	return TRUE;
}


