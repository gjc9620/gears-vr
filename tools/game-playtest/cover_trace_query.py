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

from cover_state import CoverQuery, struct_type
from world_state import registry_append_only
from cover_geometry_state import _geometry
from cover_bone_query import _AttachedBridge, _module, _ownership, _reference, _field, _chain, SubmitFailure
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


class TraceQuery(CoverQuery):
    """Permit registry tail growth only with all selected Trace dependencies fixed.

    CoverQuery remains strict for ordinary cover state. Actor.Trace may register
    unrelated objects while executing. The required LiveRegistryAnchorSampler
    fresh() already checks every used object's old registry slot, live name
    entry index/text, and raw identity at each Query identity revisit. The
    name-slot pointer itself has a separate first-witness reread below.
    """

    def __init__(self, observer):
        super().__init__(observer)
        if (type(self.s).__module__, type(self.s).__name__) != ('anchor_config', 'LiveRegistryAnchorSampler'):
            raise RuntimeError('Cover Trace requires the certified live registry sampler')
        self.object_header = self.headers[0][1]
        self.name_header = self.headers[1][1]
        objects, count, capacity = struct.unpack('<Qii', self.object_header)
        names, name_count, name_capacity = struct.unpack('<Qii', self.name_header)
        if (objects != self.r.objects_data or capacity != self.r.object_capacity or
                not 0 < self.r.object_count <= count <= capacity <= 4000000 or
                names != self.r.names_data or name_capacity != self.r.name_capacity or
                not 0 < self.r.name_count <= name_count <= name_capacity <= 4000000):
            raise RuntimeError('Cover Trace initial registry/resolver binding differs')
        self.initial_object_count = self.r.object_count
        self.initial_name_count = self.r.name_count
        self.last_object_count = count
        self.registry_observations = []
        self.name_slot_witnesses = {}

    def obj(self, address):
        value = super().obj(address)
        if not 0 <= value['internal_index'] < self.initial_object_count:
            raise RuntimeError('Cover Trace used object from newly registered tail')
        if not 0 <= value['name_index'] < self.initial_name_count:
            raise RuntimeError('Cover Trace used name from newly registered tail')
        name_index = value['name_index']
        if name_index not in self.name_slot_witnesses:
            entry = struct.unpack('<Q', self.read(self.r.names_data + name_index * 8, 8))[0]
            if not entry or entry != self.r.name_pointers[name_index]:
                raise RuntimeError('Cover Trace selected name slot differs from resolver')
            self.name_slot_witnesses[name_index] = entry
        return value

    def _check_registry(self):
        object_raw = self.r.read(self.r.objects_header, 16)
        name_raw = self.r.read(self.r.names_header, 16)
        _, count, _ = struct.unpack('<Qii', object_raw)
        if (not registry_append_only(self.object_header, object_raw) or
                count < self.last_object_count or name_raw != self.name_header):
            self.closing_rejection = dict(kind='selected_dependency_registry',
                                          before=self.object_header.hex(),
                                          after=object_raw.hex(),
                                          initial_names=self.name_header.hex(),
                                          current_names=name_raw.hex(),
                                          previous_object_count=self.last_object_count)
            raise RuntimeError('Cover Trace selected registry growth certificate differs')
        self.last_object_count = count
        self.registry_observations.append(dict(object_count=count,
                                               selected_objects=len(self.identities),
                                               selected_names=len(self.name_slot_witnesses),
                                               name_header_unchanged=True))

    def _check_selected_dependencies(self):
        # Keep Query.finish's identity/metadata order, but use only this one
        # registry reader. A parent finish could observe growth and then let
        # a later header reread hide a count rollback.
        self.closing_rejection = None
        self.observer.verify()
        for address, expected in self.identities.items():
            current = self.s.fresh(address)
            if current != expected:
                self.closing_rejection = dict(kind='identity', address=address, size=None,
                                              before=copy.deepcopy(expected), after=copy.deepcopy(current))
                raise RuntimeError('World-state metadata/registry changed at closing boundary')
        for (address, size), expected in self.metadata.items():
            current = self.r.read(address, size)
            if current != expected:
                self.closing_rejection = dict(kind='metadata', address=address, size=size,
                                              before=expected.hex(), after=current.hex())
                raise RuntimeError('World-state metadata/registry changed at closing boundary')
        self._check_registry()

    def finish(self):
        self._check_selected_dependencies()
        for (address, size), expected in list(self.values.items()):
            if self.read(address, size) != expected:
                raise RuntimeError('Cover Trace selected value changed at closing boundary')
        self._check_selected_dependencies()


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


