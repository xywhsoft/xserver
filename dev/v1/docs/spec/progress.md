# 文档编写进度追踪

## 状态定义

| 状态 | 含义 |
| --- | --- |
| `planned` | 已确认要写，但还没开始写正式文档 |
| `drafting` | 正在写 |
| `review` | 已有初稿，等待核对 |
| `done` | 已完成并可作为当前阶段正式文档 |
| `blocked` | 当前不适合写，或依赖实现尚未稳定 |
| `legacy` | 旧文档，需迁移或归档 |
| `pending_translation` | 中文已确认，等待补英文版本 |

## 当前阶段范围

当前只整理 spec，不创建正式文档。

因此新文档统一按“中文先行、英文预留”方式标记：

- 中文状态：`planned`
- 英文状态：`planned`

## 目录级计划

| 目录 | 作用 | 当前状态 |
| --- | --- | --- |
| `docs/api` | 接口与函数参考 | `planned` |
| `docs/tutorials` | 上手教程 | `planned` |
| `docs/examples` | 示例索引与说明 | `planned` |
| `docs/manual` | 用户手册 | `planned` |
| `docs/architecture` | 架构与设计落地说明 | `planned` |
| `docs/spec` | 文档规划与追踪 | `done` |

## 文件级计划

| 文档基名 | 分类 | 中文文件 | 英文文件 | 优先级 | 中文状态 | 英文状态 | 说明 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `overview` | 用户手册 | `docs/manual/overview.md` | `docs/manual/overview.en.md` | P0 | `planned` | `planned` | 产品定位、能力范围、当前阶段说明 |
| `configuration` | 用户手册 | `docs/manual/configuration.md` | `docs/manual/configuration.en.md` | P0 | `planned` | `planned` | 配置模型、字段表、示例、限制 |
| `service-types` | 用户手册 | `docs/manual/service-types.md` | `docs/manual/service-types.en.md` | P0 | `planned` | `planned` | `http/ws/tcp/udp/xtp/custom` 当前支持情况 |
| `build-and-run` | 用户手册 | `docs/manual/build-and-run.md` | `docs/manual/build-and-run.en.md` | P0 | `planned` | `planned` | 构建、`--check`、运行、目录约定 |
| `operations` | 用户手册 | `docs/manual/operations.md` | `docs/manual/operations.en.md` | P0 | `planned` | `planned` | 管理面、指标、重载、排障入口 |
| `script-host` | API | `docs/api/script-host.md` | `docs/api/script-host.en.md` | P0 | `planned` | `planned` | 稳定宿主 API 分组说明 |
| `manage-http` | API | `docs/api/manage-http.md` | `docs/api/manage-http.en.md` | P0 | `planned` | `planned` | `__xs/*` 管理接口列表与约束 |
| `bus` | API | `docs/api/bus.md` | `docs/api/bus.en.md` | P0 | `planned` | `planned` | `xsData* / xsMsg* / bus` 管理接口 |
| `callbacks` | API | `docs/api/callbacks.md` | `docs/api/callbacks.en.md` | P1 | `planned` | `planned` | HTTP / WS / TCP / UDP / XTP / Custom 回调模型 |
| `quick-start` | 教程 | `docs/tutorials/quick-start.md` | `docs/tutorials/quick-start.en.md` | P0 | `planned` | `planned` | 最短路径跑通 HTTP 服务 |
| `first-http-script` | 教程 | `docs/tutorials/first-http-script.md` | `docs/tutorials/first-http-script.en.md` | P0 | `planned` | `planned` | 第一个 `script-c` HTTP 服务 |
| `first-websocket-service` | 教程 | `docs/tutorials/first-websocket-service.md` | `docs/tutorials/first-websocket-service.en.md` | P1 | `planned` | `planned` | 第一个 WS/WSS 服务 |
| `first-xtp-service` | 教程 | `docs/tutorials/first-xtp-service.md` | `docs/tutorials/first-xtp-service.en.md` | P1 | `planned` | `planned` | 第一个 XTP/XTPS 服务 |
| `release-index` | 范例 | `docs/examples/release-index.md` | `docs/examples/release-index.en.md` | P0 | `planned` | `planned` | `release/` 示例配置与脚本索引 |
| `http` | 范例 | `docs/examples/http.md` | `docs/examples/http.en.md` | P1 | `planned` | `planned` | HTTP 示例说明 |
| `ws` | 范例 | `docs/examples/ws.md` | `docs/examples/ws.en.md` | P1 | `planned` | `planned` | WebSocket 示例说明 |
| `udp` | 范例 | `docs/examples/udp.md` | `docs/examples/udp.en.md` | P1 | `planned` | `planned` | UDP 示例说明 |
| `xtp` | 范例 | `docs/examples/xtp.md` | `docs/examples/xtp.en.md` | P1 | `planned` | `planned` | XTP 示例说明 |
| `custom` | 范例 | `docs/examples/custom.md` | `docs/examples/custom.en.md` | P1 | `planned` | `planned` | Custom / TCP 示例说明 |
| `runtime-model` | 架构 | `docs/architecture/runtime-model.md` | `docs/architecture/runtime-model.en.md` | P1 | `planned` | `planned` | Runtime / Server / Host 模型 |
| `protocol-model` | 架构 | `docs/architecture/protocol-model.md` | `docs/architecture/protocol-model.en.md` | P1 | `planned` | `planned` | 协议分层与对象边界 |
| `lifecycle-and-reload` | 架构 | `docs/architecture/lifecycle-and-reload.md` | `docs/architecture/lifecycle-and-reload.en.md` | P1 | `planned` | `planned` | 生命周期、脚本重载、配置重载 |
| `bus-and-shared-data` | 架构 | `docs/architecture/bus-and-shared-data.md` | `docs/architecture/bus-and-shared-data.en.md` | P1 | `planned` | `planned` | bus、共享数据、命名空间、TTL |

## 当前不建议直接写死为“已完成”的主题

| 主题 | 状态 | 原因 |
| --- | --- | --- |
| `force / draining` 完整行为语义 | `blocked` | 当前参数已进入接口，但行为未完全闭环 |
| 显式生命周期状态机 | `blocked` | 当前有调用顺序，但没有完整状态机对外语义 |
| 旧配置兼容层 | `blocked` | 当前实现已明显偏向新字段模型 |

## 旧文档状态

| 文件 | 当前状态 | 处理建议 |
| --- | --- | --- |
| `docs/配置说明.md` | `legacy` | 拆分迁移后归档 |
| `docs/服务类型.md` | `legacy` | 拆分迁移后归档 |
| `docs/热加载.md` | `legacy` | 拆分迁移后归档 |
| 现有设计草案文件 | `legacy` | 保留为设计依据，不直接当用户文档 |

## 下一步建议

后续正式开始写文档时，建议按这个顺序推进：

1. 先写 `docs/manual/configuration.md`
2. 再写 `docs/manual/build-and-run.md`
3. 再写 `docs/api/script-host.md`
4. 再写 `docs/api/manage-http.md`
5. 再写 `docs/examples/release-index.md`
6. 再写 `docs/tutorials/quick-start.md`
7. 每篇中文审阅通过后，再把对应英文文件状态改为 `drafting`
