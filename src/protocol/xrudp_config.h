#ifndef XS_PROTOCOL_XRUDP_CONFIG_H
#define XS_PROTOCOL_XRUDP_CONFIG_H

#ifdef XS_USE_XRUDP
/* Pure value settings are retained by a generation; private material is not.
 * key_file is a raw 32-byte key beside the executable, or an absolute path. */
typedef struct XS_RudpSettings {
	xrudpendpointconfig Native;
	unsigned char Public[32], Pin[32], Scope[16], Application[32];
	bool HasPin, HasScope, HasApplication;
} XS_RudpSettings;

static bool XS_RudpHex(xvalue* pObject, const char* sKey, unsigned char* pOut,
	size_t iSize, bool* pPresent, char* sErr, size_t iErrCap)
{
	xvalue* pValue = xrtValueObjectGet(pObject, XS_ConfigKey(sKey));
	xstrview tText;
	*pPresent = pValue != NULL;
	if ( pValue == NULL ) return true;
	if ( !xrtValueGetString(pValue, &tText) || tText.Size != iSize * 2 ) goto Invalid;
	for ( size_t i = 0; i < iSize; ++i ) {
		unsigned n = 0;
		for ( size_t j = 0; j < 2; ++j ) {
			unsigned char c = (unsigned char)tText.Data[i * 2 + j];
			if ( c >= '0' && c <= '9' ) c -= '0';
			else if ( c >= 'a' && c <= 'f' ) c = c - 'a' + 10;
			else if ( c >= 'A' && c <= 'F' ) c = c - 'A' + 10;
			else goto Invalid;
			n = n * 16 + c;
		}
		pOut[i] = (unsigned char)n;
	}
	return true;
Invalid:
	snprintf(sErr, iErrCap, "xrudp.%s requires exactly %u hexadecimal characters", sKey, (unsigned)(iSize * 2));
	return false;
}

static bool XS_RudpNumber(xvalue* pObject, const char* sKey, uint64 iMin,
	uint64 iMax, uint64* pOut, char* sErr, size_t iErrCap)
{
	xvalue* pValue = xrtValueObjectGet(pObject, XS_ConfigKey(sKey));
	int64 n;
	if ( pValue == NULL ) return true;
	if ( !xrtValueGetInt(pValue, &n) || n < 0 || (uint64)n < iMin || (uint64)n > iMax ) {
		snprintf(sErr, iErrCap, "xrudp.%s requires an integer in [%llu,%llu]", sKey,
			(unsigned long long)iMin, (unsigned long long)iMax);
		return false;
	}
	*pOut = (uint64)n;
	return true;
}

