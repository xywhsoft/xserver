/*
 * bench_route — xs 范例路由系统性能模型实测
 *
 * 三种匹配策略：
 *   1. 静态路由表：xrtMap（字符串键 → 处理函数槽），精确匹配
 *   2. 动态路由：注册回调链逐条前缀+参数段检查（线性递增）
 *   3. 动态路由批量预编译：xrtRegexSetCompile（模式集合一次预编译，
 *      单趟输入识别命中项）
 * 计时：QueryPerformanceCounter，含预热，随机交织命中/未命中防缓存友好。
 */
#define XRT_MODULE_ALL
#define XRT_IMPLEMENTATION
#include "../lib/xrt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "route_trie.h"

#define VOLATILE_SINK volatile int g_sink

/* ---------------- 计时 ---------------- */

static double g_freqPerNs;

static void timer_init(void)
{
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    g_freqPerNs = (double)f.QuadPart / 1e9;
}

static double now_ns(void)
{
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / g_freqPerNs;
}

/* xorshift64 —— 确定性随机 */
static uint64 rng(uint64* s)
{
    uint64 x = *s;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    *s = x;
    return x;
}

/* ---------------- 现实感路由名生成 ---------------- */

static const char* arrRes[] = {
    "user", "role", "group", "menu", "option", "form", "sched", "logs",
    "auth", "member", "notify", "mail", "file", "plugin", "trace",
    "brand", "item", "order", "photo", "tag", "post", "comment",
    "cache", "hook", "task", "queue", "chart", "report"
};
#define RES_COUNT (sizeof(arrRes) / sizeof(arrRes[0]))

static const char* arrForms[] = {
    "/admin/%s", "/admin/view/%s", "/admin/view/%s/add", "/admin/view/%s/edit",
    "/admin/auth/%s", "/admin/option/%s", "/api/v1/%s", "/api/v1/%s/list",
    "/api/v1/%s/detail", "/api/v1/%s/delete"
};
#define FORM_COUNT (sizeof(arrForms) / sizeof(arrForms[0]))

static int gen_routes(int count, char*** parr)
{
    char** arr = (char**)calloc(count, sizeof(char*));
    int n = 0;
    char buf[128];
    while ( n < count ) {
        for ( size_t f = 0; f < FORM_COUNT && n < count; f++ ) {
            for ( size_t r = 0; r < RES_COUNT && n < count; r++ ) {
                snprintf(buf, sizeof(buf), arrForms[f], arrRes[r]);
                arr[n++] = xrtStrDup(buf);
            }
        }
        /* 数量不足时追加编号变体 */
        if ( n < count ) {
            snprintf(buf, sizeof(buf), "/api/v1/extra%d/action", n);
            arr[n++] = xrtStrDup(buf);
        }
    }
    *parr = arr;
    return n;
}

/* ---------------- 1. 静态路由表（xrtMap） ---------------- */

static void bench_static(int count)
{
    char** arr = NULL;
    gen_routes(count, &arr);
    xmap* map = xrtMapCreate(sizeof(int));
    for ( int i = 0; i < count; i++ ) {
        int v = i + 1;
        xrtMapSet(map, (xbytesview){ (cbytes)arr[i], strlen(arr[i]) }, &v);
    }

    /* 查询串：90% 命中（随机表项）+ 10% 未命中 */
    int qn = count + count / 8 + 16;
    const char** queries = (const char**)calloc(qn, sizeof(char*));
    for ( int i = 0; i < count; i++ ) queries[i] = arr[i];
    for ( int i = count; i < qn; i++ ) queries[i] = "/no/such/route/at/all";
    for ( int i = qn - 1; i > 0; i-- ) { /* 洗牌 */
        int j = (int)(rng(&(uint64){0x9E3779B97F4A7C15ULL}) % (i + 1));
        const char* t = queries[i]; queries[i] = queries[j]; queries[j] = t;
    }

    uint64 seed = 0x12345678ABCDEFULL;
    volatile int sink = 0;
    int64 iters = 2000000;
    /* 预热 */
    for ( int i = 0; i < 50000; i++ ) {
        int* v = (int*)xrtMapGet(map, (xbytesview){ (cbytes)queries[i % qn], strlen(queries[i % qn]) });
        sink += v ? *v : 0;
    }
    double t0 = now_ns();
    for ( int64 k = 0; k < iters; k++ ) {
        const char* q = queries[rng(&seed) % qn];
        int* v = (int*)xrtMapGet(map, (xbytesview){ (cbytes)q, strlen(q) });
        sink += v ? *v : 1;
    }
    double t1 = now_ns();
    printf("static  xrtMap        N=%-4d  %8.1f ns/次   (含10%%未命中, 随机交织)\n",
        count, (t1 - t0) / (double)iters);

    for ( int i = 0; i < count; i++ ) xrtFree(arr[i]);
    xrtFree(arr);
    xrtFree(queries);
    xrtMapDestroy(map);
}

