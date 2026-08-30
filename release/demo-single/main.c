/*
 * demo-single — xs3 单 host 应用范例
 *
 * 目录约定（脚本在 host 根目录，不套 script 子目录）：
 *   demo-single/
 *   ├── main.c          ← 本文件（入口 + 路由 + 处理函数）
 *   ├── xs.json         ← 配置（devfile 指向 "main.c"）
 *   ├── wwwroot/        ← 静态文件（host.path 指向）
 *   │   ├── index.html  ← 导航页
 *   │   ├── page.html   ← 模板页（/template 渲染）
 *   │   ├── chart.html  ← ECharts 图表页（调 /chart/get）
 *   │   ├── add.html    ← layui 表单页（POST /app/add）
 *   │   └── res/        ← echarts / layui 前端库
 *   ├── includes/       ← 脚本附加头文件（dev_inc 注册为 TCC 搜索路径）
 *   ├── librarys/       ← 脚本附加库（dev_lib 注册为 TCC 库搜索路径）
 *   └── data/db/        ← SQLite 数据库
 *
 * 运行（配置内相对路径均以 xs.json 所在目录为基准，目录可整体搬移）：
 *   cd release && ./xs demo-single/xs.json
 *   cd release/demo-single && ./xs        ← 自包含方式（目录内自带 xs.exe）
 *
 * /test、/chart/get、/app/* 路由迁移自 dev/v1/release/script 脚本范例。
 */

#include <xsbase.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include <demo_extra.h>

/* ============================================================
 * 基础层：数据库
 * ============================================================ */

static sqlite3* g_DB = NULL;

static void AppInit(XS_HostInfo* pHost)
{
	str sAppPath = xrtPathParent((pHost->Path ? pHost->Path : ""));
	str sDBDir, sFileDB;

	sDBDir = xrtPathJoin(sAppPath, "data");
	xrtDirCreate(sDBDir);
	xrtFree(sDBDir);
	sDBDir = xrtPathJoin(sAppPath, "data/db");
	xrtDirCreate(sDBDir);
	xrtFree(sDBDir);
	sFileDB = xrtPathJoin(sAppPath, "data/db/main.db");
	xrtFree(sAppPath);
	if ( sqlite3_open(sFileDB, &g_DB) != SQLITE_OK ) {
		printf("[demo] sqlite open failed\n");
		g_DB = NULL;
	} else {
		xrtFree(sFileDB);
		sqlite3_exec(g_DB,
			"CREATE TABLE IF NOT EXISTS item ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"name TEXT NOT NULL, "
			"created INTEGER DEFAULT 0)",
			NULL, NULL, NULL);
		/* test 表迁移自 dev/v1 脚本范例（/app/* 路由使用） */
		sqlite3_exec(g_DB,
			"CREATE TABLE IF NOT EXISTS test ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"name TEXT NOT NULL, "
			"age INTEGER DEFAULT 0, "
			"mail TEXT DEFAULT '', "
			"\"desc\" TEXT DEFAULT '')",
			NULL, NULL, NULL);
	}
}

static void AppUnit(void)
{
	if ( g_DB ) {
		sqlite3_close(g_DB);
		g_DB = NULL;
	}
}

/* ============================================================
 * HTTP 辅助层
 * ============================================================ */

static bool RouteIs(XS_HttpReq* pReq, const char* sPath)
{
	size_t i = strlen(sPath);

	return pReq->head->Target.Size == i &&
	       memcmp(pReq->head->Target.Data, sPath, i) == 0;
}

static bool ConnSend(XS_HttpReq* pReq, const void* pData, size_t iSize)
{
	size_t iW = 0;

	if ( pReq->tls != NULL )
		return xrtTlsStreamSend(pReq->tls, pData, iSize, &iW) == XTLS_OK;
	return xrtNetStreamSend(pReq->tcp, pData, iSize) == XNET_RESULT_OK;
}

