# XServer

[English](./README.en.md) | [中文](./README.md)

## 概述

**XServer** 是一个高性能、多协议网络服务器框架，采用 C 语言编写。具有动态脚本加载能力和多虚拟主机支持，非常适合嵌入式系统、物联网网关和边缘计算场景。

## 核心特性

- 🚀 **多协议支持**: HTTP/HTTPS、MQTT、WebSocket、TCP/UDP 及自定义协议
- 🔄 **动态脚本加载**: 使用 TCC（Tiny C Compiler）实现运行时 C 代码编译
- 🌐 **虚拟主机**: 支持多域名独立配置
- 🔐 **TLS 1.3 支持**: 内置加密传输，支持 SNI
- ⚡ **高性能**: 基于 Mongoose 网络库的轻量级单一可执行文件
- 📝 **JSON 配置**: 通过 JSON 文件轻松配置服务器和主机
- 🗄️ **数据库支持**: 通过 XDO 抽象层支持 SQLite、MySQL、ODBC

## 支持的协议

| 协议 | 说明 |
|------|------|
| HTTP/HTTPS | 支持 TLS 的 Web 服务 |
| MQTT | 物联网消息协议 |
| WebSocket | 实时双向通信 |
| XTP | 自定义传输协议 |
| TCP/UDP | 原始网络协议 |
| Custom | 事件驱动的自定义服务 |
| Thread | 基于独立线程的服务 |

## 系统架构

```
┌─────────────────────────────────────────┐
│         XServer 主进程                   │
├─────────────────────────────────────────┤
│  配置加载器 (xs.json)                    │
├─────────────────────────────────────────┤
│         Mongoose 事件管理器              │
├──────────┬──────────┬──────────┬────────┤
│  HTTP    │  MQTT    │ WebSocket│ Custom │
├──────────┴──────────┴──────────┴────────┤
│       虚拟主机路由器                     │
├─────────────────────────────────────────┤
│   TCC 动态脚本编译器                     │
├─────────────────────────────────────────┤
│  XRT 运行时库 | XDO 数据库层             │
└─────────────────────────────────────────┘
```

## 快速开始

### 1. 编译

**Windows:**
```bash
build.bat
```

**Linux:**
```bash
./build.sh
```

编译命令示例：
```bash
gcc main.c lib/xrt/xrt.c lib/mongoose.c lib/sqlite3.c tcc/libtcc.c \
    -lshlwapi -lgdi32 -lws2_32 -lIPHLPAPI \
    -DMG_TLS=MG_TLS_BUILTIN -O2 -s \
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
      "devfile": "script/main.c",
      "session": "www$"
    },
    "hosts": []
  }
]
```

### 3. 运行

```bash
cd release
xs.exe [配置文件.json]  # 默认使用 xs.json
```

## 配置参考

### 服务器配置

| 字段 | 类型 | 说明 |
|------|------|------|
| enabled | boolean | 是否启用此服务器 |
| class | string | 服务器类型：`http`, `mqtt`, `ws`, `tcp`, `udp`, `custom`, `thread` |
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
| devlang | string | 开发语言：`static`, `c`, `lua`, `js` |
| devfile | string | 脚本入口文件 |
| tls_ca | string | TLS CA 证书路径 |
| tls_cert | string | TLS 证书路径 |
| tls_key | string | TLS 私钥路径 |
| session | string | Session 前缀 |

## 动态脚本开发

XServer 使用 TCC 在运行时编译 C 脚本。创建包含回调函数的脚本文件：

```c
#include <xsbase.h>

// 服务初始化（启动时调用一次）
void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
    printf("服务初始化中...\n");
    // 初始化数据库、加载资源等
}

// HTTP 请求处理器
void RequestProc(XS_ServerObject objServer, XS_HostObject objHost, 
                 struct mg_connection* c, struct mg_http_message* hm)
{
    // 路由匹配
    if (mg_match(hm->uri, mg_str("/api/hello"), NULL)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", 
                     "{\"message\":\"你好世界\"}");
        return;
    }
    
    // 提供静态文件服务
    struct mg_http_serve_opts opts = {.root_dir = objHost->Path};
    mg_http_serve_dir(c, hm, &opts);
}

// 服务清理（关闭时调用）
void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
    printf("服务关闭中...\n");
}
```

## 可用的回调函数

| 函数 | 说明 |
|------|------|
| `ServiceInit` | 服务启动前调用 |
| `ServiceStart` | 服务启动时调用（仅自定义服务） |
| `ServiceUnit` | 服务停止时调用 |
| `RequestProc` | HTTP 请求处理器 |
| `EventProc` | 网络事件处理器（自定义协议） |

## 项目结构

```
xserver/
├── lib/                    # 第三方库和自研库
│   ├── xrt/               # 运行时库（数组、字典、JSON 等）
│   ├── xdo/               # 数据库抽象层
│   ├── mongoose.c/h       # 网络库
│   ├── libtcc.h           # 动态编译器
│   └── sqlite3.c/h        # 数据库引擎
├── src/                    # 协议实现
│   ├── http.h             # HTTP 协议
│   ├── mqtt.h             # MQTT 协议
│   ├── websocket.h        # WebSocket
│   ├── tcp.h, udp.h       # TCP/UDP
│   ├── custom.h           # 自定义服务
│   └── import_c/          # 动态脚本加载器
├── release/                # 发布目录
│   ├── script/            # 业务脚本（动态加载）
│   ├── wwwroot/           # Web 根目录
│   ├── tcc/               # TCC 运行时
│   └── xs.json            # 服务器配置文件
├── tcc/                    # TCC 编译器源码
├── main.c                  # 主程序入口
└── build.bat/sh           # 编译脚本
```

## 应用场景

- **嵌入式 Web 服务器**: 用于嵌入式设备的轻量级 HTTP 服务器
- **物联网网关**: MQTT 代理和协议转换
- **边缘计算**: 网络边缘的微服务节点
- **API 网关**: RESTful API 路由和聚合
- **静态文件服务器**: 高性能静态内容分发
- **实时服务**: 基于 WebSocket 的推送通知

## 技术亮点

1. **热更新**: 通过动态 C 编译实现无需重启的业务逻辑更新
2. **单一可执行文件**: 无外部依赖，易于部署
3. **高性能**: C 语言 + Mongoose 确保出色的并发性能
4. **灵活配置**: JSON 驱动，支持多服务多主机
5. **丰富的协议**: 一套框架支持 8+ 种网络协议
6. **开发友好**: 用 C 语言编写"脚本"，运行时编译

## 依赖项

- **Mongoose**: 嵌入式网络库
- **TCC**: 运行时 C 编译器
- **SQLite3**: 嵌入式数据库
- **XRT**: 自定义运行时库
- **XDO**: 数据库抽象层

## 许可证

请参考项目许可证文件。

## 贡献

欢迎贡献！请随时提交问题和拉取请求。

## 联系方式

如有问题和功能请求，请使用 GitHub Issue Tracker。
