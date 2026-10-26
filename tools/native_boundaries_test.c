/* Build/run through test_native_boundaries.py; no running XS instance is used. */
#define XRT_MODULE_ALL
#define XRT_IMPLEMENTATION
#include "lib/xrt.h"
#include "src/core/config.h"
#include "src/runtime/generation.h"
#include "src/runtime/byte_budget.h"
#define XS_VFS_IMPLEMENTATION
#include "src/core/xs_vfs.h"
#define XS_PACK_IMPLEMENTATION
#include "src/core/xs_pack.h"
#include <assert.h>

const char* xsAppPath(void) {
#ifdef _WIN32
	return "D:/xs-test";
#else
	return "/xs-test";
#endif
}
bool g_XS_VfsDisabled = false;

static const char* valid_config =
	"{\"services\":[{\"class\":\"http\",\"name\":\"test\",\"port\":8080,"
	"\"host_default\":{\"devfile\":\"main.c\",\"path\":\"www\","
	"\"dev_inc\":\"inc;headers;;\",\"dev_lib\":\"lib;more\"}}]}";

static xatomic32 quiesced, finalized, ready, run_workers, admissions;
static XS_ByteBudget shared_budget;

static void test_quiesce(XS_ServerGeneration* generation)
{
	assert(xrtAtomic32Load(&generation->tActivities, XMEMORY_ACQUIRE) == 0);
	xrtAtomic32FetchAdd(&quiesced, 1, XMEMORY_ACQ_REL);
}

static void test_finalize(XS_ServerGeneration* generation)
{
	assert(XS_GenerationConnectionCount(generation) == 0);
	assert(xrtAtomic32Load(&generation->tActivities, XMEMORY_ACQUIRE) == 0);
	xrtMutexDestroy(generation->pTimerLock);
	xrtCondDestroy(generation->pPublishCond);
	xrtMutexDestroy(generation->pPublishLock);
	xrtFree(generation);
	xrtAtomic32FetchAdd(&finalized, 1, XMEMORY_ACQ_REL);
}

static int32 race_acquire(ptr data)
{
	XS_ServerGeneration* generation = data;
	xrtAtomic32FetchAdd(&ready, 1, XMEMORY_ACQ_REL);
	while ( !xrtAtomic32Load(&run_workers, XMEMORY_ACQUIRE) ) xrtSleep(0);
	for ( unsigned i = 0; i < 2000; i++ ) {
		if ( XS_GenerationConnectionAcquire(generation) ) {
			xrtAtomic32FetchAdd(&admissions, 1, XMEMORY_ACQ_REL);
			xrtSleep(0);
			XS_GenerationConnectionRelease(generation);
		}
		if ( XS_ByteBudgetReserve(&shared_budget, 3) ) {
			assert(xrtAtomic64Load(&shared_budget.Used, XMEMORY_ACQUIRE) <= shared_budget.Limit);
			assert(XS_ByteBudgetRelease(&shared_budget, 3));
		}
	}
	return 0;
}

static void check_lifetimes(void)
{
	assert(XS_GenerationReaperInit());
	XS_ByteBudgetInit(&shared_budget, 7);
	assert(!XS_ByteBudgetReserve(&shared_budget, UINT64_MAX));
	assert(!XS_ByteBudgetRelease(&shared_budget, 1));
	for ( unsigned round = 0; round < 40; round++ ) {
		XS_ServerInfo server = {0};
		XS_ServerGeneration* generation = XS_GenerationCreate(&server);
		xthread* threads[4];
		assert(generation != NULL);
		generation->iConnectionLimit = 2;
		assert(!XS_GenerationConnectionAcquire(generation));
		assert(XS_GenerationPublish(generation));
		assert(XS_GenerationConnectionAcquire(generation));
		assert(XS_GenerationConnectionAcquire(generation));
		assert(!XS_GenerationConnectionAcquire(generation));
		XS_GenerationConnectionRelease(generation);
		/* Keep the test's own lease until all racing callers have left. */
		assert(XS_GenerationRetain(generation));
		xrtAtomic32Store(&ready, 0, XMEMORY_RELEASE);
		xrtAtomic32Store(&run_workers, 0, XMEMORY_RELEASE);
		xrtAtomic32Store(&admissions, 0, XMEMORY_RELEASE);
		for ( unsigned i = 0; i < 4; i++ ) {
			threads[i] = xrtThreadCreate(race_acquire, generation, 0);
			assert(threads[i] != NULL);
		}
		while ( xrtAtomic32Load(&ready, XMEMORY_ACQUIRE) != 4 ) xrtSleep(1);
		xrtAtomic32Store(&run_workers, 1, XMEMORY_RELEASE);
		while ( xrtAtomic32Load(&admissions, XMEMORY_ACQUIRE) == 0 ) xrtSleep(0);
		XS_GenerationRetire(generation, NULL, NULL, false, test_quiesce, test_finalize);
		assert(!XS_GenerationConnectionAcquire(generation));
		assert(xrtAtomic32Load(&quiesced, XMEMORY_ACQUIRE) == round);
		XS_GenerationConnectionRelease(generation);
		for ( unsigned i = 0; i < 4; i++ ) {
			assert(xrtThreadWait(threads[i]) == XWAIT_OK);
			xrtThreadDestroy(threads[i]);
		}
		XS_GenerationRelease(generation);
		XS_GenerationReaperDrain();
		assert(xrtAtomic32Load(&quiesced, XMEMORY_ACQUIRE) == round + 1);
		assert(xrtAtomic32Load(&finalized, XMEMORY_ACQUIRE) == round + 1);
		assert(xrtAtomic32Load(&g_XS_RetiredGenerations, XMEMORY_ACQUIRE) == 0);
		assert(xrtAtomic64Load(&shared_budget.Used, XMEMORY_ACQUIRE) == 0);
	}
	xrtAtomic32Store(&g_XS_RetiredGenerations, XS_RETIRED_GENERATIONS_LIMIT, XMEMORY_RELEASE);
	assert(!XS_GenerationRetirementFits(1));
	assert(XS_GenerationRetirementFits(0));
	xrtAtomic32Store(&g_XS_RetiredGenerations, 0, XMEMORY_RELEASE);
	XS_GenerationReaperUnit();
	puts("PASS 40 publication/retirement races, connection limits, exactly-once finalizers and byte budgets");
}