static bool ReplyRaw(XS_HttpReq* pReq, uint16 iStatus, const char* sCT,
	const void* pBody, size_t iLen)
{
	char arrHead[1024];
	char arrLen[32];
	xhttpfield arrF[3];
	size_t iF = 0;
	size_t iH = 0;
	xstrview tReason;

	snprintf(arrLen, sizeof(arrLen), "%llu", (unsigned long long)iLen);
	arrF[iF].Name = XRT_STR_LITERAL("Content-Length");
	arrF[iF].Value = xrtStrViewN(arrLen, strlen(arrLen));
	iF++;
	if ( sCT ) {
		arrF[iF].Name = XRT_STR_LITERAL("Content-Type");
		arrF[iF].Value = xrtStrView(sCT);
		iF++;
	}
	tReason = xrtHttpStatusText(iStatus);
	if ( !xrtHttp1ResponseWrite(XHTTP_VERSION_1_1, iStatus, tReason,
		arrF, iF, arrHead, sizeof(arrHead), &iH) ) return false;
	if ( !ConnSend(pReq, arrHead, iH) ) return false;
	if ( iLen > 0 && !ConnSend(pReq, pBody, iLen) ) return false;
	return true;
}

static bool ReplyText(XS_HttpReq* pReq, uint16 iStatus, const char* sText)
{
	return ReplyRaw(pReq, iStatus, "text/plain; charset=utf-8", sText, strlen(sText));
}

static bool ReplyHTML(XS_HttpReq* pReq, uint16 iStatus, const char* sHTML)
{
	return ReplyRaw(pReq, iStatus, "text/html; charset=utf-8", sHTML, strlen(sHTML));
}

static bool ReplyJSON(XS_HttpReq* pReq, uint16 iStatus, xvalue* pObj)
{
	str sJSON = xrtJsonStringify(pObj, false, NULL);
	bool bOk = false;

	if ( sJSON ) {
		bOk = ReplyRaw(pReq, iStatus, "application/json; charset=utf-8",
			sJSON, xrtStrView(sJSON).Size);
		xrtFree(sJSON);
	}
	xrtValueRelease(pObj);
	return bOk;
}

static bool ReplyResult(XS_HttpReq* pReq, bool bOk, const char* sMsg)
{
	xvalue* pObj = xrtValueObject();
	xvalue* pB = xrtValueBool(bOk);

	xrtValueObjectSet(pObj, XRT_STR_LITERAL("result"), pB);
	xrtValueRelease(pB);
	xrtValueObjectSetNew(pObj, XRT_STR_LITERAL("msg"), xrtValueString(xrtStrView((sMsg ? sMsg : ""))));
	return ReplyJSON(pReq, 200, pObj);
}

/* ============================================================
 * 迁移层公共助手（来自 dev/v1 script 范例的 DemoSQLite* / DemoHttpReply*）
 * ============================================================ */

static bool DBPrepare(const char* sSQL, sqlite3_stmt** ppStmt)
{
	if ( ppStmt == NULL ) {
		return false;
	}
	*ppStmt = NULL;
	if ( g_DB == NULL || sSQL == NULL ) {
		return false;
	}
	return sqlite3_prepare_v2(g_DB, sSQL, -1, ppStmt, NULL) == SQLITE_OK;
}

static int DBBindTextOrEmpty(sqlite3_stmt* pStmt, int iIndex, const char* sText)
{
	if ( sText == NULL || sText[0] == '\0' ) {
		return sqlite3_bind_text(pStmt, iIndex, "", 0, SQLITE_STATIC);
	}
	return sqlite3_bind_text(pStmt, iIndex, sText, -1, SQLITE_TRANSIENT);
}

static void ReplyDBError(XS_HttpReq* pReq, const char* sMsg)
{
	if ( g_DB != NULL ) {
		(void)ReplyResult(pReq, false, sqlite3_errmsg(g_DB));
	} else {
		(void)ReplyResult(pReq, false, (sMsg ? sMsg : "database not ready"));
	}
}

/* 经 pReq->body 解码器读取完整请求体（定长 / chunked 均可）。
 * 驱动在回调前保证 body 完整；解码推进的线路字节数由驱动按
 * WireBytes 同步消费。返回 NUL 终止副本（'\0' 不计入 *piSize）；
 * 无 body 或解码失败返回 NULL */
