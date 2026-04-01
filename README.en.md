# XServer

[English](./README.en.md) | [中文](./README.md)

## Overview

**XServer** is a high-performance, multi-protocol server host framework written in C. It uses `xrt` as its infrastructure layer and provides dynamic script loading plus virtual host support for embedded systems, IoT gateways, and edge computing scenarios.

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
| `RequestProc` | HTTP request handler |
| `MessageProc` | Host bus message handler |
| `WsOpenProc / WsTextProc / WsBinaryProc / WsPingProc / WsPongProc / WsCloseProc` | WebSocket host callbacks |
| `EventOpenProc / EventDataProc / EventCloseProc` | Stream callbacks for TCP / Custom / XTP |
| `EventDgramProc` | UDP datagram callback |
| `EventXtpProc` | XTP message callback |

## Debug Endpoints

When `server.debug` or `host.debug` is `true`, HTTP hosts expose minimal debug endpoints:

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
- `GET /__xs/reload?host=<host-name>&force=true`

Host-level script hot reload is available through `xsReloadCurrentHost` and `xsReloadHostByName` in `script-c` hosts.

## Build Variant Surfaces

- `xs` keeps the core manage surface only: `status*`, `health*`, `reload*`, `reload_status*`, `check_config*`
- `xsdbg` additionally enables `dashboard*`, `__xs/bus/*`, `http/ws/xtp/udp/custom *_metrics*`, and the related `*_clear / reload_clear / reload_reset / check_config_clear` helpers
- Runtime counters such as `http_req_count`, `ws_open_count`, `xtp_conn_current`, `udp_recv_count`, `custom_conn_current`, and `bus_queue_count` are retained in `xsdbg`, not in production `xs`

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
