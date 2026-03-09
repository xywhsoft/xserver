// 热加载函数前向声明
int xsReloadHost(XS_ServerObject objServer, XS_HostObject objHost);
int xsReloadHostByDomain(XS_ServerObject objServer, const char* sDomain);
int xsReloadDefaultHost(XS_ServerObject objServer);
int xsReloadServer(XS_ServerObject objServer);

void ImportOther(TCCState* s)
{
	// TCC 状态机管理
	tcc_add_symbol(s, "xsCreateTCC", xsCreateTCC);
	tcc_add_symbol(s, "xsDestroyTCC", xsDestroyTCC);
	// 工具函数
	tcc_add_symbol(s, "sql_escape", sql_escape);
	tcc_add_symbol(s, "ParseCookies", ParseCookies);
	tcc_add_symbol(s, "FreeCookies", FreeCookies);
	// 热加载函数
	tcc_add_symbol(s, "xsReloadHost", xsReloadHost);
	tcc_add_symbol(s, "xsReloadHostByDomain", xsReloadHostByDomain);
	tcc_add_symbol(s, "xsReloadDefaultHost", xsReloadDefaultHost);
	tcc_add_symbol(s, "xsReloadServer", xsReloadServer);
}