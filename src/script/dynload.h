#ifndef XS_SCRIPT_DYNLOAD_H
#define XS_SCRIPT_DYNLOAD_H

static inline void XS_ResetServerScriptState(XS_ServerConfig* objServer)
{
	if ( objServer == NULL ) {
		return;
	}
	
	objServer->pScriptState = NULL;
	objServer->procServiceInit = NULL;
	objServer->procServiceStart = NULL;
	objServer->procServiceStop = NULL;
	objServer->procServiceUnit = NULL;
	objServer->procMessage = NULL;
	objServer->procStreamOpen = NULL;
	objServer->procStreamData = NULL;
	objServer->procStreamClose = NULL;
	objServer->procDgramRecv = NULL;
	objServer->procXtpMessage = NULL;
	objServer->procSetGlobalData = NULL;
}

static inline void XS_ResetHostScriptState(XS_HostConfig* objHost)
{
	if ( objHost == NULL ) {
		return;
	}
	
	objHost->pScriptState = NULL;
	objHost->procServiceInit = NULL;
	objHost->procServiceStart = NULL;
	objHost->procServiceStop = NULL;
	objHost->procServiceUnit = NULL;
	objHost->procHttpRequest = NULL;
	objHost->procMessage = NULL;
	objHost->procWsOpen = NULL;
	objHost->procWsText = NULL;
	objHost->procWsBinary = NULL;
	objHost->procWsPing = NULL;
	objHost->procWsPong = NULL;
	objHost->procWsClose = NULL;
	objHost->procSetGlobalData = NULL;
}

static inline bool XS_BuildServerScriptState(XS_ServerConfig* objServer, TCCState** ppState)
{
	TCCState* s;
	char* sWorkPath;
	
	if ( ppState == NULL ) {
		return FALSE;
	}
	if ( objServer == NULL || objServer->DevMode != XS_DEV_SCRIPT_C ) {
		*ppState = NULL;
		return TRUE;
	}
	if ( objServer->DevFile == NULL || objServer->DevFile[0] == '\0' ) {
		XS_ReportError("server script load failed: server=%s devfile is empty", XS_ScriptServerName(objServer));
		return FALSE;
	}
	if ( xrtFileExists(objServer->DevFile) == FALSE ) {
		XS_ReportError("server script load failed: file not found: %s", objServer->DevFile);
		return FALSE;
	}
	
	s = XS_CreateTCC(objServer->Path, XS_ImportScriptAPI);
	if ( s == NULL ) {
		XS_ReportError("server script load failed: cannot create TCC: server=%s", XS_ScriptServerName(objServer));
		return FALSE;
	}
	
	sWorkPath = xrtPathGetDir(objServer->DevFile, 0);
	if ( sWorkPath && sWorkPath[0] != '\0' ) {
		tcc_add_include_path(s, sWorkPath);
		tcc_add_library_path(s, sWorkPath);
	}
	
	if ( tcc_add_file(s, objServer->DevFile) == -1 ) {
		if ( sWorkPath ) xrtFree(sWorkPath);
		XS_DestroyTCC(s);
		XS_ReportError("server script load failed: compile error: %s", objServer->DevFile);
		return FALSE;
	}
	
	if ( sWorkPath ) {
		xrtFree(sWorkPath);
	}
	
	if ( tcc_relocate(s) < 0 ) {
		XS_DestroyTCC(s);
		XS_ReportError("server script load failed: relocate error: %s", objServer->DevFile);
		return FALSE;
	}
	
	*ppState = s;
	return TRUE;
}

