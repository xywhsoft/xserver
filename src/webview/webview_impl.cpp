/* webview_impl.cpp — webview 0.12 实现单元（单 TU，仅 Windows 平台编译）。
 *
 * 以 -I lib/webview 包含 vendored 头把 C++ 内核编进 xs；对外只暴露
 * 下面的 C ABI 出口，宿主其余代码保持纯 C（xs_app_window.c 为唯一客户）。
 * 注意：webview 官方要求传已有窗口前调用方先 CoInitializeEx(STA)，
 * 由 xs_app_window.c 的 UI 线程入口负责，不在此处隐藏。 */
#include "webview/webview.h"
#include "xs_webview.h"

/* 版本钉子一致性门禁：vendored 头的版本宏必须与 xs_webview.h 的
 * XS_WEBVIEW_VERSION_NUMBER 相符——升级 webview 库时忘同步会编译失败。 */
static_assert(XS_WEBVIEW_VERSION_NUMBER ==
	WEBVIEW_VERSION_MAJOR * 10000 + WEBVIEW_VERSION_MINOR * 100 +
	WEBVIEW_VERSION_PATCH,
	"lib/webview version drifted from src/webview/xs_webview.h pin");

extern "C" {

int xsWebviewImplVersion(void)
{
	return WEBVIEW_VERSION_MAJOR * 10000 + WEBVIEW_VERSION_MINOR * 100
		+ WEBVIEW_VERSION_PATCH;
}

/* ---- C ABI 薄封装：指针透传，语义与 webview C API 一致 ---- */

void* xsWvCreate(int iDebug)
{
	return webview_create(iDebug, nullptr);
}

void xsWvDestroy(void* pWebview)
{
	if ( pWebview != nullptr ) webview_destroy((webview_t)pWebview);
}

void xsWvRun(void* pWebview)
{
	webview_run((webview_t)pWebview);
}

/* 终止必须运行在 UI 线程：0.12.0 的 terminate 是 PostQuitMessage，
 * 只作用于调用线程队列；跨线程请经 xsWvDispatch marshal。 */
void xsWvTerminate(void* pWebview)
{
	webview_terminate((webview_t)pWebview);
}

/* webview_t 即 void*（vendored 头 typedef），回调两参皆为指针——
 * 与 xsWv* 契约签名类型全同，直接透传，无转型无堆分配。 */
void xsWvDispatch(void* pWebview, void (*pFn)(void* pWv, void* pArg), void* pArg)
{
	webview_dispatch((webview_t)pWebview, pFn, pArg);
}

void xsWvNavigate(void* pWebview, const char* sUrl)
{
	webview_navigate((webview_t)pWebview, sUrl);
}

void xsWvSetTitle(void* pWebview, const char* sTitle)
{
	webview_set_title((webview_t)pWebview, sTitle);
}

void xsWvSetSize(void* pWebview, int iWidth, int iHeight, int iHint)
{
	webview_set_size((webview_t)pWebview, iWidth, iHeight,
		(webview_hint_t)iHint);
}

void* xsWvGetWindow(void* pWebview)
{
	return webview_get_window((webview_t)pWebview);	/* Windows 下即 HWND */
}

}	/* extern "C" */
