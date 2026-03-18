#ifndef XS_CORE_BUS_H
#define XS_CORE_BUS_H

typedef enum {
	XS_MSG_TARGET_HOST = 1,
	XS_MSG_TARGET_SERVER = 2,
	XS_MSG_TARGET_BROADCAST = 3
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
	xlist pNamespaceList;
	xlist pTagList;
	xlist pCreateList;
	xlist pExpireList;
	xarray arrMessageQueue;
	int64 iNextDataID;
	int64 iNextMsgID;
} XS_Bus;

static XS_Bus g_objXsBus = { 0 };
static int32 g_iXsBusLastErrorCode = 0;
static char g_sXsBusLastError[256] = { 0 };

enum {
	XS_BUS_ERR_NONE = 0,
	XS_BUS_ERR_INVALID_VALUE = 1,
	XS_BUS_ERR_RESERVED_NAMESPACE = 2,
	XS_BUS_ERR_DEEP_COPY = 3,
	XS_BUS_ERR_PUBLISH = 4,
	XS_BUS_ERR_DATA_SLOT = 5,
	XS_BUS_ERR_REF_SLOT = 6,
	XS_BUS_ERR_CREATE_SLOT = 7,
	XS_BUS_ERR_NAMESPACE_SLOT = 8,
	XS_BUS_ERR_TAG_SLOT = 9,
	XS_BUS_ERR_EXPIRE_SLOT = 10
};

static inline void XS_BusSetLastError(int32 iCode, const char* sMessage)
{
	g_iXsBusLastErrorCode = iCode;
	snprintf(g_sXsBusLastError, sizeof(g_sXsBusLastError), "%s", sMessage ? sMessage : "");
}

static inline void XS_BusClearLastError(void)
{
	XS_BusSetLastError(XS_BUS_ERR_NONE, "");
}

static inline int32 XS_BusGetLastErrorCode(void)
{
	return g_iXsBusLastErrorCode;
}

static inline const char* XS_BusGetLastError(void)
{
	return g_sXsBusLastError;
}

static inline char* XS_BusCopyText(const char* sText)
{
	if ( sText == NULL || sText[0] == '\0' ) {
		return NULL;
	}
	
	return (char*)xrtCopyStr((str)sText, 0);
}