class SurfaceOwnershipRejected(RuntimeError):
    """Existing rejected reads only; never a usable surface certificate."""
    def __init__(self, actor, component, owner):
        super().__init__('Edge collision component ownership differs')
        self.observation = dict(actor=copy.deepcopy(actor),
                                collision_component=copy.deepcopy(component),
                                component_owner=copy.deepcopy(owner),
                                accepted=False, collision_verified=False)


def _collection_member(q, actor, component):
    """Authenticate one explicit TraceHitInfo component, never pick a member."""
    cls = q.obj(actor['cls'])
    if (cls['name'] != 'StaticMeshCollectionActor' or cls['number'] or
            q.obj(cls['cls'])['name'] != 'Class'):
        raise RuntimeError('Edge selected component differs from Actor collision component')
    field = _field(q, actor, 'StaticMeshComponents', 'ArrayProperty')
    inner, stride, raw, count = q.array(actor, 'StaticMeshComponents', 256)
    inner_address = struct.unpack('<Q', q.read(field['address'] + q.r.profile['struct_property_struct'], 8))[0]
    if (inner['outer'] != field['address'] or inner['number'] or stride != 8 or
            inner_address != inner['address'] or q.obj(inner['cls'])['name'] != 'ComponentProperty' or
            'Property' not in [owner['name'] for owner in _chain(q, inner['cls'])]):
        raise RuntimeError('Edge collection component array type differs')
    target_address = struct.unpack('<Q', q.read(inner['address'] + q.r.profile['struct_property_struct'], 8))[0]
    target = q.obj(target_address)
    if (target['name'] != 'StaticMeshComponent' or target['number'] or
            q.obj(target['cls'])['name'] != 'Class' or
            target not in list(_chain(q, component['cls']))):
        raise RuntimeError('Edge collection selected component class differs')
    members = struct.unpack('<' + 'Q' * count, raw)
    if members.count(component['address']) != 1:
        raise RuntimeError('Edge collection selected component membership is absent or ambiguous')
    return dict(collection_class=cls, array_field=field, element_property=inner,
                element_class=target, array_header_bytes=q.value(actor, field).hex(),
                member_bytes=raw.hex())


def _surface_state(q, roles, actor, selected_component=None):
    """Pin the actual collision component transform, not just its Actor name."""
    if q.obj(actor['address']) != actor:
        raise RuntimeError('Edge surface Actor identity changed')
    if _reference(q, actor, 'WorldInfo') != roles['world']:
        raise RuntimeError('Edge surface WorldInfo differs')
    component = _reference(q, actor, 'CollisionComponent')
    collection = None
    if selected_component is not None:
        if q.obj(selected_component['address']) != selected_component:
            raise RuntimeError('Edge selected component identity changed')
        if selected_component != component:
            collection = _collection_member(q, actor, selected_component)
        component = selected_component
    owner = _reference(q, component, 'Owner') if component else None
    if not component or owner != actor:
        raise SurfaceOwnershipRejected(actor, component, owner)
    matrix_field = _field(q, component, 'LocalToWorld', 'StructProperty')
    matrix_type = q.obj(struct_type(q, matrix_field))
    if (matrix_field['size'] != 64 or matrix_type['name'] != 'Matrix' or
            q.obj(matrix_type['cls'])['name'] != 'ScriptStruct' or
            struct.unpack('<I', q.read(matrix_type['address'] + 0x88, 4))[0] != 64):
        raise RuntimeError('Edge collision LocalToWorld is not a typed Matrix')
    for index, label in enumerate(('XPlane', 'YPlane', 'ZPlane', 'WPlane')):
        plane_field = _field(q, matrix_type, label, 'StructProperty', structure=True)
        plane_type = q.obj(struct_type(q, plane_field))
        if (plane_field['size'] != 16 or plane_field['offset'] != index * 16 or
                plane_type['name'] != 'Plane' or q.obj(plane_type['cls'])['name'] != 'ScriptStruct' or
                struct.unpack('<I', q.read(plane_type['address'] + 0x88, 4))[0] != 16):
            raise RuntimeError('Edge collision Matrix Plane layout differs')
        for axis, name in enumerate(('X', 'Y', 'Z', 'W')):
            # Plane may inherit X/Y/Z from Vector. Use the existing typed
            # declaring-owner walk, which also pins each actual super link.
            field = _field(q, plane_type, name, 'FloatProperty', structure=True)
            if field['size'] != 4 or field['offset'] != axis * 4:
                raise RuntimeError('Edge collision Plane scalar layout differs')
    raw = q.value(component, matrix_field)
    matrix = struct.unpack('<16f', raw)
    if (not all(math.isfinite(x) and abs(x) <= 2 ** 24 for x in matrix) or
            any(abs(matrix[i]) > 1e-5 for i in (3, 7, 11)) or abs(matrix[15] - 1) > 1e-5):
        raise RuntimeError('Edge collision transform is not finite affine')
    result = dict(actor=copy.deepcopy(actor), component=component, world=roles['world'],
                  local_to_world_bytes=raw.hex())
    if collection is not None:
        result['collection_membership'] = collection
    return result


