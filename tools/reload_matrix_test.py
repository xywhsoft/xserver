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
import ssl
import struct
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parent.parent


def free_port(sock_type: int, used: set[int]) -> int:
    while True:
        with socket.socket(socket.AF_INET, sock_type) as sock:
            sock.bind(("127.0.0.1", 0))
            port = int(sock.getsockname()[1])
        if port not in used:
            used.add(port)
            return port


def wait_port(port: int, timeout: float = 12.0) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.05)
    raise AssertionError(f"port {port} did not become ready")


def http_get(port: int, path: str, *, tls: bool = False) -> tuple[int, str]:
    if tls:
        conn = http.client.HTTPSConnection(
            "127.0.0.1", port, timeout=5,
            context=ssl._create_unverified_context(),
        )
    else:
        conn = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
    try:
        conn.request("GET", path)
        response = conn.getresponse()
        return response.status, response.read().decode()
    finally:
        conn.close()


def submit_reload(port: int, path: str) -> int:
    status, body = http_get(port, path)
    assert status == 202, (status, body)
    return int(json.loads(body)["reload_id"])


def wait_reload(port: int, reload_id: int, timeout: float = 12.0) -> dict[str, object]:
    deadline = time.time() + timeout
    while time.time() < deadline:
        status, body = http_get(port, f"/reload-status/{reload_id}")
        if status == 200:
            result = json.loads(body)
            if result["status"] in {"succeeded", "failed", "superseded", "cancelled"}:
                return result
        time.sleep(0.03)
    raise AssertionError(f"reload {reload_id} did not reach terminal state")


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


def stream_open(port: int, *, tls: bool = False) -> socket.socket:
    sock = socket.create_connection(("127.0.0.1", port), timeout=3)
    if tls:
        sock = ssl._create_unverified_context().wrap_socket(
            sock, server_hostname="localhost")
    sock.settimeout(3)
    return sock


def ws_open(port: int, host: str = "x", *, tls: bool = False) -> socket.socket:
    sock = stream_open(port, tls=tls)
    key = base64.b64encode(os.urandom(16)).decode()
    sock.sendall(
        f"GET / HTTP/1.1\r\nHost: {host}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
        f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n".encode()
    )
    response = b""
    while b"\r\n\r\n" not in response:
        response += sock.recv(4096)
    assert b" 101 " in response.split(b"\r\n", 1)[0], response[:100]
    return sock


def ws_send_expect(sock: socket.socket, payload: bytes, expected: bytes) -> None:
    mask = os.urandom(4)
    assert len(payload) < 126
    frame = bytes([0x81, 0x80 | len(payload)]) + mask
    frame += bytes(value ^ mask[index % 4] for index, value in enumerate(payload))
    sock.sendall(frame)
    data = b""
    deadline = time.time() + 3
    while expected not in data and time.time() < deadline:
        data += sock.recv(4096)
    assert expected in data, data


