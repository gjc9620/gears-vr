import contextlib
import io
import json
import os
import sys
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import armspan_calibration as calibration
import armspan_startup as startup
import armspan_wire as wire
from test_armspan_calibration import VALID, sample
from test_launcher import launcher, FakeBackend, MANIFEST


class StartupTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.path = self.root/'profile/calibration.json'
        self.args = SimpleNamespace(xr_build_dir=self.root/'xr', startup_menu_window=99,
                                    remeasure_armspan=False, cleanup_timeout=2)
        self.game = dict(pid=10, creation_time=20, image='game.exe')
        self.owner = dict(pid=30, creation_time=40, image='owner.exe')
        self.publisher = dict(pid=50, creation_time=60, image=str((self.args.xr_build_dir/'OpenXRPoseProbe.exe').resolve()))
        self.control = SimpleNamespace(identity=lambda pid: {10:self.game, 30:self.owner, 50:self.publisher}.get(pid))
        self.child = SimpleNamespace(pid=50, poll=mock.Mock(return_value=None))
        self.ticks, self.order = [1.], []
        self.runner = SimpleNamespace(xr_environment=lambda env:dict(env), cleanup_owned_xr=self.cleanup)
        self.reader = SimpleNamespace(read=self.read_sample, now=lambda:int(self.ticks[0]*1e9),
                                      close=lambda:self.order.append('reader_closed'))
        self.reader_factory = mock.Mock(return_value=self.reader)
        self.popen = mock.Mock(return_value=self.child)
        self.ui_updates = []
        self.ui = SimpleNamespace(token='12'*16, publish=self.publish_ui,
                                  close=lambda:self.order.append('ui_closed'))
        self.ui_factory = mock.Mock(return_value=self.ui)
        self.wait_presented = mock.Mock(side_effect=self.present_saved)
        self.number = 0
        self.environment = mock.patch.dict(os.environ, GEARSVR_TEST_WINDOWS_BEHIND='')
        self.environment.start()
        self.addCleanup(self.environment.stop)

    def sleep(self, amount):
        self.ticks[0] += amount

    def read_sample(self):
        self.number += 1
        return sample(self.number, int(self.ticks[0]*1e9))

    def publish_ui(self, phase, progress):
        self.ui_updates.append((phase, progress))
        return len(self.ui_updates)

    def present_saved(self, path, token, sequence, child, owner_alive, cancel, ui, **kwargs):
        self.assertEqual(calibration.load(self.path).state, 'valid')
        self.assertEqual(self.ui_updates[-1], ('saved', 1000))
        self.assertNotIn('ui_closed', self.order)
        self.assertEqual((token, child, ui), (self.ui.token, self.child, self.ui))
        self.assertTrue(owner_alive())
        return dict(armspan_ui_presented=True, token=token, phase='saved', sequence=sequence, end_frame_succeeded=True)

    def cleanup(self, child, pid, game_pid, timeout, control, receipt):
        self.assertEqual((child, pid, game_pid, timeout, control), (self.child, 50, 10, 2, self.control))
        self.assertEqual(self.order, ['reader_closed'])
        self.order.append('child_waited')
        receipt.update(xr_exit_code=0)

    def prepare(self, **kwargs):
        with contextlib.redirect_stdout(io.StringIO()):
            return startup.prepare(self.args, self.game, self.owner, self.control, self.runner,
                kwargs.pop('cancel', lambda:False), self.root, path=self.path,
                reader_factory=self.reader_factory, ui_factory=self.ui_factory,
                wait_presented=self.wait_presented, popen=self.popen, clock=lambda:self.ticks[0],
                sleep=self.sleep, timeout=kwargs.pop('timeout', 3), **kwargs)

    def test_first_run_saves_before_child_cleanup_and_returns_only_after_wait(self):
        def cleanup(*args):
            self.assertEqual(calibration.load(self.path).state, 'valid')
            self.cleanup(*args)
        self.runner.cleanup_owned_xr = cleanup
        receipt = self.prepare()
        self.assertEqual(receipt['mode'], 'measured')
        self.assertTrue(receipt['saved'])
        self.assertEqual(self.order, ['reader_closed', 'child_waited', 'ui_closed'])
        command = self.popen.call_args.args[0]
        self.assertIn('--controller-input', command)
        self.assertIn('--startup-menu-window', command)
        self.assertEqual(command[command.index('--armspan-ui')+1], self.ui.token)
        self.assertEqual(command[command.index('--armspan-game-creation')+1], '20')
        self.assertTrue(receipt['headset_feedback']['end_frame_succeeded'])
        self.wait_presented.assert_called_once()
        self.assertNotIn('--allow-simulated', command)
        self.assertAlmostEqual(calibration.load(self.path).value['armspan_metres'], 1.7)

    def test_existing_valid_record_reuses_without_starting_xr(self):
        calibration.save(self.path, VALID, calibration.load(self.path))
        before = self.path.read_bytes()
        receipt = self.prepare()
        self.assertEqual(receipt['mode'], 'reused')
        self.popen.assert_not_called()
        self.reader_factory.assert_not_called()
        self.ui_factory.assert_not_called()
        self.assertEqual(self.path.read_bytes(), before)

    def test_missing_or_bad_record_cannot_pass_without_a_completed_hold(self):
        for before in (None, b'broken'):
            with self.subTest(before=before):
                self.order.clear()
                self.ticks[0] = 1.
                if before is not None:
                    self.path.parent.mkdir(exist_ok=True)
                    self.path.write_bytes(before)
                self.reader.read = mock.Mock(side_effect=FileNotFoundError('not published'))
                with self.assertRaises(TimeoutError):
                    self.prepare(timeout=.03)
                self.assertEqual(self.order, ['reader_closed', 'child_waited', 'ui_closed'])
                self.assertEqual(self.path.read_bytes() if self.path.exists() else None, before)

    def test_remeasure_cancel_or_tracking_timeout_preserves_old_record_and_cleans_child(self):
        calibration.save(self.path, VALID, calibration.load(self.path))
        before = self.path.read_bytes()
        self.args.remeasure_armspan = True
        for outcome in ('cancel', 'tracking_lost', 'early_exit', 'contract'):
            with self.subTest(outcome=outcome):
                self.order.clear()
                self.ticks[0] = 1.
                self.child.poll.return_value = 5 if outcome == 'early_exit' else None
                self.reader.read = mock.Mock(side_effect=wire.ContractError('version') if outcome == 'contract'
                                            else wire.Unavailable('lost tracking'))
                if outcome == 'cancel':
                    self.assertIsNone(self.prepare(cancel=lambda:True))
                else:
                    error = wire.ContractError if outcome == 'contract' else RuntimeError if outcome == 'early_exit' else TimeoutError
                    with self.assertRaises(error):
                        self.prepare(timeout=.03)
                self.assertEqual(self.path.read_bytes(), before)
                self.assertEqual(self.order, ['reader_closed', 'child_waited', 'ui_closed'])

    def test_failed_identity_or_reader_setup_always_waits_own_child(self):
        for failure in ('publisher', 'reader'):
            with self.subTest(failure=failure):
                self.order.clear()
                self.publisher['image'] = 'wrong.exe' if failure == 'publisher' else str((self.args.xr_build_dir/'OpenXRPoseProbe.exe').resolve())
                self.reader_factory.side_effect = RuntimeError('reader setup')
                self.runner.cleanup_owned_xr = lambda *args: (self.order.append('child_waited'), args[-1].update(xr_exit_code=0))
                with self.assertRaises(RuntimeError):
                    self.prepare()
                self.assertEqual(self.order, ['child_waited', 'ui_closed'])
                self.assertFalse(self.path.exists())

    def test_reader_close_failure_still_waits_child_and_blocks_activation(self):
        self.reader.close = mock.Mock(side_effect=RuntimeError('reader close'))
        self.runner.cleanup_owned_xr = lambda *args: (self.order.append('child_waited'), args[-1].update(xr_exit_code=0))
        with self.assertRaisesRegex(RuntimeError, 'reader close'):
            self.prepare(cancel=lambda:True)
        self.assertEqual(self.order, ['child_waited', 'ui_closed'])

    def test_forced_or_failed_xr_exit_blocks_activation_even_after_saving(self):
        for receipt in (dict(xr_exit_code=1), dict(xr_exit_code=0, xr_forced_cleanup_reason='timeout'),
                        dict(xr_exit_code=0, xr_cleanup_error='fixture')):
            with self.subTest(receipt=receipt):
                self.ticks[0], self.number = 1., 0
                self.args.remeasure_armspan = True
                self.order.clear()
                self.runner.cleanup_owned_xr = lambda *args: args[-1].update(receipt)
                with self.assertRaisesRegex(RuntimeError, 'did not close cleanly'):
                    self.prepare()
                self.assertEqual(self.order, ['reader_closed', 'ui_closed'])
                self.assertEqual(calibration.load(self.path).state, 'valid')

    def test_unowned_startup_and_background_measurement_refused_before_xr(self):
        with mock.patch.dict(os.environ, GEARSVR_TEST_WINDOWS_BEHIND='1'):
            with self.assertRaisesRegex(ValueError, 'autonomous'):
                self.prepare()
        self.args.startup_menu_window = 0
        with self.assertRaisesRegex(ValueError, 'owned startup menu'):
            self.prepare()
        self.args.startup_menu_window = 99
        self.control.identity = lambda pid:None
        with self.assertRaisesRegex(RuntimeError, 'identity changed'):
            self.prepare()
        self.popen.assert_not_called()
        self.ui_factory.assert_not_called()

    def test_ui_creation_and_child_start_failure_cannot_skip_cleanup(self):
        self.ui_factory.side_effect = RuntimeError('ui create')
        with self.assertRaisesRegex(RuntimeError, 'ui create'):
            self.prepare()
        self.popen.assert_not_called()
        self.assertEqual(self.order, [])
        self.ui_factory.side_effect = None
        self.popen.side_effect = OSError('child start')
        with self.assertRaisesRegex(OSError, 'child start'):
            self.prepare()
        self.assertEqual(self.order, ['ui_closed'])
        self.assertEqual(self.ui_updates[-1], ('failed', 0))

    def test_missing_saved_card_feedback_blocks_handoff_without_removing_saved_record(self):
        self.wait_presented.side_effect = TimeoutError('feedback')
        with self.assertRaisesRegex(TimeoutError, 'feedback'):
            self.prepare()
        self.assertEqual(calibration.load(self.path).state, 'valid')
        self.assertEqual(self.order, ['reader_closed', 'child_waited', 'ui_closed'])

    def test_cancel_during_saved_feedback_preserves_completed_record(self):
        self.wait_presented.side_effect = None
        self.wait_presented.return_value = None
        self.assertIsNone(self.prepare())
        self.assertEqual(calibration.load(self.path).state, 'valid')
        self.assertEqual(self.ui_updates[-1], ('cancelled', 0))
        self.assertEqual(self.order, ['reader_closed', 'child_waited', 'ui_closed'])

    def test_launcher_language_reaches_owned_ui(self):
        self.args.launcher_language = 'zh-CN'
        self.prepare()
        self.ui_factory.assert_called_once_with(self.game, self.owner, 'zh-CN')


