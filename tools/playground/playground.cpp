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
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
#if !defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL) || !defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
#error Fire-invocation scope requires the shared-hand client and existing CallFunction owner
#endif
#include "../vr-common/weapon_shot_scope.h"
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
#if !defined(GEARS_PLAYGROUND_EQUIPMENT_OBSERVE)
#error Shared weapon-hand participant requires the latched equipment model
#endif
#include <chrono>
#include <cwchar>
#include <cstddef>
#include <type_traits>
#include "../vr-common/weapon_hand_client.h"
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_WRITE_TRIAL)
#include <cwchar>
#include <initializer_list>
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_WRITE_TRIAL) && !defined(GEARS_PLAYGROUND_WEAPON_GRIP_CALL_CONTEXT_PROBE)
#error Damage write trial requires the existing Result and caller context observers
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_CALL_CONTEXT_PROBE) && !defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
#error Damage caller context requires the existing raw Result observer
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
#if !defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE) || !defined(GEARS_PLAYGROUND_HAND_ANIMATION_CALL_PROBE)
#error Damage Result observer requires both existing animation observation boundaries
#endif
#include <cstddef>
#include <type_traits>
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL) || !defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) || !defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL) || !defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL) || !defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL) || !defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
#error Continuous component requires certified combined continuous phase1 hands and excludes HOLD
#endif
#include "../vr-common/weapon_component_continuous_lease.h"
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
#if !defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) || !defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL) || !defined(GEARS_PLAYGROUND_HAND_EQUIPMENT_COMBINED_TRIAL) || !defined(GEARS_PLAYGROUND_HAND_CONTINUOUS_SESSION_TRIAL)
#error Component HOLD requires certified combined continuous phase1 hands
#endif
#endif
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL) && (!defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL) || !defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL))
#error Hand tail translation requires persistent certified hand streaming
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
#include "../vr-common/hand_ik_stream_lease.h"
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
#if !defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL)
#error Hand tail translation requires the fixed-instance persistent component session
#endif
#include "../vr-common/hand_ik_tail_lease.h"
#include "hand_tail_compare_write.h"
#include <new>
#endif
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
#include "../vr-common/weapon_mesh_component_lease.h"
#include "../vr-common/weapon_mesh_follow.h"
#include "../vr-common/weapon_component_rotation.h"
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
// Scoped parsing algorithm only. No metadata, object, value or permission is
// cached; native invocation and its recursive callbacks explicitly opt out.
namespace component_names {
thread_local bool enabled{};
struct Scope {
    bool saved;
    explicit Scope(bool value) noexcept:saved(enabled){enabled=value;}
    ~Scope(){enabled=saved;}
    Scope(Scope const&)=delete;
    Scope& operator=(Scope const&)=delete;
};
std::size_t query_limit(const wchar_t* value){
    if(!value)return 0;
    std::size_t len{};while(len<127&&value[len])++len;
    return len<127?len+1:128;
}
std::size_t page_bytes(){
    static const std::size_t bytes=[](){SYSTEM_INFO info{};GetSystemInfo(&info);return std::size_t(info.dwPageSize);}();
    return bytes;
}
bool text(uptr from,wchar_t* out,size_t count){
    const auto page=page_bytes();if(!page)return false;
    for(size_t at=0;at<count;){
        const auto address=from+at*sizeof(wchar_t);
        const auto room=page-std::size_t(address%page);
        auto chars=(std::min)(count-at,room/sizeof(wchar_t));
        // Preserve the old two-byte copy for a wchar straddling the page.
        if(!chars)chars=1;
        if(!copy(out+at,address,chars*sizeof(wchar_t)))return false;
        for(size_t i=0;i<chars;++i)if(!out[at+i])return true;
        at+=chars;
    }
    out[count-1]=0;return false;
}
} // namespace component_names
// Three original nested row spans and fixed component/getter aggregates only.
// These clocks and results never grant authority; no hot-path text is written.
namespace component_cost {
enum class Kind:unsigned {initial,snapshot,name_scan};
constexpr unsigned capacity=64;
enum class AggregateKind:unsigned {current_total,read_total,slot_args,slot_preflight,slot_vm_run,slot_postresult,count};
constexpr unsigned aggregate_groups=4;
constexpr unsigned aggregate_capacity=unsigned(AggregateKind::count)*aggregate_groups;
struct Aggregate {
    std::uint64_t count{},valid_count{},invalid_clock{},sum{},maximum{};
    bool saturated{};
};
struct Row {
    Kind kind{};unsigned group{},lease{};
    std::uint64_t sequence{},revision{},begin{},finish{};
    int names_count{},hit_index{};
    bool closed{},clock_known{},result_known{},result{},count_known{},hit_known{};
};
struct State {
    std::array<Row,capacity> rows{};
    std::array<Aggregate,aggregate_capacity> aggregates{};
    unsigned count{},open{},initial_open{},lease{};
    std::uint64_t frequency{},sequence{},revision{};
    bool enabled{},pending{},frozen{},full{},dumped{};
};
State state;
thread_local bool invoke_paused{},snapshot_owner{};
thread_local unsigned snapshot_group{};
thread_local bool aggregate_owner{},slot_owner{};
thread_local unsigned aggregate_group{};
std::uint64_t stamp() noexcept {
    LARGE_INTEGER value{};
    return QueryPerformanceCounter(&value)&&value.QuadPart>0?std::uint64_t(value.QuadPart):0;
}
void reset(){
    state={};snapshot_owner=invoke_paused=aggregate_owner=slot_owner=false;
    snapshot_group=aggregate_group=0;
}
void begin(std::uint64_t sequence,std::uint64_t revision,unsigned lease){
    if(invoke_paused||state.pending||state.frozen||state.dumped||state.open)return;
    if(!state.enabled){
        state.enabled=true;LARGE_INTEGER frequency{};
        if(QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0)state.frequency=std::uint64_t(frequency.QuadPart);
    }
    state.sequence=sequence;state.revision=revision;state.lease=lease;
}
void freeze_if_closed(){if(state.pending&&!state.open)state.frozen=true;}
void failure(bool forward,bool faulted){
    if(invoke_paused||faulted||(!forward&&!state.initial_open)||!state.enabled||state.pending||state.frozen||state.dumped)return;
    state.pending=true;freeze_if_closed();
}
// Saturation keeps a long diagnostic session bounded without wrapped totals.
void aggregate_add(std::uint64_t& target,std::uint64_t value,bool& saturated){
    constexpr auto limit=~std::uint64_t{};
    if(value>limit-target){target=limit;saturated=true;}else target+=value;
}
struct Owner {
    bool saved_owner,saved_slot;unsigned saved_group;
    Owner(unsigned group,bool eligible):saved_owner(aggregate_owner),saved_slot(slot_owner),saved_group(aggregate_group){
        aggregate_owner=eligible&&!invoke_paused;aggregate_group=aggregate_owner?group:0;slot_owner=false;
    }
    ~Owner(){aggregate_owner=saved_owner;slot_owner=saved_slot;aggregate_group=saved_group;}
    Owner(Owner const&)=delete;Owner& operator=(Owner const&)=delete;
};
struct SlotOwner {
    bool saved;
    SlotOwner():saved(slot_owner){slot_owner=aggregate_owner&&!invoke_paused;}
    ~SlotOwner(){slot_owner=saved;}
    SlotOwner(SlotOwner const&)=delete;SlotOwner& operator=(SlotOwner const&)=delete;
};
struct AggregateSpan {
    unsigned index=aggregate_capacity;std::uint64_t begin{};
    explicit AggregateSpan(AggregateKind kind,bool eligible=true){
        // No owner on other threads or in the ordinary/default equipment path.
        if(!eligible||!aggregate_owner||invoke_paused)return;
        if(!state.enabled||state.pending||state.frozen||state.dumped)return;
        const unsigned group=aggregate_group==0?0:aggregate_group==1?1:aggregate_group==2?2:aggregate_group==4?3:aggregate_groups;
        if(unsigned(kind)>=unsigned(AggregateKind::count)||group==aggregate_groups)return;
        index=unsigned(kind)*aggregate_groups+group;begin=stamp();++state.open;
    }
    void close(){
        if(index==aggregate_capacity)return;
        const auto finish=stamp();auto& aggregate=state.aggregates[index];
        aggregate_add(aggregate.count,1,aggregate.saturated);
        if(state.frequency&&begin&&finish>=begin){
            const auto ticks=finish-begin;
            aggregate_add(aggregate.valid_count,1,aggregate.saturated);
            aggregate_add(aggregate.sum,ticks,aggregate.saturated);
            aggregate.maximum=(std::max)(aggregate.maximum,ticks);
        }else aggregate_add(aggregate.invalid_clock,1,aggregate.saturated);
        --state.open;index=aggregate_capacity;freeze_if_closed();
    }
    ~AggregateSpan(){close();}
    AggregateSpan(AggregateSpan const&)=delete;AggregateSpan& operator=(AggregateSpan const&)=delete;
};
struct Span {
    unsigned index=capacity;
    explicit Span(Kind kind,unsigned group,bool eligible=true){
        // Callers on other threads have no active snapshot owner. Return before
        // touching the game-thread-only aggregate when the owner is absent.
        if(!eligible)return;
        if(invoke_paused||!state.enabled||state.pending||state.frozen||state.dumped)return;
        if(state.count==capacity){state.full=true;return;}
        index=state.count++;
        auto& row=state.rows[index];row.kind=kind;row.group=group;row.lease=state.lease;
        row.sequence=state.sequence;row.revision=state.revision;row.begin=stamp();
        ++state.open;if(kind==Kind::initial)++state.initial_open;
    }
    void close(bool result_known,bool result){
        if(index==capacity)return;
        auto& row=state.rows[index];row.finish=stamp();row.closed=true;
        row.clock_known=state.frequency&&row.begin&&row.finish>=row.begin;
        row.result_known=result_known;row.result=result;
        --state.open;if(row.kind==Kind::initial)--state.initial_open;
        index=capacity;freeze_if_closed();
    }
    void finish(bool result){close(true,result);}
    ~Span(){close(false,false);}
    Span(Span const&)=delete;Span& operator=(Span const&)=delete;
};
struct InvokePause {
    bool saved;
    InvokePause():saved(invoke_paused){invoke_paused=true;}
    ~InvokePause(){invoke_paused=saved;}
    InvokePause(InvokePause const&)=delete;InvokePause& operator=(InvokePause const&)=delete;
};
struct SnapshotOwner {
    bool saved;unsigned saved_group;
    SnapshotOwner(unsigned group,bool eligible):saved(snapshot_owner),saved_group(snapshot_group){
        snapshot_owner=eligible&&!invoke_paused;snapshot_group=snapshot_owner?group:0;
    }
    ~SnapshotOwner(){snapshot_owner=saved;snapshot_group=saved_group;}
    SnapshotOwner(SnapshotOwner const&)=delete;SnapshotOwner& operator=(SnapshotOwner const&)=delete;
};
template<class F>bool snapshot(unsigned group,bool forward,bool faulted,F&& call){
    SnapshotOwner owner(group,forward&&!faulted);
    Span span(Kind::snapshot,group,snapshot_owner);
    const bool result=call();span.finish(result);return result;
}
struct Scan {
    Span span;bool found{};
    explicit Scan(int count):span(Kind::name_scan,snapshot_group,snapshot_owner&&!invoke_paused){
        if(span.index!=capacity){auto& row=state.rows[span.index];row.count_known=true;row.names_count=count;}
    }
    void hit(int index){
        found=true;
        if(span.index!=capacity){auto& row=state.rows[span.index];row.hit_known=true;row.hit_index=index;}
    }
    ~Scan(){span.finish(found);}
    Scan(Scan const&)=delete;Scan& operator=(Scan const&)=delete;
};
void dump(bool clean){
    if(!clean||invoke_paused||!state.enabled||state.dumped||state.open||!log_file)return;
    state.dumped=true;
    fwprintf(log_file,L"hand_weapon_component_cost_begin schema=1 capacity=%u count=%u frequency=%llu "
        L"pending=%u frozen=%u full=%u nested_not_additive=1 extra_getters=0\n",capacity,state.count,state.frequency,
        unsigned(state.pending),unsigned(state.frozen),unsigned(state.full));
    for(unsigned i=0;i<state.count;++i){const auto& row=state.rows[i];
        fwprintf(log_file,L"hand_weapon_component_cost schema=1 row=%u kind=%u group=%u lease=%u sequence=%llu revision=%llu "
            L"begin_qpc=%llu finish_qpc=%llu closed=%u clock_known=%u result_known=%u result=%u "
            L"names_count_known=%u names_count=%d hit_known=%u hit_index=%d\n",i,unsigned(row.kind),row.group,row.lease,
            row.sequence,row.revision,row.begin,row.finish,unsigned(row.closed),unsigned(row.clock_known),
            unsigned(row.result_known),unsigned(row.result),unsigned(row.count_known),row.names_count,unsigned(row.hit_known),row.hit_index);
    }
    constexpr const wchar_t* aggregate_names[]={L"current_total",L"read_total",L"slot_args",L"slot_preflight",L"slot_vm_run",L"slot_postresult"};
    constexpr unsigned groups[]={0,1,2,4};
    for(unsigned i=0;i<aggregate_capacity;++i){const auto& aggregate=state.aggregates[i];
        if(!aggregate.count)continue;
        // slot_vm_run is the complete existing Args::run/invoke boundary,
        // including its guards/wrappers; it is not a pure native-body timer.
        fwprintf(log_file,L"hand_weapon_component_cost_aggregate schema=1 kind=%ls group=%u count=%llu valid_count=%llu "
            L"invalid_clock=%llu sum_qpc=%llu max_qpc=%llu frequency=%llu saturated=%u nested_not_additive=1 "
            L"vm_boundary_args_run_invoke=%u extra_getters=0\n",aggregate_names[i/aggregate_groups],groups[i%aggregate_groups],
            aggregate.count,aggregate.valid_count,aggregate.invalid_clock,aggregate.sum,aggregate.maximum,state.frequency,
            unsigned(aggregate.saturated),unsigned(i/aggregate_groups==unsigned(AggregateKind::slot_vm_run)));
    }
    fwprintf(log_file,L"hand_weapon_component_cost_end schema=1\n");fflush(log_file);
}
} // namespace component_cost
#endif
bool name(uptr object,wchar_t* out,size_t count){
    if(!object||!count)return false;
    const auto n=get<Name>(object+0x48);const auto a=get<Array>(base+0x1c34800);
    if(!a.data||a.count<1000||a.count>a.capacity||a.capacity>4000000||n.index>=unsigned(a.count))return false;
    const auto entry=get<uptr>(a.data+n.index*8);
    if(!entry||get<unsigned>(entry)!=n.index)return false;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(component_names::enabled)return component_names::text(entry+20,out,count);
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_names::Scope names_scope(false);
    component_cost::InvokePause cost_pause;
#endif
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
// Revision 2. Insert after unchanged invoke(Request*) and before playground_controller.inl.
// Only the three supported component setters; old revision 4ebb was not tested.
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)

