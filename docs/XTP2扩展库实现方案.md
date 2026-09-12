# XTP2 扩展库实现方案

状态：独立核心库与 xs 可选宿主导入均已实现。核心仍为单文件 [lib/xtp2.h](/D:/GIT/xserver/lib/xtp2.h)；执行 build.bat xtp 或 bash build.sh xtp 可内置到 xs，也可与 sqlite 等扩展组合。没有增加 XTP 网络驱动，TCP/TLS 连接仍由用户管理。

目标：提供一个小巧、完整、可独立使用的 C 协议库。用户自行建立 TCP 或 TLS 连接，将收到的明文字节交给协议处理器，并自行发送编码结果。兼容历史 XTP2 线路格式。

## 1. 实施边界

库负责以下能力：

- XTP2 报文编码、完整报文解码、增量接收处理。
- 任意拆包位置、连续多包、包间无缝衔接。
- 消息头、command、参数和 body 的完整校验。
- 有序参数迭代、按 key 查找、二进制数据访问。
- 明确的内存所有权、已消费字节数、EOF 和错误状态。
- 可配置资源上限、可替换分配器。

连接创建、TLS 握手与证书验证、发送队列、线程、超时、连接关闭属于调用者。请求关联、业务路由、JSON、压缩、重试、认证、指标和管理接口由应用组合实现。

在 xs 中保持现有 `tcp`/`custom` 接入方式。新增内容作为可选扩展库装入脚本环境，库中没有 server、socket、listener 或 xs 配置对象。

“功能完整”的验收对象是协议编解码和处理器生命周期；TCP/TLS 由现有网络库提供。

## 2. XTP2 兼容规范

采用固定 32 字节头，所有多字节整数使用 little-endian：

| 偏移 | 字节数 | 字段 | 规则 |
|---|---:|---|---|
| 0 | 4 | Magic | 字节 `78 74 70 02`，即 `xtp` 加版本 2 |
| 4 | 4 | PackSize | 无符号，总包长，包含头部 |
| 8 | 8 | MsgID | 无符号，完整保留 64 位 |
| 16 | 2 | Flags | 不解释的 16 位字段，原样保留 |
| 18 | 2 | MsgType | 1=request，2=response，3=push，4=event |
| 20 | 2 | CmdSize | command 的字节数 |
| 22 | 2 | ParamCount | 参数项数量 |
| 24 | 4 | BodySize | body 的字节数 |
| 28 | 4 | Status | 32 位二进制补码有符号整数 |

后续字节排列固定为：

```text
Header[32]
ParamInfo[ParamCount]        每项 KeySize:u16 + ValueSize:u16
Command[CmdSize]
Key1, Value1, Key2, Value2, ...
Body[BodySize]
```

总包长必须满足：

```text
PackSize = 32 + 4 × ParamCount + CmdSize
           + Σ(KeySize + ValueSize) + BodySize
```

兼容性具体约定：

1. 保留四种消息类型，其他类型报错。
2. `Flags` 任意位模式都可编解码，默认发 0；历史代码没有定义可据以拒绝的合法位掩码。应用可自行约束，不给旧位追加新的库内语义。
3. `Status` 接受完整 int32 范围。0 可作为业务成功约定，不强制其他消息类型的 status 必须为 0。
4. `MsgID=0` 合法。沿用历史规则：只有 `MsgType=request && MsgID!=0` 表达需要回复的请求。
5. response 通常回显请求 MsgID；库不维护 pending 表，也不自动匹配、回复或超时。
6. command、参数名、参数值、body 均为显式长度的字节序列；允许空字段和内嵌 NUL，不强制 UTF-8，不添加字符串结束符。
7. 参数保留线路顺序和重复 key。查找返回第一个匹配项，与历史 XTP2 查找行为一致。
8. 无隐式 padding，无额外尾字节；一个完整报文的输入长度必须与 PackSize 相等。
9. 第一版只接收版本 2；遇到版本 1 或其他版本返回版本错误。
10. 兼容指线路格式与上述语义。旧的 `xsXtp*` 函数、对象布局和运行时不构成新的源码 ABI。

采用显式整数 load/store；禁止通过结构体强转或 `memcpy(struct)` 编解码线路头和参数信息。`CmdID` 是旧实现的本地派生值，没有对应 wire 字段，应用需要时可自行计算。

