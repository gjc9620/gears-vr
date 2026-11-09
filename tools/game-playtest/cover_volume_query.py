"""Bounded original nonzero-Extent controls, never a body/occupancy permit."""
import copy
import math
import struct
import time

from cover_trace_query import capture, TRACE_FLAGS, _point


def _encoded(value):
    return struct.unpack('<3f', struct.pack('<3f', *value))


def capture_volume_probe(observer, dll_path, dll_sha256, start, end, *, extent=(10, 10, 10),
                         maximum_seconds=30.0, capture_query=capture, save_result=None):
    """Four nonzero queries: swept, separated, overlapped, leaving.

    The first current positive-Extent sweep acquires its own actual surface
    and normal. A zero-Extent mesh is neither queried nor used as a surface
    permit: game30188 showed a different nonzero BlockingVolume hit. All later
    observations retain the exact acquired component/transform/context and
    query extent. Static overlap uses EXACT equal end
    points. Expected hit/empty contrasts are diagnostic controls; they do not
    certify shape coverage, penetration recovery, world synchronization or a
    whole body. FCheckResult.Time is not exposed by Actor.Trace's reflected ABI.
    """
    result = dict(schema=2, accepted=False, diagnostic_only=True, queries=[],
                  collision_verified=False, grab_authorized=False, body_modified=False,
                  physical_headset_acceptance=False, raw_hit_time_available=False,
                  shape_semantics_verified=False, zero_extent_surface_reused=False,
                  surface_acquired_by='swept_extent', started_perf_counter_ns=time.perf_counter_ns())
    deadline = None
    cohort = actor = surface = None

    def publish():
        if save_result is not None:
            save_result(copy.deepcopy(result))

    try:
        start, end = _point(start, 'Volume start'), _point(end, 'Volume end')
        extent = _point(extent, 'Volume extent', extent=True)
        length = math.dist(start, end)
        if not .001 <= length <= 300 or min(extent) < .001 or min(_encoded(extent)) < .001:
            raise ValueError('Volume controls require a bounded line and three positive extents')
        if type(maximum_seconds) not in (int, float) or not math.isfinite(maximum_seconds) or not 0 < maximum_seconds <= 30:
            raise ValueError('Volume controls require a positive maximum thirty-second budget')
        deadline = time.monotonic() + maximum_seconds
        result.update(start=start, end=end, extent=extent, maximum_calls=4, maximum_seconds=maximum_seconds)
        direction = tuple((end[i] - start[i]) / length for i in range(3))

        def query(label, origin, target, shape, wanted_hit):
            nonlocal cohort, actor, surface
            if len(result['queries']) >= 4 or time.monotonic() >= deadline:
                raise TimeoutError('Volume control budget exhausted before original call')
            row = dict(label=label, start=origin, end=target, extent=shape,
                       outcome='attempted_result_unknown', wanted_hit=wanted_hit)
            result['queries'].append(row)
            publish()  # Persist unknown BEFORE an external call; never retry.
            receipt = capture_query(observer, dll_path, dll_sha256, origin, target,
                                    extent=shape, trace_actors=False, extra_flags=TRACE_FLAGS,
                                    snapshot_surface=True, surface_actor=actor,
                                    surface_component=surface['component'] if surface is not None else None)
            row.update(receipt=copy.deepcopy(receipt), outcome='returned')
            publish()
            if (receipt.get('accepted') is not True or type(receipt.get('trace_calls')) is not int or
                    receipt['trace_calls'] != 1 or receipt.get('pending_possible') is not False or
                    receipt.get('allocation_retained') is not False):
                raise RuntimeError('Volume original query failed or pending; stop without retry')
            if time.monotonic() >= deadline:
                raise TimeoutError('Volume control budget exhausted after completed call')
            current = tuple(copy.deepcopy(receipt[k]) for k in
                            ('session_id', 'game_identity', 'owner_identity', 'roles', 'pawn_position', 'module', 'function'))
            if cohort is None:
                cohort = current
            elif current != cohort:
                raise RuntimeError('Volume control Pawn/slot/session/function binding changed')
            inputs = receipt['inputs']
            if (tuple(inputs['start']) != origin or tuple(inputs['end']) != target or tuple(inputs['extent']) != shape or
                    tuple(inputs['encoded_start']) != _encoded(origin) or tuple(inputs['encoded_end']) != _encoded(target) or
                    tuple(inputs['encoded_extent']) != _encoded(shape) or
                    inputs['stationary'] is not (origin == target) or inputs['trace_actors'] is not False or
                    inputs['extra_flags'] != TRACE_FLAGS):
                raise RuntimeError('Volume completed input/float32 encoding differs')
            observed = receipt['surface_after']
            if surface is not None and (receipt['surface_before'] != surface or observed != surface):
                raise RuntimeError('Volume selected surface changed')
            hit = receipt['hit']
            address = hit['actor_address']
            location, normal = _point(hit['location'], 'Volume returned location'), _point(hit['normal'], 'Volume returned normal')
            if type(address) is not int or address < 0:
                raise RuntimeError('Volume hit Actor address invalid')
            blocked = address != 0
            if not blocked:
                if location != (0, 0, 0) or normal != (0, 0, 0):
                    raise RuntimeError('Volume empty result has unexplained hit data')
            else:
                selected = hit['actor_identity']
                component = hit['component_identity']
                if (address in {receipt['roles'][k]['address'] for k in ('pawn', 'controller', 'link')} or
                        selected['address'] != address or selected != observed['actor'] or
                        component != observed['component'] or (actor is not None and selected != actor)):
                    raise RuntimeError('Volume query hit another/self/marker Actor or component')
                if actor is None:
                    actor, surface = copy.deepcopy(selected), copy.deepcopy(observed)
            row.update(hit_observed=blocked, returned_location=location, returned_normal=normal)
            if blocked != wanted_hit:
                raise RuntimeError('Volume control hit/empty contrast failed: ' + label)
            if label == 'swept_extent':
                distance = sum((location[i] - origin[i]) * direction[i] for i in range(3))
                off = math.sqrt(sum((location[i] - origin[i] - distance * direction[i]) ** 2 for i in range(3)))
                norm = math.sqrt(sum(x*x for x in normal))
                if (not .001 < distance < length - .001 or off > .01 or not .99 <= norm <= 1.01 or
                        abs(normal[2]) > .1 or sum(normal[i]*direction[i] for i in range(3)) > -.9):
                    raise RuntimeError('Volume swept baseline is not an interior facing wall contact')
            # Zero normal at initial overlap is retained verbatim, not changed
            # into a push-out vector. No raw collision Time is manufactured.
            return location, normal

        point, normal = query('swept_extent', start, end, extent, True)
        depth = min(extent) * .5
        overlap = tuple(point[i] - depth * normal[i] for i in range(3))
        result.update(current_swept_contact=point, contact_normal=normal, planned_overlap_center=overlap,
                      diagnostic_depth=depth, not_a_penetration_recovery_distance=True)
        query('stationary_separated', start, start, extent, False)
        query('stationary_initial_overlap', overlap, overlap, extent, True)
        query('leaving_initial_overlap', overlap, start, extent, True)
        result.update(accepted=True, actor_identity=actor, surface_state=surface,
                      controls_observed=True, finished_perf_counter_ns=time.perf_counter_ns())
    except Exception as error:
        result.update(accepted=False, rejection=str(error), rejection_type=type(error).__name__,
                      finished_perf_counter_ns=time.perf_counter_ns())
    try:
        publish()
    except Exception as error:
        result.update(accepted=False, save_error=repr(error))
    return result
