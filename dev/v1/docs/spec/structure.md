# 文档目录结构规划

## 目标

将 `docs` 重组为更适合持续维护的结构，按“读者场景 + 内容类型”拆分。

本轮只定义结构，不创建正式文档内容文件。

## 文件命名规则

正式文档文件名统一使用英文命名。

建议规则：

- 使用小写英文
- 单词之间使用连字符 `-`
- 不在文件名里使用中文
- 不在文件名里使用内部代号

每篇正式文档固定保留两份：

- `xxx.md`：中文版本
- `xxx.en.md`：英文版本

当前执行顺序：

1. 先写 `xxx.md`
2. 中文审阅通过
3. 再补 `xxx.en.md`

## 结构示意

下面示意的是“正式落地后”的推荐形态。

```text
docs/
├── api/
│   ├── script-host.md
│   └── script-host.en.md
├── tutorials/
│   ├── quick-start.md
│   └── quick-start.en.md
├── examples/
│   ├── release-index.md
│   └── release-index.en.md
├── manual/
│   ├── configuration.md
│   └── configuration.en.md
├── architecture/
│   ├── runtime-model.md
│   └── runtime-model.en.md
├── spec/
├── 配置说明.md
├── 服务类型.md
├── 热加载.md
└── 现有设计草案文件
```

## 建议结构

```text
docs/
├── api/
├── tutorials/
├── examples/
├── manual/
├── architecture/
├── spec/
├── 配置说明.md
├── 服务类型.md
├── 热加载.md
└── 现有设计草案文件
```

## 分类说明

### 1. `api/`

面向：

- 写脚本的人
- 写管理工具的人
- 需要查函数、接口、参数的人

建议放：

- 脚本宿主 API
- 管理面 HTTP API
- bus 共享数据与消息 API
- 协议侧脚本回调与对象模型

### 2. `tutorials/`

面向：

- 第一次上手的人
- 想快速跑通单个场景的人

建议放：

- 快速开始
- 第一个 HTTP 脚本服务
- 第一个 WebSocket 服务
- 第一个 XTP 服务
- 调试与热重载入门

### 3. `examples/`

面向：

- 想直接找现成配置和脚本的人
- 需要把 `release/` 示例映射到文档的人

建议放：

- `release/` 示例配置索引
- 示例脚本索引
- 每个示例适用场景与入口文件说明

### 4. `manual/`

面向：

- 普通使用者
- 运维和集成人员
- 需要系统性说明的人

建议放：

- 产品概览
- 配置模型
- 服务类型说明
- 构建与运行
- 管理面与运行维护

### 5. `architecture/`

这是补充维度。

原因：

- 当前版本的核心价值不只是“怎么用”，还有“结构为什么这么拆”
- `Runtime / Server / Host`
- `host-aware / server-only`
- 生命周期、热加载、总线等内容更适合单独归类

建议放：

- 运行时模型
- 协议分层
- 生命周期与热加载
- 总线与共享数据模型

### 6. `spec/`

面向：

- 文档维护者
- 后续继续补文档的人

建议放：

- 结构规划
- 编写约定
- 文件清单
- 进度追踪

## 首批正式文档建议清单

以下是建议的第一批目标文件。

### `manual/`

- `manual/overview(.md / .en.md)`
- `manual/configuration(.md / .en.md)`
- `manual/service-types(.md / .en.md)`
- `manual/build-and-run(.md / .en.md)`
- `manual/operations(.md / .en.md)`

### `api/`

- `api/script-host(.md / .en.md)`
- `api/manage-http(.md / .en.md)`
- `api/bus(.md / .en.md)`
- `api/callbacks(.md / .en.md)`

### `tutorials/`

- `tutorials/quick-start(.md / .en.md)`
- `tutorials/first-http-script(.md / .en.md)`
- `tutorials/first-websocket-service(.md / .en.md)`
- `tutorials/first-xtp-service(.md / .en.md)`

### `examples/`

- `examples/release-index(.md / .en.md)`
- `examples/http(.md / .en.md)`
- `examples/ws(.md / .en.md)`
- `examples/udp(.md / .en.md)`
- `examples/xtp(.md / .en.md)`
- `examples/custom(.md / .en.md)`

### `architecture/`

- `architecture/runtime-model(.md / .en.md)`
- `architecture/protocol-model(.md / .en.md)`
- `architecture/lifecycle-and-reload(.md / .en.md)`
- `architecture/bus-and-shared-data(.md / .en.md)`

## 旧文档迁移建议

### `docs/配置说明.md`

建议拆分到：

- `manual/configuration.md`
- `manual/build-and-run.md`
- `tutorials/quick-start.md`

原因：

- 旧文件混合了配置、启动、示例和历史字段
- 还包含与当前实现不一致的内容

### `docs/服务类型.md`

建议拆分到：

- `manual/service-types.md`
- `api/callbacks.md`
- `examples/*.md`

原因：

- 旧文件把服务说明、回调签名、最佳实践混在一起
- 还保留了当前版本不应继续出现在正式文档中的内容

### `docs/热加载.md`

建议拆分到：

- `architecture/lifecycle-and-reload.md`
- `api/manage-http.md`
- `manual/operations.md`

原因：

- 当前主线已经从旧热加载函数转向新的管理入口和配置热加载流程

### 现有设计草案文件

建议保留为设计文档，不直接删除。

后续处理：

- 继续作为设计依据
- 将已落地部分迁移到正式文档
- 将未落地部分保留在架构文档的“限制 / 后续计划”中
