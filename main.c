


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>



#include "lib/xrt/xrt.h"
#include "lib/mongoose.h"
#include "lib/libtcc.h"
#include "lib/sqlite3.h"
//#include "lib/sqlite3ext.h"
//#include "lib/md4c.h"
//#include "lib/md4c-html.h"





// 服务器类型定义
#define SPT_NONE		0			// 未知服务
#define SPT_HTTP		1			// HTTP 服务
#define SPT_MQTT		2			// MQTT 服务
#define SPT_WS			3			// WebSocket 服务
#define SPT_XTP			4			// XTP 服务
#define SPT_TCP			0x10000		// TCP 协议
#define SPT_UDP			0x10001		// UDP 协议
#define SPT_CUSTOM		-1			// 自定义服务（事件驱动
#define SPT_THREAD		-2			// 单开一条线程运行的特殊服务



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
	xarray Hosts;										// Host 列表
	xdict HostMap;										// Host 字典（用于快速定位 Host 数据结构）
	void* JsonNode;										// 配置文件的 JSON 对象
	struct mg_connection* Conn;							// mongoose 连接对象
	struct mg_connection* ConnTLS;						// mongoose 连接对象 TLS
} XS_ServerStruct, *XS_ServerObject;

// 轮询事件结构体
typedef struct {
	void (*pProc)(XS_ServerObject objServer, XS_HostObject objHost);
	XS_ServerObject objServer;
	XS_HostObject objHost;
} XS_LoopEventStruct;





// Mongoose 事件管理结构
struct mg_mgr mgr;

// 	全局数据 - 服务器列表
xarray ServerList;

// 全局数据 - 轮询事件列表
xarray LoopEventList;





// 功能补全函数库
#include "lib/other.h"



// 动态脚本执行导入函数
#include "src/import_c/dynload.h"



// 业务实现头文件
#include "src/http.h"
#include "src/mqtt.h"
#include "src/websocket.h"
#include "src/xtp.h"
#include "src/tcp.h"
#include "src/udp.h"
#include "src/custom.h"
#include "src/thread.h"





