


#include <stdlib.h>
#include <stdio.h>



#define MMU_USE_SAMM								// 动态结构体数组



#include "xrt/xrt.h"
#include "mmu.h"



// 区分只读版本和读写版本
#ifdef XPK_BUILD_READONLY
	#include "lz4.h"
	#include "LzmaDec/lzma.h"
#else
	#include "lz4.h"
	#include "lzma/lzma.h"
#endif



#include "xPack.h"



#define XPK_VERSION			0x106B7078			// 包文件头：0x106B7078(xpk + 版本) [ 版本 6.0 = 10 ]



// 报错入口
str xPack_Error_Text[] = {
	"文件无法访问",			// 1
	"文件格式不正确",			// 2
	"内存申请失败",			// 3
	"文件列表读取失败",		// 4
	"文件列表数据添加失败",	// 5
	"无效的文件位置",			// 6
	"文件hash校验失败",		// 7
	"文件读取失败",			// 8
	"文件写入失败",			// 9
	"文件读写位置移动失败",	// 10
	"包类型不匹配",			// 11
	"找不到文件",				// 12
	"文件名超长",				// 13
};
#define xPack_OnError_Router(xpk, err)	if ( xpk->OnError ) { xpk->OnError(err, xPack_Error_Text[err - 1]); }
#define xProc_OnError_Router(err)	xrtSetError(xPack_Error_Text[err - 1], FALSE); return 0;

// 压缩入口（压缩失败自动转换为无压缩）
XXAPI int xPack_Compress_Router(xPackObject xpk, xPack_CompInfo* pInfo)
{
	int lv = pInfo->Level & XPK_COMP_CUSTOM;
	if ( lv == XPK_COMP_FAST ) {
		// 快速压缩（LZ4）
		uint iDstSize = LZ4_compressBound(pInfo->SrcSize);
		ptr pOut = xrtMalloc(iDstSize);
		if ( pOut == 0 ) { xPack_OnError_Router(xpk, 3); xProc_OnError_Router(3) }
		uint iRet = LZ4_compress_default(pInfo->SrcAddr, pOut, pInfo->SrcSize, iDstSize);
		if ( iRet && ( iRet < pInfo->SrcSize ) ) {
			pInfo->DstAddr = pOut;
			pInfo->DstSize = iRet;
			pInfo->FreeData = TRUE;
			return TRUE;
		} else {
			xrtFree(pOut);
		}
	} else if ( lv == XPK_COMP_HIGH ) {
		// 高压缩比（LZMA）
		ptr pOut = xrtMalloc(pInfo->SrcSize);
		if ( pOut == 0 ) { xPack_OnError_Router(xpk, 3); xProc_OnError_Router(3) }
		uint iRet = Lzma_Compress(pInfo->SrcAddr, pOut, pInfo->SrcSize, pInfo->SrcSize, 6);
		if ( iRet && ( iRet < pInfo->SrcSize ) ) {
			pInfo->DstAddr = pOut;
			pInfo->DstSize = iRet;
			pInfo->FreeData = TRUE;
			return TRUE;
		} else {
			xrtFree(pOut);
		}
	} else if ( lv == XPK_COMP_CUSTOM ) {
		// 自定义压缩
		if ( xpk->OnCompress ) {
			uint iRet = xpk->OnCompress(xpk, pInfo);
			if ( iRet && ( iRet < pInfo->SrcSize ) ) {
				return TRUE;
			}
		}
	} else {
		// 不压缩
	}
	// 压缩失败直接返回输入的数据（函数执行则必定返回数据）
	pInfo->DstAddr = pInfo->SrcAddr;
	pInfo->DstSize = pInfo->SrcSize;
	pInfo->FreeData = FALSE;
	return FALSE;
}

// 解压入口
XXAPI int xPack_DeCompress_Router(xPackObject xpk, xPack_CompInfo* pInfo)
{
	int lv = pInfo->Level & XPK_COMP_CUSTOM;
	if ( lv == XPK_COMP_FAST ) {
		// 快速压缩（LZ4）
		str pOut = xrtMalloc(pInfo->DstSize + 4);
		if ( pOut == 0 ) { xPack_OnError_Router(xpk, 3); xProc_OnError_Router(3) }
		int iRet = LZ4_decompress_safe(pInfo->SrcAddr, pOut, pInfo->SrcSize, pInfo->DstSize);
		if ( iRet > 0 ) {
			pInfo->DstAddr = pOut;
			pInfo->DstSize = iRet;
			pInfo->FreeData = TRUE;
			// 添加字符串截断，方便读取后直接访问
			pOut[pInfo->DstSize] = 0;
			pOut[pInfo->DstSize+1] = 0;
			pOut[pInfo->DstSize+2] = 0;
			pOut[pInfo->DstSize+3] = 0;
			return TRUE;
		} else {
			xrtFree(pOut);
		}
	} else if ( lv == XPK_COMP_HIGH ) {
		// 高压缩比（LZMA）
		str pOut = xrtMalloc(pInfo->DstSize + 4);
		if ( pOut == 0 ) { xPack_OnError_Router(xpk, 3); xProc_OnError_Router(3) }
		int iRet = Lzma_Uncompress(pInfo->SrcAddr, pOut, pInfo->SrcSize, pInfo->DstSize);
		if ( iRet > 0 ) {
			pInfo->DstAddr = pOut;
			pInfo->DstSize = iRet;
			pInfo->FreeData = TRUE;
			// 添加字符串截断，方便读取后直接访问
			pOut[pInfo->DstSize] = 0;
			pOut[pInfo->DstSize+1] = 0;
			pOut[pInfo->DstSize+2] = 0;
			pOut[pInfo->DstSize+3] = 0;
			return TRUE;
		} else {
			xrtFree(pOut);
		}
	} else if ( lv == XPK_COMP_CUSTOM ) {
		// 自定义压缩
		if ( xpk->OnUnCompress ) {
			uint iRet = xpk->OnUnCompress(xpk, pInfo);
			if ( iRet > 0 ) {
				return TRUE;
			}
		}
	} else {
		// 不压缩
		pInfo->DstAddr = pInfo->SrcAddr;
		pInfo->DstSize = pInfo->SrcSize;
		pInfo->FreeData = FALSE;
		return TRUE;
	}
	return FALSE;
}



