"""Observe original pickup and melee hand ownership in an owned retail arena.

The external test owner launches and closes the behind-windows game. This
runner invokes only original game methods and a default-off read-only observer;
every attempted action is recorded separately from an observed animation.
"""
import argparse
import ctypes
from ctypes import wintypes
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
CALL_TERMINAL_FIELDS = {
    'callbacks', 'accepted', 'completed', 'frame_rejected', 'mask', 'writes',
}
CALL_EVENT_FIELDS = {
    'seq', 'call', 'parent', 'depth', 'kind', 'phase', 'source', 'function',
    'outer', 'object', 'function_ptr', 'native_index', 'ms', 'pickup_request',
    'melee_request', 'pickup_episode', 'melee_episode', 'ownership_proved',
    'writes', 'caller_frame', 'caller_known', 'caller_node', 'caller_object',
    'caller_code', 'caller_locals', 'caller_previous', 'caller_script',
    'caller_script_size', 'param_transport',
}
CALL_FUNCTION_RVA = 0x1d6f30
# Retained 32-byte entry certificate in tools/native-capture/ui_call_boundary.h.
# Each opt-in run compares live bytes before loading and after shutting down.
CALL_FUNCTION_ENTRY = bytes.fromhex(
    '405553565741544155415641574881ec38050000488d6c2430c5f829b5f00400')
PROCESS_EVENT_RVA = 0x1d8dd8
# The existing ProcessEvent loader certificate in tools/playground/runtime.py.
PROCESS_EVENT_ENTRY = bytes.fromhex(
    '405541544155415641574881eca0000000488d6c243048899da00000004889b5a800'
    '00004889bdb0000000488b0536268f014833c5488945604d8be0488bda4c')


def fields_of(line, label):
    fields = {}
    try:
        for item in line.split()[1:]:
            key, value = item.split('=', 1)
            if not key or not value or key in fields:
                raise ValueError('empty or duplicate field')
            fields[key] = value
    except ValueError as error:
        raise RuntimeError(f'Invalid {label} fields') from error
    return fields


