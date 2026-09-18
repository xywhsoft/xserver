webview 扩展参考实现（已编译实测，未接线）。

- wv2_fallback.c：74 行。注册表三处检测 WebView2 Evergreen Runtime -> 下载官方引导器 -> 静默安装 -> 复检。编译：gcc -O2 -Wall wv2_fallback.c ... -lurlmon -ladvapi32 -luser32
- wv2_fallback_ui.c：219 行。提示框 + 下载进度窗：IBindStatusCallback 真实百分比、单线程就地泵消息、E_ABORT 取消、安装期跑马灯。编译：gcc -O2 -Wall -mwindows wv2_fallback_ui.c ... -lurlmon -ladvapi32 -lcomctl32 -lgdi32 -luser32

归档自 2026-09-16 选型实测（w64devkit gcc，零警告，检测路径与真实下载流程已验证）。
M1 实施时迁入 src/webview/ 改造，见 ../Webview扩展与app节点设计.md §11。
