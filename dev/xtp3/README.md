# XTP3 XS 示例

运行 `python tools/run_xtp3_example.py --exe <已启用 xtp 的 xs>`。完整说明见 [XTP3 扩展库](../../docs/XTP3扩展库.md)。main.c 同时作为外层/嵌套 TCC 的 ABI 验证源；所有 Session 在脚本进程退出前 Close/Destroy。
