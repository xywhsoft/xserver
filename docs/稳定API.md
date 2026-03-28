# xserver 稳定 API 基线

本文档用于冻结当前生产版 `xs` 与调试版 `xsdbg` 的最小管理面边界。

目标只有两个：

- 让生产版 `xs` 保持接近 `old/legacy` 的薄宿主定位
- 让 `xadmin` 和后续项目有一组可以依赖的稳定管理 API











## 1. 生产版 xs 保留面

当前生产版 `xs` 固定保留以下内建管理接口：

- `GET /__xs/status`
- `GET /__xs/status_json`
- `GET /__xs/health`
- `GET /__xs/health_json`
- `GET /__xs/reload`
- `GET|POST /__xs/reload_json`
- `GET /__xs/reload_config`
- `GET|POST /__xs/reload_config_json`
- `GET /__xs/reload_status`
- `GET /__xs/reload_status_json`
- `GET /__xs/check_config`
- `GET /__xs/check_config_json`

这些接口属于生产版稳定面，后续只能做兼容性修复，不应随意扩字段语义、方法语义或错误语义。



## 2. xsdbg 专属面

以下接口明确属于 `xsdbg`，生产版 `xs` 不承诺提供：

- `GET /__xs/dashboard`
- `GET /__xs/dashboard_json`
- `GET /__xs/bus/*`
- `GET /__xs/reload_clear`
- `GET /__xs/reload_reset`
- `GET /__xs/check_config_clear`
- `GET /__xs/http_metrics`
- `GET /__xs/http_metrics_json`
- `GET /__xs/http_metrics_clear`
- `GET /__xs/ws_metrics`
- `GET /__xs/ws_metrics_json`
- `GET /__xs/ws_metrics_clear`
- `GET /__xs/xtp_metrics`
- `GET /__xs/xtp_metrics_json`
- `GET /__xs/xtp_metrics_clear`
- `GET /__xs/udp_metrics`
- `GET /__xs/udp_metrics_json`
- `GET /__xs/udp_metrics_clear`
- `GET /__xs/custom_metrics`
- `GET /__xs/custom_metrics_json`
- `GET /__xs/custom_metrics_clear`

生产版 `xs` 对这组接口固定返回 `403`，并保留明确的 xsdbg-only 提示文案。
首页 `release/wwwroot/index.html` 在 production `xs` 下也会对这组入口做自动降级：保留核心状态展示，但不再把 `dashboard / bus / *_metrics* / clear-reset` 这类 xsdbg-only 入口以可点击状态暴露给用户。



## 3. 方法契约

当前冻结的方法语义如下：

- `HEAD /__xs/status_json = 200`
- `HEAD /__xs/health_json = 200`
- `HEAD /__xs/reload_status_json = 200`
- `HEAD /__xs/check_config_json = 405`
- `HEAD /__xs/reload_json = 405`
- `HEAD /__xs/reload_config_json = 405`
- `POST /__xs/reload_json = 200`
- `POST /__xs/reload_config = 200`
- `POST /__xs/reload_config_json = 200`

`Allow` 头也属于冻结范围：

- `HEAD /__xs/check_config_json` 保持 `Allow: GET`
- `HEAD /__xs/reload_json` 保持 `Allow: GET, POST`
- `HEAD /__xs/reload_config_json` 保持 `Allow: GET, POST`
- `xsdbg` 下的 `HEAD /__xs/reload_clear`、`/__xs/reload_reset`、`/__xs/check_config_clear` 保持 `Allow: GET`



## 4. 响应语义

当前冻结的响应语义包括：

- 核心 `_json` 管理面保持 `application/json`
- 核心文本管理面保持 `text/plain`
- 未知保留路径 `GET /__xs/unknown`、`GET /__xs/unknown_json` 保持 `404`
- 已知 xsdbg-only 路径在生产版 `xs` 下保持 `403`
- 生产版 `xs` 对 xsdbg-only 路径继续返回明确的说明文案，而不是泛化的 `forbidden`



## 5. 安全头

当前冻结的保留管理面安全头包括：

