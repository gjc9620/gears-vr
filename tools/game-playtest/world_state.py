"""Bounded current objectives, interaction and PawnList observations; no input."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import time

from core import atomic_json, controllable


def registry_append_only(before, after):
    old_data, old_count, old_capacity = struct.unpack('<Qii', before)
    new_data, new_count, new_capacity = struct.unpack('<Qii', after)
    return (old_data != 0 and new_data == old_data and old_capacity == new_capacity
            and 0 < old_count <= new_count <= new_capacity <= 4000000)


class Query:
    def __init__(self, observer):
        self.observer, self.r, self.s = observer, observer.resolver, observer.sampler
        if self.r is None or self.s is None:
            raise RuntimeError('World-state query requires fresh live readers')
        self.identities, self.metadata, self.properties = {}, {}, {}
        self.started = time.monotonic()
        self.reads = self.bytes = 0
        self.headers = [(self.r.objects_header, self.r.read(self.r.objects_header, 16)),
                        (self.r.names_header, self.r.read(self.r.names_header, 16))]

    def read(self, address, size):
        self.reads += 1
        self.bytes += size
        if self.reads > 16384 or self.bytes > 2 * 1024 * 1024 or time.monotonic() - self.started > 3:
            raise RuntimeError('World-state read budget exceeded')
        return self.r.read(address, size)

    def obj(self, address):
        value = self.s.fresh(address)
        if address in self.identities and self.identities[address] != value:
            raise RuntimeError('World-state object identity changed')
        self.identities[address] = value
        return value

    def prop(self, obj, name, kind=None, *, structure=False):
        owner = obj['address'] if structure else obj['cls']
        key = owner, name
        if key not in self.properties:
            self.properties[key] = self.r.property(owner, name, not structure)
        p = self.properties[key]
        field = self.obj(p['address'])
        if field['name'] != name or (kind and p['type'] != kind):
            raise RuntimeError('World-state property type/name mismatch: ' + name)
        raw = self.read(p['address'] + 0x70, 0x28)
        dim, size = struct.unpack_from('<II', raw)
        if dim != 1 or not 0 < size <= 1024 or struct.unpack_from('<I', raw, 0x24)[0] != p['offset']:
            raise RuntimeError('World-state property layout mismatch: ' + name)
        self.metadata[(p['address'] + 0x70, len(raw))] = raw
        return dict(p, size=size, name=name)

    def value(self, obj, p):
        address = obj['address'] + p['offset']
        raw = self.read(address, p['size'])
        if self.read(address, len(raw)) != raw or self.s.fresh(obj['address']) != obj:
            raise RuntimeError('World-state value changed: ' + p['name'])
        return raw

    def pointer(self, obj, name):
        p = self.prop(obj, name, 'ObjectProperty')
        if p['size'] != 8:
            raise RuntimeError('World-state pointer width mismatch')
        pointer = struct.unpack('<Q', self.value(obj, p))[0]
        return self.obj(pointer) if pointer else None

    def scalar(self, obj, name, kind, fmt):
        p = self.prop(obj, name, kind)
        if p['size'] != struct.calcsize(fmt):
            raise RuntimeError('World-state scalar width mismatch')
        value = struct.unpack(fmt, self.value(obj, p))[0]
        if isinstance(value, float) and not math.isfinite(value):
            raise RuntimeError('World-state scalar is nonfinite')
        return value

    def position(self, obj):
        p = self.prop(obj, 'Location', 'StructProperty')
        typ = self.obj(self.r.struct_type(p))
        if typ['name'] != 'Vector' or p['size'] != 12:
            raise RuntimeError('World-state Location is not Vector')
        for name, offset in (('X', 0), ('Y', 4), ('Z', 8)):
            field = self.prop(typ, name, 'FloatProperty', structure=True)
            if field['offset'] != offset or field['size'] != 4:
                raise RuntimeError('World-state Vector layout mismatch')
        xyz = list(struct.unpack('<3f', self.value(obj, p)))
        if not all(math.isfinite(v) for v in xyz):
            raise RuntimeError('World-state position is nonfinite')
        return xyz

    def array(self, obj, name, limit=128):
        p = self.prop(obj, name, 'ArrayProperty')
        if p['size'] != 16:
            raise RuntimeError('World-state array header width mismatch')
        header = self.value(obj, p)
        data, count, capacity = struct.unpack('<Qii', header)
        if not 0 <= count <= capacity <= limit or (count and not data):
            raise RuntimeError('World-state array bounds rejected')
        inner = self.obj(self.r.u64(p['address'] + self.r.profile['struct_property_struct']))
        if inner['outer'] != p['address']:
            raise RuntimeError('World-state array inner owner mismatch')
        meta = self.read(inner['address'] + 0x70, 8)
        self.metadata[(inner['address'] + 0x70, 8)] = meta
        dim, stride = struct.unpack('<II', meta)
        if dim != 1 or not 1 <= stride <= 1024 or count * stride > 32768:
            raise RuntimeError('World-state array stride rejected')
        raw = self.read(data, count * stride) if count else b''
        if (self.read(obj['address'] + p['offset'], 16) != header or
                (count and self.read(data, count * stride) != raw)):
            raise RuntimeError('World-state array changed')
        return inner, stride, raw, count

    def decode(self, kind, raw, address=None):
        if kind == 'IntProperty' and len(raw) == 4:
            return struct.unpack('<i', raw)[0]
        if kind == 'ByteProperty' and len(raw) == 1:
            return raw[0]
        if kind == 'FloatProperty' and len(raw) == 4:
            value = struct.unpack('<f', raw)[0]
            if not math.isfinite(value):
                raise RuntimeError('Nonfinite objective field')
            return value
        if kind == 'NameProperty' and len(raw) == 8:
            index, number = struct.unpack('<II', raw)
            return dict(name=self.r.name(index), number=number)
        if kind == 'StrProperty' and len(raw) == 16:
            pointer, count, capacity = struct.unpack('<Qii', raw)
            if not 0 <= count <= capacity <= 512 or (count and not pointer):
                raise RuntimeError('Objective string bounds rejected')
            data = self.read(pointer, count * 2) if count else b''
            if count and (not data.endswith(b'\0\0') or self.read(pointer, len(data)) != data):
                raise RuntimeError('Objective string changed or is unterminated')
            return data.decode('utf-16-le')[:-1] if count else ''
        if kind == 'BoolProperty' and len(raw) == 4 and address:
            mask = self.read(address + 0xd0, 4)
            self.metadata[(address + 0xd0, 4)] = mask
            value = struct.unpack('<I', mask)[0]
            if value and not value & (value - 1):
                return bool(struct.unpack('<I', raw)[0] & value)
        return dict(unsupported=kind, raw=raw.hex())

    def objectives(self, controller):
        inner, stride, raw, count = self.array(controller, 'Objectives', 256)
        kind = self.r.class_name(inner)
        result = dict(kind=kind, count=count, stride=stride, rows=[], interpreted=True)
        fields = []
        if kind == 'StructProperty':
            typ = self.obj(self.r.struct_type(dict(address=inner['address'], type=kind)))
            result['struct'] = typ['name']
            for field in self.r.fields(typ['address']):
                if len(fields) >= 32:
                    raise RuntimeError('Objective struct has too many fields')
                if self.r.class_name(field).endswith('Property'):
                    p = self.prop(typ, field['name'], structure=True)
                    if p['offset'] + p['size'] > stride:
                        raise RuntimeError('Objective field escapes element')
                    fields.append(p)
            result['fields'] = fields
        for i in range(count):
            row = raw[i * stride:(i + 1) * stride]
            if fields:
                value = {p['name']: self.decode(p['type'], row[p['offset']:p['offset'] + p['size']], p['address'])
                         for p in fields}
            else:
                value = self.decode(kind, row, inner['address'])
            result['rows'].append(value)
        if any('unsupported' in json.dumps(row) for row in result['rows']):
            result['interpreted'] = False
        return result

    def finish(self, *, allow_registry_growth=False):
        self.observer.verify()
        if (any(self.s.fresh(p) != v for p, v in self.identities.items()) or
                any(self.r.read(p, n) != raw for (p, n), raw in self.metadata.items()) or
                any(not (current == raw or (allow_registry_growth and registry_append_only(raw, current)))
                    for p, raw in self.headers for current in (self.r.read(p, len(raw)),))):
            raise RuntimeError('World-state metadata/registry changed at closing boundary')


def combat_control(q, pawn, world):
    direct = q.pointer(pawn, 'Controller')
    controller, controlled, vehicle = direct, pawn, None
    if not direct:
        vehicle = q.pointer(pawn, 'DrivenVehicle')
        if vehicle:
            if (not q.s.derives(vehicle['cls'], 'Vehicle') or q.pointer(vehicle, 'WorldInfo') != world
                    or q.pointer(vehicle, 'Driver') != pawn):
                raise RuntimeError('Mounted pawn/vehicle reciprocal world links changed')
            controlled = vehicle
            controller = q.pointer(vehicle, 'Controller')
    enemy, ai = None, False
    if controller:
        if q.pointer(controller, 'Pawn') != controlled or q.pointer(controller, 'WorldInfo') != world:
            raise RuntimeError('Pawn/controller reciprocal world links changed')
        ai = q.s.derives(controller['cls'], 'AIController')
        enemy = q.pointer(controller, 'Enemy')
    return dict(direct_controller=direct, controller=controller, controlled_pawn=controlled,
                driven_vehicle=vehicle, ai_controller=ai, enemy=enemy)


def team_membership(q, pawn, world):
    if q.pointer(pawn, 'WorldInfo') != world:
        raise RuntimeError('Team pawn belongs to another world')
    pri = q.pointer(pawn, 'PlayerReplicationInfo')
    if not pri:
        return None
    if not q.s.derives(pri['cls'], 'PlayerReplicationInfo') or q.pointer(pri, 'WorldInfo') != world:
        raise RuntimeError('Team PRI ancestry/world mismatch')
    team = q.pointer(pri, 'Team')
    if not team:
        return None
    if not q.s.derives(team['cls'], 'WarTeamInfo') or q.pointer(team, 'WorldInfo') != world:
        raise RuntimeError('Team identity ancestry/world mismatch')
    index = q.scalar(team, 'TeamIndex', 'IntProperty', '<i')
    return dict(pri=pri, team=team, index=index) if index in (0, 1) else None


def squad_hostility(q, local, control, world):
    """Admit an opposing AI attacking a live pawn on this player's exact team."""
    enemy = control['enemy']
    if not control['ai_controller'] or not enemy or enemy == local or not q.s.derives(enemy['cls'], 'WarPawn'):
        return None
    player_team = team_membership(q, local, world)
    opponent = team_membership(q, control['controlled_pawn'], world)
    if (not player_team or not opponent or player_team['team'] == opponent['team']
            or player_team['index'] == opponent['index']):
        return None
    enemy_team = team_membership(q, enemy, world)
    if (not enemy_team or enemy_team['team'] != player_team['team']
            or enemy_team['index'] != player_team['index']
            or q.scalar(enemy, 'Health', 'IntProperty', '<i') <= 0):
        return None
    return dict(verified=True, basis='Opposing AI currently targets a live member of the local exact team',
                enemy=enemy, player_team=player_team, enemy_team=enemy_team, opponent_team=opponent)