static void check_config(const char* text, bool expected, const char* message)
{
	XS_App app;
	bool ok = XS_ConfigLoadMemory("site/xs.json", text, strlen(text), &app);
	assert(ok == expected);
	if ( !ok ) assert(strstr(app.ParseError, message) != NULL);
	if ( ok && app.ServerCount && app.Servers[0]->DefaultHost->DevInc != NULL ) {
		const char* paths = app.Servers[0]->DefaultHost->DevInc;
		const char* second = strchr(paths, ';');
		assert(xrtPathIsAbs(paths) && second != NULL && xrtPathIsAbs(second + 1));
		assert(strstr(paths, ";;") == NULL);
	}
	XS_ConfigFree(&app);
}

static int32 race_cache(ptr unused)
{
	(void)unused;
	for ( unsigned i = 0; i < 500; i++ ) {
		size_t size = 0;
		const unsigned char* data = XS_VfsEntryData(0, &size);
		assert(data != NULL && size == 1 && data[0] == 'X');
	}
	return 0;
}

static void check_cache(void)
{
	str path = NULL;
	size_t written = 0;
	xthread* threads[4];
	g_XS_Vfs.hSelf = xrtFileTemp(NULL, "xs-cache-", ".bin", &path);
	assert(g_XS_Vfs.hSelf != NULL);
	assert(xrtWriteFull(g_XS_Vfs.hSelf, "X", 1, &written));
	g_XS_Vfs.pCacheLock = xrtMutexCreate();
	g_XS_Vfs.pEntries = xrtCalloc(1, sizeof(XS_VfsEntry));
	assert(g_XS_Vfs.pCacheLock != NULL && g_XS_Vfs.pEntries != NULL);
	g_XS_Vfs.Header.EntryCount = 1;
	g_XS_Vfs.pEntries[0].OrigSize = 1;
	g_XS_Vfs.pEntries[0].CompSize = 1;
	g_XS_Vfs.pEntries[0].OrigCrc32 = XS_VfsCrc32(0, "X", 1);
	g_XS_Vfs.bActive = true;
	assert(xrtMemDebugFailAfter(0));
	size_t size = 0;
	assert(XS_VfsEntryData(0, &size) == NULL);
	xrtMemDebugFailClear();
	assert(g_XS_Vfs.iCachedBytes == 0 && g_XS_Vfs.pEntries[0].pCached == NULL);
	for ( unsigned i = 0; i < 4; i++ ) {
		threads[i] = xrtThreadCreate(race_cache, NULL, 0);
		assert(threads[i] != NULL);
	}
	for ( unsigned i = 0; i < 4; i++ ) {
		assert(xrtThreadWait(threads[i]) == XWAIT_OK);
		xrtThreadDestroy(threads[i]);
	}
	assert(g_XS_Vfs.iCachedBytes == 1);
	xrtFree(g_XS_Vfs.pEntries[0].pCached);
	g_XS_Vfs.pEntries[0].pCached = NULL;
	g_XS_Vfs.iCachedBytes = XS_VFS_CACHE_BYTES_LIMIT;
	assert(XS_VfsEntryData(0, &size) == NULL);
	assert(xrtClose(g_XS_Vfs.hSelf));
	assert(xrtFileDelete(path));
	xrtFree(path);
	xrtMutexDestroy(g_XS_Vfs.pCacheLock);
	xrtFree(g_XS_Vfs.pEntries);
	memset(&g_XS_Vfs, 0, sizeof(g_XS_Vfs));
	puts("PASS VFS cold-cache concurrency, allocation failure and cache byte limit");
}

