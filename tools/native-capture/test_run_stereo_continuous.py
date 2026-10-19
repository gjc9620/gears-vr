"""Mock lifecycle/identity checks only; no game, GPU, or XR processes launch."""
import contextlib
import importlib.util
import io
import json
import os
import pathlib
import subprocess
import tempfile
import types
import unittest
from unittest import mock

spec=importlib.util.spec_from_file_location('continuous_runner',pathlib.Path(__file__).with_name('run_stereo_continuous.py'))
runner=importlib.util.module_from_spec(spec);spec.loader.exec_module(runner)


def identity(pid):return {'pid':pid,'creation_time':pid*10,'image':'image-'+str(pid)}


class Control:
    def __init__(self):self.signals=[];self.closed=False
    def owner_lock(self,pid):return 1
    def close_lock(self,handle):self.closed=True
    def identity(self,pid):return identity(pid)
    def signal(self,name):self.signals.append(name);return True


def terminal(phase='stopped'):
    return dict(pid='123',continuous_session='1',phase=phase,closed='1',pending='0',active='0',
                owner_pid=str(os.getpid()),owner_creation_time=str(identity(os.getpid())['creation_time']),
                owner_exit_observed='0',owner_error_observed='0',
                state_created='1',release_returned='1',state_destroyed='1',state_flags='1',publisher_stopped='1',drain_ok='1',close_ok='1',
                cleanup_timeout='0',resources_retained='0',retained='0',entries_restored='1',started_qpc='1234',complete='1',simulator_operator='0')