bool component_forward_read_bytes(uptr object,uptr offset,void* output,std::size_t bytes) noexcept {
    constexpr uptr maximum=~uptr{0};
    return object&&bytes&&offset<=maximum-object&&bytes-1<=maximum-(object+offset)&&
        copy(output,object+offset,bytes);
}
template<class T> bool component_forward_read_at(uptr object,uptr offset,T& value) noexcept {
    return component_forward_read_bytes(object,offset,&value,sizeof(value));
}
bool component_forward_registered(uptr object,unsigned& index) noexcept {
    Array objects{};uptr found{};
    return component_forward_read_at(base,0x1c24730,objects)&&
        objects.data&&objects.count>1000&&objects.count<=objects.capacity&&objects.capacity<4000000&&
        component_forward_read_at(object,8,index)&&index<unsigned(objects.count)&&
        component_forward_read_at(objects.data,uptr(index)*8,found)&&found==object;
}

struct ComponentPreparedObject {
    uptr address{},klass{},outer{};
    unsigned index{},reserved{};
    Name name{};
    bool read(uptr object) noexcept {
        address=object;reserved=0;unsigned class_index{},outer_index{},last_index{};
        return component_forward_registered(object,index)&&
            component_forward_read_at(object,0x40,outer)&&
            component_forward_read_at(object,0x48,name)&&
            component_forward_read_at(object,0x50,klass)&&
            component_forward_registered(klass,class_index)&&component_forward_registered(outer,outer_index)&&
            component_forward_registered(object,last_index)&&last_index==index;
    }
    bool same(ComponentPreparedObject const& other) const noexcept {
        return address==other.address&&klass==other.klass&&outer==other.outer&&index==other.index&&
            reserved==other.reserved&&name.index==other.name.index&&name.number==other.name.number;
    }
};
template<std::size_t N>
bool component_forward_named(ComponentPreparedObject const& object,wchar_t const (&expected)[N]) noexcept {
    static_assert(N>0&&N<=96);
    Array names{};uptr entry{};unsigned index{};
    if(!component_forward_read_at(base,0x1c34800,names)||!names.data||names.count<1000||
       names.count>names.capacity||names.capacity>4000000||object.name.index>=unsigned(names.count)||
       !component_forward_read_at(names.data,uptr(object.name.index)*8,entry)||!entry||
       !component_forward_read_at(entry,0,index)||index!=object.name.index)return false;
    wchar_t label[N]{};
    for(std::size_t i=0;i<N;++i)
        if(!component_forward_read_at(entry,20+uptr(i)*sizeof(wchar_t),label[i]))return false;
    return !std::memcmp(label,expected,sizeof(label)); // Includes the real terminating NUL.
}
enum class ComponentPreparedKind {boolean,structure,real,integer};
bool component_forward_kind(ComponentPreparedObject const& type,ComponentPreparedKind kind) noexcept {
    if(kind==ComponentPreparedKind::boolean)return component_forward_named(type,L"BoolProperty");
    if(kind==ComponentPreparedKind::structure)return component_forward_named(type,L"StructProperty");
    if(kind==ComponentPreparedKind::real)return component_forward_named(type,L"FloatProperty");
    return component_forward_named(type,L"IntProperty");
}

