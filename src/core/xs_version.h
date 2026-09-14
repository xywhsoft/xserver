/* XServer 版本标识。
 *
 * XS_VERSION_* 是产品版本号，随发布演进手工维护；
 * XS_BUILD_* 由 tools/build.py 在编译期注入（commit 短哈希 / 构建日期 /
 * 目标平台），仓库外手工编译时回退为 unknown。启动横幅与 --version
 * 输出这四项，见 main.c 的 XS_PrintVersion。
 */
#ifndef XS_VERSION_H
#define XS_VERSION_H

#ifndef XS_VERSION_MAJOR
	#define XS_VERSION_MAJOR 1
#endif
#ifndef XS_VERSION_MINOR
	#define XS_VERSION_MINOR 0
#endif
#ifndef XS_VERSION_PATCH
	#define XS_VERSION_PATCH 0
#endif

#define XS_VERSION_STRING "1.0.0"

#ifndef XS_BUILD_COMMIT
	#define XS_BUILD_COMMIT "unknown"
#endif
#ifndef XS_BUILD_DATE
	#define XS_BUILD_DATE "unknown"
#endif
#ifndef XS_BUILD_PLATFORM
	#define XS_BUILD_PLATFORM "unknown"
#endif

#endif
