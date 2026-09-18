// ui.js — 对话视图层（结构对齐 XHarness dsh 原版）
//   用户气泡右对齐（MessageItem L-d22q）· 助手全宽 16/28（_5vpFrG）
//   Think 折叠行（ReasoningRow U8JO7q）· 工具内联行（Dh215a/ANgngG）
//   Composer 圆角卡 + 停靠面板（审批 rQ88rq / 待办 Xe4JHW / 排队 Q8gw3G / 统计 PvwIvW）
// 渲染纪律不变：节点级 v 版本 diff、details 折叠态迁移、事件委托。

import { renderMarkdown } from './markdown.js';
import { el } from './dom.js';
import { icon, fishLogo, stateDot } from './icons.js?v=3';
import {
  isSettingsView, closeSettings,
  toast, confirmModal, closeModal, promptModal, openSettings, openHelp,
  applySettings, exportSessionMarkdown, SLASH_COMMANDS, MOCK_FILES,
} from './chrome.js?v=14';

const fmtClock = (t) => new Date(t).toTimeString().slice(0, 5);
const fmtK = (n) => n >= 1000 ? (n / 1000).toFixed(1).replace(/\.0$/, '') + 'K' : String(n);
const fmtSec = (ms) => ms < 60000 ? `${Math.round(ms / 100) / 10}s` : `${Math.floor(ms / 60000)}m${Math.round(ms / 1000) % 60}s`;

// ============ 消息图标行（AEC1ra MessageIconActions）============
function msgActions(n, { clock = 'end', extra = [] } = {}) {
  const parts = [];
  const time = el('span', { class: 'msg-time ' + clock }, fmtClock(n.time));
  parts.push(clock === 'start' ? time : null);
  parts.push(el('button', {
    class: 'msg-action', type: 'button', 'data-tip': '复制',
    dataset: { act: 'copy-msg' }, 'aria-label': '复制',
  }, icon('IconCopyOutline16', { size: 16 })));
  for (const a of extra) parts.push(a);
  parts.push(clock === 'end' ? time : null);
  return el('div', { class: 'msg-actions', 'data-time-hover': '' }, ...parts);
}

// ============ 用户：右对齐气泡（L-d22q）============
function renderUserNode(n) {
  return el('div', { class: 'user-row flow-item', dataset: { id: n.id, v: n.v } },
    el('div', { class: 'user-stack' },
      n.images?.length ? el('div', { class: 'user-imgs' },
        n.images.map((img) => el('div', { class: 'user-img' },
          el('img', { src: img.dataUrl, alt: img.name ?? '附件' })))) : null,
      n.text ? el('div', { class: 'user-bubble' }, n.text) : null),
    msgActions(n, {
      clock: 'start',
      extra: [
        el('button', {
          class: 'msg-action', type: 'button', 'data-tip': '编辑重发',
          dataset: { act: 'edit-user' }, 'aria-label': '编辑重发',
        }, icon('IconEditOutline16', { size: 16 })),
      ],
    }));
}

// ============ 助手：全宽正文（_5vpFrG）============
function renderAssistantNode(n, keep, stats) {
  const parts = [];
  for (const b of n.blocks ?? []) {
    if (b.type === 'reasoning') parts.push(renderThink(b, n.streaming, keep));
    else if (b.type === 'image') parts.push(el('div', { class: 'msg-imgs' }, el('img', { src: b.dataUrl, alt: b.name ?? '图片' })));
    else parts.push(el('div', { class: 'md', html: renderMarkdown(b.text) }));
  }
  return el('div', { class: 'asst-row flow-item', dataset: { id: n.id, v: n.v } },
    el('div', { class: 'asst-body' }, ...parts),
    n.interrupted ? el('span', { class: 'asst-stopped' }, '已停止') : null,
    n.streaming ? null : msgActions(n, {
      extra: [
        el('button', { class: 'msg-action', type: 'button', 'data-tip': '重试本轮', dataset: { act: 'retry' }, 'aria-label': '重试本轮' }, icon('IconRefreshOutline16', { size: 16 })),
        el('button', { class: 'msg-action', type: 'button', 'data-tip': '点赞', dataset: { act: 'fb-good' }, 'aria-label': '点赞' }, icon('IconLikeOutline16', { size: 16 })),
        el('button', { class: 'msg-action', type: 'button', 'data-tip': '点踩', dataset: { act: 'fb-bad' }, 'aria-label': '点踩' }, icon('IconDislikeOutline16', { size: 16 })),
        stats ? renderStatsSpan(stats) : null,
      ],
    }));
}

/** 会话级统计（原独立统计行），悬停回复时显示在操作行尾。 */
function renderStatsSpan(st) {
  const groups = [];
  if (st.steps > 0) {
    groups.push(`${st.turns} 轮 · ${st.steps} 步`);
    const dur = [];
    if (st.llmMs > 0) dur.push(`LLM ${fmtSec(st.llmMs)}`);
    if (st.toolMs > 0) dur.push(`工具 ${fmtSec(st.toolMs)}`);
    if (dur.length) groups.push(dur.join(' · '));
    if (st.tps) groups.push(`${st.tps} tok/s`);
  }
  if (!groups.length) return null;
  return el('span', { class: 'msg-stats' },
    groups.flatMap((g, i) => i ? [el('span', { class: 'stats-sep' }, '|'), g] : [g]));
}