def streaming_levels(q, world):
    inner, stride, raw, count = q.array(world, 'StreamingLevels', 512)
    if q.r.class_name(inner) != 'ObjectProperty' or stride != 8:
        raise RuntimeError('Streaming-level array schema rejected')
    result = dict(count=count, loaded=[], errors=[], atomic_snapshot=False)
    for index in range(count):
        address = struct.unpack_from('<Q', raw, index * 8)[0]
        if not address:
            continue
        try:
            entry = q.obj(address)
            if not q.s.derives(entry['cls'], 'LevelStreaming'):
                raise RuntimeError('Streaming-level ancestry rejected')
            loaded = q.pointer(entry, 'LoadedLevel')
            if not loaded:
                continue
            if not q.s.derives(loaded['cls'], 'Level'):
                raise RuntimeError('Loaded level ancestry rejected')
            row = dict(identity=entry, loaded_level=loaded)
            for name, kind in (('PackageName', 'NameProperty'), ('bIsVisible', 'BoolProperty'),
                               ('bShouldBeVisible', 'BoolProperty'), ('bShouldBeLoaded', 'BoolProperty')):
                prop = q.prop(entry, name, kind)
                row[name] = q.decode(kind, q.value(entry, prop), prop['address'])
            result['loaded'].append(row)
        except (OSError, RuntimeError, ValueError) as error:
            result['errors'].append(dict(address=hex(address), message=str(error)))
    return result


