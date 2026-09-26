# platform

Everything the simulation must not know about: SDL3, the rendering and audio
backends, filesystem and save storage, Android and iOS packaging.

Rules for this module:

- Dependencies point inward only. `core` never includes anything from here.
- Save/load must serialise scheduler and RNG state, not just player position —
  the original's cassette image carried whole RAM blocks, and a save without
  queue and seed state is not resumable to the same behaviour.
- `ZSAVE` still only fills the in-memory cassette. The desktop window copies
  each named image to `SDL_GetPrefPath` as `<name>.dagram` (the `DAGRAM 1`
  payload, no extra envelope). `DEATH` remains `BRA *` inside the core. Once
  the player is dead the window stops delivering keystrokes and offers
  `R` to construct a new game or `L` plus a name to `restore_ram_image`.
  A living image clears that halt, because halt is the program counter and
  `LOAD90` returns to `SCHED`. A missing name reports `???`.

`tools/check-sdl3-deps.sh` lists the Debian packages required to build SDL3 from source. `--install` runs `apt` for the missing ones. The desktop target stays optional: CMake skips it when SDL3 is absent.
