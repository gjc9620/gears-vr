"""Optional playground lifecycle inside the ordinary owned launcher session."""
import hashlib
import json
from pathlib import Path
import re
import sys
import time

ROOT=Path(__file__).resolve().parents[2]
ENTRY_RVA=0x1d8dd8
ENTRY=bytes.fromhex('405541544155415641574881eca0000000488d6c243048899da00000004889b5a80000004889bdb0000000488b0536268f014833c5488945604d8be0488bda4c')


def relative_artifact(manifest):
    build=Path(manifest['playground_build']);dll=manifest['playground_dll'];digest=manifest['playground_sha256']
    if (build.is_absolute() or '..' in build.parts or build.parts[:2]!=('tools','playground') or
            Path(dll).name!=dll or not dll.lower().endswith('.dll') or
            not isinstance(digest,str) or not re.fullmatch('[0-9a-f]{64}',digest)):
        raise ValueError('Invalid playground artifact identity/path')
    return build/dll


def artifact(manifest,root=ROOT):
    path=(Path(root)/relative_artifact(manifest)).resolve();digest=manifest['playground_sha256']
    if not path.is_relative_to(Path(root).resolve()):raise ValueError('Playground artifact escaped workspace')
    if hashlib.sha256(path.read_bytes()).hexdigest()!=digest:raise RuntimeError('Playground DLL SHA256 mismatch')
    return path


def reserve(manifest,identity,*,check_live=lambda:None):
    """Create a disabled trampoline before the engine's startup allocations."""
    sys.path.insert(0,str(ROOT/'tools/playground'))
    from runtime import Bridge,InitializationError
    path=artifact(manifest);check_live();bridge=None
    try:
        try:bridge=Bridge(identity,path,expected_sha256=manifest['playground_sha256'],reserve_only=True)
        except InitializationError as error:
            if error.code not in (4,5,6,7) or error.entry_restored is not True:raise
            check_live()
            return dict(reserved=False,entry_restored=True,initialization_code=error.code,error=str(error))
        check_live();bridge.verify()
        if bridge.r.read(bridge.r.base+ENTRY_RVA,len(ENTRY))!=ENTRY:
            # Reserve must never activate the hook or modify the game entry.
            cleanup=finish(bridge);bridge=None
            raise RuntimeError('Playground reservation changed its original entry: '+str(cleanup))
        return dict(reserved=True,hook_enabled=False,entry_restored=True,dll=str(path),sha256=manifest['playground_sha256'],pid=identity['pid'])
    finally:
        if bridge is not None:bridge.close()


def start(manifest,identity,*,check_live=lambda:None,attach=lambda bridge:None,clock=time.monotonic,sleep=time.sleep):
    sys.path.insert(0,str(ROOT/'tools/playground'))
    from runtime import Bridge,InitializationError
    from resolve_camera import Resolver
    path=artifact(manifest);profile=json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
    deadline=clock()+90
    while True:
        check_live();r=Resolver(identity['pid'],profile)
        try:r.prepare();break
        except RuntimeError as error:
            if str(error) not in ('Invalid name array header','Invalid object array header') or clock()>=deadline:raise
            sleep(.2)
        finally:r.p.close()
    check_live()
    try:bridge=Bridge(identity,path,expected_sha256=manifest['playground_sha256'])
    except InitializationError as error:
        # Resource/activation failures are optional only after the component's
        # rollback and an independent 64-byte entry check. Identity/certificate
        # failures, unknown status codes and cancellation still fail normally.
        if error.code not in (4,5,6,7) or error.entry_restored is not True:raise
        check_live()
        print('Playground unavailable; continuing VR with its original engine entry verified: '+str(error),flush=True)
        return dict(enabled=False,dll=str(path),sha256=manifest['playground_sha256'],pid=identity['pid'],
                    initialization_code=error.code,entry_restored=True,error=str(error))
    attach(bridge)
    while True:
        check_live();code=bridge.export('PlaygroundEnable')
        if code==0:break
        if code!=2 or clock()>=deadline:raise RuntimeError('Playground menu activation failed: '+str(code))
        sleep(.2)
    return dict(enabled=True,dll=str(path),sha256=manifest['playground_sha256'],pid=identity['pid'],
                status_file=str(path.with_name('playground-status-'+str(identity['pid'])+'.json')),
                menu_label='happyotaku playground')


