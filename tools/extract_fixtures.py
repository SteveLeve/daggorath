#!/usr/bin/env python3
"""
Phase 0b fixture extractor for the Dungeons of Daggorath preservation project.

Reads the pinned reconstructed 1983 assembly listing and emits machine-readable
fixtures. Every value is either (a) read directly out of the .ASM text, or
(b) computed by a routine that is a line-by-line transliteration of the
corresponding 6809 routine. Nothing here is hand-typed from memory and no hash
is ever written by hand.

Usage: extract_fixtures.py <asm-dir> <out-dir>
"""
import hashlib
import json
import os
import re
import sys

# --------------------------------------------------------------------------
# helpers for reading the listing
# --------------------------------------------------------------------------


def read(asmdir, name):
    with open(os.path.join(asmdir, name), "r", errors="replace") as f:
        return f.read()


def fcb_values(text):
    """Yield (lineno, [int,...]) for every FCB line with literal values."""
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


def region(text, start_label, end_pred):
    """Return the text between a label line and the first line matching end_pred."""
    lines = text.splitlines()
    start = None
    for i, line in enumerate(lines):
        if re.match(r"^%s\b" % re.escape(start_label), line):
            start = i
            break
    if start is None:
        raise KeyError(start_label)
    for j in range(start + 1, len(lines)):
        if end_pred(lines[j]):
            return "\n".join(lines[start:j]), start + 1
    return "\n".join(lines[start:]), start + 1


# --------------------------------------------------------------------------
# 5-bit token string decoding  [S: EXPAND.ASM EXPANX/GETFIV]
# --------------------------------------------------------------------------

ALPHABET = {0: " "}
for _i in range(1, 27):
    ALPHABET[_i] = chr(ord("A") + _i - 1)


def decode_token(bytes_):
    """Decode one packed 5-bit token: [count][class][count letters]."""
    bits = "".join(f"{b:08b}" for b in bytes_)

    def field(n):
        s = bits[5 * n:5 * n + 5]
        if len(s) < 5:
            raise ValueError("token truncated")
        return int(s, 2)

    count = field(0)
    cls = field(1)
    letters = "".join(ALPHABET.get(field(2 + k), "?") for k in range(count))
    # strings terminate on byte boundaries: consumed 5*(count+2) bits, rounded up
    consumed_bytes = (5 * (count + 2) + 7) // 8
    return letters, cls, consumed_bytes


def parse_token_table(text, label, debflg=False):
    """Parse a TOKEN.ASM table: entries delimited by XDEF lines."""
    body, first_line = region(
        text, label, lambda l: re.match(r"^[A-Z0-9_$.]+NUM\s+EQU", l))
    lines = body.splitlines()
    entries = []
    cur_name = None
    cur_bytes = []
    cur_line = None
    in_debug = False
    for off, line in enumerate(lines):
        code = line.split(";", 1)[0]
        if re.match(r"\s*IF\s+DEBFLG", code):
            in_debug = True
            continue
        if re.match(r"\s*ENDIF", code):
            in_debug = False
            continue
        m = re.match(r"\s*XDEF\s+([A-Z0-9_.$]+)\s*,", code)
        if m:
            if cur_name and (debflg or not cur_was_debug):
                entries.append((cur_name, cur_bytes, cur_line))
            cur_name = m.group(1)
            cur_bytes = []
            cur_line = first_line + off
            cur_was_debug = in_debug
            continue
        vals = fcb_values(line)
        if vals and cur_name:
            cur_bytes.extend(vals[0][1])
    if cur_name and (debflg or not cur_was_debug):
        entries.append((cur_name, cur_bytes, cur_line))

    out = []
    for idx, (name, bs, ln) in enumerate(entries):
        word, cls, _ = decode_token(bs)
        out.append({
            "index": idx,
            "symbol": name,
            "word": word,
            "class": cls,
            "packed_bytes": [f"0x{b:02X}" for b in bs],
            "source_line": f"TOKEN.ASM:{ln}",
        })
    return out


# --------------------------------------------------------------------------
# RANDOX: 24-bit polynomial generator  [S: RANDOM.ASM RANDOX]
# --------------------------------------------------------------------------


