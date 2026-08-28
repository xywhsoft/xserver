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

#define XS_BUS_NAMESPACE_MAX_LEN 64
#define XS_BUS_NAMESPACE_LABEL_MAX_LEN (XS_BUS_NAMESPACE_MAX_LEN + 16)
#define XS_BUS_NAMESPACE_RULES_MAX_LEN 512
#define XS_BUS_NAMESPACE_ACTION_MAX_LEN 32
#define XS_BUS_SWEEP_BATCH_LIMIT 256
#define XS_BUS_SWEEP_DRAIN_MAX_PASS 64
#define XS_BUS_SWEEP_INTERVAL_SEC 1
#define XS_BUS_SWEEP_INTERVAL_MS (XS_BUS_SWEEP_INTERVAL_SEC * 1000)

typedef struct {
	xmutex pLock;
	xarray arrDataItems;
	xarray arrMessageQueue;
	xarray arrNamespacePolicyStats;
	int64 iNextDataID;
	int64 iNextMsgID;
	int64 iDataLimit;
	int64 iQueueLimit;
	int64 iNamespaceLimit;
	int64 iNamespaceDataLimit;
	int64 iSweepIntervalMS;
	char sReadonlyNamespaces[XS_BUS_NAMESPACE_RULES_MAX_LEN];
	char sDisabledNamespaces[XS_BUS_NAMESPACE_RULES_MAX_LEN];
	char sTTLRequiredNamespaces[XS_BUS_NAMESPACE_RULES_MAX_LEN];
	char sTagRequiredNamespaces[XS_BUS_NAMESPACE_RULES_MAX_LEN];
	int64 iTotalQueued;
	int64 iTotalDelivered;
	int64 iTotalDropped;
	int64 tLastQueued;
	int64 tLastDispatch;
	int64 iDataLimitRejectCount;
	int64 iQueueLimitRejectCount;
	int64 iNamespaceLimitRejectCount;
	int64 iNamespaceDataLimitRejectCount;
	int64 iReadonlyNamespaceRejectCount;
	int64 iDisabledNamespaceRejectCount;
	int64 iTTLRequiredNamespaceRejectCount;
	int64 iTagRequiredNamespaceRejectCount;
	int64 tLastDataLimitReject;
	int64 tLastQueueLimitReject;
	int64 tLastNamespaceLimitReject;
	int64 tLastNamespaceDataLimitReject;
	int64 tLastReadonlyNamespaceReject;
	int64 tLastDisabledNamespaceReject;
	int64 tLastTTLRequiredNamespaceReject;
	int64 tLastTagRequiredNamespaceReject;
	int64 iLastDataLimitCount;
	int64 iLastQueueLimitCount;
	int64 iLastNamespaceLimitCount;
	int64 iLastNamespaceDataLimitCount;
	char sLastNamespaceLimitNamespace[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
	char sLastNamespaceDataLimitNamespace[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
	char sLastReadonlyNamespace[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
	char sLastReadonlyNamespaceAction[XS_BUS_NAMESPACE_ACTION_MAX_LEN];
	char sLastDisabledNamespace[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
	char sLastDisabledNamespaceAction[XS_BUS_NAMESPACE_ACTION_MAX_LEN];
	char sLastTTLRequiredNamespace[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
	char sLastTTLRequiredNamespaceAction[XS_BUS_NAMESPACE_ACTION_MAX_LEN];
	char sLastTagRequiredNamespace[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
	char sLastTagRequiredNamespaceAction[XS_BUS_NAMESPACE_ACTION_MAX_LEN];
	int64 iSweepCount;
	int64 iSweepRemovedCount;
	int64 tLastSweep;
	int64 iLastSweepRemoved;
	int64 iLastSweepRemain;
	int64 iCleanupCount;
	int64 tLastCleanup;
	int64 iLastCleanupRemoved;
	int64 iLastCleanupRemain;
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

typedef struct {
	char sNamespace[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
	int64 iReadonlyRejectCount;
	int64 iDisabledRejectCount;
	int64 iTTLRequiredRejectCount;
	int64 iTagRequiredRejectCount;
	int64 tLastReadonlyReject;
	int64 tLastDisabledReject;
	int64 tLastTTLRequiredReject;
	int64 tLastTagRequiredReject;
	char sLastReadonlyAction[XS_BUS_NAMESPACE_ACTION_MAX_LEN];
	char sLastDisabledAction[XS_BUS_NAMESPACE_ACTION_MAX_LEN];
	char sLastTTLRequiredAction[XS_BUS_NAMESPACE_ACTION_MAX_LEN];
	char sLastTagRequiredAction[XS_BUS_NAMESPACE_ACTION_MAX_LEN];
} XS_BusNamespacePolicyStat;

static XS_Bus g_objXsBus = { 0 };
static XRT_TLS_STORAGE int32 g_iXsBusLastErrorCode = 0;
static XRT_TLS_STORAGE char g_sXsBusLastError[256] = { 0 };

static inline void XS_BusSweepExpiredData(void);
static inline void XS_BusSweepExpiredDataForce(void);

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
	XS_BUS_ERR_EXPIRE_SLOT = 10,
	XS_BUS_ERR_DATA_LIMIT = 11,
	XS_BUS_ERR_QUEUE_LIMIT = 12,
	XS_BUS_ERR_QUEUE_SLOT = 13,
	XS_BUS_ERR_INVALID_NAMESPACE = 14,
	XS_BUS_ERR_NAMESPACE_LIMIT = 15,
	XS_BUS_ERR_NAMESPACE_DATA_LIMIT = 16,
	XS_BUS_ERR_NAMESPACE_DISABLED = 17,
	XS_BUS_ERR_NAMESPACE_READONLY = 18,
	XS_BUS_ERR_NAMESPACE_TTL_REQUIRED = 19,
	XS_BUS_ERR_NAMESPACE_TAG_REQUIRED = 20
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

static inline int64 XS_BusNormalizeLimit(int64 iLimit)
{
	return iLimit > 0 ? iLimit : 0;
}

static inline const char* XS_BusNamespaceLabel(const char* sNamespace)
{
	if ( sNamespace == NULL || sNamespace[0] == '\0' ) {
		return "(default)";
	}

	return sNamespace;
}

static inline void XS_BusSetNamespaceLabel(char* sBuffer, size_t iBufferSize, const char* sNamespace)
{
	if ( sBuffer == NULL || iBufferSize == 0 ) {
		return;
	}

	snprintf(sBuffer, iBufferSize, "%s", XS_BusNamespaceLabel(sNamespace));
}

static inline bool XS_BusNamespaceEquals(const char* sLeft, const char* sRight)
{
	return strcmp(XS_BusNamespaceLabel(sLeft), XS_BusNamespaceLabel(sRight)) == 0;
}

static inline int64 XS_BusNamespaceCount_NoLock(void)
{
	uint32 i;
	int64 iCount = 0;

	if ( g_objXsBus.arrDataItems == NULL ) {
		return 0;
	}

	for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
		XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);
		bool bExists = FALSE;
		uint32 j;

		if ( objItem == NULL || objItem->objValue == NULL ) {
			continue;
		}

		for ( j = 1; j < i; j++ ) {
			XS_BusDataItem* objPrev = xrtArrayGet_Inline(g_objXsBus.arrDataItems, j);

			if ( objPrev == NULL || objPrev->objValue == NULL ) {
				continue;
			}
			if ( XS_BusNamespaceEquals(objItem->sNamespace, objPrev->sNamespace) ) {
				bExists = TRUE;
				break;
			}
		}

		if ( !bExists ) {
			iCount++;
		}
	}

	return iCount;
}

static inline int64 XS_BusNamespaceDataCount_NoLock(const char* sNamespace)
{
	uint32 i;
	int64 iCount = 0;

	if ( g_objXsBus.arrDataItems == NULL ) {
		return 0;
	}

	for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
		XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);

		if ( objItem == NULL || objItem->objValue == NULL ) {
			continue;
		}
		if ( XS_BusNamespaceEquals(objItem->sNamespace, sNamespace) ) {
			iCount++;
		}
	}

	return iCount;
}

static inline bool XS_BusIsLimitError(int32 iCode)
{
	return (
		iCode == XS_BUS_ERR_DATA_LIMIT ||
		iCode == XS_BUS_ERR_QUEUE_LIMIT ||
		iCode == XS_BUS_ERR_NAMESPACE_LIMIT ||
		iCode == XS_BUS_ERR_NAMESPACE_DATA_LIMIT ||
		iCode == XS_BUS_ERR_NAMESPACE_DISABLED ||
		iCode == XS_BUS_ERR_NAMESPACE_READONLY ||
		iCode == XS_BUS_ERR_NAMESPACE_TTL_REQUIRED ||
		iCode == XS_BUS_ERR_NAMESPACE_TAG_REQUIRED
	);
}

static inline bool XS_BusIsBadRequestError(int32 iCode)
{
	return iCode == XS_BUS_ERR_RESERVED_NAMESPACE || iCode == XS_BUS_ERR_INVALID_NAMESPACE;
}

static inline int64 XS_BusTimeAgeMS(int64 tLast)
{
	if ( tLast <= 0 ) {
		return -1;
	}

	return (int64)(xrtNow() - tLast) * 1000;
}

static inline char* XS_BusTimeText(int64 tLast)
{
	if ( tLast <= 0 ) {
		return NULL;
	}

	return xrtTimeToStr((xtime)tLast, XRT_TIME_FORMAT_DATETIME);
}

static inline bool XS_BusNamespaceRuleListHas(const char* sRules, const char* sNamespace);
static inline void XS_BusGetNamespacePolicyState_NoLock(const char* sNamespace, bool* pbReadonly, bool* pbDisabled, bool* pbTTLRequired, bool* pbTagRequired)
{
	bool bReadonly = FALSE;
	bool bDisabled = FALSE;
	bool bTTLRequired = FALSE;
	bool bTagRequired = FALSE;

	bDisabled = XS_BusNamespaceRuleListHas(g_objXsBus.sDisabledNamespaces, sNamespace);
	bReadonly = XS_BusNamespaceRuleListHas(g_objXsBus.sReadonlyNamespaces, sNamespace);
	bTTLRequired = XS_BusNamespaceRuleListHas(g_objXsBus.sTTLRequiredNamespaces, sNamespace);
	bTagRequired = XS_BusNamespaceRuleListHas(g_objXsBus.sTagRequiredNamespaces, sNamespace);
	if ( pbReadonly ) {
		*pbReadonly = bReadonly;
	}
	if ( pbDisabled ) {
		*pbDisabled = bDisabled;
	}
	if ( pbTTLRequired ) {
		*pbTTLRequired = bTTLRequired;
	}
	if ( pbTagRequired ) {
		*pbTagRequired = bTagRequired;
	}
}

static inline void XS_BusSetNamespaceLimitStateFields(xvalue objItem, int64 iNamespaceLimit, int64 iNamespaceCount);
static inline void XS_BusFinalizeNamespaceStatsItem(xvalue objItem, int64 tNow, int64 iNamespaceDataLimit);
static inline void XS_BusSetNamespacePolicyRejectStateFields(
	xvalue objItem,
	int64 tLastReadonlyReject,
	const char* sLastReadonlyNamespace,
	const char* sLastReadonlyAction,
	int64 tLastDisabledReject,
	const char* sLastDisabledNamespace,
	const char* sLastDisabledAction
);

static inline int64 XS_BusLimitRejectAgeMS(int64 tLast)
{
	return XS_BusTimeAgeMS(tLast);
}

static inline char* XS_BusLimitRejectTimeText(int64 tLast)
{
	return XS_BusTimeText(tLast);
}

static inline void XS_BusSetLimitRejectFields(
	xvalue objRet,
	int64 iDataLimit,
	int64 iQueueLimit,
	int64 iNamespaceLimit,
	int64 iNamespaceDataLimit,
	const char* sReadonlyNamespaces,
	const char* sDisabledNamespaces,
	const char* sTTLRequiredNamespaces,
	const char* sTagRequiredNamespaces,
	int64 iDataLimitRejectCount,
	int64 iQueueLimitRejectCount,
	int64 iNamespaceLimitRejectCount,
	int64 iNamespaceDataLimitRejectCount,
	int64 iReadonlyNamespaceRejectCount,
	int64 iDisabledNamespaceRejectCount,
	int64 iTTLRequiredNamespaceRejectCount,
	int64 iTagRequiredNamespaceRejectCount,
	int64 tLastDataLimitReject,
	int64 tLastQueueLimitReject,
	int64 tLastNamespaceLimitReject,
	int64 tLastNamespaceDataLimitReject,
	int64 tLastReadonlyNamespaceReject,
	int64 tLastDisabledNamespaceReject,
	int64 tLastTTLRequiredNamespaceReject,
	int64 tLastTagRequiredNamespaceReject,
	int64 iLastDataLimitCount,
	int64 iLastQueueLimitCount,
	int64 iLastNamespaceLimitCount,
	int64 iLastNamespaceDataLimitCount,
	const char* sLastNamespaceLimitNamespace,
	const char* sLastNamespaceDataLimitNamespace,
	const char* sLastReadonlyNamespace,
	const char* sLastReadonlyNamespaceAction,
	const char* sLastDisabledNamespace,
	const char* sLastDisabledNamespaceAction,
	const char* sLastTTLRequiredNamespace,
	const char* sLastTTLRequiredNamespaceAction,
	const char* sLastTagRequiredNamespace,
	const char* sLastTagRequiredNamespaceAction
)
{
	char* sDataLimitRejectTime;
	char* sQueueLimitRejectTime;
	char* sNamespaceLimitRejectTime;
	char* sNamespaceDataLimitRejectTime;
	char* sReadonlyNamespaceRejectTime;
	char* sDisabledNamespaceRejectTime;
	char* sTTLRequiredNamespaceRejectTime;
	char* sTagRequiredNamespaceRejectTime;

	if ( objRet == NULL ) {
		return;
	}

	sDataLimitRejectTime = XS_BusLimitRejectTimeText(tLastDataLimitReject);
	sQueueLimitRejectTime = XS_BusLimitRejectTimeText(tLastQueueLimitReject);
	sNamespaceLimitRejectTime = XS_BusLimitRejectTimeText(tLastNamespaceLimitReject);
	sNamespaceDataLimitRejectTime = XS_BusLimitRejectTimeText(tLastNamespaceDataLimitReject);
	sReadonlyNamespaceRejectTime = XS_BusLimitRejectTimeText(tLastReadonlyNamespaceReject);
	sDisabledNamespaceRejectTime = XS_BusLimitRejectTimeText(tLastDisabledNamespaceReject);
	sTTLRequiredNamespaceRejectTime = XS_BusLimitRejectTimeText(tLastTTLRequiredNamespaceReject);
	sTagRequiredNamespaceRejectTime = XS_BusLimitRejectTimeText(tLastTagRequiredNamespaceReject);
	xvoTableSetInt(objRet, "data_limit", sizeof("data_limit") - 1, iDataLimit);
	xvoTableSetInt(objRet, "queue_limit", sizeof("queue_limit") - 1, iQueueLimit);
	xvoTableSetInt(objRet, "namespace_limit", sizeof("namespace_limit") - 1, iNamespaceLimit);
	xvoTableSetInt(objRet, "namespace_data_limit", sizeof("namespace_data_limit") - 1, iNamespaceDataLimit);
	xvoTableSetText(objRet, "readonly_namespaces", sizeof("readonly_namespaces") - 1, (ptr)(sReadonlyNamespaces ? sReadonlyNamespaces : ""), 0, FALSE);
	xvoTableSetText(objRet, "disabled_namespaces", sizeof("disabled_namespaces") - 1, (ptr)(sDisabledNamespaces ? sDisabledNamespaces : ""), 0, FALSE);
	xvoTableSetText(objRet, "ttl_required_namespaces", sizeof("ttl_required_namespaces") - 1, (ptr)(sTTLRequiredNamespaces ? sTTLRequiredNamespaces : ""), 0, FALSE);
	xvoTableSetText(objRet, "tag_required_namespaces", sizeof("tag_required_namespaces") - 1, (ptr)(sTagRequiredNamespaces ? sTagRequiredNamespaces : ""), 0, FALSE);
	xvoTableSetInt(objRet, "data_limit_reject_count", sizeof("data_limit_reject_count") - 1, iDataLimitRejectCount);
	xvoTableSetText(objRet, "last_data_limit_reject_time", sizeof("last_data_limit_reject_time") - 1, sDataLimitRejectTime ? sDataLimitRejectTime : "", 0, FALSE);
	xvoTableSetInt(objRet, "last_data_limit_reject_age_ms", sizeof("last_data_limit_reject_age_ms") - 1, XS_BusLimitRejectAgeMS(tLastDataLimitReject));
	xvoTableSetInt(objRet, "last_data_limit_count", sizeof("last_data_limit_count") - 1, iLastDataLimitCount);
	xvoTableSetInt(objRet, "queue_limit_reject_count", sizeof("queue_limit_reject_count") - 1, iQueueLimitRejectCount);
	xvoTableSetText(objRet, "last_queue_limit_reject_time", sizeof("last_queue_limit_reject_time") - 1, sQueueLimitRejectTime ? sQueueLimitRejectTime : "", 0, FALSE);
	xvoTableSetInt(objRet, "last_queue_limit_reject_age_ms", sizeof("last_queue_limit_reject_age_ms") - 1, XS_BusLimitRejectAgeMS(tLastQueueLimitReject));
	xvoTableSetInt(objRet, "last_queue_limit_count", sizeof("last_queue_limit_count") - 1, iLastQueueLimitCount);
	xvoTableSetInt(objRet, "namespace_limit_reject_count", sizeof("namespace_limit_reject_count") - 1, iNamespaceLimitRejectCount);
	xvoTableSetText(objRet, "last_namespace_limit_reject_time", sizeof("last_namespace_limit_reject_time") - 1, sNamespaceLimitRejectTime ? sNamespaceLimitRejectTime : "", 0, FALSE);
	xvoTableSetInt(objRet, "last_namespace_limit_reject_age_ms", sizeof("last_namespace_limit_reject_age_ms") - 1, XS_BusLimitRejectAgeMS(tLastNamespaceLimitReject));
	xvoTableSetInt(objRet, "last_namespace_limit_count", sizeof("last_namespace_limit_count") - 1, iLastNamespaceLimitCount);
	xvoTableSetText(objRet, "last_namespace_limit_namespace", sizeof("last_namespace_limit_namespace") - 1, (ptr)(sLastNamespaceLimitNamespace ? sLastNamespaceLimitNamespace : ""), 0, FALSE);
	xvoTableSetInt(objRet, "namespace_data_limit_reject_count", sizeof("namespace_data_limit_reject_count") - 1, iNamespaceDataLimitRejectCount);
	xvoTableSetText(objRet, "last_namespace_data_limit_reject_time", sizeof("last_namespace_data_limit_reject_time") - 1, sNamespaceDataLimitRejectTime ? sNamespaceDataLimitRejectTime : "", 0, FALSE);
	xvoTableSetInt(objRet, "last_namespace_data_limit_reject_age_ms", sizeof("last_namespace_data_limit_reject_age_ms") - 1, XS_BusLimitRejectAgeMS(tLastNamespaceDataLimitReject));
	xvoTableSetInt(objRet, "last_namespace_data_limit_count", sizeof("last_namespace_data_limit_count") - 1, iLastNamespaceDataLimitCount);
	xvoTableSetText(objRet, "last_namespace_data_limit_namespace", sizeof("last_namespace_data_limit_namespace") - 1, (ptr)(sLastNamespaceDataLimitNamespace ? sLastNamespaceDataLimitNamespace : ""), 0, FALSE);
	xvoTableSetInt(objRet, "readonly_namespace_reject_count", sizeof("readonly_namespace_reject_count") - 1, iReadonlyNamespaceRejectCount);
	xvoTableSetText(objRet, "last_readonly_namespace_reject_time", sizeof("last_readonly_namespace_reject_time") - 1, sReadonlyNamespaceRejectTime ? sReadonlyNamespaceRejectTime : "", 0, FALSE);
	xvoTableSetInt(objRet, "last_readonly_namespace_reject_age_ms", sizeof("last_readonly_namespace_reject_age_ms") - 1, XS_BusLimitRejectAgeMS(tLastReadonlyNamespaceReject));
	xvoTableSetText(objRet, "last_readonly_namespace", sizeof("last_readonly_namespace") - 1, (ptr)(sLastReadonlyNamespace ? sLastReadonlyNamespace : ""), 0, FALSE);
	xvoTableSetText(objRet, "last_readonly_namespace_action", sizeof("last_readonly_namespace_action") - 1, (ptr)(sLastReadonlyNamespaceAction ? sLastReadonlyNamespaceAction : ""), 0, FALSE);
	xvoTableSetInt(objRet, "disabled_namespace_reject_count", sizeof("disabled_namespace_reject_count") - 1, iDisabledNamespaceRejectCount);
	xvoTableSetText(objRet, "last_disabled_namespace_reject_time", sizeof("last_disabled_namespace_reject_time") - 1, sDisabledNamespaceRejectTime ? sDisabledNamespaceRejectTime : "", 0, FALSE);
	xvoTableSetInt(objRet, "last_disabled_namespace_reject_age_ms", sizeof("last_disabled_namespace_reject_age_ms") - 1, XS_BusLimitRejectAgeMS(tLastDisabledNamespaceReject));
	xvoTableSetText(objRet, "last_disabled_namespace", sizeof("last_disabled_namespace") - 1, (ptr)(sLastDisabledNamespace ? sLastDisabledNamespace : ""), 0, FALSE);
	xvoTableSetText(objRet, "last_disabled_namespace_action", sizeof("last_disabled_namespace_action") - 1, (ptr)(sLastDisabledNamespaceAction ? sLastDisabledNamespaceAction : ""), 0, FALSE);
	xvoTableSetInt(objRet, "ttl_required_namespace_reject_count", sizeof("ttl_required_namespace_reject_count") - 1, iTTLRequiredNamespaceRejectCount);
	xvoTableSetText(objRet, "last_ttl_required_namespace_reject_time", sizeof("last_ttl_required_namespace_reject_time") - 1, sTTLRequiredNamespaceRejectTime ? sTTLRequiredNamespaceRejectTime : "", 0, FALSE);
	xvoTableSetInt(objRet, "last_ttl_required_namespace_reject_age_ms", sizeof("last_ttl_required_namespace_reject_age_ms") - 1, XS_BusLimitRejectAgeMS(tLastTTLRequiredNamespaceReject));
	xvoTableSetText(objRet, "last_ttl_required_namespace", sizeof("last_ttl_required_namespace") - 1, (ptr)(sLastTTLRequiredNamespace ? sLastTTLRequiredNamespace : ""), 0, FALSE);
	xvoTableSetText(objRet, "last_ttl_required_namespace_action", sizeof("last_ttl_required_namespace_action") - 1, (ptr)(sLastTTLRequiredNamespaceAction ? sLastTTLRequiredNamespaceAction : ""), 0, FALSE);
	xvoTableSetInt(objRet, "tag_required_namespace_reject_count", sizeof("tag_required_namespace_reject_count") - 1, iTagRequiredNamespaceRejectCount);
	xvoTableSetText(objRet, "last_tag_required_namespace_reject_time", sizeof("last_tag_required_namespace_reject_time") - 1, sTagRequiredNamespaceRejectTime ? sTagRequiredNamespaceRejectTime : "", 0, FALSE);
	xvoTableSetInt(objRet, "last_tag_required_namespace_reject_age_ms", sizeof("last_tag_required_namespace_reject_age_ms") - 1, XS_BusLimitRejectAgeMS(tLastTagRequiredNamespaceReject));
	xvoTableSetText(objRet, "last_tag_required_namespace", sizeof("last_tag_required_namespace") - 1, (ptr)(sLastTagRequiredNamespace ? sLastTagRequiredNamespace : ""), 0, FALSE);
	xvoTableSetText(objRet, "last_tag_required_namespace_action", sizeof("last_tag_required_namespace_action") - 1, (ptr)(sLastTagRequiredNamespaceAction ? sLastTagRequiredNamespaceAction : ""), 0, FALSE);
	if ( sDataLimitRejectTime ) {
		xrtFree(sDataLimitRejectTime);
	}
	if ( sQueueLimitRejectTime ) {
		xrtFree(sQueueLimitRejectTime);
	}
	if ( sNamespaceLimitRejectTime ) {
		xrtFree(sNamespaceLimitRejectTime);
	}
	if ( sNamespaceDataLimitRejectTime ) {
		xrtFree(sNamespaceDataLimitRejectTime);
	}
	if ( sReadonlyNamespaceRejectTime ) {
		xrtFree(sReadonlyNamespaceRejectTime);
	}
	if ( sDisabledNamespaceRejectTime ) {
		xrtFree(sDisabledNamespaceRejectTime);
	}
	if ( sTTLRequiredNamespaceRejectTime ) {
		xrtFree(sTTLRequiredNamespaceRejectTime);
	}
	if ( sTagRequiredNamespaceRejectTime ) {
		xrtFree(sTagRequiredNamespaceRejectTime);
	}
}

static inline void XS_BusSetSweepFields(
	xvalue objRet,
	int64 iSweepIntervalMS,
	int64 iSweepCount,
	int64 iSweepRemovedCount,
	int64 tLastSweep,
	int64 iLastSweepRemoved,
	int64 iLastSweepRemain,
	int64 iCleanupCount,
	int64 tLastCleanup,
	int64 iLastCleanupRemoved,
	int64 iLastCleanupRemain
)
{
	char* sLastSweepTime;
	char* sLastCleanupTime;

	if ( objRet == NULL ) {
		return;
	}

	sLastSweepTime = XS_BusTimeText(tLastSweep);
	sLastCleanupTime = XS_BusTimeText(tLastCleanup);
	xvoTableSetInt(objRet, "sweep_interval_ms", sizeof("sweep_interval_ms") - 1, iSweepIntervalMS);
	xvoTableSetInt(objRet, "sweep_batch_limit", sizeof("sweep_batch_limit") - 1, XS_BUS_SWEEP_BATCH_LIMIT);
	xvoTableSetInt(objRet, "sweep_count", sizeof("sweep_count") - 1, iSweepCount);
	xvoTableSetInt(objRet, "sweep_removed_count", sizeof("sweep_removed_count") - 1, iSweepRemovedCount);
	xvoTableSetInt(objRet, "last_sweep_time", sizeof("last_sweep_time") - 1, tLastSweep);
	xvoTableSetText(objRet, "last_sweep_time_text", sizeof("last_sweep_time_text") - 1, sLastSweepTime ? sLastSweepTime : "", 0, FALSE);
	xvoTableSetInt(objRet, "last_sweep_age_ms", sizeof("last_sweep_age_ms") - 1, XS_BusTimeAgeMS(tLastSweep));
	xvoTableSetInt(objRet, "last_sweep_removed", sizeof("last_sweep_removed") - 1, iLastSweepRemoved);
	xvoTableSetInt(objRet, "last_sweep_remain", sizeof("last_sweep_remain") - 1, iLastSweepRemain);
	xvoTableSetInt(objRet, "cleanup_count", sizeof("cleanup_count") - 1, iCleanupCount);
	xvoTableSetInt(objRet, "last_cleanup_time", sizeof("last_cleanup_time") - 1, tLastCleanup);
	xvoTableSetText(objRet, "last_cleanup_time_text", sizeof("last_cleanup_time_text") - 1, sLastCleanupTime ? sLastCleanupTime : "", 0, FALSE);
	xvoTableSetInt(objRet, "last_cleanup_age_ms", sizeof("last_cleanup_age_ms") - 1, XS_BusTimeAgeMS(tLastCleanup));
	xvoTableSetInt(objRet, "last_cleanup_removed", sizeof("last_cleanup_removed") - 1, iLastCleanupRemoved);
	xvoTableSetInt(objRet, "last_cleanup_remain", sizeof("last_cleanup_remain") - 1, iLastCleanupRemain);
	if ( sLastSweepTime ) {
		xrtFree(sLastSweepTime);
	}
	if ( sLastCleanupTime ) {
		xrtFree(sLastCleanupTime);
	}
}

static inline void XS_BusConfigureLimits(
	bool bSetDataLimit,
	int64 iDataLimit,
	bool bSetQueueLimit,
	int64 iQueueLimit,
	bool bSetNamespaceLimit,
	int64 iNamespaceLimit,
	bool bSetNamespaceDataLimit,
	int64 iNamespaceDataLimit,
	bool bSetReadonlyNamespaces,
	const char* sReadonlyNamespaces,
	bool bSetDisabledNamespaces,
	const char* sDisabledNamespaces,
	bool bSetTTLRequiredNamespaces,
	const char* sTTLRequiredNamespaces,
	bool bSetTagRequiredNamespaces,
	const char* sTagRequiredNamespaces
)
{
	if ( g_objXsBus.pLock == NULL ) {
		return;
	}

	xrtMutexLock(g_objXsBus.pLock);
	if ( bSetDataLimit ) {
		g_objXsBus.iDataLimit = XS_BusNormalizeLimit(iDataLimit);
	}
	if ( bSetQueueLimit ) {
		g_objXsBus.iQueueLimit = XS_BusNormalizeLimit(iQueueLimit);
	}
	if ( bSetNamespaceLimit ) {
		g_objXsBus.iNamespaceLimit = XS_BusNormalizeLimit(iNamespaceLimit);
	}
	if ( bSetNamespaceDataLimit ) {
		g_objXsBus.iNamespaceDataLimit = XS_BusNormalizeLimit(iNamespaceDataLimit);
	}
	if ( bSetReadonlyNamespaces ) {
		snprintf(g_objXsBus.sReadonlyNamespaces, sizeof(g_objXsBus.sReadonlyNamespaces), "%s", sReadonlyNamespaces ? sReadonlyNamespaces : "");
	}
	if ( bSetDisabledNamespaces ) {
		snprintf(g_objXsBus.sDisabledNamespaces, sizeof(g_objXsBus.sDisabledNamespaces), "%s", sDisabledNamespaces ? sDisabledNamespaces : "");
	}
	if ( bSetTTLRequiredNamespaces ) {
		snprintf(g_objXsBus.sTTLRequiredNamespaces, sizeof(g_objXsBus.sTTLRequiredNamespaces), "%s", sTTLRequiredNamespaces ? sTTLRequiredNamespaces : "");
	}
	if ( bSetTagRequiredNamespaces ) {
		snprintf(g_objXsBus.sTagRequiredNamespaces, sizeof(g_objXsBus.sTagRequiredNamespaces), "%s", sTagRequiredNamespaces ? sTagRequiredNamespaces : "");
	}
	xrtMutexUnlock(g_objXsBus.pLock);
}

static inline void XS_BusConfigureSweep(bool bSetSweepInterval, int64 iSweepIntervalMS)
{
	if ( g_objXsBus.pLock == NULL ) {
		return;
	}

	xrtMutexLock(g_objXsBus.pLock);
	if ( bSetSweepInterval ) {
		g_objXsBus.iSweepIntervalMS = iSweepIntervalMS >= 0 ? iSweepIntervalMS : XS_BUS_SWEEP_INTERVAL_MS;
	}
	xrtMutexUnlock(g_objXsBus.pLock);
}

static inline xvalue XS_BusBuildLimitValue(void)
{
	xvalue objRet = xvoCreateTable();

	if ( objRet == NULL ) {
		return NULL;
	}

	XS_BusSweepExpiredData();

	xrtMutexLock(g_objXsBus.pLock);
	xvoTableSetInt(objRet, "data_count", 10, g_objXsBus.arrDataItems ? g_objXsBus.arrDataItems->Count : 0);
	xvoTableSetInt(objRet, "queue_count", 11, g_objXsBus.arrMessageQueue ? g_objXsBus.arrMessageQueue->Count : 0);
	xvoTableSetInt(objRet, "namespace_count", sizeof("namespace_count") - 1, XS_BusNamespaceCount_NoLock());
	XS_BusSetNamespaceLimitStateFields(
		objRet,
		g_objXsBus.iNamespaceLimit,
		xvoTableGetInt(objRet, "namespace_count", sizeof("namespace_count") - 1)
	);
	XS_BusSetLimitRejectFields(
		objRet,
		g_objXsBus.iDataLimit,
		g_objXsBus.iQueueLimit,
		g_objXsBus.iNamespaceLimit,
		g_objXsBus.iNamespaceDataLimit,
		g_objXsBus.sReadonlyNamespaces,
		g_objXsBus.sDisabledNamespaces,
		g_objXsBus.sTTLRequiredNamespaces,
		g_objXsBus.sTagRequiredNamespaces,
		g_objXsBus.iDataLimitRejectCount,
		g_objXsBus.iQueueLimitRejectCount,
		g_objXsBus.iNamespaceLimitRejectCount,
		g_objXsBus.iNamespaceDataLimitRejectCount,
		g_objXsBus.iReadonlyNamespaceRejectCount,
		g_objXsBus.iDisabledNamespaceRejectCount,
		g_objXsBus.iTTLRequiredNamespaceRejectCount,
		g_objXsBus.iTagRequiredNamespaceRejectCount,
		g_objXsBus.tLastDataLimitReject,
		g_objXsBus.tLastQueueLimitReject,
		g_objXsBus.tLastNamespaceLimitReject,
		g_objXsBus.tLastNamespaceDataLimitReject,
		g_objXsBus.tLastReadonlyNamespaceReject,
		g_objXsBus.tLastDisabledNamespaceReject,
		g_objXsBus.tLastTTLRequiredNamespaceReject,
		g_objXsBus.tLastTagRequiredNamespaceReject,
		g_objXsBus.iLastDataLimitCount,
		g_objXsBus.iLastQueueLimitCount,
		g_objXsBus.iLastNamespaceLimitCount,
		g_objXsBus.iLastNamespaceDataLimitCount,
		g_objXsBus.sLastNamespaceLimitNamespace,
		g_objXsBus.sLastNamespaceDataLimitNamespace,
		g_objXsBus.sLastReadonlyNamespace,
		g_objXsBus.sLastReadonlyNamespaceAction,
		g_objXsBus.sLastDisabledNamespace,
		g_objXsBus.sLastDisabledNamespaceAction,
		g_objXsBus.sLastTTLRequiredNamespace,
		g_objXsBus.sLastTTLRequiredNamespaceAction,
		g_objXsBus.sLastTagRequiredNamespace,
		g_objXsBus.sLastTagRequiredNamespaceAction
	);
	XS_BusSetSweepFields(
		objRet,
		g_objXsBus.iSweepIntervalMS,
		g_objXsBus.iSweepCount,
		g_objXsBus.iSweepRemovedCount,
		g_objXsBus.tLastSweep,
		g_objXsBus.iLastSweepRemoved,
		g_objXsBus.iLastSweepRemain,
		g_objXsBus.iCleanupCount,
		g_objXsBus.tLastCleanup,
		g_objXsBus.iLastCleanupRemoved,
		g_objXsBus.iLastCleanupRemain
	);
	xrtMutexUnlock(g_objXsBus.pLock);
	return objRet;
}

static inline char* XS_BusBuildLimitJson(void)
{
	xvalue objLimit = XS_BusBuildLimitValue();
	char* sRet = NULL;

	if ( objLimit == NULL ) {
		return NULL;
	}

	sRet = xrtStringifyJSON(objLimit, FALSE, NULL);
	xvoUnref(objLimit);
	return sRet;
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

static inline bool XS_BusIsNamespaceChar(char ch)
{
	return (
		(ch >= '0' && ch <= '9') ||
		(ch >= 'a' && ch <= 'z') ||
		(ch >= 'A' && ch <= 'Z') ||
		ch == '.' ||
		ch == '_' ||
		ch == '-'
	);
}

static inline const char* XS_BusNamespaceInvalidReason(const char* sNamespace)
{
	size_t i;

	if ( sNamespace == NULL || sNamespace[0] == '\0' ) {
		return NULL;
	}
	if ( sNamespace[0] == '.' ) {
		return "namespace must not start with '.'";
	}

	for ( i = 0; sNamespace[i] != '\0'; i++ ) {
		if ( i >= (size_t)XS_BUS_NAMESPACE_MAX_LEN ) {
			return "namespace too long";
		}
		if ( !XS_BusIsNamespaceChar(sNamespace[i]) ) {
			return "namespace has invalid character";
		}
		if ( sNamespace[i] == '.' ) {
			if ( sNamespace[i + 1] == '\0' ) {
				return "namespace must not end with '.'";
			}
			if ( sNamespace[i + 1] == '.' ) {
				return "namespace must not contain empty segment";
			}
		}
	}

	return NULL;
}

static inline bool XS_BusNamespaceRuleIsDefaultToken(const char* sToken, size_t iTokenLen)
{
	if ( sToken == NULL ) {
		return FALSE;
	}
	if ( iTokenLen == 7 && strncmp(sToken, "default", 7) == 0 ) {
		return TRUE;
	}
	if ( iTokenLen == 9 && strncmp(sToken, "(default)", 9) == 0 ) {
		return TRUE;
	}

	return FALSE;
}

static inline bool XS_BusNamespaceRuleIsPrefixToken(const char* sToken, size_t iTokenLen)
{
	return (
		sToken != NULL &&
		iTokenLen > 2 &&
		sToken[iTokenLen - 1] == '*' &&
		sToken[iTokenLen - 2] == '.'
	);
}

static inline bool XS_BusNamespaceRuleCopyPrefixBase(const char* sToken, size_t iTokenLen, char* sBuffer, size_t iBufferSize)
{
	size_t iBaseLen;

	if ( !XS_BusNamespaceRuleIsPrefixToken(sToken, iTokenLen) || sBuffer == NULL || iBufferSize == 0 ) {
		return FALSE;
	}

	iBaseLen = iTokenLen - 2;
	if ( iBaseLen == 0 || iBaseLen >= iBufferSize ) {
		return FALSE;
	}

	memcpy(sBuffer, sToken, iBaseLen);
	sBuffer[iBaseLen] = '\0';
	return TRUE;
}

static inline bool XS_BusNamespaceRuleTokenEquals(const char* sToken, size_t iTokenLen, const char* sNamespace)
{
	char sPrefixBase[XS_BUS_NAMESPACE_MAX_LEN + 1];
	size_t iNamespaceLen;
	size_t iPrefixLen;

	if ( sToken == NULL ) {
		return FALSE;
	}
	if ( sNamespace == NULL || sNamespace[0] == '\0' ) {
		return XS_BusNamespaceRuleIsDefaultToken(sToken, iTokenLen);
	}
	if ( XS_BusNamespaceRuleIsPrefixToken(sToken, iTokenLen) ) {
		if ( !XS_BusNamespaceRuleCopyPrefixBase(sToken, iTokenLen, sPrefixBase, sizeof(sPrefixBase)) ) {
			return FALSE;
		}
		iPrefixLen = strlen(sPrefixBase);
		iNamespaceLen = strlen(sNamespace);
		return (
			iNamespaceLen > iPrefixLen &&
			memcmp(sPrefixBase, sNamespace, iPrefixLen) == 0 &&
			sNamespace[iPrefixLen] == '.'
		);
	}

	iNamespaceLen = strlen(sNamespace);
	return iNamespaceLen == iTokenLen && memcmp(sToken, sNamespace, iTokenLen) == 0;
}

static inline bool XS_BusNamespaceRuleListHasToken(const char* sRules, const char* sToken, size_t iTokenLen)
{
	const char* sPos;
	const char* sNext;
	size_t iRuleTokenLen;

	if ( sRules == NULL || sRules[0] == '\0' || sToken == NULL || iTokenLen == 0 ) {
		return FALSE;
	}

	sPos = sRules;
	while ( sPos[0] != '\0' ) {
		while ( sPos[0] == ',' ) {
			sPos++;
		}
		if ( sPos[0] == '\0' ) {
			break;
		}
		sNext = strchr(sPos, ',');
		iRuleTokenLen = sNext ? (size_t)(sNext - sPos) : strlen(sPos);
		if ( iRuleTokenLen == iTokenLen && memcmp(sPos, sToken, iTokenLen) == 0 ) {
			return TRUE;
		}
		if ( sNext == NULL ) {
			break;
		}
		sPos = sNext + 1;
	}

	return FALSE;
}

static inline bool XS_BusNamespaceRuleListHas(const char* sRules, const char* sNamespace)
{
	const char* sPos;
	const char* sNext;
	size_t iTokenLen;

	if ( sRules == NULL || sRules[0] == '\0' ) {
		return FALSE;
	}

	sPos = sRules;
	while ( sPos[0] != '\0' ) {
		while ( sPos[0] == ',' ) {
			sPos++;
		}
		if ( sPos[0] == '\0' ) {
			break;
		}
		sNext = strchr(sPos, ',');
		iTokenLen = sNext ? (size_t)(sNext - sPos) : strlen(sPos);
		if ( iTokenLen > 0 && XS_BusNamespaceRuleTokenEquals(sPos, iTokenLen, sNamespace) ) {
			return TRUE;
		}
		if ( sNext == NULL ) {
			break;
		}
		sPos = sNext + 1;
	}

	return FALSE;
}

static inline bool XS_BusNamespaceRuleTokenMatchesLabel(const char* sToken, const char* sNamespaceLabel)
{
	const char* sNamespace = sNamespaceLabel;

	if ( sToken == NULL || sToken[0] == '\0' ) {
		return FALSE;
	}
	if ( sNamespace && strcmp(sNamespace, "(default)") == 0 ) {
		sNamespace = NULL;
	}

	return XS_BusNamespaceRuleTokenEquals(sToken, strlen(sToken), sNamespace);
}

static inline XS_BusNamespacePolicyStat* XS_BusGetNamespacePolicyStat_NoLock(const char* sNamespace, bool bCreate)
{
	char sNamespaceLabel[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
	uint32 i;

	if ( g_objXsBus.arrNamespacePolicyStats == NULL ) {
		return NULL;
	}

	XS_BusSetNamespaceLabel(sNamespaceLabel, sizeof(sNamespaceLabel), sNamespace);
	for ( i = 1; i <= g_objXsBus.arrNamespacePolicyStats->Count; i++ ) {
		XS_BusNamespacePolicyStat* objStat = xrtArrayGet_Inline(g_objXsBus.arrNamespacePolicyStats, i);

		if ( objStat && strcmp(objStat->sNamespace, sNamespaceLabel) == 0 ) {
			return objStat;
		}
	}
	if ( !bCreate ) {
		return NULL;
	}

	i = xrtArrayAppend(g_objXsBus.arrNamespacePolicyStats, 1);
	if ( i == 0 ) {
		return NULL;
	}

	{
		XS_BusNamespacePolicyStat* objStat = xrtArrayGet_Inline(g_objXsBus.arrNamespacePolicyStats, i);

		if ( objStat == NULL ) {
			return NULL;
		}
		memset(objStat, 0, sizeof(XS_BusNamespacePolicyStat));
		snprintf(objStat->sNamespace, sizeof(objStat->sNamespace), "%s", sNamespaceLabel);
		return objStat;
	}
}

static inline bool XS_BusNamespaceRuleTokenIsReserved(const char* sToken, size_t iTokenLen)
{
	char sRuleNamespace[XS_BUS_NAMESPACE_MAX_LEN + 1];

	if ( sToken == NULL || iTokenLen == 0 ) {
		return FALSE;
	}
	if ( XS_BusNamespaceRuleIsDefaultToken(sToken, iTokenLen) ) {
		return FALSE;
	}
	if ( XS_BusNamespaceRuleIsPrefixToken(sToken, iTokenLen) ) {
		if ( !XS_BusNamespaceRuleCopyPrefixBase(sToken, iTokenLen, sRuleNamespace, sizeof(sRuleNamespace)) ) {
			return TRUE;
		}
		if ( strcmp(sRuleNamespace, "xs") == 0 || strcmp(sRuleNamespace, "__xs") == 0 ) {
			return TRUE;
		}
		return XS_BusIsReservedNamespace(sRuleNamespace);
	}
	if ( iTokenLen >= sizeof(sRuleNamespace) ) {
		return TRUE;
	}

	memcpy(sRuleNamespace, sToken, iTokenLen);
	sRuleNamespace[iTokenLen] = '\0';
	return XS_BusIsReservedNamespace(sRuleNamespace);
}

static inline bool XS_BusNormalizeNamespaceRules(const char* sRules, char* sBuffer, size_t iBufferSize, const char** psError)
{
	const char* sPos;
	size_t iWrite = 0;

	if ( psError ) {
		*psError = NULL;
	}
	if ( sBuffer == NULL || iBufferSize == 0 ) {
		if ( psError ) {
			*psError = "invalid buffer";
		}
		return FALSE;
	}

	sBuffer[0] = '\0';
	if ( sRules == NULL || sRules[0] == '\0' ) {
		return TRUE;
	}

	sPos = sRules;
	while ( sPos[0] != '\0' ) {
		const char* sStart;
		const char* sEnd;
		size_t iTokenLen;
		char sToken[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
		const char* sRuleError;
		bool bDuplicate;

		while ( sPos[0] == ',' || sPos[0] == ' ' || sPos[0] == '\t' || sPos[0] == '\r' || sPos[0] == '\n' ) {
			sPos++;
		}
		if ( sPos[0] == '\0' ) {
			break;
		}

		sStart = sPos;
		sEnd = sPos;
		while ( sEnd[0] != '\0' && sEnd[0] != ',' ) {
			sEnd++;
		}
		while ( sEnd > sStart && (sEnd[-1] == ' ' || sEnd[-1] == '\t' || sEnd[-1] == '\r' || sEnd[-1] == '\n') ) {
			sEnd--;
		}
		iTokenLen = (size_t)(sEnd - sStart);
		if ( iTokenLen == 0 ) {
			if ( sPos[0] == ',' ) {
				sPos++;
				continue;
			}
			break;
		}
		if ( iTokenLen >= sizeof(sToken) ) {
			if ( psError ) {
				*psError = "namespace too long";
			}
			return FALSE;
		}

		memcpy(sToken, sStart, iTokenLen);
		sToken[iTokenLen] = '\0';
		if ( XS_BusNamespaceRuleIsDefaultToken(sToken, iTokenLen) ) {
			snprintf(sToken, sizeof(sToken), "(default)");
			iTokenLen = strlen(sToken);
		} else if ( XS_BusNamespaceRuleIsPrefixToken(sToken, iTokenLen) ) {
			char sPrefixBase[XS_BUS_NAMESPACE_MAX_LEN + 1];

			if ( !XS_BusNamespaceRuleCopyPrefixBase(sToken, iTokenLen, sPrefixBase, sizeof(sPrefixBase)) ) {
				if ( psError ) {
					*psError = "invalid namespace prefix rule";
				}
				return FALSE;
			}
			sRuleError = XS_BusNamespaceInvalidReason(sPrefixBase);
			if ( sRuleError ) {
				if ( psError ) {
					*psError = sRuleError;
				}
				return FALSE;
			}
			if ( XS_BusNamespaceRuleTokenIsReserved(sToken, iTokenLen) ) {
				if ( psError ) {
					*psError = "reserved namespace";
				}
				return FALSE;
			}
		} else {
			sRuleError = XS_BusNamespaceInvalidReason(sToken);
			if ( sRuleError ) {
				if ( psError ) {
					*psError = sRuleError;
				}
				return FALSE;
			}
			if ( XS_BusIsReservedNamespace(sToken) ) {
				if ( psError ) {
					*psError = "reserved namespace";
				}
				return FALSE;
			}
		}

		bDuplicate = XS_BusNamespaceRuleListHasToken(sBuffer, sToken, iTokenLen);
		if ( !bDuplicate ) {
			if ( iWrite > 0 ) {
				if ( iWrite + 1 >= iBufferSize ) {
					if ( psError ) {
						*psError = "namespace rules too long";
					}
					return FALSE;
				}
				sBuffer[iWrite++] = ',';
			}
			if ( iWrite + iTokenLen >= iBufferSize ) {
				if ( psError ) {
					*psError = "namespace rules too long";
				}
				return FALSE;
			}
			memcpy(sBuffer + iWrite, sToken, iTokenLen);
			iWrite += iTokenLen;
			sBuffer[iWrite] = '\0';
		}

		sPos = sEnd;
		while ( sPos[0] != '\0' && sPos[0] != ',' ) {
			sPos++;
		}
		if ( sPos[0] == ',' ) {
			sPos++;
		}
	}

	return TRUE;
}

static inline void XS_BusRecordNamespacePolicyReject_NoLock(bool bReadonly, const char* sNamespace, const char* sAction)
{
	XS_BusNamespacePolicyStat* objStat = XS_BusGetNamespacePolicyStat_NoLock(sNamespace, TRUE);

	if ( bReadonly ) {
		g_objXsBus.iReadonlyNamespaceRejectCount++;
		g_objXsBus.tLastReadonlyNamespaceReject = xrtNow();
		XS_BusSetNamespaceLabel(g_objXsBus.sLastReadonlyNamespace, sizeof(g_objXsBus.sLastReadonlyNamespace), sNamespace);
		snprintf(g_objXsBus.sLastReadonlyNamespaceAction, sizeof(g_objXsBus.sLastReadonlyNamespaceAction), "%s", sAction ? sAction : "");
		if ( objStat ) {
			objStat->iReadonlyRejectCount++;
			objStat->tLastReadonlyReject = g_objXsBus.tLastReadonlyNamespaceReject;
			snprintf(objStat->sLastReadonlyAction, sizeof(objStat->sLastReadonlyAction), "%s", sAction ? sAction : "");
		}
	} else {
		g_objXsBus.iDisabledNamespaceRejectCount++;
		g_objXsBus.tLastDisabledNamespaceReject = xrtNow();
		XS_BusSetNamespaceLabel(g_objXsBus.sLastDisabledNamespace, sizeof(g_objXsBus.sLastDisabledNamespace), sNamespace);
		snprintf(g_objXsBus.sLastDisabledNamespaceAction, sizeof(g_objXsBus.sLastDisabledNamespaceAction), "%s", sAction ? sAction : "");
		if ( objStat ) {
			objStat->iDisabledRejectCount++;
			objStat->tLastDisabledReject = g_objXsBus.tLastDisabledNamespaceReject;
			snprintf(objStat->sLastDisabledAction, sizeof(objStat->sLastDisabledAction), "%s", sAction ? sAction : "");
		}
	}
}

static inline void XS_BusRecordTTLRequiredNamespaceReject_NoLock(const char* sNamespace, const char* sAction)
{
	XS_BusNamespacePolicyStat* objStat = XS_BusGetNamespacePolicyStat_NoLock(sNamespace, TRUE);

	g_objXsBus.iTTLRequiredNamespaceRejectCount++;
	g_objXsBus.tLastTTLRequiredNamespaceReject = xrtNow();
	XS_BusSetNamespaceLabel(g_objXsBus.sLastTTLRequiredNamespace, sizeof(g_objXsBus.sLastTTLRequiredNamespace), sNamespace);
	snprintf(g_objXsBus.sLastTTLRequiredNamespaceAction, sizeof(g_objXsBus.sLastTTLRequiredNamespaceAction), "%s", sAction ? sAction : "");
	if ( objStat ) {
		objStat->iTTLRequiredRejectCount++;
		objStat->tLastTTLRequiredReject = g_objXsBus.tLastTTLRequiredNamespaceReject;
		snprintf(objStat->sLastTTLRequiredAction, sizeof(objStat->sLastTTLRequiredAction), "%s", sAction ? sAction : "");
	}
}

static inline void XS_BusRecordTagRequiredNamespaceReject_NoLock(const char* sNamespace, const char* sAction)
{
	XS_BusNamespacePolicyStat* objStat = XS_BusGetNamespacePolicyStat_NoLock(sNamespace, TRUE);

	g_objXsBus.iTagRequiredNamespaceRejectCount++;
	g_objXsBus.tLastTagRequiredNamespaceReject = xrtNow();
	XS_BusSetNamespaceLabel(g_objXsBus.sLastTagRequiredNamespace, sizeof(g_objXsBus.sLastTagRequiredNamespace), sNamespace);
	snprintf(g_objXsBus.sLastTagRequiredNamespaceAction, sizeof(g_objXsBus.sLastTagRequiredNamespaceAction), "%s", sAction ? sAction : "");
	if ( objStat ) {
		objStat->iTagRequiredRejectCount++;
		objStat->tLastTagRequiredReject = g_objXsBus.tLastTagRequiredNamespaceReject;
		snprintf(objStat->sLastTagRequiredAction, sizeof(objStat->sLastTagRequiredAction), "%s", sAction ? sAction : "");
	}
}

static inline int32 XS_BusCheckNamespacePolicy_NoLock(const char* sNamespace, const char* sAction, bool bWrite)
{
	if ( XS_BusNamespaceRuleListHas(g_objXsBus.sDisabledNamespaces, sNamespace) ) {
		XS_BusRecordNamespacePolicyReject_NoLock(FALSE, sNamespace, sAction);
		XS_BusSetLastError(XS_BUS_ERR_NAMESPACE_DISABLED, "namespace disabled");
		return XS_BUS_ERR_NAMESPACE_DISABLED;
	}
	if ( bWrite && XS_BusNamespaceRuleListHas(g_objXsBus.sReadonlyNamespaces, sNamespace) ) {
		XS_BusRecordNamespacePolicyReject_NoLock(TRUE, sNamespace, sAction);
		XS_BusSetLastError(XS_BUS_ERR_NAMESPACE_READONLY, "namespace readonly");
		return XS_BUS_ERR_NAMESPACE_READONLY;
	}

	return XS_BUS_ERR_NONE;
}

static inline int32 XS_BusCheckNamespaceTTLRequiredState_NoLock(const char* sNamespace, const char* sAction, bool bWillExpire)
{
	if ( !bWillExpire && XS_BusNamespaceRuleListHas(g_objXsBus.sTTLRequiredNamespaces, sNamespace) ) {
		XS_BusRecordTTLRequiredNamespaceReject_NoLock(sNamespace, sAction);
		XS_BusSetLastError(XS_BUS_ERR_NAMESPACE_TTL_REQUIRED, "namespace ttl required");
		return XS_BUS_ERR_NAMESPACE_TTL_REQUIRED;
	}

	return XS_BUS_ERR_NONE;
}

static inline int32 XS_BusCheckNamespaceTagRequiredState_NoLock(const char* sNamespace, const char* sTag, const char* sAction)
{
	if ( XS_BusNamespaceRuleListHas(g_objXsBus.sTagRequiredNamespaces, sNamespace) && (sTag == NULL || sTag[0] == '\0') ) {
		XS_BusRecordTagRequiredNamespaceReject_NoLock(sNamespace, sAction);
		XS_BusSetLastError(XS_BUS_ERR_NAMESPACE_TAG_REQUIRED, "namespace tag required");
		return XS_BUS_ERR_NAMESPACE_TAG_REQUIRED;
	}

	return XS_BUS_ERR_NONE;
}

static inline int32 XS_BusCheckNamespaceTTLRequiredState(const char* sNamespace, const char* sAction, bool bWillExpire)
{
	int32 iCode = XS_BUS_ERR_NONE;

	if ( g_objXsBus.pLock == NULL ) {
		return XS_BUS_ERR_NONE;
	}

	xrtMutexLock(g_objXsBus.pLock);
	iCode = XS_BusCheckNamespaceTTLRequiredState_NoLock(sNamespace, sAction, bWillExpire);
	xrtMutexUnlock(g_objXsBus.pLock);
	return iCode;
}

static inline int32 XS_BusCheckNamespaceTagRequiredState(const char* sNamespace, const char* sTag, const char* sAction)
{
	int32 iCode = XS_BUS_ERR_NONE;

	if ( g_objXsBus.pLock == NULL ) {
		return XS_BUS_ERR_NONE;
	}

	xrtMutexLock(g_objXsBus.pLock);
	iCode = XS_BusCheckNamespaceTagRequiredState_NoLock(sNamespace, sTag, sAction);
	xrtMutexUnlock(g_objXsBus.pLock);
	return iCode;
}

static inline int32 XS_BusCheckNamespacePolicy(const char* sNamespace, const char* sAction, bool bWrite)
{
	int32 iCode = XS_BUS_ERR_NONE;

	if ( g_objXsBus.pLock == NULL ) {
		return XS_BUS_ERR_NONE;
	}

	xrtMutexLock(g_objXsBus.pLock);
	iCode = XS_BusCheckNamespacePolicy_NoLock(sNamespace, sAction, bWrite);
	xrtMutexUnlock(g_objXsBus.pLock);
	return iCode;
}

static bool XS_BusPublishValue(xvalue objVal);
static inline void XS_BusSweepExpiredData(void);
static inline void XS_BusSweepExpiredDataForce(void);
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
	g_objXsBus.arrNamespacePolicyStats = xrtArrayCreate(sizeof(XS_BusNamespacePolicyStat), XRT_OBJMODE_SHARED);
	g_objXsBus.iNextDataID = 1;
	g_objXsBus.iNextMsgID = 1;
	g_objXsBus.iSweepIntervalMS = XS_BUS_SWEEP_INTERVAL_MS;
	
	if ( g_objXsBus.pLock == NULL || g_objXsBus.arrDataItems == NULL || g_objXsBus.arrMessageQueue == NULL || g_objXsBus.arrNamespacePolicyStats == NULL ) {
		return FALSE;
	}
	
	xrtOwnerActivateShared(&g_objXsBus.arrDataItems->Owner);
	xrtOwnerActivateShared(&g_objXsBus.arrMessageQueue->Owner);
	xrtOwnerActivateShared(&g_objXsBus.arrNamespacePolicyStats->Owner);
	
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
	g_objXsBus.iDataLimitRejectCount = 0;
	g_objXsBus.iQueueLimitRejectCount = 0;
	g_objXsBus.iNamespaceLimitRejectCount = 0;
	g_objXsBus.iNamespaceDataLimitRejectCount = 0;
	g_objXsBus.iReadonlyNamespaceRejectCount = 0;
	g_objXsBus.iDisabledNamespaceRejectCount = 0;
	g_objXsBus.iTTLRequiredNamespaceRejectCount = 0;
	g_objXsBus.iTagRequiredNamespaceRejectCount = 0;
	g_objXsBus.tLastDataLimitReject = 0;
	g_objXsBus.tLastQueueLimitReject = 0;
	g_objXsBus.tLastNamespaceLimitReject = 0;
	g_objXsBus.tLastNamespaceDataLimitReject = 0;
	g_objXsBus.tLastReadonlyNamespaceReject = 0;
	g_objXsBus.tLastDisabledNamespaceReject = 0;
	g_objXsBus.tLastTTLRequiredNamespaceReject = 0;
	g_objXsBus.tLastTagRequiredNamespaceReject = 0;
	g_objXsBus.iLastDataLimitCount = 0;
	g_objXsBus.iLastQueueLimitCount = 0;
	g_objXsBus.iLastNamespaceLimitCount = 0;
	g_objXsBus.iLastNamespaceDataLimitCount = 0;
	g_objXsBus.sLastNamespaceLimitNamespace[0] = '\0';
	g_objXsBus.sLastNamespaceDataLimitNamespace[0] = '\0';
	g_objXsBus.sLastReadonlyNamespace[0] = '\0';
	g_objXsBus.sLastReadonlyNamespaceAction[0] = '\0';
	g_objXsBus.sLastDisabledNamespace[0] = '\0';
	g_objXsBus.sLastDisabledNamespaceAction[0] = '\0';
	g_objXsBus.sLastTTLRequiredNamespace[0] = '\0';
	g_objXsBus.sLastTTLRequiredNamespaceAction[0] = '\0';
	g_objXsBus.sLastTagRequiredNamespace[0] = '\0';
	g_objXsBus.sLastTagRequiredNamespaceAction[0] = '\0';
	g_objXsBus.iSweepCount = 0;
	g_objXsBus.iSweepRemovedCount = 0;
	g_objXsBus.tLastSweep = 0;
	g_objXsBus.iLastSweepRemoved = 0;
	g_objXsBus.iLastSweepRemain = 0;
	g_objXsBus.iCleanupCount = 0;
	g_objXsBus.tLastCleanup = 0;
	g_objXsBus.iLastCleanupRemoved = 0;
	g_objXsBus.iLastCleanupRemain = 0;
	if ( g_objXsBus.arrNamespacePolicyStats && g_objXsBus.arrNamespacePolicyStats->Count > 0 ) {
		(void)xrtArrayRemove(g_objXsBus.arrNamespacePolicyStats, 1, g_objXsBus.arrNamespacePolicyStats->Count);
	}
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
	if ( g_objXsBus.arrNamespacePolicyStats ) {
		xrtArrayDestroy(g_objXsBus.arrNamespacePolicyStats);
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
	const char* sNamespaceError;
	
	XS_BusClearLastError();
	
	if ( objValue == NULL ) {
		XS_BusSetLastError(XS_BUS_ERR_INVALID_VALUE, "value is null");
		XS_LogWarn("bus register failed: value is null");
		return 0;
	}
	sNamespaceError = XS_BusNamespaceInvalidReason(sNamespace);
	if ( sNamespaceError ) {
		XS_BusSetLastError(XS_BUS_ERR_INVALID_NAMESPACE, sNamespaceError);
		XS_LogWarn(
			"bus register failed: %s: %s",
			sNamespaceError,
			sNamespace ? sNamespace : "(null)"
		);
		return 0;
	}
	if ( XS_BusIsReservedNamespace(sNamespace) ) {
		XS_BusSetLastError(XS_BUS_ERR_RESERVED_NAMESPACE, "reserved namespace");
		XS_LogWarn("bus register failed: reserved namespace: %s", sNamespace);
		return 0;
	}
	if ( XS_BusCheckNamespacePolicy(sNamespace, "register", TRUE) != XS_BUS_ERR_NONE ) {
		XS_LogWarn(
			"bus register failed: %s: namespace=%s tag=%s",
			XS_BusGetLastError(),
			XS_BusNamespaceLabel(sNamespace),
			sTag ? sTag : "(null)"
		);
		return 0;
	}
	if ( XS_BusCheckNamespaceTagRequiredState(sNamespace, sTag, "register") != XS_BUS_ERR_NONE ) {
		XS_LogWarn(
			"bus register failed: %s: namespace=%s tag=%s",
			XS_BusGetLastError(),
			XS_BusNamespaceLabel(sNamespace),
			sTag ? sTag : "(null)"
		);
		return 0;
	}

	XS_BusSweepExpiredDataForce();
	
	iTTLSecond = XS_BusTTLToSeconds(iTTL);
	if ( XS_BusCheckNamespaceTTLRequiredState(sNamespace, "register", iTTLSecond > 0) != XS_BUS_ERR_NONE ) {
		XS_LogWarn(
			"bus register failed: %s: namespace=%s tag=%s ttl=%lld",
			XS_BusGetLastError(),
			XS_BusNamespaceLabel(sNamespace),
			sTag ? sTag : "(null)",
			(long long)iTTL
		);
		return 0;
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
	if ( g_objXsBus.iDataLimit > 0 && g_objXsBus.arrDataItems && (int64)g_objXsBus.arrDataItems->Count >= g_objXsBus.iDataLimit ) {
		g_objXsBus.iDataLimitRejectCount++;
		g_objXsBus.tLastDataLimitReject = xrtNow();
		g_objXsBus.iLastDataLimitCount = g_objXsBus.arrDataItems->Count;
		XS_BusSetLastError(XS_BUS_ERR_DATA_LIMIT, "data limit exceeded");
		XS_LogWarn(
			"bus register failed: data limit exceeded: limit=%lld count=%lld namespace=%s tag=%s",
			(long long)g_objXsBus.iDataLimit,
			(long long)g_objXsBus.arrDataItems->Count,
			sNamespace ? sNamespace : "(null)",
			sTag ? sTag : "(null)"
		);
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return 0;
	}
	if ( g_objXsBus.iNamespaceLimit > 0 || g_objXsBus.iNamespaceDataLimit > 0 ) {
		int64 iNamespaceDataCount = XS_BusNamespaceDataCount_NoLock(sNamespace);

		if ( g_objXsBus.iNamespaceLimit > 0 && iNamespaceDataCount == 0 ) {
			int64 iNamespaceCount = XS_BusNamespaceCount_NoLock();

			if ( iNamespaceCount >= g_objXsBus.iNamespaceLimit ) {
				g_objXsBus.iNamespaceLimitRejectCount++;
				g_objXsBus.tLastNamespaceLimitReject = xrtNow();
				g_objXsBus.iLastNamespaceLimitCount = iNamespaceCount;
				XS_BusSetNamespaceLabel(g_objXsBus.sLastNamespaceLimitNamespace, sizeof(g_objXsBus.sLastNamespaceLimitNamespace), sNamespace);
				XS_BusSetLastError(XS_BUS_ERR_NAMESPACE_LIMIT, "namespace limit exceeded");
				XS_LogWarn(
					"bus register failed: namespace limit exceeded: limit=%lld count=%lld namespace=%s tag=%s",
					(long long)g_objXsBus.iNamespaceLimit,
					(long long)iNamespaceCount,
					XS_BusNamespaceLabel(sNamespace),
					sTag ? sTag : "(null)"
				);
				xrtMutexUnlock(g_objXsBus.pLock);
				xvoUnref(objStore);
				return 0;
			}
		}
		if ( g_objXsBus.iNamespaceDataLimit > 0 && iNamespaceDataCount >= g_objXsBus.iNamespaceDataLimit ) {
			g_objXsBus.iNamespaceDataLimitRejectCount++;
			g_objXsBus.tLastNamespaceDataLimitReject = xrtNow();
			g_objXsBus.iLastNamespaceDataLimitCount = iNamespaceDataCount;
			XS_BusSetNamespaceLabel(g_objXsBus.sLastNamespaceDataLimitNamespace, sizeof(g_objXsBus.sLastNamespaceDataLimitNamespace), sNamespace);
			XS_BusSetLastError(XS_BUS_ERR_NAMESPACE_DATA_LIMIT, "namespace data limit exceeded");
			XS_LogWarn(
				"bus register failed: namespace data limit exceeded: limit=%lld count=%lld namespace=%s tag=%s",
				(long long)g_objXsBus.iNamespaceDataLimit,
				(long long)iNamespaceDataCount,
				XS_BusNamespaceLabel(sNamespace),
				sTag ? sTag : "(null)"
			);
			xrtMutexUnlock(g_objXsBus.pLock);
			xvoUnref(objStore);
			return 0;
		}
	}
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
	
	XS_BusSweepExpiredDataForce();
	
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

	XS_BusSweepExpiredDataForce();

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
	xvoTableSetInt(objRet, "namespace_count", sizeof("namespace_count") - 1, XS_BusNamespaceCount_NoLock());
	xvoTableSetInt(objRet, "next_data_id", 12, g_objXsBus.iNextDataID);
	xvoTableSetInt(objRet, "next_msg_id", 11, g_objXsBus.iNextMsgID);
	XS_BusSetNamespaceLimitStateFields(
		objRet,
		g_objXsBus.iNamespaceLimit,
		xvoTableGetInt(objRet, "namespace_count", sizeof("namespace_count") - 1)
	);
	XS_BusSetLimitRejectFields(
		objRet,
		g_objXsBus.iDataLimit,
		g_objXsBus.iQueueLimit,
		g_objXsBus.iNamespaceLimit,
		g_objXsBus.iNamespaceDataLimit,
		g_objXsBus.sReadonlyNamespaces,
		g_objXsBus.sDisabledNamespaces,
		g_objXsBus.sTTLRequiredNamespaces,
		g_objXsBus.sTagRequiredNamespaces,
		g_objXsBus.iDataLimitRejectCount,
		g_objXsBus.iQueueLimitRejectCount,
		g_objXsBus.iNamespaceLimitRejectCount,
		g_objXsBus.iNamespaceDataLimitRejectCount,
		g_objXsBus.iReadonlyNamespaceRejectCount,
		g_objXsBus.iDisabledNamespaceRejectCount,
		g_objXsBus.iTTLRequiredNamespaceRejectCount,
		g_objXsBus.iTagRequiredNamespaceRejectCount,
		g_objXsBus.tLastDataLimitReject,
		g_objXsBus.tLastQueueLimitReject,
		g_objXsBus.tLastNamespaceLimitReject,
		g_objXsBus.tLastNamespaceDataLimitReject,
		g_objXsBus.tLastReadonlyNamespaceReject,
		g_objXsBus.tLastDisabledNamespaceReject,
		g_objXsBus.tLastTTLRequiredNamespaceReject,
		g_objXsBus.tLastTagRequiredNamespaceReject,
		g_objXsBus.iLastDataLimitCount,
		g_objXsBus.iLastQueueLimitCount,
		g_objXsBus.iLastNamespaceLimitCount,
		g_objXsBus.iLastNamespaceDataLimitCount,
		g_objXsBus.sLastNamespaceLimitNamespace,
		g_objXsBus.sLastNamespaceDataLimitNamespace,
		g_objXsBus.sLastReadonlyNamespace,
		g_objXsBus.sLastReadonlyNamespaceAction,
		g_objXsBus.sLastDisabledNamespace,
		g_objXsBus.sLastDisabledNamespaceAction,
		g_objXsBus.sLastTTLRequiredNamespace,
		g_objXsBus.sLastTTLRequiredNamespaceAction,
		g_objXsBus.sLastTagRequiredNamespace,
		g_objXsBus.sLastTagRequiredNamespaceAction
	);
	XS_BusSetSweepFields(
		objRet,
		g_objXsBus.iSweepIntervalMS,
		g_objXsBus.iSweepCount,
		g_objXsBus.iSweepRemovedCount,
		g_objXsBus.tLastSweep,
		g_objXsBus.iLastSweepRemoved,
		g_objXsBus.iLastSweepRemain,
		g_objXsBus.iCleanupCount,
		g_objXsBus.tLastCleanup,
		g_objXsBus.iLastCleanupRemoved,
		g_objXsBus.iLastCleanupRemain
	);
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

	XS_BusClearLastError();
	
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

static inline bool XS_BusDataRetainManaged(int64 iID)
{
	XS_BusDataItem* objItem;

	XS_BusClearLastError();

	if ( iID <= 0 ) {
		return FALSE;
	}

	xrtMutexLock(g_objXsBus.pLock);
	objItem = XS_BusFindDataItemByID_NoLock(iID);
	if ( objItem == NULL || objItem->objValue == NULL ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		return FALSE;
	}
	if ( XS_BusCheckNamespacePolicy_NoLock(objItem->sNamespace, "retain", TRUE) != XS_BUS_ERR_NONE ) {
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

	XS_BusClearLastError();

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
	if ( XS_BusCheckNamespacePolicy_NoLock(objItem->sNamespace, "touch", TRUE) != XS_BUS_ERR_NONE ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		return FALSE;
	}
	if ( XS_BusCheckNamespaceTagRequiredState_NoLock(objItem->sNamespace, objItem->sTag, "touch") != XS_BUS_ERR_NONE ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		return FALSE;
	}
	if ( XS_BusCheckNamespaceTTLRequiredState_NoLock(objItem->sNamespace, "touch", iTTLSecond > 0) != XS_BUS_ERR_NONE ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		return FALSE;
	}
	objItem->tExpire = (iTTLSecond > 0) ? (xrtNow() + iTTLSecond) : 0;
	xrtMutexUnlock(g_objXsBus.pLock);
	return TRUE;
}

static inline bool XS_BusDataSetValueEx(int64 iID, xvalue objValue, int64 iTTL, bool bSetTTL)
{
	XS_BusDataItem* objItem;
	xvalue objStore;
	xvalue objOld;
	int64 iTTLSecond = 0;
	bool bWillExpire = FALSE;

	XS_BusClearLastError();

	if ( iID <= 0 || objValue == NULL ) {
		return FALSE;
	}
	if ( bSetTTL ) {
		iTTLSecond = XS_BusTTLToSeconds(iTTL);
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
	if ( XS_BusCheckNamespacePolicy_NoLock(objItem->sNamespace, "set", TRUE) != XS_BUS_ERR_NONE ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return FALSE;
	}
	if ( XS_BusCheckNamespaceTagRequiredState_NoLock(objItem->sNamespace, objItem->sTag, "set") != XS_BUS_ERR_NONE ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return FALSE;
	}
	bWillExpire = bSetTTL ? (iTTLSecond > 0) : (objItem->tExpire > 0);
	if ( XS_BusCheckNamespaceTTLRequiredState_NoLock(objItem->sNamespace, "set", bWillExpire) != XS_BUS_ERR_NONE ) {
		xrtMutexUnlock(g_objXsBus.pLock);
		xvoUnref(objStore);
		return FALSE;
	}
	objOld = objItem->objValue;
	objItem->objValue = objStore;
	if ( bSetTTL ) {
		objItem->tExpire = (iTTLSecond > 0) ? (xrtNow() + iTTLSecond) : 0;
	}
	xrtMutexUnlock(g_objXsBus.pLock);

	if ( objOld ) {
		xvoUnref(objOld);
	}
	return TRUE;
}

static inline bool XS_BusDataSetValue(int64 iID, xvalue objValue)
{
	return XS_BusDataSetValueEx(iID, objValue, 0, FALSE);
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

	XS_BusClearLastError();
	
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

static inline bool XS_BusDataReleaseManaged(int64 iID)
{
	uint32 i;
	XS_BusDataItem objItem;

	XS_BusClearLastError();

	if ( iID <= 0 ) {
		return FALSE;
	}

	xrtMutexLock(g_objXsBus.pLock);
	for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
		XS_BusDataItem* pItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);

		if ( pItem && pItem->iID == iID && pItem->objValue ) {
			if ( XS_BusCheckNamespacePolicy_NoLock(pItem->sNamespace, "release", TRUE) != XS_BUS_ERR_NONE ) {
				xrtMutexUnlock(g_objXsBus.pLock);
				return FALSE;
			}
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

	XS_BusClearLastError();

	if ( iID <= 0 ) {
		return FALSE;
	}

	memset(&objItem, 0, sizeof(objItem));

	xrtMutexLock(g_objXsBus.pLock);
	for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
		XS_BusDataItem* pItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);

		if ( pItem && pItem->iID == iID && pItem->objValue ) {
			if ( XS_BusCheckNamespacePolicy_NoLock(pItem->sNamespace, "remove", TRUE) != XS_BUS_ERR_NONE ) {
				xrtMutexUnlock(g_objXsBus.pLock);
				return FALSE;
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
	int64 tNow;
	int64 iNamespaceDataLimit;
	int64 iReadonlyNamespaceCount;
	int64 iDisabledNamespaceCount;
	int64 iTTLRequiredNamespaceCount;
	int64 iTagRequiredNamespaceCount;
	int64 tLastReadonlyNamespaceReject;
	int64 tLastDisabledNamespaceReject;
	char sLastReadonlyNamespace[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
	char sLastReadonlyNamespaceAction[XS_BUS_NAMESPACE_ACTION_MAX_LEN];
	char sLastDisabledNamespace[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
	char sLastDisabledNamespaceAction[XS_BUS_NAMESPACE_ACTION_MAX_LEN];
} XS_BusNamespaceStatsContext;

static bool XS_BusSweepCollectProc(const XS_BusDataItem* objItem, XS_BusSweepContext* objCtx)
{
	if ( objCtx == NULL ) {
		return FALSE;
	}
	
	if ( objItem && objItem->iID > 0 && objItem->objValue && objItem->tExpire > 0 && objItem->tExpire <= objCtx->tNow ) {
		if ( objCtx->iCount < XS_BUS_SWEEP_BATCH_LIMIT ) {
			objCtx->arrID[objCtx->iCount++] = objItem->iID;
		}
	}
	return FALSE;
}

static inline void XS_BusSweepExpiredDataEx(bool bForce)
{
	XS_BusSweepContext objCtx;
	int64 iRemoved = 0;
	int64 iExpiredRemain = 0;
	uint32 i;
	
	if ( g_objXsBus.pLock == NULL ) {
		return;
	}
	
	memset(&objCtx, 0, sizeof(objCtx));
	objCtx.tNow = xrtNow();
	
	xrtMutexLock(g_objXsBus.pLock);
	if (
		!bForce &&
		g_objXsBus.iSweepIntervalMS > 0 &&
		g_objXsBus.tLastSweep > 0 &&
		((objCtx.tNow - g_objXsBus.tLastSweep) * 1000) < g_objXsBus.iSweepIntervalMS
	) {
		xrtMutexUnlock(g_objXsBus.pLock);
		return;
	}
	if ( g_objXsBus.arrDataItems ) {
		for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
			XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);
			if ( objItem && objItem->iID > 0 && objItem->objValue && objItem->tExpire > 0 && objItem->tExpire <= objCtx.tNow ) {
				if ( objCtx.iCount < XS_BUS_SWEEP_BATCH_LIMIT ) {
					(void)XS_BusSweepCollectProc(objItem, &objCtx);
				} else {
					iExpiredRemain++;
				}
			}
		}
	}
	g_objXsBus.iSweepCount++;
	g_objXsBus.tLastSweep = objCtx.tNow;
	g_objXsBus.iLastSweepRemoved = 0;
	g_objXsBus.iLastSweepRemain = iExpiredRemain;
	xrtMutexUnlock(g_objXsBus.pLock);
	
	for ( i = 0; i < objCtx.iCount; i++ ) {
		if ( XS_BusDataRemove(objCtx.arrID[i]) ) {
			iRemoved++;
			XS_LogInfo("bus data expired: id=%lld", (long long)objCtx.arrID[i]);
		}
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	g_objXsBus.iSweepRemovedCount += iRemoved;
	g_objXsBus.iLastSweepRemoved = iRemoved;
	g_objXsBus.iLastSweepRemain = iExpiredRemain;
	if ( objCtx.iCount > 0 || iExpiredRemain > 0 ) {
		g_objXsBus.iCleanupCount++;
		g_objXsBus.tLastCleanup = objCtx.tNow;
		g_objXsBus.iLastCleanupRemoved = iRemoved;
		g_objXsBus.iLastCleanupRemain = iExpiredRemain;
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	
	if ( iExpiredRemain > 0 ) {
		XS_LogWarn(
			"bus sweep remain expired data: removed=%lld remain=%lld batch=%d",
			(long long)iRemoved,
			(long long)iExpiredRemain,
			(int)XS_BUS_SWEEP_BATCH_LIMIT
		);
	}
}

static inline void XS_BusSweepExpiredData(void)
{
	XS_BusSweepExpiredDataEx(FALSE);
}

static inline void XS_BusSweepExpiredDataForce(void)
{
	XS_BusSweepExpiredDataEx(TRUE);
}

static inline void XS_BusSweepExpiredDataDrain(int32 iMaxPass, int64* piPassCount, int64* piRemovedTotal, int64* piRemain)
{
	int32 iPassLimit = iMaxPass > 0 ? iMaxPass : XS_BUS_SWEEP_DRAIN_MAX_PASS;
	int64 iPassCount = 0;
	int64 iRemovedTotal = 0;
	int64 iLastRemoved = 0;
	int64 iLastRemain = 0;

	while ( iPassCount < iPassLimit ) {
		XS_BusSweepExpiredDataForce();
		xrtMutexLock(g_objXsBus.pLock);
		iLastRemoved = g_objXsBus.iLastSweepRemoved;
		iLastRemain = g_objXsBus.iLastSweepRemain;
		xrtMutexUnlock(g_objXsBus.pLock);
		iPassCount++;
		iRemovedTotal += iLastRemoved;
		if ( iLastRemain <= 0 ) {
			break;
		}
		if ( iLastRemoved <= 0 ) {
			break;
		}
	}

	if ( piPassCount ) {
		*piPassCount = iPassCount;
	}
	if ( piRemovedTotal ) {
		*piRemovedTotal = iRemovedTotal;
	}
	if ( piRemain ) {
		*piRemain = iLastRemain;
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

static bool XS_BusBuildStatusFilterProc(const XS_BusDataItem* objItem, XS_BusFilterContext* objCtx);

static inline xvalue XS_BusBuildStatusValueInternal(const char* sNamespace, const char* sTag, bool bSweep)
{
	xvalue objRet = xvoCreateTable();
	xvalue objItems = xvoCreateArray();
	XS_BusFilterContext objCtx;
	bool bFilter = FALSE;

	memset(&objCtx, 0, sizeof(objCtx));
	if ( (sNamespace && sNamespace[0] != '\0') || (sTag && sTag[0] != '\0') ) {
		bFilter = TRUE;
		objCtx.sNamespace = sNamespace;
		objCtx.sTag = sTag;
		objCtx.objItems = objItems;
	}
	if ( bSweep ) {
		XS_BusSweepExpiredData();
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	xvoTableSetInt(objRet, "queue_count", 11, g_objXsBus.arrMessageQueue ? g_objXsBus.arrMessageQueue->Count : 0);
	xvoTableSetInt(objRet, "data_count", 10, g_objXsBus.arrDataItems ? g_objXsBus.arrDataItems->Count : 0);
	xvoTableSetInt(objRet, "namespace_count", sizeof("namespace_count") - 1, XS_BusNamespaceCount_NoLock());
	xvoTableSetInt(objRet, "next_data_id", 12, g_objXsBus.iNextDataID);
	xvoTableSetInt(objRet, "next_msg_id", 11, g_objXsBus.iNextMsgID);
	xvoTableSetInt(objRet, "total_queued", 12, g_objXsBus.iTotalQueued);
	xvoTableSetInt(objRet, "total_delivered", 15, g_objXsBus.iTotalDelivered);
	xvoTableSetInt(objRet, "total_dropped", 13, g_objXsBus.iTotalDropped);
	xvoTableSetInt(objRet, "last_queue_time", 15, g_objXsBus.tLastQueued);
	xvoTableSetInt(objRet, "last_dispatch_time", 18, g_objXsBus.tLastDispatch);
	XS_BusSetTimeFields(objRet, g_objXsBus.tLastQueued, g_objXsBus.tLastDispatch);
	XS_BusSetNamespaceLimitStateFields(
		objRet,
		g_objXsBus.iNamespaceLimit,
		xvoTableGetInt(objRet, "namespace_count", sizeof("namespace_count") - 1)
	);
	XS_BusSetLimitRejectFields(
		objRet,
		g_objXsBus.iDataLimit,
		g_objXsBus.iQueueLimit,
		g_objXsBus.iNamespaceLimit,
		g_objXsBus.iNamespaceDataLimit,
		g_objXsBus.sReadonlyNamespaces,
		g_objXsBus.sDisabledNamespaces,
		g_objXsBus.sTTLRequiredNamespaces,
		g_objXsBus.sTagRequiredNamespaces,
		g_objXsBus.iDataLimitRejectCount,
		g_objXsBus.iQueueLimitRejectCount,
		g_objXsBus.iNamespaceLimitRejectCount,
		g_objXsBus.iNamespaceDataLimitRejectCount,
		g_objXsBus.iReadonlyNamespaceRejectCount,
		g_objXsBus.iDisabledNamespaceRejectCount,
		g_objXsBus.iTTLRequiredNamespaceRejectCount,
		g_objXsBus.iTagRequiredNamespaceRejectCount,
		g_objXsBus.tLastDataLimitReject,
		g_objXsBus.tLastQueueLimitReject,
		g_objXsBus.tLastNamespaceLimitReject,
		g_objXsBus.tLastNamespaceDataLimitReject,
		g_objXsBus.tLastReadonlyNamespaceReject,
		g_objXsBus.tLastDisabledNamespaceReject,
		g_objXsBus.tLastTTLRequiredNamespaceReject,
		g_objXsBus.tLastTagRequiredNamespaceReject,
		g_objXsBus.iLastDataLimitCount,
		g_objXsBus.iLastQueueLimitCount,
		g_objXsBus.iLastNamespaceLimitCount,
		g_objXsBus.iLastNamespaceDataLimitCount,
		g_objXsBus.sLastNamespaceLimitNamespace,
		g_objXsBus.sLastNamespaceDataLimitNamespace,
		g_objXsBus.sLastReadonlyNamespace,
		g_objXsBus.sLastReadonlyNamespaceAction,
		g_objXsBus.sLastDisabledNamespace,
		g_objXsBus.sLastDisabledNamespaceAction,
		g_objXsBus.sLastTTLRequiredNamespace,
		g_objXsBus.sLastTTLRequiredNamespaceAction,
		g_objXsBus.sLastTagRequiredNamespace,
		g_objXsBus.sLastTagRequiredNamespaceAction
	);
	XS_BusSetSweepFields(
		objRet,
		g_objXsBus.iSweepIntervalMS,
		g_objXsBus.iSweepCount,
		g_objXsBus.iSweepRemovedCount,
		g_objXsBus.tLastSweep,
		g_objXsBus.iLastSweepRemoved,
		g_objXsBus.iLastSweepRemain,
		g_objXsBus.iCleanupCount,
		g_objXsBus.tLastCleanup,
		g_objXsBus.iLastCleanupRemoved,
		g_objXsBus.iLastCleanupRemain
	);
	if ( g_objXsBus.arrDataItems ) {
		uint32 i;

		for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
			XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);
			if ( bFilter ) {
				(void)XS_BusBuildStatusFilterProc(objItem, &objCtx);
			} else {
				(void)XS_BusBuildStatusProc(objItem, objItems);
			}
		}
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	if ( bFilter ) {
		xvoTableSetText(objRet, "namespace", 9, (ptr)(sNamespace ? sNamespace : ""), 0, FALSE);
		xvoTableSetText(objRet, "tag", 3, (ptr)(sTag ? sTag : ""), 0, FALSE);
		xvoTableSetInt(objRet, "match_count", 11, objCtx.iCount);
	}
	xvoTableSetValue(objRet, "items", 5, objItems, TRUE);
	return objRet;
}

static inline xvalue XS_BusBuildStatusValue(void)
{
	return XS_BusBuildStatusValueInternal(NULL, NULL, TRUE);
}

static inline xvalue XS_BusBuildStatusValueNoSweep(void)
{
	return XS_BusBuildStatusValueInternal(NULL, NULL, FALSE);
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
	return XS_BusBuildStatusValueInternal(sNamespace, sTag, TRUE);
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

static inline void XS_BusSetNamespaceLimitStateFields(xvalue objItem, int64 iNamespaceLimit, int64 iNamespaceCount)
{
	int64 iRemaining;

	if ( objItem == NULL ) {
		return;
	}

	iRemaining = 0;
	if ( iNamespaceLimit > 0 && iNamespaceCount < iNamespaceLimit ) {
		iRemaining = iNamespaceLimit - iNamespaceCount;
	}
	xvoTableSetInt(objItem, "namespace_limit_remaining", sizeof("namespace_limit_remaining") - 1, iRemaining);
	xvoTableSetBool(objItem, "namespace_limit_reached", sizeof("namespace_limit_reached") - 1, iNamespaceLimit > 0 && iNamespaceCount >= iNamespaceLimit);
}

static inline xvalue XS_BusFindNamespaceStatsItem(xvalue objItems, const char* sNamespace)
{
	uint32 i;

	if ( objItems == NULL || sNamespace == NULL ) {
		return NULL;
	}

	for ( i = 0; i < xvoArrayItemCount(objItems); i++ ) {
		xvalue objItem = xvoArrayGetValue(objItems, i);
		const char* sExistNamespace;

		if ( objItem == NULL ) {
			continue;
		}

		sExistNamespace = xvoGetText(xvoTableGetValue(objItem, "namespace", 9));
		if ( sExistNamespace && strcmp(sExistNamespace, sNamespace) == 0 ) {
			return objItem;
		}
	}

	return NULL;
}

static inline void XS_BusSetNamespacePolicyFields(xvalue objItem, bool bReadonly, bool bDisabled, bool bTTLRequired, bool bTagRequired)
{
	bool bItemReadonly;
	bool bItemDisabled;
	bool bItemTTLRequired;
	bool bItemTagRequired;

	if ( objItem == NULL ) {
		return;
	}

	bItemReadonly = xvoGetBool(xvoTableGetValue(objItem, "readonly", 8)) || bReadonly;
	bItemDisabled = xvoGetBool(xvoTableGetValue(objItem, "disabled", 8)) || bDisabled;
	bItemTTLRequired = xvoGetBool(xvoTableGetValue(objItem, "ttl_required", sizeof("ttl_required") - 1)) || bTTLRequired;
	bItemTagRequired = xvoGetBool(xvoTableGetValue(objItem, "tag_required", sizeof("tag_required") - 1)) || bTagRequired;
	xvoTableSetBool(objItem, "readonly", 8, bItemReadonly);
	xvoTableSetBool(objItem, "disabled", 8, bItemDisabled);
	xvoTableSetBool(objItem, "ttl_required", sizeof("ttl_required") - 1, bItemTTLRequired);
	xvoTableSetBool(objItem, "tag_required", sizeof("tag_required") - 1, bItemTagRequired);
	xvoTableSetText(objItem, "policy_state", sizeof("policy_state") - 1, bItemDisabled ? "disabled" : (bItemReadonly ? "readonly" : (bItemTTLRequired ? "ttl_required" : (bItemTagRequired ? "tag_required" : "active"))), 0, FALSE);
}

static inline void XS_BusSetNamespacePolicyRejectStateFields_NoLock(xvalue objItem, int64 tNow)
{
	const char* sNamespace;
	int64 iReadonlyRejectCount = 0;
	int64 iDisabledRejectCount = 0;
	int64 iTTLRequiredRejectCount = 0;
	int64 iTagRequiredRejectCount = 0;
	int64 tLastReadonlyReject = 0;
	int64 tLastDisabledReject = 0;
	int64 tLastTTLRequiredReject = 0;
	int64 tLastTagRequiredReject = 0;
	const char* sLastReadonlyAction = "";
	const char* sLastDisabledAction = "";
	const char* sLastTTLRequiredAction = "";
	const char* sLastTagRequiredAction = "";
	const char* sLastReadonlyNamespace = "";
	const char* sLastDisabledNamespace = "";
	const char* sLastTTLRequiredNamespace = "";
	const char* sLastTagRequiredNamespace = "";
	int64 tLastReject = 0;
	const char* sReason = "";
	const char* sAction = "";
	const char* sRejectNamespace = "";
	char* sTimeText = NULL;
	uint32 i;

	if ( objItem == NULL ) {
		return;
	}

	sNamespace = xvoGetText(xvoTableGetValue(objItem, "namespace", 9));
	if ( sNamespace == NULL || g_objXsBus.arrNamespacePolicyStats == NULL ) {
		return;
	}

	for ( i = 1; i <= g_objXsBus.arrNamespacePolicyStats->Count; i++ ) {
		XS_BusNamespacePolicyStat* objStat = xrtArrayGet_Inline(g_objXsBus.arrNamespacePolicyStats, i);

		if ( objStat == NULL || objStat->sNamespace[0] == '\0' ) {
			continue;
		}
		if ( !XS_BusNamespaceRuleTokenMatchesLabel(sNamespace, objStat->sNamespace) ) {
			continue;
		}
		iReadonlyRejectCount += objStat->iReadonlyRejectCount;
		iDisabledRejectCount += objStat->iDisabledRejectCount;
		iTTLRequiredRejectCount += objStat->iTTLRequiredRejectCount;
		iTagRequiredRejectCount += objStat->iTagRequiredRejectCount;
		if ( objStat->tLastReadonlyReject > tLastReadonlyReject ) {
			tLastReadonlyReject = objStat->tLastReadonlyReject;
			sLastReadonlyAction = objStat->sLastReadonlyAction;
			sLastReadonlyNamespace = objStat->sNamespace;
		}
		if ( objStat->tLastDisabledReject > tLastDisabledReject ) {
			tLastDisabledReject = objStat->tLastDisabledReject;
			sLastDisabledAction = objStat->sLastDisabledAction;
			sLastDisabledNamespace = objStat->sNamespace;
		}
		if ( objStat->tLastTTLRequiredReject > tLastTTLRequiredReject ) {
			tLastTTLRequiredReject = objStat->tLastTTLRequiredReject;
			sLastTTLRequiredAction = objStat->sLastTTLRequiredAction;
			sLastTTLRequiredNamespace = objStat->sNamespace;
		}
		if ( objStat->tLastTagRequiredReject > tLastTagRequiredReject ) {
			tLastTagRequiredReject = objStat->tLastTagRequiredReject;
			sLastTagRequiredAction = objStat->sLastTagRequiredAction;
			sLastTagRequiredNamespace = objStat->sNamespace;
		}
	}

	if ( tLastDisabledReject > 0 && tLastDisabledReject >= tLastReadonlyReject && tLastDisabledReject >= tLastTTLRequiredReject && tLastDisabledReject >= tLastTagRequiredReject ) {
		tLastReject = tLastDisabledReject;
		sReason = "disabled";
		sAction = sLastDisabledAction;
		sRejectNamespace = sLastDisabledNamespace;
	} else if ( tLastReadonlyReject > 0 && tLastReadonlyReject >= tLastTTLRequiredReject && tLastReadonlyReject >= tLastTagRequiredReject ) {
		tLastReject = tLastReadonlyReject;
		sReason = "readonly";
		sAction = sLastReadonlyAction;
		sRejectNamespace = sLastReadonlyNamespace;
	} else if ( tLastTTLRequiredReject > 0 && tLastTTLRequiredReject >= tLastTagRequiredReject ) {
		tLastReject = tLastTTLRequiredReject;
		sReason = "ttl_required";
		sAction = sLastTTLRequiredAction;
		sRejectNamespace = sLastTTLRequiredNamespace;
	} else if ( tLastTagRequiredReject > 0 ) {
		tLastReject = tLastTagRequiredReject;
		sReason = "tag_required";
		sAction = sLastTagRequiredAction;
		sRejectNamespace = sLastTagRequiredNamespace;
	}

	sTimeText = XS_BusTimeText(tLastReject);
	xvoTableSetInt(objItem, "policy_reject_count", sizeof("policy_reject_count") - 1, iReadonlyRejectCount + iDisabledRejectCount + iTTLRequiredRejectCount + iTagRequiredRejectCount);
	xvoTableSetInt(objItem, "readonly_reject_count", sizeof("readonly_reject_count") - 1, iReadonlyRejectCount);
	xvoTableSetInt(objItem, "disabled_reject_count", sizeof("disabled_reject_count") - 1, iDisabledRejectCount);
	xvoTableSetInt(objItem, "ttl_required_reject_count", sizeof("ttl_required_reject_count") - 1, iTTLRequiredRejectCount);
	xvoTableSetInt(objItem, "tag_required_reject_count", sizeof("tag_required_reject_count") - 1, iTagRequiredRejectCount);
	xvoTableSetBool(objItem, "last_policy_reject_hit", sizeof("last_policy_reject_hit") - 1, tLastReject > 0);
	xvoTableSetInt(objItem, "last_policy_reject_time", sizeof("last_policy_reject_time") - 1, tLastReject);
	xvoTableSetText(objItem, "last_policy_reject_time_text", sizeof("last_policy_reject_time_text") - 1, (ptr)(sTimeText ? sTimeText : ""), 0, FALSE);
	xvoTableSetInt(objItem, "last_policy_reject_age_ms", sizeof("last_policy_reject_age_ms") - 1, (tLastReject > 0) ? ((tNow - tLastReject) * 1000) : -1);
	xvoTableSetText(objItem, "last_policy_reject_reason", sizeof("last_policy_reject_reason") - 1, (ptr)sReason, 0, FALSE);
	xvoTableSetText(objItem, "last_policy_reject_action", sizeof("last_policy_reject_action") - 1, (ptr)sAction, 0, FALSE);
	xvoTableSetText(objItem, "last_policy_reject_namespace", sizeof("last_policy_reject_namespace") - 1, (ptr)sRejectNamespace, 0, FALSE);
	if ( sTimeText ) {
		xrtFree(sTimeText);
	}
}

static inline void XS_BusEnsureNamespaceStatsPolicyItems_NoLock(XS_BusNamespaceStatsContext* objCtx, const char* sRules, bool bReadonly, bool bDisabled, bool bTTLRequired, bool bTagRequired)
{
	const char* sPos;

	if ( objCtx == NULL || objCtx->objItems == NULL || sRules == NULL || sRules[0] == '\0' ) {
		return;
	}

	sPos = sRules;
	while ( sPos[0] != '\0' ) {
		const char* sNext = strchr(sPos, ',');
		size_t iTokenLen = sNext ? (size_t)(sNext - sPos) : strlen(sPos);
		char sNamespace[XS_BUS_NAMESPACE_LABEL_MAX_LEN];
		xvalue objItem;

		if ( iTokenLen > 0 && iTokenLen < sizeof(sNamespace) ) {
			memcpy(sNamespace, sPos, iTokenLen);
			sNamespace[iTokenLen] = '\0';
			objItem = XS_BusFindNamespaceStatsItem(objCtx->objItems, sNamespace);
			if ( objItem == NULL ) {
				objItem = xvoCreateTable();
				xvoTableSetText(objItem, "namespace", 9, sNamespace, 0, FALSE);
				xvoTableSetInt(objItem, "count", 5, 0);
				xvoTableSetInt(objItem, "persistent_count", sizeof("persistent_count") - 1, 0);
				xvoTableSetInt(objItem, "ttl_count", sizeof("ttl_count") - 1, 0);
				xvoTableSetInt(objItem, "oldest_create_time", sizeof("oldest_create_time") - 1, 0);
				xvoTableSetInt(objItem, "newest_create_time", sizeof("newest_create_time") - 1, 0);
				xvoTableSetInt(objItem, "next_expire_time", sizeof("next_expire_time") - 1, 0);
				xvoArrayAppendValue(objCtx->objItems, objItem, TRUE);
			}
			XS_BusSetNamespacePolicyFields(objItem, bReadonly, bDisabled, bTTLRequired, bTagRequired);
		}

		if ( sNext == NULL ) {
			break;
		}
		sPos = sNext + 1;
	}
}

static inline void XS_BusFinalizeNamespaceStatsItem(xvalue objItem, int64 tNow, int64 iNamespaceDataLimit)
{
	int64 iCount;
	int64 tOldestCreate;
	int64 tNewestCreate;
	int64 tNextExpire;
	int64 iNextExpireInMS;
	int64 iRemaining;
	char* sOldestCreateTime;
	char* sNewestCreateTime;
	char* sNextExpireTime;

	if ( objItem == NULL ) {
		return;
	}

	iCount = xvoGetInt(xvoTableGetValue(objItem, "count", 5));
	tOldestCreate = xvoGetInt(xvoTableGetValue(objItem, "oldest_create_time", sizeof("oldest_create_time") - 1));
	tNewestCreate = xvoGetInt(xvoTableGetValue(objItem, "newest_create_time", sizeof("newest_create_time") - 1));
	tNextExpire = xvoGetInt(xvoTableGetValue(objItem, "next_expire_time", sizeof("next_expire_time") - 1));
	iNextExpireInMS = -1;
	if ( tNextExpire > 0 ) {
		iNextExpireInMS = (tNextExpire - tNow) * 1000;
		if ( iNextExpireInMS < 0 ) {
			iNextExpireInMS = 0;
		}
	}
	iRemaining = 0;
	if ( iNamespaceDataLimit > 0 && iCount < iNamespaceDataLimit ) {
		iRemaining = iNamespaceDataLimit - iCount;
	}

	sOldestCreateTime = XS_BusTimeText(tOldestCreate);
	sNewestCreateTime = XS_BusTimeText(tNewestCreate);
	sNextExpireTime = XS_BusTimeText(tNextExpire);
	xvoTableSetText(objItem, "oldest_create_time_text", sizeof("oldest_create_time_text") - 1, (ptr)(sOldestCreateTime ? sOldestCreateTime : ""), 0, FALSE);
	xvoTableSetInt(objItem, "oldest_create_age_ms", sizeof("oldest_create_age_ms") - 1, XS_BusTimeAgeMS(tOldestCreate));
	xvoTableSetText(objItem, "newest_create_time_text", sizeof("newest_create_time_text") - 1, (ptr)(sNewestCreateTime ? sNewestCreateTime : ""), 0, FALSE);
	xvoTableSetInt(objItem, "newest_create_age_ms", sizeof("newest_create_age_ms") - 1, XS_BusTimeAgeMS(tNewestCreate));
	xvoTableSetText(objItem, "next_expire_time_text", sizeof("next_expire_time_text") - 1, (ptr)(sNextExpireTime ? sNextExpireTime : ""), 0, FALSE);
	xvoTableSetInt(objItem, "next_expire_in_ms", sizeof("next_expire_in_ms") - 1, iNextExpireInMS);
	xvoTableSetInt(objItem, "namespace_data_limit", sizeof("namespace_data_limit") - 1, iNamespaceDataLimit);
	xvoTableSetInt(objItem, "namespace_data_limit_remaining", sizeof("namespace_data_limit_remaining") - 1, iRemaining);
	xvoTableSetBool(objItem, "namespace_data_limit_reached", sizeof("namespace_data_limit_reached") - 1, iNamespaceDataLimit > 0 && iCount >= iNamespaceDataLimit);
	if ( sOldestCreateTime ) {
		xrtFree(sOldestCreateTime);
	}
	if ( sNewestCreateTime ) {
		xrtFree(sNewestCreateTime);
	}
	if ( sNextExpireTime ) {
		xrtFree(sNextExpireTime);
	}
}

static bool XS_BusBuildNamespaceStatsProc(const XS_BusDataItem* objData, XS_BusNamespaceStatsContext* objCtx)
{
	const char* sNamespace;
	const char* sRuleNamespace;
	uint32 i;
	xvalue objItem;
	int64 iNamespaceCount;
	int64 iTTLCount;
	int64 iPersistentCount;
	int64 tOldestCreate;
	int64 tNewestCreate;
	int64 tNextExpire;
	bool bReadonly = FALSE;
	bool bDisabled = FALSE;
	bool bTTLRequired = FALSE;
	bool bTagRequired = FALSE;
	
	if ( objCtx == NULL || objCtx->objItems == NULL || objData == NULL || objData->objValue == NULL ) {
		return FALSE;
	}
	
	sNamespace = objData->sNamespace;
	sRuleNamespace = (objData->sNamespace && objData->sNamespace[0] != '\0') ? objData->sNamespace : NULL;
	if ( sNamespace == NULL || sNamespace[0] == '\0' ) {
		sNamespace = "(default)";
	}
	
	for ( i = 0; i < xvoArrayItemCount(objCtx->objItems); i++ ) {
		xvalue objExist = xvoArrayGetValue(objCtx->objItems, i);
		const char* sExistNamespace;
		
		if ( objExist == NULL ) {
			continue;
		}
		
		sExistNamespace = xvoGetText(xvoTableGetValue(objExist, "namespace", 9));
		if ( sExistNamespace && strcmp(sExistNamespace, sNamespace) == 0 ) {
			iNamespaceCount = xvoGetInt(xvoTableGetValue(objExist, "count", 5));
			xvoTableSetInt(objExist, "count", 5, iNamespaceCount + 1);
			if ( objData->tExpire > 0 ) {
				iTTLCount = xvoGetInt(xvoTableGetValue(objExist, "ttl_count", sizeof("ttl_count") - 1));
				xvoTableSetInt(objExist, "ttl_count", sizeof("ttl_count") - 1, iTTLCount + 1);
				tNextExpire = xvoGetInt(xvoTableGetValue(objExist, "next_expire_time", sizeof("next_expire_time") - 1));
				if ( tNextExpire <= 0 || objData->tExpire < tNextExpire ) {
					xvoTableSetInt(objExist, "next_expire_time", sizeof("next_expire_time") - 1, objData->tExpire);
				}
			} else {
				iPersistentCount = xvoGetInt(xvoTableGetValue(objExist, "persistent_count", sizeof("persistent_count") - 1));
				xvoTableSetInt(objExist, "persistent_count", sizeof("persistent_count") - 1, iPersistentCount + 1);
			}
			tOldestCreate = xvoGetInt(xvoTableGetValue(objExist, "oldest_create_time", sizeof("oldest_create_time") - 1));
			tNewestCreate = xvoGetInt(xvoTableGetValue(objExist, "newest_create_time", sizeof("newest_create_time") - 1));
			if ( tOldestCreate <= 0 || (objData->tCreate > 0 && objData->tCreate < tOldestCreate) ) {
				xvoTableSetInt(objExist, "oldest_create_time", sizeof("oldest_create_time") - 1, objData->tCreate);
			}
			if ( objData->tCreate > tNewestCreate ) {
				xvoTableSetInt(objExist, "newest_create_time", sizeof("newest_create_time") - 1, objData->tCreate);
			}
			return FALSE;
		}
	}
	
	objItem = xvoCreateTable();
	XS_BusGetNamespacePolicyState_NoLock(sRuleNamespace, &bReadonly, &bDisabled, &bTTLRequired, &bTagRequired);
	xvoTableSetText(objItem, "namespace", 9, (ptr)sNamespace, 0, FALSE);
	xvoTableSetInt(objItem, "count", 5, 1);
	xvoTableSetInt(objItem, "persistent_count", sizeof("persistent_count") - 1, objData->tExpire > 0 ? 0 : 1);
	xvoTableSetInt(objItem, "ttl_count", sizeof("ttl_count") - 1, objData->tExpire > 0 ? 1 : 0);
	xvoTableSetInt(objItem, "oldest_create_time", sizeof("oldest_create_time") - 1, objData->tCreate);
	xvoTableSetInt(objItem, "newest_create_time", sizeof("newest_create_time") - 1, objData->tCreate);
	xvoTableSetInt(objItem, "next_expire_time", sizeof("next_expire_time") - 1, objData->tExpire);
	XS_BusSetNamespacePolicyFields(objItem, bReadonly, bDisabled, bTTLRequired, bTagRequired);
	xvoArrayAppendValue(objCtx->objItems, objItem, TRUE);
	objCtx->iCount++;
	return FALSE;
}

static inline xvalue XS_BusBuildNamespaceStatsValue(void)
{
	xvalue objRet = xvoCreateTable();
	xvalue objItems = xvoCreateArray();
	XS_BusNamespaceStatsContext objCtx;
	int64 iNamespaceItemCount = 0;
	uint32 i;
	
	memset(&objCtx, 0, sizeof(objCtx));
	objCtx.objItems = objItems;
	objCtx.tNow = xrtNow();
	
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
	objCtx.iNamespaceDataLimit = g_objXsBus.iNamespaceDataLimit;
	objCtx.tLastReadonlyNamespaceReject = g_objXsBus.tLastReadonlyNamespaceReject;
	objCtx.tLastDisabledNamespaceReject = g_objXsBus.tLastDisabledNamespaceReject;
	snprintf(objCtx.sLastReadonlyNamespace, sizeof(objCtx.sLastReadonlyNamespace), "%s", g_objXsBus.sLastReadonlyNamespace);
	snprintf(objCtx.sLastReadonlyNamespaceAction, sizeof(objCtx.sLastReadonlyNamespaceAction), "%s", g_objXsBus.sLastReadonlyNamespaceAction);
	snprintf(objCtx.sLastDisabledNamespace, sizeof(objCtx.sLastDisabledNamespace), "%s", g_objXsBus.sLastDisabledNamespace);
	snprintf(objCtx.sLastDisabledNamespaceAction, sizeof(objCtx.sLastDisabledNamespaceAction), "%s", g_objXsBus.sLastDisabledNamespaceAction);
	XS_BusSetLimitRejectFields(
		objRet,
		g_objXsBus.iDataLimit,
		g_objXsBus.iQueueLimit,
		g_objXsBus.iNamespaceLimit,
		g_objXsBus.iNamespaceDataLimit,
		g_objXsBus.sReadonlyNamespaces,
		g_objXsBus.sDisabledNamespaces,
		g_objXsBus.sTTLRequiredNamespaces,
		g_objXsBus.sTagRequiredNamespaces,
		g_objXsBus.iDataLimitRejectCount,
		g_objXsBus.iQueueLimitRejectCount,
		g_objXsBus.iNamespaceLimitRejectCount,
		g_objXsBus.iNamespaceDataLimitRejectCount,
		g_objXsBus.iReadonlyNamespaceRejectCount,
		g_objXsBus.iDisabledNamespaceRejectCount,
		g_objXsBus.iTTLRequiredNamespaceRejectCount,
		g_objXsBus.iTagRequiredNamespaceRejectCount,
		g_objXsBus.tLastDataLimitReject,
		g_objXsBus.tLastQueueLimitReject,
		g_objXsBus.tLastNamespaceLimitReject,
		g_objXsBus.tLastNamespaceDataLimitReject,
		g_objXsBus.tLastReadonlyNamespaceReject,
		g_objXsBus.tLastDisabledNamespaceReject,
		g_objXsBus.tLastTTLRequiredNamespaceReject,
		g_objXsBus.tLastTagRequiredNamespaceReject,
		g_objXsBus.iLastDataLimitCount,
		g_objXsBus.iLastQueueLimitCount,
		g_objXsBus.iLastNamespaceLimitCount,
		g_objXsBus.iLastNamespaceDataLimitCount,
		g_objXsBus.sLastNamespaceLimitNamespace,
		g_objXsBus.sLastNamespaceDataLimitNamespace,
		g_objXsBus.sLastReadonlyNamespace,
		g_objXsBus.sLastReadonlyNamespaceAction,
		g_objXsBus.sLastDisabledNamespace,
		g_objXsBus.sLastDisabledNamespaceAction,
		g_objXsBus.sLastTTLRequiredNamespace,
		g_objXsBus.sLastTTLRequiredNamespaceAction,
		g_objXsBus.sLastTagRequiredNamespace,
		g_objXsBus.sLastTagRequiredNamespaceAction
	);
	XS_BusSetSweepFields(
		objRet,
		g_objXsBus.iSweepIntervalMS,
		g_objXsBus.iSweepCount,
		g_objXsBus.iSweepRemovedCount,
		g_objXsBus.tLastSweep,
		g_objXsBus.iLastSweepRemoved,
		g_objXsBus.iLastSweepRemain,
		g_objXsBus.iCleanupCount,
		g_objXsBus.tLastCleanup,
		g_objXsBus.iLastCleanupRemoved,
		g_objXsBus.iLastCleanupRemain
	);
	if ( g_objXsBus.arrDataItems ) {
		for ( i = 1; i <= g_objXsBus.arrDataItems->Count; i++ ) {
			XS_BusDataItem* objItem = xrtArrayGet_Inline(g_objXsBus.arrDataItems, i);
			(void)XS_BusBuildNamespaceStatsProc(objItem, &objCtx);
		}
	}
	XS_BusEnsureNamespaceStatsPolicyItems_NoLock(&objCtx, g_objXsBus.sReadonlyNamespaces, TRUE, FALSE, FALSE, FALSE);
	XS_BusEnsureNamespaceStatsPolicyItems_NoLock(&objCtx, g_objXsBus.sDisabledNamespaces, FALSE, TRUE, FALSE, FALSE);
	XS_BusEnsureNamespaceStatsPolicyItems_NoLock(&objCtx, g_objXsBus.sTTLRequiredNamespaces, FALSE, FALSE, TRUE, FALSE);
	XS_BusEnsureNamespaceStatsPolicyItems_NoLock(&objCtx, g_objXsBus.sTagRequiredNamespaces, FALSE, FALSE, FALSE, TRUE);
	for ( i = 0; i < xvoArrayItemCount(objItems); i++ ) {
		xvalue objItem = xvoArrayGetValue(objItems, i);

		XS_BusSetNamespacePolicyRejectStateFields_NoLock(objItem, objCtx.tNow);
	}
	xrtMutexUnlock(g_objXsBus.pLock);
	
	xvoTableSetInt(objRet, "namespace_count", 15, objCtx.iCount);
	XS_BusSetNamespaceLimitStateFields(objRet, xvoTableGetInt(objRet, "namespace_limit", sizeof("namespace_limit") - 1), objCtx.iCount);
	for ( i = 0; i < xvoArrayItemCount(objItems); i++ ) {
		xvalue objItem = xvoArrayGetValue(objItems, i);

		XS_BusFinalizeNamespaceStatsItem(objItem, objCtx.tNow, objCtx.iNamespaceDataLimit);
		iNamespaceItemCount += xvoGetInt(xvoTableGetValue(objItem, "count", 5));
		if ( xvoGetBool(xvoTableGetValue(objItem, "readonly", 8)) ) {
			objCtx.iReadonlyNamespaceCount++;
		}
		if ( xvoGetBool(xvoTableGetValue(objItem, "disabled", 8)) ) {
			objCtx.iDisabledNamespaceCount++;
		}
		if ( xvoGetBool(xvoTableGetValue(objItem, "ttl_required", sizeof("ttl_required") - 1)) ) {
			objCtx.iTTLRequiredNamespaceCount++;
		}
		if ( xvoGetBool(xvoTableGetValue(objItem, "tag_required", sizeof("tag_required") - 1)) ) {
			objCtx.iTagRequiredNamespaceCount++;
		}
	}
	xvoTableSetInt(objRet, "namespace_item_count", sizeof("namespace_item_count") - 1, iNamespaceItemCount);
	xvoTableSetInt(objRet, "readonly_namespace_count", sizeof("readonly_namespace_count") - 1, objCtx.iReadonlyNamespaceCount);
	xvoTableSetInt(objRet, "disabled_namespace_count", sizeof("disabled_namespace_count") - 1, objCtx.iDisabledNamespaceCount);
	xvoTableSetInt(objRet, "ttl_required_namespace_count", sizeof("ttl_required_namespace_count") - 1, objCtx.iTTLRequiredNamespaceCount);
	xvoTableSetInt(objRet, "tag_required_namespace_count", sizeof("tag_required_namespace_count") - 1, objCtx.iTagRequiredNamespaceCount);
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
	
	XS_BusSweepExpiredDataForce();
	
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
	
	XS_BusSweepExpiredDataForce();
	
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

	XS_BusClearLastError();
	
	if ( sTopic == NULL || sTopic[0] == '\0' ) {
		return FALSE;
	}

	if ( iDataID > 0 ) {
		XS_BusDataItem* objData;
		int32 iPolicyCode;
		char sNamespaceLabel[XS_BUS_NAMESPACE_LABEL_MAX_LEN];

		sNamespaceLabel[0] = '\0';

		xrtMutexLock(g_objXsBus.pLock);
		objData = XS_BusFindDataItemByID_NoLock(iDataID);
		if ( objData == NULL || objData->objValue == NULL ) {
			xrtMutexUnlock(g_objXsBus.pLock);
			return FALSE;
		}
		XS_BusSetNamespaceLabel(sNamespaceLabel, sizeof(sNamespaceLabel), objData->sNamespace);
		iPolicyCode = XS_BusCheckNamespacePolicy_NoLock(objData->sNamespace, "send", FALSE);
		xrtMutexUnlock(g_objXsBus.pLock);
		if ( iPolicyCode != XS_BUS_ERR_NONE ) {
			XS_LogWarn(
				"bus send failed: %s: topic=%s namespace=%s server=%s host=%s",
				XS_BusGetLastError(),
				sTopic ? sTopic : "(null)",
				sNamespaceLabel,
				sServer ? sServer : "(null)",
				sHost ? sHost : "(null)"
			);
			return FALSE;
		}
	}
	
	if ( iDataID > 0 && !XS_BusDataRetain(iDataID) ) {
		return FALSE;
	}
	
	xrtMutexLock(g_objXsBus.pLock);
	if ( g_objXsBus.iQueueLimit > 0 && g_objXsBus.arrMessageQueue && (int64)g_objXsBus.arrMessageQueue->Count >= g_objXsBus.iQueueLimit ) {
		g_objXsBus.iQueueLimitRejectCount++;
		g_objXsBus.tLastQueueLimitReject = xrtNow();
		g_objXsBus.iLastQueueLimitCount = g_objXsBus.arrMessageQueue->Count;
		XS_BusSetLastError(XS_BUS_ERR_QUEUE_LIMIT, "queue limit exceeded");
		XS_LogWarn(
			"bus send failed: queue limit exceeded: limit=%lld count=%lld topic=%s server=%s host=%s",
			(long long)g_objXsBus.iQueueLimit,
			(long long)g_objXsBus.arrMessageQueue->Count,
			sTopic ? sTopic : "(null)",
			sServer ? sServer : "(null)",
			sHost ? sHost : "(null)"
		);
		xrtMutexUnlock(g_objXsBus.pLock);
		if ( iDataID > 0 ) {
			(void)XS_BusDataRelease(iDataID);
		}
		return FALSE;
	}
	iPos = xrtArrayAppend(g_objXsBus.arrMessageQueue, 1);
	if ( iPos == 0 ) {
		XS_BusSetLastError(XS_BUS_ERR_QUEUE_SLOT, "queue slot create error");
		XS_LogWarn(
			"bus send failed: queue slot create error: topic=%s err=%s",
			sTopic ? sTopic : "(null)",
			(const char*)(xrtGetError() ? xrtGetError() : (str)"(null)")
		);
		xrtMutexUnlock(g_objXsBus.pLock);
		if ( iDataID > 0 ) {
			(void)XS_BusDataRelease(iDataID);
		}
		return FALSE;
	}
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
	XS_BusClearLastError();
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
		bHandled = FALSE;
		
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
