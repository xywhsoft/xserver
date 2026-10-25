"""Require clean GCC diagnostics in XS-owned code; keep vendor warnings visible."""
from pathlib import Path
import argparse
import json
import os
import re
import subprocess
import tempfile
from xs_extensions import ROOT, host_header, select_extensions


def owned(file):
    path = Path(file)
    path = path.resolve() if path.is_absolute() else (ROOT/path).resolve()
    return path == ROOT/'main.c' or path.is_relative_to(ROOT/'src')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='gcc')
    args = parser.parse_args()
    platform = 'windows' if os.name == 'nt' else 'linux'
    problems = []
    vendor = 0
    for variant in [[], ['all']]:
        selected = select_extensions(variant)
        with tempfile.TemporaryDirectory(prefix='xs-warnings-') as raw:
            folder = Path(raw)
            (folder/'xs_build_extensions.h').write_text(host_header(selected), encoding='utf-8')
            flags = ['-I',str(ROOT),'-I',str(ROOT/'lib'),'-I',str(ROOT/'tcc'),'-I',str(folder),
                     '-DCONFIG_TCC_BUILTIN_VFS','-Wall','-Wextra','-Wformat=2',
                     '-Wno-unused-function','-fsyntax-only','-fdiagnostics-color=never',
                     '-fno-diagnostics-show-caret']
            targets = [('main.c', 'c')]
            for entry in selected.values():
                flags += [f'-D{entry["macro"]}=1']
                features = entry.get('feature_macro', [])
                if isinstance(features, str): features = [features]
                flags += [f'-D{feature}=1' for feature in features]
                flags += [f'-I{directory}' for directory in entry.get('host_includes', [])]
                for source in entry['sources']:
                    if source['path'].startswith('src/') and platform in source.get('platforms', [platform]):
                        targets.append((source['path'],source.get('lang','c')))
            for source, language in targets:
                result = subprocess.run([args.cc,'-x',language,*flags,source], cwd=ROOT,
                                        capture_output=True,text=True,encoding='utf-8',errors='replace')
                diagnostics = []
                for line in result.stderr.splitlines():
                    match = re.match(r'^(.*?):(\d+):(?:(\d+):)? (warning|error|fatal error): (.*)$', line)
                    if match:
                        diagnostics.append(dict(file=match[1],line=int(match[2]),
                                                kind=match[4],message=match[5]))
                for diagnostic in diagnostics:
                    if diagnostic['kind'] in ['error','fatal error'] or owned(diagnostic['file']):
                        problems.append(diagnostic)
                    else: vendor += 1
                if result.returncode and not diagnostics:
                    raise RuntimeError(f'{source}: compiler exited {result.returncode}')
    for diagnostic in problems:
        print(json.dumps(diagnostic,ensure_ascii=False))
    if problems: raise SystemExit(f'FAIL {len(problems)} XS diagnostics')
    print(f'PASS XS default/all compiler diagnostics ({vendor} vendor diagnostics counted separately)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
