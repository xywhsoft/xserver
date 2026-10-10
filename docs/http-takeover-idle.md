# HTTP 接管连接的空闲策略

`XS_TAKEOVER` 把后续读写交给应用；HTTP 驱动的 Read/Writable 回调不再执行，原有的活跃时间也不再更新。继续用这个时间清理连接会关闭健康的 WebSocket 或异步响应。打开 `idle_timeout` 时，即使应用持续发送心跳，也会触发这一问题。

接管成功后，连接注册表以原子标记移交空闲策略。HTTP 空闲扫描跳过该连接，但它仍留在注册表中：宿主重载、服务停机、Close 回调和脚本引用释放均保持原生命周期。应用负责为其协议设置心跳、空闲、发送和关闭期限。普通 HTTP keepalive 和 TCP 服务仍采用原来的空闲规则。

验证命令：

```text
python tools/test_http_takeover_idle.py --exe <xs executable>
```

测试将 HTTP 空闲期限设为 1 秒，接管响应暂停 2.5 秒后继续发送；同时验证普通 HTTP keepalive 仍会被清理。TCP 和 TLS 分别验证。旧 ed8a0ad 宿主在接管响应阶段失败，修正宿主两项通过。另以 xadmin 的真实托管 WebSocket 验证明文/TLS 接管、插件重载、宿主重载、会话撤销及唯一关闭回调。都是低并发、有界功能测试，不产生压力负载。
