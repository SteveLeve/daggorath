#!/usr/bin/env python3
"""Extract VIEWER vector lists and scale tables from the pinned listing.

Usage: extract_vectors.py <asm-dir> <out-dir>

Bytes come from VARC.ASM, VERT.ASM, VOBJ.ASM, D3.ASM, D4.ASM, VIEWER.ASM,
VECTOR.ASM, SWCHAR.ASM FLATAB, DTABAS.ASM FWDOBJ/FWDCRE, and COMDAT.ASM
centroids. SVORG/SVECT/SVEND follow missing-macros.asm. Jump and subroutine
words are rewritten as offsets into the concatenated blob, because VCTLST
follows 6809 addresses that this core cannot use.
"""
import hashlib
import json
import os
import re
import sys

CTRL = {
    "V$NEW": 0xFF,
    "V$END": 0xFE,
    "V$JMP": 0xFD,
    "V$REL": 0xFC,
    "V$JSR": 0xFB,
    "V$RTS": 0xFA,
    "V$ABS": 0x00,
}

VECTOR_FILES = ["VARC.ASM", "VERT.ASM", "VOBJ.ASM", "D3.ASM", "D4.ASM"]


def read(asmdir, name):
    with open(os.path.join(asmdir, name), "r", errors="replace") as f:
        return f.read()


def code_part(line):
    return line.split(";", 1)[0].rstrip()


def parse_int(tok):
    tok = tok.strip()
    if tok in CTRL:
        return CTRL[tok]
    if re.fullmatch(r"%[01]+", tok):
        return int(tok[1:], 2) & 0xFF
    if re.fullmatch(r"\$[0-9A-Fa-f]+", tok):
        return int(tok[1:], 16)
    if re.fullmatch(r"-?\d+", tok):
        return int(tok) & 0xFF
    raise ValueError(tok)


def toward_zero_div2(n):
    return int(n / 2)


def assemble_vectors(asmdir):
    items = []
    svx = svy = 0
    for fname in VECTOR_FILES:
        text = read(asmdir, fname)
        for line in text.splitlines():
            code = code_part(line)
            if not code.strip():
                continue
            m = re.match(r"^([A-Z][A-Z0-9_$]*)\s+(.*)$", code)
            if m and not m.group(2).startswith("EQU"):
                items.append(("label", m.group(1)))
                rest = m.group(2).strip()
            else:
                eq = re.match(r"^([A-Z][A-Z0-9_$]*)\s+EQU\s+\*\s*$", code)
                if eq:
                    items.append(("label", eq.group(1)))
                    continue
                rest = code.strip()
            if not rest or rest.startswith(("NAM", "OPT", "INCLUDE", "LIBRY", "XDEF",
                                            "DSCT", "END", "IFNDEF", "IF ", "ENDC")):
                continue
            op, *tail = rest.split(None, 1)
            args = tail[0] if tail else ""
            if op == "FCB":
                for tok in args.split(","):
                    tok = tok.strip()
                    if tok:
                        items.append(("byte", parse_int(tok)))
            elif op == "FDB":
                lab = args.split(",", 1)[0].strip()
                items.append(("ref", lab))
            elif op == "SVORG":
                y, x = [int(t.strip(), 0) for t in args.split(",")[:2]]
                svx, svy = y, x
                items.append(("byte", y & 0xFF))
                items.append(("byte", x & 0xFF))
                items.append(("byte", CTRL["V$REL"]))
            elif op == "SVECT":
                y, x = [int(t.strip(), 0) for t in args.split(",")[:2]]
                packed = (((toward_zero_div2(y - svx) & 0x0F) << 4) |
                          (toward_zero_div2(x - svy) & 0x0F))
                items.append(("byte", packed & 0xFF))
                svx, svy = y, x
            elif op == "SVNEW":
                items.append(("byte", CTRL["V$ABS"]))
            elif op == "SVEND":
                items.append(("byte", CTRL["V$ABS"]))
                items.append(("byte", CTRL["V$END"]))
    symbols = {}
    blob = []
    patches = []
    for kind, val in items:
        if kind == "label":
            symbols[val] = len(blob)
        elif kind == "byte":
            blob.append(val & 0xFF)
        elif kind == "ref":
            patches.append((len(blob), val))
            blob.extend([0, 0])
    for off, lab in patches:
        if lab not in symbols:
            raise KeyError(lab)
        addr = symbols[lab]
        blob[off] = (addr >> 8) & 0xFF
        blob[off + 1] = addr & 0xFF
    return blob, symbols


def fcb_run(text, start_label, stop_pred, limit=None):
    lines = text.splitlines()
    start = None
    for i, line in enumerate(lines):
        if re.match(r"^%s\b" % re.escape(start_label), line):
            start = i
            break
    if start is None:
        raise KeyError(start_label)
    vals = []
    for line in lines[start + 1:]:
        if stop_pred(line):
            break
        code = code_part(line)
        m = re.match(r"^\s*(?:[A-Z0-9_$]+\s+)?FCB\s+(.+)$", code)
        if not m:
            continue
        for tok in m.group(1).split(","):
            vals.append(parse_int(tok))
            if limit is not None and len(vals) >= limit:
                return vals
    return vals


