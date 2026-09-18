/* wv2_fallback_ui.c — 缺少 WebView2 Runtime 时的提示框 + 下载进度窗 + 静默安装
 *
 * 单线程模型：所有阻塞点（下载回调、等待安装进程）都就地泵消息，
 * 窗口全程保持响应；不依赖任何 GUI 框架，可在主程序初始化前独立使用。
 *
 * 编译: gcc -O2 -mwindows wv2_fallback_ui.c -o app.exe \
 *         -lurlmon -ladvapi32 -lcomctl32 -lgdi32 -luser32
 * 测试: app.exe test   —— 跳过检测，仅实测下载进度窗（不安装）
 */
#define _WIN32_WINNT 0x0600
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commctrl.h>
#include <urlmon.h>
#include <wchar.h>
#include <stdio.h>

#define WV2_URL L"https://go.microsoft.com/fwlink/p/?LinkId=2124703"
#define WV2_KEY L"{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"

static const WCHAR SUB_MAIN[] = L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\" WV2_KEY;
static const WCHAR SUB_WOW[]  = L"SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\" WV2_KEY;

static HWND g_hwnd, g_label, g_bar;
static volatile LONG g_cancel;

/* ---------- 运行时检测（同无 UI 版） ---------- */

static int pv_ok(HKEY root, const WCHAR *sub) {
    WCHAR pv[64];
    DWORD n = sizeof pv;
    if (RegGetValueW(root, sub, L"pv", RRF_RT_REG_SZ, NULL, pv, &n) != ERROR_SUCCESS)
        return 0;
    return pv[0] != L'\0' && wcscmp(pv, L"0.0.0.0") != 0;
}

static int wv2_runtime_present(void) {
    return pv_ok(HKEY_CURRENT_USER, SUB_MAIN)
        || pv_ok(HKEY_LOCAL_MACHINE, SUB_MAIN)
        || pv_ok(HKEY_LOCAL_MACHINE, SUB_WOW);
}

/* ---------- 提示/进度窗口 ---------- */

static void pump(void) {
    MSG m;
    while (PeekMessageW(&m, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
}

static LRESULT CALLBACK wnd_proc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_CLOSE) {       /* 不立即销毁，置取消标志让下载循环中止 */
        g_cancel = 1;
        return 0;
    }
    return DefWindowProcW(h, msg, w, l);
}

static void ui_create(const WCHAR *title) {
    static const WCHAR cls[] = L"wv2_fallback_ui";
    WNDCLASSW wc;
    INITCOMMONCONTROLSEX icc = { sizeof icc, ICC_BAR_CLASSES };
    HINSTANCE inst = GetModuleHandleW(NULL);
    InitCommonControlsEx(&icc);
    ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.lpszClassName = cls;
    RegisterClassW(&wc);
    g_hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME, cls, title,
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                             CW_USEDEFAULT, CW_USEDEFAULT, 400, 150,
                             NULL, NULL, inst, NULL);
    g_label = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                              18, 16, 350, 20, g_hwnd, NULL, inst, NULL);
    g_bar = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE,
                            18, 48, 350, 18, g_hwnd, NULL, inst, NULL);
    SendMessageW(g_label, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
    ShowWindow(g_hwnd, SW_SHOWNORMAL);
    pump();
}

static void ui_set_text(const WCHAR *s) {
    SetWindowTextW(g_label, s);
    pump();
}

static void ui_destroy(void) {
    DestroyWindow(g_hwnd);
    UnregisterClassW(L"wv2_fallback_ui", GetModuleHandleW(NULL));
    pump();
}

/* ---------- 下载回调：真实百分比 + 就地泵消息保持响应 ---------- */

static HRESULT STDMETHODCALLTYPE bs_QueryInterface(IBindStatusCallback *This,
        REFIID riid, void **ppv) { (void)This; (void)riid; (void)ppv; return E_NOINTERFACE; }
