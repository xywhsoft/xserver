


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>  // PATH_MAX

// Windows 下 realpath 的兼容实现
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define realpath(path, resolved) _fullpath(resolved, path, PATH_MAX)
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif


// 定义 XRT_IMPLEMENTATION 导入功能实现
#define XRT_IMPLEMENTATION
#include "lib/xrt/xrt.h"
#include "lib/libtcc.h"
#include "lib/sqlite3.h"
#include "lib/xdo/xdo.h"
#include "lib/xdo/sqlite.h"





// 服务器类型定义
#define SPT_NONE		0			// 未知服务
#define SPT_HTTP		1			// HTTP 服务
#define SPT_WS			3			// WebSocket 服务
#define SPT_XTP			4			// XTP 服务
#define SPT_TCP			0x10000		// TCP 协议
#define SPT_UDP			0x10001		// UDP 协议
#define SPT_CUSTOM		-1			// 自定义服务（事件驱动）
#define SPT_THREAD		-2			// 单开一条线程运行的特殊服务



// 语言类型定义
#define SLT_STATIC		0		// 静态页
#define SLT_C			1		// C 语言
#define SLT_LUA			2		// Lua（未来可能会支持的）
#define SLT_JS			3		// JavaScript（未来可能会支持的）



// 事件类型定义 (供 TCC 脚本使用)
#define XRT_EV_ACCEPT	1		// 接受连接
#define XRT_EV_RECV		2		// 收到数据
#define XRT_EV_CLOSE	3		// 连接关闭
#define XRT_EV_SEND		4		// 发送完成



// 服务器结构体
typedef struct {
	str Name;											// 主机名称
	str Desc;											// 主机描述
	str Param;											// 启动参数
	str Host;											// 主机地址（域名）
	bool DebugMode;										// 输出额外的信息
	xtlsconfig tTlsConfig;								// TLS 配置
	str Path;											// 主机根目录
	int DevLang;										// 开发语言
	str DevFile;										// 开发文件，动态开发工程的总入口点
	ptr JsonNode;										// 配置文件的 JSON 对象
	ptr DevObj;											// 开发语言上下文对象
	ptr ServiceInit;									// 服务启动前调用（函数不存在则不会调用）
	ptr ServiceStart;									// 服务启动（HTTP等内置逻辑的服务不会调用此函数）
	ptr ServiceUnit;									// 服务启动后调用（函数不存在则不会调用）
	ptr EventProc;										// 服务器网络事件回调（函数不存在则不会调用）
	ptr RequestProc;									// HTTP 请求回调（函数不存在则不会调用）
	ptr WsEventProc;									// WebSocket 事件回调（函数不存在则不会调用）
	void (*XS_SetGlobalDate)(int idx, void* ptr);		// XS 传递全局数据回调函数
} XS_HostStruct, *XS_HostObject;

typedef struct {
	int Class;											// 服务器类型（HTTP、Custom、Thread、等）
	str Name;											// 服务器名称
	str Desc;											// 服务器描述
	str Param;											// 启动参数
	str Addr;											// 绑定地址端口
	int EnableTLS;										// 是否启用 TLS
	str AddrTLS;										// TLS 绑定地址端口
	int EnableDefaultHost;								// 是否启用默认 Host
	XS_HostStruct DefaultHost;							// 默认 Host
	uint32 HostCount;									// Host 数量
	xarray Hosts;										// Host 列表
	xdict HostMap;										// Host 字典（用于快速定位 Host 数据结构）
	ptr JsonNode;										// 配置文件的 JSON 对象
	ptr pServer;										// xrt 服务器对象 (xhttpserver*/xtcpserver*/...)
	ptr pServerTLS;										// xrt TLS 服务器对象
} XS_ServerStruct, *XS_ServerObject;





// 全局事件循环
xeventloop* g_pLoop = NULL;

// 全局数据 - 服务器列表
xarray ServerList;


// 前向声明
static void ParseAddr(const char* sAddr, char* sIP, size_t iIPSize, uint16* pPort);


// 功能补全函数库
#include "lib/other.h"



// 动态脚本执行导入函数
#include "src/import_c/dynload.h"



// 业务实现头文件
#include "src/http.h"
#include "src/websocket.h"
#include "src/xtp.h"
#include "src/tcp.h"
#include "src/udp.h"
#include "src/custom.h"
#include "src/thread.h"





