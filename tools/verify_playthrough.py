#!/usr/bin/env python3
"""Independently qualify a legal power-on replay, or check its recorded digest."""
import argparse
import hashlib
import re
import subprocess
import tempfile
from pathlib import Path


# Closed vocabulary emitted by core/game.cpp, creature_move.cpp and scheduler.cpp.
EVENT_KINDS = set("""ABSORB BURDEN CLIMB DAMAGE DARK DEATH DIALOGUE DROP ENDGAM
EXAMINE EXERT FAINT FUDGE GET HIT INCANT INIT KILL LINE LOOK LOOT MAP MISS MOVE
OUTPUT PICKUP PULL PUPDAT QUEUE RELOCATE RESTART REVEAL REVIVE RING SOUND STOW
SYNC TASK TORCH TURN USE WINNER ZLOAD ZSAVE""".split())


class InvalidReplay(ValueError):
    pass


def validate_script(text, jiffies):
    last = -1
    count = 0
    for line in text.splitlines():
        line = line.split('#', 1)[0].strip()
        if not line:
            continue
        match = re.fullmatch(r'(\d+) (SPACE|CR|BS|[A-Z])', line)
        if not match:
            raise InvalidReplay('malformed or forbidden script directive: ' + line)
        stamp = int(match[1])
        if stamp < last or stamp >= jiffies:
            raise InvalidReplay('script timestamps unordered or outside replay')
        last = stamp
        count += 1
    if not count:
        raise InvalidReplay('empty script')


def validate_trace(data):
    try:
        lines = data.decode('utf-8').splitlines()
    except UnicodeDecodeError as exc:
        raise InvalidReplay('trace is not UTF-8') from exc
    if not lines or lines[0] != '# jiffy\tclock\tevent\tdetail':
        raise InvalidReplay('missing trace header')
    previous = -1
    latest = None
    pending = None
    saves = deaths = recoveries = winners = 0
    save_names = set()
    initialized = False
    if not re.fullmatch(r'# final\trow=\d+\tcol=\d+\tdir=[0-3]\tdamage=\d+', lines[-1]):
        raise InvalidReplay('missing or malformed final summary')
    for line in lines[1:-1]:
        fields = line.split('\t')
        if len(fields) != 4:
            raise InvalidReplay('malformed trace row')
        stamp, clock, kind, detail = fields
        if not re.fullmatch(r'\d+', stamp) or not re.fullmatch(r'\d+:\d+:\d+\.\d+\.\d+', clock):
            raise InvalidReplay('malformed trace time')
        time = [int(n) for n in re.split(r'[:.]', clock)]
        if any(n >= limit for n, limit in zip(time, [24, 60, 60, 10, 6])):
            raise InvalidReplay('clock counters outside valid ranges')
        stamp = int(stamp)
        if stamp < previous or kind not in EVENT_KINDS:
            raise InvalidReplay('unordered time or malformed event')
        previous = stamp
        if winners and kind not in {'DIALOGUE', 'QUEUE', 'SYNC'}:
            raise InvalidReplay('gameplay progression after victory')
        if not initialized:
            if kind != 'INIT' or stamp != 0 or detail != 'level=0 row=16 col=11 dir=N second=6':
                raise InvalidReplay('not default Original Mode startup')
            initialized = True
        if kind == 'FUDGE':
            raise InvalidReplay('forbidden FUDGE event')
        if kind == 'ZSAVE':
            match = re.fullmatch(r'([A-Z]{1,8}) bytes=([1-9]\d*)', detail)
            if not match or pending:
                raise InvalidReplay('invalid save or save during unrecovered death')
            latest = match[1]
            if latest in save_names:
                raise InvalidReplay('checkpoint names are not unique')
            save_names.add(latest)
            saves += 1
        elif kind == 'DEATH':
            if pending or not latest:
                raise InvalidReplay('death without completed checkpoint/recovery')
            pending = [latest, False]
            deaths += 1
        elif kind == 'RESTART':
            if not pending or pending[1] or detail != 'GAME after death':
                raise InvalidReplay('unexpected restart')
            pending[1] = True
        elif kind == 'ZLOAD':
            if pending:
                if not pending[1] or detail != pending[0]:
                    raise InvalidReplay('death reload is not latest successful save after restart')
                pending = None
                recoveries += 1
            elif detail != latest:
                raise InvalidReplay('load is not latest successful save')
        elif kind == 'WINNER':
            if pending or not recoveries or detail != 'final ring':
                raise InvalidReplay('invalid victory')
            winners += 1
    if not initialized or not saves or not deaths or pending or recoveries != deaths or winners != 1:
        raise InvalidReplay('missing save, complete death recovery, or unique victory')
    return dict(saves=saves, deaths=deaths, recoveries=recoveries, last_jiffy=previous)


def qualify(dcli, script, jiffies, timeout, expected=None):
    validate_script(script.read_text(), jiffies)
    traces = []
    with tempfile.TemporaryDirectory(prefix='daggorath-qualification-') as tmp:
        for attempt in range(2):
            output = Path(tmp) / f'{attempt}.trace'
            try:
                subprocess.run([str(dcli.resolve()), '--script', str(script.resolve()),
                                '--jiffies', str(jiffies), '--trace', str(output)],
                               check=True, timeout=timeout, capture_output=True)
            except (OSError, subprocess.SubprocessError) as exc:
                raise InvalidReplay('replay process failed or timed out: ' + str(exc)) from exc
            if not output.is_file() or not output.stat().st_size:
                raise InvalidReplay('replay output missing or empty')
            data = output.read_bytes()
            summary = validate_trace(data)
            if summary['last_jiffy'] > jiffies:
                raise InvalidReplay('trace extends beyond requested replay')
            traces.append(data)
    if traces[0] != traces[1]:
        raise InvalidReplay('replay traces differ')
    digest = hashlib.sha256(traces[0]).hexdigest()
    if expected is not None and digest != expected:
        raise InvalidReplay('recorded SHA-256 mismatch')
    return digest, validate_trace(traces[0])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dcli', type=Path, required=True)
    parser.add_argument('--script', type=Path, required=True)
    parser.add_argument('--jiffies', type=int, required=True)
    parser.add_argument('--timeout', type=float, required=True)
    parser.add_argument('--sha256', help='recorded regression digest; omit to qualify a new candidate')
    args = parser.parse_args()
    try:
        if args.jiffies <= 0 or args.timeout <= 0:
            raise InvalidReplay('positive replay limits required')
        if args.sha256 and not re.fullmatch('[0-9a-f]{64}', args.sha256):
            raise InvalidReplay('malformed expected digest')
        digest, summary = qualify(args.dcli, args.script, args.jiffies, args.timeout, args.sha256)
    except (InvalidReplay, OSError) as exc:
        parser.exit(1, str(exc) + '\n')
    print('SHA256', digest)
    print(summary)


if __name__ == '__main__':
    main()
