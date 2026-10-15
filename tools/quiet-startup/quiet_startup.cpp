// Process-local automatic activation policy. No desktop/global focus changes.
#include "quiet_startup.h"
#include <MinHook.h>
#include <array>
#include <atomic>

namespace {
std::atomic<bool> enabled{};
std::array<std::atomic<std::uint64_t>,gears_quiet::count> suppressed{};
std::atomic<bool> trace_enabled{},trace_overflow{};
std::atomic<unsigned> trace_next{};
struct TraceSlot {std::atomic<bool> complete{};gears_quiet::TraceRecord record{};};
std::array<TraceSlot,gears_quiet::trace_capacity> trace_slots{};
struct TraceCall {
    TraceSlot* slot{};
    TraceCall(unsigned call,HWND target,std::uint64_t argument=0,HWND related=nullptr,std::uint64_t extra=0) noexcept {
        if(!trace_enabled.load(std::memory_order_relaxed))return;
        const DWORD error=GetLastError();
        auto index=trace_next.load(std::memory_order_relaxed);
        while(index<gears_quiet::trace_capacity && !trace_next.compare_exchange_weak(index,index+1,std::memory_order_relaxed)){}
        if(index>=gears_quiet::trace_capacity)trace_overflow.store(true,std::memory_order_relaxed);
        else {
            slot=&trace_slots[index];auto& r=slot->record;
            r.sequence=index+1;r.call=call;r.thread=GetCurrentThreadId();
            r.target=reinterpret_cast<uintptr_t>(target);r.related=reinterpret_cast<uintptr_t>(related);
            r.argument=r.effective_argument=argument;r.extra_argument=extra;
            r.enter_tick=GetTickCount64();r.foreground_before=reinterpret_cast<uintptr_t>(GetForegroundWindow());
            void* stack[gears_quiet::trace_stack_capacity]{};
            r.stack_count=CaptureStackBackTrace(1,gears_quiet::trace_stack_capacity,stack,nullptr);
            for(unsigned i=0;i<r.stack_count;++i)r.stack[i]=reinterpret_cast<uintptr_t>(stack[i]);
        }
        SetLastError(error);
    }
    void policy(std::uint64_t effective) noexcept {if(slot){slot->record.suppressed=1;slot->record.effective_argument=effective;}}
    void created(HWND window) noexcept {if(slot)slot->record.result_window=reinterpret_cast<uintptr_t>(window);}
    ~TraceCall() noexcept {
        if(!slot)return;
        const DWORD error=GetLastError();auto& r=slot->record;
        r.leave_tick=GetTickCount64();r.foreground_after=reinterpret_cast<uintptr_t>(GetForegroundWindow());
        slot->complete.store(true,std::memory_order_release);SetLastError(error);
    }
};
decltype(&ShowWindow) original_show{};
decltype(&SetForegroundWindow) original_foreground{};
decltype(&SetActiveWindow) original_active{};
decltype(&SetFocus) original_focus{};
decltype(&SetWindowPos) original_position{};
decltype(&BringWindowToTop) original_bring{};
decltype(&CreateWindowExW) original_create_w{};
decltype(&CreateWindowExA) original_create_a{};
decltype(&DefWindowProcW) original_def_window_w{};
decltype(&DefWindowProcA) original_def_window_a{};

bool trace_window_message(UINT message) noexcept {
    switch(message){
    case WM_SYSCOMMAND:case WM_ACTIVATE:case WM_SETFOCUS:case WM_MOUSEACTIVATE:case WM_NCACTIVATE:return true;
    default:return false;
    }
}
LRESULT WINAPI def_window_w(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
    if(!trace_window_message(message))return original_def_window_w(window,message,wparam,lparam);
    TraceCall trace(gears_quiet::def_window_w,window,message,reinterpret_cast<HWND>(wparam),static_cast<std::uint64_t>(lparam));
    return original_def_window_w(window,message,wparam,lparam);
}
LRESULT WINAPI def_window_a(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
    if(!trace_window_message(message))return original_def_window_a(window,message,wparam,lparam);
    TraceCall trace(gears_quiet::def_window_a,window,message,reinterpret_cast<HWND>(wparam),static_cast<std::uint64_t>(lparam));
    return original_def_window_a(window,message,wparam,lparam);
}

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
    TraceCall trace(gears_quiet::show,window,static_cast<DWORD>(command));
    if(guard(window)) {
        switch(command) {
        case SW_HIDE: {
            // ShowWindow(SW_HIDE) may activate another top-level window even
            // when this application was in the background. Hide without a
            // z-order/activation operation, preserving ShowWindow's result.
            const BOOL was_visible=IsWindowVisible(window);
            original_position(window,nullptr,0,0,0,0,
                SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE);
            trace.policy(SW_HIDE);note(gears_quiet::show);return was_visible;
        }
        case SW_SHOWNORMAL: case SW_SHOWMAXIMIZED: case SW_SHOW:
        case SW_RESTORE: case SW_SHOWDEFAULT:
            command=SW_SHOWNOACTIVATE;trace.policy(command);note(gears_quiet::show);break;
        case SW_SHOWMINIMIZED: case SW_MINIMIZE:
            command=SW_SHOWMINNOACTIVE;trace.policy(command);note(gears_quiet::show);break;
        default:break;
        }
    }
    return original_show(window,command);
}
BOOL WINAPI foreground(HWND window) {
    TraceCall trace(gears_quiet::foreground,window);
    if(guard(window,false)) {trace.policy(0);note(gears_quiet::foreground);return FALSE;}
    return original_foreground(window);
}
HWND WINAPI active(HWND window) {
    TraceCall trace(gears_quiet::active,window);
    if(guard(window,false)) {trace.policy(0);note(gears_quiet::active);return GetActiveWindow();}
    return original_active(window);
}
HWND WINAPI focus(HWND window) {
    TraceCall trace(gears_quiet::focus,window);
    if(guard(window,false)) {trace.policy(0);note(gears_quiet::focus);return GetFocus();}
    return original_focus(window);
}
BOOL WINAPI position(HWND window,HWND after,int x,int y,int width,int height,UINT flags) {
    TraceCall trace(gears_quiet::position,window,flags,after);
    if(guard(window)) {
        flags|=SWP_NOACTIVATE|SWP_NOZORDER;
        trace.policy(flags);
        note(gears_quiet::position);
    }
    return original_position(window,after,x,y,width,height,flags);
}
BOOL WINAPI bring(HWND window) {
    TraceCall trace(gears_quiet::bring,window);
    if(guard(window)) {trace.policy(0);note(gears_quiet::bring);return TRUE;}
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
    TraceCall trace(gears_quiet::create_w,nullptr,style,parent,exstyle);
    const bool guarded=background() && !(style&WS_CHILD) && parent!=HWND_MESSAGE;
    if(guarded)trace.policy(style&~WS_VISIBLE);
    if(guarded)note(gears_quiet::create_w);
    HWND result=original_create_w(exstyle,cls,title,guarded?style&~WS_VISIBLE:style,x,y,width,height,parent,menu,instance,argument);
    DWORD error=GetLastError();finish_create(result,style,guarded);trace.created(result);SetLastError(error);return result;
}
HWND WINAPI create_a(DWORD exstyle,LPCSTR cls,LPCSTR title,DWORD style,int x,int y,int width,int height,
                     HWND parent,HMENU menu,HINSTANCE instance,LPVOID argument) {
    TraceCall trace(gears_quiet::create_a,nullptr,style,parent,exstyle);
    const bool guarded=background() && !(style&WS_CHILD) && parent!=HWND_MESSAGE;
    if(guarded)trace.policy(style&~WS_VISIBLE);
    if(guarded)note(gears_quiet::create_a);
    HWND result=original_create_a(exstyle,cls,title,guarded?style&~WS_VISIBLE:style,x,y,width,height,parent,menu,instance,argument);
    DWORD error=GetLastError();finish_create(result,style,guarded);trace.created(result);SetLastError(error);return result;
}
}

