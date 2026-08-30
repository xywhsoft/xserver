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
#include "../runtime/vhost.h"
#include "../script/script.h"
#include "stream.h"

#define XS_HTTP_MAX_FIELDS	48
#define XS_HTTP_MAX_TRAILERS	8
#define XS_HTTP_MAX_EXTRA_FIELDS	8
#define XS_HTTP_SENDFILE_CHUNK	(512 * 1024)
#define XS_HTTP_BODY_WINDOW	16384

typedef struct XS_HttpRuntime {
	XS_ServerInfo*		pServer;
	XS_HostInfo*		pDefaultHost;
	bool			bTls;
	XS_ListenerSlot*	pListenerSlot;
	XS_TlsTable		tTls;
	XS_VHostTable		tVHosts;
	XS_ConnRegistry		tRegistry;
	XS_ServerGeneration*	pGeneration;
	xhttp1limits		tLimits;
	uint64			iBodyLimit;	/* 0 = 内核默认 */
	size_t			iReceiveLimit;	/* 完整请求回调模型的线路硬边界 */
	uint64			iPathLimit;	/* 0 = 默认 2048 */
	uint64			iIdleMs;
	XS_GenerationTimer	tSweepTimer;
	xatomic32		tStopping;
	/* static.headers 装配期预渲染缓存（每 host 一条；请求路径零字符串处理） */
	struct XS_HttpHostHdrs**	arrHdrCache;	/* 指针数组（每 host 一条） */
	uint32			iHdrCacheCount;
} XS_HttpRuntime;

/* host 的自定义响应头缓存：blob 内 "name\0value\0" 依次排布，字段为借用视图 */
typedef struct XS_HttpHostHdrs {
	XS_HostInfo*		pHost;
	xroot			pRoot;		/* 锚定 host 根，所有静态文件操作都在根内解析 */
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
	xhttp1body		tProbeBody;	/* 增量完整性探针，不消费 transport buffer */
	xhttp1bodyplan		tPlan;
	xhttpfield		arrTrailers[XS_HTTP_MAX_TRAILERS];
	xhttpfield		arrProbeTrailers[XS_HTTP_MAX_TRAILERS];
	xhttp1bodylimits	tBodyLimits;
	size_t			iProbeOffset;
	unsigned char*		pHeadData;	/* pin 住 Head 借用的原始字节，覆盖 body 等待期 */
	size_t			iHeadCapacity;
	int			iPhase;		/* 0=HEAD 1=BODY_DRAIN */
	bool			bWriteFailed;	/* 响应开始后任一写失败：禁止复用连接 */
} XS_HttpRecord;

static bool XS_HttpRecordRetain(XS_HttpRecord* pRec)
{
	return pRec != NULL && xrtRefRetain(&pRec->tReg.iReferences) > 0;
}

static void XS_HttpRecordRelease(XS_HttpRecord* pRec)
{
	XS_ScriptRuntime* pScript;
	XS_ServerGeneration* pGeneration;

	if ( pRec == NULL ) return;
	if ( xrtRefRelease(&pRec->tReg.iReferences) != 0 ) return;
	pScript = pRec->tReg.pScript;
	pGeneration = pRec->tReg.pGeneration;
	xrtFree(pRec->pHeadData);
	xrtFree(pRec);
	XS_ScriptRelease(pScript);
	XS_GenerationConnectionRelease(pGeneration);
}

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

static size_t XS_HttpPeekAt(XS_HttpRecord* pRec, size_t iOffset, void* pOut, size_t iSize)
{
	if ( pRec->tReg.pTls != NULL ) {
		const xnetbuf* pBuf = xrtTlsStreamBuffer(pRec->tReg.pTls);
		return pBuf != NULL ? xrtNetBufPeek(pBuf, iOffset, pOut, iSize) : 0;
	}
	return xrtNetBufPeek(xrtNetStreamBuffer(pRec->tReg.pTcp), iOffset, pOut, iSize);
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

static uint16 XS_HttpHeadErrorStatus(xhttp1error iError)
{
	switch ( iError ) {
	case XHTTP1_ERROR_HEAD_TOO_LARGE:
	case XHTTP1_ERROR_START_LINE_TOO_LARGE:
	case XHTTP1_ERROR_FIELD_LINE_TOO_LARGE:
	case XHTTP1_ERROR_TOO_MANY_FIELDS:
		return 431;
	default:
		return 400;
	}
}

/* xhttp1head 的所有字符串/字段都借用解析输入。网络 Header 消费后 body 可能在
 * 后续 Read 中覆盖同一 ring-buffer 区域，因此先复制完整 Header 并重新解析，
 * 让视图在本次请求（包括等待完整 body）期间始终指向连接自有存储。 */
static bool XS_HttpPinHead(XS_HttpRecord* pRec, xhttp1errorinfo* pErr)
{
	unsigned char* pData;
	xbytesview tInput;
	size_t iBytes = pRec->tHead.Bytes;

	if ( iBytes == 0 ) return false;
	if ( pRec->iHeadCapacity < iBytes ) {
		pData = (unsigned char*)xrtRealloc(pRec->pHeadData, iBytes);
		if ( pData == NULL ) return false;
		pRec->pHeadData = pData;
		pRec->iHeadCapacity = iBytes;
	}
	if ( XS_HttpPeek(pRec, pRec->pHeadData, iBytes) != iBytes ) return false;
	xrtHttp1HeadInit(&pRec->tHead, pRec->arrFields, XS_HTTP_MAX_FIELDS);
	tInput.Data = pRec->pHeadData;
	tInput.Size = iBytes;
	return xrtHttp1RequestParse(tInput, &pRec->tHead, &pRec->pRuntime->tLimits, pErr) ==
		XHTTP1_READY;
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
	unsigned char* pHead = arrHead;
	xhttpfield arrFields[16];
	size_t iFieldCount = 0;
	size_t i;
	xstrview tReason;
	size_t iSize = 0;
	xhttpversion eVersion;
	char arrLen[32];
	bool bResult;

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
	eVersion = pRec->tHead.Version == XHTTP_VERSION_1_0 ?
		XHTTP_VERSION_1_0 : XHTTP_VERSION_1_1;
	if ( !xrtHttp1ResponseWrite(eVersion, pResp->iStatus, tReason,
		arrFields, iFieldCount, NULL, 0, &iSize) ) {
		return false;
	}
	if ( iSize > sizeof(arrHead) ) {
		pHead = (unsigned char*)xrtMalloc(iSize);
		if ( pHead == NULL ) return false;
	}
	bResult = xrtHttp1ResponseWrite(eVersion, pResp->iStatus, tReason,
		arrFields, iFieldCount, pHead, iSize, &iSize) &&
		XS_HttpSend(&pRec->tReg, pHead, iSize) == XNET_RESULT_OK;
	if ( pHead != arrHead ) xrtFree(pHead);
	return bResult;
}

static const char* XS_HttpMime(const char* sName);
static const XS_HttpHostHdrs* XS_HttpHdrCacheFind(XS_HttpRuntime* pRuntime, XS_HostInfo* pHost);
static str XS_HttpHostRoot(XS_HttpRuntime* pRuntime, XS_HostInfo* pHost);

static bool XS_HttpSafeRelative(xstrview tPath)
{
	size_t i;

	if ( tPath.Size == 0 || tPath.Data == NULL ) return false;
	for ( i = 0; i < tPath.Size; i++ ) {
		unsigned char c = (unsigned char)tPath.Data[i];

		if ( c == 0 || c < 0x20u || c == 0x7fu || c == '\\' ) return false;
	}
	return xrtPathIsLocal(tPath, XPATH_NATIVE);
}

static xfile XS_HttpRootOpenFile(xroot pRoot, const char* sRel, xfileinfo* pInfo)
{
	xfileoptions tOpt;
	xfile hFile;

	if ( pRoot == NULL || sRel == NULL || sRel[0] == '\0' || pInfo == NULL ) return NULL;
	xrtFileOptionsInit(&tOpt);
	tOpt.Flags = XFILE_READ;
	tOpt.Share = XFILE_SHARE_READ;
	hFile = xrtRootFileOpen(pRoot, sRel, &tOpt);
	if ( hFile == NULL ) return NULL;
	if ( !xrtFileStat(hFile, pInfo) || pInfo->Type != XFILE_TYPE_FILE ) {
		xrtClose(hFile);
		return NULL;
	}
	return hFile;
}

/* 静态文件统一走有界分块发送：明文由 sendfile 零拷贝，TLS 仅持有
 * 一个固定大小的临时块。错误页不再把整个可配置文件读入内存。 */
static bool XS_HttpSendFileData(XS_HttpRecord* pRec, xfile hFile, uint64 iSize)
{
	if ( pRec->tReg.pTcp != NULL ) {
		uint64 iOffset = 0;

		while ( iOffset < iSize ) {
			uint64 iChunk = iSize - iOffset;

			if ( iChunk > XS_HTTP_SENDFILE_CHUNK ) iChunk = XS_HTTP_SENDFILE_CHUNK;
			if ( xrtNetStreamSendFile(pRec->tReg.pTcp, hFile,
				iOffset, (size_t)iChunk) != XNET_RESULT_OK ) return false;
			iOffset += iChunk;
		}
		return true;
	}
	{
		unsigned char* pBuffer = (unsigned char*)xrtMalloc(XS_HTTP_SENDFILE_CHUNK);
		uint64 iLeft = iSize;
		bool bOk = pBuffer != NULL;

		while ( bOk && iLeft > 0 ) {
			size_t iChunk = iLeft > XS_HTTP_SENDFILE_CHUNK ?
				XS_HTTP_SENDFILE_CHUNK : (size_t)iLeft;
			size_t iRead = 0;

			bOk = xrtRead(hFile, pBuffer, iChunk, &iRead) && iRead > 0 &&
				XS_HttpSend(&pRec->tReg, pBuffer, iRead) == XNET_RESULT_OK;
			if ( bOk ) iLeft -= iRead;
		}
		xrtFree(pBuffer);
		return bOk;
	}
}

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
				const XS_HttpHostHdrs* pSite = XS_HttpHdrCacheFind(
					pRec->pRuntime, pRec->pHost);
				xfile hFile;
				xfileinfo tInfo;
				bool bHeadOnly;

				memcpy(arrRel, tRel.Data, tRel.Size);
				arrRel[tRel.Size] = '\0';
				hFile = pSite != NULL && pSite->pRoot != NULL &&
					XS_HttpSafeRelative(tRel) ?
					XS_HttpRootOpenFile(pSite->pRoot, arrRel, &tInfo) : NULL;
				if ( hFile != NULL ) {
					bHeadOnly = pRec->tHead.MethodCode == XHTTP_METHOD_HEAD;
					memset(&tResp, 0, sizeof(tResp));
					tResp.iStatus = iStatus;
					tResp.sContentType = strchr(arrRel, '.') != NULL
						? XS_HttpMime(arrRel) : "text/html; charset=utf-8";
					tResp.iContentLength = tInfo.Size;
					tResp.bHeadOnly = bHeadOnly;
					if ( pSite->iCount > 0 ) {
						memcpy(tResp.arrExtra, pSite->arrFields,
							sizeof(xhttpfield) * pSite->iCount);
						tResp.iExtraCount = pSite->iCount;
					}
					if ( !XS_HttpRespond(pRec, &tResp) ||
					     (!bHeadOnly && tInfo.Size > 0 &&
					      !XS_HttpSendFileData(pRec, hFile, tInfo.Size)) ) {
						pRec->bWriteFailed = true;
					}
					xrtClose(hFile);
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
	tResp.bHeadOnly = pRec->tHead.MethodCode == XHTTP_METHOD_HEAD;
	if ( !XS_HttpRespond(pRec, &tResp) ||
	     (!tResp.bHeadOnly &&
	      XS_HttpSend(&pRec->tReg, arrBody, (size_t)iLen) != XNET_RESULT_OK) ) {
		pRec->bWriteFailed = true;
	}
}

/* ============================================================
 * 静态层（XS_FALLBACK，仅 GET/HEAD）
 * ============================================================ */

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
		const char* sLeft = sName + j;
		const char* sRight = arrMime[i].sExt;
		size_t k = 0;

		for ( ; ; k++ ) {
			unsigned char a = (unsigned char)sLeft[k];
			unsigned char b = (unsigned char)sRight[k];

			if ( a >= 'A' && a <= 'Z' ) a = (unsigned char)(a + ('a' - 'A'));
			if ( b >= 'A' && b <= 'Z' ) b = (unsigned char)(b + ('a' - 'A'));
			if ( a != b || a == 0 ) break;
		}
		if ( sLeft[k] == '\0' && sRight[k] == '\0' ) {
			return arrMime[i].sMime;
		}
	}
	return "application/octet-stream";
}

