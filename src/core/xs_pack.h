/*
	站点 VFS 打包器（xs pack 子命令，Phase 1）。

	把站点目录打成逐条 LZMA 的归档追加到 exe 尾部；基底 = 当前运行的
	xs 自身（若已打包先剥旧包）。四命令：pack / --list / --extract / --strip。
	格式与 docs/站点VFS设计.md 逐字段一致（常量复用 xs_vfs.h）。

	工具模式独占运行（main 先于一切初始化拦截，执行完退出）。
	argv 约定：argv[0] == "pack"，argv[1] 是位置参数（站点目录或目标 exe）。
*/
#ifndef XS_PACK_H
#define XS_PACK_H

#ifdef __cplusplus
extern "C" {
#endif

int XS_PackMain(int argc, char** argv);

#ifdef __cplusplus
}
#endif

/* ================================================================== */
#ifdef XS_PACK_IMPLEMENTATION

#include "lib/xrt.h"
#include "xs_vfs.h"                /* 格式常量（XS_VFS_IMPLEMENTATION 未定义） */

#include "tcc/LzmaEnc.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#endif

/* ------------------------------------------------------------------ */
/* 基础助手                                                            */
/* ------------------------------------------------------------------ */

static char* XS_PackStrDup(const char* s)
{
	size_t n = strlen(s) + 1;
	char* p = (char*)malloc(n);
	if ( p != NULL ) memcpy(p, s, n);
	return p;
}

static uint32_t s_XS_PackCrcTable[256];
static int s_XS_PackCrcReady = 0;

static uint32_t XS_PackCrc32(uint32_t uCrc, const void* pData, size_t iSize)
{
	const uint8_t* p = (const uint8_t*)pData;

	if ( !s_XS_PackCrcReady ) {
		for ( uint32_t i = 0; i < 256; i++ ) {
			uint32_t e = i;
			for ( int j = 0; j < 8; j++ )
				e = (e & 1) ? (0xEDB88320u ^ (e >> 1)) : (e >> 1);
			s_XS_PackCrcTable[i] = e;
		}
		s_XS_PackCrcReady = 1;
	}
	uCrc = ~uCrc;
	while ( iSize-- > 0 )
		uCrc = s_XS_PackCrcTable[(uCrc ^ *p++) & 0xFF] ^ (uCrc >> 8);
	return ~uCrc;
}

