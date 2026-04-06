#!/bin/sh

set -eu

if [ "${XS_TEST_STABLE_BG_RUNNING:-0}" != "1" ] && [ "${1:-}" = "--bg-log" ]; then
	shift

	SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
	LOG_DIR="$SCRIPT_DIR/tools"
	LOG_FILE="$LOG_DIR/test_stable_bg.log"

	mkdir -p "$LOG_DIR"
	rm -f "$LOG_FILE" "$LOG_FILE.pid"

	XS_TEST_STABLE_BG_RUNNING=1 nohup sh "$0" "$@" >"$LOG_FILE" 2>&1 </dev/null &
	i_pid=$!

	printf "%s\n" "$i_pid" > "$LOG_FILE.pid"
	printf "started pid=%s log=%s\n" "$i_pid" "$LOG_FILE"
	exit 0
fi

OS_NAME=$(uname -s 2>/dev/null || echo unknown)

case "$OS_NAME" in
	MINGW*|MSYS*|CYGWIN*|Windows_NT)
	cmd //c build.bat
	cmd //c build_debug.bat
	;;
	*)
	sh build.sh
	sh build_debug.sh
	;;
esac

sh tools/xs_stable_smoke.sh "$@"
