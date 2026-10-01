# Android build (Phase 9, ADR-0012)

Status: the debug APK builds on CI (workflow run 36778318482, 2026-09-30).
It has not yet been run on a device or emulator.

Private-device builds only (`docs/licensing/README.md` D5). Do not publish the
APK: it contains the copied original data listed in `DATA-NOTICE.md`.

## Requirements

- JDK 17 and Gradle 8.14 (`gradle` on PATH, or set `GRADLE=`)
- Android SDK with platform 35, and NDK `28.2.13676358` plus CMake 3.22+
  from the SDK manager. Point Gradle at the SDK with `ANDROID_HOME` or
  `android/local.properties` (`sdk.dir=...`, gitignored).
- `git`, to fetch the pinned SDL3 source into `third_party/SDL3`

## Build and install

```sh
make android          # fetches SDL3 release-3.4.2, builds a debug APK (arm64-v8a + x86_64)
make android-install  # adb install -r onto the attached device or emulator
```

The APK is `android/app/build/outputs/apk/debug/app-debug.apk`, signed with
the SDK's local debug key. No signing secret is in the repository.

Android Studio can open `android/` directly after `tools/android/fetch-sdl.sh`.

## Intended behaviour on device

None of this has been observed on a device or emulator yet (D-16 addendum,
**[OPEN]**).

- Landscape only. The layout defaults to `PhoneLandscape` on screens wider
  than 5:3 and to `Tablet4x3` otherwise. A layout stored in the preferences wins over
  that default.
- Switching away from the app opens the system menu, which pauses (D-16). No
  game time passes while you are away.
- The back key opens or backs out of the system menu, like Esc.
- Saves, slots and settings live in the app's private storage and are removed
  when the app is uninstalled.
