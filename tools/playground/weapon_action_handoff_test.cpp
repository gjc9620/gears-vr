// Finite host-services test. The production helper and pure journal/policy
// classes run here; name/association services are controlled inputs, not a
// second implementation of game reflection or its action classifiers.
#include <windows.h>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <limits>
#include <new>
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
#error WeaponActionHandoffTest requires active assertions
#endif
namespace {
using uptr=std::uintptr_t;
std::atomic<DWORD> game_thread{};
unsigned metadata_reads{},setters{};bool registered_result=true,name_result=true,hand_match=true,stow_match=true,hand_name_result=true;
wchar_t const* function_name=L"StartMeleeAttack";
bool registered(uptr){++metadata_reads;return registered_result;}
bool name(uptr,wchar_t* out,std::size_t count){++metadata_reads;if(!name_result)return false;
    const auto length=wcslen(function_name);assert(length<count);std::wmemcpy(out,function_name,length+1);return true;}
namespace handstream {
struct Trial {
    gears_hand_ik_stream::Lease lease;gears_hand_ik::StabilityGate stability;gears_weapon::FreeHandControl consumer;
    gears_hand_stream::Policy xr_policy;std::array<gears_hand_stream::GripHandBinding,2> xr_binding;
    unsigned epoch{3};bool anchored{true},ended{},releasing{},xr_ready{true};
} trial;
std::atomic<bool> active_visible{true},stop_requested{},safe_to_unload{true};bool dual_draining{},hand_writes_relinquished{};
inline bool related(uptr){++metadata_reads;return hand_match;}
inline bool component_action_name(wchar_t const*){++metadata_reads;return hand_name_result;}
namespace component {
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
gears_weapon::WeaponMeshComponentContinuousLease<unsigned> lease;
#else
gears_weapon::WeaponMeshComponentLease<unsigned> lease;
#endif
gears_weapon::WeaponMeshFollow mount;
struct Self {uptr object{1},function{2};bool armed{true};} self;
bool consume_self(uptr,uptr)=delete;
}
inline bool component_dirty(){return component::lease.dirty();}
namespace component_continuous {
enum class Pause {none,age,boundary};enum class Interval {waiting,owned,closing,suspended};
struct State {bool closed{},deferred{},terminal{},resume_stable{true};Pause pause{Pause::none};Interval interval{Interval::owned};} state;
struct Action {Action()=delete;};
bool finish()=delete;
}
void before_action(uptr,uptr)=delete;
bool restore(wchar_t const*)=delete;
bool yield_for_equipment(wchar_t const*)=delete;
void component_reset_binding(unsigned)=delete;
}
namespace vrequip {namespace stow {
gears_weapon::WeaponStowLease<unsigned> lease;gears_weapon::EquipmentStowPending<unsigned> pending;
std::atomic<bool> dirty_visible{},pending_visible{},stop_requested{};bool mutating{};std::uint64_t epoch{9};
inline bool related(uptr){++metadata_reads;return stow_match;}
void before_action(uptr,uptr)=delete;
bool restore(wchar_t const*)=delete;
void finish_pending()=delete;
void weapon_state(uptr)=delete;
}}
#include "weapon_action_handoff.inl"
using Seen=weaponaction_handoff::Seen;
void reset(){
    // End one artificial host fixture. This is not a production journal reset.
    handstream::trial.~Trial();new (&handstream::trial) handstream::Trial;
    using ComponentLease=decltype(handstream::component::lease);using StowLease=decltype(vrequip::stow::lease);
    handstream::component::lease.~ComponentLease();new (&handstream::component::lease) ComponentLease;
    vrequip::stow::lease.~StowLease();new (&vrequip::stow::lease) StowLease;
    vrequip::stow::pending={};handstream::component_continuous::state={};handstream::component::self={};
    game_thread=GetCurrentThreadId();handstream::active_visible=true;handstream::stop_requested=false;handstream::safe_to_unload=true;
    handstream::dual_draining=handstream::hand_writes_relinquished=false;vrequip::stow::dirty_visible=false;
    vrequip::stow::pending_visible=false;vrequip::stow::stop_requested=false;vrequip::stow::mutating=false;vrequip::stow::epoch=9;
    metadata_reads=setters=0;registered_result=name_result=hand_match=stow_match=hand_name_result=true;function_name=L"StartMeleeAttack";
}
struct HandBytes {
    std::array<gears_hand_ik_stream::Value,4> values{};
    bool read(unsigned i,gears_hand_ik_stream::Value& out){out=values[i];return true;}
    bool write_target(unsigned i,gears_hand_ik_stream::Target const& expected,gears_hand_ik_stream::Target const& wanted){
        assert(values[i].target==expected);values[i].target=wanted;++setters;return true;}
    bool set_strength(unsigned i,float value){values[i].strength={{value,value,0}};++setters;return true;}
};
[[maybe_unused]] void dirty_hand(){HandBytes bytes;std::array<gears_hand_ik_stream::Target,4> targets{};
    for(auto& target:targets)target[0]=1;assert(handstream::trial.lease.acquire(targets,0.5f,bytes));setters=0;}
struct StowBytes {
    gears_weapon::StowState<unsigned> state{7,gears_weapon::StowAttachment::original_hand,true};
    bool read(gears_weapon::StowState<unsigned>& out){out=state;return true;}
    bool set_holstered(bool value){++setters;state.attachment=value?gears_weapon::StowAttachment::detached:gears_weapon::StowAttachment::original_hand;return true;}
};
[[maybe_unused]] void dirty_stow(){StowBytes bytes;assert(vrequip::stow::lease.acquire(bytes));vrequip::stow::dirty_visible=true;setters=0;}
[[maybe_unused]] void pending_stow(){assert(vrequip::stow::pending.begin(7,100,1000));vrequip::stow::pending_visible=true;}
struct ComponentBytes {
    using Value=gears_weapon::WeaponMeshComponentValue;using Absolute=gears_weapon::MeshComponentAbsolute;using Bytes=gears_weapon::MeshComponentBytes12;
    Value value{};
    bool read(gears_weapon::WeaponMeshComponentState<unsigned>& out){out={7,value,true};return true;}
    bool set_absolute(unsigned,Value const&,Absolute const& wanted){value.absolute=wanted;++setters;return true;}
    bool set_translation(unsigned,Value const&,Bytes const& wanted){value.translation=wanted;++setters;return true;}
    bool set_rotation(unsigned,Value const&,Bytes const& wanted){value.rotation=wanted;++setters;return true;}
    template<class Permit>bool set_absolute(unsigned k,Value const& before,Absolute const& wanted,Permit& p){return p()&&set_absolute(k,before,wanted);}
    template<class Permit>bool set_translation(unsigned k,Value const& before,Bytes const& wanted,Permit& p){return p()&&set_translation(k,before,wanted);}
    template<class Permit>bool set_rotation(unsigned k,Value const& before,Bytes const& wanted,Permit& p){return p()&&set_rotation(k,before,wanted);}
};
[[maybe_unused]] void dirty_component(){ComponentBytes bytes;auto wanted=bytes.value;wanted.translation[0]=1;
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
    const auto allow=[](unsigned,unsigned,auto const&,auto const&,unsigned){return true;};auto authorize=allow;
    assert(handstream::component::lease.acquire(7,wanted,1u,authorize,bytes));
#else
    assert(handstream::component::lease.acquire(7,wanted,bytes));
#endif
    setters=0;
}
struct PolicyInput {gears_hand_stream::Context context{{11,12,13,14,0},7,3,1000,true,true};gears_hand_stream::Plan plan;};
[[maybe_unused]] PolicyInput live_policy(){
    PolicyInput input;auto& policy=handstream::trial.xr_policy;
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
    assert(policy.enable_certified_calibration(700,1));
#endif
    for(std::uint64_t serial=1;serial<=2;++serial){
        gears_hand_stream::Ticket ticket{{1,2,3,7},serial,serial,1000,12,13,0,100.,3};
        ticket.certificate={700,81,91,1,1,1,{1,1},1,10000000000,10000000000,1};
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
        input.plan=policy.observe_certified(input.context,&ticket,1000,10000000000);
#else
        input.plan=policy.observe(input.context,&ticket,1000);
#endif
    }
    assert(policy.commit_binding(input.plan,3));
    std::array<float,16> bone{};bone[0]=bone[5]=bone[10]=bone[15]=1;
    for(auto& binding:handstream::trial.xr_binding)assert(binding.bind({},bone));
    return input;
}
void fenced(Seen const& seen){const auto before=setters;weaponaction_handoff::revoke_no_engine(seen);assert(setters==before);}
}
int main(){
    unsigned cases{};const auto passed=[&](char const* text){++cases;std::printf("%02u %s\n",cases,text);};
    {reset();game_thread=GetCurrentThreadId()+1;const auto seen=weaponaction_handoff::inspect(1,2);assert(!seen.gt&&!metadata_reads);
        fenced(seen);assert(!metadata_reads&&!handstream::stop_requested&&!vrequip::stow::stop_requested);passed("foreign thread reads no private services");}
    {reset();game_thread=0;const auto seen=weaponaction_handoff::inspect(1,2);assert(!seen.gt&&!metadata_reads);passed("unknown GT reads no private services");}
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    {reset();dirty_hand();const auto seen=weaponaction_handoff::inspect(1,2);assert(seen.hands&&seen.hands_required&&seen.required);
        fenced(seen);assert(handstream::trial.epoch==4&&handstream::trial.lease.dirty()&&handstream::stop_requested&&!handstream::safe_to_unload);
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
        assert(handstream::hand_writes_relinquished&&seen.hands_forward_fence_supported);
#else
        assert(!seen.hands_forward_fence_supported);
#endif
        passed("dirty hand remains dirty with old forward authority withdrawn");}
    {reset();hand_match=false;const auto seen=weaponaction_handoff::inspect(1,2);assert(!seen.hands&&!seen.hands_required);
        fenced(seen);assert(handstream::trial.epoch==3);passed("hand association remains participant specific");}
    {reset();name_result=false;const auto seen=weaponaction_handoff::inspect(1,2);assert(seen.unknown_name&&seen.hands_required&&!seen.recognized);
        fenced(seen);assert(handstream::trial.epoch==4);passed("unknown related function cannot report unrelated");}
    {reset();registered_result=false;const auto seen=weaponaction_handoff::inspect(1,2);assert(seen.unknown_name&&seen.hands_required);
        passed("unregistered related function retains uncertain obligation");}
    {reset();const auto seen=weaponaction_handoff::inspect(1,2);game_thread=GetCurrentThreadId()+1;fenced(seen);
        assert(handstream::trial.epoch==3);passed("revoke rechecks current GT");}
    {reset();handstream::trial.epoch=(std::numeric_limits<unsigned>::max)();const auto seen=weaponaction_handoff::inspect(1,2);fenced(seen);
        assert(handstream::trial.epoch==(std::numeric_limits<unsigned>::max)()&&handstream::stop_requested);passed("hand epoch does not wrap");}
#if defined(GEARS_PLAYGROUND_HAND_IK_XR_STREAM_TRIAL)
    {reset();const auto live=live_policy();const auto old=live.plan;const auto seen=weaponaction_handoff::inspect(1,2);fenced(seen);
        auto& policy=handstream::trial.xr_policy;assert(!handstream::trial.xr_ready&&!policy.bound_mask()&&!policy.continuity_snapshot(1000).ready);
        assert(policy.last_observe().observed_now==0&&!policy.last_observe().new_ticket_age.known);
#if defined(GEARS_PLAYGROUND_HAND_CERTIFIED_CONTINUITY_TRIAL)
        assert(!policy.authorize_certified(old,live.context,1000,10000000000));
#else
        assert(!policy.authorize(old,live.context,1000));
#endif
        for(auto& binding:handstream::trial.xr_binding)assert(!binding.bound());
        passed("pure policy revokes old Plan and bindings without a fresh source");}
#endif
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    {reset();dirty_component();handstream::trial.releasing=true;handstream::dual_draining=true;const auto seen=weaponaction_handoff::inspect(1,2);
        assert(seen.hands_required&&!seen.hands_restore_echo&&seen.hands_restoration_busy);fenced(seen);
        assert(handstream::component::lease.dirty()&&handstream::component::lease.revoked_mask()&&handstream::component::self.armed);
#if defined(GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL)
        assert(handstream::component::lease.forward_stopped()&&handstream::component::lease.relinquished());
        assert(handstream::component_continuous::state.closed&&handstream::component_continuous::state.deferred);
#endif
        passed("mesh releasing dual-drain and matching self never exempt CF PE");}
#else
    {reset();dirty_hand();handstream::trial.releasing=true;const auto seen=weaponaction_handoff::inspect(1,2);
        assert(seen.hands_restore_echo&&!seen.hands_required&&seen.owned_restore_echo);fenced(seen);
        assert(handstream::trial.epoch==3&&!handstream::stop_requested&&handstream::trial.lease.dirty());passed("non-mesh original releasing guard suppresses extra obligation only");}
#endif
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    {reset();dirty_stow();const auto seen=weaponaction_handoff::inspect(1,2);assert(seen.stow_required);fenced(seen);
        assert(vrequip::stow::lease.dirty()&&vrequip::stow::lease.revoked()&&vrequip::stow::dirty_visible&&vrequip::stow::stop_requested&&vrequip::stow::epoch==10);
        passed("stow revoke preserves dirty attachment journal");}
    {reset();pending_stow();function_name=L"ButtonPress";hand_name_result=false;const auto seen=weaponaction_handoff::inspect(1,2);
        assert(seen.stow_required&&seen.pending&&seen.recognized);fenced(seen);assert(vrequip::stow::pending.active()&&vrequip::stow::pending_visible);
        auto wait=vrequip::stow::pending;assert(wait.poll(true,7,101,true,true,vrequip::stow::stop_requested)==gears_weapon::StowPendingResult::cancelled);
        passed("pending ButtonPress remains unresolved and Stop outranks readiness");}
    {reset();function_name=L"ButtonRelease";hand_name_result=false;const auto seen=weaponaction_handoff::inspect(1,2);
        assert(!seen.stow_required);pending_stow();const auto pending=weaponaction_handoff::inspect(1,2);assert(pending.stow_required);
        passed("ButtonRelease is a stow boundary only while pending");}
    {reset();pending_stow();function_name=L"BeginState";const auto seen=weaponaction_handoff::inspect(1,2);
        assert(seen.pending_state_boundary&&seen.stow_required);fenced(seen);assert(vrequip::stow::pending.active());
        passed("pending state boundary has no GetStateName exemption");}
    {reset();dirty_stow();stow_match=false;const auto seen=weaponaction_handoff::inspect(1,2);assert(!seen.stow_required);
        fenced(seen);assert(!vrequip::stow::lease.revoked()&&!vrequip::stow::stop_requested);passed("stow association remains participant specific");}
    {reset();dirty_stow();vrequip::stow::mutating=true;const auto seen=weaponaction_handoff::inspect(1,2);
        assert(seen.stow_restore_echo&&!seen.stow_required&&seen.owned_restore_echo);fenced(seen);
        assert(vrequip::stow::lease.dirty()&&!vrequip::stow::lease.revoked()&&!vrequip::stow::stop_requested&&vrequip::stow::epoch==9);
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
        assert(seen.hands_required&&handstream::trial.epoch==4);
#endif
        passed("stow mutating echo preserves its lease and cannot exempt hands");}
    {reset();pending_stow();vrequip::stow::epoch=(std::numeric_limits<std::uint64_t>::max)();const auto seen=weaponaction_handoff::inspect(1,2);fenced(seen);
        assert(vrequip::stow::epoch==(std::numeric_limits<std::uint64_t>::max)()&&vrequip::stow::stop_requested);
        passed("stow epoch does not wrap");}
#endif
#if !defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL) && !defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    {reset();const auto seen=weaponaction_handoff::inspect(1,2);assert(seen.gt&&!seen.required&&!seen.recognized);fenced(seen);
        passed("no old participant creates no old-journal obligation");}
#endif
    std::printf("WeaponActionHandoffTest passed: %u cases; production helper/pure journals, controlled host names/association only\n",cases);
}
