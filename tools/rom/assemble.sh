#!/bin/sh
# Assemble the pinned listing with lwasm and, when a retail image is named,
# hash both and record the byte diff.
#
#   DOD_ROM=/path/to/image.rom tools/rom/assemble.sh
#
# The image is never copied into the repository. If lwasm is not installed
# this script exits 2 and leaves docs/provenance/rom-diff.md alone: that
# document is the record of the attempt, and this script must not replace it
# with a guess.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
ASM=${ASM_DIR:-"$ROOT/third_party/dod-asm"}
OUT=${ROM_OUT:-"$ROOT/build/rom"}

if ! command -v lwasm >/dev/null 2>&1; then
    echo "lwasm is not installed; the recorded result is docs/provenance/rom-diff.md" >&2
    exit 2
fi
if [ ! -f "$ASM/DAGGORATH.ASM" ]; then
    echo "listing not present at $ASM; run 'make sources'" >&2
    exit 2
fi

mkdir -p "$OUT"
lwasm --6809 --format=raw --list="$OUT/daggorath.lst" --output="$OUT/daggorath.bin" \
    "$ASM/DAGGORATH.ASM"
python3 - "$OUT/daggorath.bin" "$OUT/daggorath.lst" ${DOD_ROM:-} <<'PY'
import hashlib, pathlib, sys
built = pathlib.Path(sys.argv[1]).read_bytes()
listing = pathlib.Path(sys.argv[2]).read_text(errors="replace")
rom_path = sys.argv[3] if len(sys.argv) > 3 and sys.argv[3] else ""
print(f"built_bytes {len(built)}")
print(f"built_sha256 {hashlib.sha256(built).hexdigest()}")
# Symbol table: "SYMBOL EQU $ADDR" style lines from an lwasm listing are not
# stable across versions, so keep the raw listing beside the image under build/.
print(f"listing_lines {listing.count(chr(10))}")
if not rom_path:
    print("retail_rom absent")
    sys.exit(0)
rom = pathlib.Path(rom_path).read_bytes()
print(f"retail_bytes {len(rom)}")
print(f"retail_sha256 {hashlib.sha256(rom).hexdigest()}")
n = min(len(built), len(rom))
diffs = [i for i in range(n) if built[i] != rom[i]]
if len(built) != len(rom):
    print(f"length_delta {len(built) - len(rom)}")
print(f"differing_bytes {len(diffs) + abs(len(built) - len(rom))}")
for i in diffs[:64]:
    print(f"diff {i:04X} built={built[i]:02X} rom={rom[i]:02X}")
if len(diffs) > 64:
    print(f"diff_truncated {len(diffs) - 64}")
PY
