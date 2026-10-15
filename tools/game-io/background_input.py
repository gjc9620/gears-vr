"""Version-gated, reversible runtime controller focus gates for Reloaded.

Only the controller block in RVA 19ab10 is changed. The mouse path, keyboard
messages and key-release cleanup keep their original focus handling. No EXE is
written. See docs/67-background-controller.md for evidence and limitations.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
import pathlib
import struct
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/package-inspector'))
sys.path.insert(0, str(ROOT / 'tools/native-capture'))
from runtime_probe import Process
from run_stereo_continuous import WindowsControl
from start_capture import modules

FUNCTION_RVA = 0x19AB10
FUNCTION_SIZE = 0x147D
FUNCTION_SHA256 = 'acab8014abe6fe624b697c25bda4ddcdf2d9ac4be1b90a50a67f950e1a851adb'
# Force only entry into the controller block. Preserve the real focus HWND for
# the keyboard cleanup and viewport tick that follow the controller block.
PATCHES = ((0x19AFCD, bytes.fromhex('75'), bytes.fromhex('eb')),
           (0x19AFF0, bytes.fromhex('0f85210d0000'), bytes.fromhex('909090909090')))
k = c.WinDLL('kernel32', use_last_error=True)
k.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]; k.OpenProcess.restype = w.HANDLE
k.CloseHandle.argtypes = [w.HANDLE]
k.CreateToolhelp32Snapshot.argtypes = [w.DWORD, w.DWORD]; k.CreateToolhelp32Snapshot.restype = w.HANDLE
k.OpenThread.argtypes = [w.DWORD, w.BOOL, w.DWORD]; k.OpenThread.restype = w.HANDLE
k.SuspendThread.argtypes = [w.HANDLE]; k.SuspendThread.restype = w.DWORD
k.ResumeThread.argtypes = [w.HANDLE]; k.ResumeThread.restype = w.DWORD
k.GetThreadContext.argtypes = [w.HANDLE, c.c_void_p]; k.GetThreadContext.restype = w.BOOL
k.VirtualProtectEx.argtypes = [w.HANDLE, c.c_void_p, c.c_size_t, w.DWORD, c.POINTER(w.DWORD)]
k.VirtualProtectEx.restype = w.BOOL
k.WriteProcessMemory.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
k.WriteProcessMemory.restype = w.BOOL
k.FlushInstructionCache.argtypes = [w.HANDLE, c.c_void_p, c.c_size_t]; k.FlushInstructionCache.restype = w.BOOL

class ThreadEntry(c.Structure):
    _fields_ = [('size', w.DWORD), ('usage', w.DWORD), ('tid', w.DWORD),
                ('owner', w.DWORD), ('base', w.LONG), ('delta', w.LONG), ('flags', w.DWORD)]
k.Thread32First.argtypes = [w.HANDLE, c.POINTER(ThreadEntry)]; k.Thread32First.restype = w.BOOL
k.Thread32Next.argtypes = [w.HANDLE, c.POINTER(ThreadEntry)]; k.Thread32Next.restype = w.BOOL

def digest(raw): return hashlib.sha256(raw).hexdigest()

class CodeNotReady(RuntimeError): pass

def classify(raw):
    if len(raw) != FUNCTION_SIZE: raise CodeNotReady('Input function is unreadable')
    if digest(raw) == FUNCTION_SHA256: return 'original'
    original = bytearray(raw)
    for rva, before, after in PATCHES:
        off = rva - FUNCTION_RVA
        if raw[off:off + len(after)] != after: raise CodeNotReady('Unknown input code; refusing patch')
        original[off:off + len(before)] = before
    if digest(original) != FUNCTION_SHA256: raise CodeNotReady('Input function hash differs from certified snapshot')
    return 'enabled'

def thread_ids(pid):
    snapshot = k.CreateToolhelp32Snapshot(4, 0)
    if snapshot == c.c_void_p(-1).value: raise c.WinError(c.get_last_error())
    try:
        row = ThreadEntry(); row.size = c.sizeof(row); found = set()
        ok = k.Thread32First(snapshot, c.byref(row))
        while ok:
            if row.owner == pid: found.add(row.tid)
            ok = k.Thread32Next(snapshot, c.byref(row))
        if c.get_last_error() != 18: raise c.WinError(c.get_last_error())
        if not found: raise RuntimeError('No live game threads')
        return found
    finally: k.CloseHandle(snapshot)

class FrozenThreads:
    def __init__(self, pid, base): self.pid, self.base, self.handles = pid, base, []
    def __enter__(self):
        try:
            seen = set()
            for _ in range(4):
                current = thread_ids(self.pid)
                for tid in sorted(current - seen):
                    handle = k.OpenThread(0x001A, False, tid) # suspend, context, query
                    if not handle: raise c.WinError(c.get_last_error())
                    if k.SuspendThread(handle) == 0xffffffff:
                        k.CloseHandle(handle); raise c.WinError(c.get_last_error())
                    self.handles.append(handle); seen.add(tid)
                    storage = c.create_string_buffer(1232 + 15)
                    address = (c.addressof(storage) + 15) & ~15
                    c.c_ulong.from_address(address + 48).value = 0x100001 # AMD64 CONTROL
                    if not k.GetThreadContext(handle, address): raise c.WinError(c.get_last_error())
                    rip = c.c_ulonglong.from_address(address + 248).value
                    if any(self.base + rva <= rip < self.base + rva + len(after) for rva, _, after in PATCHES):
                        raise RuntimeError('Thread at patch boundary; retry after it advances')
                if thread_ids(self.pid) == seen: return self
            raise RuntimeError('Game thread set did not stabilize')
        except BaseException:
            self.__exit__(None, None, None); raise
    def __exit__(self, *_):
        failed = []
        for handle in reversed(self.handles):
            if k.ResumeThread(handle) == 0xffffffff: failed.append(c.get_last_error())
            k.CloseHandle(handle)
        self.handles.clear()
        if failed: raise RuntimeError('Failed to resume game thread: ' + str(failed))

def write_code(handle, address, raw):
    old = w.DWORD(); restored = w.DWORD()
    if not k.VirtualProtectEx(handle, address, len(raw), 0x40, c.byref(old)):
        raise c.WinError(c.get_last_error())
    try:
        count = c.c_size_t(); buf = c.create_string_buffer(raw)
        if not k.WriteProcessMemory(handle, address, buf, len(raw), c.byref(count)) or count.value != len(raw):
            raise c.WinError(c.get_last_error())
        if not k.FlushInstructionCache(handle, address, len(raw)): raise c.WinError(c.get_last_error())
    finally:
        if not k.VirtualProtectEx(handle, address, len(raw), old.value, c.byref(restored)):
            raise c.WinError(c.get_last_error())

def atomic_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_suffix('.tmp'); temp.write_text(json.dumps(value, indent=2), encoding='utf-8'); temp.replace(path)

def change(pid, action, expected_creation=None):
    control = WindowsControl(); identity = control.identity(pid)
    if not identity: raise RuntimeError('Game exited')
    if expected_creation is not None and identity['creation_time'] != expected_creation:
        raise RuntimeError('Process creation identity changed; refusing reused PID')
    path = pathlib.Path(identity['image'])
    profile = json.loads((ROOT / 'tools/package-inspector/reloaded-profile.json').read_text())
    if path.name.lower() != 'gowde-steam.exe' or digest(path.read_bytes()) != profile['sha256']:
        raise RuntimeError('Unsupported executable')
    p = Process(pid)
    handle = None
    try:
        loaded = modules(p.handle); base, loaded_path = loaded['gowde-steam.exe']
        if loaded_path.resolve() != path.resolve(): raise RuntimeError('Module path mismatch')
        raw = p.read(base + FUNCTION_RVA, FUNCTION_SIZE); state = classify(raw)
        ledger = ROOT / f'analysis/runtime/background-input-{pid}-{identity["creation_time"]}.json'
        report = dict(schema=1, identity=identity, module_base=hex(base), executable_sha256=profile['sha256'],
                      input_original_sha256=FUNCTION_SHA256, state=state, action=action,
                      patches=[dict(rva=hex(r), original=b.hex(), enabled=a.hex()) for r,b,a in PATCHES])
        if action == 'status': return report
        desired = 'enabled' if action == 'enable' else 'original'
        owned = json.loads(ledger.read_text()) if ledger.exists() else None
        if state == 'enabled' and (not owned or owned['identity'] != identity or owned.get('state') != 'enabled'):
            raise RuntimeError('Enabled bytes have no matching ownership ledger; refusing to adopt or restore')
        if state == desired: return report
        handle = k.OpenProcess(0x438, False, pid)
        if not handle: raise c.WinError(c.get_last_error())
        with FrozenThreads(pid, base) as frozen:
            if control.identity(pid) != identity or p.read(base + FUNCTION_RVA, FUNCTION_SIZE) != raw:
                raise RuntimeError('Process identity or code changed before patch')
            applied = []
            try:
                # A pending ledger makes interrupted transactions visible.
                atomic_json(ledger, dict(report, state='pending', previous_state=state))
                for rva, before, after in PATCHES:
                    applied.append((rva, before if action == 'enable' else after))
                    write_code(handle, base + rva, after if action == 'enable' else before)
                if classify(p.read(base + FUNCTION_RVA, FUNCTION_SIZE)) != desired:
                    raise RuntimeError('Patch readback rejected')
                report.update(state=desired, verified=True, time_unix=time.time(), suspended_threads=len(frozen.handles))
                atomic_json(ledger, report)
            except BaseException as exc:
                for rva, prior in reversed(applied): write_code(handle, base + rva, prior)
                if p.read(base + FUNCTION_RVA, FUNCTION_SIZE) != raw: raise RuntimeError('Patch rollback failed')
                atomic_json(ledger, dict(report, state=state, rolled_back=True))
                raise RuntimeError('Input patch transaction rolled back: ' + str(exc)) from exc
        report['ledger'] = str(ledger)
        return report
    finally:
        if handle: k.CloseHandle(handle)
        p.close()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['enable', 'disable', 'status'])
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--wait-seconds', type=float, default=0)
    parser.add_argument('--creation-time', type=int, help='Bind automatic startup to the original process FILETIME')
    args = parser.parse_args()
    if not 0 <= args.wait_seconds <= 120: parser.error('wait-seconds must be between 0 and 120')
    deadline = time.monotonic() + args.wait_seconds
    while True:
        try:
            print(json.dumps(change(args.pid, args.action, args.creation_time))); return
        except (CodeNotReady, KeyError) as exc:
            if time.monotonic() >= deadline:
                print(json.dumps({'pid':args.pid, 'action':args.action, 'error':str(exc)}), file=sys.stderr); raise SystemExit(1)
            time.sleep(.2)
        except (RuntimeError, OSError) as exc:
            print(json.dumps({'pid':args.pid, 'action':args.action, 'error':str(exc)}),file=sys.stderr)
            raise SystemExit(1)

if __name__ == '__main__': main()
