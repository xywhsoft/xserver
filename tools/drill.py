"""xs3 攻防演练（长时间对抗 + 金丝雀 + 内存监控）。

用法：python tools/drill.py <hours>
循环直至时限：每轮随机对抗样本（畸形/慢速/超限/轰炸）→ 四协议金丝雀 → RSS 采样。
输出：drill_report.json（逐轮）+ 控制台摘要；金丝雀失败记为事故并继续。
"""
import base64
import http.client
import json
import os
import random
import socket
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RELEASE = ROOT / 'release'
HOURS = float(sys.argv[1]) if len(sys.argv) > 1 else 6.0
DEADLINE = time.time() + HOURS * 3600
ROUND_GAP = 30.0

report = {
    'start': time.strftime('%F %T'),
    'hours': HOURS,
    'rounds': 0,
    'attacks': 0,
    'canary_failures': [],
    'rss_kb': [],
    'events': [],
}


def log(msg):
    print(f"[{time.strftime('%H:%M:%S')}] {msg}", flush=True)


def rss_of(pid):
    out = subprocess.run(
        ['powershell', '-NoProfile', '-Command',
         f'(Get-Process -Id {pid}).WorkingSet64'],
        capture_output=True, text=True).stdout.strip()
    try:
        return int(out) // 1024
    except ValueError:
        return -1


# ---------------- 对抗样本 ----------------

def atk_tcp_garbage():
    s = socket.create_connection(('127.0.0.1', 9097), timeout=2)
    s.sendall(os.urandom(random.randint(1, 4096)))
    time.sleep(0.05)
    s.close()


def atk_tcp_slowloris():
    s = socket.create_connection(('127.0.0.1', 8080), timeout=2)
    s.sendall(b'GET / HTTP/1.1\r\nHost: x\r\n')
    for _ in range(random.randint(3, 8)):
        time.sleep(0.3)
        try:
            s.sendall(b'X-Pad: aaaa\r\n')
        except OSError:
            break
    s.close()


def atk_http_malformed():
    s = socket.create_connection(('127.0.0.1', 8080), timeout=2)
    s.sendall(random.choice([
        b'\x00\x01\x02\x03 GARBAGE\r\n\r\n',
        b'GET\r\n\r\n',
        b'GET /' + b'a' * 8000 + b' HTTP/1.1\r\nHost: x\r\n\r\n',
        b'POST /echo HTTP/1.1\r\nHost: x\r\nContent-Length: 99999999\r\n\r\nshort',
        b'GET / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\nZZZZ\r\n',
    ]))
    time.sleep(0.1)
    try:
        s.recv(4096)
    except OSError:
        pass
    s.close()


def atk_http_pipeline_bomb():
    s = socket.create_connection(('127.0.0.1', 8080), timeout=2)
    req = b'GET /text HTTP/1.1\r\nHost: x\r\n\r\n'
    s.sendall(req * 200)
    time.sleep(0.3)
    s.close()


def atk_udp_garbage():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    for _ in range(random.randint(5, 50)):
        s.sendto(os.urandom(random.randint(0, 2000)), ('127.0.0.1', 9090))
    s.close()


def atk_ws_badclient():
    # 1) 坏握手
    s = socket.create_connection(('127.0.0.1', 9098), timeout=2)
    s.sendall(b'GET / HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\n\r\n')
    time.sleep(0.05)
    s.close()
    # 2) 正常握手后发无 mask 帧与超限帧声明
    s = socket.create_connection(('127.0.0.1', 9098), timeout=2)
    key = base64.b64encode(os.urandom(16)).decode()
    s.sendall((f'GET / HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n'
               f'Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n').encode())
    buf = b''
    while b'\r\n\r\n' not in buf:
        buf += s.recv(4096)
    s.sendall(bytes([0x81, 0x02]) + b'hi')                      # 客户端帧必须 mask
    s.sendall(bytes([0x82, 0x7F]) + struct.pack('>Q', 1 << 40))  # 超大帧声明
    time.sleep(0.1)
    s.close()


def atk_reload_bomb():
    script = RELEASE / 'script' / 'http_main.c'
    src = script.read_text(encoding='utf-8')
    bad = src.replace('RequestProc(XS_HttpReq', 'RequestProc(XS_HttpReq BROKEN')
    try:
        for _ in range(3):
            script.write_text(bad, encoding='utf-8')
            try:
                c = http.client.HTTPConnection('127.0.0.1', 8080, timeout=3)
                c.request('GET', '/reload')
                c.getresponse().read()
                c.close()
            except Exception:  # noqa: BLE001
                pass
            script.write_text(src, encoding='utf-8')
            try:
                c = http.client.HTTPConnection('127.0.0.1', 8080, timeout=3)
                c.request('GET', '/reload')
                c.getresponse().read()
                c.close()
            except Exception:  # noqa: BLE001
                pass
    finally:
        script.write_text(src, encoding='utf-8')


def atk_flood_connect():
    socks = []
    try:
        for _ in range(120):
            try:
                s = socket.create_connection(('127.0.0.1', 9097), timeout=0.5)
                socks.append(s)
            except OSError:
                break
        time.sleep(0.5)
    finally:
        for s in socks:
            s.close()


