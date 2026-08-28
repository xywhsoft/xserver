#include <xsbase.h>
#include <string.h>

bool EventDgramProc(XS_ServerObject objServer, void* pSock, const void* pFromAddr, const void* pData, size_t iLen)
{
	char sBuf[512];
	const char* sFrom;
	int iWrite;
	
	sFrom = xsAddrText(pFromAddr);
	iWrite = snprintf(
		sBuf,
		sizeof(sBuf),
		"udp demo\nserver=%s\nfrom=%s\nbytes=%u\ndata=%.*s\n",
		xsServerName(objServer),
		sFrom ? sFrom : "",
		(unsigned)iLen,
		(int)iLen,
		pData ? (const char*)pData : ""
	);
	if ( iWrite < 0 ) {
		return false;
	}
	
	return xsDgramReply(pSock, pFromAddr, sBuf, strlen(sBuf)) != 0;
}
