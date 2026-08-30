#ifndef XS_RUNTIME_LISTENER_SLOT_H
#define XS_RUNTIME_LISTENER_SLOT_H

#include <string.h>

/*
 * 稳定监听槽位。
 *
 * listener/socket 的 pData 永远指向本槽位，而不是某一代 driver runtime。
 * 同端点换代时只在锁内切换 Runtime/Generation；已经 Accept 的连接持有旧代
 * lease，新 Accept 则取得新代 lease。监听资源本身持有的 generation 引用也
 * 在切槽时逐个转移，因此旧代不需要等待仍服务于新代的 listener。
 */

#include "generation.h"
#include "topology.h"

typedef struct XS_ListenerSlot {
	xmutex*			pLock;
	void*			pRuntime;
	void*			pTlsContext;	/* 当前代 XS_TlsTable*，由 TLS selector 在锁内读取 */
	XS_ServerGeneration*	pGeneration;
	uint32			iResources;	/* listener / TLS listener / UDP socket 数 */
	uint32			iResourceMask;
	uint32			iRequiredMask;	/* 启动阶段声明的完整端点集合，终态后不随之缩小 */
	void*			arrResources[3];
	bool			bAccepting;	/* 候选端点已 bind 但未发布时为 false */
	bool			bClosing;
	bool			bOwner;		/* 当前 driver runtime 的 owner ref */
} XS_ListenerSlot;

typedef enum XS_ListenerResourceKind {
	XS_LISTENER_RESOURCE_PLAIN = 0,
	XS_LISTENER_RESOURCE_TLS = 1,
	XS_LISTENER_RESOURCE_UDP = 2
} XS_ListenerResourceKind;

typedef struct XS_ListenerResources {
	xnetlistener*	pPlain;
	xtlslistener*	pTls;
	xnetudp*	pUdp;
} XS_ListenerResources;

static void XS_ListenerSlotFree(XS_ListenerSlot* pSlot)
{
	if ( pSlot == NULL ) return;
	xrtMutexDestroy(pSlot->pLock);
	xrtFree(pSlot);
}

static XS_ListenerSlot* XS_ListenerSlotCreate(
	void* pRuntime,
	XS_ServerGeneration* pGeneration,
	void* pTlsContext,
	bool bAccepting)
{
	XS_ListenerSlot* pSlot = (XS_ListenerSlot*)xrtCalloc(1, sizeof(XS_ListenerSlot));

	if ( pSlot == NULL ) return NULL;
	pSlot->pLock = xrtMutexCreate();
	if ( pSlot->pLock == NULL ) {
		xrtFree(pSlot);
		return NULL;
	}
	pSlot->pRuntime = pRuntime;
	pSlot->pGeneration = pGeneration;
	pSlot->pTlsContext = pTlsContext;
	pSlot->bAccepting = bAccepting;
	pSlot->bOwner = true;
	return pSlot;
}

/* 在创建一个真实监听资源前登记其 generation 引用；创建失败必须 Cancel。 */
static bool XS_ListenerSlotResourceAdd(
	XS_ListenerSlot* pSlot,
	XS_ListenerResourceKind iKind)
{
	bool bOk = false;
	uint32 iBit;

	if ( pSlot == NULL || iKind < 0 || iKind > XS_LISTENER_RESOURCE_UDP ) return false;
	iBit = (uint32)1u << (uint32)iKind;
	xrtMutexLock(pSlot->pLock);
	if ( pSlot->bOwner && !pSlot->bClosing &&
	     (pSlot->iResourceMask & iBit) == 0 && pSlot->pGeneration != NULL &&
	     XS_GenerationRetain(pSlot->pGeneration) ) {
		pSlot->iResources++;
		pSlot->iResourceMask |= iBit;
		pSlot->iRequiredMask |= iBit;
		bOk = true;
	}
	xrtMutexUnlock(pSlot->pLock);
	return bOk;
}

