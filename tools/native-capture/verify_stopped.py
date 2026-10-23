"""Read-only, independent entry verification after a completed capture.

Camera entries use the gated runtime signatures (protected disk EXE differs).
System-module entries use their on-disk PE bytes with DIR64 relocations applied.
No remote calls, writes or DLL unloads are performed.
"""
import argparse
import bisect
import contextlib
import os
import hashlib
import json
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/package-inspector'))
sys.path.insert(0, str(ROOT / 'tools/package-inspector/python-libs'))
import pefile
from resolve_camera import Resolver
from start_capture import modules

# Fixed historical artifact 1 complete known chunks, independent of capture
# status and live memory. The second chunk is an Output continuation, not a
# separately installed entry. Matching only its entry prefix is insufficient.
DLSS_OUTPUT_RANGES = (
    (0xd82f58, 0xc78, '1efb10541bf56d87b31aae4f452da0d281afd552da16b4bfd16d653f2cd425ea'),
    (0xd83bd0, 0x1e9, 'e051a2d325a4d5ebc12be670d63cd8eae1eead437f33dfa39946de852e61e724'),
)
DLSS_OWNED_DOF_RANGES = (
    (0xd7fd14, 0xe61, '62ceae022db3ea1d7fd0994520e2cbff6c1b1dca288023d1a56bd957fbca7d5c'),
)
DLSS_INPUT_BIND_CHUNKS = ((0x1665dc, 0x1cc), (0x8b492c, 0x2b))
DLSS_INPUT_BIND_SHA256 = '566929fd4f84a21529de0ecb749d2dc75a1bf2ab3cc04876a65f65517222a339'
NGX_WRAPPER_RANGES = (
    (0xbe5b5c, 0xf9, '53709f650d84bd1ea4cc1ae5e25eb8ef7f3462102fe51d07b790d96c48734879'),
    (0xbe5c58, 0x561, '21868e6f8203578f5c4ef689fc4f66a5c79e13fdb243eebe2eb81cc7d6309441'),
)
NGX_DISPATCH_RANGES = (
    (0x7c82f0, 0x76, '25cb57848839e9bf6d5da04fefca109a4a30326b74fcb7f90d5340aa84f969a9'),
    (0x7c8440, 0x35, 'ac6de83a5c7a3c8451f842391785c00e492980557c84328d55509537874d82e7'),
    (0x7c87e0, 0x45, 'ba5c32ea310653f741ee1d10b735249162778259755de6dcb0e8a673b02fff63'),
)
PATH_LAYOUT_RANGES = ((0x298bdc, 161, 'b5dae9f89c8182b26eef27e87ca2ae57804899ed313cb9acdc630280a03b6fb5'),)
PATH_REFLOW_CHUNKS = ((0x2983f8, 666), (0x8ead70, 435))
PATH_REFLOW_SHA256 = '9df4472db8e3ebaca59f6c14356c8de8c685a2aac1a1ff060ca0f7603d945d97'
PATH_REFLOW_CHILD_CHUNKS = ((0x298694, 201), (0x8eaf24, 39))
PATH_REFLOW_CHILD_SHA256 = 'c455b118525aa5c957e5eea29cd8a46479aa2a95ef633693d4aa04da0e15ac28'
PATH_CLOSE_CHUNKS = ((0x346574, 62),)
PATH_CLOSE_SHA256 = 'b8c82a830ef05c50a88489351c469e591b65a154310a2fefec91e57f66fb7b3a'
PATH_CLOSE_EXEC_CHUNKS = ((0x34698c, 128), (0x90dbe6, 32))
PATH_CLOSE_EXEC_SHA256 = '1f03d742fcd859a664004bb62ff35c327d80b1364b235701e64184eee53e55c1'


def check_runtime_ranges(read, base, target, ranges, source):
    checks = []
    for rva, size, expected_hash in ranges:
        current = read(base + rva, size)
        actual_hash = hashlib.sha256(current).hexdigest()
        checks.append(dict(target=hex(target), range_rva=hex(rva), source=source,
                           bytes_checked=len(current), expected_size=size, expected_sha256=expected_hash,
                           actual_sha256=actual_hash, matches=len(current) == size and actual_hash == expected_hash))
    return checks


