"""Send one Enter pair only to the verified game's HWND, never global input.

Use after observing a menu that expects confirmation. A queued Win32 message
is not proof the game consumed it; inspect the next frame before repeating.
"""
import ctypes as c
from ctypes import wintypes as w
import json
import time
from game_io import game_window, focused, interrupted


def main():
    pid, hwnd = game_window()
    if interrupted(): raise RuntimeError('Stop/F8 active; confirmation refused')
    user = c.WinDLL('user32', use_last_error=True)
    user.PostMessageW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM]
    user.PostMessageW.restype = w.BOOL
    if not user.PostMessageW(hwnd, 0x100, 13, 0x001c0001):
        raise c.WinError(c.get_last_error())
    try: time.sleep(.08)
    finally:
        if not user.PostMessageW(hwnd, 0x101, 13, 0xc01c0001):
            raise c.WinError(c.get_last_error())
    print(json.dumps({'pid':pid, 'hwnd':hwnd, 'focused':focused(hwnd),
        'enter_messages_queued':True, 'consumed_by_game':'verify next screenshot'}))


if __name__ == '__main__': main()
