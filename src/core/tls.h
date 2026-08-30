#ifndef XS_CORE_TLS_H
#define XS_CORE_TLS_H

/*
 * xs3 TLS 身份装载与 SNI 选择器（设计 §4.1 host 级证书 / §10.3 证书热替换基础）
 * - PEM 证书/私钥经 xrt fs 读取（UTF-8 路径），xrtPemDecode 转 DER 后构造 identity
 * - 证书链：同文件内多个 CERTIFICATE 块按序作为链
 * - 先解析叶证书公钥算法，再调用唯一匹配的身份构造器；构造器继续核对证书/私钥
 * - SNI：xtlsserverselectproc 按 ClientHello ServerName 匹配 host->Host（分号分隔、忽略大小写）
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"
#include "../runtime/listener_slot.h"
#include "engine.h"

/* 进程级共享 TLS 上下文由 main 显式管理；装配线程只借用，避免惰性初始化竞态。 */
static xtlscontext* g_XS_TlsContext;
static xmutex* g_XS_TlsLogLock;
static uint64 g_XS_TlsLogWindow;
static uint32 g_XS_TlsLogCount;
static uint32 g_XS_TlsLogSuppressed;

#define XS_TLS_LOG_WINDOW_US	(10u * 1000u * 1000u)
#define XS_TLS_LOG_BURST	8u

static bool XS_TlsRuntimeInit(void)
{
	xtlscontextconfig tCfg;

	if ( g_XS_TlsContext != NULL ) return true;
	xrtTlsContextConfigInit(&tCfg);
	g_XS_TlsContext = xrtTlsContextCreate(&tCfg);
	if ( g_XS_TlsContext != NULL && g_XS_TlsLogLock == NULL ) {
		g_XS_TlsLogLock = xrtMutexCreate();
	}
	return g_XS_TlsContext != NULL;
}

static xtlscontext* XS_TlsSharedContext(void)
{
	return g_XS_TlsContext;
}

/* 单连接握手失败不影响 listener，但必须留下可诊断根因。 */
static void XS_TlsHandshakeError(
	xtlslistener* pListener,
	const xerror* pError,
	ptr pData)
{
	uint64 tNow = (uint64)xrtNow();
	uint32 iSuppressed = 0;
	bool bPrint = true;

	(void)pListener;
	(void)pData;
	if ( g_XS_TlsLogLock != NULL ) {
		xrtMutexLock(g_XS_TlsLogLock);
		if ( g_XS_TlsLogWindow == 0 || tNow < g_XS_TlsLogWindow ||
		     tNow - g_XS_TlsLogWindow >= XS_TLS_LOG_WINDOW_US ) {
			iSuppressed = g_XS_TlsLogSuppressed;
			g_XS_TlsLogWindow = tNow;
			g_XS_TlsLogCount = 0;
			g_XS_TlsLogSuppressed = 0;
		}
		if ( g_XS_TlsLogCount < XS_TLS_LOG_BURST ) {
			g_XS_TlsLogCount++;
		} else {
			g_XS_TlsLogSuppressed++;
			bPrint = false;
		}
		xrtMutexUnlock(g_XS_TlsLogLock);
	}
	if ( iSuppressed > 0 ) {
		printf("[xs] tls handshake failures suppressed: %u in previous window\n",
			(unsigned)iSuppressed);
	}
	if ( bPrint ) {
		printf("[xs] tls handshake failed: %s\n",
			pError != NULL ? xrtErrorMessage(pError) : "unknown error");
	}
}

static void XS_TlsRuntimeUnit(void)
{
	if ( g_XS_TlsContext != NULL ) {
		xrtTlsContextRelease(g_XS_TlsContext);
		g_XS_TlsContext = NULL;
	}
	if ( g_XS_TlsLogLock != NULL ) {
		xrtMutexDestroy(g_XS_TlsLogLock);
		g_XS_TlsLogLock = NULL;
	}
	g_XS_TlsLogWindow = 0;
	g_XS_TlsLogCount = 0;
	g_XS_TlsLogSuppressed = 0;
}

