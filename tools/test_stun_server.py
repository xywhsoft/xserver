"""Independent stdlib UDP oracle for the real bounded Binding daemon."""
from pathlib import Path
import argparse
import binascii
import ipaddress
import os
import queue
import re
import select
import socket
import struct
import subprocess
import threading
import time

COOKIE=0x2112A442


def attribute(kind, data=b''):
    return struct.pack('!HH',kind,len(data))+data+b'\0'*((-len(data))%4)


def packet(kind=1, attrs=b'', fingerprint=True):
    transaction=os.urandom(12)
    raw=struct.pack('!HHI',kind,len(attrs)+(8 if fingerprint else 0),COOKIE)+transaction+attrs
    if fingerprint: raw+=attribute(0x8028,struct.pack('!I',binascii.crc32(raw)^0x5354554e))
    return raw


def decoded(raw, request, address, *, error=False, fingerprint=True):
    kind,length,cookie=struct.unpack('!HHI',raw[:8])
    assert kind==(0x111 if error else 0x101) and cookie==COOKIE and length+20==len(raw)
    assert raw[8:20]==request[8:20] and len(raw)<=3*len(request)
    fields={}; at=20
    while at<len(raw):
        kind,size=struct.unpack('!HH',raw[at:at+4]); data=raw[at+4:at+4+size]
        assert len(data)==size; fields[kind]=data
        if kind==0x8028: assert data==struct.pack('!I',binascii.crc32(raw[:at])^0x5354554e) and at+8==len(raw)
        at+=4+((size+3)//4)*4
    assert (0x8028 in fields)==fingerprint
    if error:
        assert fields[9][2:]==b'\x04\x14' and struct.unpack('!HH',fields[0xa])==(0x123,0x124)
    else:
        mapped=fields[0x20]; family=socket.AF_INET if mapped[1]==1 else socket.AF_INET6
        port=struct.unpack('!H',mapped[2:4])[0]^0x2112
        mask=struct.pack('!I',COOKIE)+request[8:20]
        ip=socket.inet_ntop(family,bytes(value^mask[i] for i,value in enumerate(mapped[4:])))
        assert ipaddress.ip_address(ip)==ipaddress.ip_address(address[0]) and port==address[1],(ip,port,address)
    return fields


class Server:
    def __init__(self,exe,*args,run_ms=4000):
        self.lines=[]; self.events=queue.Queue(); self.ports={}
        self.process=subprocess.Popen([str(exe),'--bind4','127.0.0.1','--bind6','::1','--port','0',
            *args,*(['--run-ms',str(run_ms)] if run_ms else [])],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
            text=True,encoding='utf-8',errors='replace',env={**os.environ,'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
        def read():
            for line in self.process.stdout:
                self.lines.append(line); self.events.put(line)
            self.events.put(None)
        self.reader=threading.Thread(target=read,daemon=True); self.reader.start()
        until=time.monotonic()+10
        while time.monotonic()<until:
            line=self.events.get(timeout=max(.01,until-time.monotonic()))
            if line is None: raise AssertionError('startup failed: '+''.join(self.lines))
            found=re.match(r'BIND family=(4|6) address=.* port=(\d+)',line)
            if found: self.ports[int(found[1])]=int(found[2])
            if line.startswith('READY'): break
        else: raise AssertionError('startup deadline: '+''.join(self.lines))

    def stop(self):
        if self.process.poll() is None: self.process.terminate()
        self.process.wait(timeout=10); self.reader.join(timeout=2)

    def finish(self):
        result=self.process.wait(timeout=10); self.reader.join(timeout=2)
        assert result==0 and 'STOP joined=1 clean=1\n' in self.lines,(result,''.join(self.lines))
        line=next(value for value in self.lines if value.startswith('STATS'))
        return {key:int(value) for key,value in re.findall(r'(\w+)=(\d+)',line)}


def client(family, port):
    s=socket.socket(socket.AF_INET if family==4 else socket.AF_INET6,socket.SOCK_DGRAM)
    s.bind(('127.0.0.1' if family==4 else '::1',0)); s.settimeout(.1)
    return s,('127.0.0.1' if family==4 else '::1',port)


def protocol(exe):
    server=Server(exe,'--rate','4096','--per-source','4096')
    try:
        assert set(server.ports)=={4,6}
        for family,port in server.ports.items():
            s,target=client(family,port)
            try:
                for fingerprint in (True,False):
                    request=packet(fingerprint=fingerprint); s.sendto(request,target)
                    response,source=s.recvfrom(512); assert source[1]==port
                    decoded(response,request,s.getsockname(),fingerprint=fingerprint)
                request=packet(attrs=attribute(0x8123,b'optional-padding'))
                s.sendto(request,target); decoded(s.recvfrom(512)[0],request,s.getsockname())
                request=packet(attrs=attribute(0x123)+attribute(0x124)+attribute(0x123))
                s.sendto(request,target); decoded(s.recvfrom(512)[0],request,s.getsockname(),error=True)
                good=packet(); bad_crc=bytearray(good); bad_crc[-1]^=1
                bad_cookie=bytearray(good); bad_cookie[4]^=1
                for bad in (b'',b'?',good[:-1],good+b'\0'*4,bad_crc,bad_cookie,packet(kind=0x11),
                            packet(kind=0x101),packet(attrs=attribute(8,b'\0'*20)),
                            packet(attrs=attribute(0x9000,b'x'*1024))):
                    s.sendto(bad,target)
                    try: unexpected=s.recvfrom(1024); raise AssertionError(('unexpected response',bad,unexpected))
                    except socket.timeout: pass
                s.sendto(good,target); decoded(s.recvfrom(512)[0],good,s.getsockname())
                # Binding is exclusive; a failed replacement cannot disturb it.
                if family==4:
                    conflict=subprocess.run([str(exe),'--bind4','127.0.0.1','--no-v6','--port',str(port),'--run-ms','50'],
                        capture_output=True,text=True,timeout=8)
                    assert conflict.returncode==1 and 'READY' not in conflict.stdout,conflict.stdout+conflict.stderr
                    s.sendto(good,target); decoded(s.recvfrom(512)[0],good,s.getsockname())
            finally: s.close()
        state=server.finish(); assert state['replied']==11 and state['invalid']>=18,state
    finally: server.stop()
    print('PASS STUN independent v4/v6 XOR/fingerprint/error420, malformed/truncated/indication/integrity silence, exclusive bind and joined close')


def limits(exe, global_limit):
    args=['--rate','2','--per-source','2'] if global_limit else ['--rate','64','--per-source','2','--no-v6']
    server=Server(exe,*args,run_ms=2200); sockets=[]
    try:
        for i in range(8):
            family=6 if global_limit and i%2 else 4
            s,target=client(family,server.ports[family]); sockets.append(s); s.sendto(packet(),target)
        received=0; end=time.monotonic()+.12
        while time.monotonic()<end:
            ready,_,_=select.select(sockets,[],[],max(0,end-time.monotonic()))
            for s in ready: s.recvfrom(512); received+=1
        assert received==2,received
        time.sleep(.55)
        s,target=client(4,server.ports[4]); sockets.append(s); request=packet(); s.sendto(request,target)
        decoded(s.recvfrom(512)[0],request,s.getsockname())
        state=server.finish(); assert state['replied']==3 and state['rate_dropped']>=6,state
    finally:
        for s in sockets: s.close()
        server.stop()
    print('PASS STUN '+('shared v4/v6 global' if global_limit else 'per-source across changing UDP ports')+' token quota, bounded refill and clean drain')


def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--exe',required=True,type=Path); args=parser.parse_args()
    exe=args.exe.resolve(); assert subprocess.run([str(exe),'--help'],capture_output=True).returncode==0
    version=subprocess.run([str(exe),'--version'],capture_output=True,text=True,timeout=5)
    assert version.returncode==0 and re.fullmatch(r'xrudp-stun xs=[0-9a-f]{40} xrudp=[0-9a-f]{40} core=[0-9a-f]{40}\s*',version.stdout),version.stdout
    for flags in (['--port','-1'],['--port','65536'],['--port','1x'],['--rate','0'],['--no-v4','--no-v6'],
                  ['--rate','1','--per-source','2'],['--no-v4','--no-v4'],['--bind4','attacker.example']):
        result=subprocess.run([str(exe),*flags],capture_output=True,timeout=5); assert result.returncode==2,flags
    protocol(exe); limits(exe,False); limits(exe,True)
    if os.name!='nt':
        server=Server(exe,run_ms=0)
        try:
            server.process.terminate(); server.finish()
        finally: server.stop()
        print('PASS STUN SIGTERM cancels UDP and joins the Engine')
    return 0


if __name__=='__main__': raise SystemExit(main())
