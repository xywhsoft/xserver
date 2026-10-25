"""站点包解包器校验测试:恶意/损坏归档必须在解析期拒绝,不得越界读写。

python tools/test_pack_validate.py [--exe release/xs.exe]

覆盖(xs_pack.h XS_PackParse 条目级校验,与运行时 XS_VfsLoad 对齐):
- 正向:正常包 pack → extract 往返一致(校验不误伤合法归档)
- 条目路径逃逸(../)→ "unsafe entry path",目标目录外无文件
- 条目数据范围越界(DataOffset 超出数据区)→ "entry data out of range"
- STORE 条目 CompSize != OrigSize → "store size mismatch"
- 归档布局非法(dataOffset >= indexOffset)→ "archive layout invalid"

样本构造:对正常包做等长字节替换后重算 indexCrc(header+40)与
headerCrc(trailer+16,覆盖 trailer 前 16B + header 64B),模拟
"校验和自洽的恶意构造"(CRC32 不是对抗边界)。
"""
from __future__ import annotations

import argparse
import json
import socket
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PASSED = 0
FAILED = 0

TRAILER_MAGIC = b"XSVPACK\x00"
HEADER_MAGIC = b"XSVFHDR\x00"
TRAILER_SIZE = 24
HEADER_SIZE = 64


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


def make_site(dir_: Path) -> None:
    www = dir_ / "wwwroot"
    www.mkdir(parents=True)
    (www / "a.txt").write_text("hello-pack-validate\n", encoding="utf-8")
    (dir_ / "main.c").write_text(
        "#include <xsbase.h>\n"
        "XS_RequestResult RequestProc(XS_HttpReq* req)\n"
        "{ (void)req; return XS_FALLBACK; }\n", encoding="utf-8")
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        port = sock.getsockname()[1]
    (dir_ / "xs.json").write_text(
        json.dumps({"services": [{"enabled": True, "class": "http", "name": "t",
                   "ip": "127.0.0.1", "port": port,
                   "host_default": {"enabled": True, "name": "h", "host": "h.local",
                                    "path": "wwwroot", "devlang": "c",
                                    "devfile": "main.c"}}]}), encoding="utf-8")


