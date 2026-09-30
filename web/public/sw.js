// Offline cache for the web build (ADR-0011). `make web` replaces the
// placeholder with a hash of the game files, so a new build gets a new cache
// and the old one is deleted.
const CACHE = "dod-__BUILD__";
const FILES = [
  "./", "index.html", "dod.js", "dod.wasm", "manifest.webmanifest",
  "icon-192.png", "icon-512.png", "icon-maskable-512.png", "apple-touch-icon.png",
  "THIRD_PARTY_NOTICES.txt",
];

self.addEventListener("install", (event) => {
  event.waitUntil(caches.open(CACHE).then((cache) => cache.addAll(FILES)).then(() => self.skipWaiting()));
});

self.addEventListener("activate", (event) => {
  event.waitUntil(
    caches.keys()
      .then((keys) => Promise.all(keys.filter((k) => k !== CACHE).map((k) => caches.delete(k))))
      .then(() => self.clients.claim()));
});

// Cache first; the network only for anything not precached. Query strings
// (?layout=) are ignored so they all hit the cached page.
self.addEventListener("fetch", (event) => {
  if (event.request.method !== "GET") return;
  event.respondWith(
    caches.match(event.request, { ignoreSearch: true })
      .then((hit) => hit || fetch(event.request)));
});
