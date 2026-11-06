#pragma once
#include "../vr-common/body_main_mount_provider.h"
#include "../vr-common/weapon_equipment_observation.h"
#include "weapon_main_mount_sender.h"
#include "../vr-common/current_continuity_for_metadata.h"
#include <cmath>
#include <cstring>

namespace gears_body_main_mount_sender {
namespace wire=gears_body_main_mount_provider;
namespace hand=gears_weapon_hand;
namespace vr=gears_vr;
namespace rig=gears_rig_tracking;
namespace policy=gears_weapon_main_mount;
using Parent=gears_weapon_main_mount_sender::Parent;
inline bool age(std::int64_t stamp,std::int64_t now,std::uint64_t maximum) noexcept {
    return stamp>0&&now>=stamp&&std::uint64_t(now-stamp)<=maximum;
}
inline bool finite_matrix(vr::StereoMatrix const& m) noexcept {
    for(const auto v:m)if(!std::isfinite(v))return false;
    return true;
}
inline wire::Status context(wire::Request const& r,Parent const& parent,std::int64_t now,
    hand::EquipmentIdentity& out) noexcept {
    out={};
    if(r.size!=sizeof(r)||r.abi_version!=wire::version||!r.query_id||!r.hand_lifecycle||!r.action_generation||
       r.maximum_tracking_age_ns!=wire::tracking_age_ns||r.maximum_scene_age_ns!=wire::scene_age_ns||
       r.maximum_equipment_age_ns!=wire::equipment_age_ns||!parent.cookie||!parent.native_started_qpc||
       !parent.game_pid||!parent.game_thread||now<=0)return wire::Status::source_mismatch;
    const auto& s=r.source;const auto& o=s.original_scene;const auto& t=s.tracking;const auto& b=s.basis;
    const auto& n=o.native;
    if(o.size!=sizeof(o)||o.abi_version!=rig::version||o.status!=rig::Status::ready||o.reserved||
       n.registration_cookie!=parent.cookie||n.native_started_qpc!=parent.native_started_qpc||
       n.game_pid!=parent.game_pid||n.game_thread!=parent.game_thread||!n.game_start||
       !o.publication_sequence||!o.scene_invocation||!o.world_generation||!o.binding_generation||
        !o.reference_generation||!s.scene_ticket||!s.scene_cache_revision||
       (!(o.flags&rig::controllers_matched)&&!gears_current_continuity::absent_controllers(o))||
       !hand::identity_valid(o.world)||!hand::identity_valid(o.controller)||!hand::identity_valid(o.pawn)||
       !hand::identity_valid(o.mesh)||!hand::identity_valid(o.weapon,true)||
       !vr::stereo_pose_valid(o.reference)||!o.native_reference.epoch||o.native_reference.reserved||
       o.native_reference.flags!=rig::reference_stable||!o.published_ns||o.published_ns>std::uint64_t(INT64_MAX)||
       s.metadata_begin_ns<=0||s.metadata_end_ns<s.metadata_begin_ns||s.metadata_end_ns>t.begin_ns||
       !age(s.metadata_begin_ns,now,wire::scene_age_ns)||
       o.published_ns>std::uint64_t(s.metadata_begin_ns)||o.scene_qpc<=0||o.qpc_frequency<=0)
        return wire::Status::source_mismatch;
    const auto scene_observed_ns=static_cast<long double>(o.scene_qpc)*1000000000.L/o.qpc_frequency;
    if(scene_observed_ns<=0||scene_observed_ns>INT64_MAX||scene_observed_ns>o.published_ns)return wire::Status::source_mismatch;
    // New tracking is a distinct real sample. Never validate/renew original
    // Rig/Hand tracking as if it supplied this query's current right Grip.
    const auto selected=std::uint64_t(t.stereo.source);
    if(selected>1||t.stereo.source!=o.stereo.source||!t.tick||t.thread!=parent.game_thread||
       t.parent_cookie!=parent.cookie||t.begin_ns>t.end_ns||
       !age(t.begin_ns,now,wire::tracking_age_ns)||!age(t.end_ns,now,wire::tracking_age_ns)||
       !age(t.stereo.received_ns,now,wire::tracking_age_ns)||
       !age(t.controllers.received_ns,now,wire::tracking_age_ns)||!age(t.controllers.sync_ns,now,wire::tracking_age_ns)||
       t.stereo.received_ns>t.end_ns||t.controllers.received_ns>t.end_ns||
       !vr::stereo_render_active(t.stereo)||!vr::stereo_tracking_valid(t.stereo,selected==1)||
       !vr::stereo_geometry_valid(t.stereo)||!vr::controller_frame_matches(t.controllers,t.stereo,now,selected==1)||
       !gears_hand_continuity::tracked_input_shape_valid(t.continuity)||
       !gears_hand_continuity::same_input(t.controllers,t.continuity.input)||
       !vr::controller_pose_usable(t.controllers.hands[1].grip,t.controllers.hands[1].grip_flags,t.controllers.hands[1].grip_active)||
       t.stereo.session!=o.stereo.session||t.stereo.space_epoch!=o.stereo.space_epoch||t.stereo.sequence<o.stereo.sequence||
       ((o.flags&rig::controllers_matched)&&(t.controllers.profile_generation!=o.controllers.profile_generation||t.controllers.input_epoch!=o.controllers.input_epoch))||
        !gears_current_continuity::metadata_scene(t.continuity,o.producer,o.controllers.sequence,
            (o.flags&rig::producer_continuity_available)!=0,true,true)||
       ((o.flags&rig::controllers_matched)&&(t.publisher_pid!=o.controller_publisher_pid||t.publisher_start!=o.controller_publisher_start))||
       !t.publisher_pid||!t.publisher_start||!t.reference_revision||(t.reference_revision&1)||
       t.native_reference.epoch!=o.native_reference.epoch||t.native_reference.flags!=rig::reference_stable||
       t.native_reference.reserved)return wire::Status::source_mismatch;
    // Native current-source authenticates a prior independent input domain
    // before admitting a Scene with no controllers; this metadata-only
    // callback does not create a writer/action permit from that absence.
    // It authenticates current continuity independently of an optional
    // earlier Scene receipt. The shared rule still rejects known stream
    // conflicts, regressed epochs and same-cohort epoch changes.
    if(!b.compose||!age(b.capture_begin_ns,now,wire::tracking_age_ns)||b.capture_begin_ns<t.end_ns||
       !finite_matrix(b.component_to_world)||!vr::stereo_rigid(b.base_view))return wire::Status::source_mismatch;
    const auto& g=b.geometry;
    if(!g.revision||!hand::identity_valid(g.mesh))return wire::Status::body_unavailable;
    if(g.reserved||g.revision!=o.binding_generation||!hand::same(g.mesh,o.mesh)||g.units_per_metre!=o.units_per_metre||
       g.basis!=wire::mount::Basis::marcus_mesh_face_forward_y||g.basis_revision!=wire::mount::supported_basis_revision)
        return wire::Status::body_mismatch;
    policy::lancer::Profile profile{};
    if(!policy::lancer::marcus115(g.geometry,g.model_scale,g.units_per_metre,profile))return wire::Status::unsupported_geometry;
    hand::EquipmentIdentity value{};
    value.key={n.game_start,n.native_started_qpc,n.registration_cookie,r.hand_lifecycle,o.scene_invocation,o.world_generation,n.game_pid,n.game_thread};
    value.world=o.world;value.controller=o.controller;value.pawn=o.pawn;value.mesh=o.mesh;value.weapon=o.weapon;
    value.reference=o.reference;value.reference_generation=o.reference_generation;
    if(!hand::equipment_identity_valid(value))return wire::Status::source_mismatch;
    out=value;return wire::Status::ready;
}
inline wire::Status eligibility(hand::EquipmentIdentity const& i,hand::GripGameState const& game,
    std::uint64_t generation) noexcept {
    if(!game.known)return wire::Status::inactive;
    if(!i.weapon.address)return wire::Status::no_weapon;
    if(!game.main_hand_held||!generation)return wire::Status::unheld;
    if(game.animation_owns_hands)return wire::Status::animation;
    if(game.busy)return wire::Status::busy;
    return game.kind==hand::WeaponKind::firearm||game.kind==hand::WeaponKind::hand_thrown?
        wire::Status::ready:wire::Status::not_firearm;
}
inline wire::Status response(wire::Request const& request,Parent const& parent,
    hand::EquipmentIdentity const& ancestry,hand::EquipmentObservation const& observation,
    std::int64_t now,wire::Response& out) noexcept {
    std::memset(&out,0,sizeof(out));out.size=sizeof(out);out.abi_version=wire::version;
    out.mount.size=sizeof(out.mount);out.mount.abi_version=wire::mount::version;
    hand::EquipmentIdentity current{};const auto status=context(request,parent,now,current);
    if(status!=wire::Status::ready)return status;
    if(!hand::same(ancestry.key,current.key)||!hand::same(ancestry.world,current.world)||
       !hand::same(ancestry.controller,current.controller)||!hand::same(ancestry.pawn,current.pawn)||
       !hand::same(ancestry.mesh,current.mesh)||!hand::same(ancestry.weapon,current.weapon)||
       ancestry.reference_generation!=current.reference_generation||!vr::stereo_same_pose(ancestry.reference,current.reference)||
       observation.end_ns<observation.begin_ns||observation.end_ns>now||
       !age(observation.begin_ns,now,wire::equipment_age_ns)||
       !hand::identity_valid(observation.component)||!hand::identity_valid(observation.asset))return wire::Status::source_mismatch;
    const auto allowed=eligibility(current,observation.game,observation.hold_generation);
    if(allowed!=wire::Status::ready)return allowed;
    const auto& g=request.source.basis.geometry;policy::lancer::Profile profile{};
    if(!policy::lancer::marcus115(g.geometry,g.model_scale,g.units_per_metre,profile))return wire::Status::unsupported_geometry;
    // The live observation reader admits only Frag in the hand-thrown branch;
    // Smoke/other thrown equipment do not inherit this visual profile.
    out.hold_kind=observation.game.kind==hand::WeaponKind::hand_thrown?wire::HoldKind::frag:wire::HoldKind::firearm;
    out.query_id=request.query_id;out.hand_lifecycle=request.hand_lifecycle;out.action_generation=request.action_generation;
    out.source=wire::source_key(request.source);out.equipment_begin_ns=observation.begin_ns;out.equipment_end_ns=observation.end_ns;
    auto& m=out.mount;m.query_id=request.query_id;m.key=current.key;m.world=current.world;m.controller=current.controller;
    m.pawn=current.pawn;m.mesh=current.mesh;m.weapon=current.weapon;m.component=observation.component;m.asset=observation.asset;
    m.hold_generation=observation.hold_generation;m.provider_revision=wire::mount::provider_revision;
    m.body_revision=g.revision;m.basis_revision=g.basis_revision;m.basis=g.basis;m.geometry=g.geometry;
    m.model_scale=g.model_scale;m.units_per_metre=g.units_per_metre;m.calibration_known=1;
    m.grip_to_wrist_metres=profile.mount.calibration.grip_to_wrist_metres;
    m.grip_to_wrist_rotation=profile.mount.calibration.grip_to_wrist_rotation;
    return wire::Status::ready;
}
} // namespace gears_body_main_mount_sender
