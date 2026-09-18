/*
	站点 VFS 运行时（单文件发布，Phase 1）。

	探测自身 exe 尾部的应用包（xs pack 追加），装载只读索引；
	条目内容首次访问时惰性解压并常驻缓存。压缩数据区不进内存——
	按偏移从 exe 文件读取，热度交给 OS 文件缓存。

	布局与算法详见 docs/站点VFS设计.md。仅 LZMA（条目数据与 TCC VFS
	同构：5B props + LZMA1 流）。索引装载后不可变，缓存填充有全局锁。
*/
#ifndef XS_VFS_H
#define XS_VFS_H

#include "lib/xrt.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* 契约 API                                                            */
/* ------------------------------------------------------------------ */

/* 探测并装载自身应用包。无包/损坏返回 false（传统目录模式，无副作用）。
 * 进程启动调用一次；成功后 XS_VfsActive() 为真。 */
bool XS_VfsBootstrap(void);

/* 应用包是否已装载 */
bool XS_VfsActive(void);

/* 按站点相对路径查条目（规范归一 + 逃逸拒绝），命中返回条目号，-1 未命中。
 * 线程安全（只读索引二分）。 */
int XS_VfsLookup(const char* sRelPath);

/* 读取条目全部内容：首次访问解压（LZMA）并缓存，之后直接返回缓存视图。
 * 返回借用指针（进程期内有效，勿释放），*pSize 收长度；失败 NULL。 */
const unsigned char* XS_VfsReadAll(const char* sRelPath, size_t* pSize);

/* 按条目号的缓存读取（HTTP 静态路径用：Lookup 一次 + 命中读内容） */
const unsigned char* XS_VfsEntryData(int iEntry, size_t* pSize);

/* 条目元信息（Content-Length / 路径核对用） */
uint64_t XS_VfsEntrySize(int iEntry);
const char* XS_VfsEntryPath(int iEntry);
int XS_VfsEntryCount(void);

#ifdef __cplusplus
}
#endif

/* ================================================================== */
/* 实现                                                                */
/* ================================================================== */
#ifdef XS_VFS_IMPLEMENTATION

#include "tcc/LzmaDec.h"

/* ------------------------------------------------------------------ */
/* 格式常量（与 docs/站点VFS设计.md 逐字段一致）                        */
/* ------------------------------------------------------------------ */

#define XS_VFS_TRAILER_SIZE 24u
#define XS_VFS_HEADER_SIZE  64u
#define XS_VFS_MAX_ENTRIES  100000u
#define XS_VFS_MAX_ENTRY_ORIG_SIZE  UINT64_C(1024 * 1024 * 1024)

static const uint8_t XS_VFS_TRAILER_MAGIC[8] = { 'X','S','V','P','A','C','K',0 };
static const uint8_t XS_VFS_HEADER_MAGIC[8]  = { 'X','S','V','F','H','D','R',0 };

typedef struct XS_VfsTrailer {
	uint64_t HeaderOffset;
	uint32_t Crc32;
	uint32_t Flags;
} XS_VfsTrailer;

typedef struct XS_VfsHeader {
	uint16_t Version;
	uint16_t HeaderSize;
	uint32_t EntryCount;
	uint64_t DataOffset;      /* 相对归档头 */
	uint64_t IndexOffset;     /* 相对归档头 */
	uint64_t ArchiveSize;
	uint32_t IndexCrc32;
	uint32_t Flags;
	uint64_t Reserved0;
	uint64_t Reserved1;
} XS_VfsHeader;

typedef struct XS_VfsEntry {
	char* sPath;             /* 规范相对路径（字典序） */
	uint32_t PathLen;
	uint8_t  Method;          /* 0=STORE 1=LZMA */
	uint64_t DataOffset;      /* 相对归档头 */
	uint32_t CompSize;
	uint64_t OrigSize;
	uint32_t OrigCrc32;
	/* 惰性缓存（锁内填充，之后只读借用） */
	unsigned char* pCached;
} XS_VfsEntry;

static struct {
	bool bActive;
	xfile hSelf;             /* 自身 exe（共享句柄，借用读） */
	uint64_t uArchiveStart;  /* 归档头绝对偏移 */
	XS_VfsHeader Header;
	XS_VfsEntry* pEntries;
	/* 缓存填充互斥（索引只读，只有 pCached 需要锁；解压可能数十毫秒，
	 * 自旋在冷启动首访竞争下可接受——站点文件就绪后全部走无锁快路径） */
	xspinlock* pCacheLock;
} g_XS_Vfs;

