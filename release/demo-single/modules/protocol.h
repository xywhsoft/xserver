/*
 * HTTP 路由模块
 *
 * 这个模块只做两件事：
 *   1. 把 URI 注册成静态路由或动态路由；
 *   2. 请求到达后先匹配 URI，再按 HTTP 方法选择处理函数。
 *
 * 静态路由使用 xrtMap，动态路由使用 xrtPattern。路由表只在
 * ServiceInit 阶段写入，开始服务后保持只读，因此请求分发不需要锁。
 */



/* 动态路由参数。
 * 名称借用编译后的 pattern，值借用本次请求的 URI；二者都不需要释放，
 * 但参数值只能在当前请求处理函数返回前使用。 */
typedef struct {
	const char*		sName;
	size_t			iNameSize;
	const char*		sValue;
	size_t			iValueSize;
} RouteParamHTTP;



/* 静态路由和动态路由使用完全相同的处理函数签名。
 * 静态路由没有路径参数，因此 arrParam 为 NULL、iParamCount 为 0。 */
typedef void (*RouteProcHTTP)(
	XS_HttpReq* pReq,
	const RouteParamHTTP* arrParam,
	uint32 iParamCount
);



/* xhttpmethod 是位码，不能直接作为数组下标，所以在路由节点内使用
 * 一组紧凑槽位保存处理函数。一个 URI 只对应一个 RouteInfoHTTP，
 * 同一个 URI 的 GET、POST、PUT 等操作分别落在不同槽位。 */
typedef enum {
	ROUTE_SLOT_GET = 0,
	ROUTE_SLOT_HEAD,
	ROUTE_SLOT_POST,
	ROUTE_SLOT_PUT,
	ROUTE_SLOT_DELETE,
	ROUTE_SLOT_CONNECT,
	ROUTE_SLOT_OPTIONS,
	ROUTE_SLOT_TRACE,
	ROUTE_SLOT_PATCH,
	ROUTE_SLOT_OTHER,
	ROUTE_SLOT_MAX
} RouteSlotHTTP;



typedef struct {
	RouteProcHTTP		arrProc[ROUTE_SLOT_MAX];
} RouteInfoHTTP;



/* 槽位、方法位码和输出名称必须保持相同顺序。 */
static const xhttpmethod G_arrSlotMethod[ROUTE_SLOT_MAX] = {
	XHTTP_METHOD_GET,
	XHTTP_METHOD_HEAD,
	XHTTP_METHOD_POST,
	XHTTP_METHOD_PUT,
	XHTTP_METHOD_DELETE,
	XHTTP_METHOD_CONNECT,
	XHTTP_METHOD_OPTIONS,
	XHTTP_METHOD_TRACE,
	XHTTP_METHOD_PATCH,
	XHTTP_METHOD_OTHER
};

static const char* G_arrSlotName[ROUTE_SLOT_MAX] = {
	"GET", "HEAD", "POST", "PUT", "DELETE",
	"CONNECT", "OPTIONS", "TRACE", "PATCH", "OTHER"
};



/* 这些对象属于当前脚本代。热重载会创建一套新的静态变量，旧代在
 * 请求全部离开后才执行 ServiceUnit，因此这里不需要跨代共享状态。 */
static xmap* G_StaticRouteTableHTTP = NULL;
static xpattern* G_DynamicRoutePattern = NULL;



/* 动态路由先登记，全部注册结束后再一次性编译成不可变 pattern。
 * 范例使用固定上限，让资源边界一眼可见，也避免引入另一套容器代码。 */
#define ROUTE_DYNAMIC_MAX	64
#define ROUTE_PARAM_MAX		8

static char* G_arrDynPattern[ROUTE_DYNAMIC_MAX];
static RouteInfoHTTP G_arrDynInfo[ROUTE_DYNAMIC_MAX];
static uint32 G_iDynCount = 0;



