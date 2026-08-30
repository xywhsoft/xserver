#ifndef XS_RUNTIME_RELOAD_H
#define XS_RUNTIME_RELOAD_H

/*
 * 热重载期望状态协调器：独立控制线程准备候选，latest-wins 合并同目标意图，
 * 极短提交栅栏内发布；旧代只由终态引用归零回收。
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"
#include "../core/config.h"
#include "../core/engine.h"
#include "../script/script.h"
#include "../protocol/http.h"
#include "../protocol/stream.h"
#include "../protocol/ws.h"
#include "../protocol/udp.h"
#include "../core/driver.h"
#include "gc.h"
#include "topology.h"

typedef enum XS_ReloadKind {
	XS_RELOAD_KIND_HOST = 1,
	XS_RELOAD_KIND_SERVER,
	XS_RELOAD_KIND_ALL
} XS_ReloadKind;

typedef struct XS_ReloadSourceFile {
	struct XS_ReloadSourceFile*	pNext;
	char*				sPath;
	bytes				pData;
	size_t				iSize;
} XS_ReloadSourceFile;

typedef struct XS_ReloadIntent {
	struct XS_ReloadIntent*	pNext;
	XS_ReloadId			iId;
	XS_ReloadKind			iKind;
	char*				sServerName;
	char*				sHostName;
	bytes				pConfigData;
	size_t				iConfigSize;
	XS_ReloadSourceFile*		pSources;
	uint64				iRevision;
	bool				bSuperseded;
	bool				bStaging;	/* 已进入 Swap/Init 副作用区 */
	bool				bCommitted;
	char				sError[256];
} XS_ReloadIntent;

#define XS_RELOAD_PENDING_LIMIT 64u
#define XS_RELOAD_RESULT_CAPACITY 256u

typedef struct XS_ReloadCoordinator {
	xmutex*			pLock;
	xcond*			pCond;
	xthread*		pThread;
	XS_App*			pApp;
	char*			sConfigPath;
	XS_ReloadIntent*	pPendingHead;
	XS_ReloadIntent*	pPendingTail;
	XS_ReloadIntent*	pActive;
	uint32			iPendingCount;
	XS_ReloadId		iNextId;
	bool			bAccepting;
	bool			bStopping;
	bool			bJoined;
	XS_ReloadResult		arrResults[XS_RELOAD_RESULT_CAPACITY];
} XS_ReloadCoordinator;

static XS_ReloadCoordinator g_XS_Reload;
static XRT_THREAD_LOCAL XS_ReloadIntent* g_XS_ReloadCurrent = NULL;

static bool XS_ReloadServerNow(XS_App* pApp, const char* sName);
static bool XS_ReloadAllAtomicNow(XS_App* pApp);

static bool XS_ReloadStateFinal(XS_ReloadState iState)
{
	return iState == XS_RELOAD_SUCCEEDED || iState == XS_RELOAD_FAILED ||
	       iState == XS_RELOAD_SUPERSEDED || iState == XS_RELOAD_CANCELLED;
}

static XS_ReloadResult* XS_ReloadResultFindLocked(XS_ReloadId iId)
{
	XS_ReloadResult* pResult;

	if ( iId == 0 ) return NULL;
	pResult = &g_XS_Reload.arrResults[iId % XS_RELOAD_RESULT_CAPACITY];
	return pResult->Id == iId ? pResult : NULL;
}

/* latest-wins 可在一个慢 active 存在时连续产生很多终态 ticket。顺序 ID
 * 若恰好绕回 active 所在环槽，跳过该 ID，而不是无谓拒绝新的期望状态。 */
static XS_ReloadResult* XS_ReloadResultReserveLocked(XS_ReloadId* piId)
{
	XS_ReloadId iCandidate = g_XS_Reload.iNextId;
	uint32 i;

	for ( i = 0; i < XS_RELOAD_RESULT_CAPACITY; i++ ) {
		XS_ReloadResult* pSlot;

		iCandidate++;
		if ( iCandidate == 0 ) iCandidate++;
		pSlot = &g_XS_Reload.arrResults[iCandidate % XS_RELOAD_RESULT_CAPACITY];
		if ( pSlot->Id == 0 || XS_ReloadStateFinal(pSlot->State) ) {
			g_XS_Reload.iNextId = iCandidate;
			*piId = iCandidate;
			return pSlot;
		}
	}
	return NULL;
}

static void XS_ReloadResultUpdateLocked(
	XS_ReloadIntent* pIntent,
	XS_ReloadState iState,
	const char* sMessage)
{
	XS_ReloadResult* pResult;

	if ( pIntent == NULL ) return;
	pResult = XS_ReloadResultFindLocked(pIntent->iId);
	if ( pResult == NULL ) return;
	pResult->State = iState;
	pResult->Revision = pIntent->iRevision;
	if ( sMessage != NULL ) snprintf(pResult->Message, sizeof(pResult->Message), "%s", sMessage);
}

static void XS_ReloadSourceFree(XS_ReloadSourceFile* pSource)
{
	while ( pSource != NULL ) {
		XS_ReloadSourceFile* pNext = pSource->pNext;

		xrtFree(pSource->sPath);
		xrtFree(pSource->pData);
		xrtFree(pSource);
		pSource = pNext;
	}
}

static void XS_ReloadIntentFree(XS_ReloadIntent* pIntent)
{
	if ( pIntent == NULL ) return;
	xrtFree(pIntent->sServerName);
	xrtFree(pIntent->sHostName);
	xrtFree(pIntent->pConfigData);
	XS_ReloadSourceFree(pIntent->pSources);
	xrtFree(pIntent);
}

static uint64 XS_ReloadHashBytes(uint64 iHash, const void* pData, size_t iSize)
{
	const uint8* pBytes = (const uint8*)pData;
	size_t i;

	if ( iHash == 0 ) iHash = UINT64_C(14695981039346656037);
	for ( i = 0; i < iSize; i++ ) {
		iHash ^= pBytes[i];
		iHash *= UINT64_C(1099511628211);
	}
	return iHash;
}

/* 返回调用方拥有的副本；缓存本身活到本次 reconcile 终态。 */
static bytes XS_ReloadReadSourceSnapshot(const char* sPath, size_t* piSize, void* pContext)
{
	XS_ReloadIntent* pIntent = (XS_ReloadIntent*)pContext;
	XS_ReloadSourceFile* pSource;
	bytes pCopy;

	if ( pIntent == NULL || sPath == NULL || piSize == NULL ) return NULL;
	for ( pSource = pIntent->pSources; pSource != NULL; pSource = pSource->pNext ) {
		if ( strcmp(pSource->sPath, sPath) == 0 ) break;
	}
	if ( pSource == NULL ) {
		pSource = (XS_ReloadSourceFile*)xrtCalloc(1, sizeof(XS_ReloadSourceFile));
		if ( pSource == NULL ) return NULL;
		pSource->sPath = xrtStrDup(sPath);
		pSource->pData = xrtFileReadAll(sPath, &pSource->iSize);
		if ( pSource->sPath == NULL || pSource->pData == NULL ) {
			xrtFree(pSource->sPath);
			xrtFree(pSource->pData);
			xrtFree(pSource);
			return NULL;
		}
		pSource->pNext = pIntent->pSources;
		pIntent->pSources = pSource;
		pIntent->iRevision = XS_ReloadHashBytes(pIntent->iRevision,
			pSource->sPath, strlen(pSource->sPath));
		pIntent->iRevision = XS_ReloadHashBytes(pIntent->iRevision,
			pSource->pData, pSource->iSize);
	}
	pCopy = (bytes)xrtMalloc(pSource->iSize > 0 ? pSource->iSize : 1);
	if ( pCopy == NULL ) return NULL;
	if ( pSource->iSize > 0 ) memcpy(pCopy, pSource->pData, pSource->iSize);
	*piSize = pSource->iSize;
	return pCopy;
}

