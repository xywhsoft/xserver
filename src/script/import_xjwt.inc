/* xjwt public surface (RFC 7519/7515/7517): sign/verify/decode/JWKS/key cache.
 * claims travel as xrt xvalue*, so the xrtValue* family (import_xrt.inc)
 * is a runtime companion dependency for scripts. */

/* 校验参数与错误便捷读取 */
XS_XJWT_SYMBOL(xjwtCheckInit)
XS_XJWT_SYMBOL(xjwtLastError)

/* 签发：一步式（HS/RS/ES 三族七规格，不注入 iss/aud） */
XS_XJWT_SYMBOL(xjwtHs256)
XS_XJWT_SYMBOL(xjwtHs384)
XS_XJWT_SYMBOL(xjwtHs512)
XS_XJWT_SYMBOL(xjwtRs256)
XS_XJWT_SYMBOL(xjwtRs384)
XS_XJWT_SYMBOL(xjwtRs512)
XS_XJWT_SYMBOL(xjwtEs256)

/* 签发：config 式（注入 iss/aud/sub/jti/exp/iat + header kid） */
XS_XJWT_SYMBOL(xjwtConfigInit)
XS_XJWT_SYMBOL(xjwtSign)

/* 验证：PEM / 公钥缓存 / JWKS 三路 */
XS_XJWT_SYMBOL(xjwtVerify)
XS_XJWT_SYMBOL(xjwtKeyParse)
XS_XJWT_SYMBOL(xjwtKeyFree)
XS_XJWT_SYMBOL(xjwtVerifyKey)
XS_XJWT_SYMBOL(xjwtJwksParse)
XS_XJWT_SYMBOL(xjwtJwksFree)
XS_XJWT_SYMBOL(xjwtVerifyJwks)

/* 解码与 claims 工具 */
XS_XJWT_SYMBOL(xjwtDecode)
XS_XJWT_SYMBOL(xjwtDecodeHeader)
XS_XJWT_SYMBOL(xjwtClaimsValid)
XS_XJWT_SYMBOL(xjwtClaimString)

/* 算法名与枚举互转 */
XS_XJWT_SYMBOL(xjwtAlgName)
XS_XJWT_SYMBOL(xjwtAlgParse)
