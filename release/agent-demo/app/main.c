/*
 * agent-demo — NativeHarness agent 前端的 C 脚本后端
 *
 * 架构：前端静态文件由 xs 静态层服务（wwwroot/）；本脚本处理 /api/* 路由。
 * 后端是"回合运行器"：POST /api/prompt 启动一个脚本化 agent 回合，
 * 事件按时间偏移逐拍生成；前端轮询 GET /api/turn/<id>/events?since=N 拉取。
 * 历史与持久化全部在前端（事件溯源 + localStorage），后端无会话状态。
 *
 * 宿主接口映射（wire.js XsHttpHost）：
 *   POST /api/prompt              {text}              → 启动回合，返回 {turnId, scriptId}
 *   GET  /api/turn/<id>/events    ?since=seq           → 到期事件数组
 *   POST /api/turn/<id>/approval  {decision}           → 审批决议，解除暂停
 *   POST /api/turn/<id>/cancel                         → 取消（标记 done）
 */

#include <string.h>
#include <xsbase.h>
#include <stdio.h>
#include <stdlib.h>

/* ================================================================== */
/* HTTP 基础（复用 demo-single 的模式；独立内联避免依赖外部模块）           */
/* ================================================================== */

static bool ConnSend(XS_HttpReq* pReq, const void* pData, size_t iSize)
{
	size_t iWritten = 0;
	if ( pReq == NULL || pData == NULL ) return false;
	if ( pReq->tls != NULL ) {
		return xrtTlsStreamSend(pReq->tls, pData, iSize, &iWritten) == XTLS_OK &&
		       iWritten == iSize;
	}
	return pReq->tcp != NULL &&
		xrtNetStreamSend(pReq->tcp, pData, iSize) == XNET_RESULT_OK;
}

static bool ReplyJSON(XS_HttpReq* pReq, uint16 iStatus, const char* sBody)
{
	char aHead[512];
	char aLen[24];
	xhttpfield aF[3];
	size_t nF = 0, iHeadSize = 0;
	xstrview tReason;

	snprintf(aLen, sizeof(aLen), "%llu", (unsigned long long)strlen(sBody));
	aF[nF].Name = XRT_STR_LITERAL("Content-Length");
	aF[nF].Value = xrtStrView(aLen); nF++;
	aF[nF].Name = XRT_STR_LITERAL("Content-Type");
	aF[nF].Value = xrtStrView("application/json; charset=utf-8"); nF++;
	aF[nF].Name = XRT_STR_LITERAL("Access-Control-Allow-Origin");
	aF[nF].Value = xrtStrView("*"); nF++;

	tReason = xrtHttpStatusText(iStatus);
	if ( !xrtHttp1ResponseWrite(XHTTP_VERSION_1_1, iStatus, tReason,
	     aF, nF, aHead, sizeof(aHead), &iHeadSize) )
		return false;
	if ( !ConnSend(pReq, aHead, iHeadSize) ) return false;
	if ( pReq->head->MethodCode != XHTTP_METHOD_HEAD )
		return ConnSend(pReq, sBody, strlen(sBody));
	return true;
}

static bool ReplyCORSOptions(XS_HttpReq* pReq)
{
	const char* s =
		"HTTP/1.1 204 No Content\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
		"Access-Control-Allow-Headers: Content-Type\r\n"
		"Content-Length: 0\r\n\r\n";
	return ConnSend(pReq, s, strlen(s));
}

