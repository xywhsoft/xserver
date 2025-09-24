


// 全局静态路由表 - HTTP
typedef struct {
	void (*Proc)(XS_ServerObject objServer, XS_HostObject objHost, struct mg_connection* c, struct mg_http_message* hm);
} RouteItemHTTP;
AVLHT32_Object StaticRouteTableHTTP;

// 添加全局静态路由表项 - HTTP
void AddStaticRouteHTTP(char* uri, void* proc)
{
	RouteItemHTTP* objItem = AVLHT32_Set(StaticRouteTableHTTP, uri, strlen(uri), NULL);
	if ( objItem ) {
		objItem->Proc = proc;
	}
}



// 初始化 HTTP 路由表
void InitRouteHTTP()
{
	// 创建 HTTP 全局静态路由表
	StaticRouteTableHTTP = AVLHT32_Create(sizeof(RouteItemHTTP));
	
	// 添加 HTTP 静态路由 - curd
	AddStaticRouteHTTP("/app/list",							Request_List);
	AddStaticRouteHTTP("/app/add",							Request_Add);
	AddStaticRouteHTTP("/app/del",							Request_Del);
	AddStaticRouteHTTP("/app/edit",							Request_Edit);
	
	// 添加 HTTP 静态路由 - Test
	AddStaticRouteHTTP("/test",								Request_Test);
}


