/*
 * demo-chat — llama.cpp Web UI 风格的流式对话服务（xs + xllm + xllm-session）
 *
 * 架构（文档 ch34 §34.3 + ch37 §37.4 + ch22 §22.2 的标准拼装）：
 *   WsText(引擎线程) ──chat:<text>──▶ 后台线程跑 xllmClientComplete
 *        xllm 流式回调(xllm 引擎线程) ──▶ 帧队列(g_lock)
 *   自重排定时器 25ms(引擎线程) ──▶ 从队列取帧 xrtWsStreamText 推给浏览器
 *
 * 简版 Agent 循环：响应带 tool_calls → 本地执行内置工具 → 回填 → 再调
 * （最多 6 轮）。会话治理：xllm-session v3 —— 精确 usage 计量 + Pi 压缩
 * （轮次边界自动 MaybeCompact），窗口画像取值（回退显式配置），
 * 工具对原子由库保证；无滑窗丢历史。
 *
 * 浏览器 → 服务端纯文本协议： "chat:<text>" / "stop" / "clear"
 * 服务端 → 浏览器 JSON 帧：   start/delta/thinking/tool_start/tool_result/
 *                             usage/done/error/compacted
 */

#include <xsbase.h>
#ifdef XS_USE_XLLM
#include <xllm.h>
#endif
#ifdef XS_USE_XLLM_SESSION
#include <xllm-session.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(XS_USE_XLLM) || !defined(XS_USE_XLLM_SESSION)
/* 无 xllm/xllm-session 变体也允许编译：握手能通，回一条错误帧。 */
void WsOpen(XS_HostInfo* pHost, xwsstream* pWs) { (void)pHost; (void)pWs; }
void WsClose(XS_HostInfo* pHost, xwsstream* pWs, uint16 iCode, xstrview tReason)
{ (void)pHost; (void)pWs; (void)iCode; (void)tReason; }
void WsText(XS_HostInfo* pHost, xwsstream* pWs, xstrview tText)
{ (void)pHost; (void)tText; (void)pWs; }
#else

/* ------------------------------------------------------------------ */
/* 配置与全局状态                                                       */
/* ------------------------------------------------------------------ */

#define CHAT_MAX_TURNS      6u      /* agent 循环上限（含工具轮） */

typedef struct ChatFrame {
    char* sData;
    struct ChatFrame* pNext;
} ChatFrame;

typedef struct ChatConn {
    xwsstream* pWs;
    uint32 iWorkerIndex;   /* 拥有该连接的网络 Worker（发送亲和） */
    struct ChatConn* pNext;
    /* g_lock 保护以下全部字段；stop 例外（volatile，流式回调免锁探测） */
    ChatFrame* pFrames;
    ChatFrame** ppFramesTail;
    xllm_session* pSession;  /* 会话治理对象（轮次线程独占使用） */
    xthread* pWorker;
    int bBusy;
    int bClosed;
    volatile int bStop;
} ChatConn;

static XS_HostInfo* g_pHost;
static xnetengine* g_pEngine;
static xmutex* g_pLock;
static ChatConn* g_pConns;
static xllm_client* g_pClient;
static char g_sBaseUrl[256] = "https://open.bigmodel.cn/api/paas/v4";
static char g_sModel[64] = "glm-5.3-flash";
static char g_sApiKey[256] = "";
static uint64_t g_uContextWindow = 131072u;  /* 无画像时的回退窗口 */
static uint32_t g_uSessionMaxOut = 8192u;    /* llm.session_max_out 可调 */
static uint32_t g_uSessionSafety = 0u;       /* llm.session_safety 可调 */
static uint32_t g_uSessionKeepRecent = 0u;   /* llm.session_keep_recent 可调 */

/* ------------------------------------------------------------------ */
/* 小工具：动态字符串 / JSON 转义 / 帧队列                              */
/* ------------------------------------------------------------------ */

typedef struct {
    char* pData;
    size_t iLen, iCap;
} SBuf;

static int sb_reserve(SBuf* pB, size_t iNeed)
{
    size_t iCap;
    char* pNew;
    if ( pB->iLen + iNeed + 1u <= pB->iCap ) { return 1; }
    iCap = pB->iCap ? pB->iCap : 256u;
    while ( iCap < pB->iLen + iNeed + 1u ) { iCap *= 2u; }
    pNew = (char*)realloc(pB->pData, iCap);
    if ( !pNew ) { return 0; }
    pB->pData = pNew;
    pB->iCap = iCap;
    return 1;
}

