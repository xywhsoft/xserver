#!/bin/sh

set -eu

OS_NAME=$(uname -s 2>/dev/null || echo unknown)

case "$OS_NAME" in
	MINGW*|MSYS*|CYGWIN*|Windows_NT)
		powershell -ExecutionPolicy Bypass -File tools/xs_soak_check.ps1 "$@"
		;;
	*)
		sh tools/xs_soak_check.sh "$@"
		;;
esac
