/* Standalone tests; no xs build or network connection is required.
 *   gcc -std=c99 -O2 -Wall -Wextra -Werror -pedantic tools/xtp2_test.c -o xtp2_test
 *   ./xtp2_test [existing-fuzz-corpus-directory]
 * Add -DXTP2_TEST_LEGACY to compare with the historical client's builder
 * (Windows additionally needs -lws2_32). No legacy network code is run.
 * Define XTP2_TEST_EXTERNAL to link a separately compiled implementation.
 */
#ifndef XTP2_TEST_EXTERNAL
#define XTP2_IMPLEMENTATION
#endif
#include "../lib/xtp2.h"
#include "../lib/xtp2.h" /* Deliberate repeated include. */
#include <stdio.h>
#include <stdlib.h>

#ifdef XTP2_TEST_LEGACY
#define main xtp2_legacy_client_main
#include "../dev/v1/tools/xtp_smoke_client.c"
#undef main
#endif

static size_t checks;
#define CHECK(expr) do { ++checks; if (!(expr)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); exit(1); \
} } while (0)

/* Independent literal fixtures, not produced by the library under test. */
static const unsigned char ping[] = {
    0x78,0x74,0x70,0x02, 0x24,0,0,0, 1,0,0,0,0,0,0,0,
    0,0, 1,0, 4,0, 0,0, 0,0,0,0, 0,0,0,0,
    'p','i','n','g'
};
static const unsigned char binary[] = {
    0x78,0x74,0x70,0x02, 0x36,0,0,0,
    0x10,0x32,0x54,0x76,0x98,0xba,0xdc,0xfe,
    0x5a,0xa5, 2,0, 3,0, 2,0, 4,0,0,0, 0,0,0,0x80,
    2,0,3,0, 2,0,0,0,
    'c',0,'m', 'k',0, 0,0xff,'z', 'k',0, 0,1,0xfe,0xff
};
static const unsigned char empty[] = {
    0x78,0x74,0x70,0x02, 0x20,0,0,0, 0,0,0,0,0,0,0,0,
    0,0, 1,0, 0,0, 0,0, 0,0,0,0, 0,0,0,0
};

static xtp2_span span(const void *data, size_t size)
{
    xtp2_span s;
    s.data = data; s.size = size;
    return s;
}

static bool same(xtp2_span a, xtp2_span b)
{
    return a.size == b.size && (!a.size || memcmp(a.data, b.data, a.size) == 0);
}

static bool zeroed(const void *data, size_t size)
{
    const unsigned char *p = (const unsigned char *)data;
    size_t i;
    for (i = 0; i < size; ++i) if (p[i]) return false;
    return true;
}

static void put32(unsigned char *p, uint32_t n)
{
    size_t i;
    for (i = 0; i < 4; ++i) p[i] = (unsigned char)(n >> (i * 8));
}

static void check_view(const xtp2_message *m, const unsigned char *wire, size_t size)
{
    xtp2_message reference;
    xtp2_iter it;
    xtp2_param param;
    size_t count = 0;
    CHECK(xtp2_decode(wire, size, NULL, &reference) == XTP2_OK);
    CHECK(m->id == reference.id && m->status == reference.status);
    CHECK(m->flags == reference.flags && m->type == reference.type);
    CHECK(same(m->command, reference.command) && same(m->body, reference.body));
    CHECK(same(m->packet, span(wire, size)));
    it = xtp2_params(m);
    while (xtp2_param_next(&it, &param)) {
        ++count;
        CHECK((const unsigned char *)param.key.data >= (const unsigned char *)m->packet.data);
        CHECK((const unsigned char *)param.value.data + param.value.size <=
              (const unsigned char *)m->body.data);
    }
    CHECK(count == m->param_count && count == reference.param_count);
    CHECK(zeroed(&param, sizeof(param)));
}

static xtp2_packet binary_packet(xtp2_param params[2])
{
    xtp2_packet p;
    memset(&p, 0, sizeof(p));
    p.id = UINT64_C(0xfedcba9876543210);
    p.status = INT32_MIN; p.type = XTP2_RESPONSE; p.flags = 0xa55a;
    p.command = span(binary + 40, 3);
    params[0].key = span(binary + 43, 2);
    params[0].value = span(binary + 45, 3);
    params[1].key = span(binary + 48, 2);
    params[1].value = span(NULL, 0);
    p.params = params; p.param_count = 2;
    p.body = span(binary + 50, 4);
    return p;
}