/* ------------------------------------------------------------------ */
/* CRC32（与 qrpng/PNG 同款表法；打包侧 python zlib.crc32 / C 工具一致） */
/* ------------------------------------------------------------------ */

static uint32_t s_XS_VfsCrcTable[256];
static int s_XS_VfsCrcReady = 0;

static uint32_t XS_VfsCrc32(uint32_t uCrc, const void* pData, size_t iSize)
{
	const uint8_t* p = (const uint8_t*)pData;

	if ( !s_XS_VfsCrcReady ) {
		for ( uint32_t i = 0; i < 256; i++ ) {
			uint32_t e = i;
			for ( int j = 0; j < 8; j++ )
				e = (e & 1) ? (0xEDB88320u ^ (e >> 1)) : (e >> 1);
			s_XS_VfsCrcTable[i] = e;
		}
		s_XS_VfsCrcReady = 1;
	}
	uCrc = ~uCrc;
	while ( iSize-- > 0 )
		uCrc = s_XS_VfsCrcTable[(uCrc ^ *p++) & 0xFF] ^ (uCrc >> 8);
	return ~uCrc;
}

/* 小端读写（格式全部小端） */
static uint16_t XS_VfsRd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t XS_VfsRd32(const uint8_t* p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t XS_VfsRd64(const uint8_t* p)
{
	return (uint64_t)XS_VfsRd32(p) | ((uint64_t)XS_VfsRd32(p + 4) << 32);
}

/* ------------------------------------------------------------------ */
/* 自身 exe 完整路径（引擎契约给的是目录，这里要文件本身）              */
/* ------------------------------------------------------------------ */

static bool XS_VfsReadAt(uint64_t uOffset, void* pBuffer, size_t iSize)
{
	size_t iRead = 0;
	if ( g_XS_Vfs.hSelf == NULL || iSize == 0 ) return false;
	if ( !xrtReadAtFull(g_XS_Vfs.hSelf, uOffset, pBuffer, iSize, &iRead) ||
	     iRead != iSize )
		return false;
	return true;
}

/* ------------------------------------------------------------------ */
/* 路径规范：拒绝绝对/反斜杠/.. 段；压缩重复斜杠；返回 malloc 副本或 NULL */
/* ------------------------------------------------------------------ */

static char* XS_VfsNormalizePath(const char* sPath, uint32_t* pLen)
{
	size_t n = sPath != NULL ? strlen(sPath) : 0;
	char* sOut;
	size_t j = 0;

	if ( n == 0 || n > 4096 ) return NULL;
	sOut = (char*)xrtMalloc(n + 1);
	if ( sOut == NULL ) return NULL;
	for ( size_t i = 0; i < n; i++ ) {
		char c = sPath[i];
		if ( c == '\\' ) { xrtFree(sOut); return NULL; }      /* 统一 / 分隔 */
		if ( c == '/' ) {
			if ( j == 0 || sOut[j - 1] == '/' ) continue;      /* 首部/重复斜杠 */
			sOut[j++] = '/';
			continue;
		}
		sOut[j++] = c;
	}
	/* 逐段查 .. 与 . */
	{
		size_t seg = 0;
		for ( size_t i = 0; i <= j; i++ ) {
			if ( i == j || sOut[i] == '/' ) {
				size_t len = i - seg;
				if ( len == 2 && sOut[seg] == '.' && sOut[seg + 1] == '.' ) {
					xrtFree(sOut); return NULL;
				}
				if ( len == 1 && sOut[seg] == '.' ) {
					xrtFree(sOut); return NULL;
				}
				seg = i + 1;
			}
		}
	}
	if ( j > 0 && sOut[j - 1] == '/' ) j--;                    /* 尾斜杠去除 */
	if ( j == 0 ) { xrtFree(sOut); return NULL; }
	sOut[j] = 0;
	if ( pLen != NULL ) *pLen = (uint32_t)j;
	return sOut;
}

/* ------------------------------------------------------------------ */
/* 自身 exe 完整路径（引擎契约给的是目录，这里要文件本身）              */
/* ------------------------------------------------------------------ */

static const char* XS_VfsSelfPath(void)
{
	static char sPath[4096];
	static bool bInit = false;

	if ( !bInit ) {
#if defined(_WIN32) || defined(_WIN64)
		wchar_t arrWide[4096];
		DWORD iWideLen = GetModuleFileNameW(NULL, arrWide,
			(DWORD)(sizeof(arrWide) / sizeof(arrWide[0])));
		int iLen;

		if ( iWideLen == 0 || iWideLen >= sizeof(arrWide) / sizeof(arrWide[0]) ) {
			snprintf(sPath, sizeof(sPath), "");
			return sPath;
		}
		iLen = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			arrWide, (int)iWideLen, NULL, 0, NULL, NULL);
		if ( iLen <= 0 || (size_t)iLen >= sizeof(sPath) ||
		     WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			arrWide, (int)iWideLen, sPath, iLen, NULL, NULL) != iLen ) {
			snprintf(sPath, sizeof(sPath), "");
			return sPath;
		}
		sPath[iLen] = '\0';
#else
		ssize_t iLen = readlink("/proc/self/exe", sPath, sizeof(sPath) - 1);
		if ( iLen <= 0 || (size_t)iLen >= sizeof(sPath) ) {
			snprintf(sPath, sizeof(sPath), "");
			return sPath;
		}
		sPath[iLen] = '\0';
#endif
		bInit = true;
	}
	return sPath;
}

