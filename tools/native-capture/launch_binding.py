"""Launch a fresh game and observe queue/swapchain creation as early as possible.

Uses existing version-gated LoadLibrary injection helpers. No camera mutations,
no global input, and no replacement/unload of a resident DLL. A missed creation
is reported as inconclusive, never inferred from device identity.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
import pathlib
import subprocess
import time
from start_capture import k, modules, remote_call, pefile, ROOT


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--seconds', type=int, default=60, choices=range(10, 181))
    parser.add_argument('--readback', action='store_true', help='One asynchronous GPU screenshot after 15 seconds')
    parser.add_argument('--manual-readback', action='store_true', help='Wait for request_readback.py instead of taking a startup screenshot')
    parser.add_argument('--gpu-share', action='store_true', help='Publish bounded continuous frames to one same-adapter GPU consumer')
    parser.add_argument('--build-dir', type=pathlib.Path, default=pathlib.Path(__file__).with_name('build-binding'))
    args = parser.parse_args()
    if args.manual_readback:
        args.readback = True
    if args.gpu_share and args.readback:
        parser.error('GPU sharing and one-shot readback are separate tests')
    build = args.build_dir.resolve()
    dll = build / 'GearsGpuBinding.dll'
    subprocess.run([str(build / 'BindingSelfTest.exe'), str(dll)], check=True)
    if args.readback:
        subprocess.run([str(build / 'BindingSelfTest.exe'), str(dll), '--readback'], check=True)
    if args.gpu_share:
        subprocess.run([str(build / 'GpuShareTest.exe')], check=True)
    closed = subprocess.run(['powershell', '-NoProfile', '-NonInteractive', '-Command',
        "if (Get-Process -Name 'GOWDE-Steam' -ErrorAction SilentlyContinue) {exit 1} else {exit 0}"], capture_output=True)
    if closed.returncode:
        raise RuntimeError('Exit the existing game first; launcher does not kill it')
    game = ROOT / 'game-installation/Gears of War Reloaded'
    exe = game / 'Binaries_x64/GOWDE-Steam.exe'
    profile = json.loads((ROOT / 'tools/package-inspector/reloaded-profile.json').read_text())
    if hashlib.sha256(exe.read_bytes()).hexdigest() != profile['sha256']:
        raise RuntimeError('Unvalidated game version')
    pe = pefile.PE(str(dll))
    export = next(e for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name == b'GearsBindingStart')
    if export.forwarder:
        raise RuntimeError('Forwarded observer entry rejected')
    export_rva = export.address
    pe.close()
    if args.readback and args.seconds<20:
        raise RuntimeError('Readback requires at least 20 seconds')
    (build / 'binding.ini').write_text(f'[Binding]\nseconds={args.seconds}\nreadback={int(args.readback)}\nmanual={int(args.manual_readback)}\nshare={int(args.gpu_share)}\n', encoding='utf-16')
    subprocess.run(['powershell', '-NoProfile', '-NonInteractive', '-File',
                    str(ROOT / 'tools/debug-launch/Start-DebugGame.ps1')], check=True)
    # Locate the fresh process by image name. The existing launcher is kept as
    # the one source of startup arguments and reversible logo/tuneables config.
    deadline = time.monotonic() + 10
    handle = None
    while time.monotonic() < deadline:
        found = subprocess.run(['powershell', '-NoProfile', '-NonInteractive', '-Command',
            "Get-Process -Name 'GOWDE-Steam' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Id"],
            capture_output=True, text=True)
        pids = [int(line) for line in found.stdout.splitlines() if line.strip().isdigit()]
        if len(pids) > 1:
            raise RuntimeError('Ambiguous fresh game processes; do not inject')
        if pids:
            pid = pids[0]
            handle = k.OpenProcess(0x43a, False, pid)
            if handle:
                try:
                    loaded = modules(handle)
                    # The first Windows loader snapshot can precede the game's
                    # graphics dependencies. Loading our graphics-linked DLL
                    # then failed in the observed build; wait for these modules
                    # without requiring a rendered window or entering a level.
                    if all(name in loaded for name in ('kernelbase.dll', 'gowde-steam.exe', 'dxgi.dll', 'd3d12.dll')):
                        break
                except OSError:
                    pass
                k.CloseHandle(handle)
                handle = None
        time.sleep(.05)
    if not handle:
        raise RuntimeError('Fresh game loader modules not ready')
    remote_memory = None
    loader_finished = False
    try:
        if pathlib.Path(loaded['gowde-steam.exe'][1]).resolve() != exe.resolve():
            raise RuntimeError('Fresh process executable path differs')
        if dll.name.lower() in loaded:
            raise RuntimeError('Observer already resident')
        local_loader = k.GetProcAddress(k.GetModuleHandleW('kernel32.dll'), b'LoadLibraryW')
        owner = w.HMODULE()
        if not k.GetModuleHandleExW(6, local_loader, c.byref(owner)):
            raise c.WinError(c.get_last_error())
        owner_path = c.create_unicode_buffer(32768)
        k.GetModuleFileNameW(owner, owner_path, len(owner_path))
        remote_loader = loaded[pathlib.Path(owner_path.value).name.lower()][0] + local_loader - owner.value
        encoded = (str(dll) + '\0').encode('utf-16-le')
        remote_memory = k.VirtualAllocEx(handle, None, len(encoded), 0x3000, 0x04)
        if not remote_memory:
            raise c.WinError(c.get_last_error())
        copied = c.c_size_t()
        buffer = c.create_string_buffer(encoded)
        if not k.WriteProcessMemory(handle, remote_memory, buffer, len(encoded), c.byref(copied)) or copied.value != len(encoded):
            raise c.WinError(c.get_last_error())
        loader_result = remote_call(handle, remote_loader, remote_memory)
        loader_finished = True
        observer = modules(handle).get(dll.name.lower())
        if not observer or observer[1].resolve() != dll:
            raise RuntimeError(f'Observer load not confirmed; LoadLibrary thread result={loader_result:#x}; do not retry blindly')
        if remote_call(handle, observer[0] + export_rva, None):
            raise RuntimeError('Observer start failed')
        output = build / f'binding-{pid}.jsonl'
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            if output.exists() and output.read_text().strip():
                ready = json.loads(output.read_text().splitlines()[0])
                if not ready.get('ok'):
                    raise RuntimeError('Observer hooks not ready; inspect output')
                print(json.dumps(dict(pid=pid, output=str(output), ready=ready, seconds=args.seconds)))
                return
            time.sleep(.05)
        raise RuntimeError('Observer readiness not confirmed')
    finally:
        if remote_memory and loader_finished:
            k.VirtualFreeEx(handle, remote_memory, 0, 0x8000)
        k.CloseHandle(handle)


if __name__ == '__main__':
    main()
