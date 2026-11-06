#pragma once
#include "body_goal_view_adapter.h"

// Owned-buffer math. capture is called only once by the live owner before its
// first write. Moving this anchor reads CTW only, never a new animated Local.
namespace gears_body_c0_neutral {
namespace a=gears_body_goal_view;namespace b=gears_full_body_c0;
struct Stored {b::Vec eye{},pelvis{},forward{},right{};b::Quat root{};b::Pose feet[2]{};};
inline b::ik::Matrix matrix(b::Matrix64 const& input) noexcept {
    b::ik::Matrix m{};for(unsigned i=0;i<16;++i)m.m[i]=input.values[i];return m;
}
inline bool rigid(b::ik::Matrix const& m) noexcept {
    return b::d::similarity(m)&&std::abs(b::d::length(b::d::row(m,0))-1)<2e-5;
}
inline b::Vec world_point(b::ik::Matrix const& m,b::Vec p) noexcept {
    return {p.x*m.m[0]+p.y*m.m[4]+p.z*m.m[8]+m.m[12],
        p.x*m.m[1]+p.y*m.m[5]+p.z*m.m[9]+m.m[13],p.x*m.m[2]+p.y*m.m[6]+p.z*m.m[10]+m.m[14]};
}
inline b::Vec local_point(b::ik::Matrix const& m,b::Vec p) noexcept {return b::d::inverse_direction(m,b::d::sub(p,b::d::position(m)));}
inline bool move(Stored const& stored,b::Matrix64 const& ctw,a::Neutral& output) noexcept {
    auto m=matrix(ctw);if(!rigid(m))return false;const auto q=b::d::rotation(m);
    a::Neutral n{};n.eye_anchor={world_point(m,stored.eye),b::d::rotate(q,stored.forward),b::d::rotate(q,stored.right)};
    n.pelvis_world=world_point(m,stored.pelvis);n.root_bone_world_q=b::d::product(q,stored.root);
    for(unsigned i=0;i<2;++i)n.feet_world[i]={world_point(m,stored.feet[i].position),b::d::product(q,stored.feet[i].orientation)};
    if(!gears_vr::cover_view_basis(n.eye_anchor)||!b::d::finite_vector(n.pelvis_world)||!b::d::goal_pose(n.feet_world[0])||!b::d::goal_pose(n.feet_world[1]))return false;
    output=n;return true;
}
inline bool capture(b::Model const& model,b::Frame const& frame,a::marker::Marker const& marker,
                    b::Workspace& work,Stored& output) noexcept {
    if(frame.count!=b::bones||frame.model_scale!=1||a::marker::detail::context(model,frame,model.canonical)!=a::marker::Status::okay)return false;
    work.component=matrix(frame.component_to_world);if(!rigid(work.component)||
        b::ik::detail::validate(frame.local.data(),model.parents.data(),b::bones,work.component)!=b::ik::Status::okay)return false;
    for(auto const& atom:frame.local)if(atom.scale!=1)return false;
    std::copy(frame.local.begin(),frame.local.end(),work.pose.local);if(!b::d::rebuild(model,work))return false;
    // Freeze the actual standing eye and ground contacts. The torso below is
    // reference-neutral, so freezing the old aiming pelvis as well would force
    // a compressed, sideways spine to reach the old animated eye. Place that
    // neutral torso under the SAME eye once; legs solve to the SAME contacts.
    // No later animation/tracking sample rebases any of these points.
    b::Pose contacts[2]{};
    for(unsigned i=0;i<2;++i)contacts[i]=b::d::pose(work.pose.world[b::legs[i][2]]);
    const auto initial_root=b::d::rotation(work.pose.world[b::root]);
    a::marker::EyeHmdWorld initial_eye{};
    if(a::marker::eye_from_head(marker,model,frame,{b::d::pose(work.pose.world[b::head])},initial_eye)!=a::marker::Status::okay)return false;
    std::copy(model.reference.begin(),model.reference.end(),work.pose.local);
    if(!b::d::rebuild(model,work))return false;
    a::marker::EyeHmdWorld eye{};
    if(a::marker::eye_from_head(marker,model,frame,{b::d::pose(work.pose.world[b::head])},eye)!=a::marker::Status::okay)return false;
    // The standing view uses reference yaw, never animated weapon-stance yaw.
    const auto f=b::d::rotate(eye.pose.orientation,{1,0,0});const double n=std::hypot(f.x,f.y);
    if(!std::isfinite(n)||n<.5)return false;const b::Vec forward{f.x/n,f.y/n,0},right{-f.y/n,f.x/n,0};
    a::marker::HeadBoneWorld neutral_head{};
    const a::marker::EyeHmdWorld fixed_eye{{initial_eye.pose.position,b::d::yaw(std::atan2(forward.y,forward.x))}};
    if(a::marker::head_from_eye(marker,model,frame,fixed_eye,neutral_head)!=a::marker::Status::okay)return false;
    // Use exactly the live planner's neutral rotations and actual translations,
    // lengths, scale and root orientation, rather than a different Ref body.
    std::copy(frame.local.begin(),frame.local.end(),work.pose.local);
    for(unsigned i:b::neutral_rotation_bones)std::memcpy(work.pose.local[i].q,model.reference[i].q,16);
    if(!b::d::rebuild(model,work))return false;
    const auto neutral_pelvis=b::d::add(b::d::position(work.pose.world[b::pelvis]),
        b::d::sub(neutral_head.pose.position,b::d::position(work.pose.world[b::head])));
    if(!b::d::finite_vector(neutral_pelvis))return false;
    const auto inv=b::d::inverse(b::d::rotation(work.component));Stored next{};
    next.eye=local_point(work.component,initial_eye.pose.position);next.pelvis=local_point(work.component,neutral_pelvis);
    next.forward=b::d::rotate(inv,forward);next.right=b::d::rotate(inv,right);
    next.root=b::d::product(inv,initial_root);
    for(unsigned i=0;i<2;++i){const auto& p=contacts[i];next.feet[i]={local_point(work.component,p.position),b::d::product(inv,p.orientation)};}
    a::Neutral check{};if(!move(next,frame.component_to_world,check))return false;output=next;return true;
}
}