#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#endif

/* ------------------------------------------------------------------ */
/* Bootstrap：探测 + 索引装载                                          */
/* ------------------------------------------------------------------ */

extern bool g_XS_VfsDisabled;

bool XS_VfsBootstrap(void)
{
	const char* sSelf = XS_VfsSelfPath();

	if ( g_XS_VfsDisabled ) return false;   /* --no-vfs */
	uint8_t aTrailer[XS_VFS_TRAILER_SIZE];
	uint8_t aHeader[XS_VFS_HEADER_SIZE];
	XS_VfsTrailer tTrailer;
	uint8_t* pIndex = NULL;
	uint64_t uSelfSize = 0;
	size_t iPos;
	uint32_t i;

	if ( g_XS_Vfs.bActive || sSelf == NULL ) return false;
	memset(&g_XS_Vfs, 0, sizeof(g_XS_Vfs));

	g_XS_Vfs.hSelf = xrtOpen(sSelf, XFILE_READ | XFILE_SHARE_READ);
	if ( g_XS_Vfs.hSelf == NULL ) return false;
	if ( !xrtFileSize(g_XS_Vfs.hSelf, &uSelfSize) ||
	     uSelfSize < XS_VFS_TRAILER_SIZE + XS_VFS_HEADER_SIZE ) goto fail;
	if ( !XS_VfsReadAt(uSelfSize - XS_VFS_TRAILER_SIZE,
	                   aTrailer, sizeof(aTrailer)) ) goto fail;

	/* 尾标：magic + CRC（尾标前 16B ‖ 归档头 64B） */
	if ( memcmp(aTrailer, XS_VFS_TRAILER_MAGIC, 8) != 0 ) goto fail;
	tTrailer.HeaderOffset = XS_VfsRd64(aTrailer + 8);
	tTrailer.Crc32 = XS_VfsRd32(aTrailer + 16);
	if ( tTrailer.HeaderOffset == 0 ||
	     tTrailer.HeaderOffset + XS_VFS_HEADER_SIZE + XS_VFS_TRAILER_SIZE > uSelfSize )
		goto fail;
	if ( !XS_VfsReadAt(tTrailer.HeaderOffset, aHeader, sizeof(aHeader)) ) goto fail;
	{
		uint8_t aCrcBuf[16 + XS_VFS_HEADER_SIZE];
		memcpy(aCrcBuf, aTrailer, 16);
		memcpy(aCrcBuf + 16, aHeader, XS_VFS_HEADER_SIZE);
		if ( XS_VfsCrc32(0, aCrcBuf, sizeof(aCrcBuf)) != tTrailer.Crc32 ) goto fail;
	}

	/* 归档头 */
	if ( memcmp(aHeader, XS_VFS_HEADER_MAGIC, 8) != 0 ) goto fail;
	g_XS_Vfs.Header.Version = XS_VfsRd16(aHeader + 8);
	if ( g_XS_Vfs.Header.Version != 1 ) goto fail;
	g_XS_Vfs.Header.EntryCount = XS_VfsRd32(aHeader + 12);
	g_XS_Vfs.Header.DataOffset = XS_VfsRd64(aHeader + 16);
	g_XS_Vfs.Header.IndexOffset = XS_VfsRd64(aHeader + 24);
	g_XS_Vfs.Header.ArchiveSize = XS_VfsRd64(aHeader + 32);
	g_XS_Vfs.Header.IndexCrc32 = XS_VfsRd32(aHeader + 40);
	if ( g_XS_Vfs.Header.EntryCount == 0 ||
	     g_XS_Vfs.Header.EntryCount > XS_VFS_MAX_ENTRIES ) goto fail;
	if ( tTrailer.HeaderOffset + g_XS_Vfs.Header.ArchiveSize != uSelfSize ) goto fail;
	if ( g_XS_Vfs.Header.DataOffset >= g_XS_Vfs.Header.ArchiveSize ||
	     g_XS_Vfs.Header.IndexOffset <= g_XS_Vfs.Header.DataOffset ||
	     g_XS_Vfs.Header.IndexOffset >= g_XS_Vfs.Header.ArchiveSize ) goto fail;

	g_XS_Vfs.uArchiveStart = tTrailer.HeaderOffset;

	/* 索引区 = [IndexOffset, ArchiveSize - TRAILER) */
	{
		uint64_t uIndexEnd = g_XS_Vfs.Header.ArchiveSize - XS_VFS_TRAILER_SIZE;
		uint64_t uLen = uIndexEnd - g_XS_Vfs.Header.IndexOffset;
		if ( uLen == 0 || uLen > (64u << 20) ) goto fail;   /* 索引区 64MB 上限 */
		pIndex = (uint8_t*)xrtMalloc((size_t)uLen);
		if ( pIndex == NULL ) goto fail;
		if ( !XS_VfsReadAt(g_XS_Vfs.uArchiveStart + g_XS_Vfs.Header.IndexOffset,
		                   pIndex, (size_t)uLen) ) goto fail;
		if ( XS_VfsCrc32(0, pIndex, (size_t)uLen) != g_XS_Vfs.Header.IndexCrc32 )
			goto fail;
		/* 逐条解析 */
		g_XS_Vfs.pEntries = (XS_VfsEntry*)xrtCalloc(
			g_XS_Vfs.Header.EntryCount, sizeof(XS_VfsEntry));
		if ( g_XS_Vfs.pEntries == NULL ) goto fail;
		iPos = 0;
		for ( i = 0; i < g_XS_Vfs.Header.EntryCount; i++ ) {
			XS_VfsEntry* pE = &g_XS_Vfs.pEntries[i];
			uint32_t uPathLen;
			if ( iPos + 2 > (size_t)uLen ) goto fail;
			uPathLen = XS_VfsRd16(pIndex + iPos);
			iPos += 2;
			if ( uPathLen == 0 || iPos + uPathLen + 26 > (size_t)uLen ) goto fail;
			pE->sPath = (char*)xrtMalloc(uPathLen + 1);
			if ( pE->sPath == NULL ) goto fail;
			memcpy(pE->sPath, pIndex + iPos, uPathLen);
			pE->sPath[uPathLen] = 0;
			/* 打包侧已规范；装载侧再做逃逸防御 */
			{
				uint32_t uCheck = 0;
				char* sNorm = XS_VfsNormalizePath(pE->sPath, &uCheck);
				if ( sNorm == NULL || uCheck != uPathLen ||
				     strcmp(sNorm, pE->sPath) != 0 ) {
					xrtFree(sNorm);
					goto fail;
				}
				xrtFree(sNorm);
			}
			pE->PathLen = uPathLen;
			iPos += uPathLen;
			pE->Method = pIndex[iPos];
			if ( pE->Method > 1 ) goto fail;                /* 仅 0=STORE 1=LZMA */
			iPos += 1;
			iPos += 1;                                       /* flags */
			pE->DataOffset = XS_VfsRd64(pIndex + iPos); iPos += 8;
			pE->CompSize = XS_VfsRd32(pIndex + iPos); iPos += 4;
			pE->OrigSize = XS_VfsRd64(pIndex + iPos); iPos += 8;
			pE->OrigCrc32 = XS_VfsRd32(pIndex + iPos); iPos += 4;
			if ( pE->OrigSize > XS_VFS_MAX_ENTRY_ORIG_SIZE ) goto fail;
			if ( pE->Method == 0 && pE->CompSize != (uint32_t)pE->OrigSize ) goto fail;
			if ( pE->DataOffset < g_XS_Vfs.Header.DataOffset ||
			     pE->DataOffset + pE->CompSize > g_XS_Vfs.Header.IndexOffset )
				goto fail;
		}
		if ( iPos != (size_t)uLen ) goto fail;
		/* 字典序校验（二分前提） */
		for ( i = 1; i < g_XS_Vfs.Header.EntryCount; i++ ) {
			if ( strcmp(g_XS_Vfs.pEntries[i - 1].sPath,
			            g_XS_Vfs.pEntries[i].sPath) >= 0 ) goto fail;
		}
	}

	xrtFree(pIndex);
	g_XS_Vfs.pCacheLock = xrtSpinCreate();
	if ( g_XS_Vfs.pCacheLock == NULL ) goto fail;
	g_XS_Vfs.bActive = true;
	return true;

fail:
	xrtFree(pIndex);
	if ( g_XS_Vfs.pCacheLock != NULL ) {
		xrtSpinDestroy(g_XS_Vfs.pCacheLock);
		g_XS_Vfs.pCacheLock = NULL;
	}
	if ( g_XS_Vfs.pEntries != NULL ) {
		for ( uint32_t k = 0; k < g_XS_Vfs.Header.EntryCount; k++ )
			xrtFree(g_XS_Vfs.pEntries[k].sPath);
		xrtFree(g_XS_Vfs.pEntries);
		g_XS_Vfs.pEntries = NULL;
	}
	if ( g_XS_Vfs.hSelf != NULL ) {
		xrtClose(g_XS_Vfs.hSelf);
		g_XS_Vfs.hSelf = NULL;
	}
	memset(&g_XS_Vfs.Header, 0, sizeof(g_XS_Vfs.Header));
	return false;
}