/* host 根路径解析（相对 appPath；结果缓存于运行时线性表） */
static str XS_HttpHostRoot(XS_HttpRuntime* pRuntime, XS_HostInfo* pHost)
{
	str sPath;

	(void)pRuntime;
	if ( pHost->Path == NULL ) {
		return xrtStrDup(XS_AppPath());
	}
	if ( xrtPathIsAbs(pHost->Path) ) {
		return xrtStrDup(pHost->Path);
	}
	sPath = xrtPathJoin(XS_AppPath(), pHost->Path);
	return sPath;
}

static void XS_HttpStatic(XS_HttpRecord* pRec)
{
	XS_HttpRuntime* pRuntime = pRec->pRuntime;
	const XS_HttpHostHdrs* pSite;
	xhttptarget tParsedTarget;
	xstrview tTarget;
	char* sEncoded = NULL;
	char* sDecoded = NULL;
	char* sRel = NULL;
	char* sSelected = NULL;
	xfile hFile = NULL;
	xfileinfo tInfo;
	size_t iEncoded;
	size_t iDecoded = 0;
	size_t iStart;
	size_t i;
	bool bDirectory = false;
	bool bHeadOnly;

	if ( (pRec->tHead.MethodCode != XHTTP_METHOD_GET) &&
	     (pRec->tHead.MethodCode != XHTTP_METHOD_HEAD) ) {
		XS_HttpErrorPage(pRec, 405);
		return;
	}
	if ( !xrtHttpTargetParse(pRec->tHead.Method, pRec->tHead.Target, &tParsedTarget) ) {
		XS_HttpErrorPage(pRec, 400);
		return;
	}
	tTarget = tParsedTarget.Path;
	if ( tTarget.Size == 0 && tParsedTarget.Form == XHTTP_TARGET_ABSOLUTE ) {
		tTarget = XRT_STR_LITERAL("/");
	}
	for ( iEncoded = 0;
	      iEncoded < tTarget.Size && tTarget.Data[iEncoded] != '?';
	      iEncoded++ ) {}
	if ( iEncoded == 0 || iEncoded == SIZE_MAX ) {
		XS_HttpErrorPage(pRec, 400);
		return;
	}
	sEncoded = (char*)xrtMalloc(iEncoded + 1u);
	sDecoded = (char*)xrtMalloc(iEncoded + 1u);
	if ( sEncoded == NULL || sDecoded == NULL ) {
		XS_HttpErrorPage(pRec, 500);
		goto cleanup;
	}
	memcpy(sEncoded, tTarget.Data, iEncoded);
	sEncoded[iEncoded] = '\0';
	if ( !xrtPercentDecode(xrtStrViewN(sEncoded, iEncoded),
		sDecoded, iEncoded, &iDecoded) ) {
		XS_HttpErrorPage(pRec, 400);
		goto cleanup;
	}
	sDecoded[iDecoded] = '\0';
	if ( pRuntime->iPathLimit > 0 && iDecoded > pRuntime->iPathLimit ) {
		XS_HttpErrorPage(pRec, 414);
		goto cleanup;
	}
	if ( iDecoded == 0 || sDecoded[0] != '/' ) {
		XS_HttpErrorPage(pRec, 400);
		goto cleanup;
	}
	if ( iDecoded > 1 && sDecoded[1] == '/' ) {
		XS_HttpErrorPage(pRec, 403);
		goto cleanup;
	}
	for ( i = 0; i < iDecoded; i++ ) {
		unsigned char c = (unsigned char)sDecoded[i];

		if ( c == 0 || c < 0x20u || c == 0x7fu ) {
			XS_HttpErrorPage(pRec, 400);
			goto cleanup;
		}
		if ( c == '\\' ) {
			XS_HttpErrorPage(pRec, 403);
			goto cleanup;
		}
	}
	sRel = sDecoded + 1;
	if ( sRel[0] != '\0' && !xrtPathIsLocal(
		xrtStrViewN(sRel, iDecoded - 1u), XPATH_NATIVE) ) {
		XS_HttpErrorPage(pRec, 403);
		goto cleanup;
	}

	/* 词法检查负责策略，xroot 负责最终的链接/重解析点根约束。 */
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
			if ( i == iDecoded || sDecoded[i] == '/' ) {
				size_t iLen = i - iStart;

				if ( iLen == 2 && sDecoded[iStart] == '.' && sDecoded[iStart + 1] == '.' ) {
					XS_HttpErrorPage(pRec, 403);
					goto cleanup;
				}
				if ( bDenyDot && iLen > 0 && sDecoded[iStart] == '.' ) {
					XS_HttpErrorPage(pRec, 403);
					goto cleanup;
				}
				iStart = i + 1;
			}
		}
	}

	pSite = XS_HttpHdrCacheFind(pRuntime, pRec->pHost);
	if ( pSite == NULL ) {
		XS_HttpErrorPage(pRec, 500);
		goto cleanup;
	}
	if ( pSite->pRoot == NULL ) {
		XS_HttpErrorPage(pRec, 404);
		goto cleanup;
	}
	if ( sRel[0] == '\0' ) {
		bDirectory = true;
	} else if ( !xrtRootStat(pSite->pRoot, sRel, true, &tInfo) ) {
		XS_HttpErrorPage(pRec, 404);
		goto cleanup;
	} else if ( tInfo.Type == XFILE_TYPE_DIRECTORY ) {
		bDirectory = true;
	} else if ( tInfo.Type != XFILE_TYPE_FILE ) {
		XS_HttpErrorPage(pRec, 404);
		goto cleanup;
	}

	if ( bDirectory ) {
		xvalue* pStatic = XS_HttpStaticCfg(pRec->pHost);
		xvalue* pIndex = pStatic != NULL ?
			xrtValueObjectGet(pStatic, XRT_STR_LITERAL("index")) : NULL;
		size_t iCount = pIndex != NULL && xrtValueType(pIndex) == XVALUE_ARRAY ?
			xrtValueCount(pIndex) : 1u;
		size_t iIdx;

		for ( iIdx = 0; iIdx < iCount && hFile == NULL; iIdx++ ) {
			xstrview tName;
			bool bHas;
			size_t iBase = strlen(sRel);
			size_t iNeed;

			if ( pIndex != NULL && xrtValueType(pIndex) == XVALUE_ARRAY ) {
				xvalue* pItem = xrtValueArrayGet(pIndex, iIdx);
				bHas = pItem != NULL && xrtValueGetString(pItem, &tName);
			} else {
				tName = XRT_STR_LITERAL("index.html");
				bHas = true;
			}
			if ( !bHas || !XS_HttpSafeRelative(tName) || tName.Data[0] == '.' ||
			     iBase > SIZE_MAX - tName.Size - 2u ) continue;
			iNeed = iBase + (iBase > 0 ? 1u : 0u) + tName.Size + 1u;
			sSelected = (char*)xrtMalloc(iNeed);
			if ( sSelected == NULL ) {
				XS_HttpErrorPage(pRec, 500);
				goto cleanup;
			}
			if ( iBase > 0 ) {
				memcpy(sSelected, sRel, iBase);
				sSelected[iBase++] = '/';
			}
			memcpy(sSelected + iBase, tName.Data, tName.Size);
			sSelected[iBase + tName.Size] = '\0';
			hFile = XS_HttpRootOpenFile(pSite->pRoot, sSelected, &tInfo);
			if ( hFile == NULL ) {
				xrtFree(sSelected);
				sSelected = NULL;
			}
		}
		if ( hFile == NULL ) {
			XS_HttpErrorPage(pRec, 404);
			goto cleanup;
		}
	} else {
		sSelected = xrtStrDup(sRel);
		if ( sSelected == NULL ) {
			XS_HttpErrorPage(pRec, 500);
			goto cleanup;
		}
		hFile = XS_HttpRootOpenFile(pSite->pRoot, sSelected, &tInfo);
		if ( hFile == NULL ) {
			XS_HttpErrorPage(pRec, 404);
			goto cleanup;
		}
	}

	bHeadOnly = pRec->tHead.MethodCode == XHTTP_METHOD_HEAD;
	{
		XS_HttpResp tResp;

		memset(&tResp, 0, sizeof(tResp));
		tResp.iStatus = 200;
		tResp.sContentType = XS_HttpMime(sSelected);
		tResp.iContentLength = tInfo.Size;
		tResp.bHeadOnly = bHeadOnly;
		if ( pSite->iCount > 0 ) {
			memcpy(tResp.arrExtra, pSite->arrFields,
				sizeof(xhttpfield) * pSite->iCount);
			tResp.iExtraCount = pSite->iCount;
		}
		if ( !XS_HttpRespond(pRec, &tResp) ) {
			pRec->bWriteFailed = true;
			goto cleanup;
		}
	}
	if ( !bHeadOnly && tInfo.Size > 0 &&
	     !XS_HttpSendFileData(pRec, hFile, tInfo.Size) ) {
		pRec->bWriteFailed = true;
	}

