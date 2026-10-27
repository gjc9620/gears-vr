import unittest

from analyze_lancer_reload_visual_runtime import summarize


def row(index, *, reloading=False, control=0.0, mag=False, ammo=1):
    return {'sample': index, 'qpc': index + 1, 'ms': index * 20,
            'world': 4 + index * .02, 'reload_start': 4 if index else 0,
            'reloading': reloading, 'control': control, 'target': control,
            'scale': 0.0, 'mag_pawn': mag, 'mag_hidden': False,
            'ammo_used': ammo, 'spare': 360}


class LancerReloadVisualTest(unittest.TestCase):
    def fixture(self):
        rows = [row(0)]
        rows += [row(i, reloading=True, control=float(i >= 3),
                     mag=5 <= i < 7) for i in range(1, 7)]
        rows += [row(7, reloading=True)]
        rows += [row(8, ammo=0), row(9, ammo=0), row(10, ammo=0)]
        return {'passed': True, 'shutdown': 0, 'game_pid': 12,
                'seeded_ammo_used': 1, 'reload_visual_timeline': rows}

    def test_ordered_original_handoff(self):
        result = summarize(self.fixture())
        bounds = result['events']['hand_magazine_attached']['elapsed_s']
        self.assertAlmostEqual(bounds[0], .08)
        self.assertAlmostEqual(bounds[1], .1)
        self.assertEqual(result['events']['gun_magazine_control_off']['sample_after'], 7)
        self.assertEqual(result['events']['reload_finished']['sample_after'], 8)

    def test_rejects_missing_handoff(self):
        capture = self.fixture()
        for row_value in capture['reload_visual_timeline']:
            row_value['mag_pawn'] = False
        with self.assertRaisesRegex(ValueError, 'transition'):
            summarize(capture)


if __name__ == '__main__':
    unittest.main()
