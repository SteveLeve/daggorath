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
python3 - "$OUT/daggorath.bin" "$OUT/daggorath.lst" "$ROOT/tools/rom/watchlist.tsv" "${DOD_ROM:-}" <<'PY'
import hashlib, pathlib, sys
built = pathlib.Path(sys.argv[1]).read_bytes()
listing = pathlib.Path(sys.argv[2]).read_text(errors="replace")
watch = pathlib.Path(sys.argv[3])
rom_path = sys.argv[4] if len(sys.argv) > 4 and sys.argv[4] else ""
print(f"built_bytes {len(built)}")
print(f"built_sha256 {hashlib.sha256(built).hexdigest()}")
print(f"listing_lines {listing.count(chr(10))}")

# LWTOOLS 4.25 listing: a label sits nine spaces after the five-digit line
# number. A mnemonic sits further right. Macro temps such as FOO are redefined;
# the last definition is the one written.
import re
label_re = re.compile(
    r"^\s*([0-9A-Fa-f]{4})\b.*\):\d+( {9})([A-Za-z._][\w.$]*)\s",
    re.M,
)
symbols = {}
for match in label_re.finditer(listing):
    symbols[match.group(3)] = match.group(1).upper()
for line in listing.splitlines():
    if "JSR" in line and "[P.TCRTN,U]" in line:
        addr = re.match(r"\s*([0-9A-Fa-f]{4})\b", line)
        if addr:
            symbols["SCHED_JSR"] = addr.group(1).upper()
        break
sym_path = pathlib.Path(sys.argv[2]).with_name("symbols.tsv")
ordered = sorted(symbols)
sym_path.write_text(
    "".join(f"{name}\t{symbols[name]}\n" for name in ordered)
)
print(f"symbols {len(symbols)} {sym_path}")
missing = []
if watch.is_file():
    for raw in watch.read_text().splitlines():
        if not raw or raw.startswith("#"):
            continue
        name = raw.split("\t", 1)[0]
        if name not in symbols:
            missing.append(name)
if missing:
    print("watch_symbols_missing " + ",".join(missing))
    sys.exit(1)
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