static int sb_append(SBuf* pB, const char* sText, size_t iLen)
{
    if ( !sb_reserve(pB, iLen) ) { return 0; }
    memcpy(pB->pData + pB->iLen, sText, iLen);
    pB->iLen += iLen;
    pB->pData[pB->iLen] = 0;
    return 1;
}

static int sb_cstr(SBuf* pB, const char* sText)
{
    return sb_append(pB, sText, strlen(sText));
}

/* JSON 字符串转义追加（含正文中的双引号/反斜杠/控制字符）。 */
static int sb_json_escape(SBuf* pB, const char* sText, size_t iLen)
{
    size_t i;
    char sTemp[8];
    for ( i = 0u; i < iLen; ++i ) {
        unsigned char c = (unsigned char)sText[i];
        switch ( c ) {
            case '"': if ( !sb_cstr(pB, "\\\"") ) return 0; break;
            case '\\': if ( !sb_cstr(pB, "\\\\") ) return 0; break;
            case '\n': if ( !sb_cstr(pB, "\\n") ) return 0; break;
            case '\r': if ( !sb_cstr(pB, "\\r") ) return 0; break;
            case '\t': if ( !sb_cstr(pB, "\\t") ) return 0; break;
            default:
                if ( c < 0x20u ) {
                    sprintf(sTemp, "\\u%04x", (unsigned)c);
                    if ( !sb_cstr(pB, sTemp) ) return 0;
                } else {
                    if ( !sb_append(pB, sText + i, 1u) ) return 0;
                }
                break;
        }
    }
    return 1;
}

/* 组一个 JSON 帧并入队（调用方持 g_lock 或处于单线程阶段）。 */
static void frame_push(ChatConn* pConn, const char* sText)
{
    ChatFrame* pFrame = (ChatFrame*)malloc(sizeof(*pFrame));
    if ( !pFrame ) { return; }
    pFrame->sData = strdup(sText);
    pFrame->pNext = NULL;
    if ( !pFrame->sData ) { free(pFrame); return; }
    *pConn->ppFramesTail = pFrame;
    pConn->ppFramesTail = &pFrame->pNext;
}

static void frame_push_text_field(ChatConn* pConn, const char* sKind,
    const char* sText, size_t iLen)
{
    SBuf tB = {0, 0, 0};
    if ( sb_cstr(&tB, "{\"type\":\"") && sb_cstr(&tB, sKind) &&
         sb_cstr(&tB, "\",\"text\":\"") && sb_json_escape(&tB, sText, iLen) &&
         sb_cstr(&tB, "\"}") ) {
        xrtMutexLock(g_pLock);
        frame_push(pConn, tB.pData);
        xrtMutexUnlock(g_pLock);
    }
    free(tB.pData);
}

/* ------------------------------------------------------------------ */
/* xllm 流式回调（xllm 引擎线程触发）：只攒帧，不碰连接                  */
/* ------------------------------------------------------------------ */

