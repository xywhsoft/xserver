#include <xsbase.h>
#include <string.h>



typedef struct WS_PeerItem
{
	void* pConn;
	char sPeerID[32];
	char* sName;
	struct WS_PeerItem* pNext;
} WS_PeerItem;



static xmutex G_PeerLock = NULL;
static WS_PeerItem* G_PeerHead = NULL;
static int64 G_iPeerSeq = 0;



static char* procDupText(const char* sText, const char* sDefault)
{
	const char* sSrc = sText;

	if ( sSrc == NULL || sSrc[0] == '\0' ) {
		sSrc = sDefault ? sDefault : "";
	}

	return xrtCopyStr((str)sSrc, 0);
}

static const char* procPeerName(WS_PeerItem* objPeer)
{
	if ( objPeer == NULL ) {
		return "";
	}
	if ( objPeer->sName && objPeer->sName[0] != '\0' ) {
		return objPeer->sName;
	}

	return objPeer->sPeerID;
}

static WS_PeerItem* procFindPeerByConn(void* pConn)
{
	WS_PeerItem* objPeer = G_PeerHead;

	while ( objPeer ) {
		if ( objPeer->pConn == pConn ) {
			return objPeer;
		}
		objPeer = objPeer->pNext;
	}

	return NULL;
}

static WS_PeerItem* procFindPeerByID(const char* sPeerID)
{
	WS_PeerItem* objPeer = G_PeerHead;

	while ( objPeer ) {
		if ( sPeerID && strcmp(objPeer->sPeerID, sPeerID) == 0 ) {
			return objPeer;
		}
		objPeer = objPeer->pNext;
	}

	return NULL;
}

static bool procSendValue(void* pConn, xvalue objVal)
{
	char* sJSON;
	bool bRet;

	if ( pConn == NULL || objVal == NULL ) {
		return FALSE;
	}

	sJSON = xrtStringifyJSON(objVal, FALSE, NULL);
	if ( sJSON == NULL ) {
		return FALSE;
	}

	bRet = xsWsSendText(pConn, sJSON, strlen(sJSON)) != 0;
	xrtFree(sJSON);
	return bRet;
}

static xvalue procBuildPeersValue(void)
{
	xvalue arrPeers = xvoCreateArray();
	WS_PeerItem* objPeer = G_PeerHead;

	while ( objPeer ) {
		xvalue objItem = xvoCreateTable();

		xvoTableSetText(objItem, "peer_id", 7, objPeer->sPeerID, 0, FALSE);
		xvoTableSetText(objItem, "name", 4, procPeerName(objPeer), 0, FALSE);
		xvoArrayAppendValue(arrPeers, objItem, TRUE);
		objPeer = objPeer->pNext;
	}

	return arrPeers;
}

static void procBroadcastPeers(void)
{
	xvalue objMsg;
	xvalue arrPeers;
	WS_PeerItem* objPeer;

	if ( G_PeerLock == NULL ) {
		return;
	}

	xrtMutexLock(G_PeerLock);
	objMsg = xvoCreateTable();
	arrPeers = procBuildPeersValue();
	xvoTableSetText(objMsg, "kind", 4, "peers", 0, FALSE);
	xvoTableSetValue(objMsg, "peers", 5, arrPeers, TRUE);
	objPeer = G_PeerHead;
	while ( objPeer ) {
		if ( xsWsIsOpen(objPeer->pConn) ) {
			(void)procSendValue(objPeer->pConn, objMsg);
		}
		objPeer = objPeer->pNext;
	}
	xvoUnref(objMsg);
	xrtMutexUnlock(G_PeerLock);
}

static void procSendError(void* pConn, const char* sMessage)
{
	xvalue objMsg = xvoCreateTable();

	xvoTableSetText(objMsg, "kind", 4, "error", 0, FALSE);
	xvoTableSetText(objMsg, "message", 7, sMessage ? sMessage : "", 0, FALSE);
	(void)procSendValue(pConn, objMsg);
	xvoUnref(objMsg);
}

