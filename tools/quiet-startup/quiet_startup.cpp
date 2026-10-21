// Process-local automatic activation policy. No desktop/global focus changes.
#include "quiet_startup.h"
#include "test_background_policy.h"
#include <MinHook.h>
#include "instance_policy.h"
#include <array>
#include <atomic>
#include <type_traits>

namespace {
std::atomic<bool> enabled{};
bool test_windows_behind{}; // Explicit automatic-test policy, never the default.
bool main_noactivate{}; // Written before hook publication; immutable thereafter.
std::array<std::atomic<std::uint64_t>,gears_quiet::count> suppressed{};
std::atomic<bool> trace_enabled{},trace_overflow{};
std::atomic<unsigned> trace_next{};
struct TraceSlot {std::atomic<bool> complete{};gears_quiet::TraceRecord2 record{};};
std::array<TraceSlot,gears_quiet::trace_capacity> trace_slots{};
std::uint64_t qpc_frequency{};
gears_quiet::StartupSnapshot startup{};
std::uint64_t qpc() noexcept {LARGE_INTEGER value{};QueryPerformanceCounter(&value);return value.QuadPart;}
gears_quiet::GuiThreadSnapshot gui(DWORD thread) noexcept {
    gears_quiet::GuiThreadSnapshot result;GUITHREADINFO info{sizeof(info)};
    result.valid=GetGUIThreadInfo(thread,&info)!=FALSE;
    if(result.valid){result.flags=info.flags;result.active=reinterpret_cast<uintptr_t>(info.hwndActive);
        result.focus=reinterpret_cast<uintptr_t>(info.hwndFocus);result.capture=reinterpret_cast<uintptr_t>(info.hwndCapture);}
    else result.error=GetLastError();
    return result;
}
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
            r.enter_qpc=qpc();r.gui_before=gui(r.thread);
            r.enter_tick=GetTickCount64();r.foreground_before=reinterpret_cast<uintptr_t>(GetForegroundWindow());
            void* stack[gears_quiet::trace_stack_capacity]{};
            r.stack_count=CaptureStackBackTrace(1,gears_quiet::trace_stack_capacity,stack,nullptr);
            for(unsigned i=0;i<r.stack_count;++i)r.stack[i]=reinterpret_cast<uintptr_t>(stack[i]);
        }
        SetLastError(error);
    }
    template<class T> static std::uint64_t bits(T value) noexcept {
        if constexpr(std::is_pointer_v<T>)return reinterpret_cast<uintptr_t>(value);
        else return static_cast<std::uint64_t>(value);
    }
    template<class T> T returned(T value) noexcept {
        if(slot){slot->record.returned_value=bits(value);slot->record.return_valid=1;}return value;
    }
    template<class T> T native(T value,unsigned call=~0u) noexcept {
        if(slot){auto& r=slot->record;r.native_value=bits(value);r.native_valid=1;
            r.native_call=call==~0u?r.call:call;r.native_last_error=GetLastError();}return returned(value);
    }
    void policy(std::uint64_t effective) noexcept {if(slot){slot->record.suppressed=1;slot->record.effective_argument=effective;}}
    void created(HWND window) noexcept {if(slot)slot->record.result_window=reinterpret_cast<uintptr_t>(window);}
    ~TraceCall() noexcept {
        if(!slot)return;
        const DWORD error=GetLastError();auto& r=slot->record;
        r.leave_qpc=qpc();r.gui_after=gui(r.thread);
        r.leave_tick=GetTickCount64();r.foreground_after=reinterpret_cast<uintptr_t>(GetForegroundWindow());
        slot->complete.store(true,std::memory_order_release);SetLastError(error);
    }
};
// Thread-local CBT observation only. Never veto activation or system commands.
std::atomic<DWORD> cbt_thread{},cbt_error{};
std::atomic<bool> cbt_installed{},cbt_overflow{};
std::atomic<unsigned> cbt_next{};
std::array<TraceSlot,gears_quiet::trace_capacity> cbt_slots{};
LRESULT CALLBACK observe_cbt(int code,WPARAM wparam,LPARAM lparam) {
    TraceSlot* slot=nullptr;const DWORD entry_error=GetLastError();
    if((code==HCBT_ACTIVATE||code==HCBT_SYSCOMMAND||code==HCBT_MINMAX)&&GetCurrentThreadId()==cbt_thread.load(std::memory_order_relaxed)){
        auto index=cbt_next.load(std::memory_order_relaxed);
        while(index<gears_quiet::trace_capacity&&!cbt_next.compare_exchange_weak(index,index+1,std::memory_order_relaxed)){}
        if(index>=gears_quiet::trace_capacity)cbt_overflow.store(true,std::memory_order_relaxed);
        else {
            slot=&cbt_slots[index];auto& r=slot->record;r.sequence=index+1;r.call=code;r.thread=GetCurrentThreadId();
            r.enter_qpc=qpc();r.enter_tick=GetTickCount64();r.foreground_before=reinterpret_cast<uintptr_t>(GetForegroundWindow());r.gui_before=gui(r.thread);
            if(code==HCBT_ACTIVATE){const auto* data=reinterpret_cast<const CBTACTIVATESTRUCT*>(lparam);
                r.target=wparam;r.related=reinterpret_cast<uintptr_t>(data->hWndActive);r.argument=data->fMouse;}
            else if(code==HCBT_MINMAX){
                // MINMAX-only encoding, no ABI change: related/result_window
                // contain IsIconic before/after; extra packs both style DWORDs.
                const auto window=reinterpret_cast<HWND>(wparam);
                r.target=wparam;r.argument=static_cast<std::uint64_t>(lparam);
                r.related=IsIconic(window)!=FALSE;
                r.extra_argument=static_cast<DWORD>(GetWindowLongPtrW(window,GWL_STYLE));
            }
            else {r.argument=wparam;r.related=static_cast<std::uint64_t>(lparam);}
            r.effective_argument=r.argument;void* stack[gears_quiet::trace_stack_capacity]{};
            r.stack_count=CaptureStackBackTrace(1,gears_quiet::trace_stack_capacity,stack,nullptr);
            for(unsigned i=0;i<r.stack_count;++i)r.stack[i]=reinterpret_cast<uintptr_t>(stack[i]);
        }
    }
    SetLastError(entry_error);
    const LRESULT result=CallNextHookEx(nullptr,code,wparam,lparam);const DWORD leave_error=GetLastError();
    if(slot){auto& r=slot->record;r.leave_qpc=qpc();r.leave_tick=GetTickCount64();
        r.foreground_after=reinterpret_cast<uintptr_t>(GetForegroundWindow());r.gui_after=gui(r.thread);
        r.native_value=r.returned_value=static_cast<std::uint64_t>(result);r.native_valid=r.return_valid=1;
        r.native_call=code;r.native_last_error=leave_error;
        if(code==HCBT_MINMAX){const auto window=reinterpret_cast<HWND>(wparam);
            r.result_window=IsIconic(window)!=FALSE;
            r.extra_argument|=static_cast<std::uint64_t>(static_cast<DWORD>(GetWindowLongPtrW(window,GWL_STYLE)))<<32;}
        slot->complete.store(true,std::memory_order_release);}
    SetLastError(leave_error);return result;
}
void observe_gui_thread(DWORD style,HWND parent) noexcept {
    if(!trace_enabled.load(std::memory_order_acquire)||(style&WS_CHILD)||parent!=nullptr)return;
    const DWORD saved=GetLastError(),thread=GetCurrentThreadId();DWORD empty=0;
    if(cbt_thread.compare_exchange_strong(empty,thread,std::memory_order_relaxed)){
        const HHOOK hook=SetWindowsHookExW(WH_CBT,observe_cbt,nullptr,thread);
        if(hook)cbt_installed.store(true,std::memory_order_release);
        else cbt_error.store(GetLastError(),std::memory_order_release);
    }
    SetLastError(saved);
}
decltype(&ShowWindow) original_show{};
decltype(&ShowWindowAsync) original_show_async{};
decltype(&SetForegroundWindow) original_foreground{},original_native_foreground{};
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
    return trace.native(original_def_window_w(window,message,wparam,lparam));
}
LRESULT WINAPI def_window_a(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
    if(!trace_window_message(message))return original_def_window_a(window,message,wparam,lparam);
    TraceCall trace(gears_quiet::def_window_a,window,message,reinterpret_cast<HWND>(wparam),static_cast<std::uint64_t>(lparam));
    return trace.native(original_def_window_a(window,message,wparam,lparam));
}

