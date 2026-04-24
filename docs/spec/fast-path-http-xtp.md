# HTTP / XTP Fast Path 重构 Spec

## 目标

本 spec 用于跟踪 xrt、xs、xadmin 三个项目围绕 HTTP 与 XTP 的性能、易用性和生产安全重构。

核心目标：

- 面向公网服务时具备生产级输入安全边界
- 默认请求路径足够轻，适合 240M 级嵌入式环境
- 常见协议分发使用解析期分类结果，避免运行期反复字符串比较
- 响应侧重新信任开发者，提供接近 mongoose 的低成本写法
- 调试、统计、治理能力与 production fast path 隔离
- 不保留不良历史兼容层；必要时允许 xrt/xs/xadmin 一次性迁移

## 总原则

1. xrt 负责通用、底层、高性能协议能力。
2. xs 负责把 xrt 能力组织成轻量服务器框架。
3. xadmin 负责迁移到新 API，不在业务层继续补 xs 的设计缺口。
4. 输入侧严格，响应侧轻量。
5. production 默认路径不承担 debug/metrics/dashboard 类成本。
6. 未知方法、未知命令保留原文，但常见分发必须走整数 ID。
7. 安全检查尽量发生在解析期、配置加载期或边界入口，不在热路径重复做厚封装。

## 跟踪标记

任务统一使用下面的标记：

| 标记 | 含义 |
| --- | --- |
| `[ ]` | 未开始 |
| `[~]` | 进行中 |
| `[x]` | 已完成 |

更新规则：

- 每次开始实现任务时，将对应项从 `[ ]` 改为 `[~]`
- 每次完成并通过必要验证后，将对应项改为 `[x]`
- 如果任务被拆分，保留父任务标记，并在对应小节追加子任务

## 非目标

- 不恢复直接暴露 mongoose API。
- 不为旧 xs 响应对象写法保留兼容宏。
- 不把业务治理、在线策略编辑、详细统计重新放回 production 默认链路。
- 不要求 xadmin 保持旧代码结构。

## 权责划分

### xrt

xrt 是所有项目共享的底层 API，必须承接通用协议能力：

- HTTP 解析期 method ID
- XTP 解析期 message type / command ID 辅助能力
- 低成本 HTTP response reply/start/send/end API
- HTTP header block 安全解析与发送
- 静态文件分块发送或 sendfile 预留接口
- XTP 低成本 send/reply builder
- 输入边界限制：request line、header count、header line、path、body、XTP packet/header/body/param

### xs

xs 是面向应用的轻量服务器框架，必须尽量薄：

- production 默认走 fast path
- 脚本 API 暴露整数方法、轻量响应和 XTP 轻量回复
- 静态 Host 支持配置级 response headers
- 详细 metrics、最近请求现场、dashboard、内置管理调试留给 xsdbg 或显式开关
- 只做 Host 定位、脚本分发、静态路径安全、配置热加载等框架职责

### xadmin

xadmin 只迁移到新模型：

- RESTful 路由改用 method ID
- HTTP 响应改用 xs 轻量 reply/start/send API
- 静态资源 CORS / cache headers 改用 xs 配置
- XTP 调用与回复改用新的轻量 helper
- 删除因当前 xs 手感不佳而产生的重复封装

## HTTP 设计

### H1. method ID

跟踪：`[x]`

xrt 的 `xhttpdrequest` 增加整数方法字段：

```c
typedef enum {
    XHTTP_METHOD_UNKNOWN = 0,
    XHTTP_METHOD_GET,
    XHTTP_METHOD_HEAD,
    XHTTP_METHOD_POST,
    XHTTP_METHOD_PUT,
    XHTTP_METHOD_DELETE,
    XHTTP_METHOD_PATCH,
    XHTTP_METHOD_OPTIONS
} xhttpmethod;
```

要求：

- [x] HTTP 解析阶段一次性填充 `iMethod`
- [x] 常见方法使用手动展开匹配
- [x] xs 内部 `GET / HEAD / POST / OPTIONS` 判断不再使用 `strcmp / strcasecmp`
- [x] 脚本 API 增加 `xsReqMethodID(req)`
- `xsReqMethod(req)` 保留为日志和非标准方法兜底

验收：

- [x] 静态文件 `GET / HEAD` 判断使用整数
- [x] 管理入口 method 判断使用整数
- [x] HTTP metrics 方法计数使用整数
- [x] xadmin RESTful 分发使用整数

### H2. 轻量响应 API

跟踪：`[x]`

xrt 提供通用连接级 API：

