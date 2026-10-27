#pragma once
#include "../vr-common/weapon_main_mount_provider.h"
#include "weapon_main_mount_source.h"
#include <cstring>

// PG-local POD translation only. Caller supplies the broker/PG lifetime,
// authenticated GT/parent and fresh prepare_retained result. None of these
// fields is a new registration, Native scope, writer or retained permission.
namespace gears_weapon_main_mount_sender {
namespace wire=gears_weapon_main_mount_provider;
namespace source=gears_weapon_main_mount_source;
namespace policy=gears_weapon_main_mount;
namespace hand=gears_weapon_hand;
struct Parent {
    std::uint64_t cookie{},native_started_qpc{};
    std::uint32_t game_pid{},game_thread{};
};
inline wire::Status translate(policy::Status s) noexcept {
    switch(s){
#define GEARS_MAIN_MOUNT_STATUS(name) case policy::Status::name:return wire::Status::name
        GEARS_MAIN_MOUNT_STATUS(ready);GEARS_MAIN_MOUNT_STATUS(inactive);GEARS_MAIN_MOUNT_STATUS(no_weapon);
        GEARS_MAIN_MOUNT_STATUS(unheld);GEARS_MAIN_MOUNT_STATUS(animation);GEARS_MAIN_MOUNT_STATUS(busy);
        GEARS_MAIN_MOUNT_STATUS(not_firearm);GEARS_MAIN_MOUNT_STATUS(unsupported_asset);GEARS_MAIN_MOUNT_STATUS(not_attached);
        GEARS_MAIN_MOUNT_STATUS(body_unavailable);GEARS_MAIN_MOUNT_STATUS(body_mismatch);GEARS_MAIN_MOUNT_STATUS(unsupported_geometry);
#undef GEARS_MAIN_MOUNT_STATUS
    }
    return wire::Status::inactive;
}
inline wire::Status context(wire::Request const& r,Parent const& parent,std::int64_t now,
    source::BodyContext& out) noexcept {
    out={};
    if(r.size!=sizeof(r)||r.abi_version!=wire::version||!r.query_id||!r.hand_lifecycle||
       r.maximum_age_ns!=wire::maximum_source_age_ns||!parent.cookie||!parent.native_started_qpc||
       !parent.game_pid||!parent.game_thread)return wire::Status::source_mismatch;
    auto const& retained=r.retained;auto const& n=retained.native;
    const auto selected=static_cast<std::uint32_t>(retained.stereo.source);
    if(selected>1||n.registration_cookie!=parent.cookie||n.native_started_qpc!=parent.native_started_qpc||
       n.game_pid!=parent.game_pid||n.game_thread!=parent.game_thread||
       !gears_rig_tracking::frame_valid(retained,now,r.maximum_age_ns,selected==1))return wire::Status::source_mismatch;
    // The request's age ceiling bounds the original input sync too; a fresh
    // receipt cannot renew an older controller sample.
    if(retained.controllers.sync_ns<=0||now<retained.controllers.sync_ns||
       std::uint64_t(now-retained.controllers.sync_ns)>r.maximum_age_ns)return wire::Status::source_mismatch;
    auto const& b=r.body;
    if(!b.revision||!hand::identity_valid(b.mesh))return wire::Status::body_unavailable;
    if(b.reserved||b.revision!=retained.binding_generation||!hand::same(b.mesh,retained.mesh)||
       b.units_per_metre!=retained.units_per_metre||b.basis!=wire::Basis::marcus_mesh_face_forward_y||
       b.basis_revision!=wire::supported_basis_revision)return wire::Status::body_mismatch;
    policy::lancer::Profile profile{};
    if(!policy::lancer::marcus115(b.geometry,b.model_scale,b.units_per_metre,profile))return wire::Status::unsupported_geometry;
    source::BodyContext value{};value.known=true;value.native=n;
    value.publication_sequence=retained.publication_sequence;value.scene_invocation=retained.scene_invocation;
    value.world_generation=retained.world_generation;value.binding_generation=retained.binding_generation;
    value.reference_generation=retained.reference_generation;value.mesh=b.mesh;value.geometry=b.geometry;
    value.model_scale=b.model_scale;value.units_per_metre=b.units_per_metre;value.revision=b.revision;
    value.basis_revision=b.basis_revision;value.basis=policy::Basis::marcus_mesh_face_forward_y;
    out=value;return wire::Status::ready;
}
inline bool calibration_same(gears_hand_wrist::Calibration const& a,gears_hand_wrist::Calibration const& b) noexcept {
    auto const& x=a.grip_to_wrist_metres;auto const& y=b.grip_to_wrist_metres;
    auto const& q=a.grip_to_wrist_rotation;auto const& p=b.grip_to_wrist_rotation;
    return a.known&&b.known&&x.x==y.x&&x.y==y.y&&x.z==y.z&&q.x==p.x&&q.y==p.y&&q.z==p.z&&q.w==p.w;
}
inline wire::Status response(wire::Request const& r,Parent const& parent,policy::Candidate const& c,
    std::int64_t now,wire::Response& out) noexcept {
    // Clear the caller's persistent response directly; do not put another
    // wire-sized response temporary on the C0/assembly-hook call stack.
    std::memset(&out,0,sizeof(out));out.size=sizeof(out);out.abi_version=wire::version;
    source::BodyContext body{};const auto status=context(r,parent,now,body);
    if(status!=wire::Status::ready)return status;
    auto const& f=r.retained;auto const& k=c.key;
    if(k.registration_cookie!=parent.cookie||k.native_started_qpc!=parent.native_started_qpc||
       k.game_pid!=parent.game_pid||k.thread_id!=parent.game_thread||k.game_start!=f.native.game_start||
       k.lifecycle!=r.hand_lifecycle||k.frame!=f.scene_invocation||k.world_generation!=f.world_generation||
       !hand::same(c.world,f.world)||!hand::same(c.controller,f.controller)||!hand::same(c.pawn,f.pawn)||
       !hand::same(c.mesh,f.mesh)||!hand::same(c.weapon,f.weapon)||!hand::identity_valid(c.component)||
       !hand::identity_valid(c.asset)||!c.hold_generation||c.provider_revision!=wire::provider_revision||
       c.body_revision!=body.revision||c.basis_revision!=body.basis_revision||c.basis!=body.basis||
       c.geometry!=body.geometry||c.model_scale!=body.model_scale||c.units_per_metre!=body.units_per_metre)
        return wire::Status::source_mismatch;
    policy::lancer::Profile profile{};
    if(!policy::lancer::marcus115(c.geometry,c.model_scale,c.units_per_metre,profile)||
       !calibration_same(c.right,profile.mount.calibration))return wire::Status::source_mismatch;
    out.query_id=r.query_id;out.key=c.key;out.world=c.world;out.controller=c.controller;
    out.pawn=c.pawn;out.mesh=c.mesh;out.weapon=c.weapon;out.component=c.component;out.asset=c.asset;
    out.hold_generation=c.hold_generation;out.provider_revision=c.provider_revision;out.body_revision=c.body_revision;
    out.basis_revision=c.basis_revision;out.basis=wire::Basis::marcus_mesh_face_forward_y;
    out.geometry=c.geometry;out.model_scale=c.model_scale;out.units_per_metre=c.units_per_metre;
    out.calibration_known=1;out.grip_to_wrist_metres=c.right.grip_to_wrist_metres;
    out.grip_to_wrist_rotation=c.right.grip_to_wrist_rotation;return wire::Status::ready;
}
static_assert(wire::provider_revision==policy::provider_revision&&wire::supported_basis_revision==policy::supported_basis_revision);
} // namespace gears_weapon_main_mount_sender