class Archive:
    """站点包字节视图:trailer/header/索引定位与 CRC 重算。"""

    def __init__(self, data: bytearray):
        self.data = data
        assert bytes(data[-8:]) == b"XSVPACK\x00"[-8:] or bytes(data[-
            TRAILER_SIZE:-TRAILER_SIZE + 8]) == TRAILER_MAGIC
        self.header_off = struct.unpack_from("<Q", data, len(data) - 16)[0]
        assert bytes(data[self.header_off:self.header_off + 8]) == HEADER_MAGIC
        (self.version, self.hdr_size, self.count) = struct.unpack_from(
            "<HHI", data, self.header_off + 8)
        (self.data_off, self.index_off, self.archive_size) = struct.unpack_from(
            "<QQQ", data, self.header_off + 16)
        self.index_crc = struct.unpack_from("<I", data, self.header_off + 40)[0]
        self.index_len = self.archive_size - TRAILER_SIZE - self.index_off
        self.index_abs = self.header_off + self.index_off

    def entries(self) -> list[dict]:
        out = []
        pos = 0
        idx = self.data[self.index_abs:self.index_abs + self.index_len]
        for _ in range(self.count):
            (nrel,) = struct.unpack_from("<H", idx, pos)
            pos += 2
            path = bytes(idx[pos:pos + nrel]).decode("utf-8")
            pos += nrel
            method, flags = idx[pos], idx[pos + 1]
            pos += 2
            doff, csize, osize, ocrc = struct.unpack_from("<QIQI", idx, pos)
            pos += 24
            out.append({"pos": pos - (26 + nrel), "nrel": nrel, "path": path,
                        "method": method, "flags": flags, "doff": doff,
                        "csize": csize, "osize": osize, "ocrc": ocrc})
        return out

    def patch_entry(self, e: dict, **kw) -> None:
        base = self.index_abs + e["pos"] + e["nrel"]
        if "method" in kw:
            self.data[base] = kw["method"]
        if "doff" in kw:
            struct.pack_into("<Q", self.data, base + 2, kw["doff"])
        if "csize" in kw:
            struct.pack_into("<I", self.data, base + 10, kw["csize"])
        if "osize" in kw:
            struct.pack_into("<Q", self.data, base + 14, kw["osize"])

    def patch_path(self, e: dict, new_path: bytes) -> None:
        assert len(new_path) == e["nrel"], "path replacement must keep length"
        off = self.index_abs + e["pos"]
        self.data[off:off + e["nrel"]] = new_path

    def patch_header(self, **kw) -> None:
        if "data_off" in kw:
            struct.pack_into("<Q", self.data, self.header_off + 16, kw["data_off"])

    def refresh_crcs(self) -> None:
        index = self.data[self.index_abs:self.index_abs + self.index_len]
        struct.pack_into("<I", self.data, self.header_off + 40, zlib.crc32(index) & 0xFFFFFFFF)
        trailer_crc_buf = (bytes(self.data[len(self.data) - TRAILER_SIZE:
                                          len(self.data) - TRAILER_SIZE + 16])
                           + bytes(self.data[self.header_off:self.header_off + HEADER_SIZE]))
        struct.pack_into("<I", self.data, len(self.data) - 8,
                         zlib.crc32(trailer_crc_buf) & 0xFFFFFFFF)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", default="release/xs.exe")
    args = parser.parse_args()
    exe = ROOT / args.exe

    with tempfile.TemporaryDirectory(prefix="xs-pack-validate-") as tmp:
        tmp_path = Path(tmp)
        site = tmp_path / "site"
        make_site(site)

        packed = tmp_path / "good.exe"
        r = run_xs(exe, "pack", str(site), "-o", str(packed))
        check("pack exits 0", r.returncode == 0, r.stdout + r.stderr)
        good = Archive(bytearray(packed.read_bytes()))
        check("good archive parses",
              good.count == 3 and good.version == 1, f"count={good.count}")
        entries = good.entries()
        by_path = {e["path"]: e for e in entries}
        check("wwwroot/a.txt is STORE method",
              by_path.get("wwwroot/a.txt", {}).get("method") == 0,
              str(by_path))

        # ---- 正向:校验不误伤合法归档 ----
        out = tmp_path / "out-good"
        r = run_xs(exe, "pack", "--extract", str(packed), "-d", str(out))
        check("extract good exits 0", r.returncode == 0, r.stdout + r.stderr)
        check("extract good round-trip",
              (out / "wwwroot" / "a.txt").read_text(encoding="utf-8")
              == "hello-pack-validate\n")

        def malicious(name: str, mutate, err_hint: str) -> None:
            data = bytearray(packed.read_bytes())
            arc = Archive(data)
            mutate(arc)
            arc.refresh_crcs()
            bad = tmp_path / f"bad-{name}.exe"
            bad.write_bytes(data)
            outdir = tmp_path / f"out-{name}"
            r = run_xs(exe, "pack", "--extract", str(bad), "-d", str(outdir))
            check(f"{name}: extract rejected", r.returncode != 0,
                  f"rc={r.returncode} out={r.stdout}{r.stderr}")
            check(f"{name}: error message",
                  err_hint in (r.stdout + r.stderr), r.stdout + r.stderr)
            written = list(outdir.rglob('*')) if outdir.exists() else []
            check(f"{name}: nothing extracted", not written, str(written[:5]))

        def swap_escape_path(a: Archive) -> None:
            e = next(e for e in a.entries() if e["path"] == "wwwroot/a.txt")
            a.patch_path(e, b"../evil-a.txt")

        # ---- 条目路径逃逸(等长替换,CRC 自洽) ----
        malicious("escape", swap_escape_path, "unsafe entry path")

        # ---- 条目数据范围越界:DataOffset 指到索引区之外 ----
        malicious("range",
                  lambda a: a.patch_entry(a.entries()[0],
                                          doff=a.index_off + 4096),
                  "entry data out of range")
        malicious("range-wrap",
                  lambda a: a.patch_entry(a.entries()[0], doff=(1 << 64) - 4),
                  "entry data out of range")

        def nul_path(a: Archive) -> None:
            e = next(e for e in a.entries() if e['path'] == 'wwwroot/a.txt')
            a.patch_path(e, b'wwwroot/\x00.txt')
        malicious('nul-path', nul_path, 'unsafe entry path')

        def unsorted(a: Archive) -> None:
            e = next(e for e in a.entries() if e['path'] == 'wwwroot/a.txt')
            a.patch_path(e, b'aaaaaaaaaaaaa')
        malicious('unsorted', unsorted, 'entry paths not strictly sorted')

        # ---- STORE 大小不一致(memcpy 长度与数据区脱钩) ----
        def bump_store(a: Archive) -> None:
            e = next(e for e in a.entries() if e["method"] == 0)
            a.patch_entry(e, csize=e["csize"] + 8)
        malicious("storesize", bump_store, "store size mismatch")

        # ---- 归档布局非法:dataOffset >= indexOffset ----
        malicious("layout",
                  lambda a: a.patch_header(data_off=a.index_off + 8),
                  "archive layout invalid")

        # A failed pack must preserve an existing destination, including failures
        # before an output stream has been opened.
        preserved = tmp_path / 'preserved.exe'
        preserved.write_bytes(b'previous valid output')
        broken_site = tmp_path / 'broken-site'
        broken_site.mkdir()
        (broken_site/'xs.json').write_text('{}', encoding='utf-8')
        # Walking an overlong filename is platform-dependent; an absent input
        # exercises an earlier failure without needing filesystem privileges.
        r = run_xs(exe, 'pack', str(tmp_path/'absent-site'), '-o', str(preserved))
        check('failed pack preserves destination', r.returncode != 0 and
              preserved.read_bytes() == b'previous valid output')

        for kind in ['directory', 'file']:
            linked_out = tmp_path / ('out-link-' + kind)
            linked_out.mkdir()
            outside = tmp_path / ('outside-' + kind)
            outside.mkdir()
            sentinel = outside/'a.txt'
            sentinel.write_text('outside preserved', encoding='utf-8')
            link = linked_out/'wwwroot' if kind == 'directory' else linked_out/'wwwroot'/'a.txt'
            if kind == 'file': link.parent.mkdir()
            try:
                link.symlink_to(outside if kind == 'directory' else sentinel,
                                target_is_directory=kind == 'directory')
            except OSError as error:
                print(f'  SKIP: {kind} symlink creation unavailable: {error}')
                continue
            r = run_xs(exe, 'pack', '--extract', str(packed), '-d', str(linked_out))
            check(f'{kind} link does not overwrite outside file',
                  sentinel.read_text(encoding='utf-8') == 'outside preserved', r.stdout+r.stderr)
            if kind == 'directory':
                check('directory link escape rejected', r.returncode != 0, r.stdout+r.stderr)
            else:
                check('file link is replaced atomically or rejected',
                      r.returncode != 0 or not link.is_symlink(), r.stdout+r.stderr)

    print(f"\n{PASSED} pass, {FAILED} fail")
    return 1 if FAILED > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
