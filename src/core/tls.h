#ifndef XS_CORE_TLS_H
#define XS_CORE_TLS_H

/*
 * xs3 TLS 身份装载与 SNI 选择器（设计 §4.1 host 级证书 / §10.3 证书热替换基础）
 * - PEM 证书/私钥经 xrt fs 读取（UTF-8 路径），xrtPemDecode 转 DER 后构造 identity
 * - 证书链：同文件内多个 CERTIFICATE 块按序作为链
 * - 密钥算法自动识别：RSA → P-256 → P-384 依次尝试
 * - SNI：xtlsserverselectproc 按 ClientHello ServerName 匹配 host->Host（分号分隔、忽略大小写）
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"
#include "../runtime/listener_slot.h"
#include "engine.h"

/* 进程级共享 TLS 上下文（惰性创建） */
static xtlscontext* XS_TlsSharedContext(void)
{
	static xtlscontext* pContext = NULL;

	if ( pContext == NULL ) {
		xtlscontextconfig tCfg;

		xrtTlsContextConfigInit(&tCfg);
		pContext = xrtTlsContextCreate(&tCfg);
	}
	return pContext;
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

/* 从 PEM 文本提取指定标签的全部 DER 块；返回块数（0 失败），调用方逐块 xrtFree */
typedef struct XS_TlsDer {
	bytes			pData;
	size_t			iSize;
} XS_TlsDer;

static uint32 XS_TlsPemBlocks(const char* sText, size_t iSize, const char* sLabel, XS_TlsDer** parrOut)
{
	XS_TlsDer* arrBlocks = NULL;
	uint32 iCount = 0;
	size_t iOffset = 0;

	for ( ; ; ) {
		xpemblock tBlock;
		bytes pDer;
		size_t iDer = 0;

		if ( !xrtPemFind(sText + iOffset, iSize - iOffset, sLabel, &tBlock) ) {
			break;
		}
		pDer = xrtPemDecodeNew(&tBlock, &iDer);
		if ( pDer == NULL ) {
			break;
		}
		{
			XS_TlsDer* pNew = (XS_TlsDer*)xrtRealloc(arrBlocks, sizeof(XS_TlsDer) * (size_t)(iCount + 1));
			if ( pNew == NULL ) {
				xrtFree(pDer);
				break;
			}
			arrBlocks = pNew;
		}
		arrBlocks[iCount].pData = pDer;
		arrBlocks[iCount].iSize = iDer;
		iCount++;
		/* 越过本块继续找（Raw 视图相对 sText+iOffset） */
		iOffset += (size_t)(tBlock.Raw.Data - (sText + iOffset)) + tBlock.Raw.Size;
	}
	if ( iCount == 0 ) {
		xrtFree(arrBlocks);
		arrBlocks = NULL;
	}
	*parrOut = arrBlocks;
	return iCount;
}

/* 依次尝试 RSA / P-256 / P-384 构造身份 */
static xtlsidentity* XS_TlsBuildIdentity(XS_TlsDer* arrCerts, uint32 iCertCount, const char* sKeyText, size_t iKeySize)
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

	for ( j = 0; j < 3 && pKeyDer == NULL; j++ ) {
		if ( xrtPemFind(sKeyText, iKeySize, arrKeyLabels[j], &tKeyBlock) ) {
			pKeyDer = xrtPemDecodeNew(&tKeyBlock, &iKeyDer);
		}
	}
	if ( pKeyDer == NULL ) {
		return NULL;
	}
	tKey.Data = (const unsigned char*)pKeyDer;
	tKey.Size = iKeyDer;

	arrViews = (xbytesview*)xrtCalloc(iCertCount, sizeof(xbytesview));
	if ( arrViews != NULL ) {
		for ( i = 0; i < iCertCount; i++ ) {
			arrViews[i].Data = (const unsigned char*)arrCerts[i].pData;
			arrViews[i].Size = arrCerts[i].iSize;
		}
		pIdentity = xrtTlsIdentityRsa(arrViews, iCertCount, tKey);
		if ( pIdentity == NULL ) {
			pIdentity = xrtTlsIdentityP256(arrViews, iCertCount, tKey);
		}
		if ( pIdentity == NULL ) {
			pIdentity = xrtTlsIdentityP384(arrViews, iCertCount, tKey);
		}
		xrtFree(arrViews);
	}
	xrtFree(pKeyDer);
	return pIdentity;
}

