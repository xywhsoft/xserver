/*
 * demo-single — xServer 单 Host C 应用范例
 *
 * 这是整个服务的入口文件。它只负责引用模块、引用路由处理代码，以及按
 * 明确顺序初始化和释放资源。具体 HTTP 处理过程位于 route_http 目录。
 *
 * 目录约定：
 *   demo-single/
 *   ├── main.c          服务总入口
 *   ├── route.h         HTTP 路由注册清单
 *   ├── db/             SQLite 数据库文件
 *   ├── includes/       TCC 默认第三方头文件目录
 *   ├── librarys/       TCC 默认第三方库目录
 *   ├── modules/        当前服务复用的功能模块
 *   ├── options/        应用配置文件
 *   ├── page/           必须经 LoadPage 返回的受控页面
 *   ├── template/       必须经 LoadTemplate 渲染的模板
 *   ├── route_http/     HTTP 路由处理函数
 *   └── wwwroot/        唯一可被 xServer 直接访问的静态网站根目录
 *
 * 运行：
 *   release\xs.exe release\demo-single\xs.json
 * 或在 demo-single 目录直接运行：
 *   xs.exe
 */



/* xServer SDK、SQLite 和 C 标准库。 */
#include <xsbase.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>



/* 演示 includes 目录会被加入 TCC 头文件搜索路径。 */
#include <demo_extra.h>



/* 公共模块按依赖顺序引用。
 * 这些 .h 是当前 TCC 应用的实现单元，最终共同编译成一个翻译单元。 */
#include "modules/http.h"
#include "modules/resource.h"
#include "modules/page.h"
#include "modules/template.h"
#include "modules/database.h"
#include "modules/protocol.h"



/* HTTP 处理函数必须先于 route.h 出现，路由注册时才能取得函数地址。 */
#include "route_http/basic.h"
#include "route_http/chart.h"
#include "route_http/item.h"
#include "route_http/app.h"



/* 集中注册全部 URI。 */
#include "route.h"



/* xServer 在本代脚本开始服务前调用。 */
void ServiceInit(XS_HostInfo* pHost)
{
	str sAppPath;

	/* host.path 指向 wwwroot，因此它的父目录就是当前应用根目录。 */
	sAppPath = xrtPathParent((pHost && pHost->Path) ? pHost->Path : "");
	if ( sAppPath == NULL ) {
		printf("[demo][error] resolve application path failed\n");
		return;
	}

	(void)DB_Init(sAppPath);
	(void)Page_Init(sAppPath);
	(void)Template_Init(sAppPath);
	RouteHTTP_Init();
	xrtFree(sAppPath);
}



/* 资源按与初始化相反的顺序释放。xServer 会等本代请求退出后再调用。 */
void ServiceUnit(XS_HostInfo* pHost)
{
	(void)pHost;
	RouteHTTP_Unit();
	Template_Unit();
	Page_Unit();
	DB_Unit();
}
