


// HTTP 请求处理
void RequestProc(XS_ServerObject objServer, XS_HostObject objHost, xnetconn* pConn, xhttpdreq* pReq)
{
	RouteItemHTTP* objItem = xrtDictGet(StaticRouteTableHTTP, pReq->sPath, strlen(pReq->sPath));
	if ( objItem ) {
		// C 语言静态路由
		objItem->Proc(objServer, objHost, pConn, pReq);
	} else {
		// 访问服务器静态资源
		xrtHttpServeDir(pConn, pReq, objHost->Path);
	}
}


