"""SQLite extension dependency, source identity and native integration contracts."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from xs_extensions import ROOT, select_extensions, host_header
from vendor_sqlite_extensions import check_lock


class SQLiteExtensionTests(unittest.TestCase):
    def test_independent_choices_share_one_sqlite(self):
        for requested in (['graphqlite'], ['sqlite-vec'], ['sqlite-vec','graphqlite'], ['all']):
            with self.subTest(requested=requested):
                selected = select_extensions(requested)
                sources = [s['path'] for e in selected.values() for s in e['sources']]
                self.assertIn('sqlite', selected)
                self.assertEqual(sources.count('lib/sqlite3.c'), 1)
                self.assertNotIn('xruntime', selected)
        self.assertEqual(set(select_extensions(['graphqlite'])), {'sqlite','graphqlite'})
        self.assertEqual(set(select_extensions(['sqlite-vec'])), {'sqlite','sqlite-vec'})

    def test_no_process_wide_auto_registration_or_public_api_redirection(self):
        for name in ('graphqlite','sqlite-vec'):
            selected = select_extensions([name])
            header = host_header(selected)
            self.assertNotIn('sqlite3_auto_extension(', header)
            self.assertNotIn('"SQLITE_CORE"', header)
            sdk = (ROOT/selected[name]['headers'][name+'.h']).read_text()
            self.assertNotIn('#include "sqlite3ext.h"', sdk)
            self.assertIn('sqlite3.h', sdk)
        for s in select_extensions(['sqlite-vec'])['sqlite-vec']['sources']:
            self.assertIn('SQLITE_CORE', s['defines'])

    def test_pinned_sources_and_generated_parser(self):
        check_lock()
        lock = json.loads((ROOT/'lib/sqlite_extensions.lock.json').read_text())
        for filename in ('cypher_scanner.c','cypher_gram.tab.c','cypher_flex.h','cypher_gram.tab.h'):
            self.assertIn('lib/graphqlite/generated/'+filename, lock['files'])

    def test_source_drift_and_path_escape_fail(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); (root/'lib').mkdir()
            p = root/'lib/source.c'; p.write_bytes(b'expected')
            lock = root/'lib/sqlite_extensions.lock.json'
            lock.write_text(json.dumps({'schema':1,'files':{'lib/source.c':{'sha256':hashlib.sha256(b'expected').hexdigest()}}}))
            check_lock(root)
            p.write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError,'snapshot drift'):
                check_lock(root)
            lock.write_text(json.dumps({'schema':1,'files':{'../escape.c':{'sha256':'0'*64}}}))
            with self.assertRaisesRegex(ValueError,'escaped'):
                check_lock(root)


if __name__ == '__main__':
    unittest.main()