```c
xnet_result xrtHttpdConnReply(xhttpdconn* c, uint32 status, const char* reason,
                              const char* headers, const void* body, size_t len);
xnet_result xrtHttpdConnStart(xhttpdconn* c, uint32 status, const char* reason,
                              const char* headers);
xnet_result xrtHttpdConnSend(xhttpdconn* c, const void* data, size_t len);
xnet_result xrtHttpdConnEnd(xhttpdconn* c);
```

xs 脚本层暴露：

```c
int xsHttpReply(XS_ResponseObject resp, unsigned status, const char* reason,
                const char* headers, const void* body, size_t len);
int xsHttpStart(XS_ResponseObject resp, unsigned status, const char* reason,
                const char* headers);
int xsHttpSend(XS_ResponseObject resp, const void* data, size_t len);
int xsHttpEnd(XS_ResponseObject resp);
```

要求：

- [x] header block 必须防 CRLF 注入
- [x] 禁止开发者覆盖 `Content-Length / Transfer-Encoding / Connection` 等框架控制头，或明确采用白名单策略
- [x] `Content-Type`、CORS、Cache-Control 等普通响应头允许直接传入
- [x] `reply` 快路径避免创建完整 response builder
- [x] `start/send/end` 支持 chunked 或框架定义的流式响应模式

验收：

- [x] xadmin 普通 JSON 响应不需要逐步构建 response object
- [x] 大响应可分段发送
- [x] 非法 header 被拒绝
- [x] 正常响应路径不构建完整 response builder，容量 smoke 限制工作集增量

### H3. 静态文件 headers 与发送

跟踪：`[x]`

xs Host 配置支持静态响应头：

```json
"headers": {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Methods": "GET, HEAD, OPTIONS",
  "Cache-Control": "public, max-age=86400"
}
```

要求：

- [x] 配置加载期解析并校验 headers
- [x] 请求期只做数组遍历设置
- [x] 静态文件发送避免默认 `read all + copy body`
- [x] 支持 HEAD 不读 body
- [x] CORS OPTIONS 可配置为 fast path 直接响应

验收：

- [x] xhome / xxrpa.com 类跨域需求不需要重写静态文件处理
- [x] 大文件不会一次性载入内存
- [x] HEAD 请求不读取文件 body

## XTP 设计

### X1. 消息分类 fast path

跟踪：`[x]`

XTP 解析阶段必须产出适合分发的整数语义：

- `msg_type`
- `flags`
- `status`
- `cmd_len`
- 可选 `cmd_id`

要求：

- [x] `msg_type`、`flags`、`status` 已存在时，xs 脚本层可直接读取整数
- [x] xadmin 不再用字符串或 JSON 状态推断
- [x] 常见命令允许使用解析期整数 `cmd_id`
- [x] 未注册命令保留 `cmd` 原文
- [x] 命令注册表在加载期建立，解析期一次性确认，运行期分发只做整数比较

建议 API：

```c
uint16 xsXtpMsgType(XS_XtpMessageObject msg);
uint16 xsXtpFlags(XS_XtpMessageObject msg);
int32 xsXtpStatus(XS_XtpMessageObject msg);
uint32 xsXtpCmdID(XS_XtpMessageObject msg);
uint32 xsXtpCmdIDFrom(const char* cmd);
const char* xsXtpCmd(XS_XtpMessageObject msg);
```

验收：

- [x] xadmin 常见 XTP command 分发不再反复 `strcmp`（当前 xadmin 无实际 XTP handler，已同步新 API 头）
- [x] xs demo 常见 XTP command 分发不再反复 `strcmp`
- [x] unknown command 仍可按字符串处理
- [x] request/response/push/event 判断走整数

### X2. 轻量 XTP builder

跟踪：`[x]`

XTP 回复应支持低成本分段或 builder-lite 模型。

建议 API：

```c
int xsXtpReplySimple(stream, req, status, cmd, body, body_len);
int xsXtpReplyStart(stream, req, status, cmd);
int xsXtpReplyParam(stream, key, value);
int xsXtpReplyBody(stream, body, body_len);
int xsXtpReplyEnd(stream);
```

要求：

- [x] 普通回复不要求构建完整对象树
- [x] param 数量和 body 大小仍受 recv/send limit 约束
- [x] builder 生命周期清晰，失败后可关闭或重置
- 不把 pending request / 持久 client 对象体系放回 production 阻塞项

验收：

- [x] xadmin 常见 XTP 回复代码明显缩短（当前 xadmin 无实际 XTP handler，已同步新 API 头）
- [x] 大 body 可避免不必要字符串拼接
- [x] 错误回复可一行完成
- [x] xs demo 普通回复可用短 API / builder 完成

### X3. XTP 输入安全

跟踪：`[x]`

必须保留或加强：

- packet header 完整性检查
- `recv_limit`
- param count 上限
- param key/value 长度上限
- body size 上限
- 非法 flags / type / status 拒绝策略
- 异常连接关闭策略

