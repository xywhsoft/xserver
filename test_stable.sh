#!/bin/sh

set -eu

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
