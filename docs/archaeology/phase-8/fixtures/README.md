# Phase 8 fixtures

Unlike `docs/archaeology/phase-0b/fixtures/` and the other phase fixture
directories, nothing here is extracted from the pinned assembly listing. Phase
8 is an adaptation layer (touch input, shell, render style), so its fixtures
capture this project's own design choices, checked for regression stability
by the corresponding test, not the original's behaviour. `make verify` and
`MANIFEST.json` do not cover this directory.

- `gesture-lines.txt` — the gesture-to-command-line mapping
  (`docs/design/touch-controls/README.md`,
  `docs/architecture/touch-input.md`), checked by
  `tests/input/gesture_tests.cpp`.
