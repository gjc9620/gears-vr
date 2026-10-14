"""Read-only, independent entry verification after a completed capture.

Camera entries use the gated runtime signatures (protected disk EXE differs).
System-module entries use their on-disk PE bytes with DIR64 relocations applied.
No remote calls, writes or DLL unloads are performed.
"""
import argparse
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
    # UI observer adds four explicitly verified VM/Canvas entries to the existing28.
    if not 2 <= len(targets) <= 32 or len(set(targets))!=len(targets):
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
        camera[0x1d52b4]=bytes.fromhex('488bc448895808555657488d68a14881ec')
        camera[0x3a4d3c]=bytes.fromhex('488bc4488958084889701855574156488d')
        camera[0x2482b8]=bytes.fromhex('488bc44889581855565741564157488d68')
        for target in targets:
            rva = target - resolver.base
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
