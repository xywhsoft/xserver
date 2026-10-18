#ifndef XS_PROTOCOL_XRUDP_H
#define XS_PROTOCOL_XRUDP_H

#include "../sdk/xsbase.h"
#include "../core/config.h"
#include "../core/engine.h"
#include "../runtime/listener_slot.h"
#include "../script/script.h"
#include "xrudp_config.h"

#ifdef XS_USE_XRUDP
typedef struct XS_RudpTransport XS_RudpTransport;
typedef struct XS_RudpRecord {
	xrudppeer* Peer; /* borrowed while live; native keeps it through PeerClose */
	XS_ServerGeneration* Generation;
	XS_ScriptRuntime* Script;
	uint32 Depth;
	bool Closed;
} XS_RudpRecord;

typedef struct XS_RudpRuntime {
	XS_ServerInfo* Server;
	XS_ServerGeneration* Generation;
	XS_ScriptRuntime* Script;
	XS_ListenerSlot* Slot;
	XS_RudpTransport* Transport;
	XS_RudpSettings Settings;
	xnetpost ReadyPost;
	xatomic32 ReadyPosted, Stopping;
} XS_RudpRuntime;

struct XS_RudpTransport {
	xatomic32 References, Stopping;
	xnetengine* Engine;
	XS_ListenerSlot* Slot; /* worker-only after startup */
	xrudpendpoint* Endpoint; /* worker-only; slot provides thread-safe close refs */
	xpromise* Started;
	xrudpresult StartResult;
	xnetaddr Local;
	xrudpendpointconfig Config;
	unsigned char Private[32], Scope[16];
	uint64 ReadyCookie;
	/* Fixed bound includes deferred record retirement during reentrant Close. */
	XS_RudpRecord Records[XRUDP_ENDPOINT_PEERS_MAX];
};

static XS_RudpTransport* XS_RudpTransportRef(XS_RudpTransport* t)
{
	if ( t != NULL ) xrtAtomic32FetchAdd(&t->References, 1, XMEMORY_RELAXED);
	return t;
}

static void XS_RudpTransportRelease(XS_RudpTransport* t)
{
	if ( t == NULL || xrtAtomic32FetchSub(&t->References, 1, XMEMORY_ACQ_REL) != 1 ) return;
	if ( t->Started != NULL ) xrtPromiseDestroy(t->Started);
	xrtSecureZero(t, sizeof(*t)); xrtFree(t);
}

static void XS_RudpStarted(XS_RudpTransport* t, xrudpresult result)
{
	if ( t->Started == NULL ) return;
	t->StartResult = result;
	(void)xrtPromiseResolve(t->Started, NULL);
	xrtPromiseDestroy(t->Started); t->Started = NULL;
}

static XS_RudpRecord* XS_RudpRecordFind(XS_RudpTransport* t, xrudppeer* peer)
{
	for ( uint32 i = 0; i < XRUDP_ENDPOINT_PEERS_MAX; ++i ) {
		if ( t->Records[i].Peer == peer ) return &t->Records[i];
	}
	return NULL;
}

static void XS_RudpRecordLeave(XS_RudpRecord* r)
{
	if ( --r->Depth == 0 && r->Closed ) {
		XS_ScriptRuntime* script = r->Script;
		XS_ServerGeneration* generation = r->Generation;
		memset(r, 0, sizeof(*r));
		XS_ScriptRelease(script);
		XS_GenerationConnectionRelease(generation);
	}
}

static bool XS_RudpAllowed(XS_RudpRuntime* runtime, const unsigned char* key,
	const xrudphandshakeinfo* info)
{
	XS_RudpSettings* s = &runtime->Settings;
	bool allowed = !s->HasPin || memcmp(s->Pin, key, 32) == 0;
	if ( info != NULL && s->HasApplication && memcmp(s->Application, info->ApplicationContext, 32) != 0 ) allowed = false;
	if ( allowed && runtime->Script->procRudpAuthorize != NULL ) {
		XS_ScriptRuntime* previous = XS_ScriptEnter(runtime->Script);
		allowed = runtime->Script->procRudpAuthorize(runtime->Server->DefaultHost, key, info);
		XS_ScriptLeave(previous);
	}
	return allowed;
}