static bool ReqBodyFeed(XS_HttpReq* pReq, const unsigned char* pData, size_t iSize,
	char** psOut, size_t* piDone, size_t* piConsumed)
{
	xbytesview tIn;
	xbytesview tData;
	xhttp1errorinfo tErr;
	xhttp1bodystatus eBody;

	tIn.Data = (unsigned char*)pData;
	tIn.Size = iSize;
	eBody = xrtHttp1BodyRead(pReq->body, tIn, false, piConsumed, &tData, &tErr);
	if ( tData.Size > 0 ) {
		char* pNew = (char*)xrtRealloc(*psOut, *piDone + tData.Size + 1);

		if ( pNew == NULL ) return false;
		*psOut = pNew;
		memcpy(*psOut + *piDone, tData.Data, tData.Size);
		*piDone += tData.Size;
	}
	return eBody != XHTTP1_BODY_ERROR && eBody != XHTTP1_BODY_FIELDS;
}

static char* ReqBodyText(XS_HttpReq* pReq, size_t* piSize)
{
	const xnetbuf* pBuf = pReq->tls != NULL ? xrtTlsStreamBuffer(pReq->tls)
	                                        : xrtNetStreamBuffer(pReq->tcp);
	size_t iAvail = pBuf != NULL ? xrtNetBufSize(pBuf) : 0;
	unsigned char arrChunk[4096];
	char* sOut = NULL;
	size_t iUsed = 0;
	size_t iDone = 0;

	if ( iAvail == 0 ) {
		return NULL;		/* 线路上没有 body 字节 */
	}
	while ( iUsed < iAvail ) {
		size_t iGot = iAvail - iUsed > sizeof(arrChunk) ? sizeof(arrChunk) : iAvail - iUsed;
		size_t iConsumed = 0;

		if ( xrtNetBufPeek(pBuf, iUsed, arrChunk, iGot) != iGot ) {
			xrtFree(sOut);
			return NULL;
		}
		if ( !ReqBodyFeed(pReq, arrChunk, iGot, &sOut, &iDone, &iConsumed) ) {
			xrtFree(sOut);
			return NULL;
		}
		iUsed += iConsumed;
		if ( pReq->body != NULL && xrtHttp1BodyDone(pReq->body) ) break;
		if ( iConsumed == 0 ) {
			xrtFree(sOut);	/* 状态机未前进：body 不完整，不应发生 */
			return NULL;
		}
	}
	/* 输入喂完：空输入让状态机给出终态（定长取满后先返回 DATA 再 DONE） */
	{
		size_t iConsumed = 0;

		if ( !ReqBodyFeed(pReq, NULL, 0, &sOut, &iDone, &iConsumed) ) {
			xrtFree(sOut);
			return NULL;
		}
	}
	if ( pReq->body == NULL || !xrtHttp1BodyDone(pReq->body) || iDone == 0 ) {
		xrtFree(sOut);
		return NULL;
	}
	sOut[iDone] = '\0';
	if ( piSize != NULL ) *piSize = iDone;
	return sOut;
}

/* 读取 body 并解析 JSON；两种失败以 *pbMissing 区分（无 body / 非法 JSON）。
 * 调用方负责 xrtValueRelease */
static xvalue* ReqBodyJSON(XS_HttpReq* pReq, bool* pbMissing)
{
	size_t iSize = 0;
	char* sBody = ReqBodyText(pReq, &iSize);
	xvalue* pBody;

	if ( sBody == NULL ) {
		if ( pbMissing != NULL ) *pbMissing = true;
		return NULL;
	}
	if ( pbMissing != NULL ) *pbMissing = false;
	pBody = xrtJsonParse(xrtStrViewN(sBody, iSize));
	xrtFree(sBody);
	return pBody;
}

/* 取对象内文本字段的 NUL 终止副本（空字段返回 NULL）；调用方 xrtFree */
static char* ObjTextDup(xvalue* pObj, const char* sKey)
{
	xstrview tView = {0};

	if ( pObj == NULL ||
	     !xrtValueGetString(xrtValueObjectGet(pObj, xrtStrViewN(sKey, strlen(sKey))), &tView) ||
	     tView.Size == 0 ) {
		return NULL;
	}
	return xrtStrDupN(tView.Data, tView.Size);
}

