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

当前 `xtp` 脚本宿主已经提供第一版同步客户端请求/应答能力：

- `xsXtpClientOpen`
- `xsXtpClientDo`
- `xsXtpClientDoText`
- `xsXtpClientDoSimple`
- `xsXtpClientDoJson`
- `xsXtpClientCall`
- `xsXtpClientCallText`
- `xsXtpClientCallSimple`
- `xsXtpClientCallJson`
- `xsXtpClientCallSimpleBody`
- `xsXtpClientCallTextBody`
- `xsXtpClientCallJsonBody`
- `xsXtpClientCallSimpleSummary`
- `xsXtpClientCallSimpleSummaryJson`
- `xsXtpClientCallSimpleResult`
- `xsXtpClientCallSimpleError`
- `xsXtpClientCallSimpleMeta`
- `xsXtpClientCallSimpleMetaJson`
- `xsXtpClientCallSimpleValue`
- `xsXtpClientCallSimpleParamsValue`
- `xsXtpClientCallSimpleBodyValue`
- `xsXtpRequestCreate / xsXtpRequestFree`
- `xsXtpRequestSetCmd / xsXtpRequestSetParamsValue / xsXtpRequestSetParamText / xsXtpRequestSetParamInt / xsXtpRequestSetParamBool`
- `xsXtpRequestSetBodyText / xsXtpRequestSetBodyJson / xsXtpRequestSetBodyValue`
- `xsXtpClientDoRequest / xsXtpClientCallRequest`
- `xsXtpClientCallRequestValue / xsXtpClientCallRequestParamsValue / xsXtpClientCallRequestBodyValue`
- `xsXtpClientCallRequestBody / xsXtpClientCallRequestResult / xsXtpClientCallRequestError`
- `xsXtpClientCallRequestMeta / xsXtpClientCallRequestMetaJson`
- `xsXtpClientCallRequestResultJson / xsXtpClientCallRequestErrorJson`
- `xsXtpClientCallRequestOK / xsXtpClientCallRequestStatus / xsXtpClientCallRequestCmd`
- `xsXtpClientCallRequestSummary / xsXtpClientCallRequestSummaryJson`
- `xsXtpClientCallTableText`
- `xsXtpClientCallTableJson`
- `xsXtpClientCallTableValue`
- `xsXtpClientCallSimpleStatus`
- `xsXtpClientCallSimpleCmd`
- `xsXtpClientCallSimpleResultJson`
- `xsXtpClientCallSimpleErrorJson`
- `xsXtpMessageFree`
- `xsXtpClientClose`

这组 API 先面向“脚本内主动连接外部 XTP 服务并同步等待响应”的场景，当前已经区分为两层：

- `ClientOpen + ClientDo* + MessageFree + ClientClose`
- `ClientCall*` 一体化打开/请求/关闭

同时补了：

- `xsXtpIsOK`
- `xsXtpResultText`
- `xsXtpErrorText`
- `xsXtpCmdDup`
- `xsXtpBodyDup`
- `xsXtpMetaText`
- `xsXtpMetaJson`
- `xsXtpValue`
- `xsXtpParamsValue`
- `xsXtpBodyValue`
- `xsXtpErrorValue`
- `xsXtpSummaryText`
- `xsXtpSummaryJson`
- `xsXtpResultText`
- `xsXtpResultJson`
- `xsXtpResultIs`
- `xsXtpStatusIs`
- `xsXtpErrorText`
- `xsXtpErrorJson`

便于像 HTTP 一样先判定响应状态，再安全读取响应 `cmd/body/result/error`，或者直接走“一体化请求并取 body / summary”的快捷路径。后续再继续补更完整的 pending request / response object 体系。

### 8.3 脚本绑定粒度

- `http/ws`：脚本绑定到 `Host`
- `tcp/udp/xtp/custom`：脚本绑定到 `Server`

当前 `server-only` 宿主能力已经覆盖：

