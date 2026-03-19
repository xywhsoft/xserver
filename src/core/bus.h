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
	xarray arrDataItems;
	xarray arrMessageQueue;
	int64 iNextDataID;
	int64 iNextMsgID;
	int64 iTotalQueued;
	int64 iTotalDelivered;
	int64 iTotalDropped;
	int64 tLastQueued;
	int64 tLastDispatch;
} XS_Bus;

typedef struct {
	int64 iID;
	xvalue objValue;
	int64 iRefCount;
	char* sNamespace;
	char* sTag;
	int64 tCreate;
	int64 tExpire;
} XS_BusDataItem;

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

static inline int64 XS_BusTTLToSeconds(int64 iTTL)
{
	int64 iTTLSecond = 0;

	if ( iTTL > 0 ) {
		iTTLSecond = (iTTL + 999) / 1000;
		if ( iTTLSecond <= 0 ) {
			iTTLSecond = 1;
		}
	}

	return iTTLSecond;
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
static inline void XS_BusSweepExpiredData(void);
static inline int64 XS_BusDataFindFirst(const char* sNamespace, const char* sTag);
static inline bool XS_BusDataSetTTL(int64 iID, int64 iTTL);

static inline XS_BusDataItem* XS_BusFindDataItemByID_NoLock(int64 iID)
{
	uint32 i;

	if ( iID <= 0 || g_objXsBus.arrDataItems == NULL ) {
		return NULL;
	}

	for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
		XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);

		if ( objItem && objItem->iID == iID ) {
			return objItem;
		}
	}

	return NULL;
}

static inline bool XS_BusMatchDataItem(const XS_BusDataItem* objItem, const char* sNamespace, const char* sTag)
{
	if ( objItem == NULL || objItem->iID <= 0 || objItem->objValue == NULL ) {
		return FALSE;
	}

	if ( sNamespace && sNamespace[0] != '\0' ) {
		if ( objItem->sNamespace == NULL || strcmp(objItem->sNamespace, sNamespace) != 0 ) {
			return FALSE;
		}
	}

	if ( sTag && sTag[0] != '\0' ) {
		if ( objItem->sTag == NULL || strcmp(objItem->sTag, sTag) != 0 ) {
			return FALSE;
		}
	}

	return TRUE;
}

static inline void XS_BusFreeDataItem(XS_BusDataItem* objItem)
{
	if ( objItem == NULL ) {
		return;
	}

	if ( objItem->sNamespace ) {
		xrtFree(objItem->sNamespace);
	}
	if ( objItem->sTag ) {
		xrtFree(objItem->sTag);
	}
	if ( objItem->objValue ) {
		xvoUnref(objItem->objValue);
	}
	memset(objItem, 0, sizeof(XS_BusDataItem));
}

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
	g_objXsBus.arrDataItems = xrtArrayCreate(sizeof(XS_BusDataItem), XRT_OBJMODE_SHARED);
	g_objXsBus.arrMessageQueue = xrtArrayCreate(sizeof(XS_Message), XRT_OBJMODE_SHARED);
	g_objXsBus.iNextDataID = 1;
	g_objXsBus.iNextMsgID = 1;
	
	if ( g_objXsBus.pLock == NULL || g_objXsBus.arrDataItems == NULL || g_objXsBus.arrMessageQueue == NULL ) {
		return FALSE;
	}
	
	xrtOwnerActivateShared(&g_objXsBus.arrDataItems->Owner);
	xrtOwnerActivateShared(&g_objXsBus.arrMessageQueue->Owner);
	
	return TRUE;
}