// 0x70..0x98 is the existing preflight's exact common property metadata range.
// Typed +0xd0 reads are separate: 4 bytes for BoolProperty, 8 for StructProperty.
struct ComponentPreparedField {
    ComponentPreparedObject identity{},type{};
    uptr next{};
    unsigned char metadata[0x28]{};
    unsigned dimension{},element_bytes{},offset{};
    unsigned long long flags{};
    bool read(uptr property,ComponentPreparedKind kind) noexcept {
        if(!identity.read(property)||!type.read(identity.klass)||!component_forward_kind(type,kind)||
           !component_forward_read_bytes(property,0x70,metadata,sizeof(metadata))||
           !component_forward_read_at(property,0x68,next))return false;
        std::memcpy(&dimension,metadata,4);std::memcpy(&element_bytes,metadata+4,4);
        std::memcpy(&flags,metadata+8,8);std::memcpy(&offset,metadata+0x24,4);
        return true;
    }
    bool same(ComponentPreparedField const& other) const noexcept {
        return identity.same(other.identity)&&type.same(other.type)&&next==other.next&&
            !std::memcmp(metadata,other.metadata,sizeof(metadata));
    }
};
struct ComponentPreparedProperty:ComponentPreparedField {
    unsigned bool_mask{};
    uptr struct_target{};
    bool read(uptr property,bool boolean) noexcept {
        if(!ComponentPreparedField::read(property,boolean?ComponentPreparedKind::boolean:ComponentPreparedKind::structure))
            return false;
        if(boolean)return component_forward_read_at(property,0xd0,bool_mask)&&bool_mask&&!(bool_mask&(bool_mask-1));
        return component_forward_read_at(property,0xd0,struct_target)&&struct_target;
    }
    bool same(ComponentPreparedProperty const& other) const noexcept {
        return ComponentPreparedField::same(other)&&bool_mask==other.bool_mask&&struct_target==other.struct_target;
    }
};
struct ComponentPreparedStructNode {
    ComponentPreparedObject identity{},type{};
    uptr super{},head{};
    unsigned bytes{};
    bool read(uptr structure) noexcept {
        return identity.read(structure)&&type.read(identity.klass)&&
            component_forward_read_at(structure,0x60,super)&&
            component_forward_read_at(structure,0x80,head)&&
            component_forward_read_at(structure,0x88,bytes);
    }
    bool same(ComponentPreparedStructNode const& other) const noexcept {
        return identity.same(other.identity)&&type.same(other.type)&&super==other.super&&head==other.head&&bytes==other.bytes;
    }
};
// Bounded metadata description only for Vector/Rotator's three four-byte fields.
// Parent traversal uses the same +0x60/+0x80/+0x68 links as playground::field.
// No unknown/extra member or incomplete traversal is accepted as a layout.
struct ComponentPreparedTriple {
    ComponentPreparedStructNode parents[40]{};
    ComponentPreparedField members[3]{};
    std::size_t parent_count{},member_count{};
    unsigned member_mask{};
    bool read(uptr structure,bool rotation) noexcept {
        auto current=structure;
        while(current){
            if(parent_count>=40||!parents[parent_count].read(current))return false;
            auto const& parent=parents[parent_count];
            if(!parent_count&&(parent.bytes!=12||
               !(rotation?component_forward_named(parent.identity,L"Rotator"):component_forward_named(parent.identity,L"Vector"))))
                return false;
            auto field=parent.head;
            while(field){
                if(member_count>=3||!members[member_count].read(field,
                   rotation?ComponentPreparedKind::integer:ComponentPreparedKind::real))return false;
                auto const& member=members[member_count];
                unsigned ordinal=3;
                if(rotation){
                    if(component_forward_named(member.identity,L"Pitch"))ordinal=0;
                    else if(component_forward_named(member.identity,L"Yaw"))ordinal=1;
                    else if(component_forward_named(member.identity,L"Roll"))ordinal=2;
                }else{
                    if(component_forward_named(member.identity,L"X"))ordinal=0;
                    else if(component_forward_named(member.identity,L"Y"))ordinal=1;
                    else if(component_forward_named(member.identity,L"Z"))ordinal=2;
                }
                if(ordinal>=3||(member_mask&(1u<<ordinal))||member.dimension!=1||
                   member.element_bytes!=4||member.offset!=ordinal*4)return false;
                member_mask|=1u<<ordinal;field=member.next;++member_count;
            }
            current=parent.super;++parent_count;
        }
        return parent_count&&member_count==3&&member_mask==7;
    }
    bool same(ComponentPreparedTriple const& other) const noexcept {
        if(parent_count!=other.parent_count||member_count!=other.member_count||member_mask!=other.member_mask)return false;
        for(std::size_t i=0;i<parent_count;++i)if(!parents[i].same(other.parents[i]))return false;
        for(std::size_t i=0;i<member_count;++i)if(!members[i].same(other.members[i]))return false;
        return true;
    }
};

