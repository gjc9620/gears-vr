import unittest
from unittest.mock import Mock, patch

from live import Observer


class ManualPreviewTests(unittest.TestCase):
    def observer(self, enabled=True):
        o = Observer.__new__(Observer)
        o.identities = dict(game_identity=dict(pid=10), owner_identity=dict(pid=20),
                            xr_identity=dict(pid=30, creation_time=300, image='owned-xr'))
        o.allow_manual_preview_activation = enabled
        o.manual_preview_windows = set()
        o.backend = Mock()
        o.user = Mock()
        o.user.GetForegroundWindow.return_value = 3
        def window_class(hwnd, buffer, length):
            buffer.value = 'OpenXR Simulator'
            return len(buffer.value)
        o.user.GetClassNameW.side_effect = window_class
        o.windows = Mock()
        o.windows.capture.return_value = dict(windows=[dict(hwnd=3, pid=30)],
            excluded_hwnds=[], traversed_windows=2)
        return o

    @patch('live.launcher.instance_policy.background_process_pids', return_value=set())
    @patch('live.find_occlusion', return_value=[dict(test_hwnd=3, covered_hwnd=9)])
    def test_authorized_preview_does_not_stop_play_after_manual_activation_or_switching_away(self, hits, pids):
        o = self.observer()
        first = o.desktop()
        self.assertFalse(first['occlusion'])
        self.assertEqual(first['user_activated_preview_occlusion'], hits.return_value)
        o.backend.runner.checked_identity.assert_called_with(o.backend.control, o.identities['xr_identity'])
        o.user.GetForegroundWindow.return_value = 9
        self.assertFalse(o.desktop()['occlusion'])
        self.assertEqual(o.manual_preview_windows, {3})

    @patch('live.launcher.instance_policy.background_process_pids', return_value=set())
    @patch('live.find_occlusion', return_value=[dict(test_hwnd=3, covered_hwnd=9)])
    def test_without_authorization_preview_occlusion_stays_blocking(self, hits, pids):
        o = self.observer(False)
        self.assertEqual(o.desktop()['occlusion'], hits.return_value)

    @patch('live.launcher.instance_policy.background_process_pids', return_value=set())
    @patch('live.find_occlusion', return_value=[dict(test_hwnd=3, covered_hwnd=9)])
    def test_other_process_or_other_window_class_never_gets_preview_exception(self, hits, pids):
        for pid, name in [(10, 'OpenXR Simulator'), (40, 'OpenXR Simulator'), (30, 'Other dialog')]:
            with self.subTest(pid=pid, name=name):
                o = self.observer()
                o.windows.capture.return_value['windows'][0]['pid'] = pid
                def window_class(hwnd, buffer, length):
                    buffer.value = name
                    return len(name)
                o.user.GetClassNameW.side_effect = window_class
                self.assertEqual(o.desktop()['occlusion'], hits.return_value)

    @patch('live.launcher.instance_policy.background_process_pids', return_value=set())
    @patch('live.find_occlusion', return_value=[dict(test_hwnd=3, covered_hwnd=9)])
    def test_xr_identity_change_rejects_exception_and_reused_window_loses_approval(self, hits, pids):
        o = self.observer()
        o.desktop()
        def other_class(hwnd, buffer, length):
            buffer.value = 'Other dialog'
            return len(buffer.value)
        o.user.GetClassNameW.side_effect = other_class
        self.assertEqual(o.desktop()['occlusion'], hits.return_value)
        self.assertFalse(o.manual_preview_windows)
        o.windows.capture.return_value['windows'][0]['pid'] = 40
        self.assertEqual(o.desktop()['occlusion'], hits.return_value)
        o.backend.runner.checked_identity.side_effect = RuntimeError('PID reused')
        with self.assertRaisesRegex(RuntimeError, 'PID reused'):
            o.desktop()
