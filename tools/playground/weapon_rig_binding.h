#pragma once
#include "../vr-common/rig_tracking_frame.h"
#include "../vr-common/controller_continuity_frame.h"

// Pure data agreement only. The caller must authenticate the hand registration
// and freshly validate live objects on the GT. No read, cache, retimestamp,
// permission, scope, lease or skeletal/camera writer is provided here.
namespace gears_equipment_rig {
enum class Status : std::uint32_t {
    matched, invalid_source, source_mismatch, inactive_hand, invalid_rig,
    missing_continuity, native_mismatch, objects_mismatch, reference_mismatch,
    reference_transition, stereo_mismatch, controllers_mismatch,
    missing_right_grip, inconsistent_world_pose
};
namespace detail {
// Recomputed convenience transforms admit 1e-5 game units per position
// component and 1e-7 per normalized quaternion component (either sign).
// Raw sample/reference comparisons below remain exact field comparisons.
inline bool near_pose(gears_vr::Pose const& a,gears_vr::Pose const& b) noexcept {
    if(!gears_vr::stereo_pose_valid(a)||!gears_vr::stereo_pose_valid(b))return false;
    if(std::abs(a.position.x-b.position.x)>1e-5||std::abs(a.position.y-b.position.y)>1e-5||
       std::abs(a.position.z-b.position.z)>1e-5)return false;
    const auto& x=a.orientation;const auto& y=b.orientation;
    const double nx=std::sqrt(x.x*x.x+x.y*x.y+x.z*x.z+x.w*x.w);
    const double ny=std::sqrt(y.x*y.x+y.y*y.y+y.z*y.z+y.w*y.w);
    const double sign=(x.x*y.x+x.y*y.y+x.z*y.z+x.w*y.w)<0?-1.:1.;
    return std::abs(x.x/nx-sign*y.x/ny)<=1e-7&&std::abs(x.y/nx-sign*y.y/ny)<=1e-7&&
        std::abs(x.z/nx-sign*y.z/ny)<=1e-7&&std::abs(x.w/nx-sign*y.w/ny)<=1e-7;
}
} // namespace detail
inline Status match(gears_weapon_hand::Frame const& hand,gears_rig_tracking::Frame const& rig,
                    std::int64_t now,std::uint32_t expectedSource) noexcept {
    namespace wh=gears_weapon_hand;namespace rt=gears_rig_tracking;
    if(expectedSource>1)return Status::invalid_source;
    const auto source=static_cast<gears_vr::StereoSource>(expectedSource);
    if(hand.stereo.source!=source||hand.controllers.source!=source||rig.stereo.source!=source||
       rig.controllers.source!=source)return Status::source_mismatch;
    if(!wh::frame_active(hand,now)||now-hand.now_ns>std::int64_t(rt::maximum_age_ns))return Status::inactive_hand;
    constexpr auto required=rt::controllers_matched|rt::producer_continuity_available;
    if((rig.flags&required)!=required)return Status::missing_continuity;
    if(!rt::frame_valid(rig,now,rt::maximum_age_ns,expectedSource==1))return Status::invalid_rig;
    const auto& k=hand.key;const auto& n=rig.native;
    if(k.game_pid!=n.game_pid||k.thread_id!=n.game_thread||k.game_start!=n.game_start||
       k.native_started_qpc!=n.native_started_qpc||k.registration_cookie!=n.registration_cookie)return Status::native_mismatch;
    if(k.world_generation!=rig.world_generation||!wh::same(hand.world,rig.world)||
       !wh::same(hand.controller,rig.controller)||!wh::same(hand.pawn,rig.pawn)||
       !wh::same(hand.mesh,rig.mesh)||!wh::same(hand.weapon,rig.weapon))return Status::objects_mismatch;
    if(hand.recentered||hand.heading_deferred)return Status::reference_transition;
    if(hand.reference_generation!=rig.reference_generation||
       !gears_vr::stereo_same_pose(hand.reference,rig.reference))return Status::reference_mismatch;
    if(!gears_vr::stereo_same(hand.stereo,rig.stereo))return Status::stereo_mismatch;
    if(!gears_hand_continuity::same_input(hand.controllers,rig.controllers))return Status::controllers_mismatch;
    if(!(rig.world_pose_mask&rt::right_grip_pose))return Status::missing_right_grip;
    for(unsigned i=0;i<2;++i)if(rig.world_pose_mask&(rt::left_grip_pose<<i)){
        gears_vr::Pose recomputed{};
        if(!gears_vr::rig_tracking::pose(rig.base_view,rig.reference,rig.controllers.hands[i].grip,
            rig.units_per_metre,recomputed)||!detail::near_pose(recomputed,rig.world_grip[i]))return Status::inconsistent_world_pose;
    }
    return Status::matched;
}
} // namespace gears_equipment_rig