static bool XS_RudpTrust(const unsigned char* key, void* context)
{
	XS_RudpTransport* t = context;
	XS_RudpRuntime* runtime = NULL;
	XS_ServerGeneration* generation = NULL;
	bool allowed;
	if ( t->Slot == NULL || !XS_ListenerSlotAcquireConnection(t->Slot, 0, (void**)&runtime, &generation) ) return false;
	allowed = XS_RudpAllowed(runtime, key, NULL);
	XS_GenerationConnectionRelease(generation);
	return allowed;
}

static void XS_RudpOnOpen(xrudpendpoint* endpoint, void* context)
{
	XS_RudpTransport* t = context;
	xrudpendpointinfo info;
	if ( xrtAtomic32Load(&t->Stopping, XMEMORY_ACQUIRE) ||
	     xrtRudpEndpointInfo(endpoint, &info) != XRUDP_OK ) {
		XS_RudpStarted(t, XRUDP_CANCELLED); (void)xrtRudpEndpointClose(endpoint); return;
	}
	t->Local = info.Local; memcpy(t->Scope, info.Scope, 16);
	XS_RudpStarted(t, XRUDP_OK);
}

static void XS_RudpOnConfirmed(xrudpendpoint* endpoint, xrudppeer* peer, void* context)
{
	XS_RudpTransport* t = context;
	XS_RudpRuntime* runtime = NULL;
	XS_ServerGeneration* generation = NULL;
	xrudphandshakeinfo info;
	xnetaddr address;
	XS_RudpRecord* record = NULL;
	(void)endpoint;
	if ( t->Slot == NULL || !XS_ListenerSlotAcquireConnection(t->Slot, 0, (void**)&runtime, &generation) ) goto Denied;
	if ( xrtRudpPeerInfo(peer, &info, &address) != XRUDP_OK || !XS_RudpAllowed(runtime, info.PeerPublic, &info) ) goto Denied;
	for ( uint32 i = 0; i < XRUDP_ENDPOINT_PEERS_MAX; ++i ) {
		if ( t->Records[i].Peer == NULL ) { record = &t->Records[i]; break; }
	}
	if ( record == NULL || !XS_ScriptRetain(runtime->Script) ) goto Denied;
	record->Peer = peer; record->Generation = generation; record->Script = runtime->Script; record->Depth = 1;
	if ( record->Script->procRudpOpen != NULL ) {
		XS_ScriptRuntime* previous = XS_ScriptEnter(record->Script);
		record->Script->procRudpOpen(record->Script->pHost, peer);
		XS_ScriptLeave(previous);
	}
	XS_RudpRecordLeave(record);
	return;
Denied:
	XS_GenerationConnectionRelease(generation);
	(void)xrtRudpPeerAbort(peer);
}

static void XS_RudpOnPeerClose(xrudpendpoint* endpoint, xrudppeer* peer, xrudpresult reason, void* context)
{
	XS_RudpRecord* record = XS_RudpRecordFind(context, peer);
	(void)endpoint;
	if ( record == NULL || record->Closed ) return;
	++record->Depth; record->Closed = true;
	if ( record->Script->procRudpClose != NULL ) {
		XS_ScriptRuntime* previous = XS_ScriptEnter(record->Script);
		record->Script->procRudpClose(record->Script->pHost, peer, reason);
		XS_ScriptLeave(previous);
	}
	XS_RudpRecordLeave(record);
}

static void XS_RudpOnChanged(xrudpendpoint* endpoint, xrudppeer* peer, uint32_t events, void* context)
{
	XS_RudpRecord* record = XS_RudpRecordFind(context, peer);
	(void)endpoint;
	if ( record == NULL || record->Closed || record->Script->procRudpChanged == NULL ) return;
	++record->Depth;
	XS_ScriptRuntime* previous = XS_ScriptEnter(record->Script);
	record->Script->procRudpChanged(record->Script->pHost, peer, events);
	XS_ScriptLeave(previous);
	XS_RudpRecordLeave(record);
}

