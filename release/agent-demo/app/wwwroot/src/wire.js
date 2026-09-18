// wire.js — 协议层骨架。两种宿主实现同一接口：
//
//   FixtureHost  —— 本地演示（脚本化事件流，离线可跑）
//   XsHttpHost   —— xs C 脚本后端（HTTP 轮询；agent-demo 的正式宿主）
//   DshHttpHost  —— dsh 兼容线协议（HTTP POST /api 上行 + WebSocket 下行）。
//
// 宿主接口（UI 只依赖这组方法）：
//   prompt(sessionId, parts)              提交一轮输入（parts 含 text/image）
//   respondApproval(id, decision)         审批决议（allow-once / allow-session / rejected）
//   cancel(sessionId)                     中断运行
//   retry(sessionId)                      重跑最后一轮（截断重放后重发）
//   editUserMessage(sessionId, seq, text) 编辑历史用户消息并重发（分支）
//
// DshHttpHost 的帧形态照抄 dsh-client-connection 的 zod schema：
//   上行  POST /api            { rpcId, payload: { kind:'request', method, params } }
//   审批  POST /api/respond    { rpcId, payload: { kind:'respond', approvalId, decision } }
//   下行  WS  /api/events.mux  { rpcId, payload: <mux frame> }
//   下行  WS  /api/events.host { rpcId, payload: <host frame> }
// 对接自写宿主时把 DshHttpHost 接上即可，UI/折叠层零改动。

export class FixtureHost {
  name = 'fixture';
  #cancelled = new Set();
  #waiters = new Map(); // approvalId -> resolve(decision)

  constructor(store, options = {}) {
    this.store = store;
    this.scale = options.scale ?? 1;      // 演示速度因子（截图/演示可调快）
    this.autoApprove = options.autoApprove ?? false;
    this.instant = options.instant ?? false; // 整块落账（无头/后台窗口截图用）
    this.effort = 'high';                 // 思考力度（演示：off 时不推 reasoning 流）
    this.ready = Promise.resolve();
  }

  async listSessions() { return []; }

  /** 用户提交输入：宿主记账 user/message（宿主是日志权威），再跑一轮脚本化回合。 */
  async prompt(sessionId, parts) { /* 由 fixture.js 注入实现 */ }
  async runShowcase(sessionId) { /* 同上 */ }
  async runMultimodal(sessionId) { /* 同上：图片+todo 推进+子代理演示 */ }
  async retry(sessionId) { /* 由 fixture.js 注入 */ }
  async editUserMessage(sessionId, seq, text) { /* 由 fixture.js 注入 */ }

  async respondApproval(id, decision) {
    const w = this.#waiters.get(id);
    if (w) { this.#waiters.delete(id); w(decision); }
  }
  async cancel(sessionId) { this.#cancelled.add(sessionId); }

  /** 脚本里等待审批：返回决议；autoApprove / 会话策略 auto 则自动放行。 */
  awaitApproval(sessionId, approval) {
    const s = this.store.sessions.get(sessionId);
    const auto = this.autoApprove || s?.policy === 'auto';
    this.store.append(sessionId, 'approval/requested', approval);
    if (auto) {
      return new Promise((resolve) => setTimeout(() => {
        this.store.append(sessionId, 'approval/resolved', { id: approval.id, decision: 'allow-session' });
        resolve('allow-session');
      }, 400 * this.scale));
    }
    return new Promise((resolve) => this.#waiters.set(approval.id, (decision) => {
      this.store.append(sessionId, 'approval/resolved', { id: approval.id, decision });
      resolve(decision);
    }));
  }

