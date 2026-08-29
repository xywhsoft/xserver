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
        [str(RELEASE / 'xs.exe'), 'func_test_config.json'],
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
            if port_status != 200 or b'admin script ok' not in port_body or \
                    duplicate_status != 400 or missing_status != 400 or \
                    legacy_status != 200 or b'xs3 static ok' not in legacy_body:
                fail('behavior/vhost-host-rules',
                     f'port={port_status}/{port_body[:30]!r} duplicate={duplicate_status} '
                     f'missing={missing_status} legacy={legacy_status}/{legacy_body[:30]!r}')
        except (OSError, ValueError, IndexError) as exc:
            fail('behavior/vhost-host-rules', str(exc))

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
                failed_result = wait_reload(submit_reload())
                if failed_result['status'] != 'failed':
                    fail('behavior/reload-failed-status', f'result={failed_result}')
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
        admin_script_path = RELEASE / 'hosts' / 'admin' / 'main.c'
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
