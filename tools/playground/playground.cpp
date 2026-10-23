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
#if defined(GEARS_PLAYGROUND_HAND_IK_TRIAL) && defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
#error The single-pulse and controlled IK candidates cannot be combined
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_TRIAL) || defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL) || defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
#define GEARS_PLAYGROUND_HAND_IK_HELPERS 1
#endif
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
#include "../vr-common/hand_animation_observation.h"
#endif
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
#include "../native-capture/ui_call_boundary.h"
#endif
#if defined(GEARS_PLAYGROUND_FREE_HANDS_PROBE) || defined(GEARS_PLAYGROUND_HAND_IK_HELPERS)
#include "../vr-common/free_hand_control.h"
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_HELPERS)
#include "../vr-common/hand_ik_trial_policy.h"
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
#include "../vr-common/hand_ik_controlled_trial_policy.h"
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
#include "../vr-common/hand_ik_stream_lease.h"
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
#include "../vr-common/weapon_mesh_component_lease.h"
#include "../vr-common/weapon_mesh_follow.h"
#if !defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL) || !defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
#error Phase1 component lease requires certified combined hands
#endif
#endif
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
#include "../vr-common/hand_stream_grip_binding.h"
#include "../vr-common/hand_stream_ticket_policy.h"
#include "hand_ik_xr_stream_request.h"
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
#include "hand_stream_session.h"
#if !defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL) || !defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
#error Continuous hand session requires certified combined hands
#endif
#endif
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
#include "../vr-common/hand_world_continuity_bridge.h"
#endif
#endif
#if defined(GEARS_PLAYGROUND_LANCER_BLADE_PROBE_TEST)
#include "../vr-common/weapon_lancer_blade_outline.h"
#endif
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_R2)
#include "../vr-common/controller_input_bridge.h"
#include "../vr-common/original_r2_fire.h"
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
#include "../vr-common/original_l2_aim.h"
#endif
#include "../vr-common/pose_bridge.h"
#include "../vr-common/stereo_bridge.h"
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_OBSERVE)
#include "../vr-common/weapon_equipment_model.h"
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
#include "../vr-common/equipment_switch_mailbox.h"
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
#include "../vr-common/weapon_stow_lease.h"
#include "../vr-common/equipment_stow_pending.h"
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
#include "../vr-common/equipment_intent_bridge.h"
#if defined(GEARS_PLAYGROUND_LANCER_RELOAD_HOLD_TEST)
#include "../vr-common/weapon_lancer_feed_anchor.h"
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
#include "../vr-common/weapon_mesh_follow.h"
#endif
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
#include "../vr-common/weapon_lancer_magazine_pose.h"
#include "../vr-common/weapon_lancer_insertion.h"
#endif
#if defined(GEARS_PLAYGROUND_LANCER_SAW_GESTURE_TRIAL) || \
    defined(GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST)
#include "../vr-common/weapon_lancer_chainsaw_pose.h"
#include "../vr-common/weapon_lancer_starter_mount.h"
#endif
#endif
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
namespace handanim {unsigned bridge_before(Request*);void bridge_after(Request*,unsigned);}
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
namespace handstream {
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
void before_action(uptr,uptr,bool=false);
bool bridge_before_allowed(uptr,uptr);
#else
void before_action(uptr,uptr);
#endif
}
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
namespace vrequip {namespace stow {void before_action(uptr,uptr);}}
namespace vrfire {
enum class EquipmentYield:unsigned {failed,settling,ready};
EquipmentYield yield_for_equipment(uptr,uptr);
}
#endif
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
namespace handstream {bool yield_for_equipment(const wchar_t*);bool equipment_ready();}
namespace vrequip {unsigned hand_equipment_phase(uptr,uptr,uptr);}
#endif
bool invoke_engine(Request* r){
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
bool invoke(Request* r){
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(r&&!handstream::bridge_before_allowed(r->object,r->function))return false;
#else
    if(r)handstream::before_action(r->object,r->function);
#endif
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    if(r)vrequip::stow::before_action(r->object,r->function);
#endif
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
    const auto observation=handanim::bridge_before(r);
#endif
    const auto result=invoke_engine(r);
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
    handanim::bridge_after(r,observation);
#endif
    return result;
}
#include "playground_controller.inl"
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
#include "hand_animation_probe.inl"
#endif
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
#include "hand_animation_call_probe.inl"
#endif
#if defined(GEARS_PLAYGROUND_FREE_HANDS_PROBE) || defined(GEARS_PLAYGROUND_HAND_IK_HELPERS)
#include "vr_free_hands.inl"
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_HELPERS)
#include "vr_hand_ik_trial.inl"
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
#include "vr_hand_ik_controlled_trial.inl"
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
#include "vr_hand_ik_stream_trial.inl"
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_OBSERVE)
#include "vr_equipment_snapshot.inl"
#endif
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_R2)
#include "vr_fire_adapter.inl"
#endif
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
namespace handstream {
#include "vr_hand_weapon_mesh_phase1.inl"
}
#endif
void __fastcall event_hook(uptr object,uptr function,void* params){
    struct Active {Active(){++callbacks;}~Active(){--callbacks;}} active;
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
    handstream::EquipmentEntryScope equipment_entry_observation(object,function);
#endif
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    handstream::EquipmentTimingHookScope equipment_timing_dump;
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    // Release before any original action dispatch, including recursive calls.
    handstream::before_action(object,function);
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    vrequip::stow::before_action(object,function);
#endif
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
    // Observe before both the recursion bypass and automatic-mode filter.
    // This independent scope never consumes, alters or invokes an engine call.
    handanim::HookScope hand_animation_observation(object,function,params);
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
    handikcontrolled::TickScope controlled_ik_tick_observation(object,function);
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
    bool post_weapon_tick=false;
#endif
#if defined(GEARS_PLAYGROUND_FREE_HANDS_PROBE)
    bool post_free_hand_tick=false;
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_TRIAL)
    bool post_hand_ik_tick=false;
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
    bool post_controlled_hand_ik_tick=false;
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    bool post_stream_hand_ik_tick=false;
#endif
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
    bool post_hand_animation_tick=false;
