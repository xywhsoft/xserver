#!/bin/bash
# xs3 build: XRT_MODULE_ALL + TCC host (builtin VFS, no on-disk tcc env)
set -e
cd "$(dirname "$0")"
gcc tools/tcc_vfs_lzma_pack.c tcc/LzmaEnc.c tcc/LzFind.c tcc/CpuArch.c -I tcc -DZ7_ST -O2 -s -o tools/tcc_vfs_lzma_pack
python3 tools/gen_tcc_resources.py
# 图标资源仅 Windows 构建需要（Linux ELF 无此机制）
WINDRES_OBJ=""
if command -v windres >/dev/null 2>&1; then
    windres res/xs.rc -O coff -o build_tmp_icon.o && WINDRES_OBJ="build_tmp_icon.o"
fi
gcc main.c tcc/libtcc.c tcc/tcc_builtin_vfs.c tcc/tcc_utf8_io.c tcc/LzmaDec.c src/script/tcc_builtin_resources.c \
	-Ilib -Itcc -DCONFIG_TCC_BUILTIN_VFS -O2 -s -Wall \
	-o release/xs $WINDRES_OBJ -ldl -lpthread
