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
import os
import shutil
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


def run_xs(exe: Path, *args: str, cwd: Path | None = None,
           timeout: float = 120) -> subprocess.CompletedProcess:
    return subprocess.run([str(exe), *args], capture_output=True, text=True,
                          encoding="utf-8", errors="replace", timeout=timeout,
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


def check_unicode_paths(exe: Path, tmp: Path) -> None:
    base = tmp / '中文路径-🌍'
    base.mkdir()
    local_exe = base / ('工具.exe' if os.name == 'nt' else '工具')
    shutil.copy2(exe, local_exe)
    site = base / '站点'
    make_site(site)
    (site/'wwwroot'/'a.txt').rename(site/'wwwroot'/'内容-🌍.txt')
    (site/'空文件').write_bytes(b'')
    (site/'压缩内容.txt').write_bytes(b'compressed content\n' * 1024)
    packed = base/'归档.exe'
    result = run_xs(local_exe, 'pack', str(site), '-o', str(packed))
    check('Unicode executable, site and entry paths pack successfully',
          result.returncode == 0, result.stdout + result.stderr)
    if result.returncode != 0:
        return
    result = run_xs(local_exe, 'pack', '--list', str(packed))
    check('Unicode archive path lists successfully', result.returncode == 0 and
          '内容-🌍.txt' in result.stdout, result.stdout + result.stderr)
    extracted = base/'解包'
    result = run_xs(local_exe, 'pack', '--extract', str(packed), '-d', str(extracted))
    check('Unicode extract preserves empty, stored and compressed files',
          result.returncode == 0 and all(
              (extracted/path.relative_to(site)).read_bytes() == path.read_bytes()
              for path in site.rglob('*') if path.is_file()), result.stdout + result.stderr)
    stripped = base/'剥包.exe'
    result = run_xs(local_exe, 'pack', '--strip', str(packed), '-o', str(stripped))
    check('Unicode strip restores exact original executable', result.returncode == 0 and
          stripped.read_bytes() == exe.read_bytes(), result.stdout + result.stderr)
    repacked = base/'再次打包.exe'
    packed.chmod(packed.stat().st_mode | 0o111)
    result = run_xs(packed, 'pack', str(site), '-o', str(repacked))
    check('Unicode packed executable replaces its previous archive', result.returncode == 0 and
          Archive(bytearray(repacked.read_bytes())).header_off == exe.stat().st_size,
          result.stdout + result.stderr)


def make_sparse_file(path: Path, size: int) -> None:
    with path.open('wb') as stream:
        if os.name == 'nt':
            import ctypes
            import msvcrt
            from ctypes import wintypes
            ioctl = ctypes.WinDLL('kernel32', use_last_error=True).DeviceIoControl
            ioctl.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.LPVOID,
                              wintypes.DWORD, wintypes.LPVOID, wintypes.DWORD,
                              ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
            ioctl.restype = wintypes.BOOL
            returned = wintypes.DWORD()
            if not ioctl(msvcrt.get_osfhandle(stream.fileno()), 0x900C4,
                         None, 0, None, 0, ctypes.byref(returned), None):
                raise ctypes.WinError(ctypes.get_last_error())
        stream.truncate(size)


def check_lzma_trailing_data(exe: Path, tmp: Path) -> None:
    site = tmp/'lzma-site'
    make_site(site)
    (site/'wwwroot'/'large.txt').write_bytes(b'A' * 1024)
    packed = tmp/'lzma-good.exe'
    result = run_xs(exe, 'pack', str(site), '-o', str(packed))
    check('LZMA consumption fixture packs successfully', result.returncode == 0,
          result.stdout + result.stderr)
    if result.returncode != 0:
        return
    archive = Archive(bytearray(packed.read_bytes()))
    entry = next(e for e in archive.entries() if e['path'] == 'wwwroot/large.txt')
    assert entry['method'] == 1 and entry['doff'] + entry['csize'] < archive.index_off
    # Extend the declared compressed span into the next entry. The original
    # stream, decoded bytes and original CRC all remain intact.
    archive.patch_entry(entry, csize=entry['csize'] + 1)
    archive.refresh_crcs()
    bad = tmp/'lzma-trailing-data.exe'
    bad.write_bytes(archive.data)
    output = tmp/'lzma-out'
    (output/'wwwroot').mkdir(parents=True)
    target = output/'wwwroot'/'large.txt'
    target.write_bytes(b'previous content')
    result = run_xs(exe, 'pack', '--extract', str(bad), '-d', str(output))
    check('LZMA trailing data rejected despite matching decoded CRC',
          result.returncode != 0 and 'decode/CRC failed' in result.stdout,
          result.stdout + result.stderr)
    check('LZMA trailing data preserves existing target', target.read_bytes() == b'previous content')
    check('LZMA trailing data leaves no temporary target',
          not list(target.parent.glob('large.txt.xs-tmp-*')))
    for label, original_size in [('equal', entry['csize']), ('larger', entry['csize'] - 1)]:
        invalid = Archive(bytearray(packed.read_bytes()))
        invalid.patch_entry(entry, osize=original_size)
        invalid.refresh_crcs()
        bad = tmp/f'lzma-input-{label}-original.exe'
        bad.write_bytes(invalid.data)
        bad.chmod(bad.stat().st_mode | 0o111)
        try:
            result = run_xs(bad, cwd=tmp, timeout=5)
        except subprocess.TimeoutExpired:
            check(f'LZMA input {label} original rejected before startup', False,
                  'invalid archive did not stop startup within 5 seconds')
        else:
            check(f'LZMA input {label} original rejected before startup',
                  result.returncode != 0 and 'app pack invalid' in result.stdout,
                  result.stdout + result.stderr)


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
        check_unicode_paths(exe, tmp_path)
        check_lzma_trailing_data(exe, tmp_path)

        # ---- 正向:校验不误伤合法归档 ----
        out = tmp_path / "out-good"
        r = run_xs(exe, "pack", "--extract", str(packed), "-d", str(out))
        check("extract good exits 0", r.returncode == 0, r.stdout + r.stderr)
        check("extract good round-trip",
              (out / "wwwroot" / "a.txt").read_text(encoding="utf-8")
              == "hello-pack-validate\n")

        def malicious(name: str, mutate, err_hint: str, *, boot: bool = False) -> None:
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
            if boot:
                bad.chmod(bad.stat().st_mode | 0o111)
                try:
                    r = run_xs(bad, cwd=tmp_path, timeout=5)
                except subprocess.TimeoutExpired:
                    check(f'{name}: runtime rejects archive before startup', False,
                          'invalid archive did not stop startup within 5 seconds')
                else:
                    check(f'{name}: runtime rejects archive before startup',
                          r.returncode != 0 and 'app pack invalid' in r.stdout,
                          r.stdout + r.stderr)

        def swap_escape_path(a: Archive) -> None:
            e = next(e for e in a.entries() if e["path"] == "wwwroot/a.txt")
            a.patch_path(e, b"../evil-a.txt")

        # ---- 条目路径逃逸(等长替换,CRC 自洽) ----
        malicious("escape", swap_escape_path, "unsafe entry path")

        for name, path in [
                ('backslash-path', b'wwwroot\\a.txt'),
                ('absolute-path', b'/wwroot/a.txt'),
                ('trailing-slash', b'wwwroot/a.tx/'),
                ('duplicate-slash', b'wwwroot//.txt'),
                ('dot-segment', b'wwwroot/./txt')]:
            malicious(name, lambda a, path=path: a.patch_path(
                next(e for e in a.entries() if e['path'] == 'wwwroot/a.txt'), path),
                'unsafe entry path', boot=True)

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
        malicious('lzma-short',
                  lambda a: a.patch_entry(a.entries()[0], method=1, csize=0, osize=1),
                  'compressed entry invalid')

        broken_boot = tmp_path/'invalid-boot.exe'
        corrupt = bytearray(packed.read_bytes())
        struct.pack_into('<Q', corrupt, len(corrupt)-16, (1 << 64)-8)
        broken_boot.write_bytes(corrupt)
        broken_boot.chmod(broken_boot.stat().st_mode | 0o111)
        (tmp_path/'xs.json').write_text('{"services":[]}', encoding='utf-8')
        r = run_xs(broken_boot, cwd=tmp_path)
        check('invalid embedded archive refuses directory fallback',
              r.returncode != 0 and 'app pack invalid' in r.stdout, r.stdout+r.stderr)

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
        if os.name != 'nt':
            unsafe_site = tmp_path/'unsafe-site'
            unsafe_site.mkdir()
            (unsafe_site/'xs.json').write_text('{}', encoding='utf-8')
            (unsafe_site/'bad\\name.txt').write_text('payload', encoding='utf-8')
            unsafe_output = tmp_path/'unsafe-output.exe'
            unsafe_output.write_bytes(b'previous valid output')
            r = run_xs(exe, 'pack', str(unsafe_site), '-o', str(unsafe_output))
            check('source path rejected by archive readers cannot be published',
                  r.returncode != 0 and 'unsafe entry path' in r.stdout and
                  unsafe_output.read_bytes() == b'previous valid output', r.stdout + r.stderr)
            check('unsafe source path fails before compression',
                  'compressing' not in r.stdout, r.stdout + r.stderr)
        oversized = tmp_path/'oversized-site'
        oversized.mkdir()
        (oversized/'xs.json').write_text('{}', encoding='utf-8')
        try:
            make_sparse_file(oversized/'large.bin', (1 << 30) + 1)
        except OSError as error:
            print(f'  SKIP: sparse file fixture unavailable: {error}')
        else:
            r = run_xs(exe, 'pack', str(oversized), '-o', str(preserved))
            check('oversized source rejected before compression and preserves destination',
                  r.returncode != 0 and 'entry size over limit' in r.stdout and
                  preserved.read_bytes() == b'previous valid output', r.stdout + r.stderr)
        directory_target = tmp_path/'directory-target'
        directory_target.mkdir()
        (directory_target/'marker').write_text('preserved', encoding='utf-8')
        r = run_xs(exe, 'pack', str(site), '-o', str(directory_target))
        check('failed pack publication preserves existing directory', r.returncode != 0 and
              (directory_target/'marker').read_text(encoding='utf-8') == 'preserved')
        check('failed pack publication removes temporary output', not list(tmp_path.glob('xs-pack-*.tmp')))

        blocked_extract = tmp_path/'blocked-extract'
        (blocked_extract/'main.c').mkdir(parents=True)
        (blocked_extract/'main.c'/'marker').write_text('preserved', encoding='utf-8')
        r = run_xs(exe, 'pack', '--extract', str(packed), '-d', str(blocked_extract))
        check('failed extract publication preserves destination', r.returncode != 0 and
              (blocked_extract/'main.c'/'marker').read_text(encoding='utf-8') == 'preserved')
        check('failed extract publication removes temporary file', not list(blocked_extract.glob('*.xs-tmp-*')))
        nested = broken_site
        try:
            for _ in range(130):
                nested = nested/'d'
                nested.mkdir()
        except OSError as error:
            print(f'  SKIP: deep directory fixture unavailable: {error}')
        else:
            r = run_xs(exe, 'pack', str(broken_site), '-o', str(preserved))
            check('failed walk preserves destination', r.returncode != 0 and
                  'directory nesting over limit' in r.stdout and
                  preserved.read_bytes() == b'previous valid output', r.stdout+r.stderr)

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

        hardlink_out = tmp_path/'out-hardlink'
        (hardlink_out/'wwwroot').mkdir(parents=True)
        hardlink_original = tmp_path/'hardlink-original.txt'
        hardlink_original.write_text('outside preserved', encoding='utf-8')
        try:
            (hardlink_out/'wwwroot'/'a.txt').hardlink_to(hardlink_original)
        except OSError as error:
            print(f'  SKIP: hardlink fixture unavailable: {error}')
        else:
            r = run_xs(exe, 'pack', '--extract', str(packed), '-d', str(hardlink_out))
            check('hardlink target outside root is preserved',
                  hardlink_original.read_text(encoding='utf-8') == 'outside preserved', r.stdout+r.stderr)

    print(f"\n{PASSED} pass, {FAILED} fail")
    return 1 if FAILED > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
