#!/bin/sh
# musl-gcc wrapper: pass through compile steps, append -static at link stage.
for a in "$@"; do
  case "$a" in -c|-E|-S)
    exec /home/ubuntu/x86_64-linux-musl-cross/bin/x86_64-linux-musl-gcc "$@"
    ;;
  esac
done
exec /home/ubuntu/x86_64-linux-musl-cross/bin/x86_64-linux-musl-gcc -static "$@"