def validate_call_boundary_log(lines, terminal):
    summaries = [(index, fields_of(line, 'CallFunction terminal'))
                 for index, line in enumerate(lines)
                 if line.startswith('hand_animation_call_probe_end ')]
    if len(summaries) != 1:
        raise RuntimeError('Expected exactly one CallFunction terminal row')
    summary_index, summary = summaries[0]
    if set(summary) != CALL_TERMINAL_FIELDS:
        raise RuntimeError('CallFunction terminal schema changed')
    try:
        summary = {name: int(value, 16 if name == 'mask' else 10)
                   for name, value in summary.items()}
    except ValueError as error:
        raise RuntimeError('Invalid CallFunction terminal counts') from error
    if any(value < 0 for value in summary.values()):
        raise RuntimeError('Invalid CallFunction terminal counts')

    pairs = {}
    sources = {}
    for line in lines:
        if not line.startswith('hand_animation_event '):
            continue
        event = fields_of(line, 'hand animation event')
        source = event.get('source')
        if source not in ('hook', 'bridge_invoke', 'call_function'):
            raise RuntimeError('Unknown hand animation event source')
        try:
            call = int(event['call'])
        except (KeyError, ValueError) as error:
            raise RuntimeError('Invalid hand animation call ID') from error
        if call <= 0 or (call in sources and sources[call] != source):
            raise RuntimeError('Hand animation call ID reused across sources')
        sources[call] = source
        if source != 'call_function':
            continue
        if set(event) != CALL_EVENT_FIELDS:
            raise RuntimeError('CallFunction event schema changed; parameters are not decoded')
        try:
            for name in ('seq', 'call', 'parent', 'depth', 'kind', 'native_index',
                         'ms', 'pickup_request', 'melee_request',
                         'pickup_episode', 'melee_episode', 'ownership_proved',
                         'writes', 'caller_known', 'caller_script_size'):
                event[name] = int(event[name])
            for name in ('caller_frame', 'caller_node', 'caller_object',
                         'caller_code', 'caller_locals', 'caller_previous',
                         'caller_script'):
                event[name] = int(event[name], 16)
        except ValueError as error:
            raise RuntimeError('Invalid CallFunction event counts') from error
        if (any(event[name] < 0 for name in event if isinstance(event[name], int)) or
                event['seq'] == 0 or not 0 <= event['kind'] < 19 or
                event['depth'] >= 16 or event['ownership_proved'] or event['writes'] or
                event['caller_known'] != 1 or not event['caller_frame'] or
                event['caller_frame'] & 7 or not event['caller_node'] or
                not event['caller_object'] or not event['caller_script'] or
                not 0 < event['caller_script_size'] <= 131072 or
                not event['caller_script'] <= event['caller_code'] <=
                    event['caller_script'] + event['caller_script_size'] or
                event['param_transport'] != 'caller_frame_unconsumed'):
            raise RuntimeError('CallFunction event is not a bounded read-only observation')
        pair = pairs.setdefault(call, {})
        phase = event['phase']
        if phase not in ('pre', 'post') or phase in pair or (phase == 'post' and not pair):
            raise RuntimeError('CallFunction pre/post pair is missing or duplicated')
        pair[phase] = event

    mask = 0
    for pair in pairs.values():
        if set(pair) != {'pre', 'post'}:
            raise RuntimeError('CallFunction pre/post pair is missing or duplicated')
        pre, post = pair['pre'], pair['post']
        stable = CALL_EVENT_FIELDS - {
            'seq', 'phase', 'ms', 'pickup_episode', 'melee_episode', 'caller_code',
        }
        if (pre['seq'] >= post['seq'] or
                any(pre[name] != post[name] for name in stable)):
            raise RuntimeError('CallFunction pre/post identity or kind changed')
        mask |= 1 << pre['kind']
    if (not pairs or not mask & (1 << 17) or summary['accepted'] != len(pairs) or
            summary['completed'] != len(pairs) or summary['mask'] != mask or
            summary['callbacks'] < summary['accepted'] + summary['frame_rejected'] or
            summary['writes'] or len(pairs) > terminal['calls']):
        raise RuntimeError('CallFunction summary or original slot_play kind17 evidence incomplete')

    restored = [(index, fields_of(line, 'CallFunction hook restoration'))
                for index, line in enumerate(lines)
                if line.startswith('hand_animation_call_hook_restored ')]
    terminal_index = next(index for index, line in enumerate(lines)
                          if line.startswith('hand_animation_probe_end '))
    if (len(restored) != 1 or restored[0][1] != {'bytes': '32', 'matched': '1'} or
            restored[0][0] <= max(summary_index, terminal_index)):
        raise RuntimeError('CallFunction hook restoration was not confirmed after sampling')
    return summary


