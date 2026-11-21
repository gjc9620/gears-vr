#pragma once
#include "hand_authored_grasp.h"
#include "hand_authored_mount_profile.h"
#include "weapon_boltok_visual_geometry.h"

// Derived authored Boltok round-pinch candidate from source capture
// 1794500066511-3. The source capture represents the whole reload cylinder;
// this profile is a derived authored validation pose, not a new capture and
// not a direct reuse of the source wrist or volume.
//
// Offline full 4-influence skin checks used a normal physical scale authored
// reference of 1. The complete in-game hand/weapon fit and physical headset
// feel remain unproven.
namespace gears_boltok_round_grasp {
namespace api=gears_hand_authored_grasp;
using P=gears_vr::Pose;
using Q=gears_vr::Quaternion;
using V=gears_vr::Vector;

inline constexpr std::uint64_t profile_id=0x424f4c544f4b0022ULL;
inline constexpr char source_capture[]="1794500066511-3";
inline constexpr bool derived_authored=true;
inline constexpr char source_semantics[]=
    "whole-cylinder source capture; derived authored single-round pinch";

inline constexpr api::Joints closed{{
    {0.22967903686186364,0.14442266522868036,0.3404771981280334,-0.9002582470321142},
    {0.054248122262465885,-0.018864309348011924,0.6529852734277368,-0.7551897190443756},
    {0.012922013479227506,-0.028420245026697467,-0.08373716763476619,-0.9959986937726235},
    {-0.006455686552256077,0.03399907843925376,0.11145996753741944,-0.9931661806631243},
    {-0.01153813147483496,-0.004493919579799631,0.3881003795936598,-0.9215339231781634},
    {-0.01736629642662083,-0.01950128670480941,0.18905611114308588,-0.981619018970564},
    {-0.10145357670618563,0.048373793082130086,0.14729782871130612,-0.9826853502384699},
    {-0.015263681288943311,0.007250891555641208,0.3429063553927283,-0.9392175871630779},
    {-0.005728638275622816,0.0016294585547322593,0.16886787548257715,-0.9856207019935844},
    {-0.12706962573919078,0.04688335699310086,0.1343857521762518,-0.9816291207292},
    {-0.05253481472428995,-0.03919201965734298,0.31191146657941493,-0.9478477281997049},
    {-0.010627244172123455,-0.01268727536884912,0.11752378366939684,-0.9929321502484698},
    {0.4541441332708237,-0.49053871537852156,-0.16574805901191847,0.7250189348315421},
    {-0.05044516374066421,-0.13886209543198863,0.22745811298808222,-0.9625151483188801},
    {-0.06626513544940205,-0.07805041155915368,0.2924257573954091,-0.9507913764286258},
}};

inline constexpr V primer_in_wrist_m{
    0.09031872809139121,-0.04960179107549576,-0.02689543574505783};
inline constexpr Q item_in_wrist_rotation{
    0.7193485857214215,0.21078312005061153,
    0.1314493326333509,0.6487134663875622};

inline bool mount(double units,P& out) noexcept {
    if(!std::isfinite(units)||units<10||units>500)return false;
    gears_hand_authored_mount::Mount left{};
    if(!gears_hand_authored_mount::marcus115(
           gears_hand_authored_mount::marcus115_signature,
           gears_hand_authored_mount::Side::left,1,100,left))return false;
    const auto& c=left.calibration;
    P candidate{};
    const auto marker=gears_vr::transform(
        gears_vr::rotation_matrix(c.grip_to_wrist_rotation),
        primer_in_wrist_m);
    candidate.position=V{c.grip_to_wrist_metres.x+marker.x,c.grip_to_wrist_metres.y+marker.y,c.grip_to_wrist_metres.z+marker.z};
    candidate.orientation=gears_vr::normalized(
        gears_vr::multiply(c.grip_to_wrist_rotation,
            item_in_wrist_rotation));
    if(!gears_vr::stereo_pose_valid(candidate))return false;
    out=candidate;
    return true;
}

// Primer plus the original +3 mm seating offset. This is a landing landmark,
// not the 25 mm contact location, the 68 mm nose, the original insertion
// tolerance, or a six-hole chamber volume.
inline bool seating_mount(double units,double physical_scale,P& out) noexcept {
    P round_base{};
    if(!mount(units,round_base)||!std::isfinite(physical_scale)||
       physical_scale<=0||physical_scale>=100)return false;
    const auto marker=gears_vr::transform(
        gears_vr::rotation_matrix(round_base.orientation),
        V{0,.003*physical_scale,0});
    P candidate{V{round_base.position.x+marker.x,round_base.position.y+marker.y,round_base.position.z+marker.z},
        round_base.orientation};
    if(!gears_vr::stereo_pose_valid(candidate))return false;
    out=candidate;
    return true;
}

inline bool world(gears_boltok_round_grasp::P const& physical_grip,
    double units,gears_boltok_round_grasp::P& out) noexcept {
    namespace rp=gears_boltok_round_grasp;
    if(!gears_vr::stereo_pose_valid(physical_grip))return false;
    rp::P local{};
    if(!rp::mount(units,local))return false;
    rp::P candidate{};
    candidate.position=gears_weapon::sum(physical_grip.position,
        gears_vr::transform(
            gears_vr::rotation_matrix(physical_grip.orientation),
            gears_weapon::scaled(local.position,units)));
    candidate.orientation=gears_vr::normalized(
        gears_vr::multiply(physical_grip.orientation,local.orientation));
    if(!gears_vr::stereo_pose_valid(candidate))return false;
    out=candidate;
    return true;
}

inline bool ports(gears_weapon::BoltokVisualGeometry const& geometry,
    gears_weapon::BoltokChamberGeometry& out) noexcept {
    namespace rp=gears_boltok_round_grasp;
    gears_weapon::BoltokChamberGeometry temp{};
    if(!geometry.ports(temp))return false;
    rp::P seat{};
    if(!rp::seating_mount(geometry.units,geometry.physical_scale(),seat))
        return false;
    for(unsigned i=0;i<6;++i)temp.ports[i].item_in_left_grip=seat;
    if(!temp.valid())return false;
    out=temp;
    return true;
}

} // namespace gears_boltok_round_grasp
