void ImportJSON(TCCState* s)
{
	tcc_add_symbol(s, "jnum_itoa", jnum_itoa);
	tcc_add_symbol(s, "jnum_ltoa", jnum_ltoa);
	tcc_add_symbol(s, "jnum_htoa", jnum_htoa);
	tcc_add_symbol(s, "jnum_lhtoa", jnum_lhtoa);
	tcc_add_symbol(s, "jnum_dtoa", jnum_dtoa);
	
	tcc_add_symbol(s, "jnum_atoi", jnum_atoi);
	tcc_add_symbol(s, "jnum_atol", jnum_atol);
	tcc_add_symbol(s, "jnum_atoh", jnum_atoh);
	tcc_add_symbol(s, "jnum_atolh", jnum_atolh);
	tcc_add_symbol(s, "jnum_atod", jnum_atod);
}