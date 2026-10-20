"""Read one live Kismet parent and its links; never activate sequence operations."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import time

from core import atomic_json, controllable
from world_state import Query
from navigation import walking_edge


def find_operations(observer, kind, *, seconds=10):
    """Find one exact live operation class with bounded registry discovery."""
    source = observer.sample()
    if source['errors'] or not controllable(source):
        raise RuntimeError('Sequence search requires current controllable state')
    r = observer.resolver
    header = r.read(r.objects_header, 16)
    data, count, capacity = struct.unpack('<Qii', header)
    if not data or not 1000 < count <= capacity < 4000000 or count > 350000:
        raise RuntimeError('Sequence inventory bound exceeded')
    pointers = struct.unpack('<' + 'Q' * count, r.read(data, count * 8))
    read_identity = r.discovery_identity_reader()
    started = time.monotonic()
    candidates = []
    for index, address in enumerate(pointers):
        if time.monotonic() - started > seconds:
            raise RuntimeError('Sequence search time bound exceeded')
        if not address:
            continue
        try:
            obj = read_identity(address)
            if (obj['internal_index'] == index and not obj['name'].startswith('Default__')
                    and r.class_name(obj) == kind):
                candidates.append(address)
        except (OSError, RuntimeError, ValueError):
            continue
    if len(candidates) > 1024 or r.read(r.objects_header, 16) != header:
        raise RuntimeError('Sequence discovery changed or exceeded candidate bound')
    q = Query(observer)
    matches = []
    for address in candidates:
        obj = q.obj(address)
        if q.obj(obj['cls'])['name'] != kind:
            raise RuntimeError('Sequence caller class changed')
        values = labels(q, obj, kind)
        parent = q.pointer(obj, 'ParentSequence') if q.s.derives(obj['cls'], 'SequenceOp') else None
        matches.append(dict(identity=obj, labels=values, parent=parent))
    q.finish()
    return dict(schema=1, game_identity=observer.identities['game_identity'], source_sample=source,
                kind=kind, matches=matches, candidates=len(candidates),
                atomic_snapshot=False, consistent_metadata_reread=True, active_execution_proven=False)


def find_remote_activators(observer, event_name, *, seconds=10):
    result = find_operations(observer, 'SeqAct_ActivateRemoteEvent', seconds=seconds)
    result['event_name'] = event_name
    if event_name is not None:
        result['matches'] = [row for row in result['matches'] if row['labels']['EventName'] == event_name]
    return result


def component_bounds(q, actor):
    prop = q.prop(actor, 'CollisionComponent', 'ComponentProperty')
    if prop['size'] != 8:
        raise RuntimeError('Volume collision pointer width mismatch')
    pointer = struct.unpack('<Q', q.value(actor, prop))[0]
    if not pointer:
        return None
    component = q.obj(pointer)
    if component['outer'] != actor['address'] or not q.s.derives(component['cls'], 'PrimitiveComponent'):
        raise RuntimeError('Volume collision owner or ancestry mismatch')
    return primitive_bounds(q, component)


def primitive_bounds(q, component):
    if not q.s.derives(component['cls'], 'PrimitiveComponent'):
        raise RuntimeError('Primitive bounds ancestry mismatch')
    prop = q.prop(component, 'Bounds', 'StructProperty')
    typ = q.obj(q.r.struct_type(prop))
    if typ['name'] != 'BoxSphereBounds' or prop['size'] != 28:
        raise RuntimeError('Volume bounds are not BoxSphereBounds')
    raw = q.value(component, prop)
    values = {}
    for name in ('Origin', 'BoxExtent'):
        vector, value = field(q, typ, raw, name, 'StructProperty')
        vec_type = q.obj(q.r.struct_type(vector))
        if vec_type['name'] != 'Vector' or len(value) != 12:
            raise RuntimeError('Volume bounds vector type mismatch')
        for axis, offset in (('X', 0), ('Y', 4), ('Z', 8)):
            axis_prop = q.prop(vec_type, axis, 'FloatProperty', structure=True)
            if axis_prop['offset'] != offset or axis_prop['size'] != 4:
                raise RuntimeError('Volume bounds vector layout mismatch')
        values[name] = list(struct.unpack('<3f', value))
    _, value = field(q, typ, raw, 'SphereRadius', 'FloatProperty')
    if len(value) != 4:
        raise RuntimeError('Volume radius width mismatch')
    radius = struct.unpack('<f', value)[0]
    if (not all(math.isfinite(v) for vector in values.values() for v in vector)
            or not all(0 < v < 20000 for v in values['BoxExtent'])
            or not math.isfinite(radius) or not 0 < radius < 40000):
        raise RuntimeError('Volume bounds rejected')
    return dict(component=component, center=values['Origin'], extent=values['BoxExtent'], radius=radius)


def structure(q, inner, expected):
    if q.r.class_name(inner) != 'StructProperty':
        raise RuntimeError('Sequence link element is not a reflected struct')
    typ = q.obj(q.r.struct_type(dict(address=inner['address'], type='StructProperty')))
    if typ['name'] != expected:
        raise RuntimeError('Unexpected sequence link struct: ' + typ['name'])
    return typ


def field(q, typ, raw, name, kind):
    prop = q.prop(typ, name, kind, structure=True)
    if prop['offset'] + prop['size'] > len(raw):
        raise RuntimeError('Sequence struct field escapes element')
    value = raw[prop['offset']:prop['offset'] + prop['size']]
    return prop, value


def object_field(q, typ, raw, name):
    _, value = field(q, typ, raw, name, 'ObjectProperty')
    if len(value) != 8:
        raise RuntimeError('Sequence pointer width mismatch')
    pointer = struct.unpack('<Q', value)[0]
    return q.obj(pointer) if pointer else None


def nested_links(q, typ, raw):
    prop, header = field(q, typ, raw, 'Links', 'ArrayProperty')
    if len(header) != 16:
        raise RuntimeError('Nested sequence array header mismatch')
    data, count, capacity = struct.unpack('<Qii', header)
    if not 0 <= count <= capacity <= 128 or (count and not data):
        raise RuntimeError('Nested sequence link bounds rejected')
    inner = q.obj(q.r.u64(prop['address'] + q.r.profile['struct_property_struct']))
    if inner['outer'] != prop['address']:
        raise RuntimeError('Nested sequence element owner mismatch')
    meta = q.read(inner['address'] + 0x70, 8)
    q.metadata[(inner['address'] + 0x70, 8)] = meta
    dim, stride = struct.unpack('<II', meta)
    if dim != 1 or not 12 <= stride <= 64:
        raise RuntimeError('Nested sequence link stride rejected')
    element_type = structure(q, inner, 'SeqOpOutputInputLink')
    contents = q.read(data, count * stride) if count else b''
    if count and q.read(data, len(contents)) != contents:
        raise RuntimeError('Nested sequence links changed')
    result = []
    for index in range(count):
        entry = contents[index * stride:(index + 1) * stride]
        linked = object_field(q, element_type, entry, 'LinkedOp')
        _, raw_index = field(q, element_type, entry, 'InputLinkIdx', 'IntProperty')
        if len(raw_index) != 4:
            raise RuntimeError('Sequence input index width mismatch')
        input_index = struct.unpack('<i', raw_index)[0]
        if not 0 <= input_index <= 127:
            raise RuntimeError('Sequence input index rejected')
        if linked:
            result.append(dict(target=linked, input_index=input_index))
    return result


def links(q, obj):
    inner, stride, contents, count = q.array(obj, 'OutputLinks', 128)
    typ = structure(q, inner, 'SeqOpOutputLink')
    result = []
    for index in range(count):
        raw = contents[index * stride:(index + 1) * stride]
        prop, value = field(q, typ, raw, 'LinkDesc', 'StrProperty')
        description = q.decode(prop['type'], value, prop['address'])
        for link in nested_links(q, typ, raw):
            result.append(dict(link, output_index=index, description=description))
    return result


def labels(q, obj, kind):
    result = {}
    names = {'SeqEvent_RemoteEvent': [('EventName', 'NameProperty')],
             'SeqAct_ActivateRemoteEvent': [('EventName', 'NameProperty')],
             'SeqEvent_SequenceActivated': [('InputLabel', 'StrProperty')],
             'SeqAct_FinishSequence': [('OutputLabel', 'StrProperty')],
             'SeqAct_CompleteObjective': [('ObjectiveName', 'NameProperty')],
             'SeqAct_AddObjective': [('ObjectiveName', 'NameProperty'), ('ObjectiveDesc', 'StrProperty')]}
    for name, kind in names.get(kind, []):
        prop = q.prop(obj, name, kind)
        result[name] = q.decode(kind, q.value(obj, prop), prop['address'])
    return result


def variable_links(q, obj):
    """Read selected variable links for evidence; do not evaluate native conditions."""
    inner, stride, raw, count = q.array(obj, 'VariableLinks', 32)
    typ = structure(q, inner, 'SeqVarLink')
    result = []
    for index in range(count):
        entry = raw[index * stride:(index + 1) * stride]
        description, txt = field(q, typ, entry, 'LinkDesc', 'StrProperty')
        prop, header = field(q, typ, entry, 'LinkedVariables', 'ArrayProperty')
        if len(header) != 16:
            raise RuntimeError('Sequence variable array header width mismatch')
        data, number, capacity = struct.unpack('<Qii', header)
        if not 0 <= number <= capacity <= 64 or (number and not data):
            raise RuntimeError('Sequence variable array bounds rejected')
        element = q.obj(q.r.u64(prop['address'] + q.r.profile['struct_property_struct']))
        meta = q.read(element['address'] + 0x70, 8)
        q.metadata[(element['address'] + 0x70, 8)] = meta
        if (element['outer'] != prop['address'] or q.r.class_name(element) != 'ObjectProperty'
                or struct.unpack('<II', meta) != (1, 8)):
            raise RuntimeError('Sequence variable element type/layout mismatch')
        content = q.read(data, number * 8) if number else b''
        if number and q.read(data, len(content)) != content:
            raise RuntimeError('Sequence variable contents changed')
        variables = []
        for (address,) in struct.iter_unpack('<Q', content):
            if not address:
                continue
            value = q.obj(address)
            kind = q.obj(value['cls'])['name']
            row = dict(identity=value, kind=kind)
            if kind == 'SeqVar_Bool':
                row['bValue'] = q.scalar(value, 'bValue', 'IntProperty', '<i')
            if kind == 'SeqVar_Object':
                row['ObjValue'] = q.pointer(value, 'ObjValue')
            if kind == 'SeqVar_Named':
                p = q.prop(value, 'FindVarName', 'NameProperty')
                row['FindVarName'] = q.decode(p['type'], q.value(value, p), p['address'])
            variables.append(row)
        result.append(dict(description=q.decode(description['type'], txt, description['address']), variables=variables))
    return result


def object_path(q, obj):
    """A bounded semantic path; object addresses never identify another run."""
    result, seen = [], set()
    while obj:
        if obj['address'] in seen or len(result) >= 16:
            raise RuntimeError('Checkpoint sequence outer path is cyclic or too deep')
        seen.add(obj['address'])
        result.append([obj['name'], obj['number'], q.obj(obj['cls'])['name']])
        obj = q.obj(obj['outer']) if obj['outer'] else None
    return list(reversed(result))


def parent_graph(observer, seed_address, *, as_sequence=False, max_nodes=256):
    if type(max_nodes) is not int or not 1 <= max_nodes <= 512:
        raise ValueError('Focused sequence budget must stay within 1..512 objects')
    source = observer.sample()
    if source['errors'] or not controllable(source):
        raise RuntimeError('Sequence graph requires current controllable state')
    q = Query(observer)
    seed = q.obj(seed_address)
    if not q.s.derives(seed['cls'], 'SequenceOp'):
        raise RuntimeError('Sequence seed is not a live operation')
    parent = seed if as_sequence and q.s.derives(seed['cls'], 'Sequence') else q.pointer(seed, 'ParentSequence')
    if not parent or not q.s.derives(parent['cls'], 'Sequence'):
        raise RuntimeError('Sequence seed has no supported parent')
    world = q.pointer(q.obj(observer.sampler.pawn), 'WorldInfo')
    inner, stride, contents, count = q.array(parent, 'SequenceObjects', 2048)
    if q.r.class_name(inner) != 'ObjectProperty' or stride != 8:
        raise RuntimeError('Sequence object-array element type mismatch')
    nodes, edges, errors = {}, [], []
    if count > max_nodes:
        raise RuntimeError('Parent sequence exceeds focused ' + str(max_nodes) + '-object budget')
    for (address,) in struct.iter_unpack('<Q', contents):
        if not address:
            continue
        obj = q.obj(address)
        kind = q.obj(obj['cls'])['name']
        key = hex(address)
        row = dict(identity=obj, kind=kind, labels=labels(q, obj, kind))
        nodes[key] = row
        if not q.s.derives(obj['cls'], 'SequenceOp'):
            continue
        if q.pointer(obj, 'ParentSequence') != parent:
            raise RuntimeError('Sequence parent backlink mismatch')
        for link in links(q, obj):
            edges.append(dict(source=key, target=hex(link['target']['address']),
                              output_index=link['output_index'], input_index=link['input_index'],
                              description=link['description']))
        if q.s.derives(obj['cls'], 'SequenceEvent'):
            row['trigger_count'] = q.scalar(obj, 'TriggerCount', 'IntProperty', '<i')
            row['max_trigger_count'] = q.scalar(obj, 'MaxTriggerCount', 'IntProperty', '<i')
            prop = q.prop(obj, 'bEnabled', 'BoolProperty')
            row['enabled'] = q.decode(prop['type'], q.value(obj, prop), prop['address'])
            origin = q.pointer(obj, 'Originator')
            if origin and q.s.derives(origin['cls'], 'Actor') and q.pointer(origin, 'WorldInfo') == world:
                row['originator'] = origin
                row['position'] = q.position(origin)
                row['same_world_verified'] = True
                if kind == 'SeqEvent_Touch':
                    prop = q.prop(obj, 'bPlayerOnly', 'BoolProperty')
                    row['player_only'] = q.decode(prop['type'], q.value(obj, prop), prop['address'])
                    inner, stride, raw, count = q.array(obj, 'ClassProximityTypes', 32)
                    if q.r.class_name(inner) != 'ClassProperty' or stride != 8:
                        raise RuntimeError('Touch proximity class metadata rejected')
                    proximity = [q.obj(address) for (address,) in struct.iter_unpack('<Q', raw) if address]
                    if any(q.obj(value['cls'])['name'] != 'Class' for value in proximity):
                        raise RuntimeError('Touch proximity entry is not a class')
                    row['player_proximity_allowed'] = any(q.s.derives(q.obj(observer.sampler.pawn)['cls'], value['name'])
                                                          for value in proximity)
                    row['bounds'] = component_bounds(q, origin)
                    if row['bounds']:
                        component = q.obj(row['bounds']['component']['address'])
                        if q.s.derives(component['cls'], 'CylinderComponent'):
                            row['trigger_shape'] = dict(kind='cylinder',
                                radius=min(row['bounds']['extent'][:2]), half_height=row['bounds']['extent'][2])
    ports = []
    inner, stride, contents, count = q.array(parent, 'InputLinks', 128)
    typ = structure(q, inner, 'SeqOpInputLink')
    for index in range(count):
        raw = contents[index * stride:(index + 1) * stride]
        prop, value = field(q, typ, raw, 'LinkDesc', 'StrProperty')
        ports.append(dict(index=index, description=q.decode(prop['type'], value, prop['address']),
                          linked_op=object_field(q, typ, raw, 'LinkedOp')))
    parent_path = object_path(q, parent)
    q.finish()
    return dict(schema=1, game_identity=observer.identities['game_identity'], source_sample=source,
                world_identity=world, seed=seed, parent=parent, parent_path=parent_path,
                disk_game_sha256=q.r.profile['sha256'], module_base=hex(q.r.base),
                nodes=nodes, edges=edges, parent_input_ports=ports,
                errors=errors, atomic_snapshot=False, consistent_metadata_reread=True,
                max_nodes=max_nodes,
                active_execution_proven=False, limits='Authored/current link observations only; conditions and execution order are not inferred.')


def capture_choice_goals(observer):
    """Observe unused touch events authored upstream of a path-choice action."""
    operations = find_operations(observer, 'SeqAct_DisplayPathChoice')
    if len(operations['matches']) > 8:
        raise RuntimeError('Path-choice operation count exceeds focused budget')
    parents, graphs, rows, world = set(), [], [], None
    for operation in operations['matches']:
        parent = operation.get('parent')
        if not parent or parent['address'] in parents:
            continue
        parents.add(parent['address'])
        graph = parent_graph(observer, operation['identity']['address'])
        if world is not None and graph['world_identity'] != world:
            raise RuntimeError('Path-choice world changed during capture')
        world = graph['world_identity']
        graphs.append(graph)
        upstream = {key for key, row in graph['nodes'].items() if row['kind'] == 'SeqAct_DisplayPathChoice'}
        for _ in range(12):
            expanded = upstream | {edge['source'] for edge in graph['edges'] if edge['target'] in upstream}
            if expanded == upstream:
                break
            upstream = expanded
        for key in upstream:
            row = graph['nodes'].get(key)
            if row and row['kind'] == 'SeqEvent_Touch':
                rows.append(dict(row, parent_sequence=graph['parent'],
                                 authorship='Touch event has a bounded authored link path to DisplayPathChoice'))
    end = observer.sample()
    source = operations['source_sample']
    if (end['errors'] or not controllable(end) or end.get('pawn_identity') != source.get('pawn_identity')
            or end['native']['world_generation'] != source['native']['world_generation']):
        raise RuntimeError('Path-choice control context changed')
    return dict(schema=1, game_identity=operations['game_identity'], source_sample=source,
                end_sample=end, world_identity=world, rows=rows, graphs=graphs,
                consistent_metadata_reread=True, atomic_snapshot=False, mission_order_verified=False)


def upstream_nodes(graph, targets):
    upstream = set(targets)
    for _ in range(12):
        expanded = upstream | {edge['source'] for edge in graph['edges'] if edge['target'] in upstream}
        if expanded == upstream:
            break
        upstream = expanded
    return upstream


def checkpoint_touch_rows(graph, targets, read_child, *, depth=0):
    """Follow authored output ports only; never evaluate gates or run operations."""
    upstream = upstream_nodes(graph, targets)
    result = [dict(row, parent_sequence=graph['parent']) for key, row in graph['nodes'].items()
              if key in upstream and row['kind'] == 'SeqEvent_Touch']
    if depth >= 2:
        return result
    for key, row in graph['nodes'].items():
        if key not in upstream or row['kind'] != 'Sequence':
            continue
        outputs = {edge['description'] for edge in graph['edges']
                   if edge['source'] == key and edge['target'] in upstream}
        child = read_child(row['identity']['address'])
        finishes = {key for key, value in child['nodes'].items()
                    if value['kind'] == 'SeqAct_FinishSequence' and
                    value.get('labels', {}).get('OutputLabel') in outputs}
        result.extend(checkpoint_touch_rows(child, finishes, read_child, depth=depth + 1))
    return result


class CheckpointCaptureBudget(RuntimeError):
    pass


def checkpoint_history_key(record):
    if not isinstance(record, dict):
        return None
    path, name, version = record.get('root_path'), record.get('event_name'), record.get('game_sha256')
    if (not isinstance(path, list) or not 1 <= len(path) <= 16 or
            not all(isinstance(p, list) and len(p) == 3 and isinstance(p[0], str) and
                    type(p[1]) is int and isinstance(p[2], str) for p in path) or
            not isinstance(name, dict) or not isinstance(name.get('name'), str) or
            type(name.get('number')) is not int or not isinstance(version, str) or
            not ((type(record.get('observed_count')) is int and record['observed_count'] > 0) or
                 record.get('current_checkpoint_verified') is True)):
        return None
    return (version, tuple(tuple(p) for p in path), name['name'], name['number'])


def observed_checkpoint_signals(graph):
    upstream = upstream_nodes(graph, {key for key, row in graph['nodes'].items()
                                      if row['kind'] == 'SeqAct_WarCheckPoint'})
    result = []
    for key, row in graph['nodes'].items():
        if key not in upstream or row['kind'] != 'SeqEvent_RemoteEvent':
            continue
        record = dict(root_path=graph.get('parent_path'), event_name=row.get('labels', {}).get('EventName'),
                      game_sha256=graph.get('disk_game_sha256'), observed_count=row.get('trigger_count'))
        if checkpoint_history_key(record):
            result.append(record)
    return result


def checkpoint_operations_from_cursor(operations, cursor, context):
    if not isinstance(cursor, dict) or cursor.get('context') != context:
        return operations
    start = next((index for index, row in enumerate(operations)
                  if row['identity'] == cursor.get('operation')), 0)
    return operations[start:]


def capture_checkpoint_goals(observer, *, resume_cursor=None, completed_signals=()):
    """Observe unused loaded touches linked to an unused checkpoint signal."""
    source = observer.sample()
    if source['errors'] or not controllable(source):
        raise RuntimeError('Checkpoint goal capture requires current control')
    from world_state import streaming_levels
    q = Query(observer)
    world = q.pointer(q.obj(observer.sampler.pawn), 'WorldInfo')
    levels = streaming_levels(q, world)
    loaded = {world['outer']} | {row['loaded_level']['address'] for row in levels['loaded']
                                 if row['bIsVisible']}
    q.finish(allow_registry_growth=True)
    context = dict(game_identity=observer.identities['game_identity'], world_identity=world,
                   world_generation=source['native']['world_generation'],
                   pawn_identity=source['pawn_identity'], loaded_levels=sorted(loaded))
    if not isinstance(completed_signals, (list, tuple)) or len(completed_signals) > 512:
        raise RuntimeError('Checkpoint history is outside its bounded record limit')
    completed = {key for row in completed_signals if (key := checkpoint_history_key(row)) is not None}
    context['checkpoint_history_sha256'] = hashlib.sha256(repr(sorted(completed)).encode()).hexdigest()
    started = time.monotonic()
    graphs, errors = {}, []

    def read_child(address):
        if address not in graphs:
            if len(graphs) >= 8 or time.monotonic() - started > 16:
                raise CheckpointCaptureBudget('Checkpoint link capture exceeded eight graphs or16 seconds')
            graph = parent_graph(observer, address, as_sequence=True, max_nodes=512)
            if graph['world_identity'] != world:
                raise RuntimeError('Checkpoint graph world changed')
            graphs[address] = graph
        return graphs[address]

    checkpoints = find_operations(observer, 'SeqAct_WarCheckPoint')
    parents = {row['parent']['address'] for row in checkpoints['matches'] if row.get('parent')}
    if len(parents) > 4:
        errors.append('Checkpoint parents truncated to the focused four-parent budget')
    signals, observed_history, skipped_history = set(), [], []
    for address in sorted(parents)[:4]:
        try:
            graph = read_child(address)
        except (OSError, RuntimeError, ValueError) as error:
            errors.append(dict(parent=hex(address), message=str(error)))
            continue
        upstream = upstream_nodes(graph, {key for key, row in graph['nodes'].items()
                                          if row['kind'] == 'SeqAct_WarCheckPoint'})
        observed_history.extend(observed_checkpoint_signals(graph))
        names = [row.get('labels', {}).get('EventName') for row in graph['nodes'].values()
                 if row['kind'] == 'SeqEvent_RemoteEvent']
        for key, row in graph['nodes'].items():
            name = row.get('labels', {}).get('EventName')
            if (key in upstream and row['kind'] == 'SeqEvent_RemoteEvent' and
                    row.get('enabled') is True and row.get('trigger_count') == 0 and
                    name and names.count(name) == 1):
                history = dict(root_path=graph.get('parent_path'), event_name=name,
                               game_sha256=graph.get('disk_game_sha256'), current_checkpoint_verified=True)
                if checkpoint_history_key(history) in completed:
                    skipped_history.append(dict(root_path=graph.get('parent_path'), event_name=name,
                                                game_sha256=graph.get('disk_game_sha256'),
                                                live_trigger_count=row['trigger_count'],
                                                basis='Scoped verified fixture checkpoint history'))
                    continue
                signals.add((name['name'], name['number']))
    activators = find_operations(observer, 'SeqAct_ActivateRemoteEvent') if signals else dict(matches=[])
    rows, next_cursor = {}, None
    operations = [row for row in activators['matches'] if row.get('parent') and
                  row['labels'].get('EventName') and
                  (row['labels']['EventName']['name'], row['labels']['EventName']['number']) in signals]
    for operation in checkpoint_operations_from_cursor(operations, resume_cursor, context):
        name = operation['labels']['EventName']
        try:
            graph = read_child(operation['parent']['address'])
            candidates = checkpoint_touch_rows(graph, {hex(operation['identity']['address'])}, read_child)
            for row in candidates:
                actor = row.get('originator')
                if (not actor or actor['outer'] not in loaded or row.get('same_world_verified') is not True
                        or row.get('enabled') is not True or row.get('trigger_count') != 0 or not row.get('bounds')):
                    continue
                row = dict(row, checkpoint_signal=name, authorship='Loaded touch has a bounded authored link path to an unused checkpoint signal')
                rows[row['identity']['address']] = row
        except CheckpointCaptureBudget as error:
            next_cursor = dict(context=context, operation=operation['identity'])
            errors.append(dict(operation=operation['identity'], message=str(error), resumable=True))
            break
        except (OSError, RuntimeError, ValueError) as error:
            errors.append(dict(operation=operation['identity'], message=str(error)))
    end = observer.sample()
    if (end['errors'] or not controllable(end) or end.get('pawn_identity') != source.get('pawn_identity')
            or end['native']['world_generation'] != source['native']['world_generation']):
        raise RuntimeError('Checkpoint goal capture crossed control context')
    return dict(schema=1, game_identity=observer.identities['game_identity'], world_identity=world,
                source_sample=source, end_sample=end, rows=list(rows.values()), graphs=list(graphs.values()),
                errors=errors, checkpoint_signals=sorted(signals), consistent_metadata_reread=True,
                completed_checkpoint_signals_observed=observed_history,
                completed_checkpoint_signals_skipped=skipped_history,
                next_cursor=next_cursor, operation_scan_complete=next_cursor is None,
                scan_scope='Bounded current roots; unsupported/truncated graphs remain in errors',
                atomic_snapshot=False, mission_order_verified=False, execution_proven=False)


def nearby_streaming_touch(row, position, loaded):
    actor, bounds = row.get('originator'), row.get('bounds')
    if (not actor or actor.get('outer') not in loaded or not bounds or
            row.get('same_world_verified') is not True or row.get('enabled') is not True or
            row.get('trigger_count') != 0 or row.get('player_only') is not True or
            row.get('player_proximity_allowed') is not True):
        return False
    center = bounds.get('center')
    return (isinstance(center, (list, tuple)) and len(center) == 3 and
            all(type(v) in (int, float) and math.isfinite(v) for v in center) and
            math.dist(center, position) <= 1800 and abs(center[2] - position[2]) <= 300)


def capture_streaming_goals(observer, *, resume_cursor=None):
    """Nearby player touches authored upstream of level-visibility waits."""
    source = observer.sample()
    if source['errors'] or not controllable(source):
        raise RuntimeError('Streaming goal capture requires current control')
    from world_state import streaming_levels
    q = Query(observer)
    world = q.pointer(q.obj(observer.sampler.pawn), 'WorldInfo')
    levels = streaming_levels(q, world)
    loaded = {world['outer']} | {r['loaded_level']['address'] for r in levels['loaded'] if r['bIsVisible']}
    q.finish(allow_registry_growth=True)
    context = dict(game_identity=observer.identities['game_identity'], world_identity=world,
                   world_generation=source['native']['world_generation'], pawn_identity=source['pawn_identity'],
                   loaded_levels=sorted(loaded))
    operations = find_operations(observer, 'SeqAct_WaitForLevelsVisible')
    started, graphs, rows, errors, cursor = time.monotonic(), {}, {}, [], None

    def read_child(address):
        if address not in graphs:
            if len(graphs) >= 8 or time.monotonic() - started > 16:
                raise CheckpointCaptureBudget('Streaming links exceeded eight graphs or16 seconds')
            graph = parent_graph(observer, address, as_sequence=True, max_nodes=512)
            if graph['world_identity'] != world:
                raise RuntimeError('Streaming graph world changed')
            graphs[address] = graph
        return graphs[address]

    for operation in checkpoint_operations_from_cursor(operations['matches'], resume_cursor, context):
        if not operation.get('parent'):
            continue
        try:
            graph = read_child(operation['parent']['address'])
            for row in checkpoint_touch_rows(graph, {hex(operation['identity']['address'])}, read_child):
                if nearby_streaming_touch(row, source['pose']['position'], loaded):
                    rows[row['identity']['address']] = dict(row, authorship='Nearby player touch linked to a level visibility wait')
        except CheckpointCaptureBudget as error:
            cursor = dict(context=context, operation=operation['identity'])
            errors.append(dict(operation=operation['identity'], message=str(error), resumable=True))
            break
        except (OSError, RuntimeError, ValueError) as error:
            errors.append(dict(operation=operation['identity'], message=str(error)))
    end = observer.sample()
    if (end['errors'] or not controllable(end) or end.get('pawn_identity') != source['pawn_identity'] or
            end['native']['world_generation'] != source['native']['world_generation']):
        raise RuntimeError('Streaming touch capture crossed control context')
    return dict(schema=1, game_identity=observer.identities['game_identity'], source_sample=source, end_sample=end,
                world_identity=world, rows=list(rows.values()), graphs=list(graphs.values()), errors=errors,
                consistent_metadata_reread=True, next_cursor=cursor, operation_scan_complete=cursor is None,
                scan_scope='Nearby loaded player-only touches; authored gates and physical clearance remain unverified',
                atomic_snapshot=False, mission_order_verified=False, execution_proven=False)


def touch_count(observer, event):
    q = Query(observer)
    current = q.obj(event['address'])
    if current != event or q.obj(current['cls'])['name'] != 'SeqEvent_Touch':
        raise RuntimeError('Touch event identity/class changed')
    origin = q.pointer(current, 'Originator')
    world = q.pointer(q.obj(observer.sampler.pawn), 'WorldInfo')
    if not origin or not q.s.derives(origin['cls'], 'Actor') or q.pointer(origin, 'WorldInfo') != world:
        raise RuntimeError('Touch event originator world changed')
    count = q.scalar(current, 'TriggerCount', 'IntProperty', '<i')
    q.finish()
    return count


def choose_choice_goal(graph, hints, position, *, avoid=(), allow_forced=False):
    if (graph.get('consistent_reread') is not True or hints.get('consistent_metadata_reread') is not True
            or graph['world_identity'] != hints['world_identity'] or graph['game_identity'] != hints['game_identity']
            or graph['source_sample']['native']['world_generation'] != hints['source_sample']['native']['world_generation']):
        return None
    nodes = graph['nodes']
    nearby = [(math.dist(position, row['position']), key) for key, row in nodes.items()
              if abs(position[2] - row['position'][2]) <= 180]
    if not nearby or min(nearby)[0] > 400:
        return None
    start = min(nearby)[1]
    adjacency = {}
    for edge in graph['edges']:
        allowed = walking_edge(edge, allow_forced=allow_forced)
        if allowed and edge['start'] in nodes and edge['end'] in nodes:
            adjacency.setdefault(edge['start'], []).append(edge['end'])
    reachable, pending = {start}, [start]
    while pending:
        for target in adjacency.get(pending.pop(), ()):
            if target not in reachable:
                reachable.add(target)
                pending.append(target)
    candidates = []
    generation = hints['source_sample']['native']['world_generation']
    for row in hints['rows']:
        event, bounds = row.get('identity'), row.get('bounds')
        if not event or not bounds or row.get('same_world_verified') is not True:
            continue
        key = (generation, event['address'], event['internal_index'], event['number'])
        if key in avoid or row.get('enabled') is not True or row.get('trigger_count') != 0:
            continue
        center, extent = bounds['center'], bounds['extent']
        if (len(center) != 3 or len(extent) != 3 or not all(math.isfinite(v) for v in center + extent)
                or not all(v > 0 for v in extent)):
            continue
        points = []
        for node in reachable:
            pos = nodes[node]['position']
            if abs(pos[2] - center[2]) > extent[2]:
                continue
            target = [max(c - e + min(20, e / 2), min(c + e - min(20, e / 2), v))
                      for v, c, e in zip(pos, center, extent)]
            shape = row.get('trigger_shape') or {}
            if shape.get('kind') == 'cylinder':
                radius = shape.get('radius')
                if type(radius) not in (int, float) or not math.isfinite(radius) or radius <= 0:
                    continue
                radius -= min(20, radius / 2)
                dx, dy = pos[0] - center[0], pos[1] - center[1]
                length = math.hypot(dx, dy)
                scale = min(1, radius / length) if length else 1
                target[:2] = [center[0] + dx * scale, center[1] + dy * scale]
            gap = math.dist(pos, target)
            if gap <= 180:
                points.append((gap, math.dist(position, target), node, target))
        if points:
            gap, distance, node, target = min(points, key=lambda point: (point[0] + point[1], point[0], point[1], point[2]))
            score = distance + gap
            candidates.append((score, event['address'], dict(event=event, position=target,
                arrival_radius=20, goal_kind='touch', reachable_node=node, node_gap=gap,
                bounds=bounds, parent_sequence=row['parent_sequence'], mission_order_verified=False,
                forced_hints_required=allow_forced, walkability_verified=False)))
    if candidates:
        return min(candidates, key=lambda item: item[:2])[2]
    return choose_choice_goal(graph, hints, position, avoid=avoid, allow_forced=True) if not allow_forced else None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--seed', type=lambda value: int(value, 0), required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--sequence', action='store_true', help='Inspect the seed sequence itself')
    args = parser.parse_args()
    from live import Observer
    observer = Observer(args.session)
    try:
        result = parent_graph(observer, args.seed, as_sequence=args.sequence)
        atomic_json(args.output, result)
        incoming = [dict(edge, node=result['nodes'].get(edge['source'])) for edge in result['edges']
                    if edge['target'] == hex(result['seed']['address'])]
        print(json.dumps(dict(output=str(args.output), parent=result['parent'], nodes=len(result['nodes']),
                              edges=len(result['edges']), incoming=incoming, ports=result['parent_input_ports'])))
    finally:
        observer.close()


if __name__ == '__main__':
    main()
