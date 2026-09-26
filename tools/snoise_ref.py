#!/usr/bin/env python3
"""Independent transliteration of SOUNDS.ASM SNOISE."""


def snoise(state):
    value = (state << 1) & 0xFFFF
    value = (value << 1) & 0xFFFF
    value = (value + state) & 0xFFFF
    value = (value & 0xFF00) | ((value + 1) & 0xFF)
    return value


if __name__ == "__main__":
    state = 1
    out = []
    for _ in range(4):
        state = snoise(state)
        out.append(state)
    print(" ".join(str(v) for v in out))