// ============ Think 折叠行（U8JO7q ReasoningRow）============
function renderThink(b, streaming, keep) {
  const text = b.text ?? '';
  const running = !!streaming;
  const firstLine = (t) => { const i = t.indexOf('\n'); return i === -1 ? t : t.slice(0, i); };
  const latestLine = (t) => { const v = t.trimEnd(); const i = v.lastIndexOf('\n'); return i === -1 ? v : v.slice(i + 1); };
  const open = running || !!keep?.thinkOpen;
  return el('div', { class: 'think-root', 'data-state': running ? 'running' : 'ok', 'data-open': open ? '' : null },
    el('div', { class: 'think-row', dataset: { act: 'toggle-think' } },
      el('span', { class: 'think-leading' }, icon('IconThinkOutline14', { size: 14 })),
      el('span', { class: 'think-title' }, 'Think'),
      el('span', { class: 'think-sep', 'aria-hidden': 'true' }),
      el('span', { class: 'think-summary' }, running ? latestLine(text) : firstLine(text)),
      el('span', { class: 'think-chevron' }, icon('IconTriangleRightFill14', { size: 12 }))),
    open ? el('div', { class: 'think-body' }, text) : null);
}

// ============ 工具内联行（Dh215a/ANgngG）============
const TOOL_LABEL = {
  bash: 'bash', read: 'read', write: 'write', edit: 'edit',
  search: 'search', glob: 'glob', fetch: 'fetch', task: 'task',
};
function toolSummaryText(n) {
  const a = n.args ?? {};
  switch (n.name) {
    case 'bash': return a.command ?? '';
    case 'read': case 'write': case 'edit': return a.path ?? '';
    case 'search': return `/${a.pattern ?? ''}/ in ${a.path ?? '.'}`;
    case 'glob': return a.pattern ?? '**/*';
    case 'fetch': return a.url ?? '';
    case 'task': return a.goal ?? '';
    default: return JSON.stringify(a);
  }
}
function toolLeadIcon(n) {
  if (n.status === 'running') return icon('IconLoadingOutline16', { size: 16 });
  if (n.status === 'error') return icon('IconWarningOutline16', { size: 16 });
  return icon('IconCheckOutline14', { size: 14 });
}
function ioCard(sections) {
  return el('div', { class: 'io-card' },
    sections.map((s, i) => [
      i > 0 ? el('div', { class: 'io-divider' }) : null,
      el('div', { class: 'io-section' },
        el('span', { class: 'io-label' }, s.label),
        el('span', { class: 'io-text', 'data-error': s.error ? '' : null }, s.text)),
    ]).flat());
}
function renderToolBody(n) {
  const out = [];
  if (n.name === 'task' && n.sub?.length) {
    out.push(el('div', { class: 'sub-calls' },
      n.sub.map((st) => el('div', {
        class: 'tool-root sub-call-row',
        'data-state': st.status === 'running' ? 'running' : 'ok',
      },
        el('div', { class: 'tool-row' },
          el('span', { class: 'tool-leading' },
            st.status === 'running' ? icon('IconLoadingOutline16', { size: 14 }) : icon('IconCheckOutline14', { size: 12 })),
          el('span', { class: 'tool-title' }, 'step'),
          el('span', { class: 'tool-sep' }),
          el('span', { class: 'tool-summary' }, st.name))))));
  }
  if (n.diff) {
    out.push(el('div', { class: 'diff-body' },
      n.diff.map((l) => el('span', { class: 'diff-line ' + ({ '+': 'add', '-': 'del' }[l[0]] ?? 'ctx') }, l))));
  } else if (n.result != null && n.result !== '') {
    if (n.name === 'bash') {
      out.push(el('div', { class: 'term-body' }, el('pre', { 'data-error': n.status === 'error' ? '' : null }, String(n.result))));
    } else if (n.name === 'read') {
      out.push(el('div', { class: 'term-body' }, el('pre', null, String(n.result))));
    } else {
      out.push(ioCard([{ label: n.name === 'search' ? 'matches' : 'output', text: String(n.result), error: n.status === 'error' }]));
    }
  } else if (n.status === 'running') {
    out.push(el('div', { class: 'io-card' }, el('div', { class: 'io-section' }, el('span', { class: 'io-label' }, '···'))));
  }
  return out;
}
function renderToolNode(n, keep) {
  const open = n.status === 'running' || keep?.toolOpen;
  const summary = n.status === 'running' ? toolSummaryText(n) : toolSummaryText(n);
  const suffix = n.status !== 'running' && n.ms != null ? ` · ${fmtSec(n.ms)}` : '';
  return el('div', {
    class: 'tool-root', 'data-state': n.status === 'running' ? 'running' : 'ok',
    'data-open': open ? '' : null,
    'data-variant': n.name === 'task' ? 'cordis' : null,
    dataset: { id: n.id, v: n.v },
  },
    el('div', { class: 'tool-row', dataset: { act: 'toggle-tool' } },
      el('span', { class: 'tool-leading' }, toolLeadIcon(n)),
      el('span', { class: 'tool-title' }, TOOL_LABEL[n.name] ?? n.name),
      el('span', { class: 'tool-sep', 'aria-hidden': 'true' }),
      el('span', { class: 'tool-summary' + (n.status === 'error' ? ' error' : '') },
        summary + suffix),
      el('span', { class: 'tool-chevron' }, icon('IconTriangleRightFill14', { size: 12 }))),
    open ? el('div', { class: 'tool-body-wrap' }, ...renderToolBody(n)) : null);
}