struct ComponentPreparedCall {
    Request const* request{};
    uptr module_base{},field_head{},native_function{};
    Event process_event{};
    ComponentPreparedObject object{},function{},function_class{};
    unsigned size{},group{};
    unsigned short parameter_bytes{},native_index{},return_offset{};
    unsigned char parameters[12]{};
    unsigned char function_header[0x120]{};
    ComponentPreparedProperty fields[3]{};
    ComponentPreparedTriple triple{};
    std::size_t field_count{};
    bool ready{};

    // A descriptor is comparison evidence, never permission. matches() must run
    // after the callback's LAST VM and before its original final clock checks.
    bool prepare(Request const& value) noexcept {
        if(request||ready)return false;
        request=&value;
        uptr object_address{},function_address{};
        if(!copy(&module_base,reinterpret_cast<uptr>(&base),sizeof(module_base))||
           !module_base||module_base>(~uptr{0})-0x1c34800||
           !copy(&process_event,reinterpret_cast<uptr>(&original),sizeof(process_event))||!process_event||
           !copy(&size,reinterpret_cast<uptr>(&value.size),sizeof(size))||size!=sizeof(parameters)||
           !copy(&object_address,reinterpret_cast<uptr>(&value.object),sizeof(object_address))||
           !copy(&function_address,reinterpret_cast<uptr>(&value.function),sizeof(function_address))||
           !copy(parameters,reinterpret_cast<uptr>(value.params),sizeof(parameters))||
           !object.read(object_address)||!function.read(function_address)||
           !function_class.read(function.klass)||!component_forward_named(function_class,L"Function"))return false;
        if(component_forward_named(function,L"SetAbsolute"))group=1;
        else if(component_forward_named(function,L"SetTranslation"))group=2;
        else if(component_forward_named(function,L"SetRotation"))group=4;
        else return false;
        // Full 0x120 reads are evidenced for these three actual setter instances
        // by MeshPreflight.function_snapshot and the sealed AA setter schemas.
        if(!component_forward_read_bytes(function_address,0,function_header,sizeof(function_header)))return false;
        std::memcpy(&parameter_bytes,function_header+0x106,2);
        std::memcpy(&native_index,function_header+0xf8,2);
        std::memcpy(&return_offset,function_header+0x108,2);
        std::memcpy(&native_function,function_header+0x114,8);
        std::memcpy(&field_head,function_header+0x80,8);
        unsigned header_index{};Name header_name{};uptr header_outer{},header_class{};
        std::memcpy(&header_index,function_header+8,4);
        std::memcpy(&header_outer,function_header+0x40,8);
        std::memcpy(&header_name,function_header+0x48,8);
        std::memcpy(&header_class,function_header+0x50,8);
        if(header_index!=function.index||header_outer!=function.outer||header_class!=function.klass||
           header_name.index!=function.name.index||header_name.number!=function.name.number||
           parameter_bytes!=12||return_offset!=0xffff||native_function<module_base+0x1000||
           native_function>=module_base+0x131b000)return false;
        const std::size_t expected_fields=group==1?3:1;
        auto field=field_head;
        while(field){
            if(field_count>=expected_fields||!fields[field_count].read(field,group==1))return false;
            auto const& item=fields[field_count];
            if(item.dimension!=1||(item.flags&0x580)!=0x80||
               item.element_bytes!=(group==1?4u:12u)||item.offset!=(group==1?unsigned(field_count)*4:0u))return false;
            if(group==1){
                const bool named=field_count==0?component_forward_named(item.identity,L"NewAbsoluteTranslation"):
                    field_count==1?component_forward_named(item.identity,L"NewAbsoluteRotation"):
                    component_forward_named(item.identity,L"NewAbsoluteScale");
                if(!named)return false;
            }else if(!(group==2?component_forward_named(item.identity,L"NewTranslation"):
                       component_forward_named(item.identity,L"NewRotation"))||
                     !triple.read(item.struct_target,group==4))return false;
            field=item.next;++field_count;
        }
        if(field_count!=expected_fields)return false;
        unsigned char header_after[0x120]{};
        if(!component_forward_read_bytes(function_address,0,header_after,sizeof(header_after))||
           std::memcmp(header_after,function_header,sizeof(function_header)))return false;
        ready=true;return true;
    }

