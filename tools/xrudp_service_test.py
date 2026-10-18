"""Real xs xrudp service/SDK processes, private loopback endpoints and keys.

Tests native byte/message/game traffic, authorization, retained generations,
atomic reload rejection, reentrant abort, IPv6 and complete engine shutdown.
No installed release files or external network services are modified.
"""
from __future__ import annotations

import argparse
import copy
import json
import os
from pathlib import Path
import re
import shutil
import signal
import socket
import subprocess
import tempfile
import time

from lifecycle_reload_test import free_port, submit_reload, wait_port, wait_reload

ROOT = Path(__file__).resolve().parent.parent


def public(private: bytes) -> bytes:
    """Independent Montgomery ladder for synthetic test pins only (RFC 7748)."""
    k = int.from_bytes(private, "little") & ((1 << 255) - 8) | (1 << 254)
    p = 2**255 - 19
    x, x2, z2, x3, z3, swap = 9, 1, 0, 9, 1, 0
    for bit in reversed(range(255)):
        b = k >> bit & 1
        swap ^= b
        if swap:
            x2, x3, z2, z3 = x3, x2, z3, z2
        swap = b
        a, b = (x2 + z2) % p, (x2 - z2) % p
        aa, bb = a * a % p, b * b % p
        e = (aa - bb) % p
        c, d = (x3 + z3) % p, (x3 - z3) % p
        da, cb = d * a % p, c * b % p
        x3, z3 = (da + cb) ** 2 % p, x * (da - cb) ** 2 % p
        x2, z2 = aa * bb % p, e * (aa + 121665 * e) % p
    if swap:
        x2, z2 = x3, z3
    return (x2 * pow(z2, p - 2, p) % p).to_bytes(32, "little")


SERVER = r'''
#include <xsbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char marker;
static bool deny;
static void check(int ok) { if(!ok) { fprintf(stderr,"SERVER_ASSERT\n"); fflush(stderr); abort(); } }
void ServiceInit(XS_HostInfo* h) {
    int64 n=0; check(xrtValueGetInt(xrtValueObjectGet(h->Custom,XRT_STR_LITERAL("marker")),&n)); marker=(unsigned char)n;
    xvalue* d=xrtValueObjectGet(h->Custom,XRT_STR_LITERAL("deny")); if(d) check(xrtValueGetBool(d,&deny));
}
void ServiceUnit(XS_HostInfo* h) { (void)h; printf("RUDP_UNIT %u\n",marker); fflush(stdout); }
void RudpReady(XS_HostInfo* h,xrudpendpoint* e) {
    xrudpendpointinfo i; (void)h; check(xrtRudpEndpointInfo(e,&i)==XRUDP_OK);
    printf("RUDP_READY %u %u\n",marker,i.Local.Port); fflush(stdout);
}
bool RudpAuthorize(XS_HostInfo* h,const unsigned char* k,const xrudphandshakeinfo* i) {
    (void)h; (void)k; (void)i; return !deny;
}
void RudpOpen(XS_HostInfo* h,xrudppeer* p) {
    size_t n; xrudpdatagramchannel c; (void)h;
    xrtRudpDatagramChannelInit(&c); c.Id=7; c.Mode=XRUDP_DATAGRAM_LATEST;
    check(xrtRudpPeerDatagramConfigure(p,&c)==XRUDP_OK);
    check(xrtRudpPeerWrite(p,&marker,1,&n)==XRUDP_OK && n==1);
    printf("RUDP_OPEN %u\n",marker); fflush(stdout);
}
void RudpClose(XS_HostInfo* h,xrudppeer* p,xrudpresult r) {
    (void)h; (void)p; printf("RUDP_CLOSE %u %u\n",marker,(unsigned)r); fflush(stdout);
}
void RudpChanged(XS_HostInfo* h,xrudppeer* p,uint32_t flags) {
    unsigned char b[256]; size_t n,w; xrudpresult r; (void)h;
    if(flags&XRUDP_PEER_READ_READY) {
        while((r=xrtRudpPeerRead(p,b,sizeof(b),&n))==XRUDP_OK && n) {
            if(b[0]=='!') { check(xrtRudpPeerAbort(p)==XRUDP_OK); return; }
            check(xrtRudpPeerWrite(p,b,n,&w)==XRUDP_OK && n==w);
        }
    }
    if(flags&XRUDP_PEER_DATAGRAM_READ_READY) {
        xrudpdatagraminfo i;
        while(xrtRudpPeerDatagramRead(p,b,sizeof(b),&i)==XRUDP_OK) {
            uint64_t sequence; check(xrtRudpPeerDatagramWrite(p,7,b,i.Size,0,&sequence)==XRUDP_OK);
        }
    }
}
void RudpStreamChanged(XS_HostInfo* h,xrudppeer* p,const xrudpstreamevent* e) {
    unsigned char b[256]; size_t n; (void)h;
    if(e->Id==1) return;
    while(xrtRudpPeerMessageRead(p,e->Id,b,sizeof(b),&n)==XRUDP_OK) check(xrtRudpPeerMessageWrite(p,e->Id,b,n)==XRUDP_OK);
}
'''

