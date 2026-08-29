"""Generation 生命周期回归：旧连接留在旧代，终态后自动无超时回收。"""

from __future__ import annotations

import argparse
import http.client
import json
import os
from pathlib import Path
import shutil
import signal
import socket
import subprocess
import tempfile
import threading
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
            with socket.create_connection(("127.0.0.1", port), timeout=0.3):
                return
        except OSError:
            time.sleep(0.1)
    raise AssertionError(f"port {port} did not become ready")


def get(port: int, path: str) -> tuple[int, str]:
    conn = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
    try:
        conn.request("GET", path)
        response = conn.getresponse()
        return response.status, response.read().decode()
    finally:
        conn.close()


def submit_reload(port: int, path: str) -> int:
    status, body = get(port, path)
    assert status == 202, (status, body)
    return int(json.loads(body)["reload_id"])


def wait_reload(port: int, reload_id: int, timeout: float = 12.0) -> dict[str, object]:
    deadline = time.time() + timeout
    connection = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
    try:
        while time.time() < deadline:
            try:
                connection.request("GET", f"/reload-status/{reload_id}")
                response = connection.getresponse()
                body = response.read().decode()
            except OSError:
                connection.close()
                connection = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
                time.sleep(0.03)
                continue
            if response.status == 200:
                result = json.loads(body)
                if result["status"] in {"succeeded", "failed", "superseded", "cancelled"}:
                    return result
            time.sleep(0.03)
    finally:
        connection.close()
    raise AssertionError(f"reload {reload_id} did not reach terminal state")


def write_config(
    path: Path,
    port: int,
    *,
    marker: int = 0,
    root_marker: int = 0,
    init_delay_ms: int = 0,
    recv_limit: int = 0,
) -> None:
    config = {
        "engine": {"workers": 2},
        "reload_root": root_marker,
        "services": [
            {
                "enabled": True,
                "class": "http",
                "name": "main",
                "ip": "127.0.0.1",
                "port": port,
                "recv_limit": recv_limit,
                "config_marker": marker,
                "host_default": {
                    "name": "app",
                    "path": "wwwroot",
                    "devlang": "c",
                    "devfile": "script/http_main.c",
                    "init_delay_ms": init_delay_ms,
                },
            }
        ],
    }
    path.write_text(json.dumps(config, ensure_ascii=False, indent=2), encoding="utf-8")


def wait_refused(port: int, timeout: float = 5.0) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                time.sleep(0.05)
        except OSError:
            return
    raise AssertionError(f"old port {port} still accepts new connections")


def wait_body(port: int, path: str, expected: str, timeout: float = 8.0) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            if get(port, path) == (200, expected):
                return
        except OSError:
            pass
        time.sleep(0.05)
    raise AssertionError(f"{path} on {port} did not become {expected!r}")


def split_post(port: int, payload: bytes) -> None:
    """强制 Header/body 分两次到达，验证 Head 借用视图在等待期被 pin。"""
    head = (
        b"POST /echo HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: "
        + str(len(payload)).encode()
        + b"\r\nConnection: close\r\n\r\n"
    )
    with socket.create_connection(("127.0.0.1", port), timeout=3) as sock:
        sock.settimeout(3)
        sock.sendall(head)
        time.sleep(0.03)
        sock.sendall(payload)
        response = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response += chunk
    response_head, response_body = response.split(b"\r\n\r\n", 1)
    assert b" 200 " in response_head.split(b"\r\n", 1)[0], response[:200]
    assert response_body == payload, response_body


