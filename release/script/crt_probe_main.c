/*
 * CRT / WinAPI 能力探针：验证精简后的 TCC 运行时仍可解析
 *   - CRT（msvcrt.def）：printf / fopen 系列（仅验证链接，不实际写文件）
 *   - kernel32.def：CreateFileA / CloseHandle / GetModuleFileNameA
 *   - ole32/oleaut32.def：COM 初始化（Excel/自动化地基，仅 CoInitialize 校验）
 *   - ws2_32.def：socket 家族声明存在性
 * 仅用于 tcc 环境回归（test.bat），不属于业务示例。
 */
#include <xsbase.h>
#include <stdio.h>
#include <windows.h>
#include <objbase.h>
#include <winsock2.h>

static int g_Ok = 0;

void ServiceInit(XS_HostInfo* pHost)
{
	HANDLE hFile;
	char sPath[4096];
	WSADATA tWsa;
	HRESULT hr;

	(void)pHost;
	/* CRT */
	printf("[probe] crt printf ok\n");
	/* kernel32 */
	GetModuleFileNameA(NULL, sPath, sizeof(sPath));
	hFile = CreateFileA(sPath, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, 0, NULL);
	g_Ok += (hFile != INVALID_HANDLE_VALUE) ? 1 : 0;
	if ( hFile != INVALID_HANDLE_VALUE ) {
		CloseHandle(hFile);
	}
	/* ole32（COM/Excel 自动化地基） */
	hr = CoInitialize(NULL);
	g_Ok += SUCCEEDED(hr) ? 1 : 0;
	if ( SUCCEEDED(hr) ) {
		CoUninitialize();
	}
	/* ws2_32 */
	g_Ok += (WSAStartup(MAKEWORD(2, 2), &tWsa) == 0) ? 1 : 0;
	if ( g_Ok >= 3 ) {
		WSACleanup();
	}
	printf("[probe] winapi rounds ok=%d/3\n", g_Ok);
}

void ServiceUnit(XS_HostInfo* pHost)
{
	(void)pHost;
}
