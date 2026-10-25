"""Bounded diagnostic CoverLink slot getters through an already loaded GT queue.

Only private Request storage is written. No Trace, setters, injection, hooks,
opaque native ABI call, collision query or grip authorization is provided.
Each returned value has its own checked interval; the pair is not atomic.
Reflected header/private source receipts belong under ignored analysis/ only.
"""
import copy
import hashlib
import json
import math
from pathlib import Path
import struct
import time

from cover_state import CoverQuery
import cover_surface_query as surface
from cover_bone_query import _AttachedBridge, _ownership, _module, SubmitFailure

CAPTURE_SECONDS, CALL_SECONDS = 12.0, 1.5
PARAM_CAP, REQUEST_SIZE, MAX_COORDINATE = 4096, 24 + 4096, 2 ** 24
PROFILE = {
 'GetSlotLocation': (0x3eef44, ((0x3eef44,141,'0bba62e399deee3162ccdf21d8bf9e04b97e73b40fb6ad86079f6ba04e51ae9f'),
                              (0x92ec82,32,'94865b43693b41693e9284780049dc26a7b7f11e6b74ff12c5fb85ef8ec5394d'))),
 'GetSlotRotation': (0x3dbab0, ((0x3dbab0,141,'49a153ff32310d2e460a36337f18d693cd03679c6931656a901fff6d8d2f6212'),
                              (0x927aec,32,'a7abee10b7e6facb510a4b137f7b893e5c16764d5a281faece19edbf9a2af9fa'))),
 'GetSlotLocationImpl': (0x10f6cc, ((0x10f6cc,273,'fbe8d1e67c3837b5c9802e9f8e8ccd9d14ada3f328b54cc6b8e18636ae087f40'),)),
 'GetSlotRotationImpl': (0x10eb80, ((0x10eb80,84,'39c24bca471af82e5ffd72b287b2131e5bdd88b1b7d6c9c1439dccbb39ff1d09'),
                                  (0x8a43d2,23,'6a68f007d4b9c27a205bc33467d53425d01ec6e1f58a47e8f25d8cf957cc14cf'))),
}


def _rva(value):
    if type(value) is not str or not value.startswith('0x') or len(value) > 18:
        raise RuntimeError('Surface getter certificate RVA syntax rejected')
    return int(value, 16)


def _certificate(path):
    if path is None:
        raise ValueError('Explicit private surface-getters2 certificate required')
    path = Path(path).resolve()
    with path.open('rb') as stream:
        raw = stream.read(65537)
    if not 1 <= len(raw) <= 65536:
        raise RuntimeError('Surface getter certificate byte budget rejected')
    data = json.loads(raw)
    methods = data.get('methods', [])
    if (data.get('complete') is not True or data.get('historical_runtime_only') is not True or
            data.get('source_sha256') != surface.HISTORICAL_SHA256 or len(methods) != 4 or
            {m['name'] for m in methods} != set(PROFILE)):
        raise RuntimeError('Surface getter certificate fixed method/source cohort rejected')
    chunks = []
    for name, (entry, expected) in PROFILE.items():
        method = next(m for m in methods if m['name'] == name)
        if _rva(method['rva']) != entry or len(method['chunks']) != len(expected):
            raise RuntimeError('Surface getter certificate fixed entry/chunk count rejected: ' + name)
        for row, (rva, size, digest) in zip(method['chunks'], expected):
            content = bytes.fromhex(row['bytes'])
            if (_rva(row['rva']) != rva or type(row['size']) is not int or row['size'] != size or
                    len(content) != size or row['sha256'] != digest or hashlib.sha256(content).hexdigest() != digest):
                raise RuntimeError('Surface getter certificate fixed full chunk rejected: ' + name)
            chunks.append((name, rva, content, digest))
    return chunks, dict(path=str(path), sha256=hashlib.sha256(raw).hexdigest(),
                        historical_source_sha256=surface.HISTORICAL_SHA256,
                        method_names=list(PROFILE), chunk_count=len(chunks), hard_pinned_profile=True)


def _remaining(deadline):
    value = deadline - time.monotonic()
    if value <= 0:
        raise RuntimeError('Surface getter bounded time budget exhausted')
    return value


def _code(q, module, chunks):
    rows = []
    for method, rva, expected, digest in chunks:
        section = surface._range(module, rva, len(expected), True)
        current = q.read(module['base'] + rva, len(expected))
        rows.append(dict(method=method, rva=hex(rva), size=len(expected), expected_sha256=digest,
                         current_sha256=hashlib.sha256(current).hexdigest(), equal=current == expected, section=section))
        if current != expected:
            raise RuntimeError('Surface getter CURRENT full chunk mismatch: ' + method + ' ' + hex(rva))
    return rows


