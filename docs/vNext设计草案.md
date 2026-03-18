# XServer vNext 设计草案

## 1. 目标

XServer vNext 是一次从零开始的宿主层重构。

重构目标：

- 让 `xserver` 全面融合 `xrt` 体系
- 保持开箱即用、配置简单的核心特点
- 将安全性、稳定性、高性能作为默认要求
- 保留 C 语言脚本的最大灵活性
- 保持单 `main.c` 入口、按功能拆分 `.h` 文件的工程风格



## 2. 职责边界

### 2.1 xrt 职责

`xrt` 负责基础设施与通用能力：

- 基础运行时
- 内存管理与调试
- 多线程与协程任务系统
- 事件循环
- TCP / UDP / TLS 等网络基础设施
- HTTP / WebSocket 等通用网络上层能力

### 2.2 xserver 职责

`xserver` 负责宿主层能力：

- 配置加载、校验、标准化
- Server / Host 对象装配
- C 脚本宿主与热加载
- 多协议业务装配
- 虚拟主机管理
- 业务扩展协议
- 默认安全策略
- 运行时治理与诊断入口

### 2.3 业务扩展协议

`xtp` 这类协议不回灌到 `xrt`，保留在 `xserver`。

原则：

- 通用能力进入 `xrt`
- 业务扩展协议保留在 `xserver`



## 3. 协议分层

XServer vNext 的协议模型分三层：

### 3.1 基础设施协议层

直接来自 `xrt`：

- TCP
- UDP
- TLS
- EventLoop
- Network Buffer

### 3.2 通用应用层协议层

优先由 `xrt` 提供：

- HTTP
- WebSocket

### 3.3 业务扩展协议层

由 `xserver` 维护：

- XTP
- 后续私有二进制协议
- 基于 `custom` 的自定义协议装配



## 4. 核心抽象

### 4.1 Server 是统一核心抽象

所有协议都必须有 `Server`。

`Server` 负责：

- 协议类型
- 监听地址
- 运行状态
- 脚本宿主或 Host 集合
- 对应的底层 `xrt` 服务句柄

### 4.2 Host 不是全协议统一抽象

`Host` 只用于 `host-aware` 协议：

- HTTP
- WebSocket

`Host` 不强行用于：

- TCP
- UDP
- XTP
- Custom

### 4.3 协议对象模型

#### Host-aware 协议

对象关系：

`Runtime -> Server -> Host`

适用于：

- HTTP
- WebSocket

#### Server-only 协议

对象关系：

`Runtime -> Server`

适用于：

- TCP
- UDP
- XTP
- Custom



## 5. 开发模式分级

XServer vNext 默认支持以下开发模式：

- `static`
- `script-c`
- `protocol`

说明：

- `static` 用于静态文件服务
- `script-c` 用于以 TCC 为宿主的 C 脚本业务
- `protocol` 用于事件驱动、自定义协议或扩展协议开发



## 6. 服务类型

vNext 建议保留的 `class`：

- `http`
- `ws`
- `tcp`
- `udp`
- `xtp`
- `custom`

说明：

- `thread` 被移除
- 对 `xrt` 来说，`thread` 与 `custom` 不再需要作为两种独立模型



## 7. 配置系统

### 7.1 设计目标

配置系统必须进入严格模式优先的生产状态。

要求：

- 类型严格校验
- 启动前完成语义校验
- 路径统一标准化
- 允许未知字段，但只给出警告

### 7.2 配置规则

- 未知字段：警告，不报错
- 类型错误：直接报错，不自动纠正
- 不支持旧版本兼容模式
- 相对路径统一以配置文件所在目录为基准

### 7.3 配置层分离

配置处理必须拆成两个阶段：

1. 解析与标准化
2. 运行时对象构建

禁止在配置解析阶段直接做这些事：

- 启动网络服务
- 编译脚本
- 建立事件循环依赖
- 做不可回滚的运行时副作用

### 7.4 配置模型

建议显式拆分：

- `XS_Config`
- `XS_ServerConfig`
- `XS_HostConfig`

其中：

- `http/ws` 使用 `server + host`
- `tcp/udp/xtp/custom` 使用 `server-only`

Server 监听配置采用按 `class` 分化后的字段模型：

- 主模型使用 `ip + port`
- `http` 的 TLS 扩展使用 `tls + port_tls`
- `ip_tls` 仅作为可选覆盖字段，默认复用 `ip`
- 不再支持 `addr / addr_tls` 这类 URL 风格绑定字段



## 8. 脚本宿主模型

### 8.1 基本原则

TCC 仍然作为全功能 C 语言脚本宿主，不做权限限制。

理由：

- TCC 本身具备完整 C 语言能力
- 无法真正通过简单导出裁剪实现安全隔离
- 与其做虚假的权限控制，不如保留最大灵活性

### 8.2 宿主责任

虽然脚本保持全开放，但 `xserver` 仍然要提供稳定的宿主便利层：

- 脚本生命周期管理
- 脚本热加载
- 全局数据注入
- 全局消息总线
- 回滚机制
- 与 Server / Host 对象绑定

