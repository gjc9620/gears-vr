#pragma once
#include "crosshair_scene_evidence.h"
#include "../vr-common/stereo_ticket_store.h"
#include <cmath>
#include <cstring>
#include <limits>

// Experimental world HUD-query point eligibility only. This does not certify
// a new query timestamp or bullet impact. No game reads. Camera agreement below
// is a numeric freshness guard, not proof of an actual firing call.
namespace gears_crosshair_finite_query {
enum class Reason {accepted,binding,clock,age,geometry,camera};
// A same-object camera can change between UpdateHUDActions and scene creation
// (including recoil). A <=50 ms clock bracket cannot authenticate its old ray.
// Compare in the unmodified source camera, before closer/HMD eye translations.
inline bool camera_matches(gears_crosshair_view::Sample const& writer,
    gears_vr::StereoMatrix const& source_view){
    double direction[3]{},origin[3]{};
    double norm=0;
    for(unsigned i=0;i<3;++i){
        if(!std::isfinite(writer.location[i])||!std::isfinite(writer.direction[i]))return false;
        norm+=double(writer.direction[i])*writer.direction[i];
    }
    if(!std::isfinite(norm)||std::abs(norm-1.)>.001)return false;
    for(unsigned col=0;col<3;++col){
        double magnitude=std::abs(double(source_view[12+col]));
        origin[col]=source_view[12+col];
        for(unsigned row=0;row<3;++row){
            direction[col]+=double(writer.direction[row])*source_view[4*row+col];
            const double term=double(writer.location[row])*source_view[4*row+col];
            origin[col]+=term;magnitude+=std::abs(term);
        }
        // Allow source float quantization/cancellation, not camera movement.
        const double tolerance=.05+8*std::numeric_limits<float>::epsilon()*magnitude;
        if(!std::isfinite(origin[col])||std::abs(origin[col])>tolerance)return false;
    }
    // Allow small Rotator/view quantization and subframe camera differences
    // (~0.057 degrees), while rejecting a visibly changed query direction.
    return std::isfinite(direction[2])&&direction[2]>0&&
        std::hypot(direction[0],direction[1])<=.001*direction[2];
}
struct Authority {
    std::uintptr_t controller{},pawn{},weapon{},camera{},local{},level{},family{},source{},owner{};
    std::uint64_t world{},invocation{};
    std::uint32_t thread{},scene_stamp{};
};
inline Reason authorize(gears_crosshair_scene::Metadata const& data,Authority const& expected,
    gears_vr::StereoMatrix const& source_view,std::int64_t now_qpc,std::int64_t qpc_frequency,
    gears_vr::StereoFiniteAim& out,bool* aiming=nullptr) {
    if(aiming)*aiming=false;
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
    // Targeting state has independent identity/time authority even when the
    // point later fails camera agreement or lies behind the current source.
    if(aiming)*aiming=view.aiming_known&&view.aiming;
    for(float value:data.point)if(!std::isfinite(value))return Reason::geometry;
    if(!gears_vr::stereo_rigid(source_view))return Reason::geometry;
    // Row vector WORLD(point,1) * frozen source view. Off-ray points are legal.
    const double depth=double(data.point[0])*source_view[2]+double(data.point[1])*source_view[6]+
        double(data.point[2])*source_view[10]+source_view[14];
    const double w=double(data.point[0])*source_view[3]+double(data.point[1])*source_view[7]+
        double(data.point[2])*source_view[11]+source_view[15];
    if(!std::isfinite(depth)||depth<=0||!std::isfinite(w)||w!=1)return Reason::geometry;
    if(!camera_matches(view,source_view))return Reason::camera;
    out={true,data.point,data.world,data.tick};return Reason::accepted;
}
}
