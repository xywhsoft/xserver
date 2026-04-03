# XServer

[English](./README.en.md) | [中文](./README.md)

## Overview

**XServer** is a lightweight, production-stable multi-protocol network framework and script host written in C. It uses `xrt` as its infrastructure layer and provides dynamic script loading plus virtual host support for embedded systems, IoT gateways, edge nodes, game servers, and agent services.

Its role is not to be a built-in management application. Its role is to provide a portable, reloadable, stable network foundation for application-layer projects.

## Key Features

- 🚀 **Multi-Protocol Support**: HTTP/HTTPS, WebSocket, TCP/UDP, XTP, and custom protocols
- 🔄 **Dynamic Script Loading**: Runtime C code compilation using TCC (Tiny C Compiler)
- 🌐 **Virtual Hosts**: Support for multiple domains with independent configurations
- 🔐 **TLS 1.3 Support**: Built-in encryption with SNI support
- ⚡ **High Performance**: Built on `xrt` network infrastructure and host-side protocol assembly
- 📝 **JSON Configuration**: Easy server and host configuration via JSON files

## Supported Protocols

| Protocol | Description |
|----------|-------------|
| HTTP/HTTPS | Web services with TLS support |
| WebSocket | Real-time bidirectional communication |
| XTP | Custom transport protocol |
| TCP/UDP | Raw network protocols |
| Custom | Event-driven custom services |

## Architecture

```
┌─────────────────────────────────────────┐
│         XServer Main Process            │
├─────────────────────────────────────────┤
│  Configuration Loader (xs.json)         │
├─────────────────────────────────────────┤
│      XRT Network Infrastructure         │
├──────────┬──────────┬──────────┬────────┤
│  HTTP    │   XTP    │  WebSocket│ Custom │
├──────────┴──────────┴──────────┴────────┤
│       Virtual Host Router               │
├─────────────────────────────────────────┤
│   TCC Dynamic Script Compiler           │
├─────────────────────────────────────────┤
│      XRT Runtime and Host Support       │
└─────────────────────────────────────────┘
```

## Quick Start

### 1. Build

**Windows Release:**
```bash
build.bat
```

**Windows Debug:**
```bash
build_debug.bat
```

**Linux Release:**
```bash
./build.sh
```

**Linux Debug:**
```bash
./build_debug.sh
```

Current build outputs:
- `release/xs(.exe)`: release build
- `release/xsdbg(.exe)`: debug build

Release build command:
```bash
gcc main.c lib/sqlite3.c tcc/libtcc.c \
    -lshlwapi -lgdi32 -lws2_32 -lIPHLPAPI -lbcrypt \
    -O2 -s \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -o release/xs.exe
```

### 2. Configure

Edit `release/xs.json`:

```json
{
	"services": [
		{
			"enabled": true,
			"class": "http",
			"name": "My HTTP Server",
			"desc": "Main web server",
			"ip": "0.0.0.0",
			"port": 80,
			"tls": false,
			"host_default": {
				"enabled": true,
				"name": "Default Host",
				"path": "wwwroot",
				"devlang": "c",
				"devfile": "script_vnext/main.c"
			},
			"hosts": []
		}
	]
}
```

### 3. Run

Run `xs` or `xsdbg` inside `release`; both use `xs.json` by default.

## Configuration Reference

### Server Configuration

| Field | Type | Description |
|-------|------|-------------|
| enabled | boolean | Enable/disable this server |
| class | string | Server type: `http`, `ws`, `tcp`, `udp`, `xtp`, `custom` |
| name | string | Server name |
| desc | string | Server description |
| ip | string | Bind IP address |
| port | integer | Bind port |
| tls | boolean | Enable TLS encryption |
| ip_tls | string | TLS bind IP address |
| port_tls | integer | TLS bind port |
| path | string | Service-level root path for XTP/TCP/UDP/Custom scripts or assets |
| devlang | string | Service-level development mode: `protocol`, `c`, `script-c` |
| devfile | string | Service-level script entry file |
| host_default | object | Default virtual host configuration |
| hosts | array | Additional virtual hosts |

### Host Configuration

| Field | Type | Description |
|-------|------|-------------|
| enabled | boolean | Enable/disable this host |
| name | string | Host name |
| host | string | Domain binding (semicolon-separated) |
| path | string | Root directory (relative or absolute) |
| devlang | string | Host development mode: `static`, `c`, `script-c` |
| devfile | string | Script entry file |
| tls_ca | string | TLS CA certificate path |
| tls_cert | string | TLS certificate path |
| tls_key | string | TLS private key path |

## Dynamic Script Development

XServer vNext currently uses `release/tcc/inc_xs/xs_vnext.h` as its minimal script host header, and `release/script_vnext/main.c` as the main demo script.

The migrated legacy demo also lives in `release/script/main.c`, and can be smoke-tested with `cmd /c test_script_demo.bat` or `sh ./test_script_demo.sh`.