// 保存文件包
XXAPI int xPack_Save(xPackObject xpk, int bReBuild)
{
	// 更新文件头数据
	xpk->PackHead.FileHead = XPK_VERSION;
	xpk->PackHead.FileCount = xpk->LDB->Count;
	if ( xpk->PackHead.FileCount ) {
		// 计算 LDB 哈希值
		uint LDB_Size = xpk->LDB->ItemLength * xpk->PackHead.FileCount;
		xpk->PackHead.LDB_Hash = xrtHash32_WithSeed(xpk->LDB->Memory, LDB_Size, HASH32_SEED);
		// 压缩文件列表
		xPack_CompInfo CompInfo;
		CompInfo.Level = XPK_COMP_HIGH;		// 默认 LZMA 压缩 LDB 数据
		CompInfo.SrcAddr = xpk->LDB->Memory;
		CompInfo.SrcSize = LDB_Size;
		int bRet = xPack_Compress_Router(xpk, &CompInfo);
		if ( bRet ) {
			xpk->PackHead.PackFlag &= ~(XPK_LDBCOMP | XPK_LDBCOMPTYPE);
			xpk->PackHead.PackFlag |= (XPK_LDBCOMP | XPK_LDBCOMPTYPE);
		} else {
			xpk->PackHead.PackFlag &= ~(XPK_LDBCOMP | XPK_LDBCOMPTYPE);
		}
		// 写入文件列表
		bRet = xrtSeek(xpk->FileObject, xpk->FileOffset + xpk->PackHead.LDB_Addr, XRT_SEEK_SET);
		if ( bRet == 0 ) { if ( CompInfo.FreeData ) { xrtFree(CompInfo.DstAddr); } xPack_OnError_Router(xpk, 10); xProc_OnError_Router(10); }
		ulong iRet = xrtPut(xpk->FileObject, CompInfo.DstAddr, CompInfo.DstSize);
		if ( iRet != CompInfo.DstSize ) { if ( CompInfo.FreeData ) { xrtFree(CompInfo.DstAddr); } xPack_OnError_Router(xpk, 9); xProc_OnError_Router(9); }
		if ( CompInfo.FreeData ) { xrtFree(CompInfo.DstAddr); }
		xpk->PackHead.LDB_Size = CompInfo.DstSize;
	} else {
		xpk->PackHead.LDB_Size = 0;
		xpk->PackHead.LDB_Hash = 0;
	}
	// 写入文件头数据
	int bRet = xrtSeek(xpk->FileObject, xpk->FileOffset, XRT_SEEK_SET);
	if ( bRet == 0 ) { xPack_OnError_Router(xpk, 10); xProc_OnError_Router(10); }
	ulong iRet = xrtPut(xpk->FileObject, &xpk->PackHead, sizeof(xPack_FileHead));
	if ( iRet != sizeof(xPack_FileHead) ) { xPack_OnError_Router(xpk, 9); xProc_OnError_Router(9); }
	xpk->IsChange = 0;
	return -1;
}

// 关闭文件包
XXAPI void xPack_Close(xPackObject xpk)
{
	// 保存文件 [存在修改时自动存储]
	if ( (xpk->ReadOnly == 0) && xpk->IsChange ) {
		xPack_Save(xpk, FALSE);
	}
	// 清理数据
	if ( xpk->FileObject ) { xrtClose(xpk->FileObject); }
	if ( xpk->LDB ) { SAMM_Destroy(xpk->LDB); }
	xrtFree(xpk);
}

