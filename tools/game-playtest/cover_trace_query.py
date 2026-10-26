"""One bounded, diagnostic Actor.Trace call in an already owned game session.

The caller supplies an Observer that has sampled the current covered Pawn and
an already loaded Playground module. This never launches a game or grants a
cover grab. A returned hit is evidence about one ray, not a collision volume.
"""
import copy
import hashlib
import math
from pathlib import Path
import struct
import time

from cover_state import CoverQuery
from cover_geometry_state import _geometry
from cover_bone_query import _AttachedBridge, _module, _ownership, SubmitFailure
import cover_surface_query as surface


REQUEST_SIZE = 24 + 4096
CAPTURE_SECONDS = 20.0
CALL_SECONDS = 1.5
TRACE_FLAGS = 8  # Reflected Engine.Actor.TRACEFLAG_Blocking; no other bits.
PARAMS = (
    ('HitLocation', 0, 12, '0x180'), ('HitNormal', 12, 12, '0x180'),
    ('TraceEnd', 24, 12, '0x80'), ('TraceStart', 36, 12, '0x90'),
    ('bTraceActors', 48, 4, '0x90'), ('Extent', 52, 12, '0x90'),
    ('HitInfo', 64, 40, '0x190'), ('ExtraTraceFlags', 104, 4, '0x90'),
    ('ReturnValue', 108, 8, '0x580'),
)


def _remaining(deadline):
    value = deadline - time.monotonic()
    if value <= 0:
        raise TimeoutError('Cover Trace diagnostic deadline exceeded')
    return value


def _point(value, label, *, extent=False):
    if not isinstance(value, (list, tuple)) or len(value) != 3 or any(type(x) not in (int, float) for x in value):
        raise ValueError(label + ' requires exactly three numeric coordinates')
    values = tuple(float(x) for x in value)
    if any(not math.isfinite(x) or abs(x) > 2 ** 24 or (extent and (x < 0 or x > 30)) for x in values):
        raise ValueError(label + ' outside finite diagnostic bounds')
    return values


def _parameters(function, start, end, extent, *, trace_actors=True, extra_flags=TRACE_FLAGS):
    """Pack only this exact reflected Trace parameter cohort, not guessed ABI."""
    fields = function['fields']
    observed_header = (function['native_index'], function['parms_size'], function['return_offset'],
                       function['function_flags_offset'], function['function_flags'], function['entry_rva'])
    # The saved retail header has 0x424401 at 0xf4. An older reader used 0xfc
    # and reported 0x5c1a from a different field as FunctionFlags.
    expected_header = (277, 116, 108, 0xf4, 0x424401, '0xcdacc')
    if observed_header != expected_header:
        raise RuntimeError('Current Actor.Trace function header differs from certified cohort: '
                           f'observed={observed_header!r}; expected={expected_header!r}')
    if [(f['name'], f['offset'], f['size'], f['flags_hex']) for f in fields] != list(PARAMS):
        raise RuntimeError('Current Actor.Trace typed parameter layout differs')
    if next(f for f in fields if f['name'] == 'bTraceActors')['bool_mask'] != 1:
        raise RuntimeError('Current Actor.Trace Boolean bit mask differs')
    if type(trace_actors) is not bool:
        raise ValueError('bTraceActors requires an explicit Boolean')
    if type(extra_flags) is not int or extra_flags not in (0, TRACE_FLAGS):
        raise ValueError('ExtraTraceFlags requires explicit diagnostic 0 or Blocking')
    start, end, extent = _point(start, 'TraceStart'), _point(end, 'TraceEnd'), _point(extent, 'Extent', extent=True)
    if not .001 <= math.dist(start, end) <= 300:
        raise ValueError('Cover Trace diagnostic segment length outside 0.001..300 game units')
    params = bytearray(function['parms_size'])
    struct.pack_into('<3f', params, 24, *end)
    struct.pack_into('<3f', params, 36, *start)
    struct.pack_into('<I', params, 48, int(trace_actors))
    struct.pack_into('<3f', params, 52, *extent)
    struct.pack_into('<I', params, 104, extra_flags)
    return bytes(params), dict(start=start, end=end, extent=extent, trace_actors=trace_actors, extra_flags=extra_flags)


