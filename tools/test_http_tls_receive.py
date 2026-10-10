"""Bounded HTTP/TLS complete-body receive-window regression, no load test."""
from __future__ import annotations

import argparse
from contextlib import closing
import hashlib
import http.client
import json
import os
from pathlib import Path
import shutil
import socket
import ssl
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = r'''
#include <xsbase.h>
#include <stdio.h>
#include <string.h>
XS_RequestResult RequestProc(XS_HttpReq* Req) {
    const xnetbuf* Buffer = Req->tls ? xrtTlsStreamBuffer(Req->tls) : xrtNetStreamBuffer(Req->tcp);
    size_t Available = xrtNetBufSize(Buffer), Offset = 0, Total = 0, Written;
    xsha256 Hash; uint8 Digest[32]; char Hex[65], Reply[512], Body[160];
    static const char Alphabet[] = "0123456789abcdef";
    xrtSha256Init(&Hash);
    if (Req->head->MethodCode == XHTTP_METHOD_POST) for (;;) {
        unsigned char Chunk[4096]; size_t Read = Available - Offset, Used = 0;
        xbytesview Input, Data = {0}; xhttp1errorinfo Error = {0}; xhttp1bodystatus Status;
        if (Read > sizeof(Chunk)) Read = sizeof(Chunk);
        if (xrtNetBufPeek(Buffer, Offset, Chunk, Read) != Read) return XS_FALLBACK;
        Input.Data = Chunk; Input.Size = Read;
        Status = xrtHttp1BodyRead(Req->body, Input, false, &Used, &Data, &Error);
        if (Used > Read || !xrtSha256Update(&Hash, Data.Data, Data.Size)) return XS_FALLBACK;
        Offset += Used; Total += Data.Size;
        if (Status == XHTTP1_BODY_DONE) break;
        if (Status == XHTTP1_BODY_ERROR || Status == XHTTP1_BODY_FIELDS || (!Used && !Data.Size)) return XS_FALLBACK;
    }
    if (!xrtSha256Final(&Hash, Digest)) return XS_FALLBACK;
    for (size_t i = 0; i < 32; ++i) { Hex[2*i] = Alphabet[Digest[i] >> 4]; Hex[2*i+1] = Alphabet[Digest[i] & 15]; }
    Hex[64] = 0;
    int Bytes = snprintf(Body, sizeof(Body), "{\"bytes\":%llu,\"sha256\":\"%s\"}", (unsigned long long)Total, Hex);
    int Count = snprintf(Reply, sizeof(Reply), "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", Bytes, Body);
    if (Count < 0 || (size_t)Count >= sizeof(Reply)) return XS_FALLBACK;
    if (Req->tls) return xrtTlsStreamSend(Req->tls, Reply, (size_t)Count, &Written) == XTLS_OK && Written == (size_t)Count ? XS_OK : XS_FALLBACK;
    return xrtNetStreamSend(Req->tcp, Reply, (size_t)Count) == XNET_RESULT_OK ? XS_OK : XS_FALLBACK;
}
'''


def port():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def connection(endpoint, secure):
    if not secure:
        return http.client.HTTPConnection("127.0.0.1", endpoint, timeout=4)
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    context.check_hostname, context.verify_mode = False, ssl.CERT_NONE  # committed loopback test certificate
    return http.client.HTTPSConnection("127.0.0.1", endpoint, timeout=4, context=context)


def check(exe: Path, window, body_limit, payload_size):
    with tempfile.TemporaryDirectory(prefix="xs-http-tls-receive-") as raw:
        site = Path(raw)
        (site / "web").mkdir()
        (site / "receiver.c").write_text(SCRIPT, encoding="utf-8")
        for kind in ("cert", "key"):
            shutil.copy2(ROOT / f"release/tls/xtps_{kind}.pem", site / f"{kind}.pem")
        plain, secure = port(), port()
        while plain == secure:
            secure = port()
        service = {"enabled": True, "class": "http", "name": "receive", "tls": True,
                   "ip": "127.0.0.1", "port": plain, "ip_tls": "127.0.0.1", "port_tls": secure,
                   "header_limit": 4096, "body_limit": body_limit,
                   "host_default": {"path": "web", "devlang": "c", "devfile": "receiver.c",
                                    "tls_cert": "cert.pem", "tls_key": "key.pem"}}
        if window is not None:
            service["recv_limit"] = window
        config = site / "xs.json"
        config.write_text(json.dumps({"services": [service]}), encoding="utf-8")
        with (site / "xs.log").open("wb") as log:
            process = subprocess.Popen([str(exe), str(config)], cwd=site, stdout=log, stderr=subprocess.STDOUT,
                                       creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0)
            try:
                deadline = time.monotonic() + 10
                while True:
                    assert process.poll() is None, (site / "xs.log").read_text(errors="replace")
                    try:
                        with closing(connection(secure, True)) as conn:
                            conn.request("GET", "/")
                            response = conn.getresponse()
                            assert response.status == 200, response.read()
                            response.read()
                        break
                    except OSError:
                        assert time.monotonic() < deadline, (site / "xs.log").read_text(errors="replace")
                        time.sleep(0.05)
                payload = (bytes(range(256)) * ((payload_size + 255) // 256))[:payload_size]
                for is_tls, endpoint in ((False, plain), (True, secure)):
                    with closing(connection(endpoint, is_tls)) as conn:
                        for chunked in (False, True):
                            data = [payload[i:i+17001] for i in range(0, len(payload), 17001)] if chunked else payload
                            conn.request("POST", "/", body=data, encode_chunked=chunked)
                            response = conn.getresponse()
                            actual = json.loads(response.read())
                            assert response.status == 200 and actual == {"bytes": len(payload), "sha256": hashlib.sha256(payload).hexdigest()}, actual
                            conn.request("GET", "/")
                            response = conn.getresponse()
                            assert response.status == 200 and json.loads(response.read())["bytes"] == 0
                    # A larger declared body is rejected from its header without
                    # sending an oversized file or filling the connection.
                    with closing(connection(endpoint, is_tls)) as conn:
                        conn.request("POST", "/", headers={"Content-Length": str(body_limit + 1)})
                        response = conn.getresponse()
                        assert response.status == 413, response.read()
                        response.read()
            finally:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill(); process.wait(timeout=5)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    exe = parser.parse_args().exe.resolve()
    check(exe, None, 512 * 1024, 300 * 1024)
    check(exe, 832 * 1024, 768 * 1024, 600 * 1024)
    check(exe, 17 * 1024, 12 * 1024, 12 * 1024)
    print("HTTP/TLS default/configured/small receive windows, fixed/chunked hash, keep-alive and body rejection: PASS")


if __name__ == "__main__":
    main()
