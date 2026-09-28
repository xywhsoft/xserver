#ifndef XS_CORE_ACME_H
#define XS_CORE_ACME_H

/* ACME 证书自动续签守护（仅 XS_USE_XACME 变体生效）。
 *
 * 形态：进程级常驻线程，条件变量定时唤醒（停机信号可立即打断），
 * 逐证书组串行执行 xrtAcmeObtain 一站式签发（内部自带续签判断：
 * 本地 store 证书剩余寿命充足时直接返回 pbRenewed=false），产物按
 * acme.sh 同构布局原子写入 host tls 目录（cert.pem=叶 / ca.pem=
 * 中间链 / key.pem / fullchain.pem），随后对本组反向索引命中的每个
 * server 调 xsTlsRefresh 热替换（拓扑 lease 线程安全）。
 *
 * 生命周期：daemon 启动时深拷贝配置，此后不回读（v1 重启生效，
 * reload 换代不影响）；线程自建 Workers=1 网络引擎与宿主引擎零耦合；
 * 停机先于服务排空调用（限时 join）。凭据（账户私钥/阿里 Key）走
 * 独立文件，xs.json 只留路径引用。
 */

#include <stdio.h>
#include <string.h>

/* ---------------- 配置模型（不随扩展门控，定义零成本） ---------------- */

typedef struct XS_AcmeGroup {
	char*			sName;		/* 展示名 */
	char**			sDomains;	/* SAN 列表；sDomains[0] 为主域名 */
	size_t			iDomainCount;
	char*			sOutDir;	/* host tls 目录（解析期转绝对路径） */
	/* 反向索引（daemon 启动时构建）：引用本目录的 server 名 */
	char**			sServers;
	size_t			iServerCount;
} XS_AcmeGroup;

/* CA 回退链条目：有序数组，顺序即回退优先级（首个成功者胜出，
 * 续签到期永远从首个重试——主 CA 恢复后自动回归）。 */
typedef struct XS_AcmeCa {
	char*			sName;			/* 展示名；空 = 取 URL 域名 */
	char*			sUrl;			/* ACME directory URL（必填） */
	char*			sCaFile;		/* 信任锚 PEM 文件；空 = 系统信任库 */
	char*			sEabKidFile;		/* EAB KID 文件；与 hmac 成对可空 */
	char*			sEabHmacFile;		/* EAB HMAC 文件 */
} XS_AcmeCa;

typedef struct XS_AcmeConfig {
	bool			bEnabled;
	char*			sAccountKeyFile;	/* 账户私钥 PEM；可空 = 新注册（各 CA 独立账户同钥） */
	char*			sEmail;
	char*			sDnsKeyFile;		/* DNS 凭据 JSON 文件 */
	int			iRenewalDays;		/* 0 = 30 */
	int			iCheckIntervalHours;	/* 0 = 12 */
	XS_AcmeCa*		pCas;			/* 有序 CA 回退链（≥1） */
	size_t			iCaCount;
	XS_AcmeGroup*		pGroups;
	size_t			iGroupCount;
} XS_AcmeConfig;

static void XS_AcmeFreeGroup(XS_AcmeGroup* pGroup)
{
	size_t i;

	if ( pGroup == NULL ) return;
	xrtFree(pGroup->sName);
	for ( i = 0; i < pGroup->iDomainCount; i++ ) xrtFree(pGroup->sDomains[i]);
	xrtFree(pGroup->sDomains);
	xrtFree(pGroup->sOutDir);
	for ( i = 0; i < pGroup->iServerCount; i++ ) xrtFree(pGroup->sServers[i]);
	xrtFree(pGroup->sServers);
	memset(pGroup, 0, sizeof(*pGroup));
}

static void XS_AcmeConfigFree(XS_AcmeConfig* pConfig)
{
	size_t i;

	if ( pConfig == NULL ) return;
	xrtFree(pConfig->sAccountKeyFile);
	xrtFree(pConfig->sEmail);
	xrtFree(pConfig->sDnsKeyFile);
	for ( i = 0; i < pConfig->iCaCount; i++ ) {
		XS_AcmeCa* pCa = &pConfig->pCas[i];

		xrtFree(pCa->sName);
		xrtFree(pCa->sUrl);
		xrtFree(pCa->sCaFile);
		xrtFree(pCa->sEabKidFile);
		xrtFree(pCa->sEabHmacFile);
	}
	xrtFree(pConfig->pCas);
	for ( i = 0; i < pConfig->iGroupCount; i++ ) XS_AcmeFreeGroup(&pConfig->pGroups[i]);
	xrtFree(pConfig->pGroups);
	memset(pConfig, 0, sizeof(*pConfig));
}

static char* XS_AcmeDupN(const char* sText, size_t iSize)
{
	return xrtStrDupN(sText, iSize);
}

#if defined(XS_USE_XACME)

#include <xacme/features.h>	/* XACME_MODULE_ALL → 全特性提升（宿主 TU 无逐源 defines） */
#include <xrt/acme.h>
#include <xrt/acme_obtain.h>
#include <xrt/acme_dns.h>
#include <xrt/acme_dns_ali.h>

/* 与 core/api.h 的契约声明一致（config.h 先于 api.h 包含）。 */
XRT_API bool xsTlsRefresh(const char* sServerName, char* sErr, size_t iErrCap);

/* ---------------- 小工具 ---------------- */

static char* XS_AcmeDup(const char* sText)
{
	if ( sText == NULL || sText[0] == '\0' ) return NULL;
	return xrtStrDup(sText);
}

