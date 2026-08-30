#!/usr/bin/env python3
"""include_win 保留集分析：从保证入口头出发计算 #include 闭包。

用法：
  python tools/tcc_include_closure.py            # 分析报告（不删除）
  python tools/tcc_include_closure.py --apply    # 删除闭包外的文件

入口集 = include_win 根部全部散头（CRT）+ ENTRY_ENTRIES 列出的 SDK 入口。
闭包外的 include_win 文件视为可删除。
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
INC = ROOT / "res" / "tcc" / "include_win"

# 保证可用的 SDK 入口：核心系统 / 网络 / 进程 / COM(Excel 自动化) /
# ADO(数据库) / Shell / XML / 签名校验 / 证书 / HTTP 客户端 / 加密
ENTRIES = [
    "windows.h", "winsock2.h", "ws2tcpip.h", "mstcpip.h", "winnetwk.h",
    "objbase.h", "oleauto.h", "oaidl.h", "ole2.h", "comcat.h",
    "msxml2.h",
    "shellapi.h", "shlobj.h", "shlwapi.h",
    "psapi.h", "tlhelp32.h", "wincon.h",
    # ADO 类型化头（adoint/msado15）为 TCC 不可解析的 IDL 重语法；
    # 数据访问保证面 = ODBC（sql 家族）+ OLE DB + IDispatch 后期绑定。
    "oledb.h",
    "wintrust.h", "mssip.h", "wincrypt.h",
    "wininet.h",     "sddl.h", "aclapi.h", "authz.h",
    "dbghelp.h", "winver.h", "lm.h", "nb30.h",
    # 本地化（mlang 代码页转换）与数据访问（ODBC / ADO 补充 / MSXML dispatch）
    "mlang.h",
    "sql.h", "sqlext.h", "sqlucode.h",
    "msxml2did.h", "msxmldid.h", "olectl.h",
]

INCLUDE_RE = re.compile(r"^\s*#\s*include\s*[<\"]([^>\"]+)[>\"]", re.M)


def search_dirs() -> list[Path]:
    return [INC, INC / "winapi", INC / "sys", INC / "sec_api", INC / "tcc", INC / "sdks"]


def all_headers() -> dict[str, Path]:
    """按 TCC 实际搜索路径建表：排除 ddk/GL（不在任何 include 路径上，
    且与 winapi 存在同名文件，setdefault 抢注会导致 winapi 正本被误删）"""
    result: dict[str, Path] = {}
    excluded = {"ddk", "gl"}
    for path in INC.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(INC).parts
        if rel and rel[0].lower() in excluded:
            continue
        result.setdefault(path.name.lower(), path)
    return result


def closure_of(entries: list[str], table: dict[str, Path]) -> set[Path]:
    seen: set[Path] = set()
    queue: list[Path] = []
    for name in entries:
        path = table.get(name.lower())
        if path is not None and path not in seen:
            seen.add(path)
            queue.append(path)
    while queue:
        path = queue.pop()
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        for name in INCLUDE_RE.findall(text):
            target = table.get(name.lower().split("/")[-1])
            if target is not None and target not in seen:
                seen.add(target)
                queue.append(target)
    return seen


def main() -> int:
    table = all_headers()
    # 根部散头（CRT 家族）全部作为入口
    root_entries = [p.name for p in INC.glob("*.h")]
    missing = [name for name in ENTRIES if name.lower() not in table]
    if missing:
        print(f"error: entries not found in include_win: {missing}", file=sys.stderr)
        return 1
    keep = closure_of(root_entries + ENTRIES, table)
    # 指定整目录处置
    keep_dir_whitelist = {INC / "sys", INC / "tcc", INC / "sec_api", INC / "sdks"}
    all_files = {p for p in INC.rglob("*") if p.is_file()}
    for directory in keep_dir_whitelist:
        keep |= {p for p in directory.rglob("*") if p.is_file()}

    candidates = sorted(all_files - keep)
    keep_size = sum(p.stat().st_size for p in keep)
    cut_size = sum(p.stat().st_size for p in candidates)
    print(f"keep: {len(keep)} files, {keep_size/1024:.0f} KB")
    print(f"cut : {len(candidates)} files, {cut_size/1024:.0f} KB")

    family: dict[str, list[Path]] = {}
    for path in candidates:
        rel = path.relative_to(INC).as_posix().lower()
        key = rel.split("/")[0] if "/" in rel else "(root)"
        family.setdefault(key, []).append(path)
    for key in sorted(family, key=lambda k: -sum(p.stat().st_size for p in family[k])):
        size = sum(p.stat().st_size for p in family[key])
        print(f"  {key:12s} {len(family[key]):4d} files {size/1024:7.0f} KB")

    if "--apply" in sys.argv:
        for path in candidates:
            path.unlink()
        # 清空目录（ddk/GL 等整族删除后的空壳）
        for path in sorted((p for p in INC.rglob("*") if p.is_dir()), reverse=True):
            try:
                path.rmdir()
            except OSError:
                pass
        print("applied.")
    else:
        preview = "\n".join(str(p.relative_to(INC)) for p in candidates[:40])
        print("---- 删除预览（前 40）----")
        print(preview)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