static void XS_RudpOnStreamChanged(xrudpendpoint* endpoint, xrudppeer* peer, const xrudpstreamevent* event, void* context)
{
	XS_RudpRecord* record = XS_RudpRecordFind(context, peer);
	(void)endpoint;
	if ( record == NULL || record->Closed || record->Script->procRudpStreamChanged == NULL ) return;
	++record->Depth;
	XS_ScriptRuntime* previous = XS_ScriptEnter(record->Script);
	record->Script->procRudpStreamChanged(record->Script->pHost, peer, event);
	XS_ScriptLeave(previous);
	XS_RudpRecordLeave(record);
}

static void XS_RudpOnClose(xrudpendpoint* endpoint, xrudpresult reason, const xerror* error, void* context)
{
	XS_RudpTransport* t = context;
	XS_ListenerSlot* slot = t->Slot;
	(void)error;
	XS_RudpStarted(t, reason == XRUDP_OK ? XRUDP_CLOSED : reason);
	t->Endpoint = NULL; t->Slot = NULL;
	XS_ListenerSlotResourceClose(slot, XS_LISTENER_RESOURCE_RUDP);
	xrtRudpEndpointDestroy(endpoint);
	XS_RudpTransportRelease(t); /* native I/O lifetime */
}

static const xrudpendpointevents g_XS_RudpEvents = {
	XS_RudpOnOpen, XS_RudpOnConfirmed, XS_RudpOnPeerClose, XS_RudpOnClose,
	XS_RudpOnChanged, XS_RudpOnStreamChanged
};

static void XS_RudpBindTask(xnetworker* worker, ptr data)
{
	XS_RudpTransport* t = data;
	xrudpresult result = XRUDP_CANCELLED;
	if ( !xrtAtomic32Load(&t->Stopping, XMEMORY_ACQUIRE) ) {
		result = xrtRudpEndpointBind(worker, &t->Local, &t->Config, &g_XS_RudpEvents, t, &t->Endpoint);
	}
	xrtSecureZero(t->Private, sizeof(t->Private));
	t->Config.StaticPrivate = NULL;
	if ( result != XRUDP_OK ) {
		XS_ListenerSlot* slot = t->Slot; t->Slot = NULL;
		XS_RudpStarted(t, result);
		XS_ListenerSlotResourceClose(slot, XS_LISTENER_RESOURCE_RUDP);
		XS_RudpTransportRelease(t);
		return;
	}
	(void)XS_ListenerSlotResourceAttach(t->Slot, XS_LISTENER_RESOURCE_RUDP, t->Endpoint);
	if ( xrtAtomic32Load(&t->Stopping, XMEMORY_ACQUIRE) ) (void)xrtRudpEndpointClose(t->Endpoint);
}

static void XS_RudpReadyTask(xnetworker* worker, ptr data)
{
	XS_RudpRuntime* requested = data;
	XS_RudpTransport* t = requested->Transport;
	XS_RudpRuntime* runtime = NULL;
	XS_ServerGeneration* generation = NULL;
	(void)worker;
	if ( t->Slot != NULL && t->Endpoint != NULL &&
	     XS_ListenerSlotAcquireConnection(t->Slot, requested->Generation->iCookie, (void**)&runtime, &generation) ) {
		if ( t->ReadyCookie != generation->iCookie && xrtRudpEndpointState(t->Endpoint) == XRUDP_ENDPOINT_OPEN ) {
			t->ReadyCookie = generation->iCookie;
			if ( runtime->Script->procRudpReady != NULL ) {
				XS_ScriptRuntime* previous = XS_ScriptEnter(runtime->Script);
				runtime->Script->procRudpReady(runtime->Script->pHost, t->Endpoint);
				XS_ScriptLeave(previous);
			}
		}
		XS_GenerationConnectionRelease(generation);
	}
	XS_RudpTransportRelease(t);
	XS_GenerationRelease(requested->Generation); /* requested may finalize now */
}

