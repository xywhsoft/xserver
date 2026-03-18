#include <stdio.h>
#include <signal.h>



// 定义 XRT_IMPLEMENTATION 导入功能实现
#define XRT_IMPLEMENTATION
#include "lib/xrt.h"

// vNext 支撑层
#include "src/support/log.h"
#include "src/support/validate.h"
#include "src/support/path.h"

// vNext 核心骨架
#include "src/core/error.h"
#include "src/core/host.h"
#include "src/core/server.h"
#include "src/core/config.h"
#include "src/core/bus.h"
#include "src/core/reload.h"

// vNext 脚本层
#include "src/script/tcc_host.h"
#include "src/script/script_api.h"
#include "src/script/dynload.h"

// vNext 协议层
#include "src/protocol/http.h"
#include "src/protocol/ws.h"
#include "src/protocol/tcp.h"
#include "src/protocol/udp.h"
#include "src/protocol/xtp.h"
#include "src/protocol/custom.h"

// vNext 运行时
#include "src/core/runtime.h"



void OnError(str sError)
{
	printf("X Runtime Error : %s\n", sError);
}



static volatile sig_atomic_t g_iXsStopFlag = 0;

static void XS_SignalHandler(int iSignal)
{
	g_iXsStopFlag = iSignal;
}



int main(int argc, char** argv)
{
	XS_Config objCfg;
	XS_Runtime objRuntime;
	char* sOptFile = NULL;
	bool bUseDefaultConfig = FALSE;
	bool bCheckOnly = FALSE;
	int iExitCode = 0;
	
	#if defined(_WIN32) || defined(_WIN64)
		SetConsoleOutputCP(65001);
	#endif
	
	xrtInit();
	xCore.OnError = OnError;
	
	XS_ResetErrors();
	
	if ( argc > 1 && argv[1] && strcmp(argv[1], "--check") == 0 ) {
		bCheckOnly = TRUE;
		if ( argc > 2 && argv[2] ) {
			sOptFile = argv[2];
		}
	}
	else if ( argc > 1 && argv[1] ) {
		sOptFile = argv[1];
	}
	
	if ( sOptFile == NULL ) {
		sOptFile = xrtPathJoin(2, xCore.AppPath, "xs.json");
		bUseDefaultConfig = TRUE;
	}
	
	if ( bCheckOnly ) {
		XS_LogInfo("mode : config check");
	} else {
		XS_LogInfo("mode : normal");
	}
	
	XS_LogInfo("XServer vNext bootstrap");
	XS_LogInfo("loading config : %s", sOptFile);
	
	if ( !XS_BusInit() ) {
		XS_LogError("bus init failed");
		iExitCode = 5;
		goto ExitMain;
	}
	
	if ( !XS_LoadConfig(&objCfg, sOptFile) ) {
		XS_LogError("config load failed");
		iExitCode = 1;
		goto ExitMain;
	}
	
	XS_PrintConfigSummary(&objCfg);
	
	if ( bCheckOnly ) {
		XS_LogInfo("config check passed");
		goto ExitConfig;
	}
	
	if ( !XS_RuntimeBuild(&objRuntime, &objCfg) ) {
		XS_LogError("runtime build failed");
		iExitCode = 2;
		goto ExitConfig;
	}
	
	XS_RuntimePrint(&objRuntime);
	
	if ( !XS_RuntimeInitServers(&objRuntime) ) {
		XS_LogError("runtime init failed");
		iExitCode = 3;
		goto ExitRuntime;
	}
	
	if ( !XS_RuntimeStartServers(&objRuntime) ) {
		XS_LogError("runtime start failed");
		iExitCode = 4;
		goto ExitRuntime;
	}
	
	signal(SIGINT, XS_SignalHandler);
	signal(SIGTERM, XS_SignalHandler);
	
	XS_LogInfo("stage complete : runtime entered serving loop");
	XS_LogInfo("press Ctrl+C to stop");
	
	while ( g_iXsStopFlag == 0 ) {
		XS_BusDispatchMessages(objRuntime.Servers);
		xrtSleep(200);
	}
	
	XS_LogInfo("stop signal received : %d", (int)g_iXsStopFlag);
	XS_BusDispatchMessages(objRuntime.Servers);
	XS_RuntimeStopServers(&objRuntime);
ExitRuntime:
	XS_FreeRuntime(&objRuntime);
ExitConfig:
	XS_FreeConfig(&objCfg);
ExitMain:
	XS_BusUnit();
	if ( bUseDefaultConfig && sOptFile ) {
		xrtFree(sOptFile);
	}
	
	xrtUnit();
	return iExitCode;
}
