#ifndef XS_CORE_BUS_H
#define XS_CORE_BUS_H

typedef enum {
	XS_MSG_TARGET_HOST = 1,
	XS_MSG_TARGET_SERVER = 2
} XS_MessageTarget;

typedef struct {
	int64 iMsgID;
	XS_MessageTarget iTargetType;
	char* sTopic;
	char* sServer;
	char* sHost;
	int64 iDataID;
	xvalue objArgs;
	int64 tPost;
} XS_Message;

typedef struct {
	xmutex pLock;
	xlist pDataList;
	xlist pRefList;
	xlist pTagList;
	xlist pCreateList;
	xarray arrMessageQueue;
	int64 iNextDataID;
	int64 iNextMsgID;
} XS_Bus;

static XS_Bus g_objXsBus = { 0 };

static inline char* XS_BusCopyText(const char* sText)
{
	if ( sText == NULL || sText[0] == '\0' ) {
		return NULL;
	}
	
	return (char*)xrtCopyStr((str)sText, 0);
}

static bool XS_BusPublishValue(xvalue objVal);

static bool XS_BusPublishListProc(int64 iKey, xvalue* ppVal, bool* pbOk)
{
	(void)iKey;
	
	if ( pbOk == NULL || *pbOk == FALSE ) {
		return FALSE;
	}
	
	if ( ppVal && ppVal[0] ) {
		*pbOk = XS_BusPublishValue(ppVal[0]);
	}
	return FALSE;
}

static bool XS_BusPublishTableProc(Dict_Key* pKey, xvalue* ppVal, bool* pbOk)
{
	(void)pKey;
	
	if ( pbOk == NULL || *pbOk == FALSE ) {
		return FALSE;
	}
	
	if ( ppVal && ppVal[0] ) {
		*pbOk = XS_BusPublishValue(ppVal[0]);
	}
	return FALSE;
}

static bool XS_BusPublishCollProc(Coll_Key* pKey, bool* pbOk)
{
	if ( pbOk == NULL || *pbOk == FALSE ) {
		return FALSE;
	}
	
	if ( pKey && pKey->Value ) {
		*pbOk = XS_BusPublishValue(pKey->Value);
	}
	return FALSE;
}

static bool XS_BusPublishValue(xvalue objVal)
{
	uint32 i;
	bool bOk = TRUE;
	
	if ( objVal == NULL || objVal->IsStatic ) {
		return TRUE;
	}
	
	switch ( objVal->Type ) {
		case XVO_DT_ARRAY:
			xrtOwnerSetShared(&objVal->vArray->Owner);
			xrtOwnerActivateShared(&objVal->vArray->Owner);
			for ( i = 1; i <= objVal->vArray->Count; i++ ) {
				xvalue objItem = xrtPtrArrayGet_Inline(objVal->vArray, i);
				if ( !XS_BusPublishValue(objItem) ) {
					return FALSE;
				}
			}
			break;
		case XVO_DT_LIST:
			xrtOwnerSetShared(&objVal->vList->Owner);
			xrtOwnerActivateShared(&objVal->vList->Owner);
			xrtListWalk(objVal->vList, (List_EachProc)XS_BusPublishListProc, &bOk);
			if ( !bOk ) {
				return FALSE;
			}
			break;
		case XVO_DT_TABLE:
			xrtOwnerSetShared(&objVal->vTable->Owner);
			xrtOwnerActivateShared(&objVal->vTable->Owner);
			xrtDictWalk(objVal->vTable, (Dict_EachProc)XS_BusPublishTableProc, &bOk);
			if ( !bOk ) {
				return FALSE;
			}
			break;
		case XVO_DT_COLL:
			xrtOwnerSetShared(&objVal->vColl->Owner);
			xrtOwnerActivateShared(&objVal->vColl->Owner);
			xrtAVLTreeWalk(objVal->vColl, (AVLTree_EachProc)XS_BusPublishCollProc, &bOk);
			if ( !bOk ) {
				return FALSE;
			}
			break;
		default:
			break;
	}
	
	return xvoMakeShared_Inline(objVal);
}

static inline void XS_BusFreeMessage(XS_Message* objMsg)
{
	if ( objMsg == NULL ) {
		return;
	}
	
	if ( objMsg->sTopic ) xrtFree(objMsg->sTopic);
	if ( objMsg->sServer ) xrtFree(objMsg->sServer);
	if ( objMsg->sHost ) xrtFree(objMsg->sHost);
	if ( objMsg->objArgs ) xvoUnref(objMsg->objArgs);
	memset(objMsg, 0, sizeof(XS_Message));
}

