#pragma once
#include "cover_grab_pose.h"
#include "hand_finger_pose.h"

// Designer-authored starting poses for Marcus115, never captured hand tracking
// or a claim of accepted skin/surface fitting. Each original L2 reference has
// its own palm/wrist basis and joint curve. Native authenticates the actual
// rig/reference and current point before selecting this cosmetic data.
namespace gears_cover_grab_profiles {
namespace vr=gears_vr;namespace grab=gears_cover_grab_pose;
using Joints=std::array<vr::Quaternion,15>;
inline constexpr char revision[]="marcus115-cover-l2-author-v1";
struct Authoring {
    double yaw{},pitch{},roll{};
    double fingers[4][3]{};
    double thumb_base{},thumb_opposition{},thumb_middle{},thumb_tip{};
};
inline constexpr Authoring points[3]={
    // Side references turn the palm toward their respective outside edge.
    {-90,-35,-15,{{48,68,35},{54,78,42},{60,82,42},{66,78,36}},18,42,42,28},
    {90,-35,15,{{42,64,32},{52,76,40},{58,80,42},{64,76,36}},24,24,46,30},
    // Top reaches forward over the low cover with a flatter knuckle arch.
    {0,25,0,{{28,68,38},{30,76,42},{34,78,42},{40,72,36}},12,34,38,24}
};
inline vr::Quaternion turn(unsigned axis,double degrees) noexcept {
    constexpr double rad=.017453292519943295;
    const double half=degrees*rad*.5;const double s=std::sin(half);
    return {axis==0?s:0,axis==1?s:0,axis==2?s:0,std::cos(half)};
}
inline bool prepare(vr::CoverEdgeKind kind,grab::mount::Signature const& actual,
    Joints const& reference,grab::Profile& out) noexcept {
    if(actual!=grab::mount::marcus115_signature)return false;
    unsigned index=3;
    if(kind==vr::CoverEdgeKind::left)index=0;else if(kind==vr::CoverEdgeKind::right)index=1;
    else if(kind==vr::CoverEdgeKind::top)index=2;
    if(index==3)return false;
    for(auto q:reference)if(!vr::stereo_pose_valid({{},q}))return false;
    grab::mount::Mount marker{};
    if(!grab::mount::marcus115(actual,grab::mount::Side::left,1,100,marker))return false;
    auto const& authored=points[index];
    const auto grip=vr::multiply(turn(2,authored.yaw),vr::multiply(turn(1,authored.pitch),turn(0,authored.roll)));
    // Place the existing authored palm/controller marker at the reference,
    // using model units. Reuse its wrist-down20 calibration exactly once.
    const auto p=marker.calibration.grip_to_wrist_metres;
    grab::Profile next{};next.id=0x434f564552000001ull+index;next.kind=kind;
    next.geometry_signature=actual;
    next.wrist_in_edge.position=vr::transform(vr::rotation_matrix(grip),{p.x*100,p.y*100,p.z*100});
    next.wrist_in_edge.orientation=vr::multiply(grip,marker.calibration.grip_to_wrist_rotation);
    next.acquire_metres=.18;next.retain_metres=.25;
    next.maximum_wrist_turn=110*.017453292519943295;
    for(unsigned joint=0;joint<15;++joint){
        for(unsigned stage=0;stage<2;++stage){
            const double weight=stage?1.:.25;
            auto q=reference[joint];
            if(joint<12)q=vr::multiply(q,turn(2,-authored.fingers[joint/3][joint%3]*weight));
            else if(joint==12)q=vr::multiply(turn(2,-authored.thumb_base*weight),
                vr::multiply(turn(1,-authored.thumb_opposition*weight),q));
            else q=vr::multiply(q,turn(2,-(joint==13?authored.thumb_middle:authored.thumb_tip)*weight));
            (stage?next.held:next.approach)[joint]=vr::normalized(q);
        }
    }
    if(!grab::profile_valid(next))return false;
    out=next;return true;
}
} // namespace gears_cover_grab_profiles
