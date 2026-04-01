#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void procPrintUsage(void)
{
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "  win_console_group_helper spawn <workdir> <logfile> <pidfile> <exe> [args...]\n");
	fprintf(stderr, "  win_console_group_helper signal <pid>\n");
}

static int procAppendChar(char** ppDst, size_t* piRemain, char c)
{
	if ( ppDst == NULL || piRemain == NULL || *ppDst == NULL || *piRemain <= 1 ) {
		return 0;
	}

	**ppDst = c;
	(*ppDst)++;
	(*piRemain)--;
	return 1;
}

static int procAppendQuotedArg(char** ppDst, size_t* piRemain, const char* sArg)
{
	size_t iSlashCount = 0;

	if ( sArg == NULL ) {
		sArg = "";
	}
	if ( !procAppendChar(ppDst, piRemain, '"') ) {
		return 0;
	}

	while ( *sArg != '\0' ) {
		size_t i;

		if ( *sArg == '\\' ) {
			iSlashCount++;
			sArg++;
			continue;
		}

		if ( *sArg == '"' ) {
			for ( i = 0; i < ((iSlashCount * 2) + 1); i++ ) {
				if ( !procAppendChar(ppDst, piRemain, '\\') ) {
					return 0;
				}
			}
			if ( !procAppendChar(ppDst, piRemain, '"') ) {
				return 0;
			}
			iSlashCount = 0;
			sArg++;
			continue;
		}

		for ( i = 0; i < iSlashCount; i++ ) {
			if ( !procAppendChar(ppDst, piRemain, '\\') ) {
				return 0;
			}
		}
		iSlashCount = 0;
		if ( !procAppendChar(ppDst, piRemain, *sArg) ) {
			return 0;
		}
		sArg++;
	}

	while ( iSlashCount-- > 0 ) {
		if ( !procAppendChar(ppDst, piRemain, '\\') ) {
			return 0;
		}
		if ( !procAppendChar(ppDst, piRemain, '\\') ) {
			return 0;
		}
	}

	if ( !procAppendChar(ppDst, piRemain, '"') ) {
		return 0;
	}

	return 1;
}

static char* procBuildCommandLine(int argc, char** argv, int iStart)
{
	size_t i;
	size_t iBufSize = 1;
	char* sCmdLine;
	char* pWrite;
	size_t iRemain;

	for ( i = (size_t)iStart; i < (size_t)argc; i++ ) {
		iBufSize += (strlen(argv[i]) * 2) + 4;
	}

	sCmdLine = (char*)malloc(iBufSize);
	if ( sCmdLine == NULL ) {
		return NULL;
	}

	pWrite = sCmdLine;
	iRemain = iBufSize;
	for ( i = (size_t)iStart; i < (size_t)argc; i++ ) {
		if ( i > (size_t)iStart ) {
			if ( !procAppendChar(&pWrite, &iRemain, ' ') ) {
				free(sCmdLine);
				return NULL;
			}
		}
		if ( !procAppendQuotedArg(&pWrite, &iRemain, argv[i]) ) {
			free(sCmdLine);
			return NULL;
		}
	}

	*pWrite = '\0';
	return sCmdLine;
}

