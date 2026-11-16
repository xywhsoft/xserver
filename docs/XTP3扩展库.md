# XTP3 扩展库

`build.bat xtp` / `bash build.sh xtp` 选择 XRT XTP3，自动包含 xws 与 xrudp。宿主逐文件原生编译 `lib/xtp/src`，脚本直接 `#include <xtp.h>`，用 `XS_USE_XTP` 判断能力。42 个外部 API 按公开声明导入；inline helper 在脚本内直接编译，不加动态包装或操作码分发。XTP1/2 头文件、符号、实现与线路兼容均已移除。

完整模块来自 XRT 提交 `c6e6dbd361e5b076e379d69e3ea655e760f029dc`；许可证见 `lib/xtp/LICENSE`，每个上游文件的来源提交和 SHA256 见 `lib/xrt_sources.lock.json`。本次只升级 xtp；核心、xws、xrudp 与其他产品保持各自固定版本，不能用 XTP 的提交号冒充核心版本。用 `python tools/sync_xrt.py --check` 校验。后续更新仍使用 `--source ../xrt --ref <commit> --library xtp`，不修改 vendored 源码。

## 可运行示例

从仓库根目录执行：

```text
python tools/build.py xtp --output .build/xtp3/xs-xtp3.exe
python tools/run_xtp3_example.py --exe .build/xtp3/xs-xtp3.exe
python tools/test_extensions_runtime.py --exe .build/xtp3/xs-xtp3.exe xtp
```

Linux 将 exe 路径换成 `.build/xtp3/xs-xtp3`。示例源码在 `dev/xtp3/main.c`，在临时的独立 custom 服务进程内运行后退出，不连接外网、不操作现有 xs 服务。它覆盖四种 Command/Params 组合、TABLE 的 ParamAt、RPC、可靠 PUSH 与 DATAGRAM 形态 NOTIFY。宿主仍按实际网络边界调用 Input，并把 Output 真正交给下层后调用 Advance。TCP/TLS/WS/xrudp 适配器借用已有网络对象，不创建连接或接管事件。

同一个 Session 的操作由宿主串行化，不同实例可以并行。输入和回调中的 Command、Params、Body、Packet 均为借用视图；异步使用须在回调返回前复制到应用自己的有界存储。异步 REQUEST 只保留 opaque Request token，再用 Reply 回复；不可保留整个回调指针。TABLE 保留线顺序，ParamAt 在 ARRAY/TABLE 都可用，字符串查找只用于 TABLE。

脚本卸载前必须停止网络输入与异步业务，分轮 Close/Drive 到 CLOSED 再 Destroy，确保没有仍指向 TCC 代码的回调。库没有隐式热重载所有权适配器。应用负责认证结果和控制面；Delivery 默认关闭，只有显式 Replay 的消息在通过证明的状态续接后重放。业务幂等、崩溃恢复和磁盘队列不由本库承诺。

## 验证

Windows/Linux GCC 宿主的直接/嵌套 TCC 均验证了 42 个 XTP 导出、回调返回 xtpdecision、32 字节 Request/Output token 按值跨编译器传递及完整请求应答。依赖闭包共 329 个导出，xrudp 原生 IPv4/IPv6 Noise 和句柄退出探针也通过。注册、构建选择、逐符号覆盖、固定快照哈希与选择性同步测试分别见 tools/test_extensions.py、test_xrt_sync.py、test_sync_xrt.py。

XRT 的游戏丢包/RTT/MTU 测试、Delivery 故障/OOM/模糊测试不等同于 XS 热重载、移动公网或长时间生产验收。强安全配置使用 TLS1.3 或 xrudp Noise；对普通裸 UDP 的解析不构成身份认证或可靠投递保证。

XTP 自有 C/头约 114.05 KiB；其源码预算超标和可恢复 RPC 默认回执窗口限制仍存在。完整功能覆盖与测量保留在固定的 XRT 提交中，XS 未把依赖源码算成 XTP 自己的体积，也未以 SDK ABI 通过冒充完整 WSS/移动网络验收。
