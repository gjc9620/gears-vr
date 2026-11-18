#pragma once
#include "body_goal_view_adapter.h"
#include "cover_posture_fit.h"

// Prepare ONCE at an independently authenticated neutral-cover boundary.
// The current C0 supplies measured hip height, not a new eye calibration.
// Eye/wall/view and feet remain the placement owner's explicit inputs.
// Callers must freeze/invalidate this value with their cover basis; neither
// this math nor a successful planner result grants collision/write authority.
namespace gears_cover_neutral_goal {
namespace a=gears_body_goal_view;
namespace fit=gears_cover_posture_fit;
namespace b=a::body;
enum class Status {okay,alias,placement,measurement,goal,fit,basis};
// Stable calibration domain only, not a new source receipt. Per-frame Scene,
// publication, metadata/tracking clocks and input sequences are not cached.
struct Basis {
    a::core::NativeSessionKey native{};
    gears_weapon_hand::ObjectIdentity controller{};
    a::vr::Pose reference{};a::core::rig::NativeReference native_reference{};
    a::vr::StereoSource source{};
    std::uint64_t session{},space{},native_revision{},observed_stream{},observed_reference{},observed_head{},producer_stream{};
    std::uint64_t publisher_pid{},publisher_start{};
    b::rig::Signature geometry{};double units{},model_scale{};
};
struct Result {
    a::Neutral neutral{};
    a::CoverPlacement binding{};
    a::CoverPoseMeasurement measured{};
    fit::Result fitting{};
    Basis basis{};
};
struct Workspace {fit::Workspace fitting{};};
inline a::core::CoreSnapshot const& parent(a::core::CoreSnapshot const& source) noexcept {return source;}
inline a::core::CoreSnapshot const& parent(gears_body_tick_source::Input const& source) noexcept {return source.observed.parent_scene;}
inline a::vr::StereoSample const& tracking(a::core::CoreSnapshot const& source) noexcept {return source.stereo;}
inline a::vr::StereoSample const& tracking(gears_body_tick_source::Input const& source) noexcept {return source.observed.stereo;}
inline Basis basis(a::core::CoreSnapshot const& s,b::Model const& model,b::Frame const& frame) noexcept {
    Basis out{};out.native=s.native;out.controller=s.controller;out.reference=s.reference;out.native_reference=s.native_reference;
    out.source=s.stereo.source;out.session=s.stereo.session;out.space=s.stereo.space_epoch;out.native_revision=s.native_reference_revision;
    out.observed_stream=s.observed.stream;out.observed_reference=s.observed.reference;out.observed_head=s.observed.head_tracking;
    out.producer_stream=s.producer.stream;out.publisher_pid=s.controller_publisher_pid;out.publisher_start=s.controller_publisher_start;
    out.geometry=model.signature;out.units=frame.units_per_metre;out.model_scale=frame.model_scale;return out;
}
inline Basis basis(gears_body_tick_source::Input const& s,b::Model const& model,b::Frame const& frame) noexcept {
    auto out=basis(parent(s),model,frame);auto const& v=s.observed;
    out.source=v.stereo.source;out.session=v.stereo.session;out.space=v.stereo.space_epoch;
    out.native_revision=v.reference_revision;out.native_reference=v.native_reference;
    out.producer_stream=v.continuity.stream_epoch;out.publisher_pid=v.publisher_pid;out.publisher_start=v.publisher_start;return out;
}
inline bool same(Basis const& a,Basis const& b) noexcept {
    return a::core::same(a.native,b.native)&&gears_weapon_hand::same(a.controller,b.controller)&&
        a::vr::stereo_same_pose(a.reference,b.reference)&&a.native_reference.epoch==b.native_reference.epoch&&
        a.native_reference.flags==b.native_reference.flags&&a.native_reference.reserved==b.native_reference.reserved&&
        a.source==b.source&&a.session==b.session&&a.space==b.space&&a.native_revision==b.native_revision&&
        a.observed_stream==b.observed_stream&&a.observed_reference==b.observed_reference&&a.observed_head==b.observed_head&&
        a.producer_stream==b.producer_stream&&a.publisher_pid==b.publisher_pid&&a.publisher_start==b.publisher_start&&
        a.geometry==b.geometry&&a.units==b.units&&a.model_scale==b.model_scale;
}
inline bool same(a::vr::Vector a,a::vr::Vector b) noexcept {return a.x==b.x&&a.y==b.y&&a.z==b.z;}
inline bool same(a::vr::CoverViewAnchor const& a,a::vr::CoverViewAnchor const& b) noexcept {
    return same(a.neutral_eye_world,b.neutral_eye_world)&&same(a.forward_world,b.forward_world)&&same(a.right_world,b.right_world);
}
inline bool same(b::Pose const& x,b::Pose const& y) noexcept {
    return x.position.x==y.position.x&&x.position.y==y.position.y&&x.position.z==y.position.z&&
        x.orientation.x==y.orientation.x&&x.orientation.y==y.orientation.y&&
        x.orientation.z==y.orientation.z&&x.orientation.w==y.orientation.w;
}
inline bool same_placement(a::CoverPlacement const& x,a::CoverPlacement const& y) noexcept {
    return gears_weapon_hand::same(x.world,y.world)&&gears_weapon_hand::same(x.pawn,y.pawn)&&
        gears_weapon_hand::same(x.mesh,y.mesh)&&gears_weapon_hand::same(x.link,y.link)&&
        x.world_generation==y.world_generation&&x.reference==y.reference&&x.slot==y.slot&&x.type==y.type&&
        same(x.anchor,y.anchor)&&same(x.view_anchor,y.view_anchor)&&
        same(x.feet_world[0],y.feet_world[0])&&same(x.feet_world[1],y.feet_world[1]);
}
inline bool fitted(Result const& frozen,a::Policy const& policy) noexcept {
    auto const& g=frozen.fitting.goal;auto const& n=frozen.neutral;
    return frozen.fitting.validation.status==b::Status::solved&&g.scope==b::Scope::fixed_cover&&
        g.target_mode==b::BodyTargetMode::world_absolute&&g.body_yaw_delta==0&&g.posture==policy.posture&&
        b::d::length(g.physical_world)==0&&b::d::length(g.committed_grip_world)==0&&
        same(n.eye_anchor,frozen.binding.view_anchor)&&
        same(b::Pose{n.pelvis_world,n.root_bone_world_q},b::Pose{g.absolute.neutral_pelvis_world,g.absolute.root_orientation.value})&&
        same(n.feet_world[0],g.feet_world[0])&&same(n.feet_world[1],g.feet_world[1]);
}
// The selected overload performs its own unchanged Core or V3 admission.
template<class Source> inline Status prepare(a::Neutral const& calibrated,a::CoverPlacement const& placement,
    Source const& source,b::Model const& model,b::Frame const& frame,
    a::marker::Marker const& marker,a::Policy const& policy,fit::Limits limits,
    Workspace& work,Result& out) noexcept {
    using namespace b::d;
    const Range ranges[]={range(&calibrated,sizeof calibrated),range(&placement,sizeof placement),
        range(&source,sizeof source),range(&model,sizeof model),range(&frame,sizeof frame),
        range(&marker,sizeof marker),range(&policy,sizeof policy),range(&work,sizeof work),range(&out,sizeof out)};
    if(!disjoint(ranges,9))return Status::alias;
    Result next{};
    if(a::place_cover_head(calibrated,placement,source,model,frame,marker,policy,next.neutral)!=a::Status::okay)
        return Status::placement;
    if(a::measure_cover_pose(source,model,frame,marker,policy,work.fitting.body_work,next.measured)!=a::Status::okay)
        return Status::measurement;
    // The fitter sees reference-relative zero displacement/orientation only.
    // Preserve the original Core, clocks and selected tracking untouched;
    // this local sample is solely a mathematical pre-HMD target, never a
    // submitted tracking sample or a current-source certificate.
    auto const& original=parent(source);
    auto neutral_tracking=original.stereo;neutral_tracking.head=original.reference;
    a::Output target{};
    if(a::prepare_axes_math(next.neutral,placement.anchor,original,neutral_tracking,model,frame,
        marker,{},policy,target)!=a::Status::okay)return Status::goal;
    // The owner now selects a reference-facing torso under the neutral head.
    // Keep the original measurement as evidence, but do not replace this new
    // authored pelvis with the retail back-to-cover hip before fitting it.
    const auto fit_pelvis=policy.posture==b::Posture::reference_neutral&&
        marker.basis()==a::marker::Basis::marcus_mesh_face_forward_y?
        target.goal.absolute.neutral_pelvis_world:next.measured.pelvis.position;
    if(fit::fit(model,frame,target.goal,fit_pelvis,limits,work.fitting,next.fitting)!=fit::Status::okay)
        return Status::fit;
    next.neutral.pelvis_world=next.fitting.goal.absolute.neutral_pelvis_world;
    next.binding=placement;
    next.basis=basis(source,model,frame);
    out=next;return Status::okay;
}

// Convert a previously fitted, frozen pre-HMD neutral using the current
// admitted source. Never place/refit the pelvis or reuse old tracking. Current
// placement evidence must independently retain the same wall/view/feet domain.
// The caller still owns current geometry/occupancy and the sole write permit.
template<class Source> inline Status convert(Result const& frozen,a::CoverPlacement const& placement,
    Source const& source,b::Model const& model,b::Frame const& frame,
    a::marker::Marker const& marker,a::vr::CoverOffset const& committed,
    a::Policy const& policy,a::Output& out) noexcept {
    using namespace b::d;
    const Range ranges[]={range(&frozen,sizeof frozen),range(&placement,sizeof placement),range(&source,sizeof source),
        range(&model,sizeof model),range(&frame,sizeof frame),range(&marker,sizeof marker),range(&committed,sizeof committed),
        range(&policy,sizeof policy),range(&out,sizeof out)};
    if(!disjoint(ranges,9))return Status::alias;
    if(a::cover_placement_current(placement,source,policy)!=a::Status::okay)return Status::placement;
    if(!fitted(frozen,policy)||
       !same(frozen.basis,basis(source,model,frame))||!same_placement(frozen.binding,placement))return Status::basis;
    a::Output next{};
    if(a::prepare_axes_math(frozen.neutral,placement.anchor,parent(source),tracking(source),model,frame,
        marker,committed,policy,next)!=a::Status::okay)return Status::goal;
    out=next;return Status::okay;
}
} // namespace gears_cover_neutral_goal
