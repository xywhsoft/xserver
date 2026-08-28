/*
 * xs3 tcp 驱动示例：连接生命周期由 xs 持有，脚本只处理数据。
 * 语义：接入发 banner；收到的数据原样回显；对端 EOF 时主动收尾。
 * 连接发送统一走 XS_TcpSend（兼容 tcp 与 tcps 两种形态）。
 */
#include <xsbase.h>

static bool XsConnSend(XS_StreamConn* pConn, const void* pData, size_t iSize)
{
	size_t iWritten = 0;

	if ( pConn->tls != NULL ) {
		return xrtTlsStreamSend(pConn->tls, pData, iSize, &iWritten) == XTLS_OK &&
		       iWritten == iSize;
	}
	return xrtNetStreamSend(pConn->tcp, pData, iSize) == XNET_RESULT_OK;
}

static void XsConnFinish(XS_StreamConn* pConn)
{
	if ( pConn->tls != NULL ) {
		(void)xrtTlsStreamClose(pConn->tls);
	} else {
		(void)xrtNetStreamClose(pConn->tcp);
	}
}

void EventOpen(XS_HostInfo* pHost, XS_StreamConn* pConn)
{
	str sBanner = xrtFormat("[xs3-tcp] host=%s ok\n", pHost->Name);

	if ( !XsConnSend(pConn, sBanner, xrtStrView(sBanner).Size) ) { /* 发送失败由 Close 流程收尾 */ }
	xrtFree(sBanner);
}

void EventData(XS_HostInfo* pHost, XS_StreamConn* pConn, xnetbuf* pBuffer)
{
	unsigned char arrChunk[4096];
	size_t iAvail;
	size_t iGot;

	(void)pHost;
	while ( (iAvail = xrtNetBufSize(pBuffer)) > 0 ) {
		iGot = iAvail > sizeof(arrChunk) ? sizeof(arrChunk) : iAvail;
		iGot = xrtNetBufPeek(pBuffer, 0, arrChunk, iGot);
		if ( iGot == 0 ) {
			return;
		}
		if ( !XsConnSend(pConn, arrChunk, iGot) ) {
			return;
		}
		if ( pConn->tls != NULL ) {
			(void)xrtTlsStreamConsume(pConn->tls, iGot);	/* tcps：消费走 TLS 流 */
		} else {
			xrtNetBufConsume(pBuffer, iGot);			/* tcp：直接消费接收链 */
		}
	}
}

void EventClose(XS_HostInfo* pHost, XS_StreamConn* pConn, xnetresult iResult, const xerror* pError)
{
	(void)pHost; (void)pConn; (void)iResult; (void)pError;
	/* Destroy 由 xs 驱动负责，脚本无需（也无法）释放连接 */
}
