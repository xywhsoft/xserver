/* wv2_fallback.c — 系统缺少 WebView2 Evergreen Runtime 时的自动引导安装
 * 链接：-lurlmon -ladvapi32 -luser32
 * 用法：在调用 webview_create() 之前执行 wv2_ensure()，返回 0 即可用
 */
#define _WIN32_WINNT 0x0600
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <urlmon.h>
#include <wchar.h>

/* Evergreen 引导器官方直链（x86，约 2MB，在线安装） */
#define WV2_URL L"https://go.microsoft.com/fwlink/p/?LinkId=2124703"
#define WV2_KEY L"{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"

static const WCHAR SUB_MAIN[] = L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\" WV2_KEY;
static const WCHAR SUB_WOW[]  = L"SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\" WV2_KEY;

/* EdgeUpdate ClientState 的 pv 值非空且不为 0.0.0.0 即视为已安装 */
static int pv_ok(HKEY root, const WCHAR *sub) {
    WCHAR pv[64];
    DWORD n = sizeof pv;
    if (RegGetValueW(root, sub, L"pv", RRF_RT_REG_SZ, NULL, pv, &n) != ERROR_SUCCESS)
        return 0;
    return pv[0] != L'\0' && wcscmp(pv, L"0.0.0.0") != 0;
}

/* 三个位置覆盖 per-user、per-machine 及 32 位注册表视图 */
static int wv2_runtime_present(void) {
    return pv_ok(HKEY_CURRENT_USER, SUB_MAIN)
        || pv_ok(HKEY_LOCAL_MACHINE, SUB_MAIN)
        || pv_ok(HKEY_LOCAL_MACHINE, SUB_WOW);
}

/* 静默运行引导器并等待完成；per-user 安装不需要管理员权限 */
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
    /* 引导器在线下载约 240MB，最长等 15 分钟 */
    if (WaitForSingleObject(pi.hProcess, 15u * 60 * 1000) == WAIT_OBJECT_0)
        GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code == 0;
}

int wv2_ensure(void) {
    WCHAR exe[MAX_PATH + 16];
    if (wv2_runtime_present())
        return 0;
    if (MessageBoxW(NULL,
            L"系统缺少 WebView2 运行时，需要联网下载安装（约 240MB）。\n现在安装吗？",
            L"提示", MB_OKCANCEL | MB_ICONQUESTION) != IDOK)
        return -1;
    if (GetTempPathW(MAX_PATH, exe) == 0)
        return -2;
    wcscat(exe, L"wv2setup.exe");
    if (FAILED(URLDownloadToFileW(NULL, WV2_URL, exe, 0, NULL)))
        return -3; /* 无网络或下载失败 */
    wv2_install_silent(exe);
    DeleteFileW(exe);
    return wv2_runtime_present() ? 0 : -4;
}
