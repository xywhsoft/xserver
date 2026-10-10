/* xs_app_window.c — app 节点窗口后端（仅 XS_USE_WEBVIEW + Win32 编译）。
 *
 * 职责（设计 §4~§7/§9）：每 app 节点一个 UI 线程（STA + 消息泵），
 * 装配期显示 loading 页，装配成功经 dispatch 跳转目标 URL，失败显示
 * 错误页；close=stop 关窗即 XS_RequestStop，close=hide 隐藏 + 命名事件
 * 再唤起（单实例）；缺 Evergreen Runtime 时按 install_fallback 三态兜底
 * （迁移自 docs/webview_reference/wv2_fallback_ui.c，实测同源）。
 * 窗口层永远 best-effort：任何失败只降级为日志，不影响服务本体。
 * webview 库经 webview_impl.cpp 的 C ABI 出口调用，本文件纯 C。
 *
 * 线程契约：dispatch 回调在 UI 线程异步执行，所有回调参数必须指向
 * 跨越异步时刻仍存活的存储（窗口记录自带缓冲），禁止传栈地址。 */
#include <windows.h>
#include <commctrl.h>
#include <urlmon.h>
#include <wchar.h>
#include <stdlib.h>

#include "src/core/appnode.h"
#include "xs_webview.h"

/* webview_impl.cpp 提供的 C ABI 出口（webview 库薄封装，语义一致） */
extern void* xsWvCreate(int iDebug);
extern void xsWvDestroy(void* pWebview);
extern void xsWvRun(void* pWebview);
extern void xsWvTerminate(void* pWebview);
extern void xsWvDispatch(void* pWebview,
	void (*pFn)(void* pWv, void* pArg), void* pArg);
extern void xsWvNavigate(void* pWebview, const char* sUrl);
extern void xsWvSetTitle(void* pWebview, const char* sTitle);
extern void xsWvSetSize(void* pWebview, int iWidth, int iHeight, int iHint);
extern void* xsWvGetWindow(void* pWebview);

/* TCC 探测契约（/xs/xs_webview.h + import_webview.inc）：只回答构建期事实 */
int xsWebviewAvailable(void)
{
	return 1;	/* 本文件仅在 XS_USE_WEBVIEW + _WIN32 变体编译 */
}

int xsWebviewVersion(void)
{
	return XS_WEBVIEW_VERSION_NUMBER;
}

#define XS_MAX_APP_WINDOWS 32

/* ============================================================
 * WebView2 Evergreen Runtime 检测与兜底安装
 * ============================================================ */

#define WV2_URL L"https://go.microsoft.com/fwlink/p/?LinkId=2124703"
#define WV2_KEY L"{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"

static const WCHAR WV2_SUB_MAIN[] = L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\" WV2_KEY;
static const WCHAR WV2_SUB_WOW[] = L"SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\" WV2_KEY;

static int wv2_pv_ok(HKEY hRoot, const WCHAR* sSub)
{
	WCHAR sPv[64];
	DWORD iSize = sizeof sPv;

	if ( RegGetValueW(hRoot, sSub, L"pv", RRF_RT_REG_SZ, NULL, sPv, &iSize)
		!= ERROR_SUCCESS ) {
		return 0;
	}
	return sPv[0] != L'\0' && wcscmp(sPv, L"0.0.0.0") != 0;
}

static int wv2_runtime_present(void)
{
	/* 测试钩子（设计 §10）：无管理员权限下模拟 Runtime 缺失，
	 * 覆盖 install_fallback 三态与 headless 降级路径。 */
	if ( getenv("XS_WV2_FORCE_MISSING") != NULL ) return 0;
	return wv2_pv_ok(HKEY_CURRENT_USER, WV2_SUB_MAIN)
		|| wv2_pv_ok(HKEY_LOCAL_MACHINE, WV2_SUB_MAIN)
		|| wv2_pv_ok(HKEY_LOCAL_MACHINE, WV2_SUB_WOW);
}

/* ---- 兜底安装的提示/进度窗（单线程模型：等待点就地泵消息）---- */

static HWND g_hFbWnd, g_hFbLabel, g_hFbBar;
static volatile LONG g_iFbCancel;

static void fb_pump(void)
{
	MSG tMsg;

	while ( PeekMessageW(&tMsg, NULL, 0, 0, PM_REMOVE) ) {
		TranslateMessage(&tMsg);
		DispatchMessageW(&tMsg);
	}
}