def validate_log(log, expected_status, require_call_boundary=False):
    lines = log.splitlines()
    terminal_lines = [line for line in lines
                      if line.startswith('hand_animation_probe_end ')]
    if len(terminal_lines) != 1:
        raise RuntimeError('Expected exactly one hand animation terminal row')
    terminal = fields_of(terminal_lines[0], 'hand animation terminal')
    if set(terminal) != TERMINAL_FIELDS:
        raise RuntimeError('Hand animation terminal schema changed')
    for name in TERMINAL_FIELDS - {'reason', 'gap'}:
        terminal[name] = int(terminal[name], 16 if name.endswith('_mask') else 10)
    call_summary = validate_call_boundary_log(lines, terminal) if require_call_boundary else None
    if (terminal['status'] != expected_status or expected_status not in (3, 4) or
            terminal['samples'] < 1 or
            terminal['snapshots'] < terminal['samples'] or
            terminal['event_lines'] != sum(line.startswith('hand_animation_event ')
                                           for line in lines) or
            terminal['hook_callbacks'] + (call_summary['callbacks'] if call_summary else 0)
            < terminal['calls'] or
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


def decline_owned_private_settings_prompt(pid):
    """Dismiss only the exact prior-launch prompt in this owned game process.

    The test's private profile must retain its prepared settings. BM_CLICK does
    not activate, move, resize or reorder the user's windows.
    """
    user32 = ctypes.WinDLL('user32', use_last_error=True)
    callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user32.EnumWindows.argtypes = [callback, wintypes.LPARAM]
    user32.EnumChildWindows.argtypes = [wintypes.HWND, callback, wintypes.LPARAM]
    user32.GetWindowThreadProcessId.argtypes = [wintypes.HWND,
                                                 ctypes.POINTER(wintypes.DWORD)]
    user32.GetWindowTextLengthW.argtypes = [wintypes.HWND]
    user32.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user32.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user32.GetDlgCtrlID.argtypes = [wintypes.HWND]
    user32.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT,
                                            wintypes.WPARAM, wintypes.LPARAM,
                                            wintypes.UINT, wintypes.UINT,
                                            ctypes.POINTER(ctypes.c_size_t)]

    def title(hwnd):
        size = user32.GetWindowTextLengthW(hwnd)
        buffer = ctypes.create_unicode_buffer(size + 1)
        user32.GetWindowTextW(hwnd, buffer, len(buffer))
        return buffer.value

    def class_name(hwnd):
        buffer = ctypes.create_unicode_buffer(128)
        user32.GetClassNameW(hwnd, buffer, len(buffer))
        return buffer.value

    matches = []

    def top(hwnd, _):
        owner = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value != pid or class_name(hwnd) != '#32770' or title(hwnd) != 'Message':
            return True
        children = []

        def child(candidate, _):
            children.append((candidate, class_name(candidate), title(candidate),
                             user32.GetDlgCtrlID(candidate)))
            return True

        user32.EnumChildWindows(hwnd, callback(child), 0)
        if any(kind == 'Static' and caption ==
               'Last launch did not complete successfully. Revert to default settings?'
               for _, kind, caption, _ in children):
            matches.extend(candidate for candidate, kind, _, control_id in children
                           if kind == 'Button' and control_id == 7)  # IDNO
        return True

    user32.EnumWindows(callback(top), 0)
    if not matches:
        return False
    if len(matches) != 1:
        raise RuntimeError('Ambiguous owned private-settings prompt')
    result = ctypes.c_size_t()
    if not user32.SendMessageTimeoutW(matches[0], 0xF5, 0, 0, 2, 2000,
                                      ctypes.byref(result)):  # BM_CLICK
        raise ctypes.WinError(ctypes.get_last_error())
    return True


def verify_entry_certificate(resolver, receipt, phase, name, rva, expected):
    address = resolver.base + rva
    raw = resolver.read(address, len(expected))
    observation = {
        'module_base': hex(resolver.base), 'rva': hex(rva),
        'address': hex(address), 'bytes': len(raw), 'entry_hex': raw.hex(),
        'matched': raw == expected,
    }
    receipt.setdefault(f'{name}_entry', {})[phase] = observation
    if not observation['matched']:
        raise RuntimeError(f'{name} entry differs from original {len(expected)}-byte certificate {phase}')


def verify_call_function_entry(resolver, receipt, phase):
    verify_entry_certificate(resolver, receipt, phase, 'call_function',
                             CALL_FUNCTION_RVA, CALL_FUNCTION_ENTRY)


def verify_process_event_entry(resolver, receipt, phase):
    verify_entry_certificate(resolver, receipt, phase, 'process_event',
                             PROCESS_EVENT_RVA, PROCESS_EVENT_ENTRY)


def verify_preload_call_function_entry(identity, runtime_root, receipt):
    """Use an independent read-only process before Bridge can load its DLL."""
    from runtime import Resolver, WindowsControl

    control = WindowsControl()
    profile_path = runtime_root.resolve() / 'tools/package-inspector/reloaded-profile.json'
    resolver = Resolver(identity['pid'], json.loads(profile_path.read_text(encoding='utf-8')))
    try:
        if control.identity(identity['pid']) != identity:
            raise RuntimeError('Playground process identity changed before CallFunction check')
        resolver.prepare()
        verify_call_function_entry(resolver, receipt, 'before_load')
        if control.identity(identity['pid']) != identity:
            raise RuntimeError('Playground process identity changed during CallFunction check')
        return True
    finally:
        resolver.p.close()


