#ifndef XS_PROTOCOL_STREAM_H
#define XS_PROTOCOL_STREAM_H

/*
 * xs3 TCP / TCP+TLS 驱动（设计 §6.3）
 * - xs 建监听（含 tcps：host 证书 → SNI 身份表），流事件透传脚本回调
 * - 连接生命周期由 xs 持有（Accept 接管 → 注册表 → Close 时 Destroy）
 * - Custom 旋钮：idle_timeout（毫秒，server Custom，缺省关闭）——超时连接
 *   graceful Close（复用注册表扫描；正式按代 drain 随重载工作包）
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"
#include "../core/config.h"
#include "../core/engine.h"
#include "../core/tls.h"
#include "../runtime/registry.h"
#include "../script/script.h"

typedef struct XS_TcpRuntime {
	XS_ServerInfo*		pServer;
	XS_HostInfo*		pHost;		/* DefaultHost：回调挂载点 */
	bool			bTls;
	xnetlistener*		pListener;	/* 明文模式 */
	xtlslistener*		pTlsListener;	/* tcps 模式 */
	XS_TlsTable		tTls;
	XS_ConnRegistry	tRegistry;
	uint64			iIdleMs;	/* 0 = 关闭 idle 保护 */
	uint64			iSweepTimer;
	volatile bool		bStopping;
} XS_TcpRuntime;

/* Custom 旋钮只读（不弹出，键仍留给应用读取） */
static bool XS_CustomGetInt(xvalue* pCustom, const char* sKey, int64* pOut)
{
	xvalue* pVal = (pCustom != NULL) ? xrtValueObjectGet(pCustom, xrtStrViewN(sKey, strlen(sKey))) : NULL;

	*pOut = 0;
	return pVal != NULL && xrtValueGetInt(pVal, pOut);
}

static void XS_TcpConnView(XS_ConnRecord* pRecord, XS_StreamConn* pConn)
{
	pConn->tcp = pRecord->pTcp;
	pConn->tls = pRecord->pTls;
}

/* —— 明文流事件 shim —— */

static void XS_TcpOnOpen(xnetstream* pStream, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pRecord->pHost->Runtime;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventOpen != NULL ) {
		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventOpen(pRecord->pHost, &tConn);
	}
	(void)pStream;
}

static void XS_TcpOnRead(xnetstream* pStream, xnetbuf* pBuffer, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pRecord->pHost->Runtime;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventData != NULL ) {
		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventData(pRecord->pHost, &tConn, pBuffer);
	}
	(void)pStream;
}

static void XS_TcpOnClose(xnetstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pRecord->pHost->Runtime;

	if ( pScript != NULL && pScript->procEventClose != NULL ) {
		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventClose(pRecord->pHost, &tConn, iResult, pError);
	}
	xrtNetStreamDestroy(pStream);
	XS_RegistryRemove(&((XS_TcpRuntime*)pRecord->pHost->Server->Runtime)->tRegistry, pRecord);
	xrtFree(pRecord);
}

static const xnetstreamevents g_XS_TcpStreamEvents = {
	NULL,			/* Open 由 Accept 侧触发语义替代：见下，Accept 后手动补发 */
	XS_TcpOnRead,
	NULL,			/* End */
	NULL, NULL, NULL,	/* HighWater / LowWater / Drain */
	XS_TcpOnClose
};

static bool XS_TcpOnAccept(xnetlistener* pListener, xnetstream* pStream, ptr pData)
{
	XS_TcpRuntime* pRuntime = (XS_TcpRuntime*)pData;
	XS_ConnRecord* pRecord = (XS_ConnRecord*)xrtCalloc(1, sizeof(XS_ConnRecord));

	(void)pListener;
	if ( pRecord == NULL ) {
		return false;		/* 拒绝接入（库立即关闭该流） */
	}
	pRecord->pHost = pRuntime->pHost;
	pRecord->pTcp = pStream;
	XS_RegistryAdd(&pRuntime->tRegistry, pRecord);
	(void)xrtNetStreamSetData(pStream, pRecord);	/* Accept 运行于目标流 Worker，合法 */
	XS_TcpOnOpen(pStream, (ptr)pRecord);		/* 补发 Open（事件表 Open 位留空防双发） */
	return true;
}

static void XS_TcpOnListenerClose(xnetlistener* pListener, ptr pData)
{
	(void)pData;
	xrtNetListenerDestroy(pListener);
}

static const xnetlistenerevents g_XS_TcpListenerEvents = {
	XS_TcpOnAccept,
	NULL,			/* Error */
	XS_TcpOnListenerClose
};

/* —— tcps 流事件 shim（buffer 为 const 视图，消费走 xrtTlsStreamConsume）—— */

static void XS_TlsOnOpen(xtlsstream* pStream, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pRecord->pHost->Runtime;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventOpen != NULL ) {
		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventOpen(pRecord->pHost, &tConn);
	}
	(void)pStream;
}

