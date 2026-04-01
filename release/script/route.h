typedef struct {
	bool (*Proc)(XS_ServerObject objServer, XS_HostObject objHost, XS_RequestObject objReq, XS_ResponseObject objResp);
} RouteItemHTTP;

xdict StaticRouteTableHTTP;



void AddStaticRouteHTTP(char* sURI, void* proc)
{
	RouteItemHTTP* objItem = xrtDictSet(StaticRouteTableHTTP, sURI, strlen(sURI), NULL);
	if ( objItem ) {
		objItem->Proc = proc;
	}
}



void InitRouteHTTP()
{
	StaticRouteTableHTTP = xrtDictCreate(sizeof(RouteItemHTTP), 0);

	AddStaticRouteHTTP("/app/list", Request_List);
	AddStaticRouteHTTP("/app/add", Request_Add);
	AddStaticRouteHTTP("/app/del", Request_Del);
	AddStaticRouteHTTP("/app/edit", Request_Edit);

	AddStaticRouteHTTP("/chart/get", Request_Chart_Get);

	AddStaticRouteHTTP("/test", Request_Test);
	AddStaticRouteHTTP("/template", Request_Template);
}