/* 读取完整请求体：Peek 到栈缓冲再喂 body 解码器（demo-single 同款模式） */
static char* ReqBody(XS_HttpReq* pReq, size_t* pSize)
{
	const xnetbuf* pBuf;
	unsigned char aChunk[4096];
	char* sOut = NULL;
	size_t iOut = 0;
	size_t iBufOff = 0;
	size_t iBufSize;

	*pSize = 0;
	if ( pReq == NULL || pReq->body == NULL ) return NULL;
	pBuf = pReq->tcp != NULL ? xrtNetStreamBuffer(pReq->tcp) :
	       xrtTlsStreamBuffer(pReq->tls);
	if ( pBuf == NULL ) return NULL;
	iBufSize = xrtNetBufSize(pBuf);
	while ( iBufOff < iBufSize ) {
		size_t iGot = xrtNetBufPeek(pBuf, iBufOff, aChunk, sizeof(aChunk));
		xhttp1errorinfo tErr;
		xhttp1bodystatus eBody;
		xbytesview tData;
		xbytesview tIn;
		size_t iConsumed = 0;

		if ( iGot == 0 ) break;
		tIn.Data = aChunk;
		tIn.Size = iGot;
		eBody = xrtHttp1BodyRead(pReq->body, tIn, false, &iConsumed, &tData, &tErr);
		iBufOff += iConsumed;
		if ( tData.Size > 0 ) {
			char* pNew = (char*)xrtRealloc(sOut, iOut + tData.Size + 1);
			if ( pNew == NULL ) { xrtFree(sOut); return NULL; }
			sOut = pNew;
			memcpy(sOut + iOut, tData.Data, tData.Size);
			iOut += tData.Size;
			sOut[iOut] = 0;
		}
		if ( eBody == XHTTP1_BODY_DONE || eBody == XHTTP1_BODY_ERROR ) break;
	}
	*pSize = iOut;
	return sOut;
}

/* 从 JSON 里取字符串字段（简单扫描，避免完整 parse 的开销） */
static bool JsonStr(const char* sJson, const char* sKey, char* pOut, size_t iCap)
{
	char aPat[64];
	const char* p;
	size_t n;

	snprintf(aPat, sizeof(aPat), "\"%s\"", sKey);
	p = strstr(sJson, aPat);
	if ( p == NULL ) return false;
	p += strlen(aPat);
	while ( *p == ' ' || *p == ':' || *p == ' ' ) p++;
	if ( *p != '"' ) return false;
	p++;
	n = 0;
	while ( p[n] != '"' && p[n] != 0 && n < iCap - 1 ) {
		pOut[n] = p[n];
		n++;
	}
	pOut[n] = 0;
	return n > 0;
}

/* ================================================================== */
/* 回合引擎                                                             */
/* ================================================================== */

#define MAX_TURNS 8
#define MAX_EVENTS 128
#define MAX_BEATS 64

typedef struct Beat {
	int atMs;                  /* 相对回合开始的偏移（或审批解除后偏移） */
	int gated;                 /* 1 = 需审批后才继续 */
	const char* sType;         /* 事件类型 */
	const char* sData;         /* 事件 JSON 数据（静态字符串或 turn 内嵌缓冲） */
} Beat;

typedef struct Turn {
	char id[24];
	int active;
	int scriptId;
	int done;
	int cancelled;
	uint64_t startMs;          /* xrtNow()/1000 毫秒 */
	int nextBeat;              /* 下一个要发射的 beat 下标 */
	int approvalGated;         /* 当前被审批阻塞的 beat 下标（-1 = 无阻塞） */
	char approvalDecision[32]; /* 用户决议 */
	uint64_t approvalAtMs;     /* 审批解除时刻 */

	/* 事件缓冲：每事件一行 JSON（seq\ttime\ttype\tdata）——极简，
	   轮询时按 since 过滤重新打包为 JSON 数组 */
	struct {
		int seq;
		uint64_t time;
		const char* sType;
		char* sData;           /* heap 或指向脚本的 static */
	} events[MAX_EVENTS];
	int nEvents;

	/* 用户文本（echo 到 user/message 事件） */
	char userText[1024];
} Turn;

static Turn g_Turns[MAX_TURNS];
static int g_SeqCounter = 0;

static uint64_t NowMs(void) { return xrtNow() / 1000; }