资源限制属于本地接收策略：默认策略可能拒绝超过默认上限的历史大包，调用者可在字段可表示范围内调整。限额拒绝不能表述为线路格式不兼容。

最小 golden 样本：request、MsgID=1、Flags=0、Status=0、command=`ping`，无参数和 body，总长 36 字节：

```text
78 74 70 02 24 00 00 00 01 00 00 00 00 00 00 00
00 00 01 00 04 00 00 00 00 00 00 00 00 00 00 00
70 69 6e 67
```

## 3. 源码与交付形态

核心采用单文件形式；用户部署时只需复制 `xtp2.h`：

```text
lib/xtp2.h                     全部公开声明 + XTP2_IMPLEMENTATION 实现区
tools/xtp2_test.c               编解码、状态机、故障注入、历史编码器兼容测试
tools/xtp2_fuzz.c               libFuzzer + ASan/UBSan 测试入口
```

上表均已提供，没有配套 `.c` 实现文件。在应用的一个编译单元中写：

```c
#define XTP2_IMPLEMENTATION
#include "xtp2.h"
```

使用 C99 能力子集，依赖 `stdint.h`、`stddef.h`、`stdbool.h`、`stdlib.h`、`string.h`。支持 C++ 调用时使用 `extern "C"`；核心无 OS 分支、可变全局状态、线程局部变量或全局命令注册表。

两种使用方式共用该实现：

- 普通程序或脚本在自己的一个编译单元中定义 `XTP2_IMPLEMENTATION`，其他编译单元仅 include 头文件。
- xs 通过构建参数 `xtp` 启用 `XS_USE_XTP2`，将单头实现编入宿主，公开声明放入 VFS，并直接向 TCC 导入公开函数。脚本此时只需 `#include <xtp2.h>`，不要再定义实现宏。

同一编译单元在“使用宿主符号”和“自行编译实现”中选一种。每个最终链接单元只出现一份外部实现。应用源码方式可用现有 `dev_inc`；仅设置 `dev_lib` 是增加搜索路径，链接原生库还需实际指定库名。

首版核心头含声明、实现、注释共 648 行，22827 字节；13 个外部函数与 2 个 static inline helper。原定规模预算为 800～1200 行，本版不需要额外的转发层或包装对象。测试另计。

## 4. 数据结构与公开 API

以下接口已在单头文件中实现；函数注释包含参数、错误和生命周期契约。

```c
typedef struct {
    const void *data;
    size_t size;
} xtp2_span;

typedef struct {
    xtp2_span key;
    xtp2_span value;
} xtp2_param;

/* 发送描述：借用调用者的数据，编码期间必须稳定。 */
typedef struct {
    uint64_t id;
    int32_t status;
    uint16_t type;
    uint16_t flags;
    xtp2_span command;
    xtp2_span body;
    const xtp2_param *params;
    size_t param_count;
} xtp2_packet;

/* 解码视图：由 decode/feed 产生，各指针指向 packet 内部。 */
typedef struct {
    uint64_t id;
    int32_t status;
    uint16_t type;
    uint16_t flags;
    uint16_t param_count;
    xtp2_span command;
    xtp2_span body;
    xtp2_span packet;
} xtp2_message;

typedef struct {
    size_t max_packet;
    size_t max_body;
    uint32_t max_command;
    uint32_t max_params;
    uint32_t max_key;
    uint32_t max_value;
} xtp2_limits;

/* size=0 时释放并返回 NULL；分配失败时原指针仍有效。 */
typedef void *(*xtp2_realloc_fn)(void *user, void *ptr, size_t size);

typedef struct {
    xtp2_limits limits;
    size_t retain_buffer;
    xtp2_realloc_fn realloc_fn;
    void *allocator_user;
} xtp2_config;

typedef struct xtp2_processor xtp2_processor;

/* 游标由 xtp2_params 初始化；调用者不修改字段。 */
typedef struct {
    const unsigned char *info;
    const unsigned char *data;
    size_t remaining;
} xtp2_iter;

enum {
    XTP2_REQUEST = 1, XTP2_RESPONSE = 2,
    XTP2_PUSH = 3, XTP2_EVENT = 4
};

typedef enum {
    XTP2_OK = 0, XTP2_MORE = 1, XTP2_MESSAGE = 2,
    XTP2_E_ARGUMENT = -1, XTP2_E_MAGIC = -2,
    XTP2_E_VERSION = -3, XTP2_E_TYPE = -4,
    XTP2_E_LENGTH = -5, XTP2_E_LIMIT = -6,
    XTP2_E_NOMEM = -7, XTP2_E_CAPACITY = -8,
    XTP2_E_TRUNCATED = -9, XTP2_E_STATE = -10
} xtp2_result;
```

