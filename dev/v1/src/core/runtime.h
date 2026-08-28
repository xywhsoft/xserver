#ifndef XS_CORE_RUNTIME_H
#define XS_CORE_RUNTIME_H

typedef struct {
	xnetengine* pEngine;
	xarray Servers;
} XS_Runtime;

static inline bool XS_RuntimeInitOneServer(XS_Runtime* objRuntime, XS_ServerConfig* objServer)
{
	if ( objRuntime == NULL || objServer == NULL ) {
		return FALSE;
	}
	
	if ( !XS_LoadServerScripts(objServer) ) {
		return FALSE;
	}
	if ( !XS_InitServerScripts(objServer) ) {
		return FALSE;
	}
	
	switch ( objServer->Class ) {
		case XS_SVC_HTTP:
			if ( !XS_HttpInitServer(objRuntime->pEngine, objServer) ) {
				return FALSE;
			}
			break;
		case XS_SVC_WS:
			if ( !XS_WsInitServer(objRuntime->pEngine, objServer) ) {
				return FALSE;
			}
			break;
		case XS_SVC_TCP:
			if ( !XS_TcpInitServer(objRuntime->pEngine, objServer) ) {
				return FALSE;
			}
			break;
		case XS_SVC_UDP:
			if ( !XS_UdpInitServer(objRuntime->pEngine, objServer) ) {
				return FALSE;
			}
			break;
		case XS_SVC_XTP:
			if ( !XS_XtpInitServer(objRuntime->pEngine, objServer) ) {
				return FALSE;
			}
			break;
		case XS_SVC_CUSTOM:
			if ( !XS_CustomInitServer(objRuntime->pEngine, objServer) ) {
				return FALSE;
			}
			break;
		default:
			XS_LogInfo(
				"runtime init placeholder: class=%s server=%s",
				XS_ServerClassName(objServer->Class),
				objServer->Name ? objServer->Name : "(null)"
			);
			break;
	}
	
	return TRUE;
}

static inline bool XS_RuntimeStartOneServer(XS_ServerConfig* objServer)
{
	if ( objServer == NULL ) {
		return FALSE;
	}
	
	if ( !XS_StartServerScripts(objServer) ) {
		return FALSE;
	}
	
	switch ( objServer->Class ) {
		case XS_SVC_HTTP:
			if ( !XS_HttpStartServer(objServer) ) {
				return FALSE;
			}
			break;
		case XS_SVC_WS:
			if ( !XS_WsStartServer(objServer) ) {
				return FALSE;
			}
			break;
		case XS_SVC_TCP:
			if ( !XS_TcpStartServer(objServer) ) {
				return FALSE;
			}
			break;
		case XS_SVC_UDP:
			if ( !XS_UdpStartServer(objServer) ) {
				return FALSE;
			}
			break;
		case XS_SVC_XTP:
			if ( !XS_XtpStartServer(objServer) ) {
				return FALSE;
			}
			break;
		case XS_SVC_CUSTOM:
			if ( !XS_CustomStartServer(objServer) ) {
				return FALSE;
			}
			break;
		default:
			XS_LogInfo(
				"runtime start placeholder: class=%s server=%s",
				XS_ServerClassName(objServer->Class),
				objServer->Name ? objServer->Name : "(null)"
			);
			break;
	}
	
	return TRUE;
}

static inline void XS_RuntimeStopOneServer(XS_ServerConfig* objServer)
{
	if ( objServer == NULL ) {
		return;
	}
	
	XS_StopServerScripts(objServer);
	
	switch ( objServer->Class ) {
		case XS_SVC_HTTP:
			XS_HttpStopServer(objServer);
			break;
		case XS_SVC_WS:
			XS_WsStopServer(objServer);
			break;
		case XS_SVC_TCP:
			XS_TcpStopServer(objServer);
			break;
		case XS_SVC_UDP:
			XS_UdpStopServer(objServer);
			break;
		case XS_SVC_XTP:
			XS_XtpStopServer(objServer);
			break;
		case XS_SVC_CUSTOM:
			XS_CustomStopServer(objServer);
			break;
		default:
			XS_LogInfo(
				"runtime stop placeholder: class=%s server=%s",
				XS_ServerClassName(objServer->Class),
				objServer->Name ? objServer->Name : "(null)"
			);
			break;
	}
	
	XS_UnloadServerScripts(objServer);
}

static inline void XS_InitRuntime(XS_Runtime* objRuntime)
{
	memset(objRuntime, 0, sizeof(XS_Runtime));
	objRuntime->Servers = xrtArrayCreate(sizeof(XS_ServerConfig), XRT_OBJMODE_LOCAL);
}