/* ============================================================
 * 路由处理函数
 * ============================================================ */

static void Handle_Text(XS_HttpReq* pReq)
{
	(void)ReplyText(pReq, 200, DEMO_EXTRA_TEXT);
}

static void Handle_JSON(XS_HostInfo* pHost, XS_HttpReq* pReq)
{
	xvalue* pObj = xrtValueObject();
	xvalue* pB = xrtValueBool(true);

	xrtValueObjectSetNew(pObj, XRT_STR_LITERAL("server"),
		xrtValueString(XRT_STR_LITERAL("xs3 demo")));
	xrtValueObjectSet(pObj, XRT_STR_LITERAL("ok"), pB);
	xrtValueRelease(pB);
	xrtValueObjectSetNew(pObj, XRT_STR_LITERAL("host"),
		xrtValueString(xrtStrView((pHost->Name ? pHost->Name : "?"))));
	(void)ReplyJSON(pReq, 200, pObj);
}

static void Handle_Template(XS_HostInfo* pHost, XS_HttpReq* pReq)
{
	str sApp = xrtPathParent((pHost->Path ? pHost->Path : ""));
	str sFile = xrtPathJoin(sApp, "wwwroot");
	str sTmp;
	str sText;
	xtemplate* pTpl;
	xvalue* pData;
	xvalue* pList;
	str sPage;
	int i;

	xrtFree(sApp);
	sTmp = xrtPathJoin(sFile, "page.html");
	xrtFree(sFile);
	sFile = sTmp;

	sText = (str)xrtFileReadAll(sFile, NULL);
	xrtFree(sFile);
	if ( sText == NULL ) {
		(void)ReplyText(pReq, 500, "template not found");
		return;
	}
	pTpl = xrtTemplateCompile(xrtStrView(sText));
	xrtFree(sText);
	if ( pTpl == NULL ) {
		(void)ReplyText(pReq, 500, "template parse failed");
		return;
	}

	pData = xrtValueObject();
	pList = xrtValueArray();
	xrtValueObjectSetNew(pData, XRT_STR_LITERAL("title"),
		xrtValueString(XRT_STR_LITERAL("模板页")));
	xrtValueObjectSetNew(pData, XRT_STR_LITERAL("desc"),
		xrtValueString(XRT_STR_LITERAL("由 xrtTemplateCompile 渲染")));
	{
		static const char* arrItems[] = { "条目甲", "条目乙", "条目丙", "条目丁", "条目戊" };

		for ( i = 0; i < 5; i++ ) {
			xrtValueArrayAppendNew(pList, xrtValueString(xrtStrView(arrItems[i])));
		}
	}
	xrtValueObjectSetNew(pData, XRT_STR_LITERAL("list"), pList);

	sPage = xrtTemplateRender(pTpl, pData, NULL);
	xrtTemplateRelease(pTpl);
	xrtValueRelease(pData);
	if ( sPage == NULL ) {
		(void)ReplyText(pReq, 500, "render failed");
		return;
	}
	(void)ReplyHTML(pReq, 200, sPage);
	xrtFree(sPage);
}

