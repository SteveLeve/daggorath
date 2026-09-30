#!/usr/bin/env bash
# Fetch the pinned SDL3 source tree into third_party/SDL3 (gitignored) for the
# Android build (Phase 9, ADR-0012). SDL's Java activity and native library
# are compiled from this tree; none of it is committed. The pin matches the
# SDL3 the web build ships (docs/provenance/ledger.md §6 and §7).
set -euo pipefail
tag=release-3.4.2
commit=683181b47cfabd293e3ea409f838915b8297a4fd
root="$(cd "$(dirname "$0")/../.." && pwd)"
dest="$root/third_party/SDL3"
if [ ! -d "$dest/.git" ]; then
  git clone --quiet --depth 1 --branch "$tag" https://github.com/libsdl-org/SDL "$dest"
fi
got="$(git -C "$dest" rev-parse HEAD)"
if [ "$got" != "$commit" ]; then
  echo "third_party/SDL3 is at $got, expected $tag = $commit" >&2
  exit 1
fi
echo "SDL3 $tag ($commit) in third_party/SDL3"
