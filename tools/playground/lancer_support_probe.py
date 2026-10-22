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
LANCER_EDGE_LOCAL = ((0, 28, 5), (0, 32, 9), (0, 55, 9), (0, 69, 8),
                     (0, 74, 6), (0, 75, 2), (0, 74, -1), (0, 69, -5),
                     (0, 62, -7), (0, 49, -6.5), (0, 35, -5), (0, 29, -2))


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


def verify_lancer_edge_rows(rows):
    maximum = 0.0
    for row in rows:
        gun = row['gun']
        edge = row['visible_edge_candidate']
        if len(edge) != len(LANCER_EDGE_LOCAL):
            raise RuntimeError('Visible edge sample count changed')
        for local, actual in zip(LANCER_EDGE_LOCAL, edge):
            expected = (gun[12] + gun[0] * local[0] + gun[4] * local[1] + gun[8] * local[2],
                        gun[13] + gun[1] * local[0] + gun[5] * local[1] + gun[9] * local[2],
                        gun[14] + gun[2] * local[0] + gun[6] * local[1] + gun[10] * local[2])
            maximum = max(maximum, *(abs(a - b) for a, b in zip(actual, expected)))
    if maximum > .01:
        raise RuntimeError('Visible edge does not match same-tick gun matrix')
    return maximum


def parse_pose_row(line):
    fields = dict(item.split('=', 1) for item in line.split()[1:])
    if set(fields) != {'sample', 'qpc', 'pawn', 'gun', 'ik_left', 'hand_left', 'hand_right'}:
        raise ValueError('Unexpected game-thread Lancer pose row')
    return {'sample': int(fields['sample']), 'qpc': int(fields['qpc']),
            **{key: matrix(fields[key]) for key in ('pawn', 'gun', 'ik_left', 'hand_left', 'hand_right')}}


def parse_mesh_trial_row(line):
    fields = dict(item.split('=', 1) for item in line.split()[1:])
    if set(fields) != {'index', 'expected_x', 'observed_x', 'error',
                       'world_y', 'world_z', 'translation_x',
                       'absolute_translation', 'rotation'}:
        raise ValueError('Unexpected original Lancer mesh trial row')
    rotations = [int(value) for value in fields['rotation'].split(',')]
    values = {key: float(fields[key]) for key in
              ('expected_x', 'observed_x', 'error', 'world_y', 'world_z',
               'translation_x')}
    if (len(rotations) != 3 or
            fields['absolute_translation'] not in ('0', '1') or
            not all(math.isfinite(value) for value in values.values())):
        raise ValueError('Invalid original Lancer mesh trial row')
    return {'index': int(fields['index']), **values,
            'absolute_translation': fields['absolute_translation'] == '1',
            'rotation': rotations}


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


def parse_magazine_follow_row(line):
    fields = dict(item.split('=', 1) for item in line.split()[1:])
    if set(fields) != {'weapon', 'component', 'left', 'world', 'matrix',
                       'attached', 'ammo_control', 'target', 'reattached'}:
        raise ValueError('Unexpected Lancer independent magazine follow row')
    flags = {key: fields[key] == '1' for key in
             ('matrix', 'attached', 'ammo_control', 'reattached')}
    if any(fields[key] not in ('0', '1') for key in flags):
        raise ValueError('Invalid Lancer magazine follow flag')
    vectors = {key: [float(value) for value in fields[key].split(',')]
               for key in ('left', 'world')}
    if any(len(value) != 3 or not all(math.isfinite(item) for item in value)
           for value in vectors.values()):
        raise ValueError('Invalid Lancer magazine follow vector')
    target = float(fields['target'])
    if not math.isfinite(target):
        raise ValueError('Invalid Lancer magazine control target')
    return {'weapon': fields['weapon'], 'component': fields['component'],
            **vectors, **flags, 'target': target}


