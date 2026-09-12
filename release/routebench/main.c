/*
 * routebench — xs 路由系统基准应用
 *
 * 复用 demo-single 的 modules/protocol.h（静态 map + 方法槽 + xrtPattern 动态表），
 * 全部路由返回常量 "ok"，保证与各语言基准的响应字节一致。
 * 运行：cd release && ./xs routebench/xs.json
 */

#include <xsbase.h>
#include <stdio.h>
#include <string.h>

// HTTP 协议与路由系统
#include "modules/protocol.h"

static bool ConnSend(XS_HttpReq* pReq, const void* pData, size_t iSize)
{
	size_t iW = 0;

	if ( pReq->tls != NULL )
		return xrtTlsStreamSend(pReq->tls, pData, iSize, &iW) == XTLS_OK;
	return xrtNetStreamSend(pReq->tcp, pData, iSize) == XNET_RESULT_OK;
}

static bool ReplyOk(XS_HttpReq* pReq)
{
	static const char arrHead[] =
		"HTTP/1.1 200 OK\r\nContent-Length: 2\r\n"
		"Content-Type: text/plain\r\nConnection: keep-alive\r\n\r\n";

	/* 预构建常量响应：单次发送（P3/P4 优化的形态） */
	return ConnSend(pReq, arrHead, sizeof(arrHead) - 1) &&
	       ConnSend(pReq, "ok", 2);
}

static bool ReplyText(XS_HttpReq* pReq, uint16 iStatus, const char* sText)
{
	char arrHead[256];
	xhttpfield arrF[2];
	size_t iH = 0;
	char arrLen[16];
	xstrview tReason;
	size_t iLen = strlen(sText);

	snprintf(arrLen, sizeof(arrLen), "%llu", (unsigned long long)iLen);
	arrF[0].Name = XRT_STR_LITERAL("Content-Length");
	arrF[0].Value = xrtStrViewN(arrLen, strlen(arrLen));
	tReason = xrtHttpStatusText(iStatus);
	if ( !xrtHttp1ResponseWrite(XHTTP_VERSION_1_1, iStatus, tReason,
		arrF, 1, arrHead, sizeof(arrHead), &iH) ) return false;
	return ConnSend(pReq, arrHead, iH) && ConnSend(pReq, sText, iLen);
}

/* 基准处理函数：静态/动态共用同一常量回复 */
static void Bench_Static(XS_HttpReq* pReq, const RouteParamHTTP* arrParam, uint32 iParamCount)
{
	(void)arrParam;
	(void)iParamCount;
	(void)ReplyOk(pReq);
}

static void Bench_Dynamic(XS_HttpReq* pReq, const RouteParamHTTP* arrParam, uint32 iParamCount)
{
	(void)arrParam;
	(void)iParamCount;
	(void)ReplyOk(pReq);
}

// 路由注册清单（生成）
#include "route.h"

void ServiceInit(XS_HostInfo* pHost)
{
	(void)pHost;
	RouteHTTP_Init();
}

void ServiceUnit(XS_HostInfo* pHost)
{
	(void)pHost;
	RouteHTTP_Unit();
}