// 打开文件包(如果文件不存在则创建)
XXAPI xPackObject xPack_Open(str sFile, uint iOffset, int bReadOnly)
{
	// 文件不存在则创建
	xPackObject xpk = xrtMalloc(sizeof(xPackStruct));
	if ( xpk == 0 ) { xProc_OnError_Router(3) }
	memset(xpk, 0, sizeof(xPackStruct));
	if ( (bReadOnly == 0) && (xrtFileExists(sFile) == 0) ) {
		xpk->PackHead.FileHead = XPK_VERSION;
		xpk->PackHead.PackFlag = XPK_CLASS_Core;
		xpk->PackHead.FileCount = 0;
		xpk->PackHead.LDB_Addr = sizeof(xPack_FileHead);
		xpk->PackHead.LDB_Size = 0;
		xpk->PackHead.LDB_Hash = 0;
		xpk->PackHead.HeadSize = 0;
		xpk->PackHead.InfoSize = 0;
		xpk->PackHead.DiscCode = 0;
		xpk->PackHead.Reserve  = 0;
		ulong iRet = xrtFilePutAll(sFile, &xpk->PackHead, sizeof(xPack_FileHead));
		if ( iRet != sizeof(xPack_FileHead) ) { xPack_OnError_Router(xpk, 9); xProc_OnError_Router(9); }
	}
	// 打开文件
	xpk->FileOffset = iOffset;
	xpk->FileObject = xrtOpen(sFile, bReadOnly, XRT_CP_BINARY);
	if ( xpk->FileObject == 0 ) {
		xrtFree(xpk);
		xProc_OnError_Router(1)
	}
	// 读取信息段，验证文件头和版本
	int bRet = xrtSeek(xpk->FileObject, iOffset, XRT_SEEK_SET);
	if ( bRet == 0 ) { xrtClose(xpk->FileObject); xrtFree(xpk); xProc_OnError_Router(10); }
	int iRet = xrtGet(xpk->FileObject, &xpk->PackHead, sizeof(xPack_FileHead));
	if ( iRet != sizeof(xPack_FileHead) ) { xrtClose(xpk->FileObject); xrtFree(xpk); xProc_OnError_Router(8); }
	if ( xpk->PackHead.FileHead != XPK_VERSION ) {
		xrtClose(xpk->FileObject);
		xrtFree(xpk);
		xProc_OnError_Router(2)
	}
	// 读取文件列表(LDB段)
	xpk->LDB = SAMM_Create(sizeof(xPack_FileInfo) + xpk->PackHead.InfoSize, 32);
	if ( xpk->LDB == 0 ) {
		xrtClose(xpk->FileObject);
		xrtFree(xpk);
		xProc_OnError_Router(3)
	}
	// 扩展 LDB 段内存
	if ( SAMM_Malloc(xpk->LDB, xpk->PackHead.FileCount) == 0 ) {
		xPack_Close(xpk);
		xProc_OnError_Router(3)
	}
	// 读取文件列表（文件列表没压缩时直接读入xBsmm）
	if ( xpk->PackHead.FileCount ) {
		uint LDB_Size = (sizeof(xPack_FileInfo) + xpk->PackHead.InfoSize) * xpk->PackHead.FileCount;
		if ( xpk->PackHead.PackFlag & XPK_LDBCOMP ) {
			// 解压文件列表(LDB段)
			ptr LDB_Data = xrtMalloc(xpk->PackHead.LDB_Size);
			if ( LDB_Data == 0 ) { xPack_Close(xpk); xProc_OnError_Router(3) }
			bRet = xFile_Seek(xpk->FileObject, iOffset + xpk->PackHead.LDB_Addr, XRT_SEEK_SET);
			if ( bRet == 0 ) { xPack_Close(xpk); xProc_OnError_Router(10); }
			iRet = xFile_Get(xpk->FileObject, LDB_Data, xpk->PackHead.LDB_Size);
			if ( iRet != xpk->PackHead.LDB_Size ) { xPack_Close(xpk); xProc_OnError_Router(8); }
			xPack_CompInfo CompInfo;
			CompInfo.Level = xpk->PackHead.PackFlag & XPK_LDBCOMPTYPE ? XPK_COMP_HIGH : XPK_COMP_FAST;
			CompInfo.SrcAddr = LDB_Data;
			CompInfo.SrcSize = xpk->PackHead.LDB_Size;
			CompInfo.DstSize = (sizeof(xPack_FileInfo) + xpk->PackHead.InfoSize) * xpk->PackHead.FileCount;
			xPack_DeCompress_Router(xpk, &CompInfo);
			if ( CompInfo.FreeData ) { xrtFree(LDB_Data); }
			memcpy(xpk->LDB->Memory, CompInfo.DstAddr, CompInfo.DstSize);
			xrtFree(CompInfo.DstAddr);
		} else {
			// 无需解压，直接读取 LDB 段
			bRet = xFile_Seek(xpk->FileObject, iOffset + xpk->PackHead.LDB_Addr, XRT_SEEK_SET);
			if ( bRet == 0 ) { xPack_Close(xpk); xProc_OnError_Router(10); }
			iRet = xFile_Get(xpk->FileObject, xpk->LDB->Memory, xpk->PackHead.LDB_Size);
			if ( iRet != xpk->PackHead.LDB_Size ) { xPack_Close(xpk); xProc_OnError_Router(8); }
		}
		// 校验文件列表数据
		int iHash = XXH32(xpk->LDB->Memory, LDB_Size, 0);
		if ( iHash != xpk->PackHead.LDB_Hash ) { xPack_Close(xpk); xProc_OnError_Router(4) }
		// 文件打开成功
		xpk->LDB->Count = xpk->PackHead.FileCount;
	}
	return xpk;
}



// 获取包文件数量
XXAPI uint xPack_FileCount(xPackObject xpk)
{
	if ( xpk && xpk->LDB ) { return xpk->LDB->Count; }
	return 0;
}

