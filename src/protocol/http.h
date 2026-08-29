#ifndef XS_PROTOCOL_HTTP_H
#define XS_PROTOCOL_HTTP_H

/*
 * xs3 HTTP/1.x 驱动（设计 §6.1）
 * - 连接驱动：accept → Read → xrtHttp1RequestParse(Buffer|Tls) → Host 路由
 *   → RequestProc（三态：XS_OK / XS_FALLBACK / XS_TAKEOVER）→ body 余量排空
 *   → keep-alive 循环
 * - 静态层（XS_FALLBACK，GET/HEAD）：路径过滤（解码/穿越/点文件/反斜杠）→
 *   MIME → 自定义头（static.headers）→ 错误页（static.error_pages / 内置）→ SendFile
 * - Custom 旋钮：body_limit / header_limit / path_limit / idle_timeout
 * - 连接生命周期与 idle 保护复用注册表（与 tcp 驱动同构）
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"
#include "../core/config.h"
#include "../core/engine.h"
#include "../core/tls.h"
#include "../runtime/registry.h"
#include "../script/script.h"
#include "stream.h"

#define XS_HTTP_MAX_FIELDS	48
#define XS_HTTP_MAX_TRAILERS	8
#define XS_HTTP_MAX_EXTRA_FIELDS	8
#define XS_HTTP_SENDFILE_CHUNK	(512 * 1024)

typedef struct XS_HttpRuntime {
	XS_ServerInfo*		pServer;
	XS_HostInfo*		pDefaultHost;
	bool			bTls;
	xnetlistener*		pListener;
	xtlslistener*		pTlsListener;
	XS_TlsTable		tTls;
	XS_ConnRegistry		tRegistry;
	xhttp1limits		tLimits;
	uint64			iBodyLimit;	/* 0 = 内核默认 */
	uint64			iPathLimit;	/* 0 = 默认 2048 */
	uint64			iIdleMs;
	uint64			iSweepTimer;
	volatile bool		bStopping;
	/* static.headers 装配期预渲染缓存（每 host 一条；请求路径零字符串处理） */
	struct XS_HttpHostHdrs**	arrHdrCache;	/* 指针数组（每 host 一条） */
	uint32			iHdrCacheCount;
} XS_HttpRuntime;

/* host 的自定义响应头缓存：blob 内 "name\0value\0" 依次排布，字段为借用视图 */
typedef struct XS_HttpHostHdrs {
	XS_HostInfo*		pHost;
	char*			sBlob;		/* 持久分配，随 runtime 释放 */
	size_t			iBlobSize;
	xhttpfield		arrFields[XS_HTTP_MAX_EXTRA_FIELDS];
	uint32			iCount;
} XS_HttpHostHdrs;

typedef struct XS_HttpRecord {
	XS_ConnRecord		tReg;		/* 注册表链（首成员） */
	XS_HttpRuntime*		pRuntime;
	XS_HostInfo*		pHost;		/* 当前请求归属 */
	xhttp1head		tHead;
	xhttpfield		arrFields[XS_HTTP_MAX_FIELDS];
	xhttp1body		tBody;
	xhttp1bodyplan		tPlan;
	xhttpfield		arrTrailers[XS_HTTP_MAX_TRAILERS];
	xhttp1bodylimits	tBodyLimits;
	int			iPhase;		/* 0=HEAD 1=BODY_DRAIN */
	bool			bTakenOver;
} XS_HttpRecord;

/* ============================================================
 * 传输抽象（tcp / tls 双形态）
 * ============================================================ */

static xnetresult XS_HttpSend(XS_ConnRecord* pReg, const void* pData, size_t iSize)
{
	size_t iWritten = 0;

	if ( pReg->pTls != NULL ) {
		return xrtTlsStreamSend(pReg->pTls, pData, iSize, &iWritten) == XTLS_OK &&
		       iWritten == iSize ? XNET_RESULT_OK : XNET_RESULT_ERROR;
	}
	return xrtNetStreamSend(pReg->pTcp, pData, iSize);
}

static void XS_HttpConsume(XS_HttpRecord* pRec, size_t iSize)
{
	if ( pRec->tReg.pTls != NULL ) {
		(void)xrtTlsStreamConsume(pRec->tReg.pTls, iSize);
	} else {
		(void)xrtNetBufConsume((xnetbuf*)xrtNetStreamBuffer(pRec->tReg.pTcp), iSize);
	}
}

/* 取前 iSize 字节到栈缓冲（body/head 视图用） */
static size_t XS_HttpPeek(XS_HttpRecord* pRec, void* pOut, size_t iSize)
{
	if ( pRec->tReg.pTls != NULL ) {
		const xnetbuf* pBuf = xrtTlsStreamBuffer(pRec->tReg.pTls);
		return pBuf != NULL ? xrtNetBufPeek(pBuf, 0, pOut, iSize) : 0;
	}
	return xrtNetBufPeek(xrtNetStreamBuffer(pRec->tReg.pTcp), 0, pOut, iSize);
}

static size_t XS_HttpAvail(XS_HttpRecord* pRec)
{
	const xnetbuf* pBuf;

	if ( pRec->tReg.pTls != NULL ) {
		pBuf = xrtTlsStreamBuffer(pRec->tReg.pTls);
	} else {
		pBuf = xrtNetStreamBuffer(pRec->tReg.pTcp);
	}
	return pBuf != NULL ? xrtNetBufSize(pBuf) : 0;
}

static xhttp1status XS_HttpParseHead(XS_HttpRecord* pRec, xhttp1errorinfo* pErr)
{
	if ( pRec->tReg.pTls != NULL ) {
		return xrtHttp1RequestParseTls(pRec->tReg.pTls, &pRec->tHead, &pRec->pRuntime->tLimits, pErr);
	}
	return xrtHttp1RequestParseBuffer((xnetbuf*)xrtNetStreamBuffer(pRec->tReg.pTcp),
		&pRec->tHead, &pRec->pRuntime->tLimits, pErr);
}

/* ============================================================
 * 响应写出（静态层与错误页专用；业务响应由脚本自建）
 * ============================================================ */

typedef struct XS_HttpResp {
	uint16		iStatus;
	const char*	sContentType;
	uint64		iContentLength;
	xhttpfield	arrExtra[8];	/* 追加响应头（借用视图，随本次调用存活） */
	size_t		iExtraCount;
	bool		bHeadOnly;
} XS_HttpResp;

