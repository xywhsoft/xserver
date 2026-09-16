/* xoauth2 public surface (RFC 6749/7636): authorize-code flow + PKCE +
 * refresh + provider presets + OIDC helpers (data-coupled with xjwt).
 * Transport is caller-injected via xoauth2httpproc; xoauth2HttpXrt* is the
 * xrt net/tls/http1 convenience implementation. */

/* 配置 / 预设 */
XS_XOAUTH2_SYMBOL(xoauth2ConfigInit)
XS_XOAUTH2_SYMBOL(xoauth2UseGithub)
XS_XOAUTH2_SYMBOL(xoauth2UseGoogle)
XS_XOAUTH2_SYMBOL(xoauth2UseWechat)
XS_XOAUTH2_SYMBOL(xoauth2UseMicrosoft)
XS_XOAUTH2_SYMBOL(xoauth2UseCustom)
XS_XOAUTH2_SYMBOL(xoauth2ClientUnit)

/* 登录流程 */
XS_XOAUTH2_SYMBOL(xoauth2BeginLogin)
XS_XOAUTH2_SYMBOL(xoauth2CompleteLogin)
XS_XOAUTH2_SYMBOL(xoauth2TokenFree)

/* token 管理 */
XS_XOAUTH2_SYMBOL(xoauth2Refresh)
XS_XOAUTH2_SYMBOL(xoauth2TokenExpiring)

/* OIDC / 通用 HTTP 辅助（数据耦合层，不依赖 xjwt） */
XS_XOAUTH2_SYMBOL(xoauth2HttpGet)
XS_XOAUTH2_SYMBOL(xoauth2NonceConsume)
XS_XOAUTH2_SYMBOL(xoauth2GetUserInfo)

/* 离线工具 */
XS_XOAUTH2_SYMBOL(xoauth2PkceGenerate)
XS_XOAUTH2_SYMBOL(xoauth2StateGenerate)
XS_XOAUTH2_SYMBOL(xoauth2UrlEncode)

/* 错误便捷读取 */
XS_XOAUTH2_SYMBOL(xoauth2LastError)

/* 便捷传输：xrt net/tls/http1（栈版 + 堆版，后者为脚本层正路） */
XS_XOAUTH2_SYMBOL(xoauth2HttpXrtInit)
XS_XOAUTH2_SYMBOL(xoauth2HttpXrtUnit)
XS_XOAUTH2_SYMBOL(xoauth2HttpXrtCreate)
XS_XOAUTH2_SYMBOL(xoauth2HttpXrtDestroy)
XS_XOAUTH2_SYMBOL(xoauth2HttpXrt)