def start_gamepad(manifest,identity,owner,*,check_live=lambda:None,simulator_operator=False):
    """Attach to the already attested owned Native prime; never inject again."""
    import struct
    sys.path.insert(0,str(ROOT/'tools/playground'))
    from runtime import Bridge
    path=(ROOT/manifest['native_build']/manifest['native_dll']).resolve()
    if not path.is_relative_to(ROOT.resolve()):raise ValueError('Native gamepad path escaped workspace')
    check_live()
    bridge=Bridge(identity,path,expected_sha256=manifest['native_sha256'],attach_only=True)
    address=None;returned=False;leased=False;gate=None
    try:
        check_live()
        address=bridge.allocate(struct.pack('<IIIIQ',24,1,owner['pid'],int(simulator_operator),owner['creation_time']))
        code=bridge.export('GearsCaptureStartVrGamepad',address);returned=True
        if code:raise RuntimeError('Owned pre-menu VR gamepad rejected: '+str(code))
        check_live()
        sys.path.insert(0,str(ROOT/'tools/game-io'))
        import background_input
        if background_input.change(identity['pid'],'status',identity['creation_time'])['state']!='original':
            raise RuntimeError('Controller focus gate already has another owner')
        if bridge.export('GearsCaptureVrGamepadGateLease',1):raise RuntimeError('Virtual-only pad gate lease rejected')
        leased=True
        gate,gate_attempts=_enable_gamepad_gate(identity,background_input.change,bridge.control.identity,check_live)
        check_live()
        return dict(enabled=True,before_level_discovery=True,keyboard_required=False,source=1 if simulator_operator else 0,
                    virtual_pad_only=True,focus_gate=gate,gate_lease_owned=True,focus_gate_attempts=gate_attempts)
    except BaseException:
        if leased:
            # Keep the Native neutral-pad retention guard if restoration fails.
            try:
                background_input.change(identity['pid'],'disable',identity['creation_time'])
                if bridge.export('GearsCaptureVrGamepadGateLease',0):raise RuntimeError('Pad gate release rejected')
            except Exception:pass
        raise
    finally:
        if returned and address:bridge.free(address)
        bridge.close()


def _enable_gamepad_gate(identity,change,check_identity,check_live):
    """Retry only a fully unwound, witnessed OpenThread disappearance.

    FrozenThreads raises the original OSError only after its cleanup succeeds;
    failed resume replaces it with RuntimeError and cannot reach this retry.
    No thread is skipped and no incomplete freeze reaches the patch body.
    """
    failures=[]
    for attempt in range(3):
        check_live()
        if check_identity(identity['pid'])!=identity:
            raise RuntimeError('Gamepad startup retry process identity changed')
        try:
            result=change(identity['pid'],'enable',identity['creation_time'])
            return dict(result,startup_thread_exit_retries=failures),attempt+1
        except OSError as error:
            if failures:error.add_note(json.dumps(dict(stage='gamepad_startup_gate_retry_history',failures=failures)))
            if getattr(error,'winerror',None)!=87 or attempt==2:raise
            notes=[]
            for note in getattr(error,'__notes__',()):
                try:value=json.loads(note)
                except (TypeError,ValueError):continue
                if isinstance(value,dict):notes.append(value)
            matches=[n for n in notes if n.get('stage')=='OpenThread' and n.get('pid')==identity['pid']
                     and n.get('winerror')==87 and isinstance(n.get('tid'),int) and n['tid']>0
                     and n.get('failed_tid_still_present') is False
                     and isinstance(n.get('enumerated'),list) and n['tid'] in n['enumerated']
                     and isinstance(n.get('fresh_threads'),list) and n['tid'] not in n['fresh_threads']
                     and not n.get('fresh_snapshot_error')]
            if len(matches)!=1:raise
            check_live()
            if check_identity(identity['pid'])!=identity:
                raise RuntimeError('Gamepad startup retry process identity changed') from error
            status=change(identity['pid'],'status',identity['creation_time'])
            if status.get('identity')!=identity or status.get('state')!='original':
                raise RuntimeError('Gamepad startup retry requires certified original code') from error
            failures.append(dict(attempt=attempt+1,error=repr(error),witness=matches[0],
                                 certified_state=status['state']))
            time.sleep(.05)
    raise RuntimeError('Gamepad gate startup did not finish')


