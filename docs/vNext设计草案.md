# XServer vNext 设计草案

## 1. 目标

XServer vNext 是一次从零开始的宿主层重构�?

重构目标�?

- �?`xserver` 全面融合 `xrt` 体系
- 保持开箱即用、配置简单的核心特点
- 将安全性、稳定性、高性能作为默认要求
- 保留 C 语言脚本的最大灵活�?
- 保持�?`main.c` 入口、按功能拆分 `.h` 文件的工程风�?



## 2. 职责边界

### 2.1 xrt 职责

`xrt` 负责基础设施与通用能力�?

- 基础运行�?
- 内存管理与调�?
- 多线程与协程任务系统
- 事件循环
- TCP / UDP / TLS 等网络基础设施
- HTTP / WebSocket 等通用网络上层能力

### 2.2 xserver 职责

`xserver` 负责宿主层能力：

- 配置加载、校验、标准化
- Server / Host 对象装配
- C 脚本宿主与热加载
- 多协议业务装�?
- 虚拟主机管理
- 业务扩展协议
- 默认安全策略
- 运行时治理与诊断入口

### 2.3 业务扩展协议

`xtp` 这类协议不回灌到 `xrt`，保留在 `xserver`�?

原则�?

- 通用能力进入 `xrt`
- 业务扩展协议保留�?`xserver`



## 3. 协议分层

XServer vNext 的协议模型分三层�?

### 3.1 基础设施协议�?

直接来自 `xrt`�?

- TCP
- UDP
- TLS
- EventLoop
- Network Buffer

### 3.2 通用应用层协议层

优先�?`xrt` 提供�?

- HTTP
- WebSocket

### 3.3 业务扩展协议�?

�?`xserver` 维护�?

- XTP
- 后续私有二进制协�?
- 基于 `custom` 的自定义协议装配



## 4. 核心抽象

### 4.1 Server 是统一核心抽象

所有协议都必须�?`Server`�?

`Server` 负责�?

- 协议类型
- 监听地址
- 运行状�?
- 脚本宿主�?Host 集合
- 对应的底�?`xrt` 服务句柄

### 4.2 Host 不是全协议统一抽象

`Host` 只用�?`host-aware` 协议�?

- HTTP
- WebSocket

`Host` 不强行用于：

- TCP
- UDP
- XTP
- Custom

### 4.3 协议对象模型

#### Host-aware 协议

对象关系�?

`Runtime -> Server -> Host`

适用于：

- HTTP
- WebSocket

#### Server-only 协议

对象关系�?

`Runtime -> Server`

适用于：

- TCP
- UDP
- XTP
- Custom



## 5. 开发模式分�?

XServer vNext 默认支持以下开发模式：

- `static`
- `script-c`
- `protocol`

说明�?

- `static` 用于静态文件服�?
- `script-c` 用于�?TCC 为宿主的 C 脚本业务
- `protocol` 用于事件驱动、自定义协议或扩展协议开�?



## 6. 服务类型

vNext 建议保留�?`class`�?

- `http`
- `ws`
- `tcp`
- `udp`
- `xtp`
- `custom`

说明�?

- `thread` 被移�?
- �?`xrt` 来说，`thread` �?`custom` 不再需要作为两种独立模�?



## 7. 配置系统

### 7.1 设计目标

配置系统必须进入严格模式优先的生产状态�?

要求�?

- 类型严格校验
- 启动前完成语义校�?
- 路径统一标准�?
- 允许未知字段，但只给出警�?

### 7.2 配置规则

- 未知字段：警告，不报�?
- 类型错误：直接报错，不自动纠�?
- 不支持旧版本兼容模式
- 相对路径统一以配置文件所在目录为基准

### 7.3 配置层分�?

配置处理必须拆成两个阶段�?

1. 解析与标准化
2. 运行时对象构�?

禁止在配置解析阶段直接做这些事：

- 启动网络服务
- 编译脚本
- 建立事件循环依赖
- 做不可回滚的运行时副作用

### 7.4 配置模型

建议显式拆分�?

- `XS_Config`
- `XS_ServerConfig`
- `XS_HostConfig`

其中�?

- `http/ws` 使用 `server + host`
- `tcp/udp/xtp/custom` 使用 `server-only`

Server 监听配置采用�?`class` 分化后的字段模型�?

- 主模型使�?`ip + port`
- `http` �?TLS 扩展使用 `tls + port_tls`
- `ip_tls` 仅作为可选覆盖字段，默认复用 `ip`
- 不再支持 `addr / addr_tls` 这类 URL 风格绑定字段



## 8. 脚本宿主模型

### 8.1 基本原则

TCC 仍然作为全功�?C 语言脚本宿主，不做权限限制�?

理由�?

- TCC 本身具备完整 C 语言能力
- 无法真正通过简单导出裁剪实现安全隔�?
- 与其做虚假的权限控制，不如保留最大灵活�?

### 8.2 宿主责任

虽然脚本保持全开放，�?`xserver` 仍然要提供稳定的宿主便利层：

- 脚本生命周期管理
- 脚本热加�?
- 全局数据注入
- 全局消息总线
- 回滚机制
- �?Server / Host 对象绑定

第一阶段已经提供一套最小宿�?API�?

- `xs_vnext.h`
- `xs_vnext_full.h`

其中�?

- `xs_vnext.h` 提供稳定宿主便利函数
- `xs_vnext_full.h` 额外引入 `xrt / libtcc / sqlite3` 的宿主可用头环境，用于旧脚本迁移�?- TCC 宿主当前会优先按 `xs/xs.exe` 所在目录解�?`release/tcc` 运行时资源，因此使用相对 `devfile` 的配置不再依赖当前工作目录；从项目根目录�?`release` 目录启动都应能稳定加载脚�?
当前 `xtp` 脚本宿主已经提供第一版同步客户端请求/应答能力�?
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

这组 API 先面向“脚本内主动连接外部 XTP 服务并同步等待响应”的场景，当前已经区分为两层�?

- `ClientOpen + ClientDo* + MessageFree + ClientClose`
- `ClientCall*` 一体化打开/请求/关闭

