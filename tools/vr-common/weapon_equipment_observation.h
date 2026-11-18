#pragma once
#include "weapon_support_grip.h"

namespace gears_weapon_hand {
// Query-local ancestry and metadata only. This is neither a Hand Frame nor a
// tracking/ownership permit. Publication/binding/ticket belong to Native.
struct EquipmentIdentity {
    FrameKey key{};
    ObjectIdentity world{},controller{},pawn{},mesh{},weapon{};
    gears_vr::Pose reference{};
    std::uint64_t reference_generation{};
};
struct EquipmentObservation {
    std::int64_t begin_ns{},end_ns{};
    GripGameState game{};
    std::uint64_t hold_generation{};
    ObjectIdentity component{},asset{}; bool boomshot{};
    bool boltok{}; // Internal classification only; never a Frame or authority.
};
// Read-only association for recording completed motion. This is deliberately
// not an actionable Frame or a fresh game-state observation. Its original
// clocks/counters must be compared with independently current Native geometry;
// only the normal action consumer may dispatch or change inventory.
struct RetainedItemInput {
    EquipmentIdentity parent{};
    Lease lease{};
    std::uint64_t press_token{},controller_sequence{};
    gears_vr::StereoSource source{};
    std::uint64_t session{},space{},input{},profile{};
    std::int64_t original_input_ns{},decision_ns{};
    std::uint32_t pressed{},released{},interaction_control{};
};
inline bool equipment_identity_valid(EquipmentIdentity const& i) noexcept {
    const auto& k=i.key;
    return k.game_start&&k.native_started_qpc&&k.registration_cookie&&k.lifecycle&&k.frame&&
        k.world_generation&&k.game_pid&&k.thread_id&&identity_valid(i.world)&&identity_valid(i.controller)&&
        identity_valid(i.pawn)&&identity_valid(i.mesh)&&identity_valid(i.weapon,true)&&
        i.reference_generation&&gears_vr::stereo_pose_valid(i.reference);
}
inline bool equipment_matches(Frame const& f,EquipmentIdentity const& i) noexcept {
    return same(f.key,i.key)&&same(f.world,i.world)&&same(f.controller,i.controller)&&same(f.pawn,i.pawn)&&
        same(f.mesh,i.mesh)&&same(f.weapon,i.weapon)&&f.reference_generation==i.reference_generation&&
        gears_vr::stereo_same_pose(f.reference,i.reference);
}
} // namespace gears_weapon_hand