/* Start 返回 native handle 后登记。若资源已经异步终态，返回 false，调用方不得再访问 handle。 */
static bool XS_ListenerSlotResourceAttach(
	XS_ListenerSlot* pSlot,
	XS_ListenerResourceKind iKind,
	void* pResource)
{
	uint32 iBit;
	bool bOk = false;

	if ( pSlot == NULL || pResource == NULL ||
	     iKind < 0 || iKind > XS_LISTENER_RESOURCE_UDP ) return false;
	iBit = (uint32)1u << (uint32)iKind;
	xrtMutexLock(pSlot->pLock);
	if ( (pSlot->iResourceMask & iBit) != 0 ) {
		pSlot->arrResources[(uint32)iKind] = pResource;
		bOk = true;
	}
	xrtMutexUnlock(pSlot->pLock);
	return bOk;
}

static void XS_ListenerSlotResourceCancel(
	XS_ListenerSlot* pSlot,
	XS_ListenerResourceKind iKind)
{
	XS_ServerGeneration* pGeneration = NULL;
	uint32 iBit;

	if ( pSlot == NULL || iKind < 0 || iKind > XS_LISTENER_RESOURCE_UDP ) return;
	iBit = (uint32)1u << (uint32)iKind;
	xrtMutexLock(pSlot->pLock);
	if ( (pSlot->iResourceMask & iBit) != 0 ) {
		pSlot->iResourceMask &= ~iBit;
		pSlot->iRequiredMask &= ~iBit;
		pSlot->arrResources[(uint32)iKind] = NULL;
		pSlot->iResources--;
		pGeneration = pSlot->pGeneration;
	}
	xrtMutexUnlock(pSlot->pLock);
	XS_GenerationRelease(pGeneration);
}

/* 只用于尚未创建任何监听资源的候选/失败路径。 */
static void XS_ListenerSlotDestroyEmpty(XS_ListenerSlot* pSlot)
{
	bool bFree = false;

	if ( pSlot == NULL ) return;
	xrtMutexLock(pSlot->pLock);
	if ( pSlot->bOwner && pSlot->iResources == 0 ) {
		pSlot->bOwner = false;
		pSlot->bClosing = true;
		pSlot->bAccepting = false;
		pSlot->pRuntime = NULL;
		pSlot->pTlsContext = NULL;
		bFree = true;
	}
	xrtMutexUnlock(pSlot->pLock);
	if ( bFree ) XS_ListenerSlotFree(pSlot);
}

/* Accept 先进 topology 读侧再进槽位：reload-all 在 topology 写锁内
 * 切换所有槽位与公开数组，因此任一 Accept 只会看到完整旧版或完整新版。
 * 返回时连接已经精确持有选中 generation。 */
static bool XS_ListenerSlotAcquireConnection(
	XS_ListenerSlot* pSlot,
	uint64 iTlsCookie,
	void** ppRuntime,
	XS_ServerGeneration** ppGeneration)
{
	bool bOk = false;

	if ( ppRuntime != NULL ) *ppRuntime = NULL;
	if ( ppGeneration != NULL ) *ppGeneration = NULL;
	if ( pSlot == NULL || ppRuntime == NULL || ppGeneration == NULL ) return false;
	if ( !XS_TopologyReadLock() ) return false;
	xrtMutexLock(pSlot->pLock);
	if ( pSlot->bOwner && !pSlot->bClosing && pSlot->bAccepting &&
	     pSlot->pRuntime != NULL &&
	     (iTlsCookie == 0 || (pSlot->pGeneration != NULL &&
	      pSlot->pGeneration->iCookie == iTlsCookie)) &&
	     XS_GenerationConnectionAcquire(pSlot->pGeneration) ) {
		*ppRuntime = pSlot->pRuntime;
		*ppGeneration = pSlot->pGeneration;
		bOk = true;
	}
	xrtMutexUnlock(pSlot->pLock);
	XS_TopologyReadUnlock();
	return bOk;
}

/* 多 server 事务在真正切槽前做无副作用预检；停机先等待 reload controller，
 * 因此预检到提交之间 listener 不会被并发关闭。 */