def capture(observer, *, max_pawns=128, include_streaming=False):
    initial = observer.sample()
    if initial['errors'] or not controllable(initial):
        raise RuntimeError('World-state capture requires certified controllable state')
    q = Query(observer)
    pawn, controller = q.obj(observer.sampler.pawn), q.obj(observer.sampler.controller)
    world = q.pointer(pawn, 'WorldInfo')
    world_time = q.scalar(world, 'TimeSeconds', 'FloatProperty', '<f')
    world_outer_chain, seen_outer, current_outer = [], set(), world['outer']
    map_package = None
    while current_outer and current_outer not in seen_outer and len(seen_outer) < 8:
        seen_outer.add(current_outer)
        parent = q.obj(current_outer)
        parent_class = q.obj(parent['cls'])['name']
        world_outer_chain.append(dict(identity=parent, class_name=parent_class))
        if parent_class == 'Package':
            map_package = parent['name']
            break
        current_outer = parent['outer']
    result = dict(schema=1, game_identity=observer.identities['game_identity'],
                  disk_game_sha256=q.r.profile['sha256'], module_base=hex(q.r.base),
                  world_identity=world, source_sample=initial, errors=[], pawns=[],
                  combat_render_hint_policy='recent-body-render-0.5s', world_time=world_time,
                  map_package=map_package, world_outer_chain=world_outer_chain,
                  persistent_world_package=map_package,
                  map_package_semantics='WorldInfo outer package; streamed mission packages are separate',
                  atomic_snapshot=False, exclusive_damage_attribution=False)
    for component, getter in (
            ('objectives', lambda: q.objectives(controller)),
            ('interaction', lambda: interaction(q, controller))):
        try:
            result[component] = getter()
        except (OSError, RuntimeError, ValueError) as error:
            result['errors'].append(dict(component=component, type=type(error).__name__, message=str(error)))
    try:
        current, seen = q.pointer(world, 'PawnList'), set()
        while current and len(seen) < max_pawns:
            if current['address'] in seen:
                raise RuntimeError('PawnList contains a cycle')
            seen.add(current['address'])
            following = q.pointer(current, 'NextPawn')
            if q.s.derives(current['cls'], 'WarPawn') and not current['name'].startswith('Default__'):
                try:
                    actor_world = q.pointer(current, 'WorldInfo')
                    if actor_world != world:
                        raise RuntimeError('Pawn belongs to another WorldInfo')
                    health = q.scalar(current, 'Health', 'IntProperty', '<i')
                    control = combat_control(q, current, world)
                    record = dict(identity=current, health=health, position=q.position(current), **control,
                        targets_player=bool(control['ai_controller'] and control['enemy'] == pawn and current != pawn),
                        observed_seconds=time.monotonic() - q.started)
                    try:
                        record['render_hint'] = render_hint(q, current, world)
                    except (OSError, RuntimeError, ValueError) as error:
                        record['render_hint_unavailable'] = str(error)
                    try:
                        record['squad_hostility'] = squad_hostility(q, pawn, control, world)
                    except (OSError, RuntimeError, ValueError) as error:
                        record['squad_hostility_unavailable'] = str(error)
                    try:
                        prop = q.prop(current, 'CollisionComponent', 'ComponentProperty')
                        if prop['size'] != 8:
                            raise RuntimeError('Collision component pointer width mismatch')
                        address = struct.unpack('<Q', q.value(current, prop))[0]
                        component = q.obj(address) if address else None
                        if component and q.s.derives(component['cls'], 'CylinderComponent'):
                            height = q.scalar(component, 'CollisionHeight', 'FloatProperty', '<f')
                            radius = q.scalar(component, 'CollisionRadius', 'FloatProperty', '<f')
                            if not 0 < height <= 300 or not 0 < radius <= 300:
                                raise RuntimeError('Pawn collision dimensions rejected')
                            record['collision'] = dict(identity=component, height=height, radius=radius)
                    except (OSError, RuntimeError, ValueError) as error:
                        record['collision_unavailable'] = str(error)
                    result['pawns'].append(record)
                except (OSError, RuntimeError, ValueError) as error:
                    result['errors'].append(dict(component='pawn', identity=current, message=str(error)))
            current = following
        result['pawn_list_truncated'] = bool(current)
        result['pawn_list_scanned'] = len(seen)
    except (OSError, RuntimeError, ValueError) as error:
        result['errors'].append(dict(component='pawn_list', type=type(error).__name__, message=str(error)))
    if include_streaming:
        result['streaming_levels'] = streaming_levels(q, world)
    q.finish(allow_registry_growth=include_streaming)
    after = observer.sample()
    if (after['errors'] or not controllable(after) or after.get('pawn_identity') != initial.get('pawn_identity')
            or after.get('native', {}).get('world_generation') != initial.get('native', {}).get('world_generation')):
        raise RuntimeError('World-state control context changed during capture')
    result['end_sample'] = after
    result.update(consistent_metadata_reread=True, selected_read_calls=q.reads,
                  selected_read_bytes=q.bytes, acquisition_seconds=time.monotonic() - q.started)
    return result