static void test_golden(void)
{
    unsigned char out[128];
    xtp2_packet p;
    xtp2_message m;
    xtp2_param params[2], param;
    xtp2_iter it;
    xtp2_span value;
    size_t size, written, a, b, c;
    const int32_t statuses[] = { INT32_MIN, -1, 0, 1, INT32_MAX };
    const uint64_t ids[] = { 0, 1, UINT64_C(0x8000000000000000), UINT64_MAX };
    memset(&p, 0, sizeof(p));
    p.type = XTP2_REQUEST; p.id = 1; p.command = xtp2_text("ping");
    CHECK(xtp2_measure(&p, NULL, &size) == XTP2_OK && size == sizeof(ping));
    CHECK(xtp2_encode(&p, NULL, out, sizeof(out), &written) == XTP2_OK);
    CHECK(written == sizeof(ping) && memcmp(out, ping, written) == 0);
    CHECK(xtp2_decode(ping, sizeof(ping), NULL, &m) == XTP2_OK);
    CHECK(m.id == 1 && m.status == 0 && m.flags == 0 && m.param_count == 0);
    CHECK(xtp2_wants_reply(&m) && same(m.command, xtp2_text("ping")));
    CHECK(!xtp2_wants_reply(NULL) && xtp2_text(NULL).size == 0);
    p = binary_packet(params);
    CHECK(xtp2_encode(&p, NULL, out + 1, sizeof(out) - 1, &written) == XTP2_OK);
    CHECK(written == sizeof(binary) && memcmp(out + 1, binary, written) == 0);
    CHECK(xtp2_decode(out + 1, written, NULL, &m) == XTP2_OK); /* Unaligned. */
    CHECK(m.id == p.id && m.status == INT32_MIN && m.flags == 0xa55a);
    CHECK(!xtp2_wants_reply(&m));
    CHECK(xtp2_find(&m, params[0].key, &value) && same(value, params[0].value));
    CHECK(!xtp2_find(&m, xtp2_text("k"), &value) && zeroed(&value, sizeof(value)));
    it = xtp2_params(&m);
    CHECK(xtp2_param_next(&it, &param) && same(param.value, params[0].value));
    CHECK(xtp2_param_next(&it, &param) && param.value.size == 0);
    CHECK(!xtp2_param_next(&it, &param) && zeroed(&param, sizeof(param)));
    for (a = 1; a <= 4; ++a) for (b = 0; b < 5; ++b) for (c = 0; c < 4; ++c) {
        p.type = (uint16_t)a; p.status = statuses[b]; p.id = ids[c]; p.flags = UINT16_MAX;
        CHECK(xtp2_encode(&p, NULL, out, sizeof(out), &written) == XTP2_OK);
        CHECK(xtp2_decode(out, written, NULL, &m) == XTP2_OK);
        CHECK(m.id == p.id && m.status == p.status && m.flags == p.flags);
        CHECK(xtp2_wants_reply(&m) == (p.type == XTP2_REQUEST && p.id != 0));
    }
    memset(&p, 0, sizeof(p));
    p.type = XTP2_REQUEST;
    CHECK(xtp2_encode(&p, NULL, out, sizeof(out), &written) == XTP2_OK);
    CHECK(written == sizeof(empty) && memcmp(out, empty, written) == 0);
    p.params = params; p.param_count = 2;
    params[0].key = span(NULL, 0); params[0].value = xtp2_text("first");
    params[1].key = xtp2_text(""); params[1].value = span(NULL, 0);
    p.command = span("ignored", 0); p.body = span("ignored", 0);
    CHECK(xtp2_encode(&p, NULL, out, sizeof(out), &written) == XTP2_OK);
    CHECK(xtp2_decode(out, written, NULL, &m) == XTP2_OK);
    CHECK(m.command.size == 0 && m.body.size == 0 && m.id == 0);
    CHECK(xtp2_find(&m, span(NULL, 0), &value) && same(value, xtp2_text("first")));
}

static void splits(const unsigned char *wire, size_t size)
{
    size_t split, used, i;
    xtp2_message m;
    xtp2_processor *rx;
    xtp2_result r;
    for (split = 0; split <= size; ++split) {
        CHECK(xtp2_create(NULL, &rx) == XTP2_OK);
        r = xtp2_feed(rx, wire, split, &used, &m);
        CHECK(used == split && r == (split == size ? XTP2_MESSAGE : XTP2_MORE));
        if (split < size) {
            CHECK(zeroed(&m, sizeof(m)));
            CHECK(xtp2_feed(rx, wire + split, size - split, &used, &m) == XTP2_MESSAGE);
            CHECK(used == size - split);
        }
        check_view(&m, wire, size);
        CHECK(xtp2_finish(rx) == XTP2_OK && xtp2_finish(rx) == XTP2_OK);
        CHECK(xtp2_feed(rx, NULL, 0, &used, &m) == XTP2_E_STATE && used == 0);
        xtp2_reset(rx);
        CHECK(xtp2_feed(rx, wire, split, &used, &m) == (split == size ? XTP2_MESSAGE : XTP2_MORE));
        r = (split == 0 || split == size) ? XTP2_OK : XTP2_E_TRUNCATED;
        CHECK(xtp2_finish(rx) == r && xtp2_finish(rx) == r);
        CHECK(xtp2_feed(rx, wire, size, &used, &m) == (r ? r : XTP2_E_STATE));
        CHECK(used == 0 && zeroed(&m, sizeof(m)));
        xtp2_reset(rx);
        for (i = 0; i < size; ++i) {
            r = xtp2_feed(rx, wire + i, 1, &used, &m);
            CHECK(used == 1 && r == (i + 1 == size ? XTP2_MESSAGE : XTP2_MORE));
        }
        check_view(&m, wire, size);
        xtp2_destroy(rx);
    }
}