static inline void XS_BusClearStats(void)
{
	xrtMutexLock(g_objXsBus.pLock);
	g_objXsBus.iTotalQueued = 0;
	g_objXsBus.iTotalDelivered = 0;
	g_objXsBus.iTotalDropped = 0;
	g_objXsBus.tLastQueued = 0;
	g_objXsBus.tLastDispatch = 0;
	xrtMutexUnlock(g_objXsBus.pLock);
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
	if ( g_objXsBus.arrDataItems ) {
		for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
			XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);
			XS_BusFreeDataItem(objItem);
		}
		xrtArrayDestroy(g_objXsBus.arrDataItems);
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
	int64 iTTLSecond = 0;
	uint32 iPos;
	XS_BusDataItem* objItem;
	
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
	
	iTTLSecond = XS_BusTTLToSeconds(iTTL);
	
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
	iPos = xrtArrayAppend(g_objXsBus.arrDataItems, 1);
	if ( iPos == 0 ) {
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
	objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, iPos);
	memset(objItem, 0, sizeof(XS_BusDataItem));
	objItem->iID = iID;
	objItem->objValue = objStore;
	objItem->iRefCount = 1;
	objItem->tCreate = xrtNow();
	objItem->tExpire = (iTTLSecond > 0) ? (objItem->tCreate + iTTLSecond) : 0;
	objItem->sNamespace = XS_BusCopyText(sNamespace);
	objItem->sTag = XS_BusCopyText(sTag);
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
	xvalue objRet = NULL;
	XS_BusDataItem* objItem;
	
	if ( iID <= 0 ) {
		return NULL;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	objItem = XS_BusFindDataItemByID_NoLock(iID);
	if ( objItem ) {
		objRet = objItem->objValue;
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	return objRet;
}

static inline xvalue XS_BusBuildDataValue(int64 iID)
{
	xvalue objRet = NULL;
	XS_BusDataItem* objItem;
	xvalue objDataCopy = NULL;

	if ( iID <= 0 ) {
		return NULL;
	}

	XS_BusSweepExpiredData();

	xrtMutexLock(g_objXsBus.pLock);
	objItem = XS_BusFindDataItemByID_NoLock(iID);
	if ( objItem && objItem->objValue ) {
		objDataCopy = xvoDeepCopy(objItem->objValue);
		if ( objDataCopy ) {
			objRet = xvoCreateTable();
			xvoTableSetInt(objRet, "id", 2, objItem->iID);
			xvoTableSetInt(objRet, "type", 4, xvoType(objItem->objValue));
			xvoTableSetInt(objRet, "ref_count", 9, objItem->iRefCount);
			xvoTableSetInt(objRet, "create_time", 11, objItem->tCreate);
			xvoTableSetInt(objRet, "expire_time", 11, objItem->tExpire);
			xvoTableSetText(objRet, "namespace", 9, objItem->sNamespace ? objItem->sNamespace : "", 0, FALSE);
			xvoTableSetText(objRet, "tag", 3, objItem->sTag ? objItem->sTag : "", 0, FALSE);
			xvoTableSetValue(objRet, "data", 4, objDataCopy, TRUE);
		}
	}
	xrtMutexUnlock(g_objXsBus.pLock);

	return objRet;
}

static inline int64 XS_BusDataResolveID(const char* sNamespace, const char* sTag, int64 iID)
{
	if ( iID > 0 ) {
		return iID;
	}

	if ( ((sNamespace && sNamespace[0] != '\0')) || ((sTag && sTag[0] != '\0')) ) {
		return XS_BusDataFindFirst(sNamespace, sTag);
	}

	return 0;
}

static inline char* XS_BusBuildDataJson(int64 iID)
{
	xvalue objData = XS_BusBuildDataValue(iID);
	char* sRet;

	if ( objData == NULL ) {
		return NULL;
	}

	sRet = xrtStringifyJSON(objData, FALSE, NULL);
	xvoUnref(objData);
	return sRet;
}

typedef struct {
	const char* sNamespace;
	const char* sTag;
	xvalue objItems;
	int64 iCount;
} XS_BusDataFilterContext;

static bool XS_BusBuildDataFilterProc(const XS_BusDataItem* objItem, XS_BusDataFilterContext* objCtx)
{
	xvalue objDataCopy;
	xvalue objRow;

	if ( objCtx == NULL || objCtx->objItems == NULL || objItem == NULL || objItem->objValue == NULL ) {
		return FALSE;
	}
	if ( !XS_BusMatchDataItem(objItem, objCtx->sNamespace, objCtx->sTag) ) {
		return FALSE;
	}

	objDataCopy = xvoDeepCopy(objItem->objValue);
	if ( objDataCopy == NULL ) {
		return FALSE;
	}

	objRow = xvoCreateTable();
	xvoTableSetInt(objRow, "id", 2, objItem->iID);
	xvoTableSetInt(objRow, "type", 4, xvoType(objItem->objValue));
	xvoTableSetInt(objRow, "ref_count", 9, objItem->iRefCount);
	xvoTableSetInt(objRow, "create_time", 11, objItem->tCreate);
	xvoTableSetInt(objRow, "expire_time", 11, objItem->tExpire);
	xvoTableSetText(objRow, "namespace", 9, objItem->sNamespace ? objItem->sNamespace : "", 0, FALSE);
	xvoTableSetText(objRow, "tag", 3, objItem->sTag ? objItem->sTag : "", 0, FALSE);
	xvoTableSetValue(objRow, "data", 4, objDataCopy, TRUE);
	xvoArrayAppendValue(objCtx->objItems, objRow, TRUE);
	objCtx->iCount++;
	return FALSE;
}

static inline xvalue XS_BusBuildDataListValueEx(const char* sNamespace, const char* sTag)
{
	xvalue objRet = xvoCreateTable();
	xvalue objItems = xvoCreateArray();
	XS_BusDataFilterContext objCtx;

	memset(&objCtx, 0, sizeof(objCtx));
	objCtx.sNamespace = sNamespace;
	objCtx.sTag = sTag;
	objCtx.objItems = objItems;

	XS_BusSweepExpiredData();

	xrtMutexLock(g_objXsBus.pLock);
	xvoTableSetInt(objRet, "queue_count", 11, g_objXsBus.arrMessageQueue ? g_objXsBus.arrMessageQueue->Count : 0);
	xvoTableSetInt(objRet, "data_count", 10, g_objXsBus.arrDataItems ? g_objXsBus.arrDataItems->Count : 0);
	xvoTableSetInt(objRet, "next_data_id", 12, g_objXsBus.iNextDataID);
	xvoTableSetInt(objRet, "next_msg_id", 11, g_objXsBus.iNextMsgID);
	xvoTableSetText(objRet, "namespace", 9, (ptr)(sNamespace ? sNamespace : ""), 0, FALSE);
	xvoTableSetText(objRet, "tag", 3, (ptr)(sTag ? sTag : ""), 0, FALSE);
	if ( g_objXsBus.arrDataItems ) {
		uint32 i;

		for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
			XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);
			(void)XS_BusBuildDataFilterProc(objItem, &objCtx);
		}
	}
	xrtMutexUnlock(g_objXsBus.pLock);

	xvoTableSetInt(objRet, "match_count", 11, objCtx.iCount);
	xvoTableSetValue(objRet, "items", 5, objItems, TRUE);
	return objRet;
}

