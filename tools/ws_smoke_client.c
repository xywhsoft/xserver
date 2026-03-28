#include <stdint.h>
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


static smoke_socket_t procSocketConnect(const char* sHost, unsigned short iPort)
{
	smoke_socket_t hSocket;
	struct sockaddr_in tAddr;

	hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( hSocket == INVALID_SOCKET ) {
		return INVALID_SOCKET;
	}

	memset(&tAddr, 0, sizeof(tAddr));
	tAddr.sin_family = AF_INET;
	tAddr.sin_port = htons(iPort);
	if ( inet_pton(AF_INET, sHost, &tAddr.sin_addr) != 1 ) {
		procSocketClose(hSocket);
		return INVALID_SOCKET;
	}
	if ( connect(hSocket, (struct sockaddr*)&tAddr, sizeof(tAddr)) == SOCKET_ERROR ) {
		procSocketClose(hSocket);
		return INVALID_SOCKET;
	}
	if ( procSocketSetTimeout(hSocket, 5000) != 0 ) {
		procSocketClose(hSocket);
		return INVALID_SOCKET;
	}

	return hSocket;
}


static int procSendAll(smoke_socket_t hSocket, const unsigned char* pData, size_t iLen)
{
	size_t iSent = 0;

	while ( iSent < iLen ) {
		int iRet = send(hSocket, (const char*)pData + iSent, (int)(iLen - iSent), 0);
		if ( iRet <= 0 ) {
			return -1;
		}
		iSent += (size_t)iRet;
	}

	return 0;
}


static int procRecvSome(smoke_socket_t hSocket, unsigned char* pBuf, size_t iCap)
{
	int iRet = recv(hSocket, (char*)pBuf, (int)iCap, 0);
	return iRet;
}


static int procDoHandshake(smoke_socket_t hSocket, const char* sHost, unsigned short iPort)
{
	char sReq[1024];
	unsigned char aBuf[2048];
	int iRead;

	snprintf(
		sReq,
		sizeof(sReq),
		"GET / HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
		"Sec-WebSocket-Protocol: xs-demo\r\n"
		"\r\n",
		sHost,
		(unsigned)iPort
	);
	if ( procSendAll(hSocket, (const unsigned char*)sReq, strlen(sReq)) != 0 ) {
		return -1;
	}

	iRead = procRecvSome(hSocket, aBuf, sizeof(aBuf) - 1u);
	if ( iRead <= 0 ) {
		return -1;
	}
	aBuf[iRead] = '\0';

	if ( strstr((const char*)aBuf, " 101 ") == NULL ) {
		return -1;
	}
	if ( strstr((const char*)aBuf, "Sec-WebSocket-Protocol: xs-demo") == NULL &&
		strstr((const char*)aBuf, "sec-websocket-protocol: xs-demo") == NULL ) {
		return -1;
	}

	return 0;
}


static int procSendMaskedText(smoke_socket_t hSocket, const char* sText)
{
	size_t iTextLen = strlen(sText);
	unsigned char aMask[4] = { 0x12u, 0x34u, 0x56u, 0x78u };
	unsigned char aBuf[512];
	size_t i;
	size_t iOffset = 0;

	if ( iTextLen > 120u ) {
		return -1;
	}

	aBuf[iOffset++] = 0x81u;
	aBuf[iOffset++] = (unsigned char)(0x80u | (unsigned char)iTextLen);
	memcpy(aBuf + iOffset, aMask, 4u);
	iOffset += 4u;

	for ( i = 0; i < iTextLen; i++ ) {
		aBuf[iOffset++] = (unsigned char)(sText[i] ^ aMask[i % 4u]);
	}

	return procSendAll(hSocket, aBuf, iOffset);
}


static int procReadText(smoke_socket_t hSocket, char* sBuf, size_t iCap)
{
	unsigned char aHead[2];
	unsigned char aLen16[2];
	unsigned char aLen64[8];
	uint64_t iLen = 0;
	size_t iNeed;

	if ( procRecvSome(hSocket, aHead, 2u) != 2 ) {
		return -1;
	}
	if ( (aHead[0] & 0x0fu) != 0x1u ) {
		return -1;
	}
	if ( (aHead[1] & 0x80u) != 0u ) {
		return -1;
	}

	iLen = (uint64_t)(aHead[1] & 0x7fu);
	if ( iLen == 126u ) {
		if ( procRecvSome(hSocket, aLen16, 2u) != 2 ) {
			return -1;
		}
		iLen = ((uint64_t)aLen16[0] << 8) | (uint64_t)aLen16[1];
	} else if ( iLen == 127u ) {
		int i;
		if ( procRecvSome(hSocket, aLen64, 8u) != 8 ) {
			return -1;
		}
		iLen = 0u;
		for ( i = 0; i < 8; i++ ) {
			iLen = (iLen << 8) | (uint64_t)aLen64[i];
		}
	}

	if ( iLen + 1u > iCap ) {
		return -1;
	}

	iNeed = (size_t)iLen;
	if ( procRecvSome(hSocket, (unsigned char*)sBuf, iNeed) != (int)iNeed ) {
		return -1;
	}
	sBuf[iNeed] = '\0';
	return 0;
}


int main(int argc, char** argv)
{
	const char* sHost;
	unsigned short iPort;
	const char* sText;
	smoke_socket_t hSocket = INVALID_SOCKET;
	char sResp[2048];
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

	hSocket = procSocketConnect(sHost, iPort);
	if ( hSocket == INVALID_SOCKET ) {
		fprintf(stderr, "connect failed\n");
		goto end;
	}
	if ( procDoHandshake(hSocket, sHost, iPort) != 0 ) {
		fprintf(stderr, "handshake failed\n");
		goto end;
	}
	if ( procSendMaskedText(hSocket, sText) != 0 ) {
		fprintf(stderr, "send frame failed\n");
		goto end;
	}
	if ( procReadText(hSocket, sResp, sizeof(sResp)) != 0 ) {
		fprintf(stderr, "read frame failed\n");
		goto end;
	}

	printf("%s", sResp);
	if ( sResp[0] && sResp[strlen(sResp) - 1] != '\n' ) {
		printf("\n");
	}

	iRet = 0;

end:
	procSocketClose(hSocket);
	procSocketUnit();
	return iRet;
}