static Turn* TurnNew(int scriptId, const char* sUserText)
{
	for ( int i = 0; i < MAX_TURNS; i++ ) {
		Turn* t = &g_Turns[i];
		if ( t->active ) continue;
		memset(t, 0, sizeof(*t));
		t->active = 1;
		t->scriptId = scriptId;
		t->startMs = NowMs();
		t->approvalGated = -1;
		snprintf(t->id, sizeof(t->id), "t%d", i);
		snprintf(t->userText, sizeof(t->userText), "%s", sUserText);
		return t;
	}
	return NULL;
}

static Turn* TurnFind(const char* sId)
{
	if ( sId == NULL ) return NULL;
	for ( int i = 0; i < MAX_TURNS; i++ )
		if ( g_Turns[i].active && strcmp(g_Turns[i].id, sId) == 0 )
			return &g_Turns[i];
	return NULL;
}

static void TurnAppend(Turn* t, const char* sType, const char* sData)
{
	if ( t->nEvents >= MAX_EVENTS ) return;
	t->events[t->nEvents].seq = ++g_SeqCounter;
	t->events[t->nEvents].time = NowMs();
	t->events[t->nEvents].sType = sType;
	/* 静态字符串直接借用；动态的由调用方 heap 分配 */
	t->events[t->nEvents].sData = (char*)sData;
	t->nEvents++;
}

/* ================================================================== */
/* 脚本定义：每个脚本是一张 Beat 表 + 生成 user/title 的动态部分           */
/* ================================================================== */

/* 简单文本回合：思考 → 流式回答 → 统计 */
static const Beat s_ScriptText[] = {
	{  0, 0, "session/running", "{\"value\":true}" },
	{300, 0, "assistant/reasoning", "{\"delta\":\"用户在问一个问题。\"}" },
	{600, 0, "assistant/reasoning", "{\"delta\":\"这个问题可以直接回答，不需要调用工具。\"}" },
	{900, 0, "assistant/chunk", "{\"delta\":\"好的，\"}" },
	{1100, 0, "assistant/chunk", "{\"delta\":\"我来回答这个问题。\"}" },
	{1300, 0, "assistant/message",
		"{\"message\":{\"model\":\"fx-1\",\"content\":[{\"type\":\"text\",\"text\":\"这是一个来自 C 脚本后端的演示回答。\\n\\n当前架构：\\n- 前端事件溯源渲染（零依赖）\\n- C 脚本回合引擎按时间拍生成事件\\n- 轮询拉取（无 WebSocket）\\n\\n`POST /api/prompt` 启动回合，`GET /api/turn/<id>/events?since=N` 按序拉取。\"}],\"usage\":{\"prompt\":120,\"completion\":45}}}" },
	{1400, 0, "session/stats", "{\"ctxTokens\":165,\"ms\":1400,\"tps\":32.1,\"promptTokens\":120,\"completionTokens\":45}" },
	{1500, 0, "session/running", "{\"value\":false}" },
};
#define SCRIPT_TEXT_COUNT (int)(sizeof(s_ScriptText) / sizeof(s_ScriptText[0]))

