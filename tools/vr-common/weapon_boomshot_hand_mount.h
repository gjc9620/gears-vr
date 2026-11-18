#pragma once
#include "hand_authored_mount_profile.h"
#include "rig_tracking_math.h"
#include "weapon_boomshot_barrel_path.h"

// Per-part authoring from this original gun's surfaces. These are candidate
// palm frames, not measured skin contact or a physical/action certificate.
namespace gears_boomshot_hand_mount {
namespace vr=gears_vr;using V=vr::Vector;using P=vr::Pose;using Q=vr::Quaternion;
// Original rigid Root OBJ0d35263a, triangle147, vertices339/336/327.
inline constexpr V support_surface_raw{-.2418916626666667,48.181250266666666,-3.4737649766666663};
inline constexpr V support_normal{-8.486632756930768e-14,-3.021751036072365e-8,-.9999999999999994};
// Original rigid Barrel OBJc6524094, triangle1454, vertices188/337/189.
inline constexpr V mechanism_surface_closed_raw{-8.47420279,63.9282252,5.558382036666667};
inline constexpr V mechanism_normal{-.8538100979409472,.000010379782188556586,-.5205845911533787};
// Marcus palm-pad authoring, in its own original model units. It follows the
// current C0 hand model scale, independently of gun or original M0 scale.
inline constexpr V palm_pad_raw{8.,-1.6,0.};
inline V sub(V a,V b)noexcept{return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V mul(V a,double s)noexcept{return {a.x*s,a.y*s,a.z*s};}
inline double dot(V a,V b)noexcept{return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V cross(V a,V b)noexcept{return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline bool unit(V a,V& out)noexcept{
    const double n=std::sqrt(dot(a,a));if(!vr::finite(a)||!std::isfinite(n)||n<1e-6)return false;out=mul(a,1/n);return true;
}
inline V rotate(Q q,V v)noexcept{return vr::transform(vr::rotation_matrix(q),v);}
inline V point(P const& part,V raw,double scale)noexcept{
    const auto d=rotate(part.orientation,mul(raw,scale));return {part.position.x+d.x,part.position.y+d.y,part.position.z+d.z};
}
inline bool palm_rotation(V outward,V finger_direction,Q& out)noexcept{
    V y{},x{};if(!unit(outward,y)||!unit(sub(finger_direction,mul(y,dot(finger_direction,y))),x))return false;
    const auto z=cross(x,y);vr::Matrix matrix{{{x.x,y.x,z.x},{x.y,y.y,z.y},{x.z,y.z,z.z}}};
    // Palm normal is local -Y. It faces INTO the original surface; +X is
    // authored finger direction projected into that surface's tangent plane.
    return vr::rig_tracking::detail::quaternion(matrix,out);
}
inline bool grip(P const& part,double part_scale,double model_scale,double units,V surface,V normal,V finger_direction,P& out)noexcept{
    if(!vr::stereo_pose_valid(part)||!std::isfinite(part_scale)||part_scale<.01||part_scale>10||
       !std::isfinite(model_scale)||model_scale<.01||model_scale>10||!vr::finite(surface))return false;
    gears_hand_authored_mount::Mount hand{};Q local{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
       gears_hand_authored_mount::Side::left,model_scale,units,hand)||!palm_rotation(normal,finger_direction,local))return false;
    const auto wrist_rotation=vr::normalized(vr::multiply(part.orientation,local));
    const auto surface_world=point(part,surface,part_scale),pad=rotate(wrist_rotation,mul(palm_pad_raw,model_scale));
    const P wrist{sub(surface_world,pad),wrist_rotation};
    const auto grip_rotation=vr::normalized(vr::multiply(wrist.orientation,vr::inverse(hand.calibration.grip_to_wrist_rotation)));
    const auto marker=rotate(grip_rotation,mul(hand.calibration.grip_to_wrist_metres,units));
    const P result{sub(wrist.position,marker),grip_rotation};if(!vr::stereo_pose_valid(result))return false;out=result;return true;
}
inline bool support(P const& root,double scale,double hand_scale,double units,P& out)noexcept{
    return grip(root,scale,hand_scale,units,support_surface_raw,support_normal,{0,1,0},out);
}
inline bool mechanism(P const& actual_barrel,double scale,double hand_scale,double units,bool closing,P& out)noexcept{
    const auto local=sub(mechanism_surface_closed_raw,gears_weapon::boomshot_barrel::closed_raw);
    return grip(actual_barrel,scale,hand_scale,units,local,mechanism_normal,closing?V{0,1,.15}:V{0,1,0},out);
}
inline bool relative(P const& right,P const& world,double units,P& out)noexcept{
    if(!vr::stereo_pose_valid(right)||!vr::stereo_pose_valid(world)||!std::isfinite(units)||units<10||units>500)return false;
    const P value{mul(rotate(vr::inverse(right.orientation),sub(world.position,right.position)),1/units),
        vr::normalized(vr::multiply(vr::inverse(right.orientation),world.orientation))};
    if(!vr::stereo_pose_valid(value))return false;out=value;return true;
}
}
