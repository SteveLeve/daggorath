#!/usr/bin/env node
// Browser test for the web build's storage (ADR-0011, web/web_storage.cpp).
// Serves web/dist, drives headless Chrome over CDP (no npm dependencies;
// Node 22+ for the global WebSocket), types into the game as a player would,
// and checks what reached localStorage and what the player was told:
//
//   1. ZSAVE stores the image; a reload puts it back on the cassette
//   2. the Video and Controls menu entries are remembered across a reload
//   3. a write that throws (quota exhausted) is reported, not claimed stored
//   4. storage that cannot be read at all is reported, and play still starts
//
//   node tools/web/storage-test.mjs [web/dist]      (CHROME=... to override)
//
// Exits non-zero on the first failed check.

import { spawn } from 'node:child_process';
import { mkdtempSync, readFileSync, rmSync, existsSync, statSync } from 'node:fs';
import { createServer } from 'node:http';
import { tmpdir } from 'node:os';
import { extname, join, normalize } from 'node:path';

if (typeof WebSocket !== 'function') {
  console.error(`storage-test needs Node 22+ (global WebSocket); this is ${process.version}`);
  process.exit(2);
}
const DIST = process.argv[2] || 'web/dist';
const CHROME = process.env.CHROME || 'google-chrome';
if (!existsSync(join(DIST, 'dod.wasm'))) throw new Error(`no build in ${DIST}; run make web`);

// --- static server for web/dist --------------------------------------------
const TYPES = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm',
  '.webmanifest': 'application/manifest+json', '.png': 'image/png', '.txt': 'text/plain' };
const server = createServer((req, res) => {
  const path = normalize(decodeURIComponent(new URL(req.url, 'http://x').pathname)).replace(/^([/\\])+/, '');
  const file = join(DIST, path || 'index.html');
  if (!file.startsWith(normalize(DIST)) || !existsSync(file) || statSync(file).isDirectory()) {
    res.writeHead(404).end();
    return;
  }
  res.writeHead(200, { 'Content-Type': TYPES[extname(file)] || 'application/octet-stream' });
  res.end(readFileSync(file));
});
await new Promise((r) => server.listen(0, '127.0.0.1', r));
const PAGE = `http://127.0.0.1:${server.address().port}/`;

// --- headless Chrome over CDP ----------------------------------------------
const profile = mkdtempSync(join(tmpdir(), 'dod-storage-'));
const chrome = spawn(CHROME, [
  '--headless=new', '--remote-debugging-port=0', `--user-data-dir=${profile}`,
  '--no-first-run', '--use-angle=swiftshader', '--enable-unsafe-swiftshader',
  '--window-size=1100,800', 'about:blank',
], { stdio: ['ignore', 'ignore', 'pipe'] });
const cleanup = () => {
  chrome.kill();
  server.close();
  rmSync(profile, { recursive: true, force: true });
};
process.on('exit', cleanup);

const wsUrl = await new Promise((resolve, reject) => {
  let buf = '';
  chrome.stderr.on('data', (d) => {
    buf += d;
    const m = buf.match(/DevTools listening on (ws:\S+)/);
    if (m) resolve(m[1]);
  });
  chrome.on('exit', (c) => reject(new Error(`chrome exited ${c}`)));
});
const port = new URL(wsUrl).port;
const targets = await (await fetch(`http://127.0.0.1:${port}/json`)).json();
const ws = new WebSocket(targets.find((t) => t.type === 'page').webSocketDebuggerUrl);
await new Promise((r) => ws.addEventListener('open', r, { once: true }));

let nextId = 1;
const pending = new Map();
let consoleLines = [];
ws.addEventListener('message', (ev) => {
  const msg = JSON.parse(ev.data);
  if (msg.id && pending.has(msg.id)) {
    const { resolve, reject } = pending.get(msg.id);
    pending.delete(msg.id);
    msg.error ? reject(new Error(msg.error.message)) : resolve(msg.result);
  } else if (msg.method === 'Runtime.consoleAPICalled') {
    consoleLines.push(msg.params.args.map((a) => a.value ?? a.description ?? '').join(' '));
  }
});
const cdp = (method, params = {}) => new Promise((resolve, reject) => {
  const id = nextId++;
  pending.set(id, { resolve, reject });
  ws.send(JSON.stringify({ id, method, params }));
});
const evaluate = async (expression) => {
  const r = await cdp('Runtime.evaluate', { expression, awaitPromise: true, returnByValue: true });
  if (r.exceptionDetails) throw new Error(r.exceptionDetails.exception?.description || r.exceptionDetails.text);
  return r.result.value;
};
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
async function until(what, predicate, ms = 15000) {
  for (const end = Date.now() + ms; Date.now() < end; await sleep(100))
    if (await predicate().catch(() => false)) return;
  throw new Error(`timed out waiting for ${what}`);
}