cleanup:
	if ( hFile != NULL ) xrtClose(hFile);
	xrtFree(sSelected);
	xrtFree(sDecoded);
	xrtFree(sEncoded);
}

/* ============================================================
 * body 排空与 keep-alive 循环
 * ============================================================ */

/* body 完整性增量探针：1=完整，0=等待，-1=协议/状态错误，-2=线路上限。
 * 探针只推进独立 Reader 与偏移，transport buffer 留给脚本使用。 */
static int XS_HttpBodyReady(XS_HttpRecord* pRec)
{
	size_t iAvail = XS_HttpAvail(pRec);
	unsigned char arrChunk[XS_HTTP_BODY_WINDOW];

	if ( pRec->tPlan.Mode == XHTTP1_BODY_NONE ) {
		return 1;
	}
	if ( pRec->tPlan.Mode == XHTTP1_BODY_FIXED ) {
		if ( pRec->tPlan.Length > (uint64)SIZE_MAX ) return -1;
		if ( iAvail >= (size_t)pRec->tPlan.Length ) return 1;
		return iAvail >= pRec->pRuntime->iReceiveLimit ? -2 : 0;
	}
	if ( pRec->tPlan.Mode != XHTTP1_BODY_CHUNKED || pRec->iProbeOffset > iAvail ) {
		return -1;
	}
	while ( pRec->iProbeOffset < iAvail ) {
		size_t iGot = iAvail - pRec->iProbeOffset;
		size_t iLocal = 0;

		if ( iGot > sizeof(arrChunk) ) iGot = sizeof(arrChunk);
		if ( XS_HttpPeekAt(pRec, pRec->iProbeOffset, arrChunk, iGot) != iGot ) {
			return 0;
		}
		while ( iLocal < iGot ) {
			xbytesview tInput;
			xbytesview tData;
			xhttp1errorinfo tErr;
			xhttp1bodystatus iStatus;
			size_t iConsumed = 0;

			tInput.Data = arrChunk + iLocal;
			tInput.Size = iGot - iLocal;
			iStatus = xrtHttp1BodyRead(&pRec->tProbeBody, tInput, false,
				&iConsumed, &tData, &tErr);
			if ( iConsumed > tInput.Size ) return -1;
			iLocal += iConsumed;
			pRec->iProbeOffset += iConsumed;
			if ( iStatus == XHTTP1_BODY_DONE ) return 1;
			if ( iStatus == XHTTP1_BODY_ERROR ) {
				return tErr.Code == XHTTP1_ERROR_BODY_TOO_LARGE ? -2 : -1;
			}
			if ( iStatus == XHTTP1_BODY_FIELDS ) return -1;
			if ( iConsumed == 0 ) {
				if ( iStatus != XHTTP1_BODY_MORE ) return -1;
				return iAvail >= pRec->pRuntime->iReceiveLimit ? -2 : 0;
			}
		}
	}
	if ( xrtHttp1BodyDone(&pRec->tProbeBody) ) return 1;
	return iAvail >= pRec->pRuntime->iReceiveLimit ? -2 : 0;
}

