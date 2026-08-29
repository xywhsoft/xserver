"""xs3 冒烟基线：WebSocket 断言（握手 101、文本回显、二进制回显、坏握手 400）。"""
import base64
import os
import socket
import struct
import sys
import time

HOST, PORT = '127.0.0.1', 9098
failures = []


def handshake(sock, key, host='x'):
    sock.sendall(
        f'GET / HTTP/1.1\r\nHost: {host}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n'
        f'Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n'.encode()
    )
    resp = b''
    while b'\r\n\r\n' not in resp:
        chunk = sock.recv(4096)
        if not chunk:
            raise ConnectionError('closed during handshake')
        resp += chunk
    return resp.split(b'\r\n\r\n', 1)


def send_frame(sock, opcode, payload):
    mask = os.urandom(4)
    n = len(payload)
    head = bytearray([0x80 | opcode])
    if n < 126:
        head.append(0x80 | n)
    else:
        head.append(0x80 | 126)
        head += struct.pack('>H', n)
    sock.sendall(bytes(head + mask + bytes(b ^ mask[i % 4] for i, b in enumerate(payload))))


def recv_until(sock, want, timeout=3.0):
    data = b''
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
        except socket.timeout:
            break
        if want in data:
            return data
    return data


# 1) 握手 + 文本回显
try:
    s = socket.create_connection((HOST, PORT), timeout=3)
    s.settimeout(2)
    key = base64.b64encode(os.urandom(16)).decode()
    head, rest = handshake(s, key)
    if b'101' not in head.split(b'\r\n', 1)[0]:
        failures.append(f'handshake: {head[:60]!r}')
    send_frame(s, 0x1, b'ws-smoke-text')
    data = recv_until(s, b'ws-smoke-text')
    if b'ws-smoke-text' not in data:
        failures.append(f'text echo: {data[:80]!r}')
    send_frame(s, 0x2, b'\x01\x02\x03bin')
    data = recv_until(s, b'\x01\x02\x03bin')
    if b'\x01\x02\x03bin' not in data:
        failures.append(f'binary echo: {data[:80]!r}')
    s.close()
except Exception as exc:  # noqa: BLE001
    failures.append(f'echo session: {exc}')

# 2) Host 选择次级 WS host，并固定其脚本。
try:
    s = socket.create_connection((HOST, PORT), timeout=3)
    s.settimeout(2)
    key = base64.b64encode(os.urandom(16)).decode()
    head, rest = handshake(s, key, host='admin.ws.example.com')
    if b'101' not in head.split(b'\r\n', 1)[0]:
        failures.append(f'vhost handshake: {head[:60]!r}')
    send_frame(s, 0x1, b'ignored-by-admin')
    data = rest + recv_until(s, b'ws-admin-script')
    if b'[xs3-ws-admin] connected' not in data or b'ws-admin-script' not in data:
        failures.append(f'vhost script: {data[:120]!r}')
    s.close()
except Exception as exc:  # noqa: BLE001
    failures.append(f'vhost session: {exc}')

# 3) 坏握手（缺 Key）→ 400 且连接关闭
try:
    s = socket.create_connection((HOST, PORT), timeout=3)
    s.settimeout(2)
    s.sendall(b'GET / HTTP/1.1\r\nHost: x\r\n\r\n')
    data = recv_until(s, b'\r\n\r\n', timeout=2.0)
    if b'400' not in data[:20]:
        failures.append(f'bad handshake: {data[:60]!r}')
    s.close()
except Exception as exc:  # noqa: BLE001
    failures.append(f'bad handshake: {exc}')

if failures:
    print('WS SMOKE FAILURES:')
    for item in failures:
        print(' -', item)
    sys.exit(1)
print('WS SMOKE OK')
