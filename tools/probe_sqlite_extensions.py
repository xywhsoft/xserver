"""Build native graph/vector coexistence and initialization failure probes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
from xs_extensions import ROOT, select_extensions
from vendor_sqlite_extensions import check_lock

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cc', default='gcc')
    p.add_argument('--sanitize', action='store_true')
    p.add_argument('--build-dir', type=Path, required=True)
    p.add_argument('--mode', choices=['normal','schema','busy','all'], default='all')
    a = p.parse_args()
    check_lock()
    selected = select_extensions(['graphqlite','sqlite-vec'])
    platform = 'windows' if os.name == 'nt' else 'linux'
    folder = a.build_dir.resolve(); folder.mkdir(parents=True, exist_ok=True)
    includes = ['-I'+str(ROOT), '-I'+str(ROOT/'lib')]
    for entry in selected.values():
        includes += ['-I'+str(ROOT/d) for d in entry.get('host_includes', [])]
    flags = ['-O1','-g',*includes]
    if a.sanitize:
        flags += ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    headers = hashlib.sha256()
    for directory in ('lib/graphqlite','lib/tre','lib/sqlite-vec'):
        for path in sorted((ROOT/directory).rglob('*.h')):
            headers.update(path.read_bytes())
    for header in ('lib/sqlite3.h', 'lib/sqlite3ext.h', 'src/sdk/graphqlite.h', 'src/sdk/sqlite-vec.h'):
        headers.update((ROOT/header).read_bytes())
    compiler = shutil.which(a.cc)
    if compiler is None:
        p.error(f'compiler not found: {a.cc}')
    headers.update(subprocess.check_output([compiler, '--version']))
    objects = []
    sources = [s for e in selected.values() for s in e['sources'] if platform in s.get('platforms',[platform])]
    sources += [{'path':'tools/native_sqlite_extensions_test.c'}]
    for index,source in enumerate(sources):
        obj = folder/f'{index}.o'
        command = [a.cc,*flags,*source.get('flags',[]),*['-D'+d for d in source.get('defines',[])],'-c',source['path'],'-o',str(obj)]
        key = hashlib.sha256(json.dumps(command).encode()+headers.digest()+(ROOT/source['path']).read_bytes()).hexdigest()
        receipt = obj.with_suffix('.sha256')
        if not obj.exists() or not receipt.exists() or receipt.read_text()!=key:
            print('COMPILE',source['path'],flush=True)
            subprocess.run(command,cwd=ROOT,check=True)
            receipt.write_text(key)
        objects.append(str(obj))
    exe = folder/('probe.exe' if os.name=='nt' else 'probe')
    subprocess.run([a.cc,*flags,*objects,'-o',str(exe),*(['-lm','-ldl','-pthread'] if os.name!='nt' else [])],cwd=ROOT,check=True)
    for mode in (['normal','schema','busy'] if a.mode=='all' else [a.mode]):
        subprocess.run([str(exe),mode],cwd=ROOT,check=True,timeout=60)

if __name__=='__main__':
    main()