function renderNode(n, keep, stats) {
  if (n.kind === 'user') return renderUserNode(n);
  if (n.kind === 'tool') return renderToolNode(n, keep);
  return renderAssistantNode(n, keep, stats);
}
function nodeText(n) {
  if (n.kind === 'user') return n.text ?? '';
  if (n.kind === 'assistant') return (n.blocks ?? []).map((b) => b.type === 'text' ? b.text : '[图片]').join('\n');
  return `${n.name} ${n.result ?? ''}`;
}
function captureKeep(row) {
  return {
    thinkOpen: !!row.querySelector('.think-root[data-open]'),
    toolOpen: !!row.querySelector('.tool-root[data-open]'),
  };
}

// ============ 停靠面板 ============
function renderDocks(root, store, host, s) {
  const parts = [];
  // 审批（rQ88rq）
  for (const a of s.approvals.values()) {
    parts.push(el('div', { class: 'appr-root' },
      el('div', { class: 'appr-card' },
        el('div', { class: 'appr-strip' },
          el('span', { class: 'appr-dot' }),
          a.title),
        el('div', { class: 'appr-body' },
          el('div', { class: 'appr-headline' }, '需要你的批准'),
          el('div', { class: 'appr-command' }, a.detail ?? '')),
        el('div', { class: 'appr-actions' },
          el('button', { class: 'dsw-btn danger', onclick: () => host.respondApproval(a.id, 'rejected') }, '拒绝'),
          el('button', { class: 'dsw-btn', onclick: () => host.respondApproval(a.id, 'allow-session') }, '本会话均允许'),
          el('button', { class: 'dsw-btn primary', onclick: () => host.respondApproval(a.id, 'allow-once') }, '允许一次')))));
  }
  // 待办（Xe4JHW）
  const tp = s.todoPanel;
  if (tp && tp.items.length) {
    const done = tp.items.filter((x) => x.done).length;
    parts.push(el('div', { class: 'todo-root', 'data-open': tp.open ? '' : null },
      el('div', { class: 'todo-body' },
        el('button', { class: 'todo-header', dataset: { act: 'toggle-todo' } },
          el('span', { class: 'todo-lead' },
            done === tp.items.length ? icon('IconCheckOutline14', { size: 14 }) : icon('IconLoadingOutline16', { size: 14 })),
          el('span', { class: 'todo-title' }, '计划'),
          el('span', { class: 'todo-progress' }, `${done}/${tp.items.length}`),
          el('span', { class: 'todo-chevron' }, icon('IconTriangleRightFill14', { size: 12 }))),
        tp.open ? el('ul', { class: 'todo-list' },
          tp.items.map((it, i) => el('li', { class: 'todo-item' + (it.done ? ' done' : '') },
            el('span', { class: 'todo-glyph ' + (it.done ? 'done' : i === done ? 'running' : 'pending') },
              it.done ? icon('IconCheckOutline14', { size: 14 }) : icon('IconQueueOutline14', { size: 14 })),
            el('span', { class: 'todo-content' }, it.text)))) : null)));
  }
  // 排队（Q8gw3G）
  if (s.queue.length) {
    const open = s._queueOpen !== false;
    parts.push(el('div', { class: 'queue-dock', 'data-open': open ? '' : null },
      el('div', { class: 'queue-panel' },
        el('button', { class: 'queue-header', dataset: { act: 'toggle-queue' } },
          icon('IconQueueOutline14', { size: 14 }),
          el('span', { class: 'queue-count' }, `排队 ${s.queue.length} 条`),
          el('span', { class: 'queue-chevron' }, icon('IconTriangleRightFill14', { size: 12 }))),
        open ? el('ul', { class: 'queue-list' },
          s.queue.map((q, i) => el('li', { class: 'queue-row' },
            el('span', { class: 'queue-preview' },
              q.parts.map((p) => p.type === 'text' ? p.text : '[图片]').join(' ')),
            el('button', {
              class: 'msg-action queue-x', 'data-tip': '移除',
              dataset: { act: 'drop-queue', idx: i },
            }, icon('IconCloseFill14', { size: 14 }))))) : null)));
  }
  root.replaceChildren(...parts);
}


// ============ Composer（_7yzX1q）============
let composerInput = null;
export function focusComposer() { composerInput?.focus(); }
const estTokens = (t) => {
  const cjk = (t.match(/[\u4e00-\u9fff\u3000-\u303f]/g) ?? []).length;
  return Math.round(cjk + (t.length - cjk) / 4);
};

/** 自绘下拉（替代原生 select：弹层配色可控，向上弹出避开视口底）。 */
function dropdown({ aria }) {
  const btn = el('button', { class: 'cmp-select', type: 'button', 'aria-label': aria });
  const menu = el('div', { class: 'dropdown cmp-dd' });
  const root = el('div', { class: 'cmp-dd-root' }, btn, menu);
  const state = { options: [], value: null, open: false };
  let onChange = null;
  const setOpen = (v) => { state.open = v; menu.classList.toggle('open', v); if (v) renderMenu(); };
  const renderMenu = () => menu.replaceChildren(...state.options.map((o) => el('button', {
    class: state.value === o.v ? 'on' : '',
    onclick: () => { setOpen(false); if (state.value !== o.v) { state.value = o.v; renderBtn(); onChange && onChange(o.v); } },
  }, el('span', null, o.label), o.sub ? el('span', { class: 'sub' }, o.sub) : null)));
  const renderBtn = () => {
    const cur = state.options.find((o) => o.v === state.value);
    btn.replaceChildren(cur ? cur.label : '', icon('IconChevronDownOutline14', { size: 12, className: 'model-pill-chevron' }));
  };
  btn.addEventListener('click', (e) => { e.stopPropagation(); setOpen(!state.open); });
  menu.addEventListener('click', (e) => e.stopPropagation());
  document.addEventListener('click', () => setOpen(false));
  return {
    root,
    set(options, value) { state.options = options; state.value = value; renderBtn(); if (state.open) renderMenu(); },
    onChange(fn) { onChange = fn; },
  };
}

