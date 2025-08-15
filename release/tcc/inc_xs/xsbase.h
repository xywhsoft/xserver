


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>



#define MMU_USE_SAMM								// 动态结构体数组
#define MMU_USE_MBMU								// 自增长缓冲区
#define MMU_USE_MM256								// 256 增量内存管理器
#define MMU_USE_MP256								// 256 增量内存池
#define MMU_USE_AVLTREE								// AVLTree
#define MMU_USE_HASH32								// 32位哈希算法
#define MMU_USE_AVLHT32								// 基于 AVLTree + 32位哈希算法的哈希表实现

#define XTE_USE_LITE								// 使用 xTemplate Lite Parser



#include "inline_xCore.h"
#include "inline_mmu.h"
#include "inline_mongoose.h"
#include "inline_libtcc.h"
#include "inline_xtemplate.h"
#include "inline_json.h"
#include "inline_sqlite3.h"
#include "inline_md4c.h"
#include "inline_other.h"



// 服务器类型定义
#define SPT_NONE		0		// 未知服务
#define SPT_HTTP		1		// HTTP 服务
#define SPT_MQTT		2		// MQTT 服务
#define SPT_CUSTOM		-1		// 自定义服务（事件驱动
#define SPT_THREAD		-2		// 单开一条线程运行的特殊服务



// 语言类型定义
#define SLT_STATIC		0		// 静态页
#define SLT_C			1		// C 语言
#define SLT_LUA			2		// Lua（未来可能会支持的）
#define SLT_JS			3		// JavaScript（未来可能会支持的）



// 服务器结构体
typedef struct {
	char* Name;											// 主机名称
	char* Desc;											// 主机描述
	char* Param;										// 启动参数
	char* Host;											// 主机地址（域名）
	char* Session;										// 主机 Session 前缀
	struct mg_str TLS_CA;								// 主机 TLS CA 证书路径
	struct mg_str TLS_Cert;								// 主机 TLS 证书路径
	struct mg_str TLS_Key;								// 主机 TLS 秘钥路径
	char* Path;											// 主机根目录
	int DevLang;										// 开发语言
	char* DevFile;										// 开发文件，动态开发工程的总入口点
	void* JsonNode;										// 配置文件的 JSON 对象
	void* DevObj;										// 开发语言上下文对象
	void* ServiceInit;									// 服务启动前调用（仅默认主机支持这个字段；函数不存在则不会调用）
	void* ServiceStart;									// 服务启动（仅默认主机支持这个字段；HTTP、MQTT等内置逻辑的服务不会调用此函数，自定义服务函数不存在则不会调用）
	void* ServiceUnit;									// 服务启动后调用（仅默认主机支持这个字段；函数不存在则不会调用）
	void* EventProc;									// 服务器网络事件回调（函数不存在则不会调用）
	void* RequestProc;									// HTTP 请求回调（函数不存在则不会调用）
	void* LoopProc;										// 轮询事件回调函数
	void (*XS_SetGlobalDate)(int idx, void* ptr);		// XS 传递全局数据回调函数
} XS_HostStruct, *XS_HostObject;
typedef struct {
	int Class;											// 服务器类型（HTTP、MQTT、Custom、Thread、等）
	char* Name;											// 服务器名称
	char* Desc;											// 服务器描述
	char* Param;										// 启动参数
	char* Addr;											// 绑定地址端口
	int EnableTLS;										// 是否启用 TLS
	char* AddrTLS;										// TLS 绑定地址端口
	int EnableDefaultHost;								// 是否启用默认 Host
	XS_HostStruct DefaultHost;							// 默认 Host
	unsigned int HostCount;								// Host 数量
	SAMM_Object Hosts;									// Host 列表（SAMM结构）
	AVLHT32_Object HostMap;								// Host 哈希表（用于快速定位 Host 数据结构）
	void* JsonNode;										// 配置文件的 JSON 对象
	struct mg_connection* Conn;							// mongoose 连接对象
	struct mg_connection* ConnTLS;						// mongoose 连接对象 TLS
} XS_ServerStruct, *XS_ServerObject;





// Mongoose 事件管理结构
struct mg_mgr* mgr;

// 	全局数据 - 服务器列表
SAMM_Object ServerList;



// 使用 extern 可能导致数据指针出现变化，原因不明，先用这样的方式传递全局数据
void XS_SetGlobalDate(int idx, void* ptr)
{
	if ( idx == 1 ) {
		mgr = ptr;
	} else if ( idx == 2 ) {
		ServerList = ptr;
	} else if ( idx == 3 ) {
		xCore = ptr;
	}
}


