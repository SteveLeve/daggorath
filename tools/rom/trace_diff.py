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


def main(argv):
    if len(argv) != 3:
        print("usage: trace_diff.py <trace-a> <trace-b>", file=sys.stderr)
        return 2
    left, right = load(argv[1]), load(argv[2])
    n = min(len(left), len(right))
    for i in range(n):
        if left[i] != right[i]:
            print(f"first divergence at trace line {i + 1}")
            print(f"  {argv[1]} jiffy {jiffy_of(left[i])}: {left[i]}")
            print(f"  {argv[2]} jiffy {jiffy_of(right[i])}: {right[i]}")
            return 1
    if len(left) != len(right):
        longer = argv[1] if len(left) > len(right) else argv[2]
        extra = (left if len(left) > len(right) else right)[n]
        print(f"first divergence at trace line {n + 1}")
        print(f"  {longer} continues at jiffy {jiffy_of(extra)}: {extra}")
        print(f"  the other trace ended after {n} lines")
        return 1
    print(f"no divergence ({n} lines)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