static void test_stream(void)
{
    unsigned char chain[sizeof(ping) + sizeof(binary) + sizeof(empty) + 17];
    unsigned char saved[sizeof(ping)];
    xtp2_processor *rx, *other;
    xtp2_message m;
    size_t offset = 0, used;
    splits(ping, sizeof(ping));
    splits(binary, sizeof(binary));
    splits(empty, sizeof(empty));
    memcpy(chain, ping, sizeof(ping));
    memcpy(chain + sizeof(ping), binary, sizeof(binary));
    memcpy(chain + sizeof(ping) + sizeof(binary), empty, sizeof(empty));
    memcpy(chain + sizeof(chain) - 17, ping, 17);
    CHECK(xtp2_create(NULL, &rx) == XTP2_OK);
    CHECK(xtp2_create(NULL, &other) == XTP2_OK);
    CHECK(xtp2_feed(rx, chain, sizeof(chain), &used, &m) == XTP2_MESSAGE);
    CHECK(used == sizeof(ping)); offset += used;
    memcpy(saved, m.packet.data, m.packet.size); /* Preserve before resuming. */
    CHECK(xtp2_feed(other, binary, 33, &used, &m) == XTP2_MORE && used == 33);
    CHECK(xtp2_feed(rx, chain + offset, sizeof(chain) - offset, &used, &m) == XTP2_MESSAGE);
    CHECK(used == sizeof(binary)); offset += used;
    check_view(&m, binary, sizeof(binary));
    CHECK(xtp2_feed(rx, chain + offset, sizeof(chain) - offset, &used, &m) == XTP2_MESSAGE);
    CHECK(used == sizeof(empty)); offset += used;
    check_view(&m, empty, sizeof(empty)); /* Reuses a previous heap buffer. */
    CHECK(xtp2_feed(rx, chain + offset, sizeof(chain) - offset, &used, &m) == XTP2_MORE);
    CHECK(used == 17 && xtp2_finish(rx) == XTP2_E_TRUNCATED);
    CHECK(xtp2_feed(other, binary + 33, sizeof(binary) - 33, &used, &m) == XTP2_MESSAGE);
    check_view(&m, binary, sizeof(binary));
    CHECK(xtp2_decode(saved, sizeof(saved), NULL, &m) == XTP2_OK);
    check_view(&m, ping, sizeof(ping));
    xtp2_destroy(rx); xtp2_destroy(other);
}

static void bad_frame(const unsigned char *data, size_t size,
                       const xtp2_limits *limits, xtp2_result expected)
{
    xtp2_config config;
    xtp2_processor *rx;
    xtp2_message m;
    xtp2_result r = XTP2_MORE;
    size_t i, used;
    memset(&m, 0xa5, sizeof(m));
    CHECK(xtp2_decode(data, size, limits, &m) == expected);
    CHECK(zeroed(&m, sizeof(m)));
    xtp2_config_init(&config);
    if (limits) config.limits = *limits;
    CHECK(xtp2_create(&config, &rx) == XTP2_OK);
    for (i = 0; i < size && r == XTP2_MORE; ++i) {
        r = xtp2_feed(rx, data + i, 1, &used, &m);
        CHECK(used <= 1 && zeroed(&m, sizeof(m)));
    }
    if (r == XTP2_MORE) r = xtp2_finish(rx);
    CHECK(r == expected);
    CHECK(xtp2_feed(rx, ping, sizeof(ping), &used, &m) == expected && used == 0);
    CHECK(xtp2_finish(rx) == expected && xtp2_finish(rx) == expected);
    xtp2_destroy(rx);
}

