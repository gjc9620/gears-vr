import copy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import Mock, patch

from control import RunControl, validate
from core import atomic_json
from test_core import sample


class ControlIntegrationTests(unittest.TestCase):
    def test_local_control_failure_holds_without_reloading_an_owned_checkpoint(self):
        from core import Evidence
        from run import observe
        for explicit_hold in (True, False):
            with self.subTest(explicit_hold=explicit_hold), tempfile.TemporaryDirectory() as directory:
                report = Evidence(Path(directory) / 'run', mode='run', provenance={})
                identities = dict(game_identity=dict(pid=10, creation_time=100),
                                  owner_identity=dict(pid=20, creation_time=200))
                observer = Mock(identities=identities, action_owner=identities['owner_identity'])
                observer._mouse_holds = {}
                observer.user.GetAsyncKeyState.return_value = 0
                observer.desktop.return_value = dict(occlusion=[], foreground=1)
                observer.bindings.return_value = dict(ThumbMouseButton1='Game_TurnLeft', MiddleMouseButton='Game_MoveBackward')
                observer.pulse.return_value = dict(release_posted=True)
                calls = []

                def state():
                    calls.append(len(calls))
                    if len(calls) == 4:
                        (report.directory / 'stop').touch()
                    return sample(len(calls))

                observer.sample.side_effect = state
                if not explicit_hold:
                    atomic_json(report.directory / 'control-request.json',
                                dict(schema=1, sequence=1, action='resume', **identities))
                with patch('scenarios.checkpoint') as checkpoint:
                    observe(report, observer, 3, explore=True, hold_on_stall=explicit_hold)
                    checkpoint.assert_not_called()
                status = json.loads((report.directory / 'control-status.json').read_text())
                self.assertTrue(status['held'])
                self.assertIn('Navigation control stalled', status['reason'])
                self.assertEqual(observer.pulse.call_count, 1)

    def test_owned_target_override_and_context_invalidation_remain_held(self):
        from core import Evidence
        from run import observe
        for action, invalidate in (('target', False), ('target', True), ('auto', False)):
            with self.subTest(action=action, invalidate=invalidate), tempfile.TemporaryDirectory() as directory:
                report = Evidence(Path(directory) / 'run', mode='run', provenance={})
                identities = dict(game_identity=dict(pid=10, creation_time=100),
                                  owner_identity=dict(pid=20, creation_time=200))
                observer = Mock(identities=identities, action_owner=identities['owner_identity'])
                observer._mouse_holds = {}
                observer.user.GetAsyncKeyState.return_value = 0
                calls = []

                def state():
                    number = len(calls)
                    calls.append(number)
                    current = sample(number, world=2 if number == 2 else 1)
                    if number == 0:
                        atomic_json(report.directory / 'control-request.json',
                                    dict(schema=1, sequence=1, action='hold', **identities))
                    elif number == 1:
                        atomic_json(report.directory / 'control-request.json',
                            dict(schema=1, sequence=2, action=action, target=[300, 0, 0], kind='walk',
                                 pawn_identity=current['pawn_identity'], world_generation=1, **identities))
                    if number == (2 if invalidate else 1):
                        (report.directory / 'stop').touch()
                    return current

                observer.sample.side_effect = state
                observe(report, observer, 3, explore=True, hold_on_stall=True)
                rows = [json.loads(line) for line in (report.directory / 'events.jsonl').read_text().splitlines()]
                overrides = [row for row in rows if row['kind'] == 'navigation_target_override']
                self.assertEqual(overrides[0]['target'], [300, 0, 0] if action == 'target' else None)
                self.assertEqual(overrides[0]['target_kind'], 'walk' if action == 'target' else None)
                self.assertEqual(overrides[0]['automatic_goals'], action == 'auto')
                if invalidate:
                    self.assertIsNone(overrides[1]['target'])
                status = json.loads((report.directory / 'control-status.json').read_text())
                self.assertTrue(status['held'])
                self.assertEqual(status['awaiting_target'], invalidate)
                observer.pulse.assert_not_called()
                observer.bindings.assert_not_called()