static void XS_RudpPublished(XS_RudpRuntime* runtime)
{
	if ( runtime == NULL || runtime->Transport == NULL ||
	     xrtAtomic32Exchange(&runtime->ReadyPosted, 1, XMEMORY_ACQ_REL) ) return;
	if ( !XS_GenerationRetain(runtime->Generation) ) return;
	XS_RudpTransportRef(runtime->Transport);
	if ( !xrtNetPost(xrtNetEngineWorker(runtime->Server->Engine, 0), &runtime->ReadyPost, XS_RudpReadyTask, runtime) ) {
		XS_RudpTransportRelease(runtime->Transport);
		XS_GenerationRelease(runtime->Generation);
		xrtAtomic32Store(&runtime->ReadyPosted, 0, XMEMORY_RELEASE);
	}
}

static void XS_RudpStop(XS_RudpRuntime* runtime)
{
	XS_ListenerResources resources;
	XS_ListenerSlot* slot;
	if ( runtime == NULL || xrtAtomic32Exchange(&runtime->Stopping, 1, XMEMORY_ACQ_REL) ) return;
	slot = runtime->Slot; runtime->Slot = NULL;
	/* A handed-off runtime owns no listener. Its peers still drain using the
	 * original script; only the active listener owner may close the transport. */
	if ( slot == NULL ) return;
	if ( runtime->Transport != NULL ) xrtAtomic32Store(&runtime->Transport->Stopping, 1, XMEMORY_RELEASE);
	if ( XS_ListenerSlotBeginClose(slot, runtime, &resources) && resources.pRudp != NULL ) {
		(void)xrtRudpEndpointClose(resources.pRudp);
		xrtRudpEndpointDestroy(resources.pRudp);
	}
}

static void XS_RudpUnit(XS_RudpRuntime* runtime)
{
	if ( runtime == NULL ) return;
	XS_ScriptRelease(runtime->Script);
	XS_RudpTransportRelease(runtime->Transport);
	if ( runtime->Slot != NULL ) XS_ListenerSlotDestroyEmpty(runtime->Slot);
	xrtFree(runtime);
}