bool XS_VfsActive(void)
{
	return g_XS_Vfs.bActive;
}

int XS_VfsEntryCount(void)
{
	return g_XS_Vfs.bActive ? (int)g_XS_Vfs.Header.EntryCount : 0;
}

const char* XS_VfsEntryPath(int iEntry)
{
	if ( !g_XS_Vfs.bActive || iEntry < 0 ||
	     (uint32_t)iEntry >= g_XS_Vfs.Header.EntryCount ) return NULL;
	return g_XS_Vfs.pEntries[iEntry].sPath;
}

uint64_t XS_VfsEntrySize(int iEntry)
{
	if ( !g_XS_Vfs.bActive || iEntry < 0 ||
	     (uint32_t)iEntry >= g_XS_Vfs.Header.EntryCount ) return 0;
	return g_XS_Vfs.pEntries[iEntry].OrigSize;
}

int XS_VfsLookup(const char* sRelPath)
{
	uint32_t uLen = 0;
	char* sNorm;
	int iLow, iHigh;

	if ( !g_XS_Vfs.bActive ) return -1;
	sNorm = XS_VfsNormalizePath(sRelPath, &uLen);
	if ( sNorm == NULL ) return -1;
	iLow = 0;
	iHigh = (int)g_XS_Vfs.Header.EntryCount - 1;
	while ( iLow <= iHigh ) {
		int iMid = (iLow + iHigh) / 2;
		int iCmp = strcmp(sNorm, g_XS_Vfs.pEntries[iMid].sPath);
		if ( iCmp == 0 ) { xrtFree(sNorm); return iMid; }
		if ( iCmp < 0 ) iHigh = iMid - 1;
		else iLow = iMid + 1;
	}
	xrtFree(sNorm);
	return -1;
}

