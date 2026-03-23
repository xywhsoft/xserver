#ifndef XS_CORE_RELOAD_H
#define XS_CORE_RELOAD_H

typedef enum {
	XS_RELOAD_SCRIPT = 1,
	XS_RELOAD_CONFIG = 2
} XS_ReloadType;

typedef struct {
	XS_ReloadType Type;
	bool Force;
} XS_ReloadOptions;

typedef struct {
	bool Force;
	char sServerName[128];
	char sHostName[128];
} XS_ConfigReloadRequest;

typedef struct {
	bool Busy;
	bool Success;
	bool HasResult;
	xtime LastTime;
	int64 iTotalCount;
	int64 iSuccessCount;
	int64 iFailureCount;
	char sServerName[128];
	char sHostName[128];
	char sMessage[256];
} XS_ConfigReloadStatus;

static volatile long g_iXsConfigReloadRequested = 0;
static volatile long g_iXsConfigReloadStateLock = 0;
static XS_ConfigReloadRequest g_tXsConfigReloadRequest;
static XS_ConfigReloadStatus g_tXsConfigReloadStatus;

enum {
	XS_CONFIG_RELOAD_REQ_IDLE = 0,
	XS_CONFIG_RELOAD_REQ_QUEUED = 1,
	XS_CONFIG_RELOAD_REQ_LOCKED = 2
};

static inline void XS_LockConfigReloadState(void)
{
	while ( __xrtAtomicCompareExchange32(&g_iXsConfigReloadStateLock, 1, 0) != 0 ) {
	}
}

static inline void XS_UnlockConfigReloadState(void)
{
	(void)__xrtAtomicExchange32(&g_iXsConfigReloadStateLock, 0);
}

static inline void XS_GetConfigReloadStatusSnapshot(XS_ConfigReloadStatus* pStatus)
{
	if ( pStatus == NULL ) {
		return;
	}

	XS_LockConfigReloadState();
	memset(pStatus, 0, sizeof(XS_ConfigReloadStatus));
	*pStatus = g_tXsConfigReloadStatus;
	XS_UnlockConfigReloadState();
}

static inline bool XS_TryLockHostScriptReload(XS_HostConfig* objHost)
{
	if ( objHost == NULL ) {
		return FALSE;
	}

	return __xrtAtomicCompareExchange32(&objHost->iScriptReloading, 1, 0) == 0;
}

static inline void XS_ForceLockHostScriptReload(XS_HostConfig* objHost)
{
	if ( objHost == NULL ) {
		return;
	}

	(void)__xrtAtomicExchange32(&objHost->iScriptReloading, 1);
}

static inline void XS_UnlockHostScriptReload(XS_HostConfig* objHost)
{
	if ( objHost == NULL ) {
		return;
	}

	(void)__xrtAtomicExchange32(&objHost->iScriptReloading, 0);
}

static inline XS_ServerConfig* XS_FindRuntimeServerByName(XS_Runtime* objRuntime, const char* sServerName)
{
	uint32 i;
	
	if ( objRuntime == NULL || objRuntime->Servers == NULL || sServerName == NULL || sServerName[0] == '\0' ) {
		return NULL;
	}
	
	for ( i = 1; i <= objRuntime->Servers->Count; i++ ) {
		XS_ServerConfig* objServer = xrtArrayGet_Inline(objRuntime->Servers, i);
		if ( objServer->Name && strcmp(objServer->Name, sServerName) == 0 ) {
			return objServer;
		}
	}
	
	return NULL;
}

static inline XS_ServerConfig* XS_FindConfigServerByName(XS_Config* objCfg, const char* sServerName)
{
	uint32 i;
	
	if ( objCfg == NULL || objCfg->Servers == NULL || sServerName == NULL || sServerName[0] == '\0' ) {
		return NULL;
	}
	
	for ( i = 1; i <= objCfg->Servers->Count; i++ ) {
		XS_ServerConfig* objServer = xrtArrayGet_Inline(objCfg->Servers, i);
		if ( objServer->Name && strcmp(objServer->Name, sServerName) == 0 ) {
			return objServer;
		}
	}
	
	return NULL;
}

