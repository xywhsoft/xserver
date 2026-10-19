# xrudp STUN 映射服务

独立的 C/XRT 前台服务，使用 xs 锁定的 XRT 与 xrudp STUN 编解码器。运行时没有新增第三方依赖，也不加载网站或账号数据。mdo 从本地可信的账号服务域名推导 UDP 3478，在自己的 xrudp 套接字上查询映射；映射地址只是候选，账号授权仍由认证中转和双方公钥绑定的 Noise 握手决定。

只支持 RFC 8489 格式的无认证 Binding 请求：返回真实 UDP 来源的 XOR-MAPPED-ADDRESS；校验并保持可选的 FINGERPRINT；未知必需属性返回有限的 420。拒绝旧 STUN、重定向、TURN、完整性未验证的请求、畸形包和非请求类型。Binding indication 不产生回复。

默认同时监听 IPv4/IPv6 UDP 3478。共享 256 次/秒令牌预算，IPv4 地址或 IPv6 /64 各有 16 次/秒预算；改变来源端口不能绕过配额。固定 256 组、每组四路来源记录，带随机 SipHash 索引，活动记录不会被碰撞请求驱逐，60 秒空闲后可复用。配额也限制解析工作，不仅限制成功回复。每次请求最多 512 字节、64 个属性，回复不超过 80 字节且不超过请求的三倍，目的地只能是内核给出的来源。

每个 UDP 端点最多 32 个/4096 字节待发包、两个 512 字节在途接收，禁用额外接收队列。单个网络 worker 共享来源和全局预算；Engine 命令、操作、缓存和定时器均有硬上限。统计输出在主线程执行，锁内只规划回复与复制计数，持锁时不调用可能同步触发错误回调的网络操作。每分钟及退出时只输出聚合计数，不记录地址、事务号、密钥或账号令牌。

在 xs 仓库构建和测试：

```text
python tools/build_stun.py --cc gcc --static --output .build/xrudp-stun-linux/xrudp-stun
python tools/test_stun_server.py --exe .build/xrudp-stun-linux/xrudp-stun
python tools/build_stun.py --cc clang --sanitize --output .build/xrudp-stun-asan/xrudp-stun
python tools/test_stun_server.py --exe .build/xrudp-stun-asan/xrudp-stun
```

Windows 省略 `--static`，使用 MinGW GCC。构建先校验 xs 的原始上游快照，结束前再次核对编译输入，输出附带源文件及二进制哈希的收据。Python 只用于构建与独立网络测试，运行服务只需要原生可执行文件。

`xrudp-stun --version` 不创建监听，直接报告构建时的 XS、XRUDP 扩展和 XRT 核心提交。
单独升级扩展时保留核心的独立版本，部署核验应同时比对该输出与构建收据，不能把扩展提交当成核心提交。

`--bind4`/`--bind6` 只接受数值地址；`--port 0` 可用于隔离测试，实际端口从启动输出获取。`--no-v4`/`--no-v6` 可关闭一个地址族。`--rate`/`--per-source` 范围为 1..4096，来源配额不能大于全局配额。`--run-ms` 提供最长十分钟的有限诊断运行；正常服务不设置此选项。SIGINT/SIGTERM 取消 UDP 并等待 Engine 退出；启动绑定冲突会失败，不占用已有服务。

提供的 systemd 用户单元将可执行文件放在 `~/.local/libexec/xrudp-stun`，不使用 root，限制内存、CPU、线程和句柄，失败时有限重启。需要用户管理器和已启用的 linger 才能在系统重启后自动运行。启用前核对端口、旧单元/可执行文件哈希，原子安装并保留旧版本；不要替换网站宿主。公网需要允许 UDP 3478，IPv6 还需要可路由的 IPv6 地址。

验收包含两平台真实 UDP、独立 Python XOR/CRC 解码、错误与静默丢弃、来源端口变换、跨族全局配额、补充令牌、独占绑定和关闭。Linux 另外运行 ASan/UBSan/泄漏检查以及 SIGTERM。此组件通过不代表双机公网直连、Android、全 NAT 类型或 72 小时长稳已完成。