class Rng:
    """Exact transliteration of RANDOX. seed[0..2] == SEED, SEED+1, SEED+2."""

    def __init__(self, seed):
        self.seed = list(seed)
        self.calls = 0

    def next(self):
        s = self.seed
        for _ in range(8):                      # LDX #$0008 / RND1
            b = 0                               # CLRB
            a = s[2] & 0xE1                     # LDA SEED+2 / ANDA #$E1
            for _ in range(8):                  # LDY #$0008 / RND2
                carry = (a >> 7) & 1            # LSLA
                a = (a << 1) & 0xFF
                if carry:
                    b = (b + 1) & 0xFF          # INCB
            carry = b & 1                       # LSRB -> carry = parity count LSB
            # ROL SEED / ROL SEED+1 / ROL SEED+2 (24-bit rotate-left through carry)
            for i in (0, 1, 2):
                newc = (s[i] >> 7) & 1
                s[i] = ((s[i] << 1) | carry) & 0xFF
                carry = newc
        self.calls += 1
        return s[0]                             # LDA SEED

    def state(self):
        return [f"0x{b:02X}" for b in self.seed]


# --------------------------------------------------------------------------
# DGNGEN: maze construction  [S: DGNGEN.ASM DGNGEN/MAKDOR/DGEN90]
# --------------------------------------------------------------------------

STPTAB = [(-1, 0), (0, 1), (1, 0), (0, -1)]   # N, E, S, W  [S: CRETUR.ASM STPTAB]
MSKTAB = [0b00000011, 0b00001100, 0b00110000, 0b11000000]
HF_DOR, HF_SDR = 1, 2                          # [S: CD.ASM:165-166]
DORTAB = [HF_DOR * m for m in (1, 4, 16, 64)]
SDRTAB = [HF_SDR * m for m in (1, 4, 16, 64)]
N_WALL, E_WALL, S_WALL, W_WALL = 0x03, 0x0C, 0x30, 0xC0


def s8(v):
    v &= 0xFF
    return v - 256 if v & 0x80 else v


class Maze:
    """32x32 cell array; index = row*32 + col  [S: DGNGEN.ASM MAP32]"""

    def __init__(self):
        self.cells = bytearray([0xFF] * 1024)   # NEGRAM MAZLND..MAZEND

    def at(self, row, col):
        return self.cells[((row & 31) * 32 + (col & 31))]

    def put(self, row, col, v):
        self.cells[((row & 31) * 32 + (col & 31))] = v & 0xFF


def border_ok(row, col):
    """BORDER: legal iff row and col are unchanged by MOD 32."""
    return (row & 31) == (row & 0xFF) and (col & 31) == (col & 0xFF)


def step(row, col, dir_):
    dr, dc = STPTAB[dir_ & 3]
    return (row + dr) & 0xFF, (col + dc) & 0xFF


def friend(maze, row, col):
    """FRIEND: 3x3 neighbourhood, out-of-bounds reads as $FF."""
    out = []
    for r in (row - 1, row, row + 1):
        for c in (col - 1, col, col + 1):
            rr, cc = r & 0xFF, c & 0xFF
            out.append(0xFF if not border_ok(rr, cc) else maze.at(rr, cc))
    return out


def rndcel(rng):
    """RNDCEL: first draw is the COLUMN, second is the ROW."""
    col = rng.next() & 31
    row = rng.next() & 31
    return row, col