/* 深拷贝（daemon 启动时快照，与配置 revision 生命周期解耦） */
static bool XS_AcmeConfigClone(const XS_AcmeConfig* pSrc, XS_AcmeConfig* pDst)
{
	size_t i, j;

	memset(pDst, 0, sizeof(*pDst));
	pDst->bEnabled = pSrc->bEnabled;
	pDst->sAccountKeyFile = XS_AcmeDup(pSrc->sAccountKeyFile);
	pDst->sEmail = XS_AcmeDup(pSrc->sEmail);
	pDst->sDnsKeyFile = XS_AcmeDup(pSrc->sDnsKeyFile);
	pDst->iRenewalDays = pSrc->iRenewalDays;
	pDst->iCheckIntervalHours = pSrc->iCheckIntervalHours;
	if ( pSrc->iCaCount > 0 ) {
		pDst->pCas = (XS_AcmeCa*)xrtCalloc(pSrc->iCaCount, sizeof(XS_AcmeCa));
		if ( pDst->pCas == NULL ) goto Fail;
		pDst->iCaCount = pSrc->iCaCount;
		for ( i = 0; i < pSrc->iCaCount; i++ ) {
			const XS_AcmeCa* pS = &pSrc->pCas[i];
			XS_AcmeCa* pD = &pDst->pCas[i];

			pD->sName = XS_AcmeDup(pS->sName);
			pD->sUrl = XS_AcmeDup(pS->sUrl);
			pD->sCaFile = XS_AcmeDup(pS->sCaFile);
			pD->sEabKidFile = XS_AcmeDup(pS->sEabKidFile);
			pD->sEabHmacFile = XS_AcmeDup(pS->sEabHmacFile);
			if ( pD->sUrl == NULL ) goto Fail;
		}
	}
	if ( pSrc->iGroupCount > 0 ) {
		pDst->pGroups = (XS_AcmeGroup*)xrtCalloc(pSrc->iGroupCount,
			sizeof(XS_AcmeGroup));
		if ( pDst->pGroups == NULL ) goto Fail;
		pDst->iGroupCount = pSrc->iGroupCount;
		for ( i = 0; i < pSrc->iGroupCount; i++ ) {
			const XS_AcmeGroup* pS = &pSrc->pGroups[i];
			XS_AcmeGroup* pD = &pDst->pGroups[i];

			pD->sName = XS_AcmeDup(pS->sName);
			pD->sOutDir = XS_AcmeDup(pS->sOutDir);
			pD->iDomainCount = pS->iDomainCount;
			pD->sDomains = (char**)xrtCalloc(pS->iDomainCount, sizeof(char*));
			if ( pD->sName == NULL || pD->sOutDir == NULL || pD->sDomains == NULL )
				goto Fail;
			for ( j = 0; j < pS->iDomainCount; j++ ) {
				pD->sDomains[j] = XS_AcmeDup(pS->sDomains[j]);
				if ( pD->sDomains[j] == NULL ) goto Fail;
			}
		}
	}
	return true;

Fail:
	XS_AcmeConfigFree(pDst);
	return false;
}

/* 路径比较键：反斜杠归一 '/'，Windows 再降大小写。 */
static bool XS_AcmePathEqual(const char* sLeft, const char* sRight)
{
	char a[1024], b[1024];
	size_t i = 0, k = 0;

	if ( sLeft == NULL ) sLeft = "";
	if ( sRight == NULL ) sRight = "";
	for ( i = 0; sLeft[i] != '\0' && k + 1 < sizeof(a); i++ ) {
		char c = sLeft[i];
		if ( c == '\\' ) c = '/';
#if defined(_WIN32) || defined(_WIN64)
		if ( c >= 'A' && c <= 'Z' ) c = (char)(c - 'A' + 'a');
#endif
		a[k++] = c;
	}
	while ( k > 1 && a[k-1] == '/' ) k--;
	a[k] = '\0'; k = 0;
	for ( i = 0; sRight[i] != '\0' && k + 1 < sizeof(b); i++ ) {
		char c = sRight[i];
		if ( c == '\\' ) c = '/';
#if defined(_WIN32) || defined(_WIN64)
		if ( c >= 'A' && c <= 'Z' ) c = (char)(c - 'A' + 'a');
#endif
		b[k++] = c;
	}
	while ( k > 1 && b[k-1] == '/' ) k--;
	b[k] = '\0';
	return strcmp(a, b) == 0;
}

/* tls 文件路径 → 所属目录。 */
static void XS_AcmeDirOf(const char* sFilePath, char* sOut, size_t iCap)
{
	const char* pSlash, *pBack;
	size_t n;

	if ( sFilePath == NULL ) { sOut[0] = '\0'; return; }
	pSlash = strrchr(sFilePath, '/');
	pBack = strrchr(sFilePath, '\\');
	if ( pBack != NULL && (pSlash == NULL || pBack > pSlash) ) pSlash = pBack;
	if ( pSlash == NULL ) { sOut[0] = '\0'; return; }
	n = (size_t)(pSlash - sFilePath);
	if ( n >= iCap ) n = iCap - 1;
	memcpy(sOut, sFilePath, n);
	sOut[n] = '\0';
}

static char* XS_AcmeReadText(const char* sPath)
{
	bytes pData;
	size_t iSize = 0;
	char* sText;

	if ( sPath == NULL || sPath[0] == '\0' ) return NULL;
	pData = xrtFileReadAll(sPath, &iSize);
	if ( pData == NULL ) return NULL;
	sText = (char*)xrtMalloc(iSize + 1);
	if ( sText == NULL ) { xrtFree(pData); return NULL; }
	memcpy(sText, pData, iSize);
	sText[iSize] = '\0';
	xrtFree(pData);
	return sText;
}

/* 就地去除首尾空白与换行（EAB 单行文件用）。 */
static void XS_AcmeTrimLine(char* sText)
{
	size_t iBegin = 0, iEnd;

	if ( sText == NULL ) return;
	iEnd = strlen(sText);
	while ( iBegin < iEnd && (unsigned char)sText[iBegin] <= ' ' ) iBegin++;
	while ( iEnd > iBegin && (unsigned char)sText[iEnd - 1] <= ' ' ) iEnd--;
	if ( iBegin > 0 || iEnd < strlen(sText) ) {
		memmove(sText, sText + iBegin, iEnd - iBegin);
	}
	sText[iEnd - iBegin] = '\0';
}

/* 相对路径以 appPath 为基准转绝对（已有绝对路径保持不变）。 */
static char* XS_AcmeResolve(const char* sPath, const char* sAppPath)
{
	str sAbs;

	if ( sPath == NULL || sPath[0] == '\0' ) return NULL;
	if ( xrtPathIsAbs(sPath) ) return xrtStrDup(sPath);
	sAbs = xrtPathJoin(sAppPath != NULL ? sAppPath : ".", sPath);
	return sAbs;
}

/* ---------------- 配置解析（config.h 静态助手可见处调用） ---------------- */