static ULONG STDMETHODCALLTYPE bs_AddRef(IBindStatusCallback *This) { (void)This; return 1; }
static ULONG STDMETHODCALLTYPE bs_Release(IBindStatusCallback *This) { (void)This; return 1; }
static HRESULT STDMETHODCALLTYPE bs_OnStartBinding(IBindStatusCallback *This,
        DWORD r, IBinding *b) { (void)This; (void)r; (void)b; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE bs_GetPriority(IBindStatusCallback *This,
        LONG *p) { (void)This; (void)p; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE bs_OnLowResource(IBindStatusCallback *This,
        DWORD r) { (void)This; (void)r; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE bs_OnProgress(IBindStatusCallback *This,
        ULONG cur, ULONG max, ULONG code, LPCWSTR txt) {
    WCHAR buf[128];
    (void)This; (void)code; (void)txt;
    pump();
    if (g_cancel)
        return E_ABORT;          /* 从回调返回 E_ABORT 即中止下载 */
    if (max) {
        SendMessageW(g_bar, PBM_SETMARQUEE, FALSE, 0);
        SendMessageW(g_bar, PBM_SETRANGE32, 0, (LPARAM)max);
        SendMessageW(g_bar, PBM_SETPOS, (WPARAM)cur, 0);
        _snwprintf(buf, 127, L"正在下载组件：%lu KB / %lu KB", cur / 1024, max / 1024);
    } else {
        SendMessageW(g_bar, PBM_SETMARQUEE, TRUE, 30);
        wcscpy(buf, L"正在下载组件…（服务器未提供总大小）");
    }
    SetWindowTextW(g_label, buf);
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE bs_OnStopBinding(IBindStatusCallback *This,
        HRESULT hr, LPCWSTR e) { (void)This; (void)hr; (void)e; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE bs_GetBindInfo(IBindStatusCallback *This,
        DWORD *f, BINDINFO *b) { (void)This; (void)f; (void)b; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE bs_OnDataAvailable(IBindStatusCallback *This,
        DWORD f, DWORD sz, FORMATETC *fe, STGMEDIUM *st) {
    (void)This; (void)f; (void)sz; (void)fe; (void)st; return E_NOTIMPL;
}
static HRESULT STDMETHODCALLTYPE bs_OnObjectAvailable(IBindStatusCallback *This,
        REFIID riid, IUnknown *p) { (void)This; (void)riid; (void)p; return E_NOTIMPL; }

static IBindStatusCallbackVtbl g_bs_vtbl = {
    bs_QueryInterface, bs_AddRef, bs_Release,
    bs_OnStartBinding, bs_GetPriority, bs_OnLowResource, bs_OnProgress,
    bs_OnStopBinding, bs_GetBindInfo, bs_OnDataAvailable, bs_OnObjectAvailable
};
static IBindStatusCallback g_bs = { &g_bs_vtbl };

/* ---------- 静默安装：等待进程期间泵消息，进度条跑马灯 ---------- */

static int wv2_install_silent(const WCHAR *exe) {
    WCHAR cmd[MAX_PATH + 32];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    DWORD code = 1;
    _snwprintf(cmd, MAX_PATH + 31, L"\"%s\" /silent /install", exe);
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return 0;
    SendMessageW(g_bar, PBM_SETMARQUEE, TRUE, 30);
    while (WaitForSingleObject(pi.hProcess, 50) == WAIT_TIMEOUT)
        pump();
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code == 0;
}

/* ---------- 总入口：返回 0 表示 Runtime 可用 ---------- */

int wv2_ensure_ui(void) {
    WCHAR dir[MAX_PATH], exe[MAX_PATH + 16];
    HRESULT hr;
    if (wv2_runtime_present())
        return 0;
    if (MessageBoxW(NULL,
            L"系统缺少 WebView2 运行时（约 240 MB），需要联网下载安装。\n现在安装吗？",
            L"提示", MB_OKCANCEL | MB_ICONQUESTION) != IDOK)
        return -1;
    if (!GetTempPathW(MAX_PATH, dir))
        return -2;
    _snwprintf(exe, MAX_PATH + 15, L"%swv2setup.exe", dir);
    ui_create(L"WebView2 组件安装");
    ui_set_text(L"正在连接下载服务器…");
    hr = URLDownloadToFileW(NULL, WV2_URL, exe, 0, &g_bs);
    if (FAILED(hr) || g_cancel) {
        ui_destroy();
        DeleteFileW(exe);
        return -3;                /* 无网络或用户取消 */
    }
    ui_set_text(L"正在安装运行时（可能需要几分钟）…");
    wv2_install_silent(exe);
    ui_destroy();
    DeleteFileW(exe);
    return wv2_runtime_present() ? 0 : -4;
}

/* ---------- 测试 ---------- */

int main(int argc, char **argv) {
    if (argc > 1 && argv[1][0] == 't') {   /* test 模式：只测下载进度窗，不安装 */
        WCHAR dir[MAX_PATH], exe[MAX_PATH + 16];
        HRESULT hr;
        GetTempPathW(MAX_PATH, dir);
        _snwprintf(exe, MAX_PATH + 15, L"%swv2setup_test.exe", dir);
        ui_create(L"下载进度窗测试");
        ui_set_text(L"正在连接下载服务器…");
        hr = URLDownloadToFileW(NULL, WV2_URL, exe, 0, &g_bs);
        ui_destroy();
        DeleteFileW(exe);
        printf("download hr=0x%lX cancel=%d\n", (unsigned long)hr, (int)g_cancel);
        return FAILED(hr);
    }
    printf("wv2_ensure_ui() = %d (0 = runtime ready)\n", wv2_ensure_ui());
    return 0;
}
