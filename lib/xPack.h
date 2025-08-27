


#define XPK_LDBCOMP			0x40000000			// 包位标记：压缩LDB数据段
#define XPK_LDBCOMPTYPE		0x80000000			// 包位标记：LDB数据段压缩方式（0为FAST、1为HIGH）

#define XPK_FILEPATHMAX		160					// 文件路径最大长度（节省内存，比 MAX_PATH 略短）

#define XPK_CLASS_Core		0x0					// 核心压缩包，仅支持使用顺序读取数据（其他方式都能用）
#define XPK_CLASS_Index		0x1					// Index访问包，可以通过 Index 访问数据
#define XPK_CLASS_Linux		0x2					// Linux文件系统兼容包，有额外的结构保存 Linux 文件数据
#define XPK_CLASS_Win32		0x3					// Win32文件系统兼容包，有额外的结构保存 Windows 文件数据
#define XPK_CLASS_MASK		0x3

#define XPK_COMP_NO 		0x0					// 压缩标记：不压缩
#define XPK_COMP_FAST		0x1					// 压缩标记：快速压缩（LZ4）
#define XPK_COMP_HIGH 		0x2					// 压缩标记：高压缩比（LZMA）
#define XPK_COMP_CUSTOM		0x3					// 压缩标记：自定义压缩（如果未指定回调，则相当于 XPK_COMP_NO）



// 包信息头（40 byte）
typedef struct {
	uint FileHead;								// 文件标识头（xpk + CptVer）
	uint PackFlag;								// 文件位标记
	uint FileCount;								// 文件数量
	uint LDB_Addr;								// 列表数据偏移
	uint LDB_Size;								// 列表数据大小
	uint LDB_Hash;								// 列表数据哈希值
	uint HeadSize;								// 文件头扩展数据大小
	uint InfoSize;								// 文件信息段单元扩展数据大小（基础20个字节不算在内）
	uint DiscCode;								// 识别代码（用户自定义数据，用于识别是否是自己的二次开发压缩包）
	uint Reserve;								// 保留数据，暂未使用
} xPack_FileHead;

// 文件信息头（20 byte）
typedef struct {
	uint DataAddr;								// 数据位置
	uint DataSize;								// 数据大小
	uint FileSize;								// 文件大小 [解压后]
	uint FileHash;								// 文件哈希值 [解压后]
	uint FileFlag;								// 文件位标记
} xPack_FileInfo;

// 文件信息头（20 + 8 byte）[Index]
typedef struct {
	uint DataAddr;								// 数据位置
	uint DataSize;								// 数据大小
	uint FileSize;								// 文件大小 [解压后]
	uint FileHash;								// 文件哈希值 [解压后]
	uint FileFlag;								// 文件位标记
	int FileIndex;								// 文件编号
	int FileTag;								// 文件附加数据
} xPack_FileInfo_Index;

// 文件信息头（20 + 180 byte）[Linux]（暂未开发完成）
typedef struct {
	uint DataAddr;								// 数据位置
	uint DataSize;								// 数据大小
	uint FileSize;								// 文件大小 [解压后]
	uint FileHash;								// 文件哈希值 [解压后]
	uint FileFlag;								// 文件位标记
	char FilePath[XPK_FILEPATHMAX];				// 文件路径
	uint PathHash;								// 文件路径的哈希值，用于快速查找文件（大小写敏感）
	int FileAttr;								// 文件属性（是否文件夹，是否链接，读写执行权限）
	uint ModifyTime;							// 文件的修改时间
	int FileTag;								// 文件附加数据
	uint Reserve;								// 保留数据，暂未使用
} xPack_FileInfo_Linux;

