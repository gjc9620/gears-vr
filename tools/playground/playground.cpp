// In-process UE3 playground bridge. Engine calls run on the observed game thread.
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <array>
#include <atomic>
#include <share.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include "MinHook.h"
extern "C" DWORD GearsPlaygroundAllocationAttempts(void);
extern "C" DWORD GearsPlaygroundAllocationError(void);

namespace {
using uptr=std::uintptr_t;
struct Array {uptr data;int count,capacity;};
struct Name {unsigned index,number;};
struct Request {volatile LONG state;unsigned size;uptr object,function;unsigned char params[4096];};
static_assert(offsetof(Request,params)==24);
using Event=void(__fastcall*)(uptr,uptr,void*);
Event original{};
uptr base{};
HMODULE module{};
std::atomic<Request*> pending{};
std::atomic<DWORD> game_thread{};
std::atomic<bool> enabled{};
bool hook_created{};
SRWLOCK lifecycle_lock=SRWLOCK_INIT;
struct LifecycleScope {
    LifecycleScope(){AcquireSRWLockExclusive(&lifecycle_lock);}
    ~LifecycleScope(){ReleaseSRWLockExclusive(&lifecycle_lock);}
};
std::atomic<unsigned> callbacks{};
thread_local bool inside{};
FILE* log_file{};
std::array<uptr,256> observed{};unsigned observed_count{};
SRWLOCK log_lock=SRWLOCK_INIT;

bool copy(void* to,uptr from,size_t size){
    if(!from)return false;
    __try{std::memcpy(to,reinterpret_cast<void const*>(from),size);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class T>T get(uptr from){T v{};copy(&v,from,sizeof(v));return v;}
bool registered(uptr object){
    const auto a=get<Array>(base+0x1c24730);
    const auto i=get<unsigned>(object+8);
    return object&&a.data&&a.count>1000&&a.count<=a.capacity&&a.capacity<4000000&&
        i<unsigned(a.count)&&get<uptr>(a.data+i*8)==object;
}
bool name(uptr object,wchar_t* out,size_t count){
    if(!object||!count)return false;
    const auto n=get<Name>(object+0x48);const auto a=get<Array>(base+0x1c34800);
    if(!a.data||a.count<1000||a.count>a.capacity||a.capacity>4000000||n.index>=unsigned(a.count))return false;
    const auto entry=get<uptr>(a.data+n.index*8);
    if(!entry||get<unsigned>(entry)!=n.index)return false;
    for(size_t i=0;i<count;++i){if(!copy(&out[i],entry+20+i*2,2))return false;if(!out[i])return true;}
    out[count-1]=0;return false;
}
void record(uptr object,uptr function){
    AcquireSRWLockExclusive(&log_lock);
    bool exists=false;for(unsigned i=0;i<observed_count;++i)if(observed[i]==function){exists=true;break;}
    if(!exists&&observed_count<observed.size()){
        observed[observed_count++]=function;
        wchar_t f[128]{},o[128]{},c[128]{};
        name(function,f,128);name(object,o,128);name(get<uptr>(object+0x50),c,128);
        if(log_file){fwprintf(log_file,L"event thread=%lu object=%p class=%ls name=%ls function=%ls rva=%llx\n",GetCurrentThreadId(),reinterpret_cast<void*>(object),c,o,f,function-base);fflush(log_file);}
    }
    ReleaseSRWLockExclusive(&log_lock);
}
bool invoke(Request* r){
    if(!r||r->size>sizeof(r->params)||!registered(r->object)||!registered(r->function))return false;
    wchar_t cls[64]{};if(!name(get<uptr>(r->function+0x50),cls,64)||wcscmp(cls,L"Function"))return false;
    const auto bytes=get<unsigned short>(r->function+0x106);
    if(bytes>r->size)return false;
    __try{
        if(!get<unsigned short>(r->function+0xf8)){
            original(r->object,r->function,r->params);return true;
        }
        // Indexed intrinsics are skipped by ProcessEvent in this build. Their
        // native wrappers consume EX_NativeParm/property pairs from an FFrame.
        // Build that frame locally without changing a global UFunction.
        struct Frame {
            uptr vtable;unsigned long long tag;unsigned line,position;
            uptr node,object;unsigned char* code;void* locals;uptr previous,out;
        } frame{base+0x133bcb0,1,1,1,r->function,r->object,nullptr,r->params,0,0};
        static_assert(offsetof(Frame,node)==0x18&&offsetof(Frame,code)==0x28&&offsetof(Frame,locals)==0x30);
        unsigned char code[1024]{};size_t used=0,count=0;
        struct Out {uptr property,address,next;};Out outputs[64]{};unsigned output_count=0;
        auto field=get<uptr>(r->function+0x80);
        while(field){
            if(!registered(field)||++count>64)return false;
            const auto flags=get<unsigned long long>(field+0x78);
            if((flags&0x480)==0x80){
                if(used+9>=sizeof(code))return false;
                code[used++]=0x29;memcpy(code+used,&field,8);used+=8;
                if(flags&0x100){
                    const auto offset=get<unsigned>(field+0x94);
                    if(offset>=r->size)return false;
                    outputs[output_count]={field,reinterpret_cast<uptr>(r->params+offset),0};
                    if(output_count)outputs[output_count-1].next=reinterpret_cast<uptr>(&outputs[output_count]);
                    ++output_count;
                }
            }
            field=get<uptr>(field+0x68);
        }
        code[used++]=0x16;code[used++]=0x53;frame.code=code;
        frame.out=output_count?reinterpret_cast<uptr>(outputs):0;
        auto function=get<uptr>(r->function+0x114);
        if(function<base+0x1000||function>=base+0x131b000)return false;
        const auto result=get<unsigned short>(r->function+0x108);
        if(result!=0xffff&&result>=r->size)return false;
        using Native=void(__fastcall*)(uptr,Frame*,void*);
        reinterpret_cast<Native>(function)(r->object,&frame,result==0xffff?nullptr:r->params+result);
        return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
#include "playground_controller.inl"
void __fastcall event_hook(uptr object,uptr function,void* params){
    struct Active {Active(){++callbacks;}~Active(){--callbacks;}} active;
    if(inside||!enabled.load(std::memory_order_acquire)){original(object,function,params);return;}
    if(playground::automatic&&object!=playground::viewport&&object!=playground::menu_button&&
        get<Name>(function+0x48).index!=playground::player_tick_index){
        original(object,function,params);return;
    }
    inside=true;
    if(!playground::automatic)record(object,function);
    if(playground::clicked(object,function,params)){inside=false;return;}
    // Tick/PlayerTick are dispatched by the main game loop in menu and gameplay.
    // Bind once, then never execute a request on another thread.
    wchar_t f[128]{};name(function,f,128);
    if((!wcscmp(f,L"PlayerTick")||!wcscmp(f,L"Tick"))&&
        (!playground::automatic||object==playground::viewport||
            (!wcscmp(f,L"PlayerTick")&&object==playground::pointer(playground::player,L"Actor")))){
        DWORD zero=0;game_thread.compare_exchange_strong(zero,GetCurrentThreadId());
        if(game_thread.load()==GetCurrentThreadId()){
            if(auto r=pending.exchange(nullptr)){
                const auto ok=invoke(r);InterlockedExchange(&r->state,ok?2:-1);
            }
            if(playground::automatic)playground::tick();
        }
    }
    inside=false;
    original(object,function,params);
}
}

static DWORD reserve_playground_hook(){
    if(hook_created)return 0;
    base=reinterpret_cast<uptr>(GetModuleHandleW(nullptr));
    if(get<unsigned short>(base)!=0x5a4d)return 1;
    // Reservation precedes the engine registries. Independently attest the
    // same complete entry as the loader, then leave the hook disabled.
    const unsigned char expected[]={
        0x40,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0xa0,0x00,0x00,
        0x00,0x48,0x8d,0x6c,0x24,0x30,0x48,0x89,0x9d,0xa0,0x00,0x00,0x00,0x48,0x89,0xb5,
        0xa8,0x00,0x00,0x00,0x48,0x89,0xbd,0xb0,0x00,0x00,0x00,0x48,0x8b,0x05,0x36,0x26,
        0x8f,0x01,0x48,0x33,0xc5,0x48,0x89,0x45,0x60,0x4d,0x8b,0xe0,0x48,0x8b,0xda,0x4c};
    static_assert(sizeof(expected)==64);
    unsigned char bytes[sizeof(expected)]{};
    if(!copy(bytes,base+0x1d8dd8,sizeof(bytes))||memcmp(bytes,expected,sizeof(bytes)))return 2;
    wchar_t path[32768]{};GetModuleFileNameW(module,path,32768);
    auto slash=wcsrchr(path,L'\\');if(!slash)return 3;
    swprintf_s(slash+1,32768-(slash+1-path),L"playground-%lu.log",GetCurrentProcessId());
    log_file=_wfsopen(path,L"w, ccs=UTF-8",_SH_DENYNO);
    if(MH_Initialize()!=MH_OK){if(log_file){fclose(log_file);log_file=nullptr;}return 4;}
    const auto created=MH_CreateHook(reinterpret_cast<void*>(base+0x1d8dd8),reinterpret_cast<void*>(event_hook),reinterpret_cast<void**>(&original));
    if(created!=MH_OK){
        const auto allocation_error=GetLastError();
        if(log_file){fwprintf(log_file,L"create_hook_failed=%u allocation_stage=%ls win32=%lu allocation_attempts=%lu allocation_last_error=%lu\n",unsigned(created),
            allocation_error==ERROR_NOT_ENOUGH_MEMORY?L"trampoline":allocation_error==ERROR_OUTOFMEMORY?L"hook_table":L"other",allocation_error,
            GearsPlaygroundAllocationAttempts(),GearsPlaygroundAllocationError());fclose(log_file);log_file=nullptr;}
        MH_Uninitialize();return 5;
    }
    // MinHook's relay lives in the same 64-byte slot as the trampoline. Verify
    // the entire slot is reachable before allowing a five-byte rel32 patch.
    const auto delta=static_cast<std::int64_t>(reinterpret_cast<uptr>(original))-static_cast<std::int64_t>(base+0x1d8dd8+5);
    if(delta<INT32_MIN||delta>INT32_MAX-64){
        MH_RemoveHook(reinterpret_cast<void*>(base+0x1d8dd8));MH_Uninitialize();
        if(log_file){fwprintf(log_file,L"relay_range_rejected delta=%lld\n",delta);fclose(log_file);log_file=nullptr;}
        return 7;
    }
    if(log_file){fwprintf(log_file,L"hook_created relay_delta=%lld allocation_attempts=%lu allocation_last_error=%lu\n",delta,GearsPlaygroundAllocationAttempts(),GearsPlaygroundAllocationError());fflush(log_file);}
    hook_created=true;return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundReserve(void*){
    LifecycleScope lifecycle;
    return reserve_playground_hook();
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundInitialize(void*){
    LifecycleScope lifecycle;
    if(enabled.load())return 0;
    base=reinterpret_cast<uptr>(GetModuleHandleW(nullptr));
    if(!registered(get<uptr>(get<Array>(base+0x1c24730).data+8)))return 1;
    const auto reserved=reserve_playground_hook();if(reserved)return reserved;
    enabled.store(true,std::memory_order_release);
    if(MH_EnableHook(reinterpret_cast<void*>(base+0x1d8dd8))!=MH_OK){
        enabled.store(false);MH_RemoveHook(reinterpret_cast<void*>(base+0x1d8dd8));MH_Uninitialize();
        hook_created=false;
        if(log_file){fclose(log_file);log_file=nullptr;}return 6;
    }
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundSubmit(void* data){
    auto r=static_cast<Request*>(data);
    if(!enabled.load()||!r||r->state!=0||r->size>4096)return 1;
    Request* empty=nullptr;
    if(!pending.compare_exchange_strong(empty,r))return 2;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundEnable(void*){
    if(!enabled.load())return 1;
    playground::viewport=playground::find_instance(L"WarGameViewportClient");
    if(!playground::viewport)return 2;
    const auto pc_class=playground::find_class(L"PlayerController");
    const auto player_tick=pc_class?playground::field(pc_class,L"PlayerTick",L"Function"):0;
    if(!player_tick)return 2;
    playground::player_tick_index=get<Name>(player_tick+0x48).index;
    playground::player=playground::find_instance(L"WarLocalPlayer");
    playground::automatic=true;return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundTestEnter(void*){
    // Available only in the user's explicit unattended, muted test child.
    wchar_t behind[4]{},muted[4]{};
    if(GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind,4)!=1||behind[0]!=L'1'||
       GetEnvironmentVariableW(L"GEARSVR_TEST_MUTE_GAME",muted,4)!=1||muted[0]!=L'1')return 1;
    if(!enabled.load()||!playground::automatic)return 2;
    playground::test_enter_pending=true;return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundShutdown(void*){
    LifecycleScope lifecycle;
    enabled.store(false,std::memory_order_release);
    if(auto r=pending.exchange(nullptr))InterlockedExchange(&r->state,-2);
    if(hook_created){
        const auto disabled=MH_DisableHook(reinterpret_cast<void*>(base+0x1d8dd8));
        if(disabled!=MH_OK&&disabled!=MH_ERROR_DISABLED)return 1;
    }
    const auto until=GetTickCount64()+2000;
    while(callbacks.load()&&GetTickCount64()<until)Sleep(1);
    if(callbacks.load())return 2;
    if(hook_created){
        if(MH_RemoveHook(reinterpret_cast<void*>(base+0x1d8dd8))!=MH_OK)return 3;
        if(MH_Uninitialize()!=MH_OK)return 4;
        hook_created=false;
    }
    playground::automatic=false;playground::ready=false;playground::requested=false;
    playground::phase=L"stopped";playground::status();
    if(log_file){fclose(log_file);log_file=nullptr;}
    return 0;
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,void*){
    if(reason==DLL_PROCESS_ATTACH){module=h;DisableThreadLibraryCalls(h);}return TRUE;
}
