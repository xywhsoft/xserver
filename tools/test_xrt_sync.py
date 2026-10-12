"""Snapshot integrity and SDK export coverage; no network or load tests."""
import json
import re
import tempfile
import unittest
from pathlib import Path

from sync_xrt import (LOCK, LIBRARIES, check_lock, digest, library_names,
                      public_functions, public_functions_text, symbol_prefix)
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
                manifest = json.loads((ROOT / f"lib/{name}/config/modules.json")
                                      .read_text(encoding="utf-8"))
                prefix = symbol_prefix(name, manifest)
                expected = public_functions([ROOT / p for p in entry["headers"].values()], prefix)
                if name == "xllm-session":
                    companion = public_functions([ROOT / p for p in registry["xllm"]["headers"].values()], "xllm")
                    expected = sorted(set(expected) - set(companion))
                actual = re.findall(entry["symbol_macro"] + r"\((\w+)\)", (ROOT / entry["symbols"]).read_text(encoding="utf-8"))
                self.assertEqual(actual, expected)

    def test_xlang_runtime_is_excluded_from_discovery_and_builds(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            for name in ("xssh", "xruntime", "xhttp"):
                manifest = directory / name / "config/modules.json"
                manifest.parent.mkdir(parents=True)
                manifest.write_text("{}", encoding="utf-8")
            self.assertEqual(library_names(directory), ("xhttp", "xssh"))
        self.assertNotIn("xruntime", load_registry())
        self.assertFalse(any(p.startswith("lib/xruntime/") for p in
                             json.loads(LOCK.read_text(encoding="utf-8"))["files"]))

    def test_inline_calls_are_not_public_exports(self):
        text = """
        XHTTP_API bool xrtHttpPublic(void);
        static inline void helper(void) { xrtErrorFree(NULL); }
        /* XHTTP_API void xrtHttpComment(void); */
        """
        self.assertEqual(public_functions_text([text], "xrt"), ["xrtHttpPublic"])

    def test_protocol_dependency_closures(self):
        from xs_extensions import select_extensions
        self.assertEqual(set(select_extensions(["xhttp-client"])),
                         {"xhttp", "xhttp-policy", "xhttp-client"})
        self.assertEqual(set(select_extensions(["xws-client"])),
                         {"xhttp", "xhttp-policy", "xhttp-client", "xws", "xws-client"})
        self.assertEqual(set(select_extensions(["xws-server"])),
                         {"xhttp", "xhttp-server", "xws", "xws-server"})
        self.assertEqual(set(select_extensions(["xws-group"])), {"xws", "xws-group"})
        self.assertEqual(set(select_extensions(["xssh"])), {"xssh"})

    def test_snapshot_covers_every_registered_upstream_product(self):
        products = {Path(item["source"]).parts[1]
                    for item in json.loads(LOCK.read_text(encoding="utf-8"))["files"].values()
                    if item.get("source", "").startswith("extlibs/")}
        self.assertEqual(products, set(LIBRARIES))
        self.assertTrue(products <= load_registry().keys())

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
