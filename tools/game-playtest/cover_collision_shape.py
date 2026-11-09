"""Current Pawn cylinder source observations; no sweep, movement or hit permit.

The retained Pawn GetCollisionHeight/GetCollisionRadius scripts read the named
CylinderComponent. Authenticate its equality to CollisionComponent instead of
assuming any selected component or historical radius describes the body. The
values remain raw engine fields: this reader does not infer half-height, world
scale, final leaned-body coverage, active physics shapes or query visibility.
"""
import copy
import math
import struct
import time

from cover_state import CoverQuery
from cover_animation_state import lineage
from cover_bone_query import _field, _reference
from cover_surface_query import _lifecycle, _roles
from cover_trace_query import _surface_state


def _dimensions(q, component):
    result = {}
    occupied = set()
    for name in ('CollisionRadius', 'CollisionHeight'):
        field = _field(q, component, name, 'FloatProperty')
        if field['size'] != 4 or field['offset'] & 3 or not 0 <= field['offset'] <= 65532:
            raise RuntimeError('Cover cylinder scalar layout rejected: ' + name)
        if field['offset'] in occupied:
            raise RuntimeError('Cover cylinder dimension fields alias')
        occupied.add(field['offset'])
        raw = q.value(component, field)
        value = struct.unpack('<f', raw)[0]
        # A diagnostic input cap, not a character dimension or sweep extent.
        if not math.isfinite(value) or not 0 < value <= 300:
            raise RuntimeError('Cover cylinder raw dimension outside diagnostic bounds: ' + name)
        result[name] = dict(value=value, raw=raw.hex(), field=field)
    return result


def capture(observer, *, query_factory=CoverQuery):
    """One strict read bracket for current cover, collider, fields and matrix.

    accepted means the raw observation closed. The same Actor/Component does
    not establish an unchanged collision world or an atomic engine snapshot.
    No query extent is generated from these fields and no zero/absent field
    is replaced with an engineering default.
    """
    out = dict(schema=1, accepted=False, read_only=True, engine_calls=False,
               queue_submissions=0, body_modified=False, activation_allowed=False,
               collision_verified=False, shape_semantics_verified=False,
               final_body_coverage_verified=False, world_synchronization_verified=False,
               atomic_snapshot_verified=False, physical_headset_acceptance=False,
               started_perf_counter_ns=time.perf_counter_ns())
    q = None
    try:
        lifecycle = _lifecycle(observer)
        q = query_factory(observer)
        roles = _roles(q)
        pawn = roles['pawn']
        cylinder = _reference(q, pawn, 'CylinderComponent')
        collision = _reference(q, pawn, 'CollisionComponent')
        if (not cylinder or cylinder != collision or cylinder['name'].startswith('Default__') or
                cylinder['outer'] != pawn['address'] or 'CylinderComponent' not in lineage(q, cylinder) or
                _reference(q, cylinder, 'Owner') != pawn):
            raise RuntimeError('Cover current named/selected cylinder class, identity or owner rejected')
        dimensions = _dimensions(q, cylinder)
        surface = _surface_state(q, roles, pawn)
        if surface['component'] != cylinder:
            raise RuntimeError('Cover cylinder changed before transform observation')
        matrix = _field(q, cylinder, 'LocalToWorld', 'StructProperty')
        if any(matrix['offset'] <= v['field']['offset'] < matrix['offset'] + matrix['size']
               for v in dimensions.values()):
            raise RuntimeError('Cover cylinder dimensions overlap its transform')
        q.finish()
        if _lifecycle(observer) != lifecycle:
            raise RuntimeError('Cover cylinder session/game/owner changed at closing boundary')
        out.update(accepted=True, roles=roles, cylinder=cylinder, dimensions=dimensions,
                   surface=surface, source=lifecycle,
                   game_profile_sha256=q.r.profile.get('sha256'), module_base=getattr(q.r, 'base', None),
                   reads=q.reads, bytes=q.bytes,
                   limits='Raw named cylinder fields and typed component transform only; no half-height, '
                          'scale, final skeletal/physics shape coverage, sweep, occupancy or synchronization inference')
    except (RuntimeError, OSError, ValueError, TypeError, KeyError, struct.error) as error:
        out.update(rejection=str(error), rejection_type=type(error).__name__)
        witness = getattr(q, 'closing_rejection', None)
        if witness is not None:
            out['closing_rejection'] = copy.deepcopy(witness)
    out['finished_perf_counter_ns'] = time.perf_counter_ns()
    return out
