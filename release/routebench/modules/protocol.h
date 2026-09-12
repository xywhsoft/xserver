

// HTTP 协议与路由系统

/*
	返回值：
		XS_OK        0  // 已写出完整响应，驱动消费 body 余量后继续 keep-alive
		XS_FALLBACK  1  // 未处理：GET/HEAD 落入静态层，其余 404
		XS_TAKEOVER  2  // 应用接管连接：用 pull/Future 自行收发并最终 Close

	pReq 是视图结构体：一组 xrt 指针 + 归属信息，非对象系统。
		* 响应由应用用 xrtHttp1ResponseWrite / xrtHttp1Chunk*Write + xrtNetStreamSend* 自行构造
		typedef struct {
			xnetstream*         tcp;		// 与 tls 二选一非空：传输层裸指针
			xtlsstream*         tls;		// 可直接使用 xrt 全套流/发送 API
			const xhttp1head*   head;		// 含 MethodCode；原始方法和字段视图在回调内有效
			xhttp1body*         body;		// 分帧状态机：定长 / chunked / 关闭
			const XS_HostInfo*  host;		// 虚拟主机对象指针
			XS_ServerInfo*      server;		// 服务器对象指针
		} XS_HttpReq;

	Head 只借用输入和字段数组；输入与数组必须覆盖 Head 的使用期。
		typedef struct xhttp1head {
			xhttpkind     Kind;           // 请求/响应
			xhttpversion  Version;
			uint32        Flags;
			uint16        Status;         // 响应侧
			uint64        ContentLength;
			size_t        Bytes;          // 整个头的线路字节数
			xstrview      Method;         // 原始方法字符串视图
			xstrview      Target;         // request-target 视图
			xstrview      Reason;
			xhttpfield*   Fields;         // 字段数组
			size_t        FieldCount;
			size_t        FieldCapacity;
			xhttpmethod   MethodCode;     // ★ 从 Method 解析一次的位码
		} xhttp1head;

		typedef struct xhttpfield {       // 字段名/值都是借用视图，不要求零结尾
			xstrview Name;
			xstrview Value;
		} xhttpfield;

		typedef enum xhttpmethod {        // 位码，可位测试；OTHER=不匹配已知集合
			XHTTP_METHOD_INVALID = 0, XHTTP_METHOD_OTHER = 0x1,
			XHTTP_METHOD_GET = 0x2, XHTTP_METHOD_HEAD = 0x4, XHTTP_METHOD_POST = 0x8,
			XHTTP_METHOD_PUT = 0x10, XHTTP_METHOD_DELETE = 0x20, XHTTP_METHOD_CONNECT = 0x40,
			XHTTP_METHOD_OPTIONS = 0x80, XHTTP_METHOD_TRACE = 0x100, XHTTP_METHOD_PATCH = 0x200
		} xhttpmethod;

	Body 只借用每次调用传入的输入；当次输入必须覆盖该次产出的数据视图
		typedef struct xhttp1body {
			xhttp1bodymode Mode;          // NONE / FIXED / CHUNKED / CLOSE / TUNNEL
			uint64 Remaining;             // FIXED 剩余字节
			uint64 Received;              // 已解码正文累计
			uint64 WireBytes;             // ★ 已消费线路字节（驱动据此同步）
			xhttpfield* Trailers;         // chunked trailer
			size_t TrailerCount, TrailerCapacity;
			xhttp1bodylimits Limits;
			uint64 ChunkSize;
			size_t ChunkLineBytes;
			uint32  State;                // 内部状态机，只由 xrtHttp1BodyRead 推进
		} xhttp1body;

		typedef enum xhttp1bodystatus {   // Body Reader 每次只发布一个片段或终态
			XHTTP1_BODY_ERROR = -1, XHTTP1_BODY_MORE = 0, XHTTP1_BODY_DATA = 1,
			XHTTP1_BODY_DONE = 2, XHTTP1_BODY_FIELDS = 3
		} xhttp1bodystatus;

	补充信息：
		typedef struct xstrview {         // 一切视图的基础类型
			cstr  Data;
			size_t Size;
		} xstrview;
*/



// HTTP 辅助层（main.c）提供的回复函数
static bool ReplyText(XS_HttpReq* pReq, uint16 iStatus, const char* sText);



/* ============================================================
 * 路由系统：静态表 + 动态表
 *
 * 静态路由 = xrtMap 哈希一次定位（O(1) 次依赖访存），耗时与
 *            路由数量、路径形状无关，时间可预估。
 * 动态路由 = xrtPattern 分段状态机：'/' 分段，{name} 捕获任意非空段，
 *            支持段内 prefix{name}suffix（如 /file/{name}.txt）；
 *            同层字面量优先于参数段，匹配 O(路径长度)。
 * 两层不是性能妥协，是各自跑在语义匹配的结构上。
 *
 * 请求方法在注册期限定：路由项是按方法分槽的结构体（一槽一个回调），
 * 同路径多方法各自占槽互不覆盖；未注册的方法返回 405。
 * ============================================================ */

