# XServer

[English](./README.en.md) | [中文](./README.md)

## 概述

**XServer** 是一个高性能、多协议网络服务器宿主框架，采用 C 语言编写。它以 `xrt` 为基础设施，提供动态脚本加载能力和多虚拟主机支持，适合嵌入式系统、物联网网关和边缘计算场景。

## 核心特性

- 🚀 **多协议支持**: HTTP/HTTPS、WebSocket、TCP/UDP、XTP 及自定义协议
- 🔄 **动态脚本加载**: 使用 TCC（Tiny C Compiler）实现运行时 C 代码编译
- 🌐 **虚拟主机**: 支持多域名独立配置
- 🔐 **TLS 1.3 支持**: 内置加密传输，支持 SNI
- ⚡ **高性能**: 基于 `xrt` 网络基础设施与宿主层装配
- 📝 **JSON 配置**: 通过 JSON 文件轻松配置服务器和主机

## 支持的协议

| 协议 | 说明 |
|------|------|
| HTTP/HTTPS | 支持 TLS 的 Web 服务 |
| WebSocket | 实时双向通信 |
| XTP | 自定义传输协议 |
| TCP/UDP | 原始网络协议 |
| Custom | 事件驱动的自定义服务 |

## 系统架构

```
┌─────────────────────────────────────────┐
│         XServer 主进程                   │
├─────────────────────────────────────────┤
│  配置加载器 (xs.json)                    │
├─────────────────────────────────────────┤
│      XRT 网络基础设施与事件循环          │
├──────────┬──────────┬──────────┬────────┤
│  HTTP    │   XTP    │ WebSocket│ Custom │
├──────────┴──────────┴──────────┴────────┤
│       虚拟主机路由器                     │
├─────────────────────────────────────────┤
│   TCC 动态脚本编译器                     │
├─────────────────────────────────────────┤
│        XRT 运行时库与宿主支撑层          │
└─────────────────────────────────────────┘
```

## 快速开始

### 1. 编译

**Windows 发布版:**
```bash
build.bat
```

**Windows 调试版:**
```bash
build_debug.bat
```

**Linux 发布版:**
```bash
./build.sh
```

**Linux 调试版:**
```bash
./build_debug.sh
```

当前构建产物：
- `release/xs(.exe)`：发布版
- `release/xsdbg(.exe)`：调试版

发布前最小检查：
- Windows：`cmd /c test_stable.bat`
- Linux：`sh ./test_stable.sh`
- 压测基线（Windows）：`cmd /c test_pressure.bat`
- 压测基线（Linux）：`sh ./test_pressure.sh`
- 长稳窗口（Windows）：`cmd /c test_soak.bat`
- 长稳窗口（Linux）：`sh ./test_soak.sh`
- 内存调试（Windows）：`cmd /c test_memdebug.bat`
- 内存调试（Linux）：`sh ./test_memdebug.sh`
- 说明文档：`docs/稳定API.md`、`docs/legacy对照.md`、`docs/发布检查清单.md`、`docs/运行与稳定补记.md`

发布版命令示例：
```bash
gcc main.c lib/sqlite3.c tcc/libtcc.c \
    -lshlwapi -lgdi32 -lws2_32 -lIPHLPAPI -lbcrypt \
    -O2 -s \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -o release/xs.exe
```

### 2. 配置

编辑 `release/xs.json`：

```json
{
	"services": [
		{
			"enabled": true,
			"class": "http",
			"name": "我的 HTTP 服务器",
			"desc": "主 Web 服务器",
			"ip": "0.0.0.0",
			"port": 80,
			"tls": false,
			"port_tls": 443,
			"host_default": {
				"enabled": true,
				"name": "默认主机",
				"path": "wwwroot",
				"devlang": "c",
				"devfile": "script_vnext/main.c"
			},
			"hosts": []
		}
	]
}
```

### 3. 运行

进入 `release` 目录后直接运行 `xs` 或 `xsdbg`，默认读取 `xs.json`。

如果准备交付 production `xs`，先执行上面的 `test_stable.*`；它会先 build 当前源码，再验证 production `xs` 与 `xsdbg` 的冻结分层是否仍然成立。

## 配置参考

### 服务器配置