static void XS_TlsOnRead(xtlsstream* pStream, const xnetbuf* pBuffer, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pRecord->pHost->Runtime;

	XS_RegistryTouch(pRecord);
	if ( pScript != NULL && pScript->procEventData != NULL ) {
		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventData(pRecord->pHost, &tConn, (xnetbuf*)pBuffer);	/* 仅 Peek；tcps 消费走 xrtTlsStreamConsume */
	}
	(void)pStream;
}

static void XS_TlsOnClose(xtlsstream* pStream, xnetresult iResult, const xerror* pError, ptr pData)
{
	XS_ConnRecord* pRecord = (XS_ConnRecord*)pData;
	XS_StreamConn tConn;
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pRecord->pHost->Runtime;

	if ( pScript != NULL && pScript->procEventClose != NULL ) {
		XS_TcpConnView(pRecord, &tConn);
		pScript->procEventClose(pRecord->pHost, &tConn, iResult, pError);
	}
	xrtTlsStreamDestroy(pStream);
	XS_RegistryRemove(&((XS_TcpRuntime*)pRecord->pHost->Server->Runtime)->tRegistry, pRecord);
	xrtFree(pRecord);
}

static const xtlsstreamevents g_XS_TlsStreamEvents = {
	XS_TlsOnOpen,
	XS_TlsOnRead,
	NULL, NULL, NULL,
	XS_TlsOnClose,
	NULL			/* Ticket */
};

static bool XS_TlsOnAccept(xtlslistener* pListener, xtlsstream* pStream, ptr pData)
{
	XS_TcpRuntime* pRuntime = (XS_TcpRuntime*)pData;
	XS_ConnRecord* pRecord = (XS_ConnRecord*)xrtCalloc(1, sizeof(XS_ConnRecord));

	(void)pListener;
	if ( pRecord == NULL ) {
		return false;
	}
	pRecord->pHost = pRuntime->pHost;
	pRecord->pTls = pStream;
	XS_RegistryAdd(&pRuntime->tRegistry, pRecord);
	(void)xrtTlsStreamSetEvents(pStream, &g_XS_TlsStreamEvents, pRecord);	/* Accept 在目标流 Worker，合法 */
	return true;
}

static void XS_TlsOnListenerClose(xtlslistener* pListener, ptr pData)
{
	(void)pData;
	xrtTlsListenerDestroy(pListener);
}

static const xtlslistenerevents g_XS_TlsListenerEvents = {
	XS_TlsOnAccept,
	NULL,			/* HandshakeError：握手失败由库内部关闭 */
	NULL,			/* Error */
	XS_TlsOnListenerClose
};

/* —— idle 扫描（引擎定时器，周期 = min(idle/2, 1s)）—— */

static void XS_TcpSweepProc(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_TcpRuntime* pRuntime = (XS_TcpRuntime*)pData;

	(void)pWorker; (void)iId;
	if ( iResult != XNET_RESULT_OK || pRuntime->bStopping ) {
		return;
	}
	(void)XS_RegistrySweepIdle(&pRuntime->tRegistry, pRuntime->iIdleMs);
	{
		uint64 iInterval = pRuntime->iIdleMs / 2;

		if ( iInterval > 1000 ) {
			iInterval = 1000;
		}
		if ( iInterval < 10 ) {
			iInterval = 10;
		}
		pRuntime->iSweepTimer = xrtNetEngineAfter(pRuntime->pServer->Engine, 0,
			iInterval * 1000, XS_TcpSweepProc, pData);
	}
}

/* —— 启动 / 停止 —— */

