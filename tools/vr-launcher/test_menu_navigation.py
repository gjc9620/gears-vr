"""Offline tests: imports no live reader, OS input or process adapter."""
import json
from pathlib import Path
import tempfile
import unittest
import sys
from types import SimpleNamespace
from unittest import mock
from menu_navigation import navigate_core, NavigationCancelled


def continue_state():
    top = dict(tag='DeltaCampaignMenu_ContinueGame', cls='WarUIScene_Campaign')
    leaf = dict(name='butContinue', cls='UILabelButton', states=['UIState_Enabled', 'UIState_Focused'])
    return dict(pid=7, consistent_reread=True, player_index=0, input_block_count=0,
                top=top, focused_leaf=leaf, focus_chain=[leaf])


def campaign_state():
    state = continue_state()
    state['top'] = dict(tag='DeltaMainMenu', cls='WarUIScene_MainMenu')
    state['focused_leaf'].update(name='CampaignButton', cls='UIButtonIgnoreStart')
    return state


class NavigationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / 'menu.jsonl'
        self.clock, self.closed, self.sent = 0., 0, []
        self.game = dict(pid=7, creation_time=123, image='game.exe')

    def sleep(self, seconds):
        self.clock += seconds

    def close(self):
        self.closed += 1

    def sample(self):
        return continue_state()

    def run_navigation(self, **kwargs):
        return navigate_core(7, self.game, kwargs.pop('deadline', 2), self.path,
                             reader_factory=kwargs.pop('reader_factory', lambda: self),
                             window=kwargs.pop('window', lambda: (7, 99)),
                             send=kwargs.pop('send', lambda *args: self.sent.append(args)),
                             interrupted=kwargs.pop('interrupted', lambda: False),
                             identity=kwargs.pop('identity', lambda pid: self.game),
                             now=lambda: self.clock, sleep=self.sleep, **kwargs)

    def test_window_missing_then_ready_same_deadline_cleanup(self):
        def window():
            if self.clock < .5:
                raise RuntimeError('Game window not found')
            return (7, 99)
        result = self.run_navigation(window=window)
        self.assertEqual(result['reason'], 'loading_requested')
        self.assertFalse(result['level_ready'])
        self.assertEqual(len(self.sent), 1)
        self.assertGreaterEqual(self.clock, 1.1)
        self.assertEqual(self.closed, 1)
        self.assertTrue(any(json.loads(x)['event'] == 'action_sent' for x in self.path.read_text().splitlines()))

    def test_missing_window_times_out_without_reader(self):
        def missing():
            raise RuntimeError('Game window not found')
        with self.assertRaisesRegex(RuntimeError, 'window wait timeout'):
            self.run_navigation(window=missing, deadline=.4)
        self.assertEqual(self.clock, .4)
        self.assertEqual(self.closed, 0)
        self.assertFalse(self.sent)

    def test_identity_changed_during_poll_stops_and_closes(self):
        with self.assertRaisesRegex(RuntimeError, 'identity changed'):
            self.run_navigation(identity=lambda pid: self.game if self.clock < .3 else None)
        self.assertEqual(self.closed, 1)
        self.assertFalse(self.sent)

    def test_cancel_stops_before_and_after_ready(self):
        with self.assertRaises(NavigationCancelled):
            self.run_navigation(interrupted=lambda: True)
        self.assertEqual(self.closed, 0)
        with self.assertRaises(NavigationCancelled):
            self.run_navigation(interrupted=lambda: self.clock >= .3)
        self.assertEqual(self.closed, 1)
        self.assertFalse(self.sent)

    def test_unknown_menu_timeout_closes(self):
        self.sample = lambda: dict(continue_state(), top=dict(tag='Unknown', cls='UIScene'))
        with self.assertRaisesRegex(RuntimeError, 'navigation failed: timeout'):
            self.run_navigation(deadline=.5)
        self.assertEqual(self.closed, 1)
        self.assertFalse(self.sent)

    def test_bound_window_replaced_restarts_snapshot_stability(self):
        result = self.run_navigation(window=lambda: (7, 99 if self.clock < .3 else 100))
        self.assertEqual(result['reason'], 'loading_requested')
        self.assertEqual(self.sent, [('confirm', (7, 100))])
        self.assertGreaterEqual(self.clock, .3 + .15 + .6)
        self.assertEqual(self.closed, 2)

    def test_bound_window_temporarily_missing_recovers_even_same_handle(self):
        def window():
            if .3 <= self.clock < .8:
                raise RuntimeError('Game window not found')
            return (7, 99)
        result = self.run_navigation(window=window)
        self.assertEqual(result['reason'], 'loading_requested')
        self.assertEqual(self.sent, [('confirm', (7, 99))])
        self.assertGreaterEqual(self.clock, .8 + .6)
        self.assertEqual(self.closed, 2)

    def test_window_changes_while_reader_is_constructed(self):
        handle = [99]
        def factory():
            handle[0] = 100
            return self
        result = self.run_navigation(window=lambda: (7, handle[0]), reader_factory=factory)
        self.assertEqual(result['reason'], 'loading_requested')
        self.assertEqual(self.closed, 2)
        self.assertEqual(self.sent, [('confirm', (7, 100))])

    def test_rebinding_keeps_ledger_with_only_one_successful_navigation_retry(self):
        def sample():
            state = continue_state()
            if self.clock < 2:
                state['top'] = dict(tag='DeltaCampaignType', cls='WarUIScene_CampaignType')
                state['focused_leaf']['name'] = 'butSinglePlayer'
            return state
        self.sample = sample
        result = self.run_navigation(window=lambda: (7, 99 if self.clock < .8 else 100), deadline=4)
        self.assertEqual(result['reason'], 'loading_requested')
        self.assertEqual(result['actions'], 3)
        self.assertEqual(self.sent, [('confirm', (7, 99)), ('confirm', (7, 100)), ('confirm', (7, 100))])

    def test_successful_campaign_pair_then_missing_window_same_handle_reconfirms_once(self):
        missing_until = [0.]
        def send(action, target):
            self.sent.append((action, target))
            if len(self.sent) == 1:
                missing_until[0] = self.clock + .4
        def window():
            if self.clock < missing_until[0]:
                raise RuntimeError('Game window not found')
            return (7, 99)
        def sample():
            if len(self.sent) < 2:
                return campaign_state()
            state = continue_state()
            if len(self.sent) == 2:
                state['top'] = dict(tag='DeltaCampaignType', cls='WarUIScene_CampaignType')
                state['focused_leaf']['name'] = 'butSinglePlayer'
            return state
        self.sample = sample
        result = self.run_navigation(window=window, send=send, deadline=6)
        self.assertEqual(result['reason'], 'loading_requested')
        self.assertEqual(result['actions'], 4)
        self.assertEqual(self.closed, 2)
        events = [json.loads(line) for line in self.path.read_text().splitlines()]
        attempts = [event for event in events if event['event'] == 'action_attempt']
        self.assertEqual([event['choice'][2] for event in attempts],
                         ['CampaignButton', 'CampaignButton', 'butSinglePlayer', 'butContinue'])
        self.assertEqual(sum(event['window_retry'] for event in attempts), 1)
        rebound = [event for event in events if event['event'] == 'window_bound'][1]
        self.assertGreaterEqual(attempts[1]['elapsed'] - rebound['elapsed'], .6)

    def test_bound_window_remains_missing_uses_original_deadline(self):
        def window():
            if self.clock >= .3:
                raise RuntimeError('Game window not found')
            return (7, 99)
        with self.assertRaisesRegex(RuntimeError, 'window wait timeout'):
            self.run_navigation(window=window, deadline=.7)
        self.assertEqual(self.clock, .7)
        self.assertEqual(self.closed, 1)
        self.assertFalse(self.sent)

    def test_rebinding_does_not_adopt_another_process(self):
        with self.assertRaisesRegex(RuntimeError, 'another PID'):
            self.run_navigation(window=lambda: (7 if self.clock < .3 else 8, 99))
        self.assertEqual(self.closed, 1)
        self.assertFalse(self.sent)

    def test_process_exit_while_window_missing_is_terminal(self):
        def window():
            if self.clock >= .3:
                raise RuntimeError('Game window not found')
            return (7, 99)
        with self.assertRaisesRegex(RuntimeError, 'identity changed'):
            self.run_navigation(window=window, identity=lambda pid: self.game if self.clock < .5 else None)
        self.assertEqual(self.closed, 1)
        self.assertFalse(self.sent)

    def test_cancel_while_window_missing_is_terminal(self):
        def window():
            if self.clock >= .3:
                raise RuntimeError('Game window not found')
            return (7, 99)
        with self.assertRaises(NavigationCancelled):
            self.run_navigation(window=window, interrupted=lambda: self.clock >= .5)
        self.assertEqual(self.closed, 1)
        self.assertFalse(self.sent)

    def test_dispatch_failure_is_not_retried_on_recreated_window(self):
        self.sample = campaign_state
        handle = [99]
        def send(action, target):
            self.sent.append((action, target))
            handle[0] = 100
            raise RuntimeError('key release failed')
        with self.assertRaisesRegex(RuntimeError, 'action_error'):
            self.run_navigation(window=lambda: (7, handle[0]), send=send)
        self.assertEqual(self.sent, [('confirm', (7, 99))])
        self.assertEqual(self.closed, 1)

    def test_cancel_at_dispatch_guard_retains_cancel_semantics(self):
        ready_checks = [0]
        def window():
            if self.clock >= .6:
                ready_checks[0] += 1
            return (7, 99)
        with self.assertRaises(NavigationCancelled):
            self.run_navigation(window=window, interrupted=lambda: ready_checks[0] >= 2)
        self.assertFalse(self.sent)
        self.assertEqual(self.closed, 1)

    def test_initialization_wait_counts_against_parent_deadline(self):
        def factory():
            if self.clock < .5:
                raise RuntimeError('Expected one registered GameUISceneClient; found 0')
            return self
        with self.assertRaisesRegex(RuntimeError, 'navigation failed: timeout'):
            self.run_navigation(reader_factory=factory, deadline=.9)
        self.assertEqual(self.clock, .9)
        self.assertEqual(self.closed, 1)
        self.assertFalse(self.sent)

    def test_own_sixty_second_cap_and_unknown_constructor_failure(self):
        def missing():
            raise RuntimeError('Game window not found')
        with self.assertRaisesRegex(RuntimeError, 'window wait timeout'):
            self.run_navigation(window=missing, deadline=120)
        self.assertEqual(self.clock, 60)
        self.clock = 0
        def factory():
            raise RuntimeError('Unknown metadata error')
        with self.assertRaisesRegex(RuntimeError, 'Unknown metadata error'):
            self.run_navigation(reader_factory=factory)
        self.assertEqual(self.clock, 0)


