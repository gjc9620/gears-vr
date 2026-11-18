#pragma once
#include <cstdint>

#include "weapon_hand_participant.h"
#include "weapon_pose_solver.h"
#include "rig_tracking_frame.h" // gears_vr::rig_tracking::detail::quaternion

namespace gears_boltok_proposal_projection {
namespace hand=gears_weapon_hand;
using P=gears_vr::Pose;
using V=gears_vr::Vector;

// Numeric geometric hint only. It is never an action, clock or Native grant.
struct Hint {
    P root{};
    hand::FrameKey key{};
    std::uint64_t sequence{},profile{};
    std::int64_t sync{};
    bool known{};

    void clear() noexcept {*this={};}

    bool capture(hand::Frame const& f,P const& relative_root) noexcept {
        if(!gears_vr::stereo_pose_valid(relative_root)||!f.key.frame||
           !f.controllers.sequence||!f.controllers.sync_ns){
            clear();return false;
        }
        root=relative_root;key=f.key;sequence=f.controllers.sequence;
        profile=f.controllers.profile_generation;
        sync=f.controllers.sync_ns;known=true;return true;
    }

    bool project(hand::Frame const& f,std::int64_t now,V& out,unsigned* failure=nullptr) const noexcept {
        const auto reject=[&](unsigned reason){if(failure)*failure=reason;return false;};
        if(failure)*failure=0;
        static constexpr std::int64_t maximum_age_ns=100000000;
        if(!known||now<sync||now-sync>maximum_age_ns)return reject(1);
        const auto current_sync=f.controllers.sync_ns;
        if(current_sync<=0||current_sync<sync||current_sync>now||
           now-current_sync>maximum_age_ns)return reject(2);
        if(f.controllers.sequence<sequence||
           f.controllers.profile_generation!=profile||
           f.key.frame<key.frame)return reject(3);
        auto expected=key;expected.frame=f.key.frame;
        if(!hand::same(expected,f.key))return reject(3);

        P raw[2]{};
        for(unsigned i=0;i<2;++i){
            const auto& h=f.controllers.hands[i];
            if(!gears_vr::controller_pose_usable(
                   h.grip,h.grip_flags,h.grip_active))return reject(4);
            const auto basis=gears_vr::xr_to_ue(
                gears_vr::rotation_matrix(h.grip.orientation));
            if(!gears_vr::rig_tracking::detail::quaternion(
                   basis,raw[i].orientation))return reject(4);
            raw[i].position=gears_vr::xr_to_ue(h.grip.position);
        }

        const P world_root{
            gears_weapon::sum(raw[1].position,
                gears_weapon::rotate(raw[1].orientation,root.position)),
            gears_vr::normalized(gears_vr::multiply(
                raw[1].orientation,root.orientation))};
        if(!gears_vr::stereo_pose_valid(world_root))return reject(5);
        out=gears_weapon::rotate(gears_vr::inverse(world_root.orientation),
            gears_weapon::difference(raw[0].position,world_root.position));
        return gears_vr::finite(out)||reject(5);
    }
};
} // namespace gears_boltok_proposal_projection

