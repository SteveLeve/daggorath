#!/usr/bin/env node
// Agent-controllable driver for a locally served web port (reference-only;
// see docs/planning/port-comparison.md). Launches headless Chrome over CDP,
// starts the game, and exposes a small HTTP control API so an agent can play
// with curl across many turns without losing the browser session.
//
//   node tools/compare/drive-web.mjs [--page URL] [--control 8081] [--headed]
//
//   POST /send   body = command line      -> sendinput(line + "\r")
//   POST /key    body = SDL keycode        -> sendkey(code)
//   POST /stopdemo                          -> end the attract demo
//   POST /shot   body = name               -> PNG of the canvas in $DOD_COMPARE_OUT
//   POST /eval   body = JS expression      -> JSON result
//   GET  /state                            -> inventory, floor, demo, menu
//   GET  /log                              -> console lines since last call
//
// Every request is appended to $DOD_COMPARE_OUT/web-actions.jsonl with a
// wall-clock timestamp, so a session can be lined up with a dcli trace.

import { spawn } from 'node:child_process';
import { mkdirSync, writeFileSync, appendFileSync, mkdtempSync } from 'node:fs';
import { createServer } from 'node:http';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const arg = (name, def) => {
  const i = process.argv.indexOf(name);
  return i >= 0 ? process.argv[i + 1] : def;
};
const PAGE = arg('--page', 'http://localhost:8080/index.local.html');
const CONTROL = Number(arg('--control', 8081));
const HEADED = process.argv.includes('--headed');
const OUT = process.env.DOD_COMPARE_OUT || 'captures/compare';
const CHROME = process.env.CHROME || 'google-chrome';
mkdirSync(OUT, { recursive: true });

const profile = mkdtempSync(join(tmpdir(), 'dod-chrome-'));
const chrome = spawn(CHROME, [
  ...(HEADED ? [] : ['--headless=new']),
  '--remote-debugging-port=0', `--user-data-dir=${profile}`,
  '--no-first-run', '--autoplay-policy=no-user-gesture-required',
  '--use-angle=swiftshader', '--enable-unsafe-swiftshader',
  '--window-size=1100,1000', 'about:blank',
], { stdio: ['ignore', 'ignore', 'pipe'] });
process.on('exit', () => chrome.kill());
process.on('SIGINT', () => process.exit(0));

const wsBrowser = await new Promise((resolve, reject) => {
  let buf = '';
  chrome.stderr.on('data', (d) => {
    buf += d;
    const m = buf.match(/DevTools listening on (ws:\S+)/);
    if (m) resolve(m[1]);
  });
  chrome.on('exit', (c) => reject(new Error(`chrome exited ${c}`)));
});
const port = new URL(wsBrowser).port;
const targets = await (await fetch(`http://127.0.0.1:${port}/json`)).json();
const ws = new WebSocket(targets.find((t) => t.type === 'page').webSocketDebuggerUrl);
await new Promise((r) => ws.addEventListener('open', r, { once: true }));

let nextId = 1;
const pending = new Map();
const consoleLines = [];
ws.addEventListener('message', (ev) => {
  const msg = JSON.parse(ev.data);
  if (msg.id && pending.has(msg.id)) {
    const { resolve, reject } = pending.get(msg.id);
    pending.delete(msg.id);
    msg.error ? reject(new Error(msg.error.message)) : resolve(msg.result);
  } else if (msg.method === 'Runtime.consoleAPICalled') {
    const line = msg.params.args.map((a) => a.value ?? a.description ?? '').join(' ');
    consoleLines.push(line);
    appendFileSync(join(OUT, 'web-console.log'), `${Date.now()} ${line}\n`);
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

await cdp('Runtime.enable');
await cdp('Page.enable');
await cdp('Page.navigate', { url: PAGE });
for (let i = 0; i < 300 && !(await evaluate('!!(window.Module && Module.calledRun !== undefined && document.readyState === "complete")').catch(() => false)); i++) await sleep(100);
if (!(await evaluate('window.crossOriginIsolated'))) throw new Error('page is not cross-origin isolated; use tools/compare/serve-web.py');
await evaluate('document.getElementById("readyPlayerOne").click()');
for (let i = 0; i < 300 && !(await evaluate('typeof isDemoFunc === "function" && Module.calledRun === true').catch(() => false)); i++) await sleep(100);

const ccall = (name, types, args) =>
  evaluate(`Module.ccall(${JSON.stringify(name)}, 'void', ${JSON.stringify(types)}, ${JSON.stringify(args)})`);

async function shot(name) {
  const clip = await evaluate(`(() => { const r = document.getElementById('canvas').getBoundingClientRect();
    return { x: r.x + scrollX, y: r.y + scrollY, width: r.width, height: r.height, scale: 1 }; })()`);
  const { data } = await cdp('Page.captureScreenshot', { format: 'png', clip, captureBeyondViewport: true });
  const file = join(OUT, `${name.replace(/[^\w.-]/g, '_') || Date.now()}.png`);
  writeFileSync(file, Buffer.from(data, 'base64'));
  return file;
}

const routes = {
  'POST /send': async (b) => (await ccall('sendinput', ['string'], [`${b}\r`]), 'ok'),
  'POST /key': async (b) => (await ccall('sendkey', ['number'], [Number(b)]), 'ok'),
  'POST /stopdemo': async () => (await ccall('stopdemo', [], []), 'ok'),
  'POST /shot': async (b) => shot(b),
  'POST /eval': async (b) => JSON.stringify(await evaluate(b)),
  'GET /state': async () => JSON.stringify(await evaluate(`({
    demo: isDemoFunc(), menu: isMenuOpenFunc(),
    inventory: Module.ccall('getinventory', 'string', []),
    floor: Module.ccall('getfloor', 'string', []) })`)),
  'GET /log': async () => consoleLines.splice(0).join('\n'),
};

createServer(async (req, res) => {
  let body = '';
  for await (const c of req) body += c;
  const key = `${req.method} ${req.url.split('?')[0]}`;
  appendFileSync(join(OUT, 'web-actions.jsonl'), JSON.stringify({ t: Date.now(), route: key, body }) + '\n');
  try {
    const route = routes[key];
    if (!route) { res.writeHead(404).end('unknown route\n'); return; }
    res.writeHead(200).end(`${await route(body.replace(/\r?\n$/, ''))}\n`);
  } catch (e) {
    res.writeHead(500).end(`${e.message}\n`);
  }
}).listen(CONTROL, '127.0.0.1', () => console.log(`control on http://127.0.0.1:${CONTROL}`));