/* 工具+审批+diff 回合 */
static const Beat s_ScriptTools[] = {
	{  0, 0, "session/running", "{\"value\":true}" },
	{200, 0, "assistant/reasoning", "{\"delta\":\"需要查看构建脚本来分析。\"}" },
	{400, 0, "assistant/message",
		"{\"message\":{\"model\":\"fx-1\",\"content\":[{\"type\":\"text\",\"text\":\"让我先看看构建配置。\"},{\"type\":\"tool-call\",\"callId\":\"c1\",\"name\":\"glob\",\"input\":{\"pattern\":\"*.json\"}}],\"usage\":{\"prompt\":89,\"completion\":12}}}" },
	{700, 0, "tool/result",
		"{\"callId\":\"c1\",\"output\":\"scripts/build.mjs\\npackage.json\\ntsconfig.json\",\"isError\":false,\"ms\":220}" },
	{900, 0, "assistant/message",
		"{\"message\":{\"model\":\"fx-1\",\"content\":[{\"type\":\"tool-call\",\"callId\":\"c2\",\"name\":\"read\",\"input\":{\"path\":\"scripts/build.mjs\"}}],\"usage\":{\"prompt\":180,\"completion\":8}}}" },
	{1150, 0, "tool/result",
		"{\"callId\":\"c2\",\"output\":\"import { build } from './core/build.mjs';\\nbuild({ entry: 'src/app.ts', outDir: 'dist', sourcemap: true });\",\"isError\":false,\"ms\":180}" },
	{1400, 0, "assistant/message",
		"{\"message\":{\"model\":\"fx-1\",\"content\":[{\"type\":\"text\",\"text\":\"构建配置清楚了。接下来我要修改 build.mjs 加一个 minify 选项。\"},{\"type\":\"tool-call\",\"callId\":\"c3\",\"name\":\"edit\",\"input\":{\"path\":\"scripts/build.mjs\"}}],\"usage\":{\"prompt\":350,\"completion\":22}}}" },
	/* 审批门：edit 工具需要用户确认 */
	{1450, 1, "approval/requested",
		"{\"id\":\"ap1\",\"kind\":\"edit\",\"title\":\"修改 scripts/build.mjs\",\"detail\":\"添加 minify: true 选项\"}" },
	/* 审批后的拍（atMs 相对审批解除时刻） */
	{ 100, 0, "assistant/chunk", "{\"delta\":\"修改已应用。\"}" },
	{ 300, 0, "tool/result",
		"{\"callId\":\"c3\",\"output\":\"已修改\",\"isError\":false,\"ms\":95,\"diff\":{\"file\":\"scripts/build.mjs\",\"hunks\":[{\"a\":\"sourcemap: true });\",\"b\":\"sourcemap: true,\\n  minify: true });\"}]}}" },
	{ 500, 0, "assistant/message",
		"{\"message\":{\"model\":\"fx-1\",\"content\":[{\"type\":\"text\",\"text\":\"已完成。变更内容：\\n\\n```diff\\n- sourcemap: true });\\n+ sourcemap: true,\\n+   minify: true });\\n```\\n\\n重新构建验证一下。\"},{\"type\":\"tool-call\",\"callId\":\"c4\",\"name\":\"bash\",\"input\":{\"cmd\":\"node scripts/build.mjs\"}}],\"usage\":{\"prompt\":520,\"completion\":28}}}" },
	{ 800, 0, "tool/result",
		"{\"callId\":\"c4\",\"output\":\"Build complete: dist/app.js (45.2KB → 31.8KB minified)\",\"isError\":false,\"ms\":2400,\"plan\":[{\"text\":\"分析构建配置\",\"done\":true},{\"text\":\"修改 build.mjs\",\"done\":true},{\"text\":\"验证构建\",\"done\":true}]}" },
	{ 950, 0, "assistant/message",
		"{\"message\":{\"model\":\"fx-1\",\"content\":[{\"type\":\"text\",\"text\":\"构建成功，minify 后体积从 45.2KB 降到 31.8KB（-30%）。\\n\\n| 指标 | 修改前 | 修改后 |\\n|---|---|---|\\n| 体积 | 45.2KB | 31.8KB |\\n| sourcemap | 有 | 有 |\\n| minify | 无 | 有 |\\n\\n3 步全部完成。\"}],\"usage\":{\"prompt\":700,\"completion\":35}}}" },
	{1050, 0, "session/stats", "{\"ctxTokens\":735,\"ms\":6800,\"tps\":28.4,\"promptTokens\":700,\"completionTokens\":35}" },
	{1100, 0, "session/running", "{\"value\":false}" },
};
#define SCRIPT_TOOLS_COUNT (int)(sizeof(s_ScriptTools) / sizeof(s_ScriptTools[0]))