/* 装载一个 host 的证书身份；无证书配置返回 NULL（合法） */
static xtlsidentity* XS_TlsLoadHost(XS_HostInfo* pHost, char* sErr, size_t iErrCap)
{
	str sCertPath, sKeyPath;
	bytes pCertText = NULL, pKeyText = NULL;
	size_t iCertSize = 0, iKeySize = 0;
	XS_TlsDer* arrCerts = NULL;
	uint32 iCertCount = 0;
	xtlsidentity* pIdentity = NULL;
	uint32 i;

	if ( pHost->TlsCert == NULL || pHost->TlsKey == NULL ) {
		return NULL;
	}
	sCertPath = XS_TlsResolvePath(pHost->TlsCert);
	sKeyPath = XS_TlsResolvePath(pHost->TlsKey);
	if ( sCertPath == NULL || sKeyPath == NULL ) {
		snprintf(sErr, iErrCap, "tls cert path resolve failed");
		goto done;
	}
	pCertText = xrtFileReadAll(sCertPath, &iCertSize);
	pKeyText = xrtFileReadAll(sKeyPath, &iKeySize);
	if ( pCertText == NULL || pKeyText == NULL ) {
		snprintf(sErr, iErrCap, "tls cert file read failed: %s", pHost->TlsCert);
		goto done;
	}
	iCertCount = XS_TlsPemBlocks((const char*)pCertText, iCertSize, "CERTIFICATE", &arrCerts);
	if ( iCertCount == 0 ) {
		snprintf(sErr, iErrCap, "tls cert parse failed: %s", pHost->TlsCert);
		goto done;
	}
	pIdentity = XS_TlsBuildIdentity(arrCerts, iCertCount, (const char*)pKeyText, iKeySize);
	if ( pIdentity == NULL ) {
		snprintf(sErr, iErrCap, "tls identity build failed: %s", pHost->TlsCert);
	}

done:
	for ( i = 0; i < iCertCount; i++ ) {
		xrtFree(arrCerts[i].pData);
	}
	xrtFree(arrCerts);
	xrtFree(pCertText);
	xrtFree(pKeyText);
	xrtFree(sCertPath);
	xrtFree(sKeyPath);
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

/* 按 host 域名列表忽略大小写匹配 SNI 名（xbytesview 非零结尾） */
static bool XS_TlsNameMatch(const char* sHostList, xbytesview tName)
{
	const char* pSeg = sHostList;

	while ( pSeg != NULL && *pSeg != '\0' ) {
		const char* pEnd = strchr(pSeg, ';');
		size_t iLen = (pEnd != NULL) ? (size_t)(pEnd - pSeg) : strlen(pSeg);

		while ( iLen > 0 && *pSeg == ' ' ) { pSeg++; iLen--; }
		while ( iLen > 0 && pSeg[iLen - 1] == ' ' ) { iLen--; }
		if ( iLen == tName.Size ) {
			size_t i;

			for ( i = 0; i < iLen; i++ ) {
				char chA = pSeg[i];
				char chB = (char)tName.Data[i];

				if ( chA >= 'A' && chA <= 'Z' ) chA = (char)(chA - 'A' + 'a');
				if ( chB >= 'A' && chB <= 'Z' ) chB = (char)(chB - 'A' + 'a');
				if ( chA != chB ) {
					break;
				}
			}
			if ( i == iLen ) {
				return true;
			}
		}
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
	/* 无匹配：回落第一项（DefaultHost 身份在构造方保证为 [0]） */
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
	}
	xrtMutexUnlock(pSlot->pLock);
	XS_TopologyReadUnlock();
	return bFound;
}

/* 从 DefaultHost + hosts[] 收集证书；DefaultHost 证书（若存在）恒为 entries[0] */
static bool XS_TlsTableBuild(XS_ServerInfo* pServer, XS_TlsTable* pTable, char* sErr, size_t iErrCap)
{
	uint32 iCount = 1 + pServer->HostCount;
	uint32 i;

	memset(pTable, 0, sizeof(*pTable));
	pTable->pEntries = (XS_TlsEntry*)xrtCalloc(iCount, sizeof(XS_TlsEntry));
	pTable->pLock = pTable->pEntries != NULL ? xrtMutexCreate() : NULL;
	if ( pTable->pEntries == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	if ( pServer->DefaultHost->TlsCert != NULL ) {
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
