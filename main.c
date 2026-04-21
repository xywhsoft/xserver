#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <signal.h>



// 定义 XRT_IMPLEMENTATION 导入功能实现
#define XRT_IMPLEMENTATION
#define XTE_ENABLE_FILE
#include "lib/xrt.h"
#ifdef XRT_BUILD_CORE
	#undef XRT_BUILD_CORE
#endif
#include "lib/xsmtp.h"
#define LZ4_DISABLE_DEPRECATE_WARNINGS
#include "lib/lz4/lz4.h"
#include "lib/lz4/lz4hc.h"
#include "lib/zstd/zstd.h"
#define Z7_ST
#include "lib/lzma/Alloc.h"
#include "lib/lzma/LzmaEnc.h"
#include "lib/lzma/LzmaDec.h"
#include "lib/lzma/Lzma2Dec.h"
#include "lib/lzma/Lzma2Enc.h"
#include "lib/md4c/src/md4c.h"
#include "lib/md4c/src/md4c-html.h"

#define XPACK_IMPLEMENTATION
#include "lib/xpack.h"

#include "lib/lz4/lz4.c"
#include "lib/lz4/lz4hc.c"
#ifdef DEBUGLOG
	#undef DEBUGLOG
#endif
#ifdef MINMATCH
	#undef MINMATCH
#endif
#include "lib/zstd/zstd.c"
#include "lib/lzma/Alloc.c"
#include "lib/lzma/CpuArch.c"
#include "lib/lzma/LzFind.c"
#include "lib/lzma/LzmaDec.c"
#include "lib/lzma/Lzma2Dec.c"
#include "lib/lzma/LzmaEnc.c"
#include "lib/lzma/Lzma2Enc.c"
#include "lib/md4c/src/entity.c"
#ifdef MAX
	#undef MAX
#endif
#ifdef MIN
	#undef MIN
#endif
#include "lib/md4c/src/md4c.c"
#ifdef ISDIGIT
	#undef ISDIGIT
#endif
#ifdef ISLOWER
	#undef ISLOWER
#endif
#ifdef ISUPPER
	#undef ISUPPER
#endif
#ifdef ISALNUM
	#undef ISALNUM
#endif
#include "lib/md4c/src/md4c-html.c"

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
static void XS_ServerBindTlsCallback(XS_ServerConfig* objServer);

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

static bool XS_TlsBytesEquals(const void* pLeft, size_t iLeftLen, const void* pRight, size_t iRightLen)
{
	if ( pLeft == pRight ) {
		return TRUE;
	}
	if ( pLeft == NULL || pRight == NULL ) {
		return FALSE;
	}
	if ( iLeftLen != iRightLen ) {
		return FALSE;
	}

	return memcmp(pLeft, pRight, iLeftLen) == 0;
}