static bool XS_RudpStartEx(XS_ServerInfo* server, bool startEndpoint, bool acceptEndpoint, char* error, size_t capacity)
{
	XS_RudpRuntime* r = xrtCalloc(1, sizeof(*r));
	XS_RudpTransport* t;
	unsigned char privateKey[32] = {0};
	xfuture* started = NULL;
	bool ok = false;
	if ( r == NULL ) { snprintf(error, capacity, "out of memory"); return false; }
	server->Runtime = r; r->Server = server; r->Generation = server->Generation;
	xrtAtomic32Init(&r->Stopping, 0); xrtAtomic32Init(&r->ReadyPosted, 0); (void)xrtNetPostInit(&r->ReadyPost);
	if ( server->RecvLimit != 0 || server->Backlog != 0 ) {
		snprintf(error, capacity, "xrudp uses bounded xrudp settings; backlog and recv_limit must be zero"); goto Done;
	}
	r->Script = XS_ScriptAcquireHost(server->DefaultHost);
	if ( r->Script == NULL || (r->Script->procRudpChanged == NULL && r->Script->procRudpStreamChanged == NULL) ) {
		snprintf(error, capacity, "xrudp server '%s' requires RudpChanged or RudpStreamChanged", server->Name); goto Done;
	}
	if ( !XS_RudpSettingsLoad(server, &r->Settings, privateKey, error, capacity) ) goto Done;
	if ( !r->Settings.HasPin && r->Script->procRudpAuthorize == NULL ) {
		snprintf(error, capacity, "xrudp server '%s' requires peer_public or RudpAuthorize", server->Name); goto Done;
	}
	if ( !startEndpoint ) { ok = true; goto Done; }
	if ( xrtNetEngineCurrent(server->Engine) != NULL ) { snprintf(error, capacity, "xrudp startup cannot wait on an engine worker"); goto Done; }
	t = xrtCalloc(1, sizeof(*t));
	if ( t == NULL ) { snprintf(error, capacity, "out of memory"); goto Done; }
	xrtAtomic32Init(&t->References, 1); xrtAtomic32Init(&t->Stopping, 0);
	r->Transport = t; t->Engine = server->Engine;
	t->Started = xrtPromiseCreate(&started, NULL);
	if ( t->Started == NULL ) { snprintf(error, capacity, "xrudp startup promise allocation failed"); goto Done; }
	if ( server->Port == 0 ) server->Port = XS_PortStickyGet(server->Name);
	if ( !xrtNetAddrParse(&t->Local, server->IP ? server->IP : "0.0.0.0", server->Port) ) {
		snprintf(error, capacity, "xrudp address parse failed"); goto Done;
	}
	r->Slot = XS_ListenerSlotCreate(r, r->Generation, NULL, acceptEndpoint);
	if ( r->Slot == NULL || !XS_ListenerSlotResourceAdd(r->Slot, XS_LISTENER_RESOURCE_RUDP) ) {
		snprintf(error, capacity, "xrudp listener slot allocation failed"); goto Done;
	}
	t->Slot = r->Slot; t->Config = r->Settings.Native;
	memcpy(t->Private, privateKey, 32); t->Config.StaticPrivate = t->Private;
	if ( r->Settings.HasScope ) { memcpy(t->Scope, r->Settings.Scope, 16); t->Config.Scope = t->Scope; }
	/* Always gate Trust through the current published slot, even with a static
	 * pin. The final callback also rechecks policy after a concurrent reload. */
	t->Config.Trust = XS_RudpTrust; t->Config.TrustContext = t;
	XS_RudpTransportRef(t); /* pending bind -> native I/O reference */
	if ( !xrtNetEnginePost(server->Engine, 0, XS_RudpBindTask, t) ) {
		XS_ListenerSlotResourceCancel(r->Slot, XS_LISTENER_RESOURCE_RUDP);
		XS_RudpTransportRelease(t);
		snprintf(error, capacity, "xrudp bind task queue full"); goto Done;
	}
	/* Timeout stops admission immediately; the accepted worker task still owns
	 * its slot/transport until it cancels or closes, so no late-bind UAF. */
	if ( xrtFutureWaitFor(started, 30000) != XWAIT_OK ) {
		snprintf(error, capacity, "xrudp bind timed out after 30 seconds"); goto Done;
	}
	if ( t->StartResult != XRUDP_OK ) { snprintf(error, capacity, "xrudp bind failed (reason %u)", (unsigned)t->StartResult); goto Done; }
	server->Port = server->PortBound = t->Local.Port;
	XS_PortStickySet(server->Name, server->Port);
	ok = true;
Done:
	xrtSecureZero(privateKey, sizeof(privateKey));
	if ( started != NULL ) xrtFutureDestroy(started);
	return ok;
}

static bool XS_RudpCanHandoff(XS_RudpRuntime* old, XS_RudpRuntime* next)
{
	if ( old == NULL || next == NULL || old->Transport == NULL ||
	     (next->Transport != NULL && next->Transport != old->Transport) ) return false;
	XS_RudpSettings* a = &old->Settings; XS_RudpSettings* b = &next->Settings;
	/* Session/identity/socket budgets are immutable for an existing endpoint.
	 * Policy pin/context are evaluated per admission and may change safely. */
	bool ok = memcmp(a->Public, b->Public, 32) == 0 &&
		memcmp(&a->Native, &b->Native, sizeof(a->Native)) == 0 &&
		(!b->HasScope || memcmp(old->Transport->Scope, b->Scope, 16) == 0);
	if ( !ok ) printf("[xs] xrudp '%s' reload rejected: identity, scope or transport budgets changed on a live endpoint\n", old->Server->Name);
	return ok;
}

static bool XS_RudpHandoff(XS_RudpRuntime* old, XS_RudpRuntime* next)
{
	if ( !XS_RudpCanHandoff(old, next) || !XS_ListenerSlotHandoff(old->Slot, old, next, next->Generation, NULL) ) return false;
	next->Slot = old->Slot; old->Slot = NULL;
	/* Rollback may hand the listener back to a runtime that already retains
	 * this shared transport. Do not replace that owner with a second reference. */
	if ( next->Transport == NULL ) next->Transport = XS_RudpTransportRef(old->Transport);
	next->Server->PortBound = old->Server->PortBound;
	next->Server->Port = old->Server->Port;
	return true;
}
#endif
#endif
