/*
 * xs3 http 驱动示例：三态返回全覆盖。
 *   GET /text     —— XS_OK：自建响应（状态行 + 头 + 定长正文）
 *   GET /json     —— XS_OK：xvalue → JSON
 *   POST /echo    —— XS_OK：body 经 xhttp1body 读取后回显
 *   GET /takeover —— XS_TAKEOVER：应用接管连接（发一条提示后自行关闭）
 *   其余          —— XS_FALLBACK：静态层（wwwroot）
 */
#include <xsbase.h>
#include <stdio.h>
#include <string.h>

static int64 g_Tick = 0;
static int64 g_ReloadCount = 0;
static int64 g_Ready = 0;
static XS_ServerInfo* g_HeldServer = NULL;

static int64 ConfigInt(xvalue* pObject, const char* sName)
{
	xvalue* pValue = pObject != NULL ?
		xrtValueObjectGet(pObject, xrtStrViewN(sName, strlen(sName))) : NULL;
	int64 iValue = 0;

	if ( pValue != NULL ) (void)xrtValueGetInt(pValue, &iValue);
	return iValue;
}

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

static const char* ReloadStateName(XS_ReloadState iState)
{
	switch ( iState ) {
	case XS_RELOAD_ACCEPTED: return "accepted";
	case XS_RELOAD_PREPARING: return "preparing";
	case XS_RELOAD_SUCCEEDED: return "succeeded";
	case XS_RELOAD_FAILED: return "failed";
	case XS_RELOAD_SUPERSEDED: return "superseded";
	case XS_RELOAD_CANCELLED: return "cancelled";
	default: return "unknown";
	}
}

static bool ReloadStatusId(XS_HttpReq* pReq, XS_ReloadId* pId)
{
	static const char sPrefix[] = "/reload-status/";
	size_t iPrefix = sizeof(sPrefix) - 1;
	size_t i;
	uint64 iValue = 0;

	if ( pReq->head->Target.Size <= iPrefix ||
	     memcmp(pReq->head->Target.Data, sPrefix, iPrefix) != 0 ) return false;
	for ( i = iPrefix; i < pReq->head->Target.Size; i++ ) {
		unsigned char c = (unsigned char)pReq->head->Target.Data[i];

		if ( c < '0' || c > '9' || iValue > (UINT64_MAX - (uint64)(c - '0')) / 10 ) return false;
		iValue = iValue * 10 + (uint64)(c - '0');
	}
	if ( iValue == 0 ) return false;
	*pId = iValue;
	return true;
}

static XS_RequestResult ReplyReloadAccepted(XS_HttpReq* pReq, XS_ReloadId iId)
{
	char arrJson[128];
	int iLen;

	if ( iId == 0 ) {
		return ReplyLit(pReq, 503, "application/json; charset=utf-8",
			"{\"status\":\"rejected\"}") ? XS_OK : XS_OK;
	}
	iLen = snprintf(arrJson, sizeof(arrJson),
		"{\"reload_id\":%llu,\"status\":\"accepted\"}", (unsigned long long)iId);
	return Reply(pReq, 202, "application/json; charset=utf-8", arrJson, (size_t)iLen) ? XS_OK : XS_OK;
}

