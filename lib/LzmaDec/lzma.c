#include <stdlib.h>
#include "LzmaDec.c"



static void *SzAlloc(ISzAllocPtr p, size_t size) { UNUSED_VAR(p); return malloc(size); }
static void SzFree(ISzAllocPtr p, void *address) { UNUSED_VAR(p); free(address); }
const ISzAlloc g_Alloc = { SzAlloc, SzFree };



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