/* ---------------- 2. 动态路由：顺序回调链 ---------------- */

typedef struct {
    const char* prefix;
    size_t len;
} DynRoute;

/* 现实感的动态检查：前缀 memcmp + 参数段非空校验 */
static inline int dyn_match(const DynRoute* r, const char* s, size_t n)
{
    if ( n <= r->len ) return 0;
    if ( memcmp(s, r->prefix, r->len) != 0 ) return 0;
    const char* p = s + r->len;
    const char* e = (const char*)memchr(p, '/', n - r->len);
    return (e ? e : s + n) > p ? 1 : 0;
}

static void bench_dynamic(int count, int worst)
{
    DynRoute* rs = (DynRoute*)calloc(count, sizeof(DynRoute));
    char buf[128];
    for ( int i = 0; i < count; i++ ) {
        snprintf(buf, sizeof(buf), "/api/v1/res%d/item/", i);
        rs[i].prefix = xrtStrDup(buf);
        rs[i].len = strlen(buf);
    }
    /* 查询路径：命中随机条 / 最末条 */
    char** qs = (char**)calloc(count, sizeof(char*));
    for ( int i = 0; i < count; i++ ) {
        snprintf(buf, sizeof(buf), "/api/v1/res%d/item/12345", i);
        qs[i] = xrtStrDup(buf);
    }

    uint64 seed = 0xDEADBEEFCAFEULL;
    volatile int sink = 0;
    int64 iters = count <= 20 ? 2000000 : 300000;
    for ( int64 k = 0; k < 20000; k++ ) {
        const char* q = qs[(worst ? count - 1 : (int)(rng(&seed) % count))];
        for ( int i = 0; i < count; i++ )
            if ( dyn_match(&rs[i], q, strlen(q)) ) { sink += i; break; }
    }
    double t0 = now_ns();
    for ( int64 k = 0; k < iters; k++ ) {
        const char* q = qs[worst ? count - 1 : (int)(rng(&seed) % count)];
        size_t qn = strlen(q);
        for ( int i = 0; i < count; i++ )
            if ( dyn_match(&rs[i], q, qn) ) { sink += i; break; }
    }
    double t1 = now_ns();
    printf("dynamic 顺序链        N=%-4d  %8.1f ns/次   (%s)\n",
        count, (t1 - t0) / (double)iters, worst ? "最坏:末条命中" : "平均:均匀命中");

    for ( int i = 0; i < count; i++ ) { xrtFree((void*)rs[i].prefix); xrtFree(qs[i]); }
    xrtFree(rs); xrtFree(qs);
}

/* ---------------- 3. 动态路由：regex set 批量预编译 ---------------- */