static bool XS_TcpStart(XS_ServerInfo* pServer, char* sErr, size_t iErrCap)
{
	XS_TcpRuntime* pRuntime = (XS_TcpRuntime*)xrtCalloc(1, sizeof(XS_TcpRuntime));
	XS_ScriptRuntime* pScript = (XS_ScriptRuntime*)pServer->DefaultHost->Runtime;
	xnetaddr tAddr;
	int64 iIdle = 0;

	if ( pRuntime == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	pRuntime->pServer = pServer;
	pRuntime->pHost = pServer->DefaultHost;
	pServer->Runtime = pRuntime;

	if ( pScript == NULL || pScript->procEventData == NULL ) {
		snprintf(sErr, iErrCap, "tcp server '%s' requires script exporting EventData", pServer->Name);
		return false;
	}
	if ( !xrtNetAddrParse(&tAddr, pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port) ) {
		snprintf(sErr, iErrCap, "tcp server '%s' addr parse failed", pServer->Name);
		return false;
	}
	if ( !XS_RegistryInit(&pRuntime->tRegistry) ) {
		snprintf(sErr, iErrCap, "registry init failed");
		return false;
	}
	(void)XS_CustomGetInt(pServer->Custom, "idle_timeout", &iIdle);
	pRuntime->iIdleMs = (iIdle > 0) ? (uint64)iIdle : 0;

	if ( !pServer->TLS ) {
		xnetlistenconfig tListen;

		xrtNetListenConfigInit(&tListen);
		tListen.Address = tAddr;
		tListen.ReuseAddress = true;	/* Windows 默认独占，快速重绑 */
		tListen.ExclusiveAddress = false;	/* 与 ReuseAddress 互斥（配置校验） */
		if ( pServer->Backlog > 0 ) {
			tListen.Backlog = (int)pServer->Backlog;
		}
		if ( pServer->RecvLimit > 0 ) {
			tListen.Stream.ReadLimit = pServer->RecvLimit;
		}
		pRuntime->pListener = xrtNetListen(pServer->Engine, &tListen,
			&g_XS_TcpListenerEvents, &g_XS_TcpStreamEvents, pRuntime);
		if ( pRuntime->pListener == NULL ) {
			const xerror* pErr = xrtGetError();
			snprintf(sErr, iErrCap, "tcp server '%s' listen failed (port %u): %s", pServer->Name,
				pServer->Port, pErr != NULL ? xrtErrorMessage(pErr) : "unknown");
			return false;
		}
	} else {
		xtlslistenerconfig tTlsListen;
		xtlscontext* pContext = XS_TlsSharedContext();

		if ( pContext == NULL ) {
			snprintf(sErr, iErrCap, "tls context create failed");
			return false;
		}
		if ( !XS_TlsTableBuild(pServer, &pRuntime->tTls, sErr, iErrCap) ) {
			return false;
		}
		if ( pRuntime->tTls.iCount == 0 ) {
			snprintf(sErr, iErrCap, "tcps server '%s' has no tls_cert/tls_key host", pServer->Name);
			return false;
		}
		xrtNetListenConfigInit(&tTlsListen.Listen);
		tTlsListen.Listen.Address = tAddr;
		tTlsListen.Listen.ReuseAddress = true;
		tTlsListen.Listen.ExclusiveAddress = false;
		if ( pServer->Backlog > 0 ) {
			tTlsListen.Listen.Backlog = (int)pServer->Backlog;
		}
		if ( pServer->RecvLimit > 0 ) {
			tTlsListen.Listen.Stream.ReadLimit = pServer->RecvLimit;
		}
		xrtTlsServerConfigInit(&tTlsListen.Tls);
		tTlsListen.Tls.Context = pContext;
		tTlsListen.Tls.Identity = pRuntime->tTls.pEntries[0].pIdentity;	/* 无 SNI 回落 */
		tTlsListen.Tls.Select = XS_TlsSelect;
		tTlsListen.Tls.SelectContext = &pRuntime->tTls;
		pRuntime->bTls = true;
		pRuntime->pTlsListener = xrtTlsListenerStart(pServer->Engine, &tTlsListen,
			&g_XS_TlsListenerEvents, &g_XS_TlsStreamEvents, pRuntime);
		if ( pRuntime->pTlsListener == NULL ) {
			XS_TlsTableUnit(&pRuntime->tTls);
			snprintf(sErr, iErrCap, "tcps server '%s' listen failed (port %u)", pServer->Name, pServer->Port);
			return false;
		}
	}

	if ( pRuntime->iIdleMs > 0 ) {
		XS_TcpSweepProc(NULL, 0, XNET_RESULT_OK, pRuntime);	/* 立即首拍并自续 */
	}
	printf("[xs] server '%s' %s ready on %s:%u%s\n", pServer->Name,
		pRuntime->bTls ? "tcps" : "tcp",
		pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port,
		pRuntime->iIdleMs > 0 ? " (idle protected)" : "");
	return true;
}

static void XS_TcpStop(XS_TcpRuntime* pRuntime)
{
	if ( pRuntime == NULL ) {
		return;
	}
	pRuntime->bStopping = true;
	if ( pRuntime->iSweepTimer != 0 ) {
		(void)xrtNetEngineTimerCancel(pRuntime->pServer->Engine, pRuntime->iSweepTimer);
		pRuntime->iSweepTimer = 0;
	}
	XS_RegistryCloseAll(&pRuntime->tRegistry);
	if ( pRuntime->pListener != NULL ) {
		xrtNetListenerClose(pRuntime->pListener);	/* Close 回调里 Destroy */
		pRuntime->pListener = NULL;
	}
	if ( pRuntime->pTlsListener != NULL ) {
		xrtTlsListenerClose(pRuntime->pTlsListener);
		pRuntime->pTlsListener = NULL;
	}
}

static void XS_TcpUnit(XS_TcpRuntime* pRuntime)
{
	if ( pRuntime == NULL ) {
		return;
	}
	XS_RegistryUnit(&pRuntime->tRegistry);
	XS_TlsTableUnit(&pRuntime->tTls);
	xrtFree(pRuntime);
}

#endif
