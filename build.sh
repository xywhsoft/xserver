#!/usr/bin/env bash
# Examples: bash build.sh | bash build.sh sqlite xtp
set -euo pipefail
XS_BUILD_ROOT=$(cd -- "$(dirname -- "$0")" && pwd)
exec python3 "$XS_BUILD_ROOT/tools/build.py" "$@"
