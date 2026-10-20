"""Read-only UE3 UI inventory through the version-gated reflected registries.

Discovery output is evidence, not permission to send input. No fixed heap
addresses are consumed. Candidate arrays are bounded and their object entries
must belong to the current registry before any identity is accepted.
"""
import argparse
import json
from pathlib import Path
import struct
import time
from resolve_camera import Resolver

ROOT = Path(__file__).resolve().parents[2]

def registered(resolver, address):
    item = resolver.identity(address)
    if not item or item['internal_index'] >= len(resolver.object_pointers):
        error = RuntimeError('Object outside registry')
        # Same failed identity/inventory observation; no extra remote reads and
        # no change to the existing rejection predicate, message or exception type.
        error.registration_failure = dict(address=hex(address) if type(address) is int else None,
            item_none=item is None, item_empty=not bool(item),
            internal_index=item.get('internal_index') if item else None,
            inventory_length=len(resolver.object_pointers))
        raise error
    if resolver.object_pointers[item['internal_index']] != address:
        raise RuntimeError('Object registry identity changed')
    if resolver.u64(resolver.objects_data + item['internal_index'] * 8) != address:
        raise RuntimeError('Live object registry slot changed')
    return item

def describe(resolver, address):
    obj = registered(resolver, address)
    return dict(address=hex(address), name=obj['name'], number=obj['number'],
                cls=resolver.class_name(obj), outer=hex(obj['outer']))

def fields(resolver, obj):
    owner = obj['cls']
    result, seen = [], set()
    while owner and owner not in seen and len(seen) < 40:
        seen.add(owner)
        for field in resolver.fields(owner):
            kind = resolver.class_name(field)
            if not kind.endswith('Property'):
                continue
            offset = resolver.u32(field['address'] + resolver.profile['property_offset_internal'])
            if offset > 1024 * 1024:
                raise RuntimeError('Implausible reflected UI property offset')
            entry = dict(name=field['name'], type=kind, offset=hex(offset),
                         owner=resolver.identity(owner)['name'], property_address=hex(field['address']))
            try:
                raw = resolver.read(obj['address'] + offset, 16)
                entry['raw16'] = raw.hex()
                if kind in ('ObjectProperty', 'ComponentProperty', 'ClassProperty'):
                    ptr = struct.unpack_from('<Q', raw)[0]
                    entry['object'] = describe(resolver, ptr) if ptr else None
                elif kind == 'NameProperty':
                    index, number = struct.unpack_from('<II', raw)
                    entry['name_value'] = [resolver.name(index), number]
                elif kind in ('IntProperty', 'ByteProperty'):
                    entry['value'] = struct.unpack_from('<i', raw)[0] if kind == 'IntProperty' else raw[0]
                elif kind == 'StrProperty':
                    ptr, count, capacity = struct.unpack('<Qii', raw)
                    if not 0 <= count <= capacity <= 4096 or (count and not ptr):
                        raise RuntimeError('Invalid bounded FString')
                    entry['string_value'] = resolver.read(ptr, count*2).decode('utf-16-le').rstrip('\0') if count else ''
                elif kind == 'ArrayProperty':
                    # Inspect the candidate UArrayProperty tail independently;
                    # accepting this layout requires a registered inner field.
                    try:
                        inner = resolver.u64(field['address'] + resolver.profile['struct_property_struct'])
                        entry['inner_field_candidate'] = describe(resolver, inner)
                        if entry['inner_field_candidate']['cls'] == 'StructProperty':
                            structure = resolver.struct_type({'address': inner, 'type': 'StructProperty'})
                            entry['inner_struct'] = describe(resolver, structure)
                            entry['inner_struct_fields'] = [
                                dict(name=f['name'], type=resolver.class_name(f),
                                     offset=hex(resolver.u32(f['address'] + resolver.profile['property_offset_internal'])))
                                for f in resolver.fields(structure)
                                if resolver.class_name(f).endswith('Property')]
                    except (RuntimeError, OSError, ValueError, UnicodeError):
                        pass
                    ptr, count, capacity = struct.unpack('<Qii', raw)
                    entry['array_header'] = dict(data=hex(ptr), count=count, capacity=capacity)
                    if ptr and count > 0 and 'inner_struct_fields' in entry:
                        entry['first_struct_fields'] = []
                        for member in entry['inner_struct_fields']:
                            if member['type'] == 'ObjectProperty':
                                value = resolver.u64(ptr + int(member['offset'], 16))
                                entry['first_struct_fields'].append({**member, 'object': describe(resolver, value) if value else None})
                    if ptr and 0 < count <= capacity <= 128:
                        pointers = struct.unpack('<' + 'Q' * count, resolver.read(ptr, count * 8))
                        try:
                            entry['registered_object_array_candidate'] = [describe(resolver, p) for p in pointers]
                        except (RuntimeError, OSError, ValueError, UnicodeError):
                            pass
            except (RuntimeError, OSError, ValueError, UnicodeError) as error:
                entry['value_error'] = str(error)
            result.append(entry)
        owner = resolver.u64(owner + resolver.profile['field_super'])
    return result

