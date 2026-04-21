# xserver 与 old/legacy 对照

本文档只回答一个问题：

- 生产版 `xs` 现在到底保留了多少“不是 old/legacy 原生就有”的内建能力



## 1. 对照结论

按当前代码库直接扫描，`old/legacy` 目录里没有发现今天这套 `__xs/*` 内建管理面。

这意味着：

- `old/legacy` 的核心定位更接近“薄宿主 + 协议回调 + 脚本处理”
- 当前 production `xs` 不再保留内建 `__xs` 管理面；相关路径回到应用层

因此，当前 production `xs` 的设计目标不是“复刻所有调试面”，而是：

- 维持 legacy 那种薄宿主定位
- 把 Bus / reload / check_config 保留为运行时能力，而不是固定 HTTP URI



## 2. 当前 production xs 的额外内建面

当前 production `xs` 不再允许保留固定内建 HTTP 管理入口。

以下路径在 production `xs` 下不再是框架契约：

- `status / status_json`
- `health / health_json`
- `reload / reload_json`
- `reload_config / reload_config_json`
- `reload_status / reload_status_json`
- `check_config / check_config_json`

如果应用项目需要这些 URI，应该在应用层自己实现；框架只提供可调用的运行时 API。



## 3. 明确不属于 production xs 的能力

以下能力不再视为 production `xs` 默认承诺：

- `dashboard`
- `__xs/bus/*`
- 协议 `*_metrics*`
- 各种 `*_clear / *_reset`
- 更重的治理、看板、统计和在线调试入口

其中协议调试、错误排查、内存调试和 reload/check 诊断可以继续放在 `xsdbg`；业务控制面、网页控制台和 Bus 管理 URI 应由应用层决定。



## 4. 后续准入规则

后续如果要新增任何内建 `__xs/*` 入口，默认规则应当是：

1. 默认只能先放到 `xsdbg`
2. 不进入 production `xs`，除非它已经被明确认定为框架层最小稳定契约
3. 如果确实进入 production，必须同时更新：
	- `docs/稳定API.md`
	- `test_stable.bat`
	- `test_stable.sh`
	- 对应 smoke 脚本

换句话说：

- 新增内建面不是“写完 handler 就算完成”
- 而是“先证明它属于框架层，而不是应用层”



## 5. 当前建议

当前阶段，production `xs` 应继续坚持：

- 不再新增内建调试面
- 不再把治理/看板能力塞回 production
- 优先保证多协议、Bus 运行时能力、动态重载和异常连接清理

如果未来需要更强的在线观测与治理，优先继续放在 `xsdbg` 或应用层，不再默认收编进 production。



## 6. 相关文档

- 稳定 API：`docs/稳定API.md`
- 发布检查：`docs/发布检查清单.md`
- 运行补记：`docs/运行与稳定补记.md`
- 验证入口：`test_stable.bat`、`test_stable.sh`