static LRESULT CALLBACK fb_wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ( uMsg == WM_CLOSE ) {	/* 吞掉关闭，由下载回调检测取消 */
		g_iFbCancel = 1;
		return 0;
	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static void fb_window_create(const WCHAR* sTitle)
{
	INITCOMMONCONTROLSEX tIcc = { sizeof tIcc, ICC_BAR_CLASSES };
	WNDCLASSW tWc;
	HINSTANCE hInst = GetModuleHandleW(NULL);

	InitCommonControlsEx(&tIcc);
	ZeroMemory(&tWc, sizeof tWc);
	tWc.lpfnWndProc = fb_wnd_proc;
	tWc.hInstance = hInst;
	tWc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
	tWc.lpszClassName = L"xs_wv2_fallback";
	RegisterClassW(&tWc);
	g_hFbWnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"xs_wv2_fallback",
		sTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, 420, 150,
		NULL, NULL, hInst, NULL);
	g_hFbLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
		18, 16, 370, 20, g_hFbWnd, NULL, hInst, NULL);
	g_hFbBar = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE,
		18, 48, 370, 18, g_hFbWnd, NULL, hInst, NULL);
	SendMessageW(g_hFbLabel, WM_SETFONT,
		(WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
	ShowWindow(g_hFbWnd, SW_SHOWNORMAL);
	fb_pump();
}

static void fb_set_text(const WCHAR* sText)
{
	SetWindowTextW(g_hFbLabel, sText);
	fb_pump();
}

static void fb_window_destroy(void)
{
	DestroyWindow(g_hFbWnd);
	UnregisterClassW(L"xs_wv2_fallback", GetModuleHandleW(NULL));
	fb_pump();
}

/* IBindStatusCallback：真实百分比 + 就地泵消息保持响应 */
static HRESULT STDMETHODCALLTYPE fb_query_interface(IBindStatusCallback* pThis,
	REFIID riid, void** ppv) { (void)pThis; (void)riid; (void)ppv; return E_NOINTERFACE; }
static ULONG STDMETHODCALLTYPE fb_addref(IBindStatusCallback* pThis) { (void)pThis; return 1; }
static ULONG STDMETHODCALLTYPE fb_release(IBindStatusCallback* pThis) { (void)pThis; return 1; }
static HRESULT STDMETHODCALLTYPE fb_on_start(IBindStatusCallback* pThis,
	DWORD r, IBinding* b) { (void)pThis; (void)r; (void)b; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE fb_get_priority(IBindStatusCallback* pThis,
	LONG* p) { (void)pThis; (void)p; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE fb_on_low_resource(IBindStatusCallback* pThis,
	DWORD r) { (void)pThis; (void)r; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE fb_on_progress(IBindStatusCallback* pThis,
	ULONG ulCur, ULONG ulMax, ULONG ulCode, LPCWSTR szText)
{
	WCHAR sBuf[128];

	(void)pThis; (void)ulCode; (void)szText;
	fb_pump();
	if ( g_iFbCancel ) return E_ABORT;
	if ( ulMax ) {
		SendMessageW(g_hFbBar, PBM_SETMARQUEE, FALSE, 0);
		SendMessageW(g_hFbBar, PBM_SETRANGE32, 0, (LPARAM)ulMax);
		SendMessageW(g_hFbBar, PBM_SETPOS, (WPARAM)ulCur, 0);
		_snwprintf(sBuf, 127, L"正在下载组件：%lu KB / %lu KB",
			ulCur / 1024, ulMax / 1024);
	}
	else {
		SendMessageW(g_hFbBar, PBM_SETMARQUEE, TRUE, 30);
		wcscpy(sBuf, L"正在下载组件…（服务器未提供总大小）");
	}
	SetWindowTextW(g_hFbLabel, sBuf);
	return S_OK;
}
static HRESULT STDMETHODCALLTYPE fb_on_stop(IBindStatusCallback* pThis,
	HRESULT hr, LPCWSTR e) { (void)pThis; (void)hr; (void)e; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE fb_get_bind_info(IBindStatusCallback* pThis,
	DWORD* f, BINDINFO* b) { (void)pThis; (void)f; (void)b; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE fb_on_data(IBindStatusCallback* pThis,
	DWORD f, DWORD sz, FORMATETC* fe, STGMEDIUM* st) {
	(void)pThis; (void)f; (void)sz; (void)fe; (void)st; return E_NOTIMPL;
}
static HRESULT STDMETHODCALLTYPE fb_on_object(IBindStatusCallback* pThis,
	REFIID riid, IUnknown* p) { (void)pThis; (void)riid; (void)p; return E_NOTIMPL; }

static IBindStatusCallbackVtbl g_fbVtbl = {
	fb_query_interface, fb_addref, fb_release,
	fb_on_start, fb_get_priority, fb_on_low_resource, fb_on_progress,
	fb_on_stop, fb_get_bind_info, fb_on_data, fb_on_object
};
static IBindStatusCallback g_fbCallback = { &g_fbVtbl };

/* 静默运行引导器并等待完成；per-user 安装不需要管理员权限。
 * 进度窗存在时跑马灯并泵消息（安装器不回报进度）。 */
static int wv2_install_silent(const WCHAR* sExe)
{
	WCHAR sCmd[MAX_PATH + 32];
	STARTUPINFOW tSi;
	PROCESS_INFORMATION tPi;
	DWORD iCode = 1;

	_snwprintf(sCmd, MAX_PATH + 31, L"\"%s\" /silent /install", sExe);
	ZeroMemory(&tSi, sizeof tSi);
	tSi.cb = sizeof tSi;
	if ( !CreateProcessW(NULL, sCmd, NULL, NULL, FALSE, 0, NULL, NULL,
		&tSi, &tPi) ) {
		return 0;
	}
	SendMessageW(g_hFbBar, PBM_SETMARQUEE, TRUE, 30);
	while ( WaitForSingleObject(tPi.hProcess, 50) == WAIT_TIMEOUT )
		fb_pump();
	GetExitCodeProcess(tPi.hProcess, &iCode);
	CloseHandle(tPi.hThread);
	CloseHandle(tPi.hProcess);
	return iCode == 0;
}

/* 引导器下载 + 静默安装，全程带进度窗（无进度阶段跑马灯，关窗=取消）。 */
static int wv2_install_with_ui(void)
{
	WCHAR sDir[MAX_PATH], sExe[MAX_PATH + 16];
	int bOk = 0;

	if ( GetTempPathW(MAX_PATH, sDir) == 0 ) return 0;
	_snwprintf(sExe, MAX_PATH + 15, L"%swv2setup.exe", sDir);
	fb_window_create(L"WebView2 组件安装");
	fb_set_text(L"正在连接下载服务器…");
	if ( SUCCEEDED(URLDownloadToFileW(NULL, WV2_URL, sExe, 0, &g_fbCallback))
		&& !g_iFbCancel ) {
		fb_set_text(L"正在安装运行时（可能需要几分钟）…");
		bOk = wv2_install_silent(sExe);
	}
	fb_window_destroy();
	DeleteFileW(sExe);
	return bOk && !g_iFbCancel && wv2_runtime_present();
}

/* ============================================================
 * 窗口记录与 UI 线程
 * ============================================================ */

enum { XS_APP_CLOSE_STOP = 0, XS_APP_CLOSE_HIDE };
enum { XS_APP_FB_ASK = 0, XS_APP_FB_SILENT, XS_APP_FB_HEADLESS };

/* 前端导航状态：主线程只写状态与缓冲（Interlocked 全屏障保序），
 * UI 线程创建完成后自查——解决 Ready/Fail 早于 WebView2 初始化完成
 * 的竞态（窗口就绪前状态已置，则直接导航而非 loading 页）。 */
enum { XS_APP_NAV_LOADING = 0, XS_APP_NAV_URL, XS_APP_NAV_FAIL, XS_APP_NAV_SHUTDOWN };

typedef struct XS_AppWindow {
	XS_ServerInfo*		pServer;	/* 配置快照借用，只读 */
	HANDLE			hThread;
	volatile void*		pWebview;	/* 仅 UI 线程写，创建完成后其余线程可读 */
	volatile LONG		iNavState;	/* XS_APP_NAV_* */
	HWND			hWnd;		/* 同上 */
	WNDPROC			pOrigProc;	/* close=hide 子类化原_proc */
	char			sTitle[128];
	char			sTargetUrl[1024];	/* 状态缓冲：写缓冲在置状态之前 */
	char			sFailPage[2560];
	int			iWidth, iHeight, iMinWidth, iMinHeight;
	int			iClose;
	int			iDebug;
	volatile LONG		iAlive;
} XS_AppWindow;

static XS_AppWindow g_tWindows[XS_MAX_APP_WINDOWS];
static int g_iWindowCount;
static volatile LONG g_iLiveStopWindows;	/* close=stop 存活计数 */
static HANDLE g_hSingleMutex, g_hWakeEvent;
static void* g_pWaitRegistration;

/* 与 lib/webview webview_hint_t 数值对齐：NONE=0 MIN=1 MAX=2 FIXED=3。
 * MIN 误写为 3（FIXED）会把窗口缩到 min 尺寸并剥掉 WS_THICKFRAME。 */
#define WV_HINT_NONE 0
#define WV_HINT_MIN  1

/* loading 页：data: URI 内联（转圈 + 文案），不依赖任何服务 */
static const char* const g_sLoadingPage =
	"data:text/html;charset=utf-8,%3Chtml%3E%3Cbody%20style%3D%22margin%3A0%3B"
	"background%3A%2320252b%3Bcolor%3A%23dfe3e8%3Bfont-family%3A"
	"system-ui%2Csans-serif%3Bdisplay%3Aflex%3Bflex-direction%3Acolumn%3B"
	"align-items%3Acenter%3Bjustify-content%3Acenter%3Bheight%3A100vh%22%3E"
	"%3Cdiv%20style%3D%22width%3A36px%3Bheight%3A36px%3Bborder%3A3px%20solid%20"
	"%23444c56%3Bborder-top-color%3A%236ea8fe%3Bborder-radius%3A50%25%3B"
	"animation%3As%201s%20linear%20infinite%22%3E%3C%2Fdiv%3E"
	"%3Cp%20style%3D%22margin-top%3A18px%22%3E%E6%AD%A3%E5%9C%A8%E5%90%AF"
	"%E5%8A%A8%E6%9C%8D%E5%8A%A1%E2%80%A6%3C%2Fp%3E%3Cstyle%3E"
	"%40keyframes%20s%7Bto%7Btransform%3Arotate(360deg)%7D%7D%3C%2Fstyle%3E"
	"%3C%2Fbody%3E%3C%2Fhtml%3E";

/* 错误页：消息转义（%/<>/&/"/'）后嵌入 data: URI */
static void app_error_page(const char* sErr, char* sOut, size_t iCap)
{
	static const char sHead[] =
		"data:text/html;charset=utf-8,%3Chtml%3E%3Cbody%20style%3D%22margin%3A0%3B"
		"background%3A%2320252b%3Bcolor%3A%23e8a0a0%3Bfont-family%3A"
		"system-ui%2Csans-serif%3Bpadding%3A32px%22%3E%3Ch2%3E%E5%90%AF"
		"%E5%8A%A8%E5%A4%B1%E8%B4%A5%3C%2Fh2%3E%3Cpre%20style%3D%22white-space%3A"
		"pre-wrap%3Bcolor%3A%23dfe3e8%22%3E";
	static const char sTail[] =
		"%3C%2Fpre%3E%3Cp%3E%E8%AF%A6%E8%A7%81%E6%8E%A7%E5%88%B6%E5%8F%B0"
		"%E6%97%A5%E5%BF%97%EF%BC%9B%E5%85%B3%E9%97%AD%E7%AA%97%E5%8F%A3"
		"%E9%80%80%E5%87%BA%E3%80%82%3C%2Fp%3E%3C%2Fbody%3E%3C%2Fhtml%3E";
	size_t iPos = 0, iHead = sizeof sHead - 1, iTail = sizeof sTail - 1;
	size_t iBodyCap = iCap - iTail - 1;

	memcpy(sOut, sHead, iHead);
	iPos = iHead;
	for ( ; sErr != NULL && *sErr != '\0' && iPos + 3 < iBodyCap; sErr++ ) {
		unsigned char c = (unsigned char)*sErr;

		if ( c == '<' ) { memcpy(sOut + iPos, "%3C", 3); iPos += 3; }
		else if ( c == '>' ) { memcpy(sOut + iPos, "%3E", 3); iPos += 3; }
		else if ( c == '&' ) { memcpy(sOut + iPos, "%26", 3); iPos += 3; }
		else if ( c == '"' ) { memcpy(sOut + iPos, "%22", 3); iPos += 3; }
		else if ( c == '\'' ) { memcpy(sOut + iPos, "%27", 3); iPos += 3; }
		else if ( c == '%' ) { memcpy(sOut + iPos, "%25", 3); iPos += 3; }
		/* ASCII 可见字符与 UTF-8 多字节序列原样透传（charset=utf-8） */
		else { sOut[iPos++] = (char)c; }
	}
	memcpy(sOut + iPos, sTail, iTail);
	sOut[iPos + iTail] = '\0';
}

/* close=hide：拦 WM_CLOSE 隐藏，再唤起经事件显示 */
/* ---- 窗口状态持久化：exe 同目录 window-state-<app名>.txt（便携） ----
   格式：x,y,w,h,max ；坏值/越屏直接丢弃回退默认。 */
static void app_state_path(const char* sName, char* pOut, size_t iCap)
{
	WCHAR sExe[MAX_PATH + 1];
	char sExeA[MAX_PATH + 1];
	DWORD n;
	size_t i;

	n = GetModuleFileNameW(NULL, sExe, MAX_PATH);
	if ( n == 0 || n >= MAX_PATH ) { snprintf(pOut, iCap, "window-state-%s.txt", sName); return; }
	WideCharToMultiByte(CP_UTF8, 0, sExe, -1, sExeA, sizeof sExeA, NULL, NULL);
	for ( i = strlen(sExeA); i > 0; i-- )
		if ( sExeA[i - 1] == '\\' || sExeA[i - 1] == '/' ) break;
	sExeA[i] = 0;
	snprintf(pOut, iCap, "%swindow-state-%s.txt", sExeA, sName);
}

static bool app_state_load(const char* sName, int* pX, int* pY, int* pW, int* pH, int* pMax)
{
	char sPath[MAX_PATH + 64];
	FILE* f;
	int x = 0, y = 0, w = 0, h = 0, m = 0;

	app_state_path(sName, sPath, sizeof sPath);
	f = fopen(sPath, "rb");
	if ( f == NULL ) return false;
	if ( fscanf(f, "%d,%d,%d,%d,%d", &x, &y, &w, &h, &m) != 5 ) { fclose(f); return false; }
	fclose(f);
	if ( w < 200 || h < 150 || w > 32767 || h > 32767 ) return false;
	/* 越屏防护：至少有一个角落在虚拟桌面内（显示器拔除后不复活到虚空） */
	if ( x < GetSystemMetrics(SM_XVIRTUALSCREEN) - 40
		|| y < GetSystemMetrics(SM_YVIRTUALSCREEN) - 40
		|| x + w > GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN) + 40
		|| y + h > GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN) + 40 )
		return false;
	*pX = x; *pY = y; *pW = w; *pH = h; *pMax = m;
	return true;
}

static void app_state_save(HWND hWnd, const char* sName)
{
	char sPath[MAX_PATH + 64];
	RECT r;
	FILE* f;
	int m;

	if ( hWnd == NULL || !GetWindowRect(hWnd, &r) ) return;
	m = IsZoomed(hWnd) ? 1 : 0;
	if ( IsIconic(hWnd) ) return;   /* 最小化时保存无意义 */
	app_state_path(sName, sPath, sizeof sPath);
	f = fopen(sPath, "wb");
	if ( f == NULL ) return;
	fprintf(f, "%ld,%ld,%ld,%ld,%d\n",
		(long)r.left, (long)r.top, (long)(r.right - r.left), (long)(r.bottom - r.top), m);
	fclose(f);
}

/* 按 HWND 反查窗口记录：不能占用窗口的 GWLP_USERDATA（WebView2 内部槽位，
   抢占会在 STOP 模式关闭路径崩溃——实测 c0000005）。 */
static XS_AppWindow* app_win_by_hwnd(HWND hWnd)
{
	int i;

	for ( i = 0; i < g_iWindowCount; i++ )
		if ( g_tWindows[i].hWnd == hWnd && g_tWindows[i].iAlive )
			return &g_tWindows[i];
	return NULL;
}

static LRESULT CALLBACK app_wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	XS_AppWindow* pWin = app_win_by_hwnd(hWnd);

	/* 关闭前保存窗口状态（此刻窗口仍存活，矩形可读） */
	if ( pWin != NULL && uMsg == WM_CLOSE )
		app_state_save(hWnd, pWin->pServer->Name);
	if ( pWin != NULL && uMsg == WM_CLOSE
		&& pWin->iClose == XS_APP_CLOSE_HIDE ) {
		ShowWindow(hWnd, SW_HIDE);
		return 0;
	}
	if ( pWin != NULL )
		return CallWindowProcW(pWin->pOrigProc,
			hWnd, uMsg, wParam, lParam);
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static DWORD WINAPI app_ui_thread(LPVOID pParam)
{
	XS_AppWindow* pWin = (XS_AppWindow*)pParam;
	LONG iState;

	/* webview 官方契约：STA 前置（设计 §14 M0 记录第 3 条） */
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	pWin->pWebview = xsWvCreate(pWin->iDebug);
	if ( pWin->pWebview == NULL ) {
		printf("[xs] app '%s': webview create failed, headless\n",
			pWin->pServer->Name);
		CoUninitialize();
		if ( pWin->iClose == XS_APP_CLOSE_STOP )
			InterlockedDecrement(&g_iLiveStopWindows);
		pWin->iAlive = 0;
		return 0;
	}
	xsWvSetTitle((void*)pWin->pWebview, pWin->sTitle);
	if ( pWin->iWidth > 0 && pWin->iHeight > 0 )
		xsWvSetSize((void*)pWin->pWebview, pWin->iWidth, pWin->iHeight, WV_HINT_NONE);
	if ( pWin->iMinWidth > 0 && pWin->iMinHeight > 0 )
		xsWvSetSize((void*)pWin->pWebview, pWin->iMinWidth, pWin->iMinHeight, WV_HINT_MIN);
	pWin->hWnd = (HWND)xsWvGetWindow((void*)pWin->pWebview);
	/* 常驻子类化：hide 语义 + 所有窗口的关闭前状态保存 */
	if ( pWin->hWnd != NULL ) {
		int x = 0, y = 0, w = 0, h = 0, m = 0;

		if ( app_state_load(pWin->pServer->Name, &x, &y, &w, &h, &m) ) {
			SetWindowPos(pWin->hWnd, NULL, x, y, w, h,
				SWP_NOZORDER | SWP_NOACTIVATE);
			if ( m ) ShowWindow(pWin->hWnd, SW_MAXIMIZE);
		}
		pWin->pOrigProc = (WNDPROC)SetWindowLongPtrW(pWin->hWnd,
			GWLP_WNDPROC, (LONG_PTR)app_wnd_proc);
	}
	/* 创建完成，自查导航状态：早于本线程完成的通知在此生效 */
	iState = InterlockedCompareExchange(&pWin->iNavState, 0, 0);
	if ( iState == XS_APP_NAV_URL )
		xsWvNavigate((void*)pWin->pWebview, pWin->sTargetUrl);
	else if ( iState == XS_APP_NAV_FAIL )
		xsWvNavigate((void*)pWin->pWebview, pWin->sFailPage);
	else if ( iState == XS_APP_NAV_SHUTDOWN )
		xsWvTerminate((void*)pWin->pWebview);
	else
		xsWvNavigate((void*)pWin->pWebview, g_sLoadingPage);
	if ( iState != XS_APP_NAV_SHUTDOWN )
		xsWvRun((void*)pWin->pWebview);	/* 窗口销毁（或 dispatch 终止）时返回 */
	xsWvDestroy((void*)pWin->pWebview);
	pWin->pWebview = NULL;
	CoUninitialize();
	if ( pWin->iClose == XS_APP_CLOSE_STOP
		&& InterlockedDecrement(&g_iLiveStopWindows) == 0 ) {
		XS_RequestStop();	/* 全部 stop 型窗口已关闭：请求优雅停机 */
	}
	pWin->iAlive = 0;
	return 0;
}

/* dispatch 回调（UI 线程执行；参数一律指向窗口记录内缓冲） */
static void app_job_navigate(void* pWv, void* pArg)
{
	xsWvNavigate(pWv, ((XS_AppWindow*)pArg)->sTargetUrl);
}

static void app_job_fail(void* pWv, void* pArg)
{
	xsWvNavigate(pWv, ((XS_AppWindow*)pArg)->sFailPage);
}

static void app_job_terminate(void* pWv, void* pArg)
{
	(void)pArg;
	xsWvTerminate(pWv);	/* 必须在 UI 线程执行（M0 实测结论） */
}

static void app_job_show(void* pWv, void* pArg)
{
	HWND hWnd = (HWND)pArg;

	(void)pWv;
	ShowWindow(hWnd, SW_SHOW);
	/* Windows 前台锁定策略可能拒绝抢占前台：降级为任务栏闪烁提醒 */
	if ( !SetForegroundWindow(hWnd) ) {
		FLASHWINFO tFlash;

		tFlash.cbSize = sizeof tFlash;
		tFlash.hwnd = hWnd;
		tFlash.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
		tFlash.uCount = 5;
		tFlash.dwTimeout = 0;
		FlashWindowEx(&tFlash);
	}
}

static void app_dispatch(XS_AppWindow* pWin, void (*pFn)(void*, void*), void* pArg)
{
	if ( pWin != NULL && pWin->pWebview != NULL )
		xsWvDispatch((void*)pWin->pWebview, pFn, pArg);
}

/* 唤起事件（线程池回调）：marshal 到各 UI 线程显示 hide 窗口 */
static VOID CALLBACK app_wake_cb(PVOID pParam, BOOLEAN bTimeout)
{
	int i;

	(void)pParam; (void)bTimeout;
	for ( i = 0; i < g_iWindowCount; i++ ) {
		XS_AppWindow* pWin = &g_tWindows[i];

		if ( pWin->iClose == XS_APP_CLOSE_HIDE && pWin->iAlive
			&& pWin->hWnd != NULL ) {
			app_dispatch(pWin, app_job_show, pWin->hWnd);
		}
	}
}

/* 单实例：按 exe 路径哈希命名互斥与唤起事件（Local\ 会话内）。 */
static unsigned long app_path_hash(void)
{
	WCHAR sPath[MAX_PATH + 1];
	unsigned long h = 2166136261u;
	WCHAR* p;

	if ( GetModuleFileNameW(NULL, sPath, MAX_PATH) == 0 ) return 0;
	for ( p = sPath; *p != L'\0'; p++ ) {
		h ^= (unsigned long)*p;
		h *= 16777619u;
	}
	return h;
}

/* 返回 false = 已有实例在跑（已发唤起信号，本进程应尽快退出）。 */
static bool app_single_instance_acquire(void)
{
	WCHAR sMutex[64], sEvent[64];
	unsigned long h = app_path_hash();

	_snwprintf(sMutex, 63, L"Local\\xs_app_single_%08lx", h);
	_snwprintf(sEvent, 63, L"Local\\xs_app_wake_%08lx", h);
	g_hWakeEvent = CreateEventW(NULL, FALSE, FALSE, sEvent);
	g_hSingleMutex = CreateMutexW(NULL, TRUE, sMutex);
	if ( g_hSingleMutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS ) {
		printf("[xs] app: another instance running, wake signal sent, exit\n");
		if ( g_hWakeEvent != NULL ) SetEvent(g_hWakeEvent);
		return false;
	}
	return true;
}

static void app_single_instance_watch(void)
{
	if ( g_hWakeEvent == NULL || g_pWaitRegistration != NULL ) return;
	RegisterWaitForSingleObject((PHANDLE)&g_pWaitRegistration, g_hWakeEvent,
		app_wake_cb, NULL, INFINITE, WT_EXECUTEDEFAULT);
}

/* 测试钩子线程体：与用户关窗完全同路径（WM_CLOSE / UI 线程终止），
 * 逐窗关闭以覆盖多窗口引用计数语义（全部关窗才停机）。 */
static DWORD WINAPI app_autoclose_thread(LPVOID pParam)
{
	int i;

	Sleep((int)(intptr_t)pParam);
	for ( i = 0; i < g_iWindowCount; i++ ) {
		XS_AppWindow* pWin = &g_tWindows[i];

		if ( pWin->hWnd != NULL )
			PostMessageW(pWin->hWnd, WM_CLOSE, 0, 0);
		else if ( pWin->pWebview != NULL )
			app_dispatch(pWin, app_job_terminate, NULL);
	}
	return 0;
}

/* ============================================================
 * window 配置读取（留在 Custom，不弹为预设字段）
 * ============================================================ */

static xvalue* app_window_object(const XS_ServerInfo* pServer)
{
	xvalue* pWin = (pServer->Custom != NULL)
		? xrtValueObjectGet(pServer->Custom, XRT_STR_LITERAL("window")) : NULL;

	return (pWin != NULL && xrtValueType(pWin) == XVALUE_OBJECT) ? pWin : NULL;
}

static void app_cfg_str(xvalue* pWin, const char* sKey,
	char* sOut, size_t iCap, const char* sDefault)
{
	xvalue* pVal = xrtValueObjectGet(pWin,
		xrtStrViewN(sKey, strlen(sKey)));
	xstrview tView;

	if ( pVal != NULL && xrtValueGetString(pVal, &tView) && tView.Size > 0 ) {
		snprintf(sOut, iCap, "%.*s", (int)tView.Size, (const char*)tView.Data);
	}
	else {
		snprintf(sOut, iCap, "%s", sDefault);
	}
}

static int app_cfg_int(xvalue* pWin, const char* sKey,
	int iDefault, int iMin, int iMax)
{
	xvalue* pVal = xrtValueObjectGet(pWin,
		xrtStrViewN(sKey, strlen(sKey)));
	int64 iValue;

	if ( pVal != NULL && xrtValueGetInt(pVal, &iValue) ) {
		if ( iValue < iMin ) iValue = iMin;
		if ( iValue > iMax ) iValue = iMax;
		return (int)iValue;
	}
	return iDefault;
}

static int app_cfg_bool(xvalue* pWin, const char* sKey, int iDefault)
{
	xvalue* pVal = xrtValueObjectGet(pWin,
		xrtStrViewN(sKey, strlen(sKey)));
	bool bValue;

	/* JSON true/false 是 XVALUE_BOOL，精确型 GetInt 会失败——必须走 GetBool */
	if ( pVal != NULL && xrtValueGetBool(pVal, &bValue) )
		return bValue ? 1 : 0;
	return iDefault;
}

/* ============================================================
 * 对 appnode.h 的后端入口
 * ============================================================ */

static XS_ServerInfo* app_first_server(XS_App* pApp)
{
	uint32 i;

	for ( i = 0; i < pApp->ServerCount; i++ ) {
		XS_ServerInfo* pServer = pApp->Servers[i];

		if ( pServer != NULL && pServer->Enabled && XS_ServerIsApp(pServer) )
			return pServer;
	}
	return NULL;
}

bool XS_AppWindowBoot(XS_App* pApp)
{
	XS_ServerInfo* pFirst = app_first_server(pApp);
	xvalue* pWin;
	char sFallback[16];
	int iFallback = XS_APP_FB_ASK;

	if ( pFirst == NULL ) return false;	/* 无 app 节点：本层静默 */
	if ( !app_single_instance_acquire() ) {
		XS_RequestStop();	/* 二次实例：唤醒既有窗口后请求退出 */
		return false;
	}
	if ( wv2_runtime_present() ) return true;

	pWin = app_window_object(pFirst);
	if ( pWin != NULL ) {
		app_cfg_str(pWin, "install_fallback", sFallback, sizeof sFallback, "ask");
		if ( strcmp(sFallback, "silent") == 0 ) iFallback = XS_APP_FB_SILENT;
		else if ( strcmp(sFallback, "headless") == 0 ) iFallback = XS_APP_FB_HEADLESS;
	}
	if ( iFallback == XS_APP_FB_HEADLESS ) {
		printf("[xs] app: WebView2 Runtime missing, headless "
			"(install_fallback=headless)\n");
		return false;
	}
	if ( iFallback == XS_APP_FB_ASK
		&& MessageBoxW(NULL,
			L"系统缺少 WebView2 运行时（约 240 MB），需要联网下载安装。\n"
			L"现在安装吗？（取消则转为浏览器访问）",
			L"xs", MB_OKCANCEL | MB_ICONQUESTION) != IDOK ) {
		printf("[xs] app: WebView2 install declined, headless\n");
		return false;
	}
	printf("[xs] app: installing WebView2 Runtime (%s)…\n",
		iFallback == XS_APP_FB_SILENT ? "silent" : "ask");
	if ( !wv2_install_with_ui() ) {
		printf("[xs] app: WebView2 install failed, headless\n");
		return false;
	}
	printf("[xs] app: WebView2 Runtime installed\n");
	return true;
}

void XS_AppWindowOpen(XS_App* pApp)
{
	uint32 i;

	for ( i = 0; i < pApp->ServerCount && g_iWindowCount < XS_MAX_APP_WINDOWS; i++ ) {
		XS_ServerInfo* pServer = pApp->Servers[i];
		XS_AppWindow* pWin;
		xvalue* pCfg;
		char sClose[16];

		if ( pServer == NULL || !pServer->Enabled || !XS_ServerIsApp(pServer) )
			continue;
		pWin = &g_tWindows[g_iWindowCount];
		ZeroMemory(pWin, sizeof *pWin);
		pWin->pServer = pServer;
		pCfg = app_window_object(pServer);
		if ( pCfg != NULL ) {
			app_cfg_str(pCfg, "title", pWin->sTitle, sizeof pWin->sTitle, "xs app");
			app_cfg_str(pCfg, "url", pWin->sTargetUrl, sizeof pWin->sTargetUrl, "/");
			pWin->iWidth = app_cfg_int(pCfg, "width", 1280, 200, 32767);
			pWin->iHeight = app_cfg_int(pCfg, "height", 800, 200, 32767);
			pWin->iMinWidth = app_cfg_int(pCfg, "min_width", 0, 0, 32767);
			pWin->iMinHeight = app_cfg_int(pCfg, "min_height", 0, 0, 32767);
			pWin->iDebug = app_cfg_bool(pCfg, "devtools", 0);
			app_cfg_str(pCfg, "close", sClose, sizeof sClose, "stop");
			pWin->iClose = strcmp(sClose, "hide") == 0
				? XS_APP_CLOSE_HIDE : XS_APP_CLOSE_STOP;
		}
		else {
			snprintf(pWin->sTitle, sizeof pWin->sTitle, "xs app");
			snprintf(pWin->sTargetUrl, sizeof pWin->sTargetUrl, "/");
			pWin->iWidth = 1280;
			pWin->iHeight = 800;
			pWin->iClose = XS_APP_CLOSE_STOP;
		}
		pWin->iAlive = 1;
		if ( pWin->iClose == XS_APP_CLOSE_STOP )
			g_iLiveStopWindows++;
		pWin->hThread = CreateThread(NULL, 0, app_ui_thread, pWin, 0, NULL);
		if ( pWin->hThread == NULL ) {
			printf("[xs] app '%s': UI thread create failed, headless\n",
				pServer->Name);
			if ( pWin->iClose == XS_APP_CLOSE_STOP )
				InterlockedDecrement(&g_iLiveStopWindows);
			pWin->iAlive = 0;
			continue;
		}
		g_iWindowCount++;
		printf("[xs] app '%s': frontend window loading (close=%s)\n",
			pServer->Name, pWin->iClose == XS_APP_CLOSE_HIDE ? "hide" : "stop");
	}
	if ( g_iWindowCount == XS_MAX_APP_WINDOWS ) {
		uint32 iTotal = 0, k;

		for ( k = 0; k < pApp->ServerCount; k++ ) {
			if ( pApp->Servers[k] != NULL && pApp->Servers[k]->Enabled
				&& XS_ServerIsApp(pApp->Servers[k]) ) iTotal++;
		}
		if ( iTotal > (uint32)g_iWindowCount )
			printf("[xs] app: %u app services but window cap %d, "
				"extra run headless\n", iTotal, g_iWindowCount);
	}
	if ( g_iWindowCount > 0 ) app_single_instance_watch();

	/* 测试钩子（设计 §10）：XS_APP_AUTOCLOSE_MS=N 毫秒后自动关窗，
	 * 与用户关窗同路径（WM_CLOSE / UI 线程终止），CI 可断言
	 * "关窗→优雅停机→退出码"全链路；正常运行为空即无操作。
	 * 主线程无消息泵，SetTimer 回调不可用，用独立 sleep 线程。 */
	if ( g_iWindowCount > 0 && getenv("XS_APP_AUTOCLOSE_MS") != NULL ) {
		int iMs = atoi(getenv("XS_APP_AUTOCLOSE_MS"));

		if ( iMs > 0 ) {
			HANDLE h = CreateThread(NULL, 0, app_autoclose_thread,
				(LPVOID)(intptr_t)iMs, 0, NULL);
			if ( h != NULL ) CloseHandle(h);
		}
	}
}

void XS_AppWindowNavigate(XS_App* pApp)
{
	int i;

	(void)pApp;
	for ( i = 0; i < g_iWindowCount; i++ ) {
		XS_AppWindow* pWin = &g_tWindows[i];
		char sUrl[1024];

		XS_AppUrlBuild(pWin->pServer, sUrl, sizeof sUrl);
		/* 先写缓冲后置状态（Interlocked 全屏障），UI 线程创建完成
		 * 后自查生效；若窗口已就绪则直接 dispatch 导航 */
		snprintf(pWin->sTargetUrl, sizeof pWin->sTargetUrl, "%s", sUrl);
		InterlockedExchange(&pWin->iNavState, XS_APP_NAV_URL);
		if ( pWin->iAlive && pWin->pWebview != NULL )
			app_dispatch(pWin, app_job_navigate, pWin);
		printf("[xs] app '%s': frontend ready -> %s\n",
			pWin->pServer->Name, sUrl);
	}
}

void XS_AppWindowFail(const char* sErr)
{
	int i;

	for ( i = 0; i < g_iWindowCount; i++ ) {
		XS_AppWindow* pWin = &g_tWindows[i];

		app_error_page((sErr != NULL && sErr[0] != '\0') ? sErr
			: "服务装配失败，详见控制台日志", pWin->sFailPage,
			sizeof pWin->sFailPage);
		InterlockedExchange(&pWin->iNavState, XS_APP_NAV_FAIL);
		if ( pWin->iAlive && pWin->pWebview != NULL )
			app_dispatch(pWin, app_job_fail, pWin);
	}
}

/* 失败页驻留：stop 型窗口全部关闭（用户读完错误）才允许进程退出；
 * hide 型窗口不阻塞（此时无人看到错误页）。 */
void XS_AppWindowWaitClosed(void)
{
	int iGuard = 6000;	/* 上限 10 分钟，防呆 */

	while ( g_iLiveStopWindows > 0 && iGuard-- > 0 && !g_XS_Stop )
		Sleep(100);
}

void XS_AppWindowClose(void)
{
	int i;

	/* 先置 SHUTDOWN 状态：仍在初始化的窗口创建完成后自查即自行终止；
	 * 已就绪的窗口经 dispatch 在 UI 线程终止（M0 实测结论）。 */
	for ( i = 0; i < g_iWindowCount; i++ ) {
		XS_AppWindow* pWin = &g_tWindows[i];

		InterlockedExchange(&pWin->iNavState, XS_APP_NAV_SHUTDOWN);
		if ( pWin->iAlive && pWin->pWebview != NULL )
			app_dispatch(pWin, app_job_terminate, NULL);
	}
	for ( i = 0; i < g_iWindowCount; i++ ) {
		if ( g_tWindows[i].hThread != NULL ) {
			WaitForSingleObject(g_tWindows[i].hThread, 5000);
			CloseHandle(g_tWindows[i].hThread);
			g_tWindows[i].hThread = NULL;
		}
	}
	if ( g_pWaitRegistration != NULL ) {
		UnregisterWaitEx((HANDLE)g_pWaitRegistration, INVALID_HANDLE_VALUE);
		g_pWaitRegistration = NULL;
	}
	g_iWindowCount = 0;
}
