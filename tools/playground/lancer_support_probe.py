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


def parse_reload_bone_pose_row(line):
    fields = dict(item.split('=', 1) for item in line.split()[1:])
    if set(fields) != {'sample', 'qpc', 'gun', 'latch', 'receiver'}:
        raise ValueError('Unexpected original Lancer reload bone row')
    return {'sample': int(fields['sample']), 'qpc': int(fields['qpc']),
            **{key: matrix(fields[key]) for key in ('gun', 'latch', 'receiver')}}


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
        elif args.mode == 'pawn_ik':
            # The original WarPawn caches existing named hand controls. Read
            # their current instance and coordinate-space fields; never call
            # SetSkelControlActive or write an effector in this probe.
            def named_field(obj, field):
                meta = property_of(obj, field)
                if meta['type'] != 'NameProperty':
                    raise RuntimeError(f'{field} name type changed')
                index, number = struct.unpack('<II', resolver.read(obj + meta['offset'], 8))
                return {'text': resolver.name(index), 'number': number}

            def scalar_field(obj, field, kind):
                meta = property_of(obj, field)
                if meta['type'] != kind:
                    raise RuntimeError(f'{field} scalar type changed')
                size = 1 if kind == 'ByteProperty' else 4
                raw = resolver.read(obj + meta['offset'], size)
                if kind == 'ByteProperty':
                    return raw[0]
                if kind == 'FloatProperty':
                    return struct.unpack('<f', raw)[0]
                if kind == 'BoolProperty':
                    mask = resolver.u32(meta['address'] + 0xd0)
                    if not mask or mask & (mask - 1):
                        raise RuntimeError(f'{field} bool mask changed')
                    return bool(struct.unpack('<I', raw)[0] & mask)
                raise RuntimeError(f'{field} scalar kind not supported')

            def struct_field(obj, field, expected):
                meta = property_of(obj, field)
                if meta['type'] != 'StructProperty' or \
                        resolver.u32(meta['address'] + 0x74) != 12:
                    raise RuntimeError(f'{field} original struct changed')
                structure = resolver.struct_type(meta)
                if resolver.identity(structure)['name'] != expected:
                    raise RuntimeError(f'{field} original struct kind changed')
                raw = resolver.read(obj + meta['offset'], 12)
                values = struct.unpack('<fff' if expected == 'Vector' else '<iii', raw)
                if expected == 'Vector' and not all(math.isfinite(v) for v in values):
                    raise RuntimeError(f'{field} nonfinite original vector')
                return list(values)

            names = ('IKCtrl_RightHand', 'IKCtrl_LeftHand',
                     'IKRotCtrl_RightHand', 'IKRotCtrl_LeftHand',
                     'IKBoneCtrl_RightHand', 'IKBoneCtrl_LeftHand',
                     'IKBoneRotCtrl_RightHand', 'IKBoneRotCtrl_LeftHand')
            controls = {}
            for name in names:
                meta = property_of(pawn, name)
                if meta['type'] != 'ObjectProperty':
                    raise RuntimeError(f'{name} cache property changed')
                current = pointer(pawn, name)
                if not current:
                    controls[name] = None
                    continue
                kind = class_name(current)
                wanted = 'SkelControlLimb' if 'RotCtrl' not in name else \
                    'SkelControlSingleBone'
                if kind != wanted:
                    raise RuntimeError(f'{name} current control class changed')
                component = pointer(current, 'SkelComponent')
                row = {'identity': resolver.identity(current),
                       'control_name': named_field(current, 'ControlName'),
                       'skel_component': resolver.identity(component) if component else None,
                       'same_component': component == pawn_mesh,
                       'next_control': hex(pointer(current, 'NextControl')),
                       'control_strength': scalar_field(current, 'ControlStrength', 'FloatProperty'),
                       'strength_target': scalar_field(current, 'StrengthTarget', 'FloatProperty'),
                       'blend_time_to_go': scalar_field(
                           current, 'BlendTimeToGo', 'FloatProperty'),
                       'blend_in': scalar_field(current, 'BlendInTime', 'FloatProperty'),
                       'blend_out': scalar_field(current, 'BlendOutTime', 'FloatProperty'),
                       'strength_from_anim_node': scalar_field(
                           current, 'bSetStrengthFromAnimNode', 'BoolProperty'),
                       'propagate_set_active': scalar_field(
                           current, 'bPropagateSetActive', 'BoolProperty'),
                       'ignore_when_not_rendered': scalar_field(
                           current, 'bIgnoreWhenNotRendered', 'BoolProperty')}
                if kind == 'SkelControlLimb':
                    row['effector_location'] = struct_field(current, 'EffectorLocation', 'Vector')
                    row['effector_space'] = scalar_field(current, 'EffectorLocationSpace', 'ByteProperty')
                    row['effector_space_bone'] = named_field(current, 'EffectorSpaceBoneName')
                    row['joint_target_location'] = struct_field(current, 'JointTargetLocation', 'Vector')
                    row['joint_target_space'] = scalar_field(current, 'JointTargetLocationSpace', 'ByteProperty')
                    row['joint_target_space_bone'] = named_field(
                        current, 'JointTargetSpaceBoneName')
                    row['take_effector_rotation'] = scalar_field(
                        current, 'bTakeRotationFromEffectorSpace', 'BoolProperty')
                    row['maintain_effector_relative_rotation'] = scalar_field(
                        current, 'bMaintainEffectorRelRot', 'BoolProperty')
                else:
                    row['bone_rotation'] = struct_field(current, 'BoneRotation', 'Rotator')
                    row['rotation_space'] = scalar_field(current, 'BoneRotationSpace', 'ByteProperty')
                    row['rotation_space_bone'] = named_field(current, 'RotationSpaceBoneName')
                    row['add_rotation'] = scalar_field(current, 'bAddRotation', 'BoolProperty')
                    row['apply_rotation'] = scalar_field(current, 'bApplyRotation', 'BoolProperty')
                controls[name] = row

            tree = pointer(pawn_mesh, 'Animations')
            if not tree or class_name(tree) != 'AnimTree':
                raise RuntimeError('Current player pawn has no AnimTree')
            lists = property_of(tree, 'SkelControlLists')
            indices = property_of(pawn_mesh, 'SkelControlIndex')
            if lists['type'] != 'ArrayProperty' or indices['type'] != 'ArrayProperty':
                raise RuntimeError('Current hand control arrays changed')
            list_data, list_count, list_capacity = struct.unpack(
                '<Qii', resolver.read(tree + lists['offset'], 16))
            index_data, index_count, index_capacity = struct.unpack(
                '<Qii', resolver.read(pawn_mesh + indices['offset'], 16))
            if (not 0 < list_count <= list_capacity <= 64 or not list_data or
                    not 36 < index_count <= index_capacity <= 512 or not index_data):
                raise RuntimeError('Current hand control array bounds changed')
            inner = resolver.u64(lists['address'] + resolver.profile['struct_property_struct'])
            if not inner or class_name(inner) != 'StructProperty':
                raise RuntimeError('Current hand control-list element changed')
            structure = resolver.struct_type({'address': inner, 'type': 'StructProperty'})
            stride = resolver.u32(structure + 0x88)
            offsets = {}
            for field in resolver.fields(structure):
                if field['name'] in ('BoneName', 'ControlHead'):
                    offsets[field['name']] = resolver.u32(field['address'] + 0x94)
            if (not 16 <= stride <= 64 or set(offsets) != {'BoneName', 'ControlHead'} or
                    max(offsets.values()) + 8 > stride):
                raise RuntimeError('Current hand control-list layout changed')
            list_rows, in_tree = [], set()
            for position in range(list_count):
                base = list_data + position * stride
                bone_name_index, bone_name_number = struct.unpack(
                    '<II', resolver.read(base + offsets['BoneName'], 8))
                current = resolver.u64(base + offsets['ControlHead'])
                chain, seen = [], set()
                while current:
                    if current in seen or len(chain) >= 32:
                        raise RuntimeError('Current hand control chain changed')
                    seen.add(current)
                    in_tree.add(current)
                    chain.append({'identity': resolver.identity(current),
                                  'class': class_name(current),
                                  'name': named_field(current, 'ControlName')})
                    current = pointer(current, 'NextControl')
                list_rows.append({'index': position,
                                  'bone_name': {'text': resolver.name(bone_name_index),
                                                'number': bone_name_number},
                                  'chain': chain})
            index_bytes = list(resolver.read(index_data, index_count))
            ignore = property_of(pawn_mesh, 'bIgnoreControllers')
            if ignore['type'] != 'IntProperty':
                raise RuntimeError('Current pawn controller-ignore field changed')
            receipt['current_pawn_ik'] = {
                'tree': resolver.identity(tree),
                'controls': controls,
                'ignore_controllers': resolver.u32(pawn_mesh + ignore['offset']),
                'ignore_when_not_rendered': scalar_field(
                    pawn_mesh, 'bIgnoreControllersWhenNotRendered', 'BoolProperty'),
                'control_list_count': list_count,
                'control_list_capacity': list_capacity,
                'control_lists': list_rows,
                'cached_in_current_tree': {
                    name: bool(controls[name] and controls[name]['identity']['address'] in in_tree)
                    for name in names},
                'skel_control_index': {'count': index_count, 'capacity': index_capacity,
                                       'left_hand_bone_11': index_bytes[11],
                                       'right_hand_bone_35': index_bytes[35],
                                       'values': index_bytes},
            }
        elif args.mode == 'anim_tree':
            # Read the equipped Lancer's existing animation graph. The saved
            # class declares an AnimTreeTemplate, but only this instance can
            # show which child and control lists are actually active.
            def name_value(address):
                index, number = struct.unpack('<II', resolver.read(address, 8))
                return {'text': resolver.name(index), 'number': number}

            def bitfield(obj, field):
                meta = property_of(obj, field)
                if meta['type'] != 'BoolProperty':
                    raise RuntimeError(f'{field} is not an original bool')
                mask = resolver.u32(meta['address'] + 0xd0)
                if not mask:
                    raise RuntimeError(f'{field} has no original mask')
                return bool(resolver.u32(obj + meta['offset']) & mask)

            def struct_array(obj, field, maximum):
                meta = property_of(obj, field)
                if meta['type'] != 'ArrayProperty':
                    raise RuntimeError(f'{field} is not an original array')
                inner = resolver.u64(meta['address'] + resolver.profile['struct_property_struct'])
                if not inner or class_name(inner) != 'StructProperty':
                    raise RuntimeError(f'{field} has no reflected struct element')
                structure = resolver.struct_type({'address': inner, 'type': 'StructProperty'})
                stride = resolver.u32(structure + 0x88)
                if not 8 <= stride <= 256:
                    raise RuntimeError(f'{field} struct stride is outside budget')
                members = {}
                for member in resolver.fields(structure):
                    kind = class_name(member['address'])
                    if kind not in ('NameProperty', 'ObjectProperty'):
                        continue
                    offset = resolver.u32(member['address'] + 0x94)
                    if offset + 8 > stride:
                        raise RuntimeError(f'{field} member exceeds reflected stride')
                    members[member['name']] = (kind, offset)
                data, count, capacity = struct.unpack('<Qii', resolver.read(obj + meta['offset'], 16))
                if not 0 <= count <= capacity <= maximum or (count and not data):
                    raise RuntimeError(f'{field} array header changed')
                rows = []
                for index in range(count):
                    base = data + index * stride
                    row = {'index': index}
                    for label, (kind, offset) in members.items():
                        address = base + offset
                        if kind == 'NameProperty':
                            row[label] = name_value(address)
                        else:
                            value = resolver.u64(address)
                            row[label] = resolver.identity(value) if value else None
                            row[label + '_address'] = hex(value)
                    rows.append(row)
                return {'structure': resolver.identity(structure), 'stride': stride,
                        'count': count, 'capacity': capacity,
                        'members': sorted(members), 'rows': rows}

            def primitive_array(obj, field, kind, maximum):
                meta = property_of(obj, field)
                if meta['type'] != 'ArrayProperty':
                    raise RuntimeError(f'{field} is not an original array')
                inner = resolver.u64(meta['address'] + resolver.profile['struct_property_struct'])
                if not inner or class_name(inner) != kind:
                    raise RuntimeError(f'{field} element type changed')
                data, count, capacity = struct.unpack('<Qii', resolver.read(
                    obj + meta['offset'], 16))
                if not 0 <= count <= capacity <= maximum or (count and not data):
                    raise RuntimeError(f'{field} array header changed')
                stride = 1 if kind == 'ByteProperty' else 8
                raw = resolver.read(data, count * stride) if count else b''
                if kind == 'ByteProperty':
                    values = list(raw)
                else:
                    values = []
                    for index in range(count):
                        item = struct.unpack_from('<Q', raw, index * stride)[0]
                        values.append(resolver.identity(item) if item else None)
                return {'count': count, 'capacity': capacity, 'values': values}

            def function_info(obj, name):
                function = bridge.function(resolver.identity(obj)['cls'], name)
                size = struct.unpack('<H', resolver.read(function['address'] + 0x106, 2))[0]
                if size > 128:
                    raise RuntimeError(f'{name} exceeds function parameter budget')
                parameters = []
                for field in resolver.fields(function['address']):
                    flags = resolver.u64(field['address'] + 0x78)
                    if flags & 0x80:
                        parameters.append({'name': field['name'],
                                           'type': resolver.class_name(field),
                                           'offset': resolver.u32(field['address'] + 0x94),
                                           'size': resolver.u32(field['address'] + 0x74),
                                           'flags': hex(flags)})
                if any(row['offset'] + row['size'] > size for row in parameters):
                    raise RuntimeError(f'{name} parameter exceeds function size')
                return {'identity': resolver.identity(function['address']),
                        'parameter_bytes': size, 'parameters': parameters}

            template = pointer(weapon_mesh, 'AnimTreeTemplate')
            tree = pointer(weapon_mesh, 'Animations')
            if not template or not tree or class_name(tree) != 'AnimTree':
                raise RuntimeError('Current Lancer does not own an AnimTree instance')
            controls = struct_array(tree, 'SkelControlLists', 64)
            children = struct_array(tree, 'Children', 16)
            chains = []
            for row in controls['rows']:
                current = int(row.get('ControlHead_address', '0'), 16)
                seen = set()
                chain = []
                while current:
                    if current in seen or len(chain) >= 16:
                        raise RuntimeError('Current Lancer control chain is cyclic or oversized')
                    seen.add(current)
                    control_name = property_of(current, 'ControlName')
                    if control_name['type'] != 'NameProperty':
                        raise RuntimeError('Original control name type changed')
                    chain.append({'identity': resolver.identity(current),
                                  'class': class_name(current),
                                  'control_name': name_value(current + control_name['offset']),
                                  'skel_component': resolver.identity(pointer(
                                      current, 'SkelComponent')) if pointer(
                                          current, 'SkelComponent') else None})
                    current = pointer(current, 'NextControl')
                chains.append(chain)
            ignore = property_of(weapon_mesh, 'bIgnoreControllers')
            if ignore['type'] != 'IntProperty':
                raise RuntimeError('Original controller-ignore field changed')
            required = property_of(weapon_mesh, 'bRequiredBonesUpToDate')
            if required['type'] != 'IntProperty':
                raise RuntimeError('Original required-bones flag type changed')
            sequence = int(children['rows'][0]['Anim_address'], 16) if children['count'] else 0
            if not sequence or class_name(sequence) != 'AnimNodeSequence':
                raise RuntimeError('Current Lancer anim child is not a sequence')
            time_field = property_of(sequence, 'CurrentTime')
            rate_field = property_of(sequence, 'Rate')
            name_field = property_of(sequence, 'AnimSeqName')
            if any(field['type'] != 'FloatProperty' for field in (time_field, rate_field)) or \
                    name_field['type'] != 'NameProperty':
                raise RuntimeError('Current sequence metadata changed')
            receipt['current_lancer_anim_tree'] = {
                'template': resolver.identity(template),
                'instance': resolver.identity(tree),
                'same_object_as_template': tree == template,
                'children': children,
                'control_lists': controls,
                'control_chains': chains,
                'ignore_controllers': resolver.u32(weapon_mesh + ignore['offset']),
                'ignore_when_not_rendered': bitfield(
                    weapon_mesh, 'bIgnoreControllersWhenNotRendered'),
                'skel_control_index': primitive_array(
                    weapon_mesh, 'SkelControlIndex', 'ByteProperty', 512),
                'required_bones': primitive_array(
                    weapon_mesh, 'RequiredBones', 'ByteProperty', 512),
                'anim_tick_array': primitive_array(
                    weapon_mesh, 'AnimTickArray', 'ObjectProperty', 64),
                'required_bones_up_to_date': resolver.u32(
                    weapon_mesh + required['offset']),
                'sequence': {'identity': resolver.identity(sequence),
                             'name': name_value(sequence + name_field['offset']),
                             'time': struct.unpack('<f', resolver.read(
                                 sequence + time_field['offset'], 4))[0],
                             'rate': struct.unpack('<f', resolver.read(
                                 sequence + rate_field['offset'], 4))[0],
                             'playing': bitfield(sequence, 'bPlaying')},
                'api': {'SetAnimTreeTemplate': function_info(
                            weapon_mesh, 'SetAnimTreeTemplate'),
                        'UpdateAnimations': function_info(
                            weapon_mesh, 'UpdateAnimations'),
                        'SetSkelControlStrength': function_info(
                            int(controls['rows'][0]['ControlHead_address'], 16),
                            'SetSkelControlStrength')},
            }
        elif args.mode == 'reload_values':
            # The original HUD calls this getter for its reload bar. Invoke
            # the same UFunction on the owned Lancer during an original reload.
            function = bridge.function(resolver.identity(weapon)['cls'],
                                       'GetActiveReloadValues')
            expected = ('ActiveReloadStartTime', 'PreReactionWindowDuration',
                        'SuperSweetSpotDuration', 'SweetSpotDuration')
            fields = {}
            for field in resolver.fields(function['address']):
                flags = resolver.u64(field['address'] + 0x78)
                if not flags & 0x80:
                    continue
                if (field['name'] not in expected or
                        resolver.class_name(field) != 'FloatProperty' or
                        not flags & 0x100 or flags & 0x400 or
                        resolver.u32(field['address'] + 0x74) != 4):
                    raise RuntimeError('Original reload getter signature changed')
                fields[field['name']] = {'offset': resolver.u32(field['address'] + 0x94),
                                         'flags': hex(flags)}
            size = struct.unpack('<H', resolver.read(function['address'] + 0x106, 2))[0]
            if set(fields) != set(expected) or size != 16 or sorted(
                    field['offset'] for field in fields.values()) != [0, 4, 8, 12]:
                raise RuntimeError('Original reload getter outputs are not four floats')

            def scalar(obj, name, kind):
                prop = property_of(obj, name)
                if prop['type'] != kind:
                    raise RuntimeError(f'Original {name} type changed')
                raw = resolver.read(obj + prop['offset'], 4)
                if kind == 'BoolProperty':
                    mask = resolver.u32(prop['address'] + 0xd0)
                    return bool(struct.unpack('<I', raw)[0] & mask)
                return struct.unpack('<f' if kind == 'FloatProperty' else '<i', raw)[0]

            before = {'used': scalar(weapon, 'AmmoUsedCount', 'IntProperty'),
                      'spare': scalar(weapon, 'SpareAmmoCount', 'IntProperty')}
            if before['used'] == 0:
                bridge.call(weapon, 'SetAmmoUsedCount', NewValue=1)
            if not bridge.call(weapon, 'CanReload'):
                raise RuntimeError('Owned original Lancer cannot reload')
            bridge.call(weapon, 'ForceReload')
            wait_for(lambda: bridge.call(weapon, 'IsReloading'), 2,
                     'Owned original Lancer did not start reload')
            start = scalar(weapon, 'ReloadStartTime', 'FloatProperty')
            samples = []
            for minimum_elapsed in (.10, .40):
                wait_for(lambda: scalar(world, 'TimeSeconds', 'FloatProperty') - start >=
                         minimum_elapsed, 2, 'Original reload clock did not advance')
                attempted_before = scalar(weapon, 'bActiveReloadAttempted', 'BoolProperty')
                bridge.call(weapon, 'GetActiveReloadValues')
                outputs = bridge.last_outputs.copy()
                if set(outputs) != set(expected):
                    raise RuntimeError('Original reload getter returned different outputs')
                decoded = {name: struct.unpack('<f', bytes.fromhex(outputs[name]))[0]
                           for name in expected}
                if not all(math.isfinite(item) for item in decoded.values()):
                    raise RuntimeError('Original reload getter returned nonfinite values')
                attempted_after = scalar(weapon, 'bActiveReloadAttempted', 'BoolProperty')
                if attempted_after != attempted_before:
                    raise RuntimeError('Original getter changed active-reload attempt flag')
                elapsed = scalar(world, 'TimeSeconds', 'FloatProperty') - start
                samples.append({'elapsed': elapsed, 'values': decoded,
                                'attempted_before': attempted_before,
                                'attempted_after': attempted_after,
                                'used': scalar(weapon, 'AmmoUsedCount', 'IntProperty')})
            wait_for(lambda: not bridge.call(weapon, 'IsReloading'), 5,
                     'Original Lancer reload did not finish after getter samples')
            receipt['original_reload_getter'] = {
                'function': resolver.identity(function['address']),
                'parameter_bytes': size, 'parameters': fields,
                'reload_duration': scalar(weapon, 'ReloadDuration', 'FloatProperty'),
                'reload_start': start, 'samples': samples,
                'ammo_before': before,
                'ammo_after': {'used': scalar(weapon, 'AmmoUsedCount', 'IntProperty'),
                               'spare': scalar(weapon, 'SpareAmmoCount', 'IntProperty')},
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
    elif args.mode == 'pawn_ik':
        ik = receipt['current_pawn_ik']
        present = [name for name, control in ik['controls'].items() if control]
        ik['present_controls'] = present
        if (len(ik['controls']) != 8 or not present or
                ik['control_list_count'] < 2 or
                ik['skel_control_index']['count'] < 36):
            raise RuntimeError('Current pawn hand-control certificate incomplete')
        receipt['source_limit'] = ('current player hand-control cache and target spaces read '
                                   'only; does not prove control ownership, a safe write, '
                                   'XR-driven hands or headset rendering')
    elif args.mode == 'anim_tree':
        tree = receipt['current_lancer_anim_tree']
        if (not tree['children']['count'] or not tree['control_lists']['count'] or
                'Anim' not in tree['children']['members'] or
                not {'BoneName', 'ControlHead'} <= set(tree['control_lists']['members']) or
                not any(control['control_name']['text'] == 'AmmoControl'
                        for chain in tree['control_chains'] for control in chain) or
                tree['skel_control_index']['count'] < 9 or
                tree['required_bones']['count'] < 9 or
                set(tree['api']) != {'SetAnimTreeTemplate', 'UpdateAnimations',
                                     'SetSkelControlStrength'}):
            raise RuntimeError('Current Lancer animation graph/control certificate incomplete')
        receipt['source_limit'] = ('current equipped Lancer graph and bone-index caches read '
                                   'only; no new controller created, attached, evaluated or '
                                   'rendered; no engine mutation UFunction called')
    elif args.mode == 'reload_values':
        getter = receipt['original_reload_getter']
        first, second = getter['samples']
        values = first['values']
        if (not 0 < first['elapsed'] < second['elapsed'] < getter['reload_duration'] or
                first['used'] != 1 or second['used'] != 1 or
                getter['ammo_after']['used'] != 0 or
                first['values'] != second['values'] or
                not 0 < values['ActiveReloadStartTime'] < getter['reload_duration'] or
                not 0 < values['SuperSweetSpotDuration'] < .5 or
                not 0 < values['SweetSpotDuration'] < 1):
            raise RuntimeError('Original Lancer getter/window certificate incomplete')
        getter['perfect_end_elapsed'] = (values['ActiveReloadStartTime'] +
                                         values['SuperSweetSpotDuration'])
        receipt['source_limit'] = ('actual original getter outputs on owned Lancer; no physical '
                                   'reload completion, perfect reward or headset acceptance')
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
        bones = [parse_reload_bone_pose_row(line) for line in lines
                 if line.startswith('lancer_reload_bone_pose ')]
        poses = [parse_magazine_pose_row(line) for line in lines
                 if line.startswith('lancer_reload_magazine_pose ')]
        first_reload = next((index for index, row in enumerate(rows) if row['reloading']), None)
        if (len(beginnings) != 1 or len(rows) < 3 or
                [row['sample'] for row in rows] != list(range(len(rows))) or
                any(rows[index]['qpc'] <= rows[index - 1]['qpc'] for index in range(1, len(rows))) or
                first_reload is None or not any(not row['reloading'] for row in rows[first_reload + 1:])):
            raise RuntimeError('Bounded original Lancer reload sequence missing from private log')
        if (len(bones) != len(rows) or
                any(bone['sample'] != row['sample'] or bone['qpc'] != row['qpc']
                    for bone, row in zip(bones, rows))):
            raise RuntimeError('Same-tick original Lancer reload bone sequence incomplete')
        if (len(poses) < 2 or len({pose['sample'] for pose in poses}) != len(poses) or
                any(pose['sample'] >= len(rows) or not rows[pose['sample']]['mag_pawn'] or
                    pose['qpc'] != rows[pose['sample']]['qpc'] for pose in poses)):
            raise RuntimeError('Original hand-held magazine pose sequence missing')
        receipt.update(game_thread_probe=beginnings[0], reload_visual_timeline=rows,
                       reload_bone_poses=bones, held_magazine_poses=poses)
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
    parser.add_argument('--mode', choices=('api', 'pawn_ik', 'anim_tree', 'reload_values', 'chainsaw_api', 'chainsaw_target_eligibility', 'chainsaw_bones', 'chainsaw_bones_held', 'component_motion', 'socket', 'socket_reload', 'support', 'mesh_follow_trial', 'chainsaw_semantics', 'chainsaw_hold_adapter', 'chainsaw_hold_shutdown', 'chainsaw_gesture_adapter', 'chainsaw_xr_no_pose', 'reload_visual', 'reload_hold',
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
