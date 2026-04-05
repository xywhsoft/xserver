#ifndef XS_SCRIPT_IMPORT_C_IMPORT_ZSTD_H
#define XS_SCRIPT_IMPORT_C_IMPORT_ZSTD_H



static inline void ImportZSTD(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	tcc_add_symbol(s, "ZSTD_versionNumber", ZSTD_versionNumber);
	tcc_add_symbol(s, "ZSTD_versionString", ZSTD_versionString);
	tcc_add_symbol(s, "ZSTD_compress", ZSTD_compress);
	tcc_add_symbol(s, "ZSTD_decompress", ZSTD_decompress);
	tcc_add_symbol(s, "ZSTD_getFrameContentSize", ZSTD_getFrameContentSize);
	tcc_add_symbol(s, "ZSTD_findFrameCompressedSize", ZSTD_findFrameCompressedSize);
	tcc_add_symbol(s, "ZSTD_compressBound", ZSTD_compressBound);
	tcc_add_symbol(s, "ZSTD_isError", ZSTD_isError);
	tcc_add_symbol(s, "ZSTD_getErrorCode", ZSTD_getErrorCode);
	tcc_add_symbol(s, "ZSTD_getErrorName", ZSTD_getErrorName);
	tcc_add_symbol(s, "ZSTD_minCLevel", ZSTD_minCLevel);
	tcc_add_symbol(s, "ZSTD_maxCLevel", ZSTD_maxCLevel);
	tcc_add_symbol(s, "ZSTD_defaultCLevel", ZSTD_defaultCLevel);
	tcc_add_symbol(s, "ZSTD_createCCtx", ZSTD_createCCtx);
	tcc_add_symbol(s, "ZSTD_freeCCtx", ZSTD_freeCCtx);
	tcc_add_symbol(s, "ZSTD_compressCCtx", ZSTD_compressCCtx);
	tcc_add_symbol(s, "ZSTD_createDCtx", ZSTD_createDCtx);
	tcc_add_symbol(s, "ZSTD_freeDCtx", ZSTD_freeDCtx);
	tcc_add_symbol(s, "ZSTD_decompressDCtx", ZSTD_decompressDCtx);
	tcc_add_symbol(s, "ZSTD_createCStream", ZSTD_createCStream);
	tcc_add_symbol(s, "ZSTD_freeCStream", ZSTD_freeCStream);
	tcc_add_symbol(s, "ZSTD_initCStream", ZSTD_initCStream);
	tcc_add_symbol(s, "ZSTD_compressStream", ZSTD_compressStream);
	tcc_add_symbol(s, "ZSTD_flushStream", ZSTD_flushStream);
	tcc_add_symbol(s, "ZSTD_endStream", ZSTD_endStream);
	tcc_add_symbol(s, "ZSTD_CStreamInSize", ZSTD_CStreamInSize);
	tcc_add_symbol(s, "ZSTD_CStreamOutSize", ZSTD_CStreamOutSize);
	tcc_add_symbol(s, "ZSTD_createDStream", ZSTD_createDStream);
	tcc_add_symbol(s, "ZSTD_freeDStream", ZSTD_freeDStream);
	tcc_add_symbol(s, "ZSTD_initDStream", ZSTD_initDStream);
	tcc_add_symbol(s, "ZSTD_decompressStream", ZSTD_decompressStream);
	tcc_add_symbol(s, "ZSTD_DStreamInSize", ZSTD_DStreamInSize);
	tcc_add_symbol(s, "ZSTD_DStreamOutSize", ZSTD_DStreamOutSize);
}

#endif
