#!/usr/bin/env python3
"""Check that relative Markdown links in docs/ and the root README resolve."""
import pathlib, re, sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
LINK = re.compile(r"\]\(([^)\s]+)\)")

def files():
    yield ROOT / "README.md"
    yield ROOT / "CLAUDE.md"
    yield from sorted((ROOT / "docs").rglob("*.md"))

problems = 0
for md in files():
    for n, line in enumerate(md.read_text(encoding="utf-8").splitlines(), 1):
        for target in LINK.findall(line):
            if re.match(r"[a-z]+:", target) or target.startswith("#"):
                continue
            path = target.split("#", 1)[0]
            if path and not (md.parent / path).exists():
                print(f"{md.relative_to(ROOT)}:{n}: missing {target}")
                problems += 1
print(f"{'OK' if not problems else 'FAIL'}: {problems} broken link(s)")
sys.exit(1 if problems else 0)
