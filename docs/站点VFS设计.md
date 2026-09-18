# XServer 站点 VFS（单文件发布）最终设计

状态：设计定稿，待实施
日期：2026-09-16

## 1. 目标与形态

把 `xs.json` 与整个站点（wwwroot、C 源码、模板、证书等）打成一个压缩归档，
**追加**到 xs.exe 末尾。打包后的单个 exe 无参数启动时自动探测归档，从包内
读配置、出文件，实现单文件部署。打包器是 xs 自身的 `pack` 子命令——零额外
分发物，格式版本与运行时同源编译。

```
开发态（不变）                     发布态
┌────────────┐   xs pack site/   ┌─────────────────────────────┐
│ xs.exe     │ ───────────────→  │ site.exe = xs.exe + 应用包   │
│ site/      │   -o site.exe     │  （尾部追加，二进制不动）      │
└────────────┘                   └─────────────────────────────┘
                                 无参数启动 → 探测尾部 → 单文件模式
```

与 TCC 内置 VFS（编译器头资源，编译期嵌 resources.c，LZMA）是两套独立机制，
互不相干；但本设计复用同一 LZMA 编解码代码与格式布局。

## 2. 二进制布局

```
偏移 0                uArchiveStart(=归档头)                 EOF-24      EOF
┌──────────────────┬──────────────┬──────────────┬──────────┬─────────┐
│ 原始 PE/ELF（不动）│ 条目数据区    │ 条目索引区    │ 归档头 64B│ 尾标 24B │
└──────────────────┴──────────────┴──────────────┴──────────┴─────────┘
```

追加式而非嵌入：打包工具无需理解 PE/ELF（尾部追加对两者合法），xs 无需重编。
可反复重打（工具先剥旧包再追加），可 strip 还原。

### 2.1 尾部标记（24B，定长）

| 偏移 | 字段 | 类型 | 说明 |
|---|---|---|---|
| 0 | magic | u64 | `"XSVPACK\0"` |
| 8 | headerOffset | u64 | 归档头绝对偏移（自文件头） |
| 16 | crc32 | u32 | CRC32(尾标前 16B ‖ 归档头 64B) |
| 20 | flags | u32 | 保留（未来签名/加密标志位），v1=0 |

运行时探测：`fseek(自身, -24, SEEK_END)` 读尾标 → 验 magic → 验 CRC →
定位归档头。一步完成，无归档时零开销。

### 2.2 归档头（64B，定长）

| 偏移 | 字段 | 类型 | 说明 |
|---|---|---|---|
| 0 | magic | u64 | `"XSVFHDR\0"` |
| 8 | version | u16 | 格式版本 = 1 |
| 10 | headerSize | u16 | = 64 |
| 12 | entryCount | u32 | 条目数 |
| 16 | dataOffset | u64 | 数据区偏移（相对归档头） |
| 24 | indexOffset | u64 | 索引区偏移（相对归档头） |
| 32 | archiveSize | u64 | 归档总长（归档头到 EOF） |
| 40 | indexCRC32 | u32 | 索引区校验 |
| 44 | flags | u32 | 保留 |
| 48 | reserved0 | u64 | 保留，恒 0 |
| 56 | reserved1 | u64 | 保留，恒 0 |

### 2.3 条目索引（变长记录，按路径字典序，支持二分查找）

| 字段 | 类型 | 说明 |
|---|---|---|
| pathLen | u16 | 路径字节数 |
| path | 变长 | UTF-8，`/` 分隔，规范相对路径；无前导 `/`；禁 `..` 段 |
| method | u8 | 0=STORE，1=LZMA（其余值保留，读到即拒绝） |
| flags | u8 | 保留，v1=0 |
| dataRelOffset | u64 | 数据偏移（相对归档头） |
| compSize | u32 | 压缩后大小（STORE 时 = origSize） |
| origSize | u64 | 原始大小 |
| origCRC32 | u32 | 原始内容校验（解压后验） |

定长部分 28B + 路径。路径大小写敏感（跨平台确定性，Windows 打包 Linux 可跑）。

### 2.4 条目数据（LZMA）

`LZMA_PROPS_SIZE`（5B 属性：lc/lp/pb+dictSize）+ LZMA1 流——与 TCC VFS
条目完全同构，`LzmaDecode()` 一步全解。不按条目对齐（fseek 读取，无需对齐）。

