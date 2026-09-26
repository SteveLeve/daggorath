#!/usr/bin/env python3
"""Decode every `SWI / FCB OUTSTI` string in the pinned listing.

EXPAND.ASM EXPANX: the first 5-bit field is a count, then count + 1 characters
follow (DECA / BPL). Character codes are CD.ASM's I.* values: 0 space, 1-26
A-Z, $1B '!', $1C '_', $1D '?', $1E '.', $1F carriage return (printed '^').

Usage: tools/decode_outsti.py [ASM_DIR]
"""
import glob
import os
import re
import sys

CHARS = {0x00: " ", 0x1B: "!", 0x1C: "_", 0x1D: "?", 0x1E: ".", 0x1F: "^"}
CHARS.update({i: chr(ord("A") + i - 1) for i in range(1, 27)})


def decode(bits):
    fields = [int(bits[k:k + 5], 2) for k in range(0, len(bits) - 4, 5)]
    count = fields[0]
    return "".join(CHARS.get(c, "<%d>" % c) for c in fields[1:count + 2])


def main():
    asm_dir = sys.argv[1] if len(sys.argv) > 1 else "third_party/dod-asm"
    for path in sorted(glob.glob(os.path.join(asm_dir, "*.ASM"))):
        with open(path, errors="replace") as f:
            lines = f.read().splitlines()
        i = 0
        while i < len(lines):
            if re.search(r"FCB\s+OUTSTI", lines[i]):
                bits = ""
                j = i + 1
                while j < len(lines) and re.match(r"\s+FCB\s+%[01]{8}", lines[j]):
                    bits += re.search(r"%([01]{8})", lines[j]).group(1)
                    j += 1
                if bits:
                    print("%s:%d\t%s" % (os.path.basename(path), i + 1, decode(bits)))
                i = j
            else:
                i += 1


if __name__ == "__main__":
    main()
