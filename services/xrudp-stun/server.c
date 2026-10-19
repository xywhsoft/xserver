/* Standalone bounded Binding reflector. Uses the committed XS XRT snapshot;
 * no website ABI, account credentials, TURN, redirects or third-party runtime. */
#define XRT_MODULE_NET_UDP
#define XRT_MODULE_CRC32
#define XRT_MODULE_HASH_KEYED
#define XRT_MODULE_RANDOM_SECURE
#define XRT_IMPLEMENTATION
#include <xrt.h>
#include <xrudp/stun.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef XRUDP_STUN_BUILD_XS
#define XRUDP_STUN_BUILD_XS "unknown"
#endif
#ifndef XRUDP_STUN_BUILD_XRUDP
#define XRUDP_STUN_BUILD_XRUDP "unknown"
#endif
#ifndef XRUDP_STUN_BUILD_CORE
#define XRUDP_STUN_BUILD_CORE "unknown"
#endif

#define STUN_RATE_SETS 256u
#define STUN_RATE_WAYS 4u
#define STUN_TOKEN UINT64_C(1000000)
#define STUN_ENTRY_IDLE UINT64_C(60000000)
typedef struct TokenBucket { uint64 At, Credit; } TokenBucket;
typedef struct SourceBucket {
    unsigned char Key[17];
    TokenBucket Tokens;
    uint64 Seen;
    bool Used;
} SourceBucket;
typedef struct StunConfig {
    xnetaddr Bind[2];
    uint32 Rate, SourceRate, RunMs;
    bool Enabled[2];
} StunConfig;
typedef struct StunServer {
    xnetengine* Engine;
    xmutex* Lock;
    xnetudp* Udp[2];
    StunConfig Config;
    SourceBucket Sources[STUN_RATE_SETS][STUN_RATE_WAYS];
    TokenBucket Global;
    xsipkey HashKey;
    uint64 Received, Replied, Invalid, RateDropped, TableDropped, IoErrors, Bytes;
    xatomic32 Opened, Failed, Stopping;
} StunServer;
static StunServer Server;
static volatile sig_atomic_t SignalStop;
static void StopSignal(int Number) { (void)Number; SignalStop=1; }
static uint64 ClockUs(void)
{
    double Now=xrtTimer();
    return Now>=0 && Now<(double)UINT64_MAX/1000000.0 ? (uint64)(Now*1000000.0):UINT64_MAX;
}
static bool Take(TokenBucket* Bucket, uint32 Rate, uint64 Now)
{
    uint64 Capacity=(uint64)Rate*STUN_TOKEN;
    if (Now<Bucket->At || Now==UINT64_MAX) return false;
    uint64 Elapsed=Now-Bucket->At;
    if (Elapsed>=STUN_TOKEN || Elapsed*Rate>=Capacity-Bucket->Credit) Bucket->Credit=Capacity;
    else Bucket->Credit+=Elapsed*Rate;
    Bucket->At=Now;
    if (Bucket->Credit<STUN_TOKEN) return false;
    Bucket->Credit-=STUN_TOKEN; return true;
}
static bool SourceTake(StunServer* S, const xnetaddr* Address, uint64 Now)
{
    /* Ports cannot evade a source quota; IPv6 privacy addresses share /64.
     * Four bounded ways, secret SipHash index, no eviction of active entries. */
    unsigned char Key[17]={0}; Key[0]=(uint8)Address->Family;
    memcpy(Key+1,Address->Address,Address->Family==XNET_FAMILY_IPV4 ? 4:8);
    size_t Set=(size_t)(xrtSipHash(Key,sizeof(Key),S->HashKey)%STUN_RATE_SETS);
    SourceBucket* Empty=NULL;
    for (size_t i=0;i<STUN_RATE_WAYS;++i) {
        SourceBucket* B=&S->Sources[Set][i];
        if (B->Used && !memcmp(B->Key,Key,sizeof(Key))) {
            B->Seen=Now;
            if (Take(&B->Tokens,S->Config.SourceRate,Now)) return true;
            ++S->RateDropped; return false;
        }
        if (!B->Used || (Now>=B->Seen && Now-B->Seen>=STUN_ENTRY_IDLE)) Empty=B;
    }
    if (!Empty) { ++S->TableDropped; return false; }
    memset(Empty,0,sizeof(*Empty)); memcpy(Empty->Key,Key,sizeof(Key));
    Empty->Used=true; Empty->Seen=Now; Empty->Tokens.At=Now;
    Empty->Tokens.Credit=(uint64)S->Config.SourceRate*STUN_TOKEN;
    return Take(&Empty->Tokens,S->Config.SourceRate,Now);
}
static size_t ResponsePlan(StunServer* S, const xnetudpmessage* Message, unsigned char Response[80])
{
    ++S->Received;
    if (xrtAtomic32Load(&S->Stopping,XMEMORY_ACQUIRE)) return 0;
    uint64 Now=ClockUs();
    /* The cheap shared limit precedes hashing or parsing hostile input. */
    if (!Take(&S->Global,S->Config.Rate,Now)) { ++S->RateDropped; return 0; }
    if ((Message->Flags&XNET_UDP_MESSAGE_TRUNCATED) || Message->Size>XRUDP_STUN_PACKET_MAX ||
        (Message->Remote.Family!=XNET_FAMILY_IPV4 && Message->Remote.Family!=XNET_FAMILY_IPV6)) { ++S->Invalid; return 0; }
    if (!SourceTake(S,&Message->Remote,Now)) return 0;
    xrudpstunmessage Request;
    if (xrtRudpStunDecode(Message->Data,Message->Size,&Request)!=XRUDP_OK ||
        Request.Type!=XRUDP_STUN_BINDING_REQUEST || Request.Integrity || Request.HasMapped || Request.ErrorCode) { ++S->Invalid; return 0; }
    xrudpaddress Mapped={0}; Mapped.Family=(uint8)Message->Remote.Family;
    Mapped.Port=Message->Remote.Port; Mapped.Zone=Message->Remote.Scope;
    memcpy(Mapped.Address,Message->Remote.Address,16);
    size_t Size=0;
    /* Response validates the real source even when it will become error420. */
    xrudpresult Result=xrtRudpStunResponse(Request.Transaction,&Mapped,Request.Fingerprint,Response,80,&Size);
    if (Result==XRUDP_OK && Request.UnknownCount)
        Result=xrtRudpStunError(Request.Transaction,420,Request.Unknown,Request.UnknownCount,Request.Fingerprint,Response,80,&Size);
    if (Result!=XRUDP_OK || Size>Message->Size*3u) { ++S->Invalid; return 0; }
    return Size;
}
static void Receive(xnetudp* Udp, const xnetudpmessage* Message, ptr Data)
{
    StunServer* S=Data; unsigned char Response[80];
    xrtMutexLock(S->Lock); size_t Size=ResponsePlan(S,Message,Response); xrtMutexUnlock(S->Lock);
    if (!Size) return;
    /* Replies return only to the kernel-provided source, never an attribute.
     * SendTo copies before returning and enforces the UDP queue hard limits. */
    xnetresult Result=xrtNetUdpSendTo(Udp,&Message->Remote,Response,Size);
    xrtMutexLock(S->Lock);
    if (Result==XNET_RESULT_OK) { ++S->Replied; S->Bytes+=Size; } else ++S->IoErrors;
    xrtMutexUnlock(S->Lock);
}
static void Opened(xnetudp* Udp, ptr Data)
{
    StunServer* S=Data; (void)Udp;
    xrtAtomic32FetchAdd(&S->Opened,1,XMEMORY_ACQ_REL);
}
static void Error(xnetudp* Udp, const xerror* Reason, ptr Data)
{
    StunServer* S=Data; (void)Udp; (void)Reason;
    xrtMutexLock(S->Lock); ++S->IoErrors; xrtMutexUnlock(S->Lock);
}
static void Closed(xnetudp* Udp, xnetresult Result, const xerror* Reason, ptr Data)
{
    StunServer* S=Data; (void)Udp; (void)Reason;
    if (Result==XNET_RESULT_ERROR || !xrtAtomic32Load(&S->Stopping,XMEMORY_ACQUIRE)) xrtAtomic32Store(&S->Failed,1,XMEMORY_RELEASE);
}
static void Stats(StunServer* S)
{
    uint64 Values[7];
    xrtMutexLock(S->Lock);
    Values[0]=S->Received; Values[1]=S->Replied; Values[2]=S->Bytes; Values[3]=S->Invalid;
    Values[4]=S->RateDropped; Values[5]=S->TableDropped; Values[6]=S->IoErrors;
    xrtMutexUnlock(S->Lock);
    printf("STATS received=%llu replied=%llu bytes=%llu invalid=%llu rate_dropped=%llu table_dropped=%llu io_errors=%llu\n",
        (unsigned long long)Values[0],(unsigned long long)Values[1],(unsigned long long)Values[2],
        (unsigned long long)Values[3],(unsigned long long)Values[4],(unsigned long long)Values[5],(unsigned long long)Values[6]);
    fflush(stdout);
}
static bool Number(cstr Text, uint32 Maximum, uint32* Output)
{
    if (!Text || !*Text) return false;
    for (cstr P=Text;*P;++P) if (*P<'0' || *P>'9') return false;
    errno=0; char* End; unsigned long Value=strtoul(Text,&End,10);
    if (errno || *End || Value>Maximum) return false;
    *Output=(uint32)Value; return true;
}
static void Usage(void)
{
    puts("xrudp-stun [--bind4 IPv4] [--bind6 IPv6] [--port 0..65535] [--no-v4|--no-v6]\n"
         "           [--rate 1..4096] [--per-source 1..4096] [--run-ms 1..600000]\n"
         "Defaults: dual-stack UDP3478, 256/s global, 16/s per IPv4 or IPv6/64; no authentication or TURN.");
}
static bool Parse(int Count, char** Args, StunConfig* C)
{
    cstr Bind[2]={"0.0.0.0","::"}; uint32 Port=3478, Seen=0; memset(C,0,sizeof(*C));
    C->Enabled[0]=C->Enabled[1]=true; C->Rate=256; C->SourceRate=16;
    for (int i=1;i<Count;++i) {
        cstr A=Args[i]; uint32 Bit=0;
        if (!strcmp(A,"--bind4")) Bit=1;
        else if (!strcmp(A,"--bind6")) Bit=2;
        else if (!strcmp(A,"--port")) Bit=4;
        else if (!strcmp(A,"--rate")) Bit=8;
        else if (!strcmp(A,"--per-source")) Bit=16;
        else if (!strcmp(A,"--run-ms")) Bit=32;
        else if (!strcmp(A,"--no-v4")) Bit=64;
        else if (!strcmp(A,"--no-v6")) Bit=128;
        if (!Bit || (Seen&Bit)) return false;
        Seen|=Bit;
        if (Bit==64 || Bit==128) { C->Enabled[Bit==64 ? 0:1]=false; continue; }
        if (++i==Count) return false;
        if (Bit==1 || Bit==2) Bind[Bit==1 ? 0:1]=Args[i];
        else if (Bit==4) { if (!Number(Args[i],65535,&Port)) return false; }
        else {
            uint32* Target=Bit==8 ? &C->Rate:(Bit==16 ? &C->SourceRate:&C->RunMs);
            if (!Number(Args[i],Bit==32 ? 600000:4096,Target) || !*Target) return false;
        }
    }
    if ((!C->Enabled[0] && !C->Enabled[1]) || C->SourceRate>C->Rate) return false;
    for (unsigned i=0;i<2;++i) if (!xrtNetAddrParse(&C->Bind[i],Bind[i],(uint16)Port) ||
        C->Bind[i].Family!=(i ? XNET_FAMILY_IPV6:XNET_FAMILY_IPV4)) return false;
    return true;
}
int main(int Count, char** Args)
{
    if (Count==2 && !strcmp(Args[1],"--help")) { Usage(); return 0; }
    if (Count==2 && !strcmp(Args[1],"--version")) {
        printf("xrudp-stun xs=%s xrudp=%s core=%s\n",XRUDP_STUN_BUILD_XS,XRUDP_STUN_BUILD_XRUDP,XRUDP_STUN_BUILD_CORE); return 0;
    }
    StunServer* S=&Server;
    if (!Parse(Count,Args,&S->Config)) { Usage(); return 2; }
    if (!xrtSecureRandom(&S->HashKey,sizeof(S->HashKey))) return 1;
    xrtAtomic32Init(&S->Opened,0); xrtAtomic32Init(&S->Failed,0); xrtAtomic32Init(&S->Stopping,0);
    S->Global.At=ClockUs(); S->Global.Credit=(uint64)S->Config.Rate*STUN_TOKEN;
    xnetengineconfig E; xnetbufpoolconfig Pool; xrtNetEngineConfigInit(&E); xrtNetBufPoolConfigInit(&Pool);
    Pool.MaxCacheBytes=32768; E.BufferPool=&Pool; E.Workers=1; E.CommandCapacity=64;
    E.NodeCacheBytes=32768; E.TimerLimit=4; E.EventBatch=16; E.PortPostLimit=16;
    E.PortWatchLimit=8; E.PortOperationLimit=64; E.PortOperationCache=16;
    S->Lock=xrtMutexCreate(); S->Engine=S->Lock ? xrtNetEngineCreate(&E):NULL;
    if (!S->Engine || !xrtNetEngineStart(S->Engine)) {
        if (S->Engine) (void)xrtNetEngineDestroy(S->Engine);
        xrtMutexDestroy(S->Lock); xrtSecureZero(&S->HashKey,sizeof(S->HashKey)); return 1;
    }
    xnetudpconfig C; xrtNetUdpConfigInit(&C); C.ReceiveSize=XRUDP_STUN_PACKET_MAX; C.ReceiveBatch=8;
    C.ReceiveConcurrency=2; C.ReceiveQueueLimit=C.ReceiveQueueByteLimit=0; C.Truncation=XNET_UDP_TRUNCATE_DROP;
    C.SendPacketLimit=32; C.SendLimit=4096; C.SendHighWater=3072; C.SendLowWater=1024;
    C.ReceiveBuffer=C.SendBuffer=65536; C.IPv6Only=true;
    xnetudpevents Events={0}; Events.Open=Opened; Events.Receive=Receive; Events.Error=Error; Events.Close=Closed;
    unsigned Expected=0;
    for (unsigned i=0;i<2;++i) if (S->Config.Enabled[i]) {
        ++Expected; S->Udp[i]=xrtNetUdpBind(S->Engine,&S->Config.Bind[i],0,&C,&Events,S);
        if (!S->Udp[i]) xrtAtomic32Store(&S->Failed,1,XMEMORY_RELEASE);
    }
    double Until=xrtTimer()+5.0;
    while (xrtAtomic32Load(&S->Opened,XMEMORY_ACQUIRE)!=Expected && !xrtAtomic32Load(&S->Failed,XMEMORY_ACQUIRE) && xrtTimer()<Until) xrtSleep(5);
    bool Ok=xrtAtomic32Load(&S->Opened,XMEMORY_ACQUIRE)==Expected && !xrtAtomic32Load(&S->Failed,XMEMORY_ACQUIRE);
    for (unsigned i=0;i<2 && Ok;++i) if (S->Udp[i]) {
        xnetaddr Local; char Text[96];
        if (!xrtNetUdpLocal(S->Udp[i],&Local) || !xrtNetAddrText(&Local,Text,sizeof(Text))) { Ok=false; break; }
        printf("BIND family=%u address=%s port=%u\n",(unsigned)Local.Family,Text,(unsigned)Local.Port);
    }
    signal(SIGINT,StopSignal); signal(SIGTERM,StopSignal);
    if (Ok) { puts("READY bounded Binding reflector; mappings are not account authority"); fflush(stdout); }
    double Started=xrtTimer(), NextStats=Started+60;
    while (Ok && !SignalStop && !xrtAtomic32Load(&S->Failed,XMEMORY_ACQUIRE) &&
        (!S->Config.RunMs || (xrtTimer()-Started)*1000.0<S->Config.RunMs)) {
        if (xrtTimer()>=NextStats) { Stats(S); NextStats=xrtTimer()+60; }
        xrtSleep(20);
    }
    if (xrtAtomic32Load(&S->Failed,XMEMORY_ACQUIRE)) Ok=false;
    xrtAtomic32Store(&S->Stopping,1,XMEMORY_RELEASE); Until=xrtTimer()+5.0;
    for (;;) {
        bool ClosedAll=true;
        for (unsigned i=0;i<2;++i) if (S->Udp[i] && xrtNetUdpState(S->Udp[i])!=XNET_UDP_CLOSED) {
            ClosedAll=false; (void)xrtNetUdpAbort(S->Udp[i]);
        }
        if (ClosedAll || xrtTimer()>=Until) { if (!ClosedAll) Ok=false; break; }
        xrtSleep(5);
    }
    for (unsigned i=0;i<2;++i) { xrtNetUdpDestroy(S->Udp[i]); S->Udp[i]=NULL; }
    bool Joined=xrtNetEngineStop(S->Engine), Destroyed=Joined && xrtNetEngineDestroy(S->Engine);
    if (!Destroyed) Ok=false;
    if (Destroyed) { Stats(S); xrtMutexDestroy(S->Lock); xrtSecureZero(&S->HashKey,sizeof(S->HashKey)); }
    printf("STOP joined=%u clean=%u\n",Destroyed,Ok); fflush(stdout); return Ok ? 0:1;
}