static bool XS_HttpRespond(XS_HttpRecord* pRec, XS_HttpResp* pResp)
{
	unsigned char arrHead[2048];
	xhttpfield arrFields[16];
	size_t iFieldCount = 0;
	size_t i;
	xstrview tReason;
	size_t iSize = 0;
	char arrLen[32];

	snprintf(arrLen, sizeof(arrLen), "%llu", (unsigned long long)pResp->iContentLength);
	arrFields[iFieldCount].Name = XRT_STR_LITERAL("Content-Length");
	arrFields[iFieldCount].Value = xrtStrViewN(arrLen, strlen(arrLen));
	iFieldCount++;
	if ( pResp->sContentType != NULL ) {
		arrFields[iFieldCount].Name = XRT_STR_LITERAL("Content-Type");
		arrFields[iFieldCount].Value = xrtStrView(pResp->sContentType);
		iFieldCount++;
	}
	for ( i = 0; i < pResp->iExtraCount && iFieldCount < 16; i++ ) {
		arrFields[iFieldCount++] = pResp->arrExtra[i];
	}
	tReason = xrtHttpStatusText(pResp->iStatus);
	if ( !xrtHttp1ResponseWrite(XHTTP_VERSION_1_1, pResp->iStatus, tReason,
		arrFields, iFieldCount, arrHead, sizeof(arrHead), &iSize) ) {
		return false;
	}
	return XS_HttpSend(&pRec->tReg, arrHead, iSize) == XNET_RESULT_OK;
}

static const char* XS_HttpMime(const char* sName);
static const XS_HttpHostHdrs* XS_HttpHdrCacheFind(XS_HttpRuntime* pRuntime, XS_HostInfo* pHost);
static str XS_HttpHostRoot(XS_HttpRuntime* pRuntime, XS_HostInfo* pHost);

/* host Custom 的 static 对象（无则 NULL） */
static xvalue* XS_HttpStaticCfg(XS_HostInfo* pHost)
{
	if ( pHost == NULL || pHost->Custom == NULL ) {
		return NULL;
	}
	return xrtValueObjectGet(pHost->Custom, XRT_STR_LITERAL("static"));
}

/* 错误页：优先 static.error_pages.<code>（host 根相对文件），缺省内置极简页 */
static void XS_HttpErrorPage(XS_HttpRecord* pRec, uint16 iStatus)
{
	char arrBody[256];
	XS_HttpResp tResp;
	xvalue* pStatic = XS_HttpStaticCfg(pRec->pHost);
	xvalue* pPages = NULL;
	int iLen;

	if ( pStatic != NULL ) {
		char arrKey[8];
		xvalue* pPath;

		snprintf(arrKey, sizeof(arrKey), "%u", (unsigned)iStatus);
		pPages = xrtValueObjectGet(pStatic, XRT_STR_LITERAL("error_pages"));
		if ( pPages != NULL &&
		     (pPath = xrtValueObjectGet(pPages, xrtStrViewN(arrKey, strlen(arrKey)))) != NULL ) {
			xstrview tRel;
			if ( xrtValueGetString(pPath, &tRel) && tRel.Size > 0 && tRel.Size < 512 ) {
				char arrRel[512];
				str sRoot, sFull;
				bytes pData;
				size_t iSize = 0;

				memcpy(arrRel, tRel.Data, tRel.Size);
				arrRel[tRel.Size] = '\0';
				sRoot = XS_HttpHostRoot(pRec->pRuntime, pRec->pHost);
				sFull = xrtPathIsAbs(arrRel) ? xrtStrDup(arrRel) : xrtPathJoin(sRoot, arrRel);
				xrtFree(sRoot);
				pData = sFull != NULL ? xrtFileReadAll(sFull, &iSize) : NULL;
				xrtFree(sFull);
				if ( pData != NULL && iSize > 0 ) {
					memset(&tResp, 0, sizeof(tResp));
					tResp.iStatus = iStatus;
					tResp.sContentType = strchr(arrRel, '.') != NULL
						? XS_HttpMime(arrRel) : "text/html; charset=utf-8";
					tResp.iContentLength = (uint64)iSize;
					if ( XS_HttpRespond(pRec, &tResp) ) {
						(void)XS_HttpSend(&pRec->tReg, pData, iSize);
					}
					xrtFree(pData);
					return;
				}
			}
		}
	}
	iLen = snprintf(arrBody, sizeof(arrBody),
		"<!DOCTYPE html><html><head><title>%u</title></head><body><h1>%u</h1><hr>xs</body></html>",
		(unsigned)iStatus, (unsigned)iStatus);
	memset(&tResp, 0, sizeof(tResp));
	tResp.iStatus = iStatus;
	tResp.sContentType = "text/html; charset=utf-8";
	tResp.iContentLength = (uint64)iLen;
	if ( XS_HttpRespond(pRec, &tResp) ) {
		(void)XS_HttpSend(&pRec->tReg, arrBody, (size_t)iLen);
	}
}

/* ============================================================
 * 静态层（XS_FALLBACK，仅 GET/HEAD）
 * ============================================================ */

static bool XS_HttpViewEq(xstrview tView, const char* sLiteral);

static const char* XS_HttpMime(const char* sName)
{
	static const struct { const char* sExt; const char* sMime; } arrMime[] = {
		{ ".html", "text/html; charset=utf-8" }, { ".htm", "text/html; charset=utf-8" },
		{ ".css", "text/css; charset=utf-8" }, { ".js", "text/javascript; charset=utf-8" },
		{ ".json", "application/json; charset=utf-8" }, { ".txt", "text/plain; charset=utf-8" },
		{ ".xml", "application/xml; charset=utf-8" }, { ".svg", "image/svg+xml" },
		{ ".png", "image/png" }, { ".jpg", "image/jpeg" }, { ".jpeg", "image/jpeg" },
		{ ".gif", "image/gif" }, { ".ico", "image/x-icon" }, { ".pdf", "application/pdf" },
		{ ".woff", "font/woff" }, { ".woff2", "font/woff2" }, { ".mp4", "video/mp4" },
		{ ".zip", "application/zip" },
	};
	size_t i;
	size_t j;

	for ( j = 0; sName[j] != '\0'; j++ ) {}
	for ( ; j > 0 && sName[j] != '.'; j-- ) {}
	if ( j == 0 ) {
		return "application/octet-stream";
	}
	for ( i = 0; i < sizeof(arrMime) / sizeof(arrMime[0]); i++ ) {
		if ( strcmp(sName + j, arrMime[i].sExt) == 0 ) {
			return arrMime[i].sMime;
		}
	}
	return "application/octet-stream";
}