static bool XS_AcmeParseGroup(xvalue* pObj, XS_AcmeGroup* pGroup,
	char* sErr, size_t iErrCap)
{
	xvalue* pDomains;
	const char* sTmp = NULL;
	size_t i;

	memset(pGroup, 0, sizeof(*pGroup));
	if ( !XS_ConfigTakeString(pObj, "name", &sTmp, sErr, iErrCap) ) return false;
	if ( sTmp == NULL || sTmp[0] == '\0' ) {
		snprintf(sErr, iErrCap, "acme certs[].name required");
		return false;
	}
	pGroup->sName = (char*)sTmp;
	if ( !XS_ConfigTakeString(pObj, "out_dir", &sTmp, sErr, iErrCap) ) return false;
	if ( sTmp == NULL || sTmp[0] == '\0' ) {
		snprintf(sErr, iErrCap, "acme certs[%s].out_dir required", pGroup->sName);
		return false;
	}
	pGroup->sOutDir = (char*)sTmp;
	pDomains = xrtValueObjectGet(pObj, XS_ConfigKey("domains"));
	if ( pDomains == NULL || xrtValueType(pDomains) != XVALUE_ARRAY ||
	     xrtValueCount(pDomains) == 0 ) {
		snprintf(sErr, iErrCap, "acme certs[%s].domains expect non-empty array",
			pGroup->sName);
		return false;
	}
	pGroup->iDomainCount = xrtValueCount(pDomains);
	pGroup->sDomains = (char**)xrtCalloc(pGroup->iDomainCount, sizeof(char*));
	if ( pGroup->sDomains == NULL ) {
		snprintf(sErr, iErrCap, "out of memory (acme domains)");
		return false;
	}
	for ( i = 0; i < pGroup->iDomainCount; i++ ) {
		xvalue* pItem = xrtValueArrayGet(pDomains, i);
		xstrview tView;

		if ( pItem == NULL || !xrtValueGetString(pItem, &tView) ||
		     tView.Size == 0 ) {
			snprintf(sErr, iErrCap, "acme certs[%s].domains[%u] expect string",
				pGroup->sName, (unsigned)i);
			return false;
		}
		pGroup->sDomains[i] = XS_AcmeDupN(tView.Data, tView.Size);
		if ( pGroup->sDomains[i] == NULL ) {
			snprintf(sErr, iErrCap, "out of memory (acme domain)");
			return false;
		}
	}
	return true;
}

/* 单个 CA 条目解析（directories[] 数组元素）。 */
static bool XS_AcmeParseCa(xvalue* pObj, XS_AcmeCa* pCa,
	char* sErr, size_t iErrCap)
{
	const char* sTmp = NULL;

	memset(pCa, 0, sizeof(*pCa));
	if ( !XS_ConfigTakeString(pObj, "name", &sTmp, sErr, iErrCap) ) return false;
	pCa->sName = (char*)sTmp; sTmp = NULL;
	if ( !XS_ConfigTakeString(pObj, "url", &sTmp, sErr, iErrCap) ) return false;
	if ( sTmp == NULL || sTmp[0] == '\0' ) {
		snprintf(sErr, iErrCap, "acme.directories[].url required");
		return false;
	}
	pCa->sUrl = (char*)sTmp; sTmp = NULL;
	if ( !XS_ConfigTakeString(pObj, "ca_file", &sTmp, sErr, iErrCap) ) return false;
	pCa->sCaFile = (char*)sTmp; sTmp = NULL;
	if ( !XS_ConfigTakeString(pObj, "eab_kid_file", &sTmp, sErr, iErrCap) ) return false;
	pCa->sEabKidFile = (char*)sTmp; sTmp = NULL;
	if ( !XS_ConfigTakeString(pObj, "eab_hmac_file", &sTmp, sErr, iErrCap) ) return false;
	pCa->sEabHmacFile = (char*)sTmp; sTmp = NULL;
	/* EAB 成对约束：只填一个 = 配置笔误 */
	if ( (pCa->sEabKidFile == NULL) != (pCa->sEabHmacFile == NULL) ) {
		snprintf(sErr, iErrCap,
			"acme.directories['%s']: eab_kid_file/eab_hmac_file must appear in pair",
			pCa->sName != NULL ? pCa->sName : pCa->sUrl);
		return false;
	}
	return true;
}

/* 根节点解析（未配置返回 NULL+true；结构校验在此，host 引用交叉
 * 校验在 daemon 启动时——Servers 那时才装配）。
 * 兼容：单字段 directory 等价单元素链；directory 与 directories
 * 互斥（同时出现 fail-fast 防歧义）。 */