// 解析地址端口 (格式: "IP:Port" 或 ":Port" 或 "http://IP:Port")
static void ParseAddr(const char* sAddr, char* sIP, size_t iIPSize, uint16* pPort)
{
	if ( sAddr == NULL || sAddr[0] == '\0' ) {
		strncpy(sIP, "0.0.0.0", iIPSize);
		*pPort = 80;
		return;
	}
	
	// 跳过 URL 协议前缀 (http://, https://, etc.)
	const char* p = sAddr;
	if ( strncmp(p, "http://", 7) == 0 ) {
		p += 7;
	} else if ( strncmp(p, "https://", 8) == 0 ) {
		p += 8;
	}
	
	// 查找最后一个冒号（端口分隔符）
	const char* pColon = strrchr(p, ':');
	if ( pColon == NULL ) {
		strncpy(sIP, p, iIPSize);
		sIP[iIPSize - 1] = '\0';
		*pPort = 80;
	} else if ( pColon == p ) {
		strncpy(sIP, "0.0.0.0", iIPSize);
		*pPort = (uint16)atoi(pColon + 1);
	} else {
		size_t iLen = pColon - p;
		if ( iLen >= iIPSize ) iLen = iIPSize - 1;
		strncpy(sIP, p, iLen);
		sIP[iLen] = '\0';
		*pPort = (uint16)atoi(pColon + 1);
	}
}



// 载入主机配置
void LoadHostConfig(xvalue objRoot, XS_HostObject objHost, XS_ServerObject objServer)
{
	const char* sPath;
	
	// 读取 Host 和 主机名
	str sHost = xvoTableGetText(objRoot, "host", 4);
	str sName = xvoTableGetText(objRoot, "name", 4);
	printf("    load host : %s (%s)\n", sHost, sName);
	
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
	objHost->Name = sName;
	objHost->Desc = xvoTableGetText(objRoot, "desc", 4);
	objHost->Param = xvoTableGetText(objRoot, "param", 5);
	objHost->Host = sHost;
	objHost->DebugMode = xvoTableGetBool(objRoot, "debug", 5);
	objHost->JsonNode = objRoot;
	
	// 创建 Host 映射
	if ( objHost->Host && (objHost->Host[0] != 0) ) {
		size_t iCount = 0;
		str* arrHost = xrtSplit(objHost->Host, 0, ";", 1, FALSE, &iCount);
		for ( int i = 0; i < iCount; i++ ) {
			size_t iRetSize = 0;
			char* sHostItem = xrtTrim(arrHost[i], 0, " \t\r\n", 4, FALSE, &iRetSize);
			bool bRet = xrtDictSetPtr(objServer->HostMap, sHostItem, iRetSize, objHost, NULL);
			if ( bRet == FALSE ) {
				printf("!!! ERROR !!! xrtDictSet Failed [HostMap] !\n");
				exit(EXIT_FAILURE);
			}
			xrtFree(sHostItem);
		}
		xrtFree(arrHost);
	}
	
	// 处理目录（相对路径转换为绝对路径）
	sPath = xvoTableGetText(objRoot, "path", 4);
	if ( (sPath == NULL) || (strlen(sPath) == 0) ) {
		printf("!!! ERROR !!! host (%s - %s) must specify a directory !\n", objServer->Name, objHost->Name);
		exit(EXIT_FAILURE);
	} else if ( xrtPathIsAbs((char*)sPath, 0) ) {
		objHost->Path = xrtCopyStr((char*)sPath, 0);
	} else {
		#ifdef PATH_MAX
			objHost->Path = malloc(PATH_MAX);
			if ( realpath(sPath, objHost->Path) == NULL ) {
				printf("!!! ERROR !!! realpath failed for path: %s\n", sPath);
				free(objHost->Path);
				objHost->Path = xrtCopyStr((char*)sPath, 0);
			}
		#else
			objHost->Path = malloc(4096);
			if ( realpath(sPath, objHost->Path) == NULL ) {
				printf("!!! ERROR !!! realpath failed for path: %s\n", sPath);
				free(objHost->Path);
				objHost->Path = xrtCopyStr((char*)sPath, 0);
			}
		#endif
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
		objHost->DevFile = xrtCopyStr((char*)sPath, 0);
	} else {
		#ifdef PATH_MAX
			objHost->DevFile = malloc(PATH_MAX);
			if ( realpath(sPath, objHost->DevFile) == NULL ) {
				printf("!!! ERROR !!! realpath failed for devfile: %s\n", sPath);
				free(objHost->DevFile);
				objHost->DevFile = xrtCopyStr((char*)sPath, 0);
			}
		#else
			objHost->DevFile = malloc(4096);
			if ( realpath(sPath, objHost->DevFile) == NULL ) {
				printf("!!! ERROR !!! realpath failed for devfile: %s\n", sPath);
				free(objHost->DevFile);
				objHost->DevFile = xrtCopyStr((char*)sPath, 0);
			}
		#endif
	}
	
	// 读取 TLS 证书配置
	if ( objServer->EnableTLS ) {
		memset(&objHost->tTlsConfig, 0, sizeof(xtlsconfig));
		
		const char* sCertPath = xvoTableGetText(objRoot, "tls_cert", 8);
		const char* sKeyPath = xvoTableGetText(objRoot, "tls_key", 7);
		
		if ( sCertPath && sCertPath[0] != '\0' ) {
			#ifdef PATH_MAX
				char sTempPath[PATH_MAX];
			#else
				char sTempPath[4096];
			#endif
			
			// 读取证书文件
			if ( realpath(sCertPath, sTempPath) != NULL ) {
				objHost->tTlsConfig.sCertFile = xrtCopyStr(sTempPath, 0);
			}
			
			// 读取私钥文件
			if ( sKeyPath && sKeyPath[0] != '\0' ) {
				if ( realpath(sKeyPath, sTempPath) != NULL ) {
					objHost->tTlsConfig.sKeyFile = xrtCopyStr(sTempPath, 0);
				}
			}
		}
	}
	
	// 加载动态开发入口文件
	if ( objHost->DevLang == SLT_C ) {
		DynLoad_C(objServer, objHost);
	} else {
		objHost->DevObj = NULL;
		objHost->ServiceInit = NULL;
		objHost->ServiceStart = NULL;
		objHost->ServiceUnit = NULL;
		objHost->EventProc = NULL;
		objHost->RequestProc = NULL;
		objHost->WsEventProc = NULL;
		objHost->XS_SetGlobalDate = NULL;
	}
}



