#pragma once
#include "weapon_boltok_owner_profiles.h"
// Private derived Marcus support-avatar preview, task324. Not a new capture.
// Original capture1794485865263-1 and physical Volume/Snap remain untouched.
// Both publications retain original profile11 so the existing Boltok support
// owner/relative-pose consumer and every current/lease/distance guard still apply.
// Offset is an immutable asset descriptor, never a cached address or permission.
namespace gears_boltok_support_avatar {
namespace vr=gears_vr;namespace api=gears_hand_authored_grasp;
inline constexpr auto source_profile_id=gears_boltok_owner_profiles::support.profile_id;
inline constexpr auto profile_id=source_profile_id;
inline constexpr char capture_id[]="1794485865263-1";
inline constexpr bool derived_avatar=true;
inline constexpr vr::Pose grip_delta_metres{{0.00011105593729247853,-0.045662837498851021,-0.008609523458646056},{0.0056910424060437259,-0.0060260921045692997,-0.041509902235413662,0.99910371146677579}};
inline constexpr api::Joints closed{{
    {0.19927813615381607,-0.28644079613477835,-0.060382327600198661,-0.9351972354933038},
    {0.070698159048168646,-0.013873241600056516,0.83021473760674525,-0.55276829950284723},
    {0.014408200791502124,-0.030304648903173751,-0.048810593593607676,-0.99824423762817027},
    {0.19191060293541298,-0.47715684854710172,-0.12175827211653908,-0.84892083584793665},
    {-0.010668551704170682,-0.015600146450378065,0.93058771830045572,-0.36558079268967886},
    {-0.040867312788550714,-0.017612154979820657,-0.13062276437673692,-0.99043291957052249},
    {0.16117575340204635,-0.36825189385528673,-0.073336371712578688,-0.91270734398892184},
    {-0.025191818083107033,-0.004032143077718881,0.90044905050457313,-0.43421264556583478},
    {-0.0090626060666855867,0.007844242662044594,-0.13185087916964275,-0.99119709578394721},
    {-0.051814770937610527,0.74481772183027362,0.0099019486987041612,-0.66517948117197079},
    {-0.016596609925171618,-0.053394770140184962,0.7844416820162835,-0.6176769370602504},
    {-0.035883718302870328,-0.023272061028603106,0.010682810791506986,-0.99902785120828053},
    {0.86358238807932242,0.064955579267169897,0.0046952074526447904,-0.49998418649819093},
    {-0.070219866492815641,-0.099951023760003851,-0.064132045670001692,-0.99043729933663094},
    {-0.12470040465645137,-0.028518344669851273,-0.29717170932511877,-0.94621640668115459},
}};
// Pose formatting only, called after original hover/lease eligibility. Reuses
// that caller's current physical geometry; no additional engine/reflection read.
inline bool target(vr::Pose const& physical,double physical_scale,vr::Pose& out) noexcept {
    if(!vr::stereo_pose_valid(physical)||!api::unit(physical.orientation)||
       !std::isfinite(physical_scale)||physical_scale<=0)return false;
    const auto t=grip_delta_metres.position;
    const auto offset=vr::transform(vr::rotation_matrix(physical.orientation),
        vr::Vector{t.x*physical_scale,t.y*physical_scale,t.z*physical_scale});
    vr::Pose candidate{{physical.position.x+offset.x,physical.position.y+offset.y,
        physical.position.z+offset.z},vr::normalized(vr::multiply(physical.orientation,
        grip_delta_metres.orientation))};
    if(!vr::stereo_pose_valid(candidate)||!api::unit(candidate.orientation))return false;
    out=candidate;return true;
}
}
