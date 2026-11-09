"""Owned synthetic receipts around the production five-query algorithm; Game=0."""
import copy
import struct
import unittest
from unittest.mock import patch
from cover_volume_query import capture_volume_probe


def encoded(p):
    return struct.unpack('<3f', struct.pack('<3f', *p))


class VolumeControlsTests(unittest.TestCase):
    def probe(self, *, mutate=None, save=None, clock=None, **options):
        calls = []
        actor = dict(address=9, name='Wall', number=1)
        component = dict(address=90, name='Component', number=1)
        surface = dict(actor=actor, component=component, transform='unchanged')
        plans = ['current_line', 'stationary_separated', 'swept_extent', 'stationary_initial_overlap', 'leaving_initial_overlap']

        def original(observer, path, digest, start, end, **kwargs):
            index = len(calls)
            self.assertEqual(kwargs['surface_actor'], None if index == 0 else actor)
            self.assertEqual(kwargs['surface_component'], None if index == 0 else component)
            self.assertTrue(kwargs['snapshot_surface'])
            self.assertFalse(kwargs['trace_actors'])
            self.assertEqual(kwargs['extra_flags'], 8)
            calls.append((plans[index], start, end, kwargs['extent']))
            blocked = index != 1
            point = (50., 0., 0.) if index in (0, 2) else start
            normal = (-1., 0., 0.) if index in (0, 2, 4) else (0., 0., 0.)
            receipt = dict(accepted=True, trace_calls=1, pending_possible=False, allocation_retained=False,
                           session_id='owned', game_identity=dict(pid=100, created=200), owner_identity=dict(pid=300),
                           roles=dict(pawn=dict(address=1), controller=dict(address=2), link=dict(address=3), current_slot=0),
                           pawn_position=(0, 0, 0), module=dict(base=10, sha256='module'), function=dict(address=20),
                           surface_after=copy.deepcopy(surface),
                           inputs=dict(start=start, end=end, extent=kwargs['extent'], encoded_start=encoded(start),
                                       encoded_end=encoded(end), encoded_extent=encoded(kwargs['extent']),
                                       stationary=start == end, trace_actors=False, extra_flags=8),
                           hit=dict(actor_address=9 if blocked else 0, location=point if blocked else (0, 0, 0),
                                    normal=normal if blocked else (0, 0, 0)))
            if blocked:
                receipt['hit'].update(actor_identity=copy.deepcopy(actor), component_identity=copy.deepcopy(component))
            if index:
                receipt['surface_before'] = copy.deepcopy(surface)
            if mutate:
                mutate(receipt, index)
            return receipt

        if clock is None:
            result = capture_volume_probe(None, 'loaded.dll', 'digest', (0., 0., 0.), (100., 0., 0.),
                                          capture_query=original, save_result=save, **options)
        else:
            with patch('cover_volume_query.time.monotonic', side_effect=clock):
                result = capture_volume_probe(None, 'loaded.dll', 'digest', (0., 0., 0.), (100., 0., 0.),
                                              capture_query=original, save_result=save, **options)
        return result, calls

    def test_five_original_controls_retain_zero_normal_and_exact_stationary_points(self):
        result, calls = self.probe()
        self.assertTrue(result['accepted'], result.get('rejection'))
        self.assertEqual(len(calls), 5)
        self.assertEqual(calls[1][1], calls[1][2])
        self.assertEqual(calls[3][1], calls[3][2])
        self.assertEqual(calls[3][1], (55., 0., 0.))
        self.assertEqual(result['queries'][3]['returned_normal'], (0., 0., 0.))
        self.assertFalse(result['raw_hit_time_available'])
        for flag in ['collision_verified', 'body_modified', 'grab_authorized', 'shape_semantics_verified']:
            self.assertFalse(result[flag])

    def test_bad_or_unknown_original_call_stops_without_retry(self):
        for change in [dict(accepted=False), dict(trace_calls=0), dict(trace_calls=True),
                       dict(pending_possible=True), dict(allocation_retained=True)]:
            for at in range(5):
                with self.subTest(change=change, at=at):
                    result, calls = self.probe(mutate=lambda r, i: r.update(change) if i == at else None)
                    self.assertFalse(result['accepted'])
                    self.assertEqual(len(calls), at + 1)
                    self.assertEqual(len(result['queries']), at + 1)

    def test_changed_cohort_fails_after_second_query(self):
        for key in ['session_id', 'game_identity', 'owner_identity', 'roles', 'pawn_position', 'module', 'function']:
            with self.subTest(key=key):
                result, calls = self.probe(mutate=lambda r, i: r.update({key: 'changed'}) if i == 1 else None)
                self.assertFalse(result['accepted'])
                self.assertEqual(len(calls), 2)

    def test_changed_surface_and_component_fail(self):
        def mutate(r, i):
            if i == 2:
                r['surface_after']['transform'] = 'changed'
        result, calls = self.probe(mutate=mutate)
        self.assertFalse(result['accepted'])
        self.assertEqual(len(calls), 3)
        def other(r, i):
            if i == 2:
                r['hit']['component_identity']['address'] = 91
        result, calls = self.probe(mutate=other)
        self.assertFalse(result['accepted'])
        self.assertEqual(len(calls), 3)

    def test_encoded_point_extent_or_stationary_claim_cannot_change(self):
        for key, value in [('encoded_start', (1, 2, 3)), ('encoded_extent', (0, 0, 0)),
                           ('stationary', False), ('trace_actors', True), ('extra_flags', 0)]:
            with self.subTest(key=key):
                result, calls = self.probe(mutate=lambda r, i: r['inputs'].update({key: value}) if i == 1 else None)
                self.assertFalse(result['accepted'])
                self.assertEqual(len(calls), 2)

    def test_missing_initial_overlap_is_a_failure_not_empty_occupancy_permission(self):
        def mutate(r, i):
            if i == 3:
                r['hit'] = dict(actor_address=0, location=(0, 0, 0), normal=(0, 0, 0))
        result, calls = self.probe(mutate=mutate)
        self.assertFalse(result['accepted'])
        self.assertEqual(len(calls), 4)
        self.assertIn('stationary_initial_overlap', result['rejection'])

    def test_separated_control_hit_is_not_ignored(self):
        def mutate(r, i):
            if i == 1:
                r['hit'] = dict(actor_address=9, actor_identity=dict(address=9, name='Wall', number=1),
                                component_identity=dict(address=90, name='Component', number=1), location=(0, 0, 0), normal=(0, 0, 0))
        result, calls = self.probe(mutate=mutate)
        self.assertFalse(result['accepted'])
        self.assertEqual(len(calls), 2)

    def test_moving_out_of_overlap_miss_is_preserved_and_stops(self):
        def mutate(r, i):
            if i == 4:
                r['hit'] = dict(actor_address=0, location=(0, 0, 0), normal=(0, 0, 0))
        result, calls = self.probe(mutate=mutate)
        self.assertFalse(result['accepted'])
        self.assertEqual(len(calls), 5)
        self.assertIn('leaving_initial_overlap', result['rejection'])

    def test_save_records_unknown_before_each_external_call_and_cannot_mutate_result(self):
        snapshots = []
        def save(value):
            snapshots.append(copy.deepcopy(value))
            value['accepted'] = 'forged'
        result, calls = self.probe(save=save)
        self.assertTrue(result['accepted'])
        unknown = [s for s in snapshots if s['queries'] and s['queries'][-1]['outcome'] == 'attempted_result_unknown']
        self.assertEqual(len(unknown), 5)
        self.assertEqual([len(s['queries']) for s in unknown], [1, 2, 3, 4, 5])

    def test_save_failure_before_call_submits_nothing(self):
        def fail(value):
            raise OSError('owned report unavailable')
        result, calls = self.probe(save=fail)
        self.assertFalse(result['accepted'])
        self.assertEqual(calls, [])
        self.assertIn('save_error', result)

    def test_budget_expiry_after_completed_call_stops(self):
        result, calls = self.probe(clock=[0, 0, 31])
        self.assertFalse(result['accepted'])
        self.assertEqual(len(calls), 1)
        self.assertEqual(result['rejection_type'], 'TimeoutError')

    def test_invalid_limits_never_call_original(self):
        for options in [dict(extent=(0, 1, 1)), dict(extent=(31, 1, 1)), dict(extent=(1, float('nan'), 1)),
                        dict(maximum_seconds=31), dict(maximum_seconds=True), dict(maximum_seconds=0)]:
            with self.subTest(options=options):
                result, calls = self.probe(**options)
                self.assertFalse(result['accepted'])
                self.assertEqual(calls, [])


if __name__ == '__main__':
    unittest.main()