static void XS_ReloadSetError(const char* sMessage)
{
	if ( g_XS_ReloadCurrent != NULL && sMessage != NULL ) {
		snprintf(g_XS_ReloadCurrent->sError,
			sizeof(g_XS_ReloadCurrent->sError), "%s", sMessage);
	}
}

static bool XS_ReloadLoadConfig(XS_App* pFresh)
{
	if ( g_XS_ReloadCurrent != NULL && g_XS_ReloadCurrent->pConfigData != NULL ) {
		return XS_ConfigLoadMemory(g_XS_Reload.sConfigPath,
			g_XS_ReloadCurrent->pConfigData,
			g_XS_ReloadCurrent->iConfigSize, pFresh);
	}
	return XS_ConfigLoad(g_XS_Reload.sConfigPath, pFresh);
}

/* 与 Submit 共用同一把短锁，给“新意图受理”和“旧候选发布”确定线性顺序。 */
static bool XS_ReloadCommitBegin(void)
{
	XS_ReloadIntent* pIntent = g_XS_ReloadCurrent;

	if ( pIntent == NULL || g_XS_Reload.pLock == NULL ) return false;
	xrtMutexLock(g_XS_Reload.pLock);
	if ( (g_XS_Reload.bStopping && !pIntent->bStaging) ||
	     g_XS_Reload.pActive != pIntent || pIntent->bSuperseded ) {
		xrtMutexUnlock(g_XS_Reload.pLock);
		return false;
	}
	return true;
}

/* 所有可失败的编译、校验和 endpoint 预绑定完成后才越过此线性点。
 * 此后新期望状态排在本 intent 之后，不再取消已经开始产生副作用的 Init。 */
static bool XS_ReloadStageBegin(void)
{
	XS_ReloadIntent* pIntent = g_XS_ReloadCurrent;
	bool bOk = false;

	if ( pIntent == NULL || g_XS_Reload.pLock == NULL ) return false;
	xrtMutexLock(g_XS_Reload.pLock);
	if ( !g_XS_Reload.bStopping && g_XS_Reload.pActive == pIntent &&
	     !pIntent->bSuperseded ) {
		pIntent->bStaging = true;
		bOk = true;
	}
	xrtMutexUnlock(g_XS_Reload.pLock);
	return bOk;
}

static void XS_ReloadCommitEnd(bool bCommitted)
{
	if ( bCommitted && g_XS_ReloadCurrent != NULL ) g_XS_ReloadCurrent->bCommitted = true;
	xrtMutexUnlock(g_XS_Reload.pLock);
}

static bool XS_ReloadIntentDominates(
	const XS_ReloadIntent* pNew,
	const XS_ReloadIntent* pOld)
{
	if ( pNew->iKind == XS_RELOAD_KIND_ALL ) return true;
	if ( pNew->iKind == XS_RELOAD_KIND_SERVER ||
	     pNew->iKind == XS_RELOAD_KIND_HOST ) {
		return pOld->sServerName != NULL &&
		       strcmp(pNew->sServerName, pOld->sServerName) == 0 &&
		       (pOld->iKind == XS_RELOAD_KIND_SERVER || pOld->iKind == XS_RELOAD_KIND_HOST);
	}
	return false;
}

static void XS_ReloadPendingTailRefreshLocked(void)
{
	XS_ReloadIntent* pIntent;

	g_XS_Reload.pPendingTail = NULL;
	for ( pIntent = g_XS_Reload.pPendingHead; pIntent != NULL; pIntent = pIntent->pNext ) {
		g_XS_Reload.pPendingTail = pIntent;
	}
}

static XS_ReloadId XS_ReloadSubmit(
	XS_ReloadKind iKind,
	const char* sServerName,
	const char* sHostName)
{
	XS_ReloadIntent* pIntent;
	XS_ReloadIntent* pRemoved = NULL;
	XS_ReloadIntent** ppLink;
	XS_ReloadResult* pSlot;
	XS_ReloadId iId;
	bool bReplacesPending = false;

	if ( g_XS_Reload.pLock == NULL ||
	     (iKind != XS_RELOAD_KIND_ALL && sServerName == NULL) ||
	     (iKind == XS_RELOAD_KIND_HOST && sHostName == NULL) ) return 0;
	pIntent = (XS_ReloadIntent*)xrtCalloc(1, sizeof(XS_ReloadIntent));
	if ( pIntent == NULL ) return 0;
	pIntent->iKind = iKind;
	if ( sServerName != NULL ) pIntent->sServerName = xrtStrDup(sServerName);
	if ( sHostName != NULL ) pIntent->sHostName = xrtStrDup(sHostName);
	if ( (sServerName != NULL && pIntent->sServerName == NULL) ||
	     (sHostName != NULL && pIntent->sHostName == NULL) ) {
		XS_ReloadIntentFree(pIntent);
		return 0;
	}

	xrtMutexLock(g_XS_Reload.pLock);
	if ( !g_XS_Reload.bAccepting || g_XS_Reload.bStopping ) {
		xrtMutexUnlock(g_XS_Reload.pLock);
		XS_ReloadIntentFree(pIntent);
		return 0;
	}
	for ( XS_ReloadIntent* pOld = g_XS_Reload.pPendingHead;
	      pOld != NULL; pOld = pOld->pNext ) {
		if ( XS_ReloadIntentDominates(pIntent, pOld) ) {
			bReplacesPending = true;
			break;
		}
	}
	if ( g_XS_Reload.iPendingCount >= XS_RELOAD_PENDING_LIMIT && !bReplacesPending ) {
		xrtMutexUnlock(g_XS_Reload.pLock);
		XS_ReloadIntentFree(pIntent);
		return 0;
	}
	pSlot = XS_ReloadResultReserveLocked(&iId);
	if ( pSlot == NULL ) {
		xrtMutexUnlock(g_XS_Reload.pLock);
		XS_ReloadIntentFree(pIntent);
		return 0;
	}
	pIntent->iId = iId;
	memset(pSlot, 0, sizeof(*pSlot));
	pSlot->Id = iId;
	pSlot->State = XS_RELOAD_ACCEPTED;
	if ( iKind == XS_RELOAD_KIND_ALL ) {
		snprintf(pSlot->Target, sizeof(pSlot->Target), "all");
	} else if ( iKind == XS_RELOAD_KIND_SERVER ) {
		snprintf(pSlot->Target, sizeof(pSlot->Target), "server:%s", sServerName);
	} else {
		snprintf(pSlot->Target, sizeof(pSlot->Target), "host:%s/%s", sServerName, sHostName);
	}
	snprintf(pSlot->Message, sizeof(pSlot->Message), "accepted");

	/* 新意图覆盖所有尚未执行且被其作用域包含的旧意图。 */
	for ( ppLink = &g_XS_Reload.pPendingHead; *ppLink != NULL; ) {
		XS_ReloadIntent* pOld = *ppLink;

		if ( XS_ReloadIntentDominates(pIntent, pOld) ) {
			*ppLink = pOld->pNext;
			pOld->pNext = pRemoved;
			pRemoved = pOld;
			g_XS_Reload.iPendingCount--;
			XS_ReloadResultUpdateLocked(pOld, XS_RELOAD_SUPERSEDED,
				"superseded by newer desired state");
		} else {
			ppLink = &pOld->pNext;
		}
	}
	XS_ReloadPendingTailRefreshLocked();
	if ( g_XS_Reload.pActive != NULL && !g_XS_Reload.pActive->bCommitted &&
	     !g_XS_Reload.pActive->bStaging &&
	     XS_ReloadIntentDominates(pIntent, g_XS_Reload.pActive) ) {
		g_XS_Reload.pActive->bSuperseded = true;
	}
	if ( g_XS_Reload.pPendingTail != NULL ) {
		g_XS_Reload.pPendingTail->pNext = pIntent;
	} else {
		g_XS_Reload.pPendingHead = pIntent;
	}
	g_XS_Reload.pPendingTail = pIntent;
	g_XS_Reload.iPendingCount++;
	(void)xrtCondSignal(g_XS_Reload.pCond);
	xrtMutexUnlock(g_XS_Reload.pLock);
	while ( pRemoved != NULL ) {
		XS_ReloadIntent* pNext = pRemoved->pNext;

		XS_ReloadIntentFree(pRemoved);
		pRemoved = pNext;
	}
	return iId;
}

