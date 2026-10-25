#pragma once
#include <limits>
#include "cover_skeleton_ik.h"
#include "../vr-common/hand_two_bone_pose.h"

// Private caller-buffer math only. Matrix64 describes the float16 row shape,
// not a native address, role, lifetime, ownership, or write permission. This
// candidate accepts ONLY proper unit-scale rigid matrices; unknown LocalScale
// is neither inferred nor preserved. No IO/engine/PG call or allocation.
namespace gears_cover_control_output_pose {
namespace ik=gears_cover_skeleton_ik;
namespace arm=gears_hand_two_bone;
using Pose=gears_vr::Pose;
using Vector=gears_vr::Vector;
using Quaternion=gears_vr::Quaternion;
struct Matrix64 {float values[16]{};};
struct LimbInput {
    Matrix64 component[3]{},component_to_world{};
    Pose wrist_world{};
    arm::LengthMode mode{arm::LengthMode::preserve};
    arm::Pole authored{},previous{}; // World-space points, explicitly supplied.
};
struct SingleInput {Matrix64 component{},component_to_world{};Quaternion world_orientation{};};
enum class Status {solved,alias,invalid_matrix,invalid_target,invalid_policy,degenerate,unreachable,numerical_failure};
// Caller owns scratch; do not add hidden game-stack pose buffers. Scratch may
// change on failure, but all outputs remain byte-identical. Inputs, outputs,
// and this complete workspace must be disjoint (including partial overlap).
struct Workspace {
    ik::Matrix component[3]{},to_world{},world[3]{};
    Pose transform{},world_pose[3]{};
    arm::Input input{};arm::Solution solution{};
    Matrix64 candidate[3]{};
};
static_assert(sizeof(Matrix64)==64&&std::is_trivially_copyable<Matrix64>::value);
namespace detail {
inline ik::Matrix unpack(Matrix64 const& packed) noexcept {
    ik::Matrix matrix{};for(unsigned j=0;j<16;++j)matrix.m[j]=packed.values[j];return matrix;
}
inline bool rigid(ik::Matrix const& matrix) noexcept {
    if(!ik::detail::similarity(matrix))return false;
    for(unsigned j=0;j<3;++j)if(std::abs(ik::detail::length(ik::detail::row(matrix,j))-1)>2e-5)return false;
    return std::abs(ik::detail::dot(ik::detail::row(matrix,0),ik::detail::row(matrix,1)))<=2e-5&&
        std::abs(ik::detail::dot(ik::detail::row(matrix,0),ik::detail::row(matrix,2)))<=2e-5&&
        std::abs(ik::detail::dot(ik::detail::row(matrix,1),ik::detail::row(matrix,2)))<=2e-5;
}
inline bool single_rigid(ik::Matrix const& matrix) noexcept {
    // Actual SingleBone output can carry float accumulation in m[15]. Permit
    // only this bounded affine-tail error for rotation replacement. All other
    // rigid checks stay unchanged, including exact zero projection entries.
    constexpr double tail_tolerance=2.0*std::numeric_limits<float>::epsilon();
    if(!std::isfinite(matrix.m[15])||std::abs(matrix.m[15]-1.0)>tail_tolerance)return false;
    auto validation=matrix;validation.m[15]=1.0;
    return rigid(validation); // Validation copy only; never normalize output.
}
inline bool unit(Quaternion q,Quaternion& out) noexcept {
    const double n=std::hypot(std::hypot(q.x,q.y),std::hypot(q.z,q.w));
    return std::isfinite(n)&&std::abs(n-1)<=1e-6&&ik::detail::normalized(q,out);
}
inline bool transform(Matrix64 const& packed,Workspace& work) noexcept {
    work.to_world=unpack(packed);
    return rigid(work.to_world)&&ik::world_pose(work.to_world,work.transform)==ik::Status::okay;
}
inline bool rotation(Quaternion world,Workspace const& work,Matrix64& out) noexcept {
    Quaternion q{};if(!unit(world,q))return false;const auto c=work.transform.orientation;
    q=ik::detail::product({-c.x,-c.y,-c.z,c.w},q);
    ik::Atom atom{};atom.scale=1;if(!ik::detail::set_quat(atom,q))return false;
    const auto m=ik::detail::atom_matrix(atom);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)out.values[i*4+j]=static_cast<float>(m.m[i*4+j]);
    return true;
}
inline bool encode(Pose const& pose,Workspace const& work,Matrix64& out) noexcept {
    // Exact 3x3 inverse of the already rigid-certified float basis avoids
    // treating float roundoff as perfectly orthogonal during position return.
    const auto a=ik::detail::row(work.to_world,0),b=ik::detail::row(work.to_world,1),c=ik::detail::row(work.to_world,2);
    const auto bc=ik::detail::cross(b,c),ca=ik::detail::cross(c,a),ab=ik::detail::cross(a,b);
    const double determinant=ik::detail::dot(a,bc);
    const auto delta=ik::detail::sub(pose.position,work.transform.position);
    const double coordinates[]={ik::detail::dot(delta,bc)/determinant,ik::detail::dot(delta,ca)/determinant,ik::detail::dot(delta,ab)/determinant};
    out={};out.values[15]=1;
    for(unsigned j=0;j<3;++j){out.values[12+j]=static_cast<float>(coordinates[j]);if(!std::isfinite(out.values[12+j]))return false;}
    return rotation(pose.orientation,work,out)&&rigid(unpack(out));
}
inline Status status(arm::Status s) noexcept {
    switch(s){case arm::Status::solved:return Status::solved;case arm::Status::unreachable:return Status::unreachable;
        case arm::Status::degenerate_bone:return Status::degenerate;case arm::Status::invalid_policy:return Status::invalid_policy;
        case arm::Status::invalid_input:case arm::Status::invalid_quaternion:return Status::invalid_target;default:return Status::numerical_failure;}
}
} // namespace detail

