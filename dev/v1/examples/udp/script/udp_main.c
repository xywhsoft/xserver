#include <xsbase.h>
#include <string.h>

#include "app_db.h"



typedef struct UDP_PeerItem
{
	char* sName;
	char* sIP;
	unsigned short iPort;
	char* sRemote;
	struct UDP_PeerItem* pNext;
} UDP_PeerItem;



static xmutex G_PeerLock = NULL;
static UDP_PeerItem* G_PeerHead = NULL;



static char* procDupText(const char* sText)
{
	return xrtCopyStr((str)(sText ? sText : ""), 0);
}

static char* procDupBodyText(const void* pData, size_t iLen)
{
	char* sText;

	if ( pData == NULL ) {
		return NULL;
	}

	sText = (char*)xrtMalloc(iLen + 1u);
	if ( sText == NULL ) {
		return NULL;
	}
	memcpy(sText, pData, iLen);
	sText[iLen] = '\0';
	return sText;
}

static bool procMakeRemoteText(const void* pFromAddr, char* sBuf, size_t iBufSize)
{
	const xnetaddr* pAddr = (const xnetaddr*)pFromAddr;
	const char* sIP;
	int iWrite;

	if ( pAddr == NULL || sBuf == NULL || iBufSize == 0 ) {
		return FALSE;
	}

	sIP = xsAddrText(pFromAddr);
	if ( sIP == NULL || sIP[0] == '\0' ) {
		return FALSE;
	}

	if ( pAddr->iFamily == AF_INET6 ) {
		iWrite = snprintf(sBuf, iBufSize, "[%s]:%u", sIP, (unsigned)pAddr->iPort);
	} else {
		iWrite = snprintf(sBuf, iBufSize, "%s:%u", sIP, (unsigned)pAddr->iPort);
	}

	return iWrite > 0 && (size_t)iWrite < iBufSize;
}

static bool procParseRemote(const char* sRemote, char* sIP, size_t iIPSize, unsigned short* piPort)
{
	const char* sPos;
	const char* sHost = sRemote;
	unsigned long iPort;
	size_t iIPLen;

	if ( sRemote == NULL || sIP == NULL || iIPSize == 0 || piPort == NULL ) {
		return FALSE;
	}

	if ( sRemote[0] == '[' ) {
		const char* sEnd = strchr(sRemote, ']');

		if ( sEnd == NULL || sEnd[1] != ':' ) {
			return FALSE;
		}
		sHost = sRemote + 1;
		sPos = sEnd + 1;
		iIPLen = (size_t)(sEnd - sHost);
	} else {
		sPos = strrchr(sRemote, ':');
		if ( sPos == NULL ) {
			return FALSE;
		}
		iIPLen = (size_t)(sPos - sHost);
	}

	if ( iIPLen == 0 || iIPLen >= iIPSize ) {
		return FALSE;
	}
	memcpy(sIP, sHost, iIPLen);
	sIP[iIPLen] = '\0';

	iPort = strtoul(sPos + 1, NULL, 10);
	if ( iPort == 0 || iPort > 65535ul ) {
		return FALSE;
	}

	*piPort = (unsigned short)iPort;
	return TRUE;
}

static UDP_PeerItem* procFindPeer(const char* sName)
{
	UDP_PeerItem* objPeer = G_PeerHead;

	while ( objPeer ) {
		if ( sName && strcmp(objPeer->sName, sName) == 0 ) {
			return objPeer;
		}
		objPeer = objPeer->pNext;
	}

	return NULL;
}

static UDP_PeerItem* procUpsertPeerMemory(const char* sName, const char* sRemote)
{
	UDP_PeerItem* objPeer;
	char sIP[128];
	unsigned short iPort;

	if ( sName == NULL || sName[0] == '\0' || sRemote == NULL || sRemote[0] == '\0' ) {
		return NULL;
	}
	if ( !procParseRemote(sRemote, sIP, sizeof(sIP), &iPort) ) {
		return NULL;
	}

	objPeer = procFindPeer(sName);
	if ( objPeer == NULL ) {
		objPeer = (UDP_PeerItem*)xrtCalloc(1, sizeof(UDP_PeerItem));
		if ( objPeer == NULL ) {
			return NULL;
		}
		objPeer->sName = procDupText(sName);
		objPeer->pNext = G_PeerHead;
		G_PeerHead = objPeer;
	} else {
		if ( objPeer->sIP ) {
			xrtFree(objPeer->sIP);
		}
		if ( objPeer->sRemote ) {
			xrtFree(objPeer->sRemote);
		}
	}

	objPeer->sIP = procDupText(sIP);
	objPeer->sRemote = procDupText(sRemote);
	objPeer->iPort = iPort;
	return objPeer;
}