bool own(HWND window) {
    DWORD pid{};
    return window && GetWindowThreadProcessId(window,&pid) && pid==GetCurrentProcessId();
}
bool background() { return enabled.load(std::memory_order_acquire) && !own(GetForegroundWindow()); }
bool guard(HWND window,bool top_level=true) {
    return background() && own(window) && (!top_level || !(GetWindowLongPtrW(window,GWL_STYLE)&WS_CHILD));
}
bool place_behind(HWND window) {
    return original_position(window,HWND_BOTTOM,0,0,0,0,
        SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOOWNERZORDER)!=FALSE;
}
BOOL show_behind(HWND window,bool asynchronous=false) {
    return original_position(window,HWND_BOTTOM,0,0,0,0,
        gears_quiet_test::behind_show_flags(asynchronous));
}
BOOL WINAPI show_async(HWND window,int command) {
    // Diagnostic only: preserve queued show behavior and its return value.
    const DWORD saved=GetLastError();
    const DWORD owner_thread=GetWindowThreadProcessId(window,nullptr);
    SetLastError(saved);
    TraceCall trace(gears_quiet::show_async,window,static_cast<DWORD>(command),nullptr,owner_thread);
    if(test_windows_behind&&guard(window)&&command!=SW_HIDE) {
        command=gears_quiet_test::show_command(command,true);trace.policy(command);
        if(command==SW_SHOWNA)return trace.native(show_behind(window,true),gears_quiet::position);
        if(!place_behind(window))return trace.returned(FALSE);
    }
    return trace.native(original_show_async(window,command));
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
            trace.native(original_position(window,nullptr,0,0,0,0,
                SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE),gears_quiet::position);
            trace.policy(SW_HIDE);note(gears_quiet::show);return trace.returned(was_visible);
        }
        case SW_SHOWNORMAL: case SW_SHOWMAXIMIZED: case SW_SHOW:
        case SW_RESTORE: case SW_SHOWDEFAULT:
            command=gears_quiet_test::show_command(command,test_windows_behind);trace.policy(command);note(gears_quiet::show);break;
        case SW_SHOWMINIMIZED: case SW_MINIMIZE:
            command=SW_SHOWMINNOACTIVE;trace.policy(command);note(gears_quiet::show);break;
        default:break;
        }
        if(test_windows_behind) {
            command=gears_quiet_test::show_command(command,true);trace.policy(command);
            if(command==SW_SHOWNA) {
                const BOOL was_visible=IsWindowVisible(window);
                trace.native(show_behind(window),gears_quiet::position);
                return trace.returned(was_visible);
            }
            if(!place_behind(window))return trace.returned(IsWindowVisible(window));
        }
    }
    return trace.native(original_show(window,command));
}
BOOL WINAPI foreground(HWND window) {
    TraceCall trace(gears_quiet::foreground,window);
    if(guard(window,false)) {trace.policy(0);note(gears_quiet::foreground);return trace.returned(FALSE);}
    return trace.native(original_foreground(window));
}
BOOL WINAPI native_foreground(HWND window) {
    TraceCall trace(gears_quiet::native_foreground,window);
    if(guard(window,false)){trace.policy(0);note(gears_quiet::foreground);return trace.returned(FALSE);}
    return trace.native(original_native_foreground(window));
}
HWND WINAPI active(HWND window) {
    TraceCall trace(gears_quiet::active,window);
    if(guard(window,false)) {trace.policy(0);note(gears_quiet::active);return trace.returned(GetActiveWindow());}
    return trace.native(original_active(window));
}
HWND WINAPI focus(HWND window) {
    TraceCall trace(gears_quiet::focus,window);
    if(guard(window,false)) {trace.policy(0);note(gears_quiet::focus);return trace.returned(GetFocus());}
    return trace.native(original_focus(window));
}
BOOL WINAPI position(HWND window,HWND after,int x,int y,int width,int height,UINT flags) {
    TraceCall trace(gears_quiet::position,window,flags,after);
    if(guard(window)) {
        flags=gears_quiet_test::position_flags(flags,test_windows_behind);
        after=gears_quiet_test::insert_after(after,test_windows_behind);
        trace.policy(flags);
        note(gears_quiet::position);
    }
    return trace.native(original_position(window,after,x,y,width,height,flags));
}
BOOL WINAPI bring(HWND window) {
    TraceCall trace(gears_quiet::bring,window);
    if(guard(window)) {trace.policy(0);note(gears_quiet::bring);return trace.returned(TRUE);}
    return trace.native(original_bring(window));
}
// WS_EX_NOACTIVATE alone still changes the creating thread's active window.
// Create hidden, then show without activation. The opt-in main-window trial
// adds only NOACTIVATE to an unowned APPWINDOW; other extended bits are kept.
void finish_create(HWND window,DWORD style,bool guarded) {
    if(window && guarded && (style&WS_VISIBLE))
        gears_quiet_test::first_show(test_windows_behind,[&]{return show_behind(window)!=FALSE;},
            [&](int command){original_show(window,command);});
}
HWND WINAPI create_w(DWORD exstyle,LPCWSTR cls,LPCWSTR title,DWORD style,int x,int y,int width,int height,
                     HWND parent,HMENU menu,HINSTANCE instance,LPVOID argument) {
    observe_gui_thread(style,parent);
    TraceCall trace(gears_quiet::create_w,nullptr,style,parent,exstyle);
    const bool guarded=background() && !(style&WS_CHILD) && parent!=HWND_MESSAGE;
    const DWORD effective_exstyle=exstyle|((main_noactivate&&guarded&&!parent&&(exstyle&WS_EX_APPWINDOW))?WS_EX_NOACTIVATE:0);
    if(guarded)trace.policy(style&~WS_VISIBLE);
    if(guarded)note(gears_quiet::create_w);
    HWND result=original_create_w(effective_exstyle,cls,title,guarded?style&~WS_VISIBLE:style,x,y,width,height,parent,menu,instance,argument);
    DWORD error=GetLastError();trace.native(result);finish_create(result,style,guarded);trace.created(result);SetLastError(error);return result;
}
HWND WINAPI create_a(DWORD exstyle,LPCSTR cls,LPCSTR title,DWORD style,int x,int y,int width,int height,
                     HWND parent,HMENU menu,HINSTANCE instance,LPVOID argument) {
    observe_gui_thread(style,parent);
    TraceCall trace(gears_quiet::create_a,nullptr,style,parent,exstyle);
    const bool guarded=background() && !(style&WS_CHILD) && parent!=HWND_MESSAGE;
    const DWORD effective_exstyle=exstyle|((main_noactivate&&guarded&&!parent&&(exstyle&WS_EX_APPWINDOW))?WS_EX_NOACTIVATE:0);
    if(guarded)trace.policy(style&~WS_VISIBLE);
    if(guarded)note(gears_quiet::create_a);
    HWND result=original_create_a(effective_exstyle,cls,title,guarded?style&~WS_VISIBLE:style,x,y,width,height,parent,menu,instance,argument);
    DWORD error=GetLastError();trace.native(result);finish_create(result,style,guarded);trace.created(result);SetLastError(error);return result;
}
}

