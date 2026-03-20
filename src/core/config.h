#ifndef XS_CORE_CONFIG_H
#define XS_CORE_CONFIG_H

typedef struct {
	char* FilePath;
	char* BaseDir;
	xvalue Root;
	xarray Servers;
} XS_Config;

static inline XS_DevMode XS_ParseDevMode(const char* sText, XS_DevMode iDefault)
{
	if ( sText == NULL || sText[0] == '\0' ) {
		return iDefault;
	}
	
	if ( strcasecmp(sText, "static") == 0 ) {
		return XS_DEV_STATIC;
	}
	if ( strcasecmp(sText, "c") == 0 || strcasecmp(sText, "script-c") == 0 ) {
		return XS_DEV_SCRIPT_C;
	}
	if ( strcasecmp(sText, "protocol") == 0 ) {
		return XS_DEV_PROTOCOL;
	}
	
	return iDefault;
}

static inline const char* XS_DevModeName(XS_DevMode iMode)
{
	switch ( iMode ) {
		case XS_DEV_STATIC: return "static";
		case XS_DEV_SCRIPT_C: return "script-c";
		case XS_DEV_PROTOCOL: return "protocol";
		default: return "unknown";
	}
}

static bool XS_WarnUnknownFieldProc(Dict_Key* pKey, ptr pVal, ptr pArg)
{
	char sKeyBuf[256];
	size_t iKeyLen;
	struct {
		const XS_FieldRule* arrRule;
		size_t iRuleCount;
		const char* sScope;
	} *objCtx = pArg;
	
	(void)pVal;

	if ( pKey == NULL || pKey->Key == NULL ) {
		return FALSE;
	}

	iKeyLen = pKey->KeyLen;
	if ( iKeyLen >= sizeof(sKeyBuf) ) {
		iKeyLen = sizeof(sKeyBuf) - 1;
	}
	memcpy(sKeyBuf, pKey->Key, iKeyLen);
	sKeyBuf[iKeyLen] = '\0';

	if ( !XS_FindFieldRule(objCtx->arrRule, objCtx->iRuleCount, sKeyBuf, strlen(sKeyBuf)) ) {
		XS_ReportWarn("%s contains unknown field: %s", objCtx->sScope, sKeyBuf);
	}
	
	return FALSE;
}

static inline void XS_WarnUnknownFields(xvalue objTable, const XS_FieldRule* arrRule, size_t iRuleCount, const char* sScope)
{
	struct {
		const XS_FieldRule* arrRule;
		size_t iRuleCount;
		const char* sScope;
	} objCtx;
	
	if ( objTable == NULL || objTable->Type != XVO_DT_TABLE ) {
		return;
	}
	
	objCtx.arrRule = arrRule;
	objCtx.iRuleCount = iRuleCount;
	objCtx.sScope = sScope;
	xrtDictWalk(objTable->vTable, XS_WarnUnknownFieldProc, &objCtx);
}

static inline bool XS_RequireFieldType(xvalue objTable, const char* sKey, int iKeyLen, int iType, const char* sScope, bool bRequired)
{
	xvalue objVal = xvoTableGetValue(objTable, (char*)sKey, iKeyLen);
	
	if ( objVal == NULL || objVal->Type == XVO_DT_NULL ) {
		if ( bRequired ) {
			XS_ReportError("%s missing required field: %s", sScope, sKey);
			return FALSE;
		}
		return TRUE;
	}
	
	if ( objVal->Type != iType ) {
		XS_ReportError("%s field type invalid: %s", sScope, sKey);
		return FALSE;
	}
	
	return TRUE;
}

static inline void XS_InitConfig(XS_Config* objCfg)
{
	memset(objCfg, 0, sizeof(XS_Config));
	objCfg->Servers = xrtArrayCreate(sizeof(XS_ServerConfig), XRT_OBJMODE_LOCAL);
}

