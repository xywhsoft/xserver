/*
 * bench_tcc — 同一路由代码在 TCC 与 GCC 下的性能对比
 * 编译：gcc -O2 / tcc（生产环境 xs 脚本的真实编译条件）
 * 内容：trie 匹配（demo 脚本侧热路径）、顺序链、微运算基线
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef uint64_t uint64;
typedef int64_t int64;
typedef uint32_t uint32;

#include <windows.h>

#include "route_trie.h"

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

static uint64 rng(uint64* s)
{
	uint64 x = *s;

	x ^= x << 13; x ^= x >> 7; x ^= x << 17;
	*s = x;
	return x;
}

/* ---- 顺序链（TCC/GCC 各编译一份的脚本侧代码） ---- */

typedef struct {
	const char* prefix;
	size_t len;
} DynRoute;

static int dyn_match(const DynRoute* r, const char* s, size_t n)
{
	const char* p;
	const char* e;

	if ( n <= r->len ) return 0;
	if ( memcmp(s, r->prefix, r->len) != 0 ) return 0;
	p = s + r->len;
	e = (const char*)memchr(p, '/', n - r->len);
	return (e ? e : s + n) > p ? 1 : 0;
}

static int bench_trie(XS_TrieNode* t, char** qs, int count, int64 iters)
{
	uint64 seed = 0xABCDEF01ULL;
	volatile intptr_t sink = 0;
	XS_TrieParam params[8];
	uint32 np = 0;
	int64 k;

	for ( k = 0; k < 50000; k++ ) {
		sink += (intptr_t)XS_TrieMatch(t, qs[(int)(rng(&seed) % count)], params, 8, &np);
	}
	{
		double t0 = now_ns();

		for ( k = 0; k < iters; k++ ) {
			sink += (intptr_t)XS_TrieMatch(t, qs[(int)(rng(&seed) % count)], params, 8, &np);
		}
		return (int)(now_ns() - t0);
	}
}

static int bench_chain(DynRoute* rs, char** qs, int count, int64 iters)
{
	uint64 seed = 0xDEADBEEFCAFEULL;
	volatile int sink = 0;
	int64 k;

	for ( k = 0; k < 20000; k++ ) {
		const char* q = qs[(int)(rng(&seed) % count)];
		int i;

		for ( i = 0; i < count; i++ )
			if ( dyn_match(&rs[i], q, strlen(q)) ) { sink += i; break; }
	}
	{
		double t0 = now_ns();

		for ( k = 0; k < iters; k++ ) {
			const char* q = qs[(int)(rng(&seed) % count)];
			size_t qn = strlen(q);
			int i;

			for ( i = 0; i < count; i++ )
				if ( dyn_match(&rs[i], q, qn) ) { sink += i; break; }
		}
		return (int)(now_ns() - t0);
	}
}

static int bench_micro(int64 iters)
{
	volatile uint64 sink = 0;
	uint64 acc = 0;
	int64 k;

	{
		double t0 = now_ns();

		for ( k = 0; k < iters; k++ ) {
			acc = acc * 1103515245u + 12345u + (uint64)k;
			acc ^= acc >> 13;
			sink = acc;
		}
		return (int)(now_ns() - t0);
	}
}

int main(void)
{
	XS_TrieNode* t = XS_TrieCreate();
	DynRoute rs[500];
	char buf[128];
	char** qs = (char**)calloc(500, sizeof(char*));
	int i;
	int64 itersT = 2000000;
	int64 itersC = 300000;
	int64 itersM = 10000000;

	timer_init();
	for ( i = 0; i < 500; i++ ) {
		snprintf(buf, sizeof(buf), "/api/v1/res%d/item/{id}", i);
		XS_TrieInsert(t, buf, (void*)(intptr_t)(i + 1));
		snprintf(buf, sizeof(buf), "/api/v1/res%d/item/", i);
		rs[i].prefix = _strdup(buf);
		rs[i].len = strlen(buf);
		snprintf(buf, sizeof(buf), "/api/v1/res%d/item/12345", i);
		qs[i] = _strdup(buf);
	}

	printf("trie   match  N=500 : %8.1f ns/op\n", bench_trie(t, qs, 500, itersT) / (double)itersT);
	printf("chain  match  N=500 : %8.1f ns/op\n", bench_chain(rs, qs, 500, itersC) / (double)itersC);
	printf("micro  intloop      : %8.2f ns/op\n", bench_micro(itersM) / (double)itersM);
	return 0;
}
