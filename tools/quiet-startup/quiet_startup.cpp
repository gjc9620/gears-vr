// Process-local automatic activation policy. No desktop/global focus changes.
#include "quiet_startup.h"
#include <MinHook.h>
#include <array>
#include <atomic>

namespace {
std::atomic<bool> enabled{};
std::array<std::atomic<std::uint64_t>,gears_quiet::count> suppressed{};
decltype(&ShowWindow) original_show{};
decltype(&SetForegroundWindow) original_foreground{};
decltype(&SetActiveWindow) original_active{};
decltype(&SetFocus) original_focus{};
decltype(&SetWindowPos) original_position{};
decltype(&BringWindowToTop) original_bring{};
decltype(&CreateWindowExW) original_create_w{};
decltype(&CreateWindowExA) original_create_a{};

bool own(HWND window) {
    DWORD pid{};
    return window && GetWindowThreadProcessId(window,&pid) && pid==GetCurrentProcessId();
}
bool background() { return enabled.load(std::memory_order_acquire) && !own(GetForegroundWindow()); }
bool guard(HWND window,bool top_level=true) {
    return background() && own(window) && (!top_level || !(GetWindowLongPtrW(window,GWL_STYLE)&WS_CHILD));
}
void note(gears_quiet::Call call) { suppressed[call].fetch_add(1,std::memory_order_relaxed); }
BOOL WINAPI show(HWND window,int command) {
    if(guard(window)) {
        switch(command) {
        case SW_HIDE: {
            // ShowWindow(SW_HIDE) may activate another top-level window even
            // when this application was in the background. Hide without a
            // z-order/activation operation, preserving ShowWindow's result.
            const BOOL was_visible=IsWindowVisible(window);
            original_position(window,nullptr,0,0,0,0,
                SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE);
            note(gears_quiet::show);return was_visible;
        }
        case SW_SHOWNORMAL: case SW_SHOWMAXIMIZED: case SW_SHOW:
        case SW_RESTORE: case SW_SHOWDEFAULT:
            command=SW_SHOWNOACTIVATE;note(gears_quiet::show);break;
        case SW_SHOWMINIMIZED: case SW_MINIMIZE:
            command=SW_SHOWMINNOACTIVE;note(gears_quiet::show);break;
        default:break;
        }
    }
    return original_show(window,command);
}
BOOL WINAPI foreground(HWND window) {
    if(guard(window,false)) {note(gears_quiet::foreground);return FALSE;}
    return original_foreground(window);
}
HWND WINAPI active(HWND window) {
    if(guard(window,false)) {note(gears_quiet::active);return GetActiveWindow();}
    return original_active(window);
}
HWND WINAPI focus(HWND window) {
    if(guard(window,false)) {note(gears_quiet::focus);return GetFocus();}
    return original_focus(window);
}
BOOL WINAPI position(HWND window,HWND after,int x,int y,int width,int height,UINT flags) {
    if(guard(window)) {
        flags|=SWP_NOACTIVATE|SWP_NOZORDER;
        note(gears_quiet::position);
    }
    return original_position(window,after,x,y,width,height,flags);
}
BOOL WINAPI bring(HWND window) {
    if(guard(window)) {note(gears_quiet::bring);return TRUE;}
    return original_bring(window);
}
// WS_EX_NOACTIVATE alone still changes the creating thread's active window.
// Create hidden, then show without activation. Keep the caller's extended
// styles unchanged, including any explicitly requested NOACTIVATE style.
void finish_create(HWND window,DWORD style,bool guarded) {
    if(window && guarded && (style&WS_VISIBLE))original_show(window,SW_SHOWNOACTIVATE);
}
HWND WINAPI create_w(DWORD exstyle,LPCWSTR cls,LPCWSTR title,DWORD style,int x,int y,int width,int height,
                     HWND parent,HMENU menu,HINSTANCE instance,LPVOID argument) {
    const bool guarded=background() && !(style&WS_CHILD) && parent!=HWND_MESSAGE;
    if(guarded)note(gears_quiet::create_w);
    HWND result=original_create_w(exstyle,cls,title,guarded?style&~WS_VISIBLE:style,x,y,width,height,parent,menu,instance,argument);
    DWORD error=GetLastError();finish_create(result,style,guarded);SetLastError(error);return result;
}
HWND WINAPI create_a(DWORD exstyle,LPCSTR cls,LPCSTR title,DWORD style,int x,int y,int width,int height,
                     HWND parent,HMENU menu,HINSTANCE instance,LPVOID argument) {
    const bool guarded=background() && !(style&WS_CHILD) && parent!=HWND_MESSAGE;
    if(guarded)note(gears_quiet::create_a);
    HWND result=original_create_a(exstyle,cls,title,guarded?style&~WS_VISIBLE:style,x,y,width,height,parent,menu,instance,argument);
    DWORD error=GetLastError();finish_create(result,style,guarded);SetLastError(error);return result;
}
}

