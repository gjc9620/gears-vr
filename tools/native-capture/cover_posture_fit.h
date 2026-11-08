#pragma once
#include "full_body_c0_planner.h"

// Pure preparation for a neutral cover pose. The caller supplies a measured
// hip height, explicit feet, wall-facing root and PRE-HMD head target. This
// returns a bounded candidate, never collision or Native write permission.
namespace gears_cover_posture_fit {
namespace body=gears_full_body_c0;
enum class Status {okay,alias,inputs,source,infeasible,iterations,limit,solver};
struct Limits {double horizontal_metres{};};
struct Result {
    body::Goal goal{};body::Report validation{};body::Vec actual_pelvis{};
    double horizontal_units{};unsigned iterations{};
};
struct Workspace {
    body::Workspace body_work{};body::Output planned{};body::Goal goal{};
    body::Vec centres[4]{},correction[4]{};double radii[4]{};
};
inline Status fit(body::Model const& model,body::Frame const& frame,
    body::Goal const& neutral,body::Vec measured_pelvis,Limits limits,
    Workspace& work,Result& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&model,sizeof model),range(&frame,sizeof frame),
        range(&neutral,sizeof neutral),range(&work,sizeof work),range(&out,sizeof out)};
    if(!disjoint(ranges,5))return Status::alias;
    if(!finite_vector(measured_pelvis)||!std::isfinite(limits.horizontal_metres)||
       limits.horizontal_metres<0||limits.horizontal_metres>1||
       neutral.scope!=body::Scope::fixed_cover||
       neutral.target_mode!=body::BodyTargetMode::world_absolute||neutral.body_yaw_delta!=0||
       length(neutral.physical_world)!=0||length(neutral.committed_grip_world)!=0||
       measured_pelvis.z<=(std::max)(neutral.feet_world[0].position.z,neutral.feet_world[1].position.z))return Status::inputs;
    work.goal=neutral;work.goal.absolute.neutral_pelvis_world=measured_pelvis;
    auto validation=body::plan(model,frame,work.goal,work.body_work,work.planned);
    unsigned iterations{};
    if(validation.status!=body::Status::solved){
        if(validation.status!=body::Status::spine_unreachable&&
           validation.status!=body::Status::leg_unreachable)return Status::source;
        if(limits.horizontal_metres==0)return Status::limit;
        auto& fk=work.body_work;
        for(unsigned i=0;i<16;++i)fk.component.m[i]=frame.component_to_world.values[i];
        std::copy(frame.local.begin(),frame.local.end(),fk.pose.local);
        if(!rebuild(model,fk))return Status::source;
        body::Quat root{};
        if(!normalized(neutral.absolute.root_orientation.value,root)||
           !world_rotation(model,fk,body::root,root))return Status::source;
        if(neutral.posture==body::Posture::reference_neutral){
            for(unsigned i:body::neutral_rotation_bones)std::memcpy(fk.pose.local[i].q,model.reference[i].q,16);
            if(!rebuild(model,fk))return Status::source;
        }
        const auto pelvis=position(fk.pose.world[body::pelvis]);
        double spheres[3]{};
        for(unsigned side=0;side<2;++side){
            const auto hip=position(fk.pose.world[body::legs[side][0]]);
            const auto knee=position(fk.pose.world[body::legs[side][1]]);
            const auto foot=position(fk.pose.world[body::legs[side][2]]);
            work.centres[side]=sub(neutral.feet_world[side].position,sub(hip,pelvis));
            spheres[side]=length(sub(knee,hip))+length(sub(foot,knee));
        }
        const auto spine_anchor=position(fk.pose.world[body::spine[0]]);
        work.centres[2]=sub(neutral.head_world.position,sub(spine_anchor,pelvis));
        for(unsigned i=0;i<4;++i)spheres[2]+=length(sub(
            position(fk.pose.world[body::spine[i+1]]),position(fk.pose.world[body::spine[i]])));
        // Fixed hip height intersects the two foot reach balls and head reach
        // ball in three XY discs. Reserve the planner's .001 native-unit
        // float-encoding margin; do not alter any segment or target pose.
        for(unsigned i=0;i<3;++i){
            const auto radius=spheres[i]-.001;
            const auto dz=measured_pelvis.z-work.centres[i].z;
            const auto squared=radius*radius-dz*dz;
            if(!finite_vector(work.centres[i])||!std::isfinite(squared)||radius<=0||squared<0)return Status::infeasible;
            work.radii[i]=std::sqrt(squared);work.centres[i].z=measured_pelvis.z;
        }
        work.centres[3]=measured_pelvis;
        work.radii[3]=limits.horizontal_metres*frame.units_per_metre;
        for(auto& c:work.correction)c={};
        auto point=measured_pelvis;
        for(;iterations<128;++iterations){
            const auto previous=point;
            for(unsigned i=0;i<4;++i){
                const auto x=add(point,work.correction[i]);const auto delta=sub(x,work.centres[i]);
                const auto distance=std::hypot(delta.x,delta.y);
                point=distance>work.radii[i]?add(work.centres[i],mul(delta,work.radii[i]/distance)):x;
                point.z=measured_pelvis.z;work.correction[i]=sub(x,point);
            }
            bool feasible=true;
            for(unsigned i=0;i<4;++i)if(length(sub(point,work.centres[i]))>work.radii[i]+1e-7)feasible=false;
            if(feasible&&length(sub(point,previous))<1e-9){++iterations;break;}
        }
        for(unsigned i=0;i<4;++i)if(length(sub(point,work.centres[i]))>work.radii[i]+1e-7)return Status::iterations;
        work.goal.absolute.neutral_pelvis_world=point;
        validation=body::plan(model,frame,work.goal,work.body_work,work.planned);
        if(validation.status!=body::Status::solved)return Status::solver;
    }
    const auto shift=sub(work.planned.pelvis_world.position,measured_pelvis);
    if(std::abs(shift.z)>.0005||std::hypot(shift.x,shift.y)>
       limits.horizontal_metres*frame.units_per_metre+.0005)return Status::limit;
    Result result{};result.goal=work.goal;result.validation=validation;
    result.actual_pelvis=work.planned.pelvis_world.position;
    result.horizontal_units=std::hypot(shift.x,shift.y);result.iterations=iterations;
    out=result;return Status::okay;
}
} // namespace gears_cover_posture_fit
