#ifndef XS_SCRIPT_IMPORT_C_IMPORT_LZMA_H
#define XS_SCRIPT_IMPORT_C_IMPORT_LZMA_H



static inline void ImportLZMA(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	tcc_add_symbol(s, "MyAlloc", MyAlloc);
	tcc_add_symbol(s, "MyFree", MyFree);
	tcc_add_symbol(s, "MyRealloc", MyRealloc);
	tcc_add_symbol(s, "g_Alloc", &g_Alloc);
	tcc_add_symbol(s, "g_BigAlloc", &g_BigAlloc);
	tcc_add_symbol(s, "g_AlignedAlloc", &g_AlignedAlloc);
	tcc_add_symbol(s, "LzmaEncProps_Init", LzmaEncProps_Init);
	tcc_add_symbol(s, "LzmaEncode", LzmaEncode);
	tcc_add_symbol(s, "LzmaProps_Decode", LzmaProps_Decode);
	tcc_add_symbol(s, "LzmaDecode", LzmaDecode);
	tcc_add_symbol(s, "Lzma2Decode", Lzma2Decode);
	tcc_add_symbol(s, "Lzma2EncProps_Init", Lzma2EncProps_Init);
	tcc_add_symbol(s, "Lzma2Enc_Create", Lzma2Enc_Create);
	tcc_add_symbol(s, "Lzma2Enc_Destroy", Lzma2Enc_Destroy);
	tcc_add_symbol(s, "Lzma2Enc_SetProps", Lzma2Enc_SetProps);
	tcc_add_symbol(s, "Lzma2Enc_SetDataSize", Lzma2Enc_SetDataSize);
	tcc_add_symbol(s, "Lzma2Enc_WriteProperties", Lzma2Enc_WriteProperties);
	tcc_add_symbol(s, "Lzma2Enc_Encode2", Lzma2Enc_Encode2);
}

#endif
