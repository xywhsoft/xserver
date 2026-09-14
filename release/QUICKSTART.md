# XServer 快速上手

**XServer** 是单文件 C 语言应用服务器：一个可执行文件 + 一份 C 源码 = 一个完整的网站 / API 服务。
编译器（TCC）、SDK 头文件、SQLite、系统库定义已全部内嵌在程序里，目标机上**什么都不用装**。

官网：<https://xs.xywhsoft.com>（开发指南 / 43 章教程书 / API 参考与全站搜索）

---

## 1. 三条命令跑起来

解压后进入目录：

```bash
# Linux / macOS
./xs demo/xs.json

# Windows
xs.exe demo\xs.json
```

浏览器打开 **<http://127.0.0.1:9081>** —— 页面上的三个按钮分别演示静态页、HTTP API 和 WebSocket 回显。
停止服务：在终端按 `Ctrl+C`。

## 2. 这个包里有什么

```
├── xs                程序本体（Linux；Windows 版为 xs.exe）
├── LICENSE           MIT 许可证
├── VERSION           版本与构建信息
├── QUICKSTART.md     本文件
├── demo/             极简示例 —— 3 个文件看懂全部契约
│   ├── xs.json       服务配置：http(9081) + ws(9082) 两个服务
│   ├── main.c        全部脚本代码约 200 行，含中文注释
│   └── wwwroot/      静态网站根目录（一页演示页）
└── demo-single/      完整工程范本 —— 路由 / 模板 / SQLite / 模块化分层
    ├── xs.json  main.c  route.h
    ├── db/  modules/  options/  page/  route_http/  template/
    └── wwwroot/
```

跑完整范本：`./xs demo-single/xs.json`，然后访问 <http://127.0.0.1:9080>。

## 3. 极简 demo 讲了什么

打开 `demo/main.c`，XServer 与脚本的**全部约定**就是六个按名字导出的函数：

| 函数 | 谁在调 | 作用 |
|---|---|---|
| `ServiceInit` / `ServiceUnit` | 宿主 | 每代脚本开始服务前 / 退役后 |
| `RequestProc` | http 服务 | 每个请求的入口；返回 `XS_OK`（已响应）/ `XS_FALLBACK`（交给 wwwroot 静态层）/ `XS_TAKEOVER`（接管连接） |
| `WsOpen` / `WsText` / `WsClose` | ws 服务 | 连接建立 / 收到文本消息 / 关闭；握手和分帧由宿主完成 |

配置 `demo/xs.json` 里声明了两个服务，共用同一份 `main.c`：
http 服务的请求进 `RequestProc`，未命中的 URI 自动落到 `wwwroot/`；
ws 服务 9082 端口的连接由宿主完成 WebSocket 握手后进 `WsText`。

修改 `main.c` 或 `wwwroot/` 下的文件后重启（或按官网指南配置热重载）即可看到变化。

## 4. 把它变成你自己的服务

1. 复制 `demo/` 为新目录，改 `xs.json` 里的 `name`、`ip`、`port`；
2. 在 `RequestProc` 里加你的 URI 分支（仿照 `/api/hello`）；
3. 静态文件丢进 `wwwroot/`；
4. 需要数据库、模板、动态路由 `{id}` 时，参考 `demo-single/` —— 那就是一张完整的工程地图。

需要 HTTPS：在服务配置里挂证书（参照官网指南《TLS 与证书》章节）。

## 5. 常见问题

- **端口被占用**：`bind failed` 说明端口已在使用，改 `xs.json` 里的 `port`。
- **局域网访问不到**：demo 默认只监听 `127.0.0.1`，把 `ip` 改为 `0.0.0.0` 并放行防火墙。
- **Linux 上无法运行**（`GLIBC_x.xx not found`）：程序依赖的 glibc 版本比系统新，查看 `VERSION` 文件里的基线说明，换用 musl 静态版或升级系统。
- **脚本编译错误**：启动日志会给出 `tcc` 的精确行号（虚拟文件路径 `/xs/script/*.c` 的行号 = `main.c` 的行号）。
- **改了脚本没生效**：重启进程，或按官网指南配置热重载后用重载接口。

## 6. 下一步

- 开发指南：<https://xs.xywhsoft.com/guide.html>
- 教程书（43 章，从零到生产）：<https://xs.xywhsoft.com/book/index.html>
- API 参考（契约 14 符号 / xs 层 20 API / xrt 2957 符号索引）：<https://xs.xywhsoft.com/api.html>
- 源码仓库：<https://gitee.com/xywhsoft/xserver> · <https://github.com/xywhsoft/xserver>
