#ifndef XS_RUNTIME_VHOST_H
#define XS_RUNTIME_VHOST_H

/*
 * HTTP / WebSocket 共用的不可变虚拟主机路由表。
 *
 * 表在 server generation 候选装配期构建，发布后只读，随 driver runtime
 * 一起释放。请求热路径不复制 Host 字符串、不拆分配置、不取得全局拓扑。
 */

#include <stdio.h>
#include <string.h>

#include "../sdk/xsbase.h"

typedef struct XS_VHostAlias {
	char*			sName;
	size_t			iNameSize;
	XS_HostInfo*		pHost;
} XS_VHostAlias;

typedef struct XS_VHostTable {
	XS_HostInfo*		pDefault;	/* disabled 时为 NULL */
	XS_VHostAlias*		pAliases;
	uint32			iAliasCount;
} XS_VHostTable;

typedef enum XS_VHostResult {
	XS_VHOST_BAD_REQUEST = -1,
	XS_VHOST_NOT_FOUND = 0,
	XS_VHOST_FOUND = 1
} XS_VHostResult;

static void XS_VHostTableUnit(XS_VHostTable* pTable)
{
	uint32 i;

	if ( pTable == NULL ) return;
	for ( i = 0; i < pTable->iAliasCount; i++ ) {
		xrtFree(pTable->pAliases[i].sName);
	}
	xrtFree(pTable->pAliases);
	memset(pTable, 0, sizeof(*pTable));
}

static bool XS_VHostAliasAdd(
	XS_VHostTable* pTable,
	XS_HostInfo* pHost,
	xstrview tText,
	char* sErr,
	size_t iErrCap)
{
	xhttpauthority tAuthority;
	XS_VHostAlias* pAliases;
	char* sName;
	uint32 i;

	if ( tText.Size == 0 || !xrtHttpHostParse(tText, &tAuthority) ||
	     tAuthority.Host.Size == 0 ||
	     (tAuthority.Flags & XHTTP_AUTHORITY_HAS_PORT) != 0 ) {
		snprintf(sErr, iErrCap, "host '%s' has invalid domain alias",
			pHost != NULL && pHost->Name != NULL ? pHost->Name : "?");
		return false;
	}
	for ( i = 0; i < pTable->iAliasCount; i++ ) {
		xstrview tExisting = xrtStrViewN(
			pTable->pAliases[i].sName,
			pTable->pAliases[i].iNameSize);

		if ( xrtHttpHostEqual(tExisting, tAuthority.Host) ) {
			snprintf(sErr, iErrCap,
				"duplicate virtual host alias '%.*s' (%s and %s)",
				(int)tAuthority.Host.Size, tAuthority.Host.Data,
				pTable->pAliases[i].pHost->Name != NULL ?
					pTable->pAliases[i].pHost->Name : "?",
				pHost != NULL && pHost->Name != NULL ? pHost->Name : "?");
			return false;
		}
	}
	sName = xrtStrDupN(tAuthority.Host.Data, tAuthority.Host.Size);
	if ( sName == NULL ) {
		snprintf(sErr, iErrCap, "out of memory while building virtual host aliases");
		return false;
	}
	pAliases = (XS_VHostAlias*)xrtRealloc(pTable->pAliases,
		(size_t)(pTable->iAliasCount + 1) * sizeof(XS_VHostAlias));
	if ( pAliases == NULL ) {
		xrtFree(sName);
		snprintf(sErr, iErrCap, "out of memory while building virtual host aliases");
		return false;
	}
	pTable->pAliases = pAliases;
	pTable->pAliases[pTable->iAliasCount].sName = sName;
	pTable->pAliases[pTable->iAliasCount].iNameSize = tAuthority.Host.Size;
	pTable->pAliases[pTable->iAliasCount].pHost = pHost;
	pTable->iAliasCount++;
	return true;
}

static bool XS_VHostAddHost(
	XS_VHostTable* pTable,
	XS_HostInfo* pHost,
	bool bRequireAlias,
	char* sErr,
	size_t iErrCap)
{
	const char* pSeg;
	bool bAdded = false;

	if ( pHost == NULL || !pHost->Enabled ) return true;
	if ( pHost->Name == NULL || pHost->Name[0] == '\0' ) {
		snprintf(sErr, iErrCap, "enabled virtual host requires non-empty name");
		return false;
	}
	if ( pHost->Host == NULL || pHost->Host[0] == '\0' ) {
		if ( bRequireAlias ) {
			snprintf(sErr, iErrCap, "virtual host '%s' requires domain alias", pHost->Name);
			return false;
		}
		return true;
	}
	pSeg = pHost->Host;
	for ( ; ; ) {
		const char* pEnd = strchr(pSeg, ';');
		size_t iLen = pEnd != NULL ? (size_t)(pEnd - pSeg) : strlen(pSeg);
		xstrview tAlias;

		while ( iLen > 0 && (*pSeg == ' ' || *pSeg == '\t') ) {
			pSeg++;
			iLen--;
		}
		while ( iLen > 0 && (pSeg[iLen - 1] == ' ' || pSeg[iLen - 1] == '\t') ) {
			iLen--;
		}
		if ( iLen == 0 ) {
			snprintf(sErr, iErrCap, "virtual host '%s' contains empty domain alias", pHost->Name);
			return false;
		}
		tAlias = xrtStrViewN(pSeg, iLen);
		if ( !XS_VHostAliasAdd(pTable, pHost, tAlias, sErr, iErrCap) ) return false;
		bAdded = true;
		if ( pEnd == NULL ) break;
		pSeg = pEnd + 1;
	}
	if ( bRequireAlias && !bAdded ) {
		snprintf(sErr, iErrCap, "virtual host '%s' requires domain alias", pHost->Name);
		return false;
	}
	return true;
}