// 载入主机配置
void LoadHostConfig(xvalue objRoot, XS_HostObject objHost, XS_ServerObject objServer)
{
	const char* sPath;
	// 读取开发语言类型
	int iLanguage = SLT_STATIC;
	char* sLanguage = xvoTableGetText(objRoot, "devlang", 7);
	if ( strcmp(sLanguage, "c") == 0 ) {
		iLanguage = SLT_C;
	} else if ( strcmp(sLanguage, "lua") == 0 ) {
		iLanguage = SLT_LUA;
	} else if ( strcmp(sLanguage, "js") == 0 ) {
		iLanguage = SLT_JS;
	}
	// 读取基本信息
	objHost->DevLang = iLanguage;
	objHost->Name = xvoTableGetText(objRoot, "name", 4);
	objHost->Desc = xvoTableGetText(objRoot, "desc", 4);
	objHost->Param = xvoTableGetText(objRoot, "param", 5);
	objHost->Host = xvoTableGetText(objRoot, "host", 4);
	objHost->Session = xvoTableGetText(objRoot, "session", 7);
	objHost->JsonNode = objRoot;
	// 创建 Host 映射
	if ( objHost->Host ) {
		str* arrHost = xrtSplit(objHost->Host, 0, ";", 1, FALSE);
		int iCount = xCore.iRet;
		for ( int i = 0; i < iCount; i++ ) {
			char* sHost = xrtReplace(arrHost[i], 0, " ", 1, "", 0);
			XS_HostObject* ppHost = xrtDictSet(objServer->HostMap, sHost, strlen(sHost), NULL);
			if ( ppHost == NULL ) {
				printf("!!! ERROR !!! xrtDictSet Failed [HostMap] !\n");
				exit(EXIT_FAILURE);
			}
			ppHost[0] = objHost;
			xrtFree(sHost);
		}
		xrtFree(arrHost);
	}
	// 处理目录（相对路径转换为绝对路径）
	sPath = xvoTableGetText(objRoot, "path", 4);
	if ( (sPath == NULL) || (strlen(sPath) == 0) ) {
		printf("!!! ERROR !!! host (%s - %s) must specify a directory !\n", objServer->Name, objHost->Name);
		exit(EXIT_FAILURE);
	} else if ( xrtPathIsAbs((char*)sPath, 0) ) {
		objHost->Path = (char*)sPath;
	} else {
		objHost->Path = malloc(4096);
		realpath(sPath, objHost->Path);
	}
	sPath = xvoTableGetText(objRoot, "devfile", 7);
	if ( (sPath == NULL) || (strlen(sPath) == 0) ) {
		if ( objHost->DevLang == SLT_C ) {
			objHost->DevFile = xrtPathJoin(2, objHost->Path, 0, "main.c", 6);
		} else if ( objHost->DevLang == SLT_LUA ) {
			objHost->DevFile = xrtPathJoin(2, objHost->Path, 0, "main.lua", 8);
		} else if ( objHost->DevLang == SLT_JS ) {
			objHost->DevFile = xrtPathJoin(2, objHost->Path, 0, "main.js", 7);
		}
	} else if ( xrtPathIsAbs((char*)sPath, 0) ) {
		objHost->DevFile = (char*)sPath;
	} else {
		objHost->DevFile = malloc(4096);
		realpath(sPath, objHost->DevFile);
	}
	// 读取证书
	if ( objServer->EnableTLS ) {
		sPath = xvoTableGetText(objRoot, "tls_ca", 6);
		char sTempPath[4096];
		if ( sPath ) {
			realpath(sPath, sTempPath);
			objHost->TLS_CA = mg_file_read(&mg_fs_posix, sTempPath);
		} else {
			objHost->TLS_CA.len = 0;
			objHost->TLS_CA.buf = NULL;
		}
		sPath = xvoTableGetText(objRoot, "tls_cert", 8);
		realpath(sPath, sTempPath);
		objHost->TLS_Cert = mg_file_read(&mg_fs_posix, sTempPath);
		sPath = xvoTableGetText(objRoot, "tls_key", 7);
		realpath(sPath, sTempPath);
		objHost->TLS_Key = mg_file_read(&mg_fs_posix, sTempPath);
	}
	// 加载动态开发入口文件
	if ( objHost->DevLang == SLT_C ) {
		DynLoad_C(objServer, objHost);
	} else if ( objHost->DevLang == SLT_LUA ) {
		objHost->DevObj = NULL;
		objHost->ServiceInit = NULL;
		objHost->ServiceStart = NULL;
		objHost->ServiceUnit = NULL;
		objHost->EventProc = NULL;
		objHost->RequestProc = NULL;
		objHost->LoopProc = NULL;
		objHost->XS_SetGlobalDate = NULL;
	} else if ( objHost->DevLang == SLT_JS ) {
		objHost->DevObj = NULL;
		objHost->ServiceInit = NULL;
		objHost->ServiceStart = NULL;
		objHost->ServiceUnit = NULL;
		objHost->EventProc = NULL;
		objHost->RequestProc = NULL;
		objHost->LoopProc = NULL;
		objHost->XS_SetGlobalDate = NULL;
	} else {
		objHost->DevObj = NULL;
		objHost->ServiceInit = NULL;
		objHost->ServiceStart = NULL;
		objHost->ServiceUnit = NULL;
		objHost->EventProc = NULL;
		objHost->RequestProc = NULL;
		objHost->LoopProc = NULL;
		objHost->XS_SetGlobalDate = NULL;
	}
	// 如果存在轮询事件，存入单独的表（加快访问速度）
	if ( objHost->LoopProc ) {
		unsigned int idx = xrtArrayAppend(LoopEventList, 1);
		if ( idx == 0 ) {
			printf("!!! ERROR !!! xrtArrayAppend Failed [LoopEventList] !\n");
			exit(EXIT_FAILURE);
		}
		XS_LoopEventStruct* objEvent = xrtArrayGet_Inline(LoopEventList, idx);
		objEvent->pProc = objHost->LoopProc;
		objEvent->objServer = objServer;
		objEvent->objHost = objHost;
	}
}