## 3. 压缩策略

- **逐条独立压缩**（随机访问、按需解压，不做整包单流——LZMA1 无块独立性）
- 打包时逐条试压：`compSize >= origSize` 即 STORE（png/jpg/woff2 自然落入）
- v1 仅 LZMA（编码器 `tcc/LzmaEnc.c + LzmaFind.c + CpuArch.c` 共 ~37KB 机器码，
  `-DZ7_ST` 与 tcc_vfs_lzma_pack 同参；解码器 `lzma-dec.o` 已在所有变体链接行）
- 仅 LZMA 方案，不做 zstd（2026-09-16 拍板：demo 语料实测字典理论上限收益 7.9%
  且集中于 <64KB 小文件，字典自身 30~110KB，中小站点净负）

## 4. 运行时设计

### 4.1 启动探测与模式判定（main.c 参数解析后、配置装载前）

```
首参数 = "pack"        → 工具模式：执行打包/列表/解包/strip，退出
显式 config 参数       → 传统目录模式（包若存在也不启用——语义干净）
无参数                 → 读自身末尾 24B：
  ├─ 合法尾标 → 单文件模式：装载索引；xs.json 从包内读（XS_ConfigLoadMem）
  └─ 无       → 现状不变（appPath/xs.json）
```

### 4.2 三层内存模型

| 层 | 进内存时机 | 常驻 | demo 实测 |
|---|---|---|---|
| 索引 | 启动装载一次 | ✅ 不可变（天然线程安全，无锁并发查表） | 30 条 ≈ 3KB |
| 压缩数据区 | **不主动进内存** | ❌ 留在 exe 文件内，按偏移 seek+read | 磁盘 576KB |
| 条目内容 | **首次访问惰性解压** | ✅ 缓存常驻不淘汰 | 全站 1.76MB 封顶 |

- 压缩数据靠 OS 文件缓存管热度（索引字典序 = 同目录文件数据区相邻，局部性好）
- 冷文件（无人访问的页面）永不占内存；首次访问多 ~1ms 级解压延迟
- v1 不做 LRU（站点量级用不上）；归档头 flags 预留"缓存上限"降级位
- 索引装载时健全性检查：条目数上限（10 万）、单条 origSize 上限（1GB）、
  索引 CRC、偏移越界拒绝——防损坏包

### 4.3 文件读取优先级（单文件模式）

**磁盘(appPath/相对路径) 优先 → VFS 兜底。**（用户约束：外部文件永远赢——
程序写文件后包内文件自然作废，避免证书等场景的陈旧数据 bug）

运维价值：线上改一个 CSS，把修正文件丢到 exe 旁即生效（并参与热重载），
删掉即恢复包内版本。热重载自动退化为仅磁盘覆盖文件。

**Phase 2 已实现**（2026-09-16）：XS_AppReadAll 统一出口（磁盘优先 VFS 兜底，
绝对路径剥 appPath 前缀查包）；TLS 证书三处（cert/key/CA）接入——证书可进包；
--no-vfs 调试开关；http.h 静态层 RootStat-miss 第四入口兜底（空目录场景关键）；
三场景覆盖 e2e（纯包 / 空目录+磁盘覆盖 / 删覆盖恢复）全绿。

### 4.4 HTTP 静态文件整合（src/protocol/http.h:345 XS_HttpRootOpenFile）

现有磁盘流保持不变；VFS 命中走**内存响应路径**：内容已在缓存、
Content-Length 索引已知，内存视图直挂发送缓冲队列（复用错误页/动态响应的
内存体路径）——比文件流还少一次 read。MIME/自定义头/HEAD 逻辑共用。

### 4.5 其余读取点

| 触点 | 现状 | 改动 |
|---|---|---|
| 配置装载 | main.c XS_ConfigLoad(路径) | 新增 XS_ConfigLoadMem(内存,长度) |
| TLS 证书 | src/core/tls.h:487 xrtFileReadAll | 包一层 XS_VfsReadAll（磁盘→VFS） |
| 脚本 devfile | src/script/script.h:191 xrtFileReadAll | 同上——C 源码可进包，真·单文件 |

