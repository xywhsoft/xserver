/* WebSocket virtual-host fixture: a distinct greeting and message handler. */
#include <xsbase.h>

void WsOpen(XS_HostInfo* pHost, xwsstream* pWs)
{
	(void)pHost;
	(void)xrtWsStreamText(pWs, XRT_STR_LITERAL("[xs3-ws-admin] connected"));
}

void WsText(XS_HostInfo* pHost, xwsstream* pWs, xstrview tText)
{
	(void)pHost;
	(void)tText;
	(void)xrtWsStreamText(pWs, XRT_STR_LITERAL("ws-admin-script"));
}

void WsBinary(XS_HostInfo* pHost, xwsstream* pWs, xbytesview tData)
{
	(void)pHost;
	(void)xrtWsStreamSend(pWs, XWS_OPCODE_BINARY, tData);
}
