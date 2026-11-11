"""One private HB empty-reload attempt through the repaired background entry.

No shared staging, global input, desktop input, focus changes or forced kills.
"""
from pathlib import Path
import ctypes
from ctypes import wintypes as w
import hashlib
import json
import os
import subprocess
import shutil
import sys
import time

ROOT=Path(__file__).resolve().parents[2]
AREA=Path(__file__).resolve().parent
TOOL=ROOT/'tools/multi-instance/isolated_sim.py'
PAUSE=ROOT/'tools/multi-instance/automatic-runtime-pause.json'


def write(path,value):path.write_text(json.dumps(value,indent=2),encoding='utf-8')


def reload_test_mode(arguments):
    # A separate focused action path; never recovery/retry in a failed game.
    focused='--focused-loaded-replacement' in arguments
    return dict(empty_only='0' if focused else '1',
                scope='loaded replacement insertion diagnostic; empty reload still pending' if focused else 'full finite empty reload and return-fire')


def resource_preflight():
    # Advisory before the large private prepare copy. The shared start path
    # still repeats admission under its resource transaction; this reserves
    # neither an instance slot nor memory during gameplay.
    sys.path.insert(0,str(TOOL.parent))
    from isolated_sim import require_memory, require_disk
    return dict(disk=require_disk(ROOT), memory=require_memory())


def wait_for_resource_admission(*,check=resource_preflight,paused=None,timeout=600,
                                clock=time.monotonic,sleep=time.sleep):
    paused=paused or PAUSE.exists
    started=clock();deadline=started+timeout;checks=0
    while True:
        if paused():raise RuntimeError('Owner paused automatic runtime; prepared instance not started')
        if clock()>=deadline:raise TimeoutError('Prepared instance resource wait expired; no start attempted')
        try:
            resources=check()
        except RuntimeError as error:
            message=str(error)
            if not (message.startswith('Two automatic instances') or
                    message.startswith('Insufficient currently available memory') or
                    '"check": "disk_admission"' in message):raise
            checks+=1
            sleep(min(1,max(0,deadline-clock())))
            continue
        if paused():raise RuntimeError('Owner paused automatic runtime; prepared instance not started')
        if clock()>=deadline:raise TimeoutError('Prepared instance resource wait expired; no start attempted')
        return dict(resources=resources,waited_seconds=clock()-started,rejected_checks=checks,
                    runtime_slot_reserved=False)


