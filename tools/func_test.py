"""xs3 功能测试：配置错误矩阵 + 协议行为矩阵 + 重载语义 + idle 超时。

用法：python tools/func_test.py  （在仓库根执行；自动启动独立实例，独立临时配置）
输出：逐项 PASS/FAIL，任一失败非零退出。
"""
import base64
import argparse
import http.client
import json
import os
import shutil
import socket
import ssl
import struct
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RELEASE = ROOT / 'release'
EXE = RELEASE / 'xs.exe'
failures = []


def fail(name, detail=''):
    failures.append(f'{name}: {detail}')


# ============================================================
# A. 配置错误矩阵（fail-fast：exit 1 + 指名错误）
# ============================================================

def config_matrix():
    cases = [
        ('bad-json', '{ not json', 'parse'),
        ('bad-class', '{"services":[{"class":"ftp","name":"x","port":1}]}', 'class'),
        ('dup-name', '{"services":[{"class":"tcp","name":"x","port":1},{"class":"tcp","name":"x","port":2}]}', 'duplicate'),
        ('missing-port', '{"services":[{"class":"tcp","name":"x"}]}', 'port'),
        ('tls-on-udp', '{"services":[{"class":"udp","name":"x","port":1,"tls":true}]}', 'tls'),
        ('udp-recv-range', '{"services":[{"class":"udp","name":"x","port":1,'
                           '"recv_limit":65536}]}', 'UDP datagram limit'),
        ('type-error', '{"services":[{"class":"tcp","name":"x","port":"80"}]}', 'expect int'),
        ('engine-type', '{"engine":1,"services":[]}', "field 'engine' expect object"),
        ('engine-workers-range', '{"engine":{"workers":-1},"services":[]}', 'engine.workers'),
        ('custom-int-type', '{"services":[{"class":"http","name":"x","port":1,'
                            '"header_limit":"large"}]}', "header_limit' expect int"),
        ('custom-int-negative', '{"services":[{"class":"http","name":"x","port":1,'
                                '"idle_timeout":-1}]}', "idle_timeout' expect non-negative"),
        ('static-type', '{"services":[{"class":"http","name":"x","port":1,'
                        '"host_default":{"static":[]}}]}', "field 'static' expect object"),
        ('static-header-invalid', '{"services":[{"class":"http","name":"x","port":1,'
                                   '"host_default":{"static":{"headers":{"Bad Name":"x"}}}}]}',
          'invalid HTTP field'),
        ('static-header-framing', '{"services":[{"class":"http","name":"x","port":1,'
                                   '"host_default":{"static":{"headers":'
                                   '{"Transfer-Encoding":"chunked"}}}}]}',
         'reserved response field'),
        ('static-error-page-key', '{"services":[{"class":"http","name":"x","port":1,'
                                  '"host_default":{"static":{"error_pages":'
                                  '{"oops":"error.html"}}}}]}',
         'expects 400..599 keys'),
        ('static-root-missing', json.dumps({'services': [{
            'class': 'http', 'name': 'x', 'port': 1,
            'host_default': {'path': str(Path(tempfile.gettempdir()) /
                                         'xs-root-must-not-exist-7b911c')}
        }]}), 'cannot open static root'),
        ('ws-protocol-type', '{"services":[{"class":"ws","name":"x","port":1,'
                             '"devlang":"c","devfile":"script/ws_main.c",'
                             '"ws_protocol":1}]}', "ws_protocol' expect string"),
        ('dup-host-name', '{"services":[{"class":"http","name":"x","port":1,"hosts":['
                          '{"name":"a","host":"a.example"},{"name":"a","host":"b.example"}]}]}',
         'duplicate host name'),
        ('missing-vhost-alias', '{"services":[{"class":"http","name":"x","port":1,'
                                '"hosts":[{"name":"a"}]}]}', 'requires non-empty'),
        ('dup-vhost-alias', '{"services":[{"class":"http","name":"x","port":1,"hosts":['
                            '{"name":"a","host":"same.example"},'
                            '{"name":"b","host":"SAME.EXAMPLE"}]}]}', 'duplicate virtual host alias'),
    ]
    for name, text, expect in cases:
        with tempfile.NamedTemporaryFile('w', suffix='.json', delete=False, encoding='utf-8') as fp:
            fp.write(text)
            path = fp.name
        try:
            proc = subprocess.run(
                [str(EXE), path],
                capture_output=True, text=True, timeout=10, cwd=str(RELEASE))
            out = proc.stdout + proc.stderr
            if proc.returncode != 1:
                fail(f'config/{name}', f'exit={proc.returncode}')
            elif expect not in out:
                fail(f'config/{name}', f'message missing "{expect}": {out.strip()[:120]}')
        except subprocess.TimeoutExpired:
            fail(f'config/{name}', 'timeout (should fail-fast)')
        finally:
            os.unlink(path)

    # dev_inc 相对路径只能基于可执行文件 appPath 解析，不得意外命中
    # 进程当前工作目录中的同名诱饵头文件。
    with tempfile.TemporaryDirectory(prefix='xs-dev-path-cwd-') as temp_dir:
        temp_root = Path(temp_dir)
        decoy = temp_root / 'decoy'
        decoy.mkdir()
        (decoy / 'cwd_only.h').write_text('#define CWD_ONLY 1\n', encoding='utf-8')
        script = temp_root / 'main.c'
        script.write_text(
            '#include <xsbase.h>\n#include <cwd_only.h>\n'
            'void ServiceInit(XS_HostInfo* p){(void)p;}\n', encoding='utf-8')
        config = temp_root / 'xs.json'
        config.write_text(json.dumps({'services': [{
            'class': 'custom', 'name': 'cwd-probe', 'enabled': True,
            'devlang': 'c', 'devfile': str(script), 'dev_inc': 'decoy'
        }]}), encoding='utf-8')
        try:
            proc = subprocess.run(
                [str(EXE), str(config)], capture_output=True, text=True,
                timeout=10, cwd=str(temp_root))
            if proc.returncode != 1 or 'script compile failed' not in proc.stdout + proc.stderr:
                fail('config/dev-path-no-cwd-fallback',
                     f'exit={proc.returncode} output={(proc.stdout + proc.stderr)[-200:]}')
        except subprocess.TimeoutExpired:
            fail('config/dev-path-no-cwd-fallback', 'timeout')


