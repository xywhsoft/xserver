"""Build the standalone xrudp Binding reflector from the locked XRT snapshot."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import tempfile
import sync_xrt

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc',default='gcc')
    parser.add_argument('--output',type=Path,default=ROOT/'.build/xrudp-stun'/('xrudp-stun.exe' if os.name=='nt' else 'xrudp-stun'))
    parser.add_argument('--sanitize',action='store_true')
    parser.add_argument('--static',action='store_true')
    args=parser.parse_args()
    if args.static and (args.sanitize or os.name=='nt'): parser.error('--static requires a nonsanitized Linux build')
    sync_xrt.check_lock()
    source=ROOT/'services/xrudp-stun/server.c'
    wire=ROOT/'lib/xrudp/src/wire/rudp_stun.c'
    paths=[Path(__file__).resolve(),source,wire,ROOT/'services/xrudp-stun/xrudp-stun.service',
           ROOT/'lib/xrt.h',ROOT/'lib/xrt_decl.h',ROOT/'lib/xrt_sources.lock.json']
    paths+=sorted((ROOT/'lib/xrudp/include').rglob('*.h'))
    paths+=sorted((ROOT/'lib/xrtshim').rglob('*.h'))
    hashes={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    output=args.output.resolve(); output.parent.mkdir(parents=True,exist_ok=True)
    flags=['-std=gnu11','-O2','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
           '-DXRUDP_MODULE_RUDP_STUN=1','-I',str(ROOT/'lib'),'-I',str(ROOT/'lib/xrudp/include'),'-I',str(ROOT/'lib/xrtshim')]
    if args.sanitize: flags+=['-g','-fno-omit-frame-pointer','-fsanitize=address,undefined','-fno-sanitize-recover=all']
    if args.static: flags+=['-static']
    flags+=['-Wl,--gc-sections']
    links=['-lws2_32','-liphlpapi'] if os.name=='nt' else ['-pthread','-lm','-ldl']
    with tempfile.TemporaryDirectory(prefix='stun-build-',dir=output.parent) as raw:
        staged=Path(raw)/output.name
        subprocess.run([args.cc,*flags,str(source),str(wire),'-o',str(staged),*links],cwd=ROOT,check=True)
        for relative,digest in hashes.items():
            if hashlib.sha256((ROOT/relative).read_bytes()).hexdigest()!=digest: raise RuntimeError('source changed during build: '+relative)
        os.replace(staged,output)
    snapshot=json.loads((ROOT/'lib/xrt_sources.lock.json').read_text(encoding='utf-8'))
    revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    dirty=bool(subprocess.check_output(['git','status','--porcelain'],cwd=ROOT).strip())
    version=subprocess.check_output([args.cc,'--version'],text=True).splitlines()[0]
    receipt={'schema':1,'xs_commit':revision,'xs_worktree_dirty':dirty,'xrt_commit':snapshot['upstream_commit'],
             'source_hashes':hashes,'compiler':args.cc,'compiler_version':version,'compile_flags':flags,'link_flags':links,
             'static':args.static,'sanitize':args.sanitize,'sha256':hashlib.sha256(output.read_bytes()).hexdigest(),'bytes':output.stat().st_size}
    output.with_name(output.name+'.build.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('built',output,receipt['bytes'],receipt['sha256'])
    return 0


if __name__=='__main__': raise SystemExit(main())