def _certificate(q, roles, module, expected_chunks):
    row, structures = {}, {}
    surface._function(q, roles['controller'], 'Actor', 'Trace', module, structures, row)
    # Playground.invoke emits EX_NativeParm tokens in the raw UField.Next order.
    # surface._function intentionally normalizes fields by name, so its row
    # cannot by itself certify the order the native wrapper will consume.
    children = surface._children(q, row['identity'])
    if [child['name'] for child in children] != [field[0] for field in PARAMS]:
        raise RuntimeError('Current Actor.Trace raw parameter chain order differs')
    if row['entry'] != module['base'] + surface.TRACE_RVA:
        raise RuntimeError('Current Actor.Trace reflected entry differs')
    _parameters(row, (0, 0, 0), (1, 0, 0), (0, 0, 0))
    slot_rva = surface.GNATIVES_RVA + surface.TRACE_INDEX * 8
    surface._range(module, slot_rva, 8)
    slot = struct.unpack('<Q', q.read(module['base'] + slot_rva, 8))[0]
    if slot != row['entry']:
        raise RuntimeError('Current Actor.Trace GNatives277 slot differs')
    chunks = []
    for rva, expected, digest in expected_chunks:
        surface._range(module, rva, len(expected), True)
        current = q.read(module['base'] + rva, len(expected))
        chunks.append(dict(rva=hex(rva), size=len(expected), sha256=hashlib.sha256(current).hexdigest()))
        if current != expected or chunks[-1]['sha256'] != digest:
            raise RuntimeError('Current Actor.Trace complete code chunk differs: ' + hex(rva))
    return row, dict(gnatives_slot_rva=hex(slot_rva), chunks=chunks, structures=structures)