static inline void XS_AttachServerScriptState(XS_ServerConfig* objServer, TCCState* s)
{
	if ( objServer == NULL ) {
		return;
	}
	
	XS_ResetServerScriptState(objServer);
	objServer->pScriptState = s;
	if ( s == NULL ) {
		return;
	}
	
	objServer->procServiceInit = (XS_ScriptServiceProc)tcc_get_symbol(s, "ServiceInit");
	objServer->procServiceStart = (XS_ScriptServiceProc)tcc_get_symbol(s, "ServiceStart");
	objServer->procServiceStop = (XS_ScriptServiceProc)tcc_get_symbol(s, "ServiceStop");
	objServer->procServiceUnit = (XS_ScriptServiceProc)tcc_get_symbol(s, "ServiceUnit");
	objServer->procMessage = (XS_ScriptMessageProc)tcc_get_symbol(s, "MessageProc");
	objServer->procStreamOpen = (XS_ScriptStreamOpenProc)tcc_get_symbol(s, "EventOpenProc");
	objServer->procStreamData = (XS_ScriptStreamDataProc)tcc_get_symbol(s, "EventDataProc");
	objServer->procStreamClose = (XS_ScriptStreamCloseProc)tcc_get_symbol(s, "EventCloseProc");
	objServer->procDgramRecv = (XS_ScriptDgramRecvProc)tcc_get_symbol(s, "EventDgramProc");
	objServer->procXtpMessage = (XS_ScriptXtpMessageProc)tcc_get_symbol(s, "EventXtpProc");
	objServer->procSetGlobalData = (XS_ScriptSetGlobalDataProc)tcc_get_symbol(s, "XS_SetGlobalDate");
	
	if ( objServer->procSetGlobalData ) {
		objServer->procSetGlobalData(1, NULL);
		objServer->procSetGlobalData(2, NULL);
		objServer->procSetGlobalData(3, xrtInit());
	}
	
	XS_LogInfo(
		"server script loaded: server=%s file=%s",
		XS_ScriptServerName(objServer),
		objServer->DevFile ? objServer->DevFile : "(null)"
	);
}

static inline bool XS_LoadServerScriptObject(XS_ServerConfig* objServer)
{
	TCCState* s = NULL;
	
	if ( !XS_BuildServerScriptState(objServer, &s) ) {
		return FALSE;
	}
	
	XS_AttachServerScriptState(objServer, s);
	return TRUE;
}

static inline void XS_UnloadServerScriptObject(XS_ServerConfig* objServer)
{
	TCCState* s;
	
	if ( objServer == NULL ) {
		return;
	}
	
	s = (TCCState*)objServer->pScriptState;
	if ( objServer->procServiceUnit ) {
		objServer->procServiceUnit(objServer, NULL);
	}
	XS_ResetServerScriptState(objServer);
	if ( s ) {
		XS_DestroyTCC(s);
	}
}

static inline bool XS_BuildHostScriptState(XS_ServerConfig* objServer, XS_HostConfig* objHost, TCCState** ppState)
{
	TCCState* s;
	char* sWorkPath;
	
	if ( ppState == NULL ) {
		return FALSE;
	}
	
	if ( objHost == NULL || objHost->DevMode != XS_DEV_SCRIPT_C ) {
		*ppState = NULL;
		return TRUE;
	}
	
	if ( objHost->DevFile == NULL || objHost->DevFile[0] == '\0' ) {
		XS_ReportError(
			"script host load failed: server=%s host=%s devfile is empty",
			XS_ScriptServerName(objServer),
			XS_ScriptHostName(objHost)
		);
		return FALSE;
	}
	
	if ( xrtFileExists(objHost->DevFile) == FALSE ) {
		XS_ReportError(
			"script host load failed: file not found: %s",
			objHost->DevFile
		);
		return FALSE;
	}
	
	s = XS_CreateTCC(objHost->Path, XS_ImportScriptAPI);
	if ( s == NULL ) {
		XS_ReportError(
			"script host load failed: cannot create TCC: server=%s host=%s",
			XS_ScriptServerName(objServer),
			XS_ScriptHostName(objHost)
		);
		return FALSE;
	}
	
	sWorkPath = xrtPathGetDir(objHost->DevFile, 0);
	if ( sWorkPath && sWorkPath[0] != '\0' ) {
		tcc_add_include_path(s, sWorkPath);
		tcc_add_library_path(s, sWorkPath);
	}
	
	if ( tcc_add_file(s, objHost->DevFile) == -1 ) {
		if ( sWorkPath ) xrtFree(sWorkPath);
		XS_DestroyTCC(s);
		XS_ReportError(
			"script host load failed: compile error: %s",
			objHost->DevFile
		);
		return FALSE;
	}
	
	if ( sWorkPath ) {
		xrtFree(sWorkPath);
	}
	
	if ( tcc_relocate(s) < 0 ) {
		XS_DestroyTCC(s);
		XS_ReportError(
			"script host load failed: relocate error: %s",
			objHost->DevFile
		);
		return FALSE;
	}
	
	(void)objServer;
	*ppState = s;
	return TRUE;
}