/* 把方法掩码转换为便于日志和 Allow 响应头阅读的文本。 */
static size_t RouteMethodTextHTTP(xhttpmethod iMethods, char* sText, size_t iCapacity)
{
	size_t iUsed = 0;
	uint32 i;

	if ( sText == NULL || iCapacity == 0 ) {
		return 0;
	}
	sText[0] = '\0';
	for ( i = 0; i < ROUTE_SLOT_MAX; i++ ) {
		size_t iNameSize;

		if ( (iMethods & G_arrSlotMethod[i]) == 0 ) {
			continue;
		}
		iNameSize = strlen(G_arrSlotName[i]);
		if ( iUsed != 0 ) {
			if ( iUsed + 2 >= iCapacity ) break;
			sText[iUsed++] = ',';
			sText[iUsed++] = ' ';
		}
		if ( iUsed + iNameSize >= iCapacity ) break;
		memcpy(sText + iUsed, G_arrSlotName[i], iNameSize);
		iUsed += iNameSize;
	}
	sText[iUsed] = '\0';
	return iUsed;
}



/* 将本次注册的方法填入节点。
 *
 * 返回值是已经存在、即将被覆盖的方法集合。未包含在 iMethods 中的槽位
 * 完全不动，所以同一个 URI 可以先注册 GET，再注册另一个 POST 回调。 */
static xhttpmethod RouteSetMethodsHTTP(
	RouteInfoHTTP* pInfo,
	xhttpmethod iMethods,
	RouteProcHTTP proc
)
{
	xhttpmethod iOverwritten = XHTTP_METHOD_INVALID;
	uint32 i;

	for ( i = 0; i < ROUTE_SLOT_MAX; i++ ) {
		if ( (iMethods & G_arrSlotMethod[i]) == 0 ) {
			continue;
		}
		if ( pInfo->arrProc[i] != NULL ) {
			iOverwritten = (xhttpmethod)(iOverwritten | G_arrSlotMethod[i]);
		}
		pInfo->arrProc[i] = proc;
	}
	return iOverwritten;
}



static bool RouteMethodsValidHTTP(xhttpmethod iMethods)
{
	uint32 iValue = (uint32)iMethods;
	uint32 iKnown = (uint32)XHTTP_METHOD_ANY;

	return (iValue & iKnown) != 0 && (iValue & ~iKnown) == 0;
}



static void RouteWarnOverwriteHTTP(const char* sPath, xhttpmethod iMethods)
{
	char arrMethods[128];

	RouteMethodTextHTTP(iMethods, arrMethods, sizeof(arrMethods));
	printf("[route][warn] duplicate route method: %s %s; callback overwritten\n",
		arrMethods, (sPath ? sPath : "?"));
}



/* 添加静态路由。
 * xrtMap 保存 RouteInfoHTTP 的值副本，因此先取出旧节点、修改对应槽位，
 * 再写回即可。 */
static void AddStaticRouteHTTP(
	const char* sPath,
	xhttpmethod iMethods,
	RouteProcHTTP proc
)
{
	xbytesview tKey;
	RouteInfoHTTP* pOld;
	RouteInfoHTTP tInfo;
	xhttpmethod iOverwritten;

	if ( G_StaticRouteTableHTTP == NULL || sPath == NULL || sPath[0] != '/' ||
	     proc == NULL || !RouteMethodsValidHTTP(iMethods) ) {
		printf("[route][error] invalid static route registration: %s\n",
			(sPath ? sPath : "?"));
		return;
	}
	tKey = (xbytesview){ (cbytes)sPath, strlen(sPath) };
	pOld = (RouteInfoHTTP*)xrtMapGet(G_StaticRouteTableHTTP, tKey);
	if ( pOld != NULL ) {
		tInfo = *pOld;
	} else {
		memset(&tInfo, 0, sizeof(tInfo));
	}

	iOverwritten = RouteSetMethodsHTTP(&tInfo, iMethods, proc);
	if ( !xrtMapSet(G_StaticRouteTableHTTP, tKey, &tInfo) ) {
		printf("[route][error] add static route failed: %s\n", sPath);
		return;
	}
	if ( iOverwritten != XHTTP_METHOD_INVALID ) {
		RouteWarnOverwriteHTTP(sPath, iOverwritten);
	}
}



/* 添加动态路由。
 * 完全相同的 pattern 会复用同一个节点，因此可以分多次为它注册不同方法。
 * 结构相同但参数名不同的 pattern（如 /item/{id} 与 /item/{name}）由
 * xrtPattern 在编译期报告冲突，避免产生含糊的参数契约。 */
