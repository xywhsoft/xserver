#ifndef XS_SDK_XSBASE_H
#define XS_SDK_XSBASE_H

/*
 * xs3 契约头 —— 应用与脚本唯一依赖的 xs 层声明
 *
 * 设计依据：docs/设计.md §4/§6/§7
 *   - xs 只透传 xrt 原生对象，不包装
 *   - 结构体即 ABI：布局冻结，新字段只许尾追加、永不重排改名
 *   - 零限制：不做沙盒、权限、命名空间，一切对应用开放
 *
 * 前置包含（二选一，由使用方保证）：
 *   宿主侧：先 #define XRT_MODULE_* + XRT_IMPLEMENTATION 并 include "lib/xrt.h"
 *   脚本侧：本头会自动 include "xrt_decl.h"（位于 tcc/inc_xs/）
 * libtcc 声明由本头自动补入（宿主已含 lib/libtcc.h 时自动跳过）。
 */

#if !defined(XRT_H) && !defined(XRT_DECLARATIONS_H)
	#include "xrt_decl.h"
#endif
#ifndef LIBTCC_H
	#include "libtcc.h"
#endif

/* 进程内直连：宿主单进程实现，脚本经 tcc_add_symbol 按地址解析，
 * 因此无需任何导出修饰 */
#define XS_API
#ifdef __cplusplus
	#define XS_EXTERN_BEGIN	extern "C" {
	#define XS_EXTERN_END	}
#else
	#define XS_EXTERN_BEGIN
	#define XS_EXTERN_END
#endif

XS_EXTERN_BEGIN			/* C++ 兼容 */

/* ============================================================
 * 运行状态
 * ============================================================ */

typedef enum {
	XS_RUN_STARTING = 0,		/* 装配中 */
	XS_RUN_RUNNING,			/* 对外服务 */
	XS_RUN_RELOAD_FAILED,		/* 上次重载失败，旧代继续服务 */
	XS_RUN_STOPPING,		/* 正在排空 */
	XS_RUN_STOPPED			/* 已停止 */
} XS_RunState;

/* reload 是期望状态协调任务，不是同步命令。每次受理返回稳定 ID，结果可查询。 */
typedef uint64 XS_ReloadId;

typedef enum {
	XS_RELOAD_UNKNOWN = 0,
	XS_RELOAD_ACCEPTED,
	XS_RELOAD_PREPARING,
	XS_RELOAD_SUCCEEDED,
	XS_RELOAD_FAILED,
	XS_RELOAD_SUPERSEDED,
	XS_RELOAD_CANCELLED
} XS_ReloadState;

typedef struct XS_ReloadResult {
	XS_ReloadId		Id;
	XS_ReloadState		State;
	uint64			Revision;	/* 本次不可变输入快照的内容指纹；无配置输入时含脚本指纹 */
	char			Target[160];
	char			Message[256];
} XS_ReloadResult;

/* ============================================================
 * 数据模型：server + host 两层（五种协议类统一，无特例）
 * 见设计 §4：恒有 DefaultHost；tcp/udp/custom 的 host 语义由应用自定
 * ============================================================ */

typedef struct XS_HostInfo {
	bool			Enabled;
	const char*			Name;		/* 主机名 */
	const char*			Host;		/* 域名绑定（分号分隔）；http/ws 用于路由 */
	const char*			Path;		/* 资源/脚本根 */
	const char*			DevLang;		/* c / static / 应用自定 */
	const char*			DevFile;		/* 脚本入口 */
	const char*			TlsCA;		/* host 级证书：SNI 选择器数据源 */
	const char*			TlsCert;
	const char*			TlsKey;
	XS_RunState			State;
	void*				Runtime;		/* 本代脚本运行时（不透明） */
	struct XS_ServerInfo*		Server;		/* 反向指针：从一个 host 摸到整个拓扑 */
	xvalue*				Custom;		/* 配置文件中非预设字段的全部内容 */
	/* 以下为尾部追加字段（ABI 纪律：只增不改） */
	const char*			DevInc;		/* 脚本额外 include 目录（分号分隔，相对 appPath） */
	const char*			DevLib;		/* 脚本额外库目录（分号分隔，相对 appPath） */
	void*				RuntimeLock;	/* 宿主侧脚本代切换锁（不透明） */
} XS_HostInfo;

