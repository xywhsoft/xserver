#ifndef XS_CORE_HOST_H
#define XS_CORE_HOST_H

typedef void (*XS_ScriptServiceProc)(ptr objServer, ptr objHost);
typedef bool (*XS_ScriptHttpRequestProc)(ptr objServer, ptr objHost, const void* pReq, void* pResp);
typedef bool (*XS_ScriptMessageProc)(ptr objServer, ptr objHost, const char* sTopic, int64 iDataID, xvalue objArgs);
typedef void (*XS_ScriptWsOpenProc)(ptr objServer, ptr objHost, void* pConn);
typedef bool (*XS_ScriptWsTextProc)(ptr objServer, ptr objHost, void* pConn, const char* pData, size_t iLen);
typedef bool (*XS_ScriptWsBinaryProc)(ptr objServer, ptr objHost, void* pConn, const void* pData, size_t iLen);
typedef void (*XS_ScriptWsPingProc)(ptr objServer, ptr objHost, void* pConn, const void* pData, size_t iLen);
typedef void (*XS_ScriptWsPongProc)(ptr objServer, ptr objHost, void* pConn, const void* pData, size_t iLen);
typedef void (*XS_ScriptWsCloseProc)(ptr objServer, ptr objHost, void* pConn, int iReason);
typedef void (*XS_ScriptSetGlobalDataProc)(int idx, void* ptr);

typedef enum {
	XS_DEV_STATIC = 0,
	XS_DEV_SCRIPT_C = 1,
	XS_DEV_PROTOCOL = 2
} XS_DevMode;

typedef struct XS_HostConfig {
	bool Enabled;
	char* Name;
	char* Desc;
	char* Host;
	char* Param;
	bool Debug;
	char* Path;
	XS_DevMode DevMode;
	char* DevFile;
	xtlsconfig TlsConfig;
	ptr pScriptState;
	volatile long iScriptReloading;
	XS_ScriptServiceProc procServiceInit;
	XS_ScriptServiceProc procServiceStart;
	XS_ScriptServiceProc procServiceStop;
	XS_ScriptServiceProc procServiceUnit;
	XS_ScriptHttpRequestProc procHttpRequest;
	XS_ScriptMessageProc procMessage;
	XS_ScriptWsOpenProc procWsOpen;
	XS_ScriptWsTextProc procWsText;
	XS_ScriptWsBinaryProc procWsBinary;
	XS_ScriptWsPingProc procWsPing;
	XS_ScriptWsPongProc procWsPong;
	XS_ScriptWsCloseProc procWsClose;
	XS_ScriptSetGlobalDataProc procSetGlobalData;
} XS_HostConfig;

static inline void XS_InitHostConfig(XS_HostConfig* objHost)
{
	memset(objHost, 0, sizeof(XS_HostConfig));
	objHost->Enabled = TRUE;
	objHost->DevMode = XS_DEV_STATIC;
}

static inline void XS_FreeHostConfig(XS_HostConfig* objHost)
{
	if ( objHost == NULL ) {
		return;
	}
	
	if ( objHost->Name ) xrtFree(objHost->Name);
	if ( objHost->Desc ) xrtFree(objHost->Desc);
	if ( objHost->Host ) xrtFree(objHost->Host);
	if ( objHost->Param ) xrtFree(objHost->Param);
	if ( objHost->Path ) xrtFree(objHost->Path);
	if ( objHost->DevFile ) xrtFree(objHost->DevFile);
	if ( objHost->TlsConfig.sCaFile ) xrtFree((void*)objHost->TlsConfig.sCaFile);
	if ( objHost->TlsConfig.sCertFile ) xrtFree((void*)objHost->TlsConfig.sCertFile);
	if ( objHost->TlsConfig.sKeyFile ) xrtFree((void*)objHost->TlsConfig.sKeyFile);
	
	memset(objHost, 0, sizeof(XS_HostConfig));
}

#endif
