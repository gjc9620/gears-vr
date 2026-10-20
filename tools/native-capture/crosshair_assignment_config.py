"""Fresh read-only assignment certificate; no process opening, calls or hooks.

Caller supplies a prepared Resolver and its current camera identity. Returned
JSON is activation evidence, never proof of a tick, assignment or scene.
"""
import hashlib
import json
import struct
import time

from ui_observer_config import GAME_SHA, LAYOUT

HISTORICAL_CODE_SHA = '911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6'
NATIVE_BYTES = {
    0x1d7ad0: '48895c240848896c2418488974242057415641574883ec30488bda4c8d358ee7',
    0x1d5ee5: '8b4178',
    0x1d5ef4: '480fbae0167234',
    0x1d5efb: '8b4174488bd60faf4170488bcd4c63c0e84f1f3a00',
}
SLOTS = {0x0f: 0x1d7ad0, 0x35: 0x1d6750, 0: 0x1d7eb0, 1: 0x1d8570}
ROLES = ('writer war_pc controller pawn weapon war_pawn war_weapon lhs member '
         'impact impact_struct vector_struct ww').split()
LINKS = ('controller_pawn pawn_controller pawn_weapon weapon_instigator').split()
VIEW_FIELDS = dict(cam_loc=('CamLoc', 'Vector'), cam_rot=('CamRot', 'Rotator'),
                   start_trace=('StartTrace', 'Vector'), trace_dir=('TraceDir', 'Vector'))


class CrosshairConfigRejected(RuntimeError):
    pass


class CrosshairRegistryChanged(CrosshairConfigRejected):
    """A complete objects/names header changed beyond monotonic count growth."""


class CrosshairControlledLinkChanged(CrosshairConfigRejected):
    """Only certified role-link values changed in the final read bracket.

    This rejects the whole acquisition; it never accepts a replacement pointer.
    """
    def __init__(self, evidence):
        self.evidence = evidence
        super().__init__('Certified controlled-role link changed before completion: ' +
                         ', '.join(change['field'] for change in evidence['changes']))


def _require(ok, reason):
    if not ok:
        raise CrosshairConfigRejected(reason)


def build_crosshair_assignment_config(resolver, camera, *, include_tick_roots=False, include_writer_view=False, include_aim_state=False, entry_reader=None):
    """Certify selected evidence, allowing only bounded registry count growth.

Requires original execLet bytes, read directly at startup or through the
caller's bound owned-hook attestation capability during world recovery.
Shared VM entry bytes are deliberately not certified (the UI owner may own it).
"""
    try:
        return _build(resolver, camera, include_tick_roots, include_writer_view, include_aim_state, entry_reader)
    except CrosshairConfigRejected:
        raise
    except (KeyError, TypeError, ValueError, RuntimeError, OSError, struct.error, UnicodeError) as e:
        raise CrosshairConfigRejected('Fresh certificate rejected: ' + str(e)) from e


