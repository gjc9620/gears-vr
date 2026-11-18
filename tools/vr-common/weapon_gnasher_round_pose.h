#pragma once
#include "weapon_gnasher_mechanics.h"

// Authored closed shell. The loader bakes authored +Z into engine-local +Y.
// These dimensions describe the existing prototype, not accepted chamber fit.
namespace gears_gnasher_round_pose {
namespace vr=gears_vr;namespace weapon=gears_weapon;
// Actual v2 insert_tip marker and vertex envelope:113.45mm. The README's
//113.5mm is rounded; it must not become a separate physical insertion point.
inline constexpr double authored_length_metres=.11345;
inline constexpr double authored_radius_metres=.01685;
inline constexpr unsigned material_slots=5; // Actual catalog sections: 0,2,3,4.
struct Mount {
    std::uint64_t calibration_id{},reference_generation{};
    vr::Pose root_in_grip_metres{};
    bool valid() const noexcept {
        return calibration_id&&reference_generation&&vr::stereo_pose_valid(root_in_grip_metres)&&
            weapon::length(root_in_grip_metres.position)<.4;
    }
    vr::Pose tip_in_grip_metres() const noexcept {
        auto tip=root_in_grip_metres;
        tip.position=weapon::sum(tip.position,weapon::rotate(tip.orientation,{0,authored_length_metres,0}));
        return tip;
    }
    bool matches_feed(weapon::GnasherMechanicalConfig const& config) const noexcept {
        if(!valid()||!config.valid()||!config.feed_calibrated||config.calibration_id!=calibration_id)return false;
        const auto tip=tip_in_grip_metres();const auto& expected=config.feed.item_in_left_grip;
        const auto a=vr::normalized(tip.orientation),b=vr::normalized(expected.orientation);
        return weapon::length(weapon::difference(tip.position,expected.position))<1e-9&&
            std::abs(std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)-1)<1e-9;
    }
};
inline bool world_root(Mount const& mount,vr::Pose const& raw_left_grip_engine,
    double units,vr::Pose& out) noexcept {
    out={};
    if(!mount.valid()||!vr::stereo_pose_valid(raw_left_grip_engine)||
       !std::isfinite(units)||units<10||units>500)return false;
    out.position=weapon::sum(raw_left_grip_engine.position,
        weapon::rotate(raw_left_grip_engine.orientation,weapon::scaled(mount.root_in_grip_metres.position,units)));
    out.orientation=vr::normalized(vr::multiply(raw_left_grip_engine.orientation,mount.root_in_grip_metres.orientation));
    return vr::stereo_pose_valid(out);
}
} // namespace gears_gnasher_round_pose
