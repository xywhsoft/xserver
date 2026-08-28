#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
sh "$SCRIPT_DIR/tools/xs_script_demo_smoke.sh" "$@"
