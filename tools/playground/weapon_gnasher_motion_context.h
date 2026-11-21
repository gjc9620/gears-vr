#pragma once
#include "weapon_gnasher_raw_input.h"
#include "weapon_gnasher_producer_motion.h"
namespace gears_gnasher_motion_context {
namespace wh=gears_weapon_hand;namespace rt=gears_rig_tracking;namespace c=gears_hand_continuity;
// Saved scalar/object facts for equality only. No saved Frame, world pose,
// grant or snapshot can stand in for the next current ownership check.
struct Context {
    wh::FrameKey key{};wh::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{},cover_link{};
    rt::NativeKey native{};rt::ObservedContinuity observed{};rt::NativeReference native_reference{};
    gears_vr::Pose reference{};c::TrackedInput raw{};c::Publisher publisher{};
    std::uint64_t reference_generation{},binding_generation{},hold_generation{},support_token{},calibration{},curve{},shot{};
    double units{};std::uint32_t view_policy{},cover_type{};std::int32_t cover_slot{-1};
};
inline bool capture(wh::Frame const& f,rt::Frame const& rig,std::int64_t current_ns,
    std::uint64_t hold,std::uint64_t calibration,std::uint64_t curve,std::uint64_t shot,
    gears_weapon::GnasherMechanicalSample const& sample,Context& out) noexcept {
    out={};Context x{};
    const gears_weapon::GnasherIdentity identity{f.world.address,f.pawn.address,f.weapon.address,f.key.world_generation,
        f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch,f.controllers.profile_generation};
    if(!hold||!calibration||!curve||!shot||!sample.reference_generation||!sample.support_grasp_token||
       !(sample.identity==identity)||sample.reference_generation!=f.reference_generation||
       sample.support_grasp_token!=f.press_token||!f.left_grip_down||!f.left_grip_known||!f.left_tracked||
       !sample.original_ready||sample.original_animation_active||!sample.left_front_held||!sample.right_handle_held||
       !sample.left_tracked||!sample.right_tracked||sample.left_round_serial||
       !gears_equipment_rig::continuity(f,rig,current_ns,unsigned(f.stereo.source),x.raw,x.publisher)||
       sample.sequence!=x.raw.input.sequence||sample.monotonic_ns!=x.raw.input.sync_ns)return false;
    x.key=f.key;x.key.frame=0;x.world=f.world;x.controller=f.controller;x.pawn=f.pawn;x.mesh=f.mesh;x.weapon=f.weapon;
    x.cover_link=rig.cover_link;x.native=rig.native;x.observed=rig.observed;x.native_reference=rig.native_reference;
    x.reference=f.reference;x.reference_generation=sample.reference_generation;x.binding_generation=rig.binding_generation;x.hold_generation=hold;
    x.support_token=sample.support_grasp_token;x.calibration=calibration;x.curve=curve;x.shot=shot;
    x.units=rig.units_per_metre;x.view_policy=rig.view_policy;x.cover_type=rig.cover_type;x.cover_slot=rig.cover_slot;
    out=x;return true;
}
inline bool same(Context const& a,Context const& b) noexcept {
    return wh::same(a.key,b.key)&&wh::same(a.world,b.world)&&wh::same(a.controller,b.controller)&&
        wh::same(a.pawn,b.pawn)&&wh::same(a.mesh,b.mesh)&&wh::same(a.weapon,b.weapon)&&wh::same(a.cover_link,b.cover_link)&&
        rt::same(a.native,b.native)&&a.observed.stream==b.observed.stream&&a.observed.reference==b.observed.reference&&
        a.observed.head_tracking==b.observed.head_tracking&&a.observed.hand_tracking[0]==b.observed.hand_tracking[0]&&
        a.observed.hand_tracking[1]==b.observed.hand_tracking[1]&&a.native_reference.epoch==b.native_reference.epoch&&
        a.native_reference.flags==b.native_reference.flags&&a.native_reference.reserved==b.native_reference.reserved&&
        gears_vr::stereo_same_pose(a.reference,b.reference)&&a.reference_generation==b.reference_generation&&
        a.binding_generation==b.binding_generation&&
        a.hold_generation==b.hold_generation&&a.support_token==b.support_token&&a.calibration==b.calibration&&
        a.curve==b.curve&&a.shot==b.shot&&a.units==b.units&&a.view_policy==b.view_policy&&
        a.cover_type==b.cover_type&&a.cover_slot==b.cover_slot&&a.publisher.pid==b.publisher.pid&&
        a.publisher.start==b.publisher.start&&a.raw.stream_epoch==b.raw.stream_epoch&&
        a.raw.tracking_epoch[0]==b.raw.tracking_epoch[0]&&a.raw.tracking_epoch[1]==b.raw.tracking_epoch[1];
}
}
