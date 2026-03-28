# xserver 与 old/legacy 对照

本文档只回答一个问题：

- 生产版 `xs` 现在到底保留了多少“不是 old/legacy 原生就有”的内建能力



## 1. 对照结论

按当前代码库直接扫描，`old/legacy` 目录里没有发现今天这套 `__xs/*` 内建管理面。

这意味着：

- `old/legacy` 的核心定位更接近“薄宿主 + 协议回调 + 脚本处理”
- 当前生产版 `xs` 保留的 `__xs` 管理面，本质上都是在 legacy 基线之上的额外能力

因此，当前 production `xs` 的设计目标不是“复刻所有调试面”，而是：

- 维持 legacy 那种薄宿主定位
- 只额外保留少量对实际交付必要的运行入口



## 2. 当前 production xs 允许保留的额外内建面

当前 production `xs` 只额外保留以下核心管理入口：

- `status / status_json`
- `health / health_json`
- `reload / reload_json`
- `reload_config / reload_config_json`
- `reload_status / reload_status_json`
- `check_config / check_config_json`

保留它们的原因只有三个：

- 需要最小运行态自检
- 需要动态脚本/配置重载
- 需要为下游项目提供稳定可依赖的最小运维面



## 3. 明确不属于 production xs 的能力

以下能力不再视为 production `xs` 默认承诺，而是归到 `xsdbg`：

- `dashboard`
- `bus`
- 协议 `*_metrics*`
- 各种 `*_clear / *_reset`
- 更重的治理、看板、统计和在线调试入口

这样做的目的不是否定这些能力，而是把它们从“运行链路默认负担”中移出去。



## 4. 后续准入规则

后续如果要新增任何内建 `__xs/*` 入口，默认规则应当是：

1. 先放到 `xsdbg`
2. 只有在确实属于 production `xs` 最小稳定面时，才允许进入 production
3. 一旦进入 production，必须同时更新：
	- `docs/稳定API.md`
	- `test_stable.bat`
	- `test_stable.sh`
	- 对应 smoke 脚本

换句话说：

- 新增内建面不是“写完 handler 就算完成”
- 而是“只有进入稳定契约并被 smoke 锁住，才算允许进入 production xs”



## 5. 当前建议

当前阶段，production `xs` 应继续坚持：

- 不再新增内建调试面
- 不再把治理/看板能力塞回 production
- 优先保证旧版可用性、动态重载、XTP 高级接口、以及稳定 API 基线

如果未来需要更强的在线观测与治理，优先继续放在 `xsdbg`，再慢慢验证是否有必要收编进 production。



## 6. 相关文档

- 稳定 API：`docs/稳定API.md`
- 发布检查：`docs/发布检查清单.md`
- 运行补记：`docs/运行与稳定补记.md`
- 验证入口：`test_stable.bat`、`test_stable.sh`
