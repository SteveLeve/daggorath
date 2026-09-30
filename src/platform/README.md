# platform

Everything the simulation must not know about: SDL3, the rendering and audio
backends, filesystem and save storage, Android and iOS packaging.

Rules for this module:

- Dependencies point inward only. `core` never includes anything from here.
- Save/load must serialise scheduler and RNG state, not just player position —
  the original's cassette image carried whole RAM blocks, and a save without
  queue and seed state is not resumable to the same behaviour.
- `ZSAVE` still only fills the in-memory cassette. The window copies each
  named image into `dag::platform::Storage` (`storage.hpp`). On the desktop
  that is `<name>.dagram` under `SDL_GetPrefPath` (the `DAGRAM 1` payload, no
  extra envelope, written atomically). In the browser it is localStorage
  (ADR-0011 decision 7). Stored saves go back on the cassette at launch and
  on a menu Restart (D-20), and the player types `ZLOAD <name>`. A missing
  name reports `???` (D-11). A store that fails is reported, never silent.
- The system menu's five save slots (`DAGSNAP 1` snapshots, ADR-0009 §5/§6)
  persist through the same `Storage`: `slot<n>.dagsnap` on the desktop,
  `dod.slot.<n>` in the browser, each a `DODSLOT 1` envelope carrying the
  slot's display name. They are put back into the shell at launch and carried
  across a menu Restart. The hidden slot is not stored yet (ADR-0011
  decision 7).
- The Video and Controls menu choices persist through the same `Storage`
  (`prefs.hpp`, `dod.prefs`). `--layout` overrides the stored layout;
  `--shots` runs ignore preferences and slots.

`tools/check-sdl3-deps.sh` lists the Debian packages required to build SDL3 from source. `--install` runs `apt` for the missing ones. The desktop target stays optional: CMake skips it when SDL3 is absent.
