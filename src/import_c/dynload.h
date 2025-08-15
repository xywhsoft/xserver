


#include "import_all.h"



// C 语言脚本报错处理
void handle_error(void *opaque, const char *msg)
{
    fprintf(opaque, "%s\n", msg);
}



// 创建动态加载 C 语言脚本运行环境
void DynLoad_C(XS_ServerObject objServer, XS_HostObject objHost)
{
	TCCState* s = tcc_new();
	if ( s == NULL ) {
		printf("!!! ERROR !!! TCCState create failed !\n");
		exit(EXIT_FAILURE);
	}
	// 设置错误输出回调函数
	tcc_set_error_func(s, stderr, handle_error);
	// 添加 引用文件、库文件 目录
	tcc_add_include_path(s, "tcc/include/winapi");
	tcc_add_include_path(s, "tcc/include");
	tcc_add_include_path(s, "tcc/inc_xs");
	tcc_add_library_path(s, "tcc/lib");
	tcc_add_include_path(s, objHost->Path);
	tcc_add_library_path(s, objHost->Path);
	char* sPath = xrtPathGetDir(objHost->DevFile, 0);
	tcc_add_include_path(s, sPath);
	tcc_add_library_path(s, sPath);
	xrtFree(sPath);
	// 设置编译到内存
	tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
	// 检查文件是否存在
	if ( xrtFileExists(objHost->DevFile) == FALSE ) {
		printf("!!! ERROR !!! cannot find file : %s\n", objHost->DevFile);
		exit(EXIT_FAILURE);
	}
	// 编译文件
	char* sCode = xrtFileReadAll(objHost->DevFile, XRT_CP_BINARY);
	if ( tcc_compile_string(s, sCode) == -1 ) {
		printf("!!! ERROR !!! tcc_compile_string failed !\n");
		exit(EXIT_FAILURE);
	}
	// 导入运行时和全局数据
	ImportAll(s);
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
	objHost->LoopProc = tcc_get_symbol(s, "ServiceLoop");
	objHost->XS_SetGlobalDate = tcc_get_symbol(s, "XS_SetGlobalDate");
}



// 动态导入全局数据
void DynLoad_C_GlobalData(XS_ServerObject objServer)
{
	if ( objServer->DefaultHost.XS_SetGlobalDate ) {
		objServer->DefaultHost.XS_SetGlobalDate(1, &mgr);
		objServer->DefaultHost.XS_SetGlobalDate(2, ServerList);
		objServer->DefaultHost.XS_SetGlobalDate(3, &xCore);
	}
}