static bool on_llm_event(void* pUserData, const xllm_event* pEvent)
{
    ChatConn* pConn = (ChatConn*)pUserData;
    if ( !pConn || !pEvent ) { return 1; }
    if ( pConn->bStop ) { return 0; } /* 停止按钮：false 即取消本次调用 */
    switch ( pEvent->eKind ) {
        case XLLM_EVENT_TEXT_DELTA:
            frame_push_text_field(pConn, "delta",
                pEvent->as.tText.sData, pEvent->as.tText.iLen);
            break;
        case XLLM_EVENT_REASONING_DELTA:
            frame_push_text_field(pConn, "thinking",
                pEvent->as.tText.sData, pEvent->as.tText.iLen);
            break;
        case XLLM_EVENT_TOOL_CALL_DELTA:
            /* 增量太碎；工具卡在收到完整调用时由工作线程发。 */
            break;
        default:
            break;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* 内置演示工具（Agent 循环的本地执行侧）                                */
/* ------------------------------------------------------------------ */

static void add_demo_tools(xllm_request* pRequest)
{
    xllmRequestAddTool(pRequest, "get_time",
        "Get the current local date and time",
        "{\"type\":\"object\",\"properties\":{}}", 0);
    xllmRequestAddTool(pRequest, "echo",
        "Echo the given JSON arguments back as text",
        "{\"type\":\"object\",\"properties\":{\"message\":{\"type\":\"string\"}}}", 0);
}

static int run_demo_tool(const char* sName, const char* sArgsJson,
    SBuf* pOut)
{
    if ( strcmp(sName, "get_time") == 0 ) {
        time_t tNow = time(NULL);
        struct tm* pTm = localtime(&tNow);
        char sWhen[64];
        strftime(sWhen, sizeof(sWhen), "%Y-%m-%d %H:%M:%S", pTm);
        return sb_cstr(pOut, sWhen);
    }
    if ( strcmp(sName, "echo" ) == 0 ) {
        return sb_cstr(pOut, sArgsJson ? sArgsJson : "{}");
    }
    return sb_cstr(pOut, "unknown tool");
}

/* ------------------------------------------------------------------ */
/* 会话治理（xllm-session v3）：画像取窗 + Pi 压缩，替代手搓滑窗          */
/* ------------------------------------------------------------------ */

/* 用户消息字节上限随窗口比例化（W/2 字节，夹在 2K..32K）：
 * 上限同时进入最坏增量包络，配得过大在小窗口下会恒判溢出。 */
static uint32_t session_user_cap(uint64_t uWindow)
{
    uint64_t uCap = uWindow / 2u;
    if ( uCap < 2048u ) { uCap = 2048u; }
    if ( uCap > 32u * 1024u ) { uCap = 32u * 1024u; }
    return (uint32_t)uCap;
}

static xllm_session* session_create(void)
{
    xllm_session_config tCfg;
    xllm_error tError;
    xllm_session* pSession;
    if ( !g_pClient ) { return NULL; }
    xllmSessionConfigInit(&tCfg);
    tCfg.uContextWindowTokens = 0u;   /* 优先取绑定 client 的模型画像 */
    tCfg.uMaxOutputTokens = g_uSessionMaxOut;
    tCfg.uSafetyReserveTokens = g_uSessionSafety;   /* 0 = 动态默认 */
    tCfg.uKeepRecentTokens = g_uSessionKeepRecent;  /* 0 = Pi 默认 */
    tCfg.uUserMessageCapBytes = session_user_cap(g_uContextWindow);
    pSession = xllmSessionCreateBound(&tCfg, g_pClient, &tError);
    if ( !pSession ) {
        printf("[demo-chat] profile window failed: %s\n", tError.sMessage);
        /* client 无画像：回退显式窗口（xs.json llm.context_window） */
        tCfg.uContextWindowTokens = g_uContextWindow;
        pSession = xllmSessionCreateBound(&tCfg, g_pClient, &tError);
        if ( !pSession ) {
            printf("[demo-chat] explicit window failed: %s\n", tError.sMessage);
        }
    }
    return pSession;
}

/* ------------------------------------------------------------------ */
/* 聊天回合工作线程（后台线程：阻塞 Complete 合法，不碰 ws 连接）          */
/* ------------------------------------------------------------------ */

typedef struct {
    ChatConn* pConn;
    char* sUserText;
} TurnArgs;

static void push_frame_locked(ChatConn* pConn, const char* sFrame)
{
    xrtMutexLock(g_pLock);
    frame_push(pConn, sFrame);
    xrtMutexUnlock(g_pLock);
}

static void run_turn(ChatConn* pConn, const char* sUserText)
{
    xllm_request tRequest;
    xllm_response* pResponse = NULL;
    xllm_error tError;
    xllm_stream_callbacks tCallbacks;
    xllm_session_stats tStats;
    uint32_t uRound;
    uint64_t uTurn;
    char sFrame[512];

    if ( !pConn->pSession ) {
        push_frame_locked(pConn,
            "{\"type\":\"error\",\"message\":\"session unavailable\"}");
        return;
    }

    uTurn = xllmSessionBeginTurn(pConn->pSession);
    sprintf(sFrame, "{\"type\":\"start\",\"turn\":%llu}",
        (unsigned long long)uTurn);
    push_frame_locked(pConn, sFrame);

    if ( !uTurn || !xllmSessionAddText(pConn->pSession, uTurn,
            XLLM_ROLE_USER, sUserText, 0u) ) {
        push_frame_locked(pConn,
            "{\"type\":\"error\",\"message\":\"message rejected (size cap?)\"}");
        return;
    }

    memset(&tCallbacks, 0, sizeof(tCallbacks));
    tCallbacks.pUserData = pConn;
    tCallbacks.OnEvent = on_llm_event;

    for ( uRound = 0u; uRound < CHAT_MAX_TURNS; ++uRound ) {
        size_t i;
        xllm_result eResult;

        xllmRequestInit(&tRequest);
        /* session 渲染：PINNED/system + 滚动摘要 + 尾窗（工具对原子）。
         * 溢出时走梯子（L1 压缩 / L2 尾窗截断）后重试一次。 */
        if ( !xllmSessionBuildRequest(pConn->pSession, &tRequest, &tError) ) {
            if ( xllmSessionOverflowLadder(pConn->pSession, &tError) ) {
                (void)xllmSessionGetStats(pConn->pSession, &tStats);
                sprintf(sFrame, "{\"type\":\"ladder\",\"generation\":%u}",
                    (unsigned)tStats.uSummaryGeneration);
                push_frame_locked(pConn, sFrame);
                xllmRequestUnit(&tRequest);
                xllmRequestInit(&tRequest);
            }
            if ( !xllmSessionBuildRequest(pConn->pSession, &tRequest, &tError) ) {
                SBuf tB = {0, 0, 0};
                if ( sb_cstr(&tB, "{\"type\":\"error\",\"message\":\"") &&
                     sb_json_escape(&tB, tError.sMessage, strlen(tError.sMessage)) &&
                     sb_cstr(&tB, "\"}") ) {
                    push_frame_locked(pConn, tB.pData);
                }
                free(tB.pData);
                xllmRequestUnit(&tRequest);
                break;
            }
        }
        xllmRequestSetModel(&tRequest, g_sModel);
        add_demo_tools(&tRequest);
        if ( pConn->bStop ) { xllmRequestUnit(&tRequest); break; }

        pResponse = NULL;
        eResult = xllmClientComplete(g_pClient, &tRequest, &tCallbacks,
            &pResponse, &tError);
        xllmRequestUnit(&tRequest);
        printf("[demo-chat] call result=%d http=%d\n", (int)eResult,
            tError.iHttpStatus);

        if ( eResult != XLLM_RESULT_OK || !pResponse ) {
            SBuf tB = {0, 0, 0};
            if ( sb_cstr(&tB, "{\"type\":\"error\",\"message\":\"") &&
                 sb_json_escape(&tB, tError.sMessage, strlen(tError.sMessage)) &&
                 sb_cstr(&tB, "\"}") ) {
                push_frame_locked(pConn, tB.pData);
            }
            free(tB.pData);
            break;
        }

        /* 入账即治理：usage 精确反馈 + 占用缓存刷新都在这一步 */
        if ( !xllmSessionAddAssistantResponse(pConn->pSession, uTurn, pResponse) ) {
            xllmResponseDestroy(pResponse);
            push_frame_locked(pConn,
                "{\"type\":\"error\",\"message\":\"ledger rejected the response\"}");
            break;
        }

        if ( xllmSessionGetStats(pConn->pSession, &tStats) ) {
            uint64_t uCtxUsed = pResponse->tUsage.uInputTokens +
                pResponse->tUsage.uOutputTokens;
            sprintf(sFrame,
                "{\"type\":\"usage\",\"input\":%llu,\"output\":%llu,"
                "\"ms\":%llu,\"tps\":%.2f,\"ctx_used\":%llu,\"ctx_max\":%llu,"
                "\"valid\":%d}",
                (unsigned long long)pResponse->tUsage.uInputTokens,
                (unsigned long long)pResponse->tUsage.uOutputTokens,
                (unsigned long long)pResponse->tStats.uTotalMs,
                pResponse->tStats.fOutputTokensPerSec,
                (unsigned long long)uCtxUsed,
                (unsigned long long)tStats.uContextWindowTokens,
                tStats.bFillExactValid ? 1 : 0);
            push_frame_locked(pConn, sFrame);
        }

        if ( pResponse->iToolCallCount == 0u || pConn->bStop ) {
            sprintf(sFrame, "{\"type\":\"done\",\"finish\":\"%s\"}",
                pResponse->sFinishReason ? pResponse->sFinishReason : "stop");
            push_frame_locked(pConn, sFrame);
            xllmResponseDestroy(pResponse);
            break;
        }

        /* Agent 循环：执行全部工具调用并回填为 tool 消息 */
        for ( i = 0u; i < pResponse->iToolCallCount; ++i ) {
            xllm_tool_call* pCall = &pResponse->pToolCalls[i];
            SBuf tOut = {0, 0, 0};
            SBuf tFrameA = {0, 0, 0};
            SBuf tFrameB = {0, 0, 0};

            if ( sb_cstr(&tFrameA, "{\"type\":\"tool_start\",\"name\":\"") &&
                 sb_json_escape(&tFrameA, pCall->sName, strlen(pCall->sName)) &&
                 sb_cstr(&tFrameA, "\",\"args\":\"") &&
                 sb_json_escape(&tFrameA, pCall->sArgumentsJson,
                     strlen(pCall->sArgumentsJson)) &&
                 sb_cstr(&tFrameA, "\"}") ) {
                push_frame_locked(pConn, tFrameA.pData);
            }
            free(tFrameA.pData);

            run_demo_tool(pCall->sName, pCall->sArgumentsJson, &tOut);

            (void)xllmSessionAddToolResult(pConn->pSession, uTurn,
                pCall->sId, tOut.pData ? tOut.pData : "(no output)");

            if ( sb_cstr(&tFrameB, "{\"type\":\"tool_result\",\"name\":\"") &&
                 sb_json_escape(&tFrameB, pCall->sName, strlen(pCall->sName)) &&
                 sb_cstr(&tFrameB, "\",\"result\":\"") &&
                 sb_json_escape(&tFrameB, tOut.pData, tOut.iLen) &&
                 sb_cstr(&tFrameB, "\"}") ) {
                push_frame_locked(pConn, tFrameB.pData);
            }
            free(tFrameB.pData);
            free(tOut.pData);
        }
        xllmResponseDestroy(pResponse);
    }

    /* 轮次边界治理：占用过阈值时自动压缩（Pi 滚动摘要） */
    {
        bool bCompact = 0;
        if ( xllmSessionMaybeCompact(pConn->pSession, &bCompact, &tError) &&
             bCompact && xllmSessionGetStats(pConn->pSession, &tStats) ) {
            sprintf(sFrame,
                "{\"type\":\"compacted\",\"generation\":%u,"
                "\"summary_tokens\":%llu,\"entries\":%llu}",
                (unsigned)tStats.uSummaryGeneration,
                (unsigned long long)tStats.uSummaryTokensExact,
                (unsigned long long)tStats.uEntryCount);
            push_frame_locked(pConn, sFrame);
        } else if ( !bCompact && tError.eCode != XLLM_ERROR_NONE ) {
            printf("[demo-chat] compact deferred: %s\n", tError.sMessage);
            tError.eCode = XLLM_ERROR_NONE;
        }
    }
    printf("[demo-chat] turn end\n");
}

static int32 chat_thread_proc(ptr pUserData)
{
    TurnArgs* pArgs = (TurnArgs*)pUserData;
    ChatConn* pConn = pArgs->pConn;

    run_turn(pConn, pArgs->sUserText);
    free(pArgs->sUserText);
    free(pArgs);

    xrtMutexLock(g_pLock);
    pConn->pWorker = NULL;
    pConn->bBusy = 0;
    if ( pConn->bClosed ) {
        /* 连接已关闭且回合结束：本线程是最后的持有者，负责回收 */
        ChatConn** ppLink = &g_pConns;
        while ( *ppLink && *ppLink != pConn ) { ppLink = &(*ppLink)->pNext; }
        if ( *ppLink ) { *ppLink = pConn->pNext; }
        xrtMutexUnlock(g_pLock);
        {
            ChatFrame* pFrame = pConn->pFrames;
            while ( pFrame ) {
                ChatFrame* pNext = pFrame->pNext;
                free(pFrame->sData);
                free(pFrame);
                pFrame = pNext;
            }
        }
        if ( pConn->pSession ) { xllmSessionDestroy(pConn->pSession); }
        free(pConn);
        return 0;
    }
    xrtMutexUnlock(g_pLock);
    return 0;
}

/* ------------------------------------------------------------------ */
/* 刷帧定时器（xs 引擎线程：这里才允许碰 ws 连接）                        */
/* ------------------------------------------------------------------ */

/* 在连接所属 Worker 上执行：成员校验 + 换队列 + 发送（全程持锁，
 * 与回收路径（解链后 free）互斥，无 UAF）。 */
static void flush_one(xnetworker* pWorker, ptr pUserData)
{
    ChatConn* pConn = (ChatConn*)pUserData;
    ChatFrame* pFrame;
    ChatConn* pScan;
    (void)pWorker;
    if ( !g_pLock || !pConn ) { return; }
    xrtMutexLock(g_pLock);
    if ( pConn->bClosed || !pConn->pWs ) { xrtMutexUnlock(g_pLock); return; }
    pScan = g_pConns;
    while ( pScan && pScan != pConn ) { pScan = pScan->pNext; }
    if ( !pScan ) { xrtMutexUnlock(g_pLock); return; }  /* 已被回收 */
    pFrame = pConn->pFrames;
    pConn->pFrames = NULL;
    pConn->ppFramesTail = &pConn->pFrames;
    while ( pFrame ) {
        ChatFrame* pNext = pFrame->pNext;
        xstrview tView;
        xnetresult eSend;
        tView.Data = pFrame->sData;
        tView.Size = strlen(pFrame->sData);
        eSend = xrtWsStreamText(pConn->pWs, tView);
        if ( eSend != XNET_RESULT_OK ) {
            printf("[demo-chat] send failed conn=%p result=%d\n",
                (void*)pConn->pWs, (int)eSend);
        }
        free(pFrame->sData);
        free(pFrame);
        pFrame = pNext;
    }
    xrtMutexUnlock(g_pLock);
}

static void flush_timer(void* pUserData)
{
    ChatConn* pConn;
    (void)pUserData;
    if ( !g_pLock || !g_pHost || !g_pEngine ) { return; }
    xrtMutexLock(g_pLock);
    pConn = g_pConns;
    while ( pConn ) {
        if ( !pConn->bClosed && pConn->pWs && pConn->pFrames ) {
            /* 发送必须发生在连接所属 Worker（xrt 守卫）：
             * 按亲和投递，而非在本定时器 Worker 上直接发。 */
            if ( !xrtNetEnginePost(g_pEngine, pConn->iWorkerIndex,
                    flush_one, (ptr)pConn) ) {
                printf("[demo-chat] post rejected\n");
            }
        }
        pConn = pConn->pNext;
    }
    xrtMutexUnlock(g_pLock);
    xsTimerAfter(g_pHost, 25u, flush_timer, NULL);
}

/* ------------------------------------------------------------------ */
/* 契约回调                                                             */
/* ------------------------------------------------------------------ */

static void read_llm_config(XS_HostInfo* pHost)
{
    const char* sEnv;
    xvalue* pRoot = xsConfigRoot();
    if ( pRoot ) {
        xvalue* pLlm = pRoot ? xrtValueObjectGet(pRoot, (xstrview){ "llm", 3u }) : NULL;
        if ( pLlm ) {
            xstrview tView;
            xvalue* pField;
            pField = xrtValueObjectGet(pLlm, (xstrview){ "base_url", 8u });
            if ( pField && xrtValueGetString(pField, &tView) && tView.Size &&
                 tView.Size < sizeof(g_sBaseUrl) ) {
                memcpy(g_sBaseUrl, tView.Data, tView.Size);
                g_sBaseUrl[tView.Size] = 0;
            }
            pField = xrtValueObjectGet(pLlm, (xstrview){ "model", 5u });
            if ( pField && xrtValueGetString(pField, &tView) && tView.Size &&
                 tView.Size < sizeof(g_sModel) ) {
                memcpy(g_sModel, tView.Data, tView.Size);
                g_sModel[tView.Size] = 0;
            }
            pField = xrtValueObjectGet(pLlm, (xstrview){ "api_key", 7u });
            if ( pField && xrtValueGetString(pField, &tView) && tView.Size &&
                 tView.Size < sizeof(g_sApiKey) ) {
                memcpy(g_sApiKey, tView.Data, tView.Size);
                g_sApiKey[tView.Size] = 0;
            }
            pField = xrtValueObjectGet(pLlm, (xstrview){ "context_window", 14u });
            if ( pField ) {
                int64 iWindow = 0;
                if ( xrtValueGetInt(pField, &iWindow) && iWindow > 0 ) {
                    g_uContextWindow = (uint64)iWindow;
                }
            }
            pField = xrtValueObjectGet(pLlm, (xstrview){ "session_max_out", 15u });
            if ( pField ) {
                int64 iValue = 0;
                if ( xrtValueGetInt(pField, &iValue) && iValue > 0 ) {
                    g_uSessionMaxOut = (uint32)iValue;
                }
            }
            pField = xrtValueObjectGet(pLlm, (xstrview){ "session_safety", 14u });
            if ( pField ) {
                int64 iValue = 0;
                if ( xrtValueGetInt(pField, &iValue) && iValue > 0 ) {
                    g_uSessionSafety = (uint32)iValue;
                }
            }
            pField = xrtValueObjectGet(pLlm, (xstrview){ "session_keep_recent", 19u });
            if ( pField ) {
                int64 iValue = 0;
                if ( xrtValueGetInt(pField, &iValue) && iValue > 0 ) {
                    g_uSessionKeepRecent = (uint32)iValue;
                }
            }
        }
        xrtValueRelease(pRoot);
    }
    sEnv = getenv("GLM_API_KEY");
    if ( sEnv && sEnv[0] && strlen(sEnv) < sizeof(g_sApiKey) ) {
        strcpy(g_sApiKey, sEnv);
    }
    (void)pHost;
}

void ServiceInit(XS_HostInfo* pHost)
{
    xllm_client_config tConfig;
    xllm_error tError;

    g_pHost = pHost;
    g_pEngine = pHost->Server->Engine;
    g_pLock = xrtMutexCreate();
    read_llm_config(pHost);

    xllmClientConfigInit(&tConfig);
    tConfig.sBaseUrl = g_sBaseUrl;
    tConfig.sApiKey = g_sApiKey;
    tConfig.sModel = g_sModel;
    tConfig.eProvider = XLLM_PROVIDER_OPENAI_COMPAT;
    tConfig.uMaxOutputTokens = 2048u;
    tConfig.uTimeoutMs = 120u * 1000u;
    tConfig.uMaxAttempts = 2u;
    g_pClient = xllmClientCreate(&tConfig, &tError);

    xsTimerAfter(pHost, 25u, flush_timer, NULL);
    {   /* xrt 主线 JSONL 模块 TCC 冒烟：声明走 /xs/xrt_decl.h，符号走 import_xrt.inc */
        xstrview tJsonl = { "{\"kind\":\"smoke\"}\n[1,2]\n", 23u };
        xvalue* pRecords = xrtJsonlParse(tJsonl);
        printf("[demo-chat] jsonl smoke: %s\n",
            pRecords && xrtValueCount(pRecords) == 2u ? "ok (2 records)" : "FAIL");
        xrtValueRelease(pRecords);
    }
    printf("[demo-chat] init: model=%s base=%s client=%s\n",
        g_sModel, g_sBaseUrl, g_pClient ? "ok" : "FAILED");
    if ( !g_pClient ) {
        printf("[demo-chat] error: %s\n", tError.sMessage);
    }
}

void ServiceUnit(XS_HostInfo* pHost)
{
    (void)pHost;
    if ( g_pClient ) { xllmClientDestroy(g_pClient); g_pClient = NULL; }
    if ( g_pLock ) { xrtMutexDestroy(g_pLock); g_pLock = NULL; }
    g_pHost = NULL;
}

void WsOpen(XS_HostInfo* pHost, xwsstream* pWs)
{
    ChatConn* pConn;
    (void)pHost;
    printf("[demo-chat] ws open %p\n", (void*)pWs);
    pConn = (ChatConn*)calloc(1u, sizeof(*pConn));
    if ( !pConn ) { return; }
    pConn->pWs = pWs;
    if ( g_pEngine ) {
        xnetworker* pWorker = xrtNetEngineCurrent(g_pEngine);
        if ( pWorker ) {
            pConn->iWorkerIndex = xrtNetWorkerIndex(pWorker);
        }
    }
    pConn->ppFramesTail = &pConn->pFrames;
    pConn->pSession = session_create();
    xrtMutexLock(g_pLock);
    pConn->pNext = g_pConns;
    g_pConns = pConn;
    xrtMutexUnlock(g_pLock);
    push_frame_locked(pConn, "{\"type\":\"hello\"}");
}

void WsClose(XS_HostInfo* pHost, xwsstream* pWs, uint16 iCode, xstrview tReason)
{
    ChatConn* pConn;
    (void)pHost; (void)iCode; (void)tReason;
    printf("[demo-chat] ws close %p\n", (void*)pWs);
    xrtMutexLock(g_pLock);
    pConn = g_pConns;
    while ( pConn && pConn->pWs != pWs ) { pConn = pConn->pNext; }
    if ( !pConn ) { xrtMutexUnlock(g_pLock); return; }
    pConn->bClosed = 1;
    pConn->pWs = NULL;
    pConn->bStop = 1;
    if ( !pConn->bBusy ) {
        /* 没有在飞回合：立即回收 */
        ChatConn** ppLink = &g_pConns;
        while ( *ppLink && *ppLink != pConn ) { ppLink = &(*ppLink)->pNext; }
        if ( *ppLink ) { *ppLink = pConn->pNext; }
        xrtMutexUnlock(g_pLock);
        if ( pConn->pSession ) { xllmSessionDestroy(pConn->pSession); }
        free(pConn);
        return;
    }
    xrtMutexUnlock(g_pLock);
    /* 在飞回合：工作线程结束时回收（见 chat_thread_proc） */
}

void WsText(XS_HostInfo* pHost, xwsstream* pWs, xstrview tText)
{
    ChatConn* pConn;
    (void)pHost;
    xrtMutexLock(g_pLock);
    pConn = g_pConns;
    while ( pConn && pConn->pWs != pWs ) { pConn = pConn->pNext; }
    if ( !pConn || pConn->bClosed ) { xrtMutexUnlock(g_pLock); return; }

    if ( tText.Size == 4u && memcmp(tText.Data, "stop", 4u) == 0 ) {
        pConn->bStop = 1; /* 流式回调下一增量即取消 */
        xrtMutexUnlock(g_pLock);
        return;
    }
    if ( tText.Size == 5u && memcmp(tText.Data, "clear", 5u) == 0 ) {
        if ( pConn->bBusy ) {
            frame_push(pConn, "{\"type\":\"error\",\"message\":\"busy\"}");
            xrtMutexUnlock(g_pLock);
            return;
        }
        if ( pConn->pSession ) { xllmSessionDestroy(pConn->pSession); }
        pConn->pSession = session_create();
        frame_push(pConn, "{\"type\":\"cleared\"}");
        xrtMutexUnlock(g_pLock);
        return;
    }
    if ( tText.Size > 6u && memcmp(tText.Data, "chat:", 5u) == 0 ) {
        TurnArgs* pArgs;
        if ( !g_pClient ) {
            frame_push(pConn,
                "{\"type\":\"error\",\"message\":\"xllm unavailable\"}");
            xrtMutexUnlock(g_pLock);
            return;
        }
        if ( pConn->bBusy ) {
            frame_push(pConn, "{\"type\":\"error\",\"message\":\"busy\"}");
            xrtMutexUnlock(g_pLock);
            return;
        }
        if ( !pConn->pSession ) {
            frame_push(pConn,
                "{\"type\":\"error\",\"message\":\"session unavailable\"}");
            xrtMutexUnlock(g_pLock);
            return;
        }
        pArgs = (TurnArgs*)malloc(sizeof(*pArgs));
        if ( !pArgs ) { xrtMutexUnlock(g_pLock); return; }
        pArgs->pConn = pConn;
        pArgs->sUserText = (char*)malloc(tText.Size - 5u + 1u);
        if ( !pArgs->sUserText ) { free(pArgs); xrtMutexUnlock(g_pLock); return; }
        memcpy(pArgs->sUserText, (const char*)tText.Data + 5u, tText.Size - 5u);
        pArgs->sUserText[tText.Size - 5u] = 0;
        pConn->bBusy = 1;
        pConn->bStop = 0;
        printf("[demo-chat] turn begin (%.60s)\n", pArgs->sUserText);
        pConn->pWorker = xrtThreadCreate(chat_thread_proc, pArgs, 0u);
        if ( !pConn->pWorker ) {
            pConn->bBusy = 0;
            free(pArgs->sUserText);
            free(pArgs);
            frame_push(pConn, "{\"type\":\"error\",\"message\":\"thread failed\"}");
        }
        xrtMutexUnlock(g_pLock);
        return;
    }
    xrtMutexUnlock(g_pLock);
}
#endif