static void test_invalid(void)
{
    unsigned char data[sizeof(binary) + 1];
    xtp2_message m;
    xtp2_config c;
    xtp2_processor *rx;
    xtp2_packet p;
    xtp2_param params[2], param;
    xtp2_span value;
    xtp2_iter it;
    size_t used, written;
    memcpy(data, binary, sizeof(binary)); data[0] = 'X';
    bad_frame(data, sizeof(binary), NULL, XTP2_E_MAGIC);
    memcpy(data, binary, sizeof(binary)); data[3] = 1;
    bad_frame(data, sizeof(binary), NULL, XTP2_E_VERSION);
    memcpy(data, binary, sizeof(binary)); data[18] = 0;
    bad_frame(data, sizeof(binary), NULL, XTP2_E_TYPE);
    data[18] = 5;
    bad_frame(data, sizeof(binary), NULL, XTP2_E_TYPE);
    memcpy(data, empty, sizeof(empty)); put32(data + 4, 31);
    bad_frame(data, sizeof(empty), NULL, XTP2_E_LENGTH);
    memcpy(data, empty, sizeof(empty)); data[22] = 255; data[23] = 255;
    bad_frame(data, sizeof(empty), NULL, XTP2_E_LENGTH);
    memcpy(data, binary, sizeof(binary)); data[32] = 3;
    bad_frame(data, sizeof(binary), NULL, XTP2_E_LENGTH);
    memcpy(data, binary, sizeof(binary)); data[34] = 2;
    bad_frame(data, sizeof(binary), NULL, XTP2_E_LENGTH);
    memcpy(data, binary, sizeof(binary)); put32(data + 24, UINT32_MAX);
    put32(data + 4, UINT32_MAX);
    bad_frame(data, sizeof(binary), NULL, XTP2_E_LENGTH);
    memcpy(data, empty, sizeof(empty)); put32(data + 4, 1048577u);
    bad_frame(data, sizeof(empty), NULL, XTP2_E_LIMIT);
    bad_frame(binary, 31, NULL, XTP2_E_TRUNCATED);
    bad_frame(binary, 39, NULL, XTP2_E_TRUNCATED);
    bad_frame(binary, sizeof(binary) - 1, NULL, XTP2_E_TRUNCATED);
    memcpy(data, binary, sizeof(binary)); data[sizeof(binary)] = 0;
    CHECK(xtp2_decode(data, sizeof(data), NULL, &m) == XTP2_E_LENGTH);
    CHECK(xtp2_decode(NULL, 0, NULL, &m) == XTP2_E_TRUNCATED);
    CHECK(xtp2_decode(NULL, 1, NULL, &m) == XTP2_E_ARGUMENT);
    CHECK(xtp2_decode(binary, sizeof(binary), NULL, NULL) == XTP2_E_ARGUMENT);
    CHECK(xtp2_create(NULL, NULL) == XTP2_E_ARGUMENT);
    CHECK(xtp2_finish(NULL) == XTP2_E_ARGUMENT);
    xtp2_config_init(NULL); xtp2_reset(NULL); xtp2_destroy(NULL);
    CHECK(xtp2_feed(NULL, NULL, 0, &used, &m) == XTP2_E_ARGUMENT && used == 0);
    CHECK(xtp2_create(NULL, &rx) == XTP2_OK);
    CHECK(xtp2_feed(rx, ping, 10, &used, &m) == XTP2_MORE && used == 10);
    CHECK(xtp2_feed(rx, NULL, 1, &used, &m) == XTP2_E_ARGUMENT && used == 0);
    CHECK(xtp2_feed(rx, ping, 1, NULL, &m) == XTP2_E_ARGUMENT);
    CHECK(xtp2_feed(rx, ping, 1, &used, NULL) == XTP2_E_ARGUMENT && used == 0);
    CHECK(xtp2_feed(rx, NULL, 0, &used, &m) == XTP2_MORE && used == 0);
    CHECK(xtp2_feed(rx, ping + 10, sizeof(ping) - 10, &used, &m) == XTP2_MESSAGE);
    check_view(&m, ping, sizeof(ping)); /* Bad calls did not advance. */
    xtp2_destroy(rx);
    it = xtp2_params(NULL);
    CHECK(!xtp2_param_next(&it, &param) && zeroed(&param, sizeof(param)));
    CHECK(!xtp2_param_next(NULL, &param) && !xtp2_param_next(&it, NULL));
    CHECK(!xtp2_find(NULL, span(NULL, 0), &value) && zeroed(&value, sizeof(value)));
    CHECK(xtp2_decode(binary, sizeof(binary), NULL, &m) == XTP2_OK);
    CHECK(!xtp2_find(&m, span(NULL, 1), &value));
    CHECK(!xtp2_find(&m, xtp2_text("key"), NULL));
    p = binary_packet(params);
    CHECK(xtp2_measure(NULL, NULL, &written) == XTP2_E_ARGUMENT && written == 0);
    CHECK(xtp2_measure(&p, NULL, NULL) == XTP2_E_ARGUMENT);
    CHECK(xtp2_encode(&p, NULL, data, sizeof(data), NULL) == XTP2_E_ARGUMENT);
    CHECK(xtp2_encode(&p, NULL, NULL, 0, &written) == XTP2_E_ARGUMENT && written == 0);
    xtp2_config_init(&c);
    CHECK(c.limits.max_packet == 1048576 && c.limits.max_params == 256);
    for (int code = XTP2_E_STATE; code <= XTP2_MESSAGE; ++code)
        CHECK(strcmp(xtp2_error_string((xtp2_result)code), "unknown XTP2 result") != 0);
    CHECK(strcmp(xtp2_error_string((xtp2_result)99), "unknown XTP2 result") == 0);
}

static void encode_error(xtp2_packet *p, const xtp2_limits *limits, size_t capacity,
                          xtp2_result expected)
{
    unsigned char out[128], saved[128];
    size_t written = 123;
    memset(out, 0xa5, sizeof(out)); memcpy(saved, out, sizeof(out));
    CHECK(xtp2_encode(p, limits, out, capacity, &written) == expected);
    CHECK(written == 0 && memcmp(out, saved, sizeof(out)) == 0);
}