    bool matches() const noexcept {
        if(!ready||!request)return false;
        ComponentPreparedCall current{};
        if(!current.prepare(*request)||current.request!=request||current.module_base!=module_base||
           current.process_event!=process_event||!current.object.same(object)||
           !current.function.same(function)||!current.function_class.same(function_class)||
           current.size!=size||current.group!=group||current.parameter_bytes!=parameter_bytes||
           current.native_index!=native_index||current.return_offset!=return_offset||
           current.native_function!=native_function||current.field_head!=field_head||
           current.field_count!=field_count||std::memcmp(current.parameters,parameters,sizeof(parameters))||
           std::memcmp(current.function_header,function_header,sizeof(function_header))||
           !current.triple.same(triple))return false;
        for(std::size_t i=0;i<field_count;++i)if(!current.fields[i].same(fields[i]))return false;
        return true;
    }
};


struct ComponentForwardGate {
    using Callback=bool(*)(void*,ComponentPreparedCall const&) noexcept;
private:
    Request const* request_{};
    void* state_{};
    Callback callback_{};
    bool entered_{},used_{},in_progress_{},invalid_{};
public:
    ComponentForwardGate(Request const& request,void* state,Callback callback) noexcept:
        request_(&request),state_(state),callback_(callback){}
    ComponentForwardGate(ComponentForwardGate const&)=delete;
    ComponentForwardGate& operator=(ComponentForwardGate const&)=delete;
    ComponentForwardGate(ComponentForwardGate&&)=delete;
    ComponentForwardGate& operator=(ComponentForwardGate&&)=delete;
    bool enter_route(Request const* request) noexcept {
        if(!request||request!=request_||!state_||!callback_||entered_||used_||in_progress_||invalid_){
            invalid_=true;return false;
        }
        entered_=true;return true;
    }
    bool belongs_to(Request const* request) const noexcept {
        return entered_&&request&&request==request_&&state_&&callback_&&!used_&&!in_progress_&&!invalid_;
    }
    bool authorize_once(Request const& request,ComponentPreparedCall const& prepared) noexcept {
        if(!belongs_to(&request)||prepared.request!=&request||!prepared.ready){invalid_=true;return false;}
        used_=true;in_progress_=true;
        const bool allowed=callback_(state_,prepared);
        in_progress_=false;
        return allowed&&!invalid_;
    }
};

