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
    def test_field_hint_validation_precedes_return_and_reader_close(self):
        import ui_observer_config as ui
        events=[];camera=dict(local_player_address='0x1',module_base='0x1000')
        r=mock.Mock();r.base=0x1000;r.resolve.return_value=camera;r.read.return_value=b'ok'
        r.finish_discovery_field_hints.side_effect=lambda:events.append('finish')
        r.p.close.side_effect=lambda:events.append('close')
        with mock.patch.object(start,'Resolver',return_value=r),\
             mock.patch.object(ui,'build_ui_observer_config',return_value=({},{})),\
             mock.patch.object(ui,'ui_observer_ini',return_value='ui'),\
             mock.patch.object(start.time,'monotonic',return_value=0):
            result=start.acquire_observer_preparation(1,{},camera,{1:b'ok'},pathlib.Path('Unused.dll'),
                ui_observer=True,check_live=lambda:events.append('live'))
        self.assertEqual(result[4],'ui')
        self.assertEqual(events[-3:],['finish','live','close'])

    def test_changed_field_chain_discards_every_partial_certificate(self):
        import ui_observer_config as ui
        from resolve_camera import DiscoveryRegistryChanged
        readers=[];camera=dict(local_player_address='0x1',module_base='0x1000')
        def reader(*args):
            r=mock.Mock();r.base=0x1000;r.resolve.return_value=dict(camera);r.read.return_value=b'ok'
            r.finish_discovery_field_hints.side_effect=DiscoveryRegistryChanged('Reflection field chain changed')
            readers.append(r);return r
        with mock.patch.object(start,'Resolver',side_effect=reader),\
             mock.patch.object(ui,'build_ui_observer_config',return_value=({},{})) as build,\
             mock.patch.object(ui,'ui_observer_ini',return_value='discard'),\
             mock.patch.object(start.time,'monotonic',return_value=0),mock.patch.object(start.time,'sleep'),\
             contextlib.redirect_stderr(io.StringIO()),self.assertRaises(DiscoveryRegistryChanged):
            start.acquire_observer_preparation(1,{},camera,{1:b'ok'},pathlib.Path('Unused.dll'),ui_observer=True)
        self.assertEqual(len(readers),3);self.assertEqual(build.call_count,3)
        for r in readers:r.p.close.assert_called_once()

    def test_late_reticle_dependencies_reject_before_ini_or_activation(self):
        for extra in ([],['--ui-observer'],['--ui-observer','--ui-hud-plane']):
            with mock.patch.object(sys,'argv',['start_capture.py','--pid','1','--ui-reticle-late-sample',*extra]),\
                 mock.patch.object(pathlib.Path,'write_text') as write,mock.patch.object(start.subprocess,'run') as launch,\
                 contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):
                start.main()
            write.assert_not_called();launch.assert_not_called()
    def test_helpers_hide_consoles_and_preserve_argument_boundaries(self):
        command=['powershell','-NoProfile','-NonInteractive','-File',r'J:\path with spaces\Start-DebugGame.ps1',
                 '-GamePath',r'J:\game path\GOWDE-Steam.exe']
        with mock.patch.object(start.subprocess,'run',return_value='result') as run:
            self.assertEqual(start.run_hidden(command,check=True,creationflags=0x200),'result')
            self.assertEqual(run.call_args.args[0],command)
            self.assertEqual(run.call_args.kwargs['creationflags'],0x200|start.subprocess.CREATE_NO_WINDOW)
            self.assertTrue(run.call_args.kwargs['check'])

    def test_crosshair_registry_retry_uses_fresh_complete_preparation(self):
        import crosshair_assignment_config as assignment
        import anchor_config as anchor
        import ui_observer_config as ui
        baseline=dict(local_player_address='0x1',module_base='0x1000')
        resolvers=[];ticks=[0.]
        def resolver(pid,profile):
            r=mock.Mock();r.base=0x1000;r.read.return_value=b'ok'
            r.resolve.return_value=dict(baseline,camera_address=f'camera{len(resolvers)+1}')
            resolvers.append(r);return r
        def build(r,camera,**kwargs):
            self.assertIs(camera,r.resolve.return_value)
            if len(resolvers)==1:raise assignment.CrosshairRegistryChanged('Registry header changed')
            return dict(camera=camera['camera_address'])
        with contextlib.ExitStack() as stack:
            stack.enter_context(mock.patch.object(start,'Resolver',side_effect=resolver))
            anch=stack.enter_context(mock.patch.object(anchor,'build_anchor_config',side_effect=lambda r,c,**kw:({},dict(camera=c['camera_address']))))
            stack.enter_context(mock.patch.object(anchor,'anchor_ini',return_value='anchor\n'))
            ui_build=stack.enter_context(mock.patch.object(ui,'build_ui_observer_config',side_effect=lambda r,c:({},dict(camera=c['camera_address']))))
            stack.enter_context(mock.patch.object(ui,'ui_observer_ini',return_value='ui\n'))
            builder=stack.enter_context(mock.patch.object(assignment,'build_crosshair_assignment_config',side_effect=build))
            stack.enter_context(mock.patch.object(assignment,'crosshair_assignment_ini',side_effect=lambda c:'assignment:'+c['camera']))
            stack.enter_context(mock.patch.object(start.time,'monotonic',side_effect=lambda:ticks[0]))
            stack.enter_context(mock.patch.object(start.time,'sleep',side_effect=lambda duration:ticks.__setitem__(0,ticks[0]+duration)))
            stack.enter_context(contextlib.redirect_stderr(io.StringIO()))
            result=start.acquire_observer_preparation(1,{},baseline,{1:b'ok'},pathlib.Path('Unused.dll'),
                anchor_observer=True,ui_observer=True,crosshair_assignment_observer=True)
        self.assertEqual(len(resolvers),2);self.assertEqual(anch.call_count,2);self.assertEqual(ui_build.call_count,2)
        self.assertEqual(builder.call_count,2)
        self.assertEqual((result[0]['camera_address'],result[1]['camera'],result[3]['camera'],result[3]['crosshair_assignment']['camera']),('camera2',)*4)
        self.assertEqual(result[4],'ui\nassignment:camera2')
        for r in resolvers:r.resolve.assert_called_once();r.read.assert_called_once();r.p.close.assert_called_once()

    def test_crosshair_churn_preserves_attempt_deadline_and_hard_failure_limits(self):
        import crosshair_assignment_config as assignment
        import anchor_config as anchor
        import ui_observer_config as ui
        baseline=dict(local_player_address='0x1',module_base='0x1000')
        for scenario,expected in (('attempts',3),('deadline',1),('late_success',1),('identity',1),('untyped_churn_message',1)):
            with self.subTest(scenario=scenario),contextlib.ExitStack() as stack:
                ticks=[0.];resolvers=[]
                def resolver(pid,profile):
                    r=mock.Mock();r.base=0x1000;r.read.return_value=b'ok';r.resolve.return_value=baseline.copy()
                    resolvers.append(r);return r
                error=(assignment.CrosshairConfigRejected('Identity changed') if scenario=='identity' else
                       assignment.CrosshairConfigRejected('Registry header changed') if scenario=='untyped_churn_message' else
                       assignment.CrosshairRegistryChanged('Registry header changed'))
                def build(*args,**kwargs):
                    if scenario=='deadline':ticks[0]=.5
                    if scenario=='late_success':ticks[0]=60;return {}
                    raise error
                stack.enter_context(mock.patch.object(start,'Resolver',side_effect=resolver))
                stack.enter_context(mock.patch.object(anchor,'build_anchor_config',return_value=({},{})))
                stack.enter_context(mock.patch.object(anchor,'anchor_ini',return_value='anchor\n'))
                stack.enter_context(mock.patch.object(ui,'build_ui_observer_config',return_value=({},{})))
                stack.enter_context(mock.patch.object(ui,'ui_observer_ini',return_value='ui\n'))
                stack.enter_context(mock.patch.object(assignment,'build_crosshair_assignment_config',side_effect=build))
                stack.enter_context(mock.patch.object(assignment,'crosshair_assignment_ini',return_value='assignment\n'))
                stack.enter_context(mock.patch.object(start.time,'monotonic',side_effect=lambda:ticks[0]))
                sleep=stack.enter_context(mock.patch.object(start.time,'sleep'))
                stack.enter_context(contextlib.redirect_stderr(io.StringIO()))
                exception=ui.UIConfigRejected if scenario=='late_success' else assignment.CrosshairConfigRejected
                with self.assertRaises(exception) as raised:
                    start.acquire_observer_preparation(1,{},baseline,{1:b'ok'},pathlib.Path('Unused.dll'),
                        anchor_observer=True,ui_observer=True,crosshair_assignment_observer=True,
                        preparation_deadline=.5 if scenario=='deadline' else 60)
                if scenario!='late_success':self.assertIs(raised.exception,error)
                self.assertEqual(len(resolvers),expected);self.assertEqual(sleep.call_count,expected-1)
                for r in resolvers:r.p.close.assert_called_once()
                self.assertEqual(start.camera_preparation_transient(raised.exception),scenario in ('attempts','deadline'))

    def test_crosshair_assignment_dependencies_and_cli_forwarding(self):
        parser=mock.Mock();parser.error.side_effect=ValueError
        base=dict(crosshair_assignment_observer=True,ui_observer=True,anchor_observer=True,
                  continuous_session=True,origin_trial=False,stereo_xr=True,frame_link_diagnostics=True)
        for changes in ({'ui_observer':False},{'anchor_observer':False},{'continuous_session':False},{'origin_trial':True}):
            with self.assertRaises(ValueError):
                start.configure_anchor_options(types.SimpleNamespace(**dict(base,**changes)),parser)
            self.assertIn('Crosshair assignment',parser.error.call_args.args[0])
        args=types.SimpleNamespace(pid=1,build_dir=pathlib.Path('missing-crosshair-build'),dll_name='Missing.dll',
                                  stereo_units_per_metre=100,**base)
        argv=continuous.native_command(args,{'pid':100,'creation_time':1000})[2:]
        with mock.patch.object(sys,'argv',['start_capture.py',*argv]),mock.patch.object(start,'configure_anchor_options',wraps=start.configure_anchor_options) as configure,mock.patch.object(start.subprocess,'run') as launch:
            with self.assertRaisesRegex(RuntimeError,'Build capture DLL'):start.main()
            self.assertTrue(configure.call_args.args[0].crosshair_assignment_observer);launch.assert_not_called()

    def test_crosshair_assignment_uses_same_fresh_resolver_and_five_returns(self):
        import anchor_config as anchor
        import ui_observer_config as ui
        import crosshair_assignment_config as assignment
        baseline=dict(local_player_address='0x1',module_base='0x1000')
        r=mock.Mock();r.base=0x1000;r.read.return_value=b'ok';r.resolve.return_value=dict(baseline,fresh=True)
        certificate={'fresh_certificate':True}
        with mock.patch.object(start,'Resolver',return_value=r),mock.patch.object(anchor,'build_anchor_config',return_value=({},{})),mock.patch.object(anchor,'anchor_ini',return_value='anchor\n'),mock.patch.object(ui,'build_ui_observer_config',return_value=({},{'existing':'kept'})) as ui_build,mock.patch.object(ui,'ui_observer_ini',return_value='ui\n'),mock.patch.object(assignment,'build_crosshair_assignment_config',return_value=certificate) as build,mock.patch.object(assignment,'crosshair_assignment_ini',return_value='assignment\n') as ini,mock.patch.object(start.time,'monotonic',return_value=0):
            result=start.acquire_observer_preparation(1,{},baseline,{1:b'ok'},pathlib.Path('Unused.dll'),anchor_observer=True,ui_observer=True,crosshair_assignment_observer=True)
            build.assert_called_once_with(r,result[0],include_tick_roots=True,include_writer_view=True,include_aim_state=True)
            self.assertIs(ui_build.call_args.args[1],result[0]);ini.assert_called_once_with(certificate)
            build.reset_mock();ini.reset_mock();ui_build.return_value=({},{'legacy':True})
            legacy=start.acquire_observer_preparation(1,{},baseline,{1:b'ok'},pathlib.Path('Unused.dll'),anchor_observer=True,ui_observer=True)
            build.assert_not_called();ini.assert_not_called();self.assertEqual(len(legacy),5)
            self.assertNotIn('crosshair_assignment',legacy[3]);self.assertEqual(legacy[4],'ui\n')
            build.reset_mock();entry_reader=mock.Mock(return_value=b'ok')
            start.acquire_observer_preparation(1,{},baseline,{1:b'ok'},pathlib.Path('Unused.dll'),anchor_observer=True,
                ui_observer=True,crosshair_assignment_observer=True,entry_reader=entry_reader)
            self.assertIs(build.call_args.kwargs['entry_reader'],entry_reader)
        self.assertEqual(len(result),5);self.assertIs(result[0],r.resolve.return_value)
        self.assertEqual(result[4],'ui\nassignment\n');self.assertIs(result[3]['crosshair_assignment'],certificate)
        self.assertEqual(result[3]['existing'],'kept');self.assertEqual(r.p.close.call_count,3)

    def test_crosshair_certificate_failure_stops_before_ini_or_activation(self):
        import crosshair_assignment_config as assignment
        import anchor_config as anchor
        import ui_observer_config as ui
        with tempfile.TemporaryDirectory() as directory:
            build=pathlib.Path(directory);dll=build/'Mock.dll'
            dll.write_bytes(b'fixture');(build/'CaptureSelfTest.exe').write_bytes(b'fixture')
            from startup_selftests import continuous_suite
            for name,_,_ in continuous_suite():(build/name).write_bytes(b'fixture')
            (build/'target-prime-status.txt').write_text('fixture')
            args=types.SimpleNamespace(pid=1,build_dir=build,dll_name=dll.name,stereo_units_per_metre=100,
                crosshair_assignment_observer=True,ui_observer=True,anchor_observer=True,ui_hud_plane=True,ui_reticle_late_sample=True)
            argv=continuous.native_command(args,{'pid':100,'creation_time':1000})[2:]
            error=assignment.CrosshairConfigRejected('Unknown crosshair certificate identity')
            with contextlib.ExitStack() as stack:
                stack.enter_context(mock.patch.object(sys,'argv',['start_capture.py',*argv]))
                stack.enter_context(mock.patch.object(start.subprocess,'run'))
                stack.enter_context(mock.patch.object(start,'preparation_live_guard',return_value=lambda:None))
                camera=dict(local_player_address='0x1',module_base='0x1000',controller_address='0x2',
                    timestamp_address='0x3',camera_address='0x4',pov_address='0x200',offsets={'Rotation':'0x10'})
                first_resolve=stack.enter_context(mock.patch.object(start,'resolve_camera_once',side_effect=AssertionError('No preliminary startup resolve')))
                resolver=mock.Mock();resolver.base=0x1000;resolver.resolve.return_value=camera;resolver.read.return_value=b'ok'
                stack.enter_context(mock.patch.object(start,'Resolver',return_value=resolver))
                stack.enter_context(mock.patch.object(start,'observer_signatures',return_value={1:b'ok'}))
                stack.enter_context(mock.patch.object(anchor,'build_anchor_config',return_value=({},{})))
                stack.enter_context(mock.patch.object(anchor,'anchor_ini',return_value='anchor\n'))
                stack.enter_context(mock.patch.object(ui,'build_ui_observer_config',return_value=({},{})))
                stack.enter_context(mock.patch.object(ui,'ui_observer_ini',return_value='ui\n'))
                builder=stack.enter_context(mock.patch.object(assignment,'build_crosshair_assignment_config',side_effect=error))
                stack.enter_context(mock.patch.object(start,'modules',return_value={dll.name.lower():(0x1000,dll)}))
                stack.enter_context(mock.patch.object(start,'pending_prime',return_value=True))
                native=stack.enter_context(mock.patch.object(start,'k'))
                native.OpenProcess.return_value=1;native.OpenEventW.side_effect=[1,1,1,0];native.WaitForSingleObject.return_value=0x102
                remote=stack.enter_context(mock.patch.object(start,'remote_call'))
                with contextlib.redirect_stdout(io.StringIO()),self.assertRaises(assignment.CrosshairConfigRejected):start.main()
                builder.assert_called_once_with(resolver,camera,include_tick_roots=True,include_writer_view=True,include_aim_state=True)
                resolver.resolve.assert_called_once();resolver.p.close.assert_called_once();first_resolve.assert_not_called()
                remote.assert_not_called();native.VirtualAllocEx.assert_not_called();native.WriteProcessMemory.assert_not_called()
                self.assertFalse((build/'capture.ini').exists());self.assertFalse((build/'ui-observer-config.json').exists())
                # A successful fresh certificate reaches the actual Capture INI;
                # stop the mock immediately after writing, before any activation.
                def stop_after_config():
                    if (build/'capture.ini').exists():raise RuntimeError('test stops before activation')
                stack.enter_context(mock.patch.object(start,'preparation_live_guard',return_value=stop_after_config))
                stack.enter_context(mock.patch.object(assignment,'crosshair_assignment_ini',return_value='[CrosshairAssignment]\nschema=1\n'))
                builder.side_effect=None;builder.return_value={'fresh_certificate':True}
                native.OpenEventW.side_effect=[1,1,1,0]
                with contextlib.redirect_stdout(io.StringIO()),self.assertRaisesRegex(RuntimeError,'test stops before activation'):start.main()
                config=(build/'capture.ini').read_text(encoding='utf-16')
                for field in ('local_player=0x1','controller=0x2','timestamp=0x3','camera=0x4','camera_yaw=0x214'):
                    self.assertIn('\n'+field+'\n',config)
                self.assertIn('\ncrosshair_assignment_observer=1\n',config)
                self.assertIn('\nui_reticle_late_sample=1\n',config)
                self.assertIn('[CrosshairAssignment]\nschema=1\n',config)
                metadata=json.loads((build/'ui-observer-config.json').read_text())
                self.assertEqual(metadata['crosshair_assignment'],{'fresh_certificate':True})
                remote.assert_not_called();native.VirtualAllocEx.assert_not_called();native.WriteProcessMemory.assert_not_called()
                self.assertEqual(resolver.resolve.call_count,2);self.assertEqual(resolver.p.close.call_count,2)
                first_resolve.assert_not_called()

    def test_hud_size_ini_and_independent_cli_rejection(self):
        self.assertEqual(start.hud_size_ini(types.SimpleNamespace()),'')
        for value in (45,62.5,100):
            self.assertEqual(start.hud_size_ini(types.SimpleNamespace(ui_hud_plane=True,hud_horizontal_fov_degrees=value)),
                             f'ui_hud_horizontal_fov_degrees={value:.17g}\n')
        for enabled,value in ((False,65),(True,True),(True,float('nan')),(True,44),(True,101)):
            with self.assertRaises(ValueError):
                start.hud_size_ini(types.SimpleNamespace(ui_hud_plane=enabled,hud_horizontal_fov_degrees=value))
        for extra in (['--hud-horizontal-fov-degrees','65'],
                      ['--ui-hud-plane','--hud-horizontal-fov-degrees','nan'],
                      ['--ui-hud-plane','--hud-horizontal-fov-degrees','101']):
            with mock.patch.object(sys,'argv',['start_capture.py','--pid','1',*extra]),mock.patch.object(pathlib.Path,'write_text') as write,mock.patch.object(start.subprocess,'run') as launch,contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit):start.main()
                write.assert_not_called();launch.assert_not_called()

    def test_hud_layout_ini_and_cli_rejection(self):
        for size,center in ((1,1),(1.25,.75),(2,0)):
            self.assertEqual(start.hud_size_ini(types.SimpleNamespace(ui_hud_plane=True,hud_element_scale=size,hud_center_scale=center)),
                             f'ui_hud_element_scale={size:.17g}\nui_hud_center_scale={center:.17g}\n')
        for key,values in (('hud_element_scale',(True,0,2.1,float('inf'))),('hud_center_scale',(True,-.1,1.1,float('nan')))):
            for value in values:
                with self.assertRaises(ValueError):start.hud_size_ini(types.SimpleNamespace(ui_hud_plane=True,**{key:value}))
            with self.assertRaises(ValueError):start.hud_size_ini(types.SimpleNamespace(**{key:1}))
        for extra in (['--hud-element-scale','1.25'],['--hud-center-scale','.8'],
                      ['--ui-hud-plane','--hud-element-scale','nan'],['--ui-hud-plane','--hud-center-scale','1.1']):
            with mock.patch.object(sys,'argv',['start_capture.py','--pid','1',*extra]),mock.patch.object(pathlib.Path,'write_text') as write,mock.patch.object(start.subprocess,'run') as launch,contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit):start.main()
                write.assert_not_called();launch.assert_not_called()

    def test_ui_changed_read_reacquires_complete_preparation(self):
        import anchor_config as anchor
        import ui_observer_config as ui
        ticks=[0.];resolvers=[];anchored=[];ui_cameras=[]
        baseline=dict(local_player_address='0x1',module_base='0x1000')
        def resolver(pid,profile):
            r=mock.Mock();r.base=0x1000;r.read.return_value=b'ok'
            r.resolve.return_value=dict(baseline,camera_address='camera'+str(len(resolvers)+1))
            resolvers.append(r);return r
        def build_anchor(r,camera,*,prepared_camera=None):
            self.assertIs(camera,prepared_camera);self.assertIs(camera,r.resolve.return_value)
            anchored.append(camera)
            return {'camera':camera['camera_address']},{'camera':camera['camera_address']}
        def build_ui(r,camera):
            self.assertIs(camera,anchored[-1]);ui_cameras.append(camera)
            if len(ui_cameras)==1:raise ui.UIConfigRejected('UI certificate read changed')
            return {'camera':camera['camera_address']},{'camera':camera['camera_address']}
        def pause(seconds):ticks[0]+=seconds
        with mock.patch.object(start,'Resolver',side_effect=resolver),\
             mock.patch.object(anchor,'build_anchor_config',side_effect=build_anchor),\
             mock.patch.object(anchor,'anchor_ini',side_effect=lambda c:'anchor:'+c['camera']),\
             mock.patch.object(ui,'build_ui_observer_config',side_effect=build_ui),\
             mock.patch.object(ui,'ui_observer_ini',side_effect=lambda c:'ui:'+c['camera']),\
             mock.patch.object(start.time,'monotonic',side_effect=lambda:ticks[0]),\
             mock.patch.object(start,'remote_call') as remote,mock.patch.object(start.subprocess,'run') as launched,\
             contextlib.redirect_stdout(io.StringIO()),contextlib.redirect_stderr(io.StringIO()):
            value=start.wait_camera_preparation(lambda:start.acquire_observer_preparation(1,{},baseline,{1:b'ok'},pathlib.Path('Unused.dll'),
                anchor_observer=True,ui_observer=True,record_anchor_failure=False,preparation_deadline=2),
                lambda:None,2,clock=lambda:ticks[0],sleep=pause)
            remote.assert_not_called();launched.assert_not_called()
        self.assertEqual(len(resolvers),2)
        self.assertIsNot(anchored[0],anchored[1])
        self.assertEqual((value[0]['camera_address'],value[1]['camera'],value[3]['camera']),('camera2',)*3)
        self.assertEqual((value[2],value[4]),('anchor:camera2','ui:camera2'))
        for r in resolvers:r.resolve.assert_called_once();r.read.assert_called_once();r.p.close.assert_called_once()

    def test_ui_changed_read_keeps_existing_attempt_and_deadline_limits(self):
        import ui_observer_config as ui
        baseline=dict(local_player_address='0x1',module_base='0x1000')
        for shared_deadline,clock_values,expected_attempts in ((60,None,3),(.5,[0,.5],1)):
            with self.subTest(shared_deadline=shared_deadline):
                resolvers=[]
                def resolver(pid,profile):
                    r=mock.Mock();r.base=0x1000;r.read.return_value=b'ok';r.resolve.return_value=baseline.copy()
                    resolvers.append(r);return r
                error=ui.UIConfigRejected('UI certificate read changed')
                clock=mock.Mock(**({'side_effect':clock_values} if clock_values else {'return_value':0}))
                with mock.patch.object(start,'Resolver',side_effect=resolver),\
                     mock.patch.object(ui,'build_ui_observer_config',side_effect=error),\
                     mock.patch.object(start.time,'monotonic',clock),mock.patch.object(start.time,'sleep') as sleep,\
                     contextlib.redirect_stderr(io.StringIO()),self.assertRaises(ui.UIConfigRejected) as raised:
                    start.acquire_observer_preparation(1,{},baseline,{1:b'ok'},pathlib.Path('Unused.dll'),
                        ui_observer=True,preparation_deadline=shared_deadline)
                self.assertIs(raised.exception,error)
                self.assertEqual(len(resolvers),expected_attempts)
                self.assertEqual(sleep.call_count,expected_attempts-1)
                for r in resolvers:r.p.close.assert_called_once();r.resolve.assert_called_once();r.read.assert_called_once()
                # Continuous startup may reacquire within the same shared budget.
                self.assertTrue(start.camera_preparation_transient(error))

    def test_ui_permanent_rejections_and_untyped_message_are_not_retried(self):
        import ui_observer_config as ui
        UIConfigRejected=ui.UIConfigRejected
        class UnknownUIFailure(UIConfigRejected):pass
        errors=[UIConfigRejected(reason) for reason in (
            'Required UI owner class missing','UI entry bytes mismatch: dispatch','UI binding proof mismatch',
            'UI final identity mismatch','UI certificate read changed: unknown')]
        errors.append(RuntimeError('UI certificate read changed'))
        errors.append(UnknownUIFailure('UI certificate read changed'))
        for error in errors:
            with self.subTest(error=str(error)):
                acquire=mock.Mock(side_effect=error);sleep=mock.Mock()
                with self.assertRaises(type(error)) as raised:
                    start.wait_camera_preparation(acquire,lambda:None,60,clock=lambda:0,sleep=sleep)
                self.assertIs(raised.exception,error)
                acquire.assert_called_once();sleep.assert_not_called()
                baseline=dict(local_player_address='0x1',module_base='0x1000')
                r=mock.Mock();r.base=0x1000;r.read.return_value=b'ok';r.resolve.return_value=baseline
                with mock.patch.object(start,'Resolver',return_value=r),\
                     mock.patch.object(ui,'build_ui_observer_config',side_effect=error) as builder,\
                     mock.patch.object(start.time,'monotonic',return_value=0),mock.patch.object(start.time,'sleep') as slept,\
                     self.assertRaises(type(error)) as raised:
                    start.acquire_observer_preparation(1,{},baseline,{1:b'ok'},pathlib.Path('Unused.dll'),ui_observer=True)
                self.assertIs(raised.exception,error)
                builder.assert_called_once();r.resolve.assert_called_once();r.p.close.assert_called_once();slept.assert_not_called()

    def test_exhausted_timestamp_only_samples_retry_but_identity_change_does_not(self):
        import copy
        from anchor_config import AnchorSamplingRejected
        reason='Cross-frame or identity/pointer change during measurement'
        guard={k:k for k in ('controller','local_player','camera','pawn','level','outer_chain','view_target','target_proof')}
        before=dict(guard,timestamp_raw='00000000');after=dict(guard,timestamp_raw='0000803f')
        raw=dict(rejection=reason,guard_before=before,guard_after=after)
        evidence=dict(stage='sample',attempts=[dict(raw_rejection=raw)])
        self.assertTrue(start.camera_preparation_transient(AnchorSamplingRejected(reason,evidence)))
        deadline_reason='Anchor sample acquisition deadline exceeded'
        self.assertTrue(start.camera_preparation_transient(AnchorSamplingRejected(deadline_reason,evidence)))
        for key in guard:
            changed=copy.deepcopy(evidence);changed['attempts'][0]['raw_rejection']['guard_after'][key]='changed'
            self.assertFalse(start.camera_preparation_transient(AnchorSamplingRejected(reason,changed)))
            self.assertFalse(start.camera_preparation_transient(AnchorSamplingRejected(deadline_reason,changed)))
        for changed in ({},{'stage':'initial','attempts':evidence['attempts']},{'stage':'sample','attempts':[]},
                        {'stage':'sample','attempts':[{'temporary_reason':'cross_frame_timestamp'}]}):
            self.assertFalse(start.camera_preparation_transient(AnchorSamplingRejected(deadline_reason,changed)))
        self.assertFalse(start.camera_preparation_transient(RuntimeError(reason)))

    def test_nested_timestamp_then_measurement_churn_reacquires_before_activation(self):
        import copy
        from anchor_config import AnchorSamplingRejected
        reason='Cross-frame or identity/pointer change during measurement'
        guard={k:k for k in ('controller','local_player','camera','pawn','level','outer_chain','view_target','target_proof')}
        measurement=dict(rejection=reason,guard_before=dict(guard,timestamp_raw='00000000'),
                         guard_after=dict(guard,timestamp_raw='0000803f'))
        for stage in ('sample_before','sample_after'):
            evidence=dict(stage=stage,attempts=[
                dict(exhausted_guard_retryable=True,raw_rejection={
                    'rejection':'Cross-frame camera timestamp during identity checks'}),
                dict(raw_rejection=measurement)])
            error=AnchorSamplingRejected(reason,evidence)
            clock=[0.0]
            def pause(seconds):clock[0]+=seconds
            acquire=mock.Mock(side_effect=[error,'fresh complete certificate'])
            with self.subTest(stage=stage),mock.patch.object(start,'remote_call') as remote,\
                 mock.patch.object(start.subprocess,'run') as activate,contextlib.redirect_stdout(io.StringIO()):
                result=start.wait_camera_preparation(acquire,lambda:None,2,
                    clock=lambda:clock[0],sleep=pause)
                self.assertEqual(result,'fresh complete certificate')
                self.assertEqual(acquire.call_count,2)
                remote.assert_not_called();activate.assert_not_called()
            for key in guard:
                changed=copy.deepcopy(evidence)
                changed['attempts'][-1]['raw_rejection']['guard_after'][key]='changed'
                self.assertFalse(start.camera_preparation_transient(AnchorSamplingRejected(reason,changed)))
            for invalid_stage in ('initial','assembly_after','final_boundary','unknown'):
                changed=dict(evidence,stage=invalid_stage)
                self.assertFalse(start.camera_preparation_transient(AnchorSamplingRejected(reason,changed)))

    def test_registry_churn_retries_beyond_three_attempts_with_shared_deadline(self):
        import crosshair_assignment_config as assignment
        self._check_churn_whole_acquisition(lambda:assignment.CrosshairRegistryChanged('Registry header changed'))

    def test_controlled_link_churn_retries_fresh_whole_acquisition_with_shared_guards(self):
        import crosshair_assignment_config as assignment
        self._check_churn_whole_acquisition(lambda:assignment.CrosshairControlledLinkChanged(
            dict(stage='crosshair_final_links',changes=[dict(field='Weapon')])))

    def test_controlled_link_retry_requires_exact_type_not_message_or_subclass(self):
        import crosshair_assignment_config as assignment
        class UnknownLinkChange(assignment.CrosshairControlledLinkChanged):pass
        evidence=dict(stage='crosshair_final_links',changes=[dict(field='Weapon')])
        typed=assignment.CrosshairControlledLinkChanged(evidence)
        self.assertTrue(start.camera_preparation_transient(typed))
        for error in (RuntimeError(str(typed)),assignment.CrosshairConfigRejected(str(typed)),
                assignment.CrosshairConfigRejected('Certificate changed before completion: address=0x20dceabc size=8'),
                UnknownLinkChange(evidence)):
            self.assertFalse(start.camera_preparation_transient(error))
            baseline=dict(local_player_address='0x1',module_base='0x1000')
            r=mock.Mock();r.base=0x1000;r.resolve.return_value=baseline;r.read.return_value=b'ok'
            import anchor_config as anchor
            import ui_observer_config as ui
            with mock.patch.object(start,'Resolver',return_value=r) as resolve,\
                 mock.patch.object(anchor,'build_anchor_config',return_value=({},{})),\
                 mock.patch.object(anchor,'anchor_ini',return_value='anchor\n'),\
                 mock.patch.object(ui,'build_ui_observer_config',return_value=({},{})),\
                 mock.patch.object(ui,'ui_observer_ini',return_value='ui\n'),\
                 mock.patch.object(assignment,'build_crosshair_assignment_config',side_effect=error),\
                 mock.patch.object(start,'remote_call') as remote,\
                 self.assertRaises(RuntimeError) as raised:
                start.acquire_observer_preparation(1,{},baseline,{1:b'ok'},pathlib.Path('Unused.dll'),
                    anchor_observer=True,ui_observer=True,crosshair_assignment_observer=True)
            self.assertIs(raised.exception,error);resolve.assert_called_once();r.p.close.assert_called_once()
            remote.assert_not_called()

    def _check_churn_whole_acquisition(self,make_error):
        import anchor_config as anchor
        import ui_observer_config as ui
        import crosshair_assignment_config as assignment
        baseline=dict(local_player_address='0x1',module_base='0x1000')
        for outcome in ('success','deadline','identity','code','cancelled','process','local_player','module'):
            with self.subTest(outcome=outcome),contextlib.ExitStack() as stack:
                ticks=[0.];resolvers=[]
                def resolver(pid,profile):
                    r=mock.Mock();r.base=0x1000;r.read.return_value=b'ok'
                    r.resolve.return_value=dict(baseline,camera_address=str(len(resolvers)+1))
                    if len(resolvers)>=3 and outcome in ('local_player','module'):
                        r.resolve.return_value['local_player_address' if outcome=='local_player' else 'module_base']='0x2000'
                    resolvers.append(r);return r
                def build(r,camera,**kwargs):
                    ticks[0]+=3
                    if len(resolvers)<=3 or outcome=='deadline':
                        raise make_error()
                    if outcome=='identity':raise assignment.CrosshairConfigRejected('Identity changed')
                    return {'camera':camera['camera_address']}
                def check():
                    if outcome=='cancelled' and len(resolvers)==3:raise RuntimeError('Owner cancelled')
                    if outcome=='process' and len(resolvers)==3:raise RuntimeError('Game process identity changed')
                def read(address,size):
                    if outcome=='code' and len(resolvers)>3:return b'wrong'
                    return b'ok'
                stack.enter_context(mock.patch.object(start,'Resolver',side_effect=resolver))
                stack.enter_context(mock.patch.object(anchor,'build_anchor_config',side_effect=lambda r,c,**kw:({},dict(camera=c['camera_address']))))
                stack.enter_context(mock.patch.object(anchor,'anchor_ini',return_value='anchor\n'))
                stack.enter_context(mock.patch.object(ui,'build_ui_observer_config',side_effect=lambda r,c,**kw:({},dict(camera=c['camera_address']))))
                stack.enter_context(mock.patch.object(ui,'ui_observer_ini',return_value='ui\n'))
                stack.enter_context(mock.patch.object(assignment,'build_crosshair_assignment_config',side_effect=build))
                stack.enter_context(mock.patch.object(assignment,'crosshair_assignment_ini',return_value='assignment\n'))
                stack.enter_context(mock.patch.object(start.time,'monotonic',side_effect=lambda:ticks[0]))
                def pause(duration):ticks[0]+=duration
                stack.enter_context(mock.patch.object(start.time,'sleep',side_effect=pause))
                remote=stack.enter_context(mock.patch.object(start,'remote_call'))
                stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
                stack.enter_context(contextlib.redirect_stderr(io.StringIO()))
                def prepare():return start.wait_camera_preparation(lambda:start.acquire_observer_preparation(
                    1,{},baseline,{1:b'ok'},pathlib.Path('Unused.dll'),anchor_observer=True,ui_observer=True,
                    crosshair_assignment_observer=True,preparation_deadline=14,check_live=check,
                    entry_reader=lambda address,size,original:read(address,size)),check,14,clock=lambda:ticks[0],sleep=pause)
                if outcome=='success':
                    value=prepare()
                    self.assertGreater(ticks[0],10)
                    self.assertEqual((value[0]['camera_address'],value[1]['camera'],value[3]['camera'],
                                      value[3]['crosshair_assignment']['camera']),('4',)*4)
                else:
                    with self.assertRaises(RuntimeError if outcome!='deadline' else TimeoutError):prepare()
                self.assertEqual(len(resolvers),5 if outcome=='deadline' else 3 if outcome in ('cancelled','process') else 4)
                self.assertLess(ticks[0],17)
                for r in resolvers:r.p.close.assert_called_once()
                remote.assert_not_called()

    def test_cinematic_camera_policy_and_command_forwarding(self):
        parser=mock.Mock();parser.error.side_effect=ValueError
        args=types.SimpleNamespace(pid=1,build_dir=pathlib.Path('unused'),dll_name='Missing.dll',
            stereo_units_per_metre=100,anchor_observer=True,cinematic_camera=True,
            stereo_xr=True,frame_link_diagnostics=True,origin_trial=False)
        start.configure_anchor_options(args,parser)
        command=continuous.native_command(args,{'pid':100,'creation_time':1000})
        self.assertIn('--cinematic-camera',command)
        for anchor,origin in ((False,False),(True,True)):
            args.anchor_observer=anchor;args.origin_trial=origin
            with self.assertRaises(ValueError):start.configure_anchor_options(args,parser)
            with self.assertRaises(ValueError):continuous.native_command(args,{'pid':100,'creation_time':1000})

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
            args.hud_horizontal_fov_degrees=62.5 if enabled else None
            argv=continuous.native_command(args,{'pid':100,'creation_time':1000})[2:]
            with mock.patch.object(sys,'argv',['start_capture.py',*argv]),mock.patch.object(start,'configure_anchor_options',wraps=start.configure_anchor_options) as configure,mock.patch.object(start.subprocess,'run') as launch:
                with self.assertRaisesRegex(RuntimeError,'Build capture DLL'):start.main()
                self.assertEqual(configure.call_args.args[0].ui_hud_plane,enabled)
                parsed=configure.call_args.args[0]
                self.assertEqual(parsed.hud_horizontal_fov_degrees,args.hud_horizontal_fov_degrees)
                self.assertEqual(start.hud_size_ini(parsed),'ui_hud_horizontal_fov_degrees=62.5\n' if enabled else '')
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


