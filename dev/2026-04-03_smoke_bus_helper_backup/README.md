# smoke bus helper backup

本目录备份的是清退 `__xs/bus/*` 内置治理回归前的 smoke 脚本版本。

当前主线目标：
- production `xs` 不内置应用
- `xsdbg` 只保留调试应用
- `__xs/bus/*` 不再作为内置 HTTP 管理面存在

因此原先依赖 `__xs/bus/*` 的 smoke helper 已从主线脚本移除，完整旧脚本保留在这里，便于后续按其他形式复用。
