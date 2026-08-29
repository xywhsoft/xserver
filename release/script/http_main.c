/*
 * xs3 http 驱动示例：三态返回全覆盖。
 *   GET /text     —— XS_OK：自建响应（状态行 + 头 + 定长正文）
 *   GET /json     —— XS_OK：xvalue → JSON
 *   POST /echo    —— XS_OK：body 经 xhttp1body 读取后回显
 *   GET /takeover —— XS_TAKEOVER：应用接管连接（发一条提示后自行关闭）
 *   其余          —— XS_FALLBACK：静态层（wwwroot）
 */
#include <xsbase.h>

static int64 g_Tick = 0;
static int64 g_ReloadCount = 0;

static void TickProc(void* pUserData)
{
	(void)pUserData;
	g_Tick++;
}

static bool Reply(XS_HttpReq* pReq, uint16 iStatus, const char* sContentType,
	const void* pBody, size_t iBodyLen)
{
	char arrHead[1024];
	char arrLen[32];
	xhttpfield arrFields[3];
	size_t iFields = 0;
	size_t iHeadLen = 0;
	size_t iWritten = 0;
	xstrview tReason;
	size_t iTotal;

	snprintf(arrLen, sizeof(arrLen), "%llu", (unsigned long long)iBodyLen);
	arrFields[iFields].Name = XRT_STR_LITERAL("Content-Length");
	arrFields[iFields].Value = xrtStrViewN(arrLen, strlen(arrLen));
	iFields++;
	if ( sContentType != NULL ) {
		arrFields[iFields].Name = XRT_STR_LITERAL("Content-Type");
		arrFields[iFields].Value = xrtStrView(sContentType);
		iFields++;
	}
	tReason = xrtHttpStatusText(iStatus);
	if ( !xrtHttp1ResponseWrite(XHTTP_VERSION_1_1, iStatus, tReason,
		arrFields, iFields, arrHead, sizeof(arrHead), &iHeadLen) ) {
		return false;
	}
	iTotal = iHeadLen;
	if ( pReq->tls != NULL ) {
		if ( xrtTlsStreamSend(pReq->tls, arrHead, iHeadLen, &iWritten) != XTLS_OK || iWritten != iHeadLen ) {
			return false;
		}
		if ( iBodyLen > 0 && (xrtTlsStreamSend(pReq->tls, pBody, iBodyLen, &iWritten) != XTLS_OK || iWritten != iBodyLen) ) {
			return false;
		}
	} else {
		if ( xrtNetStreamSend(pReq->tcp, arrHead, iHeadLen) != XNET_RESULT_OK ) {
			return false;
		}
		if ( iBodyLen > 0 && xrtNetStreamSend(pReq->tcp, pBody, iBodyLen) != XNET_RESULT_OK ) {
			return false;
		}
	}
	(void)iTotal;
	return true;
}

static bool ReplyLit(XS_HttpReq* pReq, uint16 iStatus, const char* sContentType, const char* sText)
{
	return Reply(pReq, iStatus, sContentType, sText, strlen(sText));
}

static bool PathIs(XS_HttpReq* pReq, const char* sPath)
{
	size_t i;

	for ( i = 0; sPath[i] != '\0'; i++ ) {}
	return pReq->head->Target.Size == i &&
	       memcmp(pReq->head->Target.Data, sPath, i) == 0;
}