static inline bool XS_RequestConfigReloadEx(const char* sServerName, const char* sHostName, bool bForce)
{
	int64 iTotalCount;
	int64 iSuccessCount;
	int64 iFailureCount;

	if ( __xrtAtomicCompareExchange32(&g_iXsConfigReloadRequested, XS_CONFIG_RELOAD_REQ_LOCKED, XS_CONFIG_RELOAD_REQ_IDLE) != XS_CONFIG_RELOAD_REQ_IDLE ) {
		return FALSE;
	}

	XS_LockConfigReloadState();
	iTotalCount = g_tXsConfigReloadStatus.iTotalCount;
	iSuccessCount = g_tXsConfigReloadStatus.iSuccessCount;
	iFailureCount = g_tXsConfigReloadStatus.iFailureCount;

	memset(&g_tXsConfigReloadRequest, 0, sizeof(g_tXsConfigReloadRequest));
	g_tXsConfigReloadRequest.Force = bForce;
	if ( sServerName && sServerName[0] ) {
		strncpy(g_tXsConfigReloadRequest.sServerName, sServerName, sizeof(g_tXsConfigReloadRequest.sServerName) - 1);
	}
	if ( sHostName && sHostName[0] ) {
		strncpy(g_tXsConfigReloadRequest.sHostName, sHostName, sizeof(g_tXsConfigReloadRequest.sHostName) - 1);
	}
	memset(&g_tXsConfigReloadStatus, 0, sizeof(g_tXsConfigReloadStatus));
	g_tXsConfigReloadStatus.Busy = TRUE;
	g_tXsConfigReloadStatus.iTotalCount = iTotalCount;
	g_tXsConfigReloadStatus.iSuccessCount = iSuccessCount;
	g_tXsConfigReloadStatus.iFailureCount = iFailureCount;
	if ( sServerName && sServerName[0] ) {
		strncpy(g_tXsConfigReloadStatus.sServerName, sServerName, sizeof(g_tXsConfigReloadStatus.sServerName) - 1);
	}
	if ( sHostName && sHostName[0] ) {
		strncpy(g_tXsConfigReloadStatus.sHostName, sHostName, sizeof(g_tXsConfigReloadStatus.sHostName) - 1);
	}
	XS_UnlockConfigReloadState();
	(void)__xrtAtomicExchange32(&g_iXsConfigReloadRequested, XS_CONFIG_RELOAD_REQ_QUEUED);
	return TRUE;
}

static inline bool XS_RequestConfigReload(bool bForce)
{
	return XS_RequestConfigReloadEx(NULL, NULL, bForce);
}

static inline bool XS_TakeConfigReloadRequest(XS_ConfigReloadRequest* pReq)
{
	if ( __xrtAtomicCompareExchange32(&g_iXsConfigReloadRequested, XS_CONFIG_RELOAD_REQ_LOCKED, XS_CONFIG_RELOAD_REQ_QUEUED) != XS_CONFIG_RELOAD_REQ_QUEUED ) {
		return FALSE;
	}
	
	if ( pReq ) {
		*pReq = g_tXsConfigReloadRequest;
	}
	memset(&g_tXsConfigReloadRequest, 0, sizeof(g_tXsConfigReloadRequest));
	(void)__xrtAtomicExchange32(&g_iXsConfigReloadRequested, XS_CONFIG_RELOAD_REQ_IDLE);
	return TRUE;
}