同时补了�?

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

便于�?HTTP 一样先判定响应状态，再安全读取响�?`cmd/body/result/error`，或者直接走“一体化请求并取 body / summary”的快捷路径。后续再继续补更完整�?pending request / response object 体系�?

### 8.3 脚本绑定粒度

- `http/ws`：脚本绑定到 `Host`
- `tcp/udp/xtp/custom`：脚本绑定到 `Server`

当前 `server-only` 宿主能力已经覆盖�?

- `custom/tcp`：连接打开、收包、关闭、脚本回�?
- `udp`：datagram 收包、默认回显、脚本回调与按来源地址回复
- `xtp`：`v2` 二进制包解析、请�?应答模型、零拷贝参数视图、脚本消息回调与回包
	当前脚本层已提供 `xsXtpSendRequest / xsXtpSendPush / xsXtpSendEvent / xsXtpIsRequest / xsXtpIsResponse / xsXtpIsPush / xsXtpIsEvent / xsXtpCmdIs / xsXtpHasParam / xsXtpParamText / xsXtpParamDup / xsXtpParamInt / xsXtpParamBool / xsXtpReplyText / xsXtpReplyJson / xsXtpReplyOKText / xsXtpReplyErrorText / xsXtpReplyOKJson / xsXtpReplyErrorJson / xsXtpReplyMissingParam / xsXtpReplyUnsupportedCmd`

第一阶段已落地的 WebSocket 宿主回调�?

- `WsOpenProc`
- `WsTextProc`
- `WsBinaryProc`
- `WsPingProc`
- `WsPongProc`
- `WsCloseProc`
- `ws` server 现已支持 `ws_protocol`，可要求客户端协商固定子协议
- `ws` server 现已支持 `ws_message_limit`，可单独限制单条消息聚合后的最大大小；`ws/wss` 也已支持 `idle_timeout`
- `http/ws/wss/custom/tcp/xtp/xtps` 现已支持统一 `conn_limit`，超过上限时宿主会主动关闭超额连接，管理面会同步暴露 `conn_limit`；同时协议级 metrics 也开始区分“超额连接关闭”现场，分别暴露 `*_conn_limit_close_count / *_last_conn_limit_close_time / *_last_conn_limit_close_age_ms`
- �?`ws_protocol` 已配置时，未协商正确子协议的握手请求会直接拒�?
- `ws` 现已支持 `tls + port_tls` �?`wss` 运行模式



## 9. 生命周期

### 9.1 Runtime 生命周期

建议流程�?

1. 初始�?`xrt`
2. 解析命令�?
3. 加载配置
4. 校验配置
5. 创建 Runtime
6. 构建 Server / Host 运行时对�?
7. 加载脚本
8. 调用 `ServiceInit`
9. 启动网络服务
10. 进入主循�?
11. 停止接收新流�?
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

�?`http/ws` 需�?Host 生命周期�?

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
- `ServiceStop` 用于停止接收新工�?
- `ServiceUnit` 必须在对象最终销毁前调用



## 10. 全局总线与共享数�?

### 10.1 设计目标

为独�?TCC 上下文之间提供统一的通信与数据共享机制�?

目标�?

- 支持�?Server / Host 的消息投�?
- 支持跨上下文共享 `xvalue` 数据
- 消息和数据注册表统一由宿主层维护
- 消息本体保持轻量，复杂数据通过 `data_id` 访问

### 10.2 机制组成

全局总线由两部分组成�?

- 消息队列
- 全局数据注册�?

消息负责通知目标上下文做事�?
数据注册表负责保存和共享 `xvalue`�?

### 10.3 数据模型

第一阶段使用 `int64` 作为全局数据主键�?

原因�?

- 查找更快
- 消息包更�?
- 生成简�?
- 更适合作为跨上下文句柄

建议宿主 API�?

- `xsDataRegister`
- `xsDataRegisterEx`
- `xsDataGet`
- `xsDataRetain`
- `xsDataRelease`
- `xsDataRemove`

脚本�?`xsDataRetain / xsDataRelease` 现也会受 `readonly_namespaces / disabled_namespaces` 约束；命中策略时应结�?`xsBusLastErrorCode / xsBusLastError` 读取失败原因�?
其中第一阶段建议支持�?

- `namespace`
- `tag`
- `ttl`

### 10.4 消息模型

建议宿主 API�?

- `xsMsgSendToServer`
- `xsMsgSendToHost`
- `xsMsgBroadcast`

脚本通过消息只传�?

- `topic`
- `data_id`
- 可选参数对�?

接收方在 `MessageProc` 中按 `data_id` 读取共享数据�?

### 10.5 当前实现约束

第一阶段仅支持：

- 同进程内
- 宿主层队列分�?
- `xvalue` 共享数据

当前不包括：

- 跨进�?
- 跨机�?
- 持久化总线

第一阶段已经开始提供：

- registry 状态导�?
- `namespace / tag` 定位与删�?
- `namespace / tag` 条件过滤与批量清�?
- namespace 聚合统计视图
- 注册失败错误码与错误文本
- 过期自动清理骨架

第一阶段保留约定�?

- `xs.*`
- `__xs*`

以上命名空间保留给系统内部使用，业务脚本不应注册到这些命名空间�?



## 11. 热加载设�?

### 10.1 热加载分�?

分为两类�?

- 脚本热加�?
- 配置热加�?

### 10.2 脚本热加�?

只替换脚本宿主，不修改配置�?

适用场景�?

- 业务逻辑更新
- C 脚本重新编译

### 10.3 配置热加�?

重新解析并校验配置，再决定：

- 哪些 Server 保持不变
- 哪些 Host 仅热更新脚本
- 哪些对象需要重�?
- 哪些服务需要重�?

第一阶段最小实现要求：

