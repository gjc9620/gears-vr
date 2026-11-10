"""Empty-UI admission only; no game process, engine call or input authority."""
import unittest
import json
from pathlib import Path
import sys
import tempfile
import types
from unittest.mock import patch
import cover_playground_observation as observation
from cover_playground_observation import empty_playground


class EmptyPlaygroundAdmission(unittest.TestCase):
    def setUp(self):
        self.good = dict(consistent_reread=True, read_only=True, input_block_count=0,
                         top=None, active_scenes=[])

    def test_certified_empty_menu(self):
        self.assertTrue(empty_playground(self.good))

    def test_unknown_or_incomplete_menu_rejected(self):
        for key in self.good:
            with self.subTest(missing=key):
                candidate = dict(self.good)
                del candidate[key]
                self.assertFalse(empty_playground(candidate))

    def test_active_or_untrusted_menu_rejected(self):
        for changed in (dict(consistent_reread=False), dict(read_only=False),
                        dict(input_block_count=1), dict(top={}), dict(active_scenes=[{}])):
            with self.subTest(changed=changed):
                self.assertFalse(empty_playground(dict(self.good, **changed)))


class RegistryReacquisition(unittest.TestCase):
    """Real adapter path; registry/process reads are explicit Host seams."""
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        p = self.root/'tools/package-inspector/reloaded-profile.json'
        p.parent.mkdir(parents=True)
        p.write_text('{}')
        self.errors = []
        self.resolvers = []
        self.sampler_calls = 0
        self.wrong_pawn = False
        self.change_session = False
        self.session = dict(session_id='same', game_identity=dict(pid=42),
                            owner_identity=dict(pid=43), xr_identity=dict(pid=44),
                            unified_weapons=dict(hand_connected=True))
        self.menu = dict(consistent_reread=True, read_only=True, input_block_count=0,
                         top=None, active_scenes=[])
        parent = self

        class RegistryChanged(RuntimeError):
            evidence = dict(registry='objects', rejected=True)
        self.RegistryChanged = RegistryChanged

        class Resolver:
            def __init__(self, pid, profile):
                self.pid = pid
                self.closed = False
                self.p = self
                parent.resolvers.append(self)
            def close(self):
                self.closed = True

        def sampler(resolver, cinematic_camera):
            parent.sampler_calls += 1
            if parent.errors:
                error = parent.errors.pop(0)
                if error:
                    raise error
            return types.SimpleNamespace(controller=10, pawn=99 if parent.wrong_pawn else 20,
                                         fresh=lambda address: dict(address=address))

        def acquire(sampler):
            return dict(guard_before=dict(target_proof=dict(cinematic=False)), values=dict(
                pawn_location=[1, 2, 3], controller_rotation=[0, 0, 0], pawn_rotation=[0, 0, 0],
                camera_cache_pov=dict(location=[1, 2, 4], rotation_ru=[0, 0, 0], fov=80))), dict(
                    started_perf_counter_ns=100, finished_perf_counter_ns=101)

        def close_pose():
            if self.observer.resolver:
                self.observer.resolver.p.close()
            self.observer.resolver = self.observer.sampler = None
            if parent.change_session:
                parent.session = dict(parent.session, session_id='replacement')

        self.observer = types.SimpleNamespace(
            identities=dict(game_identity=dict(pid=42)), resolver=None, sampler=None,
            sample=lambda: dict(errors=[], menu=dict(self.menu), game_identity=dict(pid=42)),
            verify=lambda: self.session, close_pose=close_pose, menu=lambda: dict(self.menu),
            _byte_property=lambda pawn, name: 0,
            _int_property=lambda pawn, name: 450)
        modules = dict(anchor_config=types.SimpleNamespace(AnchorRegistryChanged=RegistryChanged,
            LiveRegistryAnchorSampler=sampler, acquire_anchor_sample=acquire),
            resolve_camera=types.SimpleNamespace(Resolver=Resolver))
        self.patch = patch.dict(sys.modules, modules)
        self.patch.start()
        self.addCleanup(self.patch.stop)

    def sample(self):
        return observation.sample(self.observer, self.root, 10, 20)

    def test_registry_failure_rebuilds_full_resolver_once(self):
        self.errors = [self.RegistryChanged('header changed'), None]
        result = self.sample()
        self.assertEqual(self.sampler_calls, 2)
        self.assertTrue(self.resolvers[0].closed)
        self.assertFalse(self.resolvers[1].closed)
        self.assertEqual(result['special_move_raw'], 0)
        self.assertEqual(result['registry_reacquisition'][0]['evidence']['registry'], 'objects')
        self.assertEqual(result['anchor_acquisition']['finished_perf_counter_ns'], 101)

    def test_second_registry_failure_stops(self):
        self.errors = [self.RegistryChanged('first'), self.RegistryChanged('second')]
        with self.assertRaises(self.RegistryChanged) as caught:
            self.sample()
        self.assertEqual(self.sampler_calls, 2)
        self.assertTrue(all(r.closed for r in self.resolvers))
        self.assertEqual(len(caught.exception.registry_reacquisition), 2)

    def test_non_registry_failure_has_no_retry(self):
        self.errors = [RuntimeError('identity rejected')]
        with self.assertRaisesRegex(RuntimeError, 'identity rejected'):
            self.sample()
        self.assertEqual(self.sampler_calls, 1)
        self.assertTrue(self.resolvers[0].closed)

    def test_session_change_prevents_second_discovery(self):
        self.errors = [self.RegistryChanged('header changed')]
        self.change_session = True
        with self.assertRaisesRegex(RuntimeError, 'session changed'):
            self.sample()
        self.assertEqual(self.sampler_calls, 1)

    def test_new_discovery_still_rejects_wrong_actor(self):
        self.errors = [self.RegistryChanged('header changed'), None]
        self.wrong_pawn = True
        with self.assertRaisesRegex(RuntimeError, 'actor differs'):
            self.sample()
        self.assertEqual(self.sampler_calls, 2)
        self.assertTrue(all(r.closed for r in self.resolvers))


if __name__ == '__main__':
    unittest.main()