#endif
    if((!wcscmp(f,L"PlayerTick")||!wcscmp(f,L"Tick"))&&
        (!playground::automatic||object==playground::viewport||
            (!wcscmp(f,L"PlayerTick")&&object==playground::pointer(playground::player,L"Actor")))){
        DWORD zero=0;game_thread.compare_exchange_strong(zero,GetCurrentThreadId());
        if(game_thread.load()==GetCurrentThreadId()){
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
            post_stream_hand_ik_tick=playground::automatic&&!wcscmp(f,L"PlayerTick")&&
                object==playground::pointer(playground::player,L"Actor");
            if(post_stream_hand_ik_tick){
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
                handstream::pre_tick(object,equipment_entry_observation.observation());
#else
                handstream::pre_tick(object);
#endif
            }
#endif
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
            post_hand_animation_tick=playground::automatic&&!wcscmp(f,L"PlayerTick")&&
                object==playground::pointer(playground::player,L"Actor");
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_TRIAL)
            post_hand_ik_tick=playground::automatic&&!wcscmp(f,L"PlayerTick")&&
                object==playground::pointer(playground::player,L"Actor");
            if(post_hand_ik_tick)handik::pre_tick(object);
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
            post_controlled_hand_ik_tick=playground::automatic&&!wcscmp(f,L"PlayerTick")&&
                object==playground::pointer(playground::player,L"Actor");
            if(post_controlled_hand_ik_tick)handikcontrolled::pre_tick(object);
#endif
#if defined(GEARS_PLAYGROUND_FREE_HANDS_PROBE)
            post_free_hand_tick=playground::automatic&&!wcscmp(f,L"PlayerTick")&&
                object==playground::pointer(playground::player,L"Actor");
#endif
            if(auto r=pending.exchange(nullptr)){
                const auto ok=invoke(r);InterlockedExchange(&r->state,ok?2:-1);
            }
#if defined(GEARS_PLAYGROUND_EQUIPMENT_OBSERVE)
            if(playground::automatic&&!wcscmp(f,L"PlayerTick")&&
               object==playground::pointer(playground::player,L"Actor")){
                vrequip::tick(object);
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
                post_weapon_tick=true;
#endif
            }
#endif
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_R2)
            if(playground::automatic&&!wcscmp(f,L"PlayerTick")&&
               object==playground::pointer(playground::player,L"Actor"))vrfire::tick(object);
#endif
            if(playground::automatic)playground::tick();
        }
    }
    inside=false;
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    {
        handstream::EquipmentTimingScope timing(handstream::EquipmentTiming::original_player_tick,post_stream_hand_ik_tick);
        original(object,function,params);
    }
#else
    original(object,function,params);
#endif
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
    if(post_hand_animation_tick&&enabled.load(std::memory_order_acquire)&&
       game_thread.load()==GetCurrentThreadId()){
        inside=true;handanim::post_tick(object);inside=false;
    }
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_TRIAL)
    if(post_hand_ik_tick&&enabled.load(std::memory_order_acquire)&&
       game_thread.load()==GetCurrentThreadId()){
        inside=true;handik::post_tick(object);inside=false;
    }
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
    if(post_controlled_hand_ik_tick&&enabled.load(std::memory_order_acquire)&&
       game_thread.load()==GetCurrentThreadId()){
        inside=true;handikcontrolled::post_tick(object);inside=false;
    }
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    if(post_stream_hand_ik_tick&&enabled.load(std::memory_order_acquire)&&
       game_thread.load()==GetCurrentThreadId()){
        inside=true;handstream::post_tick(object);inside=false;
    }
#endif
#if defined(GEARS_PLAYGROUND_FREE_HANDS_PROBE)
    if(post_free_hand_tick&&enabled.load(std::memory_order_acquire)&&
       game_thread.load()==GetCurrentThreadId()){
        inside=true;freehands::post_tick(object);inside=false;
    }
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
    if(post_weapon_tick&&enabled.load(std::memory_order_acquire)&&
       game_thread.load()==GetCurrentThreadId()){
        inside=true;
        vrequip::support_probe_post_tick(object);
#if defined(GEARS_PLAYGROUND_LANCER_BLADE_PROBE_TEST)
        vrequip::lancer_blade_probe_post_tick(object);
#endif
        vrequip::reload_visual_post_tick(object);
        vrequip::lancer_socket_post_tick(object);
        vrequip::mesh_follow_trial_prepare(object);
        vrequip::mesh_follow_post_tick(object);
        vrequip::mesh_follow_trial_finish(object);
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
        vrequip::lancer_magazine_post_tick(object);
#endif
        inside=false;
    }
#endif
}
}