static void XS_HttpFinishRequest(XS_HttpRecord* pRec, bool bClose)
{
	xrtHttp1HeadInit(&pRec->tHead, pRec->arrFields, XS_HTTP_MAX_FIELDS);
	pRec->pHost = NULL;
	pRec->tReg.pHost = NULL;
	pRec->iPhase = 0;
	pRec->bWriteFailed = false;
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
	unsigned char arrChunk[XS_HTTP_BODY_WINDOW];
	xhttp1errorinfo tErr;

	for ( ; ; ) {
		size_t iAvail = XS_HttpAvail(pRec);
		size_t iGot;
		size_t iConsumed = 0;
		xbytesview tData;
		xhttp1bodystatus eBody;

		iGot = iAvail > sizeof(arrChunk) ? sizeof(arrChunk) : iAvail;
		if ( iGot > 0 && XS_HttpPeek(pRec, arrChunk, iGot) != iGot ) {
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
		if ( eBody == XHTTP1_BODY_FIELDS ) {
			return -1;
		}
		if ( iConsumed == 0 ) {
			return 0;		/* 需要更多网络数据 */
		}
	}
}

/* TAKEOVER 后仍保留框架终态事件：应用负责最终 Close，框架在 Close 中 Destroy。 */
static void XS_HttpTakenOnRead(xnetstream* pStream, xnetbuf* pBuffer, ptr pData)
{
	(void)pStream; (void)pBuffer; (void)pData;
}

static void XS_HttpTakenOnEnd(xnetstream* pStream, ptr pData)
{
	(void)pData;
	(void)xrtNetStreamClose(pStream);
}

static void XS_HttpTakenOnClose(xnetstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_HttpRecord* pRec = (XS_HttpRecord*)pData;

	(void)iResult; (void)pError;
	XS_RegistryRemove(pRec->tReg.pRegistry, &pRec->tReg);
	xrtNetStreamDestroy(pStream);
	XS_HttpRecordRelease(pRec);
}

static const xnetstreamevents g_XS_HttpTakenEvents = {
	NULL, XS_HttpTakenOnRead, XS_HttpTakenOnEnd, NULL, NULL, NULL, XS_HttpTakenOnClose
};

static void XS_HttpTlsTakenOnRead(xtlsstream* pStream, const xnetbuf* pBuffer, ptr pData)
{
	(void)pStream; (void)pBuffer; (void)pData;
}

static void XS_HttpTlsTakenOnEnd(xtlsstream* pStream, ptr pData)
{
	(void)pData;
	(void)xrtTlsStreamClose(pStream);
}

static void XS_HttpTlsTakenOnClose(xtlsstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_HttpRecord* pRec = (XS_HttpRecord*)pData;

	(void)iResult; (void)pError;
	XS_RegistryRemove(pRec->tReg.pRegistry, &pRec->tReg);
	xrtTlsStreamDestroy(pStream);
	XS_HttpRecordRelease(pRec);
}

static const xtlsstreamevents g_XS_HttpTlsTakenEvents = {
	NULL, XS_HttpTlsTakenOnRead, XS_HttpTlsTakenOnEnd, NULL, NULL, XS_HttpTlsTakenOnClose, NULL
};

/* 回调与收尾（三态处理）。返回 true 继续下一请求，false 停止驱动 */
static bool XS_HttpDispatch(XS_HttpRecord* pRec)
{
	XS_HttpRuntime* pRuntime = pRec->pRuntime;
	XS_ScriptRuntime* pScript = XS_ScriptAcquireHost(pRec->pHost);
	XS_HttpReq tReq;
	XS_RequestResult eResult = XS_FALLBACK;
	uint64 iWireBefore = pRec->tBody.WireBytes;
	uint64 iWireUsed;
	int iDrain;
	bool bClose;

	tReq.tcp = pRec->tReg.pTcp;
	tReq.tls = pRec->tReg.pTls;
	tReq.head = &pRec->tHead;
	tReq.body = &pRec->tBody;
	tReq.host = pRec->pHost;
	tReq.server = pRuntime->pServer;
	if ( pRec->pHost->DevFile != NULL && pRec->pHost->DevFile[0] != '\0' &&
	     (pRec->pHost->DevLang == NULL || strcmp(pRec->pHost->DevLang, "c") == 0) &&
	     pScript == NULL ) {
		XS_HttpErrorPage(pRec, 503);
		XS_HttpFinishRequest(pRec, true);
		return false;
	}
	if ( pScript != NULL && pScript->procRequest != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		eResult = pScript->procRequest(&tReq);
		XS_ScriptLeave(pPrevious);
	}
	/* 脚本通过公开 Body Reader 推进了解码状态，但网络 buffer 的消费权仍在驱动。
	 * WireBytes 是本 reader 已消费的线路字节数，精确同步其增量后再排空余量。 */
	if ( pRec->tBody.WireBytes < iWireBefore ) {
		XS_ScriptRelease(pScript);
		XS_HttpFinishRequest(pRec, true);
		return false;
	}
	iWireUsed = pRec->tBody.WireBytes - iWireBefore;
	if ( iWireUsed > (uint64)XS_HttpAvail(pRec) ) {
		XS_ScriptRelease(pScript);
		XS_HttpFinishRequest(pRec, true);
		return false;
	}
	if ( iWireUsed > 0 ) XS_HttpConsume(pRec, (size_t)iWireUsed);
	if ( eResult == XS_TAKEOVER ) {
		bool bInstalled;

		/* 应用接管仍属于本 generation；终态 Close 才出表并释放引用。 */
		pRec->tReg.pScript = pScript;
		if ( pRec->tReg.pTls != NULL ) {
			bInstalled = xrtTlsStreamSetEvents(
				pRec->tReg.pTls, &g_XS_HttpTlsTakenEvents, pRec);
		} else {
			bInstalled = xrtNetStreamSetEvents(
				pRec->tReg.pTcp, &g_XS_HttpTakenEvents, pRec);
		}
		if ( !bInstalled ) {
			pRec->tReg.pScript = NULL;
			XS_ScriptRelease(pScript);
			XS_HttpFinishRequest(pRec, true);
			return false;
		}
		return false;
	}
	XS_ScriptRelease(pScript);
	if ( eResult != XS_OK && eResult != XS_FALLBACK ) {
		/* ABI 外的返回值没有可恢复语义；禁止误当作成功后复用连接。 */
		XS_HttpFinishRequest(pRec, true);
		return false;
	}
	if ( eResult == XS_FALLBACK ) {
		XS_HttpStatic(pRec);
		if ( pRec->bWriteFailed ) {
			XS_HttpFinishRequest(pRec, true);
			return false;
		}
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
	bClose = (pRec->tHead.Flags & XHTTP1_CONNECTION_CLOSE) != 0;
	XS_HttpFinishRequest(pRec, bClose);
	return !bClose;
}

static void XS_HttpDrive(XS_HttpRecord* pRec)
{
	XS_HttpRuntime* pRuntime = pRec->pRuntime;
	xhttp1errorinfo tErr;
	int iDrain;

	for ( ; ; ) {
		if ( pRec->iPhase == 2 ) {
			int iReady;

			/* body 等待期：数据到达后重新判定就绪 */
			iReady = XS_HttpBodyReady(pRec);
			if ( iReady < 0 ) {
				XS_HttpErrorPage(pRec, iReady == -2 ? 413 : 400);
				XS_HttpFinishRequest(pRec, true);
				return;
			}
			if ( iReady == 0 ) {
				return;
			}
			if ( !XS_HttpDispatch(pRec) ) {
				return;
			}
			continue;
		}
		if ( pRec->iPhase == 1 ) {
			bool bClose;

			iDrain = XS_HttpDrainBody(pRec);
			if ( iDrain < 0 ) {
				XS_HttpErrorPage(pRec, 400);
				XS_HttpFinishRequest(pRec, true);
				return;
			}
			if ( iDrain == 0 ) {
				return;
			}
			bClose = (pRec->tHead.Flags & XHTTP1_CONNECTION_CLOSE) != 0;
			XS_HttpFinishRequest(pRec, bClose);
			if ( bClose ) return;
		}
		/* phase == HEAD */
		pRec->pHost = pRuntime->pDefaultHost;
		pRec->tReg.pHost = pRec->pHost;
		xrtHttp1HeadInit(&pRec->tHead, pRec->arrFields, XS_HTTP_MAX_FIELDS);
		switch ( XS_HttpParseHead(pRec, &tErr) ) {
		case XHTTP1_MORE:
			if ( XS_HttpAvail(pRec) >= pRuntime->iReceiveLimit ) {
				XS_HttpErrorPage(pRec, 431);
				XS_HttpFinishRequest(pRec, true);
			}
			return;
		case XHTTP1_ERROR:
			XS_HttpErrorPage(pRec, XS_HttpHeadErrorStatus(tErr.Code));
			XS_HttpFinishRequest(pRec, true);
			return;
		default:
			break;
		}
		if ( !XS_HttpPinHead(pRec, &tErr) ) {
			pRec->pHost = pRuntime->pDefaultHost;
			XS_HttpErrorPage(pRec, 500);
			XS_HttpFinishRequest(pRec, true);
			return;
		}
		{
			XS_VHostResult eRoute = XS_VHostRouteHead(&pRuntime->tVHosts,
				&pRec->tHead, pRec->tHead.Version == XHTTP_VERSION_1_1,
				&pRec->pHost);

			if ( eRoute != XS_VHOST_FOUND ) {
				pRec->pHost = pRuntime->pDefaultHost;
				pRec->tReg.pHost = pRec->pHost;
				XS_HttpErrorPage(pRec,
					eRoute == XS_VHOST_BAD_REQUEST ? 400 : 404);
				XS_HttpFinishRequest(pRec, true);
				return;
			}
			pRec->tReg.pHost = pRec->pHost;
		}

		/* body 预备 */
		if ( !xrtHttp1RequestBodyPlan(&pRec->tHead, &pRec->tPlan) ) {
			XS_HttpErrorPage(pRec, 400);
			XS_HttpFinishRequest(pRec, true);
			return;
		}
		xrtHttp1BodyLimitsInit(&pRec->tBodyLimits);
		pRec->tBodyLimits.MaxBody = pRuntime->iBodyLimit > 0 ?
			pRuntime->iBodyLimit : (uint64)pRuntime->iReceiveLimit;
		pRec->tBodyLimits.MaxTrailers = XS_HTTP_MAX_TRAILERS;
		/* Reader 的 trailer 解析要求完整连续区；窗口上限防止分块探针零推进。 */
		if ( pRec->tBodyLimits.MaxTrailer > XS_HTTP_BODY_WINDOW ) {
			pRec->tBodyLimits.MaxTrailer = XS_HTTP_BODY_WINDOW;
		}
		if ( pRec->tBodyLimits.MaxTrailerLine > pRec->tBodyLimits.MaxTrailer ) {
			pRec->tBodyLimits.MaxTrailerLine = pRec->tBodyLimits.MaxTrailer;
		}
		if ( pRec->tPlan.Mode == XHTTP1_BODY_FIXED &&
		     (pRec->tPlan.Length > pRec->tBodyLimits.MaxBody ||
		      pRec->tPlan.Length > (uint64)pRuntime->iReceiveLimit) ) {
			XS_HttpErrorPage(pRec, 413);
			XS_HttpFinishRequest(pRec, true);
			return;
		}
		if ( !xrtHttp1BodyInit(&pRec->tBody, &pRec->tPlan,
			pRec->arrTrailers, XS_HTTP_MAX_TRAILERS, &pRec->tBodyLimits) ) {
			XS_HttpErrorPage(pRec, 400);
			XS_HttpFinishRequest(pRec, true);
			return;
		}
		pRec->iProbeOffset = 0;
		if ( !xrtHttp1BodyInit(&pRec->tProbeBody, &pRec->tPlan,
			pRec->arrProbeTrailers, XS_HTTP_MAX_TRAILERS, &pRec->tBodyLimits) ) {
			XS_HttpErrorPage(pRec, 400);
			XS_HttpFinishRequest(pRec, true);
			return;
		}
		XS_HttpConsume(pRec, pRec->tHead.Bytes);

		/* body 就绪检查：回调时保证请求体完整（分段到达时等待）。
		 * FIXED 按字节数判断；CHUNKED 用独立增量 reader 探测且不消费网络数据。
		 * recv_limit 是等待完整请求期间的线路硬边界，填满仍未完成则拒绝。 */
		{
			int iReady = XS_HttpBodyReady(pRec);

			if ( iReady < 0 ) {
				XS_HttpErrorPage(pRec, iReady == -2 ? 413 : 400);
				XS_HttpFinishRequest(pRec, true);
				return;
			}
			if ( iReady == 0 ) {
				pRec->iPhase = 2;
				return;
			}
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

	(void)pBuffer;
	if ( !XS_HttpRecordRetain(pRec) ) return;
	if ( xrtNetStreamRef(pStream) == NULL ) {
		XS_HttpRecordRelease(pRec);
		return;
	}
	XS_RegistryTouch(&pRec->tReg);
	XS_HttpDrive(pRec);
	xrtNetStreamDestroy(pStream);
	XS_HttpRecordRelease(pRec);
}

static void XS_HttpOnEnd(xnetstream* pStream, ptr pData)
{
	(void)pData;
	(void)xrtNetStreamClose(pStream);
}

static void XS_HttpOnClose(xnetstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_HttpRecord* pRec = (XS_HttpRecord*)pData;

	(void)iResult; (void)pError;
	XS_RegistryRemove(pRec->tReg.pRegistry, &pRec->tReg);
	xrtNetStreamDestroy(pStream);
	XS_HttpRecordRelease(pRec);
}

static const xnetstreamevents g_XS_HttpStreamEvents = {
	NULL, XS_HttpOnRead, XS_HttpOnEnd, NULL, NULL, NULL, XS_HttpOnClose
};

static bool XS_HttpOnAccept(xnetlistener* pListener, xnetstream* pStream, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;
	XS_HttpRuntime* pRuntime = NULL;
	XS_ServerGeneration* pGeneration = NULL;
	XS_HttpRecord* pRec = (XS_HttpRecord*)xrtCalloc(1, sizeof(XS_HttpRecord));

	(void)pListener;
	if ( pRec == NULL ) return false;
	pRec->tReg.iReferences = 1;
	if ( !XS_ListenerSlotAcquireConnection(pSlot, 0, (void**)&pRuntime, &pGeneration) ) {
		XS_HttpRecordRelease(pRec);
		return false;
	}
	pRec->tReg.pGeneration = pGeneration;
	if ( xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
		XS_HttpRecordRelease(pRec);
		return false;
	}
	pRec->pRuntime = pRuntime;
	pRec->pHost = pRuntime->pDefaultHost;
	pRec->tReg.pHost = pRuntime->pDefaultHost;
	pRec->tReg.pTcp = pStream;
	xrtHttp1HeadInit(&pRec->tHead, pRec->arrFields, XS_HTTP_MAX_FIELDS);
	if ( !XS_RegistryAdd(&pRuntime->tRegistry, &pRec->tReg) ) {
		XS_HttpRecordRelease(pRec);
		return false;
	}
	if ( !xrtNetStreamSetData(pStream, pRec) ) {
		XS_RegistryRemove(&pRuntime->tRegistry, &pRec->tReg);
		XS_HttpRecordRelease(pRec);
		return false;
	}
	return true;
}

static void XS_HttpOnListenerClose(xnetlistener* pListener, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;

	XS_ListenerSlotResourceClose(pSlot, XS_LISTENER_RESOURCE_PLAIN);
	xrtNetListenerDestroy(pListener);
}

static const xnetlistenerevents g_XS_HttpListenerEvents = {
	XS_HttpOnAccept, NULL, XS_HttpOnListenerClose
};

/* TLS 形态 shim */

static void XS_HttpTlsOnRead(xtlsstream* pStream, const xnetbuf* pBuffer, ptr pData)
{
	XS_HttpRecord* pRec = (XS_HttpRecord*)pData;

	(void)pBuffer;
	if ( !XS_HttpRecordRetain(pRec) ) return;
	if ( xrtTlsStreamRef(pStream) == NULL ) {
		XS_HttpRecordRelease(pRec);
		return;
	}
	XS_RegistryTouch(&pRec->tReg);
	XS_HttpDrive(pRec);
	xrtTlsStreamDestroy(pStream);
	XS_HttpRecordRelease(pRec);
}

static void XS_HttpTlsOnEnd(xtlsstream* pStream, ptr pData)
{
	(void)pData;
	(void)xrtTlsStreamClose(pStream);
}

static void XS_HttpTlsOnClose(xtlsstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_HttpRecord* pRec = (XS_HttpRecord*)pData;

	(void)iResult; (void)pError;
	XS_RegistryRemove(pRec->tReg.pRegistry, &pRec->tReg);
	xrtTlsStreamDestroy(pStream);
	XS_HttpRecordRelease(pRec);
}

static const xtlsstreamevents g_XS_HttpTlsStreamEvents = {
	NULL, XS_HttpTlsOnRead, XS_HttpTlsOnEnd, NULL, NULL, XS_HttpTlsOnClose, NULL
};

static bool XS_HttpTlsOnAccept(xtlslistener* pListener, xtlsstream* pStream, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;
	XS_HttpRuntime* pRuntime = NULL;
	XS_ServerGeneration* pGeneration = NULL;
	XS_HttpRecord* pRec = (XS_HttpRecord*)xrtCalloc(1, sizeof(XS_HttpRecord));

	(void)pListener;
	if ( pRec == NULL ) return false;
	pRec->tReg.iReferences = 1;
	if ( !XS_TlsAcquireConnection(pSlot, pStream, (void**)&pRuntime, &pGeneration) ) {
		XS_HttpRecordRelease(pRec);
		return false;
	}
	pRec->tReg.pGeneration = pGeneration;
	if ( xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
		XS_HttpRecordRelease(pRec);
		return false;
	}
	pRec->pRuntime = pRuntime;
	pRec->pHost = pRuntime->pDefaultHost;
	pRec->tReg.pHost = pRuntime->pDefaultHost;
	pRec->tReg.pTls = pStream;
	xrtHttp1HeadInit(&pRec->tHead, pRec->arrFields, XS_HTTP_MAX_FIELDS);
	if ( !XS_RegistryAdd(&pRuntime->tRegistry, &pRec->tReg) ) {
		XS_HttpRecordRelease(pRec);
		return false;
	}
	if ( !xrtTlsStreamSetEvents(pStream, &g_XS_HttpTlsStreamEvents, pRec) ) {
		XS_RegistryRemove(&pRuntime->tRegistry, &pRec->tReg);
		XS_HttpRecordRelease(pRec);
		return false;
	}
	return true;
}

static void XS_HttpTlsOnListenerClose(xtlslistener* pListener, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;

	XS_ListenerSlotResourceClose(pSlot, XS_LISTENER_RESOURCE_TLS);
	xrtTlsListenerDestroy(pListener);
}

static const xtlslistenerevents g_XS_HttpTlsListenerEvents = {
	XS_HttpTlsOnAccept, XS_TlsHandshakeError, NULL, XS_HttpTlsOnListenerClose
};

/* ============================================================
 * idle 扫描 / 启动 / 停止
 * ============================================================ */

static void XS_HttpSweepProc(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_HttpRuntime* pRuntime = (XS_HttpRuntime*)pData;
	XS_ServerGeneration* pGeneration = XS_GenerationTimerFinish(&pRuntime->tSweepTimer);

	(void)pWorker; (void)iId;
	if ( pGeneration == NULL ) return;
	if ( iResult == XNET_RESULT_OK &&
	     xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) == 0 ) {
		(void)XS_RegistrySweepIdle(&pRuntime->tRegistry, pRuntime->iIdleMs);
		{
			uint64 iInterval = pRuntime->iIdleMs / 2;

			if ( iInterval > 1000 ) iInterval = 1000;
			if ( iInterval < 10 ) iInterval = 10;
			if ( XS_GenerationTimerSchedule(pRuntime->pGeneration,
				iInterval * 1000, XS_HttpSweepProc, pData,
				pRuntime, &pRuntime->tSweepTimer) != 0 &&
			     xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
				XS_GenerationTimerCancelOwner(pRuntime->pGeneration, pRuntime);
			}
		}
	}
	XS_GenerationActivityRelease(pGeneration);
	XS_GenerationRelease(pGeneration);
}

static bool XS_HttpScheduleSweep(XS_HttpRuntime* pRuntime)
{
	uint64 iInterval = pRuntime->iIdleMs / 2;

	if ( iInterval > 1000 ) iInterval = 1000;
	if ( iInterval < 10 ) iInterval = 10;
	return XS_GenerationTimerSchedule(pRuntime->pGeneration,
		iInterval * 1000, XS_HttpSweepProc, pRuntime,
		pRuntime, &pRuntime->tSweepTimer) != 0;
}

/* 装配期渲染一个 host 的 static.headers：JSON 遍历与字符串拷贝只发生在这里。
 * 报文形态 "name\0value\0" 依次排布进 blob，字段视图直接指入 —— 请求路径零处理 */
static void XS_HttpHdrCacheDiscard(XS_HttpHostHdrs* pHdrs)
{
	if ( pHdrs == NULL ) return;
	if ( pHdrs->pRoot != NULL ) xrtRootClose(pHdrs->pRoot);
	xrtFree(pHdrs->sBlob);
	xrtFree(pHdrs);
}

static XS_HttpHostHdrs* XS_HttpHdrCacheBuild(
	XS_HostInfo* pHost,
	char* sErr,
	size_t iErrCap)
{
	xvalue* pStatic = XS_HttpStaticCfg(pHost);
	xvalue* pHeaders;
	xvalue* pOption;
	XS_HttpHostHdrs* pHdrs;
	xvaluekey tKey;
	xvalueiter tIter;
	size_t iUsed = 0;
	size_t iCount = 0;
	str sBlob = NULL;
	str sRoot;

	pHdrs = (XS_HttpHostHdrs*)xrtCalloc(1, sizeof(XS_HttpHostHdrs));
	if ( pHdrs == NULL ) return NULL;
	pHdrs->pHost = pHost;
	sRoot = XS_HttpHostRoot(NULL, pHost);
	if ( sRoot != NULL ) {
		pHdrs->pRoot = xrtRootOpen(sRoot);
		xrtFree(sRoot);
	}
	if ( pStatic == NULL ) return pHdrs;
	if ( xrtValueType(pStatic) != XVALUE_OBJECT ) {
		snprintf(sErr, iErrCap, "http host '%s' custom field 'static' expect object",
			pHost->Name != NULL ? pHost->Name : "?");
		goto failed;
	}
	pOption = xrtValueObjectGet(pStatic, XRT_STR_LITERAL("deny_dotfiles"));
	if ( pOption != NULL ) {
		bool bValue;

		if ( !xrtValueGetBool(pOption, &bValue) ) {
			snprintf(sErr, iErrCap,
				"http host '%s' static.deny_dotfiles expect bool",
				pHost->Name != NULL ? pHost->Name : "?");
			goto failed;
		}
	}
	pOption = xrtValueObjectGet(pStatic, XRT_STR_LITERAL("index"));
	if ( pOption != NULL ) {
		size_t i;

		if ( xrtValueType(pOption) != XVALUE_ARRAY ) {
			snprintf(sErr, iErrCap, "http host '%s' static.index expect array",
				pHost->Name != NULL ? pHost->Name : "?");
			goto failed;
		}
		for ( i = 0; i < xrtValueCount(pOption); i++ ) {
			xvalue* pItem = xrtValueArrayGet(pOption, i);
			xstrview tName;

			if ( pItem == NULL || !xrtValueGetString(pItem, &tName) ||
			     !XS_HttpSafeRelative(tName) || tName.Data[0] == '.' ) {
				snprintf(sErr, iErrCap,
					"http host '%s' static.index[%llu] expect safe relative string",
					pHost->Name != NULL ? pHost->Name : "?",
					(unsigned long long)i);
				goto failed;
			}
		}
	}
	pOption = xrtValueObjectGet(pStatic, XRT_STR_LITERAL("error_pages"));
	if ( pOption != NULL ) {
		if ( xrtValueType(pOption) != XVALUE_OBJECT ) {
			snprintf(sErr, iErrCap, "http host '%s' static.error_pages expect object",
				pHost->Name != NULL ? pHost->Name : "?");
			goto failed;
		}
		if ( xrtValueIterBegin(pOption, &tIter) ) {
			for ( ;; ) {
				xvalue* pVal = xrtValueIterNext(&tIter, &tKey);
				xstrview tPath;

				if ( pVal == NULL ) break;
				if ( tKey.Type != XVALUE_KEY_STRING ||
				     !xrtValueGetString(pVal, &tPath) ||
				     !XS_HttpSafeRelative(tPath) ) {
					snprintf(sErr, iErrCap,
						"http host '%s' static.error_pages values expect safe relative strings",
						pHost->Name != NULL ? pHost->Name : "?");
					goto failed;
				}
			}
		}
	}
	pHeaders = xrtValueObjectGet(pStatic, XRT_STR_LITERAL("headers"));
	if ( pHeaders == NULL ) {
		return pHdrs;
	}
	if ( xrtValueType(pHeaders) != XVALUE_OBJECT ) {
		snprintf(sErr, iErrCap, "http host '%s' static.headers expect object",
			pHost->Name != NULL ? pHost->Name : "?");
		goto failed;
	}
	if ( xrtValueCount(pHeaders) > XS_HTTP_MAX_EXTRA_FIELDS ) {
		snprintf(sErr, iErrCap, "http host '%s' static.headers exceeds %u fields",
			pHost->Name != NULL ? pHost->Name : "?", XS_HTTP_MAX_EXTRA_FIELDS);
		goto failed;
	}
	if ( xrtValueCount(pHeaders) == 0 ) return pHdrs;
	if ( xrtValueIterBegin(pHeaders, &tIter) ) {
		for ( ;; ) {
			xvalue* pVal = xrtValueIterNext(&tIter, &tKey);
			xstrview tValView;

			if ( pVal == NULL ) break;
			if ( tKey.Type != XVALUE_KEY_STRING ||
			     !xrtValueGetString(pVal, &tValView) ||
			     !xrtHttpTokenValid(tKey.String) ||
			     !xrtHttpFieldValueValid(tValView) ) {
				snprintf(sErr, iErrCap,
					"http host '%s' static.headers contains invalid HTTP field",
					pHost->Name != NULL ? pHost->Name : "?");
				goto failed;
			}
			if ( tValView.Size > SIZE_MAX - 2u ||
			     tKey.String.Size > SIZE_MAX - tValView.Size - 2u ||
			     iUsed > SIZE_MAX - (tKey.String.Size + tValView.Size + 2u) ) {
				snprintf(sErr, iErrCap, "http host '%s' static.headers is too large",
					pHost->Name != NULL ? pHost->Name : "?");
				goto failed;
			}
			iUsed += tKey.String.Size + tValView.Size + 2u;
			iCount++;
		}
	}
	sBlob = (str)xrtMalloc(iUsed);
	if ( sBlob == NULL ) {
		snprintf(sErr, iErrCap, "out of memory building static.headers");
		goto failed;
	}
	iUsed = 0;
	iCount = 0;
	if ( xrtValueIterBegin(pHeaders, &tIter) ) {
		for ( ;; ) {
			xvalue* pVal = xrtValueIterNext(&tIter, &tKey);
			xstrview tValView;

			if ( pVal == NULL ) break;
			(void)xrtValueGetString(pVal, &tValView);
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
		return pHdrs;
	}
	pHdrs->sBlob = sBlob;
	pHdrs->iBlobSize = iUsed;
	pHdrs->iCount = (uint32)iCount;
	return pHdrs;

failed:
	XS_HttpHdrCacheDiscard(pHdrs);
	return NULL;
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

static bool XS_HttpHdrCacheBuildAll(
	XS_HttpRuntime* pRuntime,
	XS_ServerInfo* pServer,
	char* sErr,
	size_t iErrCap)
{
	uint32 iTotal = 1 + pServer->HostCount;
	uint32 i;

	pRuntime->arrHdrCache = (XS_HttpHostHdrs**)xrtCalloc(iTotal, sizeof(XS_HttpHostHdrs*));
	if ( pRuntime->arrHdrCache == NULL ) {
		return false;
	}
	{
		XS_HttpHostHdrs* pHdrs;

		if ( pServer->DefaultHost->Enabled ) {
			pHdrs = XS_HttpHdrCacheBuild(pServer->DefaultHost, sErr, iErrCap);
			if ( pHdrs == NULL ) return false;
			pRuntime->arrHdrCache[pRuntime->iHdrCacheCount++] = pHdrs;
		}
	}
	for ( i = 0; i < pServer->HostCount; i++ ) {
		XS_HttpHostHdrs* pHdrs;

		if ( !pServer->Hosts[i]->Enabled ) continue;
		pHdrs = XS_HttpHdrCacheBuild(pServer->Hosts[i], sErr, iErrCap);
		if ( pHdrs == NULL ) return false;
		pRuntime->arrHdrCache[pRuntime->iHdrCacheCount++] = pHdrs;
	}
	return true;
}

static void XS_HttpHdrCacheUnit(XS_HttpRuntime* pRuntime)
{
	uint32 i;

	for ( i = 0; i < pRuntime->iHdrCacheCount; i++ ) {
		XS_HttpHdrCacheDiscard(pRuntime->arrHdrCache[i]);
	}
	xrtFree(pRuntime->arrHdrCache);
	pRuntime->arrHdrCache = NULL;
	pRuntime->iHdrCacheCount = 0;
}

static bool XS_HttpStartEx(
	XS_ServerInfo* pServer,
	bool bStartEndpoint,
	bool bAcceptEndpoint,
	char* sErr,
	size_t iErrCap)
{
	XS_HttpRuntime* pRuntime = (XS_HttpRuntime*)xrtCalloc(1, sizeof(XS_HttpRuntime));
	xhttp1limits tDef;
	xnetlistenconfig tNetDef;
	xnetaddr tAddr;
	xnetaddr tTlsAddr;
	uint64 iVal = 0;
	uint32 i;
	bool bHasRequestProc = false;

	if ( pRuntime == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	pRuntime->pServer = pServer;
	pRuntime->pDefaultHost = pServer->DefaultHost;
	pRuntime->pGeneration = (XS_ServerGeneration*)pServer->Generation;
	xrtAtomic32Init(&pRuntime->tStopping, 0);
	pServer->Runtime = pRuntime;

	for ( i = 0; i <= pServer->HostCount; i++ ) {
		XS_HostInfo* pHost = i == 0 ? pServer->DefaultHost : pServer->Hosts[i - 1];
		XS_ScriptRuntime* pScript = pHost != NULL && pHost->Enabled ?
			(XS_ScriptRuntime*)pHost->Runtime : NULL;

		if ( pScript != NULL && pScript->procRequest != NULL ) {
			bHasRequestProc = true;
			break;
		}
	}
	if ( !bHasRequestProc ) {
		/* 无 RequestProc：纯静态服务（设计 §5.3：static 也走 fallback） */
		printf("[xs] http server '%s' static-only mode\n", pServer->Name);
	}
	if ( !xrtNetAddrParse(&tAddr, pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port) ) {
		snprintf(sErr, iErrCap, "http server '%s' addr parse failed", pServer->Name);
		return false;
	}
	if ( pServer->TLS && !xrtNetAddrParse(&tTlsAddr,
		pServer->IPTLS ? pServer->IPTLS : (pServer->IP ? pServer->IP : "0.0.0.0"),
		pServer->PortTLS) ) {
		snprintf(sErr, iErrCap, "https server '%s' tls addr parse failed", pServer->Name);
		return false;
	}
	if ( !XS_RegistryInit(&pRuntime->tRegistry) ) {
		snprintf(sErr, iErrCap, "registry init failed");
		return false;
	}
	if ( !XS_VHostTableBuild(pServer, &pRuntime->tVHosts, sErr, iErrCap) ) {
		return false;
	}

	/* 限额旋钮：header_limit / path_limit / body_limit / idle_timeout */
	xrtNetListenConfigInit(&tNetDef);
	pRuntime->iReceiveLimit = pServer->RecvLimit > 0 ?
		pServer->RecvLimit : tNetDef.Stream.ReadLimit;
	xrtHttp1LimitsInit(&tDef);
	pRuntime->tLimits = tDef;
	if ( !XS_CustomReadUInt(pServer->Custom, "header_limit", &iVal,
		sErr, iErrCap) ) return false;
	if ( iVal > (uint64)SIZE_MAX ) {
		snprintf(sErr, iErrCap, "custom field 'header_limit' out of range");
		return false;
	}
	if ( iVal > 0 ) pRuntime->tLimits.MaxHead = (size_t)iVal;
	if ( pRuntime->tLimits.MaxFields > XS_HTTP_MAX_FIELDS ) {
		pRuntime->tLimits.MaxFields = XS_HTTP_MAX_FIELDS;
	}
	if ( !XS_CustomReadUInt(pServer->Custom, "path_limit", &iVal,
		sErr, iErrCap) ) return false;
	pRuntime->iPathLimit = iVal > 0 ? iVal : 2048;
	if ( !XS_CustomReadUInt(pServer->Custom, "body_limit", &iVal,
		sErr, iErrCap) ) return false;
	pRuntime->iBodyLimit = iVal;
	if ( pRuntime->tLimits.MaxHead > pRuntime->iReceiveLimit ) {
		snprintf(sErr, iErrCap,
			"http server '%s' header_limit exceeds recv_limit", pServer->Name);
		return false;
	}
	if ( pRuntime->iBodyLimit > (uint64)pRuntime->iReceiveLimit ) {
		snprintf(sErr, iErrCap,
			"http server '%s' body_limit exceeds recv_limit", pServer->Name);
		return false;
	}
	if ( !XS_CustomReadUInt(pServer->Custom, "idle_timeout", &iVal,
		sErr, iErrCap) ) return false;
	pRuntime->iIdleMs = iVal;
	if ( !XS_HttpHdrCacheBuildAll(pRuntime, pServer, sErr, iErrCap) ) {
		if ( sErr[0] == '\0' ) {
			snprintf(sErr, iErrCap, "http static site cache allocation failed");
		}
		return false;
	}

	if ( pServer->TLS ) {
		if ( XS_TlsSharedContext() == NULL ||
		     !XS_TlsTableBuild(pServer, &pRuntime->tTls, sErr, iErrCap) ||
		     pRuntime->tTls.iCount == 0 ) {
			if ( sErr[0] == '\0' ) {
				snprintf(sErr, iErrCap,
					"https server '%s' has no usable tls identity", pServer->Name);
			}
			return false;
		}
		pRuntime->bTls = true;
	}
	if ( bStartEndpoint ) {
		pRuntime->pListenerSlot = XS_ListenerSlotCreate(pRuntime, pRuntime->pGeneration,
			pRuntime->bTls ? (void*)&pRuntime->tTls : NULL, bAcceptEndpoint);
		if ( pRuntime->pListenerSlot == NULL ) {
			snprintf(sErr, iErrCap, "http listener slot create failed");
			return false;
		}
	}

	if ( bStartEndpoint ) {
		xnetlistenconfig tListen;
		xnetlistener* pListener;

		xrtNetListenConfigInit(&tListen);
		tListen.Address = tAddr;
		tListen.ReuseAddress = true;
		tListen.ExclusiveAddress = false;
		if ( pServer->Backlog > 0 ) {
			tListen.Backlog = (int)pServer->Backlog;
		}
		XS_StreamApplyReceiveLimit(&tListen.Stream, pServer->RecvLimit);
		if ( !XS_ListenerSlotResourceAdd(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_PLAIN) ) return false;
		pListener = xrtNetListen(pServer->Engine, &tListen,
			&g_XS_HttpListenerEvents, &g_XS_HttpStreamEvents, pRuntime->pListenerSlot);
		if ( pListener == NULL ) {
			XS_ListenerSlotResourceCancel(pRuntime->pListenerSlot,
				XS_LISTENER_RESOURCE_PLAIN);
			XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
			pRuntime->pListenerSlot = NULL;
			const xerror* pErr = xrtGetError();
			snprintf(sErr, iErrCap, "http server '%s' listen failed (port %u): %s",
				pServer->Name, pServer->Port, pErr ? xrtErrorMessage(pErr) : "unknown");
			return false;
		}
		if ( !XS_ListenerSlotResourceAttach(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_PLAIN, pListener) ) {
			XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
			pRuntime->pListenerSlot = NULL;
			snprintf(sErr, iErrCap, "http server '%s' listener closed during start",
				pServer->Name);
			return false;
		}
	}
	if ( bStartEndpoint && pServer->TLS ) {
		xtlslistenerconfig tTlsListen;
		xtlscontext* pContext = XS_TlsSharedContext();
		xtlslistener* pTlsListener;

		xrtTlsListenerConfigInit(&tTlsListen);
		tTlsListen.Listen.Address = tTlsAddr;
		tTlsListen.Listen.ReuseAddress = true;
		tTlsListen.Listen.ExclusiveAddress = false;
		if ( pServer->Backlog > 0 ) {
			tTlsListen.Listen.Backlog = (int)pServer->Backlog;
		}
		XS_StreamApplyReceiveLimit(&tTlsListen.Listen.Stream, pServer->RecvLimit);
		tTlsListen.Tls.Context = pContext;
		tTlsListen.Tls.Identity = pRuntime->tTls.pEntries[0].pIdentity;
		tTlsListen.Tls.Select = XS_TlsSlotSelect;
		tTlsListen.Tls.SelectContext = pRuntime->pListenerSlot;
		if ( !XS_ListenerSlotResourceAdd(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_TLS) ) return false;
		pTlsListener = xrtTlsListenerStart(pServer->Engine, &tTlsListen,
			&g_XS_HttpTlsListenerEvents, NULL, pRuntime->pListenerSlot);
		if ( pTlsListener == NULL ) {
			XS_ListenerSlotResourceCancel(pRuntime->pListenerSlot,
				XS_LISTENER_RESOURCE_TLS);
			snprintf(sErr, iErrCap, "https server '%s' listen failed (port %u)", pServer->Name, pServer->PortTLS);
			return false;
		}
		if ( !XS_ListenerSlotResourceAttach(pRuntime->pListenerSlot,
		     XS_LISTENER_RESOURCE_TLS, pTlsListener) ) {
			snprintf(sErr, iErrCap, "https server '%s' listener closed during start",
				pServer->Name);
			return false;
		}
	}
	if ( pRuntime->iIdleMs > 0 ) {
		if ( !XS_HttpScheduleSweep(pRuntime) ) {
			snprintf(sErr, iErrCap, "http server '%s' idle timer start failed", pServer->Name);
			return false;
		}
	}
	printf("[xs] server '%s' http%s %s on %s:%u", pServer->Name,
		pRuntime->bTls ? "+https" : "",
		bStartEndpoint ? (bAcceptEndpoint ? "ready" : "bound") : "prepared",
		pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port);
	if ( pRuntime->bTls ) {
		printf(" and %s:%u", pServer->IPTLS ? pServer->IPTLS :
			(pServer->IP ? pServer->IP : "0.0.0.0"), pServer->PortTLS);
	}
	printf("\n");
	return true;
}

static bool XS_HttpHandoff(XS_HttpRuntime* pOld, XS_HttpRuntime* pNew)
{
	XS_ListenerSlot* pSlot;

	if ( pOld == NULL || pNew == NULL || pOld->bTls != pNew->bTls ||
	     pOld->pListenerSlot == NULL ) return false;
	pSlot = pOld->pListenerSlot;
	if ( !XS_ListenerSlotHandoff(pSlot, pOld, pNew, pNew->pGeneration,
		pNew->bTls ? (void*)&pNew->tTls : NULL) ) return false;
	pNew->pListenerSlot = pSlot;
	pOld->pListenerSlot = NULL;
	return true;
}

static void XS_HttpStop(XS_HttpRuntime* pRuntime)
{
	XS_ListenerSlot* pSlot;
	XS_ListenerResources tResources;

	if ( pRuntime == NULL ) {
		return;
	}
	if ( xrtAtomic32Exchange(&pRuntime->tStopping, 1, XMEMORY_ACQ_REL) != 0 ) return;
	XS_GenerationTimerCancelOwner(pRuntime->pGeneration, pRuntime);
	XS_RegistryStopAccepting(&pRuntime->tRegistry);
	pSlot = pRuntime->pListenerSlot;
	pRuntime->pListenerSlot = NULL;
	if ( pSlot != NULL && XS_ListenerSlotBeginClose(pSlot, pRuntime, &tResources) ) {
		if ( tResources.pPlain != NULL ) {
			xrtNetListenerClose(tResources.pPlain);
			xrtNetListenerDestroy(tResources.pPlain);
		}
		if ( tResources.pTls != NULL ) {
			xrtTlsListenerClose(tResources.pTls);
			xrtTlsListenerDestroy(tResources.pTls);
		}
	}
}

static void XS_HttpCloseConnections(XS_HttpRuntime* pRuntime)
{
	if ( pRuntime != NULL ) {
		XS_RegistryCloseAll(&pRuntime->tRegistry);
	}
}

static void XS_HttpUnit(XS_HttpRuntime* pRuntime)
{
	if ( pRuntime == NULL ) {
		return;
	}
	XS_RegistryUnit(&pRuntime->tRegistry);
	XS_HttpHdrCacheUnit(pRuntime);
	XS_VHostTableUnit(&pRuntime->tVHosts);
	XS_TlsTableUnit(&pRuntime->tTls);
	if ( pRuntime->pListenerSlot != NULL ) {
		XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
		pRuntime->pListenerSlot = NULL;
	}
	xrtFree(pRuntime);
}

#endif
