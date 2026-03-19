#ifndef XS_CORE_SERVER_H
#define XS_CORE_SERVER_H

typedef enum {
	XS_SVC_NONE = 0,
	XS_SVC_HTTP = 1,
	XS_SVC_WS = 2,
	XS_SVC_TCP = 3,
	XS_SVC_UDP = 4,
	XS_SVC_XTP = 5,
	XS_SVC_CUSTOM = 6
} XS_ServerClass;

typedef void (*XS_ScriptStreamOpenProc)(ptr objServer, void* pStream);
typedef bool (*XS_ScriptStreamDataProc)(ptr objServer, void* pStream, const void* pData, size_t iLen);
typedef void (*XS_ScriptStreamCloseProc)(ptr objServer, void* pStream, int iReason);
typedef bool (*XS_ScriptDgramRecvProc)(ptr objServer, void* pSock, const xnetaddr* pFrom, const void* pData, size_t iLen);
typedef bool (*XS_ScriptXtpMessageProc)(ptr objServer, void* pStream, void* pMsg);

typedef struct XS_ServerConfig {
	bool Enabled;
	XS_ServerClass Class;
	char* ClassName;
	char* Name;
	char* Desc;
	char* Param;
	uint32 Backlog;
	uint32 RecvLimit;
	uint32 WsMessageLimit;
	uint32 PathLimit;
	uint32 HeaderLimit;
	uint32 BodyLimit;
	char* BindIP;
	uint16 BindPort;
	char* Addr;
	char* WsProtocol;
	bool EnableTLS;
	char* BindIPTLS;
	uint16 BindPortTLS;
	char* AddrTLS;
	xtlsconfig TlsConfig;
	bool Debug;
	bool HostAware;
	bool EnableDefaultHost;
	XS_HostConfig DefaultHost;
	xarray Hosts;
	XS_DevMode DevMode;
	char* Path;
	char* DevFile;
	ptr pScriptState;
	XS_ScriptServiceProc procServiceInit;
	XS_ScriptServiceProc procServiceStart;
	XS_ScriptServiceProc procServiceStop;
	XS_ScriptServiceProc procServiceUnit;
	XS_ScriptMessageProc procMessage;
	XS_ScriptStreamOpenProc procStreamOpen;
	XS_ScriptStreamDataProc procStreamData;
	XS_ScriptStreamCloseProc procStreamClose;
	XS_ScriptDgramRecvProc procDgramRecv;
	XS_ScriptXtpMessageProc procXtpMessage;
	XS_ScriptSetGlobalDataProc procSetGlobalData;
	ptr pHandle;
} XS_ServerConfig;

static inline const char* XS_ServerClassName(XS_ServerClass iClass)
{
	switch ( iClass ) {
		case XS_SVC_HTTP: return "http";
		case XS_SVC_WS: return "ws";
		case XS_SVC_TCP: return "tcp";
		case XS_SVC_UDP: return "udp";
		case XS_SVC_XTP: return "xtp";
		case XS_SVC_CUSTOM: return "custom";
		default: return "unknown";
	}
}

static inline XS_ServerClass XS_ParseServerClass(const char* sClass)
{
	if ( sClass == NULL ) return XS_SVC_NONE;
	if ( strcasecmp(sClass, "http") == 0 ) return XS_SVC_HTTP;
	if ( strcasecmp(sClass, "ws") == 0 ) return XS_SVC_WS;
	if ( strcasecmp(sClass, "tcp") == 0 ) return XS_SVC_TCP;
	if ( strcasecmp(sClass, "udp") == 0 ) return XS_SVC_UDP;
	if ( strcasecmp(sClass, "xtp") == 0 ) return XS_SVC_XTP;
	if ( strcasecmp(sClass, "custom") == 0 ) return XS_SVC_CUSTOM;
	return XS_SVC_NONE;
}

static inline bool XS_IsHostAwareClass(XS_ServerClass iClass)
{
	return (iClass == XS_SVC_HTTP) || (iClass == XS_SVC_WS);
}

static inline void XS_InitServerConfig(XS_ServerConfig* objServer)
{
	memset(objServer, 0, sizeof(XS_ServerConfig));
	objServer->Enabled = TRUE;
	objServer->Backlog = 128u;
	objServer->RecvLimit = 1024u * 1024u;
	objServer->WsMessageLimit = 256u * 1024u;
	objServer->PathLimit = 200u;
	objServer->HeaderLimit = XHTTPD_MAX_HEADERS;
	objServer->BodyLimit = 256u * 1024u;
	objServer->Hosts = xrtArrayCreate(sizeof(XS_HostConfig), XRT_OBJMODE_LOCAL);
	objServer->DevMode = XS_DEV_PROTOCOL;
	objServer->pScriptState = NULL;
	objServer->pHandle = NULL;
	XS_InitHostConfig(&objServer->DefaultHost);
}

static inline void XS_FreeServerConfig(XS_ServerConfig* objServer)
{
	uint32 i;
	
	if ( objServer == NULL ) {
		return;
	}
	
	if ( objServer->ClassName ) xrtFree(objServer->ClassName);
	if ( objServer->Name ) xrtFree(objServer->Name);
	if ( objServer->Desc ) xrtFree(objServer->Desc);
	if ( objServer->Param ) xrtFree(objServer->Param);
	if ( objServer->BindIP ) xrtFree(objServer->BindIP);
	if ( objServer->Addr ) xrtFree(objServer->Addr);
	if ( objServer->WsProtocol ) xrtFree(objServer->WsProtocol);
	if ( objServer->BindIPTLS ) xrtFree(objServer->BindIPTLS);
	if ( objServer->AddrTLS ) xrtFree(objServer->AddrTLS);
	if ( objServer->TlsConfig.sCaFile ) xrtFree((void*)objServer->TlsConfig.sCaFile);
	if ( objServer->TlsConfig.sCertFile ) xrtFree((void*)objServer->TlsConfig.sCertFile);
	if ( objServer->TlsConfig.sKeyFile ) xrtFree((void*)objServer->TlsConfig.sKeyFile);
	if ( objServer->Path ) xrtFree(objServer->Path);
	if ( objServer->DevFile ) xrtFree(objServer->DevFile);
	
	XS_FreeHostConfig(&objServer->DefaultHost);
	
	if ( objServer->Hosts ) {
		for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
			XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
			XS_FreeHostConfig(objHost);
		}
		xrtArrayDestroy(objServer->Hosts);
	}
	
	memset(objServer, 0, sizeof(XS_ServerConfig));
}

#endif