def ws_echo(sock: socket.socket, payload: bytes) -> None:
    ws_send_expect(sock, payload, payload)


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
            "tls": True,
            "ip_tls": "127.0.0.1",
            "port_tls": ports["https"],
            "config_marker": marker,
            "host_default": {
                "name": "app",
                "path": "wwwroot",
                "devlang": "c",
                "devfile": "script/http_main.c",
                "tls_cert": "tls/xtps_cert.pem",
                "tls_key": "tls/xtps_key.pem",
            },
        },
        {
            "enabled": True,
            "class": "tcp",
            "name": "tcp-echo",
            "ip": "127.0.0.1",
            "port": ports["tcp"],
            "tls": True,
            "ip_tls": "127.0.0.1",
            "port_tls": ports["tcps"],
            "reload_marker": marker,
            "devlang": "c",
            "devfile": "script/tcp_main.c",
            "host_default": {
                "tls_cert": "tls/xtps_cert.pem",
                "tls_key": "tls/xtps_key.pem",
            },
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
            "tls": True,
            "ip_tls": "127.0.0.1",
            "port_tls": ports["wss"],
            "reload_marker": marker,
            "devlang": "c",
            "devfile": "script/ws_main.c",
            "host_default": {
                "tls_cert": "tls/xtps_cert.pem",
                "tls_key": "tls/xtps_key.pem",
            },
            "hosts": [
                {
                    "name": "ws-admin",
                    "host": "admin.ws.example.com",
                    "devlang": "c",
                    "devfile": "hosts/ws-admin/main.c",
                },
            ],
        },
    ]
    config = {"engine": {"workers": 2}, "reload_root": marker, "services": services}
    path.write_text(json.dumps(config, indent=2), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, default=ROOT / "release" / "xs.exe")
    args = parser.parse_args()
    source_exe = args.exe.resolve()
    used_ports: set[int] = set()
    ports = {
        "http": free_port(socket.SOCK_STREAM, used_ports),
        "https": free_port(socket.SOCK_STREAM, used_ports),
        "tcp": free_port(socket.SOCK_STREAM, used_ports),
        "tcps": free_port(socket.SOCK_STREAM, used_ports),
        "udp": free_port(socket.SOCK_DGRAM, used_ports),
        "ws": free_port(socket.SOCK_STREAM, used_ports),
        "wss": free_port(socket.SOCK_STREAM, used_ports),
    }

    with tempfile.TemporaryDirectory(prefix="xs-reload-matrix-") as temp_name:
        app = Path(temp_name)
        exe = app / source_exe.name
        log_path = app / "run.log"
        shutil.copy2(source_exe, exe)
        shutil.copytree(ROOT / "release" / "script", app / "script")
        shutil.copytree(ROOT / "release" / "wwwroot", app / "wwwroot")
        shutil.copytree(ROOT / "release" / "hosts", app / "hosts")
        shutil.copytree(ROOT / "release" / "tls", app / "tls")
        write_config(app / "xs.json", ports, 0)

        creationflags = subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0
        with log_path.open("w", encoding="utf-8") as log:
            process = subprocess.Popen(
                [str(exe)], cwd=app, stdout=log, stderr=subprocess.STDOUT,
                creationflags=creationflags,
            )
            old_http: http.client.HTTPConnection | None = None
            old_https: http.client.HTTPSConnection | None = None
            old_tcp: socket.socket | None = None
            old_tcps: socket.socket | None = None
            old_ws: socket.socket | None = None
            old_wss: socket.socket | None = None
            old_ws_admin: socket.socket | None = None
            failure: BaseException | None = None
            try:
                for name in ("http", "https", "tcp", "tcps", "ws", "wss"):
                    wait_port(ports[name])

                old_http = http.client.HTTPConnection("127.0.0.1", ports["http"], timeout=5)
                old_http.request("GET", "/configstat")
                assert old_http.getresponse().read() == b"0"
                old_http.request("GET", "/lease-hold")
                assert old_http.getresponse().read() == str(ports["tcp"]).encode()

                old_https = http.client.HTTPSConnection(
                    "127.0.0.1", ports["https"], timeout=5,
                    context=ssl._create_unverified_context(),
                )
                old_https.request("GET", "/configstat")
                assert old_https.getresponse().read() == b"0"

                old_tcp = stream_open(ports["tcp"])
                assert old_tcp.recv(256).startswith(b"[xs3-tcp]")
                tcp_echo(old_tcp, b"old-tcp-before")
                old_tcps = stream_open(ports["tcps"], tls=True)
                assert old_tcps.recv(256).startswith(b"[xs3-tcp]")
                tcp_echo(old_tcps, b"old-tcps-before")

                old_ws = ws_open(ports["ws"])
                ws_echo(old_ws, b"old-ws-before")
                old_wss = ws_open(ports["wss"], tls=True)
                ws_echo(old_wss, b"old-wss-before")
                old_ws_admin = ws_open(ports["ws"], "admin.ws.example.com")
                ws_send_expect(old_ws_admin, b"before", b"ws-admin-script")
                udp_echo(ports["udp"], b"udp-before")

                admin_ws_script = app / "hosts" / "ws-admin" / "main.c"
                admin_ws_script.write_text(
                    admin_ws_script.read_text(encoding="utf-8").replace(
                        "ws-admin-script", "ws-admin-new"),
                    encoding="utf-8",
                )
                write_config(app / "xs.json", ports, 1)
                reload_id = submit_reload(ports["http"], "/reload-all")
                result = wait_reload(ports["http"], reload_id)
                assert result["status"] == "succeeded" and result["revision"], result
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
                old_https.request("GET", "/configstat")
                assert old_https.getresponse().read() == b"0"
                tcp_echo(old_tcp, b"old-tcp-after")
                tcp_echo(old_tcps, b"old-tcps-after")
                ws_echo(old_ws, b"old-ws-after")
                ws_echo(old_wss, b"old-wss-after")
                ws_send_expect(old_ws_admin, b"after", b"ws-admin-script")

                assert http_get(ports["https"], "/configstat", tls=True) == (200, "1")

                # HTTP 旧脚本持有旧 TCP server lease：TCP 连接关闭后仍不得提前释放。
                old_tcp.close()
                old_tcp = None
                old_tcps.close()
                old_tcps = None
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
                with stream_open(ports["tcps"], tls=True) as new_tcps:
                    assert new_tcps.recv(256).startswith(b"[xs3-tcp]")
                    tcp_echo(new_tcps, b"new-tcps")
                new_ws = ws_open(ports["ws"])
                try:
                    ws_echo(new_ws, b"new-ws")
                finally:
                    new_ws.close()
                new_ws_admin = ws_open(ports["ws"], "admin.ws.example.com")
                try:
                    ws_send_expect(new_ws_admin, b"new", b"ws-admin-new")
                finally:
                    new_ws_admin.close()
                new_wss = ws_open(ports["wss"], tls=True)
                try:
                    ws_echo(new_wss, b"new-wss")
                finally:
                    new_wss.close()

                # 非 TLS 字节与立即断开的握手只能走 listener 内部终态，
                # 不得误调用尚未安装连接 pData 的业务 Close 回调。
                for tls_port in (ports["https"], ports["tcps"], ports["wss"]):
                    with socket.create_connection(("127.0.0.1", tls_port), timeout=3) as bad_tls:
                        bad_tls.sendall(b"not-a-tls-client")
                assert http_get(ports["http"], "/configstat") == (200, "1")
                udp_echo(ports["udp"], b"udp-after")
            except BaseException as exc:
                failure = exc
            finally:
                if old_http is not None:
                    old_http.close()
                if old_https is not None:
                    old_https.close()
                if old_tcp is not None:
                    old_tcp.close()
                if old_tcps is not None:
                    old_tcps.close()
                if old_ws is not None:
                    old_ws.close()
                if old_wss is not None:
                    old_wss.close()
                if old_ws_admin is not None:
                    old_ws_admin.close()
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

            if failure is not None:
                output = log_path.read_text(encoding="utf-8", errors="replace")
                raise AssertionError(
                    f"{failure}\n--- server log ---\n{output[-5000:]}") from failure

        output = log_path.read_text(encoding="utf-8", errors="replace")
        assert process.returncode == 0, output[-4000:]
        for name in ("main", "tcp-echo", "udp-echo", "ws-echo"):
            assert output.count(f"generation finalized: server '{name}'") >= 2, output[-5000:]
        assert "[xs] bye" in output and "force-free" not in output
        print("RELOAD MATRIX PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
