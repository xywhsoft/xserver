#ifndef XS_CORE_CONFIG_H
#define XS_CORE_CONFIG_H

/*
 * xs3 配置装载与数据模型实现
 * 设计依据：docs/设计.md §4（数据模型）§5（配置文件）
 *
 * 规则：
 *   - 预设字段弹出进结构体（严格类型，不符即失败并指名字段）
 *   - 对象剩余部分整体留作 Custom（同一 xvalue，零深拷贝）
 *   - 结构键（services / host_default / hosts）以 xrtValueObjectTake 移交，
 *     子树由 XS_App.Taken 持有 —— 注意 xrtValueObjectRemove 会销毁子树，
 *     只能用于已拷贝完的标量键
 *   - Backlog/RecvLimit 为 0 表示装配时用 xrt 默认值
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"

/* 宿主侧应用状态（契约结构体之外的装配信息，不进 ABI） */
typedef struct XS_App {
	char			ParseError[512];	/* 装载失败原因 */
	xvalue*			Root;			/* xs.json 根对象（弹掉 services 后即根 Custom） */
	uint32			EngineWorkers;		/* 0 = xrt 默认 */
	uint32			ServerCount;
	XS_ServerInfo**		Servers;
	xnetengine*		Engine;			/* 引擎装配后由 engine.h 回填 */
	uint32			TakenCount;		/* 持有的独立 xvalue 子树（services/hosts/...） */
	xvalue**			Taken;
} XS_App;

/* ============================================================
 * 内部：预设标量字段提取（严格类型；标量键 Remove 即弃）
 * 注意：XRT_STR_LITERAL 仅可用于字符串字面量（sizeof 求长），
 * 运行时键名必须走 strlen 构造视图
 * ============================================================ */

static xstrview XS_ConfigKey(const char* sKey)
{
	return xrtStrViewN(sKey, strlen(sKey));
}

static bool XS_ConfigTakeBool(xvalue* pObj, const char* sKey, bool* pOut, char* sErr, size_t iErrCap)
{
	xvalue* pVal = xrtValueObjectGet(pObj, XS_ConfigKey(sKey));

	/* 缺省值由调用方预设（如 Enabled=true / TLS=false），此处不写 */
	if ( pVal == NULL ) {
		return true;
	}
	if ( !xrtValueGetBool(pVal, pOut) ) {
		snprintf(sErr, iErrCap, "field '%s' expect bool", sKey);
		return false;
	}
	(void)xrtValueObjectRemove(pObj, XS_ConfigKey(sKey));
	return true;
}

static bool XS_ConfigTakeInt(xvalue* pObj, const char* sKey, int64* pOut, char* sErr, size_t iErrCap)
{
	xvalue* pVal = xrtValueObjectGet(pObj, XS_ConfigKey(sKey));

	*pOut = 0;		/* 缺省值 */
	if ( pVal == NULL ) {
		return true;
	}
	if ( !xrtValueGetInt(pVal, pOut) ) {
		snprintf(sErr, iErrCap, "field '%s' expect int", sKey);
		return false;
	}
	(void)xrtValueObjectRemove(pObj, XS_ConfigKey(sKey));
	return true;
}

static bool XS_ConfigTakeString(xvalue* pObj, const char* sKey, const char** pOut, char* sErr, size_t iErrCap)
{
	xvalue* pVal = xrtValueObjectGet(pObj, XS_ConfigKey(sKey));
	xstrview tView;

	*pOut = NULL;		/* 缺省值 */
	if ( pVal == NULL ) {
		return true;
	}
	if ( !xrtValueGetString(pVal, &tView) ) {
		snprintf(sErr, iErrCap, "field '%s' expect string", sKey);
		return false;
	}
	*pOut = xrtStrDupN(tView.Data, tView.Size);
	(void)xrtValueObjectRemove(pObj, XS_ConfigKey(sKey));
	return true;
}

/* 追踪一棵 Take 得到 / 新建的子树，卸载时统一释放 */
static bool XS_ConfigTrack(XS_App* pApp, xvalue* pValue)
{
	xvalue** pNew = (xvalue**)xrtRealloc(pApp->Taken, sizeof(xvalue*) * (size_t)(pApp->TakenCount + 1));

	if ( pNew == NULL ) {
		return false;
	}
	pApp->Taken = pNew;
	pApp->Taken[pApp->TakenCount++] = pValue;
	return true;
}

/* ============================================================
 * 内部：host / server 解析
 * ============================================================ */

