import pathlib
import tempfile
import unittest
from reload_policy import read_policy


class ReloadPolicyTest(unittest.TestCase):
    def test_editable_modes_and_separate_durations(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'6dof-reload.ini'
            self.assertEqual(read_policy(path)['packed'],4000|(4000<<16)|(2<<32))
            path.write_text('[Lancer]\nperfect_reload=original\ntactical_seconds=4.125\nempty_seconds=7.5\n',encoding='utf-8-sig')
            before=read_policy(path)
            self.assertEqual(before['packed'],4125|(7500<<16)|(1<<32))
            path.write_text('[Lancer]\nperfect_reload=off\n',encoding='utf-8')
            self.assertEqual(read_policy(path)['packed'],4000|(4000<<16))
            self.assertEqual(before['mode'],'original') # immutable session snapshot

    def test_bad_files_never_silently_extend_reward(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'6dof-reload.ini'
            for content in ('[Lancer]\ntactical_seconds=nan','[Lancer]\nempty_seconds=inf',
                '[Lancer]\nempty_seconds=0','[Lancer]\ntactical_seconds=61',
                '[Lancer]\nperfect_reload=always','[Lancer]\nempty_second=8',
                '[Other]\nempty_seconds=8','[DEFAULT]\nempty_seconds=8'):
                with self.subTest(content=content):
                    path.write_text(content,encoding='utf-8')
                    with self.assertRaises(ValueError):read_policy(path)

    def test_weapon_overrides_inherit_without_rewriting_owner_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'6dof-reload.ini'
            content=b'[Lancer]\nperfect_reload=vr\ntactical_seconds=5\nempty_seconds=8\n[Snub]\nempty_seconds=7\n[Hammerburst]\nperfect_reload=off\n'
            path.write_bytes(content)
            self.assertEqual(read_policy(path,'Snub')['packed'],5000|(7000<<16)|(2<<32))
            self.assertEqual(read_policy(path,'Hammerburst')['packed'],5000|(8000<<16))
            self.assertEqual(read_policy(path)['empty_ms'],8000)
            self.assertEqual(path.read_bytes(),content)


if __name__=='__main__':unittest.main()
