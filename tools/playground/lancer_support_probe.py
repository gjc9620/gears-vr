"""Collect bounded original left-hand and Lancer transforms in an owned retail range.

After the original GiveTo/SetCurrentWeapon path, a private game-thread callback
captures the pawn bones and unchanged gun matrix after one PlayerTick. Output
stays in ignored analysis/ and is not yet a VR grip mount.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
import time
import traceback

ROOT = Path(__file__).resolve().parents[2]


def wait_for(callback, seconds, reason):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        result = callback()
        if result:
            return result
        time.sleep(.08)
    raise TimeoutError(reason)


def matrix(text):
    values = [float(value) for value in text.split(',')]
    if len(values) != 16:
        raise ValueError('Expected 16 original bone/component matrix values')
    if not all(math.isfinite(value) for value in values):
        raise ValueError('Nonfinite original matrix')
    return values


def parse_pose_row(line):
    fields = dict(item.split('=', 1) for item in line.split()[1:])
    if set(fields) != {'sample', 'qpc', 'pawn', 'gun', 'ik_left', 'hand_left', 'hand_right'}:
        raise ValueError('Unexpected game-thread Lancer pose row')
    return {'sample': int(fields['sample']), 'qpc': int(fields['qpc']),
            **{key: matrix(fields[key]) for key in ('pawn', 'gun', 'ik_left', 'hand_left', 'hand_right')}}


def parse_visual_row(line):
    fields = dict(item.split('=', 1) for item in line.split()[1:])
    expected = {'magazine_known', 'magazine_present', 'magazine_hidden',
                'magazine_attached', 'magazine_asset', 'ammo_known', 'ammo_present',
                'ammo_strength', 'ammo_target', 'ammo_scale'}
    if set(fields) != expected:
        raise ValueError('Unexpected original Lancer magazine/control row')
    booleans = {key: fields[key] for key in expected
                if key.endswith(('known', 'present', 'hidden', 'attached'))}
    if any(value not in ('0', '1') for value in booleans.values()):
        raise ValueError('Invalid original visual flag')
    floats = {key: float(fields[key]) for key in ('ammo_strength', 'ammo_target', 'ammo_scale')}
    if not all(math.isfinite(value) for value in floats.values()):
        raise ValueError('Nonfinite original visual value')
    return {key: value == '1' for key, value in booleans.items()} | {
        'magazine_asset': fields['magazine_asset'], **floats}


def sample_owned_game(args, receipt):
    sys.path.insert(0, str(args.runtime_root.resolve() / 'tools/playground'))
    from runtime import Bridge

    if args.launcher_session:
        session = json.loads(args.launcher_session.read_text(encoding='utf-8'))
        playground = session.get('playground', {})
        if (session.get('phase') != 'session_owned' or
                not session.get('test_audio_guard', {}).get('enabled') or
                not playground.get('enabled') or
                Path(playground.get('dll', '')).resolve() != args.dll.resolve() or
                playground.get('sha256') != args.dll_sha256):
            raise ValueError('Live launcher session does not own this muted Playground DLL')
        identity = session['game_identity']
        if playground.get('pid') != identity.get('pid'):
            raise ValueError('Live launcher Playground/game identity mismatch')
        attach_only = True
    else:
        owned = json.loads(args.owned_json.read_text(encoding='utf-8'))
        if not owned.get('private_root') or not owned.get('launch', {}).get('test_windows_behind_requested'):
            raise ValueError('Not an owned behind-windows test game')
        identity = owned['game']
        attach_only = False
    bridge = None
    try:
        registry_deadline = time.monotonic() + 90
        while True:
            try:
                bridge = Bridge(identity, args.dll, expected_sha256=args.dll_sha256,
                                attach_only=attach_only)
                if attach_only:
                    # attach_only skips Resolver.prepare and component init;
                    # the owned launcher may already have shut this test DLL
                    # down after an earlier bounded probe in the same child.
                    bridge.r.prepare()
                    if bridge.export('PlaygroundInitialize') != 0:
                        raise RuntimeError('Owned Playground could not initialize')
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
            raise RuntimeError('Owned test entry rejected')
        status_path = args.dll.with_name(f'playground-status-{identity["pid"]}.json')

        def status():
            try:
                return json.loads(status_path.read_text(encoding='utf-8'))
            except (FileNotFoundError, json.JSONDecodeError):
                return {}

        ready = wait_for(lambda: status() if status().get('ready') else None,
                         120, 'Playground arena not ready')
        if ready.get('failures'):
            raise RuntimeError('Playground arena setup failed')
        resolver = bridge.r

        def property_of(obj, field):
            return resolver.property(resolver.identity(obj)['cls'], field, True)

        def pointer(obj, field):
            return resolver.u64(obj + property_of(obj, field)['offset'])

        def class_name(obj):
            return resolver.class_name(resolver.identity(obj))

        pawn = int(ready['pawn'], 0)
        pc = pointer(pawn, 'Controller')
        inventory = pointer(pawn, 'InvManager')
        world = pointer(pc, 'WorldInfo')
        pickup = 0
        for obj in resolver.object_pointers:
            if not obj:
                continue
            try:
                item = resolver.identity(obj)
                if item['name'].startswith('Default__') or class_name(obj) != 'WarDroppedPickup':
                    continue
                if pointer(obj, 'WorldInfo') != world:
                    continue
                item_class = pointer(obj, 'InventoryClass')
                if item_class and resolver.identity(item_class)['name'] == 'Weap_AssaultRifle':
                    pickup = obj
                    break
            except (RuntimeError, TypeError, KeyError):
                continue
        if not pickup:
            raise RuntimeError('Original Lancer pickup missing in owned arena')
        bridge.call(pickup, 'GiveTo', P=pawn)

        def owned_lancer():
            item = pointer(inventory, 'InventoryChain')
            for _ in range(32):
                if not item:
                    break
                if class_name(item) == 'Weap_AssaultRifle':
                    return item
                item = pointer(item, 'Inventory')
            return 0

        weapon = wait_for(owned_lancer, 5, 'Original GiveTo did not add Lancer')
        bridge.call(inventory, 'SetCurrentWeapon', DesiredWeapon=weapon)
        wait_for(lambda: pointer(pawn, 'Weapon') == weapon, 8,
                 'Original Lancer equip did not finish')
        pawn_mesh = pointer(pawn, 'Mesh')
        weapon_mesh = pointer(weapon, 'Mesh')
        if class_name(pawn_mesh) != 'SkeletalMeshComponent' or class_name(weapon_mesh) != 'SkeletalMeshComponent':
            raise RuntimeError('Held pawn/weapon mesh is not skeletal')
        receipt.update(game_pid=identity['pid'], pawn_mesh=resolver.identity(pawn_mesh),
                       weapon_mesh=resolver.identity(weapon_mesh),
                       pawn_mesh_asset=resolver.identity(pointer(pawn_mesh, 'SkeletalMesh')),
                       weapon_mesh_asset=resolver.identity(pointer(weapon_mesh, 'SkeletalMesh')))
        # Pawn.Weapon changes before its original switching stance ends. The
        # original ShouldLeftHandIKBeOn explicitly returns false while
        # bSwitchingWeapons is set, so wait for the playable held pose.
        def settled_left_hand():
            return bridge.call(pawn, 'ShouldLeftHandIKBeOn')
        receipt['left_hand_ik_on'] = bool(wait_for(
            settled_left_hand, 10, 'Original Lancer left-hand IK did not settle'))
        if bridge.export('PlaygroundLancerSupportProbeEnable') != 0:
            raise RuntimeError('Private game-thread Lancer probe refused to start')

        def probe_done():
            value = bridge.export('PlaygroundLancerSupportProbeStatus')
            state, count = value >> 16, value & 0xffff
            if state == 4:
                raise RuntimeError(f'Game-thread Lancer probe rejected after {count} samples')
            if state == 3:
                if count != 24:
                    raise RuntimeError(f'Game-thread Lancer probe stopped after {count} samples')
                return True
            return False

        wait_for(probe_done, 15, 'Game-thread Lancer pose sampling did not finish')
    finally:
        if bridge:
            try:
                receipt['shutdown'] = bridge.export('PlaygroundShutdown')
            except BaseException:
                receipt['shutdown_error'] = traceback.format_exc()
            bridge.close()
    if receipt.get('shutdown') != 0:
        raise RuntimeError('Playground shutdown was not clean')
    log_path = args.dll.with_name(f'playground-{identity["pid"]}.log')
    lines = log_path.read_text(encoding='utf-8-sig').splitlines()
    beginnings = [line for line in lines if line.startswith('lancer_support_probe_begin ')]
    poses = [parse_pose_row(line) for line in lines if line.startswith('lancer_support_pose ')]
    visuals = [parse_visual_row(line) for line in lines
               if line.startswith('lancer_support_visual ')]
    if (len(beginnings) != 1 or len(poses) != 24 or len(visuals) != 1 or
            [row['sample'] for row in poses] != list(range(24))):
        raise RuntimeError('Bounded same-tick original pose rows missing from private log')
    receipt.update(game_thread_probe=beginnings[0], game_thread_log=str(log_path), samples=poses,
                   original_visual=visuals[0],
                   log_sha256=hashlib.sha256(log_path.read_bytes()).hexdigest())
    receipt['passed'] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    owner = parser.add_mutually_exclusive_group(required=True)
    owner.add_argument('--owned-json', type=Path)
    owner.add_argument('--launcher-session', type=Path,
                       help='Attach to an already owned and muted private launcher session')
    parser.add_argument('--dll', required=True, type=Path)
    parser.add_argument('--dll-sha256', required=True)
    parser.add_argument('--runtime-root', type=Path, default=ROOT)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'analysis').resolve()):
        raise ValueError('Proprietary bone evidence must remain under analysis/')
    receipt = {'schema': 1, 'passed': False, 'source': 'owned retail runtime',
               'probe_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               'samples': []}
    owned = args.owned_json is not None and args.owned_json.is_file()
    try:
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
                (args.owned_json.parent / 'stop').write_text('stop', encoding='ascii')


if __name__ == '__main__':
    main()