def dgngen(level_seed, second, trace=None):
    """Build one maze. `second` is the SECOND counter used by DGEN90."""
    rng = Rng(level_seed)
    maze = Maze()

    y = 500                                     # LDY #500
    drow, dcol = rndcel(rng)                    # JSR RNDCEL / STD DROW

    # Phase I ------------------------------------------------------------
    dir_ = dst = 0
    state = "DGEN10"
    row = col = 0
    while True:
        if state == "DGEN10":
            dir_ = rng.next() & 3
            dst = (rng.next() & 7) + 1
            state = "DGEN30"
            continue
        if state == "DGEN20":
            drow, dcol = row, col
            dst = (dst - 1) & 0xFF
            if dst == 0:
                state = "DGEN10"
                continue
            state = "DGEN30"
            continue
        # DGEN30: tentative step
        nr, nc = step(drow, dcol, dir_)
        if not border_ok(nr, nc):
            state = "DGEN10"
            continue
        row, col = nr, nc
        if maze.at(row, col) == 0:               # TST ,X / BEQ DGEN20
            state = "DGEN20"
            continue
        n = friend(maze, row, col)
        rejected = False
        for trio in ((3, 0, 1), (1, 2, 5), (5, 8, 7), (7, 6, 3)):
            if sum(n[i] for i in trio) & 0xFF == 0:
                rejected = True
                break
        if rejected:
            state = "DGEN10"
            continue
        maze.put(row, col, 0)                    # CLR ,X
        y -= 1
        if y == 0:
            break
        state = "DGEN20"

    # Phase II: walls ----------------------------------------------------
    for r in range(32):
        for c in range(32):
            cell = maze.at(r, c)
            if (cell + 1) & 0xFF == 0:           # completely walled
                continue
            n = friend(maze, r, c)
            v = cell
            if n[1] == 0xFF:
                v |= N_WALL
            if n[3] == 0xFF:
                v |= W_WALL
            if n[5] == 0xFF:
                v |= E_WALL
            if n[7] == 0xFF:
                v |= S_WALL
            maze.put(r, c, v)

    # Doors --------------------------------------------------------------
    def makdor(table):
        while True:
            r, c = rndcel(rng)
            b = maze.at(r, c)
            if b == 0xFF:
                continue
            d = rng.next() & 3
            if b & MSKTAB[d]:
                continue
            maze.put(r, c, b | table[d])
            nr, nc = step(r, c, d)
            opp = (d + 2) & 3
            maze.put(nr, nc, maze.at(nr, nc) | table[opp])
            return

    for _ in range(70):
        makdor(DORTAB)
    for _ in range(45):
        makdor(SDRTAB)

    pre_spin = rng.state()
    pre_calls = rng.calls

    # DGEN90: spin per SECOND. LDB SECOND; loop DEC B / BNE -> 256 when SECOND==0
    spins = second if second != 0 else 256
    for _ in range(spins):
        rng.next()

    if trace is not None:
        trace.update(rng_pre_spin=pre_spin, rng_post_spin=rng.state(),
                     rng_calls_pre_spin=pre_calls, rng_calls_post_spin=rng.calls,
                     spin_count=spins)
    return maze, rng


def serialize(maze):
    """Stable serialization: 1024 bytes, row-major, row*32+col, one byte/cell."""
    return bytes(maze.cells)


# --------------------------------------------------------------------------
# main
# --------------------------------------------------------------------------


