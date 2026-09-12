/* 受控静态页面。
 * 页面只能通过 LoadPage 返回，不能像 wwwroot 文件一样被直接访问。 */



static xroot G_PageRoot = NULL;



static bool Page_Init(const char* sAppPath)
{
	str sPath;

	if ( sAppPath == NULL ) return false;
	sPath = xrtPathJoin(sAppPath, "page");
	if ( sPath == NULL ) return false;
	G_PageRoot = xrtRootOpen(sPath);
	xrtFree(sPath);
	if ( G_PageRoot == NULL ) {
		printf("[demo][error] open page directory failed\n");
		return false;
	}
	return true;
}



static void Page_Unit(void)
{
	if ( G_PageRoot != NULL ) {
		xrtRootClose(G_PageRoot);
		G_PageRoot = NULL;
	}
}



static bool LoadPage(XS_HttpReq* pReq, const char* sPage)
{
	bytes pData;
	size_t iSize = 0;
	bool bResult;

	pData = RootFileReadAll(G_PageRoot, sPage, &iSize);
	if ( pData == NULL ) {
		return ReplyText(pReq, 404, "Page Not Found");
	}
	bResult = ReplyRaw(pReq, 200, "text/html; charset=utf-8", pData, iSize);
	xrtFree(pData);
	return bResult;
}
