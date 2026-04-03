# xserver examples

当前 `examples` 下的每个子目录都是独立运行环境，目录内都带了 `xs / xs.exe / xsdbg.exe / tcc` 运行时文件。

端口规划如下：

- `http`：HTTP `18080`
- `ws`：HTTP `18180`，WebSocket `18181`
- `xtp`：HTTP `18280`，XTP `18294`
- `tcp`：HTTP `18380`，TCP `18392`
- `udp`：HTTP `18480`，UDP `18493`
- `custom`：HTTP `18580`，Custom `18591`

启动方式：

- Windows：进入对应目录后运行 `xs.exe`
- Linux：进入对应目录后运行 `./xs`

附带工具：

- `xtp/tools/push_log.py`
- `tcp/tools/tcp_peer.py`
- `udp/tools/udp_chat.py`

说明：

- 你原始需求里第二个 `tcp` 我按总列表解释成了 `udp` 聊天例子。
