/* HTTP virtual-host fixture: proves that Host selects this host's script. */
#include <xsbase.h>
#include <stdio.h>
#include <string.h>

static bool PathIs(XS_HttpReq* pReq, const char* sPath)
{
	size_t iLen = strlen(sPath);

	return pReq->head->Target.Size == iLen &&
	       memcmp(pReq->head->Target.Data, sPath, iLen) == 0;
}

static bool Reply(XS_HttpReq* pReq, uint16 iStatus, const char* sContentType,
	const char* sText)
{
	char arrHead[512];
	char arrLength[32];
	xhttpfield arrFields[2];
	size_t iHeadSize = 0;
	size_t iBodySize = strlen(sText);
	size_t iWritten = 0;

	snprintf(arrLength, sizeof(arrLength), "%llu", (unsigned long long)iBodySize);
	arrFields[0].Name = XRT_STR_LITERAL("Content-Length");
	arrFields[0].Value = xrtStrView(arrLength);
	arrFields[1].Name = XRT_STR_LITERAL("Content-Type");
	arrFields[1].Value = xrtStrView(sContentType);
	if ( !xrtHttp1ResponseWrite(XHTTP_VERSION_1_1, iStatus, xrtHttpStatusText(iStatus),
		arrFields, 2, arrHead, sizeof(arrHead), &iHeadSize) ) return false;
	if ( pReq->tls != NULL ) {
		return xrtTlsStreamSend(pReq->tls, arrHead, iHeadSize, &iWritten) == XTLS_OK &&
		       iWritten == iHeadSize &&
		       xrtTlsStreamSend(pReq->tls, sText, iBodySize, &iWritten) == XTLS_OK &&
		       iWritten == iBodySize;
	}
	return xrtNetStreamSend(pReq->tcp, arrHead, iHeadSize) == XNET_RESULT_OK &&
	       xrtNetStreamSend(pReq->tcp, sText, iBodySize) == XNET_RESULT_OK;
}

XS_RequestResult RequestProc(XS_HttpReq* pReq)
{
	if ( PathIs(pReq, "/vhost") ) {
		(void)Reply(pReq, 200, "text/plain; charset=utf-8", "xs3 admin script ok");
		return XS_OK;
	}
	if ( PathIs(pReq, "/reload") ) {
		XS_ReloadId iId = xsReloadHostSubmit((XS_HostInfo*)pReq->host);
		char arrJson[96];

		if ( iId == 0 ) {
			(void)Reply(pReq, 503, "application/json", "{\"status\":\"rejected\"}");
		} else {
			snprintf(arrJson, sizeof(arrJson), "{\"reload_id\":%llu}",
				(unsigned long long)iId);
			(void)Reply(pReq, 202, "application/json", arrJson);
		}
		return XS_OK;
	}
	return XS_FALLBACK;
}
