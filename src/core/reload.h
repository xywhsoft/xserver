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
static XS_ConfigReloadRequest g_tXsConfigReloadRequest;
static XS_ConfigReloadStatus g_tXsConfigReloadStatus;

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
	if ( g_iXsConfigReloadRequested ) {
		return FALSE;
	}
	
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
	if ( sServerName && sServerName[0] ) {
		strncpy(g_tXsConfigReloadStatus.sServerName, sServerName, sizeof(g_tXsConfigReloadStatus.sServerName) - 1);
	}
	if ( sHostName && sHostName[0] ) {
		strncpy(g_tXsConfigReloadStatus.sHostName, sHostName, sizeof(g_tXsConfigReloadStatus.sHostName) - 1);
	}
	g_iXsConfigReloadRequested = 0;
	g_iXsConfigReloadRequested = 1;
	return TRUE;
}

static inline bool XS_RequestConfigReload(bool bForce)
{
	return XS_RequestConfigReloadEx(NULL, NULL, bForce);
}

static inline bool XS_TakeConfigReloadRequest(XS_ConfigReloadRequest* pReq)
{
	if ( !g_iXsConfigReloadRequested ) {
		return FALSE;
	}
	
	if ( pReq ) {
		*pReq = g_tXsConfigReloadRequest;
	}
	memset(&g_tXsConfigReloadRequest, 0, sizeof(g_tXsConfigReloadRequest));
	g_iXsConfigReloadRequested = 0;
	return TRUE;
}

static inline void XS_SetConfigReloadStatus(bool bSuccess, const XS_ConfigReloadRequest* pReq, const char* sMessage)
{
	int64 iTotalCount;
	int64 iSuccessCount;
	int64 iFailureCount;

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
	if ( pReq ) {
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
}

static inline const XS_ConfigReloadStatus* XS_GetConfigReloadStatus(void)
{
	return &g_tXsConfigReloadStatus;
}

static inline bool XS_ConfigReloadStatusBusy(void)
{
	return g_tXsConfigReloadStatus.Busy;
}

static inline bool XS_ConfigReloadStatusHasResult(void)
{
	return g_tXsConfigReloadStatus.HasResult;
}

static inline bool XS_ConfigReloadStatusSuccess(void)
{
	return g_tXsConfigReloadStatus.Success;
}

static inline const char* XS_ConfigReloadStatusServer(void)
{
	return g_tXsConfigReloadStatus.sServerName[0] ? g_tXsConfigReloadStatus.sServerName : "(all)";
}

static inline const char* XS_ConfigReloadStatusHost(void)
{
	return g_tXsConfigReloadStatus.sHostName[0] ? g_tXsConfigReloadStatus.sHostName : "(all)";
}

static inline const char* XS_ConfigReloadStatusMessage(void)
{
	return g_tXsConfigReloadStatus.sMessage[0] ? g_tXsConfigReloadStatus.sMessage : "(none)";
}

static inline xtime XS_ConfigReloadStatusTime(void)
{
	return g_tXsConfigReloadStatus.LastTime;
}

static inline int64 XS_ConfigReloadStatusTotalCount(void)
{
	return g_tXsConfigReloadStatus.iTotalCount;
}

static inline int64 XS_ConfigReloadStatusSuccessCount(void)
{
	return g_tXsConfigReloadStatus.iSuccessCount;
}

static inline int64 XS_ConfigReloadStatusFailureCount(void)
{
	return g_tXsConfigReloadStatus.iFailureCount;
}

static inline void XS_ClearConfigReloadStatus(void)
{
	int64 iTotalCount;
	int64 iSuccessCount;
	int64 iFailureCount;

	iTotalCount = g_tXsConfigReloadStatus.iTotalCount;
	iSuccessCount = g_tXsConfigReloadStatus.iSuccessCount;
	iFailureCount = g_tXsConfigReloadStatus.iFailureCount;
	memset(&g_tXsConfigReloadStatus, 0, sizeof(g_tXsConfigReloadStatus));
	g_tXsConfigReloadStatus.iTotalCount = iTotalCount;
	g_tXsConfigReloadStatus.iSuccessCount = iSuccessCount;
	g_tXsConfigReloadStatus.iFailureCount = iFailureCount;
}

static inline void XS_ResetConfigReloadStats(void)
{
	memset(&g_tXsConfigReloadStatus, 0, sizeof(g_tXsConfigReloadStatus));
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
	(void)bForce;
	
	if ( objServer == NULL || objHost == NULL ) {
		return -1;
	}
	
	if ( objHost->DevMode != XS_DEV_SCRIPT_C ) {
		return -2;
	}
	
	return XS_ReloadHostScript(objServer, objHost) ? 0 : -3;
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
		default: return "unknown";
	}
}

#endif