- 通过 HTTP 调试入口触发 `reload_config`
- 先完成新配置�?`load/build/init`
- 仅在启动前最后一步停止旧服务
- 如果新服务启动失败，立即恢复�?runtime
- 支持 `server` 定向重载
- �?`http/ws` 支持 Host 原位重载
- 提供 `reload_status` 查询最近一次异步重载结�?
- 网络服务统一支持 `backlog / recv_limit` 这类基础安全与容量参�?
- HTTP 宿主层额外支�?`path_limit / header_limit / body_limit` 这类请求入口限制
- 静�?Host 默认仅接�?`GET / HEAD`，避免实验期过于宽松的行为进入生�?
- 静�?Host 默认拒绝敏感路径与敏感扩展名暴露，HTTP 响应默认追加基础安全�?
- `__xs/status_json` 已提供结构化宿主状态输出，便于外部工具和页面直接消�?- `__xs/health` 已提供适合探针的文本健康检查与明确状态码
- 当前管理面边界开始按编译变体收口：`build.bat` 产出�?`xs` 默认只保�?`status / health / reload / check_config` 这组最小核心入口；`build_debug.bat` 产出�?`xsdbg` 继续开�?`dashboard`��`bus` �?`http/ws/xtp/udp/custom` 这组详细 protocol metrics 入口，用于把生产�?`xs` 的运行链路和控制面保持得更简单、更稳定。首�?`index.html` �?`xs` 下也会自动退�?`status_json + health_json + check_config_json` 组合，不再因�?`dashboard_json` 被禁用而整页报�?- `__xs/dashboard` 已提供适合终端排障的纯文本总览输出 ��ҳװ������ǰҲ�Ѹĳ��ȶ� `status_json` ���ж��Ƿ���Ҫ��չ�����棬��ͨ `xs` �������ڳ��μ���ʱ�ȴ�һ����Ȼ `403` �� `dashboard_json`
- `__xs/check_config` / `__xs/check_config_json` 已提供在线配置校验入口，可在不触�?reload 的情况下验证当前配置文件
- `__xs/reload_json` 已提供结构化脚本热重载触发结果输出，便于管理面统一�?JSON
- `__xs/reload_config_json` 已提供结构化配置热加载触发结果输出，便于管理面统一�?JSON
- `__xs/reload_status_json` 已提供结构化配置热加载结果输出，便于管理面直接读�?
- `__xs/reload_clear` 已提供最近一次配置热加载结果的显式清空入�?
- `__xs/health_json` 已提供宿�?+ reload + bus 的轻量健康摘要，适合监控与探�?
- `__xs/dashboard_json` 已提�?status / health / reload / bus 的结构化总览，便于首页与外部工具减少多次请求
- ��ǰ��һ���ְ� `protocol/http.h` �ڲ�������ʱ/���� helper �����鵽 `src/manage/http_runtime.h`�����ǰһ���Ѿ������ `src/manage/http_manage.h`��`src/protocol/http.h` ������Ҫ����Э����ڡ���̬��Դ������ص� glue�������� `protocol` Ŀ¼���ջص����ӽ� `old/legacy` ��ְ��߽� ��������ԭ��ɢ���� `protocol/http.h` ��Ĵ�ι����� if-chain Ҳ���ճɵ�һ `XS_HttpHandleManageRequest(...)` ��ڣ�Э���ֻ����һ�ι�����ת����
- `src/manage/http_manage.h` ��ǰҲ�Ѽ�����ɺ��Ĺ����桢`src/manage/http_manage_bus.h` �� `src/manage/http_manage_debug.h`���������� freeze `xs` API ʱ����ֱ�ӻ����������տڣ������ٰ���չ�����߼��ѻص������ļ�
- ��ǰ release ����Ҳ��ͨ�� `#ifdef XRT_MEM_DEBUG` �� `bus / dashboard / *_metrics*` ������չ handler �� include �� dispatch �ս� `xsdbg`����ͨ `xs` �ı�����ֻ�����������Ĺ�������ܾ�����
- ͬһ���`protocol/ws.h` �� `protocol/custom.h` �����������Ӹ��١�idle thread��reject/invalid/stop-cleanup ͳ�ƣ�Ҳ�ֱ�����鵽�� `src/manage/ws_runtime.h` �� `src/manage/custom_runtime.h`���� `protocol` Ŀ¼��һ���ص���Э��ص�������
- ��һ���ְ� `protocol/xtp.h` �����������Ӹ��١�idle/stop-cleanup��reject/invalid/error remote ���յ�����ʱ���� helper �����鵽�� `src/manage/xtp_runtime.h`��`XTP` ����Ϣ�ṹ��ͬ���ͻ��˺͸߼��ӿ���Ȼ������ `protocol/xtp.h`������Ӱ�쵱ǰ��ص� `XTP` �߲�������
- �����ְ� `main.c` ����������ʱ״̬�ֿ���״̬���� API ������ `src/manage/runtime_state.h` �� `src/manage/runtime_state_api.h`��`main.c` �����Ѿ����Ի��䵽���ӽ�����������������̡�����̬��������� freeze `xs` �����߽�ʱ�����Ļ�������� `src/manage`���������ٻص� `protocol`��
- `http` 当前也支�?`idle_timeout`，超时空闲连接会被宿主主动关闭；`__xs/http_metrics` / `__xs/http_metrics_json` / `__xs/http_metrics_clear` 已提�?HTTP 请求计数、响应分布、空闲关闭计�?时间与清零入口，便于压测窗口、故障复盘与运维观察
- `ws/wss` 当前也支�?`idle_timeout`，超时空闲连接会被宿主主动关闭；`__xs/ws_metrics` / `__xs/ws_metrics_json` / `__xs/ws_metrics_clear` 已提�?WebSocket 连接、消息、ping/pong、错误计数、空�?连接上限/消息上限关闭计数与时间、最近一次帧类型/字节�?文本摘要/时间上下文、最近一次关闭原因与时间、最近一次错误现场与清零入口，便�?WebSocket 压测窗口与故障复�?- `xtp/xtps` 当前已支�?`idle_timeout`，超时空闲连接会被宿主主动关闭；`__xs/xtp_metrics` / `__xs/xtp_metrics_json` / `__xs/xtp_metrics_clear` 已提�?XTP 连接、坏包计数、消息类型分布、最近一包对端地址、长度与上下文、最近一次坏包原�?时间、最近一次系统错误现场、空�?连接上限/收包上限关闭计数与时间、收发字节统计及清零入口，便�?XTP/XTPS 压测窗口与故障复�?- `__xs/udp_metrics` / `__xs/udp_metrics_json` / `__xs/udp_metrics_clear` 已提�?UDP 收发包数、字节数、最近一包长度、最近一包上下文与最近一次错误现场及清零入口，便�?UDP 压测窗口与故障复�?
- `custom/tcp` 当前也支�?`idle_timeout`，超时空闲连接会被宿主主动关闭；`__xs/custom_metrics` / `__xs/custom_metrics_json` / `__xs/custom_metrics_clear` 已提�?custom/tcp 连接、收发、错误、坏包计数、最近一次对端地址、最近一次无效输入原因与时间、最近一次断开原因、最近一次系统错误、最近一包长度、空�?连接上限/收包上限关闭计数与时间及最近一次收包上下文与清零入口，便于 TCP 压测窗口与故障复�?- `__xs/dashboard` 文本版当前也会直接输�?`ws / xtp / udp / custom` 的关键运行态字段，便于终端排障时不�?JSON 也能看到协议侧连接、收发、错误与最近一包上下文
- `__xs/status_json`、`__xs/health_json`、`__xs/dashboard` / `__xs/dashboard_json` 当前额外输出 `bind_ip / bind_port / tls / bind_ip_tls / bind_port_tls / addr_tls / ws_protocol / ws_message_limit / ws_conn_current / ws_conn_peak / ws_open_count / ws_close_count / ws_text_count / ws_binary_count / ws_ping_count / ws_pong_count / ws_error_count / ws_invalid_count / ws_idle_close_count / ws_conn_limit_close_count / ws_message_limit_close_count / ws_last_error_code / ws_last_close_reason / ws_last_frame_type / ws_last_remote / ws_last_bytes / ws_last_text / ws_last_time / ws_last_age_ms / ws_last_error_time / ws_last_error_age_ms / ws_last_reject_reason / ws_last_reject_remote / xtp_conn_current / xtp_conn_peak / xtp_open_count / xtp_close_count / xtp_error_count / xtp_invalid_count / xtp_msg_count / xtp_req_count / xtp_resp_count / xtp_push_count / xtp_event_count / xtp_send_count / xtp_recv_bytes / xtp_send_bytes / xtp_last_msg_type / xtp_last_status / xtp_last_msg_id / xtp_last_flags / xtp_last_param_count / xtp_last_body_size / xtp_last_remote / xtp_last_cmd / xtp_last_time / xtp_last_age_ms / xtp_last_invalid_reason / xtp_last_invalid_time / xtp_last_invalid_age_ms / xtp_last_error_code / xtp_last_error_time / xtp_last_error_age_ms / xtp_last_reject_reason / xtp_last_reject_remote / xtp_idle_close_count / xtp_conn_limit_close_count / xtp_recv_limit_close_count / custom_conn_current / custom_conn_peak / custom_open_count / custom_close_count / custom_error_count / custom_invalid_count / custom_last_invalid_reason / custom_last_invalid_time / custom_last_invalid_age_ms / custom_last_close_reason / custom_last_error_code / custom_last_error_time / custom_last_error_age_ms / custom_last_reject_reason / custom_last_reject_remote / custom_recv_count / custom_send_count / custom_recv_bytes / custom_send_bytes / custom_last_remote / custom_last_text / custom_last_time / custom_last_age_ms / custom_idle_close_count / custom_conn_limit_close_count / custom_recv_limit_close_count / tls_cert_file / tls_key_file / tls_ca_file / current_dir / app_file / app_mtime / app_size / app_path / build / compiler / platform / arch / mem_debug / pid / start_time / uptime_ms / engine_workers / runtime_server_count / manage_api / config_name / config_mtime / config_size / http_req_count / http_2xx_count / http_3xx_count / http_4xx_count / http_5xx_count / http_conn_current / http_conn_peak / http_get_count / http_post_count / http_head_count / http_other_count / http_time_total_ms / http_time_max_ms / http_time_avg_ms / http_last_method / http_last_status / http_last_path / http_last_target / http_last_version / http_last_remote / http_last_time / http_last_age_ms / http_last_reject_reason / http_last_reject_remote / http_last_app_method / http_last_app_status / http_last_app_path / http_last_app_target / http_last_app_version / http_last_app_remote / http_last_app_time / http_last_app_age_ms`，便于定位实际监听地址、TLS 监听、WebSocket 子协议、WebSocket 消息上限、WebSocket 连接与消息计数、最近一�?WebSocket 帧上下文、最近一�?WebSocket 对端地址、最近一�?WebSocket 关闭原因、最近一�?WebSocket 错误现场，以及最近一�?reject 现场；XTP 最近一包的 flags、参数数量、body 大小、对端地址与坏�?系统错误现场、最近一�?reject 现场，以及空�?连接上限/收包上限关闭计数；TCP 最近一次对端地址、无效输入、传输错误与 reject 现场，以及空�?连接上限/收包上限关闭计数；TLS 证书路径、当前工作目录、程序文件与配置文件更新时间和大小、运行进程、目录、构建变体、编译器、目标平台架构、内存调试状态、持续运行时间、运行中的服务数量、工作线程规模、HTTP 请求规模、方法分布、当前连接、峰值连接、最近一次任意请求，以及最近一次非 `__xs/*` 业务请求的方法、路径、协议版本、对端地址、状态与处理耗时，以及管理面启用状�?- `ws / xtp / custom` 当前也已经单独暴�?`*_last_invalid_remote / *_last_error_remote`；坏包和系统错误的对端地址不再继续复用易被后续正常流量覆盖的通用 `last_remote`，首页对应状态卡也会直接消费这组专用 remote
- `__xs/reload_status_json`、`__xs/health_json`、`__xs/dashboard_json` 当前额外输出 `reload_total_count / reload_success_count / reload_failure_count`，用于区分最近一次结果与累计重载统计
- `__xs/reload_reset` 已提供配置热加载累计统计清零入口；与 `__xs/reload_clear` 的“只清最近一次结果”语义分开，便于窗口化观察 reload 统计；新一�?`reload_config` 排队当前也不会再把这组累计计数提前清�?- 同一 `server/host` 上的并发 `__xs/reload` / `__xs/reload_json`，以及定向到同一 Host �?`__xs/reload_config` / `__xs/reload_config_json` 热重载，当前也已共用宿主级互斥门闩；直接重入请求会稳定返�?`409 + reload busy`，而异步目�?Host reload 会计�?reload 失败统计并在日志里记�?`target host reload busy`，避�?Host 脚本状态被并发替换
- `__xs/reload_config` / `__xs/reload_config_json` 的排队门闩当前也已经改成原子状态流转；并发请求命中同一�?reload 窗口时，后续请求会稳定返�?`busy`，不会再覆盖已经挂起�?target server / host
- `__xs/reload_config` / `__xs/reload_config_json` 在传�?`host=<主机�?` 且目�?`server` 就是当前服务时，当前也会先做同步 Host 存在性校验；�?`GET /__xs/reload_config_json?host=<missing>` 这类明显无效目标会直接返�?`404 + reload host not found`，不再先排队再在 `reload_status` 里迟到失�?- `__xs/reload_clear` / `__xs/reload_reset` 当前�?reload 仍处�?`busy` 时，也只会清结果或清累计统计，不会把正在进行�?reload 伪装成空闲；`busy` 与当�?target server / host 会继续保留到本轮 reload 完成
- `__xs/reload` / `__xs/reload_json` / `__xs/reload_config` / `__xs/reload_config_json` �?`force` 参数当前也已切到严格布尔校验；像 `force=maybe` 这类非法值会直接返回 `400 invalid force`，不再静默按 `false` 回退
- reload 状态读写当前也已统一走同一份受保护快照；`__xs/reload_status` / `__xs/reload_status_json`、`__xs/health` / `__xs/health_json`、`__xs/dashboard` / `__xs/dashboard_json` 在并�?`reload_json / reload_clear / reload_reset` 压力下，也不会再在单次响应里混出半新半旧�?`busy / has_result / server / host / message / reload_*count` 组合
- 当前 `__xs/reload` / `__xs/reload_json`、`__xs/reload_config` / `__xs/reload_config_json`、`__xs/reload_status` / `__xs/reload_status_json`、`__xs/check_config` / `__xs/check_config_json`、`__xs/status` / `__xs/status_json`、`__xs/health` / `__xs/health_json`、`__xs/dashboard` / `__xs/dashboard_json` 以及 `__xs/http_metrics` / `__xs/ws_metrics` / `__xs/xtp_metrics` / `__xs/udp_metrics` / `__xs/custom_metrics` 与各�?`_json` 版本，当前都已经按同一套字段口径收平；其中 `*_clear` / `reload_clear` / `reload_reset` / `check_config_clear` 的文本响应也已对齐对应主接口的键集，便于直接做脚本化 diff、回归归档与窗口化运维观�?- `__xs/check_config_json`、`__xs/dashboard_json` 当前额外输出 `check_total_count / check_success_count / check_failure_count / check_last_time / check_last_age_ms`，用于观察在线配置校验的累计调用、最近一次校验时间以及距离上次校验过去多�?- `__xs/health` 文本接口当前也会输出 `reload_time / reload_age_ms / check_total_count / check_success_count / check_failure_count / check_last_time / check_last_age_ms / bus_queue_count / bus_data_count / bus_total_queued / bus_total_delivered / bus_total_dropped / bus_last_queue_time / bus_last_queue_time_text / bus_last_dispatch_time / bus_last_dispatch_time_text`，便于纯文本探针直接观察热加载、配置校验与总线运行�?- `__xs/health_json` 当前也已补齐 `check_total_count / check_success_count / check_failure_count / check_last_time / check_last_age_ms / bus_total_queued / bus_total_delivered / bus_total_dropped / bus_last_queue_time / bus_last_queue_time_text / bus_last_dispatch_time / bus_last_dispatch_time_text / http_last_method / http_last_status / http_last_path / http_last_target / http_last_time / http_last_age_ms`
- 首页状态卡当前已直接展�?`Reload Last / Reload Age / Reload Total / Reload Success / Reload Failure / Check Last / Check Age / Check Total / Check Success / Check Failure / HTTP Last / HTTP Last Age / HTTP Last Path / Bus Queue Time / Bus Dispatch Time`
- `__xs/check_config_clear` 已提供在线配置校验统计清零入口，便于按时间窗口重新观察配置校验调用情�?
- `__xs/bus/status` / `__xs/bus/registry` / `__xs/bus/namespaces` / `__xs/bus/find` / `__xs/bus/exists` / `__xs/bus/get` / `__xs/bus/values` / `__xs/bus/retain` / `__xs/bus/release` / `__xs/bus/touch` / `__xs/bus/register` / `__xs/bus/set` / `__xs/bus/send` / `__xs/bus/remove` / `__xs/bus/sweep` / `__xs/bus/reset` 已作为宿主层总线管理接口落地，且首页已支持显�?`data_id` 管理，也支持仅通过 `namespace/tag` 读取、续期、`retain/release`、更新与删除共享数据
- `__xs/bus/*` 管理接口在传入非法或保留 namespace 时，现已统一复用同一�?`400 bad request` 响应链；`register/send` 也已�?`find/get/set/remove` 等接口保持一致，都会直接返回 `{"result":false,"message":"invalid namespace",...}`
- `__xs/bus/send` 现只接受 `target=server|host|broadcast`；传入其他值时会直接返�?`400` + `{"result":false,"message":"invalid target",...}`，不再静默回退成默�?`server` 投�?- `__xs/bus/exists / get / retain / release / touch / set / remove / send(data_id)` 在传入非�?`id/data_id` 时，现也统一返回 `400` + `{"result":false,"message":"invalid data id","data_id":0}`，不再把非数字值静默当�?`0`
- `__xs/bus/touch / register / set / send(ttl)` 在传入非�?`ttl` 时，现也统一返回 `400` + `{"result":false,"message":"invalid ttl","ttl":0}`；其�?`touch` 的数值约束不变，传入 `ttl=0` 这类非正整数时仍会返�?`ttl must be greater than 0`
- `__xs/bus/*` 这组本身返回 JSON 的管理接口，�?`api disabled / build failed / stringify failed` 这类直接错误分支里，现也继续保持 `application/json` 口径；例如命�?`disabled.local` 这类禁用管理 host 时，`bus/status / bus/namespaces / bus/send` 会直接返回结构化 `403 JSON`，不再回退�?`text/plain`
- `__xs/bus/sweep?all=true&max_pass=<count>` �?`__xs/bus/limits` 现也切到严格十进制参数校验：`abc` 这类非法文本会统一返回 `400` + `{"result":false,"message":"invalid <field>",...}`，不再静默折叠成 `0`；�?`max_pass<=0`、`data_limit<0` 这类范围约束�?namespace rule 校验当前也继续保�?`max_pass must be > 0`、`data_limit must be >= 0`、`namespace must not contain empty segment` 这类�?message 语义，但外层响应现已统一�?`application/json`
- `__xs/bus/remove?all=<bool>`、`__xs/bus/sweep?all=<bool>&drain=<bool>`、`__xs/bus/send?persist=<bool>` 这组布尔参数当前也已切到严格校验；像 `all=maybe`、`drain=maybe`、`persist=maybe` 这类非法值会直接返回 `400` + `{"result":false,"message":"invalid <field>"}`，不再静默按 `false` 处理
- bus 管理面已提供累计统计：`total_queued / total_delivered / total_dropped / last_queue_time / last_dispatch_time / last_queue_time_text / last_dispatch_time_text`，其�?`__xs/bus/reset` 只清空统计，不清空共享数据表，且会直接返�?reset 后的 `bus/status` 同口�?JSON 快照，但不会因为这次返回而额外触发一�?host �?sweep
- `__xs/bus/sweep` 可手动触发一次过期共享数据清理，并直接返回当�?`data_limit / queue_limit / sweep / cleanup` 摘要，方便压测、巡检与故障排查时立即验证清理效果
- `__xs/bus/remove?all=true` 已支持直接批量删除当�?registry 中的共享数据；如同时提供 `namespace/tag`，则只删除匹配条件的数据，并会持续删除直到当前匹配集合清空，而不是停在单�?`256` �?- `__xs/bus/sweep?all=true` 已支持在单次管理调用中循环清理过期积压，`max_pass` 可限制最�?sweep 轮数，适合一次性处理超过单�?`256` 条的过期共享数据
- `__xs/bus/limits` 现已支持 `namespace_limit / namespace_data_limit / sweep_interval_ms`，可对活�?namespace 数量、单 namespace 共享数据量以及宿主层自动 sweep 节奏做在线治理，并统一复用现有 `409 bus_limit` 观测链路
- `__xs/bus/limits` 已继续补�?namespace 级宿主管理规则：`readonly_namespaces` 会拒�?`register/set/touch/remove/retain/release` 这类会改写共享对象状态的操作，`disabled_namespaces` 会进一步拒绝这些操作以及基于该 namespace �?`send`，`ttl_required_namespaces` 会要求命中的 namespace 共享数据最终必须带 TTL，拒�?`register` 或会把对象继续留成持久数据的 `set/update` 路径，但允许通过 `touch` �?`set(ttl>0)` 给旧持久数据补上 TTL；`tag_required_namespaces` 会要求命中的 namespace 共享数据必须�?tag，并拒绝�?tag �?`register/set/touch`；并补齐 `readonly_namespace_reject_count / disabled_namespace_reject_count / ttl_required_namespace_reject_count / tag_required_namespace_reject_count / last_*_action`；规则项同时支持精确 namespace �?`tenant.*` 这类前缀规则
- `__xs/bus/namespaces` 已补�?namespace 级治理观测：顶层可直接看�?`namespace_count / namespace_limit_remaining / namespace_limit_reached / namespace_item_count / readonly_namespace_count / disabled_namespace_count / ttl_required_namespace_count / tag_required_namespace_count`，其�?`namespace_count` 表示当前有活跃共享数据的 namespace 数量，`namespace_item_count` 表示这些 namespace 下共享数据总条数；每个 namespace 可直接看�?`policy_state / readonly / disabled / ttl_required / tag_required / policy_reject_count / readonly_reject_count / disabled_reject_count / ttl_required_reject_count / tag_required_reject_count / last_policy_reject_hit / last_policy_reject_reason / last_policy_reject_action / last_policy_reject_namespace / last_policy_reject_time / last_policy_reject_time_text / last_policy_reject_age_ms / persistent_count / ttl_count / oldest_create_time / oldest_create_time_text / oldest_create_age_ms / newest_create_time / newest_create_time_text / newest_create_age_ms / next_expire_time / next_expire_time_text / next_expire_in_ms / namespace_data_limit / namespace_data_limit_remaining / namespace_data_limit_reached`，且纯规�?namespace 也会单独出现在列表里
- 首页 dashboard 已把 `__xs/bus/namespaces` 渲染�?`Bus Namespace Summary` �?Top 列表，方便直接观察活�?namespace 数、共享数据总数、热�?namespace、配额压力和最近过期窗�?- 首页 Bus 管理区已�?`readonly_namespaces / disabled_namespaces / ttl_required_namespaces / tag_required_namespaces` 在线编辑入口，可直接调用 `__xs/bus/limits` �?namespace 策略

