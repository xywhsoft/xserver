#!/bin/bash

gcc main.c lib/xrt/xrt.c lib/mmu.c lib/mongoose.c lib/xtemplate.c lib/sqlite3.c lib/json.c lib/jnum.c tcc/libtcc.c -DMG_TLS=MG_TLS_BUILTIN -o2 -s -ffunction-sections -fdata-sections -Wl,--gc-sections -o release/xs