static bool XS_AcmeRootParse(xvalue* pRoot, XS_AcmeConfig** ppConfig,
	char* sErr, size_t iErrCap)
{
	xvalue* pAcme = xrtValueObjectGet(pRoot, XS_ConfigKey("acme"));
	XS_AcmeConfig* pConfig;
	xvalue* pCerts;
	xvalue* pDirs;
	const char* sLegacyUrl = NULL;
	const char* sLegacyCa = NULL;
	const char* sTmp = NULL;
	int64 iVal = 0;
	size_t i;

	*ppConfig = NULL;
	if ( pAcme == NULL ) return true;
	if ( xrtValueType(pAcme) != XVALUE_OBJECT ) {
		snprintf(sErr, iErrCap, "field 'acme' expect object");
		return false;
	}
	pConfig = (XS_AcmeConfig*)xrtCalloc(1, sizeof(*pConfig));
	if ( pConfig == NULL ) {
		snprintf(sErr, iErrCap, "out of memory (acme config)");
		return false;
	}
	if ( !XS_ConfigTakeBool(pAcme, "enabled", &pConfig->bEnabled, sErr, iErrCap) ) goto Fail;
	if ( !XS_ConfigTakeString(pAcme, "account_key_file", &sTmp, sErr, iErrCap) ) goto Fail;
	pConfig->sAccountKeyFile = (char*)sTmp; sTmp = NULL;
	if ( !XS_ConfigTakeString(pAcme, "email", &sTmp, sErr, iErrCap) ) goto Fail;
	pConfig->sEmail = (char*)sTmp; sTmp = NULL;
	if ( !XS_ConfigTakeString(pAcme, "dns_key_file", &sTmp, sErr, iErrCap) ) goto Fail;
	pConfig->sDnsKeyFile = (char*)sTmp; sTmp = NULL;
	if ( !XS_ConfigTakeInt(pAcme, "renew_before_days", &iVal, sErr, iErrCap) ) goto Fail;
	pConfig->iRenewalDays = (iVal > 0 && iVal <= 89) ? (int)iVal : 30;
	if ( !XS_ConfigTakeInt(pAcme, "check_interval_hours", &iVal, sErr, iErrCap) ) goto Fail;
	pConfig->iCheckIntervalHours = (iVal > 0 && iVal <= 720) ? (int)iVal : 12;

	/* CA 回退链：directories 数组优先，directory 单字段兼容 */
	pDirs = xrtValueObjectGet(pAcme, XS_ConfigKey("directories"));
	if ( !XS_ConfigTakeString(pAcme, "directory", &sTmp, sErr, iErrCap) ) goto Fail;
	sLegacyUrl = sTmp; sTmp = NULL;
	if ( !XS_ConfigTakeString(pAcme, "ca_file", &sTmp, sErr, iErrCap) ) goto Fail;
	sLegacyCa = sTmp; sTmp = NULL;
	if ( pDirs != NULL && sLegacyUrl != NULL ) {
		snprintf(sErr, iErrCap,
			"acme: 'directory' and 'directories' are mutually exclusive");
		goto Fail;
	}
	if ( pDirs != NULL ) {
		if ( xrtValueType(pDirs) != XVALUE_ARRAY || xrtValueCount(pDirs) == 0 ) {
			snprintf(sErr, iErrCap, "acme.directories expect non-empty array");
			goto Fail;
		}
		pConfig->iCaCount = xrtValueCount(pDirs);
		pConfig->pCas = (XS_AcmeCa*)xrtCalloc(pConfig->iCaCount,
			sizeof(XS_AcmeCa));
		if ( pConfig->pCas == NULL ) {
			snprintf(sErr, iErrCap, "out of memory (acme directories)");
			goto Fail;
		}
		for ( i = 0; i < pConfig->iCaCount; i++ ) {
			xvalue* pItem = xrtValueArrayGet(pDirs, i);

			if ( pItem == NULL || xrtValueType(pItem) != XVALUE_OBJECT ) {
				snprintf(sErr, iErrCap, "acme.directories[%u] expect object",
					(unsigned)i);
				goto Fail;
			}
			if ( !XS_AcmeParseCa(pItem, &pConfig->pCas[i], sErr, iErrCap) ) goto Fail;
		}
	} else if ( sLegacyUrl != NULL ) {
		pConfig->pCas = (XS_AcmeCa*)xrtCalloc(1, sizeof(XS_AcmeCa));
		if ( pConfig->pCas == NULL ) {
			snprintf(sErr, iErrCap, "out of memory (acme directory)");
			goto Fail;
		}
		pConfig->iCaCount = 1;
		pConfig->pCas[0].sUrl = xrtStrDup(sLegacyUrl);
		pConfig->pCas[0].sCaFile = xrtStrDup(sLegacyCa);
		if ( pConfig->pCas[0].sUrl == NULL ) {
			snprintf(sErr, iErrCap, "out of memory (acme url)");
			goto Fail;
		}
	} else {
		snprintf(sErr, iErrCap, "acme.directories required "
			"(e.g. [{\"url\":\"https://acme-v02.api.letsencrypt.org/directory\"}])");
		goto Fail;
	}
	/* URL 去重（同 CA 配两遍 = 笔误） */
	for ( i = 0; i < pConfig->iCaCount; i++ ) {
		size_t j;

		for ( j = i + 1; j < pConfig->iCaCount; j++ ) {
			if ( strcmp(pConfig->pCas[i].sUrl, pConfig->pCas[j].sUrl) == 0 ) {
				snprintf(sErr, iErrCap,
					"acme.directories: duplicate url %s", pConfig->pCas[i].sUrl);
				goto Fail;
			}
		}
	}
	if ( pConfig->sDnsKeyFile == NULL ) {
		snprintf(sErr, iErrCap, "acme.dns_key_file required (dns-01)");
		goto Fail;
	}
	pCerts = xrtValueObjectGet(pAcme, XS_ConfigKey("certs"));
	if ( pCerts == NULL || xrtValueType(pCerts) != XVALUE_ARRAY ||
	     xrtValueCount(pCerts) == 0 ) {
		snprintf(sErr, iErrCap, "acme.certs expect non-empty array");
		goto Fail;
	}
	pConfig->iGroupCount = xrtValueCount(pCerts);
	pConfig->pGroups = (XS_AcmeGroup*)xrtCalloc(pConfig->iGroupCount,
		sizeof(XS_AcmeGroup));
	if ( pConfig->pGroups == NULL ) {
		snprintf(sErr, iErrCap, "out of memory (acme certs)");
		goto Fail;
	}
	for ( i = 0; i < pConfig->iGroupCount; i++ ) {
		xvalue* pItem = xrtValueArrayGet(pCerts, i);

		if ( pItem == NULL || xrtValueType(pItem) != XVALUE_OBJECT ) {
			snprintf(sErr, iErrCap, "acme.certs[%u] expect object", (unsigned)i);
			goto Fail;
		}
		if ( !XS_AcmeParseGroup(pItem, &pConfig->pGroups[i], sErr, iErrCap) ) goto Fail;
	}
	if ( !pConfig->bEnabled ) {
		printf("[xs] acme: config present but disabled, daemon not started\n");
	}
	*ppConfig = pConfig;
	return true;

Fail:
	XS_AcmeConfigFree(pConfig);
	return false;
}

/* ---------------- daemon 状态 ---------------- */

/* 运行期 CA 条目：启动时加载文件内容（路径解析 + EAB/信任锚读入），
 * 之后线程私有。 */
typedef struct XS_AcmeCaRt {
	char*			sName;		/* 解析后的展示名（配置名或 URL 域名） */
	char*			sUrl;
	char*			sCaPem;		/* 已读入；NULL = 系统信任库 */
	char*			sEabKid;	/* 已读入；NULL = 无 EAB */
	char*			sEabHmac;
} XS_AcmeCaRt;

typedef struct XS_AcmeDaemon {
	XS_AcmeConfig		tConfig;	/* 启动时深拷贝，此后线程私有 */
	XS_AcmeCaRt*		pCas;		/* 运行期 CA 链（与 tConfig.pCas 同序） */
	size_t			iCaCount;
	char*			sStoreRoot;	/* xacme store 根（appPath/acme-store） */
	char*			sAccountKeyPem;	/* 启动时读入（可空 = 新注册） */
	char*			sAliKeyId;	/* 启动时读入 */
	char*			sAliKeySecret;
	xnetengine*		pEngine;	/* 自建 Workers=1 */
	xthread*		pThread;
	xmutex*			pLock;
	xcond*			pWake;
	bool			bStop;
	bool			bStarted;
} XS_AcmeDaemon;

