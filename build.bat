@echo off
rem xs3 build: XRT_MODULE_ALL + TCC host (builtin VFS, no on-disk tcc env)
rem 用法: build.bat [sqlite]   —— sqlite 变体额外编入 sqlite3 并导出符号
rem 1) LZMA 打包工具  2) VFS 资源生成（res/tcc + SDK -> 内置）  3) 宿主编译
gcc tools\tcc_vfs_lzma_pack.c tcc\LzmaEnc.c tcc\LzFind.c tcc\CpuArch.c -I tcc -DZ7_ST -O2 -s -o tools\tcc_vfs_lzma_pack.exe
if errorlevel 1 exit /b 1
python tools\gen_tcc_resources.py
if errorlevel 1 exit /b 1
windres res\xs.rc -O coff -o build_tmp_icon.o
if errorlevel 1 exit /b 1
set XS_EXTRA=
set XS_SQLITE_OBJ=
if "%1"=="sqlite" (
	set XS_EXTRA=-DXS_USE_SQLITE
	set XS_SQLITE_OBJ=build_tmp_sqlite.o
	gcc -c lib\sqlite3.c -Ilib -O2 -w -o build_tmp_sqlite.o
	if errorlevel 1 exit /b 1
)
gcc -c main.c %XS_EXTRA% -Ilib -Itcc -DCONFIG_TCC_BUILTIN_VFS -O2 -Wall -o build_tmp_main.o
if errorlevel 1 exit /b 1
gcc build_tmp_main.o %XS_SQLITE_OBJ% tcc\libtcc.c tcc\tcc_builtin_vfs.c tcc\tcc_utf8_io.c tcc\LzmaDec.c src\script\tcc_builtin_resources.c ^
	-Ilib -Itcc -DCONFIG_TCC_BUILTIN_VFS -O2 -s ^
	-o release\xs.exe build_tmp_icon.o -lws2_32 -lshell32 -liphlpapi
