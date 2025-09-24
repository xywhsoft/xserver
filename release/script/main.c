


// XS 基础服务库
#include <xsbase.h>



// 导入库
#include "xdo/xdo.h"
#include "xdo/sqlite.h"
//#include "xdo/mysql.h"
//#include "xdo/odbc.h"				/* 可以根据实际使用情况决定是否引用 ODBC 数据库驱动程序 */





// 全局路径
char* ExePath;
char* AppPath;
char* DBPath;
char* TempPath;
char* ToolPath;
char* OptionPath;
char* TemplatePath;



// 全局文件路径
char* FileDB;



// 全局数据库对象
sqlite3* objDB;




// 模板相关功能
#include "module/template.h"



// 路由调用 - HTTP
#include "route_http/test.h"



// 全局静态路由表
#include "route.h"



// HTTP 协议处理
#include "module/http.h"





// 服务初始化
void ServiceInit(XS_ServerObject objServer)
{
	// 开启 xrt 库的调试模式（会将错误输出到控制台）
	xCore->DebugMode = TRUE;
	
	
	
	// 初始化全局目录
	ExePath = xCore->AppPath;
	AppPath = objServer->DefaultHost.Path;
	DBPath = xrtPathJoin(3, ExePath, "data", "db");
	TempPath = xrtPathJoin(3, ExePath, "data", "temp");
	ToolPath = xrtPathJoin(3, ExePath, "data", "tools");
	OptionPath = xrtPathJoin(3, ExePath, "data", "options");
	TemplatePath = xrtPathJoin(3, ExePath, "data", "template");
	
	
	
	// 自动创建目录
	xrtDirCreate(TempPath);
	
	
	
	// 连接到主数据库
	FileDB = xrtPathJoin(2, DBPath, "main.db");
	int iRet = sqlite3_open(FileDB, &objDB);
	if ( iRet != SQLITE_OK ) {
		printf("!!! ERROR !!! ServiceInit - sqlite3_open error code : %d\n", iRet);
	}
	
	
	
	// 初始化模板渲染功能
	InitTemplate();
	
	
	
	// 初始化 HTTP 路由表
	InitRouteHTTP();
	
	
	
}



// 服务卸载
void ServiceUnit(XS_ServerObject objServer)
{
	
	// 释放全局路由表
	AVLHT32_Destroy(StaticRouteTableHTTP);
	
	// 释放数据库
	sqlite3_close(objDB);
	sqlite3_shutdown();
	
}