def shutdown_bridge(bridge, receipt, require_call_boundary=False):
    try:
        try:
            receipt['shutdown'] = bridge.export('PlaygroundShutdown')
        except BaseException:
            receipt['shutdown_error'] = traceback.format_exc()
        if require_call_boundary:
            for name, verify_entry in (('call_function', verify_call_function_entry),
                                       ('process_event', verify_process_event_entry)):
                try:
                    bridge.verify()
                    verify_entry(bridge.r, receipt, 'after_shutdown')
                    bridge.verify()
                except BaseException:
                    receipt[f'{name}_restore_error'] = traceback.format_exc()
    finally:
        bridge.close()


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
    require_call_boundary = getattr(args, 'require_call_boundary', False)
    try:
        if require_call_boundary:
            def check_before_load():
                try:
                    return verify_preload_call_function_entry(identity, args.runtime_root, receipt)
                except RuntimeError as error:
                    if str(error) not in ('Invalid name array header',
                                          'Invalid object array header'):
                        raise
                    return False

            wait_for(check_before_load, 90, 'Original registries unavailable before CallFunction check')

        def attach():
            nonlocal bridge
            try:
                bridge = Bridge(identity, args.dll,
                                expected_sha256=args.dll_sha256.lower())
                return True
            except RuntimeError as error:
                if str(error) not in ('Invalid name array header',
                                      'Invalid object array header'):
                    raise
                return False

        wait_for(attach, 90, 'Original object registries did not become ready')
        def enable():
            bridge.verify()
            if decline_owned_private_settings_prompt(identity['pid']):
                receipt['private_settings_restore_declined'] = True
            return bridge.export('PlaygroundEnable') == 0

        wait_for(enable, 60, 'Playground menu viewport unavailable')
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
            shutdown_bridge(bridge, receipt, require_call_boundary)

    if receipt.get('shutdown') != 0:
        raise RuntimeError('Private Playground shutdown was not clean')
    if require_call_boundary:
        for name in ('call_function', 'process_event'):
            if (receipt.get(f'{name}_restore_error') or
                    not receipt.get(f'{name}_entry', {}).get('after_shutdown', {}).get('matched')):
                raise RuntimeError(f'{name} live entry was not restored after PlaygroundShutdown')
    log = args.dll.with_name(f'playground-{identity["pid"]}.log')
    raw = log.read_bytes()
    decoded = raw.decode('utf-8-sig')
    receipt['log'] = str(log)
    receipt['log_sha256'] = hashlib.sha256(raw).hexdigest()
    receipt['terminal'] = validate_log(decoded, receipt['observer_status'] & 0xffff,
                                       require_call_boundary)
    if require_call_boundary:
        receipt['call_boundary_terminal'] = validate_call_boundary_log(
            decoded.splitlines(), receipt['terminal'])
        receipt['call_boundary_limit'] = (
            'Original CallFunction entry/return and slot_play kind17 observed; '
            'VM parameters are not decoded; animation ownership is not proved')
    receipt['slot_capture_complete'] = True
    receipt['observer_log_lines'] = [line for line in decoded.splitlines()
                                     if line.startswith(('hand_animation_probe_',
                                                         'hand_animation_call_'))][-12:]
    receipt['passed'] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--owned-json', required=True, type=Path)
    parser.add_argument('--dll', required=True, type=Path)
    parser.add_argument('--dll-sha256', required=True)
    parser.add_argument('--runtime-root', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--require-call-boundary', action='store_true',
                        help='Require original CallFunction entry/return pairs, slot_play kind17, '
                             'and live entry-byte restoration; does not prove animation ownership')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'analysis').resolve()):
        raise ValueError('Hand animation receipt must remain in worktree analysis/')
    receipt = {'schema': 1, 'passed': False,
               'source': 'owned retail, default-off read-only hand animation observer',
               'probe_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               'dll_sha256': args.dll_sha256.lower(),
               'require_call_boundary': args.require_call_boundary}
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