Minimal script example:

```c
#include <xs_vnext.h>

void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
	printf("init: %s\n", xsHostName(objHost));
}

bool RequestProc(XS_ServerObject objServer, XS_HostObject objHost,
                 XS_RequestObject objReq, XS_ResponseObject objResp)
{
	return xsHttpText(objResp, 200, "OK", "hello from vNext") != 0;
}

void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
	printf("unit: %s\n", xsHostName(objHost));
}
```

## Available Callback Functions

| Function | Description |
|----------|-------------|
| `ServiceInit / ServiceStart / ServiceStop / ServiceUnit` | Host or service script lifecycle |
| `RequestProc` | HTTP request handler; returning `true` marks the request handled, while returning `false` only falls back to `host.path` static files for `GET / HEAD`, and other methods default to `404` |
| `MessageProc` | Host bus message handler |
| `WsOpenProc / WsTextProc / WsBinaryProc / WsPingProc / WsPongProc / WsCloseProc` | WebSocket host callbacks |
| `EventOpenProc / EventDataProc / EventCloseProc` | Stream callbacks for TCP / Custom / XTP |
| `EventDgramProc` | UDP datagram callback |
| `EventXtpProc` | XTP message callback |

## Build Boundaries

The current boundary is intentionally narrow:

- production `xs` no longer ships builtin `__xs/*` routes and no longer reserves `__xs/*`
- `Bus / reload / check_config` remain runtime APIs for C code, script code, or app-defined routes
- `xsdbg` keeps a minimal builtin debug app for protocol inspection, error diagnosis, memory debugging, and reload/check diagnosis

Current builtin `xsdbg` endpoints:

- `__xs/status*`
- `__xs/health*`
- `__xs/reload*`
- `__xs/check_config*`
- `__xs/*_metrics*`
- `__xs/*_clear`

At the same time:

- `__xs/dashboard*` is no longer builtin in any build variant
- `__xs/bus/*` is no longer builtin in any build variant
- if these paths exist, they belong to the application layer
- because there is no builtin Bus HTTP manage surface anymore, `http_metrics / http_metrics_json` no longer expose `http_bus_*` debug fields in xsdbg
- `status / health / http_metrics / http_metrics_json` also no longer expose builtin control-surface reject stats such as `api_disabled / reload_busy / check_config_failed / host_not_found / method`; they now stay focused on protocol and stability diagnostics
- the builtin `status* / health*` Bus section is now reduced to a minimal runtime summary and no longer exposes namespace policies, limits, or governance lists
- the legacy dashboard source has been removed from the mainline and archived in [dev/2026-04-03_xsdbg_dashboard_code_trim_backup](/D:/git/xserver/dev/2026-04-03_xsdbg_dashboard_code_trim_backup)

Host-level script hot reload is still available through `xsReloadCurrentHost` and `xsReloadHostByName` in `script-c` hosts.

## Project Structure

```
xserver/
├── lib/                    # Libraries
│   ├── xrt.h              # XRT single-header runtime
│   ├── libtcc.h           # Dynamic compiler
│   └── sqlite3.c/h        # Database engine
├── src/                    # Protocol implementations
│   ├── core/              # Core objects and lifecycle
│   ├── protocol/          # Protocol adapter layer
│   ├── script/            # TCC script host
│   └── support/           # Support utilities
├── release/                # Distribution directory
│   ├── script/            # Legacy business script reference
│   ├── script_vnext/      # vNext demo scripts
│   ├── wwwroot/           # Web root
│   ├── tcc/               # TCC runtime
│   └── xs.json            # Server configuration
├── tcc/                    # TCC compiler source
├── main.c                  # Main entry point
└── build.bat/sh           # Build scripts
```

## Use Cases

- **Embedded Web Servers**: Lightweight HTTP server for embedded devices
- **Edge Computing**: Microservice nodes at the network edge
- **API Gateway**: RESTful API routing and aggregation
- **Static File Server**: High-performance static content delivery
- **Real-time Services**: WebSocket-based push notifications
- **Private Protocol Services**: Binary services built on XTP or TCP

## Technical Highlights

1. **Hot Reload**: Host-level C script hot reload is already available
2. **Single Binary**: No external dependencies, easy deployment
3. **High Performance**: C + XRT network facilities ensure strong concurrency
4. **Flexible Configuration**: JSON-driven, supports multiple services and host-aware protocols
5. **Rich Protocols**: One framework supports HTTP, WS, TCP, UDP, XTP, and custom protocols
6. **Developer Friendly**: Write "scripts" in C with runtime compilation

## Dependencies

- **TCC**: Tiny C Compiler for runtime compilation
- **SQLite3**: Embedded database
- **XRT**: Custom runtime library

## License

Please refer to the project license file.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## Contact

For issues and feature requests, please use the GitHub Issue Tracker.
