/*
 * xs3 udp 驱动示例：数据报回显。
 * 发送用 xrtNetUdpSendTo，对端地址取自报文 Remote 字段。
 */
#include <xsbase.h>

void EventDgram(XS_HostInfo* pHost, xnetudp* pUdp, const xnetudpmessage* pMsg)
{
	(void)pHost;
	if ( pMsg->Data != NULL && pMsg->Size > 0 ) {
		(void)xrtNetUdpSendTo(pUdp, &pMsg->Remote, pMsg->Data, pMsg->Size);
	}
}
