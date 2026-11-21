"""Sample original Gnasher bones in an explicitly owned private game.

The caller must start the private, behind-windows, muted game child and pass
its ``owned.json``. This probe never edits packages or publishes an asset. It
uses the game's own animation node or original equipped pawn hand bones and
writes only bounded matrices to ignored analysis/. A receipt is evidence only
when ``passed`` is true; a startup timeout is not a motion result.
"""
import argparse
import json
import math
from pathlib import Path
import struct
import sys
import time
import traceback

ROOT = Path(__file__).resolve().parents[2]
SEQUENCE_LENGTH = 1.1666666269302368  # verified SG_Chamber package field


def wait_for(callback, seconds, reason):
    until = time.monotonic() + seconds
    while time.monotonic() < until:
        value = callback()
        if value:
            return value
        time.sleep(.08)
    raise TimeoutError(reason)


def argument_parser():
    parser = argparse.ArgumentParser(description=__doc__)
    owner=parser.add_mutually_exclusive_group(required=True)
    owner.add_argument('--owned-json', type=Path)
    owner.add_argument('--launcher-session', type=Path)
    parser.add_argument('--game-pid', type=int)
    parser.add_argument('--game-creation-time', type=int)
    parser.add_argument('--dll', required=True, type=Path)
    parser.add_argument('--dll-sha256', required=True)
    parser.add_argument('--runtime-root', type=Path, default=ROOT,
                        help='Checkout providing the existing Playground runtime and private Python libraries')
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--held-grips-only', action='store_true',
                        help='Capture original resting character hands relative to the held Gnasher')
    parser.add_argument('--combined-reference', action='store_true', help='One original held-hand bracket followed by the bounded SG_Chamber samples')
    return parser


def matrix(bridge, mesh, index):
    raw = bytes.fromhex(bridge.call(mesh, 'GetBoneMatrix', BoneIndex=index))
    if len(raw) != 64:
        raise ValueError('GetBoneMatrix did not return a 4x4 float matrix')
    values = struct.unpack('<16f', raw)
    if not all(math.isfinite(value) for value in values):
        raise ValueError('Nonfinite bone matrix')
    return list(values)


def close_reference_bridge(bridge, receipt, launcher_owned):
    """Release only this probe's responsibility; never close a parent session."""
    try:
        if launcher_owned:
            receipt.update(shutdown_mode='launcher_owned', shutdown_deferred=True,
                           launcher_cleanup_verified=False)
        else:
            receipt['shutdown_mode'] = 'probe_owned'
            receipt['shutdown'] = bridge.export('PlaygroundShutdown')
            if receipt['shutdown'] != 0:
                raise RuntimeError('Playground shutdown was not clean')
    finally:
        bridge.close()


