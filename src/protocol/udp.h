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
#include "../runtime/listener_slot.h"
#include "../script/script.h"

typedef struct XS_UdpRuntime {
	XS_ServerInfo*		pServer;
	XS_HostInfo*		pHost;		/* DefaultHost：回调挂载点 */
	xnetudp*		pUdp;
	XS_ListenerSlot*	pListenerSlot;
	XS_ServerGeneration*	pGeneration;
	XS_ScriptRuntime*	pScript;
	xatomic32		tStopping;
} XS_UdpRuntime;

static void XS_UdpOnReceive(xnetudp* pUdp, const xnetudpmessage* pMsg, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;
	XS_UdpRuntime* pRuntime = NULL;
	XS_ServerGeneration* pGeneration = NULL;
	XS_ScriptRuntime* pScript;

	if ( !XS_ListenerSlotAcquireConnection(pSlot, (void**)&pRuntime, &pGeneration) ) return;
	if ( xrtAtomic32Load(&pRuntime->tStopping, XMEMORY_ACQUIRE) != 0 ) {
		XS_GenerationConnectionRelease(pGeneration);
		return;
	}
	pScript = pRuntime->pScript;
	if ( pScript != NULL && pScript->procEventDgram != NULL ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		pScript->procEventDgram(pRuntime->pHost, pUdp, pMsg);
		XS_ScriptLeave(pPrevious);
	}
	XS_GenerationConnectionRelease(pGeneration);
}

static void XS_UdpOnClose(xnetudp* pUdp, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)pData;

	(void)iResult; (void)pError;
	xrtNetUdpDestroy(pUdp);		/* Close 完成后释放调用方引用 */
	XS_ListenerSlotResourceClose(pSlot);
}

static const xnetudpevents g_XS_UdpEvents = {
	NULL,			/* Open */
	XS_UdpOnReceive,
	NULL, NULL, NULL, NULL, NULL,
	XS_UdpOnClose
};

static bool XS_UdpStartEx(
	XS_ServerInfo* pServer,
	bool bStartEndpoint,
	bool bAcceptEndpoint,
	char* sErr,
	size_t iErrCap)
{
	XS_UdpRuntime* pRuntime = (XS_UdpRuntime*)xrtCalloc(1, sizeof(XS_UdpRuntime));
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pServer->DefaultHost->Runtime;
	xnetaddr tAddr;
	xnetudpconfig tCfg;

	if ( pRuntime == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	pRuntime->pServer = pServer;
	pRuntime->pHost = pServer->DefaultHost;
	pRuntime->pGeneration = (XS_ServerGeneration*)pServer->Generation;
	xrtAtomic32Init(&pRuntime->tStopping, 0);
	pServer->Runtime = pRuntime;
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
	pRuntime->pScript = XS_ScriptAcquireHost(pServer->DefaultHost);
	if ( pRuntime->pScript == NULL ) {
		snprintf(sErr, iErrCap, "udp server '%s' cannot acquire generation", pServer->Name);
		return false;
	}
	if ( bStartEndpoint ) {
		pRuntime->pListenerSlot = XS_ListenerSlotCreate(pRuntime, pRuntime->pGeneration,
			NULL, bAcceptEndpoint);
		if ( pRuntime->pListenerSlot == NULL ||
		     !XS_ListenerSlotResourceAdd(pRuntime->pListenerSlot) ) {
			snprintf(sErr, iErrCap, "udp server '%s' listener slot failed", pServer->Name);
			return false;
		}
		pRuntime->pUdp = xrtNetUdpBind(pServer->Engine, &tAddr, 0, &tCfg,
			&g_XS_UdpEvents, pRuntime->pListenerSlot);
		if ( pRuntime->pUdp == NULL ) {
			XS_ListenerSlotResourceCancel(pRuntime->pListenerSlot);
			XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
			pRuntime->pListenerSlot = NULL;
			snprintf(sErr, iErrCap, "udp server '%s' bind failed (port %u)", pServer->Name, pServer->Port);
			return false;
		}
	}
	printf("[xs] server '%s' udp %s on %s:%u\n", pServer->Name,
		bStartEndpoint ? (bAcceptEndpoint ? "ready" : "bound") : "prepared",
		pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port);
	return true;
}

static bool XS_UdpHandoff(XS_UdpRuntime* pOld, XS_UdpRuntime* pNew)
{
	XS_ListenerSlot* pSlot;

	if ( pOld == NULL || pNew == NULL || pOld->pListenerSlot == NULL ) return false;
	pSlot = pOld->pListenerSlot;
	if ( !XS_ListenerSlotHandoff(pSlot, pOld, pNew, pNew->pGeneration, NULL) ) return false;
	pNew->pListenerSlot = pSlot;
	pNew->pUdp = pOld->pUdp;
	pOld->pListenerSlot = NULL;
	pOld->pUdp = NULL;
	return true;
}

static void XS_UdpStop(XS_UdpRuntime* pRuntime)
{
	XS_ListenerSlot* pSlot;

	if ( pRuntime == NULL || pRuntime->pUdp == NULL ) {
		return;
	}
	xrtAtomic32Store(&pRuntime->tStopping, 1, XMEMORY_RELEASE);
	pSlot = pRuntime->pListenerSlot;
	if ( pSlot != NULL ) XS_ListenerSlotBeginClose(pSlot, pRuntime);
	xrtNetUdpClose(pRuntime->pUdp);		/* Close 回调里 Destroy */
	pRuntime->pUdp = NULL;
	pRuntime->pListenerSlot = NULL;
}

static void XS_UdpUnit(XS_UdpRuntime* pRuntime)
{
	if ( pRuntime == NULL ) return;
	XS_ScriptRelease(pRuntime->pScript);
	if ( pRuntime->pListenerSlot != NULL ) {
		XS_ListenerSlotDestroyEmpty(pRuntime->pListenerSlot);
		pRuntime->pListenerSlot = NULL;
	}
	xrtFree(pRuntime);
}

#endif
