#ifndef XS_PROTOCOL_UDP_H
#define XS_PROTOCOL_UDP_H

typedef struct {
	xdgramsock* pSock;
	XS_ServerConfig* pServer;
} XS_UdpHandle;

static void XS_UdpOnRecv(ptr pOwner, xdgramsock* pSock, const xnetaddr* pFrom, xnetchain* pChain)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	size_t iLen;
	char* pBuf;
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
	
	XS_LogInfo(
		"udp recv: server=%s from=%s bytes=%u script_dgram=%s",
		objServer && objServer->Name ? objServer->Name : "(null)",
		pFrom ? xrtNetAddrToStr(pFrom) : "(null)",
		(unsigned)iLen,
		(objServer && objServer->procDgramRecv) ? "true" : "false"
	);
	
	if ( objServer && objServer->procDgramRecv ) {
		bHandled = objServer->procDgramRecv(objServer, pSock, pFrom, pBuf, iLen);
	}
	
	if ( !bHandled && pFrom ) {
		(void)xrtNetDgramSendTo(pSock, pFrom, pBuf, iLen);
	}
	
	xrtFree(pBuf);
}

static void XS_UdpOnError(ptr pOwner, xdgramsock* pSock, int iSysErr)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pOwner;
	(void)pSock;
	
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
