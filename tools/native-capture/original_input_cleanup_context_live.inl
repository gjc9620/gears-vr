// Include globally after weapon_hand_scene_binding/runtime and Scope. No new
// writer/hook/cache/callback, no Engine calls, and no gameplay permission.
#include "original_input_cleanup_context_policy.h"
#include "native_action_context.h"
namespace original_input_cleanup_context_live {
namespace api=gears_weapon_original_input_cleanup_context;
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
namespace policy=gears_original_input_cleanup_policy;
namespace hand=gears_weapon_hand;
namespace rt=weapon_hand_runtime;
namespace scene=weapon_hand_scene_binding;

policy::Runtime runtime() noexcept {
    policy::Runtime value{};auto* broker=rt::current();const auto session=rt::session();
    value.game_pid=session.game_pid;value.game_start=session.game_start;value.native_started=session.native_started_qpc;
    value.worker_thread=session.owner_worker_thread;value.current_thread=GetCurrentThreadId();
    value.game_thread=scene::game_thread.load();value.world_generation=committed_world_generation.load();
    value.available=rt::enabled&&broker&&rt::current()==broker&&
        !rt::detail::admission_closed.load()&&!rt::detail::stop_requested.load()&&!rt::detail::unknown_fault.load();
    if(!broker)return value;
    value.cookie=broker->cookie();value.revision=broker->lifecycle_revision();
    value.registered=broker->registration_current(value.cookie,value.native_started);
    value.terminal=broker->draining();
    value.unsafe=broker->pending_callbacks()||broker->pending_transactions()||broker->pending_aux_scopes()||broker->aux_debt()||
        broker->pending_action_scopes()||broker->pending_external_scopes()||broker->pending_action_invalidation()||
        !gears_native_context::engine_context_clear(gears_native_context::thread_ledger,
            value.game_thread&&value.current_thread==value.game_thread&&value.current_thread!=value.worker_thread);
    return value;
}
hand::ObjectIdentity object(gears_anchor::Identity const& value) noexcept {
    return value.address?hand::ObjectIdentity{value.address,value.outer,value.cls,value.index,value.name_index,value.number,1}:hand::ObjectIdentity{};
}
bool actor(gears_weapon_hand::EquipmentIdentity const& parent,scene::Binding& expected) {
    std::unique_lock<std::mutex> lock(scene::mutex,std::try_to_lock);
    if(!lock.owns_lock()||!scene::metadata_current()||!scene::proofs_current(true)||
       parent.key.world_generation!=committed_world_generation.load())return false;
    scene::Binding observed{};
    const auto read=[&](hand::ObjectIdentity const& wanted,gears_anchor::Identity& actual){
        return scene::identity(wanted.address,actual,true)&&hand::same(object(actual),wanted);
    };
    if(!read(parent.world,observed.world)||!read(parent.controller,observed.controller)||!read(parent.pawn,observed.pawn)||
       !read(parent.mesh,observed.mesh)||!read(parent.weapon,observed.weapon)||
       !scene::field(observed.special_move,observed.pawn.address,scene::base_proof.fields[4].offset)||
       !scene::metadata_current()||!scene::proofs_current(true)||parent.key.world_generation!=committed_world_generation.load())return false;
    observed.world_generation=parent.key.world_generation;observed.weapon_known=true;observed.context_valid=true;
    expected=observed;return true;
}
std::uint32_t query(api::Request const* input) noexcept {
    try {
        // Existing Scope also accounts for an active Native export while the
        // worker drains. Enter before inspecting any runtime/metadata state.
        Scope cleanup_scope;if(!cleanup_scope.world())return unsigned(api::Result::unavailable);
        api::Request request{},again{};
        if(!input||!safe_copy(&request,input,sizeof request)||!policy::shape(request))return unsigned(api::Result::rejected);
        const auto first=runtime();const auto admitted=policy::classify(request,first);
        if(admitted!=api::Result::same_incarnation)return unsigned(admitted);
        // Scope pins the actual existing Native world epoch across every read;
        // committed generation and live registration are closed again below.
        scene::Binding actual{};
        if(!actor(request.parent,actual)||!scene::current_actor_retained(actual))return unsigned(api::Result::rejected);
        const auto second=runtime();
        if(!policy::same(first,second)||policy::classify(request,second)!=api::Result::same_incarnation||
           !scene::current_actor_retained(actual)||!safe_copy(&again,input,sizeof again)||std::memcmp(&request,&again,sizeof request))
            return unsigned(api::Result::rejected);
        const auto last=runtime();
        return unsigned(policy::same(second,last)&&policy::classify(request,last)==api::Result::same_incarnation?
            api::Result::same_incarnation:api::Result::rejected);
    }catch(...){return unsigned(api::Result::unavailable);}
}
#else
std::uint32_t query(api::Request const*) noexcept {return unsigned(api::Result::unavailable);}
#endif
}
