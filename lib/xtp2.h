/*
 * xtp2.h -- standalone XTP2 framing for ordered byte streams (C99 / C++).
 *
 * Define XTP2_IMPLEMENTATION before including this file in exactly one
 * translation unit. Other units include it without that definition.
 * No xs, socket, TLS, thread or event-loop dependency.
 *
 * Wire format (little-endian, no padding):
 *   0: "xtp\2"   4: total:u32   8: id:u64       16: flags:u16
 *  18: type:u16 20: command:u16 22: params:u16   24: body:u32
 *  28: status:i32 (two's complement)
 * Then: [key:u16,value:u16]*params, command, [key,value]*params, body.
 * Flags and status are passed through. Empty/binary fields and duplicate
 * keys are valid. A request with id == 0 does not require a reply.
 *
 * Usage:
 *   #define XTP2_IMPLEMENTATION
 *   #include "xtp2.h"
 *
 *   xtp2_processor *rx;
 *   if (xtp2_create(NULL, &rx) != XTP2_OK) { ... }
 *   // For each TCP read or TLS plaintext read:
 *   while (size) {
 *       size_t used;
 *       xtp2_message msg;
 *       xtp2_result r = xtp2_feed(rx, bytes, size, &used, &msg);
 *       bytes += used; size -= used;
 *       if (r < 0) { ... break; }
 *       if (r == XTP2_MESSAGE) { ... use/copy msg before next feed ... }
 *       if (r == XTP2_MORE) break;
 *   }
 *   // On read EOF, check xtp2_finish(rx); then xtp2_destroy(rx).
 *
 * All spans are explicit byte lengths, never implicitly strlen'd.
 * Nonempty spans/inputs must point to that many readable bytes. Output
 * objects must be distinct from inputs and each other. Encoding sources
 * must not overlap the destination; feed input must not alias rx storage.
 * No concurrent operations on one processor; separate processors are
 * independent (a shared custom allocator must itself be thread-safe).
 */
#ifndef XTP2_H_INCLUDED
#define XTP2_H_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XTP2_HEADER_SIZE 32u
#define XTP2_WIRE_VERSION 2u

enum {
    XTP2_REQUEST = 1, XTP2_RESPONSE = 2, XTP2_PUSH = 3, XTP2_EVENT = 4
};

typedef enum {
    XTP2_OK = 0, XTP2_MORE = 1, XTP2_MESSAGE = 2,
    XTP2_E_ARGUMENT = -1, XTP2_E_MAGIC = -2,
    XTP2_E_VERSION = -3, XTP2_E_TYPE = -4,
    XTP2_E_LENGTH = -5, XTP2_E_LIMIT = -6,
    XTP2_E_NOMEM = -7, XTP2_E_CAPACITY = -8,
    XTP2_E_TRUNCATED = -9, XTP2_E_STATE = -10
} xtp2_result;

typedef struct {
    const void *data;
    size_t size;
} xtp2_span;

typedef struct {
    xtp2_span key, value;
} xtp2_param;

/* Borrowed encoding inputs: stable until measure/encode returns. */
typedef struct {
    uint64_t id;
    int32_t status;
    uint16_t type, flags;
    xtp2_span command, body;
    const xtp2_param *params;
    size_t param_count;
} xtp2_packet;

/* Borrowed view. feed: valid until the next feed/finish/reset/destroy.
 * decode: valid until the caller changes/frees the input packet.
 * To keep a message, copy packet bytes and decode that copy. */
typedef struct {
    uint64_t id;
    int32_t status;
    uint16_t type, flags, param_count;
    xtp2_span command, body, packet;
} xtp2_message;

typedef struct {
    size_t max_packet, max_body;
    uint32_t max_command, max_params, max_key, max_value;
} xtp2_limits;

/* realloc semantics, including object alignment and preserving ptr on
 * allocation failure. size == 0 must free ptr and return NULL.
 * The allocator context must outlive the processor. */
typedef void *(*xtp2_realloc_fn)(void *user, void *ptr, size_t size);

typedef struct {
    xtp2_limits limits;
    size_t retain_buffer;
    xtp2_realloc_fn realloc_fn;
    void *allocator_user;
} xtp2_config;

typedef struct xtp2_processor xtp2_processor;

/* Initialize only with xtp2_params; do not modify the cursor's fields. */
typedef struct {
    const unsigned char *info, *data;
    size_t remaining;
} xtp2_iter;