static inline void XS_FreeConfig(XS_Config* objCfg)
{
	uint32 i;
	
	if ( objCfg == NULL ) {
		return;
	}
	
	if ( objCfg->Servers ) {
		for ( i = 1; i <= objCfg->Servers->Count; i++ ) {
			XS_ServerConfig* objServer = xrtArrayGet_Inline(objCfg->Servers, i);
			XS_FreeServerConfig(objServer);
		}
		xrtArrayDestroy(objCfg->Servers);
	}
	
	if ( objCfg->Root ) {
		xvoUnref(objCfg->Root);
	}
	if ( objCfg->FilePath ) xrtFree(objCfg->FilePath);
	if ( objCfg->BaseDir ) xrtFree(objCfg->BaseDir);
	
	memset(objCfg, 0, sizeof(XS_Config));
}

static inline bool XS_LoadTlsConfig(xvalue objTable, const char* sBaseDir, xtlsconfig* pTlsCfg)
{
	const char* sCert;
	const char* sKey;
	const char* sCA;
	
	memset(pTlsCfg, 0, sizeof(xtlsconfig));
	sCert = xvoTableGetText(objTable, "tls_cert", 8);
	sKey = xvoTableGetText(objTable, "tls_key", 7);
	sCA = xvoTableGetText(objTable, "tls_ca", 6);
	
	if ( sCert && sCert[0] != '\0' ) {
		pTlsCfg->sCertFile = XS_NormalizePath(sBaseDir, sCert);
	}
	if ( sKey && sKey[0] != '\0' ) {
		pTlsCfg->sKeyFile = XS_NormalizePath(sBaseDir, sKey);
	}
	if ( sCA && sCA[0] != '\0' ) {
		pTlsCfg->sCaFile = XS_NormalizePath(sBaseDir, sCA);
	}
	
	return TRUE;
}

