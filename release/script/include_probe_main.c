/*
 * SDK 头闭包探针：包含全部"保证入口"头，验证 include_win 精简后
 * 这些能力面仍然完整可编译（tools/tcc_include_closure.py 的守卫）。
 * 与 crt_probe 一样属于 tcc 环境回归，不是业务示例。
 */
#include <xsbase.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <direct.h>
#include <dirent.h>
#include <unistd.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <windows.h>
#include <winnetwk.h>
#include <objbase.h>
#include <oleauto.h>
#include <oaidl.h>
#include <ole2.h>
#include <comcat.h>
#include <msxml2.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <wincon.h>
#include <oledb.h>
#include <sql.h>
#include <sqlext.h>
#include <mlang.h>
#include <wintrust.h>
#include <wincrypt.h>
#include <wininet.h>
#include <sddl.h>
#include <aclapi.h>
#include <lm.h>

void ServiceInit(XS_HostInfo* pHost)
{
	(void)pHost;
}

void ServiceUnit(XS_HostInfo* pHost)
{
	(void)pHost;
}
