#!/usr/bin/env python3
"""Extract text/map presentation fixtures from the pinned listing.

Independent of the C++ projection. MAPPER, EXAMIN, STATUX, TXTSER, COMDAT
and SWCHAR are read from the assembly; maze bytes and creature positions
come from the already-extracted Phase 0b fixtures.

Usage: extract_text.py <asm-dir> <out-dir> <generated-header> [phase-0b-fixtures]
"""
from __future__ import annotations

import hashlib
import json
import os
import re
import sys

CHARS = {0x00: " ", 0x1B: "!", 0x1C: "_", 0x1D: "?", 0x1E: ".", 0x1F: "\n"}
CHARS.update({i: chr(ord("A") + i - 1) for i in range(1, 27)})


def read(asmdir, name):
    with open(os.path.join(asmdir, name), "r", errors="replace") as f:
        return f.read()


def fcb_values(text):
    out = []
    for i, line in enumerate(text.splitlines(), 1):
        code = line.split(";", 1)[0]
        m = re.match(r"^\s*(?:[A-Z0-9_.$]+\s+)?FCB\s+(.+)$", code)
        if not m:
            continue
        vals = []
        ok = True
        for tok in m.group(1).split(","):
            tok = tok.strip()
            if re.fullmatch(r"%[01]{8}", tok):
                vals.append(int(tok[1:], 2))
            elif re.fullmatch(r"-?\d+", tok):
                vals.append(int(tok) & 0xFF)
            elif re.fullmatch(r"\$[0-9A-Fa-f]{1,2}", tok):
                vals.append(int(tok[1:], 16))
            else:
                ok = False
                break
        if ok and vals:
            out.append((i, vals))
    return out


def decode_outsti_fields(bits):
    fields = [int(bits[k:k + 5], 2) for k in range(0, len(bits) - 4, 5)]
    count = fields[0]
    return "".join(CHARS.get(c, "?") for c in fields[1:count + 2])


def pexam_strings(text):
    lines = text.splitlines()
    found = []
    i = 0
    while i < len(lines):
        if re.search(r"FCB\s+OUTSTI", lines[i]):
            bits = ""
            j = i + 1
            while j < len(lines) and re.match(r"\s+FCB\s+%[01]{8}", lines[j]):
                bits += re.search(r"%([01]{8})", lines[j]).group(1)
                j += 1
            if bits:
                found.append((i + 1, decode_outsti_fields(bits)))
            i = j
        else:
            i += 1
    return found


def packed_fcbs_between(text, start_label, end_label):
    lines = text.splitlines()
    start = end = None
    for i, line in enumerate(lines):
        if re.match(r"^%s\b" % re.escape(start_label), line):
            start = i
        if start is not None and i > start and re.match(r"^%s\b" % re.escape(end_label), line):
            end = i
            break
    if start is None or end is None:
        raise KeyError("%s .. %s" % (start_label, end_label))
    body = "\n".join(lines[start:end])
    vals = []
    for lineno, row in fcb_values(body):
        vals.extend(row)
        _ = lineno
    return vals, start + 1


# --------------------------------------------------------------------------
# Independent transliteration of MAPPER.ASM / EXAMIN / STATUX
# --------------------------------------------------------------------------


def object_name(obj, adj, gen):
    if obj is None:
        return "EMPTY"
    generic = gen[obj["cls"]]["word"]
    if obj.get("reveal", 0) != 0:
        return generic
    return adj[obj["type"]]["word"] + " " + generic


def vft_pointer(tab, level):
    x = 0
    b = level
    n = len(tab)
    while True:
        at = x
        while x < n and tab[x] < 0x80:
            x += 1
        if x < n:
            x += 1
        b = (b - 1) & 0xFF
        if b & 0x80:
            return at


def vft_marks(tab, level):
    x = vft_pointer(tab, level)
    marks = []
    n = len(tab)

    def scan():
        nonlocal x
        while x < n:
            code = tab[x]
            x += 1
            if code >= 0x80:
                return
            row = tab[x]
            col = tab[x + 1]
            x += 2
            marks.append((row, col))

    scan()
    scan()
    return marks