/* 相对路径按 appPath 解析（结果 xrtFree 释放） */
static str XS_TlsResolvePath(const char* sPath)
{
	if ( sPath == NULL ) {
		return NULL;
	}
	if ( xrtPathIsAbs(sPath) ) {
		return xrtStrDup(sPath);
	}
	return xrtPathJoin(XS_AppPath(), sPath);
}

/* 从 PEM 文本严格提取指定标签的全部 DER 块。任何残缺/不可解码块都使整链失败。 */
typedef struct XS_TlsDer {
	bytes			pData;
	size_t			iSize;
} XS_TlsDer;

static bool XS_TlsSizeMul(size_t iCount, size_t iElement, size_t* piBytes)
{
	if ( piBytes == NULL || (iCount != 0 && iElement > SIZE_MAX / iCount) ) {
		return false;
	}
	*piBytes = iCount * iElement;
	return true;
}

static size_t XS_TlsTokenCount(const char* sText, size_t iSize, const char* sToken)
{
	size_t iToken = strlen(sToken);
	size_t i;
	size_t iCount = 0;

	if ( sText == NULL || iToken == 0 || iToken > iSize ) return 0;
	for ( i = 0; iToken <= iSize - i; ) {
		if ( memcmp(sText + i, sToken, iToken) == 0 ) {
			iCount++;
			i += iToken;
		} else {
			i++;
		}
	}
	return iCount;
}

static void XS_TlsDerFree(XS_TlsDer* arrBlocks, uint32 iCount)
{
	uint32 i;

	for ( i = 0; i < iCount; i++ ) xrtFree(arrBlocks[i].pData);
	xrtFree(arrBlocks);
}

static bool XS_TlsPemBlocks(
	const char* sText,
	size_t iSize,
	const char* sLabel,
	XS_TlsDer** parrOut,
	uint32* piCount)
{
	XS_TlsDer* arrBlocks = NULL;
	uint32 iCount = 0;
	size_t iOffset = 0;
	char sBegin[96];
	char sEnd[96];
	size_t iBegin;
	size_t iEnd;

	*parrOut = NULL;
	*piCount = 0;
	if ( snprintf(sBegin, sizeof(sBegin), "-----BEGIN %s-----", sLabel) < 0 ||
	     snprintf(sEnd, sizeof(sEnd), "-----END %s-----", sLabel) < 0 ) {
		return false;
	}
	iBegin = XS_TlsTokenCount(sText, iSize, sBegin);
	iEnd = XS_TlsTokenCount(sText, iSize, sEnd);
	if ( iBegin == 0 || iBegin != iEnd || iBegin > UINT32_MAX ) return false;

	for ( ; ; ) {
		xpemblock tBlock;
		bytes pDer;
		size_t iDer = 0;
		size_t iNewCount;
		size_t iNewBytes;

		if ( !xrtPemFind(sText + iOffset, iSize - iOffset, sLabel, &tBlock) ) {
			break;
		}
		pDer = xrtPemDecodeNew(&tBlock, &iDer);
		if ( pDer == NULL ) {
			goto failed;
		}
		iNewCount = (size_t)iCount + 1u;
		if ( !XS_TlsSizeMul(iNewCount, sizeof(XS_TlsDer), &iNewBytes) ) {
			xrtFree(pDer);
			goto failed;
		}
		{
			XS_TlsDer* pNew = (XS_TlsDer*)xrtRealloc(arrBlocks, iNewBytes);
			if ( pNew == NULL ) {
				xrtFree(pDer);
				goto failed;
			}
			arrBlocks = pNew;
		}
		arrBlocks[iCount].pData = pDer;
		arrBlocks[iCount].iSize = iDer;
		iCount++;
		/* 越过本块继续找（Raw 视图相对 sText+iOffset） */
		iOffset += (size_t)(tBlock.Raw.Data - (sText + iOffset)) + tBlock.Raw.Size;
	}
	if ( iCount != (uint32)iBegin ) goto failed;
	*parrOut = arrBlocks;
	*piCount = iCount;
	return true;

failed:
	XS_TlsDerFree(arrBlocks, iCount);
	return false;
}

typedef struct XS_TlsPrivateKeyInfo {
	xx509keytype		Type;
	xx509curve		Curve;
} XS_TlsPrivateKeyInfo;

