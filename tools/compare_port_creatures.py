#!/usr/bin/env python3
"""Compare creature stats with a local Hunerlach-lineage port, if one is present.

The port is not a specification. Power and offense bytes must match the
listing tables in population.hpp. Delay bytes are tenths of a second there
and milliseconds in the port, before creatureSpeed is applied. A mismatch
in those bytes is a failure. The port's default creatureSpeed and its
post-attack reschedule are reported and not copied.
"""

import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OURS = ROOT / "src/core/include/daggorath/population.hpp"
MOVE = ROOT / "src/core/creature_move.cpp"


def parse_ours(text):
    block = text.split("kCreatureDefs{{", 1)[1].split("}};", 1)[0]
    rows = []
    for line in block.splitlines():
        match = re.search(r"\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\}", line)
        if match:
            rows.append(tuple(int(n) for n in match.groups()))
    return rows


def parse_port(text):
    rows = []
    for match in re.finditer(
        r"CDBTAB\[(\d+)\]\s*=\s*CDB\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\)",
        text,
    ):
        index = int(match.group(1))
        power, mgo, mgd, pho, phd, move_ms, atk_ms = (int(match.group(i)) for i in range(2, 9))
        rows.append((index, power, mgo, mgd, pho, phd, move_ms, atk_ms))
    rows.sort()
    return rows


def main():
    ours = parse_ours(OURS.read_text())
    if len(ours) != 12:
        print(f"expected 12 creature rows in {OURS}, found {len(ours)}")
        return 1
    move = MOVE.read_text()
    attack_at = move.find("creature_attack(self")
    returned = move.find("return attack;", attack_at)
    if attack_at < 0 or returned < 0 or returned - attack_at > 200:
        print("same-cell CMOVE must return the attack delay after creature_attack")
        return 1

    candidates = []
    if len(sys.argv) > 1:
        candidates.append(Path(sys.argv[1]))
    if os.environ.get("DAGGORATH_PORT"):
        candidates.append(Path(os.environ["DAGGORATH_PORT"]))
    candidates.append(ROOT.parent / "dungeons-of-daggorath")
    port_cpp = None
    for root in candidates:
        candidate = root / "src/creature.cpp"
        if candidate.is_file():
            port_cpp = candidate
            break
    if port_cpp is None:
        print("port tree not found; listing tables and attack delay checked only")
        return 0

    port_rows = parse_port(port_cpp.read_text())
    if len(port_rows) != 12:
        print(f"expected 12 CDBTAB rows in {port_cpp}, found {len(port_rows)}")
        return 1
    failed = False
    for index, row in enumerate(port_rows):
        _, power, mgo, mgd, pho, phd, move_ms, atk_ms = row
        ours_row = ours[index]
        port_as_tenths = (power, mgo, mgd, pho, phd, move_ms // 100, atk_ms // 100)
        if port_as_tenths != ours_row or move_ms % 100 or atk_ms % 100:
            print(f"type {index}: core {ours_row} port ms {(move_ms, atk_ms)} stats {(power, mgo, mgd, pho, phd)}")
            failed = True
    if failed:
        return 1
    port_obj = (port_cpp.parent / "object.cpp").read_text()
    ours_pop = (ROOT / "src/core/population.cpp").read_text()
    port_leather = re.search(
        r"Leather Shield\n\s*XXXTAB\[\d+\]\s*=\s*XDB\(0x10,\s*0x([0-9A-Fa-f]+),\s*0x([0-9A-Fa-f]+)",
        port_obj,
    )
    # The ShieldFix branch is a later assignment. The first Leather line is the listing pair.
    if port_leather is None:
        port_leather = re.search(
            r"XDB\(0x10,\s*0x([0-9A-Fa-f]+),\s*0x([0-9A-Fa-f]+),\s*0x00\);\s*// Leather Shield",
            port_obj,
        )
    ours_leather = re.search(r"\{(\d+),\s*(\d+),\s*0\}.*LEATHER", ours_pop)
    if port_leather is None or ours_leather is None:
        print("could not find leather shield defense bytes")
        return 1
    port_pair = (int(port_leather.group(1), 16), int(port_leather.group(2), 16))
    ours_pair = (int(ours_leather.group(1)), int(ours_leather.group(2)))
    if port_pair != ours_pair:
        print(f"leather shield core {ours_pair} port {port_pair}")
        return 1
    opts = port_cpp.parents[1] / "conf/opts.ini"
    speed = "unknown"
    if opts.is_file():
        for line in opts.read_text().splitlines():
            if line.startswith("creatureSpeed="):
                speed = line.split("=", 1)[1].strip()
    print(
        f"12 creature stat rows match {port_cpp}. "
        f"Leather shield defense is {ours_pair[0]},{ours_pair[1]} on both sides "
        "(physical 128, same as no shield). "
        f"Port opts creatureSpeed={speed} scales those delays; "
        "this core keeps the listing tenth-second attack delay."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
