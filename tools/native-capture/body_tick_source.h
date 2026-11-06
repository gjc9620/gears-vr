#pragma once
#include "body_tick_input.h"
// Distinct consumer data contract. cache revision is an immutable association,
// never proof of a live owner or eligibility for old TickInput/Core consumers.
namespace gears_body_tick_source {
namespace tick=gears_body_tick_input;
using Verdict=tick::Verdict;
struct Input {
    tick::Input observed{};std::uint64_t scene_cache_revision{};
    // Actual outer-Tick actor/Scene-metadata observation, beginning before the
    // first current_actor read and ending after its closing reread. Original
    // Scene clocks remain unchanged; this receipt never renews old tracking.
    std::int64_t metadata_begin_ns{},metadata_end_ns{};
};
// Identity-only association with the last successful independent acquisition.
// It never supplies an old pose/time or proves freshness. The live owner must
// require this before latching a source whose Scene omitted controllers; an
// unestablished/replaced input domain waits for a complete Scene receipt.
inline bool independent_domain_matches(Input const& current,Input const& previous) noexcept {
    auto const& a=current.observed;auto const& b=previous.observed;
    auto const& p=a.parent_scene;auto const& q=b.parent_scene;
    return tick::core::same(p.native,q.native)&&a.thread==b.thread&&a.parent_cookie==b.parent_cookie&&
        p.world_generation==q.world_generation&&p.binding_generation==q.binding_generation&&
        gears_weapon_hand::same(p.world,q.world)&&gears_weapon_hand::same(p.controller,q.controller)&&
        gears_weapon_hand::same(p.pawn,q.pawn)&&gears_weapon_hand::same(p.mesh,q.mesh)&&
        p.reference_generation==q.reference_generation&&p.units_per_metre==q.units_per_metre&&
        tick::vr::stereo_same_pose(p.reference,q.reference)&&a.reference_revision==b.reference_revision&&
        a.native_reference.epoch==b.native_reference.epoch&&a.native_reference.flags==b.native_reference.flags&&
        !a.native_reference.reserved&&!b.native_reference.reserved&&
        a.stereo.source==b.stereo.source&&a.stereo.session==b.stereo.session&&a.stereo.space_epoch==b.stereo.space_epoch&&
        a.publisher_pid&&a.publisher_start&&a.publisher_pid==b.publisher_pid&&a.publisher_start==b.publisher_start&&
        a.controllers.input_epoch==b.controllers.input_epoch&&a.controllers.profile_generation==b.controllers.profile_generation&&
        a.continuity.stream_epoch==b.continuity.stream_epoch&&gears_current_continuity::not_regressed(a.continuity,b.continuity);
}
// Diagnostics of immutable COPIES only, used after the unchanged hard-shape
// predicate rejected an acquisition. No game reads, retry, source replacement
// or permission. CSV stage1 reason bits identify the specific invariant instead
// of labelling a fresh-but-inconsistent packet as an age failure.
inline unsigned rejection_detail(Input const& input,gears_native_rig_tracking::CoreSnapshot const& p,
    std::int64_t now,bool allow_sim) noexcept {
    auto const& v=input.observed;auto const& s=v.stereo;auto const& c=v.controllers;
    unsigned bits{};std::int64_t scene_ns{};
    const auto bad=[&](unsigned bit,bool value){if(value)bits|=1u<<bit;};
    bad(0,std::memcmp(&v.parent_scene,&p,sizeof p)!=0);
    bad(1,!tick::core::core_snapshot_non_age_valid(p,now,tick::parent_age_ns,allow_sim));
    bad(2,!tick::scene_time(p.scene_qpc,p.qpc_frequency,scene_ns)||scene_ns>now);
    bad(3,!tick::vr::stereo_geometry_valid(p.stereo)||!tick::vr::stereo_render_active(p.stereo)||p.stereo.focused!=1);
    const bool parent_controllers=(p.flags&tick::rig::controllers_matched)!=0;
    bad(4,!parent_controllers&&!gears_current_continuity::absent_controllers(p));
    bad(5,p.controllers.sync_ns>now);
    bad(6,!v.tick||v.thread!=p.native.game_thread);
    bad(7,v.begin_ns<=0||v.end_ns<v.begin_ns||v.end_ns>now);
    bad(8,!tick::vr::stereo_tracking_valid(s,allow_sim)||!tick::vr::stereo_geometry_valid(s)||
        !tick::vr::stereo_render_active(s)||s.focused!=1||s.predicted_time<=0);
    bad(9,s.received_ns<=0||s.received_ns>v.end_ns);
    bad(10,!tick::vr::controller_frame_matches(c,s,c.received_ns,allow_sim)||c.received_ns>v.end_ns||c.sync_ns>v.end_ns);
    bad(11,!gears_hand_continuity::tracked_input_shape_valid(v.continuity)||!gears_hand_continuity::same_input(v.continuity.input,c));
    bad(12,s.source!=p.stereo.source||s.session!=p.stereo.session||s.space_epoch!=p.stereo.space_epoch);
    bad(13,parent_controllers&&(c.profile_generation!=p.controllers.profile_generation||c.input_epoch!=p.controllers.input_epoch));
    bad(14,!gears_current_continuity::metadata_scene(v.continuity,p.producer,p.controllers.sequence,
        (p.flags&tick::rig::producer_continuity_available)!=0,true,true));
    bad(15,!v.publisher_pid||!v.publisher_start||(parent_controllers&&
        (v.publisher_pid!=p.controller_publisher_pid||v.publisher_start!=p.controller_publisher_start)));
    bad(16,v.reference_revision!=p.native_reference_revision||v.native_reference.epoch!=p.native_reference.epoch||
        v.native_reference.flags!=p.native_reference.flags||v.native_reference.reserved!=p.native_reference.reserved);
    bad(17,s.sequence<p.stereo.sequence||s.received_ns<p.stereo.received_ns||s.predicted_time<p.stereo.predicted_time||
        c.received_ns<p.controllers.received_ns||c.sync_ns<p.controllers.sync_ns);
    bad(18,s.sequence==p.stereo.sequence&&(!tick::vr::stereo_same(s,p.stereo)||
        (parent_controllers&&!gears_hand_continuity::same_input(c,p.controllers))));
    bad(19,!input.scene_cache_revision||input.metadata_begin_ns<=0||input.metadata_end_ns<input.metadata_begin_ns||
        input.metadata_end_ns>v.begin_ns);
    bad(20,std::uint64_t(input.metadata_begin_ns)<p.published_ns||std::uint64_t(scene_ns)>p.published_ns);
    return bits;
}
inline Verdict classify(Input const& input,gears_native_rig_tracking::CoreSnapshot const& original,
    std::int64_t now,bool allow_sim) noexcept {
    std::int64_t scene_ns{};
    // Current-eye composition solves each tracked hand independently. A valid
    // frame with one hand unavailable must not discard the other hand or the
    // head/body solve, including the first newer loss epoch. The authenticated
    // stream stays exact; per-hand epochs may only advance with a newer cohort.
    // Shape, cohort, publisher, reference and age remain authenticated;
    // per-hand target preparation refuses the unavailable hand. The legacy
    // TickInput contract continues to require both hands.
    if(!input.scene_cache_revision||!tick::hard_shape(input.observed,original,now,allow_sim,scene_ns,false,true)||
       input.metadata_begin_ns<=0||input.metadata_end_ns<input.metadata_begin_ns||
       input.metadata_end_ns>input.observed.begin_ns||
       std::uint64_t(input.metadata_begin_ns)<original.published_ns||std::uint64_t(scene_ns)>original.published_ns)return Verdict::hard;
    const auto& t=input.observed;
    const auto old=[&](std::int64_t stamp,std::uint64_t age){return std::uint64_t(now-stamp)>age;};
    if(old(t.begin_ns,tick::tracking_age_ns)||old(t.end_ns,tick::tracking_age_ns)||
       old(t.stereo.received_ns,tick::tracking_age_ns)||old(t.controllers.received_ns,tick::tracking_age_ns)||
       old(t.controllers.sync_ns,tick::tracking_age_ns)||old(input.metadata_begin_ns,tick::parent_age_ns))return Verdict::source_age_only;
    return Verdict::okay;
}
static_assert(std::is_standard_layout_v<Input>&&std::is_trivially_copyable_v<Input>);
} // namespace gears_body_tick_source
