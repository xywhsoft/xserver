/*
 * HTTP 路由注册清单
 *
 * 阅读一个服务时可以先看这个文件：左边是 URI，中间是允许的方法，右边是
 * 处理函数。同一个 URI 可以重复注册不同方法；如果 URI 和方法都重复，
 * protocol.h 会输出警告并用后注册的回调覆盖旧回调。
 */



static void RouteHTTP_Init(void)
{
	G_StaticRouteTableHTTP = xrtMapCreate(sizeof(RouteInfoHTTP));
	if ( G_StaticRouteTableHTTP == NULL ) {
		printf("[route][error] create static route table failed\n");
		return;
	}

	/* ========================================================
	 * 基础响应与受控页面
	 * ======================================================== */
	AddStaticRouteHTTP("/text",     XHTTP_METHOD_GET, Handle_Text);
	AddStaticRouteHTTP("/json",     XHTTP_METHOD_GET, Handle_JSON);
	AddStaticRouteHTTP("/test",     XHTTP_METHOD_GET, Handle_Test);
	AddStaticRouteHTTP("/page",     XHTTP_METHOD_GET, Handle_Page);
	AddStaticRouteHTTP("/template", XHTTP_METHOD_GET, Handle_Template);

	/* ========================================================
	 * 原有 item 示例接口
	 * ======================================================== */
	AddStaticRouteHTTP("/api/list", XHTTP_METHOD_GET,  Handle_API_List);
	AddStaticRouteHTTP("/api/add",  XHTTP_METHOD_POST, Handle_API_Add);

	/* ========================================================
	 * RESTful item 接口
	 *
	 * 集合 URI 被注册两次，GET 与 POST 分别进入不同槽位。
	 * 单项动态 URI 被注册三次，GET、PUT/PATCH、DELETE 也各自独立。
	 * 若多个方法确实共用一个处理函数，也可以直接传入
	 * XHTTP_METHOD_CRUD 或 XHTTP_METHOD_ANY。
	 * ======================================================== */
	AddStaticRouteHTTP("/api/v1/items", XHTTP_METHOD_GET,  Handle_API_List);
	AddStaticRouteHTTP("/api/v1/items", XHTTP_METHOD_POST, Handle_API_Add);

	AddDynamicRouteHTTP("/api/v1/items/{id}",
		XHTTP_METHOD_GET, Handle_API_Item);
	AddDynamicRouteHTTP("/api/v1/items/{id}",
		XHTTP_METHOD_PUT | XHTTP_METHOD_PATCH, Handle_API_Item_Update);
	AddDynamicRouteHTTP("/api/v1/items/{id}",
		XHTTP_METHOD_DELETE, Handle_API_Item_Delete);

	/* 保留已经存在的动态路由示例。 */
	AddDynamicRouteHTTP("/api/v1/item/{id}",
		XHTTP_METHOD_GET, Handle_API_Item);
	AddDynamicRouteHTTP("/api/v1/file/{name}.txt",
		XHTTP_METHOD_GET, Handle_API_File);

	/* ========================================================
	 * 图表和旧版 dev/v1 表格范例
	 * ======================================================== */
	AddStaticRouteHTTP("/chart/get", XHTTP_METHOD_GET, Handle_Chart_Get);
	AddStaticRouteHTTP("/app/list",  XHTTP_METHOD_GET,  Handle_App_List);
	AddStaticRouteHTTP("/app/add",   XHTTP_METHOD_POST, Handle_App_Add);
	AddStaticRouteHTTP("/app/del",   XHTTP_METHOD_POST, Handle_App_Del);
	AddStaticRouteHTTP("/app/edit",  XHTTP_METHOD_POST, Handle_App_Edit);

	/* 动态路由必须在全部注册完成后编译一次。 */
	(void)RouteHTTP_Compile();
}



static void RouteHTTP_Unit(void)
{
	uint32 i;

	if ( G_DynamicRoutePattern != NULL ) {
		xrtPatternRelease(G_DynamicRoutePattern);
		G_DynamicRoutePattern = NULL;
	}
	for ( i = 0; i < G_iDynCount; i++ ) {
		xrtFree(G_arrDynPattern[i]);
		G_arrDynPattern[i] = NULL;
		memset(&G_arrDynInfo[i], 0, sizeof(G_arrDynInfo[i]));
	}
	G_iDynCount = 0;

	if ( G_StaticRouteTableHTTP != NULL ) {
		xrtMapDestroy(G_StaticRouteTableHTTP);
		G_StaticRouteTableHTTP = NULL;
	}
}
