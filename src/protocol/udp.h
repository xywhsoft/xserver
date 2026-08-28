#ifndef XS_PROTOCOL_UDP_H
#define XS_PROTOCOL_UDP_H

/*
 * xs3 UDP 驱动（设计 §6.4）
 * xs 绑定 socket，数据报事件透传脚本 EventDgram；发送由脚本经
 * xrtNetUdpSendTo 自行处理（对端地址取自 xnetudpmessage.Remote）。
 */

#include <stdio.h>

#include "../sdk/xsbase.h"
#include "../core/config.h"
#include "../core/engine.h"
#include "../script/script.h"

typedef struct XS_UdpRuntime {
	XS_ServerInfo*		pServer;
	XS_HostInfo*		pHost;		/* DefaultHost：回调挂载点 */
	xnetudp*		pUdp;
} XS_UdpRuntime;

static void XS_UdpOnReceive(xnetudp* pUdp, const xnetudpmessage* pMsg, ptr pData)
{
	XS_UdpRuntime* pRuntime = (XS_UdpRuntime*)pData;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pRuntime->pHost->Runtime;

	if ( pScript != NULL && pScript->procEventDgram != NULL ) {
		pScript->procEventDgram(pRuntime->pHost, pUdp, pMsg);
	}
}

static void XS_UdpOnClose(xnetudp* pUdp, xnetresult iResult, const xerror* pError, ptr pData)
{
	(void)iResult; (void)pError; (void)pData;
	xrtNetUdpDestroy(pUdp);		/* Close 完成后释放调用方引用 */
}

static const xnetudpevents g_XS_UdpEvents = {
	NULL,			/* Open */
	XS_UdpOnReceive,
	NULL, NULL, NULL, NULL, NULL,
	XS_UdpOnClose
};

static bool XS_UdpStart(XS_ServerInfo* pServer, char* sErr, size_t iErrCap)
{
	XS_UdpRuntime* pRuntime = (XS_UdpRuntime*)xrtCalloc(1, sizeof(XS_UdpRuntime));
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pServer->DefaultHost->Runtime;
	xnetaddr tAddr;
	xnetudpconfig tCfg;

	if ( pRuntime == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	if ( pScript == NULL || pScript->procEventDgram == NULL ) {
		snprintf(sErr, iErrCap, "udp server '%s' requires script exporting EventDgram", pServer->Name);
		return false;
	}
	if ( !xrtNetAddrParse(&tAddr, pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port) ) {
		snprintf(sErr, iErrCap, "udp server '%s' addr parse failed", pServer->Name);
		return false;
	}
	xrtNetUdpConfigInit(&tCfg);
	if ( pServer->RecvLimit > 0 ) {
		tCfg.ReceiveSize = pServer->RecvLimit;
	}
	pRuntime->pServer = pServer;
	pRuntime->pHost = pServer->DefaultHost;
	pServer->Runtime = pRuntime;
	pRuntime->pUdp = xrtNetUdpBind(pServer->Engine, &tAddr, 0, &tCfg, &g_XS_UdpEvents, pRuntime);
	if ( pRuntime->pUdp == NULL ) {
		snprintf(sErr, iErrCap, "udp server '%s' bind failed (port %u)", pServer->Name, pServer->Port);
		return false;
	}
	printf("[xs] server '%s' udp ready on %s:%u\n", pServer->Name,
		pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port);
	return true;
}

static void XS_UdpStop(XS_UdpRuntime* pRuntime)
{
	if ( pRuntime == NULL || pRuntime->pUdp == NULL ) {
		return;
	}
	xrtNetUdpClose(pRuntime->pUdp);		/* Close 回调里 Destroy */
	pRuntime->pUdp = NULL;
}

static void XS_UdpUnit(XS_UdpRuntime* pRuntime)
{
	xrtFree(pRuntime);
}

#endif