static void XS_PackWr16(uint8_t* p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void XS_PackWr32(uint8_t* p, uint32_t v)
{
	p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static void XS_PackWr64(uint8_t* p, uint64_t v)
{
	XS_PackWr32(p, (uint32_t)v);
	XS_PackWr32(p + 4, (uint32_t)(v >> 32));
}
static uint16_t XS_PackRd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t XS_PackRd32(const uint8_t* p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
		| ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t XS_PackRd64(const uint8_t* p)
{
	return (uint64_t)XS_PackRd32(p) | ((uint64_t)XS_PackRd32(p + 4) << 32);
}

static const uint8_t XS_PACK_TRAILER_MAGIC[8] = { 'X','S','V','P','A','C','K',0 };
static const uint8_t XS_PACK_HEADER_MAGIC[8]  = { 'X','S','V','F','H','D','R',0 };

/* LZMA 分配器（工具模式用 libc，不占 xrt 池） */
static void* XS_PackLzmaAlloc(ISzAllocPtr pAlloc, size_t size)
{
	(void)pAlloc;
	return size == 0 ? NULL : malloc(size);
}
static void XS_PackLzmaFree(ISzAllocPtr pAlloc, void* address)
{
	(void)pAlloc;
	free(address);
}
static const ISzAlloc s_XS_PackLzmaAlloc = { XS_PackLzmaAlloc, XS_PackLzmaFree };

/* 逐条 LZMA：返回 malloc 缓冲（5B props + 流），失败 NULL。
 * 调用形态与 tools/tcc_vfs_lzma_pack.c 一致（level 9、无结束标记）。 */
static uint8_t* XS_PackLzma(const uint8_t* pData, size_t iSize, size_t* pCompSize)
{
	size_t iCap = iSize + iSize / 3u + 65536u;
	uint8_t* pOut = (uint8_t*)malloc(iCap);
	CLzmaEncProps tProps;
	SizeT uPropsSize = LZMA_PROPS_SIZE;
	SizeT uPacked;
	SRes eRes;

	if ( pOut == NULL ) return NULL;
	LzmaEncProps_Init(&tProps);
	tProps.level = 9;
	tProps.writeEndMark = 0;
	tProps.numThreads = 1;
	tProps.reduceSize = (UInt64)iSize;
	uPacked = (SizeT)(iCap - LZMA_PROPS_SIZE);
	eRes = LzmaEncode(pOut + LZMA_PROPS_SIZE, &uPacked,
	                  pData, (SizeT)iSize, &tProps,
	                  pOut, &uPropsSize, 0, NULL,
	                  &s_XS_PackLzmaAlloc, &s_XS_PackLzmaAlloc);
	if ( eRes != SZ_OK || uPropsSize != LZMA_PROPS_SIZE ) {
		free(pOut);
		return NULL;
	}
	*pCompSize = (size_t)uPacked + LZMA_PROPS_SIZE;
	return pOut;
}

/* LZMA 解码（extract 用；一次全解，镜像运行时调用形态） */
static uint8_t* XS_PackUnlzma(const uint8_t* pSrc, size_t iSrcSize, uint64_t uOrig)
{
	uint8_t* pOut;
	SizeT uOut, uIn;
	ELzmaStatus eSt;

	if ( uOrig == 0 ) return NULL;
	pOut = (uint8_t*)malloc((size_t)uOrig);
	if ( pOut == NULL ) return NULL;
	uOut = (SizeT)uOrig;
	uIn = (SizeT)iSrcSize - LZMA_PROPS_SIZE;
	if ( LzmaDecode(pOut, &uOut, pSrc + LZMA_PROPS_SIZE, &uIn,
	        pSrc, LZMA_PROPS_SIZE, LZMA_FINISH_END, &eSt,
	        &s_XS_PackLzmaAlloc) != SZ_OK || uOut != (SizeT)uOrig ) {
		free(pOut);
		return NULL;
	}
	return pOut;
}

/* 文件整体读写（工具模式走 libc FILE*，路径全 ASCII/UTF-8 场景够用；
 * 自身 exe 路径来自系统 API，输出路径来自 argv——与 xrtOpen 等价的
 * UTF-8 处理依赖宿主平台 fopen 行为，Windows 10+ 默认 UTF-8 代码页场景成立；
 * 非 ASCII 输出路径在旧系统上可能失败，文档建议输出路径用 ASCII） */
static uint8_t* XS_PackReadWhole(const char* sPath, size_t* pSize)
{
	FILE* f = fopen(sPath, "rb");
	uint8_t* pData;
	long n;

	if ( f == NULL ) return NULL;
	if ( fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0 ) {
		fclose(f);
		return NULL;
	}
	pData = (uint8_t*)malloc((size_t)n + 1);
	if ( pData == NULL || (n > 0 && fread(pData, 1, (size_t)n, f) != (size_t)n) ) {
		free(pData);
		fclose(f);
		return NULL;
	}
	fclose(f);
	*pSize = (size_t)n;
	return pData;
}

static bool XS_PackWriteWhole(const char* sPath, const uint8_t* pData, size_t iSize)
{
	FILE* f = fopen(sPath, "wb");
	bool bOk;

	if ( f == NULL ) return false;
	bOk = iSize == 0 || fwrite(pData, 1, iSize, f) == iSize;
	if ( fclose(f) != 0 ) bOk = false;
	return bOk;
}

static void XS_PackMkDirs(const char* sFilePath, size_t iSkip)
{
	char aDir[4096];

	if ( strlen(sFilePath) >= sizeof(aDir) ) return;
	strcpy(aDir, sFilePath);
	for ( char* p = aDir + iSkip; *p; p++ ) {
		if ( *p == '/' ) {
			*p = 0;
			xrtDirCreate(aDir);
			*p = '/';
		}
	}
}

/* ------------------------------------------------------------------ */
/* 站点收集（xrt dir 遍历 + XDIR_STAT，递归，收集后字典序排序）          */
/* ------------------------------------------------------------------ */

typedef struct XS_PackFile {
	char* sRel;
	char* sAbs;
} XS_PackFile;

typedef struct XS_PackList {
	XS_PackFile* pFiles;
	uint32_t nCount, nCap;
	char sError[256];
} XS_PackList;

static bool XS_PackListAdd(XS_PackList* pList, const char* sRel, const char* sAbs)
{
	if ( pList->nCount == pList->nCap ) {
		uint32_t nNew = pList->nCap == 0 ? 64 : pList->nCap * 2;
		XS_PackFile* pNew = (XS_PackFile*)realloc(
			pList->pFiles, (size_t)nNew * sizeof(XS_PackFile));
		if ( pNew == NULL ) return false;
		pList->pFiles = pNew;
		pList->nCap = nNew;
	}
	pList->pFiles[pList->nCount].sRel = XS_PackStrDup(sRel);
	pList->pFiles[pList->nCount].sAbs = XS_PackStrDup(sAbs);
	if ( pList->pFiles[pList->nCount].sRel == NULL ||
	     pList->pFiles[pList->nCount].sAbs == NULL ) return false;
	pList->nCount++;
	return true;
}

static void XS_PackListUnit(XS_PackList* pList)
{
	if ( pList->pFiles != NULL ) {
		for ( uint32_t i = 0; i < pList->nCount; i++ ) {
			free(pList->pFiles[i].sRel);
			free(pList->pFiles[i].sAbs);
		}
		free(pList->pFiles);
	}
	memset(pList, 0, sizeof(*pList));
}

static bool XS_PackWalk(XS_PackList* pList, const char* sAbsDir, const char* sRel)
{
	xdir hDir = xrtDirOpen(sAbsDir, XDIR_STAT);
	xdirentry tEntry;

	if ( hDir == NULL ) {
		snprintf(pList->sError, sizeof(pList->sError),
			"cannot open directory: %s", sAbsDir);
		return false;
	}
	while ( xrtDirNext(hDir, &tEntry) == XDIR_NEXT_ITEM ) {
		char aName[512];
		char* sAbs;
		char* sRel2;
		size_t nName = tEntry.Name.Size;

		if ( nName == 0 || nName >= sizeof(aName) ) continue;
		memcpy(aName, tEntry.Name.Data, nName);
		aName[nName] = 0;
		if ( strcmp(aName, ".") == 0 || strcmp(aName, "..") == 0 ) continue;
		if ( strcmp(aName, ".DS_Store") == 0 || strcmp(aName, "Thumbs.db") == 0 )
			continue;

		sAbs = (char*)malloc(strlen(sAbsDir) + nName + 2);
		sRel2 = (char*)malloc(strlen(sRel) + nName + 2);
		if ( sAbs == NULL || sRel2 == NULL ) {
			free(sAbs); free(sRel2);
			snprintf(pList->sError, sizeof(pList->sError), "out of memory");
			xrtDirClose(hDir);
			return false;
		}
		sprintf(sAbs, "%s/%s", sAbsDir, aName);
		if ( sRel[0] == 0 ) strcpy(sRel2, aName);
		else sprintf(sRel2, "%s/%s", sRel, aName);

		if ( tEntry.Info.Type == XFILE_TYPE_DIRECTORY ) {
			bool bOk = XS_PackWalk(pList, sAbs, sRel2);
			free(sAbs); free(sRel2);
			if ( !bOk ) { xrtDirClose(hDir); return false; }
		}
		else if ( tEntry.Info.Type == XFILE_TYPE_FILE ) {
			bool bOk = XS_PackListAdd(pList, sRel2, sAbs);
			free(sAbs); free(sRel2);
			if ( !bOk ) {
				snprintf(pList->sError, sizeof(pList->sError), "out of memory");
				xrtDirClose(hDir);
				return false;
			}
		}
		else {
			free(sAbs); free(sRel2);   /* 链接/socket 等跳过 */
		}
	}
	xrtDirClose(hDir);
	return true;
}

static int XS_PackFileCmp(const void* pA, const void* pB)
{
	return strcmp(((const XS_PackFile*)pA)->sRel, ((const XS_PackFile*)pB)->sRel);
}

/* ------------------------------------------------------------------ */
/* 归档解析（list/extract/strip 共用；也用于 pack 的剥旧检测）           */
/* ------------------------------------------------------------------ */

typedef struct XS_PackEntryInfo {
	char* sPath;
	uint8_t Method;
	uint64_t DataOffset, OrigSize;
	uint32_t CompSize, OrigCrc32;
} XS_PackEntryInfo;

typedef struct XS_PackParsed {
	uint8_t* pWhole;
	size_t iWholeSize;
	uint64_t uArchiveStart;    /* 归档头绝对偏移（= 原始基底长度） */
	uint32_t nCount;
	XS_PackEntryInfo* pEntries;
} XS_PackParsed;

static void XS_PackParsedUnit(XS_PackParsed* p)
{
	if ( p->pEntries != NULL ) {
		for ( uint32_t i = 0; i < p->nCount; i++ ) free(p->pEntries[i].sPath);
		free(p->pEntries);
	}
	free(p->pWhole);
	memset(p, 0, sizeof(*p));
}

/* bRequireArchive=false：无包不算错（uArchiveStart=文件全长，entries=0）
 * bRequireArchive=true ：无包/损坏报错（list/extract/strip 用） */
static bool XS_PackParse(const char* sExe, bool bRequireArchive,
                         XS_PackParsed* pOut, char* sErr, size_t iErrCap)
{
	uint8_t aTrailer[XS_VFS_TRAILER_SIZE], aHeader[XS_VFS_HEADER_SIZE];
	uint64_t uHeaderOff, uIndexOff, uIndexLen;
	uint32_t uIndexCrc;
	uint8_t* pIndex;
	uint32_t i;

	memset(pOut, 0, sizeof(*pOut));
	pOut->pWhole = XS_PackReadWhole(sExe, &pOut->iWholeSize);
	if ( pOut->pWhole == NULL ) {
		snprintf(sErr, iErrCap, "cannot read: %s", sExe);
		return false;
	}
	if ( pOut->iWholeSize < XS_VFS_TRAILER_SIZE + XS_VFS_HEADER_SIZE ) goto nopack;
	memcpy(aTrailer, pOut->pWhole + pOut->iWholeSize - XS_VFS_TRAILER_SIZE,
	       sizeof(aTrailer));
	if ( memcmp(aTrailer, XS_PACK_TRAILER_MAGIC, 8) != 0 ) goto nopack;
	/* 归档头在尾标指向的归档起点（[base][头][数据][索引][尾标]） */
	uHeaderOff = XS_PackRd64(aTrailer + 8);
	if ( uHeaderOff == 0 ||
	     uHeaderOff + XS_VFS_HEADER_SIZE + XS_VFS_TRAILER_SIZE
	       > (uint64_t)pOut->iWholeSize ) goto nopack;
	memcpy(aHeader, pOut->pWhole + uHeaderOff, sizeof(aHeader));
	{
		uint8_t aBuf[16 + XS_VFS_HEADER_SIZE];
		memcpy(aBuf, aTrailer, 16);
		memcpy(aBuf + 16, aHeader, XS_VFS_HEADER_SIZE);
		if ( memcmp(aHeader, XS_PACK_HEADER_MAGIC, 8) != 0 ||
		     XS_PackRd16(aHeader + 8) != 1 ||
		     XS_PackCrc32(0, aBuf, sizeof(aBuf)) != XS_PackRd32(aTrailer + 16) )
			goto nopack;                                        /* 损坏当无包 */
	}
	pOut->uArchiveStart = uHeaderOff;
	pOut->nCount = XS_PackRd32(aHeader + 12);
	{
		uint64_t uArchiveSize = XS_PackRd64(aHeader + 32);
		uIndexOff = XS_PackRd64(aHeader + 24);
		if ( pOut->uArchiveStart != uHeaderOff ||
		     uHeaderOff + uArchiveSize != (uint64_t)pOut->iWholeSize ) {
			snprintf(sErr, iErrCap, "archive size mismatch (corrupt?)");
			XS_PackParsedUnit(pOut);
			return false;
		}
		uIndexLen = pOut->iWholeSize - XS_VFS_TRAILER_SIZE
			- (size_t)(uHeaderOff + uIndexOff);
	}
	if ( pOut->nCount == 0 || pOut->nCount > XS_VFS_MAX_ENTRIES ||
	     uIndexLen == 0 || uIndexLen > (64u << 20) ) {
		snprintf(sErr, iErrCap, "index sanity failed");
		XS_PackParsedUnit(pOut);
		return false;
	}
	pIndex = pOut->pWhole + uHeaderOff + uIndexOff;
	uIndexCrc = XS_PackRd32(aHeader + 40);
	if ( XS_PackCrc32(0, pIndex, (size_t)uIndexLen) != uIndexCrc ) {
		snprintf(sErr, iErrCap, "index CRC mismatch");
		XS_PackParsedUnit(pOut);
		return false;
	}
	pOut->pEntries = (XS_PackEntryInfo*)calloc(
		pOut->nCount, sizeof(XS_PackEntryInfo));
	if ( pOut->pEntries == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		XS_PackParsedUnit(pOut);
		return false;
	}
	{
		size_t iPos = 0;
		for ( i = 0; i < pOut->nCount; i++ ) {
			uint32_t nRel;
			if ( iPos + 2 > uIndexLen ) goto malformed;
			nRel = XS_PackRd16(pIndex + iPos);
			iPos += 2;
			if ( nRel == 0 || iPos + nRel + 26 > uIndexLen ) goto malformed;
			pOut->pEntries[i].sPath = (char*)malloc(nRel + 1);
			if ( pOut->pEntries[i].sPath == NULL ) {
				snprintf(sErr, iErrCap, "out of memory");
				XS_PackParsedUnit(pOut);
				return false;
			}
			memcpy(pOut->pEntries[i].sPath, pIndex + iPos, nRel);
			pOut->pEntries[i].sPath[nRel] = 0;
			iPos += nRel;
			pOut->pEntries[i].Method = pIndex[iPos];
			iPos += 2;                                           /* method + flags */
			pOut->pEntries[i].DataOffset = XS_PackRd64(pIndex + iPos); iPos += 8;
			pOut->pEntries[i].CompSize = XS_PackRd32(pIndex + iPos); iPos += 4;
			pOut->pEntries[i].OrigSize = XS_PackRd64(pIndex + iPos); iPos += 8;
			pOut->pEntries[i].OrigCrc32 = XS_PackRd32(pIndex + iPos); iPos += 4;
		}
		if ( iPos != uIndexLen ) goto malformed;
	}
	return true;

malformed:
	snprintf(sErr, iErrCap, "index malformed");
	XS_PackParsedUnit(pOut);
	return false;

nopack:
	if ( bRequireArchive ) {
		snprintf(sErr, iErrCap, "no app pack appended");
		XS_PackParsedUnit(pOut);
		return false;
	}
	pOut->uArchiveStart = pOut->iWholeSize;                    /* 无包 = 基底全长 */
	return true;
}

/* ------------------------------------------------------------------ */
/* 自身 exe 路径                                                       */
/* ------------------------------------------------------------------ */

static const char* XS_PackSelfPath(void)
{
	static char sPath[4096];
	static bool bInit = false;

	if ( !bInit ) {
#if defined(_WIN32) || defined(_WIN64)
		wchar_t aWide[4096];
		DWORD n = GetModuleFileNameW(NULL, aWide,
			(DWORD)(sizeof(aWide) / sizeof(aWide[0])));
		int iLen;
		if ( n == 0 || n >= sizeof(aWide) / sizeof(aWide[0]) ) { sPath[0] = 0; return sPath; }
		iLen = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			aWide, (int)n, NULL, 0, NULL, NULL);
		if ( iLen <= 0 || (size_t)iLen >= sizeof(sPath) ) { sPath[0] = 0; return sPath; }
		WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			aWide, (int)n, sPath, iLen, NULL, NULL);
		sPath[iLen] = 0;
#else
		ssize_t n = readlink("/proc/self/exe", sPath, sizeof(sPath) - 1);
		if ( n <= 0 ) { sPath[0] = 0; return sPath; }
		sPath[n] = 0;
#endif
		bInit = true;
	}
	return sPath;
}

/* ------------------------------------------------------------------ */
/* pack                                                                */
/* ------------------------------------------------------------------ */

static int XS_PackDoPack(const char* sSiteDir, const char* sOut)
{
	const char* sSelf = XS_PackSelfPath();
	XS_PackList tList;
	XS_PackParsed tBase;
	char sErr[256];
	uint8_t* pIndex = NULL;
	size_t iIndexCap, iIndexLen = 0;
	uint8_t aHeader[XS_VFS_HEADER_SIZE];
	uint8_t aTrailer[XS_VFS_TRAILER_SIZE];
	uint64_t uDataCursor = 0;
	uint32_t nLzma = 0, nStore = 0;
	uint32_t i;
	FILE* fOut = NULL;
	FILE* fData = NULL;
	int iExit = 1;

	memset(&tList, 0, sizeof(tList));
	if ( sSelf == NULL || sSelf[0] == 0 ) {
		printf("[pack] cannot locate self executable\n");
		return 1;
	}
	/* 站点须含 xs.json */
	{
		char sCfg[4096];
		xfile h;
		snprintf(sCfg, sizeof(sCfg), "%s/xs.json", sSiteDir);
		h = xrtOpen(sCfg, XFILE_READ);
		if ( h == NULL ) {
			printf("[pack] site directory must contain xs.json: %s\n", sSiteDir);
			return 1;
		}
		xrtClose(h);
	}
	/* 基底 = 自身（剥旧） */
	if ( !XS_PackParse(sSelf, false, &tBase, sErr, sizeof(sErr)) ) {
		printf("[pack] %s\n", sErr);
		return 1;
	}
	if ( tBase.uArchiveStart != tBase.iWholeSize )
		printf("[pack] base already packed, stripping previous archive\n");
	if ( tBase.iWholeSize == 0 ) {
		printf("[pack] empty base exe\n");
		XS_PackParsedUnit(&tBase);
		return 1;
	}

	if ( !XS_PackWalk(&tList, sSiteDir, "") ) {
		printf("[pack] %s\n", tList.sError[0] ? tList.sError : "walk failed");
		goto done;
	}
	if ( tList.nCount == 0 ) {
		printf("[pack] site directory is empty\n");
		goto done;
	}
	qsort(tList.pFiles, tList.nCount, sizeof(XS_PackFile), XS_PackFileCmp);

	iIndexCap = 64;
	for ( i = 0; i < tList.nCount; i++ )
		iIndexCap += 40 + strlen(tList.pFiles[i].sRel);
	pIndex = (uint8_t*)malloc(iIndexCap);
	if ( pIndex == NULL ) { printf("[pack] out of memory\n"); goto done; }

	printf("[pack] base %llu bytes + %u files, compressing...\n",
		(unsigned long long)tBase.uArchiveStart, (unsigned)tList.nCount);
	/* 布局：[base][归档头 64B][数据区][索引区][尾标 24B]——头在数据前，
	 * dataOffset=64 直接头后；数据区先落在临时文件再拼装 */
	fData = tmpfile();
	if ( fData == NULL ) { printf("[pack] cannot create temp file\n"); goto done; }

	for ( i = 0; i < tList.nCount; i++ ) {
		XS_PackFile* pF = &tList.pFiles[i];
		size_t iOrig = 0, iComp = 0;
		uint8_t* pOrig = XS_PackReadWhole(pF->sAbs, &iOrig);
		uint8_t* pComp = NULL;
		uint8_t uMethod = 0;
		uint32_t uCrc;

		if ( pOrig == NULL ) {
			printf("[pack] cannot read: %s\n", pF->sAbs);
			goto done;
		}
		uCrc = XS_PackCrc32(0, pOrig, iOrig);
		if ( iOrig > 64 ) {
			size_t iC = 0;
			uint8_t* pTry = XS_PackLzma(pOrig, iOrig, &iC);
			if ( pTry != NULL && iC < iOrig ) {
				pComp = pTry;
				iComp = iC;
				uMethod = 1;
			}
			else {
				free(pTry);
			}
		}
		if ( uMethod == 0 ) { iComp = iOrig; nStore++; }
		else nLzma++;
		{
			const uint8_t* pWrite = uMethod == 1 ? pComp : pOrig;
			if ( fwrite(pWrite, 1, iComp, fData) != iComp ) {
				printf("[pack] write data failed: %s\n", pF->sRel);
				free(pOrig); free(pComp);
				goto done;
			}
		}
		{
			size_t nRel = strlen(pF->sRel);
			XS_PackWr16(pIndex + iIndexLen, (uint16_t)nRel);
			iIndexLen += 2;
			memcpy(pIndex + iIndexLen, pF->sRel, nRel);
			iIndexLen += nRel;
			pIndex[iIndexLen++] = uMethod;
			pIndex[iIndexLen++] = 0;
			/* 条目偏移相对归档头（头 64B 之后是数据区） */
			XS_PackWr64(pIndex + iIndexLen,
				XS_VFS_HEADER_SIZE + uDataCursor); iIndexLen += 8;
			XS_PackWr32(pIndex + iIndexLen, (uint32_t)iComp); iIndexLen += 4;
			XS_PackWr64(pIndex + iIndexLen, (uint64_t)iOrig); iIndexLen += 8;
			XS_PackWr32(pIndex + iIndexLen, uCrc); iIndexLen += 4;
		}
		uDataCursor += iComp;
		free(pOrig);
		free(pComp);
		if ( (i & 31) == 31 ) {
			printf("\r[pack] %u/%u", i + 1, tList.nCount);
			fflush(stdout);
		}
	}
	printf("\r[pack] %u/%u compressed\n", tList.nCount, tList.nCount);

	{
		uint32_t uIndexCrc = XS_PackCrc32(0, pIndex, iIndexLen);
		uint64_t uArchiveSize = XS_VFS_HEADER_SIZE + uDataCursor
			+ iIndexLen + XS_VFS_TRAILER_SIZE;
		uint64_t uHeaderAbs = tBase.uArchiveStart;

		fOut = fopen(sOut, "wb");
		if ( fOut == NULL ) { printf("[pack] cannot write: %s\n", sOut); goto done; }
		if ( fwrite(tBase.pWhole, 1, (size_t)tBase.uArchiveStart, fOut)
		     != (size_t)tBase.uArchiveStart ) {
			printf("[pack] write base failed\n");
			goto done;
		}
		/* 归档头 */
		memcpy(aHeader, XS_PACK_HEADER_MAGIC, 8);
		XS_PackWr16(aHeader + 8, 1);
		XS_PackWr16(aHeader + 10, XS_VFS_HEADER_SIZE);
		XS_PackWr32(aHeader + 12, tList.nCount);
		XS_PackWr64(aHeader + 16, XS_VFS_HEADER_SIZE);          /* dataOffset */
		XS_PackWr64(aHeader + 24, XS_VFS_HEADER_SIZE + uDataCursor);
		XS_PackWr64(aHeader + 32, uArchiveSize);
		XS_PackWr32(aHeader + 40, uIndexCrc);
		XS_PackWr32(aHeader + 44, 0);
		XS_PackWr64(aHeader + 48, 0);
		XS_PackWr64(aHeader + 56, 0);
		if ( fwrite(aHeader, 1, sizeof(aHeader), fOut) != sizeof(aHeader) ) {
			printf("[pack] write header failed\n");
			goto done;
		}
		/* 数据区（临时文件回放） */
		{
			static unsigned char aCopy[65536];
			size_t n;
			rewind(fData);
			while ( (n = fread(aCopy, 1, sizeof(aCopy), fData)) > 0 ) {
				if ( fwrite(aCopy, 1, n, fOut) != n ) {
					printf("[pack] copy data failed\n");
					goto done;
				}
			}
		}
		/* 索引区 + 尾标 */
		if ( fwrite(pIndex, 1, iIndexLen, fOut) != iIndexLen ) {
			printf("[pack] write index failed\n");
			goto done;
		}
		memcpy(aTrailer, XS_PACK_TRAILER_MAGIC, 8);
		XS_PackWr64(aTrailer + 8, uHeaderAbs);
		{
			uint8_t aBuf[16 + XS_VFS_HEADER_SIZE];
			memcpy(aBuf, aTrailer, 16);
			memcpy(aBuf + 16, aHeader, XS_VFS_HEADER_SIZE);
			XS_PackWr32(aTrailer + 16, XS_PackCrc32(0, aBuf, sizeof(aBuf)));
		}
		XS_PackWr32(aTrailer + 20, 0);
		if ( fwrite(aTrailer, 1, sizeof(aTrailer), fOut) != sizeof(aTrailer) ) {
			printf("[pack] write trailer failed\n");
			goto done;
		}
	}
	if ( fclose(fOut) != 0 ) {
		fOut = NULL;
		printf("[pack] close failed\n");
		goto done;
	}
	fOut = NULL;
	printf("[pack] %s: %u entries (lzma %u, store %u), archive %llu bytes\n",
	       sOut, (unsigned)tList.nCount, (unsigned)nLzma, (unsigned)nStore,
	       (unsigned long long)(uDataCursor + iIndexLen
	           + XS_VFS_HEADER_SIZE + XS_VFS_TRAILER_SIZE));
	iExit = 0;

done:
	if ( fOut != NULL ) fclose(fOut);
	if ( fData != NULL ) fclose(fData);
	if ( iExit != 0 ) remove(sOut);
	free(pIndex);
	XS_PackListUnit(&tList);
	XS_PackParsedUnit(&tBase);
	return iExit;
}

/* ------------------------------------------------------------------ */
/* list / extract / strip                                              */
/* ------------------------------------------------------------------ */

static int XS_PackDoList(const char* sExe)
{
	XS_PackParsed t;
	char sErr[256];
	uint64_t uComp = 0, uOrig = 0;
	uint32_t nLzma = 0, nStore = 0;

	if ( !XS_PackParse(sExe, true, &t, sErr, sizeof(sErr)) ) {
		printf("[pack] %s\n", sErr);
		return 1;
	}
	printf("%-6s %10s %12s %12s %s\n", "method", "crc32", "orig", "comp", "path");
	for ( uint32_t i = 0; i < t.nCount; i++ ) {
		uint64_t uC = t.pEntries[i].Method == 1
			? t.pEntries[i].CompSize : t.pEntries[i].OrigSize;
		printf("%-6s %08x %12llu %12llu %s\n",
		       t.pEntries[i].Method == 1 ? "lzma" : "store",
		       t.pEntries[i].OrigCrc32,
		       (unsigned long long)t.pEntries[i].OrigSize,
		       (unsigned long long)uC, t.pEntries[i].sPath);
		uOrig += t.pEntries[i].OrigSize;
		uComp += uC;
		if ( t.pEntries[i].Method == 1 ) nLzma++; else nStore++;
	}
	printf("-- %u entries (lzma %u, store %u), %llu -> %llu bytes\n",
	       (unsigned)t.nCount, (unsigned)nLzma, (unsigned)nStore,
	       (unsigned long long)uOrig, (unsigned long long)uComp);
	XS_PackParsedUnit(&t);
	return 0;
}

static int XS_PackDoExtract(const char* sExe, const char* sDir)
{
	XS_PackParsed t;
	char sErr[256];
	int iExit = 1;

	if ( !XS_PackParse(sExe, true, &t, sErr, sizeof(sErr)) ) {
		printf("[pack] %s\n", sErr);
		return 1;
	}
	printf("[pack] extracting %u entries to %s\n", (unsigned)t.nCount, sDir);
	xrtDirCreate(sDir);
	for ( uint32_t i = 0; i < t.nCount; i++ ) {
		const uint8_t* pSrc = t.pWhole + t.uArchiveStart
			+ t.pEntries[i].DataOffset;
		uint8_t* pOutBuf;
		char sPath[4096];

		snprintf(sPath, sizeof(sPath), "%s/%s", sDir, t.pEntries[i].sPath);
		if ( t.pEntries[i].Method == 0 ) {
			pOutBuf = (uint8_t*)malloc((size_t)t.pEntries[i].OrigSize);
			if ( pOutBuf != NULL )
				memcpy(pOutBuf, pSrc, (size_t)t.pEntries[i].OrigSize);
		}
		else {
			pOutBuf = XS_PackUnlzma(pSrc, t.pEntries[i].CompSize,
				t.pEntries[i].OrigSize);
		}
		if ( pOutBuf == NULL ||
		     XS_PackCrc32(0, pOutBuf, (size_t)t.pEntries[i].OrigSize)
		       != t.pEntries[i].OrigCrc32 ) {
			printf("[pack] decode/CRC failed: %s\n", t.pEntries[i].sPath);
			free(pOutBuf);
			goto done;
		}
		XS_PackMkDirs(sPath, strlen(sDir) + 1);
		if ( !XS_PackWriteWhole(sPath, pOutBuf, (size_t)t.pEntries[i].OrigSize) ) {
			printf("[pack] write failed: %s\n", sPath);
			free(pOutBuf);
			goto done;
		}
		free(pOutBuf);
	}
	printf("[pack] extract ok\n");
	iExit = 0;
done:
	XS_PackParsedUnit(&t);
	return iExit;
}

static int XS_PackDoStrip(const char* sExe, const char* sOut)
{
	XS_PackParsed t;
	char sErr[256];
	int iExit = 1;

	if ( !XS_PackParse(sExe, true, &t, sErr, sizeof(sErr)) ) {
		printf("[pack] %s\n", sErr);
		return 1;
	}
	if ( !XS_PackWriteWhole(sOut, t.pWhole, (size_t)t.uArchiveStart) ) {
		printf("[pack] write failed: %s\n", sOut);
		goto done;
	}
	printf("[pack] stripped %llu bytes -> %s\n",
	       (unsigned long long)(t.iWholeSize - t.uArchiveStart), sOut);
	iExit = 0;
done:
	XS_PackParsedUnit(&t);
	return iExit;
}

/* ------------------------------------------------------------------ */
/* 入口：argv[0] == "pack"                                             */
/* ------------------------------------------------------------------ */

static int XS_PackUsage(void)
{
	printf(
		"usage:\n"
		"  xs pack <site-dir> -o <out.exe>       pack site onto xs (base = self)\n"
		"  xs pack --list <exe>                  list archive entries\n"
		"  xs pack --extract <exe> -d <dir>      extract archive\n"
		"  xs pack --strip <exe> -o <orig.exe>   restore unpacked binary\n");
	return 2;
}

int XS_PackMain(int argc, char** argv)
{
	const char* sOut = NULL;
	const char* sDir = NULL;
	const char* sPos = NULL;
	const char* sMode = NULL;

	if ( argc < 2 ) return XS_PackUsage();
	if ( strcmp(argv[1], "--list") == 0 || strcmp(argv[1], "--extract") == 0 ||
	     strcmp(argv[1], "--strip") == 0 ) {
		sMode = argv[1];
	}
	else if ( argv[1][0] != '-' ) {
		sMode = "pack";
		sPos = argv[1];                                         /* 站点目录 */
	}
	else {
		return XS_PackUsage();
	}
	for ( int i = 2; i < argc; i++ ) {
		if ( strcmp(argv[i], "-o") == 0 && i + 1 < argc && sOut == NULL )
			sOut = argv[++i];
		else if ( strcmp(argv[i], "-d") == 0 && i + 1 < argc && sDir == NULL )
			sDir = argv[++i];
		else if ( argv[i][0] != '-' && sPos == NULL )
			sPos = argv[i];                                     /* 目标 exe */
		else
			return XS_PackUsage();
	}

	if ( strcmp(sMode, "pack") == 0 ) {
		if ( sPos == NULL || sOut == NULL ) return XS_PackUsage();
		return XS_PackDoPack(sPos, sOut);
	}
	if ( sPos == NULL ) return XS_PackUsage();
	if ( strcmp(sMode, "--list") == 0 )
		return XS_PackDoList(sPos);
	if ( strcmp(sMode, "--extract") == 0 ) {
		if ( sDir == NULL ) return XS_PackUsage();
		return XS_PackDoExtract(sPos, sDir);
	}
	/* --strip */
	if ( sOut == NULL ) return XS_PackUsage();
	return XS_PackDoStrip(sPos, sOut);
}

#endif /* XS_PACK_IMPLEMENTATION */
#endif /* XS_PACK_H */
