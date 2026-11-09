#pragma once
#include "body_head_authored_mount.h"
#include "../vr-common/cover_grab_pose.h"

// Owned-buffer append to the EXISTING C0 transaction, never a second writer.
// The live owner authenticates the frame, edge and lease and supplies the
// current body candidate. This adapter grants no collision/body permission.
namespace gears_cover_grab_pose_adapter {
namespace body=gears_full_body_c0;
namespace grab=gears_cover_grab_pose;
namespace vr=gears_vr;
enum class Status {prepared,inactive,alias,binding,reference,body,pose,unreachable,residual,lod};
struct Result {Status status{Status::prepared};grab::Code pose{grab::Code::prepared};};
struct Workspace {
    body::Workspace body_work{};
    grab::Plan target{};
    body::two::Input arm{};
    body::two::Solution solution{};
};
inline Result compose(body::Model const& model,body::Frame const& frame,
    grab::Frame const& input,grab::Contact const& contact,grab::Profile const& profile,
    std::array<std::uint64_t,2> const& desired_bones,
    Workspace& work,body::Output& inout) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&model,sizeof model),range(&frame,sizeof frame),range(&input,sizeof input),
        range(&contact,sizeof contact),range(&profile,sizeof profile),range(&desired_bones,sizeof desired_bones),
        range(&work,sizeof work),range(&inout,sizeof inout)};
    if(!disjoint(ranges,8))return {Status::alias};
    work.target={};
    if(frame.model_scale!=1||frame.model_scale!=input.model_scale||frame.units_per_metre!=input.units_per_metre||
       model.signature!=input.actual_geometry_signature)return {Status::binding};
    if(gears_body_head_authored::detail::context(model,frame,model.canonical)!=gears_body_head_authored::Status::okay)
        return {Status::reference};
    const auto code=grab::prepare(input,contact,profile,work.target);
    if(code!=grab::Code::prepared)return {code==grab::Code::inactive?Status::inactive:Status::pose,code};
    // The live C0 owner supplies THIS call's already authenticated DesiredBones
    // snapshot. There is deliberately no default-all mask. A partial hand is
    // not an applied contact: require all written joints and their ancestors.
    // These bits alone do not authenticate a LOD/publication or grant a write.
    if(desired_bones[1]>>(body::bones-64))return {Status::lod};
    for(unsigned first=8;first<27;++first){
        unsigned bone=first;
        for(;;){
            if(!(desired_bones[bone/64]&(std::uint64_t(1)<<(bone%64))))return {Status::lod};
            if(!bone)break;
            bone=unsigned(model.parents[bone]);
        }
    }
    auto& w=work.body_work;
    for(unsigned i=0;i<16;++i)w.component.m[i]=frame.component_to_world.values[i];
    if(!similarity(w.component)||std::abs(length(row(w.component,0))-1)>2e-5||
       inout.report.status!=body::Status::solved)return {Status::body};
    if(body::ik::detail::validate(inout.local.data(),model.parents.data(),body::bones,w.component)!=body::ik::Status::okay)
        return {Status::body};
    std::copy(inout.local.begin(),inout.local.end(),w.pose.local);
    if(!rebuild(model,w))return {Status::body};
    std::copy_n(w.pose.world,body::bones,w.original_world.begin());
    // Endpoints must belong to this exact current complete body candidate.
    const unsigned stable[]={body::root,body::pelvis,body::head,body::legs[0][2],body::legs[1][2]};
    const body::Pose expected[]={inout.root_world,inout.pelvis_world,inout.head_world,inout.feet_world[0],inout.feet_world[1]};
    for(unsigned i=0;i<5;++i)
        if(!goal_pose(expected[i])||!same_position(position(w.pose.world[stable[i]]),expected[i].position,.002)||
           angle(rotation(w.pose.world[stable[i]]),expected[i].orientation)>1e-4)return {Status::body};
    if(model.parents[10]!=8||model.parents[11]!=10||model.parents[9]!=8)return {Status::reference};
    // Restore only the helper rotation; preserve all translations and scales.
    std::memcpy(w.pose.local[9].q,model.reference[9].q,16);
    if(!rebuild(model,w))return {Status::body};
    auto& arm=work.arm;arm={};
    arm.shoulder=pose(w.pose.world[8]);arm.elbow=pose(w.pose.world[10]);arm.wrist=pose(w.pose.world[11]);
    arm.target=work.target.wrist_world;arm.authored={true,arm.elbow.position};
    arm.lengths.mode=body::two::LengthMode::preserve;arm.share_forearm_twist=true;
    // A contact is not a free hand: do not project the target away from its
    // surface or stretch an arm in order to report a successful grip pose.
    const auto solved=body::two::solve(arm,work.solution);
    if(solved!=body::two::Status::solved)return {Status::unreachable};
    const unsigned chain[]={8,10,11};
    const body::Pose targets[]={work.solution.shoulder,work.solution.elbow,work.solution.wrist};
    for(unsigned i=0;i<3;++i)if(!world_rotation(model,w,chain[i],targets[i].orientation))return {Status::residual};
    for(unsigned n=0;n<15;++n){
        const unsigned i=12+n;
        if(model.parents[i]!=int(n%3?i-1:11)||!set_quat(w.pose.local[i],work.target.joints[n]))return {Status::reference};
    }
    if(!rebuild(model,w)||!same_position(position(w.pose.world[11]),arm.target.position,.002)||
       angle(rotation(w.pose.world[11]),arm.target.orientation)>1e-4)return {Status::residual};
    for(unsigned i:stable)if(std::memcmp(&w.pose.world[i],&w.original_world[i],sizeof(body::ik::Matrix)))return {Status::residual};
    std::memcpy(&w.candidate,&inout,sizeof inout);
    double error=inout.report.max_length_error;
    for(unsigned i=0;i<body::bones;++i){
        const bool owned=i==8||i==9||i==10||i==11||(i>=12&&i<27);
        if(std::memcmp(w.pose.local[i].translation,inout.local[i].translation,12)||
           std::memcmp(&w.pose.local[i].scale,&inout.local[i].scale,4)||
           (!owned&&std::memcmp(&w.pose.local[i],&inout.local[i],sizeof(body::Atom))))return {Status::residual};
        if(i){const unsigned parent=unsigned(model.parents[i]);
            error=(std::max)(error,std::abs(length(sub(position(w.pose.world[i]),position(w.pose.world[parent])))-
                length(sub(position(w.original_world[i]),position(w.original_world[parent])))));}
        w.candidate.local[i]=w.pose.local[i];
        const auto bit=std::uint64_t(1)<<(i%64);
        if(std::memcmp(&w.candidate.local[i],&frame.local[i],sizeof(body::Atom)))w.candidate.changed_mask[i/64]|=bit;
        else w.candidate.changed_mask[i/64]&=~bit;
    }
    if(!std::isfinite(error)||error>.002)return {Status::residual};
    w.candidate.report.max_length_error=error;
    w.candidate.shoulders_world[0]=pose(w.pose.world[8]);
    std::memcpy(&inout,&w.candidate,sizeof inout);return {};
}
} // namespace gears_cover_grab_pose_adapter