/* 分析+出图回合 */
static const Beat s_ScriptChart[] = {
	{  0, 0, "session/running", "{\"value\":true}" },
	{250, 0, "assistant/reasoning", "{\"delta\":\"用户想看构建趋势。\"}" },
	{400, 0, "assistant/message",
		"{\"message\":{\"model\":\"fx-1\",\"content\":[{\"type\":\"text\",\"text\":\"让我拉取最近的构建数据。\"},{\"type\":\"tool-call\",\"callId\":\"c1\",\"name\":\"bash\",\"input\":{\"cmd\":\"cat build.log | tail -4\"}}],\"usage\":{\"prompt\":60,\"completion\":10}}}" },
	{650, 0, "tool/result",
		"{\"callId\":\"c1\",\"output\":\"build #38: 15.2s\\nbuild #39: 18.1s\\nbuild #40: 16.4s\\nbuild #41: 12.0s\",\"isError\":false,\"ms\":160}" },
	{850, 0, "assistant/message",
		"{\"message\":{\"model\":\"fx-1\",\"content\":[{\"type\":\"text\",\"text\":\"数据到手，生成图表。\"},{\"type\":\"tool-call\",\"callId\":\"c2\",\"name\":\"chart\",\"input\":{\"type\":\"bar\"}}],\"usage\":{\"prompt\":210,\"completion\":8}}}" },
	{1050, 0, "tool/result",
		"{\"callId\":\"c2\",\"output\":\"chart.svg\",\"isError\":false,\"ms\":320}" },
	{1200, 0, "assistant/message",
		"{\"message\":{\"model\":\"fx-1\",\"content\":[{\"type\":\"text\",\"text\":\"## 构建耗时趋势\\n\\n最近 4 次构建平均 **15.4s**，最新一次降到 12.0s（-25%）。\\n\\n- 最快：#41（12.0s）\\n- 最慢：#39（18.1s）\\n- 趋势：下降（缓存命中增加）\\n\\n建议：保持当前缓存策略。\"}],\"usage\":{\"prompt\":380,\"completion\":42}}}" },
	{1300, 0, "session/stats", "{\"ctxTokens\":422,\"ms\":2800,\"tps\":35.2,\"promptTokens\":380,\"completionTokens\":42}" },
	{1350, 0, "session/running", "{\"value\":false}" },
};
#define SCRIPT_CHART_COUNT (int)(sizeof(s_ScriptChart) / sizeof(s_ScriptChart[0]))

static const Beat* BeatTable(int scriptId, int* pCount)
{
	switch ( scriptId ) {
	case 0: *pCount = SCRIPT_TEXT_COUNT; return s_ScriptText;
	case 1: *pCount = SCRIPT_TOOLS_COUNT; return s_ScriptTools;
	case 2: *pCount = SCRIPT_CHART_COUNT; return s_ScriptChart;
	}
	*pCount = 0;
	return NULL;
}

static const char* ScriptTitle(int scriptId, const char* sUser)
{
	(void)sUser;
	switch ( scriptId ) {
	case 0: return "C 后端文本演示";
	case 1: return "工具调用与审批";
	case 2: return "构建趋势分析";
	}
	return "Agent 会话";
}

/* 选择脚本：按用户文本关键词路由（演示用，接真实后端换 LLM 路由） */
static int PickScript(const char* sText)
{
	if ( strstr(sText, "/tools") || strstr(sText, "工具") ||
	     strstr(sText, "构建") || strstr(sText, "build") ) return 1;
	if ( strstr(sText, "/chart") || strstr(sText, "图") ||
	     strstr(sText, "趋势") || strstr(sText, "分析") ) return 2;
	return 0;
}

/* ================================================================== */
/* 回合驱动：每次轮询时推进                                             */
/* ================================================================== */

