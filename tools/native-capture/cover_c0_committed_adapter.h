#pragma once
#include "cover_committed_drag.h"

namespace gears_cover_c0_committed {
namespace c=gears_cover_committed;
namespace ng=c::ng;
namespace a=c::a;
namespace b=a::body;
enum class Code {prepared,alias,domain,placement,fit,epochs,convert};
struct Frozen {ng::Result neutral{};gears_vr::CoverEpochs epochs{};};
struct Prepared {
    a::Output body{};
    std::int64_t interaction_observed_ns{},tracking_received_ns{};
    std::uint64_t scene{},tracking_sequence{};
};
inline bool placement(c::State const& state,a::CoverPlacement const& p)noexcept{
    return p.type==state.type&&p.slot==state.key.slot&&p.world.address==state.key.world&&
        p.world_generation==state.key.world_generation&&gears_weapon_hand::same(p.pawn,state.pawn)&&
        gears_weapon_hand::same(p.mesh,state.mesh)&&gears_weapon_hand::same(p.link,state.link)&&
        ng::same(p.anchor,state.anchor)&&ng::same(p.view_anchor,state.view_anchor);
}
// The owner must separately establish the ORIGINAL neutral action before this
// one-time fit. No current/previous driven pose may serve as a new calibration.
template<class Source>Code freeze(c::State const& state,a::Neutral const& calibrated,
    a::CoverPlacement const& current,Source const& source,b::Model const& model,b::Frame const& frame,
    a::marker::Marker const& marker,a::Policy const& policy,ng::fit::Limits limits,
    ng::Workspace& work,Frozen& output)noexcept{
    using namespace b::d;
    const Range ranges[]={range(&state,sizeof state),range(&calibrated,sizeof calibrated),range(&current,sizeof current),
        range(&source,sizeof source),range(&model,sizeof model),range(&frame,sizeof frame),range(&marker,sizeof marker),
        range(&policy,sizeof policy),range(&work,sizeof work),range(&output,sizeof output)};
    if(!disjoint(ranges,10))return Code::alias;
    if(!c::domain(state,source,policy))return Code::domain;
    if(!placement(state,current))return Code::placement;
    Frozen next{};
    if(ng::prepare(calibrated,current,source,model,frame,marker,policy,limits,work,next.neutral)!=ng::Status::okay)return Code::fit;
    next.epochs=state.key.epochs;output=next;return Code::prepared;
}
template<class Source>Code prepare(Frozen const& frozen,c::State const& state,a::CoverPlacement const& current,
    Source const& source,b::Model const& model,b::Frame const& frame,a::marker::Marker const& marker,
    a::Policy const& policy,Prepared& output)noexcept{
    using namespace b::d;
    const Range ranges[]={range(&frozen,sizeof frozen),range(&state,sizeof state),range(&current,sizeof current),
        range(&source,sizeof source),range(&model,sizeof model),range(&frame,sizeof frame),range(&marker,sizeof marker),
        range(&policy,sizeof policy),range(&output,sizeof output)};
    if(!disjoint(ranges,9))return Code::alias;
    if(!c::domain(state,source,policy))return Code::domain;
    if(!c::epochs(frozen.epochs,state.key.epochs))return Code::epochs;
    if(!placement(state,current))return Code::placement;
    Prepared next{};
    // convert admits the CURRENT Core/V3 tracking, geometry and clocks. The
    // persistent offset contributes only virtual right/up displacement.
    if(ng::convert(frozen.neutral,current,source,model,frame,marker,state.offset,policy,next.body)!=ng::Status::okay)return Code::convert;
    next.interaction_observed_ns=state.observed_ns;
    next.tracking_received_ns=ng::tracking(source).received_ns;
    next.tracking_sequence=ng::tracking(source).sequence;
    next.scene=ng::parent(source).scene_invocation;
    output=next;return Code::prepared;
}
} // namespace gears_cover_c0_committed
