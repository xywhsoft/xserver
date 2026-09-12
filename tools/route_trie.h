#ifndef XS_ROUTE_TRIE_H
#define XS_ROUTE_TRIE_H

/*
 * 分段 trie 路由 —— xs 范例动态路由的平坦时间形态
 *
 * 模式语法：'/' 分段；"{name}" 段捕获任意非空段（值借用输入路径）。
 * 匹配 O(路径长度)，与路由条数无关；同层字面量子节点小数组线性扫
 * （路由树同层扇出典型 < 10，L1 友好）。
 *
 * 生命周期：root 由 XS_TrieCreate 建立后只读；热重载语义 =
 * 建新树 → 原子换指针 → 旧树等在途请求返回后 XS_TrieFree。
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct XS_TrieParam {
	const char*		sName;		/* 借用模式串 */
	const char*		sValue;		/* 借用输入路径 */
	size_t			iValueSize;
} XS_TrieParam;

typedef struct XS_TrieNode {
	struct XS_TrieNode**	arrChild;	/* 字面量子节点 */
	char**			arrSeg;		/* 对应段名（与 arrChild 同下标，按段名有序） */
	uint32_t*		arrSegLen;	/* 段长缓存：探测先比长再 memcmp */
	uint32_t		iChildCount;
	struct XS_TrieNode*	pParam;		/* "{name}" 子节点（每层至多 1 个） */
	char*			sParamName;
	void*			pHandler;	/* 本节点终结的回调；非终结为 NULL */
} XS_TrieNode;

static XS_TrieNode* XS_TrieCreate(void)
{
	return (XS_TrieNode*)calloc(1, sizeof(XS_TrieNode));
}

static void XS_TrieFree(XS_TrieNode* pNode)
{
	uint32_t i;

	if ( pNode == NULL ) return;
	for ( i = 0; i < pNode->iChildCount; i++ ) {
		XS_TrieFree(pNode->arrChild[i]);
	}
	free(pNode->arrChild);
	free(pNode->arrSeg);
	free(pNode->arrSegLen);
	XS_TrieFree(pNode->pParam);
	free(pNode->sParamName);
	free(pNode);
}

/* 取路径下一段：*pp 移过段内容，返回段长；尾段返回 0 */
static size_t XS_TrieNextSeg(const char** pp, const char* sEnd)
{
	const char* p = *pp;

	while ( p < sEnd && *p == '/' ) p++;
	*pp = p;
	while ( p < sEnd && *p != '/' ) p++;
	return (size_t)(p - *pp);
}

/* 二分查找子节点（按段名字典序）；未命中时 *piPos 为有序插入位 */
static XS_TrieNode* XS_TrieChildFind(
	const XS_TrieNode* pNode, const char* p, size_t iSeg, uint32_t* piPos)
{
	uint32_t iLo = 0, iHi = pNode->iChildCount;

	while ( iLo < iHi ) {
		uint32_t iMid = (iLo + iHi) / 2;
		int cmp;

		if ( pNode->arrSeg[iMid][0] < p[0] ) { cmp = -1; }
		else if ( pNode->arrSeg[iMid][0] > p[0] ) { cmp = 1; }
		else if ( pNode->arrSegLen[iMid] < iSeg ) { cmp = -1; }
		else if ( pNode->arrSegLen[iMid] > iSeg ) { cmp = 1; }
		else { cmp = memcmp(pNode->arrSeg[iMid], p, iSeg); }
		if ( cmp == 0 ) { *piPos = iMid; return pNode->arrChild[iMid]; }
		if ( cmp < 0 ) iLo = iMid + 1; else iHi = iMid;
	}
	*piPos = iLo;
	return NULL;
}