def project_map(cells, player, features, objects, creatures, verticals):
    """MAPPER.ASM: occupiable vs $FF, then optional objects/creatures, player, VFTTAB."""
    lines = ["MAP features=%d" % (1 if features else 0)]
    for i, b in enumerate(cells):
        if b == 0xFF:
            lines.append("SOLID %d %d" % (i // 32, i % 32))
    if features:
        for r, c in sorted(objects):
            lines.append("OBJECT %d %d" % (r, c))
        for r, c in sorted(creatures):
            lines.append("CREATURE %d %d" % (r, c))
    lines.append("PLAYER %d %d" % (player[0], player[1]))
    for r, c in sorted(set(verticals)):
        lines.append("VFEATURE %d %d" % (r, c))
    return "\n".join(lines) + "\n"


class TextPad:
    """TXTSER/COMTXT cursor: low 5 bits column, rest line. 32 columns."""

    def __init__(self, rows):
        self.rows = rows
        self.cols = 32
        self.grid = [[" "] * 32 for _ in range(rows)]
        self.cur = 0
        self.inverse_next = False
        self.inv = set()

    def line_col(self):
        return self.cur >> 5, self.cur & 31

    def put(self, ch):
        if ch == "\n":
            self.cur = (self.cur + 32) & ~31
            return
        r, c = self.line_col()
        if 0 <= r < self.rows and 0 <= c < self.cols:
            mark = "*" if self.inverse_next else ch
            self.grid[r][c] = mark
            if self.inverse_next:
                self.inv.add((r, c))
        self.cur += 1
        self.inverse_next = False

    def write(self, s):
        for ch in s:
            self.put(ch)

    def to_text(self):
        last = 0
        for i, row in enumerate(self.grid):
            if "".join(row).strip():
                last = i
        return "".join("".join(self.grid[i]) + "\n" for i in range(last + 1))


def project_examine(creature, floor_names, bag_names, torch_index):
    """PEXAM.ASM EXAMIN. Cursor starts at 10. PRTOBJ tabs to a 16-column grid."""
    pad = TextPad(19)
    pad.cur = 10
    pad.write("IN THIS ROOM\n")
    if creature:
        pad.cur += 11
        pad.write("!CREATURE!\n")
    newline = False
    for name in floor_names:
        pad.write(name)
        newline = not newline
        if newline:
            pad.cur = (pad.cur + 16) & ~15
        else:
            pad.put("\n")
    if newline:
        pad.put("\n")
    pad.write("!" * 32)
    pad.cur += 12
    pad.write("BACKPACK\n")
    newline = False
    for i, name in enumerate(bag_names):
        if i == torch_index:
            pad.inverse_next = True
        pad.write(name)
        newline = not newline
        if newline:
            pad.cur = (pad.cur + 16) & ~15
        else:
            pad.put("\n")
    return pad.to_text()


def project_status(left, right, heart):
    """STATUX: 15 spaces at 0 and 17, names, heart at column 15 (COMMON.ASM CLK30)."""
    row = [" "] * 32
    left = left[:15]
    right = right[:15]
    row[0:len(left)] = list(left)
    start = 32 - len(right)
    if start < 17:
        start = 17
    row[start:start + len(right)] = list(right)
    if heart == "small":
        row[15], row[16] = "s", "s"
    elif heart == "large":
        row[15], row[16] = "L", "L"
    return "STATUS " + "".join(row) + "\n"


def project_command(line):
    """MISC.ASM PROMPT is CR, '.', underline, BS. LINBUF follows."""
    return "COMMAND .\nLINE %s\n" % line


def load_tokens(path):
    doc = json.load(open(path))
    return doc["tables"]["ADJTAB"], doc["tables"]["GENTAB"]


def load_vft(path):
    doc = json.load(open(path))
    return [int(b, 16) for b in doc["raw_bytes"]]


def load_creatures(path, level):
    out = []
    with open(path) as f:
        for line in f:
            if line.startswith("creature %d 1 " % level):
                p = line.split()
                out.append((int(p[5]), int(p[6])))
    return out


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def write_json(path, obj):
    text = json.dumps(obj, indent=2) + "\n"
    with open(path, "w") as f:
        f.write(text)
    return text.encode()


def main():
    if len(sys.argv) < 4:
        print(__doc__.strip())
        return 2
    asmdir, outdir, header = sys.argv[1], sys.argv[2], sys.argv[3]
    pack = sys.argv[4] if len(sys.argv) > 4 else os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "docs/archaeology/phase-0b/fixtures")
    os.makedirs(outdir, exist_ok=True)

    comdat = read(asmdir, "COMDAT.ASM")
    status = read(asmdir, "STATUS.ASM")
    pexam = read(asmdir, "PEXAM.ASM")
    swchar = read(asmdir, "SWCHAR.ASM")
    cd = read(asmdir, "CD.ASM")

    def must_find(text, pat, label):
        m = re.search(pat, text, re.M)
        if not m:
            raise KeyError(label)
        return m

    cxy = must_find(comdat, r"FDB\s+128\s*;VCNTRX", "VCNTRX")
    cy = must_find(comdat, r"FDB\s+76\s*;VCNTRY", "VCNTRY")
    stsv = must_find(comdat, r"STSVDB\s+FDB\s+D0\$BAS\+(\d+)\*32", "STSVDB")
    sts_end = must_find(comdat, r"FDB\s+D0\$BAS\+(\d+)\*32\s*;160th", "status end")
    cmd_end = must_find(comdat, r"FDB\s+D0\$BAS\+(\d+)\*32\s*;192nd", "command end")
    exam_cells = must_find(comdat, r"FDB\s+32\*(\d+)\s*;TXTEXA", "TXTEXA cells")
    sts_cells = must_find(comdat, r"FDB\s+32\*(\d+)\s*;TXTSTS", "TXTSTS cells")
    pri_cells = must_find(comdat, r"FDB\s+32\*(\d+)\s*;TXTPRI", "TXTPRI cells")
    g6 = must_find(cd, r"G6\.LEN\s+EQU\s+(\d+)", "G6.LEN")
    d0 = must_find(cd, r"D0\.LEN\s+EQU\s+32\*(\d+)\*8", "D0.LEN")

    viewport = int(stsv.group(1))
    status_end = int(sts_end.group(1))
    command_end = int(cmd_end.group(1))
    if viewport != 152 or status_end != 160 or command_end != 192:
        raise SystemExit("screen region constants changed")

    empt = must_find(status, r"M\$EMPT\s+FCB\s+(.+)", "M$EMPT")
    empty_bytes = []
    for tok in empt.group(1).split(","):
        tok = tok.strip().split(";")[0].strip()
        if tok.startswith("$"):
            empty_bytes.append(int(tok[1:], 16))
    empty_name = "".join(CHARS.get(b, "") for b in empty_bytes if b != 0xFF)

    exam = pexam_strings(pexam)
    if len(exam) < 3:
        raise SystemExit("PEXAM OUTSTI count")
    room, creature, backpack = [s.replace("\n", "") for _, s in exam[:3]]

    swc, swc_line = packed_fcbs_between(swchar, "SWCTAB", "SPCTAB")
    spc, spc_line = packed_fcbs_between(swchar, "SPCTAB", "THUDD")

    regions = {
        "description": "Logical 256-wide surface regions and text-block sizes",
        "source": "COMDAT.ASM STSVDB/PRIVDB/VCNTRX/VCNTRY/TXTEXA/TXTSTS/TXTPRI; "
                  "CD.ASM G6.LEN/D0.LEN",
        "extraction_method": "EQU and FDB values read verbatim from the listing",
        "centroid_x": 128,
        "centroid_y": 76,
        "centroid_source": "COMDAT.ASM:%d VCNTRX, COMDAT.ASM:%d VCNTRY" % (
            comdat[:cxy.start()].count("\n") + 1, comdat[:cy.start()].count("\n") + 1),
        "viewport_scanline_end": viewport,
        "status_scanline_end": status_end,
        "command_scanline_end": command_end,
        "stsvdb_source": "COMDAT.ASM:%d" % (comdat[:stsv.start()].count("\n") + 1),
        "g6_len": int(g6.group(1)),
        "viewport_char_rows": int(d0.group(1)),
        "examine_char_cells": 32 * int(exam_cells.group(1)),
        "status_char_cells": 32 * int(sts_cells.group(1)),
        "command_char_cells": 32 * int(pri_cells.group(1)),
        "cell_pixel_w": 8,
        "map_cell_scanlines": 6,
        "map_source": "MAPPER.ASM DSP32 LDB #32*6",
    }
    strings = {
        "description": "Examine headers and empty-hand name",
        "source": "PEXAM.ASM OUTSTI; STATUS.ASM M$EMPT",
        "extraction_method": "OUTSTI 5-bit decode (EXPAND EXPANX count+1 chars); "
                             "M$EMPT display codes until $FF",
        "empty_hand": empty_name,
        "empty_hand_bytes": ["0x%02X" % b for b in empty_bytes],
        "in_this_room": room,
        "creature": creature,
        "backpack": backpack,
        "pexam_source_lines": [n for n, _ in exam[:3]],
    }
    font = {
        "description": "Software character tables",
        "source": "SWCHAR.ASM SWCTAB (5-bit packed, 5 bytes/glyph) and SPCTAB "
                  "(7 bytes/glyph from code $20)",
        "extraction_method": "FCB binary literals between labels",
        "swctab_line": swc_line,
        "spctab_line": spc_line,
        "swctab": ["0x%02X" % b for b in swc],
        "spctab": ["0x%02X" % b for b in spc],
    }

    adj, gen = load_tokens(os.path.join(pack, "tokens.json"))
    vft = load_vft(os.path.join(pack, "vertical-features.json"))
    player = (0x10, 0x0B)

    files = {}
    files["screen-regions.json"] = write_json(
        os.path.join(outdir, "screen-regions.json"), regions)
    files["text-strings.json"] = write_json(
        os.path.join(outdir, "text-strings.json"), strings)
    files["font.json"] = write_json(os.path.join(outdir, "font.json"), font)

    # Status / command / examine reference dumps.
    files["status-empty.txt"] = project_status(empty_name, empty_name, "small").encode()
    files["status-hands.txt"] = project_status("WOODEN SWORD", "PINE TORCH", "large").encode()
    files["status-map.txt"] = project_status(empty_name, empty_name, "off").encode()
    files["command-prompt.txt"] = project_command("").encode()
    files["command-partial.txt"] = project_command("MOVE").encode()
    files["examine-empty.txt"] = project_examine(False, [], [], -1).encode()
    files["examine-creature.txt"] = project_examine(True, [], [], -1).encode()
    files["examine-objects.txt"] = project_examine(
        False, ["WOODEN SWORD", "PINE TORCH"], ["LEATHER SHIELD"], 0).encode()

    unrevealed = {"type": 17, "cls": 4, "reveal": 1}
    revealed = {"type": 17, "cls": 4, "reveal": 0}
    files["object-names.txt"] = (
        "EMPTY\n%s\n%s\n" % (object_name(unrevealed, adj, gen),
                             object_name(revealed, adj, gen))).encode()

    for level in range(5):
        maze_path = os.path.join(pack, "maze-level-%d.bin" % level)
        cells = open(maze_path, "rb").read()
        verticals = vft_marks(vft, level)
        creatures = load_creatures(os.path.join(pack, "population-entry.txt"), level)
        plain = project_map(cells, player, False, [], [], verticals)
        featured = project_map(cells, player, True, [], creatures, verticals)
        files["map-level-%d-plain.txt" % level] = plain.encode()
        files["map-level-%d-features.txt" % level] = featured.encode()

    for name, blob in files.items():
        if name.endswith(".json"):
            continue
        with open(os.path.join(outdir, name), "wb") as f:
            f.write(blob)

    manifest = {"fixtures": []}
    for name in sorted(files):
        blob = files[name]
        manifest["fixtures"].append({
            "file": name,
            "sha256": sha(blob),
            "bytes": len(blob),
        })
    write_json(os.path.join(outdir, "MANIFEST.json"), manifest)

    swc_lit = ", ".join(str(b) for b in swc)
    spc_lit = ", ".join(str(b) for b in spc)
    with open(header, "w") as f:
        f.write(f"""// GENERATED by tools/extract_text.py from the pinned listing.
// Do not edit by hand. Ultimate source: COMDAT.ASM, CD.ASM, PEXAM.ASM,
// STATUS.ASM, SWCHAR.ASM.
#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace dag {{

inline constexpr int kCentroidX = 128;
inline constexpr int kCentroidY = 76;
inline constexpr int kViewportScanlineEnd = {viewport};
inline constexpr int kStatusScanlineEnd = {status_end};
inline constexpr int kCommandScanlineEnd = {command_end};
inline constexpr int kExamineRows = {int(exam_cells.group(1))};
inline constexpr int kExamineCols = 32;
inline constexpr int kStatusCharRows = {int(sts_cells.group(1))};
inline constexpr int kCommandRows = {int(pri_cells.group(1))};
inline constexpr int kMapCellScanlines = 6;

inline constexpr std::string_view kEmptyHand = "{empty_name}";
inline constexpr std::string_view kExamRoom = "{room}";
inline constexpr std::string_view kExamCreature = "{creature}";
inline constexpr std::string_view kExamBackpack = "{backpack}";

inline constexpr std::array<std::uint8_t, {len(swc)}> kSwcTab{{{{
    {swc_lit}
}}}};
inline constexpr std::array<std::uint8_t, {len(spc)}> kSpcTab{{{{
    {spc_lit}
}}}};

}}  // namespace dag
""")
    print("wrote %s (%d fixtures)" % (outdir, len(files)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
