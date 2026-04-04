#ifndef XS_SCRIPT_IMPORT_C_IMPORT_XSMTP_H
#define XS_SCRIPT_IMPORT_C_IMPORT_XSMTP_H

#include "../../../lib/xsmtp.h"



static inline void ImportXSMTP(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	tcc_add_symbol(s, "xrtSmtpConfigInit", xrtSmtpConfigInit);
	tcc_add_symbol(s, "xrtSmtpMessageInit", xrtSmtpMessageInit);
	tcc_add_symbol(s, "xrtSmtpResultInit", xrtSmtpResultInit);
	tcc_add_symbol(s, "xrtSmtpSendMail", xrtSmtpSendMail);
}

#endif