class ContinuousTests(unittest.TestCase):
    def test_native_failure_snapshots_survive_incomplete_cleanup(self):
        code,state,*_=self.run_mock(clean=False)
        self.assertEqual(code,1)
        self.assertFalse(state['native_cleanup_verified'])
        self.assertIn('Native cleanup incomplete',state['native_cleanup_error'])
        self.assertTrue(all(row['saved'] for row in state['native_diagnostics'].values()))
        self.assertTrue(all(not row['acceptance_evidence'] for row in state['native_diagnostics'].values()))

    def test_native_diagnostic_copy_failure_does_not_hide_other_file_or_cleanup_error(self):
        with tempfile.TemporaryDirectory() as folder:
            root=pathlib.Path(folder);build=root/'native';build.mkdir()
            (build/'capture-status.txt').write_text('owned_ui_batch faults=1 first_fault=7\n')
            state={'native_cleanup_error':'original failure','native_cleanup_verified':False}
            self.assertFalse(runner.preserve_native_diagnostics(build,root/'session.json',state))
            self.assertFalse(state['native_diagnostics']['session-status.txt']['saved'])
            self.assertIn('error',state['native_diagnostics']['session-status.txt'])
            self.assertTrue(state['native_diagnostics']['capture-status.txt']['saved'])
            self.assertEqual((root/'session.native-status.txt').read_text(),'owned_ui_batch faults=1 first_fault=7\n')
            self.assertEqual(state['native_cleanup_error'],'original failure')
            self.assertFalse(state['native_cleanup_verified'])

    def test_native_diagnostics_are_saved_when_cleanup_signal_raises(self):
        with mock.patch.object(Control,'signal',side_effect=OSError('signal failed')):
            code,state,*_=self.run_mock()
        self.assertEqual(code,1)
        self.assertFalse(state['native_cleanup_verified'])
        self.assertIn('signal failed',state['native_cleanup_error'])
        self.assertTrue(all(row['saved'] for row in state['native_diagnostics'].values()))

    def test_late_reticle_private_cli_and_native_only_forwarding(self):
        base=['start','--pid','123','--build-dir','native','--dll-name','Continuous.dll','--xr-build-dir','xr']
        flag='--ui-reticle-late-sample'
        with mock.patch.object(runner,'run_session',return_value=0) as run:
            runner.main(base);args=run.call_args.args[0]
            self.assertFalse(args.ui_reticle_late_sample)
            self.assertNotIn(flag,runner.native_command(args,identity(100)))
            for extra in ([],['--ui-observer','--anchor-observer'],['--ui-hud-plane','--anchor-observer']):
                run.reset_mock()
                with contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):runner.main(base+[flag,*extra])
                run.assert_not_called()
            runner.main(base+[flag,'--ui-hud-plane','--ui-observer','--anchor-observer'])
            args=run.call_args.args[0]
            self.assertEqual(runner.native_command(args,identity(100)).count(flag),1)
            self.assertNotIn(flag,runner.xr_command(args,pathlib.Path('unused.csv'),identity(100)))

    def test_late_reticle_direct_caller_rejects_before_process_or_lock(self):
        for ui,hud in ((False,False),(True,False),(False,True)):
            args=types.SimpleNamespace(ui_reticle_late_sample=True,ui_observer=ui,ui_hud_plane=hud)
            control=mock.Mock()
            with mock.patch.object(runner,'WindowsControl') as create,mock.patch.object(runner.subprocess,'Popen') as spawn:
                with self.assertRaisesRegex(ValueError,'Late reticle'):runner.run_session(args,control)
                control.owner_lock.assert_not_called();create.assert_not_called();spawn.assert_not_called()

    def test_camera_closer_cli_validation_and_forwarding(self):
        base=['start','--pid','123','--build-dir','native','--dll-name','Continuous.dll','--xr-build-dir','xr']
        guards=['--anchor-observer','--cinematic-camera']
        flag='--camera-closer-metres'
        with mock.patch.object(runner,'run_session',return_value=0) as run:
            runner.main(base)
            self.assertNotIn(flag,runner.native_command(run.call_args.args[0],identity(100)))
            for extra in ([flag,'.3'],*[guards+[flag,value] for value in ('nan','inf','-.01','.51')],guards+[flag,'.3','--origin-trial']):
                run.reset_mock()
                with contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):runner.main(base+extra)
                run.assert_not_called()
            runner.main(base+guards+[flag,'.3'])
            command=runner.native_command(run.call_args.args[0],identity(100))
            self.assertEqual(command[command.index(flag)+1],'0.3')

    def test_crosshair_assignment_dependency_and_forwarding(self):
        base=['start','--pid','123','--build-dir','native','--dll-name','Continuous.dll','--xr-build-dir','xr']
        flag='--crosshair-assignment-observer'
        with mock.patch.object(runner,'run_session',return_value=0) as run:
            runner.main(base);self.assertFalse(run.call_args.args[0].crosshair_assignment_observer)
            self.assertNotIn(flag,runner.native_command(run.call_args.args[0],identity(100)))
            for extra in ([],['--ui-observer'],['--anchor-observer'],['--ui-observer','--anchor-observer','--origin-trial']):
                run.reset_mock()
                with contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):runner.main(base+[flag,*extra])
                run.assert_not_called()
            runner.main(base+[flag,'--ui-observer','--anchor-observer'])
            args=run.call_args.args[0]
            self.assertEqual(runner.native_command(args,identity(100)).count(flag),1)
            self.assertNotIn(flag,runner.xr_command(args,pathlib.Path('x.csv'),identity(100)))

    def test_hud_layout_cli_validation_and_native_forwarding(self):
        base=['start','--pid','123','--build-dir','native','--dll-name','Continuous.dll','--xr-build-dir','xr']
        hud=['--ui-hud-plane','--ui-observer','--anchor-observer']
        with mock.patch.object(runner,'run_session',return_value=0) as run:
            runner.main(base)
            command=runner.native_command(run.call_args.args[0],identity(100))
            for flag,values in (('--hud-element-scale',('nan','inf','.99','2.01','true')),
                                ('--hud-center-scale',('nan','inf','-.01','1.01','true'))):
                self.assertNotIn(flag,command)
                for extra in ([flag,'1'],*[hud+[flag,value] for value in values]):
                    run.reset_mock()
                    with contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):runner.main(base+extra)
                    run.assert_not_called()
            runner.main(base+hud+['--hud-element-scale','1.2','--hud-center-scale','.8'])
            args=run.call_args.args[0];command=runner.native_command(args,identity(100))
            for key,value in (('hud_element_scale',1.2),('hud_center_scale',.8)):
                flag='--'+key.replace('_','-')
                self.assertEqual(command[command.index(flag)+1],str(value))
                self.assertNotIn(flag,runner.xr_command(args,pathlib.Path('x.csv'),identity(100)))
                setattr(args,key,True)
                with self.assertRaises(ValueError):runner.native_command(args,identity(100))
                setattr(args,key,value)

    def test_hud_layout_session_report_and_native_command(self):
        code,state,spawn,native,control,child=self.run_mock(ui_hud_plane=True,hud_layout=(1.2,.8))
        self.assertEqual(code,0)
        for key,value in (('hud_element_scale',1.2),('hud_center_scale',.8)):
            self.assertEqual(state[key],value)
            command=native.call_args_list[0].args[0];flag='--'+key.replace('_','-')
            self.assertEqual(command[command.index(flag)+1],str(value))
        _,legacy,_,native,_,_=self.run_mock()
        for key in ('hud_element_scale','hud_center_scale'):
            self.assertNotIn(key,legacy)
            self.assertNotIn('--'+key.replace('_','-'),native.call_args_list[0].args[0])

    def test_hud_size_cli_validation_and_native_forwarding(self):
        base=['start','--pid','123','--build-dir','native','--dll-name','Continuous.dll','--xr-build-dir','xr']
        hud=['--ui-hud-plane','--ui-observer','--anchor-observer']
        with mock.patch.object(runner,'run_session',return_value=0) as run:
            for extra in (['--hud-horizontal-fov-degrees','65'],
                          *[hud+['--hud-horizontal-fov-degrees',v] for v in ('nan','inf','44.99','100.01','true')]):
                run.reset_mock()
                with contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):runner.main(base+extra)
                run.assert_not_called()
            runner.main(base+hud+['--hud-horizontal-fov-degrees','62.5'])
            args=run.call_args.args[0]
            command=runner.native_command(args,identity(100))
            self.assertEqual(command[command.index('--hud-horizontal-fov-degrees')+1],'62.5')
            self.assertNotIn('--hud-horizontal-fov-degrees',runner.xr_command(args,pathlib.Path('x.csv'),identity(100)))
            args.hud_horizontal_fov_degrees=True
            with self.assertRaises(ValueError):runner.native_command(args,identity(100))

    def test_quiet_environment_is_child_only_and_keeps_runtime_selection(self):
        parent={'XR_RUNTIME_JSON':'chosen-runtime.json','PATH':'test-path'}
        child=runner.xr_environment(parent)
        self.assertEqual(child,{**parent,'XR_SIMULATOR_NO_ACTIVATE':'1'})
        self.assertNotIn('XR_SIMULATOR_NO_ACTIVATE',parent)
        inherited={'XR_SIMULATOR_NO_ACTIVATE':'0','XR_RUNTIME_JSON':'chosen-runtime.json'}
        self.assertEqual(runner.xr_environment(inherited),
                         {'XR_SIMULATOR_NO_ACTIVATE':'1','XR_RUNTIME_JSON':'chosen-runtime.json'})
        self.assertEqual(inherited['XR_SIMULATOR_NO_ACTIVATE'],'0')

    def test_menu_recovery_command_is_bound_to_game_creation_and_startup_window(self):
        args=types.SimpleNamespace(pid=123,xr_build_dir=pathlib.Path('xr'),menu_recovery=True,
            startup_menu_window=456,menu_state_file=pathlib.Path('state.menu.bin'),menu_game_creation=789)
        cmd=runner.xr_command(args,pathlib.Path('capture.csv'),identity(100))
        self.assertEqual(cmd[cmd.index('--menu-game-creation')+1],'789')
        self.assertEqual(cmd[cmd.index('--menu-state-file')+1],'state.menu.bin')
        self.assertNotIn('--movie-display',cmd)
        args.movie_display=True
        self.assertIn('--movie-display',runner.xr_command(args,pathlib.Path('capture.csv'),identity(100)))
        args.startup_menu_window=0
        with self.assertRaisesRegex(ValueError,'requires startup'):runner.xr_command(args,pathlib.Path('capture.csv'),identity(100))

    def test_world_recovery_requires_full_observers_and_trusted_digest_before_lock(self):
        args=types.SimpleNamespace(world_recovery=True,anchor_observer=True,ui_observer=True,
                                  cinematic_camera=True,native_expected_sha256='a'*64)
        runner.validate_ui_hud_plane(args)
        for key,value in (('anchor_observer',False),('ui_observer',False),('cinematic_camera',False),
                          ('native_expected_sha256','')):
            bad=types.SimpleNamespace(**{**vars(args),key:value})
            control=mock.Mock()
            with self.assertRaises(ValueError):runner.run_session(bad,control)
            control.owner_lock.assert_not_called()

    def test_snapshot_only_signals_enabled_live_owned_xr(self):
        state=dict(schema=1,kind='continuous_strict_stereo',phase='running',gpu_snapshot_requested=True,
                   owner_identity=identity(100),game_identity=identity(123),xr_identity=identity(200))
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'session.json';path.write_text(json.dumps(state))
            control=Control()
            with contextlib.redirect_stdout(io.StringIO()):self.assertEqual(runner.request_snapshot(path,control),0)
            self.assertEqual(control.signals,['Local\\GearsVR.XR.Snapshot.200'])
            for key,value in (('phase','stopped'),('gpu_snapshot_requested',False)):
                path.write_text(json.dumps(dict(state,**{key:value})));control.signals.clear()
                with self.assertRaises(RuntimeError):runner.request_snapshot(path,control)
                self.assertEqual(control.signals,[])
            bad=dict(state,xr_identity=dict(identity(200),creation_time=1));path.write_text(json.dumps(bad));control.signals.clear()
            with self.assertRaises(RuntimeError):runner.request_snapshot(path,control)
            self.assertEqual(control.signals,[])

    def test_hud_plane_cli_default_dependencies_and_native_only_forwarding(self):
        base=['start','--pid','123','--build-dir','native','--dll-name','Continuous.dll','--xr-build-dir','xr']
        with mock.patch.object(runner,'run_session',return_value=0) as run:
            runner.main(base);self.assertFalse(run.call_args.args[0].ui_hud_plane)
            for extra in ([],['--ui-observer'],['--anchor-observer']):
                run.reset_mock()
                with contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):
                    runner.main(base+['--ui-hud-plane',*extra])
                run.assert_not_called()
            runner.main(base+['--ui-hud-plane','--ui-observer','--anchor-observer'])
            args=run.call_args.args[0]
            native=runner.native_command(args,identity(100))
            for flag in ('--ui-hud-plane','--ui-observer','--anchor-observer','--continuous-session'):
                self.assertEqual(native.count(flag),1)
            self.assertNotIn('--ui-hud-plane',runner.xr_command(args,pathlib.Path('capture.csv'),identity(100)))

    def test_hud_plane_direct_runner_rejects_before_lock_or_spawn(self):
        for ui,anchor in ((False,False),(True,False),(False,True)):
            args=types.SimpleNamespace(ui_hud_plane=True,ui_observer=ui,anchor_observer=anchor)
            control=mock.Mock()
            with mock.patch.object(runner,'WindowsControl') as create,mock.patch.object(runner.subprocess,'Popen') as spawn:
                with self.assertRaisesRegex(ValueError,'HUD plane'):runner.run_session(args,control)
                control.owner_lock.assert_not_called();create.assert_not_called();spawn.assert_not_called()

    def test_hud_plane_report_defaults_and_explicit_enable(self):
        for enabled in (False,True):
            _,state,_,_,_,_=self.run_mock(ui_hud_plane=enabled)
            self.assertEqual(state['ui_hud_plane_enabled'],enabled)
            self.assertFalse(state['ui_reticle_late_sample_enabled'])

    def test_xr_image_ticket_evidence_explicit_or_origin_only(self):
        args=types.SimpleNamespace(pid=123,build_dir=pathlib.Path('native'),dll_name='Continuous.dll',
            xr_build_dir=pathlib.Path('xr'),stereo_units_per_metre=100)
        for anchor,origin,explicit in ((False,False,False),(True,False,False),(True,True,False),(False,False,True),(True,True,True)):
            args.anchor_observer=anchor;args.origin_trial=origin;args.image_ticket_diagnostics=explicit
            with self.subTest(anchor=anchor,origin=origin,explicit=explicit):
                xr=runner.xr_command(args,pathlib.Path('capture.csv'),identity(100))
                self.assertEqual(xr.count('--image-ticket-diagnostics'),int(origin or explicit))
                self.assertNotIn('--image-ticket-diagnostics',runner.native_command(args,identity(100)))
                self.assertIn('--stereo-game-pid',xr)

    def test_image_ticket_diagnostics_cli_default_off_explicit_without_origin(self):
        base=['start','--pid','123','--build-dir','native','--dll-name','Continuous.dll','--xr-build-dir','xr']
        with mock.patch.object(runner,'run_session',return_value=0) as run:
            runner.main(base);self.assertFalse(run.call_args.args[0].image_ticket_diagnostics)
            runner.main(base+['--image-ticket-diagnostics']);args=run.call_args.args[0]
            self.assertTrue(args.image_ticket_diagnostics);self.assertFalse(args.origin_trial);self.assertFalse(args.anchor_observer)

    def test_image_ticket_diagnostics_report_actual_enabled(self):
        for origin,explicit in ((False,False),(True,False),(False,True),(True,True)):
            with self.subTest(origin=origin,explicit=explicit):
                _,state,spawn,_,_,_=self.run_mock(origin_trial=origin,image_ticket_diagnostics=explicit)
                self.assertEqual(state['image_ticket_diagnostics_enabled'],origin or explicit)
                self.assertEqual(spawn.args[0].count('--image-ticket-diagnostics'),int(origin or explicit))

    def test_origin_trial_cli_default_off_dependency_and_native_only_forwarding(self):
        args=types.SimpleNamespace(pid=123,build_dir=pathlib.Path('native'),dll_name='Continuous.dll',
            xr_build_dir=pathlib.Path('xr'),stereo_units_per_metre=100)
        self.assertNotIn('--origin-trial',runner.native_command(args,identity(100)))
        args.origin_trial=True;args.anchor_observer=True
        command=runner.native_command(args,identity(100))
        for flag in ('--origin-trial','--anchor-observer','--continuous-session','--stereo-gpu-publish'):
            self.assertEqual(command.count(flag),1)
        self.assertNotIn('--origin-trial',runner.xr_command(args,pathlib.Path('capture.csv'),identity(100)))
        base=['start','--pid','123','--build-dir','native','--dll-name','Continuous.dll','--xr-build-dir','xr']
        with mock.patch.object(runner,'run_session',return_value=0) as run:
            runner.main(base);self.assertFalse(run.call_args.args[0].origin_trial)
            run.reset_mock()
            with contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):runner.main(base+['--origin-trial'])
            run.assert_not_called()
            runner.main(base+['--origin-trial','--anchor-observer'])
            self.assertTrue(run.call_args.args[0].origin_trial)

    def test_origin_trial_report_tracks_explicit_flag(self):
        for enabled in (False,True):
            with self.subTest(enabled=enabled):
                _,state,_,native,_,_=self.run_mock(origin_trial=enabled)
                self.assertEqual(state['origin_trial_enabled'],enabled)
                commands=[call.args[0] for call in native.call_args_list if call.args[0][1].endswith('start_capture.py')]
                self.assertEqual(len(commands),1)
                self.assertEqual(commands[0].count('--origin-trial'),int(enabled))

    def test_diagnostics_flags_are_default_off_and_forwarded_individually(self):
        args=types.SimpleNamespace(pid=123,build_dir=pathlib.Path('native'),dll_name='Continuous.dll',
            xr_build_dir=pathlib.Path('xr'),stereo_units_per_metre=100)
        owner=identity(100)
        for outage,shutdown in ((False,False),(True,False),(False,True),(True,True)):
            with self.subTest(outage=outage,shutdown=shutdown):
                if outage or shutdown:
                    args.outage_diagnostics=outage;args.shutdown_diagnostics=shutdown
                xr=runner.xr_command(args,pathlib.Path('capture.csv'),owner)
                native=runner.native_command(args,owner)
                self.assertEqual(xr.count('--outage-diagnostics'),int(outage))
                self.assertEqual(xr.count('--shutdown-diagnostics'),int(shutdown))
                self.assertEqual(native.count('--outage-diagnostics'),int(outage))
                self.assertNotIn('--shutdown-diagnostics',native)

    def test_anchor_observer_native_only_default_off_and_alias(self):
        args=types.SimpleNamespace(pid=123,build_dir=pathlib.Path('native'),dll_name='Continuous.dll',
            xr_build_dir=pathlib.Path('xr'),stereo_units_per_metre=100)
        self.assertNotIn('--anchor-observer',runner.native_command(args,identity(100)))
        args.anchor_observer=True
        self.assertEqual(runner.native_command(args,identity(100)).count('--anchor-observer'),1)
        self.assertNotIn('--anchor-observer',runner.xr_command(args,pathlib.Path('capture.csv'),identity(100)))
        base=['start','--pid','123','--build-dir','native','--dll-name','Continuous.dll','--xr-build-dir','xr']
        with mock.patch.object(runner,'run_session',return_value=0) as run:
            runner.main(base);self.assertFalse(run.call_args.args[0].anchor_observer)
            for option in ('--anchor-observer','--anchor-diagnostics'):
                runner.main(base+[option]);self.assertTrue(run.call_args.args[0].anchor_observer)

    def test_diagnostics_cli_defaults_and_opt_in(self):
        base=['start','--pid','123','--build-dir','native','--dll-name','Continuous.dll','--xr-build-dir','xr']
        with mock.patch.object(runner,'run_session',return_value=0) as run:
            self.assertEqual(runner.main(base),0)
            args=run.call_args.args[0]
            self.assertFalse(args.outage_diagnostics);self.assertFalse(args.shutdown_diagnostics)
            self.assertEqual(runner.main(base+['--outage-diagnostics','--shutdown-diagnostics']),0)
            args=run.call_args.args[0]
            self.assertTrue(args.outage_diagnostics);self.assertTrue(args.shutdown_diagnostics)

    def cleanup_child(self,waits=(0,),signal=True,poll=None,owned_pid=456):
        child=mock.Mock(pid=456,returncode=waits[-1] if isinstance(waits[-1],int) else None)
        child.poll.return_value=poll
        child.wait.side_effect=waits
        control=Control()
        control.identity=mock.Mock(side_effect=AssertionError('Cleanup must not reopen a PID'))
        control.signal=mock.Mock(side_effect=signal) if isinstance(signal,Exception) else mock.Mock(return_value=signal)
        state={}
        runner.cleanup_owned_xr(child,owned_pid,123,30,control,state)
        return child,control,state

    def test_owned_cleanup_poll_to_exit_uses_handle_not_identity(self):
        child,control,state=self.cleanup_child()
        self.assertEqual(state['xr_exit_code'],0)
        self.assertNotIn('xr_cleanup_error',state)
        self.assertEqual(state['xr_cleanup_actions'],['stop_signaled','wait'])
        control.identity.assert_not_called()
        child.terminate.assert_not_called();child.kill.assert_not_called()
        child.wait.assert_called_once_with(timeout=30)

    def test_owned_cleanup_closed_event_still_waits_without_force(self):
        child,control,state=self.cleanup_child(signal=False)
        self.assertEqual(state['xr_cleanup_actions'],['stop_event_absent','wait'])
        self.assertEqual(state['xr_exit_code'],0)
        self.assertIsNone(state['xr_forced_cleanup_reason'])
        child.terminate.assert_not_called();child.kill.assert_not_called()

    def test_owned_cleanup_preserves_natural_nonzero_exit(self):
        for poll in (None,7):
            child,control,state=self.cleanup_child(waits=(7,),poll=poll)
            self.assertEqual(state['xr_exit_code'],7)
            self.assertNotIn('xr_cleanup_error',state)
            child.terminate.assert_not_called();child.kill.assert_not_called()
            if poll is not None:control.signal.assert_not_called()

    def test_owned_cleanup_only_timeout_escalates_and_records_actions(self):
        timeout=subprocess.TimeoutExpired('xr',30)
        child,control,state=self.cleanup_child(waits=(timeout,subprocess.TimeoutExpired('xr',5),1))
        self.assertEqual(state['xr_exit_code'],1)
        self.assertEqual(state['xr_forced_cleanup_reason'],'cleanup_wait_timeout')
        self.assertEqual(state['xr_cleanup_actions'],['stop_signaled','wait','terminate_owned_child','kill_owned_child'])
        self.assertIn('TimeoutExpired',state['xr_cleanup_error'])
        child.terminate.assert_called_once();child.kill.assert_called_once()

    def test_owned_cleanup_other_wait_error_does_not_force(self):
        child,control,state=self.cleanup_child(waits=(OSError('wait failed'),))
        self.assertIn('wait failed',state['xr_cleanup_error'])
        self.assertIsNone(state['xr_forced_cleanup_reason'])
        child.terminate.assert_not_called();child.kill.assert_not_called()

    def test_owned_cleanup_signal_error_waits_without_force(self):
        child,control,state=self.cleanup_child(signal=RuntimeError('signal failed'))
        self.assertEqual(state['xr_exit_code'],0)
        self.assertIn('signal failed',state['xr_stop_signal_error'])
        child.terminate.assert_not_called();child.kill.assert_not_called()

    def test_owned_cleanup_pid_invariant_failure_never_controls_child(self):
        child,control,state=self.cleanup_child(owned_pid=999)
        self.assertIn('invariant',state['xr_cleanup_error'])
        child.poll.assert_not_called();child.wait.assert_not_called()
        control.signal.assert_not_called();child.terminate.assert_not_called()

    def test_atomic_state_retries_only_brief_sharing_errors(self):
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'owner.json'
            original=runner.os.replace
            calls=[]
            def replace(source,target):
                calls.append(1)
                if len(calls)<3:raise PermissionError('sharing lock')
                return original(source,target)
            with mock.patch.object(runner.os,'replace',side_effect=replace),mock.patch.object(runner.time,'sleep'):
                runner.write_state(path,{'phase':'running'})
            self.assertEqual(len(calls),3)
            self.assertEqual(json.loads(path.read_text())['phase'],'running')
            with mock.patch.object(runner.os,'replace',side_effect=OSError('disk failure')),mock.patch.object(runner.time,'sleep') as sleep:
                with self.assertRaises(OSError):runner.write_state(path,{})
                sleep.assert_not_called()

    def test_terminal_requires_every_cleanup_gate(self):
        self.assertTrue(runner.native_stopped_clean(terminal()))
        for key,value in [('phase','incomplete'),('closed','0'),('pending','1'),('active','1'),
                          ('release_returned','0'),('state_destroyed','0'),('drain_ok','0'),('entries_restored','0'),('retained','1'),
                          ('close_ok','0'),('publisher_stopped','0'),('cleanup_timeout','1'),('resources_retained','1'),('simulator_operator','1')]:
            self.assertFalse(runner.native_stopped_clean(dict(terminal(),**{key:value})),key)

    def test_fresh_matching_native_status_only(self):
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'status.txt'
            path.write_text(' '.join(k+'='+v for k,v in terminal().items())+'\n')
            floor=path.stat().st_mtime_ns
            self.assertIsNone(runner.read_session_status(path,123,floor))
            self.assertIsNone(runner.read_session_status(path,124,0))
            self.assertEqual(runner.read_session_status(path,123,0)['phase'],'stopped')

    def test_session_read_diagnostics_classify_same_read_without_changing_legacy_results(self):
        valid='continuous_session=1 pid=123 started_qpc=456 complete=1\n'
        cases=[('', 'empty_file'),(valid.rstrip(),'missing_final_newline'),
               (valid.replace('complete=1','complete=0'),'complete_marker_missing_or_invalid'),
               (valid.replace('started_qpc=456','started_qpc=0'),'started_qpc_missing_or_nonpositive'),
               (valid.replace('started_qpc=456','started_qpc=private-secret'),'invalid_started_qpc'),
               (valid.replace('pid=123','pid=124'),'pid_mismatch'),
               (valid.replace('continuous_session=1','continuous_session=0'),'continuous_session_mismatch'),
               (valid,'accepted')]
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'status.txt'
            for text,reason in cases:
                with self.subTest(reason=reason):
                    path.write_text(text);diagnostic={'old':'must be removed'}
                    previous=runner.read_session_status(path,123,0)
                    value=runner.read_session_status(path,123,0,diagnostic=diagnostic)
                    self.assertEqual(value,previous);self.assertEqual(diagnostic['reason'],reason)
                    self.assertNotIn('old',diagnostic)
                    self.assertNotIn('private-secret',json.dumps(diagnostic))
                    self.assertEqual(diagnostic['text_length'],len(text))
            diagnostic={}
            self.assertIsNone(runner.read_session_status(path,123,path.stat().st_mtime_ns,diagnostic=diagnostic))
            self.assertEqual(diagnostic,{'reason':'stale_file'})

    def test_path_status_closure_keeps_same_read_reason_and_never_retries(self):
        import path_choice_position_monitor
        for reason in ('empty_file','io_error','unknown','accepted'):
            with self.subTest(reason=reason), tempfile.TemporaryDirectory() as folder:
                args=types.SimpleNamespace(path_choice_center=True,build_dir=pathlib.Path(folder),pid=123)
                state={'native_status':{'path_position_service_compiled':'1'}}
                fields={'phase':'running'}
                def reading(path,pid,floor,*,diagnostic):
                    if reason!='unknown':diagnostic['reason']=reason
                    return fields if reason=='accepted' else None
                with mock.patch.object(path_choice_position_monitor,'PathChoicePositionMonitor') as center, \
                     mock.patch.object(runner,'read_session_status',side_effect=reading) as read:
                    runner.start_path_choice_position(args,state,pathlib.Path(folder)/'status.txt',7,Control())
                    callback=center.call_args.args[3]
                    if reason=='accepted':self.assertIs(callback(),fields)
                    else:
                        with self.assertRaisesRegex(ValueError,'Missing native status') as failure:callback()
                        self.assertEqual(failure.exception.native_status_diagnostic,{'reason':reason})
                        self.assertIn('"reason":"'+reason+'"',str(failure.exception))
                    read.assert_called_once()
                    self.assertEqual(read.call_args.args,(pathlib.Path(folder)/'status.txt',123,7))
                    self.assertNotIn('path_choice_center_start_error',state)

    def test_session_read_diagnostic_does_not_reread_empty_file_or_retry_io_failure(self):
        path=pathlib.Path('unopened-fixture.txt');stream=mock.Mock()
        stream.read.side_effect=['','continuous_session=1 pid=123 started_qpc=456 complete=1\n']
        resource=mock.MagicMock();resource.__enter__.return_value=stream;resource.__exit__.return_value=False
        diagnostic={}
        with mock.patch.object(runner,'open_session_status',return_value=resource) as opened, \
             mock.patch.object(runner.os,'fstat',return_value=types.SimpleNamespace(st_mtime_ns=10)):
            self.assertIsNone(runner.read_session_status(path,123,0,diagnostic=diagnostic))
        opened.assert_called_once_with(path);stream.read.assert_called_once()
        self.assertEqual(diagnostic['reason'],'empty_file');resource.__exit__.assert_called_once()
        for stage in ('open','stat','read'):
            with self.subTest(stage=stage):
                stream=mock.Mock();resource=mock.MagicMock()
                resource.__enter__.return_value=stream;resource.__exit__.return_value=False
                failure=PermissionError(13,'PRIVATE_CONTENT')
                with mock.patch.object(runner,'open_session_status',return_value=resource) as opened, \
                     mock.patch.object(runner.os,'fstat',return_value=types.SimpleNamespace(st_mtime_ns=10)) as stat:
                    if stage=='open':opened.side_effect=failure
                    elif stage=='stat':stat.side_effect=failure
                    else:stream.read.side_effect=failure
                    self.assertIsNone(runner.read_session_status(path,123,0,diagnostic=diagnostic))
                self.assertEqual(diagnostic['reason'],'io_error');self.assertEqual(diagnostic['stage'],stage)
                self.assertEqual(diagnostic['exception_type'],'PermissionError')
                self.assertNotIn('PRIVATE_CONTENT',json.dumps(diagnostic));opened.assert_called_once()
                self.assertEqual(stream.read.call_count,1 if stage=='read' else 0)
                self.assertEqual(resource.__exit__.call_count,0 if stage=='open' else 1)

    @staticmethod
    def publish_status_fixture(temporary,destination):
        # File-only test of the same Windows atomic namespace operation used by
        # AtomicFile. No retry and no process/window access.
        if os.name!='nt':os.replace(temporary,destination);return
        c=runner.ctypes;w=runner.wintypes;kernel=c.WinDLL('kernel32',use_last_error=True)
        kernel.CreateFileW.argtypes=[w.LPCWSTR,w.DWORD,w.DWORD,c.c_void_p,w.DWORD,w.DWORD,w.HANDLE]
        kernel.CreateFileW.restype=w.HANDLE
        kernel.SetFileInformationByHandle.argtypes=[w.HANDLE,c.c_int,c.c_void_p,w.DWORD]
        kernel.SetFileInformationByHandle.restype=w.BOOL
        kernel.CloseHandle.argtypes=[w.HANDLE]
        name=str(destination.resolve())
        class Rename(c.Structure):
            _fields_=[('flags',w.DWORD),('root',w.HANDLE),('length',w.DWORD),('name',c.c_wchar*len(name))]
        info=Rename();info.flags=1|2;info.length=len(name.encode('utf-16-le'));info.name=name
        handle=kernel.CreateFileW(str(temporary),0x10000,1|2|4,None,3,0x80,None)
        if handle==w.HANDLE(-1).value:raise c.WinError(c.get_last_error())
        try:
            if not kernel.SetFileInformationByHandle(handle,22,c.byref(info),c.sizeof(info)):
                raise c.WinError(c.get_last_error())
        finally:kernel.CloseHandle(handle)

    def test_single_status_handle_keeps_old_generation_during_atomic_replacement(self):
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'status.txt';temporary=path.with_name('status.tmp')
            a='continuous_session=1 pid=123 started_qpc=456 generation=A complete=1\n'
            b=a.replace('generation=A','generation=B');path.write_text(a);temporary.write_text(b)
            with runner.open_session_status(path) as old:
                self.publish_status_fixture(temporary,path)
                self.assertEqual(old.read(),a)
                self.assertEqual(runner.read_session_status(path,123,0)['generation'],'B')
            self.assertTrue(old.closed)
            try:
                with runner.open_session_status(path) as stream:raise RuntimeError('test close')
            except RuntimeError:pass
            self.assertTrue(stream.closed)

    def test_status_mtime_and_contents_come_from_same_open_handle(self):
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'status.txt';temporary=path.with_name('status.tmp')
            a='continuous_session=1 pid=123 started_qpc=456 generation=A complete=1\n'
            path.write_text(a);temporary.write_text(a.replace('generation=A','generation=B'))
            old_time=1600000000000000000;new_time=old_time+2000000000
            os.utime(path,ns=(old_time,old_time));os.utime(temporary,ns=(new_time,new_time))
            original=runner.open_session_status
            @contextlib.contextmanager
            def publishing(current):
                with original(current) as stream:
                    self.publish_status_fixture(temporary,path)
                    yield stream
            diagnostic={}
            with mock.patch.object(runner,'open_session_status',publishing):
                self.assertIsNone(runner.read_session_status(path,123,old_time+1000000000,diagnostic=diagnostic))
            self.assertEqual(diagnostic['reason'],'stale_file')
            self.assertEqual(runner.read_session_status(path,123,old_time+1000000000)['generation'],'B')

    def run_mock(self,clean=True,xr_timeout=False,early_exit=False,cancel_at=0,diagnostics=False,origin_trial=False,image_ticket_diagnostics=False,startup_callback=None,startup_menu=False,panel_submitted=True,ui_hud_plane=False,first_stereo=False,world_recovery=False,crosshair=False,hud_layout=None,path_center=False):
        with tempfile.TemporaryDirectory() as folder:
            root=pathlib.Path(folder);build=root/'native';build.mkdir()
            (build/'capture-status.txt').write_text('capturing pid=123 stopped disable_status=0 active=0')
            (build/'session-status.txt').write_text(' '.join(k+'='+v for k,v in terminal().items()))
            args=types.SimpleNamespace(pid=123,build_dir=build,dll_name='Continuous.dll',xr_build_dir=root/'xr',
                                       stereo_units_per_metre=100,startup_timeout=1,cleanup_timeout=.01)
            if diagnostics:args.outage_diagnostics=True;args.shutdown_diagnostics=True
            if origin_trial:args.origin_trial=True;args.anchor_observer=True
            if image_ticket_diagnostics:args.image_ticket_diagnostics=True
            if ui_hud_plane:args.ui_hud_plane=True;args.ui_observer=True;args.anchor_observer=True
            if hud_layout:args.hud_element_scale,args.hud_center_scale=hud_layout
            if startup_menu:args.startup_menu_window=789
            if path_center:
                args.path_choice_center=True;args.menu_recovery=True;args.ui_observer=True;args.anchor_observer=True
            if world_recovery:
                args.world_recovery=True;args.anchor_observer=True;args.ui_observer=True
                args.cinematic_camera=True;args.native_expected_sha256='a'*64
                args.crosshair_assignment_observer=crosshair
            control=Control();child=mock.Mock(pid=456,returncode=0)
            child.poll.return_value=0 if early_exit else None
            child.wait.side_effect=[subprocess.TimeoutExpired('xr',1),0] if xr_timeout else [0]
            evidence=dict(continuous_session_evidence=True,orderly_stop=True,shutdown_completed=True,
                          structural_failure=False,submitted_frames=10,scene_copies=8)
            reports=[]
            def report(path,owner,game):
                self.assertTrue(path.exists());self.assertEqual(owner,identity(os.getpid()));self.assertEqual(game,identity(123))
                reports.append(str(path))
            def spawn(command,stdout,stderr,env,creationflags):
                self.assertEqual(env['XR_SIMULATOR_NO_ACTIVATE'],'1')
                self.assertEqual(creationflags,getattr(subprocess,'CREATE_NO_WINDOW',0))
                self.assertEqual(len(reports),1)
                pathlib.Path(command[2]).write_text('stereo_published,session_state,should_render,head_flags,view_flags,stereo_source\n'+'1,5,1,15,15,0\n'*60)
                stdout.write(json.dumps(evidence)+'\n')
                if startup_menu and panel_submitted:
                    stdout.write(json.dumps(dict(startup_menu_first_frame=True,end_frame_succeeded=True,panel_counts_as_stereo=False))+'\n')
                    stdout.write(json.dumps(dict(startup_menu_handoff=True,end_frame_succeeded=True,ticketed_projection_rendered=True,one_way=not path_center))+'\n')
                    stdout.write(json.dumps(dict(startup_menu_evidence=True,stereo_handoff=True,startup_quad_layers=10))+'\n')
                if path_center:
                    stdout.write(json.dumps(dict(menu_recovery_evidence=True,panel_counts_as_stereo=False))+'\n')
                stdout.write(json.dumps(dict(continuous_owner_evidence=True,owner_pid=os.getpid(),owner_creation_time=identity(os.getpid())['creation_time'],owner_lost=False))+'\n')
                stdout.flush();return child
            def native(command,**kwargs):
                self.assertEqual(kwargs['creationflags'],getattr(subprocess,'CREATE_NO_WINDOW',0))
                if command[1].endswith('verify_stopped.py'):
                    pathlib.Path(command[-1]).write_text(json.dumps({'all_match':True}))
                elif first_stereo:
                    path=next((root/'analysis/runtime').glob('*.stdout.txt'))
                    with path.open('a') as stream:
                        stream.write(json.dumps(dict(reference_epoch_first_projection=True,space_epoch=1,end_frame_succeeded=True,
                            monotonic_ns=runner.time.perf_counter_ns()))+'\n')
                return types.SimpleNamespace(returncode=0)
            stages=[dict(terminal(),phase='running'),dict(terminal(),phase='stop_requested'),terminal('stopped' if clean else 'incomplete')]
            if path_center:
                stages[0]['path_position_service_compiled']='1'
                stages.insert(1,dict(terminal(),phase='running',path_position_service_compiled='1'))
            checks=0
            def cancelled():
                nonlocal checks
                checks+=1
                return checks==cancel_at
            with mock.patch.object(runner,'ROOT',root),mock.patch.object(runner.subprocess,'Popen',side_effect=spawn) as popen, \
                 mock.patch.object(runner.subprocess,'run',side_effect=native) as native_run, \
                 mock.patch.object(runner,'read_session_status',side_effect=stages),contextlib.redirect_stdout(io.StringIO()):
                code=runner.run_session(args,control,on_report=report,cancel_requested=cancelled if cancel_at else None,
                                        before_native_activation=startup_callback)
            state=json.loads(next((root/'analysis/runtime').glob('*.json')).read_text())
            return code,state,popen.call_args,native_run,control,child

    def test_one_owner_orders_native_cleanup_before_xr_and_verifies_entries(self):
        code,state,spawn,native,control,child=self.run_mock()
        self.assertEqual(code,0);self.assertEqual(state['phase'],'stopped');self.assertTrue(state['entry_restore_verified'])
        self.assertFalse(state['outage_diagnostics_enabled']);self.assertFalse(state['shutdown_diagnostics_enabled'])
        self.assertEqual(control.signals,['Local\\GearsVR.Capture.Stop.123','Local\\GearsVR.XR.Stop.123'])
        self.assertTrue(control.closed)
        self.assertEqual(spawn.args[0][1],'0');self.assertIn('--continuous-session',spawn.args[0])
        self.assertNotIn('--simulator-operator',spawn.args[0])
        self.assertEqual(set(spawn.kwargs['env'])-set(os.environ),{'XR_SIMULATOR_NO_ACTIVATE'}-set(os.environ))
        self.assertIn('--continuous-session',native.call_args_list[0].args[0])
        self.assertNotIn('--submit-pair-seconds',native.call_args_list[0].args[0])
        for command,pid_flag,time_flag in [(spawn.args[0],'--owner-pid','--owner-creation-time'),
                (native.call_args_list[0].args[0],'--continuous-owner-pid','--continuous-owner-creation-time')]:
            self.assertEqual(command[command.index(pid_flag)+1],str(os.getpid()))
            self.assertEqual(command[command.index(time_flag)+1],str(identity(os.getpid())['creation_time']))

    def test_world_monitor_pawn_change_policy_is_enabled_only_for_crosshair_observer(self):
        import world_rebind_runtime
        import world_rebind_monitor
        for enabled in (False,True):
            with self.subTest(crosshair=enabled),\
                 mock.patch.object(world_rebind_runtime,'RuntimePreparation') as adapter,\
                 mock.patch.object(world_rebind_monitor,'WorldRebindMonitor') as monitor:
                monitor.return_value.poll.return_value=[]
                code,*_=self.run_mock(world_recovery=True,crosshair=enabled)
                self.assertEqual(code,0)
                self.assertIs(monitor.call_args.kwargs['include_pawn_changes'],enabled)
                self.assertIs(monitor.call_args.kwargs['baseline'],adapter.return_value.baseline)
                monitor.return_value.close.assert_called_once()

    def test_path_choice_monitor_is_opt_in_and_closes_before_session_cleanup(self):
        import path_choice_position_monitor,menu_display_monitor
        for enabled in (False,True):
            signals=[]
            original_signal=Control.signal
            def signal(control,name):
                signals.append(name)
                return original_signal(control,name)
            with self.subTest(enabled=enabled),mock.patch.object(path_choice_position_monitor,'PathChoicePositionMonitor') as center, \
                    mock.patch.object(Control,'signal',signal), \
                    mock.patch.object(menu_display_monitor,'MenuDisplayMonitor'):
                center.return_value.poll.return_value=[dict(event='path_position_published',nonce=99)]
                center.return_value.cleanup_error=None
                center.return_value.worker_joined=True
                center.return_value.close.side_effect=lambda:self.assertEqual(signals,[])
                def finish(**proofs):
                    self.assertEqual(signals,['Local\\GearsVR.Capture.Stop.123','Local\\GearsVR.XR.Stop.123'])
                    self.assertEqual(proofs,dict(native_cleanup_verified=True,entry_restore_verified=True,game_absent=False))
                    return True
                center.return_value.finish_cleanup.side_effect=finish
                code,state,*_=self.run_mock(path_center=enabled,startup_menu=enabled)
                self.assertEqual(code,0);self.assertIs(state['path_choice_center_enabled'],enabled)
                if enabled:
                    center.assert_called_once();center.return_value.poll.assert_called_once()
                    center.return_value.close.assert_called_once()
                    center.return_value.finish_cleanup.assert_called_once()
                    self.assertEqual(center.call_args.args[0].name,'path-choice-position-trial.ini')
                    self.assertEqual(center.call_args.args[1].name,'path-choice-close-observer.ini')
                    self.assertEqual(center.call_args.args[2]['native_started_qpc'],'1234')
                    self.assertTrue(state['path_choice_center_sidecars_cleaned'])
                else:center.assert_not_called()

    def test_path_choice_dependency_validation_precedes_process_launch(self):
        args=types.SimpleNamespace(path_choice_center=True,menu_recovery=True,ui_observer=True,anchor_observer=True)
        runner.validate_ui_hud_plane(args)
        for key in ('menu_recovery','ui_observer','anchor_observer'):
            with self.subTest(key=key),self.assertRaisesRegex(ValueError,'PathChoice'):
                runner.validate_ui_hud_plane(types.SimpleNamespace(**dict(vars(args),**{key:False})))

    def test_startup_stage_timings_distinguish_running_from_first_stereo(self):
        _,state,*_=self.run_mock()
        timing=state['startup_timings']
        self.assertEqual(set(timing),{'xr_preflight','native_prepare','native_running'})
        self.assertNotIn('first_stereo_submission',timing)
        elapsed=[timing[key]['since_runner_start_ms'] for key in timing]
        self.assertEqual(elapsed,sorted(elapsed))
        for stage in timing.values():
            self.assertGreaterEqual(stage['elapsed_ms'],0)
            self.assertTrue(stage['timing_is_runner_observation'])
        callbacks=[]
        _,state,*_=self.run_mock(startup_menu=True,startup_callback=lambda live:(live(),callbacks.append(True)),first_stereo=True)
        self.assertEqual(callbacks,[True])
        self.assertEqual(set(state['startup_timings']),{'xr_preflight','camera_ready','native_prepare','native_running','first_stereo_submission'})
        self.assertIn('successful xrEndFrame',state['startup_timings']['first_stereo_submission']['evidence'])
        self.assertFalse(state['startup_timings']['first_stereo_submission']['timing_is_runner_observation'])
        self.assertGreaterEqual(state['startup_timings']['first_stereo_submission']['runner_observation_delay_ms'],0)

    def test_first_stereo_timing_uses_only_successful_existing_marker(self):
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'stdout.txt'
            self.assertFalse(runner.first_stereo_submitted(path))
            good=dict(reference_epoch_first_projection=True,space_epoch=1,end_frame_succeeded=True,monotonic_ns=123)
            for key,value in (('reference_epoch_first_projection',False),('space_epoch',2),('end_frame_succeeded',False),('monotonic_ns',0)):
                path.write_text('partial output\n'+json.dumps(dict(good,**{key:value}))+'\n')
                self.assertFalse(runner.first_stereo_submitted(path),key)
            path.write_text(json.dumps({'continuous_running':True})+'\n'+json.dumps(good)+'\n')
            self.assertEqual(runner.first_stereo_submitted(path),good)
            with mock.patch.object(pathlib.Path,'read_text',side_effect=PermissionError('busy')):
                self.assertFalse(runner.first_stereo_submitted(path))

    def test_diagnostics_enabled_report_and_commands(self):
        code,state,spawn,native,control,child=self.run_mock(diagnostics=True)
        self.assertEqual(code,0)
        self.assertTrue(state['outage_diagnostics_enabled']);self.assertTrue(state['shutdown_diagnostics_enabled'])
        self.assertIn('--outage-diagnostics',spawn.args[0]);self.assertIn('--shutdown-diagnostics',spawn.args[0])
        self.assertIn('--outage-diagnostics',native.call_args_list[0].args[0])
        self.assertNotIn('--shutdown-diagnostics',native.call_args_list[0].args[0])

    def test_level_callback_runs_under_live_xr_before_native_activation(self):
        callbacks=[]
        def ready(check_live):
            check_live();callbacks.append('ready')
        code,state,spawn,native,control,child=self.run_mock(startup_callback=ready)
        self.assertEqual(code,0);self.assertEqual(callbacks,['ready']);self.assertTrue(state['native_started'])

    def test_startup_menu_requires_actual_panel_submission_before_native(self):
        for submitted in (True,False):
            with self.subTest(submitted=submitted):
                callback=mock.Mock()
                code,state,spawn,native,control,child=self.run_mock(startup_menu=True,panel_submitted=submitted,startup_callback=callback)
                self.assertEqual(code,0 if submitted else 1)
                self.assertEqual(state.get('startup_panel_submitted',False),submitted)
                self.assertIn('--startup-menu-window',spawn.args[0])
                if submitted:callback.assert_called_once()
                else:callback.assert_not_called();native.assert_not_called()

    def test_menu_cancellation_or_failure_never_activates_native(self):
        for error,expected in ((runner.StartupCancelled(),0),(RuntimeError('menu unavailable'),1)):
            def failure(check_live):
                check_live();raise error
            with self.subTest(error=type(error).__name__):
                code,state,spawn,native,control,child=self.run_mock(startup_callback=failure)
                self.assertEqual(code,expected);native.assert_not_called();self.assertFalse(state['native_started'])
                self.assertFalse(state['passed']);child.terminate.assert_not_called()
                self.assertEqual(control.signals,['Local\\GearsVR.XR.Stop.123'])

    def test_native_incomplete_and_xr_timeout_cannot_pass(self):
        for options in ({'clean':False},{'xr_timeout':True},{'early_exit':True}):
            with self.subTest(options=options):
                code,state,spawn,native,control,child=self.run_mock(**options)
                self.assertEqual(code,1);self.assertEqual(state['phase'],'incomplete')
                if options.get('xr_timeout'):child.terminate.assert_called_once()
                if options.get('early_exit'):native.assert_not_called()

    def test_startup_cancellation_does_not_activate_native_or_claim_stereo_pass(self):
        for cancel_at in (1,2,3):
            with self.subTest(cancel_at=cancel_at):
                code,state,spawn,native,control,child=self.run_mock(cancel_at=cancel_at)
                self.assertEqual(code,0);self.assertEqual(state['phase'],'cancelled')
                self.assertFalse(state['passed']);self.assertFalse(state['native_started'])
                native.assert_not_called();child.terminate.assert_not_called()
                self.assertEqual(control.signals,[] if cancel_at==1 else ['Local\\GearsVR.XR.Stop.123'])
                self.assertTrue(control.closed)

    def test_stop_does_not_signal_reused_owner_pid(self):
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'session.json'
            state=dict(schema=1,kind='continuous_strict_stereo',session_id='unique',phase='running',
                       owner_identity=dict(identity(100),creation_time=999),game_identity=identity(123),xr_identity=identity(456))
            path.write_text(json.dumps(state));control=Control()
            with self.assertRaises(RuntimeError):runner.request_stop(path,.1,control)
            self.assertFalse(control.signals)

    def test_stop_only_signals_native_and_waits_for_joint_owner_result(self):
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'session.json';control=Control()
            state=dict(schema=1,kind='continuous_strict_stereo',session_id='unique',phase='running',
                       owner_identity=identity(100),game_identity=identity(123),xr_identity=identity(456))
            path.write_text(json.dumps(state))
            def signal(name):
                control.signals.append(name);path.write_text(json.dumps(dict(state,phase='stopped',passed=True)));return True
            control.signal=signal
            self.assertEqual(runner.request_stop(path,.1,control),0)
            self.assertEqual(control.signals,['Local\\GearsVR.Capture.Stop.123'])

    def test_exit_zero_requires_scene_and_orderly_joint_cleanup(self):
        state=dict(native_cleanup_verified=True,entry_restore_verified=True,xr_exit_code=0,owner_identity=identity(123),native_status=terminal(),
                   continuous_owner_evidence=dict(owner_pid=123,owner_creation_time=1230,owner_lost=False),
                   continuous_session_evidence=dict(orderly_stop=True,shutdown_completed=True,structural_failure=False,
                                                    submitted_frames=1,scene_copies=1))
        self.assertTrue(runner.continuous_passed(state))
        startup=dict(state,startup_menu_enabled=True,startup_panel_submitted=True,
            startup_menu_evidence=dict(stereo_handoff=True,startup_quad_layers=1),
            startup_menu_handoff=dict(end_frame_succeeded=True,ticketed_projection_rendered=True,one_way=True))
        self.assertTrue(runner.continuous_passed(startup))
        for missing in ('startup_panel_submitted','startup_menu_evidence','startup_menu_handoff'):
            changed=dict(startup);changed.pop(missing)
            self.assertFalse(runner.continuous_passed(changed),missing)
        for field,value in [('orderly_stop',False),('shutdown_completed',False),('structural_failure',True),('scene_copies',0)]:
            changed=dict(state,continuous_session_evidence=dict(state['continuous_session_evidence'],**{field:value}))
            self.assertFalse(runner.continuous_passed(changed),field)
        for watched in ({},{'owner_pid':124,'owner_creation_time':1230,'owner_lost':False},
                        {'owner_pid':123,'owner_creation_time':1231,'owner_lost':False},
                        {'owner_pid':123,'owner_creation_time':1230,'owner_lost':True}):
            self.assertFalse(runner.continuous_passed(dict(state,continuous_owner_evidence=watched)))
        for flag in ('owner_exit_observed','owner_error_observed'):
            self.assertFalse(runner.continuous_passed(dict(state,native_status=dict(terminal(),**{flag:'1'}))))

    def test_recoverable_menu_handoff_requires_configured_mode_and_runtime_evidence(self):
        state=dict(native_cleanup_verified=True,entry_restore_verified=True,xr_exit_code=0,
            owner_identity=identity(123),native_status=terminal(),
            continuous_owner_evidence=dict(owner_pid=123,owner_creation_time=1230,owner_lost=False),
            continuous_session_evidence=dict(orderly_stop=True,shutdown_completed=True,structural_failure=False,
                                             submitted_frames=10,scene_copies=3),
            startup_menu_enabled=True,menu_recovery_enabled=True,startup_panel_submitted=True,
            startup_menu_evidence=dict(stereo_handoff=True,startup_quad_layers=4),
            startup_menu_handoff=dict(end_frame_succeeded=True,ticketed_projection_rendered=True,one_way=False),
            menu_recovery_evidence=dict(menu_recovery_evidence=True,panel_counts_as_stereo=False))
        self.assertTrue(runner.continuous_passed(state))
        for field,value in [('menu_recovery_enabled',False),('menu_recovery_enabled',1),
                            ('startup_menu_enabled',False),('menu_recovery_evidence',{}),
                            ('xr_exit_code',1),('native_cleanup_verified',False)]:
            self.assertFalse(runner.continuous_passed(dict(state,**{field:value})),field)
        for field in ('end_frame_succeeded','ticketed_projection_rendered','one_way'):
            changed=dict(state['startup_menu_handoff']);changed[field]=not changed[field]
            self.assertFalse(runner.continuous_passed(dict(state,startup_menu_handoff=changed)),field)
        bad=dict(state['menu_recovery_evidence'],panel_counts_as_stereo=True)
        self.assertFalse(runner.continuous_passed(dict(state,menu_recovery_evidence=bad)))
        failed=dict(state['continuous_session_evidence'],structural_failure=True)
        self.assertFalse(runner.continuous_passed(dict(state,continuous_session_evidence=failed)))

    def test_native_owner_matches_requires_pid_and_creation(self):
        owner=identity(os.getpid())
        self.assertTrue(runner.native_owner_matches(terminal(),owner))
        self.assertFalse(runner.native_owner_matches(terminal(),dict(owner,creation_time=owner['creation_time']+1)))
        self.assertFalse(runner.native_owner_matches(terminal(),dict(owner,pid=owner['pid']+1)))

    def test_start_capture_parser_keeps_continuous_opt_in_and_old_trials(self):
        import start_capture
        with tempfile.TemporaryDirectory() as folder:
            base=['start_capture.py','--pid','123','--build-dir',folder,'--dll-name','Missing.dll',
                  '--use-reserved','--submit-pair','--submit-background','--submit-output-bounds',
                  '--viewport-diagnostics','--view-union-mask-fix','--stereo-xr',
                  '--frame-link-diagnostics','--require-target-prime','--stereo-gpu-publish']
            # Passing validation reaches only the missing-build check, never a
            # subprocess, DLL load or game access.
            owner_flags=['--continuous-owner-pid','123','--continuous-owner-creation-time','1230']
            valid=[['--continuous-session']+owner_flags,['--submit-pair-seconds','30','--seconds','45'],
                   ['--submit-pair-seconds','120','--trial-seconds','120','--seconds','135']]
            for flags in valid:
                with self.subTest(flags=flags),mock.patch.object(start_capture.sys,'argv',base+flags), \
                     mock.patch.object(start_capture.subprocess,'run') as spawn:
                    with self.assertRaisesRegex(RuntimeError,'Build capture DLL'):start_capture.main()
                    spawn.assert_not_called()
            invalid=[['--continuous-session','--submit-pair-seconds','30']+owner_flags,
                     ['--continuous-session','--simulator-operator']+owner_flags,['--continuous-session','--projection-control']+owner_flags,
                     ['--continuous-session'],['--continuous-session','--continuous-owner-pid','123'],
                     ['--continuous-session','--continuous-owner-pid','0','--continuous-owner-creation-time','1230'],
                     ['--continuous-session','--continuous-owner-pid','123','--continuous-owner-creation-time',str(2**64)],
                     owner_flags]
            for flags in invalid:
                with self.subTest(flags=flags),mock.patch.object(start_capture.sys,'argv',base+flags), \
                     mock.patch.object(start_capture.subprocess,'run') as spawn,contextlib.redirect_stderr(io.StringIO()):
                    with self.assertRaises(SystemExit) as error:start_capture.main()
                    self.assertEqual(error.exception.code,2);spawn.assert_not_called()

    def test_recenter_only_signals_an_identical_running_owner_game_xr(self):
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'session.json';control=Control()
            state=dict(schema=1,kind='continuous_strict_stereo',phase='running',owner_identity=identity(100),
                       game_identity=identity(123),xr_identity=identity(456))
            path.write_text(json.dumps(state))
            with contextlib.redirect_stdout(io.StringIO()):self.assertEqual(runner.request_recenter(path,control),0)
            self.assertEqual(control.signals,['Local\\GearsVR.Stereo.Recenter.123'])
            for changed in (dict(state,phase='starting'),dict(state,xr_identity=dict(identity(456),creation_time=999))):
                control.signals=[];path.write_text(json.dumps(changed))
                with self.assertRaises(RuntimeError):runner.request_recenter(path,control)
                self.assertFalse(control.signals)


if __name__=='__main__':unittest.main()
