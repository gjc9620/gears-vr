#pragma once
#include "hand_authored_grasp.h"
#include "weapon_reload_geometry.h"

// Original Object03 mechanism, offline fixed-length draft only. Original
// Pistol fingers plus bounded index/thumb fitting to true-facing skin pads.
// Seventeen nominal opening samples have no proper surface cuts. This does
// not certify sealed-volume contact, live model calibration, reach or pixels.
// No production caller includes this header yet. Cosmetic target and joints
// only: raw controller motion, lease, ammo and original mechanism stay separate.
namespace gears_boltok_mechanism_grasp {
namespace api=gears_hand_authored_grasp;namespace vr=gears_vr;namespace weapon=gears_weapon;
inline constexpr std::uint64_t profile=0x424f4c544f4b0002ULL;
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
    {-0.10365361567816879,-0.088597693424408749,0.14585584121388059,-0.97986348552260505},
    {0,0,0.6001059671947454,-0.7999205136369858},
    {0,0,0.092675114427036479,-0.99569640110122704},
    {-0.031980089843273163,0.10297515988349915,0.54450118541717529,-0.83180036630183984},
    {2.674468202457092e-09,4.9069073249086159e-09,0.56280499696731567,-0.82658970196139014},
    {5.2852584531137836e-09,2.128694553960031e-08,0.36243781447410583,-0.93200795631755917},
    {-0.053857769817113876,0.090775042772293091,0.56050604581832886,-0.82139649673048676},
    {-4.7294943072984097e-08,2.3804494375667673e-08,0.51877325773239136,-0.85491187093274057},
    {4.723412772023039e-08,1.1452711845549857e-07,0.39073118567466736,-0.92050482917867049},
    {-0.046133134514093399,0.078141480684280396,0.52710980176925659,-0.84493839998851272},
    {9.1099456867027584e-09,-1.0622445145713755e-08,0.45399057865142822,-0.89100648398075144},
    {-3.5337901493903701e-08,7.309840732716566e-09,0.33791670203208923,-0.94117602099063002},
    {0.72227203276759844,-0.41692314671416403,-0.12837296191869002,-0.53667362806825492},
    {0.0082995450957761754,0.033072526538547094,0.26428284521092016,-0.96384226057316091},
    {0,0,0.13400438641910026,-0.99098073867277581},
}};
inline constexpr vr::Pose grip_in_support_raw{{10.312302428686289,1.8016496626616103,5.4878695368005115},{-0.21166882991487548,-0.59112457472871016,0.6602466172255016,0.41211945845416159}};
// Original Object02 origin relative to parent Object03; same mechanical
// contact used by the current hinge, mapped through the fixed authored grip.
inline constexpr vr::Vector contact_in_grip_raw{5.1186713399261601,5.3288532032205573,10.321901706459288};
inline bool calibrated(double scale,double units) noexcept {
    return std::isfinite(scale)&&std::abs(scale-1.)<=1e-4&&
        std::isfinite(units)&&units>=10&&units<=500;
}
inline bool grip_in_support_metres(double scale,double units,vr::Pose& out) noexcept {
    if(!calibrated(scale,units))return false;
    out={weapon::scaled(grip_in_support_raw.position,scale/units),grip_in_support_raw.orientation};return true;
}
inline bool contact_in_grip_metres(double scale,double units,vr::Vector& out) noexcept {
    if(!calibrated(scale,units))return false;out=weapon::scaled(contact_in_grip_raw,scale/units);return true;
}
// Independently derived from ORIGINAL raw tracking, never the snapped hand.
// Merely maps a known local contact offset; no lease or motion is granted.
inline bool mechanical_contact_from_raw_grip(vr::Pose const& physical_grip_raw,
    double scale,double units,vr::Vector& out) noexcept {
    if(!calibrated(scale,units)||!vr::stereo_pose_valid(physical_grip_raw))return false;
    const auto candidate=weapon::sum(physical_grip_raw.position,
        weapon::rotate(physical_grip_raw.orientation,weapon::scaled(contact_in_grip_raw,scale)));
    if(!vr::finite(candidate))return false;out=candidate;return true;
}
// The caller supplies this exact current Object03 world pose in METRES.
// Its world rotation already carries the current original opening angle.
// Do not reapply root, the rest pivot, an inferred gun yaw or controller pose.
inline bool cosmetic_grip_world(vr::Pose const& support_world_metres,double scale,double units,vr::Pose& out) noexcept {
    vr::Pose local{};
    if(!vr::stereo_pose_valid(support_world_metres)||!grip_in_support_metres(scale,units,local))return false;
    const vr::Pose candidate{weapon::sum(support_world_metres.position,
        weapon::rotate(support_world_metres.orientation,local.position)),
        vr::normalized(vr::multiply(support_world_metres.orientation,local.orientation))};
    if(!vr::stereo_pose_valid(candidate))return false;out=candidate;return true;
}
// PRECONDITION: the caller independently owns the current original mechanism
// lease. The existing broker and Native consumer repeat that admission.
// Kind::magazine is this actual mechanism's existing lease route; this is
// never a rifle two-hand support token and grants no mechanical authority.
inline bool held_request(gears_weapon_hand::Frame const& source,vr::Pose const& support_world_metres,
    double scale,double units,api::Request& out) noexcept {
    api::Request q{};if(!cosmetic_grip_world(support_world_metres,scale,units,q.grip))return false;
    // Non-support V4 targets use UE WORLD RAW units in the existing C0
    // object-grasp consumer. Only this wire boundary converts metres to raw.
    q.grip.position=weapon::scaled(q.grip.position,units);
    q.source=source;q.geometry_signature=gears_hand_authored_mount::marcus115_signature;
    q.profile_id=profile;q.side=api::Side::left;q.mode=api::Mode::left_part;q.kind=api::object::Kind::magazine;
    q.reference=reference_left;q.closed=pinch;
    if(!api::structural(q))return false;out=q;return true;
}
}
