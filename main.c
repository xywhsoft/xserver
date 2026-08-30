/*
 * xs3 —— xrt 的落地化部署工具
 * 设计依据：docs/设计.md
 *
 * 主流程：配置装载 → 常驻引擎 → 协议/TCC 装配 → reload controller
 * → generation 排空的优雅停机。
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

static volatile sig_atomic_t g_XS_Stop = 0;


#if defined(_WIN32) || defined(_WIN64)
/* Windows 命令行参数是 ANSI（中文系统为 GBK），而 xrt 路径统一 UTF-8：
 * 入口处用宽字符命令行重建 UTF-8 argv，杜绝中文路径乱码（见设计 §16 开发规约）。 */
static char** XS_BuildUtf8Argv(int* piArgc)
{
	int iWideArgc = 0;
	LPWSTR* pWideArgv = CommandLineToArgvW(GetCommandLineW(), &iWideArgc);
	char** pArgv;
	int i;
	int iLen;

	if ( piArgc == NULL ) return NULL;
	*piArgc = 0;
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
		iLen = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			pWideArgv[i], -1, NULL, 0, NULL, NULL);
		if ( iLen <= 0 ) goto Failed;
		pArgv[i] = (char*)xrtMalloc((size_t)iLen);
		if ( pArgv[i] == NULL ||
		     WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pWideArgv[i], -1,
			pArgv[i], iLen, NULL, NULL) != iLen ) goto Failed;
	}
	pArgv[iWideArgc] = NULL;
	*piArgc = iWideArgc;
	LocalFree(pWideArgv);
	return pArgv;

Failed:
	xrtFree(pArgv[i]);
	while ( i > 0 ) xrtFree(pArgv[--i]);
	xrtFree(pArgv);
	LocalFree(pWideArgv);
	return NULL;
}
#endif

static int XS_MainExit(int iCode, char** pOwnedArgv, int iOwnedArgc)
{
#if defined(_WIN32) || defined(_WIN64)
	if ( pOwnedArgv != NULL ) {
		for ( int i = 0; i < iOwnedArgc; i++ ) xrtFree(pOwnedArgv[i]);
		xrtFree(pOwnedArgv);
	}
#else
	(void)pOwnedArgv;
	(void)iOwnedArgc;
#endif
	return iCode;
}

#if defined(_WIN32) || defined(_WIN64)
static BOOL WINAPI XS_ConsoleProc(DWORD dwCtrlType)
{
	(void)dwCtrlType;
	g_XS_Stop = 1;
	return TRUE;
}
#endif

