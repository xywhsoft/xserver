/*
 * xs3 —— xrt 的落地化部署工具
 * 设计依据：docs/设计.md
 *
 * 骨架阶段主流程：配置装载 → 数据模型 → 引擎启动 → 待机 → 优雅停机。
 * 协议装配（http/ws/tcp/udp/custom）、TCC 脚本宿主、软重载按实施计划后续接入。
 */

#define XRT_MODULE_ALL









#define XRT_IMPLEMENTATION
#include "lib/xrt.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "src/sdk/xsbase.h"
#include "src/core/config.h"
#include "src/core/engine.h"
#include "src/core/api.h"
#include "src/core/assemble.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <windows.h>
	#include <shellapi.h>
#endif

static volatile bool g_XS_Stop = false;


#if defined(_WIN32) || defined(_WIN64)
/* Windows 命令行参数是 ANSI（中文系统为 GBK），而 xrt 路径统一 UTF-8：
 * 入口处用宽字符命令行重建 UTF-8 argv，杜绝中文路径乱码（见设计 §16 开发规约）。
 * 该分配随进程生命周期，不释放 */
static char** XS_BuildUtf8Argv(int* piArgc)
{
	int iWideArgc = 0;
	LPWSTR* pWideArgv = CommandLineToArgvW(GetCommandLineW(), &iWideArgc);
	char** pArgv;
	int i;
	int iLen;

	if ( pWideArgv == NULL || iWideArgc <= 0 ) {
		LocalFree(pWideArgv);
		return NULL;
	}
	pArgv = (char**)xrtCalloc((size_t)iWideArgc + 1, sizeof(char*));
	if ( pArgv == NULL ) {
		LocalFree(pWideArgv);
		return NULL;
	}
	for ( i = 0; i < iWideArgc; i++ ) {
		iLen = WideCharToMultiByte(CP_UTF8, 0, pWideArgv[i], -1, NULL, 0, NULL, NULL);
		if ( iLen <= 0 ) {
			continue;
		}
		pArgv[i] = (char*)xrtMalloc((size_t)iLen);
		if ( pArgv[i] != NULL ) {
			WideCharToMultiByte(CP_UTF8, 0, pWideArgv[i], -1, pArgv[i], iLen, NULL, NULL);
		}
	}
	pArgv[iWideArgc] = NULL;
	*piArgc = iWideArgc;
	LocalFree(pWideArgv);
	return pArgv;
}
#endif

#if defined(_WIN32) || defined(_WIN64)
static BOOL WINAPI XS_ConsoleProc(DWORD dwCtrlType)
{
	(void)dwCtrlType;
	g_XS_Stop = true;
	return TRUE;
}
#endif

static void XS_SignalProc(int iSignal)
{
	(void)iSignal;
	g_XS_Stop = true;
}

static void XS_Usage(void)
{
	printf("xs - xrt deployment host\n");
	printf("usage: xs [config]\n");
	printf("  config   path to xs.json (default: <appdir>/xs.json)\n");
}

int main(int argc, char** argv)
{
	XS_App tApp;
	char sConfigPath[4200];
	const char* sArgConfig = NULL;
	int i;

	setvbuf(stdout, NULL, _IONBF, 0);
#if defined(_WIN32) || defined(_WIN64)
	{
		int iUtf8Argc = 0;
		char** pUtf8Argv = XS_BuildUtf8Argv(&iUtf8Argc);

		if ( pUtf8Argv != NULL ) {
			argc = iUtf8Argc;
			argv = pUtf8Argv;
		}
		SetConsoleOutputCP(CP_UTF8);
	}
#endif

	for ( i = 1; i < argc; i++ ) {
		if ( strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 ) {
			XS_Usage();
			return 0;
		} else if ( sArgConfig == NULL ) {
			sArgConfig = argv[i];
		} else {
			printf("[xs] unexpected argument: %s\n", argv[i]);
			XS_Usage();
			return 1;
		}
	}

	if ( sArgConfig != NULL ) {
		snprintf(sConfigPath, sizeof(sConfigPath), "%s", sArgConfig);
	} else {
		snprintf(sConfigPath, sizeof(sConfigPath), "%s/xs.json", XS_AppPath());
	}

	/* 配置装载（fail-fast，见设计 §5.4） */
	if ( !XS_ConfigLoad(sConfigPath, &tApp) ) {
		printf("[xs] config load failed: %s\n", tApp.ParseError);
		XS_ConfigFree(&tApp);
		return 1;
	}
	printf("[xs] config loaded: %s (%u servers)\n", sConfigPath, tApp.ServerCount);
	if ( tApp.ServerCount == 0 ) {
		printf("[xs] warning: no services configured\n");
	}
	XS_ConfigDump(&tApp);

	/* 引擎启动（进程级，常驻；见设计 §3.2） */
	if ( !XS_EngineStartup(&tApp) ) {
		XS_ConfigFree(&tApp);
		return 1;
	}
	g_XS_App = &tApp;
	if ( !XS_ReloadRuntimeInit() ) {
		printf("[xs] reload runtime init failed\n");
		XS_EngineShutdown(&tApp);
		XS_ConfigFree(&tApp);
		return 1;
	}


	/* 装配（本轮：custom 完整路径；其余协议驱动后续接入） */
	if ( !XS_AssembleServers(&tApp) ) {
		printf("[xs] assemble failed, exit\n");
		XS_ServersDrain(&tApp);
		XS_ShutdownServers(&tApp);
		XS_EngineShutdown(&tApp);
		XS_ShutdownAfterEngine(&tApp);
		XS_ReloadRuntimeUnit();
		XS_ConfigFree(&tApp);
		return 1;
	}

	/* 信号与待机 */
#if defined(_WIN32) || defined(_WIN64)
	SetConsoleCtrlHandler(XS_ConsoleProc, TRUE);
#endif
	signal(SIGINT, XS_SignalProc);
	signal(SIGTERM, XS_SignalProc);

	printf("[xs] running, press Ctrl+C to stop\n");
	while ( !g_XS_Stop ) {
		xrtSleep(100);
	}

	/* 优雅停机：驱动收口 → ServiceUnit → 排空销毁 TCC → 引擎 Stop/Destroy → 配置释放 */
	printf("[xs] stopping\n");
	XS_ServersDrain(&tApp);
	XS_ShutdownServers(&tApp);
	XS_EngineShutdown(&tApp);
	XS_ShutdownAfterEngine(&tApp);
	XS_ReloadRuntimeUnit();
	XS_ConfigFree(&tApp);
	printf("[xs] bye\n");
	return 0;
}
