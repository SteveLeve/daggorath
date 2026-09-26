#!/usr/bin/env python3
"""Independent VIEWER / VCTLST reference. Does not import the C++ core.

Usage:
  viewer_ref.py --vectors VECTORS.json --maze-dir PHASE0B_FIXTURES --write OUTDIR
  viewer_ref.py --vectors VECTORS.json --state STATE.json
"""
import argparse
import hashlib
import json
import os
import sys

DR = (-1, 0, 1, 0)
DC = (0, 1, 0, -1)


def as_i8(v):
    v &= 0xFF
    return v - 256 if v >= 128 else v


def as_i16(v):
    v &= 0xFFFF
    return v - 65536 if v >= 32768 else v


def scale_coord(coord, scale, centroid):
    # VCTLST.ASM ASCAL: SUBB centroid, MUL by scale, ASR 7.
    delta = as_i8((coord - (centroid & 0xFF)) & 0xFF)
    if delta >= 0:
        product = as_i16((delta * scale) & 0xFFFF)
    else:
        product = as_i16((-(((-delta) * scale) & 0xFFFF)) & 0xFFFF)
    product = product >> 7
    return centroid + product


def decode_vectors(blob, x_scale, y_scale, cx, cy, fade, start=0):
    if ((fade + 1) & 0xFF) == 0:
        return []
    out = []
    have_start = False
    x0 = y0 = 0
    x_raw = y_raw = 0
    i = start
    returns = []
    steps = 0
    n = len(blob)
    while i < n and steps < 10000:
        steps += 1
        yb = blob[i]
        if yb >= 0xFA:
            if yb == 0xFE:
                break
            if yb == 0xFF:
                i += 1
                have_start = False
                continue
            if yb in (0xFB, 0xFD):
                if i + 2 >= n:
                    break
                addr = (blob[i + 1] << 8) | blob[i + 2]
                if addr >= n:
                    break
                if yb == 0xFB:
                    returns.append(i + 3)
                i = addr
                have_start = False
                continue
            if yb == 0xFA:
                if not returns:
                    break
                i = returns.pop()
                have_start = False
                continue
            if yb == 0xFC:
                i += 1
                while i < n and blob[i] != 0:
                    packed = blob[i]
                    i += 1
                    # int8_t(uint8_t(int8_t(packed) >> 4) << 1)
                    y_delta = as_i8((as_i8(packed) >> 4) & 0xFF) << 1
                    y_delta = as_i8(y_delta)
                    x_nibble = packed & 0x0F
                    if x_nibble & 0x08:
                        x_nibble |= 0xF0
                    x_delta = as_i8((x_nibble << 1) & 0xFF)
                    raw_y = (y_raw + y_delta) & 0xFF
                    raw_x = (x_raw + x_delta) & 0xFF
                    y = scale_coord(raw_y, y_scale, cy)
                    x = scale_coord(raw_x, x_scale, cx)
                    if have_start:
                        out.append((x0, y0, x, y))
                    x0, y0 = x, y
                    y_raw, x_raw = raw_y, raw_x
                    have_start = True
                if i < n and blob[i] == 0:
                    i += 1
                have_start = False
                continue
            break
        if i + 1 >= n:
            break
        y = scale_coord(blob[i], y_scale, cy)
        x = scale_coord(blob[i + 1], x_scale, cx)
        y_raw, x_raw = blob[i], blob[i + 1]
        i += 2
        if not have_start:
            x0, y0 = x, y
            have_start = True
            continue
        out.append((x0, y0, x, y))
        x0, y0 = x, y
    return out


def set_fade(light, range_, bitmsk):
    a = light - 7 - range_
    if a >= 0:
        return 0
    if a <= -7:
        return 0xFF
    return bitmsk[8 + a]


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


def vfind(tab, level, row, col):
    x = vft_pointer(tab, level)
    n = len(tab)

    def scan(bias):
        nonlocal x
        while x < n:
            code = tab[x]
            x += 1
            if code >= 0x80:
                code -= 256
            if code < 0:
                return -1
            r = tab[x]
            x += 1
            c = tab[x]
            x += 1
            if r == row and c == col:
                return code + bias
        return -1

    up = scan(0)
    if up >= 0:
        return up
    return scan(2)


def cfind(creatures, row, col):
    for c in creatures:
        if c["row"] == row and c["col"] == col and c.get("use", 255):
            return c
    return None


def ofind(objects, level, row, col):
    found = []
    for o in objects:
        if o["level"] != level:
            continue
        if o["row"] != row or o["col"] != col:
            continue
        if o["owner"] != 0:
            continue
        found.append(o)
    return found