static inline bool XS_BusInit(void)
{
	memset(&g_objXsBus, 0, sizeof(g_objXsBus));
	g_objXsBus.pLock = xrtMutexCreate();
	g_objXsBus.pDataList = xrtListCreate(sizeof(ptr), XRT_OBJMODE_SHARED);
	g_objXsBus.pRefList = xrtListCreate(sizeof(int64), XRT_OBJMODE_SHARED);
	g_objXsBus.pTagList = xrtListCreate(sizeof(ptr), XRT_OBJMODE_SHARED);
	g_objXsBus.pCreateList = xrtListCreate(sizeof(int64), XRT_OBJMODE_SHARED);
	g_objXsBus.arrMessageQueue = xrtArrayCreate(sizeof(XS_Message), XRT_OBJMODE_SHARED);
	g_objXsBus.iNextDataID = 1;
	g_objXsBus.iNextMsgID = 1;
	
	if ( g_objXsBus.pLock == NULL || g_objXsBus.pDataList == NULL || g_objXsBus.pRefList == NULL || g_objXsBus.pTagList == NULL || g_objXsBus.pCreateList == NULL || g_objXsBus.arrMessageQueue == NULL ) {
		return FALSE;
	}
	
	xrtOwnerActivateShared(&g_objXsBus.pDataList->Owner);
	xrtOwnerActivateShared(&g_objXsBus.pRefList->Owner);
	xrtOwnerActivateShared(&g_objXsBus.pTagList->Owner);
	xrtOwnerActivateShared(&g_objXsBus.pCreateList->Owner);
	xrtOwnerActivateShared(&g_objXsBus.arrMessageQueue->Owner);
	
	return TRUE;
}

static bool XS_BusFreeDataProc(int64 iKey, ptr pVal, ptr pArg)
{
	xvalue objVal = (xvalue)pVal;
	(void)iKey;
	(void)pArg;
	
	if ( objVal ) {
		xvoUnref(objVal);
	}
	return FALSE;
}

static bool XS_BusFreeTagProc(int64 iKey, ptr pVal, ptr pArg)
{
	char* sTag = (char*)pVal;
	(void)iKey;
	(void)pArg;
	
	if ( sTag ) {
		xrtFree(sTag);
	}
	return FALSE;
}

static inline void XS_BusUnit(void)
{
	uint32 i;
	
	if ( g_objXsBus.arrMessageQueue ) {
		for ( i = 1; i <= g_objXsBus.arrMessageQueue->Count; i++ ) {
			XS_Message* objMsg = xrtArrayGet_Inline(g_objXsBus.arrMessageQueue, i);
			XS_BusFreeMessage(objMsg);
		}
		xrtArrayDestroy(g_objXsBus.arrMessageQueue);
	}
	if ( g_objXsBus.pDataList ) {
		xrtListWalk(g_objXsBus.pDataList, XS_BusFreeDataProc, NULL);
		xrtListDestroy(g_objXsBus.pDataList);
	}
	if ( g_objXsBus.pRefList ) {
		xrtListDestroy(g_objXsBus.pRefList);
	}
	if ( g_objXsBus.pTagList ) {
		xrtListWalk(g_objXsBus.pTagList, XS_BusFreeTagProc, NULL);
		xrtListDestroy(g_objXsBus.pTagList);
	}
	if ( g_objXsBus.pCreateList ) {
		xrtListDestroy(g_objXsBus.pCreateList);
	}
	if ( g_objXsBus.pLock ) {
		xrtMutexDestroy(g_objXsBus.pLock);
	}
	
	memset(&g_objXsBus, 0, sizeof(g_objXsBus));
}

