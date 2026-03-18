


#include "import_all.h"



// 创建动态加载 C 语言脚本运行环境
void DynLoad_C(XS_ServerObject objServer, XS_HostObject objHost)
{
	// 使用 xsCreateTCC 创建 TCC 状态机
	TCCState* s = xsCreateTCC(objHost->Path);
	if ( s == NULL ) {
		printf("!!! ERROR !!! TCCState create failed !\n");
		exit(EXIT_FAILURE);
	}
	// 添加开发文件目录
	char* sPath = xrtPathGetDir(objHost->DevFile, 0);
	tcc_add_include_path(s, sPath);
	tcc_add_library_path(s, sPath);
	xrtFree(sPath);
	// 检查文件是否存在
	if ( xrtFileExists(objHost->DevFile) == FALSE ) {
		printf("!!! ERROR !!! cannot find file : %s\n", objHost->DevFile);
		exit(EXIT_FAILURE);
	}
	// 编译文件
	char* sCode = xrtFileReadAll(objHost->DevFile, XRT_CP_BINARY, NULL);
	if ( tcc_compile_string(s, sCode) == -1 ) {
		printf("!!! ERROR !!! tcc_compile_string failed !\n");
		xrtFree(sCode);
		exit(EXIT_FAILURE);
	}
	xrtFree(sCode);
	// 地址重定向
	if ( tcc_relocate(s) < 0 ) {
		printf("!!! ERROR !!! tcc_relocate failed !\n");
		exit(EXIT_FAILURE);
	}
	objHost->DevObj = s;
	// 查找入口点
	objHost->ServiceInit = tcc_get_symbol(s, "ServiceInit");
	objHost->ServiceStart = tcc_get_symbol(s, "ServiceStart");
	objHost->ServiceUnit = tcc_get_symbol(s, "ServiceUnit");
	objHost->EventProc = tcc_get_symbol(s, "EventProc");
	objHost->RequestProc = tcc_get_symbol(s, "RequestProc");
	objHost->WsEventProc = tcc_get_symbol(s, "WsEventProc");
	objHost->XS_SetGlobalDate = tcc_get_symbol(s, "XS_SetGlobalDate");
}



// 动态导入全局数据
void DynLoad_C_GlobalData(XS_ServerObject objServer, XS_HostObject objHost)
{
	if ( objHost->XS_SetGlobalDate ) {
		objHost->XS_SetGlobalDate(1, g_pLoop);       // xeventloop*
		objHost->XS_SetGlobalDate(2, ServerList);    // 服务器列表
		objHost->XS_SetGlobalDate(3, &xCore);        // xrt 核心
	}
}



// 重新加载 C 语言脚本（内部函数，不导出给脚本）
// 返回: 0=成功, -1=非 C 语言, -2=文件不存在, -3=TCC 创建失败, -4=编译失败, -5=重定位失败
static int DynReload_C(XS_ServerObject objServer, XS_HostObject objHost)
{
	// 检查是否为 C 语言脚本
	if ( objHost->DevLang != SLT_C ) {
		printf("[Reload] Host '%s' is not C language script\n", objHost->Name ? (char*)objHost->Name : "(null)");
		return -1;
	}
	
	// 检查文件是否存在
	if ( xrtFileExists(objHost->DevFile) == FALSE ) {
		printf("[Reload] Cannot find file: %s\n", objHost->DevFile);
		return -2;
	}
	
	printf("[Reload] Reloading host: %s (%s)\n", objHost->Name ? (char*)objHost->Name : "(null)", objHost->DevFile);
	
	// 调用旧脚本的 ServiceUnit（清理资源）
	if ( objHost->ServiceUnit ) {
		printf("[Reload] Calling ServiceUnit...\n");
		void (*ServiceUnit)(XS_ServerObject, XS_HostObject) = objHost->ServiceUnit;
		ServiceUnit(objServer, objHost);
	}
	
	// 销毁旧 TCC 状态机
	if ( objHost->DevObj ) {
		printf("[Reload] Destroying old TCC state...\n");
		tcc_delete((TCCState*)objHost->DevObj);
		objHost->DevObj = NULL;
	}
	
	// 清空回调函数指针
	objHost->ServiceInit = NULL;
	objHost->ServiceStart = NULL;
	objHost->ServiceUnit = NULL;
	objHost->EventProc = NULL;
	objHost->RequestProc = NULL;
	objHost->WsEventProc = NULL;
	objHost->XS_SetGlobalDate = NULL;
	
	// 创建新 TCC 状态机
	TCCState* s = xsCreateTCC(objHost->Path);
	if ( s == NULL ) {
		printf("[Reload] ERROR: TCCState create failed!\n");
		return -3;
	}
	
	// 添加开发文件目录
	char* sPath = xrtPathGetDir(objHost->DevFile, 0);
	tcc_add_include_path(s, sPath);
	tcc_add_library_path(s, sPath);
	xrtFree(sPath);
	
	// 编译文件
	char* sCode = xrtFileReadAll(objHost->DevFile, XRT_CP_BINARY, NULL);
	if ( tcc_compile_string(s, sCode) == -1 ) {
		printf("[Reload] ERROR: tcc_compile_string failed!\n");
		xrtFree(sCode);
		tcc_delete(s);
		return -4;
	}
	xrtFree(sCode);
	
	// 地址重定向
	if ( tcc_relocate(s) < 0 ) {
		printf("[Reload] ERROR: tcc_relocate failed!\n");
		tcc_delete(s);
		return -5;
	}
	
	// 保存新 TCC 状态机
	objHost->DevObj = s;
	
	// 查找入口点
	objHost->ServiceInit = tcc_get_symbol(s, "ServiceInit");
	objHost->ServiceStart = tcc_get_symbol(s, "ServiceStart");
	objHost->ServiceUnit = tcc_get_symbol(s, "ServiceUnit");
	objHost->EventProc = tcc_get_symbol(s, "EventProc");
	objHost->RequestProc = tcc_get_symbol(s, "RequestProc");
	objHost->WsEventProc = tcc_get_symbol(s, "WsEventProc");
	objHost->XS_SetGlobalDate = tcc_get_symbol(s, "XS_SetGlobalDate");
	
	// 传递全局数据
	DynLoad_C_GlobalData(objServer, objHost);
	
	// 调用新脚本的 ServiceInit
	if ( objHost->ServiceInit ) {
		printf("[Reload] Calling ServiceInit...\n");
		void (*ServiceInit)(XS_ServerObject, XS_HostObject) = objHost->ServiceInit;
		ServiceInit(objServer, objHost);
	}
	
	printf("[Reload] Host '%s' reloaded successfully\n", objHost->Name ? (char*)objHost->Name : "(null)");
	return 0;
}



