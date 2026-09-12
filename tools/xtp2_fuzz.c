/* clang -std=c99 -g -O1 -fsanitize=fuzzer,address,undefined \
 *   tools/xtp2_fuzz.c -o xtp2_fuzz
 * ./xtp2_test corpus && ./xtp2_fuzz corpus -runs=100000 -max_len=65536
 * corpus must be an existing directory. */
#define XTP2_IMPLEMENTATION
#include "../lib/xtp2.h"
#include <assert.h>
#include <stdlib.h>

static void verify(const xtp2_message *m, const xtp2_limits *limits)
{
    xtp2_param params[256], param;
    xtp2_iter it = xtp2_params(m);
    xtp2_packet packet;
    xtp2_message decoded;
    unsigned char *encoded;
    size_t count = 0, size, written;
    memset(&packet, 0, sizeof(packet));
    while (xtp2_param_next(&it, &param)) {
        assert(count < 256);
        params[count++] = param;
    }
    assert(count == m->param_count);
    packet.id = m->id; packet.status = m->status;
    packet.type = m->type; packet.flags = m->flags;
    packet.command = m->command; packet.body = m->body;
    packet.params = params; packet.param_count = count;
    assert(xtp2_measure(&packet, limits, &size) == XTP2_OK);
    assert(size == m->packet.size);
    encoded = (unsigned char *)malloc(size);
    if (!encoded) abort();
    assert(xtp2_encode(&packet, limits, encoded, size, &written) == XTP2_OK);
    assert(written == size && memcmp(encoded, m->packet.data, size) == 0);
    assert(xtp2_decode(encoded, size, limits, &decoded) == XTP2_OK);
    free(encoded);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    xtp2_config config;
    xtp2_processor *rx;
    xtp2_message m;
    xtp2_result whole, r = XTP2_MORE, end;
    size_t offset = 0, messages = 0, used;
    xtp2_config_init(&config);
    config.limits.max_packet = 65536;
    config.limits.max_body = 65536;
    whole = xtp2_decode(data, size, &config.limits, &m);
    if (whole == XTP2_OK) verify(&m, &config.limits);
    assert(xtp2_create(&config, &rx) == XTP2_OK);
    while (offset < size) {
        size_t n = 1 + data[offset] % 127;
        if (n > size - offset) n = size - offset;
        r = xtp2_feed(rx, data + offset, n, &used, &m);
        assert(used <= n);
        offset += used;
        if (r < 0) break;
        assert(used > 0);
        if (r == XTP2_MESSAGE) {
            ++messages;
            verify(&m, &config.limits);
        } else {
            assert(r == XTP2_MORE && used == n);
        }
    }
    end = xtp2_finish(rx);
    assert(xtp2_finish(rx) == end);
    if (whole == XTP2_OK) {
        assert(offset == size && messages == 1 && end == XTP2_OK);
    }
    if (r < 0) assert(end == r);
    assert(xtp2_feed(rx, NULL, 0, &used, &m) == (end < 0 ? end : XTP2_E_STATE));
    assert(used == 0);
    xtp2_reset(rx);
    assert(xtp2_finish(rx) == XTP2_OK);
    xtp2_destroy(rx);
    return 0;
}
