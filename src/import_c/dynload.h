


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
	objHost->XS_SetGlobalDate = tcc_get_symbol(s, "XS_SetGlobalDate");
}



// 动态导入全局数据
void DynLoad_C_GlobalData(XS_ServerObject objServer, XS_HostObject objHost)
{
	if ( objHost->XS_SetGlobalDate ) {
		objHost->XS_SetGlobalDate(1, &mgr);
		objHost->XS_SetGlobalDate(2, ServerList);
		objHost->XS_SetGlobalDate(3, &xCore);
	}
}


