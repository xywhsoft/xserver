#!/usr/bin/env python3
"""装配 XServer 发布包（平台 × libc）并输出到官网下载目录。

每个 zip 内部结构（与 QUICKSTART.md 描述一致）：
  xserver-<platform>/
  ├── xs / xs.exe        全功能版（all：全部可选库编入）
  ├── LICENSE            MIT
  ├── VERSION            构建时间 / commit / 变体 / 链接形态
  ├── QUICKSTART.md      快速上手
  ├── sdk/               头文件副本（IDE 补全用；TCC 实际使用内嵌 VFS）
  ├── demo/              极简示例（3 文件）
  └── demo-single/       完整工程范本（不含重复二进制）

产物：OUT_DIR/xserver-<platform>.zip + manifest.json（尺寸 / SHA256）。
用法：python tools/make_release.py [--out DIR]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import zipfile
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from xs_extensions import load_registry  # noqa: E402

VARIANTS = {
    "windows-x64": {
        "binary": ROOT / "release" / "xs.exe",
        "exe": "xs.exe",
        "os": "Windows 10/11 · x64",
        "note": "开箱即用，无需安装运行库",
    },
    "linux-x64-glibc": {
        "binary": ROOT / "release" / "xs-linux-x64-glibc",
        "exe": "xs",
        "os": "Linux x86-64 · glibc 2.38+",
        "note": "Ubuntu 24.04+ / Debian 13+ 等新发行版；脚本编译需系统 libc 头（libc6-dev）",
    },
    "linux-x64-musl": {
        "binary": ROOT / "release" / "xs-linux-x64-musl",
        "exe": "xs",
        "os": "Linux x86-64 · musl 全静态",
        "note": "零动态依赖，Alpine / 最小镜像 / 任意发行版可运行，libc 已内嵌",
    },
}

SDK_HEADERS = {
    "src/sdk/xsbase.h": "xsbase.h",
    "lib/xrt_decl.h": "xrt_decl.h",
    "lib/libtcc.h": "libtcc.h",
}


def git_commit() -> str:
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=ROOT, text=True).strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def iter_files(base: Path, skip_names: set[str] = frozenset()):
    for path in sorted(base.rglob("*")):
        if path.is_file() and path.name not in skip_names:
            yield path


def build_variant(name: str, meta: dict, when: str, commit: str, out_dir: Path) -> dict:
    binary = meta["binary"]
    if not binary.is_file():
        raise RuntimeError(f"missing binary for {name}: {binary}")

    registry = load_registry()
    # 版本块直接取产物自身输出（与启动横幅/--version 同源，杜绝手拼漂移）
    import subprocess as _sp
    try:
        banner = _sp.run([str(binary), "--version"], capture_output=True, text=True,
                         encoding="utf-8", errors="replace", timeout=20).stdout.strip()
    except (OSError, _sp.TimeoutExpired):
        banner = "XServer (version unavailable)"
    version_text = (
        f"{banner}\n"
        f"platform : {name} ({meta['os']})\n"
        f"build    : {when}\n"
        f"commit   : {commit}\n"
        f"license  : MIT (bundled third-party components keep their own)\n"
    )

    def arc(prefix: str, path: Path, inner: str | None = None) -> tuple[str, Path]:
        rel = inner if inner is not None else path.name
        return f"{prefix}/{rel}", path

    entries: list[tuple[str, Path]] = []
    staging = f"xserver-{name}"
    entries.append(arc(staging, binary, meta["exe"]))
    entries.append((f"{staging}/LICENSE", ROOT / "LICENSE"))
    entries.append((f"{staging}/VERSION", None))  # 占位，稍后写内容
    entries.append((f"{staging}/QUICKSTART.md", ROOT / "release" / "QUICKSTART.md"))
    for source, target in SDK_HEADERS.items():
        entries.append((f"{staging}/sdk/{target}", ROOT / source))
    for item in registry.values():
        for target, source in item["headers"].items():
            entries.append((f"{staging}/sdk/{target}", ROOT / source))
    for path in iter_files(ROOT / "release" / "demo"):
        entries.append(arc(f"{staging}/demo", path, str(path.relative_to(ROOT / "release" / "demo"))))
    demo_single = ROOT / "release" / "demo-single"
    for path in iter_files(demo_single, skip_names={"xs.exe"}):
        entries.append(arc(f"{staging}/demo-single", path, str(path.relative_to(demo_single))))

    zip_path = out_dir / f"xserver-{name}.zip"
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for target, source in entries:
            if source is None:
                zf.writestr(target, version_text)
                continue
            zf.write(source, target)

    digest = hashlib.sha256(zip_path.read_bytes()).hexdigest()
    return {
        "platform": name,
        "os": meta["os"],
        "note": meta["note"],
        "file": zip_path.name,
        "size": zip_path.stat().st_size,
        "sha256": digest,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path,
                        default=Path(r"D:\GIT\home\host\xs\wwwroot\download"),
                        help="download output directory")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    when = datetime.now().strftime("%Y-%m-%d %H:%M")
    commit = git_commit()
    manifest = {
        "product": "XServer",
        "variant": "full (all extensions)",
        "build": when,
        "commit": commit,
        "files": [],
    }
    for name, meta in VARIANTS.items():
        info = build_variant(name, meta, when, commit, args.out)
        manifest["files"].append(info)
        print(f"[release] {info['file']}  {info['size'] / 1048576:.1f} MB  {info['sha256'][:16]}…")
    (args.out / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"[release] manifest -> {args.out / 'manifest.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