static void bench_regexset(int count)
{
    xstrview* pats = (xstrview*)calloc(count, sizeof(xstrview));
    char buf[128];
    for ( int i = 0; i < count; i++ ) {
        snprintf(buf, sizeof(buf), "^/api/v1/res%d/item/[^/]+$", i);
        pats[i] = xrtStrView(xrtStrDup(buf));
    }
    xregexset* set = xrtRegexSetCompile(pats, count);
    if ( set == NULL ) {
        printf("regexset compile failed N=%d\n", count);
        return;
    }
    xregexsetmatcher* m = xrtRegexSetMatcherCreate(set);

    char** qs = (char**)calloc(count, sizeof(char*));
    for ( int i = 0; i < count; i++ ) {
        snprintf(buf, sizeof(buf), "/api/v1/res%d/item/12345", i);
        qs[i] = xrtStrDup(buf);
    }

    uint64 seed = 0xFEEDFACE1234ULL;
    volatile size_t sink = 0;
    int64 iters = count <= 20 ? 500000 : 100000;
    for ( int64 k = 0; k < 10000; k++ ) {
        const char* q = qs[(int)(rng(&seed) % count)];
        (void)xrtRegexSetMatcherMatch(m, xrtStrView(q), 0);
        sink += xrtRegexSetMatcherCount(m);
    }
    double t0 = now_ns();
    for ( int64 k = 0; k < iters; k++ ) {
        const char* q = qs[(int)(rng(&seed) % count)];
        (void)xrtRegexSetMatcherMatch(m, xrtStrView(q), 0);
        sink += xrtRegexSetMatcherCount(m);
    }
    double t1 = now_ns();
    printf("dynamic RegexSet      N=%-4d  %8.1f ns/次   (批量预编译, 均匀命中)\n",
        count, (t1 - t0) / (double)iters);

    for ( int i = 0; i < count; i++ ) { xrtFree((void*)pats[i].Data); xrtFree(qs[i]); }
    xrtFree(pats); xrtFree(qs);
    xrtRegexSetMatcherFree(m);
    xrtRegexSetRelease(set);
}

static void bench_single_regex(void)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "^/api/v1/res%d/item/[^/]+$", 0);
    xregex* re = xrtRegexCompile(xrtStrView(buf));
    xregexmatcher* m = xrtRegexMatcherCreate(re);
    const char* q = "/api/v1/res0/item/12345";
    volatile int sink = 0;
    int64 iters = 1000000;
    for ( int64 k = 0; k < 20000; k++ ) {
        (void)xrtRegexMatcherFull(m, xrtStrView(q));
        sink++;
    }
    double t0 = now_ns();
    for ( int64 k = 0; k < iters; k++ ) {
        (void)xrtRegexMatcherFull(m, xrtStrView(q));
        sink++;
    }
    double t1 = now_ns();
    printf("regex  single compiled N=1   %8.1f ns/op\n", (t1 - t0) / (double)iters);
    xrtRegexMatcherFree(m);
    xrtRegexRelease(re);
}

static void bench_trie(int count)
{
    XS_TrieNode* t = XS_TrieCreate();
    char buf[128];
    for ( int i = 0; i < count; i++ ) {
        snprintf(buf, sizeof(buf), "/api/v1/res%d/item/{id}", i);
        if ( XS_TrieInsert(t, buf, (void*)(intptr_t)(i + 1)) != 0 ) { return; }
    }
    char** qs = (char**)calloc(count, sizeof(char*));
    for ( int i = 0; i < count; i++ ) {
        snprintf(buf, sizeof(buf), "/api/v1/res%d/item/12345", i);
        qs[i] = xrtStrDup(buf);
    }
    uint64 seed = 0xABCDEF01ULL;
    volatile intptr_t sink = 0;
    XS_TrieParam params[8];
    uint32 np = 0;
    int64 iters = 2000000;
    for ( int64 k = 0; k < 50000; k++ ) {
        sink += (intptr_t)XS_TrieMatch(t, qs[(int)(rng(&seed) % count)], params, 8, &np);
    }
    double t0 = now_ns();
    for ( int64 k = 0; k < iters; k++ ) {
        sink += (intptr_t)XS_TrieMatch(t, qs[(int)(rng(&seed) % count)], params, 8, &np);
    }
    double t1 = now_ns();
    printf("dynamic Trie           N=%-4d  %8.1f ns/op   (flat, params=%u)\n",
        count, (t1 - t0) / (double)iters, np);
    for ( int i = 0; i < count; i++ ) xrtFree(qs[i]);
    xrtFree(qs);
    XS_TrieFree(t);
}

