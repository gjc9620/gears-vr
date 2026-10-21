import importlib.util
import contextlib
import io
import json
import os
import pathlib
import struct
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest import mock

SPEC=importlib.util.spec_from_file_location('vr_launcher',pathlib.Path(__file__).with_name('launcher.py'))
launcher=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(launcher)
# Legacy fixtures intentionally exercise omitted options. The player default
# enables startup menus and graphics protection and is tested separately below.
MANIFEST=json.loads(pathlib.Path(__file__).with_name('manifest-legacy-continuous3.json').read_text())

class FakeResolver:
    def __init__(self):
        self.profile={'field_super':16};self.object_pointers=[100,200,300]
        self.objects={100:dict(name='Controller',cls=1000,outer=300,internal_index=0),
            200:dict(name='Pawn_0',cls=2000,outer=300,internal_index=1),
            300:dict(name='PersistentLevel',cls=3000,outer=0,internal_index=2),
            1000:dict(name='WarPlayerController'),2000:dict(name='WarPawn'),3000:dict(name='Level')}
        self.pointers={108:200,208:100};self.stamp=1.
    def identity(self,address):return self.objects[address]
    def property(self,owner,name,inherited):return dict(type='ObjectProperty',offset=8)
    def u64(self,address):return self.pointers.get(address,0)
    def class_name(self,obj):return self.objects[obj['cls']]['name']
    def read(self,address,size):return struct.pack('<f',self.stamp)

class FakeBackend:
    def __init__(self,existing=False,cancel=False,ready=True):
        self.owner=dict(pid=os.getpid(),creation_time=123,image='python.exe')
        self.game=dict(pid=55,creation_time=456,image=str(launcher.rooted(MANIFEST['game_path'])))
        self.control=SimpleNamespace(identity=lambda pid:self.owner if pid==os.getpid() else self.game,
            k=SimpleNamespace(CreateMutexW=lambda *a:1,CloseHandle=mock.Mock()))
        self.runner=SimpleNamespace(run_session=mock.Mock(return_value=0))
        self.start_tiny_output_monitor=mock.Mock(return_value=mock.Mock())
        self.existing=existing;self.cancel=cancel;self.ready=ready;self.reserve_calls=0;self.samples=0
    def game_pids(self):return [55] if self.existing else []
    def open_cancel(self,pid):return 2
    def cancelled(self,handle):return self.cancel
    def reserve(self,manifest,log,owner=None,game_path=None,foreground=False):
        self.reserve_calls+=1;self.prime_owner=owner;self.selected_path=game_path;self.foreground=foreground
        return {'pid':55,'game_identity':dict(self.game)}
    def game_window(self,pid,*,diagnostic=None):return 12345
    def finish_owned_prime(self,manifest,identity,owner):return {'entry_restore_verified':True}
    def player(self,pid,profile):
        if not self.ready:raise RuntimeError('Menu has no Pawn')
        self.samples+=1
        return dict(pawn='0x200',controller='0x100',camera='0x400',level='0x300',timestamp=float(self.samples))