typedef struct XS_ServerInfo {
	bool			Enabled;
	const char*			Class;		/* "http"|"ws"|"tcp"|"udp"|"custom" */
	const char*			Name;		/* 唯一名，xsServerFind 的键 */
	const char*			IP;
	uint16			Port;
	bool			TLS;
	uint16			PortTLS;
	const char*			IPTLS;		/* 缺省复用 IP */
	uint32			Backlog;		/* 直通 xrt 监听配置 */
	size_t			RecvLimit;		/* 直通 xrt 流读上限 */
	XS_HostInfo*			DefaultHost;	/* 恒存在（未配置时由服务级脚本字段合成） */
	uint32			HostCount;		/* hosts[] 数量（不含 DefaultHost） */
	XS_HostInfo**			Hosts;
	XS_RunState			State;
	void*				Runtime;
	struct xnetengine*		Engine;		/* 进程级引擎（custom 类应用的起点） */
	xvalue*				Custom;		/* 配置文件中非预设字段的全部内容 */
	/* 以下为尾部追加字段（ABI 纪律：只增不改） */
	void*				Generation;	/* 服务代生命周期拥有者（不透明） */
	void*				ConfigOwner;	/* 动态配置快照所有者（不透明） */
} XS_ServerInfo;

/* ============================================================
 * HTTP 回调契约（http / https）
 * XS_HttpReq 是视图结构体：一组 xrt 指针 + 归属信息，非对象系统。
 * 响应由应用用 xrtHttp1ResponseWrite / xrtHttp1Chunk*Write +
 * xrtNetStreamSend* 自行构造 —— xs 不提供任何 HTTP 应答便利函数。
 * ============================================================ */

typedef struct {
	xnetstream*			tcp;		/* 与 tls 二选一非空：传输层裸指针 */
	xtlsstream*			tls;		/*   可直接使用 xrt 全套流/发送 API */
	const xhttp1head*		head;		/* 含 MethodCode；原始方法和字段视图在回调内有效 */
	xhttp1body*			body;		/* 分帧状态机：定长 / chunked / 关闭 */
	const XS_HostInfo*		host;
	XS_ServerInfo*			server;
} XS_HttpReq;

typedef int XS_RequestResult;

#define XS_OK			0	/* 已写出完整响应，驱动消费 body 余量后继续 keep-alive */
#define XS_FALLBACK		1	/* 未处理：GET/HEAD 落入静态层，其余 404 */
#define XS_TAKEOVER		2	/* 应用接管连接：用 pull/Future 自行收发并最终 Close；不得替换事件表，
					 * Close 终态由 xs Destroy 并释放 generation lease */

typedef XS_RequestResult (*XS_RequestProc)(XS_HttpReq* pReq);

/* ============================================================
 * 其余协议回调契约（全部透传 xrt 原生对象）
 * ============================================================ */

/* ws / wss：xs 完成握手并 xrtWsStreamAttach，回调收 xwsstream* */
typedef void (*XS_WsOpenProc)(XS_HostInfo* pHost, xwsstream* pWs);
typedef void (*XS_WsTextProc)(XS_HostInfo* pHost, xwsstream* pWs, xstrview tText);
typedef void (*XS_WsBinaryProc)(XS_HostInfo* pHost, xwsstream* pWs, xbytesview tData);
typedef void (*XS_WsPingProc)(XS_HostInfo* pHost, xwsstream* pWs, xbytesview tData);
typedef void (*XS_WsPongProc)(XS_HostInfo* pHost, xwsstream* pWs, xbytesview tData);
typedef void (*XS_WsCloseProc)(XS_HostInfo* pHost, xwsstream* pWs, uint16 iCode, xstrview tReason);

/* tcp / tcp+tls：裸流事件透传。
 * XS_StreamConn 为视图结构体（回调栈上分配）：tcp 与 tls 二选一非空，
 * 应用按非空一侧使用对应 xrt 发送 API（tcps 下禁止绕过 tls 直写 tcp）。
 * 连接生命周期由 xs 驱动持有（Accept 接管 → 注册表 → Close 时 Destroy），
 * 应用不负责 Destroy —— 与 custom 类"一切手动"不同，见设计 §6.3 */
typedef struct XS_StreamConn {
	xnetstream*			tcp;		/* 明文 TCP 传输层 */
	xtlsstream*			tls;		/* TLS 传输层（tcps） */
} XS_StreamConn;

typedef void (*XS_EventOpenProc)(XS_HostInfo* pHost, XS_StreamConn* pConn);
typedef void (*XS_EventDataProc)(XS_HostInfo* pHost, XS_StreamConn* pConn, xnetbuf* pBuffer);
typedef void (*XS_EventCloseProc)(XS_HostInfo* pHost, XS_StreamConn* pConn, xnetresult iResult, const xerror* pError);

/* udp */
typedef void (*XS_EventDgramProc)(XS_HostInfo* pHost, xnetudp* pUdp, const xnetudpmessage* pMsg);

