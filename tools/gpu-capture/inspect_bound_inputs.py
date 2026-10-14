"""Read-only, bounded inspection of final-input objects from this same PID.

Pointers and system-module vtables are candidates, not proof of COM interfaces.
Never invokes a method, injects code or assumes a D3D12 resource state.
"""
import argparse
import json
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/native-capture'))
from start_capture import modules, pefile
from resolve_camera import Resolver


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--status', type=pathlib.Path, required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--depth', type=int, choices=(0, 1, 2, 3), default=0,
                        help='Follow only pointers whose first word belongs to a known module; at most 128 nodes')
    args = parser.parse_args()
    status = args.status.read_text()
    match = re.search(r'^capturing pid=(\d+) ', status, re.M)
    if not match or int(match[1]) != args.pid:
        raise RuntimeError('Status is not from this PID')
    rows = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines()
            if line.startswith('final_input ')]
    objects = {int(value, 16) for row in rows for value in row['textures'].split(',') if value != '0'}
    if not rows or len(objects) > 64:
        raise RuntimeError('Missing or excessive input samples')
    resolver = Resolver(args.pid, json.loads((ROOT / 'tools/package-inspector/reloaded-profile.json').read_text()))
    result = {'pid': args.pid, 'objects': [], 'resource_interface_verified': False,
              'limit': 'Post-trial memory may be stale/reused; two identical reads do not prove lifetime.'}
    try:
        resolver.prepare()
        loaded = modules(resolver.p.handle)
        ranges = []
        for name, (base, path) in loaded.items():
            if name not in ('gowde-steam.exe', 'd3d12.dll', 'd3d12core.dll', 'dxgi.dll'):
                continue
            with pefile.PE(str(path), fast_load=True) as pe:
                ranges.append((name, base, base + pe.OPTIONAL_HEADER.SizeOfImage))

        def owner(address):
            return next((name for name, start, end in ranges if start <= address < end), None)

        pending = [(address, 0) for address in sorted(objects)]
        visited = set()
        while pending and len(visited) < 128:
            address, depth = pending.pop(0)
            if address in visited:
                continue
            visited.add(address)
            raw = resolver.p.read(address, 256)
            entry = {'address': hex(address), 'bytes_read': len(raw),
                     'depth': depth,
                     'stable_two_reads': raw == resolver.p.read(address, 256),
                     'hex': raw.hex(), 'pointer_candidates': []}
            # Engine wrappers in this build contain 64-bit pointers at +0xc,
            # +0x14 and +0x24. An 8-byte scan misses these packed fields.
            for offset in range(0, len(raw) - 7, 4):
                pointer = struct.unpack_from('<Q', raw, offset)[0]
                if not 0x10000 <= pointer < 0x7fffffffffff:
                    continue
                if owner(pointer):
                    continue  # A module/vtable address is not a heap COM-object candidate.
                table_raw = resolver.p.read(pointer, 8)
                if len(table_raw) != 8:
                    continue
                table = struct.unpack('<Q', table_raw)[0]
                module = owner(table)
                if module:
                    entry['pointer_candidates'].append({'offset': hex(offset), 'pointer': hex(pointer),
                                                        'vtable': hex(table), 'vtable_module': module})
                    if depth < args.depth and pointer not in visited:
                        pending.append((pointer, depth + 1))
            result['objects'].append(entry)
        result['node_limit_reached'] = bool(pending)
    finally:
        resolver.p.close()
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({'pid': args.pid, 'objects': [{k: v for k, v in entry.items() if k != 'hex'}
                                                for entry in result['objects']]}, indent=2))


if __name__ == '__main__':
    main()