/* 注册：模式串中的 {name} 段建参数子节点；同模式重复注册后者覆盖 */
static int XS_TrieInsert(XS_TrieNode* pRoot, const char* sPattern, void* pHandler)
{
	XS_TrieNode* pNode = pRoot;
	const char* p = sPattern;
	const char* sEnd = sPattern + strlen(sPattern);

	if ( pRoot == NULL || sPattern == NULL ) return -1;
	for ( ; ; ) {
		size_t iSeg = XS_TrieNextSeg(&p, sEnd);

		if ( iSeg == 0 ) break;
		if ( p[0] == '{' && p[iSeg - 1] == '}' && iSeg > 2 ) {
			if ( pNode->pParam == NULL ) {
				pNode->pParam = (XS_TrieNode*)calloc(1, sizeof(XS_TrieNode));
				if ( pNode->pParam == NULL ) return -1;
				pNode->pParam->sParamName = (char*)malloc(iSeg - 1);
				if ( pNode->pParam->sParamName == NULL ) return -1;
				memcpy(pNode->pParam->sParamName, p + 1, iSeg - 2);
				pNode->pParam->sParamName[iSeg - 2] = '\0';
			}
			pNode = pNode->pParam;
		} else {
			uint32_t iPos = 0;
			XS_TrieNode* pHit = XS_TrieChildFind(pNode, p, iSeg, &iPos);

			if ( pHit == NULL ) {
				char** pSegNew;
				XS_TrieNode** pChildNew;
				uint32_t* pLenNew;

				pHit = (XS_TrieNode*)calloc(1, sizeof(XS_TrieNode));
				if ( pHit == NULL ) return -1;
				pSegNew = (char**)realloc(pNode->arrSeg,
					(pNode->iChildCount + 1) * sizeof(char*));
				if ( pSegNew == NULL ) { free(pHit); return -1; }
				pNode->arrSeg = pSegNew;
				pChildNew = (XS_TrieNode**)realloc(pNode->arrChild,
					(pNode->iChildCount + 1) * sizeof(XS_TrieNode*));
				if ( pChildNew == NULL ) { free(pHit); return -1; }
				pNode->arrChild = pChildNew;
				pLenNew = (uint32_t*)realloc(pNode->arrSegLen,
					(pNode->iChildCount + 1) * sizeof(uint32_t));
				if ( pLenNew == NULL ) return -1;
				pNode->arrSegLen = pLenNew;
				memmove(&pNode->arrSeg[iPos + 1], &pNode->arrSeg[iPos],
					(pNode->iChildCount - iPos) * sizeof(char*));
				memmove(&pNode->arrChild[iPos + 1], &pNode->arrChild[iPos],
					(pNode->iChildCount - iPos) * sizeof(XS_TrieNode*));
				memmove(&pNode->arrSegLen[iPos + 1], &pNode->arrSegLen[iPos],
					(pNode->iChildCount - iPos) * sizeof(uint32_t));
				pNode->arrSeg[iPos] = (char*)malloc(iSeg + 1);
				if ( pNode->arrSeg[iPos] == NULL ) return -1;
				memcpy(pNode->arrSeg[iPos], p, iSeg);
				pNode->arrSeg[iPos][iSeg] = '\0';
				pNode->arrSegLen[iPos] = (uint32_t)iSeg;
				pNode->arrChild[iPos] = pHit;
				pNode->iChildCount++;
			}
			pNode = pHit;
		}
		p += iSeg;
	}
	pNode->pHandler = pHandler;
	return 0;
}

/* 匹配：命中返回回调并用 arrParam 回填参数视图（借用输入）；未命中 NULL */
static void* XS_TrieMatch(
	XS_TrieNode* pRoot, const char* sPath,
	XS_TrieParam* arrParam, uint32_t iParamCap, uint32_t* piParamCount)
{
	XS_TrieNode* pNode = pRoot;
	const char* p = sPath;
	const char* sEnd = sPath + strlen(sPath);
	uint32_t iParam = 0;

	if ( pRoot == NULL || sPath == NULL ) return NULL;
	for ( ; ; ) {
		size_t iSeg = XS_TrieNextSeg(&p, sEnd);
		XS_TrieNode* pNext = NULL;
		uint32_t i;

		if ( iSeg == 0 ) break;
		{
			uint32_t iPos = 0;

			pNext = XS_TrieChildFind(pNode, p, iSeg, &iPos);
		}
		if ( pNext == NULL && pNode->pParam != NULL ) {
			pNext = pNode->pParam;
			if ( iParam < iParamCap && arrParam != NULL ) {
				arrParam[iParam].sName = pNext->sParamName;
				arrParam[iParam].sValue = p;
				arrParam[iParam].iValueSize = iSeg;
				iParam++;
			}
		}
		if ( pNext == NULL ) {
			if ( piParamCount ) *piParamCount = 0;
			return NULL;
		}
		pNode = pNext;
		p += iSeg;
	}
	if ( piParamCount ) *piParamCount = iParam;
	return pNode->pHandler;
}

#endif