接口分为处理器、编解码、视图访问三组：

```c
void xtp2_config_init(xtp2_config *config);
xtp2_result xtp2_create(const xtp2_config *config,
                      xtp2_processor **out);
void xtp2_destroy(xtp2_processor *processor);
void xtp2_reset(xtp2_processor *processor);

xtp2_result xtp2_feed(xtp2_processor *processor,
                     const void *data, size_t size,
                     size_t *consumed, xtp2_message *message);
xtp2_result xtp2_finish(xtp2_processor *processor);

xtp2_result xtp2_decode(const void *data, size_t size,
                       const xtp2_limits *limits,
                       xtp2_message *message);
xtp2_result xtp2_measure(const xtp2_packet *packet,
                        const xtp2_limits *limits, size_t *size);
xtp2_result xtp2_encode(const xtp2_packet *packet,
                       const xtp2_limits *limits,
                       void *output, size_t capacity,
                       size_t *written);

xtp2_iter xtp2_params(const xtp2_message *message);
bool xtp2_param_next(xtp2_iter *iter, xtp2_param *param);
bool xtp2_find(const xtp2_message *message,
               xtp2_span key, xtp2_span *value);
const char *xtp2_error_string(xtp2_result result);

/* 两个小型 static inline helper。 */
static inline xtp2_span xtp2_text(const char *text);
static inline bool xtp2_wants_reply(const xtp2_message *message);
```

统一规则：

- `config=NULL` 或 `limits=NULL` 使用文档化默认值；传入非空结构时按其字段原值执行，调用者先初始化再覆盖配置。
- create 复制配置，成功输出独立实例；失败时 `*out=NULL`。realloc_fn 为 NULL 时使用默认分配器；自定义分配器必须提供满足普通 C 对象要求的对齐，其 user context 活到 destroy 结束。
- span 的 `size=0` 表示实际空数据；此时允许 `data=NULL`。`size>0` 时 data 必须有效。只有 `xtp2_text` 使用 strlen，`xtp2_text(NULL)` 产生空 span。
- 编码输入的 param_count 使用 size_t，校验后才转换到 u16，避免入口处先发生静默截断。
- message 的成员直接读；参数迭代器只接受库生成且仍有效的 message。
- `find` 比较完整 key 长度与字节，单次 O(n)；迭代总成本 O(n)。不提供容易诱发重复扫描的索引式遍历包装。
- decode 成功返回 OK；对格式合法的头，输入不足完整帧返回 TRUNCATED，多于声明包长返回 LENGTH；失败清零 message。measure 失败将输出 size 置 0；find 未命中或迭代结束时清零相应输出。返回 xtp2_result 的函数检查必需输出指针，为 NULL 时返回 E_ARGUMENT；bool 查询函数要求提供有效输出对象。
- 错误名称为只读静态字符串；没有线程局部 last-error，也不依赖 errno。

## 5. 增量处理器的精确契约

`xtp2_feed` 每次最多交付一条消息。一次网络读取含多包时，用户根据 consumed 循环调用；业务暂停时，用户保留未消费后缀。

| 返回值 | consumed | message | 行为 |
|---|---|---|---|
| MESSAGE | 到当前帧末尾的输入字节数 | 有效 | 停在帧边界，后缀由调用者持有 |
| MORE | 本次 size | 清零 | 本次输入全部复制入处理器，等待后续输入 |
| 解析/分配错误 | 已读取的输入前缀长度 | 清零 | 处理器记住首个错误，等待 reset/destroy |
| 参数/状态错误 | 0 | 在输出指针有效时清零 | 不接收新字节 |

对有效的非空输入，成功返回必须前进，不出现 `consumed=0` 的无限循环。在正常解析状态，size 为 0 时允许 data 为 NULL，返回 MORE；零长度 feed 不表示 EOF。FAILED/EOF 状态优先按其错误契约处理。所有返回都满足 `consumed <= size`。

