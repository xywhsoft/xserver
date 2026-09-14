/*
 * demo —— XServer 极简入门示例（单文件）
 *
 * 完整契约只用了六个函数，宿主按名字查找，缺省的回调会被自动跳过：
 *
 *   ServiceInit / ServiceUnit   生命周期
 *   RequestProc                 http 服务的全部请求入口
 *   WsOpen / WsText / WsClose   ws 服务的连接与消息事件
 *
 * 本文件被 xs.json 里的两个服务（http:9081 / ws:9082）共用，
 * 各服务只会调用自己关心的那组回调。
 *
 * 运行：
 *   Linux/Mac :  xs release/demo/xs.json
 *   Windows  :  xs.exe release\demo\xs.json
 */



#include <xsbase.h>
#include <stdio.h>
#include <string.h>



/* ============================================================
 * HTTP 应答小助手（节选自 demo-single 工程的真实实现）
 * ============================================================ */

static bool ConnSend(XS_HttpReq* pReq, const void* pData, size_t iSize)
{
	size_t iWritten = 0;

	if ( pReq == NULL || pData == NULL ) {
		return false;
	}
	if ( pReq->tls != NULL ) {
		return xrtTlsStreamSend(pReq->tls, pData, iSize, &iWritten) == XTLS_OK &&
		       iWritten == iSize;
	}
	return pReq->tcp != NULL &&
		xrtNetStreamSend(pReq->tcp, pData, iSize) == XNET_RESULT_OK;
}



static bool ReplyRaw(
	XS_HttpReq* pReq, uint16 iStatus, const char* sContentType,
	const void* pBody, size_t iBodySize
)
{
	char arrHead[1024];
	char arrLength[32];
	xhttpfield arrField[2];
	xstrview tReason;
	size_t iHeadSize = 0;

	snprintf(arrLength, sizeof(arrLength), "%llu",
		(unsigned long long)iBodySize);
	arrField[0].Name = XRT_STR_LITERAL("Content-Length");
	arrField[0].Value = xrtStrView(arrLength);
	arrField[1].Name = XRT_STR_LITERAL("Content-Type");
	arrField[1].Value = xrtStrView(sContentType);

	tReason = xrtHttpStatusText(iStatus);
	if ( !xrtHttp1ResponseWrite(XHTTP_VERSION_1_1, iStatus, tReason,
		arrField, 2, arrHead, sizeof(arrHead), &iHeadSize) ) {
		return false;
	}
	if ( !ConnSend(pReq, arrHead, iHeadSize) ) {
		return false;
	}
	if ( iBodySize != 0 && pReq->head->MethodCode != XHTTP_METHOD_HEAD ) {
		return ConnSend(pReq, pBody, iBodySize);
	}
	return true;
}



static bool ReplyText(XS_HttpReq* pReq, uint16 iStatus, const char* sText)
{
	if ( sText == NULL ) sText = "";
	return ReplyRaw(pReq, iStatus, "text/plain; charset=utf-8",
		sText, strlen(sText));
}



static bool ReplyJSON(XS_HttpReq* pReq, uint16 iStatus, const xvalue* pObject)
{
	str sJSON;
	size_t iJSONSize = 0;
	bool bResult;

	if ( pObject == NULL ) {
		return false;
	}
	sJSON = xrtJsonStringify(pObject, false, &iJSONSize);
	if ( sJSON == NULL ) {
		return false;
	}
	bResult = ReplyRaw(pReq, iStatus, "application/json; charset=utf-8",
		sJSON, iJSONSize);
	xrtFree(sJSON);
	return bResult;
}



/* xstrview 不是 NUL 结尾的字符串，用字节长度比较。 */
static bool PathIs(xstrview tPath, const char* sWanted)
{
	size_t iWanted = strlen(sWanted);

	return tPath.Size == iWanted &&
	       memcmp(tPath.Data, sWanted, iWanted) == 0;
}



/* ============================================================
 * 生命周期
 * ============================================================ */

void ServiceInit(XS_HostInfo* pHost)
{
	printf("[demo] init host '%s'\n",
		((pHost && pHost->Name) ? pHost->Name : "?"));
	(void)pHost;
}



void ServiceUnit(XS_HostInfo* pHost)
{
	printf("[demo] unit host '%s'\n",
		((pHost && pHost->Name) ? pHost->Name : "?"));
	(void)pHost;
}



/* ============================================================
 * http:9081 —— 未命中的 URI 返回 XS_FALLBACK 交给 wwwroot 静态层
 * ============================================================ */

XS_RequestResult RequestProc(XS_HttpReq* pReq)
{
	xhttptarget tTarget;
	xvalue* pObject;

	if ( pReq == NULL || pReq->head == NULL ) {
		return XS_FALLBACK;
	}
	if ( !xrtHttpTargetParse(pReq->head->Method, pReq->head->Target, &tTarget) ) {
		return XS_FALLBACK;
	}
	if ( pReq->head->MethodCode != XHTTP_METHOD_GET &&
	     pReq->head->MethodCode != XHTTP_METHOD_HEAD ) {
		return (ReplyText(pReq, 405, "method not allowed") ? XS_OK : XS_FALLBACK);
	}

	/* GET /api/hello —— JSON 应答 */
	if ( PathIs(tTarget.Path, "/api/hello") ) {
		pObject = xrtValueObject();
		if ( pObject != NULL ) {
			xrtValueObjectSetNew(pObject, XRT_STR_LITERAL("hello"),
				xrtValueString(XRT_STR_LITERAL("XServer")));
			xrtValueObjectSetNew(pObject, XRT_STR_LITERAL("host"),
				xrtValueString(xrtStrView(
					((pReq->host && pReq->host->Name) ? pReq->host->Name : "?"))));
			xrtValueObjectSetNew(pObject, XRT_STR_LITERAL("ok"),
				xrtValueBool(true));
			(void)ReplyJSON(pReq, 200, pObject);
			xrtValueRelease(pObject);
			return XS_OK;
		}
		return (ReplyText(pReq, 500, "out of memory") ? XS_OK : XS_FALLBACK);
	}

	/* GET /api/echo?text=... —— 查询串原样回显 */
	if ( PathIs(tTarget.Path, "/api/echo") ) {
		char arrBody[512];
		size_t iSize = tTarget.Query.Size;

		if ( iSize > sizeof(arrBody) - 1 ) {
			iSize = sizeof(arrBody) - 1;
		}
		memcpy(arrBody, tTarget.Query.Data, iSize);
		arrBody[iSize] = '\0';
		(void)ReplyText(pReq, 200, arrBody);
		return XS_OK;
	}

	/* 其余全部交给 wwwroot 静态文件（含 / 打开的 index.html）。 */
	return XS_FALLBACK;
}



/* ============================================================
 * ws:9082 —— 握手由宿主完成，脚本只处理消息
 * ============================================================ */

void WsOpen(XS_HostInfo* pHost, xwsstream* pWs)
{
	(void)pHost; (void)pWs;
	printf("[demo] ws open\n");
}



void WsText(XS_HostInfo* pHost, xwsstream* pWs, xstrview tText)
{
	(void)pHost;
	/* 回显：把收到的文本原样发回客户端。 */
	(void)xrtWsStreamText(pWs, tText);
}



void WsClose(XS_HostInfo* pHost, xwsstream* pWs, uint16 iCode, xstrview tReason)
{
	(void)pHost; (void)pWs; (void)tReason;
	printf("[demo] ws close %u\n", (unsigned)iCode);
}
