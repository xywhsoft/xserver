#ifndef XS_SCRIPT_IMPORT_C_IMPORT_ALL_H
#define XS_SCRIPT_IMPORT_C_IMPORT_ALL_H

#include "import_xrt.h"
#include "import_xsmtp.h"
#include "import_libtcc.h"
#include "import_sqlite.h"

static inline void XS_ImportThirdPartyAPI(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	ImportXRT(s);
	ImportXSMTP(s);
	ImportLibTCC(s);
	ImportSQLite(s);

	tcc_add_symbol(s, "XS_ImportThirdPartyAPI", XS_ImportThirdPartyAPI);
}
#endif