static void XS_SignalProc(int iSignal)
{
	(void)iSignal;
	g_XS_Stop = 1;
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
	char** pOwnedArgv = NULL;
	int iOwnedArgc = 0;
	int i;

	setvbuf(stdout, NULL, _IONBF, 0);
#if !defined(_WIN32) && !defined(_WIN64)
	/* Linux sendfile 没有 MSG_NOSIGNAL；对端 RST 必须作为连接发送失败返回，
	 * 不能让默认 SIGPIPE 终止整个服务器进程。 */
	(void)signal(SIGPIPE, SIG_IGN);
#endif
#if defined(_WIN32) || defined(_WIN64)
	{
		int iUtf8Argc = 0;
		char** pUtf8Argv = XS_BuildUtf8Argv(&iUtf8Argc);

		if ( pUtf8Argv != NULL ) {
			argc = iUtf8Argc;
			argv = pUtf8Argv;
			pOwnedArgv = pUtf8Argv;
			iOwnedArgc = iUtf8Argc;
		}
		SetConsoleOutputCP(CP_UTF8);
	}
#endif

	for ( i = 1; i < argc; i++ ) {
		if ( strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 ) {
			XS_Usage();
			return XS_MainExit(0, pOwnedArgv, iOwnedArgc);
		} else if ( sArgConfig == NULL ) {
			sArgConfig = argv[i];
		} else {
			printf("[xs] unexpected argument: %s\n", argv[i]);
			XS_Usage();
			return XS_MainExit(1, pOwnedArgv, iOwnedArgc);
		}
	}

	if ( sArgConfig != NULL ) {
		i = snprintf(sConfigPath, sizeof(sConfigPath), "%s", sArgConfig);
	} else {
		const char* sAppPath = XS_AppPath();

		i = sAppPath != NULL ? snprintf(sConfigPath, sizeof(sConfigPath),
			"%s/xs.json", sAppPath) : -1;
	}
	if ( i < 0 || (size_t)i >= sizeof(sConfigPath) ) {
		printf("[xs] config path is too long\n");
		return XS_MainExit(1, pOwnedArgv, iOwnedArgc);
	}

	/* 配置装载（fail-fast，见设计 §5.4） */
	if ( !XS_ConfigLoad(sConfigPath, &tApp) ) {
		printf("[xs] config load failed: %s\n", tApp.ParseError);
		XS_ConfigFree(&tApp);
		return XS_MainExit(1, pOwnedArgv, iOwnedArgc);
	}
	printf("[xs] config loaded: %s (%u servers)\n", sConfigPath, tApp.ServerCount);
	if ( tApp.ServerCount == 0 ) {
		printf("[xs] warning: no services configured\n");
	}
	XS_ConfigDump(&tApp);

	/* 引擎启动（进程级，常驻；见设计 §3.2） */
	if ( !XS_EngineStartup(&tApp) ) {
		XS_ConfigFree(&tApp);
		return XS_MainExit(1, pOwnedArgv, iOwnedArgc);
	}
	if ( !XS_GenerationReaperInit() ) {
		printf("[xs] generation reaper init failed\n");
		XS_EngineShutdown(&tApp);
		XS_ConfigFree(&tApp);
		return XS_MainExit(1, pOwnedArgv, iOwnedArgc);
	}
	if ( !XS_TlsRuntimeInit() ) {
		printf("[xs] tls runtime init failed\n");
		XS_GenerationReaperUnit();
		XS_EngineShutdown(&tApp);
		XS_ConfigFree(&tApp);
		return XS_MainExit(1, pOwnedArgv, iOwnedArgc);
	}
	g_XS_App = &tApp;
	if ( !XS_TopologyRuntimeInit(&tApp) ) {
		printf("[xs] topology runtime init failed\n");
		XS_EngineShutdown(&tApp);
		XS_GenerationReaperUnit();
		XS_TlsRuntimeUnit();
		XS_ConfigFree(&tApp);
		return XS_MainExit(1, pOwnedArgv, iOwnedArgc);
	}
	if ( !XS_ReloadRuntimeInit(&tApp, sConfigPath) ) {
		printf("[xs] reload runtime init failed\n");
		XS_EngineShutdown(&tApp);
		XS_TopologyRuntimeUnit();
		XS_GenerationReaperUnit();
		XS_TlsRuntimeUnit();
		XS_ConfigFree(&tApp);
		return XS_MainExit(1, pOwnedArgv, iOwnedArgc);
	}


	/* 所有初始服务就绪后才开放 reload admission。 */
	if ( !XS_AssembleServers(&tApp) ) {
		printf("[xs] assemble failed, exit\n");
		XS_ServersDrain(&tApp);
		XS_ShutdownServers(&tApp);
		XS_EngineShutdown(&tApp);
		XS_ShutdownAfterEngine(&tApp);
		XS_GenerationReaperDrain();
		XS_ReloadRuntimeUnit();
		XS_TopologyRuntimeUnit();
		XS_GenerationReaperUnit();
		XS_TlsRuntimeUnit();
		XS_ConfigFree(&tApp);
		return XS_MainExit(1, pOwnedArgv, iOwnedArgc);
	}
	XS_ReloadRuntimeStart();

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

	/* 优雅停机：停止接入并退役 builtin → 等待网络终态并停止引擎 →
	 * 退役 custom → 排空 reaper 上的 ServiceUnit/终析构 → 配置释放。 */
	printf("[xs] stopping\n");
	XS_ServersDrain(&tApp);
	XS_ShutdownServers(&tApp);
	XS_EngineShutdown(&tApp);
	XS_ShutdownAfterEngine(&tApp);
	XS_GenerationReaperDrain();
	XS_ReloadRuntimeUnit();
	XS_TopologyRuntimeUnit();
	XS_GenerationReaperUnit();
	XS_TlsRuntimeUnit();
	XS_ConfigFree(&tApp);
	printf("[xs] bye\n");
	return XS_MainExit(0, pOwnedArgv, iOwnedArgc);
}