static inline char* XS_BusBuildDataListJsonEx(const char* sNamespace, const char* sTag)
{
	xvalue objStatus = XS_BusBuildDataListValueEx(sNamespace, sTag);
	char* sRet = NULL;

	if ( objStatus == NULL ) {
		return NULL;
	}

	sRet = xrtStringifyJSON(objStatus, FALSE, NULL);
	xvoUnref(objStatus);
	return sRet;
}

static inline bool XS_BusDataRetain(int64 iID)
{
	XS_BusDataItem* objItem;
	
	if ( iID <= 0 ) {
		return FALSE;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	objItem = XS_BusFindDataItemByID_NoLock(iID);
	if ( objItem == NULL || objItem->objValue == NULL ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		return FALSE;
	}
	objItem->iRefCount++;
	xrtMutexUnlock(g_objXsBus.pLock);
	return TRUE;
}

static inline bool XS_BusDataTouchTTL(int64 iID, int64 iTTL)
{
	if ( iTTL <= 0 ) {
		return FALSE;
	}

	return XS_BusDataSetTTL(iID, iTTL);
}

static inline bool XS_BusDataSetTTL(int64 iID, int64 iTTL)
{
	XS_BusDataItem* objItem;
	int64 iTTLSecond;

	if ( iID <= 0 ) {
		return FALSE;
	}

	iTTLSecond = XS_BusTTLToSeconds(iTTL);

	xrtMutexLock(g_objXsBus.pLock);
	objItem = XS_BusFindDataItemByID_NoLock(iID);
	if ( objItem == NULL || objItem->objValue == NULL ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		return FALSE;
	}
	objItem->tExpire = (iTTLSecond > 0) ? (xrtNow() + iTTLSecond) : 0;
	xrtMutexUnlock(g_objXsBus.pLock);
	return TRUE;
}

static inline bool XS_BusDataSetValue(int64 iID, xvalue objValue)
{
	XS_BusDataItem* objItem;
	xvalue objStore;
	xvalue objOld;

	if ( iID <= 0 || objValue == NULL ) {
		return FALSE;
	}

	objStore = xvoDeepCopy(objValue);
	if ( objStore == NULL ) {
		return FALSE;
	}
	if ( !XS_BusPublishValue(objStore) ) {
		xvoUnref(objStore);
		return FALSE;
	}

	xrtMutexLock(g_objXsBus.pLock);
	objItem = XS_BusFindDataItemByID_NoLock(iID);
	if ( objItem == NULL || objItem->objValue == NULL ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return FALSE;
	}
	objOld = objItem->objValue;
	objItem->objValue = objStore;
	xrtMutexUnlock(g_objXsBus.pLock);

	if ( objOld ) {
		xvoUnref(objOld);
	}
	return TRUE;
}


static inline int64 XS_BusDataGetExpireTime(int64 iID)
{
	int64 tExpire = 0;
	XS_BusDataItem* objItem;

	if ( iID <= 0 ) {
		return 0;
	}

	xrtMutexLock(g_objXsBus.pLock);
	objItem = XS_BusFindDataItemByID_NoLock(iID);
	if ( objItem && objItem->objValue ) {
		tExpire = objItem->tExpire;
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	return tExpire;
}

static inline bool XS_BusDataRelease(int64 iID)
{
	uint32 i;
	XS_BusDataItem objItem;
	
	if ( iID <= 0 ) {
		return FALSE;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
		XS_BusDataItem* pItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);

		if ( pItem && pItem->iID == iID && pItem->objValue ) {
			if ( pItem->iRefCount > 1 ) {
				pItem->iRefCount--;
				xrtMutexUnlock(g_objXsBus.pLock);
				return TRUE;
			}

			memcpy(&objItem, pItem, sizeof(XS_BusDataItem));
			memset(pItem, 0, sizeof(XS_BusDataItem));
			(void)xrtArrayRemove(g_objXsBus.arrDataItems, i, 1);
			xrtMutexUnlock(g_objXsBus.pLock);
			XS_BusFreeDataItem(&objItem);
			return TRUE;
		}
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	return FALSE;
}

static inline bool XS_BusDataRemove(int64 iID)
{
	uint32 i;
	XS_BusDataItem objItem;

	if ( iID <= 0 ) {
		return FALSE;
	}

	memset(&objItem, 0, sizeof(objItem));

	xrtMutexLock(g_objXsBus.pLock);
	for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
		XS_BusDataItem* pItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);

		if ( pItem && pItem->iID == iID && pItem->objValue ) {
			memcpy(&objItem, pItem, sizeof(XS_BusDataItem));
			memset(pItem, 0, sizeof(XS_BusDataItem));
			(void)xrtArrayRemove(g_objXsBus.arrDataItems, i, 1);
			xrtMutexUnlock(g_objXsBus.pLock);
			XS_BusFreeDataItem(&objItem);
			return TRUE;
		}
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	return FALSE;
}

static bool XS_BusBuildStatusProc(const XS_BusDataItem* objData, xvalue objArr)
{
	xvalue objItem;
	
	if ( objArr == NULL || objData == NULL || objData->objValue == NULL ) {
		return FALSE;
	}
	
	objItem = xvoCreateTable();
	
	xvoTableSetInt(objItem, "id", 2, objData->iID);
	xvoTableSetInt(objItem, "type", 4, xvoType(objData->objValue));
	xvoTableSetInt(objItem, "ref_count", 9, objData->iRefCount);
	xvoTableSetInt(objItem, "create_time", 11, objData->tCreate);
	xvoTableSetInt(objItem, "expire_time", 11, objData->tExpire);
	xvoTableSetText(objItem, "namespace", 9, objData->sNamespace ? objData->sNamespace : "", 0, FALSE);
	xvoTableSetText(objItem, "tag", 3, objData->sTag ? objData->sTag : "", 0, FALSE);
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

static bool XS_BusSweepCollectProc(const XS_BusDataItem* objItem, XS_BusSweepContext* objCtx)
{
	if ( objCtx == NULL || objCtx->iCount >= 256 ) {
		return FALSE;
	}
	
	if ( objItem && objItem->iID > 0 && objItem->objValue && objItem->tExpire > 0 && objItem->tExpire <= objCtx->tNow ) {
		objCtx->arrID[objCtx->iCount++] = objItem->iID;
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
	if ( g_objXsBus.arrDataItems ) {
		for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
			XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);
			(void)XS_BusSweepCollectProc(objItem, &objCtx);
		}
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	
	for ( i = 0; i < objCtx.iCount; i++ ) {
		XS_LogInfo("bus data expired: id=%lld", (long long)objCtx.arrID[i]);
		XS_BusDataRemove(objCtx.arrID[i]);
	}
}

static inline void XS_BusSetTimeFields(xvalue objRet, int64 tQueue, int64 tDispatch)
{
	char* sQueueTime;
	char* sDispatchTime;

	if ( objRet == NULL ) {
		return;
	}

	sQueueTime = (tQueue > 0) ? xrtTimeToStr(tQueue, XRT_TIME_FORMAT_DATETIME) : NULL;
	sDispatchTime = (tDispatch > 0) ? xrtTimeToStr(tDispatch, XRT_TIME_FORMAT_DATETIME) : NULL;
	xvoTableSetText(objRet, "last_queue_time_text", 20, (ptr)(sQueueTime ? sQueueTime : ""), 0, FALSE);
	xvoTableSetText(objRet, "last_dispatch_time_text", 23, (ptr)(sDispatchTime ? sDispatchTime : ""), 0, FALSE);
	if ( sQueueTime ) {
		xrtFree(sQueueTime);
	}
	if ( sDispatchTime ) {
		xrtFree(sDispatchTime);
	}
}

static inline xvalue XS_BusBuildStatusValue(void)
{
	xvalue objRet = xvoCreateTable();
	xvalue objItems = xvoCreateArray();
	
	XS_BusSweepExpiredData();
	
	xrtMutexLock(g_objXsBus.pLock);
	xvoTableSetInt(objRet, "queue_count", 11, g_objXsBus.arrMessageQueue ? g_objXsBus.arrMessageQueue->Count : 0);
	xvoTableSetInt(objRet, "data_count", 10, g_objXsBus.arrDataItems ? g_objXsBus.arrDataItems->Count : 0);
	xvoTableSetInt(objRet, "next_data_id", 12, g_objXsBus.iNextDataID);
	xvoTableSetInt(objRet, "next_msg_id", 11, g_objXsBus.iNextMsgID);
	xvoTableSetInt(objRet, "total_queued", 12, g_objXsBus.iTotalQueued);
	xvoTableSetInt(objRet, "total_delivered", 15, g_objXsBus.iTotalDelivered);
	xvoTableSetInt(objRet, "total_dropped", 13, g_objXsBus.iTotalDropped);
	xvoTableSetInt(objRet, "last_queue_time", 15, g_objXsBus.tLastQueued);
	xvoTableSetInt(objRet, "last_dispatch_time", 18, g_objXsBus.tLastDispatch);
	XS_BusSetTimeFields(objRet, g_objXsBus.tLastQueued, g_objXsBus.tLastDispatch);
	if ( g_objXsBus.arrDataItems ) {
		uint32 i;

		for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
			XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);
			(void)XS_BusBuildStatusProc(objItem, objItems);
		}
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	xvoTableSetValue(objRet, "items", 5, objItems, TRUE);
	return objRet;
}

static bool XS_BusBuildStatusFilterProc(const XS_BusDataItem* objItem, XS_BusFilterContext* objCtx)
{
	if ( objCtx == NULL || objCtx->objItems == NULL || objItem == NULL || objItem->objValue == NULL ) {
		return FALSE;
	}
	if ( !XS_BusMatchDataItem(objItem, objCtx->sNamespace, objCtx->sTag) ) {
		return FALSE;
	}
	
	objCtx->iCount++;
	return XS_BusBuildStatusProc(objItem, objCtx->objItems);
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
	xvoTableSetInt(objRet, "data_count", 10, g_objXsBus.arrDataItems ? g_objXsBus.arrDataItems->Count : 0);
	xvoTableSetInt(objRet, "next_data_id", 12, g_objXsBus.iNextDataID);
	xvoTableSetInt(objRet, "next_msg_id", 11, g_objXsBus.iNextMsgID);
	xvoTableSetInt(objRet, "total_queued", 12, g_objXsBus.iTotalQueued);
	xvoTableSetInt(objRet, "total_delivered", 15, g_objXsBus.iTotalDelivered);
	xvoTableSetInt(objRet, "total_dropped", 13, g_objXsBus.iTotalDropped);
	xvoTableSetInt(objRet, "last_queue_time", 15, g_objXsBus.tLastQueued);
	xvoTableSetInt(objRet, "last_dispatch_time", 18, g_objXsBus.tLastDispatch);
	XS_BusSetTimeFields(objRet, g_objXsBus.tLastQueued, g_objXsBus.tLastDispatch);
	xvoTableSetText(objRet, "namespace", 9, (ptr)(sNamespace ? sNamespace : ""), 0, FALSE);
	xvoTableSetText(objRet, "tag", 3, (ptr)(sTag ? sTag : ""), 0, FALSE);
	if ( g_objXsBus.arrDataItems ) {
		uint32 i;

		for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
			XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);
			(void)XS_BusBuildStatusFilterProc(objItem, &objCtx);
		}
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

static bool XS_BusBuildNamespaceStatsProc(const XS_BusDataItem* objData, XS_BusNamespaceStatsContext* objCtx)
{
	const char* sNamespace;
	uint32 i;
	xvalue objItem;
	int64 iNamespaceCount;
	
	if ( objCtx == NULL || objCtx->objItems == NULL || objData == NULL || objData->objValue == NULL ) {
		return FALSE;
	}
	
	sNamespace = objData->sNamespace;
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
	xvoTableSetInt(objRet, "data_count", 10, g_objXsBus.arrDataItems ? g_objXsBus.arrDataItems->Count : 0);
	xvoTableSetInt(objRet, "total_queued", 12, g_objXsBus.iTotalQueued);
	xvoTableSetInt(objRet, "total_delivered", 15, g_objXsBus.iTotalDelivered);
	xvoTableSetInt(objRet, "total_dropped", 13, g_objXsBus.iTotalDropped);
	xvoTableSetInt(objRet, "last_queue_time", 15, g_objXsBus.tLastQueued);
	xvoTableSetInt(objRet, "last_dispatch_time", 18, g_objXsBus.tLastDispatch);
	XS_BusSetTimeFields(objRet, g_objXsBus.tLastQueued, g_objXsBus.tLastDispatch);
	if ( g_objXsBus.arrDataItems ) {
		uint32 i;

		for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
			XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);
			(void)XS_BusBuildNamespaceStatsProc(objItem, &objCtx);
		}
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
	uint32 i;
	
	xrtMutexLock(g_objXsBus.pLock);
	for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
		XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);

		if ( XS_BusMatchDataItem(objItem, sNamespace, sTag) ) {
			xrtMutexUnlock(g_objXsBus.pLock);
			return objItem->iID;
		}
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	return 0;
}