static inline void XS_SetConfigReloadStatusEx(bool bSuccess, const XS_ConfigReloadRequest* pReq, const char* sMessage, const XS_ConfigReloadStatus* pBusyStatus)
{
	int64 iTotalCount;
	int64 iSuccessCount;
	int64 iFailureCount;

	XS_LockConfigReloadState();
	iTotalCount = g_tXsConfigReloadStatus.iTotalCount;
	iSuccessCount = g_tXsConfigReloadStatus.iSuccessCount;
	iFailureCount = g_tXsConfigReloadStatus.iFailureCount;

	memset(&g_tXsConfigReloadStatus, 0, sizeof(g_tXsConfigReloadStatus));
	g_tXsConfigReloadStatus.Busy = FALSE;
	g_tXsConfigReloadStatus.Success = bSuccess;
	g_tXsConfigReloadStatus.HasResult = TRUE;
	g_tXsConfigReloadStatus.LastTime = xrtNow();
	g_tXsConfigReloadStatus.iTotalCount = iTotalCount + 1;
	g_tXsConfigReloadStatus.iSuccessCount = iSuccessCount + (bSuccess ? 1 : 0);
	g_tXsConfigReloadStatus.iFailureCount = iFailureCount + (bSuccess ? 0 : 1);
	if ( pBusyStatus && pBusyStatus->Busy ) {
		g_tXsConfigReloadStatus.Busy = TRUE;
		if ( pBusyStatus->sServerName[0] ) {
			strncpy(g_tXsConfigReloadStatus.sServerName, pBusyStatus->sServerName, sizeof(g_tXsConfigReloadStatus.sServerName) - 1);
		}
		if ( pBusyStatus->sHostName[0] ) {
			strncpy(g_tXsConfigReloadStatus.sHostName, pBusyStatus->sHostName, sizeof(g_tXsConfigReloadStatus.sHostName) - 1);
		}
	} else if ( pReq ) {
		if ( pReq->sServerName[0] ) {
			strncpy(g_tXsConfigReloadStatus.sServerName, pReq->sServerName, sizeof(g_tXsConfigReloadStatus.sServerName) - 1);
		}
		if ( pReq->sHostName[0] ) {
			strncpy(g_tXsConfigReloadStatus.sHostName, pReq->sHostName, sizeof(g_tXsConfigReloadStatus.sHostName) - 1);
		}
	}
	if ( sMessage && sMessage[0] ) {
		strncpy(g_tXsConfigReloadStatus.sMessage, sMessage, sizeof(g_tXsConfigReloadStatus.sMessage) - 1);
	}
	XS_UnlockConfigReloadState();
}

static inline void XS_SetConfigReloadStatus(bool bSuccess, const XS_ConfigReloadRequest* pReq, const char* sMessage)
{
	XS_SetConfigReloadStatusEx(bSuccess, pReq, sMessage, NULL);
}

static inline void XS_ClearConfigReloadStatus(void)
{
	int64 iTotalCount;
	int64 iSuccessCount;
	int64 iFailureCount;
	bool bBusy;
	char sServerName[128];
	char sHostName[128];

	XS_LockConfigReloadState();
	iTotalCount = g_tXsConfigReloadStatus.iTotalCount;
	iSuccessCount = g_tXsConfigReloadStatus.iSuccessCount;
	iFailureCount = g_tXsConfigReloadStatus.iFailureCount;
	bBusy = g_tXsConfigReloadStatus.Busy;
	memset(sServerName, 0, sizeof(sServerName));
	memset(sHostName, 0, sizeof(sHostName));
	if ( bBusy ) {
		strncpy(sServerName, g_tXsConfigReloadStatus.sServerName, sizeof(sServerName) - 1);
		strncpy(sHostName, g_tXsConfigReloadStatus.sHostName, sizeof(sHostName) - 1);
	}
	memset(&g_tXsConfigReloadStatus, 0, sizeof(g_tXsConfigReloadStatus));
	g_tXsConfigReloadStatus.Busy = bBusy;
	g_tXsConfigReloadStatus.iTotalCount = iTotalCount;
	g_tXsConfigReloadStatus.iSuccessCount = iSuccessCount;
	g_tXsConfigReloadStatus.iFailureCount = iFailureCount;
	if ( bBusy ) {
		strncpy(g_tXsConfigReloadStatus.sServerName, sServerName, sizeof(g_tXsConfigReloadStatus.sServerName) - 1);
		strncpy(g_tXsConfigReloadStatus.sHostName, sHostName, sizeof(g_tXsConfigReloadStatus.sHostName) - 1);
	}
	XS_UnlockConfigReloadState();
}

