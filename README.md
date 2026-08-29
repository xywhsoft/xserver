# xs3

**xs 是 xrt 的落地化部署工具。** 它把 xrt 的网络、协议、TLS、并发、数据能力装配成一个由配置文件驱动的可执行程序，并在运行时把 C 脚本编译进进程。xrt 才是主要的那个；xs 只负责部署，不提供框架、不提供治理、不教应用怎么做事。

当前状态：**生产就绪**——五大协议（http(+https)/ws(+wss)/tcp(+tcps)/udp/custom）、期望状态热重载（latest-wins/整代候选/原子发布/自动排空回收）、可选库门控（sqlite）、dev_inc/dev_lib 应用 SDK 目录、静态层四旋钮、四层测试体系（冒烟/功能/压力/攻防演练 6h 704 轮 0 失败）。遗留：xadmin 试点、Linux 实测。
- 协议：HTTP 每请求虚拟主机路由（脚本+静态根）、WS 握手期路由并固定 host/脚本、TCP/TCP+TLS、UDP、custom 手动装配
- 热重载：独立 controller、同 server latest-wins ticket、不可变配置快照、候选 listener 先 bind 但不接入、host/server/all 均以完整 server generation 原子发布；旧代停止新接入后由 generation 终态引用自然排空回收，无泄漏、无宽限/超时强拆
- 可选库：`build.bat sqlite` 变体（296 符号导入 + sqlite3.h 入 VFS）
- 测试：`test.bat`（冒烟+功能+generation 生命周期+四协议 reload 矩阵）、`tools/lifecycle_reload_test.py`（旧 keep-alive 留在旧代、初始化发布屏障、拓扑 lease 与 Root 换代）、`tools/reload_matrix_test.py`（HTTP/TCP/UDP/WS 同端点换代及跨服务 lease 回收）、`tools/pressure_test.py`、`tools/drill.py`。

热换安全边界：绑定端点改变时候选可先 bind，但在拓扑事务提交前拒绝接入；同一端点由稳定 listener 槽位转交给新 generation。所有 Accept 与拓扑写锁线性化，reload-all 不会暴露半新半旧状态。旧连接继续持有旧代，最后一个终态引用归零后自动释放。协议类型、TLS 形态、backlog、recv_limit 等 listener 固化字段若在同端点改变会明确拒绝并要求重启。`xsReloadHost*` 是指定 host 的管理面入口，内部也重建并发布它所属的完整 server generation，不原地改指针；custom 因裸资源无统一终态 lease，不支持在线卸载。公开 server 查找返回显式 lease，必须 `xsServerRelease`。

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
