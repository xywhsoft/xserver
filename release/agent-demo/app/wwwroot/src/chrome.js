// chrome.js — 应用外壳（对齐 XHarness：AppFrame 三列 / SidebarRoot / ChatView 头部 / DetailsPanel）
// 与 ui.js 单向依赖：本模块只引 dom.js/icons.js；动作经 mountChrome 的 hooks 注入。

import { el } from './dom.js';
import { icon, fishLogo, stateDot } from './icons.js?v=3';

// —— @ 文件补全的 mock 工作区（模板项：接后端换成 workspace/listFiles）——
export const MOCK_FILES = [
  'src/app.ts', 'src/core/engine.ts', 'src/cli/index.ts', 'src/dashboard/chart.ts',
  'scripts/build.mjs', 'package.json', 'tsconfig.json', 'tests/engine.test.ts', 'css/app.css', 'README.md',
];

// —— 斜杠命令表（数据；执行在 ui.js 的 composer）——
export const SLASH_COMMANDS = [
  { cmd: '/demo', desc: '运行完整演示（多工具 + 审批 + diff）' },
  { cmd: '/image', desc: '多模态演示（贴图分析 + 子代理 + 出图）' },
  { cmd: '/model', desc: '切换模型' },
  { cmd: '/theme', desc: '切换深/浅主题' },
  { cmd: '/export', desc: '导出当前会话为 Markdown' },
  { cmd: '/clear', desc: '清空当前会话' },
  { cmd: '/settings', desc: '打开设置' },
  { cmd: '/help', desc: '快捷键与命令帮助' },
];

// ============ Toast ============
let toastRoot = null;
export function toast(msg) {
  if (!toastRoot) { toastRoot = el('div', { class: 'toast-root' }); document.body.append(toastRoot); }
  const t = el('div', { class: 'toast' }, msg);
  toastRoot.append(t);
  setTimeout(() => { t.classList.add('out'); setTimeout(() => t.remove(), 300); }, 2400);
}

// ============ 弹窗系统 ============
let overlayEl = null;
export function closeModal() { overlayEl?.remove(); overlayEl = null; }
export function openModal({ title, body, actions = [{ label: '关闭' }], wide = false }) {
  closeModal();
  overlayEl = el('div', { class: 'overlay', onclick: (e) => { if (e.target === overlayEl) closeModal(); } },
    el('div', { class: 'modal' + (wide ? ' wide' : '') },
      el('div', { class: 'modal-head' },
        el('span', { class: 'modal-title' }, title),
        el('button', { class: 'modal-x', onclick: closeModal }, icon('IconCloseFill14', { size: 14 }))),
      el('div', { class: 'modal-body' }, body),
      actions.length ? el('div', { class: 'modal-foot' },
        actions.map((a) => el('button', {
          class: 'dsw-btn' + (a.primary ? ' primary' : '') + (a.danger ? ' danger' : ''),
          onclick: () => { if (a.onClick?.() !== true) closeModal(); },
        }, a.label))) : null));
  document.body.append(overlayEl);
  return overlayEl;
}
export function confirmModal({ title, message, okLabel = '删除', onOk }) {
  openModal({
    title,
    body: el('div', { class: 'confirm-body' }, message),
    actions: [{ label: '取消' }, { label: okLabel, danger: true, onClick: onOk }],
  });
}
export function promptModal({ title, label, value = '', onOk }) {
  const input = el('input', { class: 'modal-input', value });
  const submit = () => { if (input.value.trim()) { onOk(input.value.trim()); closeModal(); } };
  input.addEventListener('keydown', (e) => { if (e.key === 'Enter') submit(); });
  openModal({
    title,
    body: el('div', { class: 'confirm-body' }, el('label', null, label), input),
    actions: [{ label: '取消' }, { label: '确定', primary: true, onClick: () => { if (!input.value.trim()) return true; onOk(input.value.trim()); } }],
  });
  setTimeout(() => input.focus(), 30);
}