static inline void XS_AttachHostScriptState(XS_ServerConfig* objServer, XS_HostConfig* objHost, TCCState* s)
{
	if ( objHost == NULL ) {
		return;
	}
	
	XS_ResetHostScriptState(objHost);
	objHost->pScriptState = s;
	if ( s == NULL ) {
		return;
	}
	
	objHost->procServiceInit = (XS_ScriptServiceProc)tcc_get_symbol(s, "ServiceInit");
	objHost->procServiceStart = (XS_ScriptServiceProc)tcc_get_symbol(s, "ServiceStart");
	objHost->procServiceStop = (XS_ScriptServiceProc)tcc_get_symbol(s, "ServiceStop");
	objHost->procServiceUnit = (XS_ScriptServiceProc)tcc_get_symbol(s, "ServiceUnit");
	objHost->procHttpRequest = (XS_ScriptHttpRequestProc)tcc_get_symbol(s, "RequestProc");
	objHost->procMessage = (XS_ScriptMessageProc)tcc_get_symbol(s, "MessageProc");
	objHost->procWsOpen = (XS_ScriptWsOpenProc)tcc_get_symbol(s, "WsOpenProc");
	objHost->procWsText = (XS_ScriptWsTextProc)tcc_get_symbol(s, "WsTextProc");
	objHost->procWsBinary = (XS_ScriptWsBinaryProc)tcc_get_symbol(s, "WsBinaryProc");
	objHost->procWsPing = (XS_ScriptWsPingProc)tcc_get_symbol(s, "WsPingProc");
	objHost->procWsPong = (XS_ScriptWsPongProc)tcc_get_symbol(s, "WsPongProc");
	objHost->procWsClose = (XS_ScriptWsCloseProc)tcc_get_symbol(s, "WsCloseProc");
	objHost->procSetGlobalData = (XS_ScriptSetGlobalDataProc)tcc_get_symbol(s, "XS_SetGlobalDate");
	
	if ( objHost->procSetGlobalData ) {
		objHost->procSetGlobalData(1, NULL);
		objHost->procSetGlobalData(2, NULL);
		objHost->procSetGlobalData(3, xrtInit());
	}
	
	XS_LogInfo(
		"script host loaded: server=%s host=%s file=%s",
		XS_ScriptServerName(objServer),
		XS_ScriptHostName(objHost),
		objHost->DevFile
	);
}

static inline XS_ScriptHttpRequestProc XS_GetHostHttpRequestProc(XS_HostConfig* objHost)
{
	TCCState* s;
	
	if ( objHost == NULL ) {
		return NULL;
	}
	
	s = (TCCState*)objHost->pScriptState;
	if ( s == NULL ) {
		return NULL;
	}
	
	return (XS_ScriptHttpRequestProc)tcc_get_symbol(s, "RequestProc");
}

static inline bool XS_LoadHostScript(XS_ServerConfig* objServer, XS_HostConfig* objHost)
{
	TCCState* s = NULL;
	
	if ( !XS_BuildHostScriptState(objServer, objHost, &s) ) {
		return FALSE;
	}
	
	XS_AttachHostScriptState(objServer, objHost, s);
	return TRUE;
}

static inline bool XS_ReloadHostScript(XS_ServerConfig* objServer, XS_HostConfig* objHost)
{
	TCCState* sNew = NULL;
	TCCState* sOld;
	XS_ScriptServiceProc procOldServiceInit;
	XS_ScriptServiceProc procOldServiceStart;
	XS_ScriptServiceProc procOldServiceStop;
	XS_ScriptServiceProc procOldServiceUnit;
	XS_ScriptHttpRequestProc procOldHttpRequest;
	
	if ( objHost == NULL || objHost->DevMode != XS_DEV_SCRIPT_C ) {
		return TRUE;
	}
	
	if ( !XS_BuildHostScriptState(objServer, objHost, &sNew) ) {
		return FALSE;
	}
	
	sOld = (TCCState*)objHost->pScriptState;
	procOldServiceInit = objHost->procServiceInit;
	procOldServiceStart = objHost->procServiceStart;
	procOldServiceStop = objHost->procServiceStop;
	procOldServiceUnit = objHost->procServiceUnit;
	procOldHttpRequest = objHost->procHttpRequest;
	
	if ( procOldServiceStop ) {
		procOldServiceStop(objServer, objHost);
	}
	if ( procOldServiceUnit ) {
		procOldServiceUnit(objServer, objHost);
	}
	
	XS_AttachHostScriptState(objServer, objHost, sNew);
	
	if ( objHost->procServiceInit ) {
		objHost->procServiceInit(objServer, objHost);
	}
	if ( objHost->procServiceStart ) {
		objHost->procServiceStart(objServer, objHost);
	}
	
	if ( sOld ) {
		XS_DestroyTCC(sOld);
	}
	
	(void)procOldServiceInit;
	(void)procOldServiceStart;
	(void)procOldHttpRequest;
	return TRUE;
}

