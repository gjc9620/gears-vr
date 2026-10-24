// Finite CPU proof of the actual NoAuth handoff and the mechanically extracted
// stow initial-request token. Host names/association and backend bytes are
// controlled inputs. No game helper, Scene, CF, original body or writer runs.
#include <windows.h>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <limits>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include "../vr-common/hand_ik_stream_lease.h"
#include "../vr-common/hand_ik_trial_policy.h"
#include "../vr-common/hand_stream_ticket_policy.h"
#include "../vr-common/hand_stream_grip_binding.h"
#include "../vr-common/free_hand_control.h"
#include "../vr-common/weapon_mesh_follow.h"
#include "../vr-common/weapon_component_continuous_lease.h"
#include "../vr-common/weapon_mesh_component_lease.h"
#include "../vr-common/weapon_stow_lease.h"
#include "../vr-common/equipment_stow_pending.h"
#ifdef NDEBUG
#error WeaponInitialHandoffTest requires active assertions
#endif
#if !defined(GEARS_PLAYGROUND_WEAPON_ACTION_ENTRY_TRIAL)
#error WeaponInitialHandoffTest requires the ENTRY token block
#endif
#ifndef GEARS_TEST_STOW_INITIAL_BLOCK
#error Supply the exact mechanically extracted production InitialSelf block
#endif

