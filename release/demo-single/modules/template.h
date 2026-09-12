/* 模板页面。
 * 范例在每次请求时读取、编译并渲染模板，让完整链路保持可见。需要缓存时，
 * 应用可以在自己的模块中于 ServiceInit 编译、ServiceUnit 释放。 */



static xroot G_TemplateRoot = NULL;



static bool Template_Init(const char* sAppPath)
{
	str sPath;

	if ( sAppPath == NULL ) return false;
	sPath = xrtPathJoin(sAppPath, "template");
	if ( sPath == NULL ) return false;
	G_TemplateRoot = xrtRootOpen(sPath);
	xrtFree(sPath);
	if ( G_TemplateRoot == NULL ) {
		printf("[demo][error] open template directory failed\n");
		return false;
	}
	return true;
}



static void Template_Unit(void)
{
	if ( G_TemplateRoot != NULL ) {
		xrtRootClose(G_TemplateRoot);
		G_TemplateRoot = NULL;
	}
}



/* pData 只在渲染期间借用，所有权仍属于调用方。 */
static bool LoadTemplate(
	XS_HttpReq* pReq,
	const char* sTemplate,
	const xvalue* pData
)
{
	bytes pSource;
	size_t iSourceSize = 0;
	xtemplate* pTemplate;
	str sPage;
	size_t iPageSize = 0;
	bool bResult;

	pSource = RootFileReadAll(G_TemplateRoot, sTemplate, &iSourceSize);
	if ( pSource == NULL ) {
		return ReplyText(pReq, 500, "template not found");
	}
	pTemplate = xrtTemplateCompile(
		(xstrview){ (const char*)pSource, iSourceSize });
	xrtFree(pSource);
	if ( pTemplate == NULL ) {
		return ReplyText(pReq, 500, "template parse failed");
	}

	sPage = xrtTemplateRender(pTemplate, pData, &iPageSize);
	xrtTemplateRelease(pTemplate);
	if ( sPage == NULL ) {
		return ReplyText(pReq, 500, "render failed");
	}
	bResult = ReplyRaw(pReq, 200, "text/html; charset=utf-8",
		sPage, iPageSize);
	xrtFree(sPage);
	return bResult;
}
