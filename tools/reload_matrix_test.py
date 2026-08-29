"""四协议同端点 generation handoff 回归。"""

from __future__ import annotations

import argparse
import base64
import http.client
import json
import os
from pathlib import Path
import shutil
import signal
import socket
import struct
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parent.parent


def free_port() -> int:
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def wait_port(port: int, timeout: float = 12.0) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.05)
    raise AssertionError(f"port {port} did not become ready")


def http_get(port: int, path: str) -> tuple[int, str]:
    conn = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
    try:
        conn.request("GET", path)
        response = conn.getresponse()
        return response.status, response.read().decode()
    finally:
        conn.close()


def wait_http(port: int, path: str, expected: str, timeout: float = 10.0) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            if http_get(port, path) == (200, expected):
                return
        except OSError:
            pass
        time.sleep(0.05)
    raise AssertionError(f"{path} did not become {expected!r}")


def tcp_echo(sock: socket.socket, payload: bytes) -> None:
    sock.sendall(payload)
    data = sock.recv(4096)
    assert data == payload, data


def ws_open(port: int) -> socket.socket:
    sock = socket.create_connection(("127.0.0.1", port), timeout=3)
    sock.settimeout(3)
    key = base64.b64encode(os.urandom(16)).decode()
    sock.sendall(
        f"GET / HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
        f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n".encode()
    )
    response = b""
    while b"\r\n\r\n" not in response:
        response += sock.recv(4096)
    assert b" 101 " in response.split(b"\r\n", 1)[0], response[:100]
    return sock


def ws_echo(sock: socket.socket, payload: bytes) -> None:
    mask = os.urandom(4)
    assert len(payload) < 126
    frame = bytes([0x81, 0x80 | len(payload)]) + mask
    frame += bytes(value ^ mask[index % 4] for index, value in enumerate(payload))
    sock.sendall(frame)
    data = b""
    deadline = time.time() + 3
    while payload not in data and time.time() < deadline:
        data += sock.recv(4096)
    assert payload in data, data


def udp_echo(port: int, payload: bytes) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.settimeout(3)
        sock.sendto(payload, ("127.0.0.1", port))
        data, _ = sock.recvfrom(4096)
        assert data == payload, data