| 字段 | 类型 | 说明 |
|------|------|------|
| enabled | boolean | 是否启用此服务器 |
| class | string | 服务器类型：`http`, `ws`, `tcp`, `udp`, `xtp`, `custom` |
| name | string | 服务器名称 |
| desc | string | 服务器描述 |
| ip | string | 监听 IP，例如 `0.0.0.0` |
| port | integer | 监听端口 |
| tls | boolean | 启用 TLS 加密 |
| port_tls | integer | TLS 监听端口，默认复用 `ip` |
| ip_tls | string | 可选的 TLS 监听 IP，仅在需要与 `ip` 不同时使用 |
| path | string | XTP/TCP/UDP/Custom 的资源或脚本根目录 |
| devlang | string | Service 级开发模式：`protocol`, `c`, `script-c` |
| devfile | string | Service 级脚本入口文件 |
| host_default | object | 默认虚拟主机配置 |
| hosts | array | 额外的虚拟主机列表 |

### 主机配置

| 字段 | 类型 | 说明 |
|------|------|------|
| enabled | boolean | 是否启用此主机 |
| name | string | 主机名称 |
| host | string | 域名绑定（多个用分号分隔） |
| path | string | 根目录（相对或绝对路径） |
| devlang | string | Host 开发模式：`static`, `c`, `script-c` |
| devfile | string | 脚本入口文件 |
| tls_ca | string | TLS CA 证书路径 |
| tls_cert | string | TLS 证书路径 |
| tls_key | string | TLS 私钥路径 |

## 动态脚本开发

XServer vNext 使用 TCC 在运行时编译 C 脚本。

- `release/tcc/inc_xs/xs_vnext.h`：最小宿主 API
- `release/tcc/inc_xs/xs_vnext_full.h`：迁移期全量头，额外带入 `xrt / libtcc / sqlite3` 的宿主可用头环境
- 示例脚本：`release/script_vnext/main.c`
- 已迁移的 legacy demo：`release/script/main.c`，可用 `cmd /c test_script_demo.bat` 或 `sh ./test_script_demo.sh` 做最小 smoke
- TCC 宿主当前会优先按 `xs/xs.exe` 所在目录查找 `release/tcc` 运行时资源，因此像 `release/xs_manage_test.json` 这类使用相对 `devfile` 的配置，从项目根目录启动或从 `release` 目录启动都能稳定加载脚本，不再依赖当前工作目录

最小脚本示例：

```c
#include <xs_vnext.h>

// 服务初始化
void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
	printf("init: %s\n", xsHostName(objHost));
}

// HTTP 请求处理
bool RequestProc(XS_ServerObject objServer, XS_HostObject objHost,
                 XS_RequestObject objReq, XS_ResponseObject objResp)
{
	return xsHttpText(objResp, 200, "OK", "hello from vNext") != 0;
}

// 服务清理
void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
	printf("unit: %s\n", xsHostName(objHost));
}
```

当前已提供的迁移期宿主能力包括：

