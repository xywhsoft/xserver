#ifndef XS_SCRIPT_DYNLOAD_H
#define XS_SCRIPT_DYNLOAD_H

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
	objHost->procSetGlobalData = NULL;
}

static inline bool XS_BuildHostScriptState(XS_ServerConfig* objServer, XS_HostConfig* objHost, TCCState** ppState)
{
	TCCState* s;
	char* sWorkPath;
	char* sCode;
	
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
	
	sCode = xrtFileReadAll(objHost->DevFile, XRT_CP_BINARY, NULL);
	if ( sCode == NULL ) {
		if ( sWorkPath ) xrtFree(sWorkPath);
		XS_DestroyTCC(s);
		XS_ReportError(
			"script host load failed: read error: %s",
			objHost->DevFile
		);
		return FALSE;
	}
	
	if ( tcc_compile_string(s, sCode) == -1 ) {
		xrtFree(sCode);
		if ( sWorkPath ) xrtFree(sWorkPath);
		XS_DestroyTCC(s);
		XS_ReportError(
			"script host load failed: compile error: %s",
			objHost->DevFile
		);
		return FALSE;
	}
	
	xrtFree(sCode);
	
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
}

#endif