static bool XS_ListenerSlotCanHandoff(XS_ListenerSlot* pSlot, void* pExpectedRuntime)
{
	bool bOk;

	if ( pSlot == NULL || pExpectedRuntime == NULL ) return false;
	xrtMutexLock(pSlot->pLock);
	bOk = pSlot->bOwner && !pSlot->bClosing && pSlot->bAccepting &&
	      pSlot->pRuntime == pExpectedRuntime &&
	      pSlot->pGeneration != NULL && pSlot->iResources > 0 &&
	      pSlot->iResourceMask == pSlot->iRequiredMask;
	xrtMutexUnlock(pSlot->pLock);
	return bOk;
}

/* 新端点可在事务外 bind/预热，但只能在 topology 写锁下发布。 */
static bool XS_ListenerSlotCanActivate(XS_ListenerSlot* pSlot, void* pExpectedRuntime)
{
	bool bOk;

	if ( pSlot == NULL || pExpectedRuntime == NULL ) return false;
	xrtMutexLock(pSlot->pLock);
	bOk = pSlot->bOwner && !pSlot->bClosing && !pSlot->bAccepting &&
	      pSlot->pRuntime == pExpectedRuntime && pSlot->pGeneration != NULL &&
	      pSlot->iResources > 0 && pSlot->iResourceMask == pSlot->iRequiredMask;
	xrtMutexUnlock(pSlot->pLock);
	return bOk;
}

static bool XS_ListenerSlotActivate(XS_ListenerSlot* pSlot, void* pExpectedRuntime)
{
	bool bOk;

	if ( pSlot == NULL || pExpectedRuntime == NULL ) return false;
	xrtMutexLock(pSlot->pLock);
	bOk = pSlot->bOwner && !pSlot->bClosing && !pSlot->bAccepting &&
	      pSlot->pRuntime == pExpectedRuntime && pSlot->pGeneration != NULL &&
	      pSlot->iResources > 0 && pSlot->iResourceMask == pSlot->iRequiredMask;
	if ( bOk ) pSlot->bAccepting = true;
	xrtMutexUnlock(pSlot->pLock);
	return bOk;
}

/* 仅用于多 server 事务在意外失败时撤销尚未公开的候选端点。 */
static void XS_ListenerSlotDeactivate(XS_ListenerSlot* pSlot, void* pExpectedRuntime)
{
	if ( pSlot == NULL || pExpectedRuntime == NULL ) return;
	xrtMutexLock(pSlot->pLock);
	if ( pSlot->bOwner && !pSlot->bClosing && pSlot->pRuntime == pExpectedRuntime ) {
		pSlot->bAccepting = false;
	}
	xrtMutexUnlock(pSlot->pLock);
}

/* 调用方须同时持有 topology 写锁，使公开拓扑与 Accept 切换线性化。 */
static bool XS_ListenerSlotHandoff(
	XS_ListenerSlot* pSlot,
	void* pExpectedRuntime,
	void* pNewRuntime,
	XS_ServerGeneration* pNewGeneration,
	void* pNewTlsContext)
{
	XS_ServerGeneration* pOldGeneration;
	uint32 i;
	uint32 iRetained = 0;
	uint32 iResources;

	if ( pSlot == NULL || pNewRuntime == NULL || pNewGeneration == NULL ) return false;
	xrtMutexLock(pSlot->pLock);
	if ( !pSlot->bOwner || pSlot->bClosing || !pSlot->bAccepting ||
	     pSlot->pRuntime != pExpectedRuntime ||
	     pSlot->iResources == 0 || pSlot->iResourceMask != pSlot->iRequiredMask ) {
		xrtMutexUnlock(pSlot->pLock);
		return false;
	}
	iResources = pSlot->iResources;
	for ( i = 0; i < iResources; i++ ) {
		if ( !XS_GenerationRetain(pNewGeneration) ) break;
		iRetained++;
	}
	if ( iRetained != iResources ) {
		xrtMutexUnlock(pSlot->pLock);
		while ( iRetained > 0 ) {
			iRetained--;
			XS_GenerationRelease(pNewGeneration);
		}
		return false;
	}
	pOldGeneration = pSlot->pGeneration;
	pSlot->pRuntime = pNewRuntime;
	pSlot->pGeneration = pNewGeneration;
	pSlot->pTlsContext = pNewTlsContext;
	pSlot->bAccepting = true;
	xrtMutexUnlock(pSlot->pLock);
	for ( i = 0; i < iResources; i++ ) XS_GenerationRelease(pOldGeneration);
	return true;
}

