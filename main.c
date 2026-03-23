#include <stdio.h>
#include <signal.h>



// 定义 XRT_IMPLEMENTATION 导入功能实现
#define XRT_IMPLEMENTATION
#include "lib/xrt.h"

// vNext 支撑层
#include "src/support/log.h"
#include "src/support/validate.h"
#include "src/support/path.h"

// vNext 核心骨架
#include "src/core/error.h"
#include "src/core/host.h"
#include "src/core/server.h"
#include "src/core/config.h"
#include "src/core/bus.h"

static char* g_sXsConfigFile = NULL;
static bool g_bXsConfigFileOwned = FALSE;
static xtime g_tXsStartTime = 0;
static double g_fXsStartTick = 0.0;
static uint32 g_iXsEngineWorkers = 0;
static uint32 g_iXsRuntimeServerCount = 0;
static volatile int64 g_iXsHttpReqCount = 0;
static volatile int64 g_iXsHttpManageReqCount = 0;
static volatile int64 g_iXsHttpAppReqCount = 0;
static volatile int64 g_iXsHttpResp2xxCount = 0;
static volatile int64 g_iXsHttpResp3xxCount = 0;
static volatile int64 g_iXsHttpResp4xxCount = 0;
static volatile int64 g_iXsHttpResp5xxCount = 0;
static volatile int64 g_iXsHttpConnCurrent = 0;
static volatile int64 g_iXsHttpConnPeak = 0;
static volatile int64 g_iXsHttpIdleCloseCount = 0;
static volatile int64 g_iXsHttpConnLimitCloseCount = 0;
static volatile int64 g_iXsHttpStopCleanupCount = 0;
static volatile int64 g_iXsHttpRejectCount = 0;
static volatile int64 g_iXsHttpHeaderLimitRejectCount = 0;
static volatile int64 g_iXsHttpBodyLimitRejectCount = 0;
static volatile int64 g_iXsHttpPathLimitRejectCount = 0;
static volatile int64 g_iXsHttpApiDisabledRejectCount = 0;
static volatile int64 g_iXsHttpMethodRejectCount = 0;
static volatile int64 g_iXsHttpHostNotFoundRejectCount = 0;
static volatile int64 g_iXsHttpReloadBusyRejectCount = 0;
static volatile int64 g_iXsHttpReloadFailedRejectCount = 0;
static volatile int64 g_iXsHttpCheckConfigFailedRejectCount = 0;
static volatile int64 g_iXsHttpBusBadRequestRejectCount = 0;
static volatile int64 g_iXsHttpBusNotFoundRejectCount = 0;
static volatile int64 g_iXsHttpBusLimitRejectCount = 0;
static volatile int64 g_iXsHttpBusFailedRejectCount = 0;
static volatile int64 g_iXsHttpMethodGetCount = 0;
static volatile int64 g_iXsHttpMethodPostCount = 0;
static volatile int64 g_iXsHttpMethodHeadCount = 0;
static volatile int64 g_iXsHttpMethodOtherCount = 0;
static volatile int64 g_iXsHttpLastMethodType = 0;
static volatile int64 g_iXsHttpLastAppMethodType = 0;
static volatile int64 g_iXsHttpTimeTotalMS = 0;
static volatile int64 g_iXsHttpTimeMaxMS = 0;
static volatile int64 g_iXsHttpLastTimeMS = 0;
static volatile int64 g_iXsHttpLastAppTimeMS = 0;
static volatile int64 g_iXsHttpLastStatusCode = 0;
static volatile int64 g_iXsHttpLastAppStatusCode = 0;
static volatile int64 g_iXsHttpLastRejectStatus = 0;
static xtime g_tXsHttpLastRequestTime = 0;
static xtime g_tXsHttpLastAppRequestTime = 0;
static xtime g_tXsHttpLastIdleCloseTime = 0;
static xtime g_tXsHttpLastConnLimitCloseTime = 0;
static xtime g_tXsHttpLastStopCleanupTime = 0;
static xtime g_tXsHttpLastRejectTime = 0;
static xtime g_tXsHttpLastHeaderLimitRejectTime = 0;
static xtime g_tXsHttpLastBodyLimitRejectTime = 0;
static xtime g_tXsHttpLastPathLimitRejectTime = 0;
static xtime g_tXsHttpLastApiDisabledRejectTime = 0;
static xtime g_tXsHttpLastMethodRejectTime = 0;
static xtime g_tXsHttpLastHostNotFoundRejectTime = 0;
static xtime g_tXsHttpLastReloadBusyRejectTime = 0;
static xtime g_tXsHttpLastReloadFailedRejectTime = 0;
static xtime g_tXsHttpLastCheckConfigFailedRejectTime = 0;
static xtime g_tXsHttpLastBusBadRequestRejectTime = 0;
static xtime g_tXsHttpLastBusNotFoundRejectTime = 0;
static xtime g_tXsHttpLastBusLimitRejectTime = 0;
static xtime g_tXsHttpLastBusFailedRejectTime = 0;
static volatile int64 g_iXsHttpLastStopCleanupClosed = 0;
static volatile int64 g_iXsHttpLastStopCleanupRemain = 0;
static volatile int64 g_iXsHttpLastBodyLen = 0;
static volatile int64 g_iXsHttpLastAppBodyLen = 0;
static volatile int64 g_iXsHttpLastHeaderCount = 0;
static volatile int64 g_iXsHttpLastAppHeaderCount = 0;
static volatile int64 g_iXsHttpLastQueryLen = 0;
static volatile int64 g_iXsHttpLastAppQueryLen = 0;
static char g_sXsHttpLastPath[1024] = {0};
static char g_sXsHttpLastTarget[2048] = {0};
static char g_sXsHttpLastRemote[128] = {0};
static char g_sXsHttpLastContentType[128] = {0};
static char g_sXsHttpLastHost[256] = {0};
static char g_sXsHttpLastUserAgent[256] = {0};
static char g_sXsHttpLastReferer[512] = {0};
static char g_sXsHttpLastOrigin[256] = {0};
static char g_sXsHttpLastAccept[256] = {0};
static char g_sXsHttpLastAcceptEncoding[256] = {0};
static char g_sXsHttpLastCookie[512] = {0};
static char g_sXsHttpLastForwardedFor[256] = {0};
static char g_sXsHttpLastRealIP[128] = {0};
static char g_sXsHttpLastConnection[128] = {0};
static char g_sXsHttpLastCacheControl[256] = {0};
static char g_sXsHttpLastVersion[32] = {0};
static char g_sXsHttpLastAppPath[1024] = {0};
static char g_sXsHttpLastAppTarget[2048] = {0};
static char g_sXsHttpLastAppRemote[128] = {0};
static char g_sXsHttpLastAppContentType[128] = {0};
static char g_sXsHttpLastAppHost[256] = {0};
static char g_sXsHttpLastAppUserAgent[256] = {0};
static char g_sXsHttpLastAppReferer[512] = {0};
static char g_sXsHttpLastAppOrigin[256] = {0};
static char g_sXsHttpLastAppAccept[256] = {0};
static char g_sXsHttpLastAppAcceptEncoding[256] = {0};
static char g_sXsHttpLastAppCookie[512] = {0};
static char g_sXsHttpLastAppForwardedFor[256] = {0};
static char g_sXsHttpLastAppRealIP[128] = {0};
static char g_sXsHttpLastAppConnection[128] = {0};
static char g_sXsHttpLastAppCacheControl[256] = {0};
static char g_sXsHttpLastAppVersion[32] = {0};
static char g_sXsHttpLastRejectReason[128] = {0};
static volatile int64 g_iXsWsConnCurrent = 0;
static volatile int64 g_iXsWsConnPeak = 0;
static volatile int64 g_iXsWsOpenCount = 0;
static volatile int64 g_iXsWsCloseCount = 0;
static volatile int64 g_iXsWsTextCount = 0;
static volatile int64 g_iXsWsBinaryCount = 0;
static volatile int64 g_iXsWsPingCount = 0;
static volatile int64 g_iXsWsPongCount = 0;
static volatile int64 g_iXsWsErrorCount = 0;
static volatile int64 g_iXsWsInvalidCount = 0;
static volatile int64 g_iXsWsLastErrorCode = 0;
static volatile int64 g_iXsWsLastCloseReason = 0;
static volatile int64 g_iXsWsLastFrameType = 0;
static volatile int64 g_iXsWsLastBytes = 0;
static volatile int64 g_iXsWsIdleCloseCount = 0;
static volatile int64 g_iXsWsConnLimitCloseCount = 0;
static volatile int64 g_iXsWsMessageLimitCloseCount = 0;
static volatile int64 g_iXsWsStopCleanupCount = 0;
static volatile int64 g_iXsWsRejectCount = 0;
static xtime g_tXsWsLastTime = 0;
static xtime g_tXsWsLastCloseTime = 0;
static xtime g_tXsWsLastErrorTime = 0;
static xtime g_tXsWsLastInvalidTime = 0;
static xtime g_tXsWsLastIdleCloseTime = 0;
static xtime g_tXsWsLastConnLimitCloseTime = 0;
static xtime g_tXsWsLastMessageLimitCloseTime = 0;
static xtime g_tXsWsLastStopCleanupTime = 0;
static xtime g_tXsWsLastRejectTime = 0;
static volatile int64 g_iXsWsLastStopCleanupClosed = 0;
static volatile int64 g_iXsWsLastStopCleanupRemain = 0;
static char g_sXsWsLastText[256] = {0};
static char g_sXsWsLastRemote[128] = {0};
static char g_sXsWsLastInvalidReason[128] = {0};
static char g_sXsWsLastRejectReason[128] = {0};
static volatile int64 g_iXsXtpConnCurrent = 0;
static volatile int64 g_iXsXtpConnPeak = 0;
static volatile int64 g_iXsXtpOpenCount = 0;
static volatile int64 g_iXsXtpCloseCount = 0;
static volatile int64 g_iXsXtpErrorCount = 0;
static volatile int64 g_iXsXtpLastErrorCode = 0;
static volatile int64 g_iXsXtpInvalidCount = 0;
static volatile int64 g_iXsXtpMsgCount = 0;
static volatile int64 g_iXsXtpReqCount = 0;
static volatile int64 g_iXsXtpRespCount = 0;
static volatile int64 g_iXsXtpPushCount = 0;
static volatile int64 g_iXsXtpEventCount = 0;
static volatile int64 g_iXsXtpSendCount = 0;
static volatile int64 g_iXsXtpRecvBytes = 0;
static volatile int64 g_iXsXtpSendBytes = 0;
static volatile int64 g_iXsXtpLastBytes = 0;
static volatile int64 g_iXsXtpIdleCloseCount = 0;
static volatile int64 g_iXsXtpConnLimitCloseCount = 0;
static volatile int64 g_iXsXtpRecvLimitCloseCount = 0;
static volatile int64 g_iXsXtpStopCleanupCount = 0;
static volatile int64 g_iXsXtpRejectCount = 0;
static volatile int64 g_iXsXtpLastMsgType = 0;
static volatile int64 g_iXsXtpLastStatus = 0;
static volatile int64 g_iXsXtpLastMsgID = 0;
static volatile int64 g_iXsXtpLastFlags = 0;
static volatile int64 g_iXsXtpLastParamCount = 0;
static volatile int64 g_iXsXtpLastBodySize = 0;
static xtime g_tXsXtpLastErrorTime = 0;
static xtime g_tXsXtpLastInvalidTime = 0;
static xtime g_tXsXtpLastIdleCloseTime = 0;
static xtime g_tXsXtpLastConnLimitCloseTime = 0;
static xtime g_tXsXtpLastRecvLimitCloseTime = 0;
static xtime g_tXsXtpLastStopCleanupTime = 0;
static xtime g_tXsXtpLastRejectTime = 0;
static volatile int64 g_iXsXtpLastStopCleanupClosed = 0;
static volatile int64 g_iXsXtpLastStopCleanupRemain = 0;
static xtime g_tXsXtpLastTime = 0;
static char g_sXsXtpLastCmd[256] = {0};
static char g_sXsXtpLastRemote[128] = {0};
static char g_sXsXtpLastInvalidReason[128] = {0};
static char g_sXsXtpLastRejectReason[128] = {0};
static volatile int64 g_iXsUdpRecvCount = 0;
static volatile int64 g_iXsUdpSendCount = 0;
static volatile int64 g_iXsUdpErrorCount = 0;
static volatile int64 g_iXsUdpLastErrorCode = 0;
static volatile int64 g_iXsUdpLastBytes = 0;
static volatile int64 g_iXsUdpRecvBytes = 0;
static volatile int64 g_iXsUdpSendBytes = 0;
static xtime g_tXsUdpLastTime = 0;
static xtime g_tXsUdpLastErrorTime = 0;
static char g_sXsUdpLastFrom[128] = {0};
static char g_sXsUdpLastText[256] = {0};
static volatile int64 g_iXsCustomConnCurrent = 0;
static volatile int64 g_iXsCustomConnPeak = 0;
static volatile int64 g_iXsCustomOpenCount = 0;
static volatile int64 g_iXsCustomCloseCount = 0;
static volatile int64 g_iXsCustomErrorCount = 0;
static volatile int64 g_iXsCustomInvalidCount = 0;
static volatile int64 g_iXsCustomLastCloseReason = 0;
static volatile int64 g_iXsCustomLastErrorCode = 0;
static volatile int64 g_iXsCustomRecvCount = 0;
static volatile int64 g_iXsCustomSendCount = 0;
static volatile int64 g_iXsCustomRecvBytes = 0;
static volatile int64 g_iXsCustomSendBytes = 0;
static volatile int64 g_iXsCustomLastBytes = 0;
static volatile int64 g_iXsCustomIdleCloseCount = 0;
static volatile int64 g_iXsCustomConnLimitCloseCount = 0;
static volatile int64 g_iXsCustomRecvLimitCloseCount = 0;
static volatile int64 g_iXsCustomStopCleanupCount = 0;
static volatile int64 g_iXsCustomRejectCount = 0;
static xtime g_tXsCustomLastTime = 0;
static xtime g_tXsCustomLastCloseTime = 0;
static xtime g_tXsCustomLastErrorTime = 0;
static xtime g_tXsCustomLastInvalidTime = 0;
static xtime g_tXsCustomLastIdleCloseTime = 0;
static xtime g_tXsCustomLastConnLimitCloseTime = 0;
static xtime g_tXsCustomLastRecvLimitCloseTime = 0;
static xtime g_tXsCustomLastStopCleanupTime = 0;
static xtime g_tXsCustomLastRejectTime = 0;
static volatile int64 g_iXsCustomLastStopCleanupClosed = 0;
static volatile int64 g_iXsCustomLastStopCleanupRemain = 0;
static char g_sXsCustomLastText[256] = {0};
static char g_sXsCustomLastRemote[128] = {0};
static char g_sXsCustomLastInvalidReason[128] = {0};
static char g_sXsCustomLastRejectReason[128] = {0};
static volatile int64 g_iXsCheckConfigTotalCount = 0;
static volatile int64 g_iXsCheckConfigSuccessCount = 0;
static volatile int64 g_iXsCheckConfigFailureCount = 0;
static volatile int64 g_iXsCheckConfigLastServerCount = 0;
static xtime g_tXsCheckConfigLastTime = 0;
static volatile long g_iXsCheckConfigStateLock = 0;
static bool g_bXsCheckConfigHasResult = FALSE;
static bool g_bXsCheckConfigLastResult = FALSE;
static char g_sXsCheckConfigLastFile[1024] = {0};
static char g_sXsCheckConfigLastBase[1024] = {0};
static char g_sXsCheckConfigLastMessage[128] = {0};