def listener_failure_matrix():
    """第二个 TLS endpoint 失败时，第一个 plain listener 必须完整回滚。"""
    used = set()
    plain_port = free_port(socket.SOCK_STREAM, used)
    tls_blocker = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if hasattr(socket, 'SO_EXCLUSIVEADDRUSE'):
        tls_blocker.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
    tls_blocker.bind(('127.0.0.1', 0))
    tls_blocker.listen(1)
    tls_port = int(tls_blocker.getsockname()[1])
    config = {
        'services': [{
            'class': 'http', 'name': 'partial-listener',
            'ip': '127.0.0.1', 'port': plain_port,
            'tls': True, 'ip_tls': '127.0.0.1', 'port_tls': tls_port,
            'host_default': {
                'name': 'default',
                'tls_cert': str(RELEASE / 'tls' / 'xtps_cert.pem'),
                'tls_key': str(RELEASE / 'tls' / 'xtps_key.pem'),
            },
        }],
    }
    with tempfile.NamedTemporaryFile('w', suffix='.json', delete=False,
                                     encoding='utf-8') as fp:
        json.dump(config, fp)
        path = fp.name
    try:
        proc = subprocess.run(
            [str(EXE), path], capture_output=True, text=True,
            timeout=20, cwd=str(RELEASE))
        if proc.returncode != 1 or 'listen failed' not in proc.stdout + proc.stderr:
            fail('listener/partial-tls-failure',
                 f'exit={proc.returncode} output={(proc.stdout + proc.stderr)[-200:]}')
        probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            probe.bind(('127.0.0.1', plain_port))
        except OSError as exc:
            fail('listener/plain-rollback-release', str(exc))
        finally:
            probe.close()
    except subprocess.TimeoutExpired:
        fail('listener/partial-tls-failure', 'process did not fail-fast')
    finally:
        tls_blocker.close()
        os.unlink(path)


def tls_identity_diagnostic_matrix():
    """证书算法应在构造身份前决定，类型冲突不得被后续构造器错误覆盖。"""
    cert_path = RELEASE / 'tls' / 'xtps_cert.pem'
    source_key = RELEASE / 'tls' / 'xtps_key.pem'
    if not cert_path.is_file() or not source_key.is_file():
        fail('tls/type-mismatch-diagnostic', 'missing TLS fixture')
        return
    key_text = source_key.read_text(encoding='ascii')
    if '-----BEGIN PRIVATE KEY-----' not in key_text:
        fail('tls/type-mismatch-diagnostic', 'fixture is not PKCS#8')
        return
    # 保留 DER，仅把传统 PEM 类型声明改为 EC；xs 应在进入 RSA 构造器前明确拒绝。
    key_text = key_text.replace('-----BEGIN PRIVATE KEY-----',
                                '-----BEGIN EC PRIVATE KEY-----')
    key_text = key_text.replace('-----END PRIVATE KEY-----',
                                '-----END EC PRIVATE KEY-----')
    used = set()
    plain_port = free_port(socket.SOCK_STREAM, used)
    tls_port = free_port(socket.SOCK_STREAM, used)
    with tempfile.TemporaryDirectory(prefix='xs-tls-diag-') as temp_dir:
        temp_path = Path(temp_dir)
        key_path = temp_path / 'wrong-type-key.pem'
        config_path = temp_path / 'xs.json'
        key_path.write_text(key_text, encoding='ascii')
        config_path.write_text(json.dumps({
            'services': [{
                'class': 'http', 'name': 'tls-diagnostic',
                'ip': '127.0.0.1', 'port': plain_port,
                'tls': True, 'ip_tls': '127.0.0.1', 'port_tls': tls_port,
                'host_default': {
                    'name': 'default',
                    'tls_cert': str(cert_path),
                    'tls_key': str(key_path),
                },
            }],
        }), encoding='utf-8')
        try:
            proc = subprocess.run(
                [str(EXE), str(config_path)], capture_output=True, text=True,
                timeout=20, cwd=str(RELEASE))
            output = proc.stdout + proc.stderr
            expected = 'certificate uses RSA but private key uses EC'
            if proc.returncode != 1 or expected not in output:
                fail('tls/type-mismatch-diagnostic',
                     f'exit={proc.returncode} output={output[-300:]}')
        except subprocess.TimeoutExpired:
            fail('tls/type-mismatch-diagnostic', 'process did not fail-fast')


