"""One private Source1 simulator admission; production modules stay unchanged."""
from pathlib import Path
import os,sys,types,runpy
ROOT=Path('J:\\codexWorktree\\gearsVr')
OUT=Path('J:\\codexWorktree\\gearsVr\\analysis\\solo-m1-20261028')
for folder in ('native-capture','vr-launcher','debug-launch','openxr-probe'):
    sys.path.insert(0,str(ROOT/'tools'/folder))
assert all(os.environ.get(k)=='1' for k in ('GEARSVR_TEST_WINDOWS_BEHIND','GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY','GEARSVR_TEST_MUTE_GAME'))
assert Path(os.environ['GEARSVR_INSTANCE_ROOT']).resolve().parent==ROOT/'analysis/instances'
def load(name,path,transform=lambda s:s):
    m=types.ModuleType(name);m.__file__=str(path);sys.modules[name]=m
    exec(compile(transform(path.read_text(encoding='utf-8-sig')),str(path),'exec'),m.__dict__)
    return m
if sys.argv[1]=='native':
    if os.environ.get('GEARSVR_SOLO_CAMERA_SETTLE')=='1':
        # One explicit startup-race recheck: allow the newly loaded scene to
        # settle BEFORE resolving fresh identities/certificates. All original
        # activation, identity and freshness checks still run afterward.
        __import__('time').sleep(2)
    import sixdof_manual_profile as profile
    profile.SECTIONS = profile.SECTIONS.replace("diagnostic_first_failure=0","diagnostic_first_failure=1")
    profile.SECTIONS = profile.SECTIONS.replace("[FreshPoseProducerObserver]\nenabled=1","[FreshPoseProducerObserver]\nenabled=1\nrecord_base_entry=1")
    profile.SECTIONS += '\n[CoverBodyPrototype]\nenabled=1\npose_profile=top\ntarget_source=live\n[CoverCameraPreview]\nenabled=1\n[CoverInputPrototype]\nenabled=1\nheight_half_metres=0.3\nheight_top_metres=0.7\n'
    if os.environ.get('GEARSVR_SOLO_STOCK_CHARACTER')=='1':
        # Original-animation research only, never a delivered VR profile or
        # evidence of product first-person/hand acceptance. Leave original
        # skeletal animation, arm controls and weapon attachment untouched.
        assert os.environ.get('GEARSVR_SOLO_HAND_TRACE')!='1'
        # Render/input transport only. Do not retain dependent pose evaluators
        # or atom receipts when their body/hand writer is intentionally absent.
        profile.SECTIONS='\n[WeaponHandParticipant]\nenabled=1\n[RigTracking]\nenabled=1\n'
        profile.CAPTURE=profile.CAPTURE.replace('first_person_initial_requested=1','first_person_initial_requested=0',1)
        profile.CAPTURE=profile.CAPTURE.replace('first_person_body_basis=1','first_person_body_basis=0',1)
    # Reuse the existing bounded first32 commit/alpha receipts for this one
    # owned animation-overwrite investigation; production profile unchanged.
    if os.environ.get('GEARSVR_SOLO_HANDOFF_WITNESS')=='1':
        # Only the existing bounded failure summary. Do not enable the wider
        # base-entry/render-pose observers for this isolated handoff check.
        assert os.environ.get('GEARSVR_SOLO_HAND_TRACE')!='1'
        assert os.environ.get('GEARSVR_SOLO_STOCK_CHARACTER')!='1'
        assert 'diagnostic_first_failure=0' in profile.SECTIONS
        profile.SECTIONS=profile.SECTIONS.replace('diagnostic_first_failure=0','diagnostic_first_failure=1',1)
    if os.environ.get('GEARSVR_SOLO_WALK_HANDOFF_WITNESS')=='1':
        # First32 C0 returns in40032 retained the planned bones while the
        # captured image still popped. Reuse the existing bounded render-palette
        # observer as well to inspect the actual render consumer in the next
        # coherent repair check. No new collector.
        assert os.environ.get('GEARSVR_SOLO_HAND_TRACE')!='1'
        assert os.environ.get('GEARSVR_SOLO_HANDOFF_WITNESS')!='1'
        assert 'diagnostic_first_failure=0' in profile.SECTIONS
        profile.SECTIONS=profile.SECTIONS.replace('diagnostic_first_failure=0',
            'diagnostic_first_failure=1\ndiagnostic_hand_trace_head_pitch=1\ndiagnostic_render_pose_trace=1',1)
        if os.environ.get('GEARSVR_SOLO_WALK_PRODUCER_BRANCH')=='1':
            #36216 did not enter the cached-copy witness. Reuse the existing
            #16-root aggregate to identify the actual missed producer branch.
            profile.SECTIONS=profile.SECTIONS.replace('[FreshPoseProducerObserver]\nenabled=1',
                '[FreshPoseProducerObserver]\nenabled=1\nrecord_base_entry=1',1)
    if os.environ.get('GEARSVR_SOLO_HAND_TRACE')=='1':
        profile.SECTIONS=profile.SECTIONS.replace('[FreshPoseProducerObserver]\nenabled=1','[FreshPoseProducerObserver]\nenabled=1\nrecord_base_entry=1',1)
        profile.SECTIONS=profile.SECTIONS.replace('diagnostic_first_failure=0',
            'diagnostic_first_failure=1\ndiagnostic_render_pose_trace=1',1)
        if os.environ.get('GEARSVR_SOLO_SHOULDER_TRACE')=='1':
            profile.SECTIONS=profile.SECTIONS.replace('diagnostic_first_failure=1','diagnostic_first_failure=1\ndiagnostic_hand_trace_right_shoulder=1',1)
        if os.environ.get('GEARSVR_SOLO_HEAD_PITCH_TRACE')=='1':
            assert os.environ.get('GEARSVR_SOLO_SHOULDER_TRACE')!='1'
            profile.SECTIONS=profile.SECTIONS.replace('diagnostic_first_failure=1','diagnostic_first_failure=1\ndiagnostic_hand_trace_head_pitch=1',1)
        if os.environ.get('GEARSVR_SOLO_MOUNT_TRACE')=='1':
            profile.SECTIONS=profile.SECTIONS.replace('diagnostic_held_main_mount=0','diagnostic_held_main_mount=1',1).replace('diagnostic_ready_wrist=0','diagnostic_ready_wrist=1',1)
            if os.environ.get('GEARSVR_SOLO_SAW_REENTRY_TRACE')=='1':
                profile.SECTIONS=profile.SECTIONS.replace('diagnostic_ready_wrist=1','diagnostic_ready_wrist=1\ndiagnostic_ready_wrist_saw_reentry=1',1)
                if os.environ.get('GEARSVR_SOLO_SAW_ENTRY_TARGET')=='1':
                    profile.SECTIONS=profile.SECTIONS.replace('diagnostic_ready_wrist_saw_reentry=1','diagnostic_ready_wrist_saw_reentry=1\ndiagnostic_ready_wrist_saw_entry=1',1)
    if os.environ.get('GEARSVR_SOLO_SEQUENCE_WITNESS')=='1' and 'experimental_sequence_overwrite=1' not in profile.SECTIONS:
        profile.SECTIONS=profile.SECTIONS.replace('[FreshPoseProducerObserver]\nenabled=1','[FreshPoseProducerObserver]\nenabled=1\nexperimental_sequence_overwrite=1',1)
    if os.environ.get('GEARSVR_SOLO_PER_BONE_WITNESS')=='1' and 'experimental_per_bone_overwrite=1' not in profile.SECTIONS:
        profile.SECTIONS=profile.SECTIONS.replace('[FreshPoseProducerObserver]\nenabled=1','[FreshPoseProducerObserver]\nenabled=1\nexperimental_per_bone_overwrite=1',1)
    original=profile.validate
    def validate(args):
        assert args.simulator_operator and args.sixdof_manual
        # Same production features/guards, explicitly admitted as Source1.
        check=types.SimpleNamespace(**vars(args));check.simulator_operator=False
        return original(check)
    profile.validate=validate
    sys.argv=[str(ROOT/'tools/native-capture/start_capture.py'),*sys.argv[2:]]
    def source1_capture(s):
        old='or args.first_view_mask_control or args.projection_control or args.simulator_operator)'
        if old not in s:raise RuntimeError('Expected isolated continuous-source guard changed')
        return s.replace(old,'or args.first_view_mask_control or args.projection_control)',1)
    load('__main__',ROOT/'tools/native-capture/start_capture.py',source1_capture)