static DWORD reserve_playground_hook(){
    if(hook_created)return 0;
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
    if(!handanim::test_environment())return 34;
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
    if(!handanimcall::reserve()){
        handanimcall::remove();MH_RemoveHook(reinterpret_cast<void*>(base+0x1d8dd8));MH_Uninitialize();
        if(log_file){fwprintf(log_file,L"hand_animation_call_hook_reservation_failed\n");fclose(log_file);log_file=nullptr;}
        return 30;
    }
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
    handanim::reset();
#endif
#if defined(GEARS_PLAYGROUND_FREE_HANDS_PROBE) || defined(GEARS_PLAYGROUND_HAND_IK_HELPERS)
    freehands::reset();
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_TRIAL)
    handik::reset();
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
    handikcontrolled::reset();
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    handstream::reset();
#endif
#if defined(GEARS_PLAYGROUND_LANCER_RELOAD_HOLD_TEST)
    vrequip::lancer_reload_test_start=false;
    vrequip::lancer_reload_test_complete=false;
    vrequip::lancer_reload_status=0;
    vrequip::lancer_reload_hold={};
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
    vrequip::mesh_follow_shutdown_requested.store(false,std::memory_order_release);
    vrequip::mesh_follow_requested.store(false,std::memory_order_release);
    vrequip::mesh_follow_status.store(0,std::memory_order_release);
    vrequip::mesh_follow_trial_requested.store(false,std::memory_order_release);
    vrequip::mesh_follow_trial_status.store(0,std::memory_order_release);
    vrequip::mesh_follow_trial_samples.store(0,std::memory_order_release);
    vrequip::mesh_follow_trial={};
    vrequip::support_probe_requested.store(false,std::memory_order_release);
    vrequip::support_probe_status.store(0,std::memory_order_release);
    vrequip::support_probe_count.store(0,std::memory_order_release);
    vrequip::support_probe={};
    vrequip::reload_visual_requested.store(false,std::memory_order_release);
    vrequip::reload_visual_status.store(0,std::memory_order_release);
    vrequip::reload_visual_count.store(0,std::memory_order_release);
    vrequip::reload_visual={};
    vrequip::lancer_socket_requested.store(false,std::memory_order_release);
    vrequip::lancer_socket_reload_mode.store(false,std::memory_order_release);
    vrequip::lancer_socket_status.store(0,std::memory_order_release);
    vrequip::lancer_socket_count.store(0,std::memory_order_release);
    vrequip::lancer_socket={};
#endif
    base=reinterpret_cast<uptr>(GetModuleHandleW(nullptr));
    if(!registered(get<uptr>(get<Array>(base+0x1c24730).data+8)))return 1;
    const auto reserved=reserve_playground_hook();if(reserved)return reserved;
    enabled.store(true,std::memory_order_release);
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
    const auto activated=MH_EnableHook(MH_ALL_HOOKS);
#else
    const auto activated=MH_EnableHook(reinterpret_cast<void*>(base+0x1d8dd8));
