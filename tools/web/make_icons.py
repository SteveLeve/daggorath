#!/usr/bin/env python3
"""Draw the PWA icons for the web build (ADR-0011).

Original artwork: a one-point-perspective corridor in white vectors on black,
in the spirit of the game's line-drawn view. Nothing is taken from the ROM,
the manual or any port. Usage: make_icons.py OUT_DIR
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw


def corridor(size: int, pad_frac: float) -> Image.Image:
    img = Image.new("RGB", (size, size), (0, 0, 0))
    d = ImageDraw.Draw(img)
    w = max(2, size // 48)
    pad = size * pad_frac
    outer = (pad, pad, size - pad, size - pad)
    ink = (255, 255, 255)
    rects = [outer]
    for k in (0.28, 0.44, 0.54):  # receding doorframes
        inset = (size - 2 * pad) * k / 2 + pad
        rects.append((inset, inset + size * 0.02, size - inset, size - inset))
    for r in rects:
        d.rectangle(r, outline=ink, width=w)
    far = rects[-1]
    for (x0, y0), (x1, y1) in (
        ((outer[0], outer[1]), (far[0], far[1])),
        ((outer[2], outer[1]), (far[2], far[1])),
        ((outer[0], outer[3]), (far[0], far[3])),
        ((outer[2], outer[3]), (far[2], far[3])),
    ):
        d.line((x0, y0, x1, y1), fill=ink, width=w)
    return img


def main() -> None:
    out = Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    corridor(192, 0.08).save(out / "icon-192.png")
    corridor(512, 0.08).save(out / "icon-512.png")
    corridor(512, 0.2).save(out / "icon-maskable-512.png")  # inside the safe zone
    corridor(180, 0.08).save(out / "apple-touch-icon.png")


if __name__ == "__main__":
    main()
