#pragma once
#include <windows.h>

// WGC ContentSize and DWM extended bounds are physical pixels. A runtime/layer
// may change this thread's DPI context after process initialization. Scope only
// our HWND measurements, restoring its context before any XR/runtime/UI work.
// No window creation, movement, resizing, activation or process-wide DPI change.
namespace physical_window_pixels {
struct User32 {
    using Context=DPI_AWARENESS_CONTEXT;
    static Context enter() noexcept {return SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);}
    static bool restore(Context old) noexcept {return SetThreadDpiAwarenessContext(old)!=nullptr;}
    static bool client(HWND window,RECT& out) noexcept {return GetClientRect(window,&out)!=FALSE;}
    static bool origin(HWND window,POINT& out) noexcept {return ClientToScreen(window,&out)!=FALSE;}
};
template<class Api=User32>
bool client(HWND window,RECT& out,POINT* screen_origin=nullptr) noexcept {
    const auto old=Api::enter();if(!old)return false;
    RECT rect{};POINT origin{};
    const bool read=Api::client(window,rect)&&(!screen_origin||Api::origin(window,origin));
    const bool restored=Api::restore(old);
    if(!read||!restored)return false;
    out=rect;if(screen_origin)*screen_origin=origin;return true;
}
}
