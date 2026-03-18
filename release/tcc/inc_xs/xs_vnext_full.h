#ifndef XS_VNEXT_FULL_H
#define XS_VNEXT_FULL_H

#include "xrt.h"
#include "inline_libtcc.h"
#include "inline_sqlite3.h"
#include "xs_vnext.h"

xrtGlobalData* g_pXCore = NULL;
void* g_pXsLoop = NULL;
xarray g_arrXsServerList = NULL;

static inline void XS_SetGlobalDate(int idx, void* ptr)
{
	if ( idx == 1 ) {
		g_pXsLoop = ptr;
	} else if ( idx == 2 ) {
		g_arrXsServerList = (xarray)ptr;
	} else if ( idx == 3 ) {
		g_pXCore = (xrtGlobalData*)ptr;
	}
}

int64_t xsDataRegister(xvalue objValue);
xvalue xsDataGet(int64_t iID);
int xsDataRetain(int64_t iID);
int xsDataRelease(int64_t iID);
int xsDataRemove(int64_t iID);
char* xsBusStatusJson(void);
int xsMsgSendToHost(const char* sServer, const char* sHost, const char* sTopic, int64_t iDataID, xvalue objArgs);
int xsMsgSendToServer(const char* sServer, const char* sTopic, int64_t iDataID, xvalue objArgs);

#endif
