# XServer

[English](./README.en.md) | [中文](./README.md)

## Overview

**XServer** is a high-performance, multi-protocol network server framework written in C. It features dynamic script loading capabilities and supports multiple virtual hosts, making it ideal for embedded systems, IoT gateways, and edge computing scenarios.

## Key Features

- 🚀 **Multi-Protocol Support**: HTTP/HTTPS, MQTT, WebSocket, TCP/UDP, and custom protocols
- 🔄 **Dynamic Script Loading**: Runtime C code compilation using TCC (Tiny C Compiler)
- 🌐 **Virtual Hosts**: Support for multiple domains with independent configurations
- 🔐 **TLS 1.3 Support**: Built-in encryption with SNI support
- ⚡ **High Performance**: Lightweight single executable with Mongoose network library
- 📝 **JSON Configuration**: Easy server and host configuration via JSON files
- 🗄️ **Database Support**: SQLite, MySQL, ODBC via XDO abstraction layer

## Supported Protocols

| Protocol | Description |
|----------|-------------|
| HTTP/HTTPS | Web services with TLS support |
| MQTT | IoT messaging protocol |
| WebSocket | Real-time bidirectional communication |
| XTP | Custom transport protocol |
| TCP/UDP | Raw network protocols |
| Custom | Event-driven custom services |
| Thread | Independent thread-based services |

## Architecture

```
┌─────────────────────────────────────────┐
│         XServer Main Process            │
├─────────────────────────────────────────┤
│  Configuration Loader (xs.json)         │
├─────────────────────────────────────────┤
│         Mongoose Event Manager          │
├──────────┬──────────┬──────────┬────────┤
│  HTTP    │  MQTT    │  WebSocket│ Custom │
├──────────┴──────────┴──────────┴────────┤
│       Virtual Host Router               │
├─────────────────────────────────────────┤
│   TCC Dynamic Script Compiler           │
├─────────────────────────────────────────┤
│  XRT Runtime Library | XDO Database     │
└─────────────────────────────────────────┘
```

## Quick Start

### 1. Build

**Windows:**
```bash
build.bat
```

**Linux:**
```bash
./build.sh
```

The build command compiles:
```bash
gcc main.c lib/xrt/xrt.c lib/mongoose.c lib/sqlite3.c tcc/libtcc.c \
    -lshlwapi -lgdi32 -lws2_32 -lIPHLPAPI \
    -DMG_TLS=MG_TLS_BUILTIN -O2 -s \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -o release/xs.exe
```

### 2. Configure

Edit `release/xs.json`:

```json
[
  {
    "enabled": true,
    "class": "http",
    "name": "My HTTP Server",
    "desc": "Main web server",
    "addr": "http://0.0.0.0:80",
    "tls": false,
    "host_default": {
      "enabled": true,
      "name": "Default Host",
      "path": "wwwroot",
      "devlang": "c",
      "devfile": "script/main.c",
      "session": "www$"
    },
    "hosts": []
  }
]
```

### 3. Run

```bash
cd release
xs.exe [config_file.json]  # Uses xs.json by default
```

## Configuration Reference

### Server Configuration

| Field | Type | Description |
|-------|------|-------------|
| enabled | boolean | Enable/disable this server |
| class | string | Server type: `http`, `mqtt`, `ws`, `tcp`, `udp`, `custom`, `thread` |
| name | string | Server name |
| desc | string | Server description |
| addr | string | Bind address (e.g., `http://0.0.0.0:80`) |
| tls | boolean | Enable TLS encryption |
| addr_tls | string | TLS bind address |
| host_default | object | Default virtual host configuration |
| hosts | array | Additional virtual hosts |

### Host Configuration

| Field | Type | Description |
|-------|------|-------------|
| enabled | boolean | Enable/disable this host |
| name | string | Host name |
| host | string | Domain binding (semicolon-separated) |
| path | string | Root directory (relative or absolute) |
| devlang | string | Development language: `static`, `c`, `lua`, `js` |
| devfile | string | Script entry file |
| tls_ca | string | TLS CA certificate path |
| tls_cert | string | TLS certificate path |
| tls_key | string | TLS private key path |
| session | string | Session prefix |

## Dynamic Script Development

XServer uses TCC to compile C scripts at runtime. Create a script file with callback functions:

```c
#include <xsbase.h>

// Service initialization (called once at startup)
void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
    printf("Service initializing...\n");
    // Initialize databases, load resources, etc.
}

// HTTP request handler
void RequestProc(XS_ServerObject objServer, XS_HostObject objHost, 
                 struct mg_connection* c, struct mg_http_message* hm)
{
    // Route matching
    if (mg_match(hm->uri, mg_str("/api/hello"), NULL)) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", 
                     "{\"message\":\"Hello World\"}");
        return;
    }
    
    // Serve static files
    struct mg_http_serve_opts opts = {.root_dir = objHost->Path};
    mg_http_serve_dir(c, hm, &opts);
}

// Service cleanup (called at shutdown)
void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
    printf("Service shutting down...\n");
}
```

## Available Callback Functions

| Function | Description |
|----------|-------------|
| `ServiceInit` | Called before service starts |
| `ServiceStart` | Called when service starts (custom services only) |
| `ServiceUnit` | Called when service stops |
| `RequestProc` | HTTP request handler |
| `EventProc` | Network event handler (custom protocols) |

## Project Structure

```
xserver/
├── lib/                    # Libraries
│   ├── xrt/               # Runtime library (arrays, dicts, JSON, etc.)
│   ├── xdo/               # Database abstraction layer
│   ├── mongoose.c/h       # Network library
│   ├── libtcc.h           # Dynamic compiler
│   └── sqlite3.c/h        # Database engine
├── src/                    # Protocol implementations
│   ├── http.h             # HTTP protocol
│   ├── mqtt.h             # MQTT protocol
│   ├── websocket.h        # WebSocket
│   ├── tcp.h, udp.h       # TCP/UDP
│   ├── custom.h           # Custom services
│   └── import_c/          # Dynamic script loader
├── release/                # Distribution directory
│   ├── script/            # Business scripts (dynamically loaded)
│   ├── wwwroot/           # Web root
│   ├── tcc/               # TCC runtime
│   └── xs.json            # Server configuration
├── tcc/                    # TCC compiler source
├── main.c                  # Main entry point
└── build.bat/sh           # Build scripts
```

## Use Cases

- **Embedded Web Servers**: Lightweight HTTP server for embedded devices
- **IoT Gateways**: MQTT broker and protocol conversion
- **Edge Computing**: Microservice nodes at the network edge
- **API Gateway**: RESTful API routing and aggregation
- **Static File Server**: High-performance static content delivery
- **Real-time Services**: WebSocket-based push notifications

## Technical Highlights

1. **Hot Reload**: Update business logic without restarting via dynamic C compilation
2. **Single Binary**: No external dependencies, easy deployment
3. **High Performance**: C language + Mongoose ensures excellent concurrency
4. **Flexible Configuration**: JSON-driven, supports multiple services and hosts
5. **Rich Protocols**: One framework supports 8+ network protocols
6. **Developer Friendly**: Write "scripts" in C with runtime compilation

## Dependencies

- **Mongoose**: Embedded network library
- **TCC**: Tiny C Compiler for runtime compilation
- **SQLite3**: Embedded database
- **XRT**: Custom runtime library
- **XDO**: Database abstraction layer

## License

Please refer to the project license file.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## Contact

For issues and feature requests, please use the GitHub Issue Tracker.