def tls_static_large_matrix():
    """TLS 静态文件必须跨多轮背压完整发送，并保留 keep-alive 顺序。"""
    used = set()
    plain_port = free_port(socket.SOCK_STREAM, used)
    tls_port = free_port(socket.SOCK_STREAM, used)
    cert_path = RELEASE / 'tls' / 'xtps_cert.pem'
    key_path = RELEASE / 'tls' / 'xtps_key.pem'

    if not cert_path.exists() or not key_path.exists():
        fail('tls/static-large', 'missing TLS fixture')
        return
    with tempfile.TemporaryDirectory(prefix='xs-tls-static-') as temp_dir:
        temp_root = Path(temp_dir)
        expected = b't' * (4 * 1024 * 1024)
        (temp_root / 'large.bin').write_bytes(expected)
        (temp_root / 'next.txt').write_bytes(b'next-response')
        config_path = temp_root / 'xs.json'
        config_path.write_text(json.dumps({'services': [{
            'class': 'http', 'name': 'tls-static',
            'ip': '127.0.0.1', 'port': plain_port,
            'tls': True, 'ip_tls': '127.0.0.1', 'port_tls': tls_port,
            'host_default': {
                'path': str(temp_root),
                'tls_cert': str(cert_path), 'tls_key': str(key_path)
            }
        }]}), encoding='utf-8')
        log = open(temp_root / 'xs.log', 'w', encoding='utf-8')
        proc = subprocess.Popen(
            [str(EXE), str(config_path)], cwd=str(RELEASE),
            stdout=log, stderr=subprocess.STDOUT)
        try:
            if not wait_port(tls_port):
                fail('tls/static-large', 'TLS port not up')
                return
            context = ssl._create_unverified_context()
            conn = http.client.HTTPSConnection(
                '127.0.0.1', tls_port, timeout=20, context=context)
            conn.request('GET', '/large.bin')
            response = conn.getresponse()
            body = response.read()
            conn.request('GET', '/next.txt')
            next_response = conn.getresponse()
            next_body = next_response.read()
            conn.close()
            if response.status != 200 or body != expected:
                fail('tls/static-large-complete',
                     f'status={response.status} bytes={len(body)}')
            if next_response.status != 200 or next_body != b'next-response':
                fail('tls/static-large-keepalive',
                     f'status={next_response.status} body={next_body!r}')
        except (OSError, ssl.SSLError, http.client.HTTPException) as exc:
            fail('tls/static-large', str(exc))
        finally:
            if proc.poll() is None:
                proc.kill()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                pass
            log.close()


# ============================================================
# B. 行为矩阵（独立实例，短 idle 配置）
# ============================================================