static const char* XS_TlsKeyName(xx509keytype Type, xx509curve Curve)
{
	switch ( Type ) {
	case X509_KEY_RSA: return "RSA";
	case X509_KEY_RSA_PSS: return "RSA-PSS";
	case X509_KEY_EC:
		switch ( Curve ) {
		case X509_CURVE_P256: return "EC P-256";
		case X509_CURVE_P384: return "EC P-384";
		case X509_CURVE_P521: return "EC P-521";
		default: return "EC (unknown curve)";
		}
	case X509_KEY_ED25519: return "Ed25519";
	case X509_KEY_ED448: return "Ed448";
	case X509_KEY_X25519: return "X25519";
	case X509_KEY_X448: return "X448";
	default: return "unknown";
	}
}

/* 只解析未加密 PKCS#8 的算法标识，用于在调用构造器前给出类型冲突诊断。
 * 完整密钥合法性与公钥匹配仍由 xrt TLS identity 构造器负责。 */
static XS_TlsPrivateKeyInfo XS_TlsPkcs8KeyInfo(xbytesview Der)
{
	static const unsigned char arrRsaOid[] = {
		0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01
	};
	static const unsigned char arrRsaPssOid[] = {
		0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0a
	};
	static const unsigned char arrEcOid[] = {
		0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01
	};
	static const unsigned char arrP256Oid[] = {
		0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07
	};
	static const unsigned char arrP384Oid[] = {
		0x2b, 0x81, 0x04, 0x00, 0x22
	};
	static const unsigned char arrP521Oid[] = {
		0x2b, 0x81, 0x04, 0x00, 0x23
	};
	XS_TlsPrivateKeyInfo tInfo = { X509_KEY_UNKNOWN, X509_CURVE_UNKNOWN };
	xdercursor tRoot;
	xdercursor tFields;
	xdercursor tAlgorithm;
	xdervalue tSequence;
	xdervalue tVersion;
	xdervalue tAlgorithmValue;
	xdervalue tOid;
	xdervalue tParameters;

	if ( !xrtDerInit(&tRoot, Der.Data, Der.Size) ||
	     xrtDerRead(&tRoot, &tSequence) != XDER_VALUE ||
	     !xrtDerDone(&tRoot) ||
	     !xrtDerIs(&tSequence, XASN1_UNIVERSAL, XASN1_SEQUENCE, true) ||
	     !xrtDerEnter(&tSequence, &tFields) ||
	     xrtDerRead(&tFields, &tVersion) != XDER_VALUE ||
	     !xrtDerIs(&tVersion, XASN1_UNIVERSAL, XASN1_INTEGER, false) ||
	     xrtDerRead(&tFields, &tAlgorithmValue) != XDER_VALUE ||
	     !xrtDerIs(&tAlgorithmValue, XASN1_UNIVERSAL, XASN1_SEQUENCE, true) ||
	     !xrtDerEnter(&tAlgorithmValue, &tAlgorithm) ||
	     xrtDerRead(&tAlgorithm, &tOid) != XDER_VALUE ) {
		return tInfo;
	}
	if ( xrtDerOidEqual(&tOid, arrRsaOid, sizeof(arrRsaOid)) ) {
		tInfo.Type = X509_KEY_RSA;
	} else if ( xrtDerOidEqual(&tOid, arrRsaPssOid, sizeof(arrRsaPssOid)) ) {
		tInfo.Type = X509_KEY_RSA_PSS;
	} else if ( xrtDerOidEqual(&tOid, arrEcOid, sizeof(arrEcOid)) ) {
		tInfo.Type = X509_KEY_EC;
		if ( xrtDerRead(&tAlgorithm, &tParameters) == XDER_VALUE ) {
			if ( xrtDerOidEqual(&tParameters, arrP256Oid, sizeof(arrP256Oid)) ) {
				tInfo.Curve = X509_CURVE_P256;
			} else if ( xrtDerOidEqual(&tParameters, arrP384Oid, sizeof(arrP384Oid)) ) {
				tInfo.Curve = X509_CURVE_P384;
			} else if ( xrtDerOidEqual(&tParameters, arrP521Oid, sizeof(arrP521Oid)) ) {
				tInfo.Curve = X509_CURVE_P521;
			}
		}
	}
	return tInfo;
}

