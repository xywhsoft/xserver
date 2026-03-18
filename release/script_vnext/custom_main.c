#include <xs_vnext_full.h>
#include <string.h>

void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
	char sText[256];
	(void)objHost;
	
	snprintf(
		sText,
		sizeof(sText),
		"custom init server=%s",
		xsServerName(objServer)
	);
	xsLog(sText);
}

void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
	char sText[256];
	(void)objHost;
	
	snprintf(
		sText,
		sizeof(sText),
		"custom unit server=%s",
		xsServerName(objServer)
	);
	xsLog(sText);
}

void EventOpenProc(XS_ServerObject objServer, void* pStream)
{
	char sText[256];
	(void)pStream;
	
	snprintf(
		sText,
		sizeof(sText),
		"custom open server=%s",
		xsServerName(objServer)
	);
	xsLog(sText);
}

bool EventDataProc(XS_ServerObject objServer, void* pStream, const void* pData, size_t iLen)
{
	char sBuf[512];
	int iWrite;
	
	iWrite = snprintf(
		sBuf,
		sizeof(sBuf),
		"custom demo\nserver=%s\nbytes=%u\ndata=%.*s\n",
		xsServerName(objServer),
		(unsigned)iLen,
		(int)iLen,
		pData ? (const char*)pData : ""
	);
	if ( iWrite < 0 ) {
		return false;
	}
	
	return xsStreamSend(pStream, sBuf, strlen(sBuf)) != 0;
}

void EventCloseProc(XS_ServerObject objServer, void* pStream, int iReason)
{
	char sText[256];
	(void)pStream;
	
	snprintf(
		sText,
		sizeof(sText),
		"custom close server=%s reason=%d",
		xsServerName(objServer),
		iReason
	);
	xsLog(sText);
}
