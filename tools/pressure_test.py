"""xs3 四协议本地回环压力基线。

用法：python tools/pressure_test.py [duration_sec] [--exe path]
每项输出请求总数、RPS 和错误数；临时配置使用动态端口，进程始终等待回收。
"""

from __future__ import annotations

import argparse
import base64
import http.client
import json
import os
from pathlib import Path
import signal
import shutil
import socket
import subprocess
import tempfile
import threading
import time

from smoke_config import randomize_config


ROOT = Path(__file__).resolve().parent.parent
RELEASE = ROOT / "release"
DEFAULT_EXE = RELEASE / ("xs.exe" if os.name == "nt" else "xs")
DURATION = 5.0
THREADS = 4
PORTS: dict[str, int] = {}

# 防回归阈值（本地回环；宽松基线，只拦截数量级回退）
THRESHOLDS = {
    "http": 3000,
    "tcp": 5000,
    "udp": 5000,
    "ws": 2000,
}


def wait_port(port: int, timeout: float = 10.0) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.5):
                return True
        except OSError:
            time.sleep(0.1)
    return False


def stop_process(proc: subprocess.Popen[object]) -> None:
    if proc.poll() is not None:
        return
    try:
        if os.name == "nt":
            proc.send_signal(signal.CTRL_BREAK_EVENT)
        else:
            proc.send_signal(signal.SIGTERM)
        proc.wait(timeout=12)
    except (OSError, ValueError, subprocess.TimeoutExpired):
        proc.kill()
        proc.wait(timeout=5)


def bench(name, worker):
    counts = [0] * THREADS
    errors = [0] * THREADS
    stop = time.monotonic() + DURATION
    threads = []

    def run(idx):
        try:
            counts[idx], errors[idx] = worker(stop, idx)
        except Exception:  # noqa: BLE001
            errors[idx] += 1

    for i in range(THREADS):
        thread = threading.Thread(target=run, args=(i,))
        thread.start()
        threads.append(thread)
    for thread in threads:
        thread.join()
    total, errs = sum(counts), sum(errors)
    rps = total / DURATION
    ok = errs == 0 and rps >= THRESHOLDS[name]
    print(f"{'OK ' if ok else 'FAIL'} {name:5s} total={total:<8} "
          f"rps={rps:<9.0f} errors={errs}")
    return ok


def http_worker(stop, _idx):
    n = err = 0
    connection = http.client.HTTPConnection("127.0.0.1", PORTS["main"], timeout=5)
    while time.monotonic() < stop:
        try:
            connection.request("GET", "/text")
            response = connection.getresponse()
            if response.read() != b"xs3 http ok":
                err += 1
            n += 1
        except Exception:  # noqa: BLE001
            err += 1
            connection.close()
            connection = http.client.HTTPConnection(
                "127.0.0.1", PORTS["main"], timeout=5)
    connection.close()
    return n, err


def tcp_worker(stop, _idx):
    n = err = 0
    with socket.create_connection(("127.0.0.1", PORTS["tcp-echo"]), timeout=5) as stream:
        stream.recv(200)
        payload = b"pressure"
        while time.monotonic() < stop:
            try:
                stream.sendall(payload)
                if stream.recv(100) != payload:
                    err += 1
                    break
                n += 1
            except Exception:  # noqa: BLE001
                err += 1
                break
    return n, err


def udp_worker(stop, _idx):
    n = err = 0
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as datagram:
        datagram.settimeout(2)
        payload = b"pressure"
        while time.monotonic() < stop:
            try:
                datagram.sendto(payload, ("127.0.0.1", PORTS["telemetry"]))
                data, _ = datagram.recvfrom(2048)
                if data != payload:
                    err += 1
                n += 1
            except Exception:  # noqa: BLE001
                err += 1
                break
    return n, err


def ws_worker(stop, _idx):
    n = err = 0
    with socket.create_connection(("127.0.0.1", PORTS["ws-echo"]), timeout=5) as stream:
        key = base64.b64encode(os.urandom(16)).decode()
        stream.sendall((
            "GET / HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n"
        ).encode())
        buffer = b""
        while b"\r\n\r\n" not in buffer:
            buffer += stream.recv(4096)
        # 排干 WsOpen 欢迎消息，避免污染首轮回显。
        time.sleep(0.05)
        stream.setblocking(False)
        try:
            while stream.recv(4096):
                pass
        except BlockingIOError:
            pass
        stream.setblocking(True)
        payload = b"pressure"
        mask = os.urandom(4)
        frame = bytearray([0x81, 0x80 | len(payload)]) + mask + bytes(
            value ^ mask[index % 4] for index, value in enumerate(payload))
        while time.monotonic() < stop:
            try:
                stream.sendall(frame)
                data = b""
                while len(data) < len(payload):
                    chunk = stream.recv(4096)
                    if not chunk:
                        raise ConnectionError("eof")
                    data += chunk
                if not data.endswith(payload):
                    err += 1
                    break
                n += 1
            except Exception:  # noqa: BLE001
                err += 1
                break
    return n, err


def main() -> int:
    global DURATION, THREADS, PORTS

    parser = argparse.ArgumentParser()
    parser.add_argument("duration", nargs="?", type=float, default=5.0)
    parser.add_argument("--threads", type=int, default=4)
    parser.add_argument("--exe", type=Path, default=DEFAULT_EXE)
    args = parser.parse_args()
    if args.duration <= 0 or args.threads <= 0:
        parser.error("duration and threads must be positive")
    executable = args.exe.resolve()
    if not executable.is_file():
        parser.error(f"missing executable: {executable}")
    DURATION = args.duration
    THREADS = args.threads

    config = json.loads((RELEASE / "xs.json").read_text(encoding="utf-8"))
    PORTS = randomize_config(config)
    ok = True
    with tempfile.TemporaryDirectory(prefix="xs-pressure-") as temp_name:
        temp_dir = Path(temp_name)
        for directory in ("script", "wwwroot", "hosts", "tls", "devsdk"):
            shutil.copytree(RELEASE / directory, temp_dir / directory)
        config_path = temp_dir / "xs.json"
        config_path.write_text(json.dumps(config, ensure_ascii=False, indent=2), encoding="utf-8")
        popen_options: dict[str, object] = {}
        if os.name == "nt":
            popen_options["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP
        else:
            popen_options["start_new_session"] = True
        with (temp_dir / "xs_pressure.log").open("w", encoding="utf-8") as log_file:
            proc = subprocess.Popen(
                [str(executable), str(config_path)], cwd=str(RELEASE),
                stdout=log_file, stderr=subprocess.STDOUT, **popen_options)
            try:
                if not wait_port(PORTS["main"]):
                    print("FAIL startup")
                    return 1
                time.sleep(0.5)
                print(f"pressure: {THREADS} threads x {DURATION:.1f}s per protocol")
                ok = bench("http", http_worker)
                ok = bench("tcp", tcp_worker) and ok
                ok = bench("udp", udp_worker) and ok
                ok = bench("ws", ws_worker) and ok
            finally:
                stop_process(proc)
    print("PRESSURE PASS" if ok else "PRESSURE FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
