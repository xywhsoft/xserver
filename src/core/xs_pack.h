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
#include "checked.h"
#include "limits.h"

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

	if ( uOrig == 0 || iSrcSize < LZMA_PROPS_SIZE ) return NULL;
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

/* UTF-8 paths and 64-bit sizes share the runtime file contract. Keep libc
 * allocation ownership, and reject oversized/changing inputs before publishing. */
static uint8_t* XS_PackReadWhole(const char* sPath, uint64_t uLimit, size_t* pSize)
{
	xfile hFile = xrtOpen(sPath, XFILE_READ);
	uint8_t* pData = NULL;
	uint8_t uExtra;
	uint64 uSize = 0, uFinalSize = 0;
	size_t iRead = 0;
	bool bOk = false;

	*pSize = 0;
	if ( hFile == NULL ) return NULL;
	if ( !xrtFileSize(hFile, &uSize) || uSize > uLimit || uSize > SIZE_MAX - 1u )
		goto done;
	pData = (uint8_t*)malloc((size_t)uSize + 1u);
	if ( pData == NULL ) goto done;
	if ( !xrtReadFull(hFile, pData, (size_t)uSize, &iRead) || iRead != (size_t)uSize ||
	     !xrtRead(hFile, &uExtra, 1u, &iRead) || iRead != 0 ||
	     !xrtFileSize(hFile, &uFinalSize) || uFinalSize != uSize )
		goto done;
	pData[(size_t)uSize] = 0;
	bOk = true;
done:
	if ( !xrtClose(hFile) ) bOk = false;
	if ( !bOk ) {
		free(pData);
		return NULL;
	}
	*pSize = (size_t)uSize;
	return pData;
}

static bool XS_PackWriteWhole(const char* sPath, const uint8_t* pData, size_t iSize)
{
	xbytesview tData = { pData, iSize };
	return xrtFileWriteAtomic(sPath, tData);
}

static bool XS_PackWriteFile(xfile hFile, const void* pData, size_t iSize)
{
	size_t iWritten = 0;
	return xrtWriteFull(hFile, pData, iSize, &iWritten);
}

/* Every lookup remains relative to an open root, including existing directories.
 * A temporary sibling is published only after all bytes have been written. */
static bool XS_PackExtractWrite(xroot pRoot, const char* sPath,
	const uint8_t* pData, size_t iSize)
{
	char aDir[XS_PATH_BYTES_LIMIT + 1u];
	char aTemp[XS_PATH_BYTES_LIMIT + 1u];
	static uint64 iSequence = 0;
	xfileoptions tOptions;
	xfile hFile = NULL;
	bool bOk;
	int iLength;
	if ( strlen(sPath) >= sizeof(aDir) ) return false;
	memcpy(aDir, sPath, strlen(sPath) + 1u);
	for ( char* p = aDir; *p; p++ ) {
		if ( *p == '/' ) {
			xroot pDirectory;
			*p = 0;
			pDirectory = xrtRootOpenIn(pRoot, aDir);
			if ( pDirectory == NULL ) {
				if ( !xrtRootDirCreate(pRoot, aDir, 0755) ) return false;
				pDirectory = xrtRootOpenIn(pRoot, aDir);
			}
			if ( pDirectory == NULL ) return false;
			(void)xrtRootClose(pDirectory);
			*p = '/';
		}
	}
	xrtFileOptionsInit(&tOptions);
	tOptions.Flags = XFILE_WRITE | XFILE_CREATE | XFILE_EXCLUSIVE | XFILE_NOFOLLOW;
	tOptions.Mode = 0644;
	for ( unsigned i = 0; i < 64 && hFile == NULL; i++ ) {
		iLength = snprintf(aTemp, sizeof(aTemp), "%s.xs-tmp-%llu", sPath,
			(unsigned long long)++iSequence);
		if ( iLength < 0 || (size_t)iLength >= sizeof(aTemp) ) return false;
		hFile = xrtRootFileOpen(pRoot, aTemp, &tOptions);
	}
	if ( hFile == NULL ) return false;
	bOk = XS_PackWriteFile(hFile, pData, iSize);
	if ( !xrtClose(hFile) ) bOk = false;
	if ( bOk ) bOk = xrtRootFileRename(pRoot, aTemp, sPath, true);
	if ( !bOk ) (void)xrtRootRemove(pRoot, aTemp);
	return bOk;
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
	if ( pList->nCount >= XS_VFS_MAX_ENTRIES ) return false;
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
	     pList->pFiles[pList->nCount].sAbs == NULL ) {
		free(pList->pFiles[pList->nCount].sRel);
		free(pList->pFiles[pList->nCount].sAbs);
		return false;
	}
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

