#pragma once
#include <Windows.h>

namespace gears_quiet_test {
// Pure policy only. Never minimize, resize, move, or acquire world input.
inline int show_command(int command, bool behind) noexcept {
    switch(command) {
    case SW_SHOWNORMAL:case SW_SHOWMAXIMIZED:case SW_SHOW:case SW_RESTORE:case SW_SHOWDEFAULT:
        return behind?SW_SHOWNA:SW_SHOWNOACTIVATE;
    case SW_SHOWNOACTIVATE:return behind?SW_SHOWNA:command;
    case SW_SHOWMINIMIZED:case SW_MINIMIZE:return SW_SHOWMINNOACTIVE;
    default:return command;
    }
}
inline UINT position_flags(UINT flags, bool behind) noexcept {
    return behind?(flags|SWP_NOACTIVATE|SWP_NOOWNERZORDER)&~SWP_NOZORDER:
        flags|SWP_NOACTIVATE|SWP_NOZORDER;
}
inline HWND insert_after(HWND previous, bool behind) noexcept {return behind?HWND_BOTTOM:previous;}
inline int startup_show(bool minimized, bool behind) noexcept {
    return minimized?SW_SHOWMINNOACTIVE:behind?SW_SHOWNA:SW_SHOWNOACTIVATE;
}
// A first show must fail hidden if placing the new window below others failed.
template<class Bottom,class Show> bool first_show(bool behind,Bottom bottom,Show show) {
    if(behind&&!bottom())return false;
    show(behind?SW_SHOWNA:SW_SHOWNOACTIVATE);return true;
}
}