static void TurnAdvance(Turn* t)
{
	if ( !t->active || t->done || t->cancelled ) return;

	int nBeats = 0;
	const Beat* pBeats = BeatTable(t->scriptId, &nBeats);
	if ( pBeats == NULL ) { t->done = 1; return; }

	/* 计算当前时间锚点：审批解除过则以解除时刻起算（后续拍 atMs 相对该时刻） */
	uint64_t anchor = ( t->approvalAtMs > 0 ) ? t->approvalAtMs : t->startMs;
	uint64_t elapsed = NowMs() - anchor;

	while ( t->nextBeat < nBeats ) {
		const Beat* b = &pBeats[t->nextBeat];

		/* 审批未解除：完全暂停（不看时间，防止后续拍超时漏过门） */
		if ( t->approvalGated >= 0 ) return;

		/* 审批门：gated 拍首次到达 */
		if ( b->gated ) {
			t->approvalGated = t->nextBeat;
			TurnAppend(t, b->sType, b->sData);
			t->nextBeat++;  /* 审拍已发，等解除后从下一拍继续 */
			return;
		}

		/* 非门控拍：时间到了就发 */
		if ( (long long)elapsed >= b->atMs ) {
			TurnAppend(t, b->sType, b->sData);
			t->nextBeat++;
			if ( strcmp(b->sType, "session/running") == 0 &&
			     strstr(b->sData, "\"value\":false") ) {
				t->done = 1;
				return;
			}
		} else {
			break;  /* 下一拍还没到时间 */
		}
	}

	if ( t->nextBeat >= nBeats ) t->done = 1;
}

/* ================================================================== */
/* JSON 输出                                                            */
/* ================================================================== */

static void TurnEventsJSON(Turn* t, int sinceSeq, char* pOut, size_t iCap)
{
	size_t j = 0;

	j += snprintf(pOut + j, iCap - j, "{\"ok\":true,\"turnId\":\"%s\",\"done\":%s,\"events\":[",
		t->id, t->done ? "true" : "false");
	bool bFirst = true;
	for ( int i = 0; i < t->nEvents && j < iCap - 512; i++ ) {
		if ( t->events[i].seq <= sinceSeq ) continue;
		if ( !bFirst ) pOut[j++] = ',';
		bFirst = false;
		j += snprintf(pOut + j, iCap - j,
			"{\"seq\":%d,\"time\":%llu,\"type\":\"%s\",\"data\":%s}",
			t->events[i].seq,
			(unsigned long long)t->events[i].time,
			t->events[i].sType,
			t->events[i].sData);
	}
	j += snprintf(pOut + j, iCap - j, "]}");
}

/* ================================================================== */
/* 路由处理                                                             */
/* ================================================================== */

