"""Request the one-shot copy from an already armed observer in a verified game.

No injection, game input or memory writes. The observer handles the request at
the next matching Present; a signaled event alone does not prove a saved frame.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
from start_capture import modules, ROOT
from runtime_probe import Process


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pid', type=int, required=True)
    args = parser.parse_args()
    process = Process(args.pid)
    try:
        loaded = modules(process.handle)
        expected = ROOT / 'game-installation/Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe'
        game = loaded.get('gowde-steam.exe')
        if not game or game[1].resolve() != expected.resolve() or 'gearsgpubinding.dll' not in loaded:
            raise RuntimeError('Expected game and observer are not resident')
        kernel = c.WinDLL('kernel32', use_last_error=True)
        kernel.OpenEventW.argtypes = [w.DWORD, w.BOOL, w.LPCWSTR]
        kernel.OpenEventW.restype = w.HANDLE
        kernel.SetEvent.argtypes = [w.HANDLE]
        kernel.SetEvent.restype = w.BOOL
        kernel.CloseHandle.argtypes = [w.HANDLE]
        handle = kernel.OpenEventW(2, False, f'Local\\GearsGpuReadback-{args.pid}')
        if not handle:
            raise c.WinError(c.get_last_error())
        try:
            if not kernel.SetEvent(handle):
                raise c.WinError(c.get_last_error())
        finally:
            kernel.CloseHandle(handle)
    finally:
        process.close()
    print(json.dumps(dict(pid=args.pid, requested=True, saved='verify observer log')))


if __name__ == '__main__':
    main()
