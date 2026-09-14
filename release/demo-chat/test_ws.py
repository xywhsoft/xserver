import base64, hashlib, json, os, socket, struct, sys, time

HOST, PORT = '127.0.0.1', 9091

def ws_connect():
    s = socket.create_connection((HOST, PORT), timeout=10)
    key = base64.b64encode(os.urandom(16)).decode()
    s.sendall((
        f'GET / HTTP/1.1\r\nHost: {HOST}:{PORT}\r\n'
        f'Upgrade: websocket\r\nConnection: Upgrade\r\n'
        f'Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n'
    ).encode())
    resp = b''
    while b'\r\n\r\n' not in resp:
        chunk = s.recv(4096)
        if not chunk:
            raise RuntimeError('handshake EOF')
        resp += chunk
    status = resp.split(b'\r\n', 1)[0].decode()
    if '101' not in status:
        raise RuntimeError('handshake failed: ' + status)
    return s, resp.split(b'\r\n\r\n', 1)[1]

def send_text(s, text):
    payload = text.encode()
    mask = os.urandom(4)
    header = bytearray([0x81])
    n = len(payload)
    if n < 126:
        header.append(0x80 | n)
    elif n < 65536:
        header.append(0x80 | 126)
        header += struct.pack('>H', n)
    else:
        header.append(0x80 | 127)
        header += struct.pack('>Q', n)
    header += mask
    masked = bytes(b ^ mask[i % 4] for i, b in enumerate(payload))
    s.sendall(bytes(header) + masked)

def recv_frame(s, timeout=30.0):
    s.settimeout(timeout)
    hdr = s.recv(2)
    if len(hdr) < 2:
        return None, None
    opcode = hdr[0] & 0x0F
    n = hdr[1] & 0x7F
    if n == 126:
        n = struct.unpack('>H', s.recv(2))[0]
    elif n == 127:
        n = struct.unpack('>Q', s.recv(8))[0]
    data = b''
    while len(data) < n:
        chunk = s.recv(n - len(data))
        if not chunk:
            break
        data += chunk
    return opcode, data.decode('utf-8', 'replace')

def drain_until_done(s, max_seconds=120, print_deltas=True):
    start = time.time()
    deltas = 0
    result = {'done': False, 'error': None, 'text': '', 'frames': 0}
    while time.time() - start < max_seconds:
        try:
            op, data = recv_frame(s, timeout=max_seconds)
        except socket.timeout:
            break
        if op is None:
            break
        if op == 8:
            print('[closed by server]')
            break
        if op != 1:
            continue
        result['frames'] += 1
        try:
            f = json.loads(data)
        except Exception:
            print('[non-json]', data[:80])
            continue
        t = f.get('type')
        if t == 'delta':
            deltas += 1
            result['text'] += f.get('text', '')
            if print_deltas and deltas <= 3:
                print('  delta#%d: %r' % (deltas, f.get('text', '')[:30]))
        elif t == 'thinking':
            pass
        elif t in ('hello', 'start', 'usage', 'cleared'):
            print(' ', t, f if t != 'hello' else '')
        elif t == 'error':
            result['error'] = f.get('message')
            print('  ERROR:', result['error'])
            break
        elif t == 'done':
            result['done'] = True
            print('  done, finish=%s' % f.get('finish'))
            break
        elif t in ('tool_start', 'tool_result'):
            print(' ', t, json.dumps(f, ensure_ascii=False)[:100])
    result['deltas'] = deltas
    return result

print('=== connect ===')
s, extra = ws_connect()
print('handshake ok')
if extra:
    print('pending bytes after handshake:', len(extra))

print('\n=== turn 1: plain streaming (glm-5.3-flash) ===')
send_text(s, 'chat:用一句话介绍你自己')
r1 = drain_until_done(s, max_seconds=90)
print('  deltas=%d frames=%d done=%s' % (r1['deltas'], r1['frames'], r1['done']))
print('  text[:120]:', r1['text'][:120])

print('\n=== turn 2: tool loop ===')
send_text(s, 'chat:请调用 get_time 工具告诉我现在几点，然后用一句话总结')
r2 = drain_until_done(s, max_seconds=120)
print('  text[:120]:', r2['text'][:120])

print('\n=== turn 3: stop mid-stream ===')
send_text(s, 'chat:写一篇500字的散文')
time.sleep(3)
send_text(s, 'stop')
r3 = drain_until_done(s, max_seconds=30, print_deltas=False)
print('  stopped, deltas before stop=%d, done=%s, error=%s'
      % (r3['deltas'], r3['done'], r3['error']))

print('\n=== turn 4: after stop, chat still works ===')
send_text(s, 'chat:只回复两个字：好的')
r4 = drain_until_done(s, max_seconds=60)
print('  text:', r4['text'][:60])

ok = r1['done'] and r2['done'] and r4['done'] and r1['deltas'] > 3
print('\nRESULT:', 'PASS' if ok else 'FAIL')
sys.exit(0 if ok else 1)
