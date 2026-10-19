import subprocess
import unittest
from unittest.mock import patch
from compiler_paths import linux_tcc_triplet

class CompilerPathsTests(unittest.TestCase):
    def test_gcc_uses_its_multiarch_contract(self):
        with patch('compiler_paths.subprocess.run', return_value=subprocess.CompletedProcess([], 0, 'x86_64-linux-gnu\n', '')):
            self.assertEqual(linux_tcc_triplet('gcc'), 'x86_64-linux-gnu')

    def test_clang_uses_reported_libc_headers_instead_of_vendor_triplet(self):
        search = 'clang version\n#include <...> search starts here:\n /usr/lib/clang/18/include\n /usr/local/include\n /usr/include/x86_64-linux-gnu\n /usr/include\nEnd of search list.\n'
        with patch('compiler_paths.subprocess.run', side_effect=[subprocess.CompletedProcess([], 1, '', 'unsupported'), subprocess.CompletedProcess([], 0, '', search)]), patch('compiler_paths.subprocess.check_output', return_value='x86_64-pc-linux-gnu\n'):
            self.assertEqual(linux_tcc_triplet('clang'), 'x86_64-linux-gnu')

    def test_flat_libc_layout_preserves_target(self):
        with patch('compiler_paths.subprocess.run', side_effect=[subprocess.CompletedProcess([], 1, '', ''), subprocess.CompletedProcess([], 0, '', '#include <...> search starts here:\n /usr/include\nEnd of search list.\n')]), patch('compiler_paths.subprocess.check_output', return_value='aarch64-unknown-linux-musl\n'):
            self.assertEqual(linux_tcc_triplet('clang'), 'aarch64-unknown-linux-musl')

    def test_invalid_target_cannot_be_embedded_as_a_define(self):
        with patch('compiler_paths.subprocess.run', return_value=subprocess.CompletedProcess([], 1, '', '')), patch('compiler_paths.subprocess.check_output', return_value='bad"\n#define injected'):
            with self.assertRaises(ValueError):
                linux_tcc_triplet('compiler')

if __name__ == '__main__':
    unittest.main()
