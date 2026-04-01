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
#include "src/manage/runtime_state.h"
#include "src/manage/http_page.h"

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
#include "src/manage/runtime_state_api.h"

void OnError(str sError)
{
	printf("X Runtime Error : %s\n", sError);
}



static volatile sig_atomic_t g_iXsStopFlag = 0;

static void XS_SignalHandler(int iSignal)
{
	g_iXsStopFlag = iSignal;
}

static bool XS_TextEquals(const char* sLeft, const char* sRight)
{
	if ( sLeft == sRight ) {
		return TRUE;
	}
	if ( sLeft == NULL || sRight == NULL ) {
		return FALSE;
	}

	return strcmp(sLeft, sRight) == 0;
}

static bool XS_TlsConfigEquals(const xtlsconfig* pLeft, const xtlsconfig* pRight)
{
	if ( pLeft == pRight ) {
		return TRUE;
	}
	if ( pLeft == NULL || pRight == NULL ) {
		return FALSE;
	}
	if ( !XS_TextEquals(pLeft->sCertFile, pRight->sCertFile) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(pLeft->sKeyFile, pRight->sKeyFile) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(pLeft->sCaFile, pRight->sCaFile) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(pLeft->sHostName, pRight->sHostName) ) {
		return FALSE;
	}
	if ( pLeft->bVerifyPeer != pRight->bVerifyPeer ) {
		return FALSE;
	}
	if ( pLeft->bAllowTLS12Ed25519 != pRight->bAllowTLS12Ed25519 ) {
		return FALSE;
	}
	if ( pLeft->iMaxVersion != pRight->iMaxVersion ) {
		return FALSE;
	}

	return TRUE;
}

static bool XS_HttpPageConfigEquals(const XS_HttpPageConfig* objLeft, const XS_HttpPageConfig* objRight)
{
	if ( objLeft == objRight ) {
		return TRUE;
	}
	if ( objLeft == NULL || objRight == NULL ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->DefaultPage, objRight->DefaultPage) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Page404, objRight->Page404) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Page403, objRight->Page403) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Page500, objRight->Page500) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->ErrorPage, objRight->ErrorPage) ) {
		return FALSE;
	}

	return TRUE;
}

static bool XS_HostConfigReloadEquals(const XS_HostConfig* objLeft, const XS_HostConfig* objRight)
{
	if ( objLeft == objRight ) {
		return TRUE;
	}
	if ( objLeft == NULL || objRight == NULL ) {
		return FALSE;
	}
	if ( objLeft->Enabled != objRight->Enabled ) {
		return FALSE;
	}
	if ( objLeft->Debug != objRight->Debug ) {
		return FALSE;
	}
	if ( objLeft->DevMode != objRight->DevMode ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Name, objRight->Name) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Desc, objRight->Desc) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Host, objRight->Host) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Param, objRight->Param) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Path, objRight->Path) ) {
		return FALSE;
	}
	if ( !XS_HttpPageConfigEquals(&objLeft->Pages, &objRight->Pages) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->DevFile, objRight->DevFile) ) {
		return FALSE;
	}
	if ( !XS_TlsConfigEquals(&objLeft->TlsConfig, &objRight->TlsConfig) ) {
		return FALSE;
	}

	return TRUE;
}

static bool XS_HostArrayReloadEquals(xarray arrLeft, xarray arrRight)
{
	uint32 i;
	uint32 iCountLeft = arrLeft ? arrLeft->Count : 0;
	uint32 iCountRight = arrRight ? arrRight->Count : 0;

	if ( iCountLeft != iCountRight ) {
		return FALSE;
	}

	for ( i = 1; i <= iCountLeft; i++ ) {
		XS_HostConfig* objHostLeft = xrtArrayGet_Inline(arrLeft, i);
		XS_HostConfig* objHostRight = xrtArrayGet_Inline(arrRight, i);

		if ( !XS_HostConfigReloadEquals(objHostLeft, objHostRight) ) {
			return FALSE;
		}
	}

	return TRUE;
}

