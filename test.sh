#!/usr/bin/env bash
# xs3 Linux regression entry: official build, dynamic-port smoke, then full matrices.
set -euo pipefail

XS_TEST_ROOT=$(cd -- "$(dirname -- "$0")" && pwd)
XS_SMOKE_CFG="$XS_TEST_ROOT/release/xs_smoke_config.json"
XS_SMOKE_LOG="$XS_TEST_ROOT/release/xs_smoke.log"
XS_SMOKE_PID=""

cleanup() {
    if [[ -n "$XS_SMOKE_PID" ]] && kill -0 "$XS_SMOKE_PID" 2>/dev/null; then
        kill -TERM "$XS_SMOKE_PID" 2>/dev/null || true
        wait "$XS_SMOKE_PID" 2>/dev/null || true
    fi
    rm -f -- "$XS_SMOKE_CFG"
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

cd "$XS_TEST_ROOT"
bash ./build.sh

read -r XS_HTTP_PORT XS_UDP_PORT XS_TCP_PORT XS_WS_PORT XS_ECHO_PORT \
    < <(python3 tools/smoke_config.py release/xs.json "$XS_SMOKE_CFG")
: "${XS_HTTP_PORT:?missing HTTP port}"
: "${XS_UDP_PORT:?missing UDP port}"
: "${XS_TCP_PORT:?missing TCP port}"
: "${XS_WS_PORT:?missing WS port}"
: "${XS_ECHO_PORT:?missing custom echo port}"

rm -f -- "$XS_SMOKE_LOG"
(
    cd release
    exec ./xs xs_smoke_config.json
) >"$XS_SMOKE_LOG" 2>&1 &
XS_SMOKE_PID=$!

python3 - "$XS_HTTP_PORT" "$XS_ECHO_PORT" "$XS_TCP_PORT" "$XS_WS_PORT" <<'PY'
import socket
import sys
import time

ports = {int(value) for value in sys.argv[1:]}
deadline = time.time() + 20
while time.time() < deadline:
    for port in tuple(ports):
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.25):
                ports.remove(port)
        except OSError:
            pass
    if not ports:
        raise SystemExit(0)
    else:
        time.sleep(0.05)
raise SystemExit(f"SMOKE FAIL: endpoints did not become ready: {sorted(ports)}")
PY

grep -q "config loaded" "$XS_SMOKE_LOG"
grep -q "engine started" "$XS_SMOKE_LOG"
grep -q "script loaded" "$XS_SMOKE_LOG"
grep -q "server 'echo' custom ready" "$XS_SMOKE_LOG"
grep -q "server 'tcp-echo' tcp bound on" "$XS_SMOKE_LOG"
grep -q "server 'telemetry' udp bound on" "$XS_SMOKE_LOG"
grep -q "server 'main' http bound on" "$XS_SMOKE_LOG"
grep -q "server 'ws-echo' ws bound on" "$XS_SMOKE_LOG"

python3 tools/smoke_ws.py --port "$XS_WS_PORT"
python3 tools/smoke_http.py --port "$XS_HTTP_PORT"
python3 - "$XS_ECHO_PORT" "$XS_TCP_PORT" "$XS_UDP_PORT" <<'PY'
import socket
import sys
import time

echo_port, tcp_port, udp_port = map(int, sys.argv[1:])

with socket.create_connection(("127.0.0.1", echo_port), timeout=3) as stream:
    stream.settimeout(2)
    stream.recv(200)
    stream.sendall(b"xs3-smoke")
    time.sleep(0.1)
    assert stream.recv(200) == b"xs3-smoke"

with socket.create_connection(("127.0.0.1", tcp_port), timeout=3) as stream:
    stream.settimeout(2)
    assert stream.recv(200).startswith(b"[xs3-tcp]")
    stream.sendall(b"tcp-smoke")
    time.sleep(0.1)
    assert stream.recv(200) == b"tcp-smoke"

with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as datagram:
    datagram.settimeout(2)
    datagram.sendto(b"udp-smoke", ("127.0.0.1", udp_port))
    payload, _ = datagram.recvfrom(2048)
    assert payload == b"udp-smoke"
PY

kill -TERM "$XS_SMOKE_PID"
wait "$XS_SMOKE_PID"
XS_SMOKE_PID=""
grep -q "engine stopped" "$XS_SMOKE_LOG"
grep -q "\[xs\] bye" "$XS_SMOKE_LOG"

python3 tools/func_test.py --exe release/xs
python3 tools/lifecycle_reload_test.py --exe release/xs
python3 tools/reload_matrix_test.py --exe release/xs

echo "SMOKE PASS"
