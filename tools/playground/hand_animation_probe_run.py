"""Observe original pickup and melee hand ownership in an owned retail arena.

The external test owner launches and closes the behind-windows game. This
runner invokes only original game methods and a default-off read-only observer;
every attempted action is recorded separately from an observed animation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import time
import traceback


ROOT = Path(__file__).resolve().parents[2]
TERMINAL_FIELDS = {
    'status', 'reason', 'samples', 'snapshots', 'calls', 'event_lines',
    'hook_callbacks', 'hook_mask', 'bridge_mask', 'special_changes',
    'melee_changes', 'started_active', 'overflow', 'field_limit',
    'function_limit', 'ownership_proved', 'writes', 'gap',
}


def validate_log(log, expected_status):
    lines = log.splitlines()
    terminal_lines = [line for line in lines
                      if line.startswith('hand_animation_probe_end ')]
    if len(terminal_lines) != 1:
        raise RuntimeError('Expected exactly one hand animation terminal row')
    terminal = dict(part.split('=', 1) for part in terminal_lines[0].split()[1:])
    if set(terminal) != TERMINAL_FIELDS:
        raise RuntimeError('Hand animation terminal schema changed')
    for name in TERMINAL_FIELDS - {'reason', 'gap'}:
        terminal[name] = int(terminal[name], 16 if name.endswith('_mask') else 10)
    if (terminal['status'] != expected_status or expected_status not in (3, 4) or
            terminal['samples'] < 1 or
            terminal['snapshots'] < terminal['samples'] or
            terminal['event_lines'] != sum(line.startswith('hand_animation_event ')
                                           for line in lines) or
            terminal['hook_callbacks'] < terminal['calls'] or
            any(terminal[name] for name in ('overflow', 'field_limit',
                                            'function_limit', 'ownership_proved',
                                            'writes'))):
        raise RuntimeError('Hand animation observer did not finish a bounded read-only sample')
    frames = [fields for line in lines if line.startswith('hand_animation_frame ')
              if (fields := dict(part.split('=', 1) for part in line.split()[1:]))]
    if (len(frames) != terminal['snapshots'] or
            any(not (int(frame['state_known']) & 256) or
                int(frame['slots']) < 1 for frame in frames)):
        raise RuntimeError('Current pawn body stance slots were not observed')
    return terminal


def wait_for(check, seconds, reason):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        value = check()
        if value:
            return value
        time.sleep(.08)
    raise TimeoutError(reason)


def verify_owned(args):
    owned = json.loads(args.owned_json.read_text(encoding='utf-8'))
    identity = owned['game']
    launch = owned.get('launch', {})
    retail = (args.runtime_root.resolve() / 'Gears of War Reloaded' /
              'Binaries_x64' / 'GOWDE-Steam.exe').resolve()
    if (not owned.get('private_root') or not owned.get('owner') or
            not launch.get('test_windows_behind_requested') or
            launch.get('background_input_requested') is not False or
            launch.get('steam_app_id') != '2523720' or
            Path(launch.get('game_path', '')).resolve() != retail or
            Path(identity.get('image', '')).resolve() != retail):
        raise ValueError('Expected an owned, behind-windows retail test')
    return identity


def run(args, receipt):
    sys.path.insert(0, str(args.runtime_root.resolve() / 'tools/playground'))
    from runtime import Bridge

    identity = verify_owned(args)
    bridge = None
    try:
        def attach():
            nonlocal bridge
            try:
                bridge = Bridge(identity, args.dll, expected_sha256=args.dll_sha256)
                return True
            except RuntimeError as error:
                if str(error) not in ('Invalid name array header',
                                      'Invalid object array header'):
                    raise
                return False

        wait_for(attach, 90, 'Original object registries did not become ready')
        wait_for(lambda: bridge.export('PlaygroundEnable') == 0,
                 60, 'Playground menu viewport unavailable')
        if bridge.export('PlaygroundTestEnter') != 0:
            raise RuntimeError('Private arena entry rejected')
        status_path = args.dll.with_name(f'playground-status-{identity["pid"]}.json')

        def arena_ready():
            try:
                value = json.loads(status_path.read_text(encoding='utf-8'))
            except (FileNotFoundError, json.JSONDecodeError):
                return False
            if value.get('failures'):
                raise RuntimeError('Private arena setup failed')
            return value if value.get('ready') and value.get('pickups', 0) else False

        arena = wait_for(arena_ready, 120, 'Private arena did not become ready')
        resolver = bridge.r
        resolver.prepare()  # Include newly spawned original pickup actors.

        def property_of(obj, name):
            return resolver.property(resolver.identity(obj)['cls'], name, True)

        def pointer(obj, name):
            return resolver.u64(obj + property_of(obj, name)['offset'])

        def class_name(obj):
            return resolver.class_name(resolver.identity(obj))

        pawn = int(arena['pawn'], 0)
        pc = pointer(pawn, 'Controller')
        inventory = pointer(pawn, 'InvManager')
        world = pointer(pc, 'WorldInfo')
        if not pc or not inventory or not world:
            raise RuntimeError('Current retail pawn ownership incomplete')
        special_field = property_of(pawn, 'SpecialMove')
        if (resolver.class_name(resolver.identity(special_field['address'])) != 'ByteProperty' or
                resolver.u32(special_field['address'] + 0x74) != 1):
            raise RuntimeError('Original SpecialMove declaration changed')

        def special_move():
            return resolver.read(pawn + special_field['offset'], 1)[0]

        wait_for(lambda: special_move() == 0,
                 8, 'Original pawn had a pending special move before pickup')
        body_slots_field = property_of(pawn, 'BodyStanceNodes')
        body_slots_inner = resolver.u64(body_slots_field['address'] + 0xd0)
        _, body_slots_count, body_slots_capacity = struct.unpack(
            '<Qii', resolver.read(pawn + body_slots_field['offset'], 16))
        receipt['body_slots_layout'] = {
            'kind': resolver.class_name(resolver.identity(body_slots_field['address'])),
            'offset': body_slots_field['offset'],
            'bytes': resolver.u32(body_slots_field['address'] + 0x74),
            'dimension': resolver.u32(body_slots_field['address'] + 0x70),
            'owner_bytes': resolver.u32(resolver.identity(pawn)['cls'] + 0x88),
            'inner_kind': resolver.class_name(resolver.identity(body_slots_inner)),
            'inner_bytes': resolver.u32(body_slots_inner + 0x74),
            'inner_dimension': resolver.u32(body_slots_inner + 0x70),
            'count': body_slots_count,
            'capacity': body_slots_capacity,
        }
        pickup_by_class = {}
        for obj in resolver.object_pointers:
            if not obj:
                continue
            try:
                item = resolver.identity(obj)
                if (item['name'].startswith('Default__') or
                        class_name(obj) != 'WarDroppedPickup' or
                        pointer(obj, 'WorldInfo') != world):
                    continue
                cls = pointer(obj, 'InventoryClass')
                if cls:
                    pickup_by_class.setdefault(resolver.identity(cls)['name'], obj)
            except (RuntimeError, TypeError, KeyError):
                continue

        def current_weapon(target_class):
            item = pointer(inventory, 'InventoryChain')
            for _ in range(32):
                if not item:
                    break
                if class_name(item) == target_class:
                    return item
                item = pointer(item, 'Inventory')
            return 0

        target_class = next((name for name in (
            'Weap_SniperRifle', 'Weap_Boomer', 'Weap_Bow',
            'Weap_LocustPistol', 'Weap_Shotgun')
            if name in pickup_by_class and not current_weapon(name)), None)
        if not target_class:
            raise RuntimeError('No unowned original non-Lancer pickup in owned arena')
        pickup = pickup_by_class[target_class]

        def owned_target():
            return current_weapon(target_class)

        receipt['game_pid'] = identity['pid']
        receipt['pickup_class'] = target_class
        receipt['weapon_before'] = bool(owned_target())
        if bridge.export('PlaygroundHandAnimationProbeStart') != 0:
            raise RuntimeError('Read-only hand animation observer did not arm')
        receipt['observer_armed'] = True

        # The original arena places this pickup near the equipment platform.
        # Use the original SetLocation and ServerUse paths; a request is not
        # recorded as a successful pickup unless inventory changes.
        location_field = property_of(pickup, 'Location')
        if (resolver.class_name(resolver.identity(location_field['address'])) != 'StructProperty' or
                resolver.u32(location_field['address'] + 0x74) != 12):
            raise RuntimeError('Original pickup Location declaration changed')
        px, py, pz = struct.unpack(
            '<fff', resolver.read(pickup + location_field['offset'], 12))
        receipt['pickup_location'] = [px, py, pz]
        receipt['set_location_result'] = bridge.call(
            pawn, 'SetLocation', NewLocation=struct.pack('<fff', px, py, pz + 45.0))
        time.sleep(.18)
        receipt['nearby_pickup_matches'] = bridge.call(pawn, 'FindNearbyPickup') == pickup
        weapon = owned_target()
        receipt['weapon_before_server_use'] = bool(weapon)
        if not weapon:
            receipt['server_use_requested'] = True
            bridge.call(pc, 'ServerUse')
            receipt['pickup_special_after_server_use'] = special_move()
            try:
                weapon = wait_for(owned_target, 3.0,
                                  'Original ServerUse did not acquire target weapon')
                receipt['server_use_acquired_weapon'] = True
            except TimeoutError:
                receipt['server_use_acquired_weapon'] = False
                # GiveTo is an original inventory operation but does not
                # establish the ServerUse pickup animation boundary.
                bridge.call(pickup, 'GiveTo', P=pawn)
                weapon = wait_for(owned_target, 3.0,
                                  'Original GiveTo did not acquire target weapon')
                receipt['melee_equip_fallback'] = 'original_GiveTo_without_pickup_animation'
        else:
            receipt['server_use_requested'] = False
            receipt['server_use_acquired_weapon'] = False
            receipt['pickup_special_after_server_use'] = special_move()

        if (receipt['pickup_special_after_server_use'] in (42, 53) or
                special_move() in (42, 53)):
            wait_for(lambda: special_move() == 0, 7,
                     'Original pickup special move did not exit')
            receipt['pickup_special_exited'] = True
            time.sleep(.3)  # Let the original body stance slot fade out.
        else:
            receipt['pickup_special_exited'] = False

        bridge.call(inventory, 'SetCurrentWeapon', DesiredWeapon=weapon)
        wait_for(lambda: pointer(pawn, 'Weapon') == weapon, 8,
                 'Original target weapon equip did not finish')
        time.sleep(1.0)
        receipt['start_melee_requested'] = True
        bridge.call(pawn, 'StartMeleeAttack')
        time.sleep(1.4)
        receipt['stop_melee_requested'] = True
        bridge.call(pawn, 'StopMeleeAttack', bForcedByDamage=False,
                    bForceStopFire=False)
        time.sleep(1.0)
        receipt['observer_stop_requested'] = bridge.export('PlaygroundHandAnimationProbeStop')
        def observer_finished():
            status = bridge.export('PlaygroundHandAnimationProbeStatus')
            return status if (status & 0xffff) in (3, 4, 5) else False

        receipt['observer_status'] = wait_for(
            observer_finished,
            8, 'Read-only hand animation observer did not stop')
    finally:
        if bridge:
            try:
                receipt['shutdown'] = bridge.export('PlaygroundShutdown')
            except BaseException:
                receipt['shutdown_error'] = traceback.format_exc()
            bridge.close()

    if receipt.get('shutdown') != 0:
        raise RuntimeError('Private Playground shutdown was not clean')
    log = args.dll.with_name(f'playground-{identity["pid"]}.log')
    raw = log.read_bytes()
    decoded = raw.decode('utf-8-sig')
    receipt['log'] = str(log)
    receipt['log_sha256'] = hashlib.sha256(raw).hexdigest()
    receipt['terminal'] = validate_log(decoded, receipt['observer_status'] & 0xffff)
    receipt['slot_capture_complete'] = True
    receipt['observer_log_lines'] = [line for line in decoded.splitlines()
                                     if line.startswith('hand_animation_probe_')][-12:]
    receipt['passed'] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--owned-json', required=True, type=Path)
    parser.add_argument('--dll', required=True, type=Path)
    parser.add_argument('--dll-sha256', required=True)
    parser.add_argument('--runtime-root', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'analysis').resolve()):
        raise ValueError('Hand animation receipt must remain in worktree analysis/')
    receipt = {'schema': 1, 'passed': False,
               'source': 'owned retail, default-off read-only hand animation observer',
               'probe_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    try:
        run(args, receipt)
    except BaseException:
        receipt['error'] = traceback.format_exc()
        raise
    finally:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
        (args.owned_json.parent / 'stop').write_text('stop', encoding='ascii')


if __name__ == '__main__':
    main()
