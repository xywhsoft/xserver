# xs3

**xs 是 xrt 的落地化部署工具。** 它把 xrt 的网络、协议、TLS、并发、数据能力装配成一个由配置文件驱动的可执行程序，并在运行时把 C 脚本编译进进程。xrt 才是主要的那个；xs 只负责部署，不提供框架、不提供治理、不教应用怎么做事。

当前状态：**五大协议全量交付（http(+https)/ws(+wss)/tcp(+tcps)/udp/custom）+ 热重载 + 可选库门控 + 三层测试体系**。
- 协议：HTTP 连接驱动（三态返回/虚拟主机路由/静态层/keep-alive）、WS 升级移交、TCP/TCP+TLS、UDP、custom 手动装配
- 热重载：generation 换代、编译失败回滚、ServiceSwap 交接、代际定时器
- 可选库：`build.bat sqlite` 变体（296 符号导入 + sqlite3.h 入 VFS）
- 测试：`test.bat`（冒烟+功能：配置矩阵/行为/重载/idle）、`tools/pressure_test.py`（四协议 RPS 基线：tcp/ws ~70k、udp ~60k、http ~9k）、`tools/drill.py`（攻防演练：对抗样本+金丝雀+内存监控）——`cmd /c test.bat` 一键回归：配置装载 → 引擎启动 → TCC 经内置 VFS 编译 custom 脚本（2,850 个 xrt 符号全量导入）→ 脚本自建 echo 监听 → TCP 回路验证 → 三阶段优雅停机。http/ws/tcp/udp 协议驱动与软重载按 [docs/设计.md](docs/设计.md) §14 推进。

TCC 宿主吸收了 xlang（demo6）的成熟经验并推进到**单文件交付**：`res/tcc`（VFS 构建目录）在构建期整体 LZMA 打包进二进制（SDK 头 + 精简 winapi/linux 头 + libtcc1.a + 导入库，7.3MB → 1.0MB），运行时从内置 VFS 读取——**xs 不附带任何磁盘 TCC 环境**。新增内置库 = 放入 `res/tcc` 重新构建。中文路径由 UTF-8 宽字符 IO 全链路保障。

## xs 做什么

1. 五个协议入口：`http / ws / tcp / udp / custom`（custom 全手动）
2. xrt 环境导入 TCC（脚本直接调用 xrt API）
3. 内置编译可选的扩展库（sqlite、libtcc 等，宏门控），一并导入 TCC
4. 协议初始化能力：除 custom 外，握手、解析、分帧由 xs + xrt 完成
5. 基于配置文件提供服务（`xs.json`，预设字段进结构体，其余全归应用）

## xs 不做什么

- 不做 bus、XTP、管理面、调试面、任何治理
- 不做 HTTP 便利封装、路由、session、权限——那是 xadmin 们的事
- 不做沙盒与权限限制——C 语言限制不住，一切对脚本开放
- 本阶段不使用 xhttp / xws（只基于已压实的 xrt http core 与 websocket core）

## 路线定位

xs3 是 C 语言版 xs 的完全重写，也是双轨（C 版 / xlang 版）中先行定性的技术路线：未来 xlang 版将参照 xs3 沉淀的三样东西——**透传契约（回调收 xrt 原生对象）、结构体 ABI + 全开放访问、generation 生命周期**——决定自己的形态。

## 依赖

- [xrt](../xrt)：网络 / 协议 / TLS / 并发 / value / JSON / 模板 / 压缩（锁定版本随发）
- TCC 0.9.28rc：运行时 C 编译器（兼容性已验证，见设计文档附录）