/** 上下文容量圆环（模型选择器旁）：悬停显示分段条与分类占比。 */
function contextMeter(store) {
  const svgNS = 'http://www.w3.org/2000/svg';
  const root = el('div', { class: 'meter ctx-meter' });
  const btn = el('button', { class: 'meter-trigger', type: 'button', 'aria-label': '上下文容量' });
  const panel = el('div', { class: 'meter-panel' });
  root.append(btn, panel);
  const R = 9, CIRC = 2 * Math.PI * R;

  function ring(pct) {
    const svg = document.createElementNS(svgNS, 'svg');
    svg.setAttribute('viewBox', '0 0 22 22');
    svg.setAttribute('width', '22'); svg.setAttribute('height', '22');
    const mk = (cls) => {
      const c = document.createElementNS(svgNS, 'circle');
      c.setAttribute('cx', '11'); c.setAttribute('cy', '11'); c.setAttribute('r', String(R));
      c.setAttribute('class', cls);
      return c;
    };
    const fill = mk('meter-fill');
    fill.setAttribute('stroke-dasharray', `${(CIRC * pct).toFixed(1)} ${CIRC.toFixed(1)}`);
    fill.setAttribute('transform', 'rotate(-90 11 11)');
    svg.append(mk('meter-track'), fill);
    return svg;
  }

  function update() {
    const s = store.snapshot().selected;
    const win = store.model?.contextWindow ?? 128000;
    const used = Math.min(win, s?.ctxTokens ?? 0);
    btn.replaceChildren(ring(used / win));
    // 分类估算：消息（对话文本）/ 工具结果 / 系统提示词 / 其他
    // CJK 感知粗估：中日韩字符 ≈1 token，其余 ≈4 字符/token
    const tok = (s2) => {
      if (!s2) return 0;
      const cjk = (s2.match(/[一-鿿　-〿]/g) ?? []).length;
      return Math.round(cjk + (s2.length - cjk) / 4);
    };
    let msg = 0, tool = 0;
    for (const n of s?.nodes ?? []) {
      if (n.kind === 'user') msg += tok(n.text) + 600 * (n.images?.length || 0);
      else if (n.kind === 'assistant') for (const b of n.blocks ?? []) if (b.type === 'text') msg += tok(b.text);
      else if (n.kind === 'tool') tool += tok(n.result);
    }
    const sys = 3200 + (store.settings.systemPrompt || '').length;
    // 三类实算内容按比例缩放填满已用量；差额并入「其他」
    const raw = Math.max(1, msg + tool + sys);
    const scale = used / raw;
    const parts = [
      ['消息', Math.round(msg * scale), 'var(--dsw-static-blue-450)'],
      ['工具结果', Math.round(tool * scale), '#a78bfa'],
      ['系统提示词', Math.round(sys * scale), 'var(--dsw-static-green-500)'],
      ['其他', Math.max(0, used - Math.round(msg * scale) - Math.round(tool * scale) - Math.round(sys * scale)), 'var(--dsw-alias-label-caption)'],
    ];
    const sum = Math.max(1, parts.reduce((a, p) => a + p[1], 0));
    panel.replaceChildren(
      el('div', { class: 'meter-header' },
        el('span', { class: 'meter-headline' }, '上下文容量'),
        el('span', { class: 'meter-figures' }, `${fmtK(used)} / ${fmtK(win)}（${Math.round(used / win * 100)}%）`)),
      el('div', { class: 'meter-bar' },
        ...parts.map(([label, tok, color]) => el('span', {
          class: 'meter-segment',
          style: `background:${color};flex:none;width:${Math.max(1.5, tok / sum * 100).toFixed(1)}%`,
        }))),
      el('div', { class: 'ctx-rows' },
        ...parts.map(([label, tok, color]) => el('div', { class: 'ctx-row' },
          el('span', { class: 'ctx-dot', style: `background:${color}` }),
          el('span', null, label),
          el('span', { class: 'ctx-pct' }, Math.round(tok / sum * 100) + '%')))));
  }
  store.notifier.subscribe(update);
  update();
  return root;
}