// 设置包类型（只有在还没添加文件的时候可以修改）
XXAPI int xPack_SetPackType(xPackObject xpk, int iVal)
{
	if ( xpk && xpk->LDB ) {
		if ( xpk->LDB->Count > 0 ) { return 0; }
		xpk->PackHead.PackFlag &= ~XPK_CLASS_MASK;
		xpk->PackHead.PackFlag |= (iVal & XPK_CLASS_MASK);
		if ( (iVal & XPK_CLASS_MASK) == XPK_CLASS_Index ) {
			xpk->PackHead.InfoSize = sizeof(xPack_FileInfo_Index) - sizeof(xPack_FileInfo);
			xpk->LDB->ItemLength = sizeof(xPack_FileInfo_Index);
		} else if ( (iVal & XPK_CLASS_MASK) == XPK_CLASS_Linux ) {
			xpk->PackHead.InfoSize = sizeof(xPack_FileInfo_Linux) - sizeof(xPack_FileInfo);
			xpk->LDB->ItemLength = sizeof(xPack_FileInfo_Linux);
		} else if ( (iVal & XPK_CLASS_MASK) == XPK_CLASS_Win32 ) {
			xpk->PackHead.InfoSize = sizeof(xPack_FileInfo_Win32) - sizeof(xPack_FileInfo);
			xpk->LDB->ItemLength = sizeof(xPack_FileInfo_Win32);
		} else {
			xpk->PackHead.InfoSize = 0;
			xpk->LDB->ItemLength = sizeof(xPack_FileInfo);
		}
		return -1;
	}
	return 0;
}

// 获取包类型
XXAPI int xPack_GetPackType(xPackObject xpk)
{
	if ( xpk ) { return xpk->PackHead.PackFlag & XPK_CLASS_MASK; }
	return 0;
}

// 设置文件信息扩展长度（只有在还没添加文件的时候可以修改）（只能设置 Core 模式的包）
XXAPI int xPack_SetFileInfoExtSize(xPackObject xpk, uint iVal)
{
	if ( xpk && xpk->LDB && (xpk->PackHead.PackFlag & XPK_CLASS_MASK == XPK_CLASS_Core) ) {
		if ( xpk->LDB->Count > 0 ) { return 0; }
		xpk->PackHead.InfoSize = iVal;
		return -1;
	}
	return 0;
}

// 获取文件信息扩展长度
XXAPI int xPack_GetFileInfoExtSize(xPackObject xpk)
{
	if ( xpk ) { return xpk->PackHead.InfoSize; }
	return 0;
}

// 设置包信息扩展长度（只有在还没添加文件的时候可以修改）（只能设置 Core 模式的包）
XXAPI int xPack_SetPackInfoExtSize(xPackObject xpk, uint iVal)
{
	if ( xpk && xpk->LDB && (xpk->PackHead.PackFlag & XPK_CLASS_MASK == XPK_CLASS_Core) ) {
		if ( xpk->LDB->Count > 0 ) { return 0; }
		xpk->PackHead.HeadSize = iVal;
		return -1;
	}
	return 0;
}

// 获取文件信息扩展长度
XXAPI int xPack_GetPackInfoExtSize(xPackObject xpk)
{
	if ( xpk ) { return xpk->PackHead.HeadSize; }
	return 0;
}

// 设置包文件识别代码
XXAPI int xPack_SetPackDiscCode(xPackObject xpk, uint iVal)
{
	if ( xpk ) {
		xpk->PackHead.DiscCode = iVal;
		return -1;
	}
	return 0;
}

// 获取包文件识别代码
XXAPI int xPack_GetPackDiscCode(xPackObject xpk)
{
	if ( xpk ) { return xpk->PackHead.DiscCode; }
	return 0;
}



// 获取文件信息结构体指针
XXAPI ptr xPack_GetFileInfo(xPackObject xpk, uint iPos)
{
	if ( xpk && xpk->LDB ) {
		return SAMM_GetPtr(xpk->LDB, iPos);
	}
	return NULL;
}

// 获取文件大小
XXAPI uint xPack_GetFileSize(xPackObject xpk, uint iPos)
{
	xPack_FileInfo* pInfo = xPack_GetFileInfo(xpk, iPos);
	if ( pInfo ) {
		return pInfo->FileSize;
	}
	return 0;
}

// 获取数据大小
XXAPI uint xPack_GetFileDataSize(xPackObject xpk, uint iPos)
{
	xPack_FileInfo* pInfo = xPack_GetFileInfo(xpk, iPos);
	if ( pInfo ) {
		return pInfo->DataSize;
	}
	return 0;
}

// 获取文件哈希值
XXAPI uint xPack_GetFileHash(xPackObject xpk, uint iPos)
{
	xPack_FileInfo* pInfo = xPack_GetFileInfo(xpk, iPos);
	if ( pInfo ) {
		return pInfo->FileHash;
	}
	return 0;
}

// 获取文件压缩级别
XXAPI uint xPack_GetFileCompLevel(xPackObject xpk, uint iPos)
{
	xPack_FileInfo* pInfo = xPack_GetFileInfo(xpk, iPos);
	if ( pInfo ) {
		return pInfo->FileFlag & XPK_COMP_CUSTOM;
	}
	return 0;
}



