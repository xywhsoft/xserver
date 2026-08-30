# librarys 目录

此目录已通过 xs.json 的 `dev_lib` 注册为 TCC 库搜索路径。

把导入库（Windows 下 `.a` / `.def`）放入本目录后，脚本源码中用：

```c
#pragma comment(lib, "库名")
```

即可链接。目录名与配置值保持一致即可，改名时同步修改 `dev_lib`。
