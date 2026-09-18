# Webview 扩展与 app 节点设计

状态：M0 已完成并通过全部门禁与实测（2026-09-16，见 §14 实施记录）；M1 待启动。全部决策点已拍板（§13）。实施按 §12 里程碑推进。

目标：为 xs 增加第 14 个可选扩展 `webview`，并提供 `class: "app"` 服务节点——本地 http 服务 + webview 桌面窗口。与站点 VFS 单文件交付（app-pack）组合后，产物形态为"双击即用的单 exe 桌面应用"：启动显示加载窗，服务就绪自动打开界面，关窗优雅停机。无 webview 的环境自动降级为 headless 服务，启动与兼容性不受任何影响。

## 1. 技术选型与实测基线

选型 [webview/webview](https://github.com/webview/webview) 0.12.0（MIT）：单头文件 C++ 实现 + 官方 C API，Windows 后端 WebView2。以下数据为 2026-09-16 在本机 w64devkit gcc 实测：

| 项 | 数据 |
|---|---|
| 库源码 | `webview/webview.h` 单头 4,557 行 / 143 KB |
| 编进二进制增量 | +约 296 KB（`-O2 -s`：311 KB vs 空基线 15 KB；目标文件本体 73 KB，余为静态链 C++ 运行时碎片） |
| 运行时新增 DLL | 零。导入表仅系统 DLL；`WebView2Loader.dll` 运行时从已装 Runtime 目录动态定位 |
| 编译耗时 | 单 TU 约 1.9 秒 |
| 编译期依赖 | WebView2 SDK 头（NuGet `Microsoft.Web.WebView2`，包 9.3 MB，取 `WebView2.h` + `WebView2EnvironmentOptions.h` 约 3 MB，仅构建期） |
| 许可证 | webview = MIT；WebView2 SDK 头 = 微软 BSD 式（允许源码再分发，须保留声明）。vendor 时两份 LICENSE 一并入库 |

不选 CEF 的原因：最小分发包压缩后 144 MB（stable 144 实查），与"部署工具"定位不符；渲染一致性诉求由"Windows 是 app 节点唯一窗口平台"化解。

## 2. 扩展注册与构建

`tools/extensions.json` 新增条目（第 14 个扩展）：

```json
"webview": {
  "macro": "XS_WITH_WEBVIEW",
  "sources": [
    { "path": "src/webview/xs_app_window.c" },
    { "path": "src/webview/webview_impl.cpp", "lang": "c++",
      "platforms": ["windows"], "flags": ["-std=c++17"] }
  ],
  "host_includes": ["lib/webview"],
  "link_flags": {
    "windows": ["-lstdc++", "-lole32", "-lshlwapi", "-luser32", "-lversion", "-ladvapi32"],
    "linux": []
  }
}
```

构建系统增量：

- `tools/build.py`：`compile_source` 的 `-x c` 参数化（新字段 `lang`，默认 `c`）；源级 `platforms` 过滤。各约 5~8 行。
- `tools/xs_extensions.py`：允许字段表加 `lang`、`platforms`。
- `build.bat` / `build.sh` 用法注释补 `webview`。`all` 自动包含。
- vendor：`lib/webview/`（webview.h + WebView2 SDK 两头 + EventToken.h（MinGW 兼容）+ LICENSE + README 记录钉住的版本号，升级为手动动作）。
- TCC 面按注册表三件套约束注册最小探测符号（见下）。

门控语义：`XS_USE_WEBVIEW` 宏（注册表 `XS_USE_*` 命名约定）在**选中即定义（双平台）**；窗口代码编译条件 `#if defined(XS_USE_WEBVIEW) && defined(_WIN32)`。`class: "app"` 配置**始终可解析**（schema 常识 app），`window` 字段照常读入；行为差异见 §9 降级矩阵。"不生效"的忠实实现 = 窗口功能不发生、服务照常（保住同一份 xs.json 跨平台、跨变体可移植）。

TCC 面（M0 实施修正）：注册表三件套中 `headers`（VFS 头）与 `symbols`（TCC 导入表）为**必填字段**——扩展默认即脚本库。webview 按最小诚实面注册：VFS 头 `/xs/xs_webview.h` 声明、`import_webview.inc` 导出 `xsWebviewAvailable`/`xsWebviewVersion` 两个可用性探测符号；窗口控制面不进脚本（v1 纯显示层决策不变）。

## 3. app 节点语义

`class: "app"` 不是第六种协议，而是 http 节点的前端化：内部完全走既有 http 装配路径（`host_default`/`hosts`/`static`/TLS/热重载全部同义），只多一个 `window` 描述符与前端生命周期。generation、lease、reaper、reload 语义零改动。

```json
{
  "enabled": true, "class": "app", "name": "main",
  "ip": "127.0.0.1", "port": 0,
  "host_default": { "name": "app", "path": "wwwroot", "devlang": "c",
                    "devfile": "script/http_main.c", "static": { "index": ["index.html"] } },
  "window": {
    "title": "我的应用",
    "width": 1280, "height": 800, "min_width": 480, "min_height": 320,
    "url": "/",
    "devtools": false,
    "close": "stop",
    "install_fallback": "ask"
  }
}
```

字段规则：

- `ip` 默认 `127.0.0.1`（与 http 默认 `0.0.0.0` 不同；回环绑定不触发 Windows 防火墙弹窗）。
- `url` 相对路径拼 `http://<ip>:<PortBound>/…`，允许绝对 URL。
- `close`：`"stop"`（默认，关窗 = 停整个进程）| `"hide"`（隐藏窗口，服务继续，见 §7）。
- `install_fallback`：`"ask"`（默认，弹窗问询安装，缺省运行时场景）| `"silent"`（静默下载安装，无人值守）| `"headless"`（不尝试，直接降级）。`silent`/`headless` 失败一律落到 headless 继续，服务本体不受影响。
- `window` 描述符为固化字段（与 backlog/recv_limit 同待遇）：reload 改动提示需重启；运行中改 title/size 属 v2 可选。

## 4. 线程模型与启动序列

每个 app 节点一个 UI 线程（WebView2 要求 STA + 消息泵；webview 库自管）。服务端→UI 用 `webview_dispatch()`（线程安全，marshal 到 UI 线程）；UI→服务端用 `webview_run()` 返回（窗口已销毁）触发。loading 页为 data: URI 内联 HTML（转圈 + "正在启动服务…"），不依赖任何服务。

main.c 缝合点（对齐现有顺序）：

```
XS_ConfigLoad 成功
  ↓ XS_AppFrontendInit(&tApp)          ① 解析 app 节点，为每个开 UI 线程 + loading 窗
XS_EngineStartup / Reaper / Tls / Topology / ReloadRuntime （不变）
  ↓ XS_AssembleServers(&tApp)
  ├─ 成功 → XS_AppFrontendReady()      ② 每窗 dispatch 跳转目标 URL（用 PortBound 拼）
  └─ 失败 → XS_AppFrontendFail(err)    ③ 窗口切错误页（data: URI 显示错误 + 退出指引）
while ( !g_XS_Stop ) xrtSleep(100);    （不变）
```

早期失败路径（EngineStartup/TLS init 等失败）同样要调 FrontendFail/Unit，避免窗口停在 loading 直到进程退出。ready 判定 v1 用 `XS_AssembleServers` 整体成功；v2 如需多服务细化，在 topology 发布点加 per-server 回调。可选优化：跳转前 `webview_eval` 探活 XHR，避免首请求偶发慢造成的白屏观感。

## 5. 关窗停机（close: "stop"）

- 正向：UI 线程 `webview_run()` 返回 → `webview_destroy` → 调 `XS_RequestStop()`（把 main.c 的 `static volatile sig_atomic_t g_XS_Stop` 提升为 engine 侧导出，Ctrl-C handler 同改走它）。主循环退出后的整条优雅停机链（ServersDrain → … → ConfigFree）一行不动。
- 多窗口：引用计数，全部 app 窗口关闭才 RequestStop。
- 反向（Ctrl+C/外部停机）：`"[xs] stopping"` 后、`XS_ServersDrain` 前插入 `XS_AppFrontendUnit()`：逐窗终止 + join UI 线程，再继续排空。**终止必须经 `webview_dispatch()` marshal 回 UI 线程后调用 `webview_terminate()`**——0.12.0 的 terminate 实现是 `PostQuitMessage(0)`，WM_QUIT 只进调用线程队列，跨线程直接调用是空操作（M0 探针实测确认）。
- loading 阶段关窗：UI 线程只是显示层，不中断主线程装配；装配完成后主循环见 stop 置位自然进停机链。

## 6. xsw.exe（GUI 子系统发布变体）

同一份 objects 链接两遍：`xs.exe`（控制台，行为不变）+ `xsw.exe`（`-mwindows`，无控制台黑窗，面向 app-pack 双击场景）。

- 日志：xsw 变体将 stdout/stderr 重定向到 exe 同目录 `xsw.log`（freopen，启动期完成）。
- MinGW 下 `-mwindows` 与 `main()` 入口的兼容性（运行库提供 WinMain→main 桥）在 M0 验证；若不成立，改用 `-Wl,--subsystem,windows` + 显式入口适配。
- 图标：v1 复用 `res/xs.rc` 机制；按应用定制图标属 pack 工具后续增强。
- `make_release.py` 同批产出两个 exe。

## 7. close: "hide" 与单实例再唤起

- 关窗 = 隐藏（服务继续）。v1 不做托盘。
- 再唤起：单实例检测（命名 mutex，按 exe 路径派生名字）。再次启动同应用时：检测到已运行实例 → 通过唤起通道（v1 用命名 event，UI 线程等待该事件，触发时 `webview_dispatch` 显示窗口）唤醒既有窗口 → 新进程直接退出。估约 60~80 行。
- hide 模式下停止服务的途径回到 Ctrl+C / 信号路径。

## 8. 端口 0 自动分配

- 语义：`port: 0` = OS 自动分配临时端口。**v1 仅 `class: "app"` 允许**（校验：其他类报 `port 0 (auto) is only supported for class 'app'`）。多实例（双击两次）天然不冲突。
- 机制：0 直接透传既有路径（`src/protocol/http.h` `xrtNetAddrParse` → 监听创建，内核原子分配）；成功后 `xrtNetListenerLocal()`（`xrt_decl.h:20195`）回读，`xnetaddr.Port`（开放结构体，主机字节序）即实际端口。**禁止**"探测→关闭→再绑定"（TOCTOU 竞态）。全程零 xrt 改动。
- 回填：`XS_ServerInfo` 增运行期字段 `PortBound`（不动配置快照——`pServer` 有 `ConfigOwner` 借用关系）。启动日志（`http.h` 的 listen 成功/失败行）改打实际端口。前端拼 URL 用 `PortBound`（时序成立：绑定发生在 Assemble 内部，早于 FrontendReady）。脚本经运行期信息读到实际端口。
- 热重载粘滞：reload/topology 运行时维护 per-service-name 粘滞表（`name → 已解析端口`，存活期跨 generation，reload-all 换代不得重建）。候选构建：配置 port==0 且粘滞表有值 → 显式绑粘滞端口（端点不变，走既有"稳定槽位转交"，已开窗口无感知）；无值 → bind(0) 并登记；粘滞端口被占 → 候选失败按既有 reload 失败清理。不落盘；跨重启稳定请配固定端口。
- `PortTLS` v1 不放开（app 为本地回环明文）。

## 9. 平台与变体降级矩阵

| 构建变体 | 平台 | WebView2 状态 | 行为 |
|---|---|---|---|
| webview 扩展 | Windows | Runtime 已装 | 完整功能：loading 窗、自动开 URL、close 语义 |
| webview 扩展 | Windows | Runtime 缺 | 按 `install_fallback`：ask=问询（参考实现 `docs/webview_reference/wv2_fallback_ui.c`）；silent=静默安装；headless=直接降级；失败均落 headless |
| webview 扩展 | Linux | —（无后端） | headless：服务照常，日志打 `[xs] app 'main': open http://127.0.0.1:<port>/ in a browser` |
| 无扩展 | 任意 | — | 同 headless，外加一行 `[xs] app 'main': webview extension not built in, window ignored` |

前端永远 best-effort：窗口线程任何失败（创建失败、无 Runtime、用户拒绝安装、UI 线程异常）只降级为日志，绝不让引擎装配与停机链走 fail 路径。窗口是显示层，服务是本体。WebView2 渲染器为独立进程，页面崩溃不传染 xs。

## 10. 兼容性门禁与测试

- **门禁 A（Windows 链接层）**：构建后 `objdump -p xs.exe`/`xsw.exe` 的 DLL 导入做白名单断言——禁 `libstdc++-6.dll`、`WebView2Loader.dll`、`libgcc` 系列（允许：系统 DLL + 现有依赖）。
- **门禁 B（Linux 链接层）**：`build.sh all` 产物 `ldd` 断言无 webkit/gtk/X11；跑既有 headless 冒烟；musl 静态产物回归（Linux link_flags 为空，产物依赖与现状完全一致）。
- **无 Runtime 冒烟**：临时把注册表 `EdgeUpdate\ClientState\{F3017226-…}\pv` 改 `0.0.0.0` 模拟缺失，验证 ask/silent/headless 三路径。
- **窗口自动化**：测试钩子环境变量 `XS_APP_AUTOCLOSE_MS`（N 毫秒后自动关窗），CI 断言"关窗→优雅停机→退出码"全链路；视觉人工/VM 过。
- 端口 0：断言启动后 PortBound 非 0 且日志正确；reload 矩阵加"port 0 同端点换代端口不变"用例。

## 11. 参考实现归档

`docs/webview_reference/` 存两份已编译实测的独立参考实现（未接线，M1 迁入 `src/webview/` 改造）：

- `wv2_fallback.c`（74 行）：注册表三处检测 + 下载引导器 + 静默安装 + 复检。
- `wv2_fallback_ui.c`（219 行）：提示框 + 下载进度窗（`IBindStatusCallback` 真实百分比、单线程就地泵消息、`E_ABORT` 取消、安装期跑马灯）。

## 12. 实施里程碑与工作量

| 阶段 | 内容 | 验收 | 量 |
|---|---|---|---|
| M0（半天）✅ | vendor 库与 SDK 头；build.py `lang`/`platforms`；xsw.exe 双链接 + 日志重定向；门禁 A/B 进 check 套件 | `build.bat webview` 与 `build.sh all` 双绿；门禁通过；gcc 驱动 + `-lstdc++` 链接行为确认 | ~40 行构建脚本 |
| M0 验证清单 | COM 初始化归属（库自管 or 需前置）；data: URI loading 页表现；`-mwindows` 与 `main()` 兼容 | 骨架 demo 当场验证 | — |
| M1（1~2 天） | `src/webview/xs_app_window.c` + `webview_impl.cpp`；Frontend 四缝合点；`XS_RequestStop` 导出化；close stop/hide + 单实例唤起；install_fallback 三态 | app 配置端到端：loading→开 URL→关窗停机；hide/唤起；三态兜底 | ~400 行 |
| M2（半天） | 端口 0：校验放宽、PortBound、粘滞表、日志 | §10 端口用例 | ~90 行 |
| M3（半天） | 测试钩子 `XS_APP_AUTOCLOSE_MS`、无 Runtime 冒烟、reload 矩阵用例 | CI 四层回归绿 | ~60 行 |

合计约 670 行新代码、3~4 天。TCC 暴露（`import_webview.inc`）明确划 v2。

## 13. 决策记录（2026-09-16）

1. 出 GUI 子系统发布变体，命名 `xsw.exe`（§6）。
2. 缺 Runtime 兜底采用 `install_fallback: ask|silent|headless` 三态，默认 ask（§3）。
3. `close: "hide"` 进入 v1，附带单实例再唤起（§7）。
4. 无扩展时 app 类降级而非配置拒绝，保 xs.json 跨变体可移植（§2）。
5. 其余（端口 0 仅 app 开放、粘滞表、门禁 A/B、里程碑顺序、TCC v2、版本钉子）均按设计执行。

## 14. M0 实施记录（2026-09-16）

交付物：`lib/webview/`（vendor + 版本钉子 README）、`src/webview/{xs_webview.h, xs_app_window.c, webview_impl.cpp}`、`src/script/import_webview.inc`、`tools/check_webview_gates.py`、extensions.json 第 14 条目、build.py `lang`/`platforms`/xsw 双链接、xs_extensions.py 校验扩展、main.c xsw.log 重定向、build.bat 注释。

实测结果（w64devkit gcc，全绿）：

| 项 | 结果 |
|---|---|
| `build.py webview` | xs.exe + xsw.exe 产出；C++ 单元经 `gcc -x c++` 编译，链接含 `-lstdc++` |
| `build.py all`（14 扩展） | 6,472,704 B/变体（原 13 扩展 + 约 267 KB）；横幅列 webview |
| webview-only 变体 | 4,517,376 B（纯实现增量约 261 KB，符合 §1 预估） |
| 门禁 A | PASS：9 导入（+ADVAPI32/ole32/shlwapi/user32），**无 libstdc++-6.dll / libgcc / winpthread**——静态解析确认 |
| 默认变体回归（无扩展） | extensions: none；xsw.exe 照常产出；5 导入与改造前一致 |
| xsw.exe | `--version` exit 0、控制台零输出、横幅完整落入同目录 `xsw.log`（PE Subsystem 判定 + freopen） |
| Linux 模拟（os.name=posix dry-run） | C++ 单元被 platforms 过滤、C 桩照常编译、链接行仅 `-ldl -lpthread -lm`、无 xsw 产出 |
| data: URI 探针 | 独立 exe：create → data: URI 导航 → dispatch 终止 → exit 0；窗口标题/响应正常（视觉确认留 M1） |

M0 抓到并修正的三个实现级问题（均已写入对应章节）：

1. **terminate 跨线程语义**（§5 已修正）：0.12.0 的 `terminate_impl` 为 `PostQuitMessage(0)`，只作用于调用线程队列；跨线程停止必须 `webview_dispatch` marshal 回 UI 线程。首版探针实测挂起，修正后通过。
2. **GUI 变体判定**：`GetConsoleWindow()==NULL` 在 ConPTY/MSYS 终端下误判（xs.exe 在 Git Bash 中输出被重定向）。改为解析自身 PE 头 Subsystem 字段，精确且与终端无关。
3. **COM 归属**（M1 前置条件，vendored 头文档确认）：向 `webview_create` 传已有窗口前，调用方须先 `CoInitializeEx(COINIT_APARTMENTTHREADED)`；无父窗口路径库自管。M1 的 UI 线程入口统一前置 STA 初始化。另：`webview_create` 失败返回 `WEBVIEW_ERROR_MISSING_DEPENDENCY`，即 §9 兜底流程的触发信号。

M1 补充注意：仅含版本宏常量的探针函数无法把全 inline/COMDAT 的 webview 实现拉进链接（链接器可整体消除），`webview_impl.cpp` 已内置调用真实 C API 的 `xsWebviewImplLinkProbe` 维持链接存在感；M1 控制面就位后由真实调用自然取代。

## 15. M1 实施记录（2026-09-16）

交付物：`src/core/appnode.h`（前端调度四入口 + `XS_AppUrlBuild` + headless 降级）、`src/webview/xs_app_window.c`（约 700 行：UI 线程、导航状态机、单实例唤起、Runtime 三态兜底安装——迁移自参考实现）、`webview_impl.cpp` 扩为 C ABI 出口（9 个薄封装）、`main.c` 12 个缝合点（含 `g_XS_Stop` 去静态 + `XS_RequestStop`）、class 分发 `XS_ClassIsHttp` 助手替换 12 处判定（driver 8 / reload 3 / config 1）、config app 白名单 + ip 缺省 127.0.0.1、extensions.json 链接库补 `gdi32/urlmon/comctl32`。

实测（全绿）：

| 用例 | 结果 |
|---|---|
| E2E 主链路 | loading 窗 → 装配 → `frontend ready -> http://127.0.0.1:18971/` → 窗口导航；`taskkill`（WM_CLOSE）→ 关窗 → 完整优雅停机链至 `bye`，端口释放 |
| close=hide | WM_CLOSE 后服务持续 HTTP 200、`MainWindowHandle=0`（窗口隐藏）、进程存活 |
| 单实例唤起 | 二次实例打印 `another instance running, wake signal sent`，exit 0 未抢端口；首实例窗口恢复 |
| headless 回归 | 无扩展变体接受同配置：`app 'main': open http://… in a browser`，服务照常，无窗口 |
| all 变体 + 门禁 A | 14 扩展 6,491,136 B；12 导入全为系统 DLL，仍无 libstdc++-6.dll/libgcc |

M1 抓到并修复的实现缺陷：

1. **`snprintf` 源与目标重叠**（`XS_AppUrlBuild` 首版把 `sOut` 同时当源和目标，产生自我放大输出 `http://…18971http://…18971`）。教训写死：URL 组装一律先进局部缓冲。
2. **Ready/Fail 与 WebView2 初始化的竞态**：装配毫秒级完成而 WebView2 初始化需数百毫秒，首版窗口永远停在 loading。改为 `iNavState` 导航状态机（LOADING/URL/FAIL/SHUTDOWN）：主线程只写"缓冲 + Interlocked 状态"（全屏障保写序），UI 线程创建完成后自查后到者生效；SHUTDOWN 状态同时解决 Close 时窗口仍在初始化的收口。
3. **失败页驻留**：装配失败原实现进程立即退出、错误页一闪而过。`XS_AppWindowWaitClosed`（上限 10 分钟）等用户读完错误关窗再走停机链。
4. xs 相对路径按 exe 目录归一化是既有语义（非 bug），app 测试配置须用绝对路径或相对 exe 目录书写。

与 §12 估算的差异：窗口模块约 700 行（估 400）——增量为兜底安装参考实现的迁入（~230 行）与状态机；其余条目符合。

## 16. M2/M3 实施记录（2026-09-16）

**M2（端口 0 自动分配 + 粘滞）**：

- `XS_ServerInfo` 尾部追加 `uint16 PortBound`（ABI 只增纪律）；`config.h` 放行 `port: 0`（仅 app，错误信息提示 `or 0 (auto)`）；`http.h` 在 `XS_HttpStartEx` 入口做粘滞替换、plain listener attach 成功后 `xrtNetListenerLocal` 回读。
- **实现精化（相对 §8 的 PortBound-only 方案）**：自动分配时同时**回写 `pServer->Port`**——端点标识（reload 槽位转交判断）、脚本视图（`pReq->server->Port`）、前端 URL、日志四处天然一致，无需各处映射；PortBound 保留为显式镜像。回写发生在装配/候选构建线程（发布前），不违反 revision 发布后不可变约定。
- 粘滞表：文件作用域静态数组（≤16 服务，按名记录，超额退化为不粘滞），写入仅装配/候选构建路径（串行），无需锁。
- 实测（样例脚本 http_main.c + port 0 + reload-all 全链路）：OS 分配 53455 → 日志/窗口 URL 均实际端口；`/json` 显示脚本看到 `port:53455`；`/reload-all` succeeded 后**同端口**继续服务、`/json` 端口不变——端点未漂移，已开窗口不断线。

**M3（测试钩子 + 冒烟）**：

- `XS_APP_AUTOCLOSE_MS=N`：N 毫秒后独立 sleep 线程对首窗投递 WM_CLOSE（与用户关窗同路径；主线程无消息泵，SetTimer 回调不可用）。实测 4s 自动关窗 → 完整优雅停机 → exit 0。
- `XS_WV2_FORCE_MISSING=1`：模拟 Runtime 缺失。背景：临时改 HKLM 注册表 pv 需要 UIPI 提权（会话无管理员），环境变量钩子对 CI 更友好。实测：`install_fallback=headless` 下打印降级日志、服务照常（port 0 同时工作）、无弹窗。
- 其余：所有窗口均记录 `hWnd`（原仅 hide 分支），PostMessage 关窗路径对 stop/hide 通用。

**收官状态**：M0~M3 全部完成并实测。all 变体 14 扩展 6,492,160 B，门禁 A PASS。遗留（不阻塞）：reload 矩阵正式用例进 `tools/reload_matrix_test.py`（本轮已手工验证等价链路）、`make_release.py` 收编 xsw.exe 发布物、`docs/可选扩展库.md` 补 webview 条目、Linux 侧门禁 B 实机验证。

## 17. 开发质量审计（2026-09-16，M0-M3 交付后）

审计发现并已修复四项（修复后 autoclose E2E 复测绿）：

1. **WaitClosed 死等**：失败页驻留期间 Ctrl-C 无法打断主线程（Sleep 循环不查 `g_XS_Stop`）——补停机标志退出条件。
2. **devtools 布尔配置静默失效**：JSON `true/false` 为 XVALUE_BOOL，精确型 `xrtValueGetInt` 失败回默认——`app_cfg_bool` 改走 `xrtValueGetBool`。
3. **窗口数超上限（8）静默截断**——补 `extra run headless` 日志。
4. **`.gitignore` 漏 `release/xsw.exe`/`xsw.log`**——构建产物会污染 git status。

核查通过项：自有代码 `-Wall -Wextra` 零警告（警告均来自 unity 头在第二 TU 的 statics 副本，与 config.h 同款仓库既有模式，扩展源按惯例不带 -Wall）；缩进/命名/注释与仓库一致；`g_XS_Stop` 跨 TU 类型严格对齐（sig_atomic_t）；hide 子类化的 USERDATA/原_proc 赋值窗口期在同线程无泵代码段内无竞态；单实例互斥兼容 abandoned；`UnregisterWaitEx(INVALID_HANDLE_VALUE)` 先于窗口表清零，唤起回调无竞态；错误页缓冲边界经查；M0 的 LinkProbe 已被真实调用链取代（无死码消除回退风险）。

已知开放项（非阻塞）：`xsWebviewImplVersion` 与 C 侧硬编码 1200 轻微重复（Linux headless 无该符号，注释已说明）；dispatch 回调函数指针转型 ABI 同形、实践安全（严格语言标准视角为 UB），webview 升版时需复验；§16 四项遗留不变。

## 18. Linux 实机门禁 B 验证记录（2026-09-17）

两套真实 Linux 环境完成验证（工作树打包上传构建，验证后已清理远端）：

| 环境 | 结果 |
|---|---|
| WSL2 Ubuntu 26.04 / gcc 15.2 | `build.sh all`（14 扩展）成功；门禁 B PASS（3 动态依赖）；headless 冒烟：port 0 → 33127、`/text`（TCC 脚本）与静态 `/` 均 200、降级日志正确、SIGTERM 完整优雅停机 |
| 显卡服务器 Ubuntu 22.04 / gcc 11.4 | 同上全绿：port 0 → 39763、GET / 200、门禁 B PASS、SIGTERM 优雅停机至 `bye` |

**实机首跑即抓到真缺陷**（dry-run 模拟未暴露——M0 桩是纯 C，M1 重写后才引入 Windows 头）：

1. `src/webview/xs_app_window.c` 缺 `platforms: ["windows"]` 过滤——Linux 编译因 `windows.h` 直接失败。已修（extensions.json）。
2. 连带：窗口模块被过滤后 Linux 无 `xsWebviewAvailable/Version` 定义（TCC 契约符号，main.o 引用）。已修——appnode.h 的 headless 分支内提供两符号定义（与窗口分支互斥编译，不冲突）。

修复后 Windows 侧回归：all 变体 0 错误、门禁 A PASS、autoclose E2E exit 0。

遗留更新：§16 遗留 #1（Linux 实机门禁 B）**关闭**；musl 变体仍未覆盖（两环境均无 musl-gcc，`musl-cc-wrapper.sh` 需自备工具链），保持遗留。远程验证操作要点：wsl.exe 的参数引号会被 Git Bash 多层拆坏（复杂命令写脚本文件上传执行）；MSYS 会把 `/mnt/...` 改写为 Git 安装路径（外层加引号规避）。

## 19. 已知问题清零（2026-09-17）

§17 清单的 11 项遗留全部处理完毕：

| # | 项 | 处置 |
|---|---|---|
| 2 | reload 改 window 静默忽略 | `appnode.h` 新增 `XS_AppWindowDiffNote`（Custom.window 序列化对比，advisory 不拒绝），挂在 reload.h 两个 handoff 判定点；矩阵用例断言提示日志出现 |
| 3 | make_release 不产出 xsw | windows 变体 `extra_binaries` 收编 xsw.exe，实测打包在案 |
| 4 | 端口 0 无正式回归用例 | `reload_matrix_test.py` 新增 `port0_case`（XS_WV2_FORCE_MISSING 强制 headless、日志取端口、/json 断言脚本端口视图、reload-all 后同端口、window 变更提示断言）；实测 `PORT0 STICKY PASS (port 58315)` |
| 5 | autoclose 只关首窗 | 逐窗 WM_CLOSE/terminate，覆盖多窗口引用计数 |
| 6 | loading/错误页无视觉确认 | 实机截图逐字核对：目标页（白底 /text 内容）与错误页（#20252b 深底、“启动失败”红标题、原始错误消息转义正确、页脚文案、失败页驻留）全部符合设计 |
| 7 | 版本双处维护 | `xs_webview.h` 定义唯一 `XS_WEBVIEW_VERSION_NUMBER`，`webview_impl.cpp` static_assert 与 vendored 头版本宏互锁——升版忘同步直接编译失败 |
| 8 | dispatch 转型 UB | 核实 `typedef void* webview_t`：两签名类型全同，转型移除，零 UB |
| 9 | 容量上限 | 窗口 8→32、粘滞表 16→32 |
| 10 | ask 无人值守阻塞 | `可选扩展库.md` webview 专节明示“无人值守请配 silent/headless” |
| 11 | 前台锁降级 | SetForegroundWindow 失败时 FlashWindowEx 闪烁任务栏 |
| 12 | 可选扩展库.md 缺条目 | 使用表 + webview 专节（app 节点/端口0/xsw/三态兜底/TCC 面/测试钩子） |

修复后全量回归：Windows `all` 0 错误、门禁 A PASS、`RELOAD MATRIX PASS` + `PORT0 STICKY PASS`、autoclose E2E exit 0；WSL 重建 + 门禁 B PASS；`make_release.py` 实跑产出三平台 zip（xsw 已在 windows 包内，manifest 同步写入站点 download 目录——注意该实跑以当前工作树为源刷新了 wwwroot 下载产物）。至此 webview 扩展无已知未修复问题；musl 变体回归仍待具备 musl 工具链的环境（唯一环境依赖项）。