static XS_AcmeDaemon g_XS_Acme;

/* ---------------- 反向索引：out_dir → 引用它的 server 名 ---------------- */

static void XS_AcmeBuildIndex(XS_AcmeDaemon* pDaemon, XS_App* pApp)
{
	char arrDir[1024];
	uint32 iSrv, iHost;
	size_t iG;

	for ( iSrv = 0; iSrv < pApp->ServerCount; iSrv++ ) {
		XS_ServerInfo* pServer = pApp->Servers[iSrv];

		if ( pServer == NULL || !pServer->TLS ) continue;
		for ( iHost = 0; iHost <= pServer->HostCount; iHost++ ) {
			const XS_HostInfo* pHost = iHost == 0 ? pServer->DefaultHost
			                                      : pServer->Hosts[iHost - 1];
			if ( pHost == NULL || pHost->TlsCert == NULL ) continue;
			XS_AcmeDirOf(pHost->TlsCert, arrDir, sizeof(arrDir));
			if ( arrDir[0] == '\0' ) continue;
			for ( iG = 0; iG < pDaemon->tConfig.iGroupCount; iG++ ) {
				XS_AcmeGroup* pGroup = &pDaemon->tConfig.pGroups[iG];
				char** sGrow;

				if ( !XS_AcmePathEqual(arrDir, pGroup->sOutDir) ) continue;
				sGrow = (char**)xrtRealloc(pGroup->sServers,
					(pGroup->iServerCount + 1) * sizeof(char*));
				if ( sGrow == NULL ) break;
				pGroup->sServers = sGrow;
				pGroup->sServers[pGroup->iServerCount] =
					xrtStrDup(pServer->Name != NULL ? pServer->Name : "?");
				if ( pGroup->sServers[pGroup->iServerCount] != NULL ) {
					pGroup->iServerCount++;
				}
				break;	/* 一个 host 只计入一次 */
			}
		}
	}
}

/* ---------------- 证书落盘：fullchain 拆叶/链，tmp+原子替换 ---------------- */

static bool XS_AcmeWriteOne(const char* sPath, const char* sData, size_t iSize)
{
	char sTmp[1100];
	xbytesview tData;
	bool bOK;

	snprintf(sTmp, sizeof(sTmp), "%s.tmp", sPath);
	tData.Data = (unsigned char*)sData;
	tData.Size = iSize;
	if ( !xrtFileWriteAll(sTmp, tData) ) return false;
	bOK = xrtFileMove(sTmp, sPath, true);
	if ( !bOK ) xrtFileDelete(sTmp);
	return bOK;
}

/* fullchain = [叶][中间链...]：首块写 cert.pem，其余写 ca.pem；
 * key.pem 原样；fullchain.pem 保留整链。布局与 setcerts.sh/acme.sh 同构。 */
static bool XS_AcmeWriteGrant(const XS_AcmeGroup* pGroup,
	const xacmeissuegrant* pGrant)
{
	const char* sEnd = "-----END CERTIFICATE-----";
	const char* pSplit;
	char sPath[1100];

	if ( pGrant->sFullchainPem == NULL || pGrant->sKeyPem == NULL ) return false;
	{
		const char* pFirst = strstr(pGrant->sFullchainPem, sEnd);

		if ( pFirst == NULL ) return false;
		pSplit = pFirst + strlen(sEnd);
		while ( *pSplit == '\r' || *pSplit == '\n' ) pSplit++;
	}
	snprintf(sPath, sizeof(sPath), "%s/cert.pem", pGroup->sOutDir);
	if ( !XS_AcmeWriteOne(sPath, pGrant->sFullchainPem,
		(size_t)(pSplit - pGrant->sFullchainPem)) ) return false;
	snprintf(sPath, sizeof(sPath), "%s/ca.pem", pGroup->sOutDir);
	if ( !XS_AcmeWriteOne(sPath, pSplit, strlen(pSplit)) ) return false;
	snprintf(sPath, sizeof(sPath), "%s/key.pem", pGroup->sOutDir);
	if ( !XS_AcmeWriteOne(sPath, pGrant->sKeyPem, strlen(pGrant->sKeyPem)) ) return false;
	snprintf(sPath, sizeof(sPath), "%s/fullchain.pem", pGroup->sOutDir);
	if ( !XS_AcmeWriteOne(sPath, pGrant->sFullchainPem,
		strlen(pGrant->sFullchainPem)) ) return false;
	return true;
}

/* ---------------- 续签周期 ---------------- */

