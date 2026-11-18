#pragma once
#include "weapon_gnasher_mechanics.h"
#include "weapon_support_volume.h"
// Source-derived authored contact geometry only, not an action grant or live
// fitting certificate. Full fingers/materials/stereo/headset remain separate.
namespace gears_weapon_gnasher_hold {
namespace w=gears_weapon;namespace v=gears_vr;
inline constexpr char gun_export_sha256[]="0439fa167754bf5201135ad6a52eb96701c5722542f4bd093d108b6049aa023f";
inline constexpr char hand_export_sha256[]="cc2db56c76efb9354a39affebf95f72c56b60a32624b0cbc760450621a886a4d";
inline constexpr char hand_canonical_sha256[]="3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193";
inline constexpr char contact_data_sha256[]="37fa2e03479fe9c3e17e53715c230a2d7f3e02b096d996e2fb778ff00db5a8ef";
inline constexpr std::uint64_t contact_id=14116989634314435127ull;
// Original Gnasher Main-bound triangles and original Marcus palm triangles.
inline constexpr unsigned right_gun_vertices[3]={1411,1410,1412};
inline constexpr unsigned left_gun_vertices[3]={4276,4229,4277};
inline constexpr unsigned right_hand_vertices[3]={12543,12544,12529};
inline constexpr unsigned left_hand_vertices[3]={10941,10927,10942};
inline constexpr v::Vector right_surface_mesh_raw={8.5395067532857265,1.8035105069478352,-0.69973901907602942};
inline constexpr v::Vector left_surface_mesh_raw={57.185614267985024,-2.9414685567220054,5.1350731054941816};
inline constexpr v::Pose right_grip_mesh_raw={{6.4047469391806251,4.2956558340707023,-1.0577629850326709},{-0.0013430749865537847,-0.1715523895612171,-0.088470006417695549,0.98119367698221049}};
inline constexpr v::Pose left_grip_mesh_raw={{55.394106952378024,-5.67746408150051,4.6993387559630113},{0.028091365748854236,-0.17156949407223021,0.1498395025750876,0.97330514605760365}};
inline constexpr v::Pose reference_main_mesh_raw={{17.54884147644043,9.8496897749100754e-08,9.9074153900146484},{0,-0,0.70710676908493042,-0.70710676908493042}};
struct Authored {
    w::GnasherMechanicalConfig mechanical{};w::WeaponMount support{};w::SupportBox box{};
    v::Pose right_in_component_metres{},left_in_component_metres{};
    double acquire_metres{.05},release_metres{.08};
};
inline bool authored(double component_scale,double units,unsigned capacity,Authored& out) noexcept {
    out={};if(!std::isfinite(component_scale)||component_scale<=.1||component_scale>=10||
       !std::isfinite(units)||units<10||units>500||!capacity||capacity>10000)return false;
    Authored value{};const auto factor=component_scale/units;
    const auto inverse=v::inverse(reference_main_mesh_raw.orientation);
    const auto in_main=[&](v::Pose const& p){return v::Pose{
        w::scaled(w::rotate(inverse,w::difference(p.position,reference_main_mesh_raw.position)),factor),
        v::normalized(v::multiply(inverse,p.orientation))};};
    auto& m=value.mechanical;m.calibration_id=contact_id;m.lever_calibrated=true;m.feed_calibrated=false;
    m.capacity=capacity;m.game_ticks_per_second=1000000;
    m.lever.left_front_grip_in_main=in_main(left_grip_mesh_raw);
    m.lever.right_handle_home_in_main=in_main({right_surface_mesh_raw,{}}).position;
    m.lever.pivot_in_main=w::scaled({.0000005779625098512042,20.60738182067871,-2.9246158599853516},factor);
    m.lever.hinge_axis_in_main={-1,0,0};m.right_grip_home_in_main=in_main(right_grip_mesh_raw).orientation;
    m.right_contact_in_grip=w::scaled(w::rotate(v::inverse(right_grip_mesh_raw.orientation),
        w::difference(right_surface_mesh_raw,right_grip_mesh_raw.position)),factor);
    value.right_in_component_metres={w::scaled(right_grip_mesh_raw.position,factor),right_grip_mesh_raw.orientation};
    value.left_in_component_metres={w::scaled(left_grip_mesh_raw.position,factor),left_grip_mesh_raw.orientation};
    const auto relative=w::scaled(w::rotate(v::inverse(right_grip_mesh_raw.orientation),
        w::difference(left_grip_mesh_raw.position,right_grip_mesh_raw.position)),factor);
    value.support.support_grip_in_weapon={relative.y,relative.z,-relative.x};
    const auto bore=w::rotate(v::inverse(right_grip_mesh_raw.orientation),{1,0,0});
    value.support.bore_forward_in_weapon={bore.y,bore.z,-bore.x};
    value.support.support_length_tolerance_metres=.08;value.support.maximum_support_correction_radians=.35;
    // Support route only: muzzle field supplies no Gnasher fire eligibility.
    constexpr v::Matrix conversion{{{0,1,0},{0,0,1},{-1,0,0}}};
    value.box.enabled=true;value.box.axes=v::multiply(conversion,v::rotation_matrix(v::inverse(right_grip_mesh_raw.orientation)));
    value.box.acquire_half={.07,.04,.04};value.box.retain_half={.09,.06,.06};
    if(!m.valid()||!value.support.valid()||!value.box.valid())return false;
    out=value;return true;
}
} // namespace gears_weapon_gnasher_hold