// 文件信息头（20 + 180 byte）[Win32]（暂未开发完成）
typedef struct {
	uint DataAddr;								// 数据位置
	uint DataSize;								// 数据大小
	uint FileSize;								// 文件大小 [解压后]
	uint FileHash;								// 文件哈希值 [解压后]
	uint FileFlag;								// 文件位标记
	char FilePath[XPK_FILEPATHMAX];				// 文件路径
	uint PathHash;								// 文件路径的哈希值，用于快速查找文件（转小写HASH）
	int FileAttr;								// 文件属性（系统、存档、隐藏、只读、是否文件夹）
	uint CreateTime;							// 文件的创建时间
	uint ModifyTime;							// 文件的修改时间
	int FileTag;								// 文件附加数据
} xPack_FileInfo_Win32;

// 压缩返回数据结构
typedef struct {
	int Level;									// [in]		压缩级别
	ptr SrcAddr;								// [in]		数据位置
	uint SrcSize;								// [in]		数据大小
	ptr DstAddr;								// [out]	压缩或解压后的数据
	uint DstSize;								// [in/out]	压缩或解压后的数据大小
	int FreeData;								// [out]	原始数据是否可以释放（避免返回数据和原始数据是一个值）
} xPack_CompInfo;



// xPackObject
typedef struct {
	xfile FileObject;											// 文件句柄
	uint FileOffset;											// 文件偏移
	int ReadOnly;												// 只读
	int IsChange;												// 是否修改
	xPack_FileHead PackHead;									// 包文件头
	SAMM_Object LDB;											// 列表段数据管理器
	void (*OnError)(int iErrCode, str sErrText);				// 出错回调函数
	uint (*OnCompress)(ptr xpk, xPack_CompInfo* info);			// 自定义压缩回调函数
	uint (*OnUnCompress)(ptr xpk, xPack_CompInfo* info);		// 自定义解压回调函数
} xPackStruct, *xPackObject;



// 压缩入口（压缩失败自动转换为无压缩）
XXAPI int xPack_Compress_Router(xPackObject xpk, xPack_CompInfo* pInfo);

// 解压入口
XXAPI int xPack_DeCompress_Router(xPackObject xpk, xPack_CompInfo* pInfo);

// 保存文件包
XXAPI int xPack_Save(xPackObject xpk, int bReBuild);

// 关闭文件包
XXAPI void xPack_Close(xPackObject xpk);

// 打开文件包(如果文件不存在则创建)
XXAPI xPackObject xPack_Open(str sFile, uint iOffset, int bReadOnly);

// 获取包文件数量
XXAPI uint xPack_FileCount(xPackObject xpk);

// 设置包类型（只有在还没添加文件的时候可以修改）
XXAPI int xPack_SetPackType(xPackObject xpk, int iVal);

// 获取包类型
XXAPI int xPack_GetPackType(xPackObject xpk);

// 设置文件信息扩展长度（只有在还没添加文件的时候可以修改）（只能设置 Core 模式的包）
XXAPI int xPack_SetFileInfoExtSize(xPackObject xpk, uint iVal);

// 获取文件信息扩展长度
XXAPI int xPack_GetFileInfoExtSize(xPackObject xpk);

// 设置包信息扩展长度（只有在还没添加文件的时候可以修改）（只能设置 Core 模式的包）
XXAPI int xPack_SetPackInfoExtSize(xPackObject xpk, uint iVal);

// 获取文件信息扩展长度
XXAPI int xPack_GetPackInfoExtSize(xPackObject xpk);

// 设置包文件识别代码
XXAPI int xPack_SetPackDiscCode(xPackObject xpk, uint iVal);

// 获取包文件识别代码
XXAPI int xPack_GetPackDiscCode(xPackObject xpk);

// 获取文件信息结构体指针
XXAPI ptr xPack_GetFileInfo(xPackObject xpk, uint iPos);

// 获取文件大小
XXAPI uint xPack_GetFileSize(xPackObject xpk, uint iPos);

// 获取数据大小
XXAPI uint xPack_GetFileDataSize(xPackObject xpk, uint iPos);

// 获取文件哈希值
XXAPI uint xPack_GetFileHash(xPackObject xpk, uint iPos);

// 获取文件压缩级别
XXAPI uint xPack_GetFileCompLevel(xPackObject xpk, uint iPos);

// 添加文件（核心模式）
XXAPI uint xPack_Core_AppendFile(xPackObject xpk, str sFile, int iCompLevel);

