/*
 * xs3 ws 驱动示例：文本回显 + 二进制回显（Ping 由内核 AutoPong 应答）。
 */
#include <xsbase.h>

void WsOpen(XS_HostInfo* pHost, xwsstream* pWs)
{
	(void)pHost;
	(void)xrtWsStreamText(pWs, XRT_STR_LITERAL("[xs3-ws] connected"));
}

void WsText(XS_HostInfo* pHost, xwsstream* pWs, xstrview tText)
{
	(void)pHost;
	(void)xrtWsStreamText(pWs, tText);
}

void WsBinary(XS_HostInfo* pHost, xwsstream* pWs, xbytesview tData)
{
	(void)pHost;
	(void)xrtWsStreamSend(pWs, XWS_OPCODE_BINARY, tData);
}

void WsClose(XS_HostInfo* pHost, xwsstream* pWs, uint16 iCode, xstrview tReason)
{
	(void)pHost; (void)pWs; (void)iCode; (void)tReason;
}