let checks = 0;
function check(ok, what, detail = '') {
  ++checks;
  if (!ok) {
    console.error(`FAIL: ${what}${detail ? `  [${detail}]` : ''}`);
    process.exit(1);
  }
  console.log(`ok   ${what}`);
}

// Real key events, the way a keyboard reaches SDL (no side door into the game).
async function key(k) {
  const special = { Enter: ['Enter', 13, '\r'], Escape: ['Escape', 27, ''] }[k];
  const [code, vk, text] = special ||
    [/[A-Z]/i.test(k) ? `Key${k.toUpperCase()}` : /\d/.test(k) ? `Digit${k}` : 'Space',
     k === ' ' ? 32 : k.toUpperCase().charCodeAt(0), k];
  await cdp('Input.dispatchKeyEvent', { type: 'keyDown', key: k, code, windowsVirtualKeyCode: vk, text });
  await cdp('Input.dispatchKeyEvent', { type: 'keyUp', key: k, code, windowsVirtualKeyCode: vk });
  await sleep(40);  // the game takes about one keystroke per jiffy
}
async function typeLine(line) {
  for (const ch of line) await key(ch);
  await key('Enter');
}

// Load (or reload) the page, optionally with a script run before the game's
// own, and wait for the game's start-up line.
let injected = null;
async function load(prelude = null) {
  if (injected) await cdp('Page.removeScriptToEvaluateOnNewDocument', { identifier: injected });
  injected = prelude
    ? (await cdp('Page.addScriptToEvaluateOnNewDocument', { source: prelude })).identifier
    : null;
  consoleLines = [];
  await cdp('Page.navigate', { url: PAGE });
  await until('game start-up line', async () => consoleLines.some((l) => l.startsWith('dod: ')));
  await until('loading notice gone', () => evaluate('!document.getElementById("status").classList.contains("show")'));
  await sleep(500);
  await evaluate('document.getElementById("canvas").focus()');
  return consoleLines.find((l) => l.startsWith('dod: '));
}
const toast = () => evaluate('document.getElementById("toast").classList.contains("show") ? document.getElementById("toast").textContent : ""');
const stored = (k) => evaluate(`localStorage.getItem(${JSON.stringify(k)})`);

await cdp('Runtime.enable');
await cdp('Page.enable');

// 1. ZSAVE stores; a reload mounts it.
let startup = await load();
check(/saves=0/.test(startup), 'fresh profile starts with no saves', startup);
check(/layout=tablet video=pixel/.test(startup), 'fresh profile uses the screen-shape default', startup);
await typeLine('ZSAVE WEBTEST');
await until('ZSAVE to reach localStorage', async () => ((await stored('dod.save.WEBTEST')) || '').startsWith('DAGRAM 1'));
check(true, 'ZSAVE WEBTEST stored a DAGRAM 1 image');
check((await toast()) === '', 'no storage notice after a good save');
startup = await load();
check(/saves=1/.test(startup), 'reload puts the stored save on the cassette', startup);

// 2. Video and Controls are remembered.
await key('Escape');
await key('v');
await key('c');
await key('Escape');
await until('prefs to be stored', async () => /video=crisp/.test((await stored('dod.prefs')) || ''));
check(/layout=phone/.test((await stored('dod.prefs')) || ''), 'Controls choice stored');
startup = await load();
check(/layout=phone video=crisp/.test(startup), 'reload restores Video and Controls', startup);

// 3. A write that throws is reported and not claimed as stored.
await load(`Storage.prototype.setItem = function () {
  throw new DOMException("simulated full storage", "QuotaExceededError"); };`);
await typeLine('ZSAVE FULL');
await until('storage notice', async () => /NOT STORED/.test(await toast()));
check(/ZSAVE FULL NOT STORED/.test(await toast()), 'failed ZSAVE reported to the player', await toast());
check((await stored('dod.save.FULL')) === null, 'failed ZSAVE left nothing behind');
await key('Escape');
await key('v');
await key('Escape');
await until('settings notice', async () => /SETTINGS NOT SAVED/.test(await toast()));
check(true, 'failed settings write reported to the player');

// 4. Unreadable storage is reported, and the game still runs.
startup = await load(`Object.defineProperty(window, "localStorage", {
  get() { throw new DOMException("simulated blocked storage", "SecurityError"); } });`);
check(/saves=0/.test(startup), 'blocked storage: game starts with no saves', startup);
check(/SAVED GAMES UNAVAILABLE/.test(await toast()), 'blocked storage reported to the player', await toast());

console.log(`PASS: ${checks} checks`);
process.exit(0);
