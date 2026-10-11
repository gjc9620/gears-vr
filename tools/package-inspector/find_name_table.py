"""Find pointer-table evidence for observed name entries; read-only."""
from runtime_probe import Process, readable_heap
import json
import pathlib
import struct
import sys

pid, probe_path, output = int(sys.argv[1]), pathlib.Path(sys.argv[2]), pathlib.Path(sys.argv[3])
probe = json.loads(probe_path.read_text())
p = Process(pid)
targets = {}
for hit in probe['name_hits']:
    if hit['name'] not in ('WarPlayerCamera', 'CameraCache', 'PlayerUpdateCameraNative', 'GetCameraViewPoint'):
        continue
    address = int(hit['address'], 16)
    for delta in (20,):
        targets[struct.pack('<Q', address - delta)] = dict(name=hit['name'], entry=address-delta, delta=delta)
results = []
try:
    for r in p.regions():
        # Private writable allocations only; no executable/protection sections.
        if not readable_heap(r) or r['base'] > 0x40000000:
            continue
        for offset in range(0, r['size'], 4*1024*1024):
            base = r['base'] + offset
            data = p.read(base, min(4*1024*1024+8, r['size']-offset))
            for needle, info in targets.items():
                pos = data.find(needle)
                while pos >= 0:
                    loc = base+pos
                    if loc % 8 == 0:
                        neighbors = p.read(max(loc-16, r['base']), 40)
                        pointers = list(struct.unpack('<'+'Q'*(len(neighbors)//8), neighbors))
                        strings=[]
                        for ptr in pointers:
                            entry = p.read(ptr, 100)
                            text = entry[info['delta']:].decode('utf-16-le', errors='replace').split('\0')[0]
                            strings.append(text[:35])
                        results.append(dict(info, pointer_location=hex(loc), neighbors=[hex(v) for v in pointers], strings=strings))
                    pos=data.find(needle,pos+8)
    output.write_text(json.dumps(results, indent=2), encoding='utf-8')
    print(json.dumps(results, indent=2))
finally:
    p.close()
