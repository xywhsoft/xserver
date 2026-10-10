"""POSIX clock regression for real xs, including static musl and nested TCC."""
import argparse
import json
import os
from pathlib import Path
import socket
import subprocess
import tempfile
import time

PROBE = r'''
#include <xsbase.h>
#include <libtcc.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
int ClockCheck(void) {
    const char* marker=getenv("XS_TCC_CLOCK_TEST");
    if (!marker || strcmp(marker,"host-process-marker")) return 0;
    time_t stored=0, a=time(NULL), b=time(&stored);
    struct timeval tv; struct timespec wall,mono,res;
    if (a<1760000000 || b<a || b-a>2 || stored!=b) return 0;
    if (gettimeofday(&tv,NULL) || clock_gettime(CLOCK_REALTIME,&wall) ||
        clock_gettime(CLOCK_MONOTONIC,&mono) || clock_getres(CLOCK_MONOTONIC,&res)) return 0;
    if (wall.tv_sec<a || wall.tv_sec-a>2 || tv.tv_sec<a || tv.tv_sec-a>2 ||
        tv.tv_usec<0 || tv.tv_usec>=1000000 || wall.tv_nsec<0 || wall.tv_nsec>=1000000000 ||
        mono.tv_sec<0 || mono.tv_nsec<0 || mono.tv_nsec>=1000000000 ||
        res.tv_sec<0 || res.tv_nsec<0 || res.tv_nsec>=1000000000) return 0;
    return 1;
}

void ServiceInit(XS_HostInfo* h) {
    (void)h; TCCState* state=xsCreateTCC();
    int ok=ClockCheck() && ClockCheck() && state &&
        tcc_compile_string(state,"#include <time.h>\n#include <sys/time.h>\n#include <stdlib.h>\n#include <string.h>\nint ClockCheck(void) {\n    const char* marker=getenv(\"XS_TCC_CLOCK_TEST\");\n    if (!marker || strcmp(marker,\"host-process-marker\")) return 0;\n    time_t stored=0, a=time(NULL), b=time(&stored);\n    struct timeval tv; struct timespec wall,mono,res;\n    if (a<1760000000 || b<a || b-a>2 || stored!=b) return 0;\n    if (gettimeofday(&tv,NULL) || clock_gettime(CLOCK_REALTIME,&wall) ||\n        clock_gettime(CLOCK_MONOTONIC,&mono) || clock_getres(CLOCK_MONOTONIC,&res)) return 0;\n    if (wall.tv_sec<a || wall.tv_sec-a>2 || tv.tv_sec<a || tv.tv_sec-a>2 ||\n        tv.tv_usec<0 || tv.tv_usec>=1000000 || wall.tv_nsec<0 || wall.tv_nsec>=1000000000 ||\n        mono.tv_sec<0 || mono.tv_nsec<0 || mono.tv_nsec>=1000000000 ||\n        res.tv_sec<0 || res.tv_nsec<0 || res.tv_nsec>=1000000000) return 0;\n    return 1;\n}\n")==0 && tcc_relocate(state)==0;
    int (*nested)(void)=ok?(int(*)(void))tcc_get_symbol(state,"ClockCheck"):NULL;
    ok=ok && nested && nested() && nested();
    if(state) tcc_delete(state);
    printf("%s TCC native clock outer/nested/first/repeated\n",ok?"PASS":"FAIL"); fflush(stdout);
}
void ServiceUnit(XS_HostInfo* h){(void)h;}
XS_RequestResult RequestProc(XS_HttpReq* r){(void)r;return XS_FALLBACK;}
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--host',type=Path,required=True)
    host=parser.parse_args().host.resolve()
    # Retain the disposable fixture under .build for diagnosis; never inspect
    # or delete another application's portable directory.
    base=Path(__file__).resolve().parents[1]/'.build'; base.mkdir(exist_ok=True)
    site=Path(tempfile.mkdtemp(prefix='tcc-clock-',dir=base)); (site/'web').mkdir()
    (site/'probe.c').write_text(PROBE,encoding='utf-8')
    with socket.socket() as sock: sock.bind(('127.0.0.1',0)); port=sock.getsockname()[1]
    (site/'xs.json').write_text(json.dumps({'services':[{'enabled':True,'class':'http','name':'tcc-clock-test',
        'ip':'127.0.0.1','port':port,'host_default':{'enabled':True,'name':'test','path':'web','devlang':'c','devfile':'probe.c'}}]}))
    log=site/'test.log'
    with log.open('wb') as stream:
        process=subprocess.Popen([str(host),str(site/'xs.json')],cwd=site,env={**os.environ,"XS_TCC_CLOCK_TEST":"host-process-marker"},stdout=stream,stderr=subprocess.STDOUT)
        try:
            for _ in range(100):
                output=log.read_text(encoding='utf-8',errors='replace')
                if 'PASS TCC native clock' in output: print('PASS TCC native clock outer/nested/first/repeated'); return
                if 'FAIL TCC native clock' in output or process.poll() is not None: raise RuntimeError(output)
                time.sleep(.1)
            raise RuntimeError(log.read_text(encoding='utf-8',errors='replace'))
        finally:
            if process.poll() is None: process.terminate(); process.wait(timeout=15)


if __name__=='__main__': main()
