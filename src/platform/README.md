# platform

Everything the simulation must not know about: SDL3, the rendering and audio
backends, filesystem and save storage, Android and iOS packaging.

Rules for this module:

- Dependencies point inward only. `core` never includes anything from here.
- Save/load must serialise scheduler and RNG state, not just player position —
  the original's cassette image carried whole RAM blocks, and a save without
  queue and seed state is not resumable to the same behaviour.

`tools/check-sdl3-deps.sh` lists the Debian packages required to build SDL3 from source. `--install` runs `apt` for the missing ones. The desktop target stays optional: CMake skips it when SDL3 is absent.
