#ifndef XS_SCRIPT_IMPORT_C_IMPORT_LIBTCC_H
#define XS_SCRIPT_IMPORT_C_IMPORT_LIBTCC_H



static inline void ImportLibTCC(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	// 添加函数 - LibTCC
	tcc_add_symbol(s, "tcc_set_realloc", tcc_set_realloc);
	tcc_add_symbol(s, "tcc_new", tcc_new);
	tcc_add_symbol(s, "tcc_delete", tcc_delete);
	tcc_add_symbol(s, "tcc_set_lib_path", tcc_set_lib_path);
	tcc_add_symbol(s, "tcc_set_error_func", tcc_set_error_func);
	tcc_add_symbol(s, "tcc_set_options", tcc_set_options);
	tcc_add_symbol(s, "tcc_add_include_path", tcc_add_include_path);
	tcc_add_symbol(s, "tcc_add_sysinclude_path", tcc_add_sysinclude_path);
	tcc_add_symbol(s, "tcc_define_symbol", tcc_define_symbol);
	tcc_add_symbol(s, "tcc_undefine_symbol", tcc_undefine_symbol);
	tcc_add_symbol(s, "tcc_add_file", tcc_add_file);
	tcc_add_symbol(s, "tcc_compile_string", tcc_compile_string);
	tcc_add_symbol(s, "tcc_set_output_type", tcc_set_output_type);
	tcc_add_symbol(s, "tcc_add_library_path", tcc_add_library_path);
	tcc_add_symbol(s, "tcc_add_library", tcc_add_library);
	tcc_add_symbol(s, "tcc_add_symbol", tcc_add_symbol);
	tcc_add_symbol(s, "tcc_output_file", tcc_output_file);
	tcc_add_symbol(s, "tcc_run", tcc_run);
	tcc_add_symbol(s, "tcc_relocate", tcc_relocate);
	tcc_add_symbol(s, "tcc_get_symbol", tcc_get_symbol);
	tcc_add_symbol(s, "tcc_list_symbols", tcc_list_symbols);
	tcc_add_symbol(s, "_tcc_setjmp", _tcc_setjmp);
	tcc_add_symbol(s, "tcc_set_backtrace_func", tcc_set_backtrace_func);
}



#endif
