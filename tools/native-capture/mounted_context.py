"""Read-only Turret possession certificate used by mounted anchor admission.

Consumer supplies a sampler-like object with r, fresh(address), controller,
camera, local, stamp, pov and view_target. These are the existing AnchorSampler
discovery fields; call before its WarPawn-only guard. fresh must validate live
object/name registry slots (LiveRegistryAnchorSampler.fresh does this).
capture() returns independent dictionaries for config serialization. It never
substitutes Driver for Controller.Pawn or computes an eye from pawn location.
Stable repeated reads establish consistency, not an atomic engine snapshot.
"""
import copy
import math
import struct
import hashlib
import json
from sample_player_anchor import AnchorRejected, _check_registry_header
from mounted_read_pass import MountedReadPass


class MountedTimestampChanged(AnchorRejected):
    """Camera time/frame changed while all non-camera capture evidence agrees."""


class MountedContextCertificate:
    def __init__(self, sampler, *, defer_capture=False):
        self.sampler = sampler
        self._identity_hints, self._super_hints, self._link_hints = {}, {}, {}
        self._name_index_hints = {}
        self._property_hints = self._discover_properties()
        # The live guard retains discovery across timestamp-only retries. No
        # initial certificate exists until a complete strict capture succeeds.
        self._validating_world = None
        self._initial = None if defer_capture else self._capture()

    def _discover_properties(self):
        """Walk reflection lists before the camera-frame bracket.

        These are discovery hints only. _capture freshly checks every field's
        registered identity, declaring class, ancestry, layout and live offset.
        Possession changes to an undiscovered class reject the acquisition.
        """
        s, r = self.sampler, self.sampler.r
        hints, chains = {}, {}
        def discover_identity(address):
            if not address:
                raise AnchorRejected('Null mounted discovery object')
            value = dict(r.identity(address))
            self._name_index_hints[address] = r.u32(address + r.profile['object_name'])
            before = self._identity_hints.setdefault(address, value)
            if value != before:
                raise AnchorRejected('Mounted discovery identity changed')
            return value
        def ancestry(cls):
            if cls in chains:
                return chains[cls]
            current, chain = cls, []
            while current:
                if current in chain or len(chain) >= 64:
                    raise AnchorRejected('Invalid mounted discovery superclass chain')
                value = discover_identity(current)
                discover_identity(value['cls'])
                chain.append(current)
                parent = r.u64(current + r.profile['field_super'])
                previous = self._super_hints.setdefault(current, parent)
                if parent != previous:
                    raise AnchorRejected('Mounted discovery superclass changed')
                current = parent
            chains[cls] = chain
            return chain
        def prepare(address, names):
            cls = discover_identity(address)['cls']
            # Bound/cycle-check the ancestry before asking a resolver to walk
            # it. Resolver implementations (including test doubles) need not
            # supply their own bound. Strict capture independently rereads it.
            ancestry(cls)
            for name in names:
                if (cls, name) not in hints:
                    value = dict(r.property(cls, name, True))
                    hints[cls, name] = value
                    field = discover_identity(value['address'])
                    discover_identity(field['cls'])
            return cls
        def follow(address, offset):
            if type(offset) is not int or not 0 <= offset <= 0xfff8:
                raise AnchorRejected('Unsupported mounted discovery offset')
            location = address + offset
            value = r.u64(location)
            if self._link_hints.setdefault(location, value) != value:
                raise AnchorRejected('Mounted discovery possession changed')
            return value
        pc = prepare(s.controller, ('Pawn', 'PlayerCamera', 'Player', 'WorldInfo'))
        controlled = follow(s.controller, hints[pc, 'Pawn']['offset'])
        turret = prepare(controlled, ('Driver', 'Controller', 'WorldInfo'))
        driver = follow(controlled, hints[turret, 'Driver']['offset'])
        pawn = prepare(driver, ('DrivenVehicle', 'Controller', 'WorldInfo', 'InvManager'))
        prepare(s.camera, ('PCOwner', 'WorldInfo'))
        inventory = follow(driver, hints[pawn, 'InvManager']['offset'])
        prepare(inventory, ('Owner',))
        return hints

    def capture(self):
        value = self._capture()
        if self._initial is None:
            self._initial = copy.deepcopy(value)
            return copy.deepcopy(value)
        # Possession changes require a new immutable certificate/rebind. Camera
        # timestamp and POV may advance between separate successful captures.
        for key in ('roles', 'levels', 'identities', 'supers', 'properties', 'links'):
            if value[key] != self._initial[key]:
                raise AnchorRejected('Mounted certificate changed: ' + key)
        return copy.deepcopy(value)

    def certifies_world(self, identity):
        # Cinematic target validation runs inside the sampler's mounted capture
        # bracket. It may share this exact world while the actor Levels differ.
        if self._initial is not None:return identity == self._initial['roles']['world_info']
        # Only during this capture's outer target validator. Its opening world
        # is provisional; the full outer closing proof must still pass before
        # capture() commits any immutable certificate.
        return self._validating_world is not None and identity == self._validating_world

    def _capture(self):
        s, r = self.sampler, self.sampler.r
        identities, supers, properties, links, reads = {}, {}, {}, {}, {}
        chains = {}
        planned = dict(self._identity_hints)
        if getattr(self, '_initial', None) is not None:
            planned.update(self._initial['identities'])
        names = dict(self._name_index_hints)
        names.update({address: value['name_index'] for address, value in planned.items()
                      if 'name_index' in value})
        opening = MountedReadPass(r, planned, names)
        closing = None

        def fresh(address, read_pass):
            with read_pass.bind(s):
                return s.fresh(address)

        def raw(address, size):
            if not address or address < 0 or address > (1 << 64) - size:
                raise AnchorRejected('Invalid mounted memory address')
            value = r.read(address, size)
            if len(value) != size:
                raise AnchorRejected('Short mounted memory read')
            key = address, size
            if key in reads and reads[key] != value:
                raise AnchorRejected('Mounted memory changed during discovery')
            reads[key] = value
            return value

        def pointer(address):
            value = struct.unpack('<Q', raw(address, 8))[0]
            if address in self._link_hints and value != self._link_hints[address]:
                raise AnchorRejected('Mounted possession changed since discovery')
            return value

        def identity(address):
            # Reuse only within this capture. Every visited identity is freshly
            # revalidated below, including class and property identities.
            if address in identities:
                return identities[address]
            value = fresh(address, opening)
            hint = self._identity_hints.get(address)
            if hint is not None and any(value.get(key) != expected for key, expected in hint.items()):
                raise AnchorRejected('Mounted identity changed since discovery')
            if not value['name'] or value['name'].startswith('Default__'):
                raise AnchorRejected('Default/empty mounted identity')
            identities[address] = copy.deepcopy(value)
            return value

        def chain(cls):
            if cls in chains:
                return chains[cls]
            start = cls
            result = []
            while cls:
                if cls in result or len(result) >= 64:
                    raise AnchorRejected('Invalid mounted superclass chain')
                value = identity(cls)
                meta = identity(value['cls'])
                if meta['name'] != 'Class' or meta['number'] or value['number']:
                    raise AnchorRejected('Mounted ancestry is not registered classes')
                result.append(cls)
                supers[cls] = pointer(cls + r.profile['field_super'])
                if cls in self._super_hints and supers[cls] != self._super_hints[cls]:
                    raise AnchorRejected('Mounted superclass changed since discovery')
                cls = supers[cls]
            chains[start] = result
            return result

        def derives(obj, name):
            if not any(identities[x]['name'] == name for x in chain(obj['cls'])):
                raise AnchorRejected('Mounted class mismatch: ' + name)

        def prop(obj, name, owner_name):
            value = self._property_hints.get((obj['cls'], name))
            if value is None:
                raise AnchorRejected('Mounted property discovery changed: ' + name)
            offset = value['offset']
            if value['type'] != 'ObjectProperty' or type(offset) is not int or not 0 <= offset <= 0xfff8:
                raise AnchorRejected('Unsupported mounted property: ' + name)
            field = identity(value['address'])
            kind = identity(field['cls'])
            owner = identity(field['outer'])
            if (field['name'] != name or field['number'] or kind['name'] != 'ObjectProperty' or
                    kind['number'] or identity(kind['cls'])['name'] != 'Class' or
                    owner['name'] != owner_name or owner['number'] or
                    owner['address'] not in chain(obj['cls'])):
                raise AnchorRejected('Mounted property owner/type mismatch: ' + name)
            layout = struct.unpack('<II', raw(field['address'] + 0x70, 8))
            live_offset = struct.unpack('<I', raw(field['address'] + r.profile['property_offset_internal'], 4))[0]
            if layout != (1, 8) or live_offset != offset:
                raise AnchorRejected('Mounted property layout mismatch: ' + name)
            properties[name] = dict(identity=field, offset=offset, owner=owner_name)
            return offset

        def link(label, obj, name, owner, expected=None):
            offset = prop(obj, name, owner)
            value = pointer(obj['address'] + offset)
            if expected is not None and value != expected:
                raise AnchorRejected('Mounted reciprocal link mismatch: ' + label)
            links[label] = dict(address=obj['address'] + offset, value=value)
            return value

        # Outer bracket: all registered identities, reflected layouts, ancestry
        # and relationships remain independently checked at both boundaries.
        # Camera time does not cover the expensive metadata validation.
        for header in (r.objects_header, r.names_header):
            data, count, capacity = struct.unpack('<Qii', raw(header, 16))
            if not data or not 0 < count <= capacity < 4000000:
                raise AnchorRejected('Invalid mounted registry header')
        pc, camera, local = identity(s.controller), identity(s.camera), identity(s.local)
        controlled = identity(link('controlled', pc, 'Pawn', 'Controller'))
        derives(controlled, 'Turret')
        derives(controlled, 'Vehicle')
        driver = identity(link('driver', controlled, 'Driver', 'Vehicle'))
        derives(driver, 'WarPawn')
        if len({pc['address'], camera['address'], local['address'], controlled['address'], driver['address']}) != 5:
            raise AnchorRejected('Mounted role alias')
        link('possession', controlled, 'Controller', 'Pawn', pc['address'])
        link('driven_vehicle', driver, 'DrivenVehicle', 'Pawn', controlled['address'])
        # Vehicle.DriverEnter calls C.UnPossess before C.Possess(vehicle).
        link('driver_controller', driver, 'Controller', 'Pawn', 0)
        link('camera', pc, 'PlayerCamera', 'PlayerController', camera['address'])
        link('camera_owner', camera, 'PCOwner', 'Camera', pc['address'])
        link('local', pc, 'Player', 'PlayerController', local['address'])
        world = identity(link('world', pc, 'WorldInfo', 'Actor'))
        derives(world, 'WorldInfo')
        for label, obj in (('controlled_world', controlled), ('driver_world', driver), ('camera_world', camera)):
            link(label, obj, 'WorldInfo', 'Actor', world['address'])
        level = identity(controlled['outer'])
        derives(level, 'Level')
        # Streamed actors can have different immediate Levels while sharing the
        # reflected Actor.WorldInfo above. Freeze every actual Level identity;
        # a name or an arbitrary outer pointer is never sufficient evidence.
        levels = {}
        for label, obj in (('controlled', controlled), ('driver', driver),
                           ('controller', pc), ('camera', camera), ('world_info', world)):
            owner = identity(obj['outer'])
            derives(owner, 'Level')
            if owner['cls'] != level['cls']:
                raise AnchorRejected('Mounted Level class mismatch')
            levels[label] = owner
        inventory = identity(link('inventory', driver, 'InvManager', 'Pawn'))
        derives(inventory, 'WarInventoryManager')
        # InventoryManager is an Actor contained by the Driver's registered
        # Level; Actor.Owner, not UObject.Outer, establishes gameplay ownership.
        if inventory['outer'] != levels['driver']['address']:
            raise AnchorRejected('Mounted inventory Level mismatch')
        link('inventory_owner', inventory, 'Owner', 'Actor', driver['address'])
        def validate_optional_target():
            validator = getattr(s, 'target_validator', None)
            if validator is None:return None
            self._validating_world = world
            try:return validator(s, controlled['address'], controlled, level)
            finally:self._validating_world = None
        target_proof = validate_optional_target()
        # Inner camera bracket starts only after the outer opening validation.
        stamp = raw(s.stamp, 4)
        if not math.isfinite(struct.unpack('<f', stamp)[0]):
            raise AnchorRejected('Invalid mounted camera timestamp')
        target = pointer(s.view_target) if s.view_target else None
        if not s.view_target or target != controlled['address']:
            closing = MountedReadPass(r, identities, {a: x['name_index'] for a, x in identities.items()})
            error = AnchorRejected('Mounted camera target is not controlled turret')
            observation = dict(diagnostic_only=True, accepted=False, consistent_read=False,
                atomic_snapshot=False, roles=copy.deepcopy(dict(controlled=controlled, driver=driver,
                    controller=pc, camera=camera)), target_field=dict(address=s.view_target, value=target),
                target_identity=None, target_class=None, target_is_driver=target == driver['address'],
                timestamp=dict(address=s.stamp, expected_raw=stamp.hex(), observed_raw=None, consistent=False),
                headers={}, mismatches=[])

            def diagnostic_error(stage, address, failure, size=None):
                if 'read_error' not in observation:
                    observation['read_error'] = dict(stage=stage, address=address, size=size,
                        exception_type=type(failure).__name__, error=str(failure)[:256])

            def mismatch(stage, address, size=None):
                # Every captured item is checked; only the first eight changes
                # are retained. This is a failure receipt, never retry authority.
                if len(observation['mismatches']) < 8:
                    observation['mismatches'].append(dict(stage=stage, address=address, size=size))

            # Reuse this capture's identities, including a target already seen
            # as Driver. Add at most the foreign target and its exact class.
            if target:
                try:
                    item = identities.get(target)
                    if item is None:
                        item = copy.deepcopy(fresh(target, opening)); identities[target] = item
                    observation['target_identity'] = copy.deepcopy(item)
                    cls = item['cls']; kind = identities.get(cls)
                    if kind is None:
                        kind = copy.deepcopy(fresh(cls, opening)); identities[cls] = kind
                    observation['target_class'] = copy.deepcopy(kind)
                except Exception as failure:
                    diagnostic_error('target_identity', target, failure)
            if not s.view_target:
                diagnostic_error('target_field', s.view_target, AnchorRejected('Missing mounted target field'), 8)
            for address, expected in identities.items():
                try:
                    if fresh(address, closing) != expected: mismatch('identity', address)
                except Exception as failure:
                    diagnostic_error('identity', address, failure)
            for (address, size), expected in reads.items():
                try:
                    if r.read(address, size) != expected: mismatch('memory', address, size)
                except Exception as failure:
                    diagnostic_error('memory', address, failure, size)
            # Close the diagnostic bracket with current headers, target and
            # timestamp. Equality is consistency evidence, not atomicity.
            for header, label in ((r.objects_header, 'objects'), (r.names_header, 'names')):
                expected = reads[header, 16]
                record = dict(address=header, expected_raw=expected.hex(), observed_raw=None, consistent=False)
                observation['headers'][label] = record
                try:
                    observed = r.read(header, 16); record.update(observed_raw=observed.hex(), consistent=observed == expected)
                    if observed != expected: mismatch('header', header, 16)
                except Exception as failure:
                    diagnostic_error('header', header, failure, 16)
            if s.view_target:
                try:
                    observed = r.read(s.view_target, 8)
                    observation['target_field']['observed_value'] = struct.unpack('<Q', observed)[0]
                    if observed != reads[s.view_target, 8]: mismatch('target', s.view_target, 8)
                except Exception as failure:
                    diagnostic_error('target_field', s.view_target, failure, 8)
            try:
                observed = r.read(s.stamp, 4)
                observation['timestamp'].update(observed_raw=observed.hex(), consistent=observed == stamp)
                if observed != stamp: mismatch('timestamp', s.stamp, 4)
            except Exception as failure:
                diagnostic_error('timestamp', s.stamp, failure, 4)
            observation['captured_identity_count'] = len(identities)
            observation['captured_read_count'] = len(reads)
            observation['consistent_read'] = not observation['mismatches'] and 'read_error' not in observation
            error.camera_observation = observation
            raise error
        pov = raw(s.pov, 28)
        values = struct.unpack('<3f3if', pov)
        if not all(math.isfinite(x) for x in (*values[:3], values[-1])) or not 0 < values[-1] < 180:
            raise AnchorRejected('Invalid mounted camera POV')
        roles = dict(controller=pc, camera=camera, local_player=local,
                     controlled=controlled, driver=driver, inventory_pawn=driver,
                     inventory=inventory, camera_target=controlled, level=level, world_info=world)
        role_ids = {x['address']: x for x in (*roles.values(), *levels.values())}
        def inner_boundary():
            # New buffers at each endpoint, never a reused opening pass.
            current = MountedReadPass(r, role_ids, {a: x['name_index'] for a, x in role_ids.items()})
            actual_roles = {}
            for address, expected in role_ids.items():
                actual_roles[address] = fresh(address, current)
                if actual_roles[address] != expected:
                    raise AnchorRejected('Mounted role identity changed inside camera bracket')
            actual_links = {}
            for label, item in links.items():
                observed = r.read(item['address'], 8)
                actual_links[label] = dict(address=item['address'], raw=observed.hex())
                if observed != struct.pack('<Q', item['value']):
                    raise AnchorRejected('Mounted relationship changed inside camera bracket')
            actual_target = r.read(s.view_target, 8)
            if actual_target != struct.pack('<Q', controlled['address']):
                raise AnchorRejected('Mounted target changed inside camera bracket')
            actual_pov = r.read(s.pov, 28)
            if len(actual_pov) != 28:
                raise AnchorRejected('Short mounted camera POV read')
            actual_headers = {}
            for header, registry in ((r.objects_header, 'objects'), (r.names_header, 'names')):
                observed = r.read(header, 16); actual_headers[registry] = observed.hex()
                _check_registry_header(registry, observed, struct.unpack('<Qii', reads[header, 16]))
            rotation_field = getattr(s, 'layout', {}).get('controller_rotation')
            rotation_raw = None
            if rotation_field is not None:
                rotation_raw = r.read(s.controller + rotation_field['offset'], 12)
                if len(rotation_raw) != 12:raise AnchorRejected('Short mounted controller rotation read')
            observed_stamp = r.read(s.stamp, 4)
            if len(observed_stamp) != 4 or not math.isfinite(struct.unpack('<f', observed_stamp)[0]):
                raise AnchorRejected('Invalid mounted camera timestamp at bracket boundary')
            return dict(roles=actual_roles, links=actual_links, target_raw=actual_target.hex(),
                        pov_raw=actual_pov.hex(), headers=actual_headers, timestamp_raw=observed_stamp.hex(),
                        controller_rotation_raw=None if rotation_raw is None else rotation_raw.hex())
        inner_before = inner_boundary()
        inner_after = inner_boundary()
        timestamp_changed = any(x['timestamp_raw'] != stamp.hex() for x in (inner_before, inner_after))
        if ({k:v for k,v in inner_before.items() if k not in ('timestamp_raw', 'pov_raw')} !=
                {k:v for k,v in inner_after.items() if k not in ('timestamp_raw', 'pov_raw')}):
            raise AnchorRejected('Mounted inner endpoint values changed')
        # Finish the independent OUTER validation even on frame rollover, so
        # timestamp retries never hide identity/layout/link/registry failures.
        closing = MountedReadPass(r, identities, {a: x['name_index'] for a, x in identities.items()})
        outer_after_ids, outer_after_reads = {}, {}
        for address, expected in identities.items():
            outer_after_ids[address] = fresh(address, closing)
            if outer_after_ids[address] != expected:
                raise AnchorRejected('Mounted identity changed at bracket boundary')
        for (address, size), expected in reads.items():
            if (address, size) == (s.stamp, 4) or (address, size) == (s.pov, 28):
                continue  # Camera bytes were checked only in the inner bracket.
            observed = r.read(address, size)
            outer_after_reads[address, size] = observed
            if address in (r.objects_header, r.names_header) and size == 16:
                registry = 'objects' if address == r.objects_header else 'names'
                _check_registry_header(registry, observed, struct.unpack('<Qii', expected))
            elif observed != expected:
                raise AnchorRejected('Mounted memory changed at bracket boundary')
        for header, registry in ((r.objects_header, 'objects'), (r.names_header, 'names')):
            _check_registry_header(registry, r.read(header, 16),
                                   struct.unpack('<Qii', reads[header, 16]))
        after_target_proof = validate_optional_target()
        if after_target_proof != target_proof:
            raise AnchorRejected('Mounted cinematic target validation changed')
        if timestamp_changed:
            raise MountedTimestampChanged('Mounted camera timestamp changed at bracket boundary')
        if any(x['pov_raw'] != pov.hex() for x in (inner_before, inner_after)):
            raise AnchorRejected('Mounted POV changed inside camera bracket')
        def digest(ids, memory):
            payload = dict(identities=ids, reads=[(a, n, v.hex()) for (a, n), v in sorted(memory.items())])
            return hashlib.sha256(json.dumps(payload, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        outer_before_reads = {key: value for key, value in reads.items()
                              if key not in ((s.stamp, 4), (s.pov, 28))}
        validation = dict(contract='outer_identity_layout_twice_inner_camera_roles_links_pov_twice',
                          inner_before=inner_before, inner_after=inner_after,
                          outer_before_sha256=digest(identities, outer_before_reads),
                          outer_after_sha256=digest(outer_after_ids, outer_after_reads),
                          outer_identity_count=len(identities), outer_read_count=len(outer_before_reads),
                          atomic_snapshot=False)
        result = dict(schema=1, kind='turret_driver', roles=roles, levels=levels, identities=identities,
                    supers=supers, properties=properties, links=links,
                    camera_timestamp_raw=stamp.hex(), camera_pov_raw=pov.hex(),
                    validation=validation,
                    atomic_snapshot=False, runtime_verified=False)
        if target_proof is not None:result['target_proof'] = target_proof
        return result


def mounted_guard_result(proof, boundary='inner_after'):
    """Map an independently read inner endpoint to existing serialized roles.

    This is not a second full guard. The proof explicitly contains separate
    inner reads and digests of both complete outer validation boundaries.
    """
    v = proof.get('validation')
    if not isinstance(v,dict) or not isinstance(v.get('inner_before'),dict) or not isinstance(v.get('inner_after'),dict):
        raise AnchorRejected('Missing mounted two-layer validation evidence')
    before, after = v['inner_before'], v['inner_after']
    if (v['contract'] != 'outer_identity_layout_twice_inner_camera_roles_links_pov_twice' or
            before != after or v['outer_before_sha256'] != v['outer_after_sha256'] or
            before['timestamp_raw'] != proof['camera_timestamp_raw'] or
            before['pov_raw'] != proof['camera_pov_raw']):
        raise AnchorRejected('Invalid mounted two-layer validation evidence')
    current = v[boundary]; roles = proof['roles']
    def role(name):
        item = roles[name]
        value = current['roles'][item['address']]
        if value != item:raise AnchorRejected('Mounted endpoint role mismatch')
        return copy.deepcopy(value)
    result = dict(pawn=role('controlled'), controller=role('controller'), camera=role('camera'),
                local_player=role('local_player'), level=role('level'), outer_chain=[role('level')],
                view_target=struct.unpack('<Q', bytes.fromhex(current['target_raw']))[0],
                timestamp_raw=current['timestamp_raw'], objects_header=current['headers']['objects'],
                names_header=current['headers']['names'], mounted_context=proof)
    if 'target_proof' in proof:result['target_proof'] = copy.deepcopy(proof['target_proof'])
    return result


MOUNTED_PROPERTIES = ('Pawn', 'Controller', 'Driver', 'DrivenVehicle', 'PlayerCamera',
                      'PCOwner', 'Player', 'WorldInfo', 'InvManager', 'Owner')
MOUNTED_OWNERS = ('Controller', 'Pawn', 'Vehicle', 'Pawn', 'PlayerController',
                  'Camera', 'PlayerController', 'Actor', 'Pawn', 'Actor')


def validate_mounted_config(config):
    """Check serialized semantics before native independently validates live bytes."""
    m=config['mounted'];ids=config['identities'];roles=config['roles']
    if set(m)!={'role_driver','role_inventory','role_world_info','property_offset_internal','properties'}:
        raise AnchorRejected('Incomplete mounted certificate')
    for key in ('role_driver','role_inventory','role_world_info'):
        if type(m[key]) is not int or not 0<=m[key]<len(ids):raise AnchorRejected('Invalid mounted role')
    if type(m['property_offset_internal']) is not int or not 0<=m['property_offset_internal']<=0xfffc:
        raise AnchorRejected('Invalid mounted property offset')
    if len(set([*roles.values(),m['role_driver'],m['role_inventory'],m['role_world_info']]))!=8:
        raise AnchorRejected('Aliased mounted roles')
    by_address={i['address']:i for i in ids}
    def ancestry(cls,name):
        seen=set()
        while cls:
            item=by_address.get(cls)
            if (cls in seen or len(seen)>=64 or not item or not item['check_super'] or item['number'] or
                    by_address.get(item['cls'],{}).get('name')!='Class'):
                raise AnchorRejected('Invalid mounted ancestry')
            if item['name']==name:return True
            seen.add(cls);cls=item['super']
        return False
    pawn=ids[roles['pawn']];driver=ids[m['role_driver']];inventory=ids[m['role_inventory']]
    world=ids[m['role_world_info']];level=ids[roles['level']]
    for item,name in ((pawn,'Turret'),(pawn,'Vehicle'),(driver,'WarPawn'),(inventory,'WarInventoryManager'),(world,'WorldInfo')):
        if item['name'].startswith('Default__') or not ancestry(item['cls'],name):raise AnchorRejected('Mounted role class mismatch')
    if pawn['outer']!=level['address'] or inventory['outer']!=driver['outer']:
        raise AnchorRejected('Mounted role owner mismatch')
    for obj in (pawn,driver,world,ids[roles['controller']],ids[roles['camera']]):
        owner=by_address.get(obj['outer'])
        if (not owner or not owner['name'] or owner['name'].startswith('Default__') or
                owner['cls']!=level['cls'] or not ancestry(owner['cls'],'Level')):
            raise AnchorRejected('Missing or invalid mounted Level identity')
    if set(m['properties'])!=set(MOUNTED_PROPERTIES):raise AnchorRejected('Missing mounted property')
    for name,owner in zip(MOUNTED_PROPERTIES,MOUNTED_OWNERS):
        value=m['properties'][name]
        if set(value)!={'role','offset'} or type(value['role']) is not int or not 0<=value['role']<len(ids) or type(value['offset']) is not int or not 0<=value['offset']<=0xfff8:
            raise AnchorRejected('Invalid mounted property')
        field=ids[value['role']];kind=by_address.get(field['cls'],{});owner_id=by_address.get(field['outer'],{})
        if field['name']!=name or field['number'] or kind.get('name')!='ObjectProperty' or owner_id.get('name')!=owner or owner_id.get('number'):
            raise AnchorRejected('Mounted property identity mismatch')
    for field,name in (('controller_pawn','Pawn'),('pawn_controller','Controller'),('player_camera','PlayerCamera'),('pc_owner','PCOwner'),('player','Player')):
        if config['fields'][field]!=m['properties'][name]['offset']:raise AnchorRejected('Mounted field mismatch')
    return m



def mounted_crosshair_suspension(anchor,pid,base,sha256):
    validate_mounted_config(anchor)
    m=anchor['mounted'];ids=anchor['identities']
    return dict(schema=2,native_contract=2,pid=pid,base=base,sha256=sha256,
        controller=ids[anchor['roles']['controller']]['address'],
        controlled=ids[anchor['roles']['pawn']]['address'],driver=ids[m['role_driver']]['address'],
        camera=ids[anchor['roles']['camera']]['address'])


def mounted_crosshair_ini(proof):
    if set(proof)!={'schema','native_contract','pid','base','sha256','controller','controlled','driver','camera'} or proof['schema']!=2 or proof['native_contract']!=2:
        raise AnchorRejected('Invalid mounted crosshair suspension')
    for key,value in proof.items():
        if key=='sha256':
            if value!='bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4':raise AnchorRejected('Suspension build mismatch')
        elif type(value) is not int or not 0<value<1<<64:raise AnchorRejected('Invalid suspension integer')
    return '[CrosshairAssignment]\n'+'\n'.join(f'{key}={value}' for key,value in proof.items())+'\n'
