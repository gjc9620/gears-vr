"""Focused startup-order checks; no game, XR session or desktop automation."""
import json
import pathlib
import tempfile
import unittest
from unittest import mock

from test_launcher import FakeBackend, MANIFEST, launcher


class PlaygroundStartupTests(unittest.TestCase):
    def exercise(self, scenario):
        with tempfile.TemporaryDirectory() as directory:
            index = pathlib.Path(directory)/'session.json'
            backend = FakeBackend()
            order = []
            def start_pg(manifest, identity, check):
                order.append('playground')
                if scenario == 'cancel':
                    backend.cancel = True
                check()
                return dict(enabled=True)
            backend.start_playground = mock.Mock(side_effect=start_pg)
            backend.finish_playground = mock.Mock(side_effect=lambda: order.append('cleanup') or dict(passed=True))
            backend.require_prime_active = mock.Mock()
            if scenario == 'prime_reject':
                backend.require_prime_active.side_effect = RuntimeError('prime lost')
            def run(args, control, **kwargs):
                order.append('xr')
                self.assertEqual(backend.start_playground.call_count, 1)
                if scenario == 'xr_reject':
                    raise RuntimeError('XR first frame failed')
                kwargs['before_native_activation'](lambda: None)
                self.assertEqual(backend.start_playground.call_count, 1)
                return 0
            backend.runner.run_session.side_effect = run
            manifest = dict(MANIFEST, startup_menu=True, playground_menu=True)
            if scenario == 'prime_reject':
                manifest['graphics_startup_lock'] = True
            import game_audio_guard
            with mock.patch.object(launcher, 'verify_files', return_value={}), \
                 mock.patch.object(launcher.ctypes, 'get_last_error', return_value=0), \
                 mock.patch.object(game_audio_guard, 'begin_manual', return_value=None):
                if scenario in ('xr_reject', 'prime_reject'):
                    with self.assertRaises(RuntimeError):
                        launcher.start({}, manifest, backend, index, sleep=lambda _: None)
                else:
                    self.assertEqual(launcher.start({}, manifest, backend, index, sleep=lambda _: None), 0)
            backend.finish_playground.assert_called_once()
            report = json.loads(index.read_text())
            self.assertTrue(report['prime_cleanup']['entry_restore_verified'])
            self.assertEqual(order[-1], 'cleanup')
            if scenario == 'normal':
                self.assertEqual(order, ['playground', 'xr', 'cleanup'])
                self.assertTrue(report['playground_before_xr_startup'])
            if scenario in ('prime_reject', 'cancel'):
                backend.runner.run_session.assert_not_called()
            if scenario == 'prime_reject':
                backend.start_playground.assert_not_called()

    def test_menu_before_xr_and_once(self): self.exercise('normal')
    def test_xr_failure_cleans_playground(self): self.exercise('xr_reject')
    def test_prime_rejection_prevents_enable(self): self.exercise('prime_reject')
    def test_cancel_before_xr_cleans_playground(self): self.exercise('cancel')


if __name__ == '__main__':
    unittest.main()
