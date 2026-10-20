from pathlib import Path
import tempfile
import unittest

from core import Evidence
from summarize import summarize


class AssistanceGroupingTests(unittest.TestCase):
    def test_mod_candidates_keep_xr_versions_separate_and_require_explicit_confirmation(self):
        with tempfile.TemporaryDirectory() as folder:
            paths = []
            for xr in ('one','two'):
                report = Evidence(Path(folder)/xr, mode='observe', provenance=dict(
                    artifacts=dict(game_sha256='game',native_sha256='native',xr_sha256=xr)))
                report.issue('mod_native_fault','Same native fault',classification='mod_candidate')
                report.save(phase='finished');paths.append(report.directory/'report.json')
            result = summarize(paths)
            self.assertEqual(result['issue_groups'],2)
            self.assertEqual({i['xr_sha256'] for i in result['issues']},{'one','two'})
            self.assertEqual(result['confirmed_mod_bugs'],0)

    def test_health_assistance_is_separate_from_ordinary_health_evidence(self):
        with tempfile.TemporaryDirectory() as folder:
            paths = []
            for enabled in (False, True):
                p = Path(folder) / str(enabled)
                report = Evidence(p, mode='run', provenance=dict(
                    manifest=dict(game_sha256='game', native_sha256='native'),
                    health_assistance=dict(enabled=enabled)))
                report.assisted = True
                report.issue('same_issue', 'Same title', classification='product_candidate')
                report.save(phase='finished')
                paths.append(p / 'report.json')
            result = summarize(paths)
            self.assertEqual(result['issue_groups'], 2)
            self.assertEqual({i['health_assistance'] for i in result['issues']}, {False, True})
            self.assertEqual(result['confirmed_game_bugs'], 0)

    def test_assisted_fixture_keeps_health_assistance_provenance(self):
        with tempfile.TemporaryDirectory() as folder:
            p = Path(folder) / 'inherited'
            report = Evidence(p, mode='run', provenance=dict(fixture=dict(source_health_assistance=True)))
            report.issue('same_issue', 'Same title')
            report.save(phase='finished')
            result = summarize([p / 'report.json'])
            self.assertIs(result['runs'][0]['health_assistance'], True)
            self.assertIs(result['issues'][0]['health_assistance'], True)
