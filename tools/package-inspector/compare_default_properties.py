"""Read UE3 tagged defaults from analysis copies, using export/name inventories.

This is a bounded tag reader for the observed 835/76 packages, not a general
Unreal package editor. It writes JSON only; unknown tag layouts fail explicitly.
"""
import argparse
import csv
import json
import pathlib
import struct


def table(path):
    with path.open(encoding='utf-8-sig', newline='') as stream:
        return list(csv.DictReader(stream, delimiter='\t'))


def defaults(data, names, exports, start_offset=4, depth=0):
    cursor = start_offset
    properties = {}

    def read(fmt):
        nonlocal cursor
        size = struct.calcsize(fmt)
        if cursor + size > len(data): raise ValueError('Tag exceeds export bounds')
        values = struct.unpack_from(fmt, data, cursor)
        cursor += size
        return values[0] if len(values) == 1 else values

    def fname():
        index, number = read('<ii')
        if index not in names or number < 0: raise ValueError('Invalid name reference')
        return names[index] + ('_' + str(number - 1) if number else '')

    for _ in range(10000):
        start = cursor
        name = fname()
        if name == 'None': return properties, cursor, len(data)-cursor
        kind = fname()
        if not kind.endswith('Property'): raise ValueError('Unexpected tag type '+kind)
        size, array_index = read('<ii')
        if size < 0 or array_index < 0: raise ValueError('Invalid tag size/index')
        extra = None
        if kind == 'StructProperty': extra = fname()
        elif kind == 'BoolProperty':
            extra = read('<I')  # Reloaded 835/76 serializes tag booleans as 32 bits
            if extra not in (0,1): raise ValueError('Invalid bool tag')
        if cursor+size > len(data): raise ValueError('Property exceeds export bounds')
        payload = data[cursor:cursor+size]
        cursor += size
        value = {'hex': payload.hex()}
        if kind == 'FloatProperty' and size == 4: value = struct.unpack('<f', payload)[0]
        elif kind == 'IntProperty' and size == 4: value = struct.unpack('<i', payload)[0]
        elif kind == 'BoolProperty' and size == 0: value = bool(extra)
        elif kind in ('NameProperty','ByteProperty') and size == 8:
            index, number = struct.unpack('<ii', payload)
            value = names.get(index, 'INVALID') + ('_'+str(number-1) if number else '')
        elif kind in ('ObjectProperty','ComponentProperty') and size == 4:
            index = struct.unpack('<i', payload)[0]
            value = {'index':index, 'object':exports.get(index, 'None' if index == 0 else 'import/unresolved')}
        elif kind == 'StructProperty' and extra == 'Vector' and size == 12:
            value = list(struct.unpack('<3f', payload))
        elif kind == 'StructProperty' and extra == 'Rotator' and size == 12:
            value = list(struct.unpack('<3i', payload))
        elif kind == 'StructProperty' and depth < 4:
            try:
                children, end, tail = defaults(payload,names,exports,0,depth+1)
                if end == len(payload): value = {k:v['value'] for k,v in children.items()}
            except (ValueError,struct.error): pass
        elif kind == 'StrProperty':
            if len(payload)>=4:
                length=struct.unpack_from('<i',payload)[0]
                width=2 if length<0 else 1
                if len(payload)==4+abs(length)*width:
                    value=payload[4:].decode('utf-16-le' if length<0 else 'latin-1').rstrip('\0')
        key = name + ('['+str(array_index)+']' if array_index else '')
        if key in properties:
            # Some shipped objects repeat an array index; preserve tag order.
            ordinal=2
            while key+'#'+str(ordinal) in properties: ordinal+=1
            key += '#'+str(ordinal)
        properties[key] = {'type':kind,'struct_or_enum':extra if kind != 'BoolProperty' else None,
                           'value':value,'offset':start,'serialized_size':cursor-start}
    raise ValueError('Missing property terminator')


def locate_defaults(data,names,exports):
    # Actors/components may prepend state/component metadata. Locate a complete
    # tag stream within a bounded prefix; record the offset, never infer values
    # from failed or partial parsing. Nested structs use an explicit offset.
    candidates=[]
    for start in range(0,min(80,len(data)-24)):
        index,number,type_index,type_number=struct.unpack_from('<4i',data,start)
        if number or type_number or index not in names or type_index not in names: continue
        if not names[type_index].endswith('Property'): continue
        try:
            properties,end,tail=defaults(data,names,exports,start)
            candidates.append((len(properties),-start,properties,end,tail))
        except (ValueError,struct.error): pass
    if not candidates: raise ValueError('No complete tagged property stream in bounded prefix')
    count,negative_start,properties,end,tail=max(candidates,key=lambda x:(x[0],x[1]))
    return properties,end,tail,-negative_start


def compare(root, package, include_classes=()):
    inventories = {}
    for side in ('original','mod'):
        rows = table(root/(side+'-'+package+'.tsv'))
        names = {int(r['Index']):r['Name'] for r in table(root/(side+'-'+package+'.tsv.names.tsv'))}
        inventories[side] = (rows, names, {int(r['Index']):r['Path'] for r in rows})
    original = {r['Path']:r for r in inventories['original'][0]}
    modified = {r['Path']:r for r in inventories['mod'][0]}
    result = {'package':package, 'names_identical':inventories['original'][1] == inventories['mod'][1],
              'changed_defaults':[], 'errors':[]}
    source_root = root.parent/'packages/decompressed'
    with (source_root/(package+'.u')).open('rb') as a, (root/'decompressed'/(package+'.u')).open('rb') as b:
        for path in sorted(original.keys() & modified.keys()):
            old, new = original[path], modified[path]
            if ('Default__' not in path and new['Class'] not in include_classes) or old['SHA256'] == new['SHA256']: continue
            try:
                parsed = []
                for side, row, stream in [('original',old,a),('mod',new,b)]:
                    stream.seek(int(row['Offset'])); raw = stream.read(int(row['Size']))
                    parsed.append(locate_defaults(raw,inventories[side][1],inventories[side][2]))
                before, after = parsed[0][0], parsed[1][0]
                changes = {}
                for key in sorted(before.keys() | after.keys()):
                    clean = lambda tag: {k:v for k,v in tag.items() if k not in ('offset','serialized_size')} if tag else None
                    x,y = clean(before.get(key)),clean(after.get(key))
                    if x != y: changes[key] = {'original':x, 'mod':y}
                result['changed_defaults'].append({'path':path,'class':new['Class'],'properties':changes,
                    'original_property_start':parsed[0][3],'mod_property_start':parsed[1][3],
                    'original_property_end':parsed[0][1],'mod_property_end':parsed[1][1],
                    'original_unparsed_tail':parsed[0][2],'mod_unparsed_tail':parsed[1][2]})
            except (ValueError, struct.error) as error:
                result['errors'].append({'path':path,'error':str(error)})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=pathlib.Path,default=pathlib.Path('analysis/first-person'))
    parser.add_argument('--output',type=pathlib.Path,required=True)
    parser.add_argument('--include-class',action='append',default=[])
    args = parser.parse_args()
    result = [compare(args.root,pkg,args.include_class) for pkg in ('Engine','WarfareGame')]
    args.output.write_text(json.dumps(result,indent=2),encoding='utf-8')
    for package in result:
        print(package['package'],'parsed changed defaults:',len(package['changed_defaults']),'errors:',len(package['errors']))
        for error in package['errors']: print(error)


if __name__ == '__main__': main()
