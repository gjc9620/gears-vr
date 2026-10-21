"""Sample the original Gnasher lever in an owned, test-only retail Playground.

The caller must start the private, behind-windows, muted game child and pass
its ``owned.json``. This probe never edits packages or publishes an asset. It
uses the game's own animation node and writes only bounded bone matrices to
the ignored analysis directory. A receipt is evidence only when ``passed`` is
true; a menu/startup timeout is not an animation result.
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
    parser.add_argument('--owned-json', required=True, type=Path)
    parser.add_argument('--dll', required=True, type=Path)
    parser.add_argument('--dll-sha256', required=True)
    parser.add_argument('--runtime-root', type=Path, default=ROOT,
                        help='Checkout providing the existing Playground runtime and private Python libraries')
    parser.add_argument('--output', required=True, type=Path)
    return parser


def matrix(bridge, mesh, index):
    raw = bytes.fromhex(bridge.call(mesh, 'GetBoneMatrix', BoneIndex=index))
    if len(raw) != 64:
        raise ValueError('GetBoneMatrix did not return a 4x4 float matrix')
    values = struct.unpack('<16f', raw)
    if not all(math.isfinite(value) for value in values):
        raise ValueError('Nonfinite bone matrix')
    return list(values)


def sample_owned_game(args, receipt):
    runtime_dir = args.runtime_root.resolve() / 'tools/playground'
    if not (runtime_dir / 'runtime.py').is_file():
        raise FileNotFoundError('Playground runtime absent from selected checkout')
    sys.path.insert(0, str(runtime_dir))
    from runtime import Bridge

    owned = json.loads(args.owned_json.read_text(encoding='utf-8'))
    if not owned.get('private_root') or not owned.get('launch', {}).get('test_windows_behind_requested'):
        raise ValueError('Not an owned behind-windows test game')
    identity = owned['game']
    bridge = None
    try:
        # The owned launcher publishes its child identity before UE3 registries
        # exist. Retry only these exact transient headers; all other resolver,
        # version and loader failures remain fatal.
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

        # SetPosition(false) suppresses sequence notifies. The active sequence
        # is modified only inside this private game child and then discarded.
        bridge.call(shotgun, 'SetWeaponAnim', AnimName=fname('SG_Chamber'))
        bridge.call(shotgun, 'FreezeWeaponAnim')
        for frame in range(35):
            position = SEQUENCE_LENGTH * frame / 34
            bridge.call(shotgun, 'SetWeaponAnimPosition', NewPosition=position)
            time.sleep(.025)  # allow one game animation update before sampling
            receipt['samples'].append({'frame': frame, 'position_seconds': position,
                                       'main': matrix(bridge, mesh, main),
                                       'lever': matrix(bridge, mesh, lever)})
        if len(receipt['samples']) != 35:
            raise RuntimeError('Incomplete bounded sequence sample')
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
    args = argument_parser().parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'analysis').resolve()):
        raise ValueError('Derived bone evidence must remain under analysis/')
    receipt = {'schema': 1, 'passed': False, 'source': 'owned retail runtime',
               'animation': 'SG_Chamber', 'length_seconds_from_package': SEQUENCE_LENGTH,
               'samples': []}
    owned = False
    try:
        owned = args.owned_json.is_file()
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
