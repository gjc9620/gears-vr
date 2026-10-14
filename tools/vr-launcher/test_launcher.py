import importlib.util
import json
import os
import pathlib
import struct
import tempfile
import unittest
from types import SimpleNamespace
from unittest import mock

SPEC=importlib.util.spec_from_file_location('vr_launcher',pathlib.Path(__file__).with_name('launcher.py'))
launcher=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(launcher)
MANIFEST=json.loads(pathlib.Path(__file__).with_name('manifest.json').read_text())

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
        self.existing=existing;self.cancel=cancel;self.ready=ready;self.reserve_calls=0;self.samples=0
    def game_pids(self):return [55] if self.existing else []
    def open_cancel(self,pid):return 2
    def cancelled(self,handle):return self.cancel
    def reserve(self,manifest,log,owner=None):self.reserve_calls+=1;self.prime_owner=owner;return {'pid':55}
    def game_window(self,pid):return 12345
    def finish_owned_prime(self,manifest,identity,owner):return {'entry_restore_verified':True}
    def player(self,pid,profile):
        if not self.ready:raise RuntimeError('Menu has no Pawn')
        self.samples+=1
        return dict(pawn='0x200',controller='0x100',camera='0x400',level='0x300',timestamp=float(self.samples))

class LauncherTests(unittest.TestCase):
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
        report=dict(pid=55,trampolines_reserved=True,hooks_enabled=True,native_hooks_enabled=False,
            first_view_mask_reserved=True,independent_entry_checks=[{'matches':True}])
        with tempfile.TemporaryDirectory() as directory:
            log=pathlib.Path(directory)/'reserve.log'
            with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=json.dumps(report),stderr='')) as spawn:
                self.assertEqual(backend.reserve(MANIFEST,log)['pid'],55)
                self.assertIn('--check-mask-control',spawn.call_args.args[0])
            report['first_view_mask_reserved']=False
            with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=json.dumps(report),stderr='')):
                with self.assertRaisesRegex(RuntimeError,'proof'):backend.reserve(MANIFEST,log)

    def test_empty_cim_inventory_is_normal(self):
        backend=launcher.Backend.__new__(launcher.Backend)
        with mock.patch.object(launcher.subprocess,'run',return_value=SimpleNamespace(stdout='')) as query:
            self.assertEqual(backend.game_pids(),[])
            self.assertIn('Get-CimInstance',query.call_args.args[0][-1])

    def test_scale_and_fixed_path(self):
        for value in (float('nan'),float('inf'),9,501,True,'100'):
            with self.subTest(value=value),self.assertRaises(ValueError):launcher.settings({'stereo_units_per_metre':value},MANIFEST)
        for value in (10,100,500):self.assertEqual(launcher.settings({'stereo_units_per_metre':value},MANIFEST)[1],value)
        absolute=str(launcher.rooted(MANIFEST['game_path']))
        self.assertEqual(launcher.settings({'game_path':absolute},MANIFEST)[0],pathlib.Path(absolute))
        with self.assertRaises(ValueError):launcher.settings({'game_path':'different.exe'},MANIFEST)
        with self.assertRaises(ValueError):launcher.settings({'unknown':1},MANIFEST)

    def test_hashes_and_profile(self):
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory)
            manifest=dict(MANIFEST,game_path='game.exe',native_build='native',native_dll='capture.dll',xr_build='xr')
            import hashlib
            for file,key in (('game.exe','game_sha256'),('native/capture.dll','native_sha256'),('xr/OpenXRPoseProbe.exe','xr_sha256')):
                path=root/file;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(file.encode())
                manifest[key]=hashlib.sha256(path.read_bytes()).hexdigest()
            profile=root/'tools/package-inspector/reloaded-profile.json';profile.parent.mkdir(parents=True)
            profile.write_text(json.dumps({'sha256':manifest['game_sha256']}))
            launcher.verify_files(manifest,root/'game.exe',root)
            (root/'native/capture.dll').write_bytes(b'wrong')
            with self.assertRaisesRegex(RuntimeError,'SHA256'):launcher.verify_files(manifest,root/'game.exe',root)

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

    def test_manifest_session_options_require_booleans(self):
        for key in launcher.SESSION_OPTIONS:
            with self.subTest(key=key),self.assertRaisesRegex(ValueError,'boolean'):
                launcher.settings({},dict(MANIFEST,**{key:'true'}))

    def test_preview_session_options_reach_runner(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend()
            manifest=dict(MANIFEST,**{key:True for key in launcher.SESSION_OPTIONS})
            with mock.patch.object(launcher,'verify_files',return_value={}),mock.patch.object(launcher.ctypes,'get_last_error',return_value=0):
                launcher.start({},manifest,backend,pathlib.Path(directory)/'index.json',sleep=lambda seconds:None)
            args=backend.runner.run_session.call_args.args[0]
            self.assertTrue(all(getattr(args,key) is True for key in launcher.SESSION_OPTIONS))

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
            def reserve(*args):
                events.append('reserve');clock[0]+=3
                return original_reserve(*args)
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

    def test_status_generation_requires_matching_session(self):
        with tempfile.TemporaryDirectory() as directory:
            backend,index,report,session,fields=self.recenter_fixture(directory)
            with mock.patch('builtins.print'):value=launcher.status(backend,index)
            self.assertEqual(value['reference_generation'],2);self.assertTrue(value['native_status_matched'])
            fields['started_qpc']='778'
            with mock.patch('builtins.print'):value=launcher.status(backend,index)
            self.assertIsNone(value['reference_generation']);self.assertFalse(value['native_status_matched'])

if __name__=='__main__':unittest.main()
