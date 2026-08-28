#ifndef XS_SCRIPT_IMPORT_C_IMPORT_ALL_H
#define XS_SCRIPT_IMPORT_C_IMPORT_ALL_H

#include "import_xrt.h"
#include "import_xsmtp.h"
#include "import_lz4.h"
#include "import_zstd.h"
#include "import_lzma.h"
#include "import_xpack.h"
#include "import_md4c.h"
#include "import_libtcc.h"
#include "import_sqlite.h"

static inline void XS_ImportThirdPartyAPI(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	ImportXRT(s);
	ImportXSMTP(s);
	ImportLZ4(s);
	ImportZSTD(s);
	ImportLZMA(s);
	ImportXPack(s);
	ImportMD4C(s);
	ImportLibTCC(s);
	ImportSQLite(s);

	tcc_add_symbol(s, "XS_ImportThirdPartyAPI", XS_ImportThirdPartyAPI);
}
#endif