def post_keepalive(port: int) -> None:
    """脚本读完 body 后，驱动必须同步消费 wire bytes，下一请求才能正常解析。"""
    conn = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
    try:
        for index in range(10):
            payload = f"keepalive-{index}".encode()
            conn.request("POST", "/echo", body=payload)
            response = conn.getresponse()
            assert response.status == 200 and response.read() == payload
            conn.request("GET", "/text")
            response = conn.getresponse()
            assert response.status == 200 and response.read() == b"xs3 http ok"
    finally:
        conn.close()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, default=ROOT / "release" / "xs.exe")
    args = parser.parse_args()
    source_exe = args.exe.resolve()
    if not source_exe.exists():
        raise SystemExit(f"missing executable: {source_exe}")

    port_a = free_port()
    port_b = free_port()
    while port_b == port_a:
        port_b = free_port()

    with tempfile.TemporaryDirectory(prefix="xs-lifecycle-") as temp_name:
        app = Path(temp_name)
        exe = app / source_exe.name
        log_path = app / "run.log"
        shutil.copy2(source_exe, exe)
        shutil.copytree(ROOT / "release" / "script", app / "script")
        shutil.copytree(ROOT / "release" / "wwwroot", app / "wwwroot")
        write_config(app / "xs.json", port_a)

        creationflags = subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0
        with log_path.open("w", encoding="utf-8") as log:
            process = subprocess.Popen(
                [str(exe)], cwd=app, stdout=log, stderr=subprocess.STDOUT,
                creationflags=creationflags,
            )
            held_connections: list[http.client.HTTPConnection] = []
            hammer_stop: threading.Event | None = None
            hammer_thread: threading.Thread | None = None
            try:
                wait_port(port_a)

                for index in range(20):
                    split_post(port_a, f"split-{index}".encode())
                post_keepalive(port_a)

                # Host 换代：旧 keep-alive 连接继续跑旧脚本，新连接进入新脚本。
                old = http.client.HTTPConnection("127.0.0.1", port_a, timeout=5)
                held_connections.append(old)
                old.request("GET", "/swapstat")
                assert old.getresponse().read() == b"0"
                result = wait_reload(port_a, submit_reload(port_a, "/reload"))
                assert result["status"] == "succeeded", result
                old.request("GET", "/swapstat")
                assert old.getresponse().read() == b"0"
                assert get(port_a, "/swapstat") == (200, "1")
                old.close()

                # 同端点配置换代：listener 原地切槽；旧连接仍看到旧配置。
                same = http.client.HTTPConnection("127.0.0.1", port_a, timeout=5)
                held_connections.append(same)
                same.request("GET", "/configstat")
                assert same.getresponse().read() == b"0"
                write_config(
                    app / "xs.json", port_a, marker=1, root_marker=1, init_delay_ms=300,
                )

                hammer_errors: list[str] = []
                hammer_stop = threading.Event()

                def hammer() -> None:
                    while not hammer_stop.is_set():
                        try:
                            status, body = get(port_a, "/ready")
                            if status != 200 or body != "ready":
                                hammer_errors.append(f"ready={status}/{body}")
                                return
                            status, body = get(port_a, "/topology")
                            if status != 200 or body != str(port_a):
                                hammer_errors.append(f"topology={status}/{body}")
                                return
                        except Exception as exc:  # noqa: BLE001 - test captures worker races
                            if isinstance(exc, OSError) and getattr(exc, "winerror", None) == 10048:
                                time.sleep(0.02)
                                continue
                            hammer_errors.append(repr(exc))
                            return
                        time.sleep(0.01)

                hammer_thread = threading.Thread(target=hammer, daemon=True)
                hammer_thread.start()
                reload_id = submit_reload(port_a, "/reload-svr")
                result = wait_reload(port_a, reload_id)
                assert result["status"] == "succeeded", result
                wait_body(port_a, "/configstat", "1")
                wait_body(port_a, "/rootstat", "1")
                hammer_stop.set()
                hammer_thread.join(timeout=3)
                assert not hammer_errors, hammer_errors
                same.request("GET", "/configstat")
                assert same.getresponse().read() == b"0"
                same.close()

                # listener 固化字段改变时明确拒绝，旧配置与 Root 都保持一致。
                write_config(
                    app / "xs.json", port_a, marker=2, root_marker=2,
                    init_delay_ms=0, recv_limit=4096,
                )
                result = wait_reload(port_a, submit_reload(port_a, "/reload-svr"))
                assert result["status"] == "failed", result
                assert get(port_a, "/configstat") == (200, "1")
                assert get(port_a, "/rootstat") == (200, "1")

                # Server 换代：候选端口先就绪，旧端口只保留既有连接。
                draining = http.client.HTTPConnection("127.0.0.1", port_a, timeout=5)
                held_connections.append(draining)
                draining.request("GET", "/swapstat")
                assert draining.getresponse().read() == b"2"
                write_config(app / "xs.json", port_b, marker=3, root_marker=3)
                draining.request("GET", "/reload-svr")
                response = draining.getresponse()
                assert response.status == 202
                reload_id = int(json.loads(response.read())["reload_id"])
                wait_port(port_b)
                result = wait_reload(port_b, reload_id)
                assert result["status"] == "succeeded", result
                wait_refused(port_a)
                draining.request("GET", "/topology")
                assert draining.getresponse().read() == str(port_b).encode()
                draining.request("GET", "/swapstat")
                assert draining.getresponse().read() == b"2"
                assert get(port_b, "/swapstat") == (200, "3")
                draining.close()

                # 再换回原端口，覆盖动态配置快照所有权的二次释放。
                write_config(app / "xs.json", port_a, marker=4, root_marker=4)
                reload_id = submit_reload(port_b, "/reload-svr")
                wait_port(port_a)
                result = wait_reload(port_a, reload_id)
                assert result["status"] == "succeeded", result
                wait_refused(port_b)
                assert get(port_a, "/swapstat") == (200, "4")
            finally:
                if hammer_stop is not None:
                    hammer_stop.set()
                if hammer_thread is not None:
                    hammer_thread.join(timeout=3)
                for connection in held_connections:
                    connection.close()
                if process.poll() is None:
                    if os.name == "nt":
                        process.send_signal(signal.CTRL_BREAK_EVENT)
                    else:
                        process.send_signal(signal.SIGTERM)
                try:
                    process.wait(timeout=20)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
                    raise AssertionError("server did not drain within test timeout")

        output = log_path.read_text(encoding="utf-8", errors="replace")
        assert process.returncode == 0, output[-2000:]
        assert output.count("generation finalized: server 'main'") >= 3, output[-2000:]
        assert "candidate active on retained listener" in output, output[-3000:]
        assert "same endpoint changed an immutable listener field" in output, output[-3000:]
        assert "engine stopped" in output and "[xs] bye" in output, output[-2000:]
        assert "force-free" not in output and "timeout force" not in output
        print("LIFECYCLE RELOAD PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
