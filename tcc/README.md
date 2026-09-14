# tcc/ — 内置 TCC 编译器(LGPL 组件)

本目录是 [TinyCC](https://repo.or.cz/tinycc.git) 的快照加本地修改,
作为 xs 的运行时 C 脚本编译器静态链入主程序。

## 来源与许可证

- 上游基线:tinycc mob 分支快照 `2025-03-20 @5527ca6d`(版本 0.9.28rc)。
- 上游文件(含 `tcc/lib/` 的 libtcc1 运行时源码、`tcc/include/` 头文件)
  遵循 **LGPL-2.1-or-later**,许可全文见本目录 [COPYING](COPYING)。
- 本地修改(均为 LGPL-2.1-or-later,作为 TCC 库的一部分):
  - `tcc.h` — `CONFIG_TCC_BUILTIN_VFS` 宏门控下把
    `open/close/read/lseek/fopen/fclose` 重映射到 VFS;未启用 VFS 时
    Windows 下重映射为 UTF-8 宽字符 IO。文件头有修改声明。
  - `tcc_builtin_vfs.c/.h` — 新增:内置资源 + 内存挂载 VFS。
  - `tcc_utf8_io.c/.h` — 新增:Windows UTF-8 路径宽字符 IO 桥。
  - `config.h` — 本地维护的静态配置(上游由 configure 生成)。
- LZMA SDK 文件(`LzmaDec.c`、`LzmaEnc.c`、`LzFind.c`、`CpuArch.c` 等)
  来自 Igor Pavlov 的 LZMA SDK,**公有领域**,文件头各自保留声明。

## LGPL 合规说明

xs 以静态链接方式将 TCC 编入单一二进制。按 LGPL-2.1 第 6 条,
本项目通过公开全部源码与构建脚本(`build.bat` / `build.sh` /
`tools/build.py`)满足"可修改 TCC 后重新链接生成完整程序"的要求;
修改版 TCC 部分保持 LGPL 并随仓库提供源码。项目自有代码的许可不受
影响,见根目录 `LICENSE` 与 `README.md` 的开源许可一节。
