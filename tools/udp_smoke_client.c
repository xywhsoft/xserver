#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	typedef SOCKET smoke_socket_t;
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <unistd.h>
	typedef int smoke_socket_t;
	#ifndef INVALID_SOCKET
		#define INVALID_SOCKET (-1)
	#endif
	#ifndef SOCKET_ERROR
		#define SOCKET_ERROR (-1)
	#endif
#endif


static int procSocketInit(void)
{
#ifdef _WIN32
	WSADATA tWsa;

	return WSAStartup(MAKEWORD(2, 2), &tWsa) == 0 ? 0 : -1;
#else
	return 0;
#endif
}


static void procSocketUnit(void)
{
#ifdef _WIN32
	WSACleanup();
#endif
}


static void procSocketClose(smoke_socket_t hSocket)
{
	if ( hSocket == INVALID_SOCKET ) {
		return;
	}
#ifdef _WIN32
	closesocket(hSocket);
#else
	close(hSocket);
#endif
}


static int procSocketSetTimeout(smoke_socket_t hSocket, int iTimeoutMS)
{
#ifdef _WIN32
	DWORD iValue = (DWORD)iTimeoutMS;
	if ( setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&iValue, sizeof(iValue)) != 0 ) {
		return -1;
	}
	if ( setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&iValue, sizeof(iValue)) != 0 ) {
		return -1;
	}
#else
	struct timeval tValue;

	tValue.tv_sec = iTimeoutMS / 1000;
	tValue.tv_usec = (iTimeoutMS % 1000) * 1000;
	if ( setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, &tValue, sizeof(tValue)) != 0 ) {
		return -1;
	}
	if ( setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, &tValue, sizeof(tValue)) != 0 ) {
		return -1;
	}
#endif
	return 0;
}


static smoke_socket_t procSocketOpen(void)
{
	smoke_socket_t hSocket;

	hSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if ( hSocket == INVALID_SOCKET ) {
		return INVALID_SOCKET;
	}
	if ( procSocketSetTimeout(hSocket, 5000) != 0 ) {
		procSocketClose(hSocket);
		return INVALID_SOCKET;
	}

	return hSocket;
}


int main(int argc, char** argv)
{
	const char* sHost;
	unsigned short iPort;
	const char* sText;
	smoke_socket_t hSocket = INVALID_SOCKET;
	struct sockaddr_in tAddr;
	char sBuf[2048];
	int iRead;
	int iRet = 1;

	if ( argc < 4 ) {
		fprintf(stderr, "usage: %s <host> <port> <text>\n", argv[0]);
		return 2;
	}

	sHost = argv[1];
	iPort = (unsigned short)strtoul(argv[2], NULL, 10);
	sText = argv[3];
	if ( iPort == 0u ) {
		fprintf(stderr, "invalid port\n");
		return 2;
	}

	if ( procSocketInit() != 0 ) {
		fprintf(stderr, "socket init failed\n");
		return 1;
	}

	hSocket = procSocketOpen();
	if ( hSocket == INVALID_SOCKET ) {
		fprintf(stderr, "socket open failed\n");
		goto end;
	}

	memset(&tAddr, 0, sizeof(tAddr));
	tAddr.sin_family = AF_INET;
	tAddr.sin_port = htons(iPort);
	if ( inet_pton(AF_INET, sHost, &tAddr.sin_addr) != 1 ) {
		fprintf(stderr, "invalid host\n");
		goto end;
	}

	if ( sendto(hSocket, sText, (int)strlen(sText), 0, (const struct sockaddr*)&tAddr, sizeof(tAddr)) == SOCKET_ERROR ) {
		fprintf(stderr, "sendto failed\n");
		goto end;
	}

	iRead = recvfrom(hSocket, sBuf, (int)(sizeof(sBuf) - 1u), 0, NULL, NULL);
	if ( iRead <= 0 ) {
		fprintf(stderr, "recvfrom failed\n");
		goto end;
	}

	sBuf[iRead] = '\0';
	printf("%s", sBuf);
	if ( sBuf[iRead - 1] != '\n' ) {
		printf("\n");
	}

	iRet = 0;

end:
	procSocketClose(hSocket);
	procSocketUnit();
	return iRet;
}