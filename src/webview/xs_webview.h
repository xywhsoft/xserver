/* xs_webview.h — webview 扩展对宿主与 TCC 脚本的最小契约。
 *
 * 本头有双读者：tcc_host.h 生成的扩展表把它编进宿主（取函数地址），
 * VFS 把它挂到 /xs/xs_webview.h 供脚本 #include。只允许纯 C 声明。
 *
 * v1 契约限定为可用性探测（设计 §2：app 节点纯显示层，
 * 窗口控制面不进脚本环境）；M1 落地窗口本体后再评估扩展。 */
#ifndef XS_WEBVIEW_H
#define XS_WEBVIEW_H

/* webview 库版本钉子的唯一数值源（0.12.0 → 1200，与 lib/webview/README.md
 * 一致）。webview_impl.cpp 用 static_assert 校验它与 vendored 头的版本宏
 * 相符——升级库而忘改此值会直接编译失败。 */
#define XS_WEBVIEW_VERSION_NUMBER 1200

#ifdef __cplusplus
extern "C" {
#endif

/* 窗口前端是否编入：1 = Windows 且带 webview 扩展；0 = headless 降级
 * （Linux、未选扩展、或 Runtime 兜底失败后的运行态由 xsWebviewVersion 之外
 * 的前端状态接口在 M1 区分，本函数只回答构建期事实）。 */
int xsWebviewAvailable(void);

/* 前端实现版本，来自 lib/webview 版本钉子（0.12.0 → 1200）；headless 为 0。 */
int xsWebviewVersion(void);

#ifdef __cplusplus
}
#endif

#endif /* XS_WEBVIEW_H */