def _function(q, roles, getter, module):
    row, structures = {}, {}
    surface._function(q, roles['link'], 'CoverLink', getter, module, structures, row)
    fields = {f['name']: f for f in row['fields']}
    if (row['native_index'], row['parms_size'], row['return_offset'], row['entry']) != (0,16,4,module['base'] + PROFILE[getter][0]):
        raise RuntimeError('Surface getter fixed NativeIndex/Parms/Return/entry rejected: ' + getter)
    if not row['function_flags'] & 0x400 or fields['SlotIdx']['offset'] != 0:
        raise RuntimeError('Surface getter native flag/SlotIdx typed offset rejected')
    row['structures'] = structures
    return row


def _call(observer, bridge, query_factory, baseline, getter, chunks, deadline, receipt):
    row = dict(getter=getter, accepted=False, submission='not_attempted', completion_observed=False,
               allocation_retained=False, started_perf_counter_ns=time.perf_counter_ns())
    receipt['calls'].append(row)
    address, completed, q = None, False, None
    call_deadline = min(deadline, time.monotonic() + CALL_SECONDS)
    try:
        game = _ownership(observer, receipt)
        q = query_factory(observer)
        roles = surface._roles(q)
        if roles != baseline:
            raise RuntimeError('Surface getter current PC/Pawn/World/Link/slot binding changed')
        module = _module(q, bridge, game)
        if module['sha256'] != surface.GAME_SHA256:
            raise RuntimeError('Surface getter unsupported current game image')
        row['roles'] = roles
        function = _function(q, roles, getter, module)
        row['function'] = function
        row['code_before'] = _code(q, module, chunks)
        params = bytearray(PARAM_CAP)
        field = next(f for f in function['fields'] if f['name'] == 'SlotIdx')
        argument = struct.pack('<i', roles['current_slot'])
        params[field['offset']:field['offset'] + field['size']] = argument
        header = struct.pack('<iIQQ', 0, function['parms_size'], roles['link']['address'], function['identity']['address'])
        raw = header + params
        q.finish()
        _remaining(call_deadline)
        address = bridge.allocate(REQUEST_SIZE)
        if type(address) is not int or not 0 < address <= (1 << 64) - REQUEST_SIZE:
            raise RuntimeError('Surface getter private Request allocation address rejected')
        row.update(allocation_address=address, allocation_size=REQUEST_SIZE, allocation_retained=True,
                   allocation_release_condition='Only observed queue state 2; otherwise retain until ORIGINAL process exits')
        bridge.write(address, raw)
        q.finish()
        _ownership(observer, receipt)
        row.update(submission='attempted_result_unknown', pending_possible=True)
        result = bridge.submit(address, max(1, min(1000, int(_remaining(call_deadline) * 1000))))
        row.update(submit_result=result, submission={0:'queued',1:'rejected_invalid',2:'rejected_busy'}.get(result,'unknown_return'),
                   pending_possible=result not in (1,2))
        if result != 0:
            raise RuntimeError('Surface getter PlaygroundSubmit rejected/unknown return: ' + str(result))
        while True:
            _ownership(observer, receipt)
            bridge.verify()
            state = struct.unpack('<i', bridge.read(address, 4))[0]
            row['queue_state'] = state
            if state == 2:
                completed = True
                row.update(completion_observed=True, pending_possible=False)
                break
            if state != 0:
                if state == -1:
                    row['pending_possible'] = False
                raise RuntimeError('Surface getter rejected/unknown queue state: ' + str(state))
            _remaining(call_deadline)
            time.sleep(min(.005, _remaining(call_deadline)))
        returned = bridge.read(address, REQUEST_SIZE)
        expected_header = struct.pack('<iIQQ', 2, function['parms_size'], roles['link']['address'], function['identity']['address'])
        offset = 24 + field['offset']
        if (len(returned) != REQUEST_SIZE or returned[:24] != expected_header or returned[offset:offset+4] != argument):
            raise RuntimeError('Surface getter completed Request header/SlotIdx changed')
        value = returned[24 + function['return_offset']:24 + function['return_offset'] + 12]
        decoded = list(struct.unpack('<3f' if getter == 'GetSlotLocation' else '<3i', value))
        if getter == 'GetSlotLocation' and (not all(math.isfinite(v) for v in decoded) or any(abs(v) > MAX_COORDINATE for v in decoded)):
            raise RuntimeError('Surface getter Vector nonfinite/outside diagnostic coordinate bound')
        row['code_after'] = _code(q, module, chunks)
        if surface._roles(q) != roles or _function(q, roles, getter, module) != function:
            raise RuntimeError('Surface getter closing binding/function metadata changed')
        q.finish()
        _ownership(observer, receipt)
        _remaining(call_deadline)
        row.update(accepted=True, value=decoded, return_bytes=value.hex())
        return decoded
    except BaseException as error:
        row.update(rejection=str(error), rejection_type=type(error).__name__)
        if isinstance(error, SubmitFailure):
            row['pending_possible'] = error.pending_possible
        raise
    finally:
        if completed:
            try:
                bridge.free(address)
                row['allocation_retained'] = False
            except BaseException as error:
                row.update(accepted=False, free_error=str(error), free_error_type=type(error).__name__)
        if q is not None:
            row['query_budget'] = dict(reads=q.reads, bytes=q.bytes)
        row['finished_perf_counter_ns'] = time.perf_counter_ns()