function mountComposer(root, store, host) {
  let attachments = [];
  const menu = { open: false, kind: null, items: [], idx: 0, start: 0 };

  const card = el('div', { class: 'cmp-card' });
  const menuEl = el('div', { class: 'cmp-menu' });
  const attachRow = el('div', { class: 'cmp-attach' });
  const inputEl = composerInput = el('textarea', {
    class: 'cmp-textarea', rows: '1',
    placeholder: '给 agent 发消息…（/ 命令 · @ 文件 · 可拖入或粘贴图片）',
  });
  const tokenEst = el('span', { class: 'cmp-token' });
  const addBtn = el('button', { class: 'cmp-add', type: 'button', 'data-tip': '添加图片附件' }, icon('IconPaperclipOutline16', { size: 16 }));
  const sendBtn = el('button', { class: 'cmp-send', type: 'button', 'aria-label': '发送' }, icon('IconSendOutline14', { size: 16 }));
  const fileInput = el('input', { type: 'file', accept: 'image/*', multiple: '', style: 'display:none' });
  const EFFORT_OPTS = [
    { v: 'off', label: '思考 关' }, { v: 'low', label: '思考 低' },
    { v: 'medium', label: '思考 中' }, { v: 'high', label: '思考 高' }];
  const POLICY_OPTS = [{ v: 'ask', label: '审批 询问' }, { v: 'auto', label: '审批 自动' }];

  const modelDD = dropdown({ aria: '模型' });
  modelDD.onChange((id) => {
    const m = store.models.find((x) => x.id === id);
    if (m) { store.model = m; store.notifier.markDirty(); }
  });
  const effortDD = dropdown({ aria: '思考力度' });
  effortDD.onChange((v) => { host.effort = v; toast('思考力度：' + v); });
  const policyDD = dropdown({ aria: '审批策略' });
  policyDD.onChange((v) => {
    const s = store.snapshot().selected;
    if (s) { s.policy = v; store.notifier.markDirty(); }
  });

  function syncSelects() {
    const s = store.snapshot().selected;
    modelDD.set(store.models.map((m) => ({ v: m.id, label: m.name })), store.model?.id);
    effortDD.set(EFFORT_OPTS, host.effort ?? 'high');
    policyDD.set(POLICY_OPTS, s ? s.policy : 'ask');
  }

  function addFiles(files) {
    for (const f of files ?? []) {
      if (!f.type.startsWith('image/')) { toast(`跳过非图片文件：${f.name}`); continue; }
      if (f.size > 2 * 1024 * 1024) { toast(`图片过大（>2MB）：${f.name}`); continue; }
      const reader = new FileReader();
      reader.onload = () => { attachments.push({ dataUrl: reader.result, name: f.name }); syncAttach(); };
      reader.readAsDataURL(f);
    }
  }
  function syncAttach() {
    attachRow.replaceChildren(...attachments.map((a, i) =>
      el('div', { class: 'attach-chip' },
        el('span', { class: 'attach-thumb' }, el('img', { src: a.dataUrl, alt: a.name })),
        el('span', { class: 'attach-name' }, a.name),
        el('button', { class: 'attach-x', onclick: () => { attachments.splice(i, 1); syncAttach(); } }, '✕'))));
    card.classList.toggle('has-attach', attachments.length > 0);
  }
  fileInput.addEventListener('change', () => { addFiles(fileInput.files); fileInput.value = ''; });
  inputEl.addEventListener('paste', (e) => {
    const imgs = [...(e.clipboardData?.items ?? [])].filter((it) => it.type.startsWith('image/'));
    if (imgs.length) { e.preventDefault(); addFiles(imgs.map((it) => it.getAsFile()).filter(Boolean)); }
  });
  card.addEventListener('dragover', (e) => { e.preventDefault(); card.classList.add('drag'); });
  card.addEventListener('dragleave', () => card.classList.remove('drag'));
  card.addEventListener('drop', (e) => { e.preventDefault(); card.classList.remove('drag'); addFiles(e.dataTransfer?.files); });

  function closeMenu() { menu.open = false; menuEl.classList.remove('open'); }
  function renderMenu() {
    if (!menu.open) return;
    menuEl.replaceChildren(...menu.items.map((it, i) => el('div', {
      class: 'menu-item' + (i === menu.idx ? ' sel' : ''),
      onmousedown: (e) => { e.preventDefault(); pickMenuItem(); },
    }, el('code', null, it.label), el('span', { class: 'menu-desc' }, it.desc ?? ''))));
    menuEl.classList.add('open');
  }
  function updateMenu() {
    const v = inputEl.value;
    const upto = v.slice(0, inputEl.selectionStart ?? v.length);
    const slash = v.startsWith('/') && !v.includes(' ') ? v : null;
    const atM = upto.match(/(?:^|\s)@(\S*)$/);
    if (slash) {
      const hits = SLASH_COMMANDS.filter((c) => c.cmd.startsWith(slash));
      if (hits.length && hits.some((h) => h.cmd !== slash)) {
        Object.assign(menu, { open: true, kind: 'slash', items: hits.map((h) => ({ label: h.cmd, desc: h.desc, run: h.cmd })), idx: 0, start: 0 });
        renderMenu(); return;
      }
    }
    if (atM && !slash) {
      const q = atM[1].toLowerCase();
      const hits = MOCK_FILES.filter((f) => f.toLowerCase().includes(q));
      if (hits.length) {
        Object.assign(menu, { open: true, kind: 'file', items: hits.slice(0, 8).map((f) => ({ label: '@' + f, run: f })), idx: 0, start: upto.length - atM[1].length - 1 });
        renderMenu(); return;
      }
    }
    closeMenu();
  }
  function pickMenuItem() {
    const it = menu.items[menu.idx];
    if (!it) return;
    if (menu.kind === 'slash') { inputEl.value = ''; closeMenu(); execSlash(it.run); }
    else {
      const v = inputEl.value;
      const pos = inputEl.selectionStart ?? v.length;
      inputEl.value = v.slice(0, menu.start) + it.run + ' ' + v.slice(pos);
      closeMenu();
      inputEl.focus();
      inputEl.setSelectionRange(inputEl.value.length, inputEl.value.length);
    }
    autoGrow(); syncEst(); saveDraft();
  }
  function execSlash(cmd) {
    const sid = store.selectedId;
    switch (cmd) {
      case '/demo': host.runShowcase(sid); break;
      case '/image': host.runMultimodal(sid); break;
      case '/model': {
        const i = store.models.findIndex((m) => m.id === store.model?.id);
        store.model = store.models[(i + 1) % store.models.length];
        store.notifier.markDirty(); syncSelects();
        toast('模型已切换：' + store.model.name);
        break;
      }
      case '/theme':
        store.settings.theme = store.settings.theme === 'dark' ? 'light' : 'dark';
        applySettings(store); store.persist();
        break;
      case '/export': exportSessionMarkdown(store, store.sessions.get(sid)); break;
      case '/clear':
        confirmModal({
          title: '清空会话', message: '当前会话的全部消息将被清除（其他会话不受影响）。',
          okLabel: '清空',
          onOk: () => { store.truncate(sid, 0); store.persist(); toast('会话已清空'); },
        });
        break;
      case '/settings': openSettings(store); break;
      case '/help': openHelp(); break;
      default: toast('未知命令：' + cmd);
    }
  }
  function parts() {
    return [
      ...(inputEl.value.trim() ? [{ type: 'text', text: inputEl.value.trim() }] : []),
      ...attachments.map((a) => ({ type: 'image', dataUrl: a.dataUrl, name: a.name })),
    ];
  }
  function send() {
    const s = store.snapshot().selected;
    if (!s) return;
    if (!inputEl.value.trim() && !attachments.length) return;
    const t = inputEl.value.trim();
    if (t.startsWith('/') && SLASH_COMMANDS.some((c) => c.cmd === t)) {
      inputEl.value = ''; attachments = []; syncAttach(); autoGrow(); syncEst(); saveDraft();
      execSlash(t); return;
    }
    if (s.running) {
      s.queue.push({ parts: parts() });
      inputEl.value = ''; attachments = []; syncAttach(); autoGrow(); syncEst(); saveDraft();
      store.notifier.markDirty();
      toast('已加入队列，本轮结束后自动发送');
      return;
    }
    const p = parts();
    inputEl.value = ''; attachments = []; syncAttach(); autoGrow(); syncEst(); saveDraft();
    host.prompt(s.id, p);
  }
  const autoGrow = () => {
    inputEl.style.height = 'auto';
    inputEl.style.height = Math.min(336, inputEl.scrollHeight) + 'px';
  };
  const syncEst = () => { tokenEst.textContent = inputEl.value ? `~${estTokens(inputEl.value)} tok` : ''; };
  const saveDraft = () => { const sid = store.selectedId; if (sid) store.setDraft(sid, inputEl.value); };
  inputEl.addEventListener('input', () => { autoGrow(); syncEst(); saveDraft(); updateMenu(); });
  inputEl.addEventListener('keydown', (e) => {
    if (menu.open) {
      if (e.key === 'ArrowDown') { e.preventDefault(); menu.idx = (menu.idx + 1) % menu.items.length; renderMenu(); return; }
      if (e.key === 'ArrowUp') { e.preventDefault(); menu.idx = (menu.idx - 1 + menu.items.length) % menu.items.length; renderMenu(); return; }
      if (e.key === 'Enter' || e.key === 'Tab') { e.preventDefault(); pickMenuItem(); return; }
      if (e.key === 'Escape') { e.preventDefault(); closeMenu(); return; }
    }
    if (e.key === 'Enter' && !e.shiftKey) { e.preventDefault(); send(); }
  });

  card.append(menuEl, attachRow, inputEl,
    el('div', { class: 'cmp-row' },
      addBtn,
      el('div', { class: 'cmp-tools' }, policyDD.root),
      el('div', { class: 'cmp-trailing' }, tokenEst, contextMeter(store), modelDD.root, effortDD.root, sendBtn)));
  root.replaceChildren(el('div', { class: 'cmp-stack' }, card), fileInput);

  root._sync = () => {
    const s = store.snapshot().selected;
    if (!s) return;
    syncSelects();
    if (s.running) {
      sendBtn.classList.add('stop');
      sendBtn.replaceChildren(icon('IconStopFill16', { size: 16 }));
      sendBtn.onclick = () => host.cancel(store.selectedId);
    } else {
      sendBtn.classList.remove('stop');
      sendBtn.replaceChildren(icon('IconSendOutline14', { size: 16 }));
      sendBtn.onclick = send;
    }
  };
  addBtn.onclick = () => fileInput.click();
  root._sync();
  root._restoreDraft = (sid) => {
    inputEl.value = store.drafts.get(sid) ?? '';
    attachments = []; syncAttach(); autoGrow(); syncEst(); closeMenu();
  };
}

