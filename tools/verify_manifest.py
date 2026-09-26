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


def check_population(fixture_dir: str) -> int:
    """Phase 1 coverage beyond the hash: five levels, and a recorded source.

    population.json is the human-readable record. population-entry.txt is the
    line contract the C++ tests read. Both must name the same levels.
    """
    json_path = os.path.join(fixture_dir, "population.json")
    if not os.path.exists(json_path):
        return 0
    bad = 0
    text_path = os.path.join(fixture_dir, "population-entry.txt")
    try:
        with open(json_path) as f:
            doc = json.load(f)
    except (OSError, json.JSONDecodeError) as exc:
        print(f"POPULATION {json_path}: {exc}")
        return 1
    if not str(doc.get("source", "")).strip():
        print("POPULATION population.json has no source")
        bad += 1
    if not str(doc.get("extraction_method", "")).strip():
        print("POPULATION population.json has no extraction_method")
        bad += 1
    entries = doc.get("entries")
    if not isinstance(entries, list):
        print("POPULATION population.json has no entries list")
        return bad + 1
    at_second_one = [e for e in entries if e.get("second") == 1]
    levels = {e.get("level") for e in at_second_one}
    if levels != set(range(5)):
        print(f"POPULATION SECOND=1 levels are {sorted(levels)}, expected 0..4")
        bad += 1
    required = ("slot", "type", "row", "col", "power", "use")
    for entry in entries:
        creatures = entry.get("creatures")
        if not isinstance(creatures, list):
            print(f"POPULATION level {entry.get('level')} has no creature list")
            bad += 1
            continue
        if entry.get("live") != len(creatures):
            print(f"POPULATION level {entry.get('level')} live count disagrees "
                  f"with the creature list")
            bad += 1
        if entry.get("occupied_0_0"):
            print(f"POPULATION level {entry.get('level')} second {entry.get('second')} "
                  "places a creature at (0, 0)")
            bad += 1
        for creature in creatures:
            missing = [k for k in required if k not in creature]
            if missing:
                print(f"POPULATION creature record missing {missing}")
                bad += 1
                break
    try:
        text = open(text_path, encoding="utf-8").read()
    except OSError as exc:
        print(f"POPULATION {text_path}: {exc}")
        return bad + 1
    if not text.startswith("#") or "source:" not in text.split("\n", 3)[1]:
        print("POPULATION population-entry.txt header does not name a source")
        bad += 1
    for level in range(5):
        if f"creature {level} 1 " not in text:
            print(f"POPULATION population-entry.txt has no level {level} at SECOND=1")
            bad += 1
    if "\ncregen " not in text and not text.startswith("cregen "):
        print("POPULATION population-entry.txt has no cregen line")
        bad += 1
    return bad


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

    present = {n for n in os.listdir(d)
               if n != "MANIFEST.json" and not os.path.isdir(os.path.join(d, n))}
    for extra in sorted(present - listed):
        print(f"UNLISTED {extra}")
        bad += 1

    if "population.json" in listed:
        bad += check_population(d)

    print(f"{'FAILED' if bad else 'OK'}: {len(listed)} fixtures, {bad} problems")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