class ControlTests(unittest.TestCase):
    def test_same_pawn_world_reposition_invalidates_an_explicit_target(self):
        with tempfile.TemporaryDirectory() as d:
            o, report, c, s = self.fixture(d)
            atomic_json(c.path, self.request(o, s, 1))
            c.poll(s)
            moved = copy.deepcopy(s)
            moved['pose']['position'] = [4000, 0, 0]
            self.assertEqual(c.poll(moved)['action'], 'invalidate')
            self.assertTrue(c.held)
            self.assertTrue(c.awaiting_target)
            self.assertIsNone(c.target_context)

    def test_automatic_reposition_replans_without_resuming_an_existing_hold(self):
        for held in (False, True):
            with self.subTest(held=held), tempfile.TemporaryDirectory() as d:
                o, report, c, s = self.fixture(d)
                c.automatic = True
                c.held = held
                self.assertIsNone(c.poll(s))
                failed = copy.deepcopy(s)
                failed['errors'] = [dict(component='pose')]
                failed['pose']['position'] = [4000, 0, 0]
                self.assertIsNone(c.poll(failed))
                moved = copy.deepcopy(s)
                moved['pose']['position'] = [4000, 0, 0]
                self.assertEqual(c.poll(moved)['action'], 'reposition')
                self.assertEqual(c.held, held)
                self.assertIsNone(c.poll(moved))

    def fixture(self, directory):
        identities = dict(game_identity=dict(pid=10, creation_time=100), owner_identity=dict(pid=20, creation_time=200))
        o = Mock(identities=identities, action_owner=identities['owner_identity'])
        o._mouse_holds = {}
        report = Mock(directory=Path(directory))
        return o, report, RunControl(o, report), sample(0)

    def request(self, o, state, sequence, action='target'):
        row = dict(schema=1, sequence=sequence, action=action, **o.identities)
        if action in ('target', 'auto'):
            row.update(pawn_identity=state['pawn_identity'], world_generation=1)
        if action == 'target':
            row.update(target=[300, 0, 0], kind='walk')
        return row

    def test_auto_mode_requires_current_context_and_discards_the_explicit_target_context(self):
        with tempfile.TemporaryDirectory() as d:
            o, _, c, s = self.fixture(d)
            c.hold(s, 'inspection', awaiting_target=True)
            c.target_context = (s['pawn_identity'], 1)
            row = self.request(o, s, 1, 'auto')
            bad = dict(row, world_generation=2)
            with self.assertRaises(ValueError):
                validate(bad, o.identities, s, 0)
            atomic_json(c.path, row)
            self.assertEqual(c.poll(s)['action'], 'auto')
            self.assertTrue(c.held)
            self.assertFalse(c.awaiting_target)
            self.assertIsNone(c.target_context)
            fresh = copy.deepcopy(s);fresh['native']['world_generation'] = 2
            self.assertIsNone(c.poll(fresh))

    def test_target_rejects_foreign_or_stale_context_and_unbounded_coordinates(self):
        with tempfile.TemporaryDirectory() as d:
            o, _, _, s = self.fixture(d)
            valid = self.request(o, s, 1)
            self.assertTrue(validate(valid, o.identities, s, 0))
            for field, value in [('game_identity', dict(pid=11)), ('world_generation', 2),
                                 ('pawn_identity', dict(address=99)), ('target', [float('nan'), 0, 0]),
                                 ('target', [40000, 0, 0]), ('sequence', True)]:
                bad = copy.deepcopy(valid);bad[field] = value
                with self.assertRaises(ValueError):
                    validate(bad, o.identities, s, 0)
            self.assertFalse(validate(valid, o.identities, dict(s, special_move_raw=44), 0))

    def test_hold_target_and_resume_have_separate_acknowledgements(self):
        with tempfile.TemporaryDirectory() as d:
            o, report, c, s = self.fixture(d)
            atomic_json(c.path, self.request(o, s, 1, 'hold'));c.poll(s)
            self.assertTrue(c.held)
            atomic_json(c.path, self.request(o, s, 2));c.poll(s)
            self.assertTrue(c.held)
            atomic_json(c.path, self.request(o, s, 3, 'resume'));c.poll(s)
            self.assertFalse(c.held)
            status = json.loads((Path(d) / 'control-status.json').read_text(encoding='utf8'))
            self.assertEqual(status['last_sequence'], 3)
            self.assertTrue(status['owned_call_completed'])

    def test_arrival_requires_a_new_target_before_resume(self):
        with tempfile.TemporaryDirectory() as d:
            o, report, c, s = self.fixture(d)
            c.hold(s, 'Walk target arrived', awaiting_target=True)
            atomic_json(c.path, self.request(o, s, 1, 'resume'))
            self.assertIsNone(c.poll(s));self.assertTrue(c.held)
            atomic_json(c.path, self.request(o, s, 1));self.assertIsNotNone(c.poll(s))
            atomic_json(c.path, self.request(o, s, 2, 'resume'));c.poll(s)
            self.assertFalse(c.held)

    def test_changed_possession_inhibits_old_target(self):
        with tempfile.TemporaryDirectory() as d:
            o, report, c, s = self.fixture(d)
            atomic_json(c.path, self.request(o, s, 1));c.poll(s)
            changed = dict(s, pawn_identity=dict(address=2000, internal_index=5))
            self.assertEqual(c.poll(changed)['action'], 'invalidate')
            self.assertTrue(c.held);self.assertTrue(c.awaiting_target)

    def test_passive_observer_or_malformed_request_never_accepts_control(self):
        with tempfile.TemporaryDirectory() as d:
            o, report, c, s = self.fixture(d)
            o.action_owner = None
            atomic_json(c.path, self.request(o, s, 1, 'hold'));self.assertIsNone(c.poll(s))
            self.assertFalse(c.held)
            o.action_owner = o.identities['owner_identity']
            atomic_json(c.path, ['not an object']);self.assertIsNone(c.poll(s))
            self.assertFalse(c.held)
