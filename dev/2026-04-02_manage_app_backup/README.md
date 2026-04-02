当前目录保存 `2026-04-02` 收紧 `xs / xsdbg` 边界前的内置管理面代码备份。

用途：
- 为后续重构 `xsdbg` 内置调试应用保留参考实现
- 避免在 production `xs` 主线剥离内置应用时丢失历史代码

当前备份文件：
- `http_manage.h`
- `http_manage_debug.h`
- `http_runtime.h`
- `runtime_state.h`
- `http.h`