static bool XS_PackWalk(XS_PackList* pList, const char* sAbsDir, const char* sRel, unsigned iDepth)
{
	if ( iDepth > 128u ) {
		snprintf(pList->sError, sizeof(pList->sError), "directory nesting over limit"); return false;
	}
	xdir hDir = xrtDirOpen(sAbsDir, XDIR_STAT);
	xdirentry tEntry;
	xdirnext eNext;

	if ( hDir == NULL ) {
		snprintf(pList->sError, sizeof(pList->sError),
			"cannot open directory: %s", sAbsDir);
		return false;
	}
	while ( (eNext = xrtDirNext(hDir, &tEntry)) == XDIR_NEXT_ITEM ) {
		char aName[XS_PATH_BYTES_LIMIT + 1u];
		char* sAbs;
		char* sRel2;
		size_t nName = tEntry.Name.Size;

		if ( nName == 0 ) continue;
		if ( nName >= sizeof(aName) ) {
			snprintf(pList->sError, sizeof(pList->sError), "entry name too long");
			xrtDirClose(hDir); return false;
		}
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
			bool bOk = XS_PackWalk(pList, sAbs, sRel2, iDepth + 1u);
			free(sAbs); free(sRel2);
			if ( !bOk ) { xrtDirClose(hDir); return false; }
		}
		else if ( tEntry.Info.Type == XFILE_TYPE_FILE ) {
			if ( tEntry.Info.Size > XS_VFS_MAX_ENTRY_ORIG_SIZE ) {
				snprintf(pList->sError, sizeof(pList->sError), "entry size over limit: %s", sRel2);
				free(sAbs); free(sRel2);
				xrtDirClose(hDir);
				return false;
			}
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
	if ( eNext != XDIR_NEXT_END ) {
		snprintf(pList->sError, sizeof(pList->sError), "cannot read directory: %s", sAbsDir);
		xrtDirClose(hDir);
		return false;
	}
	if ( !xrtDirClose(hDir) ) {
		snprintf(pList->sError, sizeof(pList->sError), "cannot close directory: %s", sAbsDir);
		return false;
	}
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
	uint64_t uDataOff;
	uint32_t uIndexCrc;
	uint8_t* pIndex;
	uint32_t i;

	memset(pOut, 0, sizeof(*pOut));
	pOut->pWhole = XS_PackReadWhole(sExe, SIZE_MAX - 1u, &pOut->iWholeSize);
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
	if ( uHeaderOff == 0 || !XS_U64Range(uHeaderOff,
	     XS_VFS_HEADER_SIZE + XS_VFS_TRAILER_SIZE, pOut->iWholeSize) ) goto nopack;
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
		uDataOff = XS_PackRd64(aHeader + 16);
		if ( pOut->uArchiveStart != uHeaderOff ||
		     uArchiveSize != (uint64_t)pOut->iWholeSize - uHeaderOff ) {
			snprintf(sErr, iErrCap, "archive size mismatch (corrupt?)");
			XS_PackParsedUnit(pOut);
			return false;
		}
		/* 布局校验与运行时装载器（xs_vfs.h XS_VfsLoad）一致：
		 * 数据区 [DataOffset, IndexOffset) 必须落在归档内且非空 */
		if ( uArchiveSize < XS_VFS_HEADER_SIZE + XS_VFS_TRAILER_SIZE ||
		     uDataOff < XS_VFS_HEADER_SIZE || uIndexOff <= uDataOff ||
		     uIndexOff >= uArchiveSize - XS_VFS_TRAILER_SIZE ) {
			snprintf(sErr, iErrCap, "archive layout invalid");
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
			if ( iPos + 2 > uIndexLen ) {
				snprintf(sErr, iErrCap, "index malformed");
				goto malformed;
			}
			nRel = XS_PackRd16(pIndex + iPos);
			iPos += 2;
			if ( nRel == 0 || iPos + nRel + 26 > uIndexLen ) {
				snprintf(sErr, iErrCap, "index malformed");
				goto malformed;
			}
			pOut->pEntries[i].sPath = (char*)malloc(nRel + 1);
			if ( pOut->pEntries[i].sPath == NULL ) {
				snprintf(sErr, iErrCap, "out of memory");
				XS_PackParsedUnit(pOut);
				return false;
			}
			memcpy(pOut->pEntries[i].sPath, pIndex + iPos, nRel);
			pOut->pEntries[i].sPath[nRel] = 0;
			/* 路径规范化校验（拒绝 ../、反斜杠、首/尾/重复斜杠等
			 * 逃逸形态），与运行时 XS_VfsLoad 相同：规范化结果须与
			 * 原文完全一致，否则 extract 会写到目标目录之外 */
			{
				uint32_t uNormLen = 0;
				char* sNorm = XS_VfsNormalizePath(
					pOut->pEntries[i].sPath, &uNormLen);

				if ( sNorm == NULL || uNormLen != nRel ||
				     strcmp(sNorm, pOut->pEntries[i].sPath) != 0 ) {
					xrtFree(sNorm);
					snprintf(sErr, iErrCap, "unsafe entry path: %s",
						pOut->pEntries[i].sPath);
					goto malformed;
				}
				xrtFree(sNorm);
			}
			iPos += nRel;
			pOut->pEntries[i].Method = pIndex[iPos];
			iPos += 2;                                           /* method + flags */
			pOut->pEntries[i].DataOffset = XS_PackRd64(pIndex + iPos); iPos += 8;
			pOut->pEntries[i].CompSize = XS_PackRd32(pIndex + iPos); iPos += 4;
			pOut->pEntries[i].OrigSize = XS_PackRd64(pIndex + iPos); iPos += 8;
			pOut->pEntries[i].OrigCrc32 = XS_PackRd32(pIndex + iPos); iPos += 4;
			/* 条目级校验与运行时装载器一致：方法号、原始大小上限、
			 * STORE 大小一致、数据须落在数据区内——CRC32 可被构造者
			 * 重算，不构成对抗边界，越界读/写必须在解析期拒绝 */
			if ( pOut->pEntries[i].Method > 1 ) {
				snprintf(sErr, iErrCap, "entry method invalid: %s",
					pOut->pEntries[i].sPath);
				goto malformed;
			}
			if ( pOut->pEntries[i].Method == 1 &&
			     (pOut->pEntries[i].CompSize < LZMA_PROPS_SIZE || pOut->pEntries[i].OrigSize == 0) ) {
				snprintf(sErr, iErrCap, "compressed entry invalid: %s", pOut->pEntries[i].sPath);
				goto malformed;
			}
			if ( pOut->pEntries[i].OrigSize > XS_VFS_MAX_ENTRY_ORIG_SIZE ) {
				snprintf(sErr, iErrCap, "entry size over limit: %s",
					pOut->pEntries[i].sPath);
				goto malformed;
			}
			if ( pOut->pEntries[i].Method == 0 &&
			     pOut->pEntries[i].CompSize != (uint32_t)pOut->pEntries[i].OrigSize ) {
				snprintf(sErr, iErrCap, "store size mismatch: %s",
					pOut->pEntries[i].sPath);
				goto malformed;
			}
			if ( pOut->pEntries[i].DataOffset < uDataOff ||
			     !XS_U64Range(pOut->pEntries[i].DataOffset,
				pOut->pEntries[i].CompSize, uIndexOff) ) {
				snprintf(sErr, iErrCap, "entry data out of range: %s",
					pOut->pEntries[i].sPath);
				goto malformed;
			}
			if ( i > 0 && strcmp(pOut->pEntries[i - 1].sPath, pOut->pEntries[i].sPath) >= 0 ) {
				snprintf(sErr, iErrCap, "entry paths not strictly sorted");
				goto malformed;
			}
		}
		if ( iPos != uIndexLen ) {
			snprintf(sErr, iErrCap, "index malformed");
			goto malformed;
		}
	}
	return true;

malformed:
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
	xfile fOut = NULL;
	FILE* fData = NULL;
	str sStage = NULL;
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
		int n = snprintf(sCfg, sizeof(sCfg), "%s/xs.json", sSiteDir);
		if ( n < 0 || (size_t)n >= sizeof(sCfg) ) {
			printf("[pack] site path too long\n");
			return 1;
		}
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

	if ( !XS_PackWalk(&tList, sSiteDir, "", 0) ) {
		printf("[pack] %s\n", tList.sError[0] ? tList.sError : "walk failed");
		goto done;
	}
	if ( tList.nCount == 0 ) {
		printf("[pack] site directory is empty\n");
		goto done;
	}
	qsort(tList.pFiles, tList.nCount, sizeof(XS_PackFile), XS_PackFileCmp);

	iIndexCap = 64;
	for ( i = 0; i < tList.nCount; i++ ) {
		size_t nRel = strlen(tList.pFiles[i].sRel);
		if ( nRel > UINT16_MAX || !XS_SizeAdd(iIndexCap, 40u + nRel, &iIndexCap) ||
		     iIndexCap > (64u << 20) ) {
			printf("[pack] index or entry path over limit\n"); goto done;
		}
	}
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
		uint8_t* pOrig = XS_PackReadWhole(pF->sAbs, XS_VFS_MAX_ENTRY_ORIG_SIZE, &iOrig);
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

		{
			str sDirectory = xrtPathParent(sOut);
			if ( sDirectory == NULL ) { printf("[pack] output directory allocation failed\n"); goto done; }
			fOut = xrtFileTemp(sDirectory[0] != '\0' ? sDirectory : ".", "xs-pack-", ".tmp", &sStage);
			xrtFree(sDirectory);
		}
		if ( fOut == NULL ) { printf("[pack] cannot write: %s\n", sOut); goto done; }
		if ( !XS_PackWriteFile(fOut, tBase.pWhole, (size_t)tBase.uArchiveStart) ) {
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
		if ( !XS_PackWriteFile(fOut, aHeader, sizeof(aHeader)) ) {
			printf("[pack] write header failed\n");
			goto done;
		}
		/* 数据区（临时文件回放） */
		{
			static unsigned char aCopy[65536];
			size_t n;
			rewind(fData);
			while ( (n = fread(aCopy, 1, sizeof(aCopy), fData)) > 0 ) {
				if ( !XS_PackWriteFile(fOut, aCopy, n) ) {
					printf("[pack] copy data failed\n");
					goto done;
				}
			}
			if ( ferror(fData) ) { printf("[pack] read temp data failed\n"); goto done; }
		}
		/* 索引区 + 尾标 */
		if ( !XS_PackWriteFile(fOut, pIndex, iIndexLen) ) {
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
		if ( !XS_PackWriteFile(fOut, aTrailer, sizeof(aTrailer)) ) {
			printf("[pack] write trailer failed\n");
			goto done;
		}
	}
	if ( !xrtClose(fOut) ) {
		fOut = NULL;
		printf("[pack] close failed\n");
		goto done;
	}
	fOut = NULL;
	if ( !xrtFileMove(sStage, sOut, true) ) {
		printf("[pack] publish failed: %s\n", sOut);
		goto done;
	}
	printf("[pack] %s: %u entries (lzma %u, store %u), archive %llu bytes\n",
	       sOut, (unsigned)tList.nCount, (unsigned)nLzma, (unsigned)nStore,
	       (unsigned long long)(uDataCursor + iIndexLen
	           + XS_VFS_HEADER_SIZE + XS_VFS_TRAILER_SIZE));
	iExit = 0;

done:
	if ( fOut != NULL ) xrtClose(fOut);
	if ( fData != NULL ) fclose(fData);
	if ( sStage != NULL ) { if ( iExit != 0 ) xrtFileDelete(sStage); xrtFree(sStage); }
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
	xroot pRoot = NULL;

	if ( !XS_PackParse(sExe, true, &t, sErr, sizeof(sErr)) ) {
		printf("[pack] %s\n", sErr);
		return 1;
	}
	printf("[pack] extracting %u entries to %s\n", (unsigned)t.nCount, sDir);
	/* Preflight every joined path before creating the output directory. */
	for ( uint32_t i = 0; i < t.nCount; i++ ) {
		size_t iJoined;
		if ( !XS_SizeAdd(strlen(sDir), strlen(t.pEntries[i].sPath), &iJoined) ||
		     iJoined >= XS_PATH_BYTES_LIMIT ) {
			printf("[pack] output path too long\n");
			goto done;
		}
	}
	if ( !xrtDirCreateAll(sDir) || (pRoot = xrtRootOpen(sDir)) == NULL ) {
		printf("[pack] cannot open extraction root\n");
		goto done;
	}
	for ( uint32_t i = 0; i < t.nCount; i++ ) {
		const uint8_t* pSrc = t.pWhole + t.uArchiveStart
			+ t.pEntries[i].DataOffset;
		uint8_t* pOutBuf;
		if ( t.pEntries[i].Method == 0 ) {
			pOutBuf = (uint8_t*)malloc(t.pEntries[i].OrigSize > 0
				? (size_t)t.pEntries[i].OrigSize : 1);
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
		if ( !XS_PackExtractWrite(pRoot, t.pEntries[i].sPath, pOutBuf,
			(size_t)t.pEntries[i].OrigSize) ) {
			printf("[pack] write failed in extraction root: %s\n", t.pEntries[i].sPath);
			free(pOutBuf);
			goto done;
		}
		free(pOutBuf);
	}
	printf("[pack] extract ok\n");
	iExit = 0;
done:
	if ( pRoot != NULL ) xrtRootClose(pRoot);
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
