"""Snapshot integrity and SDK export coverage; no network or load tests."""
import json
import re
import tempfile
import unittest
from pathlib import Path

from sync_xrt import LOCK, LIBRARIES, check_lock, digest, public_functions
from xs_extensions import ROOT, load_registry


class SnapshotTests(unittest.TestCase):
    def test_current_snapshot(self):
        check_lock()

    def test_shared_wait_helper_is_in_the_sdk(self):
        from gen_tcc_resources import collect
        resources = dict(collect({}))
        helper = ROOT / "lib/xrtshim/xrt/detail/wait.h"
        self.assertEqual(resources["xs/xrt/detail/wait.h"], helper)
        record = json.loads(LOCK.read_text(encoding="utf-8"))["files"]
        self.assertEqual(record["lib/xrtshim/xrt/detail/wait.h"]["source"],
                         "include/xrt/detail/wait.h")

    def test_public_extension_exports(self):
        registry = load_registry()
        for name in LIBRARIES:
            with self.subTest(name=name):
                entry = registry[name]
                prefix = "(?:xrt|xacme)" if name == "xacme" else "xrt" if name in ("xmail", "xsmtp", "xpop3", "ximap") else "xllm" if name == "xllm-session" else name
                expected = public_functions([ROOT / p for p in entry["headers"].values()], prefix)
                if name == "xllm-session":
                    companion = public_functions([ROOT / p for p in registry["xllm"]["headers"].values()], "xllm")
                    expected = sorted(set(expected) - set(companion))
                actual = re.findall(entry["symbol_macro"] + r"\((\w+)\)", (ROOT / entry["symbols"]).read_text(encoding="utf-8"))
                self.assertEqual(actual, expected)

    def test_drift_and_missing_file_are_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            file = root / "header.h"
            file.write_bytes(b"original")
            lock = root / "lock.json"
            lock.write_text(json.dumps({"schema": 1, "files": {"header.h": {"sha256": digest(b"original")}}}), encoding="utf-8")
            check_lock(root, lock)
            file.write_bytes(b"patched")
            with self.assertRaisesRegex(ValueError, "snapshot drift"):
                check_lock(root, lock)
            file.unlink()
            with self.assertRaisesRegex(ValueError, "missing"):
                check_lock(root, lock)

    def test_lock_path_cannot_escape_repository(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "repo"
            root.mkdir()
            outside = root.parent / "outside.h"
            outside.write_bytes(b"content")
            lock = root / "lock.json"
            lock.write_text(json.dumps({"schema": 1, "files": {"../outside.h": {"sha256": digest(b"content")}}}), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "out-of-project"):
                check_lock(root, lock)


if __name__ == "__main__":
    unittest.main(verbosity=2)
