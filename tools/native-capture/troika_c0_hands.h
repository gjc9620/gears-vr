#pragma once
#include "body_c0_hands.h"
#include "../vr-common/weapon_troika_driver_scope.h"

// Pose construction for the existing C0 transaction. No ordinary Core/Frame,
// engine access, input selection, clock, lease, or second skeleton writer.
namespace gears_troika_c0_hands {
namespace body=gears_full_body_c0;namespace source=gears_troika_driver;
namespace hands=gears_body_c0_hands;namespace mount=gears_hand_authored_mount;
namespace two=gears_hand_two_bone;namespace vr=gears_vr;
enum class Status {okay,alias,source,reference,body,untracked,numerical,unreachable,residual};
struct Result {Status status{Status::okay};unsigned solved_mask{},projected_mask{};};
struct Workspace {
    body::Workspace body_work{};two::Input arm[2]{};two::Solution solved[2]{};
    mount::Mount mounts[2]{};body::Pose target[2]{};
    gears_hand_fingers::PoseBlend blend[2]{};
    std::array<vr::Quaternion,15> authored[2]{};
    std::array<std::uint64_t,10> context{};
};
// The existing body consumer certifies root/head/feet/shoulders, not wrists
// or fingers. Its separately validated Local/SpaceBases readback must ALSO
// match this hand plan. A body-only receipt cannot qualify mounted hands.
inline bool local_pose_matches(body::Output const& expected,std::array<body::Atom,body::bones> const& actual) noexcept {
    for(unsigned i=0;i<body::bones;++i){auto const& a=expected.local[i];auto const& b=actual[i];
        for(unsigned n=0;n<3;++n)if(!std::isfinite(a.translation[n])||!std::isfinite(b.translation[n]))return false;
        if(!body::d::unit_animation_scale(a.scale)||!body::d::unit_animation_scale(b.scale)||
           std::memcmp(a.translation,b.translation,12)||std::memcmp(&a.scale,&b.scale,4))return false;
        const double angle=body::d::angle({a.q[0],a.q[1],a.q[2],a.q[3]},{b.q[0],b.q[1],b.q[2],b.q[3]});
        if(!std::isfinite(angle)||angle>1e-4)return false;
    }
    return true;
}
// Troika's two original cylindrical shafts have their own thumb opposition
// and full four-finger wrap. Only the right index follows R2. These are initial
// authored skin angles, not measured fit or an input/mechanical permission.
inline bool shaft_pose(body::Model const& model,unsigned side,double trigger,
                       std::array<vr::Quaternion,15>& out) noexcept {
    if(side>1||!std::isfinite(trigger)||trigger<0||trigger>1)return false;
    constexpr double degrees=.017453292519943295;
    constexpr double curl[4][3]={{55,80,40},{62,85,42},{65,85,40},{70,80,38}};
    const unsigned first=side?36u:12u;
    for(unsigned n=0;n<15;++n){
        auto const& q=model.reference[first+n].q;vr::Quaternion value{q[0],q[1],q[2],q[3]};
        if(n<3){
            const double opposition=(side?-1.:1.)*(n?12.:34.)*degrees;
            value=gears_hand_fingers::product(value,gears_hand_fingers::turn(1,opposition));
            value=gears_hand_fingers::product(value,gears_hand_fingers::turn(2,-(n==0?24.:n==1?48.:32.)*degrees));
        }else{
            const unsigned finger=(n-3)/3,joint=(n-3)%3;
            const double index=side&&finger==0?trigger*(joint==0?12.:joint==1?10.:4.):0.;
            value=gears_hand_fingers::product(value,gears_hand_fingers::turn(2,-(curl[finger][joint]+index)*degrees));
        }
        if(!vr::stereo_pose_valid({{},value}))return false;out[n]=value;
    }
    return true;
}
inline Result compose(body::Model const& model,body::Frame const& frame,
    source::Snapshot const& current,std::int64_t now,
    std::array<std::uint64_t,2> const& available,Workspace& work,body::Output& output) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&model,sizeof model),range(&frame,sizeof frame),range(&current,sizeof current),
        range(&available,sizeof available),range(&work,sizeof work),range(&output,sizeof output)};
    if(!disjoint(ranges,6))return {Status::alias};
    if(!source::valid(current,now)||current.mapping.units_per_metre!=frame.units_per_metre)return {Status::source};
    const auto tracked=current.mapping.hands.tracked_mask;if(!tracked)return {Status::untracked};
    auto& w=work.body_work;
    if(frame.count!=body::bones||model.signature!=mount::marcus115_signature||
       !mount::canonical_geometry(frame.reference.data(),body::bones,w.canonical)||w.canonical!=model.canonical)
        return {Status::reference};
    for(unsigned i=0;i<body::bones;++i){std::int32_t parent{};
        std::memcpy(&parent,w.canonical.data()+i*36+4,4);
        if(model.parents[i]!=parent||std::memcmp(model.reference[i].q,w.canonical.data()+i*36+8,16)||
           std::memcmp(model.reference[i].translation,w.canonical.data()+i*36+24,12)||model.reference[i].scale!=1)
            return {Status::reference};
        if(!unit_animation_scale(frame.local[i].scale))return {Status::body};
    }
    for(unsigned i=0;i<16;++i)w.component.m[i]=frame.component_to_world.values[i];
    if(!similarity(w.component)||std::abs(length(row(w.component,0))-1)>2e-5||
       validate(frame.local.data(),model.parents.data(),body::bones,w.component)!=body::ik::Status::okay)return {Status::body};
    std::copy(frame.local.begin(),frame.local.end(),w.pose.local);
    if(!rebuild(model,w))return {Status::numerical};
    std::copy_n(w.pose.world,body::bones,w.original_world.data());
    const auto& f=current.source.frame;
    const std::array<std::uint64_t,10> key{{f.native.native_started_qpc,current.source.entry_binding,current.source.entry_token,
        f.world_generation,f.controllers.session,f.controllers.input_epoch,f.controllers.profile_generation,
        f.driver.address,f.driver_mesh.address,f.driver_mesh.name_number}};
    if(work.context!=key){work.context=key;work.blend[0]={};work.blend[1]={};}
    Result result{};
    for(unsigned side=0;side<2;++side){if(!(tracked&(1u<<side)))continue;
        auto& authored=work.mounts[side];
        if(!mount::marcus115(model.signature,side?mount::Side::right:mount::Side::left,
                frame.model_scale,frame.units_per_metre,authored)||
           gears_hand_wrist::native_wrist_target(current.mapping.hands.world[side],authored.calibration,
                frame.units_per_metre,work.target[side])!=gears_hand_wrist::Status::okay)return {Status::reference};
        const auto& chain=authored.chain;const unsigned upper_helper=side?33u:9u,fore_helper=side?53u:29u;
        if(model.parents[upper_helper]!=int(chain[0])||model.parents[fore_helper]!=int(chain[1]))return {Status::reference};
        std::memcpy(w.pose.local[upper_helper].q,model.reference[upper_helper].q,16);
        std::memcpy(w.pose.local[fore_helper].q,model.reference[fore_helper].q,16);
        if(!rebuild(model,w))return {Status::numerical};
        auto& arm=work.arm[side];arm={};
        arm.shoulder=pose(w.pose.world[chain[0]]);arm.elbow=pose(w.pose.world[chain[1]]);arm.wrist=pose(w.pose.world[chain[2]]);
        const auto& neutral=model.reference[chain[2]].q;
        arm.wrist.orientation=vr::normalized(vr::multiply(arm.elbow.orientation,{neutral[0],neutral[1],neutral[2],neutral[3]}));
        arm.target=work.target[side];arm.lengths.mode=two::LengthMode::preserve;arm.share_forearm_twist=true;
        auto outward=sub(position(w.pose.world[body::shoulders[side]]),position(w.pose.world[body::shoulders[1-side]]));
        auto up=row(w.component,2);const double width=length(outward),height=length(up);
        if(width<1e-6||height<1e-6)return {Status::numerical};
        arm.authored={true,add(arm.shoulder.position,sub(mul(outward,.35*frame.units_per_metre/width),mul(up,.6*frame.units_per_metre/height)))};
        const double upper=length(sub(arm.elbow.position,arm.shoulder.position)),lower=length(sub(arm.wrist.position,arm.elbow.position));
        const double minimum=std::abs(upper-lower)+.0025*frame.units_per_metre,maximum=upper+lower-.0025*frame.units_per_metre;
        auto delta=sub(arm.target.position,arm.shoulder.position);const double reach=length(delta);
        if(!std::isfinite(reach)||reach<1e-8||maximum<=minimum)return {Status::unreachable};
        const double bounded=(std::clamp)(reach,minimum,maximum);
        if(bounded!=reach){arm.target.position=add(arm.shoulder.position,mul(delta,bounded/reach));result.projected_mask|=1u<<side;}
        if(two::solve(arm,work.solved[side])!=two::Status::solved)return {Status::unreachable};
        const body::Pose solved[]={work.solved[side].shoulder,work.solved[side].elbow,work.solved[side].wrist};
        for(unsigned n=0;n<3;++n)if(!world_rotation(model,w,chain[n],solved[n].orientation))return {Status::numerical};
        if(!hands::detail::pose_matches(w.pose.world[chain[2]],arm.target))return {Status::residual};
        const bool held=side?bool(current.control.right_owned):current.control.lease.kind==gears_weapon_hand::LeaseKind::support;
        const auto& controller=f.controllers.hands[side];
        const double squeeze=controller.squeeze_active?double(controller.squeeze):0.;
        const double trigger=controller.trigger_active?double(controller.trigger):0.;
        if(!shaft_pose(model,side,trigger,work.authored[side])||!work.blend[side].begin(now)||
           !hands::detail::fingers(model,w,side,held?1.:squeeze,available,
                held?gears_hand_fingers::Shape::authored:gears_hand_fingers::Shape::free,1.,trigger,&work.blend[side],
                held?work.authored[side].data():nullptr))return {Status::numerical};
        result.solved_mask|=1u<<side;
    }
    // Output travels through the existing byte-exact transaction. Initialize
    // padding too; aggregate assignment leaves Report's alignment gap unspecified.
    static_assert(std::is_trivially_copyable<body::Output>::value);
    std::memset(&w.candidate,0,sizeof w.candidate);std::copy_n(w.pose.local,body::bones,w.candidate.local.begin());
    double length_error{};
    for(unsigned i=0;i<body::bones;++i){
        if(std::memcmp(frame.local[i].translation,w.pose.local[i].translation,12)||
           std::memcmp(&frame.local[i].scale,&w.pose.local[i].scale,4))return {Status::residual};
        if(std::memcmp(&frame.local[i],&w.pose.local[i],sizeof(body::Atom)))w.candidate.changed_mask[i/64]|=std::uint64_t(1)<<(i%64);
        if(i){const unsigned parent=unsigned(model.parents[i]);length_error=(std::max)(length_error,std::abs(
            length(sub(position(w.pose.world[i]),position(w.pose.world[parent])))-
            length(sub(position(w.original_world[i]),position(w.original_world[parent])))));}
    }
    // Original seated body and all bones outside the two arm skin subtrees stay exact.
    for(unsigned i=0;i<body::bones;++i){const bool left=(i>=8&&i<=29),right=(i>=32&&i<=53);
        if((!left&&!right)||((left&&!(tracked&1u))||(right&&!(tracked&2u))))
            if(std::memcmp(&frame.local[i],&w.pose.local[i],sizeof(body::Atom)))return {Status::residual};}
    if(!std::isfinite(length_error)||length_error>.002)return {Status::residual};
    constexpr unsigned stable[]={body::root,body::pelvis,body::head,body::legs[0][2],body::legs[1][2]};
    for(unsigned i:stable)if(std::memcmp(&w.original_world[i],&w.pose.world[i],sizeof(body::ik::Matrix)))return {Status::residual};
    w.candidate.root_world=pose(w.pose.world[body::root]);w.candidate.pelvis_world=pose(w.pose.world[body::pelvis]);
    w.candidate.head_world=pose(w.pose.world[body::head]);
    for(unsigned s=0;s<2;++s){w.candidate.feet_world[s]=pose(w.pose.world[body::legs[s][2]]);w.candidate.shoulders_world[s]=pose(w.pose.world[body::shoulders[s]]);}
    w.candidate.report.status=body::Status::solved;w.candidate.report.max_length_error=length_error;
    std::memcpy(&output,&w.candidate,sizeof output);return result;
}
} // namespace gears_troika_c0_hands
