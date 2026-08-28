#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	typedef SOCKET xprobe_socket_t;
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <unistd.h>
	typedef int xprobe_socket_t;
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


static void procSocketClose(xprobe_socket_t hSocket)
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


int main(int argc, char** argv)
{
	const char* sHost;
	unsigned short iPort;
	xprobe_socket_t hSocket;
	struct sockaddr_in tAddr;

	if ( argc < 3 ) {
		fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
		return 2;
	}

	sHost = argv[1];
	iPort = (unsigned short)atoi(argv[2]);
	if ( sHost == NULL || sHost[0] == '\0' || iPort == 0 ) {
		fprintf(stderr, "invalid host/port\n");
		return 2;
	}

	if ( procSocketInit() != 0 ) {
		fprintf(stderr, "socket init failed\n");
		return 2;
	}

	hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( hSocket == INVALID_SOCKET ) {
		procSocketUnit();
		fprintf(stderr, "socket create failed\n");
		return 1;
	}

	memset(&tAddr, 0, sizeof(tAddr));
	tAddr.sin_family = AF_INET;
	tAddr.sin_port = htons(iPort);
	if ( inet_pton(AF_INET, sHost, &tAddr.sin_addr) != 1 ) {
		procSocketClose(hSocket);
		procSocketUnit();
		fprintf(stderr, "invalid host\n");
		return 2;
	}

	if ( connect(hSocket, (struct sockaddr*)&tAddr, sizeof(tAddr)) == SOCKET_ERROR ) {
		procSocketClose(hSocket);
		procSocketUnit();
		fprintf(stderr, "status=closed host=%s port=%u\n", sHost, (unsigned)iPort);
		return 1;
	}

	printf("status=open host=%s port=%u\n", sHost, (unsigned)iPort);
	procSocketClose(hSocket);
	procSocketUnit();
	return 0;
}
