import pathlib
import tempfile
import unittest
import json
from unittest.mock import patch
from menu_display_monitor import MenuDisplayMonitor, PACKET, classify, UNKNOWN, GAMEPLAY, DEATH_MENU


def sample(death=True):
    return dict(pid=11, read_only=True, consistent_reread=True, player_index=0, input_block_count=0,
                top=dict(name='DeltaGameover' if death else 'PenderCrossplayInvitePopUp',
                         tag='DeltaGameover' if death else 'PenderCrossplayInvitePopUp',
                         cls='WarUIScene' if death else 'WarUIScene_CrossplayInvitePopUp', input_mode=1 if death else 2))


class Reader:
    closed=False
    def sample(self):return sample()
    def close(self):self.closed=True


class Tests(unittest.TestCase):
    def test_semantic_gates(self):
        self.assertEqual(classify(sample()), DEATH_MENU)
        self.assertEqual(classify(sample(False)), GAMEPLAY)
        for key, value in [('consistent_reread', False), ('read_only', False), ('player_index', 1), ('input_block_count', 1)]:
            self.assertEqual(classify(dict(sample(), **{key:value})), UNKNOWN)
        for key in ('tag','cls','input_mode'):
            s=sample();s['top'][key]='unknown';self.assertEqual(classify(s), UNKNOWN)

    def monitor(self, path, reader, **kwargs):
        game=dict(pid=11,creation_time=22,image='game');owner=dict(pid=33,creation_time=44,image='owner')
        return MenuDisplayMonitor(path,game,owner,lambda pid:game,lambda:reader,
                                  kwargs.get('reader_identity',lambda r:(11,22)),lambda:100)

    def test_packet_and_stop(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            m.poll();self.assertEqual(PACKET.unpack(path.read_bytes()),(0x47564d31,1,64,2,11,22,33,44,1,100))
            m.poll();self.assertEqual(PACKET.unpack(path.read_bytes())[-2],2)
            m.close();self.assertTrue(reader.closed);self.assertEqual(PACKET.unpack(path.read_bytes())[3],UNKNOWN)

    def test_wrong_creation_never_announces_menu(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader,reader_identity=lambda r:(11,23))
            m.poll();self.assertTrue(reader.closed);self.assertEqual(PACKET.unpack(path.read_bytes())[3],UNKNOWN)
            m.close()

    def test_read_failure_invalidates_prior_menu(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            m.poll()
            def fail():raise RuntimeError('changed')
            reader.sample=fail;m.poll();self.assertEqual(PACKET.unpack(path.read_bytes())[3],UNKNOWN)
            m.close()

    def test_replace_failure_preserves_packet_and_recovers_with_new_sample(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            m.poll();previous=path.read_bytes()
            with patch('menu_display_monitor.os.replace', side_effect=PermissionError('busy')):
                m.clock=lambda:200
                m.poll()
                self.assertEqual(path.read_bytes(),previous)
                self.assertFalse(path.with_suffix('.tmp').exists())
            reader.sample=lambda:sample(False)
            m.clock=lambda:300
            m.poll()
            packet=PACKET.unpack(path.read_bytes())
            self.assertEqual((packet[3],packet[-1]),(GAMEPLAY,300))
            events=[json.loads(line).get('event') for line in path.with_suffix('.jsonl').read_text().splitlines()]
            self.assertEqual(events.count('publish_failed'),1)
            self.assertEqual(events.count('publish_recovered'),1)
            m.close()

    def test_initial_failure_is_bounded_and_close_cleans_up(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            with patch('menu_display_monitor.os.replace', side_effect=PermissionError('busy')):
                for _ in range(20):m.poll()
                self.assertFalse(path.exists())
                m.close()
            self.assertTrue(reader.closed)
            self.assertTrue(m.log.closed)
            self.assertFalse(path.exists())
            self.assertFalse(path.with_suffix('.tmp').exists())
            rows=[json.loads(line) for line in path.with_suffix('.jsonl').read_text().splitlines()]
            self.assertEqual(len(rows),1)
            self.assertEqual(rows[0]['event'],'publish_failed')

    def test_game_identity_change_still_raises_when_publish_fails(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            m.identity=lambda pid:None
            with patch('menu_display_monitor.os.replace', side_effect=PermissionError('busy')):
                with self.assertRaisesRegex(RuntimeError,'game process changed'):m.poll()
                m.close()

if __name__=='__main__':unittest.main()
