#include <xsbase.h>
#include <string.h>

#include "app_db.h"



static char* procDupBodyText(const void* pData, size_t iLen)
{
	char* sText;
	size_t iTrimLen = iLen;

	while ( iTrimLen > 0 ) {
		char ch = ((const char*)pData)[iTrimLen - 1];

		if ( ch == '\r' || ch == '\n' || ch == '\t' || ch == ' ' ) {
			iTrimLen--;
			continue;
		}
		break;
	}

	sText = (char*)xrtMalloc(iTrimLen + 1u);
	if ( sText == NULL ) {
		return NULL;
	}
	memcpy(sText, pData, iTrimLen);
	sText[iTrimLen] = '\0';
	return sText;
}



void EventOpenProc(XS_ServerObject objServer, void* pStream)
{
	char sPeerID[64];

	(void)objServer;

	procMakePeerID(pStream, sPeerID, sizeof(sPeerID));
	(void)procUpsertPeer(sPeerID, sPeerID, 1, "connected");
	(void)procInsertLog(sPeerID, sPeerID, "peer connected");
}



bool EventDataProc(XS_ServerObject objServer, void* pStream, const void* pData, size_t iLen)
{
	char sPeerID[64];
	char sReply[512];
	char* sPayload;
	xvalue objReq;
	const char* sName;
	const char* sText;
	int iWrite;

	(void)objServer;

	if ( pData == NULL || iLen == 0 ) {
		return FALSE;
	}

	sPayload = procDupBodyText(pData, iLen);
	if ( sPayload == NULL ) {
		return FALSE;
	}

	procMakePeerID(pStream, sPeerID, sizeof(sPeerID));
	objReq = xrtParseJSON(sPayload, strlen(sPayload));
	if ( objReq == NULL || objReq->Type != XVO_DT_TABLE ) {
		if ( objReq ) {
			xvoUnref(objReq);
		}
		iWrite = snprintf(
			sReply,
			sizeof(sReply),
			"{\"result\":false,\"peer_id\":\"%s\",\"message\":\"body must be json object\"}\n",
			sPeerID
		);
		xrtFree(sPayload);
		if ( iWrite <= 0 ) {
			return FALSE;
		}
		return xsStreamSend(pStream, sReply, strlen(sReply)) != 0;
	}

	sName = xvoTableGetText(objReq, "name", 4);
	sText = xvoTableGetText(objReq, "text", 4);
	if ( sName == NULL || sName[0] == '\0' ) {
		sName = sPeerID;
	}
	if ( sText == NULL ) {
		sText = "";
	}

	(void)procUpsertPeer(sPeerID, sName, 1, sText);
	(void)procInsertLog(sPeerID, sName, sText);
	iWrite = snprintf(
		sReply,
		sizeof(sReply),
		"{\"result\":true,\"peer_id\":\"%s\",\"name\":\"%s\",\"echo\":\"%s\"}\n",
		sPeerID,
		sName,
		sText
	);
	xvoUnref(objReq);
	xrtFree(sPayload);
	if ( iWrite <= 0 ) {
		return FALSE;
	}

	return xsStreamSend(pStream, sReply, strlen(sReply)) != 0;
}



void EventCloseProc(XS_ServerObject objServer, void* pStream, int iReason)
{
	char sPeerID[64];
	char sText[128];

	(void)objServer;

	procMakePeerID(pStream, sPeerID, sizeof(sPeerID));
	snprintf(sText, sizeof(sText), "peer closed, reason=%d", iReason);
	(void)procUpsertPeer(sPeerID, sPeerID, 0, sText);
	(void)procInsertLog(sPeerID, sPeerID, sText);
}