static inline bool XS_BusIsReservedNamespace(const char* sNamespace)
{
	if ( sNamespace == NULL || sNamespace[0] == '\0' ) {
		return FALSE;
	}
	
	if ( strncmp(sNamespace, "xs.", 3) == 0 ) {
		return TRUE;
	}
	if ( strncmp(sNamespace, "__xs", 4) == 0 ) {
		return TRUE;
	}
	
	return FALSE;
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
	XS_BusClearLastError();
	g_objXsBus.pLock = xrtMutexCreate();
	g_objXsBus.pDataList = xrtListCreate(sizeof(ptr), XRT_OBJMODE_SHARED);
	g_objXsBus.pRefList = xrtListCreate(sizeof(int64), XRT_OBJMODE_SHARED);
	g_objXsBus.pNamespaceList = xrtListCreate(sizeof(ptr), XRT_OBJMODE_SHARED);
	g_objXsBus.pTagList = xrtListCreate(sizeof(ptr), XRT_OBJMODE_SHARED);
	g_objXsBus.pCreateList = xrtListCreate(sizeof(int64), XRT_OBJMODE_SHARED);
	g_objXsBus.pExpireList = xrtListCreate(sizeof(int64), XRT_OBJMODE_SHARED);
	g_objXsBus.arrMessageQueue = xrtArrayCreate(sizeof(XS_Message), XRT_OBJMODE_SHARED);
	g_objXsBus.iNextDataID = 1;
	g_objXsBus.iNextMsgID = 1;
	
	if ( g_objXsBus.pLock == NULL || g_objXsBus.pDataList == NULL || g_objXsBus.pRefList == NULL || g_objXsBus.pNamespaceList == NULL || g_objXsBus.pTagList == NULL || g_objXsBus.pCreateList == NULL || g_objXsBus.pExpireList == NULL || g_objXsBus.arrMessageQueue == NULL ) {
		return FALSE;
	}
	
	xrtOwnerActivateShared(&g_objXsBus.pDataList->Owner);
	xrtOwnerActivateShared(&g_objXsBus.pRefList->Owner);
	xrtOwnerActivateShared(&g_objXsBus.pNamespaceList->Owner);
	xrtOwnerActivateShared(&g_objXsBus.pTagList->Owner);
	xrtOwnerActivateShared(&g_objXsBus.pCreateList->Owner);
	xrtOwnerActivateShared(&g_objXsBus.pExpireList->Owner);
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

static bool XS_BusFreeNamespaceProc(int64 iKey, ptr pVal, ptr pArg)
{
	char* sNamespace = (char*)pVal;
	(void)iKey;
	(void)pArg;
	
	if ( sNamespace ) {
		xrtFree(sNamespace);
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
	if ( g_objXsBus.pNamespaceList ) {
		xrtListWalk(g_objXsBus.pNamespaceList, XS_BusFreeNamespaceProc, NULL);
		xrtListDestroy(g_objXsBus.pNamespaceList);
	}
	if ( g_objXsBus.pTagList ) {
		xrtListWalk(g_objXsBus.pTagList, XS_BusFreeTagProc, NULL);
		xrtListDestroy(g_objXsBus.pTagList);
	}
	if ( g_objXsBus.pCreateList ) {
		xrtListDestroy(g_objXsBus.pCreateList);
	}
	if ( g_objXsBus.pExpireList ) {
		xrtListDestroy(g_objXsBus.pExpireList);
	}
	if ( g_objXsBus.pLock ) {
		xrtMutexDestroy(g_objXsBus.pLock);
	}
	
	memset(&g_objXsBus, 0, sizeof(g_objXsBus));
}

static inline int64 XS_BusDataRegisterEx(xvalue objValue, const char* sNamespace, const char* sTag, int64 iTTL)
{
	int64 iID;
	xvalue objStore;
	int64* pTime;
	int64* pExpire;
	int64 iTTLSecond = 0;
	char** ppNamespace;
	char** ppTag;
	
	XS_BusClearLastError();
	
	if ( objValue == NULL ) {
		XS_BusSetLastError(XS_BUS_ERR_INVALID_VALUE, "value is null");
		XS_LogWarn("bus register failed: value is null");
		return 0;
	}
	if ( XS_BusIsReservedNamespace(sNamespace) ) {
		XS_BusSetLastError(XS_BUS_ERR_RESERVED_NAMESPACE, "reserved namespace");
		XS_LogWarn("bus register failed: reserved namespace: %s", sNamespace);
		return 0;
	}
	
	if ( iTTL > 0 ) {
		iTTLSecond = (iTTL + 999) / 1000;
		if ( iTTLSecond <= 0 ) {
			iTTLSecond = 1;
		}
	}
	
	objStore = xvoDeepCopy(objValue);
	if ( objStore == NULL ) {
		XS_BusSetLastError(XS_BUS_ERR_DEEP_COPY, "deep copy error");
		XS_LogWarn("bus register failed: deep copy error");
		return 0;
	}
	
	if ( !XS_BusPublishValue(objStore) ) {
		XS_BusSetLastError(XS_BUS_ERR_PUBLISH, "publish error");
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
		XS_BusSetLastError(XS_BUS_ERR_DATA_SLOT, "data slot create error");
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
			XS_BusSetLastError(XS_BUS_ERR_REF_SLOT, "ref slot create error");
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
		XS_BusSetLastError(XS_BUS_ERR_CREATE_SLOT, "create slot error");
		(void)xrtListRemove(g_objXsBus.pRefList, iID);
		(void)xrtListRemovePtr(g_objXsBus.pDataList, iID);
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return 0;
	}
	*pTime = xrtNow();
	ppNamespace = (char**)xrtListSet(g_objXsBus.pNamespaceList, iID, NULL);
	if ( ppNamespace == NULL ) {
		XS_BusSetLastError(XS_BUS_ERR_NAMESPACE_SLOT, "namespace slot error");
		(void)xrtListRemove(g_objXsBus.pCreateList, iID);
		(void)xrtListRemove(g_objXsBus.pRefList, iID);
		(void)xrtListRemovePtr(g_objXsBus.pDataList, iID);
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return 0;
	}
	ppTag = (char**)xrtListSet(g_objXsBus.pTagList, iID, NULL);
	if ( ppTag == NULL ) {
		XS_BusSetLastError(XS_BUS_ERR_TAG_SLOT, "tag slot error");
		(void)xrtListRemovePtr(g_objXsBus.pNamespaceList, iID);
		(void)xrtListRemove(g_objXsBus.pCreateList, iID);
		(void)xrtListRemove(g_objXsBus.pRefList, iID);
		(void)xrtListRemovePtr(g_objXsBus.pDataList, iID);
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return 0;
	}
	pExpire = (int64*)xrtListSet(g_objXsBus.pExpireList, iID, NULL);
	if ( pExpire == NULL ) {
		XS_BusSetLastError(XS_BUS_ERR_EXPIRE_SLOT, "expire slot error");
		(void)xrtListRemovePtr(g_objXsBus.pTagList, iID);
		(void)xrtListRemovePtr(g_objXsBus.pNamespaceList, iID);
		(void)xrtListRemove(g_objXsBus.pCreateList, iID);
		(void)xrtListRemove(g_objXsBus.pRefList, iID);
		(void)xrtListRemovePtr(g_objXsBus.pDataList, iID);
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return 0;
	}
	*ppNamespace = XS_BusCopyText(sNamespace);
	*ppTag = XS_BusCopyText(sTag);
	*pExpire = (iTTLSecond > 0) ? (xrtNow() + iTTLSecond) : 0;
	XS_BusClearLastError();
	XS_LogInfo("bus data registered: id=%lld type=%d", (long long)iID, xvoType(objStore));
	xrtMutexUnlock(g_objXsBus.pLock);
	return iID;
}

static inline int64 XS_BusDataRegister(xvalue objValue)
{
	return XS_BusDataRegisterEx(objValue, NULL, NULL, 0);
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
		char* sNamespace = NULL;
		char* sTag = NULL;
		
		sNamespace = (char*)xrtListRemovePtr(g_objXsBus.pNamespaceList, iID);
		sTag = (char*)xrtListRemovePtr(g_objXsBus.pTagList, iID);
		(void)xrtListRemove(g_objXsBus.pExpireList, iID);
		(void)xrtListRemove(g_objXsBus.pCreateList, iID);
		(void)xrtListRemove(g_objXsBus.pRefList, iID);
		(void)xrtListRemovePtr(g_objXsBus.pDataList, iID);
		xrtMutexUnlock(g_objXsBus.pLock);
		if ( sTag ) {
			xrtFree(sTag);
		}
		if ( sNamespace ) {
			xrtFree(sNamespace);
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
		char* sNamespace = (char*)xrtListRemovePtr(g_objXsBus.pNamespaceList, iID);
		if ( sNamespace ) {
			xrtFree(sNamespace);
		}
	}
	{
		char* sTag = (char*)xrtListRemovePtr(g_objXsBus.pTagList, iID);
		if ( sTag ) {
			xrtFree(sTag);
		}
	}
	(void)xrtListRemove(g_objXsBus.pExpireList, iID);
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
	int64* pExpire;
	char* sNamespace;
	char* sTag;
	
	if ( objArr == NULL || pVal == NULL ) {
		return FALSE;
	}
	
	objItem = xvoCreateTable();
	pRef = (int64*)xrtListGet(g_objXsBus.pRefList, iKey);
	pCreate = (int64*)xrtListGet(g_objXsBus.pCreateList, iKey);
	pExpire = (int64*)xrtListGet(g_objXsBus.pExpireList, iKey);
	sNamespace = (char*)xrtListGetPtr(g_objXsBus.pNamespaceList, iKey);
	sTag = (char*)xrtListGetPtr(g_objXsBus.pTagList, iKey);
	
	xvoTableSetInt(objItem, "id", 2, iKey);
	xvoTableSetInt(objItem, "type", 4, xvoType((xvalue)pVal));
	xvoTableSetInt(objItem, "ref_count", 9, pRef ? *pRef : 0);
	xvoTableSetInt(objItem, "create_time", 11, pCreate ? *pCreate : 0);
	xvoTableSetInt(objItem, "expire_time", 11, pExpire ? *pExpire : 0);
	xvoTableSetText(objItem, "namespace", 9, sNamespace ? sNamespace : "", 0, FALSE);
	xvoTableSetText(objItem, "tag", 3, sTag ? sTag : "", 0, FALSE);
	xvoArrayAppendValue(objArr, objItem, TRUE);
	return FALSE;
}

typedef struct {
	int64 arrID[256];
	uint32 iCount;
	int64 tNow;
} XS_BusSweepContext;

typedef struct {
	const char* sNamespace;
	const char* sTag;
	xvalue objItems;
	int64 iCount;
} XS_BusFilterContext;

typedef struct {
	xvalue objItems;
	int64 iCount;
} XS_BusNamespaceStatsContext;

static bool XS_BusSweepCollectProc(int64 iKey, ptr pVal, XS_BusSweepContext* objCtx)
{
	int64* pExpire;
	(void)pVal;
	
	if ( objCtx == NULL || objCtx->iCount >= 256 ) {
		return FALSE;
	}
	
	pExpire = (int64*)xrtListGet(g_objXsBus.pExpireList, iKey);
	if ( pExpire && *pExpire > 0 && *pExpire <= objCtx->tNow ) {
		objCtx->arrID[objCtx->iCount++] = iKey;
	}
	return FALSE;
}

static inline void XS_BusSweepExpiredData(void)
{
	XS_BusSweepContext objCtx;
	uint32 i;
	
	memset(&objCtx, 0, sizeof(objCtx));
	objCtx.tNow = xrtNow();
	
	xrtMutexLock(g_objXsBus.pLock);
	if ( g_objXsBus.pDataList ) {
		xrtListWalk(g_objXsBus.pDataList, (List_EachProc)XS_BusSweepCollectProc, &objCtx);
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	
	for ( i = 0; i < objCtx.iCount; i++ ) {
		XS_LogInfo("bus data expired: id=%lld", (long long)objCtx.arrID[i]);
		XS_BusDataRemove(objCtx.arrID[i]);
	}
}

static inline xvalue XS_BusBuildStatusValue(void)
{
	xvalue objRet = xvoCreateTable();
	xvalue objItems = xvoCreateArray();
	
	XS_BusSweepExpiredData();
	
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

static bool XS_BusBuildStatusFilterProc(int64 iKey, ptr pVal, XS_BusFilterContext* objCtx)
{
	const char* sItemNamespace;
	const char* sItemTag;
	
	if ( objCtx == NULL || objCtx->objItems == NULL || pVal == NULL ) {
		return FALSE;
	}
	
	sItemNamespace = (const char*)xrtListGetPtr(g_objXsBus.pNamespaceList, iKey);
	sItemTag = (const char*)xrtListGetPtr(g_objXsBus.pTagList, iKey);
	
	if ( objCtx->sNamespace && objCtx->sNamespace[0] != '\0' ) {
		if ( sItemNamespace == NULL || strcmp(sItemNamespace, objCtx->sNamespace) != 0 ) {
			return FALSE;
		}
	}
	if ( objCtx->sTag && objCtx->sTag[0] != '\0' ) {
		if ( sItemTag == NULL || strcmp(sItemTag, objCtx->sTag) != 0 ) {
			return FALSE;
		}
	}
	
	objCtx->iCount++;
	return XS_BusBuildStatusProc(iKey, pVal, objCtx->objItems);
}

static inline xvalue XS_BusBuildStatusValueEx(const char* sNamespace, const char* sTag)
{
	xvalue objRet = xvoCreateTable();
	xvalue objItems = xvoCreateArray();
	XS_BusFilterContext objCtx;
	
	memset(&objCtx, 0, sizeof(objCtx));
	objCtx.sNamespace = sNamespace;
	objCtx.sTag = sTag;
	objCtx.objItems = objItems;
	
	XS_BusSweepExpiredData();
	
	xrtMutexLock(g_objXsBus.pLock);
	xvoTableSetInt(objRet, "queue_count", 11, g_objXsBus.arrMessageQueue ? g_objXsBus.arrMessageQueue->Count : 0);
	xvoTableSetInt(objRet, "data_count", 10, g_objXsBus.pDataList ? xrtListCount(g_objXsBus.pDataList) : 0);
	xvoTableSetInt(objRet, "next_data_id", 12, g_objXsBus.iNextDataID);
	xvoTableSetInt(objRet, "next_msg_id", 11, g_objXsBus.iNextMsgID);
	xvoTableSetText(objRet, "namespace", 9, (ptr)(sNamespace ? sNamespace : ""), 0, FALSE);
	xvoTableSetText(objRet, "tag", 3, (ptr)(sTag ? sTag : ""), 0, FALSE);
	if ( g_objXsBus.pDataList ) {
		xrtListWalk(g_objXsBus.pDataList, (List_EachProc)XS_BusBuildStatusFilterProc, &objCtx);
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	
	xvoTableSetInt(objRet, "match_count", 11, objCtx.iCount);
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

static inline char* XS_BusBuildStatusJsonEx(const char* sNamespace, const char* sTag)
{
	xvalue objStatus = XS_BusBuildStatusValueEx(sNamespace, sTag);
	char* sRet = NULL;
	
	if ( objStatus == NULL ) {
		return NULL;
	}
	
	sRet = xrtStringifyJSON(objStatus, FALSE, NULL);
	xvoUnref(objStatus);
	return sRet;
}

static bool XS_BusBuildNamespaceStatsProc(int64 iKey, ptr pVal, XS_BusNamespaceStatsContext* objCtx)
{
	const char* sNamespace;
	uint32 i;
	xvalue objItem;
	int64 iNamespaceCount;
	(void)iKey;
	(void)pVal;
	
	if ( objCtx == NULL || objCtx->objItems == NULL ) {
		return FALSE;
	}
	
	sNamespace = (const char*)xrtListGetPtr(g_objXsBus.pNamespaceList, iKey);
	if ( sNamespace == NULL || sNamespace[0] == '\0' ) {
		sNamespace = "(default)";
	}
	
	for ( i = 1; i <= xvoArrayItemCount(objCtx->objItems); i++ ) {
		xvalue objExist = xvoArrayGetValue(objCtx->objItems, i);
		const char* sExistNamespace;
		
		if ( objExist == NULL ) {
			continue;
		}
		
		sExistNamespace = xvoGetText(xvoTableGetValue(objExist, "namespace", 9));
		if ( sExistNamespace && strcmp(sExistNamespace, sNamespace) == 0 ) {
			iNamespaceCount = xvoGetInt(xvoTableGetValue(objExist, "count", 5));
			xvoTableSetInt(objExist, "count", 5, iNamespaceCount + 1);
			return FALSE;
		}
	}
	
	objItem = xvoCreateTable();
	xvoTableSetText(objItem, "namespace", 9, (ptr)sNamespace, 0, FALSE);
	xvoTableSetInt(objItem, "count", 5, 1);
	xvoArrayAppendValue(objCtx->objItems, objItem, TRUE);
	objCtx->iCount++;
	return FALSE;
}

static inline xvalue XS_BusBuildNamespaceStatsValue(void)
{
	xvalue objRet = xvoCreateTable();
	xvalue objItems = xvoCreateArray();
	XS_BusNamespaceStatsContext objCtx;
	
	memset(&objCtx, 0, sizeof(objCtx));
	objCtx.objItems = objItems;
	
	XS_BusSweepExpiredData();
	
	xrtMutexLock(g_objXsBus.pLock);
	xvoTableSetInt(objRet, "queue_count", 11, g_objXsBus.arrMessageQueue ? g_objXsBus.arrMessageQueue->Count : 0);
	xvoTableSetInt(objRet, "data_count", 10, g_objXsBus.pDataList ? xrtListCount(g_objXsBus.pDataList) : 0);
	if ( g_objXsBus.pDataList ) {
		xrtListWalk(g_objXsBus.pDataList, (List_EachProc)XS_BusBuildNamespaceStatsProc, &objCtx);
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	
	xvoTableSetInt(objRet, "namespace_count", 15, objCtx.iCount);
	xvoTableSetValue(objRet, "items", 5, objItems, TRUE);
	return objRet;
}

static inline char* XS_BusBuildNamespaceStatsJson(void)
{
	xvalue objStatus = XS_BusBuildNamespaceStatsValue();
	char* sRet = NULL;
	
	if ( objStatus == NULL ) {
		return NULL;
	}
	
	sRet = xrtStringifyJSON(objStatus, FALSE, NULL);
	xvoUnref(objStatus);
	return sRet;
}

static inline int64 XS_BusDataFindFirst(const char* sNamespace, const char* sTag)
{
	int64 iID;
	
	xrtMutexLock(g_objXsBus.pLock);
	for ( iID = 1; iID < g_objXsBus.iNextDataID; iID++ ) {
		xvalue objVal = (xvalue)xrtListGetPtr(g_objXsBus.pDataList, iID);
		const char* sItemNamespace;
		const char* sItemTag;
		
		if ( objVal == NULL ) {
			continue;
		}
		
		sItemNamespace = (const char*)xrtListGetPtr(g_objXsBus.pNamespaceList, iID);
		sItemTag = (const char*)xrtListGetPtr(g_objXsBus.pTagList, iID);
		
		if ( sNamespace && sNamespace[0] != '\0' ) {
			if ( sItemNamespace == NULL || strcmp(sItemNamespace, sNamespace) != 0 ) {
				continue;
			}
		}
		if ( sTag && sTag[0] != '\0' ) {
			if ( sItemTag == NULL || strcmp(sItemTag, sTag) != 0 ) {
				continue;
			}
		}
		
		xrtMutexUnlock(g_objXsBus.pLock);
		return iID;
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	return 0;
}

static inline int64 XS_BusDataRemoveByQuery(const char* sNamespace, const char* sTag, int32 iLimit)
{
	int64 arrID[256];
	int64 iID;
	int64 iRemoved = 0;
	uint32 iCount = 0;
	
	if ( iLimit <= 0 || iLimit > 256 ) {
		iLimit = 256;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	for ( iID = 1; iID < g_objXsBus.iNextDataID; iID++ ) {
		xvalue objVal = (xvalue)xrtListGetPtr(g_objXsBus.pDataList, iID);
		const char* sItemNamespace;
		const char* sItemTag;
		
		if ( objVal == NULL ) {
			continue;
		}
		
		sItemNamespace = (const char*)xrtListGetPtr(g_objXsBus.pNamespaceList, iID);
		sItemTag = (const char*)xrtListGetPtr(g_objXsBus.pTagList, iID);
		
		if ( sNamespace && sNamespace[0] != '\0' ) {
			if ( sItemNamespace == NULL || strcmp(sItemNamespace, sNamespace) != 0 ) {
				continue;
			}
		}
		if ( sTag && sTag[0] != '\0' ) {
			if ( sItemTag == NULL || strcmp(sItemTag, sTag) != 0 ) {
				continue;
			}
		}
		
		arrID[iCount++] = iID;
		if ( iCount >= (uint32)iLimit ) {
			break;
		}
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	
	for ( iID = 0; iID < (int64)iCount; iID++ ) {
		if ( XS_BusDataRemove(arrID[iID]) ) {
			iRemoved++;
		}
	}
	
	return iRemoved;
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

static inline bool XS_BusBroadcast(const char* sTopic, int64 iDataID, xvalue objArgs)
{
	return XS_BusQueueMessage(XS_MSG_TARGET_BROADCAST, NULL, NULL, sTopic, iDataID, objArgs);
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
			} else if ( objMsg.iTargetType == XS_MSG_TARGET_SERVER || objMsg.iTargetType == XS_MSG_TARGET_BROADCAST ) {
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
