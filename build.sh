#!/bin/bash

gcc main.c lib/sqlite3.c tcc/libtcc.c -O2 -s -ffunction-sections -fdata-sections -Wl,--gc-sections -ldl -lpthread -o release/xs
