#pragma once
#include "../vr-common/hand_personal_calibration.h"
#include <cstring>

// Caller-owned canonical bytes only; no live field access or digest engine.
// The caller separately hashes THESE SAME bytes and binds the current asset.
// Known mount/signature fields are data, not proof that this buffer is trusted.
namespace gears_hand_personal_reference {
namespace mount=gears_hand_authored_mount;
namespace personal=gears_hand_personal;
namespace detail {
inline bool record(mount::CanonicalGeometry const& geometry,std::size_t i,
    gears_vr::Pose& pose,std::int32_t& parent) noexcept {
    if(i>=115||sizeof(float)!=4||!std::numeric_limits<float>::is_iec559)return false;
    const std::uint32_t endian=1;if(*reinterpret_cast<unsigned char const*>(&endian)!=1)return false;
    // This is canonical_geometry()'s explicit index/parent/xyzw/T format,
    // not a native object layout. Preserve source floats; normalize only math.
    const auto* bytes=geometry.data()+i*36;std::uint32_t index{};float qt[7]{};
    std::memcpy(&index,bytes,4);std::memcpy(&parent,bytes+4,4);std::memcpy(qt,bytes+8,28);
    if(index!=i||(!i&&parent!=0)||(i&&(parent<0||static_cast<std::size_t>(parent)>=i)))return false;
    double norm{};
    for(unsigned j=0;j<7;++j){if(!std::isfinite(qt[j]))return false;if(j<4)norm+=double(qt[j])*qt[j];}
    if(norm<.98||norm>1.02)return false;
    gears_vr::Quaternion q{};
    if(!gears_hand_wrist::detail::unit({qt[0],qt[1],qt[2],qt[3]},q))return false;
    pose={{qt[4],qt[5],qt[6]},q};return true;
}
inline bool position(mount::CanonicalGeometry const& geometry,std::size_t bone,gears_vr::Vector& out) noexcept {
    gears_vr::Pose pose{};std::int32_t parent{};
    if(!record(geometry,bone,pose,parent))return false;
    // Standard Hamilton: qComponent=qParent*qLocal. Equivalent Native row
    // matrices use Local*Parent. Walk upward, with bounded constant scratch;
    // do not place 115 matrices or recursively nested poses on a hook stack.
    for(std::size_t steps=0;bone&&steps<115;++steps){
        bone=static_cast<std::size_t>(parent);gears_vr::Pose ancestor{};
        if(!record(geometry,bone,ancestor,parent))return false;
        const auto offset=gears_vr::transform(gears_vr::rotation_matrix(ancestor.orientation),pose.position);
        pose.position={ancestor.position.x+offset.x,ancestor.position.y+offset.y,ancestor.position.z+offset.z};
        if(!gears_vr::finite(pose.position)||
           !gears_hand_wrist::detail::unit(gears_vr::multiply(ancestor.orientation,pose.orientation),pose.orientation))return false;
    }
    if(bone)return false;
    out=pose.position;return true;
}
inline bool same_mount(mount::Mount const& value,mount::Side side,mount::Mount const& expected) noexcept {
    const auto& a=value.calibration;const auto& b=expected.calibration;
    return value.model==expected.model&&value.side==side&&value.provenance==expected.provenance&&
        value.input_space==expected.input_space&&value.geometry_signature==expected.geometry_signature&&
        value.bone_count==expected.bone_count&&value.hand_bone==expected.hand_bone&&value.chain==expected.chain&&
        value.model_to_world_scale==expected.model_to_world_scale&&value.units_per_metre==expected.units_per_metre&&
        a.known&&a.grip_to_wrist_metres.x==b.grip_to_wrist_metres.x&&a.grip_to_wrist_metres.y==b.grip_to_wrist_metres.y&&
        a.grip_to_wrist_metres.z==b.grip_to_wrist_metres.z&&a.grip_to_wrist_rotation.x==b.grip_to_wrist_rotation.x&&
        a.grip_to_wrist_rotation.y==b.grip_to_wrist_rotation.y&&a.grip_to_wrist_rotation.z==b.grip_to_wrist_rotation.z&&
        a.grip_to_wrist_rotation.w==b.grip_to_wrist_rotation.w;
}
inline double distance(gears_vr::Vector a,gears_vr::Vector b) noexcept {
    return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);
}
} // namespace detail