- `_json` 管理/监控接口当前也继续补了直接错误场景的 JSON 保真：`reload_json / reload_config_json / reload_status_json / check_config_json / status_json / health_json / dashboard_json / http_metrics_json / ws_metrics_json / xtp_metrics_json / udp_metrics_json / custom_metrics_json` 现在�?disabled / not found / build failed 这类直接错误分支也会保持 `application/json`；例�?`GET /__xs/reload_json?host=<missing>` 会直接返�?`404` + `{"result":false,"message":"reload host not found"}`，而命�?`disabled.local` 这类禁用管理 host 时，`status_json / dashboard_json / http_metrics_json` 也都会直接返回结构化 `403 JSON`
- 这组本身返回 JSON 的管理接口，当前也已把顶层请求校验一起收平：`__xs/bus/*` 与上�?`_json` 接口命中 `405 / 413 / 414 / 431` 这类 method / body_limit / path_limit / header_limit 拒绝时，也会继续保持 `application/json`；例�?`POST /__xs/bus/status` 会返�?`405 JSON`，`GET /__xs/status_json?<long query>` 会返�?`414 JSON`，�?`header_limit` 超限�?`__xs/status_json` �?`__xs/bus/status` 都会直接返回结构�?`431 JSON`；其�?`reload_json / reload_config_json` 也继续保留原本的 `GET / POST` 语义，真正只读的观测接口继续接受 `GET / HEAD`，�?`__xs/bus/*` 当前统一收成 `GET only`，即使是 `status / registry / namespaces / find / exists / get / values` 这组只读 bus 接口也不再接�?`HEAD`，因为底层读路径可能伴随 `sweep` 或存在性解析，继续放行 `HEAD` 会引入隐藏副作用；method 拒绝当前也会返回各自准确的接口名，不再统一误写�?`status api ...`。同�?`GET /__xs/bus/exists?id=<data_id>` 现在也会按真实注册表状态判断存在性，不再把任意正整数 `id` 误报�?`exists=true`
- `__xs` �?`__xs/*` 当前也继续作为保留管理前缀处理：未知管理路径不再落到脚�?host �?static host；例�?`GET /__xs/bus` �?`GET /__xs/bus/unknown` 会直接返�?`404 JSON`，`GET /__xs/unknown` �?`GET /__xs` 会直接返�?`404 text/plain`，�?`GET /__xs/unknown_json` 这类未知 JSON 风格管理路径也会直接返回结构�?`404 JSON`；这些保留管理路径默认同样附�?`Cache-Control: no-store / X-Frame-Options: DENY / Referrer-Policy: no-referrer`，并统一计入 `http_manage_req_count`；其�?`__xs/bus` �?`__xs/bus/*` 的未知路径也会统一记成 `http_last_reject_reason=bus_not_found`
- 当请�?`Host` 无法命中任何宿主时，这组 JSON 风格接口当前也继续保持结构化错误返回：例�?`GET /__xs/status_json` �?`GET /__xs/bus/status` 在无 `host_default` �?`Host: missing.local` 的场景下，会直接返回 `404` + `{"result":false,"message":"host not found"}`；对应文本接口则仍保�?`text/plain`

