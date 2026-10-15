"""CLI/config policy only, no native harness or process opened."""
import contextlib
import io
import pathlib
import sys
import types
import tempfile
import json
import unittest
from unittest import mock
import start_capture as start
import run_stereo_continuous as continuous


class AnchorStartTests(unittest.TestCase):
    def test_camera_transition_reacquires_all_certificates_without_activation(self):
        import anchor_config as anchor
        import ui_observer_config as ui
        resolvers=[];ticks=[0.];failures=mock.Mock()
        def resolver(pid,profile):
            r=types.SimpleNamespace(base=0x1000,p=types.SimpleNamespace(close=mock.Mock()),read=mock.Mock(return_value=b'ok'))
            r.resolve=mock.Mock(return_value=dict(local_player_address='0x1',module_base='0x1000',camera_address='camera'+str(len(resolvers)+1)))
            resolvers.append(r);return r
        def anchored(r,camera,*,prepared_camera=None):
            self.assertIs(prepared_camera,camera)
            self.assertIs(prepared_camera,r.resolve.return_value)
            if len(resolvers)<3:raise anchor.AnchorSamplingRejected('Camera view target is not controlled Pawn',{'stage':'initial'})
            return {'camera':camera['camera_address']},{'camera':camera['camera_address']}
        def pause(seconds):ticks[0]+=seconds
        baseline=dict(local_player_address='0x1',module_base='0x1000')
        with mock.patch.object(start,'Resolver',side_effect=resolver),mock.patch.object(anchor,'build_anchor_config',side_effect=anchored),\
             mock.patch.object(anchor,'anchor_ini',side_effect=lambda value:'anchor:'+value['camera']),\
             mock.patch.object(ui,'build_ui_observer_config',side_effect=lambda r,c:({'camera':c['camera_address']},{'camera':c['camera_address']})),\
             mock.patch.object(ui,'ui_observer_ini',side_effect=lambda value:'ui:'+value['camera']),\
             mock.patch.object(start,'save_anchor_sampling_failure') as saved,mock.patch.object(start,'remote_call') as remote,\
             mock.patch.object(start.subprocess,'run') as launched,contextlib.redirect_stdout(io.StringIO()):
            value=start.wait_camera_preparation(lambda:start.acquire_observer_preparation(1,{},baseline,{1:b'ok'},pathlib.Path('Unused.dll'),
                anchor_observer=True,ui_observer=True,record_anchor_failure=False),lambda:None,60,
                clock=lambda:ticks[0],sleep=pause,failure_receipt=failures)
            saved.assert_not_called();remote.assert_not_called();launched.assert_not_called()
        self.assertEqual((value[0]['camera_address'],value[1]['camera'],value[3]['camera']),('camera3',)*3)
        self.assertEqual((value[2],value[4]),('anchor:camera3','ui:camera3'))
        self.assertEqual(len(resolvers),3)
        for r in resolvers:r.p.close.assert_called_once();r.resolve.assert_called_once();r.read.assert_called_once()
        self.assertEqual([(call.args[1],call.args[2]) for call in failures.call_args_list],[(1,'first'),(2,'last')])

    def test_camera_wait_is_bounded_and_never_retries_hard_failures(self):
        ticks=[0.];acquire=mock.Mock(side_effect=RuntimeError('Camera view target is not controlled Pawn'))
        def pause(seconds):ticks[0]+=seconds
        receipts=mock.Mock();output=io.StringIO()
        with contextlib.redirect_stdout(output),self.assertRaisesRegex(TimeoutError,'deadline'):
            start.wait_camera_preparation(acquire,lambda:None,2,clock=lambda:ticks[0],sleep=pause,failure_receipt=receipts)
        self.assertEqual(acquire.call_count,4)
        self.assertEqual(receipts.call_count,2)
        self.assertEqual(receipts.call_args.args[1:],(4,'last'))
        self.assertEqual(len(output.getvalue().splitlines()),1)
        for reason in ('Target prologue differs; instrumentation rejected','Local player changed during preparation','Identity changed during certificate assembly'):
            acquire=mock.Mock(side_effect=RuntimeError(reason))
            with self.assertRaisesRegex(RuntimeError,reason):start.wait_camera_preparation(acquire,lambda:None,60,clock=lambda:0)
            acquire.assert_called_once()
        ticks[0]=0
        def late():ticks[0]=61;return 'late certificate'
        with self.assertRaisesRegex(TimeoutError,'after deadline'):
            start.wait_camera_preparation(late,lambda:None,60,clock=lambda:ticks[0])

    def test_camera_wait_cancellation_or_exit_prevents_another_attempt(self):
        for message in ('Owner cancelled camera preparation before activation','Game process identity changed during camera preparation'):
            acquire=mock.Mock(side_effect=RuntimeError('Camera view target is not controlled Pawn'))
            check=mock.Mock(side_effect=[None,RuntimeError(message)])
            with self.assertRaisesRegex(RuntimeError,message):start.wait_camera_preparation(acquire,check,60,clock=lambda:0)
            acquire.assert_called_once()
        acquire=mock.Mock()
        with self.assertRaisesRegex(RuntimeError,'cancelled'):
            start.wait_camera_preparation(acquire,mock.Mock(side_effect=RuntimeError('cancelled')),60,clock=lambda:0)
        acquire.assert_not_called()

    def test_preparation_live_guard_checks_identity_owner_and_existing_cancel_event(self):
        game={'pid':1,'creation_time':2};owner={'pid':3,'creation_time':4};live={1:game,3:owner}
        control=types.SimpleNamespace(identity=lambda pid:live.get(pid))
        with mock.patch.object(continuous,'WindowsControl',return_value=control),\
             mock.patch.object(start.k,'OpenEventW',return_value=9),mock.patch.object(start.k,'WaitForSingleObject',return_value=0x102) as waited,\
             mock.patch.object(start.k,'CloseHandle') as closed:
            check=start.preparation_live_guard(1,3,4)
            closed.assert_called_once_with(9)
            waited.return_value=0
            with self.assertRaisesRegex(RuntimeError,'cancelled'):check()
            waited.return_value=0x102;live[3]=dict(owner,creation_time=5)
            with self.assertRaisesRegex(RuntimeError,'Owner exited or changed'):check()
            live[3]=owner;live[1]=dict(game,creation_time=6)
            with self.assertRaisesRegex(RuntimeError,'Game process identity changed'):check()

    def test_hud_plane_dependencies_reject_before_option_mutation(self):
        parser=mock.Mock();parser.error.side_effect=ValueError
        base=dict(ui_hud_plane=True,ui_observer=True,anchor_observer=True,continuous_session=True,
                  stereo_xr=True,frame_link_diagnostics=True,family_diagnostics=False)
        for dependency in ('ui_observer','anchor_observer','continuous_session'):
            args=types.SimpleNamespace(**dict(base,**{dependency:False}))
            with self.subTest(dependency=dependency),self.assertRaises(ValueError):
                start.configure_anchor_options(args,parser)
            self.assertIn('HUD plane',parser.error.call_args.args[0])
            self.assertFalse(args.family_diagnostics)

    def test_hud_plane_cli_default_and_forwarding_without_native_launch(self):
        args=types.SimpleNamespace(pid=1,build_dir=pathlib.Path('missing-hud-plane-test-build'),
            dll_name='Missing.dll',stereo_units_per_metre=100,ui_observer=True,anchor_observer=True)
        for enabled in (False,True):
            args.ui_hud_plane=enabled
            argv=continuous.native_command(args,{'pid':100,'creation_time':1000})[2:]
            with mock.patch.object(sys,'argv',['start_capture.py',*argv]),mock.patch.object(start,'configure_anchor_options',wraps=start.configure_anchor_options) as configure,mock.patch.object(start.subprocess,'run') as launch:
                with self.assertRaisesRegex(RuntimeError,'Build capture DLL'):start.main()
                self.assertEqual(configure.call_args.args[0].ui_hud_plane,enabled)
                launch.assert_not_called()

    def test_hud_plane_alone_cli_rejects_before_write_or_launch(self):
        argv=['start_capture.py','--pid','1','--build-dir','missing','--dll-name','Missing.dll','--ui-hud-plane']
        with mock.patch.object(sys,'argv',argv),mock.patch.object(pathlib.Path,'write_text') as write,mock.patch.object(start.subprocess,'run') as launch,contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):start.main()
            write.assert_not_called();launch.assert_not_called()

    def test_origin_trial_default_off_and_each_explicit_dependency_required(self):
        parser=mock.Mock();parser.error.side_effect=ValueError
        args=types.SimpleNamespace(anchor_observer=False,stereo_xr=False,frame_link_diagnostics=False,family_diagnostics=False)
        start.configure_anchor_options(args,parser);parser.error.assert_not_called()
        base=dict(origin_trial=True,anchor_observer=True,continuous_session=True,stereo_gpu_publish=True,
                  stereo_xr=True,frame_link_diagnostics=True,family_diagnostics=False)
        for required in ('anchor_observer','continuous_session','stereo_gpu_publish','stereo_xr','frame_link_diagnostics'):
            with self.subTest(required=required):
                args=types.SimpleNamespace(**dict(base,**{required:False}))
                with self.assertRaises(ValueError):start.configure_anchor_options(args,parser)
        args=types.SimpleNamespace(**base);start.configure_anchor_options(args,parser)
        self.assertTrue(args.family_diagnostics)

    def test_origin_trial_cli_stays_off_unless_explicitly_forwarded(self):
        args=types.SimpleNamespace(pid=1,build_dir=pathlib.Path('missing-origin-cpu-test-build'),dll_name='Missing.dll',stereo_units_per_metre=100,anchor_observer=True)
        for enabled in (False,True):
            args.origin_trial=enabled
            argv=continuous.native_command(args,{'pid':100,'creation_time':1000})[2:]
            with mock.patch.object(sys,'argv',['start_capture.py',*argv]),mock.patch.object(start,'configure_anchor_options',wraps=start.configure_anchor_options) as configure,mock.patch.object(start.subprocess,'run',side_effect=AssertionError('No native process allowed')):
                with self.assertRaisesRegex(RuntimeError,'Build capture DLL'):start.main()
                self.assertEqual(configure.call_args.args[0].origin_trial,enabled)

    def test_failed_sample_receipt_is_unique_and_not_success_certificate(self):
        from anchor_config import AnchorSamplingRejected
        error=AnchorSamplingRejected('cross-frame',{'accepted':False,'attempts':[{'reason':'cross-frame'}]})
        with tempfile.TemporaryDirectory() as directory,mock.patch.object(start.time,'perf_counter_ns',side_effect=[123,124]):
            dll=pathlib.Path(directory)/'Native.dll'
            successful=dll.with_name('anchor-observer-config.json');successful.write_text('original')
            with contextlib.redirect_stderr(io.StringIO()):
                first=start.save_anchor_sampling_failure(dll,43296,error)
                second=start.save_anchor_sampling_failure(dll,43296,error)
            self.assertNotEqual(first,second);self.assertEqual(successful.read_text(),'original')
            report=json.loads(first.read_text());self.assertEqual(report['pid'],43296)
            self.assertFalse(report['accepted']);self.assertFalse(report['native_activation_attempted'])
            self.assertEqual(report['sample_acquisition']['attempts'][0]['reason'],'cross-frame')

    def test_default_preserves_options_and_missing_prerequisite_rejects(self):
        parser=mock.Mock();parser.error.side_effect=ValueError
        args=types.SimpleNamespace(anchor_observer=False,stereo_xr=False,frame_link_diagnostics=False,family_diagnostics=False)
        start.configure_anchor_options(args,parser)
        self.assertFalse(args.family_diagnostics);parser.error.assert_not_called()
        for xr,link in ((False,False),(True,False),(False,True)):
            args=types.SimpleNamespace(anchor_observer=True,stereo_xr=xr,frame_link_diagnostics=link,family_diagnostics=False)
            with self.assertRaises(ValueError):start.configure_anchor_options(args,parser)
        args.stereo_xr=args.frame_link_diagnostics=True
        start.configure_anchor_options(args,parser);self.assertTrue(args.family_diagnostics)

    def test_aliases_parse_and_enable_existing_family_without_launching(self):
        args=types.SimpleNamespace(pid=1,build_dir=pathlib.Path('missing-anchor-cpu-test-build'),dll_name='Missing.dll',stereo_units_per_metre=100,anchor_observer=True)
        owner={'pid':100,'creation_time':1000}
        argv=continuous.native_command(args,owner)[2:]
        for option in ('--anchor-observer','--anchor-diagnostics'):
            argsv=[option if x=='--anchor-observer' else x for x in argv]
            with mock.patch.object(sys,'argv',['start_capture.py',*argsv]),mock.patch.object(start,'configure_anchor_options',wraps=start.configure_anchor_options) as configure,mock.patch.object(start.subprocess,'run',side_effect=AssertionError('No native process allowed')):
                with self.assertRaisesRegex(RuntimeError,'Build capture DLL'):start.main()
                parsed=configure.call_args.args[0]
                self.assertTrue(parsed.anchor_observer);self.assertTrue(parsed.family_diagnostics)


if __name__=='__main__':unittest.main()