// Marcus115 reference template only: roots 8/32, chains 8->10->11 and
// 32->34->35. Full reference hierarchy above each root is composed as well.
// Raw reference lengths become metres by model_scale/units exactly once.
// Actual geometry signature/asset and live safety are caller responsibilities.
// All failures preserve out, including padding; out must not alias any input.
inline personal::Status dimensions(mount::CanonicalGeometry const& geometry,mount::Mount const& left,
    mount::Mount const& right,personal::Dimensions& out) noexcept {
    using personal::detail::disjoint;
    if(!disjoint(&geometry,sizeof(geometry),&out,sizeof(out))||!disjoint(&left,sizeof(left),&out,sizeof(out))||
       !disjoint(&right,sizeof(right),&out,sizeof(out)))return personal::Status::alias;
    mount::Mount expected_left{},expected_right{};
    if(!mount::marcus115(left.geometry_signature,mount::Side::left,left.model_to_world_scale,left.units_per_metre,expected_left)||
       !mount::marcus115(right.geometry_signature,mount::Side::right,right.model_to_world_scale,right.units_per_metre,expected_right)||
       !detail::same_mount(left,mount::Side::left,expected_left)||!detail::same_mount(right,mount::Side::right,expected_right)||
       left.model_to_world_scale!=right.model_to_world_scale||left.units_per_metre!=right.units_per_metre)
        return personal::Status::invalid_model;
    // Validate the complete bounded buffer, not only the requested endpoints.
    gears_vr::Pose temporary{};std::int32_t parent{};
    for(std::size_t i=0;i<115;++i)if(!detail::record(geometry,i,temporary,parent))return personal::Status::invalid_geometry;
    const std::size_t chains[2][3]={{8,10,11},{32,34,35}};
    for(unsigned side=0;side<2;++side){
        for(unsigned j=1;j<3;++j){
            if(!detail::record(geometry,chains[side][j],temporary,parent)||
               parent!=static_cast<std::int32_t>(chains[side][j-1]))return personal::Status::invalid_geometry;
        }
    }
    gears_vr::Vector points[2][3]{};
    for(unsigned side=0;side<2;++side)for(unsigned j=0;j<3;++j)
        if(!detail::position(geometry,chains[side][j],points[side][j]))return personal::Status::numerical_failure;
    personal::Dimensions candidate{};
    candidate.key={left.model,left.geometry_signature,left.bone_count,left.model_to_world_scale,left.units_per_metre};
    const double factor=left.model_to_world_scale/left.units_per_metre;
    candidate.shoulder_span_metres=detail::distance(points[0][0],points[1][0])*factor;
    for(unsigned side=0;side<2;++side){
        candidate.upper_metres[side]=detail::distance(points[side][0],points[side][1])*factor;
        candidate.lower_metres[side]=detail::distance(points[side][1],points[side][2])*factor;
        const auto& offset=(side?right:left).calibration.grip_to_wrist_metres;
        candidate.authored_controller_extension_metres[side]=std::hypot(std::hypot(offset.x,offset.y),offset.z);
        if(!personal::detail::positive(candidate.upper_metres[side])||!personal::detail::positive(candidate.lower_metres[side])||
           !personal::detail::positive(candidate.authored_controller_extension_metres[side]))return personal::Status::numerical_failure;
    }
    if(!personal::detail::positive(candidate.shoulder_span_metres))return personal::Status::numerical_failure;
    out=candidate;return personal::Status::okay;
}
} // namespace gears_hand_personal_reference
