"""Runner-only mocks: these checks do not validate native rendering or XR."""
import contextlib
import importlib.util
import io
import json
import math
import pathlib
import subprocess
import tempfile
import types
import unittest
from unittest import mock

spec = importlib.util.spec_from_file_location('stereo_runner', pathlib.Path(__file__).with_name('run_stereo_probe.py'))
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class RunnerTests(unittest.TestCase):
    def test_shutdown_diagnostics_explicit_xr_only_and_default_off(self):
        code,report,xr,native,_=self.run_mock(extra_args=['--stereo-game'],projection=True)
        self.assertEqual(code,0);self.assertNotIn('--shutdown-diagnostics',xr)
        self.assertFalse(report['shutdown_diagnostics_requested'])
        for flags in (['--stereo-game','--shutdown-diagnostics'],
                      ['--stereo-game','--shutdown-diagnostics','--trial-seconds','120',
                       '--operator-motion','--simulator-operator','--operator-profile','sixdof']):
            code,report,xr,native,_=self.run_mock(extra_args=flags,projection=True,
                operator=self.operator_fake() if '--operator-motion' in flags else None)
            self.assertEqual(code,0);self.assertEqual(xr.count('--shutdown-diagnostics'),1)
            self.assertNotIn('--shutdown-diagnostics',native.call_args.args[0])
            self.assertTrue(report['shutdown_diagnostics_requested'])

    def test_sixdof_profile_requires_explicit_120_simulation(self):
        for missing in ('--operator-motion','--simulator-operator','--stereo-game','--trial-seconds'):
            flags=['--operator-motion','--simulator-operator','--stereo-game','--trial-seconds','120']
            index=flags.index(missing);del flags[index:index+(2 if missing=='--trial-seconds' else 1)]
            with contextlib.redirect_stderr(io.StringIO()),mock.patch.object(runner.subprocess,'Popen') as spawn:
                with self.assertRaises(SystemExit):runner.main(['--pid','123','--operator-profile','sixdof',*flags])
                spawn.assert_not_called()
        code,report,*_=self.run_mock(extra_args=['--operator-motion','--simulator-operator','--stereo-game',
            '--trial-seconds','120','--operator-profile','sixdof'],operator=self.operator_fake(),projection=True)
        self.assertEqual(code,0);self.assertEqual(report['operator_profile'],'sixdof')

    def test_sixdof_relative_baseline_and_missing_axis_rejected(self):
        helper=runner.operator_module()
        initial={'position':[1.,2.,3.],'orientation':[0.,math.sqrt(.5),0.,math.sqrt(.5)]}
        targets=helper.motion_targets(initial,'sixdof')
        def rows(targets):
            return [dict(zip(('qx','qy','qz','qw'),map(str,t['orientation'])),
                         **dict(zip(('head_x','head_y','head_z'),map(str,t['position'])))) for t in targets]
        ranges=runner.sixdof_ranges(rows(targets),initial)
        for k,v in {'pitch':20,'yaw':30,'roll':20}.items():self.assertAlmostEqual(ranges['rotation_span_degrees'][k],v)
        for k,v in {'x':.12,'y':.08,'z':.10}.items():self.assertAlmostEqual(ranges['translation_span_metres'][k],v)
        opposite=rows(targets)
        for row in opposite:
            for k in ('qx','qy','qz','qw'):row[k]=str(-float(row[k]))
        self.assertEqual(runner.sixdof_ranges(opposite,initial),ranges)
        with self.assertRaises(ValueError):runner.sixdof_ranges(rows(targets),None)

    def test_sixdof_all_axes_flags_stages_and_native_motion_required(self):
        helper=runner.operator_module();initial={'position':[0.,0.,0.],'orientation':[0.,0.,0.,1.]}
        targets=helper.motion_targets(initial,'sixdof')
        with tempfile.TemporaryDirectory() as folder:
            root=pathlib.Path(folder);xr,native,status=(root/name for name in ('xr.csv','native.csv','status.txt'))
            residuals=' '.join(f'residual_{eye}_p{p}_{edge}=0' for eye in ('l','r') for p in (8,9) for edge in ('min','max'))
            status.write_text('stereo_xr first_sequence=1 last_sequence=13 accepted=13\n'+''.join(
                f'stereo_stage stage={stage} count=13 mismatch=0 view_error_max=0 {residuals}\n'
                for stage in ('built','consume_before','prepare_before','prepare_after')))
            header='stereo_sequence,stereo_published,session_state,should_render,head_flags,view_flags,stereo_source,qx,qy,qz,qw,head_x,head_y,head_z\n'
            def save_targets(values):
                xr.write_text(header+''.join(f'{i+1},1,5,1,3,15,1,'+','.join(map(str,t['orientation']+t['position']))+'\n' for i,t in enumerate(values)))
            save_targets(targets)
            native.write_text('kind,test_state,family_count,valid,'+','.join('matrix'+str(i) for i in range(16))+'\n'+''.join(
                f'{kind},77,2,512,'+','.join(str(delta if i in (0,12) else 0) for i in range(16))+'\n'
                for kind in (10,12) for delta in (-.2,.2)))
            validate=lambda:runner.validate_operator_motion(xr,native,status,True,'sixdof',initial)
            self.assertTrue(validate()['passed'])
            # yaw/x alone cannot satisfy six-axis validation.
            save_targets(helper.motion_targets(initial))
            self.assertFalse(validate()['passed'])
            save_targets(targets)
            xr.write_text(xr.read_text().replace(',3,15,1,',',3,15,0,'));self.assertFalse(validate()['passed'])
            save_targets(targets)
            status.write_text(status.read_text().replace('residual_r_p9_max=0','residual_r_p9_max=1'))
            self.assertFalse(validate()['passed'])

    def test_partial_tail_is_not_tracking_evidence(self):
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder) / 'pose.csv'
            header = b'stereo_published,session_state,should_render,view_flags,head_flags\r\n'
            valid = b'1,5,1,15,15\r\n'
            path.write_bytes(header + valid * 60 + b'1,5,1,15,')
            rows = runner.complete_csv_rows(path)
            self.assertEqual(len(rows), 60)
            self.assertTrue(all(row['head_flags'] == '15' for row in rows[-20:]))
            # A completed invalid record must still fail the strict flags gate.
            path.write_bytes(header + valid * 60 + b'1,5,1,15,3\r\n')
            self.assertEqual(runner.complete_csv_rows(path)[-1]['head_flags'], '3')
            path.write_bytes(header[:-1])
            self.assertEqual(runner.complete_csv_rows(path), [])

    def run_mock(self, native_code=0, ready=True, xr_timeout=False, native_timeout=False, extra_args=(), projection=False, snapshot=False, operator=None, motion_passed=True):
        with tempfile.TemporaryDirectory() as folder:
            root = pathlib.Path(folder)
            child = mock.Mock()
            child.poll.return_value = None if ready else 1
            child.wait.side_effect = [subprocess.TimeoutExpired('xr', 65), 1] if xr_timeout else [0]

            def spawn(command, stdout, stderr, **options):
                output = pathlib.Path(command[2])
                if ready:
                    source = int('--simulator-operator' in extra_args)
                    output.write_text('stereo_published,session_state,should_render,view_flags,head_flags,stereo_source\n'
                                      + f'1,5,1,15,15,{source}\n' * 60, encoding='utf-8')
                stdout.write(json.dumps({'submitted_image_layers': 12}) + '\n')
                if projection:
                    stdout.write(json.dumps({'stereo_game_projection_layers': 100, 'stereo_game_copies': 100}) + '\n')
                    stdout.write(json.dumps({'stereo_submission_evidence': True, 'sustained_submission_passed': True, 'structural_failure': False,
                                             'required_span_seconds': 119 if '--trial-seconds' in extra_args else 29}) + '\n')
                if snapshot:
                    pathlib.Path(str(output) + '-gpu.ppm').write_bytes(b'P6\n1 1\n255\n\x00\x00\x00')
                stdout.flush()
                return child

            native_result = mock.Mock(returncode=native_code)
            def native_run(*args, **kwargs):
                if native_timeout:
                    raise subprocess.TimeoutExpired('native', 180)
                if operator:
                    operator.events.append('native')
                    live = root / 'native/capture.csv'
                    live.parent.mkdir(parents=True, exist_ok=True)
                    live.write_text('kind,test_state,family_count,qpc\n10,77,2,100\n')
                    if operator.callback():
                        operator.events.append('motion')
                return native_result
            with mock.patch.object(runner, 'ROOT', root), \
                 mock.patch.object(runner.subprocess, 'Popen', side_effect=spawn) as popen, \
                 mock.patch.object(runner.subprocess, 'run', side_effect=native_run) as run, \
                 mock.patch.object(runner, 'operator_module', return_value=types.SimpleNamespace(
                     assert_operator_port_free=lambda: None, operator_child_env=lambda env: dict(env, XR_ENABLE_API_LAYERS='operator-test'),
                     OperatorMotion=lambda *args, **kwargs: operator)), \
                 mock.patch.object(runner, 'validate_operator_motion', return_value={'passed': motion_passed}), \
                 mock.patch.object(runner, 'validate_stereo_sources', return_value={'passed': True}), \
                 contextlib.redirect_stdout(io.StringIO()):
                code = runner.main(['--pid', '123', '--attempt', '2', '--build-dir', str(root / 'native'),
                                    '--dll-name', 'Custom.dll', '--xr-build-dir', str(root / 'xr'),
                                    '--frame-link-diagnostics', *extra_args])
            report = json.loads((root / 'analysis/runtime/stereo-xr-123-attempt2.json').read_text())
            return code, report, popen.call_args.args[0], run, child

    def test_dispatch_and_report(self):
        code, report, command, native, child = self.run_mock()
        self.assertEqual(code, 0)
        self.assertTrue(command[0].endswith('xr\\OpenXRPoseProbe.exe') or command[0].endswith('xr/OpenXRPoseProbe.exe'))
        self.assertEqual(command[-3:], ['--render', '--stereo-bridge-pid', '123'])
        dispatched = native.call_args.args[0]
        self.assertIn('--frame-link-diagnostics', dispatched)
        self.assertEqual(dispatched[dispatched.index('--dll-name') + 1], 'Custom.dll')
        self.assertEqual(dispatched[dispatched.index('--attempt') + 1], '2')
        self.assertEqual(report['render_pattern_summary']['submitted_image_layers'], 12)

    def test_native_failed(self):
        code, report, *_ = self.run_mock(native_code=1)
        self.assertEqual(code, 1)
        self.assertFalse(report['passed'])

    def test_unready_does_not_dispatch_native(self):
        code, report, command, native, child = self.run_mock(ready=False)
        self.assertEqual(code, 1)
        native.assert_not_called()
        self.assertFalse(report['native_started'])

    def test_timeouts_fail(self):
        for options in ({'xr_timeout': True}, {'native_timeout': True}):
            with self.subTest(options=options):
                code, report, command, native, child = self.run_mock(**options)
                self.assertEqual(code, 1)
                self.assertFalse(report['passed'])
                if options.get('xr_timeout'):
                    child.terminate.assert_called_once()

    def test_projection_cannot_pass_with_exit_zero_only(self):
        report = {'native_exit_code': 0, 'xr_exit_code': 0}
        self.assertFalse(runner.passed(report, True))
        report['stereo_submission_evidence'] = {'sustained_submission_passed': True, 'structural_failure': False, 'required_span_seconds': 29}
        report['stereo_game_summary'] = {'stereo_game_projection_layers': 0, 'stereo_game_copies': 10}
        self.assertFalse(runner.passed(report, True))
        report['stereo_game_summary']['stereo_game_projection_layers'] = 10
        self.assertTrue(runner.passed(report, True))
        report['unsafe_xr_teardown'] = True
        self.assertFalse(runner.passed(report, True))

    def test_collect_projection_and_rejections(self):
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder) / 'stdout.txt'
            path.write_text('runtime chatter\n' + '\n'.join(json.dumps(value) for value in (
                {'stereo_game_projection_layers': 3, 'stereo_game_copies': 3},
                {'stereo_submission_evidence': True, 'sustained_submission_passed': False},
                {'stereo_projection_rejection': 'stale_ticket', 'count': 2})), encoding='utf-8')
            report = {}
            runner.collect_xr_evidence(path, report)
        self.assertEqual(report['stereo_game_summary']['stereo_game_projection_layers'], 3)
        self.assertEqual(report['stereo_projection_rejections'][0]['count'], 2)
        self.assertFalse(runner.passed(dict(report, native_exit_code=0, xr_exit_code=0), True))

    def test_snapshot_requires_stereo_game(self):
        with mock.patch.object(runner.subprocess, 'Popen') as spawn, contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as error:
                runner.main(['--pid', '123', '--gpu-snapshot'])
        self.assertEqual(error.exception.code, 2)
        spawn.assert_not_called()
        args = mock.Mock(stereo_game=True, gpu_snapshot=False, pid=123, xr_build_dir=pathlib.Path('xr'))
        self.assertEqual(runner.xr_command(args, pathlib.Path('out.csv'))[-2:], ['--stereo-game-pid', '123'])

    def test_stereo_game_dispatch_and_coverage(self):
        code, report, command, native, child = self.run_mock(extra_args=['--stereo-game'], projection=True)
        self.assertEqual(code, 0)
        self.assertIn('--stereo-game-pid', command)
        self.assertNotIn('--stereo-bridge-pid', command)
        self.assertIn('--stereo-gpu-publish', native.call_args.args[0])
        self.assertIn('--frame-link-diagnostics', native.call_args.args[0])
        self.assertTrue(report['stereo_coverage_passed'])

    def test_snapshot_does_not_replace_projection_coverage(self):
        code, report, command, *_ = self.run_mock(extra_args=['--stereo-game', '--gpu-snapshot'], snapshot=True)
        self.assertEqual(code, 1)
        self.assertTrue(report['gpu_snapshot_saved'])
        self.assertFalse(report['stereo_coverage_passed'])
        self.assertTrue(command[command.index('--gpu-snapshot') + 1].endswith('.csv-gpu.ppm'))
        code, report, *_ = self.run_mock(extra_args=['--stereo-game', '--gpu-snapshot'], projection=True)
        self.assertEqual(code, 1)
        self.assertFalse(report['gpu_snapshot_saved'])

    def test_live_pair_gate_ignores_old_or_mono_rows(self):
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder) / 'capture.csv'
            header = 'kind,test_state,family_count,qpc\n'
            for row in ['10,77,2,10\n', '10,0,1,20\n', '10,77,2,20']:
                path.write_text(header + row)
                self.assertFalse(runner.paired_render_ready(path, 10, {}))
            path.write_text(header + '10,77,2,20\n')
            evidence = {}
            self.assertTrue(runner.paired_render_ready(path, 10, evidence))
            self.assertEqual(evidence['qpc'], 20)

    def operator_fake(self, fail=False):
        class Operator:
            def __init__(self):
                self.events = []
                self.report = {'success': not fail}
            def connect_and_read_initial(self): self.events.append('initial')
            def start_motion(self, callback, wait_timeout):
                self.events.append('armed')
                self.callback = callback
                if callback(): raise AssertionError('Motion incorrectly allowed before native pair')
            def finish(self, timeout):
                self.events.append('finish')
                if fail: raise RuntimeError('Operator tool error')
            def close(self): self.events.append('close')
        return Operator()

    def test_operator_coordinates_motion_after_native_pair_and_cleans(self):
        operator = self.operator_fake()
        code, report, *_ = self.run_mock(extra_args=['--operator-motion'], operator=operator)
        self.assertEqual(code, 0)
        self.assertEqual(operator.events, ['initial', 'armed', 'native', 'motion', 'finish', 'close'])
        self.assertTrue(report['operator_motion_valid'])

    def test_operator_error_and_no_motion_fail_entire_trial(self):
        for failed_tool, moved in [(True, True), (False, False)]:
            with self.subTest(failed_tool=failed_tool, moved=moved):
                operator = self.operator_fake(failed_tool)
                code, report, *_ = self.run_mock(extra_args=['--operator-motion'], operator=operator, motion_passed=moved)
                self.assertEqual(code, 1)
                self.assertEqual(operator.events[-1], 'close')
                self.assertFalse(report['operator_motion_valid'])

    def test_motion_validation_uses_whole_window_even_when_endpoints_match(self):
        with tempfile.TemporaryDirectory() as folder:
            root = pathlib.Path(folder)
            xr, native, status = (root / name for name in ('xr.csv','native.csv','native.status.txt'))
            header = 'stereo_sequence,stereo_published,session_state,should_render,head_flags,view_flags,qx,qy,qz,qw,head_x\n'
            xr.write_text(header + ''.join(f'{i+1},1,5,1,15,15,0,{math.sin(math.radians(yaw/2))},0,{math.cos(math.radians(yaw/2))},{x}\n'
                for i,(yaw,x) in enumerate([(0,0),(15,.06),(-15,-.06),(0,0)])))
            status.write_text('stereo_xr first_sequence=1 last_sequence=4 accepted=4\n' + ''.join(
                f'stereo_stage stage={stage} count=4 mismatch=0 view_error_max=0\n'
                for stage in ('built','consume_before','prepare_before','prepare_after')))
            native_header = 'kind,test_state,family_count,valid,' + ','.join('matrix'+str(i) for i in range(16)) + '\n'
            def native_rows(moving):
                lines = []
                for kind in (10,12):
                    for delta in ([0,.2,-.2,0] if moving else [0,0,0,0]):
                        values = [0.]*16
                        values[0] = 1.+delta
                        values[12] = delta*10
                        lines.append(f'{kind},77,2,512,' + ','.join(map(str,values)) + '\n')
                return ''.join(lines)
            native.write_text(native_header + native_rows(True))
            evidence = runner.validate_operator_motion(xr,native,status)
            self.assertTrue(evidence['passed'])
            self.assertAlmostEqual(evidence['yaw_span_degrees'],30.)
            # A simulated source retains head3 and must never pass strict mode.
            data=xr.read_text().splitlines()
            xr.write_text(data[0]+',stereo_source\n'+''.join(line.replace(',15,15,',',3,15,')+',1\n' for line in data[1:]))
            self.assertTrue(runner.validate_operator_motion(xr,native,status,True)['passed'])
            self.assertFalse(runner.validate_operator_motion(xr,native,status)['passed'])
            native.write_text(native_header + native_rows(False))
            self.assertFalse(runner.validate_operator_motion(xr,native,status,True)['passed'])

    def test_120_second_dispatch_and_coverage_guard(self):
        code, report, command, native, child = self.run_mock(extra_args=['--stereo-game','--trial-seconds','120'], projection=True)
        self.assertEqual(code, 0)
        self.assertEqual(command[1], '150')
        self.assertEqual(command[command.index('--stereo-producer-seconds') + 1], '120')
        self.assertEqual(native.call_args.kwargs['timeout'], 270)
        self.assertEqual(child.wait.call_args.kwargs['timeout'], 155)
        self.assertEqual(native.call_args.args[0][-2:], ['--trial-seconds','120'])
        report['stereo_submission_evidence']['required_span_seconds'] = 29
        self.assertFalse(runner.passed(report, True))
        with mock.patch.object(runner.subprocess, 'Popen') as spawn, contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit): runner.main(['--pid','123','--trial-seconds','120'])
            spawn.assert_not_called()

    def test_simulator_operator_requires_explicit_motion_and_game(self):
        for flags in (['--simulator-operator'], ['--simulator-operator','--operator-motion'],
                      ['--simulator-operator','--stereo-game']):
            with self.subTest(flags=flags), mock.patch.object(runner.subprocess, 'Popen') as spawn, contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as error:
                    runner.main(['--pid','123',*flags])
                self.assertEqual(error.exception.code, 2)
                spawn.assert_not_called()
        operator = self.operator_fake()
        code, report, xr, native, _ = self.run_mock(extra_args=['--simulator-operator','--operator-motion','--stereo-game'],
                                                   operator=operator, projection=True)
        self.assertEqual(code, 0)
        self.assertIn('--simulator-operator', xr)
        self.assertIn('--simulator-operator', native.call_args.args[0])
        self.assertTrue(report['simulation_not_physical_tracking'])
        code, report, xr, native, _ = self.run_mock(extra_args=['--stereo-game','--trial-seconds','120'], projection=True)
        self.assertNotIn('--simulator-operator', xr)
        self.assertNotIn('--simulator-operator', native.call_args.args[0])
        self.assertFalse(report['simulation_not_physical_tracking'])

    def test_source_window_retains_raw_flags_and_rejects_mixed_provenance(self):
        with tempfile.TemporaryDirectory() as folder:
            root=pathlib.Path(folder)
            xr,native,status=(root/name for name in ('xr.csv','native.csv','status.txt'))
            def fixture(sim=True, xr_sources=('1','1'), native_sources=('1','1'), head='3', view='15', explicit=True):
                suffix=',stereo_source' if explicit else ''
                xr.write_text('stereo_sequence,stereo_published,session_state,should_render,head_flags,view_flags'+suffix+'\n'+''.join(
                    f'{i+1},1,5,1,{head},{view}'+(','+source if explicit else '')+'\n' for i,source in enumerate(xr_sources)))
                native.write_text('stereo_ticket,stereo_source,stereo_head_flags,stereo_view_flags\n'+''.join(
                    f'{i+1},{source},{head},{view}\n' for i,source in enumerate(native_sources)))
                status.write_text(f'stereo_xr first_sequence=1 last_sequence=2 simulator_operator={int(sim)}\n'+''.join(
                    f'stereo_sample edge={edge} source={int(sim)} head_flags={head} view_flags={view}\n' for edge in ('first','last')))
            fixture()
            result=runner.validate_stereo_sources(xr,native,status,True)
            self.assertTrue(result['passed'])
            self.assertTrue(result['simulation_not_physical_tracking'])
            self.assertFalse(runner.validate_stereo_sources(xr,native,status)['passed'])
            for change in ({'xr_sources':('1','0')},{'native_sources':('1','0')},
                           {'head':'1'},{'view':'3'},{'explicit':False},{'xr_sources':('1','2')}):
                fixture(**change)
                self.assertFalse(runner.validate_stereo_sources(xr,native,status,True)['passed'],change)
            fixture(sim=False,xr_sources=('0','0'),native_sources=('0','0'),head='15',explicit=False)
            self.assertTrue(runner.validate_stereo_sources(xr,native,status)['passed'])
            fixture(sim=False,xr_sources=('0','0'),native_sources=('0','0'),head='3')
            self.assertFalse(runner.validate_stereo_sources(xr,native,status)['passed'])
            fixture()
            status.write_text(status.read_text().replace('simulator_operator=1','simulator_operator=0'))
            self.assertFalse(runner.validate_stereo_sources(xr,native,status,True)['passed'])


if __name__ == '__main__':
    unittest.main()
