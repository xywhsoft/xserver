#ifndef XS_PROTOCOL_TCP_H
#define XS_PROTOCOL_TCP_H

static inline bool XS_CustomInitServer(xnetengine* pEngine, XS_ServerConfig* objServer);
static inline bool XS_CustomStartServer(XS_ServerConfig* objServer);
static inline void XS_CustomStopServer(XS_ServerConfig* objServer);

static inline bool XS_TcpInitServer(xnetengine* pEngine, XS_ServerConfig* objServer)
{
	return XS_CustomInitServer(pEngine, objServer);
}

static inline bool XS_TcpStartServer(XS_ServerConfig* objServer)
{
	return XS_CustomStartServer(objServer);
}

static inline void XS_TcpStopServer(XS_ServerConfig* objServer)
{
	XS_CustomStopServer(objServer);
}

#endif
