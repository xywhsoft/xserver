#define XRT_MODULE_ALL
#define XRT_EXCLUDE_MEMORY_DEBUG
#define XRT_IMPLEMENTATION
#include "xrt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>

static double g_ns;
static void ti(void){ LARGE_INTEGER f; QueryPerformanceFrequency(&f); g_ns=(double)f.QuadPart/1e9; }
static double ns(void){ LARGE_INTEGER c; QueryPerformanceCounter(&c); return c.QuadPart/g_ns; }

static void build_n(int count, int padded)
{
	xpatternspec* sp=(xpatternspec*)calloc(count,sizeof(xpatternspec));
	char** pat=(char**)calloc(count,sizeof(char*));
	char buf[128];
	xpattern* p;
	double t0,t1;
	int i;
	for(i=0;i<count;i++){
		if(padded) snprintf(buf,sizeof(buf),"/api/v1/resource%08d/item/{id}",i);
		else snprintf(buf,sizeof(buf),"/api/v1/resource%d/item/{id}",i);
		pat[i]=_strdup(buf); sp[i].Pattern=xrtStrView(pat[i]);
	}
	t0=ns(); p=xrtPatternCompileMany(sp,count); t1=ns();
	printf("N=%-7d %-9s build %9.1f ms (%6.1f us/route) %s\n",count,padded?"padded":"plain",(t1-t0)/1e6,(t1-t0)/1e3/count,p?"ok":"FAILED");
	for(i=0;i<count;i++) free(pat[i]);
	free(pat); free(sp);
	if(p) xrtPatternRelease(p);
}

int main(void)
{
	ti();
	build_n(10000,1);
	build_n(25000,1);
	build_n(50000,1);
	build_n(100000,1);
	build_n(100000,0);
	return 0;
}
