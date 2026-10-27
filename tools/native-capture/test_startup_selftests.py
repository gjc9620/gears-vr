"""Synthetic artifacts and mocked test runners only; no game/native execution."""
import copy
import hashlib
import pathlib
import tempfile
import unittest
from types import SimpleNamespace
from unittest import mock
import startup_selftests as tests


class StartupSelftestTests(unittest.TestCase):
    def test_activation_accepts_only_complete_handshake_and_never_falls_back(self):
        import start_capture as start
        import run_stereo_continuous as runner
        import sys
        for scenario in ('valid','digest','partial','artifact'):
            with self.subTest(scenario=scenario),tempfile.TemporaryDirectory() as directory:
                root=pathlib.Path(directory);dll=self.fixture(root);owner=dict(pid=7,creation_time=8)
                game=dict(pid=55,creation_time=456,image='game.exe');nonce='a'*32
                proof=tests.make_proof(dll,tests.continuous_suite(),tests.artifacts(dll,tests.continuous_suite()),owner,nonce)
                proof['game_identity']=game;raw=tests.encoded(proof);path=root/'once.json';path.write_bytes(raw)
                args=SimpleNamespace(pid=55,build_dir=root,dll_name=dll.name,stereo_units_per_metre=100)
                command=runner.native_command(args,owner)[1:]+['--startup-selftest-proof',str(path),'--startup-selftest-nonce',nonce]
                if scenario!='partial':command+=['--startup-selftest-digest','0'*64 if scenario=='digest' else hashlib.sha256(raw).hexdigest()]
                if scenario=='artifact':dll.write_bytes(b'changed')
                with mock.patch.object(sys,'argv',command),mock.patch.object(tests,'run_suite') as run,\
                     mock.patch.object(start,'preparation_live_guard',return_value=lambda:None),\
                     mock.patch.object(runner,'WindowsControl',return_value=SimpleNamespace(identity=lambda pid:game)),\
                     mock.patch.object(start,'resolve_camera_once',side_effect=AssertionError('No preliminary startup resolve')) as resolve,\
                     mock.patch.object(start,'k') as native:
                    native.OpenProcess.side_effect=RuntimeError('stop before game read')
                    with self.assertRaises(RuntimeError):start.main()
                    run.assert_not_called();resolve.assert_not_called();native.VirtualAllocEx.assert_not_called()
                    if scenario=='valid':native.OpenProcess.assert_called_once_with(0x43a,False,55);self.assertFalse(path.exists())
                    else:native.OpenProcess.assert_not_called();self.assertTrue(path.exists())

    def test_reservation_executes_union_once_before_any_game_launch(self):
        import launch_reserve as reserve
        import sys
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory);dll=self.fixture(root);calls=[]
            def run(command,**options):
                if '-Command' in command:raise RuntimeError('stop after prelaunch tests')
                self.assertNotIn('-File',command)
                calls.append((pathlib.Path(command[0]).name,tuple('@dll' if pathlib.Path(value).name==dll.name else value for value in command[1:])))
            argv=['launch_reserve','--build-dir',str(root),'--dll-name',dll.name,'--prime-targets',
                '--prime-owner-pid','7','--prime-owner-creation-time','8','--check-shadow-state','--check-mask-control',
                '--check-frame-link','--startup-preflight-nonce','a'*32]
            with mock.patch.object(sys,'argv',argv),mock.patch.object(reserve,'validate_game',return_value=root/'GOWDE-Steam.exe'),\
                 mock.patch.object(reserve,'run_hidden',side_effect=run),mock.patch.object(reserve,'k') as native,\
                 mock.patch.object(reserve,'WindowsControl',return_value=SimpleNamespace(identity=lambda pid:dict(pid=7,creation_time=8))):
                with self.assertRaisesRegex(RuntimeError,'stop after prelaunch'):reserve.main()
                native.OpenProcess.assert_not_called();native.VirtualAllocEx.assert_not_called()
            for name,arguments,_ in tests.continuous_suite():self.assertEqual(calls.count((name,tuple(arguments))),1)
            self.assertEqual(len(calls),len(set(calls)))
            self.assertIn(('CaptureSelfTest.exe',('@dll','--prime-owned-handoff')),calls)

    def fixture(self,root):
        dll=root/'Capture.dll';dll.write_bytes(b'dll')
        for name,_,_ in tests.continuous_suite():(root/name).write_bytes(name.encode())
        return dll

    def test_default_command_generates_the_exact_prelaunch_suite(self):
        import start_capture as start
        import run_stereo_continuous as runner
        import sys
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory);dll=self.fixture(root)
            args=SimpleNamespace(pid=55,build_dir=root,dll_name=dll.name,stereo_units_per_metre=100)
            command=runner.native_command(args,dict(pid=7,creation_time=8))
            with mock.patch.object(sys,'argv',command[1:]),mock.patch.object(tests,'run_suite',side_effect=RuntimeError('stop at suite')) as run:
                with self.assertRaisesRegex(RuntimeError,'stop at suite'):start.main()
            self.assertEqual(run.call_args.args[1],tests.continuous_suite())
            names={row[0] for row in run.call_args.args[1]}
            self.assertNotIn('TwoViewRendererShadowTest.exe',names)
            self.assertIn('SourceViewShadowTest.exe',names)

    def test_same_invocation_proof_consumes_once_and_rejects_tampering(self):
        owner=dict(pid=7,creation_time=8);game=dict(pid=55,creation_time=456,image='game.exe');nonce='a'*32
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory);dll=self.fixture(root);suite=tests.continuous_suite()
            hashes=tests.run_suite(dll,suite,mock.Mock())
            proof=tests.make_proof(dll,suite,hashes,owner,nonce);proof['game_identity']=game
            path=root/'once.json';raw=tests.encoded(proof);path.write_bytes(raw)
            expected=dict(dll=dll,owner=owner,game=game,nonce=nonce)
            self.assertEqual(tests.consume_proof(path,hashlib.sha256(raw).hexdigest(),**expected),proof)
            with self.assertRaises(FileNotFoundError):tests.consume_proof(path,hashlib.sha256(raw).hexdigest(),**expected)
            path.write_bytes(raw+b' ')
            with self.assertRaisesRegex(RuntimeError,'digest'):tests.consume_proof(path,hashlib.sha256(raw).hexdigest(),**expected)
            for changed in ('owner','game_identity','nonce','suite','suite_sha256','artifacts','partial'):
                value=copy.deepcopy(proof)
                if changed=='owner':value[changed]['creation_time']+=1
                elif changed=='game_identity':value[changed]['pid']+=1
                elif changed=='nonce':value[changed]='b'*32
                elif changed=='suite':value[changed].pop()
                elif changed=='suite_sha256':value[changed]='0'*64
                elif changed=='artifacts':value[changed].pop(next(iter(value[changed])))
                else:value={'schema':1}
                with self.subTest(changed=changed),self.assertRaises(RuntimeError):tests.validate_proof(value,**expected)
            dll.write_bytes(b'changed')
            with self.assertRaisesRegex(RuntimeError,'hashes'):tests.validate_proof(proof,**expected)

    def test_missing_or_changed_artifact_or_failed_test_cannot_pass(self):
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory);dll=self.fixture(root);suite=tests.continuous_suite()
            run=mock.Mock(side_effect=RuntimeError('test failed'))
            with self.assertRaisesRegex(RuntimeError,'test failed'):tests.run_suite(dll,suite,run)
            run.assert_called_once()
            def mutate(*args,**kwargs):dll.write_bytes(b'changed')
            with self.assertRaisesRegex(RuntimeError,'differs|changed during suite'):tests.run_suite(dll,suite,mutate)
            (root/'SubmittedViewLifetimeTest.exe').unlink();run=mock.Mock()
            with self.assertRaises(FileNotFoundError):tests.run_suite(dll,suite,run)
            run.assert_not_called()


if __name__=='__main__':unittest.main()
