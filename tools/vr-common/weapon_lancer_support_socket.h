#pragma once
#include "hand_object_grasp.h"
#include "hand_authored_mount_profile.h"
#include "weapon_two_hand_hold.h"
#include "weapon_main_mount_provider.h"
#include <type_traits>
// Fixed geometry registration, NOT a tracked pose or a renewable action grant.
// V2/V3/V4 wire contracts remain unchanged. Every use needs a NEW exact Native
// broker frame, the same currently admitted main mount, and current raw hands.
namespace gears_lancer_support_socket {
namespace hand=gears_weapon_hand;namespace vr=gears_vr;
struct Descriptor {
    unsigned size{sizeof(Descriptor)},version{5};
    hand::Frame binding{};
    hand::ObjectIdentity component{},asset{};
    std::uint64_t hold_generation{};
    gears_hand_authored_mount::Signature geometry{};
    double model_scale{},units_per_metre{};
    vr::Pose in_main_grip{}; // metres; static GripPoint in certified main Grip.
};
inline constexpr char publish_export[]="GearsVrRegisterLancerSupportSocketV5";
// Separate optional export, same descriptor layout. The starter consumer
// additionally requires the original C owner at evaluated deployed/rest pose.
inline constexpr char starter_publish_export[]="GearsVrRegisterLancerStarterSocketV5";
#if defined(_WIN32)
using Publish=bool(__cdecl*)(Descriptor const*) noexcept;
#endif
inline bool structural(Descriptor const& d) noexcept {
    return d.size==sizeof d&&d.version==5&&hand::frame_shape_valid(d.binding)&&hand::identity_valid(d.binding.weapon)&&
        hand::identity_valid(d.component)&&hand::identity_valid(d.asset)&&d.hold_generation&&
        d.geometry==gears_hand_authored_mount::marcus115_signature&&
        std::isfinite(d.model_scale)&&d.model_scale>=.1&&d.model_scale<=10&&
        std::isfinite(d.units_per_metre)&&d.units_per_metre>=10&&d.units_per_metre<=500&&
        gears_weapon_two_hand::geometry(d.in_main_grip);
}
inline bool binding_matches(Descriptor const& d,hand::Frame const& f) noexcept {
    const auto& a=d.binding;
    // Only static registration identities. NEVER copy/renew a.now_ns, the
    // input sequence/sync time, a world hand pose or an old frame permit.
    return structural(d)&&hand::frame_shape_valid(f)&&
        a.key.game_pid==f.key.game_pid&&a.key.game_start==f.key.game_start&&
        a.key.native_started_qpc==f.key.native_started_qpc&&a.key.registration_cookie==f.key.registration_cookie&&
        a.key.lifecycle==f.key.lifecycle&&a.key.world_generation==f.key.world_generation&&
        hand::same(a.world,f.world)&&hand::same(a.controller,f.controller)&&hand::same(a.pawn,f.pawn)&&
        hand::same(a.mesh,f.mesh)&&hand::same(a.weapon,f.weapon)&&a.reference_generation==f.reference_generation&&
        a.stereo.session==f.stereo.session&&a.stereo.space_epoch==f.stereo.space_epoch&&a.stereo.source==f.stereo.source&&
        a.controllers.input_epoch==f.controllers.input_epoch&&a.controllers.profile_generation==f.controllers.profile_generation;
}
inline bool current_mount_matches(Descriptor const& d,hand::Frame const& f,
    gears_weapon_main_mount_provider::Response const& m) noexcept {
    namespace mount=gears_weapon_main_mount_provider;
    return structural(d)&&m.size==sizeof m&&m.abi_version==mount::version&&m.query_id&&
        hand::same(m.key,f.key)&&hand::same(m.world,f.world)&&hand::same(m.controller,f.controller)&&
        hand::same(m.pawn,f.pawn)&&hand::same(m.mesh,f.mesh)&&hand::same(m.weapon,f.weapon)&&
        hand::same(m.component,d.component)&&hand::same(m.asset,d.asset)&&m.hold_generation==d.hold_generation&&
        m.geometry==d.geometry&&m.model_scale==d.model_scale&&m.units_per_metre==d.units_per_metre;
}
static_assert(std::is_trivially_copyable_v<Descriptor>);
}
