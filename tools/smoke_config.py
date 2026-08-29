"""为固定端口冒烟生成仅 HTTP 端口随机化的临时配置，并输出所选端口。"""

from __future__ import annotations

import json
from pathlib import Path
import socket
import sys


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: smoke_config.py <source.json> <output.json>")
    source = Path(sys.argv[1])
    output = Path(sys.argv[2])
    config = json.loads(source.read_text(encoding="utf-8"))
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        port = int(sock.getsockname()[1])
    for service in config.get("services", []):
        if service.get("name") == "main":
            service["ip"] = "127.0.0.1"
            service["port"] = port
            break
    else:
        raise SystemExit("main service missing")
    output.write_text(json.dumps(config, ensure_ascii=False, indent=2), encoding="utf-8")
    print(port)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
