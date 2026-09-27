#!/usr/bin/env python3
"""Serve a local checkout of the web port with cross-origin isolation.

Reference-only harness (docs/planning/port-comparison.md). Nothing is copied
into this repository: the root is DOD_WEB_PORT, outside the tree. POST /log
appends one JSON line per request to $DOD_COMPARE_OUT/web-trace.jsonl so the
page can emit traces without touching the hosted site.
"""
import http.server, json, os, sys, time

ROOT = os.environ.get("DOD_WEB_PORT", os.path.expanduser("~/projects/DungeonsOfDaggorath.github.io"))
OUT = os.environ.get("DOD_COMPARE_OUT", "captures/compare")
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8080


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **k):
        super().__init__(*a, directory=ROOT, **k)

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def do_POST(self):
        if self.path != "/log":
            self.send_error(404)
            return
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        os.makedirs(OUT, exist_ok=True)
        with open(os.path.join(OUT, "web-trace.jsonl"), "a") as f:
            f.write(json.dumps({"t": time.time(), "msg": body.decode("utf-8", "replace")}) + "\n")
        self.send_response(204)
        self.end_headers()

    def log_message(self, *a):
        pass


http.server.ThreadingHTTPServer.allow_reuse_address = True
print(f"serving {ROOT} on http://localhost:{PORT}/index.local.html", flush=True)
http.server.ThreadingHTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
