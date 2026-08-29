#ifndef XS_RUNTIME_GC_H
#define XS_RUNTIME_GC_H

/*
 * generation 的唯一析构出口。
 *
 * 不轮询连接数、不等待经验宽限、不设强制超时。旧 listener/connection/
 * UDP/timer 的终态回调各自释放引用，最后一个引用自然进入这里。
 */

#include <stdio.h>
#include <string.h>

#include "generation.h"
#include "../core/config.h"
#include "../core/driver.h"
#include "../script/script.h"

/* 连接/UDP 回调/lifecycle timer 已全部离开脚本后先通知 Unit。
 * 这里只调用一次，不撤销脚本 owner；TCC/配置仍由最后总引用归零后释放。 */
static void XS_GenerationQuiesce(XS_ServerGeneration* pGeneration)
{
	XS_ServerInfo* pServer = pGeneration != NULL ? pGeneration->pServer : NULL;
	uint32 i;

	if ( pServer == NULL ) return;
	XS_ScriptRequestUnitHost(pServer->DefaultHost);
	for ( i = 0; i < pServer->HostCount; i++ ) {
		XS_ScriptRequestUnitHost(pServer->Hosts[i]);
	}
}

static void XS_GenerationFinalize(XS_ServerGeneration* pGeneration)
{
	XS_ServerInfo* pServer = pGeneration->pServer;
	void* pDriverRuntime = pGeneration->pDriverRuntime;
	void* pConfigOwner = pGeneration->pConfigOwner;
	bool bFreeServer = pGeneration->bFreeServer;
	uint32 i;

	printf("[xs] generation finalized: server '%s' (connections=%u)\n",
		pServer != NULL && pServer->Name != NULL ? pServer->Name : "?",
		XS_GenerationConnectionCount(pGeneration));

	/* 所有异步资源均已终态：此后脚本 owner 引用可安全撤销。 */
	if ( pServer != NULL ) {
		XS_ScriptUnitHost(pServer->DefaultHost);
		for ( i = 0; i < pServer->HostCount; i++ ) {
			XS_ScriptUnitHost(pServer->Hosts[i]);
		}
	}

	/* driver runtime 只使用退役时捕获的指针，不追随可变 Runtime 槽位。 */
	if ( pDriverRuntime != NULL && pServer != NULL ) {
		pServer->Runtime = pDriverRuntime;
		XS_ServerDriverUnit(pServer);
	}

	if ( pServer != NULL ) {
		pServer->Runtime = NULL;
		pServer->Generation = NULL;
	}
	if ( pGeneration->pTimerLock != NULL ) {
		xrtMutexDestroy(pGeneration->pTimerLock);
		pGeneration->pTimerLock = NULL;
	}
	xrtFree(pGeneration);

	/* 动态快照拥有完整 xvalue 树；初始配置仅在结构换代时释放 server。 */
	if ( pConfigOwner != NULL ) {
		XS_ConfigFree((XS_App*)pConfigOwner);
		xrtFree(pConfigOwner);
	} else if ( bFreeServer ) {
		XS_ConfigServerFree(pServer);
	}
}

static bool XS_GcRetireServer(XS_ServerInfo* pServer, bool bFreeServer)
{
	XS_ServerGeneration* pGeneration;

	if ( pServer == NULL || pServer->Generation == NULL ) {
		return false;
	}
	pGeneration = (XS_ServerGeneration*)pServer->Generation;
	XS_GenerationRetire(pGeneration, pServer->Runtime, pServer->ConfigOwner,
		bFreeServer, XS_GenerationQuiesce, XS_GenerationFinalize);
	return true;
}

#endif