- `Cache-Control: no-store`
- `X-Frame-Options: DENY`
- `Referrer-Policy: no-referrer`
- `X-Content-Type-Options: nosniff`

这组头部目前已经被稳定 smoke 锁定在：

- 核心保留管理面
- `dashboard_json / bus/status`
- 生产版 `xs` 下禁用的 debug-only 面



## 6. 启动与重载语义

当前冻结的运行语义包括：

- 相对配置路径优先按可执行文件目录解析
- 从仓库根目录启动 `release/xs(.exe) xs_manage_test.json` 仍然可用
- `reload_json` / `reload_config_json` 走异步排队语义，最终要回到 `reload_status_json.busy=false`
- 配置重载后 `/json` 业务链仍然保持可用



## 7. 保留业务能力

为了保证新版本 `xs` 具备旧版可用性之外的最小增强能力，当前稳定基线还额外锁住：

- HTTP `/json` 路由可用
- XTP `demo.callself`
- XTP `demo.callrequeststatus`
- WebSocket echo
- custom echo



## 8. 验证入口

当前稳定基线的发布入口以以下脚本为准：

- `test_stable.bat`
- `test_stable.sh`

底层 smoke 实现入口是：

- `tools/xs_stable_smoke.ps1`
- `tools/xs_stable_smoke.sh`

发布入口会先 build 当前源码，再执行稳定 smoke；因此它们用于判断“当前源码树是否仍然满足稳定 API 基线”。



## 9. 相关文档

- 发布检查：`docs/发布检查清单.md`
- legacy 对照：`docs/legacy对照.md`
- 运行补记：`docs/运行与稳定补记.md`

## 10. 追加冻结项

### 首页静态审查

- `tools/xs_stable_smoke.ps1`
- `tools/xs_stable_smoke.sh`

现在还会在运行时 smoke 之前，先静态检查 `release/wwwroot/index.html`：
- `dashboard / dashboard_json / *_metrics_json / *_metrics_clear / reload_reset / check_config_clear` 必须继续挂在 `data-debug-manage="true"` 下
- `bus/status` 必须继续挂在 `data-bus-manage="true"` 下
- 目标是避免 production `xs` 首页重新暴露新的 xsdbg-only `403` 调试入口

### reload 核心管理面

- `GET /__xs/reload` 保持 `200 text/plain`
- `GET /__xs/reload_config` 保持 `200 text/plain`
- `GET /__xs/reload_json` 保持 `200 application/json`
- `GET /__xs/reload_config_json` 保持 `200 application/json`
- `HEAD /__xs/reload = 405`，并保持 `Allow: GET, POST`
- `HEAD /__xs/reload_config = 405`，并保持 `Allow: GET, POST`
- 以上四条路径的 `GET` 响应继续锁定：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`

### 核心文本管理面 HEAD 契约

- `HEAD /__xs/status = 200`
- `HEAD /__xs/health = 200`
- `HEAD /__xs/reload_status = 200`
- `HEAD /__xs/check_config = 405`，并保持 `Allow: GET`
- 核心 `HEAD 200 / 405` 路径继续匹配各自的 `Content-Type`
- 核心 `HEAD 200 / 405` 路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`

### 核心只读管理面 POST 拒绝契约

- `POST /__xs/status`、`POST /__xs/status_json` 保持 `405`，并继续匹配各自的 `Allow` 与 `Content-Type`
- `POST /__xs/health`、`POST /__xs/health_json` 保持 `405`，并继续匹配各自的 `Allow` 与 `Content-Type`
- `POST /__xs/reload_status`、`POST /__xs/reload_status_json` 保持 `405`，并继续匹配各自的 `Allow` 与 `Content-Type`
- `POST /__xs/check_config`、`POST /__xs/check_config_json` 保持 `405`，并继续匹配各自的 `Allow` 与 `Content-Type`
- 上述 8 条 `POST 405` 路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`
- 这层 freeze 当前锁的是协商语义，不额外锁 body 文案

### unknown 管理路径与 xsdbg-only clear/reset 的拒绝契约

- `GET /__xs/unknown = 404 text/plain`
- `GET /__xs/unknown_json = 404 application/json`
- 上述两条 unknown 管理路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`
- `xsdbg` 下 `HEAD /__xs/reload_clear`、`HEAD /__xs/reload_reset`、`HEAD /__xs/check_config_clear` 保持 `405`
- 上述 3 条 `HEAD 405` 路径继续匹配 `Allow: GET`、`Content-Type: text/plain`，并继续锁定相同的安全头

