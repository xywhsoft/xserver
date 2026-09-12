/*
 * bench_mixed — xrt pattern (新版 prefix{id}suffix) vs demo trie 混合语料基准
 *
 * 语料五类（同一批字符串喂给两套实现；demo trie 仅支持 A/B）：
 *   A 整段字面量   /api/v1/resNNNNN/item/detail
 *   B 整段捕获     /api/v1/resNNNNN/item/{id}
 *   C 前缀+捕获   /api/v1/resNNNNN/item/id-{id}
 *   D 捕获+后缀   /api/v1/resNNNNN/item/{id}-log
 *   E 前缀+捕获+后缀 /api/v1/resNNNNN/item/id-{id}-log
 * 每类 500 条模式；每轮 1000 次匹配、15 轮取中位数；查询序列共享。
 */
#define XRT_MODULE_ALL
#define XRT_EXCLUDE_MEMORY_DEBUG
#define XRT_IMPLEMENTATION
#include "xrt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>

#include "route_trie.h"

typedef uint64_t uint64;
typedef int64_t int64;
typedef uint32_t uint32;

static double g_ns;
static void timer_init(void)
{
	LARGE_INTEGER f;
	QueryPerformanceFrequency(&f);
	g_ns = (double)f.QuadPart / 1e9;
}
static double now_ns(void)
{
	LARGE_INTEGER c;
	QueryPerformanceCounter(&c);
	return (double)c.QuadPart / g_ns;
}

#define ROUNDS 15
#define CALLS  1000
#define SCALE  500

static uint32 g_arrSeq[CALLS];

static double median_of(double* arr, int n)
{
	int i, j;

	for ( i = 0; i < n; i++ ) {
		for ( j = i + 1; j < n; j++ ) {
			if ( arr[j] < arr[i] ) { double t = arr[i]; arr[i] = arr[j]; arr[j] = t; }
		}
	}
	return arr[n / 2];
}

/* 语料形态表：模式尾部（前缀 /api/v1/res%05d/item/ 由生成器拼接） */
typedef struct {
	const char*	sName;
	const char*	sTail;		/* 模式段尾 */
	const char*	sQueryFmt;	/* 查询段尾 */
	int		bDemo;		/* demo trie 是否支持 */
} CorpusShape;

static const CorpusShape g_arrShape[] = {
	{ "A literal    ", "/detail",       "/detail",       1 },
	{ "B capture    ", "/{id}",         "/%d",           1 },
	{ "C prefix+cap ", "/id-{id}",      "/id-%d",        0 },
	{ "D cap+suffix ", "/{id}-log",     "/%d-log",       0 },
	{ "E pre+cap+suf", "/id-{id}-log",  "/id-%d-log",    0 },
};
#define SHAPE_COUNT (sizeof(g_arrShape) / sizeof(g_arrShape[0]))

static char** g_arrPat;
static char** g_arrQuery;

static void gen_corpus(const CorpusShape* s, int count)
{
	char buf[128];
	int i;

	g_arrPat = (char**)calloc(count, sizeof(char*));
	g_arrQuery = (char**)calloc(count, sizeof(char*));
	for ( i = 0; i < count; i++ ) {
		snprintf(buf, sizeof(buf), "/api/v1/res%05d/item%s", i, s->sTail);
		g_arrPat[i] = _strdup(buf);
		snprintf(buf, sizeof(buf), "/api/v1/res%05d/item", i);
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), s->sQueryFmt,
			1000000 + (i % 900) * 7654);
		g_arrQuery[i] = _strdup(buf);
	}
}

static void free_corpus(int count)
{
	int i;

	for ( i = 0; i < count; i++ ) { free(g_arrPat[i]); free(g_arrQuery[i]); }
	free(g_arrPat); free(g_arrQuery);
	g_arrPat = NULL; g_arrQuery = NULL;
}