static void Handle_API_List(XS_HttpReq* pReq)
{
	xvalue* pRet = xrtValueObject();
	xvalue* pArr = xrtValueArray();
	sqlite3_stmt* pStmt = NULL;
	int iStep;
	int iCount = 0;

	if ( g_DB == NULL ) {
		xrtValueRelease(pArr);
		xrtValueRelease(pRet);
		(void)ReplyResult(pReq, false, "database not ready");
		return;
	}
	if ( sqlite3_prepare_v2(g_DB, "SELECT id, name, created FROM item ORDER BY id",
		-1, &pStmt, NULL) != SQLITE_OK ) {
		xrtValueRelease(pArr);
		xrtValueRelease(pRet);
		(void)ReplyResult(pReq, false, sqlite3_errmsg(g_DB));
		return;
	}
	while ( (iStep = sqlite3_step(pStmt)) == SQLITE_ROW ) {
		xvalue* pRow = xrtValueObject();
		xvalue* pId = xrtValueInt(sqlite3_column_int64(pStmt, 0));
		xvalue* pCreated = xrtValueInt(sqlite3_column_int64(pStmt, 2));

		xrtValueObjectSet(pRow, XRT_STR_LITERAL("id"), pId);
		xrtValueRelease(pId);
		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("name"),
			xrtValueString(xrtStrView((cstr)sqlite3_column_text(pStmt, 1))));
		xrtValueObjectSet(pRow, XRT_STR_LITERAL("created"), pCreated);
		xrtValueRelease(pCreated);
		xrtValueArrayAppendNew(pArr, pRow);
		iCount++;
	}
	sqlite3_finalize(pStmt);

	if ( iStep != SQLITE_DONE ) {
		xrtValueRelease(pArr);
		xrtValueRelease(pRet);
		(void)ReplyResult(pReq, false, sqlite3_errmsg(g_DB));
		return;
	}
	xrtValueObjectSetNew(pRet, XRT_STR_LITERAL("count"), xrtValueInt(iCount));
	xrtValueObjectSetNew(pRet, XRT_STR_LITERAL("data"), pArr);
	(void)ReplyJSON(pReq, 200, pRet);
}

static void Handle_API_Add(XS_HttpReq* pReq)
{
	/* POST /api/add  body: {"name":"xxx"} */
	bool bMissing = false;
	xvalue* pBody;
	xvalue* pNameVal;
	xstrview tName = {0};
	sqlite3_stmt* pStmt;

	pBody = ReqBodyJSON(pReq, &bMissing);
	if ( pBody == NULL ) {
		(void)ReplyResult(pReq, false, (bMissing ? "body required" : "invalid json"));
		return;
	}
	pNameVal = xrtValueObjectGet(pBody, XRT_STR_LITERAL("name"));
	if ( pNameVal == NULL || !xrtValueGetString(pNameVal, &tName) || tName.Size == 0 ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "name required");
		return;
	}
	if ( g_DB == NULL ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "database not ready");
		return;
	}
	if ( sqlite3_prepare_v2(g_DB, "INSERT INTO item (name, created) VALUES (?, ?)",
		-1, &pStmt, NULL) != SQLITE_OK ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, sqlite3_errmsg(g_DB));
		return;
	}
	{
		char* sName = xrtStrDupN(tName.Data, tName.Size);

		sqlite3_bind_text(pStmt, 1, sName ? sName : "", -1, SQLITE_TRANSIENT);
		xrtFree(sName);
	}
	sqlite3_bind_int64(pStmt, 2, (sqlite3_int64)(xrtNow() / 1000000));
	(void)sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	xrtValueRelease(pBody);
	(void)ReplyResult(pReq, true, "ok");
}

/* ============================================================
 * 迁移路由（来自 dev/v1/release/script 范例）
 *   GET  /test       文本自检
 *   GET  /chart/get  ECharts 数据（chart.html 消费）
 *   GET  /app/list   test 表列表
 *   POST /app/add    新增（add.html 消费）
 *   POST /app/del    批量删除（body: [id,...]）
 *   POST /app/edit   单字段编辑（body: {id,field,value}）
 * ============================================================ */

static void Handle_Test(XS_HttpReq* pReq)
{
	(void)ReplyHTML(pReq, 200, "page load success !");
}

