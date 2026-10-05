"""HTTP idle ownership regression: two quiet connections, no load generation."""
import argparse,json,os,shutil,socket,ssl,subprocess,tempfile,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SCRIPT=r'''
#include <xsbase.h>
#include <string.h>
static xthread* Worker;
static xnetstream* Tcp;
static xtlsstream* Tls;
static bool Send(xnetstream* tcp,xtlsstream* tls,const char* text) {
    xdeadline until=xrtDeadlineAfter(2000000u);
    if(tls) {
        xfuture* sent=xrtTlsStreamSendAsync(tls,text,strlen(text));
        bool ok=sent&&xrtFutureWaitUntil(sent,until)==XWAIT_OK&&xrtFutureState(sent)==XFUTURE_RESOLVED;
        xrtFutureDestroy(sent);return ok;
    }
    return xrtNetStreamSend(tcp,text,strlen(text))==XNET_RESULT_OK&&
        xrtNetStreamWait(tcp,XNET_STREAM_WAIT_DRAIN,until,NULL);
}
static int32 Quiet(void* unused) {
    (void)unused;
    if(Send(Tcp,Tls,"HTTP/1.1 200 OK\r\nContent-Length:10\r\n\r\nREADY")) {
        xrtSleep(2500);(void)Send(Tcp,Tls,"ALIVE");
    }
    return 0;
}
void ServiceUnit(XS_HostInfo* host) {
    (void)host;if(Worker){xrtThreadWait(Worker);xrtThreadDestroy(Worker);}
    xrtNetStreamDestroy(Tcp);xrtTlsStreamDestroy(Tls);
}
XS_RequestResult RequestProc(XS_HttpReq* req) {
    if(xrtStrEqual(req->head->Target,XRT_STR_LITERAL("/takeover"))) {
        Tcp=req->tcp?xrtNetStreamRef(req->tcp):NULL;
        Tls=req->tls?xrtTlsStreamRef(req->tls):NULL;
        Worker=xrtThreadCreate(Quiet,NULL,0);
        return Worker?XS_TAKEOVER:XS_FALLBACK;
    }
    const char* reply="HTTP/1.1 200 OK\r\nContent-Length:2\r\n\r\nOK";
    size_t written=0;
    if(req->tls)return xrtTlsStreamSend(req->tls,reply,strlen(reply),&written)==XTLS_OK&&written==strlen(reply)?XS_OK:XS_FALLBACK;
    return xrtNetStreamSend(req->tcp,reply,strlen(reply))==XNET_RESULT_OK?XS_OK:XS_FALLBACK;
}
'''
def free_port():
    with socket.socket() as s:s.bind(('127.0.0.1',0));return s.getsockname()[1]
def read_head(s):
    data=b''
    while b'\r\n\r\n' not in data:
        chunk=s.recv(4096)
        if not chunk:raise EOFError('closed before response')
        data+=chunk
    head,body=data.split(b'\r\n\r\n',1)
    assert head.startswith(b'HTTP/1.1 200 '),head
    return body
def check(executable,secure):
    with tempfile.TemporaryDirectory(prefix='xs-takeover-idle-') as raw:
        site=Path(raw);(site/'web').mkdir();(site/'main.c').write_text(SCRIPT,encoding='utf-8')
        port=free_port();plain=free_port()
        service={'class':'http','name':'idle-test','ip':'127.0.0.1','port':port,'idle_timeout':1000,
                 'host_default':{'path':'web','devfile':'main.c','devlang':'c'}}
        if secure:
            for kind in ('cert','key'):shutil.copy2(ROOT/f'release/tls/xtps_{kind}.pem',site/f'{kind}.pem')
            service.update(tls=True,port=plain,port_tls=port)
            service['host_default'].update(tls_cert='cert.pem',tls_key='key.pem')
        config=site/'xs.json';config.write_text(json.dumps({'services':[service]}))
        def connect():
            s=socket.create_connection(('127.0.0.1',port),timeout=5)
            if secure:
                # Committed loopback certificate; certificate verification has
                # separate outbound TLS tests. This case checks idle ownership.
                context=ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
                context.check_hostname=False;context.verify_mode=ssl.CERT_NONE
                s=context.wrap_socket(s,server_hostname='localhost')
            return s
        with (site/'test.log').open('wb') as log:
            p=subprocess.Popen([str(executable),str(config)],cwd=site,stdout=log,stderr=log,
                               creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
            try:
                for _ in range(100):
                    assert p.poll() is None,(site/'test.log').read_text(errors='replace')
                    try:
                        with connect() as s:
                            s.sendall(b'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n');assert read_head(s)==b'OK'
                        break
                    except OSError:time.sleep(.05)
                else:raise AssertionError('host did not start')
                with connect() as s:
                    s.sendall(b'GET /takeover HTTP/1.1\r\nHost: localhost\r\n\r\n')
                    body=read_head(s)
                    while len(body)<10:
                        chunk=s.recv(10-len(body))
                        assert chunk,'HTTP idle incorrectly closed the taken-over connection'
                        body+=chunk
                    assert body==b'READYALIVE',body
                with connect() as s:
                    s.sendall(b'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n');assert read_head(s)==b'OK'
                    s.settimeout(.15)
                    try:assert not s.recv(1),'unexpected response bytes';raise AssertionError('HTTP connection closed immediately')
                    except socket.timeout:pass
                    s.settimeout(3);assert s.recv(1)==b'','ordinary HTTP idle must remain effective'
                print('PASS '+('TLS' if secure else 'TCP')+' takeover survives HTTP idle; ordinary keepalive still expires')
            finally:
                p.terminate();p.wait(15)
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,required=True)
    exe=parser.parse_args().exe.resolve()
    for secure in (False,True):check(exe,secure)
