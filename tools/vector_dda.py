#!/usr/bin/env python3
"""Independent transliteration of VECTOR.ASM DIVIDE and the plot walk."""


def divide88(dividend, divisor):
    if dividend == 0:
        return 0
    if dividend == divisor:
        return 0x0100
    remainder = dividend
    quotient = 0
    for _ in range(16):
        remainder = (remainder << 1) & 0xFFFFFF
        quotient = (quotient << 1) & 0xFFFF
        top = (remainder >> 8) & 0xFFFF
        if top >= divisor:
            top = (top - divisor) & 0xFFFF
            remainder = (top << 8) | (remainder & 0xFF)
            quotient = (quotient + 1) & 0xFFFF
    return quotient


def negate16(value):
    return (~value + 1) & 0xFFFF


def pixels(x0, y0, x1, y1):
    dx = (x1 - x0) & 0xFFFF
    if dx >= 0x8000:
        dx -= 0x10000
    dy = (y1 - y0) & 0xFFFF
    if dy >= 0x8000:
        dy -= 0x10000
    length = max(abs(dx), abs(dy))
    if length == 0:
        return 0
    def step_of(delta):
        negative = delta < 0
        magnitude = negate16(delta & 0xFFFF) if negative else delta & 0xFFFF
        step = divide88(magnitude, length)
        if negative:
            step = negate16(step)
        sign = 0xFF if step & 0x8000 else 0
        value = (sign << 16) | step
        if value >= 0x800000:
            value -= 0x1000000
        return value
    x = (x0 << 8) | 0x80
    y = (y0 << 8) | 0x80
    xs, ys = step_of(dx), step_of(dy)
    count = 0
    for _ in range(length):
        if ((x >> 16) & 0xFF) == 0 and 0 <= ((y >> 8) & 0xFF) < 192:
            count += 1
        x = (x + xs) & 0xFFFFFFFF
        y = (y + ys) & 0xFFFFFFFF
        if x >= 0x80000000:
            x -= 0x100000000
        if y >= 0x80000000:
            y -= 0x100000000
    return count


if __name__ == "__main__":
    print(pixels(0, 0, 10, 0))
