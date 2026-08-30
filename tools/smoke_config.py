"""为冒烟测试生成全端口随机化的临时配置。

标准输出固定为：HTTP UDP TCP WS CUSTOM，便于 batch/shell 直接解包。
"""

from __future__ import annotations

import json
import os
from pathlib import Path
import socket
import sys


def free_port(sock_type: int, used: set[int]) -> int:
    while True:
        with socket.socket(socket.AF_INET, sock_type) as sock:
            sock.bind(("127.0.0.1", 0))
            port = int(sock.getsockname()[1])
        if port not in used:
            used.add(port)
            return port


def randomize_config(config: dict[str, object]) -> dict[str, int]:
    """原地随机化全部监听端口，并返回冒烟测试使用的命名端口。"""
    ports: dict[str, int] = {}
    used: set[int] = set()
    for service in config.get("services", []):
        name = service.get("name")
        service_class = service.get("class")
        if os.name != "nt" and name in {"crt_probe", "include_probe"}:
            service["enabled"] = False
        if service_class != "custom":
            sock_type = socket.SOCK_DGRAM if service_class == "udp" else socket.SOCK_STREAM
            port = free_port(sock_type, used)
            service["ip"] = "127.0.0.1"
            service["port"] = port
            ports[str(name)] = port
        elif name == "echo":
            port = free_port(socket.SOCK_STREAM, used)
            service["port"] = port  # custom 脚本从 server Custom 读取
            ports[str(name)] = port
    required = ("main", "telemetry", "tcp-echo", "ws-echo", "echo")
    missing = [name for name in required if name not in ports]
    if missing:
        raise SystemExit(f"required smoke services missing: {', '.join(missing)}")
    return ports


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: smoke_config.py <source.json> <output.json>")
    source = Path(sys.argv[1])
    output = Path(sys.argv[2])
    config = json.loads(source.read_text(encoding="utf-8"))
    ports = randomize_config(config)
    required = ("main", "telemetry", "tcp-echo", "ws-echo", "echo")
    output.write_text(json.dumps(config, ensure_ascii=False, indent=2), encoding="utf-8")
    print(*(ports[name] for name in required))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
