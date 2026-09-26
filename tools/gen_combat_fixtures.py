#!/usr/bin/env python3
"""Independent SCAL16 / DAMAGE / ATTACK fixtures from PATTK.ASM.

The arithmetic is a transliteration of SCAL16, ATTACK and DAMAGE. The C++ core
is checked against the file this writes; the two do not share code.
"""
import json
import os
import sys

CREATURES = [
    (32, 0, 255, 128, 255),
    (56, 0, 255, 80, 128),
    (200, 0, 255, 52, 192),
    (304, 0, 255, 96, 167),
    (504, 0, 128, 96, 60),
    (704, 0, 128, 128, 48),
    (400, 255, 128, 255, 128),
    (800, 0, 64, 255, 8),
    (800, 192, 16, 192, 8),
    (1000, 255, 5, 255, 3),
    (1000, 255, 6, 255, 0),
    (8000, 255, 6, 255, 0),
]

WEAPONS = [
    (0, 5),    # EMPHND
    (0, 16),   # WOODEN
    (0, 40),   # IRON
    (64, 64),  # ELVISH
    (255, 255),  # FIRE ring offense
]

SHIELDS = [
    (0x80, 0x80),
    (108, 128),
    (96, 128),
    (64, 64),
]


def scal16(value, radix):
    return ((value * radix) >> 7) & 0xFFFF


def adjustment(atk_power, def_power, def_damage):
    scaled = ((def_power - def_damage) & 0xFFFF) << 2
    scaled &= 0xFFFF
    index = 15
    while index != 0:
        if scaled < atk_power:
            break
        scaled = (scaled - atk_power) & 0xFFFF
        index -= 1
    delta = index - 3
    if delta >= 0:
        return delta * 10
    return -((-delta) * 25)


def hits(atk_power, def_power, def_damage, roll):
    total = adjustment(atk_power, def_power, def_damage) + roll
    total = (total + 0x10000) & 0xFFFF
    if total >= 0x8000:
        total -= 0x10000
    judged = (total - 127 + 0x10000) & 0xFFFF
    if judged >= 0x8000:
        judged -= 0x10000
    return judged >= 0


def damage(power, mgo, pho, mgd, phd, already=0):
    out = already
    out = (out + scal16(scal16(power, mgo), mgd)) & 0xFFFF
    out = (out + scal16(scal16(power, pho), phd)) & 0xFFFF
    return out


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else "docs/archaeology/phase-3/fixtures"
    os.makedirs(outdir, exist_ok=True)
    cases = []
    samples = [0, 1, 5, 127, 128, 160, 255, 1000, 8000, 32768, 65535]
    for value in samples:
        for radix in (0, 1, 5, 16, 64, 127, 128, 255):
            cases.append(f"S {value} {radix} {scal16(value, radix)}")
    rows = []
    for ci, creature in enumerate(CREATURES):
        power, mgo, mgd, pho, phd = creature
        for wi, weapon in enumerate(WEAPONS):
            for si, shield in enumerate(SHIELDS):
                dealt = damage(160, weapon[0], weapon[1], mgd, phd)
                taken = damage(power, mgo, pho, shield[0], shield[1])
                rows.append({
                    "creature": ci,
                    "weapon": wi,
                    "shield": si,
                    "to_creature": dealt,
                    "to_player": taken,
                })
                cases.append(f"D {ci} {wi} {si} {dealt} {taken}")
    swings = []
    for roll in range(256):
        hit = hits(160, 32, 0, roll)
        swings.append(1 if hit else 0)
        cases.append(f"A 160 32 0 {roll} {1 if hit else 0}")
    doc = {
        "source": "PATTK.ASM SCAL16, ATTACK, DAMAGE",
        "method": "independent Python transliteration",
        "cases": cases,
        "damage_rows": rows,
        "attack_160_vs_32": swings,
    }
    path = os.path.join(outdir, "scal16-damage.json")
    with open(path, "w") as f:
        json.dump(doc, f, indent=2)
        f.write("\n")
    print(f"wrote {path} ({len(cases)} cases)")


if __name__ == "__main__":
    main()
