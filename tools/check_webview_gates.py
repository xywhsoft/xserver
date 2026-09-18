#!/usr/bin/env python3
"""兼容门禁（docs/Webview扩展与app节点设计.md §10）。

门禁 A（Windows）：objdump 解析 PE 导入表，断言未引入 C++/WebView2 运行时 DLL
  —— libstdc++-6.dll / WebView2Loader.dll / libgcc* / libwinpthread*。
  webview 扩展不得给 xs.exe / xsw.exe 增加任何硬 DLL 依赖
  （libstdc++ 静态解析、WebView2Loader 由库在运行期从 Evergreen Runtime 动态定位）。

门禁 B（Linux）：ldd 断言未引入 webkit/gtk/X11/wayland/egl 家族动态库。
  Linux 端 webview 编译为空，产物依赖必须与未加扩展前完全一致。

用法：
  python tools/check_webview_gates.py release/xs.exe [release/xsw.exe ...]   (Windows)
  python tools/check_webview_gates.py release/xs                              (Linux)
退出码：0=通过，1=有违规或工具不可用。任何平台缺少解析工具都判失败（fail-closed）。
"""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys

# 门禁 A 禁入（小写子串匹配）
FORBIDDEN_WINDOWS = (
    "libstdc++-6.dll",   # libstdc++ 必须静态解析
    "webview2loader",    # 加载器只能运行期动态定位，不得成为硬导入
    "libgcc",            # libgcc_s/libgccjit 等共享运行时
    "libwinpthread",     # winpthreads 共享运行时
)
# 门禁 B 禁入（小写子串匹配，覆盖 webkit2gtk/4.1 与其 X11/GTK 依赖族）
FORBIDDEN_LINUX = (
    "webkit", "gtk", "gdk", "pango", "cairo", "harfbuzz",
    "libx11", "libxcb", "wayland", "libegl", "libgl.so",
)


def fail(message: str) -> None:
    print(f"[gate] FAIL {message}", flush=True)


def gate_windows(executable: str) -> bool:
    objdump = shutil.which("objdump")
    if objdump is None:
        fail("objdump not found in PATH (toolchain required)")
        return False
    result = subprocess.run([objdump, "-p", executable],
                            capture_output=True, text=True)
    if result.returncode != 0:
        fail(f"objdump failed on {executable}: {result.stderr.strip()}")
        return False
    imports = re.findall(r"DLL Name:\s*(\S+)", result.stdout)
    if not imports:
        fail(f"no DLL imports parsed from {executable}")
        return False
    bad = [dll for dll in imports
           if any(token in dll.lower() for token in FORBIDDEN_WINDOWS)]
    print(f"[gate] A {executable}: {len(imports)} imports "
          f"({', '.join(imports)})")
    for dll in bad:
        fail(f"{executable} imports forbidden DLL: {dll}")
    return not bad


def gate_linux(executable: str) -> bool:
    ldd = shutil.which("ldd")
    if ldd is None:
        fail("ldd not found in PATH")
        return False
    result = subprocess.run([ldd, executable], capture_output=True, text=True)
    if result.returncode != 0:
        fail(f"ldd failed on {executable}: {result.stderr.strip()}")
        return False
    libraries = [line.split("=>")[0].strip() for line in result.stdout.splitlines()
                 if "=>" in line or line.strip().endswith(".so")
                 or re.search(r"/[^\s]*\.so[^\s]*", line)]
    bad = [lib for lib in libraries
           if lib and any(token in lib.lower() for token in FORBIDDEN_LINUX)]
    print(f"[gate] B {executable}: {len(libraries)} dynamic deps")
    for lib in bad:
        fail(f"{executable} depends on forbidden library: {lib}")
    return not bad


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 1
    targets = argv[1:]
    if any(not os.path.isfile(t) for t in targets):
        fail(f"target not found: {next(t for t in targets if not os.path.isfile(t))}")
        return 1
    gate = gate_windows if os.name == "nt" else gate_linux
    ok = all(gate(t) for t in targets)
    print(f"[gate] {'PASS' if ok else 'FAIL'} ({'A/Windows' if os.name == 'nt' else 'B/Linux'})")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
