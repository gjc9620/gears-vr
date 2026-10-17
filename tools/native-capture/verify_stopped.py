"""Read-only, independent entry verification after a completed capture.

Camera entries use the gated runtime signatures (protected disk EXE differs).
System-module entries use their on-disk PE bytes with DIR64 relocations applied.
No remote calls, writes or DLL unloads are performed.
"""
import argparse
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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--status', type=pathlib.Path, required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    status = args.status.read_text()
    if f'capturing pid={args.pid} ' not in status or 'stopped disable_status=0 active=0' not in status:
        raise RuntimeError('Matching PID and clean stopped status required')
    targets = [int(value, 16) for value in re.findall(r'hook_target=([0-9A-Fa-f]+) create=MH_OK queue=MH_OK', status)]
    # UI observer adds four VM/Canvas entries, Canvas/subtitle consumers, and
    # optionally the Material consumer, three graphics settings callbacks, and
    # the UIOptionList selection transaction and optional ResourceBarrier entry.
    # ResourceBarrier and the optional three death PSO observers (ClearState,
    # SetPipelineState, ExecuteBundle) use the existing system-module PE/relocation
    # verification below; there is no additional allowlist or byte exemption.
    # Owned Material adds its producer and the certified nonindexed upload entry.
    if not 2 <= len(targets) <= 64 or len(set(targets))!=len(targets):
        raise RuntimeError('Unexpected hook target inventory')
    resolver = Resolver(args.pid, json.loads((ROOT / 'tools/package-inspector/reloaded-profile.json').read_text()))
    checks = []
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
                # Optional read-only death constant observer. Authenticate the
                # entire saved 205-byte setter independently after disabling,
                # never use an expected entry learned from the patched process.
                expected_hash = 'd7d20d3f5c510685b8b3ffb12d12d51ae583d4070e00cf11a39655c7e086c45b'
                current = resolver.read(target, 205)
                actual_hash = hashlib.sha256(current).hexdigest()
                checks.append(dict(target=hex(target), source='artifact 1 runtime snapshot death constant setter',
                                   bytes_checked=len(current), expected_sha256=expected_hash,
                                   actual_sha256=actual_hash, matches=actual_hash == expected_hash))
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
                owner = None
                for name, (base, path) in loaded.items():
                    if name not in ('dxgi.dll', 'd3d12.dll', 'd3d12core.dll'):
                        continue
                    pe = pefile.PE(str(path))
                    if base <= target < base + pe.OPTIONAL_HEADER.SizeOfImage:
                        owner = (base, path, pe)
                        break
                    pe.close()
                if owner is None:
                    raise RuntimeError('Target not in an allowed graphics system module')
                base, path, pe = owner
                try:
                    offset = target - base
                    expected = bytearray(pe.get_data(offset, 32))
                    if len(expected) != 32:
                        raise RuntimeError('Missing on-disk entry bytes')
                    delta = base - pe.OPTIONAL_HEADER.ImageBase
                    for block in getattr(pe, 'DIRECTORY_ENTRY_BASERELOC', []):
                        for entry in block.entries:
                            if entry.type == 0 or not offset - 7 <= entry.rva < offset + 32:
                                continue
                            if entry.type != 10 or not offset <= entry.rva <= offset + 24:
                                raise RuntimeError('Unsupported or partial entry relocation')
                            at = entry.rva - offset
                            value = struct.unpack_from('<Q', expected, at)[0]
                            struct.pack_into('<Q', expected, at, (value + delta) & 0xffffffffffffffff)
                    expected = bytes(expected)
                    source = str(path)
                finally:
                    pe.close()
            actual = resolver.read(target, len(expected))
            checks.append(dict(target=hex(target), source=source, bytes_checked=len(expected), matches=actual == expected))
    finally:
        resolver.p.close()
    report = dict(pid=args.pid, checks=checks, all_match=all(item['matches'] for item in checks),
                  limitation='Only checked entry bytes; DLLs and trampolines remain resident until game exit.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
    if not report['all_match']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
