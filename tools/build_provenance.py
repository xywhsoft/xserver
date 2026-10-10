"""Bind release binaries to their source inputs and selected extensions."""
from __future__ import annotations

import hashlib
import json
import os
import re
import tempfile
from pathlib import Path


SOURCE_INPUTS = (
    "main.c", "src", "lib", "tcc", "res",
    "tools/build.py", "tools/build_provenance.py", "tools/extensions.json",
    "tools/gen_tcc_resources.py", "tools/xs_extensions.py",
    "tools/tcc_include_closure.py", "tools/tcc_vfs_lzma_pack.c",
)


def file_digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_digest(root: Path) -> str:
    digest = hashlib.sha256(b"xs-build-inputs-v1\0")
    for relative in SOURCE_INPUTS:
        base = root / relative
        if not base.exists():
            raise RuntimeError(f"missing build input: {relative}")
        # pathlib 的 Windows 排序忽略大小写；源码签名必须跨平台一致。
        files = (sorted(base.rglob("*"), key=lambda path: path.relative_to(root).as_posix())
                 if base.is_dir() else [base])
        for path in files:
            if not path.is_file() or "__pycache__" in path.parts:
                continue
            if path.suffix.lower() in (".o", ".obj", ".pyc"):
                continue
            digest.update(path.relative_to(root).as_posix().encode("utf-8") + b"\0")
            digest.update(bytes.fromhex(file_digest(path)))
    return digest.hexdigest()


def metadata_path(binary: Path) -> Path:
    return binary.with_name(binary.name + ".build.json")


def write_metadata(binary: Path, *, source_hash: str, commit: str,
                   platform: str, extensions: list[str], banner: str) -> dict:
    record = {"schema": 1, "source_commit": commit, "source_sha256": source_hash,
              "binary_sha256": file_digest(binary), "platform": platform,
              "extensions": extensions, "banner": banner}
    target = metadata_path(binary)
    fd, name = tempfile.mkstemp(prefix="xs-provenance-", suffix=".tmp", dir=target.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as stream:
            json.dump(record, stream, ensure_ascii=False, indent=2)
            stream.write("\n")
        os.replace(name, target)
    finally:
        Path(name).unlink(missing_ok=True)
    return record


def verify_metadata(binary: Path, *, source_hash: str, platform: str,
                    extensions: list[str]) -> dict:
    target = metadata_path(binary)
    if not target.is_file():
        raise RuntimeError(f"missing build metadata for {binary.name}; rebuild with tools/build.py")
    record = json.loads(target.read_text(encoding="utf-8"))
    if not isinstance(record, dict) or record.get("schema") != 1:
        raise RuntimeError(f"invalid build metadata for {binary.name}")
    commit = record.get("source_commit", "")
    if not isinstance(commit, str) or re.fullmatch(r"[0-9a-f]{7,40}", commit) is None:
        raise RuntimeError(f"uncommitted or unknown source revision for {binary.name}; commit and rebuild")
    if record.get("source_sha256") != source_hash:
        raise RuntimeError(f"stale source inputs for {binary.name}; rebuild before packaging")
    if record.get("binary_sha256") != file_digest(binary):
        raise RuntimeError(f"binary hash mismatch for {binary.name}; rebuild before packaging")
    if record.get("platform") != platform or record.get("extensions") != extensions:
        raise RuntimeError(f"build variant mismatch for {binary.name}; rebuild the all variant")
    banner = record.get("banner", "")
    if not isinstance(banner, str) or f"(commit {commit}," not in banner:
        raise RuntimeError(f"version banner mismatch for {binary.name}")
    return record