class HitComponentObservationRejected(RuntimeError):
    """Preserve already-read fields without implying a closing certificate."""
    def __init__(self, observation, cause):
        super().__init__(str(cause))
        self.observation = copy.deepcopy(observation)
        self.observation.update(closing_verified=False, read_completed=False,
                                rejection=str(cause), rejection_type=type(cause).__name__)


def _hit_component_observation(q, roles, actor, component):
    """Read the explicit hit's ownership without certifying a usable surface.

    A cooked collection can have an empty StaticMeshComponents declaration.
    Observe the actual hit and the three named component containers instead
    of selecting another component or relaxing the surface membership rule.
    The caller must close this query before marking the observation complete.
    """
    if q.obj(actor['address']) != actor or q.obj(component['address']) != component:
        raise RuntimeError('Hit component observation identity changed')
    if _reference(q, actor, 'WorldInfo') != roles['world']:
        raise RuntimeError('Hit component observation WorldInfo differs')
    result = dict(actor=copy.deepcopy(actor), component=copy.deepcopy(component),
                  actor_class=q.obj(actor['cls']), component_class=q.obj(component['cls']),
                  component_outer=q.obj(component['outer']) if component['outer'] else None,
                  component_owner=_reference(q, component, 'Owner'),
                  collision_component=_reference(q, actor, 'CollisionComponent'),
                  containers={}, accepted=False, collision_verified=False,
                  grab_authorized=False, body_modified=False)
    try:
        _hit_component_containers(q, actor, component, result)
    except (RuntimeError, OSError, ValueError, struct.error) as error:
        raise HitComponentObservationRejected(result, error) from error
    return result


def _hit_component_containers(q, actor, component, result):
    for name in ('StaticMeshComponents', 'Components', 'AllComponents'):
        try:
            field = _field(q, actor, name, 'ArrayProperty')
        except RuntimeError as error:
            if str(error) != 'Required bone property not found: ' + name:
                raise
            result['containers'][name] = dict(declared=False)
            continue
        inner, stride, raw, count = q.array(actor, name, 256)
        inner_link = q.read(field['address'] + q.r.profile['struct_property_struct'], 8)
        kind = q.obj(inner['cls'])['name']
        if (struct.unpack('<Q', inner_link)[0] != inner['address'] or
                stride != 8 or kind not in ('ObjectProperty', 'ComponentProperty') or
                'Property' not in [value['name'] for value in _chain(q, inner['cls'])]):
            raise RuntimeError('Hit component container type differs: ' + name)
        target = q.obj(struct.unpack('<Q', q.read(
            inner['address'] + q.r.profile['struct_property_struct'], 8))[0])
        if q.obj(target['cls'])['name'] != 'Class':
            raise RuntimeError('Hit component container element is not Class: ' + name)
        pointers = struct.unpack('<' + 'Q' * count, raw)
        result['containers'][name] = dict(declared=True, field=field,
            element_property=inner, element_class=target, count=count, stride=stride,
            header_bytes=q.value(actor, field).hex(), member_bytes=raw.hex(),
            explicit_component_occurrences=pointers.count(component['address']),
            explicit_component_derives_element=target in list(_chain(q, component['cls'])))