typedef struct {
	bool HasResult;
	bool LastResult;
	xtime LastTime;
	int64 iTotalCount;
	int64 iSuccessCount;
	int64 iFailureCount;
	int64 iLastServerCount;
	char sLastFile[1024];
	char sLastBase[1024];
	char sLastMessage[128];
} XS_CheckConfigStatusSnapshot;

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
} XS_ReloadStatusSnapshot;

static inline bool XS_RequestConfigReloadEx(const char* sServerName, const char* sHostName, bool bForce);
static inline XS_HostConfig* XS_FindServerHostByName(XS_ServerConfig* objServer, const char* sHostName);
static inline int XS_ReloadServerHostScript(XS_ServerConfig* objServer, XS_HostConfig* objHost, bool bForce);
static inline int XS_ReloadServerHostScriptByName(XS_ServerConfig* objServer, const char* sHostName, bool bForce);
static inline const char* XS_ReloadResultText(int iCode);
static inline void XS_ClearConfigReloadStatus(void);
static inline void XS_ResetConfigReloadStats(void);
static inline void XS_LockCheckConfigState(void);
static inline void XS_UnlockCheckConfigState(void);
static inline void XS_GetCheckConfigStatusSnapshot(XS_CheckConfigStatusSnapshot* pStatus);
static inline void XS_GetReloadStatusSnapshot(XS_ReloadStatusSnapshot* pStatus);
static inline void XS_RecordReloadStatusLite(const char* sServerName, const char* sHostName, bool bForce, int iCode);
static inline const char* XS_WsLastFrameTypeName(void);
static inline char* XS_WsLastTimeText(void);
static inline int64 XS_WsLastAgeMS(void);
static inline char* XS_WsLastCloseTimeText(void);
static inline int64 XS_WsLastCloseAgeMS(void);
static inline char* XS_WsLastErrorTimeText(void);
static inline int64 XS_WsLastErrorAgeMS(void);

