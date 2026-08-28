"""xs3 压力测试：四协议 RPS 基线（本地回环，多线程）。

用法：python tools/pressure_test.py [duration_sec]
每项输出 请求总数 / 耗时 / RPS / 错误数，并按阈值判定（防回归基线，宽松）。
"""
import base64
import http.client
import os
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RELEASE = ROOT / 'release'
DURATION = float(sys.argv[1]) if len(sys.argv) > 1 else 5.0
THREADS = 4

# 防回归阈值（本地回环；宽松基线，只拦截数量级回退）
THRESHOLDS = {
    'http': 3000,
    'tcp': 5000,
    'udp': 5000,
    'ws': 2000,
}


def wait_port(port, timeout=10.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            socket.create_connection(('127.0.0.1', port), timeout=0.5).close()
            return True
        except OSError:
            time.sleep(0.2)
    return False


def bench(name, worker):
    counts = [0] * THREADS
    errors = [0] * THREADS
    stop = time.time() + DURATION
    threads = []

    def run(idx):
        try:
            counts[idx], errors[idx] = worker(stop, idx)
        except Exception:  # noqa: BLE001
            errors[idx] += 1

    for i in range(THREADS):
        t = threading.Thread(target=run, args=(i,))
        t.start()
        threads.append(t)
    for t in threads:
        t.join()
    total, errs = sum(counts), sum(errors)
    rps = total / DURATION
    ok = errs == 0 and rps >= THRESHOLDS[name]
    print(f"{'OK ' if ok else 'FAIL'} {name:5s} total={total:<8} rps={rps:<9.0f} errors={errs}")
    return ok


def http_worker(stop, idx):
    n = err = 0
    c = http.client.HTTPConnection('127.0.0.1', 8080, timeout=5)
    while time.time() < stop:
        try:
            c.request('GET', '/text')
            r = c.getresponse()
            if r.read() != b'xs3 http ok':
                err += 1
            n += 1
        except Exception:  # noqa: BLE001
            err += 1
            try:
                c.close()
                c = http.client.HTTPConnection('127.0.0.1', 8080, timeout=5)
            except Exception:  # noqa: BLE001
                time.sleep(0.05)
    c.close()
    return n, err


def tcp_worker(stop, idx):
    n = err = 0
    s = socket.create_connection(('127.0.0.1', 9097), timeout=5)
    s.recv(200)
    payload = b'pressure'
    while time.time() < stop:
        try:
            s.sendall(payload)
            data = s.recv(100)
            if data != payload:
                err += 1
                break
            n += 1
        except Exception:  # noqa: BLE001
            err += 1
            break
    s.close()
    return n, err


def udp_worker(stop, idx):
    n = err = 0
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(2)
    payload = b'pressure'
    while time.time() < stop:
        try:
            s.sendto(payload, ('127.0.0.1', 9090))
            data, _ = s.recvfrom(2048)
            if data != payload:
                err += 1
            n += 1
        except Exception:  # noqa: BLE001
            err += 1
            break
    s.close()
    return n, err


def ws_worker(stop, idx):
    n = err = 0
    s = socket.create_connection(('127.0.0.1', 9098), timeout=5)
    key = base64.b64encode(os.urandom(16)).decode()
    s.sendall((f'GET / HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n'
               f'Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n').encode())
    buf = b''
    while b'\r\n\r\n' not in buf:
        buf += s.recv(4096)
    # 排干连接 banner（WsOpen 的欢迎消息），避免污染首轮回显长度判断
    time.sleep(0.05)
    s.setblocking(False)
    try:
        while True:
            if not s.recv(4096):
                break
    except BlockingIOError:
        pass
    s.setblocking(True)
    payload = b'pressure'
    mask = os.urandom(4)
    frame = bytearray([0x81, 0x80 | len(payload)]) + mask + bytes(
        b ^ mask[i % 4] for i, b in enumerate(payload))
    while time.time() < stop:
        try:
            s.sendall(frame)
            data = b''
            while len(data) < len(payload):
                chunk = s.recv(4096)
                if not chunk:
                    raise ConnectionError('eof')
                data += chunk
            if not data.endswith(payload):
                err += 1
                break
            n += 1
        except Exception:  # noqa: BLE001
            err += 1
            break
    s.close()
    return n, err


def main():
    log = open(tempfile.gettempdir() + '/xs_pressure.log', 'w')
    proc = subprocess.Popen([str(RELEASE / 'xs.exe')], cwd=str(RELEASE),
                            stdout=log, stderr=subprocess.STDOUT)
    ok = True
    try:
        if not wait_port(8080):
            print('FAIL startup')
            return 1
        time.sleep(1.0)
        print(f'pressure: {THREADS} threads x {DURATION:.0f}s per protocol')
        ok = bench('http', http_worker)
        ok = bench('tcp', tcp_worker) and ok
        ok = bench('udp', udp_worker) and ok
        ok = bench('ws', ws_worker) and ok
    finally:
        proc.kill()
        log.close()
    print('PRESSURE PASS' if ok else 'PRESSURE FAIL')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
