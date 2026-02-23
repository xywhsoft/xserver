#!/bin/bash

gcc main.c lib/xrt/xrt.c lib/mongoose.c lib/sqlite3.c tcc/libtcc.c -DMG_TLS=MG_TLS_BUILTIN -O2 -s -ffunction-sections -fdata-sections -Wl,--gc-sections -o release/xs