// vNext 脚本层
#include "src/script/tcc_host.h"
#include "src/script/script_api.h"
#include "src/script/dynload.h"

// vNext 协议层
#include "src/protocol/http.h"
#include "src/protocol/ws.h"
#include "src/protocol/tcp.h"
#include "src/protocol/udp.h"
#include "src/protocol/xtp.h"
#include "src/protocol/custom.h"

// vNext 运行时
#include "src/core/runtime.h"
#include "src/core/reload.h"

static inline void XS_LockCheckConfigState(void)
{
	while ( __xrtAtomicCompareExchange32(&g_iXsCheckConfigStateLock, 1, 0) != 0 ) {
	}
}

static inline void XS_UnlockCheckConfigState(void)
{
	(void)__xrtAtomicExchange32(&g_iXsCheckConfigStateLock, 0);
}

static inline void XS_GetCheckConfigStatusSnapshot(XS_CheckConfigStatusSnapshot* pStatus)
{
	if ( pStatus == NULL ) {
		return;
	}

	XS_LockCheckConfigState();
	memset(pStatus, 0, sizeof(XS_CheckConfigStatusSnapshot));
	pStatus->HasResult = g_bXsCheckConfigHasResult;
	pStatus->LastResult = g_bXsCheckConfigLastResult;
	pStatus->LastTime = g_tXsCheckConfigLastTime;
	pStatus->iTotalCount = XS_HttpMetricGet(&g_iXsCheckConfigTotalCount);
	pStatus->iSuccessCount = XS_HttpMetricGet(&g_iXsCheckConfigSuccessCount);
	pStatus->iFailureCount = XS_HttpMetricGet(&g_iXsCheckConfigFailureCount);
	pStatus->iLastServerCount = g_iXsCheckConfigLastServerCount;
	if ( g_sXsCheckConfigLastFile[0] ) {
		strncpy(pStatus->sLastFile, g_sXsCheckConfigLastFile, sizeof(pStatus->sLastFile) - 1);
	}
	if ( g_sXsCheckConfigLastBase[0] ) {
		strncpy(pStatus->sLastBase, g_sXsCheckConfigLastBase, sizeof(pStatus->sLastBase) - 1);
	}
	if ( g_sXsCheckConfigLastMessage[0] ) {
		strncpy(pStatus->sLastMessage, g_sXsCheckConfigLastMessage, sizeof(pStatus->sLastMessage) - 1);
	}
	XS_UnlockCheckConfigState();
}

