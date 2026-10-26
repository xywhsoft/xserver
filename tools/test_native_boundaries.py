"""Compile and run XS boundary/failure probes, optionally with ASan/UBSan."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='xs-boundaries-') as raw:
        exe = Path(raw) / ('probe.exe' if os.name == 'nt' else 'probe')
        flags = ['-std=gnu11', '-O0', '-g', '-ffunction-sections', '-fdata-sections',
                 '-Wl,--gc-sections', '-I', str(ROOT), '-I', str(ROOT/'lib'),
                 '-I', str(ROOT/'tcc')]
        if args.sanitize:
            flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                      '-fno-omit-frame-pointer']
        libraries = ['-lws2_32', '-liphlpapi'] if os.name == 'nt' else ['-pthread', '-ldl', '-lm']
        subprocess.run([args.cc, *flags, str(ROOT/'tools/native_boundaries_test.c'),
                        '-DZ7_ST', *[str(ROOT/'tcc'/name) for name in
                                     ['LzmaDec.c', 'LzmaEnc.c', 'LzFind.c', 'CpuArch.c']],
                        '-o', str(exe), *libraries], cwd=ROOT, check=True)
        subprocess.run([str(exe)], cwd=ROOT, check=True, timeout=60)
        # This probe replaces file/dir APIs with deterministic failure sources.
        # Whole-program optimization removes the uncalled pack/VFS entry points.
        pack_exe = exe.with_name('pack-' + exe.name)
        subprocess.run([args.cc, *flags, '-O1', '-fwhole-program',
                        str(ROOT/'tools/native_pack_test.c'), '-o', str(pack_exe)],
                       cwd=ROOT, check=True)
        subprocess.run([str(pack_exe)], cwd=ROOT, check=True, timeout=60)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
