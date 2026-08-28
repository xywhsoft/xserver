#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	typedef SOCKET idle_socket_t;
#else
	#include <arpa/inet.h>
	#include <errno.h>
	#include <netinet/in.h>
	#include <signal.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <time.h>
	#include <unistd.h>
	typedef int idle_socket_t;
	#ifndef INVALID_SOCKET
		#define INVALID_SOCKET (-1)
	#endif
	#ifndef SOCKET_ERROR
		#define SOCKET_ERROR (-1)
	#endif
#endif

#define XTP_HEADER_SIZE 32u


static int procSocketInit(void)
{
#ifdef _WIN32
	WSADATA tWsa;

	return WSAStartup(MAKEWORD(2, 2), &tWsa) == 0 ? 0 : -1;
#else
	signal(SIGPIPE, SIG_IGN);
	return 0;
#endif
}


static void procSocketUnit(void)
{
#ifdef _WIN32
	WSACleanup();
#endif
}


static void procSocketClose(idle_socket_t hSocket)
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


static int procSocketSetTimeout(idle_socket_t hSocket, int iTimeoutMS)
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


static idle_socket_t procSocketConnect(const char* sHost, unsigned short iPort)
{
	idle_socket_t hSocket;
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
	if ( procSocketSetTimeout(hSocket, 1200) != 0 ) {
		procSocketClose(hSocket);
		return INVALID_SOCKET;
	}

	return hSocket;
}


static int procSendAll(idle_socket_t hSocket, const unsigned char* pData, size_t iLen)
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


static int procRecvSome(idle_socket_t hSocket, unsigned char* pBuf, size_t iCap)
{
	return recv(hSocket, (char*)pBuf, (int)iCap, 0);
}


static int procRecvAll(idle_socket_t hSocket, unsigned char* pBuf, size_t iLen)
{
	size_t iRead = 0u;

	while ( iRead < iLen ) {
		int iRet = recv(hSocket, (char*)pBuf + iRead, (int)(iLen - iRead), 0);
		if ( iRet <= 0 ) {
			return -1;
		}
		iRead += (size_t)iRet;
	}

	return 0;
}


static void procSleepMS(int iWaitMS)
{
#ifdef _WIN32
	Sleep((DWORD)iWaitMS);
#else
	struct timespec tValue;

	tValue.tv_sec = iWaitMS / 1000;
	tValue.tv_nsec = (long)((iWaitMS % 1000) * 1000000L);
	nanosleep(&tValue, NULL);
#endif
}


static int procSocketLastError(void)
{
#ifdef _WIN32
	return (int)WSAGetLastError();
#else
	return errno;
#endif
}


static int procSocketTimeoutError(int iError)
{
#ifdef _WIN32
	return (iError == WSAETIMEDOUT) || (iError == WSAEWOULDBLOCK);
#else
	return (iError == EAGAIN) || (iError == EWOULDBLOCK) || (iError == ETIMEDOUT);
#endif
}


static void procStoreU16LE(unsigned char* pBuf, uint16_t iValue)
{
	pBuf[0] = (unsigned char)(iValue & 0xffu);
	pBuf[1] = (unsigned char)((iValue >> 8) & 0xffu);
}


static void procStoreU32LE(unsigned char* pBuf, uint32_t iValue)
{
	pBuf[0] = (unsigned char)(iValue & 0xffu);
	pBuf[1] = (unsigned char)((iValue >> 8) & 0xffu);
	pBuf[2] = (unsigned char)((iValue >> 16) & 0xffu);
	pBuf[3] = (unsigned char)((iValue >> 24) & 0xffu);
}


static void procStoreU64LE(unsigned char* pBuf, uint64_t iValue)
{
	int i;

	for ( i = 0; i < 8; i++ ) {
		pBuf[i] = (unsigned char)((iValue >> (i * 8)) & 0xffu);
	}
}


static uint32_t procLoadU32LE(const unsigned char* pBuf)
{
	return
		(uint32_t)pBuf[0] |
		((uint32_t)pBuf[1] << 8) |
		((uint32_t)pBuf[2] << 16) |
		((uint32_t)pBuf[3] << 24);
}


static int procDoWsHandshake(idle_socket_t hSocket, const char* sHost, unsigned short iPort)
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


