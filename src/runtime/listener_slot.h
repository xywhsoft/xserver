#ifndef XS_RUNTIME_LISTENER_SLOT_H
#define XS_RUNTIME_LISTENER_SLOT_H

/*
 * 稳定监听槽位。
 *
 * listener/socket 的 pData 永远指向本槽位，而不是某一代 driver runtime。
 * 同端点换代时只在锁内切换 Runtime/Generation；已经 Accept 的连接持有旧代
 * lease，新 Accept 则取得新代 lease。监听资源本身持有的 generation 引用也
 * 在切槽时逐个转移，因此旧代不需要等待仍服务于新代的 listener。
 */

#include "generation.h"

typedef struct XS_ListenerSlot {
	xmutex*			pLock;
	void*			pRuntime;
	void*			pTlsContext;	/* 当前代 XS_TlsTable*，由 TLS selector 在锁内读取 */
	XS_ServerGeneration*	pGeneration;
	uint32			iResources;	/* listener / TLS listener / UDP socket 数 */
	bool			bClosing;
} XS_ListenerSlot;

static XS_ListenerSlot* XS_ListenerSlotCreate(
	void* pRuntime,
	XS_ServerGeneration* pGeneration,
	void* pTlsContext)
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
	return pSlot;
}

/* 在创建一个真实监听资源前登记其 generation 引用；创建失败必须 Cancel。 */
static bool XS_ListenerSlotResourceAdd(XS_ListenerSlot* pSlot)
{
	bool bOk = false;

	if ( pSlot == NULL ) return false;
	xrtMutexLock(pSlot->pLock);
	if ( !pSlot->bClosing && pSlot->pGeneration != NULL &&
	     XS_GenerationRetain(pSlot->pGeneration) ) {
		pSlot->iResources++;
		bOk = true;
	}
	xrtMutexUnlock(pSlot->pLock);
	return bOk;
}

static void XS_ListenerSlotResourceCancel(XS_ListenerSlot* pSlot)
{
	XS_ServerGeneration* pGeneration = NULL;

	if ( pSlot == NULL ) return;
	xrtMutexLock(pSlot->pLock);
	if ( pSlot->iResources > 0 ) {
		pSlot->iResources--;
		pGeneration = pSlot->pGeneration;
	}
	xrtMutexUnlock(pSlot->pLock);
	XS_GenerationRelease(pGeneration);
}

/* 只用于尚未创建任何监听资源的候选/失败路径。 */
static void XS_ListenerSlotDestroyEmpty(XS_ListenerSlot* pSlot)
{
	if ( pSlot == NULL ) return;
	xrtMutexLock(pSlot->pLock);
	if ( pSlot->iResources != 0 ) {
		xrtMutexUnlock(pSlot->pLock);
		return;
	}
	pSlot->bClosing = true;
	pSlot->pRuntime = NULL;
	pSlot->pTlsContext = NULL;
	xrtMutexUnlock(pSlot->pLock);
	xrtMutexDestroy(pSlot->pLock);
	xrtFree(pSlot);
}

/* Accept 与切槽串行：返回时连接已经精确持有选中 generation。 */
static bool XS_ListenerSlotAcquireConnection(
	XS_ListenerSlot* pSlot,
	void** ppRuntime,
	XS_ServerGeneration** ppGeneration)
{
	bool bOk = false;

	if ( ppRuntime != NULL ) *ppRuntime = NULL;
	if ( ppGeneration != NULL ) *ppGeneration = NULL;
	if ( pSlot == NULL || ppRuntime == NULL || ppGeneration == NULL ) return false;
	xrtMutexLock(pSlot->pLock);
	if ( !pSlot->bClosing && pSlot->pRuntime != NULL &&
	     XS_GenerationConnectionAcquire(pSlot->pGeneration) ) {
		*ppRuntime = pSlot->pRuntime;
		*ppGeneration = pSlot->pGeneration;
		bOk = true;
	}
	xrtMutexUnlock(pSlot->pLock);
	return bOk;
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
	if ( pSlot->bClosing || pSlot->pRuntime != pExpectedRuntime ||
	     pSlot->iResources == 0 ) {
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
	xrtMutexUnlock(pSlot->pLock);
	for ( i = 0; i < iResources; i++ ) XS_GenerationRelease(pOldGeneration);
	return true;
}

/* Stop 先封槽，保证已经排队但尚未执行的 Accept 也只会拒绝。 */
static void XS_ListenerSlotBeginClose(XS_ListenerSlot* pSlot, void* pExpectedRuntime)
{
	if ( pSlot == NULL ) return;
	xrtMutexLock(pSlot->pLock);
	if ( pSlot->pRuntime == pExpectedRuntime ) {
		pSlot->bClosing = true;
		pSlot->pRuntime = NULL;
		pSlot->pTlsContext = NULL;
	}
	xrtMutexUnlock(pSlot->pLock);
}

/* 真实监听对象的唯一 Close 回调。最后一个资源负责销毁槽位。 */
static void XS_ListenerSlotResourceClose(XS_ListenerSlot* pSlot)
{
	XS_ServerGeneration* pGeneration = NULL;
	bool bLast = false;

	if ( pSlot == NULL ) return;
	xrtMutexLock(pSlot->pLock);
	if ( pSlot->iResources > 0 ) {
		pSlot->iResources--;
		pGeneration = pSlot->pGeneration;
		bLast = pSlot->iResources == 0;
		if ( bLast ) {
			pSlot->bClosing = true;
			pSlot->pRuntime = NULL;
			pSlot->pTlsContext = NULL;
		}
	}
	xrtMutexUnlock(pSlot->pLock);
	XS_GenerationRelease(pGeneration);
	if ( bLast ) {
		xrtMutexDestroy(pSlot->pLock);
		xrtFree(pSlot);
	}
}

#endif
