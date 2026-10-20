import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from progress import export_fixture


class ProgressTests(unittest.TestCase):
    def setUp(self):
        root = Path(__file__).resolve().parents[2] / 'analysis'
        self.temp = tempfile.TemporaryDirectory(dir=root)
        self.folder = Path(self.temp.name)
        self.run = self.folder / 'run'
        backup = self.run / 'backup'
        backup.mkdir(parents=True)
        original, after = b'original profile', b'post-game checkpoint'
        old_hash, new_hash = (hashlib.sha256(value).hexdigest() for value in (original, after))
        (backup / '1').write_bytes(original)
        (backup / '1.after').write_bytes(after)
        source = str(self.folder / 'SaveData/123.ue3profile')
        self.write(backup / 'manifest.json', dict(files=[dict(source=source, backup=str(backup / '1'), sha256=old_hash)]))
        self.write(backup / 'restored.json', dict(restored=True, files=[dict(source=source,
                   restored_sha256=old_hash, test_sha256=new_hash)]))
        self.report = dict(mode='run', phase='finished', assisted=False,
                           provenance=dict(manifest=dict(game_sha256='game-version'), lifecycle={key: True for key in
                            ('native_cleanup_verified', 'entry_restore_verified', 'game_exit_confirmed', 'protected_files_restored')}))
        self.write(self.run / 'report.json', self.report)

    def write(self, path, value):
        path.write_text(json.dumps(value), encoding='utf8')

    def tearDown(self):
        self.temp.cleanup()

    def test_continuation_uses_post_test_bytes_and_does_not_touch_user_files(self):
        output = self.folder / 'fixture.json'
        result = export_fixture(self.run, output)
        self.assertEqual(Path(result['files'][0]['path']).read_bytes(), b'post-game checkpoint')
        self.assertEqual((self.run / 'backup/1').read_bytes(), b'original profile')
        self.assertEqual(result['game_sha256'], 'game-version')
        self.assertTrue(result['source_run_closed_normally'])
        self.assertFalse(result['source_scenario_completed'])
        self.assertFalse((self.folder / 'SaveData').exists())

    def test_tampered_post_profile_cannot_be_exported(self):
        (self.run / 'backup/1.after').write_bytes(b'tampered')
        output = self.folder / 'fixture.json'
        with self.assertRaisesRegex(RuntimeError, 'Post-test profile bytes'):
            export_fixture(self.run, output)
        self.assertFalse(output.exists())

    def test_checkpoint_history_survives_profile_export_with_observation_provenance(self):
        old = dict(game_sha256='game-version', root_path=[['map', 0, 'Package']],
                   event_name=dict(name='fire', number=2), observed_count=1)
        self.report['provenance']['fixture'] = dict(completed_checkpoint_signals=[old])
        self.write(self.run / 'report.json', self.report)
        fresh = dict(old, observed_count=3)
        source = self.run / 'checkpoint-flow-1.json'
        self.write(source, dict(completed_checkpoint_signals_observed=[fresh]))
        result = export_fixture(self.run, self.folder / 'fixture.json')
        history = result['completed_checkpoint_signals']
        self.assertEqual(len(history), 1)
        self.assertEqual(history[0]['observed_count'], 3)
        self.assertEqual(history[0]['source_sha256'], hashlib.sha256(source.read_bytes()).hexdigest())
        self.assertFalse(result['chapter_completion_verified'])

    def test_live_or_unrestored_run_cannot_supply_a_continuation(self):
        self.report['provenance']['lifecycle']['game_exit_confirmed'] = False
        self.write(self.run / 'report.json', self.report)
        with self.assertRaisesRegex(RuntimeError, 'verified owned cleanup'):
            export_fixture(self.run, self.folder / 'fixture.json')

    def test_incomplete_scenario_needs_opt_in_and_preserves_its_status(self):
        self.report['phase'] = 'incomplete'
        self.write(self.run / 'report.json', self.report)
        with self.assertRaisesRegex(RuntimeError, 'verified owned cleanup'):
            export_fixture(self.run, self.folder / 'fixture.json')
        result = export_fixture(self.run, self.folder / 'fixture.json', allow_incomplete=True)
        self.assertEqual(result['source_run_phase'], 'incomplete')
        self.assertFalse(result['source_scenario_completed'])

    def test_opt_in_does_not_admit_live_or_unrestored_profiles(self):
        self.report['phase'] = 'incomplete'
        self.report['provenance']['lifecycle']['game_exit_confirmed'] = False
        self.write(self.run / 'report.json', self.report)
        with self.assertRaisesRegex(RuntimeError, 'verified owned cleanup'):
            export_fixture(self.run, self.folder / 'fixture.json', allow_incomplete=True)


if __name__ == '__main__':
    unittest.main()
