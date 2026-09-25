#!/bin/sh
# Replay the Phase 0b keystroke scripts with tools/rom/capture.lua.
# This is the procedure in docs/archaeology/phase-0b/traces/README.md §3.
# It does not invent a second one.
#
# Firmware stays outside the repository. Set DOD_FIRMWARE to a directory that
# contains bas13.rom (Color BASIC 1.3) and extbas11.rom (Extended Color BASIC
# 1.1), the pair in the owner's CoCo 2 and in MAME 0.264 coco2b. If either
# file is missing, or its SHA-1 is not the coco2b hash, this script exits 2
# and writes no trace.
#
# Optional:
#   DOD_MAME     mame binary (default: mame)
#   DOD_HASHPATH MAME hash directory, if the default is not installed
#   DOD_ROM      catalog 26-3093 cartridge image
#   DOD_JIFFIES  game interrupts per script after scheduler entry (default 200,
#                matching the dcli diffs). -seconds_to_run 120 is a backstop only.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
FIRMWARE=${DOD_FIRMWARE:-}
MAME=${DOD_MAME:-mame}
CART=${DOD_ROM:-"$ROOT/captures/Dungeons of Daggorath (1982) (26-3093) (Tandy).ccc"}
JIFFIES=${DOD_JIFFIES:-200}
SECONDS_TO_RUN=${DOD_SECONDS:-120}

if [ ! -f "$FIRMWARE/bas13.rom" ] || [ ! -f "$FIRMWARE/extbas11.rom" ]; then
    echo "capture not run: need bas13.rom and extbas11.rom in DOD_FIRMWARE. See docs/provenance/rom-diff.md" >&2
    exit 2
fi

python3 - "$FIRMWARE/bas13.rom" "$FIRMWARE/extbas11.rom" <<'PY'
import hashlib, pathlib, sys
expected = {
    "bas13.rom": "28b92bebe35fa4f026a084416d6ea3b1552b63d3",
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
mkdir -p "$STAGE/coco2b"
cp "$FIRMWARE/bas13.rom" "$FIRMWARE/extbas11.rom" "$STAGE/coco2b/"
mkdir -p "$ROOT/captures"

# A trace mismatch is a result, not a reason to skip the remaining scripts.
# Extra scripts may be passed as arguments. A name is a Phase 0b trace stem,
# or a path to a keystroke script (the capture stem is the filename minus .script).
status=0
if [ $# -gt 0 ]; then
    set -- "$@"
else
    set -- t1-move-turn-look t2-forward-corridor t3-burst-one-jiffy t4-parser-edges t5-keyboard-overrun
fi
for name in "$@"; do
    script="$name"
    stem=${DOD_STEM:-$(basename "$name" .script)}
    if [ ! -f "$script" ]; then
        script="$ROOT/docs/archaeology/phase-0b/traces/$stem.script"
    fi
    echo "capture $stem"
    DOD_SYMBOLS="$ROOT/build/rom/symbols.tsv" \
    DOD_WATCHES="$ROOT/tools/rom/watchlist.tsv" \
    DOD_SCRIPT="$script" \
    DOD_TRACE="$ROOT/captures/$stem.rom.trace" \
    DOD_RAW="$ROOT/captures/$stem.rom.raw.tsv" \
    DOD_JIFFIES="$JIFFIES" \
    "$MAME" \
        ${DOD_HASHPATH:+-hashpath "$DOD_HASHPATH"} \
        -rompath "$STAGE" \
        -video none -sound none -skip_gameinfo -nothrottle \
        -seconds_to_run "$SECONDS_TO_RUN" \
        -cfg_directory "$STAGE/cfg" -snapshot_directory "$STAGE/snap" \
        -nvram_directory "$STAGE/nvram" -diff_directory "$STAGE/diff" \
        coco2b -cart "$CART" \
        -autoboot_script "$ROOT/tools/rom/capture.lua"
    ref="$ROOT/docs/archaeology/phase-0b/traces/$stem.trace"
    if [ -f "$ref" ]; then
        python3 "$ROOT/tools/rom/trace_diff.py" \
            "$ref" \
            "$ROOT/captures/$stem.rom.trace" || status=1
    fi
done
exit "$status"