def sample_owned_game(args, receipt):
    runtime_dir = args.runtime_root.resolve() / 'tools/playground'
    if not (runtime_dir / 'runtime.py').is_file():
        raise FileNotFoundError('Playground runtime absent from selected checkout')
    sys.path.insert(0, str(runtime_dir))
    from runtime import Bridge

    attach_only=bool(args.launcher_session)
    if attach_only:
        session=json.loads(args.launcher_session.read_text(encoding='utf-8-sig'))
        pg=session.get('playground',{});report_path=Path(session.get('session_report',''))
        report=json.loads(report_path.read_text(encoding='utf-8-sig')) if report_path.is_file() else {}
        running=session.get('phase')=='session_owned' or (session.get('phase')=='waiting_for_level' and report.get('phase')=='running' and report.get('game_identity')==session.get('game_identity'))
        identity=session.get('game_identity',{})
        if not running or not session.get('test_audio_guard',{}).get('enabled') or not pg.get('enabled'):
            raise ValueError('Not an owned muted running launcher session')
        if pg.get('pid')!=identity.get('pid') or Path(pg.get('dll','')).resolve()!=args.dll.resolve() or pg.get('sha256')!=args.dll_sha256:
            raise ValueError('Launcher Gnasher reference DLL/game mismatch')
        if args.game_pid!=identity.get('pid') or args.game_creation_time!=identity.get('creation_time'):
            raise ValueError('Launcher identity differs from explicit run owner')
        receipt['game_identity']=identity;receipt['launcher_owner_identity']=session.get('owner_identity')
    else:
        owned=json.loads(args.owned_json.read_text(encoding='utf-8'))
        if not owned.get('private_root') or not owned.get('launch',{}).get('test_windows_behind_requested'):
            raise ValueError('Not an owned behind-windows test game')
        identity=owned['game']
    bridge = None
    try:
        # The owned launcher publishes its child identity before UE3 registries
        # exist. Retry only these exact transient headers; all other resolver,
        # version and loader failures remain fatal.
        registry_deadline = time.monotonic() + 90
        while True:
            try:
                bridge = Bridge(identity, args.dll, expected_sha256=args.dll_sha256, attach_only=attach_only)
                if attach_only:
                    bridge.r.prepare()
                    initialized=bridge.export('PlaygroundInitialize')
                    if initialized!=0:raise RuntimeError(f'Owned Playground initialize failed: {initialized}')
                break
            except RuntimeError as error:
                if str(error) not in ('Invalid name array header', 'Invalid object array header'):
                    raise
                if time.monotonic() >= registry_deadline:
                    raise TimeoutError('Original UE3 registries never became ready') from error
                time.sleep(.5)
        wait_for(lambda: bridge.export('PlaygroundEnable') == 0,
                 60, 'Playground menu viewport not available')
        if bridge.export('PlaygroundTestEnter') != 0:
            raise RuntimeError('Owned muted test entry rejected')
        status_path = args.dll.with_name(f'playground-status-{identity["pid"]}.json')

        def status():
            try:
                return json.loads(status_path.read_text(encoding='utf-8'))
            except (FileNotFoundError, json.JSONDecodeError):
                return {}

        ready = wait_for(lambda: status() if status().get('ready') else None,
                         120, 'Playground arena not ready')
        if ready.get('failures'):
            raise RuntimeError('Playground arena setup reported failures')
        resolver = bridge.r

        # The owned arena allocates pickups after Bridge initialized its name
        # and object snapshots. Refresh once the status reports those objects;
        # a concurrent registry growth is a transient discovery race only.
        def refreshed():
            try:
                resolver.prepare()
                return True
            except RuntimeError as error:
                if str(error) != 'Name registry changed during discovery':
                    raise
                return False
        wait_for(refreshed, 10, 'Original UE3 registry never stabilized')

        def property_of(obj, field):
            return resolver.property(resolver.identity(obj)['cls'], field, True)

        def pointer(obj, field):
            return resolver.u64(obj + property_of(obj, field)['offset'])

        def class_name(obj):
            return resolver.class_name(resolver.identity(obj))

        def fname(value):
            for index, address in enumerate(resolver.name_pointers):
                if not address:
                    continue
                try:
                    if resolver.name(index) == value:
                        return index, 0
                except (RuntimeError, UnicodeError):
                    continue
            raise RuntimeError('FName absent: ' + value)

        pawn = int(ready['pawn'], 0)
        pc = pointer(pawn, 'Controller')
        inventory = pointer(pawn, 'InvManager')
        world = pointer(pc, 'WorldInfo')
        pickup = 0
        for obj in resolver.object_pointers:
            if not obj:
                continue
            try:
                ident = resolver.identity(obj)
                if ident['name'].startswith('Default__') or class_name(obj) != 'WarDroppedPickup':
                    continue
                if pointer(obj, 'WorldInfo') != world:
                    continue
                item_class = pointer(obj, 'InventoryClass')
                if item_class and resolver.identity(item_class)['name'] == 'Weap_Shotgun':
                    pickup = obj
                    break
            except RuntimeError:
                continue
        if not pickup:
            raise RuntimeError('Original Gnasher pickup missing in test arena')
        bridge.call(pickup, 'GiveTo', P=pawn)

        def owned_shotgun():
            item = pointer(inventory, 'InventoryChain')
            for _ in range(32):
                if not item:
                    break
                if class_name(item) == 'Weap_Shotgun':
                    return item
                item = pointer(item, 'Inventory')
            return 0

        shotgun = wait_for(owned_shotgun, 5, 'Original GiveTo did not add Gnasher')
        bridge.call(inventory, 'SetCurrentWeapon', DesiredWeapon=shotgun)
        wait_for(lambda: pointer(pawn, 'Weapon') == shotgun, 8,
                 'Original Gnasher equip did not finish')
        mesh = pointer(shotgun, 'Mesh')
        if class_name(mesh) != 'SkeletalMeshComponent':
            raise RuntimeError('Held Gnasher mesh is not skeletal')
        animations = pointer(mesh, 'Animations')
        if class_name(animations) != 'AnimNodeSequence':
            raise RuntimeError('Held Gnasher does not use the verified direct sequence node')
        node_identity = resolver.identity(animations)
        outer = node_identity['outer']
        attached = False
        for _ in range(8):
            if outer in (mesh, shotgun):
                attached = True
                break
            outer = resolver.identity(outer)['outer'] if outer else 0
        if node_identity['name'].startswith('Default__') or not attached:
            raise RuntimeError('Animation node is not owned by the held Gnasher instance')
        main = bridge.call(mesh, 'MatchRefBone', BoneName=fname('b_Shotgun_Main'))
        lever = bridge.call(mesh, 'MatchRefBone', BoneName=fname('b_Shotgun_Lever'))
        if (main, lever) != (1, 2):
            raise RuntimeError(f'Gnasher runtime bone indices changed: {main}, {lever}')
        receipt.update(game_pid=identity['pid'], shotgun=resolver.identity(shotgun),
                       mesh=resolver.identity(mesh), anim_node=node_identity,
                       bone_indices={'main': main, 'lever': lever})

        if args.held_grips_only or args.combined_reference:
            pawn_mesh = pointer(pawn, 'Mesh')
            if class_name(pawn_mesh) != 'SkeletalMeshComponent':
                raise RuntimeError('Original equipped pawn mesh is not skeletal')
            hand_names = ('b_MF_Hand_R', 'b_MF_Hand_L', 'b_MF_IK_Hand_L')
            hand_indices = {name: bridge.call(pawn_mesh, 'MatchRefBone',
                                             BoneName=fname(name)) for name in hand_names}
            if any(index < 0 for index in hand_indices.values()) or \
                    len(set(hand_indices.values())) != len(hand_indices):
                raise RuntimeError('Original pawn hand bones unavailable or aliased')
            receipt['pawn_mesh'] = resolver.identity(pawn_mesh)
            receipt['hand_indices'] = hand_indices
            receipt['held_grips'] = []
            receipt['left_hand_ik_on'] = bool(wait_for(
                lambda: bridge.call(pawn, 'ShouldLeftHandIKBeOn'), 10,
                'Original Gnasher left-hand IK did not settle'))
            for frame in range(24):
                receipt['held_grips'].append({
                    'frame': frame,
                    'main': matrix(bridge, mesh, main),
                    'lever': matrix(bridge, mesh, lever),
                    'hand_right': matrix(bridge, pawn_mesh, hand_indices['b_MF_Hand_R']),
                    'hand_left': matrix(bridge, pawn_mesh, hand_indices['b_MF_Hand_L']),
                    'ik_left': matrix(bridge, pawn_mesh, hand_indices['b_MF_IK_Hand_L']),
                })
                time.sleep(.025)
            if len(receipt['held_grips']) != 24:
                raise RuntimeError('Incomplete original held-grip sample')
        if not args.held_grips_only:
            # SetPosition(false) suppresses sequence notifies. The active
            # sequence changes only inside this private, discarded game child.
            bridge.call(shotgun, 'SetWeaponAnim', AnimName=fname('SG_Chamber'))
            bridge.call(shotgun, 'FreezeWeaponAnim')
            receipt['capture_protocol'] = 'bracketed_original_sg35_v1'
            receipt['separate_reads_not_atomic'] = True
            for frame in range(35):
                position = SEQUENCE_LENGTH * frame / 34
                bridge.call(shotgun, 'SetWeaponAnimPosition', NewPosition=position)
                time.sleep(.025)  # allow one game animation update before sampling
                current_time=struct.unpack('<f',resolver.read(animations+property_of(animations,'CurrentTime')['offset'],4))[0]
                main_before=matrix(bridge, mesh, main)
                lever_observation=matrix(bridge, mesh, lever)
                main_after=matrix(bridge, mesh, main)
                after_time=struct.unpack('<f',resolver.read(animations+property_of(animations,'CurrentTime')['offset'],4))[0]
                receipt['samples'].append({'frame': frame, 'position_seconds': position, 'observed_node_time_seconds':current_time,
                                           'main':main_before,'lever':lever_observation,'main_after':main_after,
                                           'observed_node_time_after_seconds':after_time})
            if len(receipt['samples']) != 35:
                raise RuntimeError('Incomplete bounded sequence sample')
    finally:
        if bridge:
            try:
                close_reference_bridge(bridge, receipt, attach_only)
            except BaseException:
                receipt['shutdown_error'] = traceback.format_exc()
                raise
    if not attach_only and receipt.get('shutdown') != 0:
        raise RuntimeError('Playground shutdown was not clean')
    receipt['sampling_passed'] = True
    receipt['passed'] = not attach_only
    receipt['state'] = 'sampled_awaiting_launcher_cleanup' if attach_only else 'sampled_probe_shutdown_complete'


def main():
    args = argument_parser().parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'analysis').resolve()):
        raise ValueError('Derived bone evidence must remain under analysis/')
    receipt = {'schema': 2 if args.held_grips_only else 1,
               'passed': False, 'source': 'owned original game runtime',
               'animation': 'held idle' if args.held_grips_only else 'SG_Chamber',
               'samples': []}
    if args.combined_reference and args.held_grips_only:raise ValueError('Choose one reference mode')
    if not args.held_grips_only:
        receipt['length_seconds_from_package'] = SEQUENCE_LENGTH
    owned = False
    try:
        owned = args.owned_json is not None and args.owned_json.is_file()
        sample_owned_game(args, receipt)
    except BaseException:
        receipt['error'] = traceback.format_exc()
        raise
    finally:
        try:
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
        finally:
            if owned:
                # The owned-game launcher watches this marker and closes its child.
                (args.owned_json.parent / 'stop').write_text('stop', encoding='ascii')


if __name__ == '__main__':
    main()
