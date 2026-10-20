"""Read only: identify live D3D/DXGI import owners in the validated game."""
import argparse
import json
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/native-capture'))
from start_capture import modules, pefile
from resolve_camera import Resolver

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    profile = json.loads((ROOT / 'tools/package-inspector/reloaded-profile.json').read_text())
    resolver = Resolver(args.pid, profile)
    try:
        resolver.prepare()
        process = resolver.p
        loaded = modules(process.handle)
        base, image = loaded['gowde-steam.exe']
        with pefile.PE(str(image), fast_load=True) as pe:
            disk_imports = pe.OPTIONAL_HEADER.DATA_DIRECTORY[1]
        with pefile.PE(data=process.read(base, 65536), fast_load=True) as live_pe:
            imports = live_pe.OPTIONAL_HEADER.DATA_DIRECTORY[1]
        rows = []
        for index in range(min(imports.Size // 20, 512)):
            raw = process.read(base + imports.VirtualAddress + index * 20, 20)
            if len(raw) != 20: raise RuntimeError('Import descriptor unreadable')
            original, _, _, name_rva, thunk_rva = struct.unpack('<5I', raw)
            if not name_rva: break
            name = process.read(base + name_rva, 128).split(b'\0')[0].decode('ascii', 'replace')
            if not any(part in name.lower() for part in ('d3d', 'dxgi')): continue
            for slot in range(512):
                raw = process.read(base + (original or thunk_rva) + slot * 8, 8)
                if len(raw) != 8: raise RuntimeError('Import thunk unreadable')
                symbol_rva = struct.unpack('<Q', raw)[0]
                if not symbol_rva: break
                if symbol_rva & (1 << 63): continue
                if symbol_rva > 0x10000000: break  # bound malformed/live-resolved thunk
                symbol = process.read(base + symbol_rva + 2, 128).split(b'\0')[0].decode('ascii','replace')
                target_raw = process.read(base + thunk_rva + slot * 8, 8)
                target = struct.unpack('<Q', target_raw)[0]
                owners = []
                for module, (module_base, module_path) in loaded.items():
                    if module_base > target: continue
                    try:
                        with pefile.PE(str(module_path), fast_load=True) as module_pe:
                            if target < module_base + module_pe.OPTIONAL_HEADER.SizeOfImage: owners.append(module)
                    except OSError: pass
                rows.append(dict(import_dll=name, symbol=symbol, slot=hex(base+thunk_rva+slot*8),
                                 target=hex(target), owners=owners))
        report=dict(pid=args.pid, mode='read-only', imports=rows,
                    disk_import_directory=dict(rva=hex(disk_imports.VirtualAddress), size=disk_imports.Size),
                    live_import_directory=dict(rva=hex(imports.VirtualAddress), size=imports.Size),
                    caveat='Static import table only; dynamic device creation is not covered.')
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
        print(json.dumps(report, indent=2))
    finally: resolver.p.close()

if __name__ == '__main__': main()
