#ifndef XS_SCRIPT_TCC_HOST_H
#define XS_SCRIPT_TCC_HOST_H

/*
 * xs3 TCC 宿主：环境创建与符号注册
 * 经验来源（xlang demo6）：
 *   - 内置 VFS（tcc_builtin_vfs）+ UTF-8 宽字符 IO：中文路径全链路支持
 *   - 符号表模式：{name, value} 数组 + 生成的 .inc 清单（本仓 2850 个 xrt 函数）
 *   - SDK 头内置于 VFS /xs：脚本永远编译与宿主二进制同版本的契约头
 */

#include <stdio.h>

#include "../../lib/libtcc.h"
#include "../sdk/xsbase.h"
#include "../../tcc/tcc_builtin_vfs.h"
#include "../core/engine.h"

typedef struct XS_TccSymbol {
	const char*		sName;
	const void*		pValue;
} XS_TccSymbol;

static void XS_TccErrorProc(void* pOpaque, const char* sMsg)
{
	(void)pOpaque;
	fprintf(stderr, "[tcc] %s\n", sMsg);
}

/* —— xrt 全量符号（tools/gen_xrt_symbols.py 生成）—— */
#define XS_XRT_SYMBOL(name)	{ #name, (const void*)(name) },
static const XS_TccSymbol g_XS_XrtSymbols[] = {
#include "import_xrt.inc"
};
#undef XS_XRT_SYMBOL

/* —— xs API 符号（契约层，见 xsbase.h）—— */
#define XS_API_SYMBOL(name)	{ #name, (const void*)(name) },
static const XS_TccSymbol g_XS_ApiSymbols[] = {
	XS_API_SYMBOL(xsServerFind)
	XS_API_SYMBOL(xsServerRetain)
	XS_API_SYMBOL(xsServerRelease)
	XS_API_SYMBOL(xsEnumServers)
	XS_API_SYMBOL(xsHostFind)
	XS_API_SYMBOL(xsEnumHosts)
	XS_API_SYMBOL(xsConfigRoot)
	XS_API_SYMBOL(xsReloadHost)
	XS_API_SYMBOL(xsReloadServer)
	XS_API_SYMBOL(xsReloadAll)
	XS_API_SYMBOL(xsReloadHostSubmit)
	XS_API_SYMBOL(xsReloadServerSubmit)
	XS_API_SYMBOL(xsReloadAllSubmit)
	XS_API_SYMBOL(xsReloadQuery)
	XS_API_SYMBOL(xsTimerAfter)
	XS_API_SYMBOL(xsTimerCancel)
	XS_API_SYMBOL(xsAppPath)
	XS_API_SYMBOL(xsCreateTCC)
	XS_API_SYMBOL(xsDestroyTCC)
	XS_API_SYMBOL(xsSwapTake)
};
#undef XS_API_SYMBOL

