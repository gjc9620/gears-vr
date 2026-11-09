"""Read reflected authored body shapes inside the caller's CoverQuery.

The caller owns current setup/mesh association and MUST close the same query.
Authored shapes are not active scene shapes, a transformed body volume, a
convex cook, a sweep, an overlap result, or permission to move anything.
"""
import math
import struct

from cover_bone_query import _chain, _field
from cover_surface_query import _children, _kind, _property

MAX_SHAPES = 64
MAX_VERTICES = 256
MAX_TOPOLOGY = 1536
KINDS = {'SphereElems': 'KSphereElem', 'BoxElems': 'KBoxElem',
         'SphylElems': 'KSphylElem', 'ConvexElems': 'KConvexElem'}


class ShapeReader:
    def __init__(self, query):
        self.q = query
        self.layouts = {}

    def layout(self, typ, names):
        if typ['number'] or _kind(self.q, typ) != 'ScriptStruct':
            raise RuntimeError('Physics shape structure identity rejected')
        if typ['address'] in self.layouts:
            saved = self.layouts[typ['address']]
            if set(saved['fields']) != set(names):
                raise RuntimeError('Physics shape structure reused with a different schema')
            return saved
        size = struct.unpack('<I', self.q.read(typ['address'] + 0x88, 4))[0]
        ancestry = list(_chain(self.q, typ['address']))
        declared = [(owner, child) for owner in ancestry for child in _children(self.q, owner)]
        if (not 0 < size <= 1024 or any(_kind(self.q, owner) != 'ScriptStruct' for owner in ancestry) or
                len(declared) != len(names) or {child['name'] for _, child in declared} != set(names)):
            raise RuntimeError('Physics shape structure extent/complete field set rejected: ' + typ['name'])
        fields = {child['name']: _property(self.q, child, owner, size) for owner, child in declared}
        spans = sorted((p['offset'], p['offset'] + p['size']) for p in fields.values())
        if any(end > begin for (_, end), (begin, _) in zip(spans, spans[1:])):
            raise RuntimeError('Physics shape fields overlap')
        result = dict(identity=typ, ancestry=ancestry, size=size, fields=fields)
        self.layouts[typ['address']] = result
        return result

    def raw(self, base, field):
        at = base + field['offset']
        if not 0 < base <= at < (1 << 64) - field['size']:
            raise RuntimeError('Physics shape field address rejected')
        return self.q.read(at, field['size'])

    def vector(self, base, typ, expected):
        names = ('X', 'Y', 'Z') if expected == 'Vector' else ('X', 'Y', 'Z', 'W')
        if typ['name'] != expected:
            raise RuntimeError('Physics shape vector structure type rejected')
        layout = self.layout(typ, names)
        if layout['size'] != len(names) * 4:
            raise RuntimeError('Physics shape vector extent rejected')
        values = []
        for index, name in enumerate(names):
            field = layout['fields'][name]
            if (field['type'], field['size'], field['offset']) != ('FloatProperty', 4, index * 4):
                raise RuntimeError('Physics shape vector component layout rejected')
            value = struct.unpack('<f', self.raw(base, field))[0]
            if not math.isfinite(value):
                raise RuntimeError('Physics shape nonfinite vector component')
            values.append(value)
        return values

    def embedded(self, base, field, expected):
        if field['type'] != 'StructProperty' or field['target']['name'] != expected:
            raise RuntimeError('Physics shape embedded structure type rejected: ' + field['name'])
        typ = field['target']
        if field['size'] != struct.unpack('<I', self.q.read(typ['address'] + 0x88, 4))[0]:
            raise RuntimeError('Physics shape embedded structure width rejected')
        return base + field['offset'], typ

    def matrix(self, base, field):
        at, typ = self.embedded(base, field, 'Matrix')
        names = ('XPlane', 'YPlane', 'ZPlane', 'WPlane')
        layout = self.layout(typ, names)
        if layout['size'] != 64:
            raise RuntimeError('Physics shape Matrix extent rejected')
        rows = []
        for index, name in enumerate(names):
            plane = layout['fields'][name]
            if plane['offset'] != index * 16 or plane['size'] != 16:
                raise RuntimeError('Physics shape Matrix plane layout rejected')
            point, target = self.embedded(at, plane, 'Plane')
            rows.append(self.vector(point, target, 'Plane'))
        return rows  # No inferred local/world convention or rigid-transform claim.

    def array(self, base, field, expected, limit):
        if field['type'] != 'ArrayProperty' or field['size'] != 16:
            raise RuntimeError('Physics shape array header type/width rejected')
        header = self.raw(base, field)
        data, count, capacity = struct.unpack('<Qii', header)
        if (not 0 <= count <= capacity <= limit or
                (capacity and (not data or data & 3))):
            raise RuntimeError('Physics shape array bounds rejected; no truncation')
        inner = self.q.obj(struct.unpack('<Q', self.q.read(field['identity']['address'] + 0xd0, 8))[0])
        if inner['outer'] != field['identity']['address'] or inner['number']:
            raise RuntimeError('Physics shape array inner identity rejected')
        raw = self.q.read(inner['address'] + 0x70, 0x28)
        dimension, stride = struct.unpack_from('<II', raw)
        if dimension != 1 or not 0 < stride <= 1024 or struct.unpack_from('<I', raw, 0x24)[0]:
            raise RuntimeError('Physics shape array inner dimension/stride/offset rejected')
        target = None
        if expected == 'Int':
            if _kind(self.q, inner) != 'IntProperty' or stride != 4:
                raise RuntimeError('Physics shape integer array layout rejected')
        else:
            if _kind(self.q, inner) != 'StructProperty':
                raise RuntimeError('Physics shape struct array inner type rejected')
            target = self.q.obj(struct.unpack('<Q', self.q.read(inner['address'] + 0xd0, 8))[0])
            if (target['name'] not in expected or target['number'] or _kind(self.q, target) != 'ScriptStruct' or
                    struct.unpack('<I', self.q.read(target['address'] + 0x88, 4))[0] != stride):
                raise RuntimeError('Physics shape struct array target/extent rejected')
        if capacity and data >= (1 << 64) - capacity * stride:
            raise RuntimeError('Physics shape array pointer overflow rejected')
        if count * stride > 32768:
            raise RuntimeError('Physics shape array byte budget rejected; no truncation')
        payload = self.q.read(data, count * stride) if count else b''
        return dict(header=header.hex(), data=data, count=count, capacity=capacity, stride=stride,
                    inner=inner, structure=target, bytes=payload.hex())

    def primitive(self, base, typ):
        dimensions = {'KSphereElem': ('Radius',), 'KBoxElem': ('X', 'Y', 'Z'),
                      'KSphylElem': ('Radius', 'Length')}[typ['name']]
        fields = self.layout(typ, ('TM', *dimensions, 'bNoRBCollision'))['fields']
        result = dict(structure=typ, TM=self.matrix(base, fields['TM']), dimensions={})
        for name in dimensions:
            field = fields[name]
            if (field['type'], field['size']) != ('FloatProperty', 4):
                raise RuntimeError('Physics primitive dimension type/width rejected')
            value = struct.unpack('<f', self.raw(base, field))[0]
            if not math.isfinite(value) or (value < 0 if name == 'Length' else value <= 0):
                raise RuntimeError('Physics primitive dimension is nonfinite/invalid')
            result['dimensions'][name] = value  # Raw Radius/Length/X/Y/Z; no half/full unit inference.
        field = fields['bNoRBCollision']
        if field['type'] != 'BoolProperty':
            raise RuntimeError('Physics primitive collision Boolean type rejected')
        result['bNoRBCollision'] = bool(struct.unpack('<I', self.raw(base, field))[0] & field['bool_mask'])
        return result

    def convex(self, base, typ):
        specs = {'VertexData': (('Vector',), MAX_VERTICES), 'PermutedVertexData': (('Vector', 'Vector4'), MAX_VERTICES),
                 'FaceTriData': ('Int', MAX_TOPOLOGY), 'EdgeDirections': (('Vector',), MAX_TOPOLOGY),
                 'FaceNormalDirections': (('Vector',), MAX_TOPOLOGY), 'FacePlaneData': (('Plane',), MAX_TOPOLOGY)}
        fields = self.layout(typ, (*specs, 'ElemBox'))['fields']
        result = dict(structure=typ, arrays={})
        for name, (expected, limit) in specs.items():
            array = self.array(base, fields[name], expected, limit)
            if expected == 'Int':
                values = list(struct.unpack('<' + 'i' * array['count'], bytes.fromhex(array['bytes'])))
            else:
                values = [self.vector(array['data'] + i * array['stride'], array['structure'], array['structure']['name'])
                          for i in range(array['count'])]
            result['arrays'][name] = dict(array, values=values)
        at, box = self.embedded(base, fields['ElemBox'], 'Box')
        parts = self.layout(box, ('Min', 'Max', 'IsValid'))['fields']
        bounds = {}
        for name in ('Min', 'Max'):
            point, vector = self.embedded(at, parts[name], 'Vector')
            bounds[name] = self.vector(point, vector, 'Vector')
        valid = parts['IsValid']
        if (valid['type'], valid['size']) != ('ByteProperty', 1):
            raise RuntimeError('Physics convex bounds valid-byte type rejected')
        bounds['IsValid'] = self.raw(at, valid)[0]
        result['ElemBox'] = bounds
        return result  # Cook, topology correctness, activity and bounds authority remain unknown.