static bool XS_TlsKeyFamilyMismatch(
	const xx509pubkey* pCertificate,
	XS_TlsPrivateKeyInfo tPrivate)
{
	bool bCertRsa = pCertificate->Type == X509_KEY_RSA ||
		pCertificate->Type == X509_KEY_RSA_PSS;
	bool bPrivateRsa = tPrivate.Type == X509_KEY_RSA ||
		tPrivate.Type == X509_KEY_RSA_PSS;

	return (bCertRsa && tPrivate.Type == X509_KEY_EC) ||
		(pCertificate->Type == X509_KEY_EC && bPrivateRsa);
}

/* 由叶证书唯一选择 RSA / P-256 / P-384 构造器，避免失败原因被后续尝试覆盖。 */
static xtlsidentity* XS_TlsBuildIdentity(
	XS_TlsDer* arrCerts,
	uint32 iCertCount,
	const char* sKeyText,
	size_t iKeySize,
	char* sErr,
	size_t iErrCap)
{
	static const char* arrKeyLabels[] = { "RSA PRIVATE KEY", "PRIVATE KEY", "EC PRIVATE KEY" };
	xpemblock tKeyBlock;
	bytes pKeyDer = NULL;
	size_t iKeyDer = 0;
	xbytesview tKey;
	xtlsidentity* pIdentity = NULL;
	xbytesview* arrViews;
	uint32 i;
	uint32 j;
	size_t iKeyBlocks = 0;
	size_t iViewsBytes;
	uint32 iKeyLabel = UINT32_MAX;
	xx509cert tLeaf;
	xx509pubkey tPublicKey;
	XS_TlsPrivateKeyInfo tPrivate = { X509_KEY_UNKNOWN, X509_CURVE_UNKNOWN };
	const xerror* pError;
	const char* sCertificateType;

	if ( iCertCount == 0 || arrCerts == NULL ) {
		snprintf(sErr, iErrCap, "tls identity requires a leaf certificate");
		return NULL;
	}
	if ( !xrtX509Parse(arrCerts[0].pData, arrCerts[0].iSize, &tLeaf) ||
	     !xrtX509PublicKey(&tLeaf, &tPublicKey) ) {
		pError = xrtGetError();
		snprintf(sErr, iErrCap, "tls leaf certificate public key parse failed: %s",
			pError != NULL ? xrtErrorMessage(pError) : "unknown error");
		return NULL;
	}
	sCertificateType = XS_TlsKeyName(tPublicKey.Type, tPublicKey.Curve);
	if ( !((tPublicKey.Type == X509_KEY_RSA || tPublicKey.Type == X509_KEY_RSA_PSS) ||
	       (tPublicKey.Type == X509_KEY_EC &&
	        (tPublicKey.Curve == X509_CURVE_P256 || tPublicKey.Curve == X509_CURVE_P384))) ) {
		snprintf(sErr, iErrCap, "tls leaf certificate key is unsupported: %s",
			sCertificateType);
		return NULL;
	}

	/* 密钥文件同样只接受一个完整且可识别的私钥块，避免静默忽略坏尾部。 */
	for ( j = 0; j < 3; j++ ) {
		char sBegin[96];
		char sEnd[96];
		size_t iBegin;
		size_t iEnd;

		(void)snprintf(sBegin, sizeof(sBegin), "-----BEGIN %s-----", arrKeyLabels[j]);
		(void)snprintf(sEnd, sizeof(sEnd), "-----END %s-----", arrKeyLabels[j]);
		iBegin = XS_TlsTokenCount(sKeyText, iKeySize, sBegin);
		iEnd = XS_TlsTokenCount(sKeyText, iKeySize, sEnd);
		if ( iBegin != iEnd ) {
			snprintf(sErr, iErrCap, "tls private key PEM block is incomplete: %s",
				arrKeyLabels[j]);
			return NULL;
		}
		iKeyBlocks += iBegin;
		if ( iBegin == 1 && pKeyDer == NULL &&
		     xrtPemFind(sKeyText, iKeySize, arrKeyLabels[j], &tKeyBlock) ) {
			pKeyDer = xrtPemDecodeNew(&tKeyBlock, &iKeyDer);
			iKeyLabel = j;
		}
	}
	if ( iKeyBlocks != 1 || pKeyDer == NULL ) {
		pError = xrtGetError();
		if ( iKeyBlocks != 1 ) {
			snprintf(sErr, iErrCap,
				"tls private key expects exactly one supported PEM block; found %zu",
				iKeyBlocks);
		} else {
			snprintf(sErr, iErrCap, "tls private key PEM decode failed: %s",
				pError != NULL ? xrtErrorMessage(pError) : "unknown error");
		}
		xrtFree(pKeyDer);
		return NULL;
	}
	tKey.Data = (const unsigned char*)pKeyDer;
	tKey.Size = iKeyDer;
	if ( iKeyLabel == 0 ) {
		tPrivate.Type = X509_KEY_RSA;
	} else if ( iKeyLabel == 2 ) {
		tPrivate.Type = X509_KEY_EC;
	} else {
		tPrivate = XS_TlsPkcs8KeyInfo(tKey);
	}
	if ( XS_TlsKeyFamilyMismatch(&tPublicKey, tPrivate) ) {
		snprintf(sErr, iErrCap, "tls certificate uses %s but private key uses %s",
			sCertificateType, XS_TlsKeyName(tPrivate.Type, tPrivate.Curve));
		xrtFree(pKeyDer);
		return NULL;
	}
	if ( tPublicKey.Type == X509_KEY_EC && tPrivate.Type == X509_KEY_EC &&
	     tPrivate.Curve != X509_CURVE_UNKNOWN &&
	     tPrivate.Curve != tPublicKey.Curve ) {
		snprintf(sErr, iErrCap, "tls certificate uses %s but private key uses %s",
			sCertificateType, XS_TlsKeyName(tPrivate.Type, tPrivate.Curve));
		xrtFree(pKeyDer);
		return NULL;
	}

	if ( !XS_TlsSizeMul((size_t)iCertCount, sizeof(xbytesview), &iViewsBytes) ) {
		snprintf(sErr, iErrCap, "tls certificate chain is too large");
		xrtFree(pKeyDer);
		return NULL;
	}
	arrViews = (xbytesview*)xrtCalloc(1, iViewsBytes);
	if ( arrViews != NULL ) {
		for ( i = 0; i < iCertCount; i++ ) {
			arrViews[i].Data = (const unsigned char*)arrCerts[i].pData;
			arrViews[i].Size = arrCerts[i].iSize;
		}
		if ( tPublicKey.Type == X509_KEY_RSA || tPublicKey.Type == X509_KEY_RSA_PSS ) {
			pIdentity = xrtTlsIdentityRsa(arrViews, iCertCount, tKey);
		} else if ( tPublicKey.Curve == X509_CURVE_P256 ) {
			pIdentity = xrtTlsIdentityP256(arrViews, iCertCount, tKey);
		} else {
			pIdentity = xrtTlsIdentityP384(arrViews, iCertCount, tKey);
		}
		if ( pIdentity == NULL ) {
			pError = xrtGetError();
			snprintf(sErr, iErrCap, "tls identity build failed for %s certificate: %s",
				sCertificateType,
				pError != NULL ? xrtErrorMessage(pError) : "unknown error");
		}
		xrtFree(arrViews);
	} else {
		snprintf(sErr, iErrCap, "out of memory building tls identity");
	}
	xrtFree(pKeyDer);
	return pIdentity;
}

