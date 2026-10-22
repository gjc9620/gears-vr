"""Collect bounded original left-hand and Lancer transforms in an owned retail range.

This is a read-only pose probe after the original GiveTo/SetCurrentWeapon path.
The sequential ProcessEvent calls do not prove that the returned matrices came
from one render frame. Output stays in ignored analysis/ and is not a grip mount.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
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


def matrix(raw):
    if len(raw) != 64:
        raise ValueError('Expected a 64-byte original bone/component matrix')
    values = struct.unpack('<16f', raw)
    if not all(math.isfinite(value) for value in values):
        raise ValueError('Nonfinite original matrix')
    return list(values)


def sample_owned_game(args, receipt):
    sys.path.insert(0, str(args.runtime_root.resolve() / 'tools/playground'))
    from runtime import Bridge

    owned = json.loads(args.owned_json.read_text(encoding='utf-8'))
    if not owned.get('private_root') or not owned.get('launch', {}).get('test_windows_behind_requested'):
        raise ValueError('Not an owned behind-windows test game')
    identity = owned['game']
    bridge = None
    try:
        registry_deadline = time.monotonic() + 90
        while True:
            try:
                bridge = Bridge(identity, args.dll, expected_sha256=args.dll_sha256)
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

        def fname(value):
            for index, address in enumerate(resolver.name_pointers):
                if address and resolver.name(index) == value:
                    return index, 0
            raise RuntimeError('FName absent: ' + value)

        def component_matrix(component):
            prop = property_of(component, 'LocalToWorld')
            if prop['type'] != 'StructProperty' or resolver.u32(prop['address'] + 0x74) != 64:
                raise RuntimeError('Unexpected LocalToWorld property layout')
            return matrix(resolver.read(component + prop['offset'], 64))

        def bone_matrix(component, index):
            return matrix(bytes.fromhex(bridge.call(component, 'GetBoneMatrix', BoneIndex=index)))

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
            except RuntimeError:
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
        bones = {}
        for bone in ('b_MF_IK_Hand_L', 'b_MF_Hand_L', 'b_MF_Hand_R'):
            index = bridge.call(pawn_mesh, 'MatchRefBone', BoneName=fname(bone))
            if index < 0:
                raise RuntimeError('Original pawn bone absent: ' + bone)
            bones[bone] = index
        receipt.update(game_pid=identity['pid'], pawn_mesh=resolver.identity(pawn_mesh),
                       weapon_mesh=resolver.identity(weapon_mesh),
                       pawn_mesh_asset=resolver.identity(pointer(pawn_mesh, 'SkeletalMesh')),
                       weapon_mesh_asset=resolver.identity(pointer(weapon_mesh, 'SkeletalMesh')),
                       bone_indices=bones, left_hand_ik_on=bridge.call(pawn, 'ShouldLeftHandIKBeOn'))
        if not receipt['left_hand_ik_on']:
            raise RuntimeError('Original left-hand IK is not active in this pose')
        for sample in range(24):
            if pointer(pawn, 'Weapon') != weapon or pointer(pc, 'WorldInfo') != world:
                raise RuntimeError('Original weapon/world identity changed during sampling')
            started_ns = time.monotonic_ns()
            frames = {bone: bone_matrix(pawn_mesh, index) for bone, index in bones.items()}
            frames['pawn_local_to_world'] = component_matrix(pawn_mesh)
            frames['weapon_local_to_world'] = component_matrix(weapon_mesh)
            frames.update(sample=sample, started_ns=started_ns,
                          finished_ns=time.monotonic_ns())
            receipt['samples'].append(frames)
            time.sleep(.05)
        if len(receipt['samples']) != 24:
            raise RuntimeError('Incomplete bounded original pose sample')
    finally:
        if bridge:
            try:
                receipt['shutdown'] = bridge.export('PlaygroundShutdown')
            except BaseException:
                receipt['shutdown_error'] = traceback.format_exc()
            bridge.close()
    if receipt.get('shutdown') != 0:
        raise RuntimeError('Playground shutdown was not clean')
    receipt['passed'] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--owned-json', required=True, type=Path)
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
    owned = args.owned_json.is_file()
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
