# xs3

**xs 是 xrt 的落地化部署工具。** 它把 xrt 的网络、协议、TLS、并发、数据能力装配成一个由配置文件驱动的可执行程序，并在运行时把 C 脚本编译进进程。xrt 才是主要的那个；xs 只负责部署，不提供框架、不提供治理、不教应用怎么做事。

当前状态：**生产就绪**——五大协议（http(+https)/ws(+wss)/tcp(+tcps)/udp/custom）、热重载深化（配置级分流/证书热替换/VFS 槽位复用）、可选库门控（sqlite）、dev_inc/dev_lib 应用 SDK 目录、静态层四旋钮、四层测试体系（冒烟/功能/压力/攻防演练 6h 704 轮 0 失败）。遗留：xadmin 试点、Linux 实测。
- 协议：HTTP 连接驱动（三态返回/虚拟主机路由/静态层/keep-alive）、WS 升级移交、TCP/TCP+TLS、UDP、custom 手动装配
- 热重载：终态引用 generation、旧连接自然排空、候选 listener 先行、ServiceSwap、代际定时器精确取消；无宽限/超时强拆
- 可选库：`build.bat sqlite` 变体（296 符号导入 + sqlite3.h 入 VFS）
- 测试：`test.bat`（冒烟+功能+generation 生命周期）、`tools/lifecycle_reload_test.py`（旧 keep-alive 留在旧代、双向 listener 换代、最后连接终态自动释放）、`tools/pressure_test.py`、`tools/drill.py`。

热换安全边界：绑定端点改变时采用候选先行；同一端点的结构重建明确拒绝，避免先拆旧再危险回滚。UDP 不做原地 host 换脚本；custom 因裸资源无统一终态 lease，不支持在线卸载，进程停机时在引擎停止后才释放其 TCC。

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