static inline void XS_FreeRuntime(XS_Runtime* objRuntime)
{
	uint32 i;
	
	if ( objRuntime == NULL ) {
		return;
	}
	
	if ( objRuntime->pEngine ) {
		xrtNetEngineStop(objRuntime->pEngine);
		xrtNetEngineDestroy(objRuntime->pEngine);
	}
	
	if ( objRuntime->Servers ) {
		for ( i = 1; i <= objRuntime->Servers->Count; i++ ) {
			XS_ServerConfig* objServer = xrtArrayGet_Inline(objRuntime->Servers, i);
			XS_UnloadServerScripts(objServer);
			XS_FreeServerConfig(objServer);
		}
		xrtArrayDestroy(objRuntime->Servers);
	}
	
	memset(objRuntime, 0, sizeof(XS_Runtime));
}

static inline bool XS_RuntimeBuild(XS_Runtime* objRuntime, XS_Config* objCfg)
{
	uint32 i;
	
	XS_InitRuntime(objRuntime);
	
	if ( objCfg == NULL || objCfg->Servers == NULL ) {
		XS_ReportError("runtime build failed: config is empty");
		return FALSE;
	}
	
	{
		xnetengineconfig tEngineCfg;
		xrtNetEngineConfigInit(&tEngineCfg);
		objRuntime->pEngine = xrtNetEngineCreate(&tEngineCfg);
		if ( objRuntime->pEngine == NULL ) {
			XS_ReportError("runtime build failed: cannot create xnet engine");
			return FALSE;
		}
		if ( xrtNetEngineStart(objRuntime->pEngine) != XRT_NET_OK ) {
			XS_ReportError("runtime build failed: cannot start xnet engine");
			return FALSE;
		}
	}
	
	for ( i = 1; i <= objCfg->Servers->Count; i++ ) {
		XS_ServerConfig* objSrc = xrtArrayGet_Inline(objCfg->Servers, i);
		uint32 idx = xrtArrayAppend(objRuntime->Servers, 1);
		XS_ServerConfig* objDst = xrtArrayGet_Inline(objRuntime->Servers, idx);
		
		memcpy(objDst, objSrc, sizeof(XS_ServerConfig));
		XS_ServerBindTlsCallback(objDst);
		memset(objSrc, 0, sizeof(XS_ServerConfig));
	}
	
	objCfg->Servers->Count = 0;
	return TRUE;
}

static inline void XS_RuntimePrint(XS_Runtime* objRuntime)
{
	uint32 i;
	
	if ( objRuntime == NULL || objRuntime->Servers == NULL ) {
		return;
	}
	
	XS_LogInfo("runtime servers : %u", objRuntime->Servers->Count);
	XS_LogInfo("runtime engine workers : %u", objRuntime->pEngine ? xrtNetEngineGetWorkerCount(objRuntime->pEngine) : 0);
	
	for ( i = 1; i <= objRuntime->Servers->Count; i++ ) {
		XS_ServerConfig* objServer = xrtArrayGet_Inline(objRuntime->Servers, i);
		XS_LogInfo(
			"runtime server[%u] class=%s addr=%s name=%s",
			i - 1,
			XS_ServerClassName(objServer->Class),
			objServer->Addr ? objServer->Addr : "(null)",
			objServer->Name ? objServer->Name : "(null)"
		);
	}
}

static inline bool XS_RuntimeInitServers(XS_Runtime* objRuntime)
{
	uint32 i;
	
	if ( objRuntime == NULL || objRuntime->Servers == NULL ) {
		XS_ReportError("runtime init failed: server list is empty");
		return FALSE;
	}
	
	for ( i = 1; i <= objRuntime->Servers->Count; i++ ) {
		XS_ServerConfig* objServer = xrtArrayGet_Inline(objRuntime->Servers, i);
		if ( !XS_RuntimeInitOneServer(objRuntime, objServer) ) {
			return FALSE;
		}
	}
	
	return TRUE;
}

static inline bool XS_RuntimeStartServers(XS_Runtime* objRuntime)
{
	uint32 i;
	
	if ( objRuntime == NULL || objRuntime->Servers == NULL ) {
		return FALSE;
	}
	
	for ( i = 1; i <= objRuntime->Servers->Count; i++ ) {
		XS_ServerConfig* objServer = xrtArrayGet_Inline(objRuntime->Servers, i);
		if ( !XS_RuntimeStartOneServer(objServer) ) {
			return FALSE;
		}
	}
	
	return TRUE;
}

static inline void XS_RuntimeStopServers(XS_Runtime* objRuntime)
{
	uint32 i;
	
	if ( objRuntime == NULL || objRuntime->Servers == NULL ) {
		return;
	}
	
	for ( i = 1; i <= objRuntime->Servers->Count; i++ ) {
		XS_ServerConfig* objServer = xrtArrayGet_Inline(objRuntime->Servers, i);
		XS_RuntimeStopOneServer(objServer);
	}
}

#endif