// 添加数据（核心模式）
XXAPI uint xPack_Core_AppendData(xPackObject xpk, ptr pIn, uint iSize, int iCompLevel);

// 修改文件（核心模式）
XXAPI xPack_FileInfo* xPack_Core_ChangeFile(xPackObject xpk, uint iPos, str sFile, int iCompLevel);

// 修改数据（核心模式）
XXAPI xPack_FileInfo* xPack_Core_ChangeData(xPackObject xpk, uint iPos, ptr pIn, uint iSize, int iCompLevel);

// 解包文件（核心模式）
XXAPI xPack_FileInfo* xPack_Core_UnpackFile(xPackObject xpk, uint iPos, str sFile);

// 解包数据（核心模式）
XXAPI xPack_FileInfo* xPack_Core_UnpackData(xPackObject xpk, uint iPos, ptr* pData);

// 删除文件（核心模式）
XXAPI int xPack_Core_DeleteFile(xPackObject xpk, uint iPos);

// 通过 Index 获取 POS
XXAPI uint xPack_IndexToPos(xPackObject xpk, int iIndex);

// 添加文件（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_AppendFile(xPackObject xpk, int iIndex, str sFile, int iCompLevel);

// 添加数据（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_AppendData(xPackObject xpk, int iIndex, ptr pIn, uint iSize, int iCompLevel);

// 修改文件（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_ChangeFile(xPackObject xpk, int iIndex, str sFile, int iCompLevel);

// 修改数据（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_ChangeData(xPackObject xpk, int iIndex, ptr pIn, uint iSize, int iCompLevel);

// 解包文件（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_UnpackFile(xPackObject xpk, int iIndex, str sFile);

// 解包数据（无序索引方式）
XXAPI xPack_FileInfo_Index* xPack_Index_UnpackData(xPackObject xpk, int iIndex, ptr* pData);

// 删除文件（无序索引方式）
XXAPI int xPack_Index_DeleteFile(xPackObject xpk, int iIndex);

// 通过 Linux文件路径 获取 POS
XXAPI uint xPack_LinuxToPos(xPackObject xpk, str sPath);

// 添加文件（Linux方式）
XXAPI xPack_FileInfo_Linux* xPack_Linux_AppendFile(xPackObject xpk, str sPath, str sFile, int iCompLevel);

// 修改文件（Linux方式）
XXAPI xPack_FileInfo_Linux* xPack_Linux_ChangeFile(xPackObject xpk, str sPath, str sFile, int iCompLevel);

// 解包文件（Linux方式）
XXAPI xPack_FileInfo_Linux* xPack_Linux_UnpackFile(xPackObject xpk, str sPath, str sFile);

// 解包数据（Linux方式）
XXAPI xPack_FileInfo_Linux* xPack_Linux_UnpackData(xPackObject xpk, str sPath, ptr* pData);

// 删除文件（Linux方式）
XXAPI int xPack_Linux_DeleteFile(xPackObject xpk, str sPath);

// 通过 Win32文件路径 获取 POS（和Linux方式的主要区别为不区分大小写）
XXAPI uint xPack_Win32ToPos(xPackObject xpk, str sPath);

// 添加文件（Win32方式）
XXAPI xPack_FileInfo_Win32* xPack_Win32_AppendFile(xPackObject xpk, str sPath, str sFile, int iCompLevel);

// 修改文件（Win32方式）
XXAPI xPack_FileInfo_Win32* xPack_Win32_ChangeFile(xPackObject xpk, str sPath, str sFile, int iCompLevel);

// 解包文件（Win32方式）
XXAPI xPack_FileInfo_Win32* xPack_Win32_UnpackFile(xPackObject xpk, str sPath, str sFile);

// 解包数据（Win32方式）
XXAPI xPack_FileInfo_Win32* xPack_Win32_UnpackData(xPackObject xpk, str sPath, ptr* pData);

// 删除文件（Win32方式）
XXAPI int xPack_Win32_DeleteFile(xPackObject xpk, str sPath);


