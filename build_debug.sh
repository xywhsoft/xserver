#!/bin/bash

gcc main.c lib/sqlite3.c tcc/libtcc.c -DXRT_BUILD_CORE -DXRT_MEM_DEBUG -g -O0 -ldl -lpthread -o release/xsdbg