def project(tables, maze, state):
    blob = tables["blob"]
    sym = tables["symbols"]
    cx, cy = tables["centroid_x"], tables["centroid_y"]
    mode = state.get("mode", 0)
    if mode == 2:
        return ("MAP features" if state.get("map_features") else "MAP plain"), []
    if mode == 1:
        return "EXAMINE", []
    rlight = state["regular_light"]
    mlight = state["magic_light"]
    header = "VIEW rlight=%d mlight=%d" % (rlight, mlight)
    segs = []
    row, col = state["row"], state["col"]
    pdir = state["dir"] & 3
    level = state["level"]
    vft = state.get("vft") or []
    creatures = state.get("creatures") or []
    objects = state.get("objects") or []

    def cell(r, c):
        if not (0 <= r < 32 and 0 <= c < 32):
            return 0xFF
        return maze[r * 32 + c]

    def edges(r, c):
        v = cell(r, c)
        north, east, south, west = v & 3, (v >> 2) & 3, (v >> 4) & 3, (v >> 6) & 3
        pairs = (north, east, south, west)
        return [pairs[(pdir + rel) & 3] for rel in range(4)]

    def draw_list(name, range_, fade, kind):
        if fade == 0xFF:
            return
        scale = tables["norscl"][range_]
        off = name if isinstance(name, int) else sym[name]
        for x0, y0, x1, y1 in decode_vectors(blob, scale, scale, cx, cy, fade, off):
            segs.append((kind, x0, y0, x1, y1, range_, fade))

    def drawit(name, range_, magic, kind):
        light = mlight if magic else rlight
        draw_list(name, range_, set_fade(light, range_, tables["bitmsk"]), kind)

    for range_ in range(10):
        rel = edges(row, col)
        for entry in tables["flatab"]:
            feat = rel[entry["rel"] & 3]
            names = entry["lists"]
            if feat == 2:
                drawit(names[2], range_, True, "architecture")
                feat = 3
            drawit(names[feat], range_, False, "architecture")
        cre = cfind(creatures, row, col)
        if cre:
            drawit(tables["fwdcre"][cre["type"]], range_, bool(cre.get("mgo")), "creature")
        for side, peek, rel_i in ((3, tables["lpeek"], 3), (1, tables["rpeek"], 1)):
            if rel[rel_i] != 0:
                continue
            d = (pdir + side) & 3
            nr, nc = row + DR[d], col + DC[d]
            seen = cfind(creatures, nr, nc)
            if seen:
                drawit(peek, range_, bool(seen.get("mgo")), "peek")
        vf = vfind(vft, level, row, col) if vft else -1
        if vf < 0:
            drawit(tables["celine"], range_, False, "vertical")
        else:
            drawit(tables["fwdver"][vf], range_, False, "vertical")
        for obj in ofind(objects, level, row, col):
            name = tables["fwdobj"][obj["cls"]]
            drawit(name, range_, True, "object")
            drawit(name, range_, False, "object")
        if rel[0] != 0:
            break
        row = (row + DR[pdir]) & 0xFF
        col = (col + DC[pdir]) & 0xFF
        if not (0 <= row < 32 and 0 <= col < 32):
            break
    return header, segs


def to_text(header, segs):
    lines = [header]
    for kind, x0, y0, x1, y1, range_, fade in segs:
        lines.append("%s %d %d %d %d %d %d" % (kind, x0, y0, x1, y1, range_, fade))
    return "\n".join(lines) + "\n"


def load_maze(path):
    with open(path, "rb") as f:
        data = f.read()
    if len(data) != 1024:
        raise SystemExit("maze must be 1024 bytes: " + path)
    return list(data)


def load_population(path, level, second):
    creatures, objects = [], []
    for line in open(path, encoding="utf-8"):
        if line.startswith("creature "):
            p = line.split()
            if int(p[1]) == level and int(p[2]) == second:
                creatures.append({
                    "slot": int(p[3]), "type": int(p[4]), "row": int(p[5]),
                    "col": int(p[6]), "mgo": int(p[8]), "use": int(p[14]),
                })
        elif line.startswith("object "):
            p = line.split()
            if int(p[1]) == level and int(p[2]) == second:
                objects.append({
                    "index": int(p[3]), "row": 0, "col": 0,
                    "level": int(p[5]), "owner": int(p[6]), "cls": int(p[7]),
                })
    return creatures, objects


def load_vft(path):
    doc = json.load(open(path))
    return [int(b, 16) for b in doc["raw_bytes"]]


def write_fixtures(tables, maze_dir, outdir):
    vft = load_vft(os.path.join(maze_dir, "vertical-features.json"))
    pop = os.path.join(maze_dir, "population-entry.txt")
    names = []
    for level in range(5):
        maze = load_maze(os.path.join(maze_dir, "maze-level-%d.bin" % level))
        creatures, objects = load_population(pop, level, 1)
        for tag, rl, ml in (("dark", 0, 0), ("regular", 7, 0), ("magic", 0, 13)):
            state = {
                "row": 0x10, "col": 0x0B, "dir": 0, "level": level,
                "regular_light": rl, "magic_light": ml, "mode": 0,
                "creatures": creatures, "objects": objects, "vft": vft,
            }
            text = to_text(*project(tables, maze, state))
            name = "draw-level-%d-start-%s.txt" % (level, tag)
            with open(os.path.join(outdir, name), "w", encoding="utf-8") as f:
                f.write(text)
            names.append(name)
    return names


def write_manifest(outdir):
    entries = []
    for name in sorted(os.listdir(outdir)):
        if name == "MANIFEST.json":
            continue
        path = os.path.join(outdir, name)
        if not os.path.isfile(path):
            continue
        blob = open(path, "rb").read()
        entries.append({
            "file": name,
            "sha256": hashlib.sha256(blob).hexdigest(),
            "bytes": len(blob),
        })
    with open(os.path.join(outdir, "MANIFEST.json"), "w", encoding="utf-8") as f:
        json.dump({"fixtures": entries}, f, indent=2)
        f.write("\n")
    return entries


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--vectors", required=True)
    ap.add_argument("--maze-dir")
    ap.add_argument("--write")
    ap.add_argument("--state")
    args = ap.parse_args()
    tables = json.load(open(args.vectors))
    if args.write:
        if not args.maze_dir:
            raise SystemExit("--write needs --maze-dir")
        os.makedirs(args.write, exist_ok=True)
        # Keep vectors.json if we are writing into its directory.
        names = write_fixtures(tables, args.maze_dir, args.write)
        entries = write_manifest(args.write)
        print("draw lists", len(names), "manifest", len(entries))
        return 0
    if args.state:
        state = json.load(open(args.state))
        maze = load_maze(state["maze"])
        header, segs = project(tables, maze, state)
        sys.stdout.write(to_text(header, segs))
        return 0
    raise SystemExit("need --write or --state")


if __name__ == "__main__":
    sys.exit(main())
