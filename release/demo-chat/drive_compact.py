"""drive_compact - small-window auto compaction end-to-end check."""
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



def turn(s, text, timeout=240.0):
    frames = []
    send_text(s, "chat:" + text)
    while True:
        opcode, data = recv_frame(s, timeout)
        if data is None:
            break
        try:
            f = json.loads(data)
        except Exception:
            continue
        frames.append(f)
        if f.get("type") in ("done", "error"):
            break
    return frames


def main():
    s, _ = ws_connect()
    compacted = False
    compact_gen = None
    for i in range(1, 12):
        text = ("Remember this: project codename Aurora-%d, port %d. "
                % (i, 7000 + i)) * 100
        fs = []
        for attempt in range(8):
            more = turn(s, text)
            fs.extend(more)
            if not any(f["type"] == "error" and f.get("message") == "busy" for f in more):
                break
            time.sleep(8)
        tag = ""
        for f in fs:
            if f["type"] == "compacted":
                compacted = True
                compact_gen = f.get("generation")
                tag = "  <-- COMPACTED gen=%s tok=%s" % (
                    f.get("generation"), f.get("summary_tokens"))
            if f["type"] == "error":
                tag += "  ERR: " + str(f.get("message"))[:70]
        kinds = {}
        for f in fs:
            kinds[f["type"]] = kinds.get(f["type"], 0) + 1
            if f["type"] in ("compacted", "ladder"):
                print("     FRAME", json.dumps(f, ensure_ascii=False)[:160])
        print("     kinds:", kinds)
        u = next((f for f in reversed(fs) if f["type"] == "usage"), None)
        if u:
            print("turn %2d: in=%-5s out=%-4s ctx=%-5s/%-5s valid=%s%s" % (
                i, u["input"], u["output"], u["ctx_used"], u["ctx_max"],
                u.get("valid"), tag))
        else:
            print("turn %2d: NO USAGE FRAME%s" % (i, tag))
            return 1
        if compacted and i >= 10:
            break
    fs = turn(s, "Which project codename did I mention? Answer in one short line.")
    answer = "".join(f.get("text", "") for f in fs if f["type"] == "delta")
    print("recall answer:", answer.strip()[:140])
    print("COMPACTED =", compacted, "gen =", compact_gen,
          "| recall mentions Aurora:", "Aurora" in answer)
    s.close()
    return 0 if compacted else 1


if __name__ == "__main__":
    sys.exit(main())
