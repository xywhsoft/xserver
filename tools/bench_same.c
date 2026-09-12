/*
 * bench_same — 同条件复测：demo trie vs xrtPattern Lookup/Match
 *
 * 协议（按开发者要求）：
 *   - 相同模式结构：/api/v1/resourceXXXXXXXX/item/{id}（XXXXXXXX = 8 位编号）
 *   - 同一预生成随机查询序列（三组测试完全同序）
 *   - GCC -O2 单翻译单元（demo static 可内联；XRT 实现体同 TU 同优化级）
 *   - 每组 21 轮 x 100 万次调用，取轮均值的中位数
 */
#define XRT_MODULE_ALL
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
typedef uint32_t uint32_t_;
typedef int8_t int8_;

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

#define ROUNDS 21
#define CALLS  1000000

/* 预生成共享查询序列：三组测试按同一顺序访问同一批查询 */
static uint32* g_arrSeq;

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

static void bench_scale(int count)
{
	char** arrPat = (char**)calloc(count, sizeof(char*));
	char** arrQuery = (char**)calloc(count, sizeof(char*));
	xpatternspec* arrSpec = (xpatternspec*)calloc(count, sizeof(xpatternspec));
	XS_TrieNode* pTrie = XS_TrieCreate();
	xpattern* pPattern;
	xstrview cap[8];
	XS_TrieParam arrParam[8];
	double arrDemo[ROUNDS], arrLookup[ROUNDS], arrMatch[ROUNDS];
	volatile intptr_t sink = 0;
	int i, r, k;
	int per = count < 1000 ? count : 1000;

	/* 统一结构语料：/api/v1/resourceXXXXXXXX/item/{id} */
	for ( i = 0; i < count; i++ ) {
		char buf[96];

		snprintf(buf, sizeof(buf), "/api/v1/resource%08d/item/{id}", i);
		arrPat[i] = _strdup(buf);
		XS_TrieInsert(pTrie, buf, (void*)(intptr_t)(i + 1));
		arrSpec[i].Pattern = xrtStrView(arrPat[i]);
		arrSpec[i].Value = (ptr)(intptr_t)(i + 1);
		snprintf(buf, sizeof(buf), "/api/v1/resource%08d/item/7654321", i);
		arrQuery[i] = _strdup(buf);
	}
	pPattern = xrtPatternCompileMany(arrSpec, count);
	if ( pPattern == NULL ) {
		printf("N=%d compile failed\n", count);
		return;
	}
	/* 共享查询序列 */
	for ( k = 0; k < CALLS; k++ ) {
		g_arrSeq[k] = (uint32)((k * 2654435761u) % per);
	}

	/* demo trie */
	for ( r = 0; r < ROUNDS; r++ ) {
		uint32 n = 0;
		double t0 = now_ns();

		for ( k = 0; k < CALLS; k++ ) {
			const char* q = arrQuery[g_arrSeq[k]];
			sink += (intptr_t)XS_TrieMatch(pTrie, q, arrParam, 8, &n);
		}
		arrDemo[r] = (now_ns() - t0) / CALLS;
	}
	/* XRT Lookup（无捕获） */
	for ( r = 0; r < ROUNDS; r++ ) {
		xpatternmatch m;
		double t0 = now_ns();

		for ( k = 0; k < CALLS; k++ ) {
			const char* q = arrQuery[g_arrSeq[k]];
			(void)xrtPatternLookup(pPattern, xrtStrView(q), &m);
			sink += (intptr_t)m.Value;
		}
		arrLookup[r] = (now_ns() - t0) / CALLS;
	}
	/* XRT Match（含捕获） */
	for ( r = 0; r < ROUNDS; r++ ) {
		xpatternmatch m;
		double t0 = now_ns();

		for ( k = 0; k < CALLS; k++ ) {
			const char* q = arrQuery[g_arrSeq[k]];
			(void)xrtPatternMatch(pPattern, xrtStrView(q), cap, 8, &m);
			sink += (intptr_t)m.Value;
		}
		arrMatch[r] = (now_ns() - t0) / CALLS;
	}

	printf("N=%-6d  demo %7.1f   lookup %7.1f   match %7.1f  (ns, median)\n",
		count, median_of(arrDemo, ROUNDS), median_of(arrLookup, ROUNDS),
		median_of(arrMatch, ROUNDS));

	for ( i = 0; i < count; i++ ) { free(arrPat[i]); free(arrQuery[i]); }
	free(arrPat); free(arrQuery); free(arrSpec);
	XS_TrieFree(pTrie);
	xrtPatternRelease(pPattern);
}

/* 构建时间复测：区分语料形态 */
static void bench_build_shape(int count, int shape)
{
	xpatternspec* arrSpec = (xpatternspec*)calloc(count, sizeof(xpatternspec));
	char** arrPat = (char**)calloc(count, sizeof(char*));
	static const char* arrRes[] = {
		"user", "role", "group", "menu", "option", "form", "sched", "logs",
		"auth", "member", "notify", "mail", "file", "plugin", "trace",
		"brand", "item", "order", "photo", "tag", "post", "comment",
		"cache", "hook", "task", "queue", "chart", "report"
	};
	static const char* arrForms[] = {
		"/api/v1/%s/{id}", "/api/v1/%s/{id}/log/{page}", "/api/v2/%s/{id}",
		"/admin/%s/{sid}/detail", "/app/%s/{a}/{b}"
	};
	xpattern* p;
	double t0, t1;
	int i;

	for ( i = 0; i < count; i++ ) {
		char buf[128];

		if ( shape == 0 ) {
			/* 统一结构 */
			snprintf(buf, sizeof(buf), "/api/v1/resource%08d/item/{id}", i);
		} else {
			/* 异构多捕获（此前评估的语料） */
			if ( i < 140 ) {
				snprintf(buf, sizeof(buf), arrForms[i % 5], arrRes[i % 28]);
			} else {
				char rx[64];
				snprintf(rx, sizeof(rx), "%s%d", arrRes[i % 28], i);
				snprintf(buf, sizeof(buf), arrForms[i % 5], rx);
			}
		}
		arrPat[i] = _strdup(buf);
		arrSpec[i].Pattern = xrtStrView(arrPat[i]);
	}
	t0 = now_ns();
	p = xrtPatternCompileMany(arrSpec, count);
	t1 = now_ns();
	printf("build N=%-6d  %-10s  %9.1f ms  (%.1f us/route)  %s\n",
		count, shape == 0 ? "uniform" : "hetero",
		(t1 - t0) / 1e6, (t1 - t0) / 1e3 / count,
		p != NULL ? "ok" : "FAILED");
	for ( i = 0; i < count; i++ ) free(arrPat[i]);
	free(arrPat); free(arrSpec);
	if ( p != NULL ) xrtPatternRelease(p);
}

int main(void)
{
	timer_init();
	g_arrSeq = (uint32*)calloc(CALLS, sizeof(uint32));

	bench_scale(500);
	bench_scale(10000);
	bench_scale(100000);

	printf("\n");
	bench_build_shape(100000, 0);
	bench_build_shape(100000, 1);
	bench_build_shape(1000000, 0);
	return 0;
}