def _build(r, camera, include_tick_roots, include_writer_view, include_aim_state, entry_reader):
    p = r.profile
    started = time.perf_counter_ns()
    _require(p.get('sha256') == camera.get('sha256') == GAME_SHA and
             camera.get('pid') == r.pid and int(camera['module_base'], 0) == r.base,
             'Current camera/process/build mismatch')
    layout = {k: p[k] for k in LAYOUT}
    _require(all(type(v) is int and 0 <= v < 0x60 for k, v in layout.items()
                 if k.startswith('object_')), 'Unsupported identity layout')
    _require(p['property_offset_internal'] == 0x94 and
             0x60 <= p['struct_property_struct'] < 0x10000, 'Unsupported property layout')
    checks = {}

    def registry_header(a, n):
        return n == 16 and a in (r.objects_header, r.names_header)

    def changed(a, n, phase):
        error = CrosshairRegistryChanged if registry_header(a, n) else CrosshairConfigRejected
        raise error(f'{phase}: address={a:#x} size={n}')

    def registry_parts(value):
        ptr, count, cap = struct.unpack('<Qii', value)
        _require(ptr and 1000 < count <= cap < 4000000, 'Invalid registry')
        return ptr, count, cap

    def registry_append(previous, current):
        old_ptr, old_count, old_cap = registry_parts(previous)
        ptr, count, cap = registry_parts(current)
        return ptr == old_ptr and cap == old_cap and count >= old_count

    def stable(a, n):
        _require(type(a) is int and 0 < a < (1 << 64) - n, 'Invalid read range')
        x = r.read(a, n)
        y = r.read(a, n)
        _require(len(x) == n and len(y) == n, f'Incomplete certificate read: address={a:#x} size={n}')
        if registry_header(a, n):
            # Validate both complete observations before classifying a change.
            # Remember the latest count so growth followed by shrink rejects.
            registry_parts(x); registry_parts(y)
            if (a, n) in checks and not registry_append(checks[a, n], x):
                changed(a, n, 'Certificate read changed during assembly')
            if not registry_append(x, y):
                changed(a, n, 'Certificate double-read changed')
            checks[a, n] = y
            # Selected identities stay bounded by the first observed count.
            return x
        if y != x:
            changed(a, n, 'Certificate double-read changed')
        if (a, n) in checks:
            if checks[a, n] != x:
                changed(a, n, 'Certificate read changed during assembly')
        checks[a, n] = x
        return x

    def number(a, fmt='<Q'):
        return struct.unpack(fmt, stable(a, struct.calcsize(fmt)))[0]

    def registry(a):
        ptr, count, _ = registry_parts(stable(a, 16))
        return ptr, count

    objects, oc = registry(r.objects_header)
    names, nc = registry(r.names_header)

    def fresh(a):
        idx = number(a + p['object_internal_index'], '<I')
        ni, serial = struct.unpack('<II', stable(a + p['object_name'], 8))
        cls = number(a + p['object_class']); outer = number(a + p['object_outer'])
        _require(idx < oc and number(objects + idx * 8) == a and ni < nc, 'Object registry identity mismatch')
        entry = number(names + ni * 8)
        _require(entry and number(entry + p['name_entry_index'], '<I') == ni, 'Name registry mismatch')
        raw = stable(entry + p['name_entry_text'], 128)
        end = next((i for i in range(0, 128, 2) if raw[i:i+2] == b'\0\0'), None)
        _require(end is not None and end > 0, 'Invalid bounded name')
        name = raw[:end].decode('utf-16-le')
        _require(not any(c in name for c in '\r\n\0='), 'Invalid name')
        return dict(address=a, internal_index=idx, name_index=ni, number=serial,
                    name=name, cls=cls, outer=outer)

    ids = []; indexes = {}; supers = {}

    def add(a):
        item = fresh(a)
        if a in indexes:
            _require(ids[indexes[a]] == item, 'Identity changed')
            return indexes[a]
        _require(len(ids) < 96, 'Identity capacity exceeded')
        indexes[a] = len(ids); ids.append(item)
        return indexes[a]

    def exact(a, name, kind, outer=None):
        item = fresh(a)
        _require(item['name'] == name and not item['number'] and
                 fresh(item['cls'])['name'] == kind and
                 (outer is None or item['outer'] == outer), 'Wrong identity/type/outer: ' + name)
        add(a)
        return a

    def ancestors(cls):
        seen = set()
        while cls:
            _require(cls not in seen and len(seen) < 40, 'Invalid superclass chain')
            seen.add(cls)
            exact(cls, fresh(cls)['name'], 'Class')
            parent = number(cls + p['field_super']); supers[cls] = parent
            yield cls
            cls = parent

    def role_class(obj, name):
        _require(not fresh(obj)['name'].startswith('Default__'), 'Default object')
        chain = list(ancestors(fresh(obj)['cls']))
        matches = [a for a in chain if fresh(a)['name'] == name]
        _require(len(matches) == 1, 'Role does not derive from ' + name)
        return matches[0]

    def property_at(owner, name, kind, inherited=False):
        hint = r.property(owner, name, inherited)
        a = hint['address']; item = fresh(a)
        exact(a, name, kind)
        _require(hint['type'] == kind, 'Resolver property type mismatch')
        allowed = list(ancestors(owner)) if inherited else [owner]
        _require(item['outer'] in allowed, 'Property owner mismatch')
        offset = number(a + 0x94, '<I')
        _require(offset == hint['offset'] and offset < 65536, 'Property offset mismatch')
        return a, offset

    def link(obj, name, target=None):
        prop, offset = property_at(fresh(obj)['cls'], name, 'ObjectProperty', True)
        value = number(obj + offset)
        _require(value and (target is None or value == target), 'Current link mismatch: ' + name)
        add(value)
        return value, dict(property=add(prop), offset=offset)

    pc = int(camera['controller_address'], 0); cam = int(camera['camera_address'], 0)
    war_pc = role_class(pc, 'WarPC'); add(pc)
    link(pc, 'PlayerCamera', cam); link(cam, 'PCOwner', pc)
    pawn, pc_pawn = link(pc, 'Pawn'); war_pawn = role_class(pawn, 'WarPawn')
    if camera.get('pawn_address') is not None:
        _require(pawn == int(camera['pawn_address'], 0), 'Camera pawn mismatch')
    _, pawn_pc = link(pawn, 'Controller', pc)
    weapon, pawn_weapon = link(pawn, 'Weapon'); war_weapon = role_class(weapon, 'WarWeapon')
    _, weapon_pawn = link(weapon, 'Instigator', pawn)
    # Only these four exact pointer spans may produce a deferred final-bracket
    # transition. Their owner/property identities and offsets were authenticated
    # above; PlayerCamera/PCOwner and all assembly/double-read changes stay hard.
    controlled_links = {}
    for role, owner_role, owner, name, field in (
            ('controller_pawn', 'controller', pc, 'Pawn', pc_pawn),
            ('pawn_controller', 'pawn', pawn, 'Controller', pawn_pc),
            ('pawn_weapon', 'pawn', pawn, 'Weapon', pawn_weapon),
            ('weapon_instigator', 'weapon', weapon, 'Instigator', weapon_pawn)):
        span = (owner + field['offset'], 8)
        _require(span not in controlled_links, 'Controlled link spans overlap')
        controlled_links[span] = dict(role=role, owner_role=owner_role, owner=owner,
            field=name, property=ids[field['property']]['address'], offset=field['offset'])

    def child(owner, name, kind):
        found = [f['address'] for f in r.fields(owner) if f['name'] == name]
        _require(len(found) == 1, 'Missing/ambiguous child: ' + name)
        return exact(found[0], name, kind, owner)

    def script(fn):
        _require(number(fn + 0x114) == r.base + 0x1d6ca0, 'Script VM entry mismatch')
        address, size, capacity = struct.unpack('<Qii', stable(fn + 0x8c, 16))
        _require(address and 0 < size <= capacity <= 131072, 'Invalid script range')
        raw = stable(address, size)
        return dict(address=address, size=size, capacity=capacity,
                    sha256=hashlib.sha256(raw).hexdigest()), raw

    writer = child(war_pc, 'UpdateHUDActions', 'Function')
    meta, raw = script(writer)
    lhs, lhs_offset = property_at(war_pc, 'LastCrosshairHitLocation', 'StructProperty')
    impact, impact_offset = property_at(writer, 'Impact', 'StructProperty')
    ww, ww_offset = property_at(writer, 'WW', 'ObjectProperty')
    structure_offset = p['struct_property_struct']
    vector = number(lhs + structure_offset); exact(vector, 'Vector', 'ScriptStruct')
    impact_struct = number(impact + structure_offset); exact(impact_struct, 'ImpactInfo', 'ScriptStruct')
    member, member_offset = property_at(impact_struct, 'HitLocation', 'StructProperty')

    def prop_shape(a, size, target, excluded=0):
        _require(number(a + 0x70, '<I') == 1 and number(a + 0x74, '<I') == size and
                 not number(a + 0x78) & excluded and number(a + structure_offset) == target,
                 'Property shape/flags/structure mismatch')

    impact_size = number(impact + 0x74, '<I')
    _require(lhs_offset == 0x136c and 12 <= impact_size <= 65536 and
             member_offset <= impact_size - 12, 'Unsupported assignment layout')
    prop_shape(lhs, 12, vector, 0x20); prop_shape(member, 12, vector, 0x400000)
    prop_shape(impact, impact_size, impact_struct); prop_shape(ww, 8, war_weapon)
    writer_view = dict(enabled=False)
    if include_writer_view:
        view_roles = {}; view_offsets = {}; view_shapes = {}
        ranges = [(ww_offset, 8), (impact_offset, impact_size)]
        for role, (name, structure_name) in VIEW_FIELDS.items():
            prop = child(writer, name, 'StructProperty')
            resolved, offset = property_at(writer, name, 'StructProperty')
            _require(resolved == prop, 'Writer child/resolver mismatch')
            structure = number(prop + structure_offset)
            exact(structure, structure_name, 'ScriptStruct')
            if structure_name == 'Vector':
                _require(structure == vector, 'Writer view Vector identity mismatch')
            else:
                view_roles['rotator_struct'] = add(structure)
            prop_shape(prop, 12, structure)
            _require(offset < 65524 and all(offset + 12 <= start or start + size <= offset
                     for start, size in ranges), 'Writer view range overlap/bounds')
            ranges.append((offset, 12)); view_roles[role] = add(prop); view_offsets[role] = offset
            view_shapes[role] = dict(dim=1, size=12, structure=add(structure))
        writer_view = dict(enabled=True, roles=view_roles, offsets=view_offsets, shapes=view_shapes)
    aim_state = dict(enabled=False)
    if include_aim_state:
        _require(include_writer_view, 'Aiming state requires writer view')
        from cinematic_anchor import BOOL_CODE
        prop, offset = property_at(fresh(pc)['cls'], 'bIsTargeting', 'BoolProperty', True)
        mask = number(prop + 0xd0, '<I')
        _require(number(prop + 0x70, '<I') == 1 and number(prop + 0x74, '<I') == 4 and
                 0 < mask < (1 << 32) and not mask & (mask - 1) and offset <= 65532,
                 'Invalid aiming BoolProperty layout')
        _require(number(r.base + 0x1b86280 + 0x2d * 8) == r.base + 0x1d6ea4 and
                 number(number(prop) + 0x2a0) == r.base + 0x1daba0,
                 'Aiming BoolProperty dispatch mismatch')
        for rva, size, digest in BOOL_CODE:
            _require(hashlib.sha256(stable(r.base + rva, size)).hexdigest() == digest,
                     'Aiming BoolProperty code mismatch')
        aim_state = dict(enabled=True, property=add(prop), offset=offset, mask=mask)
    expected = b'\x0f\x01' + struct.pack('<Q', lhs) + b'\x35' + struct.pack('<QQ', member, impact_struct) + bytes(3) + struct.pack('<Q', impact)
    _require(len(raw) >= 0x380 and raw[0x359:0x37f] == expected, 'Exact assignment tokens mismatch')
    native = []; attested_entry = None
    for rva, text in NATIVE_BYTES.items():
        address = r.base + rva; size = len(bytes.fromhex(text))
        if rva == 0x1d7ad0 and entry_reader is not None:
            # Only the installed execLet entry may be read through the caller's
            # bound attestation capability. Never compare substituted originals
            # to raw patched memory in the final ordinary-read bracket.
            value = entry_reader(address, size, r.read)
            _require(isinstance(value, bytes) and len(value) == size and entry_reader(address, size, r.read) == value,
                     'Attested execLet entry changed during acquisition')
            attested_entry = (address, size, value)
        else:
            value = stable(address, size)
        _require(value.hex() == text, 'Native bytes mismatch: ' + hex(rva))
        native.append(dict(rva=rva, bytes=text, sha256=hashlib.sha256(value).hexdigest()))
    for op, rva in SLOTS.items():
        _require(number(r.base + 0x1b86280 + op * 8) == r.base + rva, 'Fresh GNatives mismatch')
    vt = number(member)
    _require(number(vt + 0x2a8) == r.base + 0x1d5ec8, 'StructProperty copy dispatch mismatch')
    roots = []
    if include_tick_roots:
        state = child(war_pc, 'PlayerDriving', 'State')
        for owner, name in ((war_pc, 'WarPC.PlayerTick'), (state, 'WarPC.PlayerDriving.PlayerTick')):
            fn = child(owner, 'PlayerTick', 'Function'); sm, _ = script(fn)
            roots.append(dict(name=name, function=add(fn), owner=add(owner), script=sm,
                              runtime_coverage_proven=False))
    # Class identity closure required by the native helper; selected class
    # superchains are certified, never inferred from an old resolver cache.
    cursor = 0
    while cursor < len(ids):
        item = ids[cursor]; add(item['cls'])
        if fresh(item['cls'])['name'] == 'Class':
            list(ancestors(item['address']))
        cursor += 1
    roles = dict(zip(ROLES, map(add, (writer, war_pc, pc, pawn, weapon, war_pawn, war_weapon,
                                     lhs, member, impact, impact_struct, vector, ww))))
    registry_change = None; link_changes = []
    for (address, size), value in checks.items():
        final = r.read(address, size)
        _require(len(final) == size, f'Incomplete final certificate read: address={address:#x} size={size}')
        if registry_header(address, size):
            if not registry_append(value, final):
                registry_change = (address, size)
        elif final != value:
            selected_link = controlled_links.get((address, size))
            if selected_link is None:
                changed(address, size, 'Certificate changed before completion')
            link_changes.append(dict(selected_link, address=address, size=size,
                old=struct.unpack('<Q', value)[0], new=struct.unpack('<Q', final)[0],
                phase='final'))
    # Finish every immutable check and serialized schema validation before any
    # deferred retryable result. A link/registry transition cannot mask corrupt
    # identities, scripts, property metadata, code or the attested entry.
    if attested_entry is not None:
        address, size, value = attested_entry
        _require(entry_reader(address, size, r.read) == value,
                 'Attested execLet entry changed before completion')
    c = dict(schema=1, pid=r.pid, base=r.base, sha256=GAME_SHA,
             objects_header=r.objects_header, names_header=r.names_header, layout=layout,
             identities=ids, supers=[dict(identity=indexes[a], parent=b) for a, b in sorted(supers.items())],
             roles=roles, fields=dict(zip(LINKS, (pc_pawn, pawn_pc, pawn_weapon, weapon_pawn))),
             script=meta['address'], script_size=meta['size'], scriptmeta=meta,
             assignment_tokens=expected.hex(),
             impact_offset=impact_offset, member_offset=member_offset, ww_offset=ww_offset,
             impact_size=impact_size, property_structure=structure_offset,
             nativebyteschecks=native, native_slots=[dict(opcode=o, rva=v) for o,v in SLOTS.items()],
             tick_roots=roots, writer_view=writer_view, aim_state=aim_state, read_only=True, runtime_coverage_proven=False,
             provenance=dict(disk_exe_sha256=GAME_SHA, historical_code_sha256=HISTORICAL_CODE_SHA,
                             native_bytes_source='fresh process reads matched to bounded historical contract',
                             acquired_perf_counter_ns=started, finished_perf_counter_ns=time.perf_counter_ns()))
    validate_crosshair_assignment_config(c)
    if link_changes:
        raise CrosshairControlledLinkChanged(dict(stage='crosshair_final_links',
            changes=link_changes, pid=r.pid, base=r.base, sha256=GAME_SHA,
            read_only=True, accepted=False))
    if registry_change:
        changed(*registry_change, 'Registry header changed before completion')
    return c