/* 生命周期（全部可选导出；ServiceSwap 的交接数据在新代 ServiceInit 时经
 * xsSwapTake 取回。Swap 必须是无破坏的快照导出：在新代正式发布前，旧代
 * 仍可能继续服务或因极端 listener 终态而取消提交，见设计 §10） */
typedef void (*XS_ServiceInitProc)(XS_HostInfo* pHost);
typedef void (*XS_ServiceUnitProc)(XS_HostInfo* pHost);
typedef bool (*XS_ServiceSwapProc)(XS_HostInfo* pHost, xvalue** ppShared);

/* 脚本需导出的符号名（TCC 宿主按此查找；custom 类由应用自定装配） */
#define XS_SYM_SERVICE_INIT	"ServiceInit"
#define XS_SYM_SERVICE_UNIT	"ServiceUnit"
#define XS_SYM_SERVICE_SWAP	"ServiceSwap"
#define XS_SYM_REQUEST_PROC	"RequestProc"
#define XS_SYM_WS_OPEN		"WsOpen"
#define XS_SYM_WS_TEXT		"WsText"
#define XS_SYM_WS_BINARY	"WsBinary"
#define XS_SYM_WS_PING		"WsPing"
#define XS_SYM_WS_PONG		"WsPong"
#define XS_SYM_WS_CLOSE		"WsClose"
#define XS_SYM_EVENT_OPEN	"EventOpen"
#define XS_SYM_EVENT_DATA	"EventData"
#define XS_SYM_EVENT_CLOSE	"EventClose"
#define XS_SYM_EVENT_DGRAM	"EventDgram"

/* ============================================================
 * xs API（其余一切能力皆为 xrt 或可选库符号）
 * ============================================================ */

/* 配置访问：
 * - xsServerFind 返回 retained server lease，必须 xsServerRelease；
 * - xsServerRetain 用于延长连接/枚举回调内借用指针的生命周期；
 * - 枚举参数只在回调内借用，如需保存必须 Retain；
 * - xsHostFind/xsEnumHosts 的 host 由其 server lease 共同保护；
 * - xsConfigRoot 返回 retained xvalue，必须 xrtValueRelease。 */
typedef bool (*XS_ServerEnumProc)(XS_ServerInfo* pServer, void* pUserData);
typedef bool (*XS_HostEnumProc)(XS_HostInfo* pHost, void* pUserData);
XS_API XS_ServerInfo* xsServerFind(const char* sName);
XS_API XS_ServerInfo* xsServerRetain(XS_ServerInfo* pServer);
XS_API void xsServerRelease(XS_ServerInfo* pServer);
XS_API XS_HostInfo* xsHostFind(XS_ServerInfo* pServer, const char* sName);
XS_API void xsEnumServers(XS_ServerEnumProc procEach, void* pUserData);
XS_API void xsEnumHosts(XS_ServerInfo* pServer, XS_HostEnumProc procEach, void* pUserData);
XS_API xvalue* xsConfigRoot(void);			/* xs.json 顶层 Custom */

/* 重载协调器：Submit 返回 0 表示未受理；非零 ID 可用 xsReloadQuery 查询终态。
 * host/server 都以完整 server generation 为发布单元，同 server 旧意图会被
 * 最新意图覆盖；失败时旧代原样服务。
 * bool 入口是兼容包装，只表示 Submit 是否返回非零。 */
XS_API XS_ReloadId xsReloadHostSubmit(XS_HostInfo* pHost);
XS_API XS_ReloadId xsReloadServerSubmit(const char* sName);
XS_API XS_ReloadId xsReloadAllSubmit(void);
XS_API bool xsReloadQuery(XS_ReloadId iId, XS_ReloadResult* pResult);
XS_API bool xsReloadHost(XS_HostInfo* pHost);
XS_API bool xsReloadServer(const char* sName);
XS_API bool xsReloadAll(void);

/* 生命周期定时器：绑定 owner 所属 generation，换代自动作废旧代 */
typedef void (*XS_TimerProc)(void* pUserData);
XS_API uint64 xsTimerAfter(XS_HostInfo* pOwner, uint32 iMillisecond, XS_TimerProc proc, void* pUserData);
XS_API bool xsTimerCancel(uint64 iTimerId);

/* 部署信息与嵌套编译（xsCreateTCC 预置标准 include/lib 路径与符号导入） */
XS_API const char* xsAppPath(void);
XS_API TCCState* xsCreateTCC(void);
XS_API void xsDestroyTCC(TCCState* pTcc);

/* 重载交接：仅在新代 ServiceInit 内有效，取走旧代 ServiceSwap 导出的数据；
 * 无交接数据返回 NULL。取走后所有权转移给应用（xrtValueRelease 释放）。 */
XS_API xvalue* xsSwapTake(XS_HostInfo* pHost);

XS_EXTERN_END

#endif