static inline bool XS_LoadHostConfig(xvalue objTable, const char* sBaseDir, XS_HostConfig* objHost, const char* sScope)
{
	static const XS_FieldRule arrRule[] = {
		{"enabled", XVO_DT_BOOL, FALSE},
		{"name", XVO_DT_TEXT, FALSE},
		{"desc", XVO_DT_TEXT, FALSE},
		{"host", XVO_DT_TEXT, FALSE},
		{"param", XVO_DT_TEXT, FALSE},
		{"debug", XVO_DT_BOOL, FALSE},
		{"path", XVO_DT_TEXT, FALSE},
		{"devlang", XVO_DT_TEXT, FALSE},
		{"devfile", XVO_DT_TEXT, FALSE},
		{"tls_ca", XVO_DT_TEXT, FALSE},
		{"tls_cert", XVO_DT_TEXT, FALSE},
		{"tls_key", XVO_DT_TEXT, FALSE}
	};
	const char* sPath;
	const char* sDevFile;
	const char* sDevLang;
	
	if ( objTable == NULL || objTable->Type != XVO_DT_TABLE ) {
		XS_ReportError("%s is not object", sScope);
		return FALSE;
	}
	
	XS_WarnUnknownFields(objTable, arrRule, sizeof(arrRule) / sizeof(arrRule[0]), sScope);
	XS_InitHostConfig(objHost);
	
	if ( !XS_RequireFieldType(objTable, "enabled", 7, XVO_DT_BOOL, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "name", 4, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "desc", 4, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "host", 4, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "param", 5, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "debug", 5, XVO_DT_BOOL, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "path", 4, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "devlang", 7, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "devfile", 7, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "tls_ca", 6, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "tls_cert", 8, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "tls_key", 7, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	
	objHost->Enabled = xvoTableGetBool(objTable, "enabled", 7);
	objHost->Name = XS_CopyText(xvoTableGetText(objTable, "name", 4));
	objHost->Desc = XS_CopyText(xvoTableGetText(objTable, "desc", 4));
	objHost->Host = XS_CopyText(xvoTableGetText(objTable, "host", 4));
	objHost->Param = XS_CopyText(xvoTableGetText(objTable, "param", 5));
	objHost->Debug = xvoTableGetBool(objTable, "debug", 5);
	
	sPath = xvoTableGetText(objTable, "path", 4);
	sDevFile = xvoTableGetText(objTable, "devfile", 7);
	sDevLang = xvoTableGetText(objTable, "devlang", 7);
	
	objHost->DevMode = XS_ParseDevMode(sDevLang, XS_DEV_STATIC);
	objHost->Path = XS_NormalizePath(sBaseDir, sPath);
	
	if ( sDevFile && sDevFile[0] != '\0' ) {
		objHost->DevFile = XS_NormalizePath(sBaseDir, sDevFile);
	} else if ( objHost->DevMode == XS_DEV_SCRIPT_C && objHost->Path ) {
		objHost->DevFile = xrtPathJoin(2, objHost->Path, "main.c");
	}
	
	XS_LoadTlsConfig(objTable, sBaseDir, &objHost->TlsConfig);
	return TRUE;
}

static inline char* XS_MakeBindText(const char* sIP, uint16 iPort)
{
	char sBuf[320];
	const char* sBindIP;
	
	if ( sIP == NULL || sIP[0] == '\0' || iPort == 0 ) {
		return NULL;
	}
	
	sBindIP = sIP;
	if ( strchr(sBindIP, ':') != NULL && sBindIP[0] != '[' ) {
		snprintf(sBuf, sizeof(sBuf), "[%s]:%u", sBindIP, (unsigned)iPort);
	} else {
		snprintf(sBuf, sizeof(sBuf), "%s:%u", sBindIP, (unsigned)iPort);
	}
	
	return xrtCopyStr(sBuf, 0);
}

static inline bool XS_LoadBindConfig(
	xvalue objTable,
	const char* sIPKey,
	int iIPKeyLen,
	const char* sPortKey,
	int iPortKeyLen,
	char** ppIP,
	uint16* pPort,
	char** ppAddr,
	const char* sScope,
	bool bRequired
)
{
	const char* sIPText;
	xvalue objPortVal;
	
	if ( ppIP ) *ppIP = NULL;
	if ( pPort ) *pPort = 0;
	if ( ppAddr ) *ppAddr = NULL;
	
	sIPText = xvoTableGetText(objTable, (char*)sIPKey, iIPKeyLen);
	objPortVal = xvoTableGetValue(objTable, (char*)sPortKey, iPortKeyLen);
	
	if ( sIPText && sIPText[0] != '\0' ) {
		int64 iPort64;
		
		if ( objPortVal == NULL || objPortVal->Type == XVO_DT_NULL ) {
			XS_ReportError("%s missing required field: %s", sScope, sPortKey);
			return FALSE;
		}
		if ( objPortVal->Type != XVO_DT_INT ) {
			XS_ReportError("%s field type invalid: %s", sScope, sPortKey);
			return FALSE;
		}
		
		iPort64 = xvoGetInt(objPortVal);
		if ( iPort64 <= 0 || iPort64 > 65535 ) {
			XS_ReportError("%s field out of range: %s", sScope, sPortKey);
			return FALSE;
		}
		
		if ( ppIP ) {
			*ppIP = XS_CopyText(sIPText);
			if ( *ppIP == NULL ) {
				XS_ReportError("%s alloc failed: %s", sScope, sIPKey);
				return FALSE;
			}
		}
		if ( pPort ) {
			*pPort = (uint16)iPort64;
		}
		if ( ppAddr ) {
			*ppAddr = XS_MakeBindText(sIPText, (uint16)iPort64);
		}
		return TRUE;
	}
	
	if ( bRequired ) {
		XS_ReportError("%s missing bind fields: %s/%s", sScope, sIPKey, sPortKey);
		return FALSE;
	}
	
	return TRUE;
}

static inline bool XS_LoadServerConfig(xvalue objTable, const char* sBaseDir, XS_ServerConfig* objServer, int iIndex)
{
	static const XS_FieldRule arrRule[] = {
		{"enabled", XVO_DT_BOOL, FALSE},
		{"class", XVO_DT_TEXT, TRUE},
		{"name", XVO_DT_TEXT, FALSE},
		{"desc", XVO_DT_TEXT, FALSE},
		{"param", XVO_DT_TEXT, FALSE},
		{"backlog", XVO_DT_INT, FALSE},
		{"conn_limit", XVO_DT_INT, FALSE},
		{"recv_limit", XVO_DT_INT, FALSE},
		{"idle_timeout", XVO_DT_INT, FALSE},
		{"ws_message_limit", XVO_DT_INT, FALSE},
		{"path_limit", XVO_DT_INT, FALSE},
		{"header_limit", XVO_DT_INT, FALSE},
		{"body_limit", XVO_DT_INT, FALSE},
		{"ip", XVO_DT_TEXT, FALSE},
		{"port", XVO_DT_INT, FALSE},
		{"ws_protocol", XVO_DT_TEXT, FALSE},
		{"tls", XVO_DT_BOOL, FALSE},
		{"ip_tls", XVO_DT_TEXT, FALSE},
		{"port_tls", XVO_DT_INT, FALSE},
		{"debug", XVO_DT_BOOL, FALSE},
		{"path", XVO_DT_TEXT, FALSE},
		{"devlang", XVO_DT_TEXT, FALSE},
		{"devfile", XVO_DT_TEXT, FALSE},
		{"tls_ca", XVO_DT_TEXT, FALSE},
		{"tls_cert", XVO_DT_TEXT, FALSE},
		{"tls_key", XVO_DT_TEXT, FALSE},
		{"host_default", XVO_DT_TABLE, FALSE},
		{"hosts", XVO_DT_ARRAY, FALSE}
	};
	char sScope[64];
	const char* sClass;
	xvalue objHostDef;
	xvalue arrHost;
	xvalue objVal;
	uint32 i;
	
	sprintf(sScope, "server[%d]", iIndex);
	
	if ( objTable == NULL || objTable->Type != XVO_DT_TABLE ) {
		XS_ReportError("%s is not object", sScope);
		return FALSE;
	}
	
	XS_WarnUnknownFields(objTable, arrRule, sizeof(arrRule) / sizeof(arrRule[0]), sScope);
	XS_InitServerConfig(objServer);
	
	if ( !XS_RequireFieldType(objTable, "enabled", 7, XVO_DT_BOOL, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "class", 5, XVO_DT_TEXT, sScope, TRUE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "name", 4, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "desc", 4, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "param", 5, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "backlog", 7, XVO_DT_INT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "conn_limit", 10, XVO_DT_INT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "recv_limit", 10, XVO_DT_INT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "idle_timeout", 12, XVO_DT_INT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "ws_message_limit", 16, XVO_DT_INT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "path_limit", 10, XVO_DT_INT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "header_limit", 12, XVO_DT_INT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "body_limit", 10, XVO_DT_INT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "ip", 2, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "port", 4, XVO_DT_INT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "ws_protocol", 11, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "tls", 3, XVO_DT_BOOL, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "ip_tls", 6, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "port_tls", 8, XVO_DT_INT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "debug", 5, XVO_DT_BOOL, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "path", 4, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "devlang", 7, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "devfile", 7, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "tls_ca", 6, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "tls_cert", 8, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "tls_key", 7, XVO_DT_TEXT, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "host_default", 12, XVO_DT_TABLE, sScope, FALSE) ) return FALSE;
	if ( !XS_RequireFieldType(objTable, "hosts", 5, XVO_DT_ARRAY, sScope, FALSE) ) return FALSE;
	
	objServer->Enabled = xvoTableGetBool(objTable, "enabled", 7);
	sClass = xvoTableGetText(objTable, "class", 5);
	objServer->ClassName = XS_CopyText(sClass);
	objServer->Class = XS_ParseServerClass(sClass);
	objServer->HostAware = XS_IsHostAwareClass(objServer->Class);
	objServer->Name = XS_CopyText(xvoTableGetText(objTable, "name", 4));
	objServer->Desc = XS_CopyText(xvoTableGetText(objTable, "desc", 4));
	objServer->Param = XS_CopyText(xvoTableGetText(objTable, "param", 5));
	objVal = xvoTableGetValue(objTable, "backlog", 7);
	if ( objVal && objVal->Type != XVO_DT_NULL ) {
		int64 iBacklog = xvoTableGetInt(objTable, "backlog", 7);
		if ( iBacklog <= 0 || iBacklog > (int64)UINT32_MAX ) {
			XS_ReportError("%s field out of range: backlog", sScope);
			return FALSE;
		}
		objServer->Backlog = (uint32)iBacklog;
	}
	objVal = xvoTableGetValue(objTable, "conn_limit", 10);
	if ( objVal && objVal->Type != XVO_DT_NULL ) {
		int64 iConnLimit = xvoTableGetInt(objTable, "conn_limit", 10);
		if ( iConnLimit < 0 || iConnLimit > (int64)UINT32_MAX ) {
			XS_ReportError("%s field out of range: conn_limit", sScope);
			return FALSE;
		}
		objServer->ConnLimit = (uint32)iConnLimit;
	}
	objVal = xvoTableGetValue(objTable, "recv_limit", 10);
	if ( objVal && objVal->Type != XVO_DT_NULL ) {
		int64 iRecvLimit = xvoTableGetInt(objTable, "recv_limit", 10);
		if ( iRecvLimit <= 0 || iRecvLimit > (int64)UINT32_MAX ) {
			XS_ReportError("%s field out of range: recv_limit", sScope);
			return FALSE;
		}
		objServer->RecvLimit = (uint32)iRecvLimit;
	}
	objVal = xvoTableGetValue(objTable, "idle_timeout", 12);
	if ( objVal && objVal->Type != XVO_DT_NULL ) {
		int64 iIdleTimeout = xvoTableGetInt(objTable, "idle_timeout", 12);
		if ( iIdleTimeout < 0 || iIdleTimeout > (int64)UINT32_MAX ) {
			XS_ReportError("%s field out of range: idle_timeout", sScope);
			return FALSE;
		}
		objServer->IdleTimeout = (uint32)iIdleTimeout;
	}
	objVal = xvoTableGetValue(objTable, "ws_message_limit", 16);
	if ( objVal && objVal->Type != XVO_DT_NULL ) {
		int64 iWsMessageLimit = xvoTableGetInt(objTable, "ws_message_limit", 16);
		if ( iWsMessageLimit <= 0 || iWsMessageLimit > (int64)UINT32_MAX ) {
			XS_ReportError("%s field out of range: ws_message_limit", sScope);
			return FALSE;
		}
		objServer->WsMessageLimit = (uint32)iWsMessageLimit;
	}
	objServer->WsProtocol = XS_CopyText(xvoTableGetText(objTable, "ws_protocol", 11));
	objServer->EnableTLS = xvoTableGetBool(objTable, "tls", 3);
	objServer->Debug = xvoTableGetBool(objTable, "debug", 5);
	objServer->Path = XS_NormalizePath(sBaseDir, xvoTableGetText(objTable, "path", 4));
	objServer->DevMode = XS_ParseDevMode(xvoTableGetText(objTable, "devlang", 7), XS_DEV_PROTOCOL);
	objServer->DevFile = XS_NormalizePath(sBaseDir, xvoTableGetText(objTable, "devfile", 7));
	XS_LoadTlsConfig(objTable, sBaseDir, &objServer->TlsConfig);
	
	if ( objServer->Class == XS_SVC_NONE ) {
		XS_ReportError("%s class invalid: %s", sScope, sClass ? sClass : "(null)");
		return FALSE;
	}

	if ( objServer->Class == XS_SVC_HTTP ) {
		objVal = xvoTableGetValue(objTable, "path_limit", 10);
		if ( objVal && objVal->Type != XVO_DT_NULL ) {
			int64 iPathLimit = xvoTableGetInt(objTable, "path_limit", 10);
			if ( iPathLimit <= 0 || iPathLimit > (int64)(XHTTPD_PATH_CAP + XHTTPD_QUERY_CAP) ) {
				XS_ReportError("%s field out of range: path_limit", sScope);
				return FALSE;
			}
			objServer->PathLimit = (uint32)iPathLimit;
		}
		objVal = xvoTableGetValue(objTable, "header_limit", 12);
		if ( objVal && objVal->Type != XVO_DT_NULL ) {
			int64 iHeaderLimit = xvoTableGetInt(objTable, "header_limit", 12);
			if ( iHeaderLimit <= 0 || iHeaderLimit > (int64)XHTTPD_MAX_HEADERS ) {
				XS_ReportError("%s field out of range: header_limit", sScope);
				return FALSE;
			}
			objServer->HeaderLimit = (uint32)iHeaderLimit;
		}
		objVal = xvoTableGetValue(objTable, "body_limit", 10);
		if ( objVal && objVal->Type != XVO_DT_NULL ) {
			int64 iBodyLimit = xvoTableGetInt(objTable, "body_limit", 10);
			if ( iBodyLimit <= 0 || iBodyLimit > (int64)UINT32_MAX ) {
				XS_ReportError("%s field out of range: body_limit", sScope);
				return FALSE;
			}
			objServer->BodyLimit = (uint32)iBodyLimit;
		}
		if ( objServer->BodyLimit > objServer->RecvLimit ) {
			XS_ReportError("%s invalid limit combination: body_limit > recv_limit", sScope);
			return FALSE;
		}
	}
	
	if ( !XS_LoadBindConfig(objTable, "ip", 2, "port", 4, &objServer->BindIP, &objServer->BindPort, &objServer->Addr, sScope, TRUE) ) {
		return FALSE;
	}
	if ( objServer->EnableTLS ) {
		if ( !XS_LoadBindConfig(objTable, "ip_tls", 6, "port_tls", 8, &objServer->BindIPTLS, &objServer->BindPortTLS, &objServer->AddrTLS, sScope, FALSE) ) {
			return FALSE;
		}
		if ( objServer->BindIPTLS == NULL && objServer->BindIP ) {
			objServer->BindIPTLS = XS_CopyText(objServer->BindIP);
		}
		if ( objServer->BindPortTLS == 0 ) {
			xvalue objPortTlsVal = xvoTableGetValue(objTable, "port_tls", 8);
			if ( objPortTlsVal && objPortTlsVal->Type == XVO_DT_INT ) {
				int64 iPortTls64 = xvoGetInt(objPortTlsVal);
				if ( iPortTls64 <= 0 || iPortTls64 > 65535 ) {
					XS_ReportError("%s field out of range: port_tls", sScope);
					return FALSE;
				}
				objServer->BindPortTLS = (uint16)iPortTls64;
			}
		}
		if ( objServer->BindPortTLS > 0 && objServer->AddrTLS == NULL ) {
			objServer->AddrTLS = XS_MakeBindText(objServer->BindIPTLS ? objServer->BindIPTLS : objServer->BindIP, objServer->BindPortTLS);
		}
	}
	
	if ( objServer->HostAware ) {
		objHostDef = xvoTableGetValue(objTable, "host_default", 12);
		arrHost = xvoTableGetValue(objTable, "hosts", 5);
		
		if ( objHostDef && objHostDef->Type == XVO_DT_TABLE ) {
			objServer->EnableDefaultHost = TRUE;
			if ( !XS_LoadHostConfig(objHostDef, sBaseDir, &objServer->DefaultHost, "server.host_default") ) {
				return FALSE;
			}
		}
		
		if ( arrHost && arrHost->Type == XVO_DT_ARRAY ) {
			for ( i = 0; i < xvoArrayItemCount(arrHost); i++ ) {
				xvalue objItem = xvoArrayGetValue(arrHost, i);
				uint32 idx = xrtArrayAppend(objServer->Hosts, 1);
				XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, idx);
				char sHostScope[64];
				
				sprintf(sHostScope, "server[%d].hosts[%u]", iIndex, i);
				if ( !XS_LoadHostConfig(objItem, sBaseDir, objHost, sHostScope) ) {
					return FALSE;
				}
			}
		}
	} else {
		if ( objServer->DevMode == XS_DEV_SCRIPT_C && objServer->DevFile == NULL && objServer->Path ) {
			objServer->DevFile = xrtPathJoin(2, objServer->Path, "main.c");
		}
	}
	
	return TRUE;
}

static inline bool XS_LoadConfig(XS_Config* objCfg, const char* sFilePath)
{
	xvalue objRoot;
	uint32 i;
	
	XS_InitConfig(objCfg);
	objCfg->FilePath = XS_CopyText(sFilePath);
	objCfg->BaseDir = xrtPathGetDir((char*)sFilePath, 0);
	objRoot = xrtParseJSON_File((char*)sFilePath);
	objCfg->Root = objRoot;
	
	if ( objRoot == NULL ) {
		XS_ReportError("cannot parse config file: %s", sFilePath);
		return FALSE;
	}
	
	if ( objRoot->Type == XVO_DT_TABLE ) {
		uint32 idx = xrtArrayAppend(objCfg->Servers, 1);
		XS_ServerConfig* objServer = xrtArrayGet_Inline(objCfg->Servers, idx);
		
		if ( !XS_LoadServerConfig(objRoot, objCfg->BaseDir, objServer, 0) ) {
			return FALSE;
		}
	} else if ( objRoot->Type == XVO_DT_ARRAY ) {
		for ( i = 0; i < xvoArrayItemCount(objRoot); i++ ) {
			uint32 idx;
			XS_ServerConfig* objServer;
			
			idx = xrtArrayAppend(objCfg->Servers, 1);
			objServer = xrtArrayGet_Inline(objCfg->Servers, idx);
			if ( !XS_LoadServerConfig(xvoArrayGetValue(objRoot, i), objCfg->BaseDir, objServer, i) ) {
				return FALSE;
			}
		}
	} else {
		XS_ReportError("config root must be object or array");
		return FALSE;
	}
	
	return !XS_HasErrors();
}

static inline void XS_PrintConfigSummary(XS_Config* objCfg)
{
	uint32 i;
	
	XS_LogInfo("config file : %s", objCfg->FilePath ? objCfg->FilePath : "(null)");
	XS_LogInfo("config base : %s", objCfg->BaseDir ? objCfg->BaseDir : "(null)");
	XS_LogInfo("server count : %u", objCfg->Servers ? objCfg->Servers->Count : 0);
	
	if ( objCfg->Servers == NULL ) {
		return;
	}
	
	for ( i = 1; i <= objCfg->Servers->Count; i++ ) {
		XS_ServerConfig* objServer = xrtArrayGet_Inline(objCfg->Servers, i);
		
		XS_LogInfo(
			"server[%u] class=%s host_aware=%s name=%s dev=%s",
			i - 1,
			XS_ServerClassName(objServer->Class),
			objServer->HostAware ? "true" : "false",
			objServer->Name ? objServer->Name : "(null)",
			XS_DevModeName(objServer->DevMode)
		);
		
		if ( objServer->HostAware ) {
			XS_LogInfo(
				"server[%u] default_host=%s host_count=%u",
				i - 1,
				objServer->EnableDefaultHost ? "true" : "false",
				objServer->Hosts ? objServer->Hosts->Count : 0
			);
		} else {
			XS_LogInfo(
				"server[%u] path=%s devfile=%s",
				i - 1,
				objServer->Path ? objServer->Path : "(null)",
				objServer->DevFile ? objServer->DevFile : "(null)"
			);
		}
	}
}

#endif
