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
