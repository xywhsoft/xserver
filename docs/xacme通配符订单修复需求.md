# xacme 通配符订单修复需求（xrt 主线 extlibs/xacme）

> 2026-09-28 · 由 xs 侧 ACME 自动续签守护（ffeefac）staging 实测发现
> 修改位置：`D:\GIT\xrt\extlibs\xacme\src\acme\xacme_flow.c`（约 3 处，~30 行）
> 验收方式见文末。

## 一、问题描述

`xacmeFlowIssue` 构建新订单时把 `*.example.com` 形式的通配符域名**剥掉 `*.` 前缀后与裸域去重**，
导致订单 identifier 只含基础域；而 CSR 的 SAN 是**全量原样（含通配符）**（flow.c L1245 起，
`Csr.Domains = pDomains` 未做任何剥离）。两者不一致。

ACME 协议（RFC 8555 §7.4 + LE 实现）要求：

1. **通配符必须在订单 identifier 中表达**：`{"type":"dns","value":"*.example.com"}`，
   CA 据此返回"通配符授权"（authz 的 `identifier.value` 是基础域 `example.com`，挑战为 DNS-01）；
2. **CSR 的 identifiers 必须与订单完全一致**（finalize 时逐项比对）。

现状：订单 `[xywhsoft.com]` vs CSR `[xywhsoft.com, *.xywhsoft.com]` → finalize 必然 403。

代码位置（flow.c，行号基于当前主线）：

- **L897-938**：注释自述错误假设——`/* 1. 新订单；identifier 用去 *.\ 后的基础域并去重
  （通配符与裸域共用一次授权；通配符语义由 CSR 的 SAN 表达）。 */`
  循环内 L918-923 对 `*.` 前缀做了 `Domain.Data += 2; Domain.Size -= 2;`。
- **L1128-1145**：DNS-01 TXT 属主用 `Bases[i]`（剥离后的基础域数组）拼接
  `_acme-challenge.<Bases[i]>`。

## 二、重现方法

1. 环境任意（xs 侧已在显卡服务器实测复现）：xACME 走 **Let's Encrypt staging**，
   域名列表 `["xywhsoft.com", "*.xywhsoft.com"]`，DNS-01 阿里云 provider；
2. 调 `xrtAcmeObtain`（或 `xrtAcmeClientIssue`）；
3. 观察：newOrder 成功（201）、账户/DNS-01 挑战全部通过、
   **finalize 返回 403**，body：

```json
{
  "type": "urn:ietf:params:acme:error:unauthorized",
  "detail": "Error finalizing order :: CSR does not specify same identifiers as Order",
  "status": 403
}
```

实测日志（GPU 服务器 /opt/www，2026-09-28 15:45）：

```
[xs] acme 'xywhsoft': obtain failed: acme finalize status=403 body={
  "type": "urn:ietf:params:acme:error:unauthorized",
  "detail": "Error finalizing order :: CSR does not specify same identifiers as Order",
  ...}
```

注意：**只有裸域列表**（如 `["xxdevs.com"]`）时流程完全正常——本 bug 仅在
同一签发里出现通配符域名（无论是否与裸域配对）时触发。

## 三、修复方案

### 改动 1：订单 identifier 保留完整域名（L897-938）

- 删除 `*.` 前缀剥离逻辑（L918-923 的 `if Domain[0]=='*' && Domain[1]=='.'` 分支）；
- 去重改为**完整字符串去重**（`*.example.com` 与 `example.com` 是两个不同 identifier，都保留）；
- 注释改为：

```c
/* 1. 新订单；identifier 用完整域名（通配符原样：*.example.com 是独立
   identifier，CA 依此返回通配符授权）。去重按完整字符串。 */
```

### 改动 2：TXT 属主改从授权返回的 identifier.value 解析（L1128-1145）

协议正源：authz JSON 自带 `identifier:{type:"dns",value:"<基础域>"}`——
通配符授权的 value 已经是去掉 `*.` 的基础域，**直接用它**拼 TXT 属主，
不再依赖与订单 identifier 的位置映射（`Bases[i]`）：