- `xsServerName / xsServerClass / xsServerAddr / xsServerParam / xsServerDebug`
- `xsHostName / xsHostPath / xsHostDevFile / xsHostParam / xsHostDebug / xsHostDevMode`
- `xsReqMethod / xsReqTarget / xsReqPath / xsReqQuery / xsReqHeader`
- `xsHttpStatus / xsHttpHeader / xsHttpText / xsHttpBody / xsHttpJson`
- `xsWsIsOpen / xsWsSendText / xsWsSendBinary / xsWsClose`
- `xsStreamSend / xsStreamClose`
- `xsXtpSend / xsXtpSendRequest / xsXtpSendPush / xsXtpSendEvent / xsXtpSendEx`
- `xsXtpReply / xsXtpReplyEx / xsXtpReplyText / xsXtpReplyJson / xsXtpReplyOKText / xsXtpReplyErrorText / xsXtpReplyOKJson / xsXtpReplyErrorJson / xsXtpReplyMissingParam / xsXtpReplyUnsupportedCmd`
- `xsXtpMsgId / xsXtpMsgType / xsXtpMsgFlags / xsXtpStatus`
- `xsXtpCmd / xsXtpCmdLen / xsXtpBody / xsXtpBodyLen`
- `xsXtpParamCount / xsXtpParamKeyAt / xsXtpParamValueAt / xsXtpFindParamView`
- `xsXtpGetParam / xsXtpParamText / xsXtpResultText / xsXtpErrorText / xsXtpParamDup / xsXtpParamInt / xsXtpParamBool`（便利函数，适合脚本直接读参数和标准错误响应）
- `xsXtpCmdDup / xsXtpBodyDup / xsXtpBodyValue / xsXtpErrorValue / xsXtpParamsValue / xsXtpValue / xsXtpMetaText / xsXtpMetaJson / xsXtpSummaryText / xsXtpSummaryJson / xsXtpResultText / xsXtpResultJson / xsXtpResultIs / xsXtpStatusIs / xsXtpErrorText / xsXtpErrorJson`（安全复制零拷贝视图，并输出统一响应摘要、结构化 `xvalue` 结果对象，以及标准结果/状态/错误文本或 JSON，便于脚本直接处理响应对象）
- `xsXtpIsRequest / xsXtpIsResponse / xsXtpIsPush / xsXtpIsEvent / xsXtpCmdIs / xsXtpHasParam`
- `xsXtpClientOpen / xsXtpClientDo / xsXtpClientDoText / xsXtpClientDoSimple / xsXtpClientDoJson / xsXtpClientCall / xsXtpClientCallText / xsXtpClientCallSimple / xsXtpClientCallJson / xsXtpClientCallSimpleBody / xsXtpClientCallTextBody / xsXtpClientCallJsonBody / xsXtpClientCallSimpleBodyValue / xsXtpClientCallSimpleValue / xsXtpClientCallSimpleParamsValue / xsXtpClientCallSimpleMeta / xsXtpClientCallSimpleMetaJson / xsXtpClientCallSimpleSummary / xsXtpClientCallSimpleSummaryJson / xsXtpClientCallSimpleResult / xsXtpClientCallSimpleResultJson / xsXtpClientCallSimpleError / xsXtpClientCallSimpleErrorJson / xsXtpClientCallSimpleStatus / xsXtpClientCallSimpleCmd / xsXtpClientCallTableText / xsXtpClientCallTableJson / xsXtpClientCallTableValue / xsXtpRequestCreate / xsXtpRequestFree / xsXtpRequestSetCmd / xsXtpRequestSetParamsValue / xsXtpRequestSetParamText / xsXtpRequestSetParamInt / xsXtpRequestSetParamBool / xsXtpRequestSetBodyText / xsXtpRequestSetBodyJson / xsXtpRequestSetBodyValue / xsXtpClientDoRequest / xsXtpClientCallRequest / xsXtpClientCallRequestValue / xsXtpClientCallRequestParamsValue / xsXtpClientCallRequestBodyValue / xsXtpClientCallRequestBody / xsXtpClientCallRequestResult / xsXtpClientCallRequestError / xsXtpClientCallRequestMeta / xsXtpClientCallRequestMetaJson / xsXtpClientCallRequestResultJson / xsXtpClientCallRequestErrorJson / xsXtpClientCallRequestOK / xsXtpClientCallRequestStatus / xsXtpClientCallRequestCmd / xsXtpClientCallRequestSummary / xsXtpClientCallRequestSummaryJson / xsXtpMessageFree / xsXtpClientClose`（第一版同步客户端请求/应答 API，已验证可访问外部 XTP 服务，并支持直接用 `xvalue table` 或 request object 构造请求）
- `xsXtpIsOK`（客户端响应成功判断）
- `xsXtpBodyDup`（按需复制响应 body，避免零拷贝视图直接当 C 字符串使用）
- `xsDgramSendTo / xsDgramReply / xsAddrText`
- `xsReloadCurrentHost / xsReloadHostByName`
- `xsDataRegister / xsDataGet / xsDataRetain / xsDataRelease / xsDataRemove`
- `xsDataRegisterEx`（支持 `namespace / tag / ttl`）
- `xsDataFindFirst`
- `xsBusLastErrorCode / xsBusLastError`

脚本侧 `xsDataRetain / xsDataRelease` 现在也会受 `readonly_namespaces / disabled_namespaces` 约束；命中策略时可通过 `xsBusLastErrorCode / xsBusLastError` 读取失败原因。
- `xsMsgSendToServer / xsMsgSendToHost`
- `xsMsgBroadcast`
- `xsAppPath`

如果需要迁移旧脚本里对 `xrt`、`sqlite3` 的直接调用，优先包含 `xs_vnext_full.h`。

当前 `release/script_vnext/main.c` 已经包含可直接访问的示例路由：

- `GET /test`
- `GET /chart/get`
- `GET /template`
- `GET /json`
- `GET /bus/status`
- `GET /app/list`
- `POST /app/add`
- `POST /app/edit`
- `POST /app/del`

另外提供了独立的 WebSocket demo：