/* host 根路径解析（相对 appPath；结果缓存于运行时线性表） */
static str XS_HttpHostRoot(XS_HttpRuntime* pRuntime, XS_HostInfo* pHost)
{
	str sPath;

	if ( pHost->Path == NULL ) {
		return xrtStrDup(XS_AppPath());
	}
	if ( xrtPathIsAbs(pHost->Path) ) {
		return xrtStrDup(pHost->Path);
	}
	sPath = xrtPathJoin(XS_AppPath(), pHost->Path);
	return sPath != NULL ? sPath : xrtStrDup(pHost->Path);
}

static void XS_HttpStatic(XS_HttpRecord* pRec)
{
	XS_HttpRuntime* pRuntime = pRec->pRuntime;
	char arrPath[1024];
	char arrDecoded[900];
	size_t iDecoded = 0;
	size_t iStart;
	size_t i;
	str sRoot;
	str sFull;
	xfileinfo tInfo;
	xhttpfield* pField;
	xstrview tTarget = pRec->tHead.Target;

	/* 方法限制 */
	if ( !(XS_HttpViewEq(pRec->tHead.Method, "GET")) &&
	     !(XS_HttpViewEq(pRec->tHead.Method, "HEAD")) ) {
		XS_HttpErrorPage(pRec, 405);
		return;
	}
	/* 取 path（去掉 query） */
	for ( i = 0; i < tTarget.Size && tTarget.Data[i] != '?'; i++ ) {}
	if ( i == 0 || i >= sizeof(arrPath) ) {
		XS_HttpErrorPage(pRec, 400);
		return;
	}
	memcpy(arrPath, tTarget.Data, i);
	arrPath[i] = '\0';
	/* 解码 + 安全过滤（% 解码后统一检查） */
	if ( !xrtPercentDecode(xrtStrViewN(arrPath, i), arrDecoded, sizeof(arrDecoded) - 1, &iDecoded) ) {
		XS_HttpErrorPage(pRec, 400);
		return;
	}
	arrDecoded[iDecoded] = '\0';
	if ( arrDecoded[0] != '/' ) {
		XS_HttpErrorPage(pRec, 400);
		return;
	}
	if ( pRuntime->iPathLimit > 0 && iDecoded > pRuntime->iPathLimit ) {
		XS_HttpErrorPage(pRec, 414);
		return;
	}
	for ( i = 0; i < iDecoded; i++ ) {
		if ( arrDecoded[i] == '\\' ) {
			XS_HttpErrorPage(pRec, 403);
			return;
		}
	}
	/* 段级检查：.. 穿越恒拒；点文件受 static.deny_dotfiles 门控（默认拒） */
	{
		bool bDenyDot = true;
		xvalue* pStatic = XS_HttpStaticCfg(pRec->pHost);
		xvalue* pDeny;

		if ( pStatic != NULL &&
		     (pDeny = xrtValueObjectGet(pStatic, XRT_STR_LITERAL("deny_dotfiles"))) != NULL ) {
			(void)xrtValueGetBool(pDeny, &bDenyDot);
		}
		iStart = 1;
		for ( i = 1; i <= iDecoded; i++ ) {
			if ( i == iDecoded || arrDecoded[i] == '/' ) {
				size_t iLen = i - iStart;

				if ( iLen == 2 && arrDecoded[iStart] == '.' && arrDecoded[iStart + 1] == '.' ) {
					XS_HttpErrorPage(pRec, 403);
					return;
				}
				if ( bDenyDot && iLen > 0 && arrDecoded[iStart] == '.' ) {
					XS_HttpErrorPage(pRec, 403);
					return;
				}
				iStart = i + 1;
			}
		}
	}

	sRoot = XS_HttpHostRoot(pRuntime, pRec->pHost);
	sFull = xrtPathJoin(sRoot, arrDecoded + 1);
	xrtFree(sRoot);
	if ( sFull == NULL ) {
		XS_HttpErrorPage(pRec, 500);
		return;
	}
	/* 目录 → static.index 逐个回落（缺省 index.html） */
	if ( xrtPathStat(sFull, true, &tInfo) && tInfo.Type == XFILE_TYPE_DIRECTORY ) {
		str sFound = NULL;
		xvalue* pStatic = XS_HttpStaticCfg(pRec->pHost);
		xvalue* pIndex = pStatic != NULL
			? xrtValueObjectGet(pStatic, XRT_STR_LITERAL("index")) : NULL;
		size_t iIdx;
		size_t iCount = 1;

		if ( pIndex != NULL && xrtValueType(pIndex) == XVALUE_ARRAY ) {
			iCount = xrtValueCount(pIndex);
		}
		for ( iIdx = 0; iIdx < iCount && sFound == NULL; iIdx++ ) {
			xstrview tName;
			bool bHas = false;
			char arrIdx[256];

			if ( pIndex != NULL && xrtValueType(pIndex) == XVALUE_ARRAY ) {
				xvalue* pItem = xrtValueArrayGet(pIndex, iIdx);
				bHas = pItem != NULL && xrtValueGetString(pItem, &tName);
			} else {
				tName = XRT_STR_LITERAL("index.html");
				bHas = true;
			}
			if ( bHas && tName.Size > 0 && tName.Size < sizeof(arrIdx) - 1 ) {
				xfileinfo tIdxInfo;

				memcpy(arrIdx, tName.Data, tName.Size);
				arrIdx[tName.Size] = '\0';
				if ( arrIdx[0] != '/' && arrIdx[0] != '.' && !strchr(arrIdx, '\\') ) {
					str sTry = xrtPathJoin(sFull, arrIdx);

					if ( sTry != NULL && xrtPathStat(sTry, true, &tIdxInfo) &&
					     tIdxInfo.Type == XFILE_TYPE_FILE ) {
						sFound = sTry;
					} else {
						xrtFree(sTry);
					}
				}
			}
		}
		xrtFree(sFull);
		sFull = sFound;
	}
	if ( sFull == NULL || !xrtPathStat(sFull, true, &tInfo) || tInfo.Type != XFILE_TYPE_FILE ) {
		xrtFree(sFull);
		XS_HttpErrorPage(pRec, 404);
		return;
	}
	{
		XS_HttpResp tResp;
		const char* sName = sFull;
		bool bOk;
		(void)pField;
		for ( ; *sName != '\0'; sName++ ) {}
		for ( ; sName > sFull && sName[-1] != '/' && sName[-1] != '\\'; sName-- ) {}

		memset(&tResp, 0, sizeof(tResp));
		tResp.iStatus = 200;
		tResp.sContentType = XS_HttpMime(sName);
		tResp.iContentLength = tInfo.Size;
		tResp.bHeadOnly = XS_HttpViewEq(pRec->tHead.Method, "HEAD");
		{
			/* static.headers：装配期预渲染缓存（见 XS_HttpHdrCacheFind），请求路径仅指针拷贝 */
			{
				const XS_HttpHostHdrs* pHdrs = XS_HttpHdrCacheFind(pRuntime, pRec->pHost);

				if ( pHdrs != NULL && pHdrs->iCount > 0 ) {
					memcpy(tResp.arrExtra, pHdrs->arrFields, sizeof(xhttpfield) * pHdrs->iCount);
					tResp.iExtraCount = pHdrs->iCount;
				}
				bOk = XS_HttpRespond(pRec, &tResp);
			}
		}
		if ( bOk && !tResp.bHeadOnly && tInfo.Size > 0 ) {
			if ( pRec->tReg.pTcp != NULL ) {
				/* 明文：SendFile（句柄复制，可立即关闭）；按 WriteLimit 分段 */
				xfileoptions tOpt;
				xfile hFile;
				uint64 iOffset = 0;

				xrtFileOptionsInit(&tOpt);
				tOpt.Flags = XFILE_READ;
				tOpt.Share = XFILE_SHARE_READ;
				hFile = xrtFileOpen(sFull, &tOpt);
				if ( hFile != NULL ) {
					while ( iOffset < tInfo.Size ) {
						uint64 iChunk = tInfo.Size - iOffset;
						if ( iChunk > XS_HTTP_SENDFILE_CHUNK ) {
							iChunk = XS_HTTP_SENDFILE_CHUNK;
						}
						if ( xrtNetStreamSendFile(pRec->tReg.pTcp, hFile, iOffset, (size_t)iChunk) != XNET_RESULT_OK ) {
							break;
						}
						iOffset += iChunk;
					}
					xrtClose(hFile);
				}
			} else {
				/* tcps：读发循环 */
				xfileoptions tOpt;
				xfile hFile;
				unsigned char* pBuf = (unsigned char*)xrtMalloc(XS_HTTP_SENDFILE_CHUNK);

				xrtFileOptionsInit(&tOpt);
				tOpt.Flags = XFILE_READ;
				tOpt.Share = XFILE_SHARE_READ;
				hFile = xrtFileOpen(sFull, &tOpt);
				if ( hFile != NULL && pBuf != NULL ) {
					uint64 iLeft = tInfo.Size;
					while ( iLeft > 0 ) {
						size_t iChunk = iLeft > XS_HTTP_SENDFILE_CHUNK ? XS_HTTP_SENDFILE_CHUNK : (size_t)iLeft;
						size_t iRead = 0;
						if ( !xrtRead(hFile, pBuf, iChunk, &iRead) || iRead == 0 ) {
							break;
						}
						if ( XS_HttpSend(&pRec->tReg, pBuf, iRead) != XNET_RESULT_OK ) {
							break;
						}
						iLeft -= iRead;
					}
				}
				xrtFree(pBuf);
				if ( hFile != NULL ) {
					xrtClose(hFile);
				}
			}
		}
	}
	xrtFree(sFull);
}