// 动态路由参数段（{name} 捕获；名称借用模式表，值借用输入路径）
typedef struct {
	const char*		sName;
	size_t			iNameSize;
	const char*		sValue;
	size_t			iValueSize;
} RouteParamHTTP;

// 路由处理函数签名（静态/动态统一；静态路由 iParamCount 恒为 0）
typedef void (*RouteProcHTTP)(XS_HttpReq* pReq, const RouteParamHTTP* arrParam, uint32 iParamCount);

// 方法槽位序（与 xhttpmethod 位码一一对应；注册与分发两侧映射用）
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

// 路由项：按请求方法各占一个回调槽（全 NULL 起步）
typedef struct {
	RouteProcHTTP		arrProc[ROUTE_SLOT_MAX];
} RouteInfoHTTP;

// 槽位 → 方法位码（注册拆位用；ANY 等全位掩码天然全填）
static const xhttpmethod G_arrSlotMethod[ROUTE_SLOT_MAX] = {
	XHTTP_METHOD_GET, XHTTP_METHOD_HEAD, XHTTP_METHOD_POST, XHTTP_METHOD_PUT,
	XHTTP_METHOD_DELETE, XHTTP_METHOD_CONNECT, XHTTP_METHOD_OPTIONS,
	XHTTP_METHOD_TRACE, XHTTP_METHOD_PATCH, XHTTP_METHOD_OTHER
};

// 全局静态路由表 - HTTP
xmap* G_StaticRouteTableHTTP = NULL;

// 全局动态路由表 - HTTP（xrtPattern 编译后的不可变匹配程序）
xpattern* G_DynamicRoutePattern = NULL;

// 动态路由注册暂存（RouteHTTP_Compile 一次性编译成上面那张表）
#define ROUTE_DYNAMIC_MAX	64
char* G_arrDynPattern[ROUTE_DYNAMIC_MAX];
RouteInfoHTTP G_arrDynInfo[ROUTE_DYNAMIC_MAX];
uint32 G_iDynCount = 0;

// 最近一次动态路由匹配捕获的参数（分发期内有效）
#define ROUTE_PARAM_MAX		8
RouteParamHTTP G_arrRouteParam[ROUTE_PARAM_MAX];
uint32 G_iRouteParamCount = 0;

// 添加 HTTP 静态路由（完整路径精确匹配；同路径多方法合并进各自槽位）
void AddStaticRouteHTTP(const char* sPath, xhttpmethod iMethods, RouteProcHTTP proc)
{
	xbytesview tKey = { (cbytes)sPath, strlen(sPath) };
	RouteInfoHTTP* pOld = (RouteInfoHTTP*)xrtMapGet(G_StaticRouteTableHTTP, tKey);
	RouteInfoHTTP tInfo;
	uint32 i;

	if ( pOld != NULL ) {
		tInfo = *pOld;
	} else {
		memset(&tInfo, 0, sizeof(tInfo));
	}
	for ( i = 0; i < ROUTE_SLOT_MAX; i++ ) {
		if ( (iMethods & G_arrSlotMethod[i]) != 0 ) {
			tInfo.arrProc[i] = proc;
		}
	}
	if ( !xrtMapSet(G_StaticRouteTableHTTP, tKey, &tInfo) ) {
		printf("add static http route failed : %s.\n", sPath);
	}
}

// 添加 HTTP 动态路由（分段模式，{name} 捕获参数段；RouteHTTP_Compile 前调用）
void AddDynamicRouteHTTP(const char* sPattern, xhttpmethod iMethods, RouteProcHTTP proc)
{
	uint32 i;

	if ( G_iDynCount >= ROUTE_DYNAMIC_MAX ) {
		printf("add dynamic http route failed : %s. (too many)\n", sPattern);
		return;
	}
	G_arrDynPattern[G_iDynCount] = xrtStrDup(sPattern);
	memset(&G_arrDynInfo[G_iDynCount], 0, sizeof(RouteInfoHTTP));
	for ( i = 0; i < ROUTE_SLOT_MAX; i++ ) {
		if ( (iMethods & G_arrSlotMethod[i]) != 0 ) {
			G_arrDynInfo[G_iDynCount].arrProc[i] = proc;
		}
	}
	G_iDynCount++;
}