- 配置文件：`release/xs_ws.json`
- TLS 配置文件：`release/xs_wss.json`
- 脚本入口：`release/script_vnext/ws_main.c`
- `ws_protocol` 可用于要求客户端协商指定子协议；当前 demo 使用 `xs-demo`
- `ws_message_limit` 可用于单独限制单条 WebSocket 消息的聚合大小；当前 demo 使用 `262144`
- `idle_timeout` 现已支持 `ws/wss`，示例配置默认使用 `3000ms`
- `conn_limit` 现已支持 `http/ws/wss/custom/tcp/xtp/xtps`，`0` 表示不限制；当活动连接数超过上限时，宿主会主动关闭后续超额连接
- 未带正确子协议的客户端握手会被拒绝，带 `xs-demo` 的客户端可以正常连接
- `wss` demo 使用 `release/tls/xtps_cert.pem` 和 `release/tls/xtps_key.pem`
- `release/wwwroot/ws.html` 现已支持一键切换 `ws://127.0.0.1:8081/` 与 `wss://127.0.0.1:8444/`

另外提供了独立的 UDP demo：

- 配置文件：`release/xs_udp.json`
- 脚本入口：`release/script_vnext/udp_main.c`

另外提供了独立的 XTP demo：

- 配置文件：`release/xs_xtp.json`
- 脚本入口：`release/script_vnext/xtp_main.c`

另外提供了独立的 XTPS demo：

- 配置文件：`release/xs_xtps.json`
- 测试证书：`release/tls/xtps_cert.pem`
- 测试私钥：`release/tls/xtps_key.pem`

当前 `XTP` 在 vNext 中只支持 `v2` 头格式，不再兼容旧版 `xtp\1`。`v2` 固定头为 32 字节，包含：

- `msg_type`
- `msg_id`
- `flags`
- `status`
- `cmd_size`
- `param_count`
- `body_size`

其中：

- `msg_id == 0` 的 `request` 视为单向消息，不要求回复
- `msg_id != 0` 的 `request` 可通过 `xsXtpReply / xsXtpReplyEx` 回包
- `response` 必须回显原始 `msg_id`
- `status` 用于响应状态码

其中 `/app/*` demo 已经切到 SQLite 预编译语句模式，不再依赖 `xdo`。
`/bus/*` demo 用于演示进程内宿主总线和全局共享数据表，不是通过 HTTP 协议转发消息。当前 registry 已支持 `namespace`、`tag` 和 `ttl(ms)`，并支持按 `namespace/tag` 过滤查看、批量清理，以及查看 namespace 聚合统计。`xs.*` 和 `__xs*` 命名空间当前保留给系统使用。注册失败时会返回 `bus_code / bus_error`，便于定位具体错误原因。

## 可用的回调函数

| 函数 | 说明 |
|------|------|
| `ServiceInit / ServiceStart / ServiceStop / ServiceUnit` | Host 或 Service 脚本生命周期 |
| `RequestProc` | HTTP 请求处理器；返回 `true` 表示已处理完成，返回 `false` 时仅 `GET / HEAD` 会继续回退 `host.path` 静态资源，其他方法默认返回 `404` |
| `MessageProc` | 宿主总线消息处理器 |
| `WsOpenProc / WsTextProc / WsBinaryProc / WsPingProc / WsPongProc / WsCloseProc` | WebSocket Host 回调 |
| `EventOpenProc / EventDataProc / EventCloseProc` | TCP / Custom / XTP 的流式回调 |
| `EventDgramProc` | UDP 数据报回调 |
| `EventXtpProc` | XTP 消息回调 |

WebSocket 脚本 API 额外提供：

- `xsWsProtocol`
- `xsWsPing`

## 调试入口

当 `server.debug` 或 `host.debug` 为 `true` 时，HTTP Host 会启用最小调试入口：

- `GET /__xs/status`
- `GET /__xs/status_json`
- `GET /__xs/health`
- `GET /__xs/health_json`
- `GET /__xs/reload`
- `GET /__xs/reload_json`
- `GET /__xs/reload_status`
- `GET /__xs/reload_status_json`
- `GET /__xs/check_config`
- `GET /__xs/check_config_json`

当前生产版 `xs` 的稳定内建管理面，先按 `old/legacy` 的“尽量薄宿主”思路冻结在这组核心入口上：
- `status / status_json`
- `health / health_json`
- `reload / reload_json / reload_config / reload_config_json / reload_status / reload_status_json`
- `check_config / check_config_json`

`dashboard`、`bus`、`http/ws/xtp/udp/custom *_metrics*` 这组扩展管理/观测入口当前视为 `xsdbg` 范围，不作为生产版 `xs` 的稳定 API 承诺。
下文如果继续列出 `dashboard`、`__xs/bus/*`、`*_metrics*` 这组接口的细节，默认都是源码能力清单或 `xsdbg` 视角，不表示生产版 `xs` 默认开放这些入口。