static void XS_AcmeRunGroup(XS_AcmeDaemon* pDaemon, XS_AcmeGroup* pGroup)
{
	xstrview* tDomains;
	char sErr[256];
	size_t i;
	size_t c;

	if ( pGroup->iServerCount == 0 ) {
		printf("[xs] acme '%s': out_dir not referenced by any tls host, skip\n",
			pGroup->sName);
		return;
	}
	if ( !xrtDirCreateAll(pGroup->sOutDir) ) {
		printf("[xs] acme '%s': create out_dir failed: %s\n",
			pGroup->sName, pGroup->sOutDir);
		return;
	}
	tDomains = (xstrview*)xrtCalloc(pGroup->iDomainCount, sizeof(xstrview));
	if ( tDomains == NULL ) return;
	for ( i = 0; i < pGroup->iDomainCount; i++ ) {
		tDomains[i] = xrtStrView(pGroup->sDomains[i]);
	}

	/* CA 回退链：按序逐 CA 尝试，首个成功者胜出。 */
	for ( c = 0; c < pDaemon->iCaCount; c++ ) {
		XS_AcmeCaRt* pCa = &pDaemon->pCas[c];
		xacmednaliconfig tAli;
		xacmednsprovider tProvider;
		xacmeaccountconfig tAccount;
		xacmeobtainconfig tObtain;
		xacmeissuegrant tGrant;
		const xerror* pError;
		bool bRenewed = false;
		bool bOK;

		/* CA 之间也响应停机 */
		xrtMutexLock(pDaemon->pLock);
		if ( pDaemon->bStop ) {
			xrtMutexUnlock(pDaemon->pLock);
			xrtFree(tDomains);
			return;
		}
		xrtMutexUnlock(pDaemon->pLock);

		/* 每次 CA 尝试全新 provider（RecordId 状态不跨 CA） */
		xrtAcmeDnsAliConfigInit(&tAli);
		tAli.sAccessKeyId = pDaemon->sAliKeyId;
		tAli.sAccessKeySecret = pDaemon->sAliKeySecret;
		memset(&tProvider, 0, sizeof(tProvider));
		if ( !xrtAcmeDnsAli(&tAli, pDaemon->pEngine, &tProvider) ) {
			printf("[xs] acme '%s': alidns provider init failed\n", pGroup->sName);
			xrtFree(tDomains);
			return;
		}

		memset(&tAccount, 0, sizeof(tAccount));
		tAccount.sDirectoryUrl = pCa->sUrl;
		tAccount.sAccountKeyPem = pDaemon->sAccountKeyPem;
		tAccount.Eab.sKid = pCa->sEabKid;
		tAccount.Eab.sHmac = pCa->sEabHmac;
		tAccount.sContactEmail = pDaemon->tConfig.sEmail;
		xrtAcmeObtainConfigInit(&tObtain);
		tObtain.pAccount = &tAccount;
		tObtain.sCaPem = pCa->sCaPem;		/* 可空 = 系统信任库 */
		tObtain.pBorrowedEngine = pDaemon->pEngine;
		tObtain.uIssueTimeoutUs = UINT64_C(300000000);	/* 单 CA 5 分钟 */
		tObtain.sStoreRoot = pDaemon->sStoreRoot;
		tObtain.iRenewalDays = pDaemon->tConfig.iRenewalDays;

		memset(&tGrant, 0, sizeof(tGrant));
		bOK = xrtAcmeObtain(&tObtain, tDomains, pGroup->iDomainCount,
			&tProvider, &tGrant, &bRenewed);
		xrtAcmeDnsAliProviderUnit(&tProvider);

		if ( !bOK ) {
			pError = xrtGetError();
			printf("[xs] acme '%s': ca '%s' failed: %s%s\n",
				pGroup->sName, pCa->sName,
				(pError != NULL && xrtErrorMessage(pError) != NULL)
					? xrtErrorMessage(pError) : "unknown",
				(c + 1 < pDaemon->iCaCount)
					? ", falling back to next ca" : "");
			continue;
		}
		if ( !bRenewed ) {
			printf("[xs] acme '%s': certificate valid (via %s), no renew needed\n",
				pGroup->sName, pCa->sName);
			xrtAcmeGrantUnit(&tGrant);
			xrtFree(tDomains);
			return;
		}
		if ( !XS_AcmeWriteGrant(pGroup, &tGrant) ) {
			printf("[xs] acme '%s': write cert files failed: %s\n",
				pGroup->sName, pGroup->sOutDir);
			xrtAcmeGrantUnit(&tGrant);
			xrtFree(tDomains);
			return;
		}
		xrtAcmeGrantUnit(&tGrant);
		xrtFree(tDomains);
		printf("[xs] acme '%s': renewed via %s, refreshing %u server(s)\n",
			pGroup->sName, pCa->sName, (unsigned)pGroup->iServerCount);
		for ( i = 0; i < pGroup->iServerCount; i++ ) {
			if ( xsTlsRefresh(pGroup->sServers[i], sErr, sizeof(sErr)) ) {
				printf("[xs] acme '%s': tls refresh '%s' ok\n",
					pGroup->sName, pGroup->sServers[i]);
			} else {
				printf("[xs] acme '%s': tls refresh '%s' failed: %s\n",
					pGroup->sName, pGroup->sServers[i], sErr);
			}
		}
		return;
	}
	printf("[xs] acme '%s': all %u ca(s) failed, retry next cycle\n",
		pGroup->sName, (unsigned)pDaemon->iCaCount);
	xrtFree(tDomains);
}

static void XS_AcmeRunCycle(XS_AcmeDaemon* pDaemon)
{
	size_t i;

	for ( i = 0; i < pDaemon->tConfig.iGroupCount; i++ ) {
		xrtMutexLock(pDaemon->pLock);
		if ( pDaemon->bStop ) {
			xrtMutexUnlock(pDaemon->pLock);
			return;
		}
		xrtMutexUnlock(pDaemon->pLock);
		XS_AcmeRunGroup(pDaemon, &pDaemon->tConfig.pGroups[i]);
	}
}

/* ---------------- 守护线程 ---------------- */

static int32 XS_AcmeThreadProc(ptr pData)
{
	XS_AcmeDaemon* pDaemon = (XS_AcmeDaemon*)pData;
	uint64 uInterval = (uint64)pDaemon->tConfig.iCheckIntervalHours *
		UINT64_C(3600000000);			/* 小时 → 微秒 */
	xdeadline tNext = xrtDeadlineAfter(UINT64_C(300000000));	/* 首查 5 分钟 */

	for ( ; ; ) {
		xrtMutexLock(pDaemon->pLock);
		while ( !pDaemon->bStop && !xrtDeadlineExpired(tNext) ) {
			xrtCondWaitUntil(pDaemon->pWake, pDaemon->pLock, tNext);
		}
		if ( pDaemon->bStop ) {
			xrtMutexUnlock(pDaemon->pLock);
			break;
		}
		xrtMutexUnlock(pDaemon->pLock);
		XS_AcmeRunCycle(pDaemon);
		tNext = xrtDeadlineAfter(uInterval);
	}
	printf("[xs] acme: daemon stopped\n");
	return 0;
}

/* ---------------- 生命周期 ---------------- */

