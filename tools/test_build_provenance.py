"""Release checks reject stale source, swapped binaries and partial variants."""
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path

from build_provenance import source_digest, verify_metadata, write_metadata


class ProvenanceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.binary = Path(self.temp.name) / "xs"
        self.binary.write_bytes(b"native binary")
        self.options = dict(source_hash="a" * 64, platform="linux", extensions=["sqlite"])
        write_metadata(self.binary, commit="abcdef0",
                       banner="[xs] XServer 1.0.0 (commit abcdef0, 2026-10-10, linux)",
                       **self.options)

    def test_valid_build(self):
        self.assertEqual(verify_metadata(self.binary, **self.options)["source_commit"], "abcdef0")

    def test_stale_source_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "stale source"):
            verify_metadata(self.binary, **{**self.options, "source_hash": "b" * 64})

    def test_replaced_binary_is_rejected(self):
        self.binary.write_bytes(b"different binary")
        with self.assertRaisesRegex(RuntimeError, "binary hash mismatch"):
            verify_metadata(self.binary, **self.options)

    def test_missing_or_partial_variant_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "variant mismatch"):
            verify_metadata(self.binary, **{**self.options, "extensions": ["sqlite", "xtp"]})
        self.binary.with_name("xs.build.json").unlink()
        with self.assertRaisesRegex(RuntimeError, "missing build metadata"):
            verify_metadata(self.binary, **self.options)

    def test_dirty_source_is_rejected(self):
        write_metadata(self.binary, commit="abcdef0-dirty", banner="dirty", **self.options)
        with self.assertRaisesRegex(RuntimeError, "uncommitted"):
            verify_metadata(self.binary, **self.options)

    def test_source_hash_tracks_native_and_sdk_inputs(self):
        from build_provenance import SOURCE_INPUTS
        root = Path(self.temp.name) / "source"
        for relative in SOURCE_INPUTS:
            path = root / relative
            if relative in ("src", "lib", "tcc", "res"):
                path.mkdir(parents=True)
                path = path / "header.h"
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"original")
        original = source_digest(root)
        (root / "src/header.h").write_bytes(b"changed ABI")
        self.assertNotEqual(source_digest(root), original)
        (root / "src/header.h").write_bytes(b"original")
        self.assertEqual(source_digest(root), original)
        (root / "README.md").write_bytes(b"documentation only")
        self.assertEqual(source_digest(root), original)

    def test_source_hash_uses_the_same_case_sensitive_order_on_every_platform(self):
        root = Path(self.temp.name) / "case-order"
        (root / "src").mkdir(parents=True)
        (root / "src/alpha.h").write_bytes(b"a")
        (root / "src/Zebra.h").write_bytes(b"z")
        with patch("build_provenance.SOURCE_INPUTS", ("src",)):
            self.assertEqual(source_digest(root),
                             "9e8c49d744dfaf9b0aabea8ebab33f37982e17f9910da212224bba4e5e8b2929")


if __name__ == "__main__":
    unittest.main(verbosity=2)