static bool XS_ReloadQueryResult(XS_ReloadId iId, XS_ReloadResult* pResult)
{
	XS_ReloadResult* pFound;

	if ( iId == 0 || pResult == NULL || g_XS_Reload.pLock == NULL ) return false;
	xrtMutexLock(g_XS_Reload.pLock);
	pFound = XS_ReloadResultFindLocked(iId);
	if ( pFound != NULL ) *pResult = *pFound;
	xrtMutexUnlock(g_XS_Reload.pLock);
	return pFound != NULL;
}

static bool XS_ReloadSnapshotConfig(XS_ReloadIntent* pIntent)
{
	pIntent->pConfigData = xrtFileReadAll(g_XS_Reload.sConfigPath, &pIntent->iConfigSize);
	if ( pIntent->pConfigData == NULL ) {
		XS_ReloadSetError("config snapshot read failed");
		return false;
	}
	pIntent->iRevision = XS_ReloadHashBytes(0,
		g_XS_Reload.sConfigPath, strlen(g_XS_Reload.sConfigPath));
	pIntent->iRevision = XS_ReloadHashBytes(pIntent->iRevision,
		pIntent->pConfigData, pIntent->iConfigSize);
	return true;
}

static bool XS_ReloadExecuteIntent(XS_ReloadIntent* pIntent)
{
	bool bOk = false;

	if ( pIntent->iKind == XS_RELOAD_KIND_HOST ) {
		XS_ServerInfo* pServer = xsServerFind(pIntent->sServerName);
		XS_HostInfo* pHost = pServer != NULL ? xsHostFind(pServer, pIntent->sHostName) : NULL;

		if ( pHost == NULL ) XS_ReloadSetError("reload target host not found");
		else bOk = XS_ReloadServerNow(g_XS_Reload.pApp, pIntent->sServerName);
		xsServerRelease(pServer);
	} else if ( pIntent->iKind == XS_RELOAD_KIND_SERVER ) {
		bOk = XS_ReloadServerNow(g_XS_Reload.pApp, pIntent->sServerName);
	} else {
		bOk = XS_ReloadAllAtomicNow(g_XS_Reload.pApp);
	}
	return bOk;
}

static int32 XS_ReloadThreadProc(ptr pData)
{
	(void)pData;
	for ( ;; ) {
		XS_ReloadIntent* pIntent;
		bool bOk;

		xrtMutexLock(g_XS_Reload.pLock);
		while ( !g_XS_Reload.bStopping && g_XS_Reload.pPendingHead == NULL ) {
			if ( xrtCondWait(g_XS_Reload.pCond, g_XS_Reload.pLock) == XWAIT_ERROR ) {
				g_XS_Reload.bStopping = true;
				break;
			}
		}
		if ( g_XS_Reload.bStopping ) {
			xrtMutexUnlock(g_XS_Reload.pLock);
			break;
		}
		pIntent = g_XS_Reload.pPendingHead;
		g_XS_Reload.pPendingHead = pIntent->pNext;
		pIntent->pNext = NULL;
		g_XS_Reload.iPendingCount--;
		XS_ReloadPendingTailRefreshLocked();
		g_XS_Reload.pActive = pIntent;
		XS_ReloadResultUpdateLocked(pIntent, XS_RELOAD_PREPARING, "preparing immutable snapshot");
		xrtMutexUnlock(g_XS_Reload.pLock);

		g_XS_ReloadCurrent = pIntent;
		XS_ScriptSetReadHook(XS_ReloadReadSourceSnapshot, pIntent);
		bOk = XS_ReloadSnapshotConfig(pIntent) && XS_ReloadExecuteIntent(pIntent);
		XS_ScriptSetReadHook(NULL, NULL);
		g_XS_ReloadCurrent = NULL;

		xrtMutexLock(g_XS_Reload.pLock);
		if ( pIntent->bSuperseded && !pIntent->bCommitted ) {
			XS_ReloadResultUpdateLocked(pIntent, XS_RELOAD_SUPERSEDED,
				"superseded before publication");
		} else if ( g_XS_Reload.bStopping && !pIntent->bCommitted ) {
			XS_ReloadResultUpdateLocked(pIntent, XS_RELOAD_CANCELLED,
				"cancelled by shutdown");
		} else if ( bOk ) {
			XS_ReloadResultUpdateLocked(pIntent, XS_RELOAD_SUCCEEDED, "active");
		} else {
			XS_ReloadResultUpdateLocked(pIntent, XS_RELOAD_FAILED,
				pIntent->sError[0] != '\0' ? pIntent->sError : "reload failed; see log");
		}
		g_XS_Reload.pActive = NULL;
		xrtMutexUnlock(g_XS_Reload.pLock);
		XS_ReloadIntentFree(pIntent);
	}
	return 0;
}

static bool XS_ReloadRuntimeInit(XS_App* pApp, const char* sConfigPath)
{
	memset(&g_XS_Reload, 0, sizeof(g_XS_Reload));
	if ( pApp == NULL || sConfigPath == NULL ) return false;
	g_XS_Reload.pLock = xrtMutexCreate();
	g_XS_Reload.pCond = xrtCondCreate();
	g_XS_Reload.sConfigPath = xrtStrDup(sConfigPath);
	g_XS_Reload.pApp = pApp;
	if ( g_XS_Reload.pLock == NULL || g_XS_Reload.pCond == NULL ||
	     g_XS_Reload.sConfigPath == NULL ) goto Failed;
	g_XS_Reload.pThread = xrtThreadCreate(XS_ReloadThreadProc, NULL, 0);
	if ( g_XS_Reload.pThread == NULL ) goto Failed;
	return true;

Failed:
	if ( g_XS_Reload.pCond != NULL ) xrtCondDestroy(g_XS_Reload.pCond);
	if ( g_XS_Reload.pLock != NULL ) xrtMutexDestroy(g_XS_Reload.pLock);
	xrtFree(g_XS_Reload.sConfigPath);
	memset(&g_XS_Reload, 0, sizeof(g_XS_Reload));
	return false;
}

static void XS_ReloadRuntimeStart(void)
{
	if ( g_XS_Reload.pLock == NULL ) return;
	xrtMutexLock(g_XS_Reload.pLock);
	if ( !g_XS_Reload.bStopping ) g_XS_Reload.bAccepting = true;
	xrtMutexUnlock(g_XS_Reload.pLock);
}

/* 停机不再发布新候选：完成已经跨过提交线性点的任务，取消其余 Active/Pending，
 * 等独立控制线程正常退出后才允许拆 listener/engine。 */
static void XS_ReloadQuiesce(void)
{
	XS_ReloadIntent* pPending;

	if ( g_XS_Reload.pLock == NULL ) return;
	xrtMutexLock(g_XS_Reload.pLock);
	if ( !g_XS_Reload.bStopping ) {
		g_XS_Reload.bAccepting = false;
		g_XS_Reload.bStopping = true;
		if ( g_XS_Reload.pActive != NULL && !g_XS_Reload.pActive->bCommitted &&
		     !g_XS_Reload.pActive->bStaging ) {
			g_XS_Reload.pActive->bSuperseded = true;
		}
	}
	pPending = g_XS_Reload.pPendingHead;
	g_XS_Reload.pPendingHead = NULL;
	g_XS_Reload.pPendingTail = NULL;
	g_XS_Reload.iPendingCount = 0;
	for ( XS_ReloadIntent* pIt = pPending; pIt != NULL; pIt = pIt->pNext ) {
		XS_ReloadResultUpdateLocked(pIt, XS_RELOAD_CANCELLED, "cancelled by shutdown");
	}
	(void)xrtCondBroadcast(g_XS_Reload.pCond);
	xrtMutexUnlock(g_XS_Reload.pLock);
	while ( pPending != NULL ) {
		XS_ReloadIntent* pNext = pPending->pNext;

		XS_ReloadIntentFree(pPending);
		pPending = pNext;
	}
	if ( g_XS_Reload.pThread != NULL && !g_XS_Reload.bJoined ) {
		(void)xrtThreadWait(g_XS_Reload.pThread);
		g_XS_Reload.bJoined = true;
	}
}

