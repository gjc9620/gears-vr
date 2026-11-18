#pragma once
#include "pose_math.h"

// Longshot chamber alignment geometry. The caller owns current source, open
// breech, held LIVE item and lease. This produces no insertion/inventory receipt:
// raw axial travel, speed and approach-before-seat remain independently required.
namespace gears_weapon_longshot_chamber_snap {
namespace vr=gears_vr;
using V=vr::Vector;using P=vr::Pose;using Q=vr::Quaternion;
inline constexpr double enter_radius_m=.06,keep_radius_m=.08;
inline constexpr double enter_depth_m=.08,keep_depth_m=.10;
inline constexpr double maximum_angle_rad=3.14159265358979323846/6;
inline constexpr double cartridge_length_m=.15;
struct Alignment {P item{};V raw_tip_local_m{};double raw_depth_m{},raw_lateral_m{},raw_angle_rad{};};
inline V add(V a,V b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V sub(V a,V b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V mul(V a,double n) noexcept {return {a.x*n,a.y*n,a.z*n};}
inline bool pose_valid(P const& p) noexcept {
 const auto q=p.orientation;const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
 return vr::finite(p.position)&&std::isfinite(n)&&std::abs(n-1)<1e-5;
}
inline bool align(P const& raw,P const& root,V port_local_m,double model_scale,double units,
                  bool keeping,Alignment& out) noexcept {
 if(!pose_valid(raw)||!pose_valid(root)||!vr::finite(port_local_m)||
    !std::isfinite(model_scale)||model_scale<=.1||model_scale>=10||
    !std::isfinite(units)||units<10||units>500)return false;
 const auto root_matrix=vr::rotation_matrix(root.orientation);
 const auto axis=vr::transform(vr::rotation_matrix(raw.orientation),{0,1,0});
 const auto tip=add(raw.position,mul(axis,cartridge_length_m*units));
 const auto local=mul(vr::transform(vr::transpose(root_matrix),sub(tip,root.position)),1/(units*model_scale));
 const auto local_axis=vr::transform(vr::transpose(root_matrix),axis);
 const double angle=std::acos(std::clamp(local_axis.x,-1.,1.));
 // Port coordinates belong to the scaled model; interaction distances are
 // physical metres. Scaling a mesh must not enlarge or shrink the hand zone.
 const auto delta=sub(local,port_local_m);const double depth=-delta.x*model_scale,lateral=std::hypot(delta.y,delta.z)*model_scale;
 if(!vr::finite(local)||!std::isfinite(angle)||!std::isfinite(depth)||!std::isfinite(lateral)||lateral>(keeping?keep_radius_m:enter_radius_m)||
    depth<-.015||depth>(keeping?keep_depth_m:enter_depth_m)||angle>maximum_angle_rad)return false;
 // Minimal swing aligns +Y cartridge axis to +X breech axis and preserves roll.
 const auto forward=vr::transform(root_matrix,{1,0,0});
 const auto cross=V{axis.y*forward.z-axis.z*forward.y,axis.z*forward.x-axis.x*forward.z,axis.x*forward.y-axis.y*forward.x};
 const double dot=axis.x*forward.x+axis.y*forward.y+axis.z*forward.z;
 const auto swing=vr::normalized(Q{cross.x,cross.y,cross.z,1+dot});
 const auto rotation=vr::normalized(vr::multiply(swing,raw.orientation));
 const auto target_tip=add(root.position,vr::transform(root_matrix,mul(V{local.x,port_local_m.y,port_local_m.z},units*model_scale)));
 const auto primer=sub(target_tip,mul(forward,cartridge_length_m*units));
 if(!vr::finite(primer))return false;
 out={{primer,rotation},local,depth,lateral,angle};return true;
}
inline bool grip_for_item(P const& item,P const& item_in_grip,P& grip) noexcept {
 if(!pose_valid(item)||!pose_valid(item_in_grip))return false;
 const auto rotation=vr::normalized(vr::multiply(item.orientation,vr::inverse(item_in_grip.orientation)));
 const auto offset=vr::transform(vr::rotation_matrix(rotation),item_in_grip.position);
 const auto position=sub(item.position,offset);if(!vr::finite(position))return false;
 grip={position,rotation};return true;
}
inline bool tip_in_grip_m(P const& item_in_grip,double units,V& out) noexcept {
 if(!pose_valid(item_in_grip)||!std::isfinite(units)||units<10||units>500)return false;
 const auto axis=vr::transform(vr::rotation_matrix(item_in_grip.orientation),{0,cartridge_length_m,0});
 const auto tip=add(mul(item_in_grip.position,1/units),axis);
 if(!vr::finite(tip))return false;out=tip;return true;
}
// A bounded COSMETIC correction, not a contact/lease/insertion permit. Its
// owner may decay it after leaving the zone while measuring current raw depth.
struct Correction {V tip_offset_m{};Q rotation_in_root{};};
inline bool make_correction(P const& raw,P const& root,V port,double scale,double units,
                           bool keeping,Correction& out) noexcept {
 Alignment goal{};if(!align(raw,root,port,scale,units,keeping,goal))return false;
 const auto swing=vr::multiply(goal.item.orientation,vr::inverse(raw.orientation));
 const auto local=vr::normalized(vr::multiply(vr::multiply(vr::inverse(root.orientation),swing),root.orientation));
 const auto delta=sub(goal.raw_tip_local_m,port);
 out={{0,-delta.y*scale,-delta.z*scale},local};return true;
}
inline bool apply_correction(P const& raw,P const& root,V port,double scale,double units,
    Correction const& correction,double amount,Alignment& out) noexcept {
 if(!pose_valid(raw)||!pose_valid(root)||!vr::finite(port)||!std::isfinite(scale)||scale<=.1||scale>=10||
    !std::isfinite(units)||units<10||units>500||!std::isfinite(amount)||amount<0||amount>1||
    !vr::finite(correction.tip_offset_m)||correction.tip_offset_m.x!=0||
    std::hypot(correction.tip_offset_m.y,correction.tip_offset_m.z)>keep_radius_m+1e-12||
    !pose_valid(P{{},correction.rotation_in_root}))return false;
 auto q=correction.rotation_in_root;if(q.w<0)q={-q.x,-q.y,-q.z,-q.w};
 if(2*std::acos(std::clamp(q.w,0.,1.))>maximum_angle_rad+1e-12)return false;
 const double t=amount*amount*(3-2*amount);
 const auto partial=vr::normalized(Q{q.x*t,q.y*t,q.z*t,1+(q.w-1)*t});
 const auto world_swing=vr::multiply(vr::multiply(root.orientation,partial),vr::inverse(root.orientation));
 const auto orientation=vr::normalized(vr::multiply(world_swing,raw.orientation));
 const auto matrix=vr::rotation_matrix(root.orientation);
 const auto raw_axis=vr::transform(vr::rotation_matrix(raw.orientation),{0,1,0});
 const auto raw_tip=add(raw.position,mul(raw_axis,cartridge_length_m*units));
 const auto local=mul(vr::transform(vr::transpose(matrix),sub(raw_tip,root.position)),1/(scale*units));
 const auto delta=sub(local,port);
 const double depth=-delta.x*scale,lateral=std::hypot(delta.y,delta.z)*scale;
 const auto local_axis=vr::transform(vr::transpose(matrix),raw_axis);
 const double angle=std::acos(std::clamp(local_axis.x,-1.,1.));
 const auto tip=add(raw_tip,vr::transform(matrix,mul(correction.tip_offset_m,t*units)));
 const auto axis=vr::transform(vr::rotation_matrix(orientation),{0,1,0});
 const auto primer=sub(tip,mul(axis,cartridge_length_m*units));
 if(!vr::finite(local)||!vr::finite(primer)||!std::isfinite(depth)||!std::isfinite(lateral)||!std::isfinite(angle))return false;
 out={{primer,orientation},local,depth,lateral,angle};return true;
}
}