// ============ 全局 Tooltip：body 级单例（不受任何祖先裁剪，自动定向） ============
const tipEl = el('div', { class: 'gtip' });
document.body.append(tipEl);
let tipAnchor = null;
let tipTimer = 0;
const hideTip = () => { clearTimeout(tipTimer); tipEl.classList.remove('show'); tipAnchor = null; };
function showTip(target) {
  tipEl.textContent = target.getAttribute('data-tip');
  tipEl.classList.add('show');
  const r = target.getBoundingClientRect();
  const tw = tipEl.offsetWidth, th = tipEl.offsetHeight;
  let x = r.left + r.width / 2 - tw / 2;
  x = Math.max(8, Math.min(x, innerWidth - tw - 8));      // 水平夹紧不出屏
  const below = r.top < th + 12;                            // 贴近视口顶部 → 朝下
  tipEl.classList.toggle('below', below);
  tipEl.style.left = Math.round(x) + 'px';
  tipEl.style.top = Math.round((below ? r.bottom + 6 : r.top - th - 6)) + 'px';
}
document.addEventListener('pointerover', (e) => {
  const t = e.target instanceof Element ? e.target.closest('[data-tip]') : null;
  if (t === tipAnchor) return;
  clearTimeout(tipTimer);
  tipAnchor = t;
  if (!t) { tipEl.classList.remove('show'); return; }
  tipTimer = setTimeout(() => { if (tipAnchor === t) showTip(t); }, 100);
});
document.addEventListener('pointerdown', hideTip);
document.addEventListener('scroll', hideTip, true);

// ============ 图片放大（全局委托） ============
document.addEventListener('click', (e) => {
  const img = e.target.closest('.user-img img, .md img, .msg-imgs img');
  if (!img || !img.src) return;
  closeModal();
  overlayEl = el('div', { class: 'overlay lightbox', onclick: closeModal }, el('img', { src: img.src }));
  document.body.append(overlayEl);
});

// ============ 设置应用（主题：body[data-ds-dark-theme]） ============
export function applySettings(store) {
  document.body.toggleAttribute('data-ds-dark-theme', store.settings.theme === 'dark');
  document.documentElement.dataset.theme = store.settings.theme;
  document.documentElement.dataset.fs = store.settings.fontSize;
}

// ============ 导出会话 Markdown ============
export function exportSessionMarkdown(store, session) {
  if (!session) return;
  const lines = [`# ${session.title}`, '', `_导出于 ${new Date().toLocaleString()} · NativeHarness_`, ''];
  for (const n of session.nodes) {
    if (n.kind === 'user') {
      lines.push('## 用户', '', n.text || '', ...(n.images?.length ? ['', ...n.images.map((i) => `![${i.name ?? '附件'}](${i.dataUrl})`)] : []), '');
    } else if (n.kind === 'assistant') {
      lines.push('## 助手', '');
      for (const b of n.blocks ?? []) {
        if (b.type === 'text') lines.push(b.text, '');
        else if (b.type === 'image') lines.push(`![${b.name ?? '图片'}](${b.dataUrl})`, '');
      }
    } else if (n.kind === 'tool') {
      lines.push(`**${n.name}** \`${JSON.stringify(n.args)}\``, '', '```', String(n.result ?? ''), '```', '');
    }
  }
  const blob = new Blob([lines.join('\n')], { type: 'text/markdown;charset=utf-8' });
  const a = el('a', { href: URL.createObjectURL(blob), download: `${session.title.replace(/[\\/:*?"<>|]/g, '_')}.md` });
  a.click();
  URL.revokeObjectURL(a.href);
  toast('已导出 Markdown');
}

// ============ AppFrame：三列 + 拖拽（columns.js 契约几何）============
const FRAME = {
  SIDEBAR_MIN: 264, SIDEBAR_MAX: 420, SIDEBAR_DEFAULT: 280, SIDEBAR_COLLAPSED: 56,
  AUTO_COLLAPSE: 1024, DETAILS_MIN: 300, DETAILS_MAX: 520, DETAILS_DEFAULT: 360, CENTER_MIN: 640,
};
function clamp(px, min, max) { return Math.min(max, Math.max(min, Math.round(px))); }
function computeColumns(viewport, sidebar, details) {
  const s = sidebar === 0 ? FRAME.SIDEBAR_COLLAPSED : clamp(sidebar, FRAME.SIDEBAR_MIN, FRAME.SIDEBAR_MAX);
  const d0 = details === 0 ? 0 : clamp(details, FRAME.DETAILS_MIN, FRAME.DETAILS_MAX);
  if (s + d0 + FRAME.CENTER_MIN <= viewport) return { sidebar: s, center: viewport - s - d0, details: d0 };
  const d1 = d0 === 0 ? 0 : Math.max(FRAME.DETAILS_MIN, viewport - s - FRAME.CENTER_MIN);
  if (s + d1 + FRAME.CENTER_MIN <= viewport) return { sidebar: s, center: FRAME.CENTER_MIN, details: d1 };
  return { sidebar: s, center: Math.max(0, viewport - s), details: 0 };
}

