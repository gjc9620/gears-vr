#pragma once
#include "cover_skeleton_ik.h"
#include "cover_passive_atoms.h"
#include "../vr-common/hand_two_bone_pose.h"
#include "../vr-common/hand_authored_mount_profile.h"
#include <array>

// Pure owned-buffer planner. No addresses, engine calls, allocation, callbacks
// during solve, stateful tracking integration, or Native write authorization.
// Only Marcus115/unit model scale; fixed foot anchors and one stationary slot.
namespace gears_full_body_c0 {
namespace ik=gears_cover_skeleton_ik;
namespace two=gears_hand_two_bone;
namespace rig=gears_hand_authored_mount;
using Vec=ik::Vec;using Quat=ik::Quat;using Pose=ik::Pose;using Atom=ik::Atom;
constexpr unsigned bones=115;
constexpr double max_body_yaw_delta=3.141592653589793238462643383279502884;
// Verified names/parents: fixture-provenance.json, complete canonical digest.
constexpr unsigned root=0,pelvis=1,head=55;
constexpr std::array<unsigned,5> spine{{3,4,5,54,55}};
constexpr unsigned legs[2][3]={{91,92,93},{95,96,97}};
constexpr unsigned shoulders[2]={8,32};
constexpr std::array<unsigned,9> neutral_rotation_bones{{1,3,4,5,6,7,30,31,54}};
struct Matrix64 {float values[16]{};};
struct Model {
    rig::Signature signature{};rig::CanonicalGeometry canonical{};
    std::array<std::int32_t,bones> parents{};std::array<Atom,bones> reference{};
};
struct Frame {
    std::uint32_t count{bones};double model_scale{1},units_per_metre{100};
    std::array<rig::ReferenceRecord68,bones> reference{};
    std::array<Atom,bones> local{};Matrix64 component_to_world{};
};
enum class Scope {stationary,fixed_cover};
enum class Posture {preserve_c0,reference_neutral};
enum class BodyTargetMode {source_relative,world_absolute};
struct RootWorldOrientation {Quat value{0,0,0,1};};
struct WorldAbsoluteBody {Vec neutral_pelvis_world{};RootWorldOrientation root_orientation{};};
struct Goal {
    Scope scope{Scope::stationary};Posture posture{Posture::preserve_c0};
    // Actual physical delta and the ALREADY committed persistent Grip offset,
    // both in the same UE world units. Neither is accumulated by this planner.
    Vec physical_world{},committed_grip_world{};
    // Explicit shortest signed heading in [-pi,+pi], supplied by body policy
    // (wall heading in Cover), not HMD yaw. Feet remain the supplied anchors;
    // an entering/turning policy must explicitly provide its new foot placement.
    double body_yaw_delta{};
    // Absolute head BONE pose. Eye/HMD marker conversion is explicitly upstream;
    // no animated wrist/head auto-calibration or eye==bone assumption is made.
    Pose head_world{},feet_world[2]{};
    // Opt-in stable actor/Cover reference supplied upstream, never inferred from
    // current animation, camera, or prior Output. Physical/Grip are added once.
    BodyTargetMode target_mode{BodyTargetMode::source_relative};
    WorldAbsoluteBody absolute{};
};
enum class Status {solved,alias,count,reference,model_scale,atom,component,goal,policy,
    feet_incompatible,leg_unreachable,spine_unreachable,spine_iterations,numerical,residual,pelvis_iterations};
struct Report {
    Status status{Status::solved};unsigned failed_bone{},spine_iterations{};
    double requested_distance{},available_reach{},head_error{},foot_error[2]{},
        foot_angle_error[2]{},head_angle_error{},max_length_error{},pelvis_projection{};
};
struct Output {
    std::array<Atom,bones> local{};std::array<std::uint64_t,2> changed_mask{};
    Pose root_world{},pelvis_world{},head_world{},feet_world[2]{},shoulders_world[2]{};
    Report report{};
};
// Caller-owned heap/TLS storage; never place this on a live game stack.
// Workspace may change after failure; Output is byte-identical on every failure.
struct Workspace {
    ik::Workspace pose{};std::array<ik::Matrix,bones> original_world{};
    rig::CanonicalGeometry canonical{};ik::Matrix component{};
    std::array<Vec,5> chain{};std::array<double,4> lengths{};
    two::Input leg_input{};two::Solution leg_solution{};Output candidate{};
};
static_assert(sizeof(Atom)==32&&sizeof(Matrix64)==64);
static_assert(std::is_trivially_copyable<Frame>::value&&std::is_trivially_copyable<Workspace>::value);
namespace d {
using namespace ik::detail;
// Retail animation blending has produced 0x3f7ffffe (1 - FLT_EPSILON).
// Use one symmetric float epsilon around unity: two representable steps below
// 1 and one above. Preserve the exact source bytes; this is not model scaling,
// bone stretching, or permission to relax the final geometric residuals.
inline bool unit_animation_scale(float scale) noexcept {
    constexpr float epsilon=0x1p-23f;
    return scale>=1.f-epsilon&&scale<=1.f+epsilon;
}
inline Vec add(Vec a,Vec b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Quat inverse(Quat q) noexcept {return {-q.x,-q.y,-q.z,q.w};}
inline Vec rotate(Quat q,Vec v) noexcept {return two::detail::rotated(q,v);}
inline Quat yaw(double a) noexcept {return {0,0,std::sin(a*.5),std::cos(a*.5)};}
inline bool same_position(Vec a,Vec b,double e=1e-8) noexcept {return length(sub(a,b))<=e;}
inline double angle(Quat a,Quat b) noexcept {
    Quat x{},y{};if(!normalized(a,x)||!normalized(b,y))return 1e100;
    return 2*std::acos((std::clamp)(std::abs(x.x*y.x+x.y*y.y+x.z*y.z+x.w*y.w),0.,1.));
}
inline bool same_rotation(Quat a,Quat b) noexcept {
    Quat x{},y{};if(!normalized(a,x)||!normalized(b,y))return false;
    const double sign=x.x*y.x+x.y*y.y+x.z*y.z+x.w*y.w<0?-1.:1.;
    return std::abs(x.x-sign*y.x)+std::abs(x.y-sign*y.y)+std::abs(x.z-sign*y.z)+std::abs(x.w-sign*y.w)<1e-10;
}
inline Pose pose(ik::Matrix const& m) noexcept {return {position(m),rotation(m)};}
inline bool rebuild(Model const& m,Workspace& w) noexcept {return build(w.pose,m.parents.data(),bones,w.component);}
inline bool world_rotation(Model const& m,Workspace& w,unsigned i,Quat q) noexcept {
    const auto parent=i?w.pose.world[unsigned(m.parents[i])]:w.component;
    return set_quat(w.pose.local[i],product(d::inverse(rotation(parent)),q))&&rebuild(m,w);
}
inline bool world_root_position(Model const& m,Workspace& w,Vec p) noexcept {
    const auto a=row(w.component,0),b=row(w.component,1),c=row(w.component,2);
    const auto bc=cross(b,c),ca=cross(c,a),ab=cross(a,b);const double det=dot(a,bc);
    const auto v=sub(p,position(w.component));const double xyz[]={dot(v,bc)/det,dot(v,ca)/det,dot(v,ab)/det};
    for(unsigned j=0;j<3;++j){w.pose.local[0].translation[j]=float(xyz[j]);if(!std::isfinite(w.pose.local[0].translation[j]))return false;}
    return rebuild(m,w);
}
inline bool goal_pose(Pose const& p) noexcept {Quat q{};return finite_vector(p.position)&&normalized(p.orientation,q);}
inline Report fail(Status s,unsigned bone=0,double distance=0,double reach=0) noexcept {Report r{};r.status=s;r.failed_bone=bone;r.requested_distance=distance;r.available_reach=reach;return r;}
}

// Hash callback is used only for explicit model construction. It receives the
// owned canonical bytes, must calculate SHA256, and returns its real digest.
// Current Frame reference is compared against every canonical byte each solve.
template<class Sha256> inline bool prepare_model(std::array<rig::ReferenceRecord68,bones> const& reference,
    Sha256&& digest,Model& out) {
    const d::Range ranges[]={d::range(&reference,sizeof reference),d::range(&out,sizeof out)};
    if(!d::disjoint(ranges,2))return false;
    Model candidate{};
    if(!rig::canonical_geometry(reference.data(),bones,candidate.canonical)||
       !digest(candidate.canonical.data(),candidate.canonical.size(),candidate.signature)||candidate.signature!=rig::marcus115_signature)return false;
    for(unsigned i=0;i<bones;++i){
        std::memcpy(&candidate.parents[i],reference[i].bytes+56,4);
        std::memcpy(candidate.reference[i].q,reference[i].bytes+12,16);
        std::memcpy(candidate.reference[i].translation,reference[i].bytes+28,12);candidate.reference[i].scale=1;
    }
    out=candidate;return true;
}

// Optional boundary conversion. Axes must already be UE F/R/U world directions.
// It adds physical and committed Grip exactly once and leaves out untouched on
// failure. Releasing Grip is represented by retaining the committed input value.
inline bool motion_world(Vec physical_metres,Vec committed_metres,Vec forward,Vec right,Vec up,
    double units_per_metre,Vec& out) noexcept {
    if(!d::finite_vector(physical_metres)||!d::finite_vector(committed_metres)||!d::finite_vector(forward)||!d::finite_vector(right)||!d::finite_vector(up)||
       !std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500)return false;
    if(std::abs(d::length(forward)-1)>1e-6||std::abs(d::length(right)-1)>1e-6||std::abs(d::length(up)-1)>1e-6||
       std::abs(d::dot(forward,right))>1e-6||std::abs(d::dot(forward,up))>1e-6||std::abs(d::dot(right,up))>1e-6||d::dot(d::cross(forward,right),up)<.999999)return false;
    const auto v=d::add(physical_metres,committed_metres);
    const auto result=d::mul(d::add(d::add(d::mul(forward,v.x),d::mul(right,v.y)),d::mul(up,v.z)),units_per_metre);
    if(!d::finite_vector(result))return false;out=result;return true;
}

namespace d {
inline Report plan_impl(Model const& model,Frame const& frame,Goal const& goal,Workspace& work,Output& output,
    bool cover_pelvis_reach) noexcept {
    using namespace d;
    const Range ranges[]={range(&model,sizeof model),range(&frame,sizeof frame),range(&goal,sizeof goal),range(&work,sizeof work),range(&output,sizeof output)};
    if(!disjoint(ranges,5))return fail(Status::alias);
    if(frame.count!=bones)return fail(Status::count);
    if(frame.model_scale!=1||!std::isfinite(frame.units_per_metre)||frame.units_per_metre<10||frame.units_per_metre>500)return fail(Status::model_scale);
    if(model.signature!=rig::marcus115_signature||!rig::canonical_geometry(frame.reference.data(),bones,work.canonical)||work.canonical!=model.canonical)return fail(Status::reference);
    for(unsigned i=0;i<bones;++i){std::int32_t parent{};std::memcpy(&parent,work.canonical.data()+i*36+4,4);
        if(model.parents[i]!=parent||std::memcmp(model.reference[i].q,work.canonical.data()+i*36+8,16)||
           std::memcmp(model.reference[i].translation,work.canonical.data()+i*36+24,12)||model.reference[i].scale!=1)return fail(Status::reference,i);}
    if((goal.scope!=Scope::stationary&&goal.scope!=Scope::fixed_cover)||(goal.posture!=Posture::preserve_c0&&goal.posture!=Posture::reference_neutral))return fail(Status::policy);
    if(!finite_vector(goal.physical_world)||!finite_vector(goal.committed_grip_world)||!std::isfinite(goal.body_yaw_delta)||
       std::abs(goal.body_yaw_delta)>max_body_yaw_delta||!goal_pose(goal.head_world)||!goal_pose(goal.feet_world[0])||!goal_pose(goal.feet_world[1]))return fail(Status::goal);
    if(goal.target_mode!=BodyTargetMode::source_relative&&goal.target_mode!=BodyTargetMode::world_absolute)return fail(Status::policy);
    const bool absolute=goal.target_mode==BodyTargetMode::world_absolute;Quat absolute_root{};
    if(cover_pelvis_reach&&(!absolute||goal.scope!=Scope::fixed_cover))return fail(Status::policy);
    if(absolute&&(goal.body_yaw_delta!=0||!finite_vector(goal.absolute.neutral_pelvis_world)||
       !normalized(goal.absolute.root_orientation.value,absolute_root)))return fail(Status::goal);
    for(unsigned j=0;j<16;++j)work.component.m[j]=frame.component_to_world.values[j];
    if(!similarity(work.component)||std::abs(length(row(work.component,0))-1)>2e-5)return fail(Status::component);
    const auto checked=gears_cover_passive_atoms::validate(frame.local.data(),model.parents.data(),bones,work.component,goal.scope==Scope::fixed_cover);
    if(checked!=ik::Status::okay)return fail(Status::atom);
    for(unsigned i=0;i<bones;++i)if(!unit_animation_scale(frame.local[i].scale))return fail(Status::model_scale,i);
    std::memcpy(work.pose.local,frame.local.data(),sizeof(frame.local));if(!rebuild(model,work))return fail(Status::numerical);
    std::copy_n(work.pose.world,bones,work.original_world.data());
    const Vec base_pelvis=position(work.pose.world[pelvis]);Quat heading=yaw(goal.body_yaw_delta);
    if(absolute){const auto current_root=rotation(work.pose.world[root]);
        // This delta only rotates the current knee-pole vector. It is not a
        // new accumulated heading or physical displacement. Root target is fixed.
        heading=product(absolute_root,d::inverse(current_root));
        if(!same_rotation(current_root,absolute_root)&&!world_rotation(model,work,root,absolute_root))return fail(Status::numerical);
    }else if(goal.body_yaw_delta!=0&&!world_rotation(model,work,root,product(heading,rotation(work.pose.world[root]))))return fail(Status::numerical);
    if(goal.posture==Posture::reference_neutral){
        // Static Ref rotations only. No replacement of animated translation,
        // segment length, scale, foot anchor, head goal or hand mount.
        // The two shoulder-deformation branches7/31 are siblings of the arm
        // roots, not part of the arm IK chain. Leaving their aiming-animation
        // rotations behind keeps deforming collar skin in a neutral VR body.
        for(unsigned i:neutral_rotation_bones)std::memcpy(work.pose.local[i].q,model.reference[i].q,16);
        if(!rebuild(model,work))return fail(Status::numerical);
    }
    const Vec desired_pelvis=add(absolute?goal.absolute.neutral_pelvis_world:base_pelvis,add(goal.physical_world,goal.committed_grip_world));
    Vec centres[3]{},correction[3]{},point=desired_pelvis;double radii[3]{};
    for(unsigned side=0;side<2;++side){
        const auto hip=position(work.pose.world[legs[side][0]]),knee=position(work.pose.world[legs[side][1]]),foot=position(work.pose.world[legs[side][2]]);
        centres[side]=sub(goal.feet_world[side].position,sub(hip,position(work.pose.world[pelvis])));
        radii[side]=length(sub(knee,hip))+length(sub(foot,knee));
        if(!std::isfinite(radii[side])||radii[side]<1e-6)return fail(Status::atom,legs[side][0]);
    }
    if(length(sub(centres[0],centres[1]))>radii[0]+radii[1])return fail(Status::feet_incompatible);
    const unsigned constraints=cover_pelvis_reach?3u:2u;
    if(cover_pelvis_reach){
        // Current pose solve, never a recalibration of the frozen neutral.
        // Root/posture are already selected. Moving the pelvis translates
        // the spine anchor by the same vector, so the exact head target adds
        // one reach ball to the two fixed-foot balls. Head/feet, orientation,
        // segment lengths, physical/Grip inputs and Goal all remain unchanged.
        centres[2]=sub(goal.head_world.position,sub(position(work.pose.world[spine[0]]),position(work.pose.world[pelvis])));
        for(unsigned i=0;i<4;++i)radii[2]+=length(sub(position(work.pose.world[spine[i+1]]),position(work.pose.world[spine[i]])));
        if(!std::isfinite(radii[2])||radii[2]<1e-6)return fail(Status::atom,head);
    }
    // Bounded pelvis projection onto the two fixed-foot maximum-reach balls.
    // Only when actual projection is needed, reserve .001 native unit inside
    // reach for float Atom root encoding; this never changes a segment length
    // or head/foot target. Already feasible C0 pelvis remains exactly in place.
    // Ordinary plan retains its head-independent pelvis policy. The explicit
    // cover solve additionally fits the current pelvis to the exact head;
    // its final body path still needs external collision/write certification.
    bool needs_projection=false;
    for(unsigned i=0;i<constraints;++i)if(length(sub(point,centres[i]))>radii[i])needs_projection=true;
    const double project_radii[]={radii[0]-(needs_projection?.001:0),radii[1]-(needs_projection?.001:0),radii[2]-(needs_projection?.001:0)};
    for(unsigned iteration=0;iteration<128;++iteration){
        const Vec prior=point;
        for(unsigned side=0;side<constraints;++side){const auto x=add(point,correction[side]);const auto v=sub(x,centres[side]);const double n=length(v);
            const auto projected=n>project_radii[side]?add(centres[side],mul(v,project_radii[side]/n)):x;correction[side]=sub(x,projected);point=projected;}
        if(length(sub(point,prior))<1e-9)break;
    }
    for(unsigned side=0;side<constraints;++side)if(length(sub(point,centres[side]))>radii[side]+1e-5)return fail(Status::pelvis_iterations,side<2?legs[side][0]:head,length(sub(point,centres[side])),radii[side]);
    const auto root_position=add(position(work.pose.world[root]),sub(point,position(work.pose.world[pelvis])));
    if(!same_position(root_position,position(work.pose.world[root]))&&!world_root_position(model,work,root_position))return fail(Status::numerical);
    Report report{};report.pelvis_projection=length(sub(point,desired_pelvis));
    for(unsigned side=0;side<2;++side){
        const auto hip=legs[side][0],knee=legs[side][1],foot=legs[side][2];
        if(same_position(position(work.pose.world[foot]),goal.feet_world[side].position)&&same_rotation(rotation(work.pose.world[foot]),goal.feet_world[side].orientation))continue;
        // Blended float quaternions can be near-unit rather than exact-unit.
        // Encoding solved rotations changes that roundoff. Refine from the
        // encoded FK, keeping the original foot goal, translations and scales;
        // never increase the final fixed-length/foot-error acceptance limits.
        for(unsigned refinement=0;refinement<3;++refinement){
        work.leg_input={};work.leg_input.shoulder=pose(work.pose.world[hip]);work.leg_input.elbow=pose(work.pose.world[knee]);work.leg_input.wrist=pose(work.pose.world[foot]);work.leg_input.target=goal.feet_world[side];
        work.leg_input.authored={true,add(point,rotate(heading,sub(position(work.original_world[knee]),base_pelvis)))};
        const auto status=two::solve(work.leg_input,work.leg_solution);
        if(status!=two::Status::solved)return fail(status==two::Status::unreachable?Status::leg_unreachable:Status::numerical,hip,length(sub(goal.feet_world[side].position,work.leg_input.shoulder.position)),radii[side]);
        const Pose solved[]={work.leg_solution.shoulder,work.leg_solution.elbow,work.leg_solution.wrist};
        for(unsigned j=0;j<3;++j)if(!world_rotation(model,work,legs[side][j],solved[j].orientation))return fail(Status::numerical,legs[side][j]);
        if(length(sub(position(work.pose.world[foot]),goal.feet_world[side].position))<=.0005&&
           angle(rotation(work.pose.world[foot]),goal.feet_world[side].orientation)<=1e-5)break;
        }
    }
    for(unsigned i=0;i<5;++i)work.chain[i]=position(work.pose.world[spine[i]]);
    double total{},longest{};
    for(unsigned i=0;i<4;++i){const double n=length(sub(work.chain[i+1],work.chain[i]));if(n<1e-6)return fail(Status::atom,spine[i+1]);work.lengths[i]=n;total+=n;longest=(std::max)(longest,n);}
    const Vec anchor=work.chain[0];const double reach=length(sub(goal.head_world.position,anchor));
    const Vec spine_pole=sub(work.chain[2],anchor);
    if(reach>total+1e-7||reach<(std::max)(0.,2*longest-total)-1e-7)return fail(Status::spine_unreachable,head,reach,total);
    if(!same_position(work.chain[4],goal.head_world.position,1e-6)){
        // At maximum reach the only solution is a straight chain. FABRIK
        // approaches that limit asymptotically and can exhaust its iteration
        // budget even though this exact fixed-length solution is available.
        if(reach>=total-1e-7){
            const auto direction=mul(sub(goal.head_world.position,anchor),1./reach);
            for(unsigned i=0;i<4;++i)work.chain[i+1]=add(work.chain[i],mul(direction,work.lengths[i]));
        }else for(unsigned iteration=0;iteration<256;++iteration){
            work.chain[4]=goal.head_world.position;
            for(int i=3;i>=0;--i){Vec u{};if(!two::detail::unit(sub(work.chain[unsigned(i)],work.chain[unsigned(i)+1]),u))return fail(Status::numerical,spine[unsigned(i)]);work.chain[unsigned(i)]=add(work.chain[unsigned(i)+1],mul(u,work.lengths[unsigned(i)]));}
            work.chain[0]=anchor;
            for(unsigned i=0;i<4;++i){Vec u{};if(!two::detail::unit(sub(work.chain[i+1],work.chain[i]),u))return fail(Status::numerical,spine[i]);work.chain[i+1]=add(work.chain[i],mul(u,work.lengths[i]));}
            report.spine_iterations=iteration+1;if(length(sub(work.chain[4],goal.head_world.position))<1e-5)break;
        }
        if(length(sub(work.chain[4],goal.head_world.position))>.001&&reach>total*.995){
            // Near full extension FABRIK converges asymptotically even for a
            // reachable head. Falling back to the animated native torso then
            // makes the head/shoulders jump while the wrists remain tracked.
            // Solve a bounded analytic two-link chain instead, splitting each
            // virtual link into its two ORIGINAL fixed-length spine segments.
            // The bend plane comes from this invocation's neutral spine (or
            // its authored frame at exact collinearity), never a prior pose.
            const double upper=work.lengths[0]+work.lengths[1],lower=work.lengths[2]+work.lengths[3];
            Vec axis{},radial{};
            if(reach>std::abs(upper-lower)&&two::detail::unit(sub(goal.head_world.position,anchor),axis)&&
               (two::detail::perpendicular(spine_pole,axis,radial)||
                two::detail::frame_perpendicular(rotation(work.pose.world[spine[0]]),axis,radial))){
                const double cosine=(std::clamp)((reach*reach+upper*upper-lower*lower)/(2*reach*upper),-1.,1.);
                const Vec elbow=add(anchor,add(mul(axis,upper*cosine),mul(radial,upper*std::sqrt((std::max)(0.,1-cosine*cosine)))));
                work.chain[1]=add(anchor,mul(sub(elbow,anchor),work.lengths[0]/upper));
                work.chain[2]=elbow;
                work.chain[3]=add(elbow,mul(sub(goal.head_world.position,elbow),work.lengths[2]/lower));
                work.chain[4]=goal.head_world.position;
            }
        }
        if(length(sub(work.chain[4],goal.head_world.position))>.001){auto failure=fail(Status::spine_iterations,head,length(sub(work.chain[4],goal.head_world.position)),total);failure.spine_iterations=report.spine_iterations;return failure;}
        for(unsigned i=0;i<4;++i){const auto b=spine[i],next=spine[i+1];Quat swing{};
            const auto old_direction=sub(position(work.pose.world[next]),position(work.pose.world[b]));const auto wanted=sub(work.chain[i+1],work.chain[i]);
            if(!two::detail::swing(old_direction,wanted,{0,0,1},rotation(work.pose.world[b]),swing)||!world_rotation(model,work,b,product(swing,rotation(work.pose.world[b]))))return fail(Status::numerical,b);}
    }
    if(!same_rotation(rotation(work.pose.world[head]),goal.head_world.orientation)&&!world_rotation(model,work,head,goal.head_world.orientation))return fail(Status::numerical,head);
    report.head_error=length(sub(position(work.pose.world[head]),goal.head_world.position));report.head_angle_error=angle(rotation(work.pose.world[head]),goal.head_world.orientation);
    for(unsigned side=0;side<2;++side){report.foot_error[side]=length(sub(position(work.pose.world[legs[side][2]]),goal.feet_world[side].position));report.foot_angle_error[side]=angle(rotation(work.pose.world[legs[side][2]]),goal.feet_world[side].orientation);}
    for(unsigned i=1;i<bones;++i){const auto p=unsigned(model.parents[i]);const double before=length(sub(position(work.original_world[i]),position(work.original_world[p]))),after=length(sub(position(work.pose.world[i]),position(work.pose.world[p])));
        report.max_length_error=(std::max)(report.max_length_error,std::abs(after-before));
        if(std::memcmp(work.pose.local[i].translation,frame.local[i].translation,12)||work.pose.local[i].scale!=frame.local[i].scale)return fail(Status::numerical,i);}
    if(report.head_error>.002||report.foot_error[0]>.002||report.foot_error[1]>.002||report.max_length_error>.002||report.head_angle_error>1e-4||report.foot_angle_error[0]>1e-4||report.foot_angle_error[1]>1e-4){report.status=Status::residual;return report;}
    if(goal.scope==Scope::fixed_cover)for(unsigned i=0;i<bones;++i){Quat q{};
        if(!normalized({frame.local[i].q[0],frame.local[i].q[1],frame.local[i].q[2],frame.local[i].q[3]},q)&&
           std::memcmp(&frame.local[i],&work.pose.local[i],sizeof(Atom)))return fail(Status::numerical,i);
    }
    work.candidate={};std::copy_n(work.pose.local,bones,work.candidate.local.begin());
    for(unsigned i=0;i<bones;++i)if(std::memcmp(&frame.local[i],&work.pose.local[i],sizeof(Atom)))work.candidate.changed_mask[i/64]|=std::uint64_t(1)<<(i%64);
    work.candidate.root_world=pose(work.pose.world[root]);work.candidate.pelvis_world=pose(work.pose.world[pelvis]);work.candidate.head_world=pose(work.pose.world[head]);
    for(unsigned side=0;side<2;++side){work.candidate.feet_world[side]=pose(work.pose.world[legs[side][2]]);work.candidate.shoulders_world[side]=pose(work.pose.world[shoulders[side]]);}
    work.candidate.report=report;output=work.candidate;return report;
}
} // namespace d
inline Report plan(Model const& model,Frame const& frame,Goal const& goal,Workspace& work,Output& output) noexcept {
    return d::plan_impl(model,frame,goal,work,output,false);
}
// Explicit exact-head cover solver. Adjusts only this invocation's actual
// pelvis/root, within fixed-foot and spine reach; never edits/re-fits Goal or
// neutral calibration and never projects the requested head. Failure leaves
// Output unchanged. Not a collision certificate or Native write permission.
inline Report plan_cover(Model const& model,Frame const& frame,Goal const& goal,Workspace& work,Output& output) noexcept {
    return d::plan_impl(model,frame,goal,work,output,true);
}
// Standing first-person fallback for an HMD goal beyond fixed spine reach.
// Solve the SAME neutral body/feet/root toward the nearest reachable head
// position, keeping the requested head orientation. Never reintroduce the
// animated weapon stance, stretch bones, or change the actual XR eye target.
// The caller must report projection separately from exact HMD/body success.
inline Report plan_with_spine_reach(Model const& model,Frame const& frame,Goal const& goal,
    Workspace& work,Output& output,double& head_projection) noexcept {
    head_projection=0;
    const auto original=plan(model,frame,goal,work,output);
    if(original.status!=Status::spine_unreachable||goal.scope!=Scope::stationary||
       goal.posture!=Posture::reference_neutral||goal.target_mode!=BodyTargetMode::world_absolute||
       original.requested_distance<=original.available_reach||original.available_reach<=0)return original;
    // These chain values are produced by the just-completed, validated plan,
    // never retained from a prior invocation or inferred from camera motion.
    auto reachable=goal;const auto anchor=work.chain[0];
    reachable.head_world.position=d::add(anchor,d::mul(d::sub(goal.head_world.position,anchor),
        original.available_reach/original.requested_distance));
    const auto result=plan(model,frame,reachable,work,output);
    if(result.status==Status::solved)head_projection=d::length(d::sub(reachable.head_world.position,goal.head_world.position));
    return result;
}
// Retain this invocation's original body when its spine cannot reach the HMD.
// Hands still use the actual eye/controller basis. This is a separately solved
// native-body goal, never a claim that the original HMD/body goal succeeded.
inline Report retain_native_body(Model const& model,Frame const& frame,Workspace& work,Output& output) noexcept {
    using namespace d;
    const Range ranges[]={range(&model,sizeof model),range(&frame,sizeof frame),range(&work,sizeof work),range(&output,sizeof output)};
    if(!disjoint(ranges,4))return fail(Status::alias);
    if(frame.count!=bones)return fail(Status::count);
    for(unsigned i=0;i<16;++i)work.component.m[i]=frame.component_to_world.values[i];
    if(validate(frame.local.data(),model.parents.data(),bones,work.component)!=ik::Status::okay)return fail(Status::atom);
    std::copy(frame.local.begin(),frame.local.end(),work.pose.local);
    if(!rebuild(model,work))return fail(Status::numerical);
    Goal native{};native.head_world=pose(work.pose.world[head]);
    for(unsigned side=0;side<2;++side)native.feet_world[side]=pose(work.pose.world[legs[side][2]]);
    return plan(model,frame,native,work,output);
}
} // namespace gears_full_body_c0