/* ------------------------------------------------------------------ */
/* 惰性解压 + 缓存                                                     */
/* ------------------------------------------------------------------ */

/* LZMA 解码：镜像 tcc_builtin_vfs.c 的调用形态（props 5B + 流） */
static void* XS_VfsLzmaAlloc(ISzAllocPtr pAlloc, size_t size)
{
	(void)pAlloc;
	return size == 0 ? NULL : xrtMalloc(size);
}
static void XS_VfsLzmaFree(ISzAllocPtr pAlloc, void* address)
{
	(void)pAlloc;
	xrtFree(address);
}
static const ISzAlloc s_XS_VfsLzmaAlloc = { XS_VfsLzmaAlloc, XS_VfsLzmaFree };

static unsigned char* XS_VfsDecodeLzma(const uint8_t* pSrc, size_t iSrcSize,
                                       uint64_t uOrigSize)
{
	unsigned char* pOut;
	size_t uOrig;
	SizeT uIn = 0;
	ELzmaStatus eStatus;
	SRes eRes;

	if ( uOrigSize == 0 || uOrigSize > XS_VFS_MAX_ENTRY_ORIG_SIZE ) return NULL;
	pOut = (unsigned char*)xrtMalloc((size_t)uOrigSize);
	if ( pOut == NULL ) return NULL;
	uOrig = (size_t)uOrigSize;
	uIn = (SizeT)(iSrcSize - LZMA_PROPS_SIZE);
	eRes = LzmaDecode(pOut, &uOrig,
		pSrc + LZMA_PROPS_SIZE, &uIn,
		pSrc, LZMA_PROPS_SIZE,
		LZMA_FINISH_END, &eStatus, &s_XS_VfsLzmaAlloc);
	if ( eRes != SZ_OK || uOrig != (size_t)uOrigSize ||
	     (uIn + LZMA_PROPS_SIZE) != iSrcSize ) {
		xrtFree(pOut);
		return NULL;
	}
	return pOut;
}