/* 装载一个 host 的证书身份；无证书配置返回 NULL（合法） */
static xtlsidentity* XS_TlsLoadHost(XS_HostInfo* pHost, char* sErr, size_t iErrCap)
{
	str sCertPath = NULL, sKeyPath = NULL, sCaPath = NULL;
	bytes pCertText = NULL, pKeyText = NULL, pCaText = NULL;
	size_t iCertSize = 0, iKeySize = 0, iCaSize = 0;
	XS_TlsDer* arrCerts = NULL;
	XS_TlsDer* arrCa = NULL;
	uint32 iCertCount = 0;
	uint32 iCaCount = 0;
	xtlsidentity* pIdentity = NULL;
	size_t iCombinedBytes;

	if ( pHost->TlsCert == NULL || pHost->TlsKey == NULL ) {
		return NULL;
	}
	sCertPath = XS_TlsResolvePath(pHost->TlsCert);
	sKeyPath = XS_TlsResolvePath(pHost->TlsKey);
	sCaPath = XS_TlsResolvePath(pHost->TlsCA);
	if ( sCertPath == NULL || sKeyPath == NULL ||
	     (pHost->TlsCA != NULL && sCaPath == NULL) ) {
		snprintf(sErr, iErrCap, "tls cert path resolve failed");
		goto done;
	}
	pCertText = xrtFileReadAll(sCertPath, &iCertSize);
	pKeyText = xrtFileReadAll(sKeyPath, &iKeySize);
	if ( pCertText == NULL || pKeyText == NULL ) {
		snprintf(sErr, iErrCap, "tls cert file read failed: %s", pHost->TlsCert);
		goto done;
	}
	if ( !XS_TlsPemBlocks((const char*)pCertText, iCertSize, "CERTIFICATE",
		&arrCerts, &iCertCount) ) {
		snprintf(sErr, iErrCap, "tls cert parse failed: %s", pHost->TlsCert);
		goto done;
	}
	if ( sCaPath != NULL ) {
		XS_TlsDer* pCombined;

		pCaText = xrtFileReadAll(sCaPath, &iCaSize);
		if ( pCaText == NULL ) {
			snprintf(sErr, iErrCap, "tls ca file read failed: %s", pHost->TlsCA);
			goto done;
		}
		if ( !XS_TlsPemBlocks((const char*)pCaText, iCaSize, "CERTIFICATE",
			&arrCa, &iCaCount) ) {
			snprintf(sErr, iErrCap, "tls ca parse failed: %s", pHost->TlsCA);
			goto done;
		}
		if ( iCaCount > UINT32_MAX - iCertCount ) {
			snprintf(sErr, iErrCap, "tls certificate chain too long");
			goto done;
		}
		if ( !XS_TlsSizeMul((size_t)iCertCount + (size_t)iCaCount,
			sizeof(XS_TlsDer), &iCombinedBytes) ) {
			snprintf(sErr, iErrCap, "tls certificate chain too long");
			goto done;
		}
		pCombined = (XS_TlsDer*)xrtRealloc(arrCerts, iCombinedBytes);
		if ( pCombined == NULL ) {
			snprintf(sErr, iErrCap, "out of memory building tls certificate chain");
			goto done;
		}
		arrCerts = pCombined;
		memcpy(arrCerts + iCertCount, arrCa, sizeof(XS_TlsDer) * (size_t)iCaCount);
		iCertCount += iCaCount;
		xrtFree(arrCa);
		arrCa = NULL;
		iCaCount = 0;
	}
	pIdentity = XS_TlsBuildIdentity(arrCerts, iCertCount,
		(const char*)pKeyText, iKeySize, sErr, iErrCap);

done:
	XS_TlsDerFree(arrCerts, iCertCount);
	XS_TlsDerFree(arrCa, iCaCount);
	xrtFree(pCertText);
	xrtFree(pKeyText);
	xrtFree(pCaText);
	xrtFree(sCertPath);
	xrtFree(sKeyPath);
	xrtFree(sCaPath);
	return pIdentity;
}

