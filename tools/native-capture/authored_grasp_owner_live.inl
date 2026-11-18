// Included after the existing C0 and Lancer mechanism owners. Only reuse their
// actual already admitted actor/class context and original additional checks.
#include "authored_grasp_owner_policy.h"
namespace authored_grasp_live {
bool lancer_support_owner_current(gears_lancer_support_socket::Descriptor const& d,gears_weapon_hand::Frame const& f) noexcept {
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
    namespace hand=gears_weapon_hand;const auto& a=full_body_c0_native_live::state.context.actor;
    const auto& m=full_body_c0_native_live::state.mount_response;
    return a.context_valid&&a.weapon_known&&a.weapon_class_known&&a.weapon_is_lancer&&
        a.world_generation==f.key.world_generation&&hand::same(rig_tracking_live::object(a.world),f.world)&&
        hand::same(rig_tracking_live::object(a.pawn),f.pawn)&&hand::same(rig_tracking_live::object(a.mesh),f.mesh)&&
        hand::same(rig_tracking_live::object(a.weapon),f.weapon)&&
        gears_lancer_support_socket::current_mount_matches(d,f,m);
#else
    (void)d;(void)f;return false;
#endif
}
bool boltok_close_owner_current(api::Request const& q) noexcept {
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
    namespace hand=gears_weapon_hand;namespace binding=weapon_hand_scene_binding;
    namespace visibility=weapon_part_visibility_live;
    if(!boltok_close_request(q))return false;
    const auto& f=q.source;const auto& a=full_body_c0_native_live::state.context.actor;
    if(!a.context_valid||!a.weapon_known||!a.weapon_class_known||a.world_generation!=f.key.world_generation||
       !hand::same(rig_tracking_live::object(a.world),f.world)||!hand::same(rig_tracking_live::object(a.pawn),f.pawn)||
       !hand::same(rig_tracking_live::object(a.mesh),f.mesh)||!hand::same(rig_tracking_live::object(a.weapon),f.weapon))return false;
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
    return lock.owns_lock()&&binding::metadata_current()&&visibility::object(f.weapon)&&
        visibility::derives(f.weapon,L"Weap_LocustPistol",L"WarfareGame")&&visibility::object(f.weapon);
#else
    (void)q;return false;
#endif
}
bool additional_owner_current(api::Request const& q) noexcept {
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
    namespace policy=gears_authored_grasp_owner;
    auto const& a=full_body_c0_native_live::state.context.actor;
    policy::Actor actor{rig_tracking_live::object(a.world),rig_tracking_live::object(a.pawn),
        rig_tracking_live::object(a.mesh),rig_tracking_live::object(a.weapon),a.world_generation,
        !a.weapon_class_known?policy::Family::unknown:
            a.weapon_is_lancer?policy::Family::lancer:policy::Family::other,a.context_valid,a.weapon_known};
    switch(policy::required(q,actor)){
    case policy::Need::none:return true;
    case policy::Need::lancer_loading:return lancer_handle_morph_live::grasp_owner_current(q.source);
    case policy::Need::lancer_starter:return lancer_grasp_live::owner_current(api::object::Kind::starter,q.source);
    default:return false;
    }
#else
    (void)q;return false;
#endif
}
}

namespace lancer_grasp_live {bool socket_owner_current(socket::Descriptor const& d,hand::Frame const& f) noexcept {return authored_grasp_live::lancer_support_owner_current(d,f);}}
namespace lancer_grasp_live {
bool starter_socket_binding_current(socket::Descriptor const& d,hand::Frame const& f) noexcept {
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
    namespace c=lancer_original_handle_live;
    return owner_current(api::Kind::starter,f)&&
        GetCurrentThreadId()==c::owner_thread.load(std::memory_order_acquire)&&
        hand::same(d.component,c::bound.component)&&hand::same(d.asset,c::bound.asset)&&
        !c::drain_requested.load(std::memory_order_acquire)&&c::held_current()&&
        c::bound.pose.wanted()==c::api::maximum_angle&&c::bound.pose.wanted_pull()==0.;
#else
    (void)d;(void)f;return false;
#endif
}
bool starter_socket_owner_current(socket::Descriptor const& d,hand::Frame const& f) noexcept {
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
    namespace c=lancer_original_handle_live;
    // Registration stores geometry only. The current main-mount response
    // exists at this actual C0 consumption boundary, not at the PG callback.
    if(!authored_grasp_live::lancer_support_owner_current(d,f)||!starter_socket_binding_current(d,f))return false;
    // Retained mechanical intent is not proof of an evaluated crossbar.
    // Reuse the original owner's read-only live bracket and ready predicate.
    c::api::Source observed{};
    return c::snapshot(observed)&&c::bound.pose.ready(observed,[](auto at,auto& value){return c::read(at,value);});
#else
    (void)d;(void)f;return false;
#endif
}
}