static inline void XS_GetReloadStatusSnapshot(XS_ReloadStatusSnapshot* pStatus)
{
	XS_ConfigReloadStatus tReloadStatus;

	if ( pStatus == NULL ) {
		return;
	}

	memset(pStatus, 0, sizeof(XS_ReloadStatusSnapshot));
	memset(&tReloadStatus, 0, sizeof(tReloadStatus));
	XS_GetConfigReloadStatusSnapshot(&tReloadStatus);

	pStatus->Busy = tReloadStatus.Busy;
	pStatus->Success = tReloadStatus.Success;
	pStatus->HasResult = tReloadStatus.HasResult;
	pStatus->LastTime = tReloadStatus.LastTime;
	pStatus->iTotalCount = tReloadStatus.iTotalCount;
	pStatus->iSuccessCount = tReloadStatus.iSuccessCount;
	pStatus->iFailureCount = tReloadStatus.iFailureCount;
	if ( tReloadStatus.sServerName[0] ) {
		strncpy(pStatus->sServerName, tReloadStatus.sServerName, sizeof(pStatus->sServerName) - 1);
	}
	if ( tReloadStatus.sHostName[0] ) {
		strncpy(pStatus->sHostName, tReloadStatus.sHostName, sizeof(pStatus->sHostName) - 1);
	}
	if ( tReloadStatus.sMessage[0] ) {
		strncpy(pStatus->sMessage, tReloadStatus.sMessage, sizeof(pStatus->sMessage) - 1);
	}
}