def capture(observer, dll_path, dll_sha256, start, end, *, extent=(0, 0, 0), trace_actors=True,
            extra_flags=TRACE_FLAGS,
            bridge_factory=_AttachedBridge, query_factory=CoverQuery):
    """Run at most one existing GT queue item; unknown pending is never retried."""
    report = dict(schema=1, accepted=False, diagnostic_only=True, trace_calls=0,
                  collision_verified=False, grab_authorized=False, body_modified=False,
                  physical_headset_acceptance=False, allocation_retained=False,
                  started_perf_counter_ns=time.perf_counter_ns())
    bridge = q = None
    address = None
    completed = False
    deadline = time.monotonic() + CAPTURE_SECONDS
    try:
        if (type(dll_sha256) is not str or len(dll_sha256) != 64 or
                any(c not in '0123456789abcdef' for c in dll_sha256)):
            raise ValueError('Exact lowercase already loaded Playground SHA256 required')
        context = surface._lifecycle(observer)
        game = _ownership(observer)
        report.update(session_id=context['session_id'], game_identity=copy.deepcopy(game),
                      owner_identity=copy.deepcopy(context['identities']['owner_identity']))
        source, expected = surface._historical(surface.DEFAULT_BLOB)
        disk = surface._disk_module(observer)
        report.update(context=context, historical_source=source)
        _remaining(deadline)
        bridge = bridge_factory(game, dll_path, expected_sha256=dll_sha256, attach_only=True)
        report['playground_submit'] = copy.deepcopy(bridge.submit_source)
        q = query_factory(observer)
        roles = surface._roles(q)
        module = _module(q, bridge, game)
        if module['sha256'] != surface.GAME_SHA256 or Path(module['path']).resolve() != Path(disk['path']).resolve():
            raise RuntimeError('Cover Trace disk/current module identity differs')
        function, proof = _certificate(q, roles, module, expected)
        params, inputs = _parameters(function, start, end, extent,
                                     trace_actors=trace_actors, extra_flags=extra_flags)
        pawn_position = tuple(_geometry(q, roles['pawn'], 'Location', 'Vector')['value'])
        if math.dist(inputs['start'], pawn_position) > 300 or math.dist(inputs['end'], pawn_position) > 500:
            raise ValueError('Cover Trace ray must remain near current controlled Pawn')
        report.update(roles=roles, module={k: module[k] for k in ('base', 'path', 'sha256', 'size')},
                      function=function, proof_before=proof, inputs=inputs, pawn_position=pawn_position)
        q.finish()
        if surface._lifecycle(observer) != context or _ownership(observer, report) != game:
            raise RuntimeError('Cover Trace lifecycle changed before submission')
        _remaining(deadline)
        address = bridge.allocate(REQUEST_SIZE)
        if type(address) is not int or not 0 < address <= (1 << 64) - REQUEST_SIZE:
            raise RuntimeError('Cover Trace private Request allocation rejected')
        report.update(allocation_address=address, allocation_size=REQUEST_SIZE, allocation_retained=True,
                      allocation_release_condition='Only observed queue state 2; else retain until original game exits')
        request = struct.pack('<iIQQ', 0, function['parms_size'], roles['controller']['address'], function['identity']['address'])
        bridge.write(address, request + params)
        q.finish()
        call_deadline = min(deadline, time.monotonic() + CALL_SECONDS)
        _ownership(observer, report)
        report['submission'] = 'attempted_result_unknown'
        report['pending_possible'] = True
        result = bridge.submit(address, max(1, min(1000, int(_remaining(call_deadline) * 1000))))
        report.update(submit_result=result, pending_possible=result not in (1, 2))
        if result != 0:
            raise RuntimeError('Cover Trace PlaygroundSubmit rejected/unknown result: ' + str(result))
        while True:
            _ownership(observer, report)
            bridge.verify()
            state = struct.unpack('<i', bridge.read(address, 4))[0]
            report['queue_state'] = state
            if state == 2:
                completed = True
                report['pending_possible'] = False
                report['trace_calls'] = 1
                break
            if state != 0:
                if state == -1:
                    report['pending_possible'] = False
                raise RuntimeError('Cover Trace rejected/unknown queue state: ' + str(state))
            time.sleep(min(.005, _remaining(call_deadline)))
        raw = bridge.read(address, REQUEST_SIZE)
        expected_header = struct.pack('<iIQQ', 2, function['parms_size'], roles['controller']['address'], function['identity']['address'])
        if len(raw) != REQUEST_SIZE or raw[:24] != expected_header:
            raise RuntimeError('Cover Trace completed Request header differs')
        returned = raw[24:24 + function['parms_size']]
        for offset, size in ((24, 12), (36, 12), (48, 4), (52, 12), (104, 4)):
            if returned[offset:offset + size] != params[offset:offset + size]:
                raise RuntimeError('Cover Trace completed input parameter changed')
        hit_location = struct.unpack_from('<3f', returned, 0)
        hit_normal = struct.unpack_from('<3f', returned, 12)
        actor = struct.unpack_from('<Q', returned, 108)[0]
        if any(not math.isfinite(x) or abs(x) > 2 ** 24 for x in (*hit_location, *hit_normal)):
            raise RuntimeError('Cover Trace output Vector is nonfinite/outside diagnostic bounds')
        report['hit'] = dict(actor_address=actor, location=hit_location, normal=hit_normal,
                             hit_info_bytes=returned[64:104].hex())
        if actor:
            report['hit']['actor_identity'] = q.obj(actor)
        if surface._roles(q) != roles:
            raise RuntimeError('Cover Trace closing Pawn/Link/slot binding differs')
        closing, after = _certificate(q, roles, module, expected)
        if closing != function or after != proof:
            raise RuntimeError('Cover Trace closing function/code certificate differs')
        q.finish()
        if surface._lifecycle(observer) != context or _ownership(observer, report) != game:
            raise RuntimeError('Cover Trace closing lifecycle differs')
        report.update(accepted=True, proof_after=after)
    except BaseException as error:
        report.update(accepted=False, rejection=str(error), rejection_type=type(error).__name__)
        if isinstance(error, SubmitFailure):
            report['pending_possible'] = error.pending_possible
    finally:
        if completed:
            try:
                bridge.free(address)
                report['allocation_retained'] = False
            except BaseException as error:
                report.update(accepted=False, free_error=str(error), free_error_type=type(error).__name__)
        if q is not None:
            report['query_budget'] = dict(reads=q.reads, bytes=q.bytes)
            if q.closing_rejection:
                report['closing_rejection'] = copy.deepcopy(q.closing_rejection)
        if bridge is not None:
            try:
                bridge.close()
            except BaseException as error:
                report.update(accepted=False, close_error=str(error), close_error_type=type(error).__name__)
        report['finished_perf_counter_ns'] = time.perf_counter_ns()
    return report
