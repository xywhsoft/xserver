"""Compile and run XS boundary/failure probes, optionally with ASan/UBSan."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
# CpuArch.h intentionally casts unaligned x86 data in these fixture encoder TUs.
# Host code and the real decoder retain every ASan/UBSan check.
SANITIZER_ALIGNMENT_EXCLUSIONS = ('tcc/LzmaEnc.c', 'tcc/LzFind.c')


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
        encoder_objects = []
        if args.sanitize:
            print('SANITIZER alignment exception for vendor fixture encoder:',
                  ', '.join(SANITIZER_ALIGNMENT_EXCLUSIONS), flush=True)
        for name in ['LzmaEnc.c', 'LzFind.c', 'CpuArch.c']:
            source = 'tcc/' + name
            obj = exe.with_name(name + '.o')
            exception = ['-fno-sanitize=alignment'] if args.sanitize and source in SANITIZER_ALIGNMENT_EXCLUSIONS else []
            subprocess.run([args.cc, *flags, *exception, '-DZ7_ST', '-c', str(ROOT/source),
                            '-o', str(obj)], cwd=ROOT, check=True)
            encoder_objects.append(str(obj))
        subprocess.run([args.cc, *flags, str(ROOT/'tools/native_boundaries_test.c'),
                        '-DZ7_ST', str(ROOT/'tcc/LzmaDec.c'), *encoder_objects,
                        '-o', str(exe), *libraries], cwd=ROOT, check=True)
        subprocess.run([str(exe)], cwd=ROOT, check=True, timeout=60)
        # This probe replaces file/dir APIs with deterministic failure sources.
        # Whole-program optimization removes the uncalled pack/VFS entry points.
        pack_exe = exe.with_name('pack-' + exe.name)
        decoder_object = exe.with_name('mock-lzma.o')
        subprocess.run([args.cc, *flags, '-O1', '-c', str(ROOT/'tcc/LzmaDec.c'),
                        '-o', str(decoder_object)], cwd=ROOT, check=True)
        subprocess.run([args.cc, *flags, '-O1', '-fwhole-program',
                        str(ROOT/'tools/native_pack_test.c'), str(decoder_object),
                        '-o', str(pack_exe)],
                       cwd=ROOT, check=True)
        subprocess.run([str(pack_exe)], cwd=ROOT, check=True, timeout=60)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
