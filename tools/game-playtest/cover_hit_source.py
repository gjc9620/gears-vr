"""Original skeletal hit-source fields inside the caller's closed CoverQuery.

The original PhysicsAsset route at0x131e80 filters BodySetup entries before
looking up FName in the mesh asset cache and reading SpaceBases. These fields
are source observations only. The caller still owns current setup/mesh/asset
association, code/slot authentication, the query close and same-pose proof.
"""
import hashlib
import struct

from cover_bone_query import _chain, _field
from cover_surface_query import _kind, _property
import cover_surface_query as surface

# Historical artifact1 chunks, not invocation permission or a dependency closure.
FILTER_CODE = (
    (0x131e80, 1535, 'e430cfd75f64760b02ad1635ddfaf565bf8f94de85744558e6eac8c32a4e1f1f'),
    (0x8aa1f6, 361, 'a1f8d8bbe3dc83d8cdd47be0d52e264b19c6ff7a48358904a720b22997f34755'),
)


def historical_chunks():
    """Load the same retained original artifact, without opening a process."""
    raw = surface.DEFAULT_BLOB.read_bytes()
    if len(raw) != surface.HISTORICAL_SIZE or hashlib.sha256(raw).hexdigest() != surface.HISTORICAL_SHA256:
        raise RuntimeError('Hit filter historical code artifact differs')
    chunks = []
    for rva, size, digest in FILTER_CODE:
        data = raw[rva - surface.HISTORICAL_RVA:rva - surface.HISTORICAL_RVA + size]
        if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
            raise RuntimeError('Hit filter historical code chunk differs')
        chunks.append((rva, data, digest))
    return chunks


class FilterReader:
    def __init__(self, query):
        self.q, self.layouts = query, {}

    def layout(self, setup):
        if setup['cls'] in self.layouts:
            return self.layouts[setup['cls']]
        query = self.q
        ancestry = list(_chain(query, setup['cls']))
        owners = [c for c in ancestry if c['name'] == 'RB_BodySetup']
        if len(owners) != 1 or owners[0]['number'] or _kind(query, owners[0]) != 'Class':
            raise RuntimeError('Hit filter setup class rejected')
        fields, declaring = {}, {}
        for name, mask in (('bBlockZeroExtent', 4), ('bBlockNonZeroExtent', 8)):
            field = _field(query, setup, name, 'BoolProperty')
            child = query.obj(field['address'])
            owner = query.obj(child['outer'])
            if owner not in ancestry or owner['number'] or _kind(query, owner) != 'Class':
                raise RuntimeError('Hit filter declaring class is outside current setup ancestry')
            prop = _property(query, child, owner, 0xc0)
            if (prop['type'], prop['offset'], prop['size'], prop['bool_mask']) != ('BoolProperty', 0xbc, 4, mask):
                raise RuntimeError('Hit filter differs from original native consumer: ' + name)
            fields[name], declaring[name] = prop, owner
        result = dict(setup_base_class=owners[0], declaring_classes=declaring, fields=fields)
        self.layouts[setup['cls']] = result
        return result


def read_setup_filters(query, setup, *, reader=None):
    """Read and bind the two original query masks; never chooses a hit route.

    A false flag is a valid observation, not missing evidence. In particular,
    bNoRBCollision on individual shapes is not substituted for these flags.
    All metadata and the shared word are remembered by the existing query;
    its caller MUST finish the same query before using the observation.
    """
    if (query.obj(setup['address']) != setup or setup['name'].startswith('Default__') or
            setup['number'] < 0):
        raise RuntimeError('Hit filter setup identity rejected')
    if reader is None:
        reader = FilterReader(query)
    elif not isinstance(reader, FilterReader) or reader.q is not query:
        raise RuntimeError('Hit filter reader belongs to another query')
    layout = reader.layout(setup)
    word = query.read(setup['address'] + 0xbc, 4)
    bits = struct.unpack('<I', word)[0]
    return dict(setup=setup, **layout, word_bytes=word.hex(),
                bBlockZeroExtent=bool(bits & 4), bBlockNonZeroExtent=bool(bits & 8),
                source_field_layout_matches=True, closing_required=True,
                selected_route_code_currently_verified=False, narrow_phase_selected=False,
                hit_geometry_verified=False, active_physics_verified=False,
                world_synchronization_verified=False, activation_allowed=False, engine_calls=False)