function mountFrame(store) {
  const app = document.getElementById('app');
  const state = { sidebarPref: FRAME.SIDEBAR_DEFAULT, detailsPref: 0, narrowExpanded: false, dragging: false };

  const sidebarHandle = el('div', { class: 'drag-handle', 'data-side': 'sidebar' });
  const detailsHandle = el('div', { class: 'drag-handle', 'data-side': 'details' });
  app.append(sidebarHandle, detailsHandle);

  function collapsed() {
    return innerWidth < FRAME.AUTO_COLLAPSE ? !state.narrowExpanded : state.sidebarPref === 0;
  }
  function apply() {
    const pref = collapsed() ? 0 : (state.sidebarPref || FRAME.SIDEBAR_DEFAULT);
    const cols = computeColumns(innerWidth, pref, state.detailsPref);
    app.style.gridTemplateColumns = `${cols.sidebar}px minmax(0,1fr) ${cols.details}px`;
    app.toggleAttribute('data-sidebar-collapsed', collapsed());
    app.toggleAttribute('data-details-collapsed', cols.details === 0);
    app.toggleAttribute('data-dragging', state.dragging);
    sidebarHandle.style.left = cols.sidebar + 'px';
    sidebarHandle.style.display = collapsed() ? 'none' : '';
    detailsHandle.style.left = (innerWidth - cols.details) + 'px';
    detailsHandle.style.display = cols.details > 0 ? '' : 'none';
    return cols;
  }
  apply();
  window.addEventListener('resize', () => { apply(); store.notifier.markDirty(); });

  function dragify(handle, side) {
    let origin = 0, base = 0, latest = 0, frame = null;
    handle.addEventListener('pointerdown', (e) => {
      e.preventDefault();
      handle.setPointerCapture(e.pointerId);
      handle.setAttribute('data-dragging', '');
      origin = latest = e.clientX;
      base = side === 'sidebar' ? state.sidebarPref : state.detailsPref;
      state.dragging = true; apply();
    });
    handle.addEventListener('pointermove', (e) => {
      if (!handle.hasPointerCapture(e.pointerId)) return;
      latest = e.clientX;
      if (frame === null) {
        frame = requestAnimationFrame(() => {
          frame = null;
          const dx = latest - origin;
          if (side === 'sidebar') state.sidebarPref = clamp(base + dx, FRAME.SIDEBAR_MIN, FRAME.SIDEBAR_MAX);
          else state.detailsPref = clamp(base - dx, FRAME.DETAILS_MIN, FRAME.DETAILS_MAX);
          apply();
        });
      }
    });
    const up = (e) => {
      if (!handle.hasPointerCapture(e.pointerId)) return;
      handle.releasePointerCapture(e.pointerId);
      handle.removeAttribute('data-dragging');
      if (frame !== null) { cancelAnimationFrame(frame); frame = null; }
      state.dragging = false; apply(); store.persist();
    };
    handle.addEventListener('pointerup', up);
    handle.addEventListener('pointercancel', up);
  }
  dragify(sidebarHandle, 'sidebar');
  dragify(detailsHandle, 'details');

  function expand() {
    if (innerWidth < FRAME.AUTO_COLLAPSE) state.narrowExpanded = true;
    else state.sidebarPref = state.sidebarPref || FRAME.SIDEBAR_DEFAULT;
  }
  return {
    apply,
    toggleSidebar() {
      if (collapsed()) expand();
      else if (innerWidth < FRAME.AUTO_COLLAPSE) state.narrowExpanded = false;
      else state.sidebarPref = 0;
      apply(); store.notifier.markDirty();
    },
    expandSidebar: expand,
    collapseSidebar() {
      if (innerWidth < FRAME.AUTO_COLLAPSE) state.narrowExpanded = false;
      else state.sidebarPref = 0;
      apply(); store.notifier.markDirty();
    },
    setDetails(open) {
      state.detailsPref = open ? (state.detailsPref || FRAME.DETAILS_DEFAULT) : 0;
      apply(); store.notifier.markDirty();
    },
    detailsOpen: () => state.detailsPref > 0,
    collapsed,
  };
}

