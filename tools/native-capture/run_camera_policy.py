"""Guarded, session-owned CameraBone gait policy; runtime acceptance is pending.

prepare/observe are read-only and consume an existing LiveRegistryAnchorSampler.
Only the selected local camera's registered CameraBone instance can be written.
The two fields are ambient-motion enable and roadie damping; Alpha, camera
options, fire effects, Pawn motion and HMD poses are never write targets.
Memory/identity brackets and masked read/modify/write are not atomic engine
transactions. Callers own world recovery and supply a fresh sampler to rebind.
"""
import copy
import ctypes as c
from ctypes import wintypes as w
import hashlib
import math
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
INSPECTOR = str(ROOT / 'tools/package-inspector')
if INSPECTOR not in sys.path:
    sys.path.insert(0, INSPECTOR)
from cinematic_anchor import BOOL_CODE

SUPPORTED_SHA256 = 'bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4'
AMBIENT = 'bDisableAmbientCameraMotion'
ROADIE = 'RoadieRunShakyCamDampeningFactor'
LIMITS = ('Saved script evidence is not current runtime acceptance. Identity and '
          'timestamp brackets are not atomic engine snapshots; masked writes '
          'preserve observed neighboring bits, not concurrent unobserved writes.')


class PolicyRejected(RuntimeError):
    pass


class WriteFailure(OSError):
    def __init__(self, message, bytes_written=0):
        super().__init__(message)
        self.bytes_written = bytes_written


class Writer:
    """Private writable pages only; process identity is checked on every write."""
    def __init__(self, pid, identity, check_identity):
        self.identity, self.check_identity = identity, check_identity
        self.k = c.WinDLL('kernel32', use_last_error=True)
        self.k.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
        self.k.OpenProcess.restype = w.HANDLE
        self.k.CloseHandle.argtypes = [w.HANDLE]
        self.k.WriteProcessMemory.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p,
                                              c.c_size_t, c.POINTER(c.c_size_t)]
        self.k.WriteProcessMemory.restype = w.BOOL
        self.handle = self.k.OpenProcess(0x438, False, pid)
        if not self.handle:
            raise c.WinError(c.get_last_error())

    def write(self, address, raw):
        from runtime_probe import MBI
        if self.check_identity(self.identity['pid']) != self.identity:
            raise PolicyRejected('Run-camera process identity changed')
        self.k.VirtualQueryEx.argtypes = [w.HANDLE, c.c_void_p, c.POINTER(MBI), c.c_size_t]
        self.k.VirtualQueryEx.restype = c.c_size_t
        region = MBI()
        if (len(raw) != 4 or not self.k.VirtualQueryEx(self.handle, address, c.byref(region), c.sizeof(region))
                or region.State != 0x1000 or region.Type != 0x20000 or region.Protect not in (4, 8)
                or not region.BaseAddress <= address <= region.BaseAddress + region.RegionSize - 4):
            raise PolicyRejected('Run-camera target is not a writable private allocation')
        written = c.c_size_t()
        data = c.create_string_buffer(raw, len(raw))
        ok = self.k.WriteProcessMemory(self.handle, address, data, len(raw), c.byref(written))
        if not ok or written.value != len(raw):
            raise WriteFailure('Run-camera WriteProcessMemory failed: ' + str(c.get_last_error()), written.value)

    def close(self):
        if self.handle:
            self.k.CloseHandle(self.handle)
            self.handle = None