当前这一轮又继续把 `protocol/http.h` 内部的运行时/治理 helper 拆到 `src/manage/http_runtime.h`；配合前一轮已经抽出的 `src/manage/http_manage.h`，`src/protocol/http.h` 现在主要保留协议入口、静态资源与请求回调 glue，用来把 `protocol` 目录继续收回到更接近旧版 `old/legacy` 的职责边界。最近这轮里，原来散落在 `protocol/http.h` 里的大段管理面 if-chain 也已经收成了单一的 `XS_HttpHandleManageRequest(...)` 入口，协议层只再做一次管理面转发，不再自己展开 `reload / status / health / check / bus / dashboard / metrics` 的分发细节。
同一轮里，`protocol/ws.h` 和 `protocol/custom.h` 顶部那批连接跟踪、idle thread、reject/invalid/stop-cleanup 统计，也分别继续抽到了 `src/manage/ws_runtime.h` 和 `src/manage/custom_runtime.h`，让 `protocol` 目录进一步回到“协议回调本身”。
这一轮又把 `protocol/xtp.h` 顶部那批连接跟踪、idle/stop-cleanup、reject/invalid/error remote 快照等运行时治理 helper 继续抽到了 `src/manage/xtp_runtime.h`；`XTP` 的消息结构、同步客户端和高级接口仍然保留在 `protocol/xtp.h`，避免影响你当前最看重的 `XTP` 高层能力。
这轮又把 `main.c` 里那批运行时状态仓库与状态快照 API 继续拆到 `src/manage/runtime_state.h` 和 `src/manage/runtime_state_api.h`；`main.c` 本体已经明显回落到更接近“启动与调度主流程”的形态，后面继续 freeze `xs` 生产边界时，重心会更多落在 `src/manage`，而不是再回到 `protocol`。

当前管理面已经按编译变体分层：

- `build.bat` 产出的 `xs` 只承诺核心入口：`status / status_json`、`health / health_json`、`reload / reload_json / reload_config / reload_config_json / reload_status / reload_status_json`、`check_config / check_config_json`
- `build_debug.bat` 产出的 `xsdbg` 额外开放 `dashboard*`、`__xs/bus/*`、`http/ws/xtp/udp/custom *_metrics*`、`*_clear / reload_clear / reload_reset / check_config_clear`
- 首页 `index.html` 在 `xs` 下会自动降级到核心状态展示，不再主动点击或轮询 `dashboard / bus / *_metrics*` 这组 xsdbg-only 入口

当前字段口径也已经跟着分层收紧：

- production `xs` 的 `__xs/status` / `__xs/status_json` 只保留服务基础信息、绑定信息、限流阈值、构建信息和脚本装载状态，例如 `bind_ip / bind_port / tls / addr_tls / backlog / path_limit / header_limit / body_limit / recv_limit / idle_timeout / conn_limit / ws_message_limit / compiler / platform / arch / build / manage_api / mem_debug / debug / host_count`
- production `xs` 的 `__xs/health` / `__xs/health_json` 只保留健康状态和最近一次 `reload / check_config` 快照，例如 `ok / reload_* / check_* / script_loaded / host_count`；不再包含 bus、HTTP、WebSocket、XTP、UDP、Custom 的运行时统计
- `http_req_count`、`ws_open_count`、`xtp_conn_current`、`udp_recv_count`、`custom_conn_current`、`bus_queue_count` 这类运行时统计字段，只在 `xsdbg` 的 `status_json / health_json / dashboard_json / *_metrics_json` 里保留

当前接口分组可以按下面理解：

- 核心接口：`/__xs/status*`、`/__xs/health*`、`/__xs/reload*`、`/__xs/check_config*`
- xsdbg-only 观测接口：`/__xs/dashboard*`、`/__xs/http_metrics*`、`/__xs/ws_metrics*`、`/__xs/xtp_metrics*`、`/__xs/udp_metrics*`、`/__xs/custom_metrics*`
- xsdbg-only 治理接口：`/__xs/bus/*`、`/__xs/reload_clear`、`/__xs/reload_reset`、`/__xs/check_config_clear`

核心 reload / check 行为当前是：