// ============ 侧栏（SidebarRoot + WorkspaceBrowser）============
const fmtClock = (t) => new Date(t).toTimeString().slice(0, 5);
const fmtDay = (t) => {
  const d = new Date(t);
  const today = new Date(); today.setHours(0, 0, 0, 0);
  if (d.getTime() >= today.getTime()) return '今天';
  if (d.getTime() >= today.getTime() - 86400000) return '昨天';
  if (d.getTime() >= today.getTime() - 7 * 86400000) return '7 天内';
  return '更早';
};

function mountSidebar(root, store, frame, hooks) {
  root.classList.add('sidebar');
  let sessionFilter = '';
  const searchInput = el('input', { type: 'search', placeholder: '搜索会话…' });

  // —— 设置视图侧栏：返回 + 分组导航（替代会话列表区）——
  const stNav = el('div', { class: 'stnav' });
  const settingsPane = el('div', { class: 'sidebar-wide' },
    el('div', { class: 'sidebar-head' },
      el('span', { class: 'logo-mark' }, fishLogo({ size: 22 })),
      el('div', { class: 'head-text' },
        el('div', { class: 'logo-title' }, 'NativeHarness'),
        el('div', { class: 'logo-sub' }, '设置')),
      el('button', { class: 'icon-btn sb-toggle', 'data-tip': '收起侧栏', onclick: () => frame.toggleSidebar() },
        icon('IconPanelLeftOutline16', { size: 16 }))),
    el('button', { class: 'stnav-back', onclick: () => { settingsState.active = false; store.notifier.markDirty(); } },
      icon('IconChevronLeftOutline14', { size: 14 }), el('span', null, '返回工作区')),
    stNav);

  // —— 展开态骨架（旧版布局；搜索框常驻，不再做展开动画）——
  const listEl = el('div', { class: 'session-list' });
  const wide = el('div', { class: 'sidebar-wide' },
    el('div', { class: 'sidebar-head' },
      el('span', { class: 'logo-mark' }, fishLogo({ size: 22 })),
      el('div', { class: 'head-text' },
        el('div', { class: 'logo-title' }, 'NativeHarness'),
        el('div', { class: 'logo-sub' }, '原生 · 无框架 · 事件溯源')),
      el('button', { class: 'icon-btn sb-toggle', 'data-tip': '收起侧栏', onclick: () => frame.toggleSidebar() },
        icon('IconPanelLeftOutline16', { size: 16 }))),
    el('button', { class: 'new-session', onclick: hooks.newSession },
      icon('IconNewChatOutline16', { size: 14 }), el('span', null, '新会话')),
    el('div', { class: 'session-search' }, searchInput),
    listEl,
    el('div', { class: 'sidebar-foot' },
      el('span', { class: 'conn-dot' }),
      el('span', { class: 'foot-text' }, `已连接 · ${store.host?.name ?? 'fixture'}`),
      el('span', { class: 'foot-spacer' }),
      el('button', { class: 'icon-btn foot-btn', 'data-tip': '快捷键帮助', onclick: hooks.openHelp }, icon('IconQuestionOutline14', { size: 14 })),
      el('button', { class: 'icon-btn foot-btn', 'data-tip': '切换主题', onclick: hooks.toggleTheme }, icon('IconDarkOutline16', { size: 16 })),
      el('button', { class: 'icon-btn foot-btn' + (settingsState.active ? ' on' : ''), 'data-tip': '设置', onclick: () => { settingsState.active ? closeSettings(store) : hooks.openSettings(); } }, icon('IconSettingsOutline16', { size: 16 }))),
  );

  // —— 折叠 rail（zcode 式：内容整体切换，不靠 CSS 挤压）——
  const rail = el('div', { class: 'sidebar-rail' },
    el('button', { class: 'icon-btn rail-btn', 'data-tip': '展开侧栏', onclick: () => frame.toggleSidebar() },
      fishLogo({ size: 20 })),
    el('button', {
      class: 'icon-btn rail-btn', 'data-tip': '新会话',
      onclick: () => { if (settingsState.active) { settingsState.active = false; } hooks.newSession(); },
    },
      icon('IconNewChatOutline16', { size: 18 })),
    el('span', { class: 'rail-gap' }),
    el('button', { class: 'icon-btn rail-btn', 'data-tip': '切换主题', onclick: hooks.toggleTheme }, icon('IconDarkOutline16', { size: 16 })),
    el('button', {
      class: 'icon-btn rail-btn' + (settingsState.active ? ' on' : ''), 'data-tip': '设置',
      onclick: () => { if (settingsState.active) { settingsState.active = false; store.notifier.markDirty(); } else hooks.openSettings(); },
    }, icon('IconSettingsOutline16', { size: 16 })));

  searchInput.addEventListener('input', () => {
    sessionFilter = searchInput.value;
    renderList();
  });

  function renderList() {
    const snap = store.snapshot();
    const q = sessionFilter.trim().toLowerCase();
    const sessions = snap.sessions.filter((s) => !q || s.title.toLowerCase().includes(q));
    const groups = new Map();
    for (const s of sessions) {
      const label = s.pinned ? '置顶' : (s.log?.events?.length ? fmtDay(s.updatedAt) : '今天');
      if (!groups.has(label)) groups.set(label, []);
      groups.get(label).push(s);
    }
    const order = ['置顶', '今天', '昨天', '7 天内', '更早'];
    listEl.replaceChildren(
      ...order.filter((k) => groups.has(k)).flatMap((label) => [
        el('div', { class: 'group-label' }, label),
        ...groups.get(label).map((s) => el('div', {
          class: 'session-item' + (s.id === store.selectedId ? ' active' : ''),
          dataset: { sid: s.id },
          onclick: () => store.select(s.id),
        },
          el('span', { class: 's-dot' + (s.running ? ' running' : s.done ? ' done' : '') }),
          el('span', { class: 's-title' }, s.title),
          el('span', { class: 's-time' }, fmtClock(s.updatedAt)),
          el('span', { class: 's-acts' },
            el('button', { class: 'icon-btn s-act', 'data-tip': '置顶/取消', onclick: (e) => { e.stopPropagation(); store.togglePin(s.id); } }, icon('IconCheckOutline14', { size: 14 })),
            el('button', { class: 'icon-btn s-act danger', 'data-tip': '删除会话', onclick: (e) => { e.stopPropagation(); hooks.deleteSession(s); } }, icon('IconTrashOutline16', { size: 14 }))))),
      ]),
      ...(sessions.length === 0 ? [el('div', { class: 'no-hit' }, '没有匹配的会话')] : []));
  }

  function renderStNav() {
    stNav.replaceChildren(...ST_SECTIONS.flatMap((g) => [
      el('div', { class: 'group-label' }, g.group),
      ...g.items.map((it) => el('div', {
        class: 'stnav-item' + (settingsState.section === it.id ? ' active' : ''),
        onclick: () => { settingsState.section = it.id; store.notifier.markDirty(); },
      },
        icon(it.icon, { size: 16 }),
        el('span', null, it.id))),
    ]));
  }
  function render() {
    frame.apply();   // 渲染前同步框架几何（resize 事件丢失时自愈）
    if (settingsState.active && !frame.collapsed()) {
      root.replaceChildren(settingsPane);
      root.classList.remove('rail-mode');
      renderStNav();
      return;
    }
    // 设置态下折叠：沿用对话 rail（齿轮 on，点齿轮退出设置）
    const c = frame.collapsed();
    const cur = root.firstElementChild;
    const want = c ? rail : (settingsState.active ? settingsPane : wide);
    if (cur !== want) root.replaceChildren(want);
    root.classList.toggle('rail-mode', c);
    if (!settingsState.active) renderList();
  }
  render();
  return { render };
}