static int procSendWsBadHandshake(idle_socket_t hSocket, const char* sHost, unsigned short iPort)
{
	char sReq[1024];

	snprintf(
		sReq,
		sizeof(sReq),
		"GET / HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"Sec-WebSocket-Protocol: xs-demo\r\n"
		"\r\n",
		sHost,
		(unsigned)iPort
	);

	return procSendAll(hSocket, (const unsigned char*)sReq, strlen(sReq));
}


static int procSendWsText(idle_socket_t hSocket, const char* sText, int bMask)
{
	size_t iTextLen = strlen(sText);
	unsigned char aMask[4] = { 0x12u, 0x34u, 0x56u, 0x78u };
	unsigned char aBuf[512];
	size_t i;
	size_t iOffset = 0u;

	if ( iTextLen > 120u ) {
		return -1;
	}

	aBuf[iOffset++] = 0x81u;
	aBuf[iOffset++] = (unsigned char)((bMask ? 0x80u : 0x00u) | (unsigned char)iTextLen);
	if ( bMask ) {
		memcpy(aBuf + iOffset, aMask, 4u);
		iOffset += 4u;

		for ( i = 0u; i < iTextLen; i++ ) {
			aBuf[iOffset++] = (unsigned char)(sText[i] ^ aMask[i % 4u]);
		}
	} else {
		memcpy(aBuf + iOffset, sText, iTextLen);
		iOffset += iTextLen;
	}

	return procSendAll(hSocket, aBuf, iOffset);
}


static int procSendWsMaskedText(idle_socket_t hSocket, const char* sText)
{
	return procSendWsText(hSocket, sText, 1);
}


static int procSendWsBadFrame(idle_socket_t hSocket, const char* sHost, unsigned short iPort)
{
	if ( procDoWsHandshake(hSocket, sHost, iPort) != 0 ) {
		return -1;
	}

	return procSendWsText(hSocket, "bad-frame", 0);
}


static int procBuildXtpRequest(const char* sCmd, const char* sTag, unsigned char** ppBuf, size_t* piPackSize)
{
	size_t iCmdSize;
	size_t iTagSize;
	size_t iPackSize;
	size_t iOffset;
	unsigned char* pBuf;

	if ( sCmd == NULL || sTag == NULL || ppBuf == NULL || piPackSize == NULL ) {
		return -1;
	}

	iCmdSize = strlen(sCmd);
	iTagSize = strlen(sTag);
	iPackSize = XTP_HEADER_SIZE + 4u + iCmdSize + 3u + iTagSize;

	pBuf = (unsigned char*)calloc(1u, iPackSize);
	if ( pBuf == NULL ) {
		return -1;
	}

	memcpy(pBuf, "xtp\2", 4u);
	procStoreU32LE(pBuf + 4u, (uint32_t)iPackSize);
	procStoreU64LE(pBuf + 8u, 1u);
	procStoreU16LE(pBuf + 16u, 0u);
	procStoreU16LE(pBuf + 18u, 1u);
	procStoreU16LE(pBuf + 20u, (uint16_t)iCmdSize);
	procStoreU16LE(pBuf + 22u, 1u);
	procStoreU32LE(pBuf + 24u, 0u);
	procStoreU32LE(pBuf + 28u, 0u);

	iOffset = XTP_HEADER_SIZE;
	procStoreU16LE(pBuf + iOffset, 3u);
	procStoreU16LE(pBuf + iOffset + 2u, (uint16_t)iTagSize);
	iOffset += 4u;

	memcpy(pBuf + iOffset, sCmd, iCmdSize);
	iOffset += iCmdSize;

	memcpy(pBuf + iOffset, "tag", 3u);
	iOffset += 3u;

	memcpy(pBuf + iOffset, sTag, iTagSize);

	*ppBuf = pBuf;
	*piPackSize = iPackSize;
	return 0;
}


