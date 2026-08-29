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


def write_config(path: Path, port: int) -> None:
    config = {
        "engine": {"workers": 2},
        "services": [
            {
                "enabled": True,
                "class": "http",
                "name": "main",
                "ip": "127.0.0.1",
                "port": port,
                "host_default": {
                    "name": "app",
                    "path": "wwwroot",
                    "devlang": "c",
                    "devfile": "script/http_main.c",
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
            try:
                wait_port(port_a)

                # Host 换代：旧 keep-alive 连接继续跑旧脚本，新连接进入新脚本。
                old = http.client.HTTPConnection("127.0.0.1", port_a, timeout=5)
                old.request("GET", "/swapstat")
                assert old.getresponse().read() == b"0"
                assert get(port_a, "/reload")[0] == 200
                time.sleep(0.5)
                old.request("GET", "/swapstat")
                assert old.getresponse().read() == b"0"
                assert get(port_a, "/swapstat") == (200, "1")
                old.close()

                # Server 换代：候选端口先就绪，旧端口只保留既有连接。
                draining = http.client.HTTPConnection("127.0.0.1", port_a, timeout=5)
                draining.request("GET", "/swapstat")
                assert draining.getresponse().read() == b"1"
                write_config(app / "xs.json", port_b)
                draining.request("GET", "/reload-svr")
                response = draining.getresponse()
                assert response.status == 200
                response.read()
                wait_port(port_b)
                wait_refused(port_a)
                draining.request("GET", "/swapstat")
                assert draining.getresponse().read() == b"1"
                assert get(port_b, "/swapstat") == (200, "2")
                draining.close()

                # 再换回原端口，覆盖动态配置快照所有权的二次释放。
                write_config(app / "xs.json", port_a)
                assert get(port_b, "/reload-svr")[0] == 200
                wait_port(port_a)
                wait_refused(port_b)
                assert get(port_a, "/swapstat") == (200, "3")
            finally:
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
        assert "engine stopped" in output and "[xs] bye" in output, output[-2000:]
        assert "force-free" not in output and "timeout force" not in output
        print("LIFECYCLE RELOAD PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
