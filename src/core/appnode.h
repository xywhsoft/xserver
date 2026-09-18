#ifndef XS_CORE_APPNODE_H
#define XS_CORE_APPNODE_H

/*
 * app 节点前端调度（docs/Webview扩展与app节点设计.md §2/§4/§9）
 *
 * 窗口层仅存在于 XS_USE_WEBVIEW + Win32 的构建变体，本体在
 * src/webview/xs_app_window.c（经 webview_impl.cpp 的 C ABI 出口调用
 * webview 库）。其余变体与平台一律 headless：服务照常、日志给出可打开
 * 的 URL，配置永远合法——"不生效"是窗口不出现，不是启动失败。
 *
 * 四个入口由 main.c 在固定顺序调用：Init(配置装载后) → Ready/Fail(装配
 * 结果) → Unit(停机链最前)。窗口层任何失败只降级，不影响服务本体。
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "../sdk/xsbase.h"
#include "config.h"

/* main.c 定义的进程停机标志；UI 线程关窗后经 XS_RequestStop 置位。
 * 类型与定义处严格一致（sig_atomic_t），避免跨翻译单元类型分歧。 */
extern volatile sig_atomic_t g_XS_Stop;
void XS_RequestStop(void);

static bool XS_ServerIsApp(const XS_ServerInfo* pServer)
{
	return pServer != NULL && strcmp(pServer->Class, "app") == 0;
}

/* window 描述符为固化字段（设计 §3）：reload 改动不生效也不参与端点判定，
 * 但必须明确提示需重启——序列化两侧 Custom.window 对比（提示性日志，
 * 键序极端差异下的误报无害）。仅在新旧皆为 app 服务时调用。 */
static void XS_AppWindowDiffNote(const XS_ServerInfo* pOld, const XS_ServerInfo* pNew)
{
	xvalue* pWinA = (pOld->Custom != NULL)
		? xrtValueObjectGet(pOld->Custom, XRT_STR_LITERAL("window")) : NULL;
	xvalue* pWinB = (pNew->Custom != NULL)
		? xrtValueObjectGet(pNew->Custom, XRT_STR_LITERAL("window")) : NULL;
	str sA = NULL, sB = NULL;

	if ( (pWinA == NULL) != (pWinB == NULL)
		|| (pWinA != NULL && xrtValueType(pWinA) != XVALUE_OBJECT)
		|| (pWinB != NULL && xrtValueType(pWinB) != XVALUE_OBJECT) ) {
		/* 一侧无 window 或类型异常：视为变更提示 */
	}
	else if ( pWinA != NULL ) {
		sA = xrtJsonStringify(pWinA, false, NULL);
		sB = xrtJsonStringify(pWinB, false, NULL);
		if ( sA != NULL && sB != NULL && strcmp(sA, sB) == 0 ) {
			xrtFree(sA);
			xrtFree(sB);
			return;	/* 未变更 */
		}
	}
	else {
		return;	/* 双侧均无 window */
	}
	xrtFree((void*)sA);
	xrtFree((void*)sB);
	printf("[xs] app '%s': window fields changed; restart required to apply\n",
		pNew->Name != NULL ? pNew->Name : "?");
}

/* 目标 URL：window.url（缺省 "/"）为绝对地址时直用，相对路径拼
 * http://ip:port。窗口后端与 headless 日志共用。
 * 注意不得让 snprintf 的源与目标重叠（曾因此产生自我放大输出）。 */
static void XS_AppUrlBuild(const XS_ServerInfo* pServer, char* sOut, size_t iCap)
{
	const char* sIp = (pServer->IP != NULL && pServer->IP[0] != '\0')
		? pServer->IP : "127.0.0.1";
	char sUrl[768] = "/";
	xvalue* pWindow = (pServer->Custom != NULL)
		? xrtValueObjectGet(pServer->Custom, XRT_STR_LITERAL("window")) : NULL;

	if ( pWindow != NULL && xrtValueType(pWindow) == XVALUE_OBJECT ) {
		xvalue* pVal = xrtValueObjectGet(pWindow, XRT_STR_LITERAL("url"));
		xstrview tView;

		if ( pVal != NULL && xrtValueGetString(pVal, &tView)
			&& tView.Size > 0 && tView.Size < sizeof sUrl ) {
			snprintf(sUrl, sizeof sUrl, "%.*s",
				(int)tView.Size, (const char*)tView.Data);
		}
	}
	if ( strncmp(sUrl, "http://", 7) == 0 || strncmp(sUrl, "https://", 8) == 0 ) {
		snprintf(sOut, iCap, "%s", sUrl);
		return;
	}
	snprintf(sOut, iCap, "http://%s:%u%s", sIp,
		(unsigned int)(pServer->PortBound != 0 ? pServer->PortBound
			: pServer->Port), sUrl);
}

#if defined(XS_USE_WEBVIEW) && defined(_WIN32)

/* 窗口后端（src/webview/xs_app_window.c） */
bool XS_AppWindowBoot(XS_App* pApp);	/* 运行时检测 + install_fallback 兜底 */
void XS_AppWindowOpen(XS_App* pApp);	/* loading 窗（装配期间） */
void XS_AppWindowNavigate(XS_App* pApp);	/* Ready：跳转目标 URL */
void XS_AppWindowFail(const char* sErr);	/* 错误页 */
void XS_AppWindowWaitClosed(void);	/* 失败页驻留至用户关窗 */
void XS_AppWindowClose(void);		/* 停机：dispatch 终止 + join UI 线程 */

static void XS_AppFrontendInit(XS_App* pApp)
{
	if ( !XS_AppWindowBoot(pApp) ) return;	/* 已 headless 降级（含二次实例唤起） */
	XS_AppWindowOpen(pApp);
}

static void XS_AppFrontendReady(XS_App* pApp)
{
	XS_AppWindowNavigate(pApp);
}

static void XS_AppFrontendFail(const char* sErr)
{
	XS_AppWindowFail(sErr);
	XS_AppWindowWaitClosed();	/* 错误页可读，关窗即继续退出 */
}

static void XS_AppFrontendUnit(void)
{
	XS_AppWindowClose();
}

#else	/* headless 降级：无窗口、不依赖任何 GUI 库 */

static void XS_AppFrontendInit(XS_App* pApp)
{
	(void)pApp;
}

static void XS_AppFrontendReady(XS_App* pApp)
{
	char sUrl[1024];
	uint32 i;

	for ( i = 0; pApp != NULL && i < pApp->ServerCount; i++ ) {
		const XS_ServerInfo* pServer = pApp->Servers[i];

		if ( pServer == NULL || !pServer->Enabled || !XS_ServerIsApp(pServer) )
			continue;
		XS_AppUrlBuild(pServer, sUrl, sizeof(sUrl));
		printf("[xs] app '%s': open %s in a browser\n",
			pServer->Name, sUrl);
	}
}

static void XS_AppFrontendFail(const char* sErr)
{
	printf("[xs] app frontend: startup failed: %s\n",
		(sErr != NULL && sErr[0] != '\0') ? sErr : "unknown error");
}

static void XS_AppFrontendUnit(void)
{
}

/* TCC 契约符号（/xs/xs_webview.h + import_webview.inc）在 headless 变体的
 * 定义；窗口变体（XS_USE_WEBVIEW + _WIN32）由 src/webview/xs_app_window.c
 * 提供，两者互斥编译，不冲突。 */
int xsWebviewAvailable(void)
{
	return 0;
}

int xsWebviewVersion(void)
{
	return 0;
}

#endif

#endif /* XS_CORE_APPNODE_H */