static void XS_ConfigHostFree(XS_HostInfo* pHost)
{
	if ( pHost == NULL ) {
		return;
	}
	xrtFree((void*)pHost->Name);
	xrtFree((void*)pHost->Host);
	xrtFree((void*)pHost->Path);
	xrtFree((void*)pHost->DevLang);
	xrtFree((void*)pHost->DevFile);
	xrtFree((void*)pHost->TlsCA);
	xrtFree((void*)pHost->TlsCert);
	xrtFree((void*)pHost->TlsKey);
	xrtFree((void*)pHost->DevInc);
	xrtFree((void*)pHost->DevLib);
	if ( pHost->RuntimeLock != NULL ) {
		xrtMutexDestroy((xmutex*)pHost->RuntimeLock);
		pHost->RuntimeLock = NULL;
	}
	xrtFree(pHost);
}

/* pObj 为该 host 的配置对象（预设键弹出后剩余部分即 Custom） */
static bool XS_ConfigParseHost(xvalue* pObj, XS_HostInfo* pHost, XS_ServerInfo* pServer, char* sErr, size_t iErrCap)
{
	pHost->Enabled = true;
	pHost->Server = pServer;
	pHost->State = XS_RUN_STARTING;
	pHost->Custom = pObj;
	pHost->RuntimeLock = xrtMutexCreate();
	if ( pHost->RuntimeLock == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}

	if ( !XS_ConfigTakeBool(pObj, "enabled", &pHost->Enabled, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "name", &pHost->Name, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "host", &pHost->Host, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "path", &pHost->Path, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "devlang", &pHost->DevLang, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "devfile", &pHost->DevFile, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "tls_ca", &pHost->TlsCA, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "tls_cert", &pHost->TlsCert, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "tls_key", &pHost->TlsKey, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "dev_inc", &pHost->DevInc, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "dev_lib", &pHost->DevLib, sErr, iErrCap) ) return false;
	return true;
}

static bool XS_ConfigClassValid(const char* sClass)
{
	return sClass != NULL && (
		strcmp(sClass, "http") == 0 || strcmp(sClass, "ws") == 0 ||
		strcmp(sClass, "tcp") == 0 || strcmp(sClass, "udp") == 0 ||
		strcmp(sClass, "custom") == 0 );
}

static void XS_ConfigServerFree(XS_ServerInfo* pServer)
{
	uint32 i;

	if ( pServer == NULL ) {
		return;
	}
	xrtFree((void*)pServer->Class);
	xrtFree((void*)pServer->Name);
	xrtFree((void*)pServer->IP);
	xrtFree((void*)pServer->IPTLS);
	XS_ConfigHostFree(pServer->DefaultHost);
	for ( i = 0; i < pServer->HostCount; i++ ) {
		XS_ConfigHostFree(pServer->Hosts[i]);
	}
	xrtFree(pServer->Hosts);
	xrtFree(pServer);
}

