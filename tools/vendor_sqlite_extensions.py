"""Check the pinned SQLite extension sources and generated Cypher parser."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOCK = Path("lib/sqlite_extensions.lock.json")


def check_lock(root=ROOT):
    root = root.resolve()
    record = json.loads((root / LOCK).read_text(encoding="utf-8"))
    files = record.get("files")
    if record.get("schema") != 1 or not isinstance(files, dict) or not files:
        raise ValueError("invalid SQLite extension source lock")
    for relative, expected in files.items():
        path = (root / relative).resolve()
        if not path.is_relative_to(root) or not path.is_file():
            raise ValueError("missing or escaped SQLite extension file: " + relative)
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected["sha256"]:
            raise ValueError("SQLite extension snapshot drift: " + relative)


if __name__ == "__main__":
    check_lock()
    print("PASS pinned SQLite extension sources")
