#pragma once
#include "crosshair_scene_evidence.h"
#include "../vr-common/stereo_ticket_store.h"
#include <cmath>
#include <cstring>

// Experimental world HUD-query point eligibility only. This does not certify
// current-camera aim, a new query timestamp, or bullet impact. No game reads.
namespace gears_crosshair_finite_query {
enum class Reason {accepted,binding,clock,age,geometry};
struct Authority {
    std::uintptr_t controller{},pawn{},weapon{},camera{},local{},level{},family{},source{},owner{};
    std::uint64_t world{},invocation{};
    std::uint32_t thread{},scene_stamp{};
};
inline Reason authorize(gears_crosshair_scene::Metadata const& data,Authority const& expected,
    gears_vr::StereoMatrix const& source_view,std::int64_t now_qpc,std::int64_t qpc_frequency,
    gears_vr::StereoFiniteAim& out) {
    auto const& view=data.writer_view;
    if(!expected.controller||!expected.pawn||!expected.weapon||!expected.camera||!expected.local||!expected.level||
       !expected.family||!expected.source||!expected.owner||expected.owner!=expected.family||
       !expected.world||!expected.invocation||!expected.thread||!data.tick||!data.root_frame||
       data.controller!=expected.controller||data.pawn!=expected.pawn||data.weapon!=expected.weapon||
       data.local!=expected.local||data.family!=expected.family||data.source!=expected.source||
       data.owner!=expected.owner||data.owner!=data.family||data.world!=expected.world||
       data.invocation!=expected.invocation||data.thread!=expected.thread||data.stamp!=expected.scene_stamp||
       !view.valid||!view.camera_binding_valid||view.camera!=expected.camera||
       view.local!=expected.local||view.level!=expected.level)return Reason::binding;
    if(qpc_frequency<=0||data.begin_qpc<=0||data.assignment_qpc<data.begin_qpc||
       data.finish_qpc<data.assignment_qpc||data.scene_qpc<data.finish_qpc||now_qpc<data.scene_qpc)
        return Reason::clock;
    // Independent host-clock bounds. Positive ordered times keep subtraction
    // within signed range; integer division conservatively floors 50 ms.
    if(now_qpc-data.begin_qpc>qpc_frequency/20||now_qpc-data.assignment_qpc>qpc_frequency/20)
        return Reason::age;
    float assignment_stamp{},scene_stamp{};
    static_assert(sizeof(assignment_stamp)==sizeof(view.assignment_stamp));
    std::memcpy(&assignment_stamp,&view.assignment_stamp,sizeof(assignment_stamp));
    std::memcpy(&scene_stamp,&expected.scene_stamp,sizeof(scene_stamp));
    if(!std::isfinite(assignment_stamp)||!std::isfinite(scene_stamp)||assignment_stamp<0||scene_stamp<0||
       scene_stamp<assignment_stamp)return Reason::clock;
    // Engine stamps are an independent continuity bracket, not a query time.
    if(double(scene_stamp)-double(assignment_stamp)>.05)return Reason::age;
    for(float value:data.point)if(!std::isfinite(value))return Reason::geometry;
    if(!gears_vr::stereo_rigid(source_view))return Reason::geometry;
    // Row vector WORLD(point,1) * frozen source view. Off-ray points are legal.
    const double depth=double(data.point[0])*source_view[2]+double(data.point[1])*source_view[6]+
        double(data.point[2])*source_view[10]+source_view[14];
    const double w=double(data.point[0])*source_view[3]+double(data.point[1])*source_view[7]+
        double(data.point[2])*source_view[11]+source_view[15];
    if(!std::isfinite(depth)||depth<=0||!std::isfinite(w)||w!=1)return Reason::geometry;
    out={true,data.point,data.world,data.tick};return Reason::accepted;
}
}