static inline void XS_RecordReloadStatusLite(const char* sServerName, const char* sHostName, bool bForce, int iCode)
{
	XS_ConfigReloadRequest tReq;
	XS_ConfigReloadStatus tBusyStatus;
	bool bPreserveBusy;

	memset(&tReq, 0, sizeof(tReq));
	memset(&tBusyStatus, 0, sizeof(tBusyStatus));
	tReq.Force = bForce;
	if ( sServerName && sServerName[0] ) {
		strncpy(tReq.sServerName, sServerName, sizeof(tReq.sServerName) - 1);
	}
	if ( sHostName && sHostName[0] ) {
		strncpy(tReq.sHostName, sHostName, sizeof(tReq.sHostName) - 1);
	}

	XS_GetConfigReloadStatusSnapshot(&tBusyStatus);
	bPreserveBusy = tBusyStatus.Busy;
	XS_SetConfigReloadStatusEx((iCode == 0), &tReq, XS_ReloadResultText(iCode), bPreserveBusy ? &tBusyStatus : NULL);
}

static inline void XS_UpdateRuntimeStats(XS_Runtime* objRuntime)
{
	if ( objRuntime && objRuntime->pEngine ) {
		g_iXsEngineWorkers = xrtNetEngineGetWorkerCount(objRuntime->pEngine);
	} else {
		g_iXsEngineWorkers = 0;
	}
	if ( objRuntime && objRuntime->Servers ) {
		g_iXsRuntimeServerCount = objRuntime->Servers->Count;
	} else {
		g_iXsRuntimeServerCount = 0;
	}
}



void OnError(str sError)
{
	printf("X Runtime Error : %s\n", sError);
}



static volatile sig_atomic_t g_iXsStopFlag = 0;

static void XS_SignalHandler(int iSignal)
{
	g_iXsStopFlag = iSignal;
}

static bool XS_ConfigReloadValidateTarget(XS_Config* objCfgNew, const XS_ConfigReloadRequest* pReq, XS_ServerConfig** ppServerNew)
{
	XS_ServerConfig* objServerNew;
	
	if ( ppServerNew ) {
		*ppServerNew = NULL;
	}
	if ( pReq == NULL || pReq->sServerName[0] == '\0' ) {
		return TRUE;
	}
	
	objServerNew = XS_FindConfigServerByName(objCfgNew, pReq->sServerName);
	if ( objServerNew == NULL ) {
		XS_LogError("config reload failed: target server not found: %s", pReq->sServerName);
		return FALSE;
	}
	if ( pReq->sHostName[0] != '\0' ) {
		if ( !objServerNew->HostAware ) {
			XS_LogError("config reload failed: target server is not host-aware: %s", pReq->sServerName);
			return FALSE;
		}
		if ( XS_FindServerHostByName(objServerNew, pReq->sHostName) == NULL ) {
			XS_LogError(
				"config reload failed: target host not found: server=%s host=%s",
				pReq->sServerName,
				pReq->sHostName
			);
			return FALSE;
		}
	}
	
	if ( ppServerNew ) {
		*ppServerNew = objServerNew;
	}
	return TRUE;
}

static bool XS_PrepareHostReload(XS_ServerConfig* objServerOld, XS_ServerConfig* objServerNew, const XS_ConfigReloadRequest* pReq, XS_HostConfig** ppHostOld, XS_HostConfig** ppHostNew)
{
	XS_HostConfig* objHostOld;
	XS_HostConfig* objHostNew;
	
	if ( ppHostOld ) {
		*ppHostOld = NULL;
	}
	if ( ppHostNew ) {
		*ppHostNew = NULL;
	}
	if ( objServerOld == NULL || objServerNew == NULL || pReq == NULL || pReq->sHostName[0] == '\0' ) {
		return FALSE;
	}
	if ( !objServerOld->HostAware || !objServerNew->HostAware ) {
		XS_LogError("config reload failed: target host reload requires host-aware server");
		return FALSE;
	}
	
	objHostOld = XS_FindServerHostByName(objServerOld, pReq->sHostName);
	objHostNew = XS_FindServerHostByName(objServerNew, pReq->sHostName);
	if ( objHostOld == NULL ) {
		XS_LogError(
			"config reload failed: current target host not found: server=%s host=%s",
			pReq->sServerName,
			pReq->sHostName
		);
		return FALSE;
	}
	if ( objHostNew == NULL ) {
		XS_LogError(
			"config reload failed: new target host not found: server=%s host=%s",
			pReq->sServerName,
			pReq->sHostName
		);
		return FALSE;
	}
	
	if ( ppHostOld ) {
		*ppHostOld = objHostOld;
	}
	if ( ppHostNew ) {
		*ppHostNew = objHostNew;
	}
	return TRUE;
}

