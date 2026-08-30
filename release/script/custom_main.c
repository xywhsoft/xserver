/*
 * xs3 custom 驱动示例：TCC 运行环境闭环验证
 * xs 不为本脚本做任何装配 —— 引擎、监听、协议全部手动（设计 §6.3）。
 * 自证清单（全部走宿主导入的符号）：
 *   xrtFormat/xrtFree(STRING)  xrtNow(TIME)  xrtPathJoin/xrtPathIsAbs/xrtFileExists(FS/PATH)
 *   xrtJsonParse/xrtValueGetInt/xrtValueRelease(JSON/VALUE)  xsAppPath(xs API)
 *   xrtNetListen/xrtNetStreamSend/xrtNetBuf*(NET) —— echo 回路
 */
#include <xsbase.h>

static xnetlistener* g_Listener = NULL;
static str g_Banner = NULL;
static size_t g_BannerSize = 0;

static void EchoOnOpen(xnetstream* pStream, ptr pData)
{
	(void)pData;
	if ( g_Banner != NULL ) {
		(void)xrtNetStreamSend(pStream, g_Banner, g_BannerSize);
	}
}

static void EchoOnRead(xnetstream* pStream, xnetbuf* pBuffer, ptr pData)
{
	unsigned char arrChunk[4096];
	size_t iAvail;
	size_t iGot;

	(void)pData;
	while ( (iAvail = xrtNetBufSize(pBuffer)) > 0 ) {
		iGot = iAvail > sizeof(arrChunk) ? sizeof(arrChunk) : iAvail;
		iGot = xrtNetBufPeek(pBuffer, 0, arrChunk, iGot);
		if ( iGot == 0 ) {
			return;
		}
		if ( xrtNetStreamSend(pStream, arrChunk, iGot) != XNET_RESULT_OK ) {
			return;
		}
		xrtNetBufConsume(pBuffer, iGot);
	}
}

static void EchoOnEnd(xnetstream* pStream, ptr pData)
{
	(void)pData;
	/* 对端关闭读端：echo 语义下主动收尾，流进入 Close 流程（Close 回调里 Destroy） */
	xrtNetStreamClose(pStream);
}

static void EchoOnClose(xnetstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	(void)iResult; (void)pError; (void)pData;
	xrtNetStreamDestroy(pStream);
}

static const xnetstreamevents g_EchoStreamEvents = {
	EchoOnOpen,		/* Open */
	EchoOnRead,		/* Read */
	EchoOnEnd,		/* End */
	NULL,			/* HighWater */
	NULL,			/* LowWater */
	NULL,			/* Drain */
	EchoOnClose		/* Close */
};

static bool EchoOnAccept(xnetlistener* pListener, xnetstream* pStream, ptr pData)
{
	(void)pListener; (void)pStream; (void)pData;
	return true;		/* 接管（流事件在 xrtNetListen 时绑定） */
}

static void EchoOnListenerClose(xnetlistener* pListener, ptr pData)
{
	(void)pData;
	xrtNetListenerDestroy(pListener);
	g_Listener = NULL;
}

static const xnetlistenerevents g_EchoListenerEvents = {
	EchoOnAccept,
	NULL,			/* Error */
	EchoOnListenerClose
};

static str BuildBanner(XS_HostInfo* pHost)
{
	xvalue* pRoot;
	int64 iCode = 0;
	str sSelf;
	str sPart1;
	str sPart2;
	str sBanner;

	/* JSON/VALUE 组 */
	pRoot = xrtJsonParse(XRT_STR_LITERAL("{\"code\":7,\"ok\":true}"));
	if ( pRoot != NULL ) {
		xvalue* pCode = xrtValueObjectGet(pRoot, XRT_STR_LITERAL("code"));
		if ( pCode != NULL ) {
			(void)xrtValueGetInt(pCode, &iCode);
		}
		xrtValueRelease(pRoot);
	}
	/* PATH/FS 组 + xs API 组 */
	sSelf = xrtPathJoin(xsAppPath(), "xs.json");
	sPart1 = xrtFormat("[xs3-tcc] host=%s json-code=%lld self-check=%s",
		pHost->Name, (long long)iCode,
		(sSelf != NULL && xrtFileExists(sSelf)) ? "ok" : "no-config");
	xrtFree(sSelf);
	/* TIME 组 */
	sPart2 = xrtFormat(" now=%lld\n", (long long)xrtNow());
	sBanner = xrtFormat("%s%s", sPart1, sPart2);
	xrtFree(sPart1);
	xrtFree(sPart2);
	return sBanner;
}

static uint16 EchoPort(XS_HostInfo* pHost)
{
	int64 iPort = 9099;
	xvalue* pValue = pHost != NULL && pHost->Server != NULL &&
		pHost->Server->Custom != NULL ?
		xrtValueObjectGet(pHost->Server->Custom, XRT_STR_LITERAL("port")) : NULL;

	if ( pValue != NULL ) {
		int64 iConfigured;

		if ( xrtValueGetInt(pValue, &iConfigured) &&
		     iConfigured > 0 && iConfigured <= 65535 ) {
			iPort = iConfigured;
		}
	}
	return (uint16)iPort;
}

void ServiceInit(XS_HostInfo* pHost)
{
	xnetlistenconfig tListen;
	xnetaddr tAddr;

	g_Banner = BuildBanner(pHost);
	g_BannerSize = g_Banner != NULL ? xrtStrView(g_Banner).Size : 0;

	if ( !xrtNetAddrParse(&tAddr, "0.0.0.0", EchoPort(pHost)) ) {
		return;
	}
	xrtNetListenConfigInit(&tListen);
	tListen.Address = tAddr;
	tListen.Backlog = 64;
	g_Listener = xrtNetListen(pHost->Server->Engine, &tListen,
		&g_EchoListenerEvents, &g_EchoStreamEvents, NULL);
}

void ServiceUnit(XS_HostInfo* pHost)
{
	(void)pHost;
	xrtFree(g_Banner);
	g_Banner = NULL;
	g_BannerSize = 0;
	if ( g_Listener != NULL ) {
		xrtNetListenerClose(g_Listener);	/* Close 回调里 Destroy 并清 g_Listener */
		g_Listener = NULL;
	}
}
