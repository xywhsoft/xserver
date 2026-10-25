"""Check the frozen x86-64 SDK layout natively and in outer/nested TCC."""
from pathlib import Path
import argparse
import json
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT/'tools/sdk_abi_x64.json'
FIELDS = {
    'XS_ReloadResult': 'Id State Revision Target Message',
    'XS_HostInfo': 'Enabled Name Host Path DevLang DevFile TlsCA TlsCert TlsKey State Runtime Server Custom DevInc DevLib RuntimeLock',
    'XS_ServerInfo': 'Enabled Class Name IP Port TLS PortTLS IPTLS Backlog RecvLimit DefaultHost HostCount Hosts State Runtime Engine Custom Generation ConfigOwner PortBound',
    'XS_HttpReq': 'tcp tls head body host server',
    'XS_StreamConn': 'tcp tls',
}
CONSTANTS = ['XS_SDK_ABI_VERSION', 'XS_RUN_STARTING', 'XS_RUN_RUNNING', 'XS_RUN_RELOAD_FAILED',
             'XS_RUN_STOPPING', 'XS_RUN_STOPPED', 'XS_RELOAD_UNKNOWN', 'XS_RELOAD_ACCEPTED',
             'XS_RELOAD_PREPARING', 'XS_RELOAD_SUCCEEDED', 'XS_RELOAD_FAILED',
             'XS_RELOAD_SUPERSEDED', 'XS_RELOAD_CANCELLED', 'XS_OK', 'XS_FALLBACK', 'XS_TAKEOVER']


def expressions():
    values = {'pointer_bytes':'sizeof(void*)', 'bool_bytes':'sizeof(bool)'}
    for name, fields in FIELDS.items():
        values[name+'.size'] = f'sizeof({name})'
        for field in fields.split():
            values[name+'.'+field] = f'offsetof({name},{field})'
    values.update({name:name for name in CONSTANTS})
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--exe', type=Path)
    parser.add_argument('--record', action='store_true', help='explicitly replace the reviewed native ABI baseline')
    args = parser.parse_args()
    expressions_ = expressions()
    with tempfile.TemporaryDirectory(prefix='xs-sdk-abi-') as raw:
        folder = Path(raw)
        source = folder/'layout.c'
        program = '#include "src/sdk/xsbase.h"\n#include <stddef.h>\n#include <stdio.h>\nint main(void){\n'
        for key, expression in expressions_.items():
            program += f'printf("{key}=%llu\\n",(unsigned long long)({expression}));\n'
        program += 'return 0;}\n'
        source.write_text(program, encoding='utf-8')
        probe = folder/('layout.exe' if os.name == 'nt' else 'layout')
        subprocess.run([args.cc, '-std=c11', '-I', str(ROOT), '-I', str(ROOT/'lib'),
                        str(source), '-o', str(probe)], check=True)
        output = subprocess.check_output([str(probe)], text=True)
        actual = {key:int(value) for key,value in (line.split('=') for line in output.splitlines())}
        assert actual['pointer_bytes'] == 8, 'the checked SDK target is x86-64'
        if args.record:
            BASELINE.write_text(json.dumps(actual, indent=2)+'\n', encoding='utf-8')
        expected = json.loads(BASELINE.read_text(encoding='utf-8'))
        assert actual == expected, {key:(expected.get(key), value) for key,value in actual.items() if expected.get(key) != value}
        print('PASS native SDK ABI:', len(actual), 'sizes, offsets and enum values')
        if args.exe:
            assertions = '#include <xsbase.h>\n#include <stddef.h>\n'
            for index, (key, expression) in enumerate(expressions_.items()):
                assertions += f'typedef char abi_check_{index}[({expression})=={expected[key]}?1:-1];\n'
            nested = assertions+'int abi_probe(void){return 731;}\n'
            outer = assertions+'#include <stdio.h>\n#include <stdlib.h>\n'
            outer += 'void ServiceInit(XS_HostInfo* host){(void)host;TCCState* t=xsCreateTCC();\n'
            outer += 'if(!t || tcc_compile_string(t,'+json.dumps(nested)+')<0 || tcc_relocate(t)<0)exit(12);\n'
            outer += 'int(*p)(void)=(int(*)(void))tcc_get_symbol(t,"abi_probe");if(!p||p()!=731)exit(13);xsDestroyTCC(t);puts("PASS outer/nested TCC SDK ABI");exit(0);}\n'
            (folder/'abi.c').write_text(outer, encoding='utf-8')
            (folder/'xs.json').write_text(json.dumps({'services':[{'class':'custom','name':'abi','devfile':'abi.c'}]}), encoding='utf-8')
            result = subprocess.run([str(args.exe.resolve()), str(folder/'xs.json')], capture_output=True,
                                    text=True, encoding='utf-8', errors='replace', timeout=30)
            assert result.returncode == 0 and 'PASS outer/nested TCC SDK ABI' in result.stdout, f'rc={result.returncode}\n'+result.stdout+result.stderr
            print('PASS outer/nested TCC SDK ABI')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
