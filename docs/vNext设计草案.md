# XServer vNext 当前说明

本文档原先是开发过程中的长篇设计草稿，已经归档到：

- `dev/2026-04-21_vnext_design_draft_backup.md`

原草稿中包含大量已经过时的治理、限流、内置管理面、dashboard、`__xs/bus/*`、异步 XTP pending request 等历史计划。当前版本不再以原草稿作为任务清单。

## 当前冻结边界

当前 production `xs` 的定位是：

- 轻量级网络框架和脚本宿主
- 支持 HTTP / WS / TCP / UDP / XTP / custom 协议
- 支持 Bus 运行时通信能力
- 支持配置加载、脚本加载、热加载和异常连接清理
- 不内置固定 `__xs/*` HTTP 管理应用
- 不提供限流、治理看板、在线策略编辑等上层应用能力

当前 `xsdbg` 的定位是：

- 协议调试
- 错误排查
- 内存调试
- reload / check_config 诊断

## 当前权威文档

后续判断任务状态时，以这些文件为准：

- `README.md`
- `README.en.md`
- `docs/稳定API.md`
- `docs/发布检查清单.md`
- `docs/配置说明.md`
- `docs/热加载.md`
- `docs/运行与稳定补记.md`

## 当前不再作为收口任务的能力

以下能力不是 production `xs` 当前交付阻塞项：

- 统一限流策略
- 治理看板
- 在线策略编辑
- 内置 `dashboard`
- 内置 `__xs/bus/*`
- 异步 XTP pending request
- 持久 XTP client 对象体系

如应用层需要这些能力，应基于 xs 提供的底层网络、Bus、reload/check API 自行封装。
