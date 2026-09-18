"""站点 VFS 端到端测试：pack → 无参启动 → HTTP 断言 → 磁盘覆盖 → --no-vfs。

python tools/test_site_vfs.py [--exe release/xs.exe]
覆盖 Phase 1+2 验收线：格式往返（list/extract/strip）、包内服务、
磁盘优先约束三场景、--no-vfs。全部使用临时目录与子进程，结束即清理。
"""
from __future__ import annotations

import argparse
import http.client
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PORT = 18731
PASSED = 0
FAILED = 0


def check(name: str, cond: bool, detail: str = "") -> None:
    global PASSED, FAILED
    if cond:
        PASSED += 1
        print(f"  PASS: {name}")
    else:
        FAILED += 1
        print(f"  FAIL: {name} {detail}")


def run_xs(exe: Path, *args: str, cwd: Path | None = None) -> subprocess.CompletedProcess:
    return subprocess.run([str(exe), *args], capture_output=True, text=True,
                         encoding="utf-8", errors="replace", timeout=120,
                         cwd=str(cwd or ROOT))


def http_get(path: str = "/", port: int = PORT) -> tuple[int, bytes]:
    try:
        conn = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
        conn.request("GET", path)
        resp = conn.getresponse()
        body = resp.read()
        conn.close()
        return resp.status, body
    except OSError:
        return 0, b""


def wait_up(port: int, timeout: float = 10.0) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        code, _ = http_get("/", port)
        if code != 0:
            return True
        time.sleep(0.3)
    return False


def wait_down(proc: subprocess.Popen) -> None:
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


def make_site(dir_: Path) -> None:
    www = dir_ / "wwwroot"
    www.mkdir(parents=True)
    (www / "index.html").write_text(
        "<html><body>pack-index</body></html>", encoding="utf-8")
    (www / "about.html").write_text(
        "<html><body>pack-about</body></html>", encoding="utf-8")
    (www / "big.js").write_text("var x = 1;\n" * 5000, encoding="utf-8")
    (dir_ / "xs.json").write_text(
        '{"services":[{"enabled":true,"class":"http","name":"t",'
        f'"ip":"127.0.0.1","port":{PORT},'
        '"host_default":{"enabled":true,"name":"h","path":"wwwroot"}}]}',
        encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", default="release/xs.exe")
    args = parser.parse_args()
    exe = ROOT / args.exe

    with tempfile.TemporaryDirectory(prefix="xs-vfs-test-") as tmp:
        tmp_path = Path(tmp)
        site = tmp_path / "site"
        make_site(site)

        # ---- 1. pack ----
        packed = tmp_path / "packed.exe"
        r = run_xs(exe, "pack", str(site), "-o", str(packed))
        check("pack exits 0", r.returncode == 0, r.stdout + r.stderr)
        check("pack output exists", packed.exists())

        # ---- 2. --list ----
        r = run_xs(exe, "pack", "--list", str(packed))
        check("list exits 0", r.returncode == 0, r.stderr)
        check("list shows 4 entries",
              "4 entries" in r.stdout and "wwwroot/index.html" in r.stdout,
              r.stdout)

        # ---- 3. --extract round-trip ----
        out = tmp_path / "extracted"
        r = run_xs(exe, "pack", "--extract", str(packed), "-d", str(out))
        check("extract exits 0", r.returncode == 0, r.stderr)
        same = ((out / "wwwroot" / "index.html").read_text(encoding="utf-8")
                == (site / "wwwroot" / "index.html").read_text(encoding="utf-8"))
        check("extract round-trip identical", same)

        # ---- 4. --strip identity ----
        stripped = tmp_path / "stripped.exe"
        r = run_xs(exe, "pack", "--strip", str(packed), "-o", str(stripped))
        check("strip exits 0", r.returncode == 0, r.stderr)
        check("strip byte-identical to original",
              stripped.read_bytes() == exe.read_bytes())

        # ---- 5. 无参启动：包内服务 ----
        rundir = tmp_path / "run"
        rundir.mkdir()
        (rundir / "wwwroot").mkdir()          # 空目录：pRoot 成功打开，
        runexe = rundir / "app.exe"            # 运行期投递文件可被磁盘层看到
        shutil.copy2(packed, runexe)
        proc = subprocess.Popen([str(runexe)], cwd=str(rundir),
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, encoding="utf-8", errors="replace")
        try:
            check("server up from pack", wait_up(PORT))
            code, body = http_get("/index.html")
            check("GET /index.html 200", code == 200)
            check("content from pack", b"pack-index" in body)
            code, body = http_get("/about.html")
            check("GET /about.html 200", code == 200 and b"pack-about" in body)
            code, body = http_get("/big.js")
            check("GET /big.js 200 (large asset)",
                  code == 200 and len(body) >= 54000)
            code, _ = http_get("/missing.html")
            check("missing 404", code == 404)
            code, _ = http_get("/../xs.json")
            check("traversal blocked", code in (403, 404))

            # ---- 6. 磁盘优先：放覆盖文件 ----
            www = rundir / "wwwroot"
            (www / "index.html").write_text(
                "<html><body>disk-wins</body></html>", encoding="utf-8")
            time.sleep(0.5)
            code, body = http_get("/index.html")
            check("disk override wins", code == 200 and b"disk-wins" in body)
            code, body = http_get("/about.html")
            check("pack fallback for others",
                  code == 200 and b"pack-about" in body)

            # ---- 7. 删覆盖恢复包内 ----
            (www / "index.html").unlink()
            time.sleep(0.5)
            code, body = http_get("/index.html")
            check("pack restored after override removed",
                  code == 200 and b"pack-index" in body)
        finally:
            wait_down(proc)

        # ---- 8. --no-vfs ----
        proc = subprocess.Popen([str(runexe), "--no-vfs"], cwd=str(rundir),
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, encoding="utf-8", errors="replace")
        try:
            time.sleep(2)
            output = proc.stdout.read() if proc.stdout is not None else ""
        finally:
            wait_down(proc)
        # 磁盘上无 xs.json → 应走磁盘路径并失败（而非读包）
        check("--no-vfs ignores pack",
              proc.returncode not in (0,) and "app pack" not in (output or ""),
              output or "")

    print(f"\n{PASSED} pass, {FAILED} fail")
    return 1 if FAILED > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
