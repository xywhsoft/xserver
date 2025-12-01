#!/bin/bash

gcc -g main.c lib/xrt/xrt.c lib/mongoose.c lib/sqlite3.c tcc/libtcc.c -DMG_TLS=MG_TLS_BUILTIN -o xs_debug
