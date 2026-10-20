import ctypes as c
import unittest
from unittest.mock import Mock, patch

from live import Observer, InputNotReady, InputInterrupted, RegistrySnapshotChanged, MouseReleaseFailed
from test_core import sample


class InputTests(unittest.TestCase):
    def fixture(self):
        observer = Observer.__new__(Observer)
        observer.identities = dict(game_identity=dict(pid=10, creation_time=100),
                                   owner_identity=dict(pid=20, creation_time=200))
        observer.action_owner = observer.identities['owner_identity']
        observer.started = 0
        observer.verify = Mock()
        observer.sample = Mock(return_value=sample(0))
        observer.menu = Mock(return_value=sample(0)['menu'])
        observer.bindings = Mock(return_value={'MiddleMouseButton': 'Game_MoveBackward'})
        observer.desktop = Mock(return_value=dict(occlusion=[], foreground=1))
        observer.backend = Mock()
        observer.backend.game_window.return_value = 2
        observer.backend.control.identity.return_value = observer.identities['game_identity']
        observer.user = Mock()
        observer.user.GetForegroundWindow.return_value = 1
        observer.user.GetAsyncKeyState.return_value = 0
        def rect(hwnd, pointer):
            pointer._obj.right, pointer._obj.bottom = 1280, 720
            return True
        observer.user.GetClientRect.side_effect = rect
        observer.user.PostMessageW.return_value = True
        return observer

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_cancel_during_hold_still_posts_release(self, background):
        observer = self.fixture()
        stop = Mock(side_effect=[False, True])
        events = []
        with self.assertRaisesRegex(RuntimeError, 'during input'):
            observer.pulse('MiddleMouseButton', .1,
                           expected_bindings=observer.bindings(), stop_requested=stop,
                           emit=lambda kind, **value: events.append(kind))
        messages = [call.args[1] for call in observer.user.PostMessageW.call_args_list]
        self.assertEqual(messages, [0x207, 0x208])
        self.assertEqual(events, ['input_down', 'input_up'])

    @patch('live.background_input.change', return_value={'state': 'enabled'})
    def test_physical_background_input_rejects_before_dispatch(self, background):
        observer = self.fixture()
        with self.assertRaisesRegex(RuntimeError, 'disabled'):
            observer.pulse('MiddleMouseButton', .1, expected_bindings=observer.bindings())
        observer.user.PostMessageW.assert_not_called()

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_changed_binding_rejects_before_dispatch(self, background):
        observer = self.fixture()
        with self.assertRaisesRegex(RuntimeError, 'bindings changed'):
            observer.pulse('MiddleMouseButton', .1, expected_bindings={'MiddleMouseButton': 'Other'})
        observer.user.PostMessageW.assert_not_called()

    def test_passive_session_can_never_inject_input(self):
        observer = self.fixture()
        observer.action_owner = None
        with self.assertRaisesRegex(RuntimeError, 'created by this test'):
            observer.pulse('MiddleMouseButton', .1, expected_bindings={})
        observer.user.PostMessageW.assert_not_called()

    def chord_clock(self):
        now = [0.0]
        def sleep(seconds):
            now[0] += seconds
        return lambda: now[0], sleep

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_aim_fire_chord_preserves_aim_flags_and_releases_both_buttons(self, background):
        observer = self.fixture()
        observer.bindings.return_value = {'RightMouseButton': 'Game_Target', 'LeftMouseButton': 'Game_Shoot'}
        clock, sleep = self.chord_clock()
        def fire(row):
            return observer.pulse('LeftMouseButton', .03, expected_bindings=observer.bindings(),
                                  held_keys=('RightMouseButton',))
        with patch('live.time.monotonic', clock), patch('live.time.sleep', sleep):
            observer.pulse('RightMouseButton', 1.8, expected_bindings=observer.bindings(), during=fire)
        self.assertEqual([(c.args[1], c.args[2]) for c in observer.user.PostMessageW.call_args_list],
                         [(0x204, 2), (0x201, 3), (0x202, 2), (0x205, 0)])
        self.assertEqual(observer._mouse_holds, {})

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_nested_fire_cancellation_releases_fire_and_outer_aim(self, background):
        observer = self.fixture()
        observer.bindings.return_value = {'RightMouseButton': 'Game_Target', 'LeftMouseButton': 'Game_Shoot'}
        clock, sleep = self.chord_clock()
        def fire(row):
            return observer.pulse('LeftMouseButton', .1, expected_bindings=observer.bindings(),
                                  held_keys=('RightMouseButton',), stop_requested=Mock(side_effect=[False, True]))
        with patch('live.time.monotonic', clock), patch('live.time.sleep', sleep):
            with self.assertRaisesRegex(RuntimeError, 'during input'):
                observer.pulse('RightMouseButton', .2, expected_bindings=observer.bindings(), during=fire)
        self.assertEqual([c.args[1] for c in observer.user.PostMessageW.call_args_list], [0x204, 0x201, 0x202, 0x205])
        self.assertEqual(observer._mouse_holds, {})

    def test_claimed_aim_hold_without_posted_down_never_admits_fire(self):
        observer = self.fixture()
        bindings = {'RightMouseButton': 'Game_Target', 'LeftMouseButton': 'Game_Shoot'}
        with self.assertRaises(InputNotReady):
            observer.pulse('LeftMouseButton', .1, expected_bindings=bindings, held_keys=('RightMouseButton',))
        observer.user.PostMessageW.assert_not_called()

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_release_post_failure_is_a_fatal_non_neutral_state(self, background):
        observer = self.fixture();observer.user.PostMessageW.side_effect = [True, False]
        clock, sleep = self.chord_clock()
        with patch('live.time.monotonic', clock), patch('live.time.sleep', sleep):
            with self.assertRaises(MouseReleaseFailed):
                observer.pulse('MiddleMouseButton', .03, expected_bindings=observer.bindings())
        self.assertIn('MiddleMouseButton', observer._mouse_holds)

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_registry_churn_releases_pulse_and_requires_new_observation(self, background):
        observer = self.fixture()
        reader = observer.reader = Mock()
        observer.menu.side_effect = RegistrySnapshotChanged('registry grew')
        with self.assertRaises(InputInterrupted):
            observer.pulse('MiddleMouseButton', .1, expected_bindings=observer.bindings())
        self.assertEqual([v.args[1] for v in observer.user.PostMessageW.call_args_list], [0x207, 0x208])
        reader.close.assert_called_once()
        self.assertIsNone(observer.reader)

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_disappearing_menu_object_releases_pulse_before_rediscovery(self, background):
        observer = self.fixture()
        observer.reader = Mock()
        observer.menu.side_effect = RuntimeError('Object outside registry')
        with self.assertRaises(InputInterrupted):
            observer.pulse('MiddleMouseButton', .1, expected_bindings=observer.bindings())
        self.assertEqual([v.args[1] for v in observer.user.PostMessageW.call_args_list], [0x207, 0x208])

    def interaction_fixture(self):
        observer = self.fixture()
        observer.sampler = Mock(controller=1000)
        observer.bindings.return_value = {'E': 'Game_Pickup'}
        event = dict(address=2000, internal_index=2, number=0, cls=3)
        return observer, event

    @patch('world_state.Query')
    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_interaction_releases_when_post_dispatch_verification_fails(self, background, query):
        observer, event = self.interaction_fixture()
        query.return_value.pointer.return_value = event
        query.return_value.s.derives.return_value = False
        observer.verify.side_effect = [None, RuntimeError('cancelled after dispatch')]
        with self.assertRaisesRegex(RuntimeError, 'after dispatch'):
            observer.interaction_key('E', event)
        self.assertEqual([v.args[1] for v in observer.user.PostMessageW.call_args_list], [0x100, 0x101])

    @patch('world_state.Query')
    def test_changed_interaction_event_never_receives_a_key(self, query):
        observer, event = self.interaction_fixture()
        query.return_value.pointer.return_value = None
        with self.assertRaisesRegex(InputNotReady, 'event changed'):
            observer.interaction_key('E', event)
        observer.user.PostMessageW.assert_not_called()

    def test_interaction_does_not_assume_e_is_pickup(self):
        observer, event = self.interaction_fixture()
        observer.bindings.return_value = {'E': 'Game_Melee'}
        with self.assertRaisesRegex(RuntimeError, 'bound to Game_Pickup'):
            observer.interaction_key('E', event)
        observer.user.PostMessageW.assert_not_called()

    @patch('world_state.Query')
    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_modifier_held_prevents_interaction_dispatch(self, background, query):
        observer, event = self.interaction_fixture()
        query.return_value.pointer.return_value = event
        query.return_value.s.derives.return_value = False
        observer.user.GetAsyncKeyState.return_value = 0x8000
        with self.assertRaisesRegex(InputNotReady, 'Modifier'):
            observer.interaction_key('E', event)
        observer.user.PostMessageW.assert_not_called()

    @patch('live.time.sleep')
    def test_menu_log_failure_does_not_leave_key_down(self, sleep):
        observer = self.fixture()
        with self.assertRaisesRegex(RuntimeError, 'report unavailable'):
            observer.menu_key('escape', sample(0)['menu'],
                              emit=Mock(side_effect=RuntimeError('report unavailable')))
        self.assertEqual([v.args[1] for v in observer.user.PostMessageW.call_args_list], [0x100, 0x101])

    @patch('live.time.sleep')
    def test_gameover_allows_only_the_observed_checkpoint_button(self, sleep):
        observer = self.fixture()
        state = dict(sample(0)['menu'], player_index=0,
                     top=dict(tag='DeltaGameover', cls='WarUIScene', input_mode=1),
                     focused_leaf=dict(name='btnCheckpoint', cls='UILabelButton'))
        observer.menu.return_value = state
        observer.menu_key('confirm', state)
        self.assertEqual([v.args[1] for v in observer.user.PostMessageW.call_args_list], [0x100, 0x101])
        observer.user.PostMessageW.reset_mock()
        state['focused_leaf']['name'] = 'btnExit'
        with self.assertRaisesRegex(RuntimeError, 'outside reviewed'):
            observer.menu_key('confirm', state)
        observer.user.PostMessageW.assert_not_called()


if __name__ == '__main__':
    unittest.main()
