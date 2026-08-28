#ifndef XS_CORE_ENGINE_H
#define XS_CORE_ENGINE_H

/*
 * xs3 引擎生命周期与应用路径
 * 设计依据：docs/设计.md §3.2（单进程单常驻引擎）§10.4（停机）
 *
 * 软重载永不重启引擎；引擎 Stop/Destroy 仅发生在进程退出。
 * xrt 保证：仍有活动网络对象时 Stop/Destroy 拒绝执行 —— 停机路径的双保险。
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <windows.h>
#else
	#include <unistd.h>
#endif

/* 应用路径（xs 可执行文件所在目录，末尾不带分隔符）；失败返回 "." */
static const char* XS_AppPath(void)
{
	static char sPath[4096];
	static bool bInit = false;

	if ( !bInit ) {
	#if defined(_WIN32) || defined(_WIN64)
		DWORD iLen = GetModuleFileNameA(NULL, sPath, (DWORD)(sizeof(sPath) - 1));
		char* sSlash;
		if ( iLen == 0 || iLen >= sizeof(sPath) - 1 ) {
			snprintf(sPath, sizeof(sPath), ".");
			return sPath;
		}
		sPath[iLen] = '\0';
		sSlash = strrchr(sPath, '\\');
		if ( sSlash == NULL ) {
			sSlash = strrchr(sPath, '/');
		}
		if ( sSlash != NULL ) {
			*sSlash = '\0';
		} else {
			snprintf(sPath, sizeof(sPath), ".");
		}
	#else
		ssize_t iLen = readlink("/proc/self/exe", sPath, sizeof(sPath) - 1);
		char* sSlash;
		if ( iLen <= 0 ) {
			snprintf(sPath, sizeof(sPath), ".");
			return sPath;
		}
		sPath[iLen] = '\0';
		sSlash = strrchr(sPath, '/');
		if ( sSlash != NULL ) {
			*sSlash = '\0';
		} else {
			snprintf(sPath, sizeof(sPath), ".");
		}
	#endif
		bInit = true;
	}
	return sPath;
}

/* 契约 API：xsAppPath */
const char* xsAppPath(void)
{
	return XS_AppPath();
}

/* 引擎启动：创建即启动，回填到 XS_App 与全部 server */
static bool XS_EngineStartup(XS_App* pApp)
{
	xnetengineconfig tCfg;
	uint32 i;

	xrtNetEngineConfigInit(&tCfg);
	if ( pApp->EngineWorkers > 0 ) {
		tCfg.Workers = pApp->EngineWorkers;
	}
	pApp->Engine = xrtNetEngineCreate(&tCfg);
	if ( pApp->Engine == NULL ) {
		printf("[xs] engine create failed\n");
		return false;
	}
	if ( !xrtNetEngineStart(pApp->Engine) ) {
		printf("[xs] engine start failed\n");
		xrtNetEngineDestroy(pApp->Engine);
		pApp->Engine = NULL;
		return false;
	}
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		pApp->Servers[i]->Engine = pApp->Engine;
	}
	printf("[xs] engine started, workers=%u\n", xrtNetEngineWorkerCount(pApp->Engine));
	return true;
}

/* 引擎停机：等待 LiveObjects 排空（listener/流的异步 Close 完成）→ Stop → Destroy。
 * xrt 保证有活对象时 Stop 拒绝；此处轮询排空是过渡方案，
 * 正式 drain 判据随"运行时与重载"工作包的连接注册表落地（设计 §10.3）。 */
static void XS_EngineShutdown(XS_App* pApp)
{
	xnetenginestats tStats;
	int iWait;

	if ( pApp->Engine == NULL ) {
		return;
	}
	for ( iWait = 0; iWait < 50; iWait++ ) {
		memset(&tStats, 0, sizeof(tStats));
		if ( !xrtNetEngineStats(pApp->Engine, &tStats) || tStats.LiveObjects == 0 ) {
			break;
		}
		xrtSleep(100);
	}
	if ( !xrtNetEngineStop(pApp->Engine) ) {
		printf("[xs] engine stop rejected (live objects remain)\n");
		return;
	}
	xrtNetEngineDestroy(pApp->Engine);
	pApp->Engine = NULL;
	printf("[xs] engine stopped\n");
}

#endif