static void XS_ReloadRuntimeUnit(void)
{
	XS_ReloadQuiesce();
	if ( g_XS_Reload.pThread != NULL ) xrtThreadDestroy(g_XS_Reload.pThread);
	if ( g_XS_Reload.pCond != NULL ) xrtCondDestroy(g_XS_Reload.pCond);
	if ( g_XS_Reload.pLock != NULL ) xrtMutexDestroy(g_XS_Reload.pLock);
	xrtFree(g_XS_Reload.sConfigPath);
	memset(&g_XS_Reload, 0, sizeof(g_XS_Reload));
}

static bool XS_StrEq(const char* a, const char* b)
{
	if ( a == NULL || b == NULL ) {
		return a == b;
	}
	return strcmp(a, b) == 0;
}

static bool XS_ValueEquals(const xvalue* pA, const xvalue* pB)
{
	if ( pA == NULL || pB == NULL ) return pA == pB;
	return xrtValueEqual(pA, pB);
}

static bool XS_HostConfigEquals(XS_HostInfo* pA, XS_HostInfo* pB)
{
	return pA != NULL && pB != NULL && pA->Enabled == pB->Enabled &&
	       XS_StrEq(pA->Name, pB->Name) && XS_StrEq(pA->Host, pB->Host) &&
	       XS_StrEq(pA->Path, pB->Path) && XS_StrEq(pA->DevLang, pB->DevLang) &&
	       XS_StrEq(pA->DevFile, pB->DevFile) && XS_StrEq(pA->DevInc, pB->DevInc) &&
	       XS_StrEq(pA->DevLib, pB->DevLib) && XS_StrEq(pA->TlsCA, pB->TlsCA) &&
	       XS_StrEq(pA->TlsCert, pB->TlsCert) && XS_StrEq(pA->TlsKey, pB->TlsKey) &&
	       XS_ValueEquals(pA->Custom, pB->Custom);
}

/* 完整配置相等：所有预设字段和剩余 Custom 都参与，任何变更都不会静默忽略。 */
static bool XS_ServerConfigEquals(XS_ServerInfo* pA, XS_ServerInfo* pB)
{
	uint32 i;

	if ( strcmp(pA->Class, pB->Class) != 0 || !XS_StrEq(pA->Name, pB->Name) ||
	     pA->Enabled != pB->Enabled ||
	     !XS_StrEq(pA->IP, pB->IP) || !XS_StrEq(pA->IPTLS, pB->IPTLS) ||
	     pA->Port != pB->Port || pA->TLS != pB->TLS || pA->PortTLS != pB->PortTLS ||
	     pA->Backlog != pB->Backlog || pA->RecvLimit != pB->RecvLimit ||
	     pA->HostCount != pB->HostCount ||
	     !XS_HostConfigEquals(pA->DefaultHost, pB->DefaultHost) ||
	     !XS_ValueEquals(pA->Custom, pB->Custom) ) {
		return false;
	}
	for ( i = 0; i < pA->HostCount; i++ ) {
		if ( !XS_HostConfigEquals(pA->Hosts[i], pB->Hosts[i]) ) return false;
	}
	return true;
}

static bool XS_BindIpEquals(const char* pA, const char* pB)
{
	const char* a = (pA == NULL || pA[0] == '\0') ? "0.0.0.0" : pA;
	const char* b = (pB == NULL || pB[0] == '\0') ? "0.0.0.0" : pB;

	return strcmp(a, b) == 0;
}

static const char* XS_ServerTlsBindIp(const XS_ServerInfo* pServer)
{
	if ( pServer->IPTLS != NULL && pServer->IPTLS[0] != '\0' ) return pServer->IPTLS;
	return (pServer->IP == NULL || pServer->IP[0] == '\0') ? "0.0.0.0" : pServer->IP;
}

static bool XS_ServerEndpointEquals(
	const XS_ServerInfo* pA,
	bool bTlsA,
	const XS_ServerInfo* pB,
	bool bTlsB)
{
	const char* sIpA = bTlsA ? XS_ServerTlsBindIp(pA) : pA->IP;
	const char* sIpB = bTlsB ? XS_ServerTlsBindIp(pB) : pB->IP;
	uint16 iPortA = bTlsA ? pA->PortTLS : pA->Port;
	uint16 iPortB = bTlsB ? pB->PortTLS : pB->Port;

	return iPortA == iPortB && XS_BindIpEquals(sIpA, sIpB);
}

/* listener 创建后固化的部分必须一致，其他配置可经稳定槽位换整个 generation。 */
static bool XS_ServerCanHandoffEndpoint(XS_ServerInfo* pOld, XS_ServerInfo* pNew)
{
	return pOld->Enabled && pNew->Enabled &&
	       strcmp(pOld->Class, pNew->Class) == 0 && strcmp(pOld->Class, "custom") != 0 &&
	       XS_BindIpEquals(pOld->IP, pNew->IP) && pOld->Port == pNew->Port &&
	       pOld->TLS == pNew->TLS && pOld->Backlog == pNew->Backlog &&
	       pOld->RecvLimit == pNew->RecvLimit &&
	       (!pOld->TLS || (pOld->PortTLS == pNew->PortTLS &&
	        XS_BindIpEquals(XS_ServerTlsBindIp(pOld), XS_ServerTlsBindIp(pNew))));
}

/* 两代不能同时绑定同一传输端点时，优先走稳定 listener 切槽。 */
static bool XS_ServerBindConflicts(XS_ServerInfo* pOld, XS_ServerInfo* pNew)
{
	bool bOldUdp = strcmp(pOld->Class, "udp") == 0;
	bool bNewUdp = strcmp(pNew->Class, "udp") == 0;

	if ( !pOld->Enabled || !pNew->Enabled || strcmp(pOld->Class, "custom") == 0 ||
	     strcmp(pNew->Class, "custom") == 0 ) return false;
	if ( bOldUdp != bNewUdp ) return false;
	if ( XS_ServerEndpointEquals(pOld, false, pNew, false) ) return true;
	if ( bOldUdp ) return false;
	return (pNew->TLS && XS_ServerEndpointEquals(pOld, false, pNew, true)) ||
	       (pOld->TLS && XS_ServerEndpointEquals(pOld, true, pNew, false)) ||
	       (pOld->TLS && pNew->TLS && XS_ServerEndpointEquals(pOld, true, pNew, true));
}

static bool XS_ReloadClassSupported(XS_ServerInfo* pServer, const char* sScope)
{
	if ( pServer == NULL || pServer->Class == NULL ) {
		return false;
	}
	if ( strcmp(pServer->Class, "custom") == 0 ) {
		printf("[xs] %s reload rejected for custom server '%s': "
			"raw application resources have no terminal lease\n",
			sScope, pServer->Name);
		return false;
	}
	return true;
}

static bool XS_ReloadEngineConfigCompatible(const XS_App* pCurrent, const XS_App* pFresh)
{
	if ( pCurrent->EngineWorkers == pFresh->EngineWorkers ) return true;
	printf("[xs] reload rejected: engine.workers is process-immutable (%u -> %u); restart required\n",
		pCurrent->EngineWorkers, pFresh->EngineWorkers);
	return false;
}

