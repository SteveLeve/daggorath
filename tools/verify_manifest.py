#!/usr/bin/env python3
"""Verify that every fixture still hashes to the value recorded in MANIFEST.json.

Usage: verify_manifest.py <fixture-dir>

Exit status 0 if every file matches, 1 otherwise. A mismatch means either the
extractor changed behaviour or a fixture was edited by hand; both need a recorded
reason in docs/archaeology/phase-0b/reconciliation.md before the manifest is
regenerated. Never regenerate a manifest just to make this pass.
"""
import hashlib
import json
import os
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__.strip())
        return 2
    d = sys.argv[1]
    manifest_path = os.path.join(d, "MANIFEST.json")
    try:
        with open(manifest_path) as f:
            manifest = json.load(f)
    except FileNotFoundError:
        print(f"no manifest at {manifest_path}")
        return 1

    listed = {e["file"] for e in manifest["fixtures"]}
    bad = 0
    for entry in manifest["fixtures"]:
        path = os.path.join(d, entry["file"])
        try:
            with open(path, "rb") as f:
                blob = f.read()
        except FileNotFoundError:
            print(f"MISSING  {entry['file']}")
            bad += 1
            continue
        got = hashlib.sha256(blob).hexdigest()
        if got != entry["sha256"]:
            print(f"MISMATCH {entry['file']}")
            print(f"         manifest {entry['sha256']}")
            print(f"         on disk  {got}")
            bad += 1
        elif len(blob) != entry["bytes"]:
            print(f"SIZE     {entry['file']}: {len(blob)} bytes, manifest says {entry['bytes']}")
            bad += 1

    present = {n for n in os.listdir(d) if n != "MANIFEST.json"}
    for extra in sorted(present - listed):
        print(f"UNLISTED {extra}")
        bad += 1

    print(f"{'FAILED' if bad else 'OK'}: {len(listed)} fixtures, {bad} problems")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