static void procSendWelcome(void* pConn, WS_PeerItem* objSelf)
{
	xvalue objMsg;
	xvalue arrPeers;

	if ( G_PeerLock == NULL || objSelf == NULL ) {
		return;
	}

	xrtMutexLock(G_PeerLock);
	objMsg = xvoCreateTable();
	arrPeers = procBuildPeersValue();
	xvoTableSetText(objMsg, "kind", 4, "welcome", 0, FALSE);
	xvoTableSetText(objMsg, "peer_id", 7, objSelf->sPeerID, 0, FALSE);
	xvoTableSetText(objMsg, "name", 4, procPeerName(objSelf), 0, FALSE);
	xvoTableSetValue(objMsg, "peers", 5, arrPeers, TRUE);
	(void)procSendValue(pConn, objMsg);
	xvoUnref(objMsg);
	xrtMutexUnlock(G_PeerLock);
}

static void procSendDirect(WS_PeerItem* objFrom, WS_PeerItem* objTo, const char* sText)
{
	xvalue objMsg;
	char* sNow;

	if ( objFrom == NULL || objTo == NULL ) {
		return;
	}

	sNow = xrtNowStr();
	objMsg = xvoCreateTable();
	xvoTableSetText(objMsg, "kind", 4, "direct", 0, FALSE);
	xvoTableSetText(objMsg, "from", 4, objFrom->sPeerID, 0, FALSE);
	xvoTableSetText(objMsg, "from_name", 9, procPeerName(objFrom), 0, FALSE);
	xvoTableSetText(objMsg, "to", 2, objTo->sPeerID, 0, FALSE);
	xvoTableSetText(objMsg, "to_name", 7, procPeerName(objTo), 0, FALSE);
	xvoTableSetText(objMsg, "text", 4, sText ? sText : "", 0, FALSE);
	xvoTableSetText(objMsg, "time", 4, sNow ? sNow : "", 0, FALSE);
	(void)procSendValue(objFrom->pConn, objMsg);
	if ( objTo->pConn != objFrom->pConn ) {
		(void)procSendValue(objTo->pConn, objMsg);
	}
	xvoUnref(objMsg);
	if ( sNow ) {
		xrtFree(sNow);
	}
}



void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
	(void)objServer;
	(void)objHost;

	if ( G_PeerLock == NULL ) {
		G_PeerLock = xrtMutexCreate();
	}
}



void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
	WS_PeerItem* objPeer;

	(void)objServer;
	(void)objHost;

	if ( G_PeerLock ) {
		xrtMutexLock(G_PeerLock);
		objPeer = G_PeerHead;
		G_PeerHead = NULL;
		while ( objPeer ) {
			WS_PeerItem* objNext = objPeer->pNext;

			if ( objPeer->sName ) {
				xrtFree(objPeer->sName);
			}
			xrtFree(objPeer);
			objPeer = objNext;
		}
		xrtMutexUnlock(G_PeerLock);
		xrtMutexDestroy(G_PeerLock);
		G_PeerLock = NULL;
	}
}



void WsOpenProc(XS_ServerObject objServer, XS_HostObject objHost, void* pConn)
{
	WS_PeerItem* objPeer;

	(void)objServer;
	(void)objHost;

	if ( G_PeerLock == NULL || pConn == NULL ) {
		return;
	}

	objPeer = (WS_PeerItem*)xrtCalloc(1, sizeof(WS_PeerItem));
	if ( objPeer == NULL ) {
		return;
	}

	objPeer->pConn = pConn;
	xrtMutexLock(G_PeerLock);
	G_iPeerSeq++;
	snprintf(objPeer->sPeerID, sizeof(objPeer->sPeerID), "peer-%lld", (long long)G_iPeerSeq);
	objPeer->sName = procDupText(objPeer->sPeerID, objPeer->sPeerID);
	objPeer->pNext = G_PeerHead;
	G_PeerHead = objPeer;
	xrtMutexUnlock(G_PeerLock);

	procSendWelcome(pConn, objPeer);
	procBroadcastPeers();
}



