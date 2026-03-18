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
    "addr": "http://0.0.0.0:80",
    "tls": false,
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
| addr | string | 绑定地址（例如：`http://0.0.0.0:80`） |
| tls | boolean | 启用 TLS 加密 |
| addr_tls | string | TLS 绑定地址 |
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
- `xsReloadCurrentHost / xsReloadHostByName`
- `xsDataRegister / xsDataGet / xsDataRetain / xsDataRelease / xsDataRemove`
- `xsMsgSendToServer / xsMsgSendToHost`
- `xsAppPath`

如果需要迁移旧脚本里对 `xrt`、`sqlite3` 的直接调用，优先包含 `xs_vnext_full.h`。

当前 `release/script_vnext/main.c` 已经包含可直接访问的示例路由：

- `GET /test`
- `GET /chart/get`
- `GET /template`
- `GET /json`
- `GET /bus/status`
- `GET /bus/send`
- `GET /app/list`
- `POST /app/add`
- `POST /app/edit`
- `POST /app/del`

其中 `/app/*` demo 已经切到 SQLite 预编译语句模式，不再依赖 `xdo`。
`/bus/*` demo 用于演示进程内宿主总线和全局共享数据表，不是通过 HTTP 协议转发消息。

## 可用的回调函数

| 函数 | 说明 |
|------|------|
| `ServiceInit` | 服务启动前调用 |
| `ServiceStart` | 服务开始接流量前调用 |
| `ServiceStop` | 服务停止接流量时调用 |
| `ServiceUnit` | 服务最终销毁前调用 |
| `RequestProc` | HTTP 请求处理器 |
| `MessageProc` | 宿主总线消息处理器 |
| `EventProc` | 网络事件处理器（自定义协议） |

## 调试入口

当 `server.debug` 或 `host.debug` 为 `true` 时，HTTP Host 会启用最小调试入口：

- `GET /__xs/status`
- `GET /__xs/reload`
- `GET /__xs/reload?host=<主机名>&force=true`

当前 `reload` 已支持 Host 级脚本热重载；`force` 参数已进入接口语义，但连接排空策略仍会在后续阶段继续完善。

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
