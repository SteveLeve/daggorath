# ADR-0012 — Mobile packaging: Android first, iOS through the same shell

**Status:** Proposed, 2026-09-30 (Phase 9, first workstream).

## Context

Phase 9 wraps the Phase 7/8 SDL3 app (`src/platform/sdl_app.cpp`) for Android
and iOS. The owner will test on private Android devices first and wants a
path to iOS later. The precondition is met by a private-build-only decision,
`docs/licensing/README.md` D5.

## Decision

1. **One app, three shells.** Android, iOS, desktop and web all compile the
   same `sdl_app.cpp` against the same core, presentation, input, shell and
   platform libraries. There is no mobile-only game code. `src/core` gains
   nothing.
2. **Android: Gradle + the root CMake project.** `android/` is a Gradle
   project whose `externalNativeBuild` is the repository's root
   `CMakeLists.txt`. Under `ANDROID`, `src/platform` builds the app as
   `libmain.so` and SDL3 from source. SDL's Java `SDLActivity` is compiled from
   the same pinned tree. `DodActivity` only names the two libraries. ABIs are
   `arm64-v8a` (devices) and `x86_64` (emulator). **Not run:** no Android SDK
   was reachable when this was written; the first CI run is the first build.
3. **SDL3 is pinned, fetched, never committed.** `tools/android/fetch-sdl.sh`
   clones `release-3.4.2` into `third_party/SDL3` and refuses any commit other
   than `683181b4…`. This is the SDL version the web build ships
   (ledger §6, pinned there by the emsdk port's SHA-512). The desktop build
   still takes whatever SDL3 `find_package` finds.
4. **iOS: the same shape, not built yet.** SDL3 builds for iOS from the same
   tree with CMake's Xcode generator (`-G Xcode -DCMAKE_SYSTEM_NAME=iOS`).
   `sdl_app.cpp` already takes SDL's `SDL_main` entry and the landscape and
   layout choices on `SDL_PLATFORM_IOS`. The `ios/` wrapper, signing and a
   simulator build need a macOS host and come after Android works on device.
   Nothing here blocks that.
5. **Lifecycle: backgrounding pauses (D-16).** When the OS sends
   `SDL_EVENT_WILL_ENTER_BACKGROUND`, the app opens the system menu, which
   pauses the shell. No jiffy is owed for time away, and play resumes only
   when the player leaves the menu. The Android back key (`SDLK_AC_BACK`) acts
   like Esc. The alternative, letting time run on, would make a phone call
   kill the player; the original offers no such situation, so pausing is the
   smaller deviation. **Open:** Android may kill a backgrounded process.
   Surviving that needs the shell's hidden slot (ADR-0009 §6) written to
   storage on background and restored at launch. That is the next workstream.
6. **Storage.** Saves, menu slots and preferences go through
   `dag::platform::Storage` at `SDL_GetPrefPath`, which on Android is the
   app's private internal storage. The manifest sets `allowBackup="false"`
   and asks for no permissions.
7. **No distribution.** CI builds the APK and uploads nothing
   (`.github/workflows/android.yml`). Devices get a build from a local
   `make android-install`. The app id ends in `.dev`, the label says
   "private", and the launcher uses Android's default icon: no store listing,
   branding or original artwork (licensing D3, D5, Q5).

## Consequences

- The Android conformance run (Phase 9 prompt step 1) is not in this
  workstream. The suite is headless, so the plan is to cross-compile
  `conformance_tests` and run it through `adb shell` on the emulator.
- Adding `SDL_EVENT_WILL_ENTER_BACKGROUND` handling to the shared loop may
  also pause the web build when a tab is hidden, if SDL's Emscripten backend
  sends that event. **[INF]** Not checked; D-21's addendum records the question.