static inline void XS_UnloadHostScript(XS_ServerConfig* objServer, XS_HostConfig* objHost)
{
	TCCState* s;
	
	if ( objHost == NULL ) {
		return;
	}
	
	s = (TCCState*)objHost->pScriptState;
	if ( objHost->procServiceUnit ) {
		objHost->procServiceUnit(objServer, objHost);
	}
	
	XS_ResetHostScriptState(objHost);
	if ( s ) {
		XS_DestroyTCC(s);
	}
}

static inline bool XS_LoadServerScripts(XS_ServerConfig* objServer)
{
	uint32 i;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	
	if ( objServer->HostAware ) {
		if ( objServer->EnableDefaultHost ) {
			if ( !XS_LoadHostScript(objServer, &objServer->DefaultHost) ) {
				return FALSE;
			}
		}
		
		for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
			XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
			if ( !XS_LoadHostScript(objServer, objHost) ) {
				return FALSE;
			}
		}
	}
	else if ( objServer->DevMode == XS_DEV_SCRIPT_C ) {
		if ( !XS_LoadServerScriptObject(objServer) ) {
			return FALSE;
		}
	}
	
	return TRUE;
}

static inline bool XS_InitServerScripts(XS_ServerConfig* objServer)
{
	uint32 i;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	
	if ( objServer->HostAware ) {
		if ( objServer->EnableDefaultHost && objServer->DefaultHost.procServiceInit ) {
			objServer->DefaultHost.procServiceInit(objServer, &objServer->DefaultHost);
		}
		
		for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
			XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
			if ( objHost->procServiceInit ) {
				objHost->procServiceInit(objServer, objHost);
			}
		}
	}
	else if ( objServer->procServiceInit ) {
		objServer->procServiceInit(objServer, NULL);
	}
	
	return TRUE;
}

static inline bool XS_StartServerScripts(XS_ServerConfig* objServer)
{
	uint32 i;
	
	if ( objServer == NULL ) {
		return FALSE;
	}
	
	if ( objServer->HostAware ) {
		if ( objServer->EnableDefaultHost && objServer->DefaultHost.procServiceStart ) {
			objServer->DefaultHost.procServiceStart(objServer, &objServer->DefaultHost);
		}
		
		for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
			XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
			if ( objHost->procServiceStart ) {
				objHost->procServiceStart(objServer, objHost);
			}
		}
	}
	else if ( objServer->procServiceStart ) {
		objServer->procServiceStart(objServer, NULL);
	}
	
	return TRUE;
}

static inline void XS_StopServerScripts(XS_ServerConfig* objServer)
{
	uint32 i;
	
	if ( objServer == NULL ) {
		return;
	}
	
	if ( objServer->HostAware ) {
		if ( objServer->EnableDefaultHost && objServer->DefaultHost.procServiceStop ) {
			objServer->DefaultHost.procServiceStop(objServer, &objServer->DefaultHost);
		}
		
		for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
			XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
			if ( objHost->procServiceStop ) {
				objHost->procServiceStop(objServer, objHost);
			}
		}
	}
	else if ( objServer->procServiceStop ) {
		objServer->procServiceStop(objServer, NULL);
	}
}

static inline void XS_UnloadServerScripts(XS_ServerConfig* objServer)
{
	uint32 i;
	
	if ( objServer == NULL ) {
		return;
	}
	
	if ( objServer->HostAware ) {
		if ( objServer->EnableDefaultHost ) {
			XS_UnloadHostScript(objServer, &objServer->DefaultHost);
		}
		
		for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
			XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
			XS_UnloadHostScript(objServer, objHost);
		}
	}
	else {
		XS_UnloadServerScriptObject(objServer);
	}
}

#endif