// 添加数据（核心模式）
XXAPI uint xPack_Core_AppendData(xPackObject xpk, ptr pIn, uint iSize, int iCompLevel)
{
	if ( xpk && xpk->LDB ) {
		// 创建文件信息
		uint iPos = SAMM_Append(xpk->LDB, 1);
		xPack_FileInfo* pInfo = SAMM_GetPtr(xpk->LDB, iPos);
		if ( pInfo == NULL ) { xPack_OnError_Router(xpk, 5); xProc_OnError_Router(5); }
		// 记录文件的数据
		uint iFileSize = iSize;
		uint iFileHash = 0;
		uint iFileFlag = XPK_COMP_NO;
		if ( iSize > 0 ) {
			iFileHash = XXH32(pIn, iSize, 0);
			// 压缩数据
			xPack_CompInfo CompInfo;
			CompInfo.Level = iCompLevel;
			CompInfo.SrcAddr = pIn;
			CompInfo.SrcSize = iSize;
			int bRet = xPack_Compress_Router(xpk, &CompInfo);
			if ( bRet ) {
				pIn = CompInfo.DstAddr;
				iSize = CompInfo.DstSize;
				iFileFlag = CompInfo.Level;
			}
			// 写入文件数据
			bRet = xFile_Seek(xpk->FileObject, xpk->FileOffset + xpk->PackHead.LDB_Addr, XRT_SEEK_SET);
			if ( bRet == 0 ) { SAMM_Remove(xpk->LDB, iPos, 1); if ( CompInfo.FreeData ) { xrtFree(pIn); } xPack_OnError_Router(xpk, 10); xProc_OnError_Router(10); }
			int iRet = xFile_Put(xpk->FileObject, pIn, iSize);
			if ( iRet != iSize ) { SAMM_Remove(xpk->LDB, iPos, 1); if ( CompInfo.FreeData ) { xrtFree(pIn); } xPack_OnError_Router(xpk, 9); xProc_OnError_Router(9); }
			if ( CompInfo.FreeData ) { xrtFree(pIn); }
		}
		// 更新文件信息
		pInfo->DataAddr = xpk->PackHead.LDB_Addr;
		pInfo->DataSize = iSize;
		pInfo->FileSize = iFileSize;
		pInfo->FileHash = iFileHash;
		pInfo->FileFlag = iFileFlag;
		xpk->PackHead.LDB_Addr += iSize;
		xpk->IsChange = -1;
		return iPos;
	}
	return 0;
}

// 添加文件（核心模式）
XXAPI uint xPack_Core_AppendFile(xPackObject xpk, str sFile, int iCompLevel)
{
	// 读取文件到内存
	xFileObject objFile = xFile_OpenA(sFile, TRUE, CHARSET_BINARY);
	if ( objFile == 0 ) { xPack_OnError_Router(xpk, 1); xProc_OnError_Router(1); }
	uint iSize = xFile_Size(objFile);
	ptr pData = NULL;
	if ( iSize ) {
		pData = xrtMalloc(iSize);
		if ( pData == 0 ) { xrtClose(objFile); xPack_OnError_Router(xpk, 3); xProc_OnError_Router(3); }
		ulong iRet = xFile_Get(objFile, pData, iSize);
		if ( iRet != iSize ) { xrtClose(objFile); if ( pData ) { xrtFree(pData); } xPack_OnError_Router(xpk, 8); xProc_OnError_Router(8); }
	}
	xrtClose(objFile);
	// 添加文件数据
	uint iRet = xPack_Core_AppendData(xpk, pData, iSize, iCompLevel);
	if ( pData ) { xrtFree(pData); }
	return iRet;
}

// 修改数据（核心模式）
XXAPI xPack_FileInfo* xPack_Core_ChangeData(xPackObject xpk, uint iPos, ptr pIn, uint iSize, int iCompLevel)
{
	if ( xpk && xpk->LDB ) {
		// 读取文件信息
		xPack_FileInfo* pInfo = SAMM_GetPtr(xpk->LDB, iPos);
		if ( pInfo == NULL ) { xPack_OnError_Router(xpk, 6); xProc_OnError_Router(6); }
		// 记录新文件的数据
		uint iFileSize = iSize;
		uint iFileHash = 0;
		uint iFileFlag = XPK_COMP_NO;
		if ( iSize > 0 ) {
			iFileHash = XXH32(pIn, iSize, 0);
			// 压缩数据
			xPack_CompInfo CompInfo;
			CompInfo.Level = iCompLevel;
			CompInfo.SrcAddr = pIn;
			CompInfo.SrcSize = iSize;
			int bRet = xPack_Compress_Router(xpk, &CompInfo);
			if ( bRet ) {
				pIn = CompInfo.DstAddr;
				iSize = CompInfo.DstSize;
				iFileFlag = CompInfo.Level;
			}
			// 写入文件数据
			bRet = xFile_Seek(xpk->FileObject, xpk->FileOffset + xpk->PackHead.LDB_Addr, XRT_SEEK_SET);
			if ( bRet == 0 ) { SAMM_Remove(xpk->LDB, iPos, 1); if ( CompInfo.FreeData ) { xrtFree(pIn); } xPack_OnError_Router(xpk, 10); xProc_OnError_Router(10); }
			int iRet = xFile_Put(xpk->FileObject, pIn, iSize);
			if ( iRet != iSize ) { SAMM_Remove(xpk->LDB, iPos, 1); if ( CompInfo.FreeData ) { xrtFree(pIn); } xPack_OnError_Router(xpk, 9); xProc_OnError_Router(9); }
			if ( CompInfo.FreeData ) { xrtFree(pIn); }
		}
		// 更新文件信息
		pInfo->DataAddr = xpk->PackHead.LDB_Addr;
		pInfo->DataSize = iSize;
		pInfo->FileSize = iFileSize;
		pInfo->FileHash = iFileHash;
		pInfo->FileFlag = iFileFlag;
		xpk->PackHead.LDB_Addr += iSize;
		xpk->IsChange = -1;
		return pInfo;
	}
	return NULL;
}