static void bench_shape(const CorpusShape* s)
{
	xpatternspec* arrSpec = (xpatternspec*)calloc(SCALE, sizeof(xpatternspec));
	XS_TrieNode* pTrie = NULL;
	xpattern* pPat = NULL;
	xstrview cap[8];
	XS_TrieParam arrParam[8];
	double arrDemo[ROUNDS], arrLookup[ROUNDS], arrMatch[ROUNDS];
	volatile intptr_t sink = 0;
	int i, r, k;

	gen_corpus(s, SCALE);
	for ( i = 0; i < SCALE; i++ ) {
		arrSpec[i].Pattern = xrtStrView(g_arrPat[i]);
		arrSpec[i].Value = (ptr)(intptr_t)(i + 1);
	}
	pPat = xrtPatternCompileMany(arrSpec, SCALE);
	if ( pPat == NULL ) {
		printf("%s  compile FAILED\n", s->sName);
		goto done;
	}
	if ( s->bDemo ) {
		pTrie = XS_TrieCreate();
		for ( i = 0; i < SCALE; i++ ) {
			XS_TrieInsert(pTrie, g_arrPat[i], (void*)(intptr_t)(i + 1));
		}
	}
	for ( k = 0; k < CALLS; k++ ) {
		g_arrSeq[k] = (uint32)((k * 2654435761u) % SCALE);
	}

	/* 正确性抽检：捕获值应为 1000000+(i%900)*7654 的数字串 */
	{
		xpatternmatch m;

		if ( xrtPatternMatch(pPat, xrtStrView(g_arrQuery[0]), cap, 8, &m) == XPATTERN_MATCH ) {
			if ( m.CaptureCount > 0 ) {
				char expect[32];
				snprintf(expect, sizeof(expect), "%d", 1000000);
				if ( !(cap[0].Size == strlen(expect) && memcmp(cap[0].Data, expect, strlen(expect)) == 0) ) {
					printf("%s  capture mismatch: %.*s\n", s->sName,
						(int)cap[0].Size, cap[0].Data);
				}
			}
		} else {
			printf("%s  self-match FAILED: %s\n", s->sName, g_arrQuery[0]);
		}
	}

	if ( pTrie != NULL ) {
		for ( r = 0; r < ROUNDS; r++ ) {
			uint32 n = 0;
			double t0 = now_ns();

			for ( k = 0; k < CALLS; k++ ) {
				sink += (intptr_t)XS_TrieMatch(pTrie, g_arrQuery[g_arrSeq[k]], arrParam, 8, &n);
			}
			arrDemo[r] = (now_ns() - t0) / CALLS;
		}
	}
	for ( r = 0; r < ROUNDS; r++ ) {
		xpatternmatch m;
		double t0 = now_ns();

		for ( k = 0; k < CALLS; k++ ) {
			(void)xrtPatternLookup(pPat, xrtStrView(g_arrQuery[g_arrSeq[k]]), &m);
			sink += (intptr_t)m.Value;
		}
		arrLookup[r] = (now_ns() - t0) / CALLS;
	}
	for ( r = 0; r < ROUNDS; r++ ) {
		xpatternmatch m;
		double t0 = now_ns();

		for ( k = 0; k < CALLS; k++ ) {
			(void)xrtPatternMatch(pPat, xrtStrView(g_arrQuery[g_arrSeq[k]]), cap, 8, &m);
			sink += (intptr_t)m.Value;
		}
		arrMatch[r] = (now_ns() - t0) / CALLS;
	}

	printf("%s  %-9s %7.1f   lookup %7.1f   match %7.1f  (ns, median of %d x %d)\n",
		s->sName, pTrie != NULL ? "demo:" : "-", 
		pTrie != NULL ? median_of(arrDemo, ROUNDS) : 0.0,
		median_of(arrLookup, ROUNDS), median_of(arrMatch, ROUNDS),
		ROUNDS, CALLS);

done:
	if ( pTrie != NULL ) XS_TrieFree(pTrie);
	if ( pPat != NULL ) xrtPatternRelease(pPat);
	free(arrSpec);
	free_corpus(SCALE);
}