static void check_config_mutations(void)
{
	char mutated[512];
	uint32 random = 0x9173u;
	xmemdebugsnapshot before, after;
	xrtClearError();
	xrtMemDebugSnapshot(&before);
	for ( unsigned i = 0; i < 512; i++ ) {
		XS_App app;
		size_t size = strlen(valid_config);
		memcpy(mutated, valid_config, size);
		random ^= random << 13; random ^= random >> 17; random ^= random << 5;
		mutated[random % size] = (char)(random >> 24);
		if ( i % 3 == 0 ) size = random % size;
		(void)XS_ConfigLoadMemory("site/xs.json", mutated, size, &app);
		XS_ConfigFree(&app);
		xrtClearError();
		xrtMemDebugSnapshot(&after);
		assert(after.LiveBytes == before.LiveBytes && after.LiveCount == before.LiveCount);
	}
	puts("PASS 512 deterministic config mutations/truncations without leaked state");
}

static uint8_t* decode_vfs_bytes(const uint8_t* data, size_t size, uint64_t original)
{
	size_t written;
	assert(size <= UINT32_MAX && xrtFileResize(g_XS_Vfs.hSelf, size));
	assert(xrtWriteAtFull(g_XS_Vfs.hSelf, 0, data, size, &written));
	return XS_VfsDecodeLzma(0, (uint32_t)size, original);
}

static void check_lzma_consumption(void)
{
	/* XS pack's LZMA1 stream for 1024 'A' bytes, without an end marker. */
	static const uint8_t compressed[] = {
		0x5d, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0xef, 0xfb,
		0xbf, 0xfe, 0xa3, 0xb0, 0xde, 0xe0, 0x72, 0x00
	};
	uint8_t trailing[sizeof(compressed) + 1u];
	xmemdebugsnapshot before, after;
	xrtClearError();
	xrtMemDebugSnapshot(&before);
	str path = NULL;
	g_XS_Vfs.hSelf = xrtFileTemp(NULL, "xs-lzma-", ".bin", &path);
	assert(g_XS_Vfs.hSelf != NULL);
	uint8_t* tool = XS_PackUnlzma(compressed, sizeof(compressed), 1024);
	uint8_t* runtime = decode_vfs_bytes(compressed, sizeof(compressed), 1024);
	assert(tool != NULL && runtime != NULL);
	for ( size_t i = 0; i < 1024; i++ ) assert(tool[i] == 'A' && runtime[i] == 'A');
	free(tool);
	xrtFree(runtime);
	memcpy(trailing, compressed, sizeof(compressed));
	for ( unsigned i = 0; i < 3; i++ ) {
		trailing[sizeof(compressed)] = i == 2 ? 0xff : (uint8_t)i;
		assert(XS_PackUnlzma(trailing, sizeof(trailing), 1024) == NULL);
		assert(decode_vfs_bytes(trailing, sizeof(trailing), 1024) == NULL);
	}
	for ( size_t size = 0; size < sizeof(compressed); size++ ) {
		assert(XS_PackUnlzma(compressed, size, 1024) == NULL);
		assert(decode_vfs_bytes(compressed, size, 1024) == NULL);
	}
	assert(XS_PackUnlzma(compressed, sizeof(compressed), 1025) == NULL);
	assert(decode_vfs_bytes(compressed, sizeof(compressed), 1025) == NULL);
	assert(xrtClose(g_XS_Vfs.hSelf));
	g_XS_Vfs.hSelf = NULL;
	assert(xrtFileDelete(path));
	xrtFree(path);
	xrtClearError();
	xrtMemDebugSnapshot(&after);
	assert(after.LiveBytes == before.LiveBytes && after.LiveCount == before.LiveCount);
	puts("PASS LZMA exact input/output consumption, trailing bytes and all truncated prefixes without leaks");
}