### xsdbg-only clear 成功路径

- `xsdbg` 下 `GET /__xs/http_metrics_clear`、`/ws_metrics_clear`、`/xtp_metrics_clear`、`/udp_metrics_clear`、`/custom_metrics_clear` 保持 `200 text/plain`
- 上述 5 条 `*_metrics_clear` 成功路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`
- `xsdbg` 下 `GET /__xs/reload_clear`、`/reload_reset`、`/check_config_clear` 保持 `200 text/plain`
- 上述 3 条 clear/reset 成功路径也继续锁定相同的安全头

### xsdbg-only bus 成功路径

- `xsdbg` 下 `GET /__xs/bus/status`、`/bus/namespaces`、`/bus/registry`、`/bus/limits`、`/bus/send`、`/bus/reset` 保持 `200 application/json`
- 上述 6 条 bus 成功路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`

### xsdbg-only metrics 成功路径

- `xsdbg` 下 `GET /__xs/http_metrics`、`/ws_metrics`、`/xtp_metrics`、`/udp_metrics`、`/custom_metrics` 保持 `200 text/plain`
- 上述 5 条 text metrics 成功路径继续锁定最小 body 锚点：`http_req_count=`、`ws_open_count=`、`xtp_open_count=`、`udp_recv_count=`、`custom_open_count=`
- `xsdbg` 下 `GET /__xs/http_metrics_json`、`/ws_metrics_json`、`/xtp_metrics_json`、`/udp_metrics_json`、`/custom_metrics_json` 保持 `200 application/json`
- 上述 5 条 json metrics 成功路径继续锁定最小 body 锚点：`"http_req_count"`、`"ws_open_count"`、`"xtp_open_count"`、`"udp_recv_count"`、`"custom_open_count"`
- 上述 10 条 metrics 成功路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`
- `xsdbg` 下 `HEAD /__xs/http_metrics`、`/ws_metrics`、`/xtp_metrics`、`/udp_metrics`、`/custom_metrics` 保持 `200 text/plain`
- `xsdbg` 下 `HEAD /__xs/http_metrics_json`、`/ws_metrics_json`、`/xtp_metrics_json`、`/udp_metrics_json`、`/custom_metrics_json` 保持 `200 application/json`
- 上述 10 条 metrics `HEAD` 成功路径也继续锁定相同的安全头

### xsdbg-only dashboard 成功路径

- `xsdbg` 下 `GET /__xs/dashboard` 保持 `200 text/plain`，并继续锁定最小 body 锚点 `http_req_count=`
- `xsdbg` 下 `GET /__xs/dashboard_json` 保持 `200 application/json`，并继续锁定最小 body 锚点 `"status"`
- `xsdbg` 下 `HEAD /__xs/dashboard` 保持 `200 text/plain`
- `xsdbg` 下 `HEAD /__xs/dashboard_json` 保持 `200 application/json`
- 上述 4 条 dashboard 成功路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`

### xsdbg-only bus HEAD 拒绝契约

- `xsdbg` 下 `HEAD /__xs/bus/status`、`/bus/namespaces`、`/bus/registry`、`/bus/limits`、`/bus/send`、`/bus/reset` 统一保持 `405`
- 上述 6 条 bus `HEAD` 拒绝路径继续匹配 `Allow: GET`
- 上述 6 条 bus `HEAD` 拒绝路径继续匹配 `Content-Type: application/json`
- 上述 6 条 bus `HEAD` 拒绝路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`

### xsdbg-only bus POST 拒绝契约

- `xsdbg` 下 `POST /__xs/bus/status`、`/bus/namespaces`、`/bus/registry`、`/bus/limits`、`/bus/send`、`/bus/reset` 统一保持 `405`
- 上述 6 条 bus `POST` 拒绝路径继续匹配 `Allow: GET`
- 上述 6 条 bus `POST` 拒绝路径继续匹配 `Content-Type: application/json`
- 上述 6 条 bus `POST` 拒绝路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`