static void XS_AcmeDaemonCleanup(XS_AcmeDaemon* pDaemon)
{
	size_t i;

	if ( pDaemon->pThread != NULL ) { /* 未 join 的异常路径：不触达 */ }
	if ( pDaemon->pWake != NULL ) xrtCondDestroy(pDaemon->pWake);
	if ( pDaemon->pLock != NULL ) xrtMutexDestroy(pDaemon->pLock);
	pDaemon->pWake = NULL;
	pDaemon->pLock = NULL;
	if ( pDaemon->pEngine != NULL ) xrtNetEngineDestroy(pDaemon->pEngine);
	pDaemon->pEngine = NULL;
	xrtFree(pDaemon->sAccountKeyPem);
	xrtFree(pDaemon->sAliKeyId);
	xrtFree(pDaemon->sAliKeySecret);
	xrtFree(pDaemon->sStoreRoot);
	for ( i = 0; i < pDaemon->iCaCount; i++ ) {
		xrtFree(pDaemon->pCas[i].sName);
		xrtFree(pDaemon->pCas[i].sUrl);
		xrtFree(pDaemon->pCas[i].sCaPem);
		xrtFree(pDaemon->pCas[i].sEabKid);
		xrtFree(pDaemon->pCas[i].sEabHmac);
	}
	xrtFree(pDaemon->pCas);
	pDaemon->pCas = NULL;
	pDaemon->iCaCount = 0;
	XS_AcmeConfigFree(&pDaemon->tConfig);
}

/* 装配成功后调用：快照配置、构建索引、起线程。 */
static bool XS_AcmeDaemonStart(XS_App* pApp, XS_AcmeConfig* pConfig,
	char* sErr, size_t iErrCap)
{
	XS_AcmeDaemon* pDaemon = &g_XS_Acme;
	xnetengineconfig tEngine;
	const char* sAppPath = XS_AppPath();
	xvalue* pJson;
	char* sText;
	size_t i;

	if ( pDaemon->bStarted || pConfig == NULL || !pConfig->bEnabled ) return true;
	if ( !XS_AcmeConfigClone(pConfig, &pDaemon->tConfig) ) {
		snprintf(sErr, iErrCap, "acme: out of memory (config snapshot)");
		return false;
	}
	/* 相对路径 → appPath 绝对 */
	{
		char* sAbs;

		sAbs = XS_AcmeResolve(pDaemon->tConfig.sAccountKeyFile, sAppPath);
		xrtFree(pDaemon->tConfig.sAccountKeyFile);
		pDaemon->tConfig.sAccountKeyFile = sAbs;
		sAbs = XS_AcmeResolve(pDaemon->tConfig.sDnsKeyFile, sAppPath);
		xrtFree(pDaemon->tConfig.sDnsKeyFile);
		pDaemon->tConfig.sDnsKeyFile = sAbs;
		for ( i = 0; i < pDaemon->tConfig.iGroupCount; i++ ) {
			XS_AcmeGroup* pGroup = &pDaemon->tConfig.pGroups[i];

			sAbs = XS_AcmeResolve(pGroup->sOutDir, sAppPath);
			xrtFree(pGroup->sOutDir);
			pGroup->sOutDir = sAbs;
			if ( pGroup->sOutDir == NULL ) {
				snprintf(sErr, iErrCap, "acme: resolve out_dir failed");
				goto Fail;
			}
		}
		for ( i = 0; i < pDaemon->tConfig.iCaCount; i++ ) {
			XS_AcmeCa* pCa = &pDaemon->tConfig.pCas[i];

			sAbs = XS_AcmeResolve(pCa->sCaFile, sAppPath);
			xrtFree(pCa->sCaFile);
			pCa->sCaFile = sAbs;
			sAbs = XS_AcmeResolve(pCa->sEabKidFile, sAppPath);
			xrtFree(pCa->sEabKidFile);
			pCa->sEabKidFile = sAbs;
			sAbs = XS_AcmeResolve(pCa->sEabHmacFile, sAppPath);
			xrtFree(pCa->sEabHmacFile);
			pCa->sEabHmacFile = sAbs;
		}
	}
	/* 运行期 CA 链：展示名解析 + 信任锚/EAB 文件读入（fail-fast） */
	{
		pDaemon->iCaCount = pDaemon->tConfig.iCaCount;
		pDaemon->pCas = (XS_AcmeCaRt*)xrtCalloc(pDaemon->iCaCount,
			sizeof(XS_AcmeCaRt));
		if ( pDaemon->pCas == NULL ) {
			snprintf(sErr, iErrCap, "acme: out of memory (ca chain)");
			goto Fail;
		}
		for ( i = 0; i < pDaemon->iCaCount; i++ ) {
			XS_AcmeCa* pCfg = &pDaemon->tConfig.pCas[i];
			XS_AcmeCaRt* pRt = &pDaemon->pCas[i];

			if ( pCfg->sName != NULL ) {
				pRt->sName = xrtStrDup(pCfg->sName);
			} else {
				/* 取 URL 域名："://" 之后到下一个 '/' */
				const char* pHost = strstr(pCfg->sUrl, "://");
				const char* pEnd;

				if ( pHost != NULL ) {
					pHost += 3;
					pEnd = strchr(pHost, '/');
					pRt->sName = xrtStrDupN(pHost,
						pEnd != NULL ? (size_t)(pEnd - pHost) : strlen(pHost));
				}
				if ( pRt->sName == NULL ) pRt->sName = xrtStrDup("ca");
			}
			pRt->sUrl = xrtStrDup(pCfg->sUrl);
			if ( pRt->sName == NULL || pRt->sUrl == NULL ) {
				snprintf(sErr, iErrCap, "acme: out of memory (ca entry)");
				goto Fail;
			}
			if ( pCfg->sCaFile != NULL ) {
				pRt->sCaPem = XS_AcmeReadText(pCfg->sCaFile);
				if ( pRt->sCaPem == NULL ) {
					snprintf(sErr, iErrCap, "acme ca '%s': ca_file unreadable: %s",
						pRt->sName, pCfg->sCaFile);
					goto Fail;
				}
			}
			if ( pCfg->sEabKidFile != NULL ) {
				pRt->sEabKid = XS_AcmeReadText(pCfg->sEabKidFile);
				pRt->sEabHmac = XS_AcmeReadText(pCfg->sEabHmacFile);
				if ( pRt->sEabKid == NULL || pRt->sEabHmac == NULL ) {
					snprintf(sErr, iErrCap,
						"acme ca '%s': eab files unreadable (%s)",
						pRt->sName, pCfg->sEabKidFile);
					goto Fail;
				}
				/* 去尾随换行 */
				XS_AcmeTrimLine(pRt->sEabKid);
				XS_AcmeTrimLine(pRt->sEabHmac);
			}
		}
	}
	pDaemon->sStoreRoot = xrtPathJoin(
		sAppPath != NULL ? sAppPath : ".", "acme-store");
	if ( pDaemon->sStoreRoot == NULL ) {
		snprintf(sErr, iErrCap, "acme: out of memory (store root)");
		goto Fail;
	}
	pDaemon->sAccountKeyPem = XS_AcmeReadText(pDaemon->tConfig.sAccountKeyFile);
	if ( pDaemon->sAccountKeyPem == NULL &&
	     pDaemon->tConfig.sAccountKeyFile != NULL ) {
		snprintf(sErr, iErrCap, "acme account_key_file unreadable: %s",
			pDaemon->tConfig.sAccountKeyFile);
		goto Fail;
	}
	/* DNS 凭据 JSON：{"access_key_id": "...", "access_key_secret": "..."} */
	sText = XS_AcmeReadText(pDaemon->tConfig.sDnsKeyFile);
	if ( sText == NULL ) {
		snprintf(sErr, iErrCap, "acme dns_key_file unreadable: %s",
			pDaemon->tConfig.sDnsKeyFile);
		goto Fail;
	}
	pJson = xrtJsonParse(xrtStrView(sText));
	xrtFree(sText);
	if ( pJson == NULL || xrtValueType(pJson) != XVALUE_OBJECT ) {
		if ( pJson != NULL ) xrtValueRelease(pJson);
		snprintf(sErr, iErrCap, "acme dns_key_file expect json object");
		goto Fail;
	}
	{
		xvalue* pItem;

		pItem = xrtValueObjectGet(pJson, XRT_STR_LITERAL("access_key_id"));
		if ( pItem != NULL && xrtValueGetString(pItem, &(xstrview){0}) ) {
			xstrview tView;

			(void)xrtValueGetString(pItem, &tView);
			pDaemon->sAliKeyId = XS_AcmeDupN(tView.Data, tView.Size);
		}
		pItem = xrtValueObjectGet(pJson, XRT_STR_LITERAL("access_key_secret"));
		if ( pItem != NULL && xrtValueGetString(pItem, &(xstrview){0}) ) {
			xstrview tView;

			(void)xrtValueGetString(pItem, &tView);
			pDaemon->sAliKeySecret = XS_AcmeDupN(tView.Data, tView.Size);
		}
	}
	xrtValueRelease(pJson);
	if ( pDaemon->sAliKeyId == NULL || pDaemon->sAliKeySecret == NULL ) {
		snprintf(sErr, iErrCap,
			"acme dns_key_file missing access_key_id/access_key_secret");
		goto Fail;
	}

	xrtNetEngineConfigInit(&tEngine);
	tEngine.Workers = 1;
	pDaemon->pEngine = xrtNetEngineCreate(&tEngine);
	if ( pDaemon->pEngine == NULL || !xrtNetEngineStart(pDaemon->pEngine) ) {
		snprintf(sErr, iErrCap, "acme engine create/start failed");
		goto Fail;
	}
	pDaemon->pLock = xrtMutexCreate();
	pDaemon->pWake = xrtCondCreate();
	if ( pDaemon->pLock == NULL || pDaemon->pWake == NULL ) {
		snprintf(sErr, iErrCap, "acme sync primitives create failed");
		goto Fail;
	}
	XS_AcmeBuildIndex(pDaemon, pApp);
	for ( i = 0; i < pDaemon->tConfig.iGroupCount; i++ ) {
		XS_AcmeGroup* pGroup = &pDaemon->tConfig.pGroups[i];

		printf("[xs] acme group '%s': %u domain(s) -> %s (%u server(s))\n",
			pGroup->sName, (unsigned)pGroup->iDomainCount, pGroup->sOutDir,
			(unsigned)pGroup->iServerCount);
	}
	pDaemon->bStop = false;
	pDaemon->pThread = xrtThreadCreate(XS_AcmeThreadProc, (ptr)pDaemon, 0);
	if ( pDaemon->pThread == NULL ) {
		snprintf(sErr, iErrCap, "acme thread create failed");
		goto Fail;
	}
	pDaemon->bStarted = true;
	for ( i = 0; i < pDaemon->iCaCount; i++ ) {
		printf("[xs] acme ca chain [%u]: '%s' -> %s%s%s\n",
			(unsigned)i, pDaemon->pCas[i].sName, pDaemon->pCas[i].sUrl,
			(pDaemon->pCas[i].sEabKid != NULL) ? " (eab)" : "",
			(pDaemon->pCas[i].sCaPem != NULL) ? " (custom trust)" : "");
	}
	printf("[xs] acme: daemon started (%u group(s), %u ca(s) in fallback chain, "
		"check every %dh, first check in 5min)\n",
		(unsigned)pDaemon->tConfig.iGroupCount, (unsigned)pDaemon->iCaCount,
		pDaemon->tConfig.iCheckIntervalHours);
	return true;

Fail:
	XS_AcmeDaemonCleanup(pDaemon);
	return false;
}