// ============ ChatView 头部（lvQYKa）============
function mountHeader(root, store, host, frame, hooks) {
  let menuOpen = false;
  let modelOpen = false;
  const crumbs = el('div', { class: 'cv-crumbs' });
  const tabs = el('div', { class: 'cv-tabs' });
  const actions = el('div', { class: 'cv-header-actions' });

  function renderCrumbs() {
    const s = store.snapshot().selected;
    crumbs.replaceChildren(
      el('button', { class: 'cv-crumb' }, 'workspace'),
      el('span', { class: 'cv-crumb-sep' }, '/'),
      el('button', { class: 'cv-crumb current' }, s?.title ?? '未选择'));
  }
  function renderTabs() {
    tabs.replaceChildren(
      el('button', { class: 'cv-tab' + (!frame.detailsOpen() ? ' active' : ''), onclick: () => frame.setDetails(false) }, '对话'),
      el('button', { class: 'cv-tab' + (frame.detailsOpen() ? ' active' : ''), onclick: () => frame.setDetails(!frame.detailsOpen()) }, '轨迹'));
  }
  function renderActions() {
    // el() 会过滤 null 子节点，replaceChildren 不会——菜单关闭时传 null 会渲染出字面量 "null"
    const menu = menuOpen ? el('div', { class: 'dropdown' },
      el('button', { onclick: () => { menuOpen = false; hooks.renameSession(); } }, icon('IconEditOutline16', { size: 16 }), '重命名会话'),
      el('button', { onclick: () => { menuOpen = false; hooks.togglePin(); } }, icon('IconCheckOutline14', { size: 16 }), '置顶/取消置顶'),
      el('button', { class: 'danger', onclick: () => { menuOpen = false; hooks.deleteSession(store.snapshot().selected); } }, icon('IconTrashOutline16', { size: 16 }), '删除会话')) : null;
    actions.replaceChildren(
      el('button', { class: 'icon-btn', 'data-tip': '导出 (Ctrl+E)', onclick: hooks.exportSession }, icon('IconDownloadOutline16', { size: 16 })),
      el('button', { class: 'icon-btn', 'data-tip': '更多', onclick: () => { menuOpen = !menuOpen; renderActions(); } }, icon('IconEllipsisOutline16', { size: 16 })),
      ...(menu ? [menu] : []));
  }
  document.addEventListener('click', (e) => {
    if (menuOpen && !e.target.closest('.cv-header-actions')) { menuOpen = false; renderActions(); }
  });

  function render() {
    renderCrumbs(); renderTabs(); renderActions();
  }
  root.replaceChildren(
    el('div', { class: 'cv-title-row' },
      el('div', { class: 'cv-title-cluster' }, crumbs),
      actions),
    tabs);
  render();
  return { render };
}

