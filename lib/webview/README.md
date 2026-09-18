# webview vendor（版本钉子）

来源与锁定版本（升级为手动动作，升级前先读 MIGRATION 与门禁）：

| 文件 | 来源 | 版本 |
|---|---|---|
| `webview/webview.h` | github.com/webview/webview tag `0.12.0`（`core/include/webview/webview.h`） | 0.12.0 |
| `webview.h` | 同上 tag 的兼容 shim（`core/include/webview.h`），仅供 `#include "webview.h"` 旧写法 | 0.12.0 |
| `webview/WebView2.h`、`webview/WebView2EnvironmentOptions.h` | NuGet `Microsoft.Web.WebView2`（仅取构建期头文件，包内含 x86/x64/ARM64 加载器未采用） | 1.0.4191.47 |
| `webview/EventToken.h` | webview 仓库 `compatibility/mingw/include/EventToken.h`（MinGW 缺失的 SDK 兼容头） | 随 0.12.0 |

许可证：`LICENSE-webview.txt`（MIT，webview 库）、`LICENSE-webview2-sdk.txt` + `NOTICE-webview2-sdk.txt`（微软 BSD 式，允许头文件再分发，须保留声明）。

使用方式：仅 `src/webview/webview_impl.cpp` 以 `-I lib/webview` 包含 `webview/webview.h` 编入实现（`-x c++ -std=c++17`，仅 Windows 平台编译）。其余源文件不直接包含这些头。运行时不携带 `WebView2Loader.dll`——库实现自行读注册表定位 Evergreen Runtime 后动态加载；`xs.exe`/`xsw.exe` 的 DLL 导入由 `tools/check_webview_gates.py` 门禁约束。

版本出处实测记录（2026-09-16，w64devkit gcc）：+296 KB 二进制增量、单 TU 编译约 1.9 秒、`gcc` 驱动 + `-lstdc++` 静态解析无 `libstdc++-6.dll` 依赖。
