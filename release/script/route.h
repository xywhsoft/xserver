


// 全局静态路由表 - HTTP
typedef struct {
	void (*Proc)(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm);
} RouteItemHTTP;
xdict StaticRouteTableHTTP;

// 添加全局静态路由表项 - HTTP
void AddStaticRouteHTTP(char* uri, void* proc)
{
	RouteItemHTTP* objItem = xrtDictSet(StaticRouteTableHTTP, uri, strlen(uri), NULL);
	if ( objItem ) {
		objItem->Proc = proc;
	}
}



// 初始化 HTTP 路由表
void InitRouteHTTP()
{
	// 创建 HTTP 全局静态路由表
	StaticRouteTableHTTP = xrtDictCreate(sizeof(RouteItemHTTP));
	
	// 添加 HTTP 静态路由 - curd
	AddStaticRouteHTTP("/app/list",							Request_List);
	AddStaticRouteHTTP("/app/add",							Request_Add);
	AddStaticRouteHTTP("/app/del",							Request_Del);
	AddStaticRouteHTTP("/app/edit",							Request_Edit);
	
	// 添加 HTTP 静态路由 - 图表
	AddStaticRouteHTTP("/chart/get",						Request_Chart_Get);
	
	// 添加 HTTP 静态路由 - Test
	AddStaticRouteHTTP("/test",								Request_Test);
	AddStaticRouteHTTP("/template",							Request_Template);
}