def read_setup_shapes(query, setup):
    """Observe every declared authored shape; the same query must finish later."""
    if (query.obj(setup['address']) != setup or setup['name'].startswith('Default__') or
            not any(c['name'] == 'RB_BodySetup' for c in _chain(query, setup['cls']))):
        raise RuntimeError('Physics shape setup identity/class rejected')
    field = _field(query, setup, 'AggGeom', 'StructProperty')
    owner = query.obj(query.obj(field['address'])['outer'])
    if owner['name'] != 'KMeshProps' or owner['number'] or _kind(query, owner) != 'Class':
        raise RuntimeError('Physics aggregate declaring class rejected')
    prop = _property(query, query.obj(field['address']), owner, field['offset'] + field['size'])
    reader = ShapeReader(query)
    at, typ = reader.embedded(setup['address'], prop, 'KAggregateGeom')
    if typ['outer'] != owner['address']:
        raise RuntimeError('Physics aggregate structure owner rejected')
    fields = reader.layout(typ, KINDS)['fields']
    out = dict(setup=setup, aggregate=typ, arrays={}, shapes=[], closing_required=True,
               active_physics_verified=False, world_pose_verified=False, collision_volume_verified=False,
               sweep_verified=False, occupancy_verified=False, activation_allowed=False)
    for name, expected in KINDS.items():
        array = reader.array(at, fields[name], (expected,), MAX_SHAPES)
        if array['structure']['outer'] != owner['address']:
            raise RuntimeError('Physics shape element declaring owner rejected')
        out['arrays'][name] = array
        if len(out['shapes']) + array['count'] > MAX_SHAPES:
            raise RuntimeError('Physics aggregate shape count exceeds bound; no truncation')
        for index in range(array['count']):
            base = array['data'] + index * array['stride']
            row = reader.convex(base, array['structure']) if name == 'ConvexElems' else reader.primitive(base, array['structure'])
            out['shapes'].append(dict(row, array=name, index=index))
    out['layouts'] = list(reader.layouts.values())
    return out