// 载入服务器配置
int LoadServerConfig(xvalue objRoot)
{
	// 检查服务器配置是否已启用
	if ( xvoTableGetBool(objRoot, "enabled", 7) == 0 ) {
		printf("server enabled = false\n");
		return 0;
	}
	// 读取网络协议类型
	int iClass = SPT_NONE;
	char* sClass = xvoTableGetText(objRoot, "class", 5);
	if ( strcasecmp(sClass, "http") == 0 ) {
		iClass = SPT_HTTP;
	} else if ( strcasecmp(sClass, "mqtt") == 0 ) {
		iClass = SPT_MQTT;
	} else if ( strcasecmp(sClass, "ws") == 0 ) {
		iClass = SPT_WS;
	} else if ( strcasecmp(sClass, "xtp") == 0 ) {
		iClass = SPT_XTP;
	} else if ( strcasecmp(sClass, "tcp") == 0 ) {
		iClass = SPT_TCP;
	} else if ( strcasecmp(sClass, "udp") == 0 ) {
		iClass = SPT_UDP;
	} else if ( strcasecmp(sClass, "custom") == 0 ) {
		iClass = SPT_CUSTOM;
	} else if ( strcasecmp(sClass, "thread") == 0 ) {
		iClass = SPT_THREAD;
	}
	if ( iClass == SPT_NONE ) {
		printf("Unsupported class : %s\n", sClass);
		return 0;
	}
	// 创建服务器数据对象
	unsigned int idx = xrtArrayAppend(ServerList, 1);
	if ( idx == 0 ) {
		printf("!!! ERROR !!! xrtArrayAppend Failed [ServerList] !\n");
		exit(EXIT_FAILURE);
	}
	XS_ServerObject objServer = xrtArrayGet_Inline(ServerList, idx);
	// 读取常规配置
	objServer->Class = iClass;
	objServer->Name = xvoTableGetText(objRoot, "name", 4);
	objServer->Desc = xvoTableGetText(objRoot, "desc", 4);
	objServer->Param = xvoTableGetText(objRoot, "param", 5);
	objServer->Addr = xvoTableGetText(objRoot, "addr", 4);
	objServer->EnableTLS = xvoTableGetBool(objRoot, "tls", 3);
	if ( objServer->EnableTLS ) {
		objServer->AddrTLS = xvoTableGetText(objRoot, "addr_tls", 8);
	} else {
		objServer->AddrTLS = NULL;
	}
	objServer->JsonNode = objRoot;
	// 读取默认主机配置
	xvalue objDefHost = xvoTableGetValue(objRoot, "host_default", 12);
	if ( objDefHost->Type != XVO_DT_TABLE ) {
		printf("host_default must be of type object !\n");
		exit(EXIT_FAILURE);
	}
	objServer->EnableDefaultHost = xvoTableGetBool(objDefHost, "enabled", 7);
	if ( objServer->EnableDefaultHost ) {
		LoadHostConfig(objDefHost, &objServer->DefaultHost, objServer);
	}
	// 创建 Host 映射表
	objServer->HostMap = xrtDictCreate(sizeof(XS_HostObject*));
	if ( objServer->HostMap == NULL ) {
		printf("!!! ERROR !!! HostMap init failed !\n");
		exit(EXIT_FAILURE);
	}
	// 读取主机列表
	xvalue arrHost = xvoTableGetValue(objRoot, "hosts", 5);
	if ( arrHost->Type != XVO_DT_ARRAY ) {
		printf("hosts must be of type array !\n");
		exit(EXIT_FAILURE);
	}
	objServer->HostCount = xvoArraySize(arrHost);
	objServer->Hosts = xrtArrayCreate(sizeof(XS_HostStruct));
	if ( objServer->Hosts == NULL ) {
		printf("!!! ERROR !!! HostList init failed !\n");
		exit(EXIT_FAILURE);
	}
	xrtArrayAlloc(objServer->Hosts, objServer->HostCount);
	// 遍历读取每一个主机的信息
	for ( int i = 0; i < objServer->HostCount; i++ ) {
		xvalue objHostInfo = xvoArrayGetValue(arrHost, i);
		if ( objHostInfo->Type != XVO_DT_TABLE ) {
			printf("!!! ERROR !!! JSON Type no object [Host] (idx : %d) !\n", i);
			exit(EXIT_FAILURE);
		}
		if ( xvoTableGetBool(objHostInfo, "enabled", 7) == 0 ) {
			printf("host enabled = false\n");
			continue;
		}
		unsigned int idx = xrtArrayAppend(objServer->Hosts, 1);
		if ( idx == 0 ) {
			printf("!!! ERROR !!! xrtArrayAppend Failed [HostList] !\n");
			exit(EXIT_FAILURE);
		}
		XS_HostObject objHost = xrtArrayGet_Inline(objServer->Hosts, idx);
		LoadHostConfig(objHostInfo, objHost, objServer);
	}
	// 输出控制台日志
	printf("addr : %s\n", objServer->Addr);
	printf("EnableTLS : %d\n", objServer->EnableTLS);
	printf("addr(tls) : %s\n", objServer->AddrTLS);
	printf("path : %s\n", objServer->DefaultHost.Path);
	return -1;
}