extern "C" __declspec(dllexport) DWORD WINAPI GearsQuietInstall(void*) {
    if(enabled.load())return ERROR_ALREADY_EXISTS;
    wchar_t trace_option[2]{};
    const bool tracing=GetEnvironmentVariableW(L"GEARSVR_QUIET_TRACE",trace_option,2)==1&&trace_option[0]==L'1';
    if(MH_Initialize()!=MH_OK)return ERROR_INVALID_FUNCTION;
    const std::array<void*,gears_quiet::trace_call_count> targets{
        reinterpret_cast<void*>(&ShowWindow),reinterpret_cast<void*>(&SetForegroundWindow),
        reinterpret_cast<void*>(&SetActiveWindow),reinterpret_cast<void*>(&SetFocus),
        reinterpret_cast<void*>(&SetWindowPos),reinterpret_cast<void*>(&BringWindowToTop),
        reinterpret_cast<void*>(&CreateWindowExW),reinterpret_cast<void*>(&CreateWindowExA),
        reinterpret_cast<void*>(&DefWindowProcW),reinterpret_cast<void*>(&DefWindowProcA)};
    const std::array<void*,gears_quiet::trace_call_count> replacements{
        reinterpret_cast<void*>(&show),reinterpret_cast<void*>(&foreground),
        reinterpret_cast<void*>(&active),reinterpret_cast<void*>(&focus),
        reinterpret_cast<void*>(&position),reinterpret_cast<void*>(&bring),
        reinterpret_cast<void*>(&create_w),reinterpret_cast<void*>(&create_a),
        reinterpret_cast<void*>(&def_window_w),reinterpret_cast<void*>(&def_window_a)};
    const std::array<void**,gears_quiet::trace_call_count> originals{
        reinterpret_cast<void**>(&original_show),reinterpret_cast<void**>(&original_foreground),
        reinterpret_cast<void**>(&original_active),reinterpret_cast<void**>(&original_focus),
        reinterpret_cast<void**>(&original_position),reinterpret_cast<void**>(&original_bring),
        reinterpret_cast<void**>(&original_create_w),reinterpret_cast<void**>(&original_create_a),
        reinterpret_cast<void**>(&original_def_window_w),reinterpret_cast<void**>(&original_def_window_a)};
    const unsigned wanted=tracing?gears_quiet::trace_call_count:gears_quiet::count;
    unsigned created{};
    for(;created<wanted;++created) {
        if(MH_CreateHook(targets[created],replacements[created],originals[created])!=MH_OK)break;
    }
    if(created!=wanted) {
        for(unsigned i=0;i<created;++i)MH_RemoveHook(targets[i]);
        MH_Uninitialize();return created>=gears_quiet::count?ERROR_NOT_SUPPORTED:ERROR_INVALID_FUNCTION;
    }
    // Entry thread is still held by the launcher. Publish policy before enabling.
    trace_enabled.store(tracing,std::memory_order_relaxed);
    enabled.store(true,std::memory_order_release);
    if(MH_EnableHook(MH_ALL_HOOKS)!=MH_OK) {
        enabled.store(false,std::memory_order_release);
        trace_enabled.store(false,std::memory_order_relaxed);
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
extern "C" __declspec(dllexport) DWORD WINAPI GearsQuietTrace(void* output) {
    if(!output)return ERROR_INVALID_PARAMETER;
    auto* out=static_cast<gears_quiet::TraceSnapshot*>(output);
    // Output storage belongs to the caller; no tracing of this read operation.
    *out=gears_quiet::TraceSnapshot{};
    out->enabled=trace_enabled.load(std::memory_order_relaxed);
    out->reserved_count=trace_next.load(std::memory_order_relaxed);
    out->overflow=trace_overflow.load(std::memory_order_relaxed);
    for(unsigned i=0;i<out->reserved_count;++i){
        if(trace_slots[i].complete.load(std::memory_order_acquire))out->records[out->completed++]=trace_slots[i].record;
        else ++out->busy_skipped;
    }
    return ERROR_SUCCESS;
}
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
