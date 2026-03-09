gcc main.c lib/sqlite3.c tcc/libtcc.c -lshlwapi -lgdi32 -lws2_32 -lIPHLPAPI -lbcrypt -O2 -s -ffunction-sections -fdata-sections -Wl,--gc-sections -o release\xs.exe
cd release
cmd