### 10.4 force 语义

热加载或重启支持 `force` 参数�?

- `force = TRUE`
	- 立即停止接收新连�?
	- 主动断开现有连接
	- 快速销毁旧对象

- `force = FALSE`
	- 停止接收新连�?
	- 等待现有连接处理完毕
	- 进入 draining 状�?
	- 待连接清空后销毁旧对象

### 10.5 回滚要求

脚本热加载与配置热加载都必须支持失败回滚�?

要求�?

- 新对象未就绪前，旧对象保持可�?
- 编译失败、校验失败、构建失败时恢复到旧对象
- 回滚过程必须保证生命周期调用顺序正确

### 10.6 第一阶段已落地的最小热加载入口

当前 vNext 主线已经提供最�?HTTP 调试入口�?

- `GET /__xs/reload`
- `GET /__xs/reload?host=<主机�?&force=true`

约束�?

- 仅在 `server.debug` �?`host.debug` �?`true` 时启�?
- 当前实现优先保证 Host 级脚本重载可�?
- `force` 参数已进�?API 语义，连接排空与强制切断策略后续继续补完



## 12. 错误模型

XServer vNext 采用分级错误模型�?

- 配置错误
- 宿主错误
- 运行时错�?
- 致命错误

建议处理方式�?

- 配置错误：启动前收集并输�?
- 宿主错误：尽量局部失败，不影响无�?Server
- 运行时错误：记录日志，不默认退出整个进�?
- 致命错误：仅�?Runtime 已不可恢复时退�?