def check_runtime_chunks(read, base, target, chunks, expected_hash, source):
    data = [read(base + rva, size) for rva, size in chunks]
    actual_hash = hashlib.sha256(b''.join(data)).hexdigest()
    return dict(target=hex(target), source=source, chunks=[dict(rva=hex(rva), size=size) for rva, size in chunks],
                bytes_checked=sum(map(len, data)), expected_size=sum(size for _, size in chunks),
                expected_sha256=expected_hash, actual_sha256=actual_hash,
                matches=all(len(raw) == size for raw, (_, size) in zip(data, chunks)) and actual_hash == expected_hash)


def check_path_close(read, base, target, native_pointer_addresses=()):
    checks=[]
    for chunks, digest, source in (
        (PATH_CLOSE_CHUNKS,PATH_CLOSE_SHA256,'CloseScene complete 62B'),
        (PATH_CLOSE_EXEC_CHUNKS,PATH_CLOSE_EXEC_SHA256,'CloseScene exec complete 160B')):
        row=check_runtime_chunks(read,base,target,chunks,digest,'artifact 1 runtime snapshot '+source)
        row.update(module_base=hex(base),historical_module_base='0x140000000',
                   source_snapshot_sha256='911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6')
        checks.append(row)
    pointers=[(base+0x1429d58+0x228,base+0x346574,'CloseScene global vtable +228')]
    pointers += [(address,base+0x34698c,'CloseScene configured UFunction native pointer') for address in native_pointer_addresses]
    for address,expected,source in pointers:
        raw=read(address,8)
        checks.append(dict(target=hex(address),source=source,bytes_checked=len(raw),expected_size=8,
                           expected_target=hex(expected),matches=raw==struct.pack('<Q',expected),module_base=hex(base)))
    return checks


def hook_targets(status):
    targets = [int(value, 16) for value in re.findall(r'hook_target=([0-9A-Fa-f]+) create=MH_OK queue=MH_OK', status)]
    # capture.cpp: largest base 60 + owned UI 4 + optional hook slots 38.
    # This is only an inventory bound; every entry still requires independent
    # byte verification below. The combined owned/NGX candidate installs 91
    # unique entries; mutually exclusive options make 102 a capacity bound.
    if not 2 <= len(targets) <= 102 or len(set(targets)) != len(targets):
        raise RuntimeError('Unexpected hook target inventory')
    return targets


def prime_preinstall_entries(status, prime_status=None):
    """Keep only complete, unique pre-hook and post-stop snapshots of one entry.

    The native hook records `before` immediately before MH_CreateHook, then checks
    the same 32 bytes after disabling hooks. The verifier still reads the stopped
    process independently; a native `restored=1` claim alone never passes.
    """
    initial, stopped, duplicates = {}, {}, set()
    for line in (prime_status if prime_status is not None else status).splitlines():
        before = re.fullmatch(r'prime_entry target=([0-9A-Fa-f]+) before=([0-9A-Fa-f]{64})', line)
        if before:
            target, data = int(before[1], 16), bytes.fromhex(before[2])
            if target in initial:duplicates.add(target)
            initial[target] = data
    for line in status.splitlines():
        after = re.fullmatch(r'prime_entry target=([0-9A-Fa-f]+) restored=([01]) before=([0-9A-Fa-f]{64})', line)
        if after:
            target, restored, data = int(after[1], 16), after[2] == '1', bytes.fromhex(after[3])
            if target in stopped:duplicates.add(target)
            stopped[target] = restored, data
    return {target: data for target, data in initial.items()
            if target not in duplicates and stopped.get(target) == (True, data)}


def session_preinstall_entries(manifest, pid, owner_pid, owner_creation_time,
                               game_creation_time=None):
    """Use a sealed session's pre-hook bytes only for its exact process owner.

    The independent remote read below still decides whether hooks were restored.
    This snapshot supplies an expected value when graphics functions already had
    a third-party detour before our hook was installed.
    """
    if (manifest.get('schema') != 1 or manifest.get('pid') != pid
            or manifest.get('owner_pid') != owner_pid
            or manifest.get('owner_creation') != owner_creation_time
            or (game_creation_time is not None
                and manifest.get('game_creation') != game_creation_time)):
        raise RuntimeError('Hook manifest process/owner identity mismatch')
    entries = manifest.get('entries')
    if not isinstance(entries, list) or not 1 <= len(entries) <= 102:
        raise RuntimeError('Unexpected hook manifest entry inventory')
    original = {}
    for item in entries:
        target = item.get('target')
        data = item.get('original')
        if (not isinstance(target, int) or target <= 0 or target in original
                or not isinstance(data, str) or len(data) != 64):
            raise RuntimeError('Invalid or duplicate hook manifest entry')
        try:
            original[target] = bytes.fromhex(data)
        except ValueError as error:
            raise RuntimeError('Invalid hook manifest original bytes') from error
    return original