class LauncherTests(unittest.TestCase):
    def setUp(self):
        environment=mock.patch.dict(os.environ,{'GEARSVR_TEST_MUTE_GAME':''})
        environment.start();self.addCleanup(environment.stop)
        import game_audio_guard
        manual=mock.patch.object(game_audio_guard,'begin_manual',return_value=None)
        manual.start();self.addCleanup(manual.stop)

    def test_manual_audio_only_for_foreground_with_test_mute_precedence(self):
        import game_audio_guard
        for foreground,mute,expect_manual,expect_mute in ((False,'',False,False),(True,'',True,False),(True,'1',False,True)):
            with self.subTest(foreground=foreground,mute=mute),tempfile.TemporaryDirectory() as directory:
                backend=FakeBackend();guard=mock.Mock();guard.log=pathlib.Path(directory)/'audio.jsonl'
                guard.close.return_value={'configured':True}
                with mock.patch.dict(os.environ,GEARSVR_TEST_MUTE_GAME=mute),\
                     mock.patch.object(game_audio_guard,'begin_manual',return_value=guard) as manual,\
                     mock.patch.object(game_audio_guard,'begin',return_value=guard) as muted,\
                     mock.patch.object(launcher,'verify_files',return_value={}),\
                     mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                    launcher.start({},dict(MANIFEST,startup_menu=True),backend,pathlib.Path(directory)/'index.json',sleep=lambda _:None,foreground=foreground)
                self.assertEqual(manual.called,expect_manual);self.assertEqual(muted.called,expect_mute)
                if expect_manual:
                    self.assertEqual(manual.call_args.args[:2],(backend.game,backend.owner))
                self.assertEqual(guard.close.call_count,int(expect_manual or expect_mute))

    def test_test_audio_restores_before_prime_cleanup_without_masking_original_cleanup(self):
        import game_audio_guard
        for scenario in ('normal','cancel','exception','audio_close_failure'):
            with self.subTest(scenario=scenario),tempfile.TemporaryDirectory() as directory:
                backend=FakeBackend(cancel=scenario=='cancel');order=[]
                guard=mock.Mock();guard.log=pathlib.Path(directory)/'audio.jsonl'
                def restore():
                    order.append('audio')
                    if scenario=='audio_close_failure':raise RuntimeError('audio cleanup fixture')
                    return {'restored':True}
                guard.close.side_effect=restore
                backend.finish_owned_prime=lambda *args:order.append('prime') or {'entry_restore_verified':True}
                if scenario=='exception':backend.runner.run_session.side_effect=RuntimeError('session failed')
                with mock.patch.dict(os.environ,GEARSVR_TEST_MUTE_GAME='1'),\
                     mock.patch.object(game_audio_guard,'begin',return_value=guard) as begin,\
                     mock.patch.object(launcher,'verify_files',return_value={}),\
                     mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                    if scenario=='exception':
                        with self.assertRaisesRegex(RuntimeError,'session failed'):
                            launcher.start({},dict(MANIFEST,startup_menu=True),backend,pathlib.Path(directory)/'index.json',sleep=lambda _:None)
                    else:launcher.start({},dict(MANIFEST,startup_menu=True),backend,pathlib.Path(directory)/'index.json',sleep=lambda _:None)
                self.assertEqual(order,['audio','prime'])
                self.assertEqual(begin.call_args.args[:2],(backend.game,backend.owner))
                guard.close.assert_called_once()

    def test_path_choice_center_requires_explicit_supported_observers(self):
        guards=dict(MANIFEST,path_choice_center=True,menu_recovery=True,startup_menu=True,ui_observer=True,anchor_observer=True)
        launcher.settings({},guards)
        for key in ('menu_recovery','ui_observer','anchor_observer'):
            with self.subTest(key=key),self.assertRaisesRegex(ValueError,'PathChoice'):
                launcher.settings({},dict(guards,**{key:False}))
        with self.assertRaisesRegex(ValueError,'boolean'):
            launcher.settings({},dict(guards,path_choice_center='true'))

    def test_prelaunch_selftest_option_requires_boolean_and_owned_startup(self):
        for value in (1,'true',None):
            with self.assertRaisesRegex(ValueError,'boolean'):
                launcher.settings({},dict(MANIFEST,startup_selftests_prelaunch=value))
        with self.assertRaisesRegex(ValueError,'owned startup'):
            launcher.settings({},dict(MANIFEST,startup_selftests_prelaunch=True,startup_menu=False))

    def test_owned_prelaunch_handshake_requires_current_successful_reservation(self):
        sys.path.insert(0,str(launcher.ROOT/'tools/native-capture'))
        import startup_selftests as tests
        import run_stereo_continuous as runner
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory);dll=root/'Fixture.dll';dll.write_bytes(b'dll')
            for name,_,_ in tests.continuous_suite():(root/name).write_bytes(name.encode())
            owner=dict(pid=7,creation_time=8)
            manifest=dict(MANIFEST,startup_menu=True,startup_selftests_prelaunch=True,native_build=str(root),native_dll=dll.name,
                          native_sha256=tests.digest(dll))
            game=dict(pid=55,creation_time=456,image=str(launcher.rooted(MANIFEST['game_path'])))
            backend=launcher.Backend.__new__(launcher.Backend)
            for scenario in ('valid','nonce','owner','game','failed'):
                log=root/(scenario+'.log')
                def reserve(command,**kwargs):
                    nonce=command[command.index('--startup-preflight-nonce')+1]
                    proof=tests.make_proof(dll,tests.continuous_suite(),tests.artifacts(dll,tests.continuous_suite()),owner,nonce)
                    proof['game_identity']=game.copy()
                    if scenario=='nonce':proof['nonce']='b'*32
                    if scenario=='owner':proof['owner']['creation_time']+=1
                    if scenario=='game':proof['game_identity']['pid']+=1
                    report=dict(pid=55,game_identity=game,game_sha256=MANIFEST['game_sha256'],trampolines_reserved=True,
                        hooks_enabled=True,native_hooks_enabled=False,first_view_mask_reserved=True,independent_entry_checks=[{'matches':True}],
                        prime_lifetime='owner',prime_owner_pid=7,prime_owner_creation_time=8,startup_selftest_proof=proof)
                    return SimpleNamespace(returncode=1 if scenario=='failed' else 0,stdout=json.dumps(report),stderr='')
                with mock.patch.object(launcher.subprocess,'run',side_effect=reserve):
                    if scenario!='valid':
                        with self.assertRaises(RuntimeError):backend.reserve(manifest,log,owner=owner)
                        self.assertFalse(list(root.glob(log.name+'.*.selftests.json')))
                    else:
                        accepted=backend.reserve(manifest,log,owner=owner);handshake=accepted['startup_selftest_handshake']
                        self.assertTrue(pathlib.Path(handshake['path']).exists())
                        args=SimpleNamespace(pid=55,build_dir=root,dll_name=dll.name,stereo_units_per_metre=100,
                            **{'startup_selftest_'+key:handshake[key] for key in ('digest','nonce')},startup_selftest_proof=handshake['path'])
                        command=runner.native_command(args,owner)
                        for key in ('proof','digest','nonce'):self.assertIn('--startup-selftest-'+key,command)

    def test_camera_distance_capability_and_value(self):
        manifest={'camera_distance_control':True,'anchor_observer':True,'cinematic_camera':True}
        self.assertEqual(launcher.camera_closer({},{}),0)
        for value in (0,.3,.5):self.assertEqual(launcher.camera_closer({'camera_closer_metres':value},manifest),value)
        for value in (True,None,'0.3',-.01,.51,float('nan'),float('inf')):
            with self.subTest(value=value),self.assertRaises(ValueError):launcher.camera_closer({'camera_closer_metres':value},manifest)
        for key in manifest:
            broken={**manifest,key:False}
            with self.subTest(key=key),self.assertRaises(ValueError):launcher.camera_closer({'camera_closer_metres':.3},broken)
        with self.assertRaises(ValueError):launcher.camera_closer({},dict(camera_distance_control=1))

    def test_failed_reserve_closes_only_exact_uninjected_birth(self):
        with tempfile.TemporaryDirectory() as directory:
            selected=pathlib.Path(directory)/'GOWDE-Steam.exe';receipt=pathlib.Path(directory)/'birth.json'
            identity=dict(pid=55,creation_time=456,image=str(selected.resolve()).casefold())
            backend=launcher.Backend.__new__(launcher.Backend)
            backend.game_windows=mock.Mock(return_value=[123]);backend.post_normal_close=mock.Mock()
            for scenario in ('exited','reused','injected','prime','unknown','wrong_path','late_window','timeout'):
                with self.subTest(scenario=scenario):
                    ticks=[0.];value=dict(schema=1,kind='reservation_birth',game_identity=identity,
                                        injection_attempted=False,prime_attempted=False)
                    if scenario=='injected':value['injection_attempted']=True
                    if scenario=='prime':value['prime_attempted']=True
                    if scenario=='unknown':del value['injection_attempted']
                    if scenario=='wrong_path':value['game_identity']=dict(identity,image='C:/other/game.exe')
                    receipt.write_text(json.dumps(value))
                    actual=dict(identity,creation_time=789) if scenario=='reused' else identity
                    backend.control=SimpleNamespace(identity=mock.Mock(return_value=actual))
                    if scenario in ('exited','late_window'):
                        backend.control.identity.side_effect=[identity,identity,None] if scenario=='exited' else [identity,identity,identity,None]
                    backend.game_windows.reset_mock();backend.game_windows.side_effect=None
                    backend.game_windows.return_value=[123]
                    if scenario=='late_window':backend.game_windows.side_effect=[[],[123]]
                    backend.post_normal_close.reset_mock()
                    result=backend.close_uninjected_reservation(receipt,selected.resolve(),clock=lambda:ticks[0],
                        sleep=lambda duration:ticks.__setitem__(0,ticks[0]+duration))
                    expected=('exited' if scenario in ('exited','late_window') else 'identity_changed' if scenario=='reused'
                        else 'birth_identity_rejected' if scenario=='wrong_path' else 'close_timeout' if scenario=='timeout'
                        else 'injection_state_unknown_or_attempted')
                    self.assertEqual(result['status'],expected)
                    if scenario in ('exited','late_window','timeout'):
                        backend.post_normal_close.assert_called_once_with(123,identity)
                    else:backend.post_normal_close.assert_not_called()

    def test_reservation_failure_and_timeout_use_unique_birth_receipt(self):
        backend=launcher.Backend.__new__(launcher.Backend)
        backend.close_uninjected_reservation=mock.Mock(return_value={'status':'exited'})
        with tempfile.TemporaryDirectory() as directory:
            log=pathlib.Path(directory)/'reserve.log';paths=[]
            for timeout in (False,True):
                def fail(command,**kwargs):
                    receipt=pathlib.Path(command[command.index('--launch-receipt')+1]);paths.append(receipt)
                    self.assertFalse(receipt.exists());self.assertEqual(kwargs['timeout'],120)
                    if timeout:raise launcher.subprocess.TimeoutExpired(command,120,output=b'partial',stderr=b'failed')
                    return SimpleNamespace(returncode=1,stdout='',stderr='loader not ready')
                with mock.patch.object(launcher.subprocess,'run',side_effect=fail),self.assertRaises(Exception):
                    backend.reserve(MANIFEST,log)
                backend.close_uninjected_reservation.assert_called_with(paths[-1],launcher.rooted(MANIFEST['game_path']))
                self.assertIn('reservation_failure_cleanup',log.read_text())
            self.assertNotEqual(paths[0],paths[1])

    def test_cli_foreground_reaches_start(self):
        for foreground in (False,True):
            argv=['launcher.py','start']+(['--foreground'] if foreground else [])
            with self.subTest(foreground=foreground),mock.patch.object(launcher.sys,'argv',argv),\
                    mock.patch.object(launcher,'Backend',return_value=object()),\
                    mock.patch.object(launcher.pathlib.Path,'read_text',return_value='{}'),\
                    mock.patch.object(launcher,'start',return_value=0) as start:
                self.assertEqual(launcher.main(),0)
                self.assertIs(start.call_args.kwargs['foreground'],foreground)

    def test_cli_export_logs_is_independent_of_running_or_stopped_backend(self):
        import diagnostics_bundle
        for phase in ('running','stopped'):
            for overwrite in (False,True):
                with self.subTest(phase=phase,overwrite=overwrite),tempfile.TemporaryDirectory() as directory:
                    root=pathlib.Path(directory);index=root/'index.json';output=root/'feedback with spaces.zip'
                    index.write_text(json.dumps({'phase':phase,'session_report':'untrusted-do-not-follow'}))
                    receipt={'diagnostics_exported':True,'output':str(output),'files':7,'warnings':['partial history']}
                    stdout,stderr=io.StringIO(),io.StringIO()
                    argv=['launcher.py','export-logs','--output',str(output)]+(['--overwrite'] if overwrite else [])
                    with mock.patch.object(launcher.sys,'argv',argv),mock.patch.object(launcher,'ROOT',root),\
                            mock.patch.object(launcher,'INDEX',index),\
                            mock.patch.object(launcher,'Backend',side_effect=AssertionError('no control backend')) as backend,\
                            mock.patch.object(diagnostics_bundle,'export_bundle',return_value=receipt) as export,\
                            contextlib.redirect_stdout(stdout),contextlib.redirect_stderr(stderr):
                        self.assertEqual(launcher.main(),0)
                    backend.assert_not_called()
                    export.assert_called_once_with(root,output,overwrite=overwrite)
                    self.assertEqual(json.loads(stdout.getvalue()),receipt)
                    self.assertEqual(len(stdout.getvalue().splitlines()),1)
                    self.assertEqual(stderr.getvalue(),'')
                    self.assertEqual(json.loads(index.read_text())['phase'],phase)

    def test_cli_export_logs_requires_output_before_backend_or_collector(self):
        import diagnostics_bundle
        with mock.patch.object(launcher.sys,'argv',['launcher.py','export-logs']),\
                mock.patch.object(launcher,'Backend') as backend,\
                mock.patch.object(diagnostics_bundle,'export_bundle') as export,\
                contextlib.redirect_stderr(io.StringIO()) as stderr:
            with self.assertRaises(SystemExit) as error:launcher.main()
        self.assertEqual(error.exception.code,2)
        self.assertIn('requires --output',stderr.getvalue())
        backend.assert_not_called();export.assert_not_called()

    def test_cli_export_logs_fatal_error_is_stderr_only_and_nonzero(self):
        import diagnostics_bundle
        for error in (FileExistsError('output already exists'),OSError('disk full')):
            with self.subTest(error=type(error).__name__),\
                    mock.patch.object(launcher.sys,'argv',['launcher.py','export-logs','--output','feedback.zip']),\
                    mock.patch.object(launcher,'Backend') as backend,\
                    mock.patch.object(diagnostics_bundle,'export_bundle',side_effect=error),\
                    contextlib.redirect_stdout(io.StringIO()) as stdout,contextlib.redirect_stderr(io.StringIO()) as stderr:
                self.assertEqual(launcher.main(),1)
            self.assertEqual(stdout.getvalue(),'')
            self.assertIn(str(error),stderr.getvalue())
            backend.assert_not_called()

    def test_existing_control_commands_do_not_require_export_output(self):
        for command in ('status','stop','recenter'):
            with self.subTest(command=command),mock.patch.object(launcher.sys,'argv',['launcher.py',command]),\
                    mock.patch.object(launcher,'Backend',return_value=object()) as backend,\
                    mock.patch.object(launcher,command,return_value=0) as dispatch:
                self.assertEqual(launcher.main(),0)
            backend.assert_called_once_with()
            dispatch.assert_called_once_with(backend.return_value)

    def test_foreground_reaches_owned_and_legacy_reservation(self):
        for owned in (False,True):
            for foreground in (False,True):
                with self.subTest(owned=owned,foreground=foreground),tempfile.TemporaryDirectory() as directory:
                    backend=FakeBackend(cancel=True)
                    with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                        launcher.start({},dict(MANIFEST,startup_menu=owned),backend,pathlib.Path(directory)/'index.json',foreground=foreground)
                    self.assertIs(backend.foreground,foreground)

    def test_debug_command_foreground_preserves_startup_policy_script(self):
        launcher.sys.path.insert(0,str(launcher.ROOT/'tools/debug-launch'))
        from game_install import debug_command
        exe=launcher.rooted(MANIFEST['game_path'])
        quiet=debug_command(exe)
        self.assertIn(str(launcher.ROOT/'tools/debug-launch/Start-DebugGame.ps1'),quiet)
        self.assertNotIn('-Foreground',quiet)
        self.assertEqual(debug_command(exe,foreground=True),quiet+['-Foreground'])

    def test_backend_player_requires_full_anchor_guard_and_closes_reader(self):
        backend=launcher.Backend.__new__(launcher.Backend)
        resolver=SimpleNamespace(p=SimpleNamespace(close=mock.Mock()),resolve=mock.Mock())
        backend.Resolver=mock.Mock(return_value=resolver)
        guard={role:{'address':address} for role,address in
               (('pawn',200),('controller',100),('camera',400),('local_player',500),('level',300))}
        guard.update(outer_chain=[{'address':300}],view_target=200,timestamp_raw=struct.pack('<f',2.).hex())
        sampler=SimpleNamespace(view_target=408,initial=guard,resolved=dict(controller_address='0x64',
            camera_address='0x190',local_player_address='0x1f4'),guard=mock.Mock(return_value=dict(guard)))
        factory=mock.Mock(return_value=sampler)
        with mock.patch.dict('sys.modules',{'anchor_config':SimpleNamespace(LiveRegistryAnchorSampler=factory)}):
            self.assertEqual(backend.player(55,{}),dict(pawn='0xc8',controller='0x64',camera='0x190',level='0x12c',timestamp=2.))
            factory.assert_called_once_with(resolver);sampler.guard.assert_called_once()
            resolver.resolve.assert_not_called();resolver.p.close.assert_called_once()
            for changed in ('camera','view_target'):
                sampler.guard.return_value=dict(guard,**{changed:({'address':401} if changed=='camera' else 201)})
                with self.assertRaisesRegex(RuntimeError,'relationship changed'):backend.player(55,{})
            sampler.view_target=None
            with self.assertRaisesRegex(RuntimeError,'readable Camera.ViewTarget'):backend.player(55,{})
            factory.side_effect=RuntimeError('Camera view target is not controlled Pawn')
            with self.assertRaisesRegex(RuntimeError,'not controlled Pawn'):backend.player(55,{})
            self.assertEqual(resolver.p.close.call_count,5)

    def test_owned_waits_for_camera_target_then_two_fresh_samples(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json';observations=[]
            original=backend.player;attempts=[0]
            def player(*args):
                attempts[0]+=1
                if attempts[0]<=2:raise RuntimeError('Camera view target is not controlled Pawn')
                return original(*args)
            backend.player=player
            def run(args,control,**kwargs):
                kwargs['before_native_activation'](lambda:None)
                self.assertEqual(attempts[0],4)
                self.assertNotIn('waiting_for_level_reason',json.loads(index.read_text()))
                return 0
            backend.runner.run_session=mock.Mock(side_effect=run)
            def pause(_):observations.append(json.loads(index.read_text())['waiting_for_level_reason'])
            with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0),mock.patch('builtins.print') as printed:
                launcher.start({},dict(MANIFEST,startup_menu=True),backend,index,clock=lambda:999.,sleep=pause)
            self.assertEqual(observations[:2],['Camera view target is not controlled Pawn']*2)
            reasons=[json.loads(call.args[0]) for call in printed.call_args_list]
            self.assertEqual(sum(row.get('waiting_for_level')=='Camera view target is not controlled Pawn' for row in reasons),1)

    def test_owned_camera_wait_can_be_cancelled_without_activation(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json';activated=[]
            backend.player=mock.Mock(side_effect=RuntimeError('Camera/controller/local-player relationship changed'))
            class Cancelled(RuntimeError):pass
            backend.runner.StartupCancelled=Cancelled
            def run(args,control,**kwargs):
                kwargs['before_native_activation'](lambda:None);activated.append(True);return 0
            backend.runner.run_session=mock.Mock(side_effect=run)
            def pause(_):
                self.assertIn('relationship changed',json.loads(index.read_text())['waiting_for_level_reason'])
                backend.cancel=True
            with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                with self.assertRaises(Cancelled):
                    launcher.start({},dict(MANIFEST,startup_menu=True),backend,index,clock=lambda:999.,sleep=pause)
            self.assertEqual(activated,[]);backend.player.assert_called_once()

    def test_player_default_uses_runtime_resolution_and_graphics_protection(self):
        directory=pathlib.Path(__file__).parent
        current=json.loads((directory/'manifest.json').read_text())
        candidates=[json.loads(path.read_text()) for path in directory.glob('manifest-*.json')]
        self.assertIn(current,candidates,'Default must exactly match a named, reviewable candidate including artifact hashes')
        launcher.settings({},current)
        self.assertIs(current.get('runtime_render_size'),True,
                      '0.0.1 must use runtime-recommended eye size, not game resolution')
        self.assertEqual(launcher.render_scale({},current),1.0)
        for scale in (.25,.7,1.,2.):
            launcher.settings({'render_scale':scale},current)
            self.assertEqual(launcher.render_scale({'render_scale':scale},current),scale)
        self.assertTrue(current['native_dll'].startswith('GearsCapture'))
        self.assertTrue(current['native_dll'].endswith('.dll'))
        self.assertEqual(pathlib.PurePosixPath(current['native_dll']).name,current['native_dll'])
        self.assertTrue(current['native_build'].startswith('tools/native-capture/build-'))
        self.assertNotIn('..',pathlib.PurePosixPath(current['native_build']).parts)
        for key in ('game_sha256','native_sha256','xr_sha256'):
            self.assertRegex(current[key],r'^[0-9a-f]{64}$')
            self.assertNotEqual(current[key],'0'*64)
        self.assertTrue(current['xr_build'].startswith('tools/openxr-probe/build-'))
        self.assertNotIn('..',pathlib.PurePosixPath(current['xr_build']).parts)
        fallback=json.loads((directory/'manifest-material32-movie23-vd.json').read_text())
        self.assertEqual(current['game_sha256'],fallback['game_sha256'])
        if current['xr_build']==fallback['xr_build']:
            self.assertEqual(current['xr_sha256'],fallback['xr_sha256'])
        self.assertEqual(launcher.hud_horizontal_fov({},current),65.)
        for key in ('startup_menu','graphics_startup_lock','graphics_menu_lock','menu_recovery','ui_hud_plane',
                    'cinematic_camera','world_recovery','hud_size_control','crosshair_assignment_observer'):
            self.assertTrue(current[key])
        for tier in ('low','high'):
            with self.assertRaisesRegex(ValueError,'Unknown configuration'):
                launcher.settings({'vr_quality':tier},current)

    def test_preserved_material32_fallback_identity_and_features(self):
        fallback=json.loads(pathlib.Path(__file__).with_name('manifest-material32-movie23-vd.json').read_text())
        launcher.settings({},fallback)
        self.assertEqual(fallback['native_dll'],'GearsCaptureMaterial32.dll')
        self.assertEqual(fallback['native_build'],'tools/native-capture/build-material32')
        self.assertTrue(fallback['world_recovery'])
        self.assertTrue(fallback['movie_display'])
        self.assertFalse(fallback.get('camera_distance_control',False))

    def test_cinematic_target_option_requires_anchor_and_boolean(self):
        for value in (1,'true',None):
            with self.assertRaisesRegex(ValueError,'boolean'):
                launcher.settings({},dict(MANIFEST,anchor_observer=True,cinematic_camera=value))
        with self.assertRaisesRegex(ValueError,'requires anchor'):
            launcher.settings({},dict(MANIFEST,anchor_observer=False,cinematic_camera=True))
        launcher.settings({},dict(MANIFEST,anchor_observer=True,cinematic_camera=True))

    def test_startup_policy_event_failure_does_not_admit_handoff(self):
        backend=launcher.Backend.__new__(launcher.Backend)
        backend.runner=SimpleNamespace(checked_identity=mock.Mock())
        backend.control=SimpleNamespace(k=SimpleNamespace(OpenEventW=mock.Mock(return_value=9),
            WaitForSingleObject=mock.Mock(return_value=258),CloseHandle=mock.Mock()))
        backend.require_prime_active({'pid':55})
        backend.control.k.CloseHandle.assert_called_once_with(9)
        for result in (0,0xffffffff):
            backend.control.k.WaitForSingleObject.return_value=result
            with self.assertRaisesRegex(RuntimeError,'stopped before handoff'):backend.require_prime_active({'pid':55})
        backend.control.k.OpenEventW.return_value=0
        with self.assertRaisesRegex(RuntimeError,'event absent'):backend.require_prime_active({'pid':55})

    def test_graphics_startup_lock_requires_owned_lifetime(self):
        with self.assertRaisesRegex(ValueError,'owned startup'):
            launcher.settings({},dict(MANIFEST,graphics_startup_lock=True,startup_menu=False))
        with self.assertRaisesRegex(ValueError,'requires startup'):
            launcher.settings({},dict(MANIFEST,graphics_menu_lock=True))

    def test_experimental_preserve_upscaler_manifest_is_explicit_boolean_and_conflicts_with_locks(self):
        experimental = dict(MANIFEST, experimental_preserve_upscaler=True,
                            graphics_startup_lock=False, graphics_menu_lock=False)
        launcher.settings({}, experimental)
        for key in ('graphics_startup_lock', 'graphics_menu_lock'):
            with self.assertRaisesRegex(ValueError, 'conflicts with graphics locks'):
                launcher.settings({}, dict(experimental, **{key:True}))
        with self.assertRaisesRegex(ValueError, 'boolean'):
            launcher.settings({}, dict(experimental, experimental_preserve_upscaler='true'))
        with self.assertRaisesRegex(ValueError, 'Unknown configuration'):
            launcher.settings({'experimental_preserve_upscaler':True}, MANIFEST)

    def test_experimental_preserve_upscaler_reaches_reservation_without_graphics_lock_flags(self):
        backend = launcher.Backend.__new__(launcher.Backend)
        report = dict(pid=55, game_identity=dict(pid=55, creation_time=456,
            image=str(launcher.rooted(MANIFEST['game_path']))), game_sha256=MANIFEST['game_sha256'],
            trampolines_reserved=True, hooks_enabled=True, native_hooks_enabled=False,
            graphics_hooks_enabled=0, scene_hooks_enabled=False, first_view_mask_reserved=True,
            independent_entry_checks=[{'matches':True}])
        with tempfile.TemporaryDirectory() as directory, mock.patch.object(launcher.subprocess, 'run',
                return_value=SimpleNamespace(returncode=0, stdout=json.dumps(report), stderr='')) as spawn:
            log = pathlib.Path(directory)/'reserve.log'
            backend.reserve(MANIFEST, log)
            self.assertNotIn('--experimental-preserve-upscaler', spawn.call_args.args[0])
            backend.reserve(dict(MANIFEST, experimental_preserve_upscaler=True), log)
            command = spawn.call_args.args[0]
            self.assertIn('--experimental-preserve-upscaler', command)
            self.assertNotIn('--graphics-startup-lock', command)
            self.assertNotIn('--graphics-menu-lock', command)

    def test_reservation_admits_only_requested_graphics_hooks(self):
        backend=launcher.Backend.__new__(launcher.Backend)
        owner=dict(pid=7,creation_time=8)
        manifest=dict(MANIFEST,graphics_startup_lock=True,startup_menu=True)
        report=dict(pid=55,game_identity=dict(pid=55,creation_time=456,image=str(launcher.rooted(MANIFEST['game_path']))),
            game_sha256=MANIFEST['game_sha256'],trampolines_reserved=True,hooks_enabled=True,native_hooks_enabled=True,
            graphics_hooks_enabled=3,scene_hooks_enabled=False,first_view_mask_reserved=True,
            independent_entry_checks=[{'matches':True}],prime_lifetime='owner',prime_owner_pid=7,prime_owner_creation_time=8)
        with tempfile.TemporaryDirectory() as directory:
            log=pathlib.Path(directory)/'reserve.log'
            with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=json.dumps(report),stderr='')) as spawn:
                self.assertEqual(backend.reserve(manifest,log,owner=owner)['graphics_hooks_enabled'],3)
                self.assertIn('--graphics-startup-lock',spawn.call_args.args[0])
                self.assertEqual(spawn.call_args.kwargs['creationflags'],launcher.subprocess.CREATE_NO_WINDOW)
                with self.assertRaisesRegex(RuntimeError,'proof'):backend.reserve(MANIFEST,log)
            report['graphics_hooks_enabled']=4
            with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=json.dumps(report),stderr='')) as spawn:
                self.assertEqual(backend.reserve(dict(manifest,graphics_menu_lock=True),log,owner=owner)['graphics_hooks_enabled'],4)
                self.assertIn('--graphics-menu-lock',spawn.call_args.args[0])
                with self.assertRaisesRegex(RuntimeError,'proof'):backend.reserve(manifest,log,owner=owner)
            report['scene_hooks_enabled']=True
            with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=json.dumps(report),stderr='')):
                with self.assertRaisesRegex(RuntimeError,'proof'):backend.reserve(manifest,log,owner=owner)

    def test_hud_plane_manifest_dependencies_fail_before_reservation(self):
        for ui,anchor in ((False,False),(True,False),(False,True)):
            manifest=dict(MANIFEST,ui_hud_plane=True,ui_observer=ui,anchor_observer=anchor)
            with tempfile.TemporaryDirectory() as directory:
                backend=FakeBackend();index=pathlib.Path(directory)/'index.json'
                with self.assertRaisesRegex(ValueError,'HUD plane'):
                    launcher.start({},manifest,backend,index)
                self.assertEqual(backend.reserve_calls,0)
                backend.runner.run_session.assert_not_called();self.assertFalse(index.exists())

    def test_hud_plane_missing_manifest_option_remains_off(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend()
            self.start_mock(backend,pathlib.Path(directory)/'index.json',sleep=lambda seconds:None)
            self.assertFalse(backend.runner.run_session.call_args.args[0].ui_hud_plane)

    def test_reserve_requires_mask_trampoline(self):
        backend=launcher.Backend.__new__(launcher.Backend)
        report=dict(pid=55,game_identity=dict(pid=55,creation_time=456,image=str(launcher.rooted(MANIFEST['game_path']))),game_sha256=MANIFEST['game_sha256'],trampolines_reserved=True,hooks_enabled=True,native_hooks_enabled=False,
            first_view_mask_reserved=True,independent_entry_checks=[{'matches':True}])
        with tempfile.TemporaryDirectory() as directory:
            log=pathlib.Path(directory)/'reserve.log'
            with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=json.dumps(report),stderr='')) as spawn:
                self.assertEqual(backend.reserve(MANIFEST,log)['pid'],55)
                self.assertIn('--check-mask-control',spawn.call_args.args[0])
            report['first_view_mask_reserved']=False
            with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=json.dumps(report),stderr='')):
                with self.assertRaisesRegex(RuntimeError,'proof'):backend.reserve(MANIFEST,log)

    def test_custom_path_reaches_reservation_and_identity_check(self):
        with tempfile.TemporaryDirectory() as directory:
            selected=pathlib.Path(directory)/'Other Install/Binaries_x64/GOWDE-Steam.exe'
            backend=FakeBackend();backend.game['image']=str(selected.resolve())
            with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                launcher.start({'game_path':str(selected)},MANIFEST,backend,pathlib.Path(directory)/'index.json',sleep=lambda _:None)
            self.assertEqual(backend.selected_path,selected.resolve())
            backend=FakeBackend();original=backend.reserve
            def changed(*args,**kwargs):
                value=original(*args,**kwargs);value['game_identity']['creation_time']+=1;return value
            backend.reserve=changed
            with self.assertRaisesRegex(RuntimeError,'identity mismatch'):
                self.start_mock(backend,pathlib.Path(directory)/'wrong.json')
            backend.runner.run_session.assert_not_called()

    def test_unvalidated_install_has_no_startup_side_effects(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();backend.open_cancel=mock.Mock();index=pathlib.Path(directory)/'index.json'
            with mock.patch.object(launcher,'verify_files',side_effect=RuntimeError('Unvalidated game build')):
                with self.assertRaisesRegex(RuntimeError,'Unvalidated'):
                    launcher.start({'game_path':str(pathlib.Path(directory)/'bad.exe')},MANIFEST,backend,index)
            backend.open_cancel.assert_not_called();backend.runner.run_session.assert_not_called()
            self.assertEqual(backend.reserve_calls,0);self.assertFalse(index.exists())

    def test_backend_selected_path_argument_and_report_gate(self):
        backend=launcher.Backend.__new__(launcher.Backend)
        selected=launcher.rooted('other path/Binaries_x64/GOWDE-Steam.exe')
        report=dict(pid=55,game_identity=dict(pid=55,creation_time=456,image=str(selected)),game_sha256=MANIFEST['game_sha256'],
            trampolines_reserved=True,hooks_enabled=True,native_hooks_enabled=False,first_view_mask_reserved=True,independent_entry_checks=[{'matches':True}])
        with tempfile.TemporaryDirectory() as directory:
            with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=json.dumps(report),stderr='')) as spawn:
                backend.reserve(MANIFEST,pathlib.Path(directory)/'log',game_path=selected)
                command=spawn.call_args.args[0];self.assertEqual(command[command.index('--game-path')+1],str(selected))
                self.assertNotIn('--foreground',command)
                backend.reserve(MANIFEST,pathlib.Path(directory)/'log',game_path=selected,foreground=True)
                self.assertIn('--foreground',spawn.call_args.args[0])
            for field,value in (('game_sha256','bad'),('game_identity',dict(pid=55,creation_time=456,image='other.exe'))):
                bad=dict(report,**{field:value})
                with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=json.dumps(bad),stderr='')):
                    with self.assertRaisesRegex(RuntimeError,'identity/hash'):
                        backend.reserve(MANIFEST,pathlib.Path(directory)/'log',game_path=selected)

    def test_empty_cim_inventory_is_normal(self):
        backend=launcher.Backend.__new__(launcher.Backend)
        with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(stdout='')) as query:
            self.assertEqual(backend.game_pids(),[])
            self.assertIn('Get-CimInstance',query.call_args.args[0][-1])

    def test_persisted_launcher_language_settings(self):
        manifest=json.loads(pathlib.Path(__file__).with_name('manifest.json').read_text())
        with tempfile.TemporaryDirectory() as directory:
            config_path=pathlib.Path(directory)/'config.json'
            base=dict(game_path='install/game.exe',stereo_units_per_metre=123,hud_horizontal_fov_degrees=62.5)
            expected=launcher.settings(base,manifest)
            for language in ('zh-CN','en'):
                with self.subTest(language=language):
                    launcher.atomic_json(config_path,dict(base,launcher_language=language))
                    config=json.loads(config_path.read_text())
                    before=dict(config)
                    self.assertEqual(launcher.settings(config,manifest),expected)
                    self.assertEqual(launcher.hud_horizontal_fov(config,manifest),62.5)
                    self.assertEqual(config,before)
                    for invalid,message in (({'unknown':1},'Unknown configuration'),
                                            ({'stereo_units_per_metre':True},'stereo_units_per_metre'),
                                            ({'hud_horizontal_fov_degrees':101},'hud_horizontal_fov_degrees'),
                                            ({'game_path':''},'game_path')):
                        with self.subTest(invalid=invalid),self.assertRaisesRegex(ValueError,message):
                            launcher.settings(dict(config,**invalid),manifest)
            for invalid in ('','fr','EN',None,True,123,[],{}):
                with self.subTest(language=invalid),self.assertRaisesRegex(ValueError,'launcher_language'):
                    launcher.settings(dict(base,launcher_language=invalid),manifest)

    def test_scale_and_fixed_path(self):
        for value in (float('nan'),float('inf'),9,501,True,'100'):
            with self.subTest(value=value),self.assertRaises(ValueError):launcher.settings({'stereo_units_per_metre':value},MANIFEST)
        for value in (10,100,500):self.assertEqual(launcher.settings({'stereo_units_per_metre':value},MANIFEST)[1],value)
        absolute=str(launcher.rooted(MANIFEST['game_path']))
        self.assertEqual(launcher.settings({'game_path':absolute},MANIFEST)[0],pathlib.Path(absolute))
        self.assertEqual(launcher.settings({'game_path':'different.exe'},MANIFEST)[0],launcher.rooted('different.exe'))
        for invalid in ('',None,123):
            with self.assertRaises(ValueError):launcher.settings({'game_path':invalid},MANIFEST)
        with self.assertRaises(ValueError):launcher.settings({'unknown':1},MANIFEST)

    def test_hashes_and_profile(self):
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory)
            manifest=dict(MANIFEST,game_path='install/Binaries_x64/GOWDE-Steam.exe',native_build='native',native_dll='capture.dll',xr_build='xr')
            import hashlib
            for file,key in (('install/Binaries_x64/GOWDE-Steam.exe','game_sha256'),('native/capture.dll','native_sha256'),('xr/OpenXRPoseProbe.exe','xr_sha256')):
                path=root/file;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(file.encode())
                manifest[key]=hashlib.sha256(path.read_bytes()).hexdigest()
            profile=root/'tools/package-inspector/reloaded-profile.json';profile.parent.mkdir(parents=True)
            profile.write_text(json.dumps({'sha256':manifest['game_sha256']}))
            config=root/'install/WarGame/Config/Steam/Cooked/Coalesced.ini';config.parent.mkdir(parents=True);config.write_bytes(b'known config')
            import sys
            sys.path.insert(0,str(launcher.ROOT/'tools/debug-launch'))
            import game_install
            with mock.patch.object(game_install,'CONFIG_HASHES',{hashlib.sha256(config.read_bytes()).hexdigest()}):
                launcher.verify_files(manifest,root/manifest['game_path'],root)
            (root/'native/capture.dll').write_bytes(b'wrong')
            with mock.patch.object(game_install,'CONFIG_HASHES',{hashlib.sha256(config.read_bytes()).hexdigest()}),self.assertRaisesRegex(RuntimeError,'SHA256'):
                launcher.verify_files(manifest,root/manifest['game_path'],root)

    def test_live_pawn_and_menu_rejection(self):
        camera={'controller_address':'0x64','camera_address':'0x400','timestamp_address':'0x500'}
        r=FakeResolver();result=launcher.possessed_player(r,camera)
        self.assertEqual(result['level'],'0x12c')
        for mutation in ('menu','backlink','registry','class','level','default','timestamp'):
            r=FakeResolver()
            if mutation=='menu':r.pointers[108]=0
            elif mutation=='backlink':r.pointers[208]=999
            elif mutation=='registry':r.object_pointers[1]=999
            elif mutation=='class':r.objects[2000]['name']='MenuPawn'
            elif mutation=='level':r.objects[3000]['name']='Package'
            elif mutation=='default':r.objects[200]['name']='Default__WarPawn'
            else:r.stamp=float('nan')
            with self.subTest(mutation=mutation),self.assertRaises(RuntimeError):launcher.possessed_player(r,camera)

    def test_timestamp_progress_and_same_player(self):
        first=dict(pawn=1,controller=2,camera=3,level=4,timestamp=1)
        self.assertFalse(launcher.stable_player(first,first))
        self.assertTrue(launcher.stable_player(dict(first,timestamp=2),first))
        self.assertFalse(launcher.stable_player(dict(first,timestamp=2,pawn=9),first))

    def start_mock(self,backend,index,**kwargs):
        with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
            return launcher.start({},MANIFEST,backend,index,**kwargs)

    def test_existing_game_refused_without_reserve(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend(existing=True)
            with self.assertRaisesRegex(RuntimeError,'existing game'):self.start_mock(backend,pathlib.Path(directory)/'index.json')
            self.assertEqual(backend.reserve_calls,0);backend.runner.run_session.assert_not_called()

    def test_movie_display_requires_owned_menu_channel(self):
        with self.assertRaisesRegex(ValueError,'Movie display requires'):
            launcher.settings({},dict(MANIFEST,movie_display=True))
        launcher.settings({},dict(MANIFEST,movie_display=True,menu_recovery=True,startup_menu=True))

    def test_manifest_session_options_require_booleans(self):
        for key in launcher.SESSION_OPTIONS:
            with self.subTest(key=key),self.assertRaisesRegex(ValueError,'boolean'):
                launcher.settings({},dict(MANIFEST,**{key:'true'}))

    def test_private_late_reticle_requires_ui_and_hud(self):
        for ui,hud in ((False,False),(True,False),(False,True)):
            with self.assertRaises(ValueError):
                launcher.settings({},dict(MANIFEST,ui_reticle_late_sample=True,ui_observer=ui,ui_hud_plane=hud,anchor_observer=True))
        launcher.settings({},dict(MANIFEST,ui_reticle_late_sample=True,ui_observer=True,ui_hud_plane=True,anchor_observer=True))

    def test_crosshair_assignment_requires_observers(self):
        for ui,anchor in ((False,False),(True,False),(False,True)):
            with self.assertRaisesRegex(ValueError,'Crosshair assignment'):
                launcher.settings({},dict(MANIFEST,crosshair_assignment_observer=True,ui_observer=ui,anchor_observer=anchor))
        launcher.settings({},dict(MANIFEST,crosshair_assignment_observer=True,ui_observer=True,anchor_observer=True))

    def test_hud_layout_capability_and_validation(self):
        capable=dict(MANIFEST,hud_layout_control=True,ui_hud_plane=True,ui_observer=True,anchor_observer=True)
        self.assertEqual(launcher.hud_layout({},MANIFEST),{})
        self.assertEqual(launcher.hud_layout({},dict(MANIFEST,hud_layout_control=False)),{})
        self.assertEqual(launcher.hud_layout({},capable),dict(hud_element_scale=1.,hud_center_scale=1.))
        for key,good,bad in (('hud_element_scale',(1,1.2,2),(.99,2.01)),('hud_center_scale',(0,.8,1),(-.01,1.01))):
            for value in good:self.assertEqual(launcher.hud_layout({key:value},capable)[key],float(value))
            for value in (*bad,True,False,None,'1',float('nan'),float('inf')):
                with self.subTest(key=key,value=value),self.assertRaisesRegex(ValueError,key):
                    launcher.settings({key:value},capable)
            for manifest in (MANIFEST,dict(capable,hud_layout_control=False),dict(capable,ui_hud_plane=False)):
                with self.assertRaises(ValueError):launcher.settings({key:1},manifest)
        defaults=dict(capable,hud_element_scale=1.2,hud_center_scale=.8)
        self.assertEqual(launcher.hud_layout({},defaults),dict(hud_element_scale=1.2,hud_center_scale=.8))
        self.assertEqual(launcher.hud_layout(dict(hud_element_scale=1.5,hud_center_scale=.5),defaults),dict(hud_element_scale=1.5,hud_center_scale=.5))
        for key,invalid in (('hud_element_scale',2.1),('hud_center_scale',-.1),('hud_element_scale',True),('hud_center_scale',float('nan'))):
            with self.subTest(key=key,invalid=invalid),self.assertRaisesRegex(ValueError,'Manifest '+key):
                launcher.settings({key:1},dict(defaults,**{key:invalid}))
        for flag in (True,1,'true',None):
            with self.subTest(flag=flag),self.assertRaises(ValueError):
                launcher.settings({},dict(MANIFEST,hud_layout_control=flag))

    def test_hud_layout_launcher_runner_and_native_command(self):
        spec=importlib.util.spec_from_file_location('layout_runner',launcher.ROOT/'tools/native-capture/run_stereo_continuous.py')
        runner=importlib.util.module_from_spec(spec)
        with mock.patch.object(sys,'path',[str(launcher.ROOT/'tools/native-capture'),*sys.path]):spec.loader.exec_module(runner)
        capable=dict(MANIFEST,hud_layout_control=True,ui_hud_plane=True,ui_observer=True,anchor_observer=True)
        fields=('hud_element_scale','hud_center_scale')
        for manifest,config,expected in ((MANIFEST,{},{}),(capable,{},dict.fromkeys(fields,1.)),
                (dict(capable,hud_element_scale=1.2,hud_center_scale=.8),{},dict(hud_element_scale=1.2,hud_center_scale=.8)),
                (capable,dict(hud_element_scale=1.2,hud_center_scale=.8,launcher_language='en'),dict(hud_element_scale=1.2,hud_center_scale=.8))):
            with tempfile.TemporaryDirectory() as directory:
                backend=FakeBackend();before=dict(config)
                with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                    launcher.start(config,manifest,backend,pathlib.Path(directory)/'index.json',sleep=lambda seconds:None)
                args=backend.runner.run_session.call_args.args[0]
                command=runner.native_command(args,dict(pid=100,creation_time=123))
                for key in fields:
                    flag='--'+key.replace('_','-')
                    if key in expected:
                        self.assertEqual(getattr(args,key),expected[key])
                        self.assertEqual(command[command.index(flag)+1],str(expected[key]))
                    else:
                        self.assertFalse(hasattr(args,key));self.assertNotIn(flag,command)
                self.assertEqual(config,before)

    def test_hud_size_config_capability_range_and_legacy_tuple(self):
        capable=dict(MANIFEST,hud_size_control=True,ui_hud_plane=True,ui_observer=True,anchor_observer=True)
        self.assertIsNone(launcher.hud_horizontal_fov({},MANIFEST))
        self.assertEqual(len(launcher.settings({},MANIFEST)),2)
        self.assertEqual(launcher.hud_horizontal_fov({},capable),65.)
        for value in (45,65.25,100):
            self.assertEqual(launcher.hud_horizontal_fov({'hud_horizontal_fov_degrees':value},capable),float(value))
        for value in (True,False,None,'65',float('nan'),float('inf'),44.99,100.01):
            with self.subTest(value=value),self.assertRaises(ValueError):
                launcher.settings({'hud_horizontal_fov_degrees':value},capable)
        for manifest in (MANIFEST,dict(capable,ui_hud_plane=False),dict(capable,hud_size_control='true')):
            with self.assertRaises(ValueError):launcher.settings({'hud_horizontal_fov_degrees':65},manifest)
        with self.assertRaises(ValueError):launcher.settings({},dict(MANIFEST,hud_size_control=1))

    def test_hud_size_reaches_runner_only_for_capable_manifest(self):
        capable=dict(MANIFEST,hud_size_control=True,ui_hud_plane=True,ui_observer=True,anchor_observer=True)
        for manifest,config,expected in ((MANIFEST,{},None),(capable,{},65.),
                                         (capable,{'hud_horizontal_fov_degrees':62.5},62.5)):
            with tempfile.TemporaryDirectory() as directory:
                backend=FakeBackend()
                with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                    launcher.start(config,manifest,backend,pathlib.Path(directory)/'index.json',sleep=lambda seconds:None)
                args=backend.runner.run_session.call_args.args[0]
                self.assertEqual(getattr(args,'hud_horizontal_fov_degrees',None),expected)
                if expected is None:self.assertFalse(hasattr(args,'hud_horizontal_fov_degrees'))

    def test_preview_session_options_reach_runner(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend()
            manifest=dict(MANIFEST,startup_menu=True,**{key:True for key in launcher.SESSION_OPTIONS})
            with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                launcher.start({},manifest,backend,pathlib.Path(directory)/'index.json',sleep=lambda seconds:None)
            args=backend.runner.run_session.call_args.args[0]
            self.assertTrue(all(getattr(args,key) is True for key in launcher.SESSION_OPTIONS))
            self.assertEqual(args.native_expected_sha256,manifest['native_sha256'])

    def test_menu_recovery_requires_startup_capture(self):
        with self.assertRaisesRegex(ValueError,'Menu recovery requires'):
            launcher.settings({},dict(MANIFEST,menu_recovery=True,startup_menu=False))

    def test_same_process_runner_after_two_samples(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend()
            self.assertEqual(self.start_mock(backend,pathlib.Path(directory)/'index.json',sleep=lambda seconds:None),0)
            self.assertEqual(backend.samples,2)
            args=backend.runner.run_session.call_args.args[0]
            self.assertEqual(args.pid,55)
            self.assertIs(backend.runner.run_session.call_args.args[1],backend.control)
            self.assertTrue(callable(backend.runner.run_session.call_args.kwargs['on_report']))
            backend.runner.run_session.assert_called_once()

    def test_real_output_monitor_starts_before_level_wait_and_spans_session(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json'
            monitor=backend.start_tiny_output_monitor.return_value
            monitor.close.return_value=True
            player=backend.player
            def observed_player(*args):
                backend.start_tiny_output_monitor.assert_called_once()
                monitor.close.assert_not_called()
                return player(*args)
            backend.player=observed_player
            def running(*args,**kwargs):
                monitor.close.assert_not_called()
                return 0
            backend.runner.run_session.side_effect=running
            self.assertEqual(self.start_mock(backend,index,sleep=lambda seconds:None),0)
            backend.start_tiny_output_monitor.assert_called_once_with(MANIFEST,backend.game,index.with_suffix('.tiny-output.jsonl'))
            monitor.close.assert_called_once()
            self.assertEqual(json.loads(index.read_text())['tiny_output_monitor_cleanup'],{'joined':True})

    def test_output_monitor_join_failure_is_preserved_without_blocking_prime_cleanup(self):
        for result in (False,RuntimeError('monitor close fault')):
            with self.subTest(result=result),tempfile.TemporaryDirectory() as directory:
                backend=FakeBackend();index=pathlib.Path(directory)/'index.json'
                monitor=backend.start_tiny_output_monitor.return_value
                if isinstance(result,Exception):monitor.close.side_effect=result
                else:monitor.close.return_value=result
                backend.finish_owned_prime=mock.Mock(return_value={'entry_restore_verified':True})
                with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                    self.assertEqual(launcher.start({},dict(MANIFEST,startup_menu=True),backend,index,sleep=lambda _:None),0)
                report=json.loads(index.read_text())
                self.assertIs(report['tiny_output_monitor_cleanup']['joined'],False)
                self.assertTrue(report['prime_cleanup']['entry_restore_verified'])
                backend.finish_owned_prime.assert_called_once()
                self.assertEqual(backend.control.k.CloseHandle.call_count,2)

    def test_output_monitor_start_failure_is_diagnostic_only(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json'
            backend.start_tiny_output_monitor.side_effect=RuntimeError('observer unavailable')
            self.assertEqual(self.start_mock(backend,index,sleep=lambda seconds:None),0)
            diagnostic=json.loads(index.with_suffix('.tiny-output.jsonl').read_text())
            self.assertEqual(diagnostic['event'],'monitor_start_error')
            self.assertNotIn('tiny_output_recovery',json.loads(index.read_text()))

    def test_autonomous_observe_only_override_keeps_monitor_nonmutating(self):
        sys.path.insert(0,str(launcher.ROOT/'tools/debug-launch'))
        backend=launcher.Backend.__new__(launcher.Backend)
        for setting,allowed in (('1',False),('0',True)):
            with self.subTest(setting=setting),mock.patch.dict(os.environ,{'GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY':setting}), \
                    mock.patch('tiny_output_recovery.TinyOutputMonitor') as monitor:
                backend.start_tiny_output_monitor(MANIFEST,{'pid':55},pathlib.Path('diagnostic.jsonl'))
                self.assertIs(monitor.call_args.kwargs['allow_recovery'],allowed)
                monitor.return_value.start.assert_called_once()

    def window_user_fixture(self,rows):
        user=SimpleNamespace(**{name:mock.Mock() for name in
            ('EnumWindows','IsWindowVisible','GetWindowThreadProcessId','GetClientRect')})
        def enum(callback,argument):
            for hwnd in rows:self.assertTrue(callback(hwnd,argument))
            return True
        def owner(hwnd,out):out._obj.value=rows[hwnd]['pid'];return 1
        def rectangle(hwnd,out):
            value=rows[hwnd].get('rect')
            if value is None:return False
            out._obj.left,out._obj.top,out._obj.right,out._obj.bottom=value;return True
        user.EnumWindows.side_effect=enum;user.GetWindowThreadProcessId.side_effect=owner
        user.IsWindowVisible.side_effect=lambda hwnd:rows[hwnd]['visible']
        user.GetClientRect.side_effect=rectangle
        return user

    def test_game_window_optional_diagnostic_uses_same_candidate_reads(self):
        rows={1:dict(pid=99,visible=True,rect=(0,0,1920,1080)),
              2:dict(pid=55,visible=False,rect=(0,0,1920,1080)),
              3:dict(pid=55,visible=True,rect=(0,0,8,8)),
              4:dict(pid=55,visible=True,rect=None),
              5:dict(pid=55,visible=True,rect=(0,0,640,360))}
        backend=launcher.Backend.__new__(launcher.Backend)
        for enabled in (False,True):
            with self.subTest(diagnostic=enabled):
                user=self.window_user_fixture(rows);diagnostic={}
                with mock.patch.object(launcher.ctypes,'WinDLL',return_value=user):
                    result=backend.game_window(55,**({'diagnostic':diagnostic} if enabled else {}))
                self.assertEqual(result,5);user.EnumWindows.assert_called_once()
                self.assertEqual(user.GetWindowThreadProcessId.call_count,5)
                self.assertEqual(user.IsWindowVisible.call_count,4)
                self.assertEqual([call.args[0] for call in user.GetClientRect.call_args_list],[3,4,5])
                if enabled:
                    self.assertEqual(diagnostic['reason'],'accepted')
                    self.assertEqual(diagnostic['matched_pid_windows'],4)
                    self.assertEqual([x['rejection'] for x in diagnostic['candidates']],
                                     ['not_visible','client_too_small','client_rect_unavailable',None])
                    self.assertIsNone(diagnostic['candidates'][0]['client_rect'])
                    self.assertEqual(diagnostic['candidates'][1]['client_rect']['right'],8)

    def test_game_window_diagnostic_bounds_and_multiple_candidates_still_reject(self):
        backend=launcher.Backend.__new__(launcher.Backend)
        for kind in ('none','hidden','multiple'):
            rows={} if kind=='none' else {i:dict(pid=55,visible=kind=='multiple',rect=(0,0,1920,1080))
                                         for i in range(1,3 if kind=='multiple' else 21)}
            user=self.window_user_fixture(rows);diagnostic={}
            with mock.patch.object(launcher.ctypes,'WinDLL',return_value=user):
                if kind=='multiple':
                    with self.assertRaisesRegex(RuntimeError,'Multiple game windows'):backend.game_window(55,diagnostic=diagnostic)
                    self.assertEqual(diagnostic['reason'],'multiple_eligible_windows')
                else:
                    self.assertIsNone(backend.game_window(55,diagnostic=diagnostic))
                    self.assertEqual(diagnostic['reason'],'no_pid_window' if kind=='none' else 'no_eligible_window')
            self.assertLessEqual(len(diagnostic['candidates']),16)
            if kind=='hidden':self.assertEqual(diagnostic['dropped_candidates'],4);user.GetClientRect.assert_not_called()

    def test_window_wait_keeps_30s_last_snapshot_and_cancellation_identity_guards(self):
        for scenario in ('timeout','cancel','identity'):
            with self.subTest(scenario=scenario),tempfile.TemporaryDirectory() as directory:
                backend=FakeBackend();index=pathlib.Path(directory)/'index.json';ticks=[0.];calls=[]
                def window(pid,*,diagnostic):
                    calls.append(ticks[0]);diagnostic.clear()
                    diagnostic.update(reason='no_eligible_window',candidates=[dict(hwnd=123,visible=True,
                        client_rect=dict(left=0,top=0,right=8,bottom=8),eligible=False,rejection='client_too_small')],
                        sample=len(calls))
                    return None
                backend.game_window=window
                if scenario=='cancel':backend.cancelled=lambda handle:ticks[0]>=10
                if scenario=='identity':
                    identify=backend.control.identity
                    backend.control.identity=lambda pid:dict(backend.game,creation_time=999) if pid==55 and ticks[0]>=10 else identify(pid)
                def sleep(seconds):self.assertEqual(seconds,.1);ticks[0]+=5
                with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                    if scenario=='timeout':
                        with self.assertRaisesRegex(TimeoutError,'Game window unavailable'):
                            launcher.start({},dict(MANIFEST,startup_menu=True),backend,index,clock=lambda:ticks[0],sleep=sleep)
                    elif scenario=='identity':
                        with self.assertRaisesRegex(RuntimeError,'Reserved game exited or changed'):
                            launcher.start({},dict(MANIFEST,startup_menu=True),backend,index,clock=lambda:ticks[0],sleep=sleep)
                    else:self.assertEqual(launcher.start({},dict(MANIFEST,startup_menu=True),backend,index,clock=lambda:ticks[0],sleep=sleep),0)
                state=json.loads(index.read_text());backend.runner.run_session.assert_not_called()
                self.assertEqual(backend.reserve_calls,1);self.assertFalse(state['native_activated'])
                if scenario=='timeout':
                    self.assertEqual(ticks[0],30);self.assertEqual(calls,[0,5,10,15,20,25])
                    self.assertEqual(state['startup_window_last_diagnostic']['sample'],6)
                    self.assertEqual(state['startup_window_last_diagnostic']['candidates'][0]['rejection'],'client_too_small')
                    self.assertTrue(state['no_automatic_retry'])
                else:self.assertEqual(calls,[0,5]);self.assertNotIn('startup_window_last_diagnostic',state)

    def test_startup_menu_opens_xr_before_waiting_for_player_without_legacy_deadline(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json';ticks=[0.];events=[]
            def run(args,control,**kwargs):
                events.append('xr');self.assertEqual(backend.samples,0)
                self.assertEqual(args.startup_menu_window,12345)
                ticks[0]=240. # An owned menu session is not bounded by legacy prime.
                kwargs['before_native_activation'](lambda:events.append('guard'))
                self.assertEqual(backend.samples,2);events.append('activate');return 0
            backend.runner.run_session=mock.Mock(side_effect=run)
            with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                self.assertEqual(launcher.start({},dict(MANIFEST,startup_menu=True),backend,index,
                    clock=lambda:ticks[0],sleep=lambda seconds:None),0)
            self.assertEqual(events[0],'xr');self.assertEqual(events[-1],'activate')
            self.assertEqual(backend.prime_owner,backend.owner)
            self.assertTrue(json.loads(index.read_text())['prime_cleanup']['entry_restore_verified'])

    def test_startup_menu_guard_failure_prevents_player_resolution_and_cleans_prime(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json'
            backend.finish_owned_prime=mock.Mock(return_value={'entry_restore_verified':True})
            def run(args,control,**kwargs):
                def failed():raise RuntimeError('XR exited')
                kwargs['before_native_activation'](failed)
            backend.runner.run_session=mock.Mock(side_effect=run)
            with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                with self.assertRaisesRegex(RuntimeError,'XR exited'):
                    launcher.start({},dict(MANIFEST,startup_menu=True),backend,index)
            self.assertEqual(backend.samples,0);backend.finish_owned_prime.assert_called_once()
            self.assertEqual(backend.control.k.CloseHandle.call_count,2)

    def test_prime_cleanup_failure_still_closes_launcher_handles(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend(cancel=True);index=pathlib.Path(directory)/'index.json'
            backend.finish_owned_prime=mock.Mock(side_effect=RuntimeError('Restore pending'))
            with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                with self.assertRaisesRegex(RuntimeError,'Restore pending'):
                    launcher.start({},dict(MANIFEST,startup_menu=True),backend,index)
            self.assertEqual(backend.control.k.CloseHandle.call_count,2)

    def test_cancel_before_activation_keeps_watchdog(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend(cancel=True);index=pathlib.Path(directory)/'index.json'
            self.assertEqual(self.start_mock(backend,index),0)
            report=json.loads(index.read_text());self.assertFalse(report['native_activated']);self.assertTrue(report['prime_watchdog_pending'])
            backend.runner.run_session.assert_not_called();self.assertEqual(backend.reserve_calls,1)

    def test_runner_cancel_exit_zero_is_not_vr_pass(self):
        (launcher.ROOT/'analysis/runtime').mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(dir=launcher.ROOT/'analysis/runtime') as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json'
            def cancelled(args,control,on_report,cancel_requested):
                report=launcher.ROOT/'analysis/runtime'/('mock-launcher-cancel-'+pathlib.Path(directory).name+'.json')
                report.write_text(json.dumps(dict(session_id='test',owner_identity=backend.owner,
                    game_identity=backend.game,phase='starting',passed=False)))
                try:
                    on_report(report,backend.owner,backend.game)
                    report.write_text(json.dumps(dict(session_id='test',owner_identity=backend.owner,phase='cancelled',
                        passed=False,startup_cancelled=True,native_started=False)))
                    return 0
                finally:
                    # start() reads the report immediately after this callback returns.
                    self.addCleanup(lambda:report.unlink(missing_ok=True))
            backend.runner.run_session=mock.Mock(side_effect=cancelled)
            self.assertEqual(self.start_mock(backend,index,sleep=lambda seconds:None),0)
            state=json.loads(index.read_text())
            self.assertFalse(state['passed']);self.assertFalse(state['native_activated'])
            self.assertEqual(state['phase'],'cancelled_before_activation')
            self.assertTrue(state['prime_watchdog_pending'])

    def test_prime_deadline_never_retries(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend(ready=False);ticks=iter((0,0,166));index=pathlib.Path(directory)/'index.json'
            with self.assertRaises(TimeoutError):self.start_mock(backend,index,clock=lambda:next(ticks),sleep=lambda seconds:None)
            backend.runner.run_session.assert_not_called();self.assertEqual(backend.reserve_calls,1)
            self.assertFalse(json.loads(index.read_text())['native_activated'])

    def test_auto_continue_precedes_player_and_runner_with_original_deadline(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json'
            clock=[100.];events=[]
            original_reserve,original_player=backend.reserve,backend.player
            def reserve(*args,**kwargs):
                events.append('reserve');clock[0]+=3
                return original_reserve(*args,**kwargs)
            def navigate(pid,identity,cancel,deadline,log):
                events.append('navigate')
                self.assertEqual((pid,identity,cancel),(55,backend.game,2))
                self.assertEqual(deadline,220.)
                self.assertEqual(log,index.with_suffix('.menu.jsonl'))
                self.assertEqual(json.loads(index.read_text())['phase'],'navigating_menu')
                clock[0]+=20
                return dict(loading_requested=True,level_ready=False)
            def player(*args):
                events.append('player')
                return original_player(*args)
            backend.reserve=reserve;backend.navigate=mock.Mock(side_effect=navigate);backend.player=player
            backend.runner.run_session.side_effect=lambda *a,**k:events.append('runner') or 0
            self.assertEqual(self.start_mock(backend,index,auto_continue=True,
                                            clock=lambda:clock[0],sleep=lambda seconds:None),0)
            self.assertEqual(events,['reserve','navigate','player','player','runner'])
            self.assertFalse(json.loads(index.read_text())['menu_navigation']['level_ready'])

    def test_auto_continue_failure_or_no_loading_never_activates(self):
        for raises in (True,False):
            with self.subTest(raises=raises),tempfile.TemporaryDirectory() as directory:
                backend=FakeBackend();index=pathlib.Path(directory)/'index.json'
                backend.navigate=mock.Mock(side_effect=RuntimeError('Menu failed')) if raises else mock.Mock(return_value={'loading_requested':False})
                with self.assertRaises(RuntimeError):
                    self.start_mock(backend,index,auto_continue=True)
                backend.runner.run_session.assert_not_called()
                self.assertEqual(backend.samples,0)
                state=json.loads(index.read_text())
                self.assertFalse(state['native_activated'])
                self.assertTrue(state['no_automatic_retry'])
                self.assertIn('watchdog',state['recovery'])

    def test_navigation_cancel_retains_prime_watchdog(self):
        from menu_navigation import NavigationCancelled
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json'
            backend.navigate=mock.Mock(side_effect=NavigationCancelled('Cancelled'))
            self.assertEqual(self.start_mock(backend,index,auto_continue=True),0)
            backend.runner.run_session.assert_not_called()
            self.assertEqual(backend.samples,0)
            state=json.loads(index.read_text())
            self.assertEqual(state['phase'],'cancelled_before_activation')
            self.assertFalse(state['native_activated'])
            self.assertTrue(state['prime_watchdog_pending'])

    def test_navigation_consuming_start_deadline_cannot_activate_even_if_loading(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json';clock=[10.]
            def navigate(*args):
                clock[0]=130.
                return dict(loading_requested=True)
            backend.navigate=mock.Mock(side_effect=navigate)
            with self.assertRaises(TimeoutError):
                self.start_mock(backend,index,auto_continue=True,clock=lambda:clock[0],sleep=lambda seconds:None)
            backend.runner.run_session.assert_not_called()
            self.assertEqual(backend.samples,0)
            self.assertFalse(json.loads(index.read_text())['native_activated'])

    def test_stop_navigating_menu_signals_owner_cancel(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json'
            backend.control.signal=mock.Mock(return_value=True)
            backend.runner.checked_identity=mock.Mock()
            backend.runner.request_stop=mock.Mock()
            index.write_text(json.dumps(dict(schema=1,kind='repository_vr_launcher',
                                             owner_identity=backend.owner,phase='navigating_menu')))
            with mock.patch('builtins.print'):
                self.assertEqual(launcher.stop(backend,index),0)
            backend.runner.checked_identity.assert_called_once_with(backend.control,backend.owner)
            backend.control.signal.assert_called_once_with(launcher.cancel_name(backend.owner['pid']))
            backend.runner.request_stop.assert_not_called()

    def test_stop_startup_and_session_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            index=pathlib.Path(directory)/'index.json';backend=FakeBackend()
            backend.control.signal=mock.Mock(return_value=True)
            backend.runner.checked_identity=mock.Mock()
            backend.runner.request_stop=mock.Mock(return_value=0)
            state=dict(schema=1,kind='repository_vr_launcher',owner_identity=backend.owner,phase='waiting_for_level')
            index.write_text(json.dumps(state));self.assertEqual(launcher.stop(backend,index),0)
            backend.runner.request_stop.assert_not_called()
            original=dict(session_id='token',owner_identity=backend.owner,phase='running')
            report=pathlib.Path(directory)/'owner.json';report.write_text(json.dumps(original))
            state.update(session_report=str(report),session_id='token');index.write_text(json.dumps(state))
            self.assertEqual(launcher.stop(backend,index),0);backend.runner.request_stop.assert_called_once()
            original['session_id']='other';report.write_text(json.dumps(original))
            with self.assertRaisesRegex(RuntimeError,'identity'):launcher.stop(backend,index)

    def recenter_fixture(self,directory):
        backend=FakeBackend();index=pathlib.Path(directory)/'index.json';report=pathlib.Path(directory)/'owner.json'
        session=dict(schema=1,kind='continuous_strict_stereo',session_id='token',owner_identity=backend.owner,
            game_identity=backend.game,xr_identity=dict(pid=66,creation_time=789,image='xr.exe'),phase='running',
            native_started_qpc=777,native_command=['start_capture.py','--build-dir',directory])
        state=dict(schema=1,kind='repository_vr_launcher',owner_identity=backend.owner,game_identity=backend.game,
            session_id='token',session_report=str(report))
        index.write_text(json.dumps(state));report.write_text(json.dumps(session))
        fields=dict(started_qpc='777',reference_generation='2',recenter_requests='1',phase='running')
        backend.runner.checked_identity=mock.Mock()
        backend.runner.request_recenter=mock.Mock(return_value=0)
        backend.runner.read_session_status=mock.Mock(return_value=fields)
        backend.runner.native_owner_matches=mock.Mock(return_value=True)
        return backend,index,report,session,fields

    def test_recenter_association_and_identity_guards(self):
        with tempfile.TemporaryDirectory() as directory:
            backend,index,report,session,fields=self.recenter_fixture(directory)
            with mock.patch('builtins.print') as output:self.assertEqual(launcher.recenter(backend,index),0)
            self.assertEqual(backend.runner.checked_identity.call_count,3)
            backend.runner.request_recenter.assert_called_once_with(report.resolve(),backend.control)
            self.assertFalse(json.loads(output.call_args.args[0])['applied_verified'])
            for field,value in (('phase','stopped'),('session_id','other'),('game_identity',dict(session['game_identity'],creation_time=457))):
                changed=dict(session,**{field:value});report.write_text(json.dumps(changed));backend.runner.request_recenter.reset_mock()
                with self.subTest(field=field),self.assertRaises(RuntimeError):launcher.recenter(backend,index)
                backend.runner.request_recenter.assert_not_called()

    def test_recenter_stale_native_and_reused_process_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            backend,index,report,session,fields=self.recenter_fixture(directory)
            fields['started_qpc']='778'
            with self.assertRaisesRegex(RuntimeError,'native status'):launcher.recenter(backend,index)
            backend.runner.request_recenter.assert_not_called()
            fields['started_qpc']='777';backend.runner.checked_identity.side_effect=RuntimeError('Creation identity changed')
            with self.assertRaisesRegex(RuntimeError,'Creation'):launcher.recenter(backend,index)
            backend.runner.request_recenter.assert_not_called()

    def test_recenter_old_build_reports_unavailable(self):
        with tempfile.TemporaryDirectory() as directory:
            backend,index,report,session,fields=self.recenter_fixture(directory)
            fields.pop('reference_generation');fields.pop('recenter_requests')
            backend.runner.request_recenter.side_effect=RuntimeError('Stereo recenter control is unavailable in this native build')
            with self.assertRaisesRegex(RuntimeError,'unavailable'):launcher.recenter(backend,index)

    def test_current_native_status_has_safe_distinct_failure_reasons_without_retry(self):
        for kind in ('empty','unknown','owner','qpc'):
            with self.subTest(kind=kind),tempfile.TemporaryDirectory() as directory:
                backend,_,_,session,fields=self.recenter_fixture(directory)
                fields['owner_pid']='PRIVATE_OWNER';fields['owner_creation_time']='PRIVATE_CREATION'
                def read(path,pid,floor,*,diagnostic):
                    if kind=='empty':diagnostic.update(reason='empty_file',text_length=0,ends_with_newline=False)
                    elif kind!='unknown':diagnostic.update(reason='accepted')
                    if kind in ('empty','unknown'):return None
                    return fields
                backend.runner.read_session_status.side_effect=read
                if kind=='owner':backend.runner.native_owner_matches.return_value=False
                if kind=='qpc':fields['started_qpc']='PRIVATE_QPC'
                with self.assertRaisesRegex(RuntimeError,'different or unverified session') as failure:
                    launcher.current_native_status(backend,session)
                detail=failure.exception.native_status_diagnostic
                self.assertEqual(detail['reason'],'reader_rejected' if kind in ('empty','unknown') else 'owner_mismatch' if kind=='owner' else 'started_qpc_mismatch')
                if kind in ('empty','unknown'):self.assertEqual(detail['reader']['reason'],'empty_file' if kind=='empty' else 'unknown')
                for private in ('PRIVATE_OWNER','PRIVATE_CREATION','PRIVATE_QPC'):
                    self.assertNotIn(private,str(failure.exception))
                backend.runner.read_session_status.assert_called_once()
                backend.runner.request_recenter.assert_not_called()

    def test_status_generation_requires_matching_session(self):
        with tempfile.TemporaryDirectory() as directory:
            backend,index,report,session,fields=self.recenter_fixture(directory)
            with mock.patch('builtins.print'):value=launcher.status(backend,index)
            self.assertEqual(value['reference_generation'],2);self.assertTrue(value['native_status_matched'])
            fields['started_qpc']='778'
            with mock.patch('builtins.print'):value=launcher.status(backend,index)
            self.assertIsNone(value['reference_generation']);self.assertFalse(value['native_status_matched'])

    def test_reservation_failure_exposes_original_cause_and_keeps_full_log_and_cleanup(self):
        backend=launcher.Backend.__new__(launcher.Backend)
        backend.close_uninjected_reservation=mock.Mock(return_value={'status':'no_birth_receipt'})
        stderr=('Traceback (most recent call last):\n  File "graphics_policy.py", line 1\n'
                'RuntimeError: Persisted DLSS must be OFF before release startup\n'
                'At C:\\Mod folder\\Start-DebugGame.ps1:1 char:1\n'
                'Traceback (most recent call last):\n'
                'subprocess.CalledProcessError: Command powershell returned non-zero exit status 1.\n')
        result=SimpleNamespace(returncode=1,stdout='startup checks passed\n',stderr=stderr)
        with tempfile.TemporaryDirectory() as directory:
            log=pathlib.Path(directory)/'reserve.log'
            with mock.patch.object(launcher.subprocess,'run',return_value=result) as spawn:
                with self.assertRaises(RuntimeError) as failed:backend.reserve(MANIFEST,log)
            message=str(failed.exception)
            self.assertIn('exit code 1',message)
            self.assertIn('RuntimeError: Persisted DLSS must be OFF before release startup',message)
            self.assertNotIn('CalledProcessError',message)
            self.assertIn(str(log),message)
            self.assertIn(stderr,log.read_text())
            self.assertIn('reservation_failure_cleanup',log.read_text())
            self.assertEqual(failed.exception.reservation_failure_cleanup,{'status':'no_birth_receipt'})
            self.assertEqual(spawn.call_count,1)
            self.assertEqual(spawn.call_args.kwargs['errors'],'replace')


    def test_reservation_error_summary_fallback_and_output_bound(self):
        self.assertEqual(launcher.reservation_failure_detail('stdout cause',''),'stdout cause')
        self.assertEqual(launcher.reservation_failure_detail('stdout cause','stderr cause'),'stderr cause')
        self.assertIn('no error output',launcher.reservation_failure_detail('',' \n'))
        self.assertEqual(launcher.reservation_failure_detail('', 'OSError: Access denied'),'OSError: Access denied')
        self.assertEqual(launcher.reservation_failure_detail('', 'Traceback:\nException: failed\nwrapper'),'Exception: failed')
        wrapper='subprocess.CalledProcessError: command exited with code 1'
        self.assertEqual(launcher.reservation_failure_detail('',wrapper),wrapper)
        detail=launcher.reservation_failure_detail('','omitted prefix\n'+'x'*5000+'\nlast line')
        self.assertLessEqual(len(detail),1500)
        self.assertNotIn('omitted prefix',detail)
        self.assertTrue(detail.endswith('last line'))


if __name__=='__main__':unittest.main()
