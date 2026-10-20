import hashlib
import importlib.util
import json
import pathlib
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest import mock

HERE=pathlib.Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('render_size_launcher',HERE/'launcher.py')
launcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(launcher)
sys.path.insert(0,str(HERE.parent/'native-capture'))
import run_stereo_continuous as runner

class RenderSizeTests(unittest.TestCase):
    def test_capability_and_scale(self):
        self.assertIsNone(launcher.render_scale({},{}))
        self.assertEqual(launcher.render_scale({},dict(runtime_render_size=True)),1.)
        for value in (True,None,'0.7',.249,2.001,float('inf'),float('nan')):
            with self.subTest(value=value),self.assertRaises(ValueError):launcher.render_scale(dict(render_scale=value),dict(runtime_render_size=True))
        with self.assertRaises(ValueError):launcher.render_scale(dict(render_scale=.7),{})
        with self.assertRaises(ValueError):launcher.render_scale({},dict(runtime_render_size='true'))
    def test_prelaunch_plan_is_authenticated_and_not_clamped(self):
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory);probe=root/'xr/OpenXRPoseProbe.exe';probe.parent.mkdir();probe.write_bytes(b'test')
            manifest=dict(xr_build='xr',xr_sha256=hashlib.sha256(b'test').hexdigest())
            plan=dict(runtime_render_size_plan=True,render_scale=.7,recommended=[3072,3264],eye=[2151,2285],source=[4302,2285])
            with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=json.dumps(plan),stderr='')) as run:
                self.assertEqual(launcher.render_size_info(manifest,.7,root),plan)
                self.assertEqual(run.call_args.args[0][-2:],['--render-scale','0.7'])
            for wrong in (dict(plan,source=[3840,2160]),dict(plan,eye=[1920,2160]),dict(plan,render_scale=1)):
                with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=json.dumps(wrong),stderr='')):
                    with self.assertRaises(RuntimeError):launcher.render_size_info(manifest,.7,root)
            with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=1,stdout='',stderr='exceeds 8192')):
                with self.assertRaisesRegex(RuntimeError,'before game launch.*8192'):launcher.render_size_info(manifest,1.5,root)
    def test_runner_forwards_only_to_xr(self):
        args=SimpleNamespace(pid=1,xr_build_dir=pathlib.Path('xr'),render_scale=.7)
        command=runner.xr_command(args,pathlib.Path('sample.csv'),dict(pid=2,creation_time=3))
        self.assertEqual(command[command.index('--render-scale')+1],'0.7')
        for value in (True,.2,3,float('nan')):
            args.render_scale=value
            with self.assertRaises(ValueError):runner.xr_command(args,pathlib.Path('sample.csv'),dict(pid=2,creation_time=3))

    def test_utf8_loader_failure_survives_windows_locale(self):
        real_run=launcher.subprocess.run
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory);probe=root/'xr/OpenXRPoseProbe.exe';probe.parent.mkdir();probe.write_bytes(b'test')
            manifest=dict(xr_build='xr',xr_sha256=hashlib.sha256(b'test').hexdigest())
            message='Runtime DLL not found: 系统找不到指定的文件。'
            # Exercise actual pipe decoding, including invalid trailing bytes,
            # without loading OpenXR or creating a window.
            payload=(message.encode('utf-8')+b'\xff').hex()
            def child(command,**kwargs):
                return real_run([sys.executable,'-c',
                    "import sys;sys.stderr.buffer.write(bytes.fromhex('"+payload+"'));sys.exit(1)"],**kwargs)
            with mock.patch.object(launcher.subprocess,'run',side_effect=child):
                with self.assertRaises(RuntimeError) as caught:launcher.render_size_info(manifest,1,root)
            self.assertIn(message,str(caught.exception))
            self.assertIn('before game launch',str(caught.exception))

if __name__=='__main__':unittest.main()
