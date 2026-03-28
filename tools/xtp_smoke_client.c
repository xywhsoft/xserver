#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	typedef SOCKET xtp_socket_t;
#else
	#include <arpa/inet.h>
	#include <errno.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <unistd.h>
	typedef int xtp_socket_t;
	#ifndef INVALID_SOCKET
		#define INVALID_SOCKET (-1)
	#endif
	#ifndef SOCKET_ERROR
		#define SOCKET_ERROR (-1)
	#endif
#endif

#define XTP_HEADER_SIZE 32u

typedef struct
{
	const char* sKey;
	const char* sValue;
	uint16_t iKeySize;
	uint16_t iValueSize;
} XTP_ParamPair;


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


static uint16_t procLoadU16LE(const unsigned char* pBuf)
{
	return (uint16_t)((uint16_t)pBuf[0] | ((uint16_t)pBuf[1] << 8));
}


static uint32_t procLoadU32LE(const unsigned char* pBuf)
{
	return
		(uint32_t)pBuf[0] |
		((uint32_t)pBuf[1] << 8) |
		((uint32_t)pBuf[2] << 16) |
		((uint32_t)pBuf[3] << 24);
}


static int32_t procLoadI32LE(const unsigned char* pBuf)
{
	return (int32_t)procLoadU32LE(pBuf);
}


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


static void procSocketClose(xtp_socket_t hSocket)
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


static int procSocketSetTimeout(xtp_socket_t hSocket, int iTimeoutMS)
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


