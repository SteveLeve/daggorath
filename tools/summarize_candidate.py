#!/usr/bin/env python3
"""Summarise a Phase 5b candidate trace as a search milestone.

Diagnostic only: this ranks planner experiments. It does not qualify a replay;
tools/verify_playthrough.py owns acceptance.
"""
import argparse
import json
import re
import sys
from pathlib import Path

# Ordered ladder. Level indices are the core's (0-4); the image is type 10 and
# the wizard type 11 (game.cpp kill_creature: endgame_image / endgame_wizard).
LADDER = ['start', 'level-1', 'level-2', 'image-killed', 'level-3',
          'level-4', 'wizard-killed', 'winner']


def summarize(lines):
    level = 0
    save_level = {}
    best = 0
    peak_power = 0
    power = 0
    deaths = []
    saves = []
    loads = 0
    killed = {}
    visit = {}          # level-4 creature damage this visit, by slot
    best_visit = [0, None]
    last_jiffy = 0
    rung = {'start': 0}

    def close_visit():
        if visit:
            slot, damage = max(visit.items(), key=lambda item: item[1])
            if damage > best_visit[0]:
                best_visit[:] = [damage, slot]
            visit.clear()

    def reach(name, jiffy):
        nonlocal best
        index = LADDER.index(name)
        if index > best:
            best = index
        rung.setdefault(name, jiffy)

    for line in lines:
        if line.startswith('#'):
            continue
        fields = line.rstrip('\n').split('\t')
        if len(fields) != 4:
            continue
        jiffy, _clock, kind, detail = fields
        jiffy = int(jiffy)
        last_jiffy = jiffy
        if kind in {'CLIMB', 'ENDGAM', 'DEATH', 'RESTART', 'ZLOAD'}:
            close_visit()
        if kind == 'CLIMB':
            level = int(detail.split('=')[1])
            if 1 <= level <= 4:
                reach(f'level-{level}', jiffy)
        elif kind == 'ENDGAM' and detail == 'wizard':
            level = 3
            reach('level-3', jiffy)
        elif kind == 'KILL':
            kill_type = int(re.search(r'type=(\d+)', detail)[1])
            visit.pop(int(re.search(r'slot=(\d+)', detail)[1]), None)
            killed[kill_type] = killed.get(kill_type, 0) + 1
            if kill_type == 10:
                reach('image-killed', jiffy)
            elif kill_type == 11:
                reach('wizard-killed', jiffy)
        elif kind == 'WINNER':
            reach('winner', jiffy)
        elif kind == 'ABSORB':
            power = int(detail.split('=')[1])
            peak_power = max(peak_power, power)
        elif kind == 'DAMAGE' and level == 4:
            match = re.fullmatch(r'slot=(\d+) damage=(\d+)', detail)
            if match:
                slot, damage = int(match[1]), int(match[2])
                visit[slot] = max(visit.get(slot, 0), damage)
        elif kind == 'ZSAVE':
            name = detail.split()[0]
            save_level[name] = level
            saves.append([name, jiffy, level])
        elif kind == 'DEATH':
            deaths.append([jiffy, level, detail])
        elif kind == 'RESTART':
            level = 0
        elif kind == 'ZLOAD':
            loads += 1
            level = save_level.get(detail, level)
        elif kind == 'INIT':
            match = re.search(r'level=(\d+)', detail)
            if match:
                level = int(match[1])
    close_visit()
    return {
        'milestone': LADDER[best],
        'rank': best,
        'first_reached': rung,
        'peak_power': peak_power,
        'final_level': level,
        'last_jiffy': last_jiffy,
        'saves': saves,
        'deaths': len(deaths),
        'death_sites': deaths[-5:],
        'loads': loads,
        'kills_by_type': dict(sorted(killed.items())),
        # Untyped: the trace names slots, not creature types. The most damage
        # any unkilled level-4 creature took in one visit is usually WIZ1's
        # (power 8000), but confirm with the dplan dump's occupant line.
        'level4_top_unkilled_damage': best_visit[0],
        'level4_top_unkilled_slot': best_visit[1],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    args = parser.parse_args()
    with args.trace.open(encoding='utf-8') as handle:
        json.dump(summarize(handle), sys.stdout, indent=1)
    print()


if __name__ == '__main__':
    main()