static XS_HostInfo* XS_ReloadServerHostAt(XS_ServerInfo* pServer, uint32 iIndex)
{
	if ( pServer == NULL ) return NULL;
	return iIndex == 0 ? pServer->DefaultHost : pServer->Hosts[iIndex - 1];
}

static XS_HostInfo* XS_ReloadMatchOldHost(
	XS_ServerInfo* pOld,
	XS_HostInfo* pNewHost,
	uint32 iIndex)
{
	uint32 i;

	if ( pOld == NULL || pNewHost == NULL ) return NULL;
	/* DefaultHost 是协议驱动的固定回落位，即使改名也按位置交接。 */
	if ( iIndex == 0 ) return pOld->DefaultHost;
	for ( i = 0; i < pOld->HostCount; i++ ) {
		if ( XS_StrEq(pOld->Hosts[i]->Name, pNewHost->Name) ) return pOld->Hosts[i];
	}
	return NULL;
}

static xvalue* XS_ReloadTakeHostSwap(XS_HostInfo* pOldHost)
{
	XS_ScriptRuntime* pScript;
	xvalue* pShared = NULL;

	if ( pOldHost == NULL ) return NULL;
	pScript = XS_ScriptAcquireHost(pOldHost);
	if ( pScript != NULL && pScript->procSwap != NULL ) {
		xvalue* pOut = NULL;
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);
		bool bExported;

		/* ABI 契约要求 Swap 只导出快照，不转移/清空旧代正在使用的状态。 */
		bExported = pScript->procSwap(pOldHost, &pOut);
		XS_ScriptLeave(pPrevious);
		if ( bExported && pOut != NULL ) {
			pShared = pOut;
		} else if ( pOut != NULL ) {
			/* false 表示未导出；实现仍写出对象时由宿主兜底释放。 */
			xrtValueRelease(pOut);
		}
	}
	XS_ScriptRelease(pScript);
	return pShared;
}

/* 先编译整个 server 的全部 host，任一编译失败都不执行 Init。
 * 随后为候选 host 装入脚本但保持 not-ready，直到拓扑提交。 */
static bool XS_ReloadPrepareServer(
	XS_ServerInfo* pServer,
	bool bStartEndpoint,
	char* sErr,
	size_t iErrCap)
{
	XS_ScriptRuntime** pScripts = NULL;
	uint32 iHostCount;
	uint32 i;
	bool bOk = false;

	if ( XS_GenerationCreate(pServer) == NULL ) {
		snprintf(sErr, iErrCap, "generation create failed");
		return false;
	}
	if ( !pServer->Enabled ) {
		pServer->State = XS_RUN_STOPPED;
		pServer->DefaultHost->State = XS_RUN_STOPPED;
		for ( i = 0; i < pServer->HostCount; i++ ) {
			pServer->Hosts[i]->State = XS_RUN_STOPPED;
		}
		return true;
	}
	if ( strcmp(pServer->Class, "http") == 0 || strcmp(pServer->Class, "ws") == 0 ) {
		XS_VHostTable tCheck;

		if ( !XS_VHostTableBuild(pServer, &tCheck, sErr, iErrCap) ) return false;
		XS_VHostTableUnit(&tCheck);
	}
	/* HTTP/WS 的路由单元是 host；其他协议仍只编译 DefaultHost。 */
	iHostCount = (strcmp(pServer->Class, "http") == 0 ||
		strcmp(pServer->Class, "ws") == 0) ? 1 + pServer->HostCount : 1;
	pScripts = (XS_ScriptRuntime**)xrtCalloc(iHostCount, sizeof(XS_ScriptRuntime*));
	if ( pScripts == NULL ) {
		snprintf(sErr, iErrCap, "out of memory while compiling server scripts");
		return false;
	}
	for ( i = 0; i < iHostCount; i++ ) {
		XS_HostInfo* pHost = XS_ReloadServerHostAt(pServer, i);

		if ( pHost == NULL || !pHost->Enabled ) continue;
		if ( XS_HostHasScript(pHost) ) {
			pScripts[i] = XS_ScriptCompile(pHost);
			if ( pScripts[i] == NULL ) {
				snprintf(sErr, iErrCap, "host '%s' script compile failed",
					pHost->Name != NULL ? pHost->Name : "?");
				goto Done;
			}
			if ( strcmp(pServer->Class, "ws") == 0 &&
			     pScripts[i]->procWsText == NULL && pScripts[i]->procWsBinary == NULL ) {
				snprintf(sErr, iErrCap,
					"ws host '%s' requires script exporting WsText/WsBinary",
					pHost->Name != NULL ? pHost->Name : "?");
				goto Done;
			}
		} else if ( strcmp(pServer->Class, "http") != 0 ) {
			snprintf(sErr, iErrCap, "class '%s' host '%s' requires devfile",
				pServer->Class, pHost->Name != NULL ? pHost->Name : "?");
			goto Done;
		}
	}
	for ( i = 0; i < iHostCount; i++ ) {
		XS_HostInfo* pHost = XS_ReloadServerHostAt(pServer, i);
		XS_ScriptRuntime* pRuntime = pScripts[i];

		if ( pRuntime != NULL ) {
			XS_ScriptStagePrepared(pRuntime);
			(void)XS_ScriptMountPrepared(pHost, pRuntime);
			pScripts[i] = NULL;	/* 所有权已转入候选 host */
		}
		pHost->State = pHost->Enabled ? XS_RUN_STARTING : XS_RUN_STOPPED;
	}
	/* 候选端点可先 bind，但一律在 topology 事务内才开放接入。 */
	if ( !XS_ServerDriverStartEx(pServer, bStartEndpoint, false, sErr, iErrCap) ) goto Done;
	pServer->State = XS_RUN_STARTING;
	bOk = true;

Done:
	for ( i = 0; i < iHostCount; i++ ) {
		if ( pScripts[i] != NULL ) XS_ScriptDiscardCompiled(pScripts[i]);
	}
	xrtFree(pScripts);
	return bOk;
}

/* endpoint 已全部验证且 intent 已越过 staging 线性点后才执行旧代 Swap
 * 与候选 Init。此后的提交只包含预检过的指针/槽位切换。 */
static void XS_ReloadInitializeCandidateScripts(
	XS_ServerInfo* pServer,
	XS_ServerInfo* pOld)
{
	uint32 i;

	if ( pServer == NULL || !pServer->Enabled ) return;
	for ( i = 0; i < 1 + pServer->HostCount; i++ ) {
		XS_HostInfo* pHost = XS_ReloadServerHostAt(pServer, i);
		XS_ScriptRuntime* pRuntime = (XS_ScriptRuntime*)pHost->Runtime;

		if ( pRuntime != NULL ) {
			XS_HostInfo* pOldHost = XS_ReloadMatchOldHost(pOld, pHost, i);

			pRuntime->pSwap = XS_ReloadTakeHostSwap(pOldHost);
			XS_ScriptInitializePrepared(pHost, pRuntime);
		}
	}
}

static void XS_ReloadActivateCandidateScripts(XS_ServerInfo* pServer)
{
	uint32 i;

	if ( pServer == NULL ) return;
	for ( i = 0; i < 1 + pServer->HostCount; i++ ) {
		XS_HostInfo* pHost = XS_ReloadServerHostAt(pServer, i);
		XS_ScriptRuntime* pRuntime = (XS_ScriptRuntime*)pHost->Runtime;

		if ( pRuntime != NULL ) XS_ScriptActivatePrepared(pRuntime);
		pHost->State = pHost->Enabled && pServer->Enabled ? XS_RUN_RUNNING : XS_RUN_STOPPED;
	}
	(void)XS_GenerationPublish((XS_ServerGeneration*)pServer->Generation);
	pServer->State = pServer->Enabled ? XS_RUN_RUNNING : XS_RUN_STOPPED;
}

