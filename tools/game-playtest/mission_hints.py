"""Bounded live sequence hints; spatial candidates do not prove mission order."""
import argparse
import json
import math
from pathlib import Path
import struct
import time

from core import atomic_json, controllable
from world_state import Query
from navigation import walking_edge


KINDS = {'SeqEvt_Interaction', 'SeqAct_AddObjective', 'SeqAct_CompleteObjective'}


def choose_goal(graph, hints, position, *, avoid=(), allow_forced=False):
    """Select an unused enabled trigger close to the directed walking component."""
    if (graph.get('consistent_reread') is not True or hints.get('consistent_metadata_reread') is not True
            or graph['world_identity'] != hints['world_identity']
            or graph['game_identity'] != hints['game_identity']):
        return None
    nodes = graph['nodes']
    nearby = [(math.dist(position, row['position']), key) for key, row in nodes.items()
              if abs(position[2] - row['position'][2]) <= 180]
    if not nearby or min(nearby)[0] > 400:
        return None
    start = min(nearby)[1]
    adjacency = {}
    for edge in graph['edges']:
        walk_hint = walking_edge(edge, allow_forced=allow_forced)
        if walk_hint and edge['start'] in nodes and edge['end'] in nodes:
            adjacency.setdefault(edge['start'], []).append(edge['end'])
    reachable, pending = {start}, [start]
    while pending:
        node = pending.pop()
        for target in adjacency.get(node, ()):
            if target not in reachable:
                reachable.add(target)
                pending.append(target)
    avoided = set(avoid)
    candidates = []
    generation = hints['source_sample']['native']['world_generation']
    if not isinstance(generation, int) or generation != graph['source_sample']['native'].get('world_generation'):
        return None
    for row in hints['rows']:
        event = (row.get('interaction_details') or {}).get('event')
        if not event or row.get('same_world_verified') is not True or not row.get('position'):
            continue
        key = (generation, event['address'], event['internal_index'], event['number'])
        state = row['interaction_details']
        count, maximum = state.get('TriggerCount'), state.get('MaxTriggerCount')
        if (key in avoided or state.get('bEnabled') is not True
                or not isinstance(count, int) or not isinstance(maximum, int)
                or count < 0 or maximum < 0 or (maximum and count >= maximum)):
            continue
        gap, node = min((math.dist(row['position'], nodes[n]['position']), n) for n in reachable)
        if gap > 180:
            continue
        candidates.append((math.dist(position, row['position']) + gap, event['address'], row, node, gap))
    if not candidates:
        return choose_goal(graph, hints, position, avoid=avoid, allow_forced=True) if not allow_forced else None
    _, _, row, node, gap = min(candidates, key=lambda item: item[:2])
    radius = (row.get('collision') or {}).get('radius')
    player_radius = (hints.get('player_collision') or {}).get('radius')
    dimensions_valid = all(isinstance(v, (int, float)) and math.isfinite(v) and v > 0 for v in (radius, player_radius))
    arrival = max(20, min(100, radius + player_radius - 20)) if dimensions_valid else 50
    return dict(event=row['interaction_details']['event'], position=row['position'],
                arrival_radius=arrival, reachable_node=node, node_gap=gap,
                reachable_nodes=len(reachable), forced_hints_required=allow_forced,
                mission_order_verified=False, walkability_verified=False)