## 13. 日志与诊�?

### 12.1 日志�?

不再全局散落 `printf`，改为轻量日志包装�?

至少分为�?

- `info`
- `warn`
- `error`
- `debug`

### 12.2 调试与发布版�?

构建产物分为�?

- `xs`
- `xsdbg`

其中�?

- `xs` 为生产版
- `xsdbg` 为调试版

`xsdbg` 建议启用�?

- `xrt` 内存调试�?
- 更详细日�?
- 生命周期跟踪
- 热加载诊断信�?
- 更严格断言

第一阶段构建脚本约定�?

- `build.bat` / `build.sh` 输出 `release/xs`
- `build_debug.bat` / `build_debug.sh` 输出 `release/xsdbg`



## 14. 建议的目录结�?

保持�?`main.c` 入口，按功能�?`.h` 文件�?

建议结构�?

```text
xserver/
├── main.c
├── src/
�?  ├── core/
�?  �?  ├── config.h
�?  �?  ├── runtime.h
�?  �?  ├── server.h
�?  �?  ├── host.h
�?  �?  ├── error.h
�?  �?  └── reload.h
�?  ├── protocol/
�?  �?  ├── http.h
�?  �?  ├── ws.h
�?  �?  ├── tcp.h
�?  �?  ├── udp.h
�?  �?  ├── xtp.h
�?  �?  └── custom.h
�?  ├── script/
�?  �?  ├── dynload.h
�?  �?  └── script_api.h
�?  └── support/
�?      ├── log.h
�?      ├── validate.h
�?      └── path.h
├── old/
�?  └── legacy/
└── docs/
```