  get cancelled() { return false; }
  isCancelled(sessionId) { return this.#cancelled.has(sessionId); }
  clearCancel(sessionId) { this.#cancelled.delete(sessionId); }
}

// ============ XsHttpHost — xs C 脚本后端（HTTP 轮询） ============
//
// 后端是「回合运行器」：POST /api/prompt 启动一个脚本化回合，
// 本宿主每 250ms 轮询 GET /api/turn/<id>/events?since=N 拉新事件
// 并 append 到 Store。历史由前端持有（事件溯源 + localStorage），
// 后端无会话状态，只管跑完当前回合。

export class XsHttpHost {
  name = 'xs';
  /** sessionId -> { turnId, since, done } */
  #turns = new Map();
  #pollTimer = null;
  #cancelled = new Set();
  #approvalTurn = null; // {sessionId, turnId}

  constructor(store, options = {}) {
    this.store = store;
    this.base = options.base ?? '';       // 同源：空串 = location.origin
    this.effort = 'high';
    this.ready = Promise.resolve();
  }

  async #api(path, opts = {}) {
    const res = await fetch(this.base + path, {
      headers: { 'content-type': 'application/json' },
      ...opts,
    });
    if (!res.ok) throw new Error(`${path} → HTTP ${res.status}`);
    return res.json();
  }

  // ---- 宿主接口 ----

  async prompt(sessionId, parts) {
    const text = parts.filter((p) => p.type === 'text').map((p) => p.text).join('\n');
    const data = await this.#api('/api/prompt', {
      method: 'POST',
      body: JSON.stringify({ sessionId, text }),
    });
    if (!data.ok) throw new Error(data.error ?? 'prompt failed');
    this.#turns.set(sessionId, { turnId: data.turnId, since: 0, done: false });
    this.#startPolling();
  }

  /** 空态演示卡片：复用 prompt 路由到 C 端的 tools / chart 脚本。 */
  async runShowcase(sessionId) {
    await this.prompt(sessionId, [{ type: 'text', text: '请用工具完整演示：构建并审批' }]);
  }
  async runMultimodal(sessionId) {
    await this.prompt(sessionId, [{ type: 'text', text: '画个趋势图分析一下' }]);
  }

  async respondApproval(id, decision) {
    const entry = this.#approvalTurn;
    if (!entry) return;
    await this.#api(`/api/turn/${entry.turnId}/approval`, {
      method: 'POST',
      body: JSON.stringify({ id, decision }),
    });
    // approval/resolved 事件会经轮询到达
  }

  async cancel(sessionId) {
    const entry = this.#turns.get(sessionId);
    if (!entry) return;
    this.#cancelled.add(sessionId);
    try {
      await this.#api(`/api/turn/${entry.turnId}/cancel`, { method: 'POST' });
    } catch { /* 后端可能已完成 */ }
  }

  async retry(sessionId) {
    const s = this.store.sessions.get(sessionId);
    if (!s) return;
    let lastUserIdx = -1, lastUserText = '';
    for (let i = 0; i < s.log.events.length; i++) {
      const ev = s.log.events[i];
      if (ev.type === 'user/message') {
        lastUserIdx = i;
        lastUserText = (ev.data.content ?? [])
          .filter((p) => p.type === 'text').map((p) => p.text).join('\n');
      }
    }
    if (lastUserIdx < 0) return;
    this.store.truncate(sessionId, lastUserIdx);
    await this.prompt(sessionId, [{ type: 'text', text: lastUserText }]);
  }

  async editUserMessage(sessionId, seq, text) {
    const s = this.store.sessions.get(sessionId);
    if (!s) return;
    const idx = s.log.events.findIndex((e) => e.seq === seq);
    if (idx < 0) return;
    this.store.truncate(sessionId, idx);
    await this.prompt(sessionId, [{ type: 'text', text }]);
  }

  get cancelled() { return this.#cancelled.size > 0; }
  isCancelled(sessionId) { return this.#cancelled.has(sessionId); }
  clearCancel(sessionId) { this.#cancelled.delete(sessionId); }

  // ---- 轮询引擎 ----

  #startPolling() {
    if (this.#pollTimer) return;
    this.#pollTimer = setTimeout(() => this.#pollTick(), 100);
  }

  async #pollTick() {
    this.#pollTimer = null;
    const active = [...this.#turns.entries()].filter(([, t]) => !t.done);
    if (active.length === 0) return;

    for (const [sessionId, entry] of active) {
      try {
        const data = await this.#api(
          `/api/turn/${entry.turnId}/events?since=${entry.since}`);
        if (!data.ok) continue;
        for (const ev of data.events ?? []) {
          this.store.append(sessionId, ev.type, ev.data);
          entry.since = ev.seq;
          if (ev.type === 'approval/requested') {
            this.#approvalTurn = { sessionId, turnId: entry.turnId };
          }
          if (ev.type === 'approval/resolved') {
            this.#approvalTurn = null;
          }
        }
        if (data.done) entry.done = true;
      } catch { /* 网络瞬断：下一轮再试 */ }
    }

    if ([...this.#turns.values()].some((t) => !t.done)) {
      this.#pollTimer = setTimeout(() => this.#pollTick(), 250);
    }
  }
}

/** dsh 线协议宿主（骨架；未验证连接，仅示意对接面）。 */
export class DshHttpHost {
  name = 'dsh-wire';
  constructor(base) {
    this.base = base.replace(/\/$/, ''); // 如 http://127.0.0.1:8443
  }
  async #post(path, payload) {
    const res = await fetch(this.base + path, {
      method: 'POST',
      headers: { 'content-type': 'application/json' },
      body: JSON.stringify({ rpcId: crypto.randomUUID(), payload }),
    });
    if (!res.ok) throw new Error(`${path} → HTTP ${res.status}`);
    const envelope = await res.json();
    if (envelope.payload?.kind === 'error') throw new Error(envelope.payload.error.message);
    return envelope.payload?.value;
  }
  request(method, params) {
    return this.#post('/api', { kind: 'request', method, params });
  }
  respond(approvalId, decision) {
    return this.#post('/api/respond', { kind: 'respond', approvalId, decision });
  }
  /** 下行帧流：/api/events.mux。async 迭代器逐帧产出 payload。 */
  async *events(signal) {
    const url = new URL('/api/events.mux', this.base);
    url.protocol = url.protocol === 'https:' ? 'wss:' : 'ws:';
    const ws = new WebSocket(url);
    signal?.addEventListener('abort', () => ws.close(), { once: true });
    const queue = []; let wake = null; let ended = false;
    ws.onmessage = (e) => {
      const full = JSON.parse(e.data);
      queue.push(full.payload); wake?.(); wake = null;
    };
    ws.onclose = () => { ended = true; wake?.(); wake = null; };
    await new Promise((resolve, reject) => { ws.onopen = resolve; ws.onerror = reject; });
    while (true) {
      while (queue.length) yield queue.shift();
      if (ended) return;
      await new Promise((resolve) => { wake = resolve; });
    }
  }
}