static inline int64 XS_BusDataRemoveByQuery(const char* sNamespace, const char* sTag, int32 iLimit)
{
	int64 arrID[256];
	int64 iRemoved = 0;
	uint32 iCount = 0;
	uint32 i;
	
	if ( iLimit <= 0 || iLimit > 256 ) {
		iLimit = 256;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
		XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);

		if ( !XS_BusMatchDataItem(objItem, sNamespace, sTag) ) {
			continue;
		}

		arrID[iCount++] = objItem->iID;
		if ( iCount >= (uint32)iLimit ) {
			break;
		}
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	
	for ( i = 0; i < iCount; i++ ) {
		if ( XS_BusDataRemove(arrID[i]) ) {
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
	g_objXsBus.iTotalQueued++;
	g_objXsBus.tLastQueued = objMsg->tPost;
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
			xrtMutexLock(g_objXsBus.pLock);
			g_objXsBus.iTotalDropped++;
			g_objXsBus.tLastDispatch = xrtNow();
			xrtMutexUnlock(g_objXsBus.pLock);
			XS_LogWarn(
				"message dropped: topic=%s server=%s host=%s data=%lld",
				objMsg.sTopic ? objMsg.sTopic : "(null)",
				objMsg.sServer ? objMsg.sServer : "(null)",
				objMsg.sHost ? objMsg.sHost : "(null)",
				(long long)objMsg.iDataID
			);
		} else {
			xrtMutexLock(g_objXsBus.pLock);
			g_objXsBus.iTotalDelivered++;
			g_objXsBus.tLastDispatch = xrtNow();
			xrtMutexUnlock(g_objXsBus.pLock);
		}
		
		if ( objMsg.iDataID > 0 ) {
			XS_BusDataRelease(objMsg.iDataID);
		}
		XS_BusFreeMessage(&objMsg);
		bHandled = FALSE;
	}
}

#endif