class SystemEntryCache:
    """PE resources and relocation indices owned by ONE verification run.

    Full parsing and all existing relocation rules are retained. The index
    limits lookup work, never the set of applicable relocation entries.
    """
    def __init__(self):
        self._images = {}
        self._resources = contextlib.ExitStack()

    def _image(self, path):
        key = os.path.normcase(os.path.abspath(path))
        if key not in self._images:
            pe = pefile.PE(str(path))
            self._resources.callback(pe.close)
            # Keep original traversal order for duplicate/overlapping entries;
            # sorting is only an index for selecting the intersecting window.
            relocations = sorted((entry.rva, index, entry.type)
                for index, entry in enumerate(entry for block in getattr(pe, 'DIRECTORY_ENTRY_BASERELOC', [])
                                               for entry in block.entries)
                if entry.type != 0)
            self._images[key] = pe, relocations
        return self._images[key]

    def expected(self, loaded, target):
        owner = None
        for name, (base, path) in loaded.items():
            if name not in ('dxgi.dll', 'd3d12.dll', 'd3d12core.dll'):
                continue
            pe, relocations = self._image(path)
            if base <= target < base + pe.OPTIONAL_HEADER.SizeOfImage:
                owner = base, path, pe, relocations
                break
        if owner is None:
            raise RuntimeError('Target not in an allowed graphics system module')
        base, path, pe, relocations = owner
        offset = target - base
        expected = bytearray(pe.get_data(offset, 32))
        if len(expected) != 32:
            raise RuntimeError('Missing on-disk entry bytes')
        delta = base - pe.OPTIONAL_HEADER.ImageBase
        first = bisect.bisect_left(relocations, (offset - 7,))
        last = bisect.bisect_left(relocations, (offset + 32,))
        for rva, _, kind in sorted(relocations[first:last], key=lambda row: row[1]):
            if kind != 10 or not offset <= rva <= offset + 24:
                raise RuntimeError('Unsupported or partial entry relocation')
            at = rva - offset
            value = struct.unpack_from('<Q', expected, at)[0]
            struct.pack_into('<Q', expected, at, (value + delta) & 0xffffffffffffffff)
        return bytes(expected), str(path)

    def close(self):
        # ExitStack runs every close even when an individual close raises.
        try:self._resources.close()
        finally:self._images.clear()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--status', type=pathlib.Path, required=True)
    parser.add_argument('--prime-status', type=pathlib.Path)
    parser.add_argument('--hook-manifest', type=pathlib.Path)
    parser.add_argument('--game-creation-time', type=int)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    status = args.status.read_text()
    if f'capturing pid={args.pid} ' not in status or 'stopped disable_status=0 active=0' not in status:
        raise RuntimeError('Matching PID and clean stopped status required')
    targets = hook_targets(status)
    prime_status = args.prime_status.read_text() if args.prime_status else None
    owner = None
    if prime_status is not None:
        owner = re.search(r'^continuous_owner_verified=1 owner_pid=(\d+) owner_creation_time=(\d+)$',
                          status, re.MULTILINE)
        prime_owner = re.search(r'^capturing pid=(\d+) lifetime=owner owner_pid=(\d+) '
                                r'owner_creation_time=(\d+)\b', prime_status, re.MULTILINE)
        if (not owner or not prime_owner or int(prime_owner[1]) != args.pid
                or prime_owner.groups()[1:] != owner.groups()):
            raise RuntimeError('Prime status PID/owner identity does not match stopped capture')
    preinstall_entries = prime_preinstall_entries(status, prime_status)
    if args.hook_manifest:
        if not owner:
            owner = re.search(r'^continuous_owner_verified=1 owner_pid=(\d+) owner_creation_time=(\d+)$',
                              status, re.MULTILINE)
        if not owner:
            raise RuntimeError('Stopped capture lacks matching owner identity')
        manifest = json.loads(args.hook_manifest.read_text())
        session_entries = session_preinstall_entries(
            manifest, args.pid, int(owner[1]), int(owner[2]), args.game_creation_time)
        if not set(session_entries).issubset(set(targets)):
            raise RuntimeError('Hook manifest contains targets outside stopped capture')
        for target, original in session_entries.items():
            if target in preinstall_entries and preinstall_entries[target] != original:
                raise RuntimeError('Prime and hook manifest originals disagree')
            preinstall_entries[target] = original
    # UI observer adds four VM/Canvas entries, Canvas/subtitle consumers, and
    # optionally the Material consumer, three graphics settings callbacks, and
    # the UIOptionList selection transaction and optional ResourceBarrier entry.
    # ResourceBarrier and the optional three death PSO observers (ClearState,
    # SetPipelineState, ExecuteBundle) use the existing system-module PE/relocation
    # verification below; there is no additional allowlist or byte exemption.
    # Owned Material adds its producer and the certified nonindexed upload entry.
    resolver = Resolver(args.pid, json.loads((ROOT / 'tools/package-inspector/reloaded-profile.json').read_text()))
    checks = []
    system_entries = SystemEntryCache()
    try:
        resolver.prepare()
        loaded = modules(resolver.p.handle)
        camera = {0x15cf08: bytes.fromhex('488bc45356574154415541564157'),
                  0x15c968: bytes.fromhex('488bc4488958084889681048897018')}
        camera[0x3a148c]=bytes.fromhex('48895c2408488974241855')
        camera[0x193d50]=bytes.fromhex('488bc4488958104889681848897020')
        camera[0x116250]=bytes.fromhex('488bc44889581848897020574154')
        camera[0x15a80c]=bytes.fromhex('48895c241055488bec4883ec70')
        camera[0x15a974]=bytes.fromhex('40534883ec20488bd9')
        camera[0x1fc060]=bytes.fromhex('488bc44889581048897018574154')
        camera[0x33bc70]=bytes.fromhex('48895c2408574883ec208bda488bf9')
        camera[0x13cfcc]=bytes.fromhex('44894c242048894c240855535657')
        camera[0x1af42c]=bytes.fromhex('488bc4488958084889701048897818')
        camera[0x2a7e48]=bytes.fromhex('488bc448895808488970104889781855')
        camera[0x1d6ca0]=bytes.fromhex('4053555641564881eca8000000488b058c')
        # Optional observation-only four-pointer call boundary. Fixed saved
        # artifact 1 bytes, current-process instruction comparison: docs/91.
        camera[0x1d6f30]=bytes.fromhex('405553565741544155415641574881ec38050000488d6c2430c5f829b5f00400')
        # Optional crosshair assignment observer. Historical runtime execLet
        # contract, independently compared after disabling hooks; never learn
        # an expected entry from the patched live process or capture log.
        camera[0x1d7ad0]=bytes.fromhex('48895c240848896c2418488974242057415641574883ec30488bda4c8d358ee7')
        camera[0x1d52b4]=bytes.fromhex('488bc448895808555657488d68a14881ec')
        camera[0x3a4d3c]=bytes.fromhex('488bc4488958084889701855574156488d')
        camera[0x2482b8]=bytes.fromhex('488bc44889581855565741564157488d68')
        camera[0x19e64c]=bytes.fromhex('488bc44889581048896818565741574883ec50c5f82970d8488bd9488b492841')
        camera[0x2a4370]=bytes.fromhex('48895c24084889742410574883ec30488b05cabf9701498bd8488bfa488bf148')
        camera[0x15a420]=bytes.fromhex('488bc4488958104889681848897020574883ec70488bd9488d4808e808feffff')
        camera[0x117430]=bytes.fromhex('488bc448895810555657415441554156')
        camera[0x280da4]=bytes.fromhex('33c041b801000000448981140300008981d4020000898180030000488981c803')
        camera[0x2ac894]=bytes.fromhex('488bc4488958104889701848897820554154415541564157488da8f8feffff48')
        camera[0xff6f4]=bytes.fromhex('488bc44889581048896818488970205741544155415641574883ec308b590c48')
        camera[0xc7f63c]=bytes.fromhex('48895c240848896c24104889742418574883ec308bda488bf93b91d80500000f')
        for target in targets:
            rva = target - resolver.base
            if rva == 0x346574:
                addresses=sorted(set(int(value) for value in re.findall(r'path_close_loaded [^\n]*?native_pointer=(\d+)',status)))
                if len(addresses)>16 or any(not 0<address<(1<<64)-8 for address in addresses):
                    raise RuntimeError('Unexpected CloseScene native pointer inventory')
                checks.extend(check_path_close(resolver.read,resolver.base,target,addresses))
                continue
            if rva in (0x2983f8, 0x298694):
                # Independent historical expectations: the complete hooked body
                # AND its authenticated child, including both cold chunks.
                for checked_target, chunks, expected_hash, source in (
                    (resolver.base + 0x2983f8, PATH_REFLOW_CHUNKS, PATH_REFLOW_SHA256,
                     'artifact 1 runtime snapshot PathChoice reflow complete chunks'),
                    (resolver.base + 0x298694, PATH_REFLOW_CHILD_CHUNKS, PATH_REFLOW_CHILD_SHA256,
                     'artifact 1 runtime snapshot PathChoice reflow child complete chunks'),
                ):
                    check = check_runtime_chunks(resolver.read, resolver.base, checked_target, chunks, expected_hash, source)
                    check.update(module_base=hex(resolver.base), historical_module_base='0x140000000',
                                 source_snapshot_sha256='911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6')
                    checks.append(check)
                for slot, expected_rva in ((0x528, 0x2983f8), (0x2c8, 0x298694)):
                    address = resolver.base + 0x138a558 + slot
                    expected = struct.pack('<Q', resolver.base + expected_rva)
                    current = resolver.read(address, 8)
                    checks.append(dict(target=hex(address), source='artifact 1 runtime snapshot PathChoice global vtable slot',
                        vtable_rva='0x138a558', slot=hex(slot), expected_target_rva=hex(expected_rva), bytes_checked=len(current),
                        expected_size=8, matches=current == expected, module_base=hex(resolver.base),
                        historical_module_base='0x140000000',
                        source_rdata_sha256='c5deb08128dea890e2606b7bb6bf30eb3e1045c19b3f477e0bebf6c7fd15231c'))
                continue
            if rva == 0x298bdc:
                values = check_runtime_ranges(resolver.read, resolver.base, target, PATH_LAYOUT_RANGES,
                              'artifact 1 runtime snapshot PathChoice layout complete body')
                for check in values:
                    check.update(module_base=hex(resolver.base), historical_module_base='0x140000000',
                                 source_snapshot_sha256='911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6')
                checks.extend(values)
                continue
            if rva == 0x296178:
                # Optional UI scene root observer: exactly the saved 13-byte
                # artifact 1 adapter, also recorded in ui_scene_root_candidate.h
                # (docs/122,130). Historical code section SHA256:
                # 911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6.
                # Never derive expected bytes from capture status or live memory.
                expected = bytes.fromhex('498b00498bc848ffa0a8040000')
                current = resolver.read(target, len(expected))
                checks.append(dict(target=hex(target), source='artifact 1 runtime snapshot UI scene root adapter',
                                   bytes_checked=len(expected), expected_sha256=hashlib.sha256(expected).hexdigest(),
                                   actual_sha256=hashlib.sha256(current).hexdigest(), matches=current == expected))
                continue
            if rva == 0x1c4b90:
                # VR DOF compatibility or its prior diagnostic: independently
                # verify the full authenticated 200-byte PS setter after restore.
                expected_hash = '10b2617802b5648fea992132f8b9e20d68d58940de7e7779f34298b8be240cb0'
                current = resolver.read(target, 200)
                actual_hash = hashlib.sha256(current).hexdigest()
                checks.append(dict(target=hex(target), source='artifact 1 runtime snapshot death PS constant setter',
                                   bytes_checked=len(current), expected_sha256=expected_hash,
                                   actual_sha256=actual_hash, matches=actual_hash == expected_hash))
                continue
            if rva == 0x1760b4:
                # Shared read-only death/DLSS route constant observer. Authenticate the
                # entire saved 205-byte setter independently after disabling,
                # never use an expected entry learned from the patched process.
                expected_hash = 'd7d20d3f5c510685b8b3ffb12d12d51ae583d4070e00cf11a39655c7e086c45b'
                current = resolver.read(target, 205)
                actual_hash = hashlib.sha256(current).hexdigest()
                checks.append(dict(target=hex(target), source='artifact 1 runtime snapshot death constant setter',
                                   bytes_checked=len(current), expected_sha256=expected_hash,
                                   actual_sha256=actual_hash, matches=actual_hash == expected_hash))
                continue
            if rva == 0xd7fd14:
                values = check_runtime_ranges(resolver.read, resolver.base, target, DLSS_OWNED_DOF_RANGES,
                              'artifact 1 runtime snapshot owned DOF renderer complete body')
                for check in values:
                    check.update(module_base=hex(resolver.base), historical_module_base='0x140000000',
                                 source_snapshot_sha256='911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6')
                checks.extend(values)
                continue
            if rva == 0xd82f58:
                checks.extend(check_runtime_ranges(resolver.read, resolver.base, target, DLSS_OUTPUT_RANGES,
                              'artifact 1 runtime snapshot DLSS Output full known chunks'))
                continue
            if rva == 0x1665dc:
                check = check_runtime_chunks(resolver.read, resolver.base, target, DLSS_INPUT_BIND_CHUNKS,
                              DLSS_INPUT_BIND_SHA256, 'artifact 1 runtime snapshot DLSS input bind full known chunks')
                check.update(module_base=hex(resolver.base), historical_module_base='0x140000000',
                             source_snapshot_sha256='911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6')
                checks.append(check)
                continue
            if rva in (0xbe5b5c, 0xbe5c58, 0x7c82f0, 0x7c8440, 0x7c87e0):
                ranges = tuple(item for item in NGX_WRAPPER_RANGES + NGX_DISPATCH_RANGES if item[0] == rva)
                values = check_runtime_ranges(resolver.read, resolver.base, target, ranges,
                              'artifact 1 runtime snapshot NGX wrapper/dispatcher complete known chunk')
                for check in values:
                    check.update(module_base=hex(resolver.base), historical_module_base='0x140000000',
                                 source_snapshot_sha256='911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6')
                checks.extend(values)
                continue
            material_ranges = {
                0x3a514c: (0x3fc, '193ddb21a13443746e258f12f799968a47ca9d21fe8badf3cd5c1884a462931b'),
                0x1af054: (0x2fb, '4cf2c5b249b2abb2decb11cae8adf0eacc3a08833345691e63fcbaf7eae7c41a'),
            }
            if rva in material_ranges:
                size, expected_hash = material_ranges[rva]
                current_hash = hashlib.sha256(resolver.read(target, size)).hexdigest()
                checks.append(dict(target=hex(target), source='artifact 1 runtime snapshot Material range',
                                   bytes_checked=size, expected_sha256=expected_hash,
                                   actual_sha256=current_hash, matches=current_hash == expected_hash))
                continue
            if rva == 0x1172c4:
                expected_hash = '2b491eda062a05982825387297016a4433ce4668f6ab65774bc7271159b7f356'
                current = resolver.read(target, 0x11742f - 0x1172c4)
                actual_hash = hashlib.sha256(current).hexdigest()
                checks.append(dict(target=hex(target), source='artifact 1 runtime snapshot flush range',
                                   bytes_checked=len(current), expected_sha256=expected_hash,
                                   actual_sha256=actual_hash, matches=actual_hash == expected_hash))
                continue
            if rva == 0x2b59f4:
                # Optional resize observer: verify both known function chunks,
                # independently of its own pre-install hash/status assertion.
                expected_hash = 'caaeccd655338a64bccd980b7844e39f05550bdbd33fd1d2e93f11cb3b220f59'
                current = resolver.read(target, 0x124) + resolver.read(resolver.base + 0x8ef386, 0x48)
                current_hash = hashlib.sha256(current).hexdigest()
                checks.append(dict(target=hex(target), source='artifact 1 runtime snapshot full known chunks',
                                   bytes_checked=len(current), expected_sha256=expected_hash,
                                   actual_sha256=current_hash, matches=current_hash == expected_hash))
                continue
            if rva in camera:
                expected = camera[rva]
                source = 'validated runtime camera signature'
            else:
                expected, source = system_entries.expected(loaded, target)
                actual = resolver.read(target, len(expected))
                original = preinstall_entries.get(target)
                matches_disk = actual == expected
                matches_preinstall = original is not None and actual == original
                checks.append(dict(target=hex(target), source=source,
                                   bytes_checked=len(expected), matches=matches_disk or matches_preinstall,
                                   matches_disk=matches_disk, matches_preinstall=matches_preinstall,
                                   disk_sha256=hashlib.sha256(expected).hexdigest(),
                                   observed_sha256=hashlib.sha256(actual).hexdigest(),
                                   preinstall_sha256=(hashlib.sha256(original).hexdigest() if original else None)))
                continue
            actual = resolver.read(target, len(expected))
            checks.append(dict(target=hex(target), source=source, bytes_checked=len(expected), matches=actual == expected))
    finally:
        try:system_entries.close()
        finally:resolver.p.close()
    report = dict(pid=args.pid, checks=checks, all_match=all(item['matches'] for item in checks),
                  limitation='Only checked entry bytes; DLLs and trampolines remain resident until game exit.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
    if not report['all_match']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
