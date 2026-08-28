#!/usr/bin/env python3
"""xs3 TCC VFS 资源构建：把 res/tcc（TCC 环境）与 SDK 三件套打包进二进制。

VFS 布局（与 XS_TccCreate 的 include/lib 路径一一对应）：
  /xs/xsbase.h /xs/xrt_decl.h /xs/libtcc.h   ← SDK（源：src/sdk + lib，单一事实源）
  /tcc/include_win/** /tcc/include/**         ← TCC 头（Windows）
  /tcc/include_linux/**                       ← TCC 头（Linux）
  /tcc/lib/**                                 ← libtcc1.a 与 .def 导入库

压缩：LZMA（tools/tcc_vfs_lzma_pack.exe），压缩无收益或过小则 STORE。
用法：python tools/gen_tcc_resources.py [--store]    # --store 全部不压缩
输出：src/script/tcc_builtin_resources.c
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RES_TCC = ROOT / "res" / "tcc"
PACK_TOOL = ROOT / "tools" / "tcc_vfs_lzma_pack.exe"
OUTPUT = ROOT / "src" / "script" / "tcc_builtin_resources.c"

# (虚拟路径, 源文件) —— SDK 三件套来自源码目录，res 只承载 TCC 环境
SDK_FILES = [
    ("/xs/xsbase.h", ROOT / "src" / "sdk" / "xsbase.h"),
    ("/xs/xrt_decl.h", ROOT / "lib" / "xrt_decl.h"),
    ("/xs/libtcc.h", ROOT / "lib" / "libtcc.h"),
]

# (虚拟目录前缀, 源目录) —— 整目录递归收录
RES_DIRS = [
    ("/tcc/include_win", RES_TCC / "include_win"),
    ("/tcc/include", RES_TCC / "include"),
    ("/tcc/include_linux", RES_TCC / "include_linux"),
    ("/tcc/lib", RES_TCC / "lib"),
]

MIN_COMPRESS_SIZE = 64


def normalize_virtual_name(text: str) -> str:
    """与 tcc_vfs_normalize_key 对齐：小写、正斜杠、去首尾斜杠（资源名必须预规范化）"""
    return text.replace("\\", "/").strip("/").lower()


def lzma_pack(data: bytes) -> bytes | None:
    if not PACK_TOOL.is_file():
        raise RuntimeError(f"missing pack tool: {PACK_TOOL}（先执行 build.bat/sh 的工具构建步骤）")
    src = ROOT / "build_tmp_pack_in"
    dst = ROOT / "build_tmp_pack_out"
    src.write_bytes(data)
    try:
        subprocess.run([str(PACK_TOOL), str(src), str(dst), "9"], check=True)
        packed = dst.read_bytes()
    finally:
        src.unlink(missing_ok=True)
        dst.unlink(missing_ok=True)
    return packed if len(packed) < len(data) else None


def collect() -> list[tuple[str, Path]]:
    items: list[tuple[str, Path]] = []
    seen: set[str] = set()

    def add(virtual: str, source: Path) -> None:
        key = normalize_virtual_name(virtual)
        if key in seen or not source.is_file():
            return
        seen.add(key)
        items.append((key, source))

    for virtual, source in SDK_FILES:
        if not source.is_file():
            raise RuntimeError(f"missing SDK source: {source}")
        add(virtual, source)
    for prefix, directory in RES_DIRS:
        if not directory.is_dir():
            raise RuntimeError(f"missing VFS source directory: {directory}")
        for path in sorted(directory.rglob("*")):
            if path.is_file():
                add(f"{prefix}/{path.relative_to(directory)}", path)
    return items


def c_bytes(data: bytes) -> str:
    return ",\n\t".join(
        ",".join(str(b) for b in data[i:i + 16])
        for i in range(0, len(data), 16)
    )


def main() -> int:
    force_store = "--store" in sys.argv
    items = collect()
    parts = [
        "/* 由 tools/gen_tcc_resources.py 生成，勿手改。VFS 构建源：res/tcc + SDK 三件套 */",
        '#include "../../tcc/tcc_builtin_vfs.h"',
        "",
    ]
    total_raw = total_packed = 0
    rows: list[tuple[str, int, int, str]] = []
    for index, (virtual, source) in enumerate(items):
        data = source.read_bytes()
        packed: bytes | None = None
        method = "TCC_BUILTIN_RESOURCE_STORE"
        if not force_store and len(data) > MIN_COMPRESS_SIZE:
            packed = lzma_pack(data)
            if packed is not None:
                method = "TCC_BUILTIN_RESOURCE_LZMA"
        payload = packed if packed is not None else data
        parts.append(f"static const unsigned char xs_resource_{index}[] = {{\n\t{c_bytes(payload)}\n}};")
        parts.append("")
        rows.append((virtual, len(data), len(payload), method))
        total_raw += len(data)
        total_packed += len(payload)
    parts.append("const TCCBuiltinResource tcc_builtin_resources[] = {")
    for index, (virtual, raw, packed_len, method) in enumerate(rows):
        parts.append(f"\t{{ \"{virtual}\", xs_resource_{index}, {packed_len}, {raw}, {method} }},")
    parts.append("};")
    parts.append(f"const unsigned int tcc_builtin_resources_count = {len(rows)};")
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.open("w", encoding="utf-8", newline="\n").write("\n".join(parts) + "\n")
    lzma_count = sum(1 for *_, m in rows if m == "TCC_BUILTIN_RESOURCE_LZMA")
    print(f"generated {OUTPUT}")
    print(f"resources: {len(rows)} (lzma {lzma_count}, store {len(rows) - lzma_count})")
    print(f"raw {total_raw/1024:.0f} KB -> packed {total_packed/1024:.0f} KB")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