// ============ 详情列（JXxdLa + 轨迹占位）============
function mountDetails(root, store) {
  const body = el('div', { class: 'dt-body' });
  root.replaceChildren(
    el('div', { class: 'dt-root' },
      el('div', { class: 'dt-header' },
        el('span', { class: 'dt-title' }, '轨迹'),
        el('button', { class: 'icon-btn', 'data-tip': '关闭' }, icon('IconCloseOutline16', { size: 14 }))),
      body));
  root.querySelector('.dt-header .icon-btn').onclick = () => { window.__app.frame.setDetails(false); };

  function render() {
    const s = store.snapshot().selected;
    if (!s) { body.replaceChildren(el('div', { class: 'dt-empty' }, '未选择会话')); return; }
    const events = s.log.events.slice(-200).reverse();
    body.replaceChildren(
      el('div', { class: 'dt-section' },
        el('div', { class: 'dt-section-label' }, `事件流（最近 ${events.length} 条，倒序）`),
        el('div', null, events.map((ev) => el('div', { class: 'traj-row' },
          el('span', { class: 'traj-time' }, fmtClock(ev.time)),
          el('span', { class: 'traj-type' }, ev.type),
          el('span', { class: 'traj-desc' }, describe(ev)))))));
  }
  function describe(ev) {
    const d = ev.data ?? {};
    if (ev.type === 'user/message') return (d.content ?? []).filter((p) => p.type === 'text').map((p) => p.text).join(' ');
    if (ev.type === 'tool/result') return `#${d.callId} ${String(d.output ?? '').split('\n')[0]}`;
    if (ev.type === 'tool/update') return `#${d.callId} 补丁`;
    if (ev.type === 'assistant/message') return (d.message?.content ?? []).map((b) => b.type).join('+');
    return '';
  }
  render();
  return { render };
}

