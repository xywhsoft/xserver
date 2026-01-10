void ImportOther(TCCState* s)
{
	// TCC 状态机管理
	tcc_add_symbol(s, "xsCreateTCC", xsCreateTCC);
	tcc_add_symbol(s, "xsDestroyTCC", xsDestroyTCC);
	// 工具函数
	tcc_add_symbol(s, "sql_escape", sql_escape);
	tcc_add_symbol(s, "http_reply", http_reply);
	tcc_add_symbol(s, "ParseCookies", ParseCookies);
	tcc_add_symbol(s, "FreeCookies", FreeCookies);
}