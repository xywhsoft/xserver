"""Committed SDK snapshots must ignore dirty trees without mutating Git state."""
from __future__ import annotations
import contextlib
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import sync_xrt as sync


class CommitSnapshotTests(unittest.TestCase):
    def test_one_library_refresh_preserves_core_and_uses_committed_bytes(self):
        with tempfile.TemporaryDirectory(prefix="xs library SDK ") as directory:
            base=Path(directory); upstream, _, _=self.repository(base)
            header=upstream/"extlibs/demo/include/demo.h"
            header.write_bytes(b"DEMO_API void demoRun(void);\n")
            manifest={"api_reference":{"function_prefix":"demo"},"module_prefix":"DEMO_MODULE_",
                      "modules":[{"name":"demo","feature":"DEMO_FEATURE_DEMO","sources":["extlibs/demo/src/demo.c"]}]}
            (upstream/"extlibs/demo/config/modules.json").write_text(json.dumps(manifest))
            self.git(upstream,"add","--all")
            self.git(upstream,"-c","user.name=Fixture","-c","user.email=fixture@example.invalid","commit","--quiet","-m","library fixture")
            commit=self.git(upstream,"rev-parse","HEAD").decode().strip()
            header.write_bytes(b"dirty header must not ship\n")
            (upstream/"single/xrt.h").write_bytes(b"dirty core must not ship\n")
            vendor=base/"vendor";vendor.mkdir()
            registry={"demo":{"macro":"XS_USE_DEMO","symbols":"src/script/import_demo.inc","symbol_macro":"XS_DEMO_SYMBOL"}}
            retained={"lib/xrt.h":b"retained older core ABI\n","lib/other/UPSTREAM.txt":b"retained companion ABI\n"}
            record={"schema":1,"upstream_commit":"1"*40,"upstream_working_tree":False,"files":{}}
            for name,data in retained.items():
                p=vendor/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
                record["files"][name]={"sha256":sync.digest(data)}
            lock=vendor/"lib/xrt_sources.lock.json";lock.write_text(json.dumps(record))
            validate=sync.check_lock
            status=self.git(upstream,"status","--porcelain")
            with patch.object(sync,"ROOT",vendor),patch.object(sync,"LOCK",lock),patch.object(sync,"REGISTRY",vendor/"tools/extensions.json"), \
                 patch.object(sync,"load_registry",return_value=registry),patch.object(sync,"check_lock",side_effect=lambda:validate(vendor,lock)), \
                 contextlib.redirect_stdout(io.StringIO()):
                sync.refresh_library_commit(upstream,commit,"demo")
            self.assertEqual((vendor/"lib/demo/include/demo.h").read_bytes(),b"DEMO_API void demoRun(void);\n")
            for name,data in retained.items():self.assertEqual((vendor/name).read_bytes(),data)
            actual=json.loads(lock.read_text());self.assertEqual(actual["upstream_commit"],"1"*40)
            self.assertEqual(actual["library_commits"],{"demo":commit})
            self.assertEqual(actual["files"]["lib/demo/include/demo.h"]["upstream_commit"],commit)
            self.assertEqual(self.git(upstream,"status","--porcelain"),status)
            self.assertIn("XS_DEMO_SYMBOL(demoRun)",(vendor/"src/script/import_demo.inc").read_text())

    def git(self, directory, *arguments):
        return subprocess.check_output(["git", "-c", "core.autocrlf=false", *arguments], cwd=directory)

    def repository(self, directory):
        root = directory / "upstream"
        root.mkdir()
        self.git(root, "init", "--quiet", "--initial-branch=master")
        files = {"single/xrt.h": b"committed core\n", "single/xrt_decl.h": b"declarations\n",
                 "include/xrt/core.h": b"core header\n",
                 "extlibs/demo/include/demo.h": b"demo $Format:%H$\n",
                 "extlibs/demo/config/modules.json": b"{}\n",
                 "extlibs/demo/src/demo.c": b"demo source\n",
                 "extlibs/demo/src/large.c": b"/* chunked raw blob */\n" * 4000,
                 "include/xrt/空 格.h": b"unicode filename\n",
                 "extlibs/demo/tests/omit.c": b"not a production source\n",
                 ".gitattributes": b"extlibs/demo/include/demo.h export-subst\n"}
        for name, content in files.items():
            target = root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(content)
        self.git(root, "add", "--all")
        self.git(root, "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid",
                 "commit", "--quiet", "-m", "fixture")
        return root, files, self.git(root, "rev-parse", "HEAD").decode().strip()

    def test_raw_committed_bytes_ignore_dirty_files_and_git_filters(self):
        with tempfile.TemporaryDirectory(prefix="xs committed SDK ") as directory:
            base = Path(directory); root, files, commit = self.repository(base)
            (root / "single/xrt.h").write_bytes(b"dirty must not ship\n")
            (root / "extlibs/demo/include/untracked.h").write_bytes(b"untracked\n")
            status = self.git(root, "status", "--porcelain", "--untracked-files=all")
            index = (root / ".git/index").read_bytes()
            output = base / "snapshot"
            sync._export_commit(root, commit, output)
            for name, content in files.items():
                if name.startswith(("single/", "include/")) or name.startswith("extlibs/") and "/tests/" not in name:
                    self.assertEqual((output / name).read_bytes(), content)
            self.assertFalse((output / "extlibs/demo/include/untracked.h").exists())
            self.assertFalse((output / "extlibs/demo/tests").exists())
            self.assertEqual(self.git(root, "status", "--porcelain", "--untracked-files=all"), status)
            self.assertEqual((root / ".git/index").read_bytes(), index)

    def test_refresh_uses_pinned_identity_and_cleans_private_snapshot(self):
        with tempfile.TemporaryDirectory() as directory:
            root, _, commit = self.repository(Path(directory)); seen = []
            def refreshed(source, *, committed):
                self.assertEqual(committed, commit)
                self.assertEqual((source / "single/xrt.h").read_bytes(), b"committed core\n")
                seen.append(source)
            with patch.object(sync, "refresh", side_effect=refreshed) as run:
                sync.refresh_commit(root, "HEAD")
                self.assertEqual(run.call_count, 1)
            self.assertEqual(len(seen), 1)
            self.assertFalse(seen[0].exists())

    def test_snapshot_cannot_overwrite_an_existing_destination(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory); root, _, commit = self.repository(base)
            output = base / "already"; output.mkdir(); (output / "keep").write_bytes(b"keep")
            with self.assertRaises(FileExistsError):
                sync._export_commit(root, commit, output)
            self.assertEqual((output / "keep").read_bytes(), b"keep")

    def test_symlink_blob_is_rejected_without_following_it(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory); root, _, _ = self.repository(base)
            blob = subprocess.check_output(["git", "hash-object", "-w", "--stdin"],
                cwd=root, input=b"../../outside").decode().strip()
            self.git(root, "update-index", "--add", "--cacheinfo", "120000", blob,
                     "extlibs/demo/src/escape.h")
            self.git(root, "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid",
                     "commit", "--quiet", "-m", "symlink fixture")
            commit = self.git(root, "rev-parse", "HEAD").decode().strip()
            with self.assertRaisesRegex(ValueError, "regular file"):
                sync._export_commit(root, commit, base / "snapshot")
            self.assertFalse((base / "snapshot").exists())

    def test_refresh_failure_also_cleans_private_snapshot(self):
        with tempfile.TemporaryDirectory() as directory:
            root, _, _ = self.repository(Path(directory)); seen = []
            def failed(source, *, committed):
                seen.append(source)
                raise ValueError("fixture failure")
            with patch.object(sync, "refresh", side_effect=failed), self.assertRaisesRegex(ValueError, "fixture failure"):
                sync.refresh_commit(root, "HEAD")
            self.assertEqual(len(seen), 1)
            self.assertFalse(seen[0].exists())

    def test_paths_reject_traversal_drives_ads_and_backslashes(self):
        for path in ("", "/absolute", "../escape", "one/../escape", "one/./dot",
                     "C:/drive", "one/file:ads", "one\\escape", "one//empty"):
            with self.subTest(path=path), self.assertRaises(ValueError):
                sync._snapshot_path(path)
        self.assertEqual(sync._snapshot_path("extlibs/demo/include/demo.h"), Path("extlibs/demo/include/demo.h"))

    def test_check_and_ref_are_rejected_before_any_work(self):
        with patch("sys.argv", ["sync_xrt.py", "--check", "--ref", "HEAD"]), \
             patch.object(sync, "check_lock") as check, \
             contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as error:
            sync.main()
        self.assertEqual(error.exception.code, 2); check.assert_not_called()


if __name__ == "__main__":
    unittest.main(verbosity=2)
