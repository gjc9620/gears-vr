"""Current PhysX image/receiver observations; no calls, writes or pose permit.

The r38 original code establishes the actor+0x40 internal-body link and the
listed virtual slots. Observing them does not establish callable ABI, an
active collision shape, physical pose consumption or a physics-world fence.
This reader deliberately does not change HookEntryAttestation's two-base set.
"""
import copy
import hashlib
from pathlib import Path
import struct
import time

from cover_bone_query import _pe_layout
from cover_state import CoverQuery

ARTIFACT_SHA256 = '22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48'
ARTIFACT_SIZE = 2711552
CHUNKS = (
    (0x1209f0, 430, 'db2940e6bdd0e03a51e17d053a296d1414c57a797a50464a4b061f8d6bc729f7'),
    (0x1360f0, 853, '4280fcf0d499789f3ceffcdceba7dc2a6910fface745e7ce4a53b4446d9a6ba9'),
    (0x125ed0, 76, '0c75be96de8a92634b3033888e26b5f3a595419a522dfe7840866575234b4558'),
    (0x1a5650, 37, 'b3b94361bd83a2235a59a928e2dc60823069a4c819d9387abc2239aab08d3f67'),
    (0x136890, 93, '98dcd9ccaf7d72ddbd7b3556a33d3510baedaa50a2a59084732c674d8cdf02f8'),
    (0x1a55d0, 115, 'dd7956e68bed7d260c098fc28c9218d3d0c069dd479703ae658fad78b9946eeb'),
    (0x133fe0, 27, '560194562dc740dbe58fc62746046def288ec241400b15b4edff5c9e417bac93'),
    (0x13c270, 1205, '53e8e00ec82c8870ce4a972974664904839e308d653ec7411607d5af419f6500'),
)
ACTOR_TARGETS = {40: 0x13c270, 72: 0x133fe0, 104: 0x1360f0, 728: 0x136890}
INTERNAL_SLOTS = (8, 16, 40, 240, 360)


class SourceRejection(RuntimeError):
    def __init__(self, message, witness):
        super().__init__(message)
        self.source_witness = witness


def _file_identity(stat):
    # Reading can update access time; size, modification, creation and file ID
    # are the stability witnesses, followed by exact content comparison.
    return stat.st_size, stat.st_mtime_ns, stat.st_ctime_ns, stat.st_ino, stat.st_dev