static void Handle_Chart_Get(XS_HttpReq* pReq)
{
	static const char* arrDays[7] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
	xvalue* pOption = xrtValueObject();
	xvalue* pXAxis = xrtValueObject();
	xvalue* pXData = xrtValueArray();
	xvalue* pYAxis = xrtValueObject();
	xvalue* pSeries = xrtValueArray();
	xvalue* pItem = xrtValueObject();
	xvalue* pData = xrtValueArray();
	xvalue* pSmooth;
	int i;

	xrtValueObjectSetNew(pXAxis, XRT_STR_LITERAL("type"),
		xrtValueString(XRT_STR_LITERAL("category")));
	for ( i = 0; i < 7; i++ ) {
		xrtValueArrayAppendNew(pXData, xrtValueString(xrtStrView(arrDays[i])));
	}
	xrtValueObjectSetNew(pXAxis, XRT_STR_LITERAL("data"), pXData);
	xrtValueObjectSetNew(pOption, XRT_STR_LITERAL("xAxis"), pXAxis);

	xrtValueObjectSetNew(pYAxis, XRT_STR_LITERAL("type"),
		xrtValueString(XRT_STR_LITERAL("value")));
	xrtValueObjectSetNew(pOption, XRT_STR_LITERAL("yAxis"), pYAxis);

	xrtValueObjectSetNew(pItem, XRT_STR_LITERAL("type"),
		xrtValueString(XRT_STR_LITERAL("line")));
	pSmooth = xrtValueBool(true);
	xrtValueObjectSet(pItem, XRT_STR_LITERAL("smooth"), pSmooth);
	xrtValueRelease(pSmooth);
	for ( i = 0; i < 7; i++ ) {
		xrtValueArrayAppendNew(pData, xrtValueInt(xrtRandRange(100, 1500)));
	}
	xrtValueObjectSetNew(pItem, XRT_STR_LITERAL("data"), pData);
	xrtValueArrayAppendNew(pSeries, pItem);
	xrtValueObjectSetNew(pOption, XRT_STR_LITERAL("series"), pSeries);

	(void)ReplyJSON(pReq, 200, pOption);
}

static void Handle_App_List(XS_HttpReq* pReq)
{
	xvalue* pRet = xrtValueObject();
	xvalue* pArr = xrtValueArray();
	sqlite3_stmt* pStmt = NULL;
	int iStep;
	int iCount = 0;

	if ( !DBPrepare("SELECT id, name, age, mail, \"desc\" FROM test", &pStmt) ) {
		xrtValueRelease(pArr);
		xrtValueRelease(pRet);
		ReplyDBError(pReq, "query failed");
		return;
	}
	while ( (iStep = sqlite3_step(pStmt)) == SQLITE_ROW ) {
		xvalue* pRow = xrtValueObject();
		cstr sID = (cstr)sqlite3_column_text(pStmt, 0);
		cstr sName = (cstr)sqlite3_column_text(pStmt, 1);
		cstr sAge = (cstr)sqlite3_column_text(pStmt, 2);
		cstr sMail = (cstr)sqlite3_column_text(pStmt, 3);
		cstr sDesc = (cstr)sqlite3_column_text(pStmt, 4);

		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("id"),
			xrtValueString(xrtStrView((sID ? sID : ""))));
		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("name"),
			xrtValueString(xrtStrView((sName ? sName : ""))));
		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("age"),
			xrtValueString(xrtStrView((sAge ? sAge : ""))));
		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("mail"),
			xrtValueString(xrtStrView((sMail ? sMail : ""))));
		xrtValueObjectSetNew(pRow, XRT_STR_LITERAL("desc"),
			xrtValueString(xrtStrView((sDesc ? sDesc : ""))));
		xrtValueArrayAppendNew(pArr, pRow);
		iCount++;
	}
	sqlite3_finalize(pStmt);
	if ( iStep != SQLITE_DONE ) {
		xrtValueRelease(pArr);
		xrtValueRelease(pRet);
		ReplyDBError(pReq, "query failed");
		return;
	}
	xrtValueObjectSetNew(pRet, XRT_STR_LITERAL("count"), xrtValueInt(iCount));
	xrtValueObjectSetNew(pRet, XRT_STR_LITERAL("code"), xrtValueInt(0));
	xrtValueObjectSetNew(pRet, XRT_STR_LITERAL("msg"),
		xrtValueString(XRT_STR_LITERAL("platform list query success")));
	xrtValueObjectSetNew(pRet, XRT_STR_LITERAL("data"), pArr);
	(void)ReplyJSON(pReq, 200, pRet);
}

