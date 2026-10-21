import unittest

from mod_audit import publication_stalls


def sample(second, *, published=10, world=1, reference=1, fresh=True):
    return dict(observed_ns=round((second + .2) * 1e9), menu_mode=1,
                native=dict(telemetry_qpc=second * 10_000_000, telemetry_qpc_frequency=10_000_000,
                            telemetry_present_calls=100 + second * 30, published=published,
                            world_generation=world, reference_generation=reference),
                periods=dict(native_seconds=1), source_states=dict(native='fresh' if fresh else 'stale'),
                source_errors={})


class ModAuditTests(unittest.TestCase):
    def test_fresh_game_present_with_frozen_stereo_requires_three_seconds(self):
        rows = [sample(t) for t in range(1, 6)]
        result = publication_stalls(rows, [dict(observed_ns=0, mode=1)])
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['seconds'], 4)
        self.assertFalse(result[0]['confirmed'])
        self.assertEqual(result[0]['classification'], 'mod_candidate')

    def test_movie_ledger_overrides_stale_gameplay_mode_in_telemetry(self):
        rows = [sample(t) for t in range(1, 10)]
        ledger = [dict(observed_ns=0, mode=1), dict(observed_ns=2_000_000_000, mode=7),
                  dict(observed_ns=8_000_000_000, mode=1)]
        self.assertEqual(publication_stalls(rows, ledger), [])

    def test_stale_missing_world_reference_and_unknown_menu_break_continuity(self):
        ledger = [dict(observed_ns=0, mode=1)]
        missing = dict(observed_ns=3_200_000_000, native={})
        for changed in (sample(3, fresh=False), sample(3, world=2), sample(3, reference=2), missing):
            with self.subTest(changed=changed):
                rows = [sample(1), sample(2), changed, sample(4), sample(5)]
                self.assertEqual(publication_stalls(rows, ledger), [])
        self.assertEqual(publication_stalls([sample(t) for t in range(1, 8)], []), [])

    def test_movie_boundary_inside_measured_period_does_not_become_stall(self):
        rows = [sample(t) for t in range(1, 7)]
        ledger = [dict(observed_ns=0, mode=1), dict(observed_ns=2_500_000_000, mode=7),
                  dict(observed_ns=3_500_000_000, mode=1)]
        self.assertEqual(publication_stalls(rows, ledger), [])


if __name__ == '__main__':
    unittest.main()