def main():
    if PAUSE.exists():raise RuntimeError('Automatic runtime is paused by the owner; no preparation/start attempt')
    if shutil.disk_usage(ROOT).free<4*1024**3:
        raise RuntimeError('Insufficient free workspace disk for owned runtime evidence; no prepare/start attempted')
    prepare_then_wait='--prepare-then-wait' in sys.argv and '--resources' not in sys.argv
    if prepare_then_wait and shutil.disk_usage(ROOT).free<5*1024**3:
        raise RuntimeError('Need existing4GiB plus1GiB private-prepare margin; no prepare/start attempted')
    resources=None if prepare_then_wait else resource_preflight()
    if '--resources' in sys.argv:
        print(json.dumps(resources,indent=2))
        return 0
    name='hb-cloak-'+time.strftime('%m%d-%H%M%S')
    target=AREA/name;target.mkdir()
    manifest=json.loads((AREA/'independent-r1/candidate-manifest.json').read_text())
    render_age='--render-age-continuity' in sys.argv
    native_mutex='--native-mutex' in sys.argv
    assert not native_mutex or render_age,'Mutex observer retains the exact age-continuity Native parent'
    latch_observation='--latch-observation-wait' in sys.argv
    latch_incumbent='--latch-incumbent-gap' in sys.argv
    extraction_history='--extraction-history' in sys.argv
    tick_cost_tail='--tick-cost-tail' in sys.argv
    history_rejection='--history-rejection' in sys.argv
    withdrawal_clock='--withdrawal-clock' in sys.argv
    history_origin='--history-origin' in sys.argv
    early_refire='--early-released-refire' in sys.argv
    assert not early_refire or history_origin,'Early refire retains the original history origin observer'
    assert not history_origin or withdrawal_clock,'History origin observer retains the first stale consumer observer'
    assert not withdrawal_clock or history_rejection,'Withdrawal clock observer includes the original history observer'
    assert not history_rejection or tick_cost_tail,'History rejection observer includes the original passive GT tail'
    assert not tick_cost_tail or extraction_history,'Passive tail candidate includes the pending extraction policy'
    assert not extraction_history or latch_incumbent,'Extraction candidate includes the tested latch incumbent policy'
    assert not latch_incumbent or latch_observation,'Latch incumbent candidate includes the tested observation wait'
    assert not latch_observation or render_age,'Observation-wait candidate requires the certified Native pair'
    assert not (render_age and '--native-input-loss' in sys.argv),'Select one explicit Native candidate'
    pg_candidate='latch-incumbent-gap-r1' if latch_incumbent else ('latch-observation-wait-r1' if latch_observation else ('render-age-pg-r1' if render_age else 'latch-seek-gap-r1'))
    if extraction_history:pg_candidate='extraction-history-r1'
    if tick_cost_tail:pg_candidate='tick-cost-tail-r1'
    if history_rejection:pg_candidate='history-rejection-r1'
    if withdrawal_clock:pg_candidate='withdrawal-clock-r1'
    if history_origin:pg_candidate='history-origin-r1'
    if early_refire:pg_candidate='early-released-refire-r1'
    proof=json.loads((AREA/pg_candidate/'prepared.json').read_text())
    if early_refire:
        check=json.loads((AREA/pg_candidate/'early-refire-test-result.json').read_text())
        assert check['passed'] and check['original_policy_unchanged'] and check['added_cases']==24 and not check['new_input_or_event']
    if history_origin:
        check=json.loads((AREA/pg_candidate/'history-origin-test-result.json').read_text())
        assert check['passed'] and check['exact_policy_reversal'] and check['extra_ring_reads']==0 and check['extra_clocks']==0
    if withdrawal_clock:
        check=json.loads((AREA/pg_candidate/'withdrawal-clock-test-result.json').read_text())
        assert check['passed'] and check['exact_policy_reversal'] and check['added_clocks']==0 and check['added_source_reads']==0
    if latch_observation:
        check=json.loads((AREA/'latch-observation-wait-r1/observation-wait-test-result.json').read_text())
        assert check['passed'] and check['existing_latch_and_history_tests_passed'] and check['original_capacity']==8
        event='source/tools/playground/weapon_hammerburst_burst_event.inl'
        event_reference=AREA/'history-origin-r1'/event if early_refire else AREA/pg_candidate/event
        if early_refire:
            from prepare_early_released_refire import HELPER
            assert (AREA/pg_candidate/event).read_text().replace(HELPER,'')==event_reference.read_text()
        assert event_reference.read_bytes()==(AREA/'render-age-pg-r1'/event).read_bytes()
        if latch_incumbent:
            check=json.loads((AREA/'latch-incumbent-gap-r1/pose-gap-test-result.json').read_text())
            assert check['exit_code']==0 and check['actual_legacy_helper_reproduced_loss'] and check['real_up_releases']
            kernel='source/tools/vr-common/weapon_hammerburst_latch.h'
            assert (AREA/pg_candidate/kernel).read_bytes()==(AREA/'latch-observation-wait-r1'/kernel).read_bytes()
    if extraction_history:
        check=json.loads((AREA/'extraction-history-r1/extraction-history-test-result.json').read_text())
        assert check['passed'] and check['cases']==32 and check['existing_magazine_and_insertion_history_tests_passed']
        rel='source/tools/playground/weapon_hammerburst_runtime.inl'
        def incumbent_block(text):
            start=text.index('void unavailable_frame(')
            return text[start:text.index('\nbool tick(uptr pc)',start)]
        assert incumbent_block((AREA/pg_candidate/rel).read_text())==incumbent_block((AREA/'latch-incumbent-gap-r1'/rel).read_text())
        if tick_cost_tail:
            assert (AREA/pg_candidate/rel).read_bytes()==(AREA/'extraction-history-r1'/rel).read_bytes()
            check=json.loads((AREA/pg_candidate/'tick-cost-test-result.json').read_text())
            assert check['passed'] and check['exact_removed_insertions_equal_original'] and check['gt_only_dump_before_stopped']
            if history_rejection:
                check=json.loads((AREA/pg_candidate/'history-observer-test-result.json').read_text())
                assert check['passed'] and check['original_pure_build_unchanged'] and check['original_build_called_once']
    assert proof['full_playground_compiled'] and not proof['runtime_exercised']
    dll=Path(proof['dll']);assert hashlib.sha256(dll.read_bytes()).hexdigest()==proof['dll_sha256']
    manifest.update(playground_build=str(dll.parent.relative_to(ROOT)),playground_sha256=proof['dll_sha256'],
                    lancer_interaction_ranges=False,candidate_validation=proof.get('scope','Private HB candidate')+'; whole weapon acceptance pending')
    if render_age:
        checks=json.loads((AREA/'render-age-pg-r1/release-handoff-test-result.json').read_text())
        assert checks['passed'] and checks['total_checks']==457
        native_proof=json.loads((AREA/'native-render-age-r1/prepared.json').read_text())
        if native_mutex:
            mutex_check=json.loads((AREA/'native-mutex-r1/mutex-test-result.json').read_text())
            assert mutex_check['passed'] and mutex_check['exact_policy_reversal'] and mutex_check['extra_waits']==0
            mutex_proof=json.loads((AREA/'native-mutex-r1/prepared.json').read_text())
            assert mutex_proof['parent_native']==native_proof['native_sha256']
            native_proof=mutex_proof
        assert native_proof['full_native_compiled'] and native_proof['policy_cases']==17 and native_proof['policy_failures']==0
        assert manifest['native_sha256']==native_proof['frozen_native']
        native_dll=Path(native_proof['dll'])
        assert hashlib.sha256(native_dll.read_bytes()).hexdigest()==native_proof['native_sha256']
        manifest.update(native_build=str(native_dll.parent.relative_to(ROOT)),native_sha256=native_proof['native_sha256'])
    elif '--native-input-loss' in sys.argv:
        native_proof=json.loads((AREA/'native-input-loss-r1/prepared.json').read_text())
        assert native_proof['full_native_compiled'] and native_proof['broker_failures']==0
        assert manifest['native_sha256']==native_proof['parent_native']
        native_dll=Path(native_proof['dll'])
        assert hashlib.sha256(native_dll.read_bytes()).hexdigest()==native_proof['native_sha256']
        manifest.update(native_build=str(native_dll.parent.relative_to(ROOT)),native_sha256=native_proof['native_sha256'])
    write(target/'candidate-manifest.json',manifest)
    env={k:v for k,v in os.environ.items() if not k.upper().startswith(('GEARS','PYTHON','XR','OPENXR'))}
    test_mode=reload_test_mode(sys.argv)
    env.update(GEARSVR_TEST_FINITE_RANGE_AMMO='1',GEARSVR_HAMMERBURST_EMPTY_RELOAD_ONLY=test_mode['empty_only'],
               GEARSVR_HAMMERBURST_RELOAD='1',GEARSVR_HAMMERBURST_REPLACEMENT='1',
               GEARSVR_LANCER_REVIEW_VOLUMES='0',PYTHONIOENCODING='utf-8')
    python=[sys.executable,'-E','-s','-B','-X','utf8']
    common=dict(cwd=ROOT,env=env,creationflags=subprocess.CREATE_NO_WINDOW)
    selected=ROOT/'analysis/instances'/name
    result=dict(instance=name,game_started=False,operator_started=False,focus_samples=0,foreground_violations=[],
                cloak_samples=0,cloak_violations=[],recording='private GPU composed frames; encode after owned cleanup',
                shared_instance_changed=False,physical_headset_accepted=False,test_mode=test_mode)
    launched=False;operator=None;operator_log=None
    sys.path.insert(0,str(ROOT/'tools/native-capture'))
    from run_stereo_continuous import WindowsControl
    control=WindowsControl()
    user=ctypes.WinDLL('user32',use_last_error=True)
    user.GetForegroundWindow.restype=w.HWND
    user.GetWindowThreadProcessId.argtypes=[w.HWND,ctypes.POINTER(w.DWORD)]
    user.IsWindowVisible.argtypes=[w.HWND];user.IsWindowVisible.restype=w.BOOL
    user.IsWindow.argtypes=[w.HWND];user.IsWindow.restype=w.BOOL
    dwm=ctypes.WinDLL('dwmapi',use_last_error=True)
    dwm.DwmGetWindowAttribute.argtypes=[w.HWND,w.DWORD,ctypes.c_void_p,w.DWORD]
    dwm.DwmGetWindowAttribute.restype=ctypes.c_long
    def observe(state):
        if PAUSE.exists():raise RuntimeError('Owner paused automatic runtime; stop own actions and clean up normally')
        identities=[]
        if state.get('game_identity'):identities.append(state['game_identity'])
        else:
            # Startup failure can precede the session's game_identity field.
            # Authenticate only this fresh instance's exact birth receipt.
            for birth in selected.glob('*.birth.json'):
                try:
                    value=json.loads(birth.read_text(encoding='utf-8-sig'))
                    identity=value.get('game_identity')
                    if value.get('kind')=='reservation_birth' and identity and control.identity(identity['pid'])==identity:
                        identities.append(identity);result['birth_game_identity']=identity;result['game_started']=True
                except (OSError,ValueError,KeyError):pass
        if state.get('game_identity') and control.identity(state['game_identity']['pid'])==state['game_identity']:
            result['game_started']=True;result['game_identity']=state['game_identity']
        report=Path(state['session_report']) if state.get('session_report') else None
        if report and report.exists():
            try:
                details=json.loads(report.read_text(encoding='utf-8-sig'))
                if details.get('game_identity')==state.get('game_identity') and details.get('xr_identity'):identities.append(details['xr_identity'])
            except (PermissionError,json.JSONDecodeError):pass
        hwnd=user.GetForegroundWindow();pid=w.DWORD()
        if hwnd:user.GetWindowThreadProcessId(hwnd,ctypes.byref(pid))
        result['focus_samples']+=1
        for identity in identities:
            if identity['pid']==pid.value and control.identity(pid.value)==identity:
                result['foreground_violations'].append(dict(observed_ns=time.perf_counter_ns(),hwnd=int(hwnd),identity=identity))
                raise RuntimeError('Owned window became foreground; stop own actions and normal cleanup')
        failure=[]
        def window(hwnd,_):
            target_pid=w.DWORD();user.GetWindowThreadProcessId(hwnd,ctypes.byref(target_pid))
            owned=[v for v in identities if v['pid']==target_pid.value]
            if not owned or control.identity(target_pid.value)!=owned[0] or not user.IsWindowVisible(hwnd):return True
            cloaked=w.DWORD();code=dwm.DwmGetWindowAttribute(hwnd,14,ctypes.byref(cloaked),ctypes.sizeof(cloaked))
            if code and not user.IsWindow(hwnd):return True
            result['cloak_samples']+=1
            if code or not cloaked.value:failure.append(dict(identity=owned[0],hwnd=int(hwnd),hresult=code,cloaked=cloaked.value,observed_ns=time.perf_counter_ns()))
            return True
        callback=ctypes.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)(window);user.EnumWindows(callback,0)
        if failure:
            result['cloak_violations']+=failure
            raise RuntimeError('Owned visible-style window is not verifiably cloaked; normal own stop')
    try:
        with (target/'prepare.log').open('wb') as log:
            subprocess.run(python+[str(TOOL),'prepare','--name',name,'--manifest',str(target/'candidate-manifest.json'),
                                  '--config',str(AREA/'independent-r1/candidate-config.json')],check=True,stdout=log,stderr=subprocess.STDOUT,**common)
        write(target/'instance-binding.json',dict(name=name,root=str(selected),candidate=proof['dll_sha256']))
        if prepare_then_wait:
            result['prepared_waiting_for_resources']=True
            write(target/'active.json',result)
            print(json.dumps(dict(instance=name,prepared=True,game_started=False,waiting_for_resources=True)),flush=True)
            result['resource_admission_wait']=wait_for_resource_admission()
            result['prepared_waiting_for_resources']=False
            write(target/'active.json',result)
        with (target/'start.log').open('wb') as log:
            result['start_attempted']=True
            subprocess.run(python+[str(TOOL),'start','--name',name,'--wait-resources-seconds','600'],check=True,stdout=log,stderr=subprocess.STDOUT,**common)
        launched=True;result['worker_started']=True
        deadline=time.monotonic()+180
        state={}
        while time.monotonic()<deadline:
            if (selected/'isolated-done.json').exists():raise RuntimeError('Owned launcher ended before action readiness')
            path=selected/'launcher-session.json'
            if path.exists():
                try:
                    state=json.loads(path.read_text());observe(state)
                    if state.get('error'):
                        raise RuntimeError('Owned launcher rejected startup: '+str(state['error']))
                    report=Path(state['session_report']) if state.get('session_report') else None
                    if report and report.exists():
                        details=json.loads(report.read_text(encoding='utf-8-sig'))
                        if details.get('xr_identity') and state.get('vr_gamepad',{}).get('source')==1:break
                except (PermissionError,json.JSONDecodeError):pass
            time.sleep(.05)
        else:raise TimeoutError('Owned startup readiness timed out')
        operator_log=(target/'operator.log').open('wb')
        command=python+[str(AREA/'hammerburst_isolated_operator.py'),'--instance',name,'--output',str(selected/'actions'),
                        '--startup-handoff','--playground-ready','--equipment-chain','--hammerburst-core',
                        '--hammerburst-reload','--hammerburst-replacement']
        operator=subprocess.Popen(command,stdin=subprocess.DEVNULL,stdout=operator_log,stderr=subprocess.STDOUT,**common)
        result['operator_started']=True;result['operator_pid']=operator.pid;write(target/'active.json',result)
        deadline=time.monotonic()+300
        while operator.poll() is None:
            observe(state)
            if time.monotonic()>deadline:raise TimeoutError('Owned bounded action attempt timed out')
            time.sleep(.05)
        result['operator_exit']=operator.returncode
    except BaseException as error:
        result['error']=repr(error)
    finally:
        if result.get('prepared_waiting_for_resources'):
            result['prepared_waiting_for_resources']=False
            result['prepared_only_no_start']=not result.get('start_attempted',False)
        if not launched and (selected/'isolated-worker.json').exists():
            admission=json.loads((selected/'isolated-worker.json').read_text())
            own_worker=admission.get('worker_identity')
            launched=bool(own_worker and control.identity(own_worker['pid'])==own_worker)
        if selected.exists() and launched:
            (selected/'stop-actions').write_text('Own action attempt ended; no further input',encoding='utf-8')
            with (target/'stop.log').open('wb') as log:
                stop=subprocess.run(python+[str(TOOL),'stop','--name',name],stdout=log,stderr=subprocess.STDOUT,**common)
                result['stop_exit']=stop.returncode
            deadline=time.monotonic()+60
            while time.monotonic()<deadline and not (selected/'isolated-done.json').exists():time.sleep(.2)
            if (selected/'isolated-done.json').exists():result['cleanup']=json.loads((selected/'isolated-done.json').read_text())
            else:result['cleanup_unresolved']=True
            if operator:
                try:result['operator_exit']=operator.wait(timeout=20)
                except subprocess.TimeoutExpired:result['operator_still_live']=control.identity(operator.pid)
        if operator_log:operator_log.close()
        result['passed']=result.get('operator_exit')==0 and result.get('cleanup',{}).get('launcher_exit')==0 and result.get('cleanup',{}).get('owned_game_absent') and not result.get('cleanup',{}).get('owned_children',{}).get('remaining') and not result.get('error') and not result['foreground_violations'] and not result['cloak_violations']
        write(target/'result.json',result)
        print(json.dumps(result,indent=2))
    return 0 if result['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