static bool XS_PerformTargetHostReload(XS_ServerConfig* objServerOld, XS_ServerConfig* objServerNew, const XS_ConfigReloadRequest* pReq)
{
	XS_HostConfig* objHostOld;
	XS_HostConfig* objHostNewSrc;
	XS_HostConfig objHostBackup;
	bool bBackupValid;
	bool bReloadLocked;
	
	if ( objServerOld == NULL || objServerNew == NULL || pReq == NULL ) {
		XS_SetConfigReloadStatus(FALSE, pReq, "invalid target");
		return FALSE;
	}
	if ( !XS_PrepareHostReload(objServerOld, objServerNew, pReq, &objHostOld, &objHostNewSrc) ) {
		XS_SetConfigReloadStatus(FALSE, pReq, "target host validation failed");
		return FALSE;
	}
	
	memset(&objHostBackup, 0, sizeof(objHostBackup));
	memcpy(&objHostBackup, objHostOld, sizeof(XS_HostConfig));
	bBackupValid = TRUE;
	bReloadLocked = FALSE;

	if ( !XS_TryLockHostScriptReload(objHostOld) ) {
		XS_LogError(
			"config reload failed: target host reload busy: server=%s host=%s",
			pReq->sServerName,
			pReq->sHostName
		);
		XS_SetConfigReloadStatus(FALSE, pReq, "target host reload busy");
		return FALSE;
	}
	bReloadLocked = TRUE;
	
	if ( objHostOld->procServiceStop ) {
		objHostOld->procServiceStop(objServerOld, objHostOld);
	}
	XS_UnloadHostScript(objServerOld, objHostOld);
	
	memcpy(objHostOld, objHostNewSrc, sizeof(XS_HostConfig));
	memset(objHostNewSrc, 0, sizeof(XS_HostConfig));
	XS_ForceLockHostScriptReload(objHostOld);
	
	if ( !XS_LoadHostScript(objServerOld, objHostOld) ) {
		XS_LogError("config reload failed: target host script load error");
		XS_SetConfigReloadStatus(FALSE, pReq, "target host script load error");
		XS_FreeHostConfig(objHostOld);
		memset(objHostOld, 0, sizeof(XS_HostConfig));
		memcpy(objHostOld, &objHostBackup, sizeof(XS_HostConfig));
		XS_ForceLockHostScriptReload(objHostOld);
		memset(&objHostBackup, 0, sizeof(XS_HostConfig));
		bBackupValid = FALSE;
		if ( XS_LoadHostScript(objServerOld, objHostOld) ) {
			if ( objHostOld->procServiceInit ) {
				objHostOld->procServiceInit(objServerOld, objHostOld);
			}
			if ( objHostOld->procServiceStart ) {
				objHostOld->procServiceStart(objServerOld, objHostOld);
			}
			XS_LogWarn(
				"config reload rollback: old target host restored: server=%s host=%s",
				pReq->sServerName,
				pReq->sHostName
			);
		} else {
			XS_LogError(
				"config reload rollback failed: old target host restore error: server=%s host=%s",
				pReq->sServerName,
				pReq->sHostName
			);
		}
		if ( bReloadLocked ) {
			XS_UnlockHostScriptReload(objHostOld);
		}
		return FALSE;
	}
	
	if ( objHostOld->procServiceInit ) {
		objHostOld->procServiceInit(objServerOld, objHostOld);
	}
	if ( objHostOld->procServiceStart ) {
		objHostOld->procServiceStart(objServerOld, objHostOld);
	}
	
	if ( bBackupValid ) {
		XS_FreeHostConfig(&objHostBackup);
	}
	
	XS_LogInfo(
		"config reload success: target server=%s target_host=%s mode=host",
		pReq->sServerName,
		pReq->sHostName
	);
	XS_SetConfigReloadStatus(TRUE, pReq, "target host reload success");
	if ( bReloadLocked ) {
		XS_UnlockHostScriptReload(objHostOld);
	}
	return TRUE;
}