ATTACKS = [
    atk_tcp_garbage, atk_tcp_slowloris, atk_http_malformed, atk_http_pipeline_bomb,
    atk_udp_garbage, atk_ws_badclient, atk_reload_bomb, atk_flood_connect,
]

# ---------------- 金丝雀 ----------------

def canary():
    bad = []
    try:
        c = http.client.HTTPConnection('127.0.0.1', 8080, timeout=5)
        c.request('GET', '/text')
        r = c.getresponse()
        if r.read() != b'xs3 http ok':
            bad.append('http-body')
        c.close()
    except Exception as exc:  # noqa: BLE001
        bad.append(f'http:{exc}')
    try:
        s = socket.create_connection(('127.0.0.1', 9097), timeout=5)
        s.settimeout(5)
        s.recv(200)
        s.sendall(b'canary')
        if s.recv(100) != b'canary':
            bad.append('tcp-body')
        s.close()
    except Exception as exc:  # noqa: BLE001
        bad.append(f'tcp:{exc}')
    for attempt in range(3):
        try:
            u = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            u.settimeout(5)
            if hasattr(socket, 'SIO_UDP_CONNRESET'):
                try:
                    u.ioctl(socket.SIO_UDP_CONNRESET, False)  # 禁用 ICMP 复位上报（Windows）
                except OSError:
                    pass
            u.sendto(b'canary', ('127.0.0.1', 9090))
            d, _ = u.recvfrom(2048)
            if d != b'canary':
                bad.append('udp-body')
            u.close()
            break
        except OSError as exc:  # noqa: BLE001
            u.close()
            if getattr(exc, 'winerror', 0) == 10054 and attempt < 2:
                continue  # Windows ICMP 竞态：重试
            if getattr(exc, 'winerror', 0) == 10054:
                report['events'].append(f'round{report["rounds"]}: udp-icmp-artifact (non-fatal)')
                break
            bad.append(f'udp:{exc}')
            break
    try:
        s = socket.create_connection(('127.0.0.1', 9098), timeout=5)
        s.settimeout(5)
        key = base64.b64encode(os.urandom(16)).decode()
        s.sendall((f'GET / HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n'
                   f'Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n').encode())
        buf = b''
        while b'\r\n\r\n' not in buf:
            buf += s.recv(4096)
        payload = b'canary'
        mask = os.urandom(4)
        s.sendall(bytes(bytearray([0x81, 0x80 | len(payload)]) + mask + bytes(
            b ^ mask[i % 4] for i, b in enumerate(payload))))
        data = b''
        for _ in range(8):
            chunk = s.recv(4096)
            if not chunk:
                break
            data += chunk
            if data.endswith(payload):
                break
        if not data.endswith(payload):
            bad.append('ws-body')
        s.close()
    except Exception as exc:  # noqa: BLE001
        bad.append(f'ws:{exc}')
    return bad


def main():
    log_path = RELEASE / 'drill_report.json'
    log(f'drill start: {HOURS}h target, round gap {ROUND_GAP}s')
    proc = subprocess.Popen([str(RELEASE / 'xs.exe')], cwd=str(RELEASE),
                            stdout=open(tempfile.gettempdir() + '/xs_drill.log', 'w'),
                            stderr=subprocess.STDOUT)
    time.sleep(4.0)
    report['pid'] = proc.pid
    rss0 = rss_of(proc.pid)
    report['rss_start_kb'] = rss0
    log(f'target pid={proc.pid} rss0={rss0}KB')

    try:
        while time.time() < DEADLINE:
            report['rounds'] += 1
            atk = random.choice(ATTACKS)
            try:
                atk()
                report['attacks'] += 1
            except Exception as exc:  # noqa: BLE001
                report['events'].append(f'round{report["rounds"]}: attack-error {atk.__name__}: {exc}')

            bad = canary()
            if bad:
                report['canary_failures'].append(
                    {'round': report['rounds'], 'after': atk.__name__, 'issues': bad})
                log(f'CANARY FAILURE round {report["rounds"]} after {atk.__name__}: {bad}')

            rss = rss_of(proc.pid)
            report['rss_kb'].append(rss)
            if report['rounds'] % 10 == 0:
                log(f'round {report["rounds"]} attacks={report["attacks"]} rss={rss}KB '
                    f'failures={len(report["canary_failures"])}')
                json.dump(report, open(log_path, 'w', encoding='utf-8'), ensure_ascii=False, indent=1)

            if proc.poll() is not None:
                report['events'].append(f'round{report["rounds"]}: SERVER EXITED code={proc.returncode}')
                log('SERVER EXITED — aborting drill')
                break
            time.sleep(ROUND_GAP)
    finally:
        report['end'] = time.strftime('%F %T')
        report['rss_end_kb'] = rss_of(proc.pid) if proc.poll() is None else -1
        proc.kill()
        json.dump(report, open(log_path, 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
        log(f"drill end: rounds={report['rounds']} attacks={report['attacks']} "
            f"canary_failures={len(report['canary_failures'])} "
            f"rss {report.get('rss_start_kb')}→{report.get('rss_end_kb')}KB")


if __name__ == '__main__':
    main()