/* ============================================================
 * 视图比较
 * ============================================================ */

static bool XS_HttpViewEq(xstrview tView, const char* sLiteral)
{
	size_t i;

	for ( i = 0; sLiteral[i] != '\0'; i++ ) {}
	if ( tView.Size != i ) {
		return false;
	}
	for ( i = 0; i < tView.Size; i++ ) {
		if ( tView.Data[i] != sLiteral[i] ) {
			return false;
		}
	}
	return true;
}

static bool XS_HttpNameMatch(const char* sHostList, const char* sName, size_t iNameLen)
{
	const char* pSeg = sHostList;

	while ( pSeg != NULL && *pSeg != '\0' ) {
		const char* pEnd = strchr(pSeg, ';');
		size_t iLen = (pEnd != NULL) ? (size_t)(pEnd - pSeg) : strlen(pSeg);

		while ( iLen > 0 && *pSeg == ' ' ) { pSeg++; iLen--; }
		while ( iLen > 0 && pSeg[iLen - 1] == ' ' ) { iLen--; }
		if ( iLen == iNameLen ) {
			size_t i;

			for ( i = 0; i < iLen; i++ ) {
				char chA = pSeg[i];
				char chB = sName[i];

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

/* Host 头（头名与值均忽略大小写）→ host；无匹配回落 DefaultHost */
static XS_HostInfo* XS_HttpRoute(XS_HttpRecord* pRec)
{
	XS_ServerInfo* pServer = pRec->pRuntime->pServer;
	size_t iField;
	size_t k;

	for ( iField = 0; iField < pRec->tHead.FieldCount; iField++ ) {
		xstrview tName = pRec->tHead.Fields[iField].Name;
		xstrview tVal;
		size_t iHostLen;
		size_t iServer;

		for ( k = 0; k < tName.Size; k++ ) {
			char chA = tName.Data[k];
			char chB = "host"[k < 4 ? k : 3];
			if ( chA >= 'A' && chA <= 'Z' ) chA = (char)(chA - 'A' + 'a');
			if ( chB >= 'A' && chB <= 'Z' ) chB = (char)(chB - 'A' + 'a');
			if ( chA != chB ) {
				break;
			}
		}
		if ( k != 4 || tName.Size != 4 ) {
			continue;
		}
		tVal = pRec->tHead.Fields[iField].Value;
		for ( iHostLen = 0; iHostLen < tVal.Size && tVal.Data[iHostLen] != ':'; iHostLen++ ) {}
		for ( iServer = 0; iServer < pServer->HostCount; iServer++ ) {
			if ( pServer->Hosts[iServer]->Host != NULL &&
			     XS_HttpNameMatch(pServer->Hosts[iServer]->Host, tVal.Data, iHostLen) ) {
				return pServer->Hosts[iServer];
			}
		}
		break;
	}
	return pServer->DefaultHost;
}

/* ============================================================
 * body 排空与 keep-alive 循环
 * ============================================================ */

/* body 就绪判断（不消费真实 body 状态；CHUNKED 用抛弃式探针） */
static bool XS_HttpBodyReady(XS_HttpRecord* pRec)
{
	size_t iAvail = XS_HttpAvail(pRec);
	unsigned char* pCopy;
	xhttp1body tProbe;
	xhttp1errorinfo tErr;
	size_t iConsumed = 0;
	xbytesview tData;
	bool bDone;

	if ( pRec->tPlan.Mode == XHTTP1_BODY_NONE ) {
		return true;
	}
	if ( pRec->tPlan.Mode == XHTTP1_BODY_FIXED ) {
		return iAvail >= (size_t)pRec->tPlan.Length;
	}
	/* CHUNKED / CLOSE：探针体全量判定 */
	pCopy = (unsigned char*)xrtMalloc(iAvail > 0 ? iAvail : 1);
	if ( pCopy == NULL ) {
		return true;		/* 判定失败按就绪处理，回调内自行兜底 */
	}
	iAvail = XS_HttpPeek(pRec, pCopy, iAvail);
	if ( !xrtHttp1BodyInit(&tProbe, &pRec->tPlan, NULL, 0, &pRec->tBodyLimits) ) {
		xrtFree(pCopy);
		return true;
	}
	tData.Data = pCopy;
	tData.Size = iAvail;
	bDone = false;
	while ( xrtHttp1BodyRead(&tProbe, tData, false, &iConsumed, &tData, &tErr) == XHTTP1_BODY_DATA ) {
		if ( iConsumed >= iAvail ) {
			break;
		}
		tData.Data = pCopy + iConsumed;
		tData.Size = iAvail - iConsumed;
	}
	bDone = (tProbe.Mode == XHTTP1_BODY_NONE || tProbe.Remaining == 0) ||
		xrtHttp1BodyRead(&tProbe, tData, true, &iConsumed, &tData, &tErr) == XHTTP1_BODY_DONE;
	xrtFree(pCopy);
	return bDone;
}

static void XS_HttpFinishRequest(XS_HttpRecord* pRec, bool bClose)
{
	xrtHttp1HeadInit(&pRec->tHead, pRec->arrFields, XS_HTTP_MAX_FIELDS);
	pRec->iPhase = 0;
	if ( bClose ) {
		if ( pRec->tReg.pTls != NULL ) {
			(void)xrtTlsStreamClose(pRec->tReg.pTls);
		} else {
			(void)xrtNetStreamClose(pRec->tReg.pTcp);
		}
	}
}

/* 用当前缓冲推进 body 解码；返回 1=完成 0=需要更多 -1=错误 */
static int XS_HttpDrainBody(XS_HttpRecord* pRec)
{
	unsigned char arrChunk[16384];
	xhttp1errorinfo tErr;

	for ( ; ; ) {
		size_t iAvail = XS_HttpAvail(pRec);
		size_t iGot;
		size_t iConsumed = 0;
		xbytesview tData;
		xhttp1bodystatus eBody;

		if ( iAvail == 0 ) {
			return 0;		/* 等待更多数据 */
		}
		iGot = iAvail > sizeof(arrChunk) ? sizeof(arrChunk) : iAvail;
		iGot = XS_HttpPeek(pRec, arrChunk, iGot);
		if ( iGot == 0 ) {
			return 0;
		}
		tData.Data = arrChunk;
		tData.Size = iGot;
		eBody = xrtHttp1BodyRead(&pRec->tBody, tData, false, &iConsumed, &tData, &tErr);
		if ( iConsumed > 0 ) {
			XS_HttpConsume(pRec, iConsumed);
		}
		if ( eBody == XHTTP1_BODY_ERROR ) {
			return -1;
		}
		if ( eBody == XHTTP1_BODY_DONE ) {
			return 1;
		}
		if ( iConsumed == 0 && eBody == XHTTP1_BODY_MORE ) {
			return 0;		/* 需要更多网络数据 */
		}
	}
}

/* TAKEOVER 后的事件表：数据忽略，Close 仅回收记录（流由应用 Destroy） */
static void XS_HttpTakenOnRead(xnetstream* pStream, xnetbuf* pBuffer, ptr pData)
{
	(void)pStream; (void)pBuffer; (void)pData;
}

static void XS_HttpTakenOnClose(xnetstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	(void)pStream; (void)iResult; (void)pError;
	xrtFree(pData);
}

static const xnetstreamevents g_XS_HttpTakenEvents = {
	NULL, XS_HttpTakenOnRead, NULL, NULL, NULL, NULL, XS_HttpTakenOnClose
};

static void XS_HttpTlsTakenOnRead(xtlsstream* pStream, const xnetbuf* pBuffer, ptr pData)
{
	(void)pStream; (void)pBuffer; (void)pData;
}

static void XS_HttpTlsTakenOnClose(xtlsstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	(void)pStream; (void)iResult; (void)pError;
	xrtFree(pData);
}

static const xtlsstreamevents g_XS_HttpTlsTakenEvents = {
	NULL, XS_HttpTlsTakenOnRead, NULL, NULL, NULL, XS_HttpTlsTakenOnClose, NULL
};

/* 回调与收尾（三态处理）。返回 true 继续下一请求，false 停止驱动 */
static bool XS_HttpDispatch(XS_HttpRecord* pRec)
{
	XS_HttpRuntime* pRuntime = pRec->pRuntime;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pRec->pHost->Runtime;
	XS_HttpReq tReq;
	XS_RequestResult eResult = XS_FALLBACK;
	int iDrain;

	tReq.tcp = pRec->tReg.pTcp;
	tReq.tls = pRec->tReg.pTls;
	tReq.head = &pRec->tHead;
	tReq.body = &pRec->tBody;
	tReq.host = pRec->pHost;
	tReq.server = pRuntime->pServer;
	if ( pScript != NULL && pScript->procRequest != NULL ) {
		eResult = pScript->procRequest(&tReq);
	}
	if ( eResult == XS_TAKEOVER ) {
		/* 应用接管：出表 + 换事件表（Close 仅回收记录，不 Destroy） */
		pRec->bTakenOver = true;
		XS_RegistryRemove(&pRuntime->tRegistry, &pRec->tReg);
		if ( pRec->tReg.pTls != NULL ) {
			(void)xrtTlsStreamSetEvents(pRec->tReg.pTls, &g_XS_HttpTlsTakenEvents, pRec);
		} else {
			(void)xrtNetStreamSetEvents(pRec->tReg.pTcp, &g_XS_HttpTakenEvents, pRec);
		}
		return false;
	}
	if ( eResult == XS_FALLBACK ) {
		XS_HttpStatic(pRec);
	}
	/* 排空 body 余量后进入下一请求 */
	iDrain = XS_HttpDrainBody(pRec);
	if ( iDrain < 0 ) {
		XS_HttpFinishRequest(pRec, true);
		return false;
	}
	if ( iDrain == 0 ) {
		pRec->iPhase = 1;
		return false;
	}
	XS_HttpFinishRequest(pRec,
		(pRec->tHead.Flags & XHTTP1_CONNECTION_CLOSE) != 0);
	return !pRec->bTakenOver;
}

static void XS_HttpDrive(XS_HttpRecord* pRec)
{
	XS_HttpRuntime* pRuntime = pRec->pRuntime;
	xhttp1errorinfo tErr;
	int iDrain;

	for ( ; ; ) {
		if ( pRec->iPhase == 2 ) {
			/* body 等待期：数据到达后重新判定就绪 */
			if ( !XS_HttpBodyReady(pRec) ) {
				return;
			}
			if ( !XS_HttpDispatch(pRec) ) {
				return;
			}
			continue;
		}
		if ( pRec->iPhase == 1 ) {
			iDrain = XS_HttpDrainBody(pRec);
			if ( iDrain < 0 ) {
				XS_HttpErrorPage(pRec, 400);
				XS_HttpFinishRequest(pRec, true);
				return;
			}
			if ( iDrain == 0 ) {
				return;
			}
			XS_HttpFinishRequest(pRec,
				(pRec->tHead.Flags & XHTTP1_CONNECTION_CLOSE) != 0);
			if ( pRec->bTakenOver ) {
				return;
			}
		}
		/* phase == HEAD */
		xrtHttp1HeadInit(&pRec->tHead, pRec->arrFields, XS_HTTP_MAX_FIELDS);
		switch ( XS_HttpParseHead(pRec, &tErr) ) {
		case XHTTP1_MORE:
			return;
		case XHTTP1_ERROR:
			XS_HttpErrorPage(pRec, 400);
			XS_HttpFinishRequest(pRec, true);
			return;
		default:
			break;
		}
		XS_HttpConsume(pRec, pRec->tHead.Bytes);
		pRec->pHost = XS_HttpRoute(pRec);

		/* body 预备 */
		if ( !xrtHttp1RequestBodyPlan(&pRec->tHead, &pRec->tPlan) ) {
			XS_HttpErrorPage(pRec, 400);
			XS_HttpFinishRequest(pRec, true);
			return;
		}
		xrtHttp1BodyLimitsInit(&pRec->tBodyLimits);
		if ( pRuntime->iBodyLimit > 0 ) {
			pRec->tBodyLimits.MaxBody = pRuntime->iBodyLimit;
		}
		if ( !xrtHttp1BodyInit(&pRec->tBody, &pRec->tPlan,
			pRec->arrTrailers, XS_HTTP_MAX_TRAILERS, &pRec->tBodyLimits) ) {
			XS_HttpErrorPage(pRec, 400);
			XS_HttpFinishRequest(pRec, true);
			return;
		}

		/* body 就绪检查：回调时保证请求体完整（分段到达时等待）。
		 * FIXED 按字节数精确判断；CHUNKED 用抛弃式探针体判断；
		 * 流式大 body 场景应由脚本用 XS_TAKEOVER 自行处理 */
		if ( !XS_HttpBodyReady(pRec) ) {
			pRec->iPhase = 2;
			return;
		}
		if ( !XS_HttpDispatch(pRec) ) {
			return;
		}
	}
}

/* ============================================================
 * 事件 shim
 * ============================================================ */

static void XS_HttpOnRead(xnetstream* pStream, xnetbuf* pBuffer, ptr pData)
{
	XS_HttpRecord* pRec = (XS_HttpRecord*)pData;

	(void)pStream; (void)pBuffer;
	XS_RegistryTouch(&pRec->tReg);
	XS_HttpDrive(pRec);
}

static void XS_HttpOnClose(xnetstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_HttpRecord* pRec = (XS_HttpRecord*)pData;

	(void)iResult; (void)pError;
	xrtNetStreamDestroy(pStream);
	XS_RegistryRemove(&((XS_HttpRuntime*)pRec->tReg.pHost->Server->Runtime)->tRegistry, &pRec->tReg);
	xrtFree(pRec);
}

static const xnetstreamevents g_XS_HttpStreamEvents = {
	NULL, XS_HttpOnRead, NULL, NULL, NULL, NULL, XS_HttpOnClose
};

static bool XS_HttpOnAccept(xnetlistener* pListener, xnetstream* pStream, ptr pData)
{
	XS_HttpRuntime* pRuntime = (XS_HttpRuntime*)pData;
	XS_HttpRecord* pRec = (XS_HttpRecord*)xrtCalloc(1, sizeof(XS_HttpRecord));

	(void)pListener;
	if ( pRec == NULL ) {
		return false;
	}
	pRec->pRuntime = pRuntime;
	pRec->tReg.pHost = pRuntime->pDefaultHost;
	pRec->tReg.pTcp = pStream;
	xrtHttp1HeadInit(&pRec->tHead, pRec->arrFields, XS_HTTP_MAX_FIELDS);
	XS_RegistryAdd(&pRuntime->tRegistry, &pRec->tReg);
	(void)xrtNetStreamSetData(pStream, pRec);
	return true;
}

static void XS_HttpOnListenerClose(xnetlistener* pListener, ptr pData)
{
	(void)pData;
	xrtNetListenerDestroy(pListener);
}

static const xnetlistenerevents g_XS_HttpListenerEvents = {
	XS_HttpOnAccept, NULL, XS_HttpOnListenerClose
};

/* TLS 形态 shim */

static void XS_HttpTlsOnRead(xtlsstream* pStream, const xnetbuf* pBuffer, ptr pData)
{
	XS_HttpRecord* pRec = (XS_HttpRecord*)pData;

	(void)pStream; (void)pBuffer;
	XS_RegistryTouch(&pRec->tReg);
	XS_HttpDrive(pRec);
}

static void XS_HttpTlsOnClose(xtlsstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_HttpRecord* pRec = (XS_HttpRecord*)pData;

	(void)iResult; (void)pError;
	xrtTlsStreamDestroy(pStream);
	XS_RegistryRemove(&((XS_HttpRuntime*)pRec->tReg.pHost->Server->Runtime)->tRegistry, &pRec->tReg);
	xrtFree(pRec);
}

static const xtlsstreamevents g_XS_HttpTlsStreamEvents = {
	NULL, XS_HttpTlsOnRead, NULL, NULL, NULL, XS_HttpTlsOnClose, NULL
};

static bool XS_HttpTlsOnAccept(xtlslistener* pListener, xtlsstream* pStream, ptr pData)
{
	XS_HttpRuntime* pRuntime = (XS_HttpRuntime*)pData;
	XS_HttpRecord* pRec = (XS_HttpRecord*)xrtCalloc(1, sizeof(XS_HttpRecord));

	(void)pListener;
	if ( pRec == NULL ) {
		return false;
	}
	pRec->pRuntime = pRuntime;
	pRec->tReg.pHost = pRuntime->pDefaultHost;
	pRec->tReg.pTls = pStream;
	xrtHttp1HeadInit(&pRec->tHead, pRec->arrFields, XS_HTTP_MAX_FIELDS);
	XS_RegistryAdd(&pRuntime->tRegistry, &pRec->tReg);
	(void)xrtTlsStreamSetEvents(pStream, &g_XS_HttpTlsStreamEvents, pRec);
	return true;
}

static void XS_HttpTlsOnListenerClose(xtlslistener* pListener, ptr pData)
{
	(void)pData;
	xrtTlsListenerDestroy(pListener);
}

static const xtlslistenerevents g_XS_HttpTlsListenerEvents = {
	XS_HttpTlsOnAccept, NULL, NULL, XS_HttpTlsOnListenerClose
};

/* ============================================================
 * idle 扫描 / 启动 / 停止
 * ============================================================ */

static void XS_HttpSweepProc(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_HttpRuntime* pRuntime = (XS_HttpRuntime*)pData;

	(void)pWorker; (void)iId;
	if ( iResult != XNET_RESULT_OK || pRuntime->bStopping ) {
		return;
	}
	(void)XS_RegistrySweepIdle(&pRuntime->tRegistry, pRuntime->iIdleMs);
	{
		uint64 iInterval = pRuntime->iIdleMs / 2;

		if ( iInterval > 1000 ) iInterval = 1000;
		if ( iInterval < 10 ) iInterval = 10;
		pRuntime->iSweepTimer = xrtNetEngineAfter(pRuntime->pServer->Engine, 0,
			iInterval * 1000, XS_HttpSweepProc, pData);
	}
}

/* 装配期渲染一个 host 的 static.headers：JSON 遍历与字符串拷贝只发生在这里。
 * 报文形态 "name\0value\0" 依次排布进 blob，字段视图直接指入 —— 请求路径零处理 */
static XS_HttpHostHdrs* XS_HttpHdrCacheBuild(XS_HostInfo* pHost)
{
	xvalue* pStatic = XS_HttpStaticCfg(pHost);
	xvalue* pHeaders;
	XS_HttpHostHdrs* pHdrs;
	xvaluekey tKey;
	xvalueiter tIter;
	size_t iUsed = 0;
	size_t iCount = 0;
	str sBlob;
	size_t i;
	size_t iOff;

	if ( pStatic == NULL ) {
		return NULL;
	}
	pHeaders = xrtValueObjectGet(pStatic, XRT_STR_LITERAL("headers"));
	if ( pHeaders == NULL || xrtValueType(pHeaders) != XVALUE_OBJECT ||
	     xrtValueCount(pHeaders) == 0 ) {
		return NULL;
	}
	sBlob = (str)xrtMalloc(4096);
	pHdrs = (XS_HttpHostHdrs*)xrtCalloc(1, sizeof(XS_HttpHostHdrs));
	if ( sBlob == NULL || pHdrs == NULL ) {
		xrtFree(sBlob);
		xrtFree(pHdrs);
		return NULL;
	}
	if ( xrtValueIterBegin(pHeaders, &tIter) ) {
		while ( iCount < XS_HTTP_MAX_EXTRA_FIELDS ) {
			xvalue* pVal = xrtValueIterNext(&tIter, &tKey);
			xstrview tValView;

			if ( pVal == NULL ) {
				break;
			}
			if ( tKey.Type != XVALUE_KEY_STRING || !xrtValueGetString(pVal, &tValView) ) {
				continue;
			}
			if ( iUsed + tKey.String.Size + tValView.Size + 2 > 4096 ) {
				break;
			}
			memcpy(sBlob + iUsed, tKey.String.Data, tKey.String.Size);
			sBlob[iUsed + tKey.String.Size] = 0;
			pHdrs->arrFields[iCount].Name.Data = sBlob + iUsed;
			pHdrs->arrFields[iCount].Name.Size = tKey.String.Size;
			iUsed += tKey.String.Size + 1;
			memcpy(sBlob + iUsed, tValView.Data, tValView.Size);
			sBlob[iUsed + tValView.Size] = 0;
			pHdrs->arrFields[iCount].Value.Data = sBlob + iUsed;
			pHdrs->arrFields[iCount].Value.Size = tValView.Size;
			iUsed += tValView.Size + 1;
			iCount++;
		}
	}
	if ( iCount == 0 ) {
		xrtFree(sBlob);
		xrtFree(pHdrs);
		return NULL;
	}
	pHdrs->pHost = pHost;
	pHdrs->sBlob = sBlob;
	pHdrs->iBlobSize = iUsed;
	pHdrs->iCount = (uint32)iCount;
	(void)iOff;
	(void)i;
	return pHdrs;
}

static const XS_HttpHostHdrs* XS_HttpHdrCacheFind(XS_HttpRuntime* pRuntime, XS_HostInfo* pHost)
{
	uint32 i;

	for ( i = 0; i < pRuntime->iHdrCacheCount; i++ ) {
		if ( pRuntime->arrHdrCache[i]->pHost == pHost ) {
			return pRuntime->arrHdrCache[i];
		}
	}
	return NULL;
}

static void XS_HttpHdrCacheBuildAll(XS_HttpRuntime* pRuntime, XS_ServerInfo* pServer)
{
	uint32 iTotal = 1 + pServer->HostCount;
	uint32 i;

	pRuntime->arrHdrCache = (XS_HttpHostHdrs**)xrtCalloc(iTotal, sizeof(XS_HttpHostHdrs*));
	if ( pRuntime->arrHdrCache == NULL ) {
		return;
	}
	{
		XS_HttpHostHdrs* pHdrs = XS_HttpHdrCacheBuild(pServer->DefaultHost);

		if ( pHdrs != NULL ) {
			pRuntime->arrHdrCache[pRuntime->iHdrCacheCount++] = pHdrs;
		}
	}
	for ( i = 0; i < pServer->HostCount; i++ ) {
		XS_HttpHostHdrs* pHdrs = XS_HttpHdrCacheBuild(pServer->Hosts[i]);

		if ( pHdrs != NULL ) {
			pRuntime->arrHdrCache[pRuntime->iHdrCacheCount++] = pHdrs;
		}
	}
}

static void XS_HttpHdrCacheUnit(XS_HttpRuntime* pRuntime)
{
	uint32 i;

	for ( i = 0; i < pRuntime->iHdrCacheCount; i++ ) {
		xrtFree(pRuntime->arrHdrCache[i]->sBlob);
		xrtFree(pRuntime->arrHdrCache[i]);
	}
	xrtFree(pRuntime->arrHdrCache);
	pRuntime->arrHdrCache = NULL;
	pRuntime->iHdrCacheCount = 0;
}

static bool XS_HttpStart(XS_ServerInfo* pServer, char* sErr, size_t iErrCap)
{
	XS_HttpRuntime* pRuntime = (XS_HttpRuntime*)xrtCalloc(1, sizeof(XS_HttpRuntime));
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pServer->DefaultHost->Runtime;
	xhttp1limits tDef;
	xnetaddr tAddr;
	int64 iVal = 0;

	if ( pRuntime == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	pRuntime->pServer = pServer;
	pRuntime->pDefaultHost = pServer->DefaultHost;
	pServer->Runtime = pRuntime;

	if ( pScript == NULL || pScript->procRequest == NULL ) {
		/* 无 RequestProc：纯静态服务（设计 §5.3：static 也走 fallback） */
		printf("[xs] http server '%s' static-only mode\n", pServer->Name);
	}
	if ( !xrtNetAddrParse(&tAddr, pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port) ) {
		snprintf(sErr, iErrCap, "http server '%s' addr parse failed", pServer->Name);
		return false;
	}
	if ( !XS_RegistryInit(&pRuntime->tRegistry) ) {
		snprintf(sErr, iErrCap, "registry init failed");
		return false;
	}

	/* 限额旋钮：header_limit / path_limit / body_limit / idle_timeout */
	xrtHttp1LimitsInit(&tDef);
	pRuntime->tLimits = tDef;
	(void)XS_CustomGetInt(pServer->Custom, "header_limit", &iVal);
	if ( iVal > 0 && (uint64)iVal < (uint64)XS_HTTP_MAX_FIELDS ) {
		pRuntime->tLimits.MaxFields = (size_t)iVal;
	}
	iVal = 0;
	(void)XS_CustomGetInt(pServer->Custom, "path_limit", &iVal);
	pRuntime->iPathLimit = (iVal > 0) ? (uint64)iVal : 2048;
	iVal = 0;
	(void)XS_CustomGetInt(pServer->Custom, "body_limit", &iVal);
	pRuntime->iBodyLimit = (iVal > 0) ? (uint64)iVal : 0;
	iVal = 0;
	(void)XS_CustomGetInt(pServer->Custom, "idle_timeout", &iVal);
	pRuntime->iIdleMs = (iVal > 0) ? (uint64)iVal : 0;

	if ( !pServer->TLS ) {
		xnetlistenconfig tListen;

		xrtNetListenConfigInit(&tListen);
		tListen.Address = tAddr;
		tListen.ReuseAddress = true;
		tListen.ExclusiveAddress = false;
		if ( pServer->Backlog > 0 ) {
			tListen.Backlog = (int)pServer->Backlog;
		}
		if ( pServer->RecvLimit > 0 ) {
			tListen.Stream.ReadLimit = pServer->RecvLimit;
		}
		pRuntime->pListener = xrtNetListen(pServer->Engine, &tListen,
			&g_XS_HttpListenerEvents, &g_XS_HttpStreamEvents, pRuntime);
		if ( pRuntime->pListener == NULL ) {
			const xerror* pErr = xrtGetError();
			snprintf(sErr, iErrCap, "http server '%s' listen failed (port %u): %s",
				pServer->Name, pServer->Port, pErr ? xrtErrorMessage(pErr) : "unknown");
			return false;
		}
	} else {
		xtlslistenerconfig tTlsListen;
		xtlscontext* pContext = XS_TlsSharedContext();

		if ( pContext == NULL || !XS_TlsTableBuild(pServer, &pRuntime->tTls, sErr, iErrCap) ||
		     pRuntime->tTls.iCount == 0 ) {
			snprintf(sErr + strlen(sErr), iErrCap - strlen(sErr),
				"https server '%s' has no usable tls identity", pServer->Name);
			return false;
		}
		xrtNetListenConfigInit(&tTlsListen.Listen);
		tTlsListen.Listen.Address = tAddr;
		tTlsListen.Listen.ReuseAddress = true;
		tTlsListen.Listen.ExclusiveAddress = false;
		if ( pServer->Backlog > 0 ) {
			tTlsListen.Listen.Backlog = (int)pServer->Backlog;
		}
		if ( pServer->RecvLimit > 0 ) {
			tTlsListen.Listen.Stream.ReadLimit = pServer->RecvLimit;
		}
		xrtTlsServerConfigInit(&tTlsListen.Tls);
		tTlsListen.Tls.Context = pContext;
		tTlsListen.Tls.Identity = pRuntime->tTls.pEntries[0].pIdentity;
		tTlsListen.Tls.Select = XS_TlsSelect;
		tTlsListen.Tls.SelectContext = &pRuntime->tTls;
		pRuntime->bTls = true;
		pRuntime->pTlsListener = xrtTlsListenerStart(pServer->Engine, &tTlsListen,
			&g_XS_HttpTlsListenerEvents, &g_XS_HttpTlsStreamEvents, pRuntime);
		if ( pRuntime->pTlsListener == NULL ) {
			XS_TlsTableUnit(&pRuntime->tTls);
			snprintf(sErr, iErrCap, "https server '%s' listen failed (port %u)", pServer->Name, pServer->Port);
			return false;
		}
	}
	if ( pRuntime->iIdleMs > 0 ) {
		XS_HttpSweepProc(NULL, 0, XNET_RESULT_OK, pRuntime);
	}
	XS_HttpHdrCacheBuildAll(pRuntime, pServer);
	printf("[xs] server '%s' %s ready on %s:%u\n", pServer->Name,
		pRuntime->bTls ? "https" : "http",
		pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port);
	return true;
}

static void XS_HttpStop(XS_HttpRuntime* pRuntime)
{
	if ( pRuntime == NULL ) {
		return;
	}
	pRuntime->bStopping = true;
	if ( pRuntime->iSweepTimer != 0 ) {
		(void)xrtNetEngineTimerCancel(pRuntime->pServer->Engine, pRuntime->iSweepTimer);
		pRuntime->iSweepTimer = 0;
	}
	XS_RegistryCloseAll(&pRuntime->tRegistry);
	if ( pRuntime->pListener != NULL ) {
		xrtNetListenerClose(pRuntime->pListener);
		pRuntime->pListener = NULL;
	}
	if ( pRuntime->pTlsListener != NULL ) {
		xrtTlsListenerClose(pRuntime->pTlsListener);
		pRuntime->pTlsListener = NULL;
	}
}

static void XS_HttpUnit(XS_HttpRuntime* pRuntime)
{
	if ( pRuntime == NULL ) {
		return;
	}
	XS_RegistryUnit(&pRuntime->tRegistry);
	XS_HttpHdrCacheUnit(pRuntime);
	XS_TlsTableUnit(&pRuntime->tTls);
	xrtFree(pRuntime);
}

#endif