/* Defaults: packet/body 1 MiB, 256 params, u16 field limits, 64 KiB cache.
 * NULL config/limits means defaults. Non-NULL fields are literal limits:
 * zero allows only an empty field, never "unlimited".
 * max_packet must be in [32, UINT32_MAX]; other limits must fit the wire. */
void xtp2_config_init(xtp2_config *config);
xtp2_result xtp2_create(const xtp2_config *config, xtp2_processor **out);
void xtp2_destroy(xtp2_processor *processor); /* NULL is harmless. */
void xtp2_reset(xtp2_processor *processor);   /* Free buffers; keep config. */

/* At most one message per call. MESSAGE stops exactly at its frame end;
 * MORE consumes all size bytes. Unconsumed suffixes remain with the caller.
 * On failure, consumed is the copied prefix length and message is zeroed.
 * Parse/allocation errors stick until reset; argument errors do not advance.
 * size == 0 permits data == NULL; it is not EOF.
 * Required output pointers must be non-NULL. */
xtp2_result xtp2_feed(xtp2_processor *processor, const void *data, size_t size,
                      size_t *consumed, xtp2_message *message);

/* Receive EOF: OK at a frame boundary, TRUNCATED for an unfinished frame,
 * or the sticky parse error. Releases the buffer and invalidates views.
 * Repeated finish is stable; feed after clean EOF returns E_STATE. */
xtp2_result xtp2_finish(xtp2_processor *processor);

/* Exactly one frame, no allocations/copies. Short input is TRUNCATED;
 * trailing data is LENGTH. Errors zero the message. */
xtp2_result xtp2_decode(const void *data, size_t size,
                        const xtp2_limits *limits, xtp2_message *message);
xtp2_result xtp2_measure(const xtp2_packet *packet,
                         const xtp2_limits *limits, size_t *size);
/* Errors set written = 0 and leave output bytes untouched. The caller
 * owns output and handles send queues, partial writes and send lifetime. */
xtp2_result xtp2_encode(const xtp2_packet *packet, const xtp2_limits *limits,
                        void *output, size_t capacity, size_t *written);

/* Only use live library-produced message views. Iteration is O(n).
 * find returns the first exact binary key match (duplicate keys allowed).
 * No match/end/invalid arguments zero the supplied output, if non-NULL. */
xtp2_iter xtp2_params(const xtp2_message *message);
bool xtp2_param_next(xtp2_iter *iter, xtp2_param *param);
bool xtp2_find(const xtp2_message *message, xtp2_span key, xtp2_span *value);
const char *xtp2_error_string(xtp2_result result);

static inline xtp2_span xtp2_text(const char *text)
{
    xtp2_span span;
    span.data = text;
    span.size = text ? strlen(text) : 0;
    return span;
}

static inline bool xtp2_wants_reply(const xtp2_message *message)
{
    return message && message->type == XTP2_REQUEST && message->id != 0;
}

#ifdef __cplusplus
}
#endif
#endif /* XTP2_H_INCLUDED */

#if defined(XTP2_IMPLEMENTATION) && !defined(XTP2_IMPLEMENTED)
#define XTP2_IMPLEMENTED
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t size, table_end, body;
    uint16_t command, count;
} xtp2_i_frame;

enum {
    XTP2_I_HEADER, XTP2_I_TABLE, XTP2_I_PAYLOAD,
    XTP2_I_READY, XTP2_I_FAILED, XTP2_I_EOF
};

struct xtp2_processor {
    xtp2_config config;
    unsigned char header[XTP2_HEADER_SIZE];
    unsigned char *buffer;
    size_t used, capacity;
    xtp2_i_frame frame;
    int state;
    xtp2_result error;
};