/* ============================================================
 * 每 server 的 SNI 身份表
 * ============================================================ */

typedef struct XS_TlsEntry {
	XS_HostInfo*		pHost;
	xtlsidentity*		pIdentity;
} XS_TlsEntry;

typedef struct XS_TlsTable {
	XS_TlsEntry*		pEntries;	/* 热替换时整体换指向（原子语义见 Refresh） */
	uint32			iCount;
	xmutex*			pLock;		/* Select（worker 线程）与 Refresh（任意线程）互斥 */
} XS_TlsTable;

/* 与 HTTP/WS 共用 xrt authority 规则匹配 SNI（xbytesview 非零结尾）。 */
static bool XS_TlsNameMatch(const char* sHostList, xbytesview tName)
{
	const char* pSeg = sHostList;
	xstrview tServerName = xrtStrViewN((const char*)tName.Data, tName.Size);

	while ( pSeg != NULL && *pSeg != '\0' ) {
		const char* pEnd = strchr(pSeg, ';');
		size_t iLen = (pEnd != NULL) ? (size_t)(pEnd - pSeg) : strlen(pSeg);
		xhttpauthority tAuthority;

		while ( iLen > 0 && (*pSeg == ' ' || *pSeg == '\t') ) { pSeg++; iLen--; }
		while ( iLen > 0 && (pSeg[iLen - 1] == ' ' || pSeg[iLen - 1] == '\t') ) { iLen--; }
		if ( xrtHttpHostParse(xrtStrViewN(pSeg, iLen), &tAuthority) &&
		     (tAuthority.Flags & XHTTP_AUTHORITY_HAS_PORT) == 0 &&
		     xrtHttpHostEqual(tAuthority.Host, tServerName) ) return true;
		pSeg = (pEnd != NULL) ? pEnd + 1 : NULL;
	}
	return false;
}