/* 组合混合语料：五类各 200 条进同一张表 */
static void bench_combined(void)
{
	xpatternspec* arrSpec = (xpatternspec*)calloc(SHAPE_COUNT * 200, sizeof(xpatternspec));
	char** arrQ = (char**)calloc(SHAPE_COUNT * 200, sizeof(char*));
	XS_TrieNode* pTrie = XS_TrieCreate();
	xpattern* pPat;
	xstrview cap[8];
	XS_TrieParam arrParam[8];
	double arrDemo[ROUNDS], arrLookup[ROUNDS], arrMatch[ROUNDS];
	volatile intptr_t sink = 0;
	int total = (int)(SHAPE_COUNT * 200);
	int i, si, r, k;
	uint32 seq[CALLS];

	i = 0;
	for ( si = 0; si < (int)SHAPE_COUNT; si++ ) {
		char buf[128];
		int j;

		for ( j = 0; j < 200; j++ ) {
			snprintf(buf, sizeof(buf), "/api/v1/res%05d/item%s", i, g_arrShape[si].sTail);
			arrSpec[i].Pattern = xrtStrView(_strdup(buf));
			arrSpec[i].Value = (ptr)(intptr_t)(i + 1);
			snprintf(buf, sizeof(buf), "/api/v1/res%05d/item", i);
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				g_arrShape[si].sQueryFmt, 1000000 + (i % 900) * 7654);
			arrQ[i] = _strdup(buf);
			i++;
		}
	}
	pPat = xrtPatternCompileMany(arrSpec, total);
	if ( pPat == NULL ) {
		printf("combined compile FAILED\n");
		return;
	}
	/* demo trie 只装 A/B 两类（其能力范围） */
	for ( i = 0; i < total; i++ ) {
		size_t si2 = (size_t)(i / 200);

		if ( g_arrShape[si2].bDemo ) {
			XS_TrieInsert(pTrie, (const char*)arrSpec[i].Pattern.Data, (void*)(intptr_t)(i + 1));
		}
	}
	for ( k = 0; k < CALLS; k++ ) {
		seq[k] = (uint32)((k * 2654435761u) % total);
	}

	for ( r = 0; r < ROUNDS; r++ ) {
		uint32 n = 0;
		double t0 = now_ns();

		for ( k = 0; k < CALLS; k++ ) {
			const char* q = arrQ[seq[k]];

			if ( g_arrShape[seq[k] / 200].bDemo ) {
				sink += (intptr_t)XS_TrieMatch(pTrie, q, arrParam, 8, &n);
			}
		}
		arrDemo[r] = (now_ns() - t0) / (CALLS * 2 / 5);
	}
	for ( r = 0; r < ROUNDS; r++ ) {
		xpatternmatch m;
		double t0 = now_ns();

		for ( k = 0; k < CALLS; k++ ) {
			(void)xrtPatternLookup(pPat, xrtStrView(arrQ[seq[k]]), &m);
			sink += (intptr_t)m.Value;
		}
		arrLookup[r] = (now_ns() - t0) / CALLS;
	}
	for ( r = 0; r < ROUNDS; r++ ) {
		xpatternmatch m;
		double t0 = now_ns();

		for ( k = 0; k < CALLS; k++ ) {
			(void)xrtPatternMatch(pPat, xrtStrView(arrQ[seq[k]]), cap, 8, &m);
			sink += (intptr_t)m.Value;
		}
		arrMatch[r] = (now_ns() - t0) / CALLS;
	}
	printf("combined 1000   demo(A/B) %7.1f   lookup %7.1f   match %7.1f  (ns, median)\n",
		median_of(arrDemo, ROUNDS), median_of(arrLookup, ROUNDS),
		median_of(arrMatch, ROUNDS));

	for ( i = 0; i < total; i++ ) {
		free((void*)arrSpec[i].Pattern.Data);
		free(arrQ[i]);
	}
	free(arrSpec); free(arrQ);
	XS_TrieFree(pTrie);
	xrtPatternRelease(pPat);
}

/* xrt 编译性能（单独） */
static void bench_compile(int count, const char* sShape)
{
	xpatternspec* arrSpec = (xpatternspec*)calloc(count, sizeof(xpatternspec));
	char buf[128];
	xpattern* p;
	double t0, t1;
	int i;

	for ( i = 0; i < count; i++ ) {
		if ( strcmp(sShape, "capture") == 0 ) {
			snprintf(buf, sizeof(buf), "/api/v1/res%05d/item/{id}", i);
		} else {
			/* 混合形态：五类轮转 */
			snprintf(buf, sizeof(buf), "/api/v1/res%05d/item%s", i,
				g_arrShape[i % SHAPE_COUNT].sTail);
		}
		arrSpec[i].Pattern = xrtStrView(_strdup(buf));
	}
	t0 = now_ns();
	p = xrtPatternCompileMany(arrSpec, count);
	t1 = now_ns();
	printf("compile N=%-6d %-8s %9.1f ms (%6.1f us/route) %s\n",
		count, sShape, (t1 - t0) / 1e6, (t1 - t0) / 1e3 / count,
		p != NULL ? "ok" : "FAILED");
	for ( i = 0; i < count; i++ ) free((void*)arrSpec[i].Pattern.Data);
	free(arrSpec);
	if ( p != NULL ) xrtPatternRelease(p);
}

int main(void)
{
	size_t si;

	timer_init();
	printf("== 混合语料匹配（每类 500 条，1000 次/轮 x 15 轮取中位）==\n");
	for ( si = 0; si < SHAPE_COUNT; si++ ) {
		bench_shape(&g_arrShape[si]);
	}
	printf("\n== 组合混合语料（五类各 200 条同表）==\n");
	bench_combined();

	printf("\n== xrt 编译性能 ==\n");
	bench_compile(1000, "capture");
	bench_compile(10000, "capture");
	bench_compile(25000, "capture");
	bench_compile(50000, "capture");
	bench_compile(100000, "capture");
	bench_compile(100000, "mixed");
	return 0;
}