def parse_flatab(text):
    lines = text.splitlines()
    i = next(n for n, l in enumerate(lines) if re.match(r"^FLATAB\b", l))
    entries = []
    rel = None
    lists = []
    for line in lines[i + 1:]:
        code = code_part(line).strip()
        if not code:
            continue
        m = re.match(r"^FCB\s+(.+)$", code)
        if m:
            tok = m.group(1).split(",")[0].strip()
            if tok in ("-1", "$FF"):
                break
            if rel is not None:
                entries.append({"rel": rel, "lists": lists})
            rel = parse_int(tok)
            if rel >= 128:
                rel -= 256
            lists = []
            continue
        m = re.match(r"^FDB\s+(.+)$", code)
        if m:
            lists.append(m.group(1).split(",")[0].strip())
    if rel is not None:
        entries.append({"rel": rel, "lists": lists})
    return entries


def parse_macro_names(text, macro, group, field_index):
    """Names from GENXXX/CREXXX invocations of a decorator macro."""
    names = []
    in_group = False
    for line in text.splitlines():
        code = code_part(line)
        if re.match(r"^%s\s+MACR" % re.escape(group), code):
            in_group = True
            continue
        if in_group and re.match(r"^\s*ENDM\b", code):
            break
        if not in_group:
            continue
        m = re.match(r"^\s*\\1\s+(.+)$", code)
        if not m:
            continue
        parts = [p.strip() for p in m.group(1).split(",")]
        names.append(parts[field_index])
    return names


def main():
    if len(sys.argv) != 3:
        print(__doc__.strip())
        return 2
    asmdir, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    blob, symbols = assemble_vectors(asmdir)
    viewer = read(asmdir, "VIEWER.ASM")
    vector = read(asmdir, "VECTOR.ASM")
    swchar = read(asmdir, "SWCHAR.ASM")
    dtabas = read(asmdir, "DTABAS.ASM")
    comdat = read(asmdir, "COMDAT.ASM")
    cd = read(asmdir, "CD.ASM")

    norscl = fcb_run(viewer, "NORSCL", lambda l: re.match(r"^HLFSCL\b", l))
    hlfscl = fcb_run(viewer, "HLFSCL", lambda l: re.match(r"^;;;;;", l), limit=10)
    bakscl = fcb_run(viewer, "BAKSCL", lambda l: re.match(r"^;;;;;", l), limit=10)
    bitmsk = [0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01]
    bits = re.findall(r"FCB\s+(BIT[0-7])\s*$", vector, re.M)
    if bits != ["BIT7", "BIT6", "BIT5", "BIT4", "BIT3", "BIT2", "BIT1", "BIT0"]:
        raise SystemExit("VECTOR.ASM BITMSK order is not BIT7..BIT0")
    if not re.search(r"^BIT7\s+EQU\s+%10000000", cd, re.M):
        raise SystemExit("CD.ASM BIT7 is not %10000000")

    cx = 128
    cy = 76
    if "FDB     128             ;VCNTRX" not in comdat or "FDB     76              ;VCNTRY" not in comdat:
        raise SystemExit("COMDAT.ASM centroids not found")
    if "152*32" not in comdat or "160*32" not in comdat or "192*32" not in comdat:
        raise SystemExit("COMDAT.ASM scan-line constants not found")
    if "D0.LEN  EQU     32*19*8" not in cd:
        raise SystemExit("CD.ASM D0.LEN not found")

    fwdobj = parse_macro_names(dtabas, "FLVL", "GENXXX", 3)
    fwdcre = parse_macro_names(dtabas, "CVL", "CREXXX", 0)
    if fwdobj != ["FFLASK", "FRING", "FSCROL", "FSHIEL", "FSWORD", "FTORCH"]:
        raise SystemExit("unexpected FWDOBJ " + str(fwdobj))
    if fwdcre != ["SPIDER", "VIPER", "SGINT1", "BLOB", "KNIGT1", "SGINT2",
                  "SCORP", "KNIGT2", "WRAITH", "BALROG", "WIZ0", "WIZ1"]:
        raise SystemExit("unexpected FWDCRE " + str(fwdcre))

    doc = {
        "description": "Vector lists and VIEWER scale/light tables",
        "source": "VARC.ASM, VERT.ASM, VOBJ.ASM, D3.ASM, D4.ASM, VIEWER.ASM NORSCL/"
                  "HLFSCL/BAKSCL, VCTLST.ASM SETFAX, VECTOR.ASM BITMSK, "
                  "SWCHAR.ASM FLATAB, DTABAS.ASM FWDOBJ/FWDCRE, COMDAT.ASM VCNTRX/"
                  "VCNTRY/STSVDB/PRIVDB, CD.ASM D0.LEN, missing-macros.asm SVORG",
        "extraction_method":
            "FCB/FDB bytes and SVORG/SVECT/SVEND expansions read from the listing. "
            "V$JMP/V$JSR words are offsets into this blob, not 6809 addresses.",
        "centroid_x": cx,
        "centroid_y": cy,
        "viewport_end_line": 152,
        "status_end_line": 160,
        "command_end_line": 192,
        "norscl": norscl,
        "hlfscl": hlfscl,
        "bakscl": bakscl,
        "bitmsk": bitmsk,
        "flatab": parse_flatab(swchar),
        "fwdobj": fwdobj,
        "fwdcre": fwdcre,
        "fwdver": ["HOLEUP", "FLUP", "HOLEDN", "FLDN"],
        "celine": "CELINE",
        "lpeek": "LPEEK",
        "rpeek": "RPEEK",
        "symbols": symbols,
        "blob": blob,
    }
    path = os.path.join(outdir, "vectors.json")
    payload = json.dumps(doc, indent=2).encode() + b"\n"
    with open(path, "wb") as f:
        f.write(payload)
    print("wrote", path, "blob", len(blob), "symbols", len(symbols))
    return 0


if __name__ == "__main__":
    sys.exit(main())
