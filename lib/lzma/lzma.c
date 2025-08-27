#include "LzmaDec.c"
#include "LzmaEnc.c"
#include "LzFind.c"
#include "LzFindMt.c"
#include "Threads.c"
#include "LzFindOpt.c"



static void *SzAlloc(ISzAllocPtr p, size_t size) { UNUSED_VAR(p); return malloc(size); }
static void SzFree(ISzAllocPtr p, void *address) { UNUSED_VAR(p); free(address); }
const ISzAlloc g_Alloc = { SzAlloc, SzFree };



unsigned int Lzma_Compress(void* src, void* dst, int srclen, int dstlen, int lv)
{
	if ( dstlen > LZMA_PROPS_SIZE ) {
		memset(dst, 0, LZMA_PROPS_SIZE);
		size_t DestLen = dstlen - LZMA_PROPS_SIZE;
		size_t PropLen = LZMA_PROPS_SIZE;
		CLzmaEncProps props;
		LzmaEncProps_Init(&props);
		props.level = lv;
		props.dictSize = 0;
		props.lc = -1;
		props.lp = -1;
		props.pb = -1;
		props.fb = -1;
		props.numThreads = -1;
		if ( LzmaEncode(dst + LZMA_PROPS_SIZE, &DestLen, src, srclen, &props, dst, &PropLen, 0, NULL, &g_Alloc, &g_Alloc) == SZ_OK ) {
			return DestLen + LZMA_PROPS_SIZE;
		}
	}
	return 0;
}

unsigned int Lzma_Uncompress(void* src, void* dst, size_t srclen, size_t dstlen)
{
	if ( dstlen > LZMA_PROPS_SIZE ) {
		ELzmaStatus status;
		srclen-= LZMA_PROPS_SIZE;
		if ( LzmaDecode(dst, &dstlen, src + LZMA_PROPS_SIZE, &srclen, src, LZMA_PROPS_SIZE, LZMA_FINISH_ANY, &status, &g_Alloc) == SZ_OK ) {
			return dstlen;
		}
	}
	return 0;
}