// ==================== 热加载 API ====================

// 热加载指定 Host 的脚本
// 参数:
//   objServer - 服务器对象
//   objHost   - Host 对象
// 返回: 0=成功, 负数=失败
int xsReloadHost(XS_ServerObject objServer, XS_HostObject objHost)
{
	if ( objServer == NULL || objHost == NULL ) {
		printf("[Reload] ERROR: Invalid parameters\n");
		return -100;
	}
	return DynReload_C(objServer, objHost);
}



// 通过域名热加载 Host 的脚本
// 参数:
//   objServer - 服务器对象
//   sDomain   - 域名（Host 配置中的 host 字段）
// 返回: 0=成功, 负数=失败
int xsReloadHostByDomain(XS_ServerObject objServer, const char* sDomain)
{
	if ( objServer == NULL || sDomain == NULL || sDomain[0] == '\0' ) {
		printf("[Reload] ERROR: Invalid parameters\n");
		return -100;
	}
	
	// 通过 HostMap 查找 Host
	XS_HostObject* ppHost = xrtDictGet(objServer->HostMap, (ptr)sDomain, strlen(sDomain));
	if ( ppHost == NULL ) {
		printf("[Reload] ERROR: Host not found for domain: %s\n", sDomain);
		return -101;
	}
	
	return DynReload_C(objServer, *ppHost);
}



// 热加载 DefaultHost 的脚本
// 参数:
//   objServer - 服务器对象
// 返回: 0=成功, 负数=失败
int xsReloadDefaultHost(XS_ServerObject objServer)
{
	if ( objServer == NULL ) {
		printf("[Reload] ERROR: Invalid server\n");
		return -100;
	}
	if ( objServer->EnableDefaultHost == FALSE ) {
		printf("[Reload] ERROR: DefaultHost is not enabled\n");
		return -102;
	}
	return DynReload_C(objServer, &objServer->DefaultHost);
}



// 热加载整个 Server 的所有 Host
// 参数:
//   objServer - 服务器对象
// 返回: 成功加载的 Host 数量，负数=失败
int xsReloadServer(XS_ServerObject objServer)
{
	if ( objServer == NULL ) {
		printf("[Reload] ERROR: Invalid server\n");
		return -100;
	}
	
	printf("[Reload] Reloading server: %s\n", objServer->Name ? (char*)objServer->Name : "(null)");
	
	int iSuccessCount = 0;
	int iFailCount = 0;
	
	// 热加载 DefaultHost
	if ( objServer->EnableDefaultHost ) {
		if ( DynReload_C(objServer, &objServer->DefaultHost) == 0 ) {
			iSuccessCount++;
		} else {
			iFailCount++;
		}
	}
	
	// 热加载所有 Hosts
	uint32 iCount = objServer->Hosts->Count;
	for ( uint32 i = 1; i <= iCount; i++ ) {
		XS_HostObject objHost = xrtArrayGet_Inline(objServer->Hosts, i);
		if ( objHost ) {
			if ( DynReload_C(objServer, objHost) == 0 ) {
				iSuccessCount++;
			} else {
				iFailCount++;
			}
		}
	}
	
	printf("[Reload] Server '%s' reload complete: %d success, %d failed\n", 
		   objServer->Name ? (char*)objServer->Name : "(null)", iSuccessCount, iFailCount);
	
	return iSuccessCount;
}