class SelectedInstallationAdapterTests(unittest.TestCase):
    def test_custom_install_uses_reserved_window_not_repository_default(self):
        import menu_navigation
        handle=[99]
        fixed_lookup=mock.Mock(side_effect=AssertionError('Repository game path must not select this window'))
        backend=SimpleNamespace(game_window=mock.Mock(side_effect=lambda pid:handle[0]),
            cancelled=lambda cancel:False,control=SimpleNamespace(identity=lambda pid:dict(pid=pid,creation_time=123,image='D:/custom/game.exe')))
        modules={'menu_state':SimpleNamespace(MenuReader=mock.Mock()),
                 'menu_control':SimpleNamespace(send_confirm=mock.Mock()),
                 'game_io':SimpleNamespace(game_window=fixed_lookup,interrupted=lambda:False)}
        with mock.patch.dict(sys.modules,modules),mock.patch.object(menu_navigation,'navigate_core',return_value='selected') as core:
            self.assertEqual(menu_navigation.navigate(7,backend.control.identity(7),backend,None,100,Path('unused')), 'selected')
            window=core.call_args.kwargs['window']
            self.assertEqual(window(),(7,99));handle[0]=100
            self.assertEqual(window(),(7,100))
            core.call_args.kwargs['send']('confirm',(7,100))
            self.assertIs(modules['menu_control'].send_confirm.call_args.kwargs['window_lookup'],window)
            handle[0]=None
            with self.assertRaisesRegex(RuntimeError,'Game window not found'):window()
            fixed_lookup.assert_not_called()
            self.assertTrue(all(call.args==(7,) for call in backend.game_window.call_args_list))


if __name__ == '__main__':
    unittest.main()