// ============ Hero（旧版空状态：居中标识 + 入口卡片）============
function renderHero(listEl, store, host) {
  if (listEl.dataset.hero === '1') return;
  listEl.dataset.hero = '1';
  const chip = (title, sub, fn) => el('div', { class: 'chip', onclick: fn },
    el('div', { class: 'c-title' }, title), el('div', { class: 'c-sub' }, sub));
  listEl.replaceChildren(
    el('div', { class: 'empty-state' },
      el('div', { class: 'empty-logo' }, fishLogo({ size: 56 })),
      el('div', { class: 'empty-title' }, '开始一次会话'),
      el('div', { class: 'empty-sub' }, '原生实现的 agent 前端 —— 事件溯源会话 · 帧合并渲染 · 工具卡 · 审批闸门 · 多模态'),
      el('div', { class: 'chips' },
        chip('▶　运行完整演示', '多工具调用 · 文件修改审批 · diff · 计划 · 汇总报表', () => host.runShowcase(store.selectedId)),
        chip('🖼　多模态演示', '贴图分析 · todo 实时推进 · 子代理回归 · 出对比图', () => host.runMultimodal(store.selectedId)),
        chip('✎　写一个正则工具函数', '转义与整词匹配，附 TypeScript 代码', () => host.prompt(store.selectedId, [{ type: 'text', text: '写一个正则工具函数，要求安全转义' }])),
        chip('⚠　解释一个报错', '实测取证 → 自我修正 → 复测通过', () => host.prompt(store.selectedId, [{ type: 'text', text: '构建报错了，帮我看看是什么错误' }])))));
}

