# demo-chat — xs + xllm 流式对话服务

llama.cpp Web UI 风格的流式对话最小应用，演示 xs 服务桥 + xllm 引擎异步调用 +
简版 Agent 循环（工具执行回填）+ 简版会话（消息数组滑窗，无压缩）。

## 运行

```bat
build.bat xllm                 （在仓库根，构建后产物 release\xs.exe）
copy release\xs.exe release\demo-chat\
cd release\demo-chat
set GLM_API_KEY=<你的密钥>      （或写入 xs.json 的 llm.api_key）
xs.exe xs.json
```

浏览器打开 <http://127.0.0.1:9090/>（对话页面）；WS 端点 9091。

端到端测试：`python test_ws.py`（流式 / 工具循环 / 中途停止 / 停后复用四场景）。

## 架构（文档 ch34§34.3 + ch37§37.4 + ch22§22.2 的标准拼装）

```
浏览器(9090 静态) ⇄ WebSocket(9091, ws 服务 + main.c 脚本)
  WsText(引擎线程) ──chat:<text>──▶ 后台线程 xllmClientComplete(带流式回调)
       xllm 回调(xllm 引擎线程) ──▶ 帧队列(g_lock)
  自重排定时器 25ms(xs 引擎线程) ──▶ xrtWsStreamText 逐帧推送
```

- **流式桥**：回调线程只入队，ws 发送只发生在 xs 引擎线程（定时器），符合
  ch37 线程边界纪律；停止按钮经 volatile 标志让回调返回 false 取消调用。
- **Agent 循环**：响应带 tool_calls → 本地执行内置工具（get_time/echo）→
  tool 消息回填历史 → 再调，最多 6 轮。
- **会话**：每连接 xllm_message 数组；估算超 24K token 或超 200 条时保留最近
  20 条滑窗截断（按需求不做压缩）。
- 连接生命周期：WsClose 无在飞回合立即回收；有则由工作线程结束回收。

## 帧协议

浏览器→服务端纯文本：`chat:<text>` / `stop` / `clear`
服务端→浏览器 JSON：`hello/start/delta/thinking/tool_start/tool_result/usage/done/error/cleared`