// 修改文件（核心模式）
XXAPI xPack_FileInfo* xPack_Core_ChangeFile(xPackObject xpk, uint iPos, str sFile, int iCompLevel)
{
	// 读取文件到内存
	xFileObject objFile = xFile_OpenA(sFile, TRUE, CHARSET_BINARY);
	if ( objFile == 0 ) { xPack_OnError_Router(xpk, 1); xProc_OnError_Router(1); }
	uint iSize = xFile_Size(objFile);
	ptr pData = NULL;
	if ( iSize ) {
		pData = xrtMalloc(iSize);
		if ( pData == 0 ) { xrtClose(objFile); xPack_OnError_Router(xpk, 3); xProc_OnError_Router(3); }
		ulong iRet = xFile_Get(objFile, pData, iSize);
		if ( iRet != iSize ) { xrtClose(objFile); if ( pData ) { xrtFree(pData); } xPack_OnError_Router(xpk, 8); xProc_OnError_Router(8); }
	}
	xrtClose(objFile);
	// 更新文件数据
	xPack_FileInfo* pInfo = xPack_Core_ChangeData(xpk, iPos, pData, iSize, iCompLevel);
	if ( pData ) { xrtFree(pData); }
	return pInfo;
}

// 解包数据（核心模式）（pData需使用 xrtFree 释放）
XXAPI xPack_FileInfo* xPack_Core_UnpackData(xPackObject xpk, uint iPos, ptr* pData)
{
	if ( xpk && xpk->LDB ) {
		// 读取文件信息
		xPack_FileInfo* pInfo = SAMM_GetPtr(xpk->LDB, iPos);
		if ( pInfo == NULL ) { xPack_OnError_Router(xpk, 6); xProc_OnError_Router(6); }
		if ( pInfo->FileSize == 0 ) { *pData = xCore.sNull; return pInfo; }
		// 读取文件内容
		uint iSize = pInfo->DataSize;
		str pOut = xrtMalloc(iSize + 4);
		if ( pOut == 0 ) { xPack_OnError_Router(xpk, 3); xProc_OnError_Router(3); }
		int bRet = xFile_Seek(xpk->FileObject, xpk->FileOffset + pInfo->DataAddr, XRT_SEEK_SET);
		if ( bRet == 0 ) { xrtFree(pOut); xPack_OnError_Router(xpk, 10); xProc_OnError_Router(10); }
		ulong iRet = xFile_Get(xpk->FileObject, pOut, iSize);
		if ( iRet != iSize ) { xrtFree(pOut); xPack_OnError_Router(xpk, 8); xProc_OnError_Router(8); }
		// 解压数据
		if ( pInfo->FileFlag & XPK_COMP_CUSTOM ) {
			xPack_CompInfo CompInfo;
			CompInfo.Level = pInfo->FileFlag & XPK_COMP_CUSTOM;
			CompInfo.SrcAddr = pOut;
			CompInfo.SrcSize = iSize;
			CompInfo.DstSize = pInfo->FileSize;
			xPack_DeCompress_Router(xpk, &CompInfo);
			if ( CompInfo.FreeData ) { xrtFree(pOut); }
			pOut = CompInfo.DstAddr;
			iSize = pInfo->FileSize;
		} else {
			pOut[iSize] = 0;
			pOut[iSize+1] = 0;
			pOut[iSize+2] = 0;
			pOut[iSize+3] = 0;
		}
		// hash 校验
		uint iHash = XXH32(pOut, iSize, 0);
		if ( pInfo->FileHash == iHash ) {
			*pData = (ptr)pOut;
			return pInfo;
		} else {
			xrtFree(pOut);
			*pData = xCore.sNull;
			xPack_OnError_Router(xpk, 7); xProc_OnError_Router(7);
		}
	}
	return NULL;
}

// 解包文件（核心模式）
XXAPI xPack_FileInfo* xPack_Core_UnpackFile(xPackObject xpk, uint iPos, str sFile)
{
	ptr pOut = NULL;
	xPack_FileInfo* pInfo = xPack_Core_UnpackData(xpk, iPos, &pOut);
	if ( pInfo ) {
		if ( pInfo->FileSize > 0 ) {
			// 写出到文件
			str sPath = strrchr(sFile, '\\');
			if ( sPath == 0 ) { sPath = strrchr(sFile, '/'); }
			if ( sPath ) {
				char Old = sPath[0];
				sPath[0] = 0;
				xFile_CreateDirA(sFile);
				sPath[0] = Old;
			}
			ulong iRet = xFile_PutAllA(sFile, pOut, pInfo->FileSize);
			if ( iRet != pInfo->FileSize ) { xrtFree(pOut); xPack_OnError_Router(xpk, 9); xProc_OnError_Router(9); }
		} else {
			// 创建空文件
			xFile_SetSizeA(sFile, 0);
		}
		return pInfo;
	}
	return 0;
}

// 删除文件（核心模式）
XXAPI int xPack_Core_DeleteFile(xPackObject xpk, uint iPos)
{
	if ( xpk && xpk->LDB ) {
		// 删除文件信息
		return SAMM_Remove(xpk->LDB, iPos, 1);
	}
	return 0;
}



// 通过 Index 获取 POS
XXAPI uint xPack_IndexToPos(xPackObject xpk, int iIndex)
{
	if ( xpk && xpk->LDB ) {
		uint iCount = xpk->LDB->Count;
		for ( int i = 0; i < iCount; i++ ) {
			xPack_FileInfo_Index* pInfo = (ptr)(&xpk->LDB->Memory[i * xpk->LDB->ItemLength]);
			if ( pInfo->FileIndex == iIndex ) {
				return i + 1;
			}
		}
	}
	return 0;
}