XS_RequestResult RequestProc(XS_HttpReq* pReq)
{
	/* POST /echo：读 body 后回显（演示 xhttp1body 用法） */
	if ( pReq->head->MethodCode == XHTTP_METHOD_POST &&
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
				/* chunked 解码视图可能指向 arrBuf 内部，源/目标允许重叠。 */
				memmove(arrBuf + iUsed, tData.Data, tData.Size);
				iUsed += tData.Size;
			}
			tData.Data = arrBuf;
			tData.Size = iGot;	/* BodyRead 的输入视图按消费递进，此处简化为单轮 */
			break;
		}
		(void)iConsumed;
		return Reply(pReq, 200, "application/octet-stream", arrBuf, iUsed) ? XS_OK : XS_OK;
	}
	if ( pReq->head->MethodCode == XHTTP_METHOD_GET ) {
		XS_ReloadId iStatusId;

		if ( ReloadStatusId(pReq, &iStatusId) ) {
			XS_ReloadResult tResult;
			char arrJson[256];
			int iLen;

			if ( !xsReloadQuery(iStatusId, &tResult) ) {
				return ReplyLit(pReq, 404, "application/json; charset=utf-8",
					"{\"status\":\"unknown\"}") ? XS_OK : XS_OK;
			}
			iLen = snprintf(arrJson, sizeof(arrJson),
				"{\"reload_id\":%llu,\"status\":\"%s\",\"revision\":%llu}",
				(unsigned long long)tResult.Id, ReloadStateName(tResult.State),
				(unsigned long long)tResult.Revision);
			return Reply(pReq, 200, "application/json; charset=utf-8",
				arrJson, (size_t)iLen) ? XS_OK : XS_OK;
		}
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
			return ReplyReloadAccepted(pReq, xsReloadServerSubmit("main"));
		}
		if ( PathIs(pReq, "/reload-all") ) {
			return ReplyReloadAccepted(pReq, xsReloadAllSubmit());
		}
		if ( PathIs(pReq, "/reload") ) {
			return ReplyReloadAccepted(pReq, xsReloadHostSubmit(pReq->host));
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
		if ( PathIs(pReq, "/configstat") ) {
			char arrNum[32];
			int iLen = snprintf(arrNum, sizeof(arrNum), "%lld",
				(long long)ConfigInt(pReq->server->Custom, "config_marker"));
			return Reply(pReq, 200, "text/plain; charset=utf-8", arrNum, (size_t)iLen) ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/rootstat") ) {
			xvalue* pRoot = xsConfigRoot();
			int64 iValue = ConfigInt(pRoot, "reload_root");
			char arrNum[32];
			int iLen;

			xrtValueRelease(pRoot);
			iLen = snprintf(arrNum, sizeof(arrNum), "%lld", (long long)iValue);
			return Reply(pReq, 200, "text/plain; charset=utf-8", arrNum, (size_t)iLen) ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/topology") ) {
			XS_ServerInfo* pFound = xsServerFind("main");
			char arrNum[32];
			int iLen = snprintf(arrNum, sizeof(arrNum), "%u",
				pFound != NULL ? (unsigned)pFound->Port : 0u);

			xsServerRelease(pFound);
			return Reply(pReq, 200, "text/plain; charset=utf-8", arrNum, (size_t)iLen) ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/ready") ) {
			return ReplyLit(pReq, g_Ready == 1 ? 200 : 503, "text/plain; charset=utf-8",
				g_Ready == 1 ? "ready" : "initializing") ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/lease-hold") ) {
			char arrNum[32];
			int iLen;

			if ( g_HeldServer == NULL ) g_HeldServer = xsServerFind("tcp-echo");
			iLen = snprintf(arrNum, sizeof(arrNum), "%u",
				g_HeldServer != NULL ? (unsigned)g_HeldServer->Port : 0u);
			return Reply(pReq, g_HeldServer != NULL ? 200 : 404,
				"text/plain; charset=utf-8", arrNum, (size_t)iLen) ? XS_OK : XS_OK;
		}
		if ( PathIs(pReq, "/lease-release") ) {
			char arrNum[32];
			int iLen = snprintf(arrNum, sizeof(arrNum), "%u",
				g_HeldServer != NULL ? (unsigned)g_HeldServer->Port : 0u);

			xsServerRelease(g_HeldServer);
			g_HeldServer = NULL;
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
	int64 iDelay = ConfigInt(pHost->Custom, "init_delay_ms");

	if ( iDelay > 0 && iDelay <= 5000 ) xrtSleep((uint32)iDelay);

	if ( pSwap != NULL ) {
		(void)xrtValueGetInt(pSwap, &g_ReloadCount);
		xrtValueRelease(pSwap);
	}
	g_Ready = 1;
}

bool ServiceSwap(XS_HostInfo* pHost, xvalue** ppShared)
{
	(void)pHost;
	*ppShared = xrtValueInt(g_ReloadCount + 1);
	return *ppShared != NULL;
}

void ServiceUnit(XS_HostInfo* pHost)
{
	(void)pHost;
	xsServerRelease(g_HeldServer);
	g_HeldServer = NULL;
}