static void Handle_App_Add(XS_HttpReq* pReq)
{
	bool bMissing = false;
	xvalue* pBody = ReqBodyJSON(pReq, &bMissing);
	char* sName;
	char* sMail;
	char* sDesc;
	sqlite3_stmt* pStmt = NULL;
	int64 iAge = 0;
	int iResult;

	if ( pBody == NULL ) {
		(void)ReplyResult(pReq, false, (bMissing ? "body required" : "invalid json"));
		return;
	}
	if ( xrtValueType(pBody) != XVALUE_OBJECT ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "body must be json object");
		return;
	}
	sName = ObjTextDup(pBody, "name");
	if ( sName == NULL ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "name required");
		return;
	}
	sMail = ObjTextDup(pBody, "mail");
	sDesc = ObjTextDup(pBody, "desc");
	(void)xrtValueGetInt(xrtValueObjectGet(pBody, XRT_STR_LITERAL("age")), &iAge);
	xrtValueRelease(pBody);

	if ( !DBPrepare("INSERT INTO test (name, age, mail, \"desc\") VALUES (?, ?, ?, ?)", &pStmt) ) {
		xrtFree(sName);
		xrtFree(sMail);
		xrtFree(sDesc);
		ReplyDBError(pReq, "prepare insert failed");
		return;
	}
	DBBindTextOrEmpty(pStmt, 1, sName);
	sqlite3_bind_int64(pStmt, 2, iAge);
	DBBindTextOrEmpty(pStmt, 3, sMail);
	DBBindTextOrEmpty(pStmt, 4, sDesc);
	iResult = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	xrtFree(sName);
	xrtFree(sMail);
	xrtFree(sDesc);
	if ( iResult != SQLITE_DONE ) {
		ReplyDBError(pReq, "insert failed");
		return;
	}
	(void)ReplyResult(pReq, true, "add success");
}

static void Handle_App_Del(XS_HttpReq* pReq)
{
	bool bMissing = false;
	xvalue* pBody = ReqBodyJSON(pReq, &bMissing);
	sqlite3_stmt* pStmt = NULL;
	size_t iCount;
	size_t i;

	if ( pBody == NULL ) {
		(void)ReplyResult(pReq, false, (bMissing ? "body required" : "invalid json"));
		return;
	}
	if ( xrtValueType(pBody) != XVALUE_ARRAY ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "body must be json array");
		return;
	}
	if ( !DBPrepare("DELETE FROM test WHERE id = ?", &pStmt) ) {
		xrtValueRelease(pBody);
		ReplyDBError(pReq, "prepare delete failed");
		return;
	}
	iCount = xrtValueCount(pBody);
	for ( i = 0; i < iCount; i++ ) {
		xvalue* pItem = xrtValueArrayGet(pBody, i);
		xstrview tID = {0};
		char* sID = NULL;
		int64 iID = 0;

		if ( pItem != NULL && xrtValueGetString(pItem, &tID) && tID.Size > 0 ) {
			sID = xrtStrDupN(tID.Data, tID.Size);
		} else if ( pItem != NULL && xrtValueGetInt(pItem, &iID) && iID > 0 ) {
			sID = xrtFormat("%lld", (long long)iID);
		}
		if ( sID == NULL ) {
			continue;
		}
		DBBindTextOrEmpty(pStmt, 1, sID);
		xrtFree(sID);
		if ( sqlite3_step(pStmt) != SQLITE_DONE ) {
			sqlite3_finalize(pStmt);
			xrtValueRelease(pBody);
			ReplyDBError(pReq, "delete failed");
			return;
		}
		sqlite3_reset(pStmt);
		sqlite3_clear_bindings(pStmt);
	}
	sqlite3_finalize(pStmt);
	xrtValueRelease(pBody);
	(void)ReplyResult(pReq, true, "delete success");
}

