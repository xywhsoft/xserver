/*
	应用文件统一出口（Phase 2）：磁盘优先 → 站点 VFS 兜底。

	单文件发布模式下，放一个同名文件到 exe 旁边即可热修包内任何资源
	（证书、模板、页面、配置外文件）——外部文件永远赢，包内版本自然作废。
	所有读取点（TLS 证书、脚本 devfile 等）经 XS_AppReadAll 收口。

	返回缓冲的释放规则：命中磁盘 = xrtMalloc（xrtFree 释放）；
	命中 VFS = 包内缓存的借用指针（进程期内有效，**不得 xrtFree**）。
	调用方统一经 XS_AppFree 释放即可无差别处理两种来源。
*/
#ifndef XS_APPFILE_H
#define XS_APPFILE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 读应用文件：sRelPath 为相对 appPath 的路径（反斜杠自动归一）。
 * 磁盘存在 → 读磁盘；否则 VFS 激活且包内有该条目 → 惰性解压缓存视图。
 * *pFromVfs 收来源（true = 借用指针，用 XS_AppFree 释放是安全的 no-op）。
 * 失败返回 NULL。 */
void* XS_AppReadAll(const char* sRelPath, size_t* pSize, bool* pFromVfs);

/* 与 XS_AppReadAll 配对的释放：VFS 借用指针为 no-op，磁盘缓冲 xrtFree。 */
void XS_AppFree(void* pData, bool bFromVfs);

/* --no-vfs 调试开关：置 true 后 XS_VfsBootstrap 不再装载（已装载则失效） */
extern bool g_XS_VfsDisabled;

#ifdef __cplusplus
}
#endif

/* ================================================================== */
#ifdef XS_APPFILE_IMPLEMENTATION

#include "lib/xrt.h"

#include <string.h>

/* VFS 契约（xs_vfs.h 已在 main.c 以实现宏包含，这里仅声明） */
extern bool XS_VfsActive(void);
extern const unsigned char* XS_VfsReadAll(const char* sRelPath, size_t* pSize);

/* 就地反斜杠→正斜杠（用于 VFS 查找键；磁盘路径两种都合法不归一） */
static void XS_AppToForward(char* sPath)
{
	for ( ; *sPath != '\0'; sPath++ ) {
		if ( *sPath == '\\' ) *sPath = '/';
	}
}

void* XS_AppReadAll(const char* sRelPath, size_t* pSize, bool* pFromVfs)
{
	str sAbs;
	void* pData;

	if ( pFromVfs != NULL ) *pFromVfs = false;
	if ( sRelPath == NULL || sRelPath[0] == '\0' ) return NULL;

	/* 1) 磁盘优先：appPath 相对路径解析（绝对路径原样） */
	sAbs = xrtPathIsAbs(sRelPath) ? xrtStrDup(sRelPath)
	                               : xrtPathJoin(XS_AppPath(), sRelPath);
	if ( sAbs != NULL ) {
		size_t iSize = 0;

		pData = xrtFileReadAll(sAbs, &iSize);
		xrtFree(sAbs);
		if ( pData != NULL ) {
			if ( pSize != NULL ) *pSize = iSize;
			return pData;                        /* 磁盘赢 */
		}
	}

	/* 2) VFS 兜底：绝对路径剥 appPath 前缀，再归一反斜杠查包 */
	if ( !XS_VfsActive() ) return NULL;
	{
		char aKey[1024];
		const char* pRel = sRelPath;
		size_t n;
		const char* sApp = XS_AppPath();
		size_t nApp = sApp != NULL ? strlen(sApp) : 0;

		if ( nApp > 0 && strlen(sRelPath) > nApp &&
		     strncmp(sRelPath, sApp, nApp) == 0 &&
		     (sRelPath[nApp] == '/' || sRelPath[nApp] == '\\') )
			pRel = sRelPath + nApp + 1;
		n = strlen(pRel);
		if ( n == 0 || n >= sizeof(aKey) ) return NULL;
		memcpy(aKey, pRel, n + 1);
		XS_AppToForward(aKey);
		{
			size_t iSize = 0;
			const unsigned char* pMem = XS_VfsReadAll(aKey, &iSize);

			if ( pMem == NULL ) return NULL;
			if ( pSize != NULL ) *pSize = iSize;
			if ( pFromVfs != NULL ) *pFromVfs = true;
			return (void*)pMem;                  /* 借用指针 */
		}
	}
}

void XS_AppFree(void* pData, bool bFromVfs)
{
	if ( pData == NULL ) return;
	if ( !bFromVfs ) xrtFree(pData);            /* VFS 借用 = no-op */
}

#endif /* XS_APPFILE_IMPLEMENTATION */
#endif /* XS_APPFILE_H */