static bool XS_TlsSelect(ptr pContext, const xtlsserverrequest* pRequest, xtlsserverchoice* pChoice)
{
	XS_TlsTable* pTable = (XS_TlsTable*)pContext;
	bool bFound = false;
	uint32 i;

	if ( pTable->pLock != NULL ) {
		xrtMutexLock(pTable->pLock);
	}
	if ( pRequest->ServerName.Size > 0 ) {
		for ( i = 0; i < pTable->iCount; i++ ) {
			if ( pTable->pEntries[i].pHost->Host != NULL &&
			     XS_TlsNameMatch(pTable->pEntries[i].pHost->Host, pRequest->ServerName) ) {
				pChoice->Identity = pTable->pEntries[i].pIdentity;
				bFound = true;
				break;
			}
		}
	}
	/* 无匹配：回落第一项（优先 DefaultHost，否则为首个可用 vhost 身份）。 */
	if ( !bFound && pTable->iCount > 0 ) {
		pChoice->Identity = pTable->pEntries[0].pIdentity;
		bFound = true;
	}
	if ( pTable->pLock != NULL ) {
		xrtMutexUnlock(pTable->pLock);
	}
	return bFound;
}

/* listener 跨 generation 复用时，TLS selector 也必须经稳定槽位取当前表。 */
static bool XS_TlsSlotSelect(ptr pContext, const xtlsserverrequest* pRequest, xtlsserverchoice* pChoice)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pContext;
	XS_TlsTable* pTable;
	bool bFound = false;

	if ( pSlot == NULL ) return false;
	if ( !XS_TopologyReadLock() ) return false;
	xrtMutexLock(pSlot->pLock);
	pTable = (XS_TlsTable*)pSlot->pTlsContext;
	if ( !pSlot->bClosing && pSlot->bAccepting && pTable != NULL ) {
		bFound = XS_TlsSelect(pTable, pRequest, pChoice);
		if ( bFound && pSlot->pGeneration != NULL ) {
			pChoice->Cookie = pSlot->pGeneration->iCookie;
		}
	}
	xrtMutexUnlock(pSlot->pLock);
	XS_TopologyReadUnlock();
	return bFound;
}

/* TLS listener 的 Accept 必须与 ClientHello 选择身份时看到同一 generation。 */
static bool XS_TlsAcquireConnection(
	XS_ListenerSlot* pSlot,
	xtlsstream* pStream,
	void** ppRuntime,
	XS_ServerGeneration** ppGeneration)
{
	xtlssession* pSession;
	uint64 iCookie = 0;

	if ( pStream == NULL ) {
		printf("[xs] tls accept rejected: null stream\n");
		return false;
	}
	pSession = xrtTlsStreamSession(pStream);
	if ( pSession == NULL ) {
		printf("[xs] tls accept rejected: session unavailable\n");
		return false;
	}
	if ( !xrtTlsServerCookie(pSession, &iCookie) ) {
		printf("[xs] tls accept rejected: cookie unavailable\n");
		return false;
	}
	if ( iCookie == 0 ) {
		printf("[xs] tls accept rejected: empty generation cookie\n");
		return false;
	}
	if ( !XS_ListenerSlotAcquireConnection(
		pSlot, iCookie, ppRuntime, ppGeneration) ) {
		printf("[xs] tls accept rejected: generation cookie %llu is no longer current\n",
			(unsigned long long)iCookie);
		return false;
	}
	return true;
}

