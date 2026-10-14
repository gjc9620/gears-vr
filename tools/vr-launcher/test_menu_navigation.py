"""Offline tests: imports no live reader, OS input or process adapter."""
import json
from pathlib import Path
import tempfile
import unittest
from menu_navigation import navigate_core, NavigationCancelled


def continue_state():
    top = dict(tag='DeltaCampaignMenu_ContinueGame', cls='WarUIScene_Campaign')
    leaf = dict(name='butContinue', cls='UILabelButton', states=['UIState_Enabled', 'UIState_Focused'])
    return dict(pid=7, consistent_reread=True, player_index=0, input_block_count=0,
                top=top, focused_leaf=leaf, focus_chain=[leaf])


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

    def test_bound_window_cannot_be_replaced(self):
        with self.assertRaises(RuntimeError):
            self.run_navigation(window=lambda: (7, 99 if self.clock < .3 else 100))
        self.assertEqual(self.closed, 1)
        self.assertFalse(self.sent)

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


if __name__ == '__main__':
    unittest.main()
