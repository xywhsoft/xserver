/* Auto-generated single header from xPack source tree. */
#ifndef XPACK_SINGLE_HEADER
#define XPACK_SINGLE_HEADER

/* Usage:
	#include "xrt.h"    // external dependency, provide declarations or implementation yourself
	#define XPACK_IMPLEMENTATION
	#include "xpack.h"

	Compile and link external dependencies yourself:
	- xrt
	- lz4 / zstd / lzma headers and libraries
*/

#if defined(XPACK_IMPLEMENTATION)
#if defined(__has_include)
#	if __has_include("xrt.h")
#		include "xrt.h"
#	elif __has_include("../lib/xrt.h")
#		include "../lib/xrt.h"
#	elif __has_include("lib/xrt.h")
#		include "lib/xrt.h"
#	else
#		error "xpack singlehead requires external xrt.h in include path"
#	endif
#else
#	include "xrt.h"
#endif
#	define XPACK_BUILD_CORE
#if defined(__has_include)
#	if __has_include("lz4.h")
#		include "lz4.h"
#	elif __has_include("../lib/lz4/lz4.h")
#		include "../lib/lz4/lz4.h"
#	else
#		error "missing third-party header: lz4.h"
#	endif
#	if __has_include("lz4hc.h")
#		include "lz4hc.h"
#	elif __has_include("../lib/lz4/lz4hc.h")
#		include "../lib/lz4/lz4hc.h"
#	else
#		error "missing third-party header: lz4hc.h"
#	endif
#	if __has_include("zstd.h")
#		include "zstd.h"
#	elif __has_include("../lib/zstd/zstd.h")
#		include "../lib/zstd/zstd.h"
#	else
#		error "missing third-party header: zstd.h"
#	endif
#	if __has_include("Alloc.h")
#		include "Alloc.h"
#	elif __has_include("../lib/lzma/Alloc.h")
#		include "../lib/lzma/Alloc.h"
#	else
#		error "missing third-party header: Alloc.h"
#	endif
#	if __has_include("Lzma2Dec.h")
#		include "Lzma2Dec.h"
#	elif __has_include("../lib/lzma/Lzma2Dec.h")
#		include "../lib/lzma/Lzma2Dec.h"
#	else
#		error "missing third-party header: Lzma2Dec.h"
#	endif
#	if __has_include("Lzma2Enc.h")
#		include "Lzma2Enc.h"
#	elif __has_include("../lib/lzma/Lzma2Enc.h")
#		include "../lib/lzma/Lzma2Enc.h"
#	else
#		error "missing third-party header: Lzma2Enc.h"
#	endif
#else
#	include "../lib/lz4/lz4.h"
#	include "../lib/lz4/lz4hc.h"
#	include "../lib/zstd/zstd.h"
#	include "../lib/lzma/Alloc.h"
#	include "../lib/lzma/Lzma2Dec.h"
#	include "../lib/lzma/Lzma2Enc.h"
#endif
#endif

/* ===== File: xpack.h ===== */

/*
	xPack 公开头文件

	负责声明 xPack 的公开常量、数据结构与 API，
	同时在 XPACK_BUILD_CORE 模式下暴露内部实现共享的核心声明。
*/

#ifndef XPACK_H
#define XPACK_H

#include <stdint.h>


// 统一时间类型，和 xrt 的 xtime 保持兼容
#ifndef XPK_XTIME_DEFINED
#define XPK_XTIME_DEFINED
typedef int64_t xtime;
#endif


#ifdef __cplusplus
extern "C" {
#endif


// 动态库导出宏
#if defined(_WIN32) || defined(_WIN64)
	#if defined(XPK_BUILD_DLL)
		#define XPKAPI __declspec(dllexport)
	#elif defined(XPK_USE_DLL)
		#define XPKAPI __declspec(dllimport)
	#else
		#define XPKAPI
	#endif
#else
	#define XPKAPI
#endif


// 文件头与布局常量
#define XPK_FILE_HEAD        0x116B7078u
#define XPK_HEAD_SIZE        64u
#define XPK_VOLUME_MIN       0x00010000u
#define XPK_VOLUME_MAX       0xFFFFFFFFu
#define XPK_PATH_BYTES       260u
#define XPK_PATH_ENTRY_SIZE  320u


// 文件类型标记
#define XPK_TYPE_CORE        0u
#define XPK_TYPE_INDEX       1u
#define XPK_TYPE_LINUX       2u
#define XPK_TYPE_WIN32       3u


// 压缩算法标记
#define XPK_ALG_STORE        0u
#define XPK_ALG_LZ4          1u
#define XPK_ALG_LZ4HC        2u
#define XPK_ALG_ZSTD         3u
#define XPK_ALG_LZMA2        4u


// 条目标记掩码
#define XPK_FLAG_COMP_MASK     0x0000000Fu
#define XPK_FLAG_TYPE_MASK     0x000000F0u
#define XPK_FLAG_DELETED_MASK  0x00000100u


// 包对象句柄
typedef struct xpkStruct* xpkObject;


// 包类型
typedef enum xpkPackType {
	XPK_PACK_CORE = 0,
	XPK_PACK_INDEX = 1,
	XPK_PACK_LINUX = 2,
	XPK_PACK_WIN32 = 3
} xpkPackType;


// 写入策略
typedef enum xpkWritePolicy {
	XPK_WRITE_BUFFERED = 0,
	XPK_WRITE_IMMEDIATE = 1
} xpkWritePolicy;


// 错误码
typedef enum xpkErrorCode {
	XPK_OK = 0,
	XPK_ERR_PARAM = -1,
	XPK_ERR_STATE = -2,
	XPK_ERR_MEMORY = -3,
	XPK_ERR_IO = -4,
	XPK_ERR_FORMAT = -5,
	XPK_ERR_HASH = -6,
	XPK_ERR_NOT_FOUND = -7,
	XPK_ERR_EXISTS = -8,
	XPK_ERR_SOLID_DATA_WRITE = -9,
	XPK_ERR_READONLY = -10,
	XPK_ERR_UNSUPPORTED = -11
} xpkErrorCode;


// 打开选项
typedef struct xpkOpenOptions {
	uint8_t readonly;         // 是否按只读模式打开
	uint8_t createIfMissing;  // 目标不存在时是否创建空包
	uint8_t bufferedDefault;  // 默认写入策略是否使用缓冲模式
	uint8_t reserved0;        // 保留字段
} xpkOpenOptions;


// 写入选项
typedef struct xpkWriteOptions {
	uint8_t compLevel;    // 压缩级别
	uint8_t writePolicy;  // 写入策略
	uint8_t fileType;     // 条目类型
	uint8_t reserved0;    // 保留字段
} xpkWriteOptions;


// 重构选项
typedef struct xpkBuildOptions {
	const char* tempPath;      // 临时输出路径
	uint8_t replaceOriginal;   // 是否替换原始包
	uint8_t reserved0[7];      // 保留字段
} xpkBuildOptions;


// 包统计信息
typedef struct xpkStat {
	uint32_t fileCount;       // 可见条目数量
	uint64_t liveDataBytes;   // 有效数据大小
	uint64_t holeBytes;       // 空洞大小
	uint64_t metaBytes;       // 元数据大小
	uint64_t entryTableBytes; // 条目表大小
} xpkStat;


// 对齐到磁盘布局的包头结构
#pragma pack(push, 1)
typedef struct {
	uint32_t fileHead;       // 包魔数
	uint32_t fileCount;      // 条目数量

	uint32_t packType    : 2;   // 包类型
	uint32_t defComp     : 4;   // 默认数据压缩级别
	uint32_t metaComp    : 4;   // 元数据压缩级别
	uint32_t infoComp    : 4;   // 条目表压缩级别
	uint32_t infoExtSize : 18;  // Core 扩展信息大小

	uint32_t solidMode   : 1;   // 是否启用 solid 布局
	uint32_t volumeMode  : 1;   // 是否启用分卷布局
	uint32_t reserved1   : 30;  // 预留位

	uint64_t dataOffset;     // 数据区起始逻辑偏移
	uint32_t volumeSize;     // 分卷大小

	uint32_t metaRawSize;    // 元数据原始大小
	uint32_t metaCompSize;   // 元数据压缩后大小
	uint32_t metaHash;       // 元数据哈希

	uint32_t infoCompSize;   // 条目表压缩后大小
	uint32_t infoHash;       // 条目表哈希

	xtime createTime;        // 创建时间
	xtime changeTime;        // 最后修改时间
} xpkHead;


// Core 模式下的基础条目信息
typedef struct {
	uint32_t flag;          // 条目标记
	uint32_t fileHash;      // 文件内容哈希
	uint64_t dataOffset;    // 数据偏移
	uint64_t dataSize;      // 存储大小
	uint64_t fileSize;      // 原始大小
} xpkFileInfo;


// Index 模式下的条目信息
typedef struct {
	uint32_t flag;          // 条目标记
	uint32_t fileHash;      // 文件内容哈希
	uint64_t dataOffset;    // 数据偏移
	uint64_t dataSize;      // 存储大小
	uint64_t fileSize;      // 原始大小
	int64_t fileIndex;      // 逻辑索引号
} xpkFileInfoIndex;


// Path 模式下的条目信息
typedef struct {
	uint32_t flag;                     // 条目标记
	uint32_t fileHash;                 // 文件内容哈希
	uint64_t dataOffset;               // 数据偏移
	uint64_t dataSize;                 // 存储大小
	uint64_t fileSize;                 // 原始大小
	char pathBytes[XPK_PATH_BYTES];    // 包内路径
	uint32_t platformAttr;             // 平台属性
	uint64_t createTime;               // 创建时间
	uint64_t modifyTime;               // 修改时间
	uint64_t accessTime;               // 访问时间
} xpkFileInfoPath;
#pragma pack(pop)


// 布局静态断言
typedef char xpkStaticAssertHeadSize[(sizeof(xpkHead) == XPK_HEAD_SIZE) ? 1 : -1];
typedef char xpkStaticAssertFileInfoSize[(sizeof(xpkFileInfo) == 32u) ? 1 : -1];
typedef char xpkStaticAssertFileInfoIndexSize[(sizeof(xpkFileInfoIndex) == 40u) ? 1 : -1];
typedef char xpkStaticAssertFileInfoPathSize[(sizeof(xpkFileInfoPath) == XPK_PATH_ENTRY_SIZE) ? 1 : -1];


// 遍历回调
typedef int (*xpkEachProc)(xpkObject xpk, uint32_t pos, const void* info, void* userData);


// 包对象管理
// 打开包并创建运行时对象
XPKAPI xpkObject xpkOpen(const char* packagePath, const xpkOpenOptions* options);

// 关闭包对象并释放运行时资源
XPKAPI int xpkClose(xpkObject xpk);

// 将当前脏状态保存回包文件
XPKAPI int xpkSave(xpkObject xpk);

// 按目标布局重构包文件
XPKAPI int xpkBuild(xpkObject xpk, const xpkBuildOptions* options);


// 包级配置
// 获取当前包类型
XPKAPI int xpkGetPackType(xpkObject xpk, xpkPackType* outType);

// 设置当前包类型
XPKAPI int xpkSetPackType(xpkObject xpk, xpkPackType type);

// 获取默认数据压缩级别
XPKAPI int xpkGetDefaultComp(xpkObject xpk, uint8_t* outLevel);

// 设置默认数据压缩级别
XPKAPI int xpkSetDefaultComp(xpkObject xpk, uint8_t level);

// 获取元数据压缩级别
XPKAPI int xpkGetMetaComp(xpkObject xpk, uint8_t* outLevel);

// 设置元数据压缩级别
XPKAPI int xpkSetMetaComp(xpkObject xpk, uint8_t level);

// 获取条目表压缩级别
XPKAPI int xpkGetInfoComp(xpkObject xpk, uint8_t* outLevel);

// 设置条目表压缩级别
XPKAPI int xpkSetInfoComp(xpkObject xpk, uint8_t level);

// 获取条目信息扩展大小
XPKAPI int xpkGetInfoExtSize(xpkObject xpk, uint32_t* outSize);

// 设置条目信息扩展大小
XPKAPI int xpkSetInfoExtSize(xpkObject xpk, uint32_t size);

// 获取分卷大小
XPKAPI int xpkGetVolumeSize(xpkObject xpk, uint32_t* outSize);

// 设置分卷大小
XPKAPI int xpkSetVolumeSize(xpkObject xpk, uint32_t size);

// 获取 Solid 模式开关
XPKAPI int xpkGetSolidMode(xpkObject xpk, int* outEnabled);

// 设置 Solid 模式开关
XPKAPI int xpkSetSolidMode(xpkObject xpk, int enabled);


// 包元数据
// 获取包元数据缓冲
XPKAPI void* xpkMetaGet(xpkObject xpk, uint32_t* outSize);

// 设置包元数据缓冲
XPKAPI int xpkMetaSet(xpkObject xpk, const void* data, uint32_t size, uint8_t compLevel);

// 清空包元数据
XPKAPI int xpkMetaClear(xpkObject xpk);


// 按位置访问条目
// 获取可见条目数量
XPKAPI uint32_t xpkCount(xpkObject xpk);

// 获取指定位置的基础条目信息
XPKAPI int xpkGetInfo(xpkObject xpk, uint32_t pos, xpkFileInfo* outInfo);

// 获取指定位置的扩展信息
XPKAPI int xpkGetInfoExt(xpkObject xpk, uint32_t pos, void* outData, uint32_t size);

// 设置指定位置的扩展信息
XPKAPI int xpkSetInfoExt(xpkObject xpk, uint32_t pos, const void* data, uint32_t size);

// 在指定位置添加文件条目
XPKAPI int xpkAddFile(xpkObject xpk, const char* srcPath, const xpkWriteOptions* options, uint32_t* outPos);

// 在指定位置添加内存数据条目
XPKAPI int xpkAddData(xpkObject xpk, const void* data, uint64_t size, const xpkWriteOptions* options, uint32_t* outPos);

// 将指定位置条目导出到文件
XPKAPI int xpkReadToFile(xpkObject xpk, uint32_t pos, const char* dstPath);

// 将指定位置条目读入内存
XPKAPI void* xpkReadToMemory(xpkObject xpk, uint32_t pos, uint64_t* outSize);

// 用文件内容更新指定位置条目
XPKAPI int xpkUpdateFile(xpkObject xpk, uint32_t pos, const char* srcPath, const xpkWriteOptions* options);

// 用内存数据更新指定位置条目
XPKAPI int xpkUpdateData(xpkObject xpk, uint32_t pos, const void* data, uint64_t size, const xpkWriteOptions* options);

// 移除指定位置条目
XPKAPI int xpkRemove(xpkObject xpk, uint32_t pos);

// 修改指定位置条目标记
XPKAPI int xpkSetFlag(xpkObject xpk, uint32_t pos, uint32_t mask, uint32_t value);


// 按索引访问条目
// 通过 fileIndex 查找条目位置
XPKAPI int xpkIndexFind(xpkObject xpk, int64_t fileIndex, uint32_t* outPos);

// 通过 fileIndex 获取条目信息
XPKAPI int xpkIndexGetInfo(xpkObject xpk, int64_t fileIndex, xpkFileInfoIndex* outInfo);

// 通过 fileIndex 添加文件条目
XPKAPI int xpkIndexAddFile(xpkObject xpk, int64_t fileIndex, const char* srcPath, const xpkWriteOptions* options);

// 通过 fileIndex 添加内存数据条目
XPKAPI int xpkIndexAddData(xpkObject xpk, int64_t fileIndex, const void* data, uint64_t size, const xpkWriteOptions* options);

// 通过 fileIndex 导出条目到文件
XPKAPI int xpkIndexReadToFile(xpkObject xpk, int64_t fileIndex, const char* dstPath);

// 通过 fileIndex 读取条目到内存
XPKAPI void* xpkIndexReadToMemory(xpkObject xpk, int64_t fileIndex, uint64_t* outSize);

// 通过 fileIndex 更新文件条目
XPKAPI int xpkIndexUpdateFile(xpkObject xpk, int64_t fileIndex, const char* srcPath, const xpkWriteOptions* options);

// 通过 fileIndex 更新内存数据条目
XPKAPI int xpkIndexUpdateData(xpkObject xpk, int64_t fileIndex, const void* data, uint64_t size, const xpkWriteOptions* options);

// 通过 fileIndex 移除条目
XPKAPI int xpkIndexRemove(xpkObject xpk, int64_t fileIndex);

// 通过 fileIndex 修改条目标记
XPKAPI int xpkIndexSetFlag(xpkObject xpk, int64_t fileIndex, uint32_t mask, uint32_t value);


// 按路径访问条目
// 判断包内路径是否存在
XPKAPI int xpkPathExists(xpkObject xpk, const char* packagePath);

// 获取包内路径对应的条目信息
XPKAPI int xpkPathGetInfo(xpkObject xpk, const char* packagePath, xpkFileInfoPath* outInfo);

// 按包内路径添加文件条目
XPKAPI int xpkPathAddFile(xpkObject xpk, const char* packagePath, const char* srcPath, const xpkWriteOptions* options);

// 按包内路径添加内存数据条目
XPKAPI int xpkPathAddData(xpkObject xpk, const char* packagePath, const void* data, uint64_t size, const xpkWriteOptions* options);

// 按包内路径导出条目到文件
XPKAPI int xpkPathReadToFile(xpkObject xpk, const char* packagePath, const char* dstPath);

// 按包内路径读取条目到内存
XPKAPI void* xpkPathReadToMemory(xpkObject xpk, const char* packagePath, uint64_t* outSize);

// 按包内路径更新文件条目
XPKAPI int xpkPathUpdateFile(xpkObject xpk, const char* packagePath, const char* srcPath, const xpkWriteOptions* options);

// 按包内路径更新内存数据条目
XPKAPI int xpkPathUpdateData(xpkObject xpk, const char* packagePath, const void* data, uint64_t size, const xpkWriteOptions* options);

// 重命名包内路径
XPKAPI int xpkPathRename(xpkObject xpk, const char* oldPath, const char* newPath);

// 按包内路径移除条目
XPKAPI int xpkPathRemove(xpkObject xpk, const char* packagePath);

// 按包内路径设置平台属性
XPKAPI int xpkPathSetAttr(xpkObject xpk, const char* packagePath, uint32_t platformAttr);


// 遍历与筛选
// 遍历全部可见条目
XPKAPI int xpkEach(xpkObject xpk, xpkEachProc proc, void* userData);

// 按模式遍历匹配条目
XPKAPI int xpkEachMatch(xpkObject xpk, const char* pattern, xpkEachProc proc, void* userData);


// 校验、统计与基础工具
// 校验单个条目内容
XPKAPI int xpkVerify(xpkObject xpk, uint32_t pos);

// 校验全部条目内容
XPKAPI int xpkVerifyAll(xpkObject xpk);

// 获取当前包统计信息
XPKAPI int xpkStatGet(xpkObject xpk, xpkStat* outStat);

// 释放 xPack 分配的返回内存
XPKAPI void xpkFree(void* ptr);

// 计算 32 位哈希值
XPKAPI uint32_t xpkHash32(const void* data, uint64_t size);


// 错误状态
// 获取最后错误码
XPKAPI xpkErrorCode xpkLastError(xpkObject xpk);

// 获取最后错误文本
XPKAPI const char* xpkLastErrorMessage(xpkObject xpk);


#ifdef __cplusplus
}
#endif


#if defined(XPACK_BUILD_CORE)

/*
	xPack 内部共享声明

	该部分只在实现编译单元中可见，
	要求在包含前先准备好 xrt 与压缩库头文件。
*/


// 错误文本缓存大小
#define XPK_ERROR_TEXT_CAP 256


// 未使用参数标记
#if defined(__GNUC__) || defined(__clang__)
	#define XPK_UNUSED __attribute__((unused))
#else
	#define XPK_UNUSED
#endif


// 内部布局常量
enum {
	XPK_INTERNAL_ERROR_TEXT_CAP = XPK_ERROR_TEXT_CAP,
	XPK_ENTRY_BASE_SIZE = 32,
	XPK_INFO_EXT_INDEX = 8,
	XPK_INFO_EXT_PATH = 288
};


// 内部前置声明
typedef struct xpkErrorState xpkErrorState;
typedef struct xpkEntry xpkEntry;
typedef struct xpkStorage xpkStorage;
typedef struct xpkWriteQueue xpkWriteQueue;


// 错误状态
struct xpkErrorState {
	int iCode;                      // 错误码
	char sText[XPK_ERROR_TEXT_CAP]; // 错误文本
};


// 运行时条目对象
struct xpkEntry {
	uint32_t iPos;              // 条目位置
	uint32_t iFlag;             // 条目标记
	uint32_t iFileHash;         // 文件哈希
	uint64_t iDataOffset;       // 数据偏移
	uint64_t iDataSize;         // 存储大小
	uint64_t iFileSize;         // 原始大小
	uint8_t bStored;            // 是否为原样存储

	int64_t iFileIndex;         // Index 模式下的索引号

	void* pInfoExt;             // Core 模式扩展信息
	char* sPath;                // Path 模式路径
	uint32_t iPlatformAttr;     // 平台属性
	uint64_t tCreateTime;       // 创建时间
	uint64_t tModifyTime;       // 修改时间
	uint64_t tAccessTime;       // 访问时间
};


// 运行时包对象
struct xpkStruct {
	xpkHead objHead;                // 当前包头快照

	uint8_t bReadonly;              // 是否只读
	uint8_t bBufferedDefault;       // 默认是否缓冲写入
	uint8_t bDirtyHead;             // 包头是否脏
	uint8_t bDirtyPackageMeta;      // 元数据是否脏
	uint8_t bDirtyEntryTable;       // 条目表是否脏
	uint8_t bDirtyData;             // 数据区是否脏
	uint8_t bSolidApplied;          // 已应用的 solid 布局
	uint8_t bVolumeApplied;         // 已应用的分卷布局
	uint16_t reserved0;             // 保留字段
	uint32_t iVolumeSizeApplied;    // 已应用的分卷大小

	uint64_t iAppendPos;            // 当前追加写入位置
	uint64_t iFileSize;             // 当前逻辑文件大小

	char* sPathPackage;             // 包路径
	void* pPackageMeta;             // 包元数据缓冲
	uint32_t iPackageMetaSize;      // 包元数据大小

	xarray_struct arrEntry;         // 条目数组
	xlist_struct lstEntry;          // 条目链表
	xdict_struct tblEntry;          // 路径查找表
	uint32_t iEntryCount;           // 条目数量

	xpkWriteQueue* pWriteQueue;     // 延迟写入队列
	xpkStorage* pStorage;           // 存储适配层
	xpkErrorState err;              // 对象级错误状态
};


// 脏位标记
typedef enum xpkDirtyBits {
	XPK_DIRTY_NONE = 0,
	XPK_DIRTY_HEAD = 1 << 0,
	XPK_DIRTY_PACKAGE_META = 1 << 1,
	XPK_DIRTY_ENTRY_TABLE = 1 << 2,
	XPK_DIRTY_DATA = 1 << 3
} xpkDirtyBits;


// 压缩级别映射
typedef struct xpkCompMap {
	uint32_t iAlgorithm;  // 目标算法
	int iNativeLevel;     // 对应原生级别
} xpkCompMap;


// 文件映射对象
typedef struct xpkMappedFile {
	const void* pView;    // 映射视图
	uint64_t iSize;       // 映射大小
#if defined(_WIN32) || defined(_WIN64)
	HANDLE hMap;          // Windows 映射句柄
#else
	void* pMap;           // POSIX 映射句柄
#endif
} xpkMappedFile;


// 松散分卷扫描上下文
typedef struct xpkLooseVolumeScan {
	xpkObject objXpk;         // 包对象
	const char* sNameBase;    // 基础文件名
	size_t iNameSize;         // 基础文件名长度
	uint32_t iStartVolume;    // 起始分卷号
	uint32_t iCount;          // 命中数量
	int bDelete;              // 是否删除命中项
	int iError;               // 扫描过程错误码
} xpkLooseVolumeScan;


// 延迟写入节点
typedef struct xpkWriteNode {
	uint32_t iPos;            // 条目位置
	uint8_t iLevel;           // 压缩级别
	uint8_t reserved0[3];     // 保留字段
	uint32_t iCompSize;       // 压缩后大小
	uint64_t iRawSize;        // 原始大小
	void* pCompData;          // 压缩数据缓冲
} xpkWriteNode;


// 延迟写入队列
struct xpkWriteQueue {
	xarray_struct arrNode;    // 队列节点数组
};


// 文件写入分块大小
#define XPK_WRITE_FILE_CHUNK_SIZE (8u * 1024u * 1024u)

// 编解码流分块大小
#define XPK_CODEC_STREAM_CHUNK_SIZE (256u * 1024u)


// Flush 前的条目快照
typedef struct xpkFlushSnapshot {
	uint32_t iPos;            // 条目位置
	uint32_t iFlag;           // 条目标记
	uint64_t iDataOffset;     // 数据偏移
	uint64_t iDataSize;       // 存储大小
	uint64_t iFileSize;       // 原始大小
} xpkFlushSnapshot;


// 保存回滚上下文
typedef struct xpkSaveRollback {
	int bFileExisted;               // 保存前文件是否存在
	int bHeadValid;                 // 是否捕获到有效包头
	int bTailInFile;                // 尾段是否落到临时文件
	int bKeepTailFile;              // 是否保留尾段文件
	uint64_t iLogicalSize;          // 保存前逻辑大小
	uint64_t iDataOffset;           // 保存前数据起点
	uint64_t iTailSize;             // 尾段大小
	uint8_t sHeadBuf[XPK_HEAD_SIZE]; // 保存前包头镜像
	void* pTailData;                // 内存尾段缓冲
	char* sTailPath;                // 临时尾段路径
} xpkSaveRollback;


// 回滚内存上限
#define XPK_SAVE_ROLLBACK_MEM_LIMIT   (8u * 1024u * 1024u)

// 回滚文件分块大小
#define XPK_SAVE_ROLLBACK_CHUNK_SIZE  (8u * 1024u * 1024u)


// 构建阶段用 LZMA 输入流
typedef struct xpkBuildLzmaSeqIn {
	ISeqInStream vt;   // LZMA 输入接口
	xfile hFile;       // 源文件句柄
	uint64_t iRemain;  // 剩余字节数
} xpkBuildLzmaSeqIn;


// 构建阶段用 LZMA 输出流
typedef struct xpkBuildLzmaSeqOut {
	ISeqOutStream vt;  // LZMA 输出接口
	xfile hFile;       // 目标文件句柄
	uint64_t iSize;    // 已写大小
} xpkBuildLzmaSeqOut;


// 写入阶段用 LZMA 输入流
typedef struct xpkWriteLzmaSeqIn {
	ISeqInStream vt;   // LZMA 输入接口
	xfile hFile;       // 源文件句柄
	uint64_t iRemain;  // 剩余字节数
} xpkWriteLzmaSeqIn;


// 写入阶段用 LZMA 输出流
typedef struct xpkWriteLzmaSeqOut {
	ISeqOutStream vt;  // LZMA 输出接口
	xfile hFile;       // 目标文件句柄
	uint64_t iSize;    // 已写大小
} xpkWriteLzmaSeqOut;


// 写入阶段用 LZMA 内存输入流
typedef struct xpkWriteLzmaMemIn {
	ISeqInStream vt;      // LZMA 输入接口
	const uint8_t* pData; // 源数据缓冲
	uint64_t iSize;       // 源数据大小
	uint64_t iPos;        // 当前读取位置
} xpkWriteLzmaMemIn;


// 获取写入队列节点数量
static inline int procXpkWriteQueueCount(xpkObject objXpk);

// 释放写入队列
static inline void procXpkUnitWriteQueue(xpkObject objXpk);

// 标记对象为干净状态
static inline void procXpkMarkClean(xpkObject objXpk);

// 初始化默认包头
static inline void procXpkInitHead(xpkObject objXpk);

// 标记已应用布局
static inline void procXpkMarkAppliedLayout(xpkObject objXpk);

// 添加内存数据条目
static inline int procXpkAddDataEntry(xpkObject objXpk, xpkEntry* pEntrySeed, const void* pData, uint64_t iSize, const xpkWriteOptions* pOpt, uint32_t* pPosRet);

// 校验条目数量状态
static inline int procXpkValidateEntryCountState(xpkObject objXpk);

// 校验活动条目查找一致性
static inline int procXpkValidateLiveEntryLookup(xpkObject objXpk, const xpkEntry* pEntry);

// 仅追加构建后的条目描述
static inline int procXpkBuildAppendEntryOnly(xpkObject objDst, const xpkEntry* pEntrySrc, uint64_t iDataOffset, uint64_t iDataSize, uint64_t iFileSize);


// 获取 solid 原样存储级别
static inline uint8_t procXpkSolidStoredCompLevel(xpkObject objXpk);

// 计算 solid 原始数据大小
static inline int procXpkCalcSolidRawSize(xpkObject objXpk, uint64_t* pSizeRet);

// 计算映射文件范围哈希
static inline int procXpkHashMappedFileRange(xpkObject objXpk, xfile hFile, uint64_t iOffset, uint64_t iSize, uint32_t* pHashRet);

// 将 LZ4 解码块复制到文件
static inline int procXpkCopyDecodedLz4BlockToFile(xpkObject objXpk, uint8_t iLevel, const void* pCompData, uint32_t iCompSize, uint64_t iRawSize, const char* sPathFile);

// 立即写入原样文件
static inline int procXpkWriteImmediateStoreFile(xpkObject objXpk, xpkEntry* pEntry, const char* sSrcPath);

// 立即写入 LZ4 数据
static inline int procXpkWriteImmediateLz4Data(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel);

// 立即写入 ZSTD 数据
static inline int procXpkWriteImmediateZstdData(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel);

// 立即写入 LZMA2 数据
static inline int procXpkWriteImmediateLzma2Data(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel);

// 缓冲写入 LZ4 数据
static inline int procXpkWriteBufferedLz4Data(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel);

// 缓冲写入 ZSTD 数据
static inline int procXpkWriteBufferedZstdData(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel);

// 缓冲写入 LZMA2 数据
static inline int procXpkWriteBufferedLzma2Data(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel);


// 基于文件句柄校验原样条目
static inline int procXpkVerifyStoredEntryWithFile(xpkObject objXpk, xpkEntry* pEntry, xfile hFile);

// 基于映射校验原样条目
static inline int procXpkVerifyStoredEntryMapped(xpkObject objXpk, xpkEntry* pEntry, const xpkMappedFile* pMap);

// 基于文件句柄校验解码条目
static inline int procXpkVerifyDecodedEntryWithFile(xpkObject objXpk, xpkEntry* pEntry, xfile hFile);

// 基于映射校验解码条目
static inline int procXpkVerifyDecodedEntryMapped(xpkObject objXpk, xpkEntry* pEntry, const xpkMappedFile* pMap);

#endif

#endif


#if defined(XPACK_IMPLEMENTATION)

/* ===== File: src/base/memory.h ===== */

/*
	xPack 内部内存适配模块

	负责统一封装内部申请与释放接口。
*/

// 分配内部内存
static inline void* xpkAllocInternal(size_t size)
{
	return xrtMalloc(size);
}

// 释放内部内存
static inline void xpkFreeInternal(void* ptr)
{
	xrtFree(ptr);
}


/* ===== File: src/base/hash.h ===== */

/*
	xPack 内部哈希适配模块

	负责统一封装数据哈希接口。
*/

// 计算内部 32 位哈希值
static inline uint32_t xpkHash32Internal(const void* data, uint64_t size)
{
	return xrtHash32((ptr)data, (size_t)size);
}


/* ===== File: src/base/time.h ===== */

/*
	xPack 内部时间适配模块

	负责统一封装当前时间获取接口。
*/

// 获取当前内部时间
static inline xtime xpkNowInternal(void)
{
	return xrtNow();
}


/* ===== File: src/service/core.h ===== */

/*
	xPack 核心基础服务模块

	负责错误状态、路径归一化、条目管理、查找表维护与对象生命周期。
*/

#include <ctype.h>

static const char sXpkErrorInvalidObject[] = "invalid xpk object";
static const char sXpkErrorInvalidParam[] = "invalid parameter";
static const char sXpkErrorOutOfMemory[] = "out of memory";
static const char sXpkErrorNotImplemented[] = "function is not implemented yet";
static const char sXpkErrorNotFound[] = "target entry not found";
static const char sXpkErrorReadonly[] = "package is readonly";
static const char sXpkErrorIoOpen[] = "failed to open package file";
static const char sXpkErrorIoRead[] = "failed to read package file";
static const char sXpkErrorIoWrite[] = "failed to write package file";
static const char sXpkErrorIoSeek[] = "failed to seek package file";
static const char sXpkErrorBadHead[] = "invalid xpk file header";
static const char sXpkErrorBadFormat[] = "invalid xpk package format";
static const char sXpkErrorHashMismatch[] = "data hash verification failed";
static const char sXpkErrorVolumeUnsupported[] = "volume mode is not implemented yet";
static const char sXpkErrorCodecUnsupported[] = "compression level is not implemented yet";
static const char sXpkErrorSeekRange[] = "file offset exceeds current seek range";
static const char sXpkErrorBlockTooLarge[] = "single data block exceeds current xpk block limit";
static const char sXpkErrorCoreExtUnsupported[] = "custom core infoExtSize is not enabled";
static const char sXpkErrorPackTypeLocked[] = "pack type can only be changed on empty package";
static const char sXpkErrorBufferedPending[] = "buffered writes are pending";
static const char sXpkErrorExists[] = "target entry already exists";
static const char sXpkErrorDeleted[] = "target entry has been deleted";
static const char sXpkErrorPackTypeMismatch[] = "pack type does not match this api";
static const char sXpkErrorSolidBuildRequired[] = "solid mode change requires xpkBuild";
static const char sXpkErrorVolumeBuildRequired[] = "volume layout change requires xpkBuild";
static const char sXpkErrorInfoExtLocked[] = "infoExtSize can only be changed on empty core package";
static const char sXpkErrorInfoExtSizeMismatch[] = "infoExt buffer size does not match current infoExtSize";
static const char sXpkErrorPathTooLong[] = "package path exceeds 259 bytes";
static const char sXpkErrorPathEmpty[] = "package path is empty";
static const char sXpkErrorCompManaged[] = "compression flag is managed by xpk";
static const char sXpkErrorTempPathExists[] = "tempPath already exists";
static const char sXpkErrorBackupPathExists[] = "backup path is occupied";
static const char sXpkErrorPackagePathExists[] = "package path is occupied";
static XRT_TLS_STORAGE xpkErrorState g_objXpkErrorTls = { XPK_OK, { 0 } };

// 设置线程错误状态
static inline void procXpkSetThreadError(int iCode, const char* sText)
{
	size_t iSizeText;

	g_objXpkErrorTls.iCode = iCode;
	if ( sText == NULL ) {
		g_objXpkErrorTls.sText[0] = '\0';
		return;
	}

	iSizeText = strlen(sText);
	if ( iSizeText >= XPK_ERROR_TEXT_CAP ) {
		iSizeText = XPK_ERROR_TEXT_CAP - 1;
	}
	memcpy(g_objXpkErrorTls.sText, sText, iSizeText);
	g_objXpkErrorTls.sText[iSizeText] = '\0';
}

// 清理错误状态
static inline void procXpkClearError(xpkObject objXpk)
{
	procXpkSetThreadError(XPK_OK, NULL);
	if ( objXpk != NULL ) {
		objXpk->err.iCode = XPK_OK;
		objXpk->err.sText[0] = '\0';
	}
}

// 设置错误状态
static inline int procXpkSetError(xpkObject objXpk, int iCode, const char* sText)
{
	size_t iSizeText;

	procXpkSetThreadError(iCode, sText);
	if ( objXpk == NULL ) {
		return iCode;
	}

	objXpk->err.iCode = iCode;
	if ( sText == NULL ) {
		objXpk->err.sText[0] = '\0';
		return iCode;
	}

	iSizeText = strlen(sText);
	if ( iSizeText >= XPK_ERROR_TEXT_CAP ) {
		iSizeText = XPK_ERROR_TEXT_CAP - 1;
	}
	memcpy(objXpk->err.sText, sText, iSizeText);
	objXpk->err.sText[iSizeText] = '\0';
	return iCode;
}

// 返回参数错误
static inline int procXpkReturnParamError(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return procXpkSetError(NULL, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
}

// 确保对象可写
static inline int procXpkEnsureWritable(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	return XPK_OK;
}

// 在对象有效时设置参数错误
static inline void procXpkSetParamErrorIfObject(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return;
	}
	procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
}

// 复制指定长度文本
static inline char* procXpkDupTextN(const char* sText, size_t iSizeText)
{
	char* sRet;

	sRet = (char*)xpkAllocInternal(iSizeText + 1);
	if ( sRet == NULL ) {
		return NULL;
	}
	if ( (sText != NULL) && (iSizeText > 0) ) {
		memcpy(sRet, sText, iSizeText);
	}
	sRet[iSizeText] = '\0';
	return sRet;
}

// 复制文本
static inline char* procXpkDupText(const char* sText)
{
	if ( sText == NULL ) {
		return NULL;
	}
	return procXpkDupTextN(sText, strlen(sText));
}

// 判断 Core 信息扩展是否启用
static inline int procXpkCoreInfoExtEnabled(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return FALSE;
	}
	if ( objXpk->objHead.packType != XPK_PACK_CORE ) {
		return FALSE;
	}
	return (objXpk->objHead.infoExtSize > 0) ? TRUE : FALSE;
}

// 分配信息扩展缓冲
static inline void* procXpkAllocInfoExt(xpkObject objXpk)
{
	void* pInfoExt;

	if ( !procXpkCoreInfoExtEnabled(objXpk) ) {
		return NULL;
	}

	pInfoExt = xpkAllocInternal(objXpk->objHead.infoExtSize);
	if ( pInfoExt == NULL ) {
		return NULL;
	}
	memset(pInfoExt, 0, objXpk->objHead.infoExtSize);
	return pInfoExt;
}

// 复制信息扩展缓冲
static inline void* procXpkDupInfoExt(xpkObject objXpk, const void* pInfoExtSrc)
{
	void* pInfoExt;

	if ( !procXpkCoreInfoExtEnabled(objXpk) ) {
		return NULL;
	}

	pInfoExt = procXpkAllocInfoExt(objXpk);
	if ( pInfoExt == NULL ) {
		return NULL;
	}
	if ( pInfoExtSrc != NULL ) {
		memcpy(pInfoExt, pInfoExtSrc, objXpk->objHead.infoExtSize);
	}
	return pInfoExt;
}

// 释放条目自有资源
static inline void procXpkFreeEntryOwned(xpkEntry* pEntry)
{
	if ( pEntry == NULL ) {
		return;
	}
	if ( pEntry->pInfoExt != NULL ) {
		xpkFreeInternal(pEntry->pInfoExt);
		pEntry->pInfoExt = NULL;
	}
	if ( pEntry->sPath != NULL ) {
		xpkFreeInternal(pEntry->sPath);
		pEntry->sPath = NULL;
	}
}

// 判断路径是否忽略大小写
static inline int procXpkPathIgnoreCase(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return FALSE;
	}
	return (objXpk->objHead.packType == XPK_PACK_WIN32) ? TRUE : FALSE;
}

// 判断字符是否为路径分隔符
static inline int procXpkPathIsSep(xpkObject objXpk, char ch)
{
	if ( ch == '/' ) {
		return TRUE;
	}
	if ( procXpkPathIgnoreCase(objXpk) && ch == '\\' ) {
		return TRUE;
	}
	return FALSE;
}

// 复制指定长度路径文本
static inline char* procXpkDupPathTextN(xpkObject objXpk, const char* sPath, size_t iSizePath)
{
	size_t iPosDst;
	size_t iPosSrc;
	char* sText;
	int bLastSep;

	if ( sPath == NULL ) {
		return NULL;
	}

	sText = procXpkDupTextN(sPath, iSizePath);
	if ( sText == NULL ) {
		return NULL;
	}

	iPosSrc = 0;
	while ( (iPosSrc + 1) < iSizePath ) {
		if ( sPath[iPosSrc] != '.' ) {
			break;
		}
		if ( !procXpkPathIsSep(objXpk, sPath[iPosSrc + 1]) ) {
			break;
		}

		iPosSrc += 2;
		while ( (iPosSrc < iSizePath) && procXpkPathIsSep(objXpk, sPath[iPosSrc]) ) {
			iPosSrc++;
		}
	}

	iPosDst = 0;
	bLastSep = FALSE;
	for ( ; iPosSrc < iSizePath; iPosSrc++ ) {
		if ( procXpkPathIsSep(objXpk, sPath[iPosSrc]) ) {
			if ( bLastSep ) {
				continue;
			}
			sText[iPosDst++] = '/';
			bLastSep = TRUE;
			continue;
		}

		if ( sPath[iPosSrc] == '.' ) {
			if ( ((iPosDst == 0) || bLastSep) &&
				(((iPosSrc + 1) == iSizePath) || procXpkPathIsSep(objXpk, sPath[iPosSrc + 1])) ) {
				continue;
			}
		}

		sText[iPosDst++] = sPath[iPosSrc];
		bLastSep = FALSE;
	}

	if ( (iPosDst > 1) && (sText[iPosDst - 1] == '/') ) {
		iPosDst--;
	}
	sText[iPosDst] = '\0';
	return sText;
}

// 复制路径文本
static inline char* procXpkDupPathText(xpkObject objXpk, const char* sPath)
{
	if ( sPath == NULL ) {
		return NULL;
	}
	return procXpkDupPathTextN(objXpk, sPath, strlen(sPath));
}

// 复制路径原样文本
static inline char* procXpkDupPathStoredText(xpkObject objXpk, const char* sPath)
{
	char* sPathRet;

	if ( sPath == NULL ) {
		return NULL;
	}

	sPathRet = procXpkDupPathText(objXpk, sPath);
	if ( sPathRet == NULL ) {
		if ( sPath[0] != '\0' ) {
			procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		return NULL;
	}
	if ( strlen(sPathRet) >= XPK_PATH_BYTES ) {
		xpkFreeInternal(sPathRet);
		procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorPathTooLong);
		return NULL;
	}
	if ( sPathRet[0] == '\0' ) {
		xpkFreeInternal(sPathRet);
		procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorPathEmpty);
		return NULL;
	}

	return sPathRet;
}

// 校验原样路径文本
static inline int procXpkValidateStoredPathText(xpkObject objXpk, const char* sPath)
{
	char* sPathDup;

	if ( sPath == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	sPathDup = procXpkDupPathStoredText(objXpk, sPath);
	if ( sPathDup == NULL ) {
		return xpkLastError(objXpk);
	}

	xpkFreeInternal(sPathDup);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 复制路径键
static inline char* procXpkDupPathKey(xpkObject objXpk, const char* sPath)
{
	size_t iPos;
	size_t iSizePath;
	char* sKey;

	sKey = procXpkDupPathText(objXpk, sPath);
	if ( sKey == NULL ) {
		return NULL;
	}
	if ( !procXpkPathIgnoreCase(objXpk) ) {
		return sKey;
	}

	iSizePath = strlen(sKey);
	for ( iPos = 0; iPos < iSizePath; iPos++ ) {
		sKey[iPos] = (char)tolower((unsigned char)sKey[iPos]);
	}
	return sKey;
}

// 复制路径键已校验
static inline char* procXpkDupPathKeyChecked(xpkObject objXpk, const char* sPath)
{
	size_t iPos;
	size_t iSizePath;
	char* sKey;

	sKey = procXpkDupPathStoredText(objXpk, sPath);
	if ( sKey == NULL ) {
		return NULL;
	}
	if ( !procXpkPathIgnoreCase(objXpk) ) {
		return sKey;
	}

	iSizePath = strlen(sKey);
	for ( iPos = 0; iPos < iSizePath; iPos++ ) {
		sKey[iPos] = (char)tolower((unsigned char)sKey[iPos]);
	}
	return sKey;
}

// 查找路径条目
static inline xpkEntry* procXpkLookupPathEntry(xpkObject objXpk, const char* sPath)
{
	char* sKey;
	char* sEntryKey;
	xpkEntry* pMap;
	xpkEntry* pEntry;

	if ( objXpk == NULL || sPath == NULL ) {
		return NULL;
	}

	procXpkClearError(objXpk);
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return NULL;
	}
	sKey = procXpkDupPathKeyChecked(objXpk, sPath);
	if ( sKey == NULL ) {
		return NULL;
	}

	pMap = (xpkEntry*)xrtDictGet(&objXpk->tblEntry, (ptr)sKey, (uint32_t)strlen(sKey));
	if ( pMap == NULL ) {
		uint32_t iPosScan;
		for ( iPosScan = 1; iPosScan <= objXpk->iEntryCount; iPosScan++ ) {
			pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPosScan);
			if ( pEntry == NULL ) {
				if ( sKey != NULL ) {
					xpkFreeInternal(sKey);
				}
				procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
				return NULL;
			}
			if ( ((pEntry->iFlag & XPK_FLAG_DELETED_MASK) != 0) ) {
				continue;
			}
			if ( (pEntry->sPath == NULL) || (pEntry->sPath[0] == '\0') || (strlen(pEntry->sPath) >= XPK_PATH_BYTES) ) {
				if ( sKey != NULL ) {
					xpkFreeInternal(sKey);
				}
				procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
				return NULL;
			}

			sEntryKey = procXpkDupPathKey(objXpk, pEntry->sPath);
			if ( sEntryKey == NULL ) {
				if ( sKey != NULL ) {
					xpkFreeInternal(sKey);
				}
				procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
				return NULL;
			}
			if ( strcmp(sEntryKey, sKey) == 0 ) {
				xpkFreeInternal(sEntryKey);
				if ( sKey != NULL ) {
					xpkFreeInternal(sKey);
				}
				procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
				return NULL;
			}
			xpkFreeInternal(sEntryKey);
		}
		if ( sKey != NULL ) {
			xpkFreeInternal(sKey);
		}
		return NULL;
	}

	if ( pMap->iPos == 0 ) {
		if ( sKey != NULL ) {
			xpkFreeInternal(sKey);
		}
		procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		return NULL;
	}
	pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, pMap->iPos);
	if ( pEntry == NULL || ((pEntry->iFlag & XPK_FLAG_DELETED_MASK) != 0) ||
		(pEntry->sPath == NULL) || (pEntry->sPath[0] == '\0') ) {
		if ( sKey != NULL ) {
			xpkFreeInternal(sKey);
		}
		procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		return NULL;
	}
	sEntryKey = procXpkDupPathKey(objXpk, pEntry->sPath);
	if ( sEntryKey == NULL ) {
		if ( sKey != NULL ) {
			xpkFreeInternal(sKey);
		}
		procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		return NULL;
	}
	if ( strcmp(sEntryKey, sKey) != 0 ) {
		xpkFreeInternal(sEntryKey);
		if ( sKey != NULL ) {
			xpkFreeInternal(sKey);
		}
		procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		return NULL;
	}
	xpkFreeInternal(sEntryKey);
	if ( sKey != NULL ) {
		xpkFreeInternal(sKey);
	}
	return pEntry;
}

// 信息扩展大小按包类型
static inline uint32_t procXpkInfoExtSizeByPackType(xpkPackType iType)
{
	switch ( iType ) {
		case XPK_PACK_CORE:
			return 0;
		case XPK_PACK_INDEX:
			return XPK_INFO_EXT_INDEX;
		case XPK_PACK_LINUX:
		case XPK_PACK_WIN32:
			return XPK_INFO_EXT_PATH;
		default:
			return UINT32_MAX;
	}
}

// 条目步长
static inline uint32_t procXpkEntryStride(const xpkHead* pHead)
{
	return XPK_ENTRY_BASE_SIZE + pHead->infoExtSize;
}

// 条目删除
static inline int procXpkEntryDeleted(const xpkEntry* pEntry)
{
	return ((pEntry->iFlag & XPK_FLAG_DELETED_MASK) != 0) ? TRUE : FALSE;
}

// 条目表原始大小
static inline uint64_t procXpkEntryTableRawSize(const xpkHead* pHead)
{
	return (uint64_t)pHead->fileCount * (uint64_t)procXpkEntryStride(pHead);
}

// 获取条目按位置
static inline xpkEntry* procXpkGetEntryByPos(xpkObject objXpk, uint32_t iPos)
{
	if ( (objXpk == NULL) || (iPos == 0) ) {
		return NULL;
	}
	return (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
}

// 校验条目数量状态
static inline int procXpkValidateEntryCountState(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( objXpk->iEntryCount != objXpk->arrEntry.Count ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	return XPK_OK;
}

// 获取公开条目按位置
static inline int procXpkGetPublicEntryByPos(xpkObject objXpk, uint32_t iPos, xpkEntry** ppEntryRet)
{
	xpkEntry* pEntry;

	if ( ppEntryRet != NULL ) {
		*ppEntryRet = NULL;
	}
	if ( objXpk == NULL || iPos == 0 ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( iPos > objXpk->iEntryCount ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}

	pEntry = procXpkGetEntryByPos(objXpk, iPos);
	if ( pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	if ( ppEntryRet != NULL ) {
		*ppEntryRet = pEntry;
	}
	return XPK_OK;
}

// 可见条目数量
static inline uint32_t procXpkVisibleEntryCount(xpkObject objXpk)
{
	uint32_t iCount;
	uint32_t iPos;
	xpkEntry* pEntry;

	if ( objXpk == NULL ) {
		return 0;
	}
	if ( objXpk->iEntryCount != objXpk->arrEntry.Count ) {
		return 0;
	}

	iCount = 0;
	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			continue;
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iCount++;
	}
	return iCount;
}

// 可见条目数量严格
static inline int procXpkVisibleEntryCountStrict(xpkObject objXpk, uint32_t* pCountRet)
{
	uint32_t iCount;
	uint32_t iPos;
	xpkEntry* pEntry;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pCountRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		*pCountRet = 0;
		return xpkLastError(objXpk);
	}

	iCount = 0;
	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			*pCountRet = 0;
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( ((objXpk->objHead.packType == XPK_PACK_LINUX) || (objXpk->objHead.packType == XPK_PACK_WIN32)) &&
			((pEntry->sPath == NULL) || (pEntry->sPath[0] == '\0')) ) {
			*pCountRet = 0;
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		if ( procXpkValidateLiveEntryLookup(objXpk, pEntry) != XPK_OK ) {
			*pCountRet = 0;
			return xpkLastError(objXpk);
		}
		iCount++;
	}

	*pCountRet = iCount;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 校验有效条目查找
static inline int procXpkValidateLiveEntryLookup(xpkObject objXpk, const xpkEntry* pEntry)
{
	xpkEntry* pMap;
	char* sKey;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return XPK_OK;
	}

	if ( objXpk->objHead.packType == XPK_PACK_INDEX ) {
		pMap = (xpkEntry*)xrtListGet(&objXpk->lstEntry, pEntry->iFileIndex);
		if ( pMap == NULL || pMap->iPos != pEntry->iPos || pMap->iFileIndex != pEntry->iFileIndex ) {
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		return XPK_OK;
	}

	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		return XPK_OK;
	}
	if ( pEntry->sPath == NULL || pEntry->sPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( strlen(pEntry->sPath) >= XPK_PATH_BYTES ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	sKey = procXpkDupPathKey(objXpk, pEntry->sPath);
	if ( sKey == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	pMap = (xpkEntry*)xrtDictGet(&objXpk->tblEntry, (ptr)sKey, (uint32_t)strlen(sKey));
	xpkFreeInternal(sKey);
	if ( pMap == NULL || pMap->iPos != pEntry->iPos ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	return XPK_OK;
}

// 释放条目文本
static inline void procXpkFreeEntryText(xpkObject objXpk)
{
	uint32_t iPos;
	xpkEntry* pEntry;

	if ( objXpk == NULL ) {
		return;
	}

	for ( iPos = 1; iPos <= objXpk->arrEntry.Count; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		procXpkFreeEntryOwned(pEntry);
	}
}

// 重置查找
static inline void procXpkResetLookup(xpkObject objXpk)
{
	xrtListUnit(&objXpk->lstEntry);
	xrtDictUnit(&objXpk->tblEntry);
	memset(&objXpk->lstEntry, 0, sizeof(objXpk->lstEntry));
	memset(&objXpk->tblEntry, 0, sizeof(objXpk->tblEntry));

	xrtListInit(&objXpk->lstEntry, sizeof(xpkEntry), XRT_OBJMODE_LOCAL);
	xrtDictInit(&objXpk->tblEntry, sizeof(xpkEntry), XRT_OBJMODE_LOCAL);
}

// 重建查找
static inline int procXpkRebuildLookup(xpkObject objXpk)
{
	uint32_t iPos;
	bool bNew;
	char* sKey;
	xpkEntry* pEntry;
	xpkEntry* pMap;
	if ( objXpk == NULL ) {
		return XPK_ERR_PARAM;
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}

	procXpkResetLookup(objXpk);
	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			procXpkResetLookup(objXpk);
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}

		pEntry->iPos = iPos;
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		if ( objXpk->objHead.packType == XPK_PACK_INDEX ) {
			pMap = (xpkEntry*)xrtListSet(&objXpk->lstEntry, pEntry->iFileIndex, &bNew);
			if ( (pMap == NULL) || !bNew ) {
				procXpkResetLookup(objXpk);
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
			*pMap = *pEntry;
		} else if ( (objXpk->objHead.packType == XPK_PACK_LINUX) || (objXpk->objHead.packType == XPK_PACK_WIN32) ) {
			if ( pEntry->sPath == NULL || pEntry->sPath[0] == '\0' ) {
				procXpkResetLookup(objXpk);
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
			if ( strlen(pEntry->sPath) >= XPK_PATH_BYTES ) {
				procXpkResetLookup(objXpk);
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}

			sKey = procXpkDupPathKey(objXpk, pEntry->sPath);
			if ( sKey == NULL ) {
				procXpkResetLookup(objXpk);
				return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			}

			pMap = (xpkEntry*)xrtDictSet(&objXpk->tblEntry, (ptr)sKey, (uint32_t)strlen(sKey), &bNew);
			if ( sKey != NULL ) {
				xpkFreeInternal(sKey);
			}
			if ( (pMap == NULL) || !bNew ) {
				procXpkResetLookup(objXpk);
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
			*pMap = *pEntry;
		}
	}
	return XPK_OK;
}

// 重置条目
static inline void procXpkResetEntries(xpkObject objXpk)
{
	procXpkFreeEntryText(objXpk);

	xrtArrayUnit(&objXpk->arrEntry);
	xrtListUnit(&objXpk->lstEntry);
	xrtDictUnit(&objXpk->tblEntry);
	memset(&objXpk->arrEntry, 0, sizeof(objXpk->arrEntry));
	memset(&objXpk->lstEntry, 0, sizeof(objXpk->lstEntry));
	memset(&objXpk->tblEntry, 0, sizeof(objXpk->tblEntry));

	xrtArrayInit(&objXpk->arrEntry, sizeof(xpkEntry), XRT_OBJMODE_LOCAL);
	xrtListInit(&objXpk->lstEntry, sizeof(xpkEntry), XRT_OBJMODE_LOCAL);
	xrtDictInit(&objXpk->tblEntry, sizeof(xpkEntry), XRT_OBJMODE_LOCAL);

	objXpk->iEntryCount = 0;
}

// 重置已加载状态
static inline void procXpkResetLoadedState(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return;
	}

	procXpkUnitWriteQueue(objXpk);
	if ( objXpk->pPackageMeta != NULL ) {
		xpkFreeInternal(objXpk->pPackageMeta);
		objXpk->pPackageMeta = NULL;
	}
	objXpk->iPackageMetaSize = 0;
	procXpkResetEntries(objXpk);
	procXpkInitHead(objXpk);
	objXpk->iAppendPos = XPK_HEAD_SIZE;
	objXpk->iFileSize = XPK_HEAD_SIZE;
	procXpkMarkClean(objXpk);
	procXpkClearError(objXpk);
}

// 追加条目自有
static inline int procXpkAppendEntryOwned(xpkObject objXpk, xpkEntry* pEntrySrc)
{
	uint32_t iPos;
	xpkEntry* pEntry;

	iPos = xrtArrayAppend(&objXpk->arrEntry, 1);
	if ( iPos == 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
	if ( pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	memset(pEntry, 0, sizeof(*pEntry));
	*pEntry = *pEntrySrc;
	pEntry->iPos = iPos;

	pEntrySrc->pInfoExt = NULL;
	pEntrySrc->sPath = NULL;
	objXpk->iEntryCount = iPos;
	return XPK_OK;
}

// 标记对象为干净状态
static inline void procXpkMarkClean(xpkObject objXpk)
{
	objXpk->bDirtyHead = FALSE;
	objXpk->bDirtyPackageMeta = FALSE;
	objXpk->bDirtyEntryTable = FALSE;
	objXpk->bDirtyData = FALSE;
}

// 标记脏条目表
static inline void procXpkMarkDirtyEntryTable(xpkObject objXpk)
{
	objXpk->bDirtyEntryTable = TRUE;
	objXpk->bDirtyHead = TRUE;
}

// 初始化默认包头
static inline void procXpkInitHead(xpkObject objXpk)
{
	xtime tNow;

	tNow = xpkNowInternal();
	memset(&objXpk->objHead, 0, sizeof(objXpk->objHead));
	objXpk->objHead.fileHead = XPK_FILE_HEAD;
	objXpk->objHead.packType = XPK_PACK_CORE;
	objXpk->objHead.defComp = 6;
	objXpk->objHead.metaComp = 6;
	objXpk->objHead.infoComp = 6;
	objXpk->objHead.infoExtSize = 0;
	objXpk->objHead.dataOffset = XPK_HEAD_SIZE;
	objXpk->objHead.createTime = tNow;
	objXpk->objHead.changeTime = tNow;
}

// 标记已应用布局
static inline void procXpkMarkAppliedLayout(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return;
	}

	objXpk->bSolidApplied = objXpk->objHead.solidMode ? TRUE : FALSE;
	objXpk->bVolumeApplied = objXpk->objHead.volumeMode ? TRUE : FALSE;
	objXpk->iVolumeSizeApplied = objXpk->objHead.volumeSize;
}

// 已应用分卷大小
static inline uint32_t procXpkAppliedVolumeSize(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return 0;
	}
	return objXpk->iVolumeSizeApplied;
}

// 已应用分卷模式
static inline int procXpkAppliedVolumeMode(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return FALSE;
	}
	return objXpk->bVolumeApplied ? TRUE : FALSE;
}

// 当前数据结束
static inline uint64_t procXpkCurrentDataEnd(xpkObject objXpk)
{
	uint64_t iDataEnd;

	if ( objXpk == NULL ) {
		return 0;
	}

	iDataEnd = objXpk->objHead.dataOffset;
	if ( objXpk->iAppendPos > iDataEnd ) {
		iDataEnd = objXpk->iAppendPos;
	}
	if ( iDataEnd < XPK_HEAD_SIZE ) {
		iDataEnd = XPK_HEAD_SIZE;
	}
	return iDataEnd;
}

// 判断分卷布局是否变化
static inline int procXpkVolumeLayoutChanged(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return FALSE;
	}
	if ( procXpkAppliedVolumeMode(objXpk) != (objXpk->objHead.volumeMode ? TRUE : FALSE) ) {
		return TRUE;
	}
	if ( procXpkAppliedVolumeSize(objXpk) != objXpk->objHead.volumeSize ) {
		return TRUE;
	}
	return FALSE;
}

// 判断 Solid 布局是否变化
static inline int procXpkSolidLayoutChanged(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return FALSE;
	}
	return (objXpk->bSolidApplied ? TRUE : FALSE) != (objXpk->objHead.solidMode ? TRUE : FALSE);
}

// 可采用目标布局
static inline int procXpkCanAdoptTargetLayout(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return FALSE;
	}
	if ( objXpk->iEntryCount != 0 || objXpk->arrEntry.Count != 0 ) {
		return FALSE;
	}
	if ( objXpk->iAppendPos != XPK_HEAD_SIZE ) {
		return FALSE;
	}
	if ( objXpk->iFileSize != XPK_HEAD_SIZE ) {
		return FALSE;
	}
	if ( procXpkWriteQueueCount(objXpk) > 0 ) {
		return FALSE;
	}
	return TRUE;
}

// 初始化对象
static inline void procXpkInitObject(xpkObject objXpk, const xpkOpenOptions* pOpt)
{
	memset(objXpk, 0, sizeof(*objXpk));
	procXpkInitHead(objXpk);
	objXpk->bReadonly = (pOpt != NULL) ? pOpt->readonly : FALSE;
	objXpk->bBufferedDefault = (pOpt != NULL) ? pOpt->bufferedDefault : FALSE;
	objXpk->iAppendPos = XPK_HEAD_SIZE;
	objXpk->iFileSize = XPK_HEAD_SIZE;
	xrtArrayInit(&objXpk->arrEntry, sizeof(xpkEntry), XRT_OBJMODE_LOCAL);
	xrtListInit(&objXpk->lstEntry, sizeof(xpkEntry), XRT_OBJMODE_LOCAL);
	xrtDictInit(&objXpk->tblEntry, sizeof(xpkEntry), XRT_OBJMODE_LOCAL);
	procXpkMarkAppliedLayout(objXpk);
	procXpkClearError(objXpk);
}

// 释放对象
static inline void procXpkUnitObject(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return;
	}

	if ( objXpk->sPathPackage != NULL ) {
		xpkFreeInternal(objXpk->sPathPackage);
		objXpk->sPathPackage = NULL;
	}
	if ( objXpk->pPackageMeta != NULL ) {
		xpkFreeInternal(objXpk->pPackageMeta);
		objXpk->pPackageMeta = NULL;
		objXpk->iPackageMetaSize = 0;
	}

	procXpkFreeEntryText(objXpk);
	xrtArrayUnit(&objXpk->arrEntry);
	xrtListUnit(&objXpk->lstEntry);
	xrtDictUnit(&objXpk->tblEntry);
	memset(&objXpk->arrEntry, 0, sizeof(objXpk->arrEntry));
	memset(&objXpk->lstEntry, 0, sizeof(objXpk->lstEntry));
	memset(&objXpk->tblEntry, 0, sizeof(objXpk->tblEntry));
}


/* ===== File: src/codec/router.h ===== */

/*
	xPack 编解码路由模块

	负责压缩级别映射、编解码分派与压缩缓冲管理。
*/

static const xpkCompMap arrXpkCompTable[16] = {
	{ XPK_ALG_STORE, 0 },
	{ XPK_ALG_LZ4, 1 },
	{ XPK_ALG_LZ4, 4 },
	{ XPK_ALG_LZ4HC, 8 },
	{ XPK_ALG_LZ4HC, 12 },
	{ XPK_ALG_ZSTD, ZSTD_fast },
	{ XPK_ALG_ZSTD, ZSTD_dfast },
	{ XPK_ALG_ZSTD, ZSTD_greedy },
	{ XPK_ALG_ZSTD, ZSTD_lazy },
	{ XPK_ALG_ZSTD, ZSTD_lazy2 },
	{ XPK_ALG_ZSTD, ZSTD_btlazy2 },
	{ XPK_ALG_ZSTD, ZSTD_btopt },
	{ XPK_ALG_ZSTD, ZSTD_btultra },
	{ XPK_ALG_ZSTD, ZSTD_btultra2 },
	{ XPK_ALG_LZMA2, 6 },
	{ XPK_ALG_LZMA2, 9 }
};

// 将压缩级别映射为算法
static inline uint32_t procXpkCompLevelToAlg(uint8_t iLevel)
{
	return arrXpkCompTable[iLevel & 0x0Fu].iAlgorithm;
}

// 将压缩级别映射为原生级别
static inline int procXpkCompLevelToNative(uint8_t iLevel)
{
	return arrXpkCompTable[iLevel & 0x0Fu].iNativeLevel;
}

// 计算编解码输出上界
static inline int procXpkCodecBound(xpkObject objXpk, uint8_t iLevel, uint32_t iRawSize, uint32_t* pSizeRet)
{
	uint64_t iBound64;
	size_t iBound;

	if ( pSizeRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	switch ( procXpkCompLevelToAlg(iLevel) ) {
		case XPK_ALG_STORE:
			*pSizeRet = iRawSize;
			return XPK_OK;
		case XPK_ALG_LZ4:
		case XPK_ALG_LZ4HC:
			iBound = (size_t)LZ4_compressBound((int)iRawSize);
			break;
		case XPK_ALG_ZSTD:
			iBound = (size_t)ZSTD_compressBound((size_t)iRawSize);
			break;
		case XPK_ALG_LZMA2:
			iBound64 = (uint64_t)iRawSize + ((uint64_t)iRawSize / 100u) + 1025u;
			if ( iBound64 > UINT32_MAX ) {
				return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
			}
			*pSizeRet = (uint32_t)iBound64;
			return XPK_OK;
		default:
			return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorCodecUnsupported);
	}

	if ( iBound > UINT32_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	*pSizeRet = (uint32_t)iBound;
	return XPK_OK;
}

// 按原样策略复制压缩缓冲
static inline int procXpkCodecStoreCopy(xpkObject objXpk, const void* pData, uint32_t iRawSize, void** pBufRet, uint32_t* pSizeRet, uint8_t* pLevelRet)
{
	void* pBuf;

	*pBufRet = NULL;
	*pSizeRet = 0;
	*pLevelRet = 0;

	if ( iRawSize == 0 ) {
		return XPK_OK;
	}

	pBuf = xpkAllocInternal(iRawSize);
	if ( pBuf == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	memcpy(pBuf, pData, iRawSize);
	*pBufRet = pBuf;
	*pSizeRet = iRawSize;
	return XPK_OK;
}

// 执行数据编码
static inline int procXpkCodecEncode(xpkObject objXpk, uint8_t iLevel, const void* pData, uint32_t iRawSize, void** pBufRet, uint32_t* pSizeRet, uint8_t* pLevelRet)
{
	uint32_t iBound;
	void* pBuf;
	int iAlg;
	int iRet;
	size_t iZstdRet;
	ZSTD_CCtx* pCctx;
	CLzma2EncHandle hLzma2;
	CLzma2EncProps objLzma2Props;
	Byte iPropByte;
	size_t iLzma2Size;
	SRes iLzmaRes;

	if ( (pBufRet == NULL) || (pSizeRet == NULL) || (pLevelRet == NULL) ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	*pBufRet = NULL;
	*pSizeRet = 0;
	*pLevelRet = iLevel;
	if ( iRawSize == 0 ) {
		return XPK_OK;
	}
	if ( pData == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iLevel == 0 ) {
		return procXpkCodecStoreCopy(objXpk, pData, iRawSize, pBufRet, pSizeRet, pLevelRet);
	}

	iRet = procXpkCodecBound(objXpk, iLevel, iRawSize, &iBound);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	pBuf = xpkAllocInternal(iBound);
	if ( pBuf == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iAlg = procXpkCompLevelToAlg(iLevel);
	if ( iAlg == XPK_ALG_LZ4 ) {
		iRet = LZ4_compress_fast((const char*)pData, (char*)pBuf, (int)iRawSize, (int)iBound, procXpkCompLevelToNative(iLevel));
		if ( iRet <= 0 ) {
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_IO, "lz4 compress failed");
		}
		*pSizeRet = (uint32_t)iRet;
	} else if ( iAlg == XPK_ALG_LZ4HC ) {
		iRet = LZ4_compress_HC((const char*)pData, (char*)pBuf, (int)iRawSize, (int)iBound, procXpkCompLevelToNative(iLevel));
		if ( iRet <= 0 ) {
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_IO, "lz4hc compress failed");
		}
		*pSizeRet = (uint32_t)iRet;
	} else if ( iAlg == XPK_ALG_ZSTD ) {
		pCctx = ZSTD_createCCtx();
		if ( pCctx == NULL ) {
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		ZSTD_CCtx_setParameter(pCctx, ZSTD_c_checksumFlag, 0);
		ZSTD_CCtx_setParameter(pCctx, ZSTD_c_strategy, (ZSTD_strategy)procXpkCompLevelToNative(iLevel));
		iZstdRet = ZSTD_compress2(pCctx, pBuf, (size_t)iBound, pData, (size_t)iRawSize);
		ZSTD_freeCCtx(pCctx);
		if ( ZSTD_isError(iZstdRet) ) {
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_IO, "zstd compress failed");
		}
		*pSizeRet = (uint32_t)iZstdRet;
	} else if ( iAlg == XPK_ALG_LZMA2 ) {
		hLzma2 = Lzma2Enc_Create(&g_Alloc, &g_BigAlloc);
		if ( hLzma2 == NULL ) {
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}

		Lzma2EncProps_Init(&objLzma2Props);
		objLzma2Props.lzmaProps.level = procXpkCompLevelToNative(iLevel);
		iLzmaRes = Lzma2Enc_SetProps(hLzma2, &objLzma2Props);
		if ( iLzmaRes != SZ_OK ) {
			Lzma2Enc_Destroy(hLzma2);
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_IO, "lzma2 set props failed");
		}

		Lzma2Enc_SetDataSize(hLzma2, (UInt64)iRawSize);
		iPropByte = Lzma2Enc_WriteProperties(hLzma2);
		((Byte*)pBuf)[0] = iPropByte;
		iLzma2Size = (size_t)iBound - 1;
		iLzmaRes = Lzma2Enc_Encode2(hLzma2, NULL, (Byte*)pBuf + 1, &iLzma2Size, NULL, (const Byte*)pData, (size_t)iRawSize, NULL);
		Lzma2Enc_Destroy(hLzma2);
		if ( iLzmaRes != SZ_OK ) {
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_IO, "lzma2 compress failed");
		}
		*pSizeRet = (uint32_t)(iLzma2Size + 1);
	} else {
		xpkFreeInternal(pBuf);
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorCodecUnsupported);
	}

	if ( *pSizeRet >= iRawSize ) {
		xpkFreeInternal(pBuf);
		return procXpkCodecStoreCopy(objXpk, pData, iRawSize, pBufRet, pSizeRet, pLevelRet);
	}

	*pBufRet = pBuf;
	return XPK_OK;
}

// 执行数据解码
static inline int procXpkCodecDecode(xpkObject objXpk, uint8_t iLevel, const void* pData, uint32_t iCompSize, uint32_t iRawSize, void** pBufRet)
{
	void* pBuf;
	int iAlg;
	int iRet;
	size_t iZstdRet;
	Byte iPropByte;
	SizeT iDstLen;
	SizeT iSrcLen;
	ELzmaStatus iLzmaStatus;
	SRes iLzmaRes;

	if ( pBufRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	*pBufRet = NULL;
	if ( iRawSize == 0 ) {
		return XPK_OK;
	}
	if ( (pData == NULL) || (iCompSize == 0) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	pBuf = xpkAllocInternal(iRawSize);
	if ( pBuf == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iAlg = procXpkCompLevelToAlg(iLevel);
	if ( iAlg == XPK_ALG_STORE ) {
		if ( iCompSize != iRawSize ) {
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		memcpy(pBuf, pData, iRawSize);
	} else if ( (iAlg == XPK_ALG_LZ4) || (iAlg == XPK_ALG_LZ4HC) ) {
		iRet = LZ4_decompress_safe((const char*)pData, (char*)pBuf, (int)iCompSize, (int)iRawSize);
		if ( iRet != (int)iRawSize ) {
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lz4 decompress failed");
		}
	} else if ( iAlg == XPK_ALG_ZSTD ) {
		iZstdRet = ZSTD_decompress(pBuf, (size_t)iRawSize, pData, (size_t)iCompSize);
		if ( ZSTD_isError(iZstdRet) || (iZstdRet != iRawSize) ) {
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
		}
	} else if ( iAlg == XPK_ALG_LZMA2 ) {
		if ( iCompSize < 1 ) {
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}

		iPropByte = ((const Byte*)pData)[0];
		iDstLen = (SizeT)iRawSize;
		iSrcLen = (SizeT)(iCompSize - 1);
		iLzmaRes = Lzma2Decode((Byte*)pBuf, &iDstLen, (const Byte*)pData + 1, &iSrcLen, iPropByte, LZMA_FINISH_END, &iLzmaStatus, &g_Alloc);
		if ( (iLzmaRes != SZ_OK) || (iDstLen != iRawSize) ) {
			xpkFreeInternal(pBuf);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
		}
	} else {
		xpkFreeInternal(pBuf);
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorCodecUnsupported);
	}

	*pBufRet = pBuf;
	return XPK_OK;
}


/* ===== File: src/storage/fileio.h ===== */

/*
	xPack 文件存储模块

	负责文件映射、分卷扫描、随机读写与物理文件操作。
*/

#include <stdio.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// 解除文件映射
static inline void procXpkUnmapFile(xpkMappedFile* pMap)
{
	if ( pMap == NULL ) {
		return;
	}

#if defined(_WIN32) || defined(_WIN64)
	if ( pMap->pView != NULL ) {
		UnmapViewOfFile(pMap->pView);
		pMap->pView = NULL;
	}
	if ( pMap->hMap != NULL ) {
		CloseHandle(pMap->hMap);
		pMap->hMap = NULL;
	}
#else
	if ( pMap->pView != NULL ) {
		munmap((void*)pMap->pView, (size_t)pMap->iSize);
		pMap->pView = NULL;
	}
	pMap->pMap = NULL;
#endif
	pMap->iSize = 0;
}

// 建立只读文件映射
static inline int procXpkMapFileReadOnly(xpkObject objXpk, xfile hFile, uint64_t iSize, xpkMappedFile* pMapRet)
{
	if ( pMapRet != NULL ) {
		memset(pMapRet, 0, sizeof(*pMapRet));
	}
	if ( objXpk == NULL || hFile == NULL || pMapRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iSize == 0 ) {
		return XPK_OK;
	}
	if ( iSize > (uint64_t)SIZE_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}

#if defined(_WIN32) || defined(_WIN64)
	{
		LARGE_INTEGER iMapSize;

		iMapSize.QuadPart = iSize;
		pMapRet->hMap = CreateFileMapping((HANDLE)hFile->obj, NULL, PAGE_READONLY, iMapSize.HighPart, iMapSize.LowPart, NULL);
		if ( pMapRet->hMap == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
		}
		pMapRet->pView = MapViewOfFile(pMapRet->hMap, FILE_MAP_READ, 0, 0, 0);
		if ( pMapRet->pView == NULL ) {
			CloseHandle(pMapRet->hMap);
			pMapRet->hMap = NULL;
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
		}
	}
#else
	pMapRet->pMap = mmap(NULL, (size_t)iSize, PROT_READ, MAP_PRIVATE, hFile->idx, 0);
	if ( pMapRet->pMap == MAP_FAILED ) {
		pMapRet->pMap = NULL;
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
	}
	pMapRet->pView = pMapRet->pMap;
#endif

	pMapRet->iSize = iSize;
	return XPK_OK;
}

// 建立读写文件映射
static inline int procXpkMapFileReadWrite(xpkObject objXpk, xfile hFile, uint64_t iSize, xpkMappedFile* pMapRet)
{
	size_t iPos;

	if ( pMapRet != NULL ) {
		memset(pMapRet, 0, sizeof(*pMapRet));
	}
	if ( objXpk == NULL || hFile == NULL || pMapRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iSize == 0 ) {
		return XPK_OK;
	}
	if ( iSize > (uint64_t)SIZE_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}
	if ( iSize > (uint64_t)INT64_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorSeekRange);
	}

	iPos = xrtSeek(hFile, (int64)iSize, XRT_SEEK_SET);
	if ( (uint64_t)iPos != iSize ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoSeek);
	}
	if ( !xrtSetEOF(hFile) ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

#if defined(_WIN32) || defined(_WIN64)
	{
		LARGE_INTEGER iMapSize;

		iMapSize.QuadPart = iSize;
		pMapRet->hMap = CreateFileMapping((HANDLE)hFile->obj, NULL, PAGE_READWRITE, iMapSize.HighPart, iMapSize.LowPart, NULL);
		if ( pMapRet->hMap == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}
		pMapRet->pView = MapViewOfFile(pMapRet->hMap, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 0);
		if ( pMapRet->pView == NULL ) {
			CloseHandle(pMapRet->hMap);
			pMapRet->hMap = NULL;
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}
	}
#else
	pMapRet->pMap = mmap(NULL, (size_t)iSize, PROT_READ | PROT_WRITE, MAP_SHARED, hFile->idx, 0);
	if ( pMapRet->pMap == MAP_FAILED ) {
		pMapRet->pMap = NULL;
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}
	pMapRet->pView = pMapRet->pMap;
#endif

	pMapRet->iSize = iSize;
	return XPK_OK;
}

// 生成分卷路径文本
static inline char* procXpkVolumePathDupText(const char* sPathPackage, uint32_t iVolume)
{
	char sSuffix[32];
	size_t iSizePath;
	size_t iSizeSuffix;
	char* sPathRet;
	int iSizePrint;

	if ( sPathPackage == NULL ) {
		return NULL;
	}
	if ( iVolume == 0 ) {
		return procXpkDupText(sPathPackage);
	}

	iSizePrint = snprintf(sSuffix, sizeof(sSuffix), ".%03u", (unsigned int)iVolume);
	if ( iSizePrint <= 0 || (size_t)iSizePrint >= sizeof(sSuffix) ) {
		return NULL;
	}

	iSizePath = strlen(sPathPackage);
	iSizeSuffix = (size_t)iSizePrint;
	sPathRet = (char*)xpkAllocInternal(iSizePath + iSizeSuffix + 1);
	if ( sPathRet == NULL ) {
		return NULL;
	}

	memcpy(sPathRet, sPathPackage, iSizePath);
	memcpy(sPathRet + iSizePath, sSuffix, iSizeSuffix + 1);
	return sPathRet;
}

// 生成带后缀的路径文本
static inline char* procXpkPathSuffixDupText(const char* sPathBase, const char* sSuffix)
{
	size_t iSizeBase;
	size_t iSizeSuffix;
	char* sPathRet;

	if ( sPathBase == NULL || sSuffix == NULL ) {
		return NULL;
	}

	iSizeBase = strlen(sPathBase);
	iSizeSuffix = strlen(sSuffix);
	sPathRet = (char*)xpkAllocInternal(iSizeBase + iSizeSuffix + 1);
	if ( sPathRet == NULL ) {
		return NULL;
	}

	memcpy(sPathRet, sPathBase, iSizeBase);
	memcpy(sPathRet + iSizeBase, sSuffix, iSizeSuffix + 1);
	return sPathRet;
}

// 匹配松散分卷文件名
static inline int procXpkLooseVolumeMatchName(const xpkLooseVolumeScan* pScan, const char* sName, uint32_t* pVolumeRet)
{
	const char* sDigit;
	uint64_t iVolume;

	if ( pVolumeRet != NULL ) {
		*pVolumeRet = 0;
	}
	if ( pScan == NULL || sName == NULL ) {
		return FALSE;
	}
	if ( strncmp(sName, pScan->sNameBase, pScan->iNameSize) != 0 ) {
		return FALSE;
	}
	if ( sName[pScan->iNameSize] != '.' ) {
		return FALSE;
	}

	sDigit = sName + pScan->iNameSize + 1;
	if ( *sDigit == '\0' ) {
		return FALSE;
	}

	iVolume = 0;
	for ( ; *sDigit != '\0'; sDigit++ ) {
		if ( (*sDigit < '0') || (*sDigit > '9') ) {
			return FALSE;
		}
		iVolume = (iVolume * 10u) + (uint64_t)(*sDigit - '0');
		if ( iVolume > UINT32_MAX ) {
			return FALSE;
		}
	}
	if ( iVolume < pScan->iStartVolume ) {
		return FALSE;
	}

	if ( pVolumeRet != NULL ) {
		*pVolumeRet = (uint32_t)iVolume;
	}
	return TRUE;
}

// 拼接路径文本
static inline char* procXpkPathJoinDupText(const char* sDirPath, const char* sName)
{
	size_t iSizeDir;
	size_t iSizeName;
	size_t iSizeSep;
	char* sPathRet;

	if ( sDirPath == NULL || sName == NULL ) {
		return NULL;
	}
	if ( strcmp(sDirPath, ".") == 0 ) {
		return procXpkDupText(sName);
	}

	iSizeDir = strlen(sDirPath);
	iSizeName = strlen(sName);
	iSizeSep = 0;
	if ( iSizeDir > 0 && sDirPath[iSizeDir - 1] != '/' && sDirPath[iSizeDir - 1] != '\\' ) {
		iSizeSep = 1;
	}

	sPathRet = (char*)xpkAllocInternal(iSizeDir + iSizeSep + iSizeName + 1);
	if ( sPathRet == NULL ) {
		return NULL;
	}

	memcpy(sPathRet, sDirPath, iSizeDir);
	if ( iSizeSep != 0 ) {
		sPathRet[iSizeDir] = '/';
	}
	memcpy(sPathRet + iSizeDir + iSizeSep, sName, iSizeName + 1);
	return sPathRet;
}

// 处理松散分卷条目
static inline int procXpkHandleLooseVolumeEntry(xpkLooseVolumeScan* pScan, const char* sDirPath, const char* sName, int bDir)
{
	char* sPathEntry;
	uint32_t iVolume;

	if ( pScan == NULL || sDirPath == NULL || sName == NULL ) {
		return procXpkSetError((pScan != NULL) ? pScan->objXpk : NULL, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( !procXpkLooseVolumeMatchName(pScan, sName, &iVolume) ) {
		return XPK_OK;
	}

	(void)iVolume;
	pScan->iCount++;
	if ( !pScan->bDelete ) {
		return XPK_OK;
	}

	sPathEntry = procXpkPathJoinDupText(sDirPath, sName);
	if ( sPathEntry == NULL ) {
		return procXpkSetError(pScan->objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	if ( bDir ) {
		xpkFreeInternal(sPathEntry);
		return procXpkSetError(pScan->objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}
	if ( !xrtFileDelete((str)sPathEntry) ) {
		xpkFreeInternal(sPathEntry);
		return procXpkSetError(pScan->objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	xpkFreeInternal(sPathEntry);
	return XPK_OK;
}

// 路径名称文本
static inline const char* procXpkPathNameText(const char* sPath)
{
	const char* sName;

	if ( sPath == NULL ) {
		return NULL;
	}

	sName = sPath;
	for ( ; *sPath != '\0'; sPath++ ) {
		if ( (*sPath == '/') || (*sPath == '\\') ) {
			sName = sPath + 1;
		}
	}
	return sName;
}

// 路径目录复制文本
static inline char* procXpkPathDirDupText(const char* sPath)
{
	const char* sSep;
	const char* sCur;
	size_t iSize;
	char* sRet;

	if ( sPath == NULL ) {
		return NULL;
	}

	sSep = NULL;
	for ( sCur = sPath; *sCur != '\0'; sCur++ ) {
		if ( (*sCur == '/') || (*sCur == '\\') ) {
			sSep = sCur;
		}
	}

	if ( sSep == NULL ) {
		return procXpkDupText(".");
	}

	iSize = (size_t)(sSep - sPath + 1);
	sRet = (char*)xpkAllocInternal(iSize + 1);
	if ( sRet == NULL ) {
		return NULL;
	}

	memcpy(sRet, sPath, iSize);
	sRet[iSize] = '\0';
	return sRet;
}

// 松散分卷扫描回调
static inline int procXpkLooseVolumeScanProc(ptr sPath, size_t iSize, int bDir, ptr pData, ptr Param)
{
	xpkLooseVolumeScan* pScan;
	const char* sName;
	const char* sDigit;
	uint64_t iVolume;

	(void)iSize;
	(void)pData;
	if ( sPath == NULL || Param == NULL ) {
		return FALSE;
	}
	if ( bDir == 2 ) {
		return FALSE;
	}
	if ( bDir != 0 && bDir != 1 ) {
		return FALSE;
	}

	pScan = (xpkLooseVolumeScan*)Param;
	sName = procXpkPathNameText((const char*)sPath);
	if ( sName == NULL ) {
		return FALSE;
	}
	if ( strncmp(sName, pScan->sNameBase, pScan->iNameSize) != 0 ) {
		return FALSE;
	}
	if ( sName[pScan->iNameSize] != '.' ) {
		return FALSE;
	}

	sDigit = sName + pScan->iNameSize + 1;
	if ( *sDigit == '\0' ) {
		return FALSE;
	}
	iVolume = 0;
	for ( ; *sDigit != '\0'; sDigit++ ) {
		if ( (*sDigit < '0') || (*sDigit > '9') ) {
			return FALSE;
		}
		if ( iVolume < UINT32_MAX ) {
			iVolume = (iVolume * 10u) + (uint64_t)(*sDigit - '0');
			if ( iVolume > UINT32_MAX ) {
				iVolume = UINT32_MAX;
			}
		}
	}
	if ( iVolume < pScan->iStartVolume ) {
		return FALSE;
	}

	pScan->iCount++;
	if ( pScan->bDelete ) {
		if ( bDir != 0 ) {
			pScan->iError = procXpkSetError(pScan->objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
			return TRUE;
		}
		if ( !xrtFileDelete((str)sPath) ) {
			pScan->iError = procXpkSetError(pScan->objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
			return TRUE;
		}
	}

	return FALSE;
}

// 扫描松散分卷文件从文本
static inline int procXpkScanLooseVolumeFilesFromText(xpkObject objXpk, const char* sPathPackage, uint32_t iStartVolume, int bDelete, uint32_t* pCountRet)
{
	xpkLooseVolumeScan objScan;
	const char* sNameBase;
	char* sDirPath;
	int iRet;

	if ( pCountRet != NULL ) {
		*pCountRet = 0;
	}
	if ( sPathPackage == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	sNameBase = procXpkPathNameText(sPathPackage);
	if ( sNameBase == NULL || sNameBase[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	sDirPath = procXpkPathDirDupText(sPathPackage);
	if ( sDirPath == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	if ( strcmp(sDirPath, ".") != 0 && !xrtDirExists((str)sDirPath) ) {
		xpkFreeInternal(sDirPath);
		return XPK_OK;
	}

	memset(&objScan, 0, sizeof(objScan));
	objScan.objXpk = objXpk;
	objScan.sNameBase = sNameBase;
	objScan.iNameSize = strlen(sNameBase);
	objScan.iStartVolume = iStartVolume;
	objScan.bDelete = bDelete ? TRUE : FALSE;
	objScan.iError = XPK_OK;

#if defined(_WIN32) || defined(_WIN64)
	{
		WIN32_FIND_DATAA objFindData;
		HANDLE hFind;
		char* sPattern;

		sPattern = procXpkPathJoinDupText(sDirPath, "*");
		if ( sPattern == NULL ) {
			xpkFreeInternal(sDirPath);
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}

		hFind = FindFirstFileA(sPattern, &objFindData);
		xpkFreeInternal(sPattern);
		if ( hFind != INVALID_HANDLE_VALUE ) {
			do {
				if ( strcmp(objFindData.cFileName, ".") == 0 || strcmp(objFindData.cFileName, "..") == 0 ) {
					continue;
				}
				iRet = procXpkHandleLooseVolumeEntry(&objScan, sDirPath, objFindData.cFileName,
					(objFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE);
				if ( iRet != XPK_OK ) {
					objScan.iError = iRet;
					break;
				}
			} while ( FindNextFileA(hFind, &objFindData) );
			FindClose(hFind);
		}
	}
#else
	{
		DIR* pDir;
		struct dirent* pEnt;

		pDir = opendir(sDirPath);
		if ( pDir != NULL ) {
			while ( (pEnt = readdir(pDir)) != NULL ) {
				int bDirEntry;

				if ( strcmp(pEnt->d_name, ".") == 0 || strcmp(pEnt->d_name, "..") == 0 ) {
					continue;
				}
#if defined(DT_DIR)
				if ( pEnt->d_type == DT_DIR ) {
					bDirEntry = TRUE;
				} else if ( pEnt->d_type == DT_REG ) {
					bDirEntry = FALSE;
				} else
#endif
				{
					struct stat objStat;
					char* sPathEntry;

					sPathEntry = procXpkPathJoinDupText(sDirPath, pEnt->d_name);
					if ( sPathEntry == NULL ) {
						objScan.iError = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
						break;
					}
					bDirEntry = (stat(sPathEntry, &objStat) == 0 && S_ISDIR(objStat.st_mode)) ? TRUE : FALSE;
					xpkFreeInternal(sPathEntry);
				}

				iRet = procXpkHandleLooseVolumeEntry(&objScan, sDirPath, pEnt->d_name, bDirEntry);
				if ( iRet != XPK_OK ) {
					objScan.iError = iRet;
					break;
				}
			}
			closedir(pDir);
		}
	}
#endif

	xpkFreeInternal(sDirPath);
	if ( objScan.iError != XPK_OK ) {
		return objScan.iError;
	}

	if ( pCountRet != NULL ) {
		*pCountRet = objScan.iCount;
	}
	return XPK_OK;
}

// 扫描松散分卷文件文本
static inline int procXpkScanLooseVolumeFilesText(xpkObject objXpk, const char* sPathPackage, int bDelete, uint32_t* pCountRet)
{
	return procXpkScanLooseVolumeFilesFromText(objXpk, sPathPackage, 0, bDelete, pCountRet);
}

// 确保分卷路径未占用文本
static inline int procXpkEnsureVolumePathUnusedText(xpkObject objXpk, const char* sPathPackage, const char* sErrorText)
{
	uint32_t iLooseVolumeCount;
	int iRet;

	if ( sPathPackage == NULL || sErrorText == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( xrtPathExists((str)sPathPackage) ) {
		return procXpkSetError(objXpk, XPK_ERR_EXISTS, sErrorText);
	}

	iLooseVolumeCount = 0;
	iRet = procXpkScanLooseVolumeFilesText(objXpk, sPathPackage, FALSE, &iLooseVolumeCount);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( iLooseVolumeCount > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_EXISTS, sErrorText);
	}

	return XPK_OK;
}

// 打开分卷文本
static inline int procXpkOpenVolumeText(xpkObject objXpk, const char* sPathPackage, uint32_t iVolume, int bReadonly, xfile* pFileRet)
{
	char* sPathVolume;
	xfile hFile;

	if ( pFileRet == NULL || sPathPackage == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	*pFileRet = NULL;
	sPathVolume = procXpkVolumePathDupText(sPathPackage, iVolume);
	if ( sPathVolume == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	hFile = xrtOpen((str)sPathVolume, bReadonly ? TRUE : FALSE, XRT_CP_BINARY);
	xpkFreeInternal(sPathVolume);
	if ( hFile == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	*pFileRet = hFile;
	return XPK_OK;
}

// 获取分卷物理大小文本
static inline int procXpkGetVolumePhysicalSizeText(xpkObject objXpk, const char* sPathPackage, uint32_t iVolume, uint64_t* pSizeRet, int* pExistsRet)
{
	char* sPathVolume;
	xfile hFile;

	if ( pSizeRet != NULL ) {
		*pSizeRet = 0;
	}
	if ( pExistsRet != NULL ) {
		*pExistsRet = FALSE;
	}
	if ( sPathPackage == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	sPathVolume = procXpkVolumePathDupText(sPathPackage, iVolume);
	if ( sPathVolume == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	if ( !xrtFileExists((str)sPathVolume) ) {
		xpkFreeInternal(sPathVolume);
		return XPK_OK;
	}

	hFile = xrtOpen((str)sPathVolume, TRUE, XRT_CP_BINARY);
	xpkFreeInternal(sPathVolume);
	if ( hFile == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	if ( pSizeRet != NULL ) {
		*pSizeRet = xrtGetEOF(hFile);
	}
	if ( pExistsRet != NULL ) {
		*pExistsRet = TRUE;
	}
	xrtClose(hFile);
	return XPK_OK;
}

// 数量分卷文件文本
static inline int procXpkCountVolumeFilesText(xpkObject objXpk, const char* sPathPackage, uint32_t* pCountRet)
{
	uint32_t iVolume;
	char* sPathVolume;

	if ( pCountRet == NULL || sPathPackage == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	*pCountRet = 0;
	for ( iVolume = 0; ; iVolume++ ) {
		sPathVolume = procXpkVolumePathDupText(sPathPackage, iVolume);
		if ( sPathVolume == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}

		if ( !xrtFileExists((str)sPathVolume) ) {
			xpkFreeInternal(sPathVolume);
			break;
		}

		xpkFreeInternal(sPathVolume);
		(*pCountRet)++;
	}

	return XPK_OK;
}

// 删除分卷文件文本
static inline int procXpkDeleteVolumeFilesText(xpkObject objXpk, const char* sPathPackage, uint32_t iStartVolume)
{
	uint32_t iVolume;
	char* sPathVolume;
	int iRet;

	if ( sPathPackage == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	for ( iVolume = iStartVolume; ; iVolume++ ) {
		sPathVolume = procXpkVolumePathDupText(sPathPackage, iVolume);
		if ( sPathVolume == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}

		if ( !xrtFileExists((str)sPathVolume) ) {
			xpkFreeInternal(sPathVolume);
			break;
		}
		if ( !xrtFileDelete((str)sPathVolume) ) {
			xpkFreeInternal(sPathVolume);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}

		xpkFreeInternal(sPathVolume);
	}

	iRet = procXpkScanLooseVolumeFilesFromText(objXpk, sPathPackage, (iStartVolume == 0) ? 1u : iStartVolume, TRUE, NULL);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	return XPK_OK;
}

// 删除分卷文件连续文本
static inline int procXpkDeleteVolumeFilesContiguousText(xpkObject objXpk, const char* sPathPackage, uint32_t iStartVolume)
{
	uint32_t iVolume;
	char* sPathVolume;

	if ( sPathPackage == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	for ( iVolume = iStartVolume; ; iVolume++ ) {
		sPathVolume = procXpkVolumePathDupText(sPathPackage, iVolume);
		if ( sPathVolume == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}

		if ( !xrtFileExists((str)sPathVolume) ) {
			xpkFreeInternal(sPathVolume);
			break;
		}
		if ( !xrtFileDelete((str)sPathVolume) ) {
			xpkFreeInternal(sPathVolume);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}

		xpkFreeInternal(sPathVolume);
	}

	return XPK_OK;
}

// 删除分卷文件精确文本
static inline int procXpkDeleteVolumeFilesExactText(xpkObject objXpk, const char* sPathPackage, uint32_t iVolumeCount)
{
	uint32_t iVolume;
	char* sPathVolume;

	if ( sPathPackage == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	for ( iVolume = 0; iVolume < iVolumeCount; iVolume++ ) {
		sPathVolume = procXpkVolumePathDupText(sPathPackage, iVolume);
		if ( sPathVolume == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}

		if ( xrtFileExists((str)sPathVolume) && !xrtFileDelete((str)sPathVolume) ) {
			xpkFreeInternal(sPathVolume);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}

		xpkFreeInternal(sPathVolume);
	}

	return XPK_OK;
}

// 转移分卷文件文本
static inline int procXpkTransferVolumeFilesText(xpkObject objXpk, const char* sPathSrc, const char* sPathDst, int bMove, int bRewrite)
{
	uint32_t iVolumeCount;
	uint32_t iVolume;
	char* sSrcVolume;
	char* sDstVolume;
	int bOk;
	int iRet;

	if ( sPathSrc == NULL || sPathDst == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	iRet = procXpkCountVolumeFilesText(objXpk, sPathSrc, &iVolumeCount);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( iVolumeCount == 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}

	for ( iVolume = 1; iVolume < iVolumeCount; iVolume++ ) {
		sSrcVolume = procXpkVolumePathDupText(sPathSrc, iVolume);
		sDstVolume = procXpkVolumePathDupText(sPathDst, iVolume);
		if ( sSrcVolume == NULL || sDstVolume == NULL ) {
			if ( sSrcVolume != NULL ) {
				xpkFreeInternal(sSrcVolume);
			}
			if ( sDstVolume != NULL ) {
				xpkFreeInternal(sDstVolume);
			}
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}

		if ( bMove ) {
			bOk = xrtFileMove((str)sSrcVolume, (str)sDstVolume, bRewrite ? TRUE : FALSE);
		} else {
			bOk = xrtFileCopy((str)sSrcVolume, (str)sDstVolume, bRewrite ? TRUE : FALSE);
		}
		if ( !bOk ) {
			xpkFreeInternal(sSrcVolume);
			xpkFreeInternal(sDstVolume);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}

		xpkFreeInternal(sSrcVolume);
		xpkFreeInternal(sDstVolume);
	}

	if ( bMove ) {
		bOk = xrtFileMove((str)sPathSrc, (str)sPathDst, bRewrite ? TRUE : FALSE);
	} else {
		bOk = xrtFileCopy((str)sPathSrc, (str)sPathDst, bRewrite ? TRUE : FALSE);
	}
	if ( !bOk ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	return XPK_OK;
}

// 移动分卷文件文本
static inline int procXpkMoveVolumeFilesText(xpkObject objXpk, const char* sPathSrc, const char* sPathDst)
{
	uint32_t iVolumeCountSrc;
	uint32_t iVolumeCountDst;
	char* sPathBackup;
	int iRet;
	int iRetRollback;

	if ( sPathSrc == NULL || sPathDst == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	iRet = procXpkCountVolumeFilesText(objXpk, sPathSrc, &iVolumeCountSrc);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( iVolumeCountSrc == 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}

	iRet = procXpkCountVolumeFilesText(objXpk, sPathDst, &iVolumeCountDst);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	sPathBackup = procXpkPathSuffixDupText(sPathDst, ".replace.bak");
	if ( sPathBackup == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	iRet = procXpkEnsureVolumePathUnusedText(objXpk, sPathBackup, sXpkErrorBackupPathExists);
	if ( iRet != XPK_OK ) {
		xpkFreeInternal(sPathBackup);
		return iRet;
	}

	if ( iVolumeCountDst > 0 ) {
		iRet = procXpkTransferVolumeFilesText(objXpk, sPathDst, sPathBackup, FALSE, FALSE);
		if ( iRet != XPK_OK ) {
			(void)procXpkDeleteVolumeFilesExactText(objXpk, sPathBackup, iVolumeCountDst);
			xpkFreeInternal(sPathBackup);
			return iRet;
		}
	}

	iRet = procXpkTransferVolumeFilesText(objXpk, sPathSrc, sPathDst, FALSE, TRUE);
	if ( iRet == XPK_OK ) {
		iRet = procXpkDeleteVolumeFilesText(objXpk, sPathDst, iVolumeCountSrc);
	}
	if ( iRet != XPK_OK ) {
		if ( iVolumeCountDst > 0 ) {
			iRetRollback = procXpkTransferVolumeFilesText(objXpk, sPathBackup, sPathDst, FALSE, TRUE);
			if ( iRetRollback == XPK_OK ) {
				iRetRollback = procXpkDeleteVolumeFilesText(objXpk, sPathDst, iVolumeCountDst);
			}
			if ( iRetRollback != XPK_OK ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, "failed to replace package and rollback");
			} else {
				(void)procXpkDeleteVolumeFilesExactText(objXpk, sPathBackup, iVolumeCountDst);
			}
		} else {
			iRetRollback = procXpkDeleteVolumeFilesText(objXpk, sPathDst, 0);
			if ( iRetRollback != XPK_OK ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, "failed to replace package and cleanup");
			}
		}
		(void)procXpkDeleteVolumeFilesExactText(objXpk, sPathSrc, iVolumeCountSrc);
		xpkFreeInternal(sPathBackup);
		return iRet;
	}

	(void)procXpkDeleteVolumeFilesExactText(objXpk, sPathSrc, iVolumeCountSrc);
	(void)procXpkDeleteVolumeFilesExactText(objXpk, sPathBackup, iVolumeCountDst);
	xpkFreeInternal(sPathBackup);
	return XPK_OK;
}

// 定位文件
static inline int procXpkSeekFile(xpkObject objXpk, xfile hFile, uint64_t iOffset)
{
	size_t iPos;

	if ( hFile == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	if ( iOffset > (uint64_t)INT64_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorSeekRange);
	}

	iPos = xrtSeek(hFile, (int64)iOffset, XRT_SEEK_SET);
	if ( (uint64_t)iPos != iOffset ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoSeek);
	}
	return XPK_OK;
}

// 计算逻辑文件大小
static inline int procXpkCalcLogicalFileSize(xpkObject objXpk, const xpkHead* pHead, uint64_t* pSizeRet)
{
	uint32_t iVolume;
	uint64_t iSizeVolume;
	uint64_t iSizeUse;
	uint64_t iSizeTotal;
	int bExists;
	int iRet;

	if ( pSizeRet == NULL || pHead == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	*pSizeRet = 0;
	if ( !pHead->volumeMode ) {
		return procXpkGetVolumePhysicalSizeText(objXpk, objXpk->sPathPackage, 0, pSizeRet, NULL);
	}

	iSizeTotal = 0;
	for ( iVolume = 0; ; iVolume++ ) {
		iRet = procXpkGetVolumePhysicalSizeText(objXpk, objXpk->sPathPackage, iVolume, &iSizeVolume, &bExists);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		if ( !bExists ) {
			break;
		}

		iSizeUse = iSizeVolume;
		if ( iSizeUse > pHead->volumeSize ) {
			iSizeUse = pHead->volumeSize;
		}

		iSizeTotal += iSizeUse;
		if ( iSizeUse < pHead->volumeSize ) {
			break;
		}
	}

	*pSizeRet = iSizeTotal;
	return XPK_OK;
}

// 读取分卷片段文本
static inline int procXpkReadVolumePartText(xpkObject objXpk, const char* sPathPackage, uint32_t iVolume, uint64_t iOffsetVolume, void* pData, uint32_t iSize)
{
	char* sPathVolume;

	if ( sPathPackage == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize == 0 ) {
		return XPK_OK;
	}

	sPathVolume = procXpkVolumePathDupText(sPathPackage, iVolume);
	if ( sPathVolume == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

#if defined(_WIN32) || defined(_WIN64)
	{
		u16str sPathW;
		HANDLE hFile;
		LARGE_INTEGER iSeek;
		DWORD iRead;

		sPathW = xrtUTF8to16((str)sPathVolume, 0, NULL);
		if ( sPathW == NULL ) {
			xpkFreeInternal(sPathVolume);
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}

		hFile = CreateFileW(sPathW, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		xrtFree(sPathW);
		xpkFreeInternal(sPathVolume);
		if ( hFile == INVALID_HANDLE_VALUE ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}

		iSeek.QuadPart = (LONGLONG)iOffsetVolume;
		if ( !SetFilePointerEx(hFile, iSeek, NULL, FILE_BEGIN) ) {
			CloseHandle(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoSeek);
		}

		iRead = 0;
		if ( !ReadFile(hFile, pData, (DWORD)iSize, &iRead, NULL) || iRead != (DWORD)iSize ) {
			CloseHandle(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
		}

		CloseHandle(hFile);
	}
#else
	{
		int hFile;
		ssize_t iRead;

		hFile = open(sPathVolume, O_RDONLY);
		xpkFreeInternal(sPathVolume);
		if ( hFile < 0 ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		if ( lseek(hFile, (off_t)iOffsetVolume, SEEK_SET) < 0 ) {
			close(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoSeek);
		}
		iRead = read(hFile, pData, (size_t)iSize);
		if ( iRead != (ssize_t)iSize ) {
			close(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
		}
		close(hFile);
	}
#endif

	return XPK_OK;
}

// 写入分卷片段文本
static inline int procXpkWriteVolumePartText(xpkObject objXpk, const char* sPathPackage, uint32_t iVolume, uint64_t iOffsetVolume, const void* pData, uint32_t iSize)
{
	char* sPathVolume;

	if ( sPathPackage == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize == 0 ) {
		return XPK_OK;
	}

	sPathVolume = procXpkVolumePathDupText(sPathPackage, iVolume);
	if ( sPathVolume == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

#if defined(_WIN32) || defined(_WIN64)
	{
		u16str sPathW;
		HANDLE hFile;
		LARGE_INTEGER iSeek;
		DWORD iWrite;

		sPathW = xrtUTF8to16((str)sPathVolume, 0, NULL);
		if ( sPathW == NULL ) {
			xpkFreeInternal(sPathVolume);
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}

		hFile = CreateFileW(sPathW, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		xrtFree(sPathW);
		xpkFreeInternal(sPathVolume);
		if ( hFile == INVALID_HANDLE_VALUE ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}

		iSeek.QuadPart = (LONGLONG)iOffsetVolume;
		if ( !SetFilePointerEx(hFile, iSeek, NULL, FILE_BEGIN) ) {
			CloseHandle(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoSeek);
		}

		iWrite = 0;
		if ( !WriteFile(hFile, pData, (DWORD)iSize, &iWrite, NULL) || iWrite != (DWORD)iSize ) {
			CloseHandle(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}

		CloseHandle(hFile);
	}
#else
	{
		int hFile;
		ssize_t iWrite;

		hFile = open(sPathVolume, O_RDWR | O_CREAT, 0644);
		xpkFreeInternal(sPathVolume);
		if ( hFile < 0 ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		if ( lseek(hFile, (off_t)iOffsetVolume, SEEK_SET) < 0 ) {
			close(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoSeek);
		}
		iWrite = write(hFile, pData, (size_t)iSize);
		if ( iWrite != (ssize_t)iSize ) {
			close(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}
		close(hFile);
	}
#endif

	return XPK_OK;
}

// 原始读取
static inline int procXpkRawRead(xpkObject objXpk, uint64_t iOffset, void* pData, uint32_t iSize)
{
	uint8_t* pCur;
	uint64_t iOffsetVolume;
	uint64_t iRemainVolume;
	uint32_t iVolumeSize;
	uint32_t iVolume;
	uint32_t iSizePart;
	int iRet;

	if ( iSize == 0 ) {
		return XPK_OK;
	}
	if ( pData == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iVolumeSize = procXpkAppliedVolumeSize(objXpk);
	if ( iVolumeSize == 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	pCur = (uint8_t*)pData;
	while ( iSize > 0 ) {
		iVolume = (uint32_t)(iOffset / iVolumeSize);
		iOffsetVolume = iOffset % iVolumeSize;
		iRemainVolume = (uint64_t)iVolumeSize - iOffsetVolume;
		iSizePart = (uint32_t)((iRemainVolume < (uint64_t)iSize) ? iRemainVolume : (uint64_t)iSize);

		iRet = procXpkReadVolumePartText(objXpk, objXpk->sPathPackage, iVolume, iOffsetVolume, pCur, iSizePart);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		pCur += iSizePart;
		iOffset += iSizePart;
		iSize -= iSizePart;
	}

	return XPK_OK;
}

// 原始写入
static inline int procXpkRawWrite(xpkObject objXpk, uint64_t iOffset, const void* pData, uint32_t iSize)
{
	const uint8_t* pCur;
	uint64_t iOffsetVolume;
	uint64_t iRemainVolume;
	uint32_t iVolumeSize;
	uint32_t iVolume;
	uint32_t iSizePart;
	int iRet;

	if ( iSize == 0 ) {
		return XPK_OK;
	}
	if ( pData == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iVolumeSize = procXpkAppliedVolumeSize(objXpk);
	if ( iVolumeSize == 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	pCur = (const uint8_t*)pData;
	while ( iSize > 0 ) {
		iVolume = (uint32_t)(iOffset / iVolumeSize);
		iOffsetVolume = iOffset % iVolumeSize;
		iRemainVolume = (uint64_t)iVolumeSize - iOffsetVolume;
		iSizePart = (uint32_t)((iRemainVolume < (uint64_t)iSize) ? iRemainVolume : (uint64_t)iSize);

		iRet = procXpkWriteVolumePartText(objXpk, objXpk->sPathPackage, iVolume, iOffsetVolume, pCur, iSizePart);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		pCur += iSizePart;
		iOffset += iSizePart;
		iSize -= iSizePart;
	}

	return XPK_OK;
}

// 读取按位置分配
static inline int procXpkReadAtAlloc(xpkObject objXpk, xfile hFile, uint64_t iOffset, uint32_t iSize, void** pDataRet)
{
	size_t iRead;
	void* pData;
	int iRet;

	if ( pDataRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	*pDataRet = NULL;
	if ( iSize == 0 ) {
		return XPK_OK;
	}

	if ( procXpkAppliedVolumeMode(objXpk) ) {
		pData = xpkAllocInternal(iSize);
		if ( pData == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}

		iRet = procXpkRawRead(objXpk, iOffset, pData, iSize);
		if ( iRet != XPK_OK ) {
			xpkFreeInternal(pData);
			return iRet;
		}

		*pDataRet = pData;
		return XPK_OK;
	}

	if ( hFile == NULL ) {
		iRet = procXpkOpenVolumeText(objXpk, objXpk->sPathPackage, 0, TRUE, &hFile);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		iRet = procXpkSeekFile(objXpk, hFile, iOffset);
		if ( iRet != XPK_OK ) {
			xrtClose(hFile);
			return iRet;
		}
		pData = xpkAllocInternal(iSize);
		if ( pData == NULL ) {
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		iRead = xrtGetBuffer(hFile, pData, iSize);
		xrtClose(hFile);
	} else {
		iRet = procXpkSeekFile(objXpk, hFile, iOffset);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		pData = xpkAllocInternal(iSize);
		if ( pData == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		iRead = xrtGetBuffer(hFile, pData, iSize);
	}

	if ( (pData == NULL) || (iRead != iSize) ) {
		if ( pData != NULL ) {
			xpkFreeInternal(pData);
		}
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
	}

	*pDataRet = pData;
	return XPK_OK;
}

// 读取按位置缓冲
static inline int procXpkReadAtBuffer(xpkObject objXpk, xfile hFile, uint64_t iOffset, void* pData, uint32_t iSize)
{
	size_t iRead;
	int iRet;

	if ( objXpk == NULL || pData == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize == 0 ) {
		return XPK_OK;
	}

	if ( procXpkAppliedVolumeMode(objXpk) ) {
		return procXpkRawRead(objXpk, iOffset, pData, iSize);
	}

	if ( hFile == NULL ) {
		iRet = procXpkOpenVolumeText(objXpk, objXpk->sPathPackage, 0, TRUE, &hFile);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		iRet = procXpkSeekFile(objXpk, hFile, iOffset);
		if ( iRet != XPK_OK ) {
			xrtClose(hFile);
			return iRet;
		}
		iRead = xrtGetBuffer(hFile, pData, iSize);
		xrtClose(hFile);
	} else {
		iRet = procXpkSeekFile(objXpk, hFile, iOffset);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		iRead = xrtGetBuffer(hFile, pData, iSize);
	}

	if ( iRead != iSize ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
	}

	return XPK_OK;
}

// 写入按位置
static inline int procXpkWriteAt(xpkObject objXpk, xfile hFile, uint64_t iOffset, const void* pData, uint32_t iSize)
{
	size_t iWrite;
	int iRet;

	if ( iSize == 0 ) {
		return XPK_OK;
	}
	if ( pData == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	if ( procXpkAppliedVolumeMode(objXpk) ) {
		(void)hFile;
		return procXpkRawWrite(objXpk, iOffset, pData, iSize);
	}

	if ( hFile == NULL ) {
		iRet = procXpkOpenVolumeText(objXpk, objXpk->sPathPackage, 0, FALSE, &hFile);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		iRet = procXpkSeekFile(objXpk, hFile, iOffset);
		if ( iRet != XPK_OK ) {
			xrtClose(hFile);
			return iRet;
		}

		iWrite = xrtPut(hFile, (ptr)pData, iSize);
		xrtClose(hFile);
	} else {
		iRet = procXpkSeekFile(objXpk, hFile, iOffset);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		iWrite = xrtPut(hFile, (ptr)pData, iSize);
	}

	if ( iWrite != iSize ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	return XPK_OK;
}

// 在指定位置设置文件 EOF
static inline int procXpkSetEOFAt(xpkObject objXpk, xfile hFile, uint64_t iOffset)
{
	uint32_t iVolumeLast;
	uint32_t iVolume;
	uint32_t iVolumeSize;
	uint64_t iLastSize;
	int iRet;

	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		if ( hFile == NULL ) {
			iRet = procXpkOpenVolumeText(objXpk, objXpk->sPathPackage, 0, FALSE, &hFile);
			if ( iRet != XPK_OK ) {
				return iRet;
			}
			iRet = procXpkSeekFile(objXpk, hFile, iOffset);
			if ( iRet != XPK_OK ) {
				xrtClose(hFile);
				return iRet;
			}
			if ( !xrtSetEOF(hFile) ) {
				xrtClose(hFile);
				return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
			}
			xrtClose(hFile);
			return XPK_OK;
		}

		iRet = procXpkSeekFile(objXpk, hFile, iOffset);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		if ( !xrtSetEOF(hFile) ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}

		return XPK_OK;
	}
	iVolumeSize = procXpkAppliedVolumeSize(objXpk);
	if ( iVolumeSize == 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	if ( iOffset == 0 ) {
		iVolumeLast = 0;
		iLastSize = 0;
	} else {
		iVolumeLast = (uint32_t)((iOffset - 1) / iVolumeSize);
		iLastSize = iOffset - ((uint64_t)iVolumeLast * (uint64_t)iVolumeSize);
	}

	for ( iVolume = 0; iVolume < iVolumeLast; iVolume++ ) {
		hFile = NULL;
		iRet = procXpkOpenVolumeText(objXpk, objXpk->sPathPackage, iVolume, FALSE, &hFile);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		iRet = procXpkSeekFile(objXpk, hFile, iVolumeSize);
		if ( iRet != XPK_OK ) {
			xrtClose(hFile);
			return iRet;
		}
		if ( !xrtSetEOF(hFile) ) {
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}

		xrtClose(hFile);
	}

	hFile = NULL;
	iRet = procXpkOpenVolumeText(objXpk, objXpk->sPathPackage, iVolumeLast, FALSE, &hFile);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = procXpkSeekFile(objXpk, hFile, iLastSize);
	if ( iRet != XPK_OK ) {
		xrtClose(hFile);
		return iRet;
	}
	if ( !xrtSetEOF(hFile) ) {
		xrtClose(hFile);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	xrtClose(hFile);
	return procXpkDeleteVolumeFilesText(objXpk, objXpk->sPathPackage, iVolumeLast + 1);
}


/* ===== File: src/format/layout.h ===== */

/*
	xPack 磁盘布局模块

	负责包头与条目表的编解码、布局校验和默认布局初始化。
*/

// 写入 32 位小端值
static inline void procXpkWrite32LE(uint8_t* pBuf, uint32_t iValue)
{
	pBuf[0] = (uint8_t)(iValue & 0xFFu);
	pBuf[1] = (uint8_t)((iValue >> 8) & 0xFFu);
	pBuf[2] = (uint8_t)((iValue >> 16) & 0xFFu);
	pBuf[3] = (uint8_t)((iValue >> 24) & 0xFFu);
}

// 写入 64 位小端值
static inline void procXpkWrite64LE(uint8_t* pBuf, uint64_t iValue)
{
	procXpkWrite32LE(pBuf, (uint32_t)(iValue & 0xFFFFFFFFu));
	procXpkWrite32LE(pBuf + 4, (uint32_t)(iValue >> 32));
}

// 读取 32 位小端值
static inline uint32_t procXpkRead32LE(const uint8_t* pBuf)
{
	return ((uint32_t)pBuf[0]) |
		((uint32_t)pBuf[1] << 8) |
		((uint32_t)pBuf[2] << 16) |
		((uint32_t)pBuf[3] << 24);
}

// 读取 64 位小端值
static inline uint64_t procXpkRead64LE(const uint8_t* pBuf)
{
	return ((uint64_t)procXpkRead32LE(pBuf)) |
		((uint64_t)procXpkRead32LE(pBuf + 4) << 32);
}

// 应用包类型默认布局
static inline int procXpkApplyPackType(xpkObject objXpk, xpkPackType iType)
{
	uint32_t iInfoExtSize;

	iInfoExtSize = procXpkInfoExtSizeByPackType(iType);
	if ( iInfoExtSize == UINT32_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	objXpk->objHead.packType = (uint32_t)iType;
	objXpk->objHead.infoExtSize = iInfoExtSize;
	return XPK_OK;
}

// 校验包头布局
static inline int procXpkValidateHead(xpkObject objXpk, const xpkHead* pHead)
{
	uint32_t iInfoExtSize;

	if ( pHead->fileHead != XPK_FILE_HEAD ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadHead);
	}
	if ( pHead->packType > XPK_PACK_WIN32 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iInfoExtSize = procXpkInfoExtSizeByPackType((xpkPackType)pHead->packType);
	if ( iInfoExtSize == UINT32_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pHead->infoExtSize != iInfoExtSize ) {
		if ( pHead->packType == XPK_PACK_CORE ) {
			if ( pHead->infoExtSize > 0x3FFFFu ) {
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
		} else {
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
	}
	if ( pHead->dataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pHead->volumeMode ) {
		if ( pHead->volumeSize < XPK_VOLUME_MIN ) {
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
	}
	if ( (!pHead->volumeMode) && (pHead->volumeSize != 0) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	return XPK_OK;
}

// 编码包头
static inline void procXpkEncodeHead(const xpkHead* pHead, uint8_t sBuf[XPK_HEAD_SIZE])
{
	uint32_t iFlags1;
	uint32_t iFlags2;

	memset(sBuf, 0, XPK_HEAD_SIZE);

	iFlags1 = (pHead->packType & 0x3u) |
		((pHead->defComp & 0xFu) << 2) |
		((pHead->metaComp & 0xFu) << 6) |
		((pHead->infoComp & 0xFu) << 10) |
		((pHead->infoExtSize & 0x3FFFFu) << 14);

	iFlags2 = (pHead->solidMode & 0x1u) |
		((pHead->volumeMode & 0x1u) << 1);

	procXpkWrite32LE(sBuf + 0, pHead->fileHead);
	procXpkWrite32LE(sBuf + 4, pHead->fileCount);
	procXpkWrite32LE(sBuf + 8, iFlags1);
	procXpkWrite32LE(sBuf + 12, iFlags2);
	procXpkWrite64LE(sBuf + 16, pHead->dataOffset);
	procXpkWrite32LE(sBuf + 24, pHead->volumeSize);
	procXpkWrite32LE(sBuf + 28, pHead->metaRawSize);
	procXpkWrite32LE(sBuf + 32, pHead->metaCompSize);
	procXpkWrite32LE(sBuf + 36, pHead->metaHash);
	procXpkWrite32LE(sBuf + 40, pHead->infoCompSize);
	procXpkWrite32LE(sBuf + 44, pHead->infoHash);
	procXpkWrite64LE(sBuf + 48, (uint64_t)pHead->createTime);
	procXpkWrite64LE(sBuf + 56, (uint64_t)pHead->changeTime);
}

// 解码包头
static inline void procXpkDecodeHead(const uint8_t sBuf[XPK_HEAD_SIZE], xpkHead* pHead)
{
	uint32_t iFlags1;
	uint32_t iFlags2;

	memset(pHead, 0, sizeof(*pHead));

	iFlags1 = procXpkRead32LE(sBuf + 8);
	iFlags2 = procXpkRead32LE(sBuf + 12);

	pHead->fileHead = procXpkRead32LE(sBuf + 0);
	pHead->fileCount = procXpkRead32LE(sBuf + 4);
	pHead->packType = iFlags1 & 0x3u;
	pHead->defComp = (iFlags1 >> 2) & 0xFu;
	pHead->metaComp = (iFlags1 >> 6) & 0xFu;
	pHead->infoComp = (iFlags1 >> 10) & 0xFu;
	pHead->infoExtSize = (iFlags1 >> 14) & 0x3FFFFu;
	pHead->solidMode = iFlags2 & 0x1u;
	pHead->volumeMode = (iFlags2 >> 1) & 0x1u;
	pHead->dataOffset = procXpkRead64LE(sBuf + 16);
	pHead->volumeSize = procXpkRead32LE(sBuf + 24);
	pHead->metaRawSize = procXpkRead32LE(sBuf + 28);
	pHead->metaCompSize = procXpkRead32LE(sBuf + 32);
	pHead->metaHash = procXpkRead32LE(sBuf + 36);
	pHead->infoCompSize = procXpkRead32LE(sBuf + 40);
	pHead->infoHash = procXpkRead32LE(sBuf + 44);
	pHead->createTime = (xtime)procXpkRead64LE(sBuf + 48);
	pHead->changeTime = (xtime)procXpkRead64LE(sBuf + 56);
}

// 编码条目表
static inline int procXpkEncodeEntryTable(xpkObject objXpk, void** pDataRet, uint32_t* pSizeRet)
{
	uint32_t iPos;
	uint32_t iStride;
	uint64_t iRawSize64;
	uint32_t iRawSize;
	uint8_t* pData;
	uint8_t* pRow;
	xpkEntry* pEntry;
	size_t iPathLen;

	if ( (pDataRet == NULL) || (pSizeRet == NULL) ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	*pDataRet = NULL;
	*pSizeRet = 0;
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}

	iStride = procXpkEntryStride(&objXpk->objHead);
	iRawSize64 = (uint64_t)objXpk->iEntryCount * (uint64_t)iStride;
	if ( iRawSize64 > UINT32_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	iRawSize = (uint32_t)iRawSize64;
	if ( iRawSize == 0 ) {
		return XPK_OK;
	}

	pData = (uint8_t*)xpkAllocInternal(iRawSize);
	if ( pData == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	memset(pData, 0, iRawSize);

	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			xpkFreeInternal(pData);
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( !procXpkEntryDeleted(pEntry) ) {
			if ( procXpkValidateLiveEntryLookup(objXpk, pEntry) != XPK_OK ) {
				xpkFreeInternal(pData);
				return xpkLastError(objXpk);
			}
		}

		pRow = pData + ((iPos - 1) * iStride);
		procXpkWrite32LE(pRow + 0, pEntry->iFlag);
		procXpkWrite32LE(pRow + 4, pEntry->iFileHash);
		procXpkWrite64LE(pRow + 8, pEntry->iDataOffset);
		procXpkWrite64LE(pRow + 16, pEntry->iDataSize);
		procXpkWrite64LE(pRow + 24, pEntry->iFileSize);

		if ( (objXpk->objHead.packType == XPK_PACK_CORE) && (objXpk->objHead.infoExtSize > 0) ) {
			if ( pEntry->pInfoExt != NULL ) {
				memcpy(pRow + 32, pEntry->pInfoExt, objXpk->objHead.infoExtSize);
			}
		} else if ( objXpk->objHead.packType == XPK_PACK_INDEX ) {
			procXpkWrite64LE(pRow + 32, (uint64_t)pEntry->iFileIndex);
		} else if ( (objXpk->objHead.packType == XPK_PACK_LINUX) || (objXpk->objHead.packType == XPK_PACK_WIN32) ) {
			if ( (pEntry->sPath == NULL) || (pEntry->sPath[0] == '\0') ) {
				xpkFreeInternal(pData);
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}

			iPathLen = strlen(pEntry->sPath);
			if ( iPathLen >= XPK_PATH_BYTES ) {
				xpkFreeInternal(pData);
				return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPathTooLong);
			}
			memcpy(pRow + 32, pEntry->sPath, iPathLen);
			procXpkWrite32LE(pRow + 32 + XPK_PATH_BYTES, pEntry->iPlatformAttr);
			procXpkWrite64LE(pRow + 32 + XPK_PATH_BYTES + 4, pEntry->tCreateTime);
			procXpkWrite64LE(pRow + 32 + XPK_PATH_BYTES + 12, pEntry->tModifyTime);
			procXpkWrite64LE(pRow + 32 + XPK_PATH_BYTES + 20, pEntry->tAccessTime);
		}
	}

	*pDataRet = pData;
	*pSizeRet = iRawSize;
	return XPK_OK;
}

// 解码条目表
static inline int procXpkDecodeEntryTable(xpkObject objXpk, const void* pData, uint32_t iSize)
{
	uint32_t iPos;
	uint32_t iStride;
	const uint8_t* pRow;
	xpkEntry objEntry;
	size_t iPathLen;
	int iRet;

	if ( iSize == 0 ) {
		procXpkResetEntries(objXpk);
		return XPK_OK;
	}
	if ( pData == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	iStride = procXpkEntryStride(&objXpk->objHead);
	if ( ((uint64_t)objXpk->objHead.fileCount * (uint64_t)iStride) != iSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	procXpkResetEntries(objXpk);
	for ( iPos = 0; iPos < objXpk->objHead.fileCount; iPos++ ) {
		memset(&objEntry, 0, sizeof(objEntry));
		pRow = (const uint8_t*)pData + (iPos * iStride);

		objEntry.iFlag = procXpkRead32LE(pRow + 0);
		objEntry.iFileHash = procXpkRead32LE(pRow + 4);
		objEntry.iDataOffset = procXpkRead64LE(pRow + 8);
		objEntry.iDataSize = procXpkRead64LE(pRow + 16);
		objEntry.iFileSize = procXpkRead64LE(pRow + 24);
		objEntry.bStored = TRUE;

		if ( (objXpk->objHead.packType == XPK_PACK_CORE) && (objXpk->objHead.infoExtSize > 0) ) {
			objEntry.pInfoExt = procXpkAllocInfoExt(objXpk);
			if ( objEntry.pInfoExt == NULL ) {
				procXpkResetEntries(objXpk);
				return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			}
			memcpy(objEntry.pInfoExt, pRow + 32, objXpk->objHead.infoExtSize);
		} else if ( objXpk->objHead.packType == XPK_PACK_INDEX ) {
			objEntry.iFileIndex = (int64_t)procXpkRead64LE(pRow + 32);
		} else if ( (objXpk->objHead.packType == XPK_PACK_LINUX) || (objXpk->objHead.packType == XPK_PACK_WIN32) ) {
			for ( iPathLen = 0; iPathLen < XPK_PATH_BYTES; iPathLen++ ) {
				if ( pRow[32 + iPathLen] == '\0' ) {
					break;
				}
			}
			if ( iPathLen == 0 ) {
				procXpkResetEntries(objXpk);
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
			if ( iPathLen >= XPK_PATH_BYTES ) {
				procXpkResetEntries(objXpk);
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
			objEntry.sPath = procXpkDupPathTextN(objXpk, (const char*)(pRow + 32), iPathLen);
			if ( (iPathLen > 0) && (objEntry.sPath == NULL) ) {
				procXpkResetEntries(objXpk);
				return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			}
			if ( objEntry.sPath == NULL || objEntry.sPath[0] == '\0' ) {
				procXpkFreeEntryOwned(&objEntry);
				procXpkResetEntries(objXpk);
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
			objEntry.iPlatformAttr = procXpkRead32LE(pRow + 32 + XPK_PATH_BYTES);
			objEntry.tCreateTime = procXpkRead64LE(pRow + 32 + XPK_PATH_BYTES + 4);
			objEntry.tModifyTime = procXpkRead64LE(pRow + 32 + XPK_PATH_BYTES + 12);
			objEntry.tAccessTime = procXpkRead64LE(pRow + 32 + XPK_PATH_BYTES + 20);
		}

		iRet = procXpkAppendEntryOwned(objXpk, &objEntry);
		if ( iRet != XPK_OK ) {
			procXpkFreeEntryOwned(&objEntry);
			procXpkResetEntries(objXpk);
			return iRet;
		}
	}

	iRet = procXpkRebuildLookup(objXpk);
	if ( iRet != XPK_OK ) {
		procXpkResetEntries(objXpk);
		return iRet;
	}

	return XPK_OK;
}


/* ===== File: src/service/open.h ===== */

/*
	xPack 打开加载模块

	负责读取包头、元数据、条目表并恢复运行时状态。
*/

// 加载并解析包文件
static inline int procXpkLoadPackage(xpkObject objXpk, const xpkOpenOptions* pOpt)
{
	xfile hFile;
	uint8_t* pHeadBuf;
	xpkMappedFile objMap;
	xpkHead objHead;
	void* pMetaComp;
	void* pEntryComp;
	void* pMetaRaw;
	void* pEntryRaw;
	uint64_t iEntryRawSize;
	uint64_t iMapSize;
	uint64_t iTailPos;
	uint32_t iLooseVolumeCount;
	int iRet;

	// 先处理目录冲突、缺失新建和空文件这几种快速返回分支。
	if ( xrtDirExists((str)objXpk->sPathPackage) ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	if ( !xrtFileExists(objXpk->sPathPackage) ) {
		if ( (pOpt != NULL) && pOpt->createIfMissing ) {
			iRet = procXpkEnsureVolumePathUnusedText(objXpk, objXpk->sPathPackage, sXpkErrorPackagePathExists);
			if ( iRet != XPK_OK ) {
				return iRet;
			}
			if ( objXpk->bReadonly ) {
				return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
			}
			objXpk->bDirtyHead = TRUE;
			procXpkClearError(objXpk);
			return XPK_OK;
		}
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}

	hFile = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
	if ( hFile == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	objXpk->iFileSize = xrtGetEOF(hFile);
	if ( objXpk->iFileSize == 0 ) {
		iLooseVolumeCount = 0;
		iRet = procXpkScanLooseVolumeFilesFromText(objXpk, objXpk->sPathPackage, 1, FALSE, &iLooseVolumeCount);
		if ( iRet != XPK_OK ) {
			xrtClose(hFile);
			return iRet;
		}
		if ( iLooseVolumeCount > 0 ) {
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_EXISTS, sXpkErrorPackagePathExists);
		}
		xrtClose(hFile);
		objXpk->bDirtyHead = TRUE;
		procXpkClearError(objXpk);
		return XPK_OK;
	}
	if ( objXpk->iFileSize < XPK_HEAD_SIZE ) {
		xrtClose(hFile);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadHead);
	}

	// 读取包头并校验布局字段，确认后再恢复运行时状态。
	iRet = procXpkReadAtAlloc(objXpk, hFile, 0, XPK_HEAD_SIZE, (void**)&pHeadBuf);
	if ( iRet != XPK_OK ) {
		xrtClose(hFile);
		return iRet;
	}

	procXpkDecodeHead(pHeadBuf, &objHead);
	xrtFree(pHeadBuf);

	iRet = procXpkValidateHead(objXpk, &objHead);
	if ( iRet != XPK_OK ) {
		xrtClose(hFile);
		return iRet;
	}

	objXpk->objHead = objHead;
	if ( objHead.volumeMode ) {
		iRet = procXpkCalcLogicalFileSize(objXpk, &objHead, &objXpk->iFileSize);
		if ( iRet != XPK_OK ) {
			xrtClose(hFile);
			return iRet;
		}
	}

	iEntryRawSize = procXpkEntryTableRawSize(&objHead);
	iTailPos = objHead.dataOffset + objHead.metaCompSize + objHead.infoCompSize;
	if ( iTailPos > objXpk->iFileSize ) {
		xrtClose(hFile);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( ((objHead.metaCompSize == 0) && (objHead.metaRawSize != 0)) ||
		((objHead.metaCompSize != 0) && (objHead.metaRawSize == 0)) ) {
		xrtClose(hFile);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( (objHead.infoCompSize == 0) && (iEntryRawSize != 0) ) {
		xrtClose(hFile);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( (objHead.infoCompSize != 0) && (iEntryRawSize == 0) ) {
		xrtClose(hFile);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	objXpk->iAppendPos = objHead.dataOffset;
	objXpk->iFileSize = iTailPos;
	procXpkMarkAppliedLayout(objXpk);
	procXpkMarkClean(objXpk);

	// 非分卷包优先走整文件映射，后续恢复元数据和条目表时能少一次额外拷贝。
	memset(&objMap, 0, sizeof(objMap));
	iMapSize = 0;
	if ( !objHead.volumeMode && ((objHead.metaCompSize > 0) || (objHead.infoCompSize > 0)) ) {
		iMapSize = xrtGetEOF(hFile);
		iRet = procXpkMapFileReadOnly(objXpk, hFile, iMapSize, &objMap);
		if ( iRet != XPK_OK ) {
			xrtClose(hFile);
			return iRet;
		}
	}

	// 先恢复包元数据，保证后续对象状态与磁盘内容一致。
	if ( objHead.metaCompSize > 0 ) {
		pMetaComp = NULL;
		pMetaRaw = NULL;
		if ( objMap.pView != NULL ) {
			if ( objHead.dataOffset > iMapSize || objHead.metaCompSize > (iMapSize - objHead.dataOffset) ) {
				procXpkUnmapFile(&objMap);
				xrtClose(hFile);
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
			iRet = procXpkCodecDecode(objXpk, (uint8_t)objHead.metaComp, (const uint8_t*)objMap.pView + objHead.dataOffset, objHead.metaCompSize, objHead.metaRawSize, &pMetaRaw);
		} else {
			iRet = procXpkReadAtAlloc(objXpk, hFile, objHead.dataOffset, objHead.metaCompSize, &pMetaComp);
			if ( iRet != XPK_OK ) {
				procXpkUnmapFile(&objMap);
				xrtClose(hFile);
				return iRet;
			}
			iRet = procXpkCodecDecode(objXpk, (uint8_t)objHead.metaComp, pMetaComp, objHead.metaCompSize, objHead.metaRawSize, &pMetaRaw);
			xrtFree(pMetaComp);
		}
		if ( iRet != XPK_OK ) {
			procXpkUnmapFile(&objMap);
			xrtClose(hFile);
			return iRet;
		}
		if ( xpkHash32Internal(pMetaRaw, objHead.metaRawSize) != objHead.metaHash ) {
			xpkFreeInternal(pMetaRaw);
			procXpkUnmapFile(&objMap);
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_HASH, sXpkErrorHashMismatch);
		}

		objXpk->pPackageMeta = pMetaRaw;
		objXpk->iPackageMetaSize = objHead.metaRawSize;
	}

	// 再恢复条目表，并重建条目数组与查找关系。
	if ( objHead.infoCompSize > 0 ) {
		pEntryComp = NULL;
		pEntryRaw = NULL;
		if ( objMap.pView != NULL ) {
			iTailPos = objHead.dataOffset + objHead.metaCompSize;
			if ( iTailPos > iMapSize || objHead.infoCompSize > (iMapSize - iTailPos) ) {
				procXpkUnmapFile(&objMap);
				xrtClose(hFile);
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
			iRet = procXpkCodecDecode(objXpk, (uint8_t)objHead.infoComp, (const uint8_t*)objMap.pView + iTailPos, objHead.infoCompSize, (uint32_t)iEntryRawSize, &pEntryRaw);
		} else {
			iRet = procXpkReadAtAlloc(objXpk, hFile, objHead.dataOffset + objHead.metaCompSize, objHead.infoCompSize, &pEntryComp);
			if ( iRet != XPK_OK ) {
				procXpkUnmapFile(&objMap);
				xrtClose(hFile);
				return iRet;
			}

			iRet = procXpkCodecDecode(objXpk, (uint8_t)objHead.infoComp, pEntryComp, objHead.infoCompSize, (uint32_t)iEntryRawSize, &pEntryRaw);
			xrtFree(pEntryComp);
		}
		if ( iRet != XPK_OK ) {
			procXpkUnmapFile(&objMap);
			xrtClose(hFile);
			return iRet;
		}
		if ( xpkHash32Internal(pEntryRaw, iEntryRawSize) != objHead.infoHash ) {
			xpkFreeInternal(pEntryRaw);
			procXpkUnmapFile(&objMap);
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_HASH, sXpkErrorHashMismatch);
		}

		iRet = procXpkDecodeEntryTable(objXpk, pEntryRaw, (uint32_t)iEntryRawSize);
		xpkFreeInternal(pEntryRaw);
		if ( iRet != XPK_OK ) {
			procXpkUnmapFile(&objMap);
			xrtClose(hFile);
			return iRet;
		}
	}

	// 统一收尾临时资源，并清空最后错误状态。
	procXpkUnmapFile(&objMap);
	xrtClose(hFile);
	procXpkClearError(objXpk);
	return XPK_OK;
}


/* ===== File: src/service/write.h ===== */

/*
	xPack 读写服务模块

	负责条目读写、延迟写入队列、压缩路径与 solid 数据处理。
*/

// 按包偏移分块写入数据
static inline int procXpkWriteAtChunkedPackage(xpkObject objXpk, xfile hFile, uint64_t iOffset, const void* pData, uint64_t iSize)
{
	const uint8_t* pCur;
	uint64_t iSizeLeft;
	uint32_t iChunkSize;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	pCur = (const uint8_t*)pData;
	iSizeLeft = iSize;
	while ( iSizeLeft > 0 ) {
		iChunkSize = (iSizeLeft > XPK_WRITE_FILE_CHUNK_SIZE) ? XPK_WRITE_FILE_CHUNK_SIZE : (uint32_t)iSizeLeft;
		iRet = procXpkWriteAt(objXpk, hFile, iOffset, pCur, iChunkSize);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		pCur += iChunkSize;
		iOffset += iChunkSize;
		iSizeLeft -= iChunkSize;
	}

	return XPK_OK;
}

// 复制源文件内容到包文件
static inline int procXpkCopySourceFileToPackage(xpkObject objXpk, xfile hFileSrc, xfile hFileDst, uint64_t iOffsetDst, uint64_t iSize)
{
	void* pChunk;
	uint64_t iSizeLeft;
	uint32_t iChunkSize;
	size_t iRead;
	int iRet;

	if ( objXpk == NULL || hFileSrc == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iSize == 0 ) {
		return XPK_OK;
	}

	pChunk = xpkAllocInternal(XPK_WRITE_FILE_CHUNK_SIZE);
	if ( pChunk == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iRet = procXpkSeekFile(objXpk, hFileSrc, 0);
	if ( iRet != XPK_OK ) {
		xpkFreeInternal(pChunk);
		return iRet;
	}

	iSizeLeft = iSize;
	while ( iSizeLeft > 0 ) {
		iChunkSize = (iSizeLeft > XPK_WRITE_FILE_CHUNK_SIZE) ? XPK_WRITE_FILE_CHUNK_SIZE : (uint32_t)iSizeLeft;
		iRead = xrtGetBuffer(hFileSrc, pChunk, iChunkSize);
		if ( iRead != iChunkSize ) {
			xpkFreeInternal(pChunk);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
		}

		iRet = procXpkWriteAt(objXpk, hFileDst, iOffsetDst, pChunk, iChunkSize);
		if ( iRet != XPK_OK ) {
			xpkFreeInternal(pChunk);
			return iRet;
		}

		iOffsetDst += iChunkSize;
		iSizeLeft -= iChunkSize;
	}

	xpkFreeInternal(pChunk);
	return XPK_OK;
}

// 获取写入队列数量
static inline int procXpkWriteQueueCount(xpkObject objXpk)
{
	if ( (objXpk == NULL) || (objXpk->pWriteQueue == NULL) ) {
		return 0;
	}
	return (int)objXpk->pWriteQueue->arrNode.Count;
}

// 初始化写入队列
static inline int procXpkInitWriteQueue(xpkObject objXpk)
{
	if ( objXpk->pWriteQueue != NULL ) {
		return XPK_OK;
	}

	objXpk->pWriteQueue = (xpkWriteQueue*)xpkAllocInternal(sizeof(*objXpk->pWriteQueue));
	if ( objXpk->pWriteQueue == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	memset(objXpk->pWriteQueue, 0, sizeof(*objXpk->pWriteQueue));
	xrtArrayInit(&objXpk->pWriteQueue->arrNode, sizeof(xpkWriteNode), XRT_OBJMODE_LOCAL);
	return XPK_OK;
}

// 释放写入队列
static inline void procXpkUnitWriteQueue(xpkObject objXpk)
{
	uint32_t iPos;
	xpkWriteNode* pNode;

	if ( (objXpk == NULL) || (objXpk->pWriteQueue == NULL) ) {
		return;
	}

	for ( iPos = 1; iPos <= objXpk->pWriteQueue->arrNode.Count; iPos++ ) {
		pNode = (xpkWriteNode*)xrtArrayGet(&objXpk->pWriteQueue->arrNode, iPos);
		if ( (pNode != NULL) && (pNode->pCompData != NULL) ) {
			xpkFreeInternal(pNode->pCompData);
			pNode->pCompData = NULL;
		}
	}

	xrtArrayUnit(&objXpk->pWriteQueue->arrNode);
	xpkFreeInternal(objXpk->pWriteQueue);
	objXpk->pWriteQueue = NULL;
}

// 查找写入队列节点
static inline xpkWriteNode* procXpkFindWriteNode(xpkObject objXpk, uint32_t iPos, uint32_t* pNodePosRet)
{
	uint32_t iNodePos;
	xpkWriteNode* pNode;

	if ( pNodePosRet != NULL ) {
		*pNodePosRet = 0;
	}
	if ( (objXpk == NULL) || (objXpk->pWriteQueue == NULL) ) {
		return NULL;
	}

	for ( iNodePos = 1; iNodePos <= objXpk->pWriteQueue->arrNode.Count; iNodePos++ ) {
		pNode = (xpkWriteNode*)xrtArrayGet(&objXpk->pWriteQueue->arrNode, iNodePos);
		if ( (pNode != NULL) && (pNode->iPos == iPos) ) {
			if ( pNodePosRet != NULL ) {
				*pNodePosRet = iNodePos;
			}
			return pNode;
		}
	}

	return NULL;
}

// 按节点位置移除延迟写入
static inline void procXpkRemoveQueuedWriteAt(xpkObject objXpk, uint32_t iNodePos)
{
	xpkWriteNode* pNode;

	if ( (objXpk == NULL) || (objXpk->pWriteQueue == NULL) || (iNodePos == 0) ) {
		return;
	}

	pNode = (xpkWriteNode*)xrtArrayGet(&objXpk->pWriteQueue->arrNode, iNodePos);
	if ( (pNode != NULL) && (pNode->pCompData != NULL) ) {
		xpkFreeInternal(pNode->pCompData);
		pNode->pCompData = NULL;
	}
	xrtArrayRemove(&objXpk->pWriteQueue->arrNode, iNodePos, 1);
}

// 按条目位置丢弃延迟写入
static inline void procXpkDropQueuedWrite(xpkObject objXpk, uint32_t iPos)
{
	uint32_t iNodePos;

	procXpkFindWriteNode(objXpk, iPos, &iNodePos);
	if ( iNodePos != 0 ) {
		procXpkRemoveQueuedWriteAt(objXpk, iNodePos);
	}
}

// 调整延迟写入位置
static inline void procXpkShiftQueuedWritePos(xpkObject objXpk, uint32_t iPosRemoved)
{
	uint32_t iNodePos;
	xpkWriteNode* pNode;

	if ( (objXpk == NULL) || (objXpk->pWriteQueue == NULL) ) {
		return;
	}

	for ( iNodePos = 1; iNodePos <= objXpk->pWriteQueue->arrNode.Count; iNodePos++ ) {
		pNode = (xpkWriteNode*)xrtArrayGet(&objXpk->pWriteQueue->arrNode, iNodePos);
		if ( (pNode != NULL) && (pNode->iPos > iPosRemoved) ) {
			pNode->iPos--;
		}
	}
}

// 立即写入原样文件并处理队列回退
static inline int procXpkWriteImmediateStoreFileWithQueuedFallback(xpkObject objXpk, xpkEntry* pEntry, const char* sSrcPath)
{
	xpkWriteNode* pNode;
	xpkWriteNode objNodeSaved;
	xpkWriteNode* pNodeRestore;
	uint32_t iNodePos;
	uint32_t iInsertPos;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}

	if ( procXpkWriteQueueCount(objXpk) == 0 ) {
		return procXpkWriteImmediateStoreFile(objXpk, pEntry, sSrcPath);
	}

	pNode = procXpkFindWriteNode(objXpk, pEntry->iPos, &iNodePos);
	if ( pNode == NULL || procXpkWriteQueueCount(objXpk) != 1 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}

	objNodeSaved = *pNode;
	pNode->pCompData = NULL;
	xrtArrayRemove(&objXpk->pWriteQueue->arrNode, iNodePos, 1);

	iRet = procXpkWriteImmediateStoreFile(objXpk, pEntry, sSrcPath);
	if ( iRet == XPK_OK ) {
		if ( objNodeSaved.pCompData != NULL ) {
			xpkFreeInternal(objNodeSaved.pCompData);
		}
		return XPK_OK;
	}

	iInsertPos = xrtArrayInsert(&objXpk->pWriteQueue->arrNode, iNodePos - 1, 1);
	if ( iInsertPos != iNodePos ) {
		if ( objNodeSaved.pCompData != NULL ) {
			xpkFreeInternal(objNodeSaved.pCompData);
		}
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	pNodeRestore = (xpkWriteNode*)xrtArrayGet(&objXpk->pWriteQueue->arrNode, iNodePos);
	if ( pNodeRestore == NULL ) {
		if ( objNodeSaved.pCompData != NULL ) {
			xpkFreeInternal(objNodeSaved.pCompData);
		}
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	*pNodeRestore = objNodeSaved;
	return iRet;
}

// 队列写入
static inline int procXpkQueueWrite(xpkObject objXpk, uint32_t iPos, uint8_t iLevel, void* pCompData, uint32_t iCompSize, uint64_t iRawSize)
{
	uint32_t iNodePos;
	xpkWriteNode* pNode;

	if ( pCompData == NULL && iCompSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	if ( procXpkInitWriteQueue(objXpk) != XPK_OK ) {
		if ( pCompData != NULL ) {
			xpkFreeInternal(pCompData);
		}
		return XPK_ERR_MEMORY;
	}

	pNode = procXpkFindWriteNode(objXpk, iPos, &iNodePos);
	if ( pNode == NULL ) {
		iNodePos = xrtArrayAppend(&objXpk->pWriteQueue->arrNode, 1);
		if ( iNodePos == 0 ) {
			if ( pCompData != NULL ) {
				xpkFreeInternal(pCompData);
			}
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		pNode = (xpkWriteNode*)xrtArrayGet(&objXpk->pWriteQueue->arrNode, iNodePos);
		if ( pNode == NULL ) {
			if ( pCompData != NULL ) {
				xpkFreeInternal(pCompData);
			}
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		memset(pNode, 0, sizeof(*pNode));
		pNode->iPos = iPos;
	} else if ( pNode->pCompData != NULL ) {
		xpkFreeInternal(pNode->pCompData);
	}

	pNode->iLevel = iLevel;
	pNode->iCompSize = iCompSize;
	pNode->iRawSize = iRawSize;
	pNode->pCompData = pCompData;
	return XPK_OK;
}

// 解析压缩级别
static inline int procXpkResolveCompLevel(xpkObject objXpk, const xpkWriteOptions* pOpt, uint8_t* pLevelRet)
{
	uint8_t iLevel;

	if ( pLevelRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	if ( pOpt == NULL ) {
		iLevel = (uint8_t)objXpk->objHead.defComp;
	} else if ( pOpt->compLevel == 0xFFu ) {
		iLevel = (uint8_t)objXpk->objHead.defComp;
	} else {
		iLevel = pOpt->compLevel;
	}

	if ( iLevel > 15 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	*pLevelRet = iLevel;
	return XPK_OK;
}

// 解析写入策略
static inline uint8_t procXpkResolveWritePolicy(xpkObject objXpk, const xpkWriteOptions* pOpt)
{
	if ( pOpt == NULL ) {
		return objXpk->bBufferedDefault ? XPK_WRITE_BUFFERED : XPK_WRITE_IMMEDIATE;
	}
	return pOpt->writePolicy;
}

// 打开映射源文件
static inline int procXpkOpenMappedSourceFile(xpkObject objXpk, const char* sPathFile, xfile* pFileRet, uint64_t* pSizeRet, xpkMappedFile* pMapRet)
{
	xfile hFile;
	uint64_t iFileSize;
	int iRet;

	if ( pFileRet != NULL ) {
		*pFileRet = NULL;
	}
	if ( pSizeRet != NULL ) {
		*pSizeRet = 0;
	}
	if ( pMapRet != NULL ) {
		memset(pMapRet, 0, sizeof(*pMapRet));
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( pFileRet == NULL || pSizeRet == NULL || pMapRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}

	hFile = xrtOpen((str)sPathFile, TRUE, XRT_CP_BINARY);
	if ( hFile == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iFileSize = xrtGetEOF(hFile);
	if ( iFileSize > UINT32_MAX ) {
		xrtClose(hFile);
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}

	iRet = procXpkMapFileReadOnly(objXpk, hFile, iFileSize, pMapRet);
	if ( iRet != XPK_OK ) {
		xrtClose(hFile);
		return iRet;
	}

	*pFileRet = hFile;
	*pSizeRet = iFileSize;
	return XPK_OK;
}

// 写入文件数据
static inline int procXpkWriteFileData(xpkObject objXpk, const char* sPathFile, const void* pData, uint64_t iSize)
{
	xfile hFile;
	size_t iChunkSize;
	const uint8_t* pCur;
	uint64_t iSizeLeft;

	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	hFile = xrtOpen((str)sPathFile, FALSE, XRT_CP_BINARY);
	if ( hFile == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	if ( procXpkSeekFile(objXpk, hFile, 0) != XPK_OK ) {
		xrtClose(hFile);
		return xpkLastError(objXpk);
	}

	pCur = (const uint8_t*)pData;
	iSizeLeft = iSize;
	while ( iSizeLeft > 0 ) {
		size_t iWrite;

		iChunkSize = (iSizeLeft > XPK_WRITE_FILE_CHUNK_SIZE) ? XPK_WRITE_FILE_CHUNK_SIZE : (size_t)iSizeLeft;
		iWrite = xrtPut(hFile, (ptr)pCur, iChunkSize);
		if ( iWrite != iChunkSize ) {
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}
		pCur += iChunkSize;
		iSizeLeft -= iChunkSize;
	}
	if ( !xrtSetEOF(hFile) ) {
		xrtClose(hFile);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	xrtClose(hFile);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 将包内范围复制到文件
static inline int procXpkCopyPackageRangeToFile(xpkObject objXpk, uint64_t iOffsetSrc, uint64_t iSize, const char* sPathFile)
{
	xfile hFileSrc;
	xfile hFileDst;
	void* pChunk;
	uint64_t iSizeLeft;
	uint32_t iChunkSize;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	hFileDst = xrtOpen((str)sPathFile, FALSE, XRT_CP_BINARY);
	if ( hFileDst == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	if ( procXpkSeekFile(objXpk, hFileDst, 0) != XPK_OK ) {
		xrtClose(hFileDst);
		return xpkLastError(objXpk);
	}

	hFileSrc = NULL;
	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileSrc = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}
	pChunk = xpkAllocInternal(XPK_WRITE_FILE_CHUNK_SIZE);
	if ( pChunk == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iSizeLeft = iSize;
	while ( iSizeLeft > 0 ) {
		size_t iWrite;

		iChunkSize = (iSizeLeft > XPK_WRITE_FILE_CHUNK_SIZE) ? XPK_WRITE_FILE_CHUNK_SIZE : (uint32_t)iSizeLeft;
		iRet = procXpkReadAtBuffer(objXpk, hFileSrc, iOffsetSrc, pChunk, iChunkSize);
		if ( iRet != XPK_OK ) {
			xpkFreeInternal(pChunk);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			xrtClose(hFileDst);
			return iRet;
		}

		iWrite = xrtPut(hFileDst, (ptr)pChunk, iChunkSize);
		if ( iWrite != iChunkSize ) {
			xpkFreeInternal(pChunk);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}

		iOffsetSrc += iChunkSize;
		iSizeLeft -= iChunkSize;
	}

	if ( !xrtSetEOF(hFileDst) ) {
		xpkFreeInternal(pChunk);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	xpkFreeInternal(pChunk);
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	xrtClose(hFileDst);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 将原样条目复制到文件
static inline int procXpkCopyStoredEntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	uint32_t iLevel;
	uint64_t iDataEnd;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( procXpkFindWriteNode(objXpk, pEntry->iPos, NULL) != NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}

	iLevel = (uint32_t)(pEntry->iFlag & XPK_FLAG_COMP_MASK);
	if ( iLevel != 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	iDataEnd = procXpkCurrentDataEnd(objXpk);
	if ( pEntry->iDataSize != pEntry->iFileSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataOffset > iDataEnd ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataSize > (iDataEnd - pEntry->iDataOffset) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	return procXpkCopyPackageRangeToFile(objXpk, pEntry->iDataOffset, pEntry->iDataSize, sPathFile);
}

// 将原样 LZ4 条目复制到文件
static inline int procXpkCopyStoredLz4EntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	xfile hFileSrc;
	xpkMappedFile objMap;
	void* pCompData;
	uint64_t iPkgSize;
	uint64_t iDataEnd;
	uint32_t iLevel;
	uint32_t iAlg;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( procXpkFindWriteNode(objXpk, pEntry->iPos, NULL) != NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}
	iDataEnd = procXpkCurrentDataEnd(objXpk);
	if ( pEntry->iDataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataOffset > iDataEnd ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataSize > (iDataEnd - pEntry->iDataOffset) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iLevel = (uint32_t)(pEntry->iFlag & XPK_FLAG_COMP_MASK);
	iAlg = procXpkCompLevelToAlg((uint8_t)iLevel);
	if ( iAlg != XPK_ALG_LZ4 && iAlg != XPK_ALG_LZ4HC ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		memset(&objMap, 0, sizeof(objMap));
		hFileSrc = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}

		iPkgSize = xrtGetEOF(hFileSrc);
		if ( pEntry->iDataOffset > iPkgSize || pEntry->iDataSize > (iPkgSize - pEntry->iDataOffset) ) {
			xrtClose(hFileSrc);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		iRet = procXpkMapFileReadOnly(objXpk, hFileSrc, iPkgSize, &objMap);
		xrtClose(hFileSrc);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		iRet = procXpkCopyDecodedLz4BlockToFile(objXpk, (uint8_t)iLevel, (const uint8_t*)objMap.pView + pEntry->iDataOffset, (uint32_t)pEntry->iDataSize, pEntry->iFileSize, sPathFile);
		procXpkUnmapFile(&objMap);
		return iRet;
	}

	if ( pEntry->iDataSize > UINT32_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}

	pCompData = NULL;
	iRet = procXpkReadAtAlloc(objXpk, NULL, pEntry->iDataOffset, (uint32_t)pEntry->iDataSize, &pCompData);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = procXpkCopyDecodedLz4BlockToFile(objXpk, (uint8_t)iLevel, pCompData, (uint32_t)pEntry->iDataSize, pEntry->iFileSize, sPathFile);
	xpkFreeInternal(pCompData);
	return iRet;
}

// 将原样 ZSTD 条目复制到文件
static inline int procXpkCopyStoredZstdEntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	xfile hFileSrc;
	xfile hFileDst;
	uint64_t iPkgSize;
	uint64_t iDataEnd;
	uint64_t iOffsetRead;
	uint64_t iSizeLeft;
	uint64_t iSizeWritten;
	ZSTD_DStream* pStream;
	ZSTD_inBuffer objIn;
	ZSTD_outBuffer objOut;
	void* pInBuf;
	void* pOutBuf;
	size_t iInCap;
	size_t iOutCap;
	size_t iZstdRet;
	uint32_t iLevel;
	uint32_t iChunkRead;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( procXpkFindWriteNode(objXpk, pEntry->iPos, NULL) != NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}
	if ( pEntry->iFileSize == 0 ) {
		return procXpkWriteFileData(objXpk, sPathFile, NULL, 0);
	}
	iDataEnd = procXpkCurrentDataEnd(objXpk);
	if ( pEntry->iDataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataOffset > iDataEnd ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataSize > (iDataEnd - pEntry->iDataOffset) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iLevel = (uint32_t)(pEntry->iFlag & XPK_FLAG_COMP_MASK);
	if ( procXpkCompLevelToAlg((uint8_t)iLevel) != XPK_ALG_ZSTD ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	hFileSrc = NULL;
	hFileDst = xrtOpen((str)sPathFile, FALSE, XRT_CP_BINARY);
	if ( hFileDst == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	if ( procXpkSeekFile(objXpk, hFileDst, 0) != XPK_OK ) {
		xrtClose(hFileDst);
		return xpkLastError(objXpk);
	}

	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileSrc = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		iPkgSize = xrtGetEOF(hFileSrc);
		if ( pEntry->iDataOffset > iPkgSize || pEntry->iDataSize > (iPkgSize - pEntry->iDataOffset) ) {
			xrtClose(hFileSrc);
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
	}

	iInCap = ZSTD_DStreamInSize();
	if ( iInCap == 0 ) {
		iInCap = 131072u;
	}
	iOutCap = ZSTD_DStreamOutSize();
	if ( iOutCap == 0 ) {
		iOutCap = 131072u;
	}

	pInBuf = xpkAllocInternal(iInCap);
	if ( pInBuf == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	pStream = ZSTD_createDStream();
	if ( pStream == NULL ) {
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iZstdRet = ZSTD_initDStream(pStream);
	if ( ZSTD_isError(iZstdRet) ) {
		ZSTD_freeDStream(pStream);
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
	}

	iOffsetRead = pEntry->iDataOffset;
	iSizeLeft = pEntry->iDataSize;
	iSizeWritten = 0;
	memset(&objIn, 0, sizeof(objIn));

	for ( ;; ) {
		if ( (objIn.pos == objIn.size) && (iSizeLeft > 0) ) {
			iChunkRead = (uint32_t)((iSizeLeft > (uint64_t)iInCap) ? iInCap : iSizeLeft);
			iRet = procXpkReadAtBuffer(objXpk, hFileSrc, iOffsetRead, pInBuf, iChunkRead);
			if ( iRet != XPK_OK ) {
				ZSTD_freeDStream(pStream);
				xpkFreeInternal(pOutBuf);
				xpkFreeInternal(pInBuf);
				if ( hFileSrc != NULL ) {
					xrtClose(hFileSrc);
				}
				xrtClose(hFileDst);
				return iRet;
			}

			objIn.src = pInBuf;
			objIn.size = iChunkRead;
			objIn.pos = 0;
			iOffsetRead += iChunkRead;
			iSizeLeft -= iChunkRead;
		} else if ( (objIn.pos == objIn.size) && (iSizeLeft == 0) ) {
			objIn.src = pInBuf;
			objIn.size = 0;
			objIn.pos = 0;
		}

		objOut.dst = pOutBuf;
		objOut.size = iOutCap;
		objOut.pos = 0;
		iZstdRet = ZSTD_decompressStream(pStream, &objOut, &objIn);
		if ( ZSTD_isError(iZstdRet) ) {
			ZSTD_freeDStream(pStream);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
		}

		if ( objOut.pos > 0 ) {
			if ( xrtPut(hFileDst, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
				ZSTD_freeDStream(pStream);
				xpkFreeInternal(pOutBuf);
				xpkFreeInternal(pInBuf);
				if ( hFileSrc != NULL ) {
					xrtClose(hFileSrc);
				}
				xrtClose(hFileDst);
				return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
			}
			iSizeWritten += objOut.pos;
		}

		if ( (iSizeLeft == 0) && (objIn.pos == objIn.size) && (iZstdRet == 0) ) {
			break;
		}
		if ( (iSizeLeft == 0) && (objIn.pos == objIn.size) && (objOut.pos == 0) && (iZstdRet > 0) ) {
			ZSTD_freeDStream(pStream);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
		}
	}

	if ( iSizeWritten != pEntry->iFileSize ) {
		ZSTD_freeDStream(pStream);
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( !xrtSetEOF(hFileDst) ) {
		ZSTD_freeDStream(pStream);
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	ZSTD_freeDStream(pStream);
	xpkFreeInternal(pOutBuf);
	xpkFreeInternal(pInBuf);
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	xrtClose(hFileDst);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 将原样 LZMA2 条目复制到文件
static inline int procXpkCopyStoredLzma2EntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	xfile hFileSrc;
	xfile hFileDst;
	uint64_t iPkgSize;
	uint64_t iDataEnd;
	uint64_t iOffsetRead;
	uint64_t iSizeLeft;
	uint64_t iSizeWritten;
	uint32_t iLevel;
	uint32_t iChunkRead;
	Byte iPropByte;
	CLzma2Dec objDec;
	ELzmaStatus iStatus;
	ELzmaFinishMode iFinishMode;
	void* pInBuf;
	void* pOutBuf;
	SizeT iInCap;
	SizeT iOutCap;
	SizeT iInputSize;
	SizeT iInputPos;
	SizeT iSrcLen;
	SizeT iDstLen;
	int iRet;
	SRes iLzmaRes;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( procXpkFindWriteNode(objXpk, pEntry->iPos, NULL) != NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}
	if ( pEntry->iFileSize == 0 ) {
		return procXpkWriteFileData(objXpk, sPathFile, NULL, 0);
	}
	if ( pEntry->iDataSize < 1 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	iDataEnd = procXpkCurrentDataEnd(objXpk);
	if ( pEntry->iDataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataOffset > iDataEnd ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataSize > (iDataEnd - pEntry->iDataOffset) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iLevel = (uint32_t)(pEntry->iFlag & XPK_FLAG_COMP_MASK);
	if ( procXpkCompLevelToAlg((uint8_t)iLevel) != XPK_ALG_LZMA2 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	hFileSrc = NULL;
	hFileDst = xrtOpen((str)sPathFile, FALSE, XRT_CP_BINARY);
	if ( hFileDst == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	if ( procXpkSeekFile(objXpk, hFileDst, 0) != XPK_OK ) {
		xrtClose(hFileDst);
		return xpkLastError(objXpk);
	}

	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileSrc = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		iPkgSize = xrtGetEOF(hFileSrc);
		if ( pEntry->iDataOffset > iPkgSize || pEntry->iDataSize > (iPkgSize - pEntry->iDataOffset) ) {
			xrtClose(hFileSrc);
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
	}

	iRet = procXpkReadAtBuffer(objXpk, hFileSrc, pEntry->iDataOffset, &iPropByte, 1);
	if ( iRet != XPK_OK ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return iRet;
	}

	iInCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	iOutCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	pInBuf = xpkAllocInternal(iInCap);
	if ( pInBuf == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	Lzma2Dec_Construct(&objDec);
	iLzmaRes = Lzma2Dec_Allocate(&objDec, iPropByte, &g_Alloc);
	if ( iLzmaRes != SZ_OK ) {
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		if ( iLzmaRes == SZ_ERROR_MEM ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
	}
	Lzma2Dec_Init(&objDec);

	iOffsetRead = pEntry->iDataOffset + 1;
	iSizeLeft = pEntry->iDataSize - 1;
	iSizeWritten = 0;
	iInputSize = 0;
	iInputPos = 0;

	for ( ;; ) {
		if ( iSizeWritten >= pEntry->iFileSize ) {
			break;
		}
		if ( (iInputPos == iInputSize) && (iSizeLeft > 0) ) {
			iChunkRead = (uint32_t)((iSizeLeft > (uint64_t)iInCap) ? iInCap : iSizeLeft);
			iRet = procXpkReadAtBuffer(objXpk, hFileSrc, iOffsetRead, pInBuf, iChunkRead);
			if ( iRet != XPK_OK ) {
				Lzma2Dec_Free(&objDec, &g_Alloc);
				xpkFreeInternal(pOutBuf);
				xpkFreeInternal(pInBuf);
				if ( hFileSrc != NULL ) {
					xrtClose(hFileSrc);
				}
				xrtClose(hFileDst);
				return iRet;
			}
			iInputSize = iChunkRead;
			iInputPos = 0;
			iOffsetRead += iChunkRead;
			iSizeLeft -= iChunkRead;
		}

		iDstLen = (SizeT)(((pEntry->iFileSize - iSizeWritten) > (uint64_t)iOutCap) ? iOutCap : (pEntry->iFileSize - iSizeWritten));
		iSrcLen = iInputSize - iInputPos;
		iFinishMode = (((iSizeWritten + iDstLen) == pEntry->iFileSize) && (iSizeLeft == 0)) ? LZMA_FINISH_END : LZMA_FINISH_ANY;
		iStatus = LZMA_STATUS_NOT_SPECIFIED;
		iLzmaRes = Lzma2Dec_DecodeToBuf(&objDec, (Byte*)pOutBuf, &iDstLen, (const Byte*)pInBuf + iInputPos, &iSrcLen, iFinishMode, &iStatus);
		if ( iLzmaRes != SZ_OK ) {
			Lzma2Dec_Free(&objDec, &g_Alloc);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
		}

		iInputPos += iSrcLen;
		if ( iDstLen > 0 ) {
			if ( xrtPut(hFileDst, (ptr)pOutBuf, iDstLen) != iDstLen ) {
				Lzma2Dec_Free(&objDec, &g_Alloc);
				xpkFreeInternal(pOutBuf);
				xpkFreeInternal(pInBuf);
				if ( hFileSrc != NULL ) {
					xrtClose(hFileSrc);
				}
				xrtClose(hFileDst);
				return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
			}
			iSizeWritten += iDstLen;
		}

		if ( (iDstLen == 0) && (iSrcLen == 0) ) {
			Lzma2Dec_Free(&objDec, &g_Alloc);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
		}
	}

	if ( iSizeWritten != pEntry->iFileSize || iSizeLeft != 0 || iInputPos != iInputSize ) {
		Lzma2Dec_Free(&objDec, &g_Alloc);
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( !xrtSetEOF(hFileDst) ) {
		Lzma2Dec_Free(&objDec, &g_Alloc);
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	Lzma2Dec_Free(&objDec, &g_Alloc);
	xpkFreeInternal(pOutBuf);
	xpkFreeInternal(pInBuf);
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	xrtClose(hFileDst);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 计算 Solid 原样原始大小
static inline int procXpkCalcSolidStoredRawSize(xpkObject objXpk, uint64_t* pSizeRet)
{
	uint64_t iRawSize;

	if ( pSizeRet != NULL ) {
		*pSizeRet = 0;
	}
	if ( objXpk == NULL || pSizeRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( !objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( objXpk->objHead.dataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( procXpkCalcSolidRawSize(objXpk, &iRawSize) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkSolidStoredCompLevel(objXpk) == 0 ) {
		if ( (objXpk->objHead.dataOffset - XPK_HEAD_SIZE) != iRawSize ) {
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
	}
	*pSizeRet = iRawSize;
	return XPK_OK;
}

// 将队列中的原样条目复制到文件
static inline int procXpkCopyQueuedStoredEntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	xpkWriteNode* pNode;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	pNode = procXpkFindWriteNode(objXpk, pEntry->iPos, NULL);
	if ( pNode == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( pNode->iLevel != 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( pNode->iCompSize != pNode->iRawSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iFileSize != pNode->iRawSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pNode->pCompData == NULL && pNode->iCompSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	return procXpkWriteFileData(objXpk, sPathFile, pNode->pCompData, pNode->iCompSize);
}

// 将解码后的 LZ4 块复制到文件
static inline int procXpkCopyDecodedLz4BlockToFile(xpkObject objXpk, uint8_t iLevel, const void* pCompData, uint32_t iCompSize, uint64_t iRawSize, const char* sPathFile)
{
	void* pRawData;
	int iDecRet;
	int iRet;
	uint32_t iAlg;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iRawSize == 0 ) {
		return procXpkWriteFileData(objXpk, sPathFile, NULL, 0);
	}
	if ( pCompData == NULL || iCompSize == 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( iRawSize > (uint64_t)INT_MAX || iCompSize > (uint32_t)INT_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}

	iAlg = procXpkCompLevelToAlg(iLevel);
	if ( iAlg != XPK_ALG_LZ4 && iAlg != XPK_ALG_LZ4HC ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	pRawData = xpkAllocInternal((size_t)iRawSize);
	if ( pRawData == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iDecRet = LZ4_decompress_safe((const char*)pCompData, (char*)pRawData, (int)iCompSize, (int)iRawSize);
	if ( iDecRet != (int)iRawSize ) {
		xpkFreeInternal(pRawData);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lz4 decompress failed");
	}

	iRet = procXpkWriteFileData(objXpk, sPathFile, pRawData, iRawSize);
	xpkFreeInternal(pRawData);
	return iRet;
}

// 将队列中的 LZ4 条目复制到文件
static inline int procXpkCopyQueuedLz4EntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	xpkWriteNode* pNode;
	uint32_t iAlg;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	pNode = procXpkFindWriteNode(objXpk, pEntry->iPos, NULL);
	if ( pNode == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	iAlg = procXpkCompLevelToAlg(pNode->iLevel);
	if ( iAlg != XPK_ALG_LZ4 && iAlg != XPK_ALG_LZ4HC ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( pNode->pCompData == NULL && pNode->iCompSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iFileSize != pNode->iRawSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	return procXpkCopyDecodedLz4BlockToFile(objXpk, pNode->iLevel, pNode->pCompData, pNode->iCompSize, pNode->iRawSize, sPathFile);
}

// 将队列中的 ZSTD 条目复制到文件
static inline int procXpkCopyQueuedZstdEntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	xpkWriteNode* pNode;
	xfile hFileDst;
	ZSTD_DStream* pStream;
	ZSTD_inBuffer objIn;
	ZSTD_outBuffer objOut;
	void* pOutBuf;
	size_t iOutCap;
	size_t iZstdRet;
	uint64_t iSizeWritten;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	pNode = procXpkFindWriteNode(objXpk, pEntry->iPos, NULL);
	if ( pNode == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( procXpkCompLevelToAlg(pNode->iLevel) != XPK_ALG_ZSTD ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( pNode->pCompData == NULL && pNode->iCompSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iFileSize != pNode->iRawSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iFileSize == 0 ) {
		return procXpkWriteFileData(objXpk, sPathFile, NULL, 0);
	}

	hFileDst = xrtOpen((str)sPathFile, FALSE, XRT_CP_BINARY);
	if ( hFileDst == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	if ( procXpkSeekFile(objXpk, hFileDst, 0) != XPK_OK ) {
		xrtClose(hFileDst);
		return xpkLastError(objXpk);
	}

	iOutCap = ZSTD_DStreamOutSize();
	if ( iOutCap == 0 ) {
		iOutCap = 131072u;
	}
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	pStream = ZSTD_createDStream();
	if ( pStream == NULL ) {
		xpkFreeInternal(pOutBuf);
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	iZstdRet = ZSTD_initDStream(pStream);
	if ( ZSTD_isError(iZstdRet) ) {
		ZSTD_freeDStream(pStream);
		xpkFreeInternal(pOutBuf);
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
	}

	objIn.src = pNode->pCompData;
	objIn.size = pNode->iCompSize;
	objIn.pos = 0;
	iSizeWritten = 0;
	for ( ;; ) {
		objOut.dst = pOutBuf;
		objOut.size = iOutCap;
		objOut.pos = 0;
		iZstdRet = ZSTD_decompressStream(pStream, &objOut, &objIn);
		if ( ZSTD_isError(iZstdRet) ) {
			ZSTD_freeDStream(pStream);
			xpkFreeInternal(pOutBuf);
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
		}
		if ( objOut.pos > 0 ) {
			if ( xrtPut(hFileDst, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
				ZSTD_freeDStream(pStream);
				xpkFreeInternal(pOutBuf);
				xrtClose(hFileDst);
				return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
			}
			iSizeWritten += objOut.pos;
		}
		if ( (objIn.pos == objIn.size) && (iZstdRet == 0) ) {
			break;
		}
		if ( (objIn.pos == objIn.size) && (objOut.pos == 0) && (iZstdRet > 0) ) {
			ZSTD_freeDStream(pStream);
			xpkFreeInternal(pOutBuf);
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
		}
	}

	if ( iSizeWritten != pEntry->iFileSize ) {
		ZSTD_freeDStream(pStream);
		xpkFreeInternal(pOutBuf);
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( !xrtSetEOF(hFileDst) ) {
		ZSTD_freeDStream(pStream);
		xpkFreeInternal(pOutBuf);
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	ZSTD_freeDStream(pStream);
	xpkFreeInternal(pOutBuf);
	xrtClose(hFileDst);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 将队列中的 LZMA2 条目复制到文件
static inline int procXpkCopyQueuedLzma2EntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	xpkWriteNode* pNode;
	xfile hFileDst;
	CLzma2Dec objDec;
	ELzmaStatus iStatus;
	ELzmaFinishMode iFinishMode;
	Byte iPropByte;
	void* pOutBuf;
	SizeT iOutCap;
	SizeT iInputPos;
	SizeT iInputSize;
	SizeT iSrcLen;
	SizeT iDstLen;
	uint64_t iSizeWritten;
	SRes iLzmaRes;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	pNode = procXpkFindWriteNode(objXpk, pEntry->iPos, NULL);
	if ( pNode == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( procXpkCompLevelToAlg(pNode->iLevel) != XPK_ALG_LZMA2 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( pNode->pCompData == NULL && pNode->iCompSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iFileSize != pNode->iRawSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iFileSize == 0 ) {
		return procXpkWriteFileData(objXpk, sPathFile, NULL, 0);
	}
	if ( pNode->iCompSize < 1 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	hFileDst = xrtOpen((str)sPathFile, FALSE, XRT_CP_BINARY);
	if ( hFileDst == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	if ( procXpkSeekFile(objXpk, hFileDst, 0) != XPK_OK ) {
		xrtClose(hFileDst);
		return xpkLastError(objXpk);
	}

	iOutCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iPropByte = ((const Byte*)pNode->pCompData)[0];
	Lzma2Dec_Construct(&objDec);
	iLzmaRes = Lzma2Dec_Allocate(&objDec, iPropByte, &g_Alloc);
	if ( iLzmaRes != SZ_OK ) {
		xpkFreeInternal(pOutBuf);
		xrtClose(hFileDst);
		if ( iLzmaRes == SZ_ERROR_MEM ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
	}
	Lzma2Dec_Init(&objDec);

	iInputPos = 1;
	iInputSize = pNode->iCompSize;
	iSizeWritten = 0;
	for ( ;; ) {
		if ( iSizeWritten >= pEntry->iFileSize ) {
			break;
		}

		iDstLen = (SizeT)(((pEntry->iFileSize - iSizeWritten) > (uint64_t)iOutCap) ? iOutCap : (pEntry->iFileSize - iSizeWritten));
		iSrcLen = iInputSize - iInputPos;
		iFinishMode = (((iSizeWritten + iDstLen) == pEntry->iFileSize) && (iInputPos + iSrcLen == iInputSize)) ? LZMA_FINISH_END : LZMA_FINISH_ANY;
		iStatus = LZMA_STATUS_NOT_SPECIFIED;
		iLzmaRes = Lzma2Dec_DecodeToBuf(&objDec, (Byte*)pOutBuf, &iDstLen, (const Byte*)pNode->pCompData + iInputPos, &iSrcLen, iFinishMode, &iStatus);
		if ( iLzmaRes != SZ_OK ) {
			Lzma2Dec_Free(&objDec, &g_Alloc);
			xpkFreeInternal(pOutBuf);
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
		}

		iInputPos += iSrcLen;
		if ( iDstLen > 0 ) {
			if ( xrtPut(hFileDst, (ptr)pOutBuf, iDstLen) != iDstLen ) {
				Lzma2Dec_Free(&objDec, &g_Alloc);
				xpkFreeInternal(pOutBuf);
				xrtClose(hFileDst);
				return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
			}
			iSizeWritten += iDstLen;
		}

		if ( (iDstLen == 0) && (iSrcLen == 0) ) {
			Lzma2Dec_Free(&objDec, &g_Alloc);
			xpkFreeInternal(pOutBuf);
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
		}
	}

	if ( iSizeWritten != pEntry->iFileSize || iInputPos != iInputSize ) {
		Lzma2Dec_Free(&objDec, &g_Alloc);
		xpkFreeInternal(pOutBuf);
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( !xrtSetEOF(hFileDst) ) {
		Lzma2Dec_Free(&objDec, &g_Alloc);
		xpkFreeInternal(pOutBuf);
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	Lzma2Dec_Free(&objDec, &g_Alloc);
	xpkFreeInternal(pOutBuf);
	xrtClose(hFileDst);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 写入立即
static inline int procXpkWriteImmediate(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel)
{
	xfile hFile;
	void* pCompData;
	uint32_t iCompSize;
	uint8_t iUsedLevel;
	int iRet;

	if ( procXpkWriteQueueCount(objXpk) > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}

	if ( iLevel > 0 ) {
		if ( procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZ4 || procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZ4HC ) {
			return procXpkWriteImmediateLz4Data(objXpk, pEntry, pData, iSize, iLevel);
		}
		if ( procXpkCompLevelToAlg(iLevel) == XPK_ALG_ZSTD ) {
			return procXpkWriteImmediateZstdData(objXpk, pEntry, pData, iSize, iLevel);
		}
		if ( procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZMA2 ) {
			return procXpkWriteImmediateLzma2Data(objXpk, pEntry, pData, iSize, iLevel);
		}
	}

	iRet = procXpkCodecEncode(objXpk, iLevel, pData, iSize, &pCompData, &iCompSize, &iUsedLevel);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	hFile = NULL;
	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFile = xrtOpen(objXpk->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFile == NULL ) {
			xpkFreeInternal(pCompData);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}

	iRet = procXpkWriteAt(objXpk, hFile, objXpk->iAppendPos, pCompData, iCompSize);
	if ( hFile != NULL ) {
		xrtClose(hFile);
	}
	if ( iRet != XPK_OK ) {
		xpkFreeInternal(pCompData);
		return iRet;
	}

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iUsedLevel;
	pEntry->iFileHash = (iSize > 0) ? xpkHash32Internal(pData, iSize) : 0;
	pEntry->iDataOffset = objXpk->iAppendPos;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iSize;
	objXpk->iAppendPos += iCompSize;
	if ( objXpk->iAppendPos > objXpk->iFileSize ) {
		objXpk->iFileSize = objXpk->iAppendPos;
	}

	xpkFreeInternal(pCompData);
	return XPK_OK;
}

// 写入缓冲
static inline int procXpkWriteBuffered(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel)
{
	if ( iSize > 0 ) {
		if ( procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZ4 || procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZ4HC ) {
			return procXpkWriteBufferedLz4Data(objXpk, pEntry, pData, iSize, iLevel);
		}
		if ( procXpkCompLevelToAlg(iLevel) == XPK_ALG_ZSTD ) {
			return procXpkWriteBufferedZstdData(objXpk, pEntry, pData, iSize, iLevel);
		}
		if ( procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZMA2 ) {
			return procXpkWriteBufferedLzma2Data(objXpk, pEntry, pData, iSize, iLevel);
		}
	}

	void* pCompData;
	uint32_t iCompSize;
	uint8_t iUsedLevel;
	int iRet;

	iRet = procXpkCodecEncode(objXpk, iLevel, pData, iSize, &pCompData, &iCompSize, &iUsedLevel);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = procXpkQueueWrite(objXpk, pEntry->iPos, iUsedLevel, pCompData, iCompSize, iSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iUsedLevel;
	pEntry->iFileHash = (iSize > 0) ? xpkHash32Internal(pData, iSize) : 0;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iSize;
	return XPK_OK;
}

// 写入临时路径复制
static inline char* procXpkWriteTempPathDup(xpkObject objXpk, const char* sTag)
{
	char sSuffix[128];
	char* sPathRet;
	uint64_t iStamp;
	uint64_t iTagObj;
	uint32_t iTry;
	int iSizePrint;

	if ( objXpk == NULL || objXpk->sPathPackage == NULL || sTag == NULL || sTag[0] == '\0' ) {
		procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
		return NULL;
	}

	iStamp = (uint64_t)xpkNowInternal();
	iTagObj = (uint64_t)(uintptr_t)objXpk;
	for ( iTry = 0; iTry < 32; iTry++ ) {
		iSizePrint = snprintf(
			sSuffix,
			sizeof(sSuffix),
			"%s.%llu.%llx.%u.tmp",
			sTag,
			(unsigned long long)iStamp,
			(unsigned long long)iTagObj,
			(unsigned int)iTry
		);
		if ( iSizePrint <= 0 || (size_t)iSizePrint >= sizeof(sSuffix) ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite), NULL;
		}

		sPathRet = procXpkPathSuffixDupText(objXpk->sPathPackage, sSuffix);
		if ( sPathRet == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory), NULL;
		}
		if ( !xrtPathExists((str)sPathRet) ) {
			return sPathRet;
		}

		xpkFreeInternal(sPathRet);
	}

	return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite), NULL;
}

// 缓冲写入 LZ4 数据
static inline int procXpkWriteBufferedLz4Data(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel)
{
	xfile hFileTmp;
	xpkMappedFile objMap;
	char* sPathTmp;
	void* pCompData;
	uint32_t iBound;
	uint32_t iCompSize;
	uint32_t iHash;
	int iAlg;
	int iCompRet;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iAlg = procXpkCompLevelToAlg(iLevel);
	if ( iAlg != XPK_ALG_LZ4 && iAlg != XPK_ALG_LZ4HC ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize == 0 ) {
		return procXpkWriteBuffered(objXpk, pEntry, pData, iSize, 0);
	}
	if ( iSize > INT_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}

	hFileTmp = NULL;
	memset(&objMap, 0, sizeof(objMap));
	sPathTmp = NULL;
	pCompData = NULL;
	iBound = 0;
	iCompSize = 0;
	iHash = xpkHash32Internal(pData, iSize);

	iBound = (uint32_t)LZ4_compressBound((int)iSize);
	sPathTmp = procXpkWriteTempPathDup(objXpk, ".lz4.data.queue");
	if ( sPathTmp == NULL ) {
		return xpkLastError(objXpk);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		xpkFreeInternal(sPathTmp);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	iRet = procXpkMapFileReadWrite(objXpk, hFileTmp, iBound, &objMap);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	if ( iAlg == XPK_ALG_LZ4 ) {
		iCompRet = LZ4_compress_fast((const char*)pData, (char*)objMap.pView, (int)iSize, (int)iBound, procXpkCompLevelToNative(iLevel));
	} else {
		iCompRet = LZ4_compress_HC((const char*)pData, (char*)objMap.pView, (int)iSize, (int)iBound, procXpkCompLevelToNative(iLevel));
	}
	if ( iCompRet <= 0 ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, (iAlg == XPK_ALG_LZ4) ? "lz4 compress failed" : "lz4hc compress failed");
		goto lblCleanup;
	}
	iCompSize = (uint32_t)iCompRet;
	if ( iCompSize >= iSize ) {
		procXpkUnmapFile(&objMap);
		iRet = XPK_OK;
		goto lblFallback;
	}

	procXpkUnmapFile(&objMap);
	iRet = procXpkSeekFile(objXpk, hFileTmp, iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}
	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	iRet = procXpkReadAtAlloc(objXpk, hFileTmp, 0, iCompSize, &pCompData);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iRet = procXpkQueueWrite(objXpk, pEntry->iPos, iLevel, pCompData, iCompSize, iSize);
	if ( iRet != XPK_OK ) {
		pCompData = NULL;
		goto lblCleanup;
	}
	pCompData = NULL;

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
	pEntry->iFileHash = iHash;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iSize;
	procXpkClearError(objXpk);
	iRet = XPK_OK;
	goto lblCleanup;

lblFallback:
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
		hFileTmp = NULL;
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
		sPathTmp = NULL;
	}
	return procXpkWriteBuffered(objXpk, pEntry, pData, iSize, 0);

lblCleanup:
	if ( pCompData != NULL ) {
		xpkFreeInternal(pCompData);
	}
	procXpkUnmapFile(&objMap);
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	return iRet;
}

// 缓冲写入 ZSTD 数据
static inline int procXpkWriteBufferedZstdData(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel)
{
	xfile hFileTmp;
	ZSTD_CCtx* pCtx;
	ZSTD_inBuffer objIn;
	ZSTD_outBuffer objOut;
	char* sPathTmp;
	void* pOutBuf;
	void* pCompData;
	const uint8_t* pCur;
	uint64_t iCompSize64;
	uint32_t iCompSize;
	uint32_t iHash;
	size_t iOutCap;
	size_t iChunkSize;
	size_t iSizeLeft;
	size_t iZstdRet;
	int bFallbackStore;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_ZSTD ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize == 0 ) {
		return procXpkWriteBuffered(objXpk, pEntry, pData, iSize, 0);
	}

	hFileTmp = NULL;
	pCtx = NULL;
	sPathTmp = NULL;
	pOutBuf = NULL;
	pCompData = NULL;
	iCompSize64 = 0;
	iCompSize = 0;
	iHash = xpkHash32Internal(pData, iSize);
	bFallbackStore = FALSE;

	sPathTmp = procXpkWriteTempPathDup(objXpk, ".zstd.data.queue");
	if ( sPathTmp == NULL ) {
		return xpkLastError(objXpk);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		xpkFreeInternal(sPathTmp);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iOutCap = ZSTD_CStreamOutSize();
	if ( iOutCap == 0 ) {
		iOutCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	}

	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	pCtx = ZSTD_createCCtx();
	if ( pCtx == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	ZSTD_CCtx_reset(pCtx, ZSTD_reset_session_only);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_checksumFlag, 0);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_strategy, (ZSTD_strategy)procXpkCompLevelToNative(iLevel));
	ZSTD_CCtx_setPledgedSrcSize(pCtx, (unsigned long long)iSize);

	pCur = (const uint8_t*)pData;
	iSizeLeft = iSize;
	while ( iSizeLeft > 0 ) {
		iChunkSize = (iSizeLeft > XPK_CODEC_STREAM_CHUNK_SIZE) ? XPK_CODEC_STREAM_CHUNK_SIZE : iSizeLeft;
		objIn.src = pCur;
		objIn.size = iChunkSize;
		objIn.pos = 0;
		while ( objIn.pos < objIn.size ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_continue);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize64 += objOut.pos;
				if ( iCompSize64 >= iSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
		}
		if ( bFallbackStore ) {
			break;
		}

		pCur += iChunkSize;
		iSizeLeft -= iChunkSize;
	}

	if ( !bFallbackStore ) {
		objIn.src = NULL;
		objIn.size = 0;
		objIn.pos = 0;
		for ( ;; ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_end);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize64 += objOut.pos;
				if ( iCompSize64 >= iSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
			if ( iZstdRet == 0 ) {
				break;
			}
		}
	}

	if ( bFallbackStore ) {
		iRet = XPK_OK;
		goto lblCleanup;
	}

	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	iCompSize = (uint32_t)iCompSize64;
	iRet = procXpkReadAtAlloc(objXpk, hFileTmp, 0, iCompSize, &pCompData);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iRet = procXpkQueueWrite(objXpk, pEntry->iPos, iLevel, pCompData, iCompSize, iSize);
	if ( iRet != XPK_OK ) {
		pCompData = NULL;
		goto lblCleanup;
	}
	pCompData = NULL;

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
	pEntry->iFileHash = iHash;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iSize;
	procXpkClearError(objXpk);
	iRet = XPK_OK;

lblCleanup:
	if ( pCompData != NULL ) {
		xpkFreeInternal(pCompData);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( pCtx != NULL ) {
		ZSTD_freeCCtx(pCtx);
	}
	if ( pOutBuf != NULL ) {
		xpkFreeInternal(pOutBuf);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	if ( bFallbackStore ) {
		return procXpkWriteBuffered(objXpk, pEntry, pData, iSize, 0);
	}
	return iRet;
}


// 立即写入 LZ4 数据
static inline int procXpkWriteImmediateLz4Data(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel)
{
	xfile hFileTmp;
	xfile hFileDst;
	xpkMappedFile objMap;
	char* sPathTmp;
	uint32_t iBound;
	uint32_t iCompSize;
	uint32_t iHash;
	int iAlg;
	int iCompRet;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_SOLID_DATA_WRITE, "solid mode does not allow file data writes");
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( procXpkWriteQueueCount(objXpk) > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}
	iAlg = procXpkCompLevelToAlg(iLevel);
	if ( iAlg != XPK_ALG_LZ4 && iAlg != XPK_ALG_LZ4HC ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize == 0 ) {
		return procXpkWriteImmediate(objXpk, pEntry, pData, iSize, 0);
	}
	if ( iSize > INT_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}

	hFileTmp = NULL;
	hFileDst = NULL;
	memset(&objMap, 0, sizeof(objMap));
	sPathTmp = NULL;
	iBound = 0;
	iCompSize = 0;
	iHash = xpkHash32Internal(pData, iSize);

	iBound = (uint32_t)LZ4_compressBound((int)iSize);
	sPathTmp = procXpkWriteTempPathDup(objXpk, ".lz4.data");
	if ( sPathTmp == NULL ) {
		return xpkLastError(objXpk);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		xpkFreeInternal(sPathTmp);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	iRet = procXpkMapFileReadWrite(objXpk, hFileTmp, iBound, &objMap);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	if ( iAlg == XPK_ALG_LZ4 ) {
		iCompRet = LZ4_compress_fast((const char*)pData, (char*)objMap.pView, (int)iSize, (int)iBound, procXpkCompLevelToNative(iLevel));
	} else {
		iCompRet = LZ4_compress_HC((const char*)pData, (char*)objMap.pView, (int)iSize, (int)iBound, procXpkCompLevelToNative(iLevel));
	}
	if ( iCompRet <= 0 ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, (iAlg == XPK_ALG_LZ4) ? "lz4 compress failed" : "lz4hc compress failed");
		goto lblCleanup;
	}
	iCompSize = (uint32_t)iCompRet;
	if ( iCompSize >= iSize ) {
		procXpkUnmapFile(&objMap);
		iRet = XPK_OK;
		goto lblFallback;
	}

	procXpkUnmapFile(&objMap);
	iRet = procXpkSeekFile(objXpk, hFileTmp, iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}
	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileDst = xrtOpen(objXpk->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
			goto lblCleanup;
		}
	}

	iRet = procXpkCopySourceFileToPackage(objXpk, hFileTmp, hFileDst, objXpk->iAppendPos, iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
	pEntry->iFileHash = iHash;
	pEntry->iDataOffset = objXpk->iAppendPos;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iSize;
	objXpk->iAppendPos += iCompSize;
	if ( objXpk->iAppendPos > objXpk->iFileSize ) {
		objXpk->iFileSize = objXpk->iAppendPos;
	}

	iRet = XPK_OK;
	goto lblCleanup;

lblFallback:
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
		hFileTmp = NULL;
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
		sPathTmp = NULL;
	}
	return procXpkWriteImmediate(objXpk, pEntry, pData, iSize, 0);

lblCleanup:
	procXpkUnmapFile(&objMap);
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	if ( iRet == XPK_OK ) {
		procXpkClearError(objXpk);
	}
	return iRet;
}

// 立即写入 ZSTD 数据
static inline int procXpkWriteImmediateZstdData(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel)
{
	xfile hFileTmp;
	xfile hFileDst;
	ZSTD_CCtx* pCtx;
	ZSTD_inBuffer objIn;
	ZSTD_outBuffer objOut;
	char* sPathTmp;
	void* pOutBuf;
	const uint8_t* pCur;
	uint64_t iCompSize64;
	uint32_t iCompSize;
	uint32_t iHash;
	size_t iOutCap;
	size_t iChunkSize;
	size_t iSizeLeft;
	size_t iZstdRet;
	int bFallbackStore;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_SOLID_DATA_WRITE, "solid mode does not allow file data writes");
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( procXpkWriteQueueCount(objXpk) > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_ZSTD ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize == 0 ) {
		return procXpkWriteImmediate(objXpk, pEntry, pData, iSize, 0);
	}

	hFileTmp = NULL;
	hFileDst = NULL;
	pCtx = NULL;
	sPathTmp = NULL;
	pOutBuf = NULL;
	iCompSize64 = 0;
	iCompSize = 0;
	iHash = xpkHash32Internal(pData, iSize);
	bFallbackStore = FALSE;

	sPathTmp = procXpkWriteTempPathDup(objXpk, ".zstd.data");
	if ( sPathTmp == NULL ) {
		return xpkLastError(objXpk);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		xpkFreeInternal(sPathTmp);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iOutCap = ZSTD_CStreamOutSize();
	if ( iOutCap == 0 ) {
		iOutCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	}

	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	pCtx = ZSTD_createCCtx();
	if ( pCtx == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	ZSTD_CCtx_reset(pCtx, ZSTD_reset_session_only);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_checksumFlag, 0);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_strategy, (ZSTD_strategy)procXpkCompLevelToNative(iLevel));
	ZSTD_CCtx_setPledgedSrcSize(pCtx, (unsigned long long)iSize);

	pCur = (const uint8_t*)pData;
	iSizeLeft = iSize;
	while ( iSizeLeft > 0 ) {
		iChunkSize = (iSizeLeft > XPK_CODEC_STREAM_CHUNK_SIZE) ? XPK_CODEC_STREAM_CHUNK_SIZE : iSizeLeft;
		objIn.src = pCur;
		objIn.size = iChunkSize;
		objIn.pos = 0;
		while ( objIn.pos < objIn.size ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_continue);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize64 += objOut.pos;
				if ( iCompSize64 >= iSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
		}
		if ( bFallbackStore ) {
			break;
		}

		pCur += iChunkSize;
		iSizeLeft -= iChunkSize;
	}

	if ( !bFallbackStore ) {
		objIn.src = NULL;
		objIn.size = 0;
		objIn.pos = 0;
		for ( ;; ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_end);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize64 += objOut.pos;
				if ( iCompSize64 >= iSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
			if ( iZstdRet == 0 ) {
				break;
			}
		}
	}

	if ( bFallbackStore ) {
		iRet = XPK_OK;
		goto lblCleanup;
	}

	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileDst = xrtOpen(objXpk->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
			goto lblCleanup;
		}
	}

	iCompSize = (uint32_t)iCompSize64;
	iRet = procXpkCopySourceFileToPackage(objXpk, hFileTmp, hFileDst, objXpk->iAppendPos, iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
	pEntry->iFileHash = iHash;
	pEntry->iDataOffset = objXpk->iAppendPos;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iSize;
	objXpk->iAppendPos += iCompSize;
	if ( objXpk->iAppendPos > objXpk->iFileSize ) {
		objXpk->iFileSize = objXpk->iAppendPos;
	}

	iRet = XPK_OK;

lblCleanup:
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( pCtx != NULL ) {
		ZSTD_freeCCtx(pCtx);
	}
	if ( pOutBuf != NULL ) {
		xpkFreeInternal(pOutBuf);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	if ( bFallbackStore ) {
		return procXpkWriteImmediate(objXpk, pEntry, pData, iSize, 0);
	}
	if ( iRet == XPK_OK ) {
		procXpkClearError(objXpk);
	}
	return iRet;
}

// 写入阶段 LZMA 顺序输入读取
static inline SRes procXpkWriteLzmaSeqInRead(ISeqInStreamPtr pStream, void* pData, size_t* pSize)
{
	xpkWriteLzmaSeqIn* pIn;
	size_t iWant;
	size_t iRead;

	if ( pStream == NULL || pSize == NULL ) {
		return SZ_ERROR_PARAM;
	}

	pIn = (xpkWriteLzmaSeqIn*)pStream;
	iWant = *pSize;
	if ( iWant == 0 ) {
		return SZ_OK;
	}
	if ( pIn->iRemain < (uint64_t)iWant ) {
		iWant = (size_t)pIn->iRemain;
	}
	if ( iWant == 0 ) {
		*pSize = 0;
		return SZ_OK;
	}

	iRead = xrtGetBuffer(pIn->hFile, pData, iWant);
	*pSize = iRead;
	if ( iRead != iWant ) {
		return SZ_ERROR_READ;
	}

	pIn->iRemain -= iRead;
	return SZ_OK;
}

// 写入阶段 LZMA 顺序输出写入
static inline size_t procXpkWriteLzmaSeqOutWrite(ISeqOutStreamPtr pStream, const void* pData, size_t iSize)
{
	xpkWriteLzmaSeqOut* pOut;
	size_t iWrite;

	if ( pStream == NULL ) {
		return 0;
	}
	if ( iSize == 0 ) {
		return 0;
	}

	pOut = (xpkWriteLzmaSeqOut*)pStream;
	iWrite = xrtPut(pOut->hFile, (ptr)pData, iSize);
	pOut->iSize += iWrite;
	return iWrite;
}

// 写入阶段 LZMA 内存输入读取
static inline SRes procXpkWriteLzmaMemInRead(ISeqInStreamPtr pStream, void* pData, size_t* pSize)
{
	xpkWriteLzmaMemIn* pIn;
	size_t iWant;

	if ( pStream == NULL || pSize == NULL ) {
		return SZ_ERROR_PARAM;
	}

	pIn = (xpkWriteLzmaMemIn*)pStream;
	iWant = *pSize;
	if ( iWant == 0 ) {
		return SZ_OK;
	}
	if ( pIn->iPos >= pIn->iSize ) {
		*pSize = 0;
		return SZ_OK;
	}
	if ( (pIn->iSize - pIn->iPos) < (uint64_t)iWant ) {
		iWant = (size_t)(pIn->iSize - pIn->iPos);
	}
	if ( iWant == 0 ) {
		*pSize = 0;
		return SZ_OK;
	}

	memcpy(pData, pIn->pData + (size_t)pIn->iPos, iWant);
	pIn->iPos += iWant;
	*pSize = iWant;
	return SZ_OK;
}

// 缓冲写入 LZMA2 数据
static inline int procXpkWriteBufferedLzma2Data(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel)
{
	xfile hFileTmp;
	CLzma2EncHandle hLzma2;
	CLzma2EncProps objProps;
	xpkWriteLzmaMemIn objIn;
	xpkWriteLzmaSeqOut objOut;
	char* sPathTmp;
	void* pCompData;
	uint64_t iCompSize64;
	uint32_t iCompSize;
	uint32_t iHash;
	Byte iPropByte;
	SRes iLzmaRes;
	int bFallbackStore;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_LZMA2 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize == 0 ) {
		return procXpkWriteBuffered(objXpk, pEntry, pData, iSize, 0);
	}

	hFileTmp = NULL;
	hLzma2 = NULL;
	sPathTmp = NULL;
	pCompData = NULL;
	iCompSize64 = 0;
	iCompSize = 0;
	iHash = xpkHash32Internal(pData, iSize);
	bFallbackStore = FALSE;

	sPathTmp = procXpkWriteTempPathDup(objXpk, ".lzma2.data.queue");
	if ( sPathTmp == NULL ) {
		return xpkLastError(objXpk);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		xpkFreeInternal(sPathTmp);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	hLzma2 = Lzma2Enc_Create(&g_Alloc, &g_BigAlloc);
	if ( hLzma2 == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	Lzma2EncProps_Init(&objProps);
	objProps.lzmaProps.level = procXpkCompLevelToNative(iLevel);
	iLzmaRes = Lzma2Enc_SetProps(hLzma2, &objProps);
	if ( iLzmaRes != SZ_OK ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, "lzma2 set props failed");
		goto lblCleanup;
	}
	Lzma2Enc_SetDataSize(hLzma2, (UInt64)iSize);

	iPropByte = Lzma2Enc_WriteProperties(hLzma2);
	if ( xrtPut(hFileTmp, (ptr)&iPropByte, 1) != 1 ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	memset(&objIn, 0, sizeof(objIn));
	objIn.vt.Read = procXpkWriteLzmaMemInRead;
	objIn.pData = (const uint8_t*)pData;
	objIn.iSize = iSize;
	objIn.iPos = 0;

	memset(&objOut, 0, sizeof(objOut));
	objOut.vt.Write = procXpkWriteLzmaSeqOutWrite;
	objOut.hFile = hFileTmp;
	objOut.iSize = 1;

	iLzmaRes = Lzma2Enc_Encode2(hLzma2, &objOut.vt, NULL, NULL, &objIn.vt, NULL, 0, NULL);
	if ( iLzmaRes != SZ_OK ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, "lzma2 encode failed");
		goto lblCleanup;
	}

	iCompSize64 = objOut.iSize;
	if ( iCompSize64 >= iSize ) {
		bFallbackStore = TRUE;
		iRet = XPK_OK;
		goto lblCleanup;
	}

	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	iCompSize = (uint32_t)iCompSize64;
	iRet = procXpkReadAtAlloc(objXpk, hFileTmp, 0, iCompSize, &pCompData);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iRet = procXpkQueueWrite(objXpk, pEntry->iPos, iLevel, pCompData, iCompSize, iSize);
	if ( iRet != XPK_OK ) {
		pCompData = NULL;
		goto lblCleanup;
	}
	pCompData = NULL;

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
	pEntry->iFileHash = iHash;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iSize;
	procXpkClearError(objXpk);
	iRet = XPK_OK;

lblCleanup:
	if ( pCompData != NULL ) {
		xpkFreeInternal(pCompData);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( hLzma2 != NULL ) {
		Lzma2Enc_Destroy(hLzma2);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	if ( bFallbackStore ) {
		return procXpkWriteBuffered(objXpk, pEntry, pData, iSize, 0);
	}
	return iRet;
}

// 立即写入 LZMA2 数据
static inline int procXpkWriteImmediateLzma2Data(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint32_t iSize, uint8_t iLevel)
{
	xfile hFileTmp;
	xfile hFileDst;
	CLzma2EncHandle hLzma2;
	CLzma2EncProps objProps;
	xpkWriteLzmaMemIn objIn;
	xpkWriteLzmaSeqOut objOut;
	char* sPathTmp;
	uint64_t iCompSize64;
	uint32_t iCompSize;
	uint32_t iHash;
	Byte iPropByte;
	SRes iLzmaRes;
	int bFallbackStore;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_SOLID_DATA_WRITE, "solid mode does not allow file data writes");
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( procXpkWriteQueueCount(objXpk) > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_LZMA2 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize == 0 ) {
		return procXpkWriteImmediate(objXpk, pEntry, pData, iSize, 0);
	}

	hFileTmp = NULL;
	hFileDst = NULL;
	hLzma2 = NULL;
	sPathTmp = NULL;
	iCompSize64 = 0;
	iCompSize = 0;
	iHash = xpkHash32Internal(pData, iSize);
	bFallbackStore = FALSE;

	sPathTmp = procXpkWriteTempPathDup(objXpk, ".lzma2.data");
	if ( sPathTmp == NULL ) {
		return xpkLastError(objXpk);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		xpkFreeInternal(sPathTmp);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	hLzma2 = Lzma2Enc_Create(&g_Alloc, &g_BigAlloc);
	if ( hLzma2 == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	Lzma2EncProps_Init(&objProps);
	objProps.lzmaProps.level = procXpkCompLevelToNative(iLevel);
	iLzmaRes = Lzma2Enc_SetProps(hLzma2, &objProps);
	if ( iLzmaRes != SZ_OK ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, "lzma2 set props failed");
		goto lblCleanup;
	}
	Lzma2Enc_SetDataSize(hLzma2, (UInt64)iSize);

	iPropByte = Lzma2Enc_WriteProperties(hLzma2);
	if ( xrtPut(hFileTmp, (ptr)&iPropByte, 1) != 1 ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	objIn.vt.Read = procXpkWriteLzmaMemInRead;
	objIn.pData = (const uint8_t*)pData;
	objIn.iSize = iSize;
	objIn.iPos = 0;
	objOut.vt.Write = procXpkWriteLzmaSeqOutWrite;
	objOut.hFile = hFileTmp;
	objOut.iSize = 0;

	iLzmaRes = Lzma2Enc_Encode2(hLzma2, &objOut.vt, NULL, NULL, &objIn.vt, NULL, 0, NULL);
	if ( iLzmaRes != SZ_OK ) {
		if ( iLzmaRes == SZ_ERROR_WRITE ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		} else {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, "lzma2 compress failed");
		}
		goto lblCleanup;
	}

	iCompSize64 = 1 + objOut.iSize;
	if ( iCompSize64 >= iSize ) {
		bFallbackStore = TRUE;
		iRet = XPK_OK;
		goto lblCleanup;
	}

	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileDst = xrtOpen(objXpk->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
			goto lblCleanup;
		}
	}

	iCompSize = (uint32_t)iCompSize64;
	iRet = procXpkCopySourceFileToPackage(objXpk, hFileTmp, hFileDst, objXpk->iAppendPos, iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
	pEntry->iFileHash = iHash;
	pEntry->iDataOffset = objXpk->iAppendPos;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iSize;
	objXpk->iAppendPos += iCompSize;
	if ( objXpk->iAppendPos > objXpk->iFileSize ) {
		objXpk->iFileSize = objXpk->iAppendPos;
	}

	iRet = XPK_OK;

lblCleanup:
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( hLzma2 != NULL ) {
		Lzma2Enc_Destroy(hLzma2);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	if ( bFallbackStore ) {
		return procXpkWriteImmediate(objXpk, pEntry, pData, iSize, 0);
	}
	if ( iRet == XPK_OK ) {
		procXpkClearError(objXpk);
	}
	return iRet;
}

// 写入原始临时文件从源
static inline int procXpkWriteRawTempFileFromSource(xpkObject objXpk, const char* sSrcPath, const char* sTag, xfile* pFileTmpRet, char** pPathTmpRet, uint64_t* pFileSizeRet, uint32_t* pHashRet)
{
	xfile hFileSrc;
	xfile hFileTmp;
	char* sPathTmp;
	void* pChunk;
	uint64_t iFileSize;
	uint64_t iOffsetRead;
	uint32_t iChunkRead;
	size_t iRead;
	uint32_t iHash;
	int iRet;

	if ( pFileTmpRet != NULL ) {
		*pFileTmpRet = NULL;
	}
	if ( pPathTmpRet != NULL ) {
		*pPathTmpRet = NULL;
	}
	if ( pFileSizeRet != NULL ) {
		*pFileSizeRet = 0;
	}
	if ( pHashRet != NULL ) {
		*pHashRet = 0;
	}
	if ( objXpk == NULL || sSrcPath == NULL || sSrcPath[0] == '\0' || sTag == NULL || sTag[0] == '\0' ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pFileTmpRet == NULL || pPathTmpRet == NULL || pFileSizeRet == NULL || pHashRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}

	hFileSrc = NULL;
	hFileTmp = NULL;
	sPathTmp = NULL;
	pChunk = NULL;
	iFileSize = 0;
	iOffsetRead = 0;
	iHash = 0;

	hFileSrc = xrtOpen((str)sSrcPath, TRUE, XRT_CP_BINARY);
	if ( hFileSrc == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iFileSize = xrtGetEOF(hFileSrc);
	if ( iFileSize == 0 ) {
		xrtClose(hFileSrc);
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	sPathTmp = procXpkWriteTempPathDup(objXpk, sTag);
	if ( sPathTmp == NULL ) {
		xrtClose(hFileSrc);
		return xpkLastError(objXpk);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		xpkFreeInternal(sPathTmp);
		xrtClose(hFileSrc);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	pChunk = xpkAllocInternal(XPK_WRITE_FILE_CHUNK_SIZE);
	if ( pChunk == NULL ) {
		xrtClose(hFileTmp);
		(void)xrtFileDelete((str)sPathTmp);
		xpkFreeInternal(sPathTmp);
		xrtClose(hFileSrc);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	while ( iOffsetRead < iFileSize ) {
		iChunkRead = (uint32_t)(((iFileSize - iOffsetRead) > XPK_WRITE_FILE_CHUNK_SIZE) ? XPK_WRITE_FILE_CHUNK_SIZE : (iFileSize - iOffsetRead));
		iRead = xrtGetBuffer(hFileSrc, pChunk, iChunkRead);
		if ( iRead != iChunkRead ) {
			xpkFreeInternal(pChunk);
			xrtClose(hFileTmp);
			(void)xrtFileDelete((str)sPathTmp);
			xpkFreeInternal(sPathTmp);
			xrtClose(hFileSrc);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
		}
		if ( xrtPut(hFileTmp, (ptr)pChunk, iChunkRead) != iChunkRead ) {
			xpkFreeInternal(pChunk);
			xrtClose(hFileTmp);
			(void)xrtFileDelete((str)sPathTmp);
			xpkFreeInternal(sPathTmp);
			xrtClose(hFileSrc);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}
		iOffsetRead += iChunkRead;
	}

	xpkFreeInternal(pChunk);
	xrtClose(hFileSrc);
	hFileSrc = NULL;

	if ( !xrtSetEOF(hFileTmp) ) {
		xrtClose(hFileTmp);
		(void)xrtFileDelete((str)sPathTmp);
		xpkFreeInternal(sPathTmp);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	iRet = procXpkHashMappedFileRange(objXpk, hFileTmp, 0, iFileSize, &iHash);
	if ( iRet != XPK_OK ) {
		xrtClose(hFileTmp);
		(void)xrtFileDelete((str)sPathTmp);
		xpkFreeInternal(sPathTmp);
		return iRet;
	}

	*pFileTmpRet = hFileTmp;
	*pPathTmpRet = sPathTmp;
	*pFileSizeRet = iFileSize;
	*pHashRet = iHash;
	return XPK_OK;
}

// 编码映射的 LZ4 块
static inline int procXpkEncodeMappedLz4Block(xpkObject objXpk, uint8_t iLevel, const xpkMappedFile* pMap, void** pCompDataRet, uint32_t* pCompSizeRet)
{
	void* pCompData;
	uint32_t iBound;
	uint32_t iAlg;
	int iCompSize;

	if ( pCompDataRet != NULL ) {
		*pCompDataRet = NULL;
	}
	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = 0;
	}
	if ( objXpk == NULL || pMap == NULL || pMap->pView == NULL || pCompDataRet == NULL || pCompSizeRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pMap->iSize > UINT32_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}

	iAlg = procXpkCompLevelToAlg(iLevel);
	if ( iAlg != XPK_ALG_LZ4 && iAlg != XPK_ALG_LZ4HC ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	iBound = (uint32_t)LZ4_compressBound((int)pMap->iSize);
	pCompData = xpkAllocInternal(iBound);
	if ( pCompData == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	if ( iAlg == XPK_ALG_LZ4 ) {
		iCompSize = LZ4_compress_fast((const char*)pMap->pView, (char*)pCompData, (int)pMap->iSize, (int)iBound, procXpkCompLevelToNative(iLevel));
	} else {
		iCompSize = LZ4_compress_HC((const char*)pMap->pView, (char*)pCompData, (int)pMap->iSize, (int)iBound, procXpkCompLevelToNative(iLevel));
	}
	if ( iCompSize <= 0 ) {
		xpkFreeInternal(pCompData);
		return procXpkSetError(objXpk, XPK_ERR_IO, "lz4 compress failed");
	}

	*pCompDataRet = pCompData;
	*pCompSizeRet = (uint32_t)iCompSize;
	return XPK_OK;
}

// 立即写入 ZSTD 文件
static inline int procXpkWriteImmediateZstdFile(xpkObject objXpk, xpkEntry* pEntry, const char* sSrcPath, uint8_t iLevel)
{
	xfile hFileSrc;
	xfile hFileTmp;
	xfile hFileDst;
	ZSTD_CCtx* pCtx;
	ZSTD_inBuffer objIn;
	ZSTD_outBuffer objOut;
	char* sPathTmp;
	void* pInBuf;
	void* pOutBuf;
	uint64_t iFileSize;
	uint64_t iCompSize;
	uint64_t iOffsetRead;
	uint32_t iHash;
	size_t iInCap;
	size_t iOutCap;
	size_t iRead;
	size_t iZstdRet;
	uint32_t iChunkRead;
	int bFallbackStore;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_SOLID_DATA_WRITE, "solid mode does not allow file data writes");
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( procXpkWriteQueueCount(objXpk) > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_ZSTD ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	hFileSrc = NULL;
	hFileTmp = NULL;
	hFileDst = NULL;
	pCtx = NULL;
	sPathTmp = NULL;
	pInBuf = NULL;
	pOutBuf = NULL;
	iFileSize = 0;
	iCompSize = 0;
	iOffsetRead = 0;
	iHash = 0;
	bFallbackStore = FALSE;

	hFileSrc = xrtOpen((str)sSrcPath, TRUE, XRT_CP_BINARY);
	if ( hFileSrc == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iFileSize = xrtGetEOF(hFileSrc);
	if ( iFileSize == 0 ) {
		xrtClose(hFileSrc);
		return procXpkWriteImmediateStoreFile(objXpk, pEntry, sSrcPath);
	}

	sPathTmp = procXpkWriteTempPathDup(objXpk, ".zstd.file");
	if ( sPathTmp == NULL ) {
		xrtClose(hFileSrc);
		return xpkLastError(objXpk);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		xpkFreeInternal(sPathTmp);
		xrtClose(hFileSrc);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iInCap = ZSTD_CStreamInSize();
	if ( iInCap == 0 ) {
		iInCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	}
	iOutCap = ZSTD_CStreamOutSize();
	if ( iOutCap == 0 ) {
		iOutCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	}

	pInBuf = xpkAllocInternal(iInCap);
	if ( pInBuf == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	pCtx = ZSTD_createCCtx();
	if ( pCtx == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	ZSTD_CCtx_reset(pCtx, ZSTD_reset_session_only);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_checksumFlag, 0);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_strategy, (ZSTD_strategy)procXpkCompLevelToNative(iLevel));
	ZSTD_CCtx_setPledgedSrcSize(pCtx, (unsigned long long)iFileSize);

	while ( iOffsetRead < iFileSize ) {
		iChunkRead = (uint32_t)(((iFileSize - iOffsetRead) > (uint64_t)iInCap) ? iInCap : (iFileSize - iOffsetRead));
		if ( iChunkRead == 0 ) {
			break;
		}

		iRead = xrtGetBuffer(hFileSrc, pInBuf, iChunkRead);
		if ( iRead != iChunkRead ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
			goto lblCleanup;
		}
		iOffsetRead += iChunkRead;

		objIn.src = pInBuf;
		objIn.size = iChunkRead;
		objIn.pos = 0;
		while ( objIn.pos < objIn.size ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_continue);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize += objOut.pos;
				if ( iCompSize >= iFileSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
		}
		if ( bFallbackStore ) {
			break;
		}
	}

	if ( !bFallbackStore ) {
		objIn.src = NULL;
		objIn.size = 0;
		objIn.pos = 0;
		for ( ;; ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_end);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize += objOut.pos;
				if ( iCompSize >= iFileSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
			if ( iZstdRet == 0 ) {
				break;
			}
		}
	}

	if ( bFallbackStore ) {
		iRet = XPK_OK;
		goto lblCleanup;
	}

	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	iRet = procXpkHashMappedFileRange(objXpk, hFileSrc, 0, iFileSize, &iHash);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileDst = xrtOpen(objXpk->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
			goto lblCleanup;
		}
	}

	iRet = procXpkCopySourceFileToPackage(objXpk, hFileTmp, hFileDst, objXpk->iAppendPos, iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
	pEntry->iFileHash = iHash;
	pEntry->iDataOffset = objXpk->iAppendPos;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iFileSize;
	objXpk->iAppendPos += iCompSize;
	if ( objXpk->iAppendPos > objXpk->iFileSize ) {
		objXpk->iFileSize = objXpk->iAppendPos;
	}

	iRet = XPK_OK;

lblCleanup:
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( pCtx != NULL ) {
		ZSTD_freeCCtx(pCtx);
	}
	if ( pOutBuf != NULL ) {
		xpkFreeInternal(pOutBuf);
	}
	if ( pInBuf != NULL ) {
		xpkFreeInternal(pInBuf);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	if ( bFallbackStore ) {
		return procXpkWriteImmediateStoreFile(objXpk, pEntry, sSrcPath);
	}
	if ( iRet == XPK_OK ) {
		procXpkClearError(objXpk);
	}
	return iRet;
}

// 立即写入 LZ4 文件
static inline int procXpkWriteImmediateLz4File(xpkObject objXpk, xpkEntry* pEntry, const char* sSrcPath, uint8_t iLevel)
{
	xfile hFileTmp;
	xfile hFileDst;
	xpkMappedFile objMap;
	char* sPathTmp;
	void* pCompData;
	uint64_t iFileSize;
	uint32_t iHash;
	uint32_t iCompSize;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_SOLID_DATA_WRITE, "solid mode does not allow file data writes");
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( procXpkWriteQueueCount(objXpk) > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_LZ4 && procXpkCompLevelToAlg(iLevel) != XPK_ALG_LZ4HC ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	hFileTmp = NULL;
	hFileDst = NULL;
	memset(&objMap, 0, sizeof(objMap));
	sPathTmp = NULL;
	pCompData = NULL;
	iFileSize = 0;
	iHash = 0;
	iCompSize = 0;

	iRet = procXpkWriteRawTempFileFromSource(objXpk, sSrcPath, ".lz4.file", &hFileTmp, &sPathTmp, &iFileSize, &iHash);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( iFileSize > UINT32_MAX ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
		goto lblCleanup;
	}

	iRet = procXpkMapFileReadOnly(objXpk, hFileTmp, iFileSize, &objMap);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iRet = procXpkEncodeMappedLz4Block(objXpk, iLevel, &objMap, &pCompData, &iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	if ( iCompSize >= iFileSize ) {
		if ( !procXpkAppliedVolumeMode(objXpk) ) {
			hFileDst = xrtOpen(objXpk->sPathPackage, FALSE, XRT_CP_BINARY);
			if ( hFileDst == NULL ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
				goto lblCleanup;
			}
		}
		iRet = procXpkCopySourceFileToPackage(objXpk, hFileTmp, hFileDst, objXpk->iAppendPos, iFileSize);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}

		pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK);
		pEntry->iFileHash = iHash;
		pEntry->iDataOffset = objXpk->iAppendPos;
		pEntry->iDataSize = iFileSize;
		pEntry->iFileSize = iFileSize;
		objXpk->iAppendPos += iFileSize;
	} else {
		if ( !procXpkAppliedVolumeMode(objXpk) ) {
			hFileDst = xrtOpen(objXpk->sPathPackage, FALSE, XRT_CP_BINARY);
			if ( hFileDst == NULL ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
				goto lblCleanup;
			}
		}
		iRet = procXpkWriteAtChunkedPackage(objXpk, hFileDst, objXpk->iAppendPos, pCompData, iCompSize);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}

		pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
		pEntry->iFileHash = iHash;
		pEntry->iDataOffset = objXpk->iAppendPos;
		pEntry->iDataSize = iCompSize;
		pEntry->iFileSize = iFileSize;
		objXpk->iAppendPos += iCompSize;
	}
	if ( objXpk->iAppendPos > objXpk->iFileSize ) {
		objXpk->iFileSize = objXpk->iAppendPos;
	}

	iRet = XPK_OK;

lblCleanup:
	if ( pCompData != NULL ) {
		xpkFreeInternal(pCompData);
	}
	procXpkUnmapFile(&objMap);
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	if ( iRet == XPK_OK ) {
		procXpkClearError(objXpk);
	}
	return iRet;
}

// 立即写入 LZMA2 文件
static inline int procXpkWriteImmediateLzma2File(xpkObject objXpk, xpkEntry* pEntry, const char* sSrcPath, uint8_t iLevel)
{
	xfile hFileSrc;
	xfile hFileTmp;
	xfile hFileDst;
	CLzma2EncHandle hLzma2;
	CLzma2EncProps objProps;
	xpkWriteLzmaSeqIn objIn;
	xpkWriteLzmaSeqOut objOut;
	char* sPathTmp;
	uint64_t iFileSize;
	uint64_t iCompSize64;
	uint32_t iCompSize;
	uint32_t iHash;
	Byte iPropByte;
	SRes iLzmaRes;
	int bFallbackStore;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_SOLID_DATA_WRITE, "solid mode does not allow file data writes");
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( procXpkWriteQueueCount(objXpk) > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_LZMA2 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	hFileSrc = NULL;
	hFileTmp = NULL;
	hFileDst = NULL;
	hLzma2 = NULL;
	sPathTmp = NULL;
	iFileSize = 0;
	iCompSize64 = 0;
	iCompSize = 0;
	iHash = 0;
	bFallbackStore = FALSE;

	hFileSrc = xrtOpen((str)sSrcPath, TRUE, XRT_CP_BINARY);
	if ( hFileSrc == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iFileSize = xrtGetEOF(hFileSrc);
	if ( iFileSize == 0 ) {
		xrtClose(hFileSrc);
		return procXpkWriteImmediateStoreFile(objXpk, pEntry, sSrcPath);
	}

	sPathTmp = procXpkWriteTempPathDup(objXpk, ".lzma2.file");
	if ( sPathTmp == NULL ) {
		xrtClose(hFileSrc);
		return xpkLastError(objXpk);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		xpkFreeInternal(sPathTmp);
		xrtClose(hFileSrc);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	hLzma2 = Lzma2Enc_Create(&g_Alloc, &g_BigAlloc);
	if ( hLzma2 == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	Lzma2EncProps_Init(&objProps);
	objProps.lzmaProps.level = procXpkCompLevelToNative(iLevel);
	iLzmaRes = Lzma2Enc_SetProps(hLzma2, &objProps);
	if ( iLzmaRes != SZ_OK ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, "lzma2 set props failed");
		goto lblCleanup;
	}
	Lzma2Enc_SetDataSize(hLzma2, (UInt64)iFileSize);

	iPropByte = Lzma2Enc_WriteProperties(hLzma2);
	if ( xrtPut(hFileTmp, (ptr)&iPropByte, 1) != 1 ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	objIn.vt.Read = procXpkWriteLzmaSeqInRead;
	objIn.hFile = hFileSrc;
	objIn.iRemain = iFileSize;
	objOut.vt.Write = procXpkWriteLzmaSeqOutWrite;
	objOut.hFile = hFileTmp;
	objOut.iSize = 0;

	iLzmaRes = Lzma2Enc_Encode2(hLzma2, &objOut.vt, NULL, NULL, &objIn.vt, NULL, 0, NULL);
	if ( iLzmaRes != SZ_OK ) {
		if ( iLzmaRes == SZ_ERROR_READ ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
		} else if ( iLzmaRes == SZ_ERROR_WRITE ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		} else {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, "lzma2 compress failed");
		}
		goto lblCleanup;
	}

	iCompSize64 = 1 + objOut.iSize;
	if ( iCompSize64 >= iFileSize ) {
		bFallbackStore = TRUE;
		iRet = XPK_OK;
		goto lblCleanup;
	}

	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	iRet = procXpkHashMappedFileRange(objXpk, hFileSrc, 0, iFileSize, &iHash);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileDst = xrtOpen(objXpk->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
			goto lblCleanup;
		}
	}

	iCompSize = (uint32_t)iCompSize64;
	iRet = procXpkCopySourceFileToPackage(objXpk, hFileTmp, hFileDst, objXpk->iAppendPos, iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
	pEntry->iFileHash = iHash;
	pEntry->iDataOffset = objXpk->iAppendPos;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iFileSize;
	objXpk->iAppendPos += iCompSize;
	if ( objXpk->iAppendPos > objXpk->iFileSize ) {
		objXpk->iFileSize = objXpk->iAppendPos;
	}

	iRet = XPK_OK;

lblCleanup:
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hLzma2 != NULL ) {
		Lzma2Enc_Destroy(hLzma2);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	if ( bFallbackStore ) {
		return procXpkWriteImmediateStoreFile(objXpk, pEntry, sSrcPath);
	}
	if ( iRet == XPK_OK ) {
		procXpkClearError(objXpk);
	}
	return iRet;
}

// 缓冲写入 ZSTD 文件
static inline int procXpkWriteBufferedZstdFile(xpkObject objXpk, xpkEntry* pEntry, const char* sSrcPath, uint8_t iLevel)
{
	xfile hFileSrc;
	xfile hFileTmp;
	ZSTD_CCtx* pCtx;
	ZSTD_inBuffer objIn;
	ZSTD_outBuffer objOut;
	char* sPathTmp;
	void* pInBuf;
	void* pOutBuf;
	void* pCompData;
	void* pRawData;
	uint64_t iFileSize;
	uint64_t iCompSize64;
	uint64_t iOffsetRead;
	uint32_t iHash;
	uint32_t iCompSize;
	uint32_t iChunkRead;
	size_t iInCap;
	size_t iOutCap;
	size_t iRead;
	size_t iZstdRet;
	int bFallbackStore;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_SOLID_DATA_WRITE, "solid mode does not allow file data writes");
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_ZSTD ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	hFileSrc = NULL;
	hFileTmp = NULL;
	pCtx = NULL;
	sPathTmp = NULL;
	pInBuf = NULL;
	pOutBuf = NULL;
	pCompData = NULL;
	pRawData = NULL;
	iFileSize = 0;
	iCompSize64 = 0;
	iOffsetRead = 0;
	iHash = 0;
	iCompSize = 0;
	bFallbackStore = FALSE;

	hFileSrc = xrtOpen((str)sSrcPath, TRUE, XRT_CP_BINARY);
	if ( hFileSrc == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iFileSize = xrtGetEOF(hFileSrc);
	if ( iFileSize > UINT32_MAX ) {
		xrtClose(hFileSrc);
		return procXpkWriteImmediateStoreFileWithQueuedFallback(objXpk, pEntry, sSrcPath);
	}
	if ( iFileSize == 0 ) {
		xrtClose(hFileSrc);
		return procXpkWriteBuffered(objXpk, pEntry, NULL, 0, iLevel);
	}

	sPathTmp = procXpkWriteTempPathDup(objXpk, ".zstd.queue");
	if ( sPathTmp == NULL ) {
		xrtClose(hFileSrc);
		return xpkLastError(objXpk);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		xpkFreeInternal(sPathTmp);
		xrtClose(hFileSrc);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iInCap = ZSTD_CStreamInSize();
	if ( iInCap == 0 ) {
		iInCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	}
	iOutCap = ZSTD_CStreamOutSize();
	if ( iOutCap == 0 ) {
		iOutCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	}

	pInBuf = xpkAllocInternal(iInCap);
	if ( pInBuf == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	pCtx = ZSTD_createCCtx();
	if ( pCtx == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	ZSTD_CCtx_reset(pCtx, ZSTD_reset_session_only);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_checksumFlag, 0);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_strategy, (ZSTD_strategy)procXpkCompLevelToNative(iLevel));
	ZSTD_CCtx_setPledgedSrcSize(pCtx, (unsigned long long)iFileSize);

	while ( iOffsetRead < iFileSize ) {
		iChunkRead = (uint32_t)(((iFileSize - iOffsetRead) > (uint64_t)iInCap) ? iInCap : (iFileSize - iOffsetRead));
		if ( iChunkRead == 0 ) {
			break;
		}

		iRead = xrtGetBuffer(hFileSrc, pInBuf, iChunkRead);
		if ( iRead != iChunkRead ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
			goto lblCleanup;
		}
		iOffsetRead += iChunkRead;

		objIn.src = pInBuf;
		objIn.size = iChunkRead;
		objIn.pos = 0;
		while ( objIn.pos < objIn.size ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_continue);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize64 += objOut.pos;
				if ( iCompSize64 >= iFileSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
		}
		if ( bFallbackStore ) {
			break;
		}
	}

	if ( !bFallbackStore ) {
		objIn.src = NULL;
		objIn.size = 0;
		objIn.pos = 0;
		for ( ;; ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_end);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objXpk, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize64 += objOut.pos;
				if ( iCompSize64 >= iFileSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
			if ( iZstdRet == 0 ) {
				break;
			}
		}
	}

	if ( bFallbackStore ) {
		pRawData = xpkAllocInternal((size_t)iFileSize);
		if ( pRawData == NULL ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			goto lblCleanup;
		}
		iRet = procXpkSeekFile(objXpk, hFileSrc, 0);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}
		iRead = xrtGetBuffer(hFileSrc, pRawData, (size_t)iFileSize);
		if ( iRead != (size_t)iFileSize ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
			goto lblCleanup;
		}

		iHash = xpkHash32Internal(pRawData, iFileSize);
		iRet = procXpkQueueWrite(objXpk, pEntry->iPos, 0, pRawData, (uint32_t)iFileSize, iFileSize);
		if ( iRet != XPK_OK ) {
			pRawData = NULL;
			goto lblCleanup;
		}
		pRawData = NULL;

		pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK);
		pEntry->iFileHash = iHash;
		pEntry->iDataSize = iFileSize;
		pEntry->iFileSize = iFileSize;
		procXpkClearError(objXpk);
		iRet = XPK_OK;
		goto lblCleanup;
	}

	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	iRet = procXpkHashMappedFileRange(objXpk, hFileSrc, 0, iFileSize, &iHash);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iCompSize = (uint32_t)iCompSize64;
	iRet = procXpkReadAtAlloc(objXpk, hFileTmp, 0, iCompSize, &pCompData);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iRet = procXpkQueueWrite(objXpk, pEntry->iPos, iLevel, pCompData, iCompSize, iFileSize);
	if ( iRet != XPK_OK ) {
		pCompData = NULL;
		goto lblCleanup;
	}
	pCompData = NULL;

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
	pEntry->iFileHash = iHash;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iFileSize;
	procXpkClearError(objXpk);
	iRet = XPK_OK;

lblCleanup:
	if ( pCompData != NULL ) {
		xpkFreeInternal(pCompData);
	}
	if ( pRawData != NULL ) {
		xpkFreeInternal(pRawData);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( pCtx != NULL ) {
		ZSTD_freeCCtx(pCtx);
	}
	if ( pOutBuf != NULL ) {
		xpkFreeInternal(pOutBuf);
	}
	if ( pInBuf != NULL ) {
		xpkFreeInternal(pInBuf);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	return iRet;
}

// 缓冲写入 LZMA2 文件
static inline int procXpkWriteBufferedLzma2File(xpkObject objXpk, xpkEntry* pEntry, const char* sSrcPath, uint8_t iLevel)
{
	xfile hFileSrc;
	xfile hFileTmp;
	CLzma2EncHandle hLzma2;
	CLzma2EncProps objProps;
	xpkWriteLzmaSeqIn objIn;
	xpkWriteLzmaSeqOut objOut;
	char* sPathTmp;
	void* pCompData;
	void* pRawData;
	uint64_t iFileSize;
	uint64_t iCompSize64;
	uint32_t iHash;
	uint32_t iCompSize;
	Byte iPropByte;
	size_t iRead;
	SRes iLzmaRes;
	int bFallbackStore;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_SOLID_DATA_WRITE, "solid mode does not allow file data writes");
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_LZMA2 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	hFileSrc = NULL;
	hFileTmp = NULL;
	hLzma2 = NULL;
	sPathTmp = NULL;
	pCompData = NULL;
	pRawData = NULL;
	iFileSize = 0;
	iCompSize64 = 0;
	iHash = 0;
	iCompSize = 0;
	bFallbackStore = FALSE;

	hFileSrc = xrtOpen((str)sSrcPath, TRUE, XRT_CP_BINARY);
	if ( hFileSrc == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iFileSize = xrtGetEOF(hFileSrc);
	if ( iFileSize > UINT32_MAX ) {
		xrtClose(hFileSrc);
		return procXpkWriteImmediateStoreFileWithQueuedFallback(objXpk, pEntry, sSrcPath);
	}
	if ( iFileSize == 0 ) {
		xrtClose(hFileSrc);
		return procXpkWriteBuffered(objXpk, pEntry, NULL, 0, iLevel);
	}

	sPathTmp = procXpkWriteTempPathDup(objXpk, ".lzma2.queue");
	if ( sPathTmp == NULL ) {
		xrtClose(hFileSrc);
		return xpkLastError(objXpk);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		xpkFreeInternal(sPathTmp);
		xrtClose(hFileSrc);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	hLzma2 = Lzma2Enc_Create(&g_Alloc, &g_BigAlloc);
	if ( hLzma2 == NULL ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	Lzma2EncProps_Init(&objProps);
	objProps.lzmaProps.level = procXpkCompLevelToNative(iLevel);
	iLzmaRes = Lzma2Enc_SetProps(hLzma2, &objProps);
	if ( iLzmaRes != SZ_OK ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, "lzma2 set props failed");
		goto lblCleanup;
	}
	Lzma2Enc_SetDataSize(hLzma2, (UInt64)iFileSize);

	iPropByte = Lzma2Enc_WriteProperties(hLzma2);
	if ( xrtPut(hFileTmp, (ptr)&iPropByte, 1) != 1 ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	objIn.vt.Read = procXpkWriteLzmaSeqInRead;
	objIn.hFile = hFileSrc;
	objIn.iRemain = iFileSize;
	objOut.vt.Write = procXpkWriteLzmaSeqOutWrite;
	objOut.hFile = hFileTmp;
	objOut.iSize = 0;

	iLzmaRes = Lzma2Enc_Encode2(hLzma2, &objOut.vt, NULL, NULL, &objIn.vt, NULL, 0, NULL);
	if ( iLzmaRes != SZ_OK ) {
		if ( iLzmaRes == SZ_ERROR_READ ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
		} else if ( iLzmaRes == SZ_ERROR_WRITE ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		} else {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, "lzma2 compress failed");
		}
		goto lblCleanup;
	}

	iCompSize64 = 1 + objOut.iSize;
	if ( iCompSize64 >= iFileSize ) {
		bFallbackStore = TRUE;
	}

	if ( bFallbackStore ) {
		pRawData = xpkAllocInternal((size_t)iFileSize);
		if ( pRawData == NULL ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			goto lblCleanup;
		}
		iRet = procXpkSeekFile(objXpk, hFileSrc, 0);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}
		iRead = xrtGetBuffer(hFileSrc, pRawData, (size_t)iFileSize);
		if ( iRead != (size_t)iFileSize ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
			goto lblCleanup;
		}

		iHash = xpkHash32Internal(pRawData, iFileSize);
		iRet = procXpkQueueWrite(objXpk, pEntry->iPos, 0, pRawData, (uint32_t)iFileSize, iFileSize);
		if ( iRet != XPK_OK ) {
			pRawData = NULL;
			goto lblCleanup;
		}
		pRawData = NULL;

		pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK);
		pEntry->iFileHash = iHash;
		pEntry->iDataSize = iFileSize;
		pEntry->iFileSize = iFileSize;
		procXpkClearError(objXpk);
		iRet = XPK_OK;
		goto lblCleanup;
	}

	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	iRet = procXpkHashMappedFileRange(objXpk, hFileSrc, 0, iFileSize, &iHash);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iCompSize = (uint32_t)iCompSize64;
	iRet = procXpkReadAtAlloc(objXpk, hFileTmp, 0, iCompSize, &pCompData);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iRet = procXpkQueueWrite(objXpk, pEntry->iPos, iLevel, pCompData, iCompSize, iFileSize);
	if ( iRet != XPK_OK ) {
		pCompData = NULL;
		goto lblCleanup;
	}
	pCompData = NULL;

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
	pEntry->iFileHash = iHash;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iFileSize;
	procXpkClearError(objXpk);
	iRet = XPK_OK;

lblCleanup:
	if ( pCompData != NULL ) {
		xpkFreeInternal(pCompData);
	}
	if ( pRawData != NULL ) {
		xpkFreeInternal(pRawData);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hLzma2 != NULL ) {
		Lzma2Enc_Destroy(hLzma2);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	return iRet;
}

// 缓冲写入 LZ4 文件
static inline int procXpkWriteBufferedLz4File(xpkObject objXpk, xpkEntry* pEntry, const char* sSrcPath, uint8_t iLevel)
{
	xfile hFileTmp;
	xpkMappedFile objMap;
	char* sPathTmp;
	void* pCompData;
	void* pRawData;
	uint64_t iFileSize;
	uint32_t iHash;
	uint32_t iCompSize;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_SOLID_DATA_WRITE, "solid mode does not allow file data writes");
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_LZ4 && procXpkCompLevelToAlg(iLevel) != XPK_ALG_LZ4HC ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	hFileTmp = NULL;
	memset(&objMap, 0, sizeof(objMap));
	sPathTmp = NULL;
	pCompData = NULL;
	pRawData = NULL;
	iFileSize = 0;
	iHash = 0;
	iCompSize = 0;

	iRet = procXpkWriteRawTempFileFromSource(objXpk, sSrcPath, ".lz4.queue", &hFileTmp, &sPathTmp, &iFileSize, &iHash);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( iFileSize > UINT32_MAX ) {
		iRet = procXpkWriteImmediateStoreFileWithQueuedFallback(objXpk, pEntry, sSrcPath);
		goto lblCleanup;
	}

	iRet = procXpkMapFileReadOnly(objXpk, hFileTmp, iFileSize, &objMap);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iRet = procXpkEncodeMappedLz4Block(objXpk, iLevel, &objMap, &pCompData, &iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	if ( iCompSize >= iFileSize ) {
		pRawData = xpkAllocInternal((size_t)iFileSize);
		if ( pRawData == NULL ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			goto lblCleanup;
		}
		memcpy(pRawData, objMap.pView, (size_t)iFileSize);

		iRet = procXpkQueueWrite(objXpk, pEntry->iPos, 0, pRawData, (uint32_t)iFileSize, iFileSize);
		if ( iRet != XPK_OK ) {
			pRawData = NULL;
			goto lblCleanup;
		}
		pRawData = NULL;

		pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK);
		pEntry->iFileHash = iHash;
		pEntry->iDataSize = iFileSize;
		pEntry->iFileSize = iFileSize;
		procXpkClearError(objXpk);
		iRet = XPK_OK;
		goto lblCleanup;
	}

	iRet = procXpkQueueWrite(objXpk, pEntry->iPos, iLevel, pCompData, iCompSize, iFileSize);
	if ( iRet != XPK_OK ) {
		pCompData = NULL;
		goto lblCleanup;
	}
	pCompData = NULL;

	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | iLevel;
	pEntry->iFileHash = iHash;
	pEntry->iDataSize = iCompSize;
	pEntry->iFileSize = iFileSize;
	procXpkClearError(objXpk);
	iRet = XPK_OK;

lblCleanup:
	if ( pCompData != NULL ) {
		xpkFreeInternal(pCompData);
	}
	if ( pRawData != NULL ) {
		xpkFreeInternal(pRawData);
	}
	procXpkUnmapFile(&objMap);
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	return iRet;
}

// 写入立即原样文件
static inline int procXpkWriteImmediateStoreFile(xpkObject objXpk, xpkEntry* pEntry, const char* sSrcPath)
{
	xfile hFileSrc;
	xfile hFileDst;
	xpkMappedFile objMap;
	uint64_t iFileSize;
	uint32_t iHash;
	const void* pHashData;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_SOLID_DATA_WRITE, "solid mode does not allow file data writes");
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( procXpkWriteQueueCount(objXpk) > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}

	hFileSrc = NULL;
	memset(&objMap, 0, sizeof(objMap));
	iFileSize = 0;
	hFileSrc = xrtOpen((str)sSrcPath, TRUE, XRT_CP_BINARY);
	if ( hFileSrc == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	iFileSize = xrtGetEOF(hFileSrc);
	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileDst = xrtOpen(objXpk->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			xrtClose(hFileSrc);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}

		iRet = procXpkCopySourceFileToPackage(objXpk, hFileSrc, hFileDst, objXpk->iAppendPos, iFileSize);
		if ( iRet == XPK_OK ) {
			if ( iFileSize == 0 ) {
				iHash = 0;
			} else {
				iRet = procXpkHashMappedFileRange(objXpk, hFileDst, objXpk->iAppendPos, iFileSize, &iHash);
			}
		}
		xrtClose(hFileDst);
		xrtClose(hFileSrc);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK);
		pEntry->iFileHash = iHash;
		pEntry->iDataOffset = objXpk->iAppendPos;
		pEntry->iDataSize = iFileSize;
		pEntry->iFileSize = iFileSize;
		objXpk->iAppendPos += iFileSize;
		if ( objXpk->iAppendPos > objXpk->iFileSize ) {
			objXpk->iFileSize = objXpk->iAppendPos;
		}

		procXpkClearError(objXpk);
		return XPK_OK;
	}

	iRet = procXpkMapFileReadOnly(objXpk, hFileSrc, iFileSize, &objMap);
	if ( iRet != XPK_OK ) {
		xrtClose(hFileSrc);
		return iRet;
	}

	hFileDst = NULL;
	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileDst = xrtOpen(objXpk->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			procXpkUnmapFile(&objMap);
			xrtClose(hFileSrc);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}

	iRet = procXpkWriteAtChunkedPackage(objXpk, hFileDst, objXpk->iAppendPos, objMap.pView, iFileSize);
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( iRet != XPK_OK ) {
		procXpkUnmapFile(&objMap);
		xrtClose(hFileSrc);
		return iRet;
	}

	if ( iFileSize == 0 ) {
		iHash = 0;
	} else {
		pHashData = (objMap.pView != NULL) ? objMap.pView : "";
		iHash = xpkHash32Internal(pHashData, iFileSize);
	}
	pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK);
	pEntry->iFileHash = iHash;
	pEntry->iDataOffset = objXpk->iAppendPos;
	pEntry->iDataSize = iFileSize;
	pEntry->iFileSize = iFileSize;
	objXpk->iAppendPos += iFileSize;
	if ( objXpk->iAppendPos > objXpk->iFileSize ) {
		objXpk->iFileSize = objXpk->iAppendPos;
	}

	procXpkUnmapFile(&objMap);
	xrtClose(hFileSrc);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 获取 Solid 目标压缩级别
static inline uint8_t procXpkSolidTargetCompLevel(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return 0;
	}
	return (uint8_t)objXpk->objHead.defComp;
}

// 获取 Solid 原样压缩级别
static inline uint8_t procXpkSolidStoredCompLevel(xpkObject objXpk)
{
	uint32_t iPos;
	xpkEntry* pEntry;

	if ( objXpk == NULL ) {
		return 0;
	}
	if ( !objXpk->bSolidApplied ) {
		return procXpkSolidTargetCompLevel(objXpk);
	}

	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( (pEntry != NULL) && !procXpkEntryDeleted(pEntry) ) {
			return (uint8_t)(pEntry->iFlag & XPK_FLAG_COMP_MASK);
		}
	}

	return procXpkSolidTargetCompLevel(objXpk);
}

// 计算 Solid 原始数据大小
static inline int procXpkCalcSolidRawSize(xpkObject objXpk, uint64_t* pSizeRet)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	uint64_t iEndPos;
	uint64_t iSizeMax;

	if ( pSizeRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	*pSizeRet = 0;
	iSizeMax = 0;
	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}

		iEndPos = pEntry->iDataOffset + pEntry->iFileSize;
		if ( iEndPos > iSizeMax ) {
			iSizeMax = iEndPos;
		}
	}

	*pSizeRet = iSizeMax;
	return XPK_OK;
}

// 读取并分配 Solid LZ4 切片
static inline int procXpkReadSolidLz4SliceAlloc(xpkObject objXpk, xpkEntry* pEntry, void** pDataRet, uint64_t* pSizeRet)
{
	xfile hFileSrc;
	xpkMappedFile objMap;
	void* pCompData;
	void* pPrefixBuf;
	void* pSliceBuf;
	const void* pCompPtr;
	uint64_t iPkgSize;
	uint64_t iSolidSize;
	uint64_t iSliceEnd;
	uint32_t iCompSize;
	int iDecRet;
	int iRet;

	if ( pDataRet != NULL ) {
		*pDataRet = NULL;
	}
	if ( pSizeRet != NULL ) {
		*pSizeRet = 0;
	}
	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pEntry->iFileSize == 0 ) {
		pSliceBuf = xpkAllocInternal(1);
		if ( pSliceBuf == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		if ( pDataRet != NULL ) {
			*pDataRet = pSliceBuf;
		}
		if ( pSizeRet != NULL ) {
			*pSizeRet = 0;
		}
		procXpkClearError(objXpk);
		return XPK_OK;
	}

	iRet = procXpkCalcSolidRawSize(objXpk, &iSolidSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( (pEntry->iDataOffset + pEntry->iFileSize) > iSolidSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( objXpk->objHead.dataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iCompSize = (uint32_t)(objXpk->objHead.dataOffset - XPK_HEAD_SIZE);
	if ( iCompSize == 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iSliceEnd = pEntry->iDataOffset + pEntry->iFileSize;
	if ( iSliceEnd > 2147483647ULL || iCompSize > 2147483647U ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}

	hFileSrc = NULL;
	memset(&objMap, 0, sizeof(objMap));
	pCompData = NULL;
	pCompPtr = NULL;
	pPrefixBuf = NULL;
	pSliceBuf = NULL;

	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileSrc = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		iPkgSize = xrtGetEOF(hFileSrc);
		if ( objXpk->objHead.dataOffset > iPkgSize ) {
			xrtClose(hFileSrc);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		iRet = procXpkMapFileReadOnly(objXpk, hFileSrc, iPkgSize, &objMap);
		xrtClose(hFileSrc);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		pCompPtr = (const uint8_t*)objMap.pView + XPK_HEAD_SIZE;
	} else {
		iRet = procXpkReadAtAlloc(objXpk, NULL, XPK_HEAD_SIZE, iCompSize, &pCompData);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		pCompPtr = pCompData;
	}

	pPrefixBuf = xpkAllocInternal((size_t)iSliceEnd);
	if ( pPrefixBuf == NULL ) {
		if ( pCompData != NULL ) {
			xpkFreeInternal(pCompData);
		}
		procXpkUnmapFile(&objMap);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iDecRet = LZ4_decompress_safe_partial((const char*)pCompPtr, (char*)pPrefixBuf, (int)iCompSize, (int)iSliceEnd, (int)iSliceEnd);
	if ( iDecRet != (int)iSliceEnd ) {
		xpkFreeInternal(pPrefixBuf);
		if ( pCompData != NULL ) {
			xpkFreeInternal(pCompData);
		}
		procXpkUnmapFile(&objMap);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lz4 decompress failed");
	}

	pSliceBuf = xpkAllocInternal((size_t)pEntry->iFileSize);
	if ( pSliceBuf == NULL ) {
		xpkFreeInternal(pPrefixBuf);
		if ( pCompData != NULL ) {
			xpkFreeInternal(pCompData);
		}
		procXpkUnmapFile(&objMap);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	memcpy(pSliceBuf, (const uint8_t*)pPrefixBuf + (size_t)pEntry->iDataOffset, (size_t)pEntry->iFileSize);
	xpkFreeInternal(pPrefixBuf);
	if ( pCompData != NULL ) {
		xpkFreeInternal(pCompData);
	}
	procXpkUnmapFile(&objMap);

	if ( pDataRet != NULL ) {
		*pDataRet = pSliceBuf;
	}
	if ( pSizeRet != NULL ) {
		*pSizeRet = pEntry->iFileSize;
	}
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 将 Solid LZ4 条目复制到文件
static inline int procXpkCopySolidLz4EntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	xfile hFileSrc;
	xpkMappedFile objMap;
	void* pCompData;
	void* pPrefixBuf;
	const void* pCompPtr;
	uint64_t iPkgSize;
	uint64_t iSolidSize;
	uint64_t iSliceEnd;
	uint32_t iCompSize;
	int iDecRet;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( pEntry->iFileSize == 0 ) {
		return procXpkWriteFileData(objXpk, sPathFile, NULL, 0);
	}

	iRet = procXpkCalcSolidRawSize(objXpk, &iSolidSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( (pEntry->iDataOffset + pEntry->iFileSize) > iSolidSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( objXpk->objHead.dataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iCompSize = (uint32_t)(objXpk->objHead.dataOffset - XPK_HEAD_SIZE);
	if ( iCompSize == 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iSliceEnd = pEntry->iDataOffset + pEntry->iFileSize;
	if ( iSliceEnd > 2147483647ULL || iCompSize > 2147483647U ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}

	hFileSrc = NULL;
	memset(&objMap, 0, sizeof(objMap));
	pCompData = NULL;
	pCompPtr = NULL;
	pPrefixBuf = NULL;

	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileSrc = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		iPkgSize = xrtGetEOF(hFileSrc);
		if ( objXpk->objHead.dataOffset > iPkgSize ) {
			xrtClose(hFileSrc);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		iRet = procXpkMapFileReadOnly(objXpk, hFileSrc, iPkgSize, &objMap);
		xrtClose(hFileSrc);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		pCompPtr = (const uint8_t*)objMap.pView + XPK_HEAD_SIZE;
	} else {
		iRet = procXpkReadAtAlloc(objXpk, NULL, XPK_HEAD_SIZE, iCompSize, &pCompData);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		pCompPtr = pCompData;
	}

	pPrefixBuf = xpkAllocInternal((size_t)iSliceEnd);
	if ( pPrefixBuf == NULL ) {
		if ( pCompData != NULL ) {
			xpkFreeInternal(pCompData);
		}
		procXpkUnmapFile(&objMap);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iDecRet = LZ4_decompress_safe_partial((const char*)pCompPtr, (char*)pPrefixBuf, (int)iCompSize, (int)iSliceEnd, (int)iSliceEnd);
	if ( iDecRet != (int)iSliceEnd ) {
		xpkFreeInternal(pPrefixBuf);
		if ( pCompData != NULL ) {
			xpkFreeInternal(pCompData);
		}
		procXpkUnmapFile(&objMap);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lz4 decompress failed");
	}

	iRet = procXpkWriteFileData(objXpk, sPathFile, (const uint8_t*)pPrefixBuf + (size_t)pEntry->iDataOffset, pEntry->iFileSize);
	xpkFreeInternal(pPrefixBuf);
	if ( pCompData != NULL ) {
		xpkFreeInternal(pCompData);
	}
	procXpkUnmapFile(&objMap);
	return iRet;
}

// 读取并分配 Solid ZSTD 切片
static inline int procXpkReadSolidZstdSliceAlloc(xpkObject objXpk, xpkEntry* pEntry, void** pDataRet, uint64_t* pSizeRet)
{
	xfile hFileSrc;
	uint64_t iPkgSize;
	uint64_t iSolidSize;
	uint64_t iOffsetRead;
	uint64_t iSizeLeft;
	uint64_t iRawPos;
	uint64_t iSliceWritten;
	uint64_t iSliceStart;
	uint64_t iSliceEnd;
	uint64_t iCopyStart;
	uint64_t iCopyEnd;
	ZSTD_DStream* pStream;
	ZSTD_inBuffer objIn;
	ZSTD_outBuffer objOut;
	void* pInBuf;
	void* pOutBuf;
	void* pSliceBuf;
	size_t iInCap;
	size_t iOutCap;
	size_t iZstdRet;
	uint32_t iCompSize;
	uint32_t iChunkRead;
	int iRet;

	if ( pDataRet != NULL ) {
		*pDataRet = NULL;
	}
	if ( pSizeRet != NULL ) {
		*pSizeRet = 0;
	}
	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pEntry->iFileSize == 0 ) {
		pSliceBuf = xpkAllocInternal(1);
		if ( pSliceBuf == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		if ( pDataRet != NULL ) {
			*pDataRet = pSliceBuf;
		}
		if ( pSizeRet != NULL ) {
			*pSizeRet = 0;
		}
		procXpkClearError(objXpk);
		return XPK_OK;
	}

	iRet = procXpkCalcSolidRawSize(objXpk, &iSolidSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( (pEntry->iDataOffset + pEntry->iFileSize) > iSolidSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( objXpk->objHead.dataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iCompSize = (uint32_t)(objXpk->objHead.dataOffset - XPK_HEAD_SIZE);
	if ( iCompSize == 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	hFileSrc = NULL;
	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileSrc = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		iPkgSize = xrtGetEOF(hFileSrc);
		if ( objXpk->objHead.dataOffset > iPkgSize ) {
			xrtClose(hFileSrc);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
	}

	pSliceBuf = xpkAllocInternal((size_t)pEntry->iFileSize);
	if ( pSliceBuf == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iInCap = ZSTD_DStreamInSize();
	if ( iInCap == 0 ) {
		iInCap = 131072u;
	}
	iOutCap = ZSTD_DStreamOutSize();
	if ( iOutCap == 0 ) {
		iOutCap = 131072u;
	}

	pInBuf = xpkAllocInternal(iInCap);
	if ( pInBuf == NULL ) {
		xpkFreeInternal(pSliceBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		xpkFreeInternal(pInBuf);
		xpkFreeInternal(pSliceBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	pStream = ZSTD_createDStream();
	if ( pStream == NULL ) {
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		xpkFreeInternal(pSliceBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iZstdRet = ZSTD_initDStream(pStream);
	if ( ZSTD_isError(iZstdRet) ) {
		ZSTD_freeDStream(pStream);
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		xpkFreeInternal(pSliceBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
	}

	iOffsetRead = XPK_HEAD_SIZE;
	iSizeLeft = iCompSize;
	iRawPos = 0;
	iSliceWritten = 0;
	iSliceStart = pEntry->iDataOffset;
	iSliceEnd = pEntry->iDataOffset + pEntry->iFileSize;
	memset(&objIn, 0, sizeof(objIn));

	for ( ;; ) {
		if ( (objIn.pos == objIn.size) && (iSizeLeft > 0) ) {
			iChunkRead = (uint32_t)((iSizeLeft > (uint64_t)iInCap) ? iInCap : iSizeLeft);
			iRet = procXpkReadAtBuffer(objXpk, hFileSrc, iOffsetRead, pInBuf, iChunkRead);
			if ( iRet != XPK_OK ) {
				ZSTD_freeDStream(pStream);
				xpkFreeInternal(pOutBuf);
				xpkFreeInternal(pInBuf);
				xpkFreeInternal(pSliceBuf);
				if ( hFileSrc != NULL ) {
					xrtClose(hFileSrc);
				}
				return iRet;
			}

			objIn.src = pInBuf;
			objIn.size = iChunkRead;
			objIn.pos = 0;
			iOffsetRead += iChunkRead;
			iSizeLeft -= iChunkRead;
		} else if ( (objIn.pos == objIn.size) && (iSizeLeft == 0) ) {
			objIn.src = pInBuf;
			objIn.size = 0;
			objIn.pos = 0;
		}

		objOut.dst = pOutBuf;
		objOut.size = iOutCap;
		objOut.pos = 0;
		iZstdRet = ZSTD_decompressStream(pStream, &objOut, &objIn);
		if ( ZSTD_isError(iZstdRet) ) {
			ZSTD_freeDStream(pStream);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			xpkFreeInternal(pSliceBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
		}

		if ( objOut.pos > 0 ) {
			iCopyStart = (iRawPos > iSliceStart) ? iRawPos : iSliceStart;
			iCopyEnd = ((iRawPos + objOut.pos) < iSliceEnd) ? (iRawPos + objOut.pos) : iSliceEnd;
			if ( iCopyEnd > iCopyStart ) {
				memcpy((uint8_t*)pSliceBuf + (size_t)(iCopyStart - iSliceStart), (const uint8_t*)pOutBuf + (size_t)(iCopyStart - iRawPos), (size_t)(iCopyEnd - iCopyStart));
				iSliceWritten += (iCopyEnd - iCopyStart);
			}
			iRawPos += objOut.pos;
		}

		if ( (iSizeLeft == 0) && (objIn.pos == objIn.size) && (iZstdRet == 0) ) {
			break;
		}
		if ( (iSizeLeft == 0) && (objIn.pos == objIn.size) && (objOut.pos == 0) && (iZstdRet > 0) ) {
			ZSTD_freeDStream(pStream);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			xpkFreeInternal(pSliceBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
		}
	}

	ZSTD_freeDStream(pStream);
	xpkFreeInternal(pOutBuf);
	xpkFreeInternal(pInBuf);
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}

	if ( iRawPos != iSolidSize || iSliceWritten != pEntry->iFileSize ) {
		xpkFreeInternal(pSliceBuf);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	if ( pDataRet != NULL ) {
		*pDataRet = pSliceBuf;
	}
	if ( pSizeRet != NULL ) {
		*pSizeRet = pEntry->iFileSize;
	}
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 将 Solid ZSTD 条目复制到文件
static inline int procXpkCopySolidZstdEntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	xfile hFileSrc;
	xfile hFileDst;
	uint64_t iPkgSize;
	uint64_t iSolidSize;
	uint64_t iOffsetRead;
	uint64_t iSizeLeft;
	uint64_t iRawPos;
	uint64_t iSliceWritten;
	uint64_t iSliceStart;
	uint64_t iSliceEnd;
	uint64_t iCopyStart;
	uint64_t iCopyEnd;
	ZSTD_DStream* pStream;
	ZSTD_inBuffer objIn;
	ZSTD_outBuffer objOut;
	void* pInBuf;
	void* pOutBuf;
	size_t iInCap;
	size_t iOutCap;
	size_t iZstdRet;
	uint32_t iCompSize;
	uint32_t iChunkRead;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( pEntry->iFileSize == 0 ) {
		return procXpkWriteFileData(objXpk, sPathFile, NULL, 0);
	}

	iRet = procXpkCalcSolidRawSize(objXpk, &iSolidSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( (pEntry->iDataOffset + pEntry->iFileSize) > iSolidSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( objXpk->objHead.dataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iCompSize = (uint32_t)(objXpk->objHead.dataOffset - XPK_HEAD_SIZE);
	if ( iCompSize == 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	hFileSrc = NULL;
	hFileDst = xrtOpen((str)sPathFile, FALSE, XRT_CP_BINARY);
	if ( hFileDst == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	if ( procXpkSeekFile(objXpk, hFileDst, 0) != XPK_OK ) {
		xrtClose(hFileDst);
		return xpkLastError(objXpk);
	}
	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileSrc = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		iPkgSize = xrtGetEOF(hFileSrc);
		if ( objXpk->objHead.dataOffset > iPkgSize ) {
			xrtClose(hFileSrc);
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
	}

	iInCap = ZSTD_DStreamInSize();
	if ( iInCap == 0 ) {
		iInCap = 131072u;
	}
	iOutCap = ZSTD_DStreamOutSize();
	if ( iOutCap == 0 ) {
		iOutCap = 131072u;
	}
	pInBuf = xpkAllocInternal(iInCap);
	if ( pInBuf == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	pStream = ZSTD_createDStream();
	if ( pStream == NULL ) {
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	iZstdRet = ZSTD_initDStream(pStream);
	if ( ZSTD_isError(iZstdRet) ) {
		ZSTD_freeDStream(pStream);
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
	}

	iOffsetRead = XPK_HEAD_SIZE;
	iSizeLeft = iCompSize;
	iRawPos = 0;
	iSliceWritten = 0;
	iSliceStart = pEntry->iDataOffset;
	iSliceEnd = pEntry->iDataOffset + pEntry->iFileSize;
	memset(&objIn, 0, sizeof(objIn));
	for ( ;; ) {
		if ( (objIn.pos == objIn.size) && (iSizeLeft > 0) ) {
			iChunkRead = (uint32_t)((iSizeLeft > (uint64_t)iInCap) ? iInCap : iSizeLeft);
			iRet = procXpkReadAtBuffer(objXpk, hFileSrc, iOffsetRead, pInBuf, iChunkRead);
			if ( iRet != XPK_OK ) {
				ZSTD_freeDStream(pStream);
				xpkFreeInternal(pOutBuf);
				xpkFreeInternal(pInBuf);
				if ( hFileSrc != NULL ) {
					xrtClose(hFileSrc);
				}
				xrtClose(hFileDst);
				return iRet;
			}
			objIn.src = pInBuf;
			objIn.size = iChunkRead;
			objIn.pos = 0;
			iOffsetRead += iChunkRead;
			iSizeLeft -= iChunkRead;
		} else if ( (objIn.pos == objIn.size) && (iSizeLeft == 0) ) {
			objIn.src = pInBuf;
			objIn.size = 0;
			objIn.pos = 0;
		}

		objOut.dst = pOutBuf;
		objOut.size = iOutCap;
		objOut.pos = 0;
		iZstdRet = ZSTD_decompressStream(pStream, &objOut, &objIn);
		if ( ZSTD_isError(iZstdRet) ) {
			ZSTD_freeDStream(pStream);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
		}

		if ( objOut.pos > 0 ) {
			iCopyStart = (iRawPos > iSliceStart) ? iRawPos : iSliceStart;
			iCopyEnd = ((iRawPos + objOut.pos) < iSliceEnd) ? (iRawPos + objOut.pos) : iSliceEnd;
			if ( iCopyEnd > iCopyStart ) {
				if ( xrtPut(hFileDst, (ptr)((const uint8_t*)pOutBuf + (size_t)(iCopyStart - iRawPos)), (size_t)(iCopyEnd - iCopyStart)) != (size_t)(iCopyEnd - iCopyStart) ) {
					ZSTD_freeDStream(pStream);
					xpkFreeInternal(pOutBuf);
					xpkFreeInternal(pInBuf);
					if ( hFileSrc != NULL ) {
						xrtClose(hFileSrc);
					}
					xrtClose(hFileDst);
					return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
				}
				iSliceWritten += (iCopyEnd - iCopyStart);
			}
			iRawPos += objOut.pos;
		}

		if ( (iSizeLeft == 0) && (objIn.pos == objIn.size) && (iZstdRet == 0) ) {
			break;
		}
		if ( (iSizeLeft == 0) && (objIn.pos == objIn.size) && (objOut.pos == 0) && (iZstdRet > 0) ) {
			ZSTD_freeDStream(pStream);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "zstd decompress failed");
		}
	}

	ZSTD_freeDStream(pStream);
	xpkFreeInternal(pOutBuf);
	xpkFreeInternal(pInBuf);
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}

	if ( iRawPos != iSolidSize || iSliceWritten != pEntry->iFileSize ) {
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( !xrtSetEOF(hFileDst) ) {
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	xrtClose(hFileDst);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 读取并分配 Solid LZMA2 切片
static inline int procXpkReadSolidLzma2SliceAlloc(xpkObject objXpk, xpkEntry* pEntry, void** pDataRet, uint64_t* pSizeRet)
{
	xfile hFileSrc;
	uint64_t iPkgSize;
	uint64_t iSolidSize;
	uint64_t iOffsetRead;
	uint64_t iSizeLeft;
	uint64_t iRawPos;
	uint64_t iSliceWritten;
	uint64_t iSliceStart;
	uint64_t iSliceEnd;
	uint64_t iCopyStart;
	uint64_t iCopyEnd;
	Byte iPropByte;
	CLzma2Dec objDec;
	ELzmaStatus iStatus;
	ELzmaFinishMode iFinishMode;
	void* pInBuf;
	void* pOutBuf;
	void* pSliceBuf;
	SizeT iInCap;
	SizeT iOutCap;
	SizeT iInputSize;
	SizeT iInputPos;
	SizeT iSrcLen;
	SizeT iDstLen;
	uint32_t iCompSize;
	uint32_t iChunkRead;
	int iRet;
	SRes iLzmaRes;

	if ( pDataRet != NULL ) {
		*pDataRet = NULL;
	}
	if ( pSizeRet != NULL ) {
		*pSizeRet = 0;
	}
	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pEntry->iFileSize == 0 ) {
		pSliceBuf = xpkAllocInternal(1);
		if ( pSliceBuf == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		if ( pDataRet != NULL ) {
			*pDataRet = pSliceBuf;
		}
		if ( pSizeRet != NULL ) {
			*pSizeRet = 0;
		}
		procXpkClearError(objXpk);
		return XPK_OK;
	}

	iRet = procXpkCalcSolidRawSize(objXpk, &iSolidSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( (pEntry->iDataOffset + pEntry->iFileSize) > iSolidSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( objXpk->objHead.dataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iCompSize = (uint32_t)(objXpk->objHead.dataOffset - XPK_HEAD_SIZE);
	if ( iCompSize < 1 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	hFileSrc = NULL;
	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileSrc = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		iPkgSize = xrtGetEOF(hFileSrc);
		if ( objXpk->objHead.dataOffset > iPkgSize ) {
			xrtClose(hFileSrc);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
	}

	pSliceBuf = xpkAllocInternal((size_t)pEntry->iFileSize);
	if ( pSliceBuf == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iRet = procXpkReadAtBuffer(objXpk, hFileSrc, XPK_HEAD_SIZE, &iPropByte, 1);
	if ( iRet != XPK_OK ) {
		xpkFreeInternal(pSliceBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return iRet;
	}

	iInCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	iOutCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	pInBuf = xpkAllocInternal(iInCap);
	if ( pInBuf == NULL ) {
		xpkFreeInternal(pSliceBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		xpkFreeInternal(pInBuf);
		xpkFreeInternal(pSliceBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	Lzma2Dec_Construct(&objDec);
	iLzmaRes = Lzma2Dec_Allocate(&objDec, iPropByte, &g_Alloc);
	if ( iLzmaRes != SZ_OK ) {
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		xpkFreeInternal(pSliceBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		if ( iLzmaRes == SZ_ERROR_MEM ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
	}
	Lzma2Dec_Init(&objDec);

	iOffsetRead = XPK_HEAD_SIZE + 1;
	iSizeLeft = iCompSize - 1;
	iRawPos = 0;
	iSliceWritten = 0;
	iSliceStart = pEntry->iDataOffset;
	iSliceEnd = pEntry->iDataOffset + pEntry->iFileSize;
	iInputSize = 0;
	iInputPos = 0;

	for ( ;; ) {
		if ( iRawPos >= iSolidSize ) {
			break;
		}
		if ( (iInputPos == iInputSize) && (iSizeLeft > 0) ) {
			iChunkRead = (uint32_t)((iSizeLeft > (uint64_t)iInCap) ? iInCap : iSizeLeft);
			iRet = procXpkReadAtBuffer(objXpk, hFileSrc, iOffsetRead, pInBuf, iChunkRead);
			if ( iRet != XPK_OK ) {
				Lzma2Dec_Free(&objDec, &g_Alloc);
				xpkFreeInternal(pOutBuf);
				xpkFreeInternal(pInBuf);
				xpkFreeInternal(pSliceBuf);
				if ( hFileSrc != NULL ) {
					xrtClose(hFileSrc);
				}
				return iRet;
			}
			iInputSize = iChunkRead;
			iInputPos = 0;
			iOffsetRead += iChunkRead;
			iSizeLeft -= iChunkRead;
		}

		iDstLen = (SizeT)(((iSolidSize - iRawPos) > (uint64_t)iOutCap) ? iOutCap : (iSolidSize - iRawPos));
		iSrcLen = iInputSize - iInputPos;
		iFinishMode = (((iRawPos + iDstLen) == iSolidSize) && (iSizeLeft == 0)) ? LZMA_FINISH_END : LZMA_FINISH_ANY;
		iStatus = LZMA_STATUS_NOT_SPECIFIED;
		iLzmaRes = Lzma2Dec_DecodeToBuf(&objDec, (Byte*)pOutBuf, &iDstLen, (const Byte*)pInBuf + iInputPos, &iSrcLen, iFinishMode, &iStatus);
		if ( iLzmaRes != SZ_OK ) {
			Lzma2Dec_Free(&objDec, &g_Alloc);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			xpkFreeInternal(pSliceBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
		}

		iInputPos += iSrcLen;
		if ( iDstLen > 0 ) {
			iCopyStart = (iRawPos > iSliceStart) ? iRawPos : iSliceStart;
			iCopyEnd = ((iRawPos + iDstLen) < iSliceEnd) ? (iRawPos + iDstLen) : iSliceEnd;
			if ( iCopyEnd > iCopyStart ) {
				memcpy((uint8_t*)pSliceBuf + (size_t)(iCopyStart - iSliceStart), (const uint8_t*)pOutBuf + (size_t)(iCopyStart - iRawPos), (size_t)(iCopyEnd - iCopyStart));
				iSliceWritten += (iCopyEnd - iCopyStart);
			}
			iRawPos += iDstLen;
		}

		if ( (iDstLen == 0) && (iSrcLen == 0) ) {
			Lzma2Dec_Free(&objDec, &g_Alloc);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			xpkFreeInternal(pSliceBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
		}
	}

	Lzma2Dec_Free(&objDec, &g_Alloc);
	xpkFreeInternal(pOutBuf);
	xpkFreeInternal(pInBuf);
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}

	if ( iRawPos != iSolidSize || iSizeLeft != 0 || iInputPos != iInputSize || iSliceWritten != pEntry->iFileSize ) {
		xpkFreeInternal(pSliceBuf);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	if ( pDataRet != NULL ) {
		*pDataRet = pSliceBuf;
	}
	if ( pSizeRet != NULL ) {
		*pSizeRet = pEntry->iFileSize;
	}
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 将 Solid LZMA2 条目复制到文件
static inline int procXpkCopySolidLzma2EntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	xfile hFileSrc;
	xfile hFileDst;
	uint64_t iPkgSize;
	uint64_t iSolidSize;
	uint64_t iOffsetRead;
	uint64_t iSizeLeft;
	uint64_t iRawPos;
	uint64_t iSliceWritten;
	uint64_t iSliceStart;
	uint64_t iSliceEnd;
	uint64_t iCopyStart;
	uint64_t iCopyEnd;
	Byte iPropByte;
	CLzma2Dec objDec;
	ELzmaStatus iStatus;
	ELzmaFinishMode iFinishMode;
	void* pInBuf;
	void* pOutBuf;
	SizeT iInCap;
	SizeT iOutCap;
	SizeT iInputSize;
	SizeT iInputPos;
	SizeT iSrcLen;
	SizeT iDstLen;
	uint32_t iCompSize;
	uint32_t iChunkRead;
	int iRet;
	SRes iLzmaRes;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( pEntry->iFileSize == 0 ) {
		return procXpkWriteFileData(objXpk, sPathFile, NULL, 0);
	}

	iRet = procXpkCalcSolidRawSize(objXpk, &iSolidSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( (pEntry->iDataOffset + pEntry->iFileSize) > iSolidSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( objXpk->objHead.dataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iCompSize = (uint32_t)(objXpk->objHead.dataOffset - XPK_HEAD_SIZE);
	if ( iCompSize < 1 ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	hFileSrc = NULL;
	hFileDst = xrtOpen((str)sPathFile, FALSE, XRT_CP_BINARY);
	if ( hFileDst == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	if ( procXpkSeekFile(objXpk, hFileDst, 0) != XPK_OK ) {
		xrtClose(hFileDst);
		return xpkLastError(objXpk);
	}
	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFileSrc = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		iPkgSize = xrtGetEOF(hFileSrc);
		if ( objXpk->objHead.dataOffset > iPkgSize ) {
			xrtClose(hFileSrc);
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
	}

	iRet = procXpkReadAtBuffer(objXpk, hFileSrc, XPK_HEAD_SIZE, &iPropByte, 1);
	if ( iRet != XPK_OK ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return iRet;
	}

	iInCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	iOutCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	pInBuf = xpkAllocInternal(iInCap);
	if ( pInBuf == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	Lzma2Dec_Construct(&objDec);
	iLzmaRes = Lzma2Dec_Allocate(&objDec, iPropByte, &g_Alloc);
	if ( iLzmaRes != SZ_OK ) {
		xpkFreeInternal(pOutBuf);
		xpkFreeInternal(pInBuf);
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xrtClose(hFileDst);
		if ( iLzmaRes == SZ_ERROR_MEM ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
	}
	Lzma2Dec_Init(&objDec);

	iOffsetRead = XPK_HEAD_SIZE + 1;
	iSizeLeft = iCompSize - 1;
	iRawPos = 0;
	iSliceWritten = 0;
	iSliceStart = pEntry->iDataOffset;
	iSliceEnd = pEntry->iDataOffset + pEntry->iFileSize;
	iInputSize = 0;
	iInputPos = 0;
	for ( ;; ) {
		if ( iRawPos >= iSolidSize ) {
			break;
		}
		if ( (iInputPos == iInputSize) && (iSizeLeft > 0) ) {
			iChunkRead = (uint32_t)((iSizeLeft > (uint64_t)iInCap) ? iInCap : iSizeLeft);
			iRet = procXpkReadAtBuffer(objXpk, hFileSrc, iOffsetRead, pInBuf, iChunkRead);
			if ( iRet != XPK_OK ) {
				Lzma2Dec_Free(&objDec, &g_Alloc);
				xpkFreeInternal(pOutBuf);
				xpkFreeInternal(pInBuf);
				if ( hFileSrc != NULL ) {
					xrtClose(hFileSrc);
				}
				xrtClose(hFileDst);
				return iRet;
			}
			iInputSize = iChunkRead;
			iInputPos = 0;
			iOffsetRead += iChunkRead;
			iSizeLeft -= iChunkRead;
		}

		iDstLen = (SizeT)(((iSolidSize - iRawPos) > (uint64_t)iOutCap) ? iOutCap : (iSolidSize - iRawPos));
		iSrcLen = iInputSize - iInputPos;
		iFinishMode = (((iRawPos + iDstLen) == iSolidSize) && (iSizeLeft == 0)) ? LZMA_FINISH_END : LZMA_FINISH_ANY;
		iStatus = LZMA_STATUS_NOT_SPECIFIED;
		iLzmaRes = Lzma2Dec_DecodeToBuf(&objDec, (Byte*)pOutBuf, &iDstLen, (const Byte*)pInBuf + iInputPos, &iSrcLen, iFinishMode, &iStatus);
		if ( iLzmaRes != SZ_OK ) {
			Lzma2Dec_Free(&objDec, &g_Alloc);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
		}

		iInputPos += iSrcLen;
		if ( iDstLen > 0 ) {
			iCopyStart = (iRawPos > iSliceStart) ? iRawPos : iSliceStart;
			iCopyEnd = ((iRawPos + iDstLen) < iSliceEnd) ? (iRawPos + iDstLen) : iSliceEnd;
			if ( iCopyEnd > iCopyStart ) {
				if ( xrtPut(hFileDst, (ptr)((const uint8_t*)pOutBuf + (size_t)(iCopyStart - iRawPos)), (size_t)(iCopyEnd - iCopyStart)) != (size_t)(iCopyEnd - iCopyStart) ) {
					Lzma2Dec_Free(&objDec, &g_Alloc);
					xpkFreeInternal(pOutBuf);
					xpkFreeInternal(pInBuf);
					if ( hFileSrc != NULL ) {
						xrtClose(hFileSrc);
					}
					xrtClose(hFileDst);
					return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
				}
				iSliceWritten += (iCopyEnd - iCopyStart);
			}
			iRawPos += iDstLen;
		}

		if ( (iDstLen == 0) && (iSrcLen == 0) ) {
			Lzma2Dec_Free(&objDec, &g_Alloc);
			xpkFreeInternal(pOutBuf);
			xpkFreeInternal(pInBuf);
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			xrtClose(hFileDst);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, "lzma2 decompress failed");
		}
	}

	Lzma2Dec_Free(&objDec, &g_Alloc);
	xpkFreeInternal(pOutBuf);
	xpkFreeInternal(pInBuf);
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}

	if ( iRawPos != iSolidSize || iSizeLeft != 0 || iInputPos != iInputSize || iSliceWritten != pEntry->iFileSize ) {
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( !xrtSetEOF(hFileDst) ) {
		xrtClose(hFileDst);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	xrtClose(hFileDst);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 读取 Solid 条目数据
static inline void* procXpkReadSolidEntryData(xpkObject objXpk, xpkEntry* pEntry, uint64_t* pSizeRet)
{
	void* pFileData;
	uint64_t iSolidSize;
	uint32_t iAlg;
	int iRet;

	if ( pSizeRet != NULL ) {
		*pSizeRet = 0;
	}
	if ( pEntry == NULL ) {
		procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
		return NULL;
	}
	if ( pEntry->iFileSize == 0 ) {
		pFileData = xpkAllocInternal(1);
		if ( pFileData == NULL ) {
			procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			return NULL;
		}
		procXpkClearError(objXpk);
		return pFileData;
	}
	if ( procXpkSolidStoredCompLevel(objXpk) == 0 ) {
		iRet = procXpkCalcSolidStoredRawSize(objXpk, &iSolidSize);
		if ( iRet != XPK_OK ) {
			return NULL;
		}
		if ( pEntry->iFileSize > UINT32_MAX ) {
			procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
			return NULL;
		}
		if ( (pEntry->iDataOffset + pEntry->iFileSize) > iSolidSize ) {
			procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			return NULL;
		}
		iRet = procXpkReadAtAlloc(objXpk, NULL, XPK_HEAD_SIZE + pEntry->iDataOffset, (uint32_t)pEntry->iFileSize, &pFileData);
		if ( iRet != XPK_OK ) {
			return NULL;
		}
		if ( pSizeRet != NULL ) {
			*pSizeRet = pEntry->iFileSize;
		}
		procXpkClearError(objXpk);
		return pFileData;
	}

	iAlg = procXpkCompLevelToAlg(procXpkSolidStoredCompLevel(objXpk));
	if ( iAlg == XPK_ALG_LZ4 || iAlg == XPK_ALG_LZ4HC ) {
		iRet = procXpkReadSolidLz4SliceAlloc(objXpk, pEntry, &pFileData, pSizeRet);
		if ( iRet != XPK_OK ) {
			return NULL;
		}
		return pFileData;
	}
	if ( iAlg == XPK_ALG_ZSTD ) {
		iRet = procXpkReadSolidZstdSliceAlloc(objXpk, pEntry, &pFileData, pSizeRet);
		if ( iRet != XPK_OK ) {
			return NULL;
		}
		return pFileData;
	}
	if ( iAlg == XPK_ALG_LZMA2 ) {
		iRet = procXpkReadSolidLzma2SliceAlloc(objXpk, pEntry, &pFileData, pSizeRet);
		if ( iRet != XPK_OK ) {
			return NULL;
		}
		return pFileData;
	}

	procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	return NULL;
}

// 将 Solid 条目复制到文件
static inline int procXpkCopySolidEntryToFile(xpkObject objXpk, xpkEntry* pEntry, const char* sPathFile)
{
	uint64_t iSolidSize;
	uint32_t iAlg;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sPathFile == NULL || sPathFile[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( !objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( pEntry->iFileSize == 0 ) {
		return procXpkWriteFileData(objXpk, sPathFile, NULL, 0);
	}
	if ( procXpkSolidStoredCompLevel(objXpk) == 0 ) {
		iRet = procXpkCalcSolidStoredRawSize(objXpk, &iSolidSize);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		if ( (pEntry->iDataOffset + pEntry->iFileSize) > iSolidSize ) {
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		return procXpkCopyPackageRangeToFile(objXpk, XPK_HEAD_SIZE + pEntry->iDataOffset, pEntry->iFileSize, sPathFile);
	}

	iAlg = procXpkCompLevelToAlg(procXpkSolidStoredCompLevel(objXpk));
	if ( iAlg == XPK_ALG_LZ4 || iAlg == XPK_ALG_LZ4HC ) {
		return procXpkCopySolidLz4EntryToFile(objXpk, pEntry, sPathFile);
	}
	if ( iAlg == XPK_ALG_ZSTD ) {
		return procXpkCopySolidZstdEntryToFile(objXpk, pEntry, sPathFile);
	}
	if ( iAlg == XPK_ALG_LZMA2 ) {
		return procXpkCopySolidLzma2EntryToFile(objXpk, pEntry, sPathFile);
	}

	return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
}

// 原样条目数据
static inline int procXpkStoreEntryData(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint64_t iSize, const xpkWriteOptions* pOpt)
{
	uint8_t iLevel;
	uint8_t iWritePolicy;
	int iRet;

	if ( objXpk->bSolidApplied ) {
		return procXpkSetError(objXpk, XPK_ERR_SOLID_DATA_WRITE, "solid mode does not allow file data writes");
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( iSize > UINT32_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}

	iRet = procXpkResolveCompLevel(objXpk, pOpt, &iLevel);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iWritePolicy = procXpkResolveWritePolicy(objXpk, pOpt);
	if ( iWritePolicy == XPK_WRITE_IMMEDIATE ) {
		iRet = procXpkWriteImmediate(objXpk, pEntry, pData, (uint32_t)iSize, iLevel);
	} else {
		iRet = procXpkWriteBuffered(objXpk, pEntry, pData, (uint32_t)iSize, iLevel);
	}
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	objXpk->bDirtyData = TRUE;
	objXpk->bDirtyEntryTable = TRUE;
	objXpk->bDirtyHead = TRUE;
	objXpk->objHead.changeTime = xpkNowInternal();
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 基于文件句柄读取条目数据
static inline void* procXpkReadEntryDataWithFile(xpkObject objXpk, xpkEntry* pEntry, uint64_t* pSizeRet, xfile hFile)
{
	xpkWriteNode* pNode;
	xfile hFileLocal;
	xpkMappedFile objMap;
	uint64_t iFileSize;
	void* pCompData;
	void* pRawData;
	int iRet;
	uint8_t iLevel;

	if ( pSizeRet != NULL ) {
		*pSizeRet = 0;
	}
	if ( pEntry == NULL ) {
		procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
		return NULL;
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
		return NULL;
	}
	if ( pEntry->iFileSize == 0 ) {
		pRawData = xpkAllocInternal(1);
		if ( pRawData == NULL ) {
			procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			return NULL;
		}
		if ( pSizeRet != NULL ) {
			*pSizeRet = 0;
		}
		procXpkClearError(objXpk);
		return pRawData;
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkReadSolidEntryData(objXpk, pEntry, pSizeRet);
	}

	pNode = procXpkFindWriteNode(objXpk, pEntry->iPos, NULL);
	iLevel = (uint8_t)(pEntry->iFlag & XPK_FLAG_COMP_MASK);
	if ( pNode != NULL ) {
		if ( pEntry->iFileSize > UINT32_MAX ) {
			procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
			return NULL;
		}
		iRet = procXpkCodecDecode(objXpk, pNode->iLevel, pNode->pCompData, pNode->iCompSize, (uint32_t)pEntry->iFileSize, &pRawData);
		if ( iRet != XPK_OK ) {
			return NULL;
		}
		if ( pSizeRet != NULL ) {
			*pSizeRet = pEntry->iFileSize;
		}
		procXpkClearError(objXpk);
		return pRawData;
	}

	hFileLocal = NULL;
	memset(&objMap, 0, sizeof(objMap));
	iFileSize = 0;
	if ( hFile == NULL && !procXpkAppliedVolumeMode(objXpk) ) {
		hFileLocal = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileLocal == NULL ) {
			procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
			return NULL;
		}
		hFile = hFileLocal;
	}
	if ( pEntry->iDataSize > UINT32_MAX || pEntry->iFileSize > UINT32_MAX ) {
		if ( hFileLocal != NULL ) {
			xrtClose(hFileLocal);
		}
		procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
		return NULL;
	}
	if ( !procXpkAppliedVolumeMode(objXpk) && iLevel != 0 ) {
		iFileSize = xrtGetEOF(hFile);
		if ( pEntry->iDataOffset > iFileSize || pEntry->iDataSize > (iFileSize - pEntry->iDataOffset) ) {
			if ( hFileLocal != NULL ) {
				xrtClose(hFileLocal);
			}
			procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			return NULL;
		}

		iRet = procXpkMapFileReadOnly(objXpk, hFile, iFileSize, &objMap);
		if ( hFileLocal != NULL ) {
			xrtClose(hFileLocal);
		}
		if ( iRet != XPK_OK ) {
			return NULL;
		}

		iRet = procXpkCodecDecode(objXpk, iLevel, (const uint8_t*)objMap.pView + pEntry->iDataOffset, (uint32_t)pEntry->iDataSize, (uint32_t)pEntry->iFileSize, &pRawData);
		procXpkUnmapFile(&objMap);
		if ( iRet != XPK_OK ) {
			return NULL;
		}
		if ( pSizeRet != NULL ) {
			*pSizeRet = pEntry->iFileSize;
		}
		procXpkClearError(objXpk);
		return pRawData;
	}

	pCompData = NULL;
	iRet = procXpkReadAtAlloc(objXpk, hFile, pEntry->iDataOffset, (uint32_t)pEntry->iDataSize, &pCompData);
	if ( hFileLocal != NULL ) {
		xrtClose(hFileLocal);
	}
	if ( iRet != XPK_OK ) {
		return NULL;
	}

	if ( iLevel == 0 ) {
		if ( pEntry->iDataSize != pEntry->iFileSize ) {
			xpkFreeInternal(pCompData);
			procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			return NULL;
		}
		if ( pSizeRet != NULL ) {
			*pSizeRet = pEntry->iFileSize;
		}
		procXpkClearError(objXpk);
		return pCompData;
	}

	iRet = procXpkCodecDecode(objXpk, iLevel, pCompData, (uint32_t)pEntry->iDataSize, (uint32_t)pEntry->iFileSize, &pRawData);
	xrtFree(pCompData);
	if ( iRet != XPK_OK ) {
		return NULL;
	}
	if ( pSizeRet != NULL ) {
		*pSizeRet = pEntry->iFileSize;
	}
	procXpkClearError(objXpk);
	return pRawData;
}

// 读取条目数据
static inline void* procXpkReadEntryData(xpkObject objXpk, xpkEntry* pEntry, uint64_t* pSizeRet)
{
	return procXpkReadEntryDataWithFile(objXpk, pEntry, pSizeRet, NULL);
}

// 刷写延迟写入队列
static inline int procXpkFlushQueuedWrites(xpkObject objXpk, xfile hFile)
{
	uint32_t iNodePos;
	xpkWriteNode* pNode;
	xpkEntry* pEntry;
	int iRet;

	if ( (objXpk == NULL) || (objXpk->pWriteQueue == NULL) || (objXpk->pWriteQueue->arrNode.Count == 0) ) {
		return XPK_OK;
	}

	// 顺序落盘队列中的压缩块，并同步回填条目的最终偏移和大小。
	for ( iNodePos = 1; iNodePos <= objXpk->pWriteQueue->arrNode.Count; iNodePos++ ) {
		pNode = (xpkWriteNode*)xrtArrayGet(&objXpk->pWriteQueue->arrNode, iNodePos);
		if ( pNode == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}

		pEntry = procXpkGetEntryByPos(objXpk, pNode->iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}

		iRet = procXpkWriteAt(objXpk, hFile, objXpk->iAppendPos, pNode->pCompData, pNode->iCompSize);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		pEntry->iDataOffset = objXpk->iAppendPos;
		pEntry->iDataSize = pNode->iCompSize;
		pEntry->iFileSize = pNode->iRawSize;
		pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_COMP_MASK) | pNode->iLevel;
		objXpk->iAppendPos += pNode->iCompSize;
		if ( objXpk->iAppendPos > objXpk->iFileSize ) {
			objXpk->iFileSize = objXpk->iAppendPos;
		}
	}

	return XPK_OK;
}


/* ===== File: src/service/save.h ===== */

/*
	xPack 保存模块

	负责将脏状态落盘，并在失败时执行回滚恢复。
*/

// 捕获刷写快照
static inline xpkFlushSnapshot* procXpkCaptureFlushSnapshots(xpkObject objXpk, uint32_t* pCountRet)
{
	xpkFlushSnapshot* arrSnapshot;
	xpkWriteNode* pNode;
	xpkEntry* pEntry;
	uint32_t iCount;
	uint32_t iNodePos;

	if ( pCountRet != NULL ) {
		*pCountRet = 0;
	}
	if ( objXpk == NULL ) {
		return NULL;
	}

	iCount = (uint32_t)procXpkWriteQueueCount(objXpk);
	if ( iCount == 0 ) {
		return NULL;
	}

	arrSnapshot = (xpkFlushSnapshot*)xpkAllocInternal(sizeof(*arrSnapshot) * iCount);
	if ( arrSnapshot == NULL ) {
		procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		return NULL;
	}

	for ( iNodePos = 1; iNodePos <= iCount; iNodePos++ ) {
		pNode = (xpkWriteNode*)xrtArrayGet(&objXpk->pWriteQueue->arrNode, iNodePos);
		if ( pNode == NULL ) {
			xpkFreeInternal(arrSnapshot);
			procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
			return NULL;
		}

		pEntry = procXpkGetEntryByPos(objXpk, pNode->iPos);
		if ( pEntry == NULL ) {
			xpkFreeInternal(arrSnapshot);
			procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
			return NULL;
		}

		arrSnapshot[iNodePos - 1].iPos = pNode->iPos;
		arrSnapshot[iNodePos - 1].iFlag = pEntry->iFlag;
		arrSnapshot[iNodePos - 1].iDataOffset = pEntry->iDataOffset;
		arrSnapshot[iNodePos - 1].iDataSize = pEntry->iDataSize;
		arrSnapshot[iNodePos - 1].iFileSize = pEntry->iFileSize;
	}

	if ( pCountRet != NULL ) {
		*pCountRet = iCount;
	}
	return arrSnapshot;
}

// 恢复刷写快照
static inline void procXpkRestoreFlushSnapshots(xpkObject objXpk, const xpkFlushSnapshot* arrSnapshot, uint32_t iCount, const xpkHead* pHeadSaved, uint64_t iAppendPosSaved, uint64_t iFileSizeSaved)
{
	xpkEntry* pEntry;
	uint32_t iIndex;

	if ( objXpk == NULL ) {
		return;
	}

	if ( pHeadSaved != NULL ) {
		objXpk->objHead = *pHeadSaved;
	}
	objXpk->iAppendPos = iAppendPosSaved;
	objXpk->iFileSize = iFileSizeSaved;

	if ( arrSnapshot == NULL ) {
		return;
	}

	for ( iIndex = 0; iIndex < iCount; iIndex++ ) {
		pEntry = procXpkGetEntryByPos(objXpk, arrSnapshot[iIndex].iPos);
		if ( pEntry == NULL ) {
			continue;
		}

		pEntry->iFlag = arrSnapshot[iIndex].iFlag;
		pEntry->iDataOffset = arrSnapshot[iIndex].iDataOffset;
		pEntry->iDataSize = arrSnapshot[iIndex].iDataSize;
		pEntry->iFileSize = arrSnapshot[iIndex].iFileSize;
	}
}

// 释放保存回滚
static inline void procXpkFreeSaveRollback(xpkSaveRollback* pRollback)
{
	if ( pRollback == NULL ) {
		return;
	}
	if ( pRollback->pTailData != NULL ) {
		xpkFreeInternal(pRollback->pTailData);
		pRollback->pTailData = NULL;
	}
	if ( pRollback->sTailPath != NULL ) {
		if ( !pRollback->bKeepTailFile && xrtFileExists((str)pRollback->sTailPath) ) {
			(void)xrtFileDelete((str)pRollback->sTailPath);
		}
		xpkFreeInternal(pRollback->sTailPath);
		pRollback->sTailPath = NULL;
	}
}

// 保存回滚路径复制
static inline char* procXpkSaveRollbackPathDup(xpkObject objXpk)
{
	char sSuffix[96];
	char* sPathRet;
	uint64_t iStamp;
	uint64_t iTag;
	uint32_t iTry;
	int iSizePrint;

	if ( objXpk == NULL || objXpk->sPathPackage == NULL ) {
		procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
		return NULL;
	}

	iStamp = (uint64_t)xpkNowInternal();
	iTag = (uint64_t)(uintptr_t)objXpk;
	for ( iTry = 0; iTry < 32; iTry++ ) {
		iSizePrint = snprintf(
			sSuffix,
			sizeof(sSuffix),
			".save.rollback.%llu.%llx.%u.tmp",
			(unsigned long long)iStamp,
			(unsigned long long)iTag,
			(unsigned int)iTry
		);
		if ( iSizePrint <= 0 || (size_t)iSizePrint >= sizeof(sSuffix) ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite), NULL;
		}

		sPathRet = procXpkPathSuffixDupText(objXpk->sPathPackage, sSuffix);
		if ( sPathRet == NULL ) {
			procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			return NULL;
		}
		if ( !xrtPathExists((str)sPathRet) ) {
			return sPathRet;
		}

		xpkFreeInternal(sPathRet);
	}

	procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	return NULL;
}

// 将回滚尾段写入文件
static inline int procXpkCaptureRollbackTailToFile(xpkObject objXpk, uint64_t iOffset, uint64_t iSize, const char* sPathTail)
{
	xfile hFile;
	void* pChunk;
	uint64_t iRemain;
	uint64_t iOffsetCur;
	uint32_t iChunk;
	size_t iWrite;
	int iRet;

	if ( objXpk == NULL || sPathTail == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	hFile = xrtOpen((str)sPathTail, FALSE, XRT_CP_BINARY);
	if ( hFile == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iRemain = iSize;
	iOffsetCur = iOffset;
	while ( iRemain > 0 ) {
		iChunk = (uint32_t)((iRemain > XPK_SAVE_ROLLBACK_CHUNK_SIZE) ? XPK_SAVE_ROLLBACK_CHUNK_SIZE : iRemain);
		pChunk = NULL;
		iRet = procXpkReadAtAlloc(objXpk, NULL, iOffsetCur, iChunk, &pChunk);
		if ( iRet != XPK_OK ) {
			xrtClose(hFile);
			return iRet;
		}

		iWrite = xrtPut(hFile, (ptr)pChunk, iChunk);
		xpkFreeInternal(pChunk);
		if ( iWrite != iChunk ) {
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}

		iOffsetCur += iChunk;
		iRemain -= iChunk;
	}

	if ( !xrtSetEOF(hFile) ) {
		xrtClose(hFile);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	xrtClose(hFile);
	return XPK_OK;
}

// 从文件恢复回滚尾段
static inline int procXpkRestoreRollbackTailFromFile(xpkObject objXpk, const xpkSaveRollback* pRollback)
{
	xfile hFile;
	void* pChunk;
	uint64_t iRemain;
	uint64_t iOffsetCur;
	uint64_t iBackupSize;
	uint32_t iChunk;
	size_t iRead;
	int iRet;

	if ( objXpk == NULL || pRollback == NULL || pRollback->sTailPath == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	hFile = xrtOpen((str)pRollback->sTailPath, TRUE, XRT_CP_BINARY);
	if ( hFile == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iBackupSize = (uint64_t)xrtGetEOF(hFile);
	if ( iBackupSize != pRollback->iTailSize ) {
		xrtClose(hFile);
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
	}

	iRemain = pRollback->iTailSize;
	iOffsetCur = pRollback->iDataOffset;
	while ( iRemain > 0 ) {
		iChunk = (uint32_t)((iRemain > XPK_SAVE_ROLLBACK_CHUNK_SIZE) ? XPK_SAVE_ROLLBACK_CHUNK_SIZE : iRemain);
		pChunk = xpkAllocInternal(iChunk);
		if ( pChunk == NULL ) {
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		iRead = xrtGetBuffer(hFile, pChunk, iChunk);
		if ( iRead != iChunk ) {
			xpkFreeInternal(pChunk);
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoRead);
		}

		iRet = procXpkWriteAt(objXpk, NULL, iOffsetCur, pChunk, iChunk);
		xpkFreeInternal(pChunk);
		if ( iRet != XPK_OK ) {
			xrtClose(hFile);
			return iRet;
		}

		iOffsetCur += iChunk;
		iRemain -= iChunk;
	}

	xrtClose(hFile);
	return XPK_OK;
}

// 捕获保存回滚上下文
static inline int procXpkCaptureSaveRollback(xpkObject objXpk, xpkSaveRollback* pRollback)
{
	void* pHeadData;
	uint64_t iLogicalSize;
	uint64_t iTailSize64;
	int iRet;

	if ( objXpk == NULL || pRollback == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	memset(pRollback, 0, sizeof(*pRollback));
	pRollback->bFileExisted = xrtFileExists((str)objXpk->sPathPackage) ? TRUE : FALSE;
	if ( !pRollback->bFileExisted ) {
		return XPK_OK;
	}

	iLogicalSize = 0;
	if ( procXpkAppliedVolumeMode(objXpk) ) {
		iRet = procXpkCalcLogicalFileSize(objXpk, &objXpk->objHead, &iLogicalSize);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
	} else {
		iLogicalSize = (uint64_t)xrtFileGetSize((str)objXpk->sPathPackage);
	}
	pRollback->iLogicalSize = iLogicalSize;
	pRollback->iDataOffset = objXpk->objHead.dataOffset;

	if ( iLogicalSize >= XPK_HEAD_SIZE ) {
		pHeadData = NULL;
		iRet = procXpkReadAtAlloc(objXpk, NULL, 0, XPK_HEAD_SIZE, &pHeadData);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		memcpy(pRollback->sHeadBuf, pHeadData, XPK_HEAD_SIZE);
		xpkFreeInternal(pHeadData);
		pRollback->bHeadValid = TRUE;
	}

	if ( iLogicalSize <= pRollback->iDataOffset ) {
		return XPK_OK;
	}

	iTailSize64 = iLogicalSize - pRollback->iDataOffset;
	pRollback->iTailSize = iTailSize64;
	if ( iTailSize64 <= XPK_SAVE_ROLLBACK_MEM_LIMIT ) {
		iRet = procXpkReadAtAlloc(objXpk, NULL, pRollback->iDataOffset, (uint32_t)iTailSize64, &pRollback->pTailData);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		return XPK_OK;
	}

	pRollback->sTailPath = procXpkSaveRollbackPathDup(objXpk);
	if ( pRollback->sTailPath == NULL ) {
		return xpkLastError(objXpk);
	}
	iRet = procXpkCaptureRollbackTailToFile(objXpk, pRollback->iDataOffset, iTailSize64, pRollback->sTailPath);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	pRollback->bTailInFile = TRUE;
	return XPK_OK;
}

// 恢复保存回滚上下文
static inline int procXpkRestoreSaveRollback(xpkObject objXpk, const xpkSaveRollback* pRollback)
{
	int iRet;
	uint32_t iVolumeLast;
	uint32_t iVolume;
	uint32_t iVolumeSize;
	uint64_t iLastSize;
	xfile hFile;

	if ( objXpk == NULL || pRollback == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	if ( !pRollback->bFileExisted ) {
		if ( xrtFileExists((str)objXpk->sPathPackage) && !xrtFileDelete((str)objXpk->sPathPackage) ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}
		return procXpkScanLooseVolumeFilesText(objXpk, objXpk->sPathPackage, TRUE, NULL);
	}

	if ( pRollback->iTailSize > 0 ) {
		if ( pRollback->bTailInFile ) {
			iRet = procXpkRestoreRollbackTailFromFile(objXpk, pRollback);
			if ( iRet != XPK_OK ) {
				((xpkSaveRollback*)pRollback)->bKeepTailFile = TRUE;
				return iRet;
			}
		} else {
			iRet = procXpkWriteAt(objXpk, NULL, pRollback->iDataOffset, pRollback->pTailData, (uint32_t)pRollback->iTailSize);
			if ( iRet != XPK_OK ) {
				return iRet;
			}
		}
	}

	if ( procXpkAppliedVolumeMode(objXpk) ) {
		iVolumeSize = procXpkAppliedVolumeSize(objXpk);
		if ( iVolumeSize == 0 ) {
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}

		if ( pRollback->iLogicalSize == 0 ) {
			iVolumeLast = 0;
			iLastSize = 0;
		} else {
			iVolumeLast = (uint32_t)((pRollback->iLogicalSize - 1) / iVolumeSize);
			iLastSize = pRollback->iLogicalSize - ((uint64_t)iVolumeLast * (uint64_t)iVolumeSize);
		}

		for ( iVolume = 0; iVolume < iVolumeLast; iVolume++ ) {
			hFile = NULL;
			iRet = procXpkOpenVolumeText(objXpk, objXpk->sPathPackage, iVolume, FALSE, &hFile);
			if ( iRet != XPK_OK ) {
				return iRet;
			}

			iRet = procXpkSeekFile(objXpk, hFile, iVolumeSize);
			if ( iRet != XPK_OK ) {
				xrtClose(hFile);
				return iRet;
			}
			if ( !xrtSetEOF(hFile) ) {
				xrtClose(hFile);
				return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
			}

			xrtClose(hFile);
		}

		hFile = NULL;
		iRet = procXpkOpenVolumeText(objXpk, objXpk->sPathPackage, iVolumeLast, FALSE, &hFile);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		iRet = procXpkSeekFile(objXpk, hFile, iLastSize);
		if ( iRet != XPK_OK ) {
			xrtClose(hFile);
			return iRet;
		}
		if ( !xrtSetEOF(hFile) ) {
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
		}

		xrtClose(hFile);
		iRet = procXpkDeleteVolumeFilesContiguousText(objXpk, objXpk->sPathPackage, iVolumeLast + 1);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
	} else {
		iRet = procXpkSetEOFAt(objXpk, NULL, pRollback->iLogicalSize);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
	}
	if ( pRollback->bHeadValid ) {
		iRet = procXpkWriteAt(objXpk, NULL, 0, pRollback->sHeadBuf, XPK_HEAD_SIZE);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
	}

	return XPK_OK;
}

// 执行保存落盘流程
static inline int procXpkSavePackage(xpkObject objXpk)
{
	xfile hFile;
	void* pMetaComp;
	void* pEntryRaw;
	void* pEntryComp;
	xpkFlushSnapshot* arrSnapshot;
	xpkSaveRollback objRollback;
	uint32_t iMetaCompSize;
	uint32_t iEntryRawSize;
	uint32_t iEntryCompSize;
	uint32_t iSnapshotCount;
	uint8_t iMetaLevel;
	uint8_t iInfoLevel;
	uint64_t iTailPos;
	uint64_t iAppendPosSaved;
	uint64_t iFileSizeSaved;
	uint8_t sHeadBuf[XPK_HEAD_SIZE];
	xpkHead objHeadSaved;
	int iRet;
	uint32_t iPos;
	xpkEntry* pEntry;

	// 先确认当前对象允许直接保存，布局变更需要交给 build 路径处理。
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkSolidLayoutChanged(objXpk) && (objXpk->iEntryCount != 0) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorSolidBuildRequired);
	}
	if ( procXpkVolumeLayoutChanged(objXpk) && !procXpkCanAdoptTargetLayout(objXpk) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorVolumeBuildRequired);
	}

	pMetaComp = NULL;
	pEntryRaw = NULL;
	pEntryComp = NULL;
	arrSnapshot = NULL;
	memset(&objRollback, 0, sizeof(objRollback));
	iMetaCompSize = 0;
	iEntryRawSize = 0;
	iEntryCompSize = 0;
	iSnapshotCount = 0;
	iMetaLevel = (uint8_t)objXpk->objHead.metaComp;
	iInfoLevel = (uint8_t)objXpk->objHead.infoComp;
	objHeadSaved = objXpk->objHead;
	iAppendPosSaved = objXpk->iAppendPos;
	iFileSizeSaved = objXpk->iFileSize;

	// 预编码元数据，并提前抓取刷写快照与回滚上下文，保证中途失败后还能恢复现场。
	iRet = procXpkCodecEncode(objXpk, (uint8_t)objXpk->objHead.metaComp, objXpk->pPackageMeta, objXpk->iPackageMetaSize, &pMetaComp, &iMetaCompSize, &iMetaLevel);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	arrSnapshot = procXpkCaptureFlushSnapshots(objXpk, &iSnapshotCount);
	if ( (iSnapshotCount > 0) && (arrSnapshot == NULL) ) {
		if ( pMetaComp != NULL ) {
			xpkFreeInternal(pMetaComp);
		}
		return xpkLastError(objXpk);
	}
	iRet = procXpkCaptureSaveRollback(objXpk, &objRollback);
	if ( iRet != XPK_OK ) {
		if ( pMetaComp != NULL ) {
			xpkFreeInternal(pMetaComp);
		}
		if ( arrSnapshot != NULL ) {
			xpkFreeInternal(arrSnapshot);
		}
		return iRet;
	}

	hFile = NULL;
	if ( !procXpkAppliedVolumeMode(objXpk) ) {
		hFile = xrtOpen(objXpk->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFile == NULL ) {
			iRet = procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		} else {
			iRet = XPK_OK;
		}
	} else {
		iRet = XPK_OK;
	}
	if ( iRet != XPK_OK ) {
		if ( pMetaComp != NULL ) {
			xpkFreeInternal(pMetaComp);
		}
		if ( pEntryRaw != NULL ) {
			xpkFreeInternal(pEntryRaw);
		}
		if ( pEntryComp != NULL ) {
			xpkFreeInternal(pEntryComp);
		}
		if ( arrSnapshot != NULL ) {
			xpkFreeInternal(arrSnapshot);
		}
		procXpkFreeSaveRollback(&objRollback);
		return iRet;
	}

	// 先把延迟写入队列刷入数据区，再重新编码条目表，保证偏移信息使用的是最终值。
	iRet = procXpkFlushQueuedWrites(objXpk, hFile);
	if ( iRet != XPK_OK ) {
		if ( hFile != NULL ) {
			xrtClose(hFile);
		}
		procXpkRestoreFlushSnapshots(objXpk, arrSnapshot, iSnapshotCount, &objHeadSaved, iAppendPosSaved, iFileSizeSaved);
		if ( procXpkRestoreSaveRollback(objXpk, &objRollback) != XPK_OK ) {
			iRet = xpkLastError(objXpk);
		}
		if ( pMetaComp != NULL ) {
			xpkFreeInternal(pMetaComp);
		}
		if ( pEntryRaw != NULL ) {
			xpkFreeInternal(pEntryRaw);
		}
		if ( pEntryComp != NULL ) {
			xpkFreeInternal(pEntryComp);
		}
		if ( arrSnapshot != NULL ) {
			xpkFreeInternal(arrSnapshot);
		}
		procXpkFreeSaveRollback(&objRollback);
		return iRet;
	}

	iRet = procXpkEncodeEntryTable(objXpk, &pEntryRaw, &iEntryRawSize);
	if ( iRet != XPK_OK ) {
		if ( hFile != NULL ) {
			xrtClose(hFile);
		}
		procXpkRestoreFlushSnapshots(objXpk, arrSnapshot, iSnapshotCount, &objHeadSaved, iAppendPosSaved, iFileSizeSaved);
		if ( procXpkRestoreSaveRollback(objXpk, &objRollback) != XPK_OK ) {
			iRet = xpkLastError(objXpk);
		}
		if ( pMetaComp != NULL ) {
			xpkFreeInternal(pMetaComp);
		}
		if ( arrSnapshot != NULL ) {
			xpkFreeInternal(arrSnapshot);
		}
		procXpkFreeSaveRollback(&objRollback);
		return iRet;
	}

	iRet = procXpkCodecEncode(objXpk, (uint8_t)objXpk->objHead.infoComp, pEntryRaw, iEntryRawSize, &pEntryComp, &iEntryCompSize, &iInfoLevel);
	if ( iRet != XPK_OK ) {
		if ( hFile != NULL ) {
			xrtClose(hFile);
		}
		procXpkRestoreFlushSnapshots(objXpk, arrSnapshot, iSnapshotCount, &objHeadSaved, iAppendPosSaved, iFileSizeSaved);
		if ( procXpkRestoreSaveRollback(objXpk, &objRollback) != XPK_OK ) {
			iRet = xpkLastError(objXpk);
		}
		if ( pMetaComp != NULL ) {
			xpkFreeInternal(pMetaComp);
		}
		if ( pEntryRaw != NULL ) {
			xpkFreeInternal(pEntryRaw);
		}
		if ( arrSnapshot != NULL ) {
			xpkFreeInternal(arrSnapshot);
		}
		procXpkFreeSaveRollback(&objRollback);
		return iRet;
	}

	// 组装新的包头与尾段布局，并把元数据、条目表和头部一次性落盘。
	objXpk->objHead.fileCount = objXpk->iEntryCount;
	objXpk->objHead.dataOffset = objXpk->iAppendPos;
	objXpk->objHead.metaComp = iMetaLevel;
	objXpk->objHead.infoComp = iInfoLevel;
	objXpk->objHead.metaRawSize = objXpk->iPackageMetaSize;
	objXpk->objHead.metaCompSize = iMetaCompSize;
	objXpk->objHead.metaHash = (objXpk->iPackageMetaSize > 0) ? xpkHash32Internal(objXpk->pPackageMeta, objXpk->iPackageMetaSize) : 0;
	objXpk->objHead.infoCompSize = iEntryCompSize;
	objXpk->objHead.infoHash = (iEntryRawSize > 0) ? xpkHash32Internal(pEntryRaw, iEntryRawSize) : 0;
	if ( objXpk->objHead.createTime == 0 ) {
		objXpk->objHead.createTime = xpkNowInternal();
	}
	objXpk->objHead.changeTime = xpkNowInternal();

	iRet = procXpkWriteAt(objXpk, hFile, objXpk->objHead.dataOffset, pMetaComp, iMetaCompSize);
	if ( iRet == XPK_OK ) {
		iRet = procXpkWriteAt(objXpk, hFile, objXpk->objHead.dataOffset + iMetaCompSize, pEntryComp, iEntryCompSize);
	}
	if ( iRet == XPK_OK ) {
		iTailPos = objXpk->objHead.dataOffset + iMetaCompSize + iEntryCompSize;
		iRet = procXpkSetEOFAt(objXpk, hFile, iTailPos);
		if ( iRet == XPK_OK ) {
			objXpk->iFileSize = iTailPos;
		}
	}
	if ( iRet == XPK_OK ) {
		procXpkEncodeHead(&objXpk->objHead, sHeadBuf);
		iRet = procXpkWriteAt(objXpk, hFile, 0, sHeadBuf, XPK_HEAD_SIZE);
	}

	// 写盘完成后统一释放临时缓冲；失败则回滚快照和原始尾段。
	if ( hFile != NULL ) {
		xrtClose(hFile);
	}
	if ( pMetaComp != NULL ) {
		xpkFreeInternal(pMetaComp);
	}
	if ( pEntryRaw != NULL ) {
		xpkFreeInternal(pEntryRaw);
	}
	if ( pEntryComp != NULL ) {
		xpkFreeInternal(pEntryComp);
	}

	if ( iRet != XPK_OK ) {
		procXpkRestoreFlushSnapshots(objXpk, arrSnapshot, iSnapshotCount, &objHeadSaved, iAppendPosSaved, iFileSizeSaved);
		if ( procXpkRestoreSaveRollback(objXpk, &objRollback) != XPK_OK ) {
			iRet = xpkLastError(objXpk);
		}
		if ( arrSnapshot != NULL ) {
			xpkFreeInternal(arrSnapshot);
		}
		procXpkFreeSaveRollback(&objRollback);
		return iRet;
	}

	// 保存成功后把条目状态标记为已落盘，并清理脏标记。
	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry != NULL ) {
			pEntry->bStored = TRUE;
		}
	}
	procXpkUnitWriteQueue(objXpk);
	if ( arrSnapshot != NULL ) {
		xpkFreeInternal(arrSnapshot);
	}
	procXpkFreeSaveRollback(&objRollback);

	procXpkMarkAppliedLayout(objXpk);
	procXpkMarkClean(objXpk);
	procXpkClearError(objXpk);
	return XPK_OK;
}


/* ===== File: src/service/build.h ===== */

/*
	xPack Build 重构模块

	负责按目标布局重写包体、复制有效条目并处理临时文件。
*/

// 判断文件系统路径文本是否相等
static inline int procXpkPathTextEqualFs(const char* sPathA, const char* sPathB)
{
	if ( sPathA == NULL || sPathB == NULL ) {
		return FALSE;
	}

#ifdef _WIN32
	size_t iPosA;
	size_t iPosB;
	int bSegStartA;
	int bSegStartB;
	unsigned char iChA;
	unsigned char iChB;

	iPosA = 0;
	iPosB = 0;
	bSegStartA = TRUE;
	bSegStartB = TRUE;
	for ( ;; ) {
		for ( ;; ) {
			if ( bSegStartA && sPathA[iPosA] == '.' &&
				(sPathA[iPosA + 1] == '/' || sPathA[iPosA + 1] == '\\') ) {
				iPosA += 2;
				while ( sPathA[iPosA] == '/' || sPathA[iPosA] == '\\' ) {
					iPosA++;
				}
				bSegStartA = TRUE;
				continue;
			}
			if ( sPathA[iPosA] == '/' || sPathA[iPosA] == '\\' ) {
				while ( sPathA[iPosA] == '/' || sPathA[iPosA] == '\\' ) {
					iPosA++;
				}
				if ( sPathA[iPosA] == '\0' ) {
					iChA = '\0';
					break;
				}
				iChA = '/';
				bSegStartA = TRUE;
				break;
			}
			iChA = (unsigned char)tolower((unsigned char)sPathA[iPosA]);
			if ( iChA != '\0' ) {
				iPosA++;
			}
			bSegStartA = FALSE;
			break;
		}

		for ( ;; ) {
			if ( bSegStartB && sPathB[iPosB] == '.' &&
				(sPathB[iPosB + 1] == '/' || sPathB[iPosB + 1] == '\\') ) {
				iPosB += 2;
				while ( sPathB[iPosB] == '/' || sPathB[iPosB] == '\\' ) {
					iPosB++;
				}
				bSegStartB = TRUE;
				continue;
			}
			if ( sPathB[iPosB] == '/' || sPathB[iPosB] == '\\' ) {
				while ( sPathB[iPosB] == '/' || sPathB[iPosB] == '\\' ) {
					iPosB++;
				}
				if ( sPathB[iPosB] == '\0' ) {
					iChB = '\0';
					break;
				}
				iChB = '/';
				bSegStartB = TRUE;
				break;
			}
			iChB = (unsigned char)tolower((unsigned char)sPathB[iPosB]);
			if ( iChB != '\0' ) {
				iPosB++;
			}
			bSegStartB = FALSE;
			break;
		}

		if ( iChA != iChB ) {
			return FALSE;
		}
		if ( iChA == '\0' ) {
			return TRUE;
		}
	}
#else
	size_t iPosA;
	size_t iPosB;
	int bSegStartA;
	int bSegStartB;
	unsigned char iChA;
	unsigned char iChB;

	iPosA = 0;
	iPosB = 0;
	bSegStartA = TRUE;
	bSegStartB = TRUE;
	for ( ;; ) {
		for ( ;; ) {
			if ( bSegStartA && sPathA[iPosA] == '.' && sPathA[iPosA + 1] == '/' ) {
				iPosA += 2;
				while ( sPathA[iPosA] == '/' ) {
					iPosA++;
				}
				bSegStartA = TRUE;
				continue;
			}
			if ( sPathA[iPosA] == '/' ) {
				while ( sPathA[iPosA] == '/' ) {
					iPosA++;
				}
				if ( sPathA[iPosA] == '\0' ) {
					iChA = '\0';
					break;
				}
				iChA = '/';
				bSegStartA = TRUE;
				break;
			}
			iChA = (unsigned char)sPathA[iPosA];
			if ( iChA != '\0' ) {
				iPosA++;
			}
			bSegStartA = FALSE;
			break;
		}

		for ( ;; ) {
			if ( bSegStartB && sPathB[iPosB] == '.' && sPathB[iPosB + 1] == '/' ) {
				iPosB += 2;
				while ( sPathB[iPosB] == '/' ) {
					iPosB++;
				}
				bSegStartB = TRUE;
				continue;
			}
			if ( sPathB[iPosB] == '/' ) {
				while ( sPathB[iPosB] == '/' ) {
					iPosB++;
				}
				if ( sPathB[iPosB] == '\0' ) {
					iChB = '\0';
					break;
				}
				iChB = '/';
				bSegStartB = TRUE;
				break;
			}
			iChB = (unsigned char)sPathB[iPosB];
			if ( iChB != '\0' ) {
				iPosB++;
			}
			bSegStartB = FALSE;
			break;
		}

		if ( iChA != iChB ) {
			return FALSE;
		}
		if ( iChA == '\0' ) {
			return TRUE;
		}
	}
#endif
}

// 复制并规范化文件系统路径
static inline char* procXpkPathNormDupFs(const char* sPath)
{
	size_t iLenPath;
	char* sPathRet;

	if ( sPath == NULL ) {
		return NULL;
	}

#ifdef _WIN32
	size_t iPosIn;
	size_t iPosOut;
	int bSegStart;
	unsigned char iCh;

	iLenPath = strlen(sPath);
	sPathRet = (char*)xpkAllocInternal(iLenPath + 2);
	if ( sPathRet == NULL ) {
		return NULL;
	}

	iPosIn = 0;
	iPosOut = 0;
	bSegStart = TRUE;
	for ( ;; ) {
		for ( ;; ) {
			if ( bSegStart && sPath[iPosIn] == '.' &&
				(sPath[iPosIn + 1] == '/' || sPath[iPosIn + 1] == '\\') ) {
				iPosIn += 2;
				while ( sPath[iPosIn] == '/' || sPath[iPosIn] == '\\' ) {
					iPosIn++;
				}
				bSegStart = TRUE;
				continue;
			}
			if ( sPath[iPosIn] == '/' || sPath[iPosIn] == '\\' ) {
				while ( sPath[iPosIn] == '/' || sPath[iPosIn] == '\\' ) {
					iPosIn++;
				}
				if ( sPath[iPosIn] == '\0' ) {
					sPathRet[iPosOut] = '\0';
					return sPathRet;
				}
				sPathRet[iPosOut++] = '/';
				bSegStart = TRUE;
				break;
			}
			iCh = (unsigned char)tolower((unsigned char)sPath[iPosIn]);
			if ( iCh == '\0' ) {
				sPathRet[iPosOut] = '\0';
				return sPathRet;
			}
			sPathRet[iPosOut++] = (char)iCh;
			iPosIn++;
			bSegStart = FALSE;
			break;
		}
	}
#else
	size_t iPosIn;
	size_t iPosOut;
	int bSegStart;

	iLenPath = strlen(sPath);
	sPathRet = (char*)xpkAllocInternal(iLenPath + 2);
	if ( sPathRet == NULL ) {
		return NULL;
	}

	iPosIn = 0;
	iPosOut = 0;
	bSegStart = TRUE;
	for ( ;; ) {
		for ( ;; ) {
			if ( bSegStart && sPath[iPosIn] == '.' && sPath[iPosIn + 1] == '/' ) {
				iPosIn += 2;
				while ( sPath[iPosIn] == '/' ) {
					iPosIn++;
				}
				bSegStart = TRUE;
				continue;
			}
			if ( sPath[iPosIn] == '/' ) {
				while ( sPath[iPosIn] == '/' ) {
					iPosIn++;
				}
				if ( sPath[iPosIn] == '\0' ) {
					sPathRet[iPosOut] = '\0';
					return sPathRet;
				}
				sPathRet[iPosOut++] = '/';
				bSegStart = TRUE;
				break;
			}
			if ( sPath[iPosIn] == '\0' ) {
				sPathRet[iPosOut] = '\0';
				return sPathRet;
			}
			sPathRet[iPosOut++] = sPath[iPosIn++];
			bSegStart = FALSE;
			break;
		}
	}
#endif
}

// 判断路径是否位于目录内
static inline int procXpkPathInDirFs(xpkObject objXpk, const char* sPath, const char* sDir, int* pMatchRet)
{
	char* sPathNorm;
	char* sDirNorm;
	size_t iDirLen;

	if ( pMatchRet != NULL ) {
		*pMatchRet = FALSE;
	}
	if ( sPath == NULL || sDir == NULL || pMatchRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	sPathNorm = procXpkPathNormDupFs(sPath);
	if ( sPathNorm == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	sDirNorm = procXpkPathNormDupFs(sDir);
	if ( sDirNorm == NULL ) {
		xpkFreeInternal(sPathNorm);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iDirLen = strlen(sDirNorm);
	if ( iDirLen > 0 && strncmp(sPathNorm, sDirNorm, iDirLen) == 0 && sPathNorm[iDirLen] == '/' ) {
		*pMatchRet = TRUE;
	}

	xpkFreeInternal(sPathNorm);
	xpkFreeInternal(sDirNorm);
	return XPK_OK;
}

// 判断路径是否属于同一分卷族
static inline int procXpkPathIsVolumeFamilyFs(xpkObject objXpk, const char* sPath, const char* sBasePath, int* pMatchRet)
{
	char* sPathNorm;
	char* sBaseNorm;
	size_t iBaseLen;
	const char* sSuffix;

	if ( pMatchRet != NULL ) {
		*pMatchRet = FALSE;
	}
	if ( sPath == NULL || sBasePath == NULL || pMatchRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	sPathNorm = procXpkPathNormDupFs(sPath);
	if ( sPathNorm == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}
	sBaseNorm = procXpkPathNormDupFs(sBasePath);
	if ( sBaseNorm == NULL ) {
		xpkFreeInternal(sPathNorm);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	iBaseLen = strlen(sBaseNorm);
	if ( strncmp(sPathNorm, sBaseNorm, iBaseLen) == 0 ) {
		sSuffix = sPathNorm + iBaseLen;
		if ( *sSuffix == '.' ) {
			sSuffix++;
			if ( *sSuffix != '\0' ) {
				*pMatchRet = TRUE;
				for ( ; *sSuffix != '\0'; sSuffix++ ) {
					if ( *sSuffix < '0' || *sSuffix > '9' ) {
						*pMatchRet = FALSE;
						break;
					}
				}
			}
		}
	}

	xpkFreeInternal(sPathNorm);
	xpkFreeInternal(sBaseNorm);
	return XPK_OK;
}

// 构建复制元数据
static inline int procXpkBuildCopyMeta(xpkObject objDst, xpkObject objSrc)
{
	void* pMetaDup;

	if ( objSrc->pPackageMeta == NULL || objSrc->iPackageMetaSize == 0 ) {
		return XPK_OK;
	}

	pMetaDup = xpkAllocInternal(objSrc->iPackageMetaSize);
	if ( pMetaDup == NULL ) {
		return procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	memcpy(pMetaDup, objSrc->pPackageMeta, objSrc->iPackageMetaSize);
	objDst->pPackageMeta = pMetaDup;
	objDst->iPackageMetaSize = objSrc->iPackageMetaSize;
	return XPK_OK;
}

// 构建复制配置
static inline int procXpkBuildCopyConfig(xpkObject objDst, xpkObject objSrc)
{
	int iRet;

	iRet = procXpkApplyPackType(objDst, (xpkPackType)objSrc->objHead.packType);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	objDst->objHead.defComp = objSrc->objHead.defComp;
	objDst->objHead.metaComp = objSrc->objHead.metaComp;
	objDst->objHead.infoComp = objSrc->objHead.infoComp;
	objDst->objHead.infoExtSize = objSrc->objHead.infoExtSize;
	objDst->objHead.volumeMode = objSrc->objHead.volumeMode;
	objDst->objHead.volumeSize = objSrc->objHead.volumeSize;
	objDst->objHead.solidMode = objSrc->objHead.solidMode;
	objDst->objHead.createTime = objSrc->objHead.createTime;
	objDst->bVolumeApplied = objDst->objHead.volumeMode ? TRUE : FALSE;
	objDst->iVolumeSizeApplied = objDst->objHead.volumeSize;
	return procXpkBuildCopyMeta(objDst, objSrc);
}

// 构建复制条目路径
static inline char* procXpkBuildDupEntryPath(xpkObject objDst, const char* sPath)
{
	char* sPathDup;
	int iErr;

	if ( sPath == NULL ) {
		return NULL;
	}
	if ( (objDst->objHead.packType != XPK_PACK_LINUX) && (objDst->objHead.packType != XPK_PACK_WIN32) ) {
		sPathDup = procXpkDupText(sPath);
		if ( sPathDup == NULL ) {
			procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		return sPathDup;
	}

	sPathDup = procXpkDupPathStoredText(objDst, sPath);
	if ( sPathDup != NULL ) {
		return sPathDup;
	}

	iErr = xpkLastError(objDst);
	if ( iErr == XPK_ERR_MEMORY ) {
		return NULL;
	}
	procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
	return NULL;
}

// 构建初始化种子
static inline int procXpkBuildInitSeed(xpkObject objDst, const xpkEntry* pEntrySrc, xpkEntry* pSeedRet)
{
	memset(pSeedRet, 0, sizeof(*pSeedRet));
	pSeedRet->iFileIndex = pEntrySrc->iFileIndex;
	pSeedRet->iPlatformAttr = pEntrySrc->iPlatformAttr;
	pSeedRet->tCreateTime = pEntrySrc->tCreateTime;
	pSeedRet->tModifyTime = pEntrySrc->tModifyTime;
	pSeedRet->tAccessTime = pEntrySrc->tAccessTime;
	if ( pEntrySrc->pInfoExt != NULL ) {
		pSeedRet->pInfoExt = procXpkDupInfoExt(objDst, pEntrySrc->pInfoExt);
		if ( pSeedRet->pInfoExt == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
	}
	if ( pEntrySrc->sPath != NULL ) {
		pSeedRet->sPath = procXpkBuildDupEntryPath(objDst, pEntrySrc->sPath);
		if ( pSeedRet->sPath == NULL ) {
			if ( pSeedRet->pInfoExt != NULL ) {
				xpkFreeInternal(pSeedRet->pInfoExt);
				pSeedRet->pInfoExt = NULL;
			}
			return xpkLastError(objDst);
		}
	}

	return XPK_OK;
}

// 构建复制条目数据
static inline int procXpkBuildCopyEntryData(xpkObject objDst, const xpkEntry* pEntrySrc, const void* pData, uint64_t iSize)
{
	xpkEntry objSeed;
	xpkEntry* pEntryDst;
	xpkWriteOptions objOpt;
	uint32_t iPosNew;
	int iRet;

	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objDst, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	iRet = procXpkBuildInitSeed(objDst, pEntrySrc, &objSeed);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	memset(&objOpt, 0, sizeof(objOpt));
	objOpt.compLevel = (uint8_t)(pEntrySrc->iFlag & XPK_FLAG_COMP_MASK);
	objOpt.writePolicy = XPK_WRITE_IMMEDIATE;
	objOpt.fileType = (uint8_t)((pEntrySrc->iFlag & XPK_FLAG_TYPE_MASK) >> 4);

	iRet = procXpkAddDataEntry(objDst, &objSeed, pData, iSize, &objOpt, &iPosNew);
	procXpkFreeEntryOwned(&objSeed);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	pEntryDst = procXpkGetEntryByPos(objDst, iPosNew);
	if ( pEntryDst == NULL ) {
		return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	pEntryDst->iFlag = (pEntryDst->iFlag & (XPK_FLAG_COMP_MASK | XPK_FLAG_TYPE_MASK)) |
		(pEntrySrc->iFlag & ~(XPK_FLAG_COMP_MASK | XPK_FLAG_TYPE_MASK | XPK_FLAG_DELETED_MASK));
	pEntryDst->iPlatformAttr = pEntrySrc->iPlatformAttr;
	pEntryDst->tCreateTime = pEntrySrc->tCreateTime;
	pEntryDst->tModifyTime = pEntrySrc->tModifyTime;
	pEntryDst->tAccessTime = pEntrySrc->tAccessTime;
	return XPK_OK;
}

// 构建写入按位置分块
static inline int procXpkBuildWriteAtChunked(xpkObject objDst, xfile hFile, uint64_t iOffset, const void* pData, uint64_t iSize)
{
	const uint8_t* pCur;
	uint64_t iSizeLeft;
	uint32_t iChunkSize;
	int iRet;

	if ( objDst == NULL ) {
		return procXpkReturnParamError(objDst);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objDst, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	pCur = (const uint8_t*)pData;
	iSizeLeft = iSize;
	while ( iSizeLeft > 0 ) {
		iChunkSize = (iSizeLeft > XPK_WRITE_FILE_CHUNK_SIZE) ? XPK_WRITE_FILE_CHUNK_SIZE : (uint32_t)iSizeLeft;
		iRet = procXpkWriteAt(objDst, hFile, iOffset, pCur, iChunkSize);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		pCur += iChunkSize;
		iOffset += iChunkSize;
		iSizeLeft -= iChunkSize;
	}

	return XPK_OK;
}

// 构建清理块文件
static inline int procXpkBuildCleanupChunkFiles(void* pChunk, xfile hFileSrc, xfile hFileDst, int iRet)
{
	if ( pChunk != NULL ) {
		xpkFreeInternal(pChunk);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	return iRet;
}

// 构建复制原样负载
static inline int procXpkBuildCopyStoredPayload(xpkObject objDst, xfile hFileDst, xpkObject objSrc, xfile hFileSrc, uint64_t iOffsetDst, uint64_t iOffsetSrc, uint64_t iSize, void* pChunkShared)
{
	void* pChunk;
	int bFreeChunk;
	uint64_t iSizeLeft;
	uint32_t iChunkSize;
	int iRet;

	pChunk = pChunkShared;
	bFreeChunk = FALSE;
	if ( pChunk == NULL ) {
		pChunk = xpkAllocInternal(XPK_WRITE_FILE_CHUNK_SIZE);
		if ( pChunk == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		bFreeChunk = TRUE;
	}

	iSizeLeft = iSize;
	while ( iSizeLeft > 0 ) {
		iChunkSize = (iSizeLeft > XPK_WRITE_FILE_CHUNK_SIZE) ? XPK_WRITE_FILE_CHUNK_SIZE : (uint32_t)iSizeLeft;
		iRet = procXpkReadAtBuffer(objSrc, hFileSrc, iOffsetSrc, pChunk, iChunkSize);
		if ( iRet != XPK_OK ) {
			if ( bFreeChunk ) {
				xpkFreeInternal(pChunk);
			}
			return iRet;
		}

		iRet = procXpkWriteAt(objDst, hFileDst, iOffsetDst, pChunk, iChunkSize);
		if ( iRet != XPK_OK ) {
			if ( bFreeChunk ) {
				xpkFreeInternal(pChunk);
			}
			return iRet;
		}

		iOffsetDst += iChunkSize;
		iOffsetSrc += iChunkSize;
		iSizeLeft -= iChunkSize;
	}

	if ( bFreeChunk ) {
		xpkFreeInternal(pChunk);
	}
	return XPK_OK;
}

// 从原始数据写入 Solid ZSTD 块
static inline int procXpkBuildWriteSolidZstdFromRaw(xpkObject objDst, const void* pData, uint64_t iRawSize, uint8_t iLevel, uint32_t* pCompSizeRet, uint8_t* pLevelRet)
{
	xfile hFileTmp;
	xfile hFileDst;
	ZSTD_CCtx* pCtx;
	ZSTD_inBuffer objIn;
	ZSTD_outBuffer objOut;
	char* sPathTmp;
	void* pOutBuf;
	const uint8_t* pCur;
	uint64_t iSizeLeft;
	uint64_t iCompSize64;
	uint32_t iChunkSize;
	uint32_t iCompSize;
	size_t iOutCap;
	size_t iZstdRet;
	int bFallbackStore;
	int iRet;

	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = 0;
	}
	if ( pLevelRet != NULL ) {
		*pLevelRet = iLevel;
	}
	if ( objDst == NULL ) {
		return procXpkReturnParamError(objDst);
	}
	if ( pData == NULL && iRawSize > 0 ) {
		return procXpkSetError(objDst, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_ZSTD ) {
		return procXpkSetError(objDst, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iRawSize == 0 ) {
		hFileDst = NULL;
		if ( !procXpkAppliedVolumeMode(objDst) ) {
			hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
			if ( hFileDst == NULL ) {
				return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
			}
		}
		iRet = procXpkWriteAt(objDst, hFileDst, XPK_HEAD_SIZE, NULL, 0);
		if ( hFileDst != NULL ) {
			xrtClose(hFileDst);
		}
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		if ( pLevelRet != NULL ) {
			*pLevelRet = 0;
		}
		procXpkClearError(objDst);
		return XPK_OK;
	}

	hFileTmp = NULL;
	hFileDst = NULL;
	pCtx = NULL;
	sPathTmp = NULL;
	pOutBuf = NULL;
	iCompSize64 = 0;
	bFallbackStore = FALSE;

	sPathTmp = procXpkWriteTempPathDup(objDst, ".solid.zstd");
	if ( sPathTmp == NULL ) {
		return xpkLastError(objDst);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		goto lblCleanup;
	}

	iOutCap = ZSTD_CStreamOutSize();
	if ( iOutCap == 0 ) {
		iOutCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	}
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		iRet = procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	pCtx = ZSTD_createCCtx();
	if ( pCtx == NULL ) {
		iRet = procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	ZSTD_CCtx_reset(pCtx, ZSTD_reset_session_only);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_checksumFlag, 0);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_strategy, (ZSTD_strategy)procXpkCompLevelToNative(iLevel));
	ZSTD_CCtx_setPledgedSrcSize(pCtx, (unsigned long long)iRawSize);

	pCur = (const uint8_t*)pData;
	iSizeLeft = iRawSize;
	while ( iSizeLeft > 0 ) {
		iChunkSize = (uint32_t)((iSizeLeft > XPK_CODEC_STREAM_CHUNK_SIZE) ? XPK_CODEC_STREAM_CHUNK_SIZE : iSizeLeft);
		objIn.src = pCur;
		objIn.size = iChunkSize;
		objIn.pos = 0;

		while ( objIn.pos < objIn.size ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_continue);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objDst, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize64 += objOut.pos;
				if ( iCompSize64 >= iRawSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
		}
		if ( bFallbackStore ) {
			break;
		}

		pCur += iChunkSize;
		iSizeLeft -= iChunkSize;
	}

	if ( !bFallbackStore ) {
		objIn.src = NULL;
		objIn.size = 0;
		objIn.pos = 0;
		for ( ;; ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_end);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objDst, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize64 += objOut.pos;
				if ( iCompSize64 >= iRawSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
			if ( iZstdRet == 0 ) {
				break;
			}
		}
	}

	hFileDst = NULL;
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
			goto lblCleanup;
		}
	}

	if ( bFallbackStore ) {
		iRet = procXpkBuildWriteAtChunked(objDst, hFileDst, XPK_HEAD_SIZE, pData, iRawSize);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}
		if ( pCompSizeRet != NULL ) {
			*pCompSizeRet = (uint32_t)iRawSize;
		}
		if ( pLevelRet != NULL ) {
			*pLevelRet = 0;
		}
		procXpkClearError(objDst);
		iRet = XPK_OK;
		goto lblCleanup;
	}

	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	iCompSize = (uint32_t)iCompSize64;
	iRet = procXpkCopySourceFileToPackage(objDst, hFileTmp, hFileDst, XPK_HEAD_SIZE, iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = iCompSize;
	}
	if ( pLevelRet != NULL ) {
		*pLevelRet = iLevel;
	}
	procXpkClearError(objDst);
	iRet = XPK_OK;

lblCleanup:
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( pCtx != NULL ) {
		ZSTD_freeCCtx(pCtx);
	}
	if ( pOutBuf != NULL ) {
		xpkFreeInternal(pOutBuf);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	return iRet;
}

// 构建写入原样文件分块
static inline int procXpkBuildWritePlainFileChunked(xpkObject objDst, xfile hFile, const void* pData, uint64_t iSize)
{
	const uint8_t* pCur;
	uint64_t iSizeLeft;
	size_t iChunkSize;
	size_t iWrite;

	if ( objDst == NULL || hFile == NULL ) {
		return procXpkReturnParamError(objDst);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objDst, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	pCur = (const uint8_t*)pData;
	iSizeLeft = iSize;
	while ( iSizeLeft > 0 ) {
		iChunkSize = (iSizeLeft > XPK_WRITE_FILE_CHUNK_SIZE) ? XPK_WRITE_FILE_CHUNK_SIZE : (size_t)iSizeLeft;
		iWrite = xrtPut(hFile, (ptr)pCur, iChunkSize);
		if ( iWrite != iChunkSize ) {
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoWrite);
		}
		pCur += iChunkSize;
		iSizeLeft -= iChunkSize;
	}

	return XPK_OK;
}

// 构建阶段 LZMA 顺序输入读取
static inline SRes procXpkBuildLzmaSeqInRead(ISeqInStreamPtr pStream, void* pData, size_t* pSize)
{
	xpkBuildLzmaSeqIn* pIn;
	size_t iWant;
	size_t iRead;

	if ( pStream == NULL || pSize == NULL ) {
		return SZ_ERROR_PARAM;
	}

	pIn = (xpkBuildLzmaSeqIn*)pStream;
	iWant = *pSize;
	if ( iWant == 0 ) {
		return SZ_OK;
	}
	if ( pIn->iRemain < (uint64_t)iWant ) {
		iWant = (size_t)pIn->iRemain;
	}
	if ( iWant == 0 ) {
		*pSize = 0;
		return SZ_OK;
	}

	iRead = xrtGetBuffer(pIn->hFile, pData, iWant);
	*pSize = iRead;
	if ( iRead != iWant ) {
		return SZ_ERROR_READ;
	}

	pIn->iRemain -= iRead;
	return SZ_OK;
}

// 构建阶段 LZMA 顺序输出写入
static inline size_t procXpkBuildLzmaSeqOutWrite(ISeqOutStreamPtr pStream, const void* pData, size_t iSize)
{
	xpkBuildLzmaSeqOut* pOut;
	size_t iWrite;

	if ( pStream == NULL ) {
		return 0;
	}
	if ( iSize == 0 ) {
		return 0;
	}

	pOut = (xpkBuildLzmaSeqOut*)pStream;
	iWrite = xrtPut(pOut->hFile, (ptr)pData, iSize);
	pOut->iSize += iWrite;
	return iWrite;
}

// 从文件写入 Solid ZSTD 块
static inline int procXpkBuildWriteSolidZstdFromFile(xpkObject objDst, const char* sPathSrc, uint64_t iRawSize, uint8_t iLevel, uint32_t* pCompSizeRet, uint8_t* pLevelRet)
{
	xfile hFileSrc;
	xfile hFileTmp;
	xfile hFileDst;
	ZSTD_CCtx* pCtx;
	ZSTD_inBuffer objIn;
	ZSTD_outBuffer objOut;
	char* sPathTmp;
	void* pInBuf;
	void* pOutBuf;
	uint64_t iCompSize64;
	uint64_t iOffsetRead;
	uint32_t iChunkRead;
	uint32_t iCompSize;
	size_t iInCap;
	size_t iOutCap;
	size_t iRead;
	size_t iZstdRet;
	int bFallbackStore;
	int iRet;

	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = 0;
	}
	if ( pLevelRet != NULL ) {
		*pLevelRet = iLevel;
	}
	if ( objDst == NULL ) {
		return procXpkReturnParamError(objDst);
	}
	if ( sPathSrc == NULL || sPathSrc[0] == '\0' ) {
		return procXpkSetError(objDst, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_ZSTD ) {
		return procXpkSetError(objDst, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iRawSize == 0 ) {
		hFileDst = NULL;
		if ( !procXpkAppliedVolumeMode(objDst) ) {
			hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
			if ( hFileDst == NULL ) {
				return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
			}
		}
		iRet = procXpkWriteAt(objDst, hFileDst, XPK_HEAD_SIZE, NULL, 0);
		if ( hFileDst != NULL ) {
			xrtClose(hFileDst);
		}
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		procXpkClearError(objDst);
		return XPK_OK;
	}

	hFileSrc = NULL;
	hFileTmp = NULL;
	hFileDst = NULL;
	pCtx = NULL;
	sPathTmp = NULL;
	pInBuf = NULL;
	pOutBuf = NULL;
	iCompSize64 = 0;
	iOffsetRead = 0;
	bFallbackStore = FALSE;

	hFileSrc = xrtOpen((str)sPathSrc, TRUE, XRT_CP_BINARY);
	if ( hFileSrc == NULL ) {
		return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	sPathTmp = procXpkWriteTempPathDup(objDst, ".solid.zstd");
	if ( sPathTmp == NULL ) {
		iRet = xpkLastError(objDst);
		goto lblCleanup;
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		goto lblCleanup;
	}

	iInCap = ZSTD_CStreamInSize();
	if ( iInCap == 0 ) {
		iInCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	}
	iOutCap = ZSTD_CStreamOutSize();
	if ( iOutCap == 0 ) {
		iOutCap = XPK_CODEC_STREAM_CHUNK_SIZE;
	}

	pInBuf = xpkAllocInternal(iInCap);
	if ( pInBuf == NULL ) {
		iRet = procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}
	pOutBuf = xpkAllocInternal(iOutCap);
	if ( pOutBuf == NULL ) {
		iRet = procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	pCtx = ZSTD_createCCtx();
	if ( pCtx == NULL ) {
		iRet = procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	ZSTD_CCtx_reset(pCtx, ZSTD_reset_session_only);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_checksumFlag, 0);
	ZSTD_CCtx_setParameter(pCtx, ZSTD_c_strategy, (ZSTD_strategy)procXpkCompLevelToNative(iLevel));
	ZSTD_CCtx_setPledgedSrcSize(pCtx, (unsigned long long)iRawSize);

	while ( iOffsetRead < iRawSize ) {
		iChunkRead = (uint32_t)(((iRawSize - iOffsetRead) > (uint64_t)iInCap) ? iInCap : (iRawSize - iOffsetRead));
		iRead = xrtGetBuffer(hFileSrc, pInBuf, iChunkRead);
		if ( iRead != iChunkRead ) {
			iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoRead);
			goto lblCleanup;
		}
		iOffsetRead += iChunkRead;

		objIn.src = pInBuf;
		objIn.size = iChunkRead;
		objIn.pos = 0;
		while ( objIn.pos < objIn.size ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_continue);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objDst, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize64 += objOut.pos;
				if ( iCompSize64 >= iRawSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
		}
		if ( bFallbackStore ) {
			break;
		}
	}

	if ( !bFallbackStore ) {
		objIn.src = NULL;
		objIn.size = 0;
		objIn.pos = 0;
		for ( ;; ) {
			objOut.dst = pOutBuf;
			objOut.size = iOutCap;
			objOut.pos = 0;
			iZstdRet = ZSTD_compressStream2(pCtx, &objOut, &objIn, ZSTD_e_end);
			if ( ZSTD_isError(iZstdRet) ) {
				iRet = procXpkSetError(objDst, XPK_ERR_IO, "zstd compress failed");
				goto lblCleanup;
			}
			if ( objOut.pos > 0 ) {
				if ( xrtPut(hFileTmp, (ptr)pOutBuf, objOut.pos) != objOut.pos ) {
					iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoWrite);
					goto lblCleanup;
				}
				iCompSize64 += objOut.pos;
				if ( iCompSize64 >= iRawSize ) {
					bFallbackStore = TRUE;
					break;
				}
			}
			if ( iZstdRet == 0 ) {
				break;
			}
		}
	}

	hFileDst = NULL;
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
			goto lblCleanup;
		}
	}

	if ( bFallbackStore ) {
		iRet = procXpkSeekFile(objDst, hFileSrc, 0);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}
		iRet = procXpkCopySourceFileToPackage(objDst, hFileSrc, hFileDst, XPK_HEAD_SIZE, iRawSize);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}
		if ( pCompSizeRet != NULL ) {
			*pCompSizeRet = (uint32_t)iRawSize;
		}
		if ( pLevelRet != NULL ) {
			*pLevelRet = 0;
		}
		procXpkClearError(objDst);
		iRet = XPK_OK;
		goto lblCleanup;
	}

	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	iCompSize = (uint32_t)iCompSize64;
	iRet = procXpkCopySourceFileToPackage(objDst, hFileTmp, hFileDst, XPK_HEAD_SIZE, iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = iCompSize;
	}
	if ( pLevelRet != NULL ) {
		*pLevelRet = iLevel;
	}
	procXpkClearError(objDst);
	iRet = XPK_OK;

lblCleanup:
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( pCtx != NULL ) {
		ZSTD_freeCCtx(pCtx);
	}
	if ( pOutBuf != NULL ) {
		xpkFreeInternal(pOutBuf);
	}
	if ( pInBuf != NULL ) {
		xpkFreeInternal(pInBuf);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	return iRet;
}

// 从文件写入 Solid LZMA2 块
static inline int procXpkBuildWriteSolidLzma2FromFile(xpkObject objDst, const char* sPathSrc, uint64_t iRawSize, uint8_t iLevel, uint32_t* pCompSizeRet, uint8_t* pLevelRet)
{
	xfile hFileSrc;
	xfile hFileTmp;
	xfile hFileDst;
	CLzma2EncHandle hLzma2;
	CLzma2EncProps objProps;
	xpkBuildLzmaSeqIn objIn;
	xpkBuildLzmaSeqOut objOut;
	char* sPathTmp;
	Byte iPropByte;
	uint64_t iCompSize64;
	uint32_t iCompSize;
	SRes iLzmaRes;
	int bFallbackStore;
	int iRet;

	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = 0;
	}
	if ( pLevelRet != NULL ) {
		*pLevelRet = iLevel;
	}
	if ( objDst == NULL ) {
		return procXpkReturnParamError(objDst);
	}
	if ( sPathSrc == NULL || sPathSrc[0] == '\0' ) {
		return procXpkSetError(objDst, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkCompLevelToAlg(iLevel) != XPK_ALG_LZMA2 ) {
		return procXpkSetError(objDst, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iRawSize == 0 ) {
		hFileDst = NULL;
		if ( !procXpkAppliedVolumeMode(objDst) ) {
			hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
			if ( hFileDst == NULL ) {
				return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
			}
		}
		iRet = procXpkWriteAt(objDst, hFileDst, XPK_HEAD_SIZE, NULL, 0);
		if ( hFileDst != NULL ) {
			xrtClose(hFileDst);
		}
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		procXpkClearError(objDst);
		return XPK_OK;
	}

	hFileSrc = NULL;
	hFileTmp = NULL;
	hFileDst = NULL;
	hLzma2 = NULL;
	sPathTmp = NULL;
	iCompSize64 = 0;
	bFallbackStore = FALSE;

	hFileSrc = xrtOpen((str)sPathSrc, TRUE, XRT_CP_BINARY);
	if ( hFileSrc == NULL ) {
		return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	sPathTmp = procXpkWriteTempPathDup(objDst, ".solid.lzma2");
	if ( sPathTmp == NULL ) {
		iRet = xpkLastError(objDst);
		goto lblCleanup;
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		goto lblCleanup;
	}

	hLzma2 = Lzma2Enc_Create(&g_Alloc, &g_BigAlloc);
	if ( hLzma2 == NULL ) {
		iRet = procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		goto lblCleanup;
	}

	Lzma2EncProps_Init(&objProps);
	objProps.lzmaProps.level = procXpkCompLevelToNative(iLevel);
	iLzmaRes = Lzma2Enc_SetProps(hLzma2, &objProps);
	if ( iLzmaRes != SZ_OK ) {
		iRet = procXpkSetError(objDst, XPK_ERR_IO, "lzma2 set props failed");
		goto lblCleanup;
	}
	Lzma2Enc_SetDataSize(hLzma2, (UInt64)iRawSize);

	iPropByte = Lzma2Enc_WriteProperties(hLzma2);
	if ( xrtPut(hFileTmp, (ptr)&iPropByte, 1) != 1 ) {
		iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	objIn.vt.Read = procXpkBuildLzmaSeqInRead;
	objIn.hFile = hFileSrc;
	objIn.iRemain = iRawSize;
	objOut.vt.Write = procXpkBuildLzmaSeqOutWrite;
	objOut.hFile = hFileTmp;
	objOut.iSize = 0;

	iLzmaRes = Lzma2Enc_Encode2(hLzma2, &objOut.vt, NULL, NULL, &objIn.vt, NULL, 0, NULL);
	if ( iLzmaRes != SZ_OK ) {
		if ( iLzmaRes == SZ_ERROR_READ ) {
			iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoRead);
		} else if ( iLzmaRes == SZ_ERROR_WRITE ) {
			iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoWrite);
		} else {
			iRet = procXpkSetError(objDst, XPK_ERR_IO, "lzma2 compress failed");
		}
		goto lblCleanup;
	}

	iCompSize64 = 1 + objOut.iSize;
	if ( iCompSize64 >= iRawSize ) {
		bFallbackStore = TRUE;
	}

	hFileDst = NULL;
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
			goto lblCleanup;
		}
	}

	if ( bFallbackStore ) {
		iRet = procXpkSeekFile(objDst, hFileSrc, 0);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}
		iRet = procXpkCopySourceFileToPackage(objDst, hFileSrc, hFileDst, XPK_HEAD_SIZE, iRawSize);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}
		if ( pCompSizeRet != NULL ) {
			*pCompSizeRet = (uint32_t)iRawSize;
		}
		if ( pLevelRet != NULL ) {
			*pLevelRet = 0;
		}
		procXpkClearError(objDst);
		iRet = XPK_OK;
		goto lblCleanup;
	}

	if ( !xrtSetEOF(hFileTmp) ) {
		iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoWrite);
		goto lblCleanup;
	}

	iCompSize = (uint32_t)iCompSize64;
	iRet = procXpkCopySourceFileToPackage(objDst, hFileTmp, hFileDst, XPK_HEAD_SIZE, iCompSize);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = iCompSize;
	}
	if ( pLevelRet != NULL ) {
		*pLevelRet = iLevel;
	}
	procXpkClearError(objDst);
	iRet = XPK_OK;

lblCleanup:
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( hFileTmp != NULL ) {
		xrtClose(hFileTmp);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hLzma2 != NULL ) {
		Lzma2Enc_Destroy(hLzma2);
	}
	if ( sPathTmp != NULL ) {
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
	}
	return iRet;
}

// 从文件写入 Solid LZ4 块
static inline int procXpkBuildWriteSolidLz4FromFile(xpkObject objDst, const char* sPathSrc, uint64_t iRawSize, uint8_t iLevel, uint32_t* pCompSizeRet, uint8_t* pLevelRet)
{
	xfile hFileSrc;
	xfile hFileTmp;
	xfile hFileDst;
	xpkMappedFile objMapSrc;
	xpkMappedFile objMapTmp;
	char* sPathTmp;
	uint32_t iBound;
	uint32_t iCompSize;
	uint32_t iAlg;
	int iRetEnc;
	int iRet;

	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = 0;
	}
	if ( pLevelRet != NULL ) {
		*pLevelRet = iLevel;
	}
	if ( objDst == NULL ) {
		return procXpkReturnParamError(objDst);
	}
	if ( sPathSrc == NULL || sPathSrc[0] == '\0' ) {
		return procXpkSetError(objDst, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	iAlg = procXpkCompLevelToAlg(iLevel);
	if ( iAlg != XPK_ALG_LZ4 && iAlg != XPK_ALG_LZ4HC ) {
		return procXpkSetError(objDst, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iRawSize == 0 ) {
		hFileDst = NULL;
		if ( !procXpkAppliedVolumeMode(objDst) ) {
			hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
			if ( hFileDst == NULL ) {
				return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
			}
		}
		iRet = procXpkWriteAt(objDst, hFileDst, XPK_HEAD_SIZE, NULL, 0);
		if ( hFileDst != NULL ) {
			xrtClose(hFileDst);
		}
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		procXpkClearError(objDst);
		return XPK_OK;
	}

	hFileTmp = NULL;
	hFileSrc = xrtOpen((str)sPathSrc, TRUE, XRT_CP_BINARY);
	if ( hFileSrc == NULL ) {
		return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	memset(&objMapSrc, 0, sizeof(objMapSrc));
	memset(&objMapTmp, 0, sizeof(objMapTmp));
	sPathTmp = NULL;
	iRet = procXpkMapFileReadOnly(objDst, hFileSrc, iRawSize, &objMapSrc);
	xrtClose(hFileSrc);
	hFileSrc = NULL;
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iCompSize = 0;
	iRet = procXpkCodecBound(objDst, iLevel, (uint32_t)iRawSize, &iBound);
	if ( iRet != XPK_OK ) {
		procXpkUnmapFile(&objMapSrc);
		return iRet;
	}

	sPathTmp = procXpkWriteTempPathDup(objDst, ".solid.lz4");
	if ( sPathTmp == NULL ) {
		procXpkUnmapFile(&objMapSrc);
		return xpkLastError(objDst);
	}

	hFileTmp = xrtOpen((str)sPathTmp, FALSE, XRT_CP_BINARY);
	if ( hFileTmp == NULL ) {
		procXpkUnmapFile(&objMapSrc);
		xpkFreeInternal(sPathTmp);
		return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	iRet = procXpkMapFileReadWrite(objDst, hFileTmp, iBound, &objMapTmp);
	if ( iRet != XPK_OK ) {
		procXpkUnmapFile(&objMapSrc);
		xrtClose(hFileTmp);
		xpkFreeInternal(sPathTmp);
		return iRet;
	}

	if ( iAlg == XPK_ALG_LZ4 ) {
		iRetEnc = LZ4_compress_fast((const char*)objMapSrc.pView, (char*)objMapTmp.pView, (int)iRawSize, (int)iBound, procXpkCompLevelToNative(iLevel));
	} else {
		iRetEnc = LZ4_compress_HC((const char*)objMapSrc.pView, (char*)objMapTmp.pView, (int)iRawSize, (int)iBound, procXpkCompLevelToNative(iLevel));
	}
	procXpkUnmapFile(&objMapSrc);
	if ( iRetEnc <= 0 ) {
		procXpkUnmapFile(&objMapTmp);
		xrtClose(hFileTmp);
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
		return procXpkSetError(objDst, XPK_ERR_IO, (iAlg == XPK_ALG_LZ4) ? "lz4 compress failed" : "lz4hc compress failed");
	}

	iCompSize = (uint32_t)iRetEnc;
	procXpkUnmapFile(&objMapTmp);
	hFileDst = NULL;
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			xrtClose(hFileTmp);
			if ( xrtFileExists((str)sPathTmp) ) {
				(void)xrtFileDelete((str)sPathTmp);
			}
			xpkFreeInternal(sPathTmp);
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}

	if ( iCompSize >= iRawSize ) {
		hFileSrc = xrtOpen((str)sPathSrc, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			xrtClose(hFileDst);
			xrtClose(hFileTmp);
			if ( xrtFileExists((str)sPathTmp) ) {
				(void)xrtFileDelete((str)sPathTmp);
			}
			xpkFreeInternal(sPathTmp);
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		iRet = procXpkCopySourceFileToPackage(objDst, hFileSrc, hFileDst, XPK_HEAD_SIZE, iRawSize);
		xrtClose(hFileSrc);
		hFileSrc = NULL;
		xrtClose(hFileDst);
		xrtClose(hFileTmp);
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		if ( pCompSizeRet != NULL ) {
			*pCompSizeRet = (uint32_t)iRawSize;
		}
		if ( pLevelRet != NULL ) {
			*pLevelRet = 0;
		}
		procXpkClearError(objDst);
		return XPK_OK;
	}

	iRet = procXpkSeekFile(objDst, hFileTmp, iCompSize);
	if ( iRet != XPK_OK ) {
		xrtClose(hFileDst);
		xrtClose(hFileTmp);
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
		return iRet;
	}
	if ( !xrtSetEOF(hFileTmp) ) {
		xrtClose(hFileDst);
		xrtClose(hFileTmp);
		if ( xrtFileExists((str)sPathTmp) ) {
			(void)xrtFileDelete((str)sPathTmp);
		}
		xpkFreeInternal(sPathTmp);
		return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	iRet = procXpkCopySourceFileToPackage(objDst, hFileTmp, hFileDst, XPK_HEAD_SIZE, iCompSize);
	xrtClose(hFileDst);
	xrtClose(hFileTmp);
	if ( xrtFileExists((str)sPathTmp) ) {
		(void)xrtFileDelete((str)sPathTmp);
	}
	xpkFreeInternal(sPathTmp);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = iCompSize;
	}
	if ( pLevelRet != NULL ) {
		*pLevelRet = iLevel;
	}
	procXpkClearError(objDst);
	return XPK_OK;
}

// 流式复制有效条目到 Solid ZSTD 目标
static inline int procXpkBuildCopyLiveEntriesSolidZstdStreaming(xpkObject objDst, xpkObject objSrc, uint64_t iSolidRawSize64)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	xpkEntry* pEntryDst;
	xpkWriteNode* pNode;
	xfile hFileSrc;
	xfile hFileRaw;
	void* pData;
	char* sPathRaw;
	uint64_t iDataSize;
	uint64_t iOffsetSolid;
	uint32_t iSolidCompSize;
	uint8_t iSolidLevel;
	uint8_t iSolidLevelUsed;
	int iRet;

	hFileSrc = NULL;
	hFileRaw = NULL;
	pData = NULL;
	sPathRaw = NULL;
	iOffsetSolid = 0;
	iSolidCompSize = 0;
	iSolidLevel = procXpkSolidTargetCompLevel(objDst);
	iSolidLevelUsed = iSolidLevel;
	pNode = NULL;

	sPathRaw = procXpkWriteTempPathDup(objDst, ".solid.raw");
	if ( sPathRaw == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return xpkLastError(objDst);
	}

	hFileRaw = xrtOpen((str)sPathRaw, FALSE, XRT_CP_BINARY);
	if ( hFileRaw == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xpkFreeInternal(sPathRaw);
		return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
			goto lblCleanup;
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			iRet = procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
			goto lblCleanup;
		}
		pNode = procXpkFindWriteNode(objSrc, pEntry->iPos, NULL);
		if ( pNode == NULL && !procXpkAppliedVolumeMode(objSrc) && hFileSrc == NULL ) {
			hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
			if ( hFileSrc == NULL ) {
				iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
				goto lblCleanup;
			}
		}

		pData = procXpkReadEntryDataWithFile(objSrc, pEntry, &iDataSize, hFileSrc);
		if ( pData == NULL ) {
			iRet = procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
			goto lblCleanup;
		}
		if ( iDataSize != pEntry->iFileSize ) {
			iRet = procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			goto lblCleanup;
		}

		iRet = procXpkBuildWritePlainFileChunked(objDst, hFileRaw, pData, iDataSize);
		xpkFree(pData);
		pData = NULL;
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}

		iRet = procXpkBuildAppendEntryOnly(objDst, pEntry, iOffsetSolid, pEntry->iFileSize, pEntry->iFileSize);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}
		iOffsetSolid += pEntry->iFileSize;
	}

	if ( hFileRaw != NULL ) {
		xrtClose(hFileRaw);
		hFileRaw = NULL;
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
		hFileSrc = NULL;
	}

	iRet = procXpkRebuildLookup(objDst);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iRet = procXpkBuildWriteSolidZstdFromFile(objDst, sPathRaw, iSolidRawSize64, iSolidLevel, &iSolidCompSize, &iSolidLevelUsed);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	objDst->objHead.defComp = iSolidLevelUsed;
	for ( iPos = 1; iPos <= objDst->iEntryCount; iPos++ ) {
		pEntryDst = (xpkEntry*)xrtArrayGet(&objDst->arrEntry, iPos);
		if ( pEntryDst == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
			goto lblCleanup;
		}
		pEntryDst->iFlag = (pEntryDst->iFlag & ~XPK_FLAG_COMP_MASK) | iSolidLevelUsed;
	}
	objDst->bSolidApplied = TRUE;
	objDst->iAppendPos = XPK_HEAD_SIZE + ((iSolidLevelUsed == 0) ? iSolidRawSize64 : iSolidCompSize);
	objDst->iFileSize = objDst->iAppendPos;
	objDst->bDirtyData = TRUE;
	objDst->bDirtyEntryTable = TRUE;
	objDst->bDirtyHead = TRUE;
	procXpkClearError(objDst);
	iRet = XPK_OK;

lblCleanup:
	if ( pData != NULL ) {
		xpkFree(pData);
	}
	if ( hFileRaw != NULL ) {
		xrtClose(hFileRaw);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( sPathRaw != NULL ) {
		if ( xrtFileExists((str)sPathRaw) ) {
			(void)xrtFileDelete((str)sPathRaw);
		}
		xpkFreeInternal(sPathRaw);
	}
	return iRet;
}

// 流式复制有效条目到 Solid LZMA2 目标
static inline int procXpkBuildCopyLiveEntriesSolidLzma2Streaming(xpkObject objDst, xpkObject objSrc, uint64_t iSolidRawSize64)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	xpkEntry* pEntryDst;
	xpkWriteNode* pNode;
	xfile hFileSrc;
	xfile hFileRaw;
	void* pData;
	char* sPathRaw;
	uint64_t iDataSize;
	uint64_t iOffsetSolid;
	uint32_t iSolidCompSize;
	uint8_t iSolidLevel;
	uint8_t iSolidLevelUsed;
	int iRet;

	hFileSrc = NULL;
	hFileRaw = NULL;
	pData = NULL;
	sPathRaw = NULL;
	iOffsetSolid = 0;
	iSolidCompSize = 0;
	iSolidLevel = procXpkSolidTargetCompLevel(objDst);
	iSolidLevelUsed = iSolidLevel;
	pNode = NULL;

	sPathRaw = procXpkWriteTempPathDup(objDst, ".solid.raw");
	if ( sPathRaw == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return xpkLastError(objDst);
	}

	hFileRaw = xrtOpen((str)sPathRaw, FALSE, XRT_CP_BINARY);
	if ( hFileRaw == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xpkFreeInternal(sPathRaw);
		return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
			goto lblCleanup;
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			iRet = procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
			goto lblCleanup;
		}
		pNode = procXpkFindWriteNode(objSrc, pEntry->iPos, NULL);
		if ( pNode == NULL && !procXpkAppliedVolumeMode(objSrc) && hFileSrc == NULL ) {
			hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
			if ( hFileSrc == NULL ) {
				iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
				goto lblCleanup;
			}
		}

		pData = procXpkReadEntryDataWithFile(objSrc, pEntry, &iDataSize, hFileSrc);
		if ( pData == NULL ) {
			iRet = procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
			goto lblCleanup;
		}
		if ( iDataSize != pEntry->iFileSize ) {
			iRet = procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			goto lblCleanup;
		}

		iRet = procXpkBuildWritePlainFileChunked(objDst, hFileRaw, pData, iDataSize);
		xpkFree(pData);
		pData = NULL;
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}

		iRet = procXpkBuildAppendEntryOnly(objDst, pEntry, iOffsetSolid, pEntry->iFileSize, pEntry->iFileSize);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}
		iOffsetSolid += pEntry->iFileSize;
	}

	if ( hFileRaw != NULL ) {
		xrtClose(hFileRaw);
		hFileRaw = NULL;
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
		hFileSrc = NULL;
	}

	iRet = procXpkRebuildLookup(objDst);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iRet = procXpkBuildWriteSolidLzma2FromFile(objDst, sPathRaw, iSolidRawSize64, iSolidLevel, &iSolidCompSize, &iSolidLevelUsed);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	objDst->objHead.defComp = iSolidLevelUsed;
	for ( iPos = 1; iPos <= objDst->iEntryCount; iPos++ ) {
		pEntryDst = (xpkEntry*)xrtArrayGet(&objDst->arrEntry, iPos);
		if ( pEntryDst == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
			goto lblCleanup;
		}
		pEntryDst->iFlag = (pEntryDst->iFlag & ~XPK_FLAG_COMP_MASK) | iSolidLevelUsed;
	}
	objDst->bSolidApplied = TRUE;
	objDst->iAppendPos = XPK_HEAD_SIZE + ((iSolidLevelUsed == 0) ? iSolidRawSize64 : iSolidCompSize);
	objDst->iFileSize = objDst->iAppendPos;
	objDst->bDirtyData = TRUE;
	objDst->bDirtyEntryTable = TRUE;
	objDst->bDirtyHead = TRUE;
	procXpkClearError(objDst);
	iRet = XPK_OK;

lblCleanup:
	if ( pData != NULL ) {
		xpkFree(pData);
	}
	if ( hFileRaw != NULL ) {
		xrtClose(hFileRaw);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( sPathRaw != NULL ) {
		if ( xrtFileExists((str)sPathRaw) ) {
			(void)xrtFileDelete((str)sPathRaw);
		}
		xpkFreeInternal(sPathRaw);
	}
	return iRet;
}

// 流式复制有效条目到 Solid LZ4 目标
static inline int procXpkBuildCopyLiveEntriesSolidLz4Streaming(xpkObject objDst, xpkObject objSrc, uint64_t iSolidRawSize64)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	xpkEntry* pEntryDst;
	xpkWriteNode* pNode;
	xfile hFileSrc;
	xfile hFileRaw;
	void* pData;
	char* sPathRaw;
	uint64_t iDataSize;
	uint64_t iOffsetSolid;
	uint32_t iSolidCompSize;
	uint8_t iSolidLevel;
	uint8_t iSolidLevelUsed;
	int iRet;

	hFileSrc = NULL;
	hFileRaw = NULL;
	pData = NULL;
	sPathRaw = NULL;
	iOffsetSolid = 0;
	iSolidCompSize = 0;
	iSolidLevel = procXpkSolidTargetCompLevel(objDst);
	iSolidLevelUsed = iSolidLevel;
	pNode = NULL;

	sPathRaw = procXpkWriteTempPathDup(objDst, ".solid.raw");
	if ( sPathRaw == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return xpkLastError(objDst);
	}

	hFileRaw = xrtOpen((str)sPathRaw, FALSE, XRT_CP_BINARY);
	if ( hFileRaw == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xpkFreeInternal(sPathRaw);
		return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
			goto lblCleanup;
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			iRet = procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
			goto lblCleanup;
		}
		pNode = procXpkFindWriteNode(objSrc, pEntry->iPos, NULL);
		if ( pNode == NULL && !procXpkAppliedVolumeMode(objSrc) && hFileSrc == NULL ) {
			hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
			if ( hFileSrc == NULL ) {
				iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
				goto lblCleanup;
			}
		}

		pData = procXpkReadEntryDataWithFile(objSrc, pEntry, &iDataSize, hFileSrc);
		if ( pData == NULL ) {
			iRet = procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
			goto lblCleanup;
		}
		if ( iDataSize != pEntry->iFileSize ) {
			iRet = procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			goto lblCleanup;
		}

		iRet = procXpkBuildWritePlainFileChunked(objDst, hFileRaw, pData, iDataSize);
		xpkFree(pData);
		pData = NULL;
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}

		iRet = procXpkBuildAppendEntryOnly(objDst, pEntry, iOffsetSolid, pEntry->iFileSize, pEntry->iFileSize);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}
		iOffsetSolid += pEntry->iFileSize;
	}

	if ( hFileRaw != NULL ) {
		xrtClose(hFileRaw);
		hFileRaw = NULL;
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
		hFileSrc = NULL;
	}

	iRet = procXpkRebuildLookup(objDst);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	iRet = procXpkBuildWriteSolidLz4FromFile(objDst, sPathRaw, iSolidRawSize64, iSolidLevel, &iSolidCompSize, &iSolidLevelUsed);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	objDst->objHead.defComp = iSolidLevelUsed;
	for ( iPos = 1; iPos <= objDst->iEntryCount; iPos++ ) {
		pEntryDst = (xpkEntry*)xrtArrayGet(&objDst->arrEntry, iPos);
		if ( pEntryDst == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
			goto lblCleanup;
		}
		pEntryDst->iFlag = (pEntryDst->iFlag & ~XPK_FLAG_COMP_MASK) | iSolidLevelUsed;
	}
	objDst->bSolidApplied = TRUE;
	objDst->iAppendPos = XPK_HEAD_SIZE + ((iSolidLevelUsed == 0) ? iSolidRawSize64 : iSolidCompSize);
	objDst->iFileSize = objDst->iAppendPos;
	objDst->bDirtyData = TRUE;
	objDst->bDirtyEntryTable = TRUE;
	objDst->bDirtyHead = TRUE;
	procXpkClearError(objDst);
	iRet = XPK_OK;

lblCleanup:
	if ( pData != NULL ) {
		xpkFree(pData);
	}
	if ( hFileRaw != NULL ) {
		xrtClose(hFileRaw);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( sPathRaw != NULL ) {
		if ( xrtFileExists((str)sPathRaw) ) {
			(void)xrtFileDelete((str)sPathRaw);
		}
		xpkFreeInternal(sPathRaw);
	}
	return iRet;
}

// 流式复制有效条目到 Solid 原样目标
static inline int procXpkBuildCopyLiveEntriesSolidStoreStreaming(xpkObject objDst, xpkObject objSrc, uint64_t iSolidRawSize64)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	xpkEntry* pEntryDst;
	xpkWriteNode* pNode;
	xfile hFileSrc;
	xfile hFileRaw;
	xfile hFileRawSrc;
	xfile hFileDst;
	void* pData;
	char* sPathRaw;
	uint64_t iDataSize;
	uint64_t iOffsetSolid;
	int iRet;

	hFileSrc = NULL;
	hFileRaw = NULL;
	hFileRawSrc = NULL;
	hFileDst = NULL;
	pData = NULL;
	sPathRaw = NULL;
	iOffsetSolid = 0;
	pNode = NULL;

	sPathRaw = procXpkWriteTempPathDup(objDst, ".solid.raw");
	if ( sPathRaw == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		return xpkLastError(objDst);
	}

	hFileRaw = xrtOpen((str)sPathRaw, FALSE, XRT_CP_BINARY);
	if ( hFileRaw == NULL ) {
		if ( hFileSrc != NULL ) {
			xrtClose(hFileSrc);
		}
		xpkFreeInternal(sPathRaw);
		return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
			goto lblCleanup;
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			iRet = procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
			goto lblCleanup;
		}
		pNode = procXpkFindWriteNode(objSrc, pEntry->iPos, NULL);
		if ( pNode == NULL && !procXpkAppliedVolumeMode(objSrc) && hFileSrc == NULL ) {
			hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
			if ( hFileSrc == NULL ) {
				iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
				goto lblCleanup;
			}
		}

		pData = procXpkReadEntryDataWithFile(objSrc, pEntry, &iDataSize, hFileSrc);
		if ( pData == NULL ) {
			iRet = procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
			goto lblCleanup;
		}
		if ( iDataSize != pEntry->iFileSize ) {
			iRet = procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			goto lblCleanup;
		}

		iRet = procXpkBuildWritePlainFileChunked(objDst, hFileRaw, pData, iDataSize);
		xpkFree(pData);
		pData = NULL;
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}

		iRet = procXpkBuildAppendEntryOnly(objDst, pEntry, iOffsetSolid, pEntry->iFileSize, pEntry->iFileSize);
		if ( iRet != XPK_OK ) {
			goto lblCleanup;
		}
		iOffsetSolid += pEntry->iFileSize;
	}

	if ( hFileRaw != NULL ) {
		xrtClose(hFileRaw);
		hFileRaw = NULL;
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
		hFileSrc = NULL;
	}

	iRet = procXpkRebuildLookup(objDst);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	hFileRawSrc = xrtOpen((str)sPathRaw, TRUE, XRT_CP_BINARY);
	if ( hFileRawSrc == NULL ) {
		iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		goto lblCleanup;
	}
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
			goto lblCleanup;
		}
	}

	iRet = procXpkCopySourceFileToPackage(objDst, hFileRawSrc, hFileDst, XPK_HEAD_SIZE, iSolidRawSize64);
	if ( iRet != XPK_OK ) {
		goto lblCleanup;
	}

	objDst->objHead.defComp = 0;
	for ( iPos = 1; iPos <= objDst->iEntryCount; iPos++ ) {
		pEntryDst = (xpkEntry*)xrtArrayGet(&objDst->arrEntry, iPos);
		if ( pEntryDst == NULL ) {
			iRet = procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
			goto lblCleanup;
		}
		pEntryDst->iFlag &= ~XPK_FLAG_COMP_MASK;
	}
	objDst->bSolidApplied = TRUE;
	objDst->iAppendPos = XPK_HEAD_SIZE + iSolidRawSize64;
	objDst->iFileSize = objDst->iAppendPos;
	objDst->bDirtyData = TRUE;
	objDst->bDirtyEntryTable = TRUE;
	objDst->bDirtyHead = TRUE;
	procXpkClearError(objDst);
	iRet = XPK_OK;

lblCleanup:
	if ( pData != NULL ) {
		xpkFree(pData);
	}
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( hFileRawSrc != NULL ) {
		xrtClose(hFileRawSrc);
	}
	if ( hFileRaw != NULL ) {
		xrtClose(hFileRaw);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( sPathRaw != NULL ) {
		if ( xrtFileExists((str)sPathRaw) ) {
			(void)xrtFileDelete((str)sPathRaw);
		}
		xpkFreeInternal(sPathRaw);
	}
	return iRet;
}

// 构建追加已复制条目
static inline int procXpkBuildAppendCopiedEntry(xpkObject objDst, const xpkEntry* pEntrySrc, uint8_t iCompLevel, uint64_t iDataOffset, uint64_t iDataSize, uint64_t iFileSize)
{
	xpkEntry objSeed;
	int iRet;

	iRet = procXpkBuildInitSeed(objDst, pEntrySrc, &objSeed);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	objSeed.iFlag = (pEntrySrc->iFlag & ~XPK_FLAG_DELETED_MASK);
	objSeed.iFlag = (objSeed.iFlag & ~XPK_FLAG_COMP_MASK) | (uint32_t)(iCompLevel & XPK_FLAG_COMP_MASK);
	objSeed.iFileHash = pEntrySrc->iFileHash;
	objSeed.iDataOffset = iDataOffset;
	objSeed.iDataSize = iDataSize;
	objSeed.iFileSize = iFileSize;
	objSeed.bStored = TRUE;

	iRet = procXpkAppendEntryOwned(objDst, &objSeed);
	procXpkFreeEntryOwned(&objSeed);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	return XPK_OK;
}

// 构建复制条目直通带文件
static inline int procXpkBuildCopyEntryDirectWithFiles(xpkObject objDst, xpkObject objSrc, const xpkEntry* pEntrySrc, xfile hFileDst, xfile hFileSrc, void* pChunkShared)
{
	xpkWriteNode* pNode;
	xfile hFileSrcLocal;
	xfile hFileDstLocal;
	uint64_t iOffsetDst;
	uint64_t iDataEndSrc;
	uint64_t iCompSize;
	uint8_t iCompLevel;
	int iRet;

	if ( objDst == NULL || objSrc == NULL || pEntrySrc == NULL ) {
		return procXpkReturnParamError(objDst);
	}

	hFileSrcLocal = NULL;
	hFileDstLocal = NULL;
	iOffsetDst = objDst->iAppendPos;
	iDataEndSrc = procXpkCurrentDataEnd(objSrc);
	pNode = procXpkFindWriteNode(objSrc, pEntrySrc->iPos, NULL);
	if ( !procXpkAppliedVolumeMode(objDst) && hFileDst == NULL ) {
		hFileDstLocal = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDstLocal == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		hFileDst = hFileDstLocal;
	}

	if ( pNode != NULL ) {
		if ( pNode->iRawSize != pEntrySrc->iFileSize ) {
			if ( hFileDstLocal != NULL ) {
				xrtClose(hFileDstLocal);
			}
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		if ( pNode->pCompData == NULL && pNode->iCompSize > 0 ) {
			if ( hFileDstLocal != NULL ) {
				xrtClose(hFileDstLocal);
			}
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}

		iCompLevel = pNode->iLevel;
		iCompSize = pNode->iCompSize;
		iRet = procXpkBuildWriteAtChunked(objDst, hFileDst, iOffsetDst, pNode->pCompData, iCompSize);
	} else {
		if ( pEntrySrc->iDataOffset < XPK_HEAD_SIZE ) {
			if ( hFileDstLocal != NULL ) {
				xrtClose(hFileDstLocal);
			}
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		if ( pEntrySrc->iDataOffset > iDataEndSrc ) {
			if ( hFileDstLocal != NULL ) {
				xrtClose(hFileDstLocal);
			}
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		if ( pEntrySrc->iDataSize > (iDataEndSrc - pEntrySrc->iDataOffset) ) {
			if ( hFileDstLocal != NULL ) {
				xrtClose(hFileDstLocal);
			}
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}

		if ( !procXpkAppliedVolumeMode(objSrc) && hFileSrc == NULL ) {
			hFileSrcLocal = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
			if ( hFileSrcLocal == NULL ) {
				if ( hFileDstLocal != NULL ) {
					xrtClose(hFileDstLocal);
				}
				return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
			}
			hFileSrc = hFileSrcLocal;
		}

		iCompLevel = (uint8_t)(pEntrySrc->iFlag & XPK_FLAG_COMP_MASK);
		iCompSize = pEntrySrc->iDataSize;
		iRet = procXpkBuildCopyStoredPayload(objDst, hFileDst, objSrc, hFileSrc, iOffsetDst, pEntrySrc->iDataOffset, iCompSize, pChunkShared);
	}

	if ( hFileSrcLocal != NULL ) {
		xrtClose(hFileSrcLocal);
	}
	if ( hFileDstLocal != NULL ) {
		xrtClose(hFileDstLocal);
	}
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = procXpkBuildAppendCopiedEntry(objDst, pEntrySrc, iCompLevel, iOffsetDst, iCompSize, pEntrySrc->iFileSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	objDst->iAppendPos = iOffsetDst + iCompSize;
	if ( objDst->iAppendPos > objDst->iFileSize ) {
		objDst->iFileSize = objDst->iAppendPos;
	}
	objDst->bDirtyData = TRUE;
	procXpkMarkDirtyEntryTable(objDst);
	procXpkClearError(objDst);
	return XPK_OK;
}

// 构建复制条目直通
static inline int procXpkBuildCopyEntryDirect(xpkObject objDst, xpkObject objSrc, const xpkEntry* pEntrySrc)
{
	return procXpkBuildCopyEntryDirectWithFiles(objDst, objSrc, pEntrySrc, NULL, NULL, NULL);
}

// 构建复制条目
static inline int procXpkBuildCopyEntry(xpkObject objDst, xpkObject objSrc, const xpkEntry* pEntrySrc)
{
	void* pData;
	uint64_t iSize;
	int iRet;

	if ( !objDst->objHead.solidMode && !objSrc->bSolidApplied ) {
		return procXpkBuildCopyEntryDirect(objDst, objSrc, pEntrySrc);
	}

	pData = procXpkReadEntryData(objSrc, (xpkEntry*)pEntrySrc, &iSize);
	if ( pData == NULL ) {
		return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
	}

	iRet = procXpkBuildCopyEntryData(objDst, pEntrySrc, pData, iSize);
	xpkFree(pData);
	return iRet;
}

// 构建追加条目仅
static inline int procXpkBuildAppendEntryOnly(xpkObject objDst, const xpkEntry* pEntrySrc, uint64_t iDataOffset, uint64_t iDataSize, uint64_t iFileSize)
{
	xpkEntry objEntry;
	xpkEntry* pEntryDst;
	int iRet;

	memset(&objEntry, 0, sizeof(objEntry));
	objEntry.iFlag = pEntrySrc->iFlag;
	objEntry.iFileHash = pEntrySrc->iFileHash;
	objEntry.iDataOffset = iDataOffset;
	objEntry.iDataSize = iDataSize;
	objEntry.iFileSize = iFileSize;
	objEntry.bStored = TRUE;
	objEntry.iFileIndex = pEntrySrc->iFileIndex;
	objEntry.iPlatformAttr = pEntrySrc->iPlatformAttr;
	objEntry.tCreateTime = pEntrySrc->tCreateTime;
	objEntry.tModifyTime = pEntrySrc->tModifyTime;
	objEntry.tAccessTime = pEntrySrc->tAccessTime;
	if ( pEntrySrc->pInfoExt != NULL ) {
		objEntry.pInfoExt = procXpkDupInfoExt(objDst, pEntrySrc->pInfoExt);
		if ( objEntry.pInfoExt == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
	}
	if ( pEntrySrc->sPath != NULL ) {
		objEntry.sPath = procXpkBuildDupEntryPath(objDst, pEntrySrc->sPath);
		if ( objEntry.sPath == NULL ) {
			if ( objEntry.pInfoExt != NULL ) {
				xpkFreeInternal(objEntry.pInfoExt);
			}
			return xpkLastError(objDst);
		}
	}

	objEntry.iFlag = (objEntry.iFlag & ~XPK_FLAG_COMP_MASK) | procXpkSolidTargetCompLevel(objDst);
	iRet = procXpkAppendEntryOwned(objDst, &objEntry);
	procXpkFreeEntryOwned(&objEntry);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	pEntryDst = procXpkGetEntryByPos(objDst, objDst->iEntryCount);
	if ( pEntryDst == NULL ) {
		return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	pEntryDst->bStored = TRUE;
	return XPK_OK;
}

// 检查 Solid 原样直拷条件
static inline int procXpkBuildCheckSolidStoreDirect(xpkObject objDst, xpkObject objSrc, int* pCanCopyRet)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	xpkWriteNode* pNode;

	if ( pCanCopyRet != NULL ) {
		*pCanCopyRet = FALSE;
	}
	if ( objDst == NULL || objSrc == NULL || pCanCopyRet == NULL ) {
		return procXpkReturnParamError(objDst);
	}
	if ( objSrc->bSolidApplied || !objDst->objHead.solidMode ) {
		return XPK_OK;
	}
	if ( procXpkSolidTargetCompLevel(objDst) != 0 ) {
		return XPK_OK;
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		if ( procXpkValidateLiveEntryLookup(objSrc, pEntry) != XPK_OK ) {
			return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
		}

		pNode = procXpkFindWriteNode(objSrc, pEntry->iPos, NULL);
		if ( pNode != NULL ) {
			if ( pNode->iLevel != 0 ) {
				return XPK_OK;
			}
			if ( pNode->iCompSize != pNode->iRawSize || pNode->iRawSize != pEntry->iFileSize ) {
				return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
			if ( pNode->pCompData == NULL && pNode->iCompSize > 0 ) {
				return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
			continue;
		}

		if ( (pEntry->iFlag & XPK_FLAG_COMP_MASK) != 0 ) {
			return XPK_OK;
		}
		if ( pEntry->iDataSize != pEntry->iFileSize ) {
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		if ( pEntry->iDataOffset < XPK_HEAD_SIZE ) {
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		if ( pEntry->iDataOffset > procXpkCurrentDataEnd(objSrc) ) {
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		if ( pEntry->iDataSize > (procXpkCurrentDataEnd(objSrc) - pEntry->iDataOffset) ) {
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
	}

	*pCanCopyRet = TRUE;
	return XPK_OK;
}

// 检查普通布局单条目 Solid 直拷条件
static inline int procXpkBuildCheckSingleEntrySolidDirectFromNormal(xpkObject objDst, xpkObject objSrc, xpkEntry** pEntryRet, xpkWriteNode** pNodeRet, uint64_t* pCompSizeRet, int* pCanCopyRet)
{
	uint32_t iPos;
	uint32_t iLiveCount;
	xpkEntry* pEntry;
	xpkWriteNode* pNode;
	uint8_t iSolidLevel;
	uint64_t iDataEnd;
	int iRet;

	if ( pEntryRet != NULL ) {
		*pEntryRet = NULL;
	}
	if ( pNodeRet != NULL ) {
		*pNodeRet = NULL;
	}
	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = 0;
	}
	if ( pCanCopyRet != NULL ) {
		*pCanCopyRet = FALSE;
	}
	if ( objDst == NULL || objSrc == NULL || pEntryRet == NULL || pNodeRet == NULL || pCompSizeRet == NULL || pCanCopyRet == NULL ) {
		return procXpkReturnParamError(objDst);
	}
	if ( objSrc->bSolidApplied || !objDst->objHead.solidMode ) {
		return XPK_OK;
	}

	iSolidLevel = procXpkSolidTargetCompLevel(objDst);
	if ( iSolidLevel == 0 ) {
		return XPK_OK;
	}

	iRet = procXpkVisibleEntryCountStrict(objSrc, &iLiveCount);
	if ( iRet != XPK_OK ) {
		return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
	}
	if ( iLiveCount != 1 ) {
		return XPK_OK;
	}

	pEntry = NULL;
	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
		}
		break;
	}

	if ( pEntry == NULL || procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	pNode = procXpkFindWriteNode(objSrc, pEntry->iPos, NULL);
	if ( pNode != NULL ) {
		if ( pNode->iLevel != iSolidLevel ) {
			return XPK_OK;
		}
		if ( pNode->iRawSize != pEntry->iFileSize ) {
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		if ( pNode->pCompData == NULL && pNode->iCompSize > 0 ) {
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		*pEntryRet = pEntry;
		*pNodeRet = pNode;
		*pCompSizeRet = pNode->iCompSize;
		*pCanCopyRet = TRUE;
		return XPK_OK;
	}

	if ( (pEntry->iFlag & XPK_FLAG_COMP_MASK) != iSolidLevel ) {
		return XPK_OK;
	}
	iDataEnd = procXpkCurrentDataEnd(objSrc);
	if ( pEntry->iDataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataOffset > iDataEnd ) {
		return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataSize > (iDataEnd - pEntry->iDataOffset) ) {
		return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	*pEntryRet = pEntry;
	*pNodeRet = NULL;
	*pCompSizeRet = pEntry->iDataSize;
	*pCanCopyRet = TRUE;
	return XPK_OK;
}

// 从普通布局直拷单条目到 Solid
static inline int procXpkBuildCopySingleEntrySolidDirectFromNormal(xpkObject objDst, xpkObject objSrc)
{
	xpkEntry* pEntry;
	xpkWriteNode* pNode;
	xfile hFileSrc;
	xfile hFileDst;
	uint64_t iCompSize;
	uint8_t iSolidLevel;
	int bCanCopy;
	int iRet;

	pEntry = NULL;
	pNode = NULL;
	iCompSize = 0;
	bCanCopy = FALSE;
	iRet = procXpkBuildCheckSingleEntrySolidDirectFromNormal(objDst, objSrc, &pEntry, &pNode, &iCompSize, &bCanCopy);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( !bCanCopy || pEntry == NULL ) {
		return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	hFileSrc = NULL;
	hFileDst = NULL;
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}

	if ( pNode != NULL ) {
		iRet = procXpkBuildWriteAtChunked(objDst, hFileDst, XPK_HEAD_SIZE, pNode->pCompData, iCompSize);
	} else {
		if ( !procXpkAppliedVolumeMode(objSrc) ) {
			hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
			if ( hFileSrc == NULL ) {
				if ( hFileDst != NULL ) {
					xrtClose(hFileDst);
				}
				return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
			}
		}
		iRet = procXpkBuildCopyStoredPayload(objDst, hFileDst, objSrc, hFileSrc, XPK_HEAD_SIZE, pEntry->iDataOffset, iCompSize, NULL);
	}

	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = procXpkBuildAppendEntryOnly(objDst, pEntry, 0, pEntry->iFileSize, pEntry->iFileSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = procXpkRebuildLookup(objDst);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iSolidLevel = procXpkSolidTargetCompLevel(objDst);
	objDst->objHead.defComp = iSolidLevel;
	objDst->bSolidApplied = TRUE;
	objDst->iAppendPos = XPK_HEAD_SIZE + iCompSize;
	objDst->iFileSize = objDst->iAppendPos;
	objDst->bDirtyData = TRUE;
	objDst->bDirtyEntryTable = TRUE;
	objDst->bDirtyHead = TRUE;
	procXpkClearError(objDst);
	return XPK_OK;
}

// 直拷有效条目到 Solid 原样目标
static inline int procXpkBuildCopyLiveEntriesSolidStoreDirect(xpkObject objDst, xpkObject objSrc)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	xpkWriteNode* pNode;
	xfile hFileSrc;
	xfile hFileDst;
	void* pChunkBuild;
	uint64_t iOffsetSolid;
	int iRet;

	hFileSrc = NULL;
	hFileDst = NULL;
	pChunkBuild = NULL;
	iOffsetSolid = 0;
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat));
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}

		pNode = procXpkFindWriteNode(objSrc, pEntry->iPos, NULL);
		if ( pNode != NULL ) {
			iRet = procXpkBuildWriteAtChunked(objDst, hFileDst, XPK_HEAD_SIZE + iOffsetSolid, pNode->pCompData, pNode->iCompSize);
		} else {
			if ( !procXpkAppliedVolumeMode(objSrc) && hFileSrc == NULL ) {
				hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
				if ( hFileSrc == NULL ) {
					if ( hFileDst != NULL ) {
						xrtClose(hFileDst);
					}
					return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
				}
			}
			if ( pChunkBuild == NULL ) {
				pChunkBuild = xpkAllocInternal(XPK_WRITE_FILE_CHUNK_SIZE);
				if ( pChunkBuild == NULL ) {
					return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory));
				}
			}
			iRet = procXpkBuildCopyStoredPayload(objDst, hFileDst, objSrc, hFileSrc, XPK_HEAD_SIZE + iOffsetSolid, pEntry->iDataOffset, pEntry->iDataSize, pChunkBuild);
		}
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, iRet);
		}

		iRet = procXpkBuildAppendEntryOnly(objDst, pEntry, iOffsetSolid, pEntry->iFileSize, pEntry->iFileSize);
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, iRet);
		}

		iOffsetSolid += pEntry->iFileSize;
	}

	if ( pChunkBuild != NULL ) {
		xpkFreeInternal(pChunkBuild);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}

	iRet = procXpkRebuildLookup(objDst);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	objDst->objHead.defComp = 0;
	objDst->bSolidApplied = TRUE;
	objDst->iAppendPos = XPK_HEAD_SIZE + iOffsetSolid;
	objDst->iFileSize = objDst->iAppendPos;
	objDst->bDirtyData = TRUE;
	objDst->bDirtyEntryTable = TRUE;
	objDst->bDirtyHead = TRUE;
	procXpkClearError(objDst);
	return XPK_OK;
}

// 检查 Solid 直拷条件
static inline int procXpkBuildCheckSolidDirectCopy(xpkObject objDst, xpkObject objSrc, uint64_t* pCompSizeRet, int* pCanCopyRet)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	uint64_t iOffsetExpect;
	int iRet;

	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = 0;
	}
	if ( pCanCopyRet != NULL ) {
		*pCanCopyRet = FALSE;
	}
	if ( objDst == NULL || objSrc == NULL || pCompSizeRet == NULL || pCanCopyRet == NULL ) {
		return procXpkReturnParamError(objDst);
	}
	if ( !objSrc->bSolidApplied || !objDst->objHead.solidMode ) {
		return XPK_OK;
	}
	if ( procXpkSolidStoredCompLevel(objSrc) != procXpkSolidTargetCompLevel(objDst) ) {
		return XPK_OK;
	}
	if ( objSrc->objHead.dataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iOffsetExpect = 0;
	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			return XPK_OK;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
		}
		if ( pEntry->iDataOffset != iOffsetExpect ) {
			return XPK_OK;
		}
		iOffsetExpect += pEntry->iFileSize;
	}

	*pCompSizeRet = objSrc->objHead.dataOffset - XPK_HEAD_SIZE;
	*pCanCopyRet = TRUE;
	return XPK_OK;
}

// 直拷 Solid 数据块
static inline int procXpkBuildCopySolidBlockDirect(xpkObject objDst, xpkObject objSrc)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	uint64_t iCompSize;
	int bCanCopy;
	xfile hFileSrc;
	xfile hFileDst;
	uint8_t iSolidLevel;
	int iRet;

	iCompSize = 0;
	bCanCopy = FALSE;
	iRet = procXpkBuildCheckSolidDirectCopy(objDst, objSrc, &iCompSize, &bCanCopy);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( !bCanCopy ) {
		return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	hFileSrc = NULL;
	hFileDst = NULL;
	if ( !procXpkAppliedVolumeMode(objSrc) ) {
		hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}

	iRet = procXpkBuildCopyStoredPayload(objDst, hFileDst, objSrc, hFileSrc, XPK_HEAD_SIZE, XPK_HEAD_SIZE, iCompSize, NULL);
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		iRet = procXpkBuildAppendEntryOnly(objDst, pEntry, pEntry->iDataOffset, pEntry->iFileSize, pEntry->iFileSize);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
	}

	iRet = procXpkRebuildLookup(objDst);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iSolidLevel = procXpkSolidStoredCompLevel(objSrc);
	objDst->objHead.defComp = iSolidLevel;
	objDst->bSolidApplied = TRUE;
	objDst->iAppendPos = XPK_HEAD_SIZE + iCompSize;
	objDst->iFileSize = objDst->iAppendPos;
	objDst->bDirtyData = TRUE;
	objDst->bDirtyEntryTable = TRUE;
	objDst->bDirtyHead = TRUE;
	procXpkClearError(objDst);
	return XPK_OK;
}

// 紧凑复制有效条目到 Solid 原样目标
static inline int procXpkBuildCopyLiveEntriesSolidStoreCompact(xpkObject objDst, xpkObject objSrc)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	xfile hFileSrc;
	xfile hFileDst;
	void* pChunkBuild;
	uint64_t iOffsetSolid;
	uint64_t iSolidSrcSize;
	int iRet;

	iRet = procXpkCalcSolidStoredRawSize(objSrc, &iSolidSrcSize);
	if ( iRet != XPK_OK ) {
		return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
	}

	hFileSrc = NULL;
	hFileDst = NULL;
	pChunkBuild = NULL;
	iOffsetSolid = 0;
	if ( !procXpkAppliedVolumeMode(objSrc) ) {
		hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat));
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc)));
		}
		if ( (pEntry->iDataOffset + pEntry->iFileSize) > iSolidSrcSize ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat));
		}

		if ( pChunkBuild == NULL ) {
			pChunkBuild = xpkAllocInternal(XPK_WRITE_FILE_CHUNK_SIZE);
			if ( pChunkBuild == NULL ) {
				return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory));
			}
		}
		iRet = procXpkBuildCopyStoredPayload(objDst, hFileDst, objSrc, hFileSrc, XPK_HEAD_SIZE + iOffsetSolid, XPK_HEAD_SIZE + pEntry->iDataOffset, pEntry->iFileSize, pChunkBuild);
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, iRet);
		}

		iRet = procXpkBuildAppendEntryOnly(objDst, pEntry, iOffsetSolid, pEntry->iFileSize, pEntry->iFileSize);
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, iRet);
		}

		iOffsetSolid += pEntry->iFileSize;
	}

	if ( pChunkBuild != NULL ) {
		xpkFreeInternal(pChunkBuild);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}

	iRet = procXpkRebuildLookup(objDst);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	objDst->objHead.defComp = 0;
	objDst->bSolidApplied = TRUE;
	objDst->iAppendPos = XPK_HEAD_SIZE + iOffsetSolid;
	objDst->iFileSize = objDst->iAppendPos;
	objDst->bDirtyData = TRUE;
	objDst->bDirtyEntryTable = TRUE;
	objDst->bDirtyHead = TRUE;
	procXpkClearError(objDst);
	return XPK_OK;
}

// 从普通布局复制有效条目到 Solid 原样目标
static inline int procXpkBuildCopyLiveEntriesSolidStoreFromNormal(xpkObject objDst, xpkObject objSrc)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	xpkWriteNode* pNode;
	xfile hFileSrc;
	xfile hFileDst;
	void* pData;
	void* pChunkBuild;
	uint64_t iDataSize;
	uint64_t iOffsetSolid;
	int iRet;

	hFileSrc = NULL;
	hFileDst = NULL;
	pChunkBuild = NULL;
	iOffsetSolid = 0;
	if ( !procXpkAppliedVolumeMode(objSrc) ) {
		hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat));
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc)));
		}

		pNode = procXpkFindWriteNode(objSrc, pEntry->iPos, NULL);
		if ( pNode != NULL && pNode->iLevel == 0 ) {
			if ( pNode->iCompSize != pNode->iRawSize || pNode->iRawSize != pEntry->iFileSize ) {
				return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat));
			}
			if ( pNode->pCompData == NULL && pNode->iCompSize > 0 ) {
				return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat));
			}
			iRet = procXpkBuildWriteAtChunked(objDst, hFileDst, XPK_HEAD_SIZE + iOffsetSolid, pNode->pCompData, pNode->iCompSize);
			iDataSize = pNode->iRawSize;
		} else if ( pNode == NULL && (pEntry->iFlag & XPK_FLAG_COMP_MASK) == 0 ) {
			if ( pEntry->iDataSize != pEntry->iFileSize ) {
				return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat));
			}
			if ( pEntry->iDataOffset < XPK_HEAD_SIZE ) {
				return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat));
			}
			if ( pEntry->iDataOffset > procXpkCurrentDataEnd(objSrc) ) {
				return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat));
			}
			if ( pEntry->iDataSize > (procXpkCurrentDataEnd(objSrc) - pEntry->iDataOffset) ) {
				return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat));
			}
			if ( pChunkBuild == NULL ) {
				pChunkBuild = xpkAllocInternal(XPK_WRITE_FILE_CHUNK_SIZE);
				if ( pChunkBuild == NULL ) {
					return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory));
				}
			}
			iRet = procXpkBuildCopyStoredPayload(objDst, hFileDst, objSrc, hFileSrc, XPK_HEAD_SIZE + iOffsetSolid, pEntry->iDataOffset, pEntry->iDataSize, pChunkBuild);
			iDataSize = pEntry->iFileSize;
		} else {
			pData = procXpkReadEntryDataWithFile(objSrc, pEntry, &iDataSize, hFileSrc);
			if ( pData == NULL ) {
				return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc)));
			}

			iRet = procXpkBuildWriteAtChunked(objDst, hFileDst, XPK_HEAD_SIZE + iOffsetSolid, pData, iDataSize);
			xpkFree(pData);
		}
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, iRet);
		}

		iRet = procXpkBuildAppendEntryOnly(objDst, pEntry, iOffsetSolid, iDataSize, iDataSize);
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, iRet);
		}

		iOffsetSolid += iDataSize;
	}

	if ( pChunkBuild != NULL ) {
		xpkFreeInternal(pChunkBuild);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}

	iRet = procXpkRebuildLookup(objDst);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	objDst->objHead.defComp = 0;
	objDst->bSolidApplied = TRUE;
	objDst->iAppendPos = XPK_HEAD_SIZE + iOffsetSolid;
	objDst->iFileSize = objDst->iAppendPos;
	objDst->bDirtyData = TRUE;
	objDst->bDirtyEntryTable = TRUE;
	objDst->bDirtyHead = TRUE;
	procXpkClearError(objDst);
	return XPK_OK;
}

// 复制有效条目到 Solid 目标
static inline int procXpkBuildCopyLiveEntriesSolid(xpkObject objDst, xpkObject objSrc)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	uint64_t iSolidRawSize64;
	uint64_t iSolidSrcSize;
	uint32_t iSolidSrcAlg;
	int bCanCopyDirect;
	int bCanStoreDirect;
	int iRet;

	iSolidSrcSize = 0;
	iSolidRawSize64 = 0;
	iSolidSrcAlg = procXpkCompLevelToAlg(procXpkSolidStoredCompLevel(objSrc));
	bCanCopyDirect = FALSE;
	bCanStoreDirect = FALSE;
	if ( !objSrc->bSolidApplied ) {
		iRet = procXpkBuildCheckSolidStoreDirect(objDst, objSrc, &bCanStoreDirect);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		if ( bCanStoreDirect ) {
			return procXpkBuildCopyLiveEntriesSolidStoreDirect(objDst, objSrc);
		}
		if ( procXpkSolidTargetCompLevel(objDst) == 0 ) {
			return procXpkBuildCopyLiveEntriesSolidStoreFromNormal(objDst, objSrc);
		}
	}
	if ( objSrc->bSolidApplied ) {
		iRet = procXpkBuildCheckSolidDirectCopy(objDst, objSrc, &iSolidSrcSize, &bCanCopyDirect);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		if ( bCanCopyDirect ) {
			return procXpkBuildCopySolidBlockDirect(objDst, objSrc);
		}
	}
	if ( objSrc->bSolidApplied && procXpkSolidStoredCompLevel(objSrc) == 0 && procXpkSolidTargetCompLevel(objDst) == 0 ) {
		return procXpkBuildCopyLiveEntriesSolidStoreCompact(objDst, objSrc);
	}
	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
		}

		iSolidRawSize64 += pEntry->iFileSize;
	}
	if ( iSolidRawSize64 > UINT32_MAX ) {
		return procXpkSetError(objDst, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}
	if ( procXpkSolidTargetCompLevel(objDst) == 0 ) {
		return procXpkBuildCopyLiveEntriesSolidStoreStreaming(objDst, objSrc, iSolidRawSize64);
	}
	if ( procXpkCompLevelToAlg(procXpkSolidTargetCompLevel(objDst)) == XPK_ALG_LZ4
		|| procXpkCompLevelToAlg(procXpkSolidTargetCompLevel(objDst)) == XPK_ALG_LZ4HC ) {
		return procXpkBuildCopyLiveEntriesSolidLz4Streaming(objDst, objSrc, iSolidRawSize64);
	}
	if ( procXpkCompLevelToAlg(procXpkSolidTargetCompLevel(objDst)) == XPK_ALG_ZSTD ) {
		return procXpkBuildCopyLiveEntriesSolidZstdStreaming(objDst, objSrc, iSolidRawSize64);
	}
	if ( procXpkCompLevelToAlg(procXpkSolidTargetCompLevel(objDst)) == XPK_ALG_LZMA2 ) {
		return procXpkBuildCopyLiveEntriesSolidLzma2Streaming(objDst, objSrc, iSolidRawSize64);
	}

	return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
}

// 从 Solid 源复制有效条目
static inline int procXpkBuildCopyLiveEntriesFromSolidSource(xpkObject objDst, xpkObject objSrc)
{
	uint32_t iPos;
	uint32_t iLiveCount;
	xpkEntry* pEntry;
	void* pData;
	uint64_t iDataSize;
	int iRet;

	iRet = procXpkVisibleEntryCountStrict(objSrc, &iLiveCount);
	if ( iRet != XPK_OK ) {
		return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
	}
	if ( iLiveCount == 0 ) {
		return XPK_OK;
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
		}

		pData = procXpkReadEntryData(objSrc, pEntry, &iDataSize);
		if ( pData == NULL ) {
			return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
		}
		if ( iDataSize != pEntry->iFileSize ) {
			xpkFree(pData);
			return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		iRet = procXpkBuildCopyEntryData(objDst, pEntry, pData, iDataSize);
		xpkFree(pData);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
	}

	return XPK_OK;
}

// 从 Solid 原样源复制有效条目
static inline int procXpkBuildCopyLiveEntriesFromSolidStoredSource(xpkObject objDst, xpkObject objSrc)
{
	uint32_t iPos;
	uint32_t iLiveCount;
	xpkEntry* pEntry;
	xfile hFileSrc;
	xfile hFileDst;
	void* pChunkBuild;
	uint64_t iSolidRawSize;
	uint64_t iOffsetDst;
	int iRet;

	iRet = procXpkVisibleEntryCountStrict(objSrc, &iLiveCount);
	if ( iRet != XPK_OK ) {
		return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
	}
	if ( iLiveCount == 0 ) {
		return XPK_OK;
	}

	iRet = procXpkCalcSolidStoredRawSize(objSrc, &iSolidRawSize);
	if ( iRet != XPK_OK ) {
		return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
	}

	hFileSrc = NULL;
	hFileDst = NULL;
	pChunkBuild = NULL;
	if ( !procXpkAppliedVolumeMode(objSrc) ) {
		hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen));
		}
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat));
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc)));
		}
		if ( (pEntry->iFlag & XPK_FLAG_COMP_MASK) != 0 ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat));
		}
		if ( pEntry->iDataSize != pEntry->iFileSize ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat));
		}
		if ( (pEntry->iDataOffset + pEntry->iFileSize) > iSolidRawSize ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat));
		}

		iOffsetDst = objDst->iAppendPos;
		if ( pChunkBuild == NULL ) {
			pChunkBuild = xpkAllocInternal(XPK_WRITE_FILE_CHUNK_SIZE);
			if ( pChunkBuild == NULL ) {
				return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory));
			}
		}
		iRet = procXpkBuildCopyStoredPayload(objDst, hFileDst, objSrc, hFileSrc, iOffsetDst, XPK_HEAD_SIZE + pEntry->iDataOffset, pEntry->iFileSize, pChunkBuild);
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, iRet);
		}

		iRet = procXpkBuildAppendCopiedEntry(objDst, pEntry, 0, iOffsetDst, pEntry->iFileSize, pEntry->iFileSize);
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, iRet);
		}

		objDst->iAppendPos = iOffsetDst + pEntry->iFileSize;
		if ( objDst->iAppendPos > objDst->iFileSize ) {
			objDst->iFileSize = objDst->iAppendPos;
		}
		objDst->bDirtyData = TRUE;
		procXpkMarkDirtyEntryTable(objDst);
	}

	if ( pChunkBuild != NULL ) {
		xpkFreeInternal(pChunkBuild);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}

	procXpkClearError(objDst);
	return XPK_OK;
}

// 检查 Solid 单条目直拷条件
static inline int procXpkBuildCheckSolidSingleEntryDirect(xpkObject objDst, xpkObject objSrc, xpkEntry** pEntryRet, uint64_t* pCompSizeRet, int* pCanCopyRet)
{
	uint32_t iPos;
	uint32_t iLiveCount;
	xpkEntry* pEntry;
	uint64_t iSolidRawSize;
	int iRet;

	if ( pEntryRet != NULL ) {
		*pEntryRet = NULL;
	}
	if ( pCompSizeRet != NULL ) {
		*pCompSizeRet = 0;
	}
	if ( pCanCopyRet != NULL ) {
		*pCanCopyRet = FALSE;
	}
	if ( objDst == NULL || objSrc == NULL || pEntryRet == NULL || pCompSizeRet == NULL || pCanCopyRet == NULL ) {
		return procXpkReturnParamError(objDst);
	}
	if ( objDst->objHead.solidMode || !objSrc->bSolidApplied ) {
		return XPK_OK;
	}

	iRet = procXpkVisibleEntryCountStrict(objSrc, &iLiveCount);
	if ( iRet != XPK_OK ) {
		return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
	}
	if ( iLiveCount != 1 ) {
		return XPK_OK;
	}

	pEntry = NULL;
	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
		}
		break;
	}

	if ( pEntry == NULL || procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( objSrc->objHead.dataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objDst, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	iRet = procXpkCalcSolidStoredRawSize(objSrc, &iSolidRawSize);
	if ( iRet != XPK_OK ) {
		return procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc));
	}
	if ( pEntry->iDataOffset != 0 || pEntry->iFileSize != iSolidRawSize ) {
		return XPK_OK;
	}

	*pEntryRet = pEntry;
	*pCompSizeRet = objSrc->objHead.dataOffset - XPK_HEAD_SIZE;
	*pCanCopyRet = TRUE;
	return XPK_OK;
}

// 直拷单个 Solid 条目
static inline int procXpkBuildCopySingleSolidEntryDirect(xpkObject objDst, xpkObject objSrc)
{
	xpkEntry* pEntry;
	xfile hFileSrc;
	xfile hFileDst;
	uint64_t iCompSize;
	uint64_t iOffsetDst;
	uint8_t iCompLevel;
	int bCanCopy;
	int iRet;

	pEntry = NULL;
	iCompSize = 0;
	bCanCopy = FALSE;
	iRet = procXpkBuildCheckSolidSingleEntryDirect(objDst, objSrc, &pEntry, &iCompSize, &bCanCopy);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( !bCanCopy || pEntry == NULL ) {
		return procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	hFileSrc = NULL;
	hFileDst = NULL;
	if ( !procXpkAppliedVolumeMode(objSrc) ) {
		hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileSrc == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			if ( hFileSrc != NULL ) {
				xrtClose(hFileSrc);
			}
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}

	iOffsetDst = objDst->iAppendPos;
	iRet = procXpkBuildCopyStoredPayload(objDst, hFileDst, objSrc, hFileSrc, iOffsetDst, XPK_HEAD_SIZE, iCompSize, NULL);
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iCompLevel = procXpkSolidStoredCompLevel(objSrc);
	iRet = procXpkBuildAppendCopiedEntry(objDst, pEntry, iCompLevel, iOffsetDst, iCompSize, pEntry->iFileSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = procXpkRebuildLookup(objDst);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	objDst->iAppendPos = iOffsetDst + iCompSize;
	if ( objDst->iAppendPos > objDst->iFileSize ) {
		objDst->iFileSize = objDst->iAppendPos;
	}
	objDst->bDirtyData = TRUE;
	procXpkMarkDirtyEntryTable(objDst);
	procXpkClearError(objDst);
	return XPK_OK;
}

// 构建复制有效条目
static inline int procXpkBuildCopyLiveEntries(xpkObject objDst, xpkObject objSrc)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	xpkEntry* pEntryDirect;
	xpkWriteNode* pNode;
	xfile hFileSrc;
	xfile hFileDst;
	void* pChunkBuild;
	uint64_t iCompSizeDirect;
	int bCanCopyDirect;
	int iRet;

	if ( objDst->objHead.solidMode ) {
		return procXpkBuildCopyLiveEntriesSolid(objDst, objSrc);
	}
	if ( objSrc->bSolidApplied ) {
		pEntryDirect = NULL;
		iCompSizeDirect = 0;
		bCanCopyDirect = FALSE;
		iRet = procXpkBuildCheckSolidSingleEntryDirect(objDst, objSrc, &pEntryDirect, &iCompSizeDirect, &bCanCopyDirect);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		if ( bCanCopyDirect ) {
			return procXpkBuildCopySingleSolidEntryDirect(objDst, objSrc);
		}
		if ( procXpkSolidStoredCompLevel(objSrc) == 0 ) {
			return procXpkBuildCopyLiveEntriesFromSolidStoredSource(objDst, objSrc);
		}
		return procXpkBuildCopyLiveEntriesFromSolidSource(objDst, objSrc);
	}

	hFileSrc = NULL;
	hFileDst = NULL;
	pChunkBuild = NULL;
	if ( !procXpkAppliedVolumeMode(objDst) ) {
		hFileDst = xrtOpen(objDst->sPathPackage, FALSE, XRT_CP_BINARY);
		if ( hFileDst == NULL ) {
			return procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen);
		}
	}

	for ( iPos = 1; iPos <= objSrc->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objSrc->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_STATE, sXpkErrorBadFormat));
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objSrc, pEntry);
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, xpkLastError(objSrc), xpkLastErrorMessage(objSrc)));
		}

		pNode = procXpkFindWriteNode(objSrc, pEntry->iPos, NULL);
		if ( pNode == NULL && !procXpkAppliedVolumeMode(objSrc) && hFileSrc == NULL ) {
			hFileSrc = xrtOpen(objSrc->sPathPackage, TRUE, XRT_CP_BINARY);
			if ( hFileSrc == NULL ) {
				return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_IO, sXpkErrorIoOpen));
			}
		}
		if ( pNode == NULL && pChunkBuild == NULL ) {
			pChunkBuild = xpkAllocInternal(XPK_WRITE_FILE_CHUNK_SIZE);
			if ( pChunkBuild == NULL ) {
				return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, procXpkSetError(objDst, XPK_ERR_MEMORY, sXpkErrorOutOfMemory));
			}
		}

		iRet = procXpkBuildCopyEntryDirectWithFiles(objDst, objSrc, pEntry, hFileDst, hFileSrc, pChunkBuild);
		if ( iRet != XPK_OK ) {
			return procXpkBuildCleanupChunkFiles(pChunkBuild, hFileSrc, hFileDst, iRet);
		}
	}

	if ( pChunkBuild != NULL ) {
		xpkFreeInternal(pChunkBuild);
	}
	if ( hFileSrc != NULL ) {
		xrtClose(hFileSrc);
	}
	if ( hFileDst != NULL ) {
		xrtClose(hFileDst);
	}

	iRet = procXpkRebuildLookup(objDst);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	return XPK_OK;
}

// 构建生成临时路径
static inline char* procXpkBuildMakeTempPath(xpkObject objXpk, const xpkBuildOptions* pOpt, int bReplaceOriginal)
{
	size_t iPathLen;
	char* sTempPath;

	if ( (pOpt != NULL) && (pOpt->tempPath != NULL) && (pOpt->tempPath[0] != '\0') ) {
		if ( procXpkPathTextEqualFs(pOpt->tempPath, objXpk->sPathPackage) ) {
			procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
			return NULL;
		}
		sTempPath = procXpkDupText(pOpt->tempPath);
		if ( sTempPath == NULL ) {
			procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			return NULL;
		}
		return sTempPath;
	}
	if ( !bReplaceOriginal ) {
		procXpkSetError(objXpk, XPK_ERR_PARAM, "tempPath is required when replaceOriginal is disabled");
		return NULL;
	}

	iPathLen = strlen(objXpk->sPathPackage);
	sTempPath = (char*)xpkAllocInternal(iPathLen + 11);
	if ( sTempPath == NULL ) {
		procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		return NULL;
	}

	memcpy(sTempPath, objXpk->sPathPackage, iPathLen);
	memcpy(sTempPath + iPathLen, ".build.tmp", 11);
	return sTempPath;
}

// 构建确保临时路径未占用
static inline int procXpkBuildEnsureTempPathUnused(xpkObject objXpk, const char* sTempPath)
{
	if ( sTempPath == NULL || sTempPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	return procXpkEnsureVolumePathUnusedText(objXpk, sTempPath, sXpkErrorTempPathExists);
}

// 构建校验选项
static inline int procXpkBuildValidateOptions(xpkObject objXpk, const xpkBuildOptions* pOpt, int* pReplaceOriginalRet)
{
	int bReplaceOriginal;
	int iRet;
	int bPathInBackupDir;
	int bPathInBackupVolume;
	char* sBackupPath;
	size_t iBackupLen;
	unsigned char iFollow;

	bReplaceOriginal = TRUE;
	if ( pOpt != NULL ) {
		bReplaceOriginal = pOpt->replaceOriginal ? TRUE : FALSE;
	}

	if ( (pOpt != NULL) && (pOpt->tempPath != NULL) && (pOpt->tempPath[0] != '\0') ) {
		if ( procXpkPathTextEqualFs(pOpt->tempPath, objXpk->sPathPackage) ) {
			return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
		}
		if ( bReplaceOriginal ) {
			sBackupPath = procXpkPathSuffixDupText(objXpk->sPathPackage, ".replace.bak");
			if ( sBackupPath == NULL ) {
				return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			}
			if ( procXpkPathTextEqualFs(pOpt->tempPath, sBackupPath) ) {
				xpkFreeInternal(sBackupPath);
				return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
			}
			iBackupLen = strlen(sBackupPath);
			if ( strncmp(pOpt->tempPath, sBackupPath, iBackupLen) == 0 ) {
				iFollow = (unsigned char)pOpt->tempPath[iBackupLen];
				if ( iFollow == '/' || iFollow == '\\' ) {
					xpkFreeInternal(sBackupPath);
					return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
				}
			}
			bPathInBackupDir = FALSE;
			iRet = procXpkPathInDirFs(objXpk, pOpt->tempPath, sBackupPath, &bPathInBackupDir);
			if ( iRet != XPK_OK ) {
				xpkFreeInternal(sBackupPath);
				return iRet;
			}
			if ( bPathInBackupDir ) {
				xpkFreeInternal(sBackupPath);
				return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
			}
			bPathInBackupVolume = FALSE;
			iRet = procXpkPathIsVolumeFamilyFs(objXpk, pOpt->tempPath, sBackupPath, &bPathInBackupVolume);
			if ( iRet != XPK_OK ) {
				xpkFreeInternal(sBackupPath);
				return iRet;
			}
			if ( bPathInBackupVolume ) {
				xpkFreeInternal(sBackupPath);
				return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
			}
			xpkFreeInternal(sBackupPath);
		}
		iRet = procXpkBuildEnsureTempPathUnused(objXpk, pOpt->tempPath);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
	} else if ( !bReplaceOriginal ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, "tempPath is required when replaceOriginal is disabled");
	}

	if ( pReplaceOriginalRet != NULL ) {
		*pReplaceOriginalRet = bReplaceOriginal;
	}
	return XPK_OK;
}

// 构建重置临时路径
static inline int procXpkBuildResetTempPath(xpkObject objXpk, const char* sTempPath)
{
	int iRet;

	if ( sTempPath == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	if ( xrtFileExists((str)sTempPath) && !xrtFileDelete((str)sTempPath) ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoWrite);
	}

	iRet = procXpkScanLooseVolumeFilesText(objXpk, sTempPath, TRUE, NULL);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	return XPK_OK;
}

// 构建重新加载自身
static inline int procXpkBuildReloadSelf(xpkObject objXpk)
{
	xpkOpenOptions objOpt;
	xpkObject objReload;
	int iRet;
	uint32_t iTry;

	memset(&objOpt, 0, sizeof(objOpt));
	objOpt.readonly = objXpk->bReadonly;
	objOpt.bufferedDefault = objXpk->bBufferedDefault;

	objReload = NULL;
	for ( iTry = 0; iTry < 8u; iTry++ ) {
		objReload = xpkOpen(objXpk->sPathPackage, &objOpt);
		if ( objReload != NULL ) {
			break;
		}
		if ( iTry + 1u < 8u ) {
			xrtSleep(10);
		}
	}
	if ( objReload == NULL ) {
		iRet = xpkLastError(NULL);
		if ( iRet == XPK_OK ) {
			iRet = XPK_ERR_IO;
		}
		return procXpkSetError(objXpk, iRet, xpkLastErrorMessage(NULL));
	}

	procXpkUnitWriteQueue(objXpk);
	procXpkUnitObject(objXpk);
	*objXpk = *objReload;
	xpkFreeInternal(objReload);
	return XPK_OK;
}

// 执行构建重构流程
static inline int procXpkBuildPackage(xpkObject objXpk, const xpkBuildOptions* pOpt)
{
	int bReplaceOriginal;
	char* sTempPath;
	char* sFinalPath;
	xpkObject objBuild;
	xpkOpenOptions objOpenOpt;
	char sBuildError[XPK_ERROR_TEXT_CAP];
	char sReplaceError[XPK_ERROR_TEXT_CAP];
	const char* sBuildText;
	size_t iReplaceTextSize;
	size_t iBuildTextSize;
	int iRet;

	// 先校验对象状态和 build 选项，避免后续在临时路径上做出不可逆修改。
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	iRet = procXpkBuildValidateOptions(objXpk, pOpt, &bReplaceOriginal);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( objXpk->bReadonly ) {
		return procXpkSetError(objXpk, XPK_ERR_READONLY, sXpkErrorReadonly);
	}

	sTempPath = procXpkBuildMakeTempPath(objXpk, pOpt, bReplaceOriginal);
	if ( sTempPath == NULL ) {
		return xpkLastError(objXpk);
	}
	iRet = procXpkBuildEnsureTempPathUnused(objXpk, sTempPath);
	if ( iRet != XPK_OK ) {
		xpkFreeInternal(sTempPath);
		return iRet;
	}

	iRet = procXpkBuildResetTempPath(objXpk, sTempPath);
	if ( iRet != XPK_OK ) {
		xpkFreeInternal(sTempPath);
		return iRet;
	}

	// 打开临时目标包，后续所有复制与保存都先落到这个安全副本里。
	memset(&objOpenOpt, 0, sizeof(objOpenOpt));
	objOpenOpt.createIfMissing = TRUE;
	objBuild = xpkOpen(sTempPath, &objOpenOpt);
	if ( objBuild == NULL ) {
		sBuildText = xpkLastErrorMessage(NULL);
		iBuildTextSize = strlen(sBuildText);
		if ( iBuildTextSize >= XPK_ERROR_TEXT_CAP ) {
			iBuildTextSize = XPK_ERROR_TEXT_CAP - 1;
		}
		memcpy(sBuildError, sBuildText, iBuildTextSize);
		sBuildError[iBuildTextSize] = '\0';
		iRet = xpkLastError(NULL);
		if ( iRet == XPK_OK ) {
			iRet = XPK_ERR_IO;
			memcpy(sBuildError, sXpkErrorIoOpen, sizeof(sXpkErrorIoOpen));
		}
		xpkFreeInternal(sTempPath);
		return procXpkSetError(objXpk, iRet, sBuildError);
	}
	sBuildError[0] = '\0';
	sFinalPath = NULL;

	// 先复制包级配置，再复制有效条目，最后把临时包完整保存下来。
	iRet = procXpkBuildCopyConfig(objBuild, objXpk);
	if ( iRet == XPK_OK ) {
		iRet = procXpkBuildCopyLiveEntries(objBuild, objXpk);
	}
	if ( iRet == XPK_OK ) {
		iRet = procXpkSavePackage(objBuild);
	}
	if ( iRet != XPK_OK ) {
		sBuildText = xpkLastErrorMessage(objBuild);
		if ( sBuildText[0] == '\0' ) {
			sBuildText = xpkLastErrorMessage(objXpk);
		}
		if ( sBuildText[0] == '\0' ) {
			sBuildText = xpkLastErrorMessage(NULL);
		}
		if ( sBuildText[0] == '\0' ) {
			sBuildText = (iRet == XPK_ERR_IO) ? sXpkErrorIoWrite : sXpkErrorInvalidParam;
		}
		iBuildTextSize = strlen(sBuildText);
		if ( iBuildTextSize >= XPK_ERROR_TEXT_CAP ) {
			iBuildTextSize = XPK_ERROR_TEXT_CAP - 1;
		}
		memcpy(sBuildError, sBuildText, iBuildTextSize);
		sBuildError[iBuildTextSize] = '\0';
	}

	if ( iRet != XPK_OK ) {
		xpkClose(objBuild);
		(void)procXpkBuildResetTempPath(objXpk, sTempPath);
		xpkFreeInternal(sTempPath);
		return procXpkSetError(objXpk, iRet, sBuildError);
	}

	// 非替换模式直接保留临时产物；替换模式则继续把结果移动回原路径。
	if ( !bReplaceOriginal ) {
		xpkClose(objBuild);
		xpkFreeInternal(sTempPath);
		procXpkClearError(objXpk);
		return XPK_OK;
	}

	if ( procXpkMoveVolumeFilesText(objXpk, sTempPath, objXpk->sPathPackage) != XPK_OK ) {
		iRet = xpkLastError(objXpk);
		sBuildText = xpkLastErrorMessage(objXpk);
		if ( sBuildText[0] == '\0' ) {
			sBuildText = xpkLastErrorMessage(NULL);
		}
		if ( sBuildText[0] == '\0' ) {
			sBuildText = (iRet == XPK_ERR_IO) ? sXpkErrorIoWrite : sXpkErrorInvalidParam;
		}
		iReplaceTextSize = strlen(sBuildText);
		if ( iReplaceTextSize >= XPK_ERROR_TEXT_CAP ) {
			iReplaceTextSize = XPK_ERROR_TEXT_CAP - 1;
		}
		memcpy(sReplaceError, sBuildText, iReplaceTextSize);
		sReplaceError[iReplaceTextSize] = '\0';
		xpkClose(objBuild);
		(void)procXpkBuildResetTempPath(objXpk, sTempPath);
		xpkFreeInternal(sTempPath);
		return procXpkSetError(objXpk, iRet, sReplaceError);
	}

	// 用新构建对象接管当前运行时状态，避免再走一次完整重开流程。
	sFinalPath = procXpkDupText(objXpk->sPathPackage);
	if ( sFinalPath == NULL ) {
		xpkClose(objBuild);
		xpkFreeInternal(sTempPath);
		return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
	}

	xpkFreeInternal(objBuild->sPathPackage);
	objBuild->sPathPackage = sFinalPath;
	sFinalPath = NULL;
	objBuild->err.iCode = XPK_OK;
	objBuild->err.sText[0] = '\0';

	procXpkUnitWriteQueue(objXpk);
	procXpkUnitObject(objXpk);
	*objXpk = *objBuild;
	xpkFreeInternal(objBuild);

	xpkFreeInternal(sTempPath);
	procXpkClearError(objXpk);
	return XPK_OK;
}


/* ===== File: src/service/verify.h ===== */

/*
	xPack 校验与统计模块

	负责条目校验、整包校验以及当前统计信息计算。
*/

// 哈希映射文件范围
static inline int procXpkHashMappedFileRange(xpkObject objXpk, xfile hFile, uint64_t iOffset, uint64_t iSize, uint32_t* pHashRet)
{
	xpkMappedFile objMap;
	uint64_t iFileSize;

	if ( objXpk == NULL || hFile == NULL || pHashRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	iFileSize = xrtGetEOF(hFile);
	if ( iOffset > iFileSize || iSize > (iFileSize - iOffset) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	memset(&objMap, 0, sizeof(objMap));
	if ( procXpkMapFileReadOnly(objXpk, hFile, iFileSize, &objMap) != XPK_OK ) {
		return xpkLastError(objXpk);
	}

	*pHashRet = (iSize > 0) ? xpkHash32Internal((const uint8_t*)objMap.pView + iOffset, iSize) : 0;
	procXpkUnmapFile(&objMap);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 校验哈希匹配
static inline int procXpkVerifyHashMatch(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint64_t iSize)
{
	uint32_t iHash;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	iHash = (iSize > 0) ? xpkHash32Internal(pData, iSize) : 0;
	if ( iHash != pEntry->iFileHash ) {
		return procXpkSetError(objXpk, XPK_ERR_HASH, sXpkErrorHashMismatch);
	}

	procXpkClearError(objXpk);
	return XPK_OK;
}

// 校验 Solid 条目
static inline int procXpkVerifySolidEntry(xpkObject objXpk, xpkEntry* pEntry)
{
	void* pData;
	xfile hFile;
	uint32_t iHash;
	uint32_t iAlg;
	uint64_t iSize;
	uint64_t iSolidSize;
	int iRet;

	if ( procXpkSolidStoredCompLevel(objXpk) == 0 ) {
		if ( !procXpkAppliedVolumeMode(objXpk) ) {
			iRet = procXpkCalcSolidStoredRawSize(objXpk, &iSolidSize);
			if ( iRet != XPK_OK ) {
				return iRet;
			}
			if ( (pEntry->iDataOffset + pEntry->iFileSize) > iSolidSize ) {
				return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
			}
			hFile = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
			if ( hFile == NULL ) {
				return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
			}
			iRet = procXpkHashMappedFileRange(objXpk, hFile, XPK_HEAD_SIZE + pEntry->iDataOffset, pEntry->iFileSize, &iHash);
			xrtClose(hFile);
			if ( iRet != XPK_OK ) {
				return iRet;
			}
			if ( iHash != pEntry->iFileHash ) {
				return procXpkSetError(objXpk, XPK_ERR_HASH, sXpkErrorHashMismatch);
			}
			procXpkClearError(objXpk);
			return XPK_OK;
		}
		if ( pEntry->iFileSize > UINT32_MAX ) {
			return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
		}
		pData = procXpkReadSolidEntryData(objXpk, pEntry, &iSize);
		if ( pData == NULL ) {
			return xpkLastError(objXpk);
		}
		iRet = procXpkVerifyHashMatch(objXpk, pEntry, pData, iSize);
		xpkFree(pData);
		return iRet;
	}

	iAlg = procXpkCompLevelToAlg(procXpkSolidStoredCompLevel(objXpk));
	if ( iAlg == XPK_ALG_LZ4 || iAlg == XPK_ALG_LZ4HC || iAlg == XPK_ALG_ZSTD || iAlg == XPK_ALG_LZMA2 ) {
		pData = procXpkReadSolidEntryData(objXpk, pEntry, &iSize);
		if ( pData == NULL ) {
			return xpkLastError(objXpk);
		}
		iRet = procXpkVerifyHashMatch(objXpk, pEntry, pData, iSize);
		xpkFree(pData);
		return iRet;
	}

	return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
}

// 校验原样条目
static inline int procXpkVerifyStoredEntry(xpkObject objXpk, xpkEntry* pEntry)
{
	return procXpkVerifyStoredEntryWithFile(objXpk, pEntry, NULL);
}

// 校验原样条目带文件
static inline int procXpkVerifyStoredEntryWithFile(xpkObject objXpk, xpkEntry* pEntry, xfile hFile)
{
	xpkWriteNode* pNode;
	xfile hFileLocal;
	xpkMappedFile objMap;
	uint64_t iDataEnd;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	pNode = procXpkFindWriteNode(objXpk, pEntry->iPos, NULL);
	if ( pNode != NULL ) {
		if ( pNode->iLevel != 0 ) {
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( pNode->iRawSize != pEntry->iFileSize || pNode->iCompSize != pEntry->iFileSize ) {
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		if ( pNode->pCompData == NULL && pNode->iCompSize > 0 ) {
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}
		return procXpkVerifyHashMatch(objXpk, pEntry, pNode->pCompData, pNode->iCompSize);
	}

	if ( pEntry->iDataSize != pEntry->iFileSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	iDataEnd = procXpkCurrentDataEnd(objXpk);
	if ( pEntry->iDataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataOffset > iDataEnd ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataSize > (iDataEnd - pEntry->iDataOffset) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( procXpkAppliedVolumeMode(objXpk) ) {
		void* pCompData;

		if ( pEntry->iDataSize > UINT32_MAX ) {
			return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
		}

		pCompData = NULL;
		iRet = procXpkReadAtAlloc(objXpk, hFile, pEntry->iDataOffset, (uint32_t)pEntry->iDataSize, &pCompData);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		iRet = procXpkVerifyHashMatch(objXpk, pEntry, pCompData, pEntry->iDataSize);
		xpkFreeInternal(pCompData);
		return iRet;
	}

	hFileLocal = NULL;
	if ( hFile == NULL ) {
		hFileLocal = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
		if ( hFileLocal == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
		}
		hFile = hFileLocal;
	}

	memset(&objMap, 0, sizeof(objMap));
	iRet = procXpkMapFileReadOnly(objXpk, hFile, xrtGetEOF(hFile), &objMap);
	if ( iRet != XPK_OK ) {
		if ( hFileLocal != NULL ) {
			xrtClose(hFileLocal);
		}
		return iRet;
	}

	iRet = procXpkVerifyStoredEntryMapped(objXpk, pEntry, &objMap);
	procXpkUnmapFile(&objMap);
	if ( hFileLocal != NULL ) {
		xrtClose(hFileLocal);
	}
	return iRet;
}

// 校验原样条目映射
static inline int procXpkVerifyStoredEntryMapped(xpkObject objXpk, xpkEntry* pEntry, const xpkMappedFile* pMap)
{
	uint64_t iDataEnd;

	if ( objXpk == NULL || pEntry == NULL || pMap == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( pMap->pView == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataSize != pEntry->iFileSize ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	iDataEnd = procXpkCurrentDataEnd(objXpk);
	if ( pEntry->iDataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataOffset > iDataEnd ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataSize > (iDataEnd - pEntry->iDataOffset) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataOffset > pMap->iSize || pEntry->iDataSize > (pMap->iSize - pEntry->iDataOffset) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	return procXpkVerifyHashMatch(objXpk, pEntry, (const uint8_t*)pMap->pView + pEntry->iDataOffset, pEntry->iDataSize);
}

// 基于文件句柄校验解码条目
static inline int procXpkVerifyDecodedEntryWithFile(xpkObject objXpk, xpkEntry* pEntry, xfile hFile)
{
	xpkWriteNode* pNode;
	xfile hFileLocal;
	xpkMappedFile objMap;
	void* pData;
	uint64_t iSize;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	pNode = procXpkFindWriteNode(objXpk, pEntry->iPos, NULL);
	if ( pNode == NULL && !procXpkAppliedVolumeMode(objXpk) ) {
		hFileLocal = NULL;
		if ( hFile == NULL ) {
			hFileLocal = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
			if ( hFileLocal == NULL ) {
				return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
			}
			hFile = hFileLocal;
		}

		memset(&objMap, 0, sizeof(objMap));
		iRet = procXpkMapFileReadOnly(objXpk, hFile, xrtGetEOF(hFile), &objMap);
		if ( iRet != XPK_OK ) {
			if ( hFileLocal != NULL ) {
				xrtClose(hFileLocal);
			}
			return iRet;
		}

		iRet = procXpkVerifyDecodedEntryMapped(objXpk, pEntry, &objMap);
		procXpkUnmapFile(&objMap);
		if ( hFileLocal != NULL ) {
			xrtClose(hFileLocal);
		}
		return iRet;
	}

	pData = procXpkReadEntryDataWithFile(objXpk, pEntry, &iSize, hFile);
	if ( pData == NULL ) {
		return xpkLastError(objXpk);
	}
	iRet = procXpkVerifyHashMatch(objXpk, pEntry, pData, iSize);
	xpkFree(pData);
	return iRet;
}

// 基于映射校验解码条目
static inline int procXpkVerifyDecodedEntryMapped(xpkObject objXpk, xpkEntry* pEntry, const xpkMappedFile* pMap)
{
	void* pData;
	uint64_t iDataEnd;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL || pMap == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( pMap->pView == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataSize > UINT32_MAX || pEntry->iFileSize > UINT32_MAX ) {
		return procXpkSetError(objXpk, XPK_ERR_UNSUPPORTED, sXpkErrorBlockTooLarge);
	}
	iDataEnd = procXpkCurrentDataEnd(objXpk);
	if ( pEntry->iDataOffset < XPK_HEAD_SIZE ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataOffset > iDataEnd ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataSize > (iDataEnd - pEntry->iDataOffset) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}
	if ( pEntry->iDataOffset > pMap->iSize || pEntry->iDataSize > (pMap->iSize - pEntry->iDataOffset) ) {
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	pData = NULL;
	iRet = procXpkCodecDecode(objXpk, (uint8_t)(pEntry->iFlag & XPK_FLAG_COMP_MASK), (const uint8_t*)pMap->pView + pEntry->iDataOffset, (uint32_t)pEntry->iDataSize, (uint32_t)pEntry->iFileSize, &pData);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = procXpkVerifyHashMatch(objXpk, pEntry, pData, pEntry->iFileSize);
	xpkFree(pData);
	return iRet;
}

// 校验全部确保数据文件
static inline int procXpkVerifyAllEnsureDataFile(xpkObject objXpk, xfile* pFileRet)
{
	if ( objXpk == NULL || pFileRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( procXpkAppliedVolumeMode(objXpk) ) {
		return XPK_OK;
	}
	if ( *pFileRet != NULL ) {
		return XPK_OK;
	}

	*pFileRet = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
	if ( *pFileRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}

	return XPK_OK;
}

// 校验条目
static inline int procXpkVerifyEntry(xpkObject objXpk, xpkEntry* pEntry)
{
	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkVerifySolidEntry(objXpk, pEntry);
	}
	if ( (pEntry->iFlag & XPK_FLAG_COMP_MASK) == 0 ) {
		return procXpkVerifyStoredEntry(objXpk, pEntry);
	}

	return procXpkVerifyDecodedEntryWithFile(objXpk, pEntry, NULL);
}

// 逐条校验全部 Solid 压缩条目
static inline int procXpkVerifyAllSolidCompressedEntriesByEntry(xpkObject objXpk)
{
	uint32_t iPos;
	uint32_t iLiveCount;
	xpkEntry* pEntry;
	int iRet;

	iRet = procXpkVisibleEntryCountStrict(objXpk, &iLiveCount);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( iLiveCount == 0 ) {
		procXpkClearError(objXpk);
		return XPK_OK;
	}

	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objXpk, pEntry);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		iRet = procXpkVerifySolidEntry(objXpk, pEntry);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
	}

	procXpkClearError(objXpk);
	return XPK_OK;
}

// 基于映射校验全部 Solid 原样条目
static inline int procXpkVerifyAllSolidStoredEntriesMapped(xpkObject objXpk)
{
	uint32_t iPos;
	uint32_t iLiveCount;
	xpkEntry* pEntry;
	xfile hFile;
	xpkMappedFile objMap;
	uint64_t iRawSize;
	uint64_t iFileSize;
	uint32_t iHash;
	int iRet;

	iRet = procXpkVisibleEntryCountStrict(objXpk, &iLiveCount);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( iLiveCount == 0 ) {
		procXpkClearError(objXpk);
		return XPK_OK;
	}
	if ( procXpkAppliedVolumeMode(objXpk) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	iRet = procXpkCalcSolidStoredRawSize(objXpk, &iRawSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	hFile = xrtOpen(objXpk->sPathPackage, TRUE, XRT_CP_BINARY);
	if ( hFile == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_IO, sXpkErrorIoOpen);
	}
	iFileSize = xrtGetEOF(hFile);
	if ( objXpk->objHead.dataOffset > iFileSize ) {
		xrtClose(hFile);
		return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
	}

	memset(&objMap, 0, sizeof(objMap));
	iRet = procXpkMapFileReadOnly(objXpk, hFile, iFileSize, &objMap);
	if ( iRet != XPK_OK ) {
		xrtClose(hFile);
		return iRet;
	}

	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			procXpkUnmapFile(&objMap);
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objXpk, pEntry);
		if ( iRet != XPK_OK ) {
			procXpkUnmapFile(&objMap);
			xrtClose(hFile);
			return iRet;
		}
		if ( (pEntry->iDataOffset + pEntry->iFileSize) > iRawSize ) {
			procXpkUnmapFile(&objMap);
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_FORMAT, sXpkErrorBadFormat);
		}

		iHash = (pEntry->iFileSize > 0) ? xpkHash32Internal((const uint8_t*)objMap.pView + XPK_HEAD_SIZE + pEntry->iDataOffset, pEntry->iFileSize) : 0;
		if ( iHash != pEntry->iFileHash ) {
			procXpkUnmapFile(&objMap);
			xrtClose(hFile);
			return procXpkSetError(objXpk, XPK_ERR_HASH, sXpkErrorHashMismatch);
		}
	}

	procXpkUnmapFile(&objMap);
	xrtClose(hFile);
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 逐条校验全部 Solid 原样条目
static inline int procXpkVerifyAllSolidStoredEntriesByEntry(xpkObject objXpk)
{
	uint32_t iPos;
	uint32_t iLiveCount;
	xpkEntry* pEntry;
	int iRet;

	iRet = procXpkVisibleEntryCountStrict(objXpk, &iLiveCount);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( iLiveCount == 0 ) {
		procXpkClearError(objXpk);
		return XPK_OK;
	}

	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objXpk, pEntry);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		iRet = procXpkVerifySolidEntry(objXpk, pEntry);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
	}

	procXpkClearError(objXpk);
	return XPK_OK;
}

// 校验全部条目
static inline int procXpkVerifyAllEntries(xpkObject objXpk)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	xpkWriteNode* pNode;
	xfile hFileData;
	xpkMappedFile objMapData;
	int iRet;

	// 先处理对象状态和 solid 专用校验路径，这些分支能避免通用逐条遍历。
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( objXpk->bSolidApplied ) {
		if ( procXpkSolidStoredCompLevel(objXpk) == 0 ) {
			if ( !procXpkAppliedVolumeMode(objXpk) ) {
				return procXpkVerifyAllSolidStoredEntriesMapped(objXpk);
			}
			return procXpkVerifyAllSolidStoredEntriesByEntry(objXpk);
		}
		if (
			procXpkCompLevelToAlg(procXpkSolidStoredCompLevel(objXpk)) == XPK_ALG_LZ4 ||
			procXpkCompLevelToAlg(procXpkSolidStoredCompLevel(objXpk)) == XPK_ALG_LZ4HC ||
			procXpkCompLevelToAlg(procXpkSolidStoredCompLevel(objXpk)) == XPK_ALG_ZSTD ||
			procXpkCompLevelToAlg(procXpkSolidStoredCompLevel(objXpk)) == XPK_ALG_LZMA2
		) {
			return procXpkVerifyAllSolidCompressedEntriesByEntry(objXpk);
		}
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	hFileData = NULL;
	memset(&objMapData, 0, sizeof(objMapData));

	// 普通布局下复用同一个数据文件句柄和映射视图，避免每个条目都重复打开文件。
	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			procXpkUnmapFile(&objMapData);
			if ( hFileData != NULL ) {
				xrtClose(hFileData);
			}
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objXpk, pEntry);
		if ( iRet != XPK_OK ) {
			procXpkUnmapFile(&objMapData);
			if ( hFileData != NULL ) {
				xrtClose(hFileData);
			}
			return iRet;
		}

		pNode = procXpkFindWriteNode(objXpk, pEntry->iPos, NULL);
		if ( pNode == NULL ) {
			iRet = procXpkVerifyAllEnsureDataFile(objXpk, &hFileData);
			if ( iRet != XPK_OK ) {
				procXpkUnmapFile(&objMapData);
				if ( hFileData != NULL ) {
					xrtClose(hFileData);
				}
				return iRet;
			}
		}

		if ( pNode == NULL && !procXpkAppliedVolumeMode(objXpk) && objMapData.pView == NULL ) {
			iRet = procXpkMapFileReadOnly(objXpk, hFileData, xrtGetEOF(hFileData), &objMapData);
			if ( iRet != XPK_OK ) {
				if ( hFileData != NULL ) {
					xrtClose(hFileData);
				}
				return iRet;
			}
		}

		// 按条目当前存储形态选择原样校验或解码校验路径。
		if ( (pEntry->iFlag & XPK_FLAG_COMP_MASK) == 0 ) {
			if ( pNode == NULL && objMapData.pView != NULL ) {
				iRet = procXpkVerifyStoredEntryMapped(objXpk, pEntry, &objMapData);
			} else {
				iRet = procXpkVerifyStoredEntryWithFile(objXpk, pEntry, hFileData);
			}
		} else {
			if ( pNode == NULL && objMapData.pView != NULL ) {
				iRet = procXpkVerifyDecodedEntryMapped(objXpk, pEntry, &objMapData);
			} else {
				iRet = procXpkVerifyDecodedEntryWithFile(objXpk, pEntry, hFileData);
			}
		}
		if ( iRet != XPK_OK ) {
			procXpkUnmapFile(&objMapData);
			if ( hFileData != NULL ) {
				xrtClose(hFileData);
			}
			return iRet;
		}
	}

	// 校验成功后统一释放共享资源并清理错误状态。
	procXpkUnmapFile(&objMapData);
	if ( hFileData != NULL ) {
		xrtClose(hFileData);
	}
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 计算当前元数据字节数
static inline int procXpkCalcCurrentMetaBytes(xpkObject objXpk, uint64_t* pSizeRet)
{
	void* pMetaComp;
	uint32_t iMetaCompSize;
	uint8_t iMetaLevel;
	int iRet;

	if ( pSizeRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	*pSizeRet = 0;
	pMetaComp = NULL;
	iMetaCompSize = 0;
	iMetaLevel = (uint8_t)objXpk->objHead.metaComp;
	iRet = procXpkCodecEncode(objXpk, (uint8_t)objXpk->objHead.metaComp, objXpk->pPackageMeta, objXpk->iPackageMetaSize, &pMetaComp, &iMetaCompSize, &iMetaLevel);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	if ( pMetaComp != NULL ) {
		xpkFreeInternal(pMetaComp);
	}
	*pSizeRet = iMetaCompSize;
	return XPK_OK;
}

// 计算当前条目字节数
static inline int procXpkCalcCurrentEntryBytes(xpkObject objXpk, uint64_t* pSizeRet)
{
	void* pEntryRaw;
	void* pEntryComp;
	uint32_t iEntryRawSize;
	uint32_t iEntryCompSize;
	uint8_t iInfoLevel;
	int iRet;

	if ( pSizeRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	*pSizeRet = 0;
	pEntryRaw = NULL;
	pEntryComp = NULL;
	iEntryRawSize = 0;
	iEntryCompSize = 0;
	iInfoLevel = (uint8_t)objXpk->objHead.infoComp;

	iRet = procXpkEncodeEntryTable(objXpk, &pEntryRaw, &iEntryRawSize);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = procXpkCodecEncode(objXpk, (uint8_t)objXpk->objHead.infoComp, pEntryRaw, iEntryRawSize, &pEntryComp, &iEntryCompSize, &iInfoLevel);
	if ( pEntryRaw != NULL ) {
		xpkFreeInternal(pEntryRaw);
	}
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	if ( pEntryComp != NULL ) {
		xpkFreeInternal(pEntryComp);
	}
	*pSizeRet = iEntryCompSize;
	return XPK_OK;
}

// 统计当前
static inline int procXpkStatCurrent(xpkObject objXpk, xpkStat* pStatRet)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	uint64_t iEndPos;
	uint64_t iDataBytes;
	uint64_t iDataRawMax;
	uint64_t iMetaBytes;
	uint64_t iEntryBytes;
	uint64_t iDataArea;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pStatRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkWriteQueueCount(objXpk) > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBufferedPending);
	}

	memset(pStatRet, 0, sizeof(*pStatRet));
	iDataBytes = 0;
	iDataRawMax = 0;
	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}

		if ( objXpk->bSolidApplied ) {
			iEndPos = pEntry->iDataOffset + pEntry->iFileSize;
			if ( iEndPos > iDataRawMax ) {
				iDataRawMax = iEndPos;
			}
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objXpk, pEntry);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		pStatRet->fileCount++;
		if ( objXpk->bSolidApplied ) {
			iDataBytes += pEntry->iFileSize;
		} else {
			iDataBytes += pEntry->iDataSize;
		}
	}

	iRet = procXpkCalcCurrentMetaBytes(objXpk, &iMetaBytes);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	iRet = procXpkCalcCurrentEntryBytes(objXpk, &iEntryBytes);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	if ( objXpk->bSolidApplied ) {
		pStatRet->liveDataBytes = iDataBytes;
		pStatRet->holeBytes = (iDataRawMax > iDataBytes) ? (iDataRawMax - iDataBytes) : 0;
	} else {
		iDataArea = 0;
		if ( objXpk->iAppendPos >= XPK_HEAD_SIZE ) {
			iDataArea = objXpk->iAppendPos - XPK_HEAD_SIZE;
		}

		pStatRet->liveDataBytes = iDataBytes;
		pStatRet->holeBytes = (iDataArea > iDataBytes) ? (iDataArea - iDataBytes) : 0;
	}
	pStatRet->metaBytes = iMetaBytes;
	pStatRet->entryTableBytes = iEntryBytes;
	procXpkClearError(objXpk);
	return XPK_OK;
}


/* ===== File: src/service/each.h ===== */

/*
	xPack 遍历服务模块

	负责按类型填充公开信息，并支持遍历与模式匹配。
*/

// 填充 Core 条目信息
static inline void procXpkFillInfoCore(const xpkEntry* pEntry, xpkFileInfo* pInfoRet)
{
	memset(pInfoRet, 0, sizeof(*pInfoRet));
	pInfoRet->flag = pEntry->iFlag;
	pInfoRet->fileHash = pEntry->iFileHash;
	pInfoRet->dataOffset = pEntry->iDataOffset;
	pInfoRet->dataSize = pEntry->iDataSize;
	pInfoRet->fileSize = pEntry->iFileSize;
}

// 填充索引条目信息
static inline void procXpkFillInfoIndex(const xpkEntry* pEntry, xpkFileInfoIndex* pInfoRet)
{
	memset(pInfoRet, 0, sizeof(*pInfoRet));
	pInfoRet->flag = pEntry->iFlag;
	pInfoRet->fileHash = pEntry->iFileHash;
	pInfoRet->dataOffset = pEntry->iDataOffset;
	pInfoRet->dataSize = pEntry->iDataSize;
	pInfoRet->fileSize = pEntry->iFileSize;
	pInfoRet->fileIndex = pEntry->iFileIndex;
}

// 填充信息路径
static inline void procXpkFillInfoPath(const xpkEntry* pEntry, xpkFileInfoPath* pInfoRet)
{
	size_t iPathLen;

	memset(pInfoRet, 0, sizeof(*pInfoRet));
	pInfoRet->flag = pEntry->iFlag;
	pInfoRet->fileHash = pEntry->iFileHash;
	pInfoRet->dataOffset = pEntry->iDataOffset;
	pInfoRet->dataSize = pEntry->iDataSize;
	pInfoRet->fileSize = pEntry->iFileSize;
	if ( pEntry->sPath != NULL ) {
		iPathLen = strlen(pEntry->sPath);
		if ( iPathLen >= XPK_PATH_BYTES ) {
			iPathLen = XPK_PATH_BYTES - 1;
		}
		memcpy(pInfoRet->pathBytes, pEntry->sPath, iPathLen);
	}
	pInfoRet->platformAttr = pEntry->iPlatformAttr;
	pInfoRet->createTime = pEntry->tCreateTime;
	pInfoRet->modifyTime = pEntry->tModifyTime;
	pInfoRet->accessTime = pEntry->tAccessTime;
}

// 条目匹配键
static inline const char* procXpkEntryMatchKey(xpkObject objXpk, const xpkEntry* pEntry, char sBuf[64], int* pCaseRet)
{
	if ( pCaseRet != NULL ) {
		*pCaseRet = FALSE;
	}

	if ( objXpk->objHead.packType == XPK_PACK_INDEX ) {
		snprintf(sBuf, 64, "%lld", (long long)pEntry->iFileIndex);
		return sBuf;
	}
	if ( (objXpk->objHead.packType == XPK_PACK_LINUX) || (objXpk->objHead.packType == XPK_PACK_WIN32) ) {
		if ( pCaseRet != NULL ) {
			*pCaseRet = (objXpk->objHead.packType == XPK_PACK_WIN32) ? TRUE : FALSE;
		}
		return pEntry->sPath;
	}

	snprintf(sBuf, 64, "%u", pEntry->iPos);
	return sBuf;
}

// 模式匹配条目
static inline int procXpkPatternMatchEntry(xpkObject objXpk, const xpkEntry* pEntry, const char* sPattern)
{
	char sBuf[64];
	const char* sKey;
	int bCase;

	if ( sPattern == NULL || sPattern[0] == '\0' ) {
		return TRUE;
	}

	sKey = procXpkEntryMatchKey(objXpk, pEntry, sBuf, &bCase);
	if ( sKey == NULL ) {
		return FALSE;
	}

	return xrtStrLike((str)sKey, 0, (str)sPattern, 0, bCase) ? TRUE : FALSE;
}

// 调用遍历回调
static inline int procXpkEachInvoke(xpkObject objXpk, const xpkEntry* pEntry, xpkEachProc procEach, void* pArg)
{
	xpkFileInfo objInfo;
	xpkFileInfoIndex objInfoIndex;
	xpkFileInfoPath objInfoPath;
	int* pCount;

	if ( procEach == NULL ) {
		pCount = (int*)pArg;
		if ( pCount != NULL ) {
			(*pCount)++;
		}
		return XPK_OK;
	}

	if ( objXpk->objHead.packType == XPK_PACK_INDEX ) {
		procXpkFillInfoIndex(pEntry, &objInfoIndex);
		return procEach(objXpk, pEntry->iPos, &objInfoIndex, pArg);
	}
	if ( (objXpk->objHead.packType == XPK_PACK_LINUX) || (objXpk->objHead.packType == XPK_PACK_WIN32) ) {
		procXpkFillInfoPath(pEntry, &objInfoPath);
		return procEach(objXpk, pEntry->iPos, &objInfoPath, pArg);
	}

	procXpkFillInfoCore(pEntry, &objInfo);
	return procEach(objXpk, pEntry->iPos, &objInfo, pArg);
}

// 遍历条目并执行回调
static inline int procXpkEachWalk(xpkObject objXpk, const char* sPattern, xpkEachProc procEach, void* pArg)
{
	uint32_t iPos;
	xpkEntry* pEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return XPK_ERR_PARAM;
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}

	for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( ((objXpk->objHead.packType == XPK_PACK_LINUX) || (objXpk->objHead.packType == XPK_PACK_WIN32)) &&
			((pEntry->sPath == NULL) || (pEntry->sPath[0] == '\0')) ) {
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}
		if ( procXpkEntryDeleted(pEntry) ) {
			continue;
		}
		iRet = procXpkValidateLiveEntryLookup(objXpk, pEntry);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		if ( !procXpkPatternMatchEntry(objXpk, pEntry, sPattern) ) {
			continue;
		}

		procXpkClearError(objXpk);
		iRet = procXpkEachInvoke(objXpk, pEntry, procEach, pArg);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
	}

	procXpkClearError(objXpk);
	return XPK_OK;
}


/* ===== File: src/api/package_api.h ===== */

/*
	xPack 包级 API 实现

	负责包对象生命周期、全局配置和元数据接口。
*/

// 确保包对象处于可修改状态
static inline int procXpkEnsurePackageMutable(xpkObject objXpk)
{
	int iRet;

	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	return XPK_OK;
}

// 打开包对象
XPKAPI xpkObject xpkOpen(const char* sPackagePath, const xpkOpenOptions* pOpt)
{
	xpkObject objXpk;
	int iRet;

	if ( sPackagePath == NULL || sPackagePath[0] == '\0' ) {
		procXpkSetError(NULL, XPK_ERR_PARAM, sXpkErrorInvalidParam);
		return NULL;
	}

	objXpk = (xpkObject)xpkAllocInternal(sizeof(*objXpk));
	if ( objXpk == NULL ) {
		procXpkSetError(NULL, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		return NULL;
	}

	procXpkInitObject(objXpk, pOpt);
	objXpk->sPathPackage = procXpkDupText(sPackagePath);
	if ( objXpk->sPathPackage == NULL ) {
		procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		procXpkUnitObject(objXpk);
		xpkFreeInternal(objXpk);
		return NULL;
	}

	iRet = procXpkLoadPackage(objXpk, pOpt);
	if ( iRet != XPK_OK ) {
		procXpkUnitObject(objXpk);
		xpkFreeInternal(objXpk);
		return NULL;
	}

	return objXpk;
}

// 关闭包对象
XPKAPI int xpkClose(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}

	procXpkClearError(objXpk);
	procXpkUnitWriteQueue(objXpk);
	procXpkUnitObject(objXpk);
	xpkFreeInternal(objXpk);
	return XPK_OK;
}

// 保存当前包状态
XPKAPI int xpkSave(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	return procXpkSavePackage(objXpk);
}

// 按目标布局重构包文件
XPKAPI int xpkBuild(xpkObject objXpk, const xpkBuildOptions* pOpt)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	return procXpkBuildPackage(objXpk, pOpt);
}

// 获取包类型
XPKAPI int xpkGetPackType(xpkObject objXpk, xpkPackType* pTypeRet)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pTypeRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	*pTypeRet = (xpkPackType)objXpk->objHead.packType;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 设置包类型
XPKAPI int xpkSetPackType(xpkObject objXpk, xpkPackType iType)
{
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( (iType < XPK_PACK_CORE) || (iType > XPK_PACK_WIN32) ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsurePackageMutable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( objXpk->objHead.packType == (uint32_t)iType ) {
		procXpkClearError(objXpk);
		return XPK_OK;
	}
	if ( objXpk->arrEntry.Count != 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeLocked);
	}

	procXpkApplyPackType(objXpk, iType);
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 获取默认压缩级别
XPKAPI int xpkGetDefaultComp(xpkObject objXpk, uint8_t* pLevelRet)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pLevelRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	*pLevelRet = (uint8_t)objXpk->objHead.defComp;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 设置默认压缩级别
XPKAPI int xpkSetDefaultComp(xpkObject objXpk, uint8_t iLevel)
{
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iLevel > 15 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsurePackageMutable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	objXpk->objHead.defComp = iLevel;
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 获取元数据压缩级别
XPKAPI int xpkGetMetaComp(xpkObject objXpk, uint8_t* pLevelRet)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pLevelRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	*pLevelRet = (uint8_t)objXpk->objHead.metaComp;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 设置元数据压缩级别
XPKAPI int xpkSetMetaComp(xpkObject objXpk, uint8_t iLevel)
{
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iLevel > 15 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsurePackageMutable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	objXpk->objHead.metaComp = iLevel;
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 获取条目表压缩级别
XPKAPI int xpkGetInfoComp(xpkObject objXpk, uint8_t* pLevelRet)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pLevelRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	*pLevelRet = (uint8_t)objXpk->objHead.infoComp;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 设置条目表压缩级别
XPKAPI int xpkSetInfoComp(xpkObject objXpk, uint8_t iLevel)
{
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iLevel > 15 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsurePackageMutable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	objXpk->objHead.infoComp = iLevel;
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 获取信息扩展大小
XPKAPI int xpkGetInfoExtSize(xpkObject objXpk, uint32_t* pSizeRet)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pSizeRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	*pSizeRet = objXpk->objHead.infoExtSize;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 设置信息扩展大小
XPKAPI int xpkSetInfoExtSize(xpkObject objXpk, uint32_t iSize)
{
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iSize > 0x3FFFFu ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( objXpk->objHead.packType != XPK_PACK_CORE ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	iRet = procXpkEnsurePackageMutable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( objXpk->objHead.infoExtSize == iSize ) {
		procXpkClearError(objXpk);
		return XPK_OK;
	}
	if ( objXpk->arrEntry.Count != 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorInfoExtLocked);
	}

	objXpk->objHead.infoExtSize = iSize;
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 获取分卷大小
XPKAPI int xpkGetVolumeSize(xpkObject objXpk, uint32_t* pSizeRet)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pSizeRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	*pSizeRet = objXpk->objHead.volumeSize;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 设置分卷大小
XPKAPI int xpkSetVolumeSize(xpkObject objXpk, uint32_t iSize)
{
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iSize != 0 && iSize < XPK_VOLUME_MIN ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, "volume size is below minimum 64KB");
	}
	iRet = procXpkEnsurePackageMutable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	objXpk->objHead.volumeSize = iSize;
	objXpk->objHead.volumeMode = (iSize != 0) ? 1 : 0;
	if ( procXpkCanAdoptTargetLayout(objXpk) ) {
		objXpk->bVolumeApplied = objXpk->objHead.volumeMode ? TRUE : FALSE;
		objXpk->iVolumeSizeApplied = objXpk->objHead.volumeSize;
	}
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 获取 Solid 模式
XPKAPI int xpkGetSolidMode(xpkObject objXpk, int* pEnabledRet)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pEnabledRet == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	*pEnabledRet = objXpk->objHead.solidMode ? TRUE : FALSE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 设置 Solid 模式
XPKAPI int xpkSetSolidMode(xpkObject objXpk, int bEnabled)
{
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	iRet = procXpkEnsurePackageMutable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	objXpk->objHead.solidMode = bEnabled ? 1 : 0;
	if ( procXpkCanAdoptTargetLayout(objXpk) ) {
		objXpk->bSolidApplied = objXpk->objHead.solidMode ? TRUE : FALSE;
	}
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 获取包元数据
XPKAPI void* xpkMetaGet(xpkObject objXpk, uint32_t* pSizeRet)
{
	void* pRet;

	if ( pSizeRet ) {
		*pSizeRet = 0;
	}
	if ( objXpk == NULL ) {
		procXpkSetError(NULL, XPK_ERR_PARAM, sXpkErrorInvalidParam);
		return NULL;
	}
	if ( objXpk->pPackageMeta == NULL || objXpk->iPackageMetaSize == 0 ) {
		procXpkClearError(objXpk);
		return NULL;
	}

	pRet = xpkAllocInternal(objXpk->iPackageMetaSize);
	if ( pRet == NULL ) {
		procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		return NULL;
	}
	memcpy(pRet, objXpk->pPackageMeta, objXpk->iPackageMetaSize);
	if ( pSizeRet ) {
		*pSizeRet = objXpk->iPackageMetaSize;
	}
	procXpkClearError(objXpk);
	return pRet;
}

// 设置包元数据
XPKAPI int xpkMetaSet(xpkObject objXpk, const void* pData, uint32_t iSize, uint8_t iCompLevel)
{
	void* pMetaNew;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iCompLevel > 15 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize > 0 && pData == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsurePackageMutable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	pMetaNew = NULL;
	if ( iSize > 0 ) {
		pMetaNew = xpkAllocInternal(iSize);
		if ( pMetaNew == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		memcpy(pMetaNew, pData, iSize);
	}

	if ( objXpk->pPackageMeta ) {
		xpkFreeInternal(objXpk->pPackageMeta);
	}
	objXpk->pPackageMeta = pMetaNew;
	objXpk->iPackageMetaSize = iSize;
	objXpk->objHead.metaComp = iCompLevel;
	objXpk->bDirtyPackageMeta = TRUE;
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 清空包元数据
XPKAPI int xpkMetaClear(xpkObject objXpk)
{
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	iRet = procXpkEnsurePackageMutable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( objXpk->pPackageMeta ) {
		xpkFreeInternal(objXpk->pPackageMeta);
	}
	objXpk->pPackageMeta = NULL;
	objXpk->iPackageMetaSize = 0;
	objXpk->bDirtyPackageMeta = TRUE;
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}


/* ===== File: src/api/default_api.h ===== */

/*
	xPack 默认位置 API 实现

	负责按位置访问条目的增删改查与读写。
*/

// 移除未保存条目
static inline int procXpkRemoveUnsavedEntry(xpkObject objXpk, uint32_t iPos)
{
	xpkEntry* pEntry;
	xpkEntry objEntry;
	uint32_t iInsertPos;
	int iRet;

	pEntry = procXpkGetEntryByPos(objXpk, iPos);
	if ( pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}

	objEntry = *pEntry;
	if ( !xrtArrayRemove(&objXpk->arrEntry, iPos, 1) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}
	objXpk->iEntryCount = objXpk->arrEntry.Count;

	iRet = procXpkRebuildLookup(objXpk);
	if ( iRet != XPK_OK ) {
		iInsertPos = xrtArrayInsert(&objXpk->arrEntry, iPos - 1, 1);
		if ( iInsertPos != iPos ) {
			procXpkFreeEntryOwned(&objEntry);
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}

		pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
		if ( pEntry == NULL ) {
			procXpkFreeEntryOwned(&objEntry);
			return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		}

		memset(pEntry, 0, sizeof(*pEntry));
		*pEntry = objEntry;
		objEntry.pInfoExt = NULL;
		objEntry.sPath = NULL;
		objXpk->iEntryCount = objXpk->arrEntry.Count;
		procXpkRebuildLookup(objXpk);
		return iRet;
	}

	procXpkFreeEntryOwned(&objEntry);
	procXpkDropQueuedWrite(objXpk, iPos);
	procXpkShiftQueuedWritePos(objXpk, iPos);
	return XPK_OK;
}

// 添加内存数据条目
static inline int procXpkAddDataEntry(xpkObject objXpk, xpkEntry* pEntrySeed, const void* pData, uint64_t iSize, const xpkWriteOptions* pOpt, uint32_t* pPosRet)
{
	xpkEntry objEntry;
	xpkEntry* pEntry;
	int iRet;

	if ( pPosRet != NULL ) {
		*pPosRet = 0;
	}

	// 先校验对象状态、写权限和输入参数，避免后面进入半完成状态。
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	// 根据种子条目复制可继承字段，并按包类型补齐扩展信息缓冲。
	memset(&objEntry, 0, sizeof(objEntry));
	if ( pEntrySeed != NULL ) {
		objEntry = *pEntrySeed;
		if ( pEntrySeed->pInfoExt != NULL ) {
			objEntry.pInfoExt = procXpkDupInfoExt(objXpk, pEntrySeed->pInfoExt);
			if ( objEntry.pInfoExt == NULL ) {
				return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			}
		} else if ( procXpkCoreInfoExtEnabled(objXpk) ) {
			objEntry.pInfoExt = procXpkAllocInfoExt(objXpk);
			if ( objEntry.pInfoExt == NULL ) {
				return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			}
		}
		if ( pEntrySeed->sPath != NULL ) {
			objEntry.sPath = procXpkDupText(pEntrySeed->sPath);
			if ( objEntry.sPath == NULL ) {
				if ( objEntry.pInfoExt != NULL ) {
					xpkFreeInternal(objEntry.pInfoExt);
				}
				return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			}
		}
	} else if ( procXpkCoreInfoExtEnabled(objXpk) ) {
		objEntry.pInfoExt = procXpkAllocInfoExt(objXpk);
		if ( objEntry.pInfoExt == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
	}

	if ( pOpt != NULL ) {
		objEntry.iFlag = (objEntry.iFlag & ~XPK_FLAG_TYPE_MASK) | (((uint32_t)pOpt->fileType & 0x0Fu) << 4);
	}

	// 先把条目挂到数组里，再写入真实数据；这样出错时可以统一走回滚路径。
	iRet = procXpkAppendEntryOwned(objXpk, &objEntry);
	if ( iRet != XPK_OK ) {
		procXpkFreeEntryOwned(&objEntry);
		return iRet;
	}

	pEntry = procXpkGetEntryByPos(objXpk, objXpk->iEntryCount);
	if ( pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	iRet = procXpkStoreEntryData(objXpk, pEntry, pData, iSize, pOpt);
	if ( iRet != XPK_OK ) {
		procXpkRemoveUnsavedEntry(objXpk, pEntry->iPos);
		return iRet;
	}

	// 数据落盘成功后重建查找表，确保位置、路径和索引视图保持一致。
	iRet = procXpkRebuildLookup(objXpk);
	if ( iRet != XPK_OK ) {
		procXpkRemoveUnsavedEntry(objXpk, pEntry->iPos);
		return iRet;
	}
	if ( pPosRet != NULL ) {
		*pPosRet = pEntry->iPos;
	}
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 写入文件来源条目
static inline int procXpkStoreEntryFile(xpkObject objXpk, xpkEntry* pEntry, const char* sSrcPath, const xpkWriteOptions* pOpt)
{
	xfile hFile;
	xpkMappedFile objMap;
	uint64_t iSize;
	uint8_t iLevel;
	uint8_t iWritePolicy;
	int iRet;

	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	// 先解析压缩级别和写入策略，后面的分发完全由这两个维度决定。
	iRet = procXpkResolveCompLevel(objXpk, pOpt, &iLevel);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	iWritePolicy = procXpkResolveWritePolicy(objXpk, pOpt);

	// 根据算法和写入策略分发到专用实现，避免所有路径都退回到单一的大内存方案。
	if ( iLevel == 0 && iWritePolicy == XPK_WRITE_IMMEDIATE ) {
		iRet = procXpkWriteImmediateStoreFile(objXpk, pEntry, sSrcPath);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		objXpk->bDirtyData = TRUE;
		procXpkMarkDirtyEntryTable(objXpk);
		objXpk->objHead.changeTime = xpkNowInternal();
		procXpkClearError(objXpk);
		return XPK_OK;
	}
	if ( (procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZ4 || procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZ4HC) && iWritePolicy == XPK_WRITE_IMMEDIATE ) {
		iRet = procXpkWriteImmediateLz4File(objXpk, pEntry, sSrcPath, iLevel);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		objXpk->bDirtyData = TRUE;
		procXpkMarkDirtyEntryTable(objXpk);
		objXpk->objHead.changeTime = xpkNowInternal();
		procXpkClearError(objXpk);
		return XPK_OK;
	}
	if ( (procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZ4 || procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZ4HC) && iWritePolicy == XPK_WRITE_BUFFERED ) {
		iRet = procXpkWriteBufferedLz4File(objXpk, pEntry, sSrcPath, iLevel);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		objXpk->bDirtyData = TRUE;
		objXpk->bDirtyEntryTable = TRUE;
		objXpk->bDirtyHead = TRUE;
		objXpk->objHead.changeTime = xpkNowInternal();
		procXpkClearError(objXpk);
		return XPK_OK;
	}
	if ( procXpkCompLevelToAlg(iLevel) == XPK_ALG_ZSTD && iWritePolicy == XPK_WRITE_IMMEDIATE ) {
		iRet = procXpkWriteImmediateZstdFile(objXpk, pEntry, sSrcPath, iLevel);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		objXpk->bDirtyData = TRUE;
		procXpkMarkDirtyEntryTable(objXpk);
		objXpk->objHead.changeTime = xpkNowInternal();
		procXpkClearError(objXpk);
		return XPK_OK;
	}
	if ( procXpkCompLevelToAlg(iLevel) == XPK_ALG_ZSTD && iWritePolicy == XPK_WRITE_BUFFERED ) {
		iRet = procXpkWriteBufferedZstdFile(objXpk, pEntry, sSrcPath, iLevel);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		objXpk->bDirtyData = TRUE;
		objXpk->bDirtyEntryTable = TRUE;
		objXpk->bDirtyHead = TRUE;
		objXpk->objHead.changeTime = xpkNowInternal();
		procXpkClearError(objXpk);
		return XPK_OK;
	}
	if ( procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZMA2 && iWritePolicy == XPK_WRITE_IMMEDIATE ) {
		iRet = procXpkWriteImmediateLzma2File(objXpk, pEntry, sSrcPath, iLevel);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		objXpk->bDirtyData = TRUE;
		procXpkMarkDirtyEntryTable(objXpk);
		objXpk->objHead.changeTime = xpkNowInternal();
		procXpkClearError(objXpk);
		return XPK_OK;
	}
	if ( procXpkCompLevelToAlg(iLevel) == XPK_ALG_LZMA2 && iWritePolicy == XPK_WRITE_BUFFERED ) {
		iRet = procXpkWriteBufferedLzma2File(objXpk, pEntry, sSrcPath, iLevel);
		if ( iRet != XPK_OK ) {
			return iRet;
		}

		objXpk->bDirtyData = TRUE;
		objXpk->bDirtyEntryTable = TRUE;
		objXpk->bDirtyHead = TRUE;
		objXpk->objHead.changeTime = xpkNowInternal();
		procXpkClearError(objXpk);
		return XPK_OK;
	}

	hFile = NULL;
	memset(&objMap, 0, sizeof(objMap));
	iSize = 0;

	// 统一回落到“源文件映射 -> 通用写入入口”路径，覆盖剩余组合分支。
	iRet = procXpkOpenMappedSourceFile(objXpk, sSrcPath, &hFile, &iSize, &objMap);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = procXpkStoreEntryData(objXpk, pEntry, objMap.pView, iSize, pOpt);
	procXpkUnmapFile(&objMap);
	xrtClose(hFile);
	return iRet;
}

// 添加文件来源条目
static inline int procXpkAddFileEntry(xpkObject objXpk, xpkEntry* pEntrySeed, const char* sSrcPath, const xpkWriteOptions* pOpt, uint32_t* pPosRet)
{
	xpkEntry objEntry;
	xpkEntry* pEntry;
	int iRet;

	if ( pPosRet != NULL ) {
		*pPosRet = 0;
	}
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}

	memset(&objEntry, 0, sizeof(objEntry));
	if ( pEntrySeed != NULL ) {
		objEntry = *pEntrySeed;
		if ( pEntrySeed->pInfoExt != NULL ) {
			objEntry.pInfoExt = procXpkDupInfoExt(objXpk, pEntrySeed->pInfoExt);
			if ( objEntry.pInfoExt == NULL ) {
				return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			}
		} else if ( procXpkCoreInfoExtEnabled(objXpk) ) {
			objEntry.pInfoExt = procXpkAllocInfoExt(objXpk);
			if ( objEntry.pInfoExt == NULL ) {
				return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			}
		}
		if ( pEntrySeed->sPath != NULL ) {
			objEntry.sPath = procXpkDupText(pEntrySeed->sPath);
			if ( objEntry.sPath == NULL ) {
				if ( objEntry.pInfoExt != NULL ) {
					xpkFreeInternal(objEntry.pInfoExt);
				}
				return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
			}
		}
	} else if ( procXpkCoreInfoExtEnabled(objXpk) ) {
		objEntry.pInfoExt = procXpkAllocInfoExt(objXpk);
		if ( objEntry.pInfoExt == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
	}

	if ( pOpt != NULL ) {
		objEntry.iFlag = (objEntry.iFlag & ~XPK_FLAG_TYPE_MASK) | (((uint32_t)pOpt->fileType & 0x0Fu) << 4);
	}

	iRet = procXpkAppendEntryOwned(objXpk, &objEntry);
	if ( iRet != XPK_OK ) {
		procXpkFreeEntryOwned(&objEntry);
		return iRet;
	}

	pEntry = procXpkGetEntryByPos(objXpk, objXpk->iEntryCount);
	if ( pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
	}

	iRet = procXpkStoreEntryFile(objXpk, pEntry, sSrcPath, pOpt);
	if ( iRet != XPK_OK ) {
		procXpkRemoveUnsavedEntry(objXpk, pEntry->iPos);
		return iRet;
	}

	iRet = procXpkRebuildLookup(objXpk);
	if ( iRet != XPK_OK ) {
		procXpkRemoveUnsavedEntry(objXpk, pEntry->iPos);
		return iRet;
	}
	if ( pPosRet != NULL ) {
		*pPosRet = pEntry->iPos;
	}
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 更新内存数据条目
static inline int procXpkUpdateEntryData(xpkObject objXpk, xpkEntry* pEntry, const void* pData, uint64_t iSize, const xpkWriteOptions* pOpt)
{
	uint32_t iFlagOld;
	int iRet;

	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}

	iFlagOld = pEntry->iFlag;
	if ( pOpt != NULL ) {
		pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_TYPE_MASK) | (((uint32_t)pOpt->fileType & 0x0Fu) << 4);
	}
	iRet = procXpkStoreEntryData(objXpk, pEntry, pData, iSize, pOpt);
	if ( iRet != XPK_OK ) {
		pEntry->iFlag = iFlagOld;
		return iRet;
	}
	return XPK_OK;
}

// 更新文件来源条目
static inline int procXpkUpdateEntryFile(xpkObject objXpk, xpkEntry* pEntry, const char* sSrcPath, const xpkWriteOptions* pOpt)
{
	uint32_t iFlagOld;
	int iRet;

	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( pEntry == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}

	iFlagOld = pEntry->iFlag;
	if ( pOpt != NULL ) {
		pEntry->iFlag = (pEntry->iFlag & ~XPK_FLAG_TYPE_MASK) | (((uint32_t)pOpt->fileType & 0x0Fu) << 4);
	}
	iRet = procXpkStoreEntryFile(objXpk, pEntry, sSrcPath, pOpt);
	if ( iRet != XPK_OK ) {
		pEntry->iFlag = iFlagOld;
		return iRet;
	}
	return XPK_OK;
}

// 校验公开位置访问是否合法
static inline int procXpkValidatePublicPosAccess(xpkObject objXpk, xpkEntry* pEntry)
{
	if ( objXpk == NULL || pEntry == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return XPK_OK;
	}
	return procXpkValidateLiveEntryLookup(objXpk, pEntry);
}

// 获取可见条目数量
XPKAPI uint32_t xpkCount(xpkObject objXpk)
{
	uint32_t iCount;

	if ( objXpk == NULL ) {
		procXpkSetError(NULL, XPK_ERR_PARAM, sXpkErrorInvalidParam);
		return 0;
	}
	if ( procXpkVisibleEntryCountStrict(objXpk, &iCount) != XPK_OK ) {
		return 0;
	}
	return iCount;
}

// 获取位置条目信息
XPKAPI int xpkGetInfo(xpkObject objXpk, uint32_t iPos, xpkFileInfo* pInfoRet)
{
	xpkEntry* pEntry;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pInfoRet == NULL || iPos == 0 ) {
		return procXpkReturnParamError(objXpk);
	}

	if ( procXpkGetPublicEntryByPos(objXpk, iPos, &pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( procXpkValidatePublicPosAccess(objXpk, pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}

	memset(pInfoRet, 0, sizeof(*pInfoRet));
	pInfoRet->flag = pEntry->iFlag;
	pInfoRet->fileHash = pEntry->iFileHash;
	pInfoRet->dataOffset = pEntry->iDataOffset;
	pInfoRet->dataSize = pEntry->iDataSize;
	pInfoRet->fileSize = pEntry->iFileSize;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 获取位置条目扩展信息
XPKAPI int xpkGetInfoExt(xpkObject objXpk, uint32_t iPos, void* pDataRet, uint32_t iSize)
{
	xpkEntry* pEntry;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( !procXpkCoreInfoExtEnabled(objXpk) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorCoreExtUnsupported);
	}
	if ( iPos == 0 ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iSize > 0 && pDataRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize != objXpk->objHead.infoExtSize ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInfoExtSizeMismatch);
	}

	if ( procXpkGetPublicEntryByPos(objXpk, iPos, &pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}

	if ( pEntry->pInfoExt == NULL ) {
		memset(pDataRet, 0, iSize);
	} else if ( iSize > 0 ) {
		memcpy(pDataRet, pEntry->pInfoExt, iSize);
	}
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 设置位置条目扩展信息
XPKAPI int xpkSetInfoExt(xpkObject objXpk, uint32_t iPos, const void* pData, uint32_t iSize)
{
	xpkEntry* pEntry;
	void* pInfoExt;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( !procXpkCoreInfoExtEnabled(objXpk) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorCoreExtUnsupported);
	}
	if ( iPos == 0 ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iSize > 0 && pData == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( iSize != objXpk->objHead.infoExtSize ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInfoExtSizeMismatch);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	if ( procXpkGetPublicEntryByPos(objXpk, iPos, &pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}

	if ( pEntry->pInfoExt == NULL ) {
		pInfoExt = procXpkAllocInfoExt(objXpk);
		if ( pInfoExt == NULL ) {
			return procXpkSetError(objXpk, XPK_ERR_MEMORY, sXpkErrorOutOfMemory);
		}
		pEntry->pInfoExt = pInfoExt;
	}

	if ( iSize > 0 ) {
		memcpy(pEntry->pInfoExt, pData, iSize);
	}
	objXpk->bDirtyEntryTable = TRUE;
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 按位置添加文件条目
XPKAPI int xpkAddFile(xpkObject objXpk, const char* sSrcPath, const xpkWriteOptions* pOpt, uint32_t* pPosRet)
{
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( objXpk->objHead.packType != XPK_PACK_CORE ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	return procXpkAddFileEntry(objXpk, NULL, sSrcPath, pOpt, pPosRet);
}

// 按位置添加内存数据条目
XPKAPI int xpkAddData(xpkObject objXpk, const void* pData, uint64_t iSize, const xpkWriteOptions* pOpt, uint32_t* pPosRet)
{
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( objXpk->objHead.packType != XPK_PACK_CORE ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	return procXpkAddDataEntry(objXpk, NULL, pData, iSize, pOpt, pPosRet);
}

// 按位置读取条目到文件
XPKAPI int xpkReadToFile(xpkObject objXpk, uint32_t iPos, const char* sDstPath)
{
	xpkEntry* pEntry;
	xpkWriteNode* pNode;
	void* pData;
	uint64_t iSize;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sDstPath == NULL || sDstPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	if ( procXpkGetPublicEntryByPos(objXpk, iPos, &pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkValidatePublicPosAccess(objXpk, pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( objXpk->bSolidApplied ) {
		return procXpkCopySolidEntryToFile(objXpk, pEntry, sDstPath);
	}
	if ( !objXpk->bSolidApplied ) {
		pNode = procXpkFindWriteNode(objXpk, pEntry->iPos, NULL);
		if ( (pNode != NULL) && ((procXpkCompLevelToAlg(pNode->iLevel) == XPK_ALG_LZ4) || (procXpkCompLevelToAlg(pNode->iLevel) == XPK_ALG_LZ4HC)) ) {
			return procXpkCopyQueuedLz4EntryToFile(objXpk, pEntry, sDstPath);
		}
		if ( (pNode != NULL) && (procXpkCompLevelToAlg(pNode->iLevel) == XPK_ALG_ZSTD) ) {
			return procXpkCopyQueuedZstdEntryToFile(objXpk, pEntry, sDstPath);
		}
		if ( (pNode != NULL) && (procXpkCompLevelToAlg(pNode->iLevel) == XPK_ALG_LZMA2) ) {
			return procXpkCopyQueuedLzma2EntryToFile(objXpk, pEntry, sDstPath);
		}
		if ( (pNode != NULL) && (pNode->iLevel == 0) ) {
			return procXpkCopyQueuedStoredEntryToFile(objXpk, pEntry, sDstPath);
		}
		if ( (pNode == NULL) && ((pEntry->iFlag & XPK_FLAG_COMP_MASK) == 0) ) {
			return procXpkCopyStoredEntryToFile(objXpk, pEntry, sDstPath);
		}
		if ( (pNode == NULL) && ((procXpkCompLevelToAlg((uint8_t)(pEntry->iFlag & XPK_FLAG_COMP_MASK)) == XPK_ALG_LZ4) || (procXpkCompLevelToAlg((uint8_t)(pEntry->iFlag & XPK_FLAG_COMP_MASK)) == XPK_ALG_LZ4HC)) ) {
			return procXpkCopyStoredLz4EntryToFile(objXpk, pEntry, sDstPath);
		}
		if ( (pNode == NULL) && (procXpkCompLevelToAlg((uint8_t)(pEntry->iFlag & XPK_FLAG_COMP_MASK)) == XPK_ALG_ZSTD) ) {
			return procXpkCopyStoredZstdEntryToFile(objXpk, pEntry, sDstPath);
		}
		if ( (pNode == NULL) && (procXpkCompLevelToAlg((uint8_t)(pEntry->iFlag & XPK_FLAG_COMP_MASK)) == XPK_ALG_LZMA2) ) {
			return procXpkCopyStoredLzma2EntryToFile(objXpk, pEntry, sDstPath);
		}
	}

	pData = procXpkReadEntryData(objXpk, pEntry, &iSize);
	if ( pData == NULL ) {
		return xpkLastError(objXpk);
	}

	iRet = procXpkWriteFileData(objXpk, sDstPath, pData, iSize);
	xpkFree(pData);
	return iRet;
}

// 按位置读取条目到内存
XPKAPI void* xpkReadToMemory(xpkObject objXpk, uint32_t iPos, uint64_t* pSizeRet)
{
	xpkEntry* pEntry;

	if ( pSizeRet != NULL ) {
		*pSizeRet = 0;
	}
	if ( objXpk == NULL ) {
		procXpkSetError(NULL, XPK_ERR_PARAM, sXpkErrorInvalidParam);
		return NULL;
	}
	if ( iPos == 0 ) {
		procXpkSetParamErrorIfObject(objXpk);
		return NULL;
	}

	if ( procXpkGetPublicEntryByPos(objXpk, iPos, &pEntry) != XPK_OK ) {
		return NULL;
	}
	if ( procXpkValidatePublicPosAccess(objXpk, pEntry) != XPK_OK ) {
		return NULL;
	}
	return procXpkReadEntryData(objXpk, pEntry, pSizeRet);
}

// 按位置更新文件条目
XPKAPI int xpkUpdateFile(xpkObject objXpk, uint32_t iPos, const char* sSrcPath, const xpkWriteOptions* pOpt)
{
	xpkEntry* pEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iPos == 0 ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( procXpkGetPublicEntryByPos(objXpk, iPos, &pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkValidatePublicPosAccess(objXpk, pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	return procXpkUpdateEntryFile(objXpk, pEntry, sSrcPath, pOpt);
}

// 按位置更新内存数据条目
XPKAPI int xpkUpdateData(xpkObject objXpk, uint32_t iPos, const void* pData, uint64_t iSize, const xpkWriteOptions* pOpt)
{
	xpkEntry* pEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iPos == 0 ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	if ( procXpkGetPublicEntryByPos(objXpk, iPos, &pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkValidatePublicPosAccess(objXpk, pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	return procXpkUpdateEntryData(objXpk, pEntry, pData, iSize, pOpt);
}

// 按位置移除条目
XPKAPI int xpkRemove(xpkObject objXpk, uint32_t iPos)
{
	xpkEntry* pEntry;
	uint32_t iFlagOld;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iPos == 0 ) {
		return procXpkReturnParamError(objXpk);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	if ( procXpkGetPublicEntryByPos(objXpk, iPos, &pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkEntryDeleted(pEntry) ) {
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorDeleted);
	}
	if ( procXpkValidatePublicPosAccess(objXpk, pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( !pEntry->bStored ) {
		iRet = procXpkRemoveUnsavedEntry(objXpk, iPos);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		objXpk->bDirtyEntryTable = TRUE;
		objXpk->bDirtyHead = TRUE;
		procXpkClearError(objXpk);
		return XPK_OK;
	}

	iFlagOld = pEntry->iFlag;
	pEntry->iFlag |= XPK_FLAG_DELETED_MASK;
	iRet = procXpkRebuildLookup(objXpk);
	if ( iRet != XPK_OK ) {
		pEntry->iFlag = iFlagOld;
		procXpkRebuildLookup(objXpk);
		return iRet;
	}
	procXpkDropQueuedWrite(objXpk, iPos);
	objXpk->bDirtyEntryTable = TRUE;
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 按位置设置条目标记
XPKAPI int xpkSetFlag(xpkObject objXpk, uint32_t iPos, uint32_t iMask, uint32_t iValue)
{
	xpkEntry* pEntry;
	uint32_t iFlagOld;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iPos == 0 ) {
		return procXpkReturnParamError(objXpk);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	if ( procXpkGetPublicEntryByPos(objXpk, iPos, &pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkValidatePublicPosAccess(objXpk, pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( (iMask & XPK_FLAG_COMP_MASK) != 0 ) {
		if ( !procXpkEntryDeleted(pEntry) ) {
			if ( ((pEntry->iFlag ^ iValue) & XPK_FLAG_COMP_MASK) != 0 ) {
				return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorCompManaged);
			}
		}
	}

	iFlagOld = pEntry->iFlag;
	pEntry->iFlag = (pEntry->iFlag & ~iMask) | (iValue & iMask);
	if ( (iMask & XPK_FLAG_DELETED_MASK) != 0 ) {
		iRet = procXpkRebuildLookup(objXpk);
		if ( iRet != XPK_OK ) {
			pEntry->iFlag = iFlagOld;
			procXpkRebuildLookup(objXpk);
			return iRet;
		}
	}
	objXpk->bDirtyEntryTable = TRUE;
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}


/* ===== File: src/api/index_api.h ===== */

/*
	xPack 索引 API 实现

	负责按 fileIndex 访问条目的增删改查。
*/

// 查找索引条目
static inline xpkEntry* procXpkLookupIndexEntry(xpkObject objXpk, int64_t iFileIndex)
{
	uint32_t iPos;
	xpkEntry* pMap;
	xpkEntry* pEntry;

	if ( objXpk == NULL ) {
		return NULL;
	}

	procXpkClearError(objXpk);
	if ( procXpkValidateEntryCountState(objXpk) != XPK_OK ) {
		return NULL;
	}
	pMap = (xpkEntry*)xrtListGet(&objXpk->lstEntry, iFileIndex);
	if ( pMap == NULL ) {
		for ( iPos = 1; iPos <= objXpk->iEntryCount; iPos++ ) {
			pEntry = (xpkEntry*)xrtArrayGet(&objXpk->arrEntry, iPos);
			if ( pEntry == NULL ) {
				procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
				return NULL;
			}
			if ( procXpkEntryDeleted(pEntry) ) {
				continue;
			}
			if ( pEntry->iFileIndex == iFileIndex ) {
				procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
				return NULL;
			}
		}
		return NULL;
	}

	pEntry = procXpkGetEntryByPos(objXpk, pMap->iPos);
	if ( pEntry == NULL || procXpkEntryDeleted(pEntry) || pEntry->iFileIndex != iFileIndex ) {
		procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorBadFormat);
		return NULL;
	}
	return pEntry;
}

// 按索引查找条目位置
XPKAPI int xpkIndexFind(xpkObject objXpk, int64_t iFileIndex, uint32_t* pPosRet)
{
	xpkEntry* pEntry;
	int iRet;

	if ( pPosRet != NULL ) {
		*pPosRet = 0;
	}
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( objXpk->objHead.packType != XPK_PACK_INDEX ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}

	pEntry = procXpkLookupIndexEntry(objXpk, iFileIndex);
	if ( pEntry == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}

	if ( pPosRet != NULL ) {
		*pPosRet = pEntry->iPos;
	}
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 按索引获取条目信息
XPKAPI int xpkIndexGetInfo(xpkObject objXpk, int64_t iFileIndex, xpkFileInfoIndex* pInfoRet)
{
	xpkEntry* pEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( objXpk->objHead.packType != XPK_PACK_INDEX ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( pInfoRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	pEntry = procXpkLookupIndexEntry(objXpk, iFileIndex);
	if ( pEntry == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}

	memset(pInfoRet, 0, sizeof(*pInfoRet));
	pInfoRet->flag = pEntry->iFlag;
	pInfoRet->fileHash = pEntry->iFileHash;
	pInfoRet->dataOffset = pEntry->iDataOffset;
	pInfoRet->dataSize = pEntry->iDataSize;
	pInfoRet->fileSize = pEntry->iFileSize;
	pInfoRet->fileIndex = pEntry->iFileIndex;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 按索引添加文件条目
XPKAPI int xpkIndexAddFile(xpkObject objXpk, int64_t iFileIndex, const char* sSrcPath, const xpkWriteOptions* pOpt)
{
	xpkEntry objEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( objXpk->objHead.packType != XPK_PACK_INDEX ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	iRet = xpkIndexFind(objXpk, iFileIndex, NULL);
	if ( iRet == XPK_OK ) {
		return procXpkSetError(objXpk, XPK_ERR_EXISTS, sXpkErrorExists);
	}
	if ( iRet != XPK_ERR_NOT_FOUND ) {
		return iRet;
	}
	procXpkClearError(objXpk);

	memset(&objEntry, 0, sizeof(objEntry));
	objEntry.iFileIndex = iFileIndex;
	return procXpkAddFileEntry(objXpk, &objEntry, sSrcPath, pOpt, NULL);
}

// 按索引添加内存数据条目
XPKAPI int xpkIndexAddData(xpkObject objXpk, int64_t iFileIndex, const void* pData, uint64_t iSize, const xpkWriteOptions* pOpt)
{
	xpkEntry objEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( objXpk->objHead.packType != XPK_PACK_INDEX ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	iRet = xpkIndexFind(objXpk, iFileIndex, NULL);
	if ( iRet == XPK_OK ) {
		return procXpkSetError(objXpk, XPK_ERR_EXISTS, sXpkErrorExists);
	}
	if ( iRet != XPK_ERR_NOT_FOUND ) {
		return iRet;
	}
	procXpkClearError(objXpk);

	memset(&objEntry, 0, sizeof(objEntry));
	objEntry.iFileIndex = iFileIndex;
	return procXpkAddDataEntry(objXpk, &objEntry, pData, iSize, pOpt, NULL);
}

// 按索引读取条目到文件
XPKAPI int xpkIndexReadToFile(xpkObject objXpk, int64_t iFileIndex, const char* sDstPath)
{
	uint32_t iPos;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( objXpk->objHead.packType != XPK_PACK_INDEX ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( sDstPath == NULL || sDstPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	iRet = xpkIndexFind(objXpk, iFileIndex, &iPos);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	return xpkReadToFile(objXpk, iPos, sDstPath);
}

// 按索引读取条目到内存
XPKAPI void* xpkIndexReadToMemory(xpkObject objXpk, int64_t iFileIndex, uint64_t* pSizeRet)
{
	uint32_t iPos;

	if ( xpkIndexFind(objXpk, iFileIndex, &iPos) != XPK_OK ) {
		if ( pSizeRet != NULL ) {
			*pSizeRet = 0;
		}
		return NULL;
	}
	return xpkReadToMemory(objXpk, iPos, pSizeRet);
}

// 按索引更新文件条目
XPKAPI int xpkIndexUpdateFile(xpkObject objXpk, int64_t iFileIndex, const char* sSrcPath, const xpkWriteOptions* pOpt)
{
	uint32_t iPos;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( objXpk->objHead.packType != XPK_PACK_INDEX ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = xpkIndexFind(objXpk, iFileIndex, &iPos);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	return xpkUpdateFile(objXpk, iPos, sSrcPath, pOpt);
}

// 按索引更新内存数据条目
XPKAPI int xpkIndexUpdateData(xpkObject objXpk, int64_t iFileIndex, const void* pData, uint64_t iSize, const xpkWriteOptions* pOpt)
{
	uint32_t iPos;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( objXpk->objHead.packType != XPK_PACK_INDEX ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = xpkIndexFind(objXpk, iFileIndex, &iPos);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	return xpkUpdateData(objXpk, iPos, pData, iSize, pOpt);
}

// 按索引移除条目
XPKAPI int xpkIndexRemove(xpkObject objXpk, int64_t iFileIndex)
{
	uint32_t iPos;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( objXpk->objHead.packType != XPK_PACK_INDEX ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = xpkIndexFind(objXpk, iFileIndex, &iPos);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	return xpkRemove(objXpk, iPos);
}

// 按索引设置条目标记
XPKAPI int xpkIndexSetFlag(xpkObject objXpk, int64_t iFileIndex, uint32_t iMask, uint32_t iValue)
{
	uint32_t iPos;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( objXpk->objHead.packType != XPK_PACK_INDEX ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	iRet = xpkIndexFind(objXpk, iFileIndex, &iPos);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	return xpkSetFlag(objXpk, iPos, iMask, iValue);
}


/* ===== File: src/api/path_api.h ===== */

/*
	xPack 路径 API 实现

	负责按包内路径访问条目的增删改查与属性操作。
*/

// 按路径检查条目是否存在
XPKAPI int xpkPathExists(xpkObject objXpk, const char* sPackagePath)
{
	xpkEntry* pEntry;
	int iRet;

	if ( objXpk == NULL ) {
		procXpkSetError(NULL, XPK_ERR_PARAM, sXpkErrorInvalidParam);
		return FALSE;
	}
	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
		return FALSE;
	}
	if ( procXpkValidateStoredPathText(objXpk, sPackagePath) != XPK_OK ) {
		return FALSE;
	}

	pEntry = procXpkLookupPathEntry(objXpk, sPackagePath);
	if ( pEntry == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return FALSE;
		}
		procXpkClearError(objXpk);
		return FALSE;
	}

	procXpkClearError(objXpk);
	return TRUE;
}

// 按路径获取条目信息
XPKAPI int xpkPathGetInfo(xpkObject objXpk, const char* sPackagePath, xpkFileInfoPath* pInfoRet)
{
	xpkEntry* pEntry;
	size_t iPathLen;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( pInfoRet == NULL ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	if ( procXpkValidateStoredPathText(objXpk, sPackagePath) != XPK_OK ) {
		return xpkLastError(objXpk);
	}

	pEntry = procXpkLookupPathEntry(objXpk, sPackagePath);
	if ( pEntry == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}

	memset(pInfoRet, 0, sizeof(*pInfoRet));
	pInfoRet->flag = pEntry->iFlag;
	pInfoRet->fileHash = pEntry->iFileHash;
	pInfoRet->dataOffset = pEntry->iDataOffset;
	pInfoRet->dataSize = pEntry->iDataSize;
	pInfoRet->fileSize = pEntry->iFileSize;
	if ( pEntry->sPath != NULL ) {
		iPathLen = strlen(pEntry->sPath);
		if ( iPathLen >= XPK_PATH_BYTES ) {
			iPathLen = XPK_PATH_BYTES - 1;
		}
		memcpy(pInfoRet->pathBytes, pEntry->sPath, iPathLen);
	}
	pInfoRet->platformAttr = pEntry->iPlatformAttr;
	pInfoRet->createTime = pEntry->tCreateTime;
	pInfoRet->modifyTime = pEntry->tModifyTime;
	pInfoRet->accessTime = pEntry->tAccessTime;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 按路径添加文件条目
XPKAPI int xpkPathAddFile(xpkObject objXpk, const char* sPackagePath, const char* sSrcPath, const xpkWriteOptions* pOpt)
{
	xpkEntry objEntry;
	char* sPathText;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( procXpkValidateStoredPathText(objXpk, sPackagePath) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( xpkPathExists(objXpk, sPackagePath) ) {
		return procXpkSetError(objXpk, XPK_ERR_EXISTS, sXpkErrorExists);
	}
	iRet = xpkLastError(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	procXpkClearError(objXpk);

	memset(&objEntry, 0, sizeof(objEntry));
	sPathText = procXpkDupPathStoredText(objXpk, sPackagePath);
	objEntry.sPath = sPathText;
	if ( objEntry.sPath == NULL ) {
		return xpkLastError(objXpk);
	}

	iRet = procXpkAddFileEntry(objXpk, &objEntry, sSrcPath, pOpt, NULL);
	if ( objEntry.sPath != NULL ) {
		xpkFreeInternal(objEntry.sPath);
	}
	return iRet;
}

// 按路径添加内存数据条目
XPKAPI int xpkPathAddData(xpkObject objXpk, const char* sPackagePath, const void* pData, uint64_t iSize, const xpkWriteOptions* pOpt)
{
	xpkEntry objEntry;
	char* sPathText;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( procXpkValidateStoredPathText(objXpk, sPackagePath) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	if ( xpkPathExists(objXpk, sPackagePath) ) {
		return procXpkSetError(objXpk, XPK_ERR_EXISTS, sXpkErrorExists);
	}
	iRet = xpkLastError(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	procXpkClearError(objXpk);

	memset(&objEntry, 0, sizeof(objEntry));
	sPathText = procXpkDupPathStoredText(objXpk, sPackagePath);
	objEntry.sPath = sPathText;
	if ( objEntry.sPath == NULL ) {
		return xpkLastError(objXpk);
	}

	iRet = procXpkAddDataEntry(objXpk, &objEntry, pData, iSize, pOpt, NULL);
	if ( objEntry.sPath != NULL ) {
		xpkFreeInternal(objEntry.sPath);
	}
	return iRet;
}

// 按路径读取条目到文件
XPKAPI int xpkPathReadToFile(xpkObject objXpk, const char* sPackagePath, const char* sDstPath)
{
	xpkEntry* pEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( procXpkValidateStoredPathText(objXpk, sPackagePath) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( sDstPath == NULL || sDstPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}

	pEntry = procXpkLookupPathEntry(objXpk, sPackagePath);
	if ( pEntry == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}
	return xpkReadToFile(objXpk, pEntry->iPos, sDstPath);
}

// 按路径读取条目到内存
XPKAPI void* xpkPathReadToMemory(xpkObject objXpk, const char* sPackagePath, uint64_t* pSizeRet)
{
	xpkEntry* pEntry;
	int iRet;

	if ( pSizeRet != NULL ) {
		*pSizeRet = 0;
	}
	if ( objXpk == NULL ) {
		procXpkSetError(NULL, XPK_ERR_PARAM, sXpkErrorInvalidParam);
		return NULL;
	}
	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
		return NULL;
	}
	if ( procXpkValidateStoredPathText(objXpk, sPackagePath) != XPK_OK ) {
		return NULL;
	}

	pEntry = procXpkLookupPathEntry(objXpk, sPackagePath);
	if ( pEntry == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return NULL;
		}
		procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
		return NULL;
	}
	return xpkReadToMemory(objXpk, pEntry->iPos, pSizeRet);
}

// 按路径更新文件条目
XPKAPI int xpkPathUpdateFile(xpkObject objXpk, const char* sPackagePath, const char* sSrcPath, const xpkWriteOptions* pOpt)
{
	xpkEntry* pEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( procXpkValidateStoredPathText(objXpk, sPackagePath) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( sSrcPath == NULL || sSrcPath[0] == '\0' ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	pEntry = procXpkLookupPathEntry(objXpk, sPackagePath);
	if ( pEntry == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}
	return xpkUpdateFile(objXpk, pEntry->iPos, sSrcPath, pOpt);
}

// 按路径更新内存数据条目
XPKAPI int xpkPathUpdateData(xpkObject objXpk, const char* sPackagePath, const void* pData, uint64_t iSize, const xpkWriteOptions* pOpt)
{
	xpkEntry* pEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( procXpkValidateStoredPathText(objXpk, sPackagePath) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( pData == NULL && iSize > 0 ) {
		return procXpkSetError(objXpk, XPK_ERR_PARAM, sXpkErrorInvalidParam);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	pEntry = procXpkLookupPathEntry(objXpk, sPackagePath);
	if ( pEntry == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}
	return xpkUpdateData(objXpk, pEntry->iPos, pData, iSize, pOpt);
}

// 按路径重命名条目
XPKAPI int xpkPathRename(xpkObject objXpk, const char* sPathOld, const char* sPathNew)
{
	xpkEntry* pEntry;
	xpkEntry* pEntryNew;
	char* sPathOldDup;
	char* sPathDup;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( sPathOld == NULL || sPathNew == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( procXpkValidateStoredPathText(objXpk, sPathOld) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	if ( procXpkValidateStoredPathText(objXpk, sPathNew) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	pEntry = procXpkLookupPathEntry(objXpk, sPathOld);
	if ( pEntry == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}

	pEntryNew = procXpkLookupPathEntry(objXpk, sPathNew);
	if ( pEntryNew == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		procXpkClearError(objXpk);
	} else if ( pEntryNew != pEntry ) {
		return procXpkSetError(objXpk, XPK_ERR_EXISTS, sXpkErrorExists);
	}

	sPathDup = procXpkDupPathStoredText(objXpk, sPathNew);
	if ( sPathDup == NULL ) {
		return xpkLastError(objXpk);
	}

	sPathOldDup = pEntry->sPath;
	pEntry->sPath = sPathDup;
	iRet = procXpkRebuildLookup(objXpk);
	if ( iRet != XPK_OK ) {
		pEntry->sPath = sPathOldDup;
		if ( sPathDup != NULL ) {
			xpkFreeInternal(sPathDup);
		}
		procXpkRebuildLookup(objXpk);
		return iRet;
	}
	if ( sPathOldDup != NULL ) {
		xpkFreeInternal(sPathOldDup);
	}
	objXpk->bDirtyEntryTable = TRUE;
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}

// 按路径移除条目
XPKAPI int xpkPathRemove(xpkObject objXpk, const char* sPackagePath)
{
	xpkEntry* pEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( procXpkValidateStoredPathText(objXpk, sPackagePath) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	pEntry = procXpkLookupPathEntry(objXpk, sPackagePath);
	if ( pEntry == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}
	return xpkRemove(objXpk, pEntry->iPos);
}

// 按路径设置平台属性
XPKAPI int xpkPathSetAttr(xpkObject objXpk, const char* sPackagePath, uint32_t iPlatformAttr)
{
	xpkEntry* pEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( (objXpk->objHead.packType != XPK_PACK_LINUX) && (objXpk->objHead.packType != XPK_PACK_WIN32) ) {
		return procXpkSetError(objXpk, XPK_ERR_STATE, sXpkErrorPackTypeMismatch);
	}
	if ( procXpkValidateStoredPathText(objXpk, sPackagePath) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	iRet = procXpkEnsureWritable(objXpk);
	if ( iRet != XPK_OK ) {
		return iRet;
	}

	pEntry = procXpkLookupPathEntry(objXpk, sPackagePath);
	if ( pEntry == NULL ) {
		iRet = xpkLastError(objXpk);
		if ( iRet != XPK_OK ) {
			return iRet;
		}
		return procXpkSetError(objXpk, XPK_ERR_NOT_FOUND, sXpkErrorNotFound);
	}

	pEntry->iPlatformAttr = iPlatformAttr;
	objXpk->bDirtyEntryTable = TRUE;
	objXpk->bDirtyHead = TRUE;
	procXpkClearError(objXpk);
	return XPK_OK;
}


/* ===== File: src/api/admin_api.h ===== */

/*
	xPack 管理 API 实现

	负责遍历、校验、统计、内存释放与错误查询接口。
*/

// 遍历全部可见条目
XPKAPI int xpkEach(xpkObject objXpk, xpkEachProc procEach, void* pArg)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	return procXpkEachWalk(objXpk, NULL, procEach, pArg);
}

// 按模式遍历匹配条目
XPKAPI int xpkEachMatch(xpkObject objXpk, const char* sPattern, xpkEachProc procEach, void* pArg)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	return procXpkEachWalk(objXpk, sPattern, procEach, pArg);
}

// 校验单个条目
XPKAPI int xpkVerify(xpkObject objXpk, uint32_t iPos)
{
	xpkEntry* pEntry;
	int iRet;

	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	if ( iPos == 0 ) {
		return procXpkReturnParamError(objXpk);
	}

	if ( procXpkGetPublicEntryByPos(objXpk, iPos, &pEntry) != XPK_OK ) {
		return xpkLastError(objXpk);
	}
	iRet = procXpkValidateLiveEntryLookup(objXpk, pEntry);
	if ( iRet != XPK_OK ) {
		return iRet;
	}
	return procXpkVerifyEntry(objXpk, pEntry);
}

// 校验全部条目
XPKAPI int xpkVerifyAll(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return procXpkReturnParamError(objXpk);
	}
	return procXpkVerifyAllEntries(objXpk);
}

// 获取包统计信息
XPKAPI int xpkStatGet(xpkObject objXpk, xpkStat* pStatRet)
{
	return procXpkStatCurrent(objXpk, pStatRet);
}

// 释放 xPack 返回的内存
XPKAPI void xpkFree(void* pMem)
{
	xpkFreeInternal(pMem);
}

// 计算 32 位哈希值
XPKAPI uint32_t xpkHash32(const void* pData, uint64_t iSize)
{
	return xpkHash32Internal(pData, iSize);
}

// 获取最后错误码
XPKAPI xpkErrorCode xpkLastError(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		return (xpkErrorCode)g_objXpkErrorTls.iCode;
	}
	return (xpkErrorCode)objXpk->err.iCode;
}

// 获取最后错误文本
XPKAPI const char* xpkLastErrorMessage(xpkObject objXpk)
{
	if ( objXpk == NULL ) {
		if ( g_objXpkErrorTls.sText[0] == '\0' ) {
			return "";
		}
		return g_objXpkErrorTls.sText;
	}
	if ( objXpk->err.sText[0] == '\0' ) {
		return "";
	}
	return objXpk->err.sText;
}


#undef XPACK_BUILD_CORE
#endif

#endif