static void test_limits(void)
{
    xtp2_param params[2];
    xtp2_packet p = binary_packet(params);
    xtp2_config c;
    xtp2_limits exact;
    xtp2_processor *rx;
    xtp2_message m;
    unsigned char out[128];
    size_t size;
    int field;
    xtp2_config_init(&c);
    exact = c.limits;
    exact.max_packet = sizeof(binary); exact.max_body = 4;
    exact.max_command = 3; exact.max_params = 2; exact.max_key = 2; exact.max_value = 3;
    CHECK(xtp2_measure(&p, &exact, &size) == XTP2_OK && size == sizeof(binary));
    CHECK(xtp2_decode(binary, sizeof(binary), &exact, &m) == XTP2_OK);
    for (field = 0; field < 6; ++field) {
        c.limits = exact;
        switch (field) {
        case 0: --c.limits.max_packet; break;
        case 1: --c.limits.max_body; break;
        case 2: --c.limits.max_command; break;
        case 3: --c.limits.max_params; break;
        case 4: --c.limits.max_key; break;
        case 5: --c.limits.max_value; break;
        }
        bad_frame(binary, sizeof(binary), &c.limits, XTP2_E_LIMIT);
        encode_error(&p, &c.limits, sizeof(out), XTP2_E_LIMIT);
    }
    memset(&c.limits, 0, sizeof(c.limits)); c.limits.max_packet = 32;
    CHECK(xtp2_decode(empty, sizeof(empty), &c.limits, &m) == XTP2_OK);
    CHECK(xtp2_create(&c, &rx) == XTP2_OK); xtp2_destroy(rx);
    for (field = 0; field < 7; ++field) {
        xtp2_config_init(&c);
        switch (field) {
        case 0: c.limits.max_packet = 31; break;
        case 1: c.limits.max_command = 65536; break;
        case 2: c.limits.max_params = 65536; break;
        case 3: c.limits.max_key = 65536; break;
        case 4: c.limits.max_value = 65536; break;
#if SIZE_MAX > UINT32_MAX
        case 5: c.limits.max_packet = (size_t)UINT32_MAX + 1; break;
        case 6: c.limits.max_body = (size_t)UINT32_MAX + 1; break;
#else
        default: c.limits.max_packet = 0; break;
#endif
        }
        CHECK(xtp2_create(&c, &rx) == XTP2_E_ARGUMENT && rx == NULL);
        CHECK(xtp2_measure(&p, &c.limits, &size) == XTP2_E_ARGUMENT && size == 0);
        CHECK(xtp2_decode(binary, sizeof(binary), &c.limits, &m) == XTP2_E_ARGUMENT);
    }
    encode_error(&p, NULL, sizeof(binary) - 1, XTP2_E_CAPACITY);
    p.type = 0; encode_error(&p, NULL, sizeof(out), XTP2_E_TYPE);
    p = binary_packet(params); p.param_count = SIZE_MAX;
    encode_error(&p, NULL, sizeof(out), XTP2_E_LENGTH);
    p = binary_packet(params); p.command.size = SIZE_MAX;
    encode_error(&p, NULL, sizeof(out), XTP2_E_LENGTH);
#if SIZE_MAX > UINT32_MAX
    p = binary_packet(params); p.body.size = SIZE_MAX;
    encode_error(&p, NULL, sizeof(out), XTP2_E_LENGTH);
#endif
    p = binary_packet(params); params[0].key.size = 65536;
    encode_error(&p, NULL, sizeof(out), XTP2_E_LENGTH);
    p = binary_packet(params); params[0].value.size = 65536;
    encode_error(&p, NULL, sizeof(out), XTP2_E_LENGTH);
    p = binary_packet(params); p.params = NULL;
    encode_error(&p, NULL, sizeof(out), XTP2_E_ARGUMENT);
    p = binary_packet(params); p.command.data = NULL;
    encode_error(&p, NULL, sizeof(out), XTP2_E_ARGUMENT);
    p = binary_packet(params); p.body.data = NULL;
    encode_error(&p, NULL, sizeof(out), XTP2_E_ARGUMENT);
    p = binary_packet(params); params[0].key.data = NULL;
    encode_error(&p, NULL, sizeof(out), XTP2_E_ARGUMENT);
    p = binary_packet(params); params[0].value.data = NULL;
    encode_error(&p, NULL, sizeof(out), XTP2_E_ARGUMENT);
}

/* Always-moving allocator with exact bookkeeping, tail guards and faults.
 * long double/pointer members provide the processor's required alignment. */
typedef union {
    size_t size;
    long double align;
    void *pointer;
    uint64_t integer;
} allocation;
typedef struct {
    size_t calls, fail_at, blocks, bytes, peak;
} tracker;