def _modules(query):
    """Enumerate using the existing read-only Resolver handle, independently."""
    import ctypes as c
    from ctypes import wintypes as w
    psapi = c.WinDLL('psapi', use_last_error=True)
    psapi.EnumProcessModulesEx.argtypes = [w.HANDLE, c.POINTER(w.HMODULE), w.DWORD,
                                         c.POINTER(w.DWORD), w.DWORD]
    psapi.EnumProcessModulesEx.restype = w.BOOL
    psapi.GetModuleFileNameExW.argtypes = [w.HANDLE, w.HMODULE, w.LPWSTR, w.DWORD]
    psapi.GetModuleFileNameExW.restype = w.DWORD
    slots, needed = (w.HMODULE * 2048)(), w.DWORD()
    handle = query.r.p.handle
    if not psapi.EnumProcessModulesEx(handle, slots, c.sizeof(slots), c.byref(needed), 3):
        raise c.WinError(c.get_last_error())
    width = c.sizeof(w.HMODULE)
    if not 0 < needed.value <= c.sizeof(slots) or needed.value % width:
        raise RuntimeError('PhysX module enumeration exceeds complete bounded buffer')
    rows = []
    for base in slots[:needed.value // width]:
        name = c.create_unicode_buffer(32768)
        size = psapi.GetModuleFileNameExW(handle, base, name, len(name))
        if not 0 < size < len(name) - 1:
            raise RuntimeError('PhysX module path unavailable or truncated')
        rows.append((int(base), Path(name.value).resolve()))
    return rows


class Artifact:
    def __init__(self, path):
        self.path = Path(path).resolve()
        before = self.path.stat()
        if before.st_size != ARTIFACT_SIZE:
            raise RuntimeError('PhysX original artifact size rejected')
        with self.path.open('rb') as stream:
            self.raw = stream.read(ARTIFACT_SIZE + 1)
        after = self.path.stat()
        if (_file_identity(before) != _file_identity(after) or len(self.raw) != ARTIFACT_SIZE or
                hashlib.sha256(self.raw).hexdigest() != ARTIFACT_SHA256):
            raise RuntimeError('PhysX original artifact hash/stability rejected')
        nt = struct.unpack_from('<I', self.raw, 0x3c)[0]
        header_size = struct.unpack_from('<I', self.raw, nt + 24 + 60)[0]
        if not 512 <= header_size <= 65536:
            raise RuntimeError('PhysX original PE header size rejected')
        self.header = self.raw[:header_size]
        self.layout = _pe_layout(self.header, len(self.raw))
        self.image_base_offset = self.layout['fields']['pe_offset'] + 24 + 24
        self.preferred = self.layout['fields']['image_base']
        self.image_size = self.layout['fields']['size_of_image']
        self.disk_stat = _file_identity(after)
        self.relocations = []
        rva, size = struct.unpack_from('<II', self.header, nt + 24 + 112 + 5 * 8)
        if not rva or not 8 <= size <= 262144:
            raise RuntimeError('PhysX relocation directory bounds rejected')
        raw = self.original(rva, size)
        at = 0
        while at < size:
            if size - at < 8:
                raise RuntimeError('PhysX relocation block truncated')
            page, length = struct.unpack_from('<II', raw, at)
            if length < 8 or length % 2 or at + length > size:
                raise RuntimeError('PhysX relocation block bounds rejected')
            for offset in range(at + 8, at + length, 2):
                value = struct.unpack_from('<H', raw, offset)[0]
                kind, delta = value >> 12, value & 0xfff
                if kind == 0:
                    continue
                if kind != 10:
                    raise RuntimeError('PhysX relocation type unsupported')
                target = page + delta
                self.section(target, 8)
                self.relocations.append(target)
            at += length
        self.relocations.sort()
        if any(a + 8 > b for a, b in zip(self.relocations, self.relocations[1:])):
            raise RuntimeError('PhysX relocation entries overlap')
        for rva, size, digest in CHUNKS:
            if hashlib.sha256(self.original(rva, size)).hexdigest() != digest:
                raise RuntimeError('PhysX r38 original code certificate differs')

    def section(self, rva, size, executable=False):
        if type(rva) is not int or type(size) is not int or rva < 0 or size <= 0:
            raise RuntimeError('PhysX module range rejected')
        matches = [s for s in self.layout['sections']
                   if s['rva'] <= rva and rva + size <= s['rva'] + s['raw_size']]
        if (len(matches) != 1 or not matches[0]['characteristics'] & 0x40000000 or
                executable and (not matches[0]['characteristics'] & 0x20000000 or
                                matches[0]['characteristics'] & 0x80000000)):
            raise RuntimeError('PhysX original file-backed/readable/code section rejected')
        return matches[0]

    def original(self, rva, size):
        section = self.section(rva, size)
        at = section['raw_pointer'] + rva - section['rva']
        return self.raw[at:at + size]

    def relocated(self, rva, size, base):
        data = bytearray(self.original(rva, size))
        for at in self.relocations:
            if at >= rva + size:
                break
            if at + 8 <= rva:
                continue
            if not rva <= at <= rva + size - 8:
                raise RuntimeError('PhysX observation cuts a relocation field')
            offset = at - rva
            value = struct.unpack_from('<Q', data, offset)[0] + base - self.preferred
            if not 0 <= value < 1 << 64:
                raise RuntimeError('PhysX rebased field overflow')
            struct.pack_into('<Q', data, offset, value)
        return bytes(data)

    def header_at_base(self, base):
        # Actual game33732 retained all header bytes except the PE32+
        # ImageBase field, which exactly matched independent enumeration.
        # Construct that complete expected form; never mask observed bytes
        # or use a live header's value to choose the permitted address.
        if (type(base) is not int or base <= 0 or base % 65536 or
                base + self.image_size >= 1 << 64):
            raise RuntimeError('PhysX bound PE header base rejected')
        header = bytearray(self.header)
        struct.pack_into('<Q', header, self.image_base_offset, base)
        return bytes(header)

    def reverify(self):
        # Reopen and hash; equal file timestamps alone are insufficient.
        current = Artifact(self.path)
        if current.raw != self.raw or current.disk_stat != self.disk_stat:
            raise RuntimeError('PhysX artifact changed before closing')


class PhysXSource:
    def __init__(self, query, game_image, *, enumerate_modules=_modules, artifact_factory=Artifact):
        self.q, self.enumerate = query, enumerate_modules
        self.path = Path(game_image).resolve().with_name('PhysXCore.dll')
        self.artifact = artifact_factory(self.path)
        self.base = self._loaded()
        bound_header = self.artifact.header_at_base(self.base)
        header = query.read(self.base, len(self.artifact.header))
        if header == self.artifact.header:
            header_mode = 'original_preferred_image_base'
        elif header == bound_header:
            header_mode = 'independently_bound_image_base'
        else:
            # Retain the EXACT already-observed bytes on failure. No second
            # read or byte mask. Both complete allowed forms are source-bound.
            raise SourceRejection('PhysX current complete PE headers differ', dict(
                kind='physx_complete_pe_header_mismatch', path=str(self.path), base=self.base,
                artifact_sha256=ARTIFACT_SHA256, expected_header=self.artifact.header.hex(),
                observed_header=header.hex(), expected_size=len(self.artifact.header), observed_size=len(header),
                expected_sha256=hashlib.sha256(self.artifact.header).hexdigest(),
                observed_sha256=hashlib.sha256(header).hexdigest(),
                independently_bound_expected_header=bound_header.hex(),
                independently_bound_expected_sha256=hashlib.sha256(bound_header).hexdigest(),
                image_base_field_offset=self.artifact.image_base_offset,
                differing_offsets=[i for i in range(min(len(header), len(self.artifact.header)))
                                   if header[i] != self.artifact.header[i]],
                closing_verified=False, source_accepted=False, activation_allowed=False,
                difference_cause_verified=False))
        self.header_correspondence = dict(
            mode=header_mode, image_base_field_offset=self.artifact.image_base_offset,
            preferred_image_base=self.artifact.preferred, independently_enumerated_base=self.base,
            observed_image_base=struct.unpack_from('<Q', header, self.artifact.image_base_offset)[0],
            disk_header_sha256=hashlib.sha256(self.artifact.header).hexdigest(),
            observed_header_sha256=hashlib.sha256(header).hexdigest(),
            complete_expected_header_equal=True, closing_exact_reread_required=True,
            loader_cause_verified=False)
        self.code = []
        self.targets = {}
        for rva, size, digest in CHUNKS:
            self.artifact.section(rva, size, True)
            self._bytes(rva, size)
            self.code.append(dict(rva=rva, size=size, original_sha256=digest))

    def _loaded(self):
        rows = self.enumerate(self.q)
        if (type(rows) is not list or not 0 < len(rows) <= 2048 or
                any(type(base) is not int or base <= 0 or base % 65536 for base, _ in rows) or
                len({base for base, _ in rows}) != len(rows)):
            raise RuntimeError('PhysX complete loaded module list rejected')
        selected = [(base, Path(path).resolve()) for base, path in rows
                    if Path(path).name.lower() == 'physxcore.dll']
        if len(selected) != 1 or selected[0][1] != self.path:
            raise RuntimeError('PhysX loaded image missing, ambiguous or outside selected game')
        base = selected[0][0]
        if base + self.artifact.image_size >= 1 << 64:
            raise RuntimeError('PhysX loaded image address overflow')
        return base

    def _bytes(self, rva, size):
        raw = self.q.read(self.base + rva, size)
        if raw != self.artifact.relocated(rva, size, self.base):
            raise RuntimeError('PhysX current original code/vtable bytes differ')
        return raw

    def _slot(self, vtable, offset):
        rva = vtable - self.base + offset
        section = self.artifact.section(rva, 8)
        if section['characteristics'] & (0x80000000 | 0x20000000):
            raise RuntimeError('PhysX vtable slot is not read-only data')
        target = struct.unpack('<Q', self._bytes(rva, 8))[0]
        if target not in self.targets:
            code_rva = target - self.base
            self.artifact.section(code_rva, 32, True)
            raw = self._bytes(code_rva, 32)
            self.targets[target] = dict(address=target, rva=code_rva,
                prefix=raw.hex(), sha256=hashlib.sha256(raw).hexdigest(), methods_called=False)
        return target

    def receiver(self, pointer):
        if type(pointer) is not int or not 0 < pointer < (1 << 64) - 0x48 or pointer & 7:
            raise RuntimeError('PhysX actor pointer rejected')
        vtable = struct.unpack('<Q', self.q.read(pointer, 8))[0]
        slots = {str(offset): self._slot(vtable, offset) for offset in ACTOR_TARGETS}
        if any(slots[str(offset)] != self.base + rva for offset, rva in ACTOR_TARGETS.items()):
            raise RuntimeError('PhysX actor slots differ from r38 analyzed methods')
        internal = struct.unpack('<Q', self.q.read(pointer + 0x40, 8))[0]
        out = dict(actor=pointer, actor_vtable=vtable, actor_slots=slots,
                   internal_body=internal, internal_present=bool(internal), methods_called=False,
                   physical_pose_verified=False, active_physics_verified=False)
        if internal:
            if internal & 7 or not 0 < internal < (1 << 64) - 8:
                raise RuntimeError('PhysX internal body pointer rejected')
            table = struct.unpack('<Q', self.q.read(internal, 8))[0]
            out.update(internal_vtable=table,
                       internal_slots={str(offset): self._slot(table, offset) for offset in INTERNAL_SLOTS})
        return out

    def reverify(self):
        if self._loaded() != self.base:
            raise RuntimeError('PhysX loaded image changed at closing')
        self.artifact.reverify()

    def result(self):
        return dict(path=str(self.path), base=self.base, image_size=self.artifact.image_size,
                    artifact_sha256=ARTIFACT_SHA256, current_analyzed_code=self.code,
                    header_correspondence=copy.deepcopy(self.header_correspondence),
                    targets=list(self.targets.values()), closing_required=True,
                    methods_called=False, physical_pose_verified=False,
                    world_synchronization_verified=False, activation_allowed=False)


def capture_image(observer, *, query_factory=CoverQuery, source_factory=PhysXSource):
    """One owned module/code observation; no cover entry or receiver reads.

    This isolates an actual image/header rejection before doing any cover
    action. Successful selected-code correspondence is not full-image, body,
    collision, pose, synchronization or headset acceptance.
    """
    from cover_bone_query import _ownership
    from cover_surface_query import _lifecycle
    out = dict(schema=1, accepted=False, read_only=True, engine_calls=False,
               queue_submissions=0, body_modified=False, cover_input_sent=False,
               full_image_verified=False, physical_pose_verified=False,
               world_synchronization_verified=False, activation_allowed=False,
               atomic_snapshot_verified=False, started_perf_counter_ns=time.perf_counter_ns())
    q = None
    try:
        game = _ownership(observer)
        lifecycle = _lifecycle(observer)
        out['source_at_start'] = copy.deepcopy(lifecycle)
        q = query_factory(observer)
        source = source_factory(q, game['image'])
        source.reverify()
        q.finish()
        _ownership(observer)
        if _lifecycle(observer) != lifecycle:
            raise RuntimeError('PhysX image observation session/game/owner changed at closing')
        out.update(accepted=True, image=source.result(),
                   limits='Current selected module path/header and eight original code ranges only; '
                          'no receivers, cover action, body shapes or physical synchronization permission')
    except (RuntimeError, OSError, ValueError, TypeError, KeyError, struct.error) as error:
        out.update(rejection=str(error), rejection_type=type(error).__name__)
        witness = getattr(error, 'source_witness', None)
        if witness is not None:
            out['source_rejection_witness'] = copy.deepcopy(witness)
        witness = getattr(q, 'closing_rejection', None)
        if witness is not None:
            out['closing_rejection'] = copy.deepcopy(witness)
    finally:
        if q is not None:
            out.update(reads=q.reads, bytes=q.bytes)
        out['finished_perf_counter_ns'] = time.perf_counter_ns()
    return out
