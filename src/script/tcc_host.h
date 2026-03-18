#ifndef XS_SCRIPT_TCC_HOST_H
#define XS_SCRIPT_TCC_HOST_H

#include "../../lib/libtcc.h"

static void XS_CreateTCC_ErrorHandler(void* pOpaque, const char* sMsg)
{
	(void)pOpaque;
	fprintf(stderr, "[TCC] %s\n", sMsg);
}

static inline TCCState* XS_CreateTCC(const char* sWorkPath, void (*procImportAll)(TCCState*))
{
	TCCState* s = tcc_new();
	
	if ( s == NULL ) {
		return NULL;
	}
	
	tcc_set_error_func(s, stderr, XS_CreateTCC_ErrorHandler);
	
	#if defined(_WIN32) || defined(_WIN64)
		tcc_add_include_path(s, "tcc/include_win/winapi");
		tcc_add_include_path(s, "tcc/include_win");
	#else
		tcc_add_include_path(s, "tcc/include_linux");
		tcc_add_include_path(s, "/usr/include");
		tcc_add_include_path(s, "/usr/include/i386-linux-gnu");
		tcc_add_include_path(s, "/usr/include/i386-linux-gnu/sys");
		tcc_add_include_path(s, "/usr/include/x86_64-linux-gnu");
		tcc_add_include_path(s, "/usr/include/x86_64-linux-gnu/sys");
		tcc_add_library_path(s, "/usr/lib");
		tcc_add_library_path(s, "/usr/lib/i386-linux-gnu");
		tcc_add_library_path(s, "/usr/lib/x86_64-linux-gnu");
	#endif
	
	tcc_add_include_path(s, "tcc/inc_xs");
	tcc_add_include_path(s, "tcc/include");
	tcc_add_library_path(s, "tcc/lib");
	
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
