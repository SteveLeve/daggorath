#!/usr/bin/env bash
# Stop: `make verify` must pass before a session ends with fixture changes.
cd "$CLAUDE_PROJECT_DIR" || exit 0
git diff --quiet HEAD -- docs/archaeology/phase-0b/fixtures tools 2>/dev/null && exit 0
if ! out=$(make -s verify 2>&1); then
  echo "make verify failed; fix or record the reason in reconciliation.md:" >&2
  echo "$out" | tail -20 >&2
  exit 2
fi
exit 0