/* 停机：条件变量立即唤醒，限时 join（组内当前步骤跑完即让出）。 */
static void XS_AcmeDaemonStop(void)
{
	XS_AcmeDaemon* pDaemon = &g_XS_Acme;

	if ( !pDaemon->bStarted ) return;
	xrtMutexLock(pDaemon->pLock);
	pDaemon->bStop = true;
	xrtCondSignal(pDaemon->pWake);
	xrtMutexUnlock(pDaemon->pLock);
	if ( pDaemon->pThread != NULL ) {
		xrtThreadWaitFor(pDaemon->pThread, UINT64_C(15000000));	/* 15s */
	}
	printf("[xs] acme: daemon stop requested\n");
}

#else /* !XS_USE_XACME */

/* 无扩展变体：消费节点防误配静默；daemon 恒空操作。 */

static bool XS_AcmeRootParse(xvalue* pRoot, XS_AcmeConfig** ppConfig,
	char* sErr, size_t iErrCap)
{
	(void)sErr; (void)iErrCap;
	*ppConfig = NULL;
	if ( xrtValueObjectGet(pRoot, XS_ConfigKey("acme")) != NULL ) {
		printf("[xs] acme: config present but xacme extension not built in, ignored\n");
	}
	return true;
}

static bool XS_AcmeDaemonStart(XS_App* pApp, XS_AcmeConfig* pConfig,
	char* sErr, size_t iErrCap)
{
	(void)pApp; (void)pConfig; (void)sErr; (void)iErrCap;
	return true;
}

static void XS_AcmeDaemonStop(void) {}

#endif /* XS_USE_XACME */

#endif /* XS_CORE_ACME_H */