// No RAII objects with non-trivial destructors in this SEH function. The caller
// owns names/InvokePause/probe pairing and the gate's lifetime on its own stack.
bool invoke_component_forward_engine(Request* request,ComponentForwardGate& gate){
    if(!gate.belongs_to(request))return false;
    ComponentPreparedCall prepared{};
    if(!prepared.prepare(*request))return false;
    __try{
        if(!prepared.native_index){
            if(!gate.authorize_once(*request,prepared))return false;
            prepared.process_event(prepared.object.address,prepared.function.address,request->params);
            return true;
        }
        struct Frame {
            uptr vtable;unsigned long long tag;unsigned line,position;
            uptr node,object;unsigned char* code;void* locals;uptr previous,out;
        } frame{prepared.module_base+0x133bcb0,1,1,1,prepared.function.address,prepared.object.address,
            nullptr,request->params,0,0};
        static_assert(offsetof(Frame,node)==0x18&&offsetof(Frame,code)==0x28&&offsetof(Frame,locals)==0x30);
        unsigned char code[1024]{};std::size_t used{};
        struct Out {uptr property,address,next;};Out outputs[64]{};std::size_t output_count{};
        for(std::size_t i=0;i<prepared.field_count;++i){
            auto const& field=prepared.fields[i];
            if((field.flags&0x480)==0x80){
                if(used+9>=sizeof(code))return false;
                code[used++]=0x29;std::memcpy(code+used,&field.identity.address,8);used+=8;
                if(field.flags&0x100){
                    if(field.offset>=prepared.size||output_count>=64)return false;
                    outputs[output_count]={field.identity.address,reinterpret_cast<uptr>(request->params+field.offset),0};
                    if(output_count)outputs[output_count-1].next=reinterpret_cast<uptr>(&outputs[output_count]);
                    ++output_count;
                }
            }
        }
        code[used++]=0x16;code[used++]=0x53;frame.code=code;
        frame.out=output_count?reinterpret_cast<uptr>(outputs):0;
        using Native=void(__fastcall*)(uptr,Frame*,void*);
        if(!gate.authorize_once(*request,prepared))return false;
        reinterpret_cast<Native>(prepared.native_function)(prepared.object.address,&frame,
            prepared.return_offset==0xffff?nullptr:request->params+prepared.return_offset);
        return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}

bool invoke_component_forward(Request* request,ComponentForwardGate& gate){
    if(!gate.enter_route(request))return false;
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_names::Scope names_scope(false);
    component_cost::InvokePause cost_pause;
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    if(request&&!handstream::bridge_before_allowed(request->object,request->function))return false;
#else
    if(request)handstream::before_action(request->object,request->function);
#endif
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    if(request)vrequip::stow::before_action(request->object,request->function);
#endif
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
    const auto observation=handanim::bridge_before(request);
#endif
    const auto result=invoke_component_forward_engine(request,gate);
#if defined(GEARS_PLAYGROUND_HAND_ANIMATION_PROBE)
    handanim::bridge_after(request,observation);
#endif
    return result;
}
#endif // GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL
#include "playground_controller.inl"
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
namespace weaponshot {
gears_weapon_shot::Token enter(uptr,uptr,uptr) noexcept;
void leave(gears_weapon_shot::Token const&) noexcept;
bool packet(uptr,uptr,gears_weapon_shot::Packet&) noexcept;
bool current(gears_weapon_shot::Packet const&,uptr,uptr) noexcept;
bool write_admitted(gears_weapon_shot::Packet const&) noexcept;
}
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE) || defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
#include "weapon_grip_damage_metadata.inl"
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_CALL_CONTEXT_PROBE)
#include "weapon_grip_call_context.inl"
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_WRITE_TRIAL)
namespace gripdamage_write {
void begin(unsigned) noexcept;
void dump();
bool reset_drained() noexcept;
}
#endif
#include "weapon_grip_damage_result_probe.inl"
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_WRITE_TRIAL)
#include "weapon_grip_damage_write_trial.inl"
#endif
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_TAIL_TRANSLATION_TRIAL)
#include "vr_hand_tail_translation.inl"
namespace handstream {
bool tail_value_allowed(unsigned,uptr,handtail::Observed const&);
bool tail_layout_allowed(unsigned,uptr,std::vector<unsigned char> const&);
void tail_observation_failed(uptr,uptr,uptr,unsigned);
}
#endif
#include "vr_hand_ik_trial.inl"
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_CONTROLLED_TRIAL)
#include "vr_hand_ik_controlled_trial.inl"
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
#include "vr_hand_ik_stream_trial.inl"
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_OBSERVE)
#if defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
namespace weaponhand {void equipment_hold_committed(uptr,gears_weapon::EquipmentSnapshot const&) noexcept;}
#endif
#include "vr_equipment_snapshot.inl"
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
#include "weapon_hand_client.inl"
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
#include "weapon_shot_scope.inl"
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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    handstream::component_hold::Token component_hold_token(object,function);
#endif
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    handstream::tick_boundary_diagnostic::Token tick_diagnostic(object,function);
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
            if(!handstream::component_hold::stream_tick_allowed(component_hold_token))post_stream_hand_ik_tick=false;