static bool XS_TlsConfigHasIdentity(const xtlsconfig* pCfg)
{
	if ( pCfg == NULL ) {
		return FALSE;
	}
	if ( pCfg->pCertData && pCfg->iCertDataLen > 0 && pCfg->pKeyData && pCfg->iKeyDataLen > 0 ) {
		return TRUE;
	}
	if ( pCfg->sCertFile && pCfg->sCertFile[0] && pCfg->sKeyFile && pCfg->sKeyFile[0] ) {
		return TRUE;
	}

	return FALSE;
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
	if ( !XS_TlsBytesEquals(pLeft->pCertData, pLeft->iCertDataLen, pRight->pCertData, pRight->iCertDataLen) ) {
		return FALSE;
	}
	if ( !XS_TlsBytesEquals(pLeft->pKeyData, pLeft->iKeyDataLen, pRight->pKeyData, pRight->iKeyDataLen) ) {
		return FALSE;
	}
	if ( !XS_TlsBytesEquals(pLeft->pCaData, pLeft->iCaDataLen, pRight->pCaData, pRight->iCaDataLen) ) {
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

static bool XS_HostArrayReloadIdentityEquals(xarray arrLeft, xarray arrRight)
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

		if ( objHostLeft == NULL || objHostRight == NULL ) {
			return FALSE;
		}
		if ( !XS_TextEquals(objHostLeft->Name, objHostRight->Name) ) {
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

static bool XS_ServerListenerCriticalEquals(const XS_ServerConfig* objLeft, const XS_ServerConfig* objRight)
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
	if ( objLeft->HostAware != objRight->HostAware ) {
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

	switch ( objLeft->Class ) {
		case XS_SVC_HTTP:
			if ( objLeft->Backlog != objRight->Backlog ) {
				return FALSE;
			}
			if ( objLeft->RecvLimit != objRight->RecvLimit ) {
				return FALSE;
			}
			return TRUE;

		case XS_SVC_WS:
			if ( objLeft->Backlog != objRight->Backlog ) {
				return FALSE;
			}
			if ( objLeft->RecvLimit != objRight->RecvLimit ) {
				return FALSE;
			}
			if ( objLeft->WsMessageLimit != objRight->WsMessageLimit ) {
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
			return TRUE;

		case XS_SVC_TCP:
		case XS_SVC_CUSTOM:
			if ( objLeft->Backlog != objRight->Backlog ) {
				return FALSE;
			}
			if ( objLeft->RecvLimit != objRight->RecvLimit ) {
				return FALSE;
			}
			return TRUE;

		case XS_SVC_UDP:
			return TRUE;

		case XS_SVC_XTP:
			if ( objLeft->Backlog != objRight->Backlog ) {
				return FALSE;
			}
			if ( objLeft->RecvLimit != objRight->RecvLimit ) {
				return FALSE;
			}
			return TRUE;

		default:
			return FALSE;
	}
}

static bool XS_ServerConfigCanSoftReload(const XS_ServerConfig* objLeft, const XS_ServerConfig* objRight)
{
	if ( objLeft == NULL || objRight == NULL ) {
		return FALSE;
	}
	if ( objLeft->Enabled != objRight->Enabled ) {
		return FALSE;
	}
	if ( objLeft->Class != objRight->Class ) {
		return FALSE;
	}
	if ( objLeft->HostAware != objRight->HostAware ) {
		return FALSE;
	}

	switch ( objLeft->Class ) {
		case XS_SVC_HTTP:
			return objLeft->HostAware && objRight->HostAware;
		case XS_SVC_WS:
			return FALSE;
		case XS_SVC_TCP:
		case XS_SVC_CUSTOM:
			return !objLeft->HostAware && !objRight->HostAware;
		case XS_SVC_UDP:
			return !objLeft->HostAware && !objRight->HostAware && XS_ServerListenerCriticalEquals(objLeft, objRight);
		case XS_SVC_XTP:
			return !objLeft->HostAware && !objRight->HostAware;
		default:
			return FALSE;
	}
}

static bool XS_NetAddrEquals(const xnetaddr* pLeft, const xnetaddr* pRight)
{
	if ( pLeft == pRight ) {
		return TRUE;
	}
	if ( pLeft == NULL || pRight == NULL ) {
		return FALSE;
	}

	return memcmp(pLeft, pRight, sizeof(xnetaddr)) == 0;
}

static bool XS_HttpBuildRuntimeConfig(XS_ServerConfig* objServer, bool bTLS, xhttpdconfig* pCfg)
{
	return XS_HttpBuildRuntimeConfigEx(objServer, bTLS, pCfg);
}

static bool XS_HttpRuntimeConfigEquals(const xhttpdconfig* pLeft, const xhttpdconfig* pRight)
{
	if ( pLeft == pRight ) {
		return TRUE;
	}
	if ( pLeft == NULL || pRight == NULL ) {
		return FALSE;
	}
	if ( !XS_NetAddrEquals(&pLeft->tBindAddr, &pRight->tBindAddr) ) {
		return FALSE;
	}
	if ( pLeft->iFlags != pRight->iFlags ) {
		return FALSE;
	}
	if ( pLeft->iBacklog != pRight->iBacklog ) {
		return FALSE;
	}
	if ( pLeft->iRecvLimit != pRight->iRecvLimit ) {
		return FALSE;
	}
	if ( !XS_TlsConfigEquals(pLeft->pTlsConfig, pRight->pTlsConfig) ) {
		return FALSE;
	}

	return TRUE;
}

static void XS_TlsConfigFreeCachedData(xtlsconfig* pCfg)
{
	if ( pCfg == NULL ) {
		return;
	}

	__xrt_tls_config_lock(pCfg);
	if ( pCfg->pCertData ) {
		xrtFree((void*)pCfg->pCertData);
		pCfg->pCertData = NULL;
		pCfg->iCertDataLen = 0;
	}
	if ( pCfg->pKeyData ) {
		xrtFree((void*)pCfg->pKeyData);
		pCfg->pKeyData = NULL;
		pCfg->iKeyDataLen = 0;
	}
	if ( pCfg->pCaData ) {
		xrtFree((void*)pCfg->pCaData);
		pCfg->pCaData = NULL;
		pCfg->iCaDataLen = 0;
	}
	__xrt_tls_config_unlock(pCfg);
}

static bool XS_TlsConfigCloneBytes(const void* pSrc, size_t iSrcLen, const void** ppDst, size_t* piDstLen)
{
	void* pCopy;

	if ( ppDst == NULL || piDstLen == NULL ) {
		return FALSE;
	}
	*ppDst = NULL;
	*piDstLen = 0;
	if ( pSrc == NULL || iSrcLen == 0 ) {
		return TRUE;
	}

	pCopy = xrtMalloc(iSrcLen);
	if ( pCopy == NULL ) {
		return FALSE;
	}
	memcpy(pCopy, pSrc, iSrcLen);
	*ppDst = pCopy;
	*piDstLen = iSrcLen;
	return TRUE;
}

static bool XS_TlsConfigReadFileData(const char* sPath, const void** ppData, size_t* piLen, const char* sScope, const char* sLabel)
{
	void* pData;
	size_t iLen = 0;

	if ( ppData == NULL || piLen == NULL ) {
		return FALSE;
	}
	*ppData = NULL;
	*piLen = 0;
	if ( sPath == NULL || sPath[0] == '\0' ) {
		return TRUE;
	}

	pData = xrtFileGetAll((str)sPath, &iLen);
	if ( pData == NULL || pData == xCore.sNull ) {
		XS_LogError("tls cache load failed: scope=%s type=%s path=%s", sScope ? sScope : "(null)", sLabel ? sLabel : "(null)", sPath);
		return FALSE;
	}

	*ppData = pData;
	*piLen = iLen;
	return TRUE;
}

static bool XS_TlsConfigPreloadCachedData(xtlsconfig* pCfg, const char* sScope)
{
	const void* pCertData = NULL;
	size_t iCertDataLen = 0;
	const void* pKeyData = NULL;
	size_t iKeyDataLen = 0;
	const void* pCaData = NULL;
	size_t iCaDataLen = 0;
	const void* pOldCertData;
	const void* pOldKeyData;
	const void* pOldCaData;

	if ( pCfg == NULL ) {
		return TRUE;
	}
	if ( !XS_TlsConfigReadFileData(pCfg->sCertFile, &pCertData, &iCertDataLen, sScope, "cert") ) {
		return FALSE;
	}
	if ( !XS_TlsConfigReadFileData(pCfg->sKeyFile, &pKeyData, &iKeyDataLen, sScope, "key") ) {
		if ( pCertData ) xrtFree((void*)pCertData);
		return FALSE;
	}
	if ( !XS_TlsConfigReadFileData(pCfg->sCaFile, &pCaData, &iCaDataLen, sScope, "ca") ) {
		if ( pCertData ) xrtFree((void*)pCertData);
		if ( pKeyData ) xrtFree((void*)pKeyData);
		return FALSE;
	}

	__xrt_tls_config_lock(pCfg);
	pOldCertData = pCfg->pCertData;
	pOldKeyData = pCfg->pKeyData;
	pOldCaData = pCfg->pCaData;
	pCfg->pCertData = pCertData;
	pCfg->iCertDataLen = iCertDataLen;
	pCfg->pKeyData = pKeyData;
	pCfg->iKeyDataLen = iKeyDataLen;
	pCfg->pCaData = pCaData;
	pCfg->iCaDataLen = iCaDataLen;
	__xrt_tls_config_unlock(pCfg);

	if ( pOldCertData ) xrtFree((void*)pOldCertData);
	if ( pOldKeyData ) xrtFree((void*)pOldKeyData);
	if ( pOldCaData ) xrtFree((void*)pOldCaData);
	return TRUE;
}

static void XS_ClearServerTlsCaches(XS_ServerConfig* objServer)
{
	uint32 i;

	if ( objServer == NULL ) {
		return;
	}

	XS_TlsConfigFreeCachedData(&objServer->TlsConfig);
	XS_TlsConfigFreeCachedData(&objServer->DefaultHost.TlsConfig);
	for ( i = 1; objServer->Hosts && i <= objServer->Hosts->Count; i++ ) {
		XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
		if ( objHost ) {
			XS_TlsConfigFreeCachedData(&objHost->TlsConfig);
		}
	}

	XS_ServerBindTlsCallback(objServer);
}

static bool XS_TlsHostEqualsToken(const char* sHostValue, const char* sToken)
{
	size_t iHostLen;
	size_t iTokenLen;
	size_t i;

	if ( sHostValue == NULL || sToken == NULL ) {
		return FALSE;
	}
	iHostLen = strlen(sHostValue);
	iTokenLen = strlen(sToken);
	if ( iHostLen == 0 || iTokenLen == 0 || iHostLen != iTokenLen ) {
		return FALSE;
	}
	for ( i = 0; i < iHostLen; i++ ) {
		if ( tolower((unsigned char)sHostValue[i]) != tolower((unsigned char)sToken[i]) ) {
			return FALSE;
		}
	}

	return TRUE;
}

static XS_HostConfig* XS_FindServerHostBySNI(XS_ServerConfig* objServer, const char* sHostName)
{
	uint32 i;

	if ( objServer == NULL || sHostName == NULL || sHostName[0] == '\0' || objServer->Hosts == NULL ) {
		return NULL;
	}

	for ( i = 1; i <= objServer->Hosts->Count; i++ ) {
		XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
		char* sHosts;
		char* sCursor;

		if ( objHost == NULL || !objHost->Enabled || objHost->Host == NULL || objHost->Host[0] == '\0' ) {
			continue;
		}

		sHosts = xrtCopyStr(objHost->Host, 0);
		if ( sHosts == NULL ) {
			continue;
		}

		sCursor = strtok(sHosts, ";");
		while ( sCursor ) {
			while ( *sCursor == ' ' || *sCursor == '\t' ) {
				sCursor++;
			}
			if ( XS_TlsHostEqualsToken(sHostName, sCursor) ) {
				xrtFree(sHosts);
				return objHost;
			}
			sCursor = strtok(NULL, ";");
		}

		xrtFree(sHosts);
	}

	return NULL;
}

static const xtlsconfig* XS_ServerResolveTlsBySNI(XS_ServerConfig* objServer, const char* sHostName)
{
	XS_HostConfig* objHost;

	if ( objServer == NULL ) {
		return NULL;
	}
	if ( objServer->HostAware && sHostName && sHostName[0] ) {
		objHost = XS_FindServerHostBySNI(objServer, sHostName);
		if ( objHost && XS_TlsConfigHasIdentity(&objHost->TlsConfig) ) {
			return &objHost->TlsConfig;
		}
		if ( objServer->EnableDefaultHost && XS_TlsConfigHasIdentity(&objServer->DefaultHost.TlsConfig) ) {
			return &objServer->DefaultHost.TlsConfig;
		}
	}
	if ( XS_TlsConfigHasIdentity(&objServer->TlsConfig) ) {
		return &objServer->TlsConfig;
	}

	return NULL;
}

static void XS_ServerTlsOnSNI(xtlssession* pSession, const char* sHostName, ptr pUserData)
{
	XS_ServerConfig* objServer = (XS_ServerConfig*)pUserData;
	const xtlsconfig* pCfg = XS_ServerResolveTlsBySNI(objServer, sHostName);

	if ( pSession == NULL || pCfg == NULL ) {
		return;
	}

	__xrt_tls_config_lock(pCfg);
	if ( pCfg->pCertData && pCfg->iCertDataLen > 0 && pCfg->pKeyData && pCfg->iKeyDataLen > 0 ) {
		(void)xrtNetTlsSessionSetCertData(pSession, pCfg->pCertData, pCfg->iCertDataLen, pCfg->pKeyData, pCfg->iKeyDataLen);
	} else if ( pCfg->sCertFile && pCfg->sCertFile[0] && pCfg->sKeyFile && pCfg->sKeyFile[0] ) {
		(void)xrtNetTlsSessionSetCert(pSession, pCfg->sCertFile, pCfg->sKeyFile);
	}
	__xrt_tls_config_unlock(pCfg);
}

static void XS_ServerBindTlsCallback(XS_ServerConfig* objServer)
{
	if ( objServer == NULL ) {
		return;
	}
	if ( objServer->EnableTLS && objServer->HostAware ) {
		objServer->TlsConfig.OnSNI = XS_ServerTlsOnSNI;
		objServer->TlsConfig.pSNIUserData = objServer;
	} else {
		objServer->TlsConfig.OnSNI = NULL;
		objServer->TlsConfig.pSNIUserData = NULL;
	}
}

static bool XS_PreloadServerTlsCaches(XS_ServerConfig* objServer)
{
	uint32 i;
	char sScope[256];

	if ( objServer == NULL ) {
		return TRUE;
	}
	if ( !objServer->EnableTLS ) {
		XS_ClearServerTlsCaches(objServer);
		return TRUE;
	}

	snprintf(sScope, sizeof(sScope), "server:%s", objServer->Name ? objServer->Name : "(null)");
	if ( !XS_TlsConfigPreloadCachedData(&objServer->TlsConfig, sScope) ) {
		return FALSE;
	}
	if ( !objServer->HostAware ) {
		XS_TlsConfigFreeCachedData(&objServer->DefaultHost.TlsConfig);
		for ( i = 1; objServer->Hosts && i <= objServer->Hosts->Count; i++ ) {
			XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
			if ( objHost ) {
				XS_TlsConfigFreeCachedData(&objHost->TlsConfig);
			}
		}
		XS_ServerBindTlsCallback(objServer);
		return TRUE;
	}

	snprintf(sScope, sizeof(sScope), "server:%s default_host", objServer->Name ? objServer->Name : "(null)");
	if ( !XS_TlsConfigPreloadCachedData(&objServer->DefaultHost.TlsConfig, sScope) ) {
		return FALSE;
	}
	for ( i = 1; objServer->Hosts && i <= objServer->Hosts->Count; i++ ) {
		XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
		snprintf(sScope, sizeof(sScope), "server:%s host:%s", objServer->Name ? objServer->Name : "(null)", (objHost && objHost->Name) ? objHost->Name : "(null)");
		if ( objHost && !XS_TlsConfigPreloadCachedData(&objHost->TlsConfig, sScope) ) {
			return FALSE;
		}
	}

	XS_ServerBindTlsCallback(objServer);
	return TRUE;
}

static bool XS_PreloadConfigTlsCaches(XS_Config* objCfg)
{
	uint32 i;

	if ( objCfg == NULL || objCfg->Servers == NULL ) {
		return TRUE;
	}

	for ( i = 1; i <= objCfg->Servers->Count; i++ ) {
		XS_ServerConfig* objServer = xrtArrayGet_Inline(objCfg->Servers, i);
		if ( objServer && !XS_PreloadServerTlsCaches(objServer) ) {
			return FALSE;
		}
	}

	return TRUE;
}

static int XS_ReloadHostTlsCache(XS_ServerConfig* objServer, XS_HostConfig* objHost)
{
	char sScope[256];

	if ( objServer == NULL || objHost == NULL ) {
		return -1;
	}
	if ( !objServer->EnableTLS || !objServer->HostAware ) {
		XS_TlsConfigFreeCachedData(&objHost->TlsConfig);
		return 0;
	}

	snprintf(sScope, sizeof(sScope), "server:%s host:%s", objServer->Name ? objServer->Name : "(null)", objHost->Name ? objHost->Name : "(null)");
	return XS_TlsConfigPreloadCachedData(&objHost->TlsConfig, sScope) ? 0 : -2;
}

static int XS_ReloadServerTlsCache(XS_ServerConfig* objServer)
{
	uint32 i;

	if ( objServer == NULL ) {
		return -1;
	}
	if ( !XS_PreloadServerTlsCaches(objServer) ) {
		return -2;
	}
	if ( !objServer->EnableTLS || !objServer->HostAware ) {
		return 0;
	}
	for ( i = 1; objServer->Hosts && i <= objServer->Hosts->Count; i++ ) {
		XS_HostConfig* objHost = xrtArrayGet_Inline(objServer->Hosts, i);
		if ( objHost && !XS_TlsConfigHasIdentity(&objHost->TlsConfig) && (objHost->TlsConfig.sCertFile || objHost->TlsConfig.sKeyFile || objHost->TlsConfig.sCaFile) ) {
			return -3;
		}
	}

	return 0;
}

static int XS_ReloadAllServerTlsCache(XS_Runtime* objRuntime)
{
	uint32 i;

	if ( objRuntime == NULL || objRuntime->Servers == NULL ) {
		return -1;
	}

	for ( i = 1; i <= objRuntime->Servers->Count; i++ ) {
		XS_ServerConfig* objServer = xrtArrayGet_Inline(objRuntime->Servers, i);
		if ( objServer && XS_ReloadServerTlsCache(objServer) != 0 ) {
			return -2;
		}
	}

	return 0;
}

static bool XS_HttpSoftReloadRestartListener(XS_ServerConfig* objServer)
{
	XS_HttpHandle* objHandle;
	xhttpdserver* pServer;
	xhttpdserver* pServerTLS;
	xhttpdconfig tConfig;
	xhttpdconfig tConfigTLS;
	bool bRestartPlain;
	bool bRestartTLS;
	int64 iClosedConn;
	int64 iRemainConn;

	if ( objServer == NULL ) {
		return FALSE;
	}

	objHandle = (XS_HttpHandle*)objServer->pHandle;
	if ( objHandle == NULL ) {
		return TRUE;
	}
	pServer = objHandle->pServer;
	pServerTLS = objHandle->pServerTLS;
	if ( pServer == NULL ) {
		return FALSE;
	}
	if ( !XS_HttpBuildRuntimeConfig(objServer, FALSE, &tConfig) ) {
		XS_LogError(
			"config reload failed: http soft reload invalid addr: server=%s addr=%s",
			objServer->Name ? objServer->Name : "(null)",
			objServer->Addr ? objServer->Addr : "(null)"
		);
		return FALSE;
	}
	if ( objServer->EnableTLS ) {
		if ( !XS_HttpBuildRuntimeConfig(objServer, TRUE, &tConfigTLS) ) {
			XS_LogError(
				"config reload failed: http tls soft reload invalid addr: server=%s addr=%s",
				objServer->Name ? objServer->Name : "(null)",
				objServer->AddrTLS ? objServer->AddrTLS : "(null)"
			);
			return FALSE;
		}
	}

	objHandle->pOwner = objServer;
	pServer->pUserData = objServer;
	if ( pServerTLS ) {
		pServerTLS->pUserData = objServer;
	}

	bRestartPlain = !XS_HttpRuntimeConfigEquals(&pServer->tConfig, &tConfig);
	if ( objServer->EnableTLS ) {
		bRestartTLS = (pServerTLS == NULL) || (!XS_HttpRuntimeConfigEquals(&pServerTLS->tConfig, &tConfigTLS));
	} else {
		bRestartTLS = (pServerTLS != NULL);
	}
	if ( !bRestartPlain && !bRestartTLS ) {
		return TRUE;
	}

	objHandle->bStopping = TRUE;
	objHandle->bStopThread = TRUE;
	if ( objHandle->hIdleThread ) {
		xrtThreadWait(objHandle->hIdleThread);
		xrtThreadDestroy(objHandle->hIdleThread);
		objHandle->hIdleThread = NULL;
	}

	iClosedConn = XS_HttpCloseTrackedConns(objHandle);
	iRemainConn = XS_HttpWaitTrackedConnDrain(objHandle, 500u);
	if ( iRemainConn > 0 ) {
		XS_HttpAbortTrackedConns(objHandle);
		iRemainConn = XS_HttpWaitTrackedConnDrain(objHandle, 1000u);
	}
	if ( iRemainConn > 0 ) {
		XS_HttpFinalizeTrackedConns(objHandle);
		iRemainConn = XS_HttpTrackedConnCount(objHandle);
	}
	if ( iClosedConn > 0 || iRemainConn > 0 ) {
		XS_HttpRecordStopCleanup(iClosedConn, iRemainConn);
		XS_LogInfo(
			"http soft reload cleanup: server=%s addr=%s closed=%lld remain=%lld",
			objServer->Name ? objServer->Name : "(null)",
			objServer->Addr ? objServer->Addr : "(null)",
			(long long)iClosedConn,
			(long long)iRemainConn
		);
	}

	if ( bRestartPlain ) {
		xrtHttpdStop(pServer);
		pServer->tConfig = tConfig;
		if ( xrtHttpdStart(pServer) != XRT_NET_OK ) {
			XS_LogError(
				"config reload failed: http soft reload start error: server=%s addr=%s",
				objServer->Name ? objServer->Name : "(null)",
				objServer->Addr ? objServer->Addr : "(null)"
			);
			return FALSE;
		}
	}
	if ( objServer->EnableTLS ) {
		if ( pServerTLS == NULL ) {
			xhttpdevents tEvents;

			XS_HttpInitEvents(&tEvents);
			pServerTLS = xrtHttpdCreate(pServer->pEngine, &tConfigTLS, &tEvents, objServer);
			if ( pServerTLS == NULL ) {
				XS_LogError(
					"config reload failed: http tls soft reload create error: server=%s addr=%s",
					objServer->Name ? objServer->Name : "(null)",
					objServer->AddrTLS ? objServer->AddrTLS : "(null)"
				);
				return FALSE;
			}
			objHandle->pServerTLS = pServerTLS;
		} else if ( bRestartTLS ) {
			xrtHttpdStop(pServerTLS);
			pServerTLS->tConfig = tConfigTLS;
		}
		if ( bRestartTLS ) {
			if ( xrtHttpdStart(pServerTLS) != XRT_NET_OK ) {
				XS_LogError(
					"config reload failed: http tls soft reload start error: server=%s addr=%s",
					objServer->Name ? objServer->Name : "(null)",
					objServer->AddrTLS ? objServer->AddrTLS : "(null)"
				);
				if ( objHandle->pServerTLS == pServerTLS && pServerTLS->pListener == NULL ) {
					xrtHttpdDestroy(pServerTLS);
					objHandle->pServerTLS = NULL;
				}
				return FALSE;
			}
		}
	} else if ( pServerTLS ) {
		xrtHttpdDestroy(pServerTLS);
		objHandle->pServerTLS = NULL;
	}

	objHandle->bStopThread = FALSE;
	objHandle->bStopping = FALSE;
	if ( bRestartPlain ) {
		XS_LogInfo(
			"http soft reload listener: server=%s addr=%s bound_port=%u",
			objServer->Name ? objServer->Name : "(null)",
			objServer->Addr ? objServer->Addr : "(null)",
			(unsigned)xrtHttpdBoundPort(pServer)
		);
	}
	if ( objHandle->pServerTLS && bRestartTLS ) {
		XS_LogInfo(
			"http tls soft reload listener: server=%s addr=%s bound_port=%u",
			objServer->Name ? objServer->Name : "(null)",
			objServer->AddrTLS ? objServer->AddrTLS : "(null)",
			(unsigned)xrtHttpdBoundPort(objHandle->pServerTLS)
		);
	}
	return TRUE;
}

static bool XS_WsBuildRuntimeConfig(XS_ServerConfig* objServer, xwsserverconfig* pCfg)
{
	if ( objServer == NULL || pCfg == NULL ) {
		return FALSE;
	}

	xrtWsServerConfigInit(pCfg);
	if ( !XS_BuildBindAddr(objServer, objServer->EnableTLS, &pCfg->tBindAddr) ) {
		return FALSE;
	}
	pCfg->iBacklog = objServer->Backlog;
	pCfg->iRecvLimit = XS_RuntimeGovernEnabled() ? (objServer->WsMessageLimit ? objServer->WsMessageLimit : objServer->RecvLimit) : 0u;
	if ( objServer->EnableTLS ) {
		pCfg->pTlsConfig = &objServer->TlsConfig;
	}
	if ( objServer->WsProtocol && objServer->WsProtocol[0] ) {
		snprintf(pCfg->sProtocol, sizeof(pCfg->sProtocol), "%s", objServer->WsProtocol);
	}
	return TRUE;
}

static bool XS_WsRuntimeConfigEquals(const xwsserverconfig* pLeft, const xwsserverconfig* pRight)
{
	if ( pLeft == pRight ) {
		return TRUE;
	}
	if ( pLeft == NULL || pRight == NULL ) {
		return FALSE;
	}
	if ( !XS_NetAddrEquals(&pLeft->tBindAddr, &pRight->tBindAddr) ) {
		return FALSE;
	}
	if ( pLeft->iFlags != pRight->iFlags ) {
		return FALSE;
	}
	if ( pLeft->iBacklog != pRight->iBacklog ) {
		return FALSE;
	}
	if ( pLeft->iRecvLimit != pRight->iRecvLimit ) {
		return FALSE;
	}
	if ( strcmp(pLeft->sProtocol, pRight->sProtocol) != 0 ) {
		return FALSE;
	}
	if ( !XS_TlsConfigEquals(pLeft->pTlsConfig, pRight->pTlsConfig) ) {
		return FALSE;
	}

	return TRUE;
}

static bool XS_WsSoftReloadRestartListener(XS_ServerConfig* objServer)
{
	XS_WsHandle* objHandle;
	xwsserver* pServer;
	xwsserverconfig tConfig;
	int64 iClosedConn;
	int64 iRemainConn;

	if ( objServer == NULL ) {
		return FALSE;
	}

	objHandle = (XS_WsHandle*)objServer->pHandle;
	if ( objHandle == NULL ) {
		return TRUE;
	}
	pServer = objHandle->pServer;
	if ( pServer == NULL ) {
		return FALSE;
	}
	if ( !XS_WsBuildRuntimeConfig(objServer, &tConfig) ) {
		XS_LogError(
			"config reload failed: ws soft reload invalid addr: server=%s addr=%s",
			objServer->Name ? objServer->Name : "(null)",
			objServer->EnableTLS ? (objServer->AddrTLS ? objServer->AddrTLS : "(null)") : (objServer->Addr ? objServer->Addr : "(null)")
		);
		return FALSE;
	}

	objHandle->pOwner = objServer;
	pServer->pUserData = objServer;
	if ( XS_WsRuntimeConfigEquals(&pServer->tConfig, &tConfig) ) {
		return TRUE;
	}

	objHandle->bStopping = TRUE;
	objHandle->bStopThread = TRUE;
	if ( objHandle->hIdleThread ) {
		xrtThreadWait(objHandle->hIdleThread);
		xrtThreadDestroy(objHandle->hIdleThread);
		objHandle->hIdleThread = NULL;
	}

	iClosedConn = XS_WsCloseTrackedConns(objHandle);
	iRemainConn = XS_WsWaitTrackedConnDrain(objHandle, 500u);
	if ( iRemainConn > 0 ) {
		XS_WsAbortTrackedConns(objHandle);
		iRemainConn = XS_WsWaitTrackedConnDrain(objHandle, 1000u);
	}
	if ( iRemainConn > 0 ) {
		XS_WsFinalizeTrackedConns(objHandle);
		iRemainConn = XS_WsTrackedConnCount(objHandle);
	}
	if ( iClosedConn > 0 || iRemainConn > 0 ) {
		XS_WsRecordStopCleanup(iClosedConn, iRemainConn);
		XS_LogInfo(
			"ws soft reload cleanup: server=%s addr=%s closed=%lld remain=%lld",
			objServer->Name ? objServer->Name : "(null)",
			objServer->EnableTLS ? (objServer->AddrTLS ? objServer->AddrTLS : "(null)") : (objServer->Addr ? objServer->Addr : "(null)"),
			(long long)iClosedConn,
			(long long)iRemainConn
		);
	}

	XS_WsDetachTrackedConns(objHandle);
	xrtWsServerStop(pServer);
	pServer->tConfig = tConfig;
	if ( xrtWsServerStart(pServer) != XRT_NET_OK ) {
		XS_LogError(
			"config reload failed: ws soft reload start error: server=%s addr=%s",
			objServer->Name ? objServer->Name : "(null)",
			objServer->EnableTLS ? (objServer->AddrTLS ? objServer->AddrTLS : "(null)") : (objServer->Addr ? objServer->Addr : "(null)")
		);
		return FALSE;
	}

	objHandle->bStopThread = FALSE;
	objHandle->bStopping = FALSE;
	XS_LogInfo(
		"ws soft reload listener: server=%s addr=%s bound_port=%u tls=%s",
		objServer->Name ? objServer->Name : "(null)",
		objServer->EnableTLS ? (objServer->AddrTLS ? objServer->AddrTLS : "(null)") : (objServer->Addr ? objServer->Addr : "(null)"),
		(unsigned)xrtWsServerBoundPort(pServer),
		objServer->EnableTLS ? "true" : "false"
	);
	return TRUE;
}

static bool XS_HttpSoftReloadSyncRuntime(XS_ServerConfig* objServer)
{
	XS_HttpHandle* objHandle;

	if ( objServer == NULL ) {
		return FALSE;
	}

	objHandle = (XS_HttpHandle*)objServer->pHandle;
	if ( objHandle == NULL ) {
		return TRUE;
	}
	if ( !XS_HttpSoftReloadRestartListener(objServer) ) {
		return FALSE;
	}

	if ( !XS_RuntimeGovernEnabled() || objServer->IdleTimeout == 0u ) {
		if ( objHandle->hIdleThread ) {
			objHandle->bStopThread = TRUE;
			xrtThreadWait(objHandle->hIdleThread);
			xrtThreadDestroy(objHandle->hIdleThread);
			objHandle->hIdleThread = NULL;
			objHandle->bStopThread = FALSE;
		}
		objHandle->bStopping = FALSE;
		return TRUE;
	}

	objHandle->bStopping = FALSE;
	if ( objHandle->hIdleThread == NULL ) {
		objHandle->bStopThread = FALSE;
		objHandle->hIdleThread = xrtThreadCreate(XS_HttpIdleThread, objHandle, 0);
		if ( objHandle->hIdleThread == NULL ) {
			XS_LogError(
				"config reload failed: http idle thread create error: server=%s",
				objServer->Name ? objServer->Name : "(null)"
			);
			return FALSE;
		}
	}

	return TRUE;
}

static bool XS_WsSoftReloadSyncRuntime(XS_ServerConfig* objServer)
{
	XS_WsHandle* objHandle;

	if ( objServer == NULL ) {
		return FALSE;
	}

	objHandle = (XS_WsHandle*)objServer->pHandle;
	if ( objHandle == NULL ) {
		return TRUE;
	}
	if ( !XS_WsSoftReloadRestartListener(objServer) ) {
		return FALSE;
	}

	if ( !XS_RuntimeGovernEnabled() || objServer->IdleTimeout == 0u ) {
		if ( objHandle->hIdleThread ) {
			objHandle->bStopThread = TRUE;
			xrtThreadWait(objHandle->hIdleThread);
			xrtThreadDestroy(objHandle->hIdleThread);
			objHandle->hIdleThread = NULL;
			objHandle->bStopThread = FALSE;
		}
		objHandle->bStopping = FALSE;
		return TRUE;
	}

	objHandle->bStopping = FALSE;
	if ( objHandle->hIdleThread == NULL ) {
		objHandle->bStopThread = FALSE;
		objHandle->hIdleThread = xrtThreadCreate(XS_WsIdleThread, objHandle, 0);
		if ( objHandle->hIdleThread == NULL ) {
			XS_LogError(
				"config reload failed: ws idle thread create error: server=%s",
				objServer->Name ? objServer->Name : "(null)"
			);
			return FALSE;
		}
	}

	return TRUE;
}

static bool XS_CustomSoftReloadRestartListener(XS_ServerConfig* objServer);
static bool XS_XtpSoftReloadSyncTlsListener(XS_Runtime* objRuntime, XS_ServerConfig* objServer);

static bool XS_CustomSoftReloadSyncRuntime(XS_ServerConfig* objServer)
{
	XS_CustomHandle* objHandle;

	if ( objServer == NULL ) {
		return FALSE;
	}

	objHandle = (XS_CustomHandle*)objServer->pHandle;
	if ( objHandle == NULL ) {
		return TRUE;
	}

	objHandle->pServer = objServer;
	if ( !XS_CustomSoftReloadRestartListener(objServer) ) {
		return FALSE;
	}

	if ( !XS_RuntimeGovernEnabled() || objServer->IdleTimeout == 0u ) {
		return TRUE;
	}

	if ( objHandle->hIdleThread == NULL ) {
		objHandle->hIdleThread = xrtThreadCreate(XS_CustomIdleThread, objHandle, 0);
		if ( objHandle->hIdleThread == NULL ) {
			XS_LogError(
				"config reload failed: custom idle thread create error: server=%s",
				objServer->Name ? objServer->Name : "(null)"
			);
			return FALSE;
		}
	}

	return TRUE;
}

static bool XS_CustomBuildRuntimeConfig(XS_ServerConfig* objServer, xnetlistenconfig* pCfg)
{
	if ( objServer == NULL || pCfg == NULL ) {
		return FALSE;
	}

	xrtNetListenConfigInit(pCfg);
	if ( !XS_BuildBindAddr(objServer, FALSE, &pCfg->tBindAddr) ) {
		return FALSE;
	}
	pCfg->iBacklog = objServer->Backlog;
	pCfg->iRecvLimit = XS_RuntimeGovernEnabled() ? objServer->RecvLimit : 0u;
	return TRUE;
}

static bool XS_CustomRuntimeConfigEquals(const xnetlistenconfig* pLeft, const xnetlistenconfig* pRight)
{
	if ( pLeft == pRight ) {
		return TRUE;
	}
	if ( pLeft == NULL || pRight == NULL ) {
		return FALSE;
	}
	if ( !XS_NetAddrEquals(&pLeft->tBindAddr, &pRight->tBindAddr) ) {
		return FALSE;
	}
	if ( pLeft->iFlags != pRight->iFlags ) {
		return FALSE;
	}
	if ( pLeft->iBacklog != pRight->iBacklog ) {
		return FALSE;
	}
	if ( pLeft->iRecvLimit != pRight->iRecvLimit ) {
		return FALSE;
	}

	return TRUE;
}

static bool XS_CustomSoftReloadRestartListener(XS_ServerConfig* objServer)
{
	XS_CustomHandle* objHandle;
	xnetlistener* pListener;
	xnetlistenconfig tConfig;
	int64 iClosedConn;
	int64 iRemainConn;

	if ( objServer == NULL ) {
		return FALSE;
	}

	objHandle = (XS_CustomHandle*)objServer->pHandle;
	if ( objHandle == NULL ) {
		return TRUE;
	}
	pListener = objHandle->pListener;
	if ( pListener == NULL ) {
		return FALSE;
	}
	if ( !XS_CustomBuildRuntimeConfig(objServer, &tConfig) ) {
		XS_LogError(
			"config reload failed: custom soft reload invalid addr: server=%s addr=%s",
			objServer->Name ? objServer->Name : "(null)",
			objServer->Addr ? objServer->Addr : "(null)"
		);
		return FALSE;
	}

	objHandle->pServer = objServer;
	pListener->pUserData = objServer;
	if ( XS_CustomRuntimeConfigEquals(&pListener->tConfig, &tConfig) ) {
		return TRUE;
	}

	objHandle->bStopAccept = TRUE;
	xrtNetListenerStop(pListener);
	if ( objHandle->hAcceptThread ) {
		xrtThreadWait(objHandle->hAcceptThread);
		xrtThreadDestroy(objHandle->hAcceptThread);
		objHandle->hAcceptThread = NULL;
	}
	if ( objHandle->hIdleThread ) {
		xrtThreadWait(objHandle->hIdleThread);
		xrtThreadDestroy(objHandle->hIdleThread);
		objHandle->hIdleThread = NULL;
	}

	iClosedConn = XS_CustomCloseTrackedConns(objHandle);
	iRemainConn = XS_CustomWaitTrackedConnDrain(objHandle, 500u);
	if ( iRemainConn > 0 ) {
		XS_CustomAbortTrackedConns(objHandle);
		iRemainConn = XS_CustomWaitTrackedConnDrain(objHandle, 1000u);
	}
	if ( iClosedConn > 0 || iRemainConn > 0 ) {
		XS_CustomRecordStopCleanup(iClosedConn, iRemainConn);
		XS_LogInfo(
			"custom soft reload cleanup: server=%s addr=%s closed=%lld remain=%lld",
			objServer->Name ? objServer->Name : "(null)",
			objServer->Addr ? objServer->Addr : "(null)",
			(long long)iClosedConn,
			(long long)iRemainConn
		);
	}

	pListener->tConfig = tConfig;
	if ( xrtNetListenerStart(pListener) != XRT_NET_OK ) {
		XS_LogError(
			"config reload failed: custom soft reload start error: server=%s addr=%s",
			objServer->Name ? objServer->Name : "(null)",
			objServer->Addr ? objServer->Addr : "(null)"
		);
		return FALSE;
	}

	objHandle->bStopAccept = FALSE;
	objHandle->hAcceptThread = xrtThreadCreate(XS_CustomAcceptThread, objHandle, 0);
	if ( objHandle->hAcceptThread == NULL ) {
		XS_LogError(
			"config reload failed: custom soft reload accept thread create error: server=%s",
			objServer->Name ? objServer->Name : "(null)"
		);
		xrtNetListenerStop(pListener);
		return FALSE;
	}

	XS_LogInfo(
		"custom soft reload listener: server=%s addr=%s bound_port=%u",
		objServer->Name ? objServer->Name : "(null)",
		objServer->Addr ? objServer->Addr : "(null)",
		(unsigned)pListener->tConfig.tBindAddr.iPort
	);
	return TRUE;
}

static bool XS_UdpSoftReloadSyncRuntime(XS_ServerConfig* objServer)
{
	XS_UdpHandle* objHandle;

	if ( objServer == NULL ) {
		return FALSE;
	}

	objHandle = (XS_UdpHandle*)objServer->pHandle;
	if ( objHandle == NULL ) {
		return TRUE;
	}

	objHandle->pServer = objServer;
	return TRUE;
}

static bool XS_TlsConfigClone(const xtlsconfig* pSrc, xtlsconfig* pDst)
{
	memset(pDst, 0, sizeof(xtlsconfig));
	if ( pSrc == NULL ) {
		return TRUE;
	}

	pDst->sCertFile = XS_CopyText(pSrc->sCertFile);
	if ( pSrc->sCertFile && pDst->sCertFile == NULL ) {
		goto fail;
	}
	pDst->sKeyFile = XS_CopyText(pSrc->sKeyFile);
	if ( pSrc->sKeyFile && pDst->sKeyFile == NULL ) {
		goto fail;
	}
	pDst->sCaFile = XS_CopyText(pSrc->sCaFile);
	if ( pSrc->sCaFile && pDst->sCaFile == NULL ) {
		goto fail;
	}
	if ( !XS_TlsConfigCloneBytes(pSrc->pCertData, pSrc->iCertDataLen, &pDst->pCertData, &pDst->iCertDataLen) ) {
		goto fail;
	}
	if ( !XS_TlsConfigCloneBytes(pSrc->pKeyData, pSrc->iKeyDataLen, &pDst->pKeyData, &pDst->iKeyDataLen) ) {
		goto fail;
	}
	if ( !XS_TlsConfigCloneBytes(pSrc->pCaData, pSrc->iCaDataLen, &pDst->pCaData, &pDst->iCaDataLen) ) {
		goto fail;
	}
	pDst->sHostName = XS_CopyText(pSrc->sHostName);
	if ( pSrc->sHostName && pDst->sHostName == NULL ) {
		goto fail;
	}

	pDst->bVerifyPeer = pSrc->bVerifyPeer;
	pDst->OnSNI = NULL;
	pDst->pSNIUserData = NULL;
	pDst->bAllowTLS12Ed25519 = pSrc->bAllowTLS12Ed25519;
	pDst->iMaxVersion = pSrc->iMaxVersion;
	pDst->pResume = pSrc->pResume;
	return TRUE;

fail:
	if ( pDst->sCertFile ) xrtFree((void*)pDst->sCertFile);
	if ( pDst->sKeyFile ) xrtFree((void*)pDst->sKeyFile);
	if ( pDst->sCaFile ) xrtFree((void*)pDst->sCaFile);
	if ( pDst->sHostName ) xrtFree((void*)pDst->sHostName);
	if ( pDst->pCertData ) xrtFree((void*)pDst->pCertData);
	if ( pDst->pKeyData ) xrtFree((void*)pDst->pKeyData);
	if ( pDst->pCaData ) xrtFree((void*)pDst->pCaData);
	memset(pDst, 0, sizeof(xtlsconfig));
	return FALSE;
}

static bool XS_HttpPageConfigClone(const XS_HttpPageConfig* objSrc, XS_HttpPageConfig* objDst)
{
	XS_InitHttpPageConfig(objDst);
	if ( objSrc == NULL ) {
		return TRUE;
	}

	objDst->DefaultPage = XS_CopyText(objSrc->DefaultPage);
	if ( objSrc->DefaultPage && objDst->DefaultPage == NULL ) {
		goto fail;
	}
	objDst->Page404 = XS_CopyText(objSrc->Page404);
	if ( objSrc->Page404 && objDst->Page404 == NULL ) {
		goto fail;
	}
	objDst->Page403 = XS_CopyText(objSrc->Page403);
	if ( objSrc->Page403 && objDst->Page403 == NULL ) {
		goto fail;
	}
	objDst->Page500 = XS_CopyText(objSrc->Page500);
	if ( objSrc->Page500 && objDst->Page500 == NULL ) {
		goto fail;
	}
	objDst->ErrorPage = XS_CopyText(objSrc->ErrorPage);
	if ( objSrc->ErrorPage && objDst->ErrorPage == NULL ) {
		goto fail;
	}
	return TRUE;

fail:
	XS_FreeHttpPageConfig(objDst);
	return FALSE;
}

static bool XS_HostConfigClone(const XS_HostConfig* objSrc, XS_HostConfig* objDst)
{
	XS_InitHostConfig(objDst);
	if ( objSrc == NULL ) {
		return TRUE;
	}

	objDst->Enabled = objSrc->Enabled;
	objDst->Debug = objSrc->Debug;
	objDst->DevMode = objSrc->DevMode;
	objDst->Name = XS_CopyText(objSrc->Name);
	if ( objSrc->Name && objDst->Name == NULL ) {
		goto fail;
	}
	objDst->Desc = XS_CopyText(objSrc->Desc);
	if ( objSrc->Desc && objDst->Desc == NULL ) {
		goto fail;
	}
	objDst->Host = XS_CopyText(objSrc->Host);
	if ( objSrc->Host && objDst->Host == NULL ) {
		goto fail;
	}
	objDst->Param = XS_CopyText(objSrc->Param);
	if ( objSrc->Param && objDst->Param == NULL ) {
		goto fail;
	}
	objDst->Path = XS_CopyText(objSrc->Path);
	if ( objSrc->Path && objDst->Path == NULL ) {
		goto fail;
	}
	if ( !XS_HttpPageConfigClone(&objSrc->Pages, &objDst->Pages) ) {
		goto fail;
	}
	objDst->DevFile = XS_CopyText(objSrc->DevFile);
	if ( objSrc->DevFile && objDst->DevFile == NULL ) {
		goto fail;
	}
	if ( !XS_TlsConfigClone(&objSrc->TlsConfig, &objDst->TlsConfig) ) {
		goto fail;
	}
	return TRUE;

fail:
	XS_FreeHostConfig(objDst);
	return FALSE;
}

static bool XS_ServerConfigCloneForRuntime(const XS_ServerConfig* objSrc, XS_ServerConfig* objDst)
{
	uint32 i;

	if ( objSrc == NULL || objDst == NULL ) {
		return FALSE;
	}

	XS_InitServerConfig(objDst);
	objDst->Enabled = objSrc->Enabled;
	objDst->Class = objSrc->Class;
	objDst->Backlog = objSrc->Backlog;
	objDst->ConnLimit = objSrc->ConnLimit;
	objDst->RecvLimit = objSrc->RecvLimit;
	objDst->IdleTimeout = objSrc->IdleTimeout;
	objDst->WsMessageLimit = objSrc->WsMessageLimit;
	objDst->PathLimit = objSrc->PathLimit;
	objDst->HeaderLimit = objSrc->HeaderLimit;
	objDst->BodyLimit = objSrc->BodyLimit;
	objDst->BindPort = objSrc->BindPort;
	objDst->EnableTLS = objSrc->EnableTLS;
	objDst->BindPortTLS = objSrc->BindPortTLS;
	objDst->Debug = objSrc->Debug;
	objDst->HostAware = objSrc->HostAware;
	objDst->EnableDefaultHost = objSrc->EnableDefaultHost;
	objDst->DevMode = objSrc->DevMode;

	objDst->ClassName = XS_CopyText(objSrc->ClassName);
	if ( objSrc->ClassName && objDst->ClassName == NULL ) {
		goto fail;
	}
	objDst->Name = XS_CopyText(objSrc->Name);
	if ( objSrc->Name && objDst->Name == NULL ) {
		goto fail;
	}
	objDst->Desc = XS_CopyText(objSrc->Desc);
	if ( objSrc->Desc && objDst->Desc == NULL ) {
		goto fail;
	}
	objDst->Param = XS_CopyText(objSrc->Param);
	if ( objSrc->Param && objDst->Param == NULL ) {
		goto fail;
	}
	objDst->BindIP = XS_CopyText(objSrc->BindIP);
	if ( objSrc->BindIP && objDst->BindIP == NULL ) {
		goto fail;
	}
	objDst->Addr = XS_CopyText(objSrc->Addr);
	if ( objSrc->Addr && objDst->Addr == NULL ) {
		goto fail;
	}
	objDst->WsProtocol = XS_CopyText(objSrc->WsProtocol);
	if ( objSrc->WsProtocol && objDst->WsProtocol == NULL ) {
		goto fail;
	}
	objDst->BindIPTLS = XS_CopyText(objSrc->BindIPTLS);
	if ( objSrc->BindIPTLS && objDst->BindIPTLS == NULL ) {
		goto fail;
	}
	objDst->AddrTLS = XS_CopyText(objSrc->AddrTLS);
	if ( objSrc->AddrTLS && objDst->AddrTLS == NULL ) {
		goto fail;
	}
	objDst->Path = XS_CopyText(objSrc->Path);
	if ( objSrc->Path && objDst->Path == NULL ) {
		goto fail;
	}
	objDst->DevFile = XS_CopyText(objSrc->DevFile);
	if ( objSrc->DevFile && objDst->DevFile == NULL ) {
		goto fail;
	}
	if ( !XS_HttpPageConfigClone(&objSrc->Pages, &objDst->Pages) ) {
		goto fail;
	}
	if ( !XS_TlsConfigClone(&objSrc->TlsConfig, &objDst->TlsConfig) ) {
		goto fail;
	}
	if ( !XS_HostConfigClone(&objSrc->DefaultHost, &objDst->DefaultHost) ) {
		goto fail;
	}

	for ( i = 1; objSrc->Hosts && i <= objSrc->Hosts->Count; i++ ) {
		XS_HostConfig* objSrcHost = xrtArrayGet_Inline(objSrc->Hosts, i);
		uint32 iPos = xrtArrayAppend(objDst->Hosts, 1);
		XS_HostConfig* objDstHost;
		if ( iPos == 0 ) {
			goto fail;
		}
		objDstHost = xrtArrayGet_Inline(objDst->Hosts, iPos);
		if ( !XS_HostConfigClone(objSrcHost, objDstHost) ) {
			goto fail;
		}
	}

	XS_ServerBindTlsCallback(objDst);
	return TRUE;

fail:
	XS_FreeServerConfig(objDst);
	return FALSE;
}

static void XS_XtpSoftReloadDropTlsListener(XS_XtpHandle* objHandle)
{
	xnetlistener* pListenerTLS;
	xthread hAcceptThreadTLS;

	if ( objHandle == NULL ) {
		return;
	}

	pListenerTLS = objHandle->pListenerTLS;
	hAcceptThreadTLS = objHandle->hAcceptThreadTLS;
	objHandle->pListenerTLS = NULL;
	objHandle->hAcceptThreadTLS = NULL;

	if ( pListenerTLS ) {
		xrtNetListenerStop(pListenerTLS);
	}
	if ( hAcceptThreadTLS ) {
		xrtThreadWait(hAcceptThreadTLS);
		xrtThreadDestroy(hAcceptThreadTLS);
	}
	if ( pListenerTLS ) {
		xrtNetListenerDestroy(pListenerTLS);
	}
}

static bool XS_XtpBuildRuntimeConfig(XS_ServerConfig* objServer, bool bTLS, xnetlistenconfig* pCfg)
{
	if ( objServer == NULL || pCfg == NULL ) {
		return FALSE;
	}
	if ( bTLS ) {
		if ( !objServer->EnableTLS ) {
			return FALSE;
		}
		if ( objServer->BindPortTLS == 0 ) {
			return FALSE;
		}
		if ( objServer->TlsConfig.sCertFile == NULL || objServer->TlsConfig.sKeyFile == NULL ) {
			return FALSE;
		}
	}

	xrtNetListenConfigInit(pCfg);
	if ( !XS_BuildBindAddr(objServer, bTLS, &pCfg->tBindAddr) ) {
		return FALSE;
	}
	pCfg->iBacklog = objServer->Backlog;
	pCfg->iRecvLimit = XS_RuntimeGovernEnabled() ? objServer->RecvLimit : 0u;
	if ( bTLS ) {
		pCfg->pTlsConfig = &objServer->TlsConfig;
	}
	return TRUE;
}

static bool XS_XtpRuntimeConfigEquals(const xnetlistenconfig* pLeft, const xnetlistenconfig* pRight)
{
	if ( pLeft == pRight ) {
		return TRUE;
	}
	if ( pLeft == NULL || pRight == NULL ) {
		return FALSE;
	}
	if ( !XS_NetAddrEquals(&pLeft->tBindAddr, &pRight->tBindAddr) ) {
		return FALSE;
	}
	if ( pLeft->iFlags != pRight->iFlags ) {
		return FALSE;
	}
	if ( pLeft->iBacklog != pRight->iBacklog ) {
		return FALSE;
	}
	if ( pLeft->iRecvLimit != pRight->iRecvLimit ) {
		return FALSE;
	}
	if ( !XS_TlsConfigEquals(pLeft->pTlsConfig, pRight->pTlsConfig) ) {
		return FALSE;
	}
	return TRUE;
}

static bool XS_XtpSoftReloadRestartListeners(XS_Runtime* objRuntime, XS_ServerConfig* objServer)
{
	XS_XtpHandle* objHandle;
	xnetlistener* pListener;
	xnetlistenconfig tConfig;
	xnetlistenconfig tConfigTLS;
	bool bRestartPlain;
	bool bRestartTLS;
	int64 iClosedConn;
	int64 iRemainConn;

	if ( objRuntime == NULL || objServer == NULL ) {
		return FALSE;
	}

	objHandle = (XS_XtpHandle*)objServer->pHandle;
	if ( objHandle == NULL ) {
		return TRUE;
	}
	pListener = objHandle->pListener;
	if ( pListener == NULL ) {
		return FALSE;
	}
	if ( !XS_XtpBuildRuntimeConfig(objServer, FALSE, &tConfig) ) {
		XS_LogError(
			"config reload failed: xtp soft reload invalid addr: server=%s addr=%s",
			objServer->Name ? objServer->Name : "(null)",
			objServer->Addr ? objServer->Addr : "(null)"
		);
		return FALSE;
	}
	if ( objServer->EnableTLS ) {
		if ( !XS_XtpBuildRuntimeConfig(objServer, TRUE, &tConfigTLS) ) {
			XS_LogError(
				"config reload failed: xtps soft reload invalid addr: server=%s addr=%s",
				objServer->Name ? objServer->Name : "(null)",
				objServer->AddrTLS ? objServer->AddrTLS : "(null)"
			);
			return FALSE;
		}
	}

	objHandle->pServer = objServer;
	pListener->pUserData = objServer;
	if ( objHandle->pListenerTLS ) {
		objHandle->pListenerTLS->pUserData = objServer;
	}

	bRestartPlain = !XS_XtpRuntimeConfigEquals(&pListener->tConfig, &tConfig);
	if ( objServer->EnableTLS ) {
		bRestartTLS = (objHandle->pListenerTLS == NULL) || (!XS_XtpRuntimeConfigEquals(&objHandle->pListenerTLS->tConfig, &tConfigTLS));
	} else {
		bRestartTLS = (objHandle->pListenerTLS != NULL);
	}
	if ( !bRestartPlain && !bRestartTLS ) {
		return TRUE;
	}

	objHandle->bStopAccept = TRUE;
	xrtNetListenerStop(pListener);
	if ( objHandle->pListenerTLS ) {
		xrtNetListenerStop(objHandle->pListenerTLS);
	}
	if ( objHandle->hAcceptThread ) {
		xrtThreadWait(objHandle->hAcceptThread);
		xrtThreadDestroy(objHandle->hAcceptThread);
		objHandle->hAcceptThread = NULL;
	}
	if ( objHandle->hAcceptThreadTLS ) {
		xrtThreadWait(objHandle->hAcceptThreadTLS);
		xrtThreadDestroy(objHandle->hAcceptThreadTLS);
		objHandle->hAcceptThreadTLS = NULL;
	}
	if ( objHandle->hIdleThread ) {
		xrtThreadWait(objHandle->hIdleThread);
		xrtThreadDestroy(objHandle->hIdleThread);
		objHandle->hIdleThread = NULL;
	}

	iClosedConn = XS_XtpCloseTrackedConns(objHandle);
	iRemainConn = XS_XtpWaitTrackedConnDrain(objHandle, 500u);
	if ( iRemainConn > 0 ) {
		XS_XtpAbortTrackedConns(objHandle);
		iRemainConn = XS_XtpWaitTrackedConnDrain(objHandle, 1000u);
	}
	if ( iClosedConn > 0 || iRemainConn > 0 ) {
		XS_XtpRecordStopCleanup(iClosedConn, iRemainConn);
		XS_LogInfo(
			"xtp soft reload cleanup: server=%s addr=%s closed=%lld remain=%lld",
			objServer->Name ? objServer->Name : "(null)",
			objServer->Addr ? objServer->Addr : "(null)",
			(long long)iClosedConn,
			(long long)iRemainConn
		);
	}

	if ( bRestartPlain ) {
		pListener->tConfig = tConfig;
		if ( xrtNetListenerStart(pListener) != XRT_NET_OK ) {
			XS_LogError(
				"config reload failed: xtp soft reload start listener error: server=%s addr=%s",
				objServer->Name ? objServer->Name : "(null)",
				objServer->Addr ? objServer->Addr : "(null)"
			);
			return FALSE;
		}
	}
	if ( !bRestartPlain ) {
		if ( xrtNetListenerStart(pListener) != XRT_NET_OK ) {
			XS_LogError(
				"config reload failed: xtp soft reload restart listener error: server=%s addr=%s",
				objServer->Name ? objServer->Name : "(null)",
				objServer->Addr ? objServer->Addr : "(null)"
			);
			return FALSE;
		}
	}

	if ( !XS_XtpSoftReloadSyncTlsListener(objRuntime, objServer) ) {
		return FALSE;
	}

	objHandle->bStopAccept = FALSE;
	objHandle->hAcceptThread = xrtThreadCreate(XS_XtpAcceptThread, objHandle, 0);
	if ( objHandle->hAcceptThread == NULL ) {
		XS_LogError(
			"config reload failed: xtp soft reload accept thread create error: server=%s",
			objServer->Name ? objServer->Name : "(null)"
		);
		xrtNetListenerStop(pListener);
		XS_XtpSoftReloadDropTlsListener(objHandle);
		return FALSE;
	}

	if ( bRestartPlain ) {
		XS_LogInfo(
			"xtp soft reload listener: server=%s addr=%s bound_port=%u",
			objServer->Name ? objServer->Name : "(null)",
			objServer->Addr ? objServer->Addr : "(null)",
			(unsigned)pListener->tConfig.tBindAddr.iPort
		);
	}
	if ( bRestartTLS ) {
		XS_LogInfo(
			"xtps soft reload listener: server=%s addr=%s bound_port=%u",
			objServer->Name ? objServer->Name : "(null)",
			objServer->AddrTLS ? objServer->AddrTLS : "(null)",
			(unsigned)(objHandle->pListenerTLS ? objHandle->pListenerTLS->tConfig.tBindAddr.iPort : 0u)
		);
	}
	return TRUE;
}

static bool XS_XtpSoftReloadSyncTlsListener(XS_Runtime* objRuntime, XS_ServerConfig* objServer)
{
	XS_XtpHandle* objHandle;
	xnetlistenconfig tCfg;
	xnetlistener* pListenerTLS;

	if ( objServer == NULL ) {
		return FALSE;
	}

	objHandle = (XS_XtpHandle*)objServer->pHandle;
	if ( objHandle == NULL ) {
		return TRUE;
	}

	if ( !objServer->EnableTLS ) {
		XS_XtpSoftReloadDropTlsListener(objHandle);
		return TRUE;
	}
	if ( objRuntime == NULL || objRuntime->pEngine == NULL ) {
		XS_LogError(
			"config reload failed: xtps soft reload runtime engine missing: server=%s",
			objServer->Name ? objServer->Name : "(null)"
		);
		return FALSE;
	}
	if ( !XS_XtpBuildRuntimeConfig(objServer, TRUE, &tCfg) ) {
		XS_LogError(
			"config reload failed: xtps soft reload invalid config: server=%s addr=%s",
			objServer->Name ? objServer->Name : "(null)",
			objServer->AddrTLS ? objServer->AddrTLS : "(null)"
		);
		return FALSE;
	}

	XS_XtpSoftReloadDropTlsListener(objHandle);
	pListenerTLS = xrtNetListenerCreate(objRuntime->pEngine, &tCfg, XS_XtpListenerEvents(), XS_XtpStreamEvents(), objServer);
	if ( pListenerTLS == NULL ) {
		XS_LogError(
			"config reload failed: xtps soft reload create tls listener error: server=%s",
			objServer->Name ? objServer->Name : "(null)"
		);
		return FALSE;
	}

	objHandle->pListenerTLS = pListenerTLS;
	if ( xrtNetListenerStart(objHandle->pListenerTLS) != XRT_NET_OK ) {
		XS_LogError(
			"config reload failed: xtps soft reload start tls listener error: server=%s",
			objServer->Name ? objServer->Name : "(null)"
		);
		xrtNetListenerDestroy(objHandle->pListenerTLS);
		objHandle->pListenerTLS = NULL;
		return FALSE;
	}

	objHandle->hAcceptThreadTLS = xrtThreadCreate(XS_XtpAcceptThreadTLS, objHandle, 0);
	if ( objHandle->hAcceptThreadTLS == NULL ) {
		XS_LogError(
			"config reload failed: xtps soft reload create tls accept thread error: server=%s",
			objServer->Name ? objServer->Name : "(null)"
		);
		xrtNetListenerStop(objHandle->pListenerTLS);
		xrtNetListenerDestroy(objHandle->pListenerTLS);
		objHandle->pListenerTLS = NULL;
		return FALSE;
	}

	return TRUE;
}

static bool XS_XtpSoftReloadSyncRuntime(XS_Runtime* objRuntime, XS_ServerConfig* objServer)
{
	XS_XtpHandle* objHandle;

	if ( objServer == NULL ) {
		return FALSE;
	}

	objHandle = (XS_XtpHandle*)objServer->pHandle;
	if ( objHandle == NULL ) {
		return TRUE;
	}

	objHandle->pServer = objServer;
	if ( !XS_XtpSoftReloadRestartListeners(objRuntime, objServer) ) {
		return FALSE;
	}

	if ( !XS_RuntimeGovernEnabled() || objServer->IdleTimeout == 0u ) {
		return TRUE;
	}

	if ( objHandle->hIdleThread == NULL ) {
		objHandle->hIdleThread = xrtThreadCreate(XS_XtpIdleThread, objHandle, 0);
		if ( objHandle->hIdleThread == NULL ) {
			XS_LogError(
				"config reload failed: xtp idle thread create error: server=%s",
				objServer->Name ? objServer->Name : "(null)"
			);
			return FALSE;
		}
	}

	return TRUE;
}

static bool XS_ServerSoftReloadSyncRuntime(XS_Runtime* objRuntime, XS_ServerConfig* objServer)
{
	if ( objServer == NULL ) {
		return FALSE;
	}

	switch ( objServer->Class ) {
		case XS_SVC_HTTP:
			return XS_HttpSoftReloadSyncRuntime(objServer);
		case XS_SVC_WS:
			return XS_WsSoftReloadSyncRuntime(objServer);
		case XS_SVC_TCP:
		case XS_SVC_CUSTOM:
			return XS_CustomSoftReloadSyncRuntime(objServer);
		case XS_SVC_UDP:
			return XS_UdpSoftReloadSyncRuntime(objServer);
		case XS_SVC_XTP:
			return XS_XtpSoftReloadSyncRuntime(objRuntime, objServer);
		default:
			return FALSE;
	}
}

static bool XS_ConfigReloadCanApplyServiceAddRemoveOnly(XS_Runtime* objRuntime, XS_Config* objCfgNew, uint32* piAddCount, uint32* piRemoveCount)
{
	uint32 i;
	uint32 iAddCount = 0u;
	uint32 iRemoveCount = 0u;

	if ( piAddCount ) {
		*piAddCount = 0u;
	}
	if ( piRemoveCount ) {
		*piRemoveCount = 0u;
	}
	if ( objRuntime == NULL || objRuntime->Servers == NULL || objCfgNew == NULL || objCfgNew->Servers == NULL ) {
		return FALSE;
	}

	for ( i = 1; i <= objCfgNew->Servers->Count; i++ ) {
		XS_ServerConfig* objServerNew = xrtArrayGet_Inline(objCfgNew->Servers, i);
		XS_ServerConfig* objServerOld = XS_FindRuntimeServerByName(objRuntime, objServerNew->Name);
		if ( objServerOld == NULL ) {
			iAddCount++;
			continue;
		}
		if ( !XS_ServerConfigReloadEquals(objServerOld, objServerNew) ) {
			return FALSE;
		}
	}

	for ( i = 1; i <= objRuntime->Servers->Count; i++ ) {
		XS_ServerConfig* objServerOld = xrtArrayGet_Inline(objRuntime->Servers, i);
		if ( XS_FindConfigServerByName(objCfgNew, objServerOld->Name) == NULL ) {
			iRemoveCount++;
		}
	}

	if ( piAddCount ) {
		*piAddCount = iAddCount;
	}
	if ( piRemoveCount ) {
		*piRemoveCount = iRemoveCount;
	}
	return (iAddCount > 0u) || (iRemoveCount > 0u);
}

static bool XS_TryConfigReloadServiceAddRemoveOnly(XS_Config* objCfg, XS_Runtime* objRuntime, XS_Config* objCfgNew, const XS_ConfigReloadRequest* pReq)
{
	uint32 i;
	uint32 iAddCount = 0u;
	uint32 iRemoveCount = 0u;
	uint32 iOldRuntimeCount = 0u;

	if ( !XS_ConfigReloadCanApplyServiceAddRemoveOnly(objRuntime, objCfgNew, &iAddCount, &iRemoveCount) ) {
		return FALSE;
	}

	iOldRuntimeCount = objRuntime->Servers->Count;
	for ( i = 1; i <= objCfgNew->Servers->Count; i++ ) {
		XS_ServerConfig* objServerNewCfg = xrtArrayGet_Inline(objCfgNew->Servers, i);
		XS_ServerConfig objServerClone;
		uint32 iPos;
		XS_ServerConfig* objServerNewRuntime;

		if ( XS_FindRuntimeServerByName(objRuntime, objServerNewCfg->Name) != NULL ) {
			continue;
		}

		memset(&objServerClone, 0, sizeof(objServerClone));
		if ( !XS_ServerConfigCloneForRuntime(objServerNewCfg, &objServerClone) ) {
			goto rollback;
		}

		iPos = xrtArrayAppend(objRuntime->Servers, 1);
		if ( iPos == 0u ) {
			XS_FreeServerConfig(&objServerClone);
			goto rollback;
		}

		objServerNewRuntime = xrtArrayGet_Inline(objRuntime->Servers, iPos);
		memcpy(objServerNewRuntime, &objServerClone, sizeof(XS_ServerConfig));
		XS_ServerBindTlsCallback(objServerNewRuntime);
		memset(&objServerClone, 0, sizeof(XS_ServerConfig));

		if ( !XS_RuntimeInitOneServer(objRuntime, objServerNewRuntime) ) {
			XS_FreeServerConfig(objServerNewRuntime);
			xrtArrayRemove(objRuntime->Servers, iPos, 1);
			goto rollback;
		}
		if ( !XS_RuntimeStartOneServer(objServerNewRuntime) ) {
			XS_RuntimeStopOneServer(objServerNewRuntime);
			XS_FreeServerConfig(objServerNewRuntime);
			xrtArrayRemove(objRuntime->Servers, iPos, 1);
			goto rollback;
		}
	}

	for ( i = iOldRuntimeCount; i >= 1u; i-- ) {
		XS_ServerConfig* objServerOld = xrtArrayGet_Inline(objRuntime->Servers, i);
		if ( XS_FindConfigServerByName(objCfgNew, objServerOld->Name) != NULL ) {
			if ( i == 1u ) {
				break;
			}
			continue;
		}

		XS_RuntimeStopOneServer(objServerOld);
		XS_FreeServerConfig(objServerOld);
		xrtArrayRemove(objRuntime->Servers, i, 1);

		if ( i == 1u ) {
			break;
		}
	}

	XS_FreeConfig(objCfg);
	*objCfg = *objCfgNew;
	memset(objCfgNew, 0, sizeof(XS_Config));
	XS_UpdateRuntimeStats(objRuntime);
	XS_LogInfo(
		"config reload success: mode=service_add_remove_only added=%u removed=%u",
		(unsigned)iAddCount,
		(unsigned)iRemoveCount
	);
	XS_SetConfigReloadStatus(TRUE, pReq, "config reload success: service add/remove only");
	return TRUE;

rollback:
	while ( objRuntime && objRuntime->Servers && objRuntime->Servers->Count > iOldRuntimeCount ) {
		XS_ServerConfig* objServerRollback = xrtArrayGet_Inline(objRuntime->Servers, objRuntime->Servers->Count);
		XS_RuntimeStopOneServer(objServerRollback);
		XS_FreeServerConfig(objServerRollback);
		xrtArrayRemove(objRuntime->Servers, objRuntime->Servers->Count, 1);
	}

	return FALSE;
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

static bool XS_PerformTargetServerSoftReload(XS_Runtime* objRuntime, XS_ServerConfig* objServerOld, XS_ServerConfig* objServerNewSrc, const XS_ConfigReloadRequest* pReq)
{
	XS_ServerConfig objServerBackup;
	bool bBackupValid;
	bool bOldScriptsStopped;
	bool bNewScriptsStarted;
	ptr pHandleKeep;

	if ( objServerOld == NULL || objServerNewSrc == NULL || pReq == NULL ) {
		XS_SetConfigReloadStatus(FALSE, pReq, "invalid target");
		return FALSE;
	}

	memset(&objServerBackup, 0, sizeof(objServerBackup));
	memcpy(&objServerBackup, objServerOld, sizeof(XS_ServerConfig));
	bBackupValid = TRUE;
	bOldScriptsStopped = FALSE;
	bNewScriptsStarted = FALSE;

	if ( !XS_LoadServerScripts(objServerNewSrc) ) {
		XS_LogError("config reload failed: target server soft reload script load error");
		XS_SetConfigReloadStatus(FALSE, pReq, "target server soft reload script load error");
		return FALSE;
	}

	XS_StopServerScripts(objServerOld);
	bOldScriptsStopped = TRUE;
	pHandleKeep = objServerOld->pHandle;

	memcpy(objServerOld, objServerNewSrc, sizeof(XS_ServerConfig));
	memset(objServerNewSrc, 0, sizeof(XS_ServerConfig));
	objServerOld->pHandle = pHandleKeep;
	XS_ServerBindTlsCallback(objServerOld);

	if ( !XS_InitServerScripts(objServerOld) ) {
		XS_LogError("config reload failed: target server soft reload init error");
		XS_SetConfigReloadStatus(FALSE, pReq, "target server soft reload init error");
		goto rollback;
	}
	if ( !XS_StartServerScripts(objServerOld) ) {
		XS_LogError("config reload failed: target server soft reload start error");
		XS_SetConfigReloadStatus(FALSE, pReq, "target server soft reload start error");
		goto rollback;
	}
	bNewScriptsStarted = TRUE;

	if ( !XS_ServerSoftReloadSyncRuntime(objRuntime, objServerOld) ) {
		XS_LogError("config reload failed: target server soft reload runtime sync error");
		XS_SetConfigReloadStatus(FALSE, pReq, "target server soft reload runtime sync error");
		goto rollback;
	}

	if ( bBackupValid ) {
		XS_UnloadServerScripts(&objServerBackup);
		XS_FreeServerConfig(&objServerBackup);
	}

	XS_LogInfo(
		"config reload success: target server=%s target_host=%s mode=soft",
		pReq->sServerName[0] ? pReq->sServerName : "(null)",
		pReq->sHostName[0] ? pReq->sHostName : "(all)"
	);
	XS_SetConfigReloadStatus(TRUE, pReq, "target server soft reload success");
	return TRUE;

rollback:
	if ( bNewScriptsStarted ) {
		XS_StopServerScripts(objServerOld);
	}
	XS_UnloadServerScripts(objServerOld);
	XS_FreeServerConfig(objServerOld);
	memset(objServerOld, 0, sizeof(XS_ServerConfig));
	memcpy(objServerOld, &objServerBackup, sizeof(XS_ServerConfig));
	memset(&objServerBackup, 0, sizeof(XS_ServerConfig));
	bBackupValid = FALSE;
	XS_ServerBindTlsCallback(objServerOld);

	if ( bOldScriptsStopped ) {
		if ( XS_StartServerScripts(objServerOld) ) {
			if ( !XS_ServerSoftReloadSyncRuntime(objRuntime, objServerOld) ) {
				XS_LogError(
					"config reload rollback failed: old target server runtime sync error: %s",
					objServerOld->Name ? objServerOld->Name : "(null)"
				);
			} else {
				XS_LogWarn(
					"config reload rollback: old target server restored: %s",
					objServerOld->Name ? objServerOld->Name : "(null)"
				);
			}
		} else {
			XS_LogError(
				"config reload rollback failed: old target server restart error: %s",
				objServerOld->Name ? objServerOld->Name : "(null)"
			);
		}
	}

	return FALSE;
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
	if ( !pReq->Force && XS_ServerConfigCanSoftReload(objServerOld, objServerNewSrc) ) {
		return XS_PerformTargetServerSoftReload(objRuntime, objServerOld, objServerNewSrc, pReq);
	}
	
	memset(&objServerBackup, 0, sizeof(objServerBackup));
	memcpy(&objServerBackup, objServerOld, sizeof(XS_ServerConfig));
	bBackupValid = TRUE;
	XS_RuntimeStopOneServer(objServerOld);
	
	memcpy(objServerOld, objServerNewSrc, sizeof(XS_ServerConfig));
	memset(objServerNewSrc, 0, sizeof(XS_ServerConfig));
	XS_ServerBindTlsCallback(objServerOld);
	
	if ( !XS_RuntimeInitOneServer(objRuntime, objServerOld) ) {
		XS_LogError("config reload failed: target server init error");
		XS_SetConfigReloadStatus(FALSE, pReq, "target server init error");
		XS_RuntimeStopOneServer(objServerOld);
		XS_FreeServerConfig(objServerOld);
		memset(objServerOld, 0, sizeof(XS_ServerConfig));
		memcpy(objServerOld, &objServerBackup, sizeof(XS_ServerConfig));
		memset(&objServerBackup, 0, sizeof(XS_ServerConfig));
		bBackupValid = FALSE;
		XS_ServerBindTlsCallback(objServerOld);
		
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
		XS_ServerBindTlsCallback(objServerOld);
		
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
	if ( !XS_PreloadConfigTlsCaches(&objCfgNew) ) {
		XS_LogError("config reload failed: tls cache preload error");
		XS_SetConfigReloadStatus(FALSE, pReq, "tls cache preload error");
		XS_FreeConfig(&objCfgNew);
		return FALSE;
	}
	
	XS_LogInfo("config reload: parsed new config");
	if ( pReq && pReq->sServerName[0] != '\0' ) {
		bool bRet = XS_PerformTargetServerReload(objCfg, objRuntime, &objCfgNew, pReq);
		XS_FreeConfig(&objCfgNew);
		return bRet;
	}
	if ( XS_TryConfigReloadServiceAddRemoveOnly(objCfg, objRuntime, &objCfgNew, pReq) ) {
		return TRUE;
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
	if ( !XS_PreloadConfigTlsCaches(&objCfg) ) {
		XS_LogError("tls cache preload failed");
		iExitCode = 1;
		goto ExitConfig;
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