const fmtK = (n) => n >= 1000 ? (n / 1000).toFixed(1).replace(/\.0$/, '') + 'K' : String(n);

// ============ 设置：ZCode 式整页视图（侧栏导航 + 主区卡片页） ============
const settingsState = { active: false, section: '外观', prevCollapsed: false };
export function openSettings(store) {
  const frame = window.__app && window.__app.frame;
  settingsState.prevCollapsed = frame ? frame.collapsed() : false;
  settingsState.active = true;
  if (frame) frame.expandSidebar();          // 设置导航需要宽度：强制展开
  store.notifier.markDirty();
}
export function closeSettings(store) {
  const frame = window.__app && window.__app.frame;
  settingsState.active = false;
  if (frame) {
    if (settingsState.prevCollapsed) frame.collapseSidebar();   // 还原进入前的收起态
    else frame.expandSidebar();
  }
  store.notifier.markDirty();
}
export function isSettingsView() { return settingsState.active; }

const SEG = (opts, val, onch) => el('div', { class: 'seg' },
  opts.map(([v, label]) => el('button', {
    class: 'seg-btn' + (v === val ? ' on' : ''),
    onclick: () => { onch(v); },
  }, label)));

const ST_ROW = (title, desc, control) => el('div', { class: 'st-row' },
  el('div', { class: 'st-row-text' },
    el('div', { class: 'st-row-title' }, title),
    desc ? el('div', { class: 'st-row-desc' }, desc) : null),
  control);

function stSection(store, id) {
  const s = store.settings;
  if (id === '外观') return [el('div', { class: 'st-card' },
    ST_ROW('主题', '界面配色，跟随系统或手动固定', SEG([['light', '浅色'], ['dark', '深色']], s.theme, (v) => { s.theme = v; applySettings(store); store.persist(); })),
    ST_ROW('字号', '对话正文与界面的基础字号', SEG([['sm', '小'], ['md', '中'], ['lg', '大']], s.fontSize, (v) => { s.fontSize = v; applySettings(store); store.persist(); })))];
  if (id === '常规') return [el('div', { class: 'st-card' },
    ST_ROW('完成提示音', '会话完成且窗口在后台时播放提示音', SEG([['on', '开'], ['off', '关']], s.sound ? 'on' : 'off', (v) => { s.sound = v === 'on'; store.persist(); })),
    ST_ROW('审批自动放行（全局）', '文件写入等敏感操作自动批准，不再逐次询问；会话内仍可在输入条单独覆盖', SEG([['on', '开'], ['off', '关']], s.autoApprove ? 'on' : 'off', (v) => { s.autoApprove = v === 'on'; store.persist(); })))];
  if (id === '模型设置') return [el('div', { class: 'st-card' },
    el('div', { class: 'st-row col' },
      el('div', { class: 'st-row-text' },
        el('div', { class: 'st-row-title' }, '系统提示词'),
        el('div', { class: 'st-row-desc' }, '随每次请求上行；留空则不附加。')),
      (() => {
        const ta = el('textarea', { class: 'modal-input sysprompt', rows: '4', placeholder: '例：你是一个谨慎的编码助手…' });
        ta.value = s.systemPrompt;
        ta.addEventListener('change', () => { s.systemPrompt = ta.value; store.persist(); });
        return ta;
      })()))];
  if (id === '数据管理') return [el('div', { class: 'st-card' },
    ST_ROW('清空全部数据', '删除所有本地会话与设置，刷新后不可恢复。数据保存在浏览器 localStorage',
      el('button', {
        class: 'dsw-btn danger', onclick: () => confirmModal({
          title: '清空全部数据', message: '将删除所有本地会话与设置，刷新后不可恢复。确定？',
          onOk: () => { store.clearPersisted(); location.reload(); },
        }),
      }, '清空全部数据')))];
  return [];
}

const ST_SECTIONS = [
  { group: '基础设置', items: [
    { id: '外观', icon: 'IconDarkOutline16' },
    { id: '常规', icon: 'IconSettingsOutline16' },
    { id: '模型设置', icon: 'IconSparkle16' },
  ] },
  { group: '数据', items: [
    { id: '数据管理', icon: 'IconDataOutline16' },
  ] },
];

