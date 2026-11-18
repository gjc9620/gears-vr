#pragma once
#include "hand_wrist_calibration.h"
#include <array>

// Bow-only original-arrow geometry. Measurements are from the pinned static
// export, not a projectile actor or a deformed/snapped hand. Runtime admission
// still authenticates the original component, current rig units and raw hands.
namespace gears_torque_bow_arrow_mount {
namespace vr=gears_vr;using V=vr::Vector;using P=vr::Pose;using Q=vr::Quaternion;
inline constexpr char export_sha256[]="c06788de423217fd5b2a89fe64606271699e7aaa29e7534cd407e682f0c98a16";
inline constexpr V forward_raw{0,-1,0};
inline constexpr V tip_raw{0,-18.724075317382812,-.007449000142514706};
// Actual triangle/plane intersection at Y4.2, Z0 on the central shaft.
inline constexpr V pad_contact_raw{.9953870763372161,4.2,0};
// Original Marcus Bow AR_Reload key25, brought to the raw-reference bone
// convention. This replaces the unused +wristX draft. It is an original
// minimum mount, not acceptance of finger/cuff surfaces or headset comfort.
inline constexpr P original_source_in_wrist_raw{{5.159653220216221,-5.457132694044283,4.357984655762095},
    {-.49355128320390024,.6048463624405919,.6212042458823152,-.06836149199624011}};
inline constexpr Q arrow_in_wrist=original_source_in_wrist_raw.orientation;
inline V add(V a,V b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V sub(V a,V b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V mul(V a,double n) noexcept {return {a.x*n,a.y*n,a.z*n};}
inline V rotate(Q q,V p) noexcept {return vr::transform(vr::rotation_matrix(q),p);}
inline bool pose_valid(P const& p) noexcept {
 const auto q=p.orientation;const auto n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
 return vr::finite(p.position)&&std::isfinite(n)&&std::abs(n-1)<1e-5;
}
struct Mount {
 P source_in_grip_units{},tip_in_grip_units{};
 V raw_tip_in_grip_m{};
 double component_scale{},units_per_metre{};
};
inline bool author(gears_hand_wrist::Calibration const& wrist,V pad_in_wrist_m,
 double component_scale,double units,Mount& out) noexcept {
 if(!wrist.known||!vr::finite(wrist.grip_to_wrist_metres)||!vr::finite(pad_in_wrist_m)||
    !pose_valid({{},wrist.grip_to_wrist_rotation})||!std::isfinite(component_scale)||component_scale<=.1||component_scale>=10||
    !std::isfinite(units)||units<10||units>500)return false;
 // Keep the component's actual uniform scale. Only the rigid mount changes;
 // no vertex/bone/arrow length is stretched to reach the feed slot.
 const auto local=mul(pad_contact_raw,component_scale/units);
 const auto centre_in_wrist=sub(pad_in_wrist_m,rotate(arrow_in_wrist,local));
 const auto centre_in_grip=add(wrist.grip_to_wrist_metres,rotate(wrist.grip_to_wrist_rotation,centre_in_wrist));
 const auto orientation=vr::normalized(vr::multiply(wrist.grip_to_wrist_rotation,arrow_in_wrist));
 const auto tip_in_grip=add(centre_in_grip,rotate(orientation,mul(tip_raw,component_scale/units)));
 Mount m{{mul(centre_in_grip,units),orientation},{mul(tip_in_grip,units),orientation},tip_in_grip,component_scale,units};
 if(!pose_valid(m.source_in_grip_units)||!pose_valid(m.tip_in_grip_units)||!vr::finite(m.raw_tip_in_grip_m))return false;
 out=m;return true;
}
inline bool author_original(gears_hand_wrist::Calibration const& wrist,double avatar_scale,
 double component_scale,double units,Mount& out) noexcept {
 if(!std::isfinite(avatar_scale)||avatar_scale<=.1||avatar_scale>=10||!std::isfinite(units)||units<10||units>500)return false;
 const auto pad_raw=add(original_source_in_wrist_raw.position,rotate(arrow_in_wrist,pad_contact_raw));
 // The palm landmark follows the authenticated avatar. The arrow keeps its
 // actual component scale, independently; it is never resized to fit a hand.
 return author(wrist,mul(pad_raw,avatar_scale/units),component_scale,units,out);
}
// A snap/slot frame refers to the same physical tip as the raw motion reader.
// Reconstruct the original component centre only after geometry alignment;
// this does not advance its axial coordinate or create a mechanical receipt.
inline bool source_from_tip(P const& tip,double component_scale,P& source) noexcept {
 if(!pose_valid(tip)||!std::isfinite(component_scale)||component_scale<=.1||component_scale>=10)return false;
 source={sub(tip.position,rotate(tip.orientation,mul(tip_raw,component_scale))),tip.orientation};
 return pose_valid(source);
}
}