static void AddDynamicRouteHTTP(
	const char* sPattern,
	xhttpmethod iMethods,
	RouteProcHTTP proc
)
{
	RouteInfoHTTP* pInfo = NULL;
	xhttpmethod iOverwritten;
	uint32 i;

	if ( sPattern == NULL || sPattern[0] != '/' || proc == NULL ||
	     !RouteMethodsValidHTTP(iMethods) ) {
		printf("[route][error] invalid dynamic route registration: %s\n",
			(sPattern ? sPattern : "?"));
		return;
	}

	/* 注册只发生在启动阶段，最多 64 项；线性查找比再维护一张索引表简单。 */
	for ( i = 0; i < G_iDynCount; i++ ) {
		if ( strcmp(G_arrDynPattern[i], sPattern) == 0 ) {
			pInfo = &G_arrDynInfo[i];
			break;
		}
	}
	if ( pInfo == NULL ) {
		if ( G_iDynCount >= ROUTE_DYNAMIC_MAX ) {
			printf("[route][error] too many dynamic routes: %s\n", sPattern);
			return;
		}
		G_arrDynPattern[G_iDynCount] = xrtStrDup(sPattern);
		if ( G_arrDynPattern[G_iDynCount] == NULL ) {
			printf("[route][error] copy dynamic route failed: %s\n", sPattern);
			return;
		}
		pInfo = &G_arrDynInfo[G_iDynCount];
		memset(pInfo, 0, sizeof(*pInfo));
		G_iDynCount++;
	}

	iOverwritten = RouteSetMethodsHTTP(pInfo, iMethods, proc);
	if ( iOverwritten != XHTTP_METHOD_INVALID ) {
		RouteWarnOverwriteHTTP(sPattern, iOverwritten);
	}
}



/* 注册完成后编译动态路由。编译对象是不可变的，可被多个工作线程并发查询。 */
static bool RouteHTTP_Compile(void)
{
	xpatternspec* arrSpec;
	uint32 i;

	if ( G_iDynCount == 0 ) {
		return true;
	}
	arrSpec = (xpatternspec*)xrtCalloc(G_iDynCount, sizeof(xpatternspec));
	if ( arrSpec == NULL ) {
		printf("[route][error] compile dynamic routes failed: out of memory\n");
		return false;
	}
	for ( i = 0; i < G_iDynCount; i++ ) {
		arrSpec[i].Pattern = xrtStrView(G_arrDynPattern[i]);
		arrSpec[i].Value = &G_arrDynInfo[i];
	}
	G_DynamicRoutePattern = xrtPatternCompileMany(arrSpec, G_iDynCount);
	xrtFree(arrSpec);
	if ( G_DynamicRoutePattern == NULL ) {
		printf("[route][error] compile dynamic routes failed\n");
		return false;
	}
	if ( xrtPatternMaxCaptureCount(G_DynamicRoutePattern) > ROUTE_PARAM_MAX ) {
		printf("[route][error] dynamic route has more than %u parameters\n",
			(unsigned int)ROUTE_PARAM_MAX);
		xrtPatternRelease(G_DynamicRoutePattern);
		G_DynamicRoutePattern = NULL;
		return false;
	}
	return true;
}



/* 根据请求中的单个方法位码找到对应槽位。
 * 这里不做 HEAD -> GET 的隐式回落：注册掩码就是路由真实支持范围。 */
static RouteProcHTTP RouteMethodProcHTTP(
	const RouteInfoHTTP* pInfo,
	xhttpmethod eMethod
)
{
	uint32 i;

	for ( i = 0; i < ROUTE_SLOT_MAX; i++ ) {
		if ( eMethod == G_arrSlotMethod[i] ) {
			return pInfo->arrProc[i];
		}
	}
	return NULL;
}



/* 按名称取得动态路由参数。返回的是借用视图，不需要释放。 */
static bool GetRouteParamHTTP(
	const RouteParamHTTP* arrParam,
	uint32 iParamCount,
	const char* sName,
	xstrview* pValue
)
{
	size_t iNameSize;
	uint32 i;

	if ( pValue != NULL ) {
		*pValue = (xstrview){0};
	}
	if ( arrParam == NULL || sName == NULL || pValue == NULL ) {
		return false;
	}
	iNameSize = strlen(sName);
	for ( i = 0; i < iParamCount; i++ ) {
		if ( arrParam[i].iNameSize == iNameSize &&
		     memcmp(arrParam[i].sName, sName, iNameSize) == 0 ) {
			*pValue = (xstrview){ arrParam[i].sValue, arrParam[i].iValueSize };
			return true;
		}
	}
	return false;
}