static const unsigned char* XS_VfsEntryDataLocked(int iEntry, size_t* pSize)
{
	XS_VfsEntry* pE = &g_XS_Vfs.pEntries[iEntry];
	unsigned char* pComp = NULL;
	unsigned char* pOut = NULL;

	if ( pE->pCached != NULL ) {                              /* 双检快路径 */
		*pSize = (size_t)pE->OrigSize;
		return pE->pCached;
	}
	/* 读压缩区间 */
	pComp = (unsigned char*)xrtMalloc(pE->CompSize);
	if ( pComp == NULL ) return NULL;
	if ( !XS_VfsReadAt(g_XS_Vfs.uArchiveStart + pE->DataOffset,
	                   pComp, pE->CompSize) ) {
		xrtFree(pComp);
		return NULL;
	}
	if ( pE->Method == 1 ) {
		pOut = XS_VfsDecodeLzma(pComp, pE->CompSize, pE->OrigSize);
		xrtFree(pComp);
		if ( pOut == NULL ) return NULL;
	}
	else {
		pOut = pComp;                                          /* STORE 直用 */
	}
	if ( XS_VfsCrc32(0, pOut, (size_t)pE->OrigSize) != pE->OrigCrc32 ) {
		xrtFree(pOut);
		return NULL;
	}
	pE->pCached = pOut;                                       /* 发布（指针写原子够用） */
	*pSize = (size_t)pE->OrigSize;
	return pOut;
}

const unsigned char* XS_VfsEntryData(int iEntry, size_t* pSize)
{
	const unsigned char* pResult;

	if ( !g_XS_Vfs.bActive || iEntry < 0 ||
	     (uint32_t)iEntry >= g_XS_Vfs.Header.EntryCount || pSize == NULL )
		return NULL;
	if ( g_XS_Vfs.pEntries[iEntry].pCached != NULL ) {         /* 无锁快路径 */
		*pSize = (size_t)g_XS_Vfs.pEntries[iEntry].OrigSize;
		return g_XS_Vfs.pEntries[iEntry].pCached;
	}
	xrtSpinLock(g_XS_Vfs.pCacheLock);
	pResult = XS_VfsEntryDataLocked(iEntry, pSize);
	xrtSpinUnlock(g_XS_Vfs.pCacheLock);
	return pResult;
}

const unsigned char* XS_VfsReadAll(const char* sRelPath, size_t* pSize)
{
	int iEntry = XS_VfsLookup(sRelPath);
	if ( iEntry < 0 ) return NULL;
	return XS_VfsEntryData(iEntry, pSize);
}

#endif /* XS_VFS_IMPLEMENTATION */
#endif /* XS_VFS_H */
