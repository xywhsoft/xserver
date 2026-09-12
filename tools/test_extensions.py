"""Fast registry/build-plan tests: python tools/test_extensions.py."""
from __future__ import annotations

import contextlib
import copy
import io
import json
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import build
import gen_tcc_resources as vfs
from xs_extensions import ROOT, host_header, load_registry, select_extensions, source_path


class ExtensionTests(unittest.TestCase):
    def test_selection(self):
        for names, expected in (
            ([], []), (["sqlite"], ["sqlite"]), (["xtp"], ["xtp"]),
            (["sqlite", "xtp"], ["sqlite", "xtp"]),
            (["xtp", "SQLITE", "xtp", "sqlite"], ["sqlite", "xtp"]),
        ):
            with self.subTest(names=names):
                self.assertEqual(list(select_extensions(names)), expected)
        with self.assertRaisesRegex(ValueError, "unknown extension"):
            select_extensions(["sqilte"])

    def test_headers_and_symbols_follow_selection(self):
        for names in ([], ["sqlite"], ["xtp"], ["sqlite", "xtp"]):
            with self.subTest(names=names):
                selected = select_extensions(names)
                resources = dict(vfs.collect(selected))
                header = host_header(selected)
                self.assertIn("xs/xsbase.h", resources)
                for name, entry in load_registry().items():
                    enabled = name in names
                    self.assertEqual(entry["symbols"] in header, enabled)
                    self.assertEqual(entry["macro"] in header, enabled)
                    for virtual in entry["headers"]:
                        self.assertEqual("xs/" + virtual in resources, enabled)

    def test_unknown_argument_is_early_and_non_mutating(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp) / "must-not-exist"
            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr), self.assertRaises(SystemExit) as caught:
                build.main(["sqlite", "typo", "--build-dir", str(directory)])
            self.assertEqual(caught.exception.code, 2)
            self.assertIn("unknown extension", stderr.getvalue())
            self.assertFalse(directory.exists())

    def test_dry_run_deduplicates_and_preserves_spaced_paths(self):
        with tempfile.TemporaryDirectory(prefix="xs build plan ") as temp:
            directory = Path(temp) / "objects"
            output = Path(temp) / "result with spaces.exe"
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                code = build.main(["xtp", "--dry-run", "sqlite", "xtp",
                                   "--build-dir", str(directory), "--output", str(output)])
            self.assertEqual(code, 0)
            text = stdout.getvalue()
            self.assertEqual(text.count("-c lib/xtp2.h "), 1)
            self.assertEqual(text.count("-c lib/sqlite3.c "), 1)
            self.assertIn("extensions=sqlite, xtp", text)
            self.assertIn(str(output.resolve()), text)
            self.assertFalse(directory.exists())
            self.assertFalse(output.exists())

    def test_empty_build_has_no_optional_objects_or_macros(self):
        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            self.assertEqual(build.main(["--dry-run"]), 0)
        text = stdout.getvalue()
        self.assertNotIn("-DXS_USE_", text)
        self.assertNotIn("extension-sqlite", text)
        self.assertNotIn("extension-xtp", text)

    def test_addition_needs_no_build_or_host_branch(self):
        registry = load_registry()
        registry["demo"] = copy.deepcopy(registry["xtp"])
        registry["demo"]["macro"] = "XS_USE_DEMO"
        registry["demo"]["headers"] = {"demo.h": "lib/xtp2.h"}
        registry["demo"]["link_flags"] = {"linux": ["-lm"]}
        selected = select_extensions(["demo"], registry)
        self.assertIn('tcc_define_symbol(pTcc, "XS_USE_DEMO", "1")', host_header(selected))
        self.assertIn("xs/demo.h", dict(vfs.collect(selected)))

    def test_collisions_and_path_bounds(self):
        registry = load_registry()
        registry["xtp"]["headers"] = {"SQLITE3.H": "lib/xtp2.h"}
        with self.assertRaisesRegex(ValueError, "collision"):
            select_extensions(["sqlite", "xtp"], registry)
        registry["xtp"]["headers"] = {"xsbase.h": "lib/xtp2.h"}
        with self.assertRaisesRegex(ValueError, "duplicate VFS"):
            vfs.collect(select_extensions(["xtp"], registry))
        with self.assertRaisesRegex(ValueError, "out-of-project"):
            source_path("../not-an-extension.h")

    def test_registry_typos_are_not_silently_ignored(self):
        for field, value in (("macro", None), ("sources", []), ("flags", [])):
            with self.subTest(field=field), tempfile.TemporaryDirectory() as temp:
                registry = load_registry()
                registry["xtp"][field] = value
                path = Path(temp) / "extensions.json"
                path.write_text(json.dumps(registry), encoding="utf-8")
                with self.assertRaises(ValueError):
                    load_registry(path)

    def test_xtp_import_covers_every_external_api(self):
        header = (ROOT / "lib/xtp2.h").read_text(encoding="utf-8")
        imports = (ROOT / "src/script/import_xtp2.inc").read_text(encoding="utf-8")
        apis = set(re.findall(r"^(?:void|bool|xtp2_result|xtp2_iter|const char \*)"
                              r"\s*(xtp2_\w+)\(", header, re.MULTILINE))
        symbols = re.findall(r"XS_XTP2_SYMBOL\((\w+)\)", imports)
        self.assertEqual(len(symbols), 13)
        self.assertEqual(len(symbols), len(set(symbols)))
        self.assertEqual(set(symbols), apis)

    def test_publish_failure_preserves_existing_binary(self):
        with tempfile.TemporaryDirectory() as temp:
            source, target = Path(temp) / "new", Path(temp) / "xs"
            source.write_bytes(b"new")
            target.write_bytes(b"existing")
            with patch("build.shutil.copy2", side_effect=OSError("injected")):
                with self.assertRaises(OSError):
                    build.publish(source, target)
            self.assertEqual(target.read_bytes(), b"existing")
            self.assertEqual({p.name for p in Path(temp).iterdir()}, {"new", "xs"})
            build.publish(source, target)
            self.assertEqual(target.read_bytes(), b"new")

    def test_link_failure_preserves_variant_executable(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            platform = "windows" if os.name == "nt" else "linux"
            target = root / platform / "default" / ("xs.exe" if os.name == "nt" else "xs")
            target.parent.mkdir(parents=True)
            target.write_bytes(b"previous executable")

            def fake_run(command, dry_run=False):
                if any(arg.endswith("main.o") for arg in command) and "-c" not in command:
                    Path(command[command.index("-o") + 1]).write_bytes(b"incomplete link")
                    raise subprocess.CalledProcessError(1, command)

            with patch("build.run", side_effect=fake_run), \
                 patch("build.vfs.generate"), \
                 patch("build.subprocess.check_output", return_value="test-triplet\n"), \
                 contextlib.redirect_stdout(io.StringIO()), \
                 contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(build.main(["--build-dir", str(root), "--output", str(target)]), 1)
            self.assertEqual(target.read_bytes(), b"previous executable")
            self.assertFalse(list(target.parent.glob("xs-link-*")))


if __name__ == "__main__":
    unittest.main(verbosity=2)
