#ifndef XS_SCRIPT_IMPORT_C_IMPORT_XSMTP_H
#define XS_SCRIPT_IMPORT_C_IMPORT_XSMTP_H



static inline void ImportXSMTP(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	tcc_add_symbol(s, "xrtSmtpConfigInit", xrtSmtpConfigInit);
	tcc_add_symbol(s, "xrtSmtpMessageInit", xrtSmtpMessageInit);
	tcc_add_symbol(s, "xrtSmtpResultInit", xrtSmtpResultInit);
	tcc_add_symbol(s, "xrtSmtpAsyncOptsInit", xrtSmtpAsyncOptsInit);
	tcc_add_symbol(s, "xrtSmtpResultFree", xrtSmtpResultFree);
	tcc_add_symbol(s, "xrtSmtpSendMail", xrtSmtpSendMail);
	tcc_add_symbol(s, "xrtSmtpSendMailFuture", xrtSmtpSendMailFuture);
	tcc_add_symbol(s, "xrtSmtpSendMailCo", xrtSmtpSendMailCo);
	tcc_add_symbol(s, "xrtSmtpSendMailAsyncWait", xrtSmtpSendMailAsyncWait);
}

#endif
