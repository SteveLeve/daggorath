#!/usr/bin/env python3
"""Collapse a dcli keystroke script into timed command lines for drive-web.

Prints "<seconds at 60 Hz>\t<line>" per CR. FUDGE lines (D-12, harness-only)
have no equivalent in the web port and are reported on stderr, not replayed.
"""
import sys

KEYS = {"SPACE": " ", "BS": "\b"}
line, fudges = "", 0
for raw in open(sys.argv[1]):
    if raw.startswith("#") or not raw.strip():
        continue
    jiffy, rest = raw.rstrip("\n").split(" ", 1)
    if rest.startswith("FUDGE"):
        fudges += 1
        continue
    if rest == "CR":
        print(f"{int(jiffy) / 60:.3f}\t{line}")
        line = ""
    elif rest == "BS":
        line = line[:-1]
    else:
        line += KEYS.get(rest, rest)
print(f"skipped {fudges} FUDGE lines", file=sys.stderr)