static xtp_socket_t procSocketConnect(const char* sHost, unsigned short iPort)
{
	xtp_socket_t hSocket;
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


static int procSocketSendAll(xtp_socket_t hSocket, const unsigned char* pData, size_t iLen)
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


static int procSocketRecvAll(xtp_socket_t hSocket, unsigned char* pData, size_t iLen)
{
	size_t iRead = 0;

	while ( iRead < iLen ) {
		int iRet = recv(hSocket, (char*)pData + iRead, (int)(iLen - iRead), 0);
		if ( iRet <= 0 ) {
			return -1;
		}
		iRead += (size_t)iRet;
	}

	return 0;
}


static int procParseParam(const char* sArg, XTP_ParamPair* pPair)
{
	const char* sEqual;
	size_t iKeySize;
	size_t iValueSize;

	if ( sArg == NULL || pPair == NULL ) {
		return -1;
	}

	sEqual = strchr(sArg, '=');
	if ( sEqual == NULL || sEqual == sArg ) {
		return -1;
	}

	iKeySize = (size_t)(sEqual - sArg);
	iValueSize = strlen(sEqual + 1);
	if ( iKeySize > 65535u || iValueSize > 65535u ) {
		return -1;
	}

	pPair->sKey = sArg;
	pPair->sValue = sEqual + 1;
	pPair->iKeySize = (uint16_t)iKeySize;
	pPair->iValueSize = (uint16_t)iValueSize;
	return 0;
}


static unsigned char* procBuildRequest(const char* sCmd, XTP_ParamPair* arrPair, int iParamCount, size_t* piPackSize)
{
	size_t iCmdSize;
	size_t iPackSize;
	size_t iOffset;
	int i;
	unsigned char* pBuf;

	if ( sCmd == NULL || piPackSize == NULL ) {
		return NULL;
	}

	iCmdSize = strlen(sCmd);
	iPackSize = XTP_HEADER_SIZE + ((size_t)iParamCount * 4u) + iCmdSize;
	for ( i = 0; i < iParamCount; i++ ) {
		iPackSize += (size_t)arrPair[i].iKeySize + (size_t)arrPair[i].iValueSize;
	}

	pBuf = (unsigned char*)calloc(1u, iPackSize);
	if ( pBuf == NULL ) {
		return NULL;
	}

	memcpy(pBuf, "xtp\2", 4u);
	procStoreU32LE(pBuf + 4u, (uint32_t)iPackSize);
	procStoreU64LE(pBuf + 8u, 1u);
	procStoreU16LE(pBuf + 16u, 0u);
	procStoreU16LE(pBuf + 18u, 1u);
	procStoreU16LE(pBuf + 20u, (uint16_t)iCmdSize);
	procStoreU16LE(pBuf + 22u, (uint16_t)iParamCount);
	procStoreU32LE(pBuf + 24u, 0u);
	procStoreU32LE(pBuf + 28u, 0u);

	iOffset = XTP_HEADER_SIZE;
	for ( i = 0; i < iParamCount; i++ ) {
		procStoreU16LE(pBuf + iOffset, arrPair[i].iKeySize);
		procStoreU16LE(pBuf + iOffset + 2u, arrPair[i].iValueSize);
		iOffset += 4u;
	}

	memcpy(pBuf + iOffset, sCmd, iCmdSize);
	iOffset += iCmdSize;

	for ( i = 0; i < iParamCount; i++ ) {
		memcpy(pBuf + iOffset, arrPair[i].sKey, arrPair[i].iKeySize);
		iOffset += arrPair[i].iKeySize;
		memcpy(pBuf + iOffset, arrPair[i].sValue, arrPair[i].iValueSize);
		iOffset += arrPair[i].iValueSize;
	}

	*piPackSize = iPackSize;
	return pBuf;
}


static int procReadResponse(xtp_socket_t hSocket, char** psCmd, int32_t* piStatus, char** psBody)
{
	unsigned char aHeader[XTP_HEADER_SIZE];
	unsigned char* pPack = NULL;
	uint32_t iPackSize;
	uint16_t iCmdSize;
	uint16_t iParamCount;
	uint32_t iBodySize;
	size_t iOffset;
	int i;

	if ( psCmd ) {
		*psCmd = NULL;
	}
	if ( piStatus ) {
		*piStatus = -1;
	}
	if ( psBody ) {
		*psBody = NULL;
	}

	if ( procSocketRecvAll(hSocket, aHeader, sizeof(aHeader)) != 0 ) {
		return -1;
	}
	if ( memcmp(aHeader, "xtp\2", 4u) != 0 ) {
		return -1;
	}

	iPackSize = procLoadU32LE(aHeader + 4u);
	iCmdSize = procLoadU16LE(aHeader + 20u);
	iParamCount = procLoadU16LE(aHeader + 22u);
	iBodySize = procLoadU32LE(aHeader + 24u);
	if ( iPackSize < XTP_HEADER_SIZE ) {
		return -1;
	}

	pPack = (unsigned char*)malloc(iPackSize);
	if ( pPack == NULL ) {
		return -1;
	}

	memcpy(pPack, aHeader, XTP_HEADER_SIZE);
	if ( iPackSize > XTP_HEADER_SIZE ) {
		if ( procSocketRecvAll(hSocket, pPack + XTP_HEADER_SIZE, iPackSize - XTP_HEADER_SIZE) != 0 ) {
			free(pPack);
			return -1;
		}
	}

	iOffset = XTP_HEADER_SIZE;
	for ( i = 0; i < (int)iParamCount; i++ ) {
		uint16_t iKeySize = procLoadU16LE(pPack + iOffset);
		uint16_t iValueSize = procLoadU16LE(pPack + iOffset + 2u);
		iOffset += 4u;
		iOffset += (size_t)iKeySize + (size_t)iValueSize;
	}
	if ( iOffset < XTP_HEADER_SIZE + ((size_t)iParamCount * 4u) ) {
		free(pPack);
		return -1;
	}

	iOffset = XTP_HEADER_SIZE + ((size_t)iParamCount * 4u);
	if ( iOffset + (size_t)iCmdSize > iPackSize ) {
		free(pPack);
		return -1;
	}

	if ( psCmd ) {
		*psCmd = (char*)calloc(1u, (size_t)iCmdSize + 1u);
		if ( *psCmd == NULL ) {
			free(pPack);
			return -1;
		}
		memcpy(*psCmd, pPack + iOffset, iCmdSize);
	}
	iOffset += (size_t)iCmdSize;

	for ( i = 0; i < (int)iParamCount; i++ ) {
		size_t iInfoOffset = XTP_HEADER_SIZE + ((size_t)i * 4u);
		uint16_t iKeySize = procLoadU16LE(pPack + iInfoOffset);
		uint16_t iValueSize = procLoadU16LE(pPack + iInfoOffset + 2u);

		if ( iOffset + (size_t)iKeySize + (size_t)iValueSize > iPackSize ) {
			if ( psCmd && *psCmd ) {
				free(*psCmd);
				*psCmd = NULL;
			}
			free(pPack);
			return -1;
		}
		iOffset += (size_t)iKeySize + (size_t)iValueSize;
	}

	if ( iOffset + (size_t)iBodySize > iPackSize ) {
		if ( psCmd && *psCmd ) {
			free(*psCmd);
			*psCmd = NULL;
		}
		free(pPack);
		return -1;
	}

	if ( piStatus ) {
		*piStatus = procLoadI32LE(pPack + 28u);
	}
	if ( psBody ) {
		*psBody = (char*)calloc(1u, (size_t)iBodySize + 1u);
		if ( *psBody == NULL ) {
			if ( psCmd && *psCmd ) {
				free(*psCmd);
				*psCmd = NULL;
			}
			free(pPack);
			return -1;
		}
		memcpy(*psBody, pPack + iOffset, iBodySize);
	}

	free(pPack);
	return 0;
}


int main(int argc, char** argv)
{
	const char* sHost;
	unsigned short iPort;
	const char* sCmd;
	XTP_ParamPair arrPair[16];
	int iParamCount = 0;
	int i;
	int iRet = 1;
	xtp_socket_t hSocket = INVALID_SOCKET;
	unsigned char* pReqBuf = NULL;
	size_t iReqSize = 0u;
	char* sRespCmd = NULL;
	char* sRespBody = NULL;
	int32_t iStatus = -1;

	if ( argc < 4 ) {
		fprintf(stderr, "usage: %s <host> <port> <cmd> [key=value ...]\n", argv[0]);
		return 2;
	}

	sHost = argv[1];
	iPort = (unsigned short)strtoul(argv[2], NULL, 10);
	sCmd = argv[3];
	if ( iPort == 0u || sCmd == NULL || sCmd[0] == '\0' ) {
		fprintf(stderr, "invalid host/port/cmd\n");
		return 2;
	}

	for ( i = 4; i < argc; i++ ) {
		if ( iParamCount >= (int)(sizeof(arrPair) / sizeof(arrPair[0])) ) {
			fprintf(stderr, "too many params\n");
			return 2;
		}
		if ( procParseParam(argv[i], &arrPair[iParamCount]) != 0 ) {
			fprintf(stderr, "invalid param: %s\n", argv[i]);
			return 2;
		}
		iParamCount++;
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

	pReqBuf = procBuildRequest(sCmd, arrPair, iParamCount, &iReqSize);
	if ( pReqBuf == NULL ) {
		fprintf(stderr, "build request failed\n");
		goto end;
	}

	if ( procSocketSendAll(hSocket, pReqBuf, iReqSize) != 0 ) {
		fprintf(stderr, "send failed\n");
		goto end;
	}

	if ( procReadResponse(hSocket, &sRespCmd, &iStatus, &sRespBody) != 0 ) {
		fprintf(stderr, "recv/parse response failed\n");
		goto end;
	}

	printf("status=%d\n", (int)iStatus);
	printf("cmd=%s\n", sRespCmd ? sRespCmd : "");
	printf("body=\n%s", sRespBody ? sRespBody : "");
	if ( sRespBody && sRespBody[0] && sRespBody[strlen(sRespBody) - 1] != '\n' ) {
		printf("\n");
	}

	iRet = 0;

end:
	if ( sRespBody ) {
		free(sRespBody);
	}
	if ( sRespCmd ) {
		free(sRespCmd);
	}
	if ( pReqBuf ) {
		free(pReqBuf);
	}
	procSocketClose(hSocket);
	procSocketUnit();
	return iRet;
}
