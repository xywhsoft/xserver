#ifdef XS_XTP3_EXAMPLE
#include <xsbase.h>
#include <stdlib.h>
#include <stdio.h>
#define REQUIRE(x) do { if (!(x)) { printf("FAIL: %s\n", #x); exit(5); } } while(0)
#endif
#include <xtp.h>
#include <string.h>
typedef struct Xtp3Probe {unsigned Requests,Pushes,Notifies,Responses,Acks,Sent;uint64_t Binding;} Xtp3Probe;
static xtpdecision Xtp3Message(xtpsession* s,const xtpmessageview* m,void* user)
{
    Xtp3Probe* p=user;xbytesview value;
    REQUIRE(xtpParamAt(&m->Params,0,&value)==XTP_PARAM_FOUND&&value.Size==1&&value.Data[0]=='v');
    if(m->Params.Kind==XTP_PARAMS_TABLE){xbytesview byname;
        REQUIRE(xtpParamFind(&m->Params,XRT_STR_LITERAL("key"),&byname)==XTP_PARAM_FOUND&&byname.Data==value.Data);}
    if(m->Type==XTP_REQUEST){xtpresponse r={0};++p->Requests;r.Body=XRT_BYTES_LITERAL("answer");
        /* GCC host calls a TCC callback returning xtpdecision; TCC then calls
         * the host with a 32-byte request token passed by value. */
        REQUIRE(xtpReply(s,m->Request,&r,0)==XTP_OK);}
    else if(m->Type==XTP_PUSH)++p->Pushes;else if(m->Type==XTP_NOTIFY)++p->Notifies;else REQUIRE(0);
    return xtpAccept();
}
static void Xtp3Complete(xtpsession* s,const xtpcompletion* c,void* user)
{
    Xtp3Probe* p=user;(void)s;
    if(c->Reason==XTP_FINISH_RESPONSE){++p->Responses;REQUIRE(c->Knowledge==XTP_KNOW_REPLIED&&c->Body.Size==6&&!memcmp(c->Body.Data,"answer",6));}
    else if(c->Reason==XTP_FINISH_ACK){++p->Acks;REQUIRE(c->Knowledge==XTP_KNOW_ACCEPTED);}
    else {REQUIRE(c->Reason==XTP_FINISH_SENT);++p->Sent;}
}
static void Xtp3Move(xtpsession* a,xtpsession* b,uint64_t binding)
{
    xtpoutput out;
    while(xtpOutput(a,0,&out)==XTP_OK){unsigned char bytes[512];size_t size=out.Data.Size;REQUIRE(size<=sizeof(bytes));
        /* Exercise a STREAM prefix crossing a frame boundary in aggregated
         * output; the remaining suffix gets a fresh opaque host ticket. */
        if(out.Mode==XTP_INPUT_STREAM&&out.Frames>1&&size>65)size=65;
        memcpy(bytes,out.Data.Data,size);REQUIRE(xtpAdvance(a,out.Ticket,size,0)==XTP_OK);
        xtpreadprogress progress;REQUIRE(xtpInput(b,binding,out.LaneID,(xbytesview){bytes,size},out.Mode==XTP_INPUT_RECORD,0,&progress)>=0&&progress.Used==size);}
}
static void CheckXtp3(void)
{
    unsigned char bytes[256];xtppacket packet={0};xtpframeview view;size_t size;
    xtpfield field={XRT_STR_INIT("key"),XRT_BYTES_INIT("v")};xbytesview arg=XRT_BYTES_LITERAL("v");
    packet.Type=XTP_REQUEST;packet.MessageID=7;packet.Command=xtpCmdID(0);packet.Params=xtpTable(&field,1);
    REQUIRE(xtpEncode(&packet,1,NULL,bytes,sizeof(bytes),&size)==XTP_OK&&bytes[3]==3);
    REQUIRE(xtpDecode((xbytesview){bytes,size},NULL,&view)==XTP_OK&&view.Command.As.ID==0);
    REQUIRE(xtpParamAt(&view.Params,0,&arg)==XTP_PARAM_FOUND&&arg.Size==1);
    bytes[3]=2;REQUIRE(xtpDecode((xbytesview){bytes,size},NULL,&view)==XTP_E_VERSION);bytes[3]=3;
    Xtp3Probe ap={0},bp={0};xtpevents events={Xtp3Message,Xtp3Complete,NULL};xtpsession* a;xtpsession* b;
    xtpconfig config;xtpConfigInit(&config);REQUIRE(config.BatchBytes==16384);
    REQUIRE(xtpCreate(&config,&events,&ap,&a)==XTP_OK&&xtpCreate(&config,&events,&bp,&b)==XTP_OK);
    xtplane lanes[]={{1,XTP_INPUT_STREAM,true,0},{2,XTP_INPUT_RECORD,false,1200}};
    xtpbinding binding={lanes,2,false,true,NULL};REQUIRE(xtpBind(a,&binding,0,&ap.Binding)==XTP_OK&&xtpBind(b,&binding,0,&bp.Binding)==XTP_OK);
    for(unsigned shape=0;shape<4;++shape){xtpmessage m={0};m.Type=XTP_REQUEST;
        m.Command=shape&1?xtpCmdName(XRT_STR_LITERAL("ping")):xtpCmdID(0);
        m.Params=shape&2?xtpArray(&arg,1):xtpTable(&field,1);REQUIRE(xtpSubmit(a,&m,NULL,0,NULL)==XTP_OK);
        Xtp3Move(a,b,bp.Binding);Xtp3Move(b,a,ap.Binding);}
    xtpmessage batch[3];for(unsigned i=0;i<3;++i){batch[i]=(xtpmessage){0};batch[i].Type=XTP_REQUEST;
        batch[i].Command=xtpCmdID(i);batch[i].Params=xtpTable(&field,1);}
    REQUIRE(xtpSubmitBatch(a,batch,3,NULL,0,NULL)==XTP_OK);
    xtpoutput grouped;REQUIRE(xtpOutput(a,1,&grouped)==XTP_OK&&grouped.Frames==3);
    Xtp3Move(a,b,bp.Binding);Xtp3Move(b,a,ap.Binding);
    xtpinfo stats;REQUIRE(xtpInfo(a,&stats)==XTP_OK&&stats.Batches&&stats.BatchedFrames>=3&&stats.BatchCopiedBytes>=grouped.Data.Size);
    xtpmessage m={0};m.Type=XTP_PUSH;m.Command=xtpCmdID(9);m.Params=xtpTable(&field,1);
    REQUIRE(xtpSubmit(a,&m,NULL,0,NULL)==XTP_OK);Xtp3Move(a,b,bp.Binding);Xtp3Move(b,a,ap.Binding);
    m.Type=XTP_NOTIFY;xtpsubmitoptions option={0};option.LaneID=2;REQUIRE(xtpSubmit(a,&m,&option,0,NULL)==XTP_OK);Xtp3Move(a,b,bp.Binding);
    REQUIRE(bp.Requests==7&&bp.Pushes==1&&bp.Notifies==1&&ap.Responses==7&&ap.Acks==1&&ap.Sent==1);
    REQUIRE(xtpClose(a,0)==XTP_CLOSED&&xtpClose(b,0)==XTP_CLOSED&&xtpDestroy(a)==XTP_OK&&xtpDestroy(b)==XTP_OK);
    printf("[sdk] xtp3 codec+four-shapes+callback-ABI+RPC+PUSH+NOTIFY+batch ok\n");
}

#ifdef XS_XTP3_EXAMPLE
void ServiceInit(XS_HostInfo* host)
{(void)host;CheckXtp3();fflush(stdout);exit(0);}
#endif