static int procDrainXtpResponse(idle_socket_t hSocket)
{
	unsigned char aHeader[XTP_HEADER_SIZE];
	unsigned char* pTail = NULL;
	uint32_t iPackSize;
	int iRet = -1;

	if ( procRecvAll(hSocket, aHeader, sizeof(aHeader)) != 0 ) {
		return -1;
	}
	if ( memcmp(aHeader, "xtp\2", 4u) != 0 ) {
		return -1;
	}

	iPackSize = procLoadU32LE(aHeader + 4u);
	if ( iPackSize < XTP_HEADER_SIZE ) {
		return -1;
	}
	if ( iPackSize == XTP_HEADER_SIZE ) {
		return 0;
	}

	pTail = (unsigned char*)malloc(iPackSize - XTP_HEADER_SIZE);
	if ( pTail == NULL ) {
		return -1;
	}
	if ( procRecvAll(hSocket, pTail, iPackSize - XTP_HEADER_SIZE) != 0 ) {
		goto end;
	}

	iRet = 0;

end:
	free(pTail);
	return iRet;
}


static int procSendXtpKick(idle_socket_t hSocket, const char* sTag)
{
	unsigned char* pReqBuf = NULL;
	size_t iReqSize = 0u;
	int iRet = -1;

	if ( procBuildXtpRequest("demo.callself", sTag, &pReqBuf, &iReqSize) != 0 ) {
		return -1;
	}
	if ( procSendAll(hSocket, pReqBuf, iReqSize) != 0 ) {
		goto end;
	}
	if ( procDrainXtpResponse(hSocket) != 0 ) {
		goto end;
	}

	iRet = 0;

end:
	free(pReqBuf);
	return iRet;
}


static int procSendXtpBadHeader(idle_socket_t hSocket)
{
	unsigned char aBuf[XTP_HEADER_SIZE];

	memset(aBuf, 0, sizeof(aBuf));
	memcpy(aBuf, "bad\2", 4u);
	procStoreU32LE(aBuf + 4u, XTP_HEADER_SIZE);
	procStoreU64LE(aBuf + 8u, 1u);

	return procSendAll(hSocket, aBuf, sizeof(aBuf));
}


static int procSendXtpBadSize(idle_socket_t hSocket)
{
	unsigned char aBuf[XTP_HEADER_SIZE];

	memset(aBuf, 0, sizeof(aBuf));
	memcpy(aBuf, "xtp\2", 4u);
	procStoreU32LE(aBuf + 4u, XTP_HEADER_SIZE);
	procStoreU64LE(aBuf + 8u, 1u);
	procStoreU16LE(aBuf + 18u, 1u);
	procStoreU32LE(aBuf + 24u, 1u);

	return procSendAll(hSocket, aBuf, sizeof(aBuf));
}


static int procSendXtpBadType(idle_socket_t hSocket)
{
	unsigned char aBuf[XTP_HEADER_SIZE];

	memset(aBuf, 0, sizeof(aBuf));
	memcpy(aBuf, "xtp\2", 4u);
	procStoreU32LE(aBuf + 4u, XTP_HEADER_SIZE);
	procStoreU64LE(aBuf + 8u, 1u);
	procStoreU16LE(aBuf + 18u, 99u);

	return procSendAll(hSocket, aBuf, sizeof(aBuf));
}


static int procSendMode(idle_socket_t hSocket, const char* sHost, unsigned short iPort, const char* sMode)
{
	static const char sHttpPartial[] =
		"GET /json HTTP/1.1\r\n"
		"Host: 127.0.0.1\r\n"
		"Connection: keep-alive\r\n";

	if ( sMode == NULL || sMode[0] == '\0' || strcmp(sMode, "none") == 0 ) {
		return 0;
	}

	if ( strcmp(sMode, "http_partial") == 0 ) {
		return procSendAll(hSocket, (const unsigned char*)sHttpPartial, strlen(sHttpPartial));
	}
	if ( strcmp(sMode, "ws_handshake") == 0 ) {
		return procDoWsHandshake(hSocket, sHost, iPort);
	}
	if ( strcmp(sMode, "ws_bad_handshake") == 0 ) {
		return procSendWsBadHandshake(hSocket, sHost, iPort);
	}
	if ( strcmp(sMode, "ws_bad_frame") == 0 ) {
		return procSendWsBadFrame(hSocket, sHost, iPort);
	}
	if ( strcmp(sMode, "xtp_ping") == 0 ) {
		return procSendXtpKick(hSocket, "idle-check");
	}
	if ( strcmp(sMode, "xtp_bad_header") == 0 ) {
		return procSendXtpBadHeader(hSocket);
	}
	if ( strcmp(sMode, "xtp_bad_size") == 0 ) {
		return procSendXtpBadSize(hSocket);
	}
	if ( strcmp(sMode, "xtp_bad_type") == 0 ) {
		return procSendXtpBadType(hSocket);
	}

	return -1;
}


