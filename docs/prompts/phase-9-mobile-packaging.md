Carry out **Phase 9: Android and iOS packaging** for the Dungeons of Daggorath
preservation project. Read the charter, `docs/licensing/README.md`,
`docs/provenance/ledger.md`, ADR-0005, ADR-0006 and ADR-0007 first.

**Precondition — stop if unmet.** Licensing questions 1, 4 and 5 in
`docs/licensing/README.md` each have a recorded answer, or the owner has
recorded a decision to build for private devices only with no store
submission. Do not create store listings, icons using original artwork, or
branding without that record.

**Preservation requirement.** The mobile builds run the same core and the same
conformance suite as desktop. Platform lifecycle must not change game time
silently: decide what backgrounding does (suspend snapshot per ADR-0005 and
resume at the same jiffy, or continue), record it as a platform deviation, and
test it.

Work in this order.

1. Android: Gradle + CMake around the SDL3 app; arm64 and x86_64 emulator
   builds; conformance tests run on device or emulator.
2. iOS: CMake-generated Xcode project; simulator build; conformance tests run.
3. Storage: save files and suspend snapshots in app-private storage.
4. CI builds for both, without signing secrets in the repository.
5. Record new third-party build dependencies in the ledger.

**Do not build:** store submission, monetisation, analytics, enhanced modes.

**Completion gate.** Build logs for both platforms, conformance results on each,
lifecycle test results, the licensing record the precondition relied on.