extern "C" __declspec(dllexport) DWORD WINAPI GearsQuietInstall(void*) {
    if(enabled.load())return ERROR_ALREADY_EXISTS;
    wchar_t behind_option[2]{};
    test_windows_behind=GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind_option,2)==1&&behind_option[0]==L'1';
    wchar_t main_option[2]{};
    main_noactivate=GetEnvironmentVariableW(L"GEARSVR_QUIET_MAIN_NOACTIVATE",main_option,2)==1&&main_option[0]==L'1';
    wchar_t trace_option[2]{};
    const bool tracing=GetEnvironmentVariableW(L"GEARSVR_QUIET_TRACE",trace_option,2)==1&&trace_option[0]==L'1';
    wchar_t native_option[2]{};
    const bool native_requested=GetEnvironmentVariableW(L"GEARSVR_QUIET_NATIVE_FOREGROUND",native_option,2)==1&&native_option[0]==L'1';
    void* native_target=nullptr;
    if(native_requested){
        const HMODULE win32u=GetModuleHandleW(L"win32u.dll");
        native_target=win32u?reinterpret_cast<void*>(GetProcAddress(win32u,"NtUserSetForegroundWindow")):nullptr;
        if(!native_target)return ERROR_NOT_SUPPORTED;
    }
    if(MH_Initialize()!=MH_OK)return ERROR_INVALID_FUNCTION;
    const std::array<void*,gears_quiet::trace_call_count> targets{
        reinterpret_cast<void*>(&ShowWindow),reinterpret_cast<void*>(&SetForegroundWindow),
        reinterpret_cast<void*>(&SetActiveWindow),reinterpret_cast<void*>(&SetFocus),
        reinterpret_cast<void*>(&SetWindowPos),reinterpret_cast<void*>(&BringWindowToTop),
        reinterpret_cast<void*>(&CreateWindowExW),reinterpret_cast<void*>(&CreateWindowExA),
        reinterpret_cast<void*>(&DefWindowProcW),reinterpret_cast<void*>(&DefWindowProcA),native_target,
        reinterpret_cast<void*>(&ShowWindowAsync)};
    const std::array<void*,gears_quiet::trace_call_count> replacements{
        reinterpret_cast<void*>(&show),reinterpret_cast<void*>(&foreground),
        reinterpret_cast<void*>(&active),reinterpret_cast<void*>(&focus),
        reinterpret_cast<void*>(&position),reinterpret_cast<void*>(&bring),
        reinterpret_cast<void*>(&create_w),reinterpret_cast<void*>(&create_a),
        reinterpret_cast<void*>(&def_window_w),reinterpret_cast<void*>(&def_window_a),reinterpret_cast<void*>(&native_foreground),
        reinterpret_cast<void*>(&show_async)};
    const std::array<void**,gears_quiet::trace_call_count> originals{
        reinterpret_cast<void**>(&original_show),reinterpret_cast<void**>(&original_foreground),
        reinterpret_cast<void**>(&original_active),reinterpret_cast<void**>(&original_focus),
        reinterpret_cast<void**>(&original_position),reinterpret_cast<void**>(&original_bring),
        reinterpret_cast<void**>(&original_create_w),reinterpret_cast<void**>(&original_create_a),
        reinterpret_cast<void**>(&original_def_window_w),reinterpret_cast<void**>(&original_def_window_a),reinterpret_cast<void**>(&original_native_foreground),
        reinterpret_cast<void**>(&original_show_async)};
    std::array<unsigned,gears_quiet::trace_call_count> selected{};unsigned wanted{};
    for(unsigned i=0;i<gears_quiet::count;++i)selected[wanted++]=i;
    if(tracing){selected[wanted++]=gears_quiet::def_window_w;selected[wanted++]=gears_quiet::def_window_a;selected[wanted++]=gears_quiet::show_async;}
    if(native_requested)selected[wanted++]=gears_quiet::native_foreground;
    unsigned created{};
    for(;created<wanted;++created){const unsigned i=selected[created];
        if(MH_CreateHook(targets[i],replacements[i],originals[i])!=MH_OK)break;
    }
    if(created!=wanted){
        for(unsigned i=0;i<created;++i)MH_RemoveHook(targets[selected[i]]);
        MH_Uninitialize();return selected[created]>=gears_quiet::count?ERROR_NOT_SUPPORTED:ERROR_INVALID_FUNCTION;
    }
    if(!gears_instance::prepare()){MH_Uninitialize();return ERROR_INVALID_PARAMETER;}
    if(!gears_instance::create_hooks()){MH_Uninitialize();return gears_instance::failure?gears_instance::failure:ERROR_MOD_NOT_FOUND;}
    if(tracing){
        const DWORD saved=GetLastError();LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);qpc_frequency=frequency.QuadPart;
        STARTUPINFOW info{sizeof(info)};GetStartupInfoW(&info);
        startup.flags=info.dwFlags;startup.show_window=info.wShowWindow;startup.x=info.dwX;startup.y=info.dwY;
        startup.width=info.dwXSize;startup.height=info.dwYSize;startup.columns=info.dwXCountChars;
        startup.rows=info.dwYCountChars;startup.fill=info.dwFillAttribute;
        startup.input=reinterpret_cast<uintptr_t>(info.hStdInput);startup.output=reinterpret_cast<uintptr_t>(info.hStdOutput);
        startup.error=reinterpret_cast<uintptr_t>(info.hStdError);SetLastError(saved);
    }
    // Entry thread is still held by the launcher. Publish policy before enabling.
    trace_enabled.store(tracing,std::memory_order_release);
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
extern "C" __declspec(dllexport) DWORD WINAPI GearsQuietTrace2(void* output) {
    if(!output)return ERROR_INVALID_PARAMETER;
    auto* out=static_cast<gears_quiet::TraceSnapshot2*>(output);*out=gears_quiet::TraceSnapshot2{};
    out->enabled=trace_enabled.load(std::memory_order_acquire);out->qpc_frequency=qpc_frequency;out->startup=startup;
    out->reserved_count=trace_next.load(std::memory_order_relaxed);out->overflow=trace_overflow.load(std::memory_order_relaxed);
    for(unsigned i=0;i<out->reserved_count;++i){
        if(trace_slots[i].complete.load(std::memory_order_acquire))out->records[out->completed++]=trace_slots[i].record;
        else ++out->busy_skipped;
    }
    return ERROR_SUCCESS;
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsQuietCbtTrace(void* output) {
    if(!output)return ERROR_INVALID_PARAMETER;
    auto* out=static_cast<gears_quiet::CbtSnapshot*>(output);*out=gears_quiet::CbtSnapshot{};
    out->enabled=trace_enabled.load(std::memory_order_acquire);out->qpc_frequency=qpc_frequency;
    out->installed=cbt_installed.load(std::memory_order_acquire);out->thread=cbt_thread.load(std::memory_order_relaxed);
    out->install_error=cbt_error.load(std::memory_order_acquire);
    const auto count=cbt_next.load(std::memory_order_relaxed);out->reserved_count=count<gears_quiet::trace_capacity?count:gears_quiet::trace_capacity;
    out->overflow=cbt_overflow.load(std::memory_order_relaxed);
    for(unsigned i=0;i<out->reserved_count;++i){
        if(cbt_slots[i].complete.load(std::memory_order_acquire))out->records[out->completed++]=cbt_slots[i].record;
        else ++out->busy_skipped;
    }
    return ERROR_SUCCESS;
}
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
