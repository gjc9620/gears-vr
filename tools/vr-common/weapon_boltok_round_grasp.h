#pragma once
#include "hand_authored_grasp.h"
#include "weapon_reload_geometry.h"

// Own offline-authored round draft, not live fitting acceptance. Original
// Pistol AR_Reload key20 (1a534915...), bounded index-only adaptation, item
// axis12deg around the same two-pad midpoint. Fixed Marcus115 lengths.
// This module formats cosmetic data and one explicit raw item mount only;
// it cannot reserve ammo, grant a lease, advance insertion or refresh input.
namespace gears_boltok_round_grasp {
namespace api=gears_hand_authored_grasp;namespace vr=gears_vr;namespace weapon=gears_weapon;
inline constexpr std::uint64_t profile=0x424f4c544f4b0004ULL;
inline constexpr api::Joints reference_left{{
    {0.073695987462997437,-0.11100184172391891,0.0043443744070827961,-0.99107450246810913},
    {1.5150532566041623e-17,-9.2641943519496922e-19,0.06103365495800972,-0.99813568592071533},
    {4.843875699392944e-17,-3.5987394175987152e-18,0.074090428650379181,-0.99725151062011719},
    {0.0047851642593741417,-0.021875778213143349,-0.0063320491462945938,-0.9997292160987854},
    {-1.3832865283107927e-17,8.4105712981142125e-17,0.052323192358016968,-0.99863022565841675},
    {-2.581321946614364e-18,-2.7689674136226814e-17,0.030531059950590134,-0.99953383207321167},
    {-0.094816975295543671,0.063338451087474823,0.027422096580266953,-0.99309921264648438},
    {-2.8278893084521812e-17,-1.2777680606510783e-17,0.039250236004590988,-0.99922943115234375},
    {1.396636065344541e-17,-4.2214077300680098e-17,0.039250236004590988,-0.99922943115234375},
    {-0.11609295755624771,0.17855842411518097,0.042987223714590073,-0.97611033916473389},
    {-6.0678354957613253e-18,2.1184170450726051e-19,0.034890979528427124,-0.99939113855361938},
    {-3.3827107781547738e-17,-6.9388939039072284e-18,-1.3877787807814457e-17,-1},
    {0.73438835144042969,-0.34662380814552307,-0.15250289440155029,-0.56326597929000854},
    {-6.8341620850100519e-18,6.2947504938109546e-17,-0.10793496668338776,-0.99415796995162964},
    {2.2402838586600293e-18,-2.3102453473036924e-18,-0.19936689734458923,-0.97992491722106934},
}};
inline constexpr api::Joints pinch{{
    {-0.097399695629802566,-0.10163691948364008,0.19721733983668499,-0.97020026631455814},
    {0,0,0.68234575385555341,-0.73102959734561812},
    {0,0,0.031731046990503525,-0.999496443543891},
    {-0.1881771294289987,0.030335398419141211,0.23520615253151037,-0.95307250373407459},
    {0,0,0.52174443800164805,-0.85310183531296224},
    {0,0,0.21671193578905609,-0.97623559497006673},
    {-0.26755577257606744,0.07312234870448929,0.33622241889706111,-0.90001195309405313},
    {0,0,0.44233527634510328,-0.89684976629349755},
    {0,0,0.16379284035767694,-0.98649475692857314},
    {-0.33564256721701718,0.07345805230872525,0.48634296700949131,-0.80337942471996615},
    {0,0,0.2478408154545732,-0.96880076909280588},
    {0,0,0.12054811243018891,-0.99270748591391145},
    {0.75173192541276279,-0.35734122745445113,-0.09060945463423567,-0.54680552869137411},
    {0.01391644032105472,0.031128879665517136,0.09289834284493545,-0.99509156535276955},
    {0,0,0.050660725730155343,-0.9987159210048141},
}};
inline constexpr vr::Pose item_in_grip_raw{{6.4923668274081194,4.4354970989596634,2.7928208249712059},{0.71477345397360381,0.23351499541699872,-0.20471747595136816,0.62662621350382763}};
// Supported authoring scale only. Do not stretch this fixed hand to a
// differently scaled weapon. The caller supplies current authenticated scale.
inline bool calibrated(double scale,double units) noexcept {
    return std::isfinite(scale)&&std::abs(scale-1.)<=1e-4&&
        std::isfinite(units)&&units>=10&&units<=500;
}
inline bool item_in_grip_metres(double scale,double units,vr::Pose& out) noexcept {
    if(!calibrated(scale,units))return false;
    out={weapon::scaled(item_in_grip_raw.position,scale/units),item_in_grip_raw.orientation};return true;
}
inline bool item_world(vr::Pose const& physical_grip_raw,double scale,double units,vr::Pose& out) noexcept {
    if(!calibrated(scale,units)||!vr::stereo_pose_valid(physical_grip_raw))return false;
    const vr::Pose candidate{weapon::sum(physical_grip_raw.position,weapon::rotate(physical_grip_raw.orientation,
        weapon::scaled(item_in_grip_raw.position,scale))),
        vr::normalized(vr::multiply(physical_grip_raw.orientation,item_in_grip_raw.orientation))};
    if(!vr::stereo_pose_valid(candidate))return false;out=candidate;return true;
}
// Exact same item mount plus the existing base seating witness. This consumes
// the unmodified physical grip, independently of the cosmetic C0 skin result.
inline bool base_witness_in_grip_metres(double scale,double units,vr::Pose& out) noexcept {
    vr::Pose item{};if(!item_in_grip_metres(scale,units,item))return false;
    out={weapon::sum(item.position,weapon::rotate(item.orientation,{0,.3*scale/units,0})),item.orientation};return true;
}
// PRECONDITION: the caller independently owns the current actual held round.
// Native repeats broker/identity/current model/fresh original input checks.
inline bool held_request(gears_weapon_hand::Frame const& source,vr::Pose const& physical_grip_raw,api::Request& out) noexcept {
    api::Request q{};q.source=source;q.geometry_signature=gears_hand_authored_mount::marcus115_signature;
    q.profile_id=profile;q.side=api::Side::left;q.mode=api::Mode::left_part;q.kind=api::object::Kind::loading;
    q.grip=physical_grip_raw;q.reference=reference_left;q.closed=pinch;
    if(!api::structural(q))return false;out=q;return true;
}
}