- `custom/tcp`：连接打开、收包、关闭、脚本回调
- `udp`：datagram 收包、默认回显、脚本回调与按来源地址回复
- `xtp`：`v2` 二进制包解析、请求/应答模型、零拷贝参数视图、脚本消息回调与回包
	当前脚本层已提供 `xsXtpSendRequest / xsXtpSendPush / xsXtpSendEvent / xsXtpIsRequest / xsXtpIsResponse / xsXtpIsPush / xsXtpIsEvent / xsXtpCmdIs / xsXtpHasParam / xsXtpParamText / xsXtpParamDup / xsXtpParamInt / xsXtpParamBool / xsXtpReplyText / xsXtpReplyJson / xsXtpReplyOKText / xsXtpReplyErrorText / xsXtpReplyOKJson / xsXtpReplyErrorJson / xsXtpReplyMissingParam / xsXtpReplyUnsupportedCmd`

第一阶段已落地的 WebSocket 宿主回调：

- `WsOpenProc`
- `WsTextProc`
- `WsBinaryProc`
- `WsPingProc`
- `WsPongProc`
- `WsCloseProc`
- `ws` server 现已支持 `ws_protocol`，可要求客户端协商固定子协议
- `ws` server 现已支持 `ws_message_limit`，可单独限制单条消息聚合后的最大大小
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
- `__xs/status_json` 已提供结构化宿主状态输出，便于外部工具和页面直接消费
- `__xs/health` 已提供适合探针的文本健康检查与明确状态码
- `__xs/dashboard` 已提供适合终端排障的纯文本总览输出
- `__xs/check_config` / `__xs/check_config_json` 已提供在线配置校验入口，可在不触发 reload 的情况下验证当前配置文件
- `__xs/reload_json` 已提供结构化脚本热重载触发结果输出，便于管理面统一走 JSON
- `__xs/reload_config_json` 已提供结构化配置热加载触发结果输出，便于管理面统一走 JSON
- `__xs/reload_status_json` 已提供结构化配置热加载结果输出，便于管理面直接读取
- `__xs/reload_clear` 已提供最近一次配置热加载结果的显式清空入口
- `__xs/health_json` 已提供宿主 + reload + bus 的轻量健康摘要，适合监控与探测
- `__xs/dashboard_json` 已提供 status / health / reload / bus 的结构化总览，便于首页与外部工具减少多次请求
- `__xs/http_metrics` / `__xs/http_metrics_json` / `__xs/http_metrics_clear` 已提供 HTTP 请求计数、响应分布与清零入口，便于压测窗口、故障复盘与运维观察
- `__xs/ws_metrics` / `__xs/ws_metrics_json` / `__xs/ws_metrics_clear` 已提供 WebSocket 连接、消息、ping/pong、错误计数、最近一次帧类型/字节数/文本摘要/时间上下文、最近一次关闭原因与时间、最近一次错误现场与清零入口，便于 WebSocket 压测窗口与故障复盘
- `__xs/xtp_metrics` / `__xs/xtp_metrics_json` / `__xs/xtp_metrics_clear` 已提供 XTP 连接、坏包计数、消息类型分布、最近一包对端地址、长度与上下文、最近一次坏包原因/时间、最近一次系统错误现场与收发字节统计及清零入口，便于 XTP/XTPS 压测窗口与故障复盘
- `__xs/udp_metrics` / `__xs/udp_metrics_json` / `__xs/udp_metrics_clear` 已提供 UDP 收发包数、字节数、最近一包长度、最近一包上下文与最近一次错误现场及清零入口，便于 UDP 压测窗口与故障复盘
- `__xs/custom_metrics` / `__xs/custom_metrics_json` / `__xs/custom_metrics_clear` 已提供 custom/tcp 连接、收发、错误、坏包计数、最近一次对端地址、最近一次无效输入原因与时间、最近一次断开原因、最近一次系统错误、最近一包长度及最近一次收包上下文与清零入口，便于 TCP 压测窗口与故障复盘
- `__xs/dashboard` 文本版当前也会直接输出 `ws / xtp / udp / custom` 的关键运行态字段，便于终端排障时不切 JSON 也能看到协议侧连接、收发、错误与最近一包上下文
- `__xs/status_json`、`__xs/health_json`、`__xs/dashboard` / `__xs/dashboard_json` 当前额外输出 `bind_ip / bind_port / tls / bind_ip_tls / bind_port_tls / addr_tls / ws_protocol / ws_message_limit / ws_conn_current / ws_conn_peak / ws_open_count / ws_close_count / ws_text_count / ws_binary_count / ws_ping_count / ws_pong_count / ws_error_count / ws_last_error_code / ws_last_close_reason / ws_last_frame_type / ws_last_remote / ws_last_bytes / ws_last_text / ws_last_time / ws_last_age_ms / ws_last_error_time / ws_last_error_age_ms / xtp_conn_current / xtp_conn_peak / xtp_open_count / xtp_close_count / xtp_error_count / xtp_invalid_count / xtp_msg_count / xtp_req_count / xtp_resp_count / xtp_push_count / xtp_event_count / xtp_send_count / xtp_recv_bytes / xtp_send_bytes / xtp_last_msg_type / xtp_last_status / xtp_last_msg_id / xtp_last_flags / xtp_last_param_count / xtp_last_body_size / xtp_last_remote / xtp_last_cmd / xtp_last_time / xtp_last_age_ms / xtp_last_invalid_reason / xtp_last_invalid_time / xtp_last_invalid_age_ms / xtp_last_error_code / xtp_last_error_time / xtp_last_error_age_ms / custom_conn_current / custom_conn_peak / custom_open_count / custom_close_count / custom_error_count / custom_invalid_count / custom_last_invalid_reason / custom_last_invalid_time / custom_last_invalid_age_ms / custom_last_close_reason / custom_last_error_code / custom_last_error_time / custom_last_error_age_ms / custom_recv_count / custom_send_count / custom_recv_bytes / custom_send_bytes / custom_last_remote / custom_last_text / custom_last_time / custom_last_age_ms / tls_cert_file / tls_key_file / tls_ca_file / current_dir / app_file / app_mtime / app_size / app_path / build / compiler / platform / arch / mem_debug / pid / start_time / uptime_ms / engine_workers / runtime_server_count / manage_api / config_name / config_mtime / config_size / http_req_count / http_2xx_count / http_3xx_count / http_4xx_count / http_5xx_count / http_conn_current / http_conn_peak / http_get_count / http_post_count / http_head_count / http_other_count / http_time_total_ms / http_time_max_ms / http_time_avg_ms / http_last_method / http_last_status / http_last_path / http_last_target / http_last_version / http_last_remote / http_last_time / http_last_age_ms / http_last_app_method / http_last_app_status / http_last_app_path / http_last_app_target / http_last_app_version / http_last_app_remote / http_last_app_time / http_last_app_age_ms`，便于定位实际监听地址、TLS 监听、WebSocket 子协议、WebSocket 消息上限、WebSocket 连接与消息计数、最近一次 WebSocket 帧上下文、最近一次 WebSocket 对端地址、最近一次 WebSocket 关闭原因、最近一次 WebSocket 错误现场、XTP 最近一包的 flags、参数数量、body 大小、对端地址与坏包/系统错误现场、TCP 最近一次对端地址、无效输入与传输错误现场、TLS 证书路径、当前工作目录、程序文件与配置文件更新时间和大小、运行进程、目录、构建变体、编译器、目标平台架构、内存调试状态、持续运行时间、运行中的服务数量、工作线程规模、HTTP 请求规模、方法分布、当前连接、峰值连接、最近一次任意请求，以及最近一次非 `__xs/*` 业务请求的方法、路径、协议版本、对端地址、状态与处理耗时，以及管理面启用状态
- `__xs/reload_status_json`、`__xs/health_json`、`__xs/dashboard_json` 当前额外输出 `reload_total_count / reload_success_count / reload_failure_count`，用于区分最近一次结果与累计重载统计
- `__xs/reload_reset` 已提供配置热加载累计统计清零入口；与 `__xs/reload_clear` 的“只清最近一次结果”语义分开，便于窗口化观察 reload 统计
- `__xs/check_config_json`、`__xs/dashboard_json` 当前额外输出 `check_total_count / check_success_count / check_failure_count / check_last_time / check_last_age_ms`，用于观察在线配置校验的累计调用、最近一次校验时间以及距离上次校验过去多久
- `__xs/health` 文本接口当前也会输出 `reload_time / reload_age_ms / check_total_count / check_success_count / check_failure_count / check_last_time / check_last_age_ms / bus_queue_count / bus_data_count / bus_total_queued / bus_total_delivered / bus_total_dropped / bus_last_queue_time / bus_last_queue_time_text / bus_last_dispatch_time / bus_last_dispatch_time_text`，便于纯文本探针直接观察热加载、配置校验与总线运行态
- `__xs/health_json` 当前也已补齐 `check_total_count / check_success_count / check_failure_count / check_last_time / check_last_age_ms / bus_total_queued / bus_total_delivered / bus_total_dropped / bus_last_queue_time / bus_last_queue_time_text / bus_last_dispatch_time / bus_last_dispatch_time_text / http_last_method / http_last_status / http_last_path / http_last_target / http_last_time / http_last_age_ms`
- 首页状态卡当前已直接展示 `Reload Last / Reload Age / Reload Total / Reload Success / Reload Failure / Check Last / Check Age / Check Total / Check Success / Check Failure / HTTP Last / HTTP Last Age / HTTP Last Path / Bus Queue Time / Bus Dispatch Time`
- `__xs/check_config_clear` 已提供在线配置校验统计清零入口，便于按时间窗口重新观察配置校验调用情况
- `__xs/bus/status` / `__xs/bus/registry` / `__xs/bus/namespaces` / `__xs/bus/find` / `__xs/bus/exists` / `__xs/bus/get` / `__xs/bus/values` / `__xs/bus/retain` / `__xs/bus/release` / `__xs/bus/touch` / `__xs/bus/register` / `__xs/bus/set` / `__xs/bus/send` / `__xs/bus/remove` / `__xs/bus/reset` 已作为宿主层总线管理接口落地，且首页已支持显式 `data_id` 管理，也支持仅通过 `namespace/tag` 读取、续期、`retain/release`、更新与删除共享数据
- bus 管理面已提供累计统计：`total_queued / total_delivered / total_dropped / last_queue_time / last_dispatch_time / last_queue_time_text / last_dispatch_time_text`，其中 `__xs/bus/reset` 只清空统计，不清空共享数据表

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