class AnchorRegistryRetryTests(unittest.TestCase):
    def test_typed_anchor_churn_reacquires_and_remains_bounded(self):
        # Exercise the same all-certificate acquisition fixture used for UI churn.
        from test_ui_registry_reacquisition import ReacquisitionTests
        from anchor_config import AnchorRegistryChanged,AnchorSamplingRejected
        for scenario in ('success','exhausted','deadline','identity','message_only'):
            with self.subTest(scenario=scenario):
                fixture=ReacquisitionTests();fixture.setUp()
                error=AnchorRegistryChanged('Object registry changed since resolution',
                    {'stage':'assembly_after','registry_change':{'registry':'objects'}})
                if scenario=='identity':error=AnchorSamplingRejected('Certificate identity changed at final boundary',{})
                if scenario=='message_only':error=AnchorSamplingRejected('Object registry changed since resolution',{})
                def anchor_builder(r,c):
                    if scenario!='success' or r.generation==1:raise error
                    return {'camera':c['camera_address']},{'camera':c['camera_address']}
                with mock.patch.object(start,'save_anchor_sampling_failure'):
                    if scenario=='success':
                        value,count,sleeps=fixture.collect(lambda r,c:({'camera':c['camera_address']},{}),anchor_builder=anchor_builder)
                        self.assertEqual((value[0]['camera_address'],value[1]['camera'],count),('cam2','cam2',2))
                    else:
                        with self.assertRaises(type(error)):
                            fixture.collect(lambda r,c:self.fail('UI must not run'),anchor_builder=anchor_builder,
                                clock=[0,0,11] if scenario=='deadline' else None)
                self.assertEqual(len(fixture.resolvers),2 if scenario=='success' else 3 if scenario=='exhausted' else 1)
                for r in fixture.resolvers:r.p.close.assert_called_once()
                self.assertEqual(start.camera_preparation_transient(error),scenario not in ('identity','message_only'))

if __name__=='__main__':unittest.main()
