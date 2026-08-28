#ifndef XS_SCRIPT_TCC_HOST_H
#define XS_SCRIPT_TCC_HOST_H

#include "../../lib/libtcc.h"

static void XS_CreateTCC_ErrorHandler(void* pOpaque, const char* sMsg)
{
	(void)pOpaque;
	fprintf(stderr, "[TCC] %s\n", sMsg);
}

static inline void XS_TCCAddIncludePathEx(TCCState* s, const char* sBasePath, const char* sRelPath)
{
	char* sPath;
	
	if ( s == NULL || sRelPath == NULL || sRelPath[0] == '\0' ) {
		return;
	}
	
	if ( sBasePath && sBasePath[0] != '\0' ) {
		sPath = xrtPathJoin(2, (char*)sBasePath, (char*)sRelPath);
		if ( sPath ) {
			if ( xrtDirExists(sPath) ) {
				tcc_add_include_path(s, sPath);
			}
			xrtFree(sPath);
		}
	}
	
	tcc_add_include_path(s, sRelPath);
}

static inline void XS_TCCAddLibraryPathEx(TCCState* s, const char* sBasePath, const char* sRelPath)
{
	char* sPath;
	
	if ( s == NULL || sRelPath == NULL || sRelPath[0] == '\0' ) {
		return;
	}
	
	if ( sBasePath && sBasePath[0] != '\0' ) {
		sPath = xrtPathJoin(2, (char*)sBasePath, (char*)sRelPath);
		if ( sPath ) {
			if ( xrtDirExists(sPath) ) {
				tcc_add_library_path(s, sPath);
			}
			xrtFree(sPath);
		}
	}
	
	tcc_add_library_path(s, sRelPath);
}

static inline TCCState* XS_CreateTCC(const char* sWorkPath, void (*procImportAll)(TCCState*))
{
	TCCState* s = tcc_new();
	const char* sAppPath = xCore.AppPath;
	
	if ( s == NULL ) {
		return NULL;
	}
	
	tcc_set_error_func(s, stderr, XS_CreateTCC_ErrorHandler);

	#ifdef XRT_MEM_DEBUG
		tcc_define_symbol(s, "XRT_MEM_DEBUG", "1");
	#endif
	
	#if defined(_WIN32) || defined(_WIN64)
		XS_TCCAddIncludePathEx(s, sAppPath, "tcc/include_win/winapi");
		XS_TCCAddIncludePathEx(s, sAppPath, "tcc/include_win");
	#else
		XS_TCCAddIncludePathEx(s, sAppPath, "tcc/include_linux");
		tcc_add_include_path(s, "/usr/include");
		tcc_add_include_path(s, "/usr/include/i386-linux-gnu");
		tcc_add_include_path(s, "/usr/include/i386-linux-gnu/sys");
		tcc_add_include_path(s, "/usr/include/x86_64-linux-gnu");
		tcc_add_include_path(s, "/usr/include/x86_64-linux-gnu/sys");
		tcc_add_library_path(s, "/usr/lib");
		tcc_add_library_path(s, "/usr/lib/i386-linux-gnu");
		tcc_add_library_path(s, "/usr/lib/x86_64-linux-gnu");
	#endif
	
	XS_TCCAddIncludePathEx(s, sAppPath, "tcc/inc_xs");
	XS_TCCAddIncludePathEx(s, sAppPath, "tcc/include");
	XS_TCCAddLibraryPathEx(s, sAppPath, "tcc/lib");
	
	if ( sWorkPath && sWorkPath[0] != '\0' ) {
		tcc_add_include_path(s, sWorkPath);
		tcc_add_library_path(s, sWorkPath);
	}
	
	tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
	
	if ( procImportAll ) {
		procImportAll(s);
	}
	
	return s;
}

static inline void XS_DestroyTCC(TCCState* s)
{
	if ( s ) {
		tcc_delete(s);
	}
}

#endif