FAILED 状态下再次 feed 返回原解析错误、consumed=0。调用参数错误不改变既有解析进度。检测到异常流后，调用者通常关闭连接；库不扫描后续 magic 猜测恢复位置。

处理器内部使用这些状态：

```text
HEADER → TABLE → PAYLOAD → READY
  ↑                         │
  └──────── 下一次 feed ─────┘

解析/分配失败 → FAILED
正常接收结束 → EOF
```

具体步骤：

1. HEADER：在固定 32 字节空间收齐头部，验证 magic、版本、类型和 header 中的资源上限。先验证 `32 + 4*n + cmd_size + body_size <= PackSize`，再进入 TABLE。
2. TABLE：收齐 `4*n` 个参数信息字节后，逐项读取 little-endian 长度，校验 key/value 上限并重算完整包长。必须严格等于 PackSize。n=0 时直接做同一总长检查。
3. PAYLOAD：只接收已校验帧的剩余字节。当前 feed 的多余后缀不复制入本帧。
4. READY：构造 command、body、packet 的只读视图，返回 MESSAGE。下一次 feed 回到 HEADER 并开始下一帧。

不在 TABLE 到达前遍历 ParamCount，不在当前帧范围外读取下一帧的数据。内部共用 header 校验、参数表校验和视图构造三个小函数，完整包 decode 与增量 feed 使用同一规则。

所有字段宽度校验通过后，包长计算使用 uint64_t 累加；上述字段的线格式上界保证计算不溢出 uint64_t。再与 UINT32_MAX、SIZE_MAX、配置上限比较，通过后才转换为 size_t、分配或移动指针。编码端同样先限制单字段宽度，避免从任意 size_t 输入直接做加法。

处理器不调用业务回调，业务在 feed 返回后自行调用处理函数。单实例禁止并发 feed/reset/finish/destroy；不同实例独立，交由各连接所属线程或 worker 使用。

## 6. 内存、消息保留与 EOF

接收端默认使用一个处理器自有的组包缓冲。处理器仅在已接收数据确有需要时扩容，按几何增长并钳制到当前包长与 max_packet；只声明很大 PackSize 不立即分配整个大包。

无参数 span 数组、参数字典、每字段字符串分配或多包队列；参数视图由线路数据和游标生成。粘包后缀留在调用者的输入中，无需把剩余包 memmove 到缓冲起点。

常态每段输入复制进组包缓冲一次，扩容可能额外搬移已有前缀；组好的 packet 直接承载字段视图。完整包 `decode` 不分配，也不复制。

消息有效期：

- feed 产出的视图有效至该 processor 下一次 feed、finish、reset 或 destroy，任一操作都视为终止前一借用期。
- decode 产出的视图有效至调用者修改或释放输入字节。
- struct 浅拷贝只复制视图，不能延长数据寿命。异步保存时复制 `message.packet` 的完整字节，再调用 decode 生成指向副本的视图；副本由应用释放。
- 同一连接有多个异步业务任务时，各任务自行保留所需数据，不能共用可被下一次 feed 覆盖的借用视图。

retain_buffer 默认 64 KiB：完成消息后的下一次 feed 保留不超过此值的缓冲，超过则释放。为遵守消息借用期，不能在返回 MESSAGE 前释放大包；等待下一次操作期间仍可能持有上一包。消费者读完后也可 reset 主动释放全部缓冲。reset 保留配置，清空进度、错误和 EOF 标记；仅可在调用者确认的帧边界或新字节流上重新开始。

finish 表示接收方向不会再来数据：帧边界返回 OK 并进入 EOF；存在半个 header/table/payload 返回 TRUNCATED 并进入 FAILED；既有解析错误原样返回。重复 finish 返回同一结束结果，EOF 后 feed 返回 STATE；重用须 reset。finish 不关闭 socket，不代表整个双向连接已关闭。

config_init、reset、destroy 的对象指针为 NULL 时均为空操作。分配器契约包含 free 语义，默认适配器对 size=0 显式调用 free，避免依赖平台对 realloc(ptr,0) 的差异。reset/destroy 释放的内存都由原分配器处理。

## 7. 默认限额与失败策略