XS_RequestResult RequestProc(XS_HttpReq* pReq)
{
	/* POST /echo：读 body 后回显（演示 xhttp1body 用法） */
	if ( pReq->head->Method.Size == 4 && memcmp(pReq->head->Method.Data, "POST", 4) == 0 &&
	     PathIs(pReq, "/echo") ) {
		unsigned char arrBuf[8192];
		size_t iUsed = 0;
		size_t iAvail;
		size_t iGot;
		size_t iConsumed = 0;
		xbytesview tData;
		xhttp1errorinfo tErr;
		const xnetbuf* pBuf;

		if ( pReq->tls != NULL ) {
			pBuf = xrtTlsStreamBuffer(pReq->tls);
		} else {
			pBuf = xrtNetStreamBuffer(pReq->tcp);
		}
		iAvail = pBuf != NULL ? xrtNetBufSize(pBuf) : 0;
		iGot = iAvail > sizeof(arrBuf) ? sizeof(arrBuf) : iAvail;
		iGot = pBuf != NULL ? xrtNetBufPeek(pBuf, 0, arrBuf, iGot) : 0;
		tData.Data = arrBuf;
		tData.Size = iGot;
		while ( xrtHttp1BodyRead(pReq->body, tData, false, &iConsumed, &tData, &tErr) == XHTTP1_BODY_DATA ) {
			if ( iUsed + tData.Size <= sizeof(arrBuf) ) {
				memcpy(arrBuf + iUsed, tData.Data, tData.Size);
				iUsed += tData.Size;
			}
			tData.Data = arrBuf;
			tData.Size = iGot;	/* BodyRead 的输入视图按消费递进，此处简化为单轮 */
			break;
		}
		(void)iConsumed;
		return Reply(pReq, 200, "application/octet-stream", arrBuf, iUsed) ? XS_OK : XS_OK;
	}
	if ( pReq->head->Method.Size == 3 && memcmp(pReq->head->Method.Data, "GET", 3) == 0 ) {
		if ( PathIs(pReq, "/text") ) {
			return ReplyLit(pReq, 200, "text/plain; charset=utf-8", "xs3 http ok") ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/json") ) {
			xvalue* pObj = xrtValueObject();
			xvalue* pBool;
			str sJson;

			xrtValueObjectSetNew(pObj, XRT_STR_LITERAL("server"), xrtValueString(XRT_STR_LITERAL("xs3")));
			/* 注意：xrtValueBool 返回进程级单例，必须用引用版 Set——
			 * SetNew 会消费并释放单例（UAF 会砸穿堆） */
			pBool = xrtValueBool(true);
			if ( pBool != NULL ) {
				xrtValueObjectSet(pObj, XRT_STR_LITERAL("ok"), pBool);
				xrtValueRelease(pBool);
			}
			xrtValueObjectSetNew(pObj, XRT_STR_LITERAL("port"), xrtValueInt((int64)pReq->server->Port));
			sJson = xrtJsonStringify(pObj, false, NULL);
			xrtValueRelease(pObj);
			if ( sJson != NULL ) {
				bool bOk = Reply(pReq, 200, "application/json; charset=utf-8",
					sJson, xrtStrView(sJson).Size);
				xrtFree(sJson);
				return bOk ? XS_OK : XS_OK;
			}
			return XS_OK;
		}
		if ( PathIs(pReq, "/reload-svr") ) {
			bool bOk = xsReloadServer("main");
			return ReplyLit(pReq, bOk ? 200 : 500, "text/plain; charset=utf-8",
				bOk ? "server reload ok" : "server reload failed") ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/reload-all") ) {
			bool bOk = xsReloadAll();
			return ReplyLit(pReq, bOk ? 200 : 500, "text/plain; charset=utf-8",
				bOk ? "all reload ok" : "all reload failed") ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/reload") ) {
			bool bOk = xsReloadHost(pReq->host);
			return ReplyLit(pReq, bOk ? 200 : 500, "text/plain; charset=utf-8",
				bOk ? "reload ok" : "reload failed") ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/tick") ) {
			(void)xsTimerAfter(pReq->host, 50, TickProc, NULL);
			return ReplyLit(pReq, 200, "text/plain; charset=utf-8", "ticked") ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/tick-get") ) {
			char arrNum[32];
			int iLen = snprintf(arrNum, sizeof(arrNum), "%lld", (long long)g_Tick);
			return Reply(pReq, 200, "text/plain; charset=utf-8", arrNum, (size_t)iLen) ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/swapstat") ) {
			char arrNum[32];
			int iLen = snprintf(arrNum, sizeof(arrNum), "%lld", (long long)g_ReloadCount);
			return Reply(pReq, 200, "text/plain; charset=utf-8", arrNum, (size_t)iLen) ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/takeover") ) {
			/* 演示接管：直接对裸流发提示并 Close；终态 Destroy 由 xs 完成 */
			const char* sMsg = "[xs3] connection taken over by script\n";
			size_t iLen = strlen(sMsg);

			if ( pReq->tls != NULL ) {
				size_t iW = 0;
				(void)xrtTlsStreamSend(pReq->tls, sMsg, iLen, &iW);
				(void)xrtTlsStreamClose(pReq->tls);	/* Close 事件由接管事件表回收记录 */
			} else {
				(void)xrtNetStreamSend(pReq->tcp, sMsg, iLen);
				(void)xrtNetStreamClose(pReq->tcp);
			}
			return XS_TAKEOVER;
		}
	}
	/* 其余：静态层 */
	return XS_FALLBACK;
}

void ServiceInit(XS_HostInfo* pHost)
{
	xvalue* pSwap = xsSwapTake(pHost);

	if ( pSwap != NULL ) {
		(void)xrtValueGetInt(pSwap, &g_ReloadCount);
		xrtValueRelease(pSwap);
	}
}

bool ServiceSwap(XS_HostInfo* pHost, xvalue** ppShared)
{
	(void)pHost;
	*ppShared = xrtValueInt(g_ReloadCount + 1);
	return *ppShared != NULL;
}
