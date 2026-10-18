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
inline UINT behind_show_flags(bool asynchronous=false) noexcept {
    return SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_SHOWWINDOW|
        (asynchronous?SWP_ASYNCWINDOWPOS:0);
}
// Display and bottom placement must be one operation: ShowWindow(SW_SHOWNA)
// after a separate bottom placement can raise the window without activating it.
template<class Bottom,class Show> bool first_show(bool behind,Bottom bottom,Show show) {
    if(behind)return bottom();
    show(SW_SHOWNOACTIVATE);return true;
}
}
