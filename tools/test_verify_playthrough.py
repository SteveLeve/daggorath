"""Negative acceptance cases; no expected game hash or fixture is synthesized."""
import hashlib
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import subprocess
from verify_playthrough import InvalidReplay, validate_script, validate_trace, qualify

HEADER = '# jiffy\tclock\tevent\tdetail\n'
FINAL = '# final\trow=16\tcol=11\tdir=0\tdamage=0\n'
ROWS = [
    '0\t0:0:6.2.5\tINIT\tlevel=0 row=16 col=11 dir=N second=6',
    '1\t0:0:6.3.0\tZSAVE\tQUEST bytes=8000',
    '2\t0:0:6.3.1\tDEATH\tpower=160 damage=165',
    '3\t0:0:0.0.0\tRESTART\tGAME after death',
    '4\t0:0:6.3.0\tZLOAD\tQUEST',
    '5\t0:0:6.3.1\tWINNER\tfinal ring',
]


def trace(rows=ROWS):
    return (HEADER + '\n'.join(rows) + '\n' + FINAL).encode()


class AcceptanceTests(unittest.TestCase):
    def test_valid_contract(self):
        self.assertEqual(validate_trace(trace())['recoveries'], 1)
        validate_script('0 Z\n1 SPACE\n2 CR\n', 6)

    def test_timed_newlvl_event_is_vocabulary(self):
        # D-19: game.cpp build_resume emits NEWLVL for every timed level build.
        rows = ROWS[:2] + ['1\t0:0:6.3.0\tNEWLVL\tlevel=1 second=54'] + ROWS[2:]
        self.assertEqual(validate_trace(trace(rows))['recoveries'], 1)

    def test_script_bypasses(self):
        for bad in ['0 FUDGE rest', 'FUDGE incoming 25', '0 SNAPSHOT x',
                    '0 CR extra', '0 7', '-1 A', '2 A\n1 B', '6 A', '# empty']:
            with self.subTest(bad=bad), self.assertRaises(InvalidReplay):
                validate_script(bad, 6)

    def test_trace_progress_bypasses(self):
        variants = [
            ROWS[:-1], ROWS[:2] + ROWS[5:],
            [row for row in ROWS if '\tRESTART\t' not in row],
            [row for row in ROWS if '\tZLOAD\t' not in row],
            [row.replace('ZLOAD\tQUEST', 'ZLOAD\tOLD') for row in ROWS],
            ROWS + [ROWS[2]], ROWS + [ROWS[-1]],
            ROWS[:2] + [ROWS[1]] + ROWS[2:],
            ROWS[:2] + [ROWS[-1].replace('5\t', '2\t')] + ROWS[2:-1],
            ROWS[:-1] + ['5\t0:0:0.0.0\tBOGUS\tunknown'] + [ROWS[-1]],
            [row.replace('0:0:6.3.1', '0:99:99.99.99') for row in ROWS],
            [row.replace('WINNER\tfinal ring', 'OUTPUT\tWINNER') for row in ROWS],
            [ROWS[0].replace('second=6', 'second=1')] + ROWS[1:],
            ROWS[:2] + ['2\t0:0:0.0.0\tFUDGE\trest'] + ROWS[2:],
        ]
        for rows in variants:
            with self.subTest(rows=rows), self.assertRaises(InvalidReplay):
                validate_trace(trace(rows))

    def test_every_death_and_latest_save(self):
        second = [
            '5\t0:0:6.3.1\tDEATH\tpower=160 damage=165',
            '6\t0:0:0.0.0\tRESTART\tGAME after death',
            '7\t0:0:6.3.0\tZLOAD\tQUEST',
            '8\t0:0:6.3.1\tWINNER\tfinal ring',
        ]
        self.assertEqual(validate_trace(trace(ROWS[:-1] + second))['recoveries'], 2)
        with self.assertRaises(InvalidReplay):
            validate_trace(trace(ROWS[:-1] + second[:1] + second[-1:]))
        older = ROWS[:2] + ['1\t0:0:6.3.0\tZSAVE\tLATEST bytes=8000'] + ROWS[2:]
        with self.assertRaises(InvalidReplay):
            validate_trace(trace(older))

    def test_malformed_trace(self):
        for data in [b'', b'WINNER', b'\xff', trace()[:-10], trace().replace(b'5\t0:', b'x\t0:'),
                     trace().replace(b'\tWINNER\t', b'\tWINNER\tbogus\t'),
                     trace().replace(b'# final', b'# spoof')]:
            with self.subTest(data=data), self.assertRaises(InvalidReplay):
                validate_trace(data)

    def test_process_failures_and_missing_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            script = Path(tmp) / 'keys'; script.write_text('0 A\n')
            for failure in [FileNotFoundError('missing binary'),
                            subprocess.CalledProcessError(1, ['dcli']),
                            subprocess.TimeoutExpired(['dcli'], 1)]:
                with patch('verify_playthrough.subprocess.run', side_effect=failure), self.assertRaises(InvalidReplay):
                    qualify(Path('/fake/dcli'), script, 6, 1)
            def empty(args, **kwargs):
                Path(args[-1]).write_bytes(b'')
            with patch('verify_playthrough.subprocess.run', side_effect=empty), self.assertRaises(InvalidReplay):
                qualify(Path('/fake/dcli'), script, 6, 1)
            with patch('verify_playthrough.subprocess.run'), self.assertRaises(InvalidReplay):
                qualify(Path('/fake/dcli'), script, 6, 1)

    def test_trace_and_hash_mismatch(self):
        with tempfile.TemporaryDirectory() as tmp:
            script = Path(tmp) / 'keys'; script.write_text('0 A\n')
            def output(args, **kwargs):
                Path(args[-1]).write_bytes(trace())
            with patch('verify_playthrough.subprocess.run', side_effect=output), self.assertRaises(InvalidReplay):
                qualify(Path('/fake/dcli'), script, 6, 1, '0' * 64)
            calls = []
            def varying(args, **kwargs):
                calls.append(1)
                Path(args[-1]).write_bytes(trace().replace(b'bytes=8000', f'bytes={8000+len(calls)}'.encode()))
            with patch('verify_playthrough.subprocess.run', side_effect=varying), self.assertRaises(InvalidReplay):
                qualify(Path('/fake/dcli'), script, 6, 1)
            with patch('verify_playthrough.subprocess.run', side_effect=output), self.assertRaises(InvalidReplay):
                qualify(Path('/fake/dcli'), script, 4, 1)
            with patch('verify_playthrough.subprocess.run', side_effect=output):
                digest, _ = qualify(Path('/fake/dcli'), script, 6, 1)
                self.assertEqual(digest, hashlib.sha256(trace()).hexdigest())


if __name__ == '__main__':
    unittest.main()
