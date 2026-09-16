/* xacme public surface: account/store config, DNS provider registry (five
 * built-ins: ali/cf/tencent/aws/huawei), opaque client API (xrt/acme_client.h),
 * one-shot obtain (xrt/acme_obtain.h) and flow entry points (src/internal/
 * xacme_flow.h, exposed to scripts via VFS under xacme/). Deeper internals
 * stay native-only. */

/* account config & CA presets */
XS_XACME_SYMBOL(xrtAcmeAccountConfigInit)

/* DNS provider interface + registry（五家内建） */
XS_XACME_SYMBOL(xrtAcmeDnsProviderValidate)
XS_XACME_SYMBOL(xrtAcmeDnsProviderCount)
XS_XACME_SYMBOL(xrtAcmeDnsProviderId)
XS_XACME_SYMBOL(xrtAcmeDnsAliConfigInit)
XS_XACME_SYMBOL(xrtAcmeDnsAli)
XS_XACME_SYMBOL(xrtAcmeDnsAliProviderUnit)
XS_XACME_SYMBOL(xrtAcmeDnsCfConfigInit)
XS_XACME_SYMBOL(xrtAcmeDnsCf)
XS_XACME_SYMBOL(xrtAcmeDnsCfProviderUnit)
XS_XACME_SYMBOL(xrtAcmeDnsTencentConfigInit)
XS_XACME_SYMBOL(xrtAcmeDnsTencent)
XS_XACME_SYMBOL(xrtAcmeDnsTencentProviderUnit)
XS_XACME_SYMBOL(xrtAcmeDnsAwsConfigInit)
XS_XACME_SYMBOL(xrtAcmeDnsAws)
XS_XACME_SYMBOL(xrtAcmeDnsAwsProviderUnit)
XS_XACME_SYMBOL(xrtAcmeDnsHuaweiConfigInit)
XS_XACME_SYMBOL(xrtAcmeDnsHuawei)
XS_XACME_SYMBOL(xrtAcmeDnsHuaweiProviderUnit)

/* certificate/account/grant store */
XS_XACME_SYMBOL(xrtAcmeStoreSaveAccount)
XS_XACME_SYMBOL(xrtAcmeStoreLoadAccount)
XS_XACME_SYMBOL(xrtAcmeStoreSaveCert)
XS_XACME_SYMBOL(xrtAcmeStoreLoadCert)
XS_XACME_SYMBOL(xrtAcmeStoreLoadCertCa)
XS_XACME_SYMBOL(xrtAcmeStoreNeedRenew)
XS_XACME_SYMBOL(xrtAcmeStoreSaveGrant)
XS_XACME_SYMBOL(xrtAcmeStoreLoadGrant)
XS_XACME_SYMBOL(xrtAcmeStoreListDomains)
XS_XACME_SYMBOL(xrtAcmeGrantUnit)

/* 不透明客户端（xrt/acme_client.h：宿主的标准用法） */
XS_XACME_SYMBOL(xrtAcmeClientConfigInit)
XS_XACME_SYMBOL(xrtAcmeClientCreate)
XS_XACME_SYMBOL(xrtAcmeClientDestroy)
XS_XACME_SYMBOL(xrtAcmeClientAccountPem)
XS_XACME_SYMBOL(xrtAcmeClientIssue)
XS_XACME_SYMBOL(xrtAcmeClientIssueEx)
XS_XACME_SYMBOL(xrtAcmeClientRevoke)
XS_XACME_SYMBOL(xrtAcmeClientRollover)
XS_XACME_SYMBOL(xrtAcmeClientDeactivate)
XS_XACME_SYMBOL(xrtAcmeClientIssueStored)

/* 一站式申领（xrt/acme_obtain.h） */
XS_XACME_SYMBOL(xrtAcmeObtainConfigInit)
XS_XACME_SYMBOL(xrtAcmeObtain)

/* 签发流程入口（src/internal/xacme_flow.h，VFS xacme/ 路径） */
XS_XACME_SYMBOL(xacmeClientInit)
XS_XACME_SYMBOL(xacmeClientUnit)
XS_XACME_SYMBOL(xacmeClientAccountPem)
XS_XACME_SYMBOL(xacmeClientIssue)
XS_XACME_SYMBOL(xacmeClientIssueStored)
XS_XACME_SYMBOL(xacmeClientRevoke)
XS_XACME_SYMBOL(xacmeClientRollover)
XS_XACME_SYMBOL(xacmeClientDeactivate)