CLIENT = r'''
#include <xsbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) {fprintf(stderr,"CLIENT_ASSERT line %d\n",__LINE__); fflush(stderr); exit(3);} } while(0)
static xnetengine* engine;
static xrudpendpoint* endpoint;
static xrudppeer* peer;
static xatomic32 bits;
static size_t received;
static uint32_t stream;
static const unsigned char serverPublic[32]={@PUBLIC@};
static unsigned char privateKey[32]={@PRIVATE@};
static unsigned char scope[16]={@SCOPE@};
static unsigned char application[32]={@APPLICATION@};
static void open_(xrudpendpoint* e,void* ctx) {(void)e;(void)ctx;xrtAtomic32FetchOr(&bits,1,XMEMORY_RELEASE);}
static void closed_(xrudpendpoint* e,xrudpresult r,const xerror* err,void* ctx) {
    (void)e;(void)r;(void)err;(void)ctx;xrtAtomic32FetchOr(&bits,64,XMEMORY_RELEASE);
}
static void peerClosed(xrudpendpoint* e,xrudppeer* p,xrudpresult r,void* ctx) {
    (void)e;(void)p;(void)r;(void)ctx;xrtAtomic32FetchOr(&bits,32,XMEMORY_RELEASE);
}
static void confirmed(xrudpendpoint* e,xrudppeer* p,void* ctx) {
    size_t n; uint64_t sequence; xrudpstreamconfig c; xrudpdatagramchannel d; (void)e;(void)ctx;
    if(@MODE@==2) return;
    if(@MODE@==3) {CHECK(xrtRudpPeerWrite(p,"!",1,&n)==XRUDP_OK && n==1); return;}
    xrtRudpDatagramChannelInit(&d); d.Id=7; d.Mode=XRUDP_DATAGRAM_LATEST;
    CHECK(xrtRudpPeerDatagramConfigure(p,&d)==XRUDP_OK);
    CHECK(xrtRudpPeerWrite(p,"abc",3,&n)==XRUDP_OK && n==3);
    xrtRudpStreamConfigInit(&c); c.Kind=XRUDP_STREAM_MESSAGES;
    CHECK(xrtRudpPeerStreamOpen(p,&c,&stream)==XRUDP_OK);
    CHECK(xrtRudpPeerMessageWrite(p,stream,"message",7)==XRUDP_OK);
    CHECK(xrtRudpPeerDatagramWrite(p,7,"game",4,0,&sequence)==XRUDP_OK);
}
static void changed(xrudpendpoint* e,xrudppeer* p,uint32_t flags,void* ctx) {
    unsigned char b[128]; size_t n; xrudpdatagraminfo i; (void)e;(void)ctx;
    if(flags&XRUDP_PEER_READ_READY) while(xrtRudpPeerRead(p,b,sizeof(b),&n)==XRUDP_OK && n) {
        const unsigned char expected[]={@MARKER@,'a','b','c','d','e','f'};
        CHECK(@MODE@!=2);
        if(@MODE@==3) continue;
        CHECK(received+n<=sizeof(expected)); CHECK(memcmp(b,expected+received,n)==0); received+=n;
        if(received>=4) xrtAtomic32FetchOr(&bits,2,XMEMORY_RELEASE);
        if(received==7) xrtAtomic32FetchOr(&bits,16,XMEMORY_RELEASE);
    }
    if(flags&XRUDP_PEER_DATAGRAM_READ_READY) while(xrtRudpPeerDatagramRead(p,b,sizeof(b),&i)==XRUDP_OK) {
        CHECK(i.Size==4 && memcmp(b,"game",4)==0); xrtAtomic32FetchOr(&bits,8,XMEMORY_RELEASE);
    }
}
static void streamChanged(xrudpendpoint* e,xrudppeer* p,const xrudpstreamevent* ev,void* ctx) {
    char b[128]; size_t n; (void)e;(void)ctx;
    if(ev->Id==stream && xrtRudpPeerMessageRead(p,stream,b,sizeof(b),&n)==XRUDP_OK) {
        CHECK(n==7 && memcmp(b,"message",7)==0); xrtAtomic32FetchOr(&bits,4,XMEMORY_RELEASE);
    }
}
static const xrudpendpointevents events={open_,confirmed,peerClosed,closed_,changed,streamChanged};
static void bind_(xnetworker* w,ptr data) {
    xrudpendpointconfig c; xnetaddr a; (void)data;
    xrtRudpEndpointConfigInit(&c); c.StaticPrivate=privateKey; c.HandshakeTimeoutUs=2000000;
    c.Session.StreamCapacity=c.Session.Local.MaxStreams=3; c.Session.Local.MaxMessageSize=1024;
    c.Session.Local.IdleTimeoutMs=2000;
    xrtRudpDatagramConfigInit(&c.Session.Datagrams);
    CHECK(xrtNetAddrParse(&a,"@LOCAL@",0));
    CHECK(xrtRudpEndpointBind(w,&a,&c,&events,NULL,&endpoint)==XRUDP_OK);
}
static void connect_(xnetworker* w,ptr data) {
    xrudpconnectconfig c={0}; xnetaddr a; (void)w;(void)data;
    c.PeerPublic=serverPublic; c.Scope=scope; c.ApplicationContext=application;
    CHECK(xrtNetAddrParse(&a,"@REMOTE@",@PORT@));
    CHECK(xrtRudpEndpointConnect(endpoint,&a,&c,&peer)==XRUDP_OK);
}
static void again(xnetworker* w,ptr data) {
    size_t n; (void)w;(void)data; CHECK(xrtRudpPeerWrite(peer,"def",3,&n)==XRUDP_OK && n==3);
}
static void wait_(uint32_t mask) {
    double end=xrtTimer()+10;
    while((xrtAtomic32Load(&bits,XMEMORY_ACQUIRE)&mask)!=mask) {CHECK(xrtTimer()<end);xrtSleep(2);}
}
void ServiceInit(XS_HostInfo* h) {
    xnetengineconfig c; double end; (void)h;
    xrtAtomic32Init(&bits,0);xrtNetEngineConfigInit(&c);c.Workers=1;
    engine=xrtNetEngineCreate(&c);CHECK(engine!=NULL && xrtNetEngineStart(engine));
    CHECK(xrtNetEnginePost(engine,0,bind_,NULL));wait_(1);
    CHECK(xrtNetEnginePost(engine,0,connect_,NULL));
    if(@MODE@==2 || @MODE@==3) wait_(32);
    else {
        wait_(14);printf("CLIENT_READY\n");fflush(stdout);
        if(@MODE@==1 || @MODE@==4) {
            end=xrtTimer()+20;
            while(!xrtPathExists("@CONTINUE@")) {CHECK(xrtTimer()<end);xrtSleep(5);}
            if(@MODE@==4) wait_(32);
            else {CHECK(xrtNetEnginePost(engine,0,again,NULL));wait_(16);}
        }
    }
    CHECK(xrtRudpEndpointClose(endpoint)==XRUDP_OK);wait_(64);
    if(peer) xrtRudpPeerDestroy(peer);xrtRudpEndpointDestroy(endpoint);
    CHECK(xrtNetEngineStop(engine));xrtNetEngineDestroy(engine);
    printf("CLIENT_PASS\n");fflush(stdout);exit(0);
}
'''


