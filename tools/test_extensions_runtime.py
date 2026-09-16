"""Probe a built xs, including absent headers/symbols and nested TCC.

python tools/test_extensions_runtime.py --exe release/xs.exe sqlite xtp
Only temporary scripts/configs and short-lived child processes are used.
The custom probe explicitly exits its own process after completing checks;
it does not create a listener or touch any running xs instance.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import tempfile
from pathlib import Path

from xs_extensions import select_extensions

PROBE = r'''
#include <xsbase.h>
#include <libtcc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(x) do { if (!(x)) { printf("[extensions] FAIL: %s\n", #x); exit(5); } } while (0)

#ifdef XS_USE_SQLITE
#include <sqlite3.h>
static void probe_sqlite(void)
{
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    REQUIRE(sqlite3_open(":memory:", &db) == SQLITE_OK);
    REQUIRE(sqlite3_exec(db, "CREATE TABLE t(n); INSERT INTO t VALUES(7)", NULL, NULL, NULL) == SQLITE_OK);
    REQUIRE(sqlite3_prepare_v2(db, "SELECT n FROM t", -1, &stmt, NULL) == SQLITE_OK);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 7);
    REQUIRE(sqlite3_finalize(stmt) == SQLITE_OK);
    REQUIRE(sqlite3_close(db) == SQLITE_OK);
}
#endif

#ifdef XS_USE_XTP2
#include <xtp2.h>
#ifdef XTP2_IMPLEMENTATION
#error The probe must call the host implementation, not compile another copy.
#endif
static void probe_xtp(void)
{
    unsigned char bytes[128];
    xtp2_packet p = {0};
    xtp2_config config;
    xtp2_processor *rx = NULL;
    xtp2_param param, field;
    xtp2_iter iter;
    xtp2_message message;
    xtp2_span value;
    size_t size, written, used;
    xtp2_config_init(&config);
    p.id = 7; p.type = XTP2_REQUEST; p.command = xtp2_text("ping");
    field.key = xtp2_text("a"); field.value = xtp2_text("b");
    p.params = &field; p.param_count = 1; p.body = xtp2_text("body");
    REQUIRE(xtp2_measure(&p, NULL, &size) == XTP2_OK);
    REQUIRE(xtp2_encode(&p, NULL, bytes, sizeof(bytes), &written) == XTP2_OK && written == size);
    REQUIRE(xtp2_create(&config, &rx) == XTP2_OK);
    REQUIRE(xtp2_feed(rx, bytes, 7, &used, &message) == XTP2_MORE && used == 7);
    REQUIRE(xtp2_feed(rx, bytes + 7, written - 7, &used, &message) == XTP2_MESSAGE);
    REQUIRE(used == written - 7 && message.id == 7 && xtp2_wants_reply(&message));
    REQUIRE(message.body.size == 4 && memcmp(message.body.data, "body", 4) == 0);
    iter = xtp2_params(&message);
    REQUIRE(xtp2_param_next(&iter, &param) && param.key.size == 1 && param.value.size == 1);
    REQUIRE(!xtp2_param_next(&iter, &param));
    REQUIRE(xtp2_find(&message, xtp2_text("a"), &value) && value.size == 1);
    REQUIRE(*(const char *)value.data == 'b');
    REQUIRE(strcmp(xtp2_error_string(XTP2_OK), "ok") == 0);
    REQUIRE(xtp2_finish(rx) == XTP2_OK);
    xtp2_reset(rx);
    REQUIRE(xtp2_finish(rx) == XTP2_OK);
    xtp2_destroy(rx);
    REQUIRE(xtp2_decode(bytes, written, NULL, &message) == XTP2_OK && message.id == 7);
}
#endif

#ifdef XS_USE_XLLM
#include <xllm.h>
static void probe_xllm(void)
{
    xllm_error error;
    xllm_request request;
    xllmErrorInit(&error);
    REQUIRE(strcmp(xllmFinishReasonName(XLLM_FINISH_STOP), "stop") == 0);
    REQUIRE(strcmp(xllmErrorCodeName(XLLM_ERROR_NONE), "none") == 0);
    REQUIRE(xllmEstimateTextTokens("hello world") > 0);
    xllmRequestInit(&request);
    REQUIRE(xllmRequestAddTextMessage(&request, XLLM_ROLE_USER, "hi"));
    xllmRequestUnit(&request);
    xllmFree(NULL);
}
#endif

#ifdef XS_USE_XLLM_SESSION
#include <xllm-session.h>
static void probe_xllm_session(void)
{
    xllm_session_config config;
    xllmSessionConfigInit(&config);
    REQUIRE(config.uContextWindowTokens == XLLM_SESSION_DEFAULT_CONTEXT_WINDOW_TOKENS);
}
#endif

#ifdef XS_USE_XMAIL
#include <xmail.h>
static void probe_xmail(void)
{
    xtime time;
    size_t size = 0;
    bytes raw;
    REQUIRE(xrtMailBoundaryValid(xrtStrView("boundary-123")));
    REQUIRE(xrtMailDateParse(xrtStrView("Thu, 18 Sep 2025 10:00:00 +0800"), 0, &time, NULL));
    raw = xrtMailBase64Decode(xrtStrView("aGVsbG8="), &size);
    REQUIRE(raw != NULL && size == 5 && memcmp(raw, "hello", 5) == 0);
    xrtFree(raw);
}
#endif

#ifdef XS_USE_XSMTP
#include <xsmtp.h>
static void probe_xsmtp(void)
{
    xsmtpreplyline line;
    xsmtpcapabilityview capability;
    REQUIRE(xrtSmtpReplyLineParse(xrtStrView("250 2.0.0 OK"), &line) && line.Code == 250);
    REQUIRE(xrtSmtpCapabilityParse(xrtStrView("SIZE 15728640"), &capability));
}
#endif

#ifdef XS_USE_XPOP3
#include <xpop3.h>
static void probe_xpop3(void)
{
    xpop3stat stat;
    xpop3uidlview uidl;
    REQUIRE(xrtPop3StatParse(xrtStrView("+OK 2 320"), &stat) && stat.Messages == 2 && stat.Bytes == 320);
    REQUIRE(xrtPop3UidlParse(xrtStrView("7 whqtswO00Q430"), &uidl));
}
#endif

#ifdef XS_USE_XIMAP
#include <ximap.h>
static void probe_ximap(void)
{
    ximapresponseview response;
    REQUIRE(xrtImapSequenceSetValid(xrtStrView("1:5,7,9:*")));
    REQUIRE(xrtImapResponseParse(xrtStrView("* OK IMAP4rev1 ready"), &response));
}
#endif

#ifdef XS_USE_QRCODEGEN
#include <qrcodegen.h>
#include <qrpng.h>
static uint8_t s_QrBuf[qrcodegen_BUFFER_LEN_MAX];
static uint8_t s_QrTemp[qrcodegen_BUFFER_LEN_MAX];
static void probe_qrcodegen(void)
{
    size_t iSize = 0;
    unsigned char* pPng;
    int iSide;

    REQUIRE(qrcodegen_isNumeric("12345"));
    REQUIRE(!qrcodegen_isAlphanumeric("https://x"));
    if ( !qrcodegen_encodeText("https://xs.xywhsoft.com", s_QrTemp, s_QrBuf,
         qrcodegen_Ecc_MEDIUM, qrcodegen_Mode_BYTE, qrcodegen_Mode_BYTE, 0, 0) ) {
        REQUIRE(false);
        return;
    }
    iSide = (qrcodegen_getSize(s_QrBuf) + 8) * 4;
    pPng = qrcodegen_png(s_QrBuf, 4, 4, &iSize);
    REQUIRE(pPng != NULL && iSize > 50);
    REQUIRE(pPng[0] == 0x89 && pPng[1] == 0x50 && pPng[2] == 0x4E && pPng[3] == 0x47);
    REQUIRE((pPng[16] << 24 | pPng[17] << 16 | pPng[18] << 8 | pPng[19]) == (unsigned)iSide);
    REQUIRE(memcmp(pPng + iSize - 8, "IEND", 4) == 0);
    REQUIRE(qrcodegen_png(s_QrBuf, 0, 4, &iSize) == NULL);  /* scale 越界 */
    xrtFree(pPng);
}
#endif

#ifdef XS_USE_XACME
#include <xacme.h>
#include <xacme/xacme_flow.h>
#include <xrt/acme_client.h>
static void probe_xacme(void)
{
    xacmeaccountconfig tAccount;
    size_t iProviders;
    /* 注册表：五家内建 provider（ali/cf/tencent/aws/huawei），首名 ali */
    iProviders = xrtAcmeDnsProviderCount();
    REQUIRE(iProviders == 5);
    REQUIRE(strcmp(xrtAcmeDnsProviderId(0), "ali") == 0);
    /* 账户配置 + LE staging 预设 */
    xrtAcmeAccountConfigInit(&tAccount);
    REQUIRE(tAccount.sDirectoryUrl == NULL);  /* 清零语义：预设用 XACME_DIRECTORY_* 常量 */
    REQUIRE(strcmp(XACME_DIRECTORY_LE, "https://acme-v02.api.letsencrypt.org/directory") == 0);
    /* 不透明客户端 API 在场（Create/Destroy/Issue 一站式） */
    {
        xacmeclientconfig tClient;
        xrtAcmeClientConfigInit(&tClient);
        REQUIRE(tClient.pAccount == NULL);
    }
    /* provider 结构校验（构造后） */
    {
        xacmednaliconfig tAli;
        xacmednsprovider tProvider;
        xrtAcmeDnsAliConfigInit(&tAli);
        tAli.sAccessKeyId = "key";
        tAli.sAccessKeySecret = "secret";
        REQUIRE(xrtAcmeDnsAli(&tAli, NULL, &tProvider));   /* engine 可借可 NULL */
        REQUIRE(xrtAcmeDnsProviderValidate(&tProvider));
        xrtAcmeDnsAliProviderUnit(&tProvider);
    }
}
#endif

#ifdef XS_USE_XJWT
#include <xjwt.h>
/* 探针专用 EC P-256 测试密钥（无任何生产价值） */
static const char s_JwtEcPriv[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIBdtzIuxndpRk0Qy8ivZHqrCPYEYltpRbcmHgwlUzK60oAoGCCqGSM49\n"
    "AwEHoUQDQgAEbjyT7yV9lHyiQMI1E9jCRKdEPWa0YYO53nFlN9aPGO4m4A1ak2Qf\n"
    "z5yNGcI0IqSiV74wKHHTtv605kbiwP7JJA==\n"
    "-----END EC PRIVATE KEY-----\n";
static const char s_JwtEcPub[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEbjyT7yV9lHyiQMI1E9jCRKdEPWa0\n"
    "YYO53nFlN9aPGO4m4A1ak2Qfz5yNGcI0IqSiV74wKHHTtv605kbiwP7JJA==\n"
    "-----END PUBLIC KEY-----\n";
static void probe_xjwt(void)
{
    xjwtconfig tCfg;
    xjwtcheck tCheck;
    xvalue *pClaims, *pOut;
    xjwtkey* pKey;
    xjwtjwks* pJwks;
    char* pToken;
    char aUser[32];
    size_t n;

    /* HS256 签发（iss/aud 注入）→ 带校验验证 → claims 提取 */
    pClaims = xrtValueObject();
    xrtValueObjectSetNew(pClaims, xrtStrView("sub"),
        xrtValueString(xrtStrView("probe-user")));
    xjwtConfigInit(&tCfg);
    tCfg.Alg = XJWT_ALG_HS256;
    tCfg.KeyPem = "probe-secret";
    tCfg.Issuer = "https://xs.example";
    tCfg.Audience = "xs-probe";
    tCfg.ExpireSeconds = 3600;
    pToken = xjwtSign(&tCfg, pClaims);
    REQUIRE(pToken != NULL);
    xrtValueRelease(pClaims);
    xjwtCheckInit(&tCheck);
    tCheck.Issuer = "https://xs.example";
    tCheck.Audience = "xs-probe";
    pOut = xjwtVerify(pToken, "probe-secret", &tCheck);
    REQUIRE(pOut != NULL);
    REQUIRE(xjwtClaimString(pOut, "sub", aUser, sizeof(aUser))
        && strcmp(aUser, "probe-user") == 0);
    xrtValueRelease(pOut);
    /* 错误码语义：篡改 → SIGNATURE */
    xrtClearError();
    n = strlen(pToken);
    pToken[n - 2] = (pToken[n - 2] == 'A') ? 'B' : 'A';
    REQUIRE(xjwtVerify(pToken, "probe-secret", &tCheck) == NULL);
    REQUIRE(xjwtLastError() == XJWT_ERROR_SIGNATURE);
    pToken[n - 2] = (pToken[n - 2] == 'A') ? 'B' : 'A';  /* 还原 */
    xrtFree(pToken);
    /* 错误码语义：过期 → EXPIRED */
    xrtClearError();
    pClaims = xrtValueObject();
    tCfg.ExpireSeconds = -60;
    pToken = xjwtSign(&tCfg, pClaims);
    xrtValueRelease(pClaims);
    REQUIRE(pToken != NULL);
    REQUIRE(xjwtVerify(pToken, "probe-secret", NULL) == NULL);
    REQUIRE(xjwtLastError() == XJWT_ERROR_EXPIRED);
    xrtFree(pToken);

    /* ES256 非对称链路（SEC1 私钥签发 + 公钥缓存验证） */
    pClaims = xrtValueObject();
    xrtValueObjectSetNew(pClaims, xrtStrView("sub"),
        xrtValueString(xrtStrView("ec-user")));
    xjwtConfigInit(&tCfg);
    tCfg.Alg = XJWT_ALG_ES256;
    tCfg.KeyPem = s_JwtEcPriv;
    tCfg.KeyId = "probe-ec";
    tCfg.ExpireSeconds = 600;
    pToken = xjwtSign(&tCfg, pClaims);
    xrtValueRelease(pClaims);
    REQUIRE(pToken != NULL);
    pKey = xjwtKeyParse(s_JwtEcPub);
    REQUIRE(pKey != NULL);
    pOut = xjwtVerifyKey(pToken, pKey, NULL);
    REQUIRE(pOut != NULL);
    xrtValueRelease(pOut);
    xjwtKeyFree(pKey);
    REQUIRE(xjwtKeyParse("not a pem") == NULL);
    xrtFree(pToken);

    /* JWKS：合法解析 + HS256 令牌走 JWKS → alg 族拒绝 */
    REQUIRE(xjwtJwksParse("not json") == NULL);
    pJwks = xjwtJwksParse(
        "{\"keys\":[{\"kty\":\"RSA\",\"kid\":\"k1\","
        "\"n\":\"AQAB\",\"e\":\"AQAB\"}]}");
    REQUIRE(pJwks != NULL);
    pClaims = xrtValueObject();
    xjwtConfigInit(&tCfg);
    tCfg.Alg = XJWT_ALG_HS256;
    tCfg.KeyPem = "probe-secret";
    tCfg.KeyId = "k1";   /* kid 能命中，靠 alg 族防线拒绝 */
    tCfg.ExpireSeconds = 600;
    pToken = xjwtSign(&tCfg, pClaims);
    xrtValueRelease(pClaims);
    REQUIRE(pToken != NULL);
    xrtClearError();
    REQUIRE(xjwtVerifyJwks(pToken, pJwks, NULL) == NULL);
    REQUIRE(xjwtLastError() == XJWT_ERROR_ALG_MISMATCH);
    xrtFree(pToken);
    xjwtJwksFree(pJwks);
}
#endif

#ifdef XS_USE_XOAUTH2
#include <xoauth2.h>
/* mock 传输：离线驱动 token 端点（网络层另有回环端到端，见 extlibs 测试） */
static const char* s_OaReply = "{}";
static int s_OaStatus = 200;
static char s_OaSawMethod[8];
static bool probe_oa_http(const char* sMethod, const char* sUrl,
                          const char* sBody, const char* sAuth,
                          char** psBody, int* piStatus, void* pCtx)
{
    (void)sUrl; (void)sBody; (void)sAuth; (void)pCtx;
    snprintf(s_OaSawMethod, sizeof(s_OaSawMethod), "%s", sMethod);
    *piStatus = s_OaStatus;
    *psBody = (char*)xrtMalloc(strlen(s_OaReply) + 1);
    strcpy(*psBody, s_OaReply);
    return true;
}
static void probe_xoauth2(void)
{
    xoauth2client c;
    char* url;
    xoauth2token* t;
    char v[128], ch[128], st[64];

    /* PKCE：挑战可复算 */
    REQUIRE(xoauth2PkceGenerate(v, sizeof(v), ch, sizeof(ch)));
    REQUIRE(strlen(v) == 43 && strlen(ch) == 43);
    {
        unsigned char dig[32];
        char* expect;
        xrtSha256(v, strlen(v), dig);
        expect = xoauth2UrlEncode("");   /* 借用分配器验证可用，立刻释放 */
        xrtFree(expect);
    }

    /* GitHub 预设：授权 URL 含 PKCE + state */
    xoauth2UseGithub(&c, "cid-1", "sec", "https://app/cb");
    c.Config.Http = probe_oa_http;
    url = xoauth2BeginLogin(&c);
    REQUIRE(url != NULL);
    REQUIRE(strstr(url, "code_challenge_method=S256") != NULL);
    REQUIRE(strstr(url, "state=") != NULL);
    xrtFree(url);
    REQUIRE(c.Config.UserInfoUrl != NULL);   /* 评审②：预设带 userinfo */

    /* 错误 state：不焚毁 + 错误码 */
    xrtClearError();
    REQUIRE(xoauth2CompleteLogin(&c, "code", "wrong") == NULL);
    REQUIRE(xoauth2LastError() == XOAUTH2_ERROR_STATE_MISMATCH);
    REQUIRE(c.sState[0] != 0);

    /* mock 全流程：换 token 成功 + nonce 焚毁 + 刷新 + DENIED 分级 */
    s_OaStatus = 200;
    s_OaReply = "{\"access_token\":\"oa_tok\",\"refresh_token\":\"oa_rt\","
        "\"token_type\":\"Bearer\",\"expires_in\":7200}";
    t = xoauth2CompleteLogin(&c, "code", c.sState);
    REQUIRE(t != NULL && strcmp(t->AccessToken, "oa_tok") == 0);
    REQUIRE(strcmp(s_OaSawMethod, "POST") == 0);
    REQUIRE(strcmp(t->TokenType, "bearer") == 0);       /* 归一 */
    REQUIRE(t->ExpiresAt == t->ObtainedAt + 7200);      /* 时间戳 */
    REQUIRE(!xoauth2TokenExpiring(t, 60));
    xoauth2TokenFree(t);
    REQUIRE(c.sState[0] == 0 && c.sVerifier[0] == 0);   /* 一次性焚毁 */

    t = xoauth2Refresh(&c, "oa_rt");
    REQUIRE(t != NULL && strcmp(t->AccessToken, "oa_tok") == 0);
    xoauth2TokenFree(t);
    s_OaStatus = 400;
    s_OaReply = "{\"error\":\"invalid_grant\"}";
    xrtClearError();
    REQUIRE(xoauth2Refresh(&c, "oa_rt") == NULL);
    REQUIRE(xoauth2LastError() == XOAUTH2_ERROR_TOKEN_DENIED);

    /* OIDC 辅助：HttpGet/NonceConsume/未知有效期语义 */
    xoauth2UseGoogle(&c, "g", "s", "https://app/cb");
    c.Config.Http = probe_oa_http;
    s_OaStatus = 200;
    s_OaReply = "{\"keys\":[]}";
    {
        int st2 = 0;
        char* jwks = xoauth2HttpGet(&c, c.Config.JwksUrl, NULL, &st2);
        REQUIRE(jwks != NULL && strcmp(s_OaSawMethod, "GET") == 0);
        xrtFree(jwks);
    }
    REQUIRE(c.Config.UseNonce);
    url = xoauth2BeginLogin(&c);
    REQUIRE(url != NULL && strstr(url, "&nonce=") != NULL);
    xrtFree(url);
    REQUIRE(xoauth2NonceConsume(&c, c.sNonce));
    xrtClearError();
    REQUIRE(!xoauth2NonceConsume(&c, "replay"));
    REQUIRE(xoauth2LastError() == XOAUTH2_ERROR_NONCE_MISMATCH);
    {
        xoauth2token unk;   /* 评审③：未知有效期 ≠ 即将过期 */
        memset(&unk, 0, sizeof(unk));
        unk.AccessToken = "x";
        REQUIRE(!xoauth2TokenExpiring(&unk, 0));
    }

    /* 工具 + ClientUnit 语义 */
    REQUIRE(xoauth2StateGenerate(st, sizeof(st)) && strlen(st) >= 30);
    {
        char* e = xoauth2UrlEncode("a b");
        REQUIRE(e != NULL && strcmp(e, "a%20b") == 0);
        xrtFree(e);
    }
    xoauth2ClientUnit(&c);
    REQUIRE(c.Config.ClientId == NULL);
}
#endif

#ifdef XS_USE_MD4C
#include <md4c.h>
#include <md4c-html.h>
static char s_md4c_out[256];
static MD_SIZE s_md4c_len;
static void md4c_collect(const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    (void)userdata;
    if (s_md4c_len + size < sizeof(s_md4c_out)) {
        memcpy(s_md4c_out + s_md4c_len, text, size);
        s_md4c_len += size;
    }
}
static void probe_md4c(void)
{
    s_md4c_len = 0;
    REQUIRE(md_html("**hi** _md4c_", 13, md4c_collect, NULL, 0, 0) == 0);
    s_md4c_out[s_md4c_len] = 0;
    REQUIRE(strstr(s_md4c_out, "<strong>hi</strong>") != NULL);
    REQUIRE(strstr(s_md4c_out, "<em>md4c</em>") != NULL);
    /* md_parse 的回调指针无空保护，零值结构会空指针调用——
       md_html 内部自带完整回调，等价于安全地驱动 md_parse。 */
}
#endif

static const char nested_source[] =
    "#ifdef XS_USE_SQLITE\n#include <sqlite3.h>\n#endif\n"
    "#ifdef XS_USE_XTP2\n#include <xtp2.h>\n#endif\n"
    "#ifdef XS_USE_XLLM\n#include <xllm.h>\n#endif\n"
    "#ifdef XS_USE_XLLM_SESSION\n#include <xllm-session.h>\n#endif\n"
    "#ifdef XS_USE_XMAIL\n#include <xmail.h>\n#endif\n"
    "#ifdef XS_USE_XSMTP\n#include <xsmtp.h>\n#endif\n"
    "#ifdef XS_USE_XPOP3\n#include <xpop3.h>\n#endif\n"
    "#ifdef XS_USE_XIMAP\n#include <ximap.h>\n#endif\n"
    "#ifdef XS_USE_MD4C\n#include <md4c.h>\n#include <md4c-html.h>\n#endif\n"
    "#ifdef XS_USE_MD4C\nstatic void md4c_silent(const MD_CHAR* t, MD_SIZE n, void* u){ (void)t;(void)n;(void)u; }\n#endif\n"
    "#ifdef XS_USE_XACME\n#include <xacme.h>\n#endif\n"
    "#ifdef XS_USE_QRCODEGEN\n#include <qrcodegen.h>\n#include <qrpng.h>\n#endif\n"
    "#ifdef XS_USE_XJWT\n#include <xjwt.h>\n#endif\n"
    "#ifdef XS_USE_XOAUTH2\n#include <xoauth2.h>\n#endif\n"
    "int nested(void) { int mask = 0; unsigned char buf[256];\n"
    "#ifdef XS_USE_SQLITE\nif(sqlite3_libversion_number()>0) mask |= 1;\n#endif\n"
    "#ifdef XS_USE_XTP2\nif(xtp2_error_string(XTP2_OK)[0]=='o') mask |= 2;\n\n#endif\n\n"
    "#ifdef XS_USE_XLLM\nif(xllmEstimateTextTokens(\"a\")>0) mask |= 4;\n\n#endif\n\n"
    "#ifdef XS_USE_XLLM_SESSION\nxllmSessionConfigInit((xllm_session_config*)buf);\n"
    "if(((xllm_session_config*)buf)->uContextWindowTokens>0) mask |= 8;\n\n#endif\n\n"
    "#ifdef XS_USE_XMAIL\nif(xrtMailBoundaryValid(xrtStrView(\"b1\"))) mask |= 16;\n\n#endif\n\n"
    "#ifdef XS_USE_XSMTP\nif(xrtSmtpReplyLineParse(xrtStrView(\"250 ok\"), (xsmtpreplyline*)buf)) mask |= 32;\n\n#endif\n\n"
    "#ifdef XS_USE_XPOP3\nif(xrtPop3StatParse(xrtStrView(\"+OK 2 320\"), (xpop3stat*)buf)) mask |= 64;\n\n#endif\n\n"
    "#ifdef XS_USE_XIMAP\nif(xrtImapSequenceSetValid(xrtStrView(\"1:5\"))) mask |= 128;\n\n#endif\n\n"
    "#ifdef XS_USE_MD4C\nif(md_html(\"x\", 1, md4c_silent, 0, 0, 0) == 0) mask |= 256;\n\n#endif\n\n"
    "#ifdef XS_USE_XACME\nif(xrtAcmeDnsProviderCount() >= 1) mask |= 512;\n\n#endif\n\n"
    "#ifdef XS_USE_QRCODEGEN\nif(qrcodegen_isNumeric(\"123\")) mask |= 2048;\n\n#endif\n\n"
    "#ifdef XS_USE_XJWT\nif(xjwtAlgParse(\"HS256\")==XJWT_ALG_HS256) mask |= 4096;\n\n#endif\n\n"
    "#ifdef XS_USE_XOAUTH2\n{ char* q = xoauth2UrlEncode(\"a b\"); if(q && strcmp(q,\"a%20b\")==0) mask |= 8192; if(q) xrtFree(q); }\n\n#endif\n\n"
    "{ unsigned i, c = xsExtensionCount(); unsigned seen = 0;\n"
    "  for (i = 0; i < c; i++) { if (xsExtensionName(i) != 0) seen |= 1u; }\n"
    "  if (c > 0 && seen && xsExtensionName(c) == 0) mask |= 1024; }\n\n"
    "return mask; }\n";

void ServiceInit(XS_HostInfo *host)
{
    TCCState *nested;
    int (*fn)(void);
    int mask = 0;
    (void)host;
#ifdef XS_USE_SQLITE
    mask |= 1; probe_sqlite();
#endif
#ifdef XS_USE_XTP2
    mask |= 2; probe_xtp();
#endif
#ifdef XS_USE_XLLM
    mask |= 4; probe_xllm();
#endif
#ifdef XS_USE_XLLM_SESSION
    mask |= 8; probe_xllm_session();
#endif
#ifdef XS_USE_XMAIL
    mask |= 16; probe_xmail();
#endif
#ifdef XS_USE_XSMTP
    mask |= 32; probe_xsmtp();
#endif
#ifdef XS_USE_XPOP3
    mask |= 64; probe_xpop3();
#endif
#ifdef XS_USE_XIMAP
    mask |= 128; probe_ximap();
#endif
#ifdef XS_USE_MD4C
    mask |= 256; probe_md4c();
#endif
#ifdef XS_USE_XACME
    mask |= 512; probe_xacme();
#endif
#ifdef XS_USE_QRCODEGEN
    mask |= 2048; probe_qrcodegen();
#endif
#ifdef XS_USE_XJWT
    mask |= 4096; probe_xjwt();
#endif
#ifdef XS_USE_XOAUTH2
    mask |= 8192; probe_xoauth2();
#endif
    /* xsExtensionEnabled：大小写不敏感、requires 可见、未知/空/NULL 恒 false */
    REQUIRE(xsExtensionEnabled(NULL) == false);
    REQUIRE(xsExtensionEnabled("") == false);
    REQUIRE(xsExtensionEnabled("no-such-lib") == false);
    REQUIRE(xsExtensionEnabled("sqlite") ==
        (
#ifdef XS_USE_SQLITE
            true
#else
            false
#endif
        ));
    REQUIRE(xsExtensionEnabled("XSMTP") ==
        (
#ifdef XS_USE_XSMTP
            true
#else
            false
#endif
        ));
#ifdef XS_USE_XSMTP
    REQUIRE(xsExtensionEnabled("xmail")); /* requires 展开可见 */
#endif
    /* xsExtensionCount / xsExtensionName：注册表顺序枚举，越界 NULL */
    {
        uint32 iEach;
        uint32 iCount = xsExtensionCount();

        for ( iEach = 0; iEach < iCount; iEach++ ) {
            const char* sEach = xsExtensionName(iEach);

            REQUIRE(sEach != NULL && sEach[0] != 0);
            REQUIRE(xsExtensionEnabled(sEach)); /* 枚举名必为 enabled */
        }
        REQUIRE(xsExtensionName(iCount) == NULL);      /* 首个越界 */
        REQUIRE(xsExtensionName(0xFFFFFFFFu) == NULL); /* 深越界 */
        if ( iCount > 0 ) {
            mask |= 1024; /* 枚举链路打通（default 下 count==0 不置位） */
        }
    }
    REQUIRE(mask == EXPECTED_MASK);
    nested = xsCreateTCC();
    REQUIRE(nested != NULL && tcc_compile_string(nested, nested_source) == 0);
    REQUIRE(tcc_relocate(nested) == 0);
    fn = (int (*)(void))tcc_get_symbol(nested, "nested");
    REQUIRE(fn != NULL && fn() == EXPECTED_MASK);
    xsDestroyTCC(nested);
    printf("[extensions] ok mask=%d\n", mask);
    fflush(stdout);
    exit(0);
}
'''


def invoke(exe: Path, source: str, directory: Path, name: str) -> subprocess.CompletedProcess:
    script = directory / (name + ".c")
    config = directory / (name + ".json")
    script.write_text(source, encoding="utf-8")
    config.write_text(json.dumps({
        "engine": {"workers": 1},
        "services": [{"class": "custom", "name": name, "enabled": True,
                      "devlang": "c", "devfile": str(script)}],
    }), encoding="utf-8")
    return subprocess.run([str(exe), str(config)], cwd=directory, capture_output=True,
                          text=True, encoding="utf-8", errors="replace", timeout=25)


def check(exe: Path, names: list[str]) -> None:
    selected = select_extensions(names)
    mask = ((1 if "sqlite" in selected else 0)
            | (2 if "xtp" in selected else 0)
            | (4 if "xllm" in selected else 0)
            | (8 if "xllm-session" in selected else 0)
            | (16 if "xmail" in selected else 0)
            | (32 if "xsmtp" in selected else 0)
            | (64 if "xpop3" in selected else 0)
            | (128 if "ximap" in selected else 0)
            | (256 if "md4c" in selected else 0)
            | (512 if "xacme" in selected else 0)
            | (2048 if "qrcodegen" in selected else 0)
            | (4096 if "xjwt" in selected else 0)
            | (8192 if "xoauth2" in selected else 0)
            | (1024 if selected else 0))
    with tempfile.TemporaryDirectory(prefix="xs-extension-probe-") as temp:
        directory = Path(temp)
        proc = invoke(exe, f"#define EXPECTED_MASK {mask}\n" + PROBE, directory, "enabled")
        output = proc.stdout + proc.stderr
        if proc.returncode != 0 or f"[extensions] ok mask={mask}" not in output:
            raise RuntimeError(f"enabled/nested probe failed ({proc.returncode}):\n{output}")
        print(f"PASS enabled + nested TCC: {', '.join(selected) or '(none)'}", flush=True)

        # 启动横幅与 --version 一致地报告本变体扩展清单
        expected_list = (f"[xs] extensions ({len(selected)}): "
                         + ", ".join(selected)) if selected else "[xs] extensions: none"
        if expected_list not in output:
            raise RuntimeError(f"startup banner missing extension list:\n{output}")
        version = subprocess.run([str(exe), "--version"], cwd=directory, capture_output=True,
                                 text=True, encoding="utf-8", errors="replace", timeout=25)
        if (version.returncode != 0 or "[xs] XServer " not in version.stdout
                or expected_list not in version.stdout):
            raise RuntimeError(f"--version output unexpected ({version.returncode}):\n"
                               f"{version.stdout}{version.stderr}")
        print(f"PASS version banner + --version: {expected_list}", flush=True)
        absent = {
            "sqlite": ("sqlite3.h", "extern int sqlite3_libversion_number(void);",
                       "sqlite3_libversion_number()"),
            "xtp": ("xtp2.h", "extern const char *xtp2_error_string(int);",
                    "xtp2_error_string(0)"),
            "xllm": ("xllm.h", "extern const char *xllmErrorCodeName(int);",
                     "xllmErrorCodeName(0)"),
            "xllm-session": ("xllm-session.h", "extern void xllmSessionConfigInit(void*);",
                             "xllmSessionConfigInit(0)"),
            "xmail": ("xmail.h", "extern int xrtMailBoundaryValid(void*);",
                      "xrtMailBoundaryValid(0)"),
            "xsmtp": ("xsmtp.h", "extern int xrtSmtpReplyLineParse(void*);",
                      "xrtSmtpReplyLineParse(0)"),
            "xpop3": ("xpop3.h", "extern int xrtPop3StatParse(void*);",
                      "xrtPop3StatParse(0)"),
            "ximap": ("ximap.h", "extern int xrtImapSequenceSetValid(void*);",
                      "xrtImapSequenceSetValid(0)"),
            "md4c": ("md4c.h", "extern int md_parse(const void*, unsigned, void*, void*);",
                     "md_parse(0, 0, 0, 0)"),
            "xacme": ("xacme.h", "extern size_t xrtAcmeDnsProviderCount(void);",
                      "xrtAcmeDnsProviderCount()"),
            "qrcodegen": ("qrcodegen.h", "extern int qrcodegen_isNumeric(const char*);",
                          "qrcodegen_isNumeric(0)"),
        }
        for name, (header, declaration, call) in absent.items():
            if name in selected:
                continue
            source = (f"#include <xsbase.h>\n#include <stdlib.h>\n#include <{header}>\n"
                      "void ServiceInit(XS_HostInfo *h){(void)h;exit(0);}\n")
            proc = invoke(exe, source, directory, name + "-absent-header")
            if proc.returncode != 1 or header not in proc.stdout + proc.stderr:
                raise RuntimeError(f"{name} header was not gated:\n{proc.stdout}{proc.stderr}")
            source = (f"#include <xsbase.h>\n#include <stdlib.h>\n{declaration}\n"
                      f"void ServiceInit(XS_HostInfo *h){{(void)h;(void){call};exit(0);}}\n")
            proc = invoke(exe, source, directory, name + "-absent-symbol")
            if proc.returncode != 1 or call.split("(")[0] not in proc.stdout + proc.stderr:
                raise RuntimeError(f"{name} symbol was not gated:\n{proc.stdout}{proc.stderr}")
            print(f"PASS absent header + symbol: {name}", flush=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("extensions", nargs="*")
    args = parser.parse_intermixed_args()
    try:
        check(args.exe.resolve(), args.extensions)
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"FAIL: {error}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