## 19. 当前管理面补充

当前 `__xs/status_json / __xs/dashboard_json / __xs/http_metrics_json` 已包含 HTTP 最近请求上下文：

- `http_last_method`
- `http_last_status`
- `http_last_path`
- `http_last_target`
- `http_last_version`
- `http_last_remote`
- `http_last_time`
- `http_last_age_ms`
- `http_last_duration_ms`
- `http_last_body_len`
- `http_last_content_type`
- `http_last_header_count`
- `http_last_query_len`
- `http_last_host`
- `http_last_user_agent`
- `http_last_referer`
- `http_last_origin`
- `http_last_accept`
- `http_last_accept_encoding`
- `http_last_cookie`
- `http_last_forwarded_for`
- `http_last_real_ip`
- `http_last_connection`
- `http_last_cache_control`
- `http_manage_req_count`
- `http_app_req_count`

同时也包含最近一次业务 HTTP 请求上下文：

- `http_last_app_method`
- `http_last_app_status`
- `http_last_app_path`
- `http_last_app_target`
- `http_last_app_version`
- `http_last_app_remote`
- `http_last_app_time`
- `http_last_app_age_ms`
- `http_last_app_duration_ms`
- `http_last_app_body_len`
- `http_last_app_content_type`
- `http_last_app_header_count`
- `http_last_app_query_len`
- `http_last_app_host`
- `http_last_app_user_agent`
- `http_last_app_referer`
- `http_last_app_origin`
- `http_last_app_accept`
- `http_last_app_accept_encoding`
- `http_last_app_cookie`
- `http_last_app_forwarded_for`
- `http_last_app_real_ip`
- `http_last_app_connection`
- `http_last_app_cache_control`