def wait_text(path: Path, text: str, process: subprocess.Popen, timeout=15) -> str:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        output = path.read_text(encoding="utf-8", errors="replace")
        if text in output:
            return output
        if process.poll() is not None:
            raise AssertionError(f"process exited {process.returncode}: {output[-4000:]}")
        time.sleep(0.02)
    raise AssertionError(f"missing {text}: {output[-4000:]}")


class Process:
    def __init__(self, exe: Path, config: Path, log: Path):
        self.log = log
        self.output = log.open("w", encoding="utf-8")
        self.process = subprocess.Popen([str(exe), str(config)], cwd=config.parent,
            stdout=self.output, stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0)

    def stop(self):
        if self.process.poll() is None:
            self.process.send_signal(signal.CTRL_BREAK_EVENT if os.name == "nt" else signal.SIGTERM)
        try:
            self.process.wait(timeout=20)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.wait(timeout=5)
            raise AssertionError("xs engine did not drain")
        finally:
            self.output.close()

    def passed(self):
        assert self.process.wait(timeout=15) == 0, self.log.read_text(errors="replace")[-4000:]
        assert "CLIENT_PASS" in self.log.read_text()
        self.output.close()


def array(value: bytes) -> str:
    return ",".join(str(n) for n in value)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--default-exe", type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="xs-xrudp-service-") as temp:
        app = Path(temp)
        exe = app / args.exe.name
        shutil.copy2(args.exe, exe)
        (app / "server.c").write_text(SERVER, encoding="utf-8")
        shutil.copy2(ROOT / "release/script/http_main.c", app / "admin.c")
        private = bytes([49]) * 32
        client_private = bytes([50]) * 32
        assert public(bytes.fromhex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a")).hex() == "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a"
        (app / "key.bin").write_bytes(private)
        scope = bytes([0x71]) * 16
        admin_port = free_port()
        settings = dict(key_file="key.bin", peer_public=public(client_private).hex(), scope=scope.hex(),
                        max_peers=8, max_half_open=4, memory_limit=33554432, streams=3,
                        message_limit=1024, datagram_channels=16, handshake_timeout_ms=2000, idle_timeout_ms=2000)
        config = {"engine": {"workers": 2}, "services": [
            dict(name="main", enabled=True, **{"class": "http"}, ip="127.0.0.1", port=admin_port, devfile="admin.c"),
            dict(name="rudp", enabled=True, **{"class": "xrudp"}, ip="127.0.0.1", port=0,
                 devfile="server.c", xrudp=settings, host_default={"name": "app", "marker": 1})]}
        config_path = app / "xs.json"

        def save():
            config_path.write_text(json.dumps(config), encoding="utf-8")

        def reload(expected="succeeded"):
            save()
            result = wait_reload(admin_port, submit_reload(admin_port, "/reload-all"))
            assert result["status"] == expected, result

        clients = []

        def client(port, marker=1, mode=0, key=client_private, remote="127.0.0.1", application=bytes(32)):
            index = len(clients)
            continuation = app / f"continue-{index}"
            text = CLIENT
            substitutions = dict(PUBLIC=array(public(private)), PRIVATE=array(key), SCOPE=array(scope),
                                 APPLICATION=array(application), MODE=str(mode), MARKER=str(marker),
                                 LOCAL="::1" if ":" in remote else "127.0.0.1", REMOTE=remote,
                                 PORT=str(port), CONTINUE=continuation.as_posix())
            for name, value in substitutions.items():
                text = text.replace(f"@{name}@", value)
            script = app / f"client-{index}.c"
            script.write_text(text, encoding="utf-8")
            cfg = app / f"client-{index}.json"
            cfg.write_text(json.dumps({"engine": {"workers": 1}, "services": [dict(name="client", enabled=True, **{"class": "custom"}, devfile=script.name)]}), encoding="utf-8")
            process = Process(exe, cfg, app / f"client-{index}.log")
            clients.append(process)
            return process, continuation

        save()
        service = Process(exe, config_path, app / "service.log")
        try:
            output = wait_text(service.log, "RUDP_READY 1 ", service.process)
            port = int(re.search(r"RUDP_READY 1 (\d+)", output)[1])
            wait_port(admin_port)
            held, continuation = client(port, mode=1)
            wait_text(held.log, "CLIENT_READY", held.process)
            config["services"][1]["host_default"]["marker"] = 2
            reload()
            wait_text(service.log, f"RUDP_READY 2 {port}", service.process)
            assert "RUDP_UNIT 1" not in service.log.read_text()
            fresh, _ = client(port, marker=2)
            fresh.passed()
            continuation.write_text("go")
            held.passed()
            wait_text(service.log, "RUDP_UNIT 1", service.process)
            # Valid settings, incompatible live socket identity/budget: rollback.
            settings["streams"] = 4
            reload("failed")
            settings["streams"] = 3
            for mutation in [{"bad_field": 1}, {"streams": True}, {"scope": "00" * 16}, {"datagram_channels": 0}, {"key_file": "missing"}]:
                previous = copy.deepcopy(settings)
                settings.update(mutation)
                reload("failed")
                settings.clear(); settings.update(previous)
            for invalid_key in [b"", bytes([49])*31, bytes([49])*33, bytes(32)]:
                (app / "key.bin").write_bytes(invalid_key)
                reload("failed")
            (app / "key.bin").write_bytes(bytes([52])*32)
            reload("failed")  # valid new private key cannot rekey a live socket
            (app / "key.bin").write_bytes(private)
            fresh, _ = client(port, marker=2)
            fresh.passed()
            # A new candidate's asynchronous UDP bind failure must not close
            # the retained service or strand a generation/engine reference.
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as occupied:
                occupied.bind(("127.0.0.1", 0))
                candidate = copy.deepcopy(config["services"][1])
                candidate["name"] = "blocked"
                candidate["port"] = occupied.getsockname()[1]
                config["services"].append(candidate)
                reload("failed")
                config["services"].pop()
            denied, _ = client(port, mode=2, key=bytes([51])*32)
            denied.passed()
            config["services"][1]["host_default"]["deny"] = True
            reload()
            denied, _ = client(port, mode=2)
            denied.passed()
            config["services"][1]["host_default"].pop("deny")
            pin = settings.pop("peer_public")
            reload()
            authorized, _ = client(port, marker=2)
            authorized.passed()
            settings["peer_public"] = pin
            settings["application_context"] = "aa" * 32
            reload()
            denied, _ = client(port, mode=2)
            denied.passed()
            settings.pop("application_context")
            reload()
            aborted, _ = client(port, mode=3)
            aborted.passed()
            # Native IPv6 uses a distinct v6-only socket, real Noise+all traffic.
            config["services"][1]["ip"] = "::1"
            config["services"][1]["port"] = free_port()
            ipv6_port = config["services"][1]["port"]
            reload()
            v6, _ = client(ipv6_port, marker=2, remote="::1")
            v6.passed()
            closing, continuation = client(ipv6_port, marker=2, mode=4, remote="::1")
            wait_text(closing.log, "CLIENT_READY", closing.process)
            config["services"][1]["enabled"] = False
            reload()
            continuation.write_text("closed")
            closing.passed()
            print("XRUDP SERVICE PASS: v4/v6 bytes/messages/game, pins/context, reentrant abort, retained reload, rollback, disable")
        except BaseException:
            print("SERVICE LOG", service.log.read_text(errors="replace")[-7000:])
            for c in clients:
                print(c.log.name, c.log.read_text(errors="replace")[-2000:])
            raise
        finally:
            for c in clients:
                c.stop()
            service.stop()
        output = service.log.read_text(errors="replace")
        assert service.process.returncode == 0 and "engine stopped" in output and "[xs] bye" in output, output[-5000:]
        if args.default_exe:
            plain = app / ("default" + (".exe" if os.name == "nt" else ""))
            shutil.copy2(args.default_exe, plain)
            config["services"][1]["enabled"] = True
            save()
            result = subprocess.run([str(plain), str(config_path)], cwd=app, capture_output=True, timeout=20)
            assert result.returncode != 0 and b"requires an xs build with the xrudp extension" in result.stdout, result.stdout[-2000:]
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