第一阶段已经提供一套最小宿主 API：

- `xs_vnext.h`
- `xs_vnext_full.h`

其中：

- `xs_vnext.h` 提供稳定宿主便利函数
- `xs_vnext_full.h` 额外引入 `xrt / libtcc / sqlite3` 的宿主可用头环境，用于旧脚本迁移期

### 8.3 脚本绑定粒度

- `http/ws`：脚本绑定到 `Host`
- `tcp/udp/xtp/custom`：脚本绑定到 `Server`

当前 `server-only` 宿主能力已经覆盖：

- `custom/tcp`：连接打开、收包、关闭、脚本回调
- `udp`：datagram 收包、默认回显、脚本回调与按来源地址回复
- `xtp`：`v2` 二进制包解析、请求/应答模型、零拷贝参数视图、脚本消息回调与回包

第一阶段已落地的 WebSocket 宿主回调：

- `WsOpenProc`
- `WsTextProc`
- `WsBinaryProc`
- `WsCloseProc`
- `ws` server 现已支持 `ws_protocol`，可要求客户端协商固定子协议
- 当 `ws_protocol` 已配置时，未协商正确子协议的握手请求会直接拒绝
- `ws` 现已支持 `tls + port_tls` 的 `wss` 运行模式



## 9. 生命周期

### 9.1 Runtime 生命周期

建议流程：

1. 初始化 `xrt`
2. 解析命令行
3. 加载配置
4. 校验配置
5. 创建 Runtime
6. 构建 Server / Host 运行时对象
7. 加载脚本
8. 调用 `ServiceInit`
9. 启动网络服务
10. 进入主循环
11. 停止接收新流量
12. 调用 `ServiceStop`
13. 调用 `ServiceUnit`
14. 销毁运行时对象
15. 卸载 `xrt`

### 9.2 Server 生命周期

建议状态：

- `created`
- `loaded`
- `initialized`
- `running`
- `draining`
- `stopped`
- `destroyed`

### 9.3 Host 生命周期

仅 `http/ws` 需要 Host 生命周期：

- `created`
- `script_loaded`
- `initialized`
- `serving`
- `reloading`
- `draining`
- `stopped`
- `destroyed`

### 9.4 生命周期保证

以下顺序要求强保证：

- `ServiceInit` 必须在服务真正开始接收业务前完成
- `ServiceStop` 用于停止接收新工作
- `ServiceUnit` 必须在对象最终销毁前调用



## 10. 全局总线与共享数据

### 10.1 设计目标

为独立 TCC 上下文之间提供统一的通信与数据共享机制。

目标：

- 支持跨 Server / Host 的消息投递
- 支持跨上下文共享 `xvalue` 数据
- 消息和数据注册表统一由宿主层维护
- 消息本体保持轻量，复杂数据通过 `data_id` 访问

### 10.2 机制组成

全局总线由两部分组成：

- 消息队列
- 全局数据注册表

消息负责通知目标上下文做事。
数据注册表负责保存和共享 `xvalue`。

### 10.3 数据模型

第一阶段使用 `int64` 作为全局数据主键。

原因：

- 查找更快
- 消息包更轻
- 生成简单
- 更适合作为跨上下文句柄

建议宿主 API：

- `xsDataRegister`
- `xsDataRegisterEx`
- `xsDataGet`
- `xsDataRetain`
- `xsDataRelease`
- `xsDataRemove`

其中第一阶段建议支持：

- `namespace`
- `tag`
- `ttl`

### 10.4 消息模型

建议宿主 API：

- `xsMsgSendToServer`
- `xsMsgSendToHost`
- `xsMsgBroadcast`

脚本通过消息只传：

- `topic`
- `data_id`
- 可选参数对象

接收方在 `MessageProc` 中按 `data_id` 读取共享数据。

### 10.5 当前实现约束

第一阶段仅支持：

- 同进程内
- 宿主层队列分发
- `xvalue` 共享数据

当前不包括：

- 跨进程
- 跨机器
- 持久化总线

第一阶段已经开始提供：

- registry 状态导出
- `namespace / tag` 定位与删除
- `namespace / tag` 条件过滤与批量清理
- namespace 聚合统计视图
- 注册失败错误码与错误文本
- 过期自动清理骨架

第一阶段保留约定：

- `xs.*`
- `__xs*`

以上命名空间保留给系统内部使用，业务脚本不应注册到这些命名空间。



## 11. 热加载设计

### 10.1 热加载分类

分为两类：

- 脚本热加载
- 配置热加载

### 10.2 脚本热加载

只替换脚本宿主，不修改配置。

适用场景：

- 业务逻辑更新
- C 脚本重新编译

### 10.3 配置热加载

重新解析并校验配置，再决定：

- 哪些 Server 保持不变
- 哪些 Host 仅热更新脚本
- 哪些对象需要重建
- 哪些服务需要重启

第一阶段最小实现要求：

