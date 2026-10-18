/* Executed through the public SDK in BOTH outer and nested TCC.
 * No copied implementation: all crypto/transport/engine calls resolve to xs.
 * Only synthetic keys, ephemeral loopback UDP ports and a private test Engine.
 * Retain CLOSED handles across Engine destruction to check the ABI/lifetime. */
typedef struct xs_rudp_sdk_test xs_rudp_sdk_test;
typedef struct xs_rudp_sdk_side { xs_rudp_sdk_test* Test; uint32_t Id; } xs_rudp_sdk_side;
struct xs_rudp_sdk_test {
    xnetengine* Engine;
    xrudpendpoint* Endpoint[2];
    xrudppeer* Peer[2];
    xrudpendpointinfo Info[2];
    xs_rudp_sdk_side Side[2];
    xatomic32 Ready, Confirmed, ReadDone, Closed, PunchDone;
    unsigned char Private[2][32], Public[2][32];
    size_t Read[2];
    uint64_t Punch;
    xnetfamily Family;
};
static const char xs_rudp_sdk_payload[] = "xs xrudp";
static void xs_rudp_sdk_wait(xs_rudp_sdk_test* t, xatomic32* value, uint32_t bits)
{
    double deadline = xrtTimer() + 8;
    (void)t;
    while ((xrtAtomic32Load(value, XMEMORY_ACQUIRE) & bits) != bits) {
        REQUIRE(xrtTimer() < deadline);
        xrtThreadYield();
    }
}
static void xs_rudp_sdk_open(xrudpendpoint* e, void* context)
{
    xs_rudp_sdk_side* s = context;
    REQUIRE(xrtRudpEndpointInfo(e, &s->Test->Info[s->Id]) == XRUDP_OK);
    xrtAtomic32FetchOr(&s->Test->Ready, 1u << s->Id, XMEMORY_RELEASE);
}
static void xs_rudp_sdk_punched(xrudpendpoint* e, uint64_t id,
    const xrudppunchstate* state, const xerror* error, void* context)
{
    xs_rudp_sdk_test* t = context;
    (void)e;
    REQUIRE(id == t->Punch && state->Reason == XRUDP_CANCELLED && error == NULL);
    REQUIRE(xrtAtomic32FetchAdd(&t->PunchDone, 1, XMEMORY_ACQ_REL) == 0);
}
static void xs_rudp_sdk_confirmed(xrudpendpoint* e, xrudppeer* p, void* context)
{
    xs_rudp_sdk_side* s = context;
    xs_rudp_sdk_test* t = s->Test;
    xrudphandshakeinfo info;
    xnetaddr remote;
    size_t written;
    (void)e;
    REQUIRE(xrtRudpPeerInfo(p, &info, &remote) == XRUDP_OK);
    REQUIRE(xrtNetAddrEqual(&remote, &t->Info[1 - s->Id].Local));
    if (s->Id == 0) {
        t->Peer[0] = xrtRudpPeerRef(p);
        REQUIRE(xrtRudpEndpointPunchCancel(t->Endpoint[0], t->Punch) == XRUDP_OK);
    }
    REQUIRE(xrtRudpPeerWrite(p, xs_rudp_sdk_payload, sizeof(xs_rudp_sdk_payload) - 1, &written) == XRUDP_OK);
    REQUIRE(written == sizeof(xs_rudp_sdk_payload) - 1 && xrtRudpPeerFinish(p) == XRUDP_OK);
    xrtAtomic32FetchOr(&t->Confirmed, 1u << s->Id, XMEMORY_RELEASE);
}
static void xs_rudp_sdk_changed(xrudpendpoint* e, xrudppeer* p, uint32_t events, void* context)
{
    xs_rudp_sdk_side* s = context;
    xs_rudp_sdk_test* t = s->Test;
    unsigned int step;
    (void)e;
    if (!(events & (XRUDP_PEER_READ_READY | XRUDP_PEER_READ_EOF))) return;
    for (step = 0; step < 8; ++step) {
        unsigned char bytes[32];
        size_t n;
        xrudpresult result = xrtRudpPeerRead(p, bytes, sizeof(bytes), &n);
        if (result == XRUDP_BACKPRESSURE) break;
        REQUIRE(result == XRUDP_OK || result == XRUDP_DONE);
        REQUIRE(t->Read[s->Id] + n <= sizeof(xs_rudp_sdk_payload) - 1);
        REQUIRE(!memcmp(bytes, xs_rudp_sdk_payload + t->Read[s->Id], n));
        t->Read[s->Id] += n;
        if (result == XRUDP_DONE) {
            REQUIRE(t->Read[s->Id] == sizeof(xs_rudp_sdk_payload) - 1);
            xrtAtomic32FetchOr(&t->ReadDone, 1u << s->Id, XMEMORY_RELEASE);
            break;
        }
    }
}
static void xs_rudp_sdk_closed(xrudpendpoint* e, xrudpresult reason,
    const xerror* error, void* context)
{
    xs_rudp_sdk_side* s = context;
    (void)e;
    REQUIRE(reason == XRUDP_CANCELLED && error == NULL);
    xrtAtomic32FetchOr(&s->Test->Closed, 1u << s->Id, XMEMORY_RELEASE);
}
static void xs_rudp_sdk_bind(xnetworker* worker, ptr context)
{
    xs_rudp_sdk_side* s = context;
    xs_rudp_sdk_test* t = s->Test;
    xrudpendpointconfig config;
    xrudpendpointevents events = {0};
    xnetaddr local;
    xrtRudpEndpointConfigInit(&config);
    config.StaticPrivate = t->Private[s->Id];
    config.PeerPublic = t->Public[1 - s->Id];
    config.Listen = s->Id == 0;
    config.MaxPeers = config.MaxHalfOpen = 2;
    REQUIRE(xrtNetAddrLoopback(&local, t->Family, 0));
    events.Open = xs_rudp_sdk_open;
    events.Confirmed = xs_rudp_sdk_confirmed;
    events.Close = xs_rudp_sdk_closed;
    events.Changed = xs_rudp_sdk_changed;
    REQUIRE(xrtRudpEndpointBind(worker, &local, &config, &events, s, &t->Endpoint[s->Id]) == XRUDP_OK);
}
static void xs_rudp_sdk_connect(xnetworker* worker, ptr context)
{
    xs_rudp_sdk_test* t = context;
    xrudpconnectconfig config = {t->Info[0].Public, t->Info[0].Scope, NULL};
    (void)worker;
    REQUIRE(xrtRudpEndpointPunch(t->Endpoint[0], &t->Info[1].Local, 1, 1000,
                               xs_rudp_sdk_punched, t, &t->Punch) == XRUDP_OK);
    REQUIRE(xrtRudpEndpointConnect(t->Endpoint[1], &t->Info[0].Local, &config, &t->Peer[1]) == XRUDP_OK);
}
static void CheckXrudpNative(xnetfamily family)
{
    xs_rudp_sdk_test t;
    xnetengineconfig config;
    uint32_t i, j;
    memset(&t, 0, sizeof(t));
    t.Family = family;
    for (i = 0; i < 2; ++i) {
        t.Side[i].Test = &t; t.Side[i].Id = i;
        for (j = 0; j < 32; ++j) t.Private[i][j] = (unsigned char)(31 + i * 67 + j);
        REQUIRE(xrtX25519Public(t.Private[i], t.Public[i]));
    }
    xrtNetEngineConfigInit(&config);
    config.Workers = 1;
    t.Engine = xrtNetEngineCreate(&config);
    REQUIRE(t.Engine != NULL && xrtNetEngineStart(t.Engine));
    for (i = 0; i < 2; ++i) REQUIRE(xrtNetEnginePost(t.Engine, 0, xs_rudp_sdk_bind, &t.Side[i]));
    xs_rudp_sdk_wait(&t, &t.Ready, 3);
    REQUIRE(xrtNetEnginePost(t.Engine, 0, xs_rudp_sdk_connect, &t));
    xs_rudp_sdk_wait(&t, &t.Confirmed, 3);
    xs_rudp_sdk_wait(&t, &t.ReadDone, 3);
    xs_rudp_sdk_wait(&t, &t.PunchDone, 1);
    for (i = 0; i < 2; ++i) REQUIRE(xrtRudpEndpointClose(t.Endpoint[i]) == XRUDP_OK);
    xs_rudp_sdk_wait(&t, &t.Closed, 3);
    REQUIRE(xrtNetEngineStop(t.Engine) && xrtNetEngineDestroy(t.Engine));
    for (i = 0; i < 2; ++i) {
        xrudpendpointinfo info;
        REQUIRE(xrtRudpEndpointInfo(t.Endpoint[i], &info) == XRUDP_OK && !info.Peers && !info.Punches);
        REQUIRE(xrtRudpEndpointPunchCancel(t.Endpoint[i], 1) == XRUDP_CLOSED);
        xrtRudpPeerDestroy(t.Peer[i]);
        xrtRudpEndpointDestroy(t.Endpoint[i]);
    }
    printf("[sdk] xrudp native family=%u Noise+reliable+FIN+cancel+closed-handles ok\n", (unsigned)family);
}
