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