/* URI 已匹配但方法槽为空时返回 405，并列出该 URI 已注册的方法。 */
static void ReplyMethodNotAllowedHTTP(XS_HttpReq* pReq, const RouteInfoHTTP* pInfo)
{
	char arrAllow[128];
	xhttpmethod iAllowed = XHTTP_METHOD_INVALID;
	xhttpfield tAllow;
	uint32 i;

	for ( i = 0; i < ROUTE_SLOT_MAX; i++ ) {
		/* OTHER 表示所有合法扩展方法，无法在 Allow 中枚举具体 token。 */
		if ( i != ROUTE_SLOT_OTHER && pInfo->arrProc[i] != NULL ) {
			iAllowed = (xhttpmethod)(iAllowed | G_arrSlotMethod[i]);
		}
	}
	RouteMethodTextHTTP(iAllowed, arrAllow, sizeof(arrAllow));
	tAllow.Name = XRT_STR_LITERAL("Allow");
	tAllow.Value = xrtStrView(arrAllow);
	(void)ReplyRawHeaders(pReq, 405, "text/plain; charset=utf-8",
		"Method Not Allowed", strlen("Method Not Allowed"), &tAllow, 1);
}



/* xServer 的 HTTP 请求入口。
 *
 * 匹配顺序固定为：静态 URI -> 动态 pattern -> 方法槽。静态 URI 优先，
 * 因而 /item/new 不会被 /item/{id} 抢走。未命中返回 XS_FALLBACK，
 * 由 xServer 继续尝试 wwwroot 静态文件。 */
XS_RequestResult RequestProc(XS_HttpReq* pReq)
{
	xhttptarget tTarget;
	RouteInfoHTTP* pInfo = NULL;
	RouteProcHTTP proc;
	RouteParamHTTP arrParam[ROUTE_PARAM_MAX];
	uint32 iParamCount = 0;

	if ( pReq == NULL || pReq->head == NULL || G_StaticRouteTableHTTP == NULL ) {
		return XS_FALLBACK;
	}
	if ( !xrtHttpTargetParse(pReq->head->Method, pReq->head->Target, &tTarget) ) {
		return XS_FALLBACK;
	}

	/* 静态表精确匹配；xrtHttpTargetParse 已经把 Query 从 Path 中分离。 */
	pInfo = (RouteInfoHTTP*)xrtMapGet(G_StaticRouteTableHTTP,
		(xbytesview){ (cbytes)tTarget.Path.Data, tTarget.Path.Size });

	/* 静态路由未命中后才进入动态匹配。匹配结果和捕获值全部放在栈上，
	 * 不同工作线程之间不会共享任何请求状态。 */
	if ( pInfo == NULL && G_DynamicRoutePattern != NULL ) {
		xpatternmatch tMatch;
		xstrview arrCapture[ROUTE_PARAM_MAX];

		if ( xrtPatternMatch(G_DynamicRoutePattern, tTarget.Path,
			arrCapture, ROUTE_PARAM_MAX, &tMatch) == XPATTERN_MATCH ) {
			uint32 i;

			pInfo = (RouteInfoHTTP*)tMatch.Value;
			iParamCount = (uint32)tMatch.CaptureCount;
			for ( i = 0; i < iParamCount; i++ ) {
				xstrview tName = {0};

				arrParam[i].sValue = arrCapture[i].Data;
				arrParam[i].iValueSize = arrCapture[i].Size;
				if ( xrtPatternCaptureName(G_DynamicRoutePattern,
					tMatch.PatternIndex, i, &tName) ) {
					arrParam[i].sName = tName.Data;
					arrParam[i].iNameSize = tName.Size;
				} else {
					arrParam[i].sName = NULL;
					arrParam[i].iNameSize = 0;
				}
			}
		}
	}
	if ( pInfo == NULL ) {
		return XS_FALLBACK;
	}

	proc = RouteMethodProcHTTP(pInfo, pReq->head->MethodCode);
	if ( proc == NULL ) {
		ReplyMethodNotAllowedHTTP(pReq, pInfo);
		return XS_OK;
	}
	proc(pReq, (iParamCount == 0 ? NULL : arrParam), iParamCount);
	return XS_OK;
}