// 添加文件（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_AppendFile(xPackObject xpk, int iIndex, str sFile, int iCompLevel)
{
	if ( xpk == 0 ) { return NULL; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Index ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_Core_AppendFile(xpk, sFile, iCompLevel);
	if ( iPos ) {
		xPack_FileInfo_Index* pInfo = SAMM_GetPtr(xpk->LDB, iPos);
		if ( pInfo == 0 ) { return NULL; }
		pInfo->FileIndex = iIndex;
		pInfo->FileTag = 0;
		return pInfo;
	}
	return NULL;
}

// 添加数据（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_AppendData(xPackObject xpk, int iIndex, ptr pIn, uint iSize, int iCompLevel)
{
	if ( xpk == 0 ) { return NULL; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Index ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_Core_AppendData(xpk, pIn, iSize, iCompLevel);
	if ( iPos ) {
		xPack_FileInfo_Index* pInfo = SAMM_GetPtr(xpk->LDB, iPos);
		if ( pInfo == 0 ) { return NULL; }
		pInfo->FileIndex = iIndex;
		pInfo->FileTag = 0;
		return pInfo;
	}
	return NULL;
}

// 修改文件（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_ChangeFile(xPackObject xpk, int iIndex, str sFile, int iCompLevel)
{
	if ( xpk == 0 ) { return NULL; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Index ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_IndexToPos(xpk, iIndex);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	xPack_FileInfo_Index* pInfo = (ptr)xPack_Core_ChangeFile(xpk, iPos, sFile, iCompLevel);
	return pInfo;
}

// 修改数据（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_ChangeData(xPackObject xpk, int iIndex, ptr pIn, uint iSize, int iCompLevel)
{
	if ( xpk == 0 ) { return NULL; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Index ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_IndexToPos(xpk, iIndex);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	xPack_FileInfo_Index* pInfo = (ptr)xPack_Core_ChangeData(xpk, iPos, pIn, iSize, iCompLevel);
	return pInfo;
}

// 解包文件（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_UnpackFile(xPackObject xpk, int iIndex, str sFile)
{
	if ( xpk == 0 ) { return NULL; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Index ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_IndexToPos(xpk, iIndex);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	xPack_FileInfo_Index* pInfo = (ptr)xPack_Core_UnpackFile(xpk, iPos, sFile);
	return pInfo;
}

// 解包数据（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_UnpackData(xPackObject xpk, int iIndex, ptr* pData)
{
	if ( xpk == 0 ) { return NULL; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Index ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_IndexToPos(xpk, iIndex);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	xPack_FileInfo_Index* pInfo = (ptr)xPack_Core_UnpackData(xpk, iPos, pData);
	return pInfo;
}

// 删除文件（无序索引方式）
XXAPI int xPack_Index_DeleteFile(xPackObject xpk, int iIndex)
{
	if ( xpk == 0 ) { return 0; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Index ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_IndexToPos(xpk, iIndex);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	return xPack_Index_DeleteFile(xpk, iPos);
}



// 通过 Linux文件路径 获取 POS
XXAPI uint xPack_LinuxToPos(xPackObject xpk, str sPath)
{
	if ( xpk && xpk->LDB ) {
		uint iCount = xpk->LDB->Count;
		uint iHash = XXH32(sPath, strlen(sPath), 0);
		for ( int i = 0; i < iCount; i++ ) {
			xPack_FileInfo_Linux* pInfo = (ptr)(&xpk->LDB->Memory[i * xpk->LDB->ItemLength]);
			if ( pInfo->PathHash == iHash ) {
				if ( strcmp(pInfo->FilePath, sPath) == 0 ) {
					return i + 1;
				}
			}
		}
	}
	return 0;
}

// 添加文件（Linux方式）
XXAPI xPack_FileInfo_Linux* xPack_Linux_AppendFile(xPackObject xpk, str sPath, str sFile, int iCompLevel)
{
	if ( xpk == 0 ) { return 0; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Linux ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iSize = strlen(sPath);
	if ( iSize >= XPK_FILEPATHMAX ) { xPack_OnError_Router(xpk, 13); xProc_OnError_Router(13); }
	uint iPos = xPack_Core_AppendFile(xpk, sFile, iCompLevel);
	if ( iPos ) {
		xPack_FileInfo_Linux* pInfo = SAMM_GetPtr(xpk->LDB, iPos);
		if ( pInfo == 0 ) { return NULL; }
		memset(pInfo->FilePath, 0, XPK_FILEPATHMAX);
		strcpy(pInfo->FilePath, sPath);
		pInfo->PathHash = XXH32(sPath, iSize, 0);
		pInfo->FileAttr = 0;
		pInfo->ModifyTime = 0;
		pInfo->FileTag = 0;
		pInfo->Reserve = 0;
		return pInfo;
	}
	return NULL;
}

// 修改文件（Linux方式）
XXAPI xPack_FileInfo_Linux* xPack_Linux_ChangeFile(xPackObject xpk, str sPath, str sFile, int iCompLevel)
{
	if ( xpk == 0 ) { return 0; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Linux ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_LinuxToPos(xpk, sPath);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	xPack_FileInfo_Linux* pInfo = (ptr)xPack_Core_ChangeFile(xpk, iPos, sFile, iCompLevel);
	return pInfo;
}

// 解包文件（Linux方式）
XXAPI xPack_FileInfo_Linux* xPack_Linux_UnpackFile(xPackObject xpk, str sPath, str sFile)
{
	if ( xpk == 0 ) { return 0; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Linux ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_LinuxToPos(xpk, sPath);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	xPack_FileInfo_Linux* pInfo = (ptr)xPack_Core_UnpackFile(xpk, iPos, sFile);
	return pInfo;
}

// 解包数据（Linux方式）
XXAPI xPack_FileInfo_Linux* xPack_Linux_UnpackData(xPackObject xpk, str sPath, ptr* pData)
{
	if ( xpk == 0 ) { return 0; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Linux ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_LinuxToPos(xpk, sPath);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	xPack_FileInfo_Linux* pInfo = (ptr)xPack_Core_UnpackData(xpk, iPos, pData);
	return pInfo;
}

// 删除文件（Linux方式）
XXAPI int xPack_Linux_DeleteFile(xPackObject xpk, str sPath)
{
	if ( xpk == 0 ) { return 0; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Linux ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_LinuxToPos(xpk, sPath);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	return xPack_Index_DeleteFile(xpk, iPos);
}



// 通过 Win32文件路径 获取 POS（和Linux方式的主要区别为不区分大小写）
XXAPI uint xPack_Win32ToPos(xPackObject xpk, str sPath)
{
	if ( xpk && xpk->LDB ) {
		uint iCount = xpk->LDB->Count;
		sPath = xrtLCase(sPath, 0, FALSE);
		uint iHash = XXH32(sPath, strlen(sPath), 0);
		for ( int i = 0; i < iCount; i++ ) {
			xPack_FileInfo_Linux* pInfo = (ptr)(&xpk->LDB->Memory[i * xpk->LDB->ItemLength]);
			if ( pInfo->PathHash == iHash ) {
				if ( stricmp(pInfo->FilePath, sPath) == 0 ) {
					xrtFree(sPath);
					return i + 1;
				}
			}
		}
		xrtFree(sPath);
	}
	return 0;
}

// 添加文件（Win32方式）
XXAPI xPack_FileInfo_Win32* xPack_Win32_AppendFile(xPackObject xpk, str sPath, str sFile, int iCompLevel)
{
	if ( xpk == 0 ) { return 0; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Win32 ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iSize = strlen(sPath);
	if ( iSize >= XPK_FILEPATHMAX ) { xPack_OnError_Router(xpk, 13); xProc_OnError_Router(13); }
	uint iPos = xPack_Core_AppendFile(xpk, sFile, iCompLevel);
	if ( iPos ) {
		xPack_FileInfo_Win32* pInfo = SAMM_GetPtr(xpk->LDB, iPos);
		if ( pInfo == 0 ) { return NULL; }
		memset(pInfo->FilePath, 0, XPK_FILEPATHMAX);
		strcpy(pInfo->FilePath, sPath);
		astr sLowPath = xrtLCase(sPath, iSize, FALSE);
		pInfo->PathHash = XXH32(sLowPath, iSize, 0);
		pInfo->FileAttr = 0;
		pInfo->CreateTime = 0;
		pInfo->ModifyTime = 0;
		pInfo->FileTag = 0;
		xrtFree(sLowPath);
		return pInfo;
	}
	return NULL;
}

// 修改文件（Win32方式）
XXAPI xPack_FileInfo_Win32* xPack_Win32_ChangeFile(xPackObject xpk, str sPath, str sFile, int iCompLevel)
{
	if ( xpk == 0 ) { return 0; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Win32 ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_Win32ToPos(xpk, sPath);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	xPack_FileInfo_Win32* pInfo = (ptr)xPack_Core_ChangeFile(xpk, iPos, sFile, iCompLevel);
	return pInfo;
}

// 解包文件（Win32方式）
XXAPI xPack_FileInfo_Win32* xPack_Win32_UnpackFile(xPackObject xpk, str sPath, str sFile)
{
	if ( xpk == 0 ) { return 0; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Win32 ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_Win32ToPos(xpk, sPath);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	xPack_FileInfo_Win32* pInfo = (ptr)xPack_Core_UnpackFile(xpk, iPos, sFile);
	return pInfo;
}

// 解包数据（Win32方式）
XXAPI xPack_FileInfo_Win32* xPack_Win32_UnpackData(xPackObject xpk, str sPath, ptr* pData)
{
	if ( xpk == 0 ) { return 0; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Win32 ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_Win32ToPos(xpk, sPath);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	xPack_FileInfo_Win32* pInfo = (ptr)xPack_Core_UnpackData(xpk, iPos, pData);
	return pInfo;
}

// 删除文件（Win32方式）
XXAPI int xPack_Win32_DeleteFile(xPackObject xpk, str sPath)
{
	if ( xpk == 0 ) { return 0; }
	if ( (xpk->PackHead.PackFlag & XPK_CLASS_MASK) != XPK_CLASS_Win32 ) { xPack_OnError_Router(xpk, 11); xProc_OnError_Router(11); }
	uint iPos = xPack_Win32ToPos(xpk, sPath);
	if ( iPos == 0 ) { xPack_OnError_Router(xpk, 12); xProc_OnError_Router(12); }
	return xPack_Index_DeleteFile(xpk, iPos);
}