static void *tracked_realloc(void *user, void *ptr, size_t size)
{
    tracker *t = (tracker *)user;
    allocation *old = ptr ? (allocation *)ptr - 1 : NULL, *next;
    size_t old_size = old ? old->size : 0, i;
    if (old) for (i = 0; i < 16; ++i)
        CHECK(((unsigned char *)ptr)[old_size + i] == 0x5a);
    if (size && ++t->calls == t->fail_at) return NULL;
    next = size ? (allocation *)malloc(sizeof(*next) + size + 16) : NULL;
    CHECK(!size || next != NULL);
    if (next) {
        next->size = size;
        if (ptr) memcpy(next + 1, ptr, old_size < size ? old_size : size);
        memset((unsigned char *)(next + 1) + size, 0x5a, 16);
        ++t->blocks; t->bytes += size;
        if (t->bytes > t->peak) t->peak = t->bytes;
    }
    if (old) { --t->blocks; t->bytes -= old_size; free(old); }
    return next ? next + 1 : NULL;
}

static void test_allocator(void)
{
    unsigned char *body = (unsigned char *)malloc(12000), *wire;
    unsigned char header[32];
    xtp2_param params[2];
    xtp2_packet p = binary_packet(params);
    xtp2_config c;
    xtp2_processor *rx;
    xtp2_message m;
    xtp2_result r;
    tracker t;
    size_t size, written, offset, used, failure, baseline;
    CHECK(body != NULL); memset(body, 0xc7, 12000); p.body = span(body, 12000);
    CHECK(xtp2_measure(&p, NULL, &size) == XTP2_OK);
    wire = (unsigned char *)malloc(size); CHECK(wire != NULL);
    CHECK(xtp2_encode(&p, NULL, wire, size, &written) == XTP2_OK);
    for (failure = 1; failure <= 16; ++failure) {
        memset(&t, 0, sizeof(t)); t.fail_at = failure;
        xtp2_config_init(&c); c.realloc_fn = tracked_realloc; c.allocator_user = &t;
        r = xtp2_create(&c, &rx);
        if (failure == 1) { CHECK(r == XTP2_E_NOMEM && rx == NULL); }
        else {
            CHECK(r == XTP2_OK);
            offset = 0;
            while (offset < size) {
                size_t n = size - offset; if (n > 17) n = 17;
                r = xtp2_feed(rx, wire + offset, n, &used, &m);
                CHECK(used <= n); offset += used;
                if (r < 0) break;
                CHECK(used == n && r == (offset == size ? XTP2_MESSAGE : XTP2_MORE));
            }
            if (r < 0) {
                CHECK(r == XTP2_E_NOMEM && zeroed(&m, sizeof(m)));
                CHECK(xtp2_feed(rx, NULL, 0, &used, &m) == r && used == 0);
                CHECK(xtp2_finish(rx) == r);
            } else {
                check_view(&m, wire, size);
                CHECK(xtp2_finish(rx) == XTP2_OK);
            }
            CHECK(t.blocks == 1);
            xtp2_reset(rx); t.fail_at = 0;
            CHECK(xtp2_feed(rx, binary, sizeof(binary), &used, &m) == XTP2_MESSAGE);
            check_view(&m, binary, sizeof(binary));
            xtp2_destroy(rx);
        }
        CHECK(t.bytes == 0 && t.blocks == 0);
    }
    memset(&t, 0, sizeof(t));
    xtp2_config_init(&c); c.realloc_fn = tracked_realloc; c.allocator_user = &t;
    c.retain_buffer = 64;
    CHECK(xtp2_create(&c, &rx) == XTP2_OK); baseline = t.bytes;
    memcpy(header, empty, 32); put32(header + 4, 1048576); put32(header + 24, 1048544);
    CHECK(xtp2_feed(rx, header, 32, &used, &m) == XTP2_MORE && used == 32);
    CHECK(t.calls == 1 && t.bytes == baseline); /* No eager 1 MiB allocation. */
    CHECK(xtp2_finish(rx) == XTP2_E_TRUNCATED);
    xtp2_reset(rx);
    CHECK(xtp2_feed(rx, binary, sizeof(binary), &used, &m) == XTP2_MESSAGE);
    CHECK(t.blocks == 2);
    CHECK(xtp2_feed(rx, NULL, 0, &used, &m) == XTP2_MORE && t.blocks == 2);
    CHECK(xtp2_feed(rx, wire, size, &used, &m) == XTP2_MESSAGE);
    check_view(&m, wire, size);
    CHECK(t.bytes == baseline + size);
    CHECK(xtp2_feed(rx, NULL, 0, &used, &m) == XTP2_MORE && t.bytes == baseline);
    xtp2_destroy(rx); CHECK(t.blocks == 0 && t.bytes == 0);
    memset(&t, 0, sizeof(t)); c.retain_buffer = 0;
    CHECK(xtp2_create(&c, &rx) == XTP2_OK);
    c.limits.max_packet = 32; /* create copied the configuration. */
    CHECK(xtp2_feed(rx, binary, sizeof(binary), &used, &m) == XTP2_MESSAGE);
    CHECK(xtp2_feed(rx, NULL, 0, &used, &m) == XTP2_MORE && t.blocks == 1);
    xtp2_destroy(rx); CHECK(t.blocks == 0);
    free(wire); free(body);
}