namespace {
using uptr=std::uintptr_t;
struct Request {volatile LONG state;unsigned size;uptr object,function;unsigned char params[4096];};
std::atomic<DWORD> game_thread{};
unsigned metadata_reads{},engine_reads{},engine_writes{},log_calls{};
bool hand_match=true,stow_match=true,name_result=true;
wchar_t const* function_name=L"StartMeleeAttack";
bool registered(uptr){++metadata_reads;return true;}
bool name(uptr,wchar_t* out,std::size_t count){
    ++metadata_reads;if(!name_result)return false;
    const auto length=wcslen(function_name);assert(length<count);
    std::wmemcpy(out,function_name,length+1);return true;
}
[[maybe_unused]] FILE* log_file=reinterpret_cast<FILE*>(1);
[[maybe_unused]] int host_log_printf(FILE*,wchar_t const*,...){++log_calls;return 0;}
[[maybe_unused]] int host_log_flush(FILE*){++log_calls;return 0;}
namespace handstream {
struct Trial {
    gears_hand_ik_stream::Lease lease;
    gears_hand_ik::StabilityGate stability;gears_weapon::FreeHandControl consumer;
    gears_hand_stream::Policy xr_policy;std::array<gears_hand_stream::GripHandBinding,2> xr_binding;
    unsigned epoch{3};bool anchored{true},ended{},releasing{},xr_ready{true};
} trial;
std::atomic<bool> active_visible{true},stop_requested{},safe_to_unload{true};
bool dual_draining{},hand_writes_relinquished{};
inline bool related(uptr){++metadata_reads;return hand_match;}
inline bool component_action_name(wchar_t const*){++metadata_reads;return true;}
namespace component {
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
gears_weapon::WeaponMeshComponentContinuousLease<unsigned> lease;
#else
gears_weapon::WeaponMeshComponentLease<unsigned> lease;
#endif
gears_weapon::WeaponMeshFollow mount;
struct Self {uptr object{100},function{200};bool armed{true};} self;
bool consume_self(uptr,uptr)=delete;
}
inline bool component_dirty(){return component::lease.dirty();}
namespace component_continuous {
enum class Pause {none,age,boundary};enum class Interval {waiting,owned,closing,suspended};
struct State {
    bool closed{},deferred{},terminal{},resume_stable{true};
    Pause pause{Pause::none};Interval interval{Interval::owned};
} state;
bool finish()=delete;
}
void before_action(uptr,uptr)=delete;
bool restore(wchar_t const*)=delete;
bool yield_for_equipment(wchar_t const*)=delete;
void component_reset_binding(unsigned)=delete;
}
namespace vrequip {namespace stow {
gears_weapon::WeaponStowLease<unsigned> lease;gears_weapon::EquipmentStowPending<unsigned> pending;
std::atomic<bool> dirty_visible{},pending_visible{},stop_requested{};
bool mutating{};std::uint64_t epoch{9};
inline bool related(uptr){++metadata_reads;return stow_match;}
void before_action(uptr,uptr)=delete;
bool restore(wchar_t const*)=delete;
void finish_pending()=delete;
void weapon_state(uptr)=delete;
// The definition and its preprocessor guard are verbatim production bytes.
#include GEARS_TEST_STOW_INITIAL_BLOCK
}}
#define fwprintf host_log_printf
#define fflush host_log_flush
#include "weapon_action_handoff.inl"
#undef fflush
#undef fwprintf
using Seen=weaponaction_handoff::Seen;
static_assert(std::is_trivially_copyable_v<vrequip::stow::InitialSelf>);

void no_engine_or_log(){assert(!engine_reads&&!engine_writes&&!log_calls);}
void start_observation(){engine_reads=engine_writes=log_calls=metadata_reads=0;}
Seen observe(){const auto result=weaponaction_handoff::inspect(100,200);no_engine_or_log();return result;}
void revoke(Seen const& seen){
    const auto before=metadata_reads;weaponaction_handoff::revoke_no_engine(seen);
    assert(metadata_reads==before);no_engine_or_log();
}
struct HandBytes {
    std::array<gears_hand_ik_stream::Value,4> values{};bool fail_write{},reads_blocked{};
    bool read(unsigned i,gears_hand_ik_stream::Value& out){++engine_reads;if(reads_blocked)return false;out=values[i];return true;}
    bool write_target(unsigned i,gears_hand_ik_stream::Target const& expected,gears_hand_ik_stream::Target const& wanted){
        ++engine_writes;assert(values[i].target==expected);values[i].target=wanted;
        if(fail_write){reads_blocked=true;return false;}return true;
    }
    bool set_strength(unsigned i,float value){++engine_writes;values[i].strength={{value,value,0}};return true;}
};
[[maybe_unused]] void dirty_hand(bool fault=false){
    HandBytes bytes;bytes.fail_write=fault;std::array<gears_hand_ik_stream::Target,4> targets{};
    for(auto& target:targets)target[0]=1;
    const bool acquired=handstream::trial.lease.acquire(targets,0.5f,bytes);
    assert(acquired!=fault&&handstream::trial.lease.dirty());
    assert(handstream::trial.lease.faulted()==fault);start_observation();
}
struct StowBytes {
    gears_weapon::StowState<unsigned> state{7,gears_weapon::StowAttachment::original_hand,true};
    bool read(gears_weapon::StowState<unsigned>& out){++engine_reads;out=state;return true;}
    bool set_holstered(bool value){
        ++engine_writes;state.attachment=value?gears_weapon::StowAttachment::detached:gears_weapon::StowAttachment::original_hand;return true;
    }
};
[[maybe_unused]] void dirty_stow(){
    StowBytes bytes;assert(vrequip::stow::lease.acquire(bytes));vrequip::stow::dirty_visible=true;start_observation();
}
[[maybe_unused]] void pending_stow(){
    assert(vrequip::stow::pending.begin(7,100,1000));vrequip::stow::pending_visible=true;start_observation();
}
struct ComponentBytes {
    using Value=gears_weapon::WeaponMeshComponentValue;using Absolute=gears_weapon::MeshComponentAbsolute;
    using Bytes=gears_weapon::MeshComponentBytes12;Value value{};
    bool read(gears_weapon::WeaponMeshComponentState<unsigned>& out){++engine_reads;out={7,value,true};return true;}
    bool set_absolute(unsigned,Value const&,Absolute const& wanted){++engine_writes;value.absolute=wanted;return true;}
    bool set_translation(unsigned,Value const&,Bytes const& wanted){++engine_writes;value.translation=wanted;return true;}
    bool set_rotation(unsigned,Value const&,Bytes const& wanted){++engine_writes;value.rotation=wanted;return true;}
    template<class Permit>bool set_absolute(unsigned key,Value const& before,Absolute const& wanted,Permit& permit){return permit()&&set_absolute(key,before,wanted);}
    template<class Permit>bool set_translation(unsigned key,Value const& before,Bytes const& wanted,Permit& permit){return permit()&&set_translation(key,before,wanted);}
    template<class Permit>bool set_rotation(unsigned key,Value const& before,Bytes const& wanted,Permit& permit){return permit()&&set_rotation(key,before,wanted);}
};
[[maybe_unused]] void dirty_component(){
    ComponentBytes bytes;auto wanted=bytes.value;wanted.translation[0]=1;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    auto authorize=[](unsigned,unsigned,auto const&,auto const&,unsigned){return true;};
    assert(handstream::component::lease.acquire(7,wanted,1u,authorize,bytes));
#else
    assert(handstream::component::lease.acquire(7,wanted,bytes));
#endif
    assert(handstream::component::lease.dirty());start_observation();
}
bool same_component_self(handstream::component::Self const& before){
    const auto now=handstream::component::self;
    return now.object==before.object&&now.function==before.function&&now.armed==before.armed;
}
Request request(uptr object=100,uptr function=200){Request result{};result.object=object;result.function=function;return result;}
void token_exact(){
    auto own=request();const auto component_before=handstream::component::self;
    assert(!vrequip::stow::consume_initial_self(&own));
    const auto before=vrequip::stow::initial_self;
    {vrequip::stow::InitialSelfScope scope(&own);
        assert(vrequip::stow::initial_self.request==&own&&vrequip::stow::initial_self.armed);
        assert(vrequip::stow::consume_initial_self(&own));
        assert(!vrequip::stow::initial_self.armed&&!vrequip::stow::consume_initial_self(&own));
        assert(same_component_self(component_before));
    }
    assert(vrequip::stow::same_initial_self(before,vrequip::stow::initial_self));no_engine_or_log();
}
void token_refused(){
    auto own=request();auto copy=own;
    vrequip::stow::InitialSelfScope scope(&own);const auto before=vrequip::stow::initial_self;
    const auto refused=[&](Request const* value){
        assert(!vrequip::stow::consume_initial_self(value));
        assert(vrequip::stow::same_initial_self(before,vrequip::stow::initial_self));
    };
    refused(nullptr);refused(&copy);
    ++own.object;refused(&own);--own.object;
    ++own.function;refused(&own);--own.function;
    game_thread=0;refused(&own);game_thread=GetCurrentThreadId();
    assert(vrequip::stow::consume_initial_self(&own));no_engine_or_log();
}
void token_field_mismatch(){
    auto own=request();vrequip::stow::InitialSelfScope scope(&own);
    const auto exact=vrequip::stow::initial_self;
    ++vrequip::stow::initial_self.object;
    assert(!vrequip::stow::consume_initial_self(&own)&&vrequip::stow::initial_self.armed);
    vrequip::stow::initial_self=exact;++vrequip::stow::initial_self.function;
    assert(!vrequip::stow::consume_initial_self(&own)&&vrequip::stow::initial_self.armed);
    vrequip::stow::initial_self=exact;vrequip::stow::initial_self.request=nullptr;
    assert(!vrequip::stow::consume_initial_self(&own)&&vrequip::stow::initial_self.armed);
    vrequip::stow::initial_self=exact;assert(vrequip::stow::consume_initial_self(&own));no_engine_or_log();
}
bool recursive_consume(Request const* own,unsigned depth){
    if(vrequip::stow::consume_initial_self(own))return true;
    return depth&&recursive_consume(own,depth-1);
}
void token_nested(){
    auto outer=request();auto inner=request();
    vrequip::stow::InitialSelfScope outer_scope(&outer);
    assert(vrequip::stow::consume_initial_self(&outer));const auto consumed=vrequip::stow::initial_self;
    assert(!recursive_consume(&outer,8));
    {vrequip::stow::InitialSelfScope inner_scope(&inner);
        assert(!vrequip::stow::consume_initial_self(&outer)&&vrequip::stow::initial_self.armed);
        assert(vrequip::stow::consume_initial_self(&inner)&&!recursive_consume(&inner,8));
    }
    assert(vrequip::stow::same_initial_self(consumed,vrequip::stow::initial_self));
    assert(!recursive_consume(&outer,8));no_engine_or_log();
}
void token_unwind(){
    auto outer=request();auto inner=request();
    vrequip::stow::InitialSelfScope outer_scope(&outer);assert(vrequip::stow::consume_initial_self(&outer));
    const auto consumed=vrequip::stow::initial_self;
    try{vrequip::stow::InitialSelfScope inner_scope(&inner);assert(vrequip::stow::consume_initial_self(&inner));throw std::runtime_error("controlled token unwind");}
    catch(std::runtime_error const&){}
    assert(vrequip::stow::same_initial_self(consumed,vrequip::stow::initial_self));
    assert(!vrequip::stow::consume_initial_self(&outer));no_engine_or_log();
}
void foreign_thread(){
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    dirty_hand();
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    dirty_component();
#endif
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    dirty_stow();pending_stow();
#endif
    auto own=request();vrequip::stow::InitialSelfScope scope(&own);const auto initial=vrequip::stow::initial_self;
    const auto seen=observe();const auto hand_epoch=handstream::trial.epoch;const auto stow_epoch=vrequip::stow::epoch;
    const bool hand_dirty=handstream::trial.lease.dirty(),component_dirty=handstream::component_dirty();
    const bool stow_dirty=vrequip::stow::lease.dirty(),pending=vrequip::stow::pending.active();
    auto* inaccessible=static_cast<Request*>(VirtualAlloc(nullptr,sizeof(Request),MEM_COMMIT|MEM_RESERVE,PAGE_NOACCESS));
    assert(inaccessible);start_observation();
    std::thread worker([&]{
        const auto foreign=weaponaction_handoff::inspect(~uptr{},~uptr{});
        assert(!foreign.gt&&!foreign.required&&!foreign.hands&&!foreign.stow);
        assert(!vrequip::stow::consume_initial_self(&own));
        assert(!vrequip::stow::consume_initial_self(inaccessible));
        weaponaction_handoff::revoke_no_engine(seen);weaponaction_handoff::revoke_no_engine(foreign);
    });worker.join();
    assert(!metadata_reads&&handstream::trial.epoch==hand_epoch&&vrequip::stow::epoch==stow_epoch);
    assert(!handstream::stop_requested&&!vrequip::stow::stop_requested);
    assert(handstream::trial.lease.dirty()==hand_dirty&&handstream::component_dirty()==component_dirty);
    assert(!handstream::trial.lease.faulted()&&!handstream::component::lease.faulted());
    assert(vrequip::stow::lease.dirty()==stow_dirty&&vrequip::stow::pending.active()==pending&&!vrequip::stow::lease.revoked());
    assert(vrequip::stow::same_initial_self(initial,vrequip::stow::initial_self));
    assert(vrequip::stow::consume_initial_self(&own));
    assert(VirtualFree(inaccessible,0,MEM_RELEASE));no_engine_or_log();
}
[[maybe_unused]] void hand_guard(){
    dirty_hand();handstream::trial.releasing=true;handstream::dual_draining=true;
    const auto self=handstream::component::self;const auto seen=observe();
    assert(seen.hands&&seen.hands_required&&seen.required&&seen.hands_restoration_busy);
#if !defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    assert(seen.hands_restore_echo&&seen.owned_restore_echo);
#endif
    revoke(seen);assert(handstream::trial.epoch==4&&handstream::trial.lease.dirty());
    assert(handstream::stop_requested&&!handstream::safe_to_unload&&same_component_self(self));
}
[[maybe_unused]] void stow_guard(){
    dirty_stow();vrequip::stow::mutating=true;const auto seen=observe();
    assert(seen.stow&&seen.stow_required&&seen.required&&seen.stow_restore_echo&&seen.owned_restore_echo);
    revoke(seen);assert(vrequip::stow::lease.dirty()&&vrequip::stow::lease.faulted()&&vrequip::stow::lease.revoked());
    assert(vrequip::stow::dirty_visible&&vrequip::stow::stop_requested&&vrequip::stow::epoch==10);
}
[[maybe_unused]] void simultaneous(){
    dirty_hand();dirty_stow();handstream::trial.releasing=true;vrequip::stow::mutating=true;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    dirty_component();
#endif
    auto own=request();vrequip::stow::InitialSelfScope scope(&own);const auto component=handstream::component::self;
    const auto seen=observe();assert(seen.hands_required&&seen.stow_required&&seen.required);
    assert(vrequip::stow::consume_initial_self(&own));const auto still=observe();
    assert(still.hands_required&&still.stow_required&&same_component_self(component));
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    assert(still.component_dirty&&!handstream::component::lease.faulted()&&!handstream::component::lease.revoked_mask());
#endif
    // A consumed token is a separate owned-route input, not a new inspect fact.
    revoke(still);assert(handstream::trial.epoch==4&&vrequip::stow::epoch==10);
    assert(handstream::trial.lease.dirty()&&vrequip::stow::lease.dirty()&&vrequip::stow::lease.revoked());
}
[[maybe_unused]] void component_only(){
    dirty_component();handstream::active_visible=false;vrequip::stow::mutating=true;
    const auto self=handstream::component::self;const auto seen=observe();
    assert(seen.hands_required&&seen.component_dirty&&!seen.stow_required);
    revoke(seen);assert(handstream::component::lease.dirty()&&handstream::component::lease.faulted());
    assert(handstream::component::lease.revoked_mask()&&same_component_self(self)&&!vrequip::stow::stop_requested);
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    assert(handstream::component::lease.forward_stopped()&&handstream::component::lease.relinquished());
    assert(handstream::component_continuous::state.closed&&handstream::component_continuous::state.deferred);
#endif
}
[[maybe_unused]] void stow_independent(){
    dirty_hand();dirty_stow();hand_match=false;handstream::trial.releasing=true;vrequip::stow::mutating=true;
    const auto seen=observe();assert(!seen.hands_required&&seen.stow_required);revoke(seen);
    assert(handstream::trial.epoch==3&&handstream::trial.lease.dirty()&&!handstream::stop_requested);
    assert(vrequip::stow::lease.revoked()&&vrequip::stow::epoch==10);
}
[[maybe_unused]] void hands_independent(){
    dirty_hand();dirty_stow();stow_match=false;handstream::trial.releasing=true;vrequip::stow::mutating=true;
    const auto seen=observe();assert(seen.hands_required&&!seen.stow_required);revoke(seen);
    assert(handstream::trial.epoch==4&&vrequip::stow::epoch==9&&!vrequip::stow::lease.revoked());
    assert(vrequip::stow::lease.dirty()&&!vrequip::stow::stop_requested);
}
[[maybe_unused]] void pending_guard(){
    pending_stow();vrequip::stow::mutating=true;function_name=L"BeginState";const auto seen=observe();
    assert(seen.stow_required&&seen.pending&&seen.pending_state_boundary&&seen.stow_restore_echo);revoke(seen);
    assert(vrequip::stow::pending.active()&&vrequip::stow::pending_visible&&vrequip::stow::stop_requested);
}
[[maybe_unused]] void hand_fault(){
    dirty_hand(true);handstream::trial.releasing=true;const auto seen=observe();assert(seen.hands_required);revoke(seen);
    assert(handstream::trial.lease.dirty()&&handstream::trial.lease.faulted()&&handstream::stop_requested);
}
[[maybe_unused]] void stow_fault(){
    dirty_stow();vrequip::stow::lease.revoke();vrequip::stow::mutating=true;start_observation();
    const auto seen=observe();assert(seen.stow_required&&seen.stow_dirty);revoke(seen);
    assert(vrequip::stow::lease.dirty()&&vrequip::stow::lease.faulted()&&vrequip::stow::lease.revoked());
}
[[maybe_unused]] void component_fault(){
    dirty_component();handstream::component::lease.relinquish();handstream::trial.releasing=true;start_observation();
    const auto seen=observe();assert(seen.hands_required&&seen.component_dirty);revoke(seen);
    assert(handstream::component::lease.dirty()&&handstream::component::lease.faulted()&&handstream::component::lease.revoked_mask());
}
struct Case {char const* name;void (*run)();};
constexpr Case cases[]={
    {"token_exact",token_exact},{"token_refused",token_refused},{"token_fields",token_field_mismatch},
    {"token_nested",token_nested},{"token_unwind",token_unwind},{"foreign_thread",foreign_thread},
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    {"hand_guard",hand_guard},{"hand_fault",hand_fault},
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    {"stow_guard",stow_guard},{"pending_guard",pending_guard},{"stow_fault",stow_fault},
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL) && defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    {"simultaneous",simultaneous},{"stow_independent",stow_independent},{"hands_independent",hands_independent},
#endif
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL) && defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    {"component_only",component_only},{"component_fault",component_fault},
#endif
};
}
int main(int argc,char** argv){
    assert(argc==2);game_thread=GetCurrentThreadId();
    for(auto const& test:cases)if(!std::strcmp(argv[1],test.name)){
        test.run();no_engine_or_log();
        std::printf("WeaponInitialHandoffTest %s passed; engine_reads=%u engine_writes=%u logs=%u\n",
            test.name,engine_reads,engine_writes,log_calls);return 0;
    }
    std::fprintf(stderr,"Unsupported test mode: %s\n",argv[1]);return 2;
}
