import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from types import SimpleNamespace

from core import Evidence
from live import Observer
from test_core import sample
import run


class ModCounterTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.report = Evidence(Path(self.temp.name)/'run', mode='observe', provenance={})

    def observe(self, second, **counters):
        row = sample(second, published=second+1)
        row['native'].update(started_qpc=1000, **counters)
        return self.report.observe(row)

    def test_report_each_counter_increase_once_and_keep_first_attachment_unknown(self):
        self.observe(0, owned_ui_faults=2)
        self.observe(1, owned_ui_faults=2)
        self.observe(2, owned_ui_faults=5)
        issue = next(iter(self.report.issues.values()))
        self.assertEqual(issue['classification'], 'mod_candidate')
        self.assertFalse(issue['confirmed'])
        self.assertEqual(issue['occurrences'], 2)
        self.assertIsNone(issue['evidence'][0]['new_count'])
        self.assertEqual(issue['evidence'][1]['new_count'], 3)

    def test_guard_and_semantic_counters_are_observations_not_mod_bugs(self):
        self.observe(0, owned_ui_faults=0, owned_material_faults=0,
                     owned_ui_semantic_incomplete=8, mirror_inventory_rejected=65)
        self.observe(1, owned_ui_faults=0, owned_material_faults=0,
                     owned_ui_semantic_incomplete=8, mirror_inventory_rejected=66)
        self.assertFalse(self.report.issues)
        self.assertEqual(self.report.counts['mod_diagnostic_counter'], 3)

    def test_reset_missing_and_invalid_values_are_not_false_increases(self):
        self.observe(0, owned_material_faults=0)
        self.observe(1)
        self.observe(2, owned_material_faults=True)
        self.observe(3, owned_material_faults=-1)
        self.assertFalse(self.report.issues)
        self.observe(4, owned_material_faults=2)
        self.observe(5, owned_material_faults=0)
        self.observe(6, owned_material_faults=0)
        self.assertEqual(next(iter(self.report.issues.values()))['occurrences'], 1)
        self.assertEqual(self.report.counts['mod_counter_reset'], 1)

    def test_unverified_status_is_ignored_but_player_acquisition_error_does_not_hide_native_fault(self):
        row = sample(0)
        row['native'].update(started_qpc=1000, owned_ui_faults=1)
        row['errors'] = [dict(component='native_status', type='RuntimeError', message='rejected')]
        self.report.observe(row)
        self.assertFalse(any(i['classification']=='mod_candidate' for i in self.report.issues.values()))
        row = sample(1, published=2)
        row['native'].update(started_qpc=1000, owned_ui_faults=1)
        row['errors'] = [dict(component='player_state', type='AnchorSamplingRejected', message='cross-frame')]
        self.report.observe(row)
        self.assertEqual(sum(i['classification']=='mod_candidate' for i in self.report.issues.values()), 1)


class ObservedClosureTests(unittest.TestCase):
    def test_missing_version_on_both_sides_is_not_a_version_match(self):
        with tempfile.TemporaryDirectory() as folder, patch.object(run,'ROOT',Path(folder)):
            root=Path(folder)/'analysis';root.mkdir()
            ids=dict(game_identity=dict(pid=1),owner_identity=dict(pid=2))
            source=root/'source.json';source.write_text(json.dumps(dict(mode='run',provenance=dict(**ids,manifest={}))),encoding='utf-8')
            observer=SimpleNamespace(identities=ids,session=dict(diagnostics_artifacts={}))
            report=Evidence(root/'passive',mode='observe',provenance={})
            with self.assertRaisesRegex(ValueError,'versions differ'):
                run.inherit_observed_context(report,observer,source)

    def test_passive_source_context_rejects_other_pid_and_preserves_assistance(self):
        with tempfile.TemporaryDirectory() as folder, patch.object(run,'ROOT',Path(folder)):
            root=Path(folder)/'analysis';root.mkdir()
            identities=dict(game_identity=dict(pid=10,creation_time=100),owner_identity=dict(pid=20,creation_time=200))
            artifacts=dict(game_sha256='game',native_sha256='native',xr_sha256='xr')
            source=root/'source.json'
            value=dict(mode='run',assisted=True,provenance=dict(**identities,manifest=artifacts,
                       health_assistance=dict(enabled=True)))
            source.write_text(json.dumps(value),encoding='utf-8')
            observer=SimpleNamespace(identities=identities,session=dict(diagnostics_artifacts=artifacts))
            report=Evidence(root/'passive',mode='observe',provenance={})
            run.inherit_observed_context(report,observer,source)
            self.assertTrue(report.assisted)
            self.assertTrue(report.provenance['observed_health_assistance'])
            self.assertEqual((report.directory/'source-run-at-attach.json').read_bytes(),source.read_bytes())
            value['provenance']['game_identity']=dict(pid=10,creation_time=101)
            source.write_text(json.dumps(value),encoding='utf-8')
            other=Evidence(root/'other',mode='observe',provenance={})
            with self.assertRaisesRegex(ValueError,'game/owner'):
                run.inherit_observed_context(other,observer,source)

    def test_only_matching_clean_source_stop_ends_passive_observation_normally(self):
        with tempfile.TemporaryDirectory() as folder:
            observer = Observer.__new__(Observer)
            observer.path = Path(folder)/'session.json'
            observer.session = dict(session_id='one')
            observer.identities = dict(game_identity=dict(pid=10, creation_time=100))
            valid = dict(session_id='one', phase='stopped', native_cleanup_verified=True,
                         entry_restore_verified=True, xr_exit_code=0, **observer.identities)
            observer.path.write_text(json.dumps(valid), encoding='utf-8')
            self.assertTrue(observer.closed_normally())
            for changed in (dict(phase='running'),dict(entry_restore_verified=False),
                            dict(xr_exit_code=1),dict(session_id='other'),
                            dict(game_identity=dict(pid=10,creation_time=101))):
                observer.path.write_text(json.dumps(dict(valid,**changed)), encoding='utf-8')
                self.assertFalse(observer.closed_normally())


if __name__ == '__main__':
    unittest.main()
