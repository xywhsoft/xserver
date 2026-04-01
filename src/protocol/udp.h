#ifndef XS_PROTOCOL_UDP_H
#define XS_PROTOCOL_UDP_H

typedef struct {
	xdgramsock* pSock;
	XS_ServerConfig* pServer;
} XS_UdpHandle;

static inline int64 XS_UdpMetricGet(const volatile int64* pValue)
{
	if ( !XS_RuntimeStatsEnabled() ) {
		return 0;
	}
	return XS_HttpMetricGet(pValue);
}

static inline int64 XS_UdpMetricAdd(volatile int64* pValue, int64 iValue)
{
	if ( !XS_RuntimeStatsEnabled() ) {
		return 0;
	}
	return XS_HttpMetricAdd(pValue, iValue);
}

static void XS_UdpOnRecv(ptr pOwner, xdgramsock* pSock, const xnetaddr* pFrom, xnetchain* pChain)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	size_t iLen;
	char* pBuf;
	const char* sFrom;
	bool bHandled = FALSE;
	
	if ( pChain == NULL ) {
		return;
	}
	
	iLen = xrtNetChainBytes(pChain);
	if ( iLen == 0 ) {
		return;
	}
	
	pBuf = (char*)xrtMalloc(iLen);
	if ( pBuf == NULL ) {
		xrtNetChainClear(pChain);
		return;
	}
	
	(void)xrtNetChainPeek(pChain, pBuf, iLen);
	xrtNetChainConsume(pChain, iLen);
	sFrom = pFrom ? xrtNetAddrToStr(pFrom) : NULL;
	if ( XS_RuntimeStatsEnabled() ) {
		XS_UdpMetricAdd(&g_iXsUdpRecvCount, 1);
		XS_UdpMetricAdd(&g_iXsUdpRecvBytes, (int64)iLen);
		g_iXsUdpLastBytes = (int64)iLen;
		g_tXsUdpLastTime = xrtNow();
		if ( sFrom ) {
			strncpy(g_sXsUdpLastFrom, sFrom, sizeof(g_sXsUdpLastFrom) - 1);
			g_sXsUdpLastFrom[sizeof(g_sXsUdpLastFrom) - 1] = '\0';
		} else {
			g_sXsUdpLastFrom[0] = '\0';
		}
		if ( iLen > 0 ) {
			size_t iCopy = iLen;
			if ( iCopy >= sizeof(g_sXsUdpLastText) ) {
				iCopy = sizeof(g_sXsUdpLastText) - 1;
			}
			memcpy(g_sXsUdpLastText, pBuf, iCopy);
			g_sXsUdpLastText[iCopy] = '\0';
		} else {
			g_sXsUdpLastText[0] = '\0';
		}
	}
	
	XS_LogInfo(
		"udp recv: server=%s from=%s bytes=%u script_dgram=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		sFrom ? sFrom : "(null)",
		(unsigned)iLen,
		(objServer && objServer->procDgramRecv) ? "true" : "false"
	);
	
	if ( objServer && objServer->procDgramRecv ) {
		bHandled = objServer->procDgramRecv(objServer, pSock, pFrom, pBuf, iLen);
	}

	if ( !bHandled && pFrom ) {
		(void)xrtNetDgramSendTo(pSock, pFrom, pBuf, iLen);
		XS_UdpMetricAdd(&g_iXsUdpSendCount, 1);
		XS_UdpMetricAdd(&g_iXsUdpSendBytes, (int64)iLen);
	}
	
	xrtFree(pBuf);
}

static void XS_UdpOnError(ptr pOwner, xdgramsock* pSock, int iSysErr)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	(void)pSock;
	
	if ( XS_RuntimeStatsEnabled() ) {
		XS_UdpMetricAdd(&g_iXsUdpErrorCount, 1);
		g_iXsUdpLastErrorCode = (int64)iSysErr;
		g_tXsUdpLastErrorTime = xrtNow();
	}
	XS_LogWarn(
		"udp error: server=%s sys=%d",
		objServer && objServer->Name ? objServer->Name : "(null)",
		iSysErr
	);
}

static const xnetdgramevents* XS_UdpEvents(void)
{
	static const xnetdgramevents tEvents = {
		XS_UdpOnRecv,
		XS_UdpOnError
	};
	
	return &tEvents;
}

static inline bool XS_UdpInitServer(xnetengine* pEngine, XS_ServerConfig* objServer)
{
	xnetdgramconfig tCfg;
	XS_UdpHandle* objHandle;
	
	if ( objServer == NULL || pEngine == NULL ) {
		return FALSE;
	}
	
	objHandle = (XS_UdpHandle*)xrtCalloc(1, sizeof(XS_UdpHandle));
	if ( objHandle == NULL ) {
		XS_ReportError("udp init failed: alloc handle");
		return FALSE;
	}
	
	xrtNetDgramConfigInit(&tCfg);
	if ( !XS_BuildBindAddr(objServer, FALSE, &tCfg.tBindAddr) ) {
		xrtFree(objHandle);
		XS_ReportError("udp init failed: invalid addr: %s", objServer->Addr ? objServer->Addr : "(null)");
		return FALSE;
	}
	
	objHandle->pSock = xrtNetDgramCreate(pEngine, &tCfg, XS_UdpEvents(), objServer);
	if ( objHandle->pSock == NULL ) {
		xrtFree(objHandle);
		XS_ReportError("udp init failed: create dgram socket");
		return FALSE;
	}
	
	objHandle->pServer = objServer;
	objServer->pHandle = objHandle;
	XS_LogInfo(
		"udp init: server=%s addr=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)"
	);
	return TRUE;
}

static inline bool XS_UdpStartServer(XS_ServerConfig* objServer)
{
	XS_UdpHandle* objHandle;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	
	objHandle = (XS_UdpHandle*)objServer->pHandle;
	if ( objHandle == NULL || objHandle->pSock == NULL ) {
		XS_ReportError("udp start failed: handle is null");
		return FALSE;
	}
	if ( xrtNetDgramStart(objHandle->pSock) != XRT_NET_OK ) {
		XS_ReportError("udp start failed: dgram start error");
		return FALSE;
	}
	
	XS_LogInfo(
		"udp start: server=%s addr=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)"
	);
	return TRUE;
}

static inline void XS_UdpStopServer(XS_ServerConfig* objServer)
{
	XS_UdpHandle* objHandle;
	
	if ( objServer == NULL ) {
		return;
	}
	
	objHandle = (XS_UdpHandle*)objServer->pHandle;
	if ( objHandle ) {
		if ( objHandle->pSock ) {
			xrtNetDgramStop(objHandle->pSock);
			xrtNetDgramDestroy(objHandle->pSock);
		}
		xrtFree(objHandle);
		objServer->pHandle = NULL;
	}
	
	XS_LogInfo(
		"udp stop: server=%s",
		objServer->Name ? objServer->Name : "(null)"
	);
}

#endif
