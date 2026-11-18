#pragma once
#include "weapon_boltok_owner_profiles.h"
#include "weapon_boltok_visual_geometry.h"
#include "weapon_part_geometry.h"

// Static owner capture to physical Grip mapping for the WHOLE loading cylinder.
// This pure transform is neither a current tracking sample nor an action permit.
namespace gears_boltok_cylinder_grasp {
using P=gears_vr::Pose;
inline constexpr gears_vr::Vector feed_raw{0,17.672944259643554,8.9682669639587385};
// Task211: original native source -> actual workbench reload-cylinder frame.
// Proper 180deg Y rotation, with measured source length translation. This is
// applied once to immutable geometry, including tangent basis and fresh bounds.
inline gears_weapon::part_geometry::Result prepare_mesh(
    gears_weapon::part_geometry::Prepared const& source,double component_scale,
    gears_weapon::part_geometry::Prepared& out) {
    namespace g=gears_weapon::part_geometry;
    if(&source==&out||!std::isfinite(component_scale)||component_scale<=0||component_scale>=100)
        return g::Result::invalid_conversion;
    auto transformed=source;
    const g::Conversion turn{{{{-1,0,0},{0,1,0},{0,0,-1}}},100,source.calibration_generation,true};
    for(auto& v:transformed.vertices){
        for(auto* packed:{&v.tangent_x,&v.tangent_y,&v.normal}){
            g::V unpacked{};if(!g::unpack(*packed,unpacked))return g::Result::invalid_vertex;
            *packed=g::pack(g::rotate(turn,unpacked),(*packed)[3]);
        }
        v.position[0]=-v.position[0];v.position[2]=-v.position[2];
        v.position[1]=static_cast<float>(v.position[1]+.06299999654293051*component_scale);
    }
    // merge_rigid recalculates outward-rounded bounds from transformed bytes.
    const std::array<g::Prepared,1> parts{{std::move(transformed)}};
    return g::merge_rigid(parts,out);
}
inline P compose(P const& a,P const& b) noexcept {
    return {gears_weapon::sum(a.position,gears_weapon::rotate(a.orientation,b.position)),
        gears_vr::normalized(gears_vr::multiply(a.orientation,b.orientation))};
}
inline bool item_in_grip(double physical_scale,P& out) noexcept {
    if(!std::isfinite(physical_scale)||physical_scale<=0||physical_scale>100)return false;
    gears_hand_authored_mount::Mount left{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
       gears_hand_authored_mount::Side::left,1,100,left))return false;
    auto const& source=gears_boltok_owner_profiles::ammo.wrist_raw_object;
    const auto wrist=gears_weapon::scaled(gears_weapon::difference(source.position,feed_raw),physical_scale/100.);
    const auto q=gears_vr::normalized(gears_vr::multiply(source.orientation,
        gears_vr::inverse(left.calibration.grip_to_wrist_rotation)));
    // Controller palm marker is physical; only the captured model offset scales.
    const auto grip=gears_weapon::difference(wrist,gears_weapon::rotate(q,left.calibration.grip_to_wrist_metres));
    const auto inverse=gears_vr::inverse(q);
    P candidate{gears_weapon::rotate(inverse,gears_weapon::scaled(grip,-1)),inverse};
    if(!gears_vr::stereo_pose_valid(candidate))return false;
    out=candidate;return true;
}
}
