/* xacme public surface: 13 xrtAcme* (include/xrt/acme*.h) + 5 flow
 * entry points (src/internal/xacme_flow.h, exposed to scripts via VFS
 * under xacme/). Deeper internals stay native-only. */

/* account config & CA presets */
XS_XACME_SYMBOL(xrtAcmeAccountConfigInit)

/* DNS provider interface + registry */
XS_XACME_SYMBOL(xrtAcmeDnsProviderValidate)
XS_XACME_SYMBOL(xrtAcmeDnsProviderCount)
XS_XACME_SYMBOL(xrtAcmeDnsProviderId)
XS_XACME_SYMBOL(xrtAcmeDnsAliConfigInit)
XS_XACME_SYMBOL(xrtAcmeDnsAli)
XS_XACME_SYMBOL(xrtAcmeDnsAliProviderUnit)

/* certificate/account store */
XS_XACME_SYMBOL(xrtAcmeStoreSaveAccount)
XS_XACME_SYMBOL(xrtAcmeStoreLoadAccount)
XS_XACME_SYMBOL(xrtAcmeStoreSaveCert)
XS_XACME_SYMBOL(xrtAcmeStoreLoadCert)
XS_XACME_SYMBOL(xrtAcmeStoreLoadCertCa)
XS_XACME_SYMBOL(xrtAcmeStoreNeedRenew)

/* issuance flow (client API) */
XS_XACME_SYMBOL(xacmeClientInit)
XS_XACME_SYMBOL(xacmeClientUnit)
XS_XACME_SYMBOL(xacmeClientAccountPem)
XS_XACME_SYMBOL(xacmeClientIssue)
XS_XACME_SYMBOL(xacmeClientIssueStored)
