# xserver 稳定边界

本文档冻结的是当前 `production xs` 与 `xsdbg` 的职责边界。

目标只有两个：

- 让 `production xs` 保持“薄宿主、无内置应用”的定位
- 让 `xsdbg` 保留一套可持续演进的内置调试应用



## 1. production xs

当前 production `xs` 的稳定边界是：

- 不内置固定的 `__xs/*` 管理应用
- 不预留 `__xs/*` 路径，相关请求直接交给应用层 Host 处理
- `Bus / reload / check_config` 继续保留为运行时能力，供 C API、脚本 API 或应用层自定义路由调用
- 不再承诺任何固定的 `status / health / reload / check_config` HTTP 返回字段

这意味着：

- `GET /__xs/status_json`
- `GET /__xs/bus/status`
- `GET /__xs/dashboard`

在 production `xs` 下是否存在、返回什么，都由应用层决定，不再是 `xs` 内建 HTTP 契约。

稳定 smoke 当前只用仓库自带的 demo 配置确认一件事：

- 上述 `__xs/*` 路径已经回到应用层，而不是继续被 `xs` 内建管理面拦截

这属于框架边界回归，不属于 production `xs` 的公共 HTTP API 承诺。



## 2. xsdbg

`xsdbg` 当前仍保留内置调试应用。

现阶段保留的调试入口包括：

- `__xs/status*`
- `__xs/health*`
- `__xs/reload*`
- `__xs/check_config*`
- `__xs/*_metrics*`
- `__xs/*_clear`

同时：

- `__xs/dashboard*` 已不再是内置调试页
- `__xs/bus/*` 已不再是内置 HTTP 管理面
- 除了上面列出的调试入口，其余 `__xs/*` 也不再由 xsdbg 预留
- 如果这些路径存在，也应理解为应用层自己定义的路由

这组入口的定位是：

- 协议调试
- 错误排查
- 内存调试
- reload / check_config 诊断

这组入口不是 production `xs` 的契约，也不应被应用项目当成业务控制面依赖。



## 3. 当前冻结项

当前真正冻结的内容只有下面两类：

1. production `xs` 的边界
- 无内置 `__xs/*` 应用
- `__xs/*` 回到应用层
- Bus / reload / check_config 保留为运行时 API，而不是固定 URI

2. xsdbg 的最小调试面
- 上述 `__xs/*` 调试入口继续可用
- `dashboard / __xs/bus/*` 不再属于 xsdbg 内建调试应用
- 现有稳定 smoke 会继续锁定这批内建调试入口的最小方法语义、`Content-Type` 和安全头

如果未来需要新增任何内建 `__xs/*` 入口：

- 默认只能先进 `xsdbg`
- 不应直接进入 production `xs`



## 4. 安全头

`xsdbg` 内置调试应用当前继续冻结以下安全头：

- `Cache-Control: no-store`
- `X-Frame-Options: DENY`
- `Referrer-Policy: no-referrer`
- `X-Content-Type-Options: nosniff`

production `xs` 下，是否返回这些头，取决于应用层自己的路由实现；这不再属于框架内建管理面的冻结范围。



## 5. 验证入口

- `test_stable.bat`
- `test_stable.sh`
- `docs/发布检查清单.md`

如果需要看当前 smoke 具体锁定了哪些 demo 行为，请直接看：

- `tools/xs_stable_smoke.ps1`
- `tools/xs_stable_smoke.sh`