bool WsTextProc(XS_ServerObject objServer, XS_HostObject objHost, void* pConn, const char* pData, size_t iLen)
{
	xvalue objReq;
	const char* sKind;
	const char* sName;
	const char* sTargetID;
	const char* sText;
	WS_PeerItem* objSelf;
	WS_PeerItem* objTarget;

	(void)objServer;
	(void)objHost;

	if ( G_PeerLock == NULL || pConn == NULL || pData == NULL || iLen == 0 ) {
		return FALSE;
	}

	objReq = xrtParseJSON((void*)pData, iLen);
	if ( objReq == NULL || objReq->Type != XVO_DT_TABLE ) {
		if ( objReq ) {
			xvoUnref(objReq);
		}
		procSendError(pConn, "消息必须为 JSON 对象。");
		return TRUE;
	}

	sKind = xvoTableGetText(objReq, "kind", 4);
	if ( sKind == NULL || sKind[0] == '\0' ) {
		xvoUnref(objReq);
		procSendError(pConn, "kind 不能为空。");
		return TRUE;
	}

	xrtMutexLock(G_PeerLock);
	objSelf = procFindPeerByConn(pConn);
	if ( objSelf == NULL ) {
		xrtMutexUnlock(G_PeerLock);
		xvoUnref(objReq);
		procSendError(pConn, "当前连接未注册。");
		return TRUE;
	}

	if ( strcmp(sKind, "hello") == 0 ) {
		sName = xvoTableGetText(objReq, "name", 4);
		if ( objSelf->sName ) {
			xrtFree(objSelf->sName);
		}
		objSelf->sName = procDupText(sName, objSelf->sPeerID);
		xrtMutexUnlock(G_PeerLock);
		xvoUnref(objReq);
		procSendWelcome(pConn, objSelf);
		procBroadcastPeers();
		return TRUE;
	}

	if ( strcmp(sKind, "direct") == 0 ) {
		sTargetID = xvoTableGetText(objReq, "to", 2);
		sText = xvoTableGetText(objReq, "text", 4);
		if ( sTargetID == NULL || sTargetID[0] == '\0' ) {
			xrtMutexUnlock(G_PeerLock);
			xvoUnref(objReq);
			procSendError(pConn, "目标 peer_id 不能为空。");
			return TRUE;
		}
		if ( sText == NULL || sText[0] == '\0' ) {
			xrtMutexUnlock(G_PeerLock);
			xvoUnref(objReq);
			procSendError(pConn, "消息内容不能为空。");
			return TRUE;
		}

		objTarget = procFindPeerByID(sTargetID);
		if ( objTarget == NULL ) {
			xrtMutexUnlock(G_PeerLock);
			xvoUnref(objReq);
			procSendError(pConn, "目标 peer 不在线。");
			return TRUE;
		}

		procSendDirect(objSelf, objTarget, sText);
		xrtMutexUnlock(G_PeerLock);
		xvoUnref(objReq);
		return TRUE;
	}

	xrtMutexUnlock(G_PeerLock);
	xvoUnref(objReq);
	procSendError(pConn, "不支持的 kind。");
	return TRUE;
}



void WsCloseProc(XS_ServerObject objServer, XS_HostObject objHost, void* pConn, int iReason)
{
	WS_PeerItem* objPrev = NULL;
	WS_PeerItem* objPeer;

	(void)objServer;
	(void)objHost;
	(void)iReason;

	if ( G_PeerLock == NULL || pConn == NULL ) {
		return;
	}

	xrtMutexLock(G_PeerLock);
	objPeer = G_PeerHead;
	while ( objPeer ) {
		if ( objPeer->pConn == pConn ) {
			if ( objPrev ) {
				objPrev->pNext = objPeer->pNext;
			} else {
				G_PeerHead = objPeer->pNext;
			}
			if ( objPeer->sName ) {
				xrtFree(objPeer->sName);
			}
			xrtFree(objPeer);
			break;
		}
		objPrev = objPeer;
		objPeer = objPeer->pNext;
	}
	xrtMutexUnlock(G_PeerLock);

	procBroadcastPeers();
}