def interaction(q, controller):
    event = q.pointer(controller, 'InteractEvent')
    if not event:
        return dict(event=None)
    result = dict(event=event, class_name=q.obj(event['cls'])['name'])
    if q.s.derives(event['cls'], 'SeqEvt_DoorOpen'):
        from interaction_flow import door_context
        result['door_context'] = door_context(q, controller, event)
    for name in ('Originator', 'Instigator'):
        try:
            result[name.lower()] = q.pointer(event, name)
        except RuntimeError as error:
            result[name.lower() + '_unavailable'] = str(error)
    for name in ('InteractDistance', 'InteractFOV'):
        try:
            result[name] = q.scalar(event, name, 'FloatProperty', '<f')
        except RuntimeError as error:
            result[name + '_unavailable'] = str(error)
    for name, kind, fmt in (('TriggerCount', 'IntProperty', '<i'), ('ActivationTime', 'FloatProperty', '<f')):
        try:
            result[name] = q.scalar(event, name, kind, fmt)
        except RuntimeError as error:
            result[name + '_unavailable'] = str(error)
    return result


def objective_fingerprint(value):
    if not value or value.get('interpreted') is not True:
        return None
    rows = value['rows']
    if value.get('struct') == 'ObjectiveInfo':
        rows = [{k: v for k, v in row.items() if k not in ('UpdatedTime', 'bUpdated', 'bNotifyPlayer')}
                for row in rows]
    semantic = dict(kind=value['kind'], rows=rows)
    return hashlib.sha256(json.dumps(semantic, sort_keys=True, ensure_ascii=False).encode()).hexdigest()


