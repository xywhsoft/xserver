/* Build/run through test_native_boundaries.py; no running XS instance is used. */
#define XRT_MODULE_ALL
#define XRT_IMPLEMENTATION
#include "lib/xrt.h"
#include "src/core/config.h"
#include <assert.h>

static const char* XS_AppPath(void) { return "."; }

static const char* valid_config =
	"{\"services\":[{\"class\":\"http\",\"name\":\"test\",\"port\":8080,"
	"\"host_default\":{\"devfile\":\"main.c\",\"path\":\"www\"}}]}";

static void check_config(const char* text, bool expected, const char* message)
{
	XS_App app;
	bool ok = XS_ConfigLoadMemory("site/xs.json", text, strlen(text), &app);
	assert(ok == expected);
	if ( !ok ) assert(strstr(app.ParseError, message) != NULL);
	XS_ConfigFree(&app);
}

int main(void)
{
	size_t size = 17;
	xmemdebugsnapshot before, after;
	assert(XS_SizeAdd(SIZE_MAX - 1u, 1u, &size) && size == SIZE_MAX);
	assert(!XS_SizeAdd(SIZE_MAX, 1u, &size) && size == SIZE_MAX);
	assert(!XS_U64Range(UINT64_MAX - 2u, 8u, UINT64_MAX));
	assert(XS_U64Range(10, 0, 10));
	assert(!XS_TextValid("a\0b", 3, 3));
	assert(xrtMemDebugEnable(true));
	check_config(valid_config, true, "");
	check_config("{\"services\":[{\"class\":\"http\\u0000tcp\",\"name\":\"x\",\"port\":80}]}",
		false, "contains NUL");
	check_config("{\"services\":[{\"class\":\"app\",\"name\":\"x\",\"port\":0}]}", true, "");
	xrtClearError();
	xrtMemDebugSnapshot(&before);
	unsigned triggered = 0;
	for ( unsigned i = 0; i < 180; i++ ) {
		XS_App app;
		assert(xrtMemDebugFailAfter(i));
		bool ok = XS_ConfigLoadMemory("site/xs.json", valid_config, strlen(valid_config), &app);
		bool failed = xrtMemDebugFailTriggered();
		xrtMemDebugFailClear();
		if ( failed ) {
			triggered++;
			if ( ok ) { fprintf(stderr, "allocation failure %u silently accepted\n", i); return 1; }
			assert(app.ParseError[0] != '\0');
		}
		XS_ConfigFree(&app);
		xrtClearError();
		xrtMemDebugSnapshot(&after);
		if ( after.LiveCount != before.LiveCount || after.LiveBytes != before.LiveBytes ) {
			fprintf(stderr, "allocation failure %u leaked: %zu/%zu -> %zu/%zu\n", i,
				before.LiveCount, before.LiveBytes, after.LiveCount, after.LiveBytes);
			return 1;
		}
		assert(after.DoubleFreeCount == 0 && after.InvalidFreeCount == 0);
	}
	assert(triggered > 20);
	printf("PASS checked arithmetic/text, config NUL rejection and %u allocation failures without leaks\n", triggered);
	return 0;
}