### 4.6 新模块

`src/core/xs_vfs.h`（头文件实现风格，与契约 API 一致）：
`XS_VfsProbe / LoadIndex / Lookup / ReadAll / CachedView / Unit`
+ 内部 CRC32（提升 qrpng 本地实现为共享静态）+ LZMA 解码（~30 行，
`LzmaDecode` 直调，镜像 tcc_builtin_vfs.c:473）。

打包器：`src/core/xs_pack.h`（工具模式专用，头文件实现），LZMA 编码封装
复用 tcc_vfs_lzma_pack.c 的调用形态。

## 5. `xs pack` 子命令

```
xs pack <站点目录> [-o <输出.exe>]      # 目录须含 xs.json；基底 = 自身 exe
                                        # （若自身已打包，先自动剥旧包）
xs pack --list <exe>                    # 条目清单（路径/方法/大小/CRC）
xs pack --extract <exe> -d <目录>       # 解包
xs pack --strip <exe> -o <原始.exe>     # 还原
```

- `pack` 为保留首参数（先于 config 路径判定拦截，执行完退出，不触碰
  引擎/TLS/拓扑初始化）；--help 用法同步列出
- 重打幂等：对已打包 exe 再 pack = 剥旧 + 追新
- strip 还原的字节与原始 exe 完全一致（追加区整体切除）
- 代码签名必须在打包**之后**（改了字节）；SmartScreen/AV 对改尾 exe 可能
  更敏感，属预期行为
- 开发便利：可选薄封装 tools/pack_site.py 转调子命令（不进发布链，非必需）

## 6. 构建整合（tools/build.py 核心源表）

```python
"lzma-dec":  "tcc/LzmaDec.c",   # 现有
"lzma-enc":  "tcc/LzmaEnc.c",   # 新增，-DZ7_ST
"lzma-find": "tcc/LzmaFind.c",  # 新增
"cpu-arch":  "tcc/CpuArch.c",   # 新增
```

编码器 ~37KB 无条件链入所有变体（含 default）。启动横幅增加一行：
`[xs] app pack: 30 entries, 576.0 KB (lzma 26, store 4)`（无包不打印）。
CLI 调试开关 `--no-vfs`：忽略包强制目录模式。

## 7. 安全

- 路径穿越：索引路径打包时规范校验（拒 `..`/绝对/反斜杠）；运行时查找同样
  归一化拒越界（沿用 xroot 逃逸检查哲学）
- 完整性：尾标 CRC → 归档头 → 索引 CRC → 逐条解压后 origCRC，四级校验
- 健全性：条目数/单条大小/偏移范围上限，防损坏包撑爆内存
- 包内容对宿主不可信场景无影响（站点资产本就公开服务；xs.json 属部署者自有）

## 8. 测试计划

- **格式单测**：pack→probe→list→extract→diff 往返；四级 CRC 各翻一位的
  拒绝；`..` 路径拒绝；重打幂等；strip 字节级还原
- **端到端**：pack demo-single（除 exe/db）→ 起服务 → HTTP 断言：包内页
  200 且内容一致、磁盘覆盖文件优先、缺席 404、HEAD/MIME 正确
- **运行期探针**：test_extensions_runtime 增加 VFS 激活断言（横幅行 +
  config-from-VFS）
- `-O2` 与 all/default 变体回归

## 9. 分期

| 期 | 内容 | 量 |
|---|---|---|
| **1 最小闭环** | 归档格式 + CRC + xs pack 四命令 + 探测/索引装载 + XS_ConfigLoadMem + HTTP 静态 VFS 兜底（demo-single 端到端） | ~2 天 |
| **2 全触点** | TLS 证书/devfile/XS_VfsReadAll 统一出口 + 磁盘覆盖优先级贯通 + 横幅 + --no-vfs | ~1 天 |
| **3 收尾** | 单测/探针/e2e 全量 + QUICKSTART/官网文档 + make_release 可选打包产出 | ~1 天 |

## 10. 明确不做（防膨胀）

- 包内写入/热更新包（修改路径 = 磁盘覆盖或重打）
- 加密/签名（flags 位预留）
- LRU 淘汰（配置位预留）
- 解压到临时目录（零落盘）
- 整包单流压缩（牺牲随机访问）
