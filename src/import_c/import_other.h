void ImportOther(TCCState* s)
{
	tcc_add_symbol(s, "GetLocalIP", GetLocalIP);
	tcc_add_symbol(s, "GetLocalName", GetLocalName);
	
	tcc_add_symbol(s, "http_reply", http_reply);
}