static void procFreePeers(void)
{
	UDP_PeerItem* objPeer = G_PeerHead;

	G_PeerHead = NULL;
	while ( objPeer ) {
		UDP_PeerItem* objNext = objPeer->pNext;

		if ( objPeer->sName ) {
			xrtFree(objPeer->sName);
		}
		if ( objPeer->sIP ) {
			xrtFree(objPeer->sIP);
		}
		if ( objPeer->sRemote ) {
			xrtFree(objPeer->sRemote);
		}
		xrtFree(objPeer);
		objPeer = objNext;
	}
}

static bool procReplyJSON(void* pSock, const void* pFromAddr, xvalue objVal)
{
	char* sJSON;
	bool bRet;

	sJSON = xrtStringifyJSON(objVal, FALSE, NULL);
	xvoUnref(objVal);
	if ( sJSON == NULL ) {
		return FALSE;
	}

	bRet = xsDgramReply(pSock, pFromAddr, sJSON, strlen(sJSON)) != 0;
	xrtFree(sJSON);
	return bRet;
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
	(void)objServer;
	(void)objHost;

	if ( G_PeerLock ) {
		xrtMutexLock(G_PeerLock);
		procFreePeers();
		xrtMutexUnlock(G_PeerLock);
		xrtMutexDestroy(G_PeerLock);
		G_PeerLock = NULL;
	}
}



bool EventDgramProc(XS_ServerObject objServer, void* pSock, const void* pFromAddr, const void* pData, size_t iLen)
{
	xvalue objReq;
	xvalue objAck;
	xvalue objPush;
	const char* sName;
	const char* sToName;
	const char* sText;
	const char* sRemote;
	char* sPayload;
	char sRemoteText[160];
	UDP_PeerItem* objTarget = NULL;
	char* sNow;

	(void)objServer;

	if ( pData == NULL || iLen == 0 ) {
		return FALSE;
	}

	sPayload = procDupBodyText(pData, iLen);
	if ( sPayload == NULL ) {
		return FALSE;
	}

	objReq = xrtParseJSON(sPayload, strlen(sPayload));
	if ( objReq == NULL || objReq->Type != XVO_DT_TABLE ) {
		if ( objReq ) {
			xvoUnref(objReq);
		}
		xrtFree(sPayload);
		return FALSE;
	}

	sName = xvoTableGetText(objReq, "name", 4);
	sToName = xvoTableGetText(objReq, "to", 2);
	sText = xvoTableGetText(objReq, "text", 4);
	if ( procMakeRemoteText(pFromAddr, sRemoteText, sizeof(sRemoteText)) ) {
		sRemote = sRemoteText;
	} else {
		sRemote = xsAddrText(pFromAddr);
	}
	if ( sName == NULL || sName[0] == '\0' || sText == NULL || sText[0] == '\0' ) {
		xvoUnref(objReq);
		xrtFree(sPayload);
		return FALSE;
	}

	xrtMutexLock(G_PeerLock);
	(void)procUpsertPeerMemory(sName, sRemote ? sRemote : "");
	if ( sToName && sToName[0] != '\0' ) {
		objTarget = procFindPeer(sToName);
	}
	xrtMutexUnlock(G_PeerLock);

	(void)procUpsertPeer(sName, sRemote ? sRemote : "", sText);
	(void)procInsertChat(sName, sToName ? sToName : "", sText, sRemote ? sRemote : "");

	sNow = xrtNowStr();
	objAck = xvoCreateTable();
	xvoTableSetBool(objAck, "result", 6, TRUE);
	xvoTableSetText(objAck, "name", 4, sName, 0, FALSE);
	xvoTableSetText(objAck, "to", 2, sToName ? sToName : "", 0, FALSE);
	xvoTableSetText(objAck, "text", 4, sText, 0, FALSE);
	xvoTableSetText(objAck, "time", 4, sNow ? sNow : "", 0, FALSE);
	(void)procReplyJSON(pSock, pFromAddr, objAck);

	if ( objTarget && objTarget->sIP ) {
		char* sJSON;

		objPush = xvoCreateTable();
		xvoTableSetText(objPush, "kind", 4, "chat", 0, FALSE);
		xvoTableSetText(objPush, "from", 4, sName, 0, FALSE);
		xvoTableSetText(objPush, "to", 2, sToName ? sToName : "", 0, FALSE);
		xvoTableSetText(objPush, "text", 4, sText, 0, FALSE);
		xvoTableSetText(objPush, "time", 4, sNow ? sNow : "", 0, FALSE);
		sJSON = xrtStringifyJSON(objPush, FALSE, NULL);
		xvoUnref(objPush);
		if ( sJSON ) {
			(void)xsDgramSendTo(pSock, objTarget->sIP, objTarget->iPort, sJSON, strlen(sJSON));
			xrtFree(sJSON);
		}
	}

	if ( sNow ) {
		xrtFree(sNow);
	}
	xvoUnref(objReq);
	xrtFree(sPayload);
	return TRUE;
}
