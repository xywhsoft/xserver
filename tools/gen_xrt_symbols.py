#!/usr/bin/env python3
"""从 lib/xrt_decl.h 生成 TCC 运行时符号清单（移植自 xlang gen_xrt_symbol_table 经验）。

用法：python tools/gen_xrt_symbols.py
输出：src/script/import_xrt.inc（XS_XRT_SYMBOL(name) 每行一个）
校验：符号数 >= 1000 且无重复，否则报错退出。
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / "lib" / "xrt_decl.h"
OUTPUT = ROOT / "src" / "script" / "import_xrt.inc"

FUNCTION = re.compile(
    r"\bXRT_API\s+[^;{}]*?\b(xrt[A-Za-z0-9_]*)\s*\([^;{}]*\)\s*;",
    re.DOTALL,
)


def declarations(header: Path) -> str:
    text = header.read_text(encoding="utf-8")
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.DOTALL)
    text = re.sub(r"//[^\r\n]*", " ", text)
    text = re.sub(r"(?m)^[ \t]*#[^\r\n]*(?:\\\r?\n[^\r\n]*)*", " ", text)
    return text


def main() -> int:
    names = FUNCTION.findall(declarations(HEADER))
    unique = sorted(set(names))
    if len(unique) != len(names):
        dup = sorted({n for n in names if names.count(n) > 1})
        print(f"error: duplicate XRT_API declarations: {dup[:5]}", file=sys.stderr)
        return 1
    if len(unique) < 1000:
        print(f"error: too few XRT_API functions: {len(unique)}", file=sys.stderr)
        return 1
    version = (ROOT / "lib" / "xrt_version.txt").read_text(encoding="utf-8").splitlines()[0]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="\n") as fp:
        fp.write("/* 由 tools/gen_xrt_symbols.py 生成，勿手改。xrt 锁定版本见 lib/xrt_version.txt */\n")
        fp.write(f"/* xrt commit: {version} | 登记公共函数 {len(unique)} 个 */\n")
        for name in unique:
            fp.write(f"XS_XRT_SYMBOL({name})\n")
    print(f"generated {OUTPUT} ({len(unique)} symbols)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
