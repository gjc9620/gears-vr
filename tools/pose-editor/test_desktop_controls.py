import json
from pathlib import Path
import tempfile
import unittest

from desktop_controls import Commands


class DesktopCommandTests(unittest.TestCase):
    def test_edge_waits_for_reader_and_preserves_press_release(self):
        with tempfile.TemporaryDirectory() as directory:
            c = Commands(directory)
            target = Path(directory)/'controller_pose_command.json'
            consumed = []
            c.hands[0]['squeeze'] = 1
            c.enqueue(0, lambda: consumed.append('press'))
            c.hands[0]['squeeze'] = 0
            c.enqueue(0, lambda: consumed.append('release'))
            c.tick()
            self.assertEqual(json.loads(target.read_text())['squeeze'], 1)
            c.tick()
            self.assertEqual(consumed, [])
            self.assertEqual(json.loads(target.read_text())['squeeze'], 1)
            target.unlink()
            c.tick()
            self.assertEqual(consumed, ['press'])
            self.assertEqual(json.loads(target.read_text())['squeeze'], 0)
            target.unlink()
            c.tick()
            self.assertEqual(consumed, ['press', 'release'])

    def test_release_clears_pending_actions_for_both_hands(self):
        with tempfile.TemporaryDirectory() as directory:
            c = Commands(directory)
            for side in range(2):
                c.hands[side].update(squeeze=1, trigger=1, buttonA=1, stickX=1)
                c.enqueue(side)
            c.release()
            self.assertEqual(len(c.queue), 2)
            for payload, _ in c.queue:
                for key in ('squeeze', 'trigger', 'buttonA', 'stickX'):
                    self.assertEqual(payload[key], 0)
            c.heartbeat()
            self.assertTrue((Path(directory)/'desktop-heartbeat').is_file())


if __name__ == '__main__':
    unittest.main()