class LauncherGateTests(unittest.TestCase):
    def candidate(self):
        return dict(MANIFEST, startup_menu=True, controller_input_observer=True,
                    anchor_observer=True, ui_observer=True, crosshair_assignment_observer=True,
                    armspan_startup_calibration=True)

    def test_candidate_capability_requires_boolean_controller_and_owned_startup(self):
        launcher.settings({}, self.candidate())
        for key in ('controller_input_observer', 'startup_menu'):
            with self.subTest(key=key), self.assertRaisesRegex(ValueError, 'Armspan startup'):
                launcher.settings({}, dict(self.candidate(), **{key:False}))
        for value in ('true', 1, None):
            with self.subTest(value=value), self.assertRaisesRegex(ValueError, 'boolean'):
                launcher.settings({}, dict(self.candidate(), armspan_startup_calibration=value))

    def test_failed_or_cancelled_calibration_never_calls_native_runner(self):
        for outcome in ('missing', 'invalid', 'cancelled'):
            with self.subTest(outcome=outcome), tempfile.TemporaryDirectory() as directory:
                backend = FakeBackend()
                backend.calibrate_armspan = mock.Mock(return_value=None) if outcome == 'cancelled' else mock.Mock(side_effect=TimeoutError(outcome))
                with mock.patch.object(launcher, 'verify_files', return_value={}), \
                     mock.patch.object(launcher.ctypes, 'get_last_error', return_value=0), \
                     mock.patch('game_audio_guard.begin_manual', return_value=None):
                    index = Path(directory)/'index.json'
                    if outcome == 'cancelled':
                        self.assertEqual(launcher.start({}, self.candidate(), backend, index, sleep=lambda _:None), 0)
                        state = json.loads(index.read_text())
                        self.assertEqual(state['phase'], 'cancelled_before_activation')
                        self.assertFalse(state['native_activated'])
                    else:
                        with self.assertRaises(TimeoutError):
                            launcher.start({}, self.candidate(), backend, index, sleep=lambda _:None)
                backend.runner.run_session.assert_not_called()

    def test_saved_record_is_passed_to_native_runner_before_command_is_frozen(self):
        with tempfile.TemporaryDirectory() as directory:
            backend = FakeBackend()
            path = Path(directory)/'record.json'
            calibration.save(path, VALID, calibration.load(path))
            backend.calibrate_armspan = mock.Mock(return_value=dict(path=str(path), mode='measured', saved=True))
            with mock.patch.object(launcher, 'verify_files', return_value={}), \
                 mock.patch.object(launcher.ctypes, 'get_last_error', return_value=0), \
                 mock.patch('game_audio_guard.begin_manual', return_value=None):
                launcher.start({}, self.candidate(), backend, Path(directory)/'index.json', sleep=lambda _:None, remeasure_armspan=True)
            calibration_args = backend.calibrate_armspan.call_args.args[0]
            args = backend.runner.run_session.call_args.args[0]
            self.assertIs(args, calibration_args)
            self.assertFalse(args.remeasure_armspan)
            self.assertEqual(args.weapon_calibration_path, path)
            sys.path.insert(0, str(launcher.ROOT/'tools/native-capture'))
            import run_stereo_continuous as runner
            args.armspan_calibration = runner.weapon_calibration.load(args.weapon_calibration_path)
            command = runner.native_command(args, backend.owner)
            self.assertEqual(command[command.index('--armspan-metres')+1], '1.7')
            self.assertEqual(command[command.index('--hands-below-head-metres')+1], '-0.15')

    def test_record_disappearing_before_handoff_cannot_start_runner(self):
        with tempfile.TemporaryDirectory() as directory:
            backend = FakeBackend()
            backend.calibrate_armspan = mock.Mock(return_value=dict(path=str(Path(directory)/'missing.json'), mode='measured', saved=True))
            with mock.patch.object(launcher, 'verify_files', return_value={}), \
                 mock.patch.object(launcher.ctypes, 'get_last_error', return_value=0), \
                 mock.patch('game_audio_guard.begin_manual', return_value=None):
                with self.assertRaisesRegex(RuntimeError, 'disappeared'):
                    launcher.start({}, self.candidate(), backend, Path(directory)/'index.json', sleep=lambda _:None)
            backend.runner.run_session.assert_not_called()

    def test_native_frozen_command_must_retain_the_same_valid_record(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'record.json'
            calibration.save(path, VALID, calibration.load(path))
            for frozen in (None, dict(VALID, armspan_metres=1.9), VALID):
                with self.subTest(frozen=frozen):
                    backend = FakeBackend()
                    backend.calibrate_armspan = mock.Mock(return_value=dict(path=str(path), mode='reused', saved=False))
                    def run(args, *unused, **callbacks):
                        args.armspan_calibration = frozen
                        callbacks['before_native_activation'](lambda:None)
                        return 0
                    backend.runner.run_session.side_effect = run
                    with mock.patch.object(launcher, 'verify_files', return_value={}), \
                         mock.patch.object(launcher.ctypes, 'get_last_error', return_value=0), \
                         mock.patch('game_audio_guard.begin_manual', return_value=None):
                        if frozen != VALID:
                            with self.assertRaisesRegex(RuntimeError, 'Native remains inactive'):
                                launcher.start({}, self.candidate(), backend, Path(directory)/'index.json', sleep=lambda _:None)
                        else:
                            self.assertEqual(launcher.start({}, self.candidate(), backend, Path(directory)/'index.json', sleep=lambda _:None), 0)

    def test_default_manifest_does_not_start_calibration(self):
        with tempfile.TemporaryDirectory() as directory:
            backend = FakeBackend()
            backend.calibrate_armspan = mock.Mock(side_effect=AssertionError('default changed'))
            with mock.patch.object(launcher, 'verify_files', return_value={}), \
                 mock.patch.object(launcher.ctypes, 'get_last_error', return_value=0), \
                 mock.patch('game_audio_guard.begin_manual', return_value=None):
                launcher.start({}, MANIFEST, backend, Path(directory)/'index.json', sleep=lambda _:None)
            backend.calibrate_armspan.assert_not_called()
            self.assertFalse(hasattr(backend.runner.run_session.call_args.args[0], 'weapon_calibration_path'))


if __name__ == '__main__':
    unittest.main()