#endif
            if(post_stream_hand_ik_tick){
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
                handstream::pre_tick(object,equipment_entry_observation.observation(),tick_diagnostic);
#else
                handstream::pre_tick(object,tick_diagnostic);
#endif
#else
#if defined(GEARS_PLAYGROUND_HAND_TRANSPORT_LIVENESS_TRIAL)
                handstream::pre_tick(object,equipment_entry_observation.observation());
#else
                handstream::pre_tick(object);
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    if(post_stream_hand_ik_tick){handstream::component_hold::ObserverScope hold_observer;
        handstream::component_hold::before_original(component_hold_token,true);}
#endif
    {
        handstream::EquipmentTimingScope timing(handstream::EquipmentTiming::original_player_tick,post_stream_hand_ik_tick);
        original(object,function,params);
    }
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_HOLD_TRIAL)
    if(post_stream_hand_ik_tick){handstream::component_hold::ObserverScope hold_observer;
        handstream::component_hold::after_original(component_hold_token,true);}
#endif
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
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL) && defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
        inside=true;handstream::post_tick(object,tick_diagnostic);inside=false;
#else
        inside=true;handstream::post_tick(object);inside=false;
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
    if(weaponhand::client.phase()!=gears_weapon_hand::ClientPhase::off||weaponhand::native_module)return 53;