## 15. 重构策略

本次重构采用“旧代码迁移后，从零搭骨架”的方式�?

原则�?

- 现有代码整体迁入 `old/legacy/`
- 旧代码只做功能参考，不做实现参�?
- 新版本从零开始建立宿主层结构
- 避免历史包袱污染新设�?



## 16. 第一阶段实施顺序

建议按以下顺序推进：

1. 冻结本设计文�?
2. 将旧实现迁入 `old/legacy/`
3. 建立新目录骨�?
4. 重写 `main.c`，只保留 bootstrap 逻辑
5. 先实�?`config / runtime / error / reload`
6. 再迁�?`http / ws / xtp / tcp / udp / custom`
7. 最后回写正式文档与示例配置



## 17. 第一阶段不做的事

以下内容不应在第一阶段投入过深�?

- 旧配置兼容层
- 细碎的性能微调
- 过早扩展 Lua / JavaScript 宿主
- 在未完成宿主层边界前继续叠加新协�?



## 18. 结论

XServer vNext 的核心方向已经明确：

- �?`xrt` 作为坚实基础设施
- �?`xserver` 专注宿主层与业务扩展�?
- �?`Server` 作为统一核心抽象
- 仅在 `http/ws` 中引�?`Host`
- 用严格配置、强生命周期和可回滚热加载进入生产级阶段