else:
    def sim_runner(s):
        s=s.replace("status.get('simulator_operator')=='0'","status.get('simulator_operator')=='1'")
        s=s.replace("status.get('simulator_operator')!='0'","status.get('simulator_operator')!='1'")
        s=s.replace("r.get('stereo_source')=='0'","r.get('stereo_source')=='1'")
        s=s.replace("'source':0,'operator_motion_requested':False,'simulation_not_physical_tracking':False", "'source':1,'operator_motion_requested':True,'simulation_not_physical_tracking':True")
        return s
    runner=load('run_stereo_continuous',ROOT/'tools/native-capture/run_stereo_continuous.py',sim_runner)
    native_command=runner.native_command;xr_command=runner.xr_command
    def native(args,owner):
        c=native_command(args,owner);c[1:2]=[str(__file__),'native']
        if os.environ.get('GEARSVR_SOLO_EQUIPMENT_FIXTURE')=='1':
            # Explicit synthetic slot dimensions, never a player measurement
            # or saved physical profile; production source/config unchanged.
            c+=['--armspan-metres','1.7','--hands-below-head-metres','-0.15']
        return c+['--simulator-operator']
    def xr(args,csv,owner):
        result=xr_command(args,csv,owner)+['--simulator-operator']
        if os.environ.get('GEARSVR_SOLO_MENU_SOURCE')=='1':result+=['--gpu-snapshot',str(csv.with_suffix('.gpu'))]
        return result
    def xr_environment(parent):
        import operator_client
        operator_client.LAYER=Path('J:/gearsVr/tools/third-party/metavr/state/tools/meta-xr-operator/meta-xr-operator-standalone-public/windows')
        private=Path(os.environ['GEARSVR_INSTANCE_ROOT'])/'xr-input/LocalAppData'
        (private/'OpenXR-Simulator').mkdir(parents=True,exist_ok=True)
        # This child uses the private simulator only. Keep its logical XR
        # input focus while the preview stays behind the owner's windows;
        # WM_ACTIVATE must not revoke simulated tracking mid-gesture. Do not
        # bypass application/physical-runtime focus gates or activate a window.
        result=operator_client.operator_child_env(parent);result.update(LOCALAPPDATA=str(private),XR_SIMULATOR_NO_ACTIVATE='1',XR_SIMULATOR_FORCE_FOCUS='1')
        return result
    runner.native_command=native;runner.xr_command=xr;runner.xr_environment=xr_environment
    def allow_private_cold_window(s):
        old="if owned_prime:\n            deadline=clock()+30\n            last_window_diagnostic={}"
        if old not in s:raise RuntimeError('Private window budget target changed')
        return s.replace(old,"if owned_prime:\n            deadline=clock()+90\n            last_window_diagnostic={}",1)
    launcher=load('solo_private_launcher',ROOT/'tools/vr-launcher/launcher.py',allow_private_cold_window)
    original_render_size_info=launcher.render_size_info
    def private_render_size_info(manifest,scale,root=ROOT):
        # Preflight and the eventual XR child must see the SAME private
        # recommended size. Never read/change the owner's simulator settings.
        return original_render_size_info(manifest,scale,root,environment=xr_environment(os.environ.copy()))
    launcher.render_size_info=private_render_size_info
    def start_private_gamepad(self,manifest,identity,owner,check_live):
        from playground_loader import start_gamepad
        return start_gamepad(manifest,identity,owner,check_live=check_live,simulator_operator=True)
    launcher.Backend.start_vr_gamepad=start_private_gamepad
    def navigate_playground(self,pid,identity,cancel,deadline,log,check_live=None):
        if check_live:check_live()
        if self.control.identity(pid)!=identity:raise RuntimeError('Owned game identity changed')
        bridge=self.playground_bridge
        if bridge is None:raise RuntimeError('Owned Playground bridge absent')
        bridge.verify();code=bridge.export('PlaygroundTestEnter')
        if code!=0:raise RuntimeError('Owned Playground entry rejected: '+str(code))
        return {'loading_requested':True,'target':'happyotaku_playground','controlled_test_entry':True}
    launcher.Backend.navigate=navigate_playground
    sys.argv=[str(ROOT/'tools/vr-launcher/launcher.py'),*sys.argv[2:]]
    raise SystemExit(launcher.main())