- `reload` 支持 Host 级脚本热重载，`reload_config` 支持最小配置热加载、按 `server` 定向重载，以及在 listener 关键字段不变时对 `http / ws / tcp / udp / xtp / custom` 做 targeted soft reload
- `reload` / `reload_json` / `reload_config` / `reload_config_json` 在命中同一目标的并发窗口时，会返回 `409 + reload busy`
- `reload` / `reload_json` / `reload_config` / `reload_config_json` 的 `force` 参数已改成严格布尔校验，非法值直接返回 `400`
- JSON 风格接口在 `disabled / not found / build failed / bad request / method not allowed` 这类直接错误分支里，仍保持 `application/json`

`__xs/bus/*` 当前只在 `xsdbg` 中提供。它负责宿主层共享数据和总线治理，包括：

- `status / registry / namespaces / limits / sweep / reset`
- `find / exists / get / values / retain / release / touch / register / set / send / remove`
- namespace 策略：`readonly_namespaces / disabled_namespaces / ttl_required_namespaces / tag_required_namespaces`

如果要看当前冻结契约、生产版 `xs` 与 `xsdbg` 的 `200 / 403 / 405` 边界，以及首页降级规则，直接看：

- `docs/稳定API.md`
- `docs/发布检查清单.md`

当前网络服务配置还支持：

- `backlog`
- `recv_limit`
- `path_limit`
- `header_limit`
- `body_limit`

其中：

- `backlog` 用于监听队列长度
- `recv_limit` 用于单连接接收缓冲上限
- `path_limit` 用于限制 HTTP 请求 `path + query` 的总长度，且不能超过 `xrt` 当前的固定解析上限
- `header_limit` 用于限制 HTTP 请求头数量，且不能超过 `xrt` 当前的固定上限
- `body_limit` 用于限制 HTTP 请求体大小，且必须小于等于 `recv_limit`
- `static` Host 默认只允许 `GET / HEAD`，其他方法返回 `405`
- `static` Host 默认拒绝点文件和反斜杠路径
- `script-c` Host 未导出 `RequestProc` 时会直接按 `host.path` 走静态资源；导出后若返回 `false`，仅 `GET / HEAD` 会继续静态回退，路径规范化、防目录穿越和 MIME 推断统一由 `xs` 处理
- HTTP 响应默认附带 `X-Content-Type-Options: nosniff`，`/__xs/*` 管理接口默认附带 `Cache-Control: no-store`

## 项目结构

```
xserver/
├── lib/                    # 第三方库和自研库
│   ├── xrt.h              # XRT 单头文件
│   ├── libtcc.h           # 动态编译器
│   └── sqlite3.c/h        # 数据库引擎
├── src/                    # 协议实现
│   ├── core/              # 核心对象与生命周期
│   ├── protocol/          # 协议装配层
│   ├── script/            # TCC 脚本宿主
│   └── support/           # 支撑工具层
├── release/                # 发布目录
│   ├── script/            # 旧业务脚本参考
│   ├── script_vnext/      # vNext 示例脚本
│   ├── wwwroot/           # Web 根目录
│   ├── tcc/               # TCC 运行时
│   └── xs.json            # 服务器配置文件
├── tcc/                    # TCC 编译器源码
├── main.c                  # 主程序入口
└── build.bat/sh           # 编译脚本
```

## 应用场景

- **嵌入式 Web 服务器**: 用于嵌入式设备的轻量级 HTTP 服务器
- **边缘计算**: 网络边缘的微服务节点
- **API 网关**: RESTful API 路由和聚合
- **静态文件服务器**: 高性能静态内容分发
- **实时服务**: 基于 WebSocket 的推送通知
- **私有协议服务**: 基于 XTP 或 TCP 的二进制协议服务

## 技术亮点

1. **热更新**: 当前已支持 Host 级 C 脚本热重载
2. **单一可执行文件**: 无外部依赖，易于部署
3. **高性能**: C 语言 + XRT 网络设施确保出色的并发性能
4. **灵活配置**: JSON 驱动，支持多服务和 Host-aware 协议
5. **丰富的协议**: 一套框架支持 HTTP、WS、TCP、UDP、XTP 与自定义协议
6. **开发友好**: 用 C 语言编写"脚本"，运行时编译

## 当前管理面摘要

- production `xs`
- `__xs/status_json` 只保留服务基础信息、绑定信息、治理阈值、构建信息和脚本装载状态
- `__xs/health_json` 只保留 `ok`、脚本装载状态，以及最近一次 `reload / check_config` 快照
- 不再返回 `http_last_* / ws_* / xtp_* / udp_* / custom_* / bus_*` 这类运行时统计

