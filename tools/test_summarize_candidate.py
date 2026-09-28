"""Milestone ranking for search diagnostics; synthetic rows, no game data."""
import unittest
from summarize_candidate import summarize

HEADER = '# jiffy\tclock\tevent\tdetail'


def rows(*events):
    return [HEADER, '0\t0:0:6.2.5\tINIT\tlevel=0 row=16 col=11 dir=N second=6'] + [
        f'{n + 1}\t0:0:6.3.0\t{kind}\t{detail}' for n, (kind, detail) in enumerate(events)]


class SummaryTests(unittest.TestCase):
    def test_ladder_keeps_furthest_rung_through_death(self):
        s = summarize(rows(('ZSAVE', 'A bytes=1'), ('CLIMB', 'level=1'), ('CLIMB', 'level=2'),
                           ('KILL', 'slot=3 type=10 matrix=0'), ('ENDGAM', 'image'),
                           ('ENDGAM', 'wizard'), ('DEATH', 'power=1 damage=2'),
                           ('RESTART', 'GAME after death'), ('ZLOAD', 'A')))
        self.assertEqual(s['milestone'], 'level-3')
        self.assertEqual(s['final_level'], 0)
        self.assertEqual(s['deaths'], 1)

    def test_load_restores_save_level(self):
        s = summarize(rows(('CLIMB', 'level=4'), ('ZSAVE', 'W bytes=1'),
                           ('RESTART', 'GAME after death'), ('ZLOAD', 'W')))
        self.assertEqual(s['final_level'], 4)

    def test_level4_damage_ignores_killed_slots(self):
        s = summarize(rows(('CLIMB', 'level=4'), ('DAMAGE', 'slot=1 damage=900'),
                           ('KILL', 'slot=1 type=5 matrix=0'), ('DAMAGE', 'slot=2 damage=400'),
                           ('DEATH', 'power=1 damage=2'), ('RESTART', 'GAME after death')))
        self.assertEqual((s['level4_top_unkilled_damage'], s['level4_top_unkilled_slot']), (400, 2))

    def test_winner_rung(self):
        s = summarize(rows(('KILL', 'slot=0 type=11 matrix=0'), ('WINNER', 'final ring')))
        self.assertEqual(s['milestone'], 'winner')


if __name__ == '__main__':
    unittest.main()