// 加载配置文件
int LoadConfig(char* sOptFile)
{
	printf("load option file : %s\n", sOptFile);
	// 初始化 ServerList 数据结构
	ServerList = xrtArrayCreate(sizeof(XS_ServerStruct));
	if ( ServerList == NULL ) {
		printf("!!! ERROR !!! ServerList init failed !\n");
		exit(EXIT_FAILURE);
	}
	// 初始化 LoopEventList 数据结构
	LoopEventList = xrtArrayCreate(sizeof(XS_LoopEventStruct));
	if ( LoopEventList == NULL ) {
		printf("!!! ERROR !!! LoopEventList init failed !\n");
		exit(EXIT_FAILURE);
	}
	// 加载配置文件
	xvalue varJSON = xrtParseJSON_File(sOptFile);
	//xvoPrintValue(varJSON, 0, 0, 0, NULL);
	if ( varJSON->Type == XVO_DT_TABLE ) {
		// 单服务端口配置
		xrtArrayAlloc(ServerList, 1);
		if ( LoadServerConfig(varJSON) == 0 ) {
			printf("!!! ERROR !!! LoadServerConfig Failed !\n");
			exit(EXIT_FAILURE);
		}
	} else if ( varJSON->Type == XVO_DT_ARRAY ) {
		// 多服务端口配置
		size_t iCount = xvoArraySize(varJSON);
		xrtArrayAlloc(ServerList, iCount);
		for ( int i = 0; i < iCount; i++ ) {
			xvalue varItem = xvoArrayGetValue(varJSON, i);
			if ( varItem->Type == XVO_DT_TABLE ) {
				LoadServerConfig(varItem);
			} else {
				printf("!!! ERROR !!! JSON Type no object [Server] (idx : %d) !\n", i);
				exit(EXIT_FAILURE);
			}
		}
	} else {
		printf("!!! ERROR !!! Option JSON File no object or array !\n");
		exit(EXIT_FAILURE);
	}
}


// 信号处理 [Ctrl-C]
static int s_signo;
static void signal_handler(int signo) {
	s_signo = signo;
}



// 启动服务器
int RunServer()
{
	if ( ServerList->Count == 0 ) {
		printf("The server list is empty .\n");
		return 0;
	}
	// 添加信号处理回调
	signal(SIGINT, signal_handler);				// 控制台按 Ctrl + C 触发
	signal(SIGTERM, signal_handler);			// 程序结束触发
	// 启动服务
	printf("run server [%d] ...\n", ServerList->Count);
	mg_log_set(MG_LL_INFO);
	mg_mgr_init(&mgr);
	for ( int i = 1; i <= ServerList->Count; i++ ) {
		XS_ServerObject objServer = xrtArrayGet_Inline(ServerList, i);
		if ( objServer->Class == SPT_HTTP ) {
			RunServerHTTP(objServer);
		} else if ( objServer->Class == SPT_MQTT ) {
			RunServerMQTT(objServer);
		} else if ( objServer->Class == SPT_WS ) {
			RunServerWS(objServer);
		} else if ( objServer->Class == SPT_XTP ) {
			RunServerXTP(objServer);
		} else if ( objServer->Class == SPT_TCP ) {
			RunServerTCP(objServer);
		} else if ( objServer->Class == SPT_UDP ) {
			RunServerUDP(objServer);
		} else if ( objServer->Class == SPT_CUSTOM ) {
			RunServerCustom(objServer);
		} else if ( objServer->Class == SPT_THREAD ) {
			RunServerThread(objServer);
		}
	}
	// 等待服务停止
	while ( s_signo == 0 ) {
		mg_mgr_poll(&mgr, 1000);
		// 调用轮询事件
		for ( int i = 1; i <= LoopEventList->Count; i++ ) {
			XS_LoopEventStruct* objEvent = xrtArrayGet_Inline(LoopEventList, i);
			objEvent->pProc(objEvent->objServer, objEvent->objHost);
		}
	}
	// 停止服务
	for ( int i = 1; i <= ServerList->Count; i++ ) {
		XS_ServerObject objServer = xrtArrayGet_Inline(ServerList, i);
		if ( objServer->Class == SPT_HTTP ) {
			StopServerHTTP(objServer);
		} else if ( objServer->Class == SPT_MQTT ) {
			StopServerMQTT(objServer);
		} else if ( objServer->Class == SPT_WS ) {
			StopServerWS(objServer);
		} else if ( objServer->Class == SPT_XTP ) {
			StopServerXTP(objServer);
		} else if ( objServer->Class == SPT_TCP ) {
			StopServerTCP(objServer);
		} else if ( objServer->Class == SPT_UDP ) {
			StopServerUDP(objServer);
		} else if ( objServer->Class == SPT_CUSTOM ) {
			StopServerCustom(objServer);
		} else if ( objServer->Class == SPT_THREAD ) {
			StopServerThread(objServer);
		}
	}
	mg_mgr_free(&mgr);
	MG_INFO(("Exiting on signal %d\n", s_signo));
}



int main(int argc, char** argv)
{
	#if defined(_WIN32) || defined(_WIN64)
		SetConsoleOutputCP(65001);
	#endif
	// 初始化 xrt 库
	xrtInit();
	// 初始化 JSON 库内存池
	
	// 从命令行中读取配置文件路径
	char* sOptFile = "xs.json";
	if ( argc > 1 ) {
		sOptFile = argv[1];
	}
	LoadConfig(sOptFile);
	// 启动服务器
	RunServer();
	// 销毁 JSON 库内存池
	
	// 卸载 xrt 库
	xrtUnit();
	return 0;
}


