#ifndef XS_SCRIPT_IMPORT_C_IMPORT_MD4C_H
#define XS_SCRIPT_IMPORT_C_IMPORT_MD4C_H



static inline void ImportMD4C(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	tcc_add_symbol(s, "md_parse", md_parse);
	tcc_add_symbol(s, "md_html", md_html);
}



#endif