| 配置 | 默认值 | 语义 |
|---|---:|---|
| max_packet | 1 MiB | 单帧总字节硬上限 |
| max_body | 1 MiB | 另受 max_packet 总长约束 |
| max_params | 256 | 参数数量上限，可调至 65535 |
| max_command | 65535 | command 字节上限 |
| max_key | 65535 | 单 key 字节上限 |
| max_value | 65535 | 单 value 字节上限 |
| retain_buffer | 64 KiB | 帧完成后的缓冲复用阈值 |

max_packet 必须至少为 32，且不得超过 wire u32 与本机 size_t 能表示的范围。其他字段不能超过各自 wire 上界；超出视为配置错误。零值不表示无限：max_body=0 只允许空 body，max_params=0 只允许无参数，其他字段同理；retain_buffer=0 表示帧间不缓存堆缓冲。

限额始终生效，不受日志、调试或治理开关影响。编码和解码接受相同类型的 limits，但发送端与接收端可以选择各自的限制。

单 processor 逻辑持有的堆缓冲不超过 max_packet，另有固定大小对象；realloc 移动时分配器可能短暂同时持有旧块和新块。跨连接总内存还包括网络缓冲、发送队列及业务保存的数据，调用者应另设总连接数、发送队列预算和单帧接收期限。仅 idle timeout 无法限制持续慢速发送的半包；协议库不创建定时器。

发现坏 magic、错误版本、非法类型、长度不一致、超限或内存分配失败时交还明确错误；不自动重试、不跳过坏帧、不自动回发 error 报文。

## 8. 编码与发送

`measure` 校验全部字段并给出精确输出大小；`encode` 调用同一测量逻辑，检查输出容量后再写入。参数 key/value、command、body 使用同一个 span 模型，二进制数据和空数据没有特别分支。

编码端执行以下顺序：

1. 校验字段宽度、配置限额、指针与长度的组合，计算完整包长。
2. 检查 output 容量；失败时 written=0，输出字节保持不变。
3. 按固定偏移写 32 字节头和参数信息表。
4. 顺序复制 command、参数内容和 body；成功时 written 等于完整包长。

编码输入与输出不能重叠；调用者负责输入对象及其 span 在整个调用期间有效且不被并发修改。编码器不分配发送对象，也不返回需要特殊释放的 buffer。

应用可用栈缓冲发送小包，或先 measure 再分配大包。request/response/push/event 共用 packet 描述，通常构造 response 时设置 type、原 MsgID 和业务 status 即可。`xtp2_wants_reply` 是提示函数，库不强制执行 RPC 策略。

网络发送有独立契约：

- 原始 socket 或同步 TLS 写入可能短写，调用者保留整帧和已发送偏移，继续发送剩余字节。
- 同一连接串行化帧发送；一帧尚未发送完时，后续帧进入调用者的 FIFO，不能交错拼接。
- 会复制输入的异步发送 API 受理后，调用者可按其契约释放原编码缓冲；借用型 API 必须等发送完成。
- 队列满是背压，不是 XTP 格式错误。调用者选择暂停业务、等待可写或关闭连接。
- 部分帧发送后出现致命错误，应结束这条连接，不能在其后重发完整帧。

初版提供完整帧编码接口即可。分段 builder、send callback、发送队列和分块 body 事件会增加状态与所有权分支，留给出现明确使用需求后的独立扩展。

## 9. TCP/TLS 使用方式

处理器处理的是 TLS 解密后的明文字节，发送编码结果交给用户的 TLS 写入函数：

```text
收：TCP → TLS 解密（可选）→ feed → 业务消息
发：业务消息 → encode → TLS 加密（可选）→ TCP
```

收发双方应在建立连接时明确普通 TCP 或 TLS。协议库不做 STARTTLS、TLS 探测或明文降级。TLS record 边界不影响 XTP 分包，证书验证由用户的 TLS 连接配置负责。

最小接收循环如下；处理器已实现，示例中的业务函数与连接状态由应用提供：

```c
size_t offset = 0;
while (offset < received_size) {
    size_t used = 0;
    xtp2_message message;
    xtp2_result r = xtp2_feed(processor,
        bytes + offset, received_size - offset, &used, &message);
    offset += used;

    if (r < 0) {
        /* 用户结束连接；按自己的生命周期释放 processor。 */
        break;
    }
    if (r == XTP2_MESSAGE) {
        /* 在下一次 feed 前使用或复制 message。 */
        handle_message(&message);
        if (connection_closing || business_paused) break;
    }
    if (r == XTP2_MORE) break;
}
/* 仅 offset 字节已消费；剩余 received_size-offset 字节由用户保留。 */
```