class _Certificate:
    def __init__(self, sampler):
        self.sampler, self.r = sampler, sampler.r
        self.identities, self.supers, self.layouts, self.links = {}, {}, {}, {}
        self.base = self.r.base
        self.profile = {key: self.r.profile[key] for key in
                        ('sha256', 'field_super', 'property_offset_internal', 'struct_property_struct')}
        if self.r.profile.get('sha256') != SUPPORTED_SHA256:
            raise PolicyRejected('Run-camera executable profile is not the supported build')
        before = sampler.guard()
        self.roles = {name: self._instance(getattr(sampler, name), kind) for name, kind in
                      (('camera', 'WarPlayerCamera'), ('controller', 'WarPC'),
                       ('local', 'WarLocalPlayer'), ('pawn', 'WarPawn'))}
        if before.get('view_target') != sampler.pawn:
            raise PolicyRejected('Run-camera requires the controlled Pawn view target')
        if getattr(sampler, 'view_target', None) is None:
            raise PolicyRejected('Run-camera view-target link is unavailable')
        self.links[sampler.view_target] = sampler.pawn
        cam, pc, local, pawn = (self.roles[x] for x in ('camera', 'controller', 'local', 'pawn'))
        self._link(pc, 'PlayerCamera', cam['address'], 'ObjectProperty')
        self._link(cam, 'PCOwner', pc['address'], 'ObjectProperty')
        self._link(pc, 'Player', local['address'], 'ObjectProperty')
        self._link(pc, 'Pawn', pawn['address'], 'ObjectProperty')
        self._link(pawn, 'Controller', pc['address'], 'ObjectProperty')
        mesh_field = self._field(pawn, 'Mesh', 'ComponentProperty', 8)
        mesh = self._instance(self.r.u64(pawn['address'] + mesh_field['offset']), 'SkeletalMeshComponent')
        self.links[pawn['address'] + mesh_field['offset']] = mesh['address']
        mod_field = self._field(cam, 'WarCamMod_CameraBone', 'ObjectProperty', 8, 'WarPlayerCamera')
        self.modifier = self._instance(self.r.u64(cam['address'] + mod_field['offset']), 'WarCamMod_CameraBone')
        self.links[cam['address'] + mod_field['offset']] = self.modifier['address']
        self._link(self.modifier, 'CameraOwner', cam['address'], 'ObjectProperty')
        self._link(self.modifier, 'Pawn', pawn['address'], 'ObjectProperty')
        self._link(self.modifier, 'Mesh', mesh['address'], 'ComponentProperty')
        self.roles['mesh'] = mesh
        self.list_field = self._field(cam, 'ModifierList', 'ArrayProperty', 16, 'Camera')
        inner_link = self.list_field['address'] + self.r.profile['struct_property_struct']
        inner = self._remember(self.r.u64(inner_link))
        self.layouts[inner_link] = struct.pack('<Q', inner['address'])
        kind = self._remember(inner['cls'])
        if kind['name'] != 'ObjectProperty' or kind['number'] or inner['outer'] != self.list_field['address']:
            raise PolicyRejected('ModifierList inner is not its ObjectProperty')
        inner_layout = self.r.read(inner['address'] + 0x70, 8)
        if struct.unpack('<II', inner_layout) != (1, 8):
            raise PolicyRejected('ModifierList inner pointer layout differs')
        self.layouts[inner['address'] + 0x70] = inner_layout
        self._membership()
        self.ambient = self._field(self.modifier, AMBIENT, 'BoolProperty', 4, 'WarCamMod_CameraBone')
        self.roadie = self._field(self.modifier, ROADIE, 'FloatProperty', 4, 'WarCamMod_CameraBone')
        if abs(self.ambient['offset'] - self.roadie['offset']) < 4:
            raise PolicyRejected('Run-camera write fields overlap')
        self.mask = self.r.u32(self.ambient['address'] + 0xd0)
        if not self.mask or self.mask & (self.mask - 1):
            raise PolicyRejected('Ambient BoolProperty mask is not one bit')
        self.layouts[self.ambient['address'] + 0xd0] = struct.pack('<I', self.mask)
        self._bool_route()
        self.verify()
        after = sampler.guard()
        if any(before.get(x) != after.get(x) for x in ('pawn', 'controller', 'camera', 'local_player', 'level', 'view_target')):
            raise PolicyRejected('Run-camera anchor changed during preparation')
        self.verify()

    def _remember(self, address):
        if not address:
            raise PolicyRejected('Run-camera null object')
        value = copy.deepcopy(self.sampler.fresh(address))
        old = self.identities.setdefault(address, value)
        if old != value:
            raise PolicyRejected('Run-camera object changed during preparation')
        return value

    def _chain(self, cls):
        chain, seen = [], set()
        while cls:
            if cls in seen or len(seen) >= 64:
                raise PolicyRejected('Run-camera invalid class chain')
            seen.add(cls)
            item = self._remember(cls)
            meta = self._remember(item['cls'])
            if item['number'] or meta['name'] != 'Class' or meta['number']:
                raise PolicyRejected('Run-camera class identity differs')
            parent = self.r.u64(cls + self.r.profile['field_super'])
            self.supers[cls] = parent
            chain.append(item)
            cls = parent
        return chain

    def _instance(self, address, expected):
        value = self._remember(address)
        if value['name'].startswith('Default__') or expected not in {x['name'] for x in self._chain(value['cls'])}:
            raise PolicyRejected('Run-camera instance is not ' + expected)
        return value

    def _field(self, owner, name, kind, size, declaring=None):
        prop = self.r.property(owner['cls'], name, True)
        identity = self._remember(prop['address'])
        actual_kind = self._remember(identity['cls'])
        meta = self._remember(actual_kind['cls'])
        chain = self._chain(owner['cls'])
        declared = next((x for x in chain if x['address'] == identity['outer']), None)
        offset = prop['offset']
        raw = self.r.read(prop['address'] + 0x70, 0x28)
        if (prop['type'] != kind or actual_kind['name'] != kind or actual_kind['number']
                or meta['name'] != 'Class' or meta['number'] or identity['name'] != name or identity['number']
                or declared is None or (declaring is not None and declared['name'] != declaring)
                or isinstance(offset, bool) or not isinstance(offset, int) or not 0 <= offset <= 0x10000 - size
                or struct.unpack_from('<II', raw) != (1, size)
                or self.r.u32(prop['address'] + self.r.profile['property_offset_internal']) != offset):
            raise PolicyRejected('Run-camera reflected field differs: ' + name)
        self.layouts[prop['address'] + 0x70] = raw
        return dict(prop, size=size)

    def _link(self, owner, name, expected, kind):
        prop = self._field(owner, name, kind, 8)
        address = owner['address'] + prop['offset']
        if self.r.u64(address) != expected:
            raise PolicyRejected('Run-camera ownership link differs: ' + name)
        self.links[address] = expected

    def _membership(self):
        address = self.roles['camera']['address'] + self.list_field['offset']
        raw = self.r.read(address, 16)
        data, count, capacity = struct.unpack('<Qii', raw)
        if not data or data % 8 or not 0 < count <= capacity <= 256:
            raise PolicyRejected('Run-camera ModifierList bounds differ')
        items = struct.unpack('<' + str(count) + 'Q', self.r.read(data, count * 8))
        if self.r.read(address, 16) != raw or items.count(self.modifier['address']) != 1:
            raise PolicyRejected('Run-camera modifier is not uniquely registered')
        return dict(address=address, data=data, count=count, capacity=capacity)

    def _bool_route(self):
        r, address = self.r, self.ambient['address']
        vtable = r.u64(address)
        if (not vtable or r.u64(r.base + 0x1b86280 + 0x2d * 8) != r.base + 0x1d6ea4
                or r.u64(vtable + 0x2a0) != r.base + 0x1daba0):
            raise PolicyRejected('Run-camera BoolProperty accessor route differs')
        for rva, size, digest in BOOL_CODE:
            if hashlib.sha256(r.read(r.base + rva, size)).hexdigest() != digest:
                raise PolicyRejected('Run-camera BoolProperty native code differs')

    def verify(self):
        if self.r.base != self.base or any(self.r.profile.get(k) != v for k, v in self.profile.items()):
            raise PolicyRejected('Run-camera module/profile changed')
        for address, expected in self.identities.items():
            if self.sampler.fresh(address) != expected:
                raise PolicyRejected('Run-camera certificate identity changed')
        for address, expected in self.supers.items():
            if self.r.u64(address + self.r.profile['field_super']) != expected:
                raise PolicyRejected('Run-camera class chain changed')
        for address, expected in self.layouts.items():
            if self.r.read(address, len(expected)) != expected:
                raise PolicyRejected('Run-camera reflected layout changed')
        for address, expected in self.links.items():
            if self.r.u64(address) != expected:
                raise PolicyRejected('Run-camera ownership context changed')
        self._membership()
        self._bool_route()

    def context_key(self):
        return (tuple(tuple(sorted(self.roles[x].items())) for x in ('camera', 'pawn', 'mesh'))
                + (tuple(sorted(self.modifier.items())),))

    def target_replaced(self):
        # A failed read is unknown, never proof that our old object was retired.
        try:
            return self.sampler.fresh(self.modifier['address']) != self.modifier
        except (RuntimeError, OSError, ValueError, struct.error):
            pass
        # LiveRegistryAnchorSampler rejects an absent/reassigned registry slot.
        # Independently bracket that one known slot to distinguish retirement
        # from a read failure; do not scan for another CameraBone instance.
        header_address = getattr(self.r, 'objects_header', None)
        if header_address is None:
            return False
        try:
            raw = self.r.read(header_address, 16)
            data, count, capacity = struct.unpack('<Qii', raw)
            index = self.modifier['internal_index']
            if not data or data % 8 or not 0 < count <= capacity < 4000000 or not 0 <= index < 4000000:
                return False
            if index >= count:
                return self.r.read(header_address, 16) == raw
            slot = data + index * 8
            value = self.r.u64(slot)
            return (value != self.modifier['address'] and self.r.read(header_address, 16) == raw
                    and self.r.u64(slot) == value)
        except (RuntimeError, OSError, ValueError, struct.error):
            return False

    def compatible_release(self, other):
        fields = (self.ambient['address'], self.roadie['address'])
        identities = (*fields, self.modifier['cls'], *(self.identities[x]['cls'] for x in fields))
        layouts = (fields[0] + 0x70, fields[0] + 0xd0, fields[1] + 0x70)
        return (self.modifier == other.modifier and self.base == other.base and self.profile == other.profile
                and self.ambient == other.ambient and self.roadie == other.roadie and self.mask == other.mask
                and all(self.identities[x] == other.identities.get(x) for x in identities)
                and all(self.layouts[x] == other.layouts.get(x) for x in layouts))

    def _diagnostic(self, owner, name, kind, size, fmt):
        try:
            prop = self.r.property(owner['cls'], name, True)
            ident = self.sampler.fresh(prop['address'])
            actual = self.sampler.fresh(ident['cls'])
            offset = prop['offset']
            if (prop['type'] != kind or actual['name'] != kind or ident['name'] != name or ident['number']
                    or not 0 <= offset <= 0x10000 - size
                    or struct.unpack('<II', self.r.read(prop['address'] + 0x70, 8)) != (1, size)
                    or self.r.u32(prop['address'] + self.r.profile['property_offset_internal']) != offset):
                raise PolicyRejected('diagnostic field layout differs')
            if name == 'Velocity':
                structure = self.r.struct_type(prop)
                if self.sampler.fresh(structure)['name'] != 'Vector':
                    raise PolicyRejected('Velocity is not a Vector')
                for component, wanted in zip(('X', 'Y', 'Z'), (0, 4, 8)):
                    field = self.r.property(structure, component)
                    if field['type'] != 'FloatProperty' or field['offset'] != wanted:
                        raise PolicyRejected('Velocity component layout differs')
            raw = self.r.read(owner['address'] + offset, size)
            values = struct.unpack(fmt, raw)
            if any(isinstance(v, float) and not math.isfinite(v) for v in values):
                raise PolicyRejected('diagnostic value is nonfinite')
            return dict(available=True, value=values[0] if len(values) == 1 else list(values), raw=raw.hex(), offset=offset)
        except (RuntimeError, OSError, ValueError, struct.error) as error:
            return dict(available=False, reason=str(error))

    def metadata(self):
        self.verify()
        stamp = self.r.read(self.sampler.stamp, 4)
        diagnostics = {name: self._diagnostic(owner, name, kind, size, fmt) for owner, name, kind, size, fmt in
                       ((self.modifier, 'CameraAnimOption', 'ByteProperty', 1, '<B'),
                        (self.modifier, 'Alpha', 'FloatProperty', 4, '<f'),
                        (self.roles['pawn'], 'CameraBoneMotionScale', 'FloatProperty', 4, '<f'),
                        (self.roles['pawn'], 'SpecialMove', 'ByteProperty', 1, '<B'),
                        (self.roles['pawn'], 'Velocity', 'StructProperty', 12, '<3f'))}
        ambient_address = self.modifier['address'] + self.ambient['offset']
        roadie_address = self.modifier['address'] + self.roadie['offset']
        word, factor = self.r.read(ambient_address, 4), self.r.read(roadie_address, 4)
        value = struct.unpack('<f', factor)[0]
        if not math.isfinite(value) or not 0 <= value <= 1:
            raise PolicyRejected('Observed roadie damping factor is outside 0..1')
        self.verify()
        end = self.r.read(self.sampler.stamp, 4)
        return dict(accepted=True, readonly=True, pid=self.r.pid, executable_sha256=SUPPORTED_SHA256,
                    module_base=hex(self.base), modifier=copy.deepcopy(self.modifier), roles=copy.deepcopy(self.roles),
                    membership=self._membership(), fields={
                        AMBIENT: dict(address=ambient_address, property=copy.deepcopy(self.ambient), mask=self.mask,
                                      raw=word.hex(), value=bool(struct.unpack('<I', word)[0] & self.mask)),
                        ROADIE: dict(address=roadie_address, property=copy.deepcopy(self.roadie),
                                     raw=factor.hex(), value=value)},
                    diagnostics=diagnostics, camera_stamp_before=stamp.hex(), camera_stamp_after=end.hex(),
                    same_camera_stamp=stamp == end, runtime_acceptance=False, limits=LIMITS)


