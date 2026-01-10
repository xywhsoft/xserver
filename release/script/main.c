


// XS 基础服务库
#include <xsbase.h>





// 全局路径
str ExePath;
str AppPath;
str WebPath;
str DBPath;
str TempPath;
str ToolPath;
str OptionPath;
str TemplatePath;



// 全局数据库对象
XDO_Connect G_DB;




// 模板相关功能
#include "module/template.h"



// 路由调用 - HTTP
#include "route_http/curd.h"
#include "route_http/chart.h"
#include "route_http/test.h"



// 全局静态路由表
#include "route.h"



// HTTP 协议处理
#include "module/http.h"





// 服务初始化
void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
	
	
	
	// 初始化全局目录
	ExePath = xCore->AppPath;
	WebPath = objHost->Path;
	AppPath = xrtPathGetDir(WebPath, 0);
	DBPath = xrtPathJoin(3, AppPath, "data", "db");
	TempPath = xrtPathJoin(3, AppPath, "data", "temp");
	ToolPath = xrtPathJoin(2, ExePath, "tools");
	OptionPath = xrtPathJoin(3, AppPath, "data", "options");
	TemplatePath = xrtPathJoin(3, AppPath, "data", "template");
	
	
	
	// 自动创建目录
	xrtDirCreate(TempPath);
	
	
	
	// 连接到主数据库
	str FileDB = xrtPathJoin(2, DBPath, "main.db");
	G_DB = xdoConnectSQLite(FileDB);
	xrtFree(FileDB);  // 释放路径内存
	if ( G_DB == NULL ) {
		printf("!!! ERROR !!! ServiceInit - xdoConnectSQLite error.\n");
		return;  // 数据库连接失败时返回
	}
	
	
	
	// 初始化模板渲染功能
	InitTemplate();
	
	
	
	// 初始化 HTTP 路由表
	InitRouteHTTP();
	
    
	
}



// 服务卸载
void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
	
	// 释放全局路由表
	if ( StaticRouteTableHTTP ) {
		xrtDictDestroy(StaticRouteTableHTTP);
		StaticRouteTableHTTP = NULL;
	}
	
	// 释放数据库
	if ( G_DB ) {
		xdoDisconnect(G_DB);
		G_DB = NULL;
	}
	
	// 释放模板资源
	FreeTemplate();
	
	// 释放全局路径内存
	if ( AppPath ) {
		xrtFree(AppPath);
		AppPath = NULL;
	}
	if ( DBPath ) {
		xrtFree(DBPath);
		DBPath = NULL;
	}
	if ( TempPath ) {
		xrtFree(TempPath);
		TempPath = NULL;
	}
	if ( ToolPath ) {
		xrtFree(ToolPath);
		ToolPath = NULL;
	}
	if ( OptionPath ) {
		xrtFree(OptionPath);
		OptionPath = NULL;
	}
	if ( TemplatePath ) {
		xrtFree(TemplatePath);
		TemplatePath = NULL;
	}
	
}