function mountSettingsPage(root, store) {
  function render() {
    root.hidden = !settingsState.active;
    if (!settingsState.active) return;
    root.replaceChildren(el('div', { class: 'st-wrap' },
      el('h1', { class: 'st-title' }, settingsState.section),
      ...stSection(store, settingsState.section)));
  }
  store.notifier.subscribe(render);
  render();
}
// ============ 帮助弹窗 ============

export function openHelp() {
  const kbd = (k) => el('kbd', null, k);
  const row = (k, d) => el('tr', null, el('td', null, k), el('td', null, d));
  openModal({
    title: '快捷键与命令',
    body: el('div', { class: 'help-body' },
      el('h4', null, '快捷键'),
      el('table', { class: 'help-table' }, el('tbody', null,
        row(el('span', null, kbd('Enter'), ' 发送'), 'Shift+Enter 换行'),
        row(kbd('Esc'), '中断运行 / 关闭弹层'),
        row(el('span', null, kbd('Ctrl'), '+', kbd('K')), '新会话'),
        row(el('span', null, kbd('Ctrl'), '+', kbd('F')), '会话内搜索'),
        row(el('span', null, kbd('Ctrl'), '+', kbd('E')), '导出 Markdown'),
        row(el('span', null, kbd('Ctrl'), '+', kbd('J')), '切换主题'),
        row(kbd('?'), '本帮助（输入框外）'))),
      el('h4', null, '斜杠命令'),
      el('table', { class: 'help-table' }, el('tbody', null, SLASH_COMMANDS.map((c) => row(el('code', null, c.cmd), c.desc)))),
      el('h4', null, '输入增强'),
      el('div', { style: 'color:var(--dsw-alias-label-tertiary);font-size:12.5px' },
        '输入 / 唤起命令菜单 · 输入 @ 补全文件路径 · 拖拽 / 粘贴 / 📎 添加图片附件；运行中发送自动排队。')),
    actions: [{ label: '知道了', primary: true }],
  });
}

// ============ 外壳挂载 ============
export function mountChrome(store, host, { sidebar, topbar, details, hooks }) {
  const frame = mountFrame(store);
  window.__app = window.__app || {};
  window.__app.frame = frame;

  const h = {
    newSession: hooks.newSession,
    deleteSession: (s) => confirmModal({
      title: '删除会话',
      message: `「${s.title}」将被删除，且不可恢复。`,
      onOk: () => { store.removeSession(s.id); store.persist(); toast('会话已删除'); },
    }),
    renameSession: () => {
      const s = store.snapshot().selected;
      if (!s) return;
      promptModal({ title: '重命名会话', label: '新名称', value: s.title, onOk: (v) => store.renameSession(s.id, v) });
    },
    togglePin: () => { const s = store.snapshot().selected; if (s) store.togglePin(s.id); },
    exportSession: () => exportSessionMarkdown(store, store.snapshot().selected),
    openSettings: () => openSettings(store),
    openHelp: () => openHelp(),
    toggleTheme: () => {
      store.settings.theme = store.settings.theme === 'dark' ? 'light' : 'dark';
      applySettings(store); store.persist();
    },
  };

  const sb = mountSidebar(sidebar, store, frame, h);
  const hd = mountHeader(topbar, store, host, frame, h);
  const dt = mountDetails(details, store);
  mountSettingsPage(document.getElementById('settings-root'), store);

  // 标签页徽标 + 提示音
  let lastDone = 0;
  function notify() {
    const doneCount = [...store.sessions.values()].filter((s) => s.done && s.id !== store.selectedId).length;
    document.title = (doneCount > 0 ? `(${doneCount}) ` : '') + 'NativeHarness — agent 前端';
    if (doneCount > lastDone && store.settings.sound && document.visibilityState !== 'visible') beep();
    lastDone = doneCount;
  }
  store.notifier.subscribe(notify);

  function render() { sb.render(); hd.render(); dt.render(); }
  store.notifier.subscribe(render);
  render();
  return { frame };
}

function beep() {
  try {
    const ctx = new AudioContext();
    const o = ctx.createOscillator(), g = ctx.createGain();
    o.connect(g); g.connect(ctx.destination);
    o.frequency.value = 880; g.gain.value = 0.06;
    o.start();
    g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.35);
    o.stop(ctx.currentTime + 0.36);
  } catch { /* 无声环境静默 */ }
}