static bool XS_ConfigParseServer(XS_App* pApp, xvalue* pObj, XS_ServerInfo* pServer, char* sErr, size_t iErrCap)
{
	xvalue* pHostDefault;
	xvalue* pHosts;
	xvalue* pHostObj;
	const char* sSvcDevLang = NULL;
	const char* sSvcDevFile = NULL;
	const char* sSvcPath = NULL;
	const char* sSvcDevInc = NULL;
	const char* sSvcDevLib = NULL;
	int64 iVal;
	uint32 i;

	pServer->Enabled = true;
	pServer->State = XS_RUN_STARTING;
	pServer->Custom = pObj;

	/* 服务级脚本字段：作为 DefaultHost 的缺省来源（host_default 优先覆盖） */
	if ( !XS_ConfigTakeString(pObj, "devlang", &sSvcDevLang, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "devfile", &sSvcDevFile, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "path", &sSvcPath, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "dev_inc", &sSvcDevInc, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "dev_lib", &sSvcDevLib, sErr, iErrCap) ) return false;

	if ( !XS_ConfigTakeBool(pObj, "enabled", &pServer->Enabled, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "class", &pServer->Class, sErr, iErrCap) ) return false;
	if ( !XS_ConfigClassValid(pServer->Class) ) {
		snprintf(sErr, iErrCap, "field 'class' must be http/ws/tcp/udp/custom");
		return false;
	}
	if ( !XS_ConfigTakeString(pObj, "name", &pServer->Name, sErr, iErrCap) ) return false;
	if ( pServer->Name == NULL ) {
		snprintf(sErr, iErrCap, "field 'name' required");
		return false;
	}
	if ( !XS_ConfigTakeString(pObj, "ip", &pServer->IP, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeBool(pObj, "tls", &pServer->TLS, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeInt(pObj, "backlog", &iVal, sErr, iErrCap) ) return false;
	if ( iVal < 0 || iVal > 0x7FFFFFFF ) {
		snprintf(sErr, iErrCap, "field 'backlog' out of range");
		return false;
	}
	pServer->Backlog = (uint32)iVal;
	if ( !XS_ConfigTakeInt(pObj, "recv_limit", &iVal, sErr, iErrCap) ) return false;
	if ( iVal < 0 ) {
		snprintf(sErr, iErrCap, "field 'recv_limit' out of range");
		return false;
	}
	pServer->RecvLimit = (size_t)iVal;

	if ( strcmp(pServer->Class, "custom") != 0 ) {
		if ( !XS_ConfigTakeInt(pObj, "port", &iVal, sErr, iErrCap) ) return false;
		if ( iVal <= 0 || iVal > 65535 ) {
			snprintf(sErr, iErrCap, "field 'port' required (1-65535) for class '%s'", pServer->Class);
			return false;
		}
		pServer->Port = (uint16)iVal;
		if ( pServer->TLS ) {
			if ( strcmp(pServer->Class, "udp") == 0 ) {
				snprintf(sErr, iErrCap, "field 'tls' not supported on udp");
				return false;
			}
			if ( !XS_ConfigTakeInt(pObj, "port_tls", &iVal, sErr, iErrCap) ) return false;
			pServer->PortTLS = (iVal > 0 && iVal <= 65535) ? (uint16)iVal : 443;
			if ( !XS_ConfigTakeString(pObj, "ip_tls", &pServer->IPTLS, sErr, iErrCap) ) return false;
		}
	}

	/* DefaultHost：恒存在。host_default 以 Take 移交后解析 */
	pServer->DefaultHost = (XS_HostInfo*)xrtCalloc(1, sizeof(XS_HostInfo));
	if ( pServer->DefaultHost == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	pHostDefault = xrtValueObjectTake(pObj, XRT_STR_LITERAL("host_default"));
	if ( pHostDefault != NULL ) {
		if ( !XS_ConfigParseHost(pHostDefault, pServer->DefaultHost, pServer, sErr, iErrCap) ) return false;
		if ( !XS_ConfigTrack(pApp, pHostDefault) ) {
			snprintf(sErr, iErrCap, "out of memory");
			return false;
		}
	} else {
		/* 合成 DefaultHost：基础字段在此补齐（XS_ConfigParseHost 未经过） */
		pServer->DefaultHost->Enabled = true;
		pServer->DefaultHost->Server = pServer;
		pServer->DefaultHost->State = XS_RUN_STARTING;
		pServer->DefaultHost->Custom = xrtValueObject();	/* 空 Custom，恒为对象 */
		pServer->DefaultHost->RuntimeLock = xrtMutexCreate();
		if ( pServer->DefaultHost->Custom == NULL || !XS_ConfigTrack(pApp, pServer->DefaultHost->Custom) ) {
			snprintf(sErr, iErrCap, "out of memory");
			return false;
		}
		if ( pServer->DefaultHost->RuntimeLock == NULL ) {
			snprintf(sErr, iErrCap, "out of memory");
			return false;
		}
	}
	/* 服务级字段补缺（host_default 未覆盖处） */
	if ( pServer->DefaultHost->DevLang == NULL ) {
		pServer->DefaultHost->DevLang = sSvcDevLang;
	} else {
		xrtFree((void*)sSvcDevLang);
	}
	if ( pServer->DefaultHost->DevFile == NULL ) {
		pServer->DefaultHost->DevFile = sSvcDevFile;
	} else {
		xrtFree((void*)sSvcDevFile);
	}
	if ( pServer->DefaultHost->Path == NULL ) {
		pServer->DefaultHost->Path = sSvcPath;
	} else {
		xrtFree((void*)sSvcPath);
	}
	if ( pServer->DefaultHost->DevInc == NULL ) {
		pServer->DefaultHost->DevInc = sSvcDevInc;
	} else {
		xrtFree((void*)sSvcDevInc);
	}
	if ( pServer->DefaultHost->DevLib == NULL ) {
		pServer->DefaultHost->DevLib = sSvcDevLib;
	} else {
		xrtFree((void*)sSvcDevLib);
	}
	if ( pServer->DefaultHost->Name == NULL ) {
		pServer->DefaultHost->Name = xrtStrDupN("default", 7);
	}
	if ( pServer->DefaultHost->Name == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}

	/* hosts[]：以 Take 移交后逐个解析 */
	pHosts = xrtValueObjectTake(pObj, XRT_STR_LITERAL("hosts"));
	if ( pHosts != NULL ) {
		if ( xrtValueType(pHosts) != XVALUE_ARRAY ) {
			snprintf(sErr, iErrCap, "field 'hosts' expect array");
			return false;
		}
		pServer->HostCount = (uint32)xrtValueCount(pHosts);
		if ( pServer->HostCount > 0 ) {
			pServer->Hosts = (XS_HostInfo**)xrtCalloc(pServer->HostCount, sizeof(XS_HostInfo*));
			if ( pServer->Hosts == NULL ) {
				snprintf(sErr, iErrCap, "out of memory");
				return false;
			}
			for ( i = 0; i < pServer->HostCount; i++ ) {
				pHostObj = xrtValueArrayGet(pHosts, i);
				if ( pHostObj == NULL || xrtValueType(pHostObj) != XVALUE_OBJECT ) {
					snprintf(sErr, iErrCap, "field 'hosts[%u]' expect object", i);
					return false;
				}
				pServer->Hosts[i] = (XS_HostInfo*)xrtCalloc(1, sizeof(XS_HostInfo));
				if ( pServer->Hosts[i] == NULL ||
				     !XS_ConfigParseHost(pHostObj, pServer->Hosts[i], pServer, sErr, iErrCap) ) {
					snprintf(sErr, iErrCap, "out of memory");
					return false;
				}
			}
		}
		if ( !XS_ConfigTrack(pApp, pHosts) ) {
			snprintf(sErr, iErrCap, "out of memory");
			return false;
		}
	}
	return true;
}

/* ============================================================
 * 装载 / 释放 / 打印
 * ============================================================ */

/* 从已经解析出的根构造独立配置快照；成功后 Root 所有权归 pApp。 */
static bool XS_ConfigBuild(const char* sSource, xvalue* pRoot, XS_App* pApp)
{
	xvalue* pServices;
	xvalue* pEngine;
	xvalue* pServerObj;
	int64 iWorkers = 0;
	uint32 i, j;

	memset(pApp, 0, sizeof(XS_App));
	if ( pRoot == NULL || xrtValueType(pRoot) != XVALUE_OBJECT ) {
		const xerror* pErr = xrtGetError();
		snprintf(pApp->ParseError, sizeof(pApp->ParseError), "parse '%.200s' failed: %.240s",
			sSource != NULL ? sSource : "<memory>",
			pRoot == NULL && pErr != NULL ? xrtErrorMessage(pErr) : "root expect object");
		if ( pRoot != NULL ) xrtValueRelease(pRoot);
		return false;
	}
	pApp->Root = pRoot;

	/* 根级 engine 旋钮：仅消费 engine.workers，其余留在根 Custom */
	pEngine = xrtValueObjectGet(pRoot, XRT_STR_LITERAL("engine"));
	if ( pEngine != NULL && xrtValueType(pEngine) == XVALUE_OBJECT ) {
		if ( !XS_ConfigTakeInt(pEngine, "workers", &iWorkers, pApp->ParseError, sizeof(pApp->ParseError)) ) {
			return false;
		}
		pApp->EngineWorkers = (iWorkers > 0 && iWorkers <= 1024) ? (uint32)iWorkers : 0;
	}

	/* services：以 Take 移交（各 server 的 Custom 指向其中对象） */
	pServices = xrtValueObjectTake(pRoot, XRT_STR_LITERAL("services"));
	if ( pServices == NULL || xrtValueType(pServices) != XVALUE_ARRAY ) {
		snprintf(pApp->ParseError, sizeof(pApp->ParseError), "field 'services' expect array");
		return false;
	}
	if ( !XS_ConfigTrack(pApp, pServices) ) {
		snprintf(pApp->ParseError, sizeof(pApp->ParseError), "out of memory");
		return false;
	}
	pApp->ServerCount = (uint32)xrtValueCount(pServices);
	if ( pApp->ServerCount > 0 ) {
		pApp->Servers = (XS_ServerInfo**)xrtCalloc(pApp->ServerCount, sizeof(XS_ServerInfo*));
		if ( pApp->Servers == NULL ) {
			snprintf(pApp->ParseError, sizeof(pApp->ParseError), "out of memory");
			return false;
		}
		for ( i = 0; i < pApp->ServerCount; i++ ) {
			pServerObj = xrtValueArrayGet(pServices, i);
			if ( pServerObj == NULL || xrtValueType(pServerObj) != XVALUE_OBJECT ) {
				snprintf(pApp->ParseError, sizeof(pApp->ParseError), "field 'services[%u]' expect object", i);
				return false;
			}
			pApp->Servers[i] = (XS_ServerInfo*)xrtCalloc(1, sizeof(XS_ServerInfo));
			if ( pApp->Servers[i] == NULL ||
			     !XS_ConfigParseServer(pApp, pServerObj, pApp->Servers[i], pApp->ParseError, sizeof(pApp->ParseError)) ) {
				if ( pApp->ParseError[0] == '\0' ) {
					snprintf(pApp->ParseError, sizeof(pApp->ParseError), "out of memory");
				}
				return false;
			}
		}
	}
	/* server 名唯一性 */
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		for ( j = i + 1; j < pApp->ServerCount; j++ ) {
			if ( strcmp(pApp->Servers[i]->Name, pApp->Servers[j]->Name) == 0 ) {
				snprintf(pApp->ParseError, sizeof(pApp->ParseError), "duplicate server name '%s'", pApp->Servers[i]->Name);
				return false;
			}
		}
	}
	return true;
}

static bool XS_ConfigLoad(const char* sPath, XS_App* pApp)
{
	if ( sPath == NULL || pApp == NULL ) return false;
	return XS_ConfigBuild(sPath, xrtJsonParseFile(sPath), pApp);
}

/* reload coordinator 先冻结文件字节，再从同一份内存反复构建候选，避免一次
 * reconcile 内不同 server 观察到不同的 xs.json 内容。 */
static bool XS_ConfigLoadMemory(
	const char* sSource,
	const void* pData,
	size_t iSize,
	XS_App* pApp)
{
	xstrview tText;

	if ( pData == NULL || pApp == NULL ) return false;
	tText = xrtStrViewN((const char*)pData, iSize);
	return XS_ConfigBuild(sSource, xrtJsonParse(tText), pApp);
}

static void XS_ConfigFree(XS_App* pApp)
{
	uint32 i;

	if ( pApp == NULL ) {
		return;
	}
	for ( i = 0; i < pApp->ServerCount; i++ ) {
		XS_ServerInfo* pServer = pApp->Servers[i];

		/* 动态重载后的 server 由其配置快照拥有；根 XS_App 只借用槽位。 */
		if ( pServer != NULL &&
		     (pServer->ConfigOwner == NULL || pServer->ConfigOwner == (void*)pApp) ) {
			XS_ConfigServerFree(pServer);
		}
	}
	xrtFree(pApp->Servers);
	for ( i = 0; i < pApp->TakenCount; i++ ) {
		xrtValueRelease(pApp->Taken[i]);
	}
	xrtFree(pApp->Taken);
	if ( pApp->Root ) {
		xrtValueRelease(pApp->Root);
	}
	memset(pApp, 0, sizeof(XS_App));
}

/* 空对象序列化为 "{}"，非空才打印 */
static void XS_ConfigPrintCustom(const char* sIndent, xvalue* pCustom)
{
	str sJson = xrtJsonStringify(pCustom, false, NULL);

	if ( sJson != NULL && sJson[1] != '}' ) {
		printf("[xs] %s%s\n", sIndent, sJson);
	}
	xrtFree(sJson);
}

static void XS_ConfigDump(const XS_App* pApp)
{
	uint32 i, j;
	XS_ServerInfo* pServer;

	for ( i = 0; i < pApp->ServerCount; i++ ) {
		pServer = pApp->Servers[i];
		printf("[xs] server '%s' class=%s %s:%u tls=%s %s\n",
			pServer->Name, pServer->Class,
			pServer->IP ? pServer->IP : "0.0.0.0", pServer->Port,
			pServer->TLS ? "on" : "off",
			pServer->Enabled ? "enabled" : "disabled");
		XS_ConfigPrintCustom("custom:", pServer->Custom);
		printf("[xs]   default-host '%s' path=%s devfile=%s\n",
			pServer->DefaultHost->Name,
			pServer->DefaultHost->Path ? pServer->DefaultHost->Path : "-",
			pServer->DefaultHost->DevFile ? pServer->DefaultHost->DevFile : "-");
		for ( j = 0; j < pServer->HostCount; j++ ) {
			printf("[xs]   host '%s' bind=%s path=%s devfile=%s\n",
				pServer->Hosts[j]->Name,
				pServer->Hosts[j]->Host ? pServer->Hosts[j]->Host : "-",
				pServer->Hosts[j]->Path ? pServer->Hosts[j]->Path : "-",
				pServer->Hosts[j]->DevFile ? pServer->Hosts[j]->DevFile : "-");
		}
	}
	XS_ConfigPrintCustom("root custom:", pApp->Root);
}

#endif