#endif
    if(activated!=MH_OK){
        enabled.store(false);
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
        const auto disabled=MH_DisableHook(MH_ALL_HOOKS);
        if(disabled!=MH_OK&&disabled!=MH_ERROR_DISABLED)return 31;
        const auto until=GetTickCount64()+2000;
        while(callbacks.load(std::memory_order_acquire)&&GetTickCount64()<until)Sleep(1);
        if(callbacks.load(std::memory_order_acquire))return 31;
        if(!handanimcall::remove())return 32;
#endif
        MH_RemoveHook(reinterpret_cast<void*>(base+0x1d8dd8));MH_Uninitialize();
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
#if defined(GEARS_PLAYGROUND_EQUIPMENT_OBSERVE)
// Private diagnostics only. A return code of 2 from PlaygroundEnable alone
// cannot distinguish an absent viewport from an unresolved PlayerTick.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundEquipmentReadiness(void*){
    if(!enabled.load(std::memory_order_acquire))return 0;
    DWORD bits=1;
    if(playground::find_instance(L"WarGameViewportClient"))bits|=2;
    const auto pc_class=playground::find_class(L"PlayerController");
    if(pc_class)bits|=4;
    if(pc_class&&playground::field(pc_class,L"PlayerTick",L"Function"))bits|=8;
    if(playground::find_instance(L"WarLocalPlayer"))bits|=16;
    return bits;
}
#endif
#if defined(GEARS_PLAYGROUND_LANCER_SAW_GESTURE_TRIAL)
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerSawGestureTrialStart(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment()||
       vrequip::lancer_saw_active_visible.load(std::memory_order_acquire)
#if defined(GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST)
       ||vrequip::lancer_saw_xr_visible.load(std::memory_order_acquire)
#endif
       )return 1;
    unsigned expected=0;
    if(!vrequip::lancer_saw_gesture_trial_status.compare_exchange_strong(expected,1))return 2;
    vrequip::lancer_saw_gesture_trial_requested.store(true,std::memory_order_release);
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerSawGestureTrialStatus(void*){
    return vrequip::lancer_saw_gesture_trial_status.load(std::memory_order_acquire);
}
#endif
#if defined(GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST)
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerSawXrPullArm(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment()||
       vrequip::lancer_saw_active_visible.load(std::memory_order_acquire)||
       vrequip::lancer_saw_xr_visible.load(std::memory_order_acquire)
#if defined(GEARS_PLAYGROUND_LANCER_SAW_GESTURE_TRIAL)
       ||vrequip::lancer_saw_gesture_trial_status.load(std::memory_order_acquire)==1
       ||vrequip::lancer_saw_gesture_trial_status.load(std::memory_order_acquire)==2
#endif
       )return 1;
    bool expected=false;
    if(!vrequip::lancer_saw_xr_arm.compare_exchange_strong(expected,true))return 2;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerSawXrPullStop(void*){
    vrequip::lancer_saw_xr_arm.store(false,std::memory_order_release);
    vrequip::lancer_saw_xr_stop.store(true,std::memory_order_release);
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerSawXrPullStatus(void*){
    return vrequip::lancer_saw_xr_status.load(std::memory_order_acquire);
}
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
// Status: 0 queued, 1 original call sent, 2 invalidated, 3 ignored/busy,
// 4 empty slot with original cue, 5 model-only stow/unholster, 6 equipped, 7 timed out,
// 8 reflected original call failed, 9 empty slot without a usable original
// empty-clip cue. Stow candidate additionally reports 10 verified stowed,
// 11 verified in hand, 12 stow/unholster rejected. Private test exports only.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundEquipmentSwitchTest(void* slot_arg){
    // Remote test producer only. Serialize teardown against publishing slot_
    // so the drained-only mailbox reset cannot race an in-flight producer.
    // A game callback must not wait on a shutdown caller that is draining it.
    if(GetCurrentThreadId()==game_thread.load(std::memory_order_acquire))return 4;
    LifecycleScope lifecycle;
    wchar_t behind[4]{},muted[4]{};
    if(GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind,4)!=1||behind[0]!=L'1'||
       GetEnvironmentVariableW(L"GEARSVR_TEST_MUTE_GAME",muted,4)!=1||muted[0]!=L'1'||
       !enabled.load(std::memory_order_acquire)||!playground::automatic)return 1;
    const auto slot=reinterpret_cast<uptr>(slot_arg);
    if(slot>=4)return 2;
    if(!vrequip::switch_requests.enqueue(unsigned(slot)))return 3;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundEquipmentSwitchStatus(void*){
    return vrequip::switch_requests.status();
}
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundEquipmentStowStop(void*){
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    handstream::stop_requested.store(true,std::memory_order_release);
#endif
    vrequip::stow::stop_requested.store(true,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundEquipmentStowStatus(void*){
    return vrequip::stow::status.load(std::memory_order_acquire)|
        (vrequip::stow::dirty_visible.load(std::memory_order_acquire)?0x10000u:0u);
}
#endif
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
// Low word: 0 idle, 1 queued, 2 sampling, 3 bounded complete, 4 explicit stop,
// 5 invalidated. High word: periodic frames, at most 1500. No status grants
// ownership; the probe is read-only and has nothing to restore.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandAnimationProbeStart(void*){
    LifecycleScope lifecycle;
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||!playground::ready||
       !handanim::test_environment()||!log_file)return 1;
    bool inactive=false;
    if(!handanim::active_visible.compare_exchange_strong(inactive,true,std::memory_order_acq_rel))return 2;
    handanim::stop_requested=false;handanim::frames=0;handanim::status=1;
    handanim::requested.store(true,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandAnimationProbeStop(void*){
    handanim::stop_requested.store(true,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandAnimationProbeStatus(void*){
    return DWORD((handanim::frames.load(std::memory_order_acquire)&0xffff)<<16)|
        DWORD(handanim::status.load(std::memory_order_acquire));
}
#endif
#if defined(GEARS_PLAYGROUND_FREE_HANDS_PROBE)
// Bounded read-only probe; no skeletal writer is compiled. Status 1 queued/
// waiting, 2 sampling, 3 complete, 4 stopped, 5 invalidated. The high word of
// status is the number of recorded samples (up to 160).
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundFreeHandsProbeStart(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!freehands::test_environment()||
       freehands::active_visible.load(std::memory_order_acquire))return 1;
    bool expected=false;
    return freehands::requested.compare_exchange_strong(expected,true)?0:2;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundFreeHandsProbeStop(void*){
    freehands::stop_requested.store(true,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundFreeHandsProbeStatus(void*){
    return DWORD(freehands::sample_count.load(std::memory_order_acquire)<<16)|
        DWORD(freehands::status.load(std::memory_order_acquire));
}
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_TRIAL)
// This is single-use per initialized private DLL. A zero Start result queues
// an observation; it does not attest a setter, bone evaluation or restoration.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandIkTrialStart(void*){
    LifecycleScope lifecycle;
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||!playground::ready||
       !freehands::test_environment()||!log_file||
       handik::shutdown_requested.load(std::memory_order_acquire))return 1;
    bool unused=false;
    if(!handik::used.compare_exchange_strong(unused,true))return 2;
    handik::stop_requested=false;handik::safe_to_unload=false;handik::status=1;handik::samples=0;
    handik::requested.store(true,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandIkTrialStop(void*){
    if(game_thread.load(std::memory_order_acquire)==GetCurrentThreadId()){
        handik::stop_requested.store(true,std::memory_order_release);return 41;
    }
    LifecycleScope lifecycle;
    return handik::stop_and_wait();
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandIkTrialStatus(void*){
    return DWORD((handik::samples.load(std::memory_order_acquire)&0xffff)<<16)|
        DWORD(handik::status.load(std::memory_order_acquire));
}
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
// Recording is single-use per initialization. Status 5 means recording ended;
// it always remains inconclusive about causal bone effects or visible 6DoF.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandIkControlledTrialStart(void*){
    LifecycleScope lifecycle;
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||!playground::ready||
       !freehands::test_environment()||!log_file||ferror(log_file)||
       freehands::active_visible.load(std::memory_order_acquire)||
       handikcontrolled::shutdown_requested.load(std::memory_order_acquire))return 1;
    bool unused=false;
    if(!handikcontrolled::used.compare_exchange_strong(unused,true))return 2;
    handikcontrolled::stop_requested=false;handikcontrolled::safe_to_unload=false;
    handikcontrolled::status=1;handikcontrolled::samples=0;
    handikcontrolled::requested.store(true,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandIkControlledTrialStop(void*){
    if(game_thread.load(std::memory_order_acquire)==GetCurrentThreadId()){
        handikcontrolled::stop_requested.store(true,std::memory_order_release);return 41;
    }
    LifecycleScope lifecycle;return handikcontrolled::stop_and_wait();
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandIkControlledTrialStatus(void*){
    return DWORD((handikcontrolled::samples.load(std::memory_order_acquire)&0xffff)<<16)|
        DWORD(handikcontrolled::status.load(std::memory_order_acquire));
}
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
// One private Start per module lifetime; public hand ownership remains unproved.
// The opt-in continuous build changes lifetime, never this frozen Start ABI.
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandIkXrStreamTrialStart(void* argument){
#else
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandIkStreamTrialStart(void*){
#endif
    LifecycleScope lifecycle;
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||!playground::ready||
       !freehands::test_environment()||!handanimcall::reserved||!log_file||ferror(log_file)||
       handstream::shutdown_requested.load(std::memory_order_acquire))return 1;
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
    // A rejected second Start must not replace the first request while its
    // game-thread begin is still queued. LifecycleScope serializes callers.
    if(handstream::used.load(std::memory_order_acquire))return 2;
    if(!handstream::prepare_xr_request(argument))return 3;
#endif
    bool unused=false;
    if(!handstream::used.compare_exchange_strong(unused,true))return 2;
    handstream::stop_requested=false;handstream::safe_to_unload=false;
    handstream::status=1;handstream::leases=0;handstream::active_visible=true;
    handstream::requested.store(true,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandIkStreamTrialStop(void*){
    if(game_thread.load(std::memory_order_acquire)==GetCurrentThreadId()){
        handstream::stop_requested.store(true,std::memory_order_release);return 41;
    }
    LifecycleScope lifecycle;return handstream::stop_and_wait();
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandIkStreamTrialStatus(void*){
#if defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
    return DWORD(gears_hand_session::status_leases(handstream::leases.load(std::memory_order_acquire))<<16)|
        DWORD(handstream::status.load(std::memory_order_acquire));
#else
    return DWORD((handstream::leases.load(std::memory_order_acquire)&0xffff)<<16)|
        DWORD(handstream::status.load(std::memory_order_acquire));
#endif
}
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST)
// 0 no grab yet, 1 spare ammo present (no object spawned), 2 empty with cue,
// 3 empty without cue, 4 invalidated/unavailable, 5 original Lancer component
// mounted to the left hand in the explicitly gated private visual trial.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundAmmoGrabStatus(void*){
    return vrequip::last_ammo_status.load(std::memory_order_acquire);
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundHandPoseStatus(void*){
    return DWORD((vrequip::pose_accepted.load(std::memory_order_acquire)&0xffff)<<16)|
           DWORD(vrequip::pose_rejected.load(std::memory_order_acquire)&0xffff);
}
#endif
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
// Synthetic left-hand path solely for the owned muted arena. It cannot be
// enabled by the normal launcher or be mistaken for Quest controller input.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerMagazineFollowStartTest(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment())return 1;
    bool expected=false;
    if(!vrequip::lancer_magazine_test_request.compare_exchange_strong(expected,true))return 2;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerMagazineInsertStartTest(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment())return 1;
    bool expected=false;
    if(!vrequip::lancer_magazine_insert_test_request.compare_exchange_strong(
           expected,true))return 2;
    return 0;
}
// Status 6 records an insertion-shaped trajectory only. No original reload
// completion, ammo settlement, or active-reload reward follows from it.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerMagazineFollowStatus(void*){
    return vrequip::lancer_magazine_status.load(std::memory_order_acquire);
}
#endif
#if defined(GEARS_PLAYGROUND_LANCER_RELOAD_HOLD_TEST)
// Test-only queue: exercises the same game-thread start/completion helpers as
// an authenticated R3, without claiming a calibrated physical insertion.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerReloadHoldStartTest(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment())return 1;
    bool expected=false;
    if(!vrequip::lancer_reload_test_start.compare_exchange_strong(expected,true))return 2;
    vrequip::lancer_reload_status.store(0,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerReloadHoldCompleteTest(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment())return 1;
    bool expected=false;
    if(!vrequip::lancer_reload_test_complete.compare_exchange_strong(expected,true))return 2;
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerReloadHoldStatus(void*){
    return vrequip::lancer_reload_status.load(std::memory_order_acquire);
}
#endif
#if defined(GEARS_PLAYGROUND_LANCER_CHAINSAW_HOLD_TEST)
// These exports exercise the original B/timer boundary only in an owned muted
// arena. A physical pull and blade contact are not yet connected to them.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerSawHoldStartTest(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment()||
       vrequip::lancer_saw_active_visible.load(std::memory_order_acquire)
#if defined(GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST)
       ||vrequip::lancer_saw_xr_visible.load(std::memory_order_acquire)
#endif
       )return 1;
    bool expected=false;
    if(!vrequip::lancer_saw_test_start.compare_exchange_strong(expected,true))return 2;
    vrequip::lancer_saw_status.store(1,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerSawHoldStopTest(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment())return 1;
    vrequip::lancer_saw_test_start.store(false,std::memory_order_release);
#if defined(GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST)
    vrequip::lancer_saw_xr_arm.store(false,std::memory_order_release);
    vrequip::lancer_saw_xr_stop.store(true,std::memory_order_release);
#endif
    vrequip::lancer_saw_test_stop.store(true,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerSawHoldStatus(void*){
    return vrequip::lancer_saw_status.load(std::memory_order_acquire)|
        (vrequip::lancer_saw_active_visible.load(std::memory_order_acquire)?0x10000u:0u);
}
#endif
#if defined(GEARS_PLAYGROUND_LANCER_BLADE_PROBE_TEST)
// Read-only original mesh and blade bones sampled after one PlayerTick.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerBladeProbeStart(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment())return 1;
    if(vrequip::lancer_blade_probe_requested.load(std::memory_order_acquire)||
       vrequip::mesh_follow_requested.load(std::memory_order_acquire)||
       vrequip::mesh_follow_active_visible.load(std::memory_order_acquire)||
       vrequip::support_probe_requested.load(std::memory_order_acquire)||
       vrequip::reload_visual_requested.load(std::memory_order_acquire)||
       vrequip::lancer_saw_active_visible.load(std::memory_order_acquire))return 2;
    unsigned expected=0;
    if(!vrequip::lancer_blade_probe_status.compare_exchange_strong(expected,1))return 3;
    vrequip::lancer_blade_probe_count.store(0,std::memory_order_release);
    vrequip::lancer_blade_probe_expected_hold.store(false,std::memory_order_release);
    vrequip::lancer_blade_probe_requested.store(true,std::memory_order_release);
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerBladeProbeStartHeld(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment()||
       !vrequip::lancer_saw_active_visible.load(std::memory_order_acquire))return 1;
    if(vrequip::lancer_blade_probe_requested.load(std::memory_order_acquire)||
       vrequip::mesh_follow_requested.load(std::memory_order_acquire)||
       vrequip::mesh_follow_active_visible.load(std::memory_order_acquire)||
       vrequip::support_probe_requested.load(std::memory_order_acquire)||
       vrequip::reload_visual_requested.load(std::memory_order_acquire))return 2;
    unsigned expected=0;
    if(!vrequip::lancer_blade_probe_status.compare_exchange_strong(expected,1))return 3;
    vrequip::lancer_blade_probe_count.store(0,std::memory_order_release);
    vrequip::lancer_blade_probe_expected_hold.store(true,std::memory_order_release);
    vrequip::lancer_blade_probe_requested.store(true,std::memory_order_release);
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerBladeProbeStatus(void*){
    return (vrequip::lancer_blade_probe_status.load(std::memory_order_acquire)<<16)|
        (vrequip::lancer_blade_probe_count.load(std::memory_order_acquire)&0xffffu);
}
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
// Bit 16: original mesh override active; bit 17: left support grip latched.
// 1 waiting for a held Lancer and fresh hand pose, 2 moving original mesh,
// 3 mesh unavailable, 4 invalid mount, 5 component call failed,
// 6 restored, 7 component destroyed, 8 restore failed.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundWeaponMeshFollowEnable(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment()||
       vrequip::mesh_follow_shutdown_requested.load(std::memory_order_acquire))return 1;
    if(vrequip::support_probe_requested.load(std::memory_order_acquire)||
       vrequip::reload_visual_requested.load(std::memory_order_acquire))return 2;
    vrequip::mesh_follow_requested.store(true,std::memory_order_release);
    vrequip::mesh_follow_status.store(1,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundWeaponMeshFollowDisable(void*){
    vrequip::mesh_follow_requested.store(false,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundWeaponMeshFollowStatus(void*){
    return (vrequip::mesh_follow_active_visible.load(std::memory_order_acquire)?0x10000u:0u)|
        (vrequip::mesh_follow_support_visible.load(std::memory_order_acquire)?0x20000u:0u)|
        vrequip::mesh_follow_status.load(std::memory_order_acquire);
}
// Synthetic game-thread transform check, isolated from real XR pose publication.
// One request runs 12 short positions and restores the original component.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundWeaponMeshFollowTrialStart(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment()||
       vrequip::mesh_follow_shutdown_requested.load(std::memory_order_acquire))return 1;
    if(vrequip::mesh_follow_requested.load(std::memory_order_acquire)||
       vrequip::mesh_follow_active_visible.load(std::memory_order_acquire)||
       vrequip::support_probe_requested.load(std::memory_order_acquire)||
       vrequip::reload_visual_requested.load(std::memory_order_acquire)||
       vrequip::mesh_follow_trial_requested.load(std::memory_order_acquire))return 2;
    unsigned expected=0;
    if(!vrequip::mesh_follow_trial_status.compare_exchange_strong(expected,1))return 3;
    vrequip::mesh_follow_trial_requested.store(true,std::memory_order_release);
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundWeaponMeshFollowTrialStatus(void*){
    return (vrequip::mesh_follow_trial_status.load(std::memory_order_acquire)<<16)|
        (vrequip::mesh_follow_trial_samples.load(std::memory_order_acquire)&0xffffu);
}
// Private read-only original-animation sample. 0 accepted, 1 wrong test
// environment, 2 candidate is moving the mesh, 3 already pending.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerSupportProbeEnable(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment())return 1;
    if(vrequip::mesh_follow_requested.load(std::memory_order_acquire)||
       vrequip::mesh_follow_active_visible.load(std::memory_order_acquire)||
       vrequip::reload_visual_requested.load(std::memory_order_acquire))return 2;
    unsigned expected=0;
    if(!vrequip::support_probe_status.compare_exchange_strong(expected,1))return 3;
    vrequip::support_probe_requested.store(true,std::memory_order_release);
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerSupportProbeStatus(void*){
    return (vrequip::support_probe_status.load(std::memory_order_acquire)<<16)|
        (vrequip::support_probe_count.load(std::memory_order_acquire)&0xffffu);
}
// The harness enables this before invoking the original ForceReload and then
// observes only one bounded, identity-checked original animation timeline.
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerReloadVisualProbeEnable(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment())return 1;
    if(vrequip::mesh_follow_requested.load(std::memory_order_acquire)||
       vrequip::mesh_follow_active_visible.load(std::memory_order_acquire)||
       vrequip::support_probe_requested.load(std::memory_order_acquire))return 2;
    unsigned expected=0;
    if(!vrequip::reload_visual_status.compare_exchange_strong(expected,1))return 3;
    vrequip::reload_visual_requested.store(true,std::memory_order_release);
    return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerReloadVisualProbeStatus(void*){
    return (vrequip::reload_visual_status.load(std::memory_order_acquire)<<16)|
        (vrequip::reload_visual_count.load(std::memory_order_acquire)&0xffffu);
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerMagazineSocketProbeEnable(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment())return 1;
    if(vrequip::mesh_follow_requested.load(std::memory_order_acquire)||
       vrequip::mesh_follow_active_visible.load(std::memory_order_acquire)||
       vrequip::support_probe_requested.load(std::memory_order_acquire)||
       vrequip::reload_visual_requested.load(std::memory_order_acquire))return 2;
    unsigned expected=0;
    if(!vrequip::lancer_socket_status.compare_exchange_strong(expected,1))return 3;
    vrequip::lancer_socket_reload_mode.store(false,std::memory_order_release);
    vrequip::mesh_follow_trial_requested.store(false,std::memory_order_release);
    vrequip::lancer_socket_requested.store(true,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerMagazineSocketReloadProbeEnable(void*){
    if(!enabled.load(std::memory_order_acquire)||!playground::automatic||
       !playground::ready||!vrequip::test_environment())return 1;
    if(vrequip::mesh_follow_requested.load(std::memory_order_acquire)||
       vrequip::mesh_follow_active_visible.load(std::memory_order_acquire)||
       vrequip::support_probe_requested.load(std::memory_order_acquire)||
       vrequip::reload_visual_requested.load(std::memory_order_acquire))return 2;
    unsigned expected=0;
    if(!vrequip::lancer_socket_status.compare_exchange_strong(expected,1))return 3;
    vrequip::lancer_socket_reload_mode.store(true,std::memory_order_release);
    vrequip::lancer_socket_requested.store(true,std::memory_order_release);return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundLancerMagazineSocketProbeStatus(void*){
    return (vrequip::lancer_socket_status.load(std::memory_order_acquire)<<16)|
           (vrequip::lancer_socket_count.load(std::memory_order_acquire)&0xffffu);
}
#endif
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundTestEnter(void*){
    // Available only in the user's explicit unattended, muted test child.
    wchar_t behind[4]{},muted[4]{};
    if(GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind,4)!=1||behind[0]!=L'1'||
       GetEnvironmentVariableW(L"GEARSVR_TEST_MUTE_GAME",muted,4)!=1||muted[0]!=L'1')return 1;
    if(!enabled.load()||!playground::automatic)return 2;
    playground::test_enter_pending=true;return 0;
}
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_R2)
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundVrFireEnable(void*){
    return vrfire::enable();
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundVrFireDisable(void*){
    return vrfire::disable();
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundVrFireStatus(void*){
    return (vrfire::mode.load(std::memory_order_acquire)<<16)|
           vrfire::last_status.load(std::memory_order_acquire);
}
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2)
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundVrAimStatus(void*){
    return (vrfire::mode.load(std::memory_order_acquire)<<16)|
           vrfire::last_aim_status.load(std::memory_order_acquire);
}
#endif
#endif
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundShutdown(void*){
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    // Do not acquire the lifecycle lock while the other caller may be holding
    // it and waiting for this game-thread callback to return.
    if(game_thread.load(std::memory_order_acquire)==GetCurrentThreadId()){
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
        handstream::shutdown_requested.store(true,std::memory_order_release);
        handstream::stop_requested.store(true,std::memory_order_release);
#endif
        vrequip::stow::shutdown_requested.store(true,std::memory_order_release);
        vrequip::stow::stop_requested.store(true,std::memory_order_release);return 51;
    }
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    if(game_thread.load(std::memory_order_acquire)==GetCurrentThreadId()){
        handstream::shutdown_requested.store(true,std::memory_order_release);
        handstream::stop_requested.store(true,std::memory_order_release);return 41;
    }
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
    if(game_thread.load(std::memory_order_acquire)==GetCurrentThreadId()){
        handikcontrolled::shutdown_requested.store(true,std::memory_order_release);
        handikcontrolled::stop_requested.store(true,std::memory_order_release);return 41;
    }
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_TRIAL)
    if(game_thread.load(std::memory_order_acquire)==GetCurrentThreadId()){
        handik::shutdown_requested.store(true,std::memory_order_release);
        handik::stop_requested.store(true,std::memory_order_release);return 41;
    }
#endif
    LifecycleScope lifecycle;
#if defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL)
    // Stop both producers before either drain. Hand restoration precedes the
    // attachment helper, including the game-thread cleanup path.
    handstream::shutdown_requested.store(true,std::memory_order_release);
    handstream::stop_requested.store(true,std::memory_order_release);
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    vrequip::stow::shutdown_requested.store(true,std::memory_order_release);
    vrequip::stow::stop_requested.store(true,std::memory_order_release);
    const auto stow_until=GetTickCount64()+2000;
    while((vrequip::stow::dirty_visible.load(std::memory_order_acquire)||
           vrequip::stow::pending_visible.load(std::memory_order_acquire)||callbacks.load(std::memory_order_acquire))&&
          GetTickCount64()<stow_until)Sleep(1);
    if(vrequip::stow::dirty_visible.load(std::memory_order_acquire)||
       vrequip::stow::pending_visible.load(std::memory_order_acquire)||callbacks.load(std::memory_order_acquire))return 50;
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    handstream::shutdown_requested.store(true,std::memory_order_release);
    if(const auto pending_restore=handstream::stop_and_wait())return pending_restore;
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
    handikcontrolled::shutdown_requested.store(true,std::memory_order_release);
    if(const auto pending_restore=handikcontrolled::stop_and_wait())return pending_restore;
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_TRIAL)
    handik::shutdown_requested.store(true,std::memory_order_release);
    if(const auto pending_restore=handik::stop_and_wait())return pending_restore;
#endif
#if defined(GEARS_PLAYGROUND_LANCER_CHAINSAW_HOLD_TEST)
    vrequip::lancer_saw_test_start.store(false,std::memory_order_release);
#if defined(GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST)
    vrequip::lancer_saw_xr_arm.store(false,std::memory_order_release);
    vrequip::lancer_saw_xr_stop.store(true,std::memory_order_release);
#endif
    if(vrequip::lancer_saw_active_visible.load(std::memory_order_acquire)
#if defined(GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST)
       ||vrequip::lancer_saw_xr_visible.load(std::memory_order_acquire)
#endif
       ){
        vrequip::lancer_saw_test_stop.store(true,std::memory_order_release);
        const auto stop_until=GetTickCount64()+2000;
        while(GetTickCount64()<stop_until){
            if(!vrequip::lancer_saw_active_visible.load(std::memory_order_acquire)
#if defined(GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST)
               &&!vrequip::lancer_saw_xr_visible.load(std::memory_order_acquire)
#endif
               )break;
            Sleep(1);
        }
        if(vrequip::lancer_saw_active_visible.load(std::memory_order_acquire)
#if defined(GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST)
           ||vrequip::lancer_saw_xr_visible.load(std::memory_order_acquire)
#endif
           )return 30;
    }
#endif
#if defined(GEARS_PLAYGROUND_EXPERIMENTAL_VR_R2)
    if(const auto stopped=vrfire::disable())return 10+stopped;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
    vrequip::support_probe_requested.store(false,std::memory_order_release);
    vrequip::reload_visual_requested.store(false,std::memory_order_release);
    vrequip::lancer_socket_requested.store(false,std::memory_order_release);
    vrequip::lancer_socket_reload_mode.store(false,std::memory_order_release);
    vrequip::mesh_follow_shutdown_requested.store(true,std::memory_order_release);
    vrequip::mesh_follow_requested.store(false,std::memory_order_release);
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
    vrequip::lancer_magazine_test_request.store(false,std::memory_order_release);
    vrequip::lancer_magazine_insert_test_request.store(false,std::memory_order_release);
#endif
    const auto follow_until=GetTickCount64()+2000;
    while(callbacks.load(std::memory_order_acquire)&&GetTickCount64()<follow_until)Sleep(1);
    if(callbacks.load(std::memory_order_acquire)||
       vrequip::mesh_follow_active_visible.load(std::memory_order_acquire)
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
       ||vrequip::lancer_magazine_active_visible.load(std::memory_order_acquire)
#endif
       )
        return 20; // Game thread must restore first.
#endif
    enabled.store(false,std::memory_order_release);
    if(auto r=pending.exchange(nullptr))InterlockedExchange(&r->state,-2);
    if(hook_created){
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
        const auto disabled=MH_DisableHook(MH_ALL_HOOKS);
#else
        const auto disabled=MH_DisableHook(reinterpret_cast<void*>(base+0x1d8dd8));
#endif
        if(disabled!=MH_OK&&disabled!=MH_ERROR_DISABLED)return 1;
    }
    const auto until=GetTickCount64()+2000;
    while(callbacks.load()&&GetTickCount64()<until)Sleep(1);
    if(callbacks.load())return 2;
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
    // All callbacks are drained. Finalize the read-only observation before
    // closing its log; no game-thread cleanup or engine call is required.
    if(handanim::probe.active)handanim::finish(4,L"shutdown");
    handanim::reset();
#endif
#if defined(GEARS_PLAYGROUND_FREE_HANDS_PROBE) || defined(GEARS_PLAYGROUND_HAND_IK_HELPERS)
    freehands::reset();
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_TRIAL)
    handik::reset();
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
    handikcontrolled::reset();
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    handstream::reset();
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST)
    vrequip::reset_switch_test();
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    vrequip::stow::reset();
#endif
#if defined(GEARS_PLAYGROUND_LANCER_RELOAD_HOLD_TEST)
    vrequip::lancer_reload_test_start=false;
    vrequip::lancer_reload_test_complete=false;
    vrequip::lancer_reload_hold={};
#endif
#if defined(GEARS_PLAYGROUND_LANCER_CHAINSAW_HOLD_TEST)
    vrequip::lancer_saw_test_start=false;
    vrequip::lancer_saw_test_stop=false;
    vrequip::lancer_saw_hold={};
    vrequip::lancer_saw_status.store(0,std::memory_order_release);
#endif
#if defined(GEARS_PLAYGROUND_LANCER_BLADE_PROBE_TEST)
    vrequip::lancer_blade_probe_requested=false;
    vrequip::lancer_blade_probe_expected_hold=false;
    vrequip::lancer_blade_probe={};
    vrequip::lancer_blade_probe_status.store(0,std::memory_order_release);
    vrequip::lancer_blade_probe_count.store(0,std::memory_order_release);
#endif
#if defined(GEARS_PLAYGROUND_LANCER_SAW_GESTURE_TRIAL)
    vrequip::lancer_saw_gesture_trial_requested=false;
    vrequip::lancer_saw_gesture_trial={};
    vrequip::lancer_saw_gesture_trial_status.store(0,std::memory_order_release);
#endif
#if defined(GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST)
    vrequip::lancer_saw_xr_arm=false;
    vrequip::lancer_saw_xr_stop=false;
    vrequip::lancer_saw_xr={};
    vrequip::lancer_saw_xr_visible.store(false,std::memory_order_release);
    vrequip::lancer_saw_xr_status.store(0,std::memory_order_release);
#endif
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
    vrequip::lancer_magazine_test_active=false;
    vrequip::lancer_magazine_test_insert_mode=false;
    vrequip::lancer_magazine_test_pose={};
    vrequip::lancer_magazine_status.store(0,std::memory_order_release);
#endif
    if(hook_created){
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
        if(!handanimcall::remove())return 33;
#endif
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