static bool XS_PerformTargetServerReload(XS_Config* objCfg, XS_Runtime* objRuntime, XS_Config* objCfgNew, const XS_ConfigReloadRequest* pReq)
{
	XS_ServerConfig* objServerOld;
	XS_ServerConfig* objServerNewSrc;
	XS_ServerConfig objServerBackup;
	bool bBackupValid;
	
	if ( objCfg == NULL || objRuntime == NULL || objCfgNew == NULL || pReq == NULL ) {
		XS_SetConfigReloadStatus(FALSE, pReq, "invalid target");
		return FALSE;
	}
	
	objServerOld = XS_FindRuntimeServerByName(objRuntime, pReq->sServerName);
	if ( objServerOld == NULL ) {
		XS_LogError("config reload failed: current target server not found: %s", pReq->sServerName);
		XS_SetConfigReloadStatus(FALSE, pReq, "current target server not found");
		return FALSE;
	}
	if ( !XS_ConfigReloadValidateTarget(objCfgNew, pReq, &objServerNewSrc) ) {
		XS_SetConfigReloadStatus(FALSE, pReq, "target validation failed");
		return FALSE;
	}
	if ( objServerNewSrc == NULL ) {
		XS_LogError("config reload failed: target server not found in new config: %s", pReq->sServerName);
		XS_SetConfigReloadStatus(FALSE, pReq, "target server not found in new config");
		return FALSE;
	}
	if ( objServerOld->Class != objServerNewSrc->Class ) {
		XS_LogError(
			"config reload failed: target server class changed: old=%s new=%s",
			XS_ServerClassName(objServerOld->Class),
			XS_ServerClassName(objServerNewSrc->Class)
		);
		XS_SetConfigReloadStatus(FALSE, pReq, "target server class changed");
		return FALSE;
	}
	if ( objServerOld->HostAware != objServerNewSrc->HostAware ) {
		XS_LogError("config reload failed: target server host-aware flag changed");
		XS_SetConfigReloadStatus(FALSE, pReq, "target server host-aware flag changed");
		return FALSE;
	}
	if ( pReq->sHostName[0] != '\0' ) {
		return XS_PerformTargetHostReload(objServerOld, objServerNewSrc, pReq);
	}
	
	memset(&objServerBackup, 0, sizeof(objServerBackup));
	memcpy(&objServerBackup, objServerOld, sizeof(XS_ServerConfig));
	bBackupValid = TRUE;
	XS_RuntimeStopOneServer(objServerOld);
	
	memcpy(objServerOld, objServerNewSrc, sizeof(XS_ServerConfig));
	memset(objServerNewSrc, 0, sizeof(XS_ServerConfig));
	
	if ( !XS_RuntimeInitOneServer(objRuntime, objServerOld) ) {
		XS_LogError("config reload failed: target server init error");
		XS_SetConfigReloadStatus(FALSE, pReq, "target server init error");
		XS_RuntimeStopOneServer(objServerOld);
		XS_FreeServerConfig(objServerOld);
		memset(objServerOld, 0, sizeof(XS_ServerConfig));
		memcpy(objServerOld, &objServerBackup, sizeof(XS_ServerConfig));
		memset(&objServerBackup, 0, sizeof(XS_ServerConfig));
		bBackupValid = FALSE;
		
		if ( XS_RuntimeInitOneServer(objRuntime, objServerOld) && XS_RuntimeStartOneServer(objServerOld) ) {
			XS_LogWarn(
				"config reload rollback: old target server restarted: %s",
				objServerOld->Name ? objServerOld->Name : "(null)"
			);
		} else {
			XS_LogError(
				"config reload rollback failed: old target server restart error: %s",
				objServerOld->Name ? objServerOld->Name : "(null)"
			);
		}
		return FALSE;
	}
	if ( !XS_RuntimeStartOneServer(objServerOld) ) {
		XS_LogError("config reload failed: target server start error");
		XS_SetConfigReloadStatus(FALSE, pReq, "target server start error");
		XS_RuntimeStopOneServer(objServerOld);
		XS_FreeServerConfig(objServerOld);
		memset(objServerOld, 0, sizeof(XS_ServerConfig));
		memcpy(objServerOld, &objServerBackup, sizeof(XS_ServerConfig));
		memset(&objServerBackup, 0, sizeof(XS_ServerConfig));
		bBackupValid = FALSE;
		
		if ( XS_RuntimeInitOneServer(objRuntime, objServerOld) && XS_RuntimeStartOneServer(objServerOld) ) {
			XS_LogWarn(
				"config reload rollback: old target server restarted: %s",
				objServerOld->Name ? objServerOld->Name : "(null)"
			);
		} else {
			XS_LogError(
				"config reload rollback failed: old target server restart error: %s",
				objServerOld->Name ? objServerOld->Name : "(null)"
			);
		}
		return FALSE;
	}
	
	if ( bBackupValid ) {
		XS_FreeServerConfig(&objServerBackup);
	}
	
	if ( objCfg && objCfg->Servers ) {
		XS_ServerConfig* objCfgServerOld = XS_FindConfigServerByName(objCfg, pReq->sServerName);
		if ( objCfgServerOld ) {
			XS_FreeServerConfig(objCfgServerOld);
		}
	}
	
	XS_LogInfo(
		"config reload success: target server=%s target_host=%s",
		pReq->sServerName[0] ? pReq->sServerName : "(null)",
		pReq->sHostName[0] ? pReq->sHostName : "(all)"
	);
	XS_SetConfigReloadStatus(TRUE, pReq, "target server reload success");
	return TRUE;
}

static bool XS_PerformConfigReload(XS_Config* objCfg, XS_Runtime* objRuntime, const char* sCfgFile, const XS_ConfigReloadRequest* pReq)
{
	XS_Config objCfgNew;
	XS_Runtime objRuntimeNew;
	bool bOldStopped = FALSE;
	
	memset(&objCfgNew, 0, sizeof(objCfgNew));
	memset(&objRuntimeNew, 0, sizeof(objRuntimeNew));
	XS_ResetErrors();
	
	if ( !XS_LoadConfig(&objCfgNew, sCfgFile) ) {
		XS_LogError("config reload failed: load error");
		XS_SetConfigReloadStatus(FALSE, pReq, "config load error");
		XS_FreeConfig(&objCfgNew);
		return FALSE;
	}
	
	XS_LogInfo("config reload: parsed new config");
	if ( pReq && pReq->sServerName[0] != '\0' ) {
		bool bRet = XS_PerformTargetServerReload(objCfg, objRuntime, &objCfgNew, pReq);
		XS_FreeConfig(&objCfgNew);
		return bRet;
	}
	
	if ( !XS_RuntimeBuild(&objRuntimeNew, &objCfgNew) ) {
		XS_LogError("config reload failed: runtime build error");
		XS_SetConfigReloadStatus(FALSE, pReq, "runtime build error");
		XS_FreeRuntime(&objRuntimeNew);
		XS_FreeConfig(&objCfgNew);
		return FALSE;
	}
	if ( !XS_RuntimeInitServers(&objRuntimeNew) ) {
		XS_LogError("config reload failed: runtime init error");
		XS_SetConfigReloadStatus(FALSE, pReq, "runtime init error");
		XS_FreeRuntime(&objRuntimeNew);
		XS_FreeConfig(&objCfgNew);
		return FALSE;
	}
	
	XS_RuntimeStopServers(objRuntime);
	bOldStopped = TRUE;
	
	if ( !XS_RuntimeStartServers(&objRuntimeNew) ) {
		XS_LogError("config reload failed: runtime start error");
		XS_SetConfigReloadStatus(FALSE, pReq, "runtime start error");
		XS_FreeRuntime(&objRuntimeNew);
		XS_FreeConfig(&objCfgNew);
		
		if ( bOldStopped ) {
			if ( XS_RuntimeStartServers(objRuntime) ) {
				XS_LogWarn("config reload rollback: old runtime restarted");
			} else {
				XS_LogError("config reload rollback failed: old runtime restart error");
			}
		}
		return FALSE;
	}
	
	XS_FreeRuntime(objRuntime);
	XS_FreeConfig(objCfg);
	*objCfg = objCfgNew;
	*objRuntime = objRuntimeNew;
	XS_UpdateRuntimeStats(objRuntime);
	XS_LogInfo("config reload success");
	XS_SetConfigReloadStatus(TRUE, pReq, "config reload success");
	return TRUE;
}