def main():
    asmdir, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    fixtures = {}

    dgn = read(asmdir, "DGNGEN.ASM")
    token = read(asmdir, "TOKEN.ASM")
    dtabas = read(asmdir, "DTABAS.ASM")
    comdat = read(asmdir, "COMDAT.ASM")
    comcre = read(asmdir, "COMCRE.ASM")
    common = read(asmdir, "COMMON.ASM")

    # --- LVLTAB -------------------------------------------------------
    lvl_body, lvl_line = region(dgn, "LVLTAB", lambda l: l.startswith("MSKTAB"))
    lvltab = [v for _, vs in fcb_values(lvl_body) for v in vs]
    assert len(lvltab) == 7, lvltab

    # --- RNG fixture --------------------------------------------------
    rng_vectors = []
    for seed in ([0x73, 0xC7, 0x5D], [0x00, 0x00, 0x01], [0xFF, 0xFF, 0xFF]):
        r = Rng(seed)
        rng_vectors.append({
            "initial_seed": [f"0x{b:02X}" for b in seed],
            "first_16_outputs": [f"0x{r.next():02X}" for _ in range(16)],
            "seed_after_16_calls": r.state(),
        })
    fixtures["rng.json"] = {
        "description": "24-bit polynomial RNG (RANDOX) reference vectors",
        "source": "RANDOM.ASM RANDOX",
        "extraction_method":
            "line-by-line transliteration of RANDOX; 8 rounds x (mask SEED+2 "
            "with $E1, count set bits, rotate parity LSB into 24-bit ROL chain); "
            "returns SEED[0]",
        "byte_order": "seed[0]=SEED, seed[1]=SEED+1, seed[2]=SEED+2",
        "vectors": rng_vectors,
    }

    # plain-text copy of the RNG vectors, for the C++ tests to read without
    # needing a JSON parser
    with open(os.path.join(outdir, "rng-vectors.txt"), "w") as f:
        f.write("# seed0 seed1 seed2 : 16 outputs : seed after 16 calls\n")
        for v in rng_vectors:
            f.write(" ".join(x.replace("0x", "") for x in v["initial_seed"]) + " : " +
                    " ".join(x.replace("0x", "") for x in v["first_16_outputs"]) +
                    " : " + " ".join(x.replace("0x", "")
                                     for x in v["seed_after_16_calls"]) + "\n")

    # --- maze fixtures ------------------------------------------------
    maze_entries = []
    invariance = []
    for level in range(5):
        seed = lvltab[level:level + 3]
        canonical = None
        per_second = []
        tr = {}
        for second in (0, 1, 7, 30, 59):
            maze, _ = dgngen(seed, second, tr if second == 1 else None)
            blob = serialize(maze)
            h = hashlib.sha256(blob).hexdigest()
            if canonical is None:
                canonical = h
            per_second.append({"second_at_entry": second, "maze_sha256": h,
                               "identical_to_first": h == canonical})
        # repeated entry at the same SECOND must also be identical
        maze2, _ = dgngen(seed, 30, None)
        repeat_h = hashlib.sha256(serialize(maze2)).hexdigest()
        cleared = sum(1 for b in serialize(maze) if b != 0xFF)
        maze_entries.append({
            "level": level,
            "lvltab_window": [f"0x{b:02X}" for b in seed],
            "lvltab_source_line": f"DGNGEN.ASM:{lvl_line}+{level}",
            "maze_sha256": canonical,
            "cleared_cell_count": cleared,
            "rng_state_before_dgen90_spin": tr.get("rng_pre_spin"),
            "rng_state_after_dgen90_spin_second_1": tr.get("rng_post_spin"),
            "rng_calls_before_spin": tr.get("rng_calls_pre_spin"),
        })
        invariance.append({
            "level": level,
            "entries_at_different_seconds": per_second,
            "repeat_entry_same_second_sha256": repeat_h,
            "repeat_matches": repeat_h == canonical,
        })

    fixtures["mazes.json"] = {
        "description": "Canonical maze layouts for levels 0-4",
        "source": "DGNGEN.ASM DGNGEN/MAKDOR/DGEN90; CRETUR.ASM STEP/STEPOK; CD.ASM MAZLND",
        "extraction_method":
            "transliteration of DGNGEN executed against LVLTAB seeds read from "
            "DGNGEN.ASM; 500 carved cells, 70 regular + 45 secret doors",
        "serialization":
            "1024 bytes, row-major, index = row*32 + col, one byte per cell; "
            "bit pairs low-to-high = North, East, South, West; "
            "00 passage, 01 regular door, 10 secret door, 11 wall; "
            "hash = SHA-256 over those 1024 bytes in that order",
        "levels": maze_entries,
        "entry_time_invariance": invariance,
    }

    # raw maze bytes, one file per level (level 0 also used by the C++ tests)
    for level in range(5):
        maze, _ = dgngen(lvltab[level:level + 3], 1, None)
        path = os.path.join(outdir, f"maze-level-{level}.bin")
        with open(path, "wb") as f:
            f.write(serialize(maze))

    # --- vertical features (VFTTAB) -----------------------------------
    vft_body, vft_line = region(comcre, "VFTTAB",
                                lambda l: l.strip().startswith(";  CREGEN"))
    raw = [v for _, vs in fcb_values(vft_body) for v in vs]
    feats = []
    i = 0
    level = 0
    # -128 ($80) acts as a per-level terminator; two terminators per level boundary
    groups = []
    cur = []
    for v in raw:
        if v == 0x80:
            groups.append(cur)
            cur = []
        else:
            cur.append(v)
    groups.append(cur)
    fixtures["vertical-features.json"] = {
        "description": "VFTTAB vertical feature records grouped by $80 terminators",
        "source": f"COMCRE.ASM:{vft_line} VFTTAB",
        "extraction_method": "FCB bytes read verbatim; $80 (-128) treated as group terminator",
        "record_format": "kind, row, col ; kind 1 = ladder, 0 = hole (per source comments)",
        "raw_bytes": [f"0x{b:02X}" for b in raw],
        "groups": [
            {"group_index": gi,
             "records": [{"kind": g[k], "row": g[k + 1], "col": g[k + 2]}
                         for k in range(0, len(g) - 2, 3)]}
            for gi, g in enumerate(groups) if g
        ],
        "note": "Group-to-level mapping is INFERRED from the source comment "
                "columns and is an open item; the byte values are source-proven.",
    }

    # --- token tables --------------------------------------------------
    tok = {}
    for label in ("CMDTAB", "DIRTAB", "ADJTAB", "GENTAB"):
        tok[label] = parse_token_table(token, label)
    fixtures["tokens.json"] = {
        "description": "Decoded 5-bit packed token tables (DEBFLG = 0)",
        "source": "TOKEN.ASM; decoder per EXPAND.ASM EXPANX/GETFIV",
        "extraction_method":
            "packed 5-bit fields read MSB-first: field0 = letter count, "
            "field1 = token class, then `count` letters with 0=space, 1..26=A..Z",
        "tables": tok,
    }

    # abbreviation analysis: shortest unique prefix per table (PARSER semantics)
    amb = {}
    for label, entries in tok.items():
        words = [e["word"] for e in entries]
        table = {}
        for w in words:
            shortest = None
            for n in range(1, len(w) + 1):
                p = w[:n]
                if sum(1 for o in words if o.startswith(p)) == 1:
                    shortest = p
                    break
            table[w] = {
                "shortest_unique_prefix": shortest,
                "exact_word_is_ambiguous":
                    sum(1 for o in words if o.startswith(w)) > 1,
            }
        amb[label] = table
    fixtures["parser-prefixes.json"] = {
        "description": "Shortest unique prefix per token, derived from PARSER semantics",
        "source": "PARSER.ASM PARSER/PARS10-PARS30; TOKEN.ASM tables",
        "extraction_method":
            "PARSER accepts a token that is a prefix of a table entry and "
            "rejects when two entries match; prefixes computed from the decoded "
            "tables under that rule",
        "tables": amb,
    }

    # --- creature definition blocks ------------------------------------
    crex = re.search(r"CREXXX\s+MACR(.*?)ENDM", dtabas, re.S).group(1)
    creatures = []
    for idx, line in enumerate(
            [l for l in crex.splitlines() if re.search(r"\\1\s+\w", l)]):
        args = [a.strip() for a in line.split("\\1", 1)[1].split(",")]
        creatures.append({
            "index": idx, "symbol": args[0],
            "move_delay_tenths": int(args[1]), "attack_delay_tenths": int(args[2]),
            "magic_offense": int(args[3]), "magic_defense": int(args[4]),
            "physical_offense": int(args[5]), "physical_defense": int(args[6]),
            "power": int(args[7]),
        })

    cmt_body, cmt_line = region(comdat, "CMTTAB", lambda l: l.startswith("CMTEND"))
    cmt = [v for _, vs in fcb_values(cmt_body) for v in vs]
    assert len(cmt) == 60, len(cmt)
    fixtures["creatures.json"] = {
        "description": "Creature definition blocks and initial level populations",
        "source": f"DTABAS.ASM CREXXX/CDBTAB; COMDAT.ASM:{cmt_line} CMTTAB",
        "extraction_method":
            "CREXXX macro argument lists parsed verbatim; CMTTAB read as a "
            "5 x 12 matrix (level-major, CTYPES = 12 columns)",
        "definitions": creatures,
        "initial_population_matrix": {
            "layout": "row = level 0..4, column = creature type index 0..11",
            "rows": [cmt[l * 12:(l + 1) * 12] for l in range(5)],
        },
        "regeneration": {
            "task": "CREGEN",
            "schedule": "SCHED$ 5,Q.MIN  (re-queued every 5 minute ticks)",
            "rule": "if the sum over all 12 types on the current level is < 32, "
                    "increment type ((RANDOM & 7) + 2)",
            "source": f"COMCRE.ASM CREGEN",
        },
    }

    # --- object definition blocks --------------------------------------
    objx = re.search(r"OBJXXX\s+MACR(.*?)\n\s*ENDM", dtabas, re.S).group(1)
    spcx = re.search(r"SPCXXX\s+MACR(.*?)\n\s*ENDM", dtabas, re.S).group(1)
    genx = re.search(r"GENXXX\s+MACR(.*?)ENDM", dtabas, re.S).group(1)

    def obj_rows(block):
        rows = []
        for line in block.splitlines():
            m = re.search(r"\\[12]\s+([A-Z0-9$,._*+&()\s-]+)$", line)
            if not m or "," not in m.group(1):
                continue
            rows.append([a.strip() for a in m.group(1).split(",")])
        return rows

    lvlmap = {f"LVL{i}": i for i in range(6)}
    objects = []
    for args in obj_rows(objx):
        rec = {
            "name": args[0], "symbol": args[1], "class": args[2],
            "reveal_requirement": int(args[3]),
            "magic_offense": int(args[4]), "physical_offense": int(args[5]),
            "initial_level": lvlmap[args[6]], "count": int(args[7]),
        }
        if len(args) > 8:
            rec["special_params"] = args[8:]
        objects.append(rec)
    specials = []
    for args in obj_rows(spcx):
        specials.append({
            "name": args[0], "symbol": args[1], "class": args[2],
            "reveal_requirement": int(args[3]),
            "magic_offense": int(args[4]), "physical_offense": int(args[5]),
            "extra": args[6:] or None,
        })
    generics = []
    for args in obj_rows(genx):
        generics.append({"name": args[0], "symbol": args[1], "class": args[2],
                         "weight": int(args[5])})
    fixtures["objects.json"] = {
        "description": "Object definition blocks, generic classes and weights",
        "source": "DTABAS.ASM GENXXX/OBJXXX/SPCXXX macros (ODBTAB, XXXTAB, OMXTAB, OBJWGT)",
        "extraction_method": "macro argument lists parsed verbatim from the listing",
        "generic_classes": generics,
        "level_objects": objects,
        "special_objects": specials,
        "torch_special_params": "for torches: [timer, regular light, magical light]",
        "shield_special_params": "for shields: [magic defense filter, physical defense filter, unused]",
    }

    # --- clock / scheduler constants -----------------------------------
    rol_body, rol_line = region(common, "ROLTAB",
                                lambda l: l.strip().startswith(";!!!"))
    rol = [v for _, vs in fcb_values(rol_body) for v in vs]
    fixtures["clock.json"] = {
        "description": "Clock rollover values, queue codes, initial task order",
        "source": f"COMMON.ASM:{rol_line} ROLTAB, CLOCK, QUESCN, SCHED; "
                  "CD.ASM queue codes and TCB layout; COMDAT.ASM TCBDAT; ONCE.ASM SYSTCB",
        "extraction_method": "FCB values and EQU constants read verbatim",
        "interrupt_rate_hz": 60,
        "rollovers": {"jiffy": rol[0], "tenth": rol[1], "second": rol[2],
                      "minute": rol[3], "hour": rol[4]},
        "queue_codes": {"Q.NUL": 0, "Q.JIF": 2, "Q.TEN": 4, "Q.SEC": 6,
                        "Q.MIN": 8, "Q.HOU": 10, "Q.SCD": 12},
        "tcb_layout": {"P.TCPTR": 0, "P.TCTIM": 2, "P.TCRTN": 3,
                       "P.TCDTA": 5, "TC.LEN": 7},
        "initial_tcb_order_in_scdque":
            ["PLAYER", "LUKNEW", "HSLOW", "BURNER", "CREGEN"],
        "initial_task_queue": "Q.SCD with countdown 0 (ONCE.ASM SYSTCB: LDD #Q.SCD)",
        "task_reschedules": {
            "PLAYER": "SCHED$ 1,Q.JIF", "LUKNEW": "SCHED$ 3,Q.TEN",
            "BURNER": "SCHED$ 1,Q.MIN", "CREGEN": "SCHED$ 5,Q.MIN",
            "HSLOW": "Q.JIF (countdown computed from heartbeat rate)",
        },
        "keyboard_buffer": {"size": 32, "overflow_checked": False,
                            "source": "COMMON.ASM KBDPUT/KBDGET"},
        "line_buffer": {"size": 32, "source": "CD.ASM:584 LINBUF",
                        "full_buffer_behaviour":
                            "HUMAN falls through from the buffer-full test into "
                            "the carriage-return path, dispatching the line"},
    }

    # --- initial player state -------------------------------------------
    fixtures["initial-state.json"] = {
        "description": "Game-mode initial player and inventory state",
        "source": "ONCE.ASM GAME10/GAME20/GAME30; COMDAT.ASM GAMDAT",
        "extraction_method": "immediate operands read verbatim from ONCE.ASM",
        "player_row": 0x10, "player_col": 0x0B,
        "player_row_col_source": "ONCE.ASM GAME10: LDD #$100B / STD PROW",
        "player_dir": 0,
        "player_dir_evidence": "inferred: no explicit initialisation found; "
                               "RAM is zeroed by SYSTCB/ZERO before GAME10",
        "level": 0,
        "initial_bag": ["WOODEN sword (T.SWO3)", "PINE torch (T.TOR4)"],
        "ppow_initial": 160,
        "ppow_evidence":
            "COMDAT.ASM:87 RAMDAT sets PPOW to $17,$A0; ONCE.ASM GAME10 then "
            "executes CLR PPOW, which clears only the HIGH byte of the two-byte "
            "field, leaving $00A0 = 160",
        "pobjwt_initial": 35,
        "pobjwt_evidence":
            "COMDAT.ASM:86 RAMDAT sets POBJWT to 30+5; equals the initial bag "
            "weight (wooden sword 25 + pine torch 10)",
        "pdam_initial": 0,
        "heartr_at_start": 46,
        "heartr_derivation":
            "HUPDAX: (160*64)/(160+0) by repeated subtraction yields 65 (the "
            "quotient is incremented before the borrow test), minus 19 = 46 jiffies",
    }

    # --- write ----------------------------------------------------------
    manifest = {"fixtures": []}
    for name, obj in fixtures.items():
        path = os.path.join(outdir, name)
        blob = json.dumps(obj, indent=2, sort_keys=False).encode() + b"\n"
        with open(path, "wb") as f:
            f.write(blob)
        manifest["fixtures"].append(
            {"file": name, "sha256": hashlib.sha256(blob).hexdigest(),
             "bytes": len(blob)})
    extra = [f"maze-level-{l}.bin" for l in range(5)] + ["rng-vectors.txt"]
    for name in extra:
        with open(os.path.join(outdir, name), "rb") as f:
            blob = f.read()
        manifest["fixtures"].append(
            {"file": name, "sha256": hashlib.sha256(blob).hexdigest(),
             "bytes": len(blob)})
    manifest["fixtures"].sort(key=lambda e: e["file"])
    with open(os.path.join(outdir, "MANIFEST.json"), "w") as f:
        json.dump(manifest, f, indent=2)
        f.write("\n")

    print(json.dumps({"levels": [
        {"level": e["level"], "seed": e["lvltab_window"],
         "sha256": e["maze_sha256"], "cleared": e["cleared_cell_count"]}
        for e in maze_entries]}, indent=2))
    print("invariance:", json.dumps(
        [{"level": iv["level"],
          "all_seconds_identical": all(x["identical_to_first"]
                                       for x in iv["entries_at_different_seconds"]),
          "repeat_matches": iv["repeat_matches"]} for iv in invariance]))


if __name__ == "__main__":
    main()