验收：

- [x] 恶意包、半包、超限包、非法 type 均有测试
  - [x] XTP 半包 / idle cleanup
  - [x] XTP 坏 header
  - [x] XTP size / recv_limit 超限
  - [x] XTP 非法 type 独立用例
- [x] xsdbg 可观测 reject 原因，但 production 默认不承担详细统计成本

## 调试与统计隔离

跟踪：`[x]`

要求：

- [x] production `xs` 默认关闭详细 runtime stats
- [x] `xsdbg` 保留协议调试、错误排查、内存调试、reload/check 诊断
- [x] 统计逻辑不得出现在 production 正常请求热路径，除非显式配置开启
- [x] fast path 测试必须能证明未触发 debug/metrics 逻辑

## 路线图

| 阶段 | 仓库 | 标记 | 任务 |
| --- | --- | --- | --- |
| P0 | xrt | `[x]` | HTTP method ID 进入 `xhttpdrequest` |
| P0 | xs | `[x]` | xs 内部 HTTP method 判断改整数 |
| P0 | xs | `[x]` | 脚本导出 `xsReqMethodID` |
| P0 | xrt | `[x]` | `xrtHttpdConnReply` |
| P0 | xs | `[x]` | `xsHttpReply` |
| P1 | xs | `[x]` | 静态 Host `headers` 配置 |
| P1 | xrt | `[x]` | 静态文件分块发送 API |
| P1 | xrt | `[x]` | HTTP `start/send/end` |
| P1 | xs | `[x]` | 脚本导出 `xsHttpStart/Send/End` |
| P1 | xs | `[x]` | XTP 解析期 `cmd_id` 与脚本导出 |
| P1 | xs | `[x]` | XTP command ID / command registry 设计 |
| P1 | xs | `[x]` | XTP 轻量 reply API |
| P2 | xs | `[x]` | production stats 默认关闭或编译隔离 |
| P2 | xadmin | `[x]` | HTTP 路由和响应迁移 |
| P2 | xadmin | `[x]` | XTP 路由和回复迁移（当前无实际 XTP handler，完成 API 同步） |
| P2 | all | `[x]` | 压测、内存峰值、安全回归 |

## 验收基线

### 安全

- [x] 超长 request line / path 被拒绝
- [x] header count / header line 超限被拒绝
  - [x] header count 超限被 xrt/xs 拒绝
  - [x] header line 超限独立用例
- [x] path traversal 被拒绝
- [x] body 超限被拒绝
- [x] 非法 response header 被拒绝
- [x] XTP 半包、坏包、超限包被拒绝
- [x] 静态文件不能访问点文件、反斜杠路径或根目录外文件

### 性能 / 容量

- [x] HTTP 普通 reply 路径容量基线：`xs-capacity` 100 次 `/json`，工作集增量上限 16MB
- [x] 静态大文件发送内存峰值基线：`xs-capacity` 4MB 静态文件，工作集增量上限 24MB
- [x] XTP 大 body 回复内存峰值基线：`xs-capacity` 20 次 64KB `demo.largebody`，工作集增量上限 32MB
- [x] 240M 级内存预算下的稳定 smoke / 压测：`xs-capacity` 工作集预算上限 240MB

### 性能

- [x] 常见 HTTP method 判断不再依赖 `strcmp / strcasecmp`
- [x] production 正常请求不触发详细 metrics
- [x] 普通 HTTP reply 不要求完整 response object 构建
- [x] 静态大文件不一次性读入内存
- [x] XTP 常见 command 分发支持整数路径

### 易用性

- [x] xadmin 普通 JSON 响应接近 mongoose 时期代码量
- [x] CORS 不需要重写静态文件服务
- [x] XTP 普通成功/错误回复可用短 API 完成
- [x] 复杂场景仍可使用 managed/builder API

## 已决策约束

1. HTTP method ID 由 xrt 解析期生成，脚本层通过 `xsReqMethodID()` 读取；常见分发走整数比较，原始 method 字符串仍可用于兼容和诊断。
2. `start/send/end` 支持流式发送；未声明 `Content-Length` 的场景走 chunked，普通完整回复优先使用 `xsHttpReply()`。
3. 静态文件发送使用固定分块缓冲，默认 `XHTTPD_FILE_CHUNK_SIZE`，避免一次性读入大文件。
4. XTP command ID 采用 xs 进程内 registry；注册后只允许精确命中获得非零 ID，未注册或碰撞命令保留原字符串并令 `CmdID=0`。
5. production stats 通过 `xs` / `xsdbg` 行为与 `XRT_MEM_DEBUG` 隔离，公网热路径默认不承担详细统计成本。