每条接收字节流拥有独立 processor；双向通信两端各自解析自己收到的流。重连开始新流时创建新 processor 或明确 reset；不能把旧连接的半包带到新连接。

## 10. 在当前 xs 中接入

库的可选内置接入已通过统一扩展清单完成：

1. tools/extensions.json 的 xtp 条目统一声明宏、源码、VFS 头和符号清单；tools/build.py 使用 -x c 将 xtp2.h 编译成独立对象，仅该对象定义 XTP2_IMPLEMENTATION。
2. 仅在选中 xtp 时将 xtp2.h 打包到 /xs；公开函数清单位于 src/script/import_xtp2.inc。
3. TCC 初始化直接导入全部 13 个外部 API，并定义 XS_USE_XTP2=1；无 XS_ScriptXtp 转发层，嵌套 xsCreateTCC 环境一致。
4. tools/test_extensions_runtime.py 覆盖脚本调用、嵌套编译与未启用时的头/符号缺失；Windows/Linux 的四种 sqlite/xtp 组合已通过构建和探针测试。
5. 用户通过 tcp 的 EventOpen/EventData/EventClose，或自建 xrt 连接，把明文字节交给处理器。

应用源码交付方式可以直接通过 dev_inc 引入同一单头文件，不需要进行上述宿主构建修改。

当前 xs 的 tcp 接口有三个必须遵守的事实：

- XS_StreamConn 在回调栈上构造，不能保存其地址；需要保存时复制两个流指针及传输类型，并遵守原流生命周期。
- stream 的用户数据和事件表已经由 xs 的连接记录占用。应用不能通过 SetData/SetEvents 覆盖它们，也不能覆盖 Host/Server 的 Runtime 字段来放置 processor。
- Close 可能在发送等操作中同步重入。应用连接会话需要保证活动 EventData 结束前仍然有效。

对 tcp 脚本示例，使用“本脚本代私有的会话表”：

| 事件 | 示例应用职责 |
|---|---|
| EventOpen | 以实际 tcp/tls 指针和传输类型为 key，创建 session 与 processor |
| EventData | 从表中获取并持有 session 引用，喂入数据，处理消息，最后释放本次引用 |
| EventClose | 标记 session closing，移出表，撤销连接持有的引用 |
| 最后一个引用释放 | destroy processor，再释放 session |

会话表只属于示例/应用；协议库没有全局连接表。多 worker 下保护表的查找和增删，不持有表锁执行消息处理或发送。业务处理触发 Close 后，EventData 检查 closing 并停止；退出前引用保护内存，不再次使用已终结的网络对象。

同一 session 的接收与业务处理串行执行；业务还在使用 message 时不能再次 feed 同一实例。示例可用 processing/pending 标记保护接收循环，同步重入 Read 时只标记待处理，由外层继续读取，避免覆盖仍在使用的视图。

接收缓冲处理建议：

- 使用 xrtNetBufFront 取得当前连续 span，喂给 feed；调用者依据 consumed 消费该 span 的对应前缀。
- TCP 使用 xrtNetStreamConsume；TLS 使用 xrtTlsStreamConsume。每次消费后重新获取 span，不保留旧 buffer 指针。
- feed 已把未完成帧复制到自己的缓冲，因此 MORE 后仍可消费其已接收字节，使 TLS 明文缓冲继续前进。无需在 TLS 内积累完整的 1 MiB XTP 包。
- 已产生的 message 指向 processor 自有内存，因此消费网络输入不会使其失效；下一次 feed 才终止其借用期。
- 正常接收路径尽量“feed → 消费 consumed → 处理消息”。如果消费操作本身导致 Close，应检查 session 状态并停止后续处理。
- 用户暂停业务时保留未消费后缀，恢复时主动处理已有缓冲；不能只等待下一次 Read 通知。若选择保留 TLS 明文继续积累输入，才按 xrt 契约使用 ReadMore。

当前 xrtTlsStreamSend 允许成功短写，XTP 示例必须处理这一点。可使用用户自有发送 FIFO，也可按 xrtTlsStreamSendAsync 的完整明文提交契约发送，并正确处理 Future 的受理、完成、失败和释放。示例不能把“返回 XTLS_OK”当作整包发送成功。

