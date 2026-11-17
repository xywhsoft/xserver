# XTP3 扩展库

`build.bat xtp` / `bash build.sh xtp` 选择 XRT XTP3，自动包含 xws 与 xrudp。宿主逐文件原生编译 `lib/xtp/src`，脚本直接 `#include <xtp.h>`，用 `XS_USE_XTP` 判断能力。42 个外部 API 按公开声明导入；inline helper 在脚本内直接编译，不加动态包装或操作码分发。XTP1/2 头文件、符号、实现与线路兼容均已移除。

完整模块来自 XRT 提交 `cf24b0155bc5fabc958d5d10baf3e6e36d8e96bd`；许可证见 `lib/xtp/LICENSE`，每个上游文件的来源提交和 SHA256 见 `lib/xrt_sources.lock.json`。本次只升级 xtp；核心、xws、xrudp 与其他产品保持各自固定版本，不能用 XTP 的提交号冒充核心版本。用 `python tools/sync_xrt.py --check` 校验。后续更新仍使用 `--source ../xrt --ref <commit> --library xtp`，不修改 vendored 源码。

## 可运行示例

从仓库根目录执行：

```text
python tools/build.py xtp --output .build/xtp3/xs-xtp3.exe
python tools/run_xtp3_example.py --exe .build/xtp3/xs-xtp3.exe
python tools/test_extensions_runtime.py --exe .build/xtp3/xs-xtp3.exe xtp
```

Linux 将 exe 路径换成 `.build/xtp3/xs-xtp3`。示例源码在 `dev/xtp3/main.c`，在临时的独立 custom 服务进程内运行后退出，不连接外网、不操作现有 xs 服务。它覆盖四种 Command/Params 组合、TABLE 的 ParamAt、RPC、可靠 PUSH 与 DATAGRAM 形态 NOTIFY，以及批量输出和跨帧短写的 host/TCC 结构及令牌 ABI。宿主仍按实际网络边界调用 Input，并把 Output 真正交给下层后调用 Advance。TCP/TLS/WS/xrudp 适配器借用已有网络对象，不创建连接或接管事件。

同一个 Session 的操作由宿主串行化，不同实例可以并行。输入和回调中的 Command、Params、Body、Packet 均为借用视图；异步使用须在回调返回前复制到应用自己的有界存储。异步 REQUEST 只保留 opaque Request token，再用 Reply 回复；不可保留整个回调指针。TABLE 保留线顺序，ParamAt 在 ARRAY/TABLE 都可用，字符串查找只用于 TABLE。

脚本卸载前必须停止网络输入与异步业务，分轮 Close/Drive 到 CLOSED 再 Destroy，确保没有仍指向 TCC 代码的回调。库没有隐式热重载所有权适配器。应用负责认证结果和控制面；Delivery 默认关闭，只有显式 Replay 的消息在通过证明的状态续接后重放。业务幂等、崩溃恢复和磁盘队列不由本库承诺。

## 验证

Windows/Linux GCC 宿主的直接/嵌套 TCC 均验证了 42 个 XTP 导出、回调返回 xtpdecision、32 字节 Request/Output token 按值跨编译器传递及完整请求应答。依赖闭包共 329 个导出，xrudp 原生 IPv4/IPv6 Noise 和句柄退出探针也通过。注册、构建选择、逐符号覆盖、固定快照哈希与选择性同步测试分别见 tools/test_extensions.py、test_xrt_sync.py、test_sync_xrt.py。

XRT 的游戏丢包/RTT/MTU 测试、Delivery 故障/OOM/模糊测试不等同于 XS 热重载、移动公网或长时间生产验收。强安全配置使用 TLS1.3 或 xrudp Noise；对普通裸 UDP 的解析不构成身份认证或可靠投递保证。

XTP 自有 C/头按 UTF-8/LF 计 121998 B（119.14 KiB），源码不设硬上限，优先简洁、安全和性能；含 Delivery 的自有目标码 text 为 44992 B，依赖另计。可恢复 RPC 已改为固定滚动完成窗口，移除默认 1024/60s 的速率限制；未消费的迟到结果只以 STATE_LOST 退休。默认 16 KiB 发送聚合仅组合已排队的同一可靠 lane，不等待凑包，并显式统计额外暂存复制；BatchBytes=0 可关闭。

固定 XRT 来源包含完整 WSS/TLS1.3/HTTP Upgrade 例程、多连接/多 worker 与真实 UDP/Noise 容量工具、x86/大端及有界持续连接验证。网络连接与 HTTP 前端仍由宿主管理；XS 的 ABI 通过不代替移动网络/NAT 或多日长稳。相应测试和性能证据位于上游 extlibs/xtp/docs/qualification.md 与 performance.md。

最终收据见 [2026-11-17 验证记录](xtp3-validation-2026-11-17.json)：固定来源、两个宿主二进制、示例和测试日志均记录 SHA256。Windows/Linux 外层及嵌套 TCC 已实际验证聚合 Frames、跨帧短写与新增配置/统计字段；注册29项、快照9项、选择性同步8项通过。旧的短样本、未固定亲和性的P99波动失败记录保留；上游后来固定CPU、每形态100万RPC、7次重复的独立实验通过原质量门限，两者不能混成同一实验或跨环境回归基线。

生产验收的标准、执行命令、证据保存、失败处理和汇报模板统一维护在上游 `D:/GIT/xrt/extlibs/xtp/docs/production-acceptance.md`。正式长稳已由用户确认为连续24小时；旧72小时任务已人工中断，不计为通过。本次仅整理交接文档，未启动新测试；完整生产验收仍待接手线程完成。
