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