热重载按既有连接规则工作：新连接使用新脚本代，旧连接与 processor 留在旧代。无需转移半包、迁移 processor 或改变旧连接版本。应用额外启动的线程、Future 回调和定时器必须有相应的生命周期保护，不能假定 xs 自动跟踪任意脚本任务。custom 自持资源仍遵循当前 custom 在线卸载限制。

## 11. 测试与验收

测试输入必须有独立参考来源。仅做“本库 encode → 本库 decode”会掩盖双方共同的布局错误。

| 测试组 | 必测内容 |
|---|---|
| 线路兼容 | 固定 golden bytes、旧客户端生成的合法报文、独立参考编码器；逐字节验证 magic、32B 头偏移、参数表和负 status |
| 所有消息类型 | request/response/push/event，MsgID=0 与 UINT64_MAX，非零 flags，Status 的 0/正值/负值/INT32_MIN |
| 字节边界 | 空 command/参数/body，嵌入 NUL，重复 key，UTF-8 仅作为普通字节处理，完整 decode 不允许尾随数据 |
| 增量行为 | 对小型 golden frame 遍历每个拆分点，逐字节 feed，随机分片、多帧拼接、当前帧后带半个下一帧 |
| 消费契约 | 每条消息只交付一次，consumed 精确停在帧末；MORE 用完本次输入；业务暂停与恢复不丢不重 |
| 畸形输入 | 仅有 32B 头却声明大 ParamCount，截断参数表，表长度超出帧，伪造总长，小于 32 的 PackSize，整数边界 |
| 资源边界 | 每个限额的等于上限/超过上限，空字段零上限配置，大小声明但无 payload 时不按整包预分配 |
| 内存失败 | 处理器创建、TABLE/PAYLOAD 扩容分别注入失败；原指针保留、无 double-free、reset/destroy 完整释放 |
| 生命周期 | 所有阶段 EOF、重复 finish、错误粘滞、reset 后新流、跨实例隔离、借用结束后的副本仍有效 |
| 编码失败 | 容量不足或字段非法时输出缓冲不变、written=0；非空指针配零长度按空字段编码 |
| 网络互操作 | 普通 TCP 与 TLS 收发相同 golden 数据，TLS record 跨帧及跨记录大包，短写/背压/半帧断连 |
| xs 接入 | TCC 符号与单头两种模式；多连接多 worker；同步 Close 重入；reload 时旧连接半包继续完成、新连接使用新代 |
| 工具链 | Windows/Linux，GCC/Clang 与当前 TCC；使用 ASan/UBSan 和 fuzz 检查解析器；有 32 位/大端环境时增加实机或模拟测试 |

互操作以旧 wire 格式和经过检查的参考客户端为依据，不把旧服务端的越界、长度截断或参数字符串截断作为期望行为。历史 smoke 客户端也需要先审查测试用到的路径，不把它当作安全解析器直接复用。

性能验证记录实际测量：encode/decode/feed 吞吐、分配次数、processor 缓冲峰值、许多小包与交替大/小包的表现。核心目标是总解析 O(包字节数 + 参数数)、顺序迭代 O(参数数)，不存在逐个参数建字典或粘包后缀反复搬移。

## 12. 实施顺序与完成条件

1. 固定上述 wire 规范、错误语义、默认限额和 golden 样本。
2. 实现 load/store、长度校验、measure/encode/decode、参数迭代，完成独立线路测试。
3. 实现 processor、consumed 契约、finish/reset、分配器与限额测试，覆盖历史半包问题。
4. 增加 TCP/TLS 示例与 xs 可选库导入，验证收发背压、会话保存和终态引用。
5. 完成互操作、TCC、动态检测及适量 fuzz，记录实际规模和分配指标。

核心库、API 注释、线路规范、接收循环示例、独立测试与可选宿主导入已交付。核心没有未实现分支或占位处理；所有公开函数都有明确的错误和所有权契约。实际 TCP/TLS 业务示例及 XTP 网络端到端、热重载测试仍未实施。

XTP2 是否启用不影响 xs 的五种协议类和既有驱动行为。TCP/TLS 的资源管理继续由用户选择的连接层完成。

## 13. 当前代码依据

