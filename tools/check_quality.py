"""Repeatable XS quality gate; use --exe for an isolated host regression pass."""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import os
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]


def test_inputs():
    return {path.name:hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted((ROOT/'tools').iterdir())
            if path.is_file() and path.suffix in ['.py','.c','.json']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--sanitize', action='store_true', help='ASan/UBSan for native boundary probes (not JIT-generated script code)')
    parser.add_argument('--report', type=Path, default=ROOT/'.build/quality-report.json')
    args = parser.parse_args()
    from build_provenance import source_digest
    source_hash = source_digest(ROOT)
    tests_hash = test_inputs()
    commands = []
    for pattern in ['test_extensions.py','test_sync_xrt.py','test_xrt_sync.py',
                    'test_build_provenance.py','test_compiler_paths.py']:
        commands.append(['-m','unittest','discover','-s','tools','-p',pattern])
    commands += [['tools/sync_xrt.py','--check'],
                 ['tools/check_xs_warnings.py','--cc',args.cc],
                 ['tools/test_native_boundaries.py','--cc',args.cc]+(['--sanitize'] if args.sanitize else []),
                 ['tools/test_sdk_abi.py','--cc',args.cc]]
    if args.exe:
        exe = args.exe.resolve()
        metadata = json.loads(exe.with_name(exe.name+'.build.json').read_text(encoding='utf-8'))
        assert hashlib.sha256(exe.read_bytes()).hexdigest() == metadata['binary_sha256'], 'executable hash differs from its receipt'
        assert source_hash == metadata['source_sha256'], 'build inputs changed; rebuild the candidate'
        extensions = metadata['extensions']
        commands += [['tools/test_sdk_abi.py','--cc',args.cc,'--exe',str(exe)],
                     ['tools/test_extensions_runtime.py','--exe',str(exe),*extensions]]
        for test in ['func_test.py','lifecycle_reload_test.py','reload_matrix_test.py',
                     'test_host_budgets.py','test_http_tls_receive.py','test_http_takeover_idle.py',
                     'test_site_vfs.py','test_packed_reload.py','test_pack_validate.py']:
            commands.append(['tools/'+test,'--exe',str(exe)])
        if 'xrudp' in extensions:
            commands.append(['tools/xrudp_service_test.py','--exe',str(exe)])
        if os.name != 'nt':
            commands.append(['tools/test_tcc_clock.py','--host',str(exe)])
    report = {'schema':1,'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'executable':str(args.exe.resolve()) if args.exe else None,
              'source_sha256':source_hash,'test_inputs_sha256':tests_hash,
              'binary_sha256':metadata['binary_sha256'] if args.exe else None,
              'native_sanitizers':args.sanitize,'checks':[]}
    code = 0
    try:
        for arguments in commands:
            print('CHECK', ' '.join(arguments), flush=True)
            started = time.monotonic()
            result = subprocess.run([sys.executable,'-X','utf8',*arguments],cwd=ROOT)
            report['checks'].append({'arguments':arguments,'seconds':round(time.monotonic()-started,3),
                                     'returncode':result.returncode})
            if result.returncode:
                code = result.returncode
                break
    finally:
        if source_digest(ROOT) != source_hash or test_inputs() != tests_hash:
            print('FAIL inputs changed during the quality gate')
            code = code or 1
        report['passed'] = code == 0 and len(report['checks']) == len(commands)
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(('PASS' if report['passed'] else 'FAIL'),len(report['checks']),'quality checks;',args.report)
    return code


if __name__ == '__main__':
    raise SystemExit(main())