/* Stop 先撤销 runtime owner 并封槽，再在锁外关闭返回的 native handles。
 * slot 由尚未终态的 resource refs 保持；异常 Close 已经清掉的 handle 不会二次关闭。 */
static bool XS_ListenerSlotBeginClose(
	XS_ListenerSlot* pSlot,
	void* pExpectedRuntime,
	XS_ListenerResources* pResources)
{
	bool bOk = false;
	bool bFree = false;

	if ( pResources != NULL ) memset(pResources, 0, sizeof(*pResources));
	if ( pSlot == NULL || pResources == NULL ) return false;
	xrtMutexLock(pSlot->pLock);
	if ( pSlot->bOwner && pSlot->pRuntime == pExpectedRuntime ) {
		if ( pSlot->arrResources[XS_LISTENER_RESOURCE_PLAIN] != NULL ) {
			pResources->pPlain = xrtNetListenerRef(
				(xnetlistener*)pSlot->arrResources[XS_LISTENER_RESOURCE_PLAIN]);
		}
		if ( pSlot->arrResources[XS_LISTENER_RESOURCE_TLS] != NULL ) {
			pResources->pTls = xrtTlsListenerRef(
				(xtlslistener*)pSlot->arrResources[XS_LISTENER_RESOURCE_TLS]);
		}
		if ( pSlot->arrResources[XS_LISTENER_RESOURCE_UDP] != NULL ) {
			pResources->pUdp = xrtNetUdpRef(
				(xnetudp*)pSlot->arrResources[XS_LISTENER_RESOURCE_UDP]);
		}
		memset(pSlot->arrResources, 0, sizeof(pSlot->arrResources));
		pSlot->bClosing = true;
		pSlot->bAccepting = false;
		pSlot->bOwner = false;
		pSlot->pRuntime = NULL;
		pSlot->pTlsContext = NULL;
		bOk = true;
		bFree = pSlot->iResources == 0;
	}
	xrtMutexUnlock(pSlot->pLock);
	if ( bFree ) XS_ListenerSlotFree(pSlot);
	return bOk;
}

/* 真实监听对象的唯一 Close 回调。资源终态和 runtime owner 都归还后才销毁槽位。 */
static void XS_ListenerSlotResourceClose(
	XS_ListenerSlot* pSlot,
	XS_ListenerResourceKind iKind)
{
	XS_ServerGeneration* pGeneration = NULL;
	uint32 iBit;
	bool bFree = false;

	if ( pSlot == NULL || iKind < 0 || iKind > XS_LISTENER_RESOURCE_UDP ) return;
	iBit = (uint32)1u << (uint32)iKind;
	xrtMutexLock(pSlot->pLock);
	if ( (pSlot->iResourceMask & iBit) != 0 ) {
		pSlot->iResourceMask &= ~iBit;
		pSlot->arrResources[(uint32)iKind] = NULL;
		pSlot->iResources--;
		pGeneration = pSlot->pGeneration;
		if ( pSlot->iResources == 0 ) {
			pSlot->bClosing = true;
			pSlot->bAccepting = false;
			pSlot->pTlsContext = NULL;
			bFree = !pSlot->bOwner;
		}
	}
	xrtMutexUnlock(pSlot->pLock);
	XS_GenerationRelease(pGeneration);
	if ( bFree ) XS_ListenerSlotFree(pSlot);
}

#endif