extern "C" __declspec(dllexport) DWORD WINAPI GearsQuietInstall(void*) {
    if(enabled.load())return ERROR_ALREADY_EXISTS;
    if(MH_Initialize()!=MH_OK)return ERROR_INVALID_FUNCTION;
    const std::array<void*,gears_quiet::count> targets{
        reinterpret_cast<void*>(&ShowWindow),reinterpret_cast<void*>(&SetForegroundWindow),
        reinterpret_cast<void*>(&SetActiveWindow),reinterpret_cast<void*>(&SetFocus),
        reinterpret_cast<void*>(&SetWindowPos),reinterpret_cast<void*>(&BringWindowToTop),
        reinterpret_cast<void*>(&CreateWindowExW),reinterpret_cast<void*>(&CreateWindowExA)};
    const std::array<void*,gears_quiet::count> replacements{
        reinterpret_cast<void*>(&show),reinterpret_cast<void*>(&foreground),
        reinterpret_cast<void*>(&active),reinterpret_cast<void*>(&focus),
        reinterpret_cast<void*>(&position),reinterpret_cast<void*>(&bring),
        reinterpret_cast<void*>(&create_w),reinterpret_cast<void*>(&create_a)};
    const std::array<void**,gears_quiet::count> originals{
        reinterpret_cast<void**>(&original_show),reinterpret_cast<void**>(&original_foreground),
        reinterpret_cast<void**>(&original_active),reinterpret_cast<void**>(&original_focus),
        reinterpret_cast<void**>(&original_position),reinterpret_cast<void**>(&original_bring),
        reinterpret_cast<void**>(&original_create_w),reinterpret_cast<void**>(&original_create_a)};
    unsigned created{};
    for(;created<targets.size();++created) {
        if(MH_CreateHook(targets[created],replacements[created],originals[created])!=MH_OK)break;
    }
    if(created!=targets.size()) {
        for(unsigned i=0;i<created;++i)MH_RemoveHook(targets[i]);
        MH_Uninitialize();return ERROR_INVALID_FUNCTION;
    }
    // Entry thread is still held by the launcher. Publish policy before enabling.
    enabled.store(true,std::memory_order_release);
    if(MH_EnableHook(MH_ALL_HOOKS)!=MH_OK) {
        enabled.store(false,std::memory_order_release);
        MH_DisableHook(MH_ALL_HOOKS);MH_Uninitialize();return ERROR_INVALID_FUNCTION;
    }
    return ERROR_SUCCESS;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsQuietStatus(gears_quiet::Status* out) {
    if(!out)return ERROR_INVALID_PARAMETER;
    gears_quiet::Status result;result.installed=enabled.load(std::memory_order_acquire);
    for(unsigned i=0;i<gears_quiet::count;++i)result.suppressed[i]=suppressed[i].load(std::memory_order_relaxed);
    *out=result;return ERROR_SUCCESS;
}
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