static void check_lzma_streaming(void)
{
	const size_t original_size = 256u << 10;
	uint8_t* original = malloc(original_size);
	uint32 random = 0x8273ab12u;
	str path = NULL;
	xmemdebugsnapshot before, after;
	xrtClearError();
	xrtMemDebugSnapshot(&before);
	assert(original != NULL);
	for ( size_t i = 0; i < original_size; i++ ) {
		random ^= random << 13; random ^= random >> 17; random ^= random << 5;
		original[i] = (uint8_t)(random >> 24);
	}
	g_XS_Vfs.hSelf = xrtFileTemp(NULL, "xs-lzma-stream-", ".bin", &path);
	assert(g_XS_Vfs.hSelf != NULL);
	for ( int marked = 0; marked < 2; marked++ ) {
		size_t capacity = original_size + original_size / 3u + 65536u;
		uint8_t* compressed = malloc(capacity);
		CLzmaEncProps properties;
		SizeT props_size = LZMA_PROPS_SIZE, packed_size = capacity - LZMA_PROPS_SIZE;
		assert(compressed != NULL);
		LzmaEncProps_Init(&properties);
		properties.level = 1;
		properties.numThreads = 1;
		properties.reduceSize = original_size;
		assert(LzmaEncode(compressed + LZMA_PROPS_SIZE, &packed_size, original, original_size,
			&properties, compressed, &props_size, marked, NULL,
			&s_XS_PackLzmaAlloc, &s_XS_PackLzmaAlloc) == SZ_OK);
		size_t compressed_size = packed_size + LZMA_PROPS_SIZE;
		assert(props_size == LZMA_PROPS_SIZE && compressed_size > 3u * XS_VFS_INPUT_CHUNK_BYTES);
		uint8_t* tool = XS_PackUnlzma(compressed, compressed_size, original_size);
		uint8_t* runtime = decode_vfs_bytes(compressed, compressed_size, original_size);
		assert(tool != NULL && runtime != NULL);
		assert(memcmp(tool, original, original_size) == 0 && memcmp(runtime, original, original_size) == 0);
		free(tool); xrtFree(runtime);
		XS_VfsEntry entry = {0};
		entry.Method = 1;
		entry.CompSize = (uint32_t)compressed_size;
		entry.OrigSize = original_size;
		entry.OrigCrc32 = XS_VfsCrc32(0, original, original_size);
		g_XS_Vfs.pEntries = &entry;
		size_t size = 0;
		runtime = (uint8_t*)XS_VfsEntryDataLocked(0, &size);
		assert(runtime != NULL && size == original_size && g_XS_Vfs.iCachedBytes == original_size);
		assert(XS_VfsEntryDataLocked(0, &size) == runtime);
		xrtFree(entry.pCached); entry.pCached = NULL; g_XS_Vfs.iCachedBytes = 0;
		entry.OrigCrc32 ^= 1;
		assert(XS_VfsEntryDataLocked(0, &size) == NULL);
		assert(entry.pCached == NULL && g_XS_Vfs.iCachedBytes == 0);
		xmemdebugsnapshot baseline;
		xrtClearError(); xrtMemDebugSnapshot(&baseline);
		for ( unsigned failure = 0; failure < 3; failure++ ) {
			assert(xrtMemDebugFailAfter(failure));
			assert(XS_VfsDecodeLzma(0, (uint32_t)compressed_size, original_size) == NULL);
			assert(xrtMemDebugFailTriggered());
			xrtMemDebugFailClear(); xrtClearError(); xrtMemDebugSnapshot(&after);
			assert(after.LiveBytes == baseline.LiveBytes && after.LiveCount == baseline.LiveCount);
		}
		/* The second chunk is absent even though the index declares a full stream. */
		assert(xrtFileResize(g_XS_Vfs.hSelf, XS_VFS_INPUT_CHUNK_BYTES + LZMA_PROPS_SIZE));
		assert(XS_VfsDecodeLzma(0, (uint32_t)compressed_size, original_size) == NULL);
		xrtClearError(); xrtMemDebugSnapshot(&after);
		assert(after.LiveBytes == baseline.LiveBytes && after.LiveCount == baseline.LiveCount);
		assert(decode_vfs_bytes(compressed, compressed_size - 1u, original_size) == NULL);
		compressed[compressed_size] = 0;
		assert(decode_vfs_bytes(compressed, compressed_size + 1u, original_size) == NULL);
		free(compressed);
		g_XS_Vfs.pEntries = NULL;
	}
	assert(xrtClose(g_XS_Vfs.hSelf));
	assert(xrtFileDelete(path)); xrtFree(path); free(original);
	memset(&g_XS_Vfs, 0, sizeof(g_XS_Vfs));
	xrtClearError(); xrtMemDebugSnapshot(&after);
	assert(after.LiveBytes == before.LiveBytes && after.LiveCount == before.LiveCount);
	puts("PASS multi-chunk marked/unmarked LZMA, cache CRC, three allocation failures and missing input without leaks");
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
	check_lifetimes();
	check_cache();
	check_config_mutations();
	check_lzma_consumption();
	check_lzma_streaming();
	return 0;
}
