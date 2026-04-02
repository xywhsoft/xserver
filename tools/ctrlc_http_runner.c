#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>
#else
	#error This helper currently supports Windows only.
#endif


static int SendHttpGet(const char* sHost, unsigned short iPort, const char* sPath)
{
	SOCKET hSock = INVALID_SOCKET;
	struct sockaddr_in tAddr;
	char sRequest[1024];
	char sRecv[256];
	int iOk = 0;

	hSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( hSock == INVALID_SOCKET ) {
		return 0;
	}

	memset(&tAddr, 0, sizeof(tAddr));
	tAddr.sin_family = AF_INET;
	tAddr.sin_port = htons(iPort);
	tAddr.sin_addr.s_addr = inet_addr(sHost);
	if ( connect(hSock, (struct sockaddr*)&tAddr, sizeof(tAddr)) == SOCKET_ERROR ) {
		closesocket(hSock);
		return 0;
	}

	snprintf(
		sRequest,
		sizeof(sRequest),
		"GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
		sPath,
		sHost
	);
	if ( send(hSock, sRequest, (int)strlen(sRequest), 0) > 0 ) {
		int iRecv = recv(hSock, sRecv, sizeof(sRecv) - 1, 0);
		if ( iRecv > 0 ) {
			sRecv[iRecv] = '\0';
			if ( strstr(sRecv, " 200 ") != NULL ) {
				iOk = 1;
			}
		}
	}

	closesocket(hSock);
	return iOk;
}


int main(int argc, char** argv)
{
	STARTUPINFOA tSi;
	PROCESS_INFORMATION tPi;
	WSADATA tWsa;
	char sCmd[2048];
	DWORD iWaitMs = 3000;
	DWORD iRequestTimeoutMs = 15000;
	DWORD iStartTick;
	const char* sExe;
	const char* sConfig;
	const char* sWorkDir;
	const char* sHost;
	const char* sPath;
	unsigned short iPort;
	int iExitCode = 1;

	if ( argc < 7 ) {
		fprintf(stderr, "usage: ctrlc_http_runner <exe> <config> <workdir> <host> <port> <path> [wait_ms]\n");
		return 2;
	}

	sExe = argv[1];
	sConfig = argv[2];
	sWorkDir = argv[3];
	sHost = argv[4];
	iPort = (unsigned short)atoi(argv[5]);
	sPath = argv[6];
	if ( argc > 7 ) {
		iWaitMs = (DWORD)atoi(argv[7]);
	}

	if ( WSAStartup(MAKEWORD(2, 2), &tWsa) != 0 ) {
		fprintf(stderr, "WSAStartup failed\n");
		return 3;
	}

	memset(&tSi, 0, sizeof(tSi));
	memset(&tPi, 0, sizeof(tPi));
	tSi.cb = sizeof(tSi);

	snprintf(sCmd, sizeof(sCmd), "\"%s\" \"%s\"", sExe, sConfig);
	if ( !CreateProcessA(
		NULL,
		sCmd,
		NULL,
		NULL,
		TRUE,
		CREATE_NEW_PROCESS_GROUP,
		NULL,
		sWorkDir,
		&tSi,
		&tPi
	) ) {
		fprintf(stderr, "CreateProcess failed: %lu\n", (unsigned long)GetLastError());
		WSACleanup();
		return 4;
	}

	iStartTick = GetTickCount();
	while ( (GetTickCount() - iStartTick) < iRequestTimeoutMs ) {
		DWORD iWait = WaitForSingleObject(tPi.hProcess, 100);
		if ( iWait == WAIT_OBJECT_0 ) {
			fprintf(stderr, "child exited before request\n");
			goto Exit;
		}
		if ( SendHttpGet(sHost, iPort, sPath) ) {
			break;
		}
	}

	Sleep(iWaitMs);
	SetConsoleCtrlHandler(NULL, TRUE);
	if ( !GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, tPi.dwProcessId) ) {
		fprintf(stderr, "GenerateConsoleCtrlEvent failed: %lu\n", (unsigned long)GetLastError());
	}

	if ( WaitForSingleObject(tPi.hProcess, 10000) == WAIT_TIMEOUT ) {
		TerminateProcess(tPi.hProcess, 1);
		WaitForSingleObject(tPi.hProcess, 5000);
	}

	iExitCode = 0;

Exit:
	CloseHandle(tPi.hThread);
	CloseHandle(tPi.hProcess);
	WSACleanup();
	return iExitCode;
}