static bool XS_RudpSettingsLoad(XS_ServerInfo* pServer, XS_RudpSettings* p,
	unsigned char Private[32], char* sErr, size_t iErrCap)
{
	static const char* const Keys[] = { "key_file", "peer_public", "scope", "application_context",
		"max_peers", "max_half_open", "memory_limit", "handshake_timeout_ms", "handshakes_per_second",
		"address_handshakes_per_second", "streams", "stream_buffer", "message_limit", "idle_timeout_ms",
		"progress_timeout_ms", "datagram_channels", "datagram_send_records", "datagram_receive_records", "datagram_payload" };
	xvalue* obj = xrtValueObjectGet(pServer->Custom, XRT_STR_LITERAL("xrudp"));
	xvalue* file;
	xstrview text;
	char* relative = NULL;
	char* path = NULL;
	xfile handle;
	uint64 size;
	size_t got, bytes;
	bool ok;
	memset(p, 0, sizeof(*p));
	xrtRudpEndpointConfigInit(&p->Native);
	p->Native.Listen = true;
	if ( obj == NULL || xrtValueType(obj) != XVALUE_OBJECT ) {
		snprintf(sErr, iErrCap, "xrudp server '%s' requires an xrudp settings object", pServer->Name); return false;
	}
	for ( size_t i = 0;; ++i ) {
		xstrview key; bool known = false;
		if ( xrtValueObjectAt(obj, i, &key) == NULL ) break;
		for ( size_t j = 0; j < sizeof(Keys)/sizeof(Keys[0]); ++j ) {
			if ( key.Size == strlen(Keys[j]) && memcmp(key.Data, Keys[j], key.Size) == 0 ) known = true;
		}
		if ( !known ) { snprintf(sErr, iErrCap, "unknown xrudp field '%.*s'", (int)(key.Size < 64 ? key.Size : 64), key.Data); return false; }
	}
	if ( !XS_RudpHex(obj, "peer_public", p->Pin, 32, &p->HasPin, sErr, iErrCap) ||
	     !XS_RudpHex(obj, "scope", p->Scope, 16, &p->HasScope, sErr, iErrCap) ||
	     !XS_RudpHex(obj, "application_context", p->Application, 32, &p->HasApplication, sErr, iErrCap) ) return false;
	if ( p->HasPin || p->HasScope ) {
		unsigned char zero[32] = {0};
		if ( (p->HasPin && memcmp(p->Pin, zero, 32) == 0) ||
		     (p->HasScope && memcmp(p->Scope, zero, 16) == 0) ) {
			snprintf(sErr, iErrCap, "xrudp peer_public and scope must be nonzero"); return false;
		}
	}
#define XS_RUDP_OPTION(name, field, low, high) do { uint64 optionValue = (field); \
	if (!XS_RudpNumber(obj, name, low, high, &optionValue, sErr, iErrCap)) { return false; } \
	(field) = optionValue; } while (0)
	XS_RUDP_OPTION("max_peers", p->Native.MaxPeers, 1, 256);
	XS_RUDP_OPTION("max_half_open", p->Native.MaxHalfOpen, 1, p->Native.MaxPeers);
	if ( p->Native.MaxHalfOpen > p->Native.MaxPeers ) p->Native.MaxHalfOpen = p->Native.MaxPeers;
	XS_RUDP_OPTION("memory_limit", p->Native.MemoryLimit, 1048576, 1073741824);
	XS_RUDP_OPTION("handshakes_per_second", p->Native.HandshakesPerSecond, 1, 65536);
	XS_RUDP_OPTION("address_handshakes_per_second", p->Native.AddressHandshakesPerSecond, 1, p->Native.HandshakesPerSecond);
	if ( p->Native.AddressHandshakesPerSecond > p->Native.HandshakesPerSecond ) p->Native.AddressHandshakesPerSecond = p->Native.HandshakesPerSecond;
	{ uint64 n = 10000; XS_RUDP_OPTION("handshake_timeout_ms", n, 1000, 60000); p->Native.HandshakeTimeoutUs = n * 1000; }
	{ uint64 n = p->Native.Session.ProgressTimeoutUs / 1000; XS_RUDP_OPTION("progress_timeout_ms", n, 1000, 600000); p->Native.Session.ProgressTimeoutUs = n * 1000; }
	XS_RUDP_OPTION("idle_timeout_ms", p->Native.Session.Local.IdleTimeoutMs, 1000, 3600000);
	XS_RUDP_OPTION("streams", p->Native.Session.StreamCapacity, 1, 64);
	p->Native.Session.Local.MaxStreams = p->Native.Session.StreamCapacity;
	XS_RUDP_OPTION("stream_buffer", p->Native.Session.ReceiveCapacity, 4096, 1048576);
	p->Native.Session.TransmitCapacity = p->Native.Session.ReceiveCapacity;
	p->Native.Session.Local.StreamCredit = p->Native.Session.ReceiveCapacity;
	XS_RUDP_OPTION("message_limit", p->Native.Session.Local.MaxMessageSize, 0, XRUDP_MESSAGE_SIZE_MAX);
	XS_RUDP_OPTION("datagram_channels", p->Native.Session.Datagrams.Channels, 0, 64);
	if ( p->Native.Session.Datagrams.Channels ) {
		p->Native.Session.Datagrams.SendCapacity = p->Native.Session.Datagrams.ReceiveCapacity = 32;
		p->Native.Session.Datagrams.PayloadCapacity = XRUDP_DATAGRAM_PAYLOAD_MAX;
	}
	XS_RUDP_OPTION("datagram_send_records", p->Native.Session.Datagrams.SendCapacity, 0, 256);
	XS_RUDP_OPTION("datagram_receive_records", p->Native.Session.Datagrams.ReceiveCapacity, 0, 256);
	XS_RUDP_OPTION("datagram_payload", p->Native.Session.Datagrams.PayloadCapacity, 0, XRUDP_DATAGRAM_PAYLOAD_MAX);
#undef XS_RUDP_OPTION
	/* Run the real library validator before exposing any candidate. */
	p->Native.Session.Peer = p->Native.Session.Local;
	if ( xrtRudpSessionAllocationSize(&p->Native.Session, &bytes) != XRUDP_OK ||
	     p->Native.MemoryLimit < bytes + xrtRudpHandshakeAllocationSize() ) {
		snprintf(sErr, iErrCap, "xrudp session settings invalid or memory_limit too small"); return false;
	}
	file = xrtValueObjectGet(obj, XRT_STR_LITERAL("key_file"));
	if ( file == NULL || !xrtValueGetString(file, &text) || text.Size == 0 ||
	     text.Size > 4096 || memchr(text.Data, 0, text.Size) != NULL ) {
		snprintf(sErr, iErrCap, "xrudp.key_file requires a path to a raw 32-byte private key"); return false;
	}
	relative = xrtStrDupN(text.Data, text.Size);
	if ( relative != NULL ) path = xrtPathIsAbs(relative) ? xrtStrDup(relative) : xrtPathJoin(XS_AppPath(), relative);
	xrtFree(relative);
	if ( path == NULL ) { snprintf(sErr, iErrCap, "out of memory resolving xrudp key path"); return false; }
	handle = xrtOpen(path, XFILE_READ);
	xrtFree(path);
	if ( handle == NULL ) { snprintf(sErr, iErrCap, "xrudp.key_file cannot be opened"); return false; }
	ok = xrtFileSize(handle, &size) && size == 32 && xrtReadFull(handle, Private, 32, &got);
	(void)xrtClose(handle);
	if ( ok ) {
		unsigned char zero[32] = {0};
		ok = memcmp(Private, zero, 32) != 0 && xrtX25519Public(Private, p->Public);
	}
	if ( !ok ) { xrtSecureZero(Private, 32); snprintf(sErr, iErrCap, "xrudp.key_file must contain a valid raw 32-byte private key"); }
	return ok;
}
#endif
#endif