def capture(observer, *, seconds=10, max_objects=350000, max_candidates=256):
    initial = observer.sample()
    if initial['errors'] or not controllable(initial):
        raise RuntimeError('Mission hints require current controllable state')
    r, sampler = observer.resolver, observer.sampler
    started = time.monotonic()
    header = r.read(r.objects_header, 16)
    data, count, capacity = struct.unpack('<Qii', header)
    if not data or not 1000 < count <= capacity < 4000000 or count > max_objects:
        raise RuntimeError('Mission object inventory bound exceeded')
    pointers = struct.unpack('<' + 'Q' * count, r.read(data, count * 8))
    discover = r.discovery_identity_reader()
    candidates, rejected, interaction_classes = [], 0, {}
    for index, address in enumerate(pointers):
        if time.monotonic() - started > seconds:
            raise RuntimeError('Mission hint discovery time bound exceeded')
        if not address:
            continue
        try:
            obj = discover(address)
            if obj['internal_index'] != index:
                rejected += 1
                continue
            kind = r.class_name(obj)
        except (OSError, RuntimeError, ValueError):
            rejected += 1
            continue
        is_interaction = kind == 'SeqEvt_Interaction'
        if kind.startswith('SeqEvt_'):
            if obj['cls'] not in interaction_classes:
                interaction_classes[obj['cls']] = sampler.derives(obj['cls'], 'SeqEvt_Interaction')
            is_interaction = interaction_classes[obj['cls']]
        if (kind in KINDS or is_interaction) and not obj['name'].startswith('Default__'):
            candidates.append((address, kind, is_interaction))
            if len(candidates) > max_candidates:
                raise RuntimeError('Mission candidate bound exceeded')
    if r.read(r.objects_header, 16) != header:
        raise RuntimeError('Mission registry changed during discovery')
    q = Query(observer)
    pawn = q.obj(sampler.pawn)
    world = q.pointer(pawn, 'WorldInfo')
    player_collision = {}
    prop = q.prop(pawn, 'CollisionComponent', 'ComponentProperty')
    if prop['size'] == 8:
        address = struct.unpack('<Q', q.value(pawn, prop))[0]
        if address:
            component = q.obj(address)
            if sampler.derives(component['cls'], 'CylinderComponent'):
                player_collision = dict(radius=q.scalar(component, 'CollisionRadius', 'FloatProperty', '<f'),
                                        height=q.scalar(component, 'CollisionHeight', 'FloatProperty', '<f'))
    rows, errors = [], []
    for address, kind, is_interaction in candidates:
        try:
            obj = q.obj(address)
            if q.obj(obj['cls'])['name'] != kind:
                raise RuntimeError('Mission candidate class changed')
            row = dict(identity=obj, kind=kind, outer_chain=[])
            outer, seen = obj['outer'], set()
            while outer and outer not in seen and len(seen) < 8:
                seen.add(outer)
                parent = q.obj(outer)
                row['outer_chain'].append(dict(name=parent['name'], number=parent['number']))
                outer = parent['outer']
            if is_interaction:
                if not sampler.derives(obj['cls'], 'SeqEvt_Interaction'):
                    raise RuntimeError('Interaction ancestry changed')
                row.update(interaction_details=interaction_event(q, obj))
                origin = row['interaction_details'].get('originator')
                if not origin or not sampler.derives(origin['cls'], 'Actor'):
                    continue
                if q.pointer(origin, 'WorldInfo') != world:
                    continue
                row['position'] = q.position(origin)
                row['distance'] = math.dist(initial['pose']['position'], row['position'])
                row['same_world_verified'] = True
                prop = q.prop(origin, 'CollisionComponent', 'ComponentProperty')
                if prop['size'] != 8:
                    raise RuntimeError('Interaction collision component width mismatch')
                component_address = struct.unpack('<Q', q.value(origin, prop))[0]
                if component_address:
                    component = q.obj(component_address)
                    row['collision_class'] = q.obj(component['cls'])['name']
                    if sampler.derives(component['cls'], 'CylinderComponent'):
                        row['collision'] = dict(
                            radius=q.scalar(component, 'CollisionRadius', 'FloatProperty', '<f'),
                            height=q.scalar(component, 'CollisionHeight', 'FloatProperty', '<f'))
            else:
                for name in ('ObjectiveName', 'ObjectiveDesc'):
                    if name == 'ObjectiveDesc' and kind != 'SeqAct_AddObjective':
                        continue
                    prop = q.prop(obj, name)
                    row[name] = q.decode(prop['type'], q.value(obj, prop), prop['address'])
            rows.append(row)
        except (OSError, RuntimeError, ValueError) as error:
            errors.append(dict(address=hex(address), kind=kind, message=str(error)))
    q.finish()
    after = observer.sample()
    if (after['errors'] or not controllable(after) or after.get('pawn_identity') != initial.get('pawn_identity')
            or after.get('native', {}).get('world_generation') != initial.get('native', {}).get('world_generation')):
        raise RuntimeError('Mission hint control context changed')
    return dict(schema=1, source_sample=initial, end_sample=after,
                game_identity=observer.identities['game_identity'], world_identity=world,
                disk_game_sha256=r.profile['sha256'], module_base=hex(r.base),
                rows=rows, errors=errors, player_collision=player_collision,
                scanned_objects=count, rejected_discovery_objects=rejected,
                acquisition_seconds=time.monotonic() - started, consistent_metadata_reread=True,
                mission_order_verified=False, walkability_verified=False, atomic_snapshot=False)


def interaction_event(q, event):
    # Reuse the same typed readers without pretending this is PC.InteractEvent.
    result = dict(event=event, originator=q.pointer(event, 'Originator'))
    for name in ('TriggerCount', 'MaxTriggerCount'):
        result[name] = q.scalar(event, name, 'IntProperty', '<i')
    for name in ('InteractDistance', 'InteractFOV'):
        result[name] = q.scalar(event, name, 'FloatProperty', '<f')
    prop = q.prop(event, 'bEnabled', 'BoolProperty')
    result['bEnabled'] = q.decode(prop['type'], q.value(event, prop), prop['address'])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    from live import Observer
    observer = Observer(args.session)
    try:
        value = capture(observer)
        atomic_json(args.output, value)
        print(json.dumps(dict(output=str(args.output), scanned=value['scanned_objects'],
                              rows=len(value['rows']), errors=len(value['errors']),
                              seconds=value['acquisition_seconds'])))
    finally:
        observer.close()


if __name__ == '__main__':
    main()
