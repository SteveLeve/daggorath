#!/usr/bin/env python3
"""Collapse a dcli keystroke script into timed command lines for drive-web.

Prints "<seconds at 60 Hz>\t<line>" per CR.
"""
import sys

KEYS = {"SPACE": " ", "BS": "\b"}
line = ""
for raw in open(sys.argv[1]):
    if raw.startswith("#") or not raw.strip():
        continue
    jiffy, rest = raw.rstrip("\n").split(" ", 1)
    if rest == "CR":
        print(f"{int(jiffy) / 60:.3f}\t{line}")
        line = ""
    elif rest == "BS":
        line = line[:-1]
    else:
        line += KEYS.get(rest, rest)
