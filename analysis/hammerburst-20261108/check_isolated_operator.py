"""Offline routing/identity checks. Starts no game, XR, recorder or input."""
from pathlib import Path
import hashlib
import json
import tempfile
import unittest
from unittest.mock import patch
import hammerburst_isolated_operator as adapter


class BindingTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.patch = patch.object(adapter, 'ROOT', self.root)
        self.patch.start()
        self.addCleanup(self.patch.stop)
        self.addCleanup(self.temp.cleanup)
        self.instance = self.root / 'analysis/instances/hb-test'
        self.instance.mkdir(parents=True)
        self.exe = self.root / 'tools/openxr-probe/build-hb-test/OpenXRPoseProbe.exe'
        self.exe.parent.mkdir(parents=True)
        self.exe.write_bytes(b'frozen fixture, not executable')
        self.owner = dict(pid=1, creation_time=100, image=str(self.root / 'python.exe'))
        self.game = dict(pid=2, creation_time=200, image=str(self.root / 'game.exe'))
        self.xr = dict(pid=3, creation_time=300, image=str(self.exe))
        self.identities = {v['pid']: v for v in (self.owner, self.game, self.xr)}
        self.control = type('Control', (), {'identity': lambda _, pid: self.identities.get(pid)})()
        self.state = dict(instance_root=str(self.instance), owner_identity=self.owner,
                          game_identity=self.game, vr_gamepad={'source': 1},
                          session_report=str(self.instance / 'report.json'))
        self.write('launcher-session.json', self.state)
        self.write('report.json', {'xr_identity': self.xr})
        self.write('manifest.json', dict(xr_build=str(self.exe.parent.relative_to(self.root)),
                                         xr_sha256=hashlib.sha256(self.exe.read_bytes()).hexdigest()))
        self.bound = adapter.Binding('hb-test', self.instance / 'actions', self.control)

    def write(self, name, value):
        (self.instance / name).write_text(json.dumps(value))

    def test_exact_owned_session(self):
        self.assertEqual(self.bound.state()['game_identity'], self.game)
        self.assertTrue(self.bound.live(self.game))

    def test_same_pid_new_lifetime_rejected(self):
        for identity in (self.owner, self.game, self.xr):
            with self.subTest(pid=identity['pid']):
                self.identities[identity['pid']] = dict(identity, creation_time=999)
                with self.assertRaises(RuntimeError):
                    self.bound.state()
                self.identities[identity['pid']] = identity

    def test_foreign_instance_receipt_rejected(self):
        self.write('launcher-session.json', dict(self.state, instance_root=str(self.root / 'foreign')))
        with self.assertRaises(RuntimeError):
            self.bound.state()

    def test_foreign_xr_even_when_live_rejected(self):
        foreign = dict(self.xr, image=str(self.root / 'foreign/OpenXRPoseProbe.exe'))
        self.identities[3] = foreign
        self.write('report.json', {'xr_identity': foreign})
        with self.assertRaises(RuntimeError):
            self.bound.state()

    def test_changed_artifact_rejected(self):
        self.exe.write_bytes(b'changed')
        with self.assertRaises(RuntimeError):
            self.bound.state()

    def test_physical_source_rejected(self):
        self.write('launcher-session.json', dict(self.state, vr_gamepad={'source': 2}))
        with self.assertRaises(RuntimeError):
            self.bound.state()

    def test_live_check_pins_lifetimes_without_file_io(self):
        self.bound.state()
        with patch.object(Path, 'read_text', side_effect=AssertionError('Unexpected per-command disk read')):
            self.assertTrue(self.bound.live(self.game))
            self.identities[3] = dict(self.xr, creation_time=301)
            self.assertFalse(self.bound.live(self.game))

    def test_own_stop_marker_halts_inputs_before_game_shutdown(self):
        self.bound.state()
        (self.instance / 'stop-actions').write_text('Normal own stop requested')
        self.assertFalse(self.bound.live(self.game))

    def test_output_cannot_escape_instance(self):
        with self.assertRaises(ValueError):
            adapter.Binding('hb-test', self.root / 'shared')
        with self.assertRaises(ValueError):
            adapter.Binding('hb-test', self.instance / '../foreign')

    def test_helper_cannot_dispatch_arbitrary_script(self):
        with self.assertRaises(ValueError):
            self.bound.helper('other.py', 'out.json')
        with self.assertRaises(ValueError):
            self.bound.helper('read_world_grips.py', '../foreign.json')

    def test_stop_is_exact_instance_without_shared_marker(self):
        with patch.object(adapter.subprocess, 'run') as run:
            self.bound.stop()
        command = run.call_args.args[0]
        self.assertEqual(command[-3:], ['stop', '--name', 'hb-test'])
        self.assertTrue(command[-4].endswith('isolated_sim.py'))
        self.assertFalse((self.root / 'analysis/solo-m1-20261028/stop-basic').exists())

    def test_stop_rejects_foreign_owner_lifetime(self):
        self.identities[1] = dict(self.owner, creation_time=101)
        with patch.object(adapter.subprocess, 'run') as run:
            with self.assertRaises(RuntimeError):
                self.bound.stop()
            run.assert_not_called()


class SourceTests(unittest.TestCase):
    def test_all_existing_action_and_reader_sources_compile(self):
        self.assertEqual(len(adapter.check_sources()), 6)

    def test_changed_source_anchor_stops_adaptation(self):
        with self.assertRaises(ValueError):
            adapter.replace_once('no matching anchor', 'old source', 'new source')

    def test_name_cannot_select_shared_instance(self):
        for name in ('solo-m1-1028d', '../hb-test', 'hb-test/../foreign'):
            with self.subTest(name=name), self.assertRaises(ValueError):
                adapter.instance_root(name)


if __name__ == '__main__':
    unittest.main(verbosity=2)