inline Status solve(LimbInput const& input,Workspace& work,Matrix64* output) noexcept {
    const ik::detail::Range ranges[]={ik::detail::range(&input,sizeof(input)),ik::detail::range(&work,sizeof(work)),ik::detail::range(output,3*sizeof(Matrix64))};
    if(!ik::detail::disjoint(ranges,3))return Status::alias;
    if(!detail::transform(input.component_to_world,work))return Status::invalid_matrix;
    Quaternion target{};if(!ik::detail::finite_vector(input.wrist_world.position)||!detail::unit(input.wrist_world.orientation,target))return Status::invalid_target;
    for(unsigned i=0;i<3;++i){work.component[i]=detail::unpack(input.component[i]);if(!detail::rigid(work.component[i]))return Status::invalid_matrix;
        work.world[i]=ik::detail::multiply(work.component[i],work.to_world);
        if(ik::world_pose(work.world[i],work.world_pose[i])!=ik::Status::okay)return Status::invalid_matrix;}
    work.input={work.world_pose[0],work.world_pose[1],work.world_pose[2],{input.wrist_world.position,target},input.authored,input.previous,{input.mode,1,1}};
    const auto result=arm::solve(work.input,work.solution);if(result!=arm::Status::solved)return detail::status(result);
    for(unsigned i=0;i<3;++i){const Pose& pose=i==0?work.solution.shoulder:i==1?work.solution.elbow:work.solution.wrist;
        if(!detail::encode(pose,work,work.candidate[i]))return Status::numerical_failure;}
    // The solver never moves the shoulder; keep its component position bits.
    std::memcpy(work.candidate[0].values+12,input.component[0].values+12,3*sizeof(float));
    std::memcpy(output,work.candidate,3*sizeof(Matrix64));return Status::solved;
}

inline Status replace_wrist_rotation(SingleInput const& input,Workspace& work,Matrix64* output) noexcept {
    const ik::detail::Range ranges[]={ik::detail::range(&input,sizeof(input)),ik::detail::range(&work,sizeof(work)),ik::detail::range(output,sizeof(Matrix64))};
    if(!ik::detail::disjoint(ranges,3))return Status::alias;
    if(!detail::transform(input.component_to_world,work)||!detail::single_rigid(detail::unpack(input.component)))return Status::invalid_matrix;
    Quaternion q{};if(!detail::unit(input.world_orientation,q))return Status::invalid_target;
    work.candidate[0]=input.component;
    if(!detail::rotation(q,work,work.candidate[0])||!detail::single_rigid(detail::unpack(work.candidate[0])))return Status::numerical_failure;
    *output=work.candidate[0];return Status::solved;
}
} // namespace gears_cover_control_output_pose