static bool XS_VHostTableBuild(
	XS_ServerInfo* pServer,
	XS_VHostTable* pTable,
	char* sErr,
	size_t iErrCap)
{
	uint32 i;

	if ( pServer == NULL || pTable == NULL || pServer->DefaultHost == NULL ) {
		snprintf(sErr, iErrCap, "invalid virtual host table input");
		return false;
	}
	memset(pTable, 0, sizeof(*pTable));
	if ( pServer->DefaultHost->Enabled ) pTable->pDefault = pServer->DefaultHost;
	if ( !XS_VHostAddHost(pTable, pServer->DefaultHost, false, sErr, iErrCap) ) goto Failed;
	for ( i = 0; i < pServer->HostCount; i++ ) {
		if ( !XS_VHostAddHost(pTable, pServer->Hosts[i], true, sErr, iErrCap) ) goto Failed;
	}
	return true;

Failed:
	XS_VHostTableUnit(pTable);
	return false;
}

static XS_VHostResult XS_VHostRouteName(
	const XS_VHostTable* pTable,
	xstrview tName,
	XS_HostInfo** ppHost)
{
	uint32 i;

	if ( ppHost != NULL ) *ppHost = NULL;
	if ( pTable == NULL || ppHost == NULL || tName.Size == 0 ) {
		return XS_VHOST_BAD_REQUEST;
	}
	for ( i = 0; i < pTable->iAliasCount; i++ ) {
		xstrview tAlias = xrtStrViewN(
			pTable->pAliases[i].sName,
			pTable->pAliases[i].iNameSize);

		if ( xrtHttpHostEqual(tAlias, tName) ) {
			*ppHost = pTable->pAliases[i].pHost;
			return XS_VHOST_FOUND;
		}
	}
	if ( pTable->pDefault != NULL ) {
		*ppHost = pTable->pDefault;
		return XS_VHOST_FOUND;
	}
	return XS_VHOST_NOT_FOUND;
}

static XS_VHostResult XS_VHostRouteHead(
	const XS_VHostTable* pTable,
	const xhttp1head* pHead,
	bool bRequireHost,
	XS_HostInfo** ppHost)
{
	const xhttpfield* pField = NULL;
	xhttpauthority tHost;
	xhttptarget tTarget;
	xhttpnext eNext;

	if ( ppHost != NULL ) *ppHost = NULL;
	if ( pTable == NULL || pHead == NULL || ppHost == NULL ) return XS_VHOST_BAD_REQUEST;
	eNext = xrtHttpFieldGetUnique(pHead->Fields, pHead->FieldCount,
		XRT_STR_LITERAL("Host"), &pField);
	if ( eNext == XHTTP_NEXT_ERROR ) return XS_VHOST_BAD_REQUEST;
	if ( eNext == XHTTP_NEXT_ITEM && !xrtHttpHostParse(pField->Value, &tHost) ) {
		return XS_VHOST_BAD_REQUEST;
	}
	if ( !xrtHttpTargetParse(pHead->Method, pHead->Target, &tTarget) ) {
		return XS_VHOST_BAD_REQUEST;
	}
	if ( eNext == XHTTP_NEXT_END ) {
		if ( bRequireHost ) return XS_VHOST_BAD_REQUEST;
		if ( tTarget.Form == XHTTP_TARGET_ABSOLUTE ||
		     tTarget.Form == XHTTP_TARGET_AUTHORITY ) {
			return XS_VHostRouteName(pTable, tTarget.Host.Host, ppHost);
		}
		if ( pTable->pDefault != NULL ) {
			*ppHost = pTable->pDefault;
			return XS_VHOST_FOUND;
		}
		return XS_VHOST_NOT_FOUND;
	}
	/* absolute-form/CONNECT 的有效 authority 来自 request-target；
	 * Host 字段仍必须单一且语法有效，但不参与路由。 */
	if ( tTarget.Form == XHTTP_TARGET_ABSOLUTE ||
	     tTarget.Form == XHTTP_TARGET_AUTHORITY ) {
		return XS_VHostRouteName(pTable, tTarget.Host.Host, ppHost);
	}
	return XS_VHostRouteName(pTable, tHost.Host, ppHost);
}

#endif