static void XS_ReloadDiscardCandidateScripts(XS_ServerInfo* pServer)
{
	uint32 i;

	if ( pServer == NULL ) return;
	XS_GenerationDiscard((XS_ServerGeneration*)pServer->Generation);
	for ( i = 0; i < 1 + pServer->HostCount; i++ ) {
		XS_HostInfo* pHost = XS_ReloadServerHostAt(pServer, i);
		XS_ScriptRuntime* pRuntime = (XS_ScriptRuntime*)pHost->Runtime;

		if ( pRuntime != NULL ) XS_ScriptDiscardMountedPrepared(pHost, pRuntime);
	}
}

static void XS_ReloadDiscardCandidate(XS_ServerInfo* pServer)
{
	XS_ConfigRevision* pOwner;

	if ( pServer == NULL ) return;
	if ( pServer->Generation == NULL ) {
		pOwner = (XS_ConfigRevision*)pServer->ConfigOwner;
		if ( pOwner != NULL ) XS_ConfigRevisionRelease(pOwner);
		return;
	}
	XS_ReloadDiscardCandidateScripts(pServer);
	if ( pServer->Runtime != NULL ) {
		XS_ServerDriverStop(pServer);
		XS_ServerDriverCloseConnections(pServer);
	}
	(void)XS_GcRetireServer(pServer, false);
}

static bool XS_ReloadServerNow(XS_App* pApp, const char* sName)
{
	XS_ServerInfo* pOld = NULL;
	XS_ServerInfo* pNew;
	XS_App tFresh;
	XS_ConfigRevision* pRevision;
	xvalue* pOldRoot = NULL;
	char sErr[256];
	uint32 i;
	bool bOk = false;
	bool bHandoff = false;
	bool bTopologyChanged = false;
	bool bCommitGate = false;

	pOld = xsServerFind(sName);
	if ( pOld == NULL || !XS_ReloadClassSupported(pOld, "server") ) {
		XS_ReloadSetError(pOld == NULL ? "server not found" : "server class cannot reload online");
		xsServerRelease(pOld);
		return false;
	}
	if ( !XS_ReloadLoadConfig(&tFresh) ) {
		printf("[xs] server reload: config re-parse failed: %s\n", tFresh.ParseError);
		XS_ReloadSetError(tFresh.ParseError);
		XS_ConfigFree(&tFresh);
		goto Done;
	}
	if ( !XS_ReloadEngineConfigCompatible(pApp, &tFresh) ) {
		XS_ReloadSetError("engine.workers changed; restart required");
		XS_ConfigFree(&tFresh);
		goto Done;
	}
	for ( i = 0; i < tFresh.ServerCount; i++ ) {
		if ( strcmp(tFresh.Servers[i]->Name, sName) == 0 ) break;
	}
	if ( i >= tFresh.ServerCount ) {
		printf("[xs] server reload: '%s' no longer exists (use reload-all to remove)\n", sName);
		XS_ReloadSetError("server no longer exists; use reload-all");
		XS_ConfigFree(&tFresh);
		goto Done;
	}
	pNew = tFresh.Servers[i];
	if ( g_XS_ReloadCurrent != NULL &&
	     g_XS_ReloadCurrent->iKind == XS_RELOAD_KIND_HOST &&
	     xsHostFind(pNew, g_XS_ReloadCurrent->sHostName) == NULL ) {
		printf("[xs] host reload: '%s/%s' no longer exists in desired config\n",
			sName, g_XS_ReloadCurrent->sHostName);
		XS_ReloadSetError("target host no longer exists in desired config");
		XS_ConfigFree(&tFresh);
		goto Done;
	}
	if ( strcmp(pNew->Class, "custom") == 0 ) {
		printf("[xs] server reload '%s' rejected: target custom generation has no lease boundary\n", sName);
		XS_ReloadSetError("custom server has no online terminal lease boundary");
		XS_ConfigFree(&tFresh);
		goto Done;
	}
	bHandoff = XS_ServerCanHandoffEndpoint(pOld, pNew);
	if ( XS_ServerBindConflicts(pOld, pNew) && !bHandoff ) {
		printf("[xs] server reload '%s' rejected: same endpoint changed an immutable listener "
			"field (class/tls/ip_tls/port_tls/backlog/recv_limit); restart or change endpoint\n", sName);
		XS_ReloadSetError("same endpoint changed immutable listener field");
		XS_ConfigFree(&tFresh);
		goto Done;
	}

	pRevision = XS_ConfigRevisionTake(&tFresh);
	if ( pRevision == NULL ) {
		XS_ReloadSetError("out of memory while owning config snapshot");
		XS_ConfigFree(&tFresh);
		goto Done;
	}
	pRevision->App.Engine = pApp->Engine;
	pNew = pRevision->App.Servers[i];
	if ( !XS_ConfigRevisionRetain(pRevision) ) {
		XS_ConfigRevisionRelease(pRevision);
		XS_ReloadSetError("cannot retain config revision");
		goto Done;
	}
	pNew->Engine = pApp->Engine;
	pNew->ConfigOwner = pRevision;
	XS_ConfigRevisionRelease(pRevision); /* 构建引用转交给候选 generation */
	sErr[0] = '\0';
	if ( !XS_ReloadPrepareServer(pNew, !bHandoff, sErr, sizeof(sErr)) ) {
		printf("[xs] server reload '%s': candidate failed: %s; old generation unchanged\n",
			sName, sErr);
		XS_ReloadSetError(sErr);
		XS_ReloadDiscardCandidate(pNew);
		goto Done;
	}
	/* 越过 staging、执行 Swap/Init 前再做一次无副作用槽位预检。 */
	if ( !(bHandoff ? XS_ServerDriverCanHandoff(pOld, pNew) :
	       XS_ServerDriverCanActivate(pNew)) ) {
		XS_ReloadSetError("listener preflight failed before script staging");
		XS_ReloadDiscardCandidate(pNew);
		goto Done;
	}
	if ( !XS_ReloadStageBegin() ) {
		XS_ReloadDiscardCandidate(pNew);
		goto Done;
	}
	XS_ReloadInitializeCandidateScripts(pNew, pOld);
	bCommitGate = XS_ReloadCommitBegin();
	if ( bCommitGate ) {
		if ( XS_TopologyWriteLock() ) {
			for ( i = 0; i < pApp->ServerCount; i++ ) {
				if ( pApp->Servers[i] == pOld ) break;
			}
			if ( i < pApp->ServerCount &&
			     (bHandoff ? XS_ServerDriverCanHandoff(pOld, pNew) :
				XS_ServerDriverCanActivate(pNew)) &&
			     (bHandoff ? XS_ServerDriverHandoff(pOld, pNew) :
				XS_ServerDriverActivate(pNew)) ) {
				if ( !bHandoff ) XS_ServerDriverDeactivate(pOld);
				pApp->Servers[i] = pNew;
				pOldRoot = XS_TopologyRootReplaceLocked(pApp,
					((XS_ConfigRevision*)pNew->ConfigOwner)->App.Root);
				XS_ReloadActivateCandidateScripts(pNew);
				bTopologyChanged = true;
			}
			XS_TopologyWriteUnlock();
		}
		XS_ReloadCommitEnd(bTopologyChanged);
	}
	xrtValueRelease(pOldRoot);
	if ( !bTopologyChanged ) {
		printf("[xs] server reload '%s': topology/listener handoff failed; old generation unchanged\n",
			sName);
		if ( g_XS_ReloadCurrent != NULL && !g_XS_ReloadCurrent->bSuperseded )
			XS_ReloadSetError("topology/listener handoff failed");
		XS_ReloadDiscardCandidate(pNew);
		goto Done;
	}
	XS_ServerDriverStop(pOld);
	(void)XS_GcRetireServer(pOld, pOld->ConfigOwner == NULL);
	printf("[xs] server reload '%s': candidate active%s; old generation draining\n",
		sName, bHandoff ? " on retained listener" : "");
	bOk = true;

Done:
	xsServerRelease(pOld);
	return bOk;
}