/* 混合负载：147 静态 + 100 动态，静态:动态:未命中 = 45:45:10 */
static void bench_hybrid(void)
{
    char** stat = NULL;
    int nStat = gen_routes(147, &stat);
    xmap* map = xrtMapCreate(sizeof(int));
    for ( int i = 0; i < nStat; i++ ) {
        int v = i + 1;
        xrtMapSet(map, (xbytesview){ (cbytes)stat[i], strlen(stat[i]) }, &v);
    }
    XS_TrieNode* trieDyn = XS_TrieCreate();
    XS_TrieNode* trieAll = XS_TrieCreate();
    char buf[128];
    char** dyn = (char**)calloc(100, sizeof(char*));
    for ( int i = 0; i < 100; i++ ) {
        snprintf(buf, sizeof(buf), "/api/v2/res%d/item/{id}", i);
        XS_TrieInsert(trieDyn, buf, (void*)(intptr_t)(i + 1));
        XS_TrieInsert(trieAll, buf, (void*)(intptr_t)(i + 1));
    }
    for ( int i = 0; i < nStat; i++ ) XS_TrieInsert(trieAll, stat[i], (void*)(intptr_t)(i + 1));
    for ( int i = 0; i < 100; i++ ) {
        snprintf(buf, sizeof(buf), "/api/v2/res%d/item/777", i);
        dyn[i] = xrtStrDup(buf);
    }
    /* 查询流：静态命中/动态命中/未命中 交错 */
    int qn = nStat + 100 + 26;
    const char** qs = (const char**)calloc(qn, sizeof(char*));
    for ( int i = 0; i < nStat; i++ ) qs[i] = stat[i];
    for ( int i = 0; i < 100; i++ ) qs[nStat + i] = dyn[i];
    for ( int i = 0; i < 26; i++ ) qs[nStat + 100 + i] = "/no/such/path/here";
    for ( int i = qn - 1; i > 0; i-- ) {
        int j = (int)(rng(&(uint64){0xC0FFEEULL}) % (i + 1));
        const char* t = qs[i]; qs[i] = qs[j]; qs[j] = t;
    }

    uint64 seed = 0x51D5ULL;
    volatile intptr_t sink = 0;
    XS_TrieParam params[8];
    uint32 np = 0;
    int64 iters = 1000000;

    /* 预热 */
    for ( int64 k = 0; k < 30000; k++ ) {
        const char* q = qs[rng(&seed) % qn];
        if ( xrtMapGet(map, (xbytesview){ (cbytes)q, strlen(q) }) ) sink++;
        else sink += (intptr_t)XS_TrieMatch(trieDyn, q, params, 8, &np);
    }
    /* 拆分：map -> miss 则 trie */
    double t0 = now_ns();
    for ( int64 k = 0; k < iters; k++ ) {
        const char* q = qs[rng(&seed) % qn];
        if ( xrtMapGet(map, (xbytesview){ (cbytes)q, strlen(q) }) ) sink++;
        else sink += (intptr_t)XS_TrieMatch(trieDyn, q, params, 8, &np);
    }
    double t1 = now_ns();
    printf("hybrid split map+trie  %8.1f ns/op\n", (t1 - t0) / (double)iters);

    /* 统一：单 trie */
    double t2 = now_ns();
    for ( int64 k = 0; k < iters; k++ ) {
        const char* q = qs[rng(&seed) % qn];
        sink += (intptr_t)XS_TrieMatch(trieAll, q, params, 8, &np);
    }
    double t3 = now_ns();
    printf("hybrid unified trie    %8.1f ns/op\n", (t3 - t2) / (double)iters);

    for ( int i = 0; i < 100; i++ ) xrtFree(dyn[i]);
    xrtFree(dyn); xrtFree(qs);
    for ( int i = 0; i < nStat; i++ ) xrtFree(stat[i]);
    xrtFree(stat);
    xrtMapDestroy(map);
    XS_TrieFree(trieDyn);
    XS_TrieFree(trieAll);
}

int main(void)
{
    timer_init();
    printf("==== 静态路由表（xrtMap 精确匹配）====\n");
    bench_static(16);
    bench_static(64);
    bench_static(147);   /* xadmin 真实规模 */
    bench_static(512);
    bench_static(2048);

    printf("\n==== 动态路由（顺序回调链）====\n");
    bench_dynamic(1, 0);
    bench_dynamic(20, 0);
    bench_dynamic(100, 0);
    bench_dynamic(500, 0);
    bench_dynamic(500, 1);

    printf("\n==== 动态路由（xrtRegexSet 批量预编译）====\n");
    bench_hybrid();
    bench_trie(20);
    bench_trie(100);
    bench_trie(500);
    bench_regexset(1);
    bench_regexset(20);
    bench_regexset(50);
    bench_regexset(100);
    bench_regexset(200);
    bench_regexset(500);
    bench_single_regex();
    return 0;
}
