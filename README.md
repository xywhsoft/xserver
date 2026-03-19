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
- `xsXtpSend / xsXtpSendEx / xsXtpReply / xsXtpReplyEx`
- `xsXtpMsgId / xsXtpMsgType / xsXtpMsgFlags / xsXtpStatus`
- `xsXtpCmd / xsXtpCmdLen / xsXtpBody / xsXtpBodyLen`
- `xsXtpParamCount / xsXtpParamKeyAt / xsXtpParamValueAt / xsXtpFindParamView`
- `xsXtpGetParam`（便利函数，内部复制到线程局部缓冲区）
- `xsDgramSendTo / xsDgramReply / xsAddrText`
- `xsReloadCurrentHost / xsReloadHostByName`
- `xsDataRegister / xsDataGet / xsDataRetain / xsDataRelease / xsDataRemove`
- `xsDataRegisterEx`（支持 `namespace / tag / ttl`）
- `xsDataFindFirst`
- `xsBusLastErrorCode / xsBusLastError`
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
- `GET /__xs/ws_metrics`
- `GET /__xs/ws_metrics_json`
- `GET /__xs/ws_metrics_clear`
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
- `GET /__xs/reload_status`
- `GET /__xs/bus/status`
- `GET /__xs/bus/registry`
- `GET /__xs/bus/namespaces`
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
- `GET /__xs/bus/remove?namespace=<命名空间>&tag=<标签>&all=true`

当前 `reload` 已支持 Host 级脚本热重载；`force` 参数已进入接口语义，但连接排空策略仍会在后续阶段继续完善。
当前 `reload_config` 已支持最小配置热加载，并在新配置 `build/init/start` 失败时保留旧服务继续可用。
当前 `reload_config` 已支持按 `server` 定向重载。
对于 `http/ws` 这类 `host-aware` 协议，传入 `host` 时会执行真正的 Host 原位重载；失败时可通过 `reload_status` 查看最近一次异步重载结果。
`__xs/bus/*` 现已作为宿主层总线管理接口落地，可直接查看、查找、读取 payload、按条件批量查看 payload、注册、更新、发送和删除 bus 共享数据，不依赖业务脚本路由。
其中 `__xs/bus/send` 在未提供 `data_id`、但提供了 `text/json/namespace/tag/ttl` 等参数时，会自动注册一条轻量共享数据后再投递；未显式传 `ttl` 时默认使用 `60000ms`。自动注册的数据默认按一次性消息处理，投递完成后会自动释放；只有显式传 `persist=true` 时才会继续保留在注册表里。

`__xs/bus/status` / `__xs/bus/registry` / `__xs/bus/namespaces` 现在还会返回累计统计：
- `total_queued`
- `total_delivered`
- `total_dropped`
- `last_queue_time`
- `last_dispatch_time`
- `last_queue_time_text`
- `last_dispatch_time_text`

`GET /__xs/bus/reset` 可清空上述 bus 累计统计，不影响当前注册表中的共享数据。
首页 `index.html` 也已经切到这些正式管理接口，并补上了配置热加载与结果查询入口；bus 区现在支持显式输入 `data_id`，也支持仅通过 `namespace/tag` 来查找、读取、续期 TTL 和删除共享数据。
`__xs/status_json`、`__xs/health_json`、`__xs/dashboard` / `__xs/dashboard_json` 当前还会返回运行中的 `bind_ip / bind_port / tls / bind_ip_tls / bind_port_tls / addr_tls / ws_protocol / ws_message_limit / ws_conn_current / ws_conn_peak / ws_open_count / ws_close_count / ws_text_count / ws_binary_count / ws_ping_count / ws_pong_count / ws_error_count / tls_cert_file / tls_key_file / tls_ca_file / current_dir / app_file / app_mtime / app_size / app_path / build / compiler / platform / arch / mem_debug / pid / start_time / uptime_ms / engine_workers / runtime_server_count / manage_api / config_name / config_mtime / config_size / http_req_count / http_2xx_count / http_3xx_count / http_4xx_count / http_5xx_count / http_conn_current / http_conn_peak / http_get_count / http_post_count / http_head_count / http_other_count / http_time_total_ms / http_time_max_ms / http_time_avg_ms / http_last_method / http_last_status / http_last_path / http_last_target / http_last_time / http_last_age_ms / http_last_app_method / http_last_app_status / http_last_app_path / http_last_app_target / http_last_app_time / http_last_app_age_ms`，便于排障时直接定位当前进程、工作目录、运行目录、监听地址、TLS 监听、WebSocket 子协议、WebSocket 消息上限、WebSocket 连接与消息计数、TLS 证书路径、当前程序文件与配置文件的更新时间和大小、构建变体、编译器、目标平台架构、内存调试状态、持续运行时长、运行中的服务数量、工作线程规模、HTTP 请求规模、方法分布、当前连接、峰值连接、最近一次任意请求，以及最近一次非 `__xs/*` 业务请求的方法、路径、状态与处理耗时，以及管理面是否启用。

`__xs/reload_status_json`、`__xs/health_json`、`__xs/dashboard_json` 当前还会返回 `reload_total_count / reload_success_count / reload_failure_count`，便于区分最近一次结果和累计重载结果。

`__xs/reload_reset` 已提供配置热加载累计统计清零入口；与 `__xs/reload_clear` 不同，它会清空累计统计而不是只清最近一次结果。

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