def capture(observer, dll_path, dll_sha256, start, end, *, extent=(0, 0, 0), trace_actors=True,
            extra_flags=TRACE_FLAGS,
            bridge_factory=_AttachedBridge, query_factory=TraceQuery,
            snapshot_surface=False, surface_actor=None, surface_component=None):
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
        if type(snapshot_surface) is not bool or (surface_actor is not None and not snapshot_surface):
            raise ValueError('Surface snapshot requires explicit observation mode')
        if surface_component is not None and (not snapshot_surface or surface_actor is None):
            raise ValueError('Selected surface component requires its already observed Actor')
        if surface_actor is not None:
            report['surface_before'] = _surface_state(q, roles, surface_actor, surface_component)
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
            hit_component = struct.unpack_from('<Q', returned, 96)[0]
            if hit_component:
                report['hit']['component_identity'] = q.obj(hit_component)
        if snapshot_surface:
            selected_actor = surface_actor or report['hit'].get('actor_identity')
            if not selected_actor:
                raise RuntimeError('Initial edge ray has no collision Actor to observe')
            selected_component = surface_component
            if actor:
                hit_component = struct.unpack_from('<Q', returned, 96)[0]
                if not hit_component:
                    raise RuntimeError('Edge Trace hit has no collision component')
                hit_component_identity = report['hit']['component_identity']
                if selected_component is not None and selected_component != hit_component_identity:
                    raise RuntimeError('Edge Trace hit a different Actor/component')
                selected_component = hit_component_identity
            report['surface_after'] = _surface_state(q, roles, selected_actor, selected_component)
            if surface_actor is not None and report['surface_before'] != report['surface_after']:
                raise RuntimeError('Edge surface transform changed across Trace')
            if actor:
                hit_component = struct.unpack_from('<Q', returned, 96)[0]
                if actor != selected_actor['address'] or hit_component != report['surface_after']['component']['address']:
                    raise RuntimeError('Edge Trace hit a different Actor/component')
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
        if isinstance(error, SurfaceOwnershipRejected):
            report['rejected_surface_observation'] = copy.deepcopy(error.observation)
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
            if isinstance(q, TraceQuery):
                report['selected_dependency_registry'] = dict(
                    initial_object_count=q.initial_object_count,
                    initial_name_count=q.initial_name_count,
                    selected_object_count=len(q.identities),
                    selected_name_count=len(q.name_slot_witnesses),
                    sampler='anchor_config.LiveRegistryAnchorSampler',
                    observations=copy.deepcopy(q.registry_observations))
        if bridge is not None:
            try:
                bridge.close()
            except BaseException as error:
                report.update(accepted=False, close_error=str(error), close_error_type=type(error).__name__)
        report['finished_perf_counter_ns'] = time.perf_counter_ns()
    return report