def parse_insertion_candidate_row(line):
    fields = dict(item.split('=', 1) for item in line.split()[1:])
    if set(fields) != {'result', 'sequence', 'depth', 'lateral', 'alignment', 'clock'}:
        raise ValueError('Unexpected Lancer insertion candidate row')
    result = int(fields['result'])
    sequence = int(fields['sequence'])
    values = {key: float(fields[key]) for key in
              ('depth', 'lateral', 'alignment', 'clock')}
    if result not in (1, 2) or sequence <= 0 or not all(
            math.isfinite(value) for value in values.values()):
        raise ValueError('Invalid Lancer insertion candidate')
    return {'result': result, 'sequence': sequence, **values}


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
        if args.mode == 'api':
            magazine = pointer(weapon, 'MagazineMesh')
            if class_name(magazine) != 'StaticMeshComponent':
                raise RuntimeError('Original Lancer independent magazine component missing')

            def reflected_function(obj, function_name):
                function = bridge.function(resolver.identity(obj)['cls'], function_name)
                address = function['address']
                parameter_bytes = struct.unpack('<H', resolver.read(address + 0x106, 2))[0]
                parameters = []
                for field in resolver.fields(address):
                    flags = resolver.u64(field['address'] + 0x78)
                    if not flags & 0x80:
                        continue
                    parameter = {'name': field['name'], 'type': resolver.class_name(field),
                                 'offset': resolver.u32(field['address'] + 0x94),
                                 'size': resolver.u32(field['address'] + 0x74),
                                 'flags': hex(flags)}
                    if parameter['type'] == 'BoolProperty':
                        parameter['mask'] = hex(resolver.u32(field['address'] + 0xd0))
                    parameters.append(parameter)
                if not all(item['offset'] + item['size'] <= parameter_bytes
                           for item in parameters):
                    raise RuntimeError(f'{function_name} parameter exceeds reflected size')
                return {'address': hex(address), 'parameter_bytes': parameter_bytes,
                        'parameters': parameters}

            receipt['magazine_component'] = resolver.identity(magazine)
            receipt['magazine_asset'] = resolver.identity(pointer(magazine, 'StaticMesh'))
            receipt['original_component_api'] = {
                'SetPawnAmmoAttachment': reflected_function(weapon, 'SetPawnAmmoAttachment'),
                'SetWeaponAmmoBoneDisplay': reflected_function(weapon, 'SetWeaponAmmoBoneDisplay'),
                'SetAbsolute': reflected_function(magazine, 'SetAbsolute'),
                'SetTranslation': reflected_function(magazine, 'SetTranslation'),
                'SetRotation': reflected_function(magazine, 'SetRotation'),
                'SetHidden': reflected_function(magazine, 'SetHidden'),
                'AttachComponentToSocket': reflected_function(pawn_mesh, 'AttachComponentToSocket'),
                'DetachComponent': reflected_function(pawn_mesh, 'DetachComponent'),
                'IsComponentAttached': reflected_function(pawn_mesh, 'IsComponentAttached'),
            }
        elif args.mode == 'chainsaw_api':
            def signature(object_address, function_name):
                function = bridge.function(resolver.identity(object_address)['cls'],
                                           function_name)
                address = function['address']
                parameter_bytes = struct.unpack('<H', resolver.read(address + 0x106, 2))[0]
                if parameter_bytes > 128:
                    raise RuntimeError(f'{function_name} parameter budget changed')
                parameters = []
                for field in resolver.fields(address):
                    flags = resolver.u64(field['address'] + 0x78)
                    if not flags & 0x80:
                        continue
                    item = {'name': field['name'], 'type': resolver.class_name(field),
                            'offset': resolver.u32(field['address'] + 0x94),
                            'size': resolver.u32(field['address'] + 0x74),
                            'return': bool(flags & 0x400)}
                    if item['offset'] + item['size'] > parameter_bytes:
                        raise RuntimeError(f'{function_name} parameter outside reflected size')
                    parameters.append(item)
                return {'path': function['name'], 'parameter_bytes': parameter_bytes,
                        'parameters': parameters}

            receipt['original_chainsaw_api'] = {
                'AcquiredVictim': signature(weapon, 'AcquiredVictim'),
                'IsTargetValidForSpecialMeleeAttack': signature(
                    weapon, 'IsTargetValidForSpecialMeleeAttack'),
                'MeleeAttackImpact': signature(weapon, 'MeleeAttackImpact'),
            }
        elif args.mode == 'chainsaw_target_eligibility':
            # This mode never calls AcquiredVictim. It establishes the original
            # controller/target-validation boundary in one owned retail arena.
            def input_parameters(obj, function_name):
                function = bridge.function(resolver.identity(obj)['cls'], function_name)
                fields = []
                for field in resolver.fields(function['address']):
                    flags = resolver.u64(field['address'] + 0x78)
                    if flags & 0x80 and not flags & 0x400:
                        fields.append((field['name'], resolver.class_name(field)))
                return fields

            def original_validator(obj, function_name, target, with_revival=False):
                fields = input_parameters(obj, function_name)
                if len(fields) != (2 if with_revival else 1) or \
                        fields[0][1] != 'ObjectProperty' or \
                        (with_revival and fields[1][1] != 'BoolProperty'):
                    raise RuntimeError(f'Original {function_name} signature changed: {fields}')
                values = {fields[0][0]: target}
                if with_revival:
                    values[fields[1][0]] = True
                return bool(bridge.call(obj, function_name, **values))

            dummy = 0
            locust_class = 0
            for obj in resolver.object_pointers:
                if not obj:
                    continue
                try:
                    identity_row = resolver.identity(obj)
                    if (identity_row['name'] == 'Pawn_GeistLocust' and
                            class_name(obj) == 'Class'):
                        locust_class = obj
                    if (class_name(obj) == 'Pawn_GeistLocust' and
                            pointer(obj, 'WorldInfo') == world and
                            not pointer(obj, 'Controller')):
                        dummy = obj
                except (RuntimeError, TypeError, KeyError):
                    continue
            if not dummy or not locust_class:
                raise RuntimeError('Controllerless arena target or original class absent')
            receipt['controllerless_original_validator'] = original_validator(
                weapon, 'IsTargetValidForSpecialMeleeAttack', dummy)
            if receipt['controllerless_original_validator']:
                raise RuntimeError('Original unexpectedly accepted controllerless target')
            location_field = property_of(pawn, 'Location')
            if (location_field['type'] != 'StructProperty' or
                    resolver.u32(location_field['address'] + 0x74) != 12):
                raise RuntimeError('Original pawn Location layout changed')
            location = struct.unpack('<3f', resolver.read(
                pawn + location_field['offset'], 12))
            if not all(math.isfinite(v) for v in location):
                raise RuntimeError('Original pawn Location is nonfinite')
            position = (location[0] + 120., location[1], location[2])
            target = bridge.call(pc, 'Spawn', SpawnClass=locust_class,
                                 SpawnLocation=struct.pack('<3f', *position),
                                 SpawnRotation=struct.pack('<3i', 0, 32768, 0),
                                 bNoCollisionFail=True)
            if not target:
                raise RuntimeError('Original Pawn_GeistLocust spawn failed')
            try:
                resolver.prepare()  # refresh registry after a game-thread spawn
                if class_name(target) != 'Pawn_GeistLocust' or \
                        pointer(target, 'WorldInfo') != world:
                    raise RuntimeError('Spawned target class/world changed')
                if not pointer(target, 'Controller'):
                    bridge.call(target, 'SpawnDefaultController')
                resolver.prepare()
                controller = wait_for(lambda: pointer(target, 'Controller'), 3,
                                      'Original SpawnDefaultController did not possess target')
                ancestry = []
                cls = resolver.identity(controller)['cls']
                for _ in range(20):
                    if not cls:
                        break
                    ancestry.append(resolver.identity(cls)['name'])
                    cls = resolver.u64(cls + 0x60)
                if ('WarAI_Drone' not in ancestry or
                        pointer(controller, 'Pawn') != target or
                        pointer(controller, 'MyWP') != target or
                        pointer(controller, 'WorldInfo') != world):
                    raise RuntimeError('Original AI controller reciprocal identity failed')
                receipt['controlled_target'] = {
                    'pawn': hex(target), 'controller': hex(controller),
                    'controller_ancestry': ancestry, 'position': position,
                    'health': struct.unpack('<i', resolver.read(
                        target + property_of(target, 'Health')['offset'], 4))[0],
                    'pawn_validator': original_validator(
                        pawn, 'IsValidMeleeTarget', target, True),
                    'weapon_validator': original_validator(
                        weapon, 'IsTargetValidForSpecialMeleeAttack', target),
                }
            finally:
                try:
                    bridge.call(target, 'Destroy')
                except BaseException:
                    receipt['target_cleanup_error'] = traceback.format_exc()
        elif args.mode in ('chainsaw_bones', 'chainsaw_bones_held'):
            held = args.mode == 'chainsaw_bones_held'
            if held:
                if bridge.export('PlaygroundLancerSawHoldStartTest') != 0:
                    raise RuntimeError('Private original saw hold rejected')
            try:
                if held:
                    def saw_hold_started():
                        return bridge.export('PlaygroundLancerSawHoldStatus') == 0x10002
                    wait_for(saw_hold_started, 5, 'Original saw hold did not start')
                    running = property_of(weapon, 'bChainSawRunning')
                    if running['type'] != 'BoolProperty':
                        raise RuntimeError('Original saw running field changed')
                    mask = resolver.u32(running['address'] + 0xd0)
                    if not mask:
                        raise RuntimeError('Original saw running bit mask missing')
                    wait_for(lambda: bool(resolver.u32(weapon + running['offset']) & mask),
                             5, 'Original blade did not enter running state')
                start = ('PlaygroundLancerBladeProbeStartHeld' if held else
                         'PlaygroundLancerBladeProbeStart')
                if bridge.export(start) != 0:
                    raise RuntimeError('Private same-tick blade probe rejected')
                def blade_probe_done():
                    result = bridge.export('PlaygroundLancerBladeProbeStatus')
                    status, count = result >> 16, result & 0xffff
                    if status == 4:
                        raise RuntimeError(f'Original blade probe failed after {count} samples')
                    return status == 3 and count == 16
                wait_for(blade_probe_done, 10,
                         'Original same-tick blade probe did not finish')
            finally:
                if held:
                    if bridge.export('PlaygroundLancerSawHoldStopTest') != 0:
                        raise RuntimeError('Private saw hold stop rejected')
                    wait_for(lambda: bridge.export('PlaygroundLancerSawHoldStatus') == 3,
                             5, 'Original saw hold did not release after blade probe')
        elif args.mode == 'component_motion':
            magazine = pointer(weapon, 'MagazineMesh')
            if (class_name(magazine) != 'StaticMeshComponent' or
                    resolver.identity(pointer(magazine, 'StaticMesh'))['name'] !=
                    'Assault_Rifle_Magazine'):
                raise RuntimeError('Original Lancer independent magazine identity changed')

            def component_struct(field, format):
                meta = property_of(magazine, field)
                if meta['type'] != 'StructProperty':
                    raise RuntimeError(f'Original {field} is not a struct')
                return struct.unpack(format, resolver.read(magazine + meta['offset'],
                                                            struct.calcsize(format)))

            before_matrix = component_struct('LocalToWorld', '<16f')
            before_translation = component_struct('Translation', '<3f')
            before_rotation = component_struct('Rotation', '<3i')
            before_absolute = tuple(bool(resolver.u32(magazine + property_of(
                magazine, field)['offset']) & resolver.u32(property_of(
                magazine, field)['address'] + 0xd0))
                for field in ('AbsoluteTranslation', 'AbsoluteRotation', 'AbsoluteScale'))
            before_attached = bool(bridge.call(pawn_mesh, 'IsComponentAttached', Component=magazine))
            if before_attached:
                raise RuntimeError('Idle Lancer magazine unexpectedly attached')
            try:
                bridge.call(weapon, 'SetPawnAmmoAttachment', bAttach=True)
                wait_for(lambda: bridge.call(pawn_mesh, 'IsComponentAttached', Component=magazine),
                         2, 'Original Lancer magazine failed to attach')
                attached_matrix = component_struct('LocalToWorld', '<16f')
                bridge.call(weapon, 'SetWeaponAmmoBoneDisplay', bShow=False)
                bridge.call(magazine, 'SetAbsolute', NewAbsoluteTranslation=True,
                            NewAbsoluteRotation=True, NewAbsoluteScale=False)
                target = (attached_matrix[12] + 5, attached_matrix[13], attached_matrix[14])
                bridge.call(magazine, 'SetTranslation', NewTranslation=struct.pack('<3f', *target))
                wait_for(lambda: abs(component_struct('LocalToWorld', '<16f')[12] - target[0]) < .1,
                         2, 'Original Lancer magazine did not follow world translation')
                moved_matrix = component_struct('LocalToWorld', '<16f')
                receipt['component_motion'] = {
                    'before_attached': before_attached,
                    'attached_after_original_call': True,
                    'before_world_origin': before_matrix[12:15],
                    'attached_world_origin': attached_matrix[12:15],
                    'requested_world_origin': target,
                    'moved_world_origin': moved_matrix[12:15],
                }
            finally:
                bridge.call(weapon, 'SetPawnAmmoAttachment', bAttach=False)
                bridge.call(weapon, 'SetWeaponAmmoBoneDisplay', bShow=True)
                bridge.call(magazine, 'SetAbsolute',
                            NewAbsoluteTranslation=before_absolute[0],
                            NewAbsoluteRotation=before_absolute[1],
                            NewAbsoluteScale=before_absolute[2])
                bridge.call(magazine, 'SetTranslation',
                            NewTranslation=struct.pack('<3f', *before_translation))
                bridge.call(magazine, 'SetRotation',
                            NewRotation=struct.pack('<3i', *before_rotation))
            receipt['component_motion']['detached_after_restore'] = not bridge.call(
                pawn_mesh, 'IsComponentAttached', Component=magazine)
        elif args.mode == 'socket':
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
        elif args.mode == 'mesh_follow_trial':
            # The VR equipment model deliberately starts neutral after an
            # ordinary game pickup. Simulate the explicit right-hand grab at
            # the Lancer's observed original slot before testing mesh follow.
            if bridge.export('PlaygroundEquipmentSwitchTest', 2) != 0:
                raise RuntimeError('Private held-Lancer slot tap was rejected')
            wait_for(lambda: bridge.export('PlaygroundEquipmentSwitchStatus') == 5,
                     3, 'Private held-Lancer slot tap did not latch the weapon')
            if bridge.export('PlaygroundWeaponMeshFollowTrialStart') != 0:
                raise RuntimeError('Private game-thread Lancer mesh trial refused to start')

            def mesh_trial_done():
                value = bridge.export('PlaygroundWeaponMeshFollowTrialStatus')
                state, count = value >> 16, value & 0xffff
                if state == 4:
                    raise RuntimeError(f'Original Lancer mesh trial failed after {count} samples')
                return state == 3 and count == 12

            wait_for(mesh_trial_done, 12,
                     'Original Lancer mesh trial did not move and restore')
            receipt['mesh_follow_trial_status'] = bridge.export(
                'PlaygroundWeaponMeshFollowStatus')
        elif args.mode in ('chainsaw_semantics', 'chainsaw_hold_adapter',
                           'chainsaw_hold_shutdown', 'chainsaw_gesture_adapter',
                           'chainsaw_xr_no_pose'):
            def fname(value):
                for index, address in enumerate(resolver.name_pointers):
                    if not address:
                        continue
                    try:
                        if resolver.name(index) == value:
                            return index, 0
                    except (RuntimeError, UnicodeError):
                        continue
                raise RuntimeError(f'Original name absent: {value}')

            def original_bool(obj, field):
                meta = property_of(obj, field)
                if meta['type'] != 'BoolProperty':
                    raise RuntimeError(f'{field} is not an original bool')
                mask = resolver.u32(meta['address'] + 0xd0)
                if not mask:
                    raise RuntimeError(f'{field} has no original bool mask')
                return bool(resolver.u32(obj + meta['offset']) & mask)

            def original_integer(obj, field):
                meta = property_of(obj, field)
                if meta['type'] == 'ByteProperty':
                    return resolver.read(obj + meta['offset'], 1)[0]
                if meta['type'] == 'IntProperty':
                    return struct.unpack('<i', resolver.read(
                        obj + meta['offset'], 4))[0]
                raise RuntimeError(f'{field} is not an original integer')

            def snapshot():
                return {
                    'holding_b': original_bool(pc, 'bHoldingButton_B'),
                    'wants_melee': original_bool(pawn, 'bWantsToMelee'),
                    # Bridge.call resolves the class method, not a current
                    # state's method. Record it, but use the original state
                    # fields for the test assertion.
                    'class_is_meleeing': bool(bridge.call(weapon, 'IsMeleeing')),
                    'doing_melee': original_bool(pawn, 'bDoingMeleeAttack'),
                    'current_fire_mode': original_integer(weapon, 'CurrentFireMode'),
                    'special_move': original_integer(pawn, 'SpecialMove'),
                    'blade_running': original_bool(weapon, 'bChainSawRunning'),
                    'target_timer': bool(bridge.call(
                        weapon, 'IsTimerActive',
                        inTimerFunc=fname('CheckForMeleeTarget'))),
                    'victim': hex(pointer(weapon, 'SpecialMeleeAttackVictim')),
                }

            if not bridge.call(weapon, 'CanDoSpecialMeleeAttack'):
                raise RuntimeError('Original Lancer melee is not available in owned arena')
            receipt['before_chainsaw'] = snapshot()
            if (receipt['before_chainsaw']['holding_b'] or
                    receipt['before_chainsaw']['doing_melee']):
                raise RuntimeError('Original Lancer was already in melee state')
            if args.mode == 'chainsaw_xr_no_pose':
                if bridge.export('PlaygroundEquipmentSwitchTest', 2) != 0:
                    raise RuntimeError('Private held-Lancer slot tap was rejected')
                wait_for(lambda: bridge.export('PlaygroundEquipmentSwitchStatus') == 5,
                         3, 'VR equipment model did not latch Lancer')
                if bridge.export('PlaygroundLancerSawXrPullArm') != 0:
                    raise RuntimeError('Private XR pull Arm was rejected')
                wait_for(lambda: bridge.export('PlaygroundLancerSawXrPullStatus') == 2,
                         3, 'Private XR pull trial did not arm')
                time.sleep(.45)
                receipt['xr_no_pose_armed'] = {
                    'status': bridge.export('PlaygroundLancerSawXrPullStatus'),
                    **snapshot(),
                }
                if bridge.export('PlaygroundLancerSawXrPullStop') != 0:
                    raise RuntimeError('Private XR pull Stop was rejected')
                wait_for(lambda: bridge.export('PlaygroundLancerSawXrPullStatus') == 8,
                         3, 'Private XR pull trial did not stop')
                receipt['xr_no_pose_stopped'] = {
                    'status': bridge.export('PlaygroundLancerSawXrPullStatus'),
                    **snapshot(),
                }
            elif args.mode in ('chainsaw_hold_adapter', 'chainsaw_hold_shutdown',
                             'chainsaw_gesture_adapter'):
                gesture = args.mode == 'chainsaw_gesture_adapter'
                if gesture:
                    if bridge.export('PlaygroundEquipmentSwitchTest', 2) != 0:
                        raise RuntimeError('Private right-hand Lancer grab was rejected')
                    wait_for(lambda: bridge.export('PlaygroundEquipmentSwitchStatus') == 5,
                             3, 'VR equipment model did not latch Lancer')
                    if bridge.export('PlaygroundLancerSawGestureTrialStart') != 0:
                        raise RuntimeError('Private synthetic saw pull was rejected')
                elif bridge.export('PlaygroundLancerSawHoldStartTest') != 0:
                    raise RuntimeError('Private game-thread Lancer B hold was not queued')
                try:
                    if gesture:
                        def gesture_running():
                            value = bridge.export('PlaygroundLancerSawGestureTrialStatus')
                            if value in (5, 6):
                                raise RuntimeError(f'Synthetic pull failed with status {value}')
                            return value == 4
                        wait_for(gesture_running, 5,
                                 'Synthetic Lancer hand pull did not start original saw')
                    def held_status():
                        value = bridge.export('PlaygroundLancerSawHoldStatus')
                        return value if value == 0x10002 else None
                    wait_for(held_status, 5, 'Private Lancer B hold did not become active')
                    def held_original_state():
                        row = snapshot()
                        return row if row['doing_melee'] and row['blade_running'] else None
                    wait_for(held_original_state, 5,
                             'Original Lancer did not enter its held melee state')
                    samples = []
                    started = time.monotonic()
                    while time.monotonic() - started < 1.2:
                        samples.append({'elapsed': round(time.monotonic() - started, 3),
                                        'adapter_status': bridge.export(
                                            'PlaygroundLancerSawHoldStatus'), **snapshot()})
                        time.sleep(.06)
                    receipt['held_chainsaw_samples'] = samples
                finally:
                    if args.mode != 'chainsaw_hold_shutdown':
                        if bridge.export('PlaygroundLancerSawHoldStopTest') != 0:
                            receipt['stop_request_failed'] = True
                        else:
                            def stopped_status():
                                value = bridge.export('PlaygroundLancerSawHoldStatus')
                                return value if value == 3 else None
                            wait_for(stopped_status, 5,
                                     'Private Lancer B hold did not release')
            else:
                button = fname('B')
                pressed = False
                try:
                    bridge.call(pc, 'ButtonPress', ButtonName=button)
                    pressed = True
                    samples = []
                    started = time.monotonic()
                    while time.monotonic() - started < .85:
                        samples.append({'elapsed': round(time.monotonic() - started, 3),
                                        **snapshot()})
                        time.sleep(.06)
                    receipt['held_chainsaw_samples'] = samples
                    if not any(row['target_timer'] for row in samples):
                        raise RuntimeError('Original Lancer target timer never appeared')
                    bridge.call(weapon, 'ClearTimer',
                                inTimerFunc=fname('CheckForMeleeTarget'))
                    receipt['cleared_chainsaw_timer'] = snapshot()
                    time.sleep(.45)
                    receipt['held_after_timer_clear'] = snapshot()
                finally:
                    if pressed:
                        bridge.call(pc, 'ButtonRelease', ButtonName=button)
            if args.mode != 'chainsaw_hold_shutdown':
                receipt['released_chainsaw'] = snapshot()
                wait_for(lambda: not original_bool(pawn, 'bDoingMeleeAttack') and
                         original_integer(weapon, 'CurrentFireMode') == 0,
                         5, 'Original Lancer did not leave melee after B release')
                receipt['settled_chainsaw'] = snapshot()
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
            if args.mode in ('reload_adapter', 'follow_adapter', 'insert_adapter'):
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
            if args.mode in ('follow_adapter', 'insert_adapter'):
                start = ('PlaygroundLancerMagazineInsertStartTest'
                         if args.mode == 'insert_adapter' else
                         'PlaygroundLancerMagazineFollowStartTest')
                if bridge.export(start) != 0:
                    raise RuntimeError('Private synthetic left-hand motion queue rejected')
                expected = 6 if args.mode == 'insert_adapter' else 2
                wait_for(lambda: bridge.export('PlaygroundLancerMagazineFollowStatus') == expected,
                         3, 'Original Lancer magazine did not reach expected synthetic stage')
                wait_for(lambda: bridge.export('PlaygroundLancerMagazineFollowStatus') == 3,
                         4, 'Original Lancer magazine did not restore on left-grip release')
                magazine = pointer(weapon, 'MagazineMesh')
                receipt['magazine_detached_after_release'] = not bool(bridge.call(
                    pawn_mesh, 'IsComponentAttached', Component=magazine))
                receipt['magazine_follow_finish_status'] = 3
                if args.mode == 'insert_adapter':
                    receipt['insertion_stage_observed'] = True
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
                if (args.mode == 'chainsaw_hold_shutdown' and
                        receipt.get('held_chainsaw_samples')):
                    time.sleep(.15)
                    receipt['shutdown_chainsaw'] = {
                        'holding_b': original_bool(pc, 'bHoldingButton_B'),
                        'wants_melee': original_bool(pawn, 'bWantsToMelee'),
                        'doing_melee': original_bool(pawn, 'bDoingMeleeAttack'),
                        'blade_running': original_bool(weapon, 'bChainSawRunning'),
                        'current_fire_mode': original_integer(weapon,
                                                              'CurrentFireMode'),
                    }
            except BaseException:
                receipt['shutdown_error'] = traceback.format_exc()
            bridge.close()
    if receipt.get('shutdown') != 0:
        raise RuntimeError('Playground shutdown was not clean')
    log_path = args.dll.with_name(f'playground-{identity["pid"]}.log')
    lines = log_path.read_text(encoding='utf-8-sig').splitlines()
    if args.mode == 'api':
        api = receipt['original_component_api']
        for name in ('SetPawnAmmoAttachment', 'SetWeaponAmmoBoneDisplay'):
            if len(api[name]['parameters']) != 1 or api[name]['parameters'][0]['type'] != 'BoolProperty':
                raise RuntimeError(f'Original {name} bool signature differs')
        if len(api['SetAbsolute']['parameters']) != 3 or any(
                item['type'] != 'BoolProperty' for item in api['SetAbsolute']['parameters']):
            raise RuntimeError('Original SetAbsolute signature differs')
    elif args.mode == 'chainsaw_api':
        api = receipt['original_chainsaw_api']
        if any(not entry['path'] or entry['parameter_bytes'] > 128
               for entry in api.values()):
            raise RuntimeError('Original Lancer chainsaw function metadata incomplete')
    elif args.mode == 'chainsaw_target_eligibility':
        if (receipt['controllerless_original_validator'] or
                'controlled_target' not in receipt or
                'WarAI_Drone' not in receipt['controlled_target']['controller_ancestry'] or
                not receipt['controlled_target']['pawn_validator'] or
                not receipt['controlled_target']['weapon_validator'] or
                receipt.get('target_cleanup_error')):
            raise RuntimeError('Original Lancer target eligibility boundary incomplete')
        receipt['source_limit'] = ('original controller and validator game-thread calls; '
                                   'no blade contact or paired execution')
    elif args.mode in ('chainsaw_bones', 'chainsaw_bones_held'):
        begins = [line for line in lines if line.startswith('lancer_blade_probe_begin ')]
        rows = []
        for line in lines:
            if not line.startswith('lancer_blade_pose '):
                continue
            fields = dict(item.split('=', 1) for item in line.split()[1:])
            edge_keys = {f'edge{index}' for index in range(12)}
            if set(fields) != {'sample', 'qpc', 'gun', 'root', 'tip'} | edge_keys:
                raise RuntimeError('Unexpected original blade-bone row')
            edge = [tuple(float(value) for value in fields[f'edge{index}'].split(','))
                    for index in range(12)]
            if any(len(point) != 3 or not all(math.isfinite(value) for value in point)
                   for point in edge):
                raise RuntimeError('Invalid transformed visible-edge candidate')
            rows.append({'sample': int(fields['sample']), 'qpc': int(fields['qpc']),
                         **{key: matrix(fields[key]) for key in
                            ('gun', 'root', 'tip')}, 'visible_edge_candidate': edge})
        if (len(begins) != 1 or len(rows) != 16 or
                [row['sample'] for row in rows] != list(range(16)) or
                any(rows[index]['qpc'] <= rows[index - 1]['qpc']
                    for index in range(1, len(rows)))):
            raise RuntimeError('Original same-tick Lancer blade-bone sample incomplete')
        edge_projection_error = verify_lancer_edge_rows(rows)
        receipt.update(blade_bone_begin=begins[0], blade_bone_samples=rows,
                       edge_projection_max_error_raw=edge_projection_error,
                       source_limit=('same game-thread tick, original Lancer; visible edge '
                                     'is a sparse model approximation, no contact target'))
    elif args.mode == 'component_motion':
        motion = receipt['component_motion']
        if (not motion['attached_after_original_call'] or
                not motion['detached_after_restore'] or
                abs(motion['moved_world_origin'][0] - motion['requested_world_origin'][0]) >= .1):
            raise RuntimeError('Original Lancer independent component motion incomplete')
    elif args.mode == 'socket':
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
    elif args.mode == 'mesh_follow_trial':
        rows = [parse_mesh_trial_row(line) for line in lines
                if line.startswith('lancer_mesh_trial_sample ')]
        ends = [line for line in lines if line.startswith('lancer_mesh_trial_end ')]
        if (len(rows) != 12 or [row['index'] for row in rows] != list(range(12)) or
                len(ends) != 1 or 'motion_ok=1 restored=1 status=6' not in ends[0] or
                any(row['error'] > .5 for row in rows) or
                any(not row['absolute_translation'] or
                    abs(row['translation_x'] - row['expected_x']) > .5 for row in rows) or
                rows[4]['observed_x'] - rows[0]['observed_x'] < 15 or
                rows[10]['observed_x'] - rows[0]['observed_x'] > -7 or
                rows[4]['rotation'] == rows[0]['rotation']):
            raise RuntimeError('Original Lancer mesh transform/restoration evidence incomplete')
        receipt.update(mesh_follow_trial_rows=rows, mesh_follow_trial_end=ends[0],
                       source_limit='synthetic hand pose on original game thread; no Quest controller')
    elif args.mode == 'chainsaw_semantics':
        rows = receipt['held_chainsaw_samples']
        if (not rows or not any(row['holding_b'] and row['wants_melee'] for row in rows) or
                not any(row['doing_melee'] and row['current_fire_mode'] == 2
                        for row in rows) or
                not any(row['target_timer'] for row in rows) or
                any(row['victim'] != '0x0' for row in rows) or
                receipt['cleared_chainsaw_timer']['target_timer'] or
                receipt['held_after_timer_clear']['target_timer'] or
                not receipt['held_after_timer_clear']['blade_running'] or
                not receipt['held_after_timer_clear']['doing_melee'] or
                receipt['released_chainsaw']['holding_b'] or
                receipt['released_chainsaw']['wants_melee'] or
                receipt['settled_chainsaw']['doing_melee'] or
                receipt['settled_chainsaw']['current_fire_mode'] != 0):
            raise RuntimeError('Original Lancer B/hold/target-timer semantics incomplete')
    elif args.mode == 'chainsaw_hold_adapter':
        rows = receipt['held_chainsaw_samples']
        if (len(rows) < 8 or any(row['adapter_status'] != 0x10002 or
                                 not row['holding_b'] or not row['doing_melee'] or
                                 not row['blade_running'] or row['target_timer'] or
                                 row['victim'] != '0x0' for row in rows) or
                receipt.get('stop_request_failed') or
                receipt['settled_chainsaw']['holding_b'] or
                receipt['settled_chainsaw']['doing_melee'] or
                receipt['settled_chainsaw']['current_fire_mode'] != 0):
            raise RuntimeError('Private Lancer saw hold/timer/release incomplete')
    elif args.mode == 'chainsaw_xr_no_pose':
        armed = receipt['xr_no_pose_armed']
        stopped = receipt['xr_no_pose_stopped']
        if (armed['status'] != 2 or stopped['status'] != 8 or
                any(row['holding_b'] or row['doing_melee'] or
                    row['blade_running'] or row['target_timer'] or
                    row['victim'] != '0x0' for row in (armed, stopped))):
            raise RuntimeError('Untracked XR pull started original Lancer saw')
    elif args.mode == 'chainsaw_gesture_adapter':
        rows = receipt['held_chainsaw_samples']
        pulls = [line for line in lines if line.startswith('lancer_saw_gesture_trial_pull ')]
        if (len(pulls) != 1 or len(rows) < 8 or
                any(row['adapter_status'] != 0x10002 or not row['holding_b'] or
                    not row['doing_melee'] or not row['blade_running'] or
                    row['target_timer'] or row['victim'] != '0x0' for row in rows) or
                receipt.get('stop_request_failed') or
                receipt['settled_chainsaw']['holding_b'] or
                receipt['settled_chainsaw']['doing_melee'] or
                receipt['settled_chainsaw']['current_fire_mode'] != 0):
            raise RuntimeError('Synthetic pull to original Lancer saw was incomplete')
        receipt['gesture_pull_log'] = pulls[0]
    elif args.mode == 'chainsaw_hold_shutdown':
        rows = receipt['held_chainsaw_samples']
        after = receipt.get('shutdown_chainsaw', {})
        if (len(rows) < 8 or any(row['adapter_status'] != 0x10002 or
                                 not row['holding_b'] or not row['doing_melee'] or
                                 not row['blade_running'] or row['target_timer'] or
                                 row['victim'] != '0x0' for row in rows) or
                not after or any(after.get(key) for key in (
                    'holding_b', 'wants_melee', 'doing_melee', 'blade_running')) or
                after.get('current_fire_mode') != 0):
            raise RuntimeError('Private Lancer saw shutdown did not release original state')
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
                (args.mode in ('reload_adapter', 'follow_adapter', 'insert_adapter') and
                 (receipt.get('fallback_timer_active') is not True or
                  receipt.get('adapter_start_status') != 1 or
                  receipt.get('adapter_finish_status') != 6)) or
                receipt.get('held_reloading') is not True or
                receipt.get('held_ammo_used') != 1 or
                receipt.get('finished_ammo_used') != 0 or
                receipt.get('finished_reloading') is not False):
            raise RuntimeError('Original held reload and explicit completion evidence incomplete')
        if args.mode in ('reload_adapter', 'follow_adapter', 'insert_adapter'):
            anchors = [parse_fixed_feed_row(line) for line in lines
                       if line.startswith('lancer_fixed_feed_anchor ')]
            if (len(anchors) < 2 or
                    len({row['weapon'] for row in anchors}) != 1 or
                    len({row['mesh'] for row in anchors}) != 1):
                raise RuntimeError('Fixed Lancer feed anchor did not remain on one gun mesh')
            receipt['fixed_feed_anchors'] = anchors
        if args.mode == 'follow_adapter':
            followed = [parse_magazine_follow_row(line) for line in lines
                        if line.startswith('lancer_magazine_follow ')]
            inserted = [line for line in lines
                        if line.startswith('lancer_magazine_insertion_candidate result=2 ')]
            # LocalToWorld may still contain the preceding scene update in the
            # first row after SetAbsolute. Subsequent rows must be rigidly
            # locked to the moving test hand, even across original detachment.
            steady = followed[1:]
            offsets = [[row['world'][axis] - row['left'][axis] for axis in range(3)]
                       for row in steady]
            if (receipt.get('magazine_follow_finish_status') != 3 or
                    receipt.get('magazine_detached_after_release') is not True or
                    len(followed) < 3 or
                    len({row['weapon'] for row in followed}) != 1 or
                    len({row['component'] for row in followed}) != 1 or
                    any(not row['matrix'] or not row['attached'] or
                        not row['ammo_control'] or row['target'] < .9 for row in followed) or
                    not any(row['reattached'] for row in followed) or
                    inserted or
                    math.dist(steady[0]['world'], steady[-1]['world']) < 6 or
                    max(math.dist(offset, offsets[0]) for offset in offsets) > .2):
                raise RuntimeError('Original Lancer synthetic hand-follow evidence incomplete')
            receipt['magazine_follow_rows'] = followed
            receipt['unexpected_insertion_candidates'] = len(inserted)
            receipt['steady_world_travel_raw'] = math.dist(
                steady[0]['world'], steady[-1]['world'])
            receipt['maximum_steady_hand_offset_drift_raw'] = max(
                math.dist(offset, offsets[0]) for offset in offsets)
        if args.mode == 'insert_adapter':
            followed = [parse_magazine_follow_row(line) for line in lines
                        if line.startswith('lancer_magazine_follow ')]
            strokes = [parse_insertion_candidate_row(line) for line in lines
                       if line.startswith('lancer_magazine_insertion_candidate ')]
            inserted = [row for row in strokes if row['result'] == 2]
            if (receipt.get('insertion_stage_observed') is not True or
                    receipt.get('magazine_follow_finish_status') != 3 or
                    receipt.get('magazine_detached_after_release') is not True or
                    len(followed) < 2 or len(strokes) < 2 or len(inserted) != 1 or
                    len({row['weapon'] for row in followed}) != 1 or
                    len({row['component'] for row in followed}) != 1 or
                    any(not row['matrix'] or not row['attached'] or
                        not row['ammo_control'] or row['target'] < .9 for row in followed) or
                    any(strokes[index]['sequence'] <= strokes[index - 1]['sequence']
                        for index in range(1, len(strokes))) or
                    strokes[0]['depth'] < 4.5 or inserted[0]['depth'] > 2.5 or
                    inserted[0]['lateral'] > 4 or inserted[0]['alignment'] < .85 or
                    inserted[0]['clock'] < .8):
                raise RuntimeError('Original Lancer aligned insertion trial incomplete')
            receipt['magazine_follow_rows'] = followed
            receipt['insertion_candidate_rows'] = strokes
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
    parser.add_argument('--mode', choices=('api', 'chainsaw_api', 'chainsaw_target_eligibility', 'chainsaw_bones', 'chainsaw_bones_held', 'component_motion', 'socket', 'socket_reload', 'support', 'mesh_follow_trial', 'chainsaw_semantics', 'chainsaw_hold_adapter', 'chainsaw_hold_shutdown', 'chainsaw_gesture_adapter', 'chainsaw_xr_no_pose', 'reload_visual', 'reload_hold',
                                          'reload_adapter', 'follow_adapter', 'insert_adapter'), default='support')
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