def hostile(pawn):
    intent = pawn.get('targets_player') is True or (pawn.get('squad_hostility') or {}).get('verified') is True
    return intent and pawn.get('ai_controller') is True and pawn.get('health', 0) > 0


def render_hint(q, pawn, world):
    actor_time = q.scalar(pawn, 'LastRenderTime', 'FloatProperty', '<f')
    hidden = q.prop(pawn, 'bHidden', 'BoolProperty')
    actor_hidden = q.decode(hidden['type'], q.value(pawn, hidden), hidden['address'])
    prop = q.prop(pawn, 'Mesh', 'ComponentProperty')
    if prop['size'] != 8:
        raise RuntimeError('Render-hint Mesh pointer width mismatch')
    address = struct.unpack('<Q', q.value(pawn, prop))[0]
    mesh = q.obj(address) if address else None
    if not mesh or mesh['outer'] != pawn['address'] or not q.s.derives(mesh['cls'], 'SkeletalMeshComponent'):
        raise RuntimeError('Render-hint Mesh ownership or ancestry mismatch')
    mesh_time = q.scalar(mesh, 'LastRenderTime', 'FloatProperty', '<f')
    hidden = q.prop(mesh, 'HiddenGame', 'BoolProperty')
    mesh_hidden = q.decode(hidden['type'], q.value(mesh, hidden), hidden['address'])
    if type(actor_hidden) is not bool or type(mesh_hidden) is not bool:
        raise RuntimeError('Render-hint hidden flags are not decoded booleans')
    world_time = q.scalar(world, 'TimeSeconds', 'FloatProperty', '<f')
    recent = (not actor_hidden and not mesh_hidden and
              mesh_time > 0 and -.1 <= world_time - mesh_time <= .5)
    return dict(recently_rendered=recent, world_time=world_time, actor_time=actor_time,
                mesh_time=mesh_time, actor_hidden=actor_hidden, mesh_hidden=mesh_hidden,
                mesh=mesh, line_of_sight_verified=False,
                scope='Render timestamp hint only; shadows, occlusion and hittability remain unproven')


def select_hostile(value, *, max_distance=2500, max_height=150, prefer_mounted=False,
                   require_recent_render=False):
    origin = value['source_sample']['pose']['position']
    candidates = []
    for pawn in value.get('pawns', []):
        if not hostile(pawn):
            continue
        if require_recent_render and (pawn.get('render_hint') or {}).get('recently_rendered') is not True:
            continue
        position = pawn.get('position')
        if not position or abs(position[2] - origin[2]) > max_height:
            continue
        distance = math.dist(origin, position)
        if distance <= max_distance:
            priority = int(not pawn.get('driven_vehicle')) if prefer_mounted else 0
            candidates.append((priority, distance, pawn['identity']['address'], pawn))
    return min(candidates)[3] if candidates else None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    from live import Observer
    observer = Observer(args.session)
    try:
        value = capture(observer)
        value['keyboard_bindings'] = observer.bindings(extra_keys=('E', 'F', 'R', 'SpaceBar'))
        atomic_json(args.output, value)
        print(json.dumps(dict(pawns=len(value['pawns']), objectives=value.get('objectives'),
                              interaction=value.get('interaction'), errors=value['errors'])))
    finally:
        observer.close()


if __name__ == '__main__':
    main()
