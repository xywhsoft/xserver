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

/* 一次 reload 解析得到的不可变配置 revision。reload-all 的多个新
 * generation 共享同一棵配置树；最后一个 generation 终态后才整体释放。 */
typedef struct XS_ConfigRevision {
	volatile int32		iReferences;
	XS_App			App;
} XS_ConfigRevision;

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
	str sCopy;

	if ( pVal == NULL ) {
		return true;
	}
	if ( !xrtValueGetString(pVal, &tView) ) {
		snprintf(sErr, iErrCap, "field '%s' expect string", sKey);
		return false;
	}
	sCopy = xrtStrDupN(tView.Data, tView.Size);
	if ( sCopy == NULL ) {
		snprintf(sErr, iErrCap, "out of memory reading field '%s'", sKey);
		return false;
	}
	xrtFree((void*)*pOut);
	*pOut = sCopy;
	(void)xrtValueObjectRemove(pObj, XS_ConfigKey(sKey));
	return true;
}

/* 追踪一棵 Take 得到 / 新建的子树，卸载时统一释放 */
static bool XS_ConfigTrack(XS_App* pApp, xvalue* pValue)
{
	xvalue** pNew;

	if ( pApp->TakenCount == UINT32_MAX ) return false;
	pNew = (xvalue**)xrtRealloc(pApp->Taken,
		sizeof(xvalue*) * ((size_t)pApp->TakenCount + 1u));

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
	int64 iVal;
	bool bHasPortTls;
	uint32 i;

	pServer->Enabled = true;
	pServer->State = XS_RUN_STARTING;
	pServer->Custom = pObj;
	pServer->DefaultHost = (XS_HostInfo*)xrtCalloc(1, sizeof(XS_HostInfo));
	if ( pServer->DefaultHost == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}

	/* 服务级脚本字段：作为 DefaultHost 的缺省来源（host_default 优先覆盖） */
	if ( !XS_ConfigTakeString(pObj, "devlang", &pServer->DefaultHost->DevLang, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "devfile", &pServer->DefaultHost->DevFile, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "path", &pServer->DefaultHost->Path, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "dev_inc", &pServer->DefaultHost->DevInc, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "dev_lib", &pServer->DefaultHost->DevLib, sErr, iErrCap) ) return false;

	if ( !XS_ConfigTakeBool(pObj, "enabled", &pServer->Enabled, sErr, iErrCap) ) return false;
	if ( !XS_ConfigTakeString(pObj, "class", &pServer->Class, sErr, iErrCap) ) return false;
	if ( !XS_ConfigClassValid(pServer->Class) ) {
		snprintf(sErr, iErrCap, "field 'class' must be http/ws/tcp/udp/custom");
		return false;
	}
	if ( !XS_ConfigTakeString(pObj, "name", &pServer->Name, sErr, iErrCap) ) return false;
	if ( pServer->Name == NULL || pServer->Name[0] == '\0' ) {
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
	if ( iVal < 0 || (uint64)iVal > (uint64)SIZE_MAX ) {
		snprintf(sErr, iErrCap, "field 'recv_limit' out of range");
		return false;
	}
	pServer->RecvLimit = (size_t)iVal;
	if ( strcmp(pServer->Class, "udp") == 0 && pServer->RecvLimit > 65535u ) {
		snprintf(sErr, iErrCap, "field 'recv_limit' exceeds UDP datagram limit 65535");
		return false;
	}

	if ( strcmp(pServer->Class, "custom") != 0 ) {
		if ( !XS_ConfigTakeInt(pObj, "port", &iVal, sErr, iErrCap) ) return false;
		if ( iVal <= 0 || iVal > 65535 ) {
			snprintf(sErr, iErrCap, "field 'port' required (1-65535) for class '%s'", pServer->Class);
			return false;
		}
		pServer->Port = (uint16)iVal;
		if ( pServer->TLS ) {
			const char* sPlainIp;
			const char* sTlsIp;

			if ( strcmp(pServer->Class, "udp") == 0 ) {
				snprintf(sErr, iErrCap, "field 'tls' not supported on udp");
				return false;
			}
			bHasPortTls = xrtValueObjectGet(pObj, XRT_STR_LITERAL("port_tls")) != NULL;
			if ( !XS_ConfigTakeInt(pObj, "port_tls", &iVal, sErr, iErrCap) ) return false;
			if ( bHasPortTls && (iVal <= 0 || iVal > 65535) ) {
				snprintf(sErr, iErrCap, "field 'port_tls' out of range (1-65535)");
				return false;
			}
			pServer->PortTLS = bHasPortTls ? (uint16)iVal : 443;
			if ( !XS_ConfigTakeString(pObj, "ip_tls", &pServer->IPTLS, sErr, iErrCap) ) return false;
			sPlainIp = (pServer->IP == NULL || pServer->IP[0] == '\0') ? "0.0.0.0" : pServer->IP;
			sTlsIp = (pServer->IPTLS == NULL || pServer->IPTLS[0] == '\0') ? sPlainIp : pServer->IPTLS;
			if ( pServer->PortTLS == pServer->Port && strcmp(sPlainIp, sTlsIp) == 0 ) {
				snprintf(sErr, iErrCap,
					"plain and tls listeners for server '%s' must use different endpoints",
					pServer->Name);
				return false;
			}
		}
	}

	/* DefaultHost：恒存在。host_default 以 Take 移交后覆盖服务级缺省。 */
	pHostDefault = xrtValueObjectTake(pObj, XRT_STR_LITERAL("host_default"));
	if ( pHostDefault != NULL ) {
		if ( !XS_ConfigTrack(pApp, pHostDefault) ) {
			xrtValueRelease(pHostDefault);
			snprintf(sErr, iErrCap, "out of memory");
			return false;
		}
		if ( xrtValueType(pHostDefault) != XVALUE_OBJECT ) {
			snprintf(sErr, iErrCap, "field 'host_default' expect object");
			return false;
		}
		if ( !XS_ConfigParseHost(pHostDefault, pServer->DefaultHost, pServer, sErr, iErrCap) ) return false;
	} else {
		/* 合成 DefaultHost：基础字段在此补齐（XS_ConfigParseHost 未经过） */
		pServer->DefaultHost->Enabled = true;
		pServer->DefaultHost->Server = pServer;
		pServer->DefaultHost->State = XS_RUN_STARTING;
		pServer->DefaultHost->Custom = xrtValueObject();	/* 空 Custom，恒为对象 */
		pServer->DefaultHost->RuntimeLock = xrtMutexCreate();
		if ( pServer->DefaultHost->Custom == NULL ) {
			snprintf(sErr, iErrCap, "out of memory");
			return false;
		}
		if ( !XS_ConfigTrack(pApp, pServer->DefaultHost->Custom) ) {
			xrtValueRelease(pServer->DefaultHost->Custom);
			pServer->DefaultHost->Custom = NULL;
			snprintf(sErr, iErrCap, "out of memory");
			return false;
		}
		if ( pServer->DefaultHost->RuntimeLock == NULL ) {
			snprintf(sErr, iErrCap, "out of memory");
			return false;
		}
	}
	if ( pServer->DefaultHost->Name == NULL ) {
		pServer->DefaultHost->Name = xrtStrDupN("default", 7);
	}
	if ( pServer->DefaultHost->Name == NULL ) {
		snprintf(sErr, iErrCap, "out of memory");
		return false;
	}
	if ( pServer->DefaultHost->Name[0] == '\0' ) {
		snprintf(sErr, iErrCap, "host_default field 'name' must not be empty");
		return false;
	}

	/* hosts[]：以 Take 移交后逐个解析 */
	pHosts = xrtValueObjectTake(pObj, XRT_STR_LITERAL("hosts"));
	if ( pHosts != NULL ) {
		if ( !XS_ConfigTrack(pApp, pHosts) ) {
			xrtValueRelease(pHosts);
			snprintf(sErr, iErrCap, "out of memory");
			return false;
		}
		if ( xrtValueType(pHosts) != XVALUE_ARRAY ) {
			snprintf(sErr, iErrCap, "field 'hosts' expect array");
			return false;
		}
		if ( xrtValueCount(pHosts) > (size_t)UINT32_MAX - 1u ) {
			snprintf(sErr, iErrCap, "field 'hosts' has too many entries");
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
				if ( pServer->Hosts[i] == NULL ) {
					snprintf(sErr, iErrCap, "out of memory");
					return false;
				}
				if ( !XS_ConfigParseHost(pHostObj, pServer->Hosts[i], pServer, sErr, iErrCap) ) {
					return false;
				}
			}
		}
	}
	/* host name 是 reload/API 键，必须在 server generation 内唯一。 */
	for ( i = 0; i < pServer->HostCount; i++ ) {
		XS_HostInfo* pHost = pServer->Hosts[i];
		uint32 j;

		if ( pHost->Name == NULL || pHost->Name[0] == '\0' ) {
			snprintf(sErr, iErrCap, "field 'hosts[%u].name' required", i);
			return false;
		}
		if ( strcmp(pHost->Name, pServer->DefaultHost->Name) == 0 ) {
			snprintf(sErr, iErrCap, "duplicate host name '%s'", pHost->Name);
			return false;
		}
		for ( j = 0; j < i; j++ ) {
			if ( strcmp(pHost->Name, pServer->Hosts[j]->Name) == 0 ) {
				snprintf(sErr, iErrCap, "duplicate host name '%s'", pHost->Name);
				return false;
			}
		}
		if ( pHost->Enabled &&
		     (strcmp(pServer->Class, "http") == 0 || strcmp(pServer->Class, "ws") == 0) &&
		     (pHost->Host == NULL || pHost->Host[0] == '\0') ) {
			snprintf(sErr, iErrCap,
				"virtual host '%s' requires non-empty field 'host'", pHost->Name);
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
	if ( pEngine != NULL ) {
		if ( xrtValueType(pEngine) != XVALUE_OBJECT ) {
			snprintf(pApp->ParseError, sizeof(pApp->ParseError),
				"field 'engine' expect object");
			return false;
		}
		if ( !XS_ConfigTakeInt(pEngine, "workers", &iWorkers, pApp->ParseError, sizeof(pApp->ParseError)) ) {
			return false;
		}
		if ( iWorkers < 0 || iWorkers > 1024 ) {
			snprintf(pApp->ParseError, sizeof(pApp->ParseError),
				"field 'engine.workers' expect integer in range 0..1024");
			return false;
		}
		pApp->EngineWorkers = (uint32)iWorkers;
	}

	/* services：以 Take 移交（各 server 的 Custom 指向其中对象） */
	pServices = xrtValueObjectTake(pRoot, XRT_STR_LITERAL("services"));
	if ( pServices == NULL ) {
		snprintf(pApp->ParseError, sizeof(pApp->ParseError), "field 'services' expect array");
		return false;
	}
	if ( !XS_ConfigTrack(pApp, pServices) ) {
		xrtValueRelease(pServices);
		snprintf(pApp->ParseError, sizeof(pApp->ParseError), "out of memory");
		return false;
	}
	if ( xrtValueType(pServices) != XVALUE_ARRAY ) {
		snprintf(pApp->ParseError, sizeof(pApp->ParseError), "field 'services' expect array");
		return false;
	}
	if ( xrtValueCount(pServices) > (size_t)UINT32_MAX - 1u ) {
		snprintf(pApp->ParseError, sizeof(pApp->ParseError), "field 'services' has too many entries");
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

/* reload coordinator 先冻结文件字节，并让一个 intent 只解析出一个共享的
 * ConfigRevision，避免同一次 reconcile 内不同 server 观察到不同的 xs.json。 */
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

/* 把局部解析结果转成引用计数 revision。成功后 pFresh 被清空。 */
static XS_ConfigRevision* XS_ConfigRevisionTake(XS_App* pFresh)
{
	XS_ConfigRevision* pRevision;

	if ( pFresh == NULL ) return NULL;
	pRevision = (XS_ConfigRevision*)xrtCalloc(1, sizeof(XS_ConfigRevision));
	if ( pRevision == NULL ) return NULL;
	pRevision->iReferences = 1;
	pRevision->App = *pFresh;
	memset(pFresh, 0, sizeof(*pFresh));
	return pRevision;
}

static bool XS_ConfigRevisionRetain(XS_ConfigRevision* pRevision)
{
	return pRevision != NULL && xrtRefRetain(&pRevision->iReferences) > 0;
}

static void XS_ConfigRevisionRelease(XS_ConfigRevision* pRevision)
{
	uint32 i;

	if ( pRevision == NULL || xrtRefRelease(&pRevision->iReferences) != 0 ) return;
	/* 根 XS_App 会跳过从动态 revision 借入的 server；revision 自己终态时
	 * 则把本 revision 的对象临时标成本 App 所有，再复用统一释放器。 */
	for ( i = 0; i < pRevision->App.ServerCount; i++ ) {
		XS_ServerInfo* pServer = pRevision->App.Servers[i];

		if ( pServer != NULL && pServer->ConfigOwner == pRevision ) {
			pServer->ConfigOwner = &pRevision->App;
		}
	}
	XS_ConfigFree(&pRevision->App);
	xrtFree(pRevision);
}

/* 空对象序列化为 "{}"，非空才打印 */
static void XS_ConfigPrintCustom(const char* sIndent, xvalue* pCustom)
{
	str sJson = xrtJsonStringify(pCustom, false, NULL);

	if ( sJson != NULL && !(sJson[0] == '{' && sJson[1] == '}' && sJson[2] == '\0') ) {
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
