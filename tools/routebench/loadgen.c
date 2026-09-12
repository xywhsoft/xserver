/*
 * loadgen — 统一负载客户端（所有被测服务共用）
 *
 * 计时契约：服务已启动并预热后，从第一条请求发出起表，
 * 到最后一条响应的末字节收完停表；输出墙钟总时长与均值。
 * 单连接 keep-alive 顺序请求，测每请求时延（含路由），非并发容量。
 *
 * 用法：loadgen.exe <host> <port> <paths.txt> <总请求数> [预热数=100]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

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

/* memmem 简替：Windows 无 glibc memmem */
static void* find_bytes(void* hay, size_t haylen, const void* needle, size_t nlen)
{
	if ( nlen == 0 ) return hay;
	for ( size_t i = 0; i + nlen <= haylen; i++ ) {
		if ( memcmp((char*)hay + i, needle, nlen) == 0 ) return (char*)hay + i;
	}
	return NULL;
}

/* 读取并消费一个完整响应（按 Content-Length）；*pbClosed = 响应声明 Connection: close */
static int read_response(SOCKET s, char* buf, size_t cap, int* pbClosed)
{
	size_t got = 0;
	size_t head_end = 0;
	size_t body_len = 0;
	int header_done = 0;

	*pbClosed = 0;
	for ( ; ; ) {
		if ( got >= cap ) return -1;
		{
			int n = recv(s, buf + got, (int)(cap - got), 0);

			if ( n <= 0 ) return -1;
			got += (size_t)n;
		}
		if ( !header_done ) {
			char* p = (char*)find_bytes(buf, got, "\r\n\r\n", 4);

			if ( p == NULL ) continue;
			header_done = 1;
			head_end = (size_t)(p - buf) + 4;
			{
				char* cl = (char*)find_bytes(buf, head_end, "Content-Length:", 15);

				if ( cl != NULL ) {
					body_len = (size_t)strtoul(cl + 15, NULL, 10);
				}
				if ( find_bytes(buf, head_end, "Connection: close", 18) != NULL ||
				     find_bytes(buf, head_end, "connection: close", 18) != NULL ) {
					*pbClosed = 1;
				}
			}
			if ( body_len == 0 ) return 0;
		}
		if ( header_done && got - head_end >= body_len ) {
			return (int)body_len;
		}
	}
}

/* 服务端关闭后重连（PHP 内建服务器每响应 close，重连成本属其真实每请求行为） */
static SOCKET reconnect_sock(SOCKET s, SOCKADDR_IN* pAddr)
{
	closesocket(s);
	s = socket(AF_INET, SOCK_STREAM, 0);
	if ( connect(s, (SOCKADDR*)pAddr, sizeof(*pAddr)) != 0 ) {
		return INVALID_SOCKET;
	}
	return s;
}

int main(int argc, char** argv)
{
	const char* host;
	int port;
	FILE* fp;
	char** arrPath;
	size_t iPathCount = 0;
	int64_t total, warm;
	SOCKET s;
	SOCKADDR_IN addr;
	WSADATA wsa;
	char* buf;
	int64_t i;
	double t0, t1;

	if ( argc < 5 ) {
		fprintf(stderr, "usage: %s <host> <port> <paths.txt> <total> [warm=100]\n", argv[0]);
		return 1;
	}
	host = argv[1];
	port = atoi(argv[2]);
	total = _atoi64(argv[4]);
	warm = argc > 5 ? _atoi64(argv[5]) : 100;

	fp = fopen(argv[3], "r");
	if ( fp == NULL ) { perror("open paths"); return 1; }
	{
		char line[512];
		size_t cap = 1024;

		arrPath = (char**)malloc(cap * sizeof(char*));
		while ( fgets(line, sizeof(line), fp) ) {
			size_t n = strlen(line);

			while ( n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r') ) line[--n] = '\0';
			if ( n == 0 ) continue;
			if ( iPathCount == cap ) {
				cap *= 2;
				arrPath = (char**)realloc(arrPath, cap * sizeof(char*));
			}
			arrPath[iPathCount++] = _strdup(line);
		}
		fclose(fp);
	}
	if ( iPathCount == 0 ) { fprintf(stderr, "empty paths\n"); return 1; }
	timer_init();

	WSAStartup(MAKEWORD(2, 2), &wsa);
	s = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons((u_short)port);
	addr.sin_addr.s_addr = inet_addr(host);
	if ( connect(s, (SOCKADDR*)&addr, sizeof(addr)) != 0 ) {
		fprintf(stderr, "connect failed %d\n", WSAGetLastError());
		return 1;
	}
	buf = (char*)malloc(1 << 20);

	/* 预热（不计时） */
	for ( i = 0; i < warm; i++ ) {
		char req[600];
		int len = snprintf(req, sizeof(req),
			"GET %s HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\n\r\n",
			arrPath[i % iPathCount], host, port);
		int bClosed = 0;

		send(s, req, len, 0);
		if ( read_response(s, buf, 1 << 20, &bClosed) < 0 ) {
			s = reconnect_sock(s, &addr);
			if ( s == INVALID_SOCKET || send(s, req, len, 0) <= 0 ||
			     read_response(s, buf, 1 << 20, &bClosed) < 0 ) {
				fprintf(stderr, "warm read failed\n");
				return 1;
			}
		}
		if ( bClosed ) {
			s = reconnect_sock(s, &addr);
			if ( s == INVALID_SOCKET ) {
				fprintf(stderr, "warm reconnect failed\n");
				return 1;
			}
		}
	}

	/* 计时：第一条请求发出 -> 最后一条响应末字节 */
	t0 = now_ns();
	for ( i = 0; i < total; i++ ) {
		char req[600];
		int len = snprintf(req, sizeof(req),
			"GET %s HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\n\r\n",
			arrPath[i % iPathCount], host, port);
		int bClosed = 0;
		int bRetry = 0;

		if ( send(s, req, len, 0) <= 0 ) {
			s = reconnect_sock(s, &addr);
			if ( s == INVALID_SOCKET || send(s, req, len, 0) <= 0 ) {
				fprintf(stderr, "reconnect failed at %lld\n", (long long)i);
				return 1;
			}
		}
		if ( read_response(s, buf, 1 << 20, &bClosed) < 0 ) {
			/* 服务端未声明就关闭连接（php -S 行为）：重连重发本条 */
			if ( bRetry ) {
				fprintf(stderr, "read failed at %lld\n", (long long)i);
				return 1;
			}
			bRetry = 1;
			s = reconnect_sock(s, &addr);
			if ( s == INVALID_SOCKET || send(s, req, len, 0) <= 0 ||
			     read_response(s, buf, 1 << 20, &bClosed) < 0 ) {
				fprintf(stderr, "read failed at %lld\n", (long long)i);
				return 1;
			}
		}
		if ( bClosed ) {
			s = reconnect_sock(s, &addr);
			if ( s == INVALID_SOCKET ) {
				fprintf(stderr, "reconnect failed at %lld\n", (long long)i);
				return 1;
			}
		}
	}
	t1 = now_ns();

	printf("%.1f %.1f\n",
		(t1 - t0) / 1e3 / (double)total,          /* 微秒/请求 */
		(double)total / ((t1 - t0) / 1e9));       /* 请求/秒 */
	closesocket(s);
	WSACleanup();
	return 0;
}