static int procProbeOpen(idle_socket_t hSocket, const char* sMode)
{
	if ( sMode && strcmp(sMode, "ws_handshake") == 0 ) {
		return procSendWsMaskedText(hSocket, "idle-probe");
	}
	if ( sMode && strcmp(sMode, "xtp_ping") == 0 ) {
		return procSendXtpKick(hSocket, "idle-probe");
	}

	return send(hSocket, "?", 1, 0);
}


int main(int argc, char** argv)
{
	const char* sHost;
	unsigned short iPort;
	int iWaitMS;
	const char* sMode = "none";
	idle_socket_t hSocket = INVALID_SOCKET;
	unsigned char aBuf[256];
	int iRead;
	int iError;
	int iProbeRet;
	int iRet = 1;

	if ( argc < 4 ) {
		fprintf(stderr, "usage: %s <host> <port> <wait_ms> [mode]\n", argv[0]);
		return 2;
	}

	sHost = argv[1];
	iPort = (unsigned short)strtoul(argv[2], NULL, 10);
	iWaitMS = atoi(argv[3]);
	if ( argc >= 5 ) {
		sMode = argv[4];
	}

	if ( iPort == 0u || iWaitMS < 0 ) {
		fprintf(stderr, "invalid arguments\n");
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

	if ( procSendMode(hSocket, sHost, iPort, sMode) != 0 ) {
		fprintf(stderr, "send mode failed\n");
		goto end;
	}

	procSleepMS(iWaitMS);

	iRead = recv(hSocket, (char*)aBuf, (int)sizeof(aBuf), 0);
	if ( iRead == 0 ) {
		printf("status=closed\nphase=recv_eof\nmode=%s\nport=%u\n", sMode, (unsigned)iPort);
		iRet = 0;
		goto end;
	}

	if ( iRead > 0 ) {
		if ( (strcmp(sMode, "ws_handshake") == 0 || strcmp(sMode, "ws_bad_frame") == 0) && ((aBuf[0] & 0x0fu) == 0x8u) ) {
			printf("status=closed\nphase=ws_close_frame\nmode=%s\nport=%u\nbytes=%d\n", sMode, (unsigned)iPort, iRead);
			iRet = 0;
			goto end;
		}
		if ( strcmp(sMode, "ws_bad_handshake") == 0 ) {
			aBuf[(iRead < (int)sizeof(aBuf)) ? iRead : ((int)sizeof(aBuf) - 1)] = '\0';
			if ( strstr((const char*)aBuf, "HTTP/1.1 ") != NULL && strstr((const char*)aBuf, " 101 ") == NULL ) {
				printf("status=closed\nphase=ws_http_error\nmode=%s\nport=%u\nbytes=%d\n", sMode, (unsigned)iPort, iRead);
				iRet = 0;
				goto end;
			}
		}

		printf("status=open\nphase=recv_data\nmode=%s\nport=%u\nbytes=%d\n", sMode, (unsigned)iPort, iRead);
		iRet = 1;
		goto end;
	}

	iError = procSocketLastError();
	if ( !procSocketTimeoutError(iError) ) {
		printf("status=closed\nphase=recv_error\nmode=%s\nport=%u\nerror=%d\n", sMode, (unsigned)iPort, iError);
		iRet = 0;
		goto end;
	}

	iProbeRet = procProbeOpen(hSocket, sMode);
	if ( iProbeRet <= 0 ) {
		iError = procSocketLastError();
		printf("status=closed\nphase=send_probe_failed\nmode=%s\nport=%u\nerror=%d\n", sMode, (unsigned)iPort, iError);
		iRet = 0;
		goto end;
	}

	printf("status=open\nphase=send_probe_ok\nmode=%s\nport=%u\n", sMode, (unsigned)iPort);
	iRet = 1;

end:
	procSocketClose(hSocket);
	procSocketUnit();
	return iRet;
}