#endif
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
    if(callbacks.load(std::memory_order_acquire))return 31;
    gripdamage::callbacks_drained();
    if(!gripdamage::reset_drained())return 32;
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundWeaponHandStart(void* native_module){
    if(weaponhand::current_gt(nullptr))return unsigned(gears_weapon_hand::Reply::reentrant);
    LifecycleScope lifecycle;return unsigned(weaponhand::start(static_cast<HMODULE>(native_module)));
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundWeaponHandStop(void*){
    if(weaponhand::current_gt(nullptr))return unsigned(gears_weapon_hand::Reply::reentrant);
    LifecycleScope lifecycle;return unsigned(weaponhand::stop());
}
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundWeaponHandStatus(void*){
    return unsigned(weaponhand::client.phase())|(weaponhand::client.armed()?0x100u:0u);
}
#endif
extern "C" __declspec(dllexport) DWORD WINAPI PlaygroundShutdown(void*){
#if defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
    // Never acquire a loader/lifecycle lock or unregister from a GT callback.
    if(weaponhand::current_gt(nullptr))return 53;
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_HAND_PARTICIPANT_TRIAL)
    if(weaponhand::stop()!=gears_weapon_hand::Reply::quiescent)return 53;
    // client remains static/alive through the existing hook-disable and PG
    // callbacks drain below. Native quiescence alone never permits PG unload.
#endif
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
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
    gripdamage::callbacks_drained();
    if(!gripdamage::reset_drained())return 32;
#endif
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