def capture_edge_interval(observer, dll_path, dll_sha256, start, end, outside_delta, *,
                          tolerance=1.0, maximum_calls=8, maximum_seconds=30.0,
                          capture_ray=capture):
    """Bracket one wall-to-clear transition with certified original GT rays.

    start/end are a measured blocked ray; outside_delta translates BOTH ends
    toward the chosen edge. No CoverSlot height is treated as collision data.
    All calls use world-only blocking Trace, and must retain the same owned
    session, Pawn, CoverLink, slot, Pawn position and hit actor. Failed or
    uncertain calls terminate the search immediately, without any retry.

    The output is a measured interval on ONE slice, not a certified grabbable
    edge, reach, solid wall, body sweep, or permission to enlarge CoverRange.
    Additional slices/reach/sweep evidence are still required by the live owner.
    """
    result = dict(schema=1, accepted=False, diagnostic_only=True, rays=[],
                  collision_verified=False, grab_authorized=False,
                  body_modified=False, physical_headset_acceptance=False)
    deadline = None
    try:
        start = _point(start, 'Edge TraceStart')
        end = _point(end, 'Edge TraceEnd')
        delta = _point(outside_delta, 'Edge outside translation')
        length = math.dist(start, end)
        travel = math.sqrt(sum(x * x for x in delta))
        if not .001 <= length <= 300 or not .001 <= travel <= 200:
            raise ValueError('Edge slice ray/translation outside bounded range')
        # Translations parallel to the ray do not search a wall boundary.
        if abs(sum((end[i] - start[i]) * delta[i] for i in range(3))) > 1e-5 * length * travel:
            raise ValueError('Edge translation must be perpendicular to the ray')
        if (type(tolerance) not in (int, float) or not math.isfinite(tolerance) or
                not 0 < tolerance < travel):
            raise ValueError('Edge interval tolerance must be positive and smaller than travel')
        if type(maximum_calls) is not int or not 2 <= maximum_calls <= 8:
            raise ValueError('Edge search requires two to eight bounded calls')
        if (type(maximum_seconds) not in (int, float) or not math.isfinite(maximum_seconds) or
                not 0 < maximum_seconds <= 30):
            raise ValueError('Edge search time budget must be positive and at most thirty seconds')
        required = 2
        width = travel
        while width > tolerance:
            width /= 2
            required += 1
        if required > maximum_calls:
            raise ValueError('Edge precision cannot fit the selected call budget')
        deadline = time.monotonic() + maximum_seconds
        result.update(start=start, end=end, outside_delta=delta,
                      tolerance=tolerance, maximum_calls=maximum_calls)
        cohort = actor_identity = observed_surface = None
        direction = tuple((end[i] - start[i]) / length for i in range(3))

        def ray(fraction):
            nonlocal cohort, actor_identity, observed_surface
            if len(result['rays']) >= maximum_calls or time.monotonic() >= deadline:
                raise TimeoutError('Edge search budget exhausted')
            origin = tuple(start[i] + fraction * delta[i] for i in range(3))
            target = tuple(end[i] + fraction * delta[i] for i in range(3))
            receipt = capture_ray(observer, dll_path, dll_sha256, origin, target,
                                  extent=(0, 0, 0), trace_actors=False, extra_flags=TRACE_FLAGS,
                                  snapshot_surface=True, surface_actor=actor_identity,
                                  surface_component=observed_surface['component'] if observed_surface is not None else None)
            result['rays'].append(dict(fraction=fraction, receipt=copy.deepcopy(receipt)))
            # A complete accepted Trace is the only usable observation. In
            # particular an unaccepted raw hit or pending request is not empty.
            if receipt.get('accepted') is not True or receipt.get('trace_calls') != 1:
                raise RuntimeError('Edge ray not accepted; search stopped without retry')
            if receipt.get('pending_possible') or receipt.get('allocation_retained'):
                raise RuntimeError('Edge ray has unresolved request ownership')
            if time.monotonic() >= deadline:
                raise TimeoutError('Edge search time budget exhausted after completed ray')
            identity = tuple(copy.deepcopy(receipt[key]) for key in
                             ('session_id', 'game_identity', 'owner_identity', 'roles',
                              'pawn_position', 'module', 'function'))
            if cohort is None:
                cohort = identity
            elif identity != cohort:
                raise RuntimeError('Edge search Pawn/Link/slot/session binding changed')
            current_surface = receipt['surface_after']
            if observed_surface is None:
                observed_surface = copy.deepcopy(current_surface)
            elif (receipt['surface_before'] != observed_surface or current_surface != observed_surface):
                raise RuntimeError('Edge collision surface changed between rays')
            inputs = receipt['inputs']
            if (tuple(inputs['start']) != origin or tuple(inputs['end']) != target or
                    tuple(inputs['extent']) != (0, 0, 0) or inputs['trace_actors'] is not False or
                    inputs['extra_flags'] != TRACE_FLAGS):
                raise RuntimeError('Edge ray completed input differs')
            hit = receipt['hit']
            actor = hit['actor_address']
            point = _point(hit['location'], 'Edge hit location')
            normal = _point(hit['normal'], 'Edge hit normal')
            if type(actor) is not int or actor < 0:
                raise RuntimeError('Edge hit actor invalid')
            if not actor:
                if point != (0, 0, 0) or normal != (0, 0, 0):
                    raise RuntimeError('Edge empty result has unexplained hit data')
                return False
            actor_id = hit['actor_identity']
            if current_surface['actor'] != actor_id:
                raise RuntimeError('Edge search crossed a different collision actor')
            excluded = {receipt['roles'][role]['address'] for role in ('pawn', 'controller', 'link')}
            if actor in excluded or actor_id['address'] != actor:
                raise RuntimeError('Edge hit belongs to controlled actor or cover marker')
            normal_length = math.sqrt(sum(x * x for x in normal))
            along = sum((point[i] - origin[i]) * direction[i] for i in range(3))
            error = math.sqrt(sum((point[i] - origin[i] - along * direction[i]) ** 2 for i in range(3)))
            # Explicit numerical observation tolerances, not game constants.
            if (not .99 <= normal_length <= 1.01 or abs(normal[2]) > .1 or
                    sum(normal[i] * direction[i] for i in range(3)) > -.9 or
                    not .001 < along < length - .001 or error > .01):
                raise RuntimeError('Edge hit is not an interior facing wall contact')
            if actor_identity is None:
                actor_identity = copy.deepcopy(actor_id)
            elif actor_id != actor_identity:
                raise RuntimeError('Edge search crossed a different collision actor')
            return True

        if not ray(0):
            raise RuntimeError('Edge search needs a positive blocked starting ray')
        if ray(1):
            raise RuntimeError('Edge search outside endpoint is still blocked')
        blocked, clear = 0.0, 1.0
        while (clear - blocked) * travel > tolerance:
            middle = (blocked + clear) / 2
            if ray(middle):
                blocked = middle
            else:
                clear = middle
        result.update(accepted=True, actor_identity=actor_identity,
                      surface_state=observed_surface,
                      blocked_fraction=blocked, clear_fraction=clear,
                      interval_width=(clear - blocked) * travel)
    except Exception as error:
        result.update(accepted=False, rejection=str(error), rejection_type=type(error).__name__)
    return result
