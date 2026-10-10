"""Packed-site reload: immutable VFS snapshots, disk overrides and rollback."""
from __future__ import annotations

import argparse
import http.client
import json
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
PROBE = r'''
#include <xsbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PROBE_VERSION "pack-v1"
XS_RequestResult RequestProc(XS_HttpReq* req) {
    char path[128], body[160], response[512];
    size_t n = req->head->Target.Size;
    int status = 200, length;
    XS_ReloadId id = 0;
    if (n >= sizeof(path)) return XS_FALLBACK;
    memcpy(path, req->head->Target.Data, n); path[n] = 0;
    if (!strcmp(path, "/revision")) snprintf(body, sizeof(body), "%s", PROBE_VERSION);
    else if (!strcmp(path, "/submit/all")) id = xsReloadAllSubmit();
    else if (!strcmp(path, "/submit/server")) id = xsReloadServerSubmit("t");
    else if (!strcmp(path, "/submit/host")) {
        XS_ServerInfo* server = xsServerFind("t");
        XS_HostInfo* host = server ? xsHostFind(server, req->host->Name) : NULL;
        id = host ? xsReloadHostSubmit(host) : 0;
        xsServerRelease(server);
    }
    else if (!strncmp(path, "/status/", 8)) {
        XS_ReloadResult result;
        XS_ReloadId ticket = 0;
        for (size_t i = 8; i < n; ++i) ticket = ticket * 10 + (unsigned)(path[i] - '0');
        if (!xsReloadQuery(ticket, &result)) {
            status = 404; strcpy(body, "unknown");
        } else {
            const char* state = result.State == XS_RELOAD_SUCCEEDED ? "succeeded" :
                (result.State == XS_RELOAD_FAILED || result.State == XS_RELOAD_SUPERSEDED ||
                 result.State == XS_RELOAD_CANCELLED) ? "failed" : "pending";
            snprintf(body, sizeof(body), "{\"state\":\"%s\",\"revision\":%llu}",
                state, (unsigned long long)result.Revision);
        }
    } else return XS_FALLBACK;
    if (!strncmp(path, "/submit/", 8)) {
        status = id ? 202 : 503;
        snprintf(body, sizeof(body), "%llu", (unsigned long long)id);
    }
    length = snprintf(response, sizeof(response),
        "HTTP/1.1 %d Test\r\nContent-Length: %u\r\n\r\n%s",
        status, (unsigned)strlen(body), body);
    xrtNetStreamSend(req->tcp, response, (size_t)length);
    return XS_OK;
}
'''


def get(port: int, path: str, host: str = "h0.local") -> tuple[int, str]:
    conn = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
    try:
        conn.request("GET", path, headers={"Host": host})
        response = conn.getresponse()
        return response.status, response.read().decode("utf-8")
    finally:
        conn.close()


def reload_site(port: int, kind: str, expected: str = "succeeded") -> int:
    status, body = get(port, "/submit/" + kind)
    assert status == 202 and int(body) > 0, (status, body)
    status_path = "/status/" + body
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        status, body = get(port, status_path)
        result = json.loads(body) if status == 200 else {}
        if result.get("state") in ("succeeded", "failed"):
            assert result["state"] == expected, result
            return int(result["revision"])
        time.sleep(0.03)
    raise AssertionError("reload did not reach a terminal state")


def check_hosts(port: int, expected: str) -> None:
    for i in range(4):
        assert get(port, "/revision", f"h{i}.local") == (200, expected)


def check(exe: Path) -> None:
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        port = sock.getsockname()[1]
    with tempfile.TemporaryDirectory(prefix="xs-packed-reload-") as temporary:
        root = Path(temporary)
        site, run = root / "site", root / "run"
        (site / "wwwroot").mkdir(parents=True)
        (run / "wwwroot").mkdir(parents=True)
        (site / "wwwroot/index.txt").write_text("packed static", encoding="utf-8")
        (site / "main.c").write_text(PROBE, encoding="utf-8")
        hosts = [{"enabled": True, "name": f"h{i}", "host": f"h{i}.local",
                  "path": "wwwroot", "devlang": "c", "devfile": "main.c"}
                 for i in range(4)]
        config = {"services": [{"enabled": True, "class": "http", "name": "t",
                                "ip": "127.0.0.1", "port": port,
                                "host_default": hosts[0], "hosts": hosts[1:]}]}
        (site / "xs.json").write_text(json.dumps(config), encoding="utf-8")
        packed = run / exe.name
        subprocess.run([str(exe), "pack", str(site), "-o", str(packed)],
                       check=True, capture_output=True, encoding="utf-8")
        shutil.copymode(exe, packed)
        with (root / "server.log").open("wb") as output:
            # Deliberately use a different CWD: snapshots must resolve at the app.
            process = subprocess.Popen([str(packed)], cwd=site, stdout=output,
                                       stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic() + 15
                while time.monotonic() < deadline:
                    try:
                        if get(port, "/revision") == (200, "pack-v1"):
                            break
                    except OSError:
                        pass
                    if process.poll() is not None:
                        raise AssertionError("packed process exited before startup")
                    time.sleep(0.05)
                check_hosts(port, "pack-v1")
                revisions = []
                for kind in ("all", "server", "host"):
                    revisions.append(reload_site(port, kind))
                    check_hosts(port, "pack-v1")
                    assert get(port, "/index.txt") == (200, "packed static")
                    print("PASS VFS-only reload:", kind, flush=True)
                assert len(set(revisions)) == 1 and revisions[0] != 0
                (run / "xs.json").write_text("{ broken config", encoding="utf-8")
                reload_site(port, "all", "failed")
                check_hosts(port, "pack-v1")
                config["probe_marker"] = "disk override"
                (run / "xs.json").write_text(json.dumps(config), encoding="utf-8")
                (run / "main.c").write_text(PROBE.replace('"pack-v1"', '"disk-v2"'), encoding="utf-8")
                revision = reload_site(port, "all")
                assert revision != revisions[0]
                check_hosts(port, "disk-v2")
                print("PASS disk config + source overrides", flush=True)
                (run / "main.c").write_text("not valid C", encoding="utf-8")
                reload_site(port, "server", "failed")
                check_hosts(port, "disk-v2")
                print("PASS malformed config/source preserve the active generation", flush=True)
                (run / "xs.json").unlink()
                (run / "main.c").unlink()
                assert reload_site(port, "all") == revisions[0]
                check_hosts(port, "pack-v1")
                print("PASS removal of disk overrides restores VFS snapshots", flush=True)
            except Exception:
                print((root / "server.log").read_text(encoding="utf-8", errors="replace"))
                raise
            finally:
                if process.poll() is None:
                    process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    args = parser.parse_args()
    check(args.exe.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