```c
/* TXT 属主 = _acme-challenge.<授权 identifier.value>（通配符授权
   返回的 value 已是基础域；协议正源，不依赖位置映射） */
{
	xbuffer Fqdn;
	xvalue* pIdent = xrtValueObjectGet(
		pAuthRoot, XRT_STR_LITERAL("identifier"));
	xvalue* pValue;
	xstrview Domain = { NULL, 0 };

	if ( pIdent != NULL &&
	     (pValue = xrtValueObjectGet(
	         pIdent, XRT_STR_LITERAL("value"))) != NULL ) {
		(void)xrtValueGetString(pValue, &Domain);
	}
	if ( Domain.Data == NULL || Domain.Size == 0u ) {
		/* 兜底：个别 CA 不回 identifier 时退回订单侧完整域名 */
		Domain = (xstrview){ Bases[i].sData, Bases[i].iSize };
	}
	/* 原有 _acme-challenge.<Domain> 拼接保持不变 */
	...
}
```

（`pAuthRoot` 在挑战循环前已由 authz GET 的 body 解析得到，作用域可见。）

### 改动 3：不需要动的部分（确认即可）

- **CSR**（L1245-1257）：SAN 全量含通配符，本来就是对的，保持；
- **授权循环**（L1011 起）：`xywhsoft.com` + `*.xywhsoft.com` 会得到两个 authz，
  两个 DNS-01 挑战的 TXT 属主相同（`_acme-challenge.xywhsoft.com`）——
  流程是逐个"铺 TXT→传播→触发→轮询→删 TXT"，顺序执行无重叠；
  已 valid 的授权已有跳过分支（L1076 `bValid → continue`）；
- **xacmeJsonQuoteAppend 写 identifier**（L954）：`*` 是合法 JSON 字符串字符，无需转义处理；
- **store/续签判断**：按主域名（列表首个）存取，不受影响。

### 边界与回归点

1. 纯裸域列表行为不变（无 `*.` 时去重与原逻辑等价）；
2. 单独通配符 `["*.example.com"]`：订单一个 identifier、一个 authz（value=基础域）、
   CSR 一个 SAN——修复后应全程通过；
3. LE 返回的 authorizations 数量可能与 identifier 数不一致（复用已 valid 授权时）——
   改动 2 之后循环不再依赖 `i` 与订单数组的位置对应，天然安全；
4. 域名数组上限 16（`Bases[16]`）不变。

## 四、验收标准

1. staging + `["xywhsoft.com", "*.xywhsoft.com"]` 全流程绿：newOrder 201 →
   两个 DNS-01 挑战 valid → finalize **201** → 证书链含两个 SAN
   （`openssl x509 -in cert.pem -noout -text` 可见 `DNS:xywhsoft.com, DNS:*.xywhsoft.com`）；
2. 纯裸域列表回归不破（可用任一单域 staging 复测）；
3. xs 侧我在修复合入后重跑 `acme-store` 清空后的 staging 续签（我会跟进）。

## 五、后续需求（另一个独立改动，非本 bug）：CA 回退链

当前 `xacmeaccountconfig` 单 directory URL，无回退。建议 xrt 侧不改（保持库单一职责），
**在 xs 宿主层（src/core/acme.h，已留好扩展位）实现**：

```json
"acme": {
  "directories": [
    { "url": "https://acme-v02.api.letsencrypt.org/directory" },
    { "url": "https://acme.litessl.com/acme/v2/directory",
      "eab_kid_file": "secrets/litessl-kid.txt",
      "eab_hmac_file": "secrets/litessl-hmac.txt" }
  ]
}
```

逻辑：`XS_AcmeRunGroup` 内对 directories 顺序尝试 `xrtAcmeObtain`，
某 CA 失败（网络/限流/审核拒绝）自动落下一个；xacme 的 store 已按
`accounts/<ca16>/` 分 CA 存账户，天然支持多 CA 并存；EAB 走文件引用不进
xs.json（litessl 需要 EAB 注册，账户配置已有 `xacmeeab` 字段可直接透传）。
预计 ~50 行（含每 CA 的 eab 文件读取与失败日志）。
