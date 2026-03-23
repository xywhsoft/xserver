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
[
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
```

### 3. 运行

进入 `release` 目录后直接运行 `xs` 或 `xsdbg`，默认读取 `xs.json`。

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
| host_default | object | 默认虚拟主机配置 |
| hosts | array | 额外的虚拟主机列表 |

### 主机配置

| 字段 | 类型 | 说明 |
|------|------|------|
| enabled | boolean | 是否启用此主机 |
| name | string | 主机名称 |
| host | string | 域名绑定（多个用分号分隔） |
| path | string | 根目录（相对或绝对路径） |
| devlang | string | 开发模式：`static`, `c`, `script-c`, `protocol` |
| devfile | string | 脚本入口文件 |
| tls_ca | string | TLS CA 证书路径 |
| tls_cert | string | TLS 证书路径 |
| tls_key | string | TLS 私钥路径 |

## 动态脚本开发

XServer vNext 使用 TCC 在运行时编译 C 脚本。

- `release/tcc/inc_xs/xs_vnext.h`：最小宿主 API
- `release/tcc/inc_xs/xs_vnext_full.h`：迁移期全量头，额外带入 `xrt / libtcc / sqlite3` 的宿主可用头环境
- 示例脚本：`release/script_vnext/main.c`
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
| `ServiceInit` | 服务启动前调用 |
| `ServiceStart` | 服务开始接流量前调用 |
| `ServiceStop` | 服务停止接流量时调用 |
| `ServiceUnit` | 服务最终销毁前调用 |
| `RequestProc` | HTTP 请求处理器 |
| `WsOpenProc` | WebSocket 连接建立时调用 |
| `WsTextProc` | WebSocket 文本消息处理器 |
| `WsBinaryProc` | WebSocket 二进制消息处理器 |
| `WsPingProc` | WebSocket 收到 Ping 时调用 |
| `WsPongProc` | WebSocket 收到 Pong 时调用 |
| `WsCloseProc` | WebSocket 连接关闭时调用 |

WebSocket 脚本 API 额外提供：

- `xsWsProtocol`
- `xsWsPing`
| `MessageProc` | 宿主总线消息处理器 |
| `EventProc` | 网络事件处理器（自定义协议） |
| `EventXtpProc` | XTP 消息处理器 |

## 调试入口

当 `server.debug` 或 `host.debug` 为 `true` 时，HTTP Host 会启用最小调试入口：

- `GET /__xs/status`
- `GET /__xs/status_json`
- `GET /__xs/health`
- `GET /__xs/health_json`
- `GET /__xs/http_metrics`
- `GET /__xs/http_metrics_json`
- `GET /__xs/http_metrics_clear`

`http` 服务器现在也支持 `idle_timeout` 配置，超时空闲连接会被宿主主动关闭；`__xs/http_metrics`、`__xs/http_metrics_json` 会额外返回 `http_idle_close_count / http_last_idle_close_time / http_last_idle_close_age_ms`，用于区分空闲清理与正常业务请求。

`http/ws/wss/custom/tcp/xtp/xtps` 现在也支持 `conn_limit` 配置，超过上限时宿主会主动关闭超额连接；当前 `__xs/status / __xs/status_json / __xs/dashboard / __xs/dashboard_json` 已会返回 `conn_limit`，便于直接核对治理配置是否生效。与此同时，协议级 metrics 也开始区分 `conn_limit` 关闭现场：`__xs/http_metrics_json` 返回 `http_conn_limit_close_count / http_last_conn_limit_close_time / http_last_conn_limit_close_age_ms`，`__xs/ws_metrics_json` 返回 `ws_conn_limit_close_count / ws_last_conn_limit_close_time / ws_last_conn_limit_close_age_ms`，`__xs/xtp_metrics_json` 返回 `xtp_conn_limit_close_count / xtp_last_conn_limit_close_time / xtp_last_conn_limit_close_age_ms`，`__xs/custom_metrics_json` 返回 `custom_conn_limit_close_count / custom_last_conn_limit_close_time / custom_last_conn_limit_close_age_ms`。

- `GET /__xs/ws_metrics`
- `GET /__xs/ws_metrics_json`
- `GET /__xs/ws_metrics_clear`

`ws/wss` 服务器现在也支持 `idle_timeout` 配置，超时空闲连接会被宿主主动关闭；`__xs/ws_metrics`、`__xs/ws_metrics_json` 会额外返回 `ws_idle_close_count / ws_last_idle_close_time / ws_last_idle_close_age_ms`，用于区分空闲清理与真实协议错误。
- `GET /__xs/xtp_metrics`
- `GET /__xs/xtp_metrics_json`
- `GET /__xs/xtp_metrics_clear`

`xtp/xtps` 服务器现在支持 `idle_timeout` 配置，超时空闲连接会被宿主主动关闭；`__xs/xtp_metrics`、`__xs/xtp_metrics_json` 会额外返回 `xtp_idle_close_count / xtp_last_idle_close_time / xtp_last_idle_close_age_ms`，用于区分空闲清理与真实传输错误。
- `GET /__xs/udp_metrics`
- `GET /__xs/udp_metrics_json`
- `GET /__xs/udp_metrics_clear`
- `GET /__xs/custom_metrics`
- `GET /__xs/custom_metrics_json`
- `GET /__xs/custom_metrics_clear`

`custom/tcp` 服务器同样支持 `idle_timeout` 配置；`__xs/custom_metrics`、`__xs/custom_metrics_json` 会额外返回 `custom_idle_close_count / custom_last_idle_close_time / custom_last_idle_close_age_ms`，用于观察空闲清理行为。
- `GET /__xs/dashboard`
- `GET /__xs/dashboard_json`
- `GET /__xs/check_config`
- `GET /__xs/check_config_json`
- `GET /__xs/check_config_clear`
- `GET /__xs/reload_status`
- `GET /__xs/reload_status_json`
- `GET /__xs/reload_clear`
- `GET /__xs/reload_reset`
- `GET /__xs/reload`
- `GET /__xs/reload_json`
- `GET /__xs/reload?host=<主机名>&force=true`
- `GET /__xs/reload_config`
- `GET /__xs/reload_config_json`
- `GET /__xs/reload_config?server=<服务名>&host=<主机名>`
- `GET /__xs/bus/status`
- `GET /__xs/bus/registry`
- `GET /__xs/bus/namespaces`
- `GET /__xs/bus/sweep`
- `GET /__xs/bus/sweep?all=true&max_pass=<count>`
- `GET /__xs/bus/limits?data_limit=<count>&queue_limit=<count>&namespace_limit=<count>&namespace_data_limit=<count>&sweep_interval_ms=<ms>`
- `GET /__xs/bus/limits?readonly_namespaces=<a,b,tenant.*>&disabled_namespaces=<c,d,queue.*>&ttl_required_namespaces=<cache.*,session.*>&tag_required_namespaces=<order.*,event.*>`
- `GET /__xs/bus/reset`
- `GET /__xs/bus/find?namespace=<命名空间>&tag=<标签>`
- `GET /__xs/bus/exists?id=<数据ID>`
- `GET /__xs/bus/exists?namespace=<命名空间>&tag=<标签>`
- `GET /__xs/bus/get?id=<数据ID>`
- `GET /__xs/bus/get?namespace=<命名空间>&tag=<标签>`
- `GET /__xs/bus/values?namespace=<命名空间>&tag=<标签>`
- `GET /__xs/bus/retain?id=<数据ID>`
- `GET /__xs/bus/retain?namespace=<命名空间>&tag=<标签>`
- `GET /__xs/bus/release?id=<数据ID>`
- `GET /__xs/bus/release?namespace=<命名空间>&tag=<标签>`
- `GET /__xs/bus/touch?id=<数据ID>&ttl=<毫秒>`
- `GET /__xs/bus/touch?namespace=<命名空间>&tag=<标签>&ttl=<毫秒>`
- `GET /__xs/bus/register?namespace=<命名空间>&tag=<标签>&ttl=<毫秒>`
- `GET /__xs/bus/set?id=<数据ID>&text=<文本>` / `json=<JSON>` / `ttl=<毫秒>`
- `GET /__xs/bus/set?namespace=<命名空间>&tag=<标签>&text=<文本>` / `json=<JSON>` / `ttl=<毫秒>`
- `GET /__xs/bus/send?target=<server|host|broadcast>&topic=<主题>`
- `GET /__xs/bus/send?target=<server|host|broadcast>&topic=<主题>&data_id=<数据ID>`
- `GET /__xs/bus/remove?id=<数据ID>`
- `GET /__xs/bus/remove?all=true`
- `GET /__xs/bus/remove?namespace=<命名空间>&tag=<标签>&all=true`

当前 `reload` 已支持 Host 级脚本热重载；`force` 参数已进入接口语义，但连接排空策略仍会在后续阶段继续完善。
当前 `reload_config` 已支持最小配置热加载，并在新配置 `build/init/start` 失败时保留旧服务继续可用。
当前 `reload_config` 已支持按 `server` 定向重载。
当前同一 `server/host` 上的并发 `reload` / `reload_json`，以及定向到同一 Host 的 `reload_config` / `reload_config_json` 热重载，也已共用宿主级互斥门闩；直接重入请求会返回 `409 + reload busy`，而异步目标 Host reload 会计入 reload 失败统计并在日志里记录 `target host reload busy`，避免脚本状态被并发替换。
对于 `http/ws` 这类 `host-aware` 协议，传入 `host` 时会执行真正的 Host 原位重载；失败时可通过 `reload_status` 查看最近一次异步重载结果。
`__xs/bus/*` 现已作为宿主层总线管理接口落地，可直接查看、查找、读取 payload、按条件批量查看 payload、注册、更新、发送和删除 bus 共享数据，不依赖业务脚本路由。
其中 `__xs/bus/send` 在未提供 `data_id`、但提供了 `text/json/namespace/tag/ttl` 等参数时，会自动注册一条轻量共享数据后再投递；未显式传 `ttl` 时默认使用 `60000ms`。自动注册的数据默认按一次性消息处理，投递完成后会自动释放；只有显式传 `persist=true` 时才会继续保留在注册表里。
当前 `__xs/bus/*` 管理接口在传入非法或保留 namespace 时，也统一复用同一条 `400 bad request` 响应链；`register/send` 已与 `find/get/set/remove` 等接口保持一致，都会直接返回 `{"result":false,"message":"invalid namespace",...}`。
`__xs/bus/send` 当前只接受 `target=server|host|broadcast`；传入其他值时会直接返回 `400` + `{"result":false,"message":"invalid target",...}`，不再静默回退成默认 `server` 投递。
当前 `__xs/bus/exists / get / retain / release / touch / set / remove / send(data_id)` 在传入非法 `id/data_id` 时，也都会统一返回 `400` + `{"result":false,"message":"invalid data id","data_id":0}`，不再把非数字值静默当成 `0`。
当前 `__xs/bus/touch / register / set / send(ttl)` 在传入非法 `ttl` 时，也都会统一返回 `400` + `{"result":false,"message":"invalid ttl","ttl":0}`；其中 `touch` 的数值约束不变，传入 `ttl=0` 这类非正整数时仍会返回 `ttl must be greater than 0`。
当前 `__xs/bus/*` 这组本身返回 JSON 的管理接口，在 `api disabled / build failed / stringify failed` 这类直接错误分支里也继续保持 `application/json` 口径；例如命中 `disabled.local` 这类禁用管理 host 时，`bus/status / bus/namespaces / bus/send` 会直接返回结构化 `403 JSON`，不再回退成 `text/plain`。
当前 `__xs/bus/sweep?all=true&max_pass=<count>` 与 `__xs/bus/limits` 也已切到严格十进制参数校验：传入 `abc` 这类非法文本时会统一返回 `400` + `{"result":false,"message":"invalid <field>",...}`，不再静默折叠成 `0`；`max_pass<=0`、`data_limit<0` 这类范围约束与 namespace rule 校验当前也继续保留原 message 语义，例如仍使用 `max_pass must be > 0`、`data_limit must be >= 0`、`namespace must not contain empty segment`，但外层响应现已统一为 `application/json`。

`__xs/bus/status` / `__xs/bus/registry` / `__xs/bus/namespaces` 现在还会返回累计统计：
- `total_queued`
- `total_delivered`
- `total_dropped`
- `last_queue_time`
- `last_dispatch_time`
- `last_queue_time_text`
- `last_dispatch_time_text`

`__xs/bus/remove?all=<bool>`、`__xs/bus/sweep?all=<bool>&drain=<bool>`、`__xs/bus/send?persist=<bool>` 这组布尔参数当前也已切到严格校验；像 `all=maybe`、`drain=maybe`、`persist=maybe` 这类非法值会直接返回 `400` + `{"result":false,"message":"invalid <field>"}`，不再静默按 `false` 处理。

`GET /__xs/bus/reset` 可清空上述 bus 累计统计，不影响当前注册表中的共享数据；当前会直接返回 reset 后的 `bus/status` 同口径 JSON 快照，但不会因为这次返回而额外触发一次 host 侧 sweep。
`GET /__xs/bus/remove?all=true` 现支持直接批量删除当前 registry 中的共享数据；如同时提供 `namespace/tag`，则只删除匹配条件的数据，并会持续删除直到当前匹配集合清空，而不是停在单批 `256` 条。
`GET /__xs/bus/sweep` 可手动触发一次过期数据清理，并直接返回当前 `data/queue/limit/sweep/cleanup` 摘要，便于压测、巡检和排障时立即确认清理结果。
`GET /__xs/bus/sweep?all=true` 可在单次管理调用里持续 sweep 到过期积压尽量清空，`max_pass` 可限制本次最多循环次数，便于一次性处理超过单批 `256` 条的过期数据积压。
`GET /__xs/bus/limits` 当前也支持在线调整 `sweep_interval_ms`，便于控制宿主层自动 sweep 节奏；设大后可降低后台清理频率，配合 `all=true` 适合做积压排查与强制清场。
`GET /__xs/bus/namespaces` 当前也会返回顶层 `namespace_count / namespace_limit_remaining / namespace_limit_reached / namespace_item_count / readonly_namespace_count / disabled_namespace_count / ttl_required_namespace_count / tag_required_namespace_count`，以及每个 namespace 的 `policy_state / readonly / disabled / ttl_required / tag_required / policy_reject_count / readonly_reject_count / disabled_reject_count / ttl_required_reject_count / tag_required_reject_count / last_policy_reject_hit / last_policy_reject_reason / last_policy_reject_action / last_policy_reject_namespace / last_policy_reject_time / last_policy_reject_time_text / last_policy_reject_age_ms / persistent_count / ttl_count / oldest_create_time / oldest_create_time_text / oldest_create_age_ms / newest_create_time / newest_create_time_text / newest_create_age_ms / next_expire_time / next_expire_time_text / next_expire_in_ms / namespace_data_limit / namespace_data_limit_remaining / namespace_data_limit_reached`；其中 `namespace_count` 表示当前有活跃共享数据的 namespace 数量，`namespace_item_count` 表示这些 namespace 下共享数据总条数；即使某个 namespace 只是被配置进 `readonly_namespaces / disabled_namespaces / ttl_required_namespaces / tag_required_namespaces`、当前没有活跃数据，也会出现在列表里，便于直接观察 namespace 压力、策略状态、累计命中次数、最近一次拒绝现场与下一次过期窗口。

首页 `index.html` 现在也会把 `__xs/bus/namespaces` 渲染成 `Bus Namespace Summary` 与 Top 列表，直接显示活跃 namespace 数、共享数据总数、上限状态、热点 namespace 与最近过期窗口，排查时不再只靠原始 JSON。
首页 `index.html` 的 Bus 区现在也提供 `readonly_namespaces / disabled_namespaces / ttl_required_namespaces / tag_required_namespaces` 输入框与 `更新 Policy` 按钮，可直接调用 `__xs/bus/limits` 在线改 namespace 策略；规则同时支持精确 namespace 与 `team.*` 这类前缀规则。
`__xs/bus/limits` 现在也支持 `readonly_namespaces / disabled_namespaces / ttl_required_namespaces / tag_required_namespaces` 四组 namespace 级宿主管理规则：前两者分别用于只读和禁用治理，`ttl_required_namespaces` 会要求命中的 namespace 共享数据最终必须带 TTL，拒绝 `register` 或会把对象继续保留为持久数据的 `set/update` 路径，但允许通过 `touch` 或 `set(ttl>0)` 给旧持久数据补上 TTL；`tag_required_namespaces` 会要求命中的 namespace 共享数据必须带 tag，并拒绝无 tag 的 `register/set/touch`；规则项既可写精确 namespace，也可写 `tenant.*` 这类前缀规则，命中时统一返回 `409`，并带 `bus_code=18(namespace readonly)`、`bus_code=17(namespace disabled)`、`bus_code=19(namespace ttl required)` 或 `bus_code=20(namespace tag required)`；相关 reject 统计会同步进入 `bus/status / bus/limits / bus/namespaces / health / dashboard`。
首页 `index.html` 也已经切到这些正式管理接口，并补上了配置热加载与结果查询入口；bus 区现在支持显式输入 `data_id`，也支持仅通过 `namespace/tag` 来查找、读取、续期 TTL 和删除共享数据。
`__xs/ws_metrics`、`__xs/ws_metrics_json` 当前还会额外返回 `ws_last_error_code / ws_last_close_reason / ws_last_close_time / ws_last_close_age_ms / ws_idle_close_count / ws_conn_limit_close_count / ws_message_limit_close_count / ws_last_idle_close_time / ws_last_idle_close_age_ms / ws_last_conn_limit_close_time / ws_last_conn_limit_close_age_ms / ws_last_message_limit_close_time / ws_last_message_limit_close_age_ms / ws_last_frame_type / ws_last_remote / ws_last_bytes / ws_last_text / ws_last_time / ws_last_age_ms / ws_last_error_time / ws_last_error_age_ms`，便于直接看到最近一次 WebSocket 文本、二进制或 ping/pong 事件的类型、对端地址、内容摘要、最近一次关闭原因与时间、空闲清理/连接上限/消息上限关闭现场以及最近一次错误现场与时间上下文。

`__xs/status_json`、`__xs/health_json`、`__xs/dashboard` / `__xs/dashboard_json` 当前还会返回运行中的 `bind_ip / bind_port / tls / bind_ip_tls / bind_port_tls / addr_tls / ws_protocol / ws_message_limit / ws_conn_current / ws_conn_peak / ws_open_count / ws_close_count / ws_text_count / ws_binary_count / ws_ping_count / ws_pong_count / ws_error_count / ws_invalid_count / ws_idle_close_count / ws_conn_limit_close_count / ws_message_limit_close_count / ws_last_error_code / ws_last_close_reason / ws_last_frame_type / ws_last_remote / ws_last_bytes / ws_last_text / ws_last_time / ws_last_age_ms / ws_last_error_time / ws_last_error_age_ms / tls_cert_file / tls_key_file / tls_ca_file / current_dir / app_file / app_mtime / app_size / app_path / build / compiler / platform / arch / mem_debug / pid / start_time / uptime_ms / engine_workers / runtime_server_count / manage_api / config_name / config_mtime / config_size / http_req_count / http_2xx_count / http_3xx_count / http_4xx_count / http_5xx_count / http_conn_current / http_conn_peak / http_get_count / http_post_count / http_head_count / http_other_count / http_time_total_ms / http_time_max_ms / http_time_avg_ms / http_last_method / http_last_status / http_last_path / http_last_target / http_last_remote / http_last_time / http_last_age_ms / http_last_app_method / http_last_app_status / http_last_app_path / http_last_app_target / http_last_app_remote / http_last_app_time / http_last_app_age_ms`，便于排障时直接定位当前进程、工作目录、运行目录、监听地址、TLS 监听、WebSocket 子协议、WebSocket 消息上限、WebSocket 连接与消息计数、最近一次 WebSocket 帧上下文、最近一次 WebSocket 对端地址、最近一次 WebSocket 关闭原因、最近一次 WebSocket 错误现场，以及空闲/连接上限/消息上限关闭计数，TLS 证书路径、当前程序文件与配置文件的更新时间和大小、构建变体、编译器、目标平台架构、内存调试状态、持续运行时长、运行中的服务数量、工作线程规模、HTTP 请求规模、方法分布、当前连接、峰值连接、最近一次任意请求，以及最近一次非 `__xs/*` 业务请求的方法、路径、对端地址、状态与处理耗时，以及管理面是否启用。

`__xs/status_json`、`__xs/dashboard_json` 当前还会额外返回 `xtp_conn_current / xtp_conn_peak / xtp_open_count / xtp_close_count / xtp_error_count / xtp_invalid_count / xtp_msg_count / xtp_req_count / xtp_resp_count / xtp_push_count / xtp_event_count / xtp_send_count / xtp_recv_bytes / xtp_send_bytes / xtp_last_msg_type / xtp_last_status / xtp_last_msg_id / xtp_last_flags / xtp_last_param_count / xtp_last_body_size / xtp_last_remote / xtp_last_bytes / xtp_last_cmd / xtp_last_time / xtp_last_age_ms / xtp_last_invalid_reason / xtp_last_invalid_time / xtp_last_invalid_age_ms / xtp_last_error_code / xtp_last_error_time / xtp_last_error_age_ms / xtp_idle_close_count / xtp_conn_limit_close_count / xtp_recv_limit_close_count / xtp_last_idle_close_time / xtp_last_idle_close_age_ms / xtp_last_conn_limit_close_time / xtp_last_conn_limit_close_age_ms / xtp_last_recv_limit_close_time / xtp_last_recv_limit_close_age_ms`，便于观察 `xtp / xtps` 的连接、坏包数量、消息类型分布、最近一包的 flags、参数数量、body 大小、对端地址、长度与上下文、最近一次坏包原因、最近一次系统错误，以及空闲/连接上限/收包上限关闭计数与收发字节统计。

`__xs/status_json`、`__xs/dashboard_json` 当前还会额外返回 `udp_recv_count / udp_send_count / udp_error_count / udp_last_error_code / udp_recv_bytes / udp_send_bytes / udp_last_from / udp_last_text / udp_last_bytes / udp_last_time / udp_last_age_ms / udp_last_error_time / udp_last_error_age_ms`，便于观察 `udp` 的收发包数、字节数、最近一包长度、最近一包上下文以及最近一次错误现场。

`__xs/custom_metrics`、`__xs/custom_metrics_json` 当前还会额外返回 `custom_invalid_count / custom_last_invalid_reason / custom_last_invalid_time / custom_last_invalid_age_ms / custom_last_close_reason / custom_last_error_code / custom_last_error_time / custom_last_error_age_ms / custom_idle_close_count / custom_conn_limit_close_count / custom_recv_limit_close_count / custom_last_idle_close_time / custom_last_idle_close_age_ms / custom_last_conn_limit_close_time / custom_last_conn_limit_close_age_ms / custom_last_recv_limit_close_time / custom_last_recv_limit_close_age_ms`，便于区分脚本层坏包原因、传输层错误、空闲/连接上限/收包上限关闭现场以及最近一次断开上下文。

`__xs/status_json`、`__xs/dashboard_json` 当前还会额外返回 `custom_conn_current / custom_conn_peak / custom_open_count / custom_close_count / custom_error_count / custom_invalid_count / custom_last_invalid_reason / custom_last_invalid_time / custom_last_invalid_age_ms / custom_last_close_reason / custom_last_error_code / custom_last_error_time / custom_last_error_age_ms / custom_recv_count / custom_send_count / custom_recv_bytes / custom_send_bytes / custom_last_remote / custom_last_bytes / custom_last_text / custom_last_time / custom_last_age_ms`，便于观察 `custom / tcp` 的连接、收发、错误、最近一次对端地址、最近一次无效输入原因、最近一次断开原因和最近一次收包上下文。

`__xs/dashboard` 文本版现在也会直接输出 `ws / xtp / udp / custom` 的关键运行态字段，便于终端排障时不切 JSON 也能看到协议侧的连接、收发、最近一包与错误现场。

`__xs/reload_status_json`、`__xs/health_json`、`__xs/dashboard_json` 当前还会返回 `reload_total_count / reload_success_count / reload_failure_count`，便于区分最近一次结果和累计重载结果。

`__xs/reload_reset` 已提供配置热加载累计统计清零入口；与 `__xs/reload_clear` 不同，它会清空累计统计而不是只清最近一次结果。新一轮 `reload_config` 排队当前也不会再把这组累计计数提前清零。

当前 `__xs/reload_config` / `__xs/reload_config_json` 的排队门闩也已经改成原子状态流转；并发管理请求命中同一个 reload 窗口时，后续请求会稳定返回 `busy`，不会再覆盖已经挂起的 target server / host。

当前 `__xs/reload_config` / `__xs/reload_config_json` 在传入 `host=<主机名>`，且目标 `server` 就是当前服务时，也会先做同步 Host 存在性校验；像 `GET /__xs/reload_config_json?host=<missing>` 这类明显无效目标现在会直接返回 `404 + reload host not found`，不再先排队再在 `reload_status` 里迟到失败。

当前 `__xs/reload_clear` / `__xs/reload_reset` 在 reload 仍处于 `busy` 时，也只会清结果或清累计统计，不会把正在进行的 reload 伪装成空闲；`busy` 与当前 target server / host 会继续保留到本轮 reload 完成。

当前 `__xs/reload` / `__xs/reload_json` / `__xs/reload_config` / `__xs/reload_config_json` 的 `force` 参数也已切到严格布尔校验；像 `force=maybe` 这类非法值会直接返回 `400 invalid force`，不再静默回退成 `false`。

当前 reload 状态读写也已统一走同一份受保护快照；`__xs/reload_status` / `__xs/reload_status_json`、`__xs/health` / `__xs/health_json`、`__xs/dashboard` / `__xs/dashboard_json` 在并发 `reload_json / reload_clear / reload_reset` 压力下，也不会再在单次响应里混出半新半旧的 `busy / has_result / server / host / message / reload_*count` 组合。

当前 `__xs/reload` / `__xs/reload_json`、`__xs/reload_config` / `__xs/reload_config_json`、`__xs/reload_status` / `__xs/reload_status_json`、`__xs/check_config` / `__xs/check_config_json`、`__xs/status` / `__xs/status_json`、`__xs/health` / `__xs/health_json`、`__xs/dashboard` / `__xs/dashboard_json` 以及 `__xs/http_metrics` / `__xs/ws_metrics` / `__xs/xtp_metrics` / `__xs/udp_metrics` / `__xs/custom_metrics` 与各自 `_json` 版本，当前都已经按同一套字段口径收平；其中 `*_clear` / `reload_clear` / `reload_reset` / `check_config_clear` 的文本响应也已对齐对应主接口的键集，便于直接做脚本化 diff 和窗口化运维观察。
这组 `_json` 管理/监控接口当前也继续补了直接错误场景的 JSON 保真：`reload_json / reload_config_json / reload_status_json / check_config_json / status_json / health_json / dashboard_json / http_metrics_json / ws_metrics_json / xtp_metrics_json / udp_metrics_json / custom_metrics_json` 现在在 disabled / not found / build failed 这类直接错误分支也会保持 `application/json`；例如 `GET /__xs/reload_json?host=<missing>` 会返回 `404` + `{"result":false,"message":"reload host not found"}`，而命中 `disabled.local` 这类禁用管理 host 时，`status_json / dashboard_json / http_metrics_json` 也都会直接返回结构化 `403 JSON`。
这组本身返回 JSON 的管理接口，当前也已把顶层请求校验一起收平：`__xs/bus/*` 与上述 `_json` 接口命中 `405 / 413 / 414 / 431` 这类 method / body_limit / path_limit / header_limit 拒绝时，也会继续保持 `application/json`；例如 `POST /__xs/bus/status` 会返回 `405 JSON`，`GET /__xs/status_json?<long query>` 会返回 `414 JSON`，而 `header_limit` 超限时 `__xs/status_json` 与 `__xs/bus/status` 都会直接返回结构化 `431 JSON`。其中 `reload_json / reload_config_json` 继续保留原本的 `GET / POST` 语义，`POST` 不会再被顶层校验误拦；真正只读的观测接口如 `status / health / dashboard / *_metrics / reload_status` 继续接受 `GET / HEAD`，而 `__xs/bus/*` 当前统一收成 `GET only`，即使是 `status / registry / namespaces / find / exists / get / values` 这组只读 bus 接口也不再接受 `HEAD`，因为底层读路径可能伴随 `sweep` 或存在性解析，继续放行 `HEAD` 会引入隐藏副作用。对应 method 拒绝当前也会返回各自准确的接口名，不再统一误写成 `status api ...`。同时 `GET /__xs/bus/exists?id=<data_id>` 现在也会按真实注册表状态判断存在性，不再把任意正整数 `id` 误报成 `exists=true`。
`/__xs` 与 `/__xs/*` 当前也继续作为保留管理前缀处理：如果路径没有命中任何内置管理接口，不会再落到脚本 host 或 static host；例如 `GET /__xs/bus` 与 `GET /__xs/bus/unknown` 会直接返回 `404 JSON`，`GET /__xs/unknown` 与 `GET /__xs` 会返回 `404 text/plain`，而像 `GET /__xs/unknown_json` 这类未知 JSON 风格管理路径也会直接返回结构化 `404 JSON`。这些保留管理路径默认同样附带 `Cache-Control: no-store / X-Frame-Options: DENY / Referrer-Policy: no-referrer`，并统一计入 `http_manage_req_count`，不再误落到业务请求统计；其中 `__xs/bus` 与 `__xs/bus/*` 的未知路径也会统一记成 `http_last_reject_reason=bus_not_found`。
当请求 `Host` 无法命中任何宿主时，这组 JSON 风格接口当前也继续保持结构化错误返回：例如 `GET /__xs/status_json` 或 `GET /__xs/bus/status` 在无 `host_default` 且 `Host: missing.local` 的场景下，会直接返回 `404` + `{"result":false,"message":"host not found"}`；对应文本接口则仍保持 `text/plain`。

`__xs/check_config_json`、`__xs/dashboard_json` 当前还会返回 `check_total_count / check_success_count / check_failure_count / check_last_time / check_last_age_ms`，用于观察在线配置校验的累计调用、最近一次校验时间以及距离上次校验过去多久。

`__xs/health` 文本接口当前也会输出 `reload_time / reload_age_ms / check_total_count / check_success_count / check_failure_count / check_last_time / check_last_age_ms / bus_queue_count / bus_data_count / bus_total_queued / bus_total_delivered / bus_total_dropped / bus_last_queue_time / bus_last_queue_time_text / bus_last_dispatch_time / bus_last_dispatch_time_text`，便于文本探针直接观察热加载、配置校验与总线运行态。

`__xs/health_json` 当前也会返回：
- `check_total_count`
- `check_success_count`
- `check_failure_count`
- `check_last_time`
- `check_last_age_ms`
- `bus_total_queued`
- `bus_total_delivered`
- `bus_total_dropped`
- `bus_last_queue_time`
- `bus_last_queue_time_text`
- `bus_last_dispatch_time`
- `bus_last_dispatch_time_text`
- `http_last_method`
- `http_last_status`
- `http_last_path`
- `http_last_target`
- `http_last_version`
- `http_last_time`
- `http_last_age_ms`

首页当前已经把这些配置校验与总线时间信息提升为状态卡，包含：
- `Reload Last`
- `Reload Age`
- `Reload Total`
- `Reload Success`
- `Reload Failure`
- `Check Last`
- `Check Age`
- `Check Total`
- `Check Success`
- `Check Failure`
- `HTTP Last`
- `HTTP Last Age`
- `HTTP Last Path`
- `Bus Queue Time`
- `Bus Dispatch Time`

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
- `static` Host 默认拒绝点文件、反斜杠路径，以及 `.c/.h/.json/.db/.sqlite/.pem/.key/.log/.bak` 等敏感文件扩展名
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

- `__xs/status_json` / `__xs/dashboard_json` 当前已包含最近一次 HTTP 请求与最近一次业务 HTTP 请求的完整上下文：
- `http_last_method / status / path / target / version / remote / time / age_ms`
- `http_last_app_method / status / path / target / version / remote / time / age_ms`
- `http_app_req_count / http_manage_req_count`
- `http_last_duration_ms / http_last_app_duration_ms`
- `http_last_body_len / http_last_app_body_len`
- `http_last_content_type / http_last_app_content_type`
- `http_last_header_count / http_last_app_header_count`
- `http_last_query_len / http_last_app_query_len`
- `http_last_host / http_last_app_host`
- `http_last_user_agent / http_last_app_user_agent`
- `http_last_referer / http_last_app_referer`
- `http_last_origin / http_last_app_origin`
- `http_last_accept / http_last_app_accept`
- `http_last_accept_encoding / http_last_app_accept_encoding`
- `http_last_cookie / http_last_app_cookie`
- `http_last_forwarded_for / http_last_app_forwarded_for`
- `http_last_real_ip / http_last_app_real_ip`
- `http_last_connection / http_last_app_connection`
- `http_last_cache_control / http_last_app_cache_control`

## 未完成任务

以下事项截至 `2026-03-22` 仍未最终完成，后续应优先按此清单继续：

1. 协议层生产化治理继续收口
- 当前 `idle_timeout / conn_limit` 已在 `http / ws / xtp / custom` 上完成真实回归。
- `ws_message_limit / xtp_recv_limit / custom_recv_limit` 的关闭计数、最近时间与首页摘要已补进管理面。
- 仍需继续补：
	- 更统一的限流策略
	- 更系统的异常连接清理
	- 更完整的请求拒绝统计
	- 各协议更一致的治理规则

2. 配置热加载做最终差异化重建
- 当前已支持：
	- 全量 reload
	- 按 `server` reload
	- `http / ws` 的 host 原位 reload
	- 失败回滚
- 仍需继续补 listener / object 级别的最小替换策略，减少不必要重建。

3. Bus / 全局共享数据机制做最终收口
- 当前已完成：
	- 注册、查找、更新、retain/release、ttl、namespace、send/broadcast
	- `__xs/bus/*` 宿主管理面
	- `data_limit / queue_limit / namespace_limit / namespace_data_limit`
	- `sweep / cleanup / sweep_interval_ms`
	- `readonly_namespaces / disabled_namespaces / ttl_required_namespaces / tag_required_namespaces`
	- `health / dashboard / index.html` 的 bus 治理摘要与在线编辑入口
- 仍需继续补：
	- 对新增治理规则做最后一轮跨接口回归归档
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
- 旧 `release/script` 那批业务脚本还未系统迁完。

6. 长稳 / 压测 / 内存调试验证
- 仍未系统完成：
	- 压测基线
	- 长时间稳定性验证
	- `xsdbg` 下的内存调试闭环

7. 文档最终发布版整理
- README 和设计稿已持续同步。
- 但仍需最后做一轮统一整理，收成正式交付版本。

## 建议续做顺序

建议下一个上下文优先按下面顺序继续：

1. 协议层生产化治理
2. XTP 接口定版
3. 配置热加载差异化重建
4. 长稳 / 压测 / 内存调试
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

## Runtime Path Notes

Current relative config arguments passed to `xs / xs.exe` are resolved against the executable directory first. For example, running `release/xs.exe xs_manage_test.json` from the project root now correctly loads `release/xs_manage_test.json` instead of depending on the current working directory.

The same executable-directory-first rule now also applies to runtime config management. `GET /__xs/check_config?file=<path>` and `GET /__xs/check_config_json?file=<path>` normalize relative `file` arguments against `xs / xs.exe`, and `GET /__xs/reload_config` / `GET /__xs/reload_config_json` reuse the already-normalized startup config path. Startup loading, runtime config checking, config reload, and the exposed `config file / config base` view now stay aligned.

The bundled TCC runtime resources under `release/tcc` follow the same rule.
