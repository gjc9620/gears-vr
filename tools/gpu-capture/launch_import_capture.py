"""Fresh RenderDoc launch with a bounded, reversible ordinary import bridge.

The validated game resolves two ordinary graphics slots to raw Windows exports,
outside RenderDoc's import interception. During startup only, point those two
slots to exact RenderDoc wrappers measured in the same signed DLL build. Restore
both original slots within 20 seconds. Does not patch code or protected sections.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
import pathlib
import struct
import subprocess
import sys
import time
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/native-capture'))
from start_capture import k,modules,pefile
from runtime_probe import Process, MBI

k.VirtualProtectEx.argtypes=[w.HANDLE,c.c_void_p,c.c_size_t,w.DWORD,c.POINTER(w.DWORD)]
k.VirtualProtectEx.restype=w.BOOL
k.VirtualQueryEx.argtypes=[w.HANDLE,c.c_void_p,c.POINTER(MBI),c.c_size_t]
k.VirtualQueryEx.restype=c.c_size_t

def pids():
    result=subprocess.run(['powershell','-NoProfile','-NonInteractive','-Command',
        "Get-Process GOWDE-Steam -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Id"],
        capture_output=True,text=True,check=False)
    return [int(row) for row in result.stdout.splitlines() if row.strip().isdigit()]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest',type=pathlib.Path,required=True)
    args=parser.parse_args()
    if pids():raise RuntimeError('Existing game must exit normally first')
    game=ROOT/'game-installation/Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe'
    profile=json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
    if hashlib.sha256(game.read_bytes()).hexdigest()!=profile['sha256']:raise RuntimeError('Game hash differs')
    rd_dir=ROOT/'tools/third-party/renderdoc/RenderDoc_1.46_64';manifest=json.loads(args.manifest.read_text())
    if hashlib.sha256((rd_dir/'renderdoc.dll').read_bytes()).hexdigest()!=manifest['renderdoc_sha256']:
        raise RuntimeError('RenderDoc DLL hash differs from observed wrappers')
    entries={row['name']:row for row in manifest['entries']}
    if set(entries)!={'D3D12CreateDevice','CreateDXGIFactory1'}:raise RuntimeError('Unexpected wrapper manifest')
    for row in entries.values():
        if len(bytes.fromhex(row['entry_bytes']))!=32:raise RuntimeError('Wrapper entry length differs')
    (ROOT/'analysis/renderdoc').mkdir(parents=True,exist_ok=True)
    command=[str(rd_dir/'renderdoccmd.exe'),'capture','--opt-soft-memory-limit','1024',
        '-d',str(game.parent),'-c',str(ROOT/'analysis/renderdoc/game-bridged'),str(game),'-NoPauseUnfocus']
    launch=subprocess.run(command,capture_output=True,text=True)
    print(launch.stdout,flush=True)
    # renderdoccmd's capture return value is not a reliable success code; require
    # the actual process, executable path, modules, bytes and pointers below.
    report=dict(mode='temporary-startup-import-bridge',launch_stdout=launch.stdout,
                slots=[],restored=False,hold_seconds=20)
    handle=None;reader=None;records=[];protection=None;page=None
    deadline=time.monotonic()+20
    try:
        while time.monotonic()<deadline:
            current=pids()
            if len(current)>1:raise RuntimeError('Ambiguous new game')
            if current and handle is None:
                report['pid']=current[0];handle=k.OpenProcess(0x43a,False,current[0]);reader=Process(current[0])
                if not handle:raise c.WinError(c.get_last_error())
            if handle:
                loaded=modules(handle)
                if all(name in loaded for name in ('gowde-steam.exe','renderdoc.dll','d3d12.dll','dxgi.dll')):
                    base,path=loaded['gowde-steam.exe'];rd_base,rd_path=loaded['renderdoc.dll']
                    if path.resolve()!=game.resolve() or rd_path.resolve()!=(rd_dir/'renderdoc.dll').resolve():
                        raise RuntimeError('Loaded module path differs')
                    # Ordinary RIP-relative call sites measured in the validated executable.
                    checks={0x373dd7:0x1b43200,0x3742ef:0x1b43200,0x417dd7:0x1b43200,0x417e3e:0x1b43228}
                    ready=all(reader.read(base+rva,6)==b'\xff\x15'+struct.pack('<i',slot-rva-6)
                              for rva,slot in checks.items())
                    proposed=[]
                    for name,module,slot_rva in [('D3D12CreateDevice','d3d12.dll',0x1b43200),
                                                 ('CreateDXGIFactory1','dxgi.dll',0x1b43228)]:
                        module_base,module_path=loaded[module]
                        with pefile.PE(str(module_path)) as pe:
                            export=next(item for item in pe.DIRECTORY_ENTRY_EXPORT.symbols if item.name==name.encode())
                            if export.forwarder:raise RuntimeError('Unexpected forwarded original export')
                        original=module_base+export.address;row=entries[name];replacement=rd_base+int(row['rva'],16)
                        if reader.read(replacement,32)!=bytes.fromhex(row['entry_bytes']):
                            raise RuntimeError('Wrapper live entry differs')
                        ready=ready and reader.read(base+slot_rva,8)==struct.pack('<Q',original)
                        proposed.append(dict(name=name,address=base+slot_rva,original=original,replacement=replacement))
                    if ready:
                        records=proposed;page=records[0]['address']&~4095
                        if any(record['address']&~4095!=page for record in records):raise RuntimeError('Unexpected slot pages')
                        information=MBI()
                        if not k.VirtualQueryEx(handle,page,c.byref(information),c.sizeof(information)):
                            raise c.WinError(c.get_last_error())
                        protection=information.Protect
                        if protection not in (0x02,0x04):raise RuntimeError(f'Unexpected data page protection {protection:#x}')
                        old=w.DWORD()
                        if not k.VirtualProtectEx(handle,page,4096,0x04,c.byref(old)):raise c.WinError(c.get_last_error())
                        if old.value!=protection:raise RuntimeError('Data page protection changed concurrently')
                        for record in records:
                            if reader.read(record['address'],8)!=struct.pack('<Q',record['original']):
                                raise RuntimeError('Slot changed before bridge write')
                            buffer=c.create_string_buffer(struct.pack('<Q',record['replacement']));written=c.c_size_t()
                            if not k.WriteProcessMemory(handle,record['address'],buffer,8,c.byref(written)) or written.value!=8:
                                raise c.WinError(c.get_last_error())
                            report['slots'].append({key:hex(value) if isinstance(value,int) else value for key,value in record.items()})
                        restored_protection=w.DWORD()
                        if not k.VirtualProtectEx(handle,page,4096,protection,c.byref(restored_protection)):
                            raise c.WinError(c.get_last_error())
                        print(json.dumps(dict(pid=report['pid'],bridge_installed=True,slots=report['slots'])),flush=True)
                        time.sleep(20)
                        break
            time.sleep(.1)
        if not records:raise RuntimeError('Startup slot readiness not observed; no bridge installed')
    finally:
        try:
            if records and handle:
                old=w.DWORD()
                if not k.VirtualProtectEx(handle,page,4096,0x04,c.byref(old)):raise c.WinError(c.get_last_error())
                try:
                    for record in records:
                        live=reader.read(record['address'],8)
                        if live==struct.pack('<Q',record['replacement']):
                            buffer=c.create_string_buffer(struct.pack('<Q',record['original']));written=c.c_size_t()
                            if not k.WriteProcessMemory(handle,record['address'],buffer,8,c.byref(written)) or written.value!=8:
                                raise c.WinError(c.get_last_error())
                        elif live!=struct.pack('<Q',record['original']):raise RuntimeError('Slot changed unexpectedly; refusing overwrite')
                    report['restored']=all(reader.read(record['address'],8)==struct.pack('<Q',record['original']) for record in records)
                finally:
                    previous=w.DWORD()
                    if protection is not None and not k.VirtualProtectEx(handle,page,4096,protection,c.byref(previous)):
                        raise c.WinError(c.get_last_error())
        finally:
            (ROOT/f"analysis/renderdoc/import-bridge-{report.get('pid','unknown')}.json").write_text(json.dumps(report,indent=2),encoding='utf-8')
            if reader:reader.close()
            if handle:k.CloseHandle(handle)
    print(json.dumps(report,indent=2))

if __name__=='__main__':
    raise RuntimeError('Disabled experiment: PID 32376 crashed while creating its swapchain. See docs/52-renderdoc-import-bridge-failure.md; use the normal game launcher.')