static uint16_t xtp2_i_u16(const unsigned char *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t xtp2_i_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void xtp2_i_put16(unsigned char *p, uint16_t n)
{
    p[0] = (unsigned char)n;
    p[1] = (unsigned char)(n >> 8);
}

static void xtp2_i_put32(unsigned char *p, uint32_t n)
{
    p[0] = (unsigned char)n;
    p[1] = (unsigned char)(n >> 8);
    p[2] = (unsigned char)(n >> 16);
    p[3] = (unsigned char)(n >> 24);
}

void xtp2_config_init(xtp2_config *config)
{
    if (!config) return;
    config->limits.max_packet = 1048576u;
    config->limits.max_body = 1048576u;
    config->limits.max_command = UINT16_MAX;
    config->limits.max_params = 256u;
    config->limits.max_key = UINT16_MAX;
    config->limits.max_value = UINT16_MAX;
    config->retain_buffer = 65536u;
    config->realloc_fn = NULL;
    config->allocator_user = NULL;
}

static xtp2_result xtp2_i_limits(const xtp2_limits *input, xtp2_limits *out)
{
    if (input) {
        *out = *input;
    } else {
        xtp2_config config;
        xtp2_config_init(&config);
        *out = config.limits;
    }
    if (out->max_packet < XTP2_HEADER_SIZE ||
        out->max_packet > UINT32_MAX || out->max_body > UINT32_MAX ||
        out->max_command > UINT16_MAX || out->max_params > UINT16_MAX ||
        out->max_key > UINT16_MAX || out->max_value > UINT16_MAX)
        return XTP2_E_ARGUMENT;
    return XTP2_OK;
}

/* Called only with all 32 header bytes available. Validate cheap bounds
 * before allocating storage or touching the parameter table. */
static xtp2_result xtp2_i_header(const unsigned char *p,
                                const xtp2_limits *limits, xtp2_i_frame *f)
{
    uint32_t size, body;
    uint16_t count, command, type;
    uint64_t minimum;
    if (p[0] != 'x' || p[1] != 't' || p[2] != 'p') return XTP2_E_MAGIC;
    if (p[3] != XTP2_WIRE_VERSION) return XTP2_E_VERSION;
    type = xtp2_i_u16(p + 18);
    if (type < XTP2_REQUEST || type > XTP2_EVENT) return XTP2_E_TYPE;
    size = xtp2_i_u32(p + 4);
    body = xtp2_i_u32(p + 24);
    command = xtp2_i_u16(p + 20);
    count = xtp2_i_u16(p + 22);
    minimum = (uint64_t)XTP2_HEADER_SIZE + (uint64_t)count * 4 +
              command + body;
    if (minimum > size) return XTP2_E_LENGTH;
    if (size > limits->max_packet || body > limits->max_body ||
        command > limits->max_command || count > limits->max_params)
        return XTP2_E_LIMIT;
    f->size = (size_t)size;
    f->body = (size_t)body;
    f->command = command;
    f->count = count;
    f->table_end = XTP2_HEADER_SIZE + (size_t)count * 4u;
    return XTP2_OK;
}

/* Called only after the complete table has arrived, never on a half-table.
 * Wire field widths bound this sum well below UINT64_MAX. */
static xtp2_result xtp2_i_table(const unsigned char *p,
                               const xtp2_limits *limits, const xtp2_i_frame *f)
{
    size_t i;
    uint64_t size = (uint64_t)f->table_end + f->command + f->body;
    for (i = 0; i < f->count; ++i) {
        uint16_t key = xtp2_i_u16(p + XTP2_HEADER_SIZE + i * 4);
        uint16_t value = xtp2_i_u16(p + XTP2_HEADER_SIZE + i * 4 + 2);
        if (key > limits->max_key || value > limits->max_value)
            return XTP2_E_LIMIT;
        size += (uint64_t)key + value;
    }
    return size == f->size ? XTP2_OK : XTP2_E_LENGTH;
}

static void xtp2_i_message(const unsigned char *p, const xtp2_i_frame *f,
                           xtp2_message *m)
{
    uint32_t status = xtp2_i_u32(p + 28);
    m->id = (uint64_t)xtp2_i_u32(p + 8) | ((uint64_t)xtp2_i_u32(p + 12) << 32);
    /* Avoid implementation-defined unsigned-to-signed overflow. */
    m->status = status <= INT32_MAX ? (int32_t)status :
                -(int32_t)(UINT32_MAX - status) - 1;
    m->type = xtp2_i_u16(p + 18);
    m->flags = xtp2_i_u16(p + 16);
    m->param_count = f->count;
    m->command.data = p + f->table_end;
    m->command.size = f->command;
    m->body.data = p + f->size - f->body;
    m->body.size = f->body;
    m->packet.data = p;
    m->packet.size = f->size;
}

xtp2_result xtp2_decode(const void *data, size_t size,
                        const xtp2_limits *limits, xtp2_message *message)
{
    xtp2_limits bound;
    xtp2_i_frame frame;
    xtp2_result r;
    const unsigned char *p = (const unsigned char *)data;
    if (message) memset(message, 0, sizeof(*message));
    if (!message || (!data && size)) return XTP2_E_ARGUMENT;
    r = xtp2_i_limits(limits, &bound);
    if (r != XTP2_OK) return r;
    if (size < XTP2_HEADER_SIZE) return XTP2_E_TRUNCATED;
    r = xtp2_i_header(p, &bound, &frame);
    if (r != XTP2_OK) return r;
    if (size < frame.size) return XTP2_E_TRUNCATED;
    if (size > frame.size) return XTP2_E_LENGTH;
    r = xtp2_i_table(p, &bound, &frame);
    if (r != XTP2_OK) return r;
    xtp2_i_message(p, &frame, message);
    return XTP2_OK;
}

static bool xtp2_i_span_ok(xtp2_span span)
{
    return span.data != NULL || span.size == 0;
}

xtp2_result xtp2_measure(const xtp2_packet *packet,
                         const xtp2_limits *limits, size_t *size)
{
    xtp2_limits bound;
    xtp2_result r;
    uint64_t total;
    size_t i;
    if (size) *size = 0;
    if (!size || !packet) return XTP2_E_ARGUMENT;
    r = xtp2_i_limits(limits, &bound);
    if (r != XTP2_OK) return r;
    if (packet->type < XTP2_REQUEST || packet->type > XTP2_EVENT)
        return XTP2_E_TYPE;
    if (packet->param_count > UINT16_MAX ||
        packet->command.size > UINT16_MAX || packet->body.size > UINT32_MAX)
        return XTP2_E_LENGTH;
    if (packet->param_count > bound.max_params ||
        packet->command.size > bound.max_command ||
        packet->body.size > bound.max_body)
        return XTP2_E_LIMIT;
    if ((packet->param_count && !packet->params) ||
        !xtp2_i_span_ok(packet->command) || !xtp2_i_span_ok(packet->body))
        return XTP2_E_ARGUMENT;
    total = (uint64_t)XTP2_HEADER_SIZE + (uint64_t)packet->param_count * 4 +
            packet->command.size + packet->body.size;
    for (i = 0; i < packet->param_count; ++i) {
        const xtp2_param *param = packet->params + i;
        if (param->key.size > UINT16_MAX || param->value.size > UINT16_MAX)
            return XTP2_E_LENGTH;
        if (param->key.size > bound.max_key || param->value.size > bound.max_value)
            return XTP2_E_LIMIT;
        if (!xtp2_i_span_ok(param->key) || !xtp2_i_span_ok(param->value))
            return XTP2_E_ARGUMENT;
        total += (uint64_t)param->key.size + param->value.size;
    }
    if (total > UINT32_MAX || total > SIZE_MAX) return XTP2_E_LENGTH;
    if (total > bound.max_packet) return XTP2_E_LIMIT;
    *size = (size_t)total;
    return XTP2_OK;
}

/* memcpy is not called on a NULL pointer, even for a zero-length field. */
static unsigned char *xtp2_i_copy(unsigned char *p, xtp2_span span)
{
    if (span.size) memcpy(p, span.data, span.size);
    return p + span.size;
}

xtp2_result xtp2_encode(const xtp2_packet *packet, const xtp2_limits *limits,
                        void *output, size_t capacity, size_t *written)
{
    size_t size, i;
    unsigned char *p = (unsigned char *)output, *payload;
    xtp2_result r;
    if (written) *written = 0;
    if (!written || !output) return XTP2_E_ARGUMENT;
    r = xtp2_measure(packet, limits, &size);
    if (r != XTP2_OK) return r;
    if (capacity < size) return XTP2_E_CAPACITY;
    /* No operation below this point can fail for valid, stable inputs. */
    memcpy(p, "xtp\2", 4);
    xtp2_i_put32(p + 4, (uint32_t)size);
    xtp2_i_put32(p + 8, (uint32_t)packet->id);
    xtp2_i_put32(p + 12, (uint32_t)(packet->id >> 32));
    xtp2_i_put16(p + 16, packet->flags);
    xtp2_i_put16(p + 18, packet->type);
    xtp2_i_put16(p + 20, (uint16_t)packet->command.size);
    xtp2_i_put16(p + 22, (uint16_t)packet->param_count);
    xtp2_i_put32(p + 24, (uint32_t)packet->body.size);
    xtp2_i_put32(p + 28, (uint32_t)packet->status);
    for (i = 0; i < packet->param_count; ++i) {
        xtp2_i_put16(p + XTP2_HEADER_SIZE + i * 4, (uint16_t)packet->params[i].key.size);
        xtp2_i_put16(p + XTP2_HEADER_SIZE + i * 4 + 2, (uint16_t)packet->params[i].value.size);
    }
    payload = xtp2_i_copy(p + XTP2_HEADER_SIZE + packet->param_count * 4, packet->command);
    for (i = 0; i < packet->param_count; ++i) {
        payload = xtp2_i_copy(payload, packet->params[i].key);
        payload = xtp2_i_copy(payload, packet->params[i].value);
    }
    xtp2_i_copy(payload, packet->body);
    *written = size;
    return XTP2_OK;
}

xtp2_iter xtp2_params(const xtp2_message *message)
{
    xtp2_iter iter = { NULL, NULL, 0 };
    if (message && message->packet.data) {
        iter.info = (const unsigned char *)message->packet.data + XTP2_HEADER_SIZE;
        iter.data = (const unsigned char *)message->command.data + message->command.size;
        iter.remaining = message->param_count;
    }
    return iter;
}

bool xtp2_param_next(xtp2_iter *iter, xtp2_param *param)
{
    if (param) memset(param, 0, sizeof(*param));
    if (!iter || !param || !iter->remaining) return false;
    param->key.data = iter->data;
    param->key.size = xtp2_i_u16(iter->info);
    param->value.data = iter->data + param->key.size;
    param->value.size = xtp2_i_u16(iter->info + 2);
    iter->data += param->key.size + param->value.size;
    iter->info += 4;
    --iter->remaining;
    return true;
}

bool xtp2_find(const xtp2_message *message, xtp2_span key, xtp2_span *value)
{
    xtp2_iter iter;
    xtp2_param param;
    if (value) memset(value, 0, sizeof(*value));
    if (!message || !value || !xtp2_i_span_ok(key)) return false;
    iter = xtp2_params(message);
    while (xtp2_param_next(&iter, &param)) {
        if (key.size == param.key.size &&
            (!key.size || memcmp(key.data, param.key.data, key.size) == 0)) {
            *value = param.value;
            return true;
        }
    }
    return false;
}

static void *xtp2_i_realloc(void *user, void *ptr, size_t size)
{
    (void)user;
    if (!size) { free(ptr); return NULL; }
    return realloc(ptr, size);
}

xtp2_result xtp2_create(const xtp2_config *config, xtp2_processor **out)
{
    xtp2_config c;
    xtp2_processor *p;
    xtp2_result r;
    if (!out) return XTP2_E_ARGUMENT;
    *out = NULL;
    if (config) c = *config;
    else xtp2_config_init(&c);
    r = xtp2_i_limits(&c.limits, &c.limits);
    if (r != XTP2_OK) return r;
    if (!c.realloc_fn) c.realloc_fn = xtp2_i_realloc;
    p = (xtp2_processor *)c.realloc_fn(c.allocator_user, NULL, sizeof(*p));
    if (!p) return XTP2_E_NOMEM;
    memset(p, 0, sizeof(*p));
    p->config = c;
    p->state = XTP2_I_HEADER;
    *out = p;
    return XTP2_OK;
}

static void xtp2_i_release(xtp2_processor *p)
{
    if (p->buffer) p->config.realloc_fn(p->config.allocator_user, p->buffer, 0);
    p->buffer = NULL;
    p->capacity = 0;
}

void xtp2_reset(xtp2_processor *p)
{
    if (!p) return;
    xtp2_i_release(p);
    p->used = 0;
    p->state = XTP2_I_HEADER;
    p->error = XTP2_OK;
}

void xtp2_destroy(xtp2_processor *p)
{
    if (!p) return;
    xtp2_i_release(p);
    p->config.realloc_fn(p->config.allocator_user, p, 0);
}

static xtp2_result xtp2_i_reserve(xtp2_processor *p, size_t need)
{
    size_t capacity;
    unsigned char *buffer;
    bool fresh = p->buffer == NULL;
    if (need <= p->capacity) return XTP2_OK;
    capacity = p->capacity ? p->capacity : 256u;
    if (capacity > p->frame.size) capacity = p->frame.size;
    while (capacity < need) {
        if (capacity > p->frame.size / 2) { capacity = p->frame.size; break; }
        capacity *= 2;
    }
    buffer = (unsigned char *)p->config.realloc_fn(
        p->config.allocator_user, p->buffer, capacity);
    if (!buffer) return XTP2_E_NOMEM; /* realloc retains the old block. */
    if (fresh) memcpy(buffer, p->header, XTP2_HEADER_SIZE);
    p->buffer = buffer;
    p->capacity = capacity;
    return XTP2_OK;
}

xtp2_result xtp2_feed(xtp2_processor *p, const void *data, size_t size,
                      size_t *consumed, xtp2_message *message)
{
    const unsigned char *input = (const unsigned char *)data;
    xtp2_result r;
    if (consumed) *consumed = 0;
    if (message) memset(message, 0, sizeof(*message));
    if (!p || !consumed || !message || (!data && size)) return XTP2_E_ARGUMENT;
    if (p->state == XTP2_I_FAILED) return p->error;
    if (p->state == XTP2_I_EOF) return XTP2_E_STATE;
    if (p->state == XTP2_I_READY) {
        if (p->capacity > p->config.retain_buffer) xtp2_i_release(p);
        p->used = 0;
        p->state = XTP2_I_HEADER;
    }
    for (;;) {
        size_t target = p->state == XTP2_I_HEADER ? XTP2_HEADER_SIZE :
                        p->state == XTP2_I_TABLE ? p->frame.table_end : p->frame.size;
        size_t take = target - p->used;
        if (take > size - *consumed) take = size - *consumed;
        if (take) {
            if (p->state == XTP2_I_HEADER) {
                memcpy(p->header + p->used, input + *consumed, take);
            } else {
                r = xtp2_i_reserve(p, p->used + take);
                if (r != XTP2_OK) break;
                memcpy(p->buffer + p->used, input + *consumed, take);
            }
            p->used += take;
            *consumed += take;
        }
        if (p->used < target) return XTP2_MORE;
        if (p->state == XTP2_I_HEADER) {
            r = xtp2_i_header(p->header, &p->config.limits, &p->frame);
            if (r != XTP2_OK) break;
            if (p->buffer) memcpy(p->buffer, p->header, XTP2_HEADER_SIZE);
            p->state = XTP2_I_TABLE;
        } else if (p->state == XTP2_I_TABLE) {
            r = xtp2_i_table(p->buffer ? p->buffer : p->header, &p->config.limits, &p->frame);
            if (r != XTP2_OK) break;
            p->state = XTP2_I_PAYLOAD;
        } else {
            xtp2_i_message(p->buffer ? p->buffer : p->header, &p->frame, message);
            p->state = XTP2_I_READY;
            return XTP2_MESSAGE;
        }
    }
    p->state = XTP2_I_FAILED;
    p->error = r;
    return r;
}

xtp2_result xtp2_finish(xtp2_processor *p)
{
    if (!p) return XTP2_E_ARGUMENT;
    xtp2_i_release(p);
    if (p->state == XTP2_I_FAILED) return p->error;
    if (p->state == XTP2_I_EOF) return XTP2_OK;
    if (p->state == XTP2_I_READY || !p->used) {
        p->state = XTP2_I_EOF;
        return XTP2_OK;
    }
    p->state = XTP2_I_FAILED;
    p->error = XTP2_E_TRUNCATED;
    return p->error;
}

const char *xtp2_error_string(xtp2_result result)
{
    switch (result) {
    case XTP2_OK:          return "ok";
    case XTP2_MORE:        return "more input needed";
    case XTP2_MESSAGE:     return "message ready";
    case XTP2_E_ARGUMENT:  return "invalid argument or configuration";
    case XTP2_E_MAGIC:     return "invalid XTP magic";
    case XTP2_E_VERSION:   return "unsupported XTP version";
    case XTP2_E_TYPE:      return "invalid message type";
    case XTP2_E_LENGTH:    return "inconsistent or unrepresentable length";
    case XTP2_E_LIMIT:     return "resource limit exceeded";
    case XTP2_E_NOMEM:     return "allocation failed";
    case XTP2_E_CAPACITY:  return "output buffer too small";
    case XTP2_E_TRUNCATED: return "truncated frame";
    case XTP2_E_STATE:     return "receive stream already finished";
    default:              return "unknown XTP2 result";
    }
}

#ifdef __cplusplus
}
#endif
#endif /* XTP2_IMPLEMENTATION && !XTP2_IMPLEMENTED */