static inline int64 XS_BusDataRegisterEx(xvalue objValue, const char* sTag)
{
	int64 iID;
	xvalue objStore;
	int64* pTime;
	char** ppTag;
	
	if ( objValue == NULL ) {
		XS_LogWarn("bus register failed: value is null");
		return 0;
	}
	
	objStore = xvoDeepCopy(objValue);
	if ( objStore == NULL ) {
		XS_LogWarn("bus register failed: deep copy error");
		return 0;
	}
	
	if ( !XS_BusPublishValue(objStore) ) {
		XS_LogWarn(
			"bus register failed: publish error: %s",
			(const char*)(xrtGetError() ? xrtGetError() : (str)"(null)")
		);
		xvoUnref(objStore);
		return 0;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	iID = g_objXsBus.iNextDataID++;
	if ( !xrtListSetPtr(g_objXsBus.pDataList, iID, objStore, NULL) ) {
		XS_LogWarn(
			"bus register failed: data slot create error: id=%lld type=%d err=%s",
			(long long)iID,
			xvoType(objStore),
			(const char*)(xrtGetError() ? xrtGetError() : (str)"(null)")
		);
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return 0;
	}
	{
		int64* pRef = xrtListSet(g_objXsBus.pRefList, iID, NULL);
		if ( pRef == NULL ) {
			XS_LogWarn(
				"bus register failed: ref slot create error: id=%lld err=%s",
				(long long)iID,
				(const char*)(xrtGetError() ? xrtGetError() : (str)"(null)")
			);
			(void)xrtListRemovePtr(g_objXsBus.pDataList, iID);
			xrtMutexUnlock(g_objXsBus.pLock);
			xvoUnref(objStore);
			return 0;
		}
		*pRef = 1;
	}
	pTime = (int64*)xrtListSet(g_objXsBus.pCreateList, iID, NULL);
	if ( pTime == NULL ) {
		(void)xrtListRemove(g_objXsBus.pRefList, iID);
		(void)xrtListRemovePtr(g_objXsBus.pDataList, iID);
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return 0;
	}
	*pTime = xrtNow();
	ppTag = (char**)xrtListSet(g_objXsBus.pTagList, iID, NULL);
	if ( ppTag == NULL ) {
		(void)xrtListRemove(g_objXsBus.pCreateList, iID);
		(void)xrtListRemove(g_objXsBus.pRefList, iID);
		(void)xrtListRemovePtr(g_objXsBus.pDataList, iID);
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return 0;
	}
	*ppTag = XS_BusCopyText(sTag);
	XS_LogInfo("bus data registered: id=%lld type=%d", (long long)iID, xvoType(objStore));
	xrtMutexUnlock(g_objXsBus.pLock);
	return iID;
}

static inline int64 XS_BusDataRegister(xvalue objValue)
{
	return XS_BusDataRegisterEx(objValue, NULL);
}

static inline xvalue XS_BusDataGet(int64 iID)
{
	xvalue objRet;
	
	if ( iID <= 0 ) {
		return NULL;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	objRet = (xvalue)xrtListGetPtr(g_objXsBus.pDataList, iID);
	xrtMutexUnlock(g_objXsBus.pLock);
	return objRet;
}

static inline bool XS_BusDataRetain(int64 iID)
{
	int64* pRef;
	xvalue objVal;
	
	if ( iID <= 0 ) {
		return FALSE;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	objVal = (xvalue)xrtListGetPtr(g_objXsBus.pDataList, iID);
	pRef = (int64*)xrtListGet(g_objXsBus.pRefList, iID);
	if ( objVal == NULL || pRef == NULL ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		return FALSE;
	}
	(*pRef)++;
	xrtMutexUnlock(g_objXsBus.pLock);
	return TRUE;
}

static inline bool XS_BusDataRelease(int64 iID)
{
	int64* pRef;
	xvalue objVal;
	
	if ( iID <= 0 ) {
		return FALSE;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	objVal = (xvalue)xrtListGetPtr(g_objXsBus.pDataList, iID);
	pRef = (int64*)xrtListGet(g_objXsBus.pRefList, iID);
	if ( objVal == NULL || pRef == NULL ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		return FALSE;
	}
	
	if ( *pRef <= 1 ) {
		char* sTag = NULL;
		
		sTag = (char*)xrtListRemovePtr(g_objXsBus.pTagList, iID);
		(void)xrtListRemove(g_objXsBus.pCreateList, iID);
		(void)xrtListRemove(g_objXsBus.pRefList, iID);
		(void)xrtListRemovePtr(g_objXsBus.pDataList, iID);
		xrtMutexUnlock(g_objXsBus.pLock);
		if ( sTag ) {
			xrtFree(sTag);
		}
		xvoUnref(objVal);
		return TRUE;
	}
	
	(*pRef)--;
	xrtMutexUnlock(g_objXsBus.pLock);
	return TRUE;
}

static inline bool XS_BusDataRemove(int64 iID)
{
	if ( iID <= 0 ) {
		return FALSE;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	{
		char* sTag = (char*)xrtListRemovePtr(g_objXsBus.pTagList, iID);
		if ( sTag ) {
			xrtFree(sTag);
		}
	}
	(void)xrtListRemove(g_objXsBus.pCreateList, iID);
	(void)xrtListRemove(g_objXsBus.pRefList, iID);
	{
		xvalue objVal = (xvalue)xrtListRemovePtr(g_objXsBus.pDataList, iID);
		if ( objVal ) {
			xvoUnref(objVal);
		}
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	return TRUE;
}

static bool XS_BusBuildStatusProc(int64 iKey, ptr pVal, xvalue objArr)
{
	xvalue objItem;
	int64* pRef;
	int64* pCreate;
	char* sTag;
	
	if ( objArr == NULL || pVal == NULL ) {
		return FALSE;
	}
	
	objItem = xvoCreateTable();
	pRef = (int64*)xrtListGet(g_objXsBus.pRefList, iKey);
	pCreate = (int64*)xrtListGet(g_objXsBus.pCreateList, iKey);
	sTag = (char*)xrtListGetPtr(g_objXsBus.pTagList, iKey);
	
	xvoTableSetInt(objItem, "id", 2, iKey);
	xvoTableSetInt(objItem, "type", 4, xvoType((xvalue)pVal));
	xvoTableSetInt(objItem, "ref_count", 9, pRef ? *pRef : 0);
	xvoTableSetInt(objItem, "create_time", 11, pCreate ? *pCreate : 0);
	xvoTableSetText(objItem, "tag", 3, sTag ? sTag : "", 0, FALSE);
	xvoArrayAppendValue(objArr, objItem, TRUE);
	return FALSE;
}

static inline xvalue XS_BusBuildStatusValue(void)
{
	xvalue objRet = xvoCreateTable();
	xvalue objItems = xvoCreateArray();
	
	xrtMutexLock(g_objXsBus.pLock);
	xvoTableSetInt(objRet, "queue_count", 11, g_objXsBus.arrMessageQueue ? g_objXsBus.arrMessageQueue->Count : 0);
	xvoTableSetInt(objRet, "data_count", 10, g_objXsBus.pDataList ? xrtListCount(g_objXsBus.pDataList) : 0);
	xvoTableSetInt(objRet, "next_data_id", 12, g_objXsBus.iNextDataID);
	xvoTableSetInt(objRet, "next_msg_id", 11, g_objXsBus.iNextMsgID);
	if ( g_objXsBus.pDataList ) {
		xrtListWalk(g_objXsBus.pDataList, (List_EachProc)XS_BusBuildStatusProc, objItems);
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	xvoTableSetValue(objRet, "items", 5, objItems, TRUE);
	return objRet;
}

static inline char* XS_BusBuildStatusJson(void)
{
	xvalue objStatus = XS_BusBuildStatusValue();
	char* sRet = NULL;
	
	if ( objStatus == NULL ) {
		return NULL;
	}
	
	sRet = xrtStringifyJSON(objStatus, FALSE, NULL);
	xvoUnref(objStatus);
	return sRet;
}

static inline bool XS_BusQueueMessage(XS_MessageTarget iTargetType, const char* sServer, const char* sHost, const char* sTopic, int64 iDataID, xvalue objArgs)
{
	uint32 iPos;
	XS_Message* objMsg;
	
	if ( sTopic == NULL || sTopic[0] == '\0' ) {
		return FALSE;
	}
	
	if ( iDataID > 0 && !XS_BusDataRetain(iDataID) ) {
		return FALSE;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	iPos = xrtArrayAppend(g_objXsBus.arrMessageQueue, 1);
	objMsg = xrtArrayGet_Inline(g_objXsBus.arrMessageQueue, iPos);
	memset(objMsg, 0, sizeof(XS_Message));
	objMsg->iMsgID = g_objXsBus.iNextMsgID++;
	objMsg->iTargetType = iTargetType;
	objMsg->sTopic = XS_BusCopyText(sTopic);
	objMsg->sServer = XS_BusCopyText(sServer);
	objMsg->sHost = XS_BusCopyText(sHost);
	objMsg->iDataID = iDataID;
	objMsg->tPost = xrtNow();
	if ( objArgs ) {
		xvoAddRef(objArgs);
		objMsg->objArgs = objArgs;
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	return TRUE;
}

static inline bool XS_BusSendToHost(const char* sServer, const char* sHost, const char* sTopic, int64 iDataID, xvalue objArgs)
{
	return XS_BusQueueMessage(XS_MSG_TARGET_HOST, sServer, sHost, sTopic, iDataID, objArgs);
}

static inline bool XS_BusSendToServer(const char* sServer, const char* sTopic, int64 iDataID, xvalue objArgs)
{
	return XS_BusQueueMessage(XS_MSG_TARGET_SERVER, sServer, NULL, sTopic, iDataID, objArgs);
}

static inline bool XS_BusDeliverHost(XS_ServerConfig* objServer, XS_HostConfig* objHost, XS_Message* objMsg)
{
	if ( objHost == NULL || objHost->procMessage == NULL ) {
		return FALSE;
	}
	
	return objHost->procMessage(objServer, objHost, objMsg->sTopic, objMsg->iDataID, objMsg->objArgs);
}

static inline bool XS_BusDispatchToServer(XS_ServerConfig* objServer, XS_Message* objMsg)
{
	uint32 i;
	bool bHandled = FALSE;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	
	if ( objServer->EnableDefaultHost ) {
		bHandled = XS_BusDeliverHost(objServer, &objServer->DefaultHost, objMsg) || bHandled;
	}
	
	for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
		XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
		bHandled = XS_BusDeliverHost(objServer, objHost, objMsg) || bHandled;
	}
	
	return bHandled;
}

static inline bool XS_BusDispatchToHost(XS_ServerConfig* objServer, XS_Message* objMsg)
{
	uint32 i;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	
	if ( objMsg->sHost == NULL || objMsg->sHost[0] == '\0' ) {
		if ( objServer->EnableDefaultHost ) {
			return XS_BusDeliverHost(objServer, &objServer->DefaultHost, objMsg);
		}
		return FALSE;
	}
	
	if ( objServer->EnableDefaultHost && objServer->DefaultHost.Name && strcmp(objServer->DefaultHost.Name, objMsg->sHost) == 0 ) {
		return XS_BusDeliverHost(objServer, &objServer->DefaultHost, objMsg);
	}
	
	for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
		XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
		if ( objHost->Name && strcmp(objHost->Name, objMsg->sHost) == 0 ) {
			return XS_BusDeliverHost(objServer, objHost, objMsg);
		}
	}
	
	return FALSE;
}

static inline void XS_BusDispatchMessages(xarray arrServers)
{
	XS_Message objMsg;
	uint32 i;
	bool bHandled = FALSE;
	
	if ( arrServers == NULL ) {
		return;
	}
	
	for ( ;; ) {
		memset(&objMsg, 0, sizeof(objMsg));
		
		xrtMutexLock(g_objXsBus.pLock);
		if ( g_objXsBus.arrMessageQueue == NULL || g_objXsBus.arrMessageQueue->Count == 0 ) {
			xrtMutexUnlock(g_objXsBus.pLock);
			break;
		}
		
		memcpy(&objMsg, xrtArrayGet_Inline(g_objXsBus.arrMessageQueue, 1), sizeof(XS_Message));
		xrtArrayRemove(g_objXsBus.arrMessageQueue, 1, 1);
		xrtMutexUnlock(g_objXsBus.pLock);
		
		for ( i = 1; i <= arrServers->Count; i++ ) {
			XS_ServerConfig* objServer = xrtArrayGet_Inline(arrServers, i);
			
			if ( objMsg.sServer && objMsg.sServer[0] != '\0' ) {
				if ( objServer->Name == NULL || strcmp(objServer->Name, objMsg.sServer) != 0 ) {
					continue;
				}
			}
			
			if ( objMsg.iTargetType == XS_MSG_TARGET_HOST ) {
				bHandled = XS_BusDispatchToHost(objServer, &objMsg) || bHandled;
			} else if ( objMsg.iTargetType == XS_MSG_TARGET_SERVER ) {
				bHandled = XS_BusDispatchToServer(objServer, &objMsg) || bHandled;
			}
		}
		
		if ( !bHandled ) {
			XS_LogWarn(
				"message dropped: topic=%s server=%s host=%s data=%lld",
				objMsg.sTopic ? objMsg.sTopic : "(null)",
				objMsg.sServer ? objMsg.sServer : "(null)",
				objMsg.sHost ? objMsg.sHost : "(null)",
				(long long)objMsg.iDataID
			);
		}
		
		if ( objMsg.iDataID > 0 ) {
			XS_BusDataRelease(objMsg.iDataID);
		}
		XS_BusFreeMessage(&objMsg);
		bHandled = FALSE;
	}
}

#endif