- `xsdbg`
- `__xs/status_json / __xs/health_json / __xs/dashboard_json` 会继续带运行时统计和最近一次请求上下文
- `http_last_* / http_last_app_* / http_app_req_count / http_manage_req_count` 这类字段只在 `xsdbg` 保留
- `__xs/http_metrics_json / __xs/ws_metrics_json / __xs/xtp_metrics_json / __xs/udp_metrics_json / __xs/custom_metrics_json` 也只在 `xsdbg` 提供

## 未完成任务

以下事项截至 `2026-04-01` 仍未最终完成，后续应优先按此清单继续：

1. 协议层生产化治理继续收口
- 当前 `idle_timeout / conn_limit` 已在 `http / ws / xtp / custom` 上完成真实回归。
- `ws_message_limit / xtp_recv_limit / custom_recv_limit` 的关闭计数、最近时间与首页摘要已补进管理面。
- `http / ws / xtp / custom` 的 `server_stopping` 拒绝链当前也已经打到真实样本，`reject_count / last_reject_reason=server_stopping / last_reject_remote` 与 warning 已经对齐；相关管理接口现在也会单独暴露 `http_last_reject_remote / ws_last_reject_remote / xtp_last_reject_remote / custom_last_reject_remote`，不再依赖会被后续正常流量覆盖的通用 `last_remote`。
- `ws / xtp / custom` 的 `invalid / error` 现场当前也已经切到专用 remote 快照；`last_invalid_remote / last_error_remote` 会和 `reason / code / time` 一起保留，不再被后续正常收包或正常请求覆盖。
- `xtp / custom` 的 accepted stream 现在会在 `accept` 阶段就带上真实 `remote`，不再出现 `accept=(none)`、`open=127.0.0.1` 这类观测断层。
- `ws / xtp / custom` 的拒绝统计当前也已经按 `server_stopping / conn_limit / message_limit|recv_limit / invalid / other` 拆分到专属 metrics、`*_clear` 与 dashboard 摘要，回归脚本也已覆盖这些字段。
- 仍需继续补：
	- 更统一的限流策略
- 更系统的异常连接清理（`ws / http / xtp / custom` 停服已统一为 `close -> wait -> abort -> wait` 两段式清理；剩余主要是长稳 / 压测 / 边界回归）
	- 各协议更一致的治理规则

2. 配置热加载做最终差异化重建
- 当前已支持：
	- 全量 reload
	- 按 `server` reload
	- `http / ws` 的 host 原位 reload
	- listener 关键字段不变时，对 `http / ws / tcp / udp / xtp / custom` 做 targeted server soft reload
	- 失败回滚
- 当前 `udp` 已经把 `recv_limit / backlog` 这类非 socket 关键参数从整服务重建里剥掉，改成对象级同步。
- 仍需继续补更细粒度的 listener / object 级别最小替换，尤其是 host 拓扑变更、listener 增删和 TLS listener 差异化重建。

3. Bus / 全局共享数据机制做最终收口
- 当前已完成：
	- 注册、查找、更新、retain/release、ttl、namespace、send/broadcast
	- `__xs/bus/*` 宿主管理面
	- `data_limit / queue_limit / namespace_limit / namespace_data_limit`
	- `sweep / cleanup / sweep_interval_ms`
	- `readonly_namespaces / disabled_namespaces / ttl_required_namespaces / tag_required_namespaces`
	- `limits -> register/set -> limits/namespaces` 这条新增治理规则跨接口 smoke 回归
	- `health / dashboard / index.html` 的 bus 治理摘要与在线编辑入口
- 仍需继续补：
	- 与最终文档、压测、长稳结论做统一收口

4. XTP 高层 API 最终定版
- 当前已完成：
	- `xtp v2 / xtps`
	- 同步客户端
	- one-shot call
	- request object
	- `body / result / error / meta / summary / value / json`
- 仍需继续补或决定：
	- 最终保留哪些 API
	- 是否补异步 pending request
	- 是否补持久客户端对象体系

5. 旧业务脚本迁移继续推进
- `release/script_vnext` 的 demo 主线已较完整。
- 仓库内 `release/script` 这批 legacy demo 已迁到 vNext 宿主，并补了 `test_script_demo.bat/sh` smoke。
- 剩余迁移工作主要落在真实业务项目或未入库旧脚本，不再是仓库内这份 demo。

