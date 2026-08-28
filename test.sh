#!/bin/bash
# xs3 骨架基线：编译 + 配置装载 + 优雅停机（Linux 直接用 SIGTERM）
set -e
cd "$(dirname "$0")"

gcc main.c -Ilib -O2 -s -Wall -o release/xs -ldl -lpthread

cd release
rm -f xs_smoke.log
./xs > xs_smoke.log 2>&1 &
XPID=$!
sleep 1.5
kill -TERM $XPID
wait $XPID || true

grep -q "config loaded" xs_smoke.log || (echo "SMOKE FAIL: config"; exit 1)
grep -q "engine started" xs_smoke.log || (echo "SMOKE FAIL: engine"; exit 1)
grep -q "engine stopped" xs_smoke.log || (echo "SMOKE FAIL: graceful stop"; exit 1)
grep -q "\[xs\] bye" xs_smoke.log || (echo "SMOKE FAIL: exit"; exit 1)

echo "SMOKE PASS"
