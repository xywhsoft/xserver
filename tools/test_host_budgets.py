"""Loopback tests for connection admission, WS memory rejection and recovery."""
from pathlib import Path
import argparse
import contextlib
import http.client
import json
import os
import signal
import socket
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
HTTP_SOURCE = r'''
#include <xsbase.h>
XS_RequestResult RequestProc(XS_HttpReq* req) {
    const char* reply="HTTP/1.1 200 OK\r\nContent-Length:2\r\n\r\nOK";
    return xrtNetStreamSend(req->tcp,reply,strlen(reply))==XNET_RESULT_OK?XS_OK:XS_FALLBACK;
}
'''
WS_SOURCE = r'''
#include <xsbase.h>
void WsBinary(XS_HostInfo* host,xwsstream* ws,xbytesview data) {
    (void)host;(void)xrtWsStreamSend(ws,XWS_OPCODE_BINARY,data);
}
'''


def free_port():
    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 0))
        return sock.getsockname()[1]


def exact(sock, size):
    data = b''
    while len(data) < size:
        part = sock.recv(size-len(data))
        if not part: raise EOFError('connection closed')
        data += part
    return data


def send_frame(sock, opcode, payload=b'', final=True):
    mask = b'test'
    head = bytes([(0x80 if final else 0) | opcode])
    head += bytes([0x80 | len(payload)]) if len(payload) < 126 else b'\xfe'+struct.pack('!H', len(payload))
    sock.sendall(head+mask+bytes(byte ^ mask[i % 4] for i, byte in enumerate(payload)))


def receive_frame(sock):
    first, second = exact(sock, 2)
    assert not second & 0x80
    size = second & 0x7f
    if size == 126: size = struct.unpack('!H', exact(sock, 2))[0]
    if size == 127: size = struct.unpack('!Q', exact(sock, 8))[0]
    assert size < 65536
    return first & 15, exact(sock, size)


def connect_ws(port):
    sock = socket.create_connection(('127.0.0.1', port), timeout=5)
    sock.sendall(b'GET / HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\n'
                 b'Connection: Upgrade\r\nSec-WebSocket-Version: 13\r\n'
                 b'Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n')
    head = b''
    while not head.endswith(b'\r\n\r\n'):
        head += exact(sock, 1)
        assert len(head) < 8192
    assert head.startswith(b'HTTP/1.1 101 '), head
    return sock


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    args = parser.parse_args()
    exe = args.exe.resolve()
    with tempfile.TemporaryDirectory(prefix='xs-host-budgets-') as raw:
        site = Path(raw)
        (site/'http.c').write_text(HTTP_SOURCE, encoding='utf-8')
        (site/'ws.c').write_text(WS_SOURCE, encoding='utf-8')
        http_port, ws_port = free_port(), free_port()
        while http_port == ws_port: ws_port = free_port()
        config = {'engine': {'workers': 2}, 'services': [
            {'class':'http','name':'http','ip':'127.0.0.1','port':http_port,
             'connection_limit':1,'devfile':'http.c'},
            {'class':'ws','name':'ws','ip':'127.0.0.1','port':ws_port,
             'ws_message_limit':1024,'ws_memory_limit':1024,'devfile':'ws.c'}]}
        path = site/'xs.json'
        path.write_text(json.dumps(config), encoding='utf-8')
        with (site/'host.log').open('w+', encoding='utf-8') as log:
            process = subprocess.Popen([str(exe), str(path)], stdout=log, stderr=log,
                creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if os.name == 'nt' else 0)
            try:
                deadline = time.monotonic()+15
                while True:
                    held = http.client.HTTPConnection('127.0.0.1', http_port, timeout=2)
                    try:
                        held.request('GET', '/')
                        response = held.getresponse()
                        assert response.status == 200 and response.read() == b'OK'
                        break
                    except OSError:
                        held.close()
                        if process.poll() is not None or time.monotonic() >= deadline: raise
                        time.sleep(.05)
                try:
                    with socket.create_connection(('127.0.0.1', http_port), timeout=3) as refused:
                        try:
                            refused.sendall(b'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n')
                            assert refused.recv(1024) == b'', 'over-limit connection accepted'
                        except ConnectionError: pass
                finally:
                    held.close()
                deadline = time.monotonic()+5
                while True:
                    recovered = http.client.HTTPConnection('127.0.0.1', http_port, timeout=2)
                    try:
                        recovered.request('GET', '/')
                        response = recovered.getresponse()
                        assert response.read() == b'OK'
                        break
                    except (OSError, http.client.HTTPException):
                        if time.monotonic() >= deadline: raise
                        time.sleep(.05)
                    finally: recovered.close()
                print('PASS connection budget rejects excess admission and recovers after close')

                with contextlib.ExitStack() as sockets:
                    first = sockets.enter_context(connect_ws(ws_port))
                    send_frame(first, 2, b'a'*800, final=False)
                    send_frame(first, 9, b'barrier')
                    assert receive_frame(first) == (10, b'barrier')
                    second = sockets.enter_context(connect_ws(ws_port))
                    send_frame(second, 2, b'b'*800)
                    opcode, payload = receive_frame(second)
                    assert opcode == 8 and struct.unpack('!H', payload[:2])[0] == 1013, (opcode, payload)
                    first.close()
                    time.sleep(.15)
                    third = sockets.enter_context(connect_ws(ws_port))
                    send_frame(third, 2, b'c'*800)
                    assert receive_frame(third) == (2, b'c'*800)
                print('PASS WS service memory budget closes with 1013 and releases reservations')
            except BaseException:
                log.flush(); log.seek(0); print(log.read())
                raise
            finally:
                if process.poll() is None:
                    process.send_signal(signal.CTRL_BREAK_EVENT if os.name == 'nt' else signal.SIGTERM)
                    try: process.wait(timeout=12)
                    except subprocess.TimeoutExpired:
                        process.kill(); process.wait(); raise AssertionError('host shutdown stalled')
                assert process.returncode == 0, process.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
