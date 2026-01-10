


// 全局模板环境变量表
xvalue tblENV = NULL;



// 全局模板表
typedef struct {
	XTE_LiteObject TemplateObject;
} TemplateItem;
xdict TemplateTable;



// 遍历添加模板文件到全局模板表
int ScanTemplateFileProc(str sPath, size_t iSize, int bDir, ptr pData, size_t iPathSize)
{
	char* sKey = xrtLCase(&sPath[iPathSize], 0, FALSE);
	// 统一将 \ 都转换为 /
	char* sPtr = sKey;
	while ( *sPtr != '\0' ) {
		if ( *sPtr == '\\' ) {
			*sPtr = '/';
		}
		sPtr++;
	}
	// 添加到全局模板表
	size_t iRetSize = 0;
	char* sText = xrtFileReadAll(sPath, XRT_CP_BINARY, &iRetSize);
	XTE_LiteObject objTemplate = xteParse(sText, iRetSize, NULL);
	if ( objTemplate->Success ) {
		//printf("load template name : %s\n", sKey);
		xrtDictSetPtr(TemplateTable, sKey, strlen(sKey), objTemplate, NULL);
	} else {
		printf("load template file error : %s\n", sPath);
		printf("Error : %s\n", objTemplate->ErrorDesc);
		printf("Error Info : Line : %d\n", objTemplate->ErrorLine);
		printf("Error Info : LinePos : %d\n", objTemplate->ErrorLinePos);
		printf("Error Info : Pos : %d\n", objTemplate->ErrorPos);
		printf("Error Info : RefLine : %d\n", objTemplate->ErrorRefLine);
		printf("Error Info : RefLinePos : %d\n", objTemplate->ErrorRefLinePos);
		printf("Error Info : RefPos : %d [%.10s]\n", objTemplate->ErrorRefPos, &sText[objTemplate->ErrorRefPos]);
		xteParseFree(objTemplate);  // 解析失败时释放模板对象
	}
	xrtFree(sText);
	xrtFree(sKey);
	return FALSE;
}



// 根据模板生成内容
char* MakePageWithTemplate(char* sTemplate, xvalue tblData, size_t* pRetSize)
{
	// 获取模板
	XTE_LiteObject objTemplate = xrtDictGetPtr(TemplateTable, sTemplate, strlen(sTemplate));
	if ( objTemplate == NULL ) {
		return xrtFormat("<!DOCTYPE html><html><head><meta charset='utf-8'><title>服务器错误</title></head><body> <p>找不到模板文件：%s</p> </body></html>", sTemplate);
	}
	// 根据模板生成页面
	str sPage = xteMake(objTemplate, tblData, tblENV, TemplateTable, pRetSize);
	if ( sPage == NULL ) {
		return xrtFormat("<!DOCTYPE html><html><head><meta charset='utf-8'><title>服务器错误</title></head><body> <p>生成页面失败：%s</p> </body></html>", sTemplate);
	}
	return sPage;
}





// 模板公共函数 - 生成 XID
xvalue TemplateProc_Project_MakeXID(xvalue varENV, xvalue varParam)
{
	return xvoCreateText(xrtMakeXIDS(), 32, TRUE);
}





// 初始化模板渲染功能
xvalue TemplateProc_Device_All(xvalue varENV, xvalue varParam);
xvalue TemplateProc_Device_Idel(xvalue varENV, xvalue varParam);
xvalue TemplateProc_Log_Ext(xvalue varENV, xvalue varParam);
void InitTemplate()
{
	// 加载模板到全局模板表
	TemplateTable = xrtDictCreate(sizeof(TemplateItem));
	xrtDirScan(TemplatePath, TRUE, ScanTemplateFileProc, (void*)(strlen(TemplatePath) + 1));
	
	// 初始化 全局模板环境变量表
	tblENV = xvoCreateTable();
	xvoTableSetFunc(tblENV, "MakeXID", 7, TemplateProc_Project_MakeXID);
}