static uint32_t random_state = UINT32_C(0x139a94db);
static uint32_t random32(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static void test_random(void)
{
    unsigned char data[256], wire[2048];
    xtp2_packet p;
    xtp2_param params[8], param;
    xtp2_message m;
    xtp2_processor *rx;
    xtp2_iter it;
    size_t iteration, i, written, used, offset;
    for (i = 0; i < sizeof(data); ++i) data[i] = (unsigned char)i;
    CHECK(xtp2_create(NULL, &rx) == XTP2_OK);
    for (iteration = 0; iteration < 4000; ++iteration) {
        memset(&p, 0, sizeof(p));
        p.id = ((uint64_t)random32() << 32) | random32();
        p.status = -(int32_t)(random32() & INT32_MAX);
        p.flags = (uint16_t)random32(); p.type = (uint16_t)(1 + random32() % 4);
        p.command = span(data, random32() % 32); p.body = span(data, random32() % 256);
        p.params = params; p.param_count = random32() % 9;
        for (i = 0; i < p.param_count; ++i) {
            params[i].key = span(data + 4, random32() % 32);
            params[i].value = span(data + 40, random32() % 64);
        }
        CHECK(xtp2_encode(&p, NULL, wire, sizeof(wire), &written) == XTP2_OK);
        CHECK(xtp2_decode(wire, written, NULL, &m) == XTP2_OK);
        CHECK(m.id == p.id && m.type == p.type && m.flags == p.flags && m.status == p.status);
        CHECK(same(m.command, p.command) && same(m.body, p.body));
        it = xtp2_params(&m);
        for (i = 0; i < p.param_count; ++i) {
            CHECK(xtp2_param_next(&it, &param));
            CHECK(same(param.key, params[i].key) && same(param.value, params[i].value));
        }
        CHECK(!xtp2_param_next(&it, &param));
        offset = 0;
        while (offset < written) {
            size_t n = 1 + random32() % 97;
            xtp2_result r;
            if (n > written - offset) n = written - offset;
            r = xtp2_feed(rx, wire + offset, n, &used, &m);
            CHECK(used == n);
            offset += used;
            CHECK(r == (offset == written ? XTP2_MESSAGE : XTP2_MORE));
        }
        check_view(&m, wire, written);
    }
    CHECK(xtp2_finish(rx) == XTP2_OK);
    xtp2_destroy(rx);
}

static void test_wire_maxima(void)
{
    xtp2_config c;
    xtp2_packet p;
    xtp2_param *params = (xtp2_param *)calloc(65535, sizeof(*params));
    unsigned char *wire, *field = (unsigned char *)malloc(65535);
    xtp2_processor *rx;
    xtp2_message m;
    size_t size, written, used, i;
    CHECK(params && field);
    memset(field, 0x73, 65535);
    memset(&p, 0, sizeof(p)); p.type = XTP2_EVENT;
    xtp2_config_init(&c); c.limits.max_params = 65535;
    p.params = params; p.param_count = 65535;
    CHECK(xtp2_measure(&p, NULL, &size) == XTP2_E_LIMIT);
    CHECK(xtp2_measure(&p, &c.limits, &size) == XTP2_OK && size == 32 + 4 * 65535);
    wire = (unsigned char *)malloc(size); CHECK(wire != NULL);
    CHECK(xtp2_encode(&p, &c.limits, wire, size, &written) == XTP2_OK);
    CHECK(xtp2_decode(wire, written, &c.limits, &m) == XTP2_OK && m.param_count == 65535);
    CHECK(xtp2_create(&c, &rx) == XTP2_OK);
    CHECK(xtp2_feed(rx, wire, 32, &used, &m) == XTP2_MORE);
    CHECK(xtp2_feed(rx, wire + 32, written - 32, &used, &m) == XTP2_MESSAGE);
    xtp2_destroy(rx);
    /* Every u16 length fits; their aggregate exceeds u32. */
    for (i = 0; i < p.param_count; ++i) {
        params[i].key = span(field, 65535); params[i].value = span(field, 65535);
        memset(wire + 32 + i * 4, 0xff, 4);
    }
    c.limits.max_packet = UINT32_MAX; c.limits.max_body = UINT32_MAX;
    CHECK(xtp2_measure(&p, &c.limits, &size) == XTP2_E_LENGTH && size == 0);
    put32(wire + 4, UINT32_MAX);
    CHECK(xtp2_create(&c, &rx) == XTP2_OK);
    CHECK(xtp2_feed(rx, wire, written, &used, &m) == XTP2_E_LENGTH && used == written);
    xtp2_destroy(rx); free(wire);
    /* Boundary-sized command/key/value fields are valid, not truncated. */
    p.param_count = 1; p.command = span(field, 65535);
    xtp2_config_init(&c);
    CHECK(xtp2_measure(&p, &c.limits, &size) == XTP2_OK);
    wire = (unsigned char *)malloc(size); CHECK(wire != NULL);
    CHECK(xtp2_encode(&p, &c.limits, wire, size, &written) == XTP2_OK);
    CHECK(xtp2_decode(wire, size, &c.limits, &m) == XTP2_OK && m.command.size == 65535);
    free(wire); free(field); free(params);
}

static void test_default_maximum(void)
{
    xtp2_config c;
    xtp2_packet p;
    xtp2_processor *rx;
    xtp2_message m;
    unsigned char *body, *wire;
    size_t size, written, offset = 0, used;
    xtp2_config_init(&c);
    body = (unsigned char *)malloc(c.limits.max_body);
    wire = (unsigned char *)malloc(c.limits.max_packet);
    CHECK(body && wire);
    memset(body, 0x6b, c.limits.max_body);
    memset(&p, 0, sizeof(p)); p.type = XTP2_PUSH;
    p.body = span(body, c.limits.max_packet - 32);
    CHECK(xtp2_measure(&p, NULL, &size) == XTP2_OK && size == 1048576);
    CHECK(xtp2_encode(&p, NULL, wire, size, &written) == XTP2_OK && written == size);
    CHECK(xtp2_decode(wire, size, NULL, &m) == XTP2_OK && same(m.body, p.body));
    CHECK(xtp2_create(NULL, &rx) == XTP2_OK);
    while (offset < size) {
        size_t n = size - offset;
        xtp2_result r;
        if (n > 1009) n = 1009; /* Crosses header and common TLS record sizes. */
        r = xtp2_feed(rx, wire + offset, n, &used, &m);
        CHECK(used == n); offset += used;
        CHECK(r == (offset == size ? XTP2_MESSAGE : XTP2_MORE));
    }
    CHECK(same(m.body, p.body) && m.packet.size == size);
    CHECK(xtp2_finish(rx) == XTP2_OK); xtp2_destroy(rx);
    ++p.body.size;
    CHECK(xtp2_measure(&p, NULL, &written) == XTP2_E_LIMIT && written == 0);
    /* max_body alone does not bypass the packet's total-size limit. */
    p.body.size = c.limits.max_body;
    CHECK(xtp2_measure(&p, NULL, &written) == XTP2_E_LIMIT && written == 0);
    free(wire); free(body);
}

#ifdef XTP2_TEST_LEGACY
static void test_legacy(void)
{
    XTP_ParamPair legacy[3] = {
        { "name", "alice", 4, 5 }, { "name", "second", 4, 6 }, { "", "", 0, 0 }
    };
    xtp2_param params[3];
    xtp2_packet p;
    xtp2_message m;
    unsigned char out[256], *old;
    size_t size, written, i;
    old = procBuildRequest("echo", legacy, 3, &size);
    CHECK(old != NULL);
    memset(&p, 0, sizeof(p)); p.type = XTP2_REQUEST; p.id = 1;
    p.command = xtp2_text("echo"); p.params = params; p.param_count = 3;
    for (i = 0; i < 3; ++i) {
        params[i].key = span(legacy[i].sKey, legacy[i].iKeySize);
        params[i].value = span(legacy[i].sValue, legacy[i].iValueSize);
    }
    CHECK(xtp2_encode(&p, NULL, out, sizeof(out), &written) == XTP2_OK);
    CHECK(written == size && memcmp(out, old, size) == 0);
    CHECK(xtp2_decode(old, size, NULL, &m) == XTP2_OK);
    CHECK(same(m.command, p.command) && m.param_count == 3 && xtp2_wants_reply(&m));
    free(old);
}
#endif

/* Optional binary corpus output is a test artifact, never library state. */
static void seed(const char *dir, const char *name, const void *data, size_t size)
{
    char path[1024];
    FILE *file;
    int n = snprintf(path, sizeof(path), "%s/%s", dir, name);
    CHECK(n > 0 && (size_t)n < sizeof(path));
    file = fopen(path, "wb"); CHECK(file != NULL);
    CHECK(fwrite(data, 1, size, file) == size);
    CHECK(fclose(file) == 0);
}

int main(int argc, char **argv)
{
    test_golden(); test_stream(); test_invalid(); test_limits();
    test_allocator(); test_random(); test_wire_maxima(); test_default_maximum();
#ifdef XTP2_TEST_LEGACY
    test_legacy();
#endif
    if (argc == 2) {
        seed(argv[1], "ping", ping, sizeof(ping));
        seed(argv[1], "binary", binary, sizeof(binary));
        seed(argv[1], "empty", empty, sizeof(empty));
    }
    printf("XTP2: %zu checks passed", checks);
#ifdef XTP2_TEST_LEGACY
    printf(" (including historical XTP2 builder)");
#endif
    puts("");
    return 0;
}
