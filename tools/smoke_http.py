"""xs3 冒烟基线：HTTP 行为断言（路由三态/静态/错误码/keep-alive/takeover）。
由 test.bat 调用；任一断言失败以非零退出。"""
import http.client
import argparse
import socket
import sys

parser = argparse.ArgumentParser(add_help=False)
parser.add_argument('--port', type=int, default=8080)
args = parser.parse_args()

HOST, PORT = '127.0.0.1', args.port
failures = []


def check(name, method, path, body=None, expect_status=None, expect_body=None):
    try:
        c = http.client.HTTPConnection(HOST, PORT, timeout=3)
        c.request(method, path, body=body)
        r = c.getresponse()
        data = r.read()
        c.close()
        ok = (expect_status is None or r.status == expect_status) and \
             (expect_body is None or expect_body in data)
        if not ok:
            failures.append(f"{name}: got {r.status} {data[:60]!r}")
    except Exception as exc:  # noqa: BLE001
        failures.append(f"{name}: exception {exc}")


check('text', 'GET', '/text', expect_status=200, expect_body=b'xs3 http ok')
check('json', 'GET', '/json', expect_status=200, expect_body=b'"server":"xs3"')
check('post-echo', 'POST', '/echo', body=b'smoke-body', expect_status=200, expect_body=b'smoke-body')
check('static', 'GET', '/', expect_status=200, expect_body=b'xs3 static ok')
check('404', 'GET', '/no-such', expect_status=404)
check('403-traversal', 'GET', '/../xs.json', expect_status=403)
check('403-dotfile', 'GET', '/.hidden', expect_status=403)
check('405', 'POST', '/', body=b'x', expect_status=405)

try:
    c = http.client.HTTPConnection(HOST, PORT, timeout=3)
    c.request('GET', '/text')
    r1 = c.getresponse()
    r1.read()
    c.request('GET', '/json')
    r2 = c.getresponse()
    r2.read()
    c.close()
    if r1.status != 200 or r2.status != 200:
        failures.append('keep-alive: status mismatch')
except Exception as exc:  # noqa: BLE001
    failures.append(f'keep-alive: exception {exc}')

try:
    s = socket.create_connection((HOST, PORT), timeout=3)
    s.settimeout(2)
    s.sendall(b'GET /takeover HTTP/1.1\r\nHost: x\r\n\r\n')
    data = s.recv(200)
    s.close()
    if b'taken over by script' not in data:
        failures.append(f'takeover: {data[:60]!r}')
except Exception as exc:  # noqa: BLE001
    failures.append(f'takeover: exception {exc}')

if failures:
    print('HTTP SMOKE FAILURES:')
    for item in failures:
        print(' -', item)
    sys.exit(1)
print('HTTP SMOKE OK')
