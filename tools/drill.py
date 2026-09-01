"""xs 四协议长时间健壮性演练。

用法：python tools/drill.py [hours] [--round-gap seconds] [--seed value] [--exe path]
每轮执行一个异常输入/重载/连接压力样本，随后运行四协议金丝雀并采样 RSS。
配置、被破坏的脚本副本和日志都位于临时目录；报告默认写入系统临时目录。
"""

from __future__ import annotations

import argparse
import base64
import http.client
import json
import os
from pathlib import Path
import random
import signal
import shutil
import socket
import struct
import subprocess
import tempfile
import time

from smoke_config import randomize_config


ROOT = Path(__file__).resolve().parent.parent
RELEASE = ROOT / "release"
DEFAULT_EXE = RELEASE / ("xs.exe" if os.name == "nt" else "xs")
PORTS: dict[str, int] = {}
SCRIPT_PATH = Path()
REPORT_PATH = Path()
report: dict[str, object] = {}


def log(message: str) -> None:
    print(f"[{time.strftime('%H:%M:%S')}] {message}", flush=True)


def write_report() -> None:
    """原子刷新报告，避免中途观察到半截 JSON。"""
    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    pending = REPORT_PATH.with_name(REPORT_PATH.name + ".tmp")
    pending.write_text(
        json.dumps(report, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    os.replace(pending, REPORT_PATH)


def wait_port(port: int, timeout: float = 12.0) -> bool:
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


def rss_of(pid: int) -> int:
    if os.name == "nt":
        result = subprocess.run(
            [
                "powershell", "-NoProfile", "-Command",
                f"(Get-Process -Id {pid} -ErrorAction Stop).WorkingSet64",
            ],
            capture_output=True, text=True, check=False)
        value = result.stdout.strip()
        try:
            return int(value) // 1024
        except ValueError:
            return -1

    status = Path(f"/proc/{pid}/status")
    try:
        for line in status.read_text(encoding="utf-8").splitlines():
            if line.startswith("VmRSS:"):
                return int(line.split()[1])
    except (OSError, ValueError, IndexError):
        pass
    result = subprocess.run(
        ["ps", "-o", "rss=", "-p", str(pid)],
        capture_output=True, text=True, check=False)
    try:
        return int(result.stdout.strip())
    except ValueError:
        return -1


# ---------------- 异常与压力样本 ----------------

def atk_tcp_garbage() -> None:
    with socket.create_connection(("127.0.0.1", PORTS["tcp-echo"]), timeout=2) as stream:
        stream.sendall(os.urandom(random.randint(1, 4096)))
        time.sleep(0.05)


def atk_tcp_slowloris() -> None:
    with socket.create_connection(("127.0.0.1", PORTS["main"]), timeout=2) as stream:
        stream.sendall(b"GET / HTTP/1.1\r\nHost: x\r\n")
        for _ in range(random.randint(3, 8)):
            time.sleep(0.3)
            try:
                stream.sendall(b"X-Pad: aaaa\r\n")
            except OSError:
                break


def atk_http_malformed() -> None:
    with socket.create_connection(("127.0.0.1", PORTS["main"]), timeout=2) as stream:
        stream.sendall(random.choice([
            b"\x00\x01\x02\x03 GARBAGE\r\n\r\n",
            b"GET\r\n\r\n",
            b"GET /" + b"a" * 8000 + b" HTTP/1.1\r\nHost: x\r\n\r\n",
            b"POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 99999999\r\n\r\nshort",
            b"GET / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\nZZZZ\r\n",
        ]))
        time.sleep(0.1)
        try:
            stream.recv(4096)
        except OSError:
            pass


def atk_http_pipeline_bomb() -> None:
    with socket.create_connection(("127.0.0.1", PORTS["main"]), timeout=2) as stream:
        request = b"GET /text HTTP/1.1\r\nHost: x\r\n\r\n"
        stream.sendall(request * 200)
        time.sleep(0.3)


def atk_udp_garbage() -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as datagram:
        for _ in range(random.randint(5, 50)):
            datagram.sendto(
                os.urandom(random.randint(0, 2000)),
                ("127.0.0.1", PORTS["telemetry"]))


def atk_ws_badclient() -> None:
    # 坏握手。
    with socket.create_connection(("127.0.0.1", PORTS["ws-echo"]), timeout=2) as stream:
        stream.sendall(b"GET / HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\n\r\n")
        time.sleep(0.05)

    # 正常握手后发送无 mask 帧和超限帧声明。
    with socket.create_connection(("127.0.0.1", PORTS["ws-echo"]), timeout=2) as stream:
        stream.settimeout(2)
        key = base64.b64encode(os.urandom(16)).decode()
        stream.sendall((
            "GET / HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n"
        ).encode())
        buffer = b""
        while b"\r\n\r\n" not in buffer:
            chunk = stream.recv(4096)
            if not chunk:
                return
            buffer += chunk
        stream.sendall(bytes([0x81, 0x02]) + b"hi")
        try:
            stream.sendall(bytes([0x82, 0x7F]) + struct.pack(">Q", 1 << 40))
        except OSError:
            pass
        time.sleep(0.1)


def request_reload() -> None:
    connection = http.client.HTTPConnection("127.0.0.1", PORTS["main"], timeout=3)
    try:
        connection.request("GET", "/reload")
        response = connection.getresponse()
        response.read()
    finally:
        connection.close()


def atk_reload_bomb() -> None:
    source = SCRIPT_PATH.read_text(encoding="utf-8")
    broken = source.replace("RequestProc(XS_HttpReq", "RequestProc(XS_HttpReq BROKEN")
    try:
        for _ in range(3):
            SCRIPT_PATH.write_text(broken, encoding="utf-8")
            try:
                request_reload()
            except (OSError, http.client.HTTPException):
                pass
            SCRIPT_PATH.write_text(source, encoding="utf-8")
            try:
                request_reload()
            except (OSError, http.client.HTTPException):
                pass
    finally:
        SCRIPT_PATH.write_text(source, encoding="utf-8")


def atk_flood_connect() -> None:
    streams: list[socket.socket] = []
    try:
        for _ in range(120):
            try:
                streams.append(socket.create_connection(
                    ("127.0.0.1", PORTS["tcp-echo"]), timeout=0.5))
            except OSError:
                break
        time.sleep(0.5)
    finally:
        for stream in streams:
            stream.close()


ATTACKS = [
    atk_tcp_garbage,
    atk_tcp_slowloris,
    atk_http_malformed,
    atk_http_pipeline_bomb,
    atk_udp_garbage,
    atk_ws_badclient,
    atk_reload_bomb,
    atk_flood_connect,
]


# ---------------- 四协议金丝雀 ----------------

def canary() -> list[str]:
    failures: list[str] = []
    try:
        connection = http.client.HTTPConnection("127.0.0.1", PORTS["main"], timeout=5)
        try:
            connection.request("GET", "/text")
            response = connection.getresponse()
            if response.read() != b"xs3 http ok":
                failures.append("http-body")
        finally:
            connection.close()
    except Exception as exc:  # noqa: BLE001
        failures.append(f"http:{exc}")

    try:
        with socket.create_connection(("127.0.0.1", PORTS["tcp-echo"]), timeout=5) as stream:
            stream.settimeout(5)
            stream.recv(200)
            stream.sendall(b"canary")
            if stream.recv(100) != b"canary":
                failures.append("tcp-body")
    except Exception as exc:  # noqa: BLE001
        failures.append(f"tcp:{exc}")

    for attempt in range(3):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as datagram:
                datagram.settimeout(5)
                if hasattr(socket, "SIO_UDP_CONNRESET"):
                    try:
                        datagram.ioctl(socket.SIO_UDP_CONNRESET, False)
                    except OSError:
                        pass
                datagram.sendto(b"canary", ("127.0.0.1", PORTS["telemetry"]))
                data, _ = datagram.recvfrom(2048)
                if data != b"canary":
                    failures.append("udp-body")
            break
        except OSError as exc:
            if getattr(exc, "winerror", 0) == 10054 and attempt < 2:
                continue
            failures.append(f"udp:{exc}")
            break

    try:
        with socket.create_connection(("127.0.0.1", PORTS["ws-echo"]), timeout=5) as stream:
            stream.settimeout(5)
            key = base64.b64encode(os.urandom(16)).decode()
            stream.sendall((
                "GET / HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n"
            ).encode())
            buffer = b""
            while b"\r\n\r\n" not in buffer:
                chunk = stream.recv(4096)
                if not chunk:
                    raise ConnectionError("websocket handshake EOF")
                buffer += chunk
            payload = b"canary"
            mask = os.urandom(4)
            stream.sendall(bytes(
                bytearray([0x81, 0x80 | len(payload)])
                + mask
                + bytes(value ^ mask[index % 4] for index, value in enumerate(payload))))
            data = b""
            for _ in range(8):
                chunk = stream.recv(4096)
                if not chunk:
                    break
                data += chunk
                if data.endswith(payload):
                    break
            if not data.endswith(payload):
                failures.append("ws-body")
    except Exception as exc:  # noqa: BLE001
        failures.append(f"ws:{exc}")
    return failures


def main() -> int:
    global PORTS, REPORT_PATH, SCRIPT_PATH, report

    parser = argparse.ArgumentParser()
    parser.add_argument("hours", nargs="?", type=float, default=6.0)
    parser.add_argument("--round-gap", type=float, default=30.0)
    parser.add_argument("--seed", type=int)
    parser.add_argument("--exe", type=Path, default=DEFAULT_EXE)
    parser.add_argument(
        "--report", type=Path,
        default=Path(tempfile.gettempdir()) / "xs_drill_report.json")
    args = parser.parse_args()
    if args.hours <= 0 or args.round_gap < 0:
        parser.error("hours must be positive and round-gap must be non-negative")
    executable = args.exe.resolve()
    if not executable.is_file():
        parser.error(f"missing executable: {executable}")
    REPORT_PATH = args.report.resolve()
    if args.seed is not None:
        random.seed(args.seed)

    report = {
        "start": time.strftime("%F %T"),
        "hours": args.hours,
        "round_gap": args.round_gap,
        "seed": args.seed,
        "rounds": 0,
        "attacks": 0,
        "canary_failures": [],
        "rss_kb": [],
        "events": [],
    }
    config = json.loads((RELEASE / "xs.json").read_text(encoding="utf-8"))
    PORTS = randomize_config(config)

    server_exited = False
    log(f"drill start: {args.hours:g}h target, round gap {args.round_gap:g}s")
    log(f"report: {REPORT_PATH}")
    with tempfile.TemporaryDirectory(prefix="xs-drill-") as temp_name:
        temp_dir = Path(temp_name)
        for directory in ("script", "wwwroot", "hosts", "tls", "devsdk"):
            shutil.copytree(RELEASE / directory, temp_dir / directory)
        SCRIPT_PATH = temp_dir / "http_main.c"
        SCRIPT_PATH.write_text(
            (RELEASE / "script" / "http_main.c").read_text(encoding="utf-8"),
            encoding="utf-8")
        for service in config["services"]:
            if service.get("name") == "main":
                service["host_default"]["devfile"] = str(SCRIPT_PATH)
                break
        else:
            parser.error("main service missing from release/xs.json")
        config_path = temp_dir / "xs.json"
        config_path.write_text(
            json.dumps(config, ensure_ascii=False, indent=2), encoding="utf-8")

        popen_options: dict[str, object] = {}
        if os.name == "nt":
            popen_options["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP
        else:
            popen_options["start_new_session"] = True
        log_path = temp_dir / "xs_drill.log"
        with log_path.open("w", encoding="utf-8") as log_file:
            proc = subprocess.Popen(
                [str(executable), str(config_path)], cwd=str(RELEASE),
                stdout=log_file, stderr=subprocess.STDOUT, **popen_options)
            try:
                if not wait_port(PORTS["main"]):
                    report["events"].append("server startup timeout")
                    log("SERVER STARTUP FAILED")
                    return 1
                time.sleep(0.5)
                report["pid"] = proc.pid
                rss_start = rss_of(proc.pid)
                report["rss_start_kb"] = rss_start
                log(f"target pid={proc.pid} rss0={rss_start}KB")
                deadline = time.monotonic() + args.hours * 3600

                while time.monotonic() < deadline:
                    report["rounds"] += 1
                    attack = random.choice(ATTACKS)
                    try:
                        attack()
                        report["attacks"] += 1
                    except Exception as exc:  # noqa: BLE001
                        report["events"].append(
                            f"round{report['rounds']}: attack-error {attack.__name__}: {exc}")

                    failures = canary()
                    if failures:
                        report["canary_failures"].append({
                            "round": report["rounds"],
                            "after": attack.__name__,
                            "issues": failures,
                        })
                        log(
                            f"CANARY FAILURE round {report['rounds']} "
                            f"after {attack.__name__}: {failures}")

                    rss = rss_of(proc.pid)
                    report["rss_kb"].append(rss)
                    if report["rounds"] % 10 == 0:
                        log(
                            f"round {report['rounds']} attacks={report['attacks']} "
                            f"rss={rss}KB failures={len(report['canary_failures'])}")
                        write_report()

                    if proc.poll() is not None:
                        server_exited = True
                        report["events"].append(
                            f"round{report['rounds']}: SERVER EXITED code={proc.returncode}")
                        log("SERVER EXITED - aborting drill")
                        break
                    remaining = deadline - time.monotonic()
                    if remaining > 0:
                        time.sleep(min(args.round_gap, remaining))
            finally:
                report["end"] = time.strftime("%F %T")
                report["rss_end_kb"] = rss_of(proc.pid) if proc.poll() is None else -1
                stop_process(proc)
                write_report()

    failures = report["canary_failures"]
    assert isinstance(failures, list)
    log(
        f"drill end: rounds={report['rounds']} attacks={report['attacks']} "
        f"canary_failures={len(failures)} "
        f"rss {report.get('rss_start_kb')}->{report.get('rss_end_kb')}KB")
    return 1 if server_exited or failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