int RequestProc(XS_HttpReq* pReq)
{
	char aPath[256];
	char aQuery[128] = {0};

	if ( pReq == NULL || pReq->head == NULL ) return XS_FALLBACK;

	{
		xstrview tUri = pReq->head->Target;
		size_t nUri = tUri.Size;
		const char* pQ = tUri.Size > 0 ? memchr(tUri.Data, '?', tUri.Size) : NULL;

		if ( pQ != NULL ) {
			/* query 单独保存（since=N 从这里取） */
			size_t nQ = tUri.Size - (size_t)(pQ - tUri.Data) - 1;

			if ( nQ >= sizeof(aQuery) ) nQ = sizeof(aQuery) - 1;
			memcpy(aQuery, pQ + 1, nQ);
			aQuery[nQ] = 0;
			nUri = (size_t)(pQ - tUri.Data);
		}
		if ( nUri >= sizeof(aPath) ) return XS_FALLBACK;
		if ( nUri > 0 ) memcpy(aPath, tUri.Data, nUri);
		aPath[nUri] = 0;
	}

	/* CORS preflight */
	if ( pReq->head->MethodCode == XHTTP_METHOD_OPTIONS ) {
		return ReplyCORSOptions(pReq) ? XS_OK : XS_OK;
	}

	/* 只处理 /api/ 前缀 */
	if ( strncmp(aPath, "/api/", 5) != 0 ) return XS_FALLBACK;

	/* ---- POST /api/prompt ---- */
	if ( strcmp(aPath, "/api/prompt") == 0 &&
	     pReq->head->MethodCode == XHTTP_METHOD_POST ) {
		size_t iBody = 0;
		char* sBody = ReqBody(pReq, &iBody);
		char aText[512] = {0};

		if ( sBody == NULL ) {
			return ReplyJSON(pReq, 400, "{\"ok\":false,\"error\":\"empty body\"}") ? XS_OK : XS_OK;
		}
		JsonStr(sBody, "text", aText, sizeof(aText));
		if ( aText[0] == 0 ) {
			xrtFree(sBody);
			return ReplyJSON(pReq, 400, "{\"ok\":false,\"error\":\"missing text\"}") ? XS_OK : XS_OK;
		}

		{
			int scriptId = PickScript(aText);
			Turn* t = TurnNew(scriptId, aText);
			char aResp[256];
			char aTitle[128];

			if ( t == NULL ) {
				xrtFree(sBody);
				return ReplyJSON(pReq, 503, "{\"ok\":false,\"error\":\"too many turns\"}") ? XS_OK : XS_OK;
			}

			/* 立即产生 title + user/message 事件 */
			snprintf(aTitle, sizeof(aTitle), "{\"title\":\"%s\"}", ScriptTitle(scriptId, aText));
			TurnAppend(t, "session/title", strdup(aTitle));

			{
				/* user/message 事件（转义用户文本中的引号和反斜杠） */
				char aEsc[1024] = {0};
				size_t j = 0;
				for ( size_t k = 0; aText[k] && j < sizeof(aEsc) - 8; k++ ) {
					if ( aText[k] == '"' || aText[k] == '\\' ) aEsc[j++] = '\\';
					aEsc[j++] = aText[k];
				}
				char aUser[1200];
				snprintf(aUser, sizeof(aUser),
					"{\"content\":[{\"type\":\"text\",\"text\":\"%s\"}]}", aEsc);
				TurnAppend(t, "user/message", strdup(aUser));
			}

			snprintf(aResp, sizeof(aResp),
				"{\"ok\":true,\"turnId\":\"%s\",\"scriptId\":%d}", t->id, scriptId);
			xrtFree(sBody);
			return ReplyJSON(pReq, 200, aResp) ? XS_OK : XS_OK;
		}
	}

	/* ---- GET /api/turn/<id>/events ---- */
	if ( strncmp(aPath, "/api/turn/", 10) == 0 &&
	     strstr(aPath, "/events") != NULL &&
	     pReq->head->MethodCode == XHTTP_METHOD_GET ) {
		char aTurnId[24] = {0};
		int since = 0;
		const char* p;

		/* 提取 turnId（/api/turn/<id>/events?since=N） */
		{
			const char* pStart = aPath + 10;
			const char* pEnd = strstr(pStart, "/events");
			if ( pEnd == NULL || (size_t)(pEnd - pStart) >= sizeof(aTurnId) ) {
				return ReplyJSON(pReq, 404, "{\"ok\":false,\"error\":\"bad turn path\"}") ? XS_OK : XS_OK;
			}
			memcpy(aTurnId, pStart, (size_t)(pEnd - pStart));
			aTurnId[pEnd - pStart] = 0;
		}
		/* since 参数（在保留的 query 里找） */
		p = strstr(aQuery, "since=");
		if ( p != NULL ) since = atoi(p + 6);

		{
			Turn* t = TurnFind(aTurnId);
			if ( t == NULL ) {
				return ReplyJSON(pReq, 404, "{\"ok\":false,\"error\":\"turn not found\"}") ? XS_OK : XS_OK;
			}
			TurnAdvance(t);
			{
				/* 8KB 输出缓冲（最多 128 事件，每事件约 60 字节均值 + 大 JSON 消息） */
				static char aOut[64 * 1024];
				TurnEventsJSON(t, since, aOut, sizeof(aOut));
				return ReplyJSON(pReq, 200, aOut) ? XS_OK : XS_OK;
			}
		}
	}

	/* ---- POST /api/turn/<id>/approval ---- */
	if ( strncmp(aPath, "/api/turn/", 10) == 0 &&
	     strstr(aPath, "/approval") != NULL &&
	     pReq->head->MethodCode == XHTTP_METHOD_POST ) {
		char aTurnId[24] = {0};
		const char* pStart = aPath + 10;
		const char* pEnd = strstr(pStart, "/approval");

		if ( pEnd == NULL || (size_t)(pEnd - pStart) >= sizeof(aTurnId) ) {
			return ReplyJSON(pReq, 404, "{\"ok\":false,\"error\":\"bad path\"}") ? XS_OK : XS_OK;
		}
		memcpy(aTurnId, pStart, (size_t)(pEnd - pStart));
		aTurnId[pEnd - pStart] = 0;

		{
			Turn* t = TurnFind(aTurnId);
			size_t iBody = 0;
			char* sBody;
			char aDecision[32] = "allow-once";

			if ( t == NULL || t->approvalGated < 0 ) {
				return ReplyJSON(pReq, 404, "{\"ok\":false,\"error\":\"no pending approval\"}") ? XS_OK : XS_OK;
			}
			sBody = ReqBody(pReq, &iBody);
			if ( sBody != NULL ) {
				JsonStr(sBody, "decision", aDecision, sizeof(aDecision));
				xrtFree(sBody);
			}

			/* 解除：追加 approval/resolved 事件，重置锚点 */
			{
				char aEv[128];
				snprintf(aEv, sizeof(aEv), "{\"id\":\"ap1\",\"decision\":\"%s\"}", aDecision);
				TurnAppend(t, "approval/resolved", strdup(aEv));
			}
			t->approvalGated = -1;
			t->approvalAtMs = NowMs();
			return ReplyJSON(pReq, 200, "{\"ok\":true}") ? XS_OK : XS_OK;
		}
	}

	/* ---- POST /api/turn/<id>/cancel ---- */
	if ( strncmp(aPath, "/api/turn/", 10) == 0 &&
	     strstr(aPath, "/cancel") != NULL &&
	     pReq->head->MethodCode == XHTTP_METHOD_POST ) {
		char aTurnId[24] = {0};
		const char* pStart = aPath + 10;
		const char* pEnd = strstr(pStart, "/cancel");

		if ( pEnd != NULL && (size_t)(pEnd - pStart) < sizeof(aTurnId) ) {
			memcpy(aTurnId, pStart, (size_t)(pEnd - pStart));
			aTurnId[pEnd - pStart] = 0;
			{
				Turn* t = TurnFind(aTurnId);
				if ( t != NULL ) {
					t->cancelled = 1;
					t->done = 1;
					TurnAppend(t, "session/interrupted", "{}");
					TurnAppend(t, "session/running", "{\"value\":false}");
				}
			}
		}
		return ReplyJSON(pReq, 200, "{\"ok\":true}") ? XS_OK : XS_OK;
	}

	return XS_FALLBACK;
}

/* ================================================================== */

void ServiceInit(XS_HostInfo* pHost)
{
	(void)pHost;
	printf("[agent-demo] C backend ready (turn engine + 3 scripts)\n");
	for ( int i = 0; i < MAX_TURNS; i++ ) g_Turns[i].active = 0;
}

void ServiceUnit(XS_HostInfo* pHost)
{
	(void)pHost;
	for ( int i = 0; i < MAX_TURNS; i++ ) {
		Turn* t = &g_Turns[i];
		for ( int j = 0; j < t->nEvents; j++ ) {
			/* 静态脚本指针不释放，只释放 heap 分配的动态事件 */
			/* strdup 产生的需要释放——但 TCC 脚本进程退出时统一回收 */
		}
		t->active = 0;
	}
}
