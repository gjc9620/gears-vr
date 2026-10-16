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
    def game_window(self,pid):return 12345
    def finish_owned_prime(self,manifest,identity,owner):return {'entry_restore_verified':True}
    def player(self,pid,profile):
        if not self.ready:raise RuntimeError('Menu has no Pawn')
        self.samples+=1
        return dict(pawn='0x200',controller='0x100',camera='0x400',level='0x300',timestamp=float(self.samples))

class LauncherTests(unittest.TestCase):
    def test_cli_foreground_reaches_start(self):
        for foreground in (False,True):
            argv=['launcher.py','start']+(['--foreground'] if foreground else [])
            with self.subTest(foreground=foreground),mock.patch.object(launcher.sys,'argv',argv),\
                    mock.patch.object(launcher,'Backend',return_value=object()),\
                    mock.patch.object(launcher.pathlib.Path,'read_text',return_value='{}'),\
                    mock.patch.object(launcher,'start',return_value=0) as start:
                self.assertEqual(launcher.main(),0)
                self.assertIs(start.call_args.kwargs['foreground'],foreground)

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

    def test_player_default_uses_game_resolution_and_graphics_protection(self):
        directory=pathlib.Path(__file__).parent
        current=json.loads((directory/'manifest.json').read_text())
        self.assertEqual(current,json.loads((directory/'manifest-final-path28-xr19-vd.json').read_text()))
        launcher.settings({},current)
        self.assertEqual(current['native_dll'],'GearsCaptureFinalPath28.dll')
        self.assertEqual(current['native_build'],'tools/native-capture/build-final-path28')
        self.assertEqual(current['xr_build'],'tools/openxr-probe/build-main-menu19-vd')
        self.assertEqual(launcher.hud_horizontal_fov({},current),65.)
        for key in ('startup_menu','graphics_startup_lock','graphics_menu_lock','menu_recovery','ui_hud_plane',
                    'cinematic_camera','world_recovery','hud_size_control','crosshair_assignment_observer'):
            self.assertTrue(current[key])
        for tier in ('low','high'):
            with self.assertRaisesRegex(ValueError,'Unknown configuration'):
                launcher.settings({'vr_quality':tier},current)

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

    def test_crosshair_assignment_requires_observers(self):
        for ui,anchor in ((False,False),(True,False),(False,True)):
            with self.assertRaisesRegex(ValueError,'Crosshair assignment'):
                launcher.settings({},dict(MANIFEST,crosshair_assignment_observer=True,ui_observer=ui,anchor_observer=anchor))
        launcher.settings({},dict(MANIFEST,crosshair_assignment_observer=True,ui_observer=True,anchor_observer=True))

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

    def test_output_monitor_start_failure_is_diagnostic_only(self):
        with tempfile.TemporaryDirectory() as directory:
            backend=FakeBackend();index=pathlib.Path(directory)/'index.json'
            backend.start_tiny_output_monitor.side_effect=RuntimeError('observer unavailable')
            self.assertEqual(self.start_mock(backend,index,sleep=lambda seconds:None),0)
            diagnostic=json.loads(index.with_suffix('.tiny-output.jsonl').read_text())
            self.assertEqual(diagnostic['event'],'monitor_start_error')
            self.assertNotIn('tiny_output_recovery',json.loads(index.read_text()))

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

    def test_status_generation_requires_matching_session(self):
        with tempfile.TemporaryDirectory() as directory:
            backend,index,report,session,fields=self.recenter_fixture(directory)
            with mock.patch('builtins.print'):value=launcher.status(backend,index)
            self.assertEqual(value['reference_generation'],2);self.assertTrue(value['native_status_matched'])
            fields['started_qpc']='778'
            with mock.patch('builtins.print'):value=launcher.status(backend,index)
            self.assertIsNone(value['reference_generation']);self.assertFalse(value['native_status_matched'])

if __name__=='__main__':unittest.main()