def validate_crosshair_assignment_config(c):
    """Validate serialized structure; this never reauthorizes cached addresses."""
    try:
        _require(c['schema'] == 1 and type(c['schema']) is int and c['sha256'] == GAME_SHA,
                 'Invalid schema/build')
        for key in ('pid', 'base', 'objects_header', 'names_header', 'script'):
            _require(type(c[key]) is int and 0 < c[key] < 1 << 64, 'Invalid address/integer')
        ids = c['identities']; _require(0 < len(ids) <= 96, 'Invalid identities')
        _require(len({i['address'] for i in ids}) == len(ids) and
                 len({i['internal_index'] for i in ids}) == len(ids), 'Duplicate identity')
        addresses = {i['address'] for i in ids}
        for i in ids:
            _require(type(i['address']) is int and i['address'] > 0 and i['cls'] in addresses and
                     isinstance(i['name'], str) and 0 < len(i['name']) < 64 and
                     not any(x in i['name'] for x in '\r\n\0='), 'Invalid identity')
            for k in ('internal_index', 'name_index', 'number'):
                _require(type(i[k]) is int and 0 <= i[k] < 4000000, 'Invalid identity index')
            _require(type(i['outer']) is int and 0 <= i['outer'] < 1 << 64, 'Invalid outer')
        def index(x):
            _require(type(x) is int and 0 <= x < len(ids), 'Invalid role index')
        _require(set(c['roles']) == set(ROLES) and set(c['fields']) == set(LINKS), 'Missing roles/fields')
        for x in c['roles'].values(): index(x)
        by_address = {i['address']:i for i in ids}
        def named(role, name, kind, outer_role=None):
            i=ids[c['roles'][role]]
            _require(i['name'] == name and i['number'] == 0 and by_address[i['cls']]['name'] == kind and
                     (outer_role is None or i['outer'] == ids[c['roles'][outer_role]]['address']),
                     'Serialized role identity mismatch: ' + role)
        for role,name in (('war_pc','WarPC'), ('war_pawn','WarPawn'), ('war_weapon','WarWeapon')):
            named(role,name,'Class')
        named('writer','UpdateHUDActions','Function','war_pc')
        named('lhs','LastCrosshairHitLocation','StructProperty','war_pc')
        named('member','HitLocation','StructProperty','impact_struct')
        named('impact','Impact','StructProperty','writer')
        named('ww','WW','ObjectProperty','writer')
        named('vector_struct','Vector','ScriptStruct'); named('impact_struct','ImpactInfo','ScriptStruct')
        view = c.get('writer_view', dict(enabled=False))
        _require(type(view) is dict and type(view.get('enabled')) is bool, 'Invalid writer view enabled')
        if view['enabled']:
            _require(set(view) == {'enabled', 'roles', 'offsets', 'shapes'} and
                     set(view['roles']) == set(VIEW_FIELDS) | {'rotator_struct'} and
                     set(view['offsets']) == set(view['shapes']) == set(VIEW_FIELDS), 'Incomplete writer view')
            for role in view['roles'].values(): index(role)
            rotator = ids[view['roles']['rotator_struct']]
            _require(rotator['name'] == 'Rotator' and rotator['number'] == 0 and
                     by_address[rotator['cls']]['name'] == 'ScriptStruct', 'Invalid Rotator identity')
            ranges = [(c['ww_offset'], 8), (c['impact_offset'], c['impact_size'])]
            for role, (name, structure_name) in VIEW_FIELDS.items():
                prop = ids[view['roles'][role]]; offset = view['offsets'][role]; shape = view['shapes'][role]
                target = c['roles']['vector_struct'] if structure_name == 'Vector' else view['roles']['rotator_struct']
                _require(prop['name'] == name and prop['number'] == 0 and
                         by_address[prop['cls']]['name'] == 'StructProperty' and
                         prop['outer'] == ids[c['roles']['writer']]['address'], 'Invalid writer view property')
                _require(type(offset) is int and 0 <= offset < 65524 and
                         type(shape) is dict and set(shape) == {'dim', 'size', 'structure'} and
                         all(type(v) is int for v in shape.values()) and
                         shape == dict(dim=1, size=12, structure=target) and
                         all(offset + 12 <= start or start + size <= offset for start, size in ranges),
                         'Invalid writer view shape/range')
                ranges.append((offset, 12))
        else:
            _require(set(view) == {'enabled'}, 'Disabled writer view has fields')
        aim = c.get('aim_state', dict(enabled=False))
        _require(type(aim) is dict and type(aim.get('enabled')) is bool, 'Invalid aiming state')
        if aim['enabled']:
            _require(set(aim) == {'enabled','property','offset','mask'} and view['enabled'], 'Incomplete aiming state')
            index(aim['property']); prop = ids[aim['property']]
            _require(prop['name'] == 'bIsTargeting' and prop['number'] == 0 and
                     by_address[prop['cls']]['name'] == 'BoolProperty' and
                     type(aim['offset']) is int and 0 <= aim['offset'] <= 65532 and
                     type(aim['mask']) is int and 0 < aim['mask'] < (1 << 32) and
                     not aim['mask'] & (aim['mask']-1), 'Invalid aiming property')
            parent = ids[c['roles']['controller']]['cls']; seen = set()
            parents = {ids[x['identity']]['address']:x['parent'] for x in c['supers']}
            while parent and parent not in seen and parent != prop['outer']:
                seen.add(parent); parent = parents.get(parent,0)
            _require(parent == prop['outer'], 'Aiming property owner mismatch')
        else:
            _require(set(aim) == {'enabled'}, 'Disabled aiming state has fields')
        for f in c['fields'].values():
            index(f['property']); _require(type(f['offset']) is int and 0 <= f['offset'] < 65536, 'Invalid field offset')
        _require(set(c['layout']) == set(LAYOUT) and c['layout']['property_offset_internal'] == 0x94, 'Invalid layout')
        for v in c['layout'].values(): _require(type(v) is int and 0 <= v < 65536, 'Invalid layout value')
        for key in ('impact_offset', 'member_offset', 'ww_offset', 'impact_size', 'property_structure', 'script_size'):
            _require(type(c[key]) is int and 0 <= c[key] <= 131072, 'Invalid layout integer')
        _require(0x380 <= c['script_size'] <= 131072 and 12 <= c['impact_size'] <= 65536 and
                 c['member_offset'] <= c['impact_size'] - 12 and 0x60 <= c['property_structure'] < 65536,
                 'Invalid assignment dimensions')
        _require(c['scriptmeta']['address'] == c['script'] and c['scriptmeta']['size'] == c['script_size'] and
                 c['script_size'] <= c['scriptmeta']['capacity'] <= 131072, 'Script metadata mismatch')
        _require(len(bytes.fromhex(c['scriptmeta']['sha256'])) == 32, 'Invalid script hash')
        addr=lambda role:ids[c['roles'][role]]['address']
        expected=b'\x0f\x01'+struct.pack('<Q',addr('lhs'))+b'\x35'+struct.pack('<QQ',addr('member'),addr('impact_struct'))+bytes(3)+struct.pack('<Q',addr('impact'))
        _require(c['assignment_tokens'] == expected.hex(), 'Serialized assignment tokens mismatch')
        for s in c['supers']:
            index(s['identity']); _require(s['parent'] == 0 or s['parent'] in addresses, 'Unknown superclass')
        _require({x['rva']:x['bytes'] for x in c['nativebyteschecks']} == NATIVE_BYTES, 'Native contract mismatch')
        for x in c['nativebyteschecks']:
            _require(hashlib.sha256(bytes.fromhex(x['bytes'])).hexdigest() == x['sha256'], 'Native hash mismatch')
        _require({x['opcode']:x['rva'] for x in c['native_slots']} == SLOTS, 'Slot contract mismatch')
        _require(len(c['tick_roots']) in (0, 2), 'Partial tick root certificate')
        for pos,root in enumerate(c['tick_roots']):
            index(root['function']); index(root['owner'])
            _require(root['runtime_coverage_proven'] is False, 'Unproven runtime coverage')
            owner=ids[root['owner']]; fn=ids[root['function']]
            _require(root['name'] == ('WarPC.PlayerTick','WarPC.PlayerDriving.PlayerTick')[pos] and
                     fn['name'] == 'PlayerTick' and fn['number'] == 0 and fn['outer'] == owner['address'] and
                     by_address[fn['cls']]['name'] == 'Function', 'Invalid tick function')
            _require((pos == 0 and owner['address'] == addr('war_pc')) or
                     (pos == 1 and owner['name'] == 'PlayerDriving' and owner['number'] == 0 and
                      owner['outer'] == addr('war_pc') and by_address[owner['cls']]['name'] == 'State'),
                     'Invalid tick owner')
            sm=root['script']
            _require(type(sm['address']) is int and sm['address'] > 0 and
                     0 < sm['size'] <= sm['capacity'] <= 131072 and len(bytes.fromhex(sm['sha256'])) == 32,
                     'Invalid tick script')
        _require(c['read_only'] is True and c['runtime_coverage_proven'] is False, 'Invalid evidence authority')
    except (KeyError, TypeError, ValueError) as e:
        raise CrosshairConfigRejected('Malformed certificate') from e
    return c