6. 长稳 / 压测 / 内存调试验证
- 当前已完成：
	- 压测基线：`tools/xs_pressure_baseline.ps1/.sh` 与 `test_pressure.bat/sh`
	- 长稳窗口：`tools/xs_soak_check.ps1/.sh` 与 `test_soak.bat/sh`
	- Windows `SIGBREAK` 停服链路已经接到 `main.c`，`CTRL_BREAK` 现在会进入正常停服与 `xrtUnit()`，不再直接把 `xsdbg` 截断在自动报告之前
	- `tools/xs_memdebug_check.ps1/.sh` 与 `test_memdebug.bat/sh` 已补齐“临时目录运行 + 自动报告 + 不写脏仓库跟踪 mem report”的收口脚本
	- `test_memdebug.*` 的 accepted threshold 已按当前脚本 workload 定版：HTTP `foreign_live_count<=900`、WS `<=840`、XTP `<=840`、Custom `<=840`，并已在 Windows 入口下完成实测

7. 文档最终发布版整理
- README 和设计稿已持续同步。
- 但仍需最后做一轮统一整理，收成正式交付版本。

## 建议续做顺序

建议下一个上下文优先按下面顺序继续：

1. 协议层生产化治理
2. XTP 接口定版
3. 配置热加载差异化重建
4. 旧业务脚本迁移
5. 文档最终整理
6. Bus 新治理规则最终回归归档

## 依赖项

- **TCC**: 运行时 C 编译器
- **SQLite3**: 嵌入式数据库
- **XRT**: 自定义运行时库

## 许可证

请参考项目许可证文件。

## 贡献

欢迎贡献！请随时提交问题和拉取请求。

## 联系方式

如有问题和功能请求，请使用 GitHub Issue Tracker。

## 运行与稳定基线说明

当前 release 线的核心结论可以直接看下面这组文档，不需要再从长段实现记录里反推：

- `docs/稳定API.md`：production `xs` 与 `xsdbg` 的冻结边界
- `docs/legacy对照.md`：为什么 production `xs` 需要继续保持“薄宿主”
- `docs/发布检查清单.md`：准备交付时最短的发布检查入口
- `docs/运行与稳定补记.md`：保留背景信息和历史补记

当前稳定验证入口仍然是：

- Windows：`cmd /c test_stable.bat`
- Linux：`sh ./test_stable.sh`

这套基线会先 build 当前源码，再确认：

- production `xs` 继续保留 `status / health / reload / check_config`
- production `xs` 继续把 `dashboard / __xs/bus/* / *_metrics* / clear-reset` 维持在 `403`
- `xsdbg` 继续保留扩展调试面
- `/json`、动态重载、仓库根目录启动、XTP 高级接口、WS/custom demo 回路都没有回退

另外，当前也已经补了单独的压测基线入口：

- Windows：`cmd /c test_pressure.bat`
- Linux：`sh ./test_pressure.sh`

这套基线会用 `xsdbg` 顺序拉起 HTTP、WS、XTP、Custom demo，清零 `*_metrics` 后发送固定轮次请求，并输出每类协议的请求数、耗时、RPS 与宿主计数器摘要，便于后续做窗口化压测对比。

另外，当前也已经补了长稳窗口入口：

- Windows：`cmd /c test_soak.bat`
- Linux：`sh ./test_soak.sh`

这套基线会在单次运行里顺序拉起 HTTP、WS、XTP、Custom demo，并对每类协议执行多轮固定批次请求，验证窗口内服务存活、计数器增长和连接回收。

`xsdbg` 的内存调试入口也已经补到：

- Windows：`cmd /c test_memdebug.bat`
- Linux：`sh ./test_memdebug.sh`

当前这套脚本会把 `xsdbg` 放到临时工作目录运行，避免把仓库根目录或 `release/` 下跟踪的 `xrt_mem_report_auto.*` 写脏；Windows 路径还通过 `SIGBREAK` + `CTRL_BREAK` helper 进入正常停服链路，确保自动 mem report 能在停服后真实落盘。
当前 accepted threshold 也已经按这套脚本的真实 workload 冻结在：

- HTTP：`foreign_live_count <= 900`
- WS：`foreign_live_count <= 840`
- XTP：`foreign_live_count <= 840`
- Custom：`foreign_live_count <= 840`

另外，当前首页也已经按这套分层自动降级：production `xs` 下保留核心状态展示，但不会把 xsdbg-only 的调试入口继续暴露成可点击链接。

## 运行路径补记（历史补记，已整理到 `docs/运行与稳定补记.md`）

这部分历史补记已经迁移到：

- `docs/运行与稳定补记.md`

主阅读路径仍然以：

- `docs/稳定API.md`
- `docs/legacy对照.md`
- `docs/发布检查清单.md`
- `test_stable.bat`
- `test_stable.sh`

为准。