### production xs bus 禁用契约

- production `xs` 下 `GET /__xs/bus/status`、`/bus/namespaces`、`/bus/registry`、`/bus/limits`、`/bus/send`、`/bus/reset` 继续保持 `403`，并继续匹配禁用文案 `bus api not included in production xs`
- production `xs` 下 `HEAD /__xs/bus/status`、`/bus/namespaces`、`/bus/registry`、`/bus/limits`、`/bus/send`、`/bus/reset` 统一保持 `403`
- production `xs` 下 `POST /__xs/bus/status`、`/bus/namespaces`、`/bus/registry`、`/bus/limits`、`/bus/send`、`/bus/reset` 统一保持 `403`，并继续匹配禁用文案 `bus api not included in production xs`
- 上述 production `xs` bus 禁用路径继续匹配 `Content-Type: application/json`
- 上述 production `xs` bus 禁用路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`

### production xs debug-only 禁用契约

- production `xs` 下 `HEAD /__xs/dashboard`、`/http_metrics`、`/ws_metrics`、`/xtp_metrics`、`/udp_metrics`、`/custom_metrics`、`/http_metrics_clear`、`/ws_metrics_clear`、`/xtp_metrics_clear`、`/udp_metrics_clear`、`/custom_metrics_clear`、`/reload_clear`、`/reload_reset`、`/check_config_clear` 统一保持 `403`
- 上述 text 路径继续匹配 `Content-Type: text/plain`
- production `xs` 下 `HEAD /__xs/dashboard_json`、`/http_metrics_json`、`/ws_metrics_json`、`/xtp_metrics_json`、`/udp_metrics_json`、`/custom_metrics_json` 统一保持 `403`
- 上述 json 路径继续匹配 `Content-Type: application/json`
- production `xs` 下对应的 `POST` 路径也统一保持 `403`，并继续匹配各自的 xsdbg-only 禁用文案
- 上述 production `xs` debug-only 禁用路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`

### xsdbg debug-only POST 方法契约

- `xsdbg` 下 `POST /__xs/dashboard`、`/http_metrics`、`/ws_metrics`、`/xtp_metrics`、`/udp_metrics`、`/custom_metrics` 统一保持 `405`
- `xsdbg` 下 `POST /__xs/dashboard_json`、`/http_metrics_json`、`/ws_metrics_json`、`/xtp_metrics_json`、`/udp_metrics_json`、`/custom_metrics_json` 统一保持 `405`
- 上述 read-only debug 入口继续匹配 `Allow: GET, HEAD`
- 其中 text 路径继续匹配 `Content-Type: text/plain`，json 路径继续匹配 `Content-Type: application/json`
- `xsdbg` 下 `POST /__xs/http_metrics_clear`、`/ws_metrics_clear`、`/xtp_metrics_clear`、`/udp_metrics_clear`、`/custom_metrics_clear`、`/reload_clear`、`/reload_reset`、`/check_config_clear` 统一保持 `405`
- 上述 get-only debug 入口继续匹配 `Allow: GET` 和 `Content-Type: text/plain`
- 上述 xsdbg debug-only `POST 405` 路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`

### bus 根路径契约

- `xsdbg` 下 `GET /__xs/bus` 保持 `404 application/json`，并继续匹配 body `manage api not found`
- `xsdbg` 下 `HEAD /__xs/bus` 保持 `404 application/json`
- `xsdbg` 下 `POST /__xs/bus` 保持 `404 application/json`，并继续匹配 body `manage api not found`
- production `xs` 下 `GET /__xs/bus` 保持 `403 application/json`，并继续匹配 body `bus api not included in production xs`
- production `xs` 下 `HEAD /__xs/bus` 保持 `403 application/json`
- production `xs` 下 `POST /__xs/bus` 保持 `403 application/json`，并继续匹配 body `bus api not included in production xs`
- 上述 `__xs/bus` 根路径也继续锁定安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`