class Plan:
    """Private baseline/receipt, exported only as detached JSON-safe metadata."""
    def __init__(self, certificate):
        self._certificate = certificate
        self._metadata = certificate.metadata()
        f = self._metadata['fields']
        original = bytes.fromhex(f[ROADIE]['raw'])
        factor = struct.unpack('<f', original)[0]
        if not math.isfinite(factor) or not 0 <= factor <= 1:
            raise PolicyRejected('Original roadie damping factor is outside 0..1')
        self._mutations = [dict(name=AMBIENT, address=f[AMBIENT]['address'], original=bytes.fromhex(f[AMBIENT]['raw']),
                                installed=struct.pack('<I', int.from_bytes(bytes.fromhex(f[AMBIENT]['raw']), 'little') |
                                                      certificate.mask),
                                mask=certificate.mask, noop=f[AMBIENT]['value'], owned=False),
                           dict(name=ROADIE, address=f[ROADIE]['address'], original=original,
                                installed=struct.pack('<f', 0.), mask=0, noop=factor == 0, owned=False)]
        self.state = 'prepared'
        certificate.verify()
        for change in self._mutations:
            current = certificate.r.read(change['address'], 4)
            if not _same(current, change['original'], change['mask']):
                raise PolicyRejected('Run-camera baseline changed during preparation')

    def export(self):
        result = copy.deepcopy(self._metadata)
        result['baseline_observation_readonly'] = result.pop('readonly')
        result['snapshot_kind'] = 'baseline_before_policy'
        result.update(state=self.state, planned_writes=[x['name'] for x in self._mutations if not x['noop']],
                      owned_fields=[x['name'] for x in self._mutations if x['owned']])
        return result


