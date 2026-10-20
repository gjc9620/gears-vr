from pathlib import Path
import tempfile
import unittest
from unittest.mock import Mock, patch

from core import Evidence
from scenarios import wait_state
from test_core import sample
import test_live_input


class ScenarioTests(unittest.TestCase):
    @patch('scenarios.time.sleep')
    def test_pause_animation_block_is_waited_out(self, sleep):
        with tempfile.TemporaryDirectory() as folder:
            report = Evidence(Path(folder) / 'run', mode='test', provenance={})
            blocked, ready = sample(0, menu=False), sample(1, menu=False)
            blocked['menu']['input_block_count'] = 1
            observer = Mock()
            observer.user.GetAsyncKeyState.return_value = 0
            observer.sample.side_effect = [blocked, ready]
            row = wait_state(observer, report, lambda value: True)
            self.assertEqual(row['sample_id'], 2)
            self.assertEqual(observer.sample.call_count, 2)

    def test_unknown_checkpoint_prompt_cannot_be_confirmed(self):
        observer = test_live_input.InputTests().fixture()
        state = sample(0)['menu']
        state['top'] = dict(tag='WarfareConfirmation', cls='UIScene_Confirmation', input_mode=1)
        state['dialog_text'] = dict(Title='Unexpected', MessageText='Delete save')
        with self.assertRaisesRegex(RuntimeError, 'outside reviewed'):
            observer.menu_key('confirm', state)
        observer.user.PostMessageW.assert_not_called()

    def test_pause_with_active_block_rejects_input(self):
        observer = test_live_input.InputTests().fixture()
        state = sample(0)['menu']
        state['top'] = dict(tag='DeltaPause-Campaign-Host', input_mode=1)
        state['focused_leaf'] = dict(name='LBtnResumeSingle')
        state['input_block_count'] = 1
        with self.assertRaisesRegex(RuntimeError, 'outside reviewed'):
            observer.menu_key('escape', state)
        observer.user.PostMessageW.assert_not_called()


if __name__ == '__main__':
    unittest.main()