static inline void XS_ResetConfigReloadStats(void)
{
	bool bBusy;
	char sServerName[128];
	char sHostName[128];

	XS_LockConfigReloadState();
	bBusy = g_tXsConfigReloadStatus.Busy;
	memset(sServerName, 0, sizeof(sServerName));
	memset(sHostName, 0, sizeof(sHostName));
	if ( bBusy ) {
		strncpy(sServerName, g_tXsConfigReloadStatus.sServerName, sizeof(sServerName) - 1);
		strncpy(sHostName, g_tXsConfigReloadStatus.sHostName, sizeof(sHostName) - 1);
	}
	memset(&g_tXsConfigReloadStatus, 0, sizeof(g_tXsConfigReloadStatus));
	g_tXsConfigReloadStatus.Busy = bBusy;
	if ( bBusy ) {
		strncpy(g_tXsConfigReloadStatus.sServerName, sServerName, sizeof(g_tXsConfigReloadStatus.sServerName) - 1);
		strncpy(g_tXsConfigReloadStatus.sHostName, sHostName, sizeof(g_tXsConfigReloadStatus.sHostName) - 1);
	}
	XS_UnlockConfigReloadState();
}

static inline bool XS_ReloadHostScript(XS_ServerConfig* objServer, XS_HostConfig* objHost);

static inline XS_HostConfig* XS_FindServerHostByName(XS_ServerConfig* objServer, const char* sHostName)
{
	uint32 i;
	
	if ( objServer == NULL || sHostName == NULL || sHostName[0] == '\0' ) {
		return NULL;
	}
	
	if ( objServer->EnableDefaultHost && objServer->DefaultHost.Name && strcmp(objServer->DefaultHost.Name, sHostName) == 0 ) {
		return &objServer->DefaultHost;
	}
	
	for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
		XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
		if ( objHost->Name && strcmp(objHost->Name, sHostName) == 0 ) {
			return objHost;
		}
	}
	
	return NULL;
}

static inline int XS_ReloadServerHostScript(XS_ServerConfig* objServer, XS_HostConfig* objHost, bool bForce)
{
	bool bOK;

	(void)bForce;
	
	if ( objServer == NULL || objHost == NULL ) {
		return -1;
	}
	
	if ( objHost->DevMode != XS_DEV_SCRIPT_C ) {
		return -2;
	}

	if ( !XS_TryLockHostScriptReload(objHost) ) {
		return -4;
	}

	bOK = XS_ReloadHostScript(objServer, objHost);
	XS_UnlockHostScriptReload(objHost);
	return bOK ? 0 : -3;
}

static inline int XS_ReloadServerDefaultHostScript(XS_ServerConfig* objServer, bool bForce)
{
	if ( objServer == NULL || !objServer->EnableDefaultHost ) {
		return -1;
	}
	
	return XS_ReloadServerHostScript(objServer, &objServer->DefaultHost, bForce);
}

static inline int XS_ReloadServerHostScriptByName(XS_ServerConfig* objServer, const char* sHostName, bool bForce)
{
	XS_HostConfig* objHost = XS_FindServerHostByName(objServer, sHostName);
	
	if ( objHost == NULL ) {
		return -1;
	}
	
	return XS_ReloadServerHostScript(objServer, objHost, bForce);
}

static inline const char* XS_ReloadResultText(int iCode)
{
	switch ( iCode ) {
		case 0: return "ok";
		case -1: return "invalid target";
		case -2: return "host is not script-c";
		case -3: return "reload failed";
		case -4: return "reload busy";
		default: return "unknown";
	}
}

#endif