def _same(a, b, mask):
    return ((struct.unpack('<I', a)[0] ^ struct.unpack('<I', b)[0]) & mask) == 0 if mask else a == b


def observe(sampler, check_identity=None):
    """Read-only observation; never constructs a Writer or mutates game state."""
    try:
        identity = check_identity(sampler.r.pid) if check_identity else None
        if check_identity and not identity:
            raise PolicyRejected('Run-camera process identity is unavailable')
        result = _Certificate(sampler).metadata()
        if check_identity and check_identity(sampler.r.pid) != identity:
            raise PolicyRejected('Run-camera process identity changed during observation')
        return result
    except (RuntimeError, OSError, ValueError, struct.error) as error:
        return dict(accepted=False, readonly=True, reason=str(error), runtime_acceptance=False, limits=LIMITS)


class Policy:
    def __init__(self, pid, check_identity, *, writer=None):
        if not callable(check_identity):
            raise ValueError('Run-camera writes require a process identity checker')
        self.pid, self.check_identity = pid, check_identity
        self.identity = copy.deepcopy(check_identity(pid))
        if not self.identity or self.identity.get('pid') != pid:
            raise PolicyRejected('Run-camera policy needs a live owned process identity')
        self.writer, self._owns_writer = writer, writer is None
        self._plan, self._blocked_context = None, None
        self.closed = False
        self.report = dict(applications=0, writes=0, restored=0, external_changes=0,
                           retired_fields=0, pending_fields=[], errors=[], runtime_acceptance=False)

    def _process(self):
        if self.closed or self.check_identity(self.pid) != self.identity:
            raise PolicyRejected('Run-camera policy/process identity is no longer current')

    def prepare(self, sampler):
        self._process()
        if sampler.r.pid != self.pid:
            raise PolicyRejected('Run-camera sampler belongs to another process')
        if self._plan and any(x['owned'] for x in self._plan._mutations):
            raise PolicyRejected('Release the existing run-camera plan before preparing another')
        plan = Plan(_Certificate(sampler))
        self._process()
        if self._blocked_context == plan._certificate.context_key():
            raise PolicyRejected('Run-camera fields changed externally; same-context reapply refused')
        self._plan = plan
        return plan

    def observe(self, sampler=None):
        if sampler is not None:
            return observe(sampler, self.check_identity)
        self._process()
        if self._plan is None:
            raise PolicyRejected('Run-camera has no prepared plan')
        return self._plan._certificate.metadata()

    def _guard(self, certificate):
        self._process()
        certificate.verify()
        self._process()

    def _write(self, address, raw):
        if self.writer is None:
            self.writer = Writer(self.pid, self.identity, self.check_identity)
        self.writer.write(address, raw)

    def apply(self, plan=None):
        plan = self._plan if plan is None else plan
        if plan is None or plan is not self._plan or plan.state != 'prepared':
            raise PolicyRejected('Run-camera apply requires its current prepared plan')
        cert = plan._certificate
        try:
            self._guard(cert)
            for change in plan._mutations:
                self._guard(cert)
                current = cert.r.read(change['address'], 4)
                if not _same(current, change['original'], change['mask']):
                    raise PolicyRejected('Run-camera original field changed before write')
                if change['noop']:
                    continue
                installed = (struct.pack('<I', struct.unpack('<I', current)[0] | change['mask'])
                             if change['mask'] else struct.pack('<f', 0.0))
                change.update(owned=True, before=current, installed=installed, attempt='unknown')
                try:
                    self._write(change['address'], installed)
                    change['attempt'] = 4
                except BaseException as error:
                    count = getattr(error, 'bytes_written', None)
                    if isinstance(count, int) and 0 <= count <= 4:
                        change['attempt'] = count
                    raise
                self.report['writes'] += 1
                if not _same(cert.r.read(change['address'], 4), installed, change['mask']):
                    raise PolicyRejected('Run-camera installed field failed readback')
            self._guard(cert)
            plan.state = 'applied'
            if not self.check():
                raise PolicyRejected(self.report['last_check']['reason'])
            self.report['applications'] += 1
            return plan.export()
        except BaseException as error:
            self._error(error)
            self.restore()
            raise

    def check(self):
        try:
            if not self._plan or self._plan.state != 'applied':
                raise PolicyRejected('Run-camera plan is not applied')
            cert = self._plan._certificate
            self._guard(cert)
            for change in self._plan._mutations:
                raw = cert.r.read(change['address'], 4)
                good = bool(struct.unpack('<I', raw)[0] & change['mask']) if change['mask'] else struct.unpack('<f', raw)[0] == 0
                if not good:
                    self._blocked_context = cert.context_key()
                    raise PolicyRejected('Run-camera desired field changed: ' + change['name'])
            self.report['last_check'] = dict(valid=True)
            return True
        except (RuntimeError, OSError, ValueError, struct.error) as error:
            self.report['last_check'] = dict(valid=False, reason=str(error))
            return False

    def _error(self, error):
        if len(self.report['errors']) < 16:
            self.report['errors'].append(dict(type=type(error).__name__, reason=str(error)))

    def restore(self, *, _certificate=None):
        plan = self._plan
        if plan is None:
            return dict(released=True, pending_fields=[])
        cert = _certificate or plan._certificate
        if _certificate is not None and not plan._certificate.compatible_release(cert):
            raise PolicyRejected('Run-camera replacement context cannot release this target')
        owned = [x for x in plan._mutations if x['owned']]
        try:
            self._guard(cert)
        except (RuntimeError, OSError, ValueError, struct.error) as error:
            try:
                process_retired = self.check_identity(self.pid) != self.identity
            except (RuntimeError, OSError, ValueError):
                process_retired = False
            if process_retired or plan._certificate.target_replaced():
                for change in owned:
                    change['owned'] = False
                    self.report['retired_fields'] += 1
            elif owned:
                self._error(error)
                self.report['pending_fields'] = [x['name'] for x in owned]
                return dict(released=False, pending_fields=list(self.report['pending_fields']), reason=str(error))
        else:
            for change in reversed(owned):
                try:
                    self._guard(cert)
                    current = cert.r.read(change['address'], 4)
                    count = change['attempt']
                    expected = change.get('restore_expected', change['installed'] if count == 'unknown' else
                                          change['installed'][:count] + change['before'][count:])
                    if _same(current, change['original'], change['mask']):
                        change['owned'] = False
                        if plan.state == 'applied':
                            self.report['external_changes'] += 1
                            self._blocked_context = plan._certificate.context_key()
                        continue
                    if change.get('restore_unknown'):
                        raise PolicyRejected('Run-camera failed restore has unknown ownership')
                    if not _same(current, expected, change['mask']):
                        if count == 'unknown':
                            raise PolicyRejected('Run-camera failed write has unknown ownership')
                        change['owned'] = False
                        self.report['external_changes'] += 1
                        self._blocked_context = plan._certificate.context_key()
                        continue
                    restored = (struct.pack('<I', (struct.unpack('<I', current)[0] & ~change['mask']) |
                                            (struct.unpack('<I', change['original'])[0] & change['mask']))
                                if change['mask'] else change['original'])
                    try:
                        self._write(change['address'], restored)
                    except (RuntimeError, OSError) as error:
                        if not _same(cert.r.read(change['address'], 4), change['original'], change['mask']):
                            written = getattr(error, 'bytes_written', None)
                            if isinstance(written, int) and 0 <= written <= 4:
                                change['restore_expected'] = restored[:written] + current[written:]
                            else:
                                change['restore_unknown'] = True
                            raise error
                    if not _same(cert.r.read(change['address'], 4), change['original'], change['mask']):
                        raise PolicyRejected('Run-camera restoration failed readback')
                    change['owned'] = False
                    self.report['restored'] += 1
                except (RuntimeError, OSError, ValueError, struct.error) as error:
                    self._error(error)
        self.report['pending_fields'] = [x['name'] for x in plan._mutations if x['owned']]
        released = not self.report['pending_fields']
        if released:
            plan.state = 'released'
        return dict(released=released, pending_fields=list(self.report['pending_fields']))

    def rebind(self, sampler, *, apply=True):
        self._process()
        if sampler.r.pid != self.pid:
            raise PolicyRejected('Run-camera sampler belongs to another process')
        old = self._plan
        if old and self.check():
            candidate = _Certificate(sampler)
            if candidate.context_key() == old._certificate.context_key():
                return old.export()
        released = self.restore()
        if not released['released']:
            # A persistent modifier can adopt a new Pawn after world recovery.
            # Validate its NEW local ownership before releasing the OLD baseline;
            # never recapture our installed bytes as a new baseline.
            candidate = _Certificate(sampler)
            self._guard(candidate)
            released = self.restore(_certificate=candidate)
        if not released['released']:
            raise PolicyRejected('Run-camera old transaction could not be safely released')
        plan = self.prepare(sampler)
        return self.apply(plan) if apply else plan.export()

    def close(self):
        if not self.closed:
            self.restore()
            if self.writer is not None and self._owns_writer:
                self.writer.close()
            self.closed = True
        return copy.deepcopy(self.report)