// 编译动态路由（注册清单末尾调用一次；失败时动态表为空，仅静态路由可用）
void RouteHTTP_Compile()
{
	xpatternspec* arrSpec;
	uint32 i;

	if ( G_iDynCount == 0 ) {
		return;
	}
	arrSpec = (xpatternspec*)xrtCalloc(G_iDynCount, sizeof(xpatternspec));
	if ( arrSpec == NULL ) {
		printf("compile dynamic http routes failed : out of memory\n");
		return;
	}
	for ( i = 0; i < G_iDynCount; i++ ) {
		arrSpec[i].Pattern = xrtStrView(G_arrDynPattern[i]);
		arrSpec[i].Value = &G_arrDynInfo[i];
	}
	G_DynamicRoutePattern = xrtPatternCompileMany(arrSpec, G_iDynCount);
	xrtFree(arrSpec);
	if ( G_DynamicRoutePattern == NULL ) {
		printf("compile dynamic http routes failed.\n");
	}
}

// 方法位码 → 回调槽（未注册方法返回 NULL；HEAD 未注册时回落 GET，符合 RFC 语义）
static RouteProcHTTP RouteMethodProc(const RouteInfoHTTP* pInfo, xhttpmethod eCode)
{
	RouteProcHTTP proc;

	switch ( eCode ) {
	case XHTTP_METHOD_GET:		proc = pInfo->arrProc[ROUTE_SLOT_GET]; break;
	case XHTTP_METHOD_HEAD:		proc = pInfo->arrProc[ROUTE_SLOT_HEAD]; break;
	case XHTTP_METHOD_POST:		proc = pInfo->arrProc[ROUTE_SLOT_POST]; break;
	case XHTTP_METHOD_PUT:		proc = pInfo->arrProc[ROUTE_SLOT_PUT]; break;
	case XHTTP_METHOD_DELETE:	proc = pInfo->arrProc[ROUTE_SLOT_DELETE]; break;
	case XHTTP_METHOD_CONNECT:	proc = pInfo->arrProc[ROUTE_SLOT_CONNECT]; break;
	case XHTTP_METHOD_OPTIONS:	proc = pInfo->arrProc[ROUTE_SLOT_OPTIONS]; break;
	case XHTTP_METHOD_TRACE:	proc = pInfo->arrProc[ROUTE_SLOT_TRACE]; break;
	case XHTTP_METHOD_PATCH:	proc = pInfo->arrProc[ROUTE_SLOT_PATCH]; break;
	case XHTTP_METHOD_OTHER:	proc = pInfo->arrProc[ROUTE_SLOT_OTHER]; break;
	default:			proc = NULL; break;
	}
	if ( proc == NULL && eCode == XHTTP_METHOD_HEAD ) {
		proc = pInfo->arrProc[ROUTE_SLOT_GET];
	}
	return proc;
}



/* ============================================================
 * HTTP 请求入口
 * ============================================================ */

XS_RequestResult RequestProc(XS_HttpReq* pReq)
{
	xhttptarget tTarget;
	RouteInfoHTTP* pInfo = NULL;
	RouteProcHTTP proc;

	// 静态表精确匹配（Path 已剥离 query）
	if ( !xrtHttpTargetParse(pReq->head->Method, pReq->head->Target, &tTarget) ) {
		return XS_FALLBACK;
	}
	G_iRouteParamCount = 0;
	pInfo = (RouteInfoHTTP*)xrtMapGet(G_StaticRouteTableHTTP,
		(xbytesview){ (cbytes)tTarget.Path.Data, tTarget.Path.Size });

	// 未命中则动态表分段匹配（{name} 捕获段回填参数）
	if ( pInfo == NULL && G_DynamicRoutePattern != NULL ) {
		xpatternmatch tMatch;
		xstrview arrCapture[ROUTE_PARAM_MAX];

		if ( xrtPatternMatch(G_DynamicRoutePattern, tTarget.Path,
			arrCapture, ROUTE_PARAM_MAX, &tMatch) == XPATTERN_MATCH ) {
			pInfo = (RouteInfoHTTP*)tMatch.Value;
			G_iRouteParamCount = (uint32)tMatch.CaptureCount;
			for ( uint32 i = 0; i < G_iRouteParamCount; i++ ) {
				xstrview tName;

				G_arrRouteParam[i].sValue = arrCapture[i].Data;
				G_arrRouteParam[i].iValueSize = arrCapture[i].Size;
				if ( xrtPatternCaptureName(G_DynamicRoutePattern, tMatch.PatternIndex, i, &tName) ) {
					G_arrRouteParam[i].sName = tName.Data;
					G_arrRouteParam[i].iNameSize = tName.Size;
				} else {
					G_arrRouteParam[i].sName = NULL;
					G_arrRouteParam[i].iNameSize = 0;
				}
			}
		}
	}
	if ( pInfo == NULL ) {
		return XS_FALLBACK;
	}

	// 方法槽位过滤
	proc = RouteMethodProc(pInfo, pReq->head->MethodCode);
	if ( proc == NULL ) {
		(void)ReplyText(pReq, 405, "Method Not Allowed");
		return XS_OK;
	}

	proc(pReq, G_arrRouteParam, G_iRouteParamCount);
	return XS_OK;
}
