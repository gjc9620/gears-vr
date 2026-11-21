import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

import transfer as t

EDITOR = Path(__file__).resolve().parents[1]/'pose-editor/pose_editor.py'
spec = importlib.util.spec_from_file_location('existing_pose_editor', EDITOR)
editor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(editor)


class TransferTests(unittest.TestCase):
    def project(self, baseline):
        parts = []
        for key, joints in baseline['parts'].items():
            parts.append(dict(id=key, hands=[dict(side='right' if key=='main_grip' else 'left',
                              retargeted=True, local_fingers_xyzw=[[q[0],-q[1],-q[2],q[3]] for q in joints],
                              finger_correction_degrees=[[0., 0., 0.] for _ in range(15)])]))
        return dict(schema=1, kind='quest_weapon_workbench', rig_signature=t.SIGNATURE,
                    model='Lancer.glb', rig_joint_basis='ue_wrist_reflect_x', model_content_fnv64=baseline['model_content_fnv64'], parts=parts)

    def test_current_baseline_roundtrip_through_actual_mod_editor(self):
        root = Path(__file__).resolve().parents[2]
        baseline = t.baselines(root)
        with tempfile.TemporaryDirectory() as directory:
            result = t.convert(self.project(baseline), baseline, Path(directory))
            self.assertEqual(len(result['files']), 5)
            self.assertEqual(result['rejected'], [])
            for name in result['files']:
                profile = json.loads(Path(name).read_text('utf-8'))
                editor.validate(profile)
                self.assertEqual(len(editor.encode(profile, 123, True)), 600)
                for q in profile['finger_degrees']:
                    self.assertLess(max(map(abs, q)), 1e-8)

    def test_real_postmultiply_correction_and_bound_rejection(self):
        baseline = t.baselines(Path(__file__).resolve().parents[2])
        project = self.project(baseline)
        hand = project['parts'][0]['hands'][0]
        hand['finger_correction_degrees'][4] = [0, 0, 25]
        with tempfile.TemporaryDirectory() as directory:
            result = t.convert(project, baseline, Path(directory)/'valid')
            profile = json.loads(Path(result['files'][0]).read_text('utf-8'))
            editor.validate(profile)
            self.assertAlmostEqual(profile['finger_degrees'][4][2], -25)
            hand['finger_correction_degrees'][4] = [0, 0, 120]
            result = t.convert(project, baseline, Path(directory)/'bounded-rejection')
            self.assertEqual(len(result['rejected']), 1)
            self.assertIn('90-degree', result['rejected'][0]['reason'])
            self.assertFalse(result['wrist_and_volume_imported'])

    def test_non_lancer_archive_remains_exportable_without_mod_conversion(self):
        baseline=t.baselines(Path(__file__).resolve().parents[2]);project=self.project(baseline)
        project['model']='Troika_turret.glb';project['capture']={'id':'123-1','part':0,'side':1}
        with tempfile.TemporaryDirectory() as directory:
            source=Path(directory)/'captures';source.mkdir();(source/'123-1.json').write_text(json.dumps(project),'utf-8')
            report=t.convert_capture_library(source,baseline,Path(directory)/'export')
            self.assertTrue(report[0]['complete_project_preserved']);self.assertNotIn('error',report[0])

    def test_wrong_rig_and_model_rejected(self):
        baseline = t.baselines(Path(__file__).resolve().parents[2])
        with tempfile.TemporaryDirectory() as directory:
            project = self.project(baseline)
            project['rig_signature'] = '0'*64
            with self.assertRaises(ValueError):
                t.convert(project, baseline, Path(directory))
            project = self.project(baseline)
            project['model'] = 'Other.glb'
            with self.assertRaises(ValueError):
                t.convert(project, baseline, Path(directory))
            project = self.project(baseline)
            project.pop('rig_joint_basis')
            with self.assertRaisesRegex(ValueError, 'joint basis'):
                t.convert(project, baseline, Path(directory))

    def test_independent_capture_library_keeps_each_selected_pose(self):
        baseline = t.baselines(Path(__file__).resolve().parents[2])
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory)/'captures'
            source.mkdir()
            for i in range(2):
                project = self.project(baseline)
                project['capture'] = dict(id=f'100-{i}', name=f'Pose {i}', part=0, side=1)
                project['parts'][0]['hands'][0]['finger_correction_degrees'][0][2] = i*10
                (source/f'100-{i}.json').write_text(json.dumps(project), 'utf-8')
            bad = self.project(baseline)
            bad['capture'] = dict(id='100-9', part=-1, side=1)
            (source/'100-9.json').write_text(json.dumps(bad), 'utf-8')
            result = t.convert_capture_library(source, baseline, Path(directory)/'converted')
            self.assertEqual(len(result), 3)
            self.assertEqual([len(row['files']) for row in result[:2]], [1, 1])
            self.assertNotEqual(result[0]['files'][0], result[1]['files'][0])
            self.assertAlmostEqual(json.loads(Path(result[1]['files'][0]).read_text('utf-8'))['finger_degrees'][0][2], -10)
            self.assertEqual(result[2]['error'], 'Invalid capture part')


if __name__ == '__main__': unittest.main()
