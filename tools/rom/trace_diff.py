#!/usr/bin/env python3
"""Report the first divergence between two traces in the Phase 0b format.

Trace lines are `jiffy <TAB> clock <TAB> event <TAB> detail`. Blank lines and
lines whose first non-whitespace character is '#' are ignored. The comparison
is line-oriented and stops at the first mismatch: the jiffy on each side and
both full lines. Exit status 0 means the traces agree, 1 means they diverge
or one is shorter, 2 means the invocation was wrong.
"""
import sys


def load(path):
    rows = []
    with open(path, encoding="utf-8") as f:
        for raw in f:
            line = raw.rstrip("\n")
            stripped = line.lstrip()
            if not stripped or stripped.startswith("#"):
                continue
            rows.append(line)
    return rows


def jiffy_of(line):
    return line.split("\t", 1)[0]


def compare(left_path, right_path, left, right):
    n = min(len(left), len(right))
    for i in range(n):
        if left[i] != right[i]:
            print(f"first divergence at trace line {i + 1}")
            print(f"  {left_path} jiffy {jiffy_of(left[i])}: {left[i]}")
            print(f"  {right_path} jiffy {jiffy_of(right[i])}: {right[i]}")
            return 1
    if len(left) != len(right):
        longer = left_path if len(left) > len(right) else right_path
        extra = (left if len(left) > len(right) else right)[n]
        print(f"first divergence at trace line {n + 1}")
        print(f"  {longer} continues at jiffy {jiffy_of(extra)}: {extra}")
        print(f"  the other trace ended after {n} lines")
        return 1
    print(f"no divergence ({n} lines)")
    return 0


def parse_args(argv):
    relative = False
    kinds = None
    paths = []
    i = 1
    while i < len(argv):
        arg = argv[i]
        if arg == "--relative-to-init":
            relative = True
        elif arg == "--kinds":
            i += 1
            if i >= len(argv):
                return None
            kinds = set(argv[i].split(","))
        else:
            paths.append(arg)
        i += 1
    if len(paths) != 2:
        return None
    return relative, kinds, paths[0], paths[1]


def kind_of(line):
    parts = line.split("\t")
    return parts[2] if len(parts) > 2 else ""


def rebase(rows, kinds):
    init_jiffy = None
    for line in rows:
        if kind_of(line) == "INIT":
            init_jiffy = int(jiffy_of(line))
            break
    if init_jiffy is None:
        init_jiffy = 0
    out = []
    for line in rows:
        kind = kind_of(line)
        if kind == "INIT":
            continue
        if kinds is not None and kind not in kinds:
            continue
        parts = line.split("\t")
        parts[0] = str(int(parts[0]) - init_jiffy)
        # Clock stays in the annotation but is not part of the comparison:
        # the INIT offset moves every later clock together.
        out.append("\t".join([parts[0], kind, parts[3] if len(parts) > 3 else ""]))
    return out


def main(argv):
    parsed = parse_args(argv)
    if parsed is None:
        print(
            "usage: trace_diff.py [--relative-to-init] [--kinds A,B] <trace-a> <trace-b>",
            file=sys.stderr,
        )
        return 2
    relative, kinds, left_path, right_path = parsed
    left, right = load(left_path), load(right_path)
    status = compare(left_path, right_path, left, right)
    if relative:
        print("relative to INIT (clock column omitted; INIT line skipped)")
        rel_status = compare(
            left_path, right_path, rebase(left, kinds), rebase(right, kinds)
        )
        if status == 0:
            status = rel_status
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv))
