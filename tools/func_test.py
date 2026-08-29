"""xs3 功能测试：配置错误矩阵 + 协议行为矩阵 + 重载语义 + idle 超时。

用法：python tools/func_test.py  （在仓库根执行；自动启动独立实例，独立临时配置）
输出：逐项 PASS/FAIL，任一失败非零退出。
"""
import base64
import http.client
import json
import os
import socket
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RELEASE = ROOT / 'release'
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
        ('type-error', '{"services":[{"class":"tcp","name":"x","port":"80"}]}', 'expect int'),
    ]
    for name, text, expect in cases:
        with tempfile.NamedTemporaryFile('w', suffix='.json', delete=False, encoding='utf-8') as fp:
            fp.write(text)
            path = fp.name
        try:
            proc = subprocess.run(
                [str(RELEASE / 'xs.exe'), path],
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


def behavior_matrix():
    cfg = json.load(open(RELEASE / 'xs.json', encoding='utf-8'))
    ports = {}
    used_ports = set()
    for s in cfg['services']:
        if s.get('class') != 'custom':
            sock_type = socket.SOCK_DGRAM if s.get('class') == 'udp' else socket.SOCK_STREAM
            s['port'] = free_port(sock_type, used_ports)
            ports[s.get('name')] = s['port']
        if s.get('name') == 'tcp-echo':
            s['idle_timeout'] = 3000
    http_port = ports['main']
    tcp_port = ports['tcp-echo']

    def local_http(path, host_header=None, method='GET', body=None):
        return http_get(path, host_header=host_header, port=http_port, method=method, body=body)

    cfg_path = RELEASE / 'func_test_config.json'
    json.dump(cfg, open(cfg_path, 'w', encoding='utf-8'), ensure_ascii=False, indent=2)

    log = open(tempfile.gettempdir() + '/xs_func.log', 'w')
    proc = subprocess.Popen(
        [str(RELEASE / 'xs.exe'), 'func_test_config.json'],
        cwd=str(RELEASE), stdout=log, stderr=subprocess.STDOUT)
    try:
        if not wait_port(http_port):
            fail('behavior/startup', 'http port not up')
            return
        time.sleep(1.5)

        # B1 虚拟主机路由：admin Host → admin host（无静态根 → 404），默认 Host → 200
        s1, _ = local_http('/', host_header='admin.example.com')
        s2, _ = local_http('/')
        if not (s1 == 404 and s2 == 200):
            fail('behavior/vhost', f'admin={s1} default={s2} (expect 404/200)')

        # B2 body 上限：body_limit=262144 → 300KB body 触发 400/断连
        try:
            s3, _ = local_http('/echo', method='POST', body=b'x' * 300000)
            if s3 != 400:
                fail('behavior/body-limit', f'status={s3} expect 400')
        except (http.client.HTTPException, ConnectionError):
            pass  # 服务端直接断连也符合分帧错误处理

        # B3 重载语义：换代 / 回滚 / swap
        script = RELEASE / 'script' / 'http_main.c'
        src = script.read_text(encoding='utf-8')
        mod = src.replace('"xs3 http ok"', '"xs3 func RELOADED"')
        bad = src.replace('RequestProc(XS_HttpReq', 'RequestProc(XS_HttpReq BROKEN')
        try:
            script.write_text(mod, encoding='utf-8')
            s4, b4 = local_http('/reload')
            time.sleep(0.4)
            _, body = local_http('/text')
            if b'RELOADED' not in body:
                fail('behavior/reload-swap', f'body={body[:40]!r}')
            script.write_text(bad, encoding='utf-8')
            s5, _ = local_http('/reload')
            if s5 != 200:
                fail('behavior/reload-queue-status', f'status={s5}')
            time.sleep(0.4)  # reload API 只确认固定 worker 已接收；编译结果异步落状态/日志
            _, body = local_http('/text')
            if b'RELOADED' not in body:
                fail('behavior/reload-rollback', 'old gen lost')
        finally:
            script.write_text(src, encoding='utf-8')
            local_http('/reload')
            time.sleep(0.3)

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
        s404, _ = local_http('/no-such')
        # 主配置未配 error_pages 时为内置页；已配 err404.html 时为自定义页（二者择一断言状态）
        if s404 != 404:
            fail('behavior/static-404', f'status={s404}')
        s403, _ = local_http('/.hidden')
        if s403 != 403:
            fail('behavior/static-dotfile', f'status={s403} expect 403')

        # B7 ws 大消息超限（ws_message_limit 未配置 → 内核默认，跳过）
    finally:
        proc.kill()
        log.close()
        cfg_path.unlink(missing_ok=True)


def main():
    config_matrix()
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
