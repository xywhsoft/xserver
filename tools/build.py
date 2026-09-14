#!/usr/bin/env python3
"""Shared Windows/Linux build: python tools/build.py [sqlite] [xtp] ..."""
from __future__ import annotations

import argparse
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

import gen_tcc_resources as vfs
from xs_extensions import ROOT, host_header, load_registry, select_extensions


def run(command: list[str], dry_run: bool = False) -> None:
    print(subprocess.list2cmdline(command) if os.name == "nt" else shlex.join(command),
          flush=True)
    if not dry_run:
        subprocess.run(command, cwd=ROOT, check=True)


def publish(source: Path, output: Path) -> None:
    """Keep the previous executable intact on compiler/linker/copy failure."""
    if source.resolve() == output.resolve():
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix="xs-publish-", suffix=".tmp", dir=output.parent)
    os.close(fd)
    temporary = Path(name)
    try:
        shutil.copy2(source, temporary)
        os.replace(temporary, output)
    finally:
        temporary.unlink(missing_ok=True)


def build(args: argparse.Namespace, selected: dict) -> Path:
    platform = "windows" if os.name == "nt" else "linux"
    suffix = ".exe" if platform == "windows" else ""
    variant = "+".join(selected) or "default"
    directory = args.build_dir.resolve() / platform / variant
    output = (args.output or ROOT / "release" / ("xs" + suffix)).resolve()
    packer = directory / ("vfs_pack" + suffix)
    resources = directory / "tcc_builtin_resources.c"
    linked = directory / ("xs" + suffix)
    print(f"[build] platform={platform} extensions={', '.join(selected) or '(none)'}",
          flush=True)
    print(f"[build] directory={directory}", flush=True)
    if not args.dry_run:
        directory.mkdir(parents=True, exist_ok=True)
        header = directory / "xs_build_extensions.h"
        content = host_header(selected)
        if not header.is_file() or header.read_text(encoding="utf-8") != content:
            header.write_text(content, encoding="utf-8", newline="\n")

    run([args.cc, "tools/tcc_vfs_lzma_pack.c", "tcc/LzmaEnc.c", "tcc/LzFind.c",
         "tcc/CpuArch.c", "-I", "tcc", "-DZ7_ST", "-O2", "-s", "-o", str(packer)],
        args.dry_run)
    if args.dry_run:
        gen_cmd = [sys.executable, "tools/gen_tcc_resources.py", *selected,
             "--output", str(resources), "--pack-tool", str(packer)]
        if args.sysroot is not None:
            gen_cmd += ["--sysroot", str(args.sysroot)]
        run(gen_cmd, True)
    else:
        vfs.generate(selected, output=resources, pack_tool=packer,
                     sysroot=args.sysroot.resolve() if args.sysroot else None)

    flags = ["-I", str(ROOT / "lib"), "-I", str(ROOT / "tcc"),
             "-I", str(ROOT), "-I", str(directory), "-DCONFIG_TCC_BUILTIN_VFS"]
    flags += [f"-D{entry['macro']}=1" for entry in selected.values()]
    # 特性宏：宿主编译也拉起扩展的完整声明闭包（unity TU 经 sources[].defines 同源）
    for entry in selected.values():
        features = entry.get("feature_macro", [])
        features = [features] if isinstance(features, str) else features
        flags += [f"-D{feature}=1" for feature in features]
    # 扩展可声明宿主编译所需的额外 include 目录（清单驱动，无逐库分支）
    flags += [f"-I{d}" for entry in selected.values()
              for d in entry.get("host_includes", [])]
    if args.sysroot is not None:
        flags.append('-DCONFIG_SYSROOT="/xsroot"')
    # 版本标识：commit / 构建日期 / 平台，注入启动横幅与 --version（见 xs_version.h）
    if not args.dry_run:
        try:
            commit = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"],
                                             text=True, cwd=ROOT,
                                             stderr=subprocess.DEVNULL).strip()
        except (OSError, subprocess.CalledProcessError):
            commit = "unknown"
        flags.append(f'-DXS_BUILD_COMMIT="{commit}"')
        flags.append(f'-DXS_BUILD_DATE="{time.strftime("%Y-%m-%d")}"')
        flags.append(f'-DXS_BUILD_PLATFORM="{platform}"')
    if platform == "linux":
        triplet = "<compiler-triplet>" if args.dry_run else subprocess.check_output(
            [args.cc, "-dumpmachine"], text=True, cwd=ROOT).strip()
        flags.append(f'-DCONFIG_TRIPLET="{triplet}"')
    objects: list[str] = []

    def compile_source(source: str, name: str, extra: list[str]) -> None:
        obj = str(directory / (name + ".o"))
        run([args.cc, "-x", "c", "-c", source, *flags, "-O2", *extra, "-o", obj],
            args.dry_run)
        objects.append(obj)

    for name, entry in selected.items():
        for index, source in enumerate(entry["sources"]):
            extra = ["-D" + value for value in source.get("defines", [])]
            compile_source(source["path"], f"extension-{name}-{index}",
                           extra + source.get("flags", []))
    compile_source("main.c", "main", ["-Wall"])
    core = {
        "tcc-libtcc": "tcc/libtcc.c",
        "tcc-vfs": "tcc/tcc_builtin_vfs.c",
        "lzma-dec": "tcc/LzmaDec.c",
        "resources": str(resources),
    }
    if platform == "windows":
        core["tcc-utf8"] = "tcc/tcc_utf8_io.c"
    for name, source in core.items():
        compile_source(source, name, [])
    if platform == "windows":
        icon = str(directory / "icon.o")
        run(["windres", "res/xs.rc", "-O", "coff", "-o", icon], args.dry_run)
        objects.append(icon)
        libraries = ["-lws2_32", "-lshell32", "-liphlpapi"]
    else:
        libraries = ["-ldl", "-lpthread", "-lm"]
    for entry in selected.values():
        libraries += entry.get("link_flags", {}).get(platform, [])
    if args.link_extra:
        libraries += shlex.split(args.link_extra)
    if args.dry_run:
        staged = directory / "link-stage" / ("xs" + suffix)
        run([args.cc, *objects, "-O2", "-s", "-o", str(staged), *libraries], True)
    else:
        # Even --output=<variant>/xs is protected from a failed linker.
        with tempfile.TemporaryDirectory(prefix="xs-link-", dir=directory) as temp:
            staged = Path(temp) / ("xs" + suffix)
            run([args.cc, *objects, "-O2", "-s", "-o", str(staged), *libraries])
            publish(staged, linked)
            publish(linked, output)
    print(f"[build] {'would publish' if args.dry_run else 'built'} {output}", flush=True)
    return output


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Build xs with any combination of optional extension libraries.",
        epilog="Examples: build.bat sqlite xtp | bash build.sh xtp sqlite")
    parser.add_argument("extensions", nargs="*", metavar="LIB")
    parser.add_argument("--list-extensions", action="store_true", help="list registered libraries")
    parser.add_argument("--dry-run", action="store_true", help="print commands without writing/building")
    parser.add_argument("--build-dir", type=Path, default=ROOT / ".build",
                        help="artifact root; platform/extension variants are isolated below it")
    parser.add_argument("--output", type=Path, help="output executable (default: release/xs[.exe])")
    parser.add_argument("--cc", default="gcc", help="C compiler executable (default: gcc)")
    parser.add_argument("--sysroot", type=Path, default=None,
                        help="embed libc headers (e.g. musl include) at virtual /xsroot/usr/include "
                             "and compile libtcc with -DCONFIG_SYSROOT=/xsroot")
    parser.add_argument("--link-extra", default="", help="extra flags appended to the final link")
    args = parser.parse_intermixed_args(argv)
    try:
        registry = load_registry()
        selected = select_extensions(args.extensions, registry)
    except (ValueError, OSError) as error:
        parser.error(str(error))
    if args.list_extensions:
        for name, entry in registry.items():
            print(f"{name:12} {entry['macro']:20} {', '.join(entry['headers'])}")
        return 0
    try:
        build(args, selected)
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"[build] failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
