#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <exception>

// Opt-in lifecycle diagnostics. No logging on frames, no shared ABI change.
// Phase names are static literals supplied by the owner. Handler intentionally
// stays installed through local and CRT destruction after main's return marker.
namespace shutdown_diagnostics {
inline std::atomic<bool> enabled{false};
inline std::atomic<const char*> phase{"diagnostics_disabled"};
inline std::atomic_flag terminating=ATOMIC_FLAG_INIT;
inline HANDLE output=INVALID_HANDLE_VALUE;
struct Buffer {
    char bytes[4096]{};DWORD size{};
    void character(char c) noexcept {if(size<sizeof(bytes))bytes[size++]=c;}
    void text(const char* s) noexcept {if(s)for(unsigned i=0;i<256&&s[i];++i)character(s[i]);}
    void number(std::uint64_t n) noexcept {char digits[20];unsigned count{};do{digits[count++]=char('0'+n%10);n/=10;}while(n);while(count)character(digits[--count]);}
    void signed_number(std::int64_t n) noexcept {if(n<0){character('-');number(std::uint64_t(-(n+1))+1);}else number(std::uint64_t(n));}
    void address(void* p) noexcept {text("0x");auto n=reinterpret_cast<std::uintptr_t>(p);for(int shift=int(sizeof(n)*8)-4;shift>=0;shift-=4)character("0123456789abcdef"[(n>>shift)&15]);}
    void write() noexcept {if(output&&output!=INVALID_HANDLE_VALUE){DWORD written{};WriteFile(output,bytes,size,&written,nullptr);}}
};
inline void marker(const char* name,bool result_present=false,std::int64_t result=0,bool window_state=false) noexcept {
    if(!enabled.load(std::memory_order_relaxed))return;
    phase.store(name,std::memory_order_relaxed);
    Buffer b;b.text("shutdown_diag phase=");b.text(name);b.text(" thread=");b.number(GetCurrentThreadId());
    if(window_state){
        const auto tick=GetTickCount64();const auto window=GetForegroundWindow();DWORD pid{};
        if(window)GetWindowThreadProcessId(window,&pid);
        b.text(" tick_ms=");b.number(tick);b.text(" foreground_hwnd=");b.address(window);
        b.text(" foreground_pid=");b.number(pid);
    }
    if(result_present){b.text(" returned_result=");b.signed_number(result);}
    b.character('\n');b.write();
}
inline void initialization(const char* name) noexcept {marker(name,false,0,true);}
[[noreturn]] inline void terminate_handler() noexcept {
    if(terminating.test_and_set(std::memory_order_relaxed))std::abort();
    Buffer b;b.text("shutdown_diag terminate phase=");b.text(phase.load(std::memory_order_relaxed));
    b.text(" thread=");b.number(GetCurrentThreadId());b.text(" current_exception_present=");
    b.number(std::current_exception()?1:0);
    void* frames[32]{};const auto count=CaptureStackBackTrace(0,32,frames,nullptr);
    b.text(" stack_count=");b.number(count);b.character('\n');
    for(unsigned i=0;i<count;++i){b.text("shutdown_diag stack index=");b.number(i);b.text(" address=");b.address(frames[i]);b.character('\n');}
    b.write();std::abort();
}
inline void enable() noexcept {
    output=GetStdHandle(STD_ERROR_HANDLE);enabled.store(true,std::memory_order_relaxed);
    std::set_terminate(terminate_handler);marker("diagnostics_enabled");
}
// Reset and exception-unwind deletion share the same bounded markers. No
// completed marker is emitted unless the actual delete expression returns.
template<class T> struct DeleteMarker {
    const char* before{};const char* after{};
    void operator()(T* p) const noexcept {if(!p)return;marker(before);delete p;marker(after);}
};
}
