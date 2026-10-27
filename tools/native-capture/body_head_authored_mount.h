#pragma once
#include "full_body_c0_planner.h"

// Pure authored geometry, not tracking calibration or Native permission.
// Model must come from body::prepare_model with the real canonical SHA256.
namespace gears_body_head_authored {
namespace body=gears_full_body_c0;
constexpr unsigned head_bone=55,left_eye_bone=57,right_eye_bone=60;
inline constexpr char profile_id[]="marcus115-ref-eyes-ue-actor-axes-authored-v1";
inline constexpr char marcus_face_profile_id[]="marcus115-ref-eyes-face-forward-y-authored-v2";
enum class Basis {unknown,designer_authored_ue_actor_axes,marcus_mesh_face_forward_y};
enum class Status {okay,alias,basis,model,count,scale,reference,numerical,pose,unprepared};
// Both are already UE-world/native-unit poses. No metres/OpenXR conversion,
// physical delta, Grip offset, actor heading, or camera write happens here.
struct EyeHmdWorld {body::Pose pose{};};
struct HeadBoneWorld {body::Pose pose{};};
struct Workspace {
    body::rig::CanonicalGeometry canonical{};
    std::array<body::ik::Matrix,body::bones> reference_space{};
};
class Marker;
inline Status prepare(body::Model const&,body::Frame const&,Basis,Workspace&,Marker&) noexcept;
inline Status head_from_eye(Marker const&,body::Model const&,body::Frame const&,EyeHmdWorld const&,HeadBoneWorld&) noexcept;
inline Status eye_from_head(Marker const&,body::Model const&,body::Frame const&,HeadBoneWorld const&,EyeHmdWorld&) noexcept;
class Marker {
    friend Status prepare(body::Model const&,body::Frame const&,Basis,Workspace&,Marker&) noexcept;
    friend Status head_from_eye(Marker const&,body::Model const&,body::Frame const&,EyeHmdWorld const&,HeadBoneWorld&) noexcept;
    friend Status eye_from_head(Marker const&,body::Model const&,body::Frame const&,HeadBoneWorld const&,EyeHmdWorld&) noexcept;
    Basis basis_{};body::rig::Signature signature_{};body::rig::CanonicalGeometry canonical_{};
    body::Pose head_to_eye_{},reference_head_{},reference_eye_{};
    Status matches(body::Model const&,body::Frame const&) const noexcept;
public:
    // Value copies preserve an authored profile. These values grant no access.
    Basis basis() const noexcept{return basis_;}
    body::Pose head_to_eye_local() const noexcept{return head_to_eye_;}
    body::Pose reference_head_model() const noexcept{return reference_head_;}
    body::Pose reference_eye_model() const noexcept{return reference_eye_;}
};
static_assert(sizeof(EyeHmdWorld)==sizeof(body::Pose)&&sizeof(HeadBoneWorld)==sizeof(body::Pose));
static_assert(std::is_trivially_copyable<Marker>::value&&std::is_trivially_copyable<Workspace>::value);
namespace detail {
inline Status context(body::Model const& m,body::Frame const& f,body::rig::CanonicalGeometry const& c) noexcept {
    if(f.count!=body::bones)return Status::count;
    if(f.model_scale!=1||!std::isfinite(f.units_per_metre)||f.units_per_metre<10||f.units_per_metre>500)return Status::scale;
    if(m.signature!=body::rig::marcus115_signature||m.canonical!=c)return Status::model;
    for(unsigned i=0;i<body::bones;++i){
        std::uint32_t index{};std::int32_t parent{};const auto* p=c.data()+i*36;
        std::memcpy(&index,p,4);std::memcpy(&parent,p+4,4);
        if(index!=i||(!i&&parent!=0)||(i&&(parent<0||unsigned(parent)>=i))||m.parents[i]!=parent||
           std::memcmp(f.reference[i].bytes+56,p+4,4)||std::memcmp(f.reference[i].bytes+12,p+8,28)||
           std::memcmp(m.reference[i].q,p+8,16)||std::memcmp(m.reference[i].translation,p+24,12)||m.reference[i].scale!=1)return Status::reference;
        body::Quat q{};const auto& a=m.reference[i];
        if(!body::d::normalized({a.q[0],a.q[1],a.q[2],a.q[3]},q)||
           !body::d::finite_vector({a.translation[0],a.translation[1],a.translation[2]}))return Status::reference;
    }
    // Both actual eye descendants pass through Face root56. Do not infer eyes
    // by number/count; the complete independently named Ref join pins57/60.
    for(unsigned eye:{left_eye_bone,right_eye_bone}){
        unsigned at=eye;for(unsigned n=0;n<body::bones&&at!=head_bone&&at;n++)at=unsigned(m.parents[at]);
        if(at!=head_bone)return Status::reference;
    }
    return Status::okay;
}
}
inline Status Marker::matches(body::Model const& m,body::Frame const& f) const noexcept {
    if(basis_!=Basis::designer_authored_ue_actor_axes&&basis_!=Basis::marcus_mesh_face_forward_y)return Status::unprepared;
    if(signature_!=m.signature)return Status::model;
    return detail::context(m,f,canonical_);
}
inline Status prepare(body::Model const& model,body::Frame const& frame,Basis basis,Workspace& work,Marker& output) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&model,sizeof model),range(&frame,sizeof frame),range(&work,sizeof work),range(&output,sizeof output)};
    if(!disjoint(ranges,4))return Status::alias;
    if(basis!=Basis::designer_authored_ue_actor_axes&&basis!=Basis::marcus_mesh_face_forward_y)return Status::basis;
    if(!body::rig::canonical_geometry(frame.reference.data(),frame.count,work.canonical))return Status::reference;
    const auto valid=detail::context(model,frame,work.canonical);if(valid!=Status::okay)return valid;
    for(unsigned i=0;i<body::bones;++i){
        const auto local=atom_matrix(model.reference[i]);
        work.reference_space[i]=i?multiply(local,work.reference_space[unsigned(model.parents[i])]):local;
        if(!similarity(work.reference_space[i]))return Status::numerical;
    }
    Marker candidate{};candidate.basis_=basis;candidate.signature_=model.signature;candidate.canonical_=work.canonical;
    candidate.reference_head_=pose(work.reference_space[head_bone]);
    candidate.reference_eye_.position=mul(add(position(work.reference_space[left_eye_bone]),position(work.reference_space[right_eye_bone])),.5);
    // Retain the old X-forward profile for frozen history. Marcus Ref115 has
    // left shoulder/eye on +X, right on -X, and both eyes ahead of Head on +Y:
    // Eye +X forward maps to model +Y, Eye +Y right maps to model -X.
    // This static asset basis is not a player IPD or animated calibration.
    candidate.reference_eye_.orientation=basis==Basis::marcus_mesh_face_forward_y?
        yaw(1.57079632679489661923):body::Quat{0,0,0,1};
    const auto inverse_head=body::d::inverse(candidate.reference_head_.orientation);
    candidate.head_to_eye_.position=rotate(inverse_head,sub(candidate.reference_eye_.position,candidate.reference_head_.position));
    candidate.head_to_eye_.orientation=basis==Basis::designer_authored_ue_actor_axes?inverse_head:
        product(inverse_head,candidate.reference_eye_.orientation);
    if(!goal_pose(candidate.head_to_eye_)||!goal_pose(candidate.reference_head_)||!goal_pose(candidate.reference_eye_))return Status::numerical;
    output=candidate;return Status::okay;
}
inline Status head_from_eye(Marker const& marker,body::Model const& model,body::Frame const& frame,EyeHmdWorld const& eye,HeadBoneWorld& output) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&marker,sizeof marker),range(&model,sizeof model),range(&frame,sizeof frame),range(&eye,sizeof eye),range(&output,sizeof output)};
    if(!disjoint(ranges,5))return Status::alias;
    const auto valid=marker.matches(model,frame);if(valid!=Status::okay)return valid;
    body::Quat q{};if(!finite_vector(eye.pose.position)||!normalized(eye.pose.orientation,q))return Status::pose;
    HeadBoneWorld candidate{};
    if(!normalized(product(q,body::d::inverse(marker.head_to_eye_.orientation)),candidate.pose.orientation))return Status::numerical;
    candidate.pose.position=sub(eye.pose.position,rotate(candidate.pose.orientation,marker.head_to_eye_.position));
    if(!goal_pose(candidate.pose))return Status::numerical;output=candidate;return Status::okay;
}
inline Status eye_from_head(Marker const& marker,body::Model const& model,body::Frame const& frame,HeadBoneWorld const& head,EyeHmdWorld& output) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&marker,sizeof marker),range(&model,sizeof model),range(&frame,sizeof frame),range(&head,sizeof head),range(&output,sizeof output)};
    if(!disjoint(ranges,5))return Status::alias;
    const auto valid=marker.matches(model,frame);if(valid!=Status::okay)return valid;
    body::Quat q{};if(!finite_vector(head.pose.position)||!normalized(head.pose.orientation,q))return Status::pose;
    EyeHmdWorld candidate{};
    if(!normalized(product(q,marker.head_to_eye_.orientation),candidate.pose.orientation))return Status::numerical;
    candidate.pose.position=add(head.pose.position,rotate(q,marker.head_to_eye_.position));
    if(!goal_pose(candidate.pose))return Status::numerical;output=candidate;return Status::okay;
}
} // namespace gears_body_head_authored
