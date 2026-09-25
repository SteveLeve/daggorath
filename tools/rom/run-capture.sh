#!/bin/sh
# Replay the Phase 0b keystroke scripts with tools/rom/capture.lua.
# This is the procedure in docs/archaeology/phase-0b/traces/README.md §3.
# It does not invent a second one.
#
# Firmware stays outside the repository. Set DOD_FIRMWARE to a directory that
# contains bas12.rom (Color BASIC 1.2) and extbas11.rom (Extended Color BASIC
# 1.1). If either file is missing, or its SHA-1 is not the MAME 0.264 coco
# BIOS b12e11 hash, this script exits 2 and writes no trace.
#
# Optional:
#   DOD_MAME     mame binary (default: mame)
#   DOD_HASHPATH MAME hash directory, if the default is not installed
#   DOD_ROM      catalog 26-3093 cartridge image
#   DOD_JIFFIES  sampled frames per script (default 200, matching the dcli diffs)
set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
FIRMWARE=${DOD_FIRMWARE:-}
MAME=${DOD_MAME:-mame}
CART=${DOD_ROM:-"$ROOT/captures/Dungeons of Daggorath (1982) (26-3093) (Tandy).ccc"}
JIFFIES=${DOD_JIFFIES:-200}

if [ ! -f "$FIRMWARE/bas12.rom" ] || [ ! -f "$FIRMWARE/extbas11.rom" ]; then
    echo "capture not run: need bas12.rom and extbas11.rom in DOD_FIRMWARE. See docs/provenance/rom-diff.md" >&2
    exit 2
fi

python3 - "$FIRMWARE/bas12.rom" "$FIRMWARE/extbas11.rom" <<'PY'
import hashlib, pathlib, sys
expected = {
    "bas12.rom": "0f14dc46c647510eb0b7bd3f53e33da07907d04f",
    "extbas11.rom": "ad927fb4f30746d820cb8b860ebb585e7f095dea",
}
for path in sys.argv[1:]:
    data = pathlib.Path(path).read_bytes()
    digest = hashlib.sha1(data).hexdigest()
    name = pathlib.Path(path).name
    if len(data) != 8192 or digest != expected[name]:
        sys.stderr.write(
            f"capture not run: {name} is {len(data)} bytes, SHA-1 {digest}, "
            f"expected 8192 bytes and {expected[name]}\n"
        )
        raise SystemExit(2)
    print(f"firmware_ok {name} {digest}")
PY

if [ ! -x "$MAME" ] && ! command -v "$MAME" >/dev/null 2>&1; then
    echo "capture not run: mame not found ($MAME)" >&2
    exit 2
fi
if [ ! -f "$ROOT/build/rom/symbols.tsv" ]; then
    echo "capture not run: build/rom/symbols.tsv is missing; run tools/rom/assemble.sh" >&2
    exit 2
fi
if [ ! -f "$CART" ]; then
    echo "capture not run: cartridge not found at $CART" >&2
    exit 2
fi

STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/coco"
cp "$FIRMWARE/bas12.rom" "$FIRMWARE/extbas11.rom" "$STAGE/coco/"
mkdir -p "$ROOT/captures"

for name in t1-move-turn-look t2-forward-corridor t3-burst-one-jiffy t4-parser-edges t5-keyboard-overrun; do
    echo "capture $name"
    DOD_SYMBOLS="$ROOT/build/rom/symbols.tsv" \
    DOD_WATCHES="$ROOT/tools/rom/watchlist.tsv" \
    DOD_SCRIPT="$ROOT/docs/archaeology/phase-0b/traces/$name.script" \
    DOD_TRACE="$ROOT/captures/$name.rom.trace" \
    DOD_RAW="$ROOT/captures/$name.rom.raw.tsv" \
    DOD_JIFFIES="$JIFFIES" \
    "$MAME" \
        ${DOD_HASHPATH:+-hashpath "$DOD_HASHPATH"} \
        -rompath "$STAGE" \
        -video none -sound none -skip_gameinfo \
        coco -cart "$CART" \
        -autoboot_script "$ROOT/tools/rom/capture.lua"
    python3 "$ROOT/tools/rom/trace_diff.py" \
        "$ROOT/docs/archaeology/phase-0b/traces/$name.trace" \
        "$ROOT/captures/$name.rom.trace"
done