def write_config(path: Path, ports: dict[str, int], marker: int) -> None:
    services = [
        {
            "enabled": True,
            "class": "http",
            "name": "main",
            "ip": "127.0.0.1",
            "port": ports["http"],
            "config_marker": marker,
            "host_default": {
                "name": "app",
                "path": "wwwroot",
                "devlang": "c",
                "devfile": "script/http_main.c",
            },
        },
        {
            "enabled": True,
            "class": "tcp",
            "name": "tcp-echo",
            "ip": "127.0.0.1",
            "port": ports["tcp"],
            "reload_marker": marker,
            "devlang": "c",
            "devfile": "script/tcp_main.c",
        },
        {
            "enabled": True,
            "class": "udp",
            "name": "udp-echo",
            "ip": "127.0.0.1",
            "port": ports["udp"],
            "reload_marker": marker,
            "devlang": "c",
            "devfile": "script/udp_main.c",
        },
        {
            "enabled": True,
            "class": "ws",
            "name": "ws-echo",
            "ip": "127.0.0.1",
            "port": ports["ws"],
            "reload_marker": marker,
            "devlang": "c",
            "devfile": "script/ws_main.c",
        },
    ]
    config = {"engine": {"workers": 2}, "reload_root": marker, "services": services}
    path.write_text(json.dumps(config, indent=2), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, default=ROOT / "release" / "xs.exe")
    args = parser.parse_args()
    source_exe = args.exe.resolve()
    ports = {name: free_port() for name in ("http", "tcp", "udp", "ws")}

    with tempfile.TemporaryDirectory(prefix="xs-reload-matrix-") as temp_name:
        app = Path(temp_name)
        exe = app / source_exe.name
        log_path = app / "run.log"
        shutil.copy2(source_exe, exe)
        shutil.copytree(ROOT / "release" / "script", app / "script")
        shutil.copytree(ROOT / "release" / "wwwroot", app / "wwwroot")
        write_config(app / "xs.json", ports, 0)

        creationflags = subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0
        with log_path.open("w", encoding="utf-8") as log:
            process = subprocess.Popen(
                [str(exe)], cwd=app, stdout=log, stderr=subprocess.STDOUT,
                creationflags=creationflags,
            )
            old_http: http.client.HTTPConnection | None = None
            old_tcp: socket.socket | None = None
            old_ws: socket.socket | None = None
            try:
                for name in ("http", "tcp", "ws"):
                    wait_port(ports[name])

                old_http = http.client.HTTPConnection("127.0.0.1", ports["http"], timeout=5)
                old_http.request("GET", "/configstat")
                assert old_http.getresponse().read() == b"0"
                old_http.request("GET", "/lease-hold")
                assert old_http.getresponse().read() == str(ports["tcp"]).encode()

                old_tcp = socket.create_connection(("127.0.0.1", ports["tcp"]), timeout=3)
                old_tcp.settimeout(3)
                assert old_tcp.recv(256).startswith(b"[xs3-tcp]")
                tcp_echo(old_tcp, b"old-tcp-before")

                old_ws = ws_open(ports["ws"])
                ws_echo(old_ws, b"old-ws-before")
                udp_echo(ports["udp"], b"udp-before")

                write_config(app / "xs.json", ports, 1)
                assert http_get(ports["http"], "/reload-all")[0] == 200
                wait_http(ports["http"], "/configstat", "1")
                wait_http(ports["http"], "/rootstat", "1")

                deadline = time.time() + 8
                while time.time() < deadline:
                    text = log_path.read_text(encoding="utf-8", errors="replace")
                    if text.count("candidate active on retained listener") >= 4:
                        break
                    time.sleep(0.05)
                else:
                    raise AssertionError(text[-4000:])

                old_http.request("GET", "/configstat")
                assert old_http.getresponse().read() == b"0"
                old_http.request("GET", "/topology")
                assert old_http.getresponse().read() == str(ports["http"]).encode()
                tcp_echo(old_tcp, b"old-tcp-after")
                ws_echo(old_ws, b"old-ws-after")

                # HTTP 旧脚本持有旧 TCP server lease：TCP 连接关闭后仍不得提前释放。
                old_tcp.close()
                old_tcp = None
                time.sleep(0.2)
                text = log_path.read_text(encoding="utf-8", errors="replace")
                assert "generation finalized: server 'tcp-echo'" not in text, text[-3000:]
                old_http.request("GET", "/lease-release")
                assert old_http.getresponse().read() == str(ports["tcp"]).encode()
                deadline = time.time() + 5
                while time.time() < deadline:
                    text = log_path.read_text(encoding="utf-8", errors="replace")
                    if "generation finalized: server 'tcp-echo'" in text:
                        break
                    time.sleep(0.05)
                else:
                    raise AssertionError(text[-3000:])

                with socket.create_connection(("127.0.0.1", ports["tcp"]), timeout=3) as new_tcp:
                    new_tcp.settimeout(3)
                    assert new_tcp.recv(256).startswith(b"[xs3-tcp]")
                    tcp_echo(new_tcp, b"new-tcp")
                new_ws = ws_open(ports["ws"])
                try:
                    ws_echo(new_ws, b"new-ws")
                finally:
                    new_ws.close()
                udp_echo(ports["udp"], b"udp-after")
            finally:
                if old_http is not None:
                    old_http.close()
                if old_tcp is not None:
                    old_tcp.close()
                if old_ws is not None:
                    old_ws.close()
                if process.poll() is None:
                    if os.name == "nt":
                        process.send_signal(signal.CTRL_BREAK_EVENT)
                    else:
                        process.send_signal(signal.SIGTERM)
                try:
                    process.wait(timeout=25)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
                    raise AssertionError("reload matrix server did not drain")

        output = log_path.read_text(encoding="utf-8", errors="replace")
        assert process.returncode == 0, output[-4000:]
        for name in ("main", "tcp-echo", "udp-echo", "ws-echo"):
            assert output.count(f"generation finalized: server '{name}'") >= 2, output[-5000:]
        assert "[xs] bye" in output and "force-free" not in output
        print("RELOAD MATRIX PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