// 释放 Host 资源
void FreeHostResource(XS_HostObject objHost)
{
	if ( objHost == NULL ) return;
	
	// 释放 TLS 证书路径
	if ( objHost->tTlsConfig.sCertFile ) {
		xrtFree((void*)objHost->tTlsConfig.sCertFile);
		objHost->tTlsConfig.sCertFile = NULL;
	}
	if ( objHost->tTlsConfig.sKeyFile ) {
		xrtFree((void*)objHost->tTlsConfig.sKeyFile);
		objHost->tTlsConfig.sKeyFile = NULL;
	}
	
	// 释放路径内存
	if ( objHost->Path ) {
		xrtFree(objHost->Path);
		objHost->Path = NULL;
	}
	if ( objHost->DevFile ) {
		xrtFree(objHost->DevFile);
		objHost->DevFile = NULL;
	}
	
	// 释放 TCC 状态机
	if ( objHost->DevObj ) {
		tcc_delete((TCCState*)objHost->DevObj);
		objHost->DevObj = NULL;
	}
}



// 载入服务器配置
bool LoadServerConfig(xvalue objRoot)
{
	// 检查服务器配置是否已启用
	if ( xvoTableGetBool(objRoot, "enabled", 7) == FALSE ) {
		printf("server enabled = false\n");
		return FALSE;
	}
	
	// 读取 服务名 和 服务描述
	str sName = xvoTableGetText(objRoot, "name", 4);
	str sDesc = xvoTableGetText(objRoot, "desc", 4);
	printf("\nload server : %s (%s)\n", sName, sDesc);
	
	// 读取网络协议类型
	int iClass = SPT_NONE;
	char* sClass = xvoTableGetText(objRoot, "class", 5);
	if ( strcasecmp(sClass, "http") == 0 ) {
		iClass = SPT_HTTP;
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
		return FALSE;
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
	objServer->Name = sName;
	objServer->Desc = sDesc;
	objServer->Param = xvoTableGetText(objRoot, "param", 5);
	objServer->Addr = xvoTableGetText(objRoot, "addr", 4);
	objServer->EnableTLS = xvoTableGetBool(objRoot, "tls", 3);
	if ( objServer->EnableTLS ) {
		objServer->AddrTLS = xvoTableGetText(objRoot, "addr_tls", 8);
	} else {
		objServer->AddrTLS = NULL;
	}
	objServer->JsonNode = objRoot;
	objServer->pServer = NULL;
	objServer->pServerTLS = NULL;
	
	// 读取默认主机配置
	xvalue objDefHost = xvoTableGetValue(objRoot, "host_default", 12);
	if ( objDefHost->Type != XVO_DT_TABLE ) {
		printf("host_default must be of type object !\n");
		exit(EXIT_FAILURE);
	}
	objServer->EnableDefaultHost = xvoTableGetBool(objDefHost, "enabled", 7);
	if ( objServer->EnableDefaultHost ) {
		LoadHostConfig(objDefHost, &objServer->DefaultHost, objServer);
	} else {
		memset(&objServer->DefaultHost, 0, sizeof(XS_HostStruct));
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
	objServer->HostCount = 0;
	objServer->Hosts = xrtArrayCreate(sizeof(XS_HostStruct));
	if ( objServer->Hosts == NULL ) {
		printf("!!! ERROR !!! HostList init failed !\n");
		exit(EXIT_FAILURE);
	}
	int iCount = xvoArrayItemCount(arrHost);
	xrtArrayAlloc(objServer->Hosts, iCount);
	
	// 遍历读取每一个主机的信息
	for ( int i = 0; i < iCount; i++ ) {
		xvalue objHostInfo = xvoArrayGetValue(arrHost, i);
		if ( objHostInfo->Type != XVO_DT_TABLE ) {
			printf("!!! ERROR !!! JSON Type no object [Host] (idx : %d) !\n", i);
			exit(EXIT_FAILURE);
		}
		if ( xvoTableGetBool(objHostInfo, "enabled", 7) == 0 ) {
			printf("    %s host enabled = false\n", xvoTableGetText(objHostInfo, "name", 4));
			continue;
		}
		unsigned int idx = xrtArrayAppend(objServer->Hosts, 1);
		if ( idx == 0 ) {
			printf("!!! ERROR !!! xrtArrayAppend Failed [HostList] !\n");
			exit(EXIT_FAILURE);
		}
		XS_HostObject objHost = xrtArrayGet_Inline(objServer->Hosts, idx);
		LoadHostConfig(objHostInfo, objHost, objServer);
		objServer->HostCount++;
	}
	return TRUE;
}



// 加载配置文件
int LoadConfig(str sOptFile)
{
	printf("load option file : %s\n", sOptFile);
	// 初始化 ServerList 数据结构
	ServerList = xrtArrayCreate(sizeof(XS_ServerStruct));
	if ( ServerList == NULL ) {
		printf("!!! ERROR !!! ServerList init failed !\n");
		exit(EXIT_FAILURE);
	}
	// 加载配置文件
	xvalue varJSON = xrtParseJSON_File(sOptFile);
	// xvoPrintValue(varJSON, 0, 0, 0, NULL);
	if ( varJSON->Type == XVO_DT_TABLE ) {
		// 单服务端口配置
		xrtArrayAlloc(ServerList, 1);
		if ( LoadServerConfig(varJSON) == 0 ) {
			printf("!!! ERROR !!! LoadServerConfig Failed !\n");
			exit(EXIT_FAILURE);
		}
	} else if ( varJSON->Type == XVO_DT_ARRAY ) {
		// 多服务端口配置
		size_t iCount = xvoArrayItemCount(varJSON);
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
	return 0;
}


// 信号处理 [Ctrl-C]
static volatile sig_atomic_t s_signo = 0;
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
	
	// 创建全局事件循环
	g_pLoop = xrtEventLoopCreate();
	if ( g_pLoop == NULL ) {
		printf("!!! ERROR !!! xrtEventLoopCreate failed !\n");
		exit(EXIT_FAILURE);
	}
	
	// 启动服务
	for ( int i = 1; i <= ServerList->Count; i++ ) {
		XS_ServerObject objServer = xrtArrayGet_Inline(ServerList, i);
		
		// 输出控制台日志
		printf("\nrun server : %s (%s) ...\n", objServer->Name, objServer->Desc);
		printf("    addr : %s\n", objServer->Addr);
		if ( objServer->EnableTLS ) {
			printf("    addr(tls) : %s\n", objServer->AddrTLS);
		}
		printf("    path : %s\n", objServer->DefaultHost.Path);
		
		// 服务初始化脚本回调函数
		DynLoad_C_GlobalData(objServer, &objServer->DefaultHost);
		if ( objServer->DefaultHost.ServiceInit ) {
			void (*ServiceInit)(XS_ServerObject objServer, XS_HostObject objHost) = objServer->DefaultHost.ServiceInit;
			ServiceInit(objServer, &objServer->DefaultHost);
		}
		for ( int j = 1; j <= objServer->HostCount; j++ ) {
			XS_HostObject objHost = xrtArrayGet_Inline(objServer->Hosts, j);
			printf("    init host : %s (%s)\n", objHost->Host, objHost->Name);
			DynLoad_C_GlobalData(objServer, objHost);
			if ( objHost->ServiceInit ) {
				void (*ServiceInit)(XS_ServerObject objServer, XS_HostObject objHost) = objHost->ServiceInit;
				ServiceInit(objServer, objHost);
			}
		}
		
		// 启动服务
		if ( objServer->Class == SPT_HTTP ) {
			RunServerHTTP(objServer);
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
	printf("\n");
	
	// 事件循环 (使用 xrt 事件循环)
	while ( s_signo == 0 ) {
		xrtEventLoopRunOnce(g_pLoop, 1000);
	}
	
	// 停止服务
	for ( int i = 1; i <= ServerList->Count; i++ ) {
		XS_ServerObject objServer = xrtArrayGet_Inline(ServerList, i);
		// 服务卸载脚本回调函数
		if ( objServer->DefaultHost.ServiceUnit ) {
			void (*ServiceUnit)(XS_ServerObject objServer, XS_HostObject objHost) = objServer->DefaultHost.ServiceUnit;
			ServiceUnit(objServer, &objServer->DefaultHost);
		}
		for ( int j = 1; j <= objServer->HostCount; j++ ) {
			XS_HostObject objHost = xrtArrayGet_Inline(objServer->Hosts, j);
			if ( objHost->ServiceUnit ) {
				void (*ServiceUnit)(XS_ServerObject objServer, XS_HostObject objHost) = objHost->ServiceUnit;
				ServiceUnit(objServer, objHost);
			}
		}
		// 卸载服务
		if ( objServer->Class == SPT_HTTP ) {
			StopServerHTTP(objServer);
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
		// 释放 Host 资源
		FreeHostResource(&objServer->DefaultHost);
		for ( int j = 1; j <= objServer->HostCount; j++ ) {
			XS_HostObject objHost = xrtArrayGet_Inline(objServer->Hosts, j);
			FreeHostResource(objHost);
		}
		// 释放 Host 列表和映射表
		if ( objServer->Hosts ) {
			xrtArrayDestroy(objServer->Hosts);
		}
		if ( objServer->HostMap ) {
			xrtDictDestroy(objServer->HostMap);
		}
	}
	
	// 销毁事件循环
	xrtEventLoopDestroy(g_pLoop);
	g_pLoop = NULL;
	
	// 释放服务器列表
	xrtArrayDestroy(ServerList);
	printf("Exiting on signal %d\n", s_signo);
	return 0;
}



void OnError(str sError)
{
	printf("X Runtime Error : %s\n", sError);
}



int main(int argc, char** argv)
{
	#if defined(_WIN32) || defined(_WIN64)
		SetConsoleOutputCP(65001);
	#endif
	
	// 初始化 xrt 库
	xrtInit();
	xCore.OnError = OnError;
	
	// 从命令行中读取配置文件路径
	str sOptFile;
	bool bFreeOptFile = FALSE;
	if ( argc > 1 ) {
		sOptFile = argv[1];
	} else {
		sOptFile = xrtPathJoin(2, xCore.AppPath, "xs.json");
		bFreeOptFile = TRUE;
	}
	LoadConfig(sOptFile);
	if ( bFreeOptFile ) {
		xrtFree(sOptFile);
	}
	
	// 启动服务器
	RunServer();
	
	// 卸载 xrt 库
	xrtUnit();
	return 0;
}


