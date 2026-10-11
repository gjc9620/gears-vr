"""Read-only game-process diagnostics. No writes, debugger attach or injection."""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import pathlib
import struct
import time

class MBI(c.Structure):
    _fields_ = [('BaseAddress', c.c_void_p), ('AllocationBase', c.c_void_p),
                ('AllocationProtect', w.DWORD), ('PartitionId', w.WORD),
                ('RegionSize', c.c_size_t), ('State', w.DWORD),
                ('Protect', w.DWORD), ('Type', w.DWORD)]

k = c.WinDLL('kernel32', use_last_error=True)
k.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
k.OpenProcess.restype = w.HANDLE
k.VirtualQueryEx.argtypes = [w.HANDLE, c.c_void_p, c.POINTER(MBI), c.c_size_t]
k.VirtualQueryEx.restype = c.c_size_t
k.ReadProcessMemory.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
k.ReadProcessMemory.restype = w.BOOL
k.CloseHandle.argtypes = [w.HANDLE]

class Process:
    def __init__(self, pid):
        self.handle = k.OpenProcess(0x410, False, pid)  # QUERY_INFORMATION | VM_READ
        if not self.handle:
            raise OSError(c.get_last_error(), 'OpenProcess read-only failed')
    def read(self, address, size):
        buffer = c.create_string_buffer(size)
        n = c.c_size_t()
        if not k.ReadProcessMemory(self.handle, address, buffer, size, c.byref(n)):
            return b''
        return buffer.raw[:n.value]
    def regions(self):
        address = 0
        while address < 0x7fffffffffff:
            mbi = MBI()
            if not k.VirtualQueryEx(self.handle, address, c.byref(mbi), c.sizeof(mbi)):
                break
            base = mbi.BaseAddress or 0
            yield dict(base=base, size=mbi.RegionSize, state=mbi.State,
                       protect=mbi.Protect, type=mbi.Type)
            address = base + mbi.RegionSize
    def close(self):
        k.CloseHandle(self.handle)

def readable_heap(region):
    return (region['state'] == 0x1000 and region['type'] == 0x20000
            and not region['protect'] & 0x100
            and region['protect'] & 0xff in (0x04, 0x08))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('pid', type=int)
    parser.add_argument('output', type=pathlib.Path)
    parser.add_argument('--max-mb', type=int, default=2048)
    args = parser.parse_args()
    p = Process(args.pid)
    try:
        regions = list(p.regions())
        names = ['WarPlayerCamera', 'CameraCache', 'PlayerUpdateCameraNative', 'GetCameraViewPoint', 'WarPC', 'WarLocalPlayer']
        patterns = [(name, encoding, (name + '\0').encode(encoding))
                    for name in names for encoding in ('ascii', 'utf-16-le')]
        hits = []
        read_bytes = 0
        started = time.monotonic()
        for r in regions:
            if not readable_heap(r):
                continue
            for offset in range(0, r['size'], 4 * 1024 * 1024):
                size = min(4 * 1024 * 1024 + 128, r['size'] - offset)
                if read_bytes + size > args.max_mb * 1024 * 1024:
                    break
                base = r['base'] + offset
                data = p.read(base, size)
                read_bytes += len(data)
                for name, enc, pattern in patterns:
                    pos = data.find(pattern)
                    while pos >= 0:
                        if sum(h['name'] == name for h in hits) < 30:
                            address = base + pos
                            before = p.read(address - 32, 32)
                            hits.append(dict(name=name, encoding=enc, address=hex(address),
                                             before_hex=before.hex(), region_base=hex(r['base'])))
                        pos = data.find(pattern, pos + len(pattern))
        result = dict(pid=args.pid, mode='read-only', read_bytes=read_bytes,
                      elapsed_seconds=round(time.monotonic() - started, 2), name_hits=hits,
                      regions=regions)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
        print(json.dumps({key: value for key, value in result.items() if key != 'regions'}, indent=2))
    finally:
        p.close()

if __name__ == '__main__':
    main()