// ============ 总装 ============
export function mountApp(store, host, refs) {
  const { chatScroll, chatList, docks, composerWrap, cvRoot, composerSeat } = refs;

  // 会话内搜索
  const find = { open: false, q: '' };
  const findBar = el('div', { class: 'find-bar' },
    el('input', { type: 'search', placeholder: '在会话内搜索…' }),
    el('span', { class: 'find-count' }),
    el('button', { class: 'icon-btn', 'data-tip': '关闭 Esc' }, icon('IconCloseFill14', { size: 14 })));
  findBar.querySelector('.icon-btn').onclick = closeFind;
  findBar.querySelector('input').addEventListener('input', (e) => {
    find.q = e.target.value.trim().toLowerCase();
    store.notifier.markDirty();
  });
  function openFind() { find.open = true; findBar.classList.add('open'); findBar.querySelector('input').focus(); }
  function closeFind() { find.open = false; find.q = ''; findBar.classList.remove('open'); store.notifier.markDirty(); focusComposer(); }

  // 回到底部（to-bottom-slot 常驻 sticky 槽）
  const toBottom = el('button', { class: 'to-bottom' }, icon('IconChevronDownOutline14', { size: 14 }));
  toBottom.onclick = () => { chatScroll.scrollTop = chatScroll.scrollHeight; };
  const toBottomSlot = el('div', { class: 'to-bottom-slot' }, toBottom);
  chatScroll.parentElement.append(findBar);

  // composer 高度发布（浮动件让位，dsh --dsh-composer-height 同义）
  const seatObserver = new ResizeObserver(() => {
    chatScroll.style.setProperty('--composer-height', composerSeat.offsetHeight + 'px');
  });
  seatObserver.observe(composerSeat);

  mountComposer(composerWrap, store, host);

  // 事件委托：复制 / 折叠切换 / 重试 / 编辑 / 反馈 / 排队移除
  // （对话行在 chatList，停靠面板在 docks —— 两处共用同一处理器）
  const onAct = (e) => {
    const copyBtn = e.target.closest('[data-copy-id]');
    if (copyBtn) {
      const pre = document.getElementById(copyBtn.dataset.copyId);
      if (pre) navigator.clipboard?.writeText(pre.textContent).then(() => {
        copyBtn.textContent = '已复制';
        setTimeout(() => { copyBtn.textContent = '复制'; }, 1200);
      });
      return;
    }
    const act = e.target.closest('[data-act]');
    if (!act) return;
    const s = store.snapshot().selected;
    if (!s) return;
    const kind = act.dataset.act;
    const row = act.closest('[data-id]');
    const node = row ? s.nodes.find((n) => n.id === row.dataset.id) : null;
    switch (kind) {
      case 'toggle-think': {
        const r = act.closest('.think-root');
        if (r) r.toggleAttribute('data-open');
        break;
      }
      case 'toggle-tool': {
        const r = act.closest('.tool-root');
        if (r) r.toggleAttribute('data-open');
        break;
      }
      case 'toggle-todo': {
        if (s.todoPanel) { s.todoPanel.open = !s.todoPanel.open; store.notifier.markDirty(); }
        break;
      }
      case 'toggle-queue': {
        s._queueOpen = s._queueOpen === false; store.notifier.markDirty();
        break;
      }
      case 'drop-queue': {
        s.queue.splice(+act.dataset.idx ?? 0, 1); store.notifier.markDirty();
        break;
      }
      case 'copy-msg':
        if (node) navigator.clipboard?.writeText(nodeText(node)).then(() => toast('已复制'));
        break;
      case 'retry':
        if (!s.running) host.retry(s.id);
        else toast('运行中，先停止再重试');
        break;
      case 'edit-user': {
        if (s.running) { toast('运行中不能编辑历史消息'); break; }
        const seq = +node.id.slice(1);
        promptModal({
          title: '编辑并重发', label: '修改这条消息后将重跑该轮（其后的回复会被替换）', value: node.text ?? '',
          onOk: (v) => host.editUserMessage(s.id, seq, v),
        });
        break;
      }
      case 'fb-good': act.classList.add('on'); act.nextElementSibling?.classList.remove('on'); break;
      case 'fb-bad': act.classList.add('on'); act.previousElementSibling?.classList.remove('on'); break;
    }
  };
  chatList.addEventListener('click', onAct);
  docks.addEventListener('click', onAct);

  // 全局快捷键
  document.addEventListener('keydown', (e) => {
    const inInput = /input|textarea|select/i.test(e.target.tagName);
    if (e.key === 'Escape') {
      if (document.querySelector('.overlay')) { closeModal(); return; }
      if (isSettingsView()) { closeSettings(store); return; }
      if (find.open) { closeFind(); return; }
      const s = store.snapshot().selected;
      if (s?.running) { host.cancel(s.id); toast('已请求中断'); }
      return;
    }
    const k = e.key.toLowerCase();
    if ((e.ctrlKey || e.metaKey) && k === 'k') { e.preventDefault(); window.__app.newSession(); return; }
    if ((e.ctrlKey || e.metaKey) && k === 'f') { e.preventDefault(); openFind(); return; }
    if ((e.ctrlKey || e.metaKey) && k === 'e') { e.preventDefault(); exportSessionMarkdown(store, store.snapshot().selected); return; }
    if ((e.ctrlKey || e.metaKey) && e.key === ',') { e.preventDefault(); openSettings(store); return; }
    if ((e.ctrlKey || e.metaKey) && k === 'j') {
      e.preventDefault();
      store.settings.theme = store.settings.theme === 'dark' ? 'light' : 'dark';
      applySettings(store); store.persist(); return;
    }
    if (e.key === '?' && !inInput) { e.preventDefault(); openHelp(); return; }
  });
  window.addEventListener('focus', () => { document.title = 'NativeHarness — agent 前端'; });

  let lastSid = null;
  function render() {
    const settingsOn = isSettingsView();
    cvRoot.hidden = settingsOn;
    if (settingsOn) return;
    composerWrap._sync?.();
    const s = store.snapshot().selected;
    if (!s) return;
    if (s.id !== lastSid) { lastSid = s.id; composerWrap._restoreDraft?.(s.id); }

    // 阶段：hero（空） / active（有内容）
    const phase = s.nodes.length === 0 && !s.lastError ? 'hero' : 'active';
    if (cvRoot.dataset.phase !== phase) {
      cvRoot.dataset.phase = phase;
      if (phase === 'hero') chatList.replaceChildren();
    }
    composerWrap.querySelector('.cmp-stack')?.classList.toggle('hero', phase === 'hero');

    // 排队派发
    if (!s.running && !s.approvals.size && s.queue.length && !s._queueBusy) {
      const item = s.queue.shift();
      s._queueBusy = true;
      store.notifier.markDirty();
      host.prompt(s.id, item.parts).finally(() => { s._queueBusy = false; store.notifier.markDirty(); });
      return;
    }

    // hero 分支
    if (phase === 'hero') { renderHero(chatList, store, host); renderDocks(docks, store, host, s); return; }
    if (chatList.dataset.hero === '1') { chatList.dataset.hero = '0'; chatList.replaceChildren(); }

    const pinned = chatScroll.scrollHeight - chatScroll.scrollTop - chatScroll.clientHeight < 90;

    // 错误横幅
    const banner = chatList.querySelector('.error-banner');
    if (s.lastError && !banner) {
      chatList.prepend(el('div', { class: 'error-banner' },
        el('span', null, `⚠ ${s.lastError.message}`),
        el('button', { class: 'icon-btn', onclick: () => { s.lastError = null; store.notifier.markDirty(); } }, icon('IconCloseFill14', { size: 14 }))));
    } else if (!s.lastError && banner) banner.remove();

    // 节点对账（流列结构：error-banner + flow-scroll > flow-column > rows + toBottomSlot）
    let scroll = chatList.querySelector('.flow-scroll');
    if (!scroll) {
      scroll = el('div', { class: 'flow-scroll' }, el('div', { class: 'flow-column' }));
      chatList.append(scroll);
    }
    const col = scroll.querySelector('.flow-column');
    const q = find.open && find.q ? find.q : null;
    let hits = 0;
    const nodes = s.nodes;
    let lastAsstId = null;
    for (let i = nodes.length - 1; i >= 0; i--) {
      if (nodes[i].kind === 'assistant') { lastAsstId = nodes[i].id; break; }
    }
    const rows = [...col.children].filter((c) => c.classList?.contains('flow-item'));
    for (let i = 0; i < nodes.length; i++) {
      const n = nodes[i];
      let cur = rows[i];
      if (!cur || cur.dataset.id !== n.id || +cur.dataset.v !== n.v) {
        const keep = cur?.dataset.id === n.id ? captureKeep(cur) : undefined;
        const fresh = renderNode(n, keep, n.id === lastAsstId ? (s.statsAcc ?? null) : null);
        if (cur) { cur.replaceWith(fresh); rows[i] = fresh; }
        else { col.append(fresh); rows.push(fresh); }
        cur = fresh;
      }
      if (q) {
        const hit = nodeText(n).toLowerCase().includes(q);
        cur.classList.toggle('find-hide', !hit);
        if (hit) hits++;
      } else cur.classList.remove('find-hide');
    }
    while (rows.length > nodes.length) rows.pop()?.remove();
    if (!col.contains(toBottomSlot)) col.append(toBottomSlot);
    if (find.open) findBar.querySelector('.find-count').textContent = q ? `${hits} 处匹配` : '';

    renderDocks(docks, store, host, s);
    if (pinned) chatScroll.scrollTop = chatScroll.scrollHeight;
  }

  store.notifier.subscribe(render);
  render();
  applySettings(store);
}