后续所有实现都应以本设计草案为准，旧版本行为只作为参考，不作为结构约束�?



## 19. 当前管理面补�?

当前 `__xs/status_json / __xs/dashboard_json / __xs/http_metrics_json` 已包�?HTTP 最近请求上下文�?

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

同时也包含最近一次业�?HTTP 请求上下文：

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



## 20. 交接清单

以下事项截至 `2026-03-22` 仍未最终完成，后续上下文应以此作为继续实现的主清单�?
### 20.1 协议层生产化治理

当前已完成：

- `idle_timeout` 已在 `http / ws / xtp / custom` 上落地并做过真实回归
- `conn_limit` 已在 `http / ws / xtp / custom` 上落地并做过真实回归
- 管理面和首页已经能看�?`idle close / conn limit close` 计数与时�?- `ws_message_limit / xtp_recv_limit / custom_recv_limit` 的关闭计数、最近时间与首页摘要已补进管理面
- `http / ws / xtp / custom` �?`server_stopping` 拒绝链当前也已经打到真实样本，`reject_count / last_reject_reason=server_stopping / last_reject_remote` �?warning 日志能互相对上；管理面已经单独暴�?`http_last_reject_remote / ws_last_reject_remote / xtp_last_reject_remote / custom_last_reject_remote`，避�?reject 现场继续复用易被覆盖的通用 `last_remote`
- `ws / xtp / custom` �?`invalid / error` 现场当前也已经切到专�?remote 快照；`last_invalid_remote / last_error_remote` 会和 `reason / code / time` 一起保留下来，不再被后续正常收包覆�?- `xtp / custom` �?accepted stream 现在�?`accept` 阶段就能看到真实 `remote`，accept/open/close 三条连接生命周期日志已统一

仍需继续补：

- 更统一的限流策�?
- 更系统的异常连接清理（`ws / http / xtp / custom` 停服已统一�?`close -> wait -> abort -> wait` 两段式清理；剩余主要是长�?/ 压测 / 边界回归�?- 更完整的请求拒绝统计
- 各协议更一致的治理规则



### 20.2 配置热加载最终化

当前已完成：

- 全量 reload
- �?`server` reload
- `http / ws` �?host 原位 reload
- 失败回滚

仍需继续补：

- listener / object 级别的最小替换策�?
- 更优的差异化重建流程



### 20.3 Bus / 全局共享数据治理

当前已完成：

- 注册 / 查找 / 更新 / 删除
- retain / release
- ttl
- namespace
- send / broadcast
- `__xs/bus/*` 宿主管理�?- 配额 / 上限
- `sweep / cleanup / sweep_interval_ms`
- `readonly_namespaces / disabled_namespaces / ttl_required_namespaces / tag_required_namespaces`
- `health / dashboard / index.html` 的治理摘要与在线策略编辑

仍需继续补：

- 对新增治理规则做最后一轮跨接口回归归档
- 与最终文档、压测、长稳结论做统一收口



### 20.4 XTP 高层 API 定版

当前已完成：

- `xtp v2 / xtps`
- 同步客户�?
- one-shot call
- request object
- `body / result / error / meta / summary / value / json`

仍需继续补或决定�?

- 最终保留哪�?API
- 是否补异�?pending request
- 是否补持久客户端对象体系



### 20.5 旧业务脚本迁�?

当前已完成：

- `release/script_vnext` demo 主线已较完整

仍需继续补：

- �?`release/script` 业务脚本系统迁移
- `xs_vnext.h / xs_vnext_full.h` 进一步收�?



### 20.6 最终验�?

仍未系统完成�?

- 压测基线
- 长时间稳定性验�?
- `xsdbg` 下的内存调试闭环



### 20.7 文档最终整�?

仍需继续补：

- README 最终交付版整理
- 设计稿与实现状态的最终统一



### 20.8 建议续做顺序

建议下一个上下文按下面顺序继续：

1. 协议层生产化治理
2. XTP 接口定版
3. 配置热加载差异化重建
4. 长稳 / 压测 / 内存调试
5. 文档最终整�?6. Bus 新治理规则最终回归归�?
### 路径语义补充

- 传给 `xs / xs.exe` 的相对配置路径，当前也会优先按可执行文件所在目录解析，避免启动入口继续依赖当前工作目录
- `GET /__xs/check_config?file=<path>` �?`GET /__xs/check_config_json?file=<path>` 传入的相�?`file`，当前也统一按可执行文件所在目录优先解析，保持和启动入口一致的 `config file / config base` 语义
- `GET /__xs/reload_config` �?`GET /__xs/reload_config_json` 当前也复用启动时已经规范化后的配置路径，不会在运行期热重载时重新退回原始相�?argv 的工作目录语�?- TCC 宿主加载 `release/tcc` 运行时资源时也采用可执行文件目录优先策略，因此相�?`devfile` 与相对配置路径在项目根目录和 `release` 目录两种启动方式下都应保持一�?
