void ImportOther(TCCState* s)
{
	tcc_add_symbol(s, "http_reply", http_reply);
	tcc_add_symbol(s, "ParseCookies", ParseCookies);
}