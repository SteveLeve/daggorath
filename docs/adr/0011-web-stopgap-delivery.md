# ADR-0011 — Web stopgap: WebAssembly PWA build (local only; Cloudflare Workers config)

**Status:** Accepted, 2026-09-28.

## Context

Phase 9 (native Android and iOS) is gated on licensing questions 1, 4 and 5
(`docs/licensing/README.md`) and is a large piece of work. Phase 8 already
built the phone-oriented UI (touch overlay, `PhoneLandscape` layout, system
menu) inside the SDL3 platform shell, `src/platform/sdl_app.cpp`. A browser
build of that same shell is a short route to playing and testing on real
phones before Phase 9. The model is daggorath.online (served from `DungeonsOfDaggorath/DungeonsOfDaggorath.github.io`, ledger §3), the cognitivegears SDL2
port as WASM, which the ledger records as reference-only. Nothing from it is
used here: no code, WASM, manifest, service worker or icons.

This work is outside the charter's phase sequence (roadmap track W). The
owner requested it on 2026-09-28 while Phase 8 was open. It adds a delivery
target and changes no game behaviour.

## Decision

1. **Monorepo platform target.** The web build is a second build of the
   existing `dod` target. It is not a separate repository or a fork of the
   shell. Emscripten's SDL3 port (`--use-port=sdl3`) compiles
   `sdl_app.cpp` unchanged apart from `#ifdef __EMSCRIPTEN__` blocks, so a
   change to the shell cannot silently leave the web build behind.
   `web/` (page shell, manifest, service worker, headers, Wrangler config)
   only consumes the build output and can move to its own repository if
   branding or licensing ever calls for it.
2. **The main loop stays as it is.** It is built with `-sASYNCIFY`. SDL 3.4's
   `SDL_Delay` yields to the browser under ASYNCIFY (`emscripten_sleep`), so
   the blocking `while (running)` loop and its animation delays run
   unmodified. Refactoring to SDL main callbacks (`SDL_AppIterate`) is
   deferred. That refactor is also what iOS wants, and belongs with Phase 9.
3. **Input discipline is unchanged.** Every input reaches the core through
   the SDL keyboard and pointer events the desktop build already handles.
   The page has no HTML controls that inject commands (the reference site's
   `ccall('sendinput')` pattern would be a side door).
4. **Browser specifics are confined to `#ifdef __EMSCRIPTEN__` blocks and
   `src/platform/web/`:**
   - *Saves, menu slots and preferences.* See decision 7.
   - *Canvas sizing.* The window is `SDL_WINDOW_RESIZABLE` and the page's CSS
     sizes the canvas. The game keeps drawing in its fixed window coordinates
     and `SDL_SetRenderLogicalPresentation(..., LETTERBOX)` scales them to fit.
     Pointer events are mapped back with `SDL_ConvertEventToRenderCoordinates`.
     `SDL_WINDOW_HIGH_PIXEL_DENSITY` is deliberately not set: with it,
     SDL 3.4 writes inline CSS sizes onto the canvas, including a 1×1 one when
     the WebGL renderer recreates the window, and these override the page. The
     Controls entry changes only the logical size; it does not rebuild the window.
   - *Stack.* `-sSTACK_SIZE=8MB`. The loop keeps full-screen `std::array`
     frames as locals, and Emscripten's 64 KB default overflows at start-up.
5. **Hosting: a static-assets-only Cloudflare Worker** (`web/wrangler.jsonc`,
   `assets.directory = ./dist`, no Worker script, so `_headers` applies to
   every response). No threads are used, so no COOP/COEP headers are needed.
6. **Visibility (owner decision, 2026-09-28): local only, not deployed.**
   Serving `dod.wasm` from any URL other people can reach distributes the
   extracted original tables (licensing Q4, from the source the Q2 grant
   covers). The charter requires qualified legal review before public
   distribution. So the build runs only under `wrangler dev` on the
   developer's machine or LAN, and it is not deployed until one of these is
   recorded in `docs/licensing/README.md`: answers to Q1, Q2, Q4 and Q5, or a
   private deployment behind an access gate (Cloudflare Access). An earlier
   draft of this decision (hosted publicly but unlisted) was withdrawn the same
   day after the evidence audit. CI builds the target on PRs, uploads no
   artifact, and has no deploy step. The Worker config and the noindex
   headers (`_headers`, `robots.txt`, meta tag) stay in place so that a later
   gated deployment starts unlisted. A custom domain and store-style
   presentation wait for Q5 (name and branding).