def inspect(pid):
    profile = json.loads((ROOT / 'tools/package-inspector/reloaded-profile.json').read_text())
    resolver = Resolver(pid, profile)
    began = time.monotonic()
    try:
        resolver.prepare()
        clients, scenes, skipped = [], [], 0
        for index, address in enumerate(resolver.object_pointers):
            if not address:
                continue
            try:
                obj = registered(resolver, address)
                cls = resolver.class_name(obj)
                if obj['name'].startswith('Default__') or cls == 'Class':
                    continue
                is_client = 'SceneClient' in cls or cls.endswith('UIInteraction')
                is_scene = cls.startswith(('WarUIScene', 'UIScene'))
                if is_client:
                    clients.append({**describe(resolver, address), 'properties': fields(resolver, obj)})
                elif is_scene:
                    scenes.append(describe(resolver, address))
            except (RuntimeError, OSError, ValueError, UnicodeError):
                skipped += 1
        if resolver.read(resolver.objects_header, 16) != struct.pack('<Qii', resolver.objects_data, resolver.object_count, resolver.object_capacity):
            raise RuntimeError('Object registry changed during UI discovery; retry')
        active_details = []
        for client in clients:
            for prop in client['properties']:
                if prop['name'] != 'ActiveScenes':
                    continue
                for scene in prop.get('registered_object_array_candidate', []):
                    obj = registered(resolver, int(scene['address'], 16))
                    active_details.append({**scene, 'properties': fields(resolver, obj)})
        focus_chain = []
        if active_details:
            current = int(active_details[-1]['address'], 16)
            visited = set()
            while current and current not in visited and len(visited) < 16:
                visited.add(current)
                obj = registered(resolver, current)
                props = fields(resolver, obj)
                focus_chain.append({**describe(resolver, current), 'properties': props})
                focus_prop = next((p for p in props if p['name'] == 'FocusControls'), {})
                focused = next((f['object'] for f in focus_prop.get('first_struct_fields', []) if f['name'] == 'FocusedControl'), None)
                if not focused:
                    break
                current = int(focused['address'], 16)
        return dict(pid=pid, executable=resolver.path, sha256=profile['sha256'],
                    module_base=hex(resolver.base), object_count=resolver.object_count,
                    clients=clients, active_scene_details=active_details, focus_chain_candidate=focus_chain,
                    loaded_scene_candidates=scenes, skipped_objects=skipped,
                    elapsed_seconds=round(time.monotonic()-began, 3), read_only=True,
                    limitation='Loaded scene existence alone does not prove activity or focus.')
    finally:
        resolver.p.close()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    result = inspect(args.pid)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in result.items() if k not in ('clients', 'loaded_scene_candidates', 'active_scene_details', 'focus_chain_candidate')}))
    print(json.dumps({'clients': [{k: v for k, v in x.items() if k != 'properties'} for x in result['clients']],
                      'scene_count': len(result['loaded_scene_candidates']), 'output': str(args.out)}))

if __name__ == '__main__':
    main()