static HANDLE procOpenLogHandle(const char* sPath)
{
	SECURITY_ATTRIBUTES tSecAttr;
	const char* sTarget = sPath;

	memset(&tSecAttr, 0, sizeof(tSecAttr));
	tSecAttr.nLength = sizeof(tSecAttr);
	tSecAttr.bInheritHandle = TRUE;

	if ( sTarget == NULL || sTarget[0] == '\0' ) {
		sTarget = "NUL";
	}

	return CreateFileA(
		sTarget,
		FILE_APPEND_DATA | SYNCHRONIZE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&tSecAttr,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
}

static HANDLE procOpenNullInputHandle(void)
{
	SECURITY_ATTRIBUTES tSecAttr;

	memset(&tSecAttr, 0, sizeof(tSecAttr));
	tSecAttr.nLength = sizeof(tSecAttr);
	tSecAttr.bInheritHandle = TRUE;

	return CreateFileA(
		"NUL",
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&tSecAttr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
}

static int procWritePidFile(const char* sPath, DWORD iPid)
{
	HANDLE hFile;
	char sText[64];
	DWORD iWritten = 0;
	SECURITY_ATTRIBUTES tSecAttr;

	memset(&tSecAttr, 0, sizeof(tSecAttr));
	tSecAttr.nLength = sizeof(tSecAttr);
	tSecAttr.bInheritHandle = FALSE;

	if ( sPath == NULL || sPath[0] == '\0' || strcmp(sPath, "-") == 0 ) {
		return 1;
	}

	hFile = CreateFileA(
		sPath,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		&tSecAttr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if ( hFile == INVALID_HANDLE_VALUE ) {
		return 0;
	}

	snprintf(sText, sizeof(sText), "%lu\r\n", (unsigned long)iPid);
	if ( !WriteFile(hFile, sText, (DWORD)strlen(sText), &iWritten, NULL) ) {
		CloseHandle(hFile);
		return 0;
	}

	CloseHandle(hFile);
	return 1;
}

static int procSpawnChild(int argc, char** argv)
{
	STARTUPINFOA tStartup;
	PROCESS_INFORMATION tProcInfo;
	HANDLE hLogFile = INVALID_HANDLE_VALUE;
	HANDLE hNullInput = INVALID_HANDLE_VALUE;
	char* sCmdLine = NULL;
	BOOL bRet;

	if ( argc < 6 ) {
		procPrintUsage();
		return 2;
	}

	sCmdLine = procBuildCommandLine(argc, argv, 5);
	if ( sCmdLine == NULL ) {
		fprintf(stderr, "build command line failed\n");
		return 3;
	}

	hLogFile = procOpenLogHandle(argv[3]);
	if ( hLogFile == INVALID_HANDLE_VALUE ) {
		fprintf(stderr, "open logfile failed: %lu\n", (unsigned long)GetLastError());
		free(sCmdLine);
		return 4;
	}
	hNullInput = procOpenNullInputHandle();
	if ( hNullInput == INVALID_HANDLE_VALUE ) {
		fprintf(stderr, "open nul stdin failed: %lu\n", (unsigned long)GetLastError());
		CloseHandle(hLogFile);
		free(sCmdLine);
		return 4;
	}

	SetFilePointer(hLogFile, 0, NULL, FILE_END);
	(void)SetHandleInformation(GetStdHandle(STD_INPUT_HANDLE), HANDLE_FLAG_INHERIT, 0);
	(void)SetHandleInformation(GetStdHandle(STD_OUTPUT_HANDLE), HANDLE_FLAG_INHERIT, 0);
	(void)SetHandleInformation(GetStdHandle(STD_ERROR_HANDLE), HANDLE_FLAG_INHERIT, 0);

	memset(&tStartup, 0, sizeof(tStartup));
	memset(&tProcInfo, 0, sizeof(tProcInfo));
	tStartup.cb = sizeof(tStartup);
	tStartup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	tStartup.hStdInput = hNullInput;
	tStartup.hStdOutput = hLogFile;
	tStartup.hStdError = hLogFile;
	tStartup.wShowWindow = SW_HIDE;

	bRet = CreateProcessA(
		argv[5],
		sCmdLine,
		NULL,
		NULL,
		TRUE,
		CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
		NULL,
		argv[2],
		&tStartup,
		&tProcInfo
	);

	if ( !bRet ) {
		fprintf(stderr, "CreateProcess failed: %lu\n", (unsigned long)GetLastError());
		CloseHandle(hNullInput);
		CloseHandle(hLogFile);
		free(sCmdLine);
		return 5;
	}
	if ( !procWritePidFile(argv[4], tProcInfo.dwProcessId) ) {
		fprintf(stderr, "write pid file failed\n");
		CloseHandle(tProcInfo.hThread);
		CloseHandle(tProcInfo.hProcess);
		CloseHandle(hNullInput);
		CloseHandle(hLogFile);
		free(sCmdLine);
		return 6;
	}

	CloseHandle(tProcInfo.hThread);
	CloseHandle(tProcInfo.hProcess);
	CloseHandle(hNullInput);
	CloseHandle(hLogFile);
	free(sCmdLine);
	return 0;
}

static int procSendSignal(int argc, char** argv)
{
	DWORD iPid;

	if ( argc != 3 ) {
		procPrintUsage();
		return 2;
	}

	iPid = (DWORD)strtoul(argv[2], NULL, 10);
	if ( iPid == 0 ) {
		fprintf(stderr, "invalid pid\n");
		return 3;
	}

	if ( !SetConsoleCtrlHandler(NULL, TRUE) ) {
		fprintf(stderr, "SetConsoleCtrlHandler failed: %lu\n", (unsigned long)GetLastError());
		return 4;
	}
	FreeConsole();
	if ( !AttachConsole(iPid) ) {
		fprintf(stderr, "AttachConsole failed: %lu\n", (unsigned long)GetLastError());
		return 5;
	}

	if ( !GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, iPid) ) {
		fprintf(stderr, "GenerateConsoleCtrlEvent failed: %lu\n", (unsigned long)GetLastError());
		FreeConsole();
		return 6;
	}

	Sleep(200);
	FreeConsole();
	return 0;
}

int main(int argc, char** argv)
{
	if ( argc < 2 ) {
		procPrintUsage();
		return 1;
	}

	if ( strcmp(argv[1], "spawn") == 0 ) {
		return procSpawnChild(argc, argv);
	}
	if ( strcmp(argv[1], "signal") == 0 ) {
		return procSendSignal(argc, argv);
	}

	procPrintUsage();
	return 1;
}