def capture(observer, dll_path, dll_sha256, *, certificate_path=None,
            bridge_factory=_AttachedBridge, query_factory=CoverQuery):
    """Execute exactly current-slot Location and Rotation, at most once each.

    Requires observer.sample(), exact action ownership and a pinned, ALREADY
    loaded Playground DLL. A pending or failed call stops the pair without retry.
    MAX_COORDINATE is a diagnostic output cap, not a certified map/collision bound.
    """
    out = dict(schema=1, accepted=False, diagnostic_getters_only=True, read_only_getters=True,
               engine_calls=False, submit_attempts=0, queue_submissions=0, process_event_route=True, native_index=0,
               actor_or_mesh_fields_written=False, activation_allowed=False, collision_verified=False,
               atomic_snapshot_verified=False, non_atomic=True, native_impl_semantics_verified=False,
               location_matrix_helper_current_verified=False, calls=[], values={},
               time_limits=dict(capture_seconds=CAPTURE_SECONDS, call_seconds=CALL_SECONDS),
               diagnostic_coordinate_abs_limit=MAX_COORDINATE,
               started_perf_counter_ns=time.perf_counter_ns(),
               gaps=['GetSlotLocationImpl calls RVA 0x1330e0; its complete current code is not checked here',
                     'Read-only getter execution does not prove collision, legal grabbing, head/eye/render alignment or full native semantics'])
    bridge = None
    deadline = time.monotonic() + CAPTURE_SECONDS
    try:
        if (type(dll_sha256) is not str or len(dll_sha256) != 64 or any(c not in '0123456789abcdef' for c in dll_sha256)):
            raise ValueError('Exact lowercase already-loaded Playground DLL SHA256 required')
        if not isinstance(dll_path, (str, Path)) or not str(dll_path):
            raise ValueError('Exact already-loaded Playground DLL path required')
        game = _ownership(observer)
        out.update(session_id=observer.session['session_id'], game_identity=copy.deepcopy(game),
                   owner_identity=copy.deepcopy(observer.identities['owner_identity']))
        chunks, out['certificate'] = _certificate(certificate_path)
        q = query_factory(observer)
        roles = surface._roles(q)
        q.finish()
        out['roles'] = roles
        _remaining(deadline)
        bridge = bridge_factory(game, dll_path, expected_sha256=dll_sha256, attach_only=True)
        out['playground_submit'] = bridge.submit_source
        for getter in ('GetSlotLocation', 'GetSlotRotation'):
            out['values'][getter] = _call(observer, bridge, query_factory, roles, getter, chunks, deadline, out)
            if out['calls'][-1].get('free_error'):
                raise RuntimeError('Surface getter completed allocation release failed; retained responsibility')
        _ownership(observer, out)
        _remaining(deadline)
        out['accepted'] = True
    except BaseException as error:
        out.update(accepted=False, rejection=str(error), rejection_type=type(error).__name__)
    finally:
        if bridge is not None:
            try:
                bridge.close()
            except BaseException as error:
                out.update(accepted=False, close_error=str(error), close_error_type=type(error).__name__)
                out.setdefault('rejection', 'Surface getter Bridge close failed: ' + str(error))
        out['submit_attempts'] = sum(c['submission'] != 'not_attempted' for c in out['calls'])
        out['queue_submissions'] = sum(c.get('submit_result') == 0 for c in out['calls'])
        out['engine_calls'] = any(c['completion_observed'] for c in out['calls'])
        out['execution_possible_unobserved'] = any(c.get('pending_possible',False) and not c['completion_observed'] for c in out['calls'])
        out['retained_allocations'] = [dict(address=c['allocation_address'], size=c['allocation_size'],
             game_identity=out.get('game_identity'), pending_possible=c.get('pending_possible',False),
             release_condition=c['allocation_release_condition']) for c in out['calls'] if c['allocation_retained']]
        out['finished_perf_counter_ns'] = time.perf_counter_ns()
    return out