typedef struct XS_ReloadPlanItem {
	XS_ServerInfo*	pOld;
	XS_ServerInfo*	pNew;
	bool		bReuse;
	bool		bHandoff;
	bool		bHandedOff;
	bool		bActivated;
} XS_ReloadPlanItem;

static int32 XS_ReloadFindServerIndex(
	XS_ServerInfo** pServers,
	uint32 iCount,
	const char* sName)
{
	uint32 i;

	for ( i = 0; i < iCount; i++ ) {
		if ( pServers[i] != NULL && pServers[i]->Name != NULL &&
		     strcmp(pServers[i]->Name, sName) == 0 ) return (int32)i;
	}
	return -1;
}

/* 一个 reload-all revision 只解析一次；每个非复用候选持有同一 revision
 * 的独立引用，最后一个 generation 终态后再整体释放配置树。 */
static XS_ServerInfo* XS_ReloadAttachConfiguredServer(
	XS_App* pApp,
	XS_ConfigRevision* pRevision,
	uint32 iIndex)
{
	XS_ServerInfo* pServer;

	if ( pApp == NULL || pRevision == NULL ||
	     iIndex >= pRevision->App.ServerCount ||
	     !XS_ConfigRevisionRetain(pRevision) ) return NULL;
	pServer = pRevision->App.Servers[iIndex];
	pServer->Engine = pApp->Engine;
	pServer->ConfigOwner = pRevision;
	return pServer;
}

/* reload-all 是一份不可变配置 revision 的整批事务：所有候选先完成，任何失败
 * 都不改拓扑；最后在一个 topology 写锁内完成全部 listener handoff 和数组换槽。 */
static bool XS_ReloadAllAtomicNow(XS_App* pApp)
{
	XS_App tDesired;
	XS_ConfigRevision* pRevision = NULL;
	XS_App* pDesired = NULL;
	XS_ServerInfo** pCurrent = NULL;
	XS_ServerInfo** pNextTopology = NULL;
	XS_ServerInfo** pOldTopology = NULL;
	XS_ReloadPlanItem* pPlan = NULL;
	bool* pCurrentUsed = NULL;
	xvalue* pOldRoot = NULL;
	uint32 iCurrent = 0;
	uint32 iDesired = 0;
	uint32 i;
	uint32 j;
	bool bOk = false;
	bool bCommit = false;
	char sErr[256];

	memset(&tDesired, 0, sizeof(tDesired));
	if ( !XS_ReloadLoadConfig(&tDesired) ) {
		printf("[xs] reload all: config snapshot parse failed: %s\n", tDesired.ParseError);
		XS_ReloadSetError(tDesired.ParseError);
		goto Done;
	}
	if ( !XS_ReloadEngineConfigCompatible(pApp, &tDesired) ) {
		XS_ReloadSetError("engine.workers changed; restart required");
		goto Done;
	}
	pRevision = XS_ConfigRevisionTake(&tDesired);
	if ( pRevision == NULL ) {
		XS_ReloadSetError("out of memory while owning config revision");
		goto Done;
	}
	pDesired = &pRevision->App;
	pDesired->Engine = pApp->Engine;
	iDesired = pDesired->ServerCount;
	if ( !XS_TopologyServerSnapshot(&pCurrent, &iCurrent) ) {
		XS_ReloadSetError("cannot snapshot current topology");
		goto Done;
	}
	pPlan = (XS_ReloadPlanItem*)xrtCalloc(iDesired > 0 ? iDesired : 1,
		sizeof(XS_ReloadPlanItem));
	pNextTopology = (XS_ServerInfo**)xrtCalloc(iDesired > 0 ? iDesired : 1,
		sizeof(XS_ServerInfo*));
	pCurrentUsed = (bool*)xrtCalloc(iCurrent > 0 ? iCurrent : 1, sizeof(bool));
	if ( pPlan == NULL || pNextTopology == NULL || pCurrentUsed == NULL ) {
		XS_ReloadSetError("out of memory while building reload plan");
		goto Done;
	}

	for ( i = 0; i < iDesired; i++ ) {
		XS_ServerInfo* pWanted = pDesired->Servers[i];
		int32 iOld = XS_ReloadFindServerIndex(pCurrent, iCurrent, pWanted->Name);
		XS_ServerInfo* pOld = iOld >= 0 ? pCurrent[(uint32)iOld] : NULL;
		XS_ServerInfo* pNew;
		bool bHandoff = false;

		pPlan[i].pOld = pOld;
		if ( iOld >= 0 ) pCurrentUsed[(uint32)iOld] = true;
		if ( strcmp(pWanted->Class, "custom") == 0 ||
		     (pOld != NULL && strcmp(pOld->Class, "custom") == 0) ) {
			if ( pOld != NULL && strcmp(pOld->Class, "custom") == 0 &&
			     strcmp(pWanted->Class, "custom") == 0 &&
			     XS_ServerConfigEquals(pOld, pWanted) ) {
				pPlan[i].bReuse = true;
				pNextTopology[i] = pOld;
				continue;
			}
			printf("[xs] reload all rejected: custom server '%s' changed/added; restart required\n",
				pWanted->Name);
			XS_ReloadSetError("custom server topology/config changed; restart required");
			goto Done;
		}
		if ( pOld != NULL ) {
			bHandoff = XS_ServerCanHandoffEndpoint(pOld, pWanted);
			if ( XS_ServerBindConflicts(pOld, pWanted) && !bHandoff ) {
				printf("[xs] reload all rejected: server '%s' changed immutable same-endpoint field\n",
					pWanted->Name);
				XS_ReloadSetError("same endpoint changed immutable listener field");
				goto Done;
			}
		}
		pNew = XS_ReloadAttachConfiguredServer(pApp, pRevision, i);
		if ( pNew == NULL ) {
			XS_ReloadSetError("cannot retain configured server revision");
			goto Done;
		}
		pPlan[i].pNew = pNew;
		pPlan[i].bHandoff = bHandoff;
		pNextTopology[i] = pNew;
		sErr[0] = '\0';
		if ( !XS_ReloadPrepareServer(pNew, !bHandoff, sErr, sizeof(sErr)) ) {
			printf("[xs] reload all: candidate '%s' failed: %s; active revision unchanged\n",
				pWanted->Name, sErr);
			XS_ReloadSetError(sErr);
			goto Done;
		}
	}
	for ( i = 0; i < iCurrent; i++ ) {
		if ( !pCurrentUsed[i] && strcmp(pCurrent[i]->Class, "custom") == 0 ) {
			printf("[xs] reload all rejected: removing custom server '%s' requires restart\n",
				pCurrent[i]->Name);
			XS_ReloadSetError("removing custom server requires restart");
			goto Done;
		}
	}
	for ( i = 0; i < iDesired; i++ ) {
		if ( pPlan[i].bReuse ) continue;
		if ( pPlan[i].bHandoff ) {
			if ( !XS_ServerDriverCanHandoff(pPlan[i].pOld, pPlan[i].pNew) ) break;
		} else if ( !XS_ServerDriverCanActivate(pPlan[i].pNew) ) {
			break;
		}
	}
	if ( i != iDesired ) {
		XS_ReloadSetError("listener preflight failed before script staging");
		goto Done;
	}
	if ( !XS_ReloadStageBegin() ) goto Done;
	for ( i = 0; i < iDesired; i++ ) {
		if ( !pPlan[i].bReuse ) {
			XS_ReloadInitializeCandidateScripts(pPlan[i].pNew, pPlan[i].pOld);
		}
	}

	if ( !XS_ReloadCommitBegin() ) goto Done;
	if ( !XS_TopologyWriteLock() ) {
		XS_ReloadCommitEnd(false);
		XS_ReloadSetError("cannot acquire topology commit lock");
		goto Done;
	}
	/* 单 controller 理论上不会变化；仍做指针级校验，防止未来入口绕过协调器。 */
	bCommit = pApp->ServerCount == iCurrent;
	for ( i = 0; bCommit && i < iCurrent; i++ ) {
		bCommit = pApp->Servers[i] == pCurrent[i];
	}
	for ( i = 0; bCommit && i < iDesired; i++ ) {
		if ( pPlan[i].bHandoff ) {
			bCommit = XS_ServerDriverCanHandoff(pPlan[i].pOld, pPlan[i].pNew);
		} else if ( !pPlan[i].bReuse ) {
			bCommit = XS_ServerDriverCanActivate(pPlan[i].pNew);
		}
	}
	for ( i = 0; bCommit && i < iDesired; i++ ) {
		if ( pPlan[i].bHandoff ) {
			bCommit = XS_ServerDriverHandoff(pPlan[i].pOld, pPlan[i].pNew);
			pPlan[i].bHandedOff = bCommit;
		} else if ( !pPlan[i].bReuse ) {
			bCommit = XS_ServerDriverActivate(pPlan[i].pNew);
			pPlan[i].bActivated = bCommit;
		}
	}
	if ( !bCommit ) {
		/* 预检后原则上不可失败；若底层仍拒绝，逆向切回所有已完成槽位。 */
		for ( j = iDesired; j > 0; j-- ) {
			uint32 k = j - 1;

			if ( pPlan[k].bActivated ) {
				XS_ServerDriverDeactivate(pPlan[k].pNew);
			}
			if ( pPlan[k].bHandedOff &&
			     !XS_ServerDriverHandoff(pPlan[k].pNew, pPlan[k].pOld) ) {
				printf("[xs] fatal: listener handoff rollback failed for '%s'\n",
					pPlan[k].pOld->Name);
			}
		}
	} else {
		/* 新 revision 解锁前先封住所有不再复用的旧端点。
		 * 后续 Stop 只做异步 Close，不再存在旧代新接入窗口。 */
		for ( i = 0; i < iCurrent; i++ ) {
			bool bListenerReused = false;

			for ( j = 0; j < iDesired; j++ ) {
				if ( pPlan[j].pOld == pCurrent[i] &&
				     (pPlan[j].bReuse || pPlan[j].bHandoff) ) {
					bListenerReused = true;
					break;
				}
			}
			if ( !bListenerReused ) XS_ServerDriverDeactivate(pCurrent[i]);
		}
		pOldTopology = pApp->Servers;
		pApp->Servers = pNextTopology;
		pApp->ServerCount = iDesired;
		pNextTopology = NULL;
		pOldRoot = XS_TopologyRootReplaceLocked(pApp, pDesired->Root);
		for ( i = 0; i < iDesired; i++ ) {
			if ( !pPlan[i].bReuse ) XS_ReloadActivateCandidateScripts(pPlan[i].pNew);
		}
	}
	XS_TopologyWriteUnlock();
	XS_ReloadCommitEnd(bCommit);
	if ( !bCommit ) {
		XS_ReloadSetError("atomic topology/listener commit failed");
		goto Done;
	}
	xrtValueRelease(pOldRoot);
	xrtFree(pOldTopology);
	pOldTopology = NULL;
	for ( i = 0; i < iDesired; i++ ) {
		if ( pPlan[i].bHandoff ) {
			printf("[xs] server reload '%s': candidate active on retained listener; "
				"old generation draining\n", pPlan[i].pNew->Name);
		}
	}

	/* 拓扑已切到完整新 revision；旧 listener 此后只停止接入，连接自然排空。 */
	for ( i = 0; i < iCurrent; i++ ) {
		bool bReused = false;

		for ( j = 0; j < iDesired; j++ ) {
			if ( pPlan[j].bReuse && pPlan[j].pOld == pCurrent[i] ) {
				bReused = true;
				break;
			}
		}
		if ( !bReused ) {
			XS_ServerDriverStop(pCurrent[i]);
			(void)XS_GcRetireServer(pCurrent[i], pCurrent[i]->ConfigOwner == NULL);
		}
	}
	printf("[xs] reload all: revision %llu active (%u servers); old generations draining\n",
		(unsigned long long)(g_XS_ReloadCurrent != NULL ? g_XS_ReloadCurrent->iRevision : 0),
		iDesired);
	bOk = true;

Done:
	if ( !bOk && pPlan != NULL ) {
		for ( i = 0; i < iDesired; i++ ) {
			if ( pPlan[i].pNew != NULL ) XS_ReloadDiscardCandidate(pPlan[i].pNew);
		}
	}
	if ( pCurrent != NULL ) {
		for ( i = 0; i < iCurrent; i++ ) XS_TopologyServerRelease(pCurrent[i]);
	}
	xrtFree(pCurrent);
	xrtFree(pNextTopology);
	xrtFree(pOldTopology);
	xrtFree(pCurrentUsed);
	xrtFree(pPlan);
	XS_ConfigFree(&tDesired);
	XS_ConfigRevisionRelease(pRevision); /* 撤销本次 reconcile 的构建引用 */
	return bOk;
}