def _restore_gamepad_gate(identity,change,check_identity):
    # Toolhelp can enumerate a short-lived retail worker immediately before it
    # exits; OpenThread then reports ERROR_INVALID_PARAMETER. Restart the WHOLE
    # certified transaction, never skip a thread or relax its freeze checks.
    for attempt in range(3):
        actual=check_identity(identity['pid'])
        if actual is None:return dict(game_exited=True),attempt+1
        if actual!=identity:raise RuntimeError('Gamepad cleanup process identity changed')
        try:return change(identity['pid'],'disable',identity['creation_time']),attempt+1
        except OSError as error:
            if getattr(error,'winerror',None)!=87 or attempt==2:raise
            # No retry may adopt unknown/mixed bytes. Status revalidates the
            # complete certified function; disable also checks the owner ledger.
            status=change(identity['pid'],'status',identity['creation_time'])
            if status.get('identity')!=identity or status.get('state') not in ('enabled','original'):
                raise RuntimeError('Gamepad cleanup retry has no certified code state') from error
            time.sleep(.05)
    raise RuntimeError('Gamepad gate restoration did not finish')


def finish_gamepad(manifest,identity,proof):
    if not proof or not proof.get('gate_lease_owned'):return dict(owned=False)
    sys.path.insert(0,str(ROOT/'tools/game-io'))
    sys.path.insert(0,str(ROOT/'tools/playground'))
    import background_input
    from runtime import Bridge
    from run_stereo_continuous import WindowsControl
    restored,attempts=_restore_gamepad_gate(identity,background_input.change,WindowsControl().identity)
    if restored.get('game_exited'):return restored
    path=(ROOT/manifest['native_build']/manifest['native_dll']).resolve()
    bridge=Bridge(identity,path,expected_sha256=manifest['native_sha256'],attach_only=True)
    try:
        if bridge.export('GearsCaptureVrGamepadGateLease',0):raise RuntimeError('Restored controller gate lease release rejected')
    finally:bridge.close()
    return dict(restored=restored['state']=='original',virtual_only_until_restored=True,restore_attempts=attempts)


def finish(bridge):
    if bridge is None:return dict(loaded=False)
    try:
        if bridge.control.game_absent(bridge.identity['pid']):return dict(loaded=True,game_exited=True,entry_restored=True)
        bridge.verify();code=bridge.export('PlaygroundShutdown')
        raw=bridge.r.read(bridge.r.base+ENTRY_RVA,len(ENTRY));restored=raw==ENTRY
        return dict(loaded=True,shutdown_code=code,entry_restored=restored,passed=code==0 and restored)
    finally:bridge.close()


def finish_reservation(manifest,identity):
    """Release an early, disabled hook even if startup stopped before Enable."""
    sys.path.insert(0,str(ROOT/'tools/playground'))
    from runtime import Bridge,WindowsControl
    if WindowsControl().game_absent(identity['pid']):return dict(loaded=True,game_exited=True,entry_restored=True)
    # Cleanup may attach to the recorded DLL but must never load it or reserve
    # another trampoline. No engine registry is needed for this operation.
    bridge=Bridge(identity,artifact(manifest),expected_sha256=manifest['playground_sha256'],attach_only=True)
    return dict(finish(bridge),reservation_only=True)
