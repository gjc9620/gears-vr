#pragma once
#include "weapon_torque_bow_artist_grasp.h"
#include "weapon_torque_bow_arrow_mount.h"
#include "weapon_mesh_follow.h"
#include "weapon_two_hand_hold.h"
#include "rig_tracking_math.h"
// Original minimum authoring on the fixed Marcus115 MODEL. Native independently
// matches every reference local and owns all C0 writes. No skin-fit acceptance.
namespace gears_torque_bow_grasp_pose {
namespace vr=gears_vr;namespace artist=gears_torque_bow_artist_grasp;namespace api=gears_hand_authored_grasp;
namespace am=gears_torque_bow_arrow_mount;using Joints=api::Joints;
enum class Part:unsigned {main_grip,support,arrow_shaft,arrow_feed};
struct Profile {std::uint64_t id{};unsigned side{};Joints closed{},trigger_closed{};bool has_trigger_endpoint{};};
inline bool author(Part part,Joints const& reference,Profile& out) noexcept {
 if(unsigned(part)>unsigned(Part::arrow_feed))return false;
 const bool right=part==Part::main_grip;const auto& expected=right?artist::reference_right:artist::reference_left;
 for(unsigned i=0;i<15;++i){
  if(!api::unit(reference[i]))return false;const auto a=reference[i],b=expected[i];
  const double sign=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0?-1.:1.;
  if(std::abs(a.x-sign*b.x)>1e-6||std::abs(a.y-sign*b.y)>1e-6||std::abs(a.z-sign*b.z)>1e-6||std::abs(a.w-sign*b.w)>1e-6)return false;
 }
 Profile p{};p.id=0x54424f5700000200ULL+unsigned(part);p.side=right?1:0;p.has_trigger_endpoint=right;
 p.closed=right?artist::main_rest:part==Part::support?artist::support:part==Part::arrow_shaft?artist::shaft:artist::feed;
 if(right)p.trigger_closed=artist::main_trigger;
 for(auto q:p.closed)if(!api::unit(q))return false;
 out=p;return true;
}
inline bool request(Part part,api::Signature const& actual,Joints const& reference,
 gears_weapon_hand::Frame const& source,vr::Pose const& grip,bool proximity,api::Request& out) noexcept {
 if(actual!=gears_hand_authored_mount::marcus115_signature)return false;
 Profile p{};if(!author(part,reference,p))return false;
 api::Request q{};q.source=source;q.geometry_signature=actual;q.profile_id=p.id;
 q.side=p.side?api::Side::right:api::Side::left;q.mode=p.side?api::Mode::right_main_fingers:api::Mode::left_part;
 q.kind=p.side?api::object::Kind::none:part==Part::support?api::object::Kind::support:api::object::Kind::loading;
 q.grip=grip;q.proximity=proximity;q.reference=reference;q.closed=p.closed;q.trigger_closed=p.trigger_closed;
 q.has_trigger_endpoint=p.has_trigger_endpoint;
 if(!api::structural(q))return false;out=q;return true;
}
// Output is UE right-grip-local METRES for the common sole two-hand consumer.
// No controller/world observations or cosmetic/C0 poses enter this author.
inline bool support_grip(gears_weapon::WeaponMeshFollow const& fixed,double bow_scale,double avatar_scale,
 double units,gears_hand_wrist::Calibration const& wrist,vr::Pose& out) noexcept {
 if(!fixed.bound()||!wrist.known||!std::isfinite(bow_scale)||bow_scale<=.1||bow_scale>=10||
    !std::isfinite(avatar_scale)||std::abs(bow_scale-avatar_scale)>1e-5*bow_scale||
    !std::isfinite(units)||units<10||units>500||!am::pose_valid({{},wrist.grip_to_wrist_rotation})||
    !vr::finite(wrist.grip_to_wrist_metres))return false;
 vr::Viewpoint mesh{};if(!fixed.solve({{},{}},mesh))return false;
 vr::Quaternion mesh_q{};if(!vr::rig_tracking::detail::quaternion(vr::ue_matrix(mesh.rotation),mesh_q))return false;
 const auto root=vr::normalized(vr::multiply(mesh_q,vr::Quaternion{0,0,.7071067811865475244,.7071067811865475244}));
 const auto& artist_wrist=artist::support_wrist_in_root_raw;
 const auto rotation=vr::normalized(vr::multiply(vr::multiply(root,artist_wrist.orientation),vr::inverse(wrist.grip_to_wrist_rotation)));
 const auto position=am::sub(am::add(am::mul(mesh.position,1/units),am::rotate(root,am::mul(artist_wrist.position,bow_scale/units))),
    am::rotate(rotation,wrist.grip_to_wrist_metres));
 const vr::Pose candidate{position,rotation};if(!gears_weapon_two_hand::geometry(candidate))return false;
 out=candidate;return true;
}
}