/* lifecycle timer 同时持有脚本代和服务代；取消也必须经过唯一终态回调。 */
typedef struct XS_TimerWrap {
	XS_GenerationTimer	tTimer;
	XS_TimerProc		proc;
	void*			pUserData;
	XS_ScriptRuntime*	pScript;
} XS_TimerWrap;

static void XS_TimerFire(xnetworker* pWorker, uint64 iId, xnetresult iResult, ptr pData)
{
	XS_TimerWrap* pWrap = (XS_TimerWrap*)pData;
	XS_ServerGeneration* pGeneration = XS_GenerationTimerFinish(&pWrap->tTimer);
	XS_ScriptRuntime* pScript = pWrap->pScript;

	(void)pWorker; (void)iId;
	if ( iResult == XNET_RESULT_OK && !XS_GenerationIsRetired(pGeneration) &&
	     !XS_ScriptIsRetired(pScript) && XS_ScriptWaitReady(pScript) ) {
		XS_ScriptRuntime* pPrevious = XS_ScriptEnter(pScript);

		pWrap->proc(pWrap->pUserData);
		XS_ScriptLeave(pPrevious);
	}
	XS_ScriptRelease(pScript);
	xrtFree(pWrap);
	XS_GenerationActivityRelease(pGeneration);
	XS_GenerationRelease(pGeneration);
}

static uint64 XS_TimerAfter(XS_HostInfo* pOwner, uint32 iMillisecond, XS_TimerProc proc, void* pUserData)
{
	XS_TimerWrap* pWrap;
	XS_ScriptRuntime* pScript;
	XS_ServerGeneration* pGeneration;
	uint64 iId;

	if ( pOwner == NULL || pOwner->Server == NULL || pOwner->Server->Engine == NULL ||
	     proc == NULL ) {
		return 0;
	}
	pScript = g_XS_CurrentScript;
	if ( pScript == NULL || pScript->pHost != pOwner || !XS_ScriptRetain(pScript) ) {
		pScript = XS_ScriptAcquireHost(pOwner);
	}
	if ( pScript == NULL ) return 0;
	pGeneration = (XS_ServerGeneration*)pOwner->Server->Generation;
	pWrap = (XS_TimerWrap*)xrtCalloc(1, sizeof(XS_TimerWrap));
	if ( pWrap == NULL ) {
		XS_ScriptRelease(pScript);
		return 0;
	}
	pWrap->proc = proc;
	pWrap->pUserData = pUserData;
	pWrap->pScript = pScript;
	iId = XS_GenerationTimerSchedule(pGeneration, (uint64)iMillisecond * 1000,
		XS_TimerFire, pWrap, pScript, &pWrap->tTimer);
	if ( iId == 0 ) {
		XS_ScriptRelease(pScript);
		xrtFree(pWrap);
	}
	return iId;
}

#endif