/* 从启用的 DefaultHost + hosts[] 收集证书；首个可用身份是 SNI 回落。 */
static bool XS_TlsTableBuild(XS_ServerInfo* pServer, XS_TlsTable* pTable, char* sErr, size_t iErrCap)
{
	uint32 iCount = 1 + pServer->HostCount;
	uint32 i;
	size_t iEntriesBytes;

	memset(pTable, 0, sizeof(*pTable));
	if ( !XS_TlsSizeMul((size_t)iCount, sizeof(XS_TlsEntry), &iEntriesBytes) ) {
		snprintf(sErr, iErrCap, "too many tls hosts");
		return false;
	}
	pTable->pEntries = (XS_TlsEntry*)xrtCalloc(1, iEntriesBytes);
	pTable->pLock = pTable->pEntries != NULL ? xrtMutexCreate() : NULL;
	if ( pTable->pEntries == NULL || pTable->pLock == NULL ) {
		xrtFree(pTable->pEntries);
		pTable->pEntries = NULL;
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	if ( pServer->DefaultHost->Enabled &&
	     (((pServer->DefaultHost->TlsCert == NULL) != (pServer->DefaultHost->TlsKey == NULL)) ||
	      (pServer->DefaultHost->TlsCA != NULL && pServer->DefaultHost->TlsCert == NULL)) ) {
		snprintf(sErr, iErrCap, "tls host '%s' requires tls_cert/tls_key before tls_ca",
			pServer->DefaultHost->Name);
		return false;
	}
	if ( pServer->DefaultHost->Enabled && pServer->DefaultHost->TlsCert != NULL ) {
		pTable->pEntries[pTable->iCount].pHost = pServer->DefaultHost;
		pTable->pEntries[pTable->iCount].pIdentity = XS_TlsLoadHost(pServer->DefaultHost, sErr, iErrCap);
		if ( pTable->pEntries[pTable->iCount].pIdentity == NULL && sErr[0] != '\0' ) {
			return false;
		}
		if ( pTable->pEntries[pTable->iCount].pIdentity != NULL ) {
			pTable->iCount++;
		}
	}
	for ( i = 0; i < pServer->HostCount; i++ ) {
		if ( !pServer->Hosts[i]->Enabled ) continue;
		if ( ((pServer->Hosts[i]->TlsCert == NULL) != (pServer->Hosts[i]->TlsKey == NULL)) ||
		     (pServer->Hosts[i]->TlsCA != NULL && pServer->Hosts[i]->TlsCert == NULL) ) {
			snprintf(sErr, iErrCap, "tls host '%s' requires tls_cert/tls_key before tls_ca",
				pServer->Hosts[i]->Name);
			return false;
		}
		if ( pServer->Hosts[i]->TlsCert == NULL ) {
			continue;
		}
		pTable->pEntries[pTable->iCount].pHost = pServer->Hosts[i];
		pTable->pEntries[pTable->iCount].pIdentity = XS_TlsLoadHost(pServer->Hosts[i], sErr, iErrCap);
		if ( pTable->pEntries[pTable->iCount].pIdentity == NULL && sErr[0] != '\0' ) {
			return false;
		}
		if ( pTable->pEntries[pTable->iCount].pIdentity != NULL ) {
			pTable->iCount++;
		}
	}
	return true;
}

static void XS_TlsTableUnit(XS_TlsTable* pTable)
{
	uint32 i;

	for ( i = 0; i < pTable->iCount; i++ ) {
		if ( pTable->pEntries[i].pIdentity != NULL ) {
			xrtTlsIdentityRelease(pTable->pEntries[i].pIdentity);
		}
	}
	if ( pTable->pLock != NULL ) {
		xrtMutexDestroy(pTable->pLock);
		pTable->pLock = NULL;
	}
	xrtFree(pTable->pEntries);
	memset(pTable, 0, sizeof(*pTable));
}

#endif
