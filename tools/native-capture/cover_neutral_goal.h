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
enum class Status {okay,alias,placement,measurement,goal,fit};
struct Result {
    a::Neutral neutral{};
    a::CoverPlacement binding{};
    a::CoverPoseMeasurement measured{};
    fit::Result fitting{};
};
struct Workspace {fit::Workspace fitting{};};
inline Status prepare(a::Neutral const& calibrated,a::CoverPlacement const& placement,
    a::core::CoreSnapshot const& source,b::Model const& model,b::Frame const& frame,
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
    auto neutral_tracking=source.stereo;neutral_tracking.head=source.reference;
    a::Output target{};
    if(a::prepare_axes_math(next.neutral,placement.anchor,source,neutral_tracking,model,frame,
        marker,{},policy,target)!=a::Status::okay)return Status::goal;
    if(fit::fit(model,frame,target.goal,next.measured.pelvis.position,limits,work.fitting,next.fitting)!=fit::Status::okay)
        return Status::fit;
    next.neutral.pelvis_world=next.fitting.goal.absolute.neutral_pelvis_world;
    next.binding=placement;
    out=next;return Status::okay;
}
} // namespace gears_cover_neutral_goal
