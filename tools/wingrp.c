/*
 * xs3 tools/wingrp —— Windows 优雅停机测试辅助
 * 用法：
 *   wingrp run <logfile> <pidfile> <exe> [args...]   以独立进程组启动子进程
 *              （输出重定向 logfile，PID 写入 pidfile），等待退出后打印 "exit <code>"
 *   wingrp signal <pid>                              向进程组投递 CTRL_BREAK（走子进程控制台处理器）
 * 原理同 xserver tools/win_console_group_helper.c：CREATE_NEW_PROCESS_GROUP +
 * AttachConsole + GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid) 定向投递。
 */
#include <stdio.h>
#include <windows.h>

static int WingrpRun(const char* sLogPath, const char* sPidPath, char** argv, int iStart)
{
	SECURITY_ATTRIBUTES tSa;
	STARTUPINFOA tSi;
	PROCESS_INFORMATION tPi;
	HANDLE hFile;
	FILE* pPid;
	char sCmd[4096];
	int i;
	DWORD iExit = (DWORD)-1;

	tSa.nLength = sizeof(tSa);
	tSa.bInheritHandle = TRUE;
	tSa.lpSecurityDescriptor = NULL;
	hFile = CreateFileA(sLogPath, GENERIC_WRITE, FILE_SHARE_READ,
		&tSa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ) {
		fprintf(stderr, "open log failed: %lu\n", (unsigned long)GetLastError());
		return 1;
	}
	if ( !SetHandleInformation(hFile, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT) ) {
		fprintf(stderr, "set handle info failed\n");
		CloseHandle(hFile);
		return 1;
	}

	snprintf(sCmd, sizeof(sCmd), "\"%s\"", argv[iStart]);
	for ( i = iStart + 1; argv[i]; i++ ) {
		snprintf(sCmd + strlen(sCmd), sizeof(sCmd) - strlen(sCmd), " \"%s\"", argv[i]);
	}

	memset(&tSi, 0, sizeof(tSi));
	tSi.cb = sizeof(tSi);
	tSi.dwFlags = STARTF_USESTDHANDLES;
	tSi.hStdInput = NULL;
	tSi.hStdOutput = hFile;
	tSi.hStdError = hFile;
	memset(&tPi, 0, sizeof(tPi));

	if ( !CreateProcessA(NULL, sCmd, NULL, NULL, TRUE,
		CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
		NULL, NULL, &tSi, &tPi) ) {
		fprintf(stderr, "CreateProcess failed: %lu\n", (unsigned long)GetLastError());
		CloseHandle(hFile);
		return 1;
	}
	CloseHandle(hFile);

	pPid = fopen(sPidPath, "w");
	if ( pPid != NULL ) {
		fprintf(pPid, "%lu\n", (unsigned long)tPi.dwProcessId);
		fclose(pPid);
	}
	printf("pid %lu\n", (unsigned long)tPi.dwProcessId);
	fflush(stdout);

	WaitForSingleObject(tPi.hProcess, INFINITE);
	GetExitCodeProcess(tPi.hProcess, &iExit);
	CloseHandle(tPi.hThread);
	CloseHandle(tPi.hProcess);
	printf("exit %lu\n", (unsigned long)iExit);
	return 0;
}

static int WingrpSignal(const char* sPid)
{
	DWORD iPid = (DWORD)strtoul(sPid, NULL, 10);

	if ( iPid == 0 ) {
		fprintf(stderr, "invalid pid\n");
		return 1;
	}
	/* 免疫自身，再挂到目标进程的控制台（GenerateConsoleCtrlEvent 只对
	 * 与调用者共享控制台的进程组生效） */
	if ( !SetConsoleCtrlHandler(NULL, TRUE) ) {
		fprintf(stderr, "SetConsoleCtrlHandler failed: %lu\n", (unsigned long)GetLastError());
		return 2;
	}
	FreeConsole();
	if ( !AttachConsole(iPid) ) {
		fprintf(stderr, "AttachConsole failed: %lu\n", (unsigned long)GetLastError());
		return 3;
	}
	if ( !GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, iPid) ) {
		fprintf(stderr, "GenerateConsoleCtrlEvent failed: %lu\n", (unsigned long)GetLastError());
		FreeConsole();
		return 4;
	}
	Sleep(200);
	FreeConsole();
	return 0;
}

int main(int argc, char** argv)
{
	if ( argc >= 5 && strcmp(argv[1], "run") == 0 ) {
		return WingrpRun(argv[2], argv[3], argv, 4);
	}
	if ( argc == 3 && strcmp(argv[1], "signal") == 0 ) {
		return WingrpSignal(argv[2]);
	}
	fprintf(stderr, "usage: wingrp run <logfile> <pidfile> <exe> [args...] | wingrp signal <pid>\n");
	return 1;
}