def wait_port(port, timeout=10.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            socket.create_connection(('127.0.0.1', port), timeout=0.5).close()
            return True
        except OSError:
            time.sleep(0.2)
    return False


def free_port(sock_type=socket.SOCK_STREAM, used=None):
    used = used if used is not None else set()
    while True:
        with socket.socket(socket.AF_INET, sock_type) as sock:
            sock.bind(('127.0.0.1', 0))
            port = int(sock.getsockname()[1])
        if port not in used:
            used.add(port)
            return port


def http_get(path, host_header=None, port=8080, method='GET', body=None):
    c = http.client.HTTPConnection('127.0.0.1', port, timeout=5)
    headers = {}
    if host_header:
        headers['Host'] = host_header
    c.request(method, path, body=body, headers=headers)
    r = c.getresponse()
    d = r.read()
    c.close()
    return r.status, d


def raw_http(port, request):
    with socket.create_connection(('127.0.0.1', port), timeout=5) as sock:
        sock.settimeout(5)
        sock.sendall(request)
        data = b''
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
    status = int(data.split(b'\r\n', 1)[0].split()[1])
    body = data.split(b'\r\n\r\n', 1)[1] if b'\r\n\r\n' in data else b''
    return status, body


def raw_http_parts(port, parts):
    with socket.create_connection(('127.0.0.1', port), timeout=5) as sock:
        sock.settimeout(5)
        for part in parts:
            sock.sendall(part)
            time.sleep(0.01)
        data = b''
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
    status = int(data.split(b'\r\n', 1)[0].split()[1])
    body = data.split(b'\r\n\r\n', 1)[1] if b'\r\n\r\n' in data else b''
    return status, body


def behavior_matrix():
    cfg = json.load(open(RELEASE / 'xs.json', encoding='utf-8'))
    if os.name != 'nt':
        for service in cfg.get('services', []):
            if service.get('name') in {'crt_probe', 'include_probe'}:
                service['enabled'] = False
    fixture = tempfile.TemporaryDirectory(prefix='xs-func-fixture-')
    fixture_root = Path(fixture.name)
    fixture_main = fixture_root / 'http_main.c'
    fixture_admin = fixture_root / 'admin_main.c'
    fixture_wwwroot = fixture_root / 'wwwroot'
    outside_secret = fixture_root / 'outside-secret.txt'
    shutil.copy2(RELEASE / 'script' / 'http_main.c', fixture_main)
    shutil.copy2(RELEASE / 'hosts' / 'admin' / 'main.c', fixture_admin)
    shutil.copytree(RELEASE / 'wwwroot', fixture_wwwroot)
    (fixture_wwwroot / 'large.bin').write_bytes(b'x' * (4 * 1024 * 1024))
    outside_secret.write_text('must-not-escape-static-root', encoding='utf-8')
    symlink_ready = False
    try:
        os.symlink(outside_secret, fixture_wwwroot / 'outside-link')
        symlink_ready = True
    except OSError:
        pass
    # 仅测试夹具增加 nested TCC 入口；不改 tracked 示例脚本。它会与 reload
    # 编译并发执行，覆盖动态 VFS mount/unmount 与全局虚拟 FD 表。
    fixture_text = fixture_main.read_text(encoding='utf-8')
    fixture_text = fixture_text.replace(
        '#include <string.h>\n', '#include <string.h>\n#include <libtcc.h>\n')
    fixture_text = fixture_text.replace(
        'XS_RequestResult RequestProc(XS_HttpReq* pReq)\n{',
        '''typedef struct TakeoverEchoContext {
\txnetstream* tcp;
\txtlsstream* tls;
} TakeoverEchoContext;

static int32 TakeoverEchoThread(void* pData)
{
\tTakeoverEchoContext* pCtx = (TakeoverEchoContext*)pData;
\txfuture* pRecv;
\txfutureresult tResult;

\t/* RequestProc 返回后驱动才安装 pull 事件表；稍后从独立线程提交读取。 */
\txrtSleep(20);
\tpRecv = pCtx->tls != NULL ? xrtTlsStreamRecvAsync(pCtx->tls, 65536) :
\t\txrtNetStreamRecvAsync(pCtx->tcp, 65536);
\tif ( pRecv != NULL && xrtFutureWaitFor(pRecv, 5000000) == XWAIT_OK &&
\t     xrtFutureResult(pRecv, &tResult) && tResult.State == XFUTURE_RESOLVED ) {
\t\txbytesview tData = xrtNetBytesView((xnetbytes*)tResult.Value);

\t\tif ( pCtx->tls != NULL ) {
\t\t\txfuture* pSend = xrtTlsStreamSendAsync(pCtx->tls, tData.Data, tData.Size);
\t\t\tif ( pSend != NULL ) xrtFutureDestroy(pSend);
\t\t\t(void)xrtTlsStreamClose(pCtx->tls);
\t\t} else {
\t\t\t(void)xrtNetStreamSend(pCtx->tcp, tData.Data, tData.Size);
\t\t\t(void)xrtNetStreamClose(pCtx->tcp);
\t\t}
\t} else if ( pCtx->tls != NULL ) {
\t\t(void)xrtTlsStreamClose(pCtx->tls);
\t} else {
\t\t(void)xrtNetStreamClose(pCtx->tcp);
\t}
\txrtFutureDestroy(pRecv);
\tif ( pCtx->tls != NULL ) xrtTlsStreamDestroy(pCtx->tls);
\tif ( pCtx->tcp != NULL ) xrtNetStreamDestroy(pCtx->tcp);
\txrtFree(pCtx);
\treturn 0;
}

static bool TakeoverEchoStart(XS_HttpReq* pReq)
{
\tTakeoverEchoContext* pCtx = (TakeoverEchoContext*)xrtCalloc(1, sizeof(*pCtx));
\txthread* pThread;

\tif ( pCtx == NULL ) return false;
\tpCtx->tls = pReq->tls != NULL ? xrtTlsStreamRef(pReq->tls) : NULL;
\tpCtx->tcp = pReq->tcp != NULL ? xrtNetStreamRef(pReq->tcp) : NULL;
\tif ( pCtx->tls == NULL && pCtx->tcp == NULL ) {
\t\txrtFree(pCtx);
\t\treturn false;
\t}
\tpThread = xrtThreadCreate(TakeoverEchoThread, pCtx, 0);
\tif ( pThread == NULL ) {
\t\tif ( pCtx->tls != NULL ) xrtTlsStreamDestroy(pCtx->tls);
\t\tif ( pCtx->tcp != NULL ) xrtNetStreamDestroy(pCtx->tcp);
\t\txrtFree(pCtx);
\t\treturn false;
\t}
\txrtThreadDestroy(pThread); /* 运行线程自持引用，安全分离 */
\treturn true;
}

static bool NestedCompile(void)
{
\tTCCState* pTcc = xsCreateTCC();
\tint iOk = pTcc != NULL ? tcc_compile_string(pTcc,
\t\t"#include <stddef.h>\\nint xs_nested(void){return (int)sizeof(size_t);}") : -1;

\tif ( iOk >= 0 ) iOk = tcc_relocate(pTcc);
\txsDestroyTCC(pTcc);
\treturn iOk >= 0;
}

XS_RequestResult RequestProc(XS_HttpReq* pReq)
{''')
    fixture_text = fixture_text.replace(
        'if ( PathIs(pReq, "/text") ) {',
        'if ( PathIs(pReq, "/takeover-echo") ) {\n'
        '\t\t\tif ( !TakeoverEchoStart(pReq) ) {\n'
        '\t\t\t\tif ( pReq->tls != NULL ) (void)xrtTlsStreamClose(pReq->tls);\n'
        '\t\t\t\telse (void)xrtNetStreamClose(pReq->tcp);\n'
        '\t\t\t}\n'
        '\t\t\treturn XS_TAKEOVER;\n\t\t}\n\t\t'
        'if ( PathIs(pReq, "/nested") ) {\n'
        '\t\t\treturn ReplyLit(pReq, NestedCompile() ? 200 : 500, '
        '"text/plain", "nested") ? XS_OK : XS_OK;\n\t\t}\n\t\t'
        'if ( PathIs(pReq, "/text") ) {')
    fixture_main.write_text(fixture_text, encoding='utf-8')
    ports = {}
    used_ports = set()
    for s in cfg['services']:
        if s.get('class') != 'custom':
            sock_type = socket.SOCK_DGRAM if s.get('class') == 'udp' else socket.SOCK_STREAM
            s['port'] = free_port(sock_type, used_ports)
            ports[s.get('name')] = s['port']
        if s.get('name') == 'tcp-echo':
            s['idle_timeout'] = 3000
        elif s.get('name') == 'main':
            # 让 HTTP Header/body 线路硬边界可在小数据量下回归。
            s['recv_limit'] = 32768
            s['header_limit'] = 32768
            s['body_limit'] = 16384
            s['host_default']['devfile'] = str(fixture_main)
            s['host_default']['path'] = str(fixture_wwwroot)
            for host in s.get('hosts', []):
                if host.get('name') == 'admin':
                    host['devfile'] = str(fixture_admin)
        elif s.get('name') == 'ws-echo':
            # MaxHead 大于 ReadLimit，专门验证缓冲填满时不会永久 MORE。
            s['recv_limit'] = 4096
    http_port = ports['main']
    tcp_port = ports['tcp-echo']
    ws_port = ports['ws-echo']

    def local_http(path, host_header=None, method='GET', body=None):
        return http_get(path, host_header=host_header, port=http_port, method=method, body=body)

    def submit_reload(path='/reload', host_header=None):
        status, body = local_http(path, host_header=host_header)
        if status != 202:
            raise AssertionError(f'{path} status={status} body={body[:120]!r}')
        return int(json.loads(body)['reload_id'])

    def wait_reload(reload_id, timeout=8.0):
        deadline = time.time() + timeout
        connection = http.client.HTTPConnection('127.0.0.1', http_port, timeout=5)
        try:
            while time.time() < deadline:
                connection.request('GET', f'/reload-status/{reload_id}')
                response = connection.getresponse()
                body = response.read()
                if response.status == 200:
                    result = json.loads(body)
                    if result['status'] in {'succeeded', 'failed', 'superseded', 'cancelled'}:
                        return result
                time.sleep(0.02)
        finally:
            connection.close()
        raise AssertionError(f'reload {reload_id} did not reach terminal state')

    cfg_path = RELEASE / 'func_test_config.json'
    json.dump(cfg, open(cfg_path, 'w', encoding='utf-8'), ensure_ascii=False, indent=2)

    log = open(tempfile.gettempdir() + '/xs_func.log', 'w')
    proc = subprocess.Popen(
        [str(EXE), 'func_test_config.json'],
        cwd=str(RELEASE), stdout=log, stderr=subprocess.STDOUT)
    try:
        if not wait_port(http_port):
            fail('behavior/startup', 'http port not up')
            return
        time.sleep(1.5)

        # B1 虚拟主机路由：Host 同时选择静态根和脚本。
        s1, admin_static = local_http('/', host_header='admin.example.com')
        s_script, admin_script = local_http('/vhost', host_header='admin.example.com')
        s_absolute, absolute_static = local_http(
            'http://admin.example.com/', host_header='default.example.com')
        s2, default_static = local_http('/')
        if not (s1 == 200 and b'admin static ok' in admin_static and
                s_script == 200 and b'admin script ok' in admin_script and
                s_absolute == 200 and b'admin static ok' in absolute_static and
                s2 == 200 and b'xs3 static ok' in default_static):
            fail('behavior/vhost',
                 f'admin-static={s1}/{admin_static[:40]!r} '
                 f'admin-script={s_script}/{admin_script[:40]!r} '
                 f'absolute={s_absolute}/{absolute_static[:40]!r} '
                 f'default={s2}/{default_static[:40]!r}')

        # 同一 keep-alive TCP 连接的两个请求可分别路由到不同 host。
        try:
            vhost_conn = http.client.HTTPConnection('127.0.0.1', http_port, timeout=5)
            vhost_conn.request('GET', '/vhost', headers={'Host': 'admin.example.com'})
            first = vhost_conn.getresponse()
            first_body = first.read()
            vhost_conn.request('GET', '/text', headers={'Host': 'default.example.com'})
            second = vhost_conn.getresponse()
            second_body = second.read()
            vhost_conn.close()
            if first.status != 200 or b'admin script ok' not in first_body or \
                    second.status != 200 or b'xs3 http ok' not in second_body:
                fail('behavior/vhost-keepalive',
                     f'first={first.status}/{first_body[:40]!r} '
                     f'second={second.status}/{second_body[:40]!r}')
        except (OSError, http.client.HTTPException) as exc:
            fail('behavior/vhost-keepalive', str(exc))

        # TAKEOVER 安装终态事件后必须处于 pull 模式；客户端后续发送的字节
        # 由应用 RecvAsync 读取并回显，不得被框架空 Read 回调吞掉。
        try:
            takeover = socket.create_connection(('127.0.0.1', http_port), timeout=3)
            takeover.settimeout(6)
            takeover.sendall(
                b'GET /takeover-echo HTTP/1.1\r\nHost: default.example.com\r\n\r\n')
            time.sleep(0.08)
            payload = b'takeover-pull-echo'
            takeover.sendall(payload)
            echoed = b''
            while len(echoed) < len(payload):
                chunk = takeover.recv(1024)
                if not chunk:
                    break
                echoed += chunk
            takeover.close()
            if echoed != payload:
                fail('behavior/takeover-pull-read', f'echoed={echoed!r}')
        except OSError as exc:
            fail('behavior/takeover-pull-read', str(exc))

        try:
            port_status, port_body = raw_http(http_port,
                f'GET /vhost HTTP/1.1\r\nHost: ADMIN.EXAMPLE.COM:{http_port}\r\n'
                'Connection: close\r\n\r\n'.encode())
            duplicate_status, _ = raw_http(http_port,
                b'GET / HTTP/1.1\r\nHost: a\r\nHost: b\r\nConnection: close\r\n\r\n')
            missing_status, _ = raw_http(http_port,
                b'GET / HTTP/1.1\r\nConnection: close\r\n\r\n')
            legacy_status, legacy_body = raw_http(http_port,
                b'GET / HTTP/1.0\r\nConnection: close\r\n\r\n')
            extension_status, _ = raw_http(http_port,
                b'PURGE / HTTP/1.1\r\nHost: default.example.com\r\nConnection: close\r\n\r\n')
            lowercase_status, _ = raw_http(http_port,
                b'get / HTTP/1.1\r\nHost: default.example.com\r\nConnection: close\r\n\r\n')
            invalid_method_status, _ = raw_http(http_port,
                b'GE(T / HTTP/1.1\r\nHost: default.example.com\r\nConnection: close\r\n\r\n')
            if port_status != 200 or b'admin script ok' not in port_body or \
                    duplicate_status != 400 or missing_status != 400 or \
                    extension_status != 405 or lowercase_status != 405 or \
                    invalid_method_status != 400 or \
                    legacy_status != 200 or b'xs3 static ok' not in legacy_body:
                fail('behavior/vhost-host-rules',
                     f'port={port_status}/{port_body[:30]!r} duplicate={duplicate_status} '
                     f'missing={missing_status} extension={extension_status} '
                     f'lowercase={lowercase_status} invalid_method={invalid_method_status} '
                     f'legacy={legacy_status}/{legacy_body[:30]!r}')
        except (OSError, ValueError, IndexError) as exc:
            fail('behavior/vhost-host-rules', str(exc))

        # B2 body 上限：body_limit=16384 → 超限请求明确 413/断连。
        try:
            s3, _ = local_http('/echo', method='POST', body=b'x' * 300000)
            if s3 != 413:
                fail('behavior/body-limit', f'status={s3} expect 413')
        except (http.client.HTTPException, ConnectionError):
            pass  # 服务端直接断连也符合分帧错误处理

        # 增量 chunked 三态：跨 Read 分段仍完整交给脚本；畸形和累计超限
        # 分别进入 400/413，不得误当 READY 调业务回调。
        chunk_head = (
            b'POST /echo HTTP/1.1\r\nHost: default.example.com\r\n'
            b'Transfer-Encoding: chunked\r\nConnection: close\r\n\r\n')
        try:
            chunk_status, chunk_body = raw_http_parts(
                http_port, (chunk_head, b'4\r\nte', b'st\r\n0\r', b'\n\r\n'))
            malformed_status, _ = raw_http_parts(
                http_port, (chunk_head, b'Z\r\nbad\r\n0\r\n\r\n'))
            over_status, _ = raw_http_parts(
                http_port, (chunk_head, b'4e20\r\n'))
            if chunk_status != 200 or b'test' not in chunk_body:
                fail('behavior/chunked-fragmented',
                     f'status={chunk_status} body={chunk_body[:50]!r}')
            if malformed_status != 400:
                fail('behavior/chunked-malformed', f'status={malformed_status}')
            if over_status != 413:
                fail('behavior/chunked-over-limit', f'status={over_status}')
        except (OSError, ValueError, IndexError) as exc:
            fail('behavior/chunked-state', str(exc))

        # ReadLimit 恰好填满且 Header 仍不完整时，HTTP/WS 都必须终止而非卡死。
        for protocol, port, limit in (('http', http_port, 32768), ('ws', ws_port, 4096)):
            try:
                request = bytearray(b'GET / HTTP/1.1\r\nHost: x\r\n')
                while len(request) + 3012 < limit:
                    request += b'X-Fill: ' + b'a' * 3000 + b'\r\n'
                tail = b'X-Tail: '
                request += tail + b'b' * (limit - len(request) - len(tail))
                assert len(request) == limit
                with socket.create_connection(('127.0.0.1', port), timeout=3) as boundary:
                    boundary.settimeout(3)
                    boundary.sendall(request)
                    response = boundary.recv(1024)
                if b' 431 ' not in response.split(b'\r\n', 1)[0]:
                    fail(f'behavior/{protocol}-receive-boundary', response[:80])
            except OSError as exc:
                fail(f'behavior/{protocol}-receive-boundary', str(exc))

        # 解析器主动判定字段数量超限时也必须返回 431，而不是落入通用 400。
        too_many_fields = (
            b'GET / HTTP/1.1\r\nHost: default.example.com\r\n' +
            b''.join(f'X-{index}: v\r\n'.encode() for index in range(64)) +
            b'Connection: close\r\n\r\n'
        )
        for protocol, port in (('http', http_port), ('ws', ws_port)):
            try:
                field_status, _ = raw_http(port, too_many_fields)
                if field_status != 431:
                    fail(f'behavior/{protocol}-field-limit',
                         f'status={field_status} expect 431')
            except (OSError, ValueError, IndexError) as exc:
                fail(f'behavior/{protocol}-field-limit', str(exc))

        # B3 重载语义：换代 / 回滚 / swap
        script = fixture_main
        src = script.read_text(encoding='utf-8')
        mod = src.replace('"xs3 http ok"', '"xs3 func RELOADED"')
        bad = src.replace('RequestProc(XS_HttpReq', 'RequestProc(XS_HttpReq BROKEN')
        try:
            script.write_text(mod, encoding='utf-8')
            try:
                first_id = submit_reload()
                first_result = wait_reload(first_id)
                if first_result['status'] != 'succeeded' or not first_result['revision']:
                    fail('behavior/reload-result', f'result={first_result}')
            except (AssertionError, KeyError, ValueError, json.JSONDecodeError) as exc:
                fail('behavior/reload-accepted', str(exc))
            _, body = local_http('/text')
            if b'RELOADED' not in body:
                fail('behavior/reload-swap', f'body={body[:40]!r}')

            # 同目标风暴只保留一个 trailing desired intent；中间 ticket 必须明确 superseded。
            try:
                burst_ids = []
                burst_conn = http.client.HTTPConnection('127.0.0.1', http_port, timeout=5)
                try:
                    for _ in range(20):
                        burst_conn.request('GET', '/reload')
                        response = burst_conn.getresponse()
                        payload = response.read()
                        if response.status != 202:
                            raise AssertionError(f'/reload status={response.status}')
                        burst_ids.append(int(json.loads(payload)['reload_id']))
                finally:
                    burst_conn.close()
                burst_results = [wait_reload(reload_id) for reload_id in burst_ids]
                states = [result['status'] for result in burst_results]
                latest = burst_results[burst_ids.index(max(burst_ids))]
                if latest['status'] != 'succeeded' or 'superseded' not in states:
                    fail('behavior/reload-latest-wins', f'states={states}')
            except (AssertionError, KeyError, ValueError, json.JSONDecodeError) as exc:
                fail('behavior/reload-latest-wins', str(exc))

            script.write_text(bad, encoding='utf-8')
            try:
                failed_results = [wait_reload(submit_reload()) for _ in range(6)]
                if any(result['status'] != 'failed' for result in failed_results):
                    fail('behavior/reload-failed-status', f'results={failed_results}')
            except (AssertionError, KeyError, ValueError, json.JSONDecodeError) as exc:
                fail('behavior/reload-queue-status', str(exc))
            _, body = local_http('/text')
            if b'RELOADED' not in body:
                fail('behavior/reload-rollback', 'old gen lost')
        finally:
            script.write_text(src, encoding='utf-8')
            try:
                wait_reload(submit_reload())
            except (AssertionError, KeyError, ValueError, json.JSONDecodeError):
                pass

        # B3b 次级 host 入口同样发布完整 server generation。
        admin_script_path = fixture_admin
        admin_src = admin_script_path.read_text(encoding='utf-8')
        admin_mod = admin_src.replace('xs3 admin script ok', 'xs3 admin RELOADED')
        try:
            admin_script_path.write_text(admin_mod, encoding='utf-8')
            try:
                admin_result = wait_reload(submit_reload(
                    '/reload', host_header='admin.example.com'))
                if admin_result['status'] != 'succeeded' or not admin_result['revision']:
                    fail('behavior/vhost-reload-result', f'result={admin_result}')
            except (AssertionError, KeyError, ValueError, json.JSONDecodeError) as exc:
                fail('behavior/vhost-reload-submit', str(exc))
            status, body = local_http('/vhost', host_header='admin.example.com')
            if status != 200 or b'admin RELOADED' not in body:
                fail('behavior/vhost-reload-active', f'{status}/{body[:50]!r}')
        finally:
            admin_script_path.write_text(admin_src, encoding='utf-8')
            try:
                wait_reload(submit_reload('/reload', host_header='admin.example.com'))
            except (AssertionError, KeyError, ValueError, json.JSONDecodeError):
                pass

        # B3c nested TCC 与 reload 并发：各 worker 复用自己的 keep-alive，
        # 同时 controller 反复 mount/unmount 新代源码。
        nested_errors = []
        nested_start = threading.Event()

        def nested_worker():
            connection = http.client.HTTPConnection('127.0.0.1', http_port, timeout=8)
            try:
                nested_start.wait(3)
                for _ in range(12):
                    connection.request('GET', '/nested')
                    response = connection.getresponse()
                    body = response.read()
                    if response.status != 200 or body != b'nested':
                        raise AssertionError(f'{response.status}/{body[:40]!r}')
            except (OSError, http.client.HTTPException, AssertionError) as exc:
                nested_errors.append(str(exc))
            finally:
                connection.close()

        nested_threads = [threading.Thread(target=nested_worker) for _ in range(4)]
        for thread in nested_threads:
            thread.start()
        nested_start.set()
        try:
            nested_ids = [submit_reload() for _ in range(6)]
            nested_results = [wait_reload(reload_id, timeout=20) for reload_id in nested_ids]
            if nested_results[-1]['status'] != 'succeeded':
                nested_errors.append(f'latest reload={nested_results[-1]}')
        except (AssertionError, KeyError, ValueError, json.JSONDecodeError) as exc:
            nested_errors.append(str(exc))
        for thread in nested_threads:
            thread.join(timeout=20)
            if thread.is_alive():
                nested_errors.append('worker timeout')
        if nested_errors:
            fail('behavior/tcc-vfs-concurrent', '; '.join(nested_errors[:4]))

        # B3d 结果环：一个进入 Init 的慢 active 占住模槽时，连续超过环容量
        # 的 latest-wins ticket 仍应扫描其他终态槽，而不是全部返回 503。
        ring_cfg = json.loads(cfg_path.read_text(encoding='utf-8'))
        for service in ring_cfg['services']:
            if service.get('name') == 'main':
                service['host_default']['init_delay_ms'] = 5000
        cfg_path.write_text(json.dumps(ring_cfg, ensure_ascii=False, indent=2), encoding='utf-8')
        ring_ids = []
        try:
            ring_ids.append(submit_reload())
            time.sleep(1.0)  # 让 active 越过 staging 线性点进入 ServiceInit
            ring_conn = http.client.HTTPConnection('127.0.0.1', http_port, timeout=8)
            try:
                for _ in range(270):
                    ring_conn.request('GET', '/reload')
                    response = ring_conn.getresponse()
                    payload = response.read()
                    if response.status != 202:
                        raise AssertionError(f'status={response.status} body={payload[:80]!r}')
                    ring_ids.append(int(json.loads(payload)['reload_id']))
            finally:
                ring_conn.close()
        except (OSError, http.client.HTTPException, AssertionError,
                ValueError, KeyError, json.JSONDecodeError) as exc:
            fail('behavior/reload-result-ring-submit', str(exc))
        finally:
            cfg_path.write_text(json.dumps(cfg, ensure_ascii=False, indent=2), encoding='utf-8')
        if ring_ids:
            try:
                ring_slow = wait_reload(ring_ids[0], timeout=12)
                ring_latest = wait_reload(ring_ids[-1], timeout=25)
                if ring_slow['status'] != 'succeeded' or ring_latest['status'] != 'succeeded':
                    fail('behavior/reload-result-ring-latest',
                         f'slow={ring_slow} latest={ring_latest}')
            except (AssertionError, KeyError, ValueError, json.JSONDecodeError) as exc:
                fail('behavior/reload-result-ring-latest', str(exc))

        # B4 定时器
        local_http('/tick')
        time.sleep(0.5)
        _, tick = local_http('/tick-get')
        if tick == b'0':
            fail('behavior/timer', 'timer never fired')

        # B5 tcp idle：连接静默 4.5s → 服务端应已关闭（EOF）
        s6 = socket.create_connection(('127.0.0.1', tcp_port), timeout=3)
        s6.settimeout(6)
        s6.recv(200)  # banner
        t0 = time.time()
        try:
            data = s6.recv(100)
            elapsed = time.time() - t0
            if data != b'' or elapsed > 5.5:
                fail('behavior/tcp-idle', f'data={data[:20]!r} elapsed={elapsed:.1f}')
        except socket.timeout:
            fail('behavior/tcp-idle', 'not closed after idle window')
        s6.close()

        # B6 静态层旋钮（主配置已带 static 四旋钮）
        import http.client as _hc
        c = _hc.HTTPConnection('127.0.0.1', http_port, timeout=5)
        c.request('GET', '/')
        r = c.getresponse(); body = r.read(); hdrs = dict(r.getheaders()); c.close()
        if hdrs.get('X-XS-Static') != 'on' or hdrs.get('Cache-Control') != 'no-cache':
            fail('behavior/static-headers', f'{hdrs}')
        if b'xs3 static ok' not in body:
            fail('behavior/static-index', f'{body[:40]!r}')
        s404, body404 = local_http('/no-such')
        if s404 != 404 or b'custom 404 page' not in body404:
            fail('behavior/static-404', f'status={s404} body={body404[:60]!r}')
        s403, _ = local_http('/.hidden')
        if s403 != 403:
            fail('behavior/static-dotfile', f'status={s403} expect 403')

        # 根句柄边界：解码后的 rooted path/NUL/反斜杠/父级段均不得进入文件层。
        static_escape_cases = (
            ('double-slash', '//Windows/win.ini', {400, 403}),
            ('encoded-slash', '/%2FWindows/win.ini', {400, 403}),
            ('encoded-nul', '/bad%00name', {400}),
            ('encoded-backslash', '/%5CWindows%5Cwin.ini', {403}),
            ('parent', '/../outside-secret.txt', {403}),
            ('parent-percent-upper', '/%2E%2E/outside-secret.txt', {403}),
            ('parent-percent-lower', '/%2e%2e/outside-secret.txt', {403}),
            ('parent-percent-mixed', '/%2E%2e/outside-secret.txt', {403}),
        )
        for case_name, target, expected in static_escape_cases:
            try:
                status, escaped_body = raw_http(
                    http_port,
                    f'GET {target} HTTP/1.1\r\nHost: default.example.com\r\n'
                    'Connection: close\r\n\r\n'.encode())
                if status not in expected or b'must-not-escape-static-root' in escaped_body:
                    fail(f'behavior/static-root-{case_name}',
                         f'status={status} body={escaped_body[:50]!r}')
            except (OSError, ValueError, IndexError) as exc:
                fail(f'behavior/static-root-{case_name}', str(exc))
        if symlink_ready:
            link_status, link_body = local_http('/outside-link')
            if link_status == 200 or b'must-not-escape-static-root' in link_body:
                fail('behavior/static-root-symlink',
                     f'status={link_status} body={link_body[:50]!r}')

        # 大文件必须在有界写队列上经历多次背压后完整传输，且排空后
        # 同一 keep-alive 连接必须能继续解析下一条请求。
        try:
            large_conn = _hc.HTTPConnection('127.0.0.1', http_port, timeout=15)
            large_conn.request('GET', '/large.bin')
            large_response = large_conn.getresponse()
            large_body = large_response.read()
            large_conn.request('GET', '/text')
            next_response = large_conn.getresponse()
            next_body = next_response.read()
            large_conn.close()
            if large_response.status != 200 or len(large_body) != 4 * 1024 * 1024 or \
                    large_body != b'x' * (4 * 1024 * 1024):
                fail('behavior/static-large-complete',
                     f'status={large_response.status} bytes={len(large_body)}')
            if next_response.status != 200 or b'xs3 http ok' not in next_body:
                fail('behavior/static-large-keepalive',
                     f'status={next_response.status} body={next_body[:40]!r}')
        except (OSError, _hc.HTTPException) as exc:
            fail('behavior/static-large-complete', str(exc))

        # Header 已发出后让客户端以 RST 中断大文件；驱动只能走连接终态，
        # 不得补发错误响应、悬挂 generation 或拖垮后续请求。
        try:
            reset_client = socket.create_connection(('127.0.0.1', http_port), timeout=3)
            reset_client.settimeout(3)
            reset_client.sendall(
                b'GET /large.bin HTTP/1.1\r\nHost: default.example.com\r\n\r\n')
            response_head = b''
            while b'\r\n\r\n' not in response_head and len(response_head) < 8192:
                response_head += reset_client.recv(1024)
            linger_format = 'hh' if os.name == 'nt' else 'ii'
            reset_client.setsockopt(
                socket.SOL_SOCKET, socket.SO_LINGER, struct.pack(linger_format, 1, 0))
            reset_client.close()
            time.sleep(0.2)
            health_status, health_body = local_http('/text')
            if b' 200 ' not in response_head.split(b'\r\n', 1)[0] or \
                    health_status != 200 or b'xs3 http ok' not in health_body:
                fail('behavior/static-send-failure',
                     f'head={response_head[:60]!r} health={health_status}/{health_body[:30]!r}')
        except (OSError, ValueError) as exc:
            fail('behavior/static-send-failure',
                 f'{exc}; server_exit={proc.poll()}')

        # B7 ws 大消息超限（ws_message_limit 未配置 → 内核默认，跳过）
    finally:
        if proc.poll() is None:
            proc.kill()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            pass
        log.close()
        cfg_path.unlink(missing_ok=True)
        fixture.cleanup()


def main():
    global EXE

    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', type=Path, default=EXE)
    args = parser.parse_args()
    EXE = args.exe.resolve()
    config_matrix()
    listener_failure_matrix()
    tls_identity_diagnostic_matrix()
    tls_static_large_matrix()
    behavior_matrix()
    if failures:
        print('FUNC TEST FAILURES (%d):' % len(failures))
        for item in failures:
            print(' -', item)
        return 1
    print('FUNC TEST OK')
    return 0


if __name__ == '__main__':
    sys.exit(main())
