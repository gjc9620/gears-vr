#pragma once
#include "../vr-common/native_rig_tracking_frame.h"
#include "../vr-common/controller_continuity_frame.h"
#include <cstring>
#include <limits>
namespace gears_body_tick_input {
namespace core=gears_native_rig_tracking;
namespace vr=gears_vr;
namespace rig=gears_rig_tracking;
struct Input {
    core::CoreSnapshot parent_scene{};
    std::uint64_t tick{},thread{},parent_cookie{};
    std::int64_t begin_ns{},end_ns{};
    vr::StereoSample stereo{};
    vr::ControllerInputFrame controllers{};
    gears_hand_continuity::TrackedInput continuity{};
    rig::NativeReference native_reference{};
    std::uint64_t reference_revision{},publisher_pid{},publisher_start{};
};
enum class Verdict {okay,source_age_only,hard};
inline constexpr std::uint64_t tracking_age_ns=40000000,parent_age_ns=50000000;
inline bool scene_time(std::int64_t ticks,std::int64_t frequency,std::int64_t& out) noexcept {
    if(ticks<=0||frequency<=0)return false;
    const auto t=std::uint64_t(ticks),f=std::uint64_t(frequency),s=t/f,r=t%f;
    constexpr auto lim=std::uint64_t((std::numeric_limits<std::int64_t>::max)());
    if(s>lim/1000000000||r>lim/1000000000)return false;
    const auto n=s*1000000000+r*1000000000/f;
    if(!n||n>lim)return false;
    out=std::int64_t(n);return true;
}
inline bool hard_shape(Input const& input,core::CoreSnapshot const& original,std::int64_t now,bool allow_sim,std::int64_t& scene_ns) noexcept {
    const auto& s=input.stereo;const auto& c=input.controllers;const auto& p=original;
    if(std::memcmp(&input.parent_scene,&original,sizeof original)||
       !core::core_snapshot_non_age_valid(p,now,parent_age_ns,allow_sim)||
       !scene_time(p.scene_qpc,p.qpc_frequency,scene_ns)||scene_ns>now||
       !vr::stereo_geometry_valid(p.stereo)||!vr::stereo_render_active(p.stereo)||p.stereo.focused!=1||
       !(p.flags&rig::controllers_matched)||!(p.flags&rig::producer_continuity_available)||
       p.controllers.sync_ns>now||!input.tick||input.thread!=p.native.game_thread||
       input.begin_ns<=0||input.end_ns<input.begin_ns||input.end_ns>now||
       !vr::stereo_tracking_valid(s,allow_sim)||!vr::stereo_geometry_valid(s)||
       !vr::stereo_render_active(s)||s.focused!=1||s.predicted_time<=0||
       s.received_ns<=0||s.received_ns>input.end_ns||
       !vr::controller_frame_matches(c,s,c.received_ns,allow_sim)||c.received_ns>input.end_ns||c.sync_ns>input.end_ns||
       !gears_hand_continuity::tracked_input_shape_valid(input.continuity)||
       !gears_hand_continuity::same_input(input.continuity.input,c)||
       s.source!=p.stereo.source||s.session!=p.stereo.session||s.space_epoch!=p.stereo.space_epoch||
       c.profile_generation!=p.controllers.profile_generation||c.input_epoch!=p.controllers.input_epoch||
       input.continuity.stream_epoch!=p.producer.stream||
       input.continuity.tracking_epoch[0]!=p.producer.tracking[0]||input.continuity.tracking_epoch[1]!=p.producer.tracking[1]||
       input.publisher_pid!=p.controller_publisher_pid||input.publisher_start!=p.controller_publisher_start||
       input.reference_revision!=p.native_reference_revision||
       input.native_reference.epoch!=p.native_reference.epoch||input.native_reference.flags!=p.native_reference.flags||
       input.native_reference.reserved!=p.native_reference.reserved||
       s.sequence<p.stereo.sequence||s.received_ns<p.stereo.received_ns||s.predicted_time<p.stereo.predicted_time||
       c.received_ns<p.controllers.received_ns||c.sync_ns<p.controllers.sync_ns)return false;
    if(s.sequence==p.stereo.sequence&&(!vr::stereo_same(s,p.stereo)||!gears_hand_continuity::same_input(c,p.controllers)))return false;
    for(unsigned i=0;i<2;++i)if(!vr::controller_pose_usable(c.hands[i].grip,c.hands[i].grip_flags,c.hands[i].grip_active))return false;
    return true;
}
inline Verdict classify(Input const& input,core::CoreSnapshot const& original,std::int64_t now,bool allow_sim) noexcept {
    std::int64_t scene_ns{};
    if(!hard_shape(input,original,now,allow_sim,scene_ns))return Verdict::hard;
    const auto& s=input.stereo;const auto& c=input.controllers;const auto& p=original;
    const auto old=[&](std::int64_t stamp,std::uint64_t age){return std::uint64_t(now-stamp)>age;};
    if(old(input.begin_ns,tracking_age_ns)||old(input.end_ns,tracking_age_ns)||old(s.received_ns,tracking_age_ns)||
       old(c.received_ns,tracking_age_ns)||old(c.sync_ns,tracking_age_ns)||
       std::uint64_t(now)-p.published_ns>parent_age_ns||old(scene_ns,parent_age_ns)||
       old(p.stereo.received_ns,parent_age_ns)||old(p.controllers.received_ns,parent_age_ns)||old(p.controllers.sync_ns,parent_age_ns))return Verdict::source_age_only;
    return Verdict::okay;
}
static_assert(std::is_standard_layout_v<Input>&&std::is_trivially_copyable_v<Input>);
} // namespace gears_body_tick_input