7. **Storage: one API, two backends, failures told to the player
   (2026-09-29).** `src/platform/include/daggorath/storage.hpp` stores ZSAVE
   images, the system menu's five save slots and preferences.
   - *Desktop* (`file_storage.cpp`): `<name>.dagram`, `slot<n>.dagsnap` and
     `dod.prefs` in the `SDL_GetPrefPath` directory, each written to a
     temporary file and renamed into place.
   - *Browser* (`web/web_storage.cpp`): localStorage keys `dod.save.<NAME>`,
     `dod.slot.<n>` and `dod.prefs`. This replaces the first draft's IDBFS mount. IDBFS synced
     asynchronously and only logged failures, so a ZSAVE could look
     successful while it existed only in memory; a reload then lost it, and
     after a failed load a later flush could overwrite IndexedDB with an
     incomplete directory (code review, 2026-09-29). localStorage is
     synchronous: `setItem` updates the storage area or throws before
     `store_save` returns, so a failure is known at once. When the browser
     writes that area to disk is up to the browser. The desktop backend
     renames without an fsync, so neither backend guarantees the bytes
     survive an OS or browser crash in the moment after a save.
     A ZSAVE image is about 8 KB of text. A menu slot is about 8.5 KB plus
     about 8 KB for each entry on the cassette it carries (measured
     2026-09-29 with `Game::snapshot()`). The cassette has no limit: every
     ZSAVE is appended, repeats of a name included, and the stored saves are
     mounted at start-up. So the total, shared by the whole origin under the
     usual 5 MB quota, grows with play and can reach it. A store that hits it
     is reported as NOT STORED, like any other failed store.
   - *Reporting.* A failed store, or storage that cannot be read at start-up,
     is reported through `report_storage_problem`: stderr on the desktop, a
     notice over the top of the page in the browser. Notices that arrive
     while one is showing are listed together, so a later one never hides an
     earlier failure. Nothing is drawn on the
     game screen, which belongs to the core's text page. The in-game ZSAVE
     still fills the in-memory cassette, so an unstored save stays loadable
     for the rest of the session, and the notice says so.
   - *Menu slots* (added 2026-09-29, after the owner found on localhost that
     a menu save never reached localStorage). Until then the five slots
     (ADR-0009 §6) lived only in the running `Shell` on both platforms, so a
     reload or relaunch emptied them, and so did a menu Restart. That mattered
     most on touch, where the menu is the way to save (ZSAVE needs the `⌨`
     keyboard). Now:
     - Each slot a menu key fills (a save, or a confirmed overwrite) is stored
       at once as `DODSLOT 1\n<name>\n<DAGSNAP 1 snapshot>`. The envelope
       carries the slot's display name, which the shell derives from the game
       at save time and cannot recompute without restoring it.
     - Stored slots go back into the shell at start-up through
       `Shell::put_slot`, which touches no state of the running game. Stored
       bytes may be damaged, so `put_slot` checks more than
       `load_from_slot`'s header check: it restores the snapshot into a
       scratch `Game` and accepts it only if that game's snapshot reproduces
       the bytes exactly (about 0.3 ms per slot, measured natively
       2026-09-29). A slot that fails is reported as `SAVE SLOT <n> DAMAGED`
       and left empty in the menu. Loading a slot is still a menu choice.
     - The core's image reader trusted the counts and lengths in the body, so
       an edited size field could exhaust memory in that trial, at every
       start-up (evidence audit, 2026-09-29). `read_bounded_count`
       (`scheduler.hpp`) now fails the read when a count exceeds the bytes
       remaining, since each element takes at least one byte. A well-formed
       image never trips it. The same reader serves `DAGSNAP 1` and
       `DAGRAM 1`, so the bound also applies to a stored ZSAVE at ZLOAD. Only
       the snapshot's tape count is tested (`shell_tests`); the other bounded
       fields, and the ZLOAD path, are bounded by the same helper but not
       tested. A damaged image that stays within its bounds can still load
       partial state at ZLOAD; only slots get the exact round-trip check.
     - The round-trip check assumes `snapshot()` is canonical: restoring and
       re-snapshotting gives the same bytes. That holds for a fresh game and
       for the test script's mid-game state (`shell_tests`), not for every
       state. A slot that failed it would be reported as DAMAGED at launch,
       or when a menu Restart carries it across.
     - A slot carries its game's cassette (ADR-0009 §5). Loading a slot
       stored in an earlier session therefore replaces the cassette mounted
       at start-up. ZSAVEs made after that slot drop off the in-memory tape
       until the next launch or menu Restart mounts them again. They stay in
       storage. ADR-0009 §5's rollback now reaches across sessions.
     - Every menu save is stored, a failed one included, so saving again
       retries. A failed store is reported as `SAVE SLOT <n> NOT STORED`, and
       the slot stays in the shell for the session.
     - A menu Restart now carries all five slots into the new `Shell`. They
       belong to the player, not to the abandoned game, and this keeps an
       unstored slot's "kept for this session" true. A slot the new shell
       refuses is reported. Like the ZSAVE carry, it lives in `sdl_app.cpp`
       and has no automated test.
     - `--shots` runs neither read nor write slots, as for preferences.
     - The hidden slot is not stored. Nothing writes it yet, because the
       backgrounding hook (ADR-0009 §6) is unbuilt. Storing it belongs with that
       hook.
   - *Preferences.* The system menu's Video (pixel/crisp) and Controls
     (phone/tablet) choices are remembered on both platforms (`prefs.hpp`). At
     start-up an explicit `--layout` wins, then the remembered layout, then
     `--default-layout` (the page derives one from the screen's shape), then
     Tablet4x3. `--shots` runs neither read nor write preferences, so scripted
     screenshots stay the same on every machine.
   - *Menu restart.* Restart from the system menu now puts the stored saves
     back on the new game's cassette, as a launch does (D-20). Before this,
     they were missing until the next launch. A menu Restart builds a new
     `Game`, whereas a death restart keeps the core's cassette (D-18). So any
     ZSAVE this session failed to store is also carried across, taking
     precedence over a stored save of the same name, which keeps the notice's
     "kept for this session" true. That carry lives in `sdl_app.cpp` and has
     no automated test.
   - *Eviction.* Browsers may still evict localStorage. WebKit's tracking
     prevention, as announced in 2020, reportedly deletes a site's
     script-writable storage after 7 days of Safari use without interaction
     with that site, and exempts sites installed to the home screen. Not
     checked here. The page asks for `navigator.storage.persist()`,
     but neither backend choice removes this risk.

## Consequences

- `make all` stays emsdk-free. `make web` needs an active emsdk and writes
  `web/dist/`, which is gitignored. The first build fetches the SDL3 port
  into emsdk's cache.
- The offline cache name in `sw.js` comes from a hash of the built files, so
  each build replaces the previous cache.
- iOS ignores the manifest's `orientation`. The page shows a "turn the device"
  notice in portrait on touch devices.
- The canvas renders at CSS-pixel resolution, not device-pixel resolution
  (see decision 4). Sharper output on high-DPI screens would need SDL's
  high-density path to stop overriding page CSS, or a page-side workaround.
- Audio starts on the first user gesture, as browsers require. The 6000 Hz
  U8 stream is resampled by SDL.
- Frame pacing comes from the browser's timers under ASYNCIFY, not a native
  sleep. Jiffies are still counted from elapsed time (`dag::jiffies_due`), so
  the simulation rate is unchanged. The deviation is presentation-only and is
  listed in `docs/specification/clock-and-scheduler.md` §13 as D-21.

## Evidence and open questions

- Verified 2026-09-28 in desktop Chrome against `wrangler dev`: start-up,
  keyboard commands, on-screen button taps (coordinate mapping), the system
  menu, the Controls layout switch, and ZSAVE → reload → ZLOAD. That check
  ran on the first draft's IDBFS backend. For decision 7's localStorage
  backend, the test below shows the image stored and put back on the
  cassette after a reload (`saves=1`). It does not type ZLOAD, and a
  ZLOAD through the new backend has not been checked by hand. Decision 7 is
  covered on every PR by `make web-test` (`tools/web/storage-test.mjs`,
  headless Chrome, 22 checks):
  - ZSAVE is stored, and a reload mounts it.
  - A menu slot save is stored with its name, and a reload puts it back
    (`slots=1`). Loading it lets play continue: a ZSAVE afterwards is
    stored. A confirmed overwrite replaces the stored slot. Whether the
    loaded game is the saved one is checked headlessly instead
    (`shell_tests`: a slot put into a fresh shell restores the exact
    snapshot, and a truncated one or one with an absurd tape count is
    refused).
  - A damaged stored slot is reported and left out of the menu.
  - Video and Controls survive a reload.
  - A `setItem` that throws QuotaExceededError is reported and leaves nothing
    behind, for a ZSAVE, a settings change and a slot save. The notices
    are listed together.
  - Unreadable storage (SecurityError) is reported, and play still starts.

  The test was run once against a deliberately broken `store_save` that
  ignored write errors (2026-09-29, the tree committed as c918382), and it
  failed. It was also run against a `store_slot` that did the same
  (2026-09-29, c918382 plus this change's uncommitted tree, 22-check
  test), and it failed with "timed out waiting for slot notice". No
  artifact was kept. The
  desktop backend is covered by `tests/platform/storage_tests.cpp`
  (`storage_tests`): round trips, skipping invalid files, writing through a
  temporary file with none left behind (an interrupted write is not tested),
  reporting an unwritable directory, and the same for slots, including a
  damaged slot file being skipped. Not yet verified: real iOS Safari and Android Chrome, Add to
  Home Screen, offline start, and audio on the first tap.
- **[OPEN] Deployment is blocked on licensing** (decision 6), the same way
  Phase 9 is. Phase 9's precondition also accepts "a recorded decision to
  build privately only", and an access-gated deployment would be the web
  equivalent.
- Testing on phones over the LAN with `wrangler dev --ip 0.0.0.0` works for
  play. A service worker, and so the offline cache and a full PWA install,
  needs HTTPS or `localhost`, so those parts cannot be verified that way.
