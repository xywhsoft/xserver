#include <xs_vnext_full.h>
#include <string.h>

void ServiceInit(XS_ServerObject objServer, XS_HostObject objHost)
{
	char sText[256];
	
	snprintf(
		sText,
		sizeof(sText),
		"ws init server=%s host=%s",
		xsServerName(objServer),
		xsHostName(objHost)
	);
	xsLog(sText);
}

void ServiceUnit(XS_ServerObject objServer, XS_HostObject objHost)
{
	char sText[256];
	
	snprintf(
		sText,
		sizeof(sText),
		"ws unit server=%s host=%s",
		xsServerName(objServer),
		xsHostName(objHost)
	);
	xsLog(sText);
}

void WsOpenProc(XS_ServerObject objServer, XS_HostObject objHost, void* pConn)
{
	char sText[256];
	
	snprintf(
		sText,
		sizeof(sText),
		"ws open server=%s host=%s protocol=%s",
		xsServerName(objServer),
		xsHostName(objHost),
		xsWsProtocol(pConn)
	);
	xsLog(sText);
}

bool WsTextProc(XS_ServerObject objServer, XS_HostObject objHost, void* pConn, const char* pData, size_t iLen)
{
	char sBuf[512];
	int iWrite;
	
	iWrite = snprintf(
		sBuf,
		sizeof(sBuf),
		"ws demo\naction=echo\nserver=%s\nhost=%s\nprotocol=%s\ntext=%.*s\n",
		xsServerName(objServer),
		xsHostName(objHost),
		xsWsProtocol(pConn),
		(int)iLen,
		pData ? pData : ""
	);
	if ( iWrite < 0 ) {
		return false;
	}
	
	return xsWsSendText(pConn, sBuf, strlen(sBuf)) != 0;
}

void WsCloseProc(XS_ServerObject objServer, XS_HostObject objHost, void* pConn, int iReason)
{
	char sText[256];
	(void)pConn;
	
	snprintf(
		sText,
		sizeof(sText),
		"ws close server=%s host=%s reason=%d",
		xsServerName(objServer),
		xsHostName(objHost),
		iReason
	);
	xsLog(sText);
}