### `/__xs` 根路径与 unknown 语义

- `xs` 与 `xsdbg` 下 `GET /__xs` 统一保持 `404 text/plain`，并继续匹配 body `manage api not found`
- `xs` 与 `xsdbg` 下 `HEAD /__xs` 统一保持 `404 text/plain`
- `xs` 与 `xsdbg` 下 `POST /__xs` 统一保持 `404 text/plain`，并继续匹配 body `manage api not found`
- `xs` 与 `xsdbg` 下 `GET /__xs/unknown` 统一保持 `404 text/plain`
- `xs` 与 `xsdbg` 下 `HEAD /__xs/unknown` 统一保持 `404 text/plain`
- `xs` 与 `xsdbg` 下 `POST /__xs/unknown` 统一保持 `404 text/plain`，并继续匹配 body `manage api not found`
- `xs` 与 `xsdbg` 下 `GET /__xs/unknown_json` 统一保持 `404 application/json`
- `xs` 与 `xsdbg` 下 `HEAD /__xs/unknown_json` 统一保持 `404 application/json`
- `xs` 与 `xsdbg` 下 `POST /__xs/unknown_json` 统一保持 `404 application/json`，并继续匹配 body `manage api not found`
- 上述 `__xs` 根路径与 unknown 路径继续锁定统一安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`
- 其中 `GET /__xs/unknown` 与 `GET /__xs/unknown_json` 当前只冻结状态码、`Content-Type` 与安全头，不再冻结 body 文案；这是按现有运行时真实行为收口，避免把非稳定 body 误当成稳定 API 契约

### 仓库根目录启动语义
- 从仓库根目录启动 `release/xs(.exe) xs_manage_test.json` 与 `release/xsdbg(.exe) xs_manage_test.json` 时，`GET /__xs/status_json` 继续保持 `200`
- 上述仓库根目录启动路径下，`GET /__xs/health_json`、`GET /__xs/health`、`GET /__xs/reload_status_json`、`GET /__xs/reload_status`、`GET /__xs/check_config_json`、`GET /__xs/check_config` 继续匹配与常规启动一致的核心内建管理面契约
- 上述仓库根目录启动路径下，`GET /__xs/reload`、`GET /__xs/reload_json`、`GET /__xs/reload_config`、`GET /__xs/reload_config_json` 也继续匹配与常规启动一致的核心 reload 契约，并继续要求重载完成后能够回到 idle
- 上述仓库根目录启动路径下，`POST /__xs/reload`、`POST /__xs/reload_json`、`POST /__xs/reload_config`、`POST /__xs/reload_config_json` 也继续匹配与常规启动一致的核心 reload 触发契约，其中 text 路径继续保持 `text/plain`，json 路径继续保持 `application/json`，并继续要求触发完成后能够回到 idle
- 上述仓库根目录启动路径下，`GET /__xs/status` 也继续匹配与常规启动一致的核心内建管理面契约；`GET /__xs/status_json` 继续匹配 `application/json`，`GET /__xs/status` 继续匹配 `text/plain`
- 上述仓库根目录启动路径下，`HEAD /__xs/status_json`、`/status`、`/health_json`、`/health`、`/reload_status_json`、`/reload_status` 继续匹配与常规启动一致的 `200` 契约，以及对应的 `Content-Type` 与安全头
- 上述仓库根目录启动路径下，`POST /__xs/status_json`、`/status`、`/health_json`、`/health`、`/reload_status_json`、`/reload_status` 继续匹配与常规启动一致的 `405` 契约，以及对应的 `Allow`、`Content-Type` 与安全头
- 上述仓库根目录启动路径下，`HEAD /__xs/check_config_json`、`/check_config` 继续匹配与常规启动一致的 `405 + Allow: GET` 契约
- 上述仓库根目录启动路径下，`HEAD /__xs/reload_json`、`/reload`、`/reload_config_json`、`/reload_config` 继续匹配与常规启动一致的 `405 + Allow: GET, POST` 契约
- 上述仓库根目录启动路径下，`GET /__xs`、`HEAD /__xs`、`POST /__xs` 继续匹配与常规启动一致的 `404 text/plain` 契约
- 上述仓库根目录启动路径下，`xsdbg` 的 `GET/HEAD/POST /__xs/http_metrics`、`/__xs/http_metrics_json` 也继续匹配与常规启动一致的调试面契约，其中 text 路径继续保持 `text/plain`，json 路径继续保持 `application/json`
- 上述仓库根目录启动路径下，`xsdbg` 的 `GET/HEAD/POST /__xs/ws_metrics`、`/__xs/ws_metrics_json` 也继续匹配与常规启动一致的调试面契约，其中 text 路径继续保持 `text/plain`，json 路径继续保持 `application/json`
- 上述仓库根目录启动路径下，`xsdbg` 的 `GET/HEAD/POST /__xs/xtp_metrics`、`/__xs/xtp_metrics_json` 也继续匹配与常规启动一致的调试面契约，其中 text 路径继续保持 `text/plain`，json 路径继续保持 `application/json`
- 上述仓库根目录启动路径下，`xsdbg` 的 `GET/HEAD/POST /__xs/bus` 继续匹配与常规启动一致的 `404 application/json` 契约，其中 `GET/POST` 继续匹配 body `manage api not found`
- 上述仓库根目录启动路径下，`GET/HEAD/POST /__xs/unknown` 继续匹配与常规启动一致的 `404 text/plain` 契约
- 上述仓库根目录启动路径下，`GET/HEAD/POST /__xs/unknown_json` 继续匹配与常规启动一致的 `404 application/json` 契约
- 上述仓库根目录启动路径下，production `xs` 的 `GET/HEAD/POST /__xs/http_metrics`、`/__xs/http_metrics_json` 继续匹配与常规启动一致的 `403` xsdbg-only 禁用契约
- 上述仓库根目录启动路径下，production `xs` 的 `GET/HEAD/POST /__xs/ws_metrics`、`/__xs/ws_metrics_json` 继续匹配与常规启动一致的 `403` xsdbg-only 禁用契约
- 上述仓库根目录启动路径下，production `xs` 的 `GET/HEAD/POST /__xs/xtp_metrics`、`/__xs/xtp_metrics_json` 继续匹配与常规启动一致的 `403` xsdbg-only 禁用契约
- 上述仓库根目录启动路径下，production `xs` 的 `GET/HEAD/POST /__xs/bus` 继续匹配与常规启动一致的 `403 application/json` bus-disabled 契约，其中 `GET/POST` 继续匹配 body `bus api not included in production xs`
- 上述仓库根目录启动路径下，production `xs` 的 `GET/HEAD/POST /__xs/dashboard` 继续匹配与常规启动一致的 `403 text/plain` xsdbg-only 契约
- 上述仓库根目录启动路径下，production `xs` 的 `GET/HEAD/POST /__xs/dashboard_json` 继续匹配与常规启动一致的 `403 application/json` xsdbg-only 契约
- 上述仓库根目录启动路径下，production `xs` 的 `GET/HEAD/POST /__xs/bus/status` 继续匹配与常规启动一致的 `403 application/json` bus-disabled 契约
- 上述仓库根目录启动路径下，`xsdbg` 的 `GET /__xs/dashboard` 继续匹配与常规启动一致的 `200 text/plain` 契约，`HEAD /__xs/dashboard` 继续保持 `200`，`POST /__xs/dashboard` 继续保持 `405 + Allow: GET, HEAD`
- 上述仓库根目录启动路径下，`xsdbg` 的 `GET /__xs/dashboard_json` 继续匹配与常规启动一致的 `200 application/json` 契约，`HEAD /__xs/dashboard_json` 继续保持 `200`，`POST /__xs/dashboard_json` 继续保持 `405 + Allow: GET, HEAD`
- 上述仓库根目录启动路径下，`xsdbg` 的 `GET /__xs/bus/status` 继续匹配与常规启动一致的 `200 application/json` 契约，`HEAD/POST /__xs/bus/status` 继续保持 `405 + Allow: GET`
- 上述仓库根目录启动路径也继续锁定同一组安全头：`Cache-Control: no-store`、`X-Frame-Options: DENY`、`Referrer-Policy: no-referrer`、`X-Content-Type-Options: nosniff`
- 上述仓库根目录启动路径下，`xsdbg` 的 `GET/HEAD/POST /__xs/udp_metrics`、`/__xs/udp_metrics_json` 也继续匹配与常规启动一致的调试面契约，其中 text 路径继续保持 `text/plain`，json 路径继续保持 `application/json`
- 上述仓库根目录启动路径下，`xsdbg` 的 `GET/HEAD/POST /__xs/custom_metrics`、`/__xs/custom_metrics_json` 也继续匹配与常规启动一致的调试面契约，其中 text 路径继续保持 `text/plain`，json 路径继续保持 `application/json`
- 上述仓库根目录启动路径下，production `xs` 的 `GET/HEAD/POST /__xs/udp_metrics`、`/__xs/udp_metrics_json` 继续匹配与常规启动一致的 `403` xsdbg-only 禁用契约
- 上述仓库根目录启动路径下，production `xs` 的 `GET/HEAD/POST /__xs/custom_metrics`、`/__xs/custom_metrics_json` 继续匹配与常规启动一致的 `403` xsdbg-only 禁用契约
- 上述仓库根目录启动路径下，`xsdbg` 的 `GET/HEAD/POST /__xs/bus/namespaces`、`/__xs/bus/registry`、`/__xs/bus/limits`、`/__xs/bus/send`、`/__xs/bus/reset` 也继续匹配与常规启动一致的 bus 调试面契约
- 上述仓库根目录启动路径下，production `xs` 的 `GET/HEAD/POST /__xs/bus/namespaces`、`/__xs/bus/registry`、`/__xs/bus/limits`、`/__xs/bus/send`、`/__xs/bus/reset` 也继续匹配与常规启动一致的 `403 application/json` bus-disabled 契约
- `xsdbg` 下 `GET /__xs/http_metrics_clear`、`/ws_metrics_clear`、`/xtp_metrics_clear`、`/udp_metrics_clear`、`/custom_metrics_clear` 现在已经由 smoke 真实冻结为 `200 text/plain`，并继续匹配最小 body 锚点 `http_req_count=`、`ws_open_count=`、`xtp_open_count=`、`udp_recv_count=`、`custom_open_count=`
- 上述 5 条 `*_metrics_clear` 成功路径在仓库根目录启动下也继续匹配同样的 `200/405/405`、`text/plain` 与统一安全头契约
- 仓库根目录启动下，`xsdbg` 的 `GET/HEAD/POST /__xs/reload_clear`、`/__xs/reload_reset`、`/__xs/check_config_clear` 现在也继续匹配与常规启动一致的 `200/405/405 text/plain` 契约，并继续锁住统一安全头与最小 body 锚点 `reload_total_count=`、`check_total_count=`
- 仓库根目录启动下，production `xs` 的 `GET/HEAD/POST /__xs/http_metrics_clear`、`/__xs/ws_metrics_clear`、`/__xs/xtp_metrics_clear`、`/__xs/udp_metrics_clear`、`/__xs/custom_metrics_clear`、`/__xs/reload_clear`、`/__xs/reload_reset`、`/__xs/check_config_clear` 现在也继续匹配与常规启动一致的 `403` xsdbg-only 禁用契约
- `test_stable.*` / `xs_stable_smoke.*` 现在还会在所有 case 完成后自动验证没有残留 `xs / xsdbg` 进程；这也属于当前稳定基线的一部分
- `test_stable.*` / `xs_stable_smoke.*` 现在还会自动验证两份被跟踪的 `xrt_mem_report_auto.json` 在运行前后哈希不变，并确认 smoke helper 产物已经清理干净