def crosshair_assignment_json(config):
    validate_crosshair_assignment_config(config)
    return json.dumps(config, ensure_ascii=True, sort_keys=True, indent=2) + '\n'


def crosshair_assignment_ini(config):
    """Offline transport only; the native caller must revalidate live evidence."""
    c=validate_crosshair_assignment_config(config)
    values={k:c[k] for k in ('schema','pid','base','sha256','objects_header','names_header',
            'script','script_size','impact_offset','member_offset','ww_offset','impact_size','property_structure')}
    values.update(native_contract=1, identity_count=len(c['identities']), super_count=len(c['supers']),
                  tick_root_count=len(c['tick_roots']))
    values.update(c['layout']); values.update(c['roles'])
    view = c.get('writer_view', dict(enabled=False)); values['view_enabled'] = int(view['enabled'])
    if view['enabled']:
        for role, index in view['roles'].items(): values['view_' + role] = index
        for role, offset in view['offsets'].items(): values['view_' + role + '_offset'] = offset
    aim = c.get('aim_state', dict(enabled=False)); values['aim_enabled'] = int(aim['enabled'])
    if aim['enabled']:
        for key in ('property','offset','mask'): values['aim_'+key] = aim[key]
    for name,field in c['fields'].items():
        values[name+'_property']=field['property']; values[name+'_offset']=field['offset']
    for n,item in enumerate(c['identities']):
        for key,value in item.items():
            values[f'id{n}_{"index" if key=="internal_index" else key}']=value
    for n,item in enumerate(c['supers']):
        for key,value in item.items(): values[f'super{n}_{key}']=value
    for n,item in enumerate(c['tick_roots']):
        for key in ('owner','function'): values[f'root{n}_{key}']=item[key]
        values[f'root{n}_script']=item['script']['address']
        values[f'root{n}_size']=item['script']['size']
    return '[CrosshairAssignment]\n' + ''.join(f'{key}={value}\n' for key,value in values.items())
