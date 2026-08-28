#ifndef XS_SCRIPT_IMPORT_C_IMPORT_LZ4_H
#define XS_SCRIPT_IMPORT_C_IMPORT_LZ4_H



static inline void ImportLZ4(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	tcc_add_symbol(s, "LZ4_versionNumber", LZ4_versionNumber);
	tcc_add_symbol(s, "LZ4_versionString", LZ4_versionString);
	tcc_add_symbol(s, "LZ4_compressBound", LZ4_compressBound);
	tcc_add_symbol(s, "LZ4_compress_default", LZ4_compress_default);
	tcc_add_symbol(s, "LZ4_decompress_safe", LZ4_decompress_safe);
	tcc_add_symbol(s, "LZ4_compress_fast", LZ4_compress_fast);
	tcc_add_symbol(s, "LZ4_sizeofState", LZ4_sizeofState);
	tcc_add_symbol(s, "LZ4_compress_fast_extState", LZ4_compress_fast_extState);
	tcc_add_symbol(s, "LZ4_createStream", LZ4_createStream);
	tcc_add_symbol(s, "LZ4_freeStream", LZ4_freeStream);
	tcc_add_symbol(s, "LZ4_resetStream_fast", LZ4_resetStream_fast);
	tcc_add_symbol(s, "LZ4_loadDict", LZ4_loadDict);
	tcc_add_symbol(s, "LZ4_compress_fast_continue", LZ4_compress_fast_continue);
	tcc_add_symbol(s, "LZ4_saveDict", LZ4_saveDict);
	tcc_add_symbol(s, "LZ4_createStreamDecode", LZ4_createStreamDecode);
	tcc_add_symbol(s, "LZ4_freeStreamDecode", LZ4_freeStreamDecode);
	tcc_add_symbol(s, "LZ4_setStreamDecode", LZ4_setStreamDecode);
	tcc_add_symbol(s, "LZ4_decompress_safe_continue", LZ4_decompress_safe_continue);
	tcc_add_symbol(s, "LZ4_decompress_safe_usingDict", LZ4_decompress_safe_usingDict);
	tcc_add_symbol(s, "LZ4_compress_HC", LZ4_compress_HC);
	tcc_add_symbol(s, "LZ4_sizeofStateHC", LZ4_sizeofStateHC);
	tcc_add_symbol(s, "LZ4_compress_HC_extStateHC", LZ4_compress_HC_extStateHC);
}

#endif
