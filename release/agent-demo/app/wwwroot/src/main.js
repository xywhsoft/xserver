// main.js — 启动装配
//
// 本版默认 XsHttpHost（xs C 脚本后端，HTTP 轮询）。
// 离线模式（python -m http.server）或 ?host=fixture 切回 FixtureHost。
// 参数：?host=fixture 离线演示   ?host=xs 强制 xs 后端（默认）
//       ?reset=1 忽略本地存档冷启动

import { Store } from './core.js';
import { FixtureHost, XsHttpHost } from './wire.js';
import { installScripts } from './fixture.js';
import { mountApp } from './ui.js?v=17';
import { applySettings, mountChrome } from './chrome.js?v=14';

const params = new URLSearchParams(location.search);

const store = new Store();
store.models = [
  { id: 'fixture/fx-1', name: 'FX-1 · 演示模型', contextWindow: 128000 },
  { id: 'deepseek-v4-flash', name: 'DeepSeek V4 Flash', contextWindow: 128000 },
  { id: 'local/qwen3.5-4b', name: '本地 Qwen3.5-4B', contextWindow: 32768 },
];
store.model = store.models[0];

// 主题默认跟随系统（原版语义），首次持久化后以存档为准
if (!localStorage.getItem('nativeharness:v1')) {
  store.settings.theme = matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
}

// 宿主选择：?host=fixture 强制离线；?host=xs 或同源可达时用 xs C 后端
const hostParam = params.get('host') ?? '';
let host;
if (hostParam === 'fixture') {
  host = new FixtureHost(store, {
    scale: parseFloat(params.get('speed') ?? '1') || 1,
    autoApprove: params.get('autoapprove') === '1',
    instant: params.get('instant') === '1',
  });
  installScripts(host, store);
} else {
  host = new XsHttpHost(store, {});
  // 检测可达性：连不上则降级到 fixture（开发时离线打开）
  try {
    const probe = await fetch('api/prompt', { method: 'OPTIONS' });
    // 204/200 都算可达
  } catch {
    host = new FixtureHost(store, {
      scale: parseFloat(params.get('speed') ?? '1') || 1,
      autoApprove: params.get('autoapprove') === '1',
      instant: params.get('instant') === '1',
    });
    installScripts(host, store);
    console.info('[host] xs backend unreachable, falling back to fixture');
  }
}
store.host = host;
store.settings.autoApprove = params.get('autoapprove') === '1' || store.settings.autoApprove;

applySettings(store);

// 恢复存档；xs 模式无种子数据（空态等用户输入），fixture 模式种子演示
const restored = params.get('reset') === '1' ? false : store.restore();
if (!restored && host.name === 'fixture') host.seed();
// xs 模式冷启动无会话：建一个空会话让输入框立即可用
if (!store.selectedId) {
  const s = store.createSession();
  store.select(s.id);
}

// 持久化：任何渲染后防抖 500ms 落盘（大附件配额溢出时静默降级为内存态）
let persistTimer = null;
store.notifier.subscribe(() => {
  clearTimeout(persistTimer);
  persistTimer = setTimeout(() => store.persist(), 500);
});

window.__app = window.__app || {};
window.__app.store = store;
window.__app.host = host;
window.__app.newSession = () => {
  const s = store.createSession();
  store.select(s.id);
  store.persist();
  import('./ui.js?v=17').then((m) => m.focusComposer());
};

// 外壳（三列框架 / 侧栏 / 头部 / 详情列）
mountChrome(store, host, {
  sidebar: document.getElementById('sidebar-col'),
  topbar: document.getElementById('topbar'),
  details: document.getElementById('details-col'),
  hooks: {
    newSession: () => window.__app.newSession(),
    openSettings: () => import('./chrome.js?v=14').then((m) => m.openSettings(store)),
    openHelp: () => import('./chrome.js?v=14').then((m) => m.openHelp()),
  },
});

// 对话视图
mountApp(store, host, {
  cvRoot: document.getElementById('cv-root'),
  chatScroll: document.getElementById('chat-scroll'),
  chatList: document.getElementById('chat-list'),
  docks: document.getElementById('docks'),
  composerWrap: document.getElementById('composer-wrap'),
  composerSeat: document.getElementById('composer-seat'),
});

if (params.get('autostart') === '1' && host.runShowcase) {
  setTimeout(() => host.runShowcase(store.selectedId), 400);
}