- 历史 XTP2 头字段：[xtp.h](/D:/GIT/xserver/dev/v1/src/protocol/xtp.h:4)。
- 历史需要回复的判定：[xtp.h](/D:/GIT/xserver/dev/v1/src/protocol/xtp.h:2212)。
- 历史解析器与参数表读取顺序：[xtp.h](/D:/GIT/xserver/dev/v1/src/protocol/xtp.h:1465)。
- 参考客户端的小端编码：[xtp_smoke_client.c](/D:/GIT/xserver/dev/v1/tools/xtp_smoke_client.c:236)。
- 当前 XS_StreamConn 契约：[xsbase.h](/D:/GIT/xserver/src/sdk/xsbase.h:157)。
- 当前 TCP 用户数据归属与回调引用：[stream.h](/D:/GIT/xserver/src/protocol/stream.h:123)。
- 当前 TLS 发送与明文消费契约：[xrt_decl.h](/D:/GIT/xserver/lib/xrt_decl.h:27469)。
- 当前应用 include/lib 路径：[tcc_host.h](/D:/GIT/xserver/src/script/tcc_host.h:164)。
- 当前可选扩展库部署约定：[设计.md](/D:/GIT/xserver/docs/设计.md:454)。

## 14. 首版使用与验证记录

最小发送示例（编译时增加 `-Ilib`，路径按应用目录调整）：

```c
#define XTP2_IMPLEMENTATION
#include "xtp2.h"

int main(void)
{
    unsigned char bytes[256];
    size_t written;
    xtp2_packet request = {0};
    request.type = XTP2_REQUEST;
    request.id = 1;
    request.command = xtp2_text("ping");
    if (xtp2_encode(&request, NULL, bytes, sizeof(bytes), &written) != XTP2_OK)
        return 1;
    /* 将 bytes[0..written) 交给用户的 TCP/TLS 发送接口。
       短写时保留偏移并继续发送；异步借用时保留缓冲至完成。 */
    return 0;
}
```

接收时每条字节流 `xtp2_create(NULL, &processor)`，按第 9 节循环 feed，收到 EOF 调用 finish 检查是否半包，最后 destroy。需要调整限额时先 config_init；用户无须安装 xs 或链接 xrt 即可使用。

测试文件：[单元测试](/D:/GIT/xserver/tools/xtp2_test.c)、[模糊测试入口](/D:/GIT/xserver/tools/xtp2_fuzz.c)。

从仓库根目录运行（输出目录可自行指定）：

```sh
gcc -std=c99 -O2 -Wall -Wextra -Werror -pedantic tools/xtp2_test.c -o xtp2_test
./xtp2_test

# Windows 使用仓库自带 TCC：
tcc/tcc.exe -Bres/tcc -Ires/tcc/include_win -Wall -Werror tools/xtp2_test.c -o xtp2_test.exe

# Windows 额外运行旧客户端编码器比较，不会建立网络连接：
gcc -std=c99 -O2 -DXTP2_TEST_LEGACY tools/xtp2_test.c -o xtp2_legacy.exe -lws2_32

# Linux 动态检查与模糊测试：
clang -std=c99 -O1 -g -fsanitize=address,undefined tools/xtp2_test.c -o xtp2_test
mkdir -p xtp2_corpus
./xtp2_test xtp2_corpus
clang -std=c99 -O1 -g -fsanitize=fuzzer,address,undefined tools/xtp2_fuzz.c -o xtp2_fuzz
./xtp2_fuzz xtp2_corpus -runs=100000 -max_len=65536
```

2026-09-05 验证：

- Windows GCC、仓库自带 TCC：单元测试通过，每次超过 20 万次断言检查。
- 历史 `procBuildRequest` 生成的重复参数/空字段报文，与本库编码结果逐字节一致；本库可直接解码。
- C++11 编译实现，再从独立 C99 测试单元链接并运行：通过。
- Linux Clang + AddressSanitizer/UndefinedBehaviorSanitizer：单元测试通过。
- 带上述动态检测的 libFuzzer：从三个固定报文种子完成 100000 轮，未发现崩溃或动态检测错误。

单元测试还覆盖所有小型样本拆分点、4000 组随机有效报文、单字节与随机分片、多包后缀、分配故障与泄漏核对、65535 个参数、总长 u32 溢出、默认 1 MiB 包长边界及其分片接收。动态检测和有限轮次 fuzz 不是对所有输入或平台的穷尽证明；32 位、大端平台、真实 TCP/TLS 发送队列和 xs 热重载集成尚未验证。