static bool XS_ServerConfigReloadEquals(const XS_ServerConfig* objLeft, const XS_ServerConfig* objRight)
{
	if ( objLeft == objRight ) {
		return TRUE;
	}
	if ( objLeft == NULL || objRight == NULL ) {
		return FALSE;
	}
	if ( objLeft->Enabled != objRight->Enabled ) {
		return FALSE;
	}
	if ( objLeft->Class != objRight->Class ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Name, objRight->Name) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Desc, objRight->Desc) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Param, objRight->Param) ) {
		return FALSE;
	}
	if ( objLeft->Backlog != objRight->Backlog ) {
		return FALSE;
	}
	if ( objLeft->ConnLimit != objRight->ConnLimit ) {
		return FALSE;
	}
	if ( objLeft->RecvLimit != objRight->RecvLimit ) {
		return FALSE;
	}
	if ( objLeft->IdleTimeout != objRight->IdleTimeout ) {
		return FALSE;
	}
	if ( objLeft->WsMessageLimit != objRight->WsMessageLimit ) {
		return FALSE;
	}
	if ( objLeft->PathLimit != objRight->PathLimit ) {
		return FALSE;
	}
	if ( objLeft->HeaderLimit != objRight->HeaderLimit ) {
		return FALSE;
	}
	if ( objLeft->BodyLimit != objRight->BodyLimit ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->BindIP, objRight->BindIP) ) {
		return FALSE;
	}
	if ( objLeft->BindPort != objRight->BindPort ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Addr, objRight->Addr) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->WsProtocol, objRight->WsProtocol) ) {
		return FALSE;
	}
	if ( objLeft->EnableTLS != objRight->EnableTLS ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->BindIPTLS, objRight->BindIPTLS) ) {
		return FALSE;
	}
	if ( objLeft->BindPortTLS != objRight->BindPortTLS ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->AddrTLS, objRight->AddrTLS) ) {
		return FALSE;
	}
	if ( !XS_TlsConfigEquals(&objLeft->TlsConfig, &objRight->TlsConfig) ) {
		return FALSE;
	}
	if ( objLeft->Debug != objRight->Debug ) {
		return FALSE;
	}
	if ( !XS_HttpPageConfigEquals(&objLeft->Pages, &objRight->Pages) ) {
		return FALSE;
	}
	if ( objLeft->HostAware != objRight->HostAware ) {
		return FALSE;
	}
	if ( objLeft->EnableDefaultHost != objRight->EnableDefaultHost ) {
		return FALSE;
	}
	if ( objLeft->EnableDefaultHost && !XS_HostConfigReloadEquals(&objLeft->DefaultHost, &objRight->DefaultHost) ) {
		return FALSE;
	}
	if ( !XS_HostArrayReloadEquals(objLeft->Hosts, objRight->Hosts) ) {
		return FALSE;
	}
	if ( objLeft->DevMode != objRight->DevMode ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->Path, objRight->Path) ) {
		return FALSE;
	}
	if ( !XS_TextEquals(objLeft->DevFile, objRight->DevFile) ) {
		return FALSE;
	}

	return TRUE;
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
	if ( !pReq->Force && XS_HostConfigReloadEquals(objHostOld, objHostNewSrc) ) {
		XS_LogInfo(
			"config reload skipped: target host unchanged: server=%s host=%s",
			pReq->sServerName,
			pReq->sHostName
		);
		XS_SetConfigReloadStatus(TRUE, pReq, "target host unchanged");
		return TRUE;
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
	if ( !pReq->Force && XS_ServerConfigReloadEquals(objServerOld, objServerNewSrc) ) {
		XS_LogInfo(
			"config reload skipped: target server unchanged: %s",
			pReq->sServerName
		);
		XS_SetConfigReloadStatus(TRUE, pReq, "target server unchanged");
		return TRUE;
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
	#if defined(_WIN32) || defined(_WIN64)
		signal(SIGBREAK, XS_SignalHandler);
	#endif
	
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
