void ImportMD4C(TCCState* s)
{
	// 添加函数 - MD4C
	tcc_add_symbol(s, "md_parse", md_parse);
	tcc_add_symbol(s, "md_html", md_html);
	
}