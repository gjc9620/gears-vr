// Exercise the reviewed push-only hook with a deliberately occupied near window.
// Reservations consume address space only; no game or desktop window is used.
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "MinHook.h"

using Fn=int(__fastcall*)();
static Fn original;
static int __fastcall detour(){return original()+1;}
struct Region {
    void* p{};
    ~Region(){if(p)VirtualFree(p,0,MEM_RELEASE);}
};
struct HookLifetime {~HookLifetime(){MH_Uninitialize();}};

static bool check(std::uintptr_t radius,bool expect_success){
    constexpr std::uintptr_t anchor=0x4000000000ull,granularity=0x10000;
    Region code,below,above;
    code.p=VirtualAlloc(reinterpret_cast<void*>(anchor),granularity,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    below.p=VirtualAlloc(reinterpret_cast<void*>(anchor-radius),radius,MEM_RESERVE,PAGE_NOACCESS);
    above.p=VirtualAlloc(reinterpret_cast<void*>(anchor+granularity),radius,MEM_RESERVE,PAGE_NOACCESS);
    if(!code.p||!below.p||!above.p){std::printf("reservation_failed win32=%lu\n",GetLastError());return false;}
    // push rbp/r12/r13; mov eax,42; pop r13/r12/rbp; ret.
    const unsigned char body[]={0x40,0x55,0x41,0x54,0x41,0x55,0xb8,42,0,0,0,0x41,0x5d,0x41,0x5c,0x5d,0xc3};
    std::memcpy(code.p,body,sizeof(body));FlushInstructionCache(GetCurrentProcess(),code.p,64);
    unsigned char before[64];std::memcpy(before,code.p,sizeof(before));
    auto fn=reinterpret_cast<Fn>(code.p);if(fn()!=42||MH_Initialize()!=MH_OK)return false;
    HookLifetime cleanup;
    const auto status=MH_CreateHook(code.p,reinterpret_cast<void*>(detour),reinterpret_cast<void**>(&original));
    const auto error=GetLastError();
    if(!expect_success){
        const bool passed=status==MH_ERROR_MEMORY_ALLOC&&error==ERROR_NOT_ENOUGH_MEMORY&&std::memcmp(before,code.p,64)==0&&fn()==42;
        std::printf("occupied_rel32_window status=%d allocation_error=%lu original_entry_unchanged=%d passed=%d\n",status,error,std::memcmp(before,code.p,64)==0,passed);
        return passed;
    }
    if(status!=MH_OK){std::printf("create_failed status=%d allocation_error=%lu\n",status,error);return false;}
    if(fn()!=42||std::memcmp(before,code.p,64))return false;
    const auto delta=static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(original))-static_cast<std::int64_t>(anchor+5);
    if(!(delta<-0x60000000ll&&delta>=INT32_MIN&&delta<=INT32_MAX-64))return false;
    if(MH_EnableHook(code.p)!=MH_OK||fn()!=43||original()!=42)return false;
    if(MH_DisableHook(code.p)!=MH_OK||fn()!=42||std::memcmp(before,code.p,64))return false;
    if(MH_RemoveHook(code.p)!=MH_OK)return false;
    std::printf("occupied_1_5g_window trampoline_delta=%lld hook_result=43 original_result=42 restored=1 passed=1\n",delta);
    return true;
}
int main(){
    const bool expanded=check(0x60000000,true);
    const bool exhausted=check(0x7fff0000,false);
    return expanded&&exhausted?0:1;
}
