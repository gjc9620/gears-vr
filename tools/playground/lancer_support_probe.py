"""Collect bounded original Lancer support, socket, or reload evidence in an owned range.

After the original GiveTo/SetCurrentWeapon path, a private game-thread callback
captures either the pawn bones/unchanged gun matrix or the original reload's
magazine/control state after PlayerTick. Output stays in ignored analysis/.
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


def parse_reload_visual_row(line):
    fields = dict(item.split('=', 1) for item in line.split()[1:])
    expected = {'sample', 'qpc', 'ms', 'world', 'reload_start', 'reloading',
                'ammo_used', 'spare', 'mag_hidden', 'mag_pawn', 'control',
                'target', 'scale'}
    if set(fields) != expected or any(fields[key] not in ('0', '1') for key in
                                      ('reloading', 'mag_hidden', 'mag_pawn')):
        raise ValueError('Unexpected original Lancer reload visual row')
    floats = {key: float(fields[key]) for key in ('world', 'reload_start',
                                                 'control', 'target', 'scale')}
    if not all(math.isfinite(value) for value in floats.values()):
        raise ValueError('Nonfinite original reload visual value')
    return {key: int(fields[key]) for key in ('sample', 'qpc', 'ms', 'ammo_used', 'spare')} | {
        key: fields[key] == '1' for key in ('reloading', 'mag_hidden', 'mag_pawn')} | floats


def parse_magazine_pose_row(line):
    fields = dict(item.split('=', 1) for item in line.split()[1:])
    if set(fields) != {'sample', 'qpc', 'gun', 'pawn', 'left', 'magazine'}:
        raise ValueError('Unexpected held Lancer magazine pose row')
    return {'sample': int(fields['sample']), 'qpc': int(fields['qpc']),
            **{key: matrix(fields[key]) for key in ('gun', 'pawn', 'left', 'magazine')}}


def parse_socket_row(line):
    fields = dict(item.split('=', 1) for item in line.split()[1:])
    reload = line.startswith('lancer_socket_reload_sample ')
    expected = {'sample', 'qpc', 'gun', 'world', 'rotation'}
    if reload:
        expected |= {'time', 'start', 'reloading'}
    if set(fields) not in (expected, expected | {'bone'}):
        raise ValueError('Unexpected original Lancer socket row')
    world = [float(item) for item in fields['world'].split(',')]
    rotation = [int(item) for item in fields['rotation'].split(',')]
    if len(world) != 3 or len(rotation) != 3 or not all(math.isfinite(x) for x in world):
        raise ValueError('Invalid original Lancer socket transform')
    row = {'sample': int(fields['sample']), 'qpc': int(fields['qpc']),
           'gun': matrix(fields['gun']), 'world': world, 'rotation': rotation}
    if 'bone' in fields:
        row['bone'] = matrix(fields['bone'])
    if reload:
        if fields['reloading'] not in ('0', '1'):
            raise ValueError('Invalid original Lancer reload state')
        row.update(time=float(fields['time']), start=float(fields['start']),
                   reloading=fields['reloading'] == '1')
        if not all(math.isfinite(row[key]) for key in ('time', 'start')):
            raise ValueError('Nonfinite original Lancer reload time')
    return row


def parse_fixed_feed_row(line):
    fields = dict(item.split('=', 1) for item in line.split()[1:])
    if set(fields) != {'weapon', 'mesh', 'point', 'outward'}:
        raise ValueError('Unexpected fixed Lancer feed row')
    point = [float(value) for value in fields['point'].split(',')]
    outward = [float(value) for value in fields['outward'].split(',')]
    if (len(point) != 3 or len(outward) != 3 or
            not all(math.isfinite(value) for value in point + outward) or
            not .999 < math.dist(outward, [0, 0, 0]) < 1.001):
        raise ValueError('Invalid fixed Lancer feed pose')
    return {'weapon': fields['weapon'], 'mesh': fields['mesh'],
            'point': point, 'outward': outward}


def original_state_function(resolver, weapon, state_name, function_name):
    owner = resolver.identity(weapon)['cls']
    seen = set()
    while owner and owner not in seen:
        seen.add(owner)
        for state in resolver.fields(owner):
            if state['name'] != state_name or resolver.class_name(state) != 'State':
                continue
            for function in resolver.fields(state['address']):
                if (function['name'] == function_name and
                        resolver.class_name(function) == 'Function' and
                        struct.unpack('<H', resolver.read(function['address'] + 0x106, 2))[0] == 0):
                    return function
        owner = resolver.u64(owner + 0x60)
    raise RuntimeError(f'Original state function {state_name}.{function_name} missing')


def call_exact_state_function(bridge, weapon, function):
    # Bridge.call already invokes the selected UFunction on its validated game
    # thread. Replace only this one lookup in this single-use test harness;
    # never change the game object, reflected function, or global dispatcher.
    lookup = bridge.function
    def exact(owner, name):
        if name != function['name']:
            raise RuntimeError('Exact state function lookup mismatch')
        return function
    try:
        bridge.function = exact
        bridge.call(weapon, function['name'])
    finally:
        bridge.function = lookup


def sample_owned_game(args, receipt):
    sys.path.insert(0, str(args.runtime_root.resolve() / 'tools/playground'))
    from runtime import Bridge

    if args.launcher_session:
        session = json.loads(args.launcher_session.read_text(encoding='utf-8'))
        playground = session.get('playground', {})
        phase = session.get('phase')
        report_path = Path(session.get('session_report', ''))
        report = json.loads(report_path.read_text(encoding='utf-8')) if (
            phase == 'waiting_for_level' and report_path.is_file()) else {}
        running = phase == 'session_owned' or (
            phase == 'waiting_for_level' and report.get('phase') == 'running' and
            report.get('game_identity') == session.get('game_identity'))
        if (not running or
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
                    initialized = bridge.export('PlaygroundInitialize')
                    if initialized != 0:
                        raise RuntimeError(f'Owned Playground could not initialize: {initialized}')
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

        def arena_with_pickups():
            current = status()
            return current if current.get('ready') and current.get('pickups', 0) > 0 else None

        ready = wait_for(arena_with_pickups,
                         120, 'Playground arena/pickups not ready')
        if ready.get('failures'):
            raise RuntimeError('Playground arena setup failed')
        resolver = bridge.r
        # attach_only prepared the object registry before TestEnter. Refresh
        # after pickups exist so the scan includes arena-spawned objects.
        resolver.prepare()

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
        if args.mode == 'socket':
            if bridge.export('PlaygroundLancerMagazineSocketProbeEnable') != 0:
                raise RuntimeError('Private Lancer same-tick socket probe refused to start')

            def socket_probe_done():
                value = bridge.export('PlaygroundLancerMagazineSocketProbeStatus')
                state, count = value >> 16, value & 0xffff
                if state == 4:
                    raise RuntimeError(f'Original Lancer socket probe failed after {count} samples')
                return state == 3 and count == 16

            wait_for(socket_probe_done, 8,
                     'Same-tick original Lancer socket sampling did not finish')
        elif args.mode == 'support':
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
        else:
            def read_int(obj, field):
                meta = property_of(obj, field)
                if meta['type'] != 'IntProperty':
                    raise RuntimeError(f'{field} is not an original integer property')
                return struct.unpack('<i', resolver.read(obj + meta['offset'], 4))[0]

            used = read_int(weapon, 'AmmoUsedCount')
            spare = read_int(weapon, 'SpareAmmoCount')
            receipt.update(original_ammo_used=used, original_spare=spare)
            if used == 0:
                bridge.call(weapon, 'SetAmmoUsedCount', NewValue=1)
                if read_int(weapon, 'AmmoUsedCount') != 1:
                    raise RuntimeError('Original test-arena ammo seed did not stick')
                receipt['seeded_ammo_used'] = 1
            if not bridge.call(weapon, 'CanReload'):
                raise RuntimeError('Original Lancer cannot reload in owned arena')
            if args.mode == 'reload_visual':
                if bridge.export('PlaygroundLancerReloadVisualProbeEnable') != 0:
                    raise RuntimeError('Private Lancer reload visual probe refused to start')
                time.sleep(.12)  # One idle sample before invoking the original reload.
                if bridge.export('PlaygroundLancerReloadVisualProbeStatus') >> 16 == 4:
                    raise RuntimeError('Original reload visual probe failed before ForceReload')
            elif args.mode == 'socket_reload':
                if bridge.export('PlaygroundLancerMagazineSocketReloadProbeEnable') != 0:
                    raise RuntimeError('Private Lancer socket reload probe refused to start')
                time.sleep(.12)
                if bridge.export('PlaygroundLancerMagazineSocketProbeStatus') >> 16 == 4:
                    raise RuntimeError('Original socket reload probe failed before ForceReload')
            if args.mode == 'reload_adapter':
                if bridge.export('PlaygroundLancerReloadHoldStartTest') != 0:
                    raise RuntimeError('Private game-thread Lancer reload start queue rejected')

                def adapter_started():
                    state = bridge.export('PlaygroundLancerReloadHoldStatus')
                    if state > 1:
                        raise RuntimeError(f'Private Lancer reload start rejected: {state}')
                    return state == 1

                wait_for(adapter_started, 3, 'Private Lancer reload adapter did not start')
                receipt['adapter_start_status'] = 1
            else:
                bridge.call(weapon, 'ForceReload')
            wait_for(lambda: bridge.call(weapon, 'IsReloading'), 2,
                     'Original Lancer did not enter Reloading after ForceReload')
            if args.mode == 'reload_visual':
                def reload_probe_done():
                    value = bridge.export('PlaygroundLancerReloadVisualProbeStatus')
                    state, count = value >> 16, value & 0xffff
                    if state == 4:
                        raise RuntimeError(f'Original reload visual probe failed after {count} samples')
                    return state == 3

                wait_for(reload_probe_done, 8, 'Original reload visual timeline did not finish')
            elif args.mode == 'socket_reload':
                def socket_reload_done():
                    value = bridge.export('PlaygroundLancerMagazineSocketProbeStatus')
                    state, count = value >> 16, value & 0xffff
                    if state == 4:
                        raise RuntimeError(f'Original Lancer socket reload failed after {count} samples')
                    return state == 3 and count >= 3

                wait_for(socket_reload_done, 8, 'Original Lancer socket reload did not finish')
            else:
                fn = bridge.function(resolver.identity(weapon)['cls'], 'EndOfReloadTimer')
                name_offset = int(resolver.profile['object_name'])
                timer_index = resolver.u32(fn['address'] + name_offset)
                if resolver.name(timer_index) != 'EndOfReloadTimer':
                    raise RuntimeError('Original reload timer name identity mismatch')
                timer_name = (timer_index, 0)
                receipt['timer_active_before_clear'] = bool(bridge.call(
                    weapon, 'IsTimerActive', inTimerFunc=timer_name))
                if not receipt['timer_active_before_clear']:
                    raise RuntimeError('Original EndOfReloadTimer was not active')
                if args.mode == 'reload_hold':
                    bridge.call(weapon, 'ClearTimer', inTimerFunc=timer_name)
                    receipt['timer_active_after_clear'] = bool(bridge.call(
                        weapon, 'IsTimerActive', inTimerFunc=timer_name))
                    if receipt['timer_active_after_clear']:
                        raise RuntimeError('Original EndOfReloadTimer did not clear')
                else:
                    receipt['fallback_timer_active'] = receipt['timer_active_before_clear']
                def read_float(obj, field):
                    meta = property_of(obj, field)
                    if meta['type'] != 'FloatProperty':
                        raise RuntimeError(f'{field} is not an original float property')
                    return struct.unpack('<f', resolver.read(obj + meta['offset'], 4))[0]
                reload_start = read_float(weapon, 'ReloadStartTime')
                wait_for(lambda: read_float(world, 'TimeSeconds') - reload_start >= 3.2,
                         6, 'Original game clock did not pass the normal reload duration')
                receipt['held_elapsed_game_seconds'] = read_float(world, 'TimeSeconds') - reload_start
                receipt['held_reloading'] = bool(bridge.call(weapon, 'IsReloading'))
                receipt['held_ammo_used'] = read_int(weapon, 'AmmoUsedCount')
                if not receipt['held_reloading'] or receipt['held_ammo_used'] != 1:
                    raise RuntimeError('Original reload auto-committed after timer clearance')
                if args.mode == 'reload_hold':
                    state_function = original_state_function(
                        resolver, weapon, 'Reloading', 'EndOfReloadTimer')
                    receipt['state_function'] = {
                        'address': hex(state_function['address']),
                        'outer': hex(state_function['outer']), 'name': state_function['name']}
                    call_exact_state_function(bridge, weapon, state_function)
                else:
                    if bridge.export('PlaygroundLancerReloadHoldCompleteTest') != 0:
                        raise RuntimeError('Private game-thread Lancer completion queue rejected')
                    wait_for(lambda: bridge.export('PlaygroundLancerReloadHoldStatus') == 6,
                             2, 'Private Lancer completion did not finish')
                    receipt['adapter_finish_status'] = 6
                wait_for(lambda: not bridge.call(weapon, 'IsReloading') and
                         read_int(weapon, 'AmmoUsedCount') == 0,
                         2, 'Original state completion did not commit ammo and return Active')
                receipt['finished_ammo_used'] = read_int(weapon, 'AmmoUsedCount')
                receipt['finished_reloading'] = bool(bridge.call(weapon, 'IsReloading'))
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
    if args.mode == 'socket':
        begins = [line for line in lines if line.startswith('lancer_socket_begin ')]
        rows = [parse_socket_row(line) for line in lines
                if line.startswith('lancer_socket_sample ')]
        if (len(begins) != 1 or len(rows) != 16 or
                [row['sample'] for row in rows] != list(range(16)) or
                any(rows[index]['qpc'] <= rows[index - 1]['qpc']
                    for index in range(1, 16))):
            raise RuntimeError('Original Lancer socket evidence incomplete')
        receipt.update(socket_begin=begins[0], socket_samples=rows)
    elif args.mode == 'support':
        beginnings = [line for line in lines if line.startswith('lancer_support_probe_begin ')]
        poses = [parse_pose_row(line) for line in lines if line.startswith('lancer_support_pose ')]
        visuals = [parse_visual_row(line) for line in lines
                   if line.startswith('lancer_support_visual ')]
        if (len(beginnings) != 1 or len(poses) != 24 or len(visuals) != 1 or
                [row['sample'] for row in poses] != list(range(24))):
            raise RuntimeError('Bounded same-tick original pose rows missing from private log')
        receipt.update(game_thread_probe=beginnings[0], samples=poses,
                       original_visual=visuals[0])
    elif args.mode == 'socket_reload':
        beginnings = [line for line in lines if line.startswith('lancer_socket_reload_begin ')]
        rows = [parse_socket_row(line) for line in lines
                if line.startswith('lancer_socket_reload_sample ')]
        first_reload = next((index for index, row in enumerate(rows)
                             if row['reloading']), None)
        if (len(beginnings) != 1 or len(rows) < 3 or
                [row['sample'] for row in rows] != list(range(len(rows))) or
                any(rows[index]['qpc'] <= rows[index - 1]['qpc']
                    for index in range(1, len(rows))) or
                first_reload is None or
                not any(not row['reloading'] for row in rows[first_reload + 1:])):
            raise RuntimeError('Original Lancer socket reload sequence incomplete')
        receipt.update(socket_reload_begin=beginnings[0], socket_reload_samples=rows)
    elif args.mode == 'reload_visual':
        beginnings = [line for line in lines if line.startswith('lancer_reload_visual_begin ')]
        rows = [parse_reload_visual_row(line) for line in lines
                if line.startswith('lancer_reload_visual sample=')]
        poses = [parse_magazine_pose_row(line) for line in lines
                 if line.startswith('lancer_reload_magazine_pose ')]
        first_reload = next((index for index, row in enumerate(rows) if row['reloading']), None)
        if (len(beginnings) != 1 or len(rows) < 3 or
                [row['sample'] for row in rows] != list(range(len(rows))) or
                any(rows[index]['qpc'] <= rows[index - 1]['qpc'] for index in range(1, len(rows))) or
                first_reload is None or not any(not row['reloading'] for row in rows[first_reload + 1:])):
            raise RuntimeError('Bounded original Lancer reload sequence missing from private log')
        if (len(poses) < 2 or len({pose['sample'] for pose in poses}) != len(poses) or
                any(pose['sample'] >= len(rows) or not rows[pose['sample']]['mag_pawn'] or
                    pose['qpc'] != rows[pose['sample']]['qpc'] for pose in poses)):
            raise RuntimeError('Original hand-held magazine pose sequence missing')
        receipt.update(game_thread_probe=beginnings[0], reload_visual_timeline=rows,
                       held_magazine_poses=poses)
    else:
        if (receipt.get('timer_active_before_clear') is not True or
                (args.mode == 'reload_hold' and
                 receipt.get('timer_active_after_clear') is not False) or
                (args.mode == 'reload_adapter' and
                 (receipt.get('fallback_timer_active') is not True or
                  receipt.get('adapter_start_status') != 1 or
                  receipt.get('adapter_finish_status') != 6)) or
                receipt.get('held_reloading') is not True or
                receipt.get('held_ammo_used') != 1 or
                receipt.get('finished_ammo_used') != 0 or
                receipt.get('finished_reloading') is not False):
            raise RuntimeError('Original held reload and explicit completion evidence incomplete')
        if args.mode == 'reload_adapter':
            anchors = [parse_fixed_feed_row(line) for line in lines
                       if line.startswith('lancer_fixed_feed_anchor ')]
            if (len(anchors) < 2 or
                    len({row['weapon'] for row in anchors}) != 1 or
                    len({row['mesh'] for row in anchors}) != 1):
                raise RuntimeError('Fixed Lancer feed anchor did not remain on one gun mesh')
            receipt['fixed_feed_anchors'] = anchors
    receipt.update(game_thread_log=str(log_path),
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
    parser.add_argument('--mode', choices=('socket', 'socket_reload', 'support', 'reload_visual', 'reload_hold',
                                          'reload_adapter'), default='support')
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'analysis').resolve()):
        raise ValueError('Retail game evidence must remain under analysis/')
    receipt = {'schema': 1, 'passed': False, 'source': 'owned retail runtime',
               'mode': args.mode,
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