- 通过 HTTP 调试入口触发 `reload_config`
- 先完成新配置的 `load/build/init`
- 仅在启动前最后一步停止旧服务
- 如果新服务启动失败，立即恢复旧 runtime
- 支持 `server` 定向重载
- 对 `http/ws` 支持 Host 原位重载
- 提供 `reload_status` 查询最近一次异步重载结果
- 网络服务统一支持 `backlog / recv_limit` 这类基础安全与容量参数
- HTTP 宿主层额外支持 `path_limit / header_limit / body_limit` 这类请求入口限制
- 静态 Host 默认仅接受 `GET / HEAD`，避免实验期过于宽松的行为进入生产
- 静态 Host 默认拒绝敏感路径与敏感扩展名暴露，HTTP 响应默认追加基础安全头

### 10.4 force 语义

热加载或重启支持 `force` 参数：

- `force = TRUE`
	- 立即停止接收新连接
	- 主动断开现有连接
	- 快速销毁旧对象

- `force = FALSE`
	- 停止接收新连接
	- 等待现有连接处理完毕
	- 进入 draining 状态
	- 待连接清空后销毁旧对象

### 10.5 回滚要求

脚本热加载与配置热加载都必须支持失败回滚。

要求：

- 新对象未就绪前，旧对象保持可用
- 编译失败、校验失败、构建失败时恢复到旧对象
- 回滚过程必须保证生命周期调用顺序正确

### 10.6 第一阶段已落地的最小热加载入口

当前 vNext 主线已经提供最小 HTTP 调试入口：

- `GET /__xs/reload`
- `GET /__xs/reload?host=<主机名>&force=true`

约束：

- 仅在 `server.debug` 或 `host.debug` 为 `true` 时启用
- 当前实现优先保证 Host 级脚本重载可用
- `force` 参数已进入 API 语义，连接排空与强制切断策略后续继续补完



## 12. 错误模型

XServer vNext 采用分级错误模型：

- 配置错误
- 宿主错误
- 运行时错误
- 致命错误

建议处理方式：

- 配置错误：启动前收集并输出
- 宿主错误：尽量局部失败，不影响无关 Server
- 运行时错误：记录日志，不默认退出整个进程
- 致命错误：仅在 Runtime 已不可恢复时退出



## 13. 日志与诊断

### 12.1 日志层

不再全局散落 `printf`，改为轻量日志包装。

至少分为：

- `info`
- `warn`
- `error`
- `debug`

### 12.2 调试与发布版本

构建产物分为：

- `xs`
- `xsdbg`

其中：

- `xs` 为生产版
- `xsdbg` 为调试版

`xsdbg` 建议启用：

- `xrt` 内存调试宏
- 更详细日志
- 生命周期跟踪
- 热加载诊断信息
- 更严格断言

第一阶段构建脚本约定：

- `build.bat` / `build.sh` 输出 `release/xs`
- `build_debug.bat` / `build_debug.sh` 输出 `release/xsdbg`



## 14. 建议的目录结构

保持单 `main.c` 入口，按功能拆 `.h` 文件。

建议结构：

```text
xserver/
├── main.c
├── src/
│   ├── core/
│   │   ├── config.h
│   │   ├── runtime.h
│   │   ├── server.h
│   │   ├── host.h
│   │   ├── error.h
│   │   └── reload.h
│   ├── protocol/
│   │   ├── http.h
│   │   ├── ws.h
│   │   ├── tcp.h
│   │   ├── udp.h
│   │   ├── xtp.h
│   │   └── custom.h
│   ├── script/
│   │   ├── dynload.h
│   │   └── script_api.h
│   └── support/
│       ├── log.h
│       ├── validate.h
│       └── path.h
├── old/
│   └── legacy/
└── docs/
```



## 15. 重构策略

本次重构采用“旧代码迁移后，从零搭骨架”的方式。

原则：

- 现有代码整体迁入 `old/legacy/`
- 旧代码只做功能参考，不做实现参考
- 新版本从零开始建立宿主层结构
- 避免历史包袱污染新设计



## 16. 第一阶段实施顺序

建议按以下顺序推进：

1. 冻结本设计文档
2. 将旧实现迁入 `old/legacy/`
3. 建立新目录骨架
4. 重写 `main.c`，只保留 bootstrap 逻辑
5. 先实现 `config / runtime / error / reload`
6. 再迁移 `http / ws / xtp / tcp / udp / custom`
7. 最后回写正式文档与示例配置



## 17. 第一阶段不做的事

以下内容不应在第一阶段投入过深：

- 旧配置兼容层
- 细碎的性能微调
- 过早扩展 Lua / JavaScript 宿主
- 在未完成宿主层边界前继续叠加新协议



## 18. 结论

XServer vNext 的核心方向已经明确：

- 用 `xrt` 作为坚实基础设施
- 用 `xserver` 专注宿主层与业务扩展层
- 用 `Server` 作为统一核心抽象
- 仅在 `http/ws` 中引入 `Host`
- 用严格配置、强生命周期和可回滚热加载进入生产级阶段

后续所有实现都应以本设计草案为准，旧版本行为只作为参考，不作为结构约束。