/* —— libtcc + VFS 符号（嵌套编译：应用层插件系统的地基）—— */
static const XS_TccSymbol g_XS_TccSymbols[] = {
	{ "tcc_new",			(const void*)tcc_new },
	{ "tcc_delete",			(const void*)tcc_delete },
	{ "tcc_set_lib_path",		(const void*)tcc_set_lib_path },
	{ "tcc_set_error_func",		(const void*)tcc_set_error_func },
	{ "tcc_set_options",		(const void*)tcc_set_options },
	{ "tcc_add_include_path",	(const void*)tcc_add_include_path },
	{ "tcc_add_sysinclude_path",	(const void*)tcc_add_sysinclude_path },
	{ "tcc_define_symbol",		(const void*)tcc_define_symbol },
	{ "tcc_undefine_symbol",	(const void*)tcc_undefine_symbol },
	{ "tcc_add_library_path",	(const void*)tcc_add_library_path },
	{ "tcc_add_library",		(const void*)tcc_add_library },
	{ "tcc_add_file",		(const void*)tcc_add_file },
	{ "tcc_add_symbol",		(const void*)tcc_add_symbol },
	{ "tcc_set_output_type",	(const void*)tcc_set_output_type },
	{ "tcc_compile_string",		(const void*)tcc_compile_string },
	{ "tcc_output_file",		(const void*)tcc_output_file },
	{ "tcc_relocate",		(const void*)tcc_relocate },
	{ "tcc_get_symbol",		(const void*)tcc_get_symbol },
	{ "tcc_vfs_open",		(const void*)tcc_vfs_open },
	{ "tcc_vfs_close",		(const void*)tcc_vfs_close },
	{ "tcc_vfs_read",		(const void*)tcc_vfs_read },
	{ "tcc_vfs_lseek",		(const void*)tcc_vfs_lseek },
	{ "tcc_vfs_fopen",		(const void*)tcc_vfs_fopen },
	{ "tcc_vfs_fclose",		(const void*)tcc_vfs_fclose },
	{ "tcc_vfs_mount_memory",	(const void*)tcc_vfs_mount_memory },
	{ "tcc_vfs_unmount",		(const void*)tcc_vfs_unmount },
};

#ifdef XS_USE_SQLITE
/* sqlite3 符号（构建生成清单；仅 XS_USE_SQLITE 构建存在） */
#include "../../lib/sqlite3.h"
#define XS_SQLITE_SYMBOL(name)	{ #name, (const void*)(name) },
static const XS_TccSymbol g_XS_SqliteSymbols[] = {
#include "import_sqlite.inc"
};
#undef XS_SQLITE_SYMBOL
#endif

static void XS_TccAddSymbols(TCCState* pTcc, const XS_TccSymbol* pSymbols, size_t iCount)
{
	size_t i;

	for ( i = 0; i < iCount; i++ ) {
		(void)tcc_add_symbol(pTcc, pSymbols[i].sName, pSymbols[i].pValue);
	}
}

/* 创建就绪的 TCC 环境：全部资源来自内置 VFS（/xs 与 /tcc），
 * 运行时不依赖任何磁盘上的 TCC 环境（res/tcc 已在构建期打包进二进制） */
static TCCState* XS_TccCreate(void)
{
	TCCState* pTcc = tcc_new();

	if ( pTcc == NULL ) {
		return NULL;
	}
	tcc_set_error_func(pTcc, stderr, XS_TccErrorProc);
	tcc_set_output_type(pTcc, TCC_OUTPUT_MEMORY);

	/* TCC 运行时头：尖括号包含走 sysinclude，引号包含走 include */
#if defined(_WIN32) || defined(_WIN64)
	tcc_add_include_path(pTcc, "/tcc/include_win/winapi");
	tcc_add_sysinclude_path(pTcc, "/tcc/include_win/winapi");
	tcc_add_include_path(pTcc, "/tcc/include_win");
	tcc_add_sysinclude_path(pTcc, "/tcc/include_win");
#else
	tcc_add_include_path(pTcc, "/tcc/include_linux");
	tcc_add_sysinclude_path(pTcc, "/tcc/include_linux");
#endif
	tcc_add_include_path(pTcc, "/tcc/include");
	tcc_add_sysinclude_path(pTcc, "/tcc/include");
	tcc_add_library_path(pTcc, "/tcc/lib");

	/* SDK 头（xsbase.h / xrt_decl.h / libtcc.h），与二进制同版本 */
	tcc_add_include_path(pTcc, "/xs");
	tcc_add_sysinclude_path(pTcc, "/xs");

	/* Windows 导入库（tcc\lib 下的 .def 文件）：内存模式下 msvcrt 与 kernel32
	 * 自动挂载，其余需显式声明——这是脚本使用 进程/COM(Excel)/Socket/Shell
	 * 等 系统 API 的解析通道（与保留的 def 文件一一对应） */
#if defined(_WIN32) || defined(_WIN64)
	tcc_add_library(pTcc, "user32");
	tcc_add_library(pTcc, "gdi32");
	tcc_add_library(pTcc, "shell32");
	tcc_add_library(pTcc, "ole32");
	tcc_add_library(pTcc, "oleaut32");
	tcc_add_library(pTcc, "ws2_32");
	tcc_add_library(pTcc, "imm32");
#endif

	XS_TccAddSymbols(pTcc, g_XS_XrtSymbols, sizeof(g_XS_XrtSymbols) / sizeof(g_XS_XrtSymbols[0]));
	XS_TccAddSymbols(pTcc, g_XS_ApiSymbols, sizeof(g_XS_ApiSymbols) / sizeof(g_XS_ApiSymbols[0]));
	XS_TccAddSymbols(pTcc, g_XS_TccSymbols, sizeof(g_XS_TccSymbols) / sizeof(g_XS_TccSymbols[0]));
#ifdef XS_USE_SQLITE
	XS_TccAddSymbols(pTcc, g_XS_SqliteSymbols, sizeof(g_XS_SqliteSymbols) / sizeof(g_XS_SqliteSymbols[0]));
#endif
	return pTcc;
}