static void Handle_App_Edit(XS_HttpReq* pReq)
{
	static const struct {
		const char* sField;
		const char* sSQL;
	} arrFields[4] = {
		{ "name", "UPDATE test SET name = ? WHERE id = ?" },
		{ "age",  "UPDATE test SET age = ? WHERE id = ?" },
		{ "mail", "UPDATE test SET mail = ? WHERE id = ?" },
		{ "desc", "UPDATE test SET \"desc\" = ? WHERE id = ?" }
	};
	xvalue* pBody;
	char* sID;
	char* sField;
	char* sValue;
	const char* sSQL = NULL;
	sqlite3_stmt* pStmt = NULL;
	int iResult;
	size_t i;
	bool bMissing = false;

	pBody = ReqBodyJSON(pReq, &bMissing);
	if ( pBody == NULL ) {
		(void)ReplyResult(pReq, false, (bMissing ? "body required" : "invalid json"));
		return;
	}
	if ( xrtValueType(pBody) != XVALUE_OBJECT ) {
		xrtValueRelease(pBody);
		(void)ReplyResult(pReq, false, "body must be json object");
		return;
	}
	sID = ObjTextDup(pBody, "id");
	sField = ObjTextDup(pBody, "field");
	sValue = ObjTextDup(pBody, "value");
	xrtValueRelease(pBody);
	if ( sID == NULL || sField == NULL ) {
		xrtFree(sID);
		xrtFree(sField);
		xrtFree(sValue);
		(void)ReplyResult(pReq, false, (sID == NULL ? "id required" : "field required"));
		return;
	}
	for ( i = 0; i < sizeof(arrFields) / sizeof(arrFields[0]); i++ ) {
		if ( strcmp(sField, arrFields[i].sField) == 0 ) {
			sSQL = arrFields[i].sSQL;
			break;
		}
	}
	if ( sSQL == NULL ) {
		xrtFree(sID);
		xrtFree(sField);
		xrtFree(sValue);
		(void)ReplyResult(pReq, false, "invalid field");
		return;
	}
	if ( !DBPrepare(sSQL, &pStmt) ) {
		xrtFree(sID);
		xrtFree(sField);
		xrtFree(sValue);
		ReplyDBError(pReq, "prepare update failed");
		return;
	}
	DBBindTextOrEmpty(pStmt, 1, sValue);
	DBBindTextOrEmpty(pStmt, 2, sID);
	iResult = sqlite3_step(pStmt);
	sqlite3_finalize(pStmt);
	xrtFree(sID);
	xrtFree(sField);
	xrtFree(sValue);
	if ( iResult != SQLITE_DONE ) {
		ReplyDBError(pReq, "update failed");
		return;
	}
	(void)ReplyResult(pReq, true, "edit success");
}

/* ============================================================
 * 入口
 * ============================================================ */

XS_RequestResult RequestProc(XS_HttpReq* pReq)
{
	XS_HostInfo* pHost = (XS_HostInfo*)pReq->host;

	if ( pReq->head->MethodCode == XHTTP_METHOD_GET ) {
		if ( RouteIs(pReq, "/text") )     { Handle_Text(pReq);              return XS_OK; }
		if ( RouteIs(pReq, "/json") )     { Handle_JSON(pHost, pReq);       return XS_OK; }
		if ( RouteIs(pReq, "/template") ) { Handle_Template(pHost, pReq);   return XS_OK; }
		if ( RouteIs(pReq, "/api/list") ) { Handle_API_List(pReq);         return XS_OK; }
		if ( RouteIs(pReq, "/test") )     { Handle_Test(pReq);             return XS_OK; }
		if ( RouteIs(pReq, "/chart/get") ) { Handle_Chart_Get(pReq);       return XS_OK; }
		if ( RouteIs(pReq, "/app/list") ) { Handle_App_List(pReq);         return XS_OK; }
	}
	if ( pReq->head->MethodCode == XHTTP_METHOD_POST ) {
		if ( RouteIs(pReq, "/api/add") )  { Handle_API_Add(pReq);          return XS_OK; }
		if ( RouteIs(pReq, "/app/add") )  { Handle_App_Add(pReq);          return XS_OK; }
		if ( RouteIs(pReq, "/app/del") )  { Handle_App_Del(pReq);          return XS_OK; }
		if ( RouteIs(pReq, "/app/edit") ) { Handle_App_Edit(pReq);         return XS_OK; }
	}
	return XS_FALLBACK;
}

void ServiceInit(XS_HostInfo* pHost)
{
	AppInit(pHost);
}

void ServiceUnit(XS_HostInfo* pHost)
{
	(void)pHost;
	AppUnit();
}