int main(int argc, char** argv)
{
	XS_Config objCfg;
	XS_Runtime objRuntime;
	XS_ConfigReloadRequest tReloadReq;
	char* sOptFile = NULL;
	bool bUseDefaultConfig = FALSE;
	bool bCheckOnly = FALSE;
	int iExitCode = 0;
	
	memset(&objCfg, 0, sizeof(objCfg));
	memset(&objRuntime, 0, sizeof(objRuntime));
	memset(&tReloadReq, 0, sizeof(tReloadReq));
	
	#if defined(_WIN32) || defined(_WIN64)
		SetConsoleOutputCP(65001);
	#endif
	
	xrtInit();
	xCore.OnError = OnError;
	g_tXsStartTime = xrtNow();
	g_fXsStartTick = xrtTimer();
	
	XS_ResetErrors();
	
	if ( argc > 1 && argv[1] && strcmp(argv[1], "--check") == 0 ) {
		bCheckOnly = TRUE;
		if ( argc > 2 && argv[2] ) {
			sOptFile = argv[2];
		}
	}
	else if ( argc > 1 && argv[1] ) {
		sOptFile = argv[1];
	}
	
	if ( sOptFile == NULL ) {
		sOptFile = xrtPathJoin(2, xCore.AppPath, "xs.json");
		bUseDefaultConfig = TRUE;
	}
	
	if ( bCheckOnly ) {
		XS_LogInfo("mode : config check");
	} else {
		XS_LogInfo("mode : normal");
	}
	
	XS_LogInfo("XServer vNext bootstrap");
	if ( g_sXsConfigFile && g_bXsConfigFileOwned ) {
		xrtFree(g_sXsConfigFile);
	}
	g_sXsConfigFile = XS_NormalizePath(xCore.AppPath, sOptFile);
	g_bXsConfigFileOwned = (g_sXsConfigFile != NULL);
	if ( !g_bXsConfigFileOwned ) {
		g_sXsConfigFile = sOptFile;
	}
	XS_LogInfo("loading config : %s", g_sXsConfigFile ? g_sXsConfigFile : sOptFile);
	
	if ( !XS_BusInit() ) {
		XS_LogError("bus init failed");
		iExitCode = 5;
		goto ExitMain;
	}
	
	if ( !XS_LoadConfig(&objCfg, g_sXsConfigFile ? g_sXsConfigFile : sOptFile) ) {
		XS_LogError("config load failed");
		iExitCode = 1;
		goto ExitMain;
	}
	
	XS_PrintConfigSummary(&objCfg);
	
	if ( bCheckOnly ) {
		XS_LogInfo("config check passed");
		goto ExitConfig;
	}
	
	if ( !XS_RuntimeBuild(&objRuntime, &objCfg) ) {
		XS_LogError("runtime build failed");
		iExitCode = 2;
		goto ExitConfig;
	}
	XS_UpdateRuntimeStats(&objRuntime);
	
	XS_RuntimePrint(&objRuntime);
	
	if ( !XS_RuntimeInitServers(&objRuntime) ) {
		XS_LogError("runtime init failed");
		iExitCode = 3;
		goto ExitRuntime;
	}
	
	if ( !XS_RuntimeStartServers(&objRuntime) ) {
		XS_LogError("runtime start failed");
		iExitCode = 4;
		goto ExitRuntime;
	}
	
	signal(SIGINT, XS_SignalHandler);
	signal(SIGTERM, XS_SignalHandler);
	
	XS_LogInfo("stage complete : runtime entered serving loop");
	XS_LogInfo("press Ctrl+C to stop");
	
	while ( g_iXsStopFlag == 0 ) {
		if ( XS_TakeConfigReloadRequest(&tReloadReq) ) {
			XS_LogInfo(
				"config reload requested: server=%s host=%s force=%s",
				tReloadReq.sServerName[0] ? tReloadReq.sServerName : "(all)",
				tReloadReq.sHostName[0] ? tReloadReq.sHostName : "(all)",
				tReloadReq.Force ? "true" : "false"
			);
			(void)XS_PerformConfigReload(&objCfg, &objRuntime, g_sXsConfigFile ? g_sXsConfigFile : sOptFile, &tReloadReq);
		}
		XS_BusDispatchMessages(objRuntime.Servers);
		XS_BusSweepExpiredData();
		xrtSleep(200);
	}
	
	XS_LogInfo("stop signal received : %d", (int)g_iXsStopFlag);
	XS_BusDispatchMessages(objRuntime.Servers);
	XS_RuntimeStopServers(&objRuntime);
ExitRuntime:
	XS_FreeRuntime(&objRuntime);
ExitConfig:
	XS_FreeConfig(&objCfg);
ExitMain:
	XS_BusUnit();
	g_iXsEngineWorkers = 0;
	if ( g_sXsConfigFile && g_bXsConfigFileOwned ) {
		xrtFree(g_sXsConfigFile);
	}
	g_sXsConfigFile = NULL;
	g_bXsConfigFileOwned = FALSE;
	if ( bUseDefaultConfig && sOptFile ) {
		xrtFree(sOptFile);
	}
	
	xrtUnit();
	return iExitCode;
}
