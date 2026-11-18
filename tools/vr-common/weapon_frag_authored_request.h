#pragma once
#include "weapon_frag_grasp_pose.h"
#include "hand_authored_grasp.h"

// Forms only the existing public cosmetic request. The production caller must
// independently certify this exact Frag class/mesh/hold, and Native repeats
// broker/current-scene/reference/physical-input admission before consumption.
// No Native export is called here and no action or skeleton permission exists.
namespace gears_weapon_frag_authored {
namespace api=gears_hand_authored_grasp;
namespace hand=gears_weapon_hand;
inline bool request(hand::Frame const& source,
    gears_hand_authored_mount::Signature const& signature,api::Request& out) noexcept {
    constexpr unsigned required=hand::scene_owned|hand::input_matched|hand::gameplay_interactable;
    const auto& right=source.controllers.hands[1];
    if((source.flags&required)!=required||source.controllers.focused!=1||
       source.now_ns<=0||source.controllers.sync_ns<=0||source.now_ns<source.controllers.sync_ns||
       source.now_ns-source.controllers.sync_ns>100000000||
       !hand::identity_valid(source.weapon)||
       right.squeeze_active!=1||!hand::control_held(right,gears_vr::control_squeeze)||
       !gears_vr::controller_pose_usable(right.grip,right.grip_flags,right.grip_active))return false;
    api::Request candidate{};
    if(!gears_weapon_frag_grasp_pose::read(signature,candidate.reference,candidate.closed))return false;
    candidate.source=source;
    candidate.geometry_signature=signature;
    candidate.profile_id=gears_weapon_frag_grasp_pose::profile_id;
    candidate.side=api::Side::right;
    candidate.mode=api::Mode::right_main_fingers;
    candidate.kind=api::object::Kind::none;
    // Identity grip and absent trigger endpoint are prescribed by the public
    // main-finger format. Neither index input nor a cosmetic snapped hand can
    // become the release/trajectory source.
    if(!api::structural(candidate))return false;
    out=candidate;return true;
}
}