/* host 级额外目录（dev_inc / dev_lib，分号分隔，相对 appPath 解析）：
 * 头目录同时注册 include 与 sysinclude（尖括号/引号都可命中），真实磁盘路径 */
static bool XS_TccAddHostPaths(TCCState* pTcc, const char* sList, bool bLib)
{
	const char* pSeg = sList;

	while ( pSeg != NULL && *pSeg != '\0' ) {
		const char* pEnd = strchr(pSeg, ';');
		size_t iLen = (pEnd != NULL) ? (size_t)(pEnd - pSeg) : strlen(pSeg);
		char arrDir[1024];

		while ( iLen > 0 && *pSeg == ' ' ) { pSeg++; iLen--; }
		while ( iLen > 0 && pSeg[iLen - 1] == ' ' ) { iLen--; }
		if ( iLen >= sizeof(arrDir) - 1 ) return false;
		if ( iLen > 0 ) {
			str sAbs;
			bool bOk = true;

			memcpy(arrDir, pSeg, iLen);
			arrDir[iLen] = '\0';
			sAbs = xrtPathIsAbs(arrDir) ? xrtStrDup(arrDir) : xrtPathJoin(XS_AppPath(), arrDir);
			if ( sAbs == NULL ) return false;
			/* 缺失目录按文档静默跳过；一旦存在，只注册基于 appPath
			 * 解析的唯一绝对路径，不再回落到进程启动目录。 */
			if ( xrtDirExists(sAbs) ) {
				if ( bLib ) {
					bOk = tcc_add_library_path(pTcc, sAbs) >= 0;
				} else {
					bOk = tcc_add_include_path(pTcc, sAbs) >= 0 &&
					      tcc_add_sysinclude_path(pTcc, sAbs) >= 0;
				}
			}
			xrtFree(sAbs);
			if ( !bOk ) return false;
		}
		pSeg = (pEnd != NULL) ? pEnd + 1 : NULL;
	}
	return true;
}

/* 带 host 环境的 TCC 创建：基础环境 + dev_inc/dev_lib（脚本编译用） */
static TCCState* XS_TccCreateForHost(XS_HostInfo* pHost)
{
	TCCState* pTcc = XS_TccCreate();

	if ( pTcc == NULL ) {
		return NULL;
	}
	if ( pHost != NULL ) {
		if ( pHost->DevInc != NULL ) {
			if ( !XS_TccAddHostPaths(pTcc, pHost->DevInc, false) ) {
				tcc_delete(pTcc);
				return NULL;
			}
		}
		if ( pHost->DevLib != NULL ) {
			if ( !XS_TccAddHostPaths(pTcc, pHost->DevLib, true) ) {
				tcc_delete(pTcc);
				return NULL;
			}
		}
	}
	return pTcc;
}

#endif
