#!/bin/sh

set -eu

CONFIG="${1:-xs_script_demo.json}"
PORT="${2:-18081}"
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
RELEASE_DIR="$REPO_ROOT/release"
OS_NAME=$(uname -s 2>/dev/null || echo unknown)

case "$OS_NAME" in
	MINGW*|MSYS*|CYGWIN*|Windows_NT)
	powershell -ExecutionPolicy Bypass -File "$SCRIPT_DIR/xs_script_demo_smoke.ps1" "$CONFIG" "$PORT"
	exit $?
	;;
esac

proc_fetch_status() {
	curl -s --max-time 2 -o "$BODY_FILE" -w "%{http_code}" "$1"
}

proc_wait_ready() {
	i=0
	while [ "$i" -lt 50 ]; do
		if [ "$(proc_fetch_status "http://127.0.0.1:$PORT/test" || true)" = "200" ]; then
			return 0
		fi
		i=$((i + 1))
		sleep 0.2
	done
	return 1
}

proc_check_body() {
	s_name="$1"
	s_expect="$2"
	i_status=$(proc_fetch_status "http://127.0.0.1:$PORT/$s_name" || true)
	s_body=$(cat "$BODY_FILE" 2>/dev/null || true)
	if [ "$i_status" != "200" ]; then
		echo "FAIL $s_name status=$i_status"
		return 1
	fi
	case "$s_body" in
		*"$s_expect"*)
			echo "OK   script_demo $s_name"
			return 0
			;;
	esac
	echo "FAIL $s_name missing=$s_expect"
	return 1
}

BODY_FILE="$SCRIPT_DIR/.xs_script_demo_body.tmp"
PID_FILE="$SCRIPT_DIR/.xs_script_demo.pid"

rm -f "$BODY_FILE" "$PID_FILE"
pkill -f '/xs([.]exe)? .*xs_script_demo[.]json' >/dev/null 2>&1 || true
pkill -x xs >/dev/null 2>&1 || true

(
	cd "$RELEASE_DIR"
	./xs "$CONFIG"
) >/dev/null 2>&1 &
i_pid=$!
echo "$i_pid" > "$PID_FILE"

cleanup() {
	if [ -f "$PID_FILE" ]; then
		i_pid="$(cat "$PID_FILE" 2>/dev/null || true)"
		if [ -n "${i_pid:-}" ]; then
			kill "$i_pid" >/dev/null 2>&1 || true
		fi
	fi
	rm -f "$BODY_FILE" "$PID_FILE"
}

trap cleanup EXIT INT TERM

if ! proc_wait_ready; then
	echo "FAIL server not ready"
	exit 1
fi

proc_check_body "test" "page load success"
proc_check_body "template" "<html"
proc_check_body "chart/get" "\"